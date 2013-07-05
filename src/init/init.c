/* Copyright (C) 2011, 2012, 2013 The uOFW team
   See the file COPYING for copying permission.
*/

/**
 * uofw/src/init/init.c
 * 
 * Init is responsible for loading the rest of the PSP kernel modules
 * listed in the PSP boot configuration file (as of now: /kd/pspbtcnf.txt) 
 * right below Init.
 * 
 * When the user starts a game, an application or an updater in the XMB, the
 * PSP kernel reboots and Init initializes the kernel modules again, as well
 * as the corresponding game/application/updater modules.
 * 
 * Init itself is loaded and started by Loadcore and stops and unloads itself
 * after loading all the modules it has to.
 */

#include <common_imp.h>
#include <init.h>
#include <interruptman.h>
#include <iofilemgr_kernel.h>
#include <iofilemgr_stdio.h>
#include <loadcore.h>
#include <loadexec_kernel.h>
#include <modulemgr_init.h>
#include <modulemgr_kernel.h>
#include <modulemgr_user.h>
#include <sysmem_user.h>
#include <sysmem_kernel.h>
#include <sysmem_kdebug.h>
#include <sysmem_sysclib.h>
#include <sysmem_suspend_kernel.h>
#include <sysmem_utils_kernel.h>
#include <threadman_kernel.h>

#include "init_int.h"
#include "libcUtils.h"
#include "gamePatching.h"

SCE_MODULE_INFO(INIT_MODULE_NAME, SCE_MODULE_KERNEL | SCE_MODULE_ATTR_EXCLUSIVE_LOAD | SCE_MODULE_ATTR_EXCLUSIVE_START, 
                1, 11);
SCE_MODULE_BOOTSTART("InitInit");
SCE_SDK_VERSION(SDK_VERSION);

#define RAM_SIZE_32_MB                      (0x2000000)

/* 
 * The PSP boot configuration file containing the kernel modules to boot and
 * and the order in which each module is booted. 
 */
#define PSP_BOOT_CONFIG_FILE_PATH           "/kd/pspbtcnf.txt"

/* The size of the PSP boot configuration file copy used by Init. */
#define INIT_BOOT_CONFIG_FILE_COPY_SIZE     (256)

/* Init's internal control block. */
SceInit *g_init;

/* Pointer to a memory clear function executed in StopInit(). */
void (*g_MemClearFunc)(void *, SceSize);
/* The start address of the memory block to clear. */
void *g_MemBase;
/* The size of the particular memory block to clear. */
SceSize g_MemSize;

/* A check if there is a PSP_BOOT_CONFIG_FILE. */
SceBool g_hasConfigFile;
/* A copy of a subpart of the PSP_BOOT_CONFIG_FILE. */
char g_configFileCopy[INIT_BOOT_CONFIG_FILE_COPY_SIZE];

/* A buffer containing information about a NP-DRM module. */
SceNpDrm g_npDrmData;

/* A copy of the module's boot information used by Init to boot the module. */
SceLoadCoreBootModuleInfo g_mod;

/* Initialize Init's control block. */
static void InitCBInit(SceLoadCoreBootInfo *bootInfo)
{   
    if (bootInfo->configFile == NULL)
        g_hasConfigFile = SCE_FALSE;
    else {
        strncpy(g_configFileCopy, bootInfo->configFile, sizeof g_configFileCopy);
        g_hasConfigFile = SCE_TRUE;
    }
    
    g_init->applicationType = SCE_INIT_APPLICATION_VSH;
    g_init->numPowerLocks = 1;
    g_init->apiType = 0;
    g_init->lptSummary = 0;
    g_init->fileModAddr = NULL;
    g_init->discModAddr = NULL;
    g_init->paramSfoBase = NULL;
    g_init->paramSfoSize = 0;
    g_init->bootCallbacks1 = NULL;
    g_init->curBootCallback1 = NULL;
    g_init->bootCallbacks2 = NULL;
    g_init->curBootCallback2 = NULL;
    
    g_npDrmData.size = sizeof(SceNpDrm);
    g_npDrmData.fileOffset = -1;
    
    u32 i;
    for (i = 0; i < sizeof g_npDrmData.keyData; i++)
        g_npDrmData.keyData[i] = 0;
}

/* Exit Init and restart the kernel. */
static s32 ExitInit(s32 error)
{
    void *blk;
    SceUID blkId;
    
    g_init->vshParam.size = sizeof(SceKernelLoadExecVSHParam);
    blkId = sceKernelGetChunk(0);
    if (blkId <= 0) {
        blk = &g_init->unk60;
        g_init->unk60 = 32;
        g_init->unk64 = 32; 
        g_init->unk68 = 0;
        g_init->unk76 = 0;
    } else
        blk = sceKernelGetBlockHeadAddr(blkId);

    *(s32 *)(blk + 24) = error;
    g_init->vshParam.args = *(s32 *)blk;
    g_init->vshParam.configFile = PSP_BOOT_CONFIG_FILE_PATH;
    g_init->vshParam.vshmainArgp = blk;
    g_init->vshParam.string = NULL;
    g_init->vshParam.argp = blk;
    g_init->vshParam.vshmainArgs = *(s32 *)blk;
    g_init->vshParam.key = NULL;
    
    blkId = sceKernelGetChunk(4);
    if (blkId <= 0) {
        g_init->vshParam.extArgp = NULL;
        g_init->vshParam.extArgs = 0;
    } else {
        g_init->vshParam.extArgs = SysMemForKernel_CC31DEAD(blkId);
        g_init->vshParam.extArgp = sceKernelGetBlockHeadAddr(blkId);
    }
    /* Reboot the kernel. */
    sceKernelExitVSHKernel(&g_init->vshParam);
    return SCE_ERROR_OK;
}

