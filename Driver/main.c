#include "definitions.h"
#include "structs.h"

#define drv_device L"\\Device\\WdmAudMixer"
#define drv_dos_device L"\\DosDevices\\WdmAudMixer"
#define drv  L"\\Driver\\WdmAudMixer"

PDEVICE_OBJECT driver_object;
UNICODE_STRING dev , dos;

NTSTATUS read_physical( ULONGLONG address , PVOID buffer , SIZE_T size , SIZE_T* bytes );
NTSTATUS write_physical( ULONGLONG address, PVOID buffer, SIZE_T size, SIZE_T* bytes );
ULONGLONG translate_linear( ULONGLONG directory_base, ULONGLONG address );

NTSTATUS copy_memory( PEPROCESS src_proc , PEPROCESS target_proc , PVOID src , PVOID dst , SIZE_T size ) {
	PSIZE_T bytes;
	return MmCopyVirtualMemory( target_proc , src , src_proc , dst , size , UserMode , &bytes );
}

NTSTATUS read_virtual_physical(
	ULONGLONG dtb ,
	ULONGLONG virtual_address ,
	PVOID buffer ,
	SIZE_T size
) {
	SIZE_T offset = 0;

	while ( size ) {
		ULONGLONG phys = translate_linear( dtb , virtual_address );
		if ( !phys ) return STATUS_UNSUCCESSFUL;

		SIZE_T page_offset = phys & 0xFFF;
		SIZE_T chunk = min( size , 0x1000 - page_offset );

		SIZE_T bytes = 0;
		if ( !NT_SUCCESS( read_physical( phys , ( PUCHAR ) buffer + offset , chunk , &bytes ) ) )
			return STATUS_UNSUCCESSFUL;

		virtual_address += chunk;
		offset += chunk;
		size -= chunk;
	}

	return STATUS_SUCCESS;
}

NTSTATUS write_virtual_physical(
	ULONGLONG dtb ,
	ULONGLONG virtual_address ,
	PVOID buffer ,
	SIZE_T size
) {
	SIZE_T offset = 0;

	while ( size ) {
		ULONGLONG phys = translate_linear( dtb , virtual_address );
		if ( !phys ) return STATUS_UNSUCCESSFUL;

		SIZE_T page_offset = phys & 0xFFF;
		SIZE_T chunk = min( size , 0x1000 - page_offset );

		SIZE_T bytes = 0;
		if ( !NT_SUCCESS( write_physical( phys , ( PUCHAR ) buffer + offset , chunk , &bytes ) ) )
			return STATUS_UNSUCCESSFUL;

		virtual_address += chunk;
		offset += chunk;
		size -= chunk;
	}

	return STATUS_SUCCESS;
}

ULONGLONG get_module_handle( ULONG pid , LPCWSTR module_name ) {
	PEPROCESS target_proc;
	ULONGLONG base = 0;
	if ( !NT_SUCCESS( PsLookupProcessByProcessId( ( HANDLE ) pid , &target_proc ) ) )
		return 0;

	KeAttachProcess( ( PKPROCESS ) target_proc );

	PPEB peb = PsGetProcessPeb( target_proc );
	if ( !peb )
		goto end;

	if ( !peb->Ldr || !peb->Ldr->Initialized )
		goto end;


	UNICODE_STRING module_name_unicode;
	RtlInitUnicodeString( &module_name_unicode , module_name );
	for ( PLIST_ENTRY list = peb->Ldr->InLoadOrderModuleList.Flink;
		list != &peb->Ldr->InLoadOrderModuleList;
		list = list->Flink ) {
		PLDR_DATA_TABLE_ENTRY entry = CONTAINING_RECORD( list , LDR_DATA_TABLE_ENTRY , InLoadOrderLinks );
		if ( RtlCompareUnicodeString( &entry->BaseDllName , &module_name_unicode , TRUE ) == 0 ) {
			base = entry->DllBase;
			goto end;
		}
	}

end:
	KeDetachProcess( );
	ObDereferenceObject( target_proc );
	return base;
}

NTSTATUS read_physical( ULONGLONG address, PVOID buffer, SIZE_T size, SIZE_T* bytes ) {
	MM_COPY_ADDRESS target_address = { 0 };
	target_address.PhysicalAddress.QuadPart = address;
	return MmCopyMemory(buffer, target_address, size, MM_COPY_MEMORY_PHYSICAL, bytes);
}

NTSTATUS write_physical( ULONGLONG address , PVOID buffer , SIZE_T size , SIZE_T* bytes ) {
	PHYSICAL_ADDRESS pa;
	pa.QuadPart = address;

	PVOID mapped = MmMapIoSpaceEx( pa , size , PAGE_READWRITE );
	if ( !mapped )
		return STATUS_UNSUCCESSFUL;

	RtlCopyMemory( mapped , buffer , size );
	MmUnmapIoSpace( mapped , size );

	if ( bytes )
		*bytes = size;

	return STATUS_SUCCESS;
}


