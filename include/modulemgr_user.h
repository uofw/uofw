/* Copyright (C) 2011, 2012, 2013 The uOFW team
   See the file COPYING for copying permission.
*/

#ifndef MODULEMGR_USER_H
#define	MODULEMGR_USER_H

#include "common_header.h"
#include "modulemgr_options.h"
#include "sysmem_user.h"

SceUID sceKernelLoadModule(const char *path, s32 flags, SceKernelLMOption *option);
s32 sceKernelStartModule(SceUID modId, SceSize args, void *argp, s32 *result, SceKernelSMOption *option);
s32 sceKernelUnloadModule(SceUID modId);


#endif	/* MODULEMGR_USER_H */

