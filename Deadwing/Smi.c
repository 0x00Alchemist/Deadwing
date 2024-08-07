#include <Uefi.h>

#include <Library/UefiLib.h>
#include <Library/BaseLib.h>
#include <Library/IoLib.h>
#include <Library/SmmMemLib.h>

#include <Protocol/MmCommunication2.h>

#include "Conf.h"
#include "Globals.h"
#include "Defs.h"
#include "Serial.h"
#include "Utils.h"
#include "Memory.h"
#include "Commands.h"


#define DEADWING_COMMUNICATE_HEADER_SIZE (OFFSET_OF(EFI_MM_COMMUNICATE_HEADER, Data))


/**
 * \brief Main SMI handler.
 * 
 * Performs operations depending on the command passed to the handler. 
 * The command is formed by the user and passed through the structure for communication.
 * The structure for communication between the OS and SMM should be located in the communication buffer.
 * 
 * Warning: return only EFI_SUCCESS in the SMI handler to prevent calls of other platform SMI's. Return real status only
 * in the communication structure
 * 
 * \param DispatchHandle The unique handle which assigned by platform to this SMI handler
 * \param Context        Optional data for SMI handler which was specified when handler was registered
 * \param CommBuffer     Communication buffer with specific data which SMI handler should process
 * \param CommBufferSize Size of the communication buffer
 * 
 * Layout of inter-mode communication structure:
 *
 *      +---------------------+
 *     /                     /|
 *    +---------------------+ | <- Start of communication structure
 *    |                     | |
 *    |    Command Value    | | <- Command magic value
 *    |                     | | 
 *    +---------------------+ |  
 *    |                     | |
 *    |    Return Status    | | <- Real handler's returned status, which
 *    |                     | |    will be interpreted by OS module(s)
 *    +---------------------+ |
 *    |                     | |
 *    |     Buffer Size     | | <- Size of communication buffer which was
 *    |                     | |    allocated at DXE phase
 *    +---------------------+ |
 *    |                     | |
 *    |         Data        | | <- Data which will be consumed by SMI handler
 *    |    +--------------+ | |
 *    |   /              /| | |
 *    |  +--------------+ | | |
 *    |  |    Value 1   | | | | <- Specific value for specified command (for example,
 *    |  +--------------+ | | |    address of something)
 *    |  |      ...     | | | |
 *    |  +--------------+ | | |
 *    |  |    Value N   |/  | |
 *    |  +--------------+   | |
 *    |                     | +
 *    +---------------------+/  <- End of communication sturcture
 * 
 * \return EFI_SUCCESS - SMI handler performed operation successfully. The actual statuses of the handler are 
 *                       communicated through the structure for communication. This is done to avoid calling other handlers 
 *                       in case of an error
 */
EFI_STATUS
EFIAPI
DeadwingSmiHandler(
	IN           EFI_HANDLE  DispatchHandle,
	IN     CONST VOID       *Context        OPTIONAL,
	IN OUT       VOID       *CommBuffer     OPTIONAL,
	IN OUT       UINTN      *CommBufferSize OPTIONAL
) {
	EFI_STATUS Status;
	UINTN TempSize;
	UINTN PayloadSize;
	PDEADWING_COMMUNICATION SmiCommCtx;
	
	SerialPrint("[ SMM ] Hit Deadwing SMI handler\r\n");

	if(CommBuffer == NULL || CommBufferSize == 0) {
		SerialPrint("[ SMM ] Invalid communication buffer pointer or size\r\n");
		return EFI_SUCCESS;
	}

	TempSize = *CommBufferSize;
	PayloadSize = TempSize - DEADWING_COMMUNICATE_HEADER_SIZE;

	// validate passed buffer
	if(!SmmIsBufferOutsideSmmValid((UINTN)CommBuffer, PayloadSize)) {
		SerialPrint("[ SMM ] Communication buffer overlaps SMRAM!\r\n");
		return EFI_SUCCESS;
	}
	
	// store record
	SmiCommCtx = (PDEADWING_COMMUNICATION)CommBuffer;

	/// \note @0x00Alchemist: call SpeculationBarrier function to 
	/// ensure the check for CommBuffer have been completed
	SpeculationBarrier(); 

	// dispatch command and save real status
	SmiCommCtx->SmiRetStatus = CmdMainHandler(SmiCommCtx);

	return EFI_SUCCESS;
}

/**
 * \brief Registers a SMI handler int the platform
 * 
 * \return EFI_SUCCESS - SMI handler was registered
 * \return Other - Cannot register SMI handler
 */
EFI_STATUS
EFIAPI
SmiRegisterHandler(
	VOID
) {
	EFI_STATUS Status;
	EFI_HANDLE Handle;

	// register child SMI handler with custom HandlerType (GUID)
	Status = gSmst2->SmiHandlerRegister(DeadwingSmiHandler, &gDeadwingSmiHandlerGuid, &Handle);
	if(EFI_ERROR(Status))
		SerialPrint("[ SMM ] Unable to register child SMI handler\r\n");

	return Status;
}
