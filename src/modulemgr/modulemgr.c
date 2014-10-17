/* Copyright (C) 2011 - 2014 The uOFW team
   See the file COPYING for copying permission.
*/

#include <common_imp.h>
#include <interruptman.h>
#include <iofilemgr_kernel.h>
#include <loadcore.h>
#include <modulemgr.h>
#include <modulemgr_nids.h>
#include <modulemgr_options.h>
#include <sysmem_kernel.h>
#include <threadman_kernel.h>

#include "loadModuleChecks_inline.h"
#include "modulemgr_int.h"

#define GET_MCB_STATUS(status)  (status & 0xF)
#define SET_MCB_STATUS(v, m)    (v = (v & ~0xF) | m)

#define FILE_USER_ACCESS_PERMISSIONS    (SCE_STM_RUSR | SCE_STM_XUSR | SCE_STM_XGRP | SCE_STM_XOTH)

typedef struct {
    SceUID threadId; // 0
    SceUID semaId; // 4
    SceUID eventId; // 8
    SceUID userThreadId; // 12
    u32 unk16;
    u32 unk20;
    u32 unk24;
    u32 unk28;
    void (*npDrmGetModuleKeyFunction)(s32 fd, void *, void *); // 32
    u32 unk36;
} SceModuleManagerCB;

typedef struct {
    u8 modeStart; //0 The Operation to start on, Use one of the ModuleMgrExeModes modes
    u8 modeFinish; //1 The Operation to finish on, Use one of the ModuleMgrExeModes modes
    u8 position; //2
    u8 access; //3
    SceUID *returnId; //4
    u32 *status;
    SceModule *pMod; //12
    SceLoadCoreExecFileInfo *execInfo; //16
    u32 apiType; //20
    SceUID fd; // 24
    s32 threadPriority; //28
    u32 threadAttr; //32
    u32 mpIdText; // 36
    u32 mpIdData; // 40
    SceUID threadMpIdStack; //44
    SceSize stackSize; //48
    SceUID modId; //52
    SceUID callerModId; //56
    void *file_buffer; //60
    u32 unk64;
    SceSize argSize; //68
    void *argp; //72
    u32 unk76;
    u32 unk80;
    s32 *pStatus; //84
    u32 eventId; //88
    u32 unk96;
    u32 unk100;
    u32 unk104;
    u32 unk108;
    u32 unk112;
    u32 unk116;
    u32 unk120;
    u32 unk124;
    // TODO: Add #define for size. 
    char secureInstallId[16]; // 128
    SceUID memBlockId; //144
    u32 unk148;
    SceOff memBlockOffset; // 152
} SceModuleMgrParam; //size = 160

enum ModuleMgrExecModes {
    CMD_LOAD_MODULE, //0
    CMD_RELOCATE_MODULE, //1
    CMD_START_MODULE, //2
    CMD_STOP_MODULE, //3
    CMD_UNLOAD_MODULE, //4
};

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
static s32 exe_thread(SceSize args __attribute__((unused)), void *argp)
{
    SceModuleMgrParam *modParams;
    SceLoadCoreExecFileInfo execInfo;
    s32 status;
    
    status = SCE_ERROR_OK;
    modParams = (SceModuleMgrParam *)argp;
    
    for (int i = 0; i < sizeof(SceLoadCoreExecFileInfo) / sizeof(u32); i++)
        ((u32 *)execInfo)[i] = 0; // 0x000001A8
    
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
            for (int i = 0; i < sizeof(SceLoadCoreExecFileInfo) / sizeof(u32); i++) //0x00000238
                 ((u32 *)execInfo)[i] = 0; // 0x000001A8
        }
                 
        modParams->execInfo = &execInfo;
            
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

/**
 * Load a module by path specifying the api type
 * 
 * @param apiType The api type of the module
 * @param path A pointer to a '\0' terminated string containing the path to the module
 * @param flags Unused, pass 0
 * @param pOpt A pointer to a SceKernelLMOption structure, which holds various options about the way to load the module. 
 *             Pass NULL if you don't want to specify any option.
 *
 * @return SCE_ERROR_OK on success, < 0 on error.
 * @return SCE_ERROR_KERNEL_CANNOT_BE_CALLED_FROM_INTERRUPT if function was called in an interruption.
 * @return SCE_ERROR_KERNEL_ILLEGAL_PERMISSION_CALL if function was called from a user context
 * @return SCE_ERROR_KERNEL_ILLEGAL_ADDR if the provided pointers are NULL or can't be accessed from the current context.
 * @return SCE_ERROR_KERNEL_UNKNOWN_MODULE_FILE if the path contains a '%' (protection against formatted strings attack)
 * @return SCE_ERROR_KERNEL_ILLEGAL_SIZE if SdkVersion >= 2.80 and opt->size != sizeof(SceKernelLMOption)
 * @return One of the errors of sceIoOpen() if failed
 * @return One of the errors of sceIoIoctl() if failed
 */
// Subroutine ModuleMgrForKernel_2B7FC10D - Address 0x000004A8            
s32 sceKernelLoadModuleForLoadExecForUser(s32 apiType, const char *file, s32 flags __attribute__((unused)), 
        const SceKernelLMOption *pOpt)
{
    s32 oldK1;
    s32 status;
    s32 ioctlCmd;
    SceUID fd;
    SceModuleMgrParam modParams;
    
    oldK1 = pspShiftK1(); //0x000004B4
    
    if (sceKernelIsIntrContext()) { //0x000004E0
        pspSetK1(oldK1);
        return SCE_ERROR_KERNEL_CANNOT_BE_CALLED_FROM_INTERRUPT;
    }
    
    //0x000004EC - 0x000006A8
    if ((status = _checkCallConditionKernel()) < 0 || (status = _checkPathConditions(file)) < 0 
            || (status = _checkLMOptionConditions(pOpt)) < 0) {
        pspSetK1(oldK1);
        return status;
    }
    
    fd = sceIoOpen(file, SCE_O_FGAMEDATA | SCE_O_RDONLY, FILE_USER_ACCESS_PERMISSIONS); //0x00000528
    if (fd < 0) { //0x00000534
        pspSetK1(oldK1);
        return fd;
    }
    
    switch (apiType) { //0x00000568
    case SCE_INIT_APITYPE_GAME_EBOOT:
    case SCE_INIT_APITYPE_EMU_EBOOT_MS:
    case SCE_INIT_APITYPE_EMU_EBOOT_EF:
        ioctlCmd = 0x208010; // 0x00000568 & 0x0000056C
        break;
    case SCE_INIT_APITYPE_GAME_BOOT:
    case SCE_INIT_APITYPE_EMU_BOOT_MS:
    case SCE_INIT_APITYPE_EMU_BOOT_EF:
        ioctlCmd = 0x208011; //0x00000630 & 0x00000638
        break;
    }
    status = sceIoIoctl(fd, ioctlCmd, NULL, 0, NULL, 0); //0x00000580
    if (status < 0) { //0x0000058C
        sceIoClose(fd); //0x00000600
        pspSetK1(oldK1);
        return SCE_ERROR_KERNEL_PROHIBIT_LOADMODULE_DEVICE;
    }
    
    pspClearMemory32(&modParams, sizeof(modParams)); //0x000005A0
        
    modParams.apiType = apiType; // 0x000005A8
    modParams.modeFinish = CMD_RELOCATE_MODULE; // 0x000005B4
    modParams.modeStart = CMD_LOAD_MODULE; // 0x000005C0
    modParams.unk64 = 0; // 0x000005CC
    modParams.fd = fd; // 0x000005D8
    modParams.unk124 = 0; // 0x000005E0
        
    status = sceIoIoctl(fd, 0x208081, NULL, 0, NULL, 0); //0x000005DC
    if (status >= 0) //0x000005E4
        modParams.unk100 = 0x10; // 0x000005EC
        
    status = _loadModuleByBufferID(&modParams, pOpt); //0x000005F4
        
    sceIoClose(fd); //0x00000600
    pspSetK1(oldK1);
    return status;
}

/**
 * Load a module by path
 * 
 * @param path A pointer to a '\0' terminated string containing the path to the module
 * @param flags Unused, pass 0
 * @param pOpt A pointer to a SceKernelLMOption structure, which holds various options about the way to load the module. Pass NULL if you don't want to specify any option.
 *
 * @return SCE_ERROR_OK on success, < 0 on error.
 * @return SCE_ERROR_KERNEL_CANNOT_BE_CALLED_FROM_INTERRUPT if function was called in an interruption.
 * @return SCE_ERROR_KERNEL_ILLEGAL_PERMISSION_CALL if function was not called from a user context
 * @return SCE_ERROR_KERNEL_ILLEGAL_ADDR if the provided pointers are NULL or can't be accessed from the current context.
 * @return SCE_ERROR_KERNEL_UNKNOWN_MODULE_FILE if the path contains a '%' (protection against formatted strings attack)
 * @return SCE_ERROR_KERNEL_ILLEGAL_SIZE if SdkVersion >= 2.71 and opt->size != sizeof(SceKernelLMOption)
 * @return One of the errors of sceIoOpen() if failed
 * @return One of the errors of sceIoIoctl() if failed
 */
// Subroutine sceKernelLoadModule - Address 0x000006B8 
s32 sceKernelLoadModuleForUser(const char *path, u32 flags __attribute__((unused)),
    const SceKernelLMOption *pOpt)
{
    s32 oldK1;
    s32 fd;
    s32 status;
    SceModuleMgrParam modParams;

    oldK1 = pspShiftK1(); // 0x000006C4

    // Cannot be called in an interruption
    if (sceKernelIsIntrContext()) { // 0x000006E0
        pspSetK1(oldK1);
        return SCE_ERROR_KERNEL_CANNOT_BE_CALLED_FROM_INTERRUPT;
    }
    
    //0x000006F8 - 0x0000087C
    if ((status = _checkCallConditionUser()) < 0 || (status = _checkPathConditions(path)) < 0 
            || (status = _checkLMOptionConditions(pOpt)) < 0) {
        pspSetK1(oldK1);
        return status;
    }

    fd = sceIoOpen(path, SCE_O_FGAMEDATA | SCE_O_RDONLY, FILE_USER_ACCESS_PERMISSIONS); // 0x00000734
    if (fd < 0) { // 0x00000740
        pspSetK1(oldK1);
        return fd;
    }

    status = sceIoIoctl(fd, 0x208001, NULL, 0, NULL, 0); // 0x00000760
    if (status < 0) { // 0x0000076C
        sceIoClose(fd); // 0x000007E0
        pspSetK1(oldK1);
        return SCE_ERROR_KERNEL_PROHIBIT_LOADMODULE_DEVICE;
    }

    pspClearMemory32(&modParams, sizeof(modParams)); // 0x00000784

    modParams.modeFinish = CMD_RELOCATE_MODULE; // 0x00000790
    modParams.apiType = 0x10; // 0x0000079C
    modParams.modeStart = CMD_LOAD_MODULE; // 0x000007A8
    modParams.unk64 = 0; // 0x000007B4
    modParams.fd = fd; // 0x000007BC
    modParams.unk124 = 0; // 0x000007C4

    status = sceIoIoctl(fd, 0x208081, NULL, 0, NULL, 0); // 0x000007C0
    if (status >= 0) // 0x000007C8
        modParams.unk100 = 0x10; // 0x000007CC
    
    status = _loadModuleByBufferID(&modParams, pOpt); // 0x000007D4
    
    sceIoClose(fd); // 0x000007E0
    pspSetK1(oldK1);
    return status;
}

/**
 * Load a module by file descriptor
 * 
 * @param inputId The file descriptor that was obtained when opening the module with sceIoOpen()
 * @param flag Unused, pass 0
 * @param pOpt A pointer to a SceKernelLMOption structure, which holds various options about the way to load the module. Pass NULL if you don't want to specify any option.
 *
 * @return SCE_ERROR_OK on success, < 0 on error.
 * @return SCE_ERROR_KERNEL_CANNOT_BE_CALLED_FROM_INTERRUPT if function was called in an interruption.
 * @return SCE_ERROR_KERNEL_ILLEGAL_PERMISSION_CALL if function was not called from a user context
 * @return SCE_ERROR_KERNEL_ILLEGAL_ADDR if the pointer to SceKernelLMOption can't be accessed from the current context.
 * @return SCE_ERROR_KERNEL_ILLEGAL_SIZE if SdkVersion >= 2.80 and opt->size != sizeof(SceKernelLMOption)
 * @return One of the errors of sceIoValidateFd() if failed
 * @return SCE_ERROR_KERNEL_PROHIBIT_LOADMODULE_DEVICE if sceIoIoctl() failed
 */
// Subroutine ModuleMgrForUser_B7F46618 - Address 0x0000088C
// TODO: Rename to sceKernelLoadModuleByIDForUser() ?
s32 sceKernelLoadModuleByID(SceUID inputId, u32 flag __attribute__((unused)),
        const SceKernelLMOption *pOpt)
{
    s32 oldK1;
    s32 status;
    SceModuleMgrParam modParams;
    
    oldK1 = pspShiftK1(); //0x00000898
    
    if (sceKernelIsIntrContext()) { //0x000008C0
        pspSetK1(oldK1);
        return SCE_ERROR_KERNEL_CANNOT_BE_CALLED_FROM_INTERRUPT;
    }
    
    //0x000008CC - 0x00000944
    if ((status = _checkCallConditionUser()) < 0 || (status = _checkLMOptionConditions(pOpt)) < 0) {
        pspSetK1(oldK1);
        return status;
    }
    
    status = sceIoValidateFd(inputId, 4); //0x00000950
    if (status < SCE_ERROR_OK) { //0x00000958
        pspSetK1(oldK1);
        return status;
    }
    
    status = sceIoIoctl(inputId, 0x00208001, NULL, 0, NULL, 0); // 0x00000978
    if (status < 0) { // 0x00000984
        pspSetK1(oldK1);
        return SCE_ERROR_KERNEL_PROHIBIT_LOADMODULE_DEVICE;
    }
    
    pspClearMemory32(&modParams, sizeof(modParams)); //0x0000099C
        
    modParams.apiType = 0x10; // 0x000009C8
    modParams.modeFinish = CMD_RELOCATE_MODULE; // 0x000009C4
    modParams.modeStart = CMD_LOAD_MODULE; // 0x000009CC
    modParams.unk64 = 0; // 0x000005CC
    modParams.fd = inputId; // 0x000005D8
    modParams.unk124 = 0; // 0x000009DC
        
    status = sceIoIoctl(inputId, 0x208081, NULL, 0, NULL, 0); //0x000009D8
    if (status >= 0) //0x000009E0
        modParams.unk100 = 0x10; // 0x000009E4
        
    status = _loadModuleByBufferID(&modParams, pOpt); //0x000009EC
        
    pspSetK1(oldK1);
    return status;
}

/**
 * Load a module by path specifying a block and an offset
 * 
 * @param path The file descriptor that was obtained when opening the module with sceIoOpen()
 * @param block Memory block where the module will be loaded
 * @param Aligned-to-64-offset (from the memory block) where the module will be loaded
 *
 * @return SCE_ERROR_OK on success, < 0 on error.
 * @return SCE_ERROR_KERNEL_CANNOT_BE_CALLED_FROM_INTERRUPT if function was called in an interruption.
 * @return SCE_ERROR_KERNEL_ILLEGAL_PERMISSION_CALL if function was not called from a user context
 * @return SCE_ERROR_KERNEL_ILLEGAL_ADDR if the path pointer is NULL or can't be accessed from the current context, or if the address corresponding to the block can't be accessed from the current context.
 * @return SCE_ERROR_KERNEL_UNKNOWN_MODULE_FILE if the path contains a '%' (protection against formatted strings attack)
 * @return SCE_ERROR_KERNEL_INVALID_ARGUMENT if the offset is incorrect or not aligned to 64
 * @return One of the errors of sceIoOpen() if failed
 * @return One of the errors of sceIoValidateFd() if failed
 * @return SCE_ERROR_KERNEL_PROHIBIT_LOADMODULE_DEVICE if sceIoIoctl() failed
 */
// Subroutine ModuleMgrForUser_E4C4211C - Address 0x000009FC
s32 sceKernelLoadModuleWithBlockOffset(const char *path, SceUID block, SceOff offset)
{
    s32 oldK1;
    s32 status;
    SceUID fd;
    SceSysmemMemoryBlockInfo blkInfo;
    SceModuleMgrParam modParams;
    
    oldK1 = pspShiftK1();  //0x00000898
    
    if (sceKernelIsIntrContext()) { //0x00000A08
        pspSetK1(oldK1);
        return SCE_ERROR_KERNEL_CANNOT_BE_CALLED_FROM_INTERRUPT;
    }
    
    //0x00000A50 - 0x00000AB8
    if ((status = _checkCallConditionUser()) < 0 || (status = _checkPathConditions(path)) < 0) {
        pspSetK1(oldK1);
        return status;
    }
    
    status = sceKernelQueryMemoryBlockInfo(block, &blkInfo); //0x00000ACC
    if (status < SCE_ERROR_OK) { // 0x00000AD4
        pspSetK1(oldK1);
        return status; 
    }
    //0x00000B04 - 0x00000B3C
    status = _checkMemoryBlockInfoConditions(&blkInfo, offset);
    if (status < 0) {
        pspSetK1(oldK1);
        return status;
    }
    
    fd = sceIoOpen(path, SCE_O_FGAMEDATA | SCE_O_RDONLY, FILE_USER_ACCESS_PERMISSIONS); //0x00000B5C
    if (fd < 0) { //0x00000B68
        pspSetK1(oldK1);
        return fd;
    }  
    status = sceIoIoctl(fd, 0x208001, NULL, 0, NULL, 0); // 0x00000B88
    if (status < 0) { // 0x00000B94
        pspSetK1(oldK1);
        return SCE_ERROR_KERNEL_PROHIBIT_LOADMODULE_DEVICE;
    }
    
    pspClearMemory32(&modParams, sizeof(modParams)); //0x00000BAC
         
    modParams.apiType = 0x10; // 0x00000BC4
    modParams.modeFinish = CMD_RELOCATE_MODULE; // 0x00000BB8
    modParams.modeStart = CMD_LOAD_MODULE; // 0x00000BD0
    modParams.unk64 = 0; // 0x00000BDC
    modParams.fd = fd; // 0x00000BE4
    modParams.unk124 = 0; // 0x00000BEC
    
    status = sceIoIoctl(fd, 0x208081, NULL, 0, NULL, 0); //0x00000BE8
    if (status >= 0) //0x00000BF0
        modParams.unk100 = 0x10; // 0x00000BF4
        
    // 0x00000BFC
    modParams.memBlockId = block;
    modParams.memBlockOffset = offset;
    status = _loadModuleByBufferID(&modParams, NULL); //0x00000C08
    
    sceIoClose(fd); //0x00000C14
    pspSetK1(oldK1);
    return status;
}

