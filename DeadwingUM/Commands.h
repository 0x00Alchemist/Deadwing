#pragma once

BOOLEAN
WINAPI
CmdMainDispatcher(
	_In_  PWCHAR   Command,
	_In_  HANDLE   hDriverHandle,
	_Out_ PBOOLEAN TerminationSignal
);
