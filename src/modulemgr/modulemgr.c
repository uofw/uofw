/* Copyright (C) 2011 - 2014 The uOFW team
   See the file COPYING for copying permission.
*/

#include <common_imp.h>
#include <interruptman.h>
#include <iofilemgr_kernel.h>
#include <loadcore.h>
#include <modulemgr.h>
#include <modulemgr_init.h>
#include <modulemgr_kernel.h>
#include <modulemgr_nids.h>
#include <modulemgr_options.h>
#include <sysmem_kernel.h>
#include <threadman_kernel.h>

#include "loadModuleChecks_inline.h"
#include "modulemgr_int.h"

#define GET_MCB_STATUS(status)  (status & 0xF)
#define SET_MCB_STATUS(v, m)    (v = (v & ~0xF) | m)

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

static SceBool canOverflowOccur(u32 s1, u32 s2)
{
    return ((s1 + s2) < s2);
}

// sub_00000000
static s32 _EpilogueModule(SceModule *pMod)
{
    void *pCurEntry;
    void *pLastEntry;
    s32 status;
    
    pCurEntry = pMod->entTop;
    pLastEntry = pMod->entTop + pMod->entSize;
    status = SCE_ERROR_OK;
    
    while (pCurEntry < pLastEntry) {
        SceResidentLibraryEntryTable *pCurTable = (SceResidentLibraryEntryTable *)pCurEntry;
        if (pCurTable->attribute & SCE_LIB_IS_SYSLIB)
            continue;
        
        status = sceKernelCanReleaseLibrary(pCurTable); //0x00000048
        if (status != SCE_ERROR_OK)
            return status;
        
        pCurEntry += pCurTable->len * sizeof(void *); 
    }
    if (pMod->stubTop != (void *)-1)
        sceKernelUnLinkLibraryEntries(pMod->stubTop, pMod->stubSize); //0x00000080
    
    _ModuleReleaseLibraries(pMod); //0x00000088
    return status;
}

// 0x000000B0
static s32 _UnloadModule(SceModule *pMod)
{
    u32 modStat;
    
    modStat = GET_MCB_STATUS(pMod->status);
    if (modStat < MCB_STATUS_LOADED || (modStat >= MCB_STATUS_STARTING && modStat != MCB_STATUS_STOPPED))
        return SCE_ERROR_KERNEL_MODULE_CANNOT_REMOVE;

    sceKernelMemset32((void *)pMod->textAddr, 0x4D, UPALIGN4(pMod->textSize)); // 0x00000110
    sceKernelMemset((void *)(pMod->textAddr + pMod->textSize), -1, pMod->dataSize + pMod->bssSize); //0x00000130
    
    sceKernelIcacheInvalidateAll(); //0x00000138
    sceKernelReleaseModule(pMod); //0x00000140
    
    if ((pMod->status & 0x1000) == 0) //0x00000150
        sceKernelFreePartitionMemory(pMod->memId); //0x00000168
    
    sceKernelDeleteModule(pMod); //0x00000158

    return SCE_ERROR_OK;
}

// 0x00000178
static s32 exe_thread(SceSize args, void *argp)
{
    SceModuleMgrParam *modParams;
    SceLoadCoreExecFileInfo execInfo;
    s32 status;

	(void)args;
    
    status = SCE_ERROR_OK;
    modParams = (SceModuleMgrParam *)argp;
	pspClearMemory32(&execInfo, sizeof execInfo); // 0x000001A8
    
    SceModule *mod = modParams->pMod;
    
    switch (modParams->modeStart) { // 0x000001D0
    case CMD_LOAD_MODULE:
        if (!mod) {
            mod = sceKernelCreateModule(); //0x0000048C
            modParams->pMod = mod;
            
            if (!mod)
                break; // 0x000004A0
        }
        //0x000001E0
        modParams->execInfo = &execInfo;
        status = _LoadModule(modParams); //0x000001E4
            
        sceKernelChangeThreadPriority(SCE_KERNEL_TH_SELF, SCE_KERNEL_MODULE_INIT_PRIORITY); // 0x000001F4
            
        if (status < SCE_ERROR_OK) { //0x000001FC
            modParams->returnId[0] = status; //0x00000480
            if (modParams->pMod != NULL) //0x47C
                sceKernelDeleteModule(modParams->pMod);
            break;
        }

        modParams->returnId[0] = modParams->pMod->modId; //0x0000020C
        if (modParams->modeFinish == CMD_LOAD_MODULE) //0x00000214
            break;
    // 0x0000021C
    case CMD_RELOCATE_MODULE:
        if (mod == NULL) {
            mod = sceKernelCreateModule(); //0x00000448
            modParams->pMod = mod;
                
            // 0x00000454
            if (mod == NULL)
                break;

            SET_MCB_STATUS(mod->status, MCB_STATUS_LOADED);
            sceKernelRegisterModule(mod); //0x0000046C
        }
        if (modParams->execInfo == NULL) {
			pspClearMemory32(&execInfo, sizeof execInfo); // 0x00000238
			modParams->execInfo = &execInfo;
        }
            
        status = _RelocateModule(modParams); //0x00000244
        if (status < SCE_ERROR_OK) {
            modParams->returnId[0] = status; //0x0000042C
                
            if (mod == NULL) //0x00000428
               break;
                
            //0x00000430
            sceKernelReleaseModule(mod);
            sceKernelDeleteModule(mod);
            break;
        }
        modParams->returnId[0] = modParams->pMod->modId; //0x00000260        
        if (modParams->modeFinish == CMD_RELOCATE_MODULE) //0x00000268
            break;
    //0x00000270
    case CMD_START_MODULE:
        mod = sceKernelGetModuleFromUID(modParams->modId); //0x00000270
        if (mod == NULL && (mod = sceKernelFindModuleByUID(modParams->modId)) == NULL) //0x00000400
            modParams->returnId[0] = SCE_ERROR_KERNEL_UNKNOWN_MODULE; //0x00000420
        else {
            status = _StartModule(modParams, mod, modParams->argSize, modParams->argp, modParams->pStatus); //0x00000290
            if (status == SCE_ERROR_OK)
                modParams->returnId[0] = modParams->pMod->modId; //0x000003FC
            else if (status == 1)
                modParams->returnId[0] = 0; //0x000002A4
            else
                modParams->returnId[0] = status; //0x000002B0   
        }
        if (status < SCE_ERROR_OK || modParams->modeFinish == CMD_START_MODULE) //0x000002B4 & 0x000002C0
            break;
    //0x000002C8
    case CMD_STOP_MODULE:
        if (mod == NULL) { //0x000002C8
            mod = sceKernelGetModuleFromUID(modParams->modId);
            if (mod == NULL) { //0x000003D0
                modParams->returnId[0] = SCE_ERROR_KERNEL_UNKNOWN_MODULE; //0x000003AC
                break;
            }
        }
        status = _StopModule(modParams, mod, modParams->modeStart, modParams->callerModId, modParams->argSize, 
                modParams->argp, modParams->pStatus); //0x000002E8
            
        if (status == SCE_ERROR_OK) //0x000002F0
            modParams->returnId[0] = 0;
        else if (status == 1)
            modParams->returnId[0] = modParams->pMod->modId; //0x000002FC
        else
            modParams->returnId[0] = status; //0x00000308
        
        if (status < SCE_ERROR_OK || modParams->modeFinish == CMD_STOP_MODULE) //0x0000030C & 0x00000318
            break;
    
    //0x00000320
    case CMD_UNLOAD_MODULE:
        mod = sceKernelGetModuleFromUID(modParams->modId); //0x00000320
        if (mod == NULL) { // 0x00000328
            modParams->returnId[0] = SCE_ERROR_KERNEL_UNKNOWN_MODULE; //0x000003AC
            break;
        }
        status = _UnloadModule(mod); //0x00000330
        if (status < SCE_ERROR_OK) //0x00000338
            modParams->returnId[0] = status;
        else
            modParams->returnId[0] = modParams->pMod->modId; //0x00000348

        break;       
    }
    // 00000350
    if (modParams->eventId != 0) {
        sceKernelChangeThreadPriority(SCE_KERNEL_TH_SELF, 1); //0x00000374
        sceKernelSetEventFlag(modParams->eventId, 1); //0x00000380
    }
    return SCE_ERROR_OK;
}

