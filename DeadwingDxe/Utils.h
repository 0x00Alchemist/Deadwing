#pragma once

#include <IndustryStandard/PeImage.h>

VOID
EFIAPI
CopyMemory(
	IN UINT8 *Dest,
	IN UINT8 *Src,
	IN UINTN  Length
);

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
