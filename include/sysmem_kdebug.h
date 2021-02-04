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

/** 
 * Boot parameter 49 of the PSP Development Tool (DTP-T1000).
 * 
 * Defines if initial clock frequency limits imposed by the system when a game/app/updater
 * process is launched should be discarded and to impose no frequency limits.
 * 
 * 0: PSP system operates with default clock frequency limits. For the PSP-1000 series, for example, this means
 * that the PLL is set to operate at a clock frequency of 222MHz and WLAN can only operate at a PLL clock
 * frequency of 222 MHz as well.
 * 1: PSP system operates at maximum clock frequencies on startup and without clock frequency limits. For the
 * PSP-1000 series, for example, this means that the PLL is set to operate at its maximum allowed clock frequency
 * of 333MHz and that WLAN can also operate at maximum PLL clock frequency.
 */
#define PSP_DIPSW_BIT_GAME_APP_UPDATER_PSP_CLOCK_FREQUENCIES_NO_LIMIT       49

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

