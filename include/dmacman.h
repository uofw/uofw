/* Copyright (C) 2011, 2012 The uOFW team
   See the file COPYING for copying permission.
*/

#include "common_header.h"

typedef struct {
    void *unk0;
    u32 unk4;
    u32 unk8;
    SceDmaOp *next;
    u32 unk16;
    u32 unk20;
    int (*)(int, int) callback;
    u16 unk28;
    u16 unk30;
    u32 unk32;
    u32 unk36;
    u32 unk40;
    u32 unk44;
    u32 unk48;
    u32 unk56;
} SceDmaOp;

int sceKernelDmaOpQuit(SceDmaOp*);
int sceKernelDmaOpAssign(u32*, int, int, int, int);
int sceKernelDmaOpSetCallback(u32*, int (*)(int, int), int);
int sceKernelDmaOpSetupLink(u32*, int, u32*);
int sceKernelDmaOpEnQueue(u32*);
int sceKernelDmaOpDeQueue(u32*);
u32 *sceKernelDmaOpAlloc(void);
s32 sceKernelDmaOpFree(u32*);
int DmacManForKernel_E18A93A5(void*, void*);

