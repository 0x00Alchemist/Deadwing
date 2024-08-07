#pragma once

typedef struct _DEADWING_COMMUNICATION {
	UINT32      Command;
	EFI_STATUS  SmiRetStatus;
	UINT64      CommBufSize;

	struct {
		UINT64  TargetProcessId;
		VOID   *PhysReadAddress;
		VOID   *VaReadAddress;
		VOID   *ReadResult;
		UINT64  ReadLength;
	} Read;

	struct {
		UINT64  TargetProcessId;
		VOID   *PhysWriteAddress;
		VOID   *VaWriteAddress;
		VOID   *DataToWrite;
		UINT64  WriteLength;
	} Write;

	struct {
		UINT64  ControllerProcessId;
		VOID   *VaPsInitialSysProcess;
		UINT64  DirBase;
	} Cache;

	struct {
		UINT64  TargetPid;
		VOID   *AddressToTranslate;
		VOID   *Translated;
	} Vtop;

	struct {
		UINT32  Signature;
		VOID   *Buffer;
	} Acpi;
} DEADWING_COMMUNICATION, *PDEADWING_COMMUNICATION;

typedef struct _DEADWING_TRANSFER {
	struct {
		VOID  *CommBufVirtual;
		VOID  *CommBufPhys;
		UINTN  BufSize;
	} Buffer;

	struct {
		VOID  *SetupBufFunction;
		VOID  *SmmCommunicateFunction;
	} API;
} DEADWING_TRANSFER, *PDEADWING_TRANSFER;
