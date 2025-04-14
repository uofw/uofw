/* Copyright (C) The uOFW team
   See the file COPYING for copying permission.
*/

#include "common_header.h"

s32 sceKernelRegisterResumeHandler(s32 reg, s32 (*handler)(s32 unk, void *param), void *param);
s32 sceKernelRegisterSuspendHandler(s32 reg, s32 (*handler)(s32 unk, void *param), void *param);

typedef struct
{
    int size; // 0
    s32 (*tick)(s32); // 4
    s32 (*lock)(s32); // 8
    s32 (*unlock)(s32); // 12
    s32 (*lockForUser)(s32); // 16
    s32 (*unlockForUser)(s32); // 20
    s32 (*rebootStart)(s32); // 24
    s32 (*memLock)(s32, void**, SceSize *); // 28
    s32 (*memTryLock)(s32, void**, SceSize *); // 32
    s32 (*memUnlock)(s32); // 36
} ScePowerHandlers;

int sceKernelRegisterPowerHandlers(const ScePowerHandlers* handlers);

int sceKernelDispatchSuspendHandlers(int unk);
int sceKernelDispatchResumeHandlers(int unk);

#define SCE_KERNEL_POWER_LOCK_DEFAULT       (0)
s32 sceKernelPowerLock(s32 lockType);
s32 sceKernelPowerLockForUser(s32 lockType);
s32 sceKernelPowerUnlock(s32 lockType);
s32 sceKernelPowerUnlockForUser(s32 lockType);

/** Cancels all timers. */
#define SCE_KERNEL_POWER_TICK_DEFAULT			0     
/** Cancels only the timer related to automatic suspension. */
#define SCE_KERNEL_POWER_TICK_SUSPENDONLY		1
/** Cancels the timer related to the LCD. */
#define SCE_KERNEL_POWER_TICK_LCDONLY			6	
int sceKernelPowerTick(int tickType);

#define SCE_KERNEL_VOLATILE_MEM_DEFAULT		(0)
int sceKernelVolatileMemLock(int unk, void **ptr, SceSize *size);
int sceKernelVolatileMemTryLock(int unk, void **ptr, SceSize *size);
int sceKernelVolatileMemUnlock(int unk);

s32 sceKernelPowerRebootStart(s32);

int sceSuspendForKernel_67B59042(int arg0);
int sceSuspendForKernel_B2C9640B(int arg0);

