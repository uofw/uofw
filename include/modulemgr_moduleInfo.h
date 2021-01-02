/* Copyright (C) 2011 - 2015 The uOFW team
   See the file COPYING for copying permission.
*/

#ifndef MODULEMGR_MODULEINFO_H
#define	MODULEMGR_MODULEINFO_H

#include "common_header.h"
#include "loadcore.h"

/** Holds various information about a module, can be obtained using sceKernelQueryModuleInfo() */
typedef struct {
    /** Size of this structure. size = sizeof(SceKernelModuleInfo). */
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
    /** Value of gp */
    u32 gpValue; //44
    /** Start address of the text segment */
    u32 textAddr; //48
    /** Size of the text segment. */
    SceSize textSize; //52
    /** Size of the data segment. */
    SceSize dataSize; //56
    /** Size of the bss segment. */
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
    /** Size of this structure. size = sizeof(SceKernelModuleInfoV1). */
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
    /** Value of gp */
    u32 gpValue; //44
    /** Start address of the text segment */
    u32 textAddr; //48
    /** Size of the text segment */
    SceSize textSize; //52
    /** Size of the data segment. */
    SceSize dataSize; //56
    /** Size of the bss segment. */
    SceSize bssSize; //60
} SceKernelModuleInfoV1; // size = 64

#endif	/* MODULEMGR_MODULEINFO_H */