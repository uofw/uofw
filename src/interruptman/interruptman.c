/* Copyright (C) 2011, 2012 The uOFW team
   See the file COPYING for copying permission.
*/

#include "common.h"

#include "exceptionman.h"

#include "interruptman.h"
#include "end.h"
#include "start.h"

char intrMgrStr[] = "InterruptManager"; // 0x36F4
char **intrMgrStrPtr = (char**)&intrMgrStr; // 0x33E8

// 0x3458
PSP_MODULE_INFO("sceInterruptManager", 0x1007, 1, 9);
PSP_MODULE_BOOTSTART(IntrManInit);
PSP_MODULE_REBOOT_BEFORE(IntrManTerminate);
PSP_SDK_VERSION(0x06060010);

char heapName[] = "SceInterruptManager"; // 0x3778

CbMap emptyMap = {NULL, 0, 0, 0, {}}; // 0x38D0

CbMap cbMap = // 0x37C0
{
    &emptyMap, 0, 0x00000100, 0x00000110,
    {
    sub_0CC0, sub_0CC0, sub_0864, sub_0CC0,
    sub_0CC0, sub_0CC0, sub_0CC0, sub_0CC0,
    sub_0ECC, sub_0EF8, sub_0F20, sub_0F58,
    sub_0FE4, sub_0FF0, sub_0F64, sub_0FA4,
    sub_0FF8, sub_1000, sub_1008, sub_0EC0,
    sub_1010, sub_0CC0, sub_0CC0, sub_0CC0,
    sub_0CC0, sub_0CC0, sub_0CC0, sub_0CC0,
    sub_0CC0, sub_0CC0, sub_0CC0, sub_0CC0,
    sub_0CC0, sub_0CC0, sub_0CC0, sub_0CC0,
    sub_0CC0, sub_0CC0, sub_0CC0, sub_0CC0,
    sub_0CC0, sub_0CC0, sub_0CC0, sub_0CC0,
    sub_0CC0, sub_0CC0, sub_0CC0, sub_0CC0,
    sub_0CC0, sub_0CC0, sub_0CC0, sub_0CC0,
    sub_0CC0, sub_0CC0, sub_0CC0, sub_0CC0,
    sub_0CC0, sub_0CC0, sub_0CC0, sub_0CC0,
    sub_0CC0, sub_0CC0, sub_0CC0, sub_0CC0
    }
};

/* TODO: find what lays here [0x3818 - 0x3ABC] */

int wut; // 0x3AC0

/* TODO: find what lays here [0x3AC4 - 0x56FC] */

int g_sp; // 0x5700

/* TODO: find what lays here [0x5704 - 0x5840] */

typedef struct
{
    int grpsOpt[24]; // 0
    char unused2; // 96
    char opt2; // 97
    char opt; // 98
    char unused3; // 99
    MonitorCb monitorCbBefore; // 100
    MonitorCb monitorCbAfter; // 104
    int unused; // 108
    int intrStack; // 112
    int intrStackArg; // 116
    u32 clockCounterLo; // 120
    u32 clockCounterHi; // 124
    Interrupt intr[68]; // 128
    int subIntrMemoryPoolId; // 4088
    int intcState[2]; // 4092
    int count;
    int compare;
} InterruptInfo;

InterruptInfo intInfo; // 0x5840

int IntrManInit()
{
    int oldIc = sceKernelCpuSuspendIntr();
    pspCop0CtrlSet(COP0_CTRL_IS_INTERRUPT, 0);
    pspCop0CtrlSet(COP0_CTRL_SP_KERNEL, (int)globUnk);
    pspCop0CtrlSet(COP0_CTRL_SP_USER, 0);
    mymemset(&intInfo, 0, sizeof(InterruptInfo));
    intInfo.opt = 1;
    sub_1030();
    ReleaseContextHooks();
    // 1150
    int i;
    for (i = 67; i >= 0; i--) {
        intInfo.intr[i].u16 = -1;
        intInfo.intr[i].u20 = -1;
    }
    sceKernelRegisterExceptionHandler(EXCEP_INT, (void*)sub_0038);
    sceKernelRegisterPriorityExceptionHandler(EXCEP_INT, 3, (void*)sub_0924);
    sceKernelRegisterExceptionHandler(EXCEP_SYS, (void*)sub_0CF8);
    sceKernelRegisterIntrHandler(67, 0, sub_0000, 0, 0);
    sceKernelRegisterSuspendHandler(29, SuspendIntc, 0);
    sceKernelRegisterResumeHandler(29, ResumeIntc, 0);
    sceKernelSetIntrLogging(67, 0);
    pspCop0StateSet(COP0_STATE_STATUS, (pspCop0StateGet(COP0_STATE_STATUS) & 0xFFFF00FF) | 0x0400);
    sceKernelEnableIntr(67);
    pspCop0StateSet(COP0_STATE_COUNT, 0);
    pspCop0StateSet(COP0_STATE_COMPARE, 0x80000000);
    sceKernelCpuResumeIntr(oldIc);
    int ret = sceKernelRegisterLibrary(intrMgrStrPtr); // This address contains a pointer to "InterruptManager"
    if (ret < 0)
        return ret;
    return 0;
}

