/* Copyright (C) 2011, 2012, 2013 The uOFW team
   See the file COPYING for copying permission.
*/

#ifndef MODULEMGR_KERNEL_H
#define	MODULEMGR_KERNEL_H

#include "common_header.h"
#include "modulemgr_options.h"

s32 sceKernelLoadModuleForLoadExecForUser(s32 apiType, const char *file, s32 flags, SceKernelLMOption *option);
s32 sceKernelLoadModuleForLoadExecNpDrm(s32 apiType, const char *file, SceOff fileOffset, s32 buf[4], s32 flags, SceKernelLMOption *option);
s32 sceKernelLoadModuleForLoadExecVSHDisc(const char *file, s32 flags, SceKernelLMOption *option);
s32 sceKernelLoadModuleForLoadExecVSHDiscUpdater(const char *file, s32 flags, SceKernelLMOption *option);
s32 sceKernelLoadModuleForLoadExecVSHDiscDebug(const char *file, s32 flags, SceKernelLMOption *option);
s32 sceKernelLoadModuleForLoadExecVSHDiscEmu(s32 apiType, const char *file, s32 flags, SceKernelLMOption *option);
s32 ModuleMgrForKernel_C2A5E6CA(s32 apiType, const char *file, s32 flags, SceKernelLMOption *option);
s32 sceKernelLoadModuleForLoadExecVSHMs1(s32 apiType, const char *file, s32 flags, SceKernelLMOption *option);
s32 sceKernelLoadModuleForLoadExecVSHMs2(s32 apiType, const char *file, s32 flags, SceKernelLMOption *option);
s32 sceKernelLoadModuleForLoadExecVSHMs3(s32 apiType, const char *file, s32 flags, SceKernelLMOption *option);
s32 sceKernelLoadModuleForLoadExecVSHMs4(s32 apiType, const char *file, s32 flags, SceKernelLMOption *option);
s32 sceKernelLoadModuleForLoadExecVSHMs5(s32 apiType, const char *file, s32 flags, SceKernelLMOption *option);
s32 sceKernelLoadModuleForLoadExecVSHMs6(s32 apiType, const char *file, s32 flags, SceKernelLMOption *option);
s32 ModuleMgrForKernel_8DD336D4(s32 apiType, const char *file, s32 flags, SceKernelLMOption *option);

s32 sceKernelLoadModuleBufferForLoadExecBufferVSHUsbWlan(s32 apiType, u32 *modBuf, s32 flags, SceKernelLMOption *option);
s32 sceKernelLoadModuleBufferForLoadExecBufferVSHUsbWlanDebug(s32 apiType, u32 *modBuf, s32 flags, SceKernelLMOption *option);
s32 sceKernelLoadModuleBufferForExitVSHKernel(u32 *modBuf, s32 flags, SceKernelLMOption *option, int);
s32 sceKernelLoadModuleBufferForExitGame(u32 *modBuf, s32 flags, SceKernelLMOption *option, int);
s32 sceKernelLoadModuleBufferForExitVSHVSH(u32 *modBuf, s32 flags, SceKernelLMOption *option, int);
s32 sceKernelLoadModuleBufferForRebootKernel(u32 *modBuf, s32 flags, SceKernelLMOption *option, int);

s32 sceKernelLoadModuleBootInitBtcnf(u32 *modBuf, s32 flags, SceKernelLMOption *option); /* Disabled - returns error */
s32 sceKernelLoadModuleBufferBootInitBtcnf(SceSize modSize, u32 *modBuf, s32 flags, SceKernelLMOption *option, s32);

s32 sceKernelStopUnloadSelfModuleWithStatusKernel(s32 exitStatus, SceSize args, void *argp, s32 *status, SceKernelSMOption *option);

#endif	/* MODULEMGR_KERNEL_H */

