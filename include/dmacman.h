/* Copyright (C) 2011, 2012 The uOFW team
   See the file COPYING for copying permission.
*/

#include "common_header.h"

typedef struct SceDmaOp {
    struct SceDmaOp *unk0; // Prev (for unrelated ops)
    struct SceDmaOp *unk4; // Next (for unrelated ops)
    struct SceDmaOp *unk8; // Prev (for linked ops)
    struct SceDmaOp *next; // Next (for linked ops)
    u32 unk16;
    u32 unk20;
    int (*callback)(int, int);
    u16 unk28;      // Status
    u16 unk30;
    u32 unk32;
    u32 unk36;
    u32 unk40;
    u32 unk44;
    u32 unk48;
    u16 unk52;      // id?
    u16 unk54;      // num ops in link
    u32 unk56;
    u32 unk60;
} SceDmaOp;

int sceKernelDmaOpQuit(SceDmaOp*);
int sceKernelDmaOpAssign(SceDmaOp*, int, int, int, int);
int sceKernelDmaOpSetCallback(SceDmaOp*, int (*)(int, int), int);
int sceKernelDmaOpSetupLink(SceDmaOp*, int, u32*);
int sceKernelDmaOpEnQueue(SceDmaOp*);
int sceKernelDmaOpDeQueue(SceDmaOp*);
SceDmaOp *sceKernelDmaOpAlloc(void);
s32 sceKernelDmaOpFree(SceDmaOp*);
int DmacManForKernel_E18A93A5(void*, void*);

