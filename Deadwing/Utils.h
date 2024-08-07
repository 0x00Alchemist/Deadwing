#pragma once

#include <IndustryStandard/PeImage.h>

#include "Defs.h"


EFI_IMAGE_NT_HEADERS64 *
EFIAPI
GetNtHeaders(
	IN VOID *ImageBase
);

VOID *
EFIAPI
FindBaseAddress(
	IN UINT64 Entry
);
