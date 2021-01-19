/* Copyright (C) 2011, 2012 The uOFW team
   See the file COPYING for copying permission.
*/

#include <common_imp.h>
#include <exceptionman.h>
#include <sysmem_kernel.h>
#include <sysmem_suspend_kernel.h>

#include <interruptman.h>

#include "end.h"
#include "interruptman.h"
#include "start.h"

SceResidentLibraryEntryTable intrEntry = {
    .libName = "InterruptManager",
    .version = {
        0x11,
        0
    },
    .attribute = 0x4000,
    .len = 4,
    .vStubCount = 0,
    .stubCount = 9,
    .entryTable = (void*)(s32[]) {
        0x5CB5A78B,
        0x7860E0DC,
        0x8A389411,
        0xCA04A2B9,
        0xD2E8363F,
        0xD61E6961,
        0xEEE43F47,
        0xFB8E22EC,
        0xFC4374B8,
        (s32)sceKernelSuspendSubIntr,
        (s32)sceKernelResumeSubIntr,
        (s32)sceKernelDisableSubIntr,
        (s32)sceKernelRegisterSubIntrHandler,
        (s32)QueryIntrHandlerInfoForUser,
        (s32)sceKernelReleaseSubIntrHandler,
        (s32)sceKernelRegisterUserSpaceIntrStack,
        (s32)sceKernelEnableSubIntr,
        (s32)sceKernelIsSubInterruptOccured
    },
    .unk16 = 0,
    .unk18 = 0,
    .unk19 = 0
};

// 0x3458
SCE_MODULE_INFO("sceInterruptManager", SCE_MODULE_KERNEL | SCE_MODULE_ATTR_CANT_STOP | SCE_MODULE_ATTR_EXCLUSIVE_LOAD |
                                       SCE_MODULE_ATTR_EXCLUSIVE_START, 1, 9);
SCE_MODULE_BOOTSTART("IntrManInit");
SCE_MODULE_REBOOT_BEFORE("IntrManTerminate");
SCE_SDK_VERSION(SDK_VERSION);

SceSyscallTable g_emptyTable = { // 0x3810
    .next = NULL,
    .seed = 0,
    .tableSize = 0,
    .funcTableSize = 0,
};