/**
 * Load a module by file descriptor specifying a block and an offset
 * 
 * @param inputId The file descriptor that was obtained when opening the module with sceIoOpen()
 * @param block Memory block where the module will be loaded
 * @param Aligned-to-64-offset (from the memory block) where the module will be loaded

 * @return SCE_ERROR_OK on success, < 0 on error.
 * @return SCE_ERROR_KERNEL_CANNOT_BE_CALLED_FROM_INTERRUPT if function was called in an interruption.
 * @return SCE_ERROR_KERNEL_ILLEGAL_PERMISSION_CALL if function was not called from a user context
 * @return SCE_ERROR_KERNEL_ILLEGAL_ADDR if the address corresponding to the block can't be accessed from the current context.
 * @return SCE_ERROR_KERNEL_INVALID_ARGUMENT if the offset is incorrect or not aligned to 64
 * @return One of the errors of sceIoValidateFd() if failed
 * @return SCE_ERROR_KERNEL_PROHIBIT_LOADMODULE_DEVICE if sceIoIoctl() failed
 */
// Subroutine ModuleMgrForUser_FBE27467 - Address 0x00000C34
s32 sceKernelLoadModuleByIDWithBlockOffset(SceUID inputId, SceUID block, SceOff offset)
{
    s32 oldK1;
    s32 status;
    SceSysmemMemoryBlockInfo blkInfo;
    SceModuleMgrParam modParams;
    
    oldK1 = pspShiftK1(); //0x00000C40
    
    if (sceKernelIsIntrContext()) { //0x00000C6C
        pspSetK1(oldK1);
        return SCE_ERROR_KERNEL_CANNOT_BE_CALLED_FROM_INTERRUPT;
    }
    status = _checkCallConditionUser(); //0x00000C84
    if (status < 0) {
         pspSetK1(oldK1);
        return status;
    }
    
    status = sceKernelQueryMemoryBlockInfo(block, &blkInfo); //0x00000CD0
    if (status < SCE_ERROR_OK) { // 0x00000AD4
        pspSetK1(oldK1);
        return status; 
    }
    //0x00000D00 - 0x00000D38
    status = _checkMemoryBlockInfoConditions(&blkInfo, offset);
    if (status < 0) {
        pspSetK1(oldK1);
        return status;
    }
    
    status = sceIoValidateFd(inputId, 4); //0x00000D50
    if (status < SCE_ERROR_OK) { //0x00000D58
        pspSetK1(oldK1);
        return status;
    }
     
    status = sceIoIoctl(inputId, 0x208001, NULL, 0, NULL, 0); // 0x00000D78
    if (status < 0) { // 0x00000D84
        pspSetK1(oldK1);
        return SCE_ERROR_KERNEL_PROHIBIT_LOADMODULE_DEVICE;
    }
    
    pspClearMemory32(&modParams, sizeof(modParams)); //0x00000D9C
         
    modParams.apiType = 0x10; // 0x00000DB4
    modParams.modeFinish = CMD_RELOCATE_MODULE; // 0x00000DA8
    modParams.modeStart = CMD_LOAD_MODULE; // 0x00000DC0
    modParams.unk64 = 0; // 0x00000DCC
    modParams.fd = inputId; // 0x00000DD4
    modParams.unk124 = 0; // 0x00000DDC
    
    status = sceIoIoctl(inputId, 0x208081, NULL, 0, NULL, 0); //0x00000DD8
    if (status >= 0) //0x00000DE0
        modParams.unk100 = 0x10; // 0x00000DE4
        
    // 0x00000DEC
    modParams.memBlockId = block;
    modParams.memBlockOffset = offset;
    status = _loadModuleByBufferID(&modParams, NULL); //0x00000DF8
    
    pspSetK1(oldK1);
    return status;
}

/**
 * Load a DNAS module by path and secureInstallId
 * 
 * @param path The file descriptor that was obtained when opening the module with sceIoOpen()
 * @param secureInstallId A pointer to a secure installation identifier string, which is an encryption key used to decrypt the module, example: "123456789abcdef123456789abcdef12". It is often used in games that encrypt their modules (and other files) to prevent unauthorized access. This makes reverse engineering slightly harder because one needs to find the keys first.
 * @param flag Unused, pass 0
 * @param opt A pointer to a SceKernelLMOption structure, which holds various options about the way to load the module. Pass NULL if you don't want to specify any option.

 * @return SCE_ERROR_OK on success, < 0 on error.
 * @return SCE_ERROR_KERNEL_CANNOT_BE_CALLED_FROM_INTERRUPT if function was called in an interruption.
 * @return SCE_ERROR_KERNEL_ILLEGAL_PERMISSION_CALL if function was not called from a user context
 * @return SCE_ERROR_KERNEL_ILLEGAL_ADDR if the path or secureInstallId or pOpt is NULL or can't be accessed from the current context.
 * @return SCE_ERROR_KERNEL_UNKNOWN_MODULE_FILE if the path contains a '%' (protection against formatted strings attack)
 * @return SCE_ERROR_KERNEL_INVALID_ARGUMENT if the offset is incorrect or not aligned to 64
 * @return SCE_ERROR_KERNEL_ILLEGAL_SIZE if SdkVersion >= 2.80 and opt->size != sizeof(SceKernelLMOption)
 * @return One of the errors of sceIoOpen() if failed
 * @return One of the errors of sceIoIoctl() if failed
 * @return SCE_ERROR_KERNEL_PROHIBIT_LOADMODULE_DEVICE if sceIoIoctl() failed
 */
// Subroutine ModuleMgrForUser_FEF27DC1 - Address 0x00000E18
s32 sceKernelLoadModuleDNAS(const char *path, const char *secureInstallId, s32 flag __attribute__((unused)), 
        const SceKernelLMOption *pOpt)
{
    s32 oldK1;
    s32 fd;
    s32 status;
    SceModuleMgrParam modParams;
    
    oldK1 = pspShiftK1(); // 0x00000E24

    // Cannot be called in an interrupt
    if (sceKernelIsIntrContext()) { // 0x00000E48
        pspSetK1(oldK1);
        return SCE_ERROR_KERNEL_CANNOT_BE_CALLED_FROM_INTERRUPT;
    }
    
    //0x00000E60 - 0x00001050
    if ((status = _checkCallConditionUser()) < 0 || (status = _checkPathConditions(path)) < 0 
            || (status = _checkSecureInstalledIdConditions(secureInstallId)) < 0 
            || (status = _checkLMOptionConditions(pOpt)) < 0) {
        pspSetK1(oldK1);
        return status;
    }

    fd = sceIoOpen(path, SCE_O_FGAMEDATA | SCE_O_UNKNOWN0 | SCE_O_RDONLY, 
            FILE_USER_ACCESS_PERMISSIONS); // 0x00000EB8
    if (fd < 0) { // 0x00000EC4
        pspSetK1(oldK1);
        return fd;
    }

    // TODO: Set ioctl command to SCE_GAMEDATA_SET_SECURE_INSTALL_ID
    status = sceIoIoctl(fd, 0x4100001, secureInstallId, 16, NULL, 0); // 0x00000EE4
    if (status < 0) { // 0x00000EEC
        sceIoClose(fd); // 0x00000FDC
        pspSetK1(oldK1);
        return status;
    }

    status = sceIoIoctl(fd, 0x208002, NULL, 0, NULL, 0); // 0x00000F14
    if (status < 0) { // 0x00000F20
        sceIoClose(fd); //0x00000FA8
        pspSetK1(oldK1);
        return SCE_ERROR_KERNEL_PROHIBIT_LOADMODULE_DEVICE;
    }
    
    pspClearMemory32(&modParams, sizeof(modParams)); // 0x00000F34
         
    modParams.apiType = 0x13; // 0x00000F44
    modParams.modeFinish = CMD_RELOCATE_MODULE; // 0x00000F50
    modParams.modeStart = CMD_LOAD_MODULE; // 0x00000F5C
    modParams.unk64 = 0; // 0x00000F68
    modParams.fd = fd; // 0x00000F70
    modParams.unk124 = 0; // 0x00000F78
    
    status = sceIoIoctl(fd, 0x208081, NULL, 0, NULL, 0); //0x00000F74
    if (status >= 0) //0x00000F7C
        modParams.unk100 = 0x10; // 0x00000DE4
    
    memcpy(&modParams.secureInstallId, secureInstallId, 16) //0x00000F90
    
    status = _loadModuleByBufferID(&modParams, pOpt); // 0x00000F9C
    
    sceIoClose(fd); // 0x00000FA8
    pspSetK1(oldK1);
    return status;
}

/**
 * Load an NPDRM SPRX module, sceNpDrmSetLicenseeKey() needs to be called first in order to set the key
 * 
 * @param path A pointer to a '\0' terminated string containing the path to the module
 * @param flag Unused, pass 0
 * @param pOpt A pointer to a SceKernelLMOption structure, which holds various options about the way to load the module. Pass NULL if you don't want to specify any option.

 * @return SCE_ERROR_OK on success, < 0 on error.
 * @return SCE_ERROR_KERNEL_CANNOT_BE_CALLED_FROM_INTERRUPT if function was called in an interruption.
 * @return SCE_ERROR_KERNEL_ILLEGAL_PERMISSION_CALL if function was not called from a user context
 * @return SCE_ERROR_KERNEL_ILLEGAL_ADDR if the path is NULL, or path/pOpt can't be accessed from the current context.
 * @return SCE_ERROR_KERNEL_UNKNOWN_MODULE_FILE if the path contains a '%' (protection against formatted strings attack)
 * @return SCE_ERROR_KERNEL_ILLEGAL_SIZE if SdkVersion >= 2.80 and opt->size != sizeof(SceKernelLMOption)
 * @return One of the errors of sceIoOpen() if failed
 * @return SCE_ERROR_KERNEL_ERROR If the callback npDrmGetModuleKeyFunction in the g_ModuleManager structure is NULL
 * @return One of the errors of sceIoIoctl() if failed
 * @return SCE_ERROR_KERNEL_PROHIBIT_LOADMODULE_DEVICE if sceIoIoctl() failed
 *
 * @see sceNpDrmSetLicenseeKey()
 */
// Subroutine ModuleMgrForUser_F2D8D1B4 - Address 0x00001060 
void sceKernelLoadModuleNpDrm(const char *path, s32 flags __attribute__((unused)), const SceKernelLMOption *pOpt)
{
    s32 oldK1;
    s32 fd;
    s32 status;
    char secInstallId[16];
    SceNpDrm npDrmData;
    SceModuleMgrParam modParams;
    
    oldK1 = pspShiftK1(); // 0x0000106C

    // Cannot be called in an interruption
    if (sceKernelIsIntrContext()) { // 0x00001094
        pspSetK1(oldK1);
        return SCE_ERROR_KERNEL_CANNOT_BE_CALLED_FROM_INTERRUPT;
    }
    
    //0x000010A0 - 0x0000122C
    if ((status = _checkCallConditionUser()) < 0 || (status = _checkPathConditions(path)) < 0 
            || (status = _checkLMOptionConditions(pOpt)) < 0) {
        pspSetK1(oldK1);
        return status;
    }

    fd = sceIoOpen(path, SCE_O_UNKNOWN0 | SCE_O_RDONLY, FILE_USER_ACCESS_PERMISSIONS); // 0x000010DC
    if (fd < 0) { // 0x000010E8
        pspSetK1(oldK1);
        return fd;
    }
    
    if (g_ModuleManager.npDrmGetModuleKeyFunction == NULL) { //0x000010FC
        sceIoClose(fd);
        pspSetK1(oldK1);
        return SCE_ERROR_KERNEL_ERROR;
    }   
    status = g_ModuleManager.npDrmGetModuleKeyFunction(fd, &secInstallId, &npDrmData); // 0x00001110
    if (status < 0) { // 0x00001118
        sceIoClose(fd);
        pspSetK1(oldK1);
        return status;
    }
    
    sceIoLseek(fd, npDrmData.fileOffset, SCE_SEEK_SET); // 0x0000112C

    status = sceIoIoctl(fd, 0x208002, NULL, 0, NULL, 0); // 0x0000114C
    if (status < 0) { // 0x00001158
        sceIoClose(fd);
        pspSetK1(oldK1);
        return SCE_ERROR_KERNEL_PROHIBIT_LOADMODULE_DEVICE;
    }

    pspClearMemory32(&modParams, sizeof(modParams)); // 0x0000116C

    modParams.apiType = 0x14; // 0x0000117C
    modParams.modeFinish = CMD_RELOCATE_MODULE; // 0x00001188
    modParams.modeStart = CMD_LOAD_MODULE; // 0x00001194
    modParams.unk64 = 0; // 0x000011A0
    modParams.fd = fd; // 0x000011A8
    modParams.unk124 = 0;

    status = sceIoIoctl(fd, 0x208081, NULL, 0, NULL, 0); // 0x000011AC
    if (status >= 0) // 0x000011B4
        modParams.unk100 = 0x10; // 0x000011C0
    
    memcpy(modParams.secureInstallId, secInstallId, 16); //0x000011C8
    
    status = _loadModuleByBufferID(&modParams, pOpt); // 0x000011D4
    
    sceIoClose(fd);
    pspSetK1(oldK1);
    return status;
}

// Subroutine ModuleMgrForUser_710F61B5 - Address 0x0000128C
s32 sceKernelLoadModuleMs(const char *path, s32 flags, SceKernelLMOption *pOpt)
{
    s32 oldK1;
    s32 fd;
    s32 status;
    SceModuleMgrParam modParams;
    
    (void)flags;

    oldK1 = pspShiftK1(); // 0x00001298

    // Cannot be called in an interruption
    if (sceKernelIsIntrContext()) { // 0x000012B0
        pspSetK1(oldK1);
        return SCE_ERROR_KERNEL_CANNOT_BE_CALLED_FROM_INTERRUPT;
    }
    
    if ((status = _checkCallConditionUser()) < 0) { //0x000012C4
        pspSetK1(oldK1);
        return status;
    }
    
    status = sceKernelGetUserLevel();
    /* Verify if user level relates to the MS API. */
    if (status != SCE_USER_LEVEL_MS) { //0x00001300
        pspSetK1(oldK1);
        return SCE_ERROR_KERNEL_ILLEGAL_PERMISSION_CALL;
    }
    
    //0x0000130C - 0x00001468
    if ((status = _checkPathConditions(path)) < 0  || (status = _checkLMOptionConditions(pOpt)) < 0) {
        pspSetK1(oldK1);
        return status;
    }

    fd = sceIoOpen(path, SCE_O_UNKNOWN0 | SCE_O_RDONLY, FILE_USER_ACCESS_PERMISSIONS); // 0x0000133C
    if (fd < 0) { // 0x00001348
        pspSetK1(oldK1);
        return fd;
    }

    status = sceIoIoctl(fd, 0x208002, NULL, 0, NULL, 0); // 0x00001368
    if (status < 0) { // 0x00001374
        sceIoClose(fd);
        pspSetK1(oldK1);
        return SCE_ERROR_KERNEL_PROHIBIT_LOADMODULE_DEVICE;
    }

    pspClearMemory32(&modParams, sizeof(modParams)); // 0x00001388

    modParams.apiType = 0x11;
    modParams.modeStart = CMD_LOAD_MODULE; // 0x000013B0
    modParams.modeFinish = CMD_RELOCATE_MODULE; // 0x000013A4
    modParams.unk64 = 0; // 0x000013BC
    modParams.fd = fd; // 0x000013C4
    modParams.unk124 = 0;

    status = sceIoIoctl(fd, 0x208081, NULL, 0, NULL, 0); // 0x000013C8
    if (status >= 0) // 0x000013D0
        modParams.unk100 = 0x10; // 0x000013D8
    
    status = _loadModuleByBufferID(&modParams, pOpt); // 0x000013E0
    
    sceIoClose(fd);
    pspSetK1(oldK1);
    return status;
}

