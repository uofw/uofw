/* Copyright (C) 2011, 2012 The uOFW team
   See the file COPYING for copying permission.
*/

#include "common_header.h"

typedef struct {   
    int size;
    int (*ops[])();
} SceKernelDeci2Ops;

int sceKernelDeci2pRegisterOperations(SceKernelDeci2Ops *ops);
SceKernelDeci2Ops *sceKernelDeci2pReferOperations(void);

void *sceKernelSm1ReferOperations();

void Kprintf(const char *format, ...);

// TODO: Add more boot parameters and apply them throughout uofw.

/** Boot parameter 11 of the PSP Development Tool (DTP-T1000). */
#define PSP_DIPSW_BIT_PLL_WLAN_COEXISTENCY_CLOCK            11

/* Valid values for boot parameter 11. */

#define PSP_DIPSW_PLL_WLAN_COEXISTENCY_CLOCK_222MHz         0 /* Corresponds to the PSP-1000 series */
#define PSP_DIPSW_PLL_WLAN_COEXISTENCY_CLOCK_333MHz         1 /* COrresponds to the PSP-2000 series and later */

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

