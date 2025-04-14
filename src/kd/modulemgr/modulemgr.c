/* Copyright (C) 2011 - 2015 The uOFW team
   See the file COPYING for copying permission.
*/

/**
 * uofw/src/modulemgr/modulemgr.c
 *
 * Module Manager - high-level API for the Program Loader
 *
 * The Program Loader loads modules - i.e from flash memory, Memory-Sticks or UMDs - 
 * into memory, relocates modules and manages the loaded modules.
 * The loader consists of a high-level API provided by the Module Manager and a 
 * low-level API provided by Loadcore. 
 *
 * The Module Manager makes API calls to Loadcore and the I/O-Manager to provide the
 * following functions:
 *
 *      • Loading module files
 *      • Executing modules
 *      • Stopping modules
 *      • Unloading modules
 *
 * Additionally, the Module Manager is responsible for calling the moduleReboot{Before, Phase}()
 * entry functions of the registered modules during a PSP Reboot process.
 */

#include <common_imp.h>
#include <interruptman.h>
#include <iofilemgr_kernel.h>
#include <loadcore.h>
#include <modulemgr_init.h>
#include <modulemgr_kernel.h>
#include <modulemgr_nids.h>
#include <sysmem_kernel.h>
#include <sysmem_kdebug.h>
#include <sysmem_sysclib.h>
#include <sysmem_utils_kernel.h>
#include <threadman_kernel.h>

#include "modulemgr_int.h"
#include "override.h"
#include "pbp.h"

SCE_MODULE_INFO(
    "sceModuleManager",
    SCE_MODULE_KIRK_MEMLMD_LIB |
    SCE_MODULE_KERNEL |
    SCE_MODULE_ATTR_CANT_STOP | SCE_MODULE_ATTR_EXCLUSIVE_LOAD | SCE_MODULE_ATTR_EXCLUSIVE_START,
    1, 18);
SCE_MODULE_BOOTSTART("ModuleMgrInit");
SCE_MODULE_REBOOT_BEFORE("ModuleMgrRebootBefore");
SCE_MODULE_REBOOT_PHASE("ModuleMgrRebootPhase");
SCE_SDK_VERSION(SDK_VERSION);

SceModuleManagerCB g_ModuleManager; // 0x00009A20

/* local functions */
static s32 _ModuleReleaseLibraries(SceModule *pMod);

static s32 _PrologueModule(SceModuleMgrParam *pModParams, SceModule *pMod);
static s32 _StartModule(SceModuleMgrParam *pModParams, SceModule *pMod, SceSize argSize, void *argp, s32 *pStatus);
static s32 _StopModule(SceModuleMgrParam *pModParams, SceModule *pMod, s32 startOp, SceUID modId, SceSize argSize,
    void *argp, s32 *pStatus);
static s32 _LoadModule(SceModuleMgrParam *pModParams);
static s32 _RelocateModule(SceModuleMgrParam *pModParams);

static s32 _CheckSkipPbpHeader(SceModuleMgrParam *pModParams, SceUID fd, void *pBlock, u32 *pOffset, s32 apiType);
static void _FreeMemoryResources(SceUID fd, SceUID partitionId, SceModuleMgrParam *pModParams,
    SceLoadCoreExecFileInfo *pExecInfo);
static s32 _GivenBlockInfoFromBlock(SceUID blockId, SceModuleMgrParam *pModParams);
static s32 _CheckKernelOnlyModulePartition(SceUID memoryPartitionId);
static s32 _PartitionCheck(SceModuleMgrParam *pModParams, SceLoadCoreExecFileInfo *pExecInfo);

static void _CleanupMemory(SceUID gzipMemId, SceLoadCoreExecFileInfo *pExecInfo);
static s32 allocate_module_block(SceModuleMgrParam *pModParams);
static s32 _ProcessModuleExportEnt(SceModule *pMod, SceResidentLibraryEntryTable *pLib);
static s32 ModuleRegisterLibraries(SceModule *pMod);

static SceBool canOverflowOccur(u32 s1, u32 s2)
{
    return ((s1 + s2) < s2);
}

// sub_00000000
/*
 * Unregister the resident libraries of the module and unlink the stub libraries
 * it is using. 
 * If the resident libraries cannot be unregistered, the stub libraries won't be unlinked 
 * and a corresponding error is returned. 
 */
static s32 _EpilogueModule(SceModule *pMod)
{
    void *pCurEntry;
    void *pLastEntry;
    s32 status;

    pCurEntry = pMod->entTop;
    pLastEntry = pMod->entTop + pMod->entSize;
    status = SCE_ERROR_OK;

    /* Check if the resident libraries of the module can be released. */
    while (pCurEntry < pLastEntry) {
        SceResidentLibraryEntryTable *pCurTable = (SceResidentLibraryEntryTable *)pCurEntry;
        if (pCurTable->attribute & SCE_LIB_IS_SYSLIB) {
            pCurEntry += pCurTable->len * sizeof(void *);
            continue;
        }

        status = sceKernelCanReleaseLibrary(pCurTable); //0x00000048
        if (status != SCE_ERROR_OK)
            return status;

        pCurEntry += pCurTable->len * sizeof(void *);
    }

    /* Unlink the stub libraries of the module. */
    if (pMod->stubTop != SCE_KERNEL_PTR_UNITIALIZED)
        sceKernelUnlinkLibraryEntries(pMod->stubTop, pMod->stubSize); //0x00000080

    /* Release the resident libraries of the module. */
    _ModuleReleaseLibraries(pMod); //0x00000088
    return status;
}

// 0x000000B0
/* 
 * Unload the specified module. Only modules which have not been started or
 * have been stopped can be unloaded successfully.
 */
static s32 _UnloadModule(SceModule *pMod)
{
    u32 modStatus;

    modStatus = GET_MCB_STATUS(pMod->status);
    if (modStatus != MCB_STATUS_LOADED && modStatus != MCB_STATUS_RELOCATED 
            && modStatus != MCB_STATUS_STOPPED)
        return SCE_ERROR_KERNEL_MODULE_CANNOT_REMOVE;

    sceKernelMemset32((void *)pMod->textAddr, MAKE_BREAKCODE_INSTR(SCE_BREAKCODE_ONE), UPALIGN4(pMod->textSize)); // 0x00000110
    sceKernelMemset((void *)(pMod->textAddr + pMod->textSize), 0xFF, pMod->dataSize + pMod->bssSize); //0x00000130

    sceKernelIcacheInvalidateAll(); //0x00000138
    sceKernelReleaseModule(pMod); //0x00000140

    if (!(pMod->status & 0x1000)) //0x00000150
        sceKernelFreePartitionMemory(pMod->moduleBlockId); //0x00000168

    sceKernelDeleteModule(pMod); //0x00000158
    // uOFW: here, Sony forgot to reset g_ModuleManager.pModule to NULL when it's equal to pMod

    return SCE_ERROR_OK;
}

// 0x00000178
/* 
 * Perform the {load, relocate, start, stop, unload} operations for a module. 
 */
static s32 exe_thread(SceSize args, void *argp)
{
    SceModuleMgrParam *pModParams;
    SceLoadCoreExecFileInfo execInfo;
    s32 status;

    (void)args;

    status = SCE_ERROR_OK;
    pModParams = (SceModuleMgrParam *)argp;
    pspClearMemory32(&execInfo, sizeof execInfo); // 0x000001A8

    SceModule *pMod = pModParams->pMod;
    switch (pModParams->modeStart) { // 0x000001D0
    case CMD_LOAD_MODULE:
        if (pMod == NULL) {
            pMod = sceKernelCreateModule(); //0x0000048C
            pModParams->pMod = pMod;

            if (pMod == NULL)
                break; // 0x000004A0
        }
        pModParams->pExecInfo = &execInfo; //0x000001E0

        /* Load the specified module into memory. */
        status = _LoadModule(pModParams); //0x000001E4

        sceKernelChangeThreadPriority(SCE_KERNEL_THREAD_ID_SELF, SCE_KERNEL_MODULE_INIT_PRIORITY); // 0x000001F4

        if (status < SCE_ERROR_OK) { //0x000001FC
            *(pModParams->pResult) = status; //0x00000480

            if (pModParams->pMod != NULL) //0x47C
                sceKernelDeleteModule(pModParams->pMod);

            break;
        }

        /* Return the ID of the successfully loaded module. */
        *(pModParams->pResult) = pModParams->pMod->modId; //0x0000020C

        if (pModParams->modeFinish == CMD_LOAD_MODULE) //0x00000214
            break;
        /* FALLTHRU */
    case CMD_RELOCATE_MODULE: // 0x0000021C
        if (pMod == NULL) {
            pMod = sceKernelCreateModule(); //0x00000448
            pModParams->pMod = pMod;

            // 0x00000454
            if (pMod == NULL)
                break;

            SET_MCB_STATUS(pMod->status, MCB_STATUS_LOADED);
            sceKernelRegisterModule(pMod); //0x0000046C
        }
        if (pModParams->pExecInfo == NULL) {
            pspClearMemory32(&execInfo, sizeof execInfo); // 0x00000238
            pModParams->pExecInfo = &execInfo;
        }

        /* Relocate the specified module. */
        status = _RelocateModule(pModParams); //0x00000244

        if (status < SCE_ERROR_OK) {
            *(pModParams->pResult) = status; //0x0000042C

            if (pMod == NULL) //0x00000428
                break;

            //0x00000430
            sceKernelReleaseModule(pMod);
            sceKernelDeleteModule(pMod);
            break;
        }

        *(pModParams->pResult) = pModParams->pMod->modId; //0x00000260        
        if (pModParams->modeFinish == CMD_RELOCATE_MODULE) //0x00000268
            break;
        /* FALLTHRU */
    case CMD_START_MODULE: //0x00000270
        pMod = sceKernelGetModuleFromUID(pModParams->modId); //0x00000270
        if (pMod == NULL && (pMod = sceKernelFindModuleByUID(pModParams->modId)) == NULL) //0x00000400
            *(pModParams->pResult) = SCE_ERROR_KERNEL_UNKNOWN_MODULE; //0x00000420
        else {
            /* Start the specified module. */
            status = _StartModule(pModParams, pMod, pModParams->argSize, pModParams->argp, pModParams->pStatus); //0x00000290

            if (status == SCE_KERNEL_RESIDENT)
                /* Return the module ID if it is a resident one. */
                *(pModParams->pResult) = pMod->modId; //0x000003FC
            else if (status == SCE_KERNEL_NO_RESIDENT)
                *(pModParams->pResult) = SCE_ERROR_OK; //0x000002A4
            else
                /* Error: module could not be started successfully. */
                *(pModParams->pResult) = status; //0x000002B0   
        }

        if (status < SCE_ERROR_OK || pModParams->modeFinish == CMD_START_MODULE) //0x000002B4 & 0x000002C0
            break;
        /* FALLTHRU */
    case CMD_STOP_MODULE: //0x000002C8
        if (pMod == NULL) { //0x000002C8
            pMod = sceKernelGetModuleFromUID(pModParams->modId);
            if (pMod == NULL) { //0x000003D0
                *(pModParams->pResult) = SCE_ERROR_KERNEL_UNKNOWN_MODULE; //0x000003AC

                break;
            }
        }
        /* Stop the specified module. */
        status = _StopModule(pModParams, pMod, pModParams->modeStart, pModParams->callerModId, pModParams->argSize,
            pModParams->argp, pModParams->pStatus); //0x000002E8

        if (status == SCE_KERNEL_STOP_SUCCESS) //0x000002F0
            *(pModParams->pResult) = SCE_ERROR_OK;
        else if (status == SCE_KERNEL_STOP_FAIL)
            /* Error: Return the module ID if the module could not be stopped successfully. */
            *(pModParams->pResult) = pMod->modId; //0x000002FC
        else
            /* Error: an undefined status was returned. */
            *(pModParams->pResult) = status; //0x00000308

        if (status < SCE_ERROR_OK || pModParams->modeFinish == CMD_STOP_MODULE) //0x0000030C & 0x00000318
            break;
        /* FALLTHRU */
    case CMD_UNLOAD_MODULE: //0x00000320
        pMod = sceKernelGetModuleFromUID(pModParams->modId); //0x00000320
        if (pMod == NULL) { // 0x00000328
            *(pModParams->pResult) = SCE_ERROR_KERNEL_UNKNOWN_MODULE; //0x000003AC

            break;
        }
        /* Unload the specified module. */
        status = _UnloadModule(pMod); //0x00000330

        if (status < SCE_ERROR_OK) //0x00000338
            *(pModParams->pResult) = status;
        else
            /* Return the module ID if it could be unloaded successfully. */
            *(pModParams->pResult) = pMod->modId; //0x00000348

        break;
    }
    // 00000350
    /* Set thread priority to maximum to reduce waiting time for the calling module thread. */
    if (pModParams->eventId != 0) {
        sceKernelChangeThreadPriority(SCE_KERNEL_THREAD_ID_SELF, SCE_KERNEL_HIGHEST_PRIORITY_KERNEL); //0x00000374
        sceKernelSetEventFlag(pModParams->eventId, 1); //0x00000380
    }

    return SCE_ERROR_OK;
}