/**
 * Load a module from a buffer, used for external bootable binaries which were sent then booted using the gamesharing API (allow them to load modules that were sent with the executable)
 * 
 * @param bufSize The size of the buffer containing the module
 * @param pBuffer The start address of the buffer containing the module
 * @param flags Unused, pass 0
 * @param pOpt A pointer to a SceKernelLMOption structure, which holds various options about the way to load the module. Pass NULL if you don't want to specify any option.

 * @return The ID of the module on success, < 0 on error.
 * @return SCE_ERROR_KERNEL_CANNOT_BE_CALLED_FROM_INTERRUPT if function was called in an interruption.
 * @return SCE_ERROR_KERNEL_ILLEGAL_PERMISSION_CALL if function was not called from a user context.
 * @return SCE_ERROR_KERNEL_ILLEGAL_ADDR if pBuffer/pOpt can't be accessed from the current context.
 * @return SCE_ERROR_KERNEL_ILLEGAL_SIZE if SdkVersion >= 2.80 and opt->size != sizeof(SceKernelLMOption).
 * @return One of the errors of sceIoOpen() if failed
 * @return SCE_ERROR_KERNEL_ERROR If the callback npDrmGetModuleKeyFunction in the g_ModuleManager structure is NULL
 * @return One of the errors of sceIoIoctl() if failed
 * @return SCE_ERROR_KERNEL_PROHIBIT_LOADMODULE_DEVICE if sceIoIoctl() failed
 *
 * @see sceNpDrmSetLicenseeKey()
 */
// Subroutine sceKernelLoadModuleBufferUsbWlan - Address 0x00001478 
SceUID sceKernelLoadModuleBufferUsbWlan(SceSize bufSize, void *pBuffer, u32 flags __attribute__((unused)), const SceKernelLMOption *pOpt)
{
    s32 oldK1;
    s32 fd;
    SceModuleMgrParam modParams;
    s32 status;

    oldK1 = pspShiftK1(); // 0x00001484

    // Cannot be called in an interruption
    if (sceKernelIsIntrContext()) { // 0x000012B0
        pspSetK1(oldK1);
        return SCE_ERROR_KERNEL_CANNOT_BE_CALLED_FROM_INTERRUPT;
    }  
    
    if ((status = _checkCallConditionUser()) < 0) { // 0x000014B8
        pspSetK1(oldK1);
        return status;
    }  

    if (sceKernelGetUserLevel() != SCE_USER_LEVEL_MS && sceKernelGetUserLevel() != SCE_USER_LEVEL_USBWLAN) { // 0x0000150C
        pspSetK1(oldK1);
        return SCE_ERROR_KERNEL_ILLEGAL_PERMISSION_CALL;
    }

    if (!pspK1DynBufOk(pBuffer, bufSize)) { // 0x00001528
        pspSetK1(oldK1);
        return SCE_ERROR_KERNEL_ILLEGAL_ADDR;
    }
    
    status = _checkLMOptionConditions(pOpt); //0x00001530 - 0x00001678
    if (status < 0) {
        pspSetK1(oldK1);
        return status;
    }

    // sub_00008568
    if (!_CheckOverride(0x30, pBuffer, &fd)) {
        pspClearMemory32(&modParams, sizeof(modParams)); // 0x0000160C

        modParams.apiType = 0x30;
        modParams.modeFinish = CMD_RELOCATE_MODULE;
        modParams.file_buffer = bufSize;

        // TODO: understand this, and fix the structure field if necessary
        modParams.fd = pBuffer;
        modParams.unk124 = 0;
        modParams.modeStart = CMD_RELOCATE_MODULE;
        modParams.unk64 = pBuffer;

        // sub_000075B4
        status = _loadModuleByBufferID(&modParams, pOpt); // 0x0000163C
        pspSetK1(oldK1);
        return status;
    }

    if (fd < 0) {
        // Congrats Sony, that sure is the best way to deal with errors!
        while (1) {}
    }

    pspClearMemory32(&modParams, sizeof(modParams)); // 0x00001584

    modParams.apiType = 0x30;
    modParams.modeFinish = CMD_RELOCATE_MODULE;
    modParams.modeStart = CMD_LOAD_MODULE;
    modParams.unk64 = 0;
    modParams.fd = fd;
    modParams.unk124 = 0;

    status = sceIoIoctl(fd, 0x208081, NULL, 0, NULL, 0); // 0x000015C4
    if (status >= 0) // 0x000015CC
        modParams.unk100 = 0x10;

    // sub_000075B4
    status = _loadModuleByBufferID(&modParams, pOpt); // 0x000015E4

    sceIoClose(fd); // 0x000015F0

    pspSetK1(oldK1);
    return status;
}

// Subroutine ModuleMgrForKernel_CE0A74A5 - Address 0x00001688
s32 sceKernelLoadModuleForLoadExecVSHDisc(const char *path, s32 flags, SceKernelLMOption *pOption)
{
    s32 oldK1;
    s32 fd;
    s32 status;
    SceModuleMgrParam modParams;
    
    (void)flags;

    oldK1 = pspShiftK1(); // 0x00001694

    // Cannot be called in an interruption
    if (sceKernelIsIntrContext()) { // 0x000016AC
        pspSetK1(oldK1);
        return SCE_ERROR_KERNEL_CANNOT_BE_CALLED_FROM_INTERRUPT;
    }
    
    if ((status = _checkCallConditionKernel()) < 0 ) { //0x000016C4
        pspSetK1(oldK1);
        return status;
    }
    
    //0x000016D0, 0x000017F8 - 0x00001850
    if ((status = _checkPathConditions(path)) < 0  || (status = _checkLMOptionConditions(pOption)) < 0) {
        pspSetK1(oldK1);
        return status;
    }

    fd = sceIoOpen(path, SCE_O_UNKNOWN0 | SCE_O_RDONLY, FILE_USER_ACCESS_PERMISSIONS); // 0x00001700
    if (fd < 0) { // 0x0000170C
        pspSetK1(oldK1);
        return fd;
    }

    status = sceIoIoctl(fd, 0x208011, NULL, 0, NULL, 0); // 0x0000172C
    if (status < 0) { // 0x00001738
        sceIoClose(fd);
        pspSetK1(oldK1);
        return SCE_ERROR_KERNEL_PROHIBIT_LOADMODULE_DEVICE;
    }

    pspClearMemory32(&modParams, sizeof(modParams)); // 0x0000174C

    modParams.apiType = SCE_INIT_APITYPE_DISC;
    modParams.modeStart = CMD_LOAD_MODULE; // 0x00001774
    modParams.modeFinish = CMD_RELOCATE_MODULE; // 0x00001768
    modParams.unk64 = 0; // 0x00001780
    modParams.fd = fd;
    modParams.unk124 = 0;

    status = sceIoIoctl(fd, 0x208081, NULL, 0, NULL, 0); // 0x0000178C
    if (status >= 0) // 0x00001794
        modParams.unk100 = 0x10; // 0x0000179C
    
    status = _loadModuleByBufferID(&modParams, pOption); // 0x000017A4
    
    sceIoClose(fd);
    pspSetK1(oldK1);
    return status;
}

// Subroutine ModuleMgrForKernel_CAE8E169 - Address 0x00001858
s32 sceKernelLoadModuleForLoadExecVSHDiscUpdater(const char *path, s32 flags, SceKernelLMOption *pOption)
{
    s32 oldK1;
    s32 fd;
    s32 status;
    SceModuleMgrParam modParams;
    
    (void)flags;

    oldK1 = pspShiftK1(); // 0x00001864

    // Cannot be called in an interruption
    if (sceKernelIsIntrContext()) { // 0x0000187C
        pspSetK1(oldK1);
        return SCE_ERROR_KERNEL_CANNOT_BE_CALLED_FROM_INTERRUPT;
    }
    
    if ((status = _checkCallConditionKernel()) < 0) { //0x00001894
        pspSetK1(oldK1);
        return status;
    }
    
    //0x000018A0 - 0x000019AC, 0x000019C8 - 0x00001A20
    if ((status = _checkPathConditions(path)) < 0  || (status = _checkLMOptionConditions(pOption)) < 0) {
        pspSetK1(oldK1);
        return status;
    }

    fd = sceIoOpen(path, SCE_O_UNKNOWN0 | SCE_O_RDONLY, FILE_USER_ACCESS_PERMISSIONS); // 0x000018D0
    if (fd < 0) { // 0x000018DC
        pspSetK1(oldK1);
        return fd;
    }

    status = sceIoIoctl(fd, 0x208011, NULL, 0, NULL, 0); // 0x000018FC
    if (status < 0) { // 0x00001908
        sceIoClose(fd);
        pspSetK1(oldK1);
        return SCE_ERROR_KERNEL_PROHIBIT_LOADMODULE_DEVICE;
    }

    pspClearMemory32(&modParams, sizeof(modParams)); // 0x0000191C

    modParams.apiType = SCE_INIT_APITYPE_DISC_UPDATER;
    modParams.modeStart = CMD_LOAD_MODULE; // 0x00001944
    modParams.modeFinish = CMD_RELOCATE_MODULE; // 0x00001938
    modParams.unk64 = 0; // 0x00001950
    modParams.fd = fd;
    modParams.unk124 = 0;

    status = sceIoIoctl(fd, 0x208081, NULL, 0, NULL, 0); // 0x0000195C
    if (status >= 0) // 0x00001964
        modParams.unk100 = 0x10; // 0x0000196C
    
    status = _loadModuleByBufferID(&modParams, pOption); // 0x00001980
    
    sceIoClose(fd);
    pspSetK1(oldK1);
    return status;
}

// Subroutine ModuleMgrForKernel_2C4F270D - Address 0x00001A28
s32 sceKernelLoadModuleForLoadExecVSHDiscDebug(const char *path, s32 flags, SceKernelLMOption *pOption)
{
    s32 oldK1;
    s32 fd;
    s32 status;
    SceModuleMgrParam modParams;
    
    (void)flags;

    oldK1 = pspShiftK1(); // 0x00001A34

    // Cannot be called in an interruption
    if (sceKernelIsIntrContext()) { // 0x00001A4C
        pspSetK1(oldK1);
        return SCE_ERROR_KERNEL_CANNOT_BE_CALLED_FROM_INTERRUPT;
    }
    
    if ((status = _checkCallConditionKernel()) < 0) { //0x00001A64
        pspSetK1(oldK1);
        return status;
    }
    
    //0x00001A70 - 0x00001B94, 0x00001B98 - 0x00001BF0
    if ((status = _checkPathConditions(path)) < 0  || (status = _checkLMOptionConditions(pOption)) < 0) {
        pspSetK1(oldK1);
        return status;
    }

    fd = sceIoOpen(path, SCE_O_UNKNOWN0 | SCE_O_RDONLY, FILE_USER_ACCESS_PERMISSIONS); // 0x00001AA0
    if (fd < 0) { // 0x00001AAC
        pspSetK1(oldK1);
        return fd;
    }

    status = sceIoIoctl(fd, 0x208011, NULL, 0, NULL, 0); // 0x00001ACC
    if (status < 0) { // 0x00001AD8
        sceIoClose(fd);
        pspSetK1(oldK1);
        return SCE_ERROR_KERNEL_PROHIBIT_LOADMODULE_DEVICE;
    }

    pspClearMemory32(&modParams, sizeof(modParams)); // 0x00001AEC

    modParams.apiType = SCE_INIT_APITYPE_DISC_DEBUG;
    modParams.modeStart = CMD_LOAD_MODULE; // 0x00001B14
    modParams.modeFinish = CMD_RELOCATE_MODULE; // 0x00001B08
    modParams.unk64 = 0; // 0x00001B20
    modParams.fd = fd; //0x00001B28
    modParams.unk124 = 0;

    status = sceIoIoctl(fd, 0x208081, NULL, 0, NULL, 0); // 0x00001B2C
    if (status >= 0) // 0x00001B34
        modParams.unk100 = 0x10; // 0x00001B3C
    
    status = _loadModuleByBufferID(&modParams, pOption); // 0x00001B44
    
    sceIoClose(fd);
    pspSetK1(oldK1);
    return status;
}

// Subroutine ModuleMgrForKernel_853A6C16 - Address 0x00001BF8
s32 sceKernelLoadModuleForLoadExecVSHDiscEmu(s32 apiType, const char *path, s32 flags, SceKernelLMOption *pOption)
{
    s32 oldK1;
    s32 fd;
    s32 status;
    SceModuleMgrParam modParams;
    
    (void)flags;

    oldK1 = pspShiftK1(); // 0x00001C04

    // Cannot be called in an interruption
    if (sceKernelIsIntrContext()) { // 0x00001C24
        pspSetK1(oldK1);
        return SCE_ERROR_KERNEL_CANNOT_BE_CALLED_FROM_INTERRUPT;
    }
    
    if ((status = _checkCallConditionKernel()) < 0) { //0x00001C3C
        pspSetK1(oldK1);
        return status;
    }
    
    //0x00001C48 - 0x00001D6C, 0x00001D70 - 0x00001DC8
    if ((status = _checkPathConditions(path)) < 0  || (status = _checkLMOptionConditions(pOption)) < 0) {
        pspSetK1(oldK1);
        return status;
    }

    fd = sceIoOpen(path, SCE_O_UNKNOWN0 | SCE_O_RDONLY, FILE_USER_ACCESS_PERMISSIONS); // 0x00001C78
    if (fd < 0) { // 0x00001C84
        pspSetK1(oldK1);
        return fd;
    }

    status = sceIoIoctl(fd, 0x208010, NULL, 0, NULL, 0); // 0x00001CA4
    if (status < 0) { // 0x00001CB0
        sceIoClose(fd);
        pspSetK1(oldK1);
        return SCE_ERROR_KERNEL_PROHIBIT_LOADMODULE_DEVICE;
    }

    pspClearMemory32(&modParams, sizeof(modParams)); // 0x00001CC4

    modParams.apiType = apiType; //0x00001CCC
    modParams.modeStart = CMD_LOAD_MODULE; // 0x00001CE4
    modParams.modeFinish = CMD_RELOCATE_MODULE; // 0x00001CD8
    modParams.unk64 = 0; // 0x00001CF0
    modParams.fd = fd; //0x00001CFC
    modParams.unk124 = 0;

    status = sceIoIoctl(fd, 0x208081, NULL, 0, NULL, 0); // 0x00001D00
    if (status >= 0) // 0x00001D08
        modParams.unk100 = 0x10;
    
    status = _loadModuleByBufferID(&modParams, pOption); // 0x00001D18
    
    sceIoClose(fd);
    pspSetK1(oldK1);
    return status;
}

// Subroutine ModuleMgrForKernel_C2A5E6CA - Address 0x00001DD0
s32 ModuleMgrForKernel_C2A5E6CA(s32 apiType, const char *path, s32 flags, SceKernelLMOption *pOption)
{
    s32 oldK1;
    s32 fd;
    s32 status;
    char installId[16];
    SceModuleMgrParam modParams;
    
    (void)flags;

    oldK1 = pspShiftK1(); // 0x00001DDC

    // Cannot be called in an interruption
    if (sceKernelIsIntrContext()) { // 0x00001E00
        pspSetK1(oldK1);
        return SCE_ERROR_KERNEL_CANNOT_BE_CALLED_FROM_INTERRUPT;
    }
    
    if ((status = _checkCallConditionKernel()) < 0) { //0x00001E18
        pspSetK1(oldK1);
        return status;
    }
    
    //0x00001E24 - 0x00001F74, 0x00001F78 - 0x00001FD4
    if ((status = _checkPathConditions(path)) < 0  || (status = _checkLMOptionConditions(pOption)) < 0) {
        pspSetK1(oldK1);
        return status;
    }

    fd = sceIoOpen(path, SCE_O_UNKNOWN0 | SCE_O_RDONLY, FILE_USER_ACCESS_PERMISSIONS); // 0x00001E54
    if (fd < 0) { // 0x00001E60
        pspSetK1(oldK1);
        return fd;
    }

    status = sceIoIoctl(fd, 0x208013, NULL, 0, NULL, 0); // 0x00001E7C
    if (status < 0) { // 0x00001E8C
        sceIoClose(fd);
        pspSetK1(oldK1);
        return SCE_ERROR_KERNEL_PROHIBIT_LOADMODULE_DEVICE;
    }
    
    memset(installId, 0, sizeof installId); //0x00001EA0
    pspClearMemory32(&modParams, sizeof(modParams)); // 0x00001EB4

    modParams.apiType = apiType; //0x00001EBC
    modParams.modeStart = CMD_LOAD_MODULE; // 0x00001ED4
    modParams.modeFinish = CMD_RELOCATE_MODULE; // 0x00001EC8
    modParams.unk64 = 0; // 0x00001EE0
    modParams.fd = fd; //0x00001EEC
    modParams.unk124 = 0; //0x00001EF4

    status = sceIoIoctl(fd, 0x208081, NULL, 0, NULL, 0); // 0x00001EF0
    if (status >= 0) // 0x00001EF8
        modParams.unk100 = 0x10;
    
    memcpy(modParams.secureInstallId, installId, sizeof modParams.secureInstallId); //0x00001F10
    
    status = _loadModuleByBufferID(&modParams, pOption); // 0x00001F1C
    
    sceIoClose(fd);
    pspSetK1(oldK1);
    return status;
}