// 0x0000501C
s32 ModuleMgrRebootPhase(s32 argc, void *argp)
{
    (void)argc;
    (void)argp;
        
    return SCE_ERROR_OK;
}

// 0x00005024
s32 ModuleMgrRebootBefore(s32 argc, void *argp) 
{
    (void)argc;
    (void)argp;
    
    return sceKernelSuspendThread(g_ModuleManager.threadId); //0x00005034
}

// 0x00005048
s32 ModuleMgrInit(SceSize argc, void *argp) 
{
    (void)argc;
    (void)argp;
    
    ChunkInit();
    
    g_ModuleManager.threadId = sceKernelCreateThread("SceKernelModmgrWorker", (SceKernelThreadEntry)exe_thread, 
            SCE_KERNEL_MODULE_INIT_PRIORITY, 0x4000, 0, NULL); // 0x00005078
    g_ModuleManager.semaId = sceKernelCreateSema("SceKernelModmgr", SCE_KERNEL_SA_THFIFO, 1, 1, NULL); // 0x0000509C

    g_ModuleManager.eventId = sceKernelCreateEventFlag("SceKernelModmgr", SCE_KERNEL_EW_AND, 0, NULL); // 0x000050B8
    
    g_ModuleManager.userThreadId = -1; // 0x000050DC
    g_ModuleManager.unk16 = -1; // 0x000050D0
    
    g_ModuleManager.unk20 = &g_ModuleManager.unk20; // 0x000050D8
    g_ModuleManager.unk24 = &g_ModuleManager.unk20; //0x000050F0
    g_ModuleManager.npDrmGetModuleKeyFunction = NULL; // 0x000050E0
    g_ModuleManager.unk36 = 0; // 0x000050D4
    
    return SCE_KERNEL_RESIDENT;
}

// Subroutine ModuleMgrForUser_CDE1C1FE - Address 0x00005B10
s32 ModuleMgrForUser_CDE1C1FE()
{
    // TODO: Figure out structure member unk36 of SceModuleManagerCB
}

// Subroutine ModuleMgrForKernel_A40EC254 - Address 0x00005B6C
s32 sceKernelSetNpDrmGetModuleKeyFunction(s32 (*function)(s32 fd, void *, void *))
{
    g_ModuleManager.npDrmGetModuleKeyFunction = function;
}

// Subroutine ModuleMgrForKernel_C3DDABEF - Address 0x00005B7C
s32 sceKernelNpDrmGetModuleKey(s32 fd, void *arg2, void *arg3)
{
    s32 status;
    
    if (arg2 == NULL || arg3 == NULL)
        return SCE_ERROR_KERNEL_ILLEGAL_ADDR;
    
    if (g_ModuleManager.npDrmGetModuleKeyFunction == NULL)
        return SCE_ERROR_KERNEL_ERROR;
    
    status = g_ModuleManager.npDrmGetModuleKeyFunction(fd, arg2, arg3); // 0x00005BB0
    
    return (status < SCE_ERROR_OK) ? status : SCE_ERROR_OK; // 0x00005BBC
}

// Subroutine ModuleMgrForKernel_1CFFC5DE - Address 0x00005BD0
s32 sceKernelModuleMgrMode(s32 mode)
{
    (void)mode;
    
    return SCE_ERROR_OK;
}