// 0x0000501C
s32 ModuleMgrRebootPhase(s32 arg0 __attribute__((unused)), void *arg1 __attribute__((unused)), s32 arg2 __attribute__((unused)), s32 arg3 __attribute__((unused)))
{
    return SCE_ERROR_OK;
}

// 0x00005024
s32 ModuleMgrRebootBefore(void *arg0 __attribute__((unused)), s32 arg1 __attribute__((unused)), s32 arg2 __attribute__((unused)), s32 arg3 __attribute__((unused)))
{
    return sceKernelSuspendThread(g_ModuleManager.threadId); //0x00005034
}
// 0x00005048
/*
 * Create the work objects (threads,...) for the module manager.
 */
s32 ModuleMgrInit(SceSize argSize __attribute__((unused)), const void *argBlock __attribute__((unused)))
{
    ChunkInit();

    g_ModuleManager.threadId = sceKernelCreateThread("SceKernelModmgrWorker", (SceKernelThreadEntry)exe_thread,
        SCE_KERNEL_MODULE_INIT_PRIORITY, 0x1000, SCE_KERNEL_TH_DEFAULT_ATTR, NULL); // 0x00005078
    g_ModuleManager.mutexId = sceKernelCreateMutex("SceKernelModmgr", SCE_KERNEL_MUTEX_ATTR_TH_FIFO, 0, NULL); // 0x0000509C
    g_ModuleManager.eventId = sceKernelCreateEventFlag("SceKernelModmgr", SCE_KERNEL_EW_AND, 0, NULL); // 0x000050B8

    g_ModuleManager.userThreadId = SCE_KERNEL_VALUE_UNITIALIZED; // 0x000050DC
    g_ModuleManager.unk16 = SCE_KERNEL_VALUE_UNITIALIZED; // 0x000050D0

    g_ModuleManager.unk20 = &g_ModuleManager.unk20; // 0x000050D8
    g_ModuleManager.unk24 = &g_ModuleManager.unk20; //0x000050F0
    g_ModuleManager.npDrmGetModuleKeyFunction = NULL; // 0x000050E0
    g_ModuleManager.pModule = NULL; // 0x000050D4

    return SCE_KERNEL_RESIDENT;
}

// Subroutine ModuleMgrForUser_CDE1C1FE - Address 0x00005B10
SceBool sceKernelCheckTextSegment(void)
{
    s32 oldK1;
    SceModule *pMod;

    oldK1 = pspShiftK1();

    if (g_ModuleManager.pModule == NULL)
        //uOFW: pspSetK1(oldK1) forgotten by Sony
        return SCE_TRUE;

    pMod = g_ModuleManager.pModule;

    // 0x00005B2C - 0x00005B50
    u32 i, sum = 0;
    u32 *pTextSeg = (u32 *)pMod->segmentAddr[0];
    for (i = 0; i < pMod->textSize; i += 4)
        sum += pTextSeg[i >> 2];

    pspSetK1(oldK1);
    return (pMod->textSegmentChecksum == sum); // 0x00005B60
}

// Subroutine ModuleMgrForKernel_A40EC254 - Address 0x00005B6C
s32 sceKernelSetNpDrmGetModuleKeyFunction(s32(*function)(s32 fd, void *, void *))
{
    g_ModuleManager.npDrmGetModuleKeyFunction = function;
    return SCE_ERROR_OK;
}

// Subroutine ModuleMgrForKernel_C3DDABEF - Address 0x00005B7C
s32 sceKernelNpDrmGetModuleKey(SceUID fd, void *arg2, void *arg3)
{
    s32 status;

    if (arg2 == NULL || arg3 == NULL)
        return SCE_ERROR_KERNEL_ILLEGAL_ADDR;

    if (g_ModuleManager.npDrmGetModuleKeyFunction == NULL)
        return SCE_ERROR_KERNEL_ERROR;

    status = g_ModuleManager.npDrmGetModuleKeyFunction(fd, arg2, arg3); // 0x00005BB0

    return pspMin(status, SCE_ERROR_OK); // 0x00005BBC
}

// Subroutine ModuleMgrForKernel_1CFFC5DE - Address 0x00005BD0
s32 sceKernelModuleMgrMode(s32 mode)
{
    (void)mode;

    return SCE_ERROR_OK;
}

