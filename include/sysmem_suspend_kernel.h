/* Copyright (C) The uOFW team
   See the file COPYING for copying permission.
*/

#include "common_header.h"

int sceKernelRegisterResumeHandler(int reg, int (*handler)(int unk, void *param), void *param);
int sceKernelRegisterSuspendHandler(int reg, int (*handler)(int unk, void *param), void *param);

typedef struct
{
    int size; // 0
    int (*tick)(int); // 4
    int (*lock)(int); // 8
    int (*unlock)(int); // 12
    int (*lockForUser)(int); // 16
    int (*unlockForUser)(int); // 20
    int (*rebootStart)(int); // 24
    int (*memLock)(int, void**, SceSize *); // 28
    int (*memTryLock)(int, void**, SceSize *); // 32
    int (*memUnlock)(int); // 36
} ScePowerHandlers;

int sceKernelRegisterPowerHandlers(const ScePowerHandlers* handlers);

int sceKernelDispatchSuspendHandlers(int unk);
int sceKernelDispatchResumeHandlers(int unk);

#define SCE_KERNEL_POWER_LOCK_DEFAULT       (0)
int sceKernelPowerLock(int lockType);
int sceKernelPowerLockForUser(int lockType);
int sceKernelPowerUnlock(int lockType);
int sceKernelPowerUnlockForUser(int lockType);

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

int sceKernelPowerRebootStart(int);

int sceSuspendForKernel_67B59042(int arg0);
int sceSuspendForKernel_B2C9640B(int arg0);

