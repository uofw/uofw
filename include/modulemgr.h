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

/** The module will remain in memory and act as a resident library. */
#define SCE_KERNEL_RESIDENT                     (0)

/** The module is not a resident one, meaning it won't stay in memory and act as a resident library. */
#define SCE_KERNEL_NO_RESIDENT                  (1)

#define SCE_KERNEL_STOP_SUCCESS                 (0)
#define SCE_KERNEL_STOP_FAIL                    (1)

/** Holds various information about a module, can be obtained using sceKernelQueryModuleInfo() */
typedef struct {
    /** Size of this structure (96). */
    SceSize size; //0
    /** Number of segments of the module */
    u8 nsegment; //4
    /** Reserved, unused */
    u8 reserved[3]; //5
    /** Start address of the segment */
    u32 segmentAddr[SCE_KERNEL_MAX_MODULE_SEGMENT]; //8
    /** Size of the segment */
    SceSize segmentSize[SCE_KERNEL_MAX_MODULE_SEGMENT]; //24
    /** Entry address of the module */
    u32 entryAddr; //40
    /** Value of gp? */
    u32 gpValue; //44
    /** Start address of the text segment */
    u32 textAddr; //48
    /** Size of the text segment */
    SceSize textSize; //52
    /** Size of the data segment? */
    SceSize dataSize; //56
    /** Size of the bss segment (part of the data segment containing statically-allocated variables)? */
    SceSize bssSize; //60
    /** Module attribute */ 
    u16 attribute; //64
    /** Module version */
    u8 version[MODULE_VERSION_NUMBER_CATEGORY_SIZE]; //66
    /** Module name */
    char modName[SCE_MODULE_NAME_LEN]; //68
    /** String terminator (always '\0') */
    char terminal; //95
} SceKernelModuleInfo; // size = 96

/** Deprecated since firmware 1.50, use SceKernelModuleInfo instead. Holds various information about a module, can be obtained using sceKernelQueryModuleInfo()
 * @see :SceKernelModuleInfo
**/
typedef struct {
    /** Size of this structure (64). */
    SceSize size; //0
    /** Number of segments of the module */
    u8 nsegment; //4
    /** Reserved, unused */
    u8 reserved[3]; //5
    /** Start address of the segment */
    u32 segmentAddr[SCE_KERNEL_MAX_MODULE_SEGMENT]; //8
    /** Size of the segment */
    SceSize segmentSize[SCE_KERNEL_MAX_MODULE_SEGMENT]; //24
    /** Entry address of the module */
    u32 entryAddr; //40
    /** Value of gp? */
    u32 gpValue; //44
    /** Start address of the text segment */
    u32 textAddr; //48
    /** Size of the text segment */
    SceSize textSize; //52
    /** Size of the data segment? */
    SceSize dataSize; //56
    /** Size of the bss segment (part of the data segment containing statically-allocated variables)? */
    SceSize bssSize; //60
} SceKernelModuleInfoV1 __attribute__((deprecated)); // size = 64

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