// sub_00005C4C
static s32 _LoadModule(SceModuleMgrParam *pModParams)
{
    SceOff pos;
    s32 status;
    SceModule *pMod;
    SceLoadCoreExecFileInfo *pExecInfo;

    pMod = pModParams->pMod;
    if (pMod == NULL || GET_MCB_STATUS(pMod->status) != MCB_STATUS_NOT_LOADED) // 0x00005C8C & 0x00005CA0
        return SCE_ERROR_KERNEL_ERROR;

    SET_MCB_STATUS(pMod->status, MCB_STATUS_LOADING); // 0x00005CBC

    pExecInfo = pModParams->pExecInfo;
    //memset(pExecInfo, 0, sizeof(SceLoadCoreExecFileInfo)); // 0x00005CC8
    pspClearMemory32(pExecInfo, sizeof(SceLoadCoreExecFileInfo));
    pExecInfo->apiType = pModParams->apiType;

    pos = sceIoLseek(pModParams->fd, 0, SCE_SEEK_CUR); //0x00005CE8

    SceUID tmpModuleFileBlockId = sceKernelAllocPartitionMemory(SCE_KERNEL_PRIMARY_KERNEL_PARTITION, "SceModmgrLMTmp",
        SCE_KERNEL_SMEM_High, 512, 0); // 0x00005D0C
    if (tmpModuleFileBlockId < SCE_ERROR_OK)
        return tmpModuleFileBlockId;

    SceUID newFd = 0; // 0x00005C84
    SceUID fd = pModParams->fd;
    void *pBlock = sceKernelGetBlockHeadAddr(tmpModuleFileBlockId); // 0x00005D20

    do {
        status = sceIoRead(fd, pBlock, 512); // 0x00005D34
        if (status <= 0) { // 0x00005D3C
            ClearFreePartitionMemory(tmpModuleFileBlockId); // 0x000067BC - 0x000067F0
            return (status < 0) ? status : (s32)SCE_ERROR_KERNEL_FILE_READ_ERROR; // 0x00006728
        }
        sceIoLseek(fd, pos, SCE_SEEK_SET); // 0x00005D50

        if (newFd != 0 || pModParams->unk124 != 0) // 0x00005D5C & 0x00005D68
            break;

        status = _CheckOverride(pModParams->apiType, pBlock, &newFd); //0x00005D74
        if (status == SCE_FALSE) // 0x00005D7C
            break;

        if (newFd < 0) // 0x00005D84
            while (1) {} // 0x00005D98

        fd = newFd;
        pModParams->unk124 = 1; // 0x00005D90
    } while (1); // 0x00005D90

    /* The first 512 bytes of the file of the module to be loaded have been read into pBlock. */
    u32 execCodeOffset;
    status = _CheckSkipPbpHeader(pModParams, fd, pBlock, &execCodeOffset, pModParams->apiType); // 0x00005DB0
    if (status < 0) { // 0x00005DB8
        ClearFreePartitionMemory(tmpModuleFileBlockId); // 0x0000674C - 0x00006780
        if (newFd != 0) // 0x0000678C
            sceIoClose(newFd); // 0x0000679C

        return status;
    }
    if (status > 0) { // 0x00005DC0
        /* The module contains a PBP header and we skip the PBP header for the module loading. */
        pos = sceIoLseek(fd, 0, SCE_SEEK_CUR); // 0x00005DD0

        status = sceIoRead(fd, pBlock, 512); // 0x00005DE8
        if (status <= 0) { // 0x00005DF0
            ClearFreePartitionMemory(tmpModuleFileBlockId); // 0x000066C0 - 0x00006708
            if (newFd != 0) // 0x0000678C
                sceIoClose(newFd); // 0x0000672C

            return (status < 0) ? status : (s32)SCE_ERROR_KERNEL_FILE_READ_ERROR; // 0x00006724
        }
        sceIoLseek(fd, pos, SCE_SEEK_SET); // 0x00005E04
    }
    pExecInfo->modeAttribute = SCE_EXEC_FILE_NO_HEADER_COMPRESSION; // 0x00005E10
    if (pModParams->unk124 & 0x1) // 0x00005E1C
        pExecInfo->isSignChecked = SCE_TRUE;

    pExecInfo->secureInstallId = pModParams->secureInstallId; // 0x00005E28
    char *secInstallId = pModParams->secureInstallId; // 0x00005E38

    status = sceKernelCheckExecFile(pBlock, pExecInfo); // 0x00005E34
    // TODO: starting here, pBlock contains status (in $s5)!
    if (status < SCE_ERROR_OK) { // 0x00005E3C
        _FreeMemoryResources(newFd, tmpModuleFileBlockId, pModParams, pExecInfo);
        return status;
    }

    /* Calculate size of executable. */
    if (pExecInfo->execSize == 0) { // 0x00005E48
        pos = sceIoLseek(fd, 0, SCE_SEEK_CUR); // 0x00006678
        u32 endPos = (u32)sceIoLseek(fd, 0, SCE_SEEK_END); // 0x00006694
        sceIoLseek(fd, pos, SCE_SEEK_SET); // 0x000066AC

        pExecInfo->execSize = endPos - (u32)pos; // 0x000066BC
    }

    if (pExecInfo->elfType == (u32)SCE_EXEC_FILE_TYPE_INVALID_ELF || pExecInfo->elfType == 0) { // 0x00005E5C
        _FreeMemoryResources(newFd, tmpModuleFileBlockId, pModParams, pExecInfo);
        return SCE_ERROR_KERNEL_UNSUPPORTED_PRX_TYPE;
    }
    // TODO: Does this make sense?
    pExecInfo->isCompressed = (pExecInfo->execAttribute & SCE_EXEC_FILE_COMPRESSED) ? SCE_FALSE : SCE_TRUE; // 0x00005E70

    /* 
     * Determine the memory partition to load the module to according to the module's privilege
     * level. Via the SceKernelLMOption field supplied custom memory partitions for the .text or 
     * the .data section are also checked for validity given the privilege level.
     * 
     * Additionally, if we are provided with an external memory block ID to load the module to, 
     * checks are performed if the memory block is located in a valid memory partition according 
     * to the module's privilege level.
     */
    if (pModParams->externMemBlockIdUser != 0) { // 0x00005E78
        if (pExecInfo->isKernelMod) { // 0x00005E84
            /* Cannot load a Kernel module into user space. */
            _FreeMemoryResources(newFd, tmpModuleFileBlockId, pModParams, pExecInfo);
            return SCE_ERROR_KERNEL_PARTITION_MISMATCH;
        }
        _GivenBlockInfoFromBlock(pModParams->externMemBlockIdUser, pModParams); // 0x00005E8C - 0x00005EC0
        pExecInfo->partitionId = pModParams->externMemBlockPartitionId; // 0x00005EC8
    } else if (pModParams->externMemBlockIdKernel != 0) { // 0x00005E78 & 0x00006510
        _GivenBlockInfoFromBlock(pModParams->externMemBlockIdKernel, pModParams); // 0x00006534 - 0x00006564

        /* Check memory partition conditions for kernel module. */
        if (pExecInfo->isKernelMod) { // 0x0000656C
            status = _CheckKernelOnlyModulePartition(pModParams->externMemBlockPartitionId); // 0x00006574 - 0x0000659C
            if (status < SCE_ERROR_OK) { // 0x00006608 & 0x00006618
                _FreeMemoryResources(newFd, tmpModuleFileBlockId, pModParams, pExecInfo);
                return status;
            }
            pExecInfo->partitionId = pModParams->externMemBlockPartitionId; // 0x00006630

            if (pModParams->mpIdData != SCE_KERNEL_UNKNOWN_PARTITION) { // 0x00006638
                status = _CheckKernelOnlyModulePartition(pModParams->mpIdData); // 0x00006640 - 0x0000666C
                if (status < SCE_ERROR_OK) {
                    _FreeMemoryResources(newFd, tmpModuleFileBlockId, pModParams, pExecInfo);
                    return status;
                }
            }
        } else {
            /* Check memory partition conditions for user module. */
            status = _CheckUserModulePartition(pModParams->externMemBlockPartitionId); // 0x000065F8 - 0x00006620
            if (status < SCE_ERROR_OK) { // 0x00006608 & 0x00006618
                _FreeMemoryResources(newFd, tmpModuleFileBlockId, pModParams, pExecInfo);
                return status;
            }
            pExecInfo->partitionId = pModParams->externMemBlockPartitionId; // 0x000065AC

            if (pModParams->mpIdData != SCE_KERNEL_UNKNOWN_PARTITION) { // 0x000065B0
                status = _CheckUserModulePartition(pModParams->mpIdData); // 0x000065BC - 0x000065E4
                if (status < SCE_ERROR_OK) {
                    _FreeMemoryResources(newFd, tmpModuleFileBlockId, pModParams, pExecInfo);
                    return status;
                }
            }
        }
    } else {
        status = _PartitionCheck(pModParams, pExecInfo); // 0x0000651C
        //pBlock = status; // 0x00006528 -- continue at loc_00005ECC
        if (status != SCE_ERROR_OK) { // 0x00006524
            _FreeMemoryResources(newFd, tmpModuleFileBlockId, pModParams, pExecInfo);
            return status;
        }
    }

    if (!(pExecInfo->execAttribute & SCE_EXEC_FILE_COMPRESSED)) // 0x00005ED4
        pExecInfo->maxAllocSize = pspMax(pExecInfo->execSize, pExecInfo->modCodeSize); // 0x00006504
    else if (!(pExecInfo->execAttribute & SCE_EXEC_FILE_GZIP_OVERLAP)) { // 0x00005EF0
        pExecInfo->maxAllocSize = pspMax(pExecInfo->modCodeSize, pExecInfo->decSize);
    } else
        pExecInfo->maxAllocSize = pspMax(pExecInfo->modCodeSize, pExecInfo->decSize) + pExecInfo->overlapSize * 256; // 0x00005F04

    if (pModParams->externMemBlockIdUser != 0) { // 0x00005F0C
        /* 
         * Check memory block offset conditions: 
         * 
         *  • highOffset cannot be != 0 (checked both in sceKernelLoadModuleWithBlockOffset()
         *    and here (highOffset > 0))
         *  • (lowOffset + pExecInfo->maxAllocSize) cannot overflow
         *  • (lowOffset + pExecInfo->maxAllocSize) cannot be greater than the memory block size
         */
        s32 highOffset = (s32)(pModParams->memBlockOffset >> 32); // 0x00005F18 -- $a3
        u32 lowOffset = (u32)pModParams->memBlockOffset; // 0x00005F14 -- $a2
        u8 overFlow = canOverflowOccur(lowOffset, pExecInfo->maxAllocSize); // 0x00005F2C -- overFlow == $t3

        if (highOffset > 0 || (highOffset == 0 && overFlow) // 0x00005F3C
                || ((highOffset + overFlow) == 0 // 0x00005F44
                    && (pModParams->externMemBlockSize < (u32)(lowOffset + pExecInfo->maxAllocSize)))) { // 0x00005F3C
            _FreeMemoryResources(newFd, tmpModuleFileBlockId, pModParams, pExecInfo);
            return SCE_ERROR_KERNEL_ERROR;
        }
    }
    // 0x00005F50 & 0x00005F64
    if (pModParams->externMemBlockIdKernel != 0 
            && pModParams->externMemBlockSize < pExecInfo->maxAllocSize) {
        _FreeMemoryResources(newFd, tmpModuleFileBlockId, pModParams, pExecInfo);
        return SCE_ERROR_KERNEL_ERROR;
    }

    // 0x00005F90
    /* Setup the memory block to load the (uncompressed) module to. */
    switch (pExecInfo->elfType) {
    case SCE_EXEC_FILE_TYPE_INVALID_ELF:
    case SCE_EXEC_FILE_TYPE_PRX_2: // 0x000064A8
        _FreeMemoryResources(newFd, tmpModuleFileBlockId, pModParams, pExecInfo);
        return SCE_ERROR_KERNEL_UNSUPPORTED_PRX_TYPE;
    case SCE_EXEC_FILE_TYPE_PRX: // 0x00005F98
        if (pModParams->externMemBlockIdUser != 0) // 0x00005F98
            pExecInfo->decompressionMemId = pModParams->externMemBlockIdUser;
        else if (pModParams->externMemBlockIdKernel != 0) // 0x0000642C
            pExecInfo->decompressionMemId = pModParams->externMemBlockIdKernel; // 0x00006438
        else {
            u8 blkAllocType = pModParams->position;
            u32 addr = 0; // 0x00006430      
            if (pExecInfo->maxSegAlign > 0x100) { // 0x00006448
                if (pModParams->position == SCE_KERNEL_LM_POS_LOW) {
                    blkAllocType = SCE_KERNEL_SMEM_LOWALIGNED; // 0x0000649C
                    addr = pExecInfo->maxSegAlign;
                } else if (pModParams->position == SCE_KERNEL_LM_POS_HIGH) { // 0x00006460
                    blkAllocType = SCE_KERNEL_SMEM_HIGHALIGNED; // 0x00006490
                    addr = pExecInfo->maxSegAlign;
                }
            }
            /* Allocate memory to load the module PRX into. */
            pExecInfo->decompressionMemId = sceKernelAllocPartitionMemory(pExecInfo->partitionId, "SceModmgrLMFileBufferPRX",
                blkAllocType, pExecInfo->maxAllocSize, addr); // 0x00006474
            if (pExecInfo->decompressionMemId < SCE_ERROR_OK) { // 0x0000647C
                _FreeMemoryResources(newFd, tmpModuleFileBlockId, pModParams, pExecInfo);
                return pExecInfo->decompressionMemId;
            }
        }
        break;
    case SCE_EXEC_FILE_TYPE_ELF: // 0x000064B4
        if (pModParams->externMemBlockIdKernel != 0) { // 0x000064B4
            _FreeMemoryResources(newFd, tmpModuleFileBlockId, pModParams, pExecInfo);
            return SCE_ERROR_KERNEL_ERROR;
        }
        /* Allocate memory for the module ELF to load into. */
        pExecInfo->topAddr = (void *)((s32)pExecInfo->topAddr & 0xFFFFFF00); // 0x000064D0
        pExecInfo->decompressionMemId = sceKernelAllocPartitionMemory(pExecInfo->partitionId, "SceModmgrLMFileBufferELF",
            SCE_KERNEL_SMEM_Addr, pExecInfo->maxAllocSize, (u32)pExecInfo->topAddr); // 0x00006474
        if (pExecInfo->decompressionMemId < SCE_ERROR_OK) { // 0x0000647C
            _FreeMemoryResources(newFd, tmpModuleFileBlockId, pModParams, pExecInfo);
            return pExecInfo->decompressionMemId;
        }
        break;
    default: // 0x00005F78
        _FreeMemoryResources(newFd, tmpModuleFileBlockId, pModParams, pExecInfo);
        return SCE_ERROR_KERNEL_UNSUPPORTED_PRX_TYPE;
    }

    void *baseAddr = sceKernelGetBlockHeadAddr(pExecInfo->decompressionMemId); // 0x00005FB8

    /* Use the specified offset into the supplied target memory partition by the the user. */
    if (pModParams->memBlockOffset != 0) // 0x00005FB0
        baseAddr += (u32)pModParams->memBlockOffset; // 0x00005FC4

    pExecInfo->fileBase = baseAddr; // 0x00005FCC

    pExecInfo->maxAllocSize = UPALIGN256(pExecInfo->maxAllocSize); // 0x00005FE0
    if (tmpModuleFileBlockId > 0) { // 0x00005FDC
        status = ClearFreePartitionMemory(tmpModuleFileBlockId); // 0x00005FE4 - 0x00006030, 0x00006034
        tmpModuleFileBlockId = 0; // 0x00006038
    }
    // 0x0000603C
    if (pExecInfo->execAttribute & SCE_EXEC_FILE_COMPRESSED) { // 0x00006044
        void *gzipBuf;
        SceSize size;
        pExecInfo->topAddr = pExecInfo->fileBase; // 0x00006050
        if (pModParams->externMemBlockIdUser != 0) { // 0x00006058
            tmpModuleFileBlockId = 0; // 0x00006360
            baseAddr = sceKernelGetBlockHeadAddr(pModParams->externMemBlockIdUser); // 0x0000635C
            pModParams->blockGzip = baseAddr + (pModParams->externMemBlockSize - UPALIGN64(pExecInfo->execSize)); // 0x00006358
            size = pExecInfo->execSize;
            gzipBuf = pModParams->blockGzip;
        } else if (pModParams->externMemBlockIdKernel != 0) { // 0x00006064
            tmpModuleFileBlockId = 0; // 0x00006324
            baseAddr = sceKernelGetBlockHeadAddr(pModParams->externMemBlockIdKernel); // 0x00006320
            if (pModParams->position != SCE_KERNEL_LM_POS_LOW && pModParams->position != SCE_KERNEL_LM_POS_HIGH) { // 0x00006330
                _FreeMemoryResources(newFd, tmpModuleFileBlockId, pModParams, pExecInfo);
                return status;
            }
            pModParams->blockGzip = baseAddr + (pModParams->externMemBlockSize - UPALIGN64(pExecInfo->execSize)); // 0x00006358
            size = pExecInfo->execSize;
            gzipBuf = pModParams->blockGzip;
        } else if (pExecInfo->execAttribute & SCE_EXEC_FILE_GZIP_OVERLAP) { // 0x0000606C
            baseAddr = sceKernelGetBlockHeadAddr(pExecInfo->decompressionMemId); // 0x00006074
            size = pExecInfo->execSize;
            gzipBuf = baseAddr + (pExecInfo->maxAllocSize - UPALIGN64(pExecInfo->execSize)); // 0x00006098
        } else {
            /* Allocate buffer for holding the GZIP compressed file. */
            tmpModuleFileBlockId = sceKernelAllocPartitionMemory(pExecInfo->partitionId, "SceModmgrGzipBuffer", SCE_KERNEL_SMEM_High,
                pExecInfo->execSize, 0); // 0x000062F8
            if (tmpModuleFileBlockId < SCE_ERROR_OK) {
                _FreeMemoryResources(newFd, tmpModuleFileBlockId, pModParams, pExecInfo);
                return tmpModuleFileBlockId;
            }
            gzipBuf = sceKernelGetBlockHeadAddr(tmpModuleFileBlockId); // 0x0000630C
            size = pExecInfo->execSize; // 
        }
        SceOff curPos = sceIoLseek(fd, 0, SCE_SEEK_CUR); // 0x000060A8

        /* Read rest of the content (segments,...) of the module to load. */
        status = sceIoRead(fd, gzipBuf, size); // 0x000060C0
        if (status >= SCE_ERROR_OK) // 0x000060C8
            status = ((SceSize)status == size) ? SCE_ERROR_OK : SCE_ERROR_KERNEL_ERROR;

        sceIoLseek(fd, curPos, SCE_SEEK_SET); // 0x000060F0
        if (status < SCE_ERROR_OK) { // 0x000060F8
            _FreeMemoryResources(newFd, tmpModuleFileBlockId, pModParams, pExecInfo);
            return status;
        }
        // 0x0000613C
        pExecInfo->modeAttribute = SCE_EXEC_FILE_DECRYPT; // 0x00006100
        if (pModParams->unk124 & 0x1) // 0x0000610C
            pExecInfo->isSignChecked = SCE_TRUE;

        pExecInfo->secureInstallId = secInstallId; // 0x0000612C

        /* Decrypt, decompress the module file. */
        status = sceKernelCheckExecFile(gzipBuf, pExecInfo); // 0x00006128
        if (status < SCE_ERROR_OK) {
            _FreeMemoryResources(newFd, tmpModuleFileBlockId, pModParams, pExecInfo);
            return status;
        }

        pExecInfo->isCompressed = SCE_TRUE;

        if (tmpModuleFileBlockId > 0) // 0x0000613C
            ClearFreePartitionMemory(tmpModuleFileBlockId);
    } else if (pExecInfo->isCompressed) { // 0x00006384
        SceOff curPos = sceIoLseek(fd, 0, SCE_SEEK_CUR); // 0x000063BC

        /* Read module file into module buffer. */
        status = sceIoRead(fd, pExecInfo->fileBase, pExecInfo->execSize); // 0x000063D4
        if (status >= SCE_ERROR_OK) // 0x000063DC
            status = ((SceSize)status == pExecInfo->execSize) ? SCE_ERROR_OK : SCE_ERROR_KERNEL_ERROR;

        sceIoLseek(fd, curPos, SCE_SEEK_SET); // 0x00006404
        if (status < SCE_ERROR_OK) {
            _FreeMemoryResources(newFd, tmpModuleFileBlockId, pModParams, pExecInfo);
            return status;
        }
        pExecInfo->modeAttribute = SCE_EXEC_FILE_DECRYPT; // 0x00006418
    } else {
        pExecInfo->isCompressed = SCE_TRUE; // 0x0000638C
        sceKernelMemmoveWithFill(pExecInfo->fileBase, NULL, pExecInfo->execSize, 0); // 0x00006398
    }

    if (newFd != 0) // 0x00006194
        sceIoClose(newFd);

    SET_MCB_STATUS(pMod->status, MCB_STATUS_LOADED); // 0x000061B0

    sceKernelRegisterModule(pMod);

    return SCE_ERROR_OK;
}