/* Check if the kernel has to be re-started. */
static u32 ExitCheck(void)
{
    SceUID blkId;
    void *blk;
    
    blkId = sceKernelGetChunk(0);
    if (blkId <= 0)
        return SCE_ERROR_OK;
    
    blk = sceKernelGetBlockHeadAddr(blkId);
    s32 arg = *(s32 *)(blk + 20);
    if (arg != 0)
        ExitInit(arg);
        
    return SCE_ERROR_OK;
}

static void PowerUnlock(void)
{
    sceKernelSetSystemStatus(0x20000);
    if (g_init->numPowerLocks != 0 && sceKernelPowerUnlock(SCE_KERNEL_POWER_LOCK_DEFAULT) >= SCE_ERROR_OK)
        g_init->numPowerLocks--;
}

/* Call the registered boot callbacks of the loaded modules. */
static void invoke_init_callback(s32 arg)
{
    s32 i; 
    s32 unk;
    SceBootCallback *bootCallBacks;
    SceKernelBootCallbackFunction bootCbFunc;
    
    if (arg >= 4) {
        bootCallBacks = g_init->bootCallbacks2;
        i = 4;
        unk = 5;
    } else {
        bootCallBacks = g_init->bootCallbacks1;
        i = 0;
        unk = 4;
    }
    s32 starti = i;
    
    for (; i <= arg; i++) {
        if ((arg == (unk - 1)) && (arg == i)) {
            if (arg >= 4) //0x00000514
                g_init->bootCallbacks2 = NULL;
            else
                g_init->bootCallbacks1 = NULL;
        }
        s32 j;
        for (j = 0; bootCallBacks[j].bootCBFunc != NULL; j++) { 
            if ((starti + ((s32)bootCallBacks[j].bootCBFunc & 3)) == i) {
                bootCbFunc = (SceKernelBootCallbackFunction)((s32)bootCallBacks[j].bootCBFunc & ~0x3);
                if (i != (unk - 1))
                    bootCbFunc((void *)bootCallBacks, 1, NULL);
                else
                    bootCbFunc(((void *)&bootCallBacks[j]) + 2, 0, NULL);
            }
        }
        if (i == (arg - 1)) {
            while (j > 0) {
                j--; //0x00000590
                if ((((s32)bootCallBacks[j].bootCBFunc & 3) + starti) >= arg)
                    break;
                bootCallBacks[j].bootCBFunc = NULL;
            }
        }
    }
}

static u32 sub_05F0(void)
{   
    s32 pspModel;
    s32 apiType;
    s32 intrState;
    
    invoke_init_callback(3);
    patchGames();   
    sceKernelSetDNAS(0);
    
    intrState = sceKernelCpuSuspendIntr();
    
    pspModel = sceKernelGetModel();
    if (pspModel >= PSP_4000) {
        apiType = sceKernelApplicationType();
        switch (apiType) {
        case SCE_INIT_APPLICATION_VSH: 
        case SCE_INIT_APPLICATION_UPDATER: 
        case SCE_INIT_APPLICATION_POPS:
            UtilsForKernel_39FFB756(81);
            break;
        default:
            UtilsForKernel_39FFB756(0);
            break;
        }
    }
    sceKernelCpuResumeIntr(intrState);
    PowerUnlock();
    return SCE_ERROR_OK;
}

/* 
 * Free memory used by protected modules and free the memory containing the 
 * information about the modules to boot on a kernel (re-)start.
 */
static s32 CleanupPhase1(SceLoadCoreBootInfo *bootInfo)
{
    SceUID partId;
    SceUID blkId;
    s32 status;
    
    if ((u32)g_init->discModAddr > 0)
        return SCE_ERROR_OK;
    
    g_init->discModAddr += 1;
    
    s32 i;
    for (i = 0; i < bootInfo->numProtects; i++) {
        SceLoadCoreProtectInfo *prot = &bootInfo->protects[i];
        if (GET_PROTECT_INFO_TYPE(prot->attr) == 0x200 
                && (GET_PROTECT_INFO_STATE(prot->attr) & SCE_PROTECT_INFO_STATE_IS_ALLOCATED)) {
            sceKernelFreePartitionMemory(prot->partId);
            prot->attr = REMOVE_PROTECT_INFO_STATE(SCE_PROTECT_INFO_STATE_IS_ALLOCATED, prot->attr);
        }
    }
    if (bootInfo->modules == NULL)
        return SCE_ERROR_OK;
    
    status = sceKernelQueryMemoryInfo((u32)bootInfo->modules, &partId, &blkId);
    if (status < SCE_ERROR_OK)
        return status;
    
    sceKernelFreePartitionMemory(blkId);
    return SCE_ERROR_OK;
}

/*
 * Free memory used by protected modules, free the memory containing protection
 * information and free the memory holding the kernel boot information.
 */