// Subroutine ModuleMgrForKernel_FE61F16D - Address 0x00001FD8
s32 sceKernelLoadModuleForLoadExecVSHMs1(s32 apiType, const char *path, s32 flags, SceKernelLMOption *pOption)
{
    s32 oldK1;
    s32 fd;
    s32 status;
    SceModuleMgrParam modParams;
    
    (void)flags;

    oldK1 = pspShiftK1(); // 0x00001FE4

    // Cannot be called in an interruption
    if (sceKernelIsIntrContext()) { // 0x00002004
        pspSetK1(oldK1);
        return SCE_ERROR_KERNEL_CANNOT_BE_CALLED_FROM_INTERRUPT;
    }
    
    if ((status = _checkCallConditionKernel()) < 0) { //0x0000201C
        pspSetK1(oldK1);
        return status;
    }
    
    //0x00002028 - 0x0000214C, 0x00002150 - 0x000021AC
    if ((status = _checkPathConditions(path)) < 0  || (status = _checkLMOptionConditions(pOption)) < 0) {
        pspSetK1(oldK1);
        return status;
    }

    fd = sceIoOpen(path, SCE_O_UNKNOWN0 | SCE_O_RDONLY, FILE_USER_ACCESS_PERMISSIONS); // 0x00002058
    if (fd < 0) { // 0x00002064
        pspSetK1(oldK1);
        return fd;
    }

    status = sceIoIoctl(fd, 0x208013, NULL, 0, NULL, 0); // 0x00002084
    if (status < 0) { // 0x00002090
        sceIoClose(fd);
        pspSetK1(oldK1);
        return SCE_ERROR_KERNEL_PROHIBIT_LOADMODULE_DEVICE;
    }

    pspClearMemory32(&modParams, sizeof(modParams)); // 0x000020A4

    modParams.apiType = apiType; //0x000020AC
    modParams.modeStart = CMD_LOAD_MODULE; // 0x000020C4
    modParams.modeFinish = CMD_RELOCATE_MODULE; // 0x000020B8
    modParams.unk64 = 0; // 0x000020D0
    modParams.fd = fd; //0x000020DC
    modParams.unk124 = 0;

    status = sceIoIoctl(fd, 0x208081, NULL, 0, NULL, 0); // 0x000020E0
    if (status >= 0) // 0x000020E8
        modParams.unk100 = 0x10;
    
    status = _loadModuleByBufferID(&modParams, pOption); // 0x000020F8
    
    sceIoClose(fd);
    pspSetK1(oldK1);
    return status;
}

// Subroutine ModuleMgrForKernel_7BD53193 - Address 0x000021B0
s32 sceKernelLoadModuleForLoadExecVSHMs2(s32 apiType, const char *path, s32 flags, SceKernelLMOption *pOption)
{
    s32 oldK1;
    s32 fd;
    s32 status;
    SceModuleMgrParam modParams;
    
    (void)flags;

    oldK1 = pspShiftK1(); // 0x000021BC

    // Cannot be called in an interruption
    if (sceKernelIsIntrContext()) { // 0x000021DC
        pspSetK1(oldK1);
        return SCE_ERROR_KERNEL_CANNOT_BE_CALLED_FROM_INTERRUPT;
    }
    
    if ((status = _checkCallConditionKernel()) < 0) { //0x000021F4
        pspSetK1(oldK1);
        return status;
    }
    
    //0x00002200 - 0x00002324, 0x00002328 - 0x00002384
    if ((status = _checkPathConditions(path)) < 0  || (status = _checkLMOptionConditions(pOption)) < 0) {
        pspSetK1(oldK1);
        return status;
    }

    fd = sceIoOpen(path, SCE_O_UNKNOWN0 | SCE_O_RDONLY, FILE_USER_ACCESS_PERMISSIONS); // 0x00002230
    if (fd < SCE_ERROR_OK) { // 0x0000223C
        pspSetK1(oldK1);
        return fd;
    }

    status = sceIoIoctl(fd, 0x208013, NULL, 0, NULL, 0); // 0x0000225C
    if (status < SCE_ERROR_OK) { // 0x00002268
        sceIoClose(fd);
        pspSetK1(oldK1);
        return SCE_ERROR_KERNEL_PROHIBIT_LOADMODULE_DEVICE;
    }

    pspClearMemory32(&modParams, sizeof(modParams)); // 0x0000227C

    modParams.apiType = apiType; //0x00002284
    modParams.modeStart = CMD_LOAD_MODULE; // 0x0000229C
    modParams.modeFinish = CMD_RELOCATE_MODULE; // 0x00002290
    modParams.unk64 = 0; // 0x000022A8
    modParams.fd = fd; //0x000022B4
    modParams.unk124 = 0;

    status = sceIoIoctl(fd, 0x208081, NULL, 0, NULL, 0); // 0x000022D0
    if (status >= 0) // 0x000022C0
        modParams.unk100 = 0x10;
    
    status = _loadModuleByBufferID(&modParams, pOption); // 0x000022D0
    
    sceIoClose(fd);
    pspSetK1(oldK1);
    return status;
}

// Subroutine ModuleMgrForKernel_D60AB6CC - Address 0x00002388
s32 sceKernelLoadModuleForLoadExecVSHMs3(s32 apiType, const char *path, s32 flags, SceKernelLMOption *pOption)
{
    s32 oldK1;
    s32 fd;
    s32 status;
    char installId[16];
    SceModuleMgrParam modParams;
    
    (void)flags;

    oldK1 = pspShiftK1(); // 0x00002394

    // Cannot be called in an interruption
    if (sceKernelIsIntrContext()) { // 0x000023B8
        pspSetK1(oldK1);
        return SCE_ERROR_KERNEL_CANNOT_BE_CALLED_FROM_INTERRUPT;
    }
    
    if ((status = _checkCallConditionKernel()) < 0) { //0x000023D0
        pspSetK1(oldK1);
        return status;
    }
    
    //0x000023DC - 0x0000252C, 0x00002530 - 0x0000258C
    if ((status = _checkPathConditions(path)) < 0  || (status = _checkLMOptionConditions(pOption)) < 0) {
        pspSetK1(oldK1);
        return status;
    }

    fd = sceIoOpen(path, SCE_O_UNKNOWN0 | SCE_O_RDONLY, FILE_USER_ACCESS_PERMISSIONS); // 0x0000240C
    if (fd < 0) { // 0x00002418
        pspSetK1(oldK1);
        return fd;
    }

    status = sceIoIoctl(fd, 0x208013, NULL, 0, NULL, 0); // 0x00002438
    if (status < 0) { // 0x00002444
        sceIoClose(fd);
        pspSetK1(oldK1);
        return SCE_ERROR_KERNEL_PROHIBIT_LOADMODULE_DEVICE;
    }

    memset(installId, 0, sizeof installId); //0x00002458
    pspClearMemory32(&modParams, sizeof(modParams)); // 0x0000246C

    modParams.apiType = apiType; //0x00002474
    modParams.modeStart = CMD_LOAD_MODULE; // 0x0000248C
    modParams.modeFinish = CMD_RELOCATE_MODULE; // 0x00002480
    modParams.unk64 = 0; // 0x00002498
    modParams.fd = fd; //0x000024A4
    modParams.unk124 = 0;

    status = sceIoIoctl(fd, 0x208081, NULL, 0, NULL, 0); // 0x000024A8
    if (status >= 0) // 0x000024B0
        modParams.unk100 = 0x10;
    
    memcpy(modParams.secureInstallId, installId, sizeof modParams.secureInstallId); //0x000024C8
    
    status = _loadModuleByBufferID(&modParams, pOption); // 0x000024D4
    
    sceIoClose(fd);
    pspSetK1(oldK1);
    return status;
}

// Subroutine ModuleMgrForKernel_76F0E956 - Address 0x00002590
s32 sceKernelLoadModuleForLoadExecVSHMs4(s32 apiType, const char *path, s32 flags, SceKernelLMOption *pOption)
{
    s32 oldK1;
    s32 fd;
    s32 status;
    SceModuleMgrParam modParams;
    
    (void)flags;

    oldK1 = pspShiftK1(); // 0x0000259C

    // Cannot be called in an interruption
    if (sceKernelIsIntrContext()) { // 0x000025BC
        pspSetK1(oldK1);
        return SCE_ERROR_KERNEL_CANNOT_BE_CALLED_FROM_INTERRUPT;
    }
    
    if ((status = _checkCallConditionKernel()) < 0) { //0x000025D4
        pspSetK1(oldK1);
        return status;
    }
    
    //0x000025E0 - 0x00002704, 0x00002708 - 0x00002764
    if ((status = _checkPathConditions(path)) < 0  || (status = _checkLMOptionConditions(pOption)) < 0) {
        pspSetK1(oldK1);
        return status;
    }

    fd = sceIoOpen(path, SCE_O_UNKNOWN0 | SCE_O_RDONLY, FILE_USER_ACCESS_PERMISSIONS); // 0x00002610
    if (fd < 0) {
        pspSetK1(oldK1);
        return fd;
    }

    status = sceIoIoctl(fd, 0x208013, NULL, 0, NULL, 0); // 0x0000263C
    if (status < 0) {
        sceIoClose(fd);
        pspSetK1(oldK1);
        return SCE_ERROR_KERNEL_PROHIBIT_LOADMODULE_DEVICE;
    }

    pspClearMemory32(&modParams, sizeof(modParams)); // 0x0000265C

    modParams.apiType = apiType; //0x00002664
    modParams.modeStart = CMD_LOAD_MODULE; // 0x0000267C
    modParams.modeFinish = CMD_RELOCATE_MODULE; // 0x00002670
    modParams.unk64 = 0; // 0x00002688
    modParams.fd = fd; //0x00002694
    modParams.unk124 = 0;

    status = sceIoIoctl(fd, 0x208081, NULL, 0, NULL, 0); // 0x00002698
    if (status >= 0)
        modParams.unk100 = 0x10;
    
    status = _loadModuleByBufferID(&modParams, pOption); // 0x000026B0
    
    sceIoClose(fd);
    pspSetK1(oldK1);
    return status;
}

// Subroutine ModuleMgrForKernel_4E8A2C9D - Address 0x00002768
s32 sceKernelLoadModuleForLoadExecVSHMs5(s32 apiType, const char *path, s32 flags, SceKernelLMOption *pOption)
{
    s32 oldK1;
    s32 fd;
    s32 status;
    char installId[16];
    SceModuleMgrParam modParams;
    
    (void)flags;

    oldK1 = pspShiftK1(); // 0x00002774

    // Cannot be called in an interruption
    if (sceKernelIsIntrContext()) { // 0x0000279C
        pspSetK1(oldK1);
        return SCE_ERROR_KERNEL_CANNOT_BE_CALLED_FROM_INTERRUPT;
    }
    
    if ((status = _checkCallConditionKernel()) < 0) { //0x000027B4
        pspSetK1(oldK1);
        return status;
    }
    
    //0x000027C0 - 0x00002914, 0x00002918 - 0x00002974
    if ((status = _checkPathConditions(path)) < 0  || (status = _checkLMOptionConditions(pOption)) < 0) {
        pspSetK1(oldK1);
        return status;
    }

    fd = sceIoOpen(path, SCE_O_UNKNOWN0 | SCE_O_RDONLY, FILE_USER_ACCESS_PERMISSIONS); // 0x000027F0
    if (fd < 0) {
        pspSetK1(oldK1);
        return fd;
    }

    status = sceIoIoctl(fd, 0x208013, NULL, 0, NULL, 0); // 0x00002820
    if (status < 0) { // 0x00002828
        sceIoClose(fd);
        pspSetK1(oldK1);
        return SCE_ERROR_KERNEL_PROHIBIT_LOADMODULE_DEVICE;
    }
    
    // TODO: Update sceKernelGetId prototype to match given arguments
    status = sceKernelGetId(path, installId); //0x00002838
    if (status < 0) {
        sceIoClose(fd);
        pspSetK1(oldK1);
        return status;
    }

    pspClearMemory32(&modParams, sizeof(modParams)); // 0x00002854

    modParams.apiType = apiType; //0x00002860
    modParams.modeStart = CMD_LOAD_MODULE; // 0x00002878
    modParams.modeFinish = CMD_RELOCATE_MODULE; // 0x0000286C
    modParams.unk64 = 0; // 0x00002884
    modParams.fd = fd; //0x0000288C
    modParams.unk124 = 0;

    status = sceIoIoctl(fd, 0x208081, NULL, 0, NULL, 0); // 0x00002890
    if (status >= 0) // 0x00002898
        modParams.unk100 = 0x10;
    
    memcpy(modParams.secureInstallId, installId, sizeof modParams.secureInstallId); //0x000028AC
    
    status = _loadModuleByBufferID(&modParams, pOption); // 0x000028B8
    
    sceIoClose(fd);
    pspSetK1(oldK1);
    return status;
}

// Subroutine ModuleMgrForKernel_E8422026 - Address 0x00002978
s32 sceKernelLoadModuleForLoadExecVSHMs6(s32 apiType, const char *path, s32 flags, SceKernelLMOption *pOption)
{
    s32 oldK1;
    s32 fd;
    s32 status;
    char installId[16];
    SceModuleMgrParam modParams;
    
    (void)flags;

    oldK1 = pspShiftK1(); // 0x00002984

    // Cannot be called in an interruption
    if (sceKernelIsIntrContext()) { // 0x000029AC
        pspSetK1(oldK1);
        return SCE_ERROR_KERNEL_CANNOT_BE_CALLED_FROM_INTERRUPT;
    }
    
    if ((status = _checkCallConditionKernel()) < 0 ) { //0x000029C4
        pspSetK1(oldK1);
        return status;
    }
    
    //0x000029D0 - 0x00002B24, 0x00002B28 - 0x00002B84
    if ((status = _checkPathConditions(path)) < 0  || (status = _checkLMOptionConditions(pOption)) < 0) {
        pspSetK1(oldK1);
        return status;
    }

    fd = sceIoOpen(path, SCE_O_UNKNOWN0 | SCE_O_RDONLY, FILE_USER_ACCESS_PERMISSIONS); // 0x00002A00
    if (fd < 0) {
        pspSetK1(oldK1);
        return fd;
    }

    status = sceIoIoctl(fd, 0x208013, NULL, 0, NULL, 0); // 0x00002A30
    if (status < 0) { // 0x00002A38
        sceIoClose(fd);
        pspSetK1(oldK1);
        return SCE_ERROR_KERNEL_PROHIBIT_LOADMODULE_DEVICE;
    }
    
    // TODO: Update sceKernelGetId prototype to match given arguments
    status = sceKernelGetId(path, installId); //0x00002A48
    if (status < 0) {
        sceIoClose(fd);
        pspSetK1(oldK1);
        return status;
    }

    pspClearMemory32(&modParams, sizeof(modParams)); // 0x00002A64

    modParams.apiType = apiType; //0x00002A70
    modParams.modeStart = CMD_LOAD_MODULE; // 0x00002A88
    modParams.modeFinish = CMD_RELOCATE_MODULE; // 0x00002A7C
    modParams.unk64 = 0; // 0x00002A94
    modParams.fd = fd; //0x00002AA4
    modParams.unk124 = 0;

    status = sceIoIoctl(fd, 0x208081, NULL, 0, NULL, 0); // 0x00002AA0
    if (status >= 0) // 0x00002AA8
        modParams.unk100 = 0x10;
    
    memcpy(modParams.secureInstallId, installId, sizeof modParams.secureInstallId); //0x00002ABC
    
    status = _loadModuleByBufferID(&modParams, pOption); // 0x00002AC8
    
    sceIoClose(fd);
    pspSetK1(oldK1);
    return status;
}

// Subroutine ModuleMgrForKernel_8DD336D4 - Address 0x00002B88
s32 ModuleMgrForKernel_8DD336D4(s32 apiType, const char *path, s32 flags, SceKernelLMOption *pOption)
{
    s32 oldK1;
    s32 fd;
    s32 status;
    char installId[16];
    SceModuleMgrParam modParams;
    
    (void)flags;

    oldK1 = pspShiftK1(); // 0x00002B94

    // Cannot be called in an interruption
    if (sceKernelIsIntrContext()) { // 0x00002BB8
        pspSetK1(oldK1);
        return SCE_ERROR_KERNEL_CANNOT_BE_CALLED_FROM_INTERRUPT;
    }
    
    if ((status = _checkCallConditionKernel()) < 0 ) { //0x00002BD0
        pspSetK1(oldK1);
        return status;
    }
    
    //0x00002BDC - 0x00002D2C, 0x00002D30 - 0x00002D8C
    if ((status = _checkPathConditions(path)) < 0  || (status = _checkLMOptionConditions(pOption)) < 0) {
        pspSetK1(oldK1);
        return status;
    }

    fd = sceIoOpen(path, SCE_O_UNKNOWN0 | SCE_O_RDONLY, FILE_USER_ACCESS_PERMISSIONS); // 0x00002C0C
    if (fd < 0) {
        pspSetK1(oldK1);
        return fd;
    }

    status = sceIoIoctl(fd, 0x208013, NULL, 0, NULL, 0); // 0x00002C38
    if (status < 0) { // 0x00002C44
        sceIoClose(fd);
        pspSetK1(oldK1);
        return SCE_ERROR_KERNEL_PROHIBIT_LOADMODULE_DEVICE;
    }
    
    memset(installId, 0, sizeof installId); //0x00002C58
    pspClearMemory32(&modParams, sizeof(modParams)); // 0x00002C6C

    modParams.apiType = apiType; //0x00002C74
    modParams.modeStart = CMD_LOAD_MODULE; // 0x00002C8C
    modParams.modeFinish = CMD_RELOCATE_MODULE; // 0x00002C80
    modParams.unk64 = 0; // 0x00002C98
    modParams.fd = fd; //0x00002CA4
    modParams.unk124 = 0;

    status = sceIoIoctl(fd, 0x208081, NULL, 0, NULL, 0); // 0x00002CA8
    if (status >= 0) // 0x00002CB0
        modParams.unk100 = 0x10;
    
    memcpy(modParams.secureInstallId, installId, sizeof modParams.secureInstallId); //0x00002CC8
    
    status = _loadModuleByBufferID(&modParams, pOption); // 0x00002CD4
    
    sceIoClose(fd);
    pspSetK1(oldK1);
    return status;
}