/*
 * Obtain the partition type and the memory size of the memory block
 * specified by <blockId>.
 */
static s32 _GivenBlockInfoFromBlock(SceUID blockId, SceModuleMgrParam *pModParams) 
{
    s32 status;
    SceUID partId;
    SceUID memBlkId;
    SceSysmemMemoryBlockInfo blkInfo;

    blkInfo.size = sizeof(SceSysmemMemoryBlockInfo);
    status = sceKernelQueryMemoryBlockInfo(blockId, &blkInfo); // 0x00005EA8
    if (status < SCE_ERROR_OK)
        return status;

    status = sceKernelQueryMemoryInfo(blkInfo.addr, &partId, &memBlkId);
    if (status >= SCE_ERROR_OK) {
        pModParams->externMemBlockPartitionId = partId;
        pModParams->externMemBlockSize = blkInfo.memSize;
    }
    return status;
}

/*
 * Free the memory resources used by a module during its load process. This includes
 * its file descriptor and, if not externally provided, the memory block to load the 
 * module into.
 * Call this function to handle cleanup in an error case.
 */
static void _FreeMemoryResources(SceUID fd, SceUID blockId, SceModuleMgrParam *pModParams,
    SceLoadCoreExecFileInfo *pExecInfo)
{
    if (fd > 0) // 0x00006204
        sceIoClose(fd); // 0x000062D8

    ClearFreePartitionMemory(blockId); // 0x0000620C - 0x00006258
    if (pModParams->externMemBlockIdUser != 0 || pModParams->externMemBlockIdKernel != 0 ||
            pExecInfo == NULL || pExecInfo->decompressionMemId <= 0) // 0x00006260 & 0x0000626C & 0x00006274 & 0x00006280
        return;

    ClearFreePartitionMemory(pExecInfo->decompressionMemId); // 0x00006284 - 0x000062CC
}

/*
 * Reset and free the specified memory block.
 */
s32 ClearFreePartitionMemory(SceUID blockId)
{
    s32 status;

    if (blockId <= 0)
        return SCE_ERROR_OK;

    SceSysmemMemoryBlockInfo blkInfo;
    blkInfo.size = sizeof(SceSysmemMemoryBlockInfo);
    status = sceKernelQueryMemoryBlockInfo(blockId, &blkInfo); // 0x000066D0
    if (status < SCE_ERROR_OK)
        return status;

    sceKernelMemset((void *)blkInfo.addr, 0, blkInfo.memSize); // 0x00006778
    status = sceKernelFreePartitionMemory(blockId); // 0x00006780

    return status;
}

/*
 * Check if specified memory partition is a USER partition.
*/
s32 _CheckUserModulePartition(SceUID memoryPartitionId)
{
    s32 status;
    SceSysmemPartitionInfo partitionInfo;

    partitionInfo.size = sizeof(SceSysmemPartitionInfo);
    status = sceKernelQueryMemoryPartitionInfo(memoryPartitionId, &partitionInfo);
    if (status < SCE_ERROR_OK || !(partitionInfo.attr & 3))
        return SCE_ERROR_KERNEL_PARTITION_MISMATCH;

    return SCE_ERROR_OK;
}

/*
* Check if specified memory partition is a KERNEL only partition.
*/
static s32 _CheckKernelOnlyModulePartition(SceUID memoryPartitionId)
{
    s32 status;
    SceSysmemPartitionInfo partitionInfo;

    partitionInfo.size = sizeof(SceSysmemPartitionInfo);
    status = sceKernelQueryMemoryPartitionInfo(memoryPartitionId, &partitionInfo);
    if (status < SCE_ERROR_OK || partitionInfo.attr != 12)
        return SCE_ERROR_KERNEL_PARTITION_MISMATCH;

    return SCE_ERROR_OK;
}

