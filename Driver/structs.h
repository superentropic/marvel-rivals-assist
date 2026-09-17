#pragma once
#include "definitions.h"
#define read_memory CTL_CODE(FILE_DEVICE_UNKNOWN, 0x73A, METHOD_BUFFERED, FILE_SPECIAL_ACCESS)
#define write_memory CTL_CODE(FILE_DEVICE_UNKNOWN, 0x7B1, METHOD_BUFFERED, FILE_SPECIAL_ACCESS)
#define get_module_base CTL_CODE(FILE_DEVICE_UNKNOWN, 0x72C, METHOD_BUFFERED, FILE_SPECIAL_ACCESS)
#define protect_virutal_memory CTL_CODE(FILE_DEVICE_UNKNOWN, 0x7D4, METHOD_BUFFERED, FILE_SPECIAL_ACCESS)
#define allocate_virtual_memory CTL_CODE(FILE_DEVICE_UNKNOWN, 0x7E9, METHOD_BUFFERED, FILE_SPECIAL_ACCESS)

#define invoke_translate CTL_CODE(FILE_DEVICE_UNKNOWN, 0x71F, METHOD_BUFFERED, FILE_SPECIAL_ACCESS)
#define invoke_dtb CTL_CODE(FILE_DEVICE_UNKNOWN, 0x756, METHOD_BUFFERED, FILE_SPECIAL_ACCESS)
#define read_memory_v  CTL_CODE(FILE_DEVICE_UNKNOWN, 0x768, METHOD_BUFFERED, FILE_SPECIAL_ACCESS)
#define write_memory_v CTL_CODE(FILE_DEVICE_UNKNOWN, 0x78D, METHOD_BUFFERED, FILE_SPECIAL_ACCESS)

typedef struct _k_get_base_module_request {
	ULONG pid;
	ULONGLONG handle;
	WCHAR name[260];
} k_get_base_module_request, *pk_get_base_module_request;

typedef struct _k_rw_request {
	ULONG pid;
	ULONGLONG src;
	ULONGLONG dst;
	ULONGLONG size;
} k_rw_request, *pk_rw_request;

typedef struct _k_alloc_mem_request {
	ULONG pid, allocation_type, protect;
	ULONGLONG addr;
	SIZE_T size;
} k_alloc_mem_request, *pk_alloc_mem_request;

typedef struct _k_protect_mem_request {
	ULONG pid, protect;
	ULONGLONG addr;
	SIZE_T size;
} k_protect_mem_request, *pk_protect_mem_request;

typedef struct _translate_invoke {

	ULONGLONG virtual_address;

	ULONGLONG directory_base;

	ULONGLONG physical_address; // Will be filled by driver

} translate_invoke, *ptranslate_invoke;

typedef struct _dtb_invoke {

	ULONG pid;

	ULONGLONG dtb;

} dtb_invoke, *pdtb_invoke;

typedef struct _k_rw_virtual_request {
	ULONG pid;
	ULONGLONG dtb;
	ULONGLONG virtual_address;
	ULONGLONG buffer;
	SIZE_T size;
} k_rw_virtual_request , * pk_rw_virtual_request;