// 58DD8978
int sceKernelRegisterIntrHandler(int intrNum, int arg1, void *func, int arg3, SceIntrHandler *handler)
{
    if (sceKernelIsIntrContext() != 0)
        return 0x80020064;
    if (intrNum >= 68)
        return 0x80020065;
    if (handler != NULL && handler->size != 12)
        return 0x8002006B;
    // 12B8
    int oldIc = sceKernelCpuSuspendIntr();
    if (handler != NULL && handler->size > 0 && intInfo.subIntrMemoryPoolId == 0)
    {
        // 1468
        int ret = sceKernelCreateHeap(1, 1, 0x800, heapName); // "SceInterruptManager"
        if (ret > 0)
            intInfo.subIntrMemoryPoolId = ret;
        if (ret < 0) {
            sceKernelCpuResumeIntr(oldIc);
            return ret;
        }
    }
    // 12E4
    Interrupt *intr = &intInfo.intr[intrNum];
    if (intr->handler != 0 && (intr->handler & 3) != 1) {
        sceKernelCpuResumeIntr(oldIc);
        return 0x80020067;
    }
    // 1354
    if ((int)func >= 0) {
        sceKernelCpuResumeIntr(oldIc);
        return 0x80020065;
    }
    intr->subIntrs = NULL;
    // 1368
    intr->cb = NULL;
    int size;
    if (handler == NULL || (size = handler->attr) <= 0)
    {
        // 144C
        if (intr->subIntrs != NULL) {
            sceKernelFreeHeapMemory(intInfo.subIntrMemoryPoolId, intr->subIntrs); // Free memory space
            intr->subIntrs = NULL;
        }
    }
    else
    {
        SubInterrupt *subIntrs = sceKernelAllocHeapMemory(intInfo.subIntrMemoryPoolId, size); // Allocate space to store the subIntrs
        if (subIntrs != NULL)
            mymemset(subIntrs, 0, sizeof(*subIntrs));
        // 13A4
        intr->subIntrs = subIntrs;
        if (subIntrs == NULL)
        {
            // 1440
            sceKernelCpuResumeIntr(oldIc);
            return 0x80020190;
        }
        intr->cb = handler->cb;
    }
    // 13B4
    sceKernelSuspendIntr(intrNum, 0);
    int unk2;
    if (intr->handler != 1 && intrNum < 64)
        unk2 = 3;
    else
        unk2 = 0;
    // 13E0
    if (arg1 != 0)
        arg1 = 2;
    intr->v48 = (intr->v48 & 0xFFFFFCFF) | ((unk2 << 8) & 0x300);
    intr->handler = arg1 | (int)func;
    intr->gp = sceKernelGetModuleGPByAddressForKernel(func);
    intr->arg = arg3;
    *(char*)&intr->v48 = 0;
    if (handler != NULL)
        *(char*)&intr->v48 = *(char*)&handler->attr;
    // 1420
    intr->v48 = ((intr->v48 & 0x7FFF7FFF) | ((intInfo.opt << 15) & 0x8000)) & 0x7FFFFFFF;
    *(char*)(&intr->v48 + 2) = intrNum;
    sceKernelCpuResumeIntr(oldIc);
    return 0;
}

// A1B88367
int sceKernelSetUserModeIntrHanlerAcceptable(int intrNum, int subIntrNum, int setBit)
{
    if (sceKernelIsIntrContext() != 0)
        return 0x80020064;
    if (intrNum >= 68)
        return 0x80020065;
    int oldIntr = sceKernelCpuSuspendIntr();
    Interrupt *intr = &intInfo.intr[intrNum];
    if (intr->handler == 0 || (intr->handler & 3) == 1)
    {
        // 15F4
        sceKernelCpuResumeIntr(oldIntr);
        return 0x80020068;
    }
    if (subIntrNum < 0 || subIntrNum >= (intr->v48 & 0xFF))
    {
        // 1538
        sceKernelCpuResumeIntr(oldIntr);
        return 0x80020065;
    }
    // 156C
    SubInterrupt *subIntr = &intr->subIntrs[subIntrNum];
    subIntr->v48 = (subIntr->v48 & 0xFFFFFBFF) | (((setBit != 0) << 10) & 0x00000400);
    intr->v48 &= 0xFFFFF7FF;
    char num = intr->v48 & 0xFF;
    char v = (subIntr->v48 >> 10) & 1;
    // 15B4
    int i;
    for (i = 0; i < num; i++)
    {
        if (v != 0)
        {
            // 15E4
            intr->v48 |= 0x800;
            break;
        }
    }
    // 15C4
    if (((intr->v48 >> 11) & 1) != 0)
        sceKernelSetIntrLevel(intrNum, 2);
    sceKernelCpuResumeIntr(oldIntr);
    return 0;
}

// F987B1F0
int sceKernelReleaseIntrHandler(int intrNum)
{
    if (sceKernelIsIntrContext() != 0)
        return 0x80020064;
    if (intrNum >= 68)
        return 0x80020065;
    int oldIntr = sceKernelCpuSuspendIntr();
    Interrupt *intr = &intInfo.intr[intrNum];
    if (intr->handler == 0) {
        sceKernelCpuResumeIntr(oldIntr);
        return 0x80020068;
    }
    // 1698
    if (((intr->v48 >> 8) & 3) != 0)
    {
        // 16CC
        intr->handler = 0;
        sceKernelSuspendIntr(intrNum, 0);
    }
    else
        intr->handler = 1;
    // 16B0
    intr->cb = NULL;
    if (intr->subIntrs != NULL)
        sceKernelFreeHeapMemory(intInfo.subIntrMemoryPoolId, intr->subIntrs);
    // 16C4
    intr->subIntrs = NULL;
    sceKernelCpuResumeIntr(oldIntr);
    return 0;
}