// sub_00005C4C
static s32 _LoadModule(SceModuleMgrParam *modParams)
{
    s64 pos;
    s32 status;
    SceModule *pMod;
    SceLoadCoreExecFileInfo *pExecInfo;
    
    pMod = modParams->pMod;
    if (pMod == NULL || GET_MCB_STATUS(pMod->status) != MCB_STATUS_NOT_LOADED) // 0x00005C8C & 0x00005CA0
        return SCE_ERROR_KERNEL_ERROR;
    
    SET_MCB_STATUS(pMod->status, MCB_STATUS_LOADING); // 0x00005CBC
    
    pExecInfo = modParams->execInfo;
    memset(pExecInfo, 0, sizeof(SceLoadCoreExecFileInfo)); // 0x00005CC8
    pExecInfo->apiType = modParams->apiType;
            
    pos = sceIoLseek(modParams->fd, 0, SCE_SEEK_CUR); //0x00005CE8
    
    SceUID partitionId = sceKernelAllocPartitionMemory(SCE_KERNEL_PRIMARY_KERNEL_PARTITION, "SceModmgrLMTmp", 
            SCE_KERNEL_SMEM_High, 512, 0); // 0x00005D0C
    if (partitionId < SCE_ERROR_OK)
        return partitionId;
    
    s32 data = 0; // 0x00005C84
    SceUID fd = modParams->fd;
    void *pBlock = sceKernelGetBlockHeadAddr(partitionId); // 0x00005D20
    do {
        status = sceIoRead(fd, pBlock, 512); // 0x00005D34
        if (status <= 0) { // 0x00005D3C
            // TODO: Insert this if partitionId can be 0
            // if (partitionID == 0)
            //        return (status < 0) ? status : SCE_ERROR_KERNEL_FILE_READ_ERROR; // 0x00006728
            ClearFreePartitionMemory(partitionId); // 0x000067BC - 0x000067F0
            return (status < 0) ? status : SCE_ERROR_KERNEL_FILE_READ_ERROR; // 0x00006728
        }
        sceIoLseek(fd, pos, SCE_SEEK_SET); // 0x00005D50
        
        if (data != 0 || modParams->unk124 != 0) // 0x00005D5C & 0x00005D68
            break;
        
		// If we get here, we always break outside the loop
        status = _CheckOverride(modParams->apiType, pBlock, &data); //0x00005D74
        if (status == 0) // 0x00005D7C
            break;
        
        if (data < 0) // 0x00005D84
            while (1) { } // 0x00005D98
        
        fd = data;
        modParams->unk124 = 1; // 0x00005D90
    }
    while (1); // 0x00005D90
    
    s32 execCodeOffset;
	status = _CheckSkipPbpHeader(modParams, fd, pBlock, &execCodeOffset, modParams->apiType); // 0x00005DB0
    if (status < 0) { // 0x00005DB8
        ClearFreePartitionMemory(partitionId); // 0x0000674C - 0x00006780
        if (data != 0) // 0x0000678C
            sceIoClose(data); // 0x0000679C
        
        return status;
    }
    if (status > 0) { // 0x00005DC0
        pos = sceIoLseek(data, 0, SCE_SEEK_CUR); // 0x00005DD0
        status = sceIoRead(data, pBlock, 512); // 0x00005DE8
        if (status <= 0) { // 0x00005DF0
            ClearFreePartitionMemory(partitionId); // 0x000066C0 - 0x00006708
            if (data != 0) // 0x0000678C
                sceIoClose(data); // 0x0000672C
            
            return (status < 0) ? status : SCE_ERROR_KERNEL_FILE_READ_ERROR; // 0x00006724
        }
        sceIoLseek(data, pos, SCE_SEEK_SET); // 0x00005E04
    }
    pExecInfo->modeAttribute = SCE_EXEC_FILE_NO_HEADER_COMPRESSION; // 0x00005E10
    if (modParams->unk124 & 0x1) // 0x00005E1C
        pExecInfo->isSignChecked = SCE_TRUE; 
    pExecInfo->secureInstallId = modParams->secureInstallId; // 0x00005E28
    char *secInstallId = modParams->secureInstallId; // 0x00005E38
    
    status = sceKernelCheckExecFile(pBlock, pExecInfo); // 0x00005E34
    if (status < SCE_ERROR_OK) { // 0x00005E3C
        _FreeMemoryResources(data, partitionId, modParams, pExecInfo);
        return status;
    }
    
    /* Calculate size of executable. */
    if (pExecInfo->execSize == 0) { // 0x00005E48
        pos = sceIoLseek(data, 0, SCE_SEEK_CUR); // 0x00006678
        u64 endPos = sceIoLseek(data, 0, SCE_SEEK_END); // 0x00006694
        sceIoLseek(data, pos, SCE_SEEK_SET); // 0x000066AC
        
        pExecInfo->execSize = endPos - pos; // 0x000066BC
    }
    
    if (pExecInfo->elfType == SCE_EXEC_FILE_TYPE_INVALID_ELF || pExecInfo->elfType == 0) { // 0x00005E5C
        _FreeMemoryResources(data, partitionId, modParams, pExecInfo);
        return SCE_ERROR_KERNEL_UNSUPPORTED_PRX_TYPE;
    }
    // TODO: Does this make sense?
    pExecInfo->isCompressed = (pExecInfo->execAttribute & SCE_EXEC_FILE_COMPRESSED) ? SCE_FALSE : SCE_TRUE; // 0x00005E70
    
    if (modParams->memBlockId == 0) { // 0x00005E78
        if (modParams->unk104 == 0) { // 0x00006510
            status = _PartitionCheck(modParams, pExecInfo); // 0x0000651C
            pBlock = status; // 0x00006528 -- continue at loc_00005ECC
            if (status != SCE_ERROR_OK) { // 0x00006524
                _FreeMemoryResources(data, partitionId, modParams, pExecInfo);
                return SCE_ERROR_KERNEL_UNSUPPORTED_PRX_TYPE;
            }   
        } else {
            SceUID partId;
            SceUID memBlkId;
            SceSysmemMemoryBlockInfo blkInfo;
            blkInfo.size = sizeof(SceSysmemMemoryBlockInfo);
            status = sceKernelQueryMemoryBlockInfo(modParams->unk104, &blkInfo); // 0x00006538
            if (status >= SCE_ERROR_OK) { // 0x00006540
                status = sceKernelQueryMemoryInfo(blkInfo.addr, &partId, &memBlkId);
                if (status >= SCE_ERROR_OK) {
                    modParams->unk108 = partId;
                    modParams->unk112 = blkInfo.memSize;
                }
            }
            if (pExecInfo->isKernelMod) { // 0x0000656C
                status = _CheckKernelOnlyModulePartition(modParams->unk108); // 0x00006574 - 0x0000659C
                if (status < SCE_ERROR_OK) { // 0x00006608 & 0x00006618
                    _FreeMemoryResources(data, partitionId, modParams, pExecInfo);
                    return status;
                }
                pExecInfo->partitionId = modParams->unk108; // 0x00006630
                if (modParams->mpIdData != 0) { // 0x00006638
                    status = _CheckKernelOnlyModulePartition(modParams->mpIdData); // 0x00006640 - 0x0000666C
                    if (status < SCE_ERROR_OK) {
                        _FreeMemoryResources(data, partitionId, modParams, pExecInfo);
                        return status;
                    }
                }
            }
            else {
                status = _CheckUserModulePartition(modParams->unk108); // 0x000065F8 - 0x00006620
                if (status < SCE_ERROR_OK) { // 0x00006608 & 0x00006618
                    _FreeMemoryResources(data, partitionId, modParams, pExecInfo);
                    return status;
                }
                pExecInfo->partitionId = modParams->unk108; // 0x000065AC
                if (modParams->mpIdData != 0) { // 0x000065B0
                    status = _CheckUserModulePartition(modParams->mpIdData); // 0x000065BC - 0x000065E4
                    if (status < SCE_ERROR_OK) {
                        _FreeMemoryResources(data, partitionId, modParams, pExecInfo);
                        return status;
                    }
                }
            }
        }
    } else {
        if (pExecInfo->isKernelMod) { // 0x00005E84
            _FreeMemoryResources(data, partitionId, modParams, pExecInfo);
            return SCE_ERROR_KERNEL_PARTITION_MISMATCH;
        }
        SceUID partId;
        SceUID memBlkId;
        SceSysmemMemoryBlockInfo blkInfo;
        blkInfo.size = sizeof(SceSysmemMemoryBlockInfo);
        status = sceKernelQueryMemoryBlockInfo(modParams->memBlockId, &blkInfo); // 0x00005EA8
        if (status >= SCE_ERROR_OK) { // 0x00006540
            status = sceKernelQueryMemoryInfo(blkInfo.addr, &partId, &memBlkId);
            if (status >= SCE_ERROR_OK) {
                modParams->unk108 = partId;
                modParams->unk112 = blkInfo.memSize;
            }
        }      
    }
    pExecInfo->partitionId = modParams->unk108; // 0x00005EC8
    
    if (!(pExecInfo->execAttribute & SCE_EXEC_FILE_COMPRESSED)) // 0x00005ED4
        pExecInfo->maxAllocSize = pspMax(pExecInfo->execSize, pExecInfo->largestSegSize); // 0x00006504
    else if (!(pExecInfo->execAttribute & SCE_EXEC_FILE_GZIP_OVERLAP)) { // 0x00005EF0
        pExecInfo->maxAllocSize = pspMax(pExecInfo->largestSegSize, pExecInfo->decSize);
    } else
        pExecInfo->maxAllocSize = pExecInfo->decSize + pExecInfo->overlapSize * 256; // 0x00005F04
    
    if (modParams->memBlockId != 0) { // 0x00005F0C
        // TODO: Check back
        u8 overFlow = canOverflowOccur((u32)modParams->memBlockOffset, pExecInfo->maxAllocSize);
        if ((modParams->memBlockOffset >> 32) > 0 || ((modParams->memBlockOffset >> 32) == 0 && overFlow)
               || (((modParams->memBlockOffset >> 32) + overFlow) == 0 && 
                (modParams->unk112 < (u32)modParams->memBlockOffset + pExecInfo->maxAllocSize))) { // 0x00005F3C
            _FreeMemoryResources(data, partitionId, modParams, pExecInfo);
            return SCE_ERROR_KERNEL_ERROR;
        } 
    }
    // 0x00005F50 & 0x00005F64
    if (modParams->unk104 != 0 && modParams->unk112 < pExecInfo->maxAllocSize) {
        _FreeMemoryResources(data, partitionId, modParams, pExecInfo);
        return SCE_ERROR_KERNEL_ERROR;
    }
    
    // 0x00005F90
    switch (pExecInfo->elfType) {
    case SCE_EXEC_FILE_TYPE_INVALID_ELF: 
    case SCE_EXEC_FILE_TYPE_PRX_2: // 0x000064A8
         _FreeMemoryResources(data, partitionId, modParams, pExecInfo);
        return SCE_ERROR_KERNEL_UNSUPPORTED_PRX_TYPE;
    case SCE_EXEC_FILE_TYPE_PRX: // 0x00005F98
        if (modParams->memBlockId != 0) // 0x00005F98
            pExecInfo->decompressionMemId = modParams->memBlockId;
        else if (modParams->unk104 != 0) // 0x0000642C
            pExecInfo->decompressionMemId = modParams->unk104; // 0x00006438
        else {
            u8 blkAllocType = modParams->position;
            u32 addr = 0; // 0x00006430      
            if (pExecInfo->maxSegAlign > 0x100) { // 0x00006448
                if (modParams->position == SCE_KERNEL_SMEM_Low) {
                    blkAllocType = SCE_KERNEL_SMEM_LOWALIGNED; // 0x0000649C
                    addr = pExecInfo->maxSegAlign;
                }
                else if (modParams->position == SCE_KERNEL_SMEM_High) { // 0x00006460
                    blkAllocType = SCE_KERNEL_SMEM_HIGHALIGNED; // 0x00006490
                    addr = pExecInfo->maxSegAlign;
                }
            }
            pExecInfo->decompressionMemId = sceKernelAllocPartitionMemory(pExecInfo->partitionId, "SceModmgrLMFileBufferPRX", 
                    blkAllocType, pExecInfo->maxAllocSize, addr); // 0x00006474
            if (pExecInfo->decompressionMemId < SCE_ERROR_OK) { // 0x0000647C
                _FreeMemoryResources(data, partitionId, modParams, pExecInfo);
                return pExecInfo->decompressionMemId;
            }
        }
        break;
    case SCE_EXEC_FILE_TYPE_ELF: // 0x000064B4
        if (modParams->unk104 != 0) { // 0x000064B4
             _FreeMemoryResources(data, partitionId, modParams, pExecInfo);
             return SCE_ERROR_KERNEL_ERROR;
        }
        pExecInfo->topAddr &= 0xFFFFFF00; // 0x000064D0
        pExecInfo->decompressionMemId = sceKernelAllocPartitionMemory(pExecInfo->partitionId, "SceModmgrLMFileBufferELF", 
                    SCE_KERNEL_SMEM_Addr, pExecInfo->maxAllocSize, pExecInfo->topAddr); // 0x00006474
            if (pExecInfo->decompressionMemId < SCE_ERROR_OK) { // 0x0000647C
                _FreeMemoryResources(data, partitionId, modParams, pExecInfo);
                return pExecInfo->decompressionMemId;
            }
    default: // 0x00005F78
         _FreeMemoryResources(data, partitionId, modParams, pExecInfo);
        return SCE_ERROR_KERNEL_UNSUPPORTED_PRX_TYPE;
    }
    
    void *baseAddr = sceKernelGetBlockHeadAddr(pExecInfo->decompressionMemId); // 0x0000641C
    if (modParams->memBlockOffset != 0) // 0x00005FB0
        baseAddr += (u32)modParams->memBlockOffset; // 0x00005FC4
    pExecInfo->fileBase = baseAddr; // 0x00005FCC
    
    pExecInfo->maxAllocSize = UPALIGN256(pExecInfo->maxAllocSize); // 0x00005FE0
    if (partitionId > SCE_ERROR_OK) { // 0x00005FDC
        status = ClearFreePartitionMemory(partitionId); // 0x00005FE4 - 0x00006030
        pBlock = (status < 0) ? (void *)status : NULL; // 0x00006034
        partitionId = 0; // 0x00006038
    }
    // 0x0000603C
    if (pExecInfo->execAttribute & SCE_EXEC_FILE_COMPRESSED) { // 0x00006044
        void *buf;
        SceSize size;
        pExecInfo->topAddr = pExecInfo->fileBase; // 0x00006050
        if (modParams->memBlockId != 0) { // 0x00006058
            partitionId = 0; // 0x00006360
            baseAddr = sceKernelGetBlockHeadAddr(modParams->memBlockId); // 0x0000635C
            modParams->unk120 = (modParams->unk112 - UPALIGN64(pExecInfo->execSize)) + baseAddr; // 0x00006358
            size = pExecInfo->execSize;
            buf = modParams->unk120;
        } else if (modParams->unk104 != 0) { // 0x00006064
            partitionId = 0; // 0x00006324
            baseAddr = sceKernelGetBlockHeadAddr(modParams->unk104); // 0x00006320
            if (modParams->position != SCE_KERNEL_SMEM_Low && modParams->position != SCE_KERNEL_SMEM_High) { // 0x00006330
                _FreeMemoryResources(data, partitionId, modParams, pExecInfo);
                return (s32)pBlock;
            }
            modParams->unk120 = (modParams->unk112 - UPALIGN64(pExecInfo->execSize)) + baseAddr; // 0x00006358
            size = pExecInfo->execSize;
            buf = modParams->unk120;
        } else if (pExecInfo->execAttribute & SCE_EXEC_FILE_GZIP_OVERLAP) { // 0x0000606C
            baseAddr = sceKernelGetBlockHeadAddr(pExecInfo->decompressionMemId); // 0x00006074
            size = pExecInfo->execSize;
            buf = (pExecInfo->maxAllocSize = UPALIGN64(pExecInfo->execSize)) + baseAddr; // 0x00006098
        } else {
            s32 partId = sceKernelAllocPartitionMemory(pExecInfo->partitionId, "SceModmgrGzipBuffer", SCE_KERNEL_SMEM_High, 
                    pExecInfo->execSize, 0); // 0x000062F8
            if (partId < SCE_ERROR_OK) {
                _FreeMemoryResources(data, partitionId, modParams, pExecInfo);
                return partId;
            }
            buf = sceKernelGetBlockHeadAddr(partId); // 0x0000630C
            size = pExecInfo->execSize; // 
        }
        SceOff curPos = sceIoLseek(data, 0, SCE_SEEK_CUR); // 0x000060A8
        
        status = sceIoRead(data, buf, size); // 0x000060C0
        if (status >= SCE_ERROR_OK) // 0x000060C8
            status = (status == size) ? SCE_ERROR_OK : SCE_ERROR_KERNEL_ERROR;
        pBlock = status; // 0x000060E8
        
        sceIoLseek(data, curPos, SCE_SEEK_SET); // 0x000060F0
        if (status < SCE_ERROR_OK) { // 0x000060F8
            _FreeMemoryResources(data, partitionId, modParams, pExecInfo);
            return status;
        }
        // 0x0000613C
        pExecInfo->modeAttribute = SCE_EXEC_FILE_DECRYPT; // 0x00006100
        if (modParams->unk124 & 0x1) // 0x0000610C
            pExecInfo->isSignChecked = SCE_TRUE;
        pExecInfo->secureInstallId = secInstallId; // 0x0000612C
        
        status = sceKernelCheckExecFile(buf, pExecInfo); // 0x00006128
        if (status < SCE_ERROR_OK) {
            _FreeMemoryResources(data, partitionId, modParams, pExecInfo);
            return status;
        }
        if (partitionId > 0) // 0x0000613C
            ClearFreePartitionMemory(partitionId); 
    } else if (pExecInfo->isCompressed) { // 0x00006384
        SceOff curPos = sceIoLseek(data, 0, SCE_SEEK_CUR); // 0x000063BC
        
        status = sceIoRead(data, pExecInfo->fileBase, pExecInfo->execSize); // 0x000063D4
        if (status >= SCE_ERROR_OK) // 0x000063DC
            status = (status == pExecInfo->execSize) ? SCE_ERROR_OK : SCE_ERROR_KERNEL_ERROR;
        
        sceIoLseek(data, curPos, SCE_SEEK_SET); // 0x00006404
        if (status < SCE_ERROR_OK) {
            _FreeMemoryResources(data, partitionId, modParams, pExecInfo);
            return status;
        }
        pExecInfo->modeAttribute = SCE_EXEC_FILE_DECRYPT; // 0x00006418
    } else {
        pExecInfo->isCompressed = SCE_TRUE; // 0x0000638C
        sceKernelMemmoveWithFill(pExecInfo->fileBase, NULL, pExecInfo->execSize, 0); // 0x00006398
    }
    if (data != 0) // 0x00006194
        sceIoClose(data);
    
    SET_MCB_STATUS(pMod->status, MCB_STATUS_LOADED); // 0x000061B0
    sceKernelRegisterModule(pMod);
    
    return SCE_ERROR_OK;
}