// Subroutine ModuleMgrForKernel_30727524 - Address 0x00002D90
s32 sceKernelLoadModuleForLoadExecNpDrm(s32 apiType, const char *path, SceOff fileOffset, u8 keyData[16], s32 flags, 
        SceKernelLMOption *pOption)
{
    s32 oldK1;
    s32 fd;
    s32 status;
    SceModuleMgrParam modParams;
    
    (void)flags;

    oldK1 = pspShiftK1(); // 0x00002D9C

    // Cannot be called in an interruption
    if (sceKernelIsIntrContext()) { // 0x00002DD4
        pspSetK1(oldK1);
        return SCE_ERROR_KERNEL_CANNOT_BE_CALLED_FROM_INTERRUPT;
    }
    
    if ((status = _checkCallConditionKernel()) < 0 ) { //0x00002DEC
        pspSetK1(oldK1);
        return status;
    }
    
    //0x00002DF8 - 0x00002F64, 0x00002F68 - 0x00002FC4
    if ((status = _checkPathConditions(path)) < 0  || (status = _checkLMOptionConditions(pOption)) < 0) {
        pspSetK1(oldK1);
        return status;
    }
    
    if (!pspK1StaBufOk(keyData, sizeof keyData)) { //0x00002E2C
        pspSetK1(oldK1);
        return SCE_ERROR_KERNEL_ILLEGAL_ADDR;
    }

    fd = sceIoOpen(path, SCE_O_UNKNOWN0 | SCE_O_RDONLY, FILE_USER_ACCESS_PERMISSIONS); // 0x00002E40
    if (fd < 0) {
        pspSetK1(oldK1);
        return fd;
    }
    
    sceIoLseek(fd, fileOffset, SCE_SEEK_SET); //0x00002E64

    status = sceIoIoctl(fd, 0x208013, NULL, 0, NULL, 0); // 0x00002E84
    if (status < 0) { // 0x00002E8C
        sceIoClose(fd);
        pspSetK1(oldK1);
        return SCE_ERROR_KERNEL_PROHIBIT_LOADMODULE_DEVICE;
    }
    
    pspClearMemory32(&modParams, sizeof(modParams)); // 0x00002EA0

    modParams.apiType = apiType; //0x00002EA8
    modParams.modeStart = CMD_LOAD_MODULE; // 0x00002EC0
    modParams.modeFinish = CMD_RELOCATE_MODULE; // 0x00002EB4
    modParams.unk64 = 0; // 0x00002ECC
    modParams.fd = fd; //0x00002ED8
    modParams.unk124 = 0;

    status = sceIoIoctl(fd, 0x208081, NULL, 0, NULL, 0); // 0x00002EDC
    if (status >= 0) // 0x00002EE4
        modParams.unk100 = 0x10;
    
    memcpy(modParams.secureInstallId, keyData, sizeof modParams.secureInstallId); //0x00002EF8
    
    status = _loadModuleByBufferID(&modParams, pOption); // 0x00002F04
    
    sceIoClose(fd);
    pspSetK1(oldK1);
    return status;
}

// Subroutine ModuleMgrForKernel_D5DDAB1F - Address 0x00002FC8
s32 sceKernelLoadModuleVSH(const char *path, s32 flags, SceKernelLMOption *pOption)
{
    s32 oldK1;
    s32 fd;
    s32 status;
    SceModuleMgrParam modParams;
    
    (void)flags;

    oldK1 = pspShiftK1(); // 0x00002FD4

    // Cannot be called in an interruption
    if (sceKernelIsIntrContext()) { // 0x00002FEC
        pspSetK1(oldK1);
        return SCE_ERROR_KERNEL_CANNOT_BE_CALLED_FROM_INTERRUPT;
    }
    
    if ((status = _checkCallConditionUser()) < 0 ) { //0x00003000
        pspSetK1(oldK1);
        return status;
    }
    
    status = sceKernelGetUserLevel(); // 0x00003030
    if (status != SCE_USER_LEVEL_VSH) { //0x00001300
        pspSetK1(oldK1);
        return SCE_ERROR_KERNEL_ILLEGAL_PERMISSION_CALL;
    }
    
    //0x00003048, 0x00003180; 0x00003184 - 0x000031D8
    if ((status = _checkPathConditions(path)) < 0  || (status = _checkLMOptionConditions(pOption)) < 0) {
        pspSetK1(oldK1);
        return status;
    }

    fd = sceIoOpen(path, SCE_O_UNKNOWN0 | SCE_O_RDONLY, FILE_USER_ACCESS_PERMISSIONS); // 0x00003078
    if (fd < 0) { // 0x00003084
        pspSetK1(oldK1);
        return fd;
    }

    status = sceIoIoctl(fd, 0x208003, NULL, 0, NULL, 0); // 0x000030A4
    if (status < 0) { // 0x000030B0
        sceIoClose(fd);
        pspSetK1(oldK1);
        return SCE_ERROR_KERNEL_PROHIBIT_LOADMODULE_DEVICE;
    }

    pspClearMemory32(&modParams, sizeof(modParams)); // 0x000030C4

    modParams.apiType = 0x20; //0x000030D4
    modParams.modeStart = CMD_LOAD_MODULE; // 0x000030EC
    modParams.modeFinish = CMD_RELOCATE_MODULE; // 0x000030E0
    modParams.unk64 = 0; // 0x000030F8
    modParams.fd = fd; // 0x00003100
    modParams.unk124 = 0;

    status = sceIoIoctl(fd, 0x208081, NULL, 0, NULL, 0); // 0x00003104
    if (status >= 0) // 0x0000310C
        modParams.unk100 = 0x10;
    
    status = sceIoIoctl(fd, 0x208082, NULL, 0, NULL, 0); // 0x00003130
    if (status < 0) // 0x00003138
        modParams.unk124 = 1; //0x00003164
    
    status = _loadModuleByBufferID(&modParams, pOption); // 0x00003144
    
    sceIoClose(fd);
    pspSetK1(oldK1);
    return status;
}

// Subroutine ModuleMgrForKernel_CBA02988 - Address 0x000031E4
s32 sceKernelLoadModuleVSHByID(s32 inputId, s32 flags, SceKernelLMOption *pOption)
{
    s32 oldK1;
    s32 status;
    SceModuleMgrParam modParams;
    
    (void)flags;

    oldK1 = pspShiftK1(); // 0x000031F0

    // Cannot be called in an interruption
    if (sceKernelIsIntrContext()) { // 0x00003214
        pspSetK1(oldK1);
        return SCE_ERROR_KERNEL_CANNOT_BE_CALLED_FROM_INTERRUPT;
    }
    
    if ((status = _checkCallConditionUser()) < 0) { //0x0000321C
        pspSetK1(oldK1);
        return status;
    }
    
    status = sceKernelGetUserLevel(); // 0x0000324C
    if (status != SCE_USER_LEVEL_VSH) { //0x00003258
        pspSetK1(oldK1);
        return SCE_ERROR_KERNEL_ILLEGAL_PERMISSION_CALL;
    }
    
    //0x00003260 - 0x000032AC
    if (status = _checkLMOptionConditions(pOption) < 0) {
        pspSetK1(oldK1);
        return status;
    }
    
    status = sceIoValidateFd(inputId, 4); //0x000032B4
    if (status < SCE_ERROR_OK) { //0x000032BC
        pspSetK1(oldK1);
        return status;
    }

    status = sceIoIoctl(inputId, 0x208003, NULL, 0, NULL, 0); // 0x000032DC
    if (status < 0) { // 0x000032E8
        pspSetK1(oldK1);
        return SCE_ERROR_KERNEL_PROHIBIT_LOADMODULE_DEVICE;
    }

    pspClearMemory32(&modParams, sizeof(modParams)); // 0x000032FC

    modParams.apiType = 0x20; //0x0000330C
    modParams.modeStart = CMD_LOAD_MODULE; // 0x00003324
    modParams.modeFinish = CMD_RELOCATE_MODULE; // 0x00003318
    modParams.unk64 = 0; // 0x00003330
    modParams.fd = inputId; // 0x00003338
    modParams.unk124 = 0;

    status = sceIoIoctl(inputId, 0x208081, NULL, 0, NULL, 0); // 0x0000333C
    if (status >= 0) // 0x00003344
        modParams.unk100 = 0x10;
    
    status = sceIoIoctl(inputId, 0x208082, NULL, 0, NULL, 0); // 0x00003368
    if (status < 0) // 0x00003390
        modParams.unk124 = 1; //0x00003164
    
    status = _loadModuleByBufferID(&modParams, pOption); // 0x0000337C
    
    pspSetK1(oldK1);
    return status;
}

// Subroutine ModuleMgrForKernel_939E4270 - Address 0x00003394
s32 sceKernelLoadModuleForKernel(const char *path, u32 flags, const SceKernelLMOption *pOption)
{
    s32 oldK1;
    s32 fd;
    s32 status;
    SceModuleMgrParam modParams;
    
    (void)flags;

    oldK1 = pspShiftK1(); // 0x000033A0

    // Cannot be called in an interruption
    if (sceKernelIsIntrContext()) { // 0x000033C4
        pspSetK1(oldK1);
        return SCE_ERROR_KERNEL_CANNOT_BE_CALLED_FROM_INTERRUPT;
    }
    
    //0x000033D0 - 0x000033F0, 0x00003514 - 0x0000358C
    if ((status = _checkCallConditionKernel()) < 0 || (status = _checkPathConditions(path)) < 0 
            || (status = _checkLMOptionConditions(pOption)) < 0) {
        pspSetK1(oldK1);
        return status;
    }

    fd = sceIoOpen(path, SCE_O_UNKNOWN0 | SCE_O_RDONLY, FILE_USER_ACCESS_PERMISSIONS); // 0x0000340C
    if (fd < 0) { // 0x00000740
        pspSetK1(oldK1);
        return fd;
    }

    status = sceIoIoctl(fd, 0x208006, NULL, 0, NULL, 0); // 0x00003438
    if (status < 0) { // 0x00003444
        sceIoClose(fd);
        pspSetK1(oldK1);
        return SCE_ERROR_KERNEL_PROHIBIT_LOADMODULE_DEVICE;
    }

    pspClearMemory32(&modParams, sizeof(modParams)); // 0x00003458

    modParams.apiType = 0x0; // 0x00003470
    modParams.modeStart = CMD_LOAD_MODULE; // 0x0000347C
    modParams.modeFinish = CMD_RELOCATE_MODULE; // 0x00003464
    modParams.unk64 = 0; // 0x00003488
    modParams.fd = fd; // 0x00003490
    modParams.unk124 = 0;

    status = sceIoIoctl(fd, 0x208081, NULL, 0, NULL, 0); // 0x00003494
    if (status >= 0) // 0x0000349C
        modParams.unk100 = 0x10; // 0x000034A4
    
    status = sceIoIoctl(fd, 0x208082, NULL, 0, NULL, 0); // 0x000034C0
    if (status < 0) // 0x000034C8
        modParams.unk124 = 1; //0x00003510
    
    status = _loadModuleByBufferID(&modParams, pOption); // 0x000034D4
    
    sceIoClose(fd); // 0x000034E0
    pspSetK1(oldK1);
    return status;
}

// Subroutine ModuleMgrForKernel_EEC2A745 - Address 0x00003590
s32 sceKernelLoadModuleByIDForKernel(SceUID inputId, u32 flags,
        const SceKernelLMOption *pOption)
{
    s32 oldK1;
    s32 status;
    SceModuleMgrParam modParams;
    
    (void)flags;
    
    oldK1 = pspShiftK1(); //0x0000359C
    
    if (sceKernelIsIntrContext()) { //0x000035B4
        pspSetK1(oldK1);
        return SCE_ERROR_KERNEL_CANNOT_BE_CALLED_FROM_INTERRUPT;
    }
    
    //0x000035CC, 0x000035D4 - 0x000035E8, 0x00003614 - 0x00003644
    if ((status = _checkCallConditionKernel()) < 0 || (status = _checkLMOptionConditions(pOption)) < 0) {
        pspSetK1(oldK1);
        return status;
    }
    
    status = sceIoValidateFd(inputId, 4); //0x0000364C
    if (status < SCE_ERROR_OK) { //0x00003654
        pspSetK1(oldK1);
        return status;
    }
    
    status = sceIoIoctl(inputId, 0x208006, NULL, 0, NULL, 0); // 0x00003674
    if (status < 0) { // 0x00003680
        pspSetK1(oldK1);
        return SCE_ERROR_KERNEL_PROHIBIT_LOADMODULE_DEVICE;
    }
    
    pspClearMemory32(&modParams, sizeof(modParams)); //0x00003694
        
    modParams.apiType = 0x0; // 0x000036AC
    modParams.modeFinish = CMD_RELOCATE_MODULE; // 0x000036A0
    modParams.modeStart = CMD_LOAD_MODULE; // 0x000036B8
    modParams.unk64 = 0; // 0x000036C4
    modParams.fd = inputId; // 0x000036CC
    modParams.unk124 = 0;
        
    status = sceIoIoctl(inputId, 0x208081, NULL, 0, NULL, 0); //0x000036D0
    if (status >= 0) //0x000036D8
        modParams.unk100 = 0x10; // 0x000036E0
    
    status = sceIoIoctl(inputId, 0x208082, NULL, 0, NULL, 0); // 0x000036FC
    if (status < 0) // 0x00003704
        modParams.unk124 = 1; //0x00003724
        
    status = _loadModuleByBufferID(&modParams, pOption); //0x00003710
        
    pspSetK1(oldK1);
    return status;
}

// Subroutine ModuleMgrForKernel_D4EE2D26 - Address 0x00003728
s32 sceKernelLoadModuleToBlock(const char *path, u32 block, u32 *arg2, u32 flags, const SceKernelLMOption *pOption)
{
    s32 oldK1;
    s32 status;
    SceUID fd;
    u32 buf;
    SceModuleMgrParam modParams;
    
    (void)flags;
    
    oldK1 = pspShiftK1(); //0x0000375C
    
    if (sceKernelIsIntrContext()) { //0x00003774
        pspSetK1(oldK1);
        return SCE_ERROR_KERNEL_CANNOT_BE_CALLED_FROM_INTERRUPT;
    }
    
    //0x0000378C, 0x00003798 - 0x000037AC
    if ((status = _checkCallConditionKernel()) < 0 || (status = _checkPathConditions(path)) < 0
            || _checkLMOptionConditions(pOption)) {
        pspSetK1(oldK1);
        return status;
    }
    
    // 0x000037BC
    if (arg2 == NULL) {
        pspSetK1(oldK1);
        return SCE_ERROR_KERNEL_INVALID_ARGUMENT;
    }
    
    // 0x000037C4
    if (pOption != NULL && pOption->position >= SCE_KERNEL_SMEM_Addr) {
        // Missing pspSetK1(oldk1) here
        return SCE_ERROR_KERNEL_INVALID_ARGUMENT;
    }
    
    // 0x000037E8
    if (!pspK1StaBufOk(arg2)) {
        pspSetK1(oldK1);
        return SCE_ERROR_KERNEL_ILLEGAL_ADDR;
    }
    
    // 0x000037F8
    fd = sceIoOpen(path, SCE_O_UNKNOWN0 | SCE_O_RDONLY, FILE_USER_ACCESS_PERMISSIONS);
    if (fd < 0) { // 0x00003804
        pspSetK1(oldK1);
        return fd;
    }
    
    status = sceIoIoctl(fd, 0x208007, NULL, 0, NULL, 0); // 0x00003824
    if (status < 0) { // 0x00003830
        pspSetK1(oldK1);
        return SCE_ERROR_KERNEL_PROHIBIT_LOADMODULE_DEVICE;
    }
    
    pspClearMemory32(&modParams, sizeof(modParams)); //0x00003844
        
    modParams.apiType = 0x3; // 0x00003854
    modParams.modeFinish = CMD_RELOCATE_MODULE; // 0x00003860
    modParams.modeStart = CMD_LOAD_MODULE; // 0x0000386C
    modParams.unk64 = 0; // 0x00003878
    modParams.fd = fd; // 0x00003880
    modParams.unk124 = 0;
        
    status = sceIoIoctl(fd, 0x208081, NULL, 0, NULL, 0); //0x00003884
    if (status >= 0) //0x0000388C
        modParams.unk100 = 0x10; // 0x000036E0
    
    modParams.unk104 = block; // 0x0000389C
    modParams.status = &buf; //0x000038C0
    status = sceIoIoctl(fd, 0x208082, NULL, 0, NULL, 0); // 0x000038BC
    if (status < 0) // 0x000038C4
        modParams.unk124 = 1; //0x00003924
        
    status = _loadModuleByBufferID(&modParams, pOption); //0x000038D0
    if (status >= 0)
        *arg2 = buf;
        
    sceIoClose(fd);
    pspSetK1(oldK1);
    return status;
}

