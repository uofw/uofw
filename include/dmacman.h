/* Copyright (C) 2011, 2012 The uOFW team
   See the file COPYING for copying permission.
*/

#include "common_header.h"

typedef struct PL080_CHREGS PL080_CHREGS, *PPL080_CHREGS;
typedef struct PL080_LLI    PL080_LLI,    *PPL080_LLI;
typedef struct PL080_REGS   PL080_REGS,   *PPL080_REGS;

struct PL080_CHREGS {
    u32 SrcAddr;
    u32 DestAddr;
    u32 LLI;
    u32 Control;
    u32 Configuration;
    u32 _14_1C[3];
};

struct PL080_LLI {
    u32 SrcAddr;
    u32 DestAddr;
    u32 LLI;
    u32 Control;
};

struct PL080_REGS {
    u32 IntStatus;
    u32 IntTCStatus;
    u32 IntTCClear;
    u32 IntErrorStatus;
    u32 IntErrClr;
    u32 RawIntTCStatus;
    u32 RawIntErrorStatus;
    u32 EnbldChns;
    u32 SoftBReq;
    u32 SoftSReq;
    u32 SoftLBReq;
    u32 SoftLSReq;
    u32 Configuration;
    u32 Sync;
    u32 SReqMask;
    u32 _03C_0FC[49];
    struct PL080_CHREGS Ch[8];
    u32 _200_FDC[888];
    u32 PeriphID0;
    u32 PeriphID1;
    u32 PeriphID2;
    u32 PeriphID3;
    u32 CellID0;
    u32 CellID1;
    u32 CellID2;
    u32 CellID3;
};

typedef struct sceKernelDmaOperation sceKernelDmaOperation, *PsceKernelDmaOperation;

struct sceKernelDmaOperation {
    struct sceKernelDmaOperation * pNext; // 0
    struct sceKernelDmaOperation * pPrev; // 4
    struct sceKernelDmaOperation * pParent; // 8
    struct sceKernelDmaOperation * pChild; // 12
    int    (* cbfunc)(struct sceKernelDmaOperation *, int, int, void *); // 16
    void    * pCookie; // 20
    u32       evpat; // 24
    SceUShort16 uiFlag; // 28
    short     dmaCh; // 30
    SceUInt   uiSrcAddr; // 32
    SceUInt   uiDestAddr; // 36
    SceUInt   uiLLI; // 40
    SceUInt   uiControl; // 44
    SceUInt   uiConfiguration; // 48
    SceUShort16 uiChMask[2]; // 52
    struct PL080_LLI * HeadLLI; // 56
    struct PL080_LLI * EndLLI; // 60
};

int sceKernelDmaOpQuit(sceKernelDmaOperation*);
int sceKernelDmaOpAssign(sceKernelDmaOperation*, int, int, int, int);
int sceKernelDmaOpSetCallback(sceKernelDmaOperation*, int (*)(int, int), int);
int sceKernelDmaOpSetupLink(sceKernelDmaOperation*, int, u32*);
int sceKernelDmaOpEnQueue(sceKernelDmaOperation*);
int sceKernelDmaOpDeQueue(sceKernelDmaOperation*);
sceKernelDmaOperation *sceKernelDmaOpAlloc(void);
s32 sceKernelDmaOpFree(sceKernelDmaOperation*);
int sceKernelDmaOpConcatenate(sceKernelDmaOperation*, void*);

