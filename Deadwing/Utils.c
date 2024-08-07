#include <Uefi.h>

#include <Library/UefiLib.h>
#include <Library/BaseLib.h>

#include <IndustryStandard/PeImage.h>

#include "Globals.h"
#include "Defs.h"
#include "Serial.h"
#include "Memory.h"

#define LOW_STUB_FIRST_BYTES        0x00000001000600E9
#define LOW_STUB_FIRST_BYTES_MOD    0x0000001000604DE9
#define NTOSKRNL_BASE_ADDRESS_START 0xfffff80000000000

#define ALPC_HANDLE_DATA_TYPE_VALUE 0x64486C4100000009ULL


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
 * \brief Searches MZ signature in specific range
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

		Entry -= EFI_PAGE_SIZE;
	}

	return NULL;
}