// B941600E
int sceKernelSetIntrLevel(int intrNum, int num)
{
    if (intrNum >= 64)
        return 0x80020065;
    if (num < 1 || num >= 4)
        return 0x80020069;
    Interrupt *intr = &intInfo.intr[intrNum];
    int oldIntr = sceKernelCpuSuspendIntr();
    if (intr->handler == 0 || (intr->handler & 3) == 1)
    {
        // 1814
        sceKernelCpuResumeIntr(oldIntr);
        return 0x80020068;
    }
    if (((intr->v48 >> 11) & 1) != 0 && ((num ^ 2) != 0))
    {
        // 1800
        sceKernelCpuResumeIntr(oldIntr);
        return 0x80020069;
    }
    if (((intInfo.grpsOpt[(intrNum >> 5) + 24] >> (intrNum & 0x1F)) & 1) == 0)
    {
        // 17F4
        intr->v48 = (intr->v48 & 0xFFFFFCFF) | ((num << 8) & 0x00000300);
    }
    else
    {
        if (((intr->v48 >> 8) & 3) < num)
        {
            // 17E4
            InterruptDisableInTable(intrNum);
        }
        // 17A8
        intr->v48 = (intr->v48 & 0xFFFFFCFF) | ((num << 8) & 0x00000300);
        sub_29B0(intrNum);
    }
    // 17B8
    sceKernelCpuResumeIntr(oldIntr);
    return 0;
}

// AB1FC793
int sceKernelSetIntrLogging(int intrNum, int arg1)
{
    if (intrNum >= 68)
        return 0x80020065;
    int oldIc = sceKernelCpuSuspendIntr();
    Interrupt *intr = &intInfo.intr[intrNum];
    if (arg1 == 0) {
        // 189C
        intr->v48 |= 0x00008000;
    }
    else
        intr->v48 &= 0xFFFF7FFF;
    // 1878
    sceKernelCpuResumeIntr(oldIc);
    return 0;
}

// 4D6E7305
int sceKernelEnableIntr(int intNum)
{
    int v;
    if (intNum >= 68)
        return 0x80020065;
    int oldIc = sceKernelCpuSuspendIntr();
    if (intInfo.intr[intNum].handler == 0) {
        sceKernelCpuResumeIntr(oldIc);
        return 0x80020068;
    }
    // 1928
    if (intNum < 64)
    {
        // 1978
        sub_29B0(intNum);
        sceKernelCpuResumeIntr(oldIc);
        return 0;
    }
    if (intNum == 64)
        v = 0x100;
    else if (intNum == 65)
        v = 0x200;
    else if (intNum == 66)
        v = 0x1000;
    else if ((intNum & 0x43) == 0)
        v = 0x8000;
    else
        v = 0;
    // 1960
    pspCop0StateSet(COP0_STATE_STATUS, pspCop0StateGet(COP0_STATE_STATUS) | (v & 0xFF00));
    sceKernelCpuResumeIntr(oldIc);
    return 0;
}

// 2412F096
int sceKernelSuspendIntr(int arg0, int arg1)
{
    int ret = 0;
    int mask;
    if (arg0 >= 68)
        return 0x80020065;
    int oldIc = sceKernelCpuSuspendIntr();
    if (arg0 == 64)
        mask = 0x100;
    else if (arg0 == 65)
        mask = 0x200;
    else if (arg0 == 66)
        mask = 0x1000;
    else if (arg0 == 67)
        mask = 0x8000;
    else
        mask = 0;
    // 19F8
    if (intInfo.intr[arg0].handler != 0)
    {
        // 1A78
        if (arg1 != 0)
        {
            if (arg0 >= 64) {
                // 1AA8
                *(int*)arg1 = (pspCop0StateGet(COP0_STATE_STATUS) & 0xFF00) | mask;
            }
            else
                *(int*)arg1 = (intInfo.grpsOpt[(arg0 >> 5) + 24] >> (arg0 & 0x1F)) & 1;
        }
    }
    else
    {
        ret = 0x80020068;
        if (arg1 != 0)
            *(int*)arg1 = 0;
    }
    // 1A24
    if (arg0 < 64)
    {
        // 1A68
        AllLevelInterruptDisable(arg0);
    }
    else
        pspCop0StateSet(COP0_STATE_STATUS, pspCop0StateGet(COP0_STATE_STATUS) & ~(mask & 0xFF00));
    // 1A3C
    sceKernelCpuResumeIntr(oldIc);
    return ret;
}

// DB14CBE0
int sceKernelResumeIntr(int intrNum, int arg1)
{
    if (intrNum >= 68)
        return 0x80020065;
    int oldIntr = sceKernelCpuSuspendIntr();
    int mask;
    if (intrNum == 64)
        mask = 0x100;
    else if (intrNum == 65)
        mask = 0x200;
    else if (intrNum == 66)
        mask = 0x1000;
    else if (intrNum == 67)
        mask = 0x8000;
    else
        mask = 0;
    // 1B2C
    if (intInfo.intr[intrNum].handler != 0)
    {
        // 1BA8
        if (intrNum >= 64)
        {
            // 1BC8
            if (arg1 != 0)
            {
                pspCop0StateSet(COP0_STATE_STATUS, pspCop0StateGet(COP0_STATE_STATUS) | (mask & 0xFF00));
                sceKernelCpuResumeIntr(oldIntr);
                return 0;
            }
        }
        else
        {
            if (arg1 == 0)
            {
                // 1B98
                AllLevelInterruptDisable(intrNum);
            }
            else
                sub_29B0(intrNum);
            sceKernelCpuResumeIntr(oldIntr);
            return 0;
        }
    }
    else if (intrNum < 64)
    {
        // 1B98
        AllLevelInterruptDisable(intrNum);
        sceKernelCpuResumeIntr(oldIntr);
        return 0x80020068;
    }
    else
    {
        pspCop0StateSet(COP0_STATE_STATUS, pspCop0StateGet(COP0_STATE_STATUS) | ~(mask & 0xFF00));
        sceKernelCpuResumeIntr(oldIntr);
        return 0x80020068;
    }
    // 1B5C
    pspCop0StateSet(COP0_STATE_STATUS, pspCop0StateGet(COP0_STATE_STATUS) | ~(mask & 0xFF00));
    sceKernelCpuResumeIntr(oldIntr);
    return 0;
}

