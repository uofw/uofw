/* Copyright (C) 2011 - 2015 The uOFW team
   See the file COPYING for copying permission.
*/

#ifndef MODULEMGR_INT_H
#define MODULEMGR_INT_H

#include <common_header.h>
#include <loadcore.h>

typedef struct {
	u8 modeStart; //0 The Operation to start on, Use one of the ModuleMgrExeModes modes
	u8 modeFinish; //1 The Operation to finish on, Use one of the ModuleMgrExeModes modes
	// SceSysMemBlockType position
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

typedef struct {
	SceUID threadId; // 0
	SceUID semaId; // 4
	SceUID eventId; // 8
	SceUID userThreadId; // 12
	u32 unk16;
	u32 unk20;
	u32 unk24;
	u32 unk28;
	s32(*npDrmGetModuleKeyFunction)(s32 fd, void *, void *); // 32
	u32 *unk36;
} SceModuleManagerCB;

enum ModuleMgrExecModes {
	CMD_LOAD_MODULE, //0
	CMD_RELOCATE_MODULE, //1
	CMD_START_MODULE, //2
	CMD_STOP_MODULE, //3
	CMD_UNLOAD_MODULE, //4
};

enum ModuleMgrApiType {
	MODULEMGR_API_LOADMODULE = 0x10,
	MODULEMGR_API_LOADMODULE_MS = 0x11,
	MODULEMGR_API_LOADMODULE_VSH = 0x20,
	MODULEMGR_API_LOADMODULE_USBWLAN = 0x30,
};

extern SceModuleManagerCB g_ModuleManager;

s32 _start_exe_thread(SceModuleMgrParam *modParams);

void ChunkInit(void);

#endif	/* MODULEMGR_INT_H */

