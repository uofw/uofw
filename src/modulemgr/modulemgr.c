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
} SceModuleManagerCB;

SCE_MODULE_INFO("sceModuleManager", SCE_MODULE_KIRK_MEMLMD_LIB | SCE_MODULE_KERNEL | SCE_MODULE_ATTR_CANT_STOP | 
                                 SCE_MODULE_ATTR_EXCLUSIVE_LOAD | SCE_MODULE_ATTR_EXCLUSIVE_START, 1, 
                                 18);
SCE_MODULE_BOOTSTART("ModuleMgrInit");
SCE_SDK_VERSION(SDK_VERSION);

SceModuleManagerCB g_ModuleManager; // 0x00009A20

s32 ModuleMgrInit(SceSize argc __attribute__((unused)), void *argp) 
{
    g_ModuleManager.threadId = sceKernelCreateThread("SceKernelModmgrWorker", (void *)exe_thread, SCE_KERNEL_MODULE_INIT_PRIORITY, 
            0x4000, 0, NULL); // 0x00005078
	g_ModuleManager.semaId = sceKernelCreateSema("SceKernelModmgr", 0, 1, 1, NULL);

	g_ModuleManager.eventId = sceKernelCreateEventFlag("SceKernelModmgr", SCE_EVENT_WAITAND, 0, 0); // 0x000050B8
	g_ModuleManager.user_thread = -1;
	g_ModuleManager.unk2 = (SceUID *)&modMgrCB.unk2;
    
	return SCE_KERNEL_RESIDENT;
}