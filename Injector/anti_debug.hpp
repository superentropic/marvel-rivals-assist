#pragma once
#include <Windows.h>
#include <cstdint>
#include <intrin.h>
#include <winternl.h>
#include <tlhelp32.h>

#pragma comment(lib, "ntdll.lib")

// Not defined in standard headers
typedef struct _SYSTEM_KERNEL_DEBUGGER_INFORMATION {
	BOOLEAN KernelDebuggerEnabled;
	BOOLEAN KernelDebuggerNotPresent;
} SYSTEM_KERNEL_DEBUGGER_INFORMATION, *PSYSTEM_KERNEL_DEBUGGER_INFORMATION;

namespace anti_debug {

	inline bool check_debugger_present( ) {
		return IsDebuggerPresent( ) != FALSE;
	}

	inline bool check_remote_debugger( ) {
		BOOL present = FALSE;
		CheckRemoteDebuggerPresent( GetCurrentProcess( ) , &present );
		return present != FALSE;
	}

	inline bool check_peb_being_debugged( ) {
#ifdef _M_X64
		auto peb = reinterpret_cast< PPEB >( __readgsqword( 0x60 ) );
#else
		auto peb = reinterpret_cast< PPEB >( __readfsdword( 0x30 ) );
#endif
		return peb->BeingDebugged != 0;
	}

	inline bool check_nt_global_flag( ) {
#ifdef _M_X64
		auto peb = reinterpret_cast< uint8_t* >( __readgsqword( 0x60 ) );
		uint32_t flags = *reinterpret_cast< uint32_t* >( peb + 0xBC );
#else
		auto peb = reinterpret_cast< uint8_t* >( __readfsdword( 0x30 ) );
		uint32_t flags = *reinterpret_cast< uint32_t* >( peb + 0x68 );
#endif
		constexpr uint32_t FLG_HEAP_ENABLE_TAIL_CHECK = 0x10;
		constexpr uint32_t FLG_HEAP_ENABLE_FREE_CHECK = 0x20;
		constexpr uint32_t FLG_HEAP_VALIDATE_PARAMETERS = 0x40;
		return ( flags & ( FLG_HEAP_ENABLE_TAIL_CHECK | FLG_HEAP_ENABLE_FREE_CHECK | FLG_HEAP_VALIDATE_PARAMETERS ) ) != 0;
	}

	inline bool check_hardware_breakpoints( ) {
		CONTEXT ctx {};
		ctx.ContextFlags = CONTEXT_DEBUG_REGISTERS;
		if ( GetThreadContext( GetCurrentThread( ) , &ctx ) ) {
			return ( ctx.Dr0 || ctx.Dr1 || ctx.Dr2 || ctx.Dr3 );
		}
		return false;
	}

	inline bool check_timing_rdtsc( ) {
		uint64_t t1 = __rdtsc( );
		volatile int dummy = 0;
		for ( int i = 0; i < 100; ++i )
			dummy += i;
		uint64_t t2 = __rdtsc( );
		return ( t2 - t1 ) > 0x1000000;
	}

	inline bool check_query_performance( ) {
		LARGE_INTEGER freq , t1 , t2;
		QueryPerformanceFrequency( &freq );
		QueryPerformanceCounter( &t1 );
		volatile int dummy = 0;
		for ( int i = 0; i < 1000; ++i )
			dummy += i;
		QueryPerformanceCounter( &t2 );
		double elapsed_ms = ( double ) ( t2.QuadPart - t1.QuadPart ) / ( double ) freq.QuadPart * 1000.0;
		return elapsed_ms > 50.0;
	}

	inline bool check_process_debug_port( ) {
		using NtQueryInformationProcessFn = NTSTATUS( NTAPI* )( HANDLE , ULONG , PVOID , ULONG , PULONG );
		static auto fn = reinterpret_cast< NtQueryInformationProcessFn >(
			GetProcAddress( GetModuleHandleA( "ntdll.dll" ) , "NtQueryInformationProcess" ) );
		if ( !fn ) return false;

		HANDLE debug_port = nullptr;
		NTSTATUS status = fn( GetCurrentProcess( ) , 7 , &debug_port , sizeof( debug_port ) , nullptr );
		return ( status == 0 && debug_port != nullptr );
	}

	inline bool check_process_debug_flags( ) {
		using NtQueryInformationProcessFn = NTSTATUS( NTAPI* )( HANDLE , ULONG , PVOID , ULONG , PULONG );
		static auto fn = reinterpret_cast< NtQueryInformationProcessFn >(
			GetProcAddress( GetModuleHandleA( "ntdll.dll" ) , "NtQueryInformationProcess" ) );
		if ( !fn ) return false;

		DWORD no_debug = 0;
		NTSTATUS status = fn( GetCurrentProcess( ) , 0x1F , &no_debug , sizeof( no_debug ) , nullptr );
		return ( status == 0 && no_debug == 0 );
	}