static void _FreeMemoryResources(SceUID fd, SceUID partitionId, SceModuleMgrParam *pModParams, 
        SceLoadCoreExecFileInfo *pExecInfo)
{
    if (fd > 0) // 0x00006204
        sceIoClose(fd); // 0x000062D8
        
    ClearFreePartitionMemory(partitionId); // 0x0000620C - 0x00006258
    if (pModParams->memBlockId != 0 || pModParams->unk104 != 0 || 
            pExecInfo == NULL || pExecInfo->decompressionMemId <= 0) // 0x00006260 & 0x0000626C & 0x00006274 & 0x00006280
        return;
        
    ClearFreePartitionMemory(pExecInfo->decompressionMemId); // 0x00006284 - 0x000062CC
}

static s32 ClearFreePartitionMemory(s32 partitionId)
{
    s32 status;
    
    if (partitionId <= 0)
        return 0;
    
    SceSysmemMemoryBlockInfo blkInfo;
    blkInfo.size = sizeof(SceSysmemMemoryBlockInfo);
    status = sceKernelQueryMemoryBlockInfo(partitionId, &blkInfo); // 0x000066D0
    if (status < SCE_ERROR_OK)
        return status;
    
    sceKernelMemset(blkInfo.addr, 0, blkInfo.memSize); // 0x00006778
    status = sceKernelFreePartitionMemory(partitionId); // 0x00006780
    
    return status;
}

