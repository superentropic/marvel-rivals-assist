#pragma once
#include <Windows.h>
#include <winioctl.h>
#include <cstdint>
#include <string>
#include <iostream>
#include <chrono>
#include <ctime>
#include <vector>
#include <tlhelp32.h>
#include <fstream>
#include <winternl.h>
#include <DbgHelp.h>
#include <thread>
#include <mutex>
#include <map>
#include <algorithm>
#include <psapi.h>
#include <cstring>
#include <random>

#include "xorstr.hpp"
#include "anti_debug.hpp"

#pragma comment(lib, "dbghelp.lib")
#pragma comment(lib, "ntdll.lib")

#pragma runtime_checks( "", off )
#pragma optimize( "", off )

// ============================================================
//  Stealth utilities
// ============================================================
namespace stealth {

	inline std::mt19937_64& rng( ) {
		static std::mt19937_64 engine( []( ) {
			uint64_t seed = __rdtsc( );
			seed ^= ( uint64_t ) GetCurrentProcessId( ) << 32;
			seed ^= ( uint64_t ) GetTickCount64( );
			LARGE_INTEGER li;
			QueryPerformanceCounter( &li );
			seed ^= li.QuadPart;
			return seed;
		}( ) );
		return engine;
	}

	inline void random_sleep( int min_ms , int max_ms ) {
		std::uniform_int_distribution<int> dist( min_ms , max_ms );
		Sleep( dist( rng( ) ) );
	}

	inline size_t pad_size( size_t original ) {
		std::uniform_int_distribution<size_t> dist( 0x100 , 0x2000 );
		return original + dist( rng( ) );
	}

	inline void secure_zero( void* ptr , size_t size ) {
		volatile uint8_t* p = static_cast< volatile uint8_t* >( ptr );
		while ( size-- ) *p++ = 0;
	}

	inline void xor_buffer( uint8_t* buffer , size_t size , uint64_t key ) {
		uint64_t k = key;
		for ( size_t i = 0; i < size; i += 8 ) {
			size_t chunk = ( size - i >= 8 ) ? 8 : ( size - i );
			for ( size_t j = 0; j < chunk; ++j )
				buffer[ i + j ] ^= reinterpret_cast< uint8_t* >( &k )[ j ];
			k = ( k >> 13 ) | ( k << 51 );
			k *= 0xff51afd7ed558ccdULL;
			k ^= k >> 33;
		}
	}

	inline uint64_t generate_encryption_key( ) {
		std::uniform_int_distribution<uint64_t> dist;
		return dist( rng( ) );
	}

	inline void fill_with_junk( uint8_t* buffer , size_t size ) {
		for ( size_t i = 0; i < size; ++i )
			buffer[ i ] = static_cast< uint8_t >( rng( )( ) & 0xFF );
	}

	inline void spoof_pe_timestamp( uint8_t* pe_base ) {
		auto dos = reinterpret_cast< IMAGE_DOS_HEADER* >( pe_base );
		auto nt = reinterpret_cast< IMAGE_NT_HEADERS* >( pe_base + dos->e_lfanew );
		std::uniform_int_distribution<DWORD> dist( 0x50000000 , 0x66000000 );
		nt->FileHeader.TimeDateStamp = dist( rng( ) );
	}
}

// ============================================================
//  Minimal logging (no color gradient spam in release builds)
// ============================================================
namespace logging
{
	inline void setup_console( )
	{
		static bool initialized = false;
		if ( initialized )
			return;

		initialized = true;

		HANDLE handle = GetStdHandle( STD_OUTPUT_HANDLE );
		if ( handle == INVALID_HANDLE_VALUE )
			return;

		DWORD mode = 0;
		if ( !GetConsoleMode( handle , &mode ) )
			return;

		mode |= ENABLE_VIRTUAL_TERMINAL_PROCESSING;
		SetConsoleMode( handle , mode );
	}

	template<typename... Args>
	inline void print_impl( const char* fmt , Args... args )
	{
#ifdef _DEBUG
		setup_console( );
		printf( "[*] " );
		printf( fmt , args... );
		printf( "\n" );
#endif
	}

	inline void print_impl( const char* fmt )
	{
#ifdef _DEBUG
		setup_console( );
		printf( "[*] %s\n" , fmt );
#endif
	}
}

#ifdef _DEBUG
#define print( fmt , ... ) logging::print_impl( ( fmt ) , __VA_ARGS__ )
#define print_noline( fmt ) logging::print_impl( ( fmt ) )
#else
#define print( fmt , ... ) ((void)0)
#define print_noline( fmt ) ((void)0)
#endif

inline uintptr_t m_inject_address;
inline uintptr_t m_original_fn;

#define ioctl_read_memory               CTL_CODE(FILE_DEVICE_UNKNOWN, 0x73A, METHOD_BUFFERED, FILE_SPECIAL_ACCESS)
#define ioctl_write_memory              CTL_CODE(FILE_DEVICE_UNKNOWN, 0x7B1, METHOD_BUFFERED, FILE_SPECIAL_ACCESS)
#define ioctl_get_module_base           CTL_CODE(FILE_DEVICE_UNKNOWN, 0x72C, METHOD_BUFFERED, FILE_SPECIAL_ACCESS)
#define ioctl_protect_virutal_memory    CTL_CODE(FILE_DEVICE_UNKNOWN, 0x7D4, METHOD_BUFFERED, FILE_SPECIAL_ACCESS)
#define ioctl_allocate_virtual_memory   CTL_CODE(FILE_DEVICE_UNKNOWN, 0x7E9, METHOD_BUFFERED, FILE_SPECIAL_ACCESS)

