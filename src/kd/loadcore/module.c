/* Copyright (C) 2011, 2012, 2013 The uOFW team
   See the file COPYING for copying permission.
*/

/*
 * uOFW/trunk/src/loadcore/module.c
 * 
 * Module - an interface for the Module Manager.
 * 
 * This interface provides a couple of backbone functions
 * for the PSP's module manager (creating, assigning, 
 * registering, releasing a module) and some supportive 
 * functions like finding modules in memory.  
 * 
 * Basic tasks:
 *    1) Create the unique UID type of "module" objects.  
 *       The UID type is created only once and then used to create 
 *       the UIDs of every created module. 
 *    2) Update the UIDs of modules during creating, assigning,
 *       registering a module.  Updating the UID name is done,
 *       because the purpose and the name of a module is not known
 *       when it is created and thus a temporary UID name is used.
 *       Assigning a meaningful name to each UID makes it easier
 *       to keep track of them.
 *    3) Maintain a linked-list of currently loaded modules to 
 *       enable easy searching for a loaded module by giving its
 *       name, its UID or an address within it.
 * 
 */

#include <sysmem_utils_kernel.h>
#include <modulemgr_nids.h>
#include "clibUtils.h"
#include "module.h"

#define UID_MODULE_DO_INITIALIZE                (0xD310D2D9)
#define UID_MODULE_DO_DELETE                    (0x87089863)

#define MODULE_TEMPORARY_UID_NAME               "SceModuleTmp"
#define MODULE_UID_TYPE_NAME                    "SceModule"

#define MODULE_LIST_MEM_BLOCK_NAME              "SceModmgrModuleList"

#define MODULE_SYSTEM_BLOCK_NAME                "sceSystemBlock"
#define MODULE_SYSTEM_MODULE_NAME               "sceSystemModule"

#define NUM_MD5_HASHES							(6)

#define BLACKLIST_UNSUPPORTED_DEVKIT_VERSION    (0x506FFFF)

/* ifhandle.prx' module name prior to SDK version 3.70. */
#define MODULE_IFHANDLE_NAME_OLD                "sceNetIfhandle_Service"
/* ifhandle.prx' module name since SDK version 3.70. */
#define MODULE_IFHANDLE_NAME_NEW                "sceNetInterface_Service"

static SceUID module_do_initialize(SceSysmemUidCB *cb, SceSysmemUidCB *uidWithFunc, s32 funcId, 
                                   va_list ap);
static SceUID module_do_delete(SceSysmemUidCB *cb, SceSysmemUidCB *uidWithFunc, s32 funcId, 
                               va_list ap);
static s32 CheckDevkitVersion(SceModuleInfo *modInfo, u32 *fileDevKitVersion);
static void updateUIDName(SceModule *mod);

//0x00008020
/*
 * This structure probably contains the first function being
 * called on the creation process of a module (module_do_initialize)
 * and the last function called in the delete process of a
 * module.
 */
static SceSysmemUidLookupFunc ModuleFuncs[] = { 
    { .id = UID_MODULE_DO_INITIALIZE, .func = module_do_initialize },
    { .id = UID_MODULE_DO_DELETE,     .func = module_do_delete },
    { .id = 0,                        .func = NULL }
};

//0x00008038
const u32 g_hashData[NUM_MD5_HASHES * sizeof(u32)] = {
	0x2CDEEB73, 0xF7E529B9, 0xF85FB599, 0xA8B6B2D3,
	0xA0811E0E, 0xF91FE045, 0x51339FF7, 0xBF9F32B2,
	0x89F0A05E, 0xF413FE28, 0x1F24A87E, 0x7C1ADFE2,
	0x3D3203F8, 0x4C6E9246, 0x4DCA0BCB, 0x71A9322A,
	0x7EAEEDE2, 0xC7BACA41, 0xC38D13D2, 0xE54A1434,
	0x0FC5364D, 0xF2EA99AA, 0x7A418AA1, 0xFEC29EA1,
};

static SceSysmemUidCB *g_ModuleType; //0x00008404