static s32 _CheckUserModulePartition(SceUID memoryPartitionId) 
{
    s32 status;
    SceSysmemPartitionInfo partitionInfo;
    
    partitionInfo.size = sizeof(SceSysmemPartitionInfo);
    status = sceKernelQueryMemoryPartitionInfo(memoryPartitionId, &partitionInfo);
    if (status < SCE_ERROR_OK || !(partitionInfo.attr & 3))
        return SCE_ERROR_KERNEL_PARTITION_MISMATCH;
    return SCE_ERROR_OK;
}

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
static s32 _RelocateModule(SceModuleMgrParam *modParams)
{
	s32 status;
	SceLoadCoreExecFileInfo *pExecInfo;
	
	pExecInfo = modParams->execInfo;
	pExecInfo->apiType = modParams->apiType; // 0x00006844
	pExecInfo->unk12 = modParams->unk100; // 0x00006858

	if (pExecInfo->elfType == SCE_EXEC_FILE_TYPE_INVALID_ELF || pExecInfo->elfType == 0) { // 0x00006854
		pExecInfo->modeAttribute = SCE_EXEC_FILE_NO_HEADER_COMPRESSION; // 0x00006860
		pExecInfo->fileBase = modParams->unk64; // 0x00006874
		if (modParams->unk124 & 0x1) // 0x00006870
			pExecInfo->isSignChecked = SCE_TRUE; // 0x00006878

		pExecInfo->secureInstallId = modParams->secureInstallId; //0x00006880
		status = sceKernelCheckExecFile(pExecInfo->fileBase, pExecInfo); // 0x00006884
		if (status < SCE_ERROR_OK) { // 0x0000688C

		}
	}
}

