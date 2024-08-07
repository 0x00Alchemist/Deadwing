#pragma once

NTSTATUS
NTAPI
TransferWorker(
	VOID
);

NTSTATUS
NTAPI
FormPacket(
	_In_     UINT64                  Command,
	_In_opt_ UINT64                  ProcessId,
	_In_opt_ UINT64                  Arg1,
	_In_opt_ UINT64                  Arg2,
	_In_opt_ UINT64                  Arg3,
	_Out_    PDEADWING_COMMUNICATION Packet
);

NTSTATUS
NTAPI
EfiStatusToNtStatus(
	_In_ UINT64 EfiStatus
);
