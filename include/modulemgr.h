/* Copyright (C) 2011 - 2015 The uOFW team
   See the file COPYING for copying permission.
*/

/*
 * uofw/include/modulemgr.h
 *
 * The module manager API.
 *
 */

#include "common_header.h"
#include "common/module.h"

#ifndef MODULEMGR_H
#define	MODULEMGR_H

#define SCE_KERNEL_MAX_MODULE_SEGMENT       (4) /** The maximum number of segments a module can have. */

#define SCE_MODULE_PRIVILEGE_LEVELS         (SCE_MODULE_MS | SCE_MODULE_USB_WLAN | SCE_MODULE_APP | SCE_MODULE_VSH | SCE_MODULE_KERNEL)

/** SceModule.status */
#define GET_MCB_STATUS(status)              (status & 0xF)
#define SET_MCB_STATUS(v, m)                (v = (v & ~0xF) | m)

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

#endif	/* MODULEMGR_H */

