#pragma once

#include <Library/SmmServicesTableLib.h>

EFI_SYSTEM_TABLE *gST;
EFI_BOOT_SERVICES *gBS;
EFI_RUNTIME_SERVICES *gRT;

EFI_SMM_SYSTEM_TABLE2 *gSmst2;

EFI_PHYSICAL_ADDRESS gRemapPage;