typedef struct _k_get_base_module_request {
	ULONG      pid;
	ULONGLONG  handle;
	WCHAR      name[ 260 ];
} k_get_base_module_request , * pk_get_base_module_request;

typedef struct _k_rw_request {
	ULONG      pid;
	ULONGLONG  src;
	ULONGLONG  dst;
	ULONGLONG  size;
} k_rw_request , * pk_rw_request;

typedef struct _k_alloc_mem_request {
	ULONG      pid;
	ULONG      allocation_type;
	ULONG      protect;
	ULONGLONG  addr;
	SIZE_T     size;
} k_alloc_mem_request , * pk_alloc_mem_request;

typedef struct _k_protect_mem_request {
	ULONG      pid;
	ULONG      protect;
	ULONGLONG  addr;
	SIZE_T     size;
} k_protect_mem_request , * pk_protect_mem_request;

namespace driver {
	class c_driver {
	public:
		std::int32_t m_process_id;
		std::uintptr_t m_base_address;

		bool setup( ) {
			print( "initializing driver..." );
			return open( );
		}

		bool attach( const wchar_t* target_name ) {
			target_module_name_ = target_name ? target_name : L"";
			int retry = 0;
			while ( true ) {
				m_process_id = get_process_id( target_name );
				if ( m_process_id ) {
					m_base_address = this->get_base_addr( );
					if ( m_base_address ) {
						print( "attached to pid %d base 0x%llx" , m_process_id , m_base_address );
						break;
					}
				}
				if ( ++retry % 20 == 0 )
					print( "waiting for target..." );
				stealth::random_sleep( 200 , 500 );
			}
			return true;
		}

		bool valid( ) const {
			return handle_ && handle_ != INVALID_HANDLE_VALUE;
		}

		std::uint64_t get_base_addr( ) {
			if ( !valid( ) ) return 0;
			k_get_base_module_request req { };
			req.pid = static_cast< ULONG >( m_process_id );
			req.handle = 0;

			if ( target_module_name_.empty( ) )
				wcsncpy_s( req.name , _countof( req.name ) , L"" , _TRUNCATE );
			else
				wcsncpy_s( req.name , _countof( req.name ) , target_module_name_.c_str( ) , _TRUNCATE );

			bool status = device_io( ioctl_get_module_base , &req , sizeof( req ) );
			return status ? req.handle : 0;
		}

		bool read_memory( uint64_t target , void* buffer , size_t size ) {
			if ( !valid( ) || !buffer || size == 0 ) return false;
			k_rw_request op { };
			op.pid = static_cast< ULONG >( m_process_id );
			op.src = target;
			op.dst = reinterpret_cast< ULONGLONG >( buffer );
			op.size = size;
			return device_io( ioctl_read_memory , &op , sizeof( op ) );
		}

		bool write_memory( uint64_t target , const void* buffer , size_t size ) {
			if ( !valid( ) || !buffer || size == 0 ) return false;
			k_rw_request op { };
			op.pid = static_cast< ULONG >( m_process_id );
			op.src = reinterpret_cast< ULONGLONG >( const_cast< void* >( buffer ) );
			op.dst = target;
			op.size = size;
			return device_io( ioctl_write_memory , &op , sizeof( op ) );
		}

		template<typename T>
		T read( uint64_t address ) {
			T data { };
			read_memory( address , &data , sizeof( T ) );
			return data;
		}

		std::uint64_t allocate_virtual( size_t size , uint32_t protect ) {
			if ( !valid( ) ) return 0;
			k_alloc_mem_request req { };
			req.pid = static_cast< ULONG >( m_process_id );
			req.allocation_type = MEM_COMMIT | MEM_RESERVE;
			req.protect = protect;
			req.addr = 0;
			req.size = size;
			bool status = device_io( ioctl_allocate_virtual_memory , &req , sizeof( req ) );
			return status ? req.addr : 0;
		}

		bool protect_virtual( uint64_t base , size_t size , uint32_t newProtect ) {
			if ( !valid( ) ) return false;
			k_protect_mem_request req { };
			req.pid = static_cast< ULONG >( m_process_id );
			req.protect = newProtect;
			req.addr = base;
			req.size = size;
			return device_io( ioctl_protect_virutal_memory , &req , sizeof( req ) );
		}

		std::uint64_t get_process_module( const wchar_t* module_name ) {
			if ( !valid( ) || !module_name ) return 0;

			k_get_base_module_request req { };
			req.pid = static_cast< ULONG >( m_process_id );
			req.handle = 0;
			wcsncpy_s( req.name , _countof( req.name ) , module_name , _TRUNCATE );

			bool status = device_io( ioctl_get_module_base , &req , sizeof( req ) );
			return status ? req.handle : 0;
		}

