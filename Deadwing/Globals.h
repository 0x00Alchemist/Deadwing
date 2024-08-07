#pragma once

#include <Library/SmmServicesTableLib.h>

EFI_SYSTEM_TABLE *gST;
EFI_BOOT_SERVICES *gBS;
EFI_RUNTIME_SERVICES *gRT;

EFI_SMM_SYSTEM_TABLE2 *gSmst2;

EFI_PHYSICAL_ADDRESS gRemapPage;

STATIC CONST EFI_GUID gDeadwingSmiHandlerGuid = { 0x2BFADA50, 0xAF38, 0x49A1, { 0x85, 0x34, 0x08, 0xF6, 0xAE, 0x1B, 0x4C, 0x96 } };
