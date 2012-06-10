/* Copyright (C) 2011, 2012 The uOFW team
   See the file COPYING for copying permission.
*/

#include <common.h>

#include <interruptman.h>
#include <sysmem_suspend_kernel.h>

#include <systimer.h>

SCE_MODULE_INFO("sceSystimer", SCE_MODULE_KERNEL | SCE_MODULE_NO_STOP |
                               SCE_MODULE_SINGLE_LOAD | SCE_MODULE_SINGLE_START, 1, 2);
SCE_MODULE_BOOTSTART("STimerInit");
SCE_SDK_VERSION(SDK_VERSION);

typedef struct {
    s32 unk0, unk4, unk8, unk12;
    s32 unused[240];
    s32 unk256;
} SceHwTimer;

typedef struct {
    SceHwTimer *hw;
    s32 s32Num;
    s32 curTimer;
    s32 unk12, unk16;
    SceSysTimerCb cb;
    s32 unk24, unk28;
} SceSysTimer;

SceSysTimer timers[] = {
    { (void*)0xBC500000, 0x0F, -1, 0, 0, NULL, 0, 0 },
    { (void*)0xBC500010, 0x10, -1, 0, 0, NULL, 0, 0 },
    { (void*)0xBC500020, 0x11, -1, 0, 0, NULL, 0, 0 },
    { (void*)0xBC500030, 0x12, -1, 0, 0, NULL, 0, 0 }
};

typedef struct {
    s32 unk0, unk4, unk8;
} SceSysTimerSave;

SceSysTimerSave timerSave[4];

s32 initVar = 0x00352341;

s32 systimerhandler(s32 arg0, SceSysTimer *arg1, s32 arg2);
void _sceSTimerStopCount(SceSysTimer *arg);
s32 _sceSTimerGetCount(SceSysTimer *arg);
s32 suspendSTimer();
s32 resumeSTimer();

s32 STimerInit()
{
    s32 oldIntr = sceKernelCpuSuspendIntr();
    s32 i;
    for (i = 0; i < 4; i++) {
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
        sceKernelRegisterIntrHandler(timers[i].s32Num, 2, systimerhandler, &timers[i], 0);
    }
    sceKernelRegisterSuspendHandler(10, suspendSTimer, 0);
    sceKernelRegisterResumeHandler(10, resumeSTimer, 0);
    sceKernelCpuResumeIntr(oldIntr);
    return 0;
}

s32 module_reboot_before()
{
    s32 oldIntr = sceKernelCpuSuspendIntr();
    s32 i;
    for (i = 0; i < 4; i++) {
        _sceSTimerStopCount(&timers[i]);
        sceKernelReleaseIntrHandler(timers[i].s32Num);
    }
    sceKernelCpuResumeIntr(oldIntr);
    return 0;
}

s32 systimerhandler(s32 arg0 __attribute__((unused)), SceSysTimer *arg1, s32 arg2)
{
    if (arg1->cb == NULL)
        return -1;
    s32 var = arg1->hw->unk0;
    s32 v1 = var >> 24;
    s32 v2 = var & 0x003FFFFF;
    if (arg1->unk12 != 0) {
        v1--;
        arg1->unk24 += arg1->unk12;
    }
    arg1->unk24 += v2 * (v1 - arg1->unk16);
    arg1->unk12 = 0;
    arg1->unk16 = 0;
    if (arg1->cb(arg1->curTimer, arg1->unk24 + _sceSTimerGetCount(arg1), arg1->unk28, arg2) != -1) {
        _sceSTimerStopCount(arg1);
        return -2;
    }
    return -1;
}

void _sceSTimerStopCount(SceSysTimer *arg)
{
    arg->hw->unk0 = arg->hw->unk256 & 0x7F7FFFFF;
}

s32 _sceSTimerGetCount(SceSysTimer *arg)
{
    return (arg->hw->unk256 & 0x003FFFFF) - (arg->hw->unk4 & 0x003FFFFF);
}

s32 suspendSTimer()
{
    s32 i;
    for (i = 0; i < 4; i++) {
        timerSave[i].unk0 = timers[i].hw->unk0;
        timerSave[i].unk4 = timers[i].hw->unk8;
        timerSave[i].unk8 = timers[i].hw->unk12;
        sceKernelDisableIntr(timers[i].s32Num);
    }
    return 0;
}

s32 resumeSTimer()
{
    s32 i;
    for (i = 0; i < 4; i++) {
        timers[i].hw->unk12 = timerSave[i].unk8;
        timers[i].hw->unk8 = timerSave[i].unk4;
        (void)timers[i].hw->unk0;
        timers[i].hw->unk0 = timerSave[i].unk0 & 0x7FFFFFFF;
        sceKernelEnableIntr(timers->s32Num);
    }
    return 0;
}

