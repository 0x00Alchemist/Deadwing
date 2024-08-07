#include <Uefi.h>
#include <Base.h>

#include <Library/UefiLib.h>

#include <Protocol/SmmReadyToLock.h>
#include <Protocol/SmmVariable.h>

#include "Globals.h"
#include "Defs.h"
#include "Serial.h"
#include "Utils.h"


EFI_STATUS
EFIAPI
RegisterSmmVarCallback(
	IN EFI_GUID   *Guid,
	IN VOID       *Interface,
	IN EFI_HANDLE  Handle
) {
	// cache EFI_SMM_VARIABLE_PROTOCOL protocol
	gSmmVar = (EFI_SMM_VARIABLE_PROTOCOL *)Interface;
}

/**
 * \brief Indicates that the SMRAM is ready to be locked.
 * 
 * The function checks SmiCmd in the FADT/FACP ACPI table and publishes the information 
 * to the OS modules. Everything should already be initialized at this point.
 * 
 * \param Guid      Points to the protocol's unique identifier
 * \param Interface Points to the interface instance
 * \param Handle    The handle on which the interface was installed
 * 
 * \return 
 */
EFI_STATUS
EFIAPI
LockBoxNotifier(
	IN EFI_GUID   *Guid,
	IN VOID       *Interface,
	IN EFI_HANDLE  Handle
) {
	SerialPrint("[ SMM ] SMM ready to lock, publishing transfer packet\r\n");

	// Check SmiCmd port value. For some reason, 
	// ACPI does not immediately publish the tables.
	gSmiTrigger = CheckSmiTriggerPort();
	if(gSmiTrigger == 0) {
		SerialPrint("[ SMM ] Unable to locate FADT/FACP ACPI table or SMM not supported\r\n");
		return EFI_UNSUPPORTED;
	}

	// publish transfer info for OS module(s)
	DeadwingPublishTransferPacket();

	return EFI_SUCCESS;
}

/**
 * \brief Registers notifier(s) to handle some SMM events.
 * 
 * \return EFI_SUCCESS - Succesfully registered notifier(s)
 * \return Other - Unable to register notifier(s)
 */
EFI_STATUS
EFIAPI
InstallSmmNotifiers(
	VOID
) {
	EFI_STATUS Status;
	VOID *Registration;

	// register SMRAM lock notifier
	VOID *LockBoxReg;
	Status = gSmst2->SmmRegisterProtocolNotify(&gEfiSmmReadyToLockProtocolGuid, LockBoxNotifier, &LockBoxReg);
	if(EFI_ERROR(Status)) {
		SerialPrint("[ SMM ] Unable to register Ready To Lock protocol notifier\r\n");
		return Status;
	}
	/// \note @0x00Alchemist: The GetVariable service from the runtime services does not work 
	/// in the SMM environment (after a fired interrupt when the handler is running). In order 
	/// to get the necessary information from the DXE driver, the EFI_SMM_VARIABLE_PROTOCOL protocol 
	/// must be cached
	Status = gSmst2->SmmLocateProtocol(&gEfiSmmVariableProtocolGuid, NULL, (VOID **)&gSmmVar);
	if(EFI_ERROR(Status)) {
		Status = gSmst2->SmmRegisterProtocolNotify(&gEfiSmmVariableProtocolGuid, RegisterSmmVarCallback, &Registration);
		if(EFI_ERROR(Status))
			SerialPrint("[ SMM ] Unable to register SMM Var protocol notify\r\n");
	}

	return Status;
}