//Subroutine LoadCoreForKernel_2C44F793 - Address 0x00006844
/*
 * A new SceModule structure is allocated by creating an UID based
 * on g_ModuleType (the UID type for modules) and is
 * set to be the UID of the newly created module.
 */
SceModule *sceKernelCreateModule(void)
{
    s32 status;
    s32 intrState;    
    SceModule *mod;
    SceSysmemUidCB *cb = NULL;
    
    intrState = loadCoreCpuSuspendIntr();
    
    /*
     * The name of the created UID is temporary; once the module
     * is in the process of being loaded, its UID name is updated
     * based on the module's name and its purpose. 
     */
    status = sceKernelCreateUID(g_ModuleType, MODULE_TEMPORARY_UID_NAME, 0, &cb); //0x00006874
    if (status < 0) { //0x0000687C
        loadCoreCpuResumeIntr(intrState);
        return NULL;
    }
    
    mod = UID_CB_TO_DATA(cb, g_ModuleType, SceModule);
    if (mod == NULL) { //0x00006898
        loadCoreCpuResumeIntr(intrState);
        return NULL;
    }
    //0x000068D0 - 0x0000689C
    mod->modId = cb->uid; //0x000068A0
    mod->moduleStartThreadPriority = SCE_KERNEL_VALUE_UNITIALIZED;
    mod->moduleStartThreadStacksize = SCE_KERNEL_VALUE_UNITIALIZED;
    mod->moduleStartThreadAttr = SCE_KERNEL_VALUE_UNITIALIZED;
    mod->moduleStopThreadPriority = SCE_KERNEL_VALUE_UNITIALIZED;
    mod->moduleStopThreadStacksize = SCE_KERNEL_VALUE_UNITIALIZED;
    mod->moduleStopThreadAttr = SCE_KERNEL_VALUE_UNITIALIZED;
    mod->moduleRebootBeforeThreadPriority = SCE_KERNEL_VALUE_UNITIALIZED;
    mod->moduleRebootBeforeThreadStacksize = SCE_KERNEL_VALUE_UNITIALIZED;
    mod->moduleRebootBeforeThreadAttr = SCE_KERNEL_VALUE_UNITIALIZED;   
    mod->countRegVal = pspCop0StateGet(COP0_STATE_COUNT);
    
    loadCoreCpuResumeIntr(intrState); //0x000068D4
    return mod; 
}

//Subroutine LoadCoreForKernel_F3DD4808 - Address 0x000068F8
/*
 * Assign a module which is a necessary step after a module
 * is created.  We check if the specified module can be loaded and
 * if everything checked is correct, its moduleInfo section is
 * copied with the moduleInfo section content provided by 
 * execFileInfo.
 */
