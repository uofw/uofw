/* Copyright (C) 2011 - 2015 The uOFW team
   See the file COPYING for copying permission.
*/

#ifndef MODULEMGR_KERNEL_H
#define	MODULEMGR_KERNEL_H

#include "common_header.h"
#include "modulemgr_moduleInfo.h"
#include "modulemgr_options.h"

#define SCE_SECURE_INSTALL_ID_LEN       (16)
#define SCE_NPDRM_LICENSEE_KEY_LEN      (16)

typedef struct {
    SceSize size; //0
    s32 unk4; //4
    SceOff fileOffset; //8
    u8 keyData[SCE_NPDRM_LICENSEE_KEY_LEN]; //16  -- TODO: Confirm
} SceNpDrm;

/* load module */
SceUID sceKernelLoadModuleForLoadExecForUser(s32 apiType, const char *file, s32 flag,
    const SceKernelLMOption *pOption);
SceUID sceKernelLoadModuleForLoadExecVSHDisc(const char *path, s32 flag, 
    const SceKernelLMOption *pOption);
SceUID sceKernelLoadModuleForLoadExecVSHDiscUpdater(const char *path, s32 flag, 
    const SceKernelLMOption *pOption);
SceUID sceKernelLoadModuleForLoadExecVSHDiscDebug(const char *path, s32 flag, 
    const SceKernelLMOption *pOption);
SceUID sceKernelLoadModuleForLoadExecVSHDiscEmu(s32 apiType, const char *path, s32 flag, 
    const SceKernelLMOption *pOption);
SceUID ModuleMgrForKernel_C2A5E6CA(s32 apiType, const char *path, s32 flag, 
    const SceKernelLMOption *pOption);
SceUID sceKernelLoadModuleForLoadExecVSHMs1(s32 apiType, const char *path, s32 flag, 
    const SceKernelLMOption *pOption);
SceUID sceKernelLoadModuleForLoadExecVSHMs2(s32 apiType, const char *path, s32 flag, 
    const SceKernelLMOption *pOption);
SceUID sceKernelLoadModuleForLoadExecVSHMs3(s32 apiType, const char *path, s32 flag, 
    const SceKernelLMOption *pOption);
SceUID sceKernelLoadModuleForLoadExecVSHMs4(s32 apiType, const char *path, s32 flag, 
    const SceKernelLMOption *pOption);
SceUID sceKernelLoadModuleForLoadExecVSHMs5(s32 apiType, const char *path, s32 flag, 
    const SceKernelLMOption *pOption);
SceUID sceKernelLoadModuleForLoadExecVSHMs6(s32 apiType, const char *path, s32 flag, 
    const SceKernelLMOption *pOption);
SceUID ModuleMgrForKernel_8DD336D4(s32 apiType, const char *path, s32 flag, 
    const SceKernelLMOption *pOption);
SceUID sceKernelLoadModuleForLoadExecNpDrm(s32 apiType, const char *path, SceOff fileOffset, 
    const char *secureInstallId, s32 flag, const SceKernelLMOption *pOption);
SceUID sceKernelLoadModuleVSH(const char *path, s32 flag, const SceKernelLMOption *pOption);
SceUID sceKernelLoadModuleVSHByID(SceUID inputId, s32 flag, const SceKernelLMOption *pOption);
SceUID sceKernelLoadModuleForKernel(const char *path, s32 flag, const SceKernelLMOption *pOption);
SceUID sceKernelLoadModuleByIDForKernel(SceUID inputId, s32 flag, const SceKernelLMOption *pOption);
SceUID sceKernelLoadModuleToBlock(const char *path, SceUID blockId, SceUID *pNewBlockId, 
    s32 flag, const SceKernelLMOption *pOption);
SceUID sceKernelLoadModuleBootInitConfig(const char *path, s32 flag, 
    const SceKernelLMOption *pOption);
SceUID sceKernelLoadModuleDeci(const char *path, s32 flag, const SceKernelLMOption *pOption);

