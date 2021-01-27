/* Copyright (C) The uOFW team
   See the file COPYING for copying permission.
*/

#include <common_imp.h>
#include <interruptman.h>
#include <syscon.h>
#include <sysmem_sysclib.h>

typedef struct 
{
    u32 unk16;
    u32 unk20;
    u32 unk24;
    u32 unk28;
    u32 unk32;
    u32 unk36;
    u32 unk40;
    u32 unk44;
    u32 unk48;
    u32 unk52;
} ScePowerIdleData; //size: 40

typedef struct 
{
    u32 unk0;
    u32 unk4;
    u32 unk8;
    u32 unk12;
    ScePowerIdleData data[8];
} ScePowerIdle;

static s32 _scePowerVblankInterrupt(s32 subIntNm, void* arg);
static s32 GetGp(void);

ScePowerIdle g_PowerIdle; //0x0000C400

//Subroutine scePower_EFD3C963 - Address 0x00002F94 - Aliases: scePower_driver_0EFEE60E
// TODO: Verify function
u32 scePowerTick(u32 tickType)
{
    u32 i;

    //0x00002F9C - 0x00002FF8
    for (i = 0; i < 8; i++) {
        if ((((s32)tickType >> i) & 0x1) == 0 && (tickType != 0 || g_PowerIdle.data[i].unk40 & 0x200)) //0x00002FB0 & 0x00002FB8 & 0x00002FC8
            continue;

        if (g_PowerIdle.data[i].unk16 != 0) //0x00002FD4
            continue;

        g_PowerIdle.data[i].unk16 = 1;
        g_PowerIdle.data[i].unk24 = g_PowerIdle.unk0; //0x00002FF0
        g_PowerIdle.data[i].unk28 = g_PowerIdle.unk4; //0x00002FF4

    }
    return SCE_ERROR_OK;
}

//Subroutine scePower_EDC13FE5 - Address 0x00003008 - Aliases: scePower_driver_DF336CDE
// TODO: Verify function
u32 scePowerGetIdleTimer(u32 slot, SceKernelSysClock* sysClock, u32* arg2)
{
    s32 oldK1;
    s64 sysTime;

    oldK1 = pspShiftK1(); //0x00003018

    if (slot >= 8) { //0x00003040
        pspSetK1(oldK1);
        return SCE_ERROR_INVALID_INDEX;
    }
    if (!pspK1PtrOk(sysClock) || !pspK1PtrOk(arg2)) { //0x00003054 & 0x0000305C
        pspSetK1(oldK1);
        return SCE_ERROR_PRIV_REQUIRED;
    }

    sysTime = sceKernelGetSystemTimeWide(); //0x00003088
    if (sysClock != NULL) { //0x000030C0
        sysClock->low = (u32)sysTime - g_PowerIdle.data[slot].unk24; //0x000030BC
        sysClock->hi = ((u32)(sysTime >> 32) - g_PowerIdle.data[slot].unk28) - ((u32)sysTime < g_PowerIdle.data[slot].unk24); //0x000030CC & 0x000030C4 & 0x000030B8
    }
    if (arg2 != NULL) { //0x000030D0
        *arg2 = g_PowerIdle.data[slot].unk32 - ((u32)sysTime - g_PowerIdle.data[slot].unk24); //0x000030E8
        *(u32*)(arg2 + 4) = (g_PowerIdle.data[slot].unk36 - (((u32)(sysTime >> 32) - g_PowerIdle.data[slot].unk28) -
            ((u32)sysTime < g_PowerIdle.data[slot].unk24))) -
            (g_PowerIdle.data[slot].unk32 < (u32)sysTime - g_PowerIdle.data[slot].unk24);
    }
    return (u32)sysTime - g_PowerIdle.data[slot].unk24;
}

//Subroutine scePower_driver_1BA2FCAE - Address 0x00003100
// TODO: Verify function
u32 scePowerSetIdleCallback(u32 slot, u32 arg1, u32 arg2, u32 arg3, u32 arg4, u32 arg5)
{
    s32 intrState;
    s64 sysTime;

    if (slot >= 8) //0x00003148
        return SCE_ERROR_INVALID_INDEX;

    intrState = sceKernelCpuSuspendIntr(); //0x00003150

    if (arg3 != 0 && g_PowerIdle.data[slot].unk44 != 0) { //0x00003170 & 0x0000317C
        sceKernelCpuResumeIntr(intrState);
        return SCE_ERROR_ALREADY;
    }
    sysTime = sceKernelGetSystemTimeWide(); //0x00003198
    g_PowerIdle.data[slot].unk40 = arg1; //0x0000319C
    g_PowerIdle.data[slot].unk44 = arg4; //0x000031B8
    g_PowerIdle.data[slot].unk24 = (u32)sysTime; //0x000031AC
    g_PowerIdle.data[slot].unk28 = (u32)(sysTime >> 32); //0x000031B0
    g_PowerIdle.data[slot].unk16 = 0; //0x000031D4
    g_PowerIdle.data[slot].unk20 = -1; //0x000031BC
    g_PowerIdle.data[slot].unk32 = arg2; //0x000031C0
    g_PowerIdle.data[slot].unk36 = arg3; //0x000031C4
    g_PowerIdle.data[slot].unk48 = pspGetGp(); //0x000031C8
    g_PowerIdle.data[slot].unk52 = arg5; //0x000031CC

    sceKernelCpuResumeIntr(intrState); //0x000031D0
    return SCE_ERROR_OK;
}

