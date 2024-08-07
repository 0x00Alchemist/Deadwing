#pragma once

BOOLEAN
EFIAPI
RelProcessRelocTable(
	IN VOID *ImageBase,
	IN VOID *NewBase
);

VOID *
EFIAPI
RelProcessSelfReloc(
	IN VOID *Image
);
