/* Copyright (C) 2011, 2012, 2013, 2014 The uOFW team
   See the file COPYING for copying permission.
*/

#include <common_imp.h>
#include <interruptman.h>
#include <iofilemgr_kernel.h>
#include <loadcore.h>
#include <modulemgr.h>
#include <modulemgr_options.h>
#include <sysmem_kernel.h>
#include <threadman_kernel.h>

#include "modulemgr_int.h"

#define SET_MCB_STATUS(v, m) (v = (v & 0xFFF0) | m)

typedef struct {
    SceUID threadId; // 0
    SceUID semaId; // 4
    SceUID eventId; // 8
    SceUID userThreadId; // 12
    u32 unk16;
    u32 unk20;
    u32 unk24;
    u32 unk28;
    u32 unk32;
    u32 unk36;
} SceModuleManagerCB;

typedef struct
{
	u8 modeStart; //0 The Operation to start on, Use one of the ModuleMgrExeModes modes
	u8 modeFinish; //1 The Operation to finish on, Use one of the ModuleMgrExeModes modes
	u8 position; //2
	u8 access; //3
	//u32 apitype; //4
	SceUID *returnId; //4
	SceModule *mod; //12
    /*
	SceUID mpidText; //16
	SceUID mpidData; //20
	SceUID mpidStack; //24
	SceSize stackSize; //28
	int priority; //32
	u32 attribute; //36
	SceUID modid; //40
	SceUID callerModid; //44
	SceUID fd; //48
     */
	SceLoadCoreExecFileInfo *execInfo; //16
    u32 apiType; //20
    SceUID fd; // 24
    SceUID modId; //52
    SceUID callerModId; //56
	void *file_buffer; //60
    u32 unk64;
	SceSize argSize; //68
	void *argp; //72
	u32 unk_76;
	u32 unk_80;
	int *status; //84
	u32 eventId; //88
    u32 unk100;
    u32 unk124;
    SceUID memBlockId; //144
    u32 unk148;
    SceOff memBlockOffset; // 152
} SceModuleMgrParam; //size = 160

enum ModuleMgrExecModes
{
	CMD_LOAD_MODULE, //0
	CMD_RELOCATE_MODULE, //1
	CMD_START_MODULE, //2
	CMD_STOP_MODULE, //3
	CMD_UNLOAD_MODULE, //4
};

SCE_MODULE_INFO("sceModuleManager", SCE_MODULE_KIRK_MEMLMD_LIB | SCE_MODULE_KERNEL | SCE_MODULE_ATTR_CANT_STOP | 
                                 SCE_MODULE_ATTR_EXCLUSIVE_LOAD | SCE_MODULE_ATTR_EXCLUSIVE_START, 1, 
                                 18);
SCE_MODULE_BOOTSTART("ModuleMgrInit");
SCE_MODULE_REBOOT_BEFORE("ModuleMgrRebootBefore");
SCE_MODULE_REBOOT_PHASE("ModuleMgrRebootPhase");
SCE_SDK_VERSION(SDK_VERSION);

SceModuleManagerCB g_ModuleManager; // 0x00009A20


// TODO: Reverse function sub_00000000
// 0x00000000
void sub_00000000()
{
}