s32 sceSTimerSetPrscl(s32 timerId, s32 arg1, s32 arg2)
{
    SceSysTimer *timer = &timers[timerId & 3];
    if (timer->curTimer != timerId)
        return 0x80020097;
    if (arg1 == 0 || arg2 == 0)
        return 0x80020099;
    if (arg2 / arg1 < 12)
        return 0x80020099;
    s32 oldIntr = sceKernelCpuSuspendIntr();
    if ((timer->hw->unk256 & 0x00800000) != 0) {
        sceKernelCpuResumeIntr(oldIntr);
        return 0x8002009A;
    }
    timer->hw->unk8 = arg1;
    timer->hw->unk12 = arg2;
    sceKernelCpuResumeIntr(oldIntr);
    return 0;
}

s32 sceSTimerAlloc()
{
    if (sceKernelIsIntrContext() != 0)
        return 0x80020064;
    s32 oldIntr = sceKernelCpuSuspendIntr();
    s32 i;
    for (i = 0; i < 4; i++) {
        if (timers[i].curTimer == -1) {
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
            sceKernelCpuResumeIntr(oldIntr);
            return timers[i].curTimer;
        }
    }
    sceKernelCpuResumeIntr(oldIntr);
    return 0x80020096;
}

s32 sceSTimerSetHandler(s32 timerId, s32 arg1, SceSysTimerCb arg2, s32 arg3)
{
    SceSysTimer *timer = &timers[timerId & 3];
    if (timer->curTimer != timerId)
        return 0x80020097;
    s32 oldIntr = sceKernelCpuSuspendIntr();
    s32 mask;
    if (arg1 <= 0x3FFFFF)
        mask = arg1;
    else
        mask = 0x3FFFFF;
    if (arg2 == NULL) {
        timer->cb = NULL;
        timer->unk24 = 0;
        timer->unk28 = 0;
        _sceSTimerStopCount(timer);
        sceKernelDisableIntr(timer->s32Num);
    } else {
        timer->cb = arg2;
        timer->unk28 = arg3;
        timer->hw->unk0 = timer->hw->unk256 & 0xFFC00000;
        timer->hw->unk0 |= mask;
        timer->hw->unk0 = timer->hw->unk256 | 0x80400000;
        sceKernelEnableIntr(timer->s32Num);
    }
    sceKernelCpuResumeIntr(oldIntr);
    return 0;
}

s32 sceSTimerStartCount(s32 timerId)
{
    SceSysTimer *timer = &timers[timerId & 3];
    if (timer->curTimer != timerId)
        return 0x80020097;
    s32 oldIntr = sceKernelCpuSuspendIntr();
    if ((timer->hw->unk256 & 0x00800000) != 0) {
        sceKernelCpuResumeIntr(oldIntr);
        return 0x8002009A;
    }
    timer->hw->unk0 = timer->hw->unk256 | 0x00800000;
    sceKernelCpuResumeIntr(oldIntr);
    return 0;
}

s32 sceSTimerGetCount(s32 timerId, s32 *arg1)
{
    SceSysTimer *timer = &timers[timerId & 3];
    if (timer->curTimer != timerId)
        return 0x80020097;
    s32 oldIntr = sceKernelCpuSuspendIntr();
    *arg1 = _sceSTimerGetCount(timer);
    sceKernelCpuResumeIntr(oldIntr);
    return 0;
}

s32 sceSTimerResetCount(s32 timerId)
{
    SceSysTimer *timer = &timers[timerId & 3];
    if (timer->curTimer != timerId)
        return 0x80020097;
    s32 oldIntr = sceKernelCpuSuspendIntr();
    timer->hw->unk0 |= 0x80000000;
    timer->unk24 = 0;
    sceKernelCpuResumeIntr(oldIntr);
    return 0;
}

s32 sceSTimerSetTMCY(s32 timerId, s32 arg1)
{
    SceSysTimer *timer = &timers[timerId & 3];
    if (timer->curTimer != timerId)
        return 0x80020097;
    s32 oldIntr = sceKernelCpuSuspendIntr();
    s32 val = timer->hw->unk256;
    s32 val2 = val >> 24;
    val &= 0x003FFFFF;
    timer->unk12 = val;
    timer->unk16 = val2;
    timer->unk24 += val * (val2 - timer->unk16);
    timer->hw->unk0 = (timer->hw->unk256 & 0xFFC00000) | (arg1 - 1 > 0x003FFFFF ? 0x003FFFFF : arg1 - 1);
    sceKernelCpuResumeIntr(oldIntr);
    return 0;
}

s32 sceSTimerStopCount(s32 timerId)
{
    SceSysTimer *timer = &timers[timerId & 3];
    if (timer->curTimer != timerId)
        return 0x80020097;
    s32 oldIntr = sceKernelCpuSuspendIntr();
    _sceSTimerStopCount(timer);
    sceKernelCpuResumeIntr(oldIntr);
    return 0;
}

s32 sceSTimerFree(s32 timerId)
{
    SceSysTimer *timer = &timers[timerId & 3];
    if (sceKernelIsIntrContext() != 0)
        return 0x80020064;
    if (timer->curTimer != timerId)
        return 0x80020097;
    s32 oldIntr = sceKernelCpuSuspendIntr();
    _sceSTimerStopCount(timer);
    sceKernelDisableIntr(timer->s32Num);
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

