/* Copyright (C) 2011 - 2015 The uOFW team
   See the file COPYING for copying permission.
*/

#include <common_imp.h>
#include <modulemgr_kernel.h>
#include <modulemgr_options.h>

#include "loadModuleChecks_inline.h"
#include "modulemgr_int.h"

static s32 _SelfStopUnloadModule(s32 returnStatus, const void *codeAddr, SceSize args, void *argp, s32 *pStatus,
    const SceKernelSMOption *pOption);
static s32 _StopUnloadSelfModuleWithStatus(s32 returnStatus, void *addr, SceSize args, void *argp, s32 *pStatus,
    const SceKernelSMOption *pOption);

// Subroutine ModuleMgrForUser_50F0C1EC - Address 0x00003D98 - Aliases: ModuleMgrForKernel_3FF74DF1
s32 sceKernelStartModule(SceUID modId, SceSize args, const void *argp, s32 *pModResult,
    const SceKernelSMOption *pOption)
{
    s32 oldK1;
    s32 status;
    SceModuleMgrParam modParams;

    dbg_fbfill(1, 255, 1);

    oldK1 = pspShiftK1();

    if (sceKernelIsIntrContext()) { //0x00003DA4
        pspSetK1(oldK1);
        return SCE_ERROR_KERNEL_CANNOT_BE_CALLED_FROM_INTERRUPT;
    }

    if (argp != NULL && !pspK1DynBufOk(argp, args)) { //0x00003DE4, 0x00003DFC
        pspSetK1(oldK1);
        return SCE_ERROR_KERNEL_ILLEGAL_ADDR;
    }

    if (pModResult != NULL && !pspK1PtrOk(pModResult)) { //0x00003E18, 0x00003E10
        pspSetK1(oldK1);
        return SCE_ERROR_KERNEL_ILLEGAL_ADDR;
    }

    status = _checkSMOptionConditions(pOption); //0x00003FB8 - 0x00003EAC
    if (status < 0) {
        pspSetK1(oldK1);
        return status;
    }

    pspClearMemory32(&modParams, sizeof modParams); //0x00003EBC

    // 0x00003EC8
    modParams.modeFinish = CMD_START_MODULE;
    modParams.modeStart = CMD_START_MODULE;
    modParams.modId = modId; // 0x00003ECC
    modParams.argSize = args;
    modParams.argp = (void *)argp;
    modParams.pStatus = pModResult;

    if (pOption != NULL) { //0x00003EDC
        modParams.threadMpIdStack = pOption->mpIdStack;
        modParams.stackSize = pOption->stackSize;
        modParams.threadPriority = pOption->priority;
        modParams.threadAttr = pOption->attribute;
    } else { //0x00003F14
        modParams.threadMpIdStack = SCE_KERNEL_UNKNOWN_PARTITION;
        modParams.stackSize = 0;
        modParams.threadPriority = SCE_KERNEL_INVALID_PRIORITY;
        modParams.threadAttr = SCE_KERNEL_TH_DEFAULT_ATTR;
    }
    status = _start_exe_thread(&modParams); //0x00003F04

    //UOFW: It doesn't return to here....
    //dbg_fbfill(1, 1, 255);

    pspSetK1(oldK1);
    return status;
}

