#pragma once

CHAR8 *gEfiCallerBaseName = "DeadwingSMM";
CONST UINT32 _gUefiDriverRevision = 0;
CONST UINT8  _gDriverUnloadImageCount = 0;

BOOLEAN mPostEBS = FALSE;
EFI_SYSTEM_TABLE *mDebugST = NULL;