#define _CRT_SECURE_NO_WARNINGS

#include <Windows.h>
#include <stdio.h>

#include "Service.h"
#include "Commands.h"


/**
 * \brief Terminates current driver session
 * 
 * \param hDriver            Handle of the driver 
 * \param ServiceTermination Logical value which signals if service should be deleted
 */
VOID
WINAPI
TerminateKmSession(
	_In_ HANDLE  hDriver,
	_In_ BOOLEAN ServiceTermination
) {
	StopDeadwingService();

	// delete service if user wants it
	if(ServiceTermination) {
		DeleteDeadwingService();
	}

	CloseHandle(hDriver);
}

/**
 * \brief Creates and starts service
 * 
 * \param DriverPath Path to the KM driver
 * \param hDriver    Output driver handle
 * 
 * \return TRUE if service succesfully setuped and handle was obtained.
 */
BOOLEAN
WINAPI
SetupKmSession(
	_In_  PWCHAR  DriverPath,
	_Out_ PHANDLE hDriver
) {
	// create service of KM driver
	if(!CreateDeadwingService(DriverPath))
		return FALSE;

	// start it
	if(!StartDeadwingService())
		return FALSE;

	// open handle of the device
	HANDLE hOutput = CreateFileW(L"\\\\.\\DeadwingKM", (FILE_READ_ACCESS | FILE_WRITE_ACCESS), (FILE_SHARE_READ | FILE_SHARE_WRITE), NULL, OPEN_EXISTING, 0, NULL);
	if(hOutput == INVALID_HANDLE_VALUE) {
		wprintf(L"[ DeadwingUM ] Unable to open handle of the drivers device\n");
		return FALSE;
	}

	*hDriver = hOutput;

	return TRUE;
}

/**
 * \brief Entry point of the controller application
 * 
 * \returns 0 if the exit was made by the user. Otherwise - 1.
 */
INT 
WINAPI
main(
	VOID
) {
	wprintf(L"\t\t=[ DeadwingUM ]=\n");
	wprintf(L"[ An UM application for inter-mode communication ]\n\n");

	WCHAR Path[MAX_PATH] = { 0 };

	wprintf(L"[ DeadwingUM ] Provide path to the driver: ");
	wscanf(L"%s", Path);
	if(wcslen(Path) > MAX_PATH || Path == NULL) {
		wprintf(L"[ DeadwingUM ] Invalid driver path\n");
		return 1;
	}

	// open session
	HANDLE hDwDriver = NULL;
	if(!SetupKmSession(Path, &hDwDriver))
		return 1;

	wprintf(L"[ ! ] Service was initialized. Please, enter \"cache\" command\n");
	wprintf(L"[ ! ] This is necessary in order to save data about the system and controller processes\n");

	// dispatch user commands
	BOOLEAN TerminationSignal = FALSE;
	while(TRUE) {
		WCHAR Command[32] = { 0 };

		wprintf(L"\n[ DeadwingUM ] Enter command: ");
		wscanf(L"%s", Command);
		
		// exit form program if TRUE returned
		if(CmdMainDispatcher(Command, hDwDriver, &TerminationSignal))
			break;
	}

	TerminateKmSession(hDwDriver, TerminationSignal);

	wprintf(L"[ DeadwingUM ] Bye-bye!\n");

	system("pause");

	return 0;
}
