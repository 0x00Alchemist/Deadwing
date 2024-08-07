#pragma once

#include <ntdef.h>

#define EFI_SUCCESS              0x0

#define EFI_INVALID_PARAMETER    0x8000000000000002ULL
#define EFI_UNSUPPORTED          0x8000000000000003ULL
#define EFI_NOT_READY            0x8000000000000006ULL
#define EFI_NOT_FOUND            0x8000000000000014ULL
#define EFI_ABORTED              0x8000000000000021ULL

#define CMD_DEADWING_PING_SMI             0xD700DEADULL
#define CMD_DEADWING_READ_PHYS            0xD800AAABULL
#define CMD_DEADWING_WRITE_PHYS           0xD800BBCDULL
#define CMD_DEADWING_VIRT_TO_PHYS         0xD800FF11ULL
#define CMD_DEADWING_READ_VIRTUAL         0xD900CCEFULL
#define CMD_DEADWING_WRITE_VIRTUAL        0xD900DDAFULL
#define CMD_DEADWING_PRIV_ESC             0xD100AC91ULL
#define CMD_DEADWING_CACHE_SESSION_INFO   0xD110A110ULL

static CONST GUID gDeadwingTransferVarGuid = { 0xE8E00F56, 0x2350, 0x49BF, { 0x9E, 0x25, 0x3A, 0x36, 0x8E, 0x8B, 0xB3, 0x73 } };

UINT32 gBufSize;

typedef UINT64 EFI_STATUS;

typedef struct _DEADWING_COMMUNICATION {
	UINT32      Command;
	UINT64      SmiRetStatus;

	UINT64      CommBufSize;

	struct {
		UINT64  TargetProcessId;
		PVOID   PhysReadAddress;
		PVOID   VaReadAddress;
		PVOID   ReadResult;
		UINT64  ReadLength;
	} Read;

	struct {
		UINT64  TargetProcessId;
		PVOID   PhysWriteAddress;
		PVOID   VaWriteAddress;
		PVOID   DataToWrite;
		UINT64  WriteLength;
	} Write;

	struct {
		UINT64  ControllerProcessId;
		PVOID   VaPsInitialSysProcess;
		UINT64  DirBase;
	} Cache;

	struct {
		UINT64  TargetPid;
		PVOID   AddressToTranslate;
		PVOID   Translated;
	} Vtop;
} DEADWING_COMMUNICATION, *PDEADWING_COMMUNICATION;

typedef struct _DEADWING_TRANSFER {
	struct {
		PVOID  CommBufVirtual;
		PVOID  CommBufPhys;
		UINT64 CommBufSize;
	} Buffer;

	struct {
		PVOID  SetupBufFunction;
		PVOID  SmmCommunicateFunction;
	} API;
} DEADWING_TRANSFER, *PDEADWING_TRANSFER;

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