// DB9A5496
void ReleaseContextHooks()
{
    InterruptManagerForKernel_E790EAED(sub_091C, sub_091C);
}

void InterruptManagerForKernel_E790EAED(int (*arg0)(), int (*arg1)())
{
    int oldIc = sceKernelCpuSuspendIntr();
    *(int*)(mod_0468) = 0x0C000000 + (((int)arg1 >> 2) & 0x3FFFFFF);
    pspCache(0x1A, mod_0468);
    pspCache(0x08, mod_0468);
    *(int*)(mod_0400) = 0x0C000000 + (((int)arg0 >> 2) & 0x3FFFFFF);
    pspCache(0x1A, mod_0400);
    pspCache(0x08, mod_0400);
    sceKernelCpuResumeIntr(oldIc);
}

// 0C5F7AE3
int sceKernelCallSubIntrHandler(int intrNum, int subIntrNum, int arg2, int arg3)
{
    SubInterrupt *subIntr = &intInfo.intr[intrNum].subIntrs[subIntrNum];
    if (subIntr->handler < 2)
        return 0x80020068;
    if (intInfo.monitorCbBefore != NULL)
    {
        if (((subIntr->v48 >> 15) & 1) == 0)
            subIntr->u12++;
        // 1D18
        intInfo.monitorCbBefore(intrNum, subIntrNum, arg2, subIntr->arg, arg3, subIntr->handler, intInfo.opt2);
    }
    // 1D28
    int oldGp = pspGetGp();
    pspSetGp(subIntr->gp);
    int ret;
    if (subIntr->handler >= 0) {
        // 1DB8
        ret = sceKernelCallUserIntrHandler(arg2, subIntr->arg, arg3, 0, subIntr->handler, sceKernelGetUserIntrStack());
    }
    else {
        int (*func)(int, int, int) = (void*)subIntr->handler;
        ret = func(arg2, subIntr->arg, arg3);
    }
    // 1D4C
    pspSetGp(oldGp);
    if (intInfo.monitorCbAfter != 0) {
        // 1D94
        intInfo.monitorCbAfter(intrNum, subIntrNum, arg2, subIntr->arg, arg3, ret, intInfo.opt2);
    }
    return ret;
}

// CD36EB65
int sceKernelGetUserIntrStack()
{
    if (intInfo.opt2 == 0)
        return intInfo.intrStack;
    return *(int*)(pspCop0CtrlGet(COP0_CTRL_SP_KERNEL) + 180);
}

// FFA8B183 (kernel) / CA04A2B9
int sceKernelRegisterSubIntrHandler(int intrNum, int subIntrNum, void *handler, void *arg)
{
    if (intrNum >= 68)
        return 0x80020065;
    int oldK1 = pspShiftK1();
    int oldIntr = sceKernelCpuSuspendIntr();
    Interrupt *intr = &intInfo.intr[intrNum];
    if (intr->handler == 0) {
        sceKernelCpuResumeIntr(oldIntr);
        pspSetK1(oldK1);
        return 0x80020068;
    }
    // 1ED4
    if (((int)handler >> 31) == 0 && pspK1IsUserMode()) // 200C
    {
        sceKernelCpuResumeIntr(oldIntr);
        pspSetK1(oldK1);
        return 0x80020065;
    }
    else
    {
        if (intInfo.intrStack == 0)
            return 0x80020065;
        if (((intr->v48 >> 11) & 1) == 0)
        {
            sceKernelCpuResumeIntr(oldIntr);
            pspSetK1(oldK1);
            return 0x80020065;
        }
    }
    // 1F00
    if (subIntrNum < 0 || subIntrNum >= (intr->v48 & 0xFF)) {
        sceKernelCpuResumeIntr(oldIntr);
        pspSetK1(oldK1);
        return 0x80020065;
    }
    SubInterrupt *subIntr = &intr->subIntrs[subIntrNum];
    if (subIntr->handler != 0)
    {
        sceKernelCpuResumeIntr(oldIntr);
        pspSetK1(oldK1);
        return 0x80020067;
    }
    // 1F3C
    if (((int)handler >> 31) == 0 && (((subIntr->v48 & 0xFF) >> 10) & 1) == 0)
    {
        sceKernelCpuResumeIntr(oldIntr);
        pspSetK1(oldK1);
        return 0x80020065;
    }
    // 1F60
    if (intr->cb != NULL && intr->cb->cbRegBefore != NULL)
    {
        // 1FEC
        int ret = intr->cb->cbRegBefore(intrNum, subIntrNum, handler, arg);
        if (ret != 0)
        {
            sceKernelCpuResumeIntr(oldIntr);
            pspSetK1(oldK1);
            return ret;
        }
    }
    subIntr->handler = (int)handler;
    // 1F78
    subIntr->gp = sceKernelGetModuleGPByAddressForKernel(handler);
    *(char*)&subIntr->v48 = 0;
    subIntr->arg = (int)arg;
    subIntr->v48 = (subIntr->v48 & 0xFFFF7FFF) | (intr->v48 & 0x00008000) | 0x80000300;
    if (intr->cb != NULL && intr->cb->cbRegAfter != NULL)
    {
        // 1FD4
        int ret = intr->cb->cbRegAfter(intrNum, subIntrNum, handler, arg);
        if (ret != 0)
        {
            // (moved 1FC4)
            subIntr->handler = 0;
            sceKernelCpuResumeIntr(oldIntr);
            pspSetK1(oldK1);
            return ret;
        }
    }
    sceKernelCpuResumeIntr(oldIntr);
    pspSetK1(oldK1);
    return 0;
}

