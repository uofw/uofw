/* Copyright (C) 2011, 2012, 2013 The uOFW team
   See the file COPYING for copying permission.
*/

#ifndef MODULEMGR_USER_H
#define	MODULEMGR_USER_H

#include "common_header.h"
#include "modulemgr_options.h"
#include "sysmem_user.h"

// TODO: rename to sceKernelLoadModuleForUser() ?
SceUID sceKernelLoadModule(const char *path, s32 flags, SceKernelLMOption *option);

/**
 * Load a module using the MS API.
 * 
 * @param path Path of the module to load.
 * @param flags Unused.
 * @param pOpt A pointer to a SceKernelLMOption structure, which holds various options about the way to load the module. 
 *              Pass NULL if you don't want to specify any option. When specifying the SceKernelLMOption structure for 
 *              the pOpt argument, set the size of the structure in SceKernelLMOption.size.

 * @return SCE_ERROR_OK on success, < 0 on error.
 * @return SCE_ERROR_KERNEL_CANNOT_BE_CALLED_FROM_INTERRUPT if function was called by an interrupt handler.
 * @return SCE_ERROR_KERNEL_ILLEGAL_PERMISSION_CALL if function was not called from a user context.
 * @return SCE_ERROR_KERNEL_ILLEGAL_ADDR if the path is NULL, or path/pOpt can't be accessed from the current context.
 * @return SCE_ERROR_KERNEL_UNKNOWN_MODULE_FILE if the path contains a '%' (protection against formatted strings attack)
 * @return SCE_ERROR_KERNEL_ILLEGAL_SIZE if SdkVersion >= 2.80 and opt->size != sizeof(SceKernelLMOption)
 * @return One of the errors of sceIoOpen() if failed
 * @return SCE_ERROR_KERNEL_PROHIBIT_LOADMODULE_DEVICE if sceIoIoctl() failed
 */
// Subroutine ModuleMgrForUser_710F61B5 - Address 0x0000128C
s32 sceKernelLoadModuleMs(const char *path, s32 flags, SceKernelLMOption *pOpt);

s32 sceKernelStartModule(SceUID modId, SceSize args, void *argp, s32 *result, SceKernelSMOption *option);
s32 sceKernelUnloadModule(SceUID modId);


#endif	/* MODULEMGR_USER_H */

