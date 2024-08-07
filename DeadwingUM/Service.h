#pragma once

BOOLEAN
WINAPI
StartDeadwingService(
	VOID
);

VOID
WINAPI
StopDeadwingService(
	VOID
);

BOOLEAN
WINAPI
DeleteDeadwingService(
	VOID
);

BOOLEAN
WINAPI
CreateDeadwingService(
	_In_ PWCHAR DriverPath
);
