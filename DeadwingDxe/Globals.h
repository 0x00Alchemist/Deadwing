#pragma once

#include <Protocol/MmCommunication2.h>

EFI_SYSTEM_TABLE *gST;
EFI_BOOT_SERVICES *gBS;
EFI_RUNTIME_SERVICES *gRT;

EFI_MM_COMMUNICATION2_PROTOCOL *gMmCommunicate2;

STATIC CONST EFI_GUID gDeadwingTransferVarGuid = { 0xE8E00F56, 0x2350, 0x49BF, { 0x9E, 0x25, 0x3A, 0x36, 0x8E, 0x8B, 0xB3, 0x73 } };
STATIC CONST EFI_GUID gDeadwingSmiHandlerGuid = { 0x2BFADA50, 0xAF38, 0x49A1, { 0x85, 0x34, 0x08, 0xF6, 0xAE, 0x1B, 0x4C, 0x96 } };

UINTN gCommSize;
VOID *gPhysCommBuf;
VOID *gCommBuf;

EFI_EVENT gGoneVirtual;
EFI_EVENT gRegNotify;
EFI_EVENT gExitBs;