// sub_00006800
static s32 _RelocateModule(SceModuleMgrParam *pModParams)
{
    s32 status;
    SceModule *pMod;
    SceUID gzipBlockId;
    SceLoadCoreExecFileInfo *pExecInfo;

    gzipBlockId = 0; // 0x00006808

    pExecInfo = pModParams->pExecInfo;
    pExecInfo->apiType = pModParams->apiType; // 0x00006844
    pExecInfo->unk12 = pModParams->unk100; // 0x00006858

    pMod = pModParams->pMod; // 0x0000683C

    /* 
     * We only enter the following block if the module is loaded via sceKernelLoadModuleBuffer_*()
     * functions. In other words, it is already loaded into memory.
     */
	// TODO: In the assembly, it's pExecInfo->elfType < 1, are there other possible elfType values?
    if ((s32)pExecInfo->elfType == SCE_KERNEL_VALUE_UNITIALIZED || pExecInfo->elfType == 0) { // 0x00006854

        pExecInfo->modeAttribute = SCE_EXEC_FILE_NO_HEADER_COMPRESSION; // 0x00006860
        pExecInfo->fileBase = pModParams->fileBase; // 0x00006874
        if (pModParams->unk124 & 0x1) // 0x00006870
            pExecInfo->isSignChecked = SCE_TRUE; // 0x00006878

        pExecInfo->secureInstallId = pModParams->secureInstallId; //0x00006880

        status = sceKernelCheckExecFile(pExecInfo->fileBase, pExecInfo); // 0x00006884
        if (status < SCE_ERROR_OK) { // 0x0000688C
            _CleanupMemory(gzipBlockId, pExecInfo);
            return status;
        }

        status = _PartitionCheck(pModParams, pExecInfo); // 0x00006898
        if (status < SCE_ERROR_OK) {
            _CleanupMemory(gzipBlockId, pExecInfo); // 0x000068A0
            return status;
        }

        if ((pExecInfo->execAttribute & SCE_EXEC_FILE_COMPRESSED)
                || pExecInfo->apiType == SCE_EXEC_FILE_APITYPE_MODULE_BUFFER_KERNEL 
                || pExecInfo->apiType == SCE_EXEC_FILE_APITYPE_MODULE_BUFFER_VSH 
                || pExecInfo->apiType == SCE_EXEC_FILE_APITYPE_MODULE_BUFFER_USBWLAN
                || pExecInfo->apiType == SCE_EXEC_FILE_APITYPE_MODULE_BUFFER_MS
                || pExecInfo->apiType == SCE_EXEC_FILE_APITYPE_MODULE_BUFFER_APP) // 0x000068B0 - 0x000068E8
        {

            SceSize decSize = (pExecInfo->decSize < pExecInfo->modCodeSize) ? pExecInfo->modCodeSize : pExecInfo->decSize; // 0x00006904
            SceSize execSize = (pExecInfo->execSize < decSize) ? decSize : pExecInfo->execSize; // 0x0000690C
            SceSize bufSize = (pModParams->modSize < execSize) ? execSize : pModParams->modSize; // 0x00006918

            pExecInfo->maxAllocSize = UPALIGN256(bufSize); // 0x00006934

            /* Allocate memory space for the module to relocate. */
            SceUID moduleBlockId;
            u8 blkAllocType;
            u32 addr;
            switch (pExecInfo->elfType) {
            case SCE_EXEC_FILE_TYPE_PRX:
                // 0x000006954
                blkAllocType = pModParams->position;
                addr = 0;
                if (pExecInfo->maxSegAlign > 0x100) { // 0x00006964
                    if (pModParams->position == SCE_KERNEL_LM_POS_LOW) { // 0x00006974
                        blkAllocType = SCE_KERNEL_SMEM_LOWALIGNED; // 0x00006F20
                        addr = pExecInfo->maxSegAlign;
                    }

                    if (pModParams->position == SCE_KERNEL_LM_POS_HIGH) { // 0x00006974
                        blkAllocType = SCE_KERNEL_SMEM_HIGHALIGNED; // 0x00006F14
                        addr = pExecInfo->maxSegAlign;
                    }
                }
                moduleBlockId = sceKernelAllocPartitionMemory(pExecInfo->partitionId, "SceModmgrModuleBody",
                    blkAllocType, pExecInfo->maxAllocSize, addr); // 0x00006990
                if (moduleBlockId < 0) { // 0x0000699C
                    _CleanupMemory(gzipBlockId, pExecInfo);
                    return moduleBlockId;
                }
                break;
            case SCE_EXEC_FILE_TYPE_ELF:
                // 0x00006F38
                moduleBlockId = sceKernelAllocPartitionMemory(pExecInfo->partitionId, "SceModmgrElfBody",
                    SCE_KERNEL_SMEM_Addr, pExecInfo->maxAllocSize, (u32)pExecInfo->topAddr); // 0x00006F4C
                if (moduleBlockId < 0) // 0x00006F54
                    return moduleBlockId;

                break;
            default:
                // 0x00006F2C
                _CleanupMemory(gzipBlockId, pExecInfo);
                return SCE_ERROR_KERNEL_UNSUPPORTED_PRX_TYPE;
            }

            pExecInfo->topAddr = sceKernelGetBlockHeadAddr(moduleBlockId); // 0x000069A4
            pMod->moduleBlockId = moduleBlockId;
            pMod->mpIdText = pExecInfo->partitionId; // 0x000069C0
            pMod->mpIdData = pExecInfo->partitionId; // 0x000069C8

            pExecInfo->isCompressed = SCE_TRUE; // 0x000069D0
            pExecInfo->decompressionMemId = moduleBlockId; // 0x000069D8

            if (pExecInfo->apiType == SCE_EXEC_FILE_APITYPE_MODULE_BUFFER_KERNEL
                    || pExecInfo->apiType == SCE_EXEC_FILE_APITYPE_MODULE_BUFFER_VSH
                    || pExecInfo->apiType == SCE_EXEC_FILE_APITYPE_MODULE_BUFFER_USBWLAN
                    || pExecInfo->apiType == SCE_EXEC_FILE_APITYPE_MODULE_BUFFER_MS
                    || pExecInfo->apiType == SCE_EXEC_FILE_APITYPE_MODULE_BUFFER_APP) { // 0x000069D4

                // 0x00006E48
                if (pExecInfo->execAttribute & SCE_EXEC_FILE_COMPRESSED) {

                    // 0x00006E50
                    void *topAddr;
                    if (pModParams->externMemBlockIdKernel == 0) { // 0x00006E64
                        gzipBlockId = sceKernelAllocPartitionMemory(pExecInfo->partitionId, "SceModmgrRelocateModuleGzip",
                            (pModParams->position == SCE_KERNEL_LM_POS_HIGH) ? SCE_KERNEL_LM_POS_LOW : SCE_KERNEL_LM_POS_HIGH, 
                            pExecInfo->execSize, 0); // 0x00006EC0
                        if (gzipBlockId < 0) { // 0x00006ECC
                            _CleanupMemory(gzipBlockId, pExecInfo);
                            return gzipBlockId;
                        }
                        topAddr = sceKernelGetBlockHeadAddr(gzipBlockId); // 0x00006ED4
                    } else {
                        gzipBlockId = 0; // 0x00006E70
                        topAddr = sceKernelGetBlockHeadAddr(pModParams->externMemBlockIdKernel) + 
                            (pModParams->externMemBlockSize - UPALIGN64(pExecInfo->execSize)); // 0x00006E6C - 0x00006E90
                        pModParams->blockGzip = topAddr; // 0x00006E94
                    }

                    sceKernelMemmove(topAddr, pExecInfo->fileBase, pExecInfo->execSize); // 0x00006EA0
                    pExecInfo->fileBase = topAddr; // 0x00006EAC
                } else {
                    sceKernelMemmove(pExecInfo->topAddr, pExecInfo->fileBase, bufSize); // 0x00006EE8
                    pExecInfo->fileBase = pExecInfo->topAddr; // 0x00006EF8
                }
            }
        }

        // 0x00006A00
        pExecInfo->modeAttribute = SCE_EXEC_FILE_DECRYPT; // 0x000068EC
        if (pModParams->unk124 & 0x1) // 0x00006A08
            pExecInfo->isSignChecked = SCE_TRUE; // 0x00006A14

        pExecInfo->secureInstallId = pModParams->secureInstallId; // 0x00006A18

        status = sceKernelCheckExecFile(pExecInfo->fileBase, pExecInfo); // 0x00006A1C
        if (status < SCE_ERROR_OK) { // 0x00006A24
            _CleanupMemory(gzipBlockId, pExecInfo);
            return status;
        }
    }

    status = allocate_module_block(pModParams); // 0x00006A2C
    if (status < SCE_ERROR_OK) {
        _CleanupMemory(gzipBlockId, pExecInfo);
        return status;
    }

    status = sceKernelLoadExecutableObject(pExecInfo->fileBase, pExecInfo); // 0x00006A40
    if (status < SCE_ERROR_OK) {
        _CleanupMemory(gzipBlockId, pExecInfo);
        return status;
    }

    ClearFreePartitionMemory(gzipBlockId); // 0x00006A50 - 0x00006A98
    gzipBlockId = 0; // 0x00006A9C

    if (!pExecInfo->isCompressed) { // 0x00006AA4
        ClearFreePartitionMemory(pExecInfo->decompressionMemId);

        if (pExecInfo->memBlockId > 0) // 0x00006B04
            pMod->moduleBlockId = pExecInfo->memBlockId; // 0x00006B08

        pExecInfo->decompressionMemId = pMod->moduleBlockId; // 0x00006B10
    }
    if (pModParams->externMemBlockIdUser != 0) { // 0x00006B18
        SceSysmemMemoryBlockInfo blkInfo;
        blkInfo.size = sizeof(SceSysmemMemoryBlockInfo);
        sceKernelQueryMemoryBlockInfo(pModParams->externMemBlockIdUser, &blkInfo); // 0x00006E28

        if ((pExecInfo->modCodeSize + pModParams->memBlockOffset) < blkInfo.memSize) // 0x00006E38
            sceKernelMemset((void *)(blkInfo.addr + pExecInfo->modCodeSize + (u32)pModParams->memBlockOffset), 0,
                            blkInfo.memSize - (pExecInfo->modCodeSize + (u32)pModParams->memBlockOffset)); // 0x00006D8C
    } else if (pModParams->externMemBlockIdKernel != 0) { // 0x00006B24
		SceBool cutBefore = SCE_FALSE;
        if (pModParams->position == SCE_KERNEL_LM_POS_HIGH) // 0x00006B38
            cutBefore = SCE_TRUE;

        pModParams->unk116 = sceKernelSeparateMemoryBlock(pModParams->externMemBlockIdKernel, cutBefore, pExecInfo->modCodeSize); // 0x00006B48
        if (pModParams->pNewBlockId != NULL) // 0x00006B54
            *(pModParams->pNewBlockId) = pModParams->unk116; // 0x00006B60

        SceSysmemMemoryBlockInfo blkInfo;
        blkInfo.size = sizeof(SceSysmemMemoryBlockInfo);
        sceKernelQueryMemoryBlockInfo(pModParams->externMemBlockIdKernel, &blkInfo); // 0x00006B74

        if (pExecInfo->modCodeSize < blkInfo.memSize) // 0x00006B84
            sceKernelMemset((void *)(blkInfo.addr + pExecInfo->modCodeSize), 0, blkInfo.memSize - pExecInfo->modCodeSize); // 0x00006D8C
    } else {
        SceSysmemMemoryBlockInfo blkInfo;
        blkInfo.size = sizeof(SceSysmemMemoryBlockInfo);
        sceKernelQueryMemoryBlockInfo(pMod->moduleBlockId, &blkInfo); // 0x00006DB0

        if (pExecInfo->modCodeSize < blkInfo.memSize) // 0x00006DC0
            sceKernelMemset((void *)(blkInfo.addr + pExecInfo->modCodeSize), 0, blkInfo.memSize - pExecInfo->modCodeSize); // 0x00006E08

        if (pExecInfo->isCompressed) { // 0x00006DCC
            status = sceKernelResizeMemoryBlock(pExecInfo->decompressionMemId, 0, pExecInfo->modCodeSize - pExecInfo->maxAllocSize); // 0x00006DE4
            if (status >= 0) // 0x00006DEC
                pExecInfo->maxAllocSize = pExecInfo->modCodeSize; // 0x00006DFC
        }
    }
    
    if (pModParams->externMemBlockIdUser != 0) // 0x00006B90
        pMod->status = 0x2000; //0x00006B9C

    status = sceKernelAssignModule(pMod, pExecInfo); // 0x00006BA0
    if (status < SCE_ERROR_OK) {
        _CleanupMemory(gzipBlockId, pExecInfo);
        return status;
    }

    if (pModParams->externMemBlockIdUser != 0) // 0x00006BB4
        pMod->status |= 0x3000; // 0x00006BC4
    if (pModParams->externMemBlockIdKernel != 0) // 0x00006BCC
        pMod->status |= 0x1000; // 0x00006BD8

    sceKernelDcacheWritebackAll(); // 0x00006BE0

    SET_MCB_STATUS(pMod->status, MCB_STATUS_RELOCATED); // 0x00006BFC

    if (pMod->entSize == 0)
        return status;

    void *pCurEntry = pMod->entTop; // 0x00006BF8
    void *pLastEntry = pMod->entTop + pMod->entSize; // 0x00006C0C

    // 0x00006C14 - 0x00006C34
    while (pCurEntry < pLastEntry) {
        SceResidentLibraryEntryTable *pCurTable = (SceResidentLibraryEntryTable *)pCurEntry;
        if (pCurTable->attribute & SCE_LIB_IS_SYSLIB) // 0x00006C18
            _ProcessModuleExportEnt(pMod, pCurTable); // 0x00006C6C

        pCurEntry += pCurTable->len * sizeof(void *);
    }

    return status;
}

//loc_00006CCC
/*
 * Performs cleanup of module resources used for the relocation process.
 * Call this function in an error case for _RelocateModule().
 */
static void _CleanupMemory(SceUID gzipMemId, SceLoadCoreExecFileInfo *pExecInfo)
{
    ClearFreePartitionMemory(gzipMemId);

    if (pExecInfo == NULL) // 0x00006CCC
        return;

    ClearFreePartitionMemory(pExecInfo->decompressionMemId); // 0x00006CD8 - 0x00006D20
    ClearFreePartitionMemory(pExecInfo->memBlockId); // 0x00006D28 - 0x00006D74
}

// sub_00006F80
/*
 * Release the resident libraries of the specified module.
*/
static s32 _ModuleReleaseLibraries(SceModule *pMod)
{
    void *pCurEntry;
    void *pLastEntry;

    if (pMod->entSize == 0)
        return SCE_ERROR_OK;

    pCurEntry = pMod->entTop;
    pLastEntry = pMod->entTop + pMod->entSize;

    while (pCurEntry < pLastEntry) {
        SceResidentLibraryEntryTable *pCurTable = (SceResidentLibraryEntryTable *)pCurEntry;
        if (pCurTable->attribute & SCE_LIB_IS_SYSLIB) { //0x00006FB4
            pCurEntry += pCurTable->len * sizeof(void *);
            continue;
        }

        sceKernelReleaseLibrary(pCurTable); //0x00006FBC

        pCurEntry += pCurTable->len * sizeof(void *);
    }
    return SCE_ERROR_OK;
}