// Subroutine ModuleMgrForKernel_F7C7FEBC - Address 0x000039C0
s32 sceKernelLoadModuleBootInitConfig(const char *path, u32 flags, SceKernelLMOption *pOption)
{
    s32 oldK1;
    s32 status;
    SceUID fd;
    SceModuleMgrParam modParams;
    
    (void)flags;
    
    oldK1 = pspShiftK1(); //0x000039CC
    
    if (!sceKernelIsDevelopmentToolMode()) { // 0x000039E4
        pspSetK1(oldK1);
        return SCE_ERROR_KERNEL_ILLEGAL_PERMISSION_CALL;
    }
        
    if (sceKernelIsIntrContext()) { //0x00003A1C
        pspSetK1(oldK1);
        return SCE_ERROR_KERNEL_CANNOT_BE_CALLED_FROM_INTERRUPT;
    }
    
    //0x00003C20, 0x00003C2C - 0x00003C40, 0x00003D1C - 0x00003D94
    if ((status = _checkCallConditionKernel()) < 0 || (status = _checkPathConditions(path)) < 0 
            || (status = _checkLMOptionConditions(pOption)) < 0) {
        pspSetK1(oldK1);
        return status;
    }
    
    fd = sceIoOpen(path, SCE_O_UNKNOWN0 | SCE_O_RDONLY, FILE_USER_ACCESS_PERMISSIONS); // 0x00003C5C
    if (fd < 0) { // 0x00003C68
        pspSetK1(oldK1);
        return fd;
    }
    
    status = sceIoIoctl(fd, 0x20800C, NULL, 0, NULL, 0); // 0x00003C88
    if (status < 0) { // 0x00003C94
        pspSetK1(oldK1);
        return SCE_ERROR_KERNEL_PROHIBIT_LOADMODULE_DEVICE;
    }
    
    pspClearMemory32(&modParams, sizeof(modParams)); //0x00003CA8
        
    modParams.apiType = 0x70; // 0x00003CB8
    modParams.modeFinish = CMD_RELOCATE_MODULE; // 0x00003CC4
    modParams.modeStart = CMD_LOAD_MODULE; // 0x00003CD0
    modParams.unk64 = 0; // 0x00003CDC
    modParams.fd = fd; // 0x00003CE4
    modParams.unk124 = 0;
        
    status = sceIoIoctl(fd, 0x208081, NULL, 0, NULL, 0); //0x00003CE8
    if (status >= 0) //0x00003CF0
        modParams.unk100 = 0x10;
        
    status = _loadModuleByBufferID(&modParams, pOption); //0x00003D00
        
    sceIoClose(fd);
    pspSetK1(oldK1);
    return status;
}

// Subroutine ModuleMgrForKernel_4493E013 - Address 0x00003BAC
s32 sceKernelLoadModuleDeci(const char *path, u32 flags, SceKernelLMOption *pOption)
{
    s32 oldK1;
    s32 status;
    SceUID fd;
    SceModuleMgrParam modParams;
    
    (void)flags;
    
    oldK1 = pspShiftK1(); //0x00003BB8
    
    if (!sceKernelIsDevelopmentToolMode()) { // 0x00003BD0
        pspSetK1(oldK1);
        return SCE_ERROR_KERNEL_ILLEGAL_PERMISSION_CALL;
    }
        
    if (sceKernelIsIntrContext()) { // 0x00003C08
        pspSetK1(oldK1);
        return SCE_ERROR_KERNEL_CANNOT_BE_CALLED_FROM_INTERRUPT;
    }
    
    //0x00003A34, 0x00003A40 - 0x00003A50, 0x00003B30 - 0x00003BA8
    if ((status = _checkCallConditionKernel()) < 0 || (status = _checkLMOptionConditions(pOption)) < 0) {
        pspSetK1(oldK1);
        return status;
    }
    
    fd = sceIoOpen(path, SCE_O_UNKNOWN0 | SCE_O_RDONLY, FILE_USER_ACCESS_PERMISSIONS); // 0x00003A70
    if (fd < 0) { // 0x00003804
        pspSetK1(oldK1);
        return fd;
    }
    
    status = sceIoIoctl(fd, 0x208009, NULL, 0, NULL, 0); // 0x00003A9C
    if (status < 0) { // 0x00003AA8
        pspSetK1(oldK1);
        return SCE_ERROR_KERNEL_PROHIBIT_LOADMODULE_DEVICE;
    }
    
    pspClearMemory32(&modParams, sizeof(modParams)); //0x00003ABC
        
    modParams.apiType = 0x52; // 0x00003ACC
    modParams.modeFinish = CMD_RELOCATE_MODULE; // 0x00003AD8
    modParams.modeStart = CMD_LOAD_MODULE; // 0x00003AE4
    modParams.unk64 = 0; // 0x00003AF0
    modParams.fd = fd; // 0x00003AF8
    modParams.unk124 = 0;
        
    status = sceIoIoctl(fd, 0x208081, NULL, 0, NULL, 0); //0x00003B14
    if (status >= 0) //0x00003B04
        modParams.unk100 = 0x10;
        
    status = _loadModuleByBufferID(&modParams, pOption); //0x00003710
        
    sceIoClose(fd);
    pspSetK1(oldK1);
    return status;
}

// Subroutine ModuleMgrForUser_50F0C1EC - Address 0x00003D98 - Aliases: ModuleMgrForKernel_3FF74DF1
s32 sceKernelStartModule(SceUID modId, SceSize args, const void *argp, s32 *pModResult, 
        const SceKernelSMOption *pOpt)
{
    s32 oldK1;
    s32 status;
    SceModuleMgrParam modParams;
    
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
    
    status = _checkSMOptionConditions(pOpt); //0x00003FB8 - 0x00003EAC
    if (status < 0) {
        pspSetK1(oldK1);
        return status;
    }
    
    pspClearMemory32(&modParams, sizeof(modParams)); //0x00003EBC
     
    // 0x00003EC8
    modParams.modeFinish = CMD_START_MODULE;
    modParams.modeStart = CMD_START_MODULE;
    modParams.modId = modId; // 0x00003ECC
    modParams.argSize = args;
    modParams.argp = argp;
    modParams.pStatus = pModResult;
    
    if (pOpt != NULL) { //0x00003EDC
        modParams.threadMpIdStack = pOpt->mpidstack;
        modParams.stackSize = pOpt->stacksize;
        modParams.threadPriority = pOpt->priority;
        modParams.threadAttr = pOpt->attribute;
    } else { //0x00003F14
        modParams.threadMpIdStack = 0;
        modParams.stackSize = 0;
        modParams.threadPriority = 0;
        modParams.threadAttr = 0;
    }
    status = _start_exe_thread(&modParams); //0x00003F04
    
    pspSetK1(oldK1);
    return status;
}

// Subroutine ModuleMgrForUser_D1FF982A - Address 0x00003F28 - Aliases: ModuleMgrForKernel_E5D6087B
s32 sceKernelStopModule(SceUID modId, SceSize args, const void *argp, s32 *pModResult, const SceKernelSMOption *pOpt)
{
    s32 oldK1;
    s32 status;
    u32 retAddr;
    SceModule *pMod;
    SceModuleMgrParam modParams;
    
    oldK1 = pspShiftK1();
    retAddr = pspGetRa();
    
    if (sceKernelIsIntrContext()) { //0x00003F68
        pspSetK1(oldK1);
        return SCE_ERROR_KERNEL_CANNOT_BE_CALLED_FROM_INTERRUPT;
    }
    
    if (argp != NULL && !pspK1DynBufOk(argp, args)) { //0x00003F7C, 0x00003F94
        pspSetK1(oldK1);
        return SCE_ERROR_KERNEL_ILLEGAL_ADDR;
    }
    
    if (pModResult != NULL && !pspK1StaBufOk(pModResult, sizeof(*pModResult))) { //0x00003F9C, 0x00003FB0
        pspSetK1(oldK1);
        return SCE_ERROR_KERNEL_ILLEGAL_ADDR;
    }
    
    status = _checkSMOptionConditions(pOpt); //0x00003FB8 - 0x0000404C
    if (status < 0) {
        pspSetK1(oldK1);
        return status;
    }
    
    pMod = sceKernelFindModuleByAddress(retAddr); //0x00004054
    if (pMod == NULL) {
        pspSetK1(oldK1);
        return SCE_ERROR_KERNEL_MODULE_CANNOT_STOP;
    }
    
    pspClearMemory32(&modParams, sizeof(modParams)); //0x00004078
   
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
    modParams.argp = argp;
    modParams.pStatus = pModResult;
    
    if (pOpt != NULL) { //0x000040C4
        modParams.threadMpIdStack = pOpt->mpidstack;
        modParams.stackSize = pOpt->stacksize;
        modParams.threadPriority = pOpt->priority;
        modParams.threadAttr = pOpt->attribute;
    } else { //0x000040FC
        modParams.threadMpIdStack = 0;
        modParams.stackSize = 0;
        modParams.threadPriority = 0;
        modParams.threadAttr = 0;
    }
    status = _start_exe_thread(&modParams); //0x000040EC
    
    pspSetK1(oldK1);
    return status;
}

// Subroutine ModuleMgrForUser_D675EBB8 - Address 0x00004110 - Aliases: ModuleMgrForKernel_5805C1CA
s32 sceKernelSelfStopUnloadModule(s32 modStatus, SceSize args, void *argp)
{
    s32 oldK1;
    s32 retAddr;
    s32 status;
    
    oldK1 = pspShiftK1();
    retAddr = pspGetRa();
    
    if (sceKernelIsIntrContext()) { //0x0000414C
        pspSetK1(oldK1);
        return SCE_ERROR_KERNEL_CANNOT_BE_CALLED_FROM_INTERRUPT;
    }
    
    if (argp != NULL && !pspK1DynBufOk(argp, args)) { //0x00004158, 0x0000416C
        pspSetK1(oldK1);
        return SCE_ERROR_KERNEL_ILLEGAL_ADDR;
    }
    
    if (pspK1IsUserMode()) //0x00004174
        retAddr = sceKernelGetSyscallRA();
    
    if (!pspK1PtrOk(retAddr)) { //0x0000419C
        pspSetK1(oldK1);
        return SCE_ERROR_KERNEL_ILLEGAL_ADDR;
    }
    
    status = _SelfStopUnloadModule(modStatus, retAddr, args, argp, NULL, NULL); //0x000041A4
    
    pspSetK1(oldK1);
    return status;
}

/**
 * Find the id of the module from which this function is called
 * 
 * @param modIdList A pointer which will hold the module id list
 * @param size Maximum size of the returned buffer
 * @param idCount The number of module ids in the list (can be greater than the number of returned ids)
 *
 * @return SCE_ERROR_OK on success, < 0 on error.
 * @return SCE_ERROR_KERNEL_ILLEGAL_ADDR if the provided pointers can't be accessed from the current context or are NULL.
 */
// Subroutine sceKernelGetModuleIdList - Address 0x000041E8 - Aliases: ModuleMgrForKernel_303FAB7F
s32 sceKernelGetModuleIdList(SceUID *modIdList, SceSize size, u32 *idCount)
{
    s32 oldK1;
    s32 retVal;

    oldK1 = pspShiftK1();

    if (modIdList == NULL || idCount == NULL || !pspK1DynBufOk(modIdList, size) || !pspK1StaBufOk(idCount, sizeof(u32))) { // 0x00004200, 0x00004220, 0x00004238, 0x00004244 
        pspSetK1(oldK1);
        return SCE_ERROR_KERNEL_ILLEGAL_ADDR;
    }

    retVal = sceKernelGetModuleIdListForKernel(modIdList, size, idCount, pspK1IsUserMode());
    
    pspSetK1(oldK1);
    return retVal;
}

/**
 * Get module information from id
 * 
 * @param modId The module id
 * @param pModInfo Pointer to SceKernelModuleInfo, content will be modified on success with info from the module
 * 
 * @return SCE_ERROR_OK on success, < 0 on error.
 * @return SCE_ERROR_KERNEL_CANNOT_BE_CALLED_FROM_INTERRUPT if function was called in an interruption.
 * @return SCE_ERROR_KERNEL_ILLEGAL_ADDR if the provided pointer is NULL or can't be accessed from the current context.
 * @return SCE_ERROR_KERNEL_ILLEGAL_SIZE if SDK version >= 2.80  and modInfo->size != sizeof(SceKernelModuleInfoV1) && modInfo->size != sizeof(*modInfo)
 * @return SCE_ERROR_KERNEL_UNKNOWN_MODULE if module couldn't be found
 * @return SCE_ERROR_KERNEL_CANNOT_GET_MODULE_INFO if you don't have the right to access information about this module
 */
// Subroutine sceKernelQueryModuleInfo - Address 0x00004270 - Aliases: ModuleMgrForKernel_22BDBEFF
s32 sceKernelQueryModuleInfo(SceUID modId, SceKernelModuleInfo *pModInfo)
{
    s32 oldK1;
    u32 sdkVersion;
    s32 intrState;
    SceModule *pMod;

    oldK1 = pspShiftK1();

    // Cannot be called from interrupt
    if (sceKernelIsIntrContext()) { // 0x0000429C
        pspSetK1(oldK1);
        return SCE_ERROR_KERNEL_CANNOT_BE_CALLED_FROM_INTERRUPT;
    }

    if (pModInfo == NULL || !pspK1StaBufOk(pModInfo, sizeof(*pModInfo))) { // 0x000042AC, 0x000042BC
        pspSetK1(oldK1);
        return SCE_ERROR_KERNEL_ILLEGAL_ADDR;
    }

    sdkVersion = sceKernelGetCompiledSdkVersion() & 0xFFFF0000; // 0x000042F4

    if (sdkVersion > 0x2070FFFF && pModInfo->size != sizeof(SceKernelModuleInfoV1) // 0x0000430C, 0x00004320
                                && pModInfo->size != sizeof(*pModInfo)) { // 0x00004324
        pspSetK1(oldK1);
        return SCE_ERROR_KERNEL_ILLEGAL_SIZE;
    }

    intrState = sceKernelLoadCoreLock(); // 0x00004334

    pMod = sceKernelFindModuleByUID(modId); // 0x00004340
    if (pMod == NULL) { // 0x00004348
        pspSetK1(oldK1);
        return SCE_ERROR_KERNEL_UNKNOWN_MODULE;
    }

    if (pspK1IsUserMode()) { // 0x00004350
        if (!(pMod->status & SCE_MODULE_USER_MODULE)) {
            pspSetK1(oldK1);
            return SCE_ERROR_KERNEL_CANNOT_GET_MODULE_INFO;
        }

        // TODO: find what 0x1E00 exactly represents
        if ((sceKernelGetUserLevel() == SCE_USER_LEVEL_USBWLAN && pMod->attribute & 0x1E00 != SCE_MODULE_USB_WLAN) // 0x00004368,0x00004370,0x000044C0
            || (sceKernelGetUserLevel() == SCE_USER_LEVEL_MS && pMod->attribute & 0x1E00 != SCE_MODULE_MS) // 0x0000437C,0x00004388,0x000044A8
            || (sceKernelGetUserLevel() == SCE_USER_LEVEL_APP && pMod->attribute & 0x1E00 != SCE_MODULE_APP) // 0x00004390,0x0000439C,0x00004490
            && (pMod->attribute & 0x1E00) // 0x000043A8
            ) {

            sceKernelLoadCoreUnlock(intrState);
            pspSetK1(oldK1);
            return SCE_ERROR_KERNEL_CANNOT_GET_MODULE_INFO;
        }
    }

    s32 modStatus = GET_MCB_STATUS(pMod->status);
    if (modStatus == MCB_STATUS_NOT_LOADED || modStatus == MCB_STATUS_LOADING || modStatus == MCB_STATUS_LOADED 
             || modStatus == MCB_STATUS_UNLOADED) { // 0x000043C0
        sceKernelLoadCoreUnlock(intrState);
        pspSetK1(oldK1);
        return SCE_ERROR_KERNEL_CANNOT_GET_MODULE_INFO;
    }

    pModInfo->nsegment = pMod->nSegments; // 0x000043C8

    for (int i = 0; i < SCE_KERNEL_MAX_MODULE_SEGMENT; i++) { // 0x000043F0
        pModInfo->segmentAddr[i] = pMod->segmentAddr[i]; // 0x000043E4
        pModInfo->segmentSize[i] = pMod->segmentSize[i]; // 0x000043F4
    }

    pModInfo->entryAddr = pMod->entryAddr; // 0x00004404
    pModInfo->gpValue = pMod->gpValue; // 0x0000440C
    pModInfo->textAddr = pMod->textAddr; // 0x00004414
    pModInfo->textSize = pMod->textSize; // 0x0000441C
    pModInfo->dataSize = pMod->dataSize; // 0x00004424
    pModInfo->bssSize = pMod->bssSize; // 0x00004430

    // If we have a v1 structure (less fields, size: 64)
    if (pModInfo->size != sizeof(SceKernelModuleInfo)) {
        sceKernelLoadCoreUnlock(intrState);
        pspSetK1(oldK1);
        return SCE_ERROR_OK;
    }

    // If we have a v2 structure (more fields, size: 96)
    // TODO: find what 0x1E00 exactly represents
    pModInfo->attribute = pMod->attribute & ~0x1E00;
    pModInfo->version[MODULE_VERSION_MINOR] = pMod->version[MODULE_VERSION_MINOR];
    pModInfo->version[MODULE_VERSION_MAJOR] = pMod->version[MODULE_VERSION_MAJOR];
    strncpy(pModInfo->modName, pModInfo->modName, SCE_MODULE_NAME_LEN);
    pModInfo->terminal = pMod->terminal;

    sceKernelLoadCoreUnlock(intrState);
    pspSetK1(oldK1);
    return SCE_ERROR_OK;
}

/**
 * Find the id of the module from which this function is called
 * 
 * @return The module id on success, < 0 on error.
 * @return SCE_ERROR_KERNEL_CANNOT_BE_CALLED_FROM_INTERRUPT if function was called in an interruption.
 * @return SCE_ERROR_KERNEL_ILLEGAL_ADDR if the provided pointer can't be accessed from the current context.
 * @return SCE_ERROR_KERNEL_ERROR if module couldn't be found.
 */