	inline bool check_process_debug_object( ) {
		using NtQueryInformationProcessFn = NTSTATUS( NTAPI* )( HANDLE , ULONG , PVOID , ULONG , PULONG );
		static auto fn = reinterpret_cast< NtQueryInformationProcessFn >(
			GetProcAddress( GetModuleHandleA( "ntdll.dll" ) , "NtQueryInformationProcess" ) );
		if ( !fn ) return false;

		HANDLE debug_object = nullptr;
		NTSTATUS status = fn( GetCurrentProcess( ) , 0x1E , &debug_object , sizeof( debug_object ) , nullptr );
		return ( status == 0 && debug_object != nullptr );
	}

	inline bool check_known_windows( ) {
		const char* dbg_windows[ ] = {
			"OLLYDBG" , "x64dbg" , "x32dbg" , "IDA" ,
			"Scylla" , "Process Hacker" , "Process Monitor" ,
			"Cheat Engine" , "ReClass"
		};
		for ( auto& w : dbg_windows ) {
			if ( FindWindowA( nullptr , w ) )
				return true;
			if ( FindWindowA( w , nullptr ) )
				return true;
		}
		return false;
	}

	inline bool check_blacklisted_processes( ) {
		const wchar_t* blacklist[ ] = {
			L"x64dbg.exe" , L"x32dbg.exe" , L"ollydbg.exe" ,
			L"ida.exe" , L"ida64.exe" , L"idaq.exe" , L"idaq64.exe" ,
			L"ProcessHacker.exe" , L"procmon.exe" , L"procmon64.exe" ,
			L"Wireshark.exe" , L"fiddler.exe" , L"dnSpy.exe" ,
			L"HxD.exe" , L"cheatengine-x86_64.exe" , L"cheatengine-i386.exe" ,
			L"ReClass.NET.exe" , L"Scylla_x64.exe" , L"Scylla_x86.exe" ,
			L"httpdebugger.exe" , L"HTTPDebuggerUI.exe" ,
			L"die.exe" , L"pestudio.exe" , L"lordpe.exe"
		};

		HANDLE snapshot = CreateToolhelp32Snapshot( TH32CS_SNAPPROCESS , 0 );
		if ( snapshot == INVALID_HANDLE_VALUE ) return false;

		PROCESSENTRY32W entry {};
		entry.dwSize = sizeof( entry );
		bool found = false;

		if ( Process32FirstW( snapshot , &entry ) ) {
			do {
				for ( auto& proc : blacklist ) {
					if ( _wcsicmp( entry.szExeFile , proc ) == 0 ) {
						found = true;
						break;
					}
				}
				if ( found ) break;
			} while ( Process32NextW( snapshot , &entry ) );
		}

		CloseHandle( snapshot );
		return found;
	}

	inline bool check_virtual_machine( ) {
		int cpuinfo[ 4 ] {};
		__cpuid( cpuinfo , 1 );
		return ( cpuinfo[ 2 ] >> 31 ) & 1;
	}

	inline bool check_kernel_debugger( ) {
		SYSTEM_KERNEL_DEBUGGER_INFORMATION info {};
		using NtQuerySystemInformationFn = NTSTATUS( NTAPI* )( ULONG , PVOID , ULONG , PULONG );
		static auto fn = reinterpret_cast< NtQuerySystemInformationFn >(
			GetProcAddress( GetModuleHandleA( "ntdll.dll" ) , "NtQuerySystemInformation" ) );
		if ( !fn ) return false;
		fn( 0x23 , &info , sizeof( info ) , nullptr );
		return info.KernelDebuggerEnabled && !info.KernelDebuggerNotPresent;
	}

	inline void hide_thread_from_debugger( ) {
		using NtSetInformationThreadFn = NTSTATUS( NTAPI* )( HANDLE , ULONG , PVOID , ULONG );
		static auto fn = reinterpret_cast< NtSetInformationThreadFn >(
			GetProcAddress( GetModuleHandleA( "ntdll.dll" ) , "NtSetInformationThread" ) );
		if ( fn ) {
			fn( GetCurrentThread( ) , 0x11 , nullptr , 0 );
		}
	}

	inline void set_handle_trace( ) {
		using NtSetInformationProcessFn = NTSTATUS( NTAPI* )( HANDLE , ULONG , PVOID , ULONG );
		static auto fn = reinterpret_cast< NtSetInformationProcessFn >(
			GetProcAddress( GetModuleHandleA( "ntdll.dll" ) , "NtSetInformationProcess" ) );
		if ( fn ) {
			ULONG flags = 0;
			fn( GetCurrentProcess( ) , 0x21 , &flags , sizeof( flags ) );
		}
	}

	inline bool run_all_checks( ) {
		if ( check_debugger_present( ) ) return true;
		if ( check_remote_debugger( ) ) return true;
		if ( check_peb_being_debugged( ) ) return true;
		if ( check_nt_global_flag( ) ) return true;
		if ( check_hardware_breakpoints( ) ) return true;
		if ( check_timing_rdtsc( ) ) return true;
		if ( check_process_debug_port( ) ) return true;
		if ( check_process_debug_flags( ) ) return true;
		if ( check_process_debug_object( ) ) return true;
		if ( check_known_windows( ) ) return true;
		if ( check_blacklisted_processes( ) ) return true;
		if ( check_kernel_debugger( ) ) return true;
		return false;
	}

	inline void apply_protections( ) {
		hide_thread_from_debugger( );
		set_handle_trace( );
	}
}
