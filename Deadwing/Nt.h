#pragma once

UINT64
EFIAPI
NtGetDirBaseByPid(
	IN  UINT64   ProcessId,
	IN  VOID    *SysEprocess,
	IN  UINT64   SysDir,
	OUT VOID   **TargetEprocess OPTIONAL
);

EFI_STATUS
EFIAPI
NtExchageProcessToken(
	IN UINT64 SysEprocess,
	IN UINT64 ControllerEprocess
);