static s32 CleanupPhase2(SceLoadCoreBootInfo *bootInfo)
{
    SceUID partId;
    SceUID blkId;
    s32 status;
    
    s32 i;
    for (i = 0; i < bootInfo->numProtects; i++) {
        SceLoadCoreProtectInfo *prot = &bootInfo->protects[i];
        switch (GET_PROTECT_INFO_TYPE(prot->attr)) { 
        case SCE_PROTECT_INFO_TYPE_USER_PARAM: case 0x200:
            if (GET_PROTECT_INFO_STATE(prot->attr) & SCE_PROTECT_INFO_STATE_IS_ALLOCATED) {
                sceKernelFreePartitionMemory(prot->partId);
                prot->attr = REMOVE_PROTECT_INFO_STATE(SCE_PROTECT_INFO_STATE_IS_ALLOCATED, prot->attr);
            }
            break;
        case SCE_PROTECT_INFO_TYPE_FILE_NAME:
            if (GET_PROTECT_INFO_STATE(prot->attr) & SCE_PROTECT_INFO_STATE_IS_ALLOCATED) {
                sceKernelFreePartitionMemory(prot->partId);
                prot->attr = REMOVE_PROTECT_INFO_STATE(SCE_PROTECT_INFO_STATE_IS_ALLOCATED, prot->attr);
                g_init->fileModAddr = NULL;
            }
            break;
        }
    }
    if (bootInfo->protects != NULL) {
        status = sceKernelQueryMemoryInfo((u32)bootInfo->protects, &partId, &blkId);
        if (status < SCE_ERROR_OK)
            return status;
        
        sceKernelFreePartitionMemory(partId);
    }
    sceKernelQueryMemoryInfo((u32)bootInfo, &partId, &blkId);
    sceKernelFreePartitionMemory(partId);
    return SCE_ERROR_OK;
}

/* Allocate memory for protected modules. */
static void ProtectHandling(SceLoadCoreBootInfo *bootInfo)
{
    SceUID partId;
    SceSysmemMemoryBlockInfo blkInfo;
    
    s32 i;
    for (i = 0; i < bootInfo->numProtects; i++) {
        SceLoadCoreProtectInfo *protectInfo = &bootInfo->protects[i];
        switch (GET_PROTECT_INFO_TYPE(protectInfo->attr)) { 
        case SCE_PROTECT_INFO_TYPE_FILE_NAME:
            if (protectInfo->size != 0) { //0x000009AC
                partId = sceKernelAllocPartitionMemory(SCE_KERNEL_PRIMARY_KERNEL_PARTITION, "SceInitFileName", 
                        SCE_KERNEL_SMEM_High, protectInfo->size, 0);
                if (partId > 0) {
                    void *addr = sceKernelGetBlockHeadAddr(partId);
                    memmove(addr, (void *)protectInfo->addr, protectInfo->size);
                    
                    protectInfo->addr = (u32)addr;
                    protectInfo->partId = partId;
                    protectInfo->attr = SET_PROTECT_INFO_STATE(SCE_PROTECT_INFO_STATE_IS_ALLOCATED, protectInfo->attr);
                }
            }
            if (i == bootInfo->modProtId)
                g_init->fileModAddr = (void *)protectInfo->addr;
            break;
        case SCE_PROTECT_INFO_TYPE_VSH_PARAM: 
            if (protectInfo->size != 0) {
                partId = sceKernelAllocPartitionMemory(SCE_KERNEL_PRIMARY_KERNEL_PARTITION, "SceInitVSHParam", 
                        SCE_KERNEL_SMEM_High, protectInfo->size, 0);
                if (partId > 0) {
                    blkInfo.size = sizeof blkInfo;
                    sceKernelQueryMemoryBlockInfo(partId, &blkInfo);
                    void *addr = (void *)blkInfo.addr;
                    memmove(addr, (void *)protectInfo->addr, protectInfo->size);
                    
                    protectInfo->addr = (u32)addr;
                    protectInfo->attr = SET_PROTECT_INFO_STATE(SCE_PROTECT_INFO_STATE_IS_ALLOCATED, protectInfo->attr);
                    protectInfo->partId = partId;
                    
                    *(u32 *)(addr + 4) = 32;
                    *(u32 *)(addr + 0) = blkInfo.memSize;
                    *(s32 *)(addr + 24) = 0;
                    *(s32 *)(addr + 20) = 0;
                    sceKernelRegisterChunk(0, partId);
                }
            }
            break;
        case SCE_PROTECT_INFO_TYPE_DISC_IMAGE:
            if (protectInfo->size != 0) {
                partId = sceKernelAllocPartitionMemory(SCE_KERNEL_PRIMARY_KERNEL_PARTITION, "SceInitDiscImage", 
                        SCE_KERNEL_SMEM_High, protectInfo->size, 0);
                if (partId > 0) {
                    void *addr = sceKernelGetBlockHeadAddr(partId);
                    memmove(addr, (void *)protectInfo->addr, protectInfo->size);
                    
                    protectInfo->addr = (u32)addr;
                    protectInfo->attr = SET_PROTECT_INFO_STATE(SCE_PROTECT_INFO_STATE_IS_ALLOCATED, protectInfo->attr);
                    protectInfo->partId = partId;
                    
                    sceKernelRegisterChunk(3, partId);
                    g_init->discModAddr = addr;
                }
            }
            break;
        case SCE_PROTECT_INFO_TYPE_NPDRM_DATA: 
            if ((protectInfo->size) != 0 && (*(u32 *)(protectInfo->addr) == sizeof(SceNpDrm)))
                memcpy(&g_npDrmData, (void *)protectInfo->addr, sizeof(SceNpDrm));
            break;
        case SCE_PROTECT_INFO_TYPE_USER_PARAM:
            if (protectInfo->size != 0) {
                partId = sceKernelAllocPartitionMemory(SCE_KERNEL_PRIMARY_KERNEL_PARTITION, "SceInitUserParam", 
                        SCE_KERNEL_SMEM_High, protectInfo->size, 0);
                if (partId > 0) {
                    void *addr = sceKernelGetBlockHeadAddr(partId);
                    memmove(addr, (void *)protectInfo->addr, protectInfo->size);
                    
                    protectInfo->addr = (u32)addr;
                    protectInfo->attr = SET_PROTECT_INFO_STATE(SCE_PROTECT_INFO_STATE_IS_ALLOCATED, protectInfo->attr);
                    protectInfo->partId = partId;
                }
            }
            break;
        case SCE_PROTECT_INFO_TYPE_PARAM_SFO:
            if (protectInfo->size != 0) { //0x00000B24
                partId = sceKernelAllocPartitionMemory(SCE_KERNEL_PRIMARY_KERNEL_PARTITION, "SceInitParamsfo", 
                        SCE_KERNEL_SMEM_High, protectInfo->size, 0);
                if (partId > 0) { //0x00000B40
                    void *addr = sceKernelGetBlockHeadAddr(partId);
                    memmove(addr, (void *)protectInfo->addr, protectInfo->size);
                    
                    protectInfo->addr = (u32)addr;
                    protectInfo->attr = SET_PROTECT_INFO_STATE(SCE_PROTECT_INFO_STATE_IS_ALLOCATED, protectInfo->attr);
                    protectInfo->partId = partId;
                    
                    sceKernelRegisterChunk(4, partId);
                    g_init->paramSfoBase = addr;
                    g_init->paramSfoSize = protectInfo->size;
                }
            }
            break;
        }
    }
    if (sceKernelGetChunk(0) < SCE_ERROR_OK) {
        partId = sceKernelAllocPartitionMemory(SCE_KERNEL_PRIMARY_KERNEL_PARTITION, "SceInitVSHParam", 
                SCE_KERNEL_SMEM_High, 32, 0);
        if (partId > 0) {
            blkInfo.size = sizeof blkInfo;
            sceKernelQueryMemoryBlockInfo(partId, &blkInfo);
            
            void *addr = (void *)blkInfo.addr;
            *(s32 *)(addr + 4) = 32;
            *(s32 *)(addr + 0) = blkInfo.memSize;
            *(s32 *)(addr + 24) = 0;
            *(s32 *)(addr + 8) = 0;
            *(s32 *)(addr + 12) = 0;
            *(s32 *)(addr + 16) = 0;
            *(s32 *)(addr + 20) = 0;
            sceKernelRegisterChunk(0, partId);
        }
    }
}

