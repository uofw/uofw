/* Copyright (C) 2011, 2012 The uOFW team
   See the file COPYING for copying permission.
*/

#include "common.h"

/** The PSP SDK defines this as PSP_POWER_TICK_ALL. Cancels all timers. */
#define SCE_KERNEL_POWER_TICK_DEFAULT           0 

int sceKernelRegisterResumeHandler(int reg, int (*handler)(int unk, void *param), void *param);
int sceKernelRegisterSuspendHandler(int reg, int (*handler)(int unk, void *param), void *param);

int sceKernelPowerLock(int);
int sceKernelPowerLockForUser(int);
int sceKernelPowerUnlock(int);
int sceKernelPowerUnlockForUser(int);
int sceKernelPowerTick(int);