// D61E6961 (both kernel and user)
int sceKernelReleaseSubIntrHandler(int intrNum, int subIntrNum)
{
    if (intrNum >= 68)
        return 0x80020065;
    int oldK1 = pspShiftK1();
    int oldIntr = sceKernelCpuSuspendIntr();
    Interrupt *intr = &intInfo.intr[intrNum];
    if (intr->handler == 0)
    {
        // 2108
        sceKernelCpuResumeIntr(oldIntr);
        pspSetK1(oldK1);
        return 0x80020068;
    }
    if (subIntrNum < 0 || subIntrNum >= (intr->v48 & 0xFF))
    {
        // 20A0
        sceKernelCpuResumeIntr(oldIntr);
        pspSetK1(oldK1);
        return 0x80020065;
    }
    // 20E0
    SubInterrupt *subIntr = &intr->subIntrs[subIntrNum];
    if (subIntr->handler == 0 || (subIntr->handler < 0 && pspK1IsUserMode()))
    {
        // 2108
        sceKernelCpuResumeIntr(oldIntr);
        pspSetK1(oldK1);
        return 0x80020068;
    }
    // 2110
    if (intr->cb != NULL && intr->cb->cbRelBefore != NULL)
    {
        // 2160
        int ret = intr->cb->cbRelBefore(intrNum, subIntrNum);
        if (ret != 0)
        {
            sceKernelCpuResumeIntr(oldIntr);
            pspSetK1(oldK1);
            return ret;
        }
    }
    // 2128
    int oldHandler = subIntr->handler;
    subIntr->handler = 0;
    if (intr->cb != NULL && intr->cb->cbRelAfter != NULL)
    {
        // 2150
        int ret = intr->cb->cbRelAfter(intrNum, subIntrNum);
        // (moved 2140)
        if (ret != 0)
        {
            subIntr->handler = oldHandler;
            sceKernelCpuResumeIntr(oldIntr);
            pspSetK1(oldK1);
            return ret;
        }
    }
    sceKernelCpuResumeIntr(oldIntr);
    pspSetK1(oldK1);
    return 0;
}

// FB8E22EC (both user and kernel)
int sceKernelEnableSubIntr(int intrNum, int subIntrNum)
{
    if (intrNum >= 68)
        return 0x80020065;
    int oldK1 = pspShiftK1();
    int oldIntr = sceKernelCpuSuspendIntr();
    Interrupt *intr = &intInfo.intr[intrNum];
    if (subIntrNum < 0 || subIntrNum >= (intr->v48 & 0xFF)
     || (pspK1IsUserMode() && (((intr->v48 >> 11) & 1) == 0
                              || ((intr->subIntrs[subIntrNum].v48 >> 10) & 1) == 0)))
    {
        // 2210
        sceKernelCpuResumeIntr(oldIntr);
        pspSetK1(oldK1);
        return 0x80020065;
    }
    // 2248
    if (intr->handler == 0 || intr->cb == NULL || intr->cb->cbEnable == NULL)
    {
        // 226C
        sceKernelCpuResumeIntr(oldIntr);
        pspSetK1(oldK1);
        return 0x80020068;
    }
    // 2274
    int ret = intr->cb->cbEnable(intrNum, subIntrNum);
    sceKernelCpuResumeIntr(oldIntr);
    pspSetK1(oldK1);
    return ret;
}

// 4023E1A7 (kernel) / 8A389411 (user)
int sceKernelDisableSubIntr(int intrNum, int subIntrNum)
{
    if (intrNum >= 68)
        return 0x80020065;
    int oldK1 = pspShiftK1();
    int oldIntr = sceKernelCpuSuspendIntr();
    Interrupt *intr = &intInfo.intr[intrNum];
    if (subIntrNum < 0 || subIntrNum >= (intr->v48 & 0xFF)
     || (pspK1IsUserMode() && (((intr->v48 >> 11) & 1) == 0
                              || ((intr->subIntrs[subIntrNum].v48 >> 10) & 1) == 0)))
    {
        // 231C
        sceKernelCpuResumeIntr(oldIntr);
        pspSetK1(oldK1);
        return 0x80020065;
    }
    // 2354
    if (intr->handler == 0 || intr->cb == NULL || intr->cb->cbDisable == NULL)
    {
        // 2378
        sceKernelCpuResumeIntr(oldIntr);
        pspSetK1(oldK1);
        return 0x80020068;
    }
    int ret = intr->cb->cbDisable(intrNum, subIntrNum);
    sceKernelCpuResumeIntr(oldIntr);
    pspSetK1(oldK1);
    return ret;
}

