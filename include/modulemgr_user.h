/* Copyright (C) 2011 - 2015 The uOFW team
   See the file COPYING for copying permission.
*/

#ifndef MODULEMGR_USER_H
#define	MODULEMGR_USER_H

#include "common_header.h"
#include "modulemgr_moduleInfo.h"
#include "modulemgr_options.h"
#include "sysmem_user.h"

#define SCE_SECURE_INSTALL_ID_LEN       (16)

/* load module */
SceUID sceKernelLoadModule(const char *path, s32 flag, const SceKernelLMOption *pOption);
SceUID sceKernelLoadModuleByID(SceUID inputId, s32 flag, const SceKernelLMOption *pOption);

SceUID sceKernelLoadModuleWithBlockOffset(const char *path, SceUID blockId, SceOff offset);
SceUID sceKernelLoadModuleByIDWithBlockOffset(SceUID inputId, SceUID blockId, SceOff offset);

SceUID sceKernelLoadModuleNpDrm(const char *path, s32 flag, const SceKernelLMOption *pOption);
SceUID sceKernelLoadModuleDNAS(const char *path, const char *secureInstallId, s32 flag, 
    const SceKernelLMOption *pOption);
SceUID sceKernelLoadModuleMs(const char *path, s32 flag, const SceKernelLMOption *pOption);

/* load module buffer */
SceUID sceKernelLoadModuleBufferUsbWlan(SceSize size, void *base, s32 flag, 
    const SceKernelLMOption *pOption);
SceUID sceKernelLoadModuleBufferMs(SceSize bufSize, void *base, s32 flag, 
    const SceKernelLMOption *pOption);
SceUID sceKernelLoadModuleBufferApp(SceSize size, void *base, s32 flag, 
    const SceKernelLMOption *pOption);

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
s32 sceKernelGetModuleGPByAddress(const void *addr, u32 *pGP);
s32 sceKernelGetModuleIdList(SceUID *pModIdList, SceSize size, u32 *pIdCount);

/* Misc */
SceBool sceKernelCheckTextSegment(void);

#endif	/* MODULEMGR_USER_H */