// Subroutine sub_00006FF4 - Address 0x00006FF4 
static s32 _StartModule(SceModuleMgrParam *pModParams, SceModule *pMod, SceSize argSize, void *argp, s32 *pStatus)
{
    s32 status;

    if (GET_MCB_STATUS(pMod->status) != MCB_STATUS_RELOCATED) // 0x00007034
        return SCE_ERROR_KERNEL_ERROR;

    SET_MCB_STATUS(pMod->status, MCB_STATUS_STARTING); //0x0000704C

    status = _PrologueModule(pModParams, pMod); // 0x00007048
    if (status < SCE_ERROR_OK) {
        SET_MCB_STATUS(pMod->status, MCB_STATUS_RELOCATED); //0x00007138
        return SCE_ERROR_KERNEL_ERROR;
    }

    /*
     * UOFW: If pMod has no entry point, _PrologueModule returns SCE_ERROR_OK, with pMod->userModThid == SCE_KERNEL_VALUE_UNITIALIZED.
     * This theoretically allows modules to be loaded without having a dedicated module_start() functions.
     * No cleanup for such module is performed.
     * The SDK specifies, however, that every module needs to have a module_start() function.
     */
    s32 modStartStatus = SCE_KERNEL_START_SUCCESS; // 0x00007060
    if (pMod->userModThid != SCE_KERNEL_VALUE_UNITIALIZED) { //0x0000705C
        g_ModuleManager.userThreadId = pMod->userModThid; // 0x0000706C

        /* Execute the entry function module_start() of the specified module. */
        status = sceKernelStartThread(pMod->userModThid, argSize, argp); //0x00007078
        if (status == SCE_ERROR_OK) // 0x00007080
            /* wait for the module_start() function to finish. */
            modStartStatus = sceKernelWaitThreadEnd(pMod->userModThid, NULL); // 0x00007114

        /* Perform cleanup once module_start() was finished. */
        g_ModuleManager.userThreadId = SCE_KERNEL_VALUE_UNITIALIZED;
        sceKernelDeleteThread(pMod->userModThid); // 0x00007094
        pMod->userModThid = SCE_KERNEL_VALUE_UNITIALIZED;
    }

    if (pStatus != NULL) // 0x000070A0
        *pStatus = modStartStatus;

    /*
     * Unload the given module, if
     *		a) the module is not a resident module
     *		b) it couldn't be started successfully
     */
    if (modStartStatus != SCE_KERNEL_RESIDENT) { // 0x000070A8
        _EpilogueModule(pMod);
        SET_MCB_STATUS(pMod->status, MCB_STATUS_STOPPED); //0x00007100

        _UnloadModule(pMod); // 0x00007104

        return modStartStatus;
    }
    
    /* The started module is a resident module. */
    SET_MCB_STATUS(pMod->status, MCB_STATUS_STARTED); //0x000070B8
    return SCE_KERNEL_RESIDENT;
}

// Subroutine sub_0000713C - Address 0x0000713C
static s32 _StopModule(SceModuleMgrParam *pModParams, SceModule *pMod, s32 startOp, SceUID modId, SceSize argSize,
    void *argp, s32 *pStatus)
{
    s32 status;
    s32 threadPriority;
    SceSize stackSize;
    SceUInt threadAttr;

    (void)startOp;
    (void)modId;

    switch (GET_MCB_STATUS(pMod->status)) { // 0x000071A4
    case MCB_STATUS_NOT_LOADED: 
    case MCB_STATUS_LOADING: 
    case MCB_STATUS_STARTING: 
        return SCE_ERROR_KERNEL_ERROR;

    case MCB_STATUS_LOADED: 
    case MCB_STATUS_RELOCATED:
        // 0x000071AC
        return SCE_ERROR_KERNEL_MODULE_NOT_STARTED;
    case MCB_STATUS_STARTED:
        // 0x000071F4
        if (pMod->moduleStop != SCE_KERNEL_PTR_UNITIALIZED) { // 0x000071FC
            /* Setup the thread attributes for the module STOP thread. */
            threadPriority = pModParams->threadPriority;
            if (threadPriority == SCE_KERNEL_INVALID_PRIORITY) { //0x00007208
                threadPriority = (pMod->moduleStopThreadPriority == SCE_KERNEL_VALUE_UNITIALIZED) ? SCE_KERNEL_MODULE_INIT_PRIORITY
                    : pMod->moduleStopThreadPriority; //0x0000721C
            }
            stackSize = pModParams->stackSize;
            if (stackSize == 0)
                stackSize = (pMod->moduleStopThreadStacksize == (SceSize)SCE_KERNEL_VALUE_UNITIALIZED) ? 0 : pMod->moduleStopThreadStacksize; // 0x00007238

            threadAttr = (pMod->moduleStopThreadAttr == (u32)SCE_KERNEL_VALUE_UNITIALIZED) ? SCE_KERNEL_TH_DEFAULT_ATTR : pMod->moduleStopThreadAttr; //0x00007250
            if (pModParams->threadAttr != SCE_KERNEL_TH_DEFAULT_ATTR)
                threadAttr |= (pModParams->threadAttr & THREAD_SM_LEGAL_ATTR); // 0x00007260           
            threadAttr &= ~0xF0000000; //0x0000726C

            if (!(pMod->status & SCE_MODULE_USER_MODULE)) //0x00007268
                stackSize = (stackSize == 0) ? SCE_KERNEL_TH_KERNEL_DEFAULT_STACKSIZE : stackSize;
            else {
                stackSize = (stackSize == 0) ? SCE_KERNEL_TH_USER_DEFAULT_STACKSIZE : stackSize; //0x00007280

                if ((pMod->attribute & SCE_MODULE_PRIVILEGE_LEVELS) == SCE_MODULE_VSH) //0x00007284
                    threadAttr |= SCE_KERNEL_TH_VSH_MODE;
                else if ((pMod->attribute & SCE_MODULE_PRIVILEGE_LEVELS) == SCE_MODULE_APP) // 0x00007290
                    threadAttr |= SCE_KERNEL_TH_APP_MODE;
                else if ((pMod->attribute & SCE_MODULE_PRIVILEGE_LEVELS) == SCE_MODULE_USB_WLAN) // 0x0000729C
                    threadAttr |= SCE_KERNEL_TH_USB_WLAN_MODE;
                else if ((pMod->attribute & SCE_MODULE_PRIVILEGE_LEVELS) == SCE_MODULE_MS) // 0x000072A8
                    threadAttr |= SCE_KERNEL_TH_MS_MODE;
                else
                    threadAttr |= SCE_KERNEL_TH_USER_MODE;
            }
            SceKernelThreadOptParam threadOptions;
            threadOptions.size = sizeof(SceKernelThreadOptParam);
            threadOptions.stackMpid = (pModParams->threadMpIdStack == SCE_KERNEL_UNKNOWN_PARTITION) ? pMod->mpIdData : pModParams->threadMpIdStack; // 0x000072C0 & 0x000072CC
            if (threadOptions.stackMpid != SCE_KERNEL_UNKNOWN_PARTITION) { //0x000072D4
                if (pMod->status & SCE_MODULE_USER_MODULE) // 0x000072E0
                    status = _CheckUserModulePartition(threadOptions.stackMpid);
                else
                    status = _CheckKernelOnlyModulePartition(threadOptions.stackMpid);

                if (status < SCE_ERROR_OK)
                    return status;
            }
            s32 oldGp = pspSetGp(pMod->gpValue); //0x0000732C

            /* Create the module STOP thread. */
            status = sceKernelCreateThread("SceKernelModmgrStop", pMod->moduleStop, threadPriority, stackSize,
                threadAttr, &threadOptions); // 0x00007348
            pMod->userModThid = status;

            pspSetGp(oldGp); // 0x00007354

            if (status < SCE_ERROR_OK) // 0x0000735C
                return status;

            g_ModuleManager.userThreadId = status; // 0x00007370

            SET_MCB_STATUS(pMod->status, MCB_STATUS_STOPPING); // 0x0000737C  
            s32 stopResult = SCE_KERNEL_STOP_FAIL; // 0x00007390

            /* Execute the module STOP thread. */
            status = sceKernelStartThread(pMod->userModThid, argSize, argp); // 0x0000738C
            if (status == SCE_ERROR_OK) // 0x00007394
                /* Wait for the module STOP thread to finish. */
                stopResult = sceKernelWaitThreadEnd(pMod->userModThid, NULL); //0x0000747C

            /* Delete the module STOP thread. */
            g_ModuleManager.userThreadId = SCE_KERNEL_VALUE_UNITIALIZED;
            sceKernelDeleteThread(pMod->userModThid); // 0x000073A8
            pMod->userModThid = SCE_KERNEL_VALUE_UNITIALIZED;

            sceKernelChangeThreadPriority(SCE_KERNEL_THREAD_ID_SELF, SCE_KERNEL_MODULE_INIT_PRIORITY); // 0x000073B8

            if (pStatus != NULL) // 0x000073C0
                *pStatus = stopResult;

            if (sceKernelIsToolMode() != 0) { // 0x000073D0
                status = sceKernelDipsw(25); // 0x0000742C

                if (status != 1 && sceKernelGetCompiledSdkVersion() >= 0x3030000
                        && (u32)sceKernelSegmentChecksum(pMod) != pMod->segmentChecksum)
                    pspBreak(SCE_BREAKCODE_ZERO); // 0x00007470
            }
            /* 
             * If the module_stop entry function was executed successfully, unlink 
             * the stub libraries and and unregister the resident libraries of the 
             * stopped module.
             */
            if (stopResult == SCE_KERNEL_STOP_SUCCESS) { // 0x000073D8
                status = _EpilogueModule(pMod); // 0x00007408
                if (status < SCE_ERROR_OK) {
                    SET_MCB_STATUS(pMod->status, MCB_STATUS_STARTED);
                    return status; // 0x000073FC
                }
                SET_MCB_STATUS(pMod->status, MCB_STATUS_STOPPED); //0x00007428
                return SCE_KERNEL_STOP_SUCCESS; // 0x000073FC
            }

            /* The module couldn't be stopped successfully. */
            if (stopResult == SCE_KERNEL_STOP_FAIL) { // 0x000073E0
                SET_MCB_STATUS(pMod->status, MCB_STATUS_STARTED); // 0x000073F4
                return SCE_ERROR_KERNEL_MODULE_CANNOT_STOP;
            }

            /* The module_stop entry function returned a user-specific value. */
            SET_MCB_STATUS(pMod->status, MCB_STATUS_STARTED); // 0x000073F4
            return stopResult;
        }
        /* 
         * Unlink the stub libraries and and unregister the resident libraries of the
         * stopped module.
         */
        status = _EpilogueModule(pMod); // 0x00007408
        if (status < SCE_ERROR_OK) {
            SET_MCB_STATUS(pMod->status, MCB_STATUS_STARTED);
            return status; // 0x000073FC
        }
        SET_MCB_STATUS(pMod->status, MCB_STATUS_STOPPED); //0x00007428
        return SCE_KERNEL_STOP_SUCCESS; // 0x000073FC

    case MCB_STATUS_STOPPING:
        // 0x000074CC
        return SCE_ERROR_KERNEL_MODULE_ALREADY_STOPPING;
    case MCB_STATUS_STOPPED:
        // 0x000074D8
        return SCE_ERROR_KERNEL_MODULE_ALREADY_STOPPED;
    default: // 0x0000718C
        return SCE_ERROR_KERNEL_ERROR;
    }
}

// Subroutine sub_000074E4 - Address 0x000074E4 
s32 _start_exe_thread(SceModuleMgrParam *pModParams)
{
    SceUID threadId;
    SceUID result;
    s32 status;

    threadId = sceKernelGetThreadId();
    if (threadId < 0)
        return threadId;

    /* 
     * Prevent deadlock situation. A deadlock would occur when the module_start-thread 
     * of the currently started module wants to load/start/stop/unload another module. 
     */
    if (threadId == g_ModuleManager.userThreadId) {
        Kprintf("module manager busy.\n");
        return SCE_ERROR_KERNEL_MODULE_MANAGER_BUSY;
    }

    pModParams->pResult = &result;
    pModParams->eventId = g_ModuleManager.eventId;

    /* Ensure that only ONE module is loaded/started/stopped/unloaded at any given time. */
    status = sceKernelLockMutex(g_ModuleManager.mutexId, 1, NULL);
    if (status < SCE_ERROR_OK)
        return status;
    
    status = sceKernelStartThread(g_ModuleManager.threadId, sizeof(SceModuleMgrParam), pModParams);
    if (status < SCE_ERROR_OK) {
        sceKernelUnlockMutex(g_ModuleManager.mutexId, 1);
        /* UOFW: 'status' should be returned here to return the correct error code. 'result' might be undefined. */
        return result;
    }
    /* Wait for the module {load, start, stop, unload}-operation to finish. */
    sceKernelWaitEventFlag(g_ModuleManager.eventId, 1, SCE_KERNEL_EW_CLEAR_ALL | SCE_KERNEL_EW_OR, NULL, NULL);

    sceKernelUnlockMutex(g_ModuleManager.mutexId, 1);
    return result;
}