SceSyscallTable g_syscallTable = { // 0x3700
    .next = &g_emptyTable,
    .seed = 0,
    .tableSize = 0x00000100,
    .funcTableSize = 0x00000110,
    .syscalls = {
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

s8 g_stackBottom[672]; // 0x3820?

s8 g_stackMiddle[7232]; // 0x3AC0

s8 g_stackTop[304]; // 0x5700

s8 g_syscCtx[16]; // 0x5830

typedef struct
{
    s32 grpsOpt[24]; // 0
    s8 unused2; // 96
    s8 opt2; // 97
    s8 opt; // 98
    s8 unused3; // 99
    MonitorCb monitorCbBefore; // 100
    MonitorCb monitorCbAfter; // 104
    s32 unused; // 108
    s32 intrStack; // 112
    s32 intrStackArg; // 116
    u32 clockCounterLo; // 120
    u32 clockCounterHi; // 124
    Interrupt intr[68]; // 128
    s32 subIntrMemoryPoolId; // 4088
    s32 intcState[2]; // 4092
    s32 count;
    s32 compare;
} InterruptInfo;

InterruptInfo intInfo; // 0x5840

s32 IntrManInit()
{
    dbg_init(1, FB_NONE, FAT_HARDWARE);
    dbg_printf("Called %s\n", __FUNCTION__);
    s32 oldIc = sceKernelCpuSuspendIntr();
    pspCop0CtrlSet(COP0_CTRL_IS_INTERRUPT, 0);
    pspCop0CtrlSet(COP0_CTRL_SP_KERNEL, (s32)g_stackTop);
    pspCop0CtrlSet(COP0_CTRL_SP_USER, 0);
    mymemset(&intInfo, 0, sizeof(InterruptInfo));
    intInfo.opt = 1;
    sub_1030();
    ReleaseContextHooks();
    // 1150
    s32 i;
    for (i = 67; i >= 0; i--) {
        intInfo.intr[i].u16 = -1;
        intInfo.intr[i].u20 = -1;
    }
    sceKernelRegisterExceptionHandler(EXCEP_INT, (void*)intrExcepHandler);
    sceKernelRegisterPriorityExceptionHandler(EXCEP_INT, 3, (void*)intrExcepHandler2);
    sceKernelRegisterExceptionHandler(EXCEP_SYS, (void*)syscallExcepHandler);
    sceKernelRegisterIntrHandler(67, 0, sub_0000, 0, 0);
    sceKernelRegisterSuspendHandler(29, SuspendIntc, 0);
    sceKernelRegisterResumeHandler(29, ResumeIntc, 0);
    sceKernelSetIntrLogging(67, 0);
    pspCop0StateSet(COP0_STATE_STATUS, (pspCop0StateGet(COP0_STATE_STATUS) & 0xFFFF00FF) | 0x0400);
    sceKernelEnableIntr(67);
    pspCop0StateSet(COP0_STATE_COUNT, 0);
    pspCop0StateSet(COP0_STATE_COMPARE, 0x80000000);
    sceKernelCpuResumeIntr(oldIc);
    s32 ret = sceKernelRegisterLibrary(&intrEntry);
    dbg_printf("init ok\n");
    if (ret < 0)
        return ret;
    return 0;
}

// 58DD8978
s32 sceKernelRegisterIntrHandler(s32 intrNum, s32 arg1, void *func, void *arg3, SceIntrHandler *handler)
{
    dbg_printf("sceKernelRegisterIntrHandler(%d, %d, %08x, %08x, %08x)\n", intrNum, arg1, func, arg3, handler);
    if (sceKernelIsIntrContext() != 0)
        return SCE_ERROR_KERNEL_CANNOT_BE_CALLED_FROM_INTERRUPT;
    if (intrNum >= 68)
        return SCE_ERROR_KERNEL_INVALID_INTERRUPT;
    if (handler != NULL && handler->size != 12)
        return SCE_ERROR_KERNEL_INVALID_INTERRUPT_ARGUMENT;
    // 12B8
    s32 oldIc = sceKernelCpuSuspendIntr();
    if (handler != NULL && handler->size > 0 && intInfo.subIntrMemoryPoolId == 0) {
        // 1468
        s32 ret = sceKernelCreateHeap(1, 1, 0x800, "SceInterruptManager");
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
        return SCE_ERROR_KERNEL_HANDLER_ALREADY_EXISTS;
    }
    // 1354
    if ((s32)func >= 0) {
        sceKernelCpuResumeIntr(oldIc);
        return SCE_ERROR_KERNEL_INVALID_INTERRUPT;
    }
    intr->subIntrs = NULL;
    // 1368
    intr->cb = NULL;
    s32 size;
    if (handler == NULL || (size = handler->attr) <= 0) {
        // 144C
        if (intr->subIntrs != NULL) {
            sceKernelFreeHeapMemory(intInfo.subIntrMemoryPoolId, intr->subIntrs); // Free memory space
            intr->subIntrs = NULL;
        }
    } else {
        SubInterrupt *subIntrs = sceKernelAllocHeapMemory(intInfo.subIntrMemoryPoolId, size); // Allocate space to store the subIntrs
        if (subIntrs != NULL)
            mymemset(subIntrs, 0, sizeof(*subIntrs));
        // 13A4
        intr->subIntrs = subIntrs;
        if (subIntrs == NULL) {
            // 1440
            sceKernelCpuResumeIntr(oldIc);
            return SCE_ERROR_KERNEL_NO_MEMORY;
        }
        intr->cb = handler->cb;
    }
    // 13B4
    sceKernelSuspendIntr(intrNum, 0);
    s32 unk2;
    if (intr->handler != 1 && intrNum < 64)
        unk2 = 3;
    else
        unk2 = 0;
    // 13E0
    if (arg1 != 0)
        arg1 = 2;
    intr->v48 = (intr->v48 & 0xFFFFFCFF) | ((unk2 << 8) & 0x300);
    intr->handler = arg1 | (s32)func;
    intr->gp = sceKernelGetModuleGPByAddressForKernel((u32)func);
    intr->arg = arg3;
    *(s8*)&intr->v48 = 0;
    if (handler != NULL)
        *(s8*)&intr->v48 = *(s8*)&handler->attr;
    // 1420
    intr->v48 = ((intr->v48 & 0x7FFF7FFF) | ((intInfo.opt << 15) & 0x8000)) & 0x7FFFFFFF;
    *(s8*)(&intr->v48 + 2) = intrNum;
    sceKernelCpuResumeIntr(oldIc);
    return 0;
}

// A1B88367
s32 sceKernelSetUserModeIntrHanlerAcceptable(s32 intrNum, s32 subIntrNum, s32 setBit)
{
    dbg_printf("Called %s\n", __FUNCTION__);
    if (sceKernelIsIntrContext() != 0)
        return SCE_ERROR_KERNEL_CANNOT_BE_CALLED_FROM_INTERRUPT;
    if (intrNum >= 68)
        return SCE_ERROR_KERNEL_INVALID_INTERRUPT;
    s32 oldIntr = sceKernelCpuSuspendIntr();
    Interrupt *intr = &intInfo.intr[intrNum];
    if (intr->handler == 0 || (intr->handler & 3) == 1) {
        // 15F4
        sceKernelCpuResumeIntr(oldIntr);
        return SCE_ERROR_KERNEL_HANDLER_NOTFOUND;
    }
    if (subIntrNum < 0 || subIntrNum >= (intr->v48 & 0xFF)) {
        // 1538
        sceKernelCpuResumeIntr(oldIntr);
        return SCE_ERROR_KERNEL_INVALID_INTERRUPT;
    }
    // 156C
    SubInterrupt *subIntr = &intr->subIntrs[subIntrNum];
    subIntr->v48 = (subIntr->v48 & 0xFFFFFBFF) | (((setBit != 0) << 10) & 0x00000400);
    intr->v48 &= 0xFFFFF7FF;
    s8 num = intr->v48 & 0xFF;
    s8 v = (subIntr->v48 >> 10) & 1;
    // 15B4
    s32 i;
    for (i = 0; i < num; i++) {
        if (v != 0) {
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
s32 sceKernelReleaseIntrHandler(s32 intrNum)
{
    dbg_printf("Called %s\n", __FUNCTION__);
    if (sceKernelIsIntrContext() != 0)
        return SCE_ERROR_KERNEL_CANNOT_BE_CALLED_FROM_INTERRUPT;
    if (intrNum >= 68)
        return SCE_ERROR_KERNEL_INVALID_INTERRUPT;
    s32 oldIntr = sceKernelCpuSuspendIntr();
    Interrupt *intr = &intInfo.intr[intrNum];
    if (intr->handler == 0) {
        sceKernelCpuResumeIntr(oldIntr);
        return SCE_ERROR_KERNEL_HANDLER_NOTFOUND;
    }
    // 1698
    if (((intr->v48 >> 8) & 3) != 0) {
        // 16CC
        intr->handler = 0;
        sceKernelSuspendIntr(intrNum, 0);
    } else
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
s32 sceKernelSetIntrLevel(s32 intrNum, s32 num)
{
    dbg_printf("Called %s\n", __FUNCTION__);
    if (intrNum >= 64)
        return SCE_ERROR_KERNEL_INVALID_INTERRUPT;
    if (num < 1 || num >= 4)
        return SCE_ERROR_KERNEL_INVALID_INTERRUPT_LEVEL;
    Interrupt *intr = &intInfo.intr[intrNum];
    s32 oldIntr = sceKernelCpuSuspendIntr();
    if (intr->handler == 0 || (intr->handler & 3) == 1) {
        // 1814
        sceKernelCpuResumeIntr(oldIntr);
        return SCE_ERROR_KERNEL_HANDLER_NOTFOUND;
    }
    if (((intr->v48 >> 11) & 1) != 0 && ((num ^ 2) != 0)) {
        // 1800
        sceKernelCpuResumeIntr(oldIntr);
        return SCE_ERROR_KERNEL_INVALID_INTERRUPT_LEVEL;
    }
    if (((intInfo.grpsOpt[(intrNum >> 5) + 24] >> (intrNum & 0x1F)) & 1) == 0) {
        // 17F4
        intr->v48 = (intr->v48 & 0xFFFFFCFF) | ((num << 8) & 0x00000300);
    } else {
        if (((intr->v48 >> 8) & 3) < num) {
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
s32 sceKernelSetIntrLogging(s32 intrNum, s32 arg1)
{
    dbg_printf("Called %s\n", __FUNCTION__);
    if (intrNum >= 68)
        return SCE_ERROR_KERNEL_INVALID_INTERRUPT;
    s32 oldIc = sceKernelCpuSuspendIntr();
    Interrupt *intr = &intInfo.intr[intrNum];
    if (arg1 == 0) {
        // 189C
        intr->v48 |= 0x00008000;
    } else
        intr->v48 &= 0xFFFF7FFF;
    // 1878
    sceKernelCpuResumeIntr(oldIc);
    return 0;
}

// 4D6E7305
s32 sceKernelEnableIntr(s32 intNum)
{
    dbg_printf("sceKernelEnableIntr(%d)\n", intNum);
    s32 v;
    if (intNum >= 68)
        return SCE_ERROR_KERNEL_INVALID_INTERRUPT;
    s32 oldIc = sceKernelCpuSuspendIntr();
    if (intInfo.intr[intNum].handler == 0) {
        sceKernelCpuResumeIntr(oldIc);
        return SCE_ERROR_KERNEL_HANDLER_NOTFOUND;
    }
    // 1928
    if (intNum < 64) {
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
s32 sceKernelSuspendIntr(s32 arg0, s32 *arg1)
{
    dbg_printf("sceKernelSuspendIntr(%d, %08x)\n", arg0, arg1);
    s32 ret = 0;
    s32 mask;
    if (arg0 >= 68)
        return SCE_ERROR_KERNEL_INVALID_INTERRUPT;
    s32 oldIc = sceKernelCpuSuspendIntr();
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
    if (intInfo.intr[arg0].handler != 0) {
        // 1A78
        if (arg1 != NULL) {
            if (arg0 >= 64) {
                // 1AA8
                *arg1 = (pspCop0StateGet(COP0_STATE_STATUS) & 0xFF00) | mask;
            } else
                *arg1 = (intInfo.grpsOpt[(arg0 >> 5) + 24] >> (arg0 & 0x1F)) & 1;
        }
    } else {
        ret = SCE_ERROR_KERNEL_HANDLER_NOTFOUND;
        if (arg1 != NULL)
            *arg1 = 0;
    }
    // 1A24
    if (arg0 < 64) {
        // 1A68
        AllLevelInterruptDisable(arg0);
    } else
        pspCop0StateSet(COP0_STATE_STATUS, pspCop0StateGet(COP0_STATE_STATUS) & ~(mask & 0xFF00));
    // 1A3C
    sceKernelCpuResumeIntr(oldIc);
    return ret;
}

// DB14CBE0
s32 sceKernelResumeIntr(s32 intrNum, s32 arg1)
{
    dbg_printf("Called %s\n", __FUNCTION__);
    if (intrNum >= 68)
        return SCE_ERROR_KERNEL_INVALID_INTERRUPT;
    s32 oldIntr = sceKernelCpuSuspendIntr();
    s32 mask;
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
    if (intInfo.intr[intrNum].handler != 0) {
        // 1BA8
        if (intrNum >= 64) {
            // 1BC8
            if (arg1 != 0) {
                pspCop0StateSet(COP0_STATE_STATUS, pspCop0StateGet(COP0_STATE_STATUS) | (mask & 0xFF00));
                sceKernelCpuResumeIntr(oldIntr);
                return 0;
            }
        } else {
            if (arg1 == 0) {
                // 1B98
                AllLevelInterruptDisable(intrNum);
            } else
                sub_29B0(intrNum);
            sceKernelCpuResumeIntr(oldIntr);
            return 0;
        }
    } else if (intrNum < 64) {
        // 1B98
        AllLevelInterruptDisable(intrNum);
        sceKernelCpuResumeIntr(oldIntr);
        return SCE_ERROR_KERNEL_HANDLER_NOTFOUND;
    } else {
        pspCop0StateSet(COP0_STATE_STATUS, pspCop0StateGet(COP0_STATE_STATUS) | ~(mask & 0xFF00));
        sceKernelCpuResumeIntr(oldIntr);
        return SCE_ERROR_KERNEL_HANDLER_NOTFOUND;
    }
    // 1B5C
    pspCop0StateSet(COP0_STATE_STATUS, pspCop0StateGet(COP0_STATE_STATUS) | ~(mask & 0xFF00));
    sceKernelCpuResumeIntr(oldIntr);
    return 0;
}

// DB9A5496
void ReleaseContextHooks()
{
    dbg_printf("Called %s\n", __FUNCTION__);
    InterruptManagerForKernel_E790EAED(sub_091C, sub_091C);
}

void InterruptManagerForKernel_E790EAED(s32 (*arg0)(), s32 (*arg1)())
{
    dbg_printf("Called %s\n", __FUNCTION__);
    s32 oldIc = sceKernelCpuSuspendIntr();
    mod_0468 = 0x0C000000 + (((s32)arg1 >> 2) & 0x3FFFFFF);
    pspCache(0x1A, &mod_0468);
    pspCache(0x08, &mod_0468);
    mod_0400 = 0x0C000000 + (((s32)arg0 >> 2) & 0x3FFFFFF);
    pspCache(0x1A, &mod_0400);
    pspCache(0x08, &mod_0400);
    sceKernelCpuResumeIntr(oldIc);
}

// 0C5F7AE3
s32 sceKernelCallSubIntrHandler(s32 intrNum, s32 subIntrNum, s32 arg2, s32 arg3)
{
    dbg_printf("Called %s\n", __FUNCTION__);
    SubInterrupt *subIntr = &intInfo.intr[intrNum].subIntrs[subIntrNum];
    if (subIntr->handler < 2)
        return SCE_ERROR_KERNEL_HANDLER_NOTFOUND;
    if (intInfo.monitorCbBefore != NULL) {
        if (((subIntr->v48 >> 15) & 1) == 0)
            subIntr->u12++;
        // 1D18
        intInfo.monitorCbBefore(intrNum, subIntrNum, arg2, subIntr->arg, arg3, subIntr->handler, intInfo.opt2);
    }
    // 1D28
    s32 oldGp = pspGetGp();
    pspSetGp(subIntr->gp);
    s32 ret;
    if (subIntr->handler >= 0) {
        // 1DB8
        ret = sceKernelCallUserIntrHandler(arg2, subIntr->arg, arg3, 0, subIntr->handler, sceKernelGetUserIntrStack());
    } else {
        s32 (*func)(s32, s32, s32) = (void*)subIntr->handler;
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
s32 sceKernelGetUserIntrStack()
{
    dbg_printf("Called %s\n", __FUNCTION__);
    if (intInfo.opt2 == 0)
        return intInfo.intrStack;
    return *(s32*)(pspCop0CtrlGet(COP0_CTRL_SP_KERNEL) + 180);
}

// FFA8B183 (kernel) / CA04A2B9
s32 sceKernelRegisterSubIntrHandler(s32 intrNum, s32 subIntrNum, void *handler, void *arg)
{
    dbg_printf("Called %s\n", __FUNCTION__);
    if (intrNum >= 68)
        return SCE_ERROR_KERNEL_INVALID_INTERRUPT;
    s32 oldK1 = pspShiftK1();
    s32 oldIntr = sceKernelCpuSuspendIntr();
    Interrupt *intr = &intInfo.intr[intrNum];
    if (intr->handler == 0) {
        sceKernelCpuResumeIntr(oldIntr);
        pspSetK1(oldK1);
        return SCE_ERROR_KERNEL_HANDLER_NOTFOUND;
    }
    // 1ED4
    if (((s32)handler >> 31) == 0 && pspK1IsUserMode()) { // 200C
        sceKernelCpuResumeIntr(oldIntr);
        pspSetK1(oldK1);
        return SCE_ERROR_KERNEL_INVALID_INTERRUPT;
    } else {
        if (intInfo.intrStack == 0)
            return SCE_ERROR_KERNEL_INVALID_INTERRUPT;
        if (((intr->v48 >> 11) & 1) == 0) {
            sceKernelCpuResumeIntr(oldIntr);
            pspSetK1(oldK1);
            return SCE_ERROR_KERNEL_INVALID_INTERRUPT;
        }
    }
    // 1F00
    if (subIntrNum < 0 || subIntrNum >= (intr->v48 & 0xFF)) {
        sceKernelCpuResumeIntr(oldIntr);
        pspSetK1(oldK1);
        return SCE_ERROR_KERNEL_INVALID_INTERRUPT;
    }
    SubInterrupt *subIntr = &intr->subIntrs[subIntrNum];
    if (subIntr->handler != 0) {
        sceKernelCpuResumeIntr(oldIntr);
        pspSetK1(oldK1);
        return SCE_ERROR_KERNEL_HANDLER_ALREADY_EXISTS;
    }
    // 1F3C
    if (((s32)handler >> 31) == 0 && (((subIntr->v48 & 0xFF) >> 10) & 1) == 0) {
        sceKernelCpuResumeIntr(oldIntr);
        pspSetK1(oldK1);
        return SCE_ERROR_KERNEL_INVALID_INTERRUPT;
    }
    // 1F60
    if (intr->cb != NULL && intr->cb->cbRegBefore != NULL) {
        // 1FEC
        s32 ret = intr->cb->cbRegBefore(intrNum, subIntrNum, handler, arg);
        if (ret != 0) {
            sceKernelCpuResumeIntr(oldIntr);
            pspSetK1(oldK1);
            return ret;
        }
    }
    subIntr->handler = (s32)handler;
    // 1F78
    subIntr->gp = sceKernelGetModuleGPByAddressForKernel((u32)handler);
    *(s8*)&subIntr->v48 = 0;
    subIntr->arg = (s32)arg;
    subIntr->v48 = (subIntr->v48 & 0xFFFF7FFF) | (intr->v48 & 0x00008000) | 0x80000300;
    if (intr->cb != NULL && intr->cb->cbRegAfter != NULL) {
        // 1FD4
        s32 ret = intr->cb->cbRegAfter(intrNum, subIntrNum, handler, arg);
        if (ret != 0) {
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
s32 sceKernelReleaseSubIntrHandler(s32 intrNum, s32 subIntrNum)
{
    dbg_printf("Called %s\n", __FUNCTION__);
    if (intrNum >= 68)
        return SCE_ERROR_KERNEL_INVALID_INTERRUPT;
    s32 oldK1 = pspShiftK1();
    s32 oldIntr = sceKernelCpuSuspendIntr();
    Interrupt *intr = &intInfo.intr[intrNum];
    if (intr->handler == 0) {
        // 2108
        sceKernelCpuResumeIntr(oldIntr);
        pspSetK1(oldK1);
        return SCE_ERROR_KERNEL_HANDLER_NOTFOUND;
    }
    if (subIntrNum < 0 || subIntrNum >= (intr->v48 & 0xFF)) {
        // 20A0
        sceKernelCpuResumeIntr(oldIntr);
        pspSetK1(oldK1);
        return SCE_ERROR_KERNEL_INVALID_INTERRUPT;
    }
    // 20E0
    SubInterrupt *subIntr = &intr->subIntrs[subIntrNum];
    if (subIntr->handler == 0 || (subIntr->handler < 0 && pspK1IsUserMode())) {
        // 2108
        sceKernelCpuResumeIntr(oldIntr);
        pspSetK1(oldK1);
        return SCE_ERROR_KERNEL_HANDLER_NOTFOUND;
    }
    // 2110
    if (intr->cb != NULL && intr->cb->cbRelBefore != NULL) {
        // 2160
        s32 ret = intr->cb->cbRelBefore(intrNum, subIntrNum);
        if (ret != 0) {
            sceKernelCpuResumeIntr(oldIntr);
            pspSetK1(oldK1);
            return ret;
        }
    }
    // 2128
    s32 oldHandler = subIntr->handler;
    subIntr->handler = 0;
    if (intr->cb != NULL && intr->cb->cbRelAfter != NULL) {
        // 2150
        s32 ret = intr->cb->cbRelAfter(intrNum, subIntrNum);
        // (moved 2140)
        if (ret != 0) {
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
s32 sceKernelEnableSubIntr(s32 intrNum, s32 subIntrNum)
{
    dbg_printf("Called %s\n", __FUNCTION__);
    if (intrNum >= 68)
        return SCE_ERROR_KERNEL_INVALID_INTERRUPT;
    s32 oldK1 = pspShiftK1();
    s32 oldIntr = sceKernelCpuSuspendIntr();
    Interrupt *intr = &intInfo.intr[intrNum];
    if (subIntrNum < 0 || subIntrNum >= (intr->v48 & 0xFF)
     || (pspK1IsUserMode() && (((intr->v48 >> 11) & 1) == 0
                              || ((intr->subIntrs[subIntrNum].v48 >> 10) & 1) == 0))) {
        // 2210
        sceKernelCpuResumeIntr(oldIntr);
        pspSetK1(oldK1);
        return SCE_ERROR_KERNEL_INVALID_INTERRUPT;
    }
    // 2248
    if (intr->handler == 0 || intr->cb == NULL || intr->cb->cbEnable == NULL) {
        // 226C
        sceKernelCpuResumeIntr(oldIntr);
        pspSetK1(oldK1);
        return SCE_ERROR_KERNEL_HANDLER_NOTFOUND;
    }
    // 2274
    s32 ret = intr->cb->cbEnable(intrNum, subIntrNum);
    sceKernelCpuResumeIntr(oldIntr);
    pspSetK1(oldK1);
    return ret;
}

// 4023E1A7 (kernel) / 8A389411 (user)
s32 sceKernelDisableSubIntr(s32 intrNum, s32 subIntrNum)
{
    dbg_printf("Called %s\n", __FUNCTION__);
    if (intrNum >= 68)
        return SCE_ERROR_KERNEL_INVALID_INTERRUPT;
    s32 oldK1 = pspShiftK1();
    s32 oldIntr = sceKernelCpuSuspendIntr();
    Interrupt *intr = &intInfo.intr[intrNum];
    if (subIntrNum < 0 || subIntrNum >= (intr->v48 & 0xFF)
     || (pspK1IsUserMode() && (((intr->v48 >> 11) & 1) == 0
                              || ((intr->subIntrs[subIntrNum].v48 >> 10) & 1) == 0))) {
        // 231C
        sceKernelCpuResumeIntr(oldIntr);
        pspSetK1(oldK1);
        return SCE_ERROR_KERNEL_INVALID_INTERRUPT;
    }
    // 2354
    if (intr->handler == 0 || intr->cb == NULL || intr->cb->cbDisable == NULL) {
        // 2378
        sceKernelCpuResumeIntr(oldIntr);
        pspSetK1(oldK1);
        return SCE_ERROR_KERNEL_HANDLER_NOTFOUND;
    }
    s32 ret = intr->cb->cbDisable(intrNum, subIntrNum);
    sceKernelCpuResumeIntr(oldIntr);
    pspSetK1(oldK1);
    return ret;
}

// C495F536 (kernel) / 5CB5A78B (user)
s32 sceKernelSuspendSubIntr(s32 intrNum, s32 subIntrNum, s32 *arg2)
{
    dbg_printf("Called %s\n", __FUNCTION__);
    if (intrNum >= 68)
        return SCE_ERROR_KERNEL_INVALID_INTERRUPT;
    s32 oldK1 = pspShiftK1();
    if (!pspK1PtrOk(arg2)) {
        // 24C4
        pspSetK1(oldK1);
        return SCE_ERROR_KERNEL_ILLEGAL_ADDR;
    }
    s32 oldIntr = sceKernelCpuSuspendIntr();
    Interrupt *intr = &intInfo.intr[intrNum];
    if (subIntrNum < 0 || subIntrNum >= (intr->v48 & 0xFF)
     || (pspK1IsUserMode() && (((intr->v48 >> 11) & 1) == 0
                              || ((intr->subIntrs[subIntrNum].v48 >> 10) & 1) == 0))) {
        // 2440
        sceKernelCpuResumeIntr(oldIntr);
        pspSetK1(oldK1);
        return SCE_ERROR_KERNEL_INVALID_INTERRUPT;
    }
    // 247C
    if (intr->handler == 0 || intr->cb == NULL || intr->cb->cbSuspend == NULL) {
        // 24A0
        if (arg2 != NULL)
            *arg2 = 0;
        sceKernelCpuResumeIntr(oldIntr);
        pspSetK1(oldK1);
        return SCE_ERROR_KERNEL_HANDLER_NOTFOUND;
    }
    
    s32 ret = intr->cb->cbSuspend(intrNum, subIntrNum, arg2);
    sceKernelCpuResumeIntr(oldIntr);
    pspSetK1(oldK1);
    return ret;
}

// 2980AE03 (kernel) / 7860E0DC (user)
s32 sceKernelResumeSubIntr(s32 intrNum, s32 subIntrNum, s32 arg2)
{
    dbg_printf("Called %s\n", __FUNCTION__);
    if (intrNum >= 68)
        return SCE_ERROR_KERNEL_INVALID_INTERRUPT;
    s32 oldK1 = pspShiftK1();
    s32 oldIntr = sceKernelCpuSuspendIntr();
    Interrupt *intr = &intInfo.intr[intrNum];
    if (subIntrNum < 0 || subIntrNum >= (intr->v48 & 0xFF)
     || (pspK1IsUserMode() && (((intr->v48 >> 11) & 1) == 0
                              || ((intr->subIntrs[subIntrNum].v48 >> 10) & 1) == 0))) {
        // 2574
        sceKernelCpuResumeIntr(oldIntr);
        pspSetK1(oldK1);
        return SCE_ERROR_KERNEL_INVALID_INTERRUPT;
    }
    // 25B0
    if (intr->handler == 0 || intr->cb == NULL || intr->cb->cbResume == NULL) {
        // 25D4
        sceKernelCpuResumeIntr(oldIntr);
        pspSetK1(oldK1);
        return SCE_ERROR_KERNEL_HANDLER_NOTFOUND;
    }
    s32 ret = intr->cb->cbResume(intrNum, subIntrNum, arg2);
    sceKernelCpuResumeIntr(oldIntr);
    pspSetK1(oldK1);
    return ret;
}

// 4351DD4E (kernel) / FC4374B8 (user)
s32 sceKernelIsSubInterruptOccured(s32 intrNum, s32 subIntrNum)
{
    dbg_printf("Called %s\n", __FUNCTION__);
    if (intrNum >= 68)
        return SCE_ERROR_KERNEL_INVALID_INTERRUPT;
    s32 oldK1 = pspShiftK1();
    s32 oldIntr = sceKernelCpuSuspendIntr();
    Interrupt *intr = &intInfo.intr[intrNum];
    if (subIntrNum < 0 || subIntrNum >= (intr->v48 & 0xFF)
     || (pspK1IsUserMode() && (((intr->v48 >> 11) & 1) == 0
                              || ((intr->subIntrs[subIntrNum].v48 >> 10) & 1) == 0))) {
        // 2688
        sceKernelCpuResumeIntr(oldIntr);
        pspSetK1(oldK1);
        return SCE_ERROR_KERNEL_INVALID_INTERRUPT;
    }
    // 26C0
    if (intr->handler == 0 || intr->cb == NULL || intr->cb->cbIsOccured == NULL) {
        // 26E4
        sceKernelCpuResumeIntr(oldIntr);
        pspSetK1(oldK1);
        return SCE_ERROR_KERNEL_HANDLER_NOTFOUND;
    }
    // 26EC
    s32 ret = intr->cb->cbIsOccured(intrNum, subIntrNum);
    sceKernelCpuResumeIntr(oldIntr);
    pspSetK1(oldK1);
    return ret;
}

// 68B5CA51
s32 sceKernelQueryIntrHandlerInfo(s32 intrNum, s32 subIntrNum, s32 out)
{
    dbg_printf("Called %s\n", __FUNCTION__);
    if (intrNum >= 68)
        return SCE_ERROR_KERNEL_INVALID_INTERRUPT;
    if (*(s32*)(out + 0) != 56)
        return SCE_ERROR_KERNEL_INVALID_INTERRUPT_ARGUMENT;
    s32 oldIntr = sceKernelCpuSuspendIntr();
    Interrupt *intr = &intInfo.intr[intrNum];
    s8 numSubIntr = intr->v48 & 0xFF;
    if (numSubIntr != 0) {
        if (subIntrNum < 0 || subIntrNum >= numSubIntr) {
            // (2918)
            // 291C
            if (subIntrNum != -1) {
                sceKernelCpuResumeIntr(oldIntr);
                return SCE_ERROR_KERNEL_INVALID_INTERRUPT;
            }
        } else {
            if (intr->subIntrs == NULL)
                subIntrNum = 0;
            else
                intr = (Interrupt*)&intr->subIntrs[subIntrNum];
        }
    }
    // (2794)
    // 2798
    *(void**)(out + 8) = intr->arg;
    *(s32*)(out + 4) = intr->handler;
    *(s32*)(out + 12) = intr->gp;
    *(s16*)(out + 16) = intrNum;
    *(s16*)(out + 20) = (intr->v48 >> 8) & 3;
    if (intrNum >= 64) {
        // 28D0
        s32 st = pspCop0StateGet(COP0_STATE_STATUS) & 0xFF00;;
        s32 mask;
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
        *(s16*)(out + 22) = (st & mask) != 0;
    } else {
        s8 bit = (intInfo.grpsOpt[(intrNum >> 5) + 24] >> (intrNum & 0x1F)) & 1;
        *(s16*)(out + 22) = bit;
        if ((bit & (~subIntrNum >> 31)) != 0) {
            if (intr->handler != 0) {
                // 287C
                if (intr->cb != NULL && intr->cb->cbSuspend != NULL && intr->cb->cbResume != NULL) {
                    s32 sp;
                    intr->cb->cbSuspend(intrNum, subIntrNum, &sp);
                    intr->cb->cbResume(intrNum, subIntrNum, sp);
                    *(s16*)(out + 22) = (sp != 0);
                }
            }
            else
                *(s16*)(out + 22) = 0;
        }
    }
    // 280C
    *(s32*)(out + 32) = intr->u32;
    *(s32*)(out + 36) = intr->u36;
    *(s16*)(out + 18) = intr->v48 & 0xFF;
    *(s32*)(out + 40) = intr->u16;
    *(s32*)(out + 44) = intr->u20;
    *(s32*)(out + 24) = intr->u12;
    *(s32*)(out + 48) = intr->u24;
    *(s32*)(out + 52) = intr->u28;
    sceKernelCpuResumeIntr(oldIntr);
    return 0;
}

// 2938
void *mymemset(void *dstVoid, s8 c, s32 n)
{
    dbg_printf("Called %s\n", __FUNCTION__);
    s8 *dst = (s8*)dstVoid;
    if (dst != NULL) {
        // 2950
        s8 *actDst = dst;
        while ((n--) > 0)
            *(actDst++) = c;
    }
    return dst;
}

// 2968
void InterruptDisableInTable(s32 intrNum)
{
    dbg_printf("Called %s\n", __FUNCTION__);
    s32 mask = ~(1 << (intrNum & 0x1F));
    s32 *ptr = &intInfo.grpsOpt[intrNum >> 5];
    s32 i = 4;
    // 2990
    while ((i--) >= 0) {
        *ptr &= mask;
        ptr += 2;
    }
}

void sub_29B0(s32 intrNum)
{
    dbg_printf("Called %s\n", __FUNCTION__);
    s32 val = (intInfo.intr[intrNum].v48 >> 8) & 3;
    s32 mask = 1 << (intrNum & 0x1F);
    s32 *ptr = &intInfo.grpsOpt[(intrNum >> 5) + (val << 1)];
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
    dbg_printf(" sub_29B0 ok\n");
}

// 2A40
void AllLevelInterruptDisable(s32 intrNum)
{
    dbg_printf("Called %s\n", __FUNCTION__);
    s32 *ptr = &intInfo.grpsOpt[intrNum >> 5];
    s32 mask = ~(1 << (intrNum & 0x1F));
    s32 i = 11;
    // 2A78
    while ((--i) >= 0) {
        *ptr &= mask;
        ptr += 2;
    }
    s32 sp[2];
    sub_1080(sp);
    sp[intrNum >> 5] &= mask;
    sub_10A8(sp);
}

// 55D18836
s32 sceKernelSetPrimarySyscallHandler(s32 syscallId, void (*syscall)())
{
    dbg_printf("Called %s\n", __FUNCTION__);
    s32 oldIntr = sceKernelCpuSuspendIntr();
    if (syscallId <= 0 || g_syscallTable.tableSize + g_syscallTable.funcTableSize < syscallId * 4) {
        sceKernelCpuResumeIntr(oldIntr);
        return SCE_ERROR_KERNEL_INVALID_SYSCALL_ID;
    }
    if (g_syscallTable.syscalls[syscallId] != g_syscallTable.syscalls[0]) {
        sceKernelCpuResumeIntr(oldIntr);
        return SCE_ERROR_KERNEL_SYSCALL_HANDLER_ALREADY_EXISTS;
    }
    // 2B68
    if ((s32)syscall >= 0) {
        // 2B8C
        sceKernelCpuResumeIntr(oldIntr);
        return SCE_ERROR_KERNEL_ILLEGAL_ADDR;
    }
    g_syscallTable.syscalls[syscallId] = syscall;
    pspCache(0x1A, &g_syscallTable.syscalls[syscallId]);
    sceKernelCpuResumeIntr(oldIntr);
    return 0;
}

s32 IntrManTerminate()
{
    dbg_printf("Called %s\n", __FUNCTION__);
    pspCop0StateSet(COP0_STATE_STATUS, pspCop0StateGet(COP0_STATE_STATUS) & 0xFFFF7BFF);
    sceKernelReleaseIntrHandler(67);
    sceKernelReleaseExceptionHandler(EXCEP_INT, (void*)intrExcepHandler);
    sceKernelReleaseExceptionHandler(EXCEP_INT, (void*)intrExcepHandler2);
    sceKernelReleaseExceptionHandler(EXCEP_SYS, (void*)syscallExcepHandler);
    return 0;
}

// 02314986
void sceKernelCpuEnableIntr()
{
    dbg_printf("Called %s\n", __FUNCTION__);
    asm("mtic %0, $0" : : "r" (1));
}

s32 InterruptManagerForKernel_6FCBA912(s32 set)
{
    dbg_printf("Called %s\n", __FUNCTION__);
    intInfo.opt = (set == 0);
    return 0;
}

// A4C1C627
s32 sceKernelClearIntrLogging(s32 intrNum)
{
    dbg_printf("Called %s\n", __FUNCTION__);
    if (intrNum >= 68)
        return SCE_ERROR_KERNEL_INVALID_INTERRUPT;
    s32 oldIntr = sceKernelCpuSuspendIntr();
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
s32 sceKernelIsInterruptOccurred(s32 intrNum)
{
    dbg_printf("Called %s\n", __FUNCTION__);
    s32 sp[2];
    if (intrNum >= 64)
        return SCE_ERROR_KERNEL_INVALID_INTERRUPT;
    sub_1050(sp);
    return (sp[intrNum >> 5] >> (intrNum & 0x1F)) & 1;
}

// D774BA45
s32 sceKernelDisableIntr(s32 intrNum)
{
    dbg_printf("Called %s\n", __FUNCTION__);
    return sceKernelSuspendIntr(intrNum, 0);
}

// DBD52A5D
void RegisterSubIntrruptMonitor(MonitorCb before, MonitorCb after)
{
    dbg_printf("Called %s\n", __FUNCTION__);
    intInfo.monitorCbAfter = after;
    intInfo.monitorCbBefore = before;
}

// 19596CD3
void ReleaseSubIntrruptMonitor()
{
    dbg_printf("Called %s\n", __FUNCTION__);
    intInfo.monitorCbAfter = 0;
    intInfo.monitorCbBefore = 0;
}

// 8357E7FA
s32 UnSupportIntr(s32 intrNum)
{
    dbg_printf("Called %s\n", __FUNCTION__);
    if (intrNum >= 64)
        return SCE_ERROR_KERNEL_INVALID_INTERRUPT;
    s32 oldIntr = sceKernelCpuSuspendIntr();
    sceKernelSuspendIntr(intrNum, 0);
    Interrupt *intr = &intInfo.intr[intrNum];
    intr->handler = 1;
    intr->v48 &= 0xFFFFFCFF;
    sceKernelCpuResumeIntr(oldIntr);
    return 0;
}

s32 InterruptManagerForKernel_8DFBD787()
{
    dbg_printf("Called %s\n", __FUNCTION__);
    if (intInfo.opt2 != 0) {
        s32 addr = pspCop0CtrlGet(COP0_CTRL_TCB);
        if (addr != 0)
            return *(s32*)addr;
    }
    return 0;
}

// D2E8363F
s32 QueryIntrHandlerInfoForUser()
{
    dbg_printf("Called %s\n", __FUNCTION__);
    return SCE_ERROR_KERNEL_ERROR;
}

// EEE43F47
s32 sceKernelRegisterUserSpaceIntrStack(s32 addr, s32 size, s32 arg2)
{
    dbg_printf("Called %s\n", __FUNCTION__);
    if (size != 0x2000)
        return SCE_ERROR_INVALID_SIZE;
    s32 oldIntr = sceKernelCpuSuspendIntr();
    if (intInfo.intrStack != 0) {
        sceKernelCpuResumeIntr(oldIntr);
        return SCE_ERROR_KERNEL_STACK_ALREADY_SET;
    }
    if (addr < 0 || arg2 < 0) {
        sceKernelCpuResumeIntr(oldIntr);
        return SCE_ERROR_KERNEL_INVALID_STACK_ADDRESS;
    }
    intInfo.intrStackArg = arg2;
    intInfo.intrStack = addr + 0x2000;
    // 2EF8
    sceKernelCpuResumeIntr(oldIntr);
    return 0;
}

// 30C08374
s32 sceKernelGetCpuClockCounter()
{
    dbg_printf("Called %s\n", __FUNCTION__);
    return pspCop0StateGet(COP0_STATE_COUNT);
}

// F9E06DF1
u64 sceKernelGetCpuClockCounterWide()
{
    dbg_printf("Called %s\n", __FUNCTION__);
    s32 oldIntr = sceKernelCpuSuspendIntr();
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
s32 *QueryInterruptManCB()
{
    dbg_printf("Called %s\n", __FUNCTION__);
    return intInfo.grpsOpt;
}

// E6FB16E3
u32 _sceKernelGetCpuClockCounterLow()
{
    dbg_printf("Called %s\n", __FUNCTION__);
    return intInfo.clockCounterLo;
}

// 14D4C61A
s32 sceKernelRegisterSystemCallTable(SceSyscallTable *newMap)
{
    dbg_printf("Called %s\n", __FUNCTION__);
    if (newMap->next != NULL)
        return SCE_ERROR_KERNEL_SYSCALLTABLE_ALREADY_REGISTERED;
    if (newMap->tableSize < 0 || newMap->funcTableSize - 16 < newMap->tableSize)
        return SCE_ERROR_KERNEL_INVALID_SYSCALLTABLE;
    // 2F7C
    s32 oldIntr = sceKernelCpuSuspendIntr();
    SceSyscallTable *cur = g_syscallTable.next;
    SceSyscallTable *prev = &g_syscallTable;
    // 2FA0
    while (cur->seed != 0) {
        prev = cur;
        cur = cur->next;
    }
    // 2FB0
    newMap->next = cur;
    prev->next = newMap;
    pspCop0CtrlSet(COP0_CTRL_SC_TABLE, (s32)g_syscallTable.next);
    sceKernelCpuResumeIntr(oldIntr);
    return 0;
}

// F153B371
s32 sceKernelQuerySystemCall(void (*sysc)())
{   
    SceSyscallTable *cur = &g_syscallTable;
    // 2FE4
    do {
        s32 count = (cur->funcTableSize < 16 ? cur->funcTableSize - 13 : cur->funcTableSize - 16) / 4;
        // 3008
        s32 i;
        for (i = 0; i < count; i++) {
            void (*curSys)(void) = (void*)((s32)cur->syscalls[i] | 0x80000000);
            if (curSys == sysc) {
                cur->syscalls[i] = curSys;
                // 303C
                return (cur->seed < 0 ? cur->seed + 3 : cur->seed) / 4 + i;
            }
        }
        // 3028
        cur = cur->next;
    } while (cur != NULL);
    return -1;
}

void InterruptManagerForKernel_E526B767(s32 arg)
{
    dbg_printf("Called %s\n", __FUNCTION__);
    if (arg == 0)
        mod_0E48 = 0x42000018; // eret
    // 30F0
    if (arg == 1) {
        // 3104
        mod_0E50 = 0x00005821; // t3 = 0
    }
}

// 3110
s32 SuspendIntc(s32 unk __attribute__((unused)), void *param __attribute__((unused)))
{
    dbg_printf("Called %s\n", __FUNCTION__);
    s32 sp[2];
    intInfo.compare = pspCop0StateGet(COP0_STATE_COMPARE);
    intInfo.count = pspCop0StateGet(COP0_STATE_COUNT);
    sub_1080(intInfo.intcState);
    mymemset(sp, 0, 8);
    sub_10A8(sp);
    return 0;
}

// 3160
s32 ResumeIntc(s32 unk __attribute__((unused)), void *param __attribute__((unused)))
{
    dbg_printf("Called %s\n", __FUNCTION__);
    sub_10A8(intInfo.intcState);
    pspCop0StateSet(COP0_STATE_COMPARE, intInfo.compare);
    pspCop0StateSet(COP0_STATE_COUNT, intInfo.count);
    return 0;
}