static SceBool sub_0CFC(SceLoadCoreBootModuleInfo *mod)
{
    SceSysmemPartitionInfo partInfo;
    
    partInfo.size = sizeof(SceSysmemPartitionInfo);
    sceKernelQueryMemoryPartitionInfo(SCE_KERNEL_SC_USER_PARTITION, &partInfo);
    
    return ((u32)UCACHED(mod->modBuf + mod->modSize)) >= partInfo.startAddr;
}

/* Clear memory blocks used by booted modules. */
static s32 ClearFreeBlock(void)
{
    s32 status;
    
    sceKernelFillFreeBlock(SCE_KERNEL_PRIMARY_KERNEL_PARTITION, 0);
    sceKernelFillFreeBlock(SCE_KERNEL_VSHELL_PARTITION, 0);
    
    sceKernelVolatileMemUnlock(SCE_KERNEL_VOLATILE_MEM_DEFAULT);
    
    status = sceKernelFillFreeBlock(SCE_KERNEL_PRIMARY_USER_PARTITION, 0);
    
    sceKernelMemoryExtendSize(); //0x00000D84
    
    sceKernelFillFreeBlock(SCE_KERNEL_EXTENDED_SC_KERNEL_PARTITION, 0);
    sceKernelFillFreeBlock(SCE_KERNEL_VSHELL_KERNEL_PARTITION, 0);
    
    sceKernelMemoryShrinkSize();
    
    sceKernelMemset32((void *)SCE_SCRATCHPAD_ADDR_K0, 0, SCE_SCRATCHPAD_SIZE);
    return status;
}

static u32 sub_0DD0(SceLoadCoreBootInfo *bootInfo)
{
    if (sceKernelIsDevelopmentToolMode() == SCE_FALSE || bootInfo->unk24 != 0)
        return 0;
    return (sceKernelApplicationType() == SCE_INIT_APPLICATION_GAME);
}