s32 sceKernelAssignModule(SceModule *mod, SceLoadCoreExecFileInfo *execFileInfo)
{
    SceModule *tmpMod = NULL;
    SceModule *foundMod = NULL;
    SceUID foundUid = 0;
    s32 status;
    u8 digest[16];
    u32 modDevKitVersion;
    u32 i;
    u32 j;
   
    if (mod == NULL) //0x0000692C
        return SCE_ERROR_KERNEL_ERROR;
    
    modDevKitVersion = 0;
    mod->next = NULL; //0x00006934
    //0x0000695C - 0x0000698C
    /*
     * Here, we check if a module with the same name as the 
     * to-be-assigned module is already loaded into memory.  If there
     * is more than one module with the same name, search for the 
     * latest loaded module among them.  Once we are done with that 
     * search, we check if the loaded module has the attribute 
     * SCE_MODULE_ATTR_EXCLUSIVE_LOAD, if this is the case, we cannot
     * assign the recently created module.
     */
    for (tmpMod = g_loadCore.registeredMods; tmpMod; tmpMod = tmpMod->next) {
         if (strcmp(execFileInfo->moduleInfo->modName, tmpMod->modName) == 0) { //0x0000695C
             if (tmpMod->modId > foundUid) { //0x00006974
                 foundUid = mod->modId; //0x0000697C
                 foundMod = mod; //0x00006980
             }
        }
    }
    if (foundMod && foundMod->attribute == SCE_MODULE_ATTR_EXCLUSIVE_LOAD) //0x00006990 & 0x000069A0
        return SCE_ERROR_KERNEL_EXCLUSIVE_LOAD;
    
    status = CheckDevkitVersion(execFileInfo->moduleInfo, &modDevKitVersion); //0x000069AC
    if (status < SCE_ERROR_OK) //0x000069B8
        return SCE_ERROR_KERNEL_UNSUPPORTED_PRX_TYPE;
    
    mod->attribute = (execFileInfo->moduleInfo->modAttribute & ~SCE_MODULE_PRIVILEGE_LEVELS) | execFileInfo->modInfoAttribute; //0x000069CC & 0x000069E0
    if ((mod->attribute & SCE_MODULE_PRIVILEGE_LEVELS) == SCE_MODULE_KERNEL)
        mod->status &= ~SCE_MODULE_USER_MODULE; //0x00006BB4
    else
        mod->status |= SCE_MODULE_USER_MODULE; //0x000069E8
    
    mod->version[MODULE_VERSION_MINOR] = execFileInfo->moduleInfo->modVersion[MODULE_VERSION_MINOR]; //0x000069EC & 0x000069F8
    mod->version[MODULE_VERSION_MAJOR] = execFileInfo->moduleInfo->modVersion[MODULE_VERSION_MAJOR]; //0x00006A00 & 0x00006A08
    
    if (strcmp(execFileInfo->moduleInfo->modName, MODULE_IFHANDLE_NAME_OLD) == 0 || 
      strcmp(execFileInfo->moduleInfo->modName, MODULE_IFHANDLE_NAME_NEW) == 0) //0x00006A0C & 0x00006A20
        return SCE_ERROR_KERNEL_UNSUPPORTED_PRX_TYPE;
    
	/* Compute MD5-hash of the name of the module to be loaded and check it against a MD5-hash name blacklist. */
    sceKernelUtilsMd5Digest((u8 *)execFileInfo->moduleInfo->modName, strlen(execFileInfo->moduleInfo->modName), digest); //0x00006A70
	//0x00006A78 - 0x00006ACC
	for (i = 0; i < NUM_MD5_HASHES; i++) {
		u8 *curMD5Hash = (u8 *)&g_hashData[i * (16 / sizeof g_hashData[0])];
		//0x00006A98 - 0x00006AB8
		for (j = 0; j < sizeof digest; j++) {
			if (digest[j] != curMD5Hash[j]) //0x00006AA8
				break;
		}
		/* MD5-hash of module name matches a hash entry in the module blacklist. */
		if (j == 16) { //0x00006AC0
			if ((modDevKitVersion & 0xFFFFFF00) <= BLACKLIST_UNSUPPORTED_DEVKIT_VERSION) //0x00006BA4
				return SCE_ERROR_KERNEL_UNSUPPORTED_PRX_TYPE;
			break;
		}
	}
    /* copy over the moduleInfo data of the provided execFileInfo. */
    strncpy(mod->modName, execFileInfo->moduleInfo->modName, SCE_MODULE_NAME_LEN); //0x00006ADC
    mod->terminal = 0; //0x00006AE4
    mod->entryAddr =  execFileInfo->entryAddr; //0x00006AEC & 0x00006B08
    mod->entTop = execFileInfo->moduleInfo->entTop; //0x00006B1C        
    mod->textSize = execFileInfo->textSize; //0x00006B20
    mod->stubTop = execFileInfo->moduleInfo->stubTop; //0x00006B28
    mod->dataSize = execFileInfo->dataSize; //0x00006B2C
    mod->gpValue = (u32)execFileInfo->moduleInfo->gpValue; //0x00006B34
    mod->bssSize = execFileInfo->bssSize; //0x00006B38
    mod->entSize = execFileInfo->moduleInfo->entEnd - execFileInfo->moduleInfo->entTop; //0x00006B3C
    mod->stubSize = execFileInfo->moduleInfo->stubEnd - execFileInfo->moduleInfo->stubTop; //0x00006B40
    mod->nSegments = execFileInfo->numSegments; //0x00006B48
    
    for (i = 0; i < SCE_KERNEL_MAX_MODULE_SEGMENT; i++) {
    	mod->segmentAddr[i] = execFileInfo->segmentAddr[i]; //0x00006B60
        mod->segmentSize[i] = execFileInfo->segmentSize[i]; //0x00006B6C
        mod->segmentAlign[i] = execFileInfo->segmentAlign[i]; //0x00006B78
    }
    updateUIDName(mod); //0x00006B7C
    
    return SCE_ERROR_OK;
}

