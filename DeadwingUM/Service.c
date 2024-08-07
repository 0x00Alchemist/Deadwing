#include <Windows.h>
#include <stdio.h>

#pragma comment(lib, "advapi32.lib")


/**
 * \brief Starts Deadwing service
 *
 * \return TRUE - Service has been started
 * \return FALSE - Unable to start Deadwing service
 */
BOOLEAN
WINAPI
StartDeadwingService(
	VOID
) {
	SC_HANDLE hSc = OpenSCManagerW(NULL, NULL, SC_MANAGER_ALL_ACCESS);
	if(hSc == NULL) {
		wprintf(L"[ DeadwingUM ] Unable to open SC Manager!\n");
		return FALSE;
	}

	SC_HANDLE hService = OpenServiceW(hSc, L"Deadwing", (SERVICE_START | SERVICE_STOP | DELETE));
	if(hService == NULL) {
		wprintf(L"[ DeadwingUM ] Cannot open service!\n");
		return FALSE;
	}

	if(!StartServiceW(hService, 0, NULL)) {
		DWORD dwErr = GetLastError();
		if(dwErr == ERROR_SERVICE_ALREADY_RUNNING) {
			wprintf(L"[ DeadwingUM ] Service already running!\n");
		} else {
			wprintf(L"[ DeadwingUM ] Unable to start service!\n");
			return FALSE;
		}
	} else {
		wprintf(L"[ DeadwingUM ] Service was started succesfully\n");
	}

	CloseServiceHandle(hService);
	CloseServiceHandle(hSc);

	return TRUE;
}

/**
 * \brief Stops Deadwing service
 */
VOID
WINAPI
StopDeadwingService(
	VOID
) {
	SC_HANDLE hSc = OpenSCManagerW(NULL, NULL, SC_MANAGER_ALL_ACCESS);
	if(hSc == NULL) {
		wprintf(L"[ DeadwingUM ] Unable to open SC Manager!\n");
		return;
	}

	SC_HANDLE hService = OpenServiceW(hSc, L"Deadwing", (SERVICE_START | SERVICE_STOP | DELETE));
	if(hService == NULL) {
		wprintf(L"[ DeadwingUM ] Cannot open service!\n");
		return;
	}

	SERVICE_STATUS SvStatus = { 0 };
	if(!ControlService(hService, SERVICE_CONTROL_STOP, &SvStatus)) {
		wprintf(L"[ DeadwingUM ] Cannot stop service!\n");
	} else {
		wprintf(L"[ DeadwingUM ] Service was stopped succesfully\n");
	}

	CloseServiceHandle(hService);
	CloseServiceHandle(hSc);
}

/**
 * \brief Deletes DeadwingService
 *
 * \return TRUE - Service has been deleted
 * \return FALSE - Unable to delete service
 */
BOOLEAN
WINAPI
DeleteDeadwingService(
	VOID
) {
	SC_HANDLE hSc = OpenSCManagerW(NULL, NULL, SC_MANAGER_ALL_ACCESS);
	if(hSc == NULL) {
		wprintf(L"[ DeadwingUM ] Unable to open SC Manager!\n");
		return FALSE;
	}

	SC_HANDLE hService = OpenServiceW(hSc, L"Deadwing", (SERVICE_START | SERVICE_STOP | DELETE));
	if(hService == NULL) {
		wprintf(L"[ DeadwingUM ] Cannot open service!\n");
		return FALSE;
	}

	if(!DeleteService(hService)) {
		wprintf(L"[ DeadwingUM ] Cannot delete DeadwingService! You can delete it manually\n");
	} else {
		wprintf(L"[ DeadwingUM ] Succesfully deleted Deadwing service\n");
	}

	CloseServiceHandle(hService);
	CloseServiceHandle(hSc);

	return TRUE;
}

/**
 * \brief Creates Deadwing service
 *
 * \param DriverPath Path to Deadwing driver
 *
 * \return TRUE - Service has been created
 * \return FALSE - Unable to create service
 */
BOOLEAN
WINAPI
CreateDeadwingService(
	_In_ PWCHAR DriverPath
) {
	SC_HANDLE hSc = OpenSCManagerW(NULL, NULL, SC_MANAGER_CREATE_SERVICE);
	if(hSc == NULL) {
		wprintf(L"[ DeadwingUM ] Unable to open SC Manager!\n");
		return FALSE;
	}

	SC_HANDLE hService = CreateServiceW(hSc, L"Deadwing", L"Deadwing", (SERVICE_START | SERVICE_STOP | DELETE), SERVICE_KERNEL_DRIVER, SERVICE_DEMAND_START, SERVICE_ERROR_IGNORE, DriverPath, NULL, NULL, NULL, NULL, NULL);
	if(hService == NULL) {
		DWORD dwError = GetLastError();
		if(dwError == ERROR_SERVICE_EXISTS) {
			/// \note @0x00Alchemist: we're not uninstalling service after exit, just stopping it
			wprintf(L"[ DeadwingUM ] Service already exists, we'll start it..\n");
		} else {
			wprintf(L"[ DeadwingUM ] Cannot create service!\n");
			return FALSE;
		}
	}

	wprintf(L"[ DeadwingUM ] Deadwing service initialized\n");

	CloseServiceHandle(hService);
	CloseServiceHandle(hSc);

	return TRUE;
}
