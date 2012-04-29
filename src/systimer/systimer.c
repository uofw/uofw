/* Copyright (C) 2011, 2012 The uOFW team
   See the file COPYING for copying permission.
*/

#include "../common/common.h"

#include "../sysmem/sysmem.h"

#include "systimer.h"

PSP_SDK_VERSION(0x06060010);
PSP_MODULE_INFO("sceSystimer", 0x1007, 1, 2);
PSP_MODULE_BOOTSTART("STimerInit");

typedef struct {
    int unk0, unk4, unk8, unk12;
    int unused[240];
    int unk256;
} SceHwTimer;

typedef struct {
    SceHwTimer *hw;
    int intNum;
    int curTimer;
    int unk12, unk16;
    SceSysTimerCb cb;
    int unk24, unk28;
} SceSysTimer;

SceSysTimer timers[] = 
{
    { (void*)0xBC500000, 0x0F, -1, 0, 0, NULL, 0, 0 },
    { (void*)0xBC500010, 0x10, -1, 0, 0, NULL, 0, 0 },
    { (void*)0xBC500020, 0x11, -1, 0, 0, NULL, 0, 0 },
    { (void*)0xBC500030, 0x12, -1, 0, 0, NULL, 0, 0 }
};

typedef struct {
    int unk0, unk4, unk8;
} SceSysTimerSave;

SceSysTimerSave timerSave[4];

int initVar = 0x00352341; // 0x0BA0

int systimerhandler(int arg0, SceSysTimer *arg1, int arg2);
void _sceSTimerStopCount(SceSysTimer *arg);
int _sceSTimerGetCount(SceSysTimer *arg);
int suspendSTimer();
int resumeSTimer();

int STimerInit()
{
    int oldIntr = sceKernelCpuSuspendIntr();
    // 0328
    int i;
    for (i = 0; i < 4; i++)
    {
        timers[i].hw->unk0 = 0x80000000;
        timers[i].hw->unk8 = -1;
        timers[i].hw->unk12 = -1;
        (void)timers[i].hw->unk0;
        timers[i].cb = NULL;
        timers[i].unk12 = 0;
        timers[i].unk16 = 0;
        timers[i].unk24 = 0;
        timers[i].unk28 = 0;
        timers[i].curTimer = -1;
        sceKernelRegisterIntrHandler(timers[i].intNum, 2, systimerhandler, &timers[i], 0);
    }
    sceKernelRegisterSuspendHandler(10, suspendSTimer, 0);
    sceKernelRegisterResumeHandler(10, resumeSTimer, 0);
    sceKernelCpuResumeIntr(oldIntr);
    return 0;
}

int module_reboot_before()
{
    int oldIntr = sceKernelCpuSuspendIntr();
    // 03F8
    int i;
    for (i = 0; i < 4; i++) {
        _sceSTimerStopCount(&timers[i]);
        sceKernelReleaseIntrHandler(timers[i].intNum);
    }
    sceKernelCpuResumeIntr(oldIntr);
    return 0;
}

// 00E0
int systimerhandler(int arg0 __attribute__((unused)), SceSysTimer *arg1, int arg2)
{
    if (arg1->cb == NULL)
        return -1;
    int var = arg1->hw->unk0;
    int v1 = var >> 24;
    int v2 = var & 0x003FFFFF;
    if (arg1->unk12 != 0) {
        v1--;
        arg1->unk24 += arg1->unk12;
    }
    // 0134
    arg1->unk24 += v2 * (v1 - arg1->unk16);
    arg1->unk12 = 0;
    arg1->unk16 = 0;
    if (arg1->cb(arg1->curTimer, arg1->unk24 + _sceSTimerGetCount(arg1), arg1->unk28, arg2) != -1)
    {
        // 01A4
        _sceSTimerStopCount(arg1);
        return -2;
    }
    return -1;
}

// 02B0
void _sceSTimerStopCount(SceSysTimer *arg)
{
    arg->hw->unk0 = arg->hw->unk256 & 0x7F7FFFFF;
}

// 02CC
int _sceSTimerGetCount(SceSysTimer *arg)
{
    return (arg->hw->unk256 & 0x003FFFFF) - (arg->hw->unk4 & 0x003FFFFF);
}

// 0864
int suspendSTimer()
{
    // 088C
    int i;
    for (i = 0; i < 4; i++)
    {
        timerSave[i].unk0 = timers[i].hw->unk0;
        timerSave[i].unk4 = timers[i].hw->unk8;
        timerSave[i].unk8 = timers[i].hw->unk12;
        sceKernelDisableIntr(timers[i].intNum);
    }
    return 0;
}

// 08DC
int resumeSTimer()
{
    // 0908
    int i;
    for (i = 0; i < 4; i++)
    {
        timers[i].hw->unk12 = timerSave[i].unk8;
        timers[i].hw->unk8 = timerSave[i].unk4;
        (void)timers[i].hw->unk0;
        timers[i].hw->unk0 = timerSave[i].unk0 & 0x7FFFFFFF;
        sceKernelEnableIntr(timers->intNum);
    }
    return 0;
}

// B53534B4
int sceSTimerSetPrscl(int timerId, int arg1, int arg2)
{
    SceSysTimer *timer = &timers[timerId & 3];
    if (timer->curTimer != timerId)
        return 0x80020097;
    if (arg1 == 0 || arg2 == 0)
        return 0x80020099;
    // 0560
    if (arg2 / arg1 < 12)
        return 0x80020099;
    int oldIntr = sceKernelCpuSuspendIntr();
    if ((timer->hw->unk256 & 0x00800000) != 0)
    {
        // 05C4
        sceKernelCpuResumeIntr(oldIntr);
        return 0x8002009A;
    }
    timer->hw->unk8 = arg1;
    timer->hw->unk12 = arg2;
    sceKernelCpuResumeIntr(oldIntr);
    return 0;
}

