#include <Uefi.h>

#include <Library/UefiLib.h>
#include <Library/BaseLib.h>

#include <IndustryStandard/PeImage.h>

#include "Globals.h"


/**
 * Copies memory to destination address
 * 
 * \param Dest   Destination address
 * \param Src    Source data which will be copied to destination address
 * \param Length Length of data to copy
 */
VOID
EFIAPI
CopyMemory(
	IN UINT8 *Dest,
	IN UINT8 *Src,
	IN UINTN  Length
) {
	for (UINT8* D = Dest, *S = Src; Length--; *D++ = *S++);
}

/**
 * \brief Gets NT headers
 * 
 * \param ImageBase Image base
 * 
 * \returns NT headers 
 */
EFI_IMAGE_NT_HEADERS64 *
EFIAPI
GetNtHeaders(
	IN VOID *ImageBase
) {
	EFI_IMAGE_DOS_HEADER *Dos;
	EFI_IMAGE_NT_HEADERS64 *Nt64;

	Dos = (EFI_IMAGE_DOS_HEADER *)ImageBase;
	if(Dos->e_magic != EFI_IMAGE_DOS_SIGNATURE)
		return NULL;

	Nt64 = (EFI_IMAGE_NT_HEADERS64 *)((UINT8 *)ImageBase + Dos->e_lfanew);
	if(Nt64->Signature != EFI_IMAGE_NT_SIGNATURE)
		return NULL;

	return Nt64;
}

/**
 * \biref Searches MZ signature in specific range
 * 
 * \param Entry Start address
 * 
 * \returns Address of target image
 */
VOID *
EFIAPI
FindBaseAddress(
	IN UINT64 Entry
) {
	UINT32 MaxPages;
	EFI_IMAGE_DOS_HEADER *Dos;

	/// \note @0x00Alchemist: scan around 32 pages.
	/// This size should be sufficient, otherwise you need to expand the range
	MaxPages = 32;

	Entry &= ~(EFI_PAGE_SIZE - 1);
	while(MaxPages--) {
		Dos = (EFI_IMAGE_DOS_HEADER *)Entry;

		// check if current address contains MZ signature
		if(Dos->e_magic == EFI_IMAGE_DOS_SIGNATURE)
			return (VOID *)Entry;

		Entry += EFI_PAGE_SIZE;
	}

	return NULL;
}