// sub_00006F80
static s32 _ModuleReleaseLibraries(SceModule *pMod)
{
    void *pCurEntry;
    void *pLastEntry;
    
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
s32 _StartModule(SceModuleMgrParam *modParams, SceModule *pMod, SceSize argSize, void *argp, s32 *pStatus)
{
    s32 status;
    
    if (GET_MCB_STATUS(pMod->status) != MCB_STATUS_RELOCATED) // 0x00007034
        return SCE_ERROR_KERNEL_ERROR;
    
    SET_MCB_STATUS(pMod->status, MCB_STATUS_STARTING); //0x0000704C
    
    status = _PrologueModule(modParams, pMod); // 0x00007048
    if (status < SCE_ERROR_OK) {
        SET_MCB_STATUS(pMod->status , MCB_STATUS_RELOCATED); //0x00007138
        return SCE_ERROR_KERNEL_ERROR;
    }
    
    if (pMod->userModThid != -1) { //0x0000705C
        g_ModuleManager.userThreadId = pMod->userModThid; // 0x0000706C
        
        status = sceKernelStartThread(pMod->userModThid, argSize, argp); //0x00007078
        if (status == SCE_ERROR_OK) // 0x00007080
            status = sceKernelWaitThreadEnd(pMod->userModThid, NULL); // 0x00007114
        
        g_ModuleManager.userThreadId = -1;
        sceKernelDeleteThread(pMod->userModThid); // 0x00007094
        pMod->userModThid = -1;
    }
    
    if (pStatus != NULL) // 0x000070A0
        *pStatus = status;
    
    if (status != SCE_ERROR_OK) { // 0x000070A8
        _EpilogueModule(pMod);
        SET_MCB_STATUS(pMod->status, MCB_STATUS_STOPPED); //0x00007100
        _UnloadModule(pMod); // 0x00007104
        
        return status;
    }
    
    SET_MCB_STATUS(pMod->status, MCB_STATUS_STARTED); //0x000070B8
    return SCE_ERROR_OK;
}

// Subroutine sub_0000713C - Address 0x0000713C
s32 _StopModule(SceModuleMgrParam *modParams, SceModule *pMod, s32 startOp, SceUID modId, SceSize argSize, 
                void *argp, s32 *pStatus)
{
    s32 status;
    s32 threadPriority;
    SceSize stackSize;
    SceUInt threadAttr;
    
    switch (GET_MCB_STATUS(pMod->status)) { // 0x000071A4
    case MCB_STATUS_NOT_LOADED: MCB_STATUS_LOADING: MCB_STATUS_STARTING: MCB_STATUS_UNLOADED:
        return SCE_ERROR_KERNEL_ERROR;
    case MCB_STATUS_LOADED: MCB_STATUS_RELOCATED:
        // 0x000071AC
        return SCE_ERROR_KERNEL_MODULE_NOT_STARTED;
    case MCB_STATUS_STARTED:
        // 0x000071F4
        if ((s32)pMod->moduleStop != -1) { // 0x000071FC
            threadPriority = modParams->threadPriority;
            if (threadPriority == 0) { //0x00007208
                threadPriority = (pMod->moduleStopThreadPriority == -1) ? SCE_KERNEL_MODULE_INIT_PRIORITY 
                        : pMod->moduleStopThreadPriority; //0x0000721C
            }
            stackSize = modParams->stackSize;
            if (stackSize == 0)
                stackSize = (pMod->moduleStopThreadStacksize == -1) ? 0 : pMod->moduleStopThreadStacksize; // 0x00007238
            
            threadAttr = (pMod->moduleStopThreadAttr == -1) ? SCE_KERNEL_TH_DEFAULT_ATTR : pMod->moduleStopThreadAttr; //0x00007250
            if (modParams->threadAttr != SCE_KERNEL_TH_DEFAULT_ATTR)
                threadAttr |= (modParams->threadAttr & 0xF06000); // 0x00007260           
            threadAttr &= ~0xF0000000; //0x0000726C
            
            if (!(pMod->status & SCE_MODULE_USER_MODULE)) //0x00007268
                stackSize = (stackSize == 0) ? SCE_KERNEL_TH_DEFAULT_SIZE : 0;
            else {
                stackSize = (stackSize == 0) ? 0x40000 : 0; //0x00007280
            
                if ((pMod->attribute & 0x1E00) == SCE_MODULE_VSH) //0x00007284
                    threadAttr |= SCE_KERNEL_TH_VSH_MODE;
                else if ((pMod->attribute & 0x1E00) == SCE_MODULE_APP) // 0x00007290
                    threadAttr |= SCE_KERNEL_TH_APP_MODE;
                else if ((pMod->attribute & 0x1E00) == SCE_MODULE_USB_WLAN) // 0x0000729C
                    threadAttr |= SCE_KERNEL_TH_USB_WLAN_MODE;
                else if ((pMod->attribute & 0x1E00) == SCE_MODULE_MS) // 0x000072A8
                    threadAttr |= SCE_KERNEL_TH_MS_MODE;
                else
                    threadAttr |= SCE_KERNEL_TH_USER_MODE;
            }
            SceKernelThreadOptParam threadOptions;
            threadOptions.size = sizeof(SceKernelThreadOptParam);
            threadOptions.stackMpid = (modParams->threadMpIdStack == 0) ? pMod->mpIdData : modParams->threadMpIdStack; // 0x000072C0 & 0x000072CC
            if (threadOptions.stackMpid != 0) { //0x000072D4
                if (pMod->status & SCE_MODULE_USER_MODULE) // 0x000072E0
                    status = _CheckUserModulePartition(threadOptions.stackMpid);
                else
                    status = _CheckKernelOnlyModulePartition(threadOptions.stackMpid);
                
                if (status < SCE_ERROR_OK)
                    return status;
            }
            s32 oldGp = pspSetGp(pMod->gpValue); //0x0000732C
            
            status = sceKernelCreateThread("SceKernelModmgrStop", pMod->moduleStop, threadPriority, stackSize, 
                    threadAttr, &threadOptions); // 0x00007348
            pMod->userModThid = status;
            
            pspSetGp(oldGp); // 0x00007354
            
            if (status < SCE_ERROR_OK) // 0x0000735C
                return status;
            
            g_ModuleManager.userThreadId = status; // 0x00007370
            SET_MCB_STATUS(pMod->status, MCB_STATUS_STOPPING); // 0x0000737C  
            s32 stopResult = 1; // 0x00007390
            
            status = sceKernelStartThread(pMod->userModThid, argSize, argp); // 0x0000738C
            if (status == SCE_ERROR_OK) // 0x00007394
                stopResult = sceKernelWaitThreadEnd(pMod->userModThid, NULL); //0x0000747C
            g_ModuleManager.userThreadId = -1;
            
            sceKernelDeleteThread(pMod->userModThid); // 0x000073A8
            pMod->userModThid = -1;
            
            sceKernelChangeThreadPriority(SCE_KERNEL_TH_SELF, SCE_KERNEL_MODULE_INIT_PRIORITY); // 0x000073B8
            
            if (pStatus != NULL) // 0x000073C0
                *pStatus = stopResult;
            
            if (sceKernelIsToolMode()) { // 0x000073D0
                status = sceKernelDipsw(25); // 0x0000742C
                
                if (status != 1 && sceKernelGetCompiledSdkVersion() >= 0x3030000 
                        && sceKernelSegmentChecksum(pMod) != pMod->segmentChecksum)
                    pspBreak(0); // 0x00007470
            }
            if (stopResult == 1) // 0x000073E0
                return SCE_ERROR_KERNEL_MODULE_CANNOT_STOP;
            
            if (stopResult != SCE_ERROR_OK) {
                SET_MCB_STATUS(pMod->status, MCB_STATUS_STARTED); // 0x000073FC
                return stopResult;
            }
            status = _EpilogueModule(pMod); // 0x00007408
            if (status < SCE_ERROR_OK) {
                SET_MCB_STATUS(pMod->status, MCB_STATUS_STARTED);
                return status; // 0x000073FC
            }
            SET_MCB_STATUS(pMod->status, MCB_STATUS_STOPPED); //0x00007428
            return SCE_ERROR_OK; // 0x000073FC
        }
    case MCB_STATUS_STOPPING:
        // 0x000074CC
        return SCE_ERROR_KERNEL_MODULE_ALREADY_STOPPING;
    case MCB_STATUS_STOPPED:
        // 0x000074D8
        return SCE_ERROR_KERNEL_MODULE_ALREADY_STOPPED;
    default: // 0x0000718C
       return SCE_ERROR_KERNEL_ERROR;
    }
    
    return SCE_ERROR_OK;
}

// Subroutine sub_000074E4 - Address 0x000074E4 
s32 _start_exe_thread(SceModuleMgrParam *modParams)
{
    SceUID threadId;
    SceUID returnId;
    s32 status;

    threadId = sceKernelGetThreadId();
    if (threadId < 0)
        return threadId;

    if (threadId == g_ModuleManager.userThreadId) {
        Kprintf("module manager busy.\n");
        return SCE_ERROR_KERNEL_MODULE_MANAGER_BUSY;
    }

    modParams->returnId = &returnId;
    modParams->eventId = g_ModuleManager.eventId;

    status = sceKernelLockMutex(g_ModuleManager.semaId, 1, 0);
    if (status < SCE_ERROR_OK) {
        return status;
    }

    status = sceKernelStartThread(g_ModuleManager.threadId, sizeof(SceModuleMgrParam), modParams);
    if (status < SCE_ERROR_OK) {
        sceKernelUnlockMutex(g_ModuleManager.semaId, 1);
        return returnId;
    }

    sceKernelWaitEventFlag(g_ModuleManager.eventId, 1, SCE_KERNEL_EW_CLEAR_ALL | SCE_KERNEL_EW_OR, NULL, NULL);

    sceKernelUnlockMutex(g_ModuleManager.semaId, 1);
    return returnId;
}

// Subroutine sub_000076CC - Address 0x000076CC 
s32 _SelfStopUnloadModule(s32 returnStatus, const void *codeAddr, SceSize args, void *argp, s32 *pStatus, 
        const SceKernelSMOption *pOpt)
{
    SceModule *pMod;
    s32 status;
    SceModuleMgrParam modParams;
    s32 status2;

    pMod = sceKernelFindModuleByAddress(codeAddr); // 0x000076FC
    if (pMod == NULL || pMod->attribute & SCE_MODULE_ATTR_CANT_STOP) // 0x0000770C & 0x00007720
        return SCE_ERROR_KERNEL_MODULE_CANNOT_STOP;

    pspClearMemory32(&modParams, sizeof(modParams)); // 0x00007734

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

    if (pOpt == NULL) { // 0x0000776C
        modParams.threadMpIdStack = 0;
        modParams.stackSize = 0;
        modParams.threadPriority = 0;
        modParams.threadAttr = 0;
    } else {
        modParams.threadMpIdStack = pOpt->mpidstack; // 0x00007784
        modParams.stackSize = pOpt->stacksize; // 0x00007788
        modParams.threadPriority = pOpt->priority; // 0x0000778C
        modParams.threadAttr = pOpt->attribute; // 0x00007790
    }

    status = _start_exe_thread(&modParams); // 0x00007794
    if (status < 0)
        return status;

    sceKernelExitDeleteThread(returnStatus); // 0x000077A4

    return status;
}

// Subroutine sub_000077F0 - Address 0x000077F0 
s32 _StopUnloadSelfModuleWithStatus(s32 returnStatus, void *codeAddr, SceSize args, void *argp, s32 *pStatus, 
        SceKernelSMOption *pOpt)
{
    s32 oldK1;
    void *codeAddr2;
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

    if (pStatus != NULL && !pspK1StaBufOk(pStatus, sizeof(*pStatus))) { // 0x00007878
        pspSetK1(oldK1);
        return SCE_ERROR_KERNEL_ILLEGAL_ADDR;
    }

    status = _checkSMOptionConditions(pOpt); // 0x000078E4 - 0x000078F8
    if (status < 0) {
        pspSetK1(oldK1);
        return status;
    }

    if (pspK1IsUserMode()) // 0x0000791C
        codeAddr2 = sceKernelGetSyscallRA(); // 0x00007958
    else
        codeAddr2 = codeAddr;

    if (!pspK1PtrOk(codeAddr2)) { // 0x0000792C
        pspSetK1(oldK1);
        return SCE_ERROR_KERNEL_ILLEGAL_ADDR;
    }

    status = _SelfStopUnloadModule(returnStatus, codeAddr2, args, argp, pStatus, pOpt); // 0x00007948
    pspSetK1(oldK1);

    return status;
}

// sub_00007968
static s32 _ProcessModuleExportEnt(SceModule *pMod, SceResidentLibraryEntryTable *pLib)
{
    u32 i;
    SceModuleEntryThread *entryThread;
    
    if (pMod->status & 0x400) // 0x00007970
        return SCE_ERROR_OK;
    
    pMod->status |= 0x400; // 0x0000797C
    
    //0x00007988 - 0x000079F8
    for (i = 0; i < pLib->stubCount; i++) {        
         switch (pLib->entryTable[i]) { 
         case NID_MODULE_REBOOT_PHASE: //0x000079C0
             pMod->moduleRebootPhase = (SceKernelThreadEntry)pLib->entryTable[pLib->vStubCount + pLib->stubCount + i]; //0x00007C14
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
             pMod->moduleRebootBefore = (SceKernelThreadEntry)pLib->entryTable[pLib->vStubCount + pLib->stubCount + i]; //0x00007B8C
             break;
         }         
    }
    //0x00007A04 - 0x00007A6C
    for (i = 0; i < pLib->vStubCount; i++) {
         switch (pLib->entryTable[pLib->vStubCount + i]) {
         case NID_MODULE_STOP_THREAD_PARAM:
             entryThread = (SceModuleEntryThread *)pLib->entryTable[2 * pLib->stubCount + pLib->vStubCount + i];
             pMod->moduleStopThreadPriority = entryThread->initPriority; //0x00007AF0
             pMod->moduleStopThreadStacksize = entryThread->stackSize; //0x00007AF8
             pMod->moduleStopThreadAttr = entryThread->attr; //0x00007B04
             break;
         case NID_MODULE_REBOOT_BEFORE_THREAD_PARAM: //0x00007B08
             entryThread = (SceModuleEntryThread *)pLib->entryTable[2 * pLib->stubCount + pLib->vStubCount + i];
             pMod->moduleRebootBeforeThreadPriority = entryThread->initPriority; //0x00007B30
             pMod->moduleRebootBeforeThreadStacksize = entryThread->stackSize; //0x00007B38
             pMod->moduleRebootBeforeThreadAttr = entryThread->attr; //0x00007B44
             break;               
         case NID_MODULE_START_THREAD_PARAM: 
             entryThread = (SceModuleEntryThread *)pLib->entryTable[2 * pLib->stubCount + pLib->vStubCount + i];
             pMod->moduleStartThreadPriority = entryThread->initPriority; //0x00007A98
             pMod->moduleStartThreadStacksize = entryThread->stackSize; //0x00007AA0
             pMod->moduleStartThreadAttr = entryThread->attr; //0x00007AA8
             break;
        }
    }
    return SCE_ERROR_OK;
}

// TODO: Reverse function sub_00007C34
// 0x00007C34
void allocate_module_block()
{
}

// sub_00007ED8
// TODO: Add a PBP header structure
s32 _CheckSkipPbpHeader(SceModuleMgrParam *pModParams, SceUID fd, void *pBlock, u32 *pOffset, s32 apiType)
{
    (void)pModParams;
    u8 *data = (u8 *)pBlock;
    
    switch (apiType) {
    case SCE_INIT_APITYPE_DISC_EMU_MS2:
    case SCE_INIT_APITYPE_DISC_EMU_EF2:
    case SCE_INIT_APITYPE_MS1:
    case SCE_INIT_APITYPE_MS2:
    case SCE_INIT_APITYPE_MS3:
    case SCE_INIT_APITYPE_MS4:
    case SCE_INIT_APITYPE_MS5:
    case SCE_INIT_APITYPE_MS6:
    case SCE_INIT_APITYPE_EF1:
    case SCE_INIT_APITYPE_EF2:
    case SCE_INIT_APITYPE_EF3:
    case SCE_INIT_APITYPE_EF4:
    case SCE_INIT_APITYPE_EF5:
    case SCE_INIT_APITYPE_EF6:
    case SCE_INIT_APITYPE_UNK_GAME1:
    case SCE_INIT_APITYPE_UNK_GAME2:
        // 0x00007F14
		// TODO: define PBP magic value
        if (data[0] != 0 || data[1] != 'P' || data[2] != 'B' || data[3] != 'P') // 0x00007F40
            return SCE_ERROR_KERNEL_UNSUPPORTED_PRX_TYPE;
        
        if (pOffset != NULL)
            *pOffset = ((u32 *)data)[9] - ((u32 *)data)[8];
            
         sceIoLseek(fd, ((u32 *)data)[8], SCE_SEEK_CUR); // 0x00007F6C
         return ((u32 *)data)[8];
        
    default:
        // 0x00007F8C
        if (data[0] == 0 && data[1] == 'P' && data[2] == 'B' && data[3] == 'P') // 0x00007FB8
            return SCE_ERROR_KERNEL_UNSUPPORTED_PRX_TYPE;
        
        if (pOffset != NULL)
            *pOffset = 0;
        return SCE_ERROR_OK;
    }
}

// sub_00007FD0
s32 _PartitionCheck(SceModuleMgrParam *pModParams, SceLoadCoreExecFileInfo *pExecInfo)
{
    s32 status;
    
    if (pExecInfo->isKernelMod) { // 0x00007FE8
        pExecInfo->partitionId = SCE_KERNEL_PRIMARY_KERNEL_PARTITION; // 0x000080A8
        if (pModParams->mpIdText != 0) {
            status = _CheckKernelOnlyModulePartition(pModParams->mpIdText);
            if (status < SCE_ERROR_OK)
                return status;
            pExecInfo->partitionId = pModParams->mpIdText; // 0x00008120
        }
        if (pModParams->mpIdData != 0) { // 0x000080B0
            status = _CheckKernelOnlyModulePartition(pModParams->mpIdText);
            if (status < SCE_ERROR_OK)
                return status;
        }
    } else {
        pExecInfo->partitionId = SCE_KERNEL_PRIMARY_USER_PARTITION;
        if (pModParams->mpIdText != 0) {
            status = _CheckUserModulePartition(pModParams->mpIdText);
            if (status < SCE_ERROR_OK)
                return status;
            pExecInfo->partitionId = pModParams->mpIdText; // 0x00008098
        }
        if (pModParams->mpIdData != 0) { // 0x00008004
            status = _CheckUserModulePartition(pModParams->mpIdData);
            if (status < SCE_ERROR_OK)
                return status;
        }
    }
    return SCE_ERROR_OK;
}

// sub_00008124
s32 _PrologueModule(SceModuleMgrParam *modParams, SceModule *pMod)
{
    s32 oldGp;
    s32 status;
    
    oldGp = pspGetGp(); // 0x00008150
    
    status = ModuleRegisterLibraries(pMod); // 0x00008154
    if (status < SCE_ERROR_OK) // 0x0000815C
        return status;
    
    // 0x0000816C
    if ((s32)pMod->stubTop != -1) {
        if (pMod->status & SCE_MODULE_USER_MODULE) // 0x0000817C
            status = sceKernelLinkLibraryEntriesWithModule(pMod, pMod->stubTop, pMod->stubSize); // 0x00008188
        else
            status = sceKernelLinkLibraryEntries(pMod->stubTop, pMod->stubSize); // 0x0000843C
        
        if (status < SCE_ERROR_OK) { // 0x00008190
            _ModuleReleaseLibraries(pMod); // 0x00008424
            return status;
        }
    }
    pMod->segmentChecksum = sceKernelSegmentChecksum(pMod); // 0x00008198
    if (pMod->unk224 != 0) { // 0x000081A4
        s32 sum;
        u32 *textSegment = (u32 *)pMod->segmentAddr[0];
        for (s32 i = 0; i < pMod->textSize / sizeof(u32); i++) // 0x000081D0
            sum += textSegment[i];
        pMod->unk220 = sum; // 0x000081D8
    } else
        pMod->unk220 = 0; // 0x00008420
        
    void *modStart = pMod->moduleStart;
    if (pMod->moduleStart == -1) // 0x000081E4
        modStart = (pMod->moduleBootstart != modStart) ? pMod->moduleBootstart : (u32 *)pMod->entryAddr;
    
    /* No module entry point found */
    if ((s32)modStart == -1 || modStart == NULL) {
        pMod->userModThid = -1;
        return SCE_ERROR_OK;
    }
    s32 threadPriority = modParams->threadPriority;
    if (threadPriority == 0) { // 0x0000823C
        threadPriority = (pMod->moduleStartThreadPriority == -1) ? SCE_KERNEL_MODULE_INIT_PRIORITY 
                : pMod->moduleStartThreadPriority; //0x0000824C
    }
    s32 stackSize = modParams->stackSize; // 0x00008258
    if (stackSize == 0) // 0x00008258
        stackSize = (pMod->moduleStartThreadStacksize == -1) ? 0 : pMod->moduleStartThreadStacksize; // 0x0000826C
            
    s32 threadAttr = (pMod->moduleStartThreadAttr == -1) ? SCE_KERNEL_TH_DEFAULT_ATTR : pMod->moduleStartThreadAttr; //0x00007250
    if (modParams->threadAttr != SCE_KERNEL_TH_DEFAULT_ATTR) // 0x00008280
        threadAttr |= (modParams->threadAttr & 0xF06000); // 0x00008294           
    threadAttr &= ~0xF0000000; //0x000082A4
    
    if (!(pMod->status & SCE_MODULE_USER_MODULE)) //0x000082A0
        stackSize = (stackSize == 0) ? SCE_KERNEL_TH_DEFAULT_SIZE : 0; // 0x00008404
    else {
        stackSize = (stackSize == 0) ? 0x40000 : 0; //0x000082B8
            
        if ((pMod->attribute & 0x1E00) == SCE_MODULE_VSH) //0x000082BC
            threadAttr |= SCE_KERNEL_TH_VSH_MODE;
        else if ((pMod->attribute & 0x1E00) == SCE_MODULE_APP) // 0x000082C8
            threadAttr |= SCE_KERNEL_TH_APP_MODE;
        else if ((pMod->attribute & 0x1E00) == SCE_MODULE_USB_WLAN) // 0x000082D4
            threadAttr |= SCE_KERNEL_TH_USB_WLAN_MODE;
        else if ((pMod->attribute & 0x1E00) == SCE_MODULE_MS) // 0x000082E0
            threadAttr |= SCE_KERNEL_TH_MS_MODE;
        else
            threadAttr |= SCE_KERNEL_TH_USER_MODE;
    }
    SceKernelThreadOptParam threadOptions;
    threadOptions.size = sizeof(SceKernelThreadOptParam);
    threadOptions.stackMpid = (modParams->threadMpIdStack == 0) ? pMod->mpIdData : modParams->threadMpIdStack; // 0x000082F8 & 0x000083F8
    if (threadOptions.stackMpid != 0) { // 0x0000830C
        if (pMod->status & SCE_MODULE_USER_MODULE)
            status = _CheckUserModulePartition(threadOptions.stackMpid);
        else
            status = _CheckKernelOnlyModulePartition(threadOptions.stackMpid);
        
        if (status < SCE_ERROR_OK) {
            if (pMod->stubTop != (void *)-1) // 0x00008364
                sceKernelUnLinkLibraryEntries(pMod->stubTop, pMod->stubSize); // 0x0000836C
                 
            _ModuleReleaseLibraries(pMod); // 0x00008374
            return status;
        }
    }
    pspSetGp(pMod->gpValue); //0x00008384
            
    status = sceKernelCreateThread("SceModmgrStart", (SceKernelThreadEntry)modStart, threadPriority, stackSize, 
                    threadAttr, &threadOptions); // 0x000083A0
    pMod->userModThid = status; // 0x000083A8
            
    pspSetGp(oldGp); // 0x000083AC
            
    if (status < SCE_ERROR_OK) { // 0x000083C0
        if ((s32)pMod->stubTop != -1) // 0x00008364
            sceKernelUnLinkLibraryEntries(pMod->stubTop, pMod->stubSize); // 0x0000836C
                 
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
    
    pCurEntry = pMod->entTop;
    pLastEntry = pMod->entTop + pMod->entSize;
    
    // 0x00008468 & 0x0000847C - 0x000084C4
    while (pCurEntry < pLastEntry) {
        SceResidentLibraryEntryTable *pCurTable = (SceResidentLibraryEntryTable *)pCurEntry;
        if (!pCurTable->attribute & SCE_LIB_AUTO_EXPORT) { //0x00006FB4
            if (pCurTable->attribute == SCE_LIB_IS_SYSLIB)
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

// TODO: Reverse function sub_00008568
// 0x00008568
s32 _CheckOverride(s32 apiType, void *pBuffer, void *data)
{
    
}

// TODO: Reverse function sub_000086C0
// 0x000086C0
void sub_000086C0()
{
}
