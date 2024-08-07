#pragma once

#include "Defs.h"

/// \note @0x00Alchemist: This API is exposed and provided by the DXE driver. 
/// To clarify the operation of this API, go to the DeadwingDxe/DxeMain.c 
typedef VOID(FASTCALL *__T_SetupCommunicationBuffer)(_In_ PDEADWING_COMMUNICATION, _In_ UINT64);
typedef PDEADWING_COMMUNICATION(FASTCALL *__T_DeadwingSmmCommunicate)(VOID);

__T_SetupCommunicationBuffer SetupCommunicationBuffer;
__T_DeadwingSmmCommunicate DeadwingSmmCommunicate;