ULONGLONG translate_linear( ULONGLONG directory_base, ULONGLONG address ) {
	directory_base &= ~0xFULL;

	ULONGLONG virt_addr = address & ~( ( ~0ULL ) << 12 );
	ULONGLONG pte = ( ( address >> 12 ) & 0x1FF );
	ULONGLONG pt = ( ( address >> 21 ) & 0x1FF );
	ULONGLONG pd = ( ( address >> 30 ) & 0x1FF );
	ULONGLONG pdp = ( ( address >> 39 ) & 0x1FF );
	const ULONGLONG p_mask = 0xFFFFFFFFFFFFF000ULL;

	SIZE_T readsize = 0;
	ULONGLONG pdpe = 0;
	if ( !NT_SUCCESS( read_physical( directory_base + 8 * pdp, &pdpe, sizeof( pdpe ), &readsize ) ) )
		return 0;
	if ( ( pdpe & 1 ) == 0 )
		return 0;

	ULONGLONG pde = 0;
	if ( !NT_SUCCESS( read_physical( ( pdpe & p_mask ) + 8 * pd, &pde, sizeof( pde ), &readsize ) ) )
		return 0;
	if ( ( pde & 1 ) == 0 )
		return 0;

	// 1GB large page
	if ( pde & 0x80 )
		return ( pde & p_mask ) + ( address & ~( ( ~0ULL ) << 30 ) );

	ULONGLONG pteAddr = 0;
	if ( !NT_SUCCESS( read_physical( ( pde & p_mask ) + 8 * pt, &pteAddr, sizeof( pteAddr ), &readsize ) ) )
		return 0;
	if ( ( pteAddr & 1 ) == 0 )
		return 0;

	// 2MB large page
	if ( pteAddr & 0x80 )
		return ( pteAddr & p_mask ) + ( address & ~( ( ~0ULL ) << 21 ) );

	ULONGLONG final = 0;
	if ( !NT_SUCCESS( read_physical( ( pteAddr & p_mask ) + 8 * pte, &final, sizeof( final ), &readsize ) ) )
		return 0;

	final &= p_mask;
	if ( !final ) return 0;

	return final + virt_addr;
}

