/* Copyright (C) 2011, 2012 The uOFW team
   See the file COPYING for copying permission.
*/

/* 
 * modulemgr.h
 * 
 * The module manager API.
 *
 */

#include "common_header.h"

#ifndef MODULEMGR_H
#define	MODULEMGR_H

/** The maximum number of segments a module can have. */
#define SCE_KERNEL_MAX_MODULE_SEGMENT           (4)

/** The module could be started successfully. */
#define SCE_KERNEL_START_SUCCESS				(0)
/** The module could not be started successfully. */
#define SCE_KERNEL_START_FAIL					(1)


/** The module will remain in memory and act as a resident library. */
#define SCE_KERNEL_RESIDENT                     (SCE_KERNEL_START_SUCCESS)

/** The module is not a resident one, meaning it won't stay in memory and act as a resident library. */
#define SCE_KERNEL_NO_RESIDENT                  (SCE_KERNEL_START_FAIL)

/** The module could be stopped successfully. */
#define SCE_KERNEL_STOP_SUCCESS                 (0)
/** The module could not be stopped successfully. */
#define SCE_KERNEL_STOP_FAIL                    (1)

enum ModuleMgrMcbStatus {
	MCB_STATUS_NOT_LOADED = 0,
	MCB_STATUS_LOADING = 1,
	MCB_STATUS_LOADED = 2,
	MCB_STATUS_RELOCATED = 3,
	MCB_STATUS_STARTING = 4,
	MCB_STATUS_STARTED = 5,
	MCB_STATUS_STOPPING = 6,
	MCB_STATUS_STOPPED = 7,
	MCB_STATUS_UNLOADED = 8
};

s32 ModuleMgrForKernel_C3DDABEF(SceUID, void *, void *);
s32 sceKernelRebootBeforeForUser(void *);
s32 sceKernelRebootPhaseForKernel(s32, void *, s32, s32);
s32 sceKernelRebootBeforeForKernel(void *, s32, s32, s32);

#endif	/* MODULEMGR_H */