//Subroutine LoadCoreForKernel_B17F5075 - Address 0x00006BC0
/*
 * Find the specified module in the linked-list of loaded modules and
 * unlink it from that list.
 */
s32 sceKernelReleaseModule(SceModule *mod)
{
    u32 intrState;
    SceModule *curMod;
    void *prevMod;
   
    if (mod == NULL) 
        return SCE_ERROR_KERNEL_INVALID_ARGUMENT;
       
    intrState = loadCoreCpuSuspendIntr(); //0x00006BDC
    
    //0x00006BF0 - 0x00006C0C
    prevMod = &g_loadCore.registeredMods;
    for (curMod = g_loadCore.registeredMods; curMod; curMod = ((SceModule *)prevMod)->next) {
        if (curMod == mod) { //0x00006C00        
            ((SceModule *)prevMod)->next = curMod->next; //0x00006C34
             
            if (curMod->next == NULL) 
                g_loadCore.lastRegMod = prevMod;
                
            loadCoreCpuResumeIntr(intrState);
            g_loadCore.regModCount--;
            return SCE_ERROR_OK;
        }
        prevMod = curMod; //0x00006C10
    }
    loadCoreCpuResumeIntr(intrState);
    return SCE_ERROR_KERNEL_ERROR;
}

//Subroutine LoadCoreForKernel_37E6F41B - Address 0x00006C58
s32 sceKernelGetModuleIdListForKernel(SceUID *modIdList, u32 size, u32 *modCount, u32 userModsOnly)
{
    SceModule *curMod = NULL;   
    u32 modIdListElements;
    u32 intrState;
    u32 i;
    
    if (modIdList == NULL || modCount == NULL)
        return SCE_ERROR_KERNEL_ILLEGAL_ADDR; //0x00006C8C
    
    intrState = loadCoreCpuSuspendIntr();
    
    modIdListElements = size / sizeof size; //0x00006CC0
    for (i = 0, curMod = g_loadCore.registeredMods; curMod; curMod = curMod->next) {
         if ((!userModsOnly || curMod->status & SCE_MODULE_USER_MODULE) && i < modIdListElements) //0x00006CC4 & 0x00006D18
             modIdList[i++] = curMod->modId; //0x00006D24
    }
    *modCount = i;
    
    loadCoreCpuResumeIntr(intrState);
    return SCE_ERROR_OK;
}

//sub_00006D30
/*
 * Create the UID type for modules.  Every created UID via 
 * sceKernelCreateModule() is stored in the linked-list 
 * belonging to this UID type, ensuring that module objects have
 * their unique UID list.
 * 
 */
s32 ModuleServiceInit(void)
{
    return sceKernelCreateUIDtype(MODULE_UID_TYPE_NAME, sizeof(SceModule), ModuleFuncs, 0, &g_ModuleType);
}

//Subroutine LoadCoreForKernel_CD26E0CA - Address 0x00006D68
SceModule *sceKernelGetModuleFromUID(SceUID uid)
{
    u32 intrState; 
    u32 intrState2;   
    SceModule *mod = NULL;
    SceSysmemUidCB *block = NULL;
    
    intrState = loadCoreCpuSuspendIntr(); //0x00006D80
    
    if (sceKernelGetUIDcontrolBlockWithType(uid, g_ModuleType, &block) < SCE_ERROR_OK) { //0x00006D94 & 0x00006D9C
        intrState2 = loadCoreCpuSuspendIntr(); //0x00006DD8
        
        for (mod = g_loadCore.registeredMods; mod; mod = mod->next) { //0x00006DF4 - 0x00006E10
             if (mod->modId == uid || mod->secId == uid) //0x00006DF4 & 0x00006E00
                 break;
        }
        loadCoreCpuResumeIntr(intrState2); //0x00006E14
    }
    else {
        mod = UID_CB_TO_DATA(block, g_ModuleType, SceModule);
    }
    
    loadCoreCpuResumeIntr(intrState); //0x00006DB4
    return mod;
}