NTSTATUS io_device_control( PDEVICE_OBJECT device , PIRP irp ) {
	NTSTATUS status;
	ULONG info_size = 0;
	PIO_STACK_LOCATION stack = IoGetCurrentIrpStackLocation( irp );
	ULONG control_code = stack->Parameters.DeviceIoControl.IoControlCode;
	 
	switch ( control_code ) {
	case invoke_dtb: {
		pdtb_invoke in = ( pdtb_invoke ) irp->AssociatedIrp.SystemBuffer;
		PEPROCESS process = 0;
		status = PsLookupProcessByProcessId( ( HANDLE ) in->pid , &process );
		if ( NT_SUCCESS( status ) && process ) {
			ULONGLONG process_dtb = *(ULONGLONG*)( ( PUCHAR ) process + 0x28 );
			if ( !process_dtb ) {
				process_dtb = *(ULONGLONG*)( ( PUCHAR ) process + 0x388 );
			}
			in->dtb = process_dtb;
			ObfDereferenceObject( process );
			status = STATUS_SUCCESS;
		}
		info_size = sizeof( dtb_invoke );
	} break;

	case invoke_translate: {
		ptranslate_invoke in = ( ptranslate_invoke ) irp->AssociatedIrp.SystemBuffer;
		ULONGLONG phys = translate_linear( in->directory_base, in->virtual_address );
		in->physical_address = phys;
		status = STATUS_SUCCESS;
		info_size = sizeof( translate_invoke );
	} break;

	case allocate_virtual_memory: {
		pk_alloc_mem_request in = ( pk_alloc_mem_request ) irp->AssociatedIrp.SystemBuffer;
		PEPROCESS target_proc;
		status = PsLookupProcessByProcessId( in->pid , &target_proc );
		if ( NT_SUCCESS( status ) ) {
			KAPC_STATE apc;
			KeStackAttachProcess( target_proc , &apc );
			status = ZwAllocateVirtualMemory( ZwCurrentProcess( ) , &in->addr , 0 , &in->size ,
				in->allocation_type , in->protect );
			KeUnstackDetachProcess( &apc );
			ObfDereferenceObject( target_proc );
		}
		info_size = sizeof( k_alloc_mem_request );
	} break;

	case protect_virutal_memory: {
		pk_protect_mem_request in = ( pk_protect_mem_request ) irp->AssociatedIrp.SystemBuffer;
		PEPROCESS target_proc;
		status = PsLookupProcessByProcessId( in->pid , &target_proc );
		if ( NT_SUCCESS( status ) ) {
			KAPC_STATE apc;
			ULONG old_protection;
			KeStackAttachProcess( target_proc , &apc );
			status = ZwProtectVirtualMemory( ZwCurrentProcess( ) , &in->addr , &in->size , in->protect , &old_protection );
			KeUnstackDetachProcess( &apc );
			in->protect = old_protection;
			ObfDereferenceObject( target_proc );
		}
		info_size = sizeof( k_protect_mem_request );
	} break;

	case read_memory: {
		pk_rw_request in = ( pk_rw_request ) irp->AssociatedIrp.SystemBuffer;
		PEPROCESS target_proc;
		status = PsLookupProcessByProcessId( in->pid , &target_proc );
		if ( NT_SUCCESS( status ) ) {
			status = copy_memory( PsGetCurrentProcess( ) , target_proc , in->src , in->dst , in->size );
			ObfDereferenceObject( target_proc );
		}
		info_size = sizeof( k_rw_request );
	} break;

	case write_memory: {
		pk_rw_request in = ( pk_rw_request ) irp->AssociatedIrp.SystemBuffer;
		PEPROCESS target_proc;
		status = PsLookupProcessByProcessId( in->pid , &target_proc );
		if ( NT_SUCCESS( status ) ) {
			status = copy_memory( target_proc , PsGetCurrentProcess( ) , in->src , in->dst , in->size );
			ObfDereferenceObject( target_proc );
		}
		info_size = sizeof( k_rw_request );
	} break;

	case get_module_base: {
		pk_get_base_module_request in = ( pk_get_base_module_request ) irp->AssociatedIrp.SystemBuffer;
		ULONGLONG handle = get_module_handle( in->pid , in->name );
		in->handle = handle;
		status = STATUS_SUCCESS;
		info_size = sizeof( k_get_base_module_request );
	} break;

	case read_memory_v: {
		pk_rw_virtual_request in = ( pk_rw_virtual_request ) irp->AssociatedIrp.SystemBuffer;
		status = read_virtual_physical(
			in->dtb ,
			in->virtual_address ,
			( PVOID ) in->buffer ,
			in->size
		);
		info_size = sizeof( k_rw_virtual_request );
	} break;

	case write_memory_v: {
		pk_rw_virtual_request in = ( pk_rw_virtual_request ) irp->AssociatedIrp.SystemBuffer;
		status = write_virtual_physical(
			in->dtb ,
			in->virtual_address ,
			( PVOID ) in->buffer ,
			in->size
		);
		info_size = sizeof( k_rw_virtual_request );
	} break;

	default:
		status = STATUS_INVALID_PARAMETER;
		info_size = 0;
		break;
	}

	irp->IoStatus.Status = status;
	irp->IoStatus.Information = info_size;
	IoCompleteRequest( irp , IO_NO_INCREMENT );
	return status;
}

NTSTATUS unload_driver( PDRIVER_OBJECT driver ) {
	IoDeleteSymbolicLink( &dos );
	IoDeleteDevice( driver->DeviceObject );
}

NTSTATUS create( PDEVICE_OBJECT device , PIRP irp ) {
	irp->IoStatus.Status = STATUS_SUCCESS;
	irp->IoStatus.Information = 0;
	IoCompleteRequest( irp , IO_NO_INCREMENT );
	return STATUS_SUCCESS;
}

NTSTATUS close( PDEVICE_OBJECT device , PIRP irp ) {
	irp->IoStatus.Status = STATUS_SUCCESS;
	irp->IoStatus.Information = 0;
	IoCompleteRequest( irp , IO_NO_INCREMENT );
	return STATUS_SUCCESS;
}

NTSTATUS init( PDRIVER_OBJECT driver , PUNICODE_STRING path ) {
	RtlInitUnicodeString( &dev , drv_device );
	RtlInitUnicodeString( &dos , drv_dos_device );

	IoCreateDevice( driver , 0 , &dev , FILE_DEVICE_UNKNOWN , FILE_DEVICE_SECURE_OPEN , FALSE , &driver_object );
	IoCreateSymbolicLink( &dos , &dev );

	driver->MajorFunction [IRP_MJ_DEVICE_CONTROL] = io_device_control;
	driver->MajorFunction [IRP_MJ_CREATE] = create;
	driver->MajorFunction [IRP_MJ_CLOSE] = close;
	driver->DriverUnload = unload_driver;

	driver_object->Flags |= DO_DIRECT_IO;
	driver_object->Flags &= ~DO_DEVICE_INITIALIZING;

	return STATUS_SUCCESS;
}

NTSTATUS DriverEntry( PDRIVER_OBJECT driver , PUNICODE_STRING path ) {
	NTSTATUS        status;
	UNICODE_STRING drv_name;
	RtlInitUnicodeString( &drv_name , drv );
	return IoCreateDriver( &drv_name , &init );
}