//0x00003220
// TODO: Verify function
static s32 _scePowerVblankInterrupt(s32 subIntNm, void* arg)
{
    s64 sysTime;
    u8 isAcSupplied;
    s32 gp;
    u32 i;
    u32 data;
    u32 diff;
    void (*func)(u32, u32, u32, u32);

    (void)subIntNm;
    (void)arg;

    sysTime = sceKernelGetSystemTimeWide(); //0x00003240
    g_PowerIdle.unk0 = (u32)sysTime; //0x00003258
    g_PowerIdle.unk4 = (u32)(sysTime >> 32);

    isAcSupplied = sceSysconIsAcSupplied(); //0x0000325C
    gp = pspGetGp(); //0x00003264

    //0x0000326C
    for (i = 0; i < 8; i++) {
        if (g_PowerIdle.data[i].unk44 == 0) //0x00003278
            continue;

        if ((g_PowerIdle.unk8 & (1 << i)) == 0) //0x00003288 & 0x00003290
            continue;

        if (isAcSupplied && ((g_PowerIdle.data[i].unk40 & 0x100) == 0)) { //0x000032DC & 0x000032EC
            g_PowerIdle.data[i].unk16 = 1; //0x000032F4
            g_PowerIdle.data[i].unk24 = (u32)sysTime;
            g_PowerIdle.data[i].unk16 = (u32)(sysTime >> 32);
        }
        if (g_PowerIdle.data[i].unk16 != 0) { //0x00003304
            g_PowerIdle.data[i].unk16 = 0; //0x00003314
            if (g_PowerIdle.data[i].unk20 == 0) //0x00003310
                continue;

            g_PowerIdle.data[i].unk20 = 0; //0x00003318
            pspSetGp(g_PowerIdle.data[i].unk48); //0x00003324
            func = g_PowerIdle.data[i].unk44;
            func(i, 0, g_PowerIdle.data[i].unk52, &g_PowerIdle.data[i].unk16); //0x00003334
            continue;
        }
        if (g_PowerIdle.data[i].unk20 == 1) //0x0000334C
            continue;

        data = ((u32)(sysTime >> 32) - g_PowerIdle.data[i].unk28) - (g_PowerIdle.data[i].unk24 < (u32)sysTime); //0x00003360 & 0x00003364 & 0x00003368
        if (data < g_PowerIdle.data[i].unk36) //0x0000336C & 0x00003370
            continue;

        diff = (u32)sysTime - g_PowerIdle.data[i].unk24; //0x00003374
        if ((data == g_PowerIdle.data[i].unk36) && (diff < g_PowerIdle.data[i].unk32)) //0x00003378 & 0x000033B4
            continue;

        g_PowerIdle.data[i].unk20 = 1; //0x00003380

        pspSetGp(g_PowerIdle.data[i].unk48); //0x00003390
        func = g_PowerIdle.data[i].unk44; //0x00003398
        func(i, diff, g_PowerIdle.data[i].unk52, &g_PowerIdle.data[i].unk16); //0x000033A0
    }
    return -1;
}

//sub_000033C4
// TODO: Verify function
u32 _scePowerIdleInit(void)
{
    u64 sysTime;

    memset(&g_PowerIdle, 0, sizeof g_PowerIdle); //0x000033E0
    sysTime = sceKernelGetSystemTimeWide(); //0x000033E8

    g_PowerIdle.unk0 = (u32)sysTime; //0x00003408
    g_PowerIdle.unk4 = (u32)(sysTime >> 32);
    g_PowerIdle.unk8 = 255;

    sceKernelRegisterSubIntrHandler(SCE_VBLANK_INT, 0x1A, _scePowerVblankInterrupt, NULL); //0x0000340C
    sceKernelEnableSubIntr(SCE_VBLANK_INT, 0x1A); //0x0000341C
    return SCE_ERROR_OK;
}

//sub_00003438
// TODO: Verify function
u32 _scePowerIdleEnd(void)
{
    sceKernelReleaseSubIntrHandler(SCE_VBLANK_INT, 0x1A); //0x00003444
    return SCE_ERROR_OK;
}

//Subroutine scePower_7F30B3B1 - Address 0x0000345C - Aliases: scePower_driver_1E3B1FAE
// TODO: Verify function
u32 scePowerIdleTimerEnable(u32 slot)
{
    s32 intrState;
    u32 data;

    if (slot >= 8) //0x00003474
        return SCE_ERROR_INVALID_INDEX;

    intrState = sceKernelCpuSuspendIntr(); //0x0000347C

    data = g_PowerIdle.unk8;
    g_PowerIdle.unk8 |= (1 << slot); //0x0000348C - 0x0000349C

    sceKernelCpuResumeIntr(intrState);
    return (data >> slot) & 0x1;
}

//Subroutine scePower_972CE941 - Address 0x000034C8 - Aliases: scePower_driver_961A06A5
// TODO: Verify function
u32 scePowerIdleTimerDisable(u32 slot)
{
    s32 intrState;
    u32 data;

    if (slot >= 8) //0x000034D0
        return SCE_ERROR_INVALID_INDEX;

    intrState = sceKernelCpuSuspendIntr(); //0x000034E8

    data = g_PowerIdle.unk8; //0x000034F8
    g_PowerIdle.unk8 &= (~1 << slot); //0x00003508

    sceKernelCpuResumeIntr(intrState);
    return (data >> slot) & 0x1; //0x000031A4
}

//0x00003534
static s32 GetGp(void)
{
    return pspGetGp();
}