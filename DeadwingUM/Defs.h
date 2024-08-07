#pragma once

typedef struct _EFI_ACPI_COMMON_HEADER {
	UINT32    Signature;
	UINT32    Length;
} EFI_ACPI_COMMON_HEADER, *PEFI_ACPI_COMMON_HEADER;

typedef struct _DEADWING_UM_KM_COMMUNICATION {
	UINT64 ProcessId;

	struct {
		PVOID  PhysReadAddress;
		PVOID  VaReadAddress;
		PVOID  VaReadResultAddress;
		UINT64 ReadLength;
	} Read;

	struct {
		PVOID  PhysWriteAddress;
		PVOID  VaWriteAddress;
		PVOID  VaDataAddress;
		UINT64 WriteLength;
	} Write;

	struct {
		PVOID  AddressToTranslate;
		UINT64 Translated;
	} Vtop;
} DEADWING_UM_KM_COMMUNICATION, *PDEADWING_UM_KM_COMMUNICATION;