// C495F536 (kernel) / 5CB5A78B (user)
int sceKernelSuspendSubIntr(int intrNum, int subIntrNum, int *arg2)
{
    if (intrNum >= 68)
        return 0x80020065;
    int oldK1 = pspShiftK1();
    if (!pspK1PtrOk(arg2))
    {
        // 24C4
        pspSetK1(oldK1);
        return 0x800200D3;
    }
    int oldIntr = sceKernelCpuSuspendIntr();
    Interrupt *intr = &intInfo.intr[intrNum];
    if (subIntrNum < 0 || subIntrNum >= (intr->v48 & 0xFF)
     || (pspK1IsUserMode() && (((intr->v48 >> 11) & 1) == 0
                              || ((intr->subIntrs[subIntrNum].v48 >> 10) & 1) == 0)))
    {
        // 2440
        sceKernelCpuResumeIntr(oldIntr);
        pspSetK1(oldK1);
        return 0x80020065;
    }
    // 247C
    if (intr->handler == 0 || intr->cb == NULL || intr->cb->cbSuspend == NULL)
    {
        // 24A0
        if (arg2 != NULL)
            *arg2 = 0;
        sceKernelCpuResumeIntr(oldIntr);
        pspSetK1(oldK1);
        return 0x80020068;
    }
    
    int ret = intr->cb->cbSuspend(intrNum, subIntrNum, arg2);
    sceKernelCpuResumeIntr(oldIntr);
    pspSetK1(oldK1);
    return ret;
}

// 2980AE03 (kernel) / 7860E0DC (user)
int sceKernelResumeSubIntr(int intrNum, int subIntrNum, int arg2)
{
    if (intrNum >= 0)
        return 0x80020065;
    int oldK1 = pspShiftK1();
    int oldIntr = sceKernelCpuSuspendIntr();
    Interrupt *intr = &intInfo.intr[intrNum];
    if (subIntrNum < 0 || subIntrNum >= (intr->v48 & 0xFF)
     || (pspK1IsUserMode() && (((intr->v48 >> 11) & 1) == 0
                              || ((intr->subIntrs[subIntrNum].v48 >> 10) & 1) == 0)))
    {
        // 2574
        sceKernelCpuResumeIntr(oldIntr);
        pspSetK1(oldK1);
        return 0x80020065;
    }
    // 25B0
    if (intr->handler == 0 || intr->cb == NULL || intr->cb->cbResume == NULL)
    {
        // 25D4
        sceKernelCpuResumeIntr(oldIntr);
        pspSetK1(oldK1);
        return 0x80020068;
    }
    int ret = intr->cb->cbResume(intrNum, subIntrNum, arg2);
    sceKernelCpuResumeIntr(oldIntr);
    pspSetK1(oldK1);
    return ret;
}

// 4351DD4E (kernel) / FC4374B8 (user)
int sceKernelIsSubInterruptOccured(int intrNum, int subIntrNum)
{
    if (intrNum >= 68)
        return 0x80020065;
    int oldK1 = pspShiftK1();
    int oldIntr = sceKernelCpuSuspendIntr();
    Interrupt *intr = &intInfo.intr[intrNum];
    if (subIntrNum < 0 || subIntrNum >= (intr->v48 & 0xFF)
     || (pspK1IsUserMode() && (((intr->v48 >> 11) & 1) == 0
                              || ((intr->subIntrs[subIntrNum].v48 >> 10) & 1) == 0)))
    {
        // 2688
        sceKernelCpuResumeIntr(oldIntr);
        pspSetK1(oldK1);
        return 0x80020065;
    }
    // 26C0
    if (intr->handler == 0 || intr->cb == NULL || intr->cb->cbIsOccured == NULL)
    {
        // 26E4
        sceKernelCpuResumeIntr(oldIntr);
        pspSetK1(oldK1);
        return 0x80020068;
    }
    // 26EC
    int ret = intr->cb->cbIsOccured(intrNum, subIntrNum);
    sceKernelCpuResumeIntr(oldIntr);
    pspSetK1(oldK1);
    return ret;
}

// 68B5CA51
int sceKernelQueryIntrHandlerInfo(int intrNum, int subIntrNum, int out)
{
    if (intrNum >= 68)
        return 0x80020065;
    if (*(int*)(out + 0) != 56)
        return 0x8002006B;
    int oldIntr = sceKernelCpuSuspendIntr();
    Interrupt *intr = &intInfo.intr[intrNum];
    char numSubIntr = intr->v48 & 0xFF;
    if (numSubIntr != 0)
    {
        if (subIntrNum < 0 || subIntrNum >= numSubIntr)
        {
            // (2918)
            // 291C
            if (subIntrNum != -1) {
                sceKernelCpuResumeIntr(oldIntr);
                return 0x80020065;
            }
        }
        else
        {
            if (intr->subIntrs == NULL)
                subIntrNum = 0;
            else
                intr = (Interrupt*)&intr->subIntrs[subIntrNum];
        }
    }
    // (2794)
    // 2798
    *(int*)(out + 8) = intr->arg;
    *(int*)(out + 4) = intr->handler;
    *(int*)(out + 12) = intr->gp;
    *(short*)(out + 16) = intrNum;
    *(short*)(out + 20) = (intr->v48 >> 8) & 3;
    if (intrNum >= 64)
    {
        // 28D0
        int st = pspCop0StateGet(COP0_STATE_STATUS) & 0xFF00;;
        int mask;
        if (intrNum == 64)
            mask = 0x0100;
        else if (intrNum == 65)
            mask = 0x0200;
        else if (intrNum == 66)
            mask = 0x1000;
        else if (intrNum == 67)
            mask = 0x8000;
        else
            mask = 0;
        // 2908
        *(short*)(out + 22) = (st & mask) != 0;
    }
    else
    {
        char bit = (intInfo.grpsOpt[(intrNum >> 5) + 24] >> (intrNum & 0x1F)) & 1;
        *(short*)(out + 22) = bit;
        if ((bit & (~subIntrNum >> 31)) != 0)
        {
            if (intr->handler != 0)
            {
                // 287C
                if (intr->cb != NULL && intr->cb->cbSuspend != NULL && intr->cb->cbResume != NULL)
                {
                    int sp;
                    intr->cb->cbSuspend(intrNum, subIntrNum, &sp);
                    intr->cb->cbResume(intrNum, subIntrNum, sp);
                    *(short*)(out + 22) = (sp != 0);
                }
            }
            else
                *(short*)(out + 22) = 0;
        }
    }
    // 280C
    *(int*)(out + 32) = intr->u32;
    *(int*)(out + 36) = intr->u36;
    *(short*)(out + 18) = intr->v48 & 0xFF;
    *(int*)(out + 40) = intr->u16;
    *(int*)(out + 44) = intr->u20;
    *(int*)(out + 24) = intr->u12;
    *(int*)(out + 48) = intr->u24;
    *(int*)(out + 52) = intr->u28;
    sceKernelCpuResumeIntr(oldIntr);
    return 0;
}

