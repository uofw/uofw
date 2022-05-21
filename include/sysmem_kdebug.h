/* Copyright (C) 2011, 2012 The uOFW team
   See the file COPYING for copying permission.
*/

#include "common_header.h"

#define SCE_DECI2OP_GE_SETOPS 6
#define SCE_DECI2OP_GE_BREAK 7
#define SCE_DECI2OP_GE_PUT_BP 8

typedef struct {   
    int size;
    int (*ops[])();
} SceKernelDeci2Ops;

int sceKernelDeci2pRegisterOperations(SceKernelDeci2Ops *ops);
SceKernelDeci2Ops *sceKernelDeci2pReferOperations(void);

void *sceKernelSm1ReferOperations();

void Kprintf(const char *format, ...);

int sceKernelDipsw(u32 reg);
u32 sceKernelDipswAll();
u32 sceKernelDipswLow32();
u32 sceKernelDipswHigh32();
int sceKernelDipswSet(u32 reg);
int sceKernelDipswClear(u32 reg);
int sceKernelDipswCpTime(void);

int sceKernelIsToolMode(void);
int sceKernelIsDevelopmentToolMode(void);
int sceKernelIsDVDMode(void);

int sceKernelDebugWrite(SceUID fd, const void *data, SceSize size);
int sceKernelDebugRead(SceUID fd, const void *data, SceSize size);
int sceKernelDebugEcho(void);
void sceKernelRegisterDebugPutcharByBootloader(void (*func)(short*, int));

void sceKernelRegisterAssertHandler(void (*func)(int));
void sceKernelAssert(int test, int lvl);

