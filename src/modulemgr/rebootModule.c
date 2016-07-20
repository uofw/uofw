/* Copyright (C) 2011 - 2015 The uOFW team
   See the file COPYING for copying permission.
*/

#include <common_imp.h>
#include <loadcore.h>
#include <modulemgr_kernel.h>
#include <modulemgr_options.h>
#include <sysmem_kdebug.h>
#include <threadman_kernel.h>

#include "loadModuleChecks_inline.h"
#include "modulemgr_int.h"

// Subroutine ModuleMgrForKernel_CC873DFA - Address 0x000046E4
s32 sceKernelRebootBeforeForUser(void *arg)
{
    s32 oldGp;
    u32 modCount;
    s32 status;
    SceUID uidBlkId;
    SceUID *pUidList;
    char threadArgs[16];

    oldGp = pspGetGp();
    sceKernelLockMutex(g_ModuleManager.semaId, 1, NULL); //0x00004724

    memcpy(threadArgs, arg, sizeof threadArgs); //0x00004734
    ((u32 *)threadArgs)[0] = sizeof threadArgs;

    uidBlkId = sceKernelGetModuleListWithAlloc(&modCount); //0x00004744
    if (uidBlkId < SCE_ERROR_OK)
        return uidBlkId;

    pUidList = sceKernelGetBlockHeadAddr(uidBlkId); //0x00004754

    s32 i;
    SceModule *pMod;
    s32 threadMode;
    for (i = modCount - 1; i >= 0; i--) { //0x00004760
        pMod = sceKernelFindModuleByUID(pUidList[i]); //0x00004774
        if (pMod == NULL || pMod->moduleRebootBefore == SCE_KERNEL_PTR_UNITIALIZED) //0x0000477C
            continue;

        if (GET_MCB_STATUS(pMod->status) != MCB_STATUS_STARTED || !(pMod->status & SCE_MODULE_USER_MODULE)) //0x0000479C - 0x00004830
            continue;

        s32 priority = pMod->moduleRebootBeforeThreadPriority;
        if (priority == SCE_KERNEL_VALUE_UNITIALIZED)
            priority = SCE_KERNEL_MODULE_INIT_PRIORITY; //0x00004864

        s32 stackSize = pMod->moduleRebootBeforeThreadStacksize;
        if (stackSize == SCE_KERNEL_VALUE_UNITIALIZED) //0x00004870
            stackSize = SCE_KERNEL_TH_KERNEL_DEFAULT_STACKSIZE;

        s32 attr = pMod->moduleRebootBeforeThreadAttr;
        if (attr == SCE_KERNEL_VALUE_UNITIALIZED) //0x00004874
            attr = SCE_KERNEL_TH_DEFAULT_ATTR;

        switch (pMod->attribute & SCE_MODULE_PRIVILEGE_LEVELS) {
        case SCE_MODULE_VSH:
            threadMode = SCE_KERNEL_TH_VSH_MODE;
            break;
        case SCE_MODULE_APP: //0x00004884
            threadMode = SCE_KERNEL_TH_APP_MODE;
            break;
        case SCE_MODULE_USB_WLAN: //0x00004890
            threadMode = SCE_KERNEL_TH_USB_WLAN_MODE;
            break;
        case SCE_MODULE_MS: //0x0000489C
            threadMode = SCE_KERNEL_TH_MS_MODE;
            break;
        default: //0x000048A4
            threadMode = SCE_KERNEL_TH_USER_MODE;
            break;
        }

        SceKernelThreadOptParam threadParams;
        threadParams.size = sizeof threadParams; //0x000048AC
        threadParams.stackMpid = pMod->mpIdData; //0x000048CC

        status = _CheckUserModulePartition(threadParams.stackMpid); // 0x000048BC - 0x000048E0
        if (status < SCE_ERROR_OK)
            threadParams.stackMpid = SCE_KERNEL_PRIMARY_USER_PARTITION;

        pspSetGp(pMod->gpValue); //0x00004900

        pMod->userModThid = sceKernelCreateThread("SceModmgrRebootBefore", (SceKernelThreadEntry)pMod->moduleRebootBefore, priority,
            stackSize, threadMode | attr, &threadParams); //0x0000491C

        pspSetGp(oldGp);

        // TODO: Add proper structure for threadArgs
        status = sceKernelStartThread(pMod->userModThid, sizeof threadArgs, threadArgs); //0x00004934
        if (status == SCE_ERROR_OK)
            sceKernelWaitThreadEnd(pMod->userModThid, NULL); //0x000049AC

        sceKernelDeleteThread(pMod->userModThid); //0x00004944
        pMod->userModThid = SCE_KERNEL_VALUE_UNITIALIZED;

        if (!sceKernelIsToolMode()) //0x00004954
            continue;

        status = sceKernelDipsw(25); //0x0000495C
        if (status == 1) //0x00004968
            continue;

        s32 sdkVersion = sceKernelGetCompiledSdkVersion(); //0x00004970
        if (sdkVersion < 0x03030000) //0x00004984
            continue;

        s32 checkSum = sceKernelSegmentChecksum(pMod);
        if (checkSum == (s32)pMod->segmentChecksum)
            continue;

        pspBreak(SCE_BREAKCODE_ZERO);
        continue;
    }

    status = ClearFreePartitionMemory(uidBlkId); // 0x000047BC - 0x000047F0
    // TODO: UOFW: Missing sceKernelUnlockMutex()?
    return status;
}