/* load module buffer */
SceUID sceKernelLoadModuleBufferMs(SceSize size, void *base, s32 flag, const SceKernelLMOption *pOption);
SceUID sceKernelLoadModuleBufferApp(SceSize size, void *base, s32 flag, const SceKernelLMOption *pOption);
SceUID sceKernelLoadModuleBufferVSH(SceSize size, void *base, s32 flag, const SceKernelLMOption *pOption);
SceUID sceKernelLoadModuleBufferForKernel(SceSize size, void *base, s32 flag, const SceKernelLMOption *pOption);
SceUID sceKernelLoadModuleBufferBootInitBtcnf(SceSize size, void *base, s32 flag, 
    const SceKernelLMOption *pOption, s32 opt);
s32 sceKernelLoadModuleBufferBootInitConfig(void);

SceUID sceKernelLoadModuleBufferForExitGame(void *base, s32 flag, const SceKernelLMOption *pOption, s32 opt);
SceUID sceKernelLoadModuleBufferForExitVSHKernel(void *base, s32 flag, const SceKernelLMOption *pOption, s32 opt);
SceUID sceKernelLoadModuleBufferForRebootKernel(void *base, s32 flag, const SceKernelLMOption *pOption, s32 opt);
SceUID sceKernelLoadModuleBufferForExitVSHVSH(void *base, s32 flag, const SceKernelLMOption *pOption, s32 opt);

SceUID sceKernelLoadModuleBufferForLoadExecBufferVSHUsbWlan(s32 apiType, void *base, s32 flag, 
    const SceKernelLMOption *pOption);
SceUID sceKernelLoadModuleBufferForLoadExecBufferVSHUsbWlanDebug(s32 apiType, void *base, s32 flag, 
    const SceKernelLMOption *pOption);

s32 sceKernelLoadModuleBootInitBtcnf(void *base, s32 flag, const SceKernelLMOption *pOption);

/* start module */
s32 sceKernelStartModule(SceUID modId, SceSize args, const void *argp, s32 *pModResult,
    const SceKernelSMOption *pOption);

/* stop module */
s32 sceKernelStopModule(SceUID modId, SceSize args, const void *argp, s32 *pModResult,
    const SceKernelSMOption *pOption);

/* unload module */
SceUID sceKernelUnloadModule(SceUID modId);

s32 sceKernelStopUnloadSelfModuleWithStatus(s32 exitStatus, SceSize args, void *argp, 
    s32 *pModResult, const SceKernelSMOption *pOption);
s32 sceKernelStopUnloadSelfModule(SceSize args, void *argp, s32 *pModResult, 
    const SceKernelSMOption *pOption);

s32 sceKernelSelfStopUnloadModule(s32 exitStatus, SceSize args, void *argp); /* backward compatibility. */

/* obtain module information */
s32 sceKernelQueryModuleInfo(SceUID modId, SceKernelModuleInfo *pModInfo);
SceUID sceKernelGetModuleId(void);
SceUID sceKernelGetModuleIdByAddress(const void *addr);
SceUID sceKernelSearchModuleByName(const char *name);
SceUID sceKernelSearchModuleByAddress(const void *addr);

s32 sceKernelGetModuleIdList(SceUID *pModIdList, SceSize size, u32 *pIdCount);

/* PSP reboot phase functions */
s32 sceKernelRebootBeforeForUser(void *arg);
s32 sceKernelRebootPhaseForKernel(s32 arg1, void *argp, s32 arg3, s32 arg4);
s32 sceKernelRebootBeforeForKernel(void *argp, s32 arg2, s32 arg3, s32 arg4);

/* NP-DRM key functions */
s32 sceKernelSetNpDrmGetModuleKeyFunction(s32(*function)(SceUID fd, void *, void *));
s32 sceKernelNpDrmGetModuleKey(SceUID fd, void *arg2, void *arg3);

/* Misc */
s32 sceKernelModuleMgrMode(s32 mode);

#endif	/* MODULEMGR_KERNEL_H */