// 0x000000B0
s32 _UnloadModule(SceModule *pMod)
{
    u32 modStat;
    
    modStat = (pMod->status & 0xF);
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
    
    SceModule *mod = modParams->mod;
    
    switch (modParams->modeStart) { // 0x000001D0
    case CMD_LOAD_MODULE:
        if (!mod) {
            mod = sceKernelCreateModule(); //0x0000048C
            modParams->mod = mod;

			if (!mod)
                break; // 0x000004A0
        }
        //0x000001E0
        modParams->execInfo = &execInfo;
        status = _LoadModule(modParams); //0x000001E4
            
        sceKernelChangeThreadPriority(0, SCE_KERNEL_MODULE_INIT_PRIORITY); // 0x000001F4
            
        if (status < SCE_ERROR_OK) { //0x000001FC
            modParams->returnId[0] = status; //0x00000480
            if (modParams->mod != NULL) //0x47C
                sceKernelDeleteModule(modParams->mod);
            break;
        }

        modParams->returnId[0] = modParams->mod->modId; //0x0000020C
        if (modParams->modeFinish == CMD_LOAD_MODULE) //0x00000214
            break;
    // 0x0000021C
    case CMD_RELOCATE_MODULE:
        if (mod == NULL) {
			mod = sceKernelCreateModule(); //0x00000448
			modParams->mod = mod;
                
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
        modParams->returnId[0] = modParams->mod->modId; //0x00000260        
        if (modParams->modeFinish == CMD_RELOCATE_MODULE) //0x00000268
            break;
    //0x00000270
    case CMD_START_MODULE:
        mod = sceKernelGetModuleFromUID(modParams->modId); //0x00000270
        if (mod == NULL && (mod = sceKernelFindModuleByUID(modParams->modId)) == NULL) //0x00000400
            modParams->returnId[0] = SCE_ERROR_KERNEL_UNKNOWN_MODULE; //0x00000420
        else {
            status = _StartModule(modParams, mod, modParams->argSize, modParams->argp, modParams->status); //0x00000290
            if (status == SCE_ERROR_OK)
                modParams->returnId[0] = modParams->mod->modId; //0x000003FC
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
                modParams->argp, modParams->status); //0x000002E8
            
        if (status == SCE_ERROR_OK) //0x000002F0
            modParams->returnId[0] = 0;
        else if (status == 1)
            modParams->returnId[0] = modParams->mod->modId; //0x000002FC
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
            modParams->returnId[0] = modParams->mod->modId; //0x00000348

        break;       
    }
    // 00000350
    if (modParams->eventId != 0) {
		sceKernelChangeThreadPriority(0, 1); //0x00000374
		sceKernelSetEventFlag(modParams->eventId, 1); //0x00000380
	}
    return SCE_ERROR_OK;
}

/**
 * Load a module specifying the api type
 * 
 * @param apiType The api type of the module
 * @param path A pointer to a '\0' terminated string containing the path to the module
 * @param flags Unused, pass 0
 * @param opt A pointer to a SceKernelLMOption structure, which holds various options about the way to load the module
 *
 * @return SCE_ERROR_OK on success, < 0 on error.
 * @return SCE_ERROR_KERNEL_CANNOT_BE_CALLED_FROM_INTERRUPT if function was called in an interruption.
 * @return SCE_ERROR_KERNEL_ILLEGAL_PERMISSION_CALL if function was called in a user context
 * @return SCE_ERROR_KERNEL_ILLEGAL_ADDR if the provided pointers are NULL or can't be accessed with the current rights.
 * @return SCE_ERROR_KERNEL_UNKNOWN_MODULE_FILE if the path contains a '%' (protection against formatted strings attack)
 * @return SCE_ERROR_KERNEL_ILLEGAL_SIZE if SdkVersion >= 2.80 and opt->size != sizeof(SceKernelLMOption)
 * @return One of the errors of sceIoOpen() if failed
 * @return One of the errors of sceIoIoctl() if failed
 */
// Subroutine ModuleMgrForKernel_2B7FC10D - Address 0x000004A8            
s32 sceKernelLoadModuleForLoadExecForUser(s32 apiType, const char *file, s32 flags __attribute__((unused)), 
        const SceKernelLMOption *option)
{
    s32 oldK1;
    s32 sdkVersion;
    s32 status;
    s32 ioctlCmd;
    SceUID fd;
    SceModuleMgrParam modParams;
    
    oldK1 = pspShiftK1(); //0x000004B4
    
    if (sceKernelIsIntrContext()) { //0x000004E0
        pspSetK1(oldK1);
        return SCE_ERROR_KERNEL_CANNOT_BE_CALLED_FROM_INTERRUPT;
    }
    if (pspK1IsUserMode()) { //0x000004EC
        pspSetK1(oldK1);
        return SCE_ERROR_KERNEL_ILLEGAL_PERMISSION_CALL;
    }
    
    if (file == NULL || !pspK1PtrOk(file)) { //0x000004F8 & 0x00000508
        pspSetK1(oldK1);
        return SCE_ERROR_KERNEL_ILLEGAL_ADDR;
    }
    
    // Protection against formatted string attacks, path cannot contain a '%'
    if (strchr(file, '%')) { //0x00000648
        pspSetK1(oldK1);
        return SCE_ERROR_KERNEL_UNKNOWN_MODULE_FILE;
    }
    
    if (option != NULL) { // 0x00000658
        if (!pspK1StaBufOk(option, sizeof(*option))) { //0x0000066C
            pspSetK1(oldK1);
            return SCE_ERROR_KERNEL_ILLEGAL_ADDR;
        }
        sdkVersion = sceKernelGetCompiledSdkVersion(); //0x0000067C
        // Firmware >= 2.80, updated size field
        if (sdkVersion >= 0x2080000 && option->size != sizeof(SceKernelLMOption)) { // 0x00000694 & 0x000006A8
            pspSetK1(oldK1);
            return SCE_ERROR_KERNEL_ILLEGAL_SIZE;
        }
    }
    
    fd = sceIoOpen(file, SCE_O_FGAMEDATA | SCE_O_RDONLY, SCE_STM_RUSR | SCE_STM_XUSR | SCE_STM_XGRP | SCE_STM_XOTH); //0x00000528
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
    for (int i = 0; i < sizeof(modParams) / sizeof(u32); i++) //0x000005A0
         ((u32 *)modParams)[i] = 0;
        
    modParams.apiType = apiType; // 0x000005A8
    modParams.modeFinish = CMD_RELOCATE_MODULE; // 0x000005B4
    modParams.modeStart = CMD_LOAD_MODULE; // 0x000005C0
    modParams.unk64 = 0; // 0x000005CC
    modParams.fd = fd; // 0x000005D8
    modParams.unk124 = 0; // 0x000005E0
        
    status = sceIoIoctl(fd, 0x208081, NULL, 0, NULL, 0); //0x000005DC
    if (status >= 0) //0x000005E4
        modParams.unk100 = 0x10; // 0x000005EC
        
    status = _LoadModuleByBufferID(&modParams, option); //0x000005F4
        
    sceIoClose(fd); //0x00000600
    pspSetK1(oldK1);
    return status;
}

/**
 * Load a module
 * 
 * @param path A pointer to a '\0' terminated string containing the path to the module
 * @param flags Unused, pass 0
 * @param opt A pointer to a SceKernelLMOption structure, which holds various options about the way to load the module
 *
 * @return SCE_ERROR_OK on success, < 0 on error.
 * @return SCE_ERROR_KERNEL_CANNOT_BE_CALLED_FROM_INTERRUPT if function was called in an interruption.
 * @return SCE_ERROR_KERNEL_ILLEGAL_PERMISSION_CALL if function was not called in a user context
 * @return SCE_ERROR_KERNEL_ILLEGAL_ADDR if the provided pointers are NULL or can't be accessed with the current rights.
 * @return SCE_ERROR_KERNEL_UNKNOWN_MODULE_FILE if the path contains a '%' (protection against formatted strings attack)
 * @return SCE_ERROR_KERNEL_ILLEGAL_SIZE if SdkVersion >= 2.71 and opt->size != sizeof(SceKernelLMOption)
 * @return One of the errors of sceIoOpen() if failed
 * @return One of the errors of sceIoIoctl() if failed
 */
// Subroutine sceKernelLoadModule - Address 0x000006B8 
s32 sceKernelLoadModuleForUser(const char *path, u32 flags __attribute__((unused)),
    const SceKernelLMOption *opt)
{
    s32 oldK1;
    s32 fd;
    s32 status;
    u32 SDKVersion;
    SceModuleMgrParam modParams;

    
    oldK1 = pspShiftK1(); // 0x000006C4

    // Cannot be called in an interruption
    if (sceKernelIsIntrContext()) { // 0x000006E0
        pspSetK1(oldK1);
        return SCE_ERROR_KERNEL_CANNOT_BE_CALLED_FROM_INTERRUPT;
    }

    // Only exported in user space
    if (!pspK1IsUserMode()) { // 0x000006F8
        pspSetK1(oldK1);
        return SCE_ERROR_KERNEL_ILLEGAL_PERMISSION_CALL;
    }

    if (path == NULL || !pspK1PtrOk(path)) { // 0x00000704, 0x00000714
        pspSetK1(oldK1);
        return SCE_ERROR_KERNEL_ILLEGAL_ADDR; 
    }
    // Protection against formatted string attacks, path cannot contain a '%'
    if (strchr(path, '%')) { // 0x0000081C
        pspSetK1(oldK1);
        return SCE_ERROR_KERNEL_UNKNOWN_MODULE_FILE; 
    }
    
    if (opt != NULL) { // 0x0000082C
        if (!pspK1StaBufOk(option, sizeof(SceKernelLMOption))) { //0x00000840
            pspSetK1(oldK1);
            return SCE_ERROR_KERNEL_ILLEGAL_ADDR;
        }
        SDKVersion = sceKernelGetCompiledSdkVersion(); // 0x00000850
        SDKVersion &= 0xFFFF0000; // 0x0000085C
        // Firmware >= 2.71, updated size field
        if (SDKVersion > 0x2070FFFF && opt->size != sizeof(SceKernelLMOption)) { // 0x00000868, 0x0000087C 
            pspSetK1(oldK1);
            return SCE_ERROR_KERNEL_ILLEGAL_SIZE;
        }
    }

    fd = sceIoOpen(path, SCE_O_FGAMEDATA | SCE_O_RDONLY, SCE_STM_RUSR | SCE_STM_XUSR | SCE_STM_XGRP | SCE_STM_XOTH); // 0x00000734
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

    for (int i = 0; i < sizeof(modParams) / sizeof(u32); i++) // 0x00000784
         ((u32 *)modParams)[i] = 0;

    modParams.modeFinish = CMD_RELOCATE_MODULE; // 0x00000790
    modParams.apiType = 0x10; // 0x0000079C
    modParams.modeStart = CMD_LOAD_MODULE; // 0x000007A8
    modParams.unk64 = 0; // 0x000007B4
    modParams.fd = fd; // 0x000007BC
    modParams.unk124 = 0; // 0x000007C4

    status = sceIoIoctl(fd, 0x208081, NULL, 0, NULL, 0); // 0x000007C0
    if (status >= 0) // 0x000007C8
        modParams.unk100 = 0x10; // 0x000007CC
    
    status = _LoadModuleByBufferID(&modParams, opt); // 0x000007D4
    
    sceIoClose(fd); // 0x000007E0
    pspSetK1(oldK1);
    return status;
}

// Subroutine ModuleMgrForUser_B7F46618 - Address 0x0000088C
// TODO: Rename to sceKernelLoadModuleByIDForUser() ?
s32 sceKernelLoadModuleByID(SceUID inputId, u32 flag __attribute__((unused)),
        const SceKernelLMOption *opt)
{
    s32 oldK1;
    s32 sdkVersion;
    s32 status;
    SceUID fd;
    SceModuleMgrParam modParams;
    
    oldK1 = pspShiftK1(); //0x00000898
    
    if (sceKernelIsIntrContext()) { //0x000008C0
        pspSetK1(oldK1);
        return SCE_ERROR_KERNEL_CANNOT_BE_CALLED_FROM_INTERRUPT;
    }
    if (!pspK1IsUserMode()) { //0x000008CC
        pspSetK1(oldK1);
        return SCE_ERROR_KERNEL_ILLEGAL_PERMISSION_CALL;
    }
    
    if (opt != NULL) { // 0x000008D4
        if (!pspK1StaBufOk(opt, sizeof(SceKernelLMOption))) { //0x000008E8
            pspSetK1(oldK1);
            return SCE_ERROR_KERNEL_ILLEGAL_ADDR;
        }
        sdkVersion = sceKernelGetCompiledSdkVersion(); //0x00000918
        // Firmware >= 2.80, updated size field
        if (sdkVersion >= 0x2080000 && opt->size != sizeof(SceKernelLMOption)) { // 0x00000930 & 0x00000944
            pspSetK1(oldK1);
            return SCE_ERROR_KERNEL_ILLEGAL_SIZE;
        }
    }
    
    status = sceIoValidateFd(inputId, 4); //0x00000950
    if (status < SCE_ERROR_OK) { //0x00000958
        pspSetK1(oldK1);
        return status;
    }
    
    status = sceIoIoctl(fd, 0x00208001, NULL, 0, NULL, 0); // 0x00000978
    if (status < 0) { // 0x00000984
        pspSetK1(oldK1);
        return SCE_ERROR_KERNEL_PROHIBIT_LOADMODULE_DEVICE;
    }
    
    for (int i = 0; i < sizeof(modParams) / sizeof(u32); i++) //0x0000099C
         ((u32 *)modParams)[i] = 0;
        
    modParams.apiType = 0x10; // 0x000009C8
    modParams.modeFinish = CMD_RELOCATE_MODULE; // 0x000009C4
    modParams.modeStart = CMD_LOAD_MODULE; // 0x000009CC
    modParams.unk64 = 0; // 0x000005CC
    modParams.fd = inputId; // 0x000005D8
    modParams.unk124 = 0; // 0x000009DC
        
    status = sceIoIoctl(fd, 0x208081, NULL, 0, NULL, 0); //0x000009D8
    if (status >= 0) //0x000009E0
        modParams.unk100 = 0x10; // 0x000009E4
        
    status = _LoadModuleByBufferID(&modParams, opt); //0x000009EC
        
    pspSetK1(oldK1);
    return status;
}

// Subroutine ModuleMgrForUser_E4C4211C - Address 0x000009FC
s32 sceKernelLoadModuleWithBlockOffset(const char *path, SceUID block, SceOff offset)
{
    s32 oldK1;
    s32 status;
    s32 offsetLow;
    u32 offsetHigh;
    SceUID fd;
    SceSysmemMemoryBlockInfo blkInfo;
    SceModuleMgrParam modParams;
    
    oldK1 = pspShiftK1(); //0x00000898
    
    if (sceKernelIsIntrContext()) { //0x00000A08
        pspSetK1(oldK1);
        return SCE_ERROR_KERNEL_CANNOT_BE_CALLED_FROM_INTERRUPT;
    }
    if (!pspK1IsUserMode()) { //0x00000A50
        pspSetK1(oldK1);
        return SCE_ERROR_KERNEL_ILLEGAL_PERMISSION_CALL;
    }
    
    if (path == NULL || !pspK1PtrOk(path)) { // 0x00000A5C & 0x00000A6C
        pspSetK1(oldK1);
        return SCE_ERROR_KERNEL_ILLEGAL_ADDR;
    }  
    /* Protection against formatted string attacks, path cannot contain a '%'. */
    if (strchr(path, '%')) { // 0x00000AB8
        pspSetK1(oldK1);
        return SCE_ERROR_KERNEL_UNKNOWN_MODULE_FILE; 
    }
    
    status = sceKernelQueryMemoryBlockInfo(block, &blkInfo); //0x00000ACC
    if (status < SCE_ERROR_OK) { // 0x00000AD4
        pspSetK1(oldK1);
        return status; 
    }
    if (!pspK1DynBufOk(blkInfo.addr, blkInfo.memSize)) { // 0x00000B04
        pspSetK1(oldK1);
        return SCE_ERROR_KERNEL_ILLEGAL_ADDR;
    }
    
    offsetLow = (u32)offset;
    offsetHigh = (u32)(offset >> 32);
    if (offsetHigh > 0 || ((offsetHigh == 0) && (blkInfo.memSize < offsetLow))) { //0x00000B14 & 0x00000B1C & 0x00000C24
        pspSetK1(oldK1);
        return SCE_ERROR_KERNEL_INVALID_ARGUMENT;
    }  
    // TODO: Create global ALIGNEMNT check (in common/memory.h)?
    /* Proceed only with offset being a multiple of 64. */
    if (offsetLow & 0x3F) { // 0x00000B3C
        pspSetK1(oldK1);
        return SCE_ERROR_KERNEL_INVALID_ARGUMENT;
    }
    
    fd = sceIoOpen(path, SCE_O_FGAMEDATA | SCE_O_RDONLY, SCE_STM_RUSR | SCE_STM_XUSR | SCE_STM_XGRP | SCE_STM_XOTH); //0x00000B5C
    if (fd < 0) { //0x00000B68
        pspSetK1(oldK1);
        return fd;
    }  
    status = sceIoIoctl(fd, 0x208001, NULL, 0, NULL, 0); // 0x00000B88
    if (status < 0) { // 0x00000B94
        pspSetK1(oldK1);
        return SCE_ERROR_KERNEL_PROHIBIT_LOADMODULE_DEVICE;
    }
    
    for (int i = 0; i < sizeof(modParams) / sizeof(u32); i++) //0x00000BAC
         ((u32 *)modParams)[i] = 0;
         
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
    status = _LoadModuleByBufferID(&modParams, NULL); //0x00000C08
    
    sceIoClose(fd); //0x00000C14
    pspSetK1(oldK1);
    return status;
}

// Subroutine ModuleMgrForUser_FBE27467 - Address 0x00000C34
s32 sceKernelLoadModuleByIDWithBlockOffset(SceUID inputId, SceUID block, SceOff offset)
{
    s32 oldK1;
    s32 status;
    s32 offsetLow;
    u32 offsetHigh;
    SceUID fd;
    SceSysmemMemoryBlockInfo blkInfo;
    SceModuleMgrParam modParams;
    
    oldK1 = pspShiftK1(); //0x00000C40
    
    if (sceKernelIsIntrContext()) { //0x00000C6C
        pspSetK1(oldK1);
        return SCE_ERROR_KERNEL_CANNOT_BE_CALLED_FROM_INTERRUPT;
    }
    if (!pspK1IsUserMode()) { //0x00000C84
        pspSetK1(oldK1);
        return SCE_ERROR_KERNEL_ILLEGAL_PERMISSION_CALL;
    }
    
    status = sceKernelQueryMemoryBlockInfo(block, &blkInfo); //0x00000CD0
    if (status < SCE_ERROR_OK) { // 0x00000AD4
        pspSetK1(oldK1);
        return status; 
    }
    if (!pspK1DynBufOk(blkInfo.addr, blkInfo.memSize)) { // 0x00000D00
        pspSetK1(oldK1);
        return SCE_ERROR_KERNEL_ILLEGAL_ADDR;
    }
    
    offsetLow = (u32)offset;
    offsetHigh = (u32)(offset >> 32);
    if (offsetHigh > 0 || ((offsetHigh == 0) && (blkInfo.memSize < offsetLow))) { //0x00000D10 & 0x00000D18 & 0x00000E08
        pspSetK1(oldK1);
        return SCE_ERROR_KERNEL_INVALID_ARGUMENT;
    }  
    // TODO: Create global ALIGNEMNT check (in common/memory.h)?
    /* Proceed only with offset being a multiple of 64. */
    if (offsetLow & 0x3F) { // 0x00000D38
        pspSetK1(oldK1);
        return SCE_ERROR_KERNEL_INVALID_ARGUMENT;
    }
    
    status = sceIoValidateFd(inputId, 4); //0x00000D50
    if (status < SCE_ERROR_OK) { //0x00000D58
        pspSetK1(oldK1);
        return status;
    }
     
    status = sceIoIoctl(fd, 0x208001, NULL, 0, NULL, 0); // 0x00000D78
    if (status < 0) { // 0x00000D84
        pspSetK1(oldK1);
        return SCE_ERROR_KERNEL_PROHIBIT_LOADMODULE_DEVICE;
    }
    
    for (int i = 0; i < sizeof(modParams) / sizeof(u32); i++) //0x00000D9C
         ((u32 *)modParams)[i] = 0;
         
    modParams.apiType = 0x10; // 0x00000DB4
    modParams.modeFinish = CMD_RELOCATE_MODULE; // 0x00000DA8
    modParams.modeStart = CMD_LOAD_MODULE; // 0x00000DC0
    modParams.unk64 = 0; // 0x00000DCC
    modParams.fd = inputId; // 0x00000DD4
    modParams.unk124 = 0; // 0x00000DDC
    
    status = sceIoIoctl(fd, 0x208081, NULL, 0, NULL, 0); //0x00000DD8
    if (status >= 0) //0x00000DE0
        modParams.unk100 = 0x10; // 0x00000DE4
        
    // 0x00000DEC
    modParams.memBlockId = block;
    modParams.memBlockOffset = offset;
    status = _LoadModuleByBufferID(&modParams, NULL); //0x00000DF8
    
    pspSetK1(oldK1);
    return status;
}

// TODO: Reverse function sceKernelLoadModuleDNAS
// 0x00000E18
void sceKernelLoadModuleDNAS()
{
}

// TODO: Reverse function sceKernelLoadModuleNpDrm
// 0x00001060
void sceKernelLoadModuleNpDrm()
{
}

// TODO: Reverse function sceKernelLoadModuleMs
// 0x0000128C
void sceKernelLoadModuleMs()
{
}

// TODO: Reverse function sceKernelLoadModuleBufferUsbWlan
// 0x00001478
void sceKernelLoadModuleBufferUsbWlan()
{
}

// TODO: Reverse function ModuleMgrForKernel_CE0A74A5
// 0x00001688
void ModuleMgrForKernel_CE0A74A5()
{
}

// TODO: Reverse function ModuleMgrForKernel_CAE8E169
// 0x00001858
void ModuleMgrForKernel_CAE8E169()
{
}

// TODO: Reverse function ModuleMgrForKernel_2C4F270D
// 0x00001A28
void ModuleMgrForKernel_2C4F270D()
{
}

// TODO: Reverse function ModuleMgrForKernel_853A6C16
// 0x00001BF8
void ModuleMgrForKernel_853A6C16()
{
}

// TODO: Reverse function ModuleMgrForKernel_C2A5E6CA
// 0x00001DD0
void ModuleMgrForKernel_C2A5E6CA()
{
}

// TODO: Reverse function ModuleMgrForKernel_FE61F16D
// 0x00001FD8
void ModuleMgrForKernel_FE61F16D()
{
}

// TODO: Reverse function ModuleMgrForKernel_7BD53193
// 0x000021B0
void ModuleMgrForKernel_7BD53193()
{
}

// TODO: Reverse function ModuleMgrForKernel_D60AB6CC
// 0x00002388
void ModuleMgrForKernel_D60AB6CC()
{
}

// TODO: Reverse function ModuleMgrForKernel_76F0E956
// 0x00002590
void ModuleMgrForKernel_76F0E956()
{
}

// TODO: Reverse function ModuleMgrForKernel_4E8A2C9D
// 0x00002768
void ModuleMgrForKernel_4E8A2C9D()
{
}

// TODO: Reverse function ModuleMgrForKernel_E8422026
// 0x00002978
void ModuleMgrForKernel_E8422026()
{
}

// TODO: Reverse function ModuleMgrForKernel_8DD336D4
// 0x00002B88
void ModuleMgrForKernel_8DD336D4()
{
}

// TODO: Reverse function ModuleMgrForKernel_30727524
// 0x00002D90
void ModuleMgrForKernel_30727524()
{
}

// TODO: Reverse function ModuleMgrForKernel_D5DDAB1F
// 0x00002FC8
void ModuleMgrForKernel_D5DDAB1F()
{
}

// TODO: Reverse function ModuleMgrForKernel_CBA02988
// 0x000031E4
void ModuleMgrForKernel_CBA02988()
{
}

// TODO: Reverse function ModuleMgrForKernel_939E4270
// 0x00003394
void ModuleMgrForKernel_939E4270()
{
}

// TODO: Reverse function ModuleMgrForKernel_EEC2A745
// 0x00003590
void ModuleMgrForKernel_EEC2A745()
{
}

// TODO: Reverse function ModuleMgrForKernel_D4EE2D26
// 0x00003728
void ModuleMgrForKernel_D4EE2D26()
{
}

// TODO: Reverse function ModuleMgrForKernel_F7C7FEBC
// 0x000039C0
void ModuleMgrForKernel_F7C7FEBC()
{
}

// TODO: Reverse function ModuleMgrForKernel_4493E013
// 0x00003BAC
void ModuleMgrForKernel_4493E013()
{
}

// TODO: Reverse function sceKernelStartModule
// 0x00003D98
void sceKernelStartModule()
{
}

// TODO: Reverse function sceKernelStopModule
// 0x00003F28
void sceKernelStopModule()
{
}

// TODO: Reverse function sceKernelSelfStopUnloadModule
// 0x00004110
void sceKernelSelfStopUnloadModule()
{
}

/**
 * Find the id of the module from which this function is called
 * 
 * @param modIdList A pointer which will hold the module id list
 * @param size Maximum size of the returned buffer
 * @param idCount The number of module ids in the list (can be greater than the number of returned ids)
 *
 * @return SCE_ERROR_OK on success, < 0 on error.
 * @return SCE_ERROR_KERNEL_ILLEGAL_ADDR if the provided pointers can't be accessed with the current rights or are NULL.
 */
// Subroutine sceKernelGetModuleIdList - Address 0x000041E8 - Aliases: ModuleMgrForKernel_303FAB7F
s32 sceKernelGetModuleIdList(SceUID *modIdList, SceSize size, u32 *idCount)
{
    s32 oldK1;
    s32 retVal;

    oldK1 = pspShiftK1();

    if (modIdList == NULL || idCount == NULL || !pspK1DynBufOk(modIdList, size) || !pspK1StaBufOk(idCount, 4)) { // 0x00004200, 0x00004220, 0x00004238, 0x00004244 
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
 * @param modInfo Pointer to SceKernelModuleInfo, content will be modified on success with info from the module
 * 
 * @return SCE_ERROR_OK on success, < 0 on error.
 * @return SCE_ERROR_KERNEL_CANNOT_BE_CALLED_FROM_INTERRUPT if function was called in an interruption.
 * @return SCE_ERROR_KERNEL_ILLEGAL_ADDR if the provided pointer is NULL or can't be accessed with the current rights.
 * @return SCE_ERROR_KERNEL_ILLEGAL_SIZE if SDK version >= 2.80  and modInfo->size != sizeof(SceKernelModuleInfoV1) && modInfo->size != sizeof(*modInfo)
 * @return SCE_ERROR_KERNEL_UNKNOWN_MODULE if module couldn't be found
 * @return SCE_ERROR_KERNEL_CANNOT_GET_MODULE_INFO if the module status is 0x100 or you don't have the right to access information about this module
 */
// Subroutine sceKernelQueryModuleInfo - Address 0x00004270 - Aliases: ModuleMgrForKernel_22BDBEFF
s32 sceKernelQueryModuleInfo(SceUID modId, SceKernelModuleInfo *modInfo)
{
    s32 oldK1;
    s32 SDKVersion;
    s32 intrState;
    SceModule *pMod;

    oldK1 = pspShiftK1();

    // Cannot be called from interrupt
    if (sceKernelIsIntrContext()) { // 0x0000429C
        pspSetK1(oldK1);
        return SCE_ERROR_KERNEL_CANNOT_BE_CALLED_FROM_INTERRUPT;
    }

    if (modInfo == NULL || !pspK1StaBufOk(modInfo, sizeof(*modInfo))) { // 0x000042AC, 0x000042BC
        pspSetK1(oldK1);
        return SCE_ERROR_KERNEL_ILLEGAL_ADDR;
    }

    SDKVersion = sceKernelGetCompiledSdkVersion(); // 0x000042F4
    SDKVersion &= 0xFFFF;

    if (SDKVersion > 0x2070FFFF && modInfo->size != sizeof(SceKernelModuleInfoV1) // 0x0000430C, 0x00004320
                                && modInfo->size != sizeof(*modInfo)) { // 0x00004324
        pspSetK1(oldK1);
        return SCE_ERROR_KERNEL_ILLEGAL_SIZE;
    }

    intrState = sceKernelLoadCoreLock(); // 0x00004334

    pMod = sceKernelFindModuleByUID(modId); // 0x00004340
    if (pMod == NULL) { // 0x00004348
        pspSetK1(oldK1);
        return SCE_ERROR_KERNEL_UNKNOWN_MODULE;
    }

    if (!pspK1IsUserMode()) { // 0x00004350
        // TODO: document pMod->status
        if (!(pMod->status & 0x100)) {
            pspSetK1(oldK1);
            return SCE_ERROR_KERNEL_CANNOT_GET_MODULE_INFO;
        }

        // TODO: find what 0x1E00 exactly represents
        if ((sceKernelGetUserLevel() == 2 && pMod->attribute & 0x1E00 != SCE_MODULE_USB_WLAN) // 0x00004368,0x00004370,0x000044C0
            || (sceKernelGetUserLevel() == 1 && pMod->attribute & 0x1E00 != SCE_MODULE_MS) // 0x0000437C,0x00004388,0x000044A8
            || (sceKernelGetUserLevel() == 3 && pMod->attribute & 0x1E00 != SCE_MODULE_APP) // 0x00004390,0x0000439C,0x00004490
            && (pMod->attribute & 0x1E00) // 0x000043A8
            ) {

            sceKernelLoadCoreUnlock(intrState);
            pspSetK1(oldK1);
            return SCE_ERROR_KERNEL_CANNOT_GET_MODULE_INFO;
        }
    }


    // Trying to access Kernel module information?
    if ((pMod->status & 0b1111 - 0b11) >= 0b101) { // 0x000043C0
        sceKernelLoadCoreUnlock(intrState);
        pspSetK1(oldK1);
        return SCE_ERROR_KERNEL_CANNOT_GET_MODULE_INFO;
    }

    modInfo->nsegment = pMod->nSegments; // 0x000043C8

    for (int i=0;i<SCE_KERNEL_MAX_MODULE_SEGMENT;i++) { // 0x000043F0
        modInfo->segmentAddr[i] = pMod->segmentAddr[i]; // 0x000043E4
        modInfo->segmentSize[i] = pMod->segmentSize[i]; // 0x000043F4
    }

    modInfo->entryAddr = pMod->entryAddr; // 0x00004404
    modInfo->gpValue = pMod->gpValue; // 0x0000440C
    modInfo->textAddr = pMod->textAddr; // 0x00004414
    modInfo->textSize = pMod->textSize; // 0x0000441C
    modInfo->dataSize = pMod->dataSize; // 0x00004424
    modInfo->bssSize = pMod->bssSize; // 0x00004430

    // If we have a v1 structure (less fields, size: 64)
    if (modInfo->size != sizeof(SceKernelModuleInfo)) {
        sceKernelLoadCoreUnlock(intrState);
        pspSetK1(oldK1);
        return SCE_ERROR_OK;
    }

    // If we have a v2 structure (more fields, size: 96)
    // TODO: find what 0x1E00 exactly represents
    modInfo->attribute = pMod->attribute &~ 0x0001E00;
    modInfo->version[MODULE_VERSION_MINOR] = pMod->version[MODULE_VERSION_MINOR];
    modInfo->version[MODULE_VERSION_MAJOR] = pMod->version[MODULE_VERSION_MAJOR];
    strncpy(modInfo->modName, modInfo->modName, SCE_MODULE_NAME_LEN);
    modInfo->terminal = pMod->terminal;

    sceKernelLoadCoreUnlock(intrState);
    pspSetK1(oldK1);
    return SCE_ERROR_OK;
}

/**
 * Find the id of the module from which this function is called
 * 
 * @return The module id on success, < 0 on error.
 * @return SCE_ERROR_KERNEL_CANNOT_BE_CALLED_FROM_INTERRUPT if function was called in an interruption.
 * @return SCE_ERROR_KERNEL_ILLEGAL_ADDR if the provided pointer can't be accessed with the current rights.
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
 * @return SCE_ERROR_KERNEL_ILLEGAL_ADDR if the provided pointer can't be accessed with the current rights.
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
 * @return SCE_ERROR_KERNEL_ILLEGAL_ADDR if the provided pointer can't be accessed with the current rights.
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
    if (pMod == NULL) { // 0x000046A0
        retVal = SCE_ERROR_KERNEL_UNKNOWN_MODULE; // 0x0000469C
    } else {
        retVal = SCE_ERROR_OK; // 0x000046AC
        *pGP = pMod->gpValue; // 0x000046B0
    }

    sceKernelLoadCoreUnlock(intrState); // 0x000046B4

    pspSetK1(oldK1);
    return retVal;
}

// TODO: Reverse function ModuleMgrForKernel_CC873DFA
// 0x000046E4
void ModuleMgrForKernel_CC873DFA()
{
}

// TODO: Reverse function ModuleMgrForKernel_9B7102E2
// 0x000049BC
void ModuleMgrForKernel_9B7102E2()
{
}

// TODO: Reverse function ModuleMgrForKernel_5FC3B3DA
// 0x00004B6C
void ModuleMgrForKernel_5FC3B3DA()
{
}

// 0x0000501C
s32 ModuleMgrRebootPhase(s32 argc __attribute__((unused)), void *argp __attribute__((unused)))
{
    return SCE_ERROR_OK;
}

// 0x00005024
s32 ModuleMgrRebootBefore(s32 argc __attribute__((unused)), void *argp __attribute__((unused))) 
{
    s32 status;
    
    status = sceKernelSuspendThread(g_ModuleManager.threadId); //0x00005034
    return status;
}

// 0x00005048
s32 ModuleMgrInit(SceSize argc __attribute__((unused)), void *argp __attribute__((unused))) 
{
    ChunkInit();
    
    g_ModuleManager.threadId = sceKernelCreateThread("SceKernelModmgrWorker", (SceKernelThreadEntry)exe_thread, 
            SCE_KERNEL_MODULE_INIT_PRIORITY, 0x4000, 0, NULL); // 0x00005078
    g_ModuleManager.semaId = sceKernelCreateSema("SceKernelModmgr", 0, 1, 1, NULL); // 0x0000509C

    g_ModuleManager.eventId = sceKernelCreateEventFlag("SceKernelModmgr", SCE_EVENT_WAITAND, 0, 0); // 0x000050B8
    
    g_ModuleManager.userThreadId = -1; // 0x000050DC
    g_ModuleManager.unk16 = -1; // 0x000050D0
    
    g_ModuleManager.unk20 = &g_ModuleManager.unk20; // 0x000050D8
    g_ModuleManager.unk24 = &g_ModuleManager.unk20; //0x000050F0
    g_ModuleManager.unk32 = 0; // 0x000050E0
    g_ModuleManager.unk36 = 0; // 0x000050D4
    
    return SCE_KERNEL_RESIDENT;
}

// TODO: Reverse function ModuleMgrForKernel_61E3EC69
// 0x000050FC
void ModuleMgrForKernel_61E3EC69()
{
}

// TODO: Reverse function ModuleMgrForUser_1196472E
// 0x000051AC
void ModuleMgrForUser_1196472E()
{
}

// TODO: Reverse function ModuleMgrForUser_24EC0641
// 0x0000529C
void ModuleMgrForUser_24EC0641()
{
}

// TODO: Reverse function ModuleMgrForKernel_2F3F9B6A
// 0x00005378
void ModuleMgrForKernel_2F3F9B6A()
{
}

// TODO: Reverse function ModuleMgrForKernel_C13E2DE5
// 0x00005434
void ModuleMgrForKernel_C13E2DE5()
{
}

// TODO: Reverse function ModuleMgrForKernel_C6DE0B9C
// 0x000054F0
void ModuleMgrForKernel_C6DE0B9C()
{
}

// TODO: Reverse function ModuleMgrForKernel_9236B422
// 0x000055CC
void ModuleMgrForKernel_9236B422()
{
}

// TODO: Reverse function ModuleMgrForKernel_4E62C48A
// 0x0000567C
void ModuleMgrForKernel_4E62C48A()
{
}

// TODO: Reverse function ModuleMgrForKernel_253AA17C
// 0x00005744
void ModuleMgrForKernel_253AA17C()
{
}

// TODO: Reverse function ModuleMgrForKernel_4E38EA1D
// 0x000057F4
void ModuleMgrForKernel_4E38EA1D()
{
}

// TODO: Reverse function ModuleMgrForKernel_955D6CB2
// 0x000058A4
void ModuleMgrForKernel_955D6CB2()
{
}

// TODO: Reverse function ModuleMgrForKernel_1CF0B794
// 0x000058B0
void ModuleMgrForKernel_1CF0B794()
{
}

// TODO: Reverse function ModuleMgrForKernel_5FC32087
// 0x00005978
void ModuleMgrForKernel_5FC32087()
{
}

// TODO: Reverse function ModuleMgrForKernel_E8B9D19D
// 0x00005984
void ModuleMgrForKernel_E8B9D19D()
{
}

// TODO: Reverse function sceKernelUnloadModule
// 0x00005990
void sceKernelUnloadModule()
{
}

// TODO: Reverse function sceKernelStopUnloadSelfModuleWithStatus
// 0x00005A14
void sceKernelStopUnloadSelfModuleWithStatus()
{
}

// TODO: Reverse function sceKernelStopUnloadSelfModule
// 0x00005A4C
void sceKernelStopUnloadSelfModule()
{
}

// TODO: Reverse function ModuleMgrForKernel_D86DD11B
// 0x00005A80
void ModuleMgrForKernel_D86DD11B()
{
}

// TODO: Reverse function ModuleMgrForKernel_12F99392
// 0x00005AE0
void ModuleMgrForKernel_12F99392()
{
}

// TODO: Reverse function ModuleMgrForUser_CDE1C1FE
// 0x00005B10
void ModuleMgrForUser_CDE1C1FE()
{
}

// TODO: Reverse function ModuleMgrForKernel_A40EC254
// 0x00005B6C
void ModuleMgrForKernel_A40EC254()
{
}

// TODO: Reverse function ModuleMgrForKernel_C3DDABEF
// 0x00005B7C
void ModuleMgrForKernel_C3DDABEF()
{
}

// TODO: Reverse function ModuleMgrForKernel_1CFFC5DE
// 0x00005BD0
void ModuleMgrForKernel_1CFFC5DE()
{
}

// TODO: Reverse function sub_00005C4C
// 0x00005C4C
void _LoadModule()
{
}

// TODO: Reverse function sub_00006800
// 0x00006800
void _RelocateModule()
{
}

// TODO: Reverse function sub_00006F80
// 0x00006F80
void sub_00006F80()
{
}

// TODO: Reverse function sub_00006FF4
// 0x00006FF4
void _StartModule()
{
}

// TODO: Reverse function sub_0000713C
// 0x0000713C
void _StopModule()
{
}

// TODO: Reverse function sub_000074E4
// 0x000074E4
void sub_000074E4()
{
}

// TODO: Reverse function sub_000075B4
// 0x000075B4
void _LoadModuleByBufferID()
{
}

// TODO: Reverse function sub_00007620
// 0x00007620
void sub_00007620()
{
}

// TODO: Reverse function sub_00007698
// 0x00007698
void sub_00007698()
{
}

// TODO: Reverse function sub_000076CC
// 0x000076CC
void sub_000076CC()
{
}

// TODO: Reverse function sub_000077F0
// 0x000077F0
void sub_000077F0()
{
}

// TODO: Reverse function sub_00007968
// 0x00007968
void sub_00007968()
{
}

// TODO: Reverse function sub_00007C34
// 0x00007C34
void sub_00007C34()
{
}

// TODO: Reverse function sub_00007ED8
// 0x00007ED8
void sub_00007ED8()
{
}

// TODO: Reverse function sub_00007FD0
// 0x00007FD0
void sub_00007FD0()
{
}

// TODO: Reverse function sub_00008124
// 0x00008124
void sub_00008124()
{
}

// TODO: Reverse function sub_0000844C
// 0x0000844C
void sub_0000844C()
{
}

// TODO: Reverse function sub_00008568
// 0x00008568
void sub_00008568()
{
}

// TODO: Reverse function sub_000086C0
// 0x000086C0
void sub_000086C0()
{
}