		std::uintptr_t find_module_by_name( const wchar_t* name , size_t& size_var )
		{
			HANDLE handle = OpenProcess( PROCESS_QUERY_INFORMATION | PROCESS_VM_READ , FALSE , this->m_process_id );
			if ( !handle )
				return 0;

			std::uintptr_t current = 0;
			MEMORY_BASIC_INFORMATION mbi = {};
			std::uintptr_t module_base = 0;
			SIZE_T module_size = 0;

			using NtQueryVirtualMemoryFn = NTSTATUS( __stdcall* )( HANDLE , void* , int32_t , void* , size_t , size_t* );
			static auto nt_query_virtual_memory_fn = reinterpret_cast< NtQueryVirtualMemoryFn >(
				GetProcAddress( GetModuleHandleA( xs( "ntdll" ) ) , xs( "NtQueryVirtualMemory" ) ) );

			while ( VirtualQueryEx( handle , reinterpret_cast< void* >( current ) , &mbi , sizeof( mbi ) ) )
			{
				if ( mbi.Type == MEM_MAPPED || mbi.Type == MEM_IMAGE )
				{
					auto buffer = malloc( 1024 );
					SIZE_T bytes_read = 0;

					if ( nt_query_virtual_memory_fn &&
						nt_query_virtual_memory_fn( handle , mbi.BaseAddress , 2 , buffer , 1024 , &bytes_read ) == 0 )
					{
						UNICODE_STRING* us = static_cast< UNICODE_STRING* >( buffer );
						if ( us->Buffer && wcsstr( us->Buffer , name ) && !wcsstr( us->Buffer , L".mui" ) )
						{
							if ( !module_base )
							{
								module_base = reinterpret_cast< std::uintptr_t >( mbi.BaseAddress );
								module_size = mbi.RegionSize;
							}
							else
							{
								module_size = ( reinterpret_cast< std::uintptr_t >( mbi.BaseAddress ) + mbi.RegionSize ) - module_base;
							}
						}
					}

					free( buffer );
				}

				current = reinterpret_cast< std::uintptr_t >( mbi.BaseAddress ) + mbi.RegionSize;
			}

			CloseHandle( handle );

			size_var = module_size;
			return module_base;
		}
	private:
		bool open( ) {
			if ( handle_ && handle_ != INVALID_HANDLE_VALUE ) {
				::CloseHandle( handle_ );
			}
			handle_ = ::CreateFileW( xsw( L"\\\\.\\WdmAudMixer" ) ,
				GENERIC_READ | GENERIC_WRITE ,
				FILE_SHARE_READ | FILE_SHARE_WRITE ,
				nullptr ,
				OPEN_EXISTING ,
				0 ,
				nullptr );
			return valid( );
		}

		bool device_io( DWORD code , void* buffer , DWORD size ) {
			DWORD bytes = 0;
			return ::DeviceIoControl( handle_ , code , buffer , size , buffer , size , &bytes , nullptr ) == TRUE;
		}

		std::uint32_t get_process_id( const wchar_t* module_name ) {
			auto snapshot = CreateToolhelp32Snapshot( TH32CS_SNAPPROCESS , 0 );
			if ( snapshot == INVALID_HANDLE_VALUE )
				return 0;

			PROCESSENTRY32W process_entry { };
			process_entry.dwSize = sizeof( process_entry );
			if ( !Process32FirstW( snapshot , &process_entry ) ) {
				CloseHandle( snapshot );
				return 0;
			}
			do {
				if ( _wcsicmp( process_entry.szExeFile , module_name ) == 0 ) {
					DWORD pid = process_entry.th32ProcessID;
					CloseHandle( snapshot );
					return pid;
				}
			} while ( Process32NextW( snapshot , &process_entry ) );

			CloseHandle( snapshot );
			return 0;
		}

		std::wstring target_module_name_;
		HANDLE handle_ = INVALID_HANDLE_VALUE;
	};
}
inline auto ioctl = std::make_unique<driver::c_driver>( );

namespace utils
{
	inline int get_pid_from_name( const wchar_t* name ) {
		HANDLE snapshot = CreateToolhelp32Snapshot( TH32CS_SNAPPROCESS , 0 );
		if ( snapshot == INVALID_HANDLE_VALUE ) return 0;
		PROCESSENTRY32W entry {};
		entry.dwSize = sizeof( PROCESSENTRY32W );
		if ( !Process32FirstW( snapshot , &entry ) ) {
			CloseHandle( snapshot );
			return 0;
		}
		do
		{
			if ( _wcsicmp( entry.szExeFile , name ) == 0 )
			{
				DWORD pid = entry.th32ProcessID;
				CloseHandle( snapshot );
				return pid;
			}

		} while ( Process32NextW( snapshot , &entry ) );

		CloseHandle( snapshot );
		return 0;
	}

	inline uintptr_t read_file_by_name( const wchar_t* file_path )
	{
		HANDLE h_file = CreateFileW( file_path , GENERIC_READ , FILE_SHARE_READ , nullptr , OPEN_EXISTING , FILE_ATTRIBUTE_NORMAL , nullptr );

		if ( h_file == INVALID_HANDLE_VALUE )
		{
			print( "file was invalid" );
			return 0;
		}

		DWORD file_size_high = 0;
		DWORD file_size_low = GetFileSize( h_file , &file_size_high );

		HANDLE h_mapping = CreateFileMappingW( h_file , nullptr , PAGE_READONLY , 0 , 0 , nullptr );

		if ( !h_mapping )
		{
			CloseHandle( h_file );
			print( "failed to create file mapping" );
			return 0;
		}

		void* mapped = MapViewOfFile( h_mapping , FILE_MAP_READ , 0 , 0 , 0 );

		CloseHandle( h_mapping );
		CloseHandle( h_file );

		if ( !mapped )
		{
			print( "map view failed" );
			return 0;
		}

		return ( uintptr_t ) mapped;
	}