// 2938
void *mymemset(void *dstVoid, char c, int n)
{
    char *dst = (char*)dstVoid;
    if (dst != NULL)
    {
        // 2950
        char *actDst = dst;
        while ((n--) > 0)
            *(actDst++) = c;
    }
    return dst;
}

// 2968
void InterruptDisableInTable(int intrNum)
{
    int mask = ~(1 << (intrNum & 0x1F));
    int *ptr = &intInfo.grpsOpt[intrNum >> 5];
    int i = 4;
    // 2990
    while ((i--) >= 0) {
        *ptr &= mask;
        ptr += 2;
    }
}

void sub_29B0(int intrNum)
{
    int val = (intInfo.intr[intrNum].v48 >> 8) & 3;
    int mask = 1 << (intrNum & 0x1F);
    int *ptr = &intInfo.grpsOpt[(intrNum >> 5) + (val << 1)];
    // 29F8
    while ((val++) < 4) {
        *ptr |= mask;
        ptr += 2;
    }
    // 2A14
    if (sceKernelIsIntrContext() == 0) {
        // 2A30
        sub_10A8(&intInfo.grpsOpt[6]);
    }
}

// 2A40
void AllLevelInterruptDisable(int intrNum)
{
    int *ptr = &intInfo.grpsOpt[intrNum >> 5];
    int mask = ~(1 << (intrNum & 0x1F));
    int i = 11;
    // 2A78
    while ((--i) >= 0) {
        *ptr &= mask;
        ptr += 2;
    }
    int sp[2];
    sub_1080(sp);
    sp[intrNum >> 5] &= mask;
    sub_10A8(sp);
}

// 55D18836
int sceKernelSetPrimarySyscallHandler(int arg0, void (*arg1)())
{
    int oldIntr = sceKernelCpuSuspendIntr();
    int shift = arg0 * 4;
    if (arg0 <= 0 || cbMap.unk2 + cbMap.unk3 < shift) {
        sceKernelCpuResumeIntr(oldIntr);
        return 0x80020038;
    }
    if (cbMap.callbacks[arg0] != cbMap.callbacks[0]) {
        sceKernelCpuResumeIntr(oldIntr);
        return 0x80020039;
    }
    // 2B68
    if ((int)arg1 >= 0)
    {
        // 2B8C
        sceKernelCpuResumeIntr(oldIntr);
        return 0x800200D3;
    }
    cbMap.callbacks[arg0] = arg1;
    pspCache(0x1A, &cbMap.callbacks[arg0]);
    sceKernelCpuResumeIntr(oldIntr);
    return 0;
}

int IntrManTerminate()
{
    pspCop0StateSet(COP0_STATE_STATUS, pspCop0StateGet(COP0_STATE_STATUS) & 0xFFFF7BFF);
    sceKernelReleaseIntrHandler(67);
    sceKernelReleaseExceptionHandler(EXCEP_INT, (void*)sub_0038);
    sceKernelReleaseExceptionHandler(EXCEP_INT, (void*)sub_0924);
    sceKernelReleaseExceptionHandler(EXCEP_SYS, (void*)sub_0CF8);
    return 0;
}

// 02314986
void sceKernelCpuEnableIntr()
{
    asm("mtic %0, $0" : : "r" (1));
}

int InterruptManagerForKernel_6FCBA912(int set)
{
    intInfo.opt = (set == 0);
    return 0;
}

// A4C1C627
int sceKernelClearIntrLogging(int intrNum)
{
    if (intrNum >= 68)
        return 0x80020065;
    int oldIntr = sceKernelCpuSuspendIntr();
    Interrupt *intr = &intInfo.intr[intrNum];
    intr->u16 = -1;
    intr->u20 = -1;
    intr->u24 = 0;
    intr->u28 = 0;
    intr->u32 = 0;
    intr->u36 = 0;
    intr->u12 = 0;
    sceKernelCpuResumeIntr(oldIntr);
    return 0;
}

// F2F1E983
int sceKernelIsInterruptOccurred(int intrNum)
{
    int sp[2];
    if (intrNum >= 64)
        return 0x80020065;
    sub_1050(sp);
    return (sp[intrNum >> 5] >> (intrNum & 0x1F)) & 1;
}

