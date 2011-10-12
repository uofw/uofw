#include "../global.h"

#include "systimer.h"

typedef struct {
    int unk0, unk4, unk8, unk12;
    int unused[240];
    int unk256;
} SceHwTimer;

typedef struct {
    SceHwTimer *hw;
    int intNum;
    int unk8, unk12, unk16;
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

int sub_00E0(int arg0, SceSysTimer *arg1, int arg2);
void sub_02B0(SceSysTimer *arg);
int sub_02CC(SceSysTimer *arg);
int sub_0864();
int sub_08DC();

int module_bootstart()
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
        timers[i].unk8 = -1;
        sceKernelRegisterIntrHandler(timers[i].intNum, 2, sub_00E0, timers[i], 0);
    }
    sceKernelRegisterSuspendHandler(10, sub_0864, 0);
    sceKernelRegisterResumeHandler(10, sub_08DC, 0);
    sceKernelCpuResumeIntr(oldIntr);
    return 0;
}

int module_reboot_before()
{
    int oldIntr = sceKernelCpuSuspendIntr();
    // 03F8
    int i;
    for (i = 0; i < 4; i++) {
        sub_02B0(&timers[i]);
        sceKernelReleaseIntrHandler(timers[i].intNum);
    }
    sceKernelCpuResumeIntr(oldIntr);
    return 0;
}

int sub_00E0(int arg0, SceSysTimer *arg1, int arg2)
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
    if (arg1->cb(arg1->unk8, arg1->unk24 + sub_02CC(arg1), arg1->unk28, arg2) != -1)
    {
        // 01A4
        sub_02B0(arg1);
        return -2;
    }
    return -1;
}

void sub_02B0(SceSysTimer *arg)
{
    arg->hw->unk0 = arg->hw->unk256 & 0x7F7FFFFF;
}

int sub_02CC(SceSysTimer *arg)
{
    return (arg->hw->unk256 & 0x003FFFFF) - (arg->hw->unk4 & 0x003FFFFF);
}

int sub_0864()
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

int sub_08DC()
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

int SysTimerForKernel_4467BD60(int arg0, int arg1, int arg2)
{
    SceSysTimer *timer = &timers[arg0 & 3];
    if (timer->unk8 != arg0)
        return 0x80020097;
    if (arg1 == 0 || arg2 == 0)
        return 0x80020099;
    // 0560
    if (arg2 / arg1 < 12)
        return 0x80020099;
    int oldIntr = sceKernelCpuSuspendIntr();
    if (timer->hw->unk256 & 0x00800000 != 0)
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

int SysTimerForKernel_71059CBF()
{
    if (sceKernelIsIntrContext() != 0)
        return 0x80020064;
    int oldIntr = sceKernelCpuSuspendIntr();
    // 004C
    int i;
    for (i = 0; i < 4; i++)
    {
        if (timers[i].unk8 == -1)
        {
            // 008C
            timers[i].cb = NULL;
            timers[i].hw->unk0 = 0x80000000;
            timers[i].hw->unk8 = -1;
            timers[i].hw->unk12 = -1;
            (void)timers[i].hw->unk0;
            timers[i].unk8 = -1;
            timers[i].unk12 = 0;
            timers[i].unk16 = 0;
            timers[i].unk24 = 0;
            timers[i].unk28 = 0;
            timers[i].unk8 = ((initVar << 2) | i) & 0x7FFFFFFF;
            initVar += 7;
            // 006C
            sceKernelCpuResumeIntr(oldIntr);
            return timers[i].unk8;
        }
    }
    // 006C
    sceKernelCpuResumeIntr(oldIntr);
    return 0x80020096;
}

int SysTimerForKernel_847D785B(int arg0, int arg1, SceSysTimerCb arg2, int arg3)
{
    SceSysTimer *timer = &timers[arg0 & 3];
    if (timer->unk8 != arg0)
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
        sub_02B0(timer);
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

int SysTimerForKernel_8FB264FB(int arg)
{
    SceSysTimer *timer = &timers[arg & 3];
    if (timer->unk8 != arg)
        return 0x80020097;
    int oldIntr = sceKernelCpuSuspendIntr();
    if (timer->hw->unk256 & 0x00800000 != 0)
    {
        // 0650
        sceKernelCpuResumeIntr(oldIntr);
        return 0x8002009A;
    }
    timer->hw->unk0 = timer->hw->unk256 | 0x00800000;
    sceKernelCpuResumeIntr(oldIntr);
    return 0;
}

int SysTimerForKernel_94BA6594(int arg0, int *arg1)
{
    SceSysTimer *timer = &timers[arg0 & 3];
    if (timer->unk8 != arg0)
        return 0x80020097;
    int oldIntr = sceKernelCpuSupendIntr();
    *arg1 = sub_02CC(timer);
    sceKernelCpuResumeIntr(oldIntr);
    return 0;
}

int SysTimerForKernel_964F73FD(int arg)
{
    SceSysTimer *timer = &timers[arg & 3];
    if (timer->unk8 != arg)
        return 0x80020097;
    int oldIntr = sceKernelCpuSuspendIntr();
    timer->hw->unk0 |= 0x80000000;
    timer->unk24 = 0;
    sceKernelCpuResumeIntr(oldIntr);
    return 0;
}

int SysTimerForKernel_CE5A60D8(int arg0, int arg1)
{
    SceSysTimer *timer = &timers[arg0 & 3];
    if (timer->unk8 != arg0)
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

int SysTimerForKernel_D01C6E08(int arg)
{
    SceSysTimer *timer = &timers[arg & 3];
    if (timer->unk8 != arg)
        return 0x80020097;
    int oldIntr = sceKernelCpuSuspendIntr();
    sub_02B0(timer);
    sceKernelCpuResumeIntr(oldIntr);
    return 0;
}

int SysTimerForKernel_F03AE143(int arg)
{
    SceSysTimer *timer = &timers[arg & 3];
    if (sceKernelIsIntrContext() != 0)
        return 0x80020064;
    if (timer->unk8 != arg)
        return 0x80020097;
    int oldIntr = sceKernelCpuSuspendIntr();
    sub_02B0(timer);
    sceKernelDisableIntr(timer->intNum);
    timer->hw->unk0 = 0x80000000;
    timer->hw->unk8 = -1;
    timer->hw->unk12 = -1;
    timer->cb = NULL;
    (void)timer->hw->unk0;
    timer->unk28 = 0;
    timer->unk8 = -1;
    timer->unk12 = 0;
    timer->unk16 = 0;
    timer->unk24 = 0;
    sceKernelCpuResumeIntr(oldIntr);
    return 0;
}