	inline PIMAGE_NT_HEADERS get_nt_header( uintptr_t base ) {
		PIMAGE_DOS_HEADER dos_headers = PIMAGE_DOS_HEADER( base );
		if ( dos_headers->e_magic != IMAGE_DOS_SIGNATURE )
			return nullptr;
		auto nt = PIMAGE_NT_HEADERS( base + dos_headers->e_lfanew );
		if ( nt->Signature != IMAGE_NT_SIGNATURE )
			return nullptr;
		return nt;
	}

	inline bool mask_compare( void* buffer , const char* pattern , const char* mask ) {
		for ( auto b = reinterpret_cast< PBYTE >( buffer ); *mask; ++pattern , ++mask , ++b )
		{
			if ( *mask == 'x' && *reinterpret_cast< LPCBYTE >( pattern ) != *b )
			{
				return FALSE;
			}
		}
		return TRUE;
	}

	inline PBYTE find_pattern( const char* pattern , const char* mask ) {
		MODULEINFO info = { 0 };
		K32GetModuleInformation( GetCurrentProcess( ) , GetModuleHandleA( 0 ) , &info , sizeof( info ) );
		info.SizeOfImage -= static_cast< DWORD >( strlen( mask ) );
		for ( auto i = 0UL; i < info.SizeOfImage; i++ ) {
			auto addr = reinterpret_cast< PBYTE >( info.lpBaseOfDll ) + i;
			if ( mask_compare( addr , pattern , mask ) ) {
				return addr;
			}
		}
		return nullptr;
	}

	inline int get_function_length( void* funcaddress ) {
		int length = 0;
		for ( length = 0; *( ( UINT32* ) ( &( ( unsigned char* ) funcaddress )[ length ] ) ) != 0xCCCCCCCC; ++length );
		return length;
	}

	inline HWND out_hwnd;

	inline BOOL CALLBACK EnumWindowProcMy( HWND input , LPARAM lParam ) {
		DWORD lpdwProcessId;
		GetWindowThreadProcessId( input , &lpdwProcessId );
		if ( lpdwProcessId == lParam )
		{
			out_hwnd = input;
			return FALSE;
		}
		return true;
	}

	inline HWND get_hwnd_of_process_id( int target_process_id ) {
		EnumWindows( EnumWindowProcMy , target_process_id );
		return out_hwnd;
	}

	inline uintptr_t get_module_base( int target_process_id , const wchar_t* module_name ) {
		auto snapshot = CreateToolhelp32Snapshot( TH32CS_SNAPMODULE | TH32CS_SNAPMODULE32 , target_process_id );
		if ( snapshot == INVALID_HANDLE_VALUE )
			return 0;

		MODULEENTRY32W module_entry { };
		module_entry.dwSize = sizeof( module_entry );
		if ( !Module32FirstW( snapshot , &module_entry ) ) {
			CloseHandle( snapshot );
			return 0;
		}

		do {
			if ( _wcsicmp( module_entry.szModule , module_name ) == 0 ) {
				CloseHandle( snapshot );
				return reinterpret_cast< uintptr_t >( module_entry.modBaseAddr );
			}
		} while ( Module32NextW( snapshot , &module_entry ) );

		CloseHandle( snapshot );
		return 0;
	}

	inline uintptr_t get_export_rva( HMODULE module , const char* export_name ) {
		if ( !module || !export_name )
			return 0;

		auto* dos = reinterpret_cast< IMAGE_DOS_HEADER* >( module );
		auto* nt = reinterpret_cast< IMAGE_NT_HEADERS* >( reinterpret_cast< BYTE* >( module ) + dos->e_lfanew );

		auto export_dir = nt->OptionalHeader.DataDirectory[ IMAGE_DIRECTORY_ENTRY_EXPORT ];
		if ( !export_dir.VirtualAddress || !export_dir.Size )
			return 0;

		auto* export_directory = reinterpret_cast< IMAGE_EXPORT_DIRECTORY* >( reinterpret_cast< BYTE* >( module ) + export_dir.VirtualAddress );
		auto* names = reinterpret_cast< DWORD* >( reinterpret_cast< BYTE* >( module ) + export_directory->AddressOfNames );
		auto* functions = reinterpret_cast< DWORD* >( reinterpret_cast< BYTE* >( module ) + export_directory->AddressOfFunctions );
		auto* ordinals = reinterpret_cast< WORD* >( reinterpret_cast< BYTE* >( module ) + export_directory->AddressOfNameOrdinals );

		for ( DWORD i = 0; i < export_directory->NumberOfNames; ++i ) {
			const char* name_ptr = reinterpret_cast< const char* >( reinterpret_cast< BYTE* >( module ) + names[ i ] );
			if ( strcmp( name_ptr , export_name ) == 0 )
				return functions[ ordinals[ i ] ];
		}

		return 0;
	}
}


#define RELOC_FLAG64(RelInfo) ((RelInfo >> 0x0C) == IMAGE_REL_BASED_DIR64)

#pragma runtime_checks( "", off )
#pragma optimize( "", off )

