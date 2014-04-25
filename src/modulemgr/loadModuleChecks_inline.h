/* Copyright (C) 2011, 2012, 2013, 2014 The uOFW team
   See the file COPYING for copying permission.
*/

#ifndef LOADMODULECHECKS_INLINE_H
#define	LOADMODULECHECKS_INLINE_H

#include <common_imp.h>

#include <interruptman.h>
#include <modulemgr_options.h>
#include <sysmem_kernel.h>

static inline s32 _setupChecks(void)
{
    return pspShiftK1();
}

static inline s32 _checkCallConditionKernel(void)
{
    if (pspK1IsUserMode()) {
        return SCE_ERROR_KERNEL_ILLEGAL_PERMISSION_CALL;
    }
    return SCE_ERROR_OK;
}

static inline s32 _checkCallConditionUser(void)
{
    if (!pspK1IsUserMode()) {
        return SCE_ERROR_KERNEL_ILLEGAL_PERMISSION_CALL;
    }
    return SCE_ERROR_OK;
}

static inline s32 _checkPathConditions(const char *path) 
{
    if (path == NULL || !pspK1PtrOk(path)) { 
        return SCE_ERROR_KERNEL_ILLEGAL_ADDR;
    }  
    /* Protection against formatted string attacks, path cannot contain a '%'. */
    if (strchr(path, '%')) { 
        return SCE_ERROR_KERNEL_UNKNOWN_MODULE_FILE; 
    }
    return SCE_ERROR_OK;
}

// sub_00007620
static inline s32 _checkLMOptionConditions(SceKernelLMOption *pOpt)
{
    s32 sdkVersion; 
    
    if (pOpt != NULL) { 
        if (!pspK1StaBufOk(option, sizeof(SceKernelLMOption))) {
            return SCE_ERROR_KERNEL_ILLEGAL_ADDR;
        }
        sdkVersion = sceKernelGetCompiledSdkVersion() & 0xFFFF0000;
        // Firmware >= 2.80, updated size field
        if (sdkVersion >= 0x02080000 && pOpt->size != sizeof(SceKernelLMOption)) {
            return SCE_ERROR_KERNEL_ILLEGAL_SIZE;
        }
    }
    return SCE_ERROR_OK;
}

static inline s32 _checkMemoryBlockInfoConditions(SceSysmemMemoryBlockInfo *pBlkInfo, u64 offset)
{
    u32 offsetLow;
    u32 offsetHigh;
    
    if (!pspK1DynBufOk(pBlkInfo->addr, pBlkInfo->memSize)) { // 0x00000D00
        return SCE_ERROR_KERNEL_ILLEGAL_ADDR;
    }
    
    offsetLow = (u32)offset;
    offsetHigh = (u32)(offset >> 32);
    if (offsetHigh > 0 || ((offsetHigh == 0) && (pBlkInfo->memSize < offsetLow))) { //0x00000D10 & 0x00000D18 & 0x00000E08
        return SCE_ERROR_KERNEL_INVALID_ARGUMENT;
    }  
    // TODO: Create global ALIGNMENT check (in common/memory.h)?
    /* Proceed only with offset being a multiple of 64. */
    if (offsetLow & 0x3F) { // 0x00000D38
        return SCE_ERROR_KERNEL_INVALID_ARGUMENT;
    }
    return SCE_ERROR_OK;
}

static inline s32 _checkSecureInstalledIdConditions(const char *secureInstallId)
{
    if (secureInstallId == NULL || !pspK1StaBufOk(secureInstallId, 16)) {
        return SCE_ERROR_KERNEL_ILLEGAL_ADDR;
    }
    return SCE_ERROR_OK;
}

static inline void _terminateChecks(s32 k1State)
{
    pspSetK1(k1State);
}

#endif	/* LOADMODULECHECKS_INLINE_H */

