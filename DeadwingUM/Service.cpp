#include <Windows.h>
#include <iostream>

#pragma comment(lib, "advapi32.lib")


namespace Service {
	/**
	* \brief Starts Deadwing service
	*
	* \return TRUE - Service has been started
	* \return FALSE - Unable to start Deadwing service
	*/
	bool
	WINAPI
	StartDeadwingService(
		void
	) {
		SC_HANDLE hSc = OpenSCManagerW(nullptr, nullptr, SC_MANAGER_ALL_ACCESS);
		if(hSc == nullptr) {
			std::wprintf(L"[ DwUM ] Unable to open SC Manager!\n");
			return false;
		}

		SC_HANDLE hService = OpenServiceW(hSc, L"Deadwing", (SERVICE_START | SERVICE_STOP | DELETE));
		if(hService == nullptr) {
			std::wprintf(L"[ DwUM ] Cannot open service!\n");
			return false;
		}

		if(!StartServiceW(hService, 0, nullptr)) {
			DWORD dwErr = GetLastError();
			if(dwErr == ERROR_SERVICE_ALREADY_RUNNING) {
				std::wprintf(L"[ DwUM ] Service already running!\n");
			} else {
				std::wprintf(L"[ DwUM ] Unable to start service!\n");
				return false;
			}
		} else {
			wprintf(L"[ DwUM ] Service was started succesfully\n");
		}

		CloseServiceHandle(hService);
		CloseServiceHandle(hSc);

		return true;
	}

	/**
	 * \brief Stops Deadwing service
	 */
	void
	WINAPI
	StopDeadwingService(
		void
	) {
		SC_HANDLE hSc = OpenSCManagerW(nullptr, nullptr, SC_MANAGER_ALL_ACCESS);
		if(hSc == nullptr) {
			std::wprintf(L"[ DwUM ] Unable to open SC Manager!\n");
			return;
		}

		SC_HANDLE hService = OpenServiceW(hSc, L"Deadwing", (SERVICE_START | SERVICE_STOP | DELETE));
		if(hService == nullptr) {
			std::wprintf(L"[ DwUM ] Cannot open service!\n");
			return;
		}

		SERVICE_STATUS SvStatus = { 0 };
		if(!ControlService(hService, SERVICE_CONTROL_STOP, &SvStatus)) {
			std::wprintf(L"[ DwUM ] Cannot stop service!\n");
		} else {
			std::wprintf(L"[ DwUM ] Service was stopped succesfully\n");
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
	bool
	WINAPI
	DeleteDeadwingService(
		void
	) {
		SC_HANDLE hSc = OpenSCManagerW(nullptr, nullptr, SC_MANAGER_ALL_ACCESS);
		if(hSc == nullptr) {
			std::wprintf(L"[ DwUM ] Unable to open SC Manager!\n");
			return false;
		}

		SC_HANDLE hService = OpenServiceW(hSc, L"Deadwing", (SERVICE_START | SERVICE_STOP | DELETE));
		if(hService == nullptr) {
			std::wprintf(L"[ DwUM ] Cannot open service!\n");
			return false;
		}

		if(!DeleteService(hService)) {
			std::wprintf(L"[ DwUM ] Cannot delete DeadwingService! You can delete it manually\n");
		} else {
			std::wprintf(L"[ DwUM ] Succesfully deleted Deadwing service\n");
		}

		CloseServiceHandle(hService);
		CloseServiceHandle(hSc);

		return true;
	}

	/**
	 * \brief Creates Deadwing service
	 *
	 * \param DriverPath Path to Deadwing driver
	 *
	 * \return TRUE - Service has been created
	 * \return FALSE - Unable to create service
	 */
	bool
	WINAPI
	CreateDeadwingService(
		_In_ const wchar_t *DriverPath
	) {
		SC_HANDLE hSc = OpenSCManagerW(nullptr, nullptr, SC_MANAGER_CREATE_SERVICE);
		if(hSc == nullptr) {
			wprintf(L"[ DwUM ] Unable to open SC Manager!\n");
			return false;
		}

		SC_HANDLE hService = CreateServiceW(hSc, L"Deadwing", L"Deadwing", (SERVICE_START | SERVICE_STOP | DELETE), SERVICE_KERNEL_DRIVER, SERVICE_DEMAND_START, SERVICE_ERROR_IGNORE, DriverPath, nullptr, nullptr, nullptr, nullptr, nullptr);
		if(hService == nullptr) {
			DWORD dwError = GetLastError();
			if(dwError == ERROR_SERVICE_EXISTS) {
				/// \note @0x00Alchemist: service still exist after previous session
				std::wprintf(L"[ DwUM ] Service already exists, we'll start it..\n");
			} else {
				std::wprintf(L"[ DwUM ] Cannot create service!\n");
				return false;
			}
		}

		std::wprintf(L"[ DwUM ] Deadwing service initialized\n");

		CloseServiceHandle(hService);
		CloseServiceHandle(hSc);

		return true;
	}

}