/* Load the modules listed in the PSP boot configuration file. */
static SceUID LoadModuleAnchorInBtcnf(char *file)
{
    char *uFile = file;
    SceUID status;
    
    switch (g_init->apiType) {
    case SCE_INIT_APITYPE_GAME_EBOOT: 
    case SCE_INIT_APITYPE_GAME_BOOT: 
    case SCE_INIT_APITYPE_EMU_EBOOT_MS: 
    case SCE_INIT_APITYPE_EMU_BOOT_MS: 
    case SCE_INIT_APITYPE_EMU_EBOOT_EF: 
    case SCE_INIT_APITYPE_EMU_BOOT_EF:
        status = sceKernelLoadModuleForLoadExecForUser(g_init->apiType, file, 0, NULL);
        uFile = NULL;
        break;
    case SCE_INIT_APITYPE_NPDRM_MS: 
    case SCE_INIT_APITYPE_NPDRM_EF:
        status = sceKernelLoadModuleForLoadExecNpDrm(g_init->apiType, file, g_npDrmData.fileOffset, g_npDrmData.keyData, 0, NULL);
        uFile = NULL;
        break;
    case SCE_INIT_APITYPE_DISC:
        status = sceKernelLoadModuleForLoadExecVSHDisc(file, 0, NULL);
        break;
    case SCE_INIT_APITYPE_DISC_UPDATER:
        status = sceKernelLoadModuleForLoadExecVSHDiscUpdater(file, 0, NULL);
        break;
    case SCE_INIT_APITYPE_DISC_DEBUG:
        if (sceKernelIsToolMode() == SCE_FALSE)
            status = SCE_ERROR_KERNEL_ILLEGAL_PERMISSION;
        else
            status = sceKernelLoadModuleForLoadExecVSHDiscDebug(file, 0, NULL);
        break;
    case SCE_INIT_APITYPE_DISC_EMU_MS1: 
    case SCE_INIT_APITYPE_DISC_EMU_EF1: 
    case SCE_INIT_APITYPE_MLNAPP_MS: 
    case SCE_INIT_APITYPE_MLNAPP_EF:
        status = sceKernelLoadModuleForLoadExecVSHDiscEmu(g_init->apiType, file, 0, NULL);
        uFile = (char *)g_init->discModAddr;
        break;
    case SCE_INIT_APITYPE_DISC_EMU_MS2: 
    case SCE_INIT_APITYPE_DISC_EMU_EF2:
        status = ModuleMgrForKernel_C2A5E6CA(g_init->apiType, file, 0, NULL);
        uFile = (char *)g_init->discModAddr;
        break;
    case SCE_INIT_APITYPE_MS1: 
    case SCE_INIT_APITYPE_EF1:
        status = sceKernelLoadModuleForLoadExecVSHMs1(g_init->apiType, file, 0, NULL);
        break;
    case SCE_INIT_APITYPE_MS2: 
    case SCE_INIT_APITYPE_EF2:
        status = sceKernelLoadModuleForLoadExecVSHMs2(g_init->apiType, file, 0, NULL);
        break;
    case SCE_INIT_APITYPE_MS3: 
    case SCE_INIT_APITYPE_EF3:
        status = sceKernelLoadModuleForLoadExecVSHMs3(g_init->apiType, file, 0, NULL);
        break;
    case SCE_INIT_APITYPE_MS4: 
    case SCE_INIT_APITYPE_EF4:
        status = sceKernelLoadModuleForLoadExecVSHMs4(g_init->apiType, file, 0, NULL);
        break;
    case SCE_INIT_APITYPE_MS5: 
    case SCE_INIT_APITYPE_EF5:
        status = sceKernelLoadModuleForLoadExecVSHMs5(g_init->apiType, file, 0, NULL);
        break;
    case SCE_INIT_APITYPE_MS6: 
    case SCE_INIT_APITYPE_EF6:
        status = sceKernelLoadModuleForLoadExecVSHMs6(g_init->apiType, file, 0, NULL);
        break;
    case 0x160: 
    case 0x161:
        status = ModuleMgrForKernel_8DD336D4(g_init->apiType, file, 0, NULL);
        break;
    default:
        status = SCE_ERROR_KERNEL_ILLEGAL_PERMISSION;
        break;
    }
    if (status >= 0 && uFile != NULL)
        SysMemForKernel_BFE08689(uFile);
    return status;
}

/* Load modules (via their buffers) listed in the PSP boot configuration file. */
static SceUID LoadModuleBufferAnchorInBtcnf(void *modBuf, s32 opt)
{
    SceUID status;
    
    switch (g_init->apiType) {
    case SCE_INIT_APITYPE_USBWLAN: 
    case 0x132:
        status = sceKernelLoadModuleBufferForLoadExecBufferVSHUsbWlan(g_init->apiType, modBuf, 0, NULL);
        break;
    case SCE_INIT_APITYPE_USBWLAN_DEBUG: 
    case 0x133:
        if (sceKernelIsToolMode() == SCE_FALSE)
            status = SCE_ERROR_KERNEL_ILLEGAL_PERMISSION;
        else
            status = sceKernelLoadModuleBufferForLoadExecBufferVSHUsbWlanDebug(g_init->apiType, modBuf, 0, NULL);
        break;
    case SCE_INIT_APITYPE_KERNEL_1:
        status = sceKernelLoadModuleBufferForExitVSHKernel(modBuf, 0, NULL, opt);
        break;
    case SCE_INIT_APITYPE_VSH_1:
        status = sceKernelLoadModuleBufferForExitGame(modBuf, 0, NULL, opt);
        break;
    case SCE_INIT_APITYPE_VSH_2:
        status = sceKernelLoadModuleBufferForExitVSHVSH(modBuf, 0, NULL, opt);
        break;
    case SCE_INIT_APITYPE_KERNEL_REBOOT:
        status = sceKernelLoadModuleBufferForRebootKernel(modBuf, 0, NULL, opt);
        break;
    default:
        status = SCE_ERROR_KERNEL_ILLEGAL_PERMISSION;
        break;
    }
    SysMemForKernel_BFE08689(NULL);
    return status;
}