// Subroutine ModuleMgrForKernel_9B7102E2 - Address 0x000049BC
s32 sceKernelRebootPhaseForKernel(s32 arg1, void *argp, s32 arg3, s32 arg4)
{
    SceUID uidBlkId;
    SceUID *pUidList;
    SceModule *pMod;
    u32 modCount;
    s32 status;

    uidBlkId = sceKernelGetModuleListWithAlloc(&modCount); //0x000049F8
    if (uidBlkId < SCE_ERROR_OK)
        return uidBlkId;

    pUidList = sceKernelGetBlockHeadAddr(uidBlkId); //0x00004A08

    s32 i;
    for (i = modCount - 1; i >= 0; i--) { //0x00004A14 - 0x00004A64
        pMod = sceKernelFindModuleByUID(pUidList[i]); //0x00004A34
        if (pMod == NULL || pMod->moduleRebootPhase == SCE_KERNEL_PTR_UNITIALIZED) //0x00004A3C, 0x00004A48
            continue;

        if (GET_MCB_STATUS(pMod->status) != MCB_STATUS_STARTED || (pMod->status & SCE_MODULE_USER_MODULE)) //0x00004A58 - 0x00004B04
            continue;

        pMod->moduleRebootPhase(arg1, (u32)argp, arg3, arg4); //0x00004B0C

        if (!sceKernelIsToolMode()) //0x00004B1C
            continue;

        status = sceKernelDipsw(25); //0x00004B24
        if (status == 1) //0x00004B30
            continue;

        s32 sdkVersion = sceKernelGetCompiledSdkVersion(); //0x00004B38
        if (sdkVersion < 0x03030000) //0x00004B44
            continue;

        s32 checkSum = sceKernelSegmentChecksum(pMod); //0x00004B4C
        if (checkSum == (s32)pMod->segmentChecksum) //0x00004B58
            continue;

        pspBreak(SCE_BREAKCODE_ZERO);
        continue;
    }

    status = ClearFreePartitionMemory(uidBlkId); // 0x00004A74 - 0x00004AB4
    return ((status > SCE_ERROR_OK) ? SCE_ERROR_OK : status);
}

// ModuleMgrForKernel_5FC3B3DA - Address 0x00004B6C
s32 sceKernelRebootBeforeForKernel(void *argp, s32 arg2, s32 arg3, s32 arg4)
{
    SceUID uidBlkId;
    SceUID *pUidList;
    SceModule *pMod;
    u32 modCount;
    s32 status;

    uidBlkId = sceKernelGetModuleListWithAlloc(&modCount); //0x00004BA8
    if (uidBlkId < SCE_ERROR_OK)
        return uidBlkId;

    pUidList = sceKernelGetBlockHeadAddr(uidBlkId); //0x00004BB8

    s32 i;
    for (i = modCount - 1; i >= 0; i--) { //0x00004BC4 - 0x00004C10
        pMod = sceKernelFindModuleByUID(pUidList[i]); //0x00004BE4
        if (pMod == NULL || pMod->moduleRebootBefore == SCE_KERNEL_PTR_UNITIALIZED) //0x00004BEC, 0x00004BF8
            continue;

        if (GET_MCB_STATUS(pMod->status) != MCB_STATUS_STARTED || (pMod->status & SCE_MODULE_USER_MODULE)) //0x00004C08,  0x00004CB0
            continue;

        pMod->moduleRebootBefore((u32)argp, arg2, arg3, arg4); //0x00004CB8
    }

    status = ClearFreePartitionMemory(uidBlkId); // 0x00004C24 - 0x00004C60
    return ((status > SCE_ERROR_OK) ? SCE_ERROR_OK : status);
}