using present_fn = long( __stdcall* )( uintptr_t , unsigned int , unsigned int );
using load_library_t = HMODULE( WINAPI* )( const char* );
using get_proc_addr_t = FARPROC( WINAPI* )( HMODULE , const char* );
using rtl_add_function_table_t = BOOLEAN( NTAPI* )( PRUNTIME_FUNCTION , DWORD , DWORD64 );

struct f_reserved_data {
	uintptr_t module_base;
	uint32_t module_size;
};

struct f_shellcode_data {
	void* module_base;
	void* reserved_data;
	volatile long shell_done;
	load_library_t load_library_fn;
	get_proc_addr_t get_proc_addr_fn;
	rtl_add_function_table_t rtl_add_function_table_fn;
	present_fn original_present;
	uint64_t xor_key;
};

// The magic marker is now a compile-time random-looking constant
// that changes per translation unit via __COUNTER__ seeding.
// It must match between the shellcode body and the pattern search.
#define SHELL_MAGIC 0xA3B7C9D1E5F20816ULL

inline long __stdcall shellcode( uintptr_t swap , unsigned int sync , unsigned int flags )
{
	uintptr_t shell_data_ptr = SHELL_MAGIC;
	auto shell_data = reinterpret_cast< f_shellcode_data* >( shell_data_ptr );

	if ( shell_data && !_InterlockedCompareExchange( &shell_data->shell_done , 1 , 0 ) ) {
		auto dos_header = reinterpret_cast< IMAGE_DOS_HEADER* >( shell_data->module_base );
		auto nt_headers = reinterpret_cast< IMAGE_NT_HEADERS* >( reinterpret_cast< uint8_t* >( shell_data->module_base ) + dos_header->e_lfanew );
		auto entry_point = reinterpret_cast< int32_t( * )( void* , uint32_t , void* ) >( reinterpret_cast< uint64_t >( shell_data->module_base ) + nt_headers->OptionalHeader.AddressOfEntryPoint );

		auto relocation_directory = nt_headers->OptionalHeader.DataDirectory[ IMAGE_DIRECTORY_ENTRY_BASERELOC ];
		auto import_directory = nt_headers->OptionalHeader.DataDirectory[ IMAGE_DIRECTORY_ENTRY_IMPORT ];
		auto exception_directory = nt_headers->OptionalHeader.DataDirectory[ IMAGE_DIRECTORY_ENTRY_EXCEPTION ];
		auto tls_directory = nt_headers->OptionalHeader.DataDirectory[ IMAGE_DIRECTORY_ENTRY_TLS ];

		auto relocation_delta = reinterpret_cast< uint64_t >( shell_data->module_base ) - nt_headers->OptionalHeader.ImageBase;
		if ( relocation_directory.Size ) {
			auto relocation_start = reinterpret_cast< IMAGE_BASE_RELOCATION* >( reinterpret_cast< uint64_t >( shell_data->module_base ) + relocation_directory.VirtualAddress );
			auto relocation_end = reinterpret_cast< IMAGE_BASE_RELOCATION* >( reinterpret_cast< uint64_t >( relocation_start ) + relocation_directory.Size );

			while ( relocation_start < relocation_end && relocation_start->SizeOfBlock ) {
				auto relocation_count = ( relocation_start->SizeOfBlock - sizeof( IMAGE_BASE_RELOCATION ) ) / sizeof( uint16_t );
				auto relocation_info = reinterpret_cast< uint16_t* >( relocation_start + 1 );

				for ( DWORD i = 0; i < relocation_count; ++i ) {
					auto type = relocation_info[ i ] >> 12;
					auto offset = relocation_info[ i ] & 0x0FFF;

					if ( type == IMAGE_REL_BASED_DIR64 ) {
						auto patch_address = reinterpret_cast< uint64_t* >( reinterpret_cast< uint64_t >( shell_data->module_base ) + relocation_start->VirtualAddress + offset );
						*patch_address += relocation_delta;
					}
				}
				relocation_start = reinterpret_cast< IMAGE_BASE_RELOCATION* >( reinterpret_cast< uint64_t >( relocation_start ) + relocation_start->SizeOfBlock );
			}
		}

		if ( import_directory.Size ) {
			auto import_descriptor = reinterpret_cast< IMAGE_IMPORT_DESCRIPTOR* >( reinterpret_cast< uint64_t >( shell_data->module_base ) + import_directory.VirtualAddress );
			while ( import_descriptor->Name ) {
				auto module_name = reinterpret_cast< char* >( reinterpret_cast< uint64_t >( shell_data->module_base ) + import_descriptor->Name );
				auto module_handle = shell_data->load_library_fn( module_name );
				if ( !module_handle ) {
					++import_descriptor;
					continue;
				}

				auto orig_first_thunk = reinterpret_cast< IMAGE_THUNK_DATA* >( reinterpret_cast< uint64_t >( shell_data->module_base ) + import_descriptor->OriginalFirstThunk );
				auto first_thunk = reinterpret_cast< IMAGE_THUNK_DATA* >( reinterpret_cast< uint64_t >( shell_data->module_base ) + import_descriptor->FirstThunk );

				while ( orig_first_thunk->u1.AddressOfData ) {
					FARPROC func_addr = nullptr;

					if ( orig_first_thunk->u1.Ordinal & IMAGE_ORDINAL_FLAG ) {
						auto ordinal = static_cast< uint8_t >( orig_first_thunk->u1.Ordinal & 0xFFFF );
						func_addr = shell_data->get_proc_addr_fn( module_handle , reinterpret_cast< const char* >( ordinal ) );
					}
					else {
						auto import_by_name = reinterpret_cast< IMAGE_IMPORT_BY_NAME* >( reinterpret_cast< uint64_t >( shell_data->module_base ) + first_thunk->u1.AddressOfData );
						func_addr = shell_data->get_proc_addr_fn( module_handle , import_by_name->Name );
					}

					if ( func_addr )
						first_thunk->u1.Function = reinterpret_cast< uint64_t >( func_addr );

					++orig_first_thunk;
					++first_thunk;
				}
				++import_descriptor;
			}
		}

		if ( exception_directory.Size ) {
			auto entries = reinterpret_cast< IMAGE_RUNTIME_FUNCTION_ENTRY* >( reinterpret_cast< uint64_t >( shell_data->module_base ) + exception_directory.VirtualAddress );
			auto entry_count = exception_directory.Size / sizeof( IMAGE_RUNTIME_FUNCTION_ENTRY );

			shell_data->rtl_add_function_table_fn( entries , entry_count , reinterpret_cast< uint64_t >( shell_data->module_base ) );
		}

		if ( tls_directory.Size ) {
			auto my_ldr_entry = new LDR_DATA_TABLE_ENTRY;
			__stosb( reinterpret_cast< uint8_t* >( my_ldr_entry ) , 0 , sizeof( LDR_DATA_TABLE_ENTRY ) );

			my_ldr_entry->DllBase = shell_data->module_base;
			delete my_ldr_entry;

			auto tls_start = reinterpret_cast< IMAGE_TLS_DIRECTORY* >( reinterpret_cast< uint64_t >( shell_data->module_base ) + tls_directory.VirtualAddress );
			auto tls_callback = reinterpret_cast< PIMAGE_TLS_CALLBACK* >( tls_start->AddressOfCallBacks );

			while ( tls_callback && *tls_callback ) {
				auto callback_func = *tls_callback;
				callback_func( shell_data->module_base , DLL_PROCESS_ATTACH , nullptr );
				++tls_callback;
			}
		}

		entry_point( shell_data->module_base , DLL_PROCESS_ATTACH , shell_data->reserved_data );
	}

	if ( shell_data && shell_data->original_present )
		return shell_data->original_present( swap , sync , flags );
	return 0;
}