//Subroutine LoadCoreForKernel_001B57BB - Address 0x00006E38
s32 sceKernelDeleteModule(SceModule *mod)
{
    s32 status;
    u32 intrState;

    intrState = loadCoreCpuSuspendIntr(); //0x00006E48
    
    status = sceKernelDeleteUID(mod->modId); //0x00006E54
    
    loadCoreCpuResumeIntr(intrState); //0x00006E60
    return status;
}

//Subroutine LoadCoreForKernel_84D5C971 - Address 0x00006E80
SceModule *sceKernelCreateAssignModule(SceLoadCoreExecFileInfo *execFileInfo)
{
    u32 intrState;
    u32 intrState2; 
    SceModule *mod = NULL;    
          
    intrState = loadCoreCpuSuspendIntr(); //0x00006E94
    
    mod = sceKernelCreateModule(); //0x00006E9C
    if (mod != NULL) { //0x00006EAC
        if (sceKernelAssignModule(mod, execFileInfo) < SCE_ERROR_OK) { //0x00006EB4 - 0x00006EC0            
            intrState2 = loadCoreCpuSuspendIntr(); //0x00006EE8
            
            sceKernelDeleteUID(mod->modId); //0x00006EF8
            mod = NULL; //0x00006EFC
            
            loadCoreCpuResumeIntr(intrState2); //0x00006F00
        }
    }   
    loadCoreCpuResumeIntr(intrState); //0x00006EC4
    return mod; //0x00006ECC
}

//Subroutine LoadCoreForKernel_BF2E388C - Address 0x00006F10
s32 sceKernelRegisterModule(SceModule *mod)
{
    u32 intrState;
    
    intrState = loadCoreCpuSuspendIntr(); //0x00006F20    
    
    updateUIDName(mod); //0x00006F2C   
    mod->secId = g_loadCore.secModId++; //0x00006F50
    
    if (g_loadCore.lastRegMod != NULL)
        g_loadCore.lastRegMod->next = mod;
    else
        g_loadCore.registeredMods = mod;
    
    mod->next = NULL; //0x00006F60
    g_loadCore.lastRegMod = mod; //0x00006F6C    
    g_loadCore.regModCount++; //0x00006F74
    
    loadCoreCpuResumeIntr(intrState);
    return SCE_ERROR_OK;
}

//Subroutine LoadCoreForKernel_F6B1BF0F - Address 0x00006F98
SceModule *sceKernelFindModuleByName(const char *name)
{
    SceModule *foundMod = NULL;
    SceModule *curMod = NULL;
    SceUID modId;
    u32 intrState;
    
    modId = 0;
    intrState = loadCoreCpuSuspendIntr();
    
    for (curMod = g_loadCore.registeredMods; curMod; curMod = curMod->next) { //0x6FCC
         if (strcmp(name, curMod->modName) == 0 && curMod->modId > modId) { //0x6FD8 - 0x6FE0
             modId = curMod->modId;
             foundMod = curMod;
         }
    }
    loadCoreCpuResumeIntr(intrState);
    return foundMod;
}

//Subroutine LoadCoreForKernel_BC99C625 - Address 0x00007038
SceModule *sceKernelFindModuleByAddress(u32 addr)
{
    SceModule *curMod = NULL;
    
    for (curMod = g_loadCore.registeredMods; curMod; curMod = curMod->next) {
         if (addr >= curMod->textAddr && addr < curMod->textAddr + curMod->textSize + curMod->dataSize + curMod->bssSize) //0x7050
             break;
    }
    return curMod;
}

