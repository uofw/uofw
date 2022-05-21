/* Copyright (C) 2011 - 2015 The uOFW team
   See the file COPYING for copying permission.
*/

#ifndef LOADMODULECHECKS_INLINE_H
#define LOADMODULECHECKS_INLINE_H

#include <common_imp.h>

#include <interruptman.h>
#include <modulemgr_options.h>
#include <sysmem_sysclib.h>
#include <sysmem_kernel.h>

#include "modulemgr_int.h"

static inline s32 _checkCallConditionKernel(void)
{
    if (pspK1IsUserMode())
        return SCE_ERROR_KERNEL_ILLEGAL_PERMISSION_CALL;

    return SCE_ERROR_OK;
}

static inline s32 _checkCallConditionUser(void)
{
    if (!pspK1IsUserMode())
        return SCE_ERROR_KERNEL_ILLEGAL_PERMISSION_CALL;

    return SCE_ERROR_OK;
}

static inline s32 _checkPathConditions(const char *path)
{
    if (path == NULL || !pspK1PtrOk(path))
        return SCE_ERROR_KERNEL_ILLEGAL_ADDR;

    /* Protection against formatted string attacks, path cannot contain a '%'. */
    if (strchr(path, '%'))
        return SCE_ERROR_KERNEL_UNKNOWN_MODULE_FILE;

    return SCE_ERROR_OK;
}

// sub_00007620
static inline s32 _checkLMOptionConditions(const SceKernelLMOption *pOption)
{
    s32 sdkVersion;

    if (pOption == NULL)
        return SCE_ERROR_OK;

    if (!pspK1StaBufOk(pOption, sizeof(SceKernelLMOption)))
        return SCE_ERROR_KERNEL_ILLEGAL_ADDR;

    sdkVersion = sceKernelGetCompiledSdkVersion() & 0xFFFF0000;
    if (sdkVersion >= 0x02080000 && pOption->size != sizeof(SceKernelLMOption))
        return SCE_ERROR_KERNEL_ILLEGAL_SIZE;

    return SCE_ERROR_OK;
}

// sub_00007620
static inline s32 _checkSMOptionConditions(const SceKernelSMOption *pOption)
{
    s32 sdkVersion;

    if (pOption == NULL)
        return SCE_ERROR_OK;

    if (!pspK1StaBufOk(pOption, sizeof(SceKernelSMOption)))
        return SCE_ERROR_KERNEL_ILLEGAL_ADDR;

    sdkVersion = sceKernelGetCompiledSdkVersion() & 0xFFFF0000;
    if (sdkVersion >= 0x02080000 && pOption->size != sizeof(SceKernelSMOption))
        return SCE_ERROR_KERNEL_ILLEGAL_SIZE;

    if (pOption->attribute & ~THREAD_SM_LEGAL_ATTR)
        return SCE_ERROR_KERNEL_ERROR;

    return SCE_ERROR_OK;
}

static inline s32 _checkMemoryBlockInfoConditions(const SceSysmemMemoryBlockInfo *pBlkInfo, u64 offset)
{
    u32 offsetLow;
    u32 offsetHigh;

    if (!pspK1DynBufOk((void *)pBlkInfo->addr, pBlkInfo->memSize)) // 0x00000D00
        return SCE_ERROR_KERNEL_ILLEGAL_ADDR;

    offsetLow = (u32)offset;
    offsetHigh = (u32)(offset >> 32);
    if (offsetHigh > 0 || ((offsetHigh == 0) && (pBlkInfo->memSize < offsetLow))) //0x00000D10 & 0x00000D18 & 0x00000E08
        return SCE_ERROR_KERNEL_INVALID_ARGUMENT;

    /* Only allow offsets which are a multiple of 64. */
    if (offsetLow & 0x3F) // 0x00000D38
        return SCE_ERROR_KERNEL_INVALID_ARGUMENT;

    return SCE_ERROR_OK;
}

static inline s32 _checkSecureInstalledIdConditions(const char *secureInstallId)
{
    if (secureInstallId == NULL || !pspK1StaBufOk(secureInstallId, SCE_SECURE_INSTALL_ID_LEN))
        return SCE_ERROR_KERNEL_ILLEGAL_ADDR;

    return SCE_ERROR_OK;
}

#endif	/* LOADMODULECHECKS_INLINE_H */