// Subroutine ModuleMgrForUser_D1FF982A - Address 0x00003F28 - Aliases: ModuleMgrForKernel_E5D6087B
s32 sceKernelStopModule(SceUID modId, SceSize args, const void *argp, s32 *pModResult, const SceKernelSMOption *pOption)
{
    s32 oldK1;
    s32 status;
    void *callerAddr;
    SceModule *pMod;
    SceModuleMgrParam modParams;

    oldK1 = pspShiftK1();
    callerAddr = (void *)pspGetRa();

    if (sceKernelIsIntrContext()) { //0x00003F68
        pspSetK1(oldK1);
        return SCE_ERROR_KERNEL_CANNOT_BE_CALLED_FROM_INTERRUPT;
    }

    if (argp != NULL && !pspK1DynBufOk(argp, args)) { //0x00003F7C, 0x00003F94
        pspSetK1(oldK1);
        return SCE_ERROR_KERNEL_ILLEGAL_ADDR;
    }

    if (pModResult != NULL && !pspK1StaBufOk(pModResult, sizeof *pModResult)) { //0x00003F9C, 0x00003FB0
        pspSetK1(oldK1);
        return SCE_ERROR_KERNEL_ILLEGAL_ADDR;
    }

    status = _checkSMOptionConditions(pOption); //0x00003FB8 - 0x0000404C
    if (status < 0) {
        pspSetK1(oldK1);
        return status;
    }

    // TODO: Why do we need to check if the module which calls this API is still loaded?
    pMod = sceKernelFindModuleByAddress((u32)callerAddr); //0x00004054
    if (pMod == NULL) {
        pspSetK1(oldK1);
        return SCE_ERROR_KERNEL_MODULE_CANNOT_STOP;
    }

    pspClearMemory32(&modParams, sizeof modParams); //0x00004078

    modParams.pMod = sceKernelGetModuleFromUID(modId); //0x00004080
    if (modParams.pMod && modParams.pMod->attribute & SCE_MODULE_ATTR_CANT_STOP) {
        pspSetK1(oldK1);
        return SCE_ERROR_KERNEL_MODULE_CANNOT_STOP;
    }

    // 0x000040A4
    modParams.modeFinish = CMD_STOP_MODULE;
    modParams.modeStart = CMD_STOP_MODULE;
    modParams.modId = modId; // 0x000040AC
    modParams.callerModId = pMod->modId; // 0x000040BC
    modParams.argSize = args;
    modParams.argp = (void *)argp;
    modParams.pStatus = pModResult;

    if (pOption != NULL) { //0x000040C4
        modParams.threadMpIdStack = pOption->mpIdStack;
        modParams.stackSize = pOption->stackSize;
        modParams.threadPriority = pOption->priority;
        modParams.threadAttr = pOption->attribute;
    } else { //0x000040FC
        modParams.threadMpIdStack = SCE_KERNEL_UNKNOWN_PARTITION;
        modParams.stackSize = 0;
        modParams.threadPriority = SCE_KERNEL_INVALID_PRIORITY;
        modParams.threadAttr = SCE_KERNEL_TH_DEFAULT_ATTR;
    }
    status = _start_exe_thread(&modParams); //0x000040EC

    pspSetK1(oldK1);
    return status;
}

// Subroutine ModuleMgrForUser_D675EBB8 - Address 0x00004110 - Aliases: ModuleMgrForKernel_5805C1CA
s32 sceKernelSelfStopUnloadModule(s32 exitStatus, SceSize args, void *argp)
{
    s32 oldK1;
    void *callerAddr;
    s32 status;

    oldK1 = pspShiftK1();
    callerAddr = (void *)pspGetRa();

    if (sceKernelIsIntrContext()) { //0x0000414C
        pspSetK1(oldK1);
        return SCE_ERROR_KERNEL_CANNOT_BE_CALLED_FROM_INTERRUPT;
    }

    if (argp != NULL && !pspK1DynBufOk(argp, args)) { //0x00004158, 0x0000416C
        pspSetK1(oldK1);
        return SCE_ERROR_KERNEL_ILLEGAL_ADDR;
    }

    if (pspK1IsUserMode()) //0x00004174
        callerAddr = (void *)sceKernelGetSyscallRA();

    if (!pspK1PtrOk(callerAddr)) { //0x0000419C
        pspSetK1(oldK1);
        return SCE_ERROR_KERNEL_ILLEGAL_ADDR;
    }

    status = _SelfStopUnloadModule(exitStatus, callerAddr, args, argp, NULL, NULL); //0x000041A4

    pspSetK1(oldK1);
    return status;
}

// Subroutine ModuleMgrForUser_2E0911AA - Address 0x00005990 - Aliases: ModuleMgrForKernel_387E3CA9
SceUID sceKernelUnloadModule(SceUID modId)
{
    s32 oldK1;
    s32 status;
    SceModuleMgrParam modParams;

    oldK1 = pspShiftK1(); // 0x0000599C

    // Cannot be called in an interruption
    if (sceKernelIsIntrContext()) { // 0x000059A8
        pspSetK1(oldK1);
        return SCE_ERROR_KERNEL_CANNOT_BE_CALLED_FROM_INTERRUPT;
    }

    pspClearMemory32(&modParams, sizeof modParams); //0x000059C8

    modParams.modeFinish = CMD_UNLOAD_MODULE; // 0x000059D8
    modParams.modeStart = CMD_UNLOAD_MODULE; // 0x000059DC
    modParams.modId = modId; // 0x000059E0
    modParams.argSize = 0; // 0x000059E4
    modParams.argp = NULL; // 0x000059E8
    modParams.pStatus = NULL; // 0x000059F0

    status = _start_exe_thread(&modParams); // 0x000059EC

    pspSetK1(oldK1);
    return status;
}

// Subroutine ModuleMgrForUser_8F2DF740 - Address 0x00005A14 - Aliases: ModuleMgrForKernel_EE6E8F49
s32 sceKernelStopUnloadSelfModuleWithStatus(s32 exitStatus, SceSize args, void *argp, s32 *pModResult,
    const SceKernelSMOption *pOption)
{
    return _StopUnloadSelfModuleWithStatus(exitStatus, (void *)pspGetRa(), args, argp, pModResult, pOption);
}