// sub_00007968
static s32 _ProcessModuleExportEnt(SceModule *pMod, SceResidentLibraryEntryTable *pLib)
{
    u32 i;
    SceModuleEntryThread *pEntryThread;

    if (pMod->status & 0x400) // 0x00007970
        return SCE_ERROR_OK;

    pMod->status |= 0x400; // 0x0000797C

    //0x00007988 - 0x000079F8
    for (i = 0; i < pLib->stubCount; i++) {
        switch (pLib->entryTable[i]) {
        case NID_MODULE_REBOOT_PHASE: //0x000079C0
            pMod->moduleRebootPhase = (SceKernelRebootPhaseForKernel)pLib->entryTable[pLib->vStubCount + pLib->stubCount + i]; //0x00007C14
            break;
        case NID_MODULE_BOOTSTART: //0x00007B94
            pMod->moduleBootstart = (SceKernelThreadEntry)pLib->entryTable[pLib->vStubCount + pLib->stubCount + i]; //0x00007BF4
            break;
        case NID_MODULE_START: //0x00007BCC
            pMod->moduleStart = (SceKernelThreadEntry)pLib->entryTable[pLib->vStubCount + pLib->stubCount + i]; //0x00007BF0
            break;
        case NID_MODULE_STOP: //0x00007BA4
            pMod->moduleStop = (SceKernelThreadEntry)pLib->entryTable[pLib->vStubCount + pLib->stubCount + i]; //0x00007BC8
            break;
        case NID_MODULE_REBOOT_BEFORE: //0x000079E8
            pMod->moduleRebootBefore = (SceKernelRebootBeforeForKernel)pLib->entryTable[pLib->vStubCount + pLib->stubCount + i]; //0x00007B8C
            break;
        case NID_592743D8: // 0x000079D8
            break;
        }
    }
    //0x00007A04 - 0x00007A6C
    for (i = 0; i < pLib->vStubCount; i++) {
        switch (pLib->entryTable[pLib->stubCount + i]) {
        case NID_MODULE_TEXT_CHECKSUM: // 0x00007A48
            g_ModuleManager.pModule = pMod; // 0x00007B64
            pMod->computeTextSegmentChecksum = *(u32 *)(pLib->entryTable[2 * pLib->stubCount + pLib->vStubCount + i]); // 0x00007B70
            break;
        case NID_MODULE_INFO: // 0x00007AB8
            break;
        case NID_MODULE_STOP_THREAD_PARAM:
            pEntryThread = (SceModuleEntryThread *)pLib->entryTable[2 * pLib->stubCount + pLib->vStubCount + i];
            pMod->moduleStopThreadPriority = pEntryThread->initPriority; //0x00007AF0
            pMod->moduleStopThreadStacksize = pEntryThread->stackSize; //0x00007AF8
            pMod->moduleStopThreadAttr = pEntryThread->attr; //0x00007B04
            break;
        case NID_MODULE_REBOOT_BEFORE_THREAD_PARAM: //0x00007B08
            pEntryThread = (SceModuleEntryThread *)pLib->entryTable[2 * pLib->stubCount + pLib->vStubCount + i];
            pMod->moduleRebootBeforeThreadPriority = pEntryThread->initPriority; //0x00007B30
            pMod->moduleRebootBeforeThreadStacksize = pEntryThread->stackSize; //0x00007B38
            pMod->moduleRebootBeforeThreadAttr = pEntryThread->attr; //0x00007B44
            break;
        case NID_MODULE_START_THREAD_PARAM:
            pEntryThread = (SceModuleEntryThread *)pLib->entryTable[2 * pLib->stubCount + pLib->vStubCount + i];
            pMod->moduleStartThreadPriority = pEntryThread->initPriority; //0x00007A98
            pMod->moduleStartThreadStacksize = pEntryThread->stackSize; //0x00007AA0
            pMod->moduleStartThreadAttr = pEntryThread->attr; //0x00007AA8
            break;
        }
    }
    return SCE_ERROR_OK;
}

// sub_00007C34
static s32 allocate_module_block(SceModuleMgrParam *pModParams)
{
    s32 status;
    SceModule *pMod;
    SceLoadCoreExecFileInfo *pExecInfo;

    pMod = pModParams->pMod; // 0x00007C54
    pExecInfo = pModParams->pExecInfo; // 0x00007C50

    status = sceKernelProbeExecutableObject(pModParams->pExecInfo->fileBase, pModParams->pExecInfo); // 0x00007C5C
	if (status < SCE_ERROR_OK) // 0x00007C68
		return status;

    // 0x00007C70 - 0x00007C98
    switch (pExecInfo->elfType) {
    case SCE_EXEC_FILE_TYPE_PRX:
        // 0x00007D0C
        if ((pExecInfo->modInfoAttribute & SCE_MODULE_PRIVILEGE_LEVELS) == SCE_MODULE_KERNEL) { // 0x00007D18
            status = _CheckKernelOnlyModulePartition(pExecInfo->partitionId); // 0x00007E88 - 0x00007EC40x00007EB0
			if (status < SCE_ERROR_OK) // 0x00007EBC
				return status;

        } else {
            status = _CheckUserModulePartition(pExecInfo->partitionId);
			if (status < SCE_ERROR_OK)
				return status;
        }
        if (pExecInfo->isCompressed) { // 0x00007D68
            void *base = pExecInfo->fileBase; // 0x00007E28
            pExecInfo->topAddr = pExecInfo->fileBase; // 0x00007E28 & 0x00007E40

            if (pModParams->externMemBlockIdKernel != 0 && pModParams->position == SCE_KERNEL_LM_POS_HIGH) { // 0x00007E24 & 0x00007E30
				base = sceKernelGetBlockHeadAddr(pModParams->externMemBlockIdKernel); // 0x00007E5C

                /* Place module at highest possible address in the provided memory block. */
                pExecInfo->topAddr = base + (pModParams->externMemBlockSize - UPALIGN64(pExecInfo->modCodeSize)); // 0x00007E68 - 0x00007E84 & 0x00007E40
            }
            pMod->moduleBlockId = pExecInfo->decompressionMemId; // 0x00007E44
            //pMod->mpIdText = pExecInfo->partitionId; // 0x00007E4C
            //pMod->mpIdData = pExecInfo->partitionId; // 0x00007E58
        } else if (pModParams->position == SCE_KERNEL_LM_POS_ADDR) { // 0x00007D78
			pExecInfo->topAddr = pExecInfo->fileBase; // 0x00007E14
            pMod->moduleBlockId = pExecInfo->decompressionMemId; // 0x00007E1C
        } else { // 0x00007D80
            u8 blkAllocType = pModParams->position; // 0x00007DA8
            u32 addr = 0; // 0x00007DAC

            if (pExecInfo->maxSegAlign > 0x100) { // 0x00007D8C
                if (pModParams->position == SCE_KERNEL_LM_POS_LOW) { // 0x00007D94
                    blkAllocType = SCE_KERNEL_SMEM_LOWALIGNED; // 0x00007E08
                    addr = pExecInfo->maxSegAlign;
                }
                if (pModParams->position == SCE_KERNEL_LM_POS_HIGH) { // 0x00007D9C
                    blkAllocType = SCE_KERNEL_SMEM_HIGHALIGNED; // 0x00007E00
                    addr = pExecInfo->maxSegAlign;
                }
            }
            SceUID moduleBlockId = sceKernelAllocPartitionMemory(pExecInfo->partitionId, "SceModmgrModuleBlockAuto",
                blkAllocType, pExecInfo->modCodeSize, addr); // 0x00007DB0
			if (moduleBlockId < 0)
				return status;

            pExecInfo->topAddr = sceKernelGetBlockHeadAddr(moduleBlockId); // 0x00007DC4
            pMod->moduleBlockId = moduleBlockId; // 0x00007DD0
        }
        pMod->mpIdText = pExecInfo->partitionId; // 0x00007E4C (for if part), 0x00007DD8 (for else if part)
        pMod->mpIdData = pExecInfo->partitionId; // 0x00007E58 (for if part), 0x00007DE0 (for else if part)

        return (pExecInfo->topAddr == NULL) ? SCE_ERROR_KERNEL_NO_MEMORY : SCE_ERROR_OK; // 0x00007DF4
    case SCE_EXEC_FILE_TYPE_ELF:
        // 0x00007CA0
        /* Don't permit kernel mode for an ELF file. */
		if ((pExecInfo->modInfoAttribute & SCE_MODULE_PRIVILEGE_LEVELS) == SCE_MODULE_KERNEL)
			return SCE_ERROR_KERNEL_ERROR;

        if (pExecInfo->isCompressed) { // 0x00007CC0
            pExecInfo->topAddr = pExecInfo->fileBase; // 0x00007D00
            pMod->moduleBlockId = pExecInfo->decompressionMemId; // 0x00007CC4 & 0x00007D08
        }
        pMod->mpIdText = pExecInfo->partitionId; // 0x00007CCC
        pMod->mpIdData = pExecInfo->partitionId; // 0x00007CD4

        return SCE_ERROR_OK;
    default: // 0x00007ECC
        return SCE_ERROR_KERNEL_ILLEGAL_OBJECT_FORMAT;
    }
}

// sub_00007ED8
static s32 _CheckSkipPbpHeader(SceModuleMgrParam *pModParams, SceUID fd, void *pBlock, u32 *pOffset, s32 apiType)
{
    PBPHeader *pPbpHdr;

    (void)pModParams;
    pPbpHdr = (PBPHeader *)pBlock;

    switch (apiType) {
    case SCE_EXEC_FILE_APITYPE_DISC_EMU_MS2:
    case SCE_EXEC_FILE_APITYPE_DISC_EMU_EF2:
    case SCE_EXEC_FILE_APITYPE_MS1:
    case SCE_EXEC_FILE_APITYPE_MS2:
    case SCE_EXEC_FILE_APITYPE_MS3:
    case SCE_EXEC_FILE_APITYPE_MS4:
    case SCE_EXEC_FILE_APITYPE_MS5:
    case SCE_EXEC_FILE_APITYPE_MS6:
    case SCE_EXEC_FILE_APITYPE_EF1:
    case SCE_EXEC_FILE_APITYPE_EF2:
    case SCE_EXEC_FILE_APITYPE_EF3:
    case SCE_EXEC_FILE_APITYPE_EF4:
    case SCE_EXEC_FILE_APITYPE_EF5:
    case SCE_EXEC_FILE_APITYPE_EF6:
    case SCE_EXEC_FILE_APITYPE_UNK160:
    case SCE_EXEC_FILE_APITYPE_UNK161:
        // 0x00007F14
        if (pPbpHdr->magic[0] != 0 || pPbpHdr->magic[1] != 'P' || pPbpHdr->magic[2] != 'B' || pPbpHdr->magic[3] != 'P') // 0x00007F40
            return SCE_ERROR_KERNEL_UNSUPPORTED_PRX_TYPE;

        if (pOffset != NULL)
            *pOffset = pPbpHdr->dataPSAROff - pPbpHdr->dataPSPOff;

        sceIoLseek(fd, pPbpHdr->dataPSPOff, SCE_SEEK_CUR); // 0x00007F6C
        return pPbpHdr->dataPSPOff;
    default:
        // 0x00007F8C
        if (pPbpHdr->magic[0] == 0 && pPbpHdr->magic[1] == 'P' && pPbpHdr->magic[2] == 'B' && pPbpHdr->magic[3] == 'P') // 0x00007FB8
            return SCE_ERROR_KERNEL_UNSUPPORTED_PRX_TYPE;

        if (pOffset != NULL)
            *pOffset = 0;

        return 0;
    }
}

