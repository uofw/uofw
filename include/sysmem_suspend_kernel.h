/* Copyright (C) 2011, 2012, 2013 The uOFW team
   See the file COPYING for copying permission.
*/

#include "common_header.h"

int sceKernelRegisterResumeHandler(int reg, int (*handler)(int unk, void *param), void *param);
int sceKernelRegisterSuspendHandler(int reg, int (*handler)(int unk, void *param), void *param);

#define SCE_KERNEL_POWER_LOCK_DEFAULT       (0)
int sceKernelPowerLock(int lockType);
int sceKernelPowerLockForUser(int lockType);
int sceKernelPowerUnlock(int lockType);
int sceKernelPowerUnlockForUser(int lockType);

#define SCE_KERNEL_POWER_TICK_DEFAULT       (0) /** Cancel all timers. */
#define SCE_KERNEL_POWER_TICK_SUSPEND_ONLY	(1) /** Cancel auto-suspend-related timer. */				
#define SCE_KERNEL_POWER_TICK_LCD_ONLY		(6) /** Cancel LCD-related timer .*/	
int sceKernelPowerTick(int tickType);

#define SCE_KERNEL_VOLATILE_MEM_DEFAULT		(0)
int sceKernelVolatileMemLock(int unk, void **ptr, int *size);
int sceKernelVolatileMemTryLock(int unk, void **ptr, int *size);
int sceKernelVolatileMemUnlock(int unk);

int sceKernelPowerRebootStart(int);