//Subroutine LoadCoreForKernel_410084F9 - Address 0x00007094
s32 sceKernelGetModuleGPByAddressForKernel(u32 addr)
{
    SceModule *curMod = NULL;
    
    for (curMod = g_loadCore.registeredMods; curMod; curMod = curMod->next) {
         if (addr >= curMod->textAddr && addr < curMod->textAddr + curMod->textSize + curMod->dataSize + curMod->bssSize)
             break;
    }
    if (curMod != NULL) 
        return curMod->gpValue;
    
    return 0;
}

//Subroutine LoadCoreForKernel_40972E6E - Address 0x000070FC
SceModule *sceKernelFindModuleByUID(SceUID uid)
{   
    u32 intrState;
    SceModule *curMod = NULL;
    
    intrState = loadCoreCpuSuspendIntr(); //0x0000710C
    
    for (curMod = g_loadCore.registeredMods; curMod; curMod = curMod->next) {
         if (curMod->modId == uid || curMod->secId == uid) 
             break;
    }
    loadCoreCpuResumeIntr(intrState);
    return curMod;
}
    

//Subroutine LoadCoreForKernel_3FE631F0 - Address 0x00007178
SceUID sceKernelGetModuleListWithAlloc(u32 *modCount)
{    
    u32 i;
    u32 intrState;
    SceUID blockId;
    SceUID *modIdList;
    SceModule *curMod;
    
    if (modCount == NULL) 
        return SCE_ERROR_KERNEL_ILLEGAL_ADDR; //0x00007194
    
    intrState = loadCoreCpuSuspendIntr(); //0x0000719C
    
    *modCount = g_loadCore.regModCount;
    /*
     * Allocate an array able to hold g_loadCore.regModCount elements
     * (the number of currently loaded modules) and fill that
     * array with the UIDs of the loaded modules.
     */
    blockId = sceKernelAllocPartitionMemory(SCE_KERNEL_PRIMARY_KERNEL_PARTITION, MODULE_LIST_MEM_BLOCK_NAME, 
                                            SCE_KERNEL_SMEM_High, g_loadCore.regModCount << 2, 0); //0x000071CC
    if (blockId > 0) { //0x000071CC
        modIdList = sceKernelGetBlockHeadAddr(blockId);
        for (i = 0, curMod = g_loadCore.registeredMods; curMod; curMod = curMod->next, i++)
             modIdList[i] = curMod->modId; //0x000071F8
    }
    loadCoreCpuResumeIntr(intrState);//0x00007204
    return blockId;
}

//sub_00007228
/*
 * Register the created UIDControlBlock for a new module in the
 * linked-list of the UID type of "module" objects?
 */
static SceUID module_do_initialize(SceSysmemUidCB *cb, SceSysmemUidCB *uidWithFunc, s32 funcId, 
                                   va_list ap)
{
    SceModule *mod;
    
    sceKernelCallUIDObjCommonFunction(cb, uidWithFunc, funcId, ap); //0x00007234
    
    mod = UID_CB_TO_DATA(cb, g_ModuleType, SceModule);
    
    mod->modId = cb->uid; //0x0000725C
    mod->entryAddr = SCE_KERNEL_VALUE_UNITIALIZED; //0x00007260
    mod->version[MODULE_VERSION_MINOR] = 0; //0x00007264
    mod->version[MODULE_VERSION_MAJOR] = 0; //0x00007268
    mod->terminal = 0; //0x0000726C
    mod->moduleStart = (SceKernelThreadEntry)SCE_KERNEL_VALUE_UNITIALIZED; //0x00007270
    mod->moduleStop = (SceKernelThreadEntry)SCE_KERNEL_VALUE_UNITIALIZED; //0x00007274
    mod->moduleBootstart = (SceKernelThreadEntry)SCE_KERNEL_VALUE_UNITIALIZED; //0x00007278
    mod->moduleRebootBefore = (SceKernelRebootBeforeForKernel)SCE_KERNEL_VALUE_UNITIALIZED; //0x0000727C
    mod->moduleRebootPhase = (SceKernelRebootPhaseForKernel)SCE_KERNEL_VALUE_UNITIALIZED; //0x00007280
    mod->textSize = 0; //0x00007284
    mod->dataSize = 0; //0x00007288
    mod->bssSize = 0; //0x0000728C
    mod->segmentChecksum = 0; //0x00007290
    mod->textSegmentChecksum = 0; //0x00007294
    mod->unk224 = 0; //0x00007298
    mod->status = 0; //0x000072A4
    mod->next = NULL; //0x000072A8
    mod->attribute = 0; //0x000072AC
    mod->entTop = (void *)SCE_KERNEL_VALUE_UNITIALIZED; //0x000072B0
    mod->stubTop = (void *)SCE_KERNEL_VALUE_UNITIALIZED; //0x000072B4
    
    return cb->uid;
}

