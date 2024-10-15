#pragma once

namespace Service {
	bool
	WINAPI
	StartDeadwingService(
		void
	);

	void
	WINAPI
	StopDeadwingService(
		void
	);

	bool
	WINAPI
	DeleteDeadwingService(
		void
	);

	bool
	WINAPI
	CreateDeadwingService(
		_In_ const wchar_t *DriverPath
	);
}
