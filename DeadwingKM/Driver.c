#include <ntddk.h>

#include "Defs.h"
#include "Communication.h"
#include "Utils.h"


/**
 * \brief Drivers IOCTL handler
 * 
 * \param DriverObject Our driver object
 * \param Irp          Received I/O request packet
 * 
 *  UM <-> KM communication structure layout:
 * 
 *      +---------------------+
 *     /                     /|
 *    +---------------------+ | <- Start of communication structure
 *    |                     | |
 *    |         Data        | | <- Data passed to the KM driver
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
 * 
 * \return STATUS_SUCCESS - Succesfully handled and performed command
 * \return STATUS_INVALID_DEVICE_REQUEST - Invalid command has been passed
 * \return Other - SMI handler cannot perform operation
 */
NTSTATUS
NTAPI
DriverDispatch(
	_In_ PDRIVER_OBJECT DriverObject,
	_In_ PIRP           Irp
) {
	UNREFERENCED_PARAMETER(DriverObject);

	NTSTATUS Status;
	
	PIO_STACK_LOCATION IoStack = IoGetCurrentIrpStackLocation(Irp);
	UINT64 Output = 0;
	
	// validate length of the passed buffer
	if(IoStack->Parameters.DeviceIoControl.InputBufferLength == sizeof(DEADWING_UM_KM_COMMUNICATION)) {
		// check passed buffer
		if(Irp->AssociatedIrp.SystemBuffer != NULL) {
			// dispatch request
			Status = CommUmCmdHandler(Irp, IoStack, &Output);
			if(!NT_SUCCESS(Status))
				KdPrint(("[ DeadwingKM ] Cannot process request\n"));
		} else {
			KdPrint(("[ DeadwingKM ] Invalid buffer has been passed\n"));
			Status = STATUS_INVALID_PARAMETER;
		}
	} else {
		KdPrint(("[ DeadwingKM ] Invalid buffer length\n"));
		Status = STATUS_BUFFER_OVERFLOW;
	}

	Irp->IoStatus.Information = Output;
	Irp->IoStatus.Status = Status;
	IoCompleteRequest(Irp, IO_NO_INCREMENT);

	return Status;
}

/**
 * \brief Drivers create/close handler
 * 
 * \param DriverObject Our driver object
 * \param Irp          Received I/O request packet
 * 
 * \return STATUS_SUCCESS - Function was dispatched succesfully
 */
NTSTATUS
NTAPI
DriverCreateClose(
	_In_ PDRIVER_OBJECT DriverObject,
	_In_ PIRP           Irp
) {
	UNREFERENCED_PARAMETER(DriverObject);

	Irp->IoStatus.Information = 0;
	Irp->IoStatus.Status = STATUS_SUCCESS;

	IoCompleteRequest(Irp, IO_NO_INCREMENT);

	return STATUS_SUCCESS;
}

/**
 * \brief Trap for the unimplemented functions
 * 
 * \param DriverObject Our driver object
 * \param Irp          Received I/O request packet
 * 
 * \return STATUS_SUCCESS - Function was dispatched succesfully
 */
NTSTATUS
NTAPI
DriverUnimplemented(
	_In_ PDRIVER_OBJECT DriverObject,
	_In_ PIRP           Irp
) {
	UNREFERENCED_PARAMETER(DriverObject);

	Irp->IoStatus.Information = 0;
	Irp->IoStatus.Status = STATUS_ILLEGAL_FUNCTION;

	IoCompleteRequest(Irp, IO_NO_INCREMENT);

	return STATUS_SUCCESS;
}

/**
 * \brief Unloads communication driver
 * 
 * \param DriverObject Our driver object
 * 
 * \return STATUS_SUCCESS - Driver has been unloaded succesfully
 */
NTSTATUS
NTAPI
DriverUnload(
	_In_ PDRIVER_OBJECT DriverObject
) {
	UNICODE_STRING DosDevice = RTL_CONSTANT_STRING(L"\\DosDevices\\DeadwingKM");
	IoDeleteSymbolicLink(&DosDevice);
	
	IoDeleteDevice(DriverObject->DeviceObject);

	return STATUS_SUCCESS;
}

/**
 * \brief Driver entry point
 * 
 * \param DriverObject Driver object
 * \param RegistryPath Registry path
 * 
 * \return STATUS_SUCCESS - Driver initialized succesfully
 * \return Other - Cannot get transfer data or unable to perform demo command
 */
NTSTATUS
NTAPI
DriverEntry(
	_In_ PDRIVER_OBJECT   DriverObject,
	_In_ PUNICODE_STRING  RegistryPath
) {
	UNREFERENCED_PARAMETER(RegistryPath);

	KdPrint(("=[ DeadwingKM ]=\n"));
	KdPrint(("=[ A demo driver for communication between OS and SMM ]=\n"));

	// try to cache Deadwing information 
	NTSTATUS Status = TransferWorker();
	if(!NT_SUCCESS(Status))
		return Status;

	for(INT i = 0; i < IRP_MJ_MAXIMUM_FUNCTION; i++)
		DriverObject->MajorFunction[i] = DriverUnimplemented;

	// initialize main dispatchers
	DriverObject->DriverUnload = DriverUnload;
	DriverObject->MajorFunction[IRP_MJ_CLOSE] = DriverCreateClose;
	DriverObject->MajorFunction[IRP_MJ_CREATE] = DriverCreateClose;
	DriverObject->MajorFunction[IRP_MJ_DEVICE_CONTROL] = DriverDispatch;

	DEVICE_OBJECT DeviceObject;
	RtlSecureZeroMemory(&DeviceObject, sizeof(DEVICE_OBJECT));

	// initialize device object
	UNICODE_STRING DeviceName = RTL_CONSTANT_STRING(L"\\Device\\DeadwingKM");
	Status = IoCreateDevice(DriverObject, 0, &DeviceName, FILE_DEVICE_UNKNOWN, FILE_DEVICE_SECURE_OPEN, TRUE, &DeviceObject);
	if(!NT_SUCCESS(Status)) {
		KdPrint(("[ DeadwingKM ] Unable to create device (0x%lX)\n", Status));
		return Status;
	}

	DeviceObject.Flags |= DO_BUFFERED_IO;
	DeviceObject.Flags &= DO_DEVICE_INITIALIZING;

	UNICODE_STRING DosDevice = RTL_CONSTANT_STRING(L"\\DosDevices\\DeadwingKM");
	Status = IoCreateSymbolicLink(&DosDevice, &DeviceName);
	if(!NT_SUCCESS(Status)) {
		KdPrint(("[ DeadwingKM ] Unable to create symbolic link (0x%lX)\n", Status));
		return Status;
	}

	return STATUS_SUCCESS;
}
