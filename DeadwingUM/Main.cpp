#define _CRT_SECURE_NO_WARNINGS

#include <Windows.h>
#include <iostream>

#include "DeadwingCppLib.hpp"
#include "Service.hpp"
#include "Commands.hpp"

/**
 * \brief Terminates current driver session
 *
 * \param ServiceTermination Logical value which signals if service should be deleted
 */
void
WINAPI
TerminateKmSession(
	_In_ bool ServiceTermination
) {
	Service::StopDeadwingService();

	// delete service if user wants it
	if(ServiceTermination)
		Service::DeleteDeadwingService();
}

int
WINAPI
main(
	void
) {
	std::wprintf(L"\t\t  =[ DwUM ]=  \n");
	std::wprintf(L"[ An UM application for inter-mode communication ]\n\n");

	wchar_t Path[MAX_PATH] = { 0 };

	std::wprintf(L"[ DwUM ] Provide path to the driver: ");
	std::wscanf(L"%s", Path);
	if(std::wcslen(Path) > MAX_PATH || Path == NULL) {
		wprintf(L"[ DwUM ] Invalid driver path\n");
		return 1;
	}

	if(!Service::CreateDeadwingService(Path))
		return 1;

	if(!Service::StartDeadwingService())
		return 1;

	// open session
	Deadwing::Commands DwCommands;
	if(!DwCommands.InitializeSession(L"\\\\.\\DeadwingKM")) {
		std::wprintf(L"[ DwUM ] Unable to initialize session\n");
		return 1;
	}

	wprintf(L"[ ! ] Service and session was initialized\n");

	// dispatch user commands
	bool TerminationSignal = false;
	while(true) {
		wchar_t Command[32] = { 0 };

		std::wprintf(L"\n[ DwUM ] Enter command: ");
		std::wscanf(L"%s", Command);

		// exit form program if true returned
		if(Commands::CmdMainDispatcher(&DwCommands, Command, &TerminationSignal))
			break;
	}

	TerminateKmSession(TerminationSignal);

	std::wprintf(L"[ DwUM ] Bye-bye!\n");
	std::system("pause");

	return 0;
}