// Subroutine ModuleMgrForUser_F0A26395 - Address 0x000058F8 - Aliases: ModuleMgrForKernel_CECA0FFC
s32 sceKernelGetModuleId(void)
{
    s32 oldK1;
    s32 retAddr;
    s32 intrState;
    s32 retVal;
    SceModule *pMod;
    
    oldK1 = pspShiftK1();
    retAddr = pspGetRa();
    
    if (sceKernelIsIntrContext()) { //0x0000450C
        pspSetK1(oldK1);
        return SCE_ERROR_KERNEL_CANNOT_BE_CALLED_FROM_INTERRUPT;
    }
    
    if (pspK1IsUserMode()) //0x00004520
        retAddr = sceKernelGetSyscallRA();
    
    if (!pspK1PtrOk(retAddr)) { //0x0000452C
        pspSetK1(oldK1);
        return SCE_ERROR_KERNEL_ILLEGAL_ADDR;
    }
    
    intrState = sceKernelLoadCoreLock();
    
    pMod = sceKernelFindModuleByAddress(retAddr); //0x00004544
    if (pMod == NULL)
        retVal = SCE_ERROR_KERNEL_ERROR;
    else
        retVal = pMod->modId;
    
    sceKernelLoadCoreUnlock(intrState); //0x0000455C
    
    pspSetK1(oldK1);
    return retVal;
}

/**
 * Find the id of the module whose codeAddr belongs to
 * 
 * @param codeAddr An address inside the module
 * 
 * @return The module id on success, < 0 on error.
 * @return SCE_ERROR_KERNEL_CANNOT_BE_CALLED_FROM_INTERRUPT if function was called in an interruption.
 * @return SCE_ERROR_KERNEL_ILLEGAL_ADDR if the provided pointer can't be accessed from the current context.
 * @return SCE_ERROR_KERNEL_UNKNOWN_MODULE if module couldn't be found.
 */
 // Subroutine sceKernelGetModuleIdByAddress - Address 0x00004598 - Aliases: ModuleMgrForKernel_433D5287
s32 sceKernelGetModuleIdByAddress(const void *codeAddr)
{
    s32 oldK1;
    s32 intrState;
    s32 retVal;
    SceModule *pMod;

    oldK1 = pspShiftK1();
    
    if (sceKernelIsIntrContext()) { // 0x000045B4
        pspSetK1(oldK1);
        return SCE_ERROR_KERNEL_CANNOT_BE_CALLED_FROM_INTERRUPT;
    }

    if (!pspK1PtrOk(codeAddr)) { // 0x000045D0
        pspSetK1(oldK1);
        return SCE_ERROR_KERNEL_ILLEGAL_ADDR;
    }

    intrState = sceKernelLoadCoreLock(); // 0x000045D8

    pMod = sceKernelFindModuleByAddress(codeAddr); // 0x000045E4
    if (pMod == NULL)
        retVal = SCE_ERROR_KERNEL_UNKNOWN_MODULE;
    else
        retVal = pMod->modId;

    sceKernelLoadCoreUnlock(intrState); // 0x00004600

    pspSetK1(oldK1);
    return retVal;
}

/**
 * Find the offset from the start of the TEXT segment of the module whose codeAddr belongs to
 * 
 * @param codeAddr An address inside the module
 * @param pGP A pointer to a location where the GP offset will be stored
 * 
 * @return SCE_ERROR_OK and sets pGP on success, < 0 on error.
 * @return SCE_ERROR_KERNEL_ILLEGAL_ADDR if the provided pointer can't be accessed from the current context.
 * @return SCE_ERROR_KERNEL_UNKNOWN_MODULE if module couldn't be found.
 */
// Subroutine sceKernelGetModuleGPByAddress - Address 0x00004628 
s32 sceKernelGetModuleGPByAddress(const void *codeAddr, u32 *pGP)
{
    s32 oldK1;
    s32 intrState;
    s32 retVal;
    SceModule *pMod;

    oldK1 = pspShiftK1(); // 0x0000463C

    if (!pspK1PtrOk(codeAddr) || pGP == NULL || !pspK1PtrOk(pGP)) { // 0x00004660, 0x00004670, 0x0000467C
        pspSetK1(oldK1);
        return SCE_ERROR_KERNEL_ILLEGAL_ADDR;
    }

    intrState = sceKernelLoadCoreLock(); // 0x00004684

    pMod = sceKernelFindModuleByAddress(codeAddr); // 0x00004698
    if (pMod == NULL) // 0x000046A0
        retVal = SCE_ERROR_KERNEL_UNKNOWN_MODULE; // 0x0000469C
    else {
        retVal = SCE_ERROR_OK; // 0x000046AC
        *pGP = pMod->gpValue; // 0x000046B0
    }

    sceKernelLoadCoreUnlock(intrState); // 0x000046B4

    pspSetK1(oldK1);
    return retVal;
}

