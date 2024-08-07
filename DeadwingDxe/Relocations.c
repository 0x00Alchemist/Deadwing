#include <Uefi.h>

#include <Library/UefiLib.h>
#include <Library/BaseLib.h>

#include <IndustryStandard/PeImage.h>

#include "Globals.h"
#include "Utils.h"


/**
 * \brief Fixes image relocation
 * 
 * \param ImageBase Current image base
 * \param NewBase   New base
 * 
 * \returns TRUE if relocations fixed succesfully
 */
BOOLEAN
EFIAPI
RelProcessRelocTable(
	IN VOID *ImageBase, 
	IN VOID *NewBase
) {
	// get PE headers
	EFI_IMAGE_NT_HEADERS64 *Nt64 = GetNtHeaders(ImageBase);
	if(Nt64 == NULL)
		return FALSE;

	// get old image base & image delta
	UINT64 OldBase = Nt64->OptionalHeader.ImageBase;
	UINT32 ImageDelta = (UINT64)((UINT8 *)NewBase - (UINT8 *)OldBase);

	// process relocations
	EFI_IMAGE_DATA_DIRECTORY *RelocDir = &Nt64->OptionalHeader.DataDirectory[EFI_IMAGE_DIRECTORY_ENTRY_BASERELOC];
	if(RelocDir) {
		EFI_IMAGE_BASE_RELOCATION* Relocs = (EFI_IMAGE_BASE_RELOCATION *)((UINT8 *)ImageBase + RelocDir->VirtualAddress);
		UINT32 BlockSize = Relocs->SizeOfBlock;

		// get reloc block
		while(Relocs->SizeOfBlock && BlockSize > 0) {
			UINT16 *BlockRVA = (UINT16 *)(Relocs + EFI_IMAGE_SIZEOF_BASE_RELOCATION);
			UINT32 NumberOfBlocks = (UINT32)((Relocs->SizeOfBlock - EFI_IMAGE_SIZEOF_BASE_RELOCATION) / sizeof(UINT16));

			// traverse through reloc block
			for(INT16 i = 0; i < NumberOfBlocks; ++i) {
				UINT16 CurrentBlock = &BlockRVA[i];

				// get information about current reloc block
				if(CurrentBlock) {
					UINT64 *CurrentBase = (UINT64 *)((UINT8 *)ImageBase + Relocs->VirtualAddress);
					UINT32 CurrentRVA = Relocs->SizeOfBlock + (CurrentBlock & 0xFFF);
					UINT16 CurrentType = (CurrentBlock & 0xF000) >> 12;

					// resolve current block
					switch(CurrentType) {
						case EFI_IMAGE_REL_BASED_ABSOLUTE:
						break;
						case EFI_IMAGE_REL_BASED_DIR64:
							*CurrentBase += ImageDelta;
						break;
						default:
							return FALSE;
					}
				}
			}

			// get next relocation block
			Relocs = (EFI_IMAGE_BASE_RELOCATION *)(Relocs + Relocs->SizeOfBlock);
			BlockSize -= Relocs->SizeOfBlock;
		}
	}

	return TRUE;
}

/**
 * \brief Relocates entire image to new address
 * 
 * \param Image Image base
 * 
 * \return Address of new image base
 */
VOID *
EFIAPI
RelProcessSelfReloc(
	IN VOID *Image
) {
	// get PE headers
	EFI_IMAGE_NT_HEADERS64 *Nt64 = GetNtHeaders(Image);
	if(Nt64 == NULL)
		return NULL;

	// get size of image in pages
	UINT64 ImageSize = Nt64->OptionalHeader.SizeOfImage;
	UINT32 SizeInPages = EFI_SIZE_TO_PAGES(ImageSize) + 1;

	// allocate pages for image
	EFI_PHYSICAL_ADDRESS NewAddr;
	EFI_STATUS Status = gBS->AllocatePages(AllocateAnyPages, EfiRuntimeServicesCode, SizeInPages, &NewAddr);
	if(!EFI_ERROR(Status)) {
		// copy image to new address
		gBS->CopyMem((VOID *)NewAddr, Image, ImageSize);

		// get image base of new image
		Nt64 = GetNtHeaders((VOID *)NewAddr);
		if(Nt64 == NULL)
			return NULL;

		// process relocs
		if(!RelProcessRelocTable((VOID *)NewAddr, (VOID *)((UINT8 *)Nt64->OptionalHeader.ImageBase - (UINT8 *)Image + (UINT8 *)NewAddr))) {
			gBS->FreePages(NewAddr, SizeInPages);
			return NULL;
		}
	} else {
		return NULL;
	}

	return (VOID *)NewAddr;
}
