#pragma once


VOID
EFIAPI
PhysMemCpy(
	IN VOID   *Dest,
	IN VOID   *Src,
	IN UINT32  Len
);

VOID
EFIAPI
MemRestoreSmramMappings(
	VOID
);

UINT64
EFIAPI
MemProcessOutsideSmramPhysMemory(
	IN UINT64 PhysAddress
);

UINT64
EFIAPI
MemTranslateVirtualToPhys(
	IN VOID   *Address,
	IN UINT64  Dir
);

UINT64
EFIAPI
MemMapVirtualAddress(
	IN VOID    *VirtualAddress,
	IN UINT64   DirBase,
	OUT VOID  **UnmappedAddress
);