//sub_000072C0
/*
 * Unlink a UIDControlBlock belonging to an unloaded module
 * from the linked-list of the UID type of "module" objects?
 */
static SceUID module_do_delete(SceSysmemUidCB *cb, SceSysmemUidCB *uidWithFunc, s32 funcId, 
                               va_list ap)
{
    sceKernelCallUIDObjCommonFunction(cb, uidWithFunc, funcId, ap);
    return cb->uid;
}

//sub_000072E8
/*
 * We check if a module's resident libraries have a legal SDK
 * version.  A library's SDK version is legal when it is 
 * not HIGHER than the global SDK version of the PSP device.  
 * If a resident library does not have a legal SDK version, 
 * the module won't be loaded.
 */
static s32 CheckDevkitVersion(SceModuleInfo *modInfo, u32 *fileDevKitVersion)
{
    u32 i;
    u32 ver;
    SceResidentLibraryEntryTable *curEntryTable;  
    
    //0x000072F4 - 0x00007320
    for (curEntryTable = modInfo->entTop; (void *)curEntryTable < modInfo->entEnd; 
       curEntryTable = (void *)curEntryTable + curEntryTable->len * sizeof(u32)) {
         if ((curEntryTable->attribute & SCE_LIB_IS_SYSLIB) == SCE_LIB_IS_SYSLIB) { //0x00007300
             //0x0000732C - 0x00007370
             for (i = 0; i < curEntryTable->vStubCount; i++) {
                  if (curEntryTable->entryTable[curEntryTable->stubCount + i] == NID_MODULE_SDK_VERSION) { //0x0000735C
                      ver = *(u32 *)curEntryTable->entryTable[curEntryTable->stubCount * 2 + curEntryTable->stubCount + i];
                      if (ver > SDK_VERSION) //0x000073A0
                          return SCE_ERROR_KERNEL_UNSUPPORTED_PRX_TYPE;
                      *fileDevKitVersion = ver;
                      return SCE_ERROR_OK;
                  }
                  break;
             } 
         }
    }
    return SCE_ERROR_OK;
}

//sub_000073B8
/*
 * Once a module is in the process of being loaded, its UID name
 * is updated in order to make identifying the module easier.
 * Recall that creating a module only sets a temporary UID name,
 * as we didn't know the module's name or is purpose back then.
 */
static void updateUIDName(SceModule *mod)
{
    if ((mod->status & 0x2000) == 0 && mod->moduleBlockId > 0) { //0x000073D0 & 0x000073DC
        if ((mod->attribute & (SCE_MODULE_KIRK_MEMLMD_LIB | SCE_MODULE_KIRK_SEMAPHORE_LIB)) == 0) {
            sceKernelRenameUID(mod->moduleBlockId, (const char *)mod->modName);
            sceKernelRenameUID(mod->modId, (const char *)mod->modName);
        } 
        else {
            /*
             * The module uses KIRK libraries and thus is classified as a system
             * module.
             */
            sceKernelRenameUID(mod->moduleBlockId, MODULE_SYSTEM_BLOCK_NAME);
            sceKernelRenameUID(mod->modId, MODULE_SYSTEM_MODULE_NAME);
        }
    }
}