int sceSTimerAlloc()
{
    if (sceKernelIsIntrContext() != 0)
        return 0x80020064;
    int oldIntr = sceKernelCpuSuspendIntr();
    // 004C
    int i;
    for (i = 0; i < 4; i++)
    {
        if (timers[i].curTimer == -1)
        {
            // 008C
            timers[i].cb = NULL;
            timers[i].hw->unk0 = 0x80000000;
            timers[i].hw->unk8 = -1;
            timers[i].hw->unk12 = -1;
            (void)timers[i].hw->unk0;
            timers[i].curTimer = -1;
            timers[i].unk12 = 0;
            timers[i].unk16 = 0;
            timers[i].unk24 = 0;
            timers[i].unk28 = 0;
            timers[i].curTimer = ((initVar << 2) | i) & 0x7FFFFFFF;
            initVar += 7;
            // 006C
            sceKernelCpuResumeIntr(oldIntr);
            return timers[i].curTimer;
        }
    }
    // 006C
    sceKernelCpuResumeIntr(oldIntr);
    return 0x80020096;
}

int sceSTimerSetHandler(int timerId, int arg1, SceSysTimerCb arg2, int arg3)
{
    SceSysTimer *timer = &timers[timerId & 3];
    if (timer->curTimer != timerId)
        return 0x80020097;
    int oldIntr = sceKernelCpuSuspendIntr();
    int mask;
    if (arg1 <= 0x3FFFFF)
        mask = arg1;
    else
        mask = 0x3FFFFF;
    if (arg2 == NULL)
    {
        // 0290
        timer->cb = NULL;
        timer->unk24 = 0;
        timer->unk28 = 0;
        _sceSTimerStopCount(timer);
        sceKernelDisableIntr(timer->intNum);
    }
    else
    {
        timer->cb = arg2;
        timer->unk28 = arg3;
        timer->hw->unk0 = timer->hw->unk256 & 0xFFC00000;
        timer->hw->unk0 |= mask;
        timer->hw->unk0 = timer->hw->unk256 | 0x80400000;
        sceKernelEnableIntr(timer->intNum);
    }
    // 0264
    sceKernelCpuResumeIntr(oldIntr);
    return 0;
}

int sceSTimerStartCount(int timerId)
{
    SceSysTimer *timer = &timers[timerId & 3];
    if (timer->curTimer != timerId)
        return 0x80020097;
    int oldIntr = sceKernelCpuSuspendIntr();
    if ((timer->hw->unk256 & 0x00800000) != 0)
    {
        // 0650
        sceKernelCpuResumeIntr(oldIntr);
        return 0x8002009A;
    }
    timer->hw->unk0 = timer->hw->unk256 | 0x00800000;
    sceKernelCpuResumeIntr(oldIntr);
    return 0;
}

int sceSTimerGetCount(int timerId, int *arg1)
{
    SceSysTimer *timer = &timers[timerId & 3];
    if (timer->curTimer != timerId)
        return 0x80020097;
    int oldIntr = sceKernelCpuSuspendIntr();
    *arg1 = _sceSTimerGetCount(timer);
    sceKernelCpuResumeIntr(oldIntr);
    return 0;
}

int sceSTimerResetCount(int timerId)
{
    SceSysTimer *timer = &timers[timerId & 3];
    if (timer->curTimer != timerId)
        return 0x80020097;
    int oldIntr = sceKernelCpuSuspendIntr();
    timer->hw->unk0 |= 0x80000000;
    timer->unk24 = 0;
    sceKernelCpuResumeIntr(oldIntr);
    return 0;
}

// 53231A15
int sceSTimerSetTMCY(int timerId, int arg1)
{
    SceSysTimer *timer = &timers[timerId & 3];
    if (timer->curTimer != timerId)
        return 0x80020097;
    int oldIntr = sceKernelCpuSuspendIntr();
    int val = timer->hw->unk256;
    int val2 = val >> 24;
    val &= 0x003FFFFF;
    timer->unk12 = val;
    timer->unk16 = val2;
    timer->unk24 += val * (val2 - timer->unk16);
    timer->hw->unk0 = (timer->hw->unk256 & 0xFFC00000) | (arg1 - 1 > 0x003FFFFF ? 0x003FFFFF : arg1 - 1);
    sceKernelCpuResumeIntr(oldIntr);
    return 0;
}

int sceSTimerStopCount(int timerId)
{
    SceSysTimer *timer = &timers[timerId & 3];
    if (timer->curTimer != timerId)
        return 0x80020097;
    int oldIntr = sceKernelCpuSuspendIntr();
    _sceSTimerStopCount(timer);
    sceKernelCpuResumeIntr(oldIntr);
    return 0;
}

int sceSTimerFree(int timerId)
{
    SceSysTimer *timer = &timers[timerId & 3];
    if (sceKernelIsIntrContext() != 0)
        return 0x80020064;
    if (timer->curTimer != timerId)
        return 0x80020097;
    int oldIntr = sceKernelCpuSuspendIntr();
    _sceSTimerStopCount(timer);
    sceKernelDisableIntr(timer->intNum);
    timer->hw->unk0 = 0x80000000;
    timer->hw->unk8 = -1;
    timer->hw->unk12 = -1;
    timer->cb = NULL;
    (void)timer->hw->unk0;
    timer->unk28 = 0;
    timer->curTimer = -1;
    timer->unk12 = 0;
    timer->unk16 = 0;
    timer->unk24 = 0;
    sceKernelCpuResumeIntr(oldIntr);
    return 0;
}