// Subroutine ModuleMgrForUser_CC1D3699 - Address 0x00005A4C - Aliases: ModuleMgrForKernel_E97E0DB7
s32 sceKernelStopUnloadSelfModule(SceSize args, void *argp, s32 *pModResult, const SceKernelSMOption *pOption)
{
    return _StopUnloadSelfModuleWithStatus(SCE_ERROR_OK, (void *)pspGetRa(), args, argp, pModResult, pOption);
}

// Subroutine sub_000076CC - Address 0x000076CC 
static s32 _SelfStopUnloadModule(s32 returnStatus, const void *codeAddr, SceSize args, void *argp, s32 *pStatus,
    const SceKernelSMOption *pOption)
{
    SceModule *pMod;
    s32 status;
    SceModuleMgrParam modParams;
    s32 status2;

    pMod = sceKernelFindModuleByAddress((u32)codeAddr); // 0x000076FC
    if (pMod == NULL || pMod->attribute & SCE_MODULE_ATTR_CANT_STOP) // 0x0000770C & 0x00007720
        return SCE_ERROR_KERNEL_MODULE_CANNOT_STOP;

    pspClearMemory32(&modParams, sizeof modParams); // 0x00007734

    modParams.modeStart = CMD_STOP_MODULE; // 0x00007744
    modParams.modeFinish = CMD_UNLOAD_MODULE; // 0x00007748
    modParams.argp = argp; // 0x00007750
    modParams.modId = pMod->modId; // 0x00007754
    modParams.argSize = args; // 0x0000775C
    modParams.callerModId = pMod->modId; // 0x00007764

    if (pStatus == NULL) // 0x00007760
        modParams.pStatus = &status2; // 0x000077EC
    else
        modParams.pStatus = pStatus; // 0x00007768

    if (pOption == NULL) { // 0x0000776C
        modParams.threadMpIdStack = SCE_KERNEL_UNKNOWN_PARTITION;
        modParams.stackSize = 0;
        modParams.threadPriority = SCE_KERNEL_INVALID_PRIORITY;
        modParams.threadAttr = SCE_KERNEL_TH_DEFAULT_ATTR;
    } else {
        modParams.threadMpIdStack = pOption->mpIdStack; // 0x00007784
        modParams.stackSize = pOption->stackSize; // 0x00007788
        modParams.threadPriority = pOption->priority; // 0x0000778C
        modParams.threadAttr = pOption->attribute; // 0x00007790
    }

    status = _start_exe_thread(&modParams); // 0x00007794
    if (status < 0)
        return status;

    /* Terminate and delete the calling thread (belonging to the stopped & unloaded module). */
    sceKernelExitDeleteThread(returnStatus); // 0x000077A4
    return status;
}

// Subroutine sub_000077F0 - Address 0x000077F0 
static s32 _StopUnloadSelfModuleWithStatus(s32 returnStatus, void *addr, SceSize args, void *argp, s32 *pStatus,
    const SceKernelSMOption *pOption)
{
    s32 oldK1;
    s32 status;

    oldK1 = pspShiftK1();

    // Cannot be called from interrupt
    if (sceKernelIsIntrContext()) { // 0x0000783C
        pspSetK1(oldK1);
        return SCE_ERROR_KERNEL_CANNOT_BE_CALLED_FROM_INTERRUPT;
    }

    if (argp != NULL && !pspK1DynBufOk(argp, args)) { // 0x0000785C
        pspSetK1(oldK1);
        return SCE_ERROR_KERNEL_ILLEGAL_ADDR;
    }

    if (pStatus != NULL && !pspK1StaBufOk(pStatus, sizeof *pStatus)) { // 0x00007878
        pspSetK1(oldK1);
        return SCE_ERROR_KERNEL_ILLEGAL_ADDR;
    }

    status = _checkSMOptionConditions(pOption); // 0x000078E4 - 0x000078F8
    if (status < 0) {
        pspSetK1(oldK1);
        return status;
    }

    if (pspK1IsUserMode()) // 0x0000791C
        addr = (void *)sceKernelGetSyscallRA(); // 0x00007958

    if (!pspK1PtrOk(addr)) { // 0x0000792C
        pspSetK1(oldK1);
        return SCE_ERROR_KERNEL_ILLEGAL_ADDR;
    }

    status = _SelfStopUnloadModule(returnStatus, addr, args, argp, pStatus, pOption); // 0x00007948

    pspSetK1(oldK1);
    return status;
}
