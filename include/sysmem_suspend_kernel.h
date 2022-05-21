/* Copyright (C) 2011, 2012, 2013 The uOFW team
   See the file COPYING for copying permission.
*/

#include "common_header.h"

s32 sceKernelRegisterResumeHandler(s32 reg, s32 (*handler)(s32 unk, void *param), void *param);
s32 sceKernelRegisterSuspendHandler(s32 reg, s32 (*handler)(s32 unk, void *param), void *param);

#define SCE_KERNEL_POWER_LOCK_DEFAULT       (0)
s32 sceKernelPowerLock(s32 lockType);
s32 sceKernelPowerLockForUser(s32 lockType);
s32 sceKernelPowerUnlock(s32 lockType);
s32 sceKernelPowerUnlockForUser(s32 lockType);

#define SCE_KERNEL_POWER_TICK_DEFAULT       (0) /** Cancel all timers. */
#define SCE_KERNEL_POWER_TICK_SUSPEND_ONLY	(1) /** Cancel auto-suspend-related timer. */				
#define SCE_KERNEL_POWER_TICK_LCD_ONLY		(6) /** Cancel LCD-related timer .*/	
s32 sceKernelPowerTick(s32 tickType);

#define SCE_KERNEL_VOLATILE_MEM_DEFAULT		(0)
s32 sceKernelVolatileMemLock(s32 unk, void **ptr, s32 *size);
s32 sceKernelVolatileMemTryLock(s32 unk, void **ptr, s32 *size);
s32 sceKernelVolatileMemUnlock(s32 unk);

s32 sceKernelPowerRebootStart(s32);