/* Stop the kernel initialization process. */
static void StopInit(const char *str __attribute__((unused)), s32 line __attribute__((unused)))
{
    s32 intrState = sceKernelCpuSuspendIntr();
    
    HW(HW_RAM_SIZE) &= ~(RAM_TYPE_32_MB | RAM_TYPE_64_MB);
    if (g_MemSize > RAM_SIZE_32_MB)
        HW(HW_RAM_SIZE) |= RAM_TYPE_64_MB;
    else
        HW(HW_RAM_SIZE) |= RAM_TYPE_32_MB;
    
    memset((void *)HWPTR(HW_RESET_VECTOR), 0, HW_RESET_VECTOR_SIZE);
    memcpy((void *)HWPTR(HW_RESET_VECTOR), initClearMem, INIT_CLEAR_MEM_SIZE);
    g_MemClearFunc = (void (*)(void *, u32))HWPTR(HW_RESET_VECTOR);
    g_MemClearFunc(g_MemBase, g_MemSize);
    
    sceKernelCpuResumeIntr(intrState);
    
    for (;;)
        ;
}

/* 
 * Load all the modules listed in the PSP boot config file right below init. 
 * Also load game modules if the user restarts the kernel by executing a game.
 */
static void InitThreadEntry(SceSize args, void *argp)
{
    SceBool foundMod;
    SceUID threadId;
    SceLoadCoreBootInfo *bootInfo;
    s32 modStatus1, modStatus2;
    
    if (args < 8)
        return;
    
    threadId = ((u32 *)argp)[0];
    bootInfo = (SceLoadCoreBootInfo *)((u32 *)argp)[1];
    
    foundMod = SCE_FALSE;
    
    sceKernelWaitThreadEnd(threadId, NULL);
    
    if (sceKernelIsDevelopmentToolMode() != SCE_FALSE)
        printf("devkit version 0x%08x\n", SDK_VERSION);
    
    if (bootInfo->buildVersion != 0)
        printf("build version 0x%08x\n", bootInfo->buildVersion);
    
    SceBootCallback bootCallbacks[bootInfo->numModules];
    SceBootCallback *tmpBootCb = (SceBootCallback *)UPALIGN16((u32)bootCallbacks);
    tmpBootCb->bootCBFunc = NULL;
    g_init->bootCallbacks1 = tmpBootCb;
    g_init->curBootCallback1 = tmpBootCb;
    
    SceBootCallback bootCallbacks2[bootInfo->numModules];
    tmpBootCb = (SceBootCallback *)UPALIGN16((u32)bootCallbacks2);
    tmpBootCb->bootCBFunc = NULL;
    g_init->bootCallbacks2 = tmpBootCb;
    g_init->curBootCallback2 = tmpBootCb;
    
    g_init->lptSummary = bootInfo->unk76;
    
    if (bootInfo->unk24 == 0) {
        SceLoadCoreBootModuleInfo *mod = &bootInfo->modules[bootInfo->numModules - 1];
        if (mod->attr & 2)
            g_init->apiType = mod->bootData;

        if (sceKernelIsDevelopmentToolMode() != SCE_FALSE && sceKernelDipsw(29) != 1)
            g_init->applicationType = SCE_INIT_APPLICATION_GAME;
        else
            g_init->applicationType = SCE_INIT_APPLICATION_VSH;
    } else {
        if (bootInfo->unk24 >= 3) {
            g_init->applicationType = SCE_INIT_APPLICATION_VSH;
            StopInit(__FUNCTION__, __LINE__);
        }
        g_init->apiType = bootInfo->modules[bootInfo->numModules - 1].bootData;
        switch (g_init->apiType) {
        case SCE_INIT_APITYPE_GAME_EBOOT: case SCE_INIT_APITYPE_GAME_BOOT: case SCE_INIT_APITYPE_EMU_EBOOT_MS: 
        case SCE_INIT_APITYPE_EMU_BOOT_MS: case SCE_INIT_APITYPE_EMU_EBOOT_EF: case SCE_INIT_APITYPE_EMU_BOOT_EF: 
        case SCE_INIT_APITYPE_NPDRM_MS: case SCE_INIT_APITYPE_NPDRM_EF:
        case SCE_INIT_APITYPE_DISC: case SCE_INIT_APITYPE_DISC_DEBUG: case SCE_INIT_APITYPE_DISC_EMU_MS1: 
        case SCE_INIT_APITYPE_DISC_EMU_MS2: case SCE_INIT_APITYPE_DISC_EMU_EF1: case SCE_INIT_APITYPE_DISC_EMU_EF2:
        case SCE_INIT_APITYPE_USBWLAN: case SCE_INIT_APITYPE_USBWLAN_DEBUG: case 0x132: case 0x133:
        case SCE_INIT_APITYPE_MS2: case SCE_INIT_APITYPE_MS3: case SCE_INIT_APITYPE_MS6:
        case SCE_INIT_APITYPE_EF2: case SCE_INIT_APITYPE_EF3: case SCE_INIT_APITYPE_EF6:
        case 0x160:
        case SCE_INIT_APITYPE_MLNAPP_MS: case SCE_INIT_APITYPE_MLNAPP_EF:
            g_init->applicationType = SCE_INIT_APPLICATION_GAME;
            break;
        case SCE_INIT_APITYPE_DISC_UPDATER:
        case SCE_INIT_APITYPE_MS1:
        case SCE_INIT_APITYPE_EF1:
            g_init->applicationType = SCE_INIT_APPLICATION_UPDATER;
            break;
        case SCE_INIT_APITYPE_MS4:
        case SCE_INIT_APITYPE_EF4:
            g_init->applicationType = SCE_INIT_APPLICATION_APP;
            break;
        case SCE_INIT_APITYPE_MS5:
        case SCE_INIT_APITYPE_EF5:
            g_init->applicationType = SCE_INIT_APPLICATION_POPS;
            break;
        default:
            g_init->applicationType = SCE_INIT_APPLICATION_VSH;
            break;
        }
    }
    switch (g_init->apiType) {
    case SCE_INIT_APITYPE_EMU_EBOOT_EF: case SCE_INIT_APITYPE_EMU_BOOT_EF: case SCE_INIT_APITYPE_NPDRM_EF: 
    case SCE_INIT_APITYPE_DISC_EMU_EF1: 
    case 0x132: case 0x133: 
    case SCE_INIT_APITYPE_EF2: case SCE_INIT_APITYPE_EF3: case SCE_INIT_APITYPE_EF4: 
    case SCE_INIT_APITYPE_EF5: case SCE_INIT_APITYPE_EF6: 
    case SCE_INIT_APITYPE_MLNAPP_EF:
        SysMemForKernel_40B744A4(127);
        break;
    default:
        SysMemForKernel_40B744A4(0);
        break;
    }
        
    if (bootInfo->loadedModules < bootInfo->numModules) {
        SceSysmemPartitionInfo partInfo;
        partInfo.size = sizeof(SceSysmemPartitionInfo);
        sceKernelQueryMemoryPartitionInfo(SCE_KERNEL_VSHELL_PARTITION, &partInfo);
        s32 curMod = bootInfo->numModules; //0x00001478
        
        s32 i;
        for (i = bootInfo->loadedModules; i < (s32)bootInfo->numModules; i++) {
            if (bootInfo->modules[i].attr & 0x1) {
                curMod = i;
                break;
            }
        }
        void *buf = UCACHED(partInfo.startAddr);
        SceLoadCoreBootModuleInfo *mod;
        for (i = curMod; i < (s32)bootInfo->numModules; i++) {
            mod = &bootInfo->modules[i];
            if (sub_0CFC(mod) == SCE_TRUE) {
                sceKernelMemmove(buf, mod->modBuf, mod->modSize);
                mod->modBuf = buf;
                buf = (void *)UPALIGN64((u32)buf + mod->modSize);
            }
        }
        
        for (i = bootInfo->numModules - 1; i >= 0; i--) {
            SceLoadCoreBootModuleInfo *mod = &bootInfo->modules[i];
            if (mod->attr & 2) {
                foundMod = SCE_TRUE;
                
                SceUID protId = bootInfo->modProtId;
                if (protId != -1) {
                    SceLoadCoreProtectInfo *prot = &bootInfo->protects[protId];
                    mod->modSize = prot->size;
                    mod->modBuf = (u8 *)prot->addr;
                    if (GET_PROTECT_INFO_TYPE(prot->attr) & SCE_PROTECT_INFO_TYPE_FILE_NAME) {
                        mod->attr |= 4;
                        g_init->fileModAddr = (void *)prot->addr;
                    }
                }
                protId = bootInfo->modArgProtId;
                if (protId != -1) { 
                    SceLoadCoreProtectInfo *prot = &bootInfo->protects[protId];
                    mod->argPartId = prot->partId;
                    mod->argSize = prot->size;
                }
            }
            if (sub_0CFC(mod) == SCE_FALSE)
                mod->bootData = 0;
            else {
                SceUID partId = sceKernelAllocPartitionMemory(SCE_KERNEL_PRIMARY_USER_PARTITION, "backup area", 1, 
                        mod->modSize, 0);
                void *addr = sceKernelGetBlockHeadAddr(partId);
                sceKernelMemmoveWithFill(addr, mod->modBuf, mod->modSize, 0);
                mod->bootData = partId;
                mod->modBuf = addr;
            }
        }
        
        for (i = bootInfo->loadedModules; i < (s32)bootInfo->numModules; i++) {
            if (i == curMod && g_init->applicationType == SCE_INIT_APPLICATION_GAME) {
                /*
                 * "Encrypted" string for "flash2:/opnssmp.bin"
                 * The file opnssmp.bin is used for EBOOT decryption.
                 * 
                 * "Decrypt" the string to load the module opnssmp.bin.
                 */
                s32 opnssmpFile[] = { 0x8C9E9399, 0xD0C5CD97, 0x8C918F90, 0xD18F928C, 0xFF91969D };
                u32 j;
                for (j = 0; j < sizeof opnssmpFile; j++)
                    ((char *)opnssmpFile)[j] = ~((char *)opnssmpFile)[j];
                    
                SceKernelGameInfo *gameInfo = sceKernelGetGameInfo();
                void *argp = (void *)gameInfo->unk216;
                SceIoStat stat;
                if (argp != NULL && sceIoGetstat((char *)opnssmpFile, &stat) >= SCE_ERROR_OK) {
                    SceUID modId = sceKernelLoadModule((char *)opnssmpFile, 0, NULL);
                    if (modId >= SCE_ERROR_OK) {
                        if (sceKernelStartModule(modId, sizeof argp, &argp, &modStatus1, NULL) < SCE_ERROR_OK) {
                            sceKernelUnloadModule(modId);
                            sceIoRemove((char *)opnssmpFile);
                        }
                    }
                }
            }
            SceLoadCoreBootModuleInfo *mod = &bootInfo->modules[i];
            if (mod->bootData != 0)
                sceKernelFreePartitionMemory(mod->bootData);

            SceUID modId;
            if ((mod->attr & 2) == 0) {
                SceKernelLMOption options = {
                    .size = sizeof(SceKernelLMOption),
                    .mpIdText = (mod->attr >> 16) & 0xFF,
                    .mpIdData = (mod->attr >> 16) & 0xFF,
                    .position = SCE_KERNEL_LM_POS_LOW,
                    .access = SCE_KERNEL_LM_ACCESS_NOSEEK,
                };
                if ((mod->attr & 4) == 0) {
                    modId = sceKernelLoadModuleBufferBootInitBtcnf(mod->modSize, (u32 *)mod->modBuf, 0, &options, 
                            (mod->attr >> 8) & 1);
                    if (modId > SCE_ERROR_OK)
                        sceKernelMemset32(mod->modBuf, 0, mod->modSize);
                } else
                    modId = sceKernelLoadModuleBootInitBtcnf((u32 *)mod->modBuf, 0, &options);
            } else {
                memmove(&g_mod, mod, sizeof(SceLoadCoreBootModuleInfo));
                
                CleanupPhase1(bootInfo);
                ExitCheck();
                
                if (sub_0DD0(bootInfo) == 0) {
                    if (g_mod.attr & 4)
                        modId = LoadModuleAnchorInBtcnf((char *)g_mod.modBuf); //TODO: Shouldn't it be g_mod.modPath?
                    else {
                        modId = LoadModuleBufferAnchorInBtcnf(g_mod.modBuf, (mod->attr >> 8) & 1);
                        if (modId > SCE_ERROR_OK)
                            sceKernelMemset32(g_mod.modBuf, 0, g_mod.modSize);
                    }
                    if (modId < SCE_ERROR_OK)
                        ExitInit(modId);
                } else
                    modId = SCE_ERROR_KERNEL_ERROR;
            }
            if (mod->attr & 2) {
                ClearFreeBlock();
                sub_05F0();
            }
            if (modId >= 0) {
                s32 status;
                if (mod->argPartId == 0 || mod->argSize == 0)
                    status = sceKernelStartModule(modId, 0, NULL, &modStatus2, NULL);
                else
                    status = sceKernelStartModule(modId, mod->argSize, sceKernelGetBlockHeadAddr(mod->argPartId), &modStatus2, 
                            NULL);
                if (status < SCE_ERROR_OK && (mod->attr & 2))
                    ExitInit(status);
            } else if ((mod->attr & 2) == 0 || sub_0DD0(bootInfo) == 0)
                StopInit(__FUNCTION__, __LINE__);
            
            if (mod->argPartId != 0)
                sceKernelFreePartitionMemory(mod->argPartId);
            
            bootInfo->loadedModules++;
            if (mod->attr & 2)
                break;
        }
    }
    /* 
     * All modules have been booted now:
     *      Step 1: Free the memory containing the boot information.
     */
    CleanupPhase1(bootInfo);
    CleanupPhase2(bootInfo);
    if (foundMod == SCE_FALSE)
        ClearFreeBlock();

    printf("Loading all modules ... Ready\n");
    
    if (sceKernelGetSystemStatus() == 0)
        sub_05F0();

    /* Step 2: Call the last module boot callbacks. */
    if (foundMod == SCE_TRUE)
        invoke_init_callback(4);

    void *ptr = sceKernelDeci2pReferOperations();
    if (ptr != NULL) {
        void (*func)(s32) = (void (*)(s32))*(s32 *)(ptr + 8);
        func(2);
    }
    
    /* Step3: Init stops and unloads itself now. */
    sceKernelStopUnloadSelfModuleWithStatusKernel(1, 0, NULL, NULL, NULL);
}

