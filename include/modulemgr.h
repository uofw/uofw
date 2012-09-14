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