// sub_00007FD0
/* 
 * Check if the target memory partitions for the module to be loaded into, 
 * specified by the user via SceKernelLMOption, match the priviledge level 
 * of the module.
 */
static s32 _PartitionCheck(SceModuleMgrParam *pModParams, SceLoadCoreExecFileInfo *pExecInfo)
{
    s32 status;

    if (pExecInfo->isKernelMod) { // 0x00007FE8
        pExecInfo->partitionId = SCE_KERNEL_PRIMARY_KERNEL_PARTITION; // 0x000080A8

        if (pModParams->mpIdText != SCE_KERNEL_UNKNOWN_PARTITION) {
            status = _CheckKernelOnlyModulePartition(pModParams->mpIdText);
            if (status < SCE_ERROR_OK)
                return status;

            pExecInfo->partitionId = pModParams->mpIdText; // 0x00008120
        }
        if (pModParams->mpIdData != SCE_KERNEL_UNKNOWN_PARTITION) { // 0x000080B0
            status = _CheckKernelOnlyModulePartition(pModParams->mpIdData);
            if (status < SCE_ERROR_OK)
                return status;
        }
    } else {
        pExecInfo->partitionId = SCE_KERNEL_PRIMARY_USER_PARTITION;

        if (pModParams->mpIdText != SCE_KERNEL_UNKNOWN_PARTITION) {
            status = _CheckUserModulePartition(pModParams->mpIdText);
            if (status < SCE_ERROR_OK)
                return status;

            pExecInfo->partitionId = pModParams->mpIdText; // 0x00008098
        }
        if (pModParams->mpIdData != SCE_KERNEL_UNKNOWN_PARTITION) { // 0x00008004
            status = _CheckUserModulePartition(pModParams->mpIdData);
            if (status < SCE_ERROR_OK)
                return status;
        }
    }
    return SCE_ERROR_OK;
}

// sub_00008124
static s32 _PrologueModule(SceModuleMgrParam *pModParams, SceModule *pMod)
{
    s32 oldGp;
    s32 status;

    oldGp = pspGetGp(); // 0x00008150

    /* Register resident libraries. */
    status = ModuleRegisterLibraries(pMod); // 0x00008154
    if (status < SCE_ERROR_OK) // 0x0000815C
        return status;

    /* Link the module's stub libraries with their corresponding resident libraries. */
    if (pMod->stubTop != SCE_KERNEL_PTR_UNITIALIZED) { // 0x0000816C
        if (pMod->status & SCE_MODULE_USER_MODULE) // 0x0000817C
            status = sceKernelLinkLibraryEntriesWithModule(pMod, pMod->stubTop, pMod->stubSize); // 0x00008188
        else
            status = sceKernelLinkLibraryEntries(pMod->stubTop, pMod->stubSize); // 0x0000843C

        /* Release registered resident libraries in case of linking error. */
        if (status < SCE_ERROR_OK) { // 0x00008190
            _ModuleReleaseLibraries(pMod); // 0x00008424
            return status;
        }
    }

    pMod->segmentChecksum = sceKernelSegmentChecksum(pMod); // 0x00008198
    if (pMod->computeTextSegmentChecksum != 0) { // 0x000081A4
        u32 sum = 0;
        u32 *pTextSegment = (u32 *)pMod->segmentAddr[0];
        u32 i;
        for (i = 0; i < pMod->textSize; i += 4) // 0x000081D0
            sum += pTextSegment[i >> 2];
        pMod->textSegmentChecksum = sum; // 0x000081D8
    } else
        pMod->textSegmentChecksum = 0; // 0x00008420

    /* Obtain the module entry point. */
    SceKernelThreadEntry modStart = pMod->moduleStart;
    if ((void *)pMod->moduleStart == SCE_KERNEL_PTR_UNITIALIZED) // 0x000081E4
        modStart = (pMod->moduleBootstart != modStart) ? pMod->moduleBootstart : (SceKernelThreadEntry)pMod->entryAddr;

    /* No module entry point found. */
    if (modStart == SCE_KERNEL_PTR_UNITIALIZED || modStart == NULL) { // 0x000081FC
        pMod->userModThid = SCE_KERNEL_VALUE_UNITIALIZED;
        return SCE_ERROR_OK;
    }

    /* Obtain the thread attributes for the module START thread. */
    s32 threadPriority = pModParams->threadPriority;
    if (threadPriority == SCE_KERNEL_INVALID_PRIORITY) { // 0x0000823C
        threadPriority = (pMod->moduleStartThreadPriority == SCE_KERNEL_VALUE_UNITIALIZED) ? SCE_KERNEL_MODULE_INIT_PRIORITY
            : pMod->moduleStartThreadPriority; //0x0000824C
    }
    SceSize stackSize = pModParams->stackSize; // 0x00008258
    if (stackSize == 0) // 0x00008258
        stackSize = (pMod->moduleStartThreadStacksize == (SceSize)SCE_KERNEL_VALUE_UNITIALIZED) ? 0 : pMod->moduleStartThreadStacksize; // 0x0000826C

    s32 threadAttr = (pMod->moduleStartThreadAttr == (u32)SCE_KERNEL_VALUE_UNITIALIZED) ? SCE_KERNEL_TH_DEFAULT_ATTR : pMod->moduleStartThreadAttr; //0x00007250
    if (pModParams->threadAttr != SCE_KERNEL_TH_DEFAULT_ATTR) // 0x00008280
        threadAttr |= (pModParams->threadAttr & THREAD_SM_LEGAL_ATTR); // 0x00008294           
    threadAttr &= ~0xF0000000; //0x000082A4

    if (!(pMod->status & SCE_MODULE_USER_MODULE)) //0x000082A0
        stackSize = (stackSize == 0) ? SCE_KERNEL_TH_KERNEL_DEFAULT_STACKSIZE : stackSize; // 0x00008404
    else {
        stackSize = (stackSize == 0) ? SCE_KERNEL_TH_USER_DEFAULT_STACKSIZE : stackSize; //0x000082B8

        if ((pMod->attribute & SCE_MODULE_PRIVILEGE_LEVELS) == SCE_MODULE_VSH) //0x000082BC
            threadAttr |= SCE_KERNEL_TH_VSH_MODE;
        else if ((pMod->attribute & SCE_MODULE_PRIVILEGE_LEVELS) == SCE_MODULE_APP) // 0x000082C8
            threadAttr |= SCE_KERNEL_TH_APP_MODE;
        else if ((pMod->attribute & SCE_MODULE_PRIVILEGE_LEVELS) == SCE_MODULE_USB_WLAN) // 0x000082D4
            threadAttr |= SCE_KERNEL_TH_USB_WLAN_MODE;
        else if ((pMod->attribute & SCE_MODULE_PRIVILEGE_LEVELS) == SCE_MODULE_MS) // 0x000082E0
            threadAttr |= SCE_KERNEL_TH_MS_MODE;
        else
            threadAttr |= SCE_KERNEL_TH_USER_MODE;
    }
    /* Setup the (optional) thread options. */
    SceKernelThreadOptParam threadOptions;
    threadOptions.size = sizeof(SceKernelThreadOptParam);
    threadOptions.stackMpid = (pModParams->threadMpIdStack == SCE_KERNEL_UNKNOWN_PARTITION) ? pMod->mpIdData : pModParams->threadMpIdStack; // 0x000082F8 & 0x000083F8
    if (threadOptions.stackMpid != SCE_KERNEL_UNKNOWN_PARTITION) { // 0x0000830C
        if (pMod->status & SCE_MODULE_USER_MODULE)
            status = _CheckUserModulePartition(threadOptions.stackMpid);
        else
            status = _CheckKernelOnlyModulePartition(threadOptions.stackMpid);

        /* 
         * In case of an error unlink the stub libraries and unregister the 
         * resident libraries of the module. 
         */
        if (status < SCE_ERROR_OK) {
            if (pMod->stubTop != SCE_KERNEL_PTR_UNITIALIZED) // 0x00008364
                sceKernelUnlinkLibraryEntries(pMod->stubTop, pMod->stubSize); // 0x0000836C

            _ModuleReleaseLibraries(pMod); // 0x00008374
            return status;
        }
    }
    pspSetGp(pMod->gpValue); //0x00008384

    /* Create the module START thread. */
    status = sceKernelCreateThread("SceModmgrStart", (SceKernelThreadEntry)modStart, threadPriority, stackSize,
        threadAttr, &threadOptions); // 0x000083A0
    pMod->userModThid = status; // 0x000083A8

    pspSetGp(oldGp); // 0x000083AC

    /* 
     * If the module START thread couldn't be created successfully, unlink 
     * the stub libraries and unregister the resident libraries of the module.
     */
    if (status < SCE_ERROR_OK) { // 0x000083C0
        if (pMod->stubTop != SCE_KERNEL_PTR_UNITIALIZED) // 0x00008364
            sceKernelUnlinkLibraryEntries(pMod->stubTop, pMod->stubSize); // 0x0000836C

        _ModuleReleaseLibraries(pMod); // 0x00008374
        return status;
    }
    return SCE_ERROR_OK;
}

// sub_0000844C
static s32 ModuleRegisterLibraries(SceModule *pMod)
{
    void *pCurEntry;
    void *pLastEntry;
    s32 status;

    if (pMod->entSize == 0)
        return SCE_ERROR_OK;

    pCurEntry = pMod->entTop;
    pLastEntry = pMod->entTop + pMod->entSize;

    // 0x00008468 & 0x0000847C - 0x000084C4
    while (pCurEntry < pLastEntry) {
        SceResidentLibraryEntryTable *pCurTable = (SceResidentLibraryEntryTable *)pCurEntry;
        if (!(pCurTable->attribute & SCE_LIB_AUTO_EXPORT)) { //0x00006FB4
            if (pCurTable->attribute & SCE_LIB_IS_SYSLIB)
                _ProcessModuleExportEnt(pMod, pCurTable); // 0x00008558

            pCurEntry += pCurTable->len * sizeof(void *);
            continue;
        }

        /* Register resident libraries. */
        if (pMod->status & SCE_MODULE_USER_MODULE) // 0x0000849C
            status = sceKernelRegisterLibraryForUser(pCurEntry); // 0x000084A4
        else
            status = sceKernelRegisterLibrary(pCurEntry); // 0x0000853C

        /* Unregister the successfully registered resident libraries in case of an error. */
        if (status < SCE_ERROR_OK) { // 0x000084AC
            pLastEntry = pCurEntry;
            pCurEntry = pMod->entTop;
            // 0x000084F4
            while (pCurEntry < pLastEntry) {
                pCurTable = (SceResidentLibraryEntryTable *)pCurEntry;
                if (pCurTable->attribute & SCE_LIB_AUTO_EXPORT) // 0x00008504
                    sceKernelReleaseLibrary(pCurTable); // 0x0000852C

                pCurEntry += pCurTable->len * sizeof(void *);
            }
            return status;
        }
        pCurEntry += pCurTable->len * sizeof(void *);  // 0x000084BC
    }
    return SCE_ERROR_OK;
}