#ifdef INSTALLER
/* Reference to patch.S (needed because when patching init, SystemControl only has access to its bootstart's address)*/
asm(".word init_patch\n");
#endif

/* Setup the boot process of the rest of the PSP kernel modules. */
s32 InitInit(SceSize argc __attribute__((unused)), void *argp)
{
    SceUID threadId;
     
    SceLoadCoreBootInfo *bootInfo = argp;
    g_MemBase = bootInfo->memBase;
    g_MemSize = bootInfo->memSize;
    
    g_init = sceKernelQueryInitCB();
    InitCBInit(bootInfo);
    
    threadId = sceKernelGetThreadId();
    if (threadId < SCE_ERROR_OK)
        return threadId;
    
    ProtectHandling(bootInfo);
    
    SceLoadCoreBootModuleInfo *modules = bootInfo->modules;
    if (bootInfo->unk24 == 0 && (u32)modules->modBuf <= (SCE_USERSPACE_GAME_ADDR_K0 - 1)) {
        void *minStart = (void *)-1;
        void *maxEnd = NULL;
        
        s32 i;
        for (i = 0; i < (s32)bootInfo->numModules; i++) {
            void *start = modules[i].modBuf;
            void *end = start + modules[i].modSize;
            if (maxEnd < end)
                maxEnd = end;
            if (start < minStart)
                minStart = start;
        }
        minStart += 0x77A00000;
        sceKernelMemset32((void *)REBOOT_BASE_ADDR_K0, 0, (u32)minStart);
        sceKernelMemset32(maxEnd, 0, SCE_USERSPACE_ADDR_K0 - (u32)maxEnd);
    } else
        sceKernelMemset32((void *)REBOOT_BASE_ADDR_K0, 0, SCE_USERSPACE_ADDR_K0 - REBOOT_BASE_ADDR_K0);
    
    SceUID id = sceKernelCreateThread("SceKernelInitThread", (SceKernelThreadEntry)InitThreadEntry, 
            SCE_KERNEL_MODULE_INIT_PRIORITY, 0x4000, 0, NULL);
    
    u32 threadArgs[2] = { 
        threadId, 
        (u32)bootInfo, 
    };
    return sceKernelStartThread(id, sizeof threadArgs, threadArgs);
}