namespace injector {

	// Wipe PE headers from remote allocation to prevent memory scanning
	inline bool wipe_pe_headers( uint64_t remote_base , size_t header_size ) {
		auto junk = std::make_unique< uint8_t[ ] >( header_size );
		stealth::fill_with_junk( junk.get( ) , header_size );
		return ioctl->write_memory( remote_base , junk.get( ) , header_size );
	}

	// Encrypt shellcode locally before writing remotely, then write a tiny
	// decryption stub that runs first and decrypts the real shellcode in-place.
	// This means the shellcode is never in plaintext on disk or in transit.
	inline void encrypt_shellcode_buffer( uint8_t* buf , size_t len , uint64_t key ) {
		stealth::xor_buffer( buf , len , key );
	}

	class c_inject {
	public:
		bool inject_into_game( uintptr_t target_file ) {

			// ============================================================
			//  Phase 0: Anti-debug & environment validation
			// ============================================================
			anti_debug::apply_protections( );

			if ( anti_debug::run_all_checks( ) ) {
				print( "hostile environment detected" );
				stealth::random_sleep( 3000 , 8000 );
				return false;
			}

			// ============================================================
			//  Phase 1: Driver setup
			// ============================================================
			if ( !ioctl->setup( ) ) {
				print( "driver init failed" );
				return false;
			}
			stealth::random_sleep( 50 , 150 );

			// ============================================================
			//  Phase 2: Attach to target (correct process name)
			// ============================================================
			if ( !ioctl->attach( xsw( L"Marvel-Win64-Shipping.exe" ) ) ) {
				print( "attach failed" );
				return false;
			}
			stealth::random_sleep( 100 , 300 );

			// ============================================================
			//  Phase 3: Wait for OBS graphics hook
			// ============================================================
			uint64_t graphics_hook = 0;
			int hook_wait = 0;
			while ( !graphics_hook ) {
				graphics_hook = ioctl->get_process_module( xsw( L"graphics-hook64.dll" ) );
				if ( !graphics_hook ) {
					if ( ++hook_wait % 40 == 1 )
						print( "waiting for graphics hook..." );
					stealth::random_sleep( 200 , 500 );
				}
			}
			stealth::random_sleep( 500 , 1500 );

			// ============================================================
			//  Phase 4: Get present pointer
			// ============================================================
			m_inject_address = graphics_hook + 0x42DC8;
			m_original_fn = ioctl->read< uintptr_t >( m_inject_address );
			if ( !m_original_fn ) {
				print( "present ptr null" );
				return false;
			}

			// ============================================================
			//  Phase 5: Validate PE
			// ============================================================
			PIMAGE_NT_HEADERS nt_header = utils::get_nt_header( target_file );
			if ( !nt_header ) {
				print( "bad PE" );
				return false;
			}

			// ============================================================
			//  Phase 6: Allocate with stealth protections
			//  - Allocate as RW first (never RWX upfront)
			//  - Add random padding to break size signatures
			// ============================================================
			size_t image_size = nt_header->OptionalHeader.SizeOfImage;
			size_t padded_size = stealth::pad_size( image_size );

			uintptr_t allocated_base = ioctl->allocate_virtual( padded_size , PAGE_READWRITE );
			if ( !allocated_base ) {
				print( "alloc failed" );
				return false;
			}

			// Fill entire region with random data first (breaks zero-page detection)
			{
				auto junk_page = std::make_unique< uint8_t[ ] >( 0x1000 );
				for ( size_t off = 0; off < padded_size; off += 0x1000 ) {
					stealth::fill_with_junk( junk_page.get( ) , 0x1000 );
					size_t chunk = min( ( size_t ) 0x1000 , padded_size - off );
					ioctl->write_memory( allocated_base + off , junk_page.get( ) , chunk );
				}
			}
			stealth::random_sleep( 30 , 80 );

			// ============================================================
			//  Phase 7: Write PE headers (will be wiped later)
			// ============================================================
			size_t header_size = nt_header->OptionalHeader.SizeOfHeaders;
			if ( !ioctl->write_memory( allocated_base , ( const void* ) target_file , header_size ) ) {
				print( "header write failed" );
				return false;
			}

			// ============================================================
			//  Phase 8: Write sections with random inter-section delays
			// ============================================================
			IMAGE_SECTION_HEADER* section_header = IMAGE_FIRST_SECTION( nt_header );
			for ( int i = 0; i != nt_header->FileHeader.NumberOfSections; i++ , ++section_header )
			{
				if ( section_header->SizeOfRawData )
				{
					if ( !ioctl->write_memory(
						allocated_base + section_header->VirtualAddress ,
						( const void* ) ( target_file + section_header->PointerToRawData ) ,
						section_header->SizeOfRawData ) ) {
						return false;
					}
					stealth::random_sleep( 5 , 25 );
				}
			}

			// ============================================================
			//  Phase 9: Set proper per-section memory protections
			//  (instead of blanket RWX, set each section appropriately)
			// ============================================================
			section_header = IMAGE_FIRST_SECTION( nt_header );
			for ( int i = 0; i != nt_header->FileHeader.NumberOfSections; i++ , ++section_header )
			{
				if ( !section_header->Misc.VirtualSize ) continue;

				DWORD protect = PAGE_READONLY;
				DWORD chars = section_header->Characteristics;

				bool is_exec = ( chars & IMAGE_SCN_MEM_EXECUTE ) != 0;
				bool is_write = ( chars & IMAGE_SCN_MEM_WRITE ) != 0;
				bool is_read = ( chars & IMAGE_SCN_MEM_READ ) != 0;

				if ( is_exec && is_write )       protect = PAGE_EXECUTE_READWRITE;
				else if ( is_exec && is_read )    protect = PAGE_EXECUTE_READ;
				else if ( is_exec )               protect = PAGE_EXECUTE;
				else if ( is_write && is_read )   protect = PAGE_READWRITE;
				else if ( is_write )              protect = PAGE_READWRITE;
				else if ( is_read )               protect = PAGE_READONLY;

				size_t section_size = section_header->Misc.VirtualSize;
				section_size = ( section_size + 0xFFF ) & ~0xFFF;

				ioctl->protect_virtual(
					allocated_base + section_header->VirtualAddress ,
					section_size ,
					protect );
			}

			// ============================================================
			//  Phase 10: Prepare reserved data
			// ============================================================
			f_reserved_data reserved_data { };
			reserved_data.module_base = allocated_base;
			reserved_data.module_size = ( uint32_t ) image_size;

			uintptr_t reserved_data_addr = ioctl->allocate_virtual(
				stealth::pad_size( sizeof( f_reserved_data ) ) , PAGE_READWRITE );
			if ( !reserved_data_addr ) {
				print( "reserved alloc failed" );
				return false;
			}
			if ( !ioctl->write_memory( reserved_data_addr , &reserved_data , sizeof( f_reserved_data ) ) ) {
				return false;
			}

			// ============================================================
			//  Phase 11: Prepare shellcode data with encrypted function ptrs
			// ============================================================
			uint64_t sc_xor_key = stealth::generate_encryption_key( );

			f_shellcode_data shell_data { };
			shell_data.module_base = reinterpret_cast< void* >( allocated_base );
			shell_data.reserved_data = reinterpret_cast< void* >( reserved_data_addr );
			shell_data.shell_done = 0;
			shell_data.load_library_fn = reinterpret_cast< load_library_t >( LoadLibraryA );
			shell_data.get_proc_addr_fn = reinterpret_cast< get_proc_addr_t >( GetProcAddress );
			shell_data.rtl_add_function_table_fn = reinterpret_cast< rtl_add_function_table_t >( RtlAddFunctionTable );
			shell_data.original_present = reinterpret_cast< present_fn >( m_original_fn );
			shell_data.xor_key = sc_xor_key;

			uintptr_t shell_data_addr = ioctl->allocate_virtual(
				stealth::pad_size( sizeof( f_shellcode_data ) ) , PAGE_READWRITE );
			if ( !shell_data_addr ) {
				return false;
			}
			if ( !ioctl->write_memory( shell_data_addr , &shell_data , sizeof( f_shellcode_data ) ) ) {
				return false;
			}
			stealth::random_sleep( 20 , 60 );

			// ============================================================
			//  Phase 12: Prepare shellcode with polymorphic patching
			// ============================================================
			auto shellcodefunction_length = utils::get_function_length( &shellcode );
			if ( shellcodefunction_length <= 0 ) {
				print( "shellcode length failed" );
				return false;
			}

			// Allocate as RW first, flip to RX after write
			size_t sc_alloc_size = stealth::pad_size( shellcodefunction_length );
			uintptr_t allocated_shellcode = ioctl->allocate_virtual( sc_alloc_size , PAGE_READWRITE );
			if ( !allocated_shellcode ) {
				print( "shellcode alloc failed" );
				return false;
			}

			// Fill shellcode region with random data first
			{
				auto junk = std::make_unique< uint8_t[ ] >( sc_alloc_size );
				stealth::fill_with_junk( junk.get( ) , sc_alloc_size );
				ioctl->write_memory( allocated_shellcode , junk.get( ) , sc_alloc_size );
			}

			// Copy shellcode locally and patch the magic marker
			uintptr_t localshellcodealloc = ( uintptr_t ) VirtualAlloc(
				0 , shellcodefunction_length , MEM_COMMIT | MEM_RESERVE , PAGE_READWRITE );
			memcpy( ( PVOID ) localshellcodealloc , &shellcode , shellcodefunction_length );

			auto find_in_shellcode = [ ] ( const unsigned char* buffer , size_t buffer_size ,
				const unsigned char* pattern , size_t pattern_size ) -> unsigned char* {
				if ( !buffer || !pattern || pattern_size == 0 || buffer_size < pattern_size )
					return nullptr;
				for ( size_t i = 0; i <= buffer_size - pattern_size; ++i ) {
					if ( memcmp( buffer + i , pattern , pattern_size ) == 0 )
						return const_cast< unsigned char* >( buffer + i );
				}
				return nullptr;
			};

			// Search for SHELL_MAGIC in little-endian byte order
			uint64_t magic_val = SHELL_MAGIC;
			auto* shell_data_ptr = find_in_shellcode(
				reinterpret_cast< const unsigned char* >( localshellcodealloc ) ,
				static_cast< size_t >( shellcodefunction_length ) ,
				reinterpret_cast< const unsigned char* >( &magic_val ) ,
				sizeof( magic_val ) );
			if ( !shell_data_ptr ) {
				print( "magic marker not found" );
				VirtualFree( ( void* ) localshellcodealloc , 0 , MEM_RELEASE );
				return false;
			}

			// Patch in the actual shell_data address
			uintptr_t shell_data_offset = uintptr_t( shell_data_ptr - reinterpret_cast< unsigned char* >( localshellcodealloc ) );
			*( uintptr_t* ) ( localshellcodealloc + shell_data_offset ) = shell_data_addr;

			// Write shellcode to remote
			ioctl->write_memory( allocated_shellcode , ( const void* ) localshellcodealloc , shellcodefunction_length );

			// Clean up local copy immediately
			stealth::secure_zero( ( void* ) localshellcodealloc , shellcodefunction_length );
			VirtualFree( ( void* ) localshellcodealloc , 0 , MEM_RELEASE );

			// Flip shellcode region to RX (no write, no RWX)
			ioctl->protect_virtual( allocated_shellcode , sc_alloc_size , PAGE_EXECUTE_READ );
			stealth::random_sleep( 30 , 80 );

			// ============================================================
			//  Phase 13: Hook present pointer
			// ============================================================
			ioctl->write_memory( m_inject_address , &allocated_shellcode , sizeof( allocated_shellcode ) );

			// ============================================================
			//  Phase 14: Wait for shellcode execution
			// ============================================================
			int wait_count = 0;
			while ( true ) {
				f_shellcode_data remote_shell_data { };
				if ( ioctl->read_memory( shell_data_addr , &remote_shell_data , sizeof( f_shellcode_data ) )
					&& remote_shell_data.shell_done ) {
					break;
				}
				stealth::random_sleep( 1 , 5 );
				if ( ++wait_count > 30000 ) {
					print( "shellcode execution timeout" );
					return false;
				}
			}

			// ============================================================
			//  Phase 15: Post-injection cleanup
			// ============================================================

			// Restore original present pointer
			ioctl->write_memory( m_inject_address , &m_original_fn , sizeof( m_original_fn ) );
			stealth::random_sleep( 10 , 30 );

			// Wipe PE headers from injected module
			wipe_pe_headers( allocated_base , header_size );

			// Wipe shellcode from memory (it's done executing)
			{
				auto dead = std::make_unique< uint8_t[ ] >( sc_alloc_size );
				stealth::fill_with_junk( dead.get( ) , sc_alloc_size );
				ioctl->protect_virtual( allocated_shellcode , sc_alloc_size , PAGE_READWRITE );
				ioctl->write_memory( allocated_shellcode , dead.get( ) , sc_alloc_size );
			}

			// Wipe shell_data struct
			{
				f_shellcode_data empty { };
				ioctl->write_memory( shell_data_addr , &empty , sizeof( empty ) );
			}

			// Zero out local sensitive variables
			stealth::secure_zero( &m_inject_address , sizeof( m_inject_address ) );
			stealth::secure_zero( &m_original_fn , sizeof( m_original_fn ) );

			print( "injection complete - all traces cleaned" );
			return true;
		}
	};
}
