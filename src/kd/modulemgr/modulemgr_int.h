/* Copyright (C) 2011 - 2015 The uOFW team
   See the file COPYING for copying permission.
*/

#ifndef MODULEMGR_INT_H
#define MODULEMGR_INT_H

#include <common_header.h>
#include <loadcore.h>

#define THREAD_SM_LEGAL_ATTR     (SCE_KERNEL_TH_NO_FILLSTACK | SCE_KERNEL_TH_CLEAR_STACK \
                                 | SCE_KERNEL_TH_LOW_STACK | SCE_KERNEL_TH_UNK_800000 \
                                 | SCE_KERNEL_TH_USE_VFPU | SCE_KERNEL_TH_NEVERUSE_FPU)

typedef struct {
    u8 modeStart; //0 The Operation to start on, Use one of the ModuleMgrExeModes modes
    u8 modeFinish; //1 The Operation to finish on, Use one of the ModuleMgrExeModes modes
    // SceSysMemBlockType position
    u8 position; //2
    u8 access; //3
    SceUID *pResult; //4
    SceUID *pNewBlockId; // 8
    SceModule *pMod; //12
    SceLoadCoreExecFileInfo *pExecInfo; //16
    u32 apiType; //20
    SceUID fd; // 24
    s32 threadPriority; //28
    u32 threadAttr; //32
    SceUID mpIdText; // 36
    SceUID mpIdData; // 40
    SceUID threadMpIdStack; //44
    SceSize stackSize; //48
    SceUID modId; //52
    SceUID callerModId; //56
    SceSize modSize; //60
    void *fileBase; // 64
    SceSize argSize; //68
    void *argp; //72
    u32 unk76; // 76
    u32 unk80; // 80
    s32 *pStatus; // 84
    SceUID eventId; // 88
    u32 unk92; // 92
    u32 unk96; // 96
    u32 unk100; // 100
    SceUID externMemBlockIdKernel; // 104
    SceUID externMemBlockPartitionId; // 108
    SceSize externMemBlockSize; // 112
    u32 unk116; // 116
    void *blockGzip; // 120 
    u32 unk124; // 124
    char secureInstallId[SCE_SECURE_INSTALL_ID_LEN]; // 128
    SceUID externMemBlockIdUser; //144
    u32 unk148; // 148
    SceOff memBlockOffset; // 152
} SceModuleMgrParam; //size = 160

typedef struct {
    SceUID threadId; // 0
    SceUID mutexId; // 4
    SceUID eventId; // 8
    SceUID userThreadId; // 12
    u32 unk16;
    void *unk20;
    void *unk24;
    u32 unk28;
    s32(*npDrmGetModuleKeyFunction)(SceUID fd, void *, void *); // 32
    /** Module whose text segment checksum can be checked with sceKernelCheckTextSegment (user). */
    SceModule *pModule;
} SceModuleManagerCB;

enum ModuleMgrExecModes {
    CMD_LOAD_MODULE, //0
    CMD_RELOCATE_MODULE, //1
    CMD_START_MODULE, //2
    CMD_STOP_MODULE, //3
    CMD_UNLOAD_MODULE, //4
};

extern SceModuleManagerCB g_ModuleManager;

// DEBUG
//extern int c;

// UOFW: Some functions should be static but we need offsets of them to apply
// uofwinst patches and hooks. Static functions don't appear in the symbol 
// map so this workaround is used to remove static modifier on those functions
// if compiling for installer.
#ifdef INSTALLER
#define INSTALLER_NO_STATIC
#else
#define INSTALLER_NO_STATIC static
#endif

s32 ClearFreePartitionMemory(s32 partitionId);
s32 _CheckUserModulePartition(SceUID memoryPartitionId);
s32 _start_exe_thread(SceModuleMgrParam *pModParams);

void ChunkInit(void);

#endif	/* MODULEMGR_INT_H */

