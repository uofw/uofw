/* Copyright (C) 2011, 2012, 2013, 2014 The uOFW team
   See the file COPYING for copying permission.
*/

#include <common_imp.h>
#include <modulemgr.h>
#include <threadman_kernel.h>

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

SCE_MODULE_INFO("sceModuleManager", SCE_MODULE_KIRK_MEMLMD_LIB | SCE_MODULE_KERNEL | SCE_MODULE_ATTR_CANT_STOP | 
                                 SCE_MODULE_ATTR_EXCLUSIVE_LOAD | SCE_MODULE_ATTR_EXCLUSIVE_START, 1, 
                                 18);
SCE_MODULE_BOOTSTART("ModuleMgrInit");
SCE_MODULE_REBOOT_BEFORE("_ModuleMgrRebootBefore");
SCE_SDK_VERSION(SDK_VERSION);

SceModuleManagerCB g_ModuleManager; // 0x00009A20

// 0x00000178
static void exe_thread(SceSize args, void *argp)
{
    
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

s32 _ModuleMgrRebootBefore(s32 argc __attribute__((unused)), void *argp __attribute__((unused))) 
{
    return 0;
}