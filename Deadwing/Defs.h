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
} DEADWING_COMMUNICATION, *PDEADWING_COMMUNICATION;

typedef struct _DEADWING_LIVE_SESSION_INFO {
	struct {
		VOID   *VaPsInitialSysProcess;
		VOID   *PhysPsInitialSysProcess;
		UINT64  DirBase;
	} SysProcess;

	struct {
		VOID   *PhysUmControllerEprocess;
		UINT64  UmControllerDirBase;
	} UmController;
} DEADWING_LIVE_SESSION_INFO, *PDEADWING_LIVE_SESSION_INFO;

typedef enum _DEADWING_PAGE_TRANSLATION_SIZE {
	EDeadwingPage4Kb = 0,
	EDeadwingPage2Mb,
	EDeadwingPage1Gb
} DEADWING_PAGE_TRANSLATION_SIZE;