// Subroutine ModuleMgrForKernel_CC873DFA - Address 0x000046E4
s32 sceKernelRebootBeforeForUser(void *arg)
{
    s32 oldGp;
    u32 modCount;
    s32 status;
    SceUID uidBlkId;
    SceUID *uidList;
    char threadArgs[16];
    
    oldGp = pspGetGp();
    sceKernelLockMutex(g_ModuleManager.semaId, 1, NULL); //0x00004724
    
    memcpy(threadArgs, arg, sizeof(threadArgs)); //0x00004734
    ((u32 *)threadArgs)[0] = 16;
    
    uidBlkId = sceKernelGetModuleListWithAlloc(&modCount); //0x00004744
    if (uidBlkId < SCE_ERROR_OK)
        return uidBlkId;
    
    uidList = sceKernelGetBlockHeadAddr(uidBlkId); //0x00004754
    
    u32 i;
    SceModule *pMod;
    s32 threadMode;
    for (i = modCount - 1; i >= 0; i--) { //0x00004760
        pMod = sceKernelFindModuleByUID(uidList[i]); //0x00004774
        if (pMod == NULL || ((s32)pMod->moduleRebootBefore) == -1) //0x0000477C
            continue;
        
        if (GET_MCB_STATUS(pMod->status) != MCB_STATUS_STARTED || !(pMod->status & SCE_MODULE_USER_MODULE)) //0x0000479C - 0x00004830
            continue;
        
        s32 priority = pMod->moduleRebootBeforeThreadPriority;
        if (priority == -1)
            priority = SCE_KERNEL_MODULE_INIT_PRIORITY; //0x00004864
        
        s32 stackSize = pMod->moduleRebootBeforeThreadStacksize;
        if (stackSize == -1) //0x00004870
            stackSize = SCE_KERNEL_TH_DEFAULT_SIZE;
        
        s32 attr = pMod->moduleRebootBeforeThreadAttr;
        if (attr == -1) //0x00004874
            attr = SCE_KERNEL_TH_DEFAULT_ATTR;
        
        // TODO: Add proper define for 0x1E00
        switch (pMod->attribute & 0x1E00) {
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
        threadParams.size = sizeof(threadParams); //0x000048AC
        threadParams.stackMpid = pMod->mpIdData; //0x000048CC
        
        status = _CheckUserModulePartition(pMod->mpIdData); // 0x000048BC - 0x000048E0
        if (status < SCE_ERROR_OK)
            threadParams.stackMpid = SCE_KERNEL_PRIMARY_USER_PARTITION;
                
        pspSetGp(pMod->gpValue); //0x00004900
        
        pMod->userModThid = sceKernelCreateThread("SceModmgrRebootBefore", pMod->moduleRebootBefore, priority, 
                stackSize, threadMode | attr, &threadParams); //0x0000491C
        
        pspSetGp(oldGp);
        
        // TODO: Add proper structure for threadArgs
        status = sceKernelStartThread(pMod->userModThid, sizeof threadArgs, threadArgs); //0x00004934
        if (status == SCE_ERROR_OK) 
            sceKernelWaitThreadEnd(pMod->userModThid, NULL); //0x000049AC
        
        sceKernelDeleteThread(pMod->userModThid); //0x00004944
        pMod->userModThid = -1;
        
        if (!sceKernelIsToolMode()) //0x00004954
            continue;
        
        status = sceKernelDipsw(25); //0x0000495C
        if (status == 1) //0x00004968
            continue;
        
        s32 sdkVersion = sceKernelGetCompiledSdkVersion(); //0x00004970
        if (sdkVersion < 0x03030000) //0x00004984
            continue;
        
        s32 checkSum = sceKernelSegmentChecksum(pMod);
        if (checkSum == pMod->segmentChecksum)
            continue;
        
        pspBreak(0);
        continue;
    }
    
    SceSysmemMemoryBlockInfo blkInfo;
    blkInfo.size = sizeof(SceSysmemMemoryBlockInfo);
    status = sceKernelQueryMemoryBlockInfo(uidBlkId, &blkInfo); //0x000047BC
    if (status < SCE_ERROR_OK) //0x000047C4
        return status;
    
    sceKernelMemset(blkInfo.addr, 0, blkInfo.memSize); //0x000047F0
    status = sceKernelFreePartitionMemory(uidBlkId);
    
    return status;
}

// Subroutine ModuleMgrForKernel_9B7102E2 - Address 0x000049BC
s32 sceKernelRebootPhaseForKernel(SceSize args, void *argp, s32 arg3, s32 arg4)
{
    SceUID uidBlkId;
    SceUID *uidList;
    SceModule *pMod;
    s32 modCount;
    s32 status;
    
    uidBlkId = sceKernelGetModuleListWithAlloc(&modCount); //0x000049F8
    if (uidBlkId < SCE_ERROR_OK)
        return uidBlkId;
    
    uidList = sceKernelGetBlockHeadAddr(uidBlkId); //0x00004A08
    
    s32 i;
    for (i = modCount - 1; i >= 0; i--) { //0x00004A14 - 0x00004A64
        pMod = sceKernelFindModuleByUID(uidList[i]); //0x00004A34
        if (pMod == NULL || ((s32)pMod->moduleRebootPhase) == -1) //0x00004A3C, 0x00004A48
            continue;
        
        if (GET_MCB_STATUS(pMod->status) != MCB_STATUS_STARTED || (pMod->status & SCE_MODULE_USER_MODULE)) //0x00004A58 - 0x00004B04
            continue;
        
        // TODO: Re-define moduleRebootPhase (it currently is a SceKernelThreadEntry)
        pMod->moduleRebootPhase(args, argp, arg3, arg4); //0x00004B0C
        
        if (!sceKernelIsToolMode()) //0x00004B1C
            continue;
        
        status = sceKernelDipsw(25); //0x00004B24
        if (status == 1) //0x00004B30
            continue;
        
        s32 sdkVersion = sceKernelGetCompiledSdkVersion(); //0x00004B38
        if (sdkVersion < 0x03030000) //0x00004B44
            continue;
        
        s32 checkSum = sceKernelSegmentChecksum(pMod); //0x00004B4C
        if (checkSum == pMod->segmentChecksum) //0x00004B58
            continue;
        
        pspBreak(0);
        continue;
    }
    
    status = ClearFreePartitionMemory(uidBlkId); // 0x00004A74 - 0x00004AB4
    return ((status > SCE_ERROR_OK) ? SCE_ERROR_OK : status);
}

// ModuleMgrForKernel_5FC3B3DA - Address 0x00004B6C
s32 sceKernelRebootBeforeForKernel(void *argp, s32 arg2, s32 arg3, s32 arg4)
{
    SceUID uidBlkId;
    SceUID *uidList;
    SceModule *pMod;
    s32 modCount;
    s32 status;
    
    uidBlkId = sceKernelGetModuleListWithAlloc(&modCount); //0x00004BA8
    if (uidBlkId < SCE_ERROR_OK)
        return uidBlkId;
    
    uidList = sceKernelGetBlockHeadAddr(uidBlkId); //0x00004BB8
    
    s32 i;
    for (i = modCount - 1; i >= 0; i--) { //0x00004BC4 - 0x00004C10
        pMod = sceKernelFindModuleByUID(uidList[i]); //0x00004BE4
        if (pMod == NULL || ((s32)pMod->moduleRebootBefore) == -1) //0x00004BEC, 0x00004BF8
            continue;
        
        if (GET_MCB_STATUS(pMod->status) != MCB_STATUS_STARTED || (pMod->status & SCE_MODULE_USER_MODULE)) //0x00004C08,  0x00004CB0
            continue;
        
        // TODO: Re-define moduleRebootBefore (it currently is a SceKernelThreadEntry)
        pMod->moduleRebootBefore(argp, arg2, arg3, arg4); //0x00004CB8
    }
    
    status = ClearFreePartitionMemory(uidBlkId); // 0x00004C24 - 0x00004C60
    return ((status > SCE_ERROR_OK) ? SCE_ERROR_OK : status);
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

// Subroutine ModuleMgrForKernel_61E3EC69 - Address 0x000050FC
s32 sceKernelLoadModuleBufferForExitGame(u32 *modBuf, s32 flags, SceKernelLMOption *option, s32 opt)
{
    s32 oldK1;
    s32 status;
    SceModuleMgrParam modParams;
    
    (void)flags;

    oldK1 = pspShiftK1(); // 0x00005108

    // Cannot be called in an interruption
    if (sceKernelIsIntrContext()) { // 0x0000511C
        pspSetK1(oldK1);
        return SCE_ERROR_KERNEL_CANNOT_BE_CALLED_FROM_INTERRUPT;
    }
    
    // 0x00005138
    if (status = _checkCallConditionKernel() < 0) {
        pspSetK1(oldK1);
        return status;
    }
    //0x00005134 - 0x0000514C
    if (!pspK1PtrOk(modBuf)) {
        pspSetK1(oldK1);
        return SCE_ERROR_KERNEL_ILLEGAL_ADDR;
    }
    
    status = _checkLMOptionConditions(option); //0x00005170
    if (status < 0) {
        pspSetK1(oldK1);
        return status;
    }
    
    _createMgrParamStruct(&modParams, SCE_INIT_APITYPE_VSH_1, modBuf, NULL, opt); // 0x00005190
    status = _loadModuleByBufferID(&modParams, option); //0x0000519C
    
    pspSetK1(oldK1);
    return status;
}

// Subroutine ModuleMgrForUser_1196472E - Address 0x000051AC
s32 sceKernelLoadModuleBufferMs(SceSize bufSize, void *pBuffer, u32 flags, const SceKernelLMOption *pOption)
{
    s32 oldK1;
    s32 status;
    SceModuleMgrParam modParams;

    (void)flags;
    
    oldK1 = pspShiftK1(); // 0x000051B8

    // Cannot be called in an interruption
    if (sceKernelIsIntrContext()) { // 0x000051D4
        pspSetK1(oldK1);
        return SCE_ERROR_KERNEL_CANNOT_BE_CALLED_FROM_INTERRUPT;
    }  
    
    if ((status = _checkCallConditionUser()) < 0) { // 0x000051E8
        pspSetK1(oldK1);
        return status;
    }  

    /* Check for MS API */
    if (sceKernelGetUserLevel() != SCE_USER_LEVEL_MS && sceKernelGetUserLevel() != SCE_USER_LEVEL_APP) { // 0x00005220 & 0x0000522C
        pspSetK1(oldK1);
        return SCE_ERROR_KERNEL_ILLEGAL_PERMISSION_CALL;
    }

    if (!pspK1DynBufOk(pBuffer, bufSize)) { // 0x00005258
        pspSetK1(oldK1);
        return SCE_ERROR_KERNEL_ILLEGAL_ADDR;
    }
    
    status = _checkLMOptionConditions(pOption); //0x00005260
    if (status < 0) { // 0x0000526C
        pspSetK1(oldK1);
        return status;
    }

    _createMgrParamStruct(&modParams, 0x42, pBuffer, bufSize, 0); // 0x00005280
    status = _loadModuleByBufferID(&modParams, pOption); // 0x00005368
    
    pspSetK1(oldK1);
    return status;
}

// Subroutine ModuleMgrForUser_24EC0641 - Address 0x0000529C
s32 sceKernelLoadModuleBufferApp(SceSize bufSize, void *pBuffer, u32 flags, const SceKernelLMOption *pOption)
{
    s32 oldK1;
    s32 status;
    SceModuleMgrParam modParams;

    (void)flags;
    
    oldK1 = pspShiftK1(); // 0x000052A8

    // Cannot be called in an interruption
    if (sceKernelIsIntrContext()) { // 0x000052C4
        pspSetK1(oldK1);
        return SCE_ERROR_KERNEL_CANNOT_BE_CALLED_FROM_INTERRUPT;
    }  
    
    if ((status = _checkCallConditionUser()) < 0) { // 0x000052D8
        pspSetK1(oldK1);
        return status;
    }  

    /* Check for APP API */
    if (sceKernelGetUserLevel() != SCE_USER_LEVEL_APP) { // 0x00005308
        pspSetK1(oldK1);
        return SCE_ERROR_KERNEL_ILLEGAL_PERMISSION_CALL;
    }

    if (!pspK1DynBufOk(pBuffer, bufSize)) { // 0x00005334
        pspSetK1(oldK1);
        return SCE_ERROR_KERNEL_ILLEGAL_ADDR;
    }
    
    status = _checkLMOptionConditions(pOption); // 0x0000533C
    if (status < 0) { // 0x00005348
        pspSetK1(oldK1);
        return status;
    }

    _createMgrParamStruct(&modParams, 0x43, pBuffer, bufSize, 0); // 0x0000535C
    status = _loadModuleByBufferID(&modParams, pOption); //0x00005368
    
    pspSetK1(oldK1);
    return status;
}

// Subroutine ModuleMgrForKernel_2F3F9B6A - Address 0x00005378
s32 sceKernelLoadModuleBufferForLoadExecBufferVSHUsbWlan(s32 apiType, u32 *modBuf, s32 flags, SceKernelLMOption *option)
{
    s32 oldK1;
    s32 status;
    SceModuleMgrParam modParams;
    
    (void)flags;

    oldK1 = pspShiftK1(); // 0x00005384

    // Cannot be called in an interruption
    if (sceKernelIsIntrContext()) { // 0x000053A0
        pspSetK1(oldK1);
        return SCE_ERROR_KERNEL_CANNOT_BE_CALLED_FROM_INTERRUPT;
    }
    
    // 0x000053BC
    if (status = _checkCallConditionKernel() < 0) {
        pspSetK1(oldK1);
        return status;
    }
    // 0x000053CC
    if (!pspK1PtrOk(modBuf)) {
        pspSetK1(oldK1);
        return SCE_ERROR_KERNEL_ILLEGAL_ADDR;
    }
    
    status = _checkLMOptionConditions(option); // 0x000053F8
    if (status < 0) {
        pspSetK1(oldK1);
        return status;
    }
    
    _createMgrParamStruct(&modParams, apiType, modBuf, NULL, 0); // 0x00005418
    status = _loadModuleByBufferID(&modParams, option); // 0x00005424
    
    pspSetK1(oldK1);
    return status;
}

// Subroutine ModuleMgrForKernel_C13E2DE5 - Address 0x00005434
s32 sceKernelLoadModuleBufferForLoadExecBufferVSHUsbWlanDebug(s32 apiType, u32 *modBuf, s32 flags, SceKernelLMOption *option)
{
    s32 oldK1;
    s32 status;
    SceModuleMgrParam modParams;
    
    (void)flags;

    oldK1 = pspShiftK1(); // 0x00005440

    // Cannot be called in an interruption
    if (sceKernelIsIntrContext()) { // 0x0000545C
        pspSetK1(oldK1);
        return SCE_ERROR_KERNEL_CANNOT_BE_CALLED_FROM_INTERRUPT;
    }
    
    // 0x00005478
    if (status = _checkCallConditionKernel() < 0) {
        pspSetK1(oldK1);
        return status;
    }
    // 0x00005488
    if (!pspK1PtrOk(modBuf)) {
        pspSetK1(oldK1);
        return SCE_ERROR_KERNEL_ILLEGAL_ADDR;
    }
    
    status = _checkLMOptionConditions(option); // 0x000054B4
    if (status < 0) {
        pspSetK1(oldK1);
        return status;
    }
    
    _createMgrParamStruct(&modParams, apiType, modBuf, NULL, 0); // 0x000054D4
    status = _loadModuleByBufferID(&modParams, option); // 0x000054E0
    
    pspSetK1(oldK1);
    return status;
}

// Subroutine ModuleMgrForKernel_C6DE0B9C - Address 0x000054F0
s32 sceKernelLoadModuleBufferVSH(SceSize bufSize, void *pBuffer, u32 flags, const SceKernelLMOption *pOption)
{
    s32 oldK1;
    s32 status;
    SceModuleMgrParam modParams;

    (void)flags;
    
    oldK1 = pspShiftK1(); // 0x000054FC

    // Cannot be called in an interruption
    if (sceKernelIsIntrContext()) { // 0x00005518
        pspSetK1(oldK1);
        return SCE_ERROR_KERNEL_CANNOT_BE_CALLED_FROM_INTERRUPT;
    }  
    
    if ((status = _checkCallConditionKernel()) < 0) { // 0x0000552C
        pspSetK1(oldK1);
        return status;
    }  

    /* Check for VSH API */
    if (sceKernelGetUserLevel() != SCE_USER_LEVEL_VSH) { // 0x0000555C
        pspSetK1(oldK1);
        return SCE_ERROR_KERNEL_ILLEGAL_PERMISSION_CALL;
    }

    if (!pspK1DynBufOk(pBuffer, bufSize)) { // 0x00005588
        pspSetK1(oldK1);
        return SCE_ERROR_KERNEL_ILLEGAL_ADDR;
    }
    
    status = _checkLMOptionConditions(pOption); // 0x00005590
    if (status < 0) { // 0x0000559C
        pspSetK1(oldK1);
        return status;
    }

    _createMgrParamStruct(&modParams, 0x21, pBuffer, bufSize, 0); // 0x000055B0
    status = _loadModuleByBufferID(&modParams, pOption); // 0x000055BC
    
    pspSetK1(oldK1);
    return status;
}

// Subroutine ModuleMgrForKernel_9236B422 - Address 0x000055CC
s32 sceKernelLoadModuleBufferForExitVSHVSH(u32 *modBuf, s32 flags, SceKernelLMOption *pOption, s32 opt)
{
    s32 oldK1;
    s32 status;
    SceModuleMgrParam modParams;
    
    (void)flags;
    (void)opt;

    oldK1 = pspShiftK1(); // 0x000055D8

    // Cannot be called in an interruption
    if (sceKernelIsIntrContext()) { // 0x000055EC
        pspSetK1(oldK1);
        return SCE_ERROR_KERNEL_CANNOT_BE_CALLED_FROM_INTERRUPT;
    }
    
    // 0x00005608
    if (status = _checkCallConditionKernel() < 0) {
        pspSetK1(oldK1);
        return status;
    }
    
    // 0x00005618
    if (!pspK1PtrOk(modBuf)) {
        pspSetK1(oldK1);
        return SCE_ERROR_KERNEL_ILLEGAL_ADDR;
    }
    
    status = _checkLMOptionConditions(pOption); // 0x00005640
    if (status < 0) {
        pspSetK1(oldK1);
        return status;
    }
    
    _createMgrParamStruct(&modParams, SCE_INIT_APITYPE_VSH_2, modBuf, NULL, 1); // 0x00005660
    status = _loadModuleByBufferID(&modParams, pOption); //0x0000566C
    
    pspSetK1(oldK1);
    return status;
}

// Subroutine ModuleMgrForKernel_4E62C48A - Address 0x0000567C
s32 sceKernelLoadModuleBufferForKernel(SceSize bufSize, void *pBuffer, u32 flags, const SceKernelLMOption *pOption)
{
    s32 oldK1;
    s32 status;
    SceModuleMgrParam modParams;

    (void)flags;
    
    oldK1 = pspShiftK1(); // 0x00005688

    // Cannot be called in an interruption
    if (sceKernelIsIntrContext()) { // 0x000056A4
        pspSetK1(oldK1);
        return SCE_ERROR_KERNEL_CANNOT_BE_CALLED_FROM_INTERRUPT;
    }  
    
    if ((status = _checkCallConditionKernel()) < 0) { // 0x000056CC
        pspSetK1(oldK1);
        return status;
    } 

    if (!pspK1DynBufOk(pBuffer, bufSize)) { // 0x000056DC
        pspSetK1(oldK1);
        return SCE_ERROR_KERNEL_ILLEGAL_ADDR;
    }
    
    status = _checkLMOptionConditions(pOption); // 0x00005708
    if (status < 0) { // 0x00005714
        pspSetK1(oldK1);
        return status;
    }

    _createMgrParamStruct(&modParams, 0x2, pBuffer, bufSize, 0); // 0x00005728
    status = _loadModuleByBufferID(&modParams, pOption); // 0x00005734
    
    pspSetK1(oldK1);
    return status;
}

// Subroutine ModuleMgrForKernel_253AA17C - Address 0x00005744
s32 sceKernelLoadModuleBufferForExitVSHKernel(u32 *modBuf, s32 flags, SceKernelLMOption *pOption, s32 opt)
{
    s32 oldK1;
    s32 status;
    SceModuleMgrParam modParams;
    
    (void)flags;
    (void)opt;

    oldK1 = pspShiftK1(); // 0x00005750

    // Cannot be called in an interruption
    if (sceKernelIsIntrContext()) { // 0x00005764
        pspSetK1(oldK1);
        return SCE_ERROR_KERNEL_CANNOT_BE_CALLED_FROM_INTERRUPT;
    }
    
    // 0x00005780
    if (status = _checkCallConditionKernel() < 0) {
        pspSetK1(oldK1);
        return status;
    }
    
    // 0x00005790
    if (!pspK1PtrOk(modBuf)) {
        pspSetK1(oldK1);
        return SCE_ERROR_KERNEL_ILLEGAL_ADDR;
    }
    
    status = _checkLMOptionConditions(pOption); // 0x000057B8
    if (status < 0) {
        pspSetK1(oldK1);
        return status;
    }
    
    _createMgrParamStruct(&modParams, SCE_INIT_APITYPE_KERNEL_1, modBuf, NULL, 1); // 0x000057D8
    status = _loadModuleByBufferID(&modParams, pOption); //0x000057E4
    
    pspSetK1(oldK1);
    return status;
}

// Subroutine ModuleMgrForKernel_4E38EA1D - Address 0x000057F4
s32 sceKernelLoadModuleBufferForRebootKernel(u32 *modBuf, s32 flags, SceKernelLMOption *pOption, s32 opt)
{
    s32 oldK1;
    s32 status;
    SceModuleMgrParam modParams;
    
    (void)flags;
    (void)opt;

    oldK1 = pspShiftK1(); // 0x00005800

    // Cannot be called in an interruption
    if (sceKernelIsIntrContext()) { // 0x00005814
        pspSetK1(oldK1);
        return SCE_ERROR_KERNEL_CANNOT_BE_CALLED_FROM_INTERRUPT;
    }
    
    // 0x00005830
    if (status = _checkCallConditionKernel() < 0) {
        pspSetK1(oldK1);
        return status;
    }
    
    // 0x00005840
    if (!pspK1PtrOk(modBuf)) {
        pspSetK1(oldK1);
        return SCE_ERROR_KERNEL_ILLEGAL_ADDR;
    }
    
    status = _checkLMOptionConditions(pOption); // 0x00005868
    if (status < 0) {
        pspSetK1(oldK1);
        return status;
    }
    
    _createMgrParamStruct(&modParams, SCE_INIT_APITYPE_KERNEL_REBOOT, modBuf, NULL, 1); // 0x00005888
    status = _loadModuleByBufferID(&modParams, pOption); //0x00005894
    
    pspSetK1(oldK1);
    return status;
}

// Subroutine ModuleMgrForKernel_955D6CB2 - Address 0x000058A4
// TODO: Check back function name
s32 sceKernelLoadModuleBootInitBtcnf(u32 *modBuf, s32 flags, SceKernelLMOption *pOption)
{
    (void)modBuf;
    (void)flags;
    (void)pOption;
    
    return SCE_ERROR_KERNEL_ILLEGAL_PERMISSION_CALL;
}

// Subroutine ModuleMgrForKernel_1CF0B794 - Address 0x000058B0
s32 sceKernelLoadModuleBufferBootInitBtcnf(SceSize modSize, u32 *modBuf, s32 flags, SceKernelLMOption *pOption, s32 opt)
{
    s32 oldK1;
    s32 status;
    SceModuleMgrParam modParams;

    (void)flags;
    
    oldK1 = pspShiftK1(); // 0x000058BC

    // Cannot be called in an interruption
    if (sceKernelIsIntrContext()) { // 0x000058D8
        pspSetK1(oldK1);
        return SCE_ERROR_KERNEL_CANNOT_BE_CALLED_FROM_INTERRUPT;
    }  
    
    if ((status = _checkCallConditionKernel()) < 0) { // 0x00005900
        pspSetK1(oldK1);
        return status;
    } 

    if (!pspK1DynBufOk(modBuf, modSize)) { // 0x00005910
        pspSetK1(oldK1);
        return SCE_ERROR_KERNEL_ILLEGAL_ADDR;
    }
    
    status = _checkLMOptionConditions(pOption); // 0x0000593C
    if (status < 0) { // 0x00005714
        pspSetK1(oldK1);
        return status;
    }

    _createMgrParamStruct(&modParams, 0x51, modBuf, modSize, 1); // 0x0000595C
    status = _loadModuleByBufferID(&modParams, pOption); // 0x00005968
    
    pspSetK1(oldK1);
    return status;
}

// Subroutine ModuleMgrForKernel_5FC32087 - Address 0x00005978
s32 sceKernelLoadModuleByIDBootInitConfig(void)
{
    return SCE_ERROR_KERNEL_ILLEGAL_PERMISSION_CALL;
}

// Subroutine ModuleMgrForKernel_E8B9D19D - Address 0x00005984
s32 sceKernelLoadModuleBufferBootInitConfig(void)
{
    return SCE_ERROR_KERNEL_ILLEGAL_PERMISSION_CALL;
}

// Subroutine ModuleMgrForUser_2E0911AA - Address 0x00005990 - Aliases: ModuleMgrForKernel_387E3CA9
s32 sceKernelUnloadModule(SceUID modId)
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
    
    pspClearMemory32(&modParams, sizeof(modParams)); //0x000059C8
     
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
s32 sceKernelStopUnloadSelfModuleWithStatus(s32 exitStatus, SceSize args, void *argp, s32 *pModResult, SceKernelSMOption *pOption)
{
    return _StopUnloadSelfModuleWithStatus(exitStatus, pspGetRa(), args, argp, pModResult, pOption);
}

// Subroutine ModuleMgrForUser_CC1D3699 - Address 0x00005A4C - Aliases: ModuleMgrForKernel_E97E0DB7
s32 sceKernelStopUnloadSelfModule(SceSize args, void *argp, s32 *pModResult, SceKernelSMOption *pOption)
{
    return _StopUnloadSelfModuleWithStatus(SCE_ERROR_OK, pspGetRa(), args, argp, pModResult, pOption);
}

// Subroutine ModuleMgrForKernel_D86DD11B - Address 0x00005A80
s32 sceKernelSearchModuleByName(const char *name)
{
    s32 oldK1;
    SceModule *pMod;
    
    oldK1 = pspShiftK1();
    
    if (!pspK1PtrOk(name)) {
        pspSetK1(oldK1);
        return SCE_ERROR_KERNEL_ILLEGAL_ADDR;
    }
    
    pMod = sceKernelFindModuleByName(name); // 0x00005AA8
    
    pspSetK1(oldK1);
    return (pMod != NULL) ? pMod->modId : SCE_ERROR_KERNEL_UNKNOWN_MODULE;
}

// Subroutine ModuleMgrForKernel_12F99392 - Address 0x00005AE0
s32 sceKernelSearchModuleByAddress(u32 addr)
{
    SceModule *pMod;
    
    pMod = sceKernelFindModuleByAddress(addr); // 0x00005AE8
    
    return (pMod != NULL) ? pMod->modId : SCE_ERROR_KERNEL_UNKNOWN_MODULE;
}

// Subroutine ModuleMgrForUser_CDE1C1FE - Address 0x00005B10
s32 ModuleMgrForUser_CDE1C1FE()
{
    // TODO: Figure out structure member unk36 of SceModuleManagerCB
}

// Subroutine ModuleMgrForKernel_A40EC254 - Address 0x00005B6C
s32 sceKernelSetNpDrmGetModuleKeyFunction(void (*function)(s32 fd, void *, void *))
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
        
        status = _CheckOverride(modParams->apiType, pBlock, &data); //0x00005D74
        if (status == 0) // 0x00005D7C
            break;
        
        if (data < 0) // 0x00005D84
            while (1) { } // 0x00005D98
        
        fd = data;
        modParams->unk124 = 1; // 0x00005D90
    }
    while (1); // 0x00005D90
    
    s32 data2;
    status = _CheckSkipPbpHeader(modParams, fd, pBlock, &data2, modParams->apiType); // 0x00005DB0
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

// Subroutine sub_000075B4 - Address 0x000075B4 
SceUID _loadModuleByBufferID(SceModuleMgrParam *modParams, const SceKernelLMOption *pOpt)
{
    if (pOpt == NULL) { // 0x000075BC
        modParams->access = 0x1;
        modParams->mpIdText = 0;
        modParams->mpIdData = 0;
        modParams->position = SCE_KERNEL_SMEM_Low;
    } else {
        modParams->mpIdText = pOpt->mpIdText;
        modParams->mpIdData = pOpt->mpIdData;
        modParams->position = pOpt->position;
        modParams->access = pOpt->access;
    }

    // 0x000075E4
    modParams->unk76 = 0;
    modParams->unk80 = 0;
    modParams->unk96 = 0;
    modParams->execInfo = NULL;

    return _start_exe_thread(modParams); // 0x000075F4
}

// Subroutine sub_00007698 - Address 0x00007698
SceModuleMgrParam* _createMgrParamStruct(SceModuleMgrParam *modParams, u32 apiType, SceUID fd, void *file_buffer, u32 unk124)
{
    pspClearMemory32(modParams); // 0x000076A0

    modParams->unk124 = unk124; // 0x000076AC
    modParams->apiType = apiType; // 0x000076B0
    modParams->modeFinish = CMD_RELOCATE_MODULE; // 0x000076B4
    modParams->file_buffer = file_buffer; // 0x000076B8
    modParams->fd = fd; // 0x000076BC
    modParams->modeStart = CMD_RELOCATE_MODULE; // 0x000076C0

    return modParams;
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
        if (data[0] != 0 || data[1] != 'P' || data[2] != 'B' || data[3] != 'P') // 0x00007F40
            return SCE_ERROR_KERNEL_UNSUPPORTED_PRX_TYPE;
        
        if (pOffset != NULL)
            *pOffset = ((u32 *)data)[9] - ((u32 *)data)[8];
            
         sceIoLseek(fd, ((u32 *)data)[8], SCE_SEEK_CUR); // 0x00007F6C
         return ((u32 *)data)[8];
        
    default:
        // 0x00007F8C
        if (data[0] != 0 || data[1] != 'P' || data[2] != 'B' || data[3] != 'P') // 0x00007FB8
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
                *pCurTable = (SceResidentLibraryEntryTable *)pCurEntry;
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

