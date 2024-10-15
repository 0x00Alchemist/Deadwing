#pragma once

namespace Commands {
	bool
	WINAPI
	CmdMainDispatcher(
		_In_  Deadwing::Commands *DwCommands,
		_In_  wchar_t            *Command,
		_Out_ bool               *TerminationSignal
	);
}