// D774BA45
int sceKernelDisableIntr(int intrNum)
{
    return sceKernelSuspendIntr(intrNum, 0);
}

// DBD52A5D
void RegisterSubIntrruptMonitor(MonitorCb before, MonitorCb after)
{
    intInfo.monitorCbAfter = after;
    intInfo.monitorCbBefore = before;
}

// 19596CD3
void ReleaseSubIntrruptMonitor()
{
    intInfo.monitorCbAfter = 0;
    intInfo.monitorCbBefore = 0;
}

// 8357E7FA
int UnSupportIntr(int intrNum)
{
    if (intrNum >= 64)
        return 0x80020065;
    int oldIntr = sceKernelCpuSuspendIntr();
    sceKernelSuspendIntr(intrNum, 0);
    Interrupt *intr = &intInfo.intr[intrNum];
    intr->handler = 1;
    intr->v48 &= 0xFFFFFCFF;
    sceKernelCpuResumeIntr(oldIntr);
    return 0;
}

int InterruptManagerForKernel_8DFBD787()
{
    if (intInfo.opt2 != 0)
    {
        int addr = pspCop0CtrlGet(COP0_CTRL_TCB);
        if (addr != 0)
            return *(int*)addr;
    }
    return 0;
}

// D2E8363F
int QueryIntrHandlerInfoForUser()
{
    return 0x80020001;
}

// EEE43F47
int sceKernelRegisterUserSpaceIntrStack(int addr, int size, int arg2)
{
    if (size != 0x2000)
        return 0x80000104;
    int oldIntr = sceKernelCpuSuspendIntr();
    if (intInfo.intrStack != 0) {
        sceKernelCpuResumeIntr(oldIntr);
        return 0x8002006D;
    }
    if (addr < 0 || arg2 < 0) {
        sceKernelCpuResumeIntr(oldIntr);
        return 0x8002006C;
    }
    intInfo.intrStackArg = arg2;
    intInfo.intrStack = addr + 0x2000;
    // 2EF8
    sceKernelCpuResumeIntr(oldIntr);
    return 0;
}

// 30C08374
int sceKernelGetCpuClockCounter()
{
    return pspCop0StateGet(COP0_STATE_COUNT);
}

// F9E06DF1
u64 sceKernelGetCpuClockCounterWide()
{
    int oldIntr = sceKernelCpuSuspendIntr();
    u32 hi = intInfo.clockCounterHi;
    u32 count = pspCop0StateGet(COP0_STATE_COUNT);
    if (count >= intInfo.clockCounterLo)
        intInfo.clockCounterLo = count;
    else
        hi++;
    // 2EDC
    sceKernelCpuResumeIntr(oldIntr);
    return ((u64)hi << 32) | (u64)count;
}

// 6DDA4D7B
int *QueryInterruptManCB()
{
    return intInfo.grpsOpt;
}

// E6FB16E3
u32 _sceKernelGetCpuClockCounterLow()
{
    return intInfo.clockCounterLo;
}

// 14D4C61A
int sceKernelRegisterSystemCallTable(CbMap *newMap)
{
    if (newMap->next != NULL)
        return 0x80020036;
    int limit = newMap->unk2;
    if (limit < 0 || newMap->unk3 - 16 < limit) {
        // 2FF0
        return 0x80020037;
    }
    // 3008
    int oldIntr = sceKernelCpuSuspendIntr();
    CbMap *map = cbMap.next;
    CbMap *oldMap = &cbMap;
    // 302C
    while (map->unk1 != 0) {
        oldMap = map;
        map = map->next;
    }
    // 305C is useless?
    // 303C
    newMap->next = map;
    oldMap->next = newMap;
    pspCop0CtrlSet(COP0_CTRL_SC_TABLE, (int)cbMap.next);
    sceKernelCpuResumeIntr(oldIntr);
    return 0;
}

// F153B371
int sceKernelQuerySystemCall(int (*arg)())
{
    CbMap *cur = &cbMap;
    // 2FE4
    do
    {
        int count = (cur->unk3 < 16) ? cur->unk3 - 13 : cur->unk3;
        count /= 4;
        // 3008
        int i;
        for (i = 0; i < count; i++)
        {
            if ((void*)((int)cur->callbacks[i] | 0x80000000) == arg)
            {
                cur->callbacks[i] = (void*)((int)cur->callbacks[i] | 0x80000000);
                // 303C
                return (cur->unk1 < 0 ? cur->unk1 + 3 : cur->unk1) * 4 + i;
            }
        }
        // 3028
        cur = cur->next;
    } while (cur != NULL);
    return -1;
}

void InterruptManagerForKernel_E526B767(int arg)
{
    if (arg == 0)
        *(int*)(mod_0E48) = 0x42000018; // eret
    // 30F0
    if (arg == 1)
    {
        // 3104
        *(int*)(mod_0E50) = 0x00005821; // t3 = 0
    }
}

// 3110
int SuspendIntc()
{
    int sp[2];
    intInfo.compare = pspCop0StateGet(COP0_STATE_COMPARE);
    intInfo.count = pspCop0StateGet(COP0_STATE_COUNT);
    sub_1080(intInfo.intcState);
    mymemset(sp, 0, 8);
    sub_10A8(sp);
    return 0;
}

// 3160
int ResumeIntc()
{
    sub_10A8(intInfo.intcState);
    pspCop0StateSet(COP0_STATE_COMPARE, intInfo.compare);
    pspCop0StateSet(COP0_STATE_COUNT, intInfo.count);
    return 0;
}

