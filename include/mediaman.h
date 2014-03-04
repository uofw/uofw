/* Copyright (C) 2011, 2012, 2013, 2014 The uOFW team
   See the file COPYING for copying permission.
*/

#include "common_header.h"

#ifndef MEDIAMAN_H
#define	MEDIAMAN_H

#ifdef	__cplusplus
extern "C" {
#endif
    
/**
 * SCE UMD disc states.
 */    
enum SceUmdDiscStates {
    /** The UMD driver has been initiated. */
    SCE_UMD_INIT	  =	(0),
    /** A medium has been removed. */
    SCE_UMD_MEDIA_OUT =	(1 << 0),
    /** A medium has been inserted. */
    SCE_UMD_MEDIA_IN  =	(1 << 1),
    /** The inserted medium has changed. */
    SCE_UMD_MEDIA_CHG =	(1 << 2),
    /** The UMD device is not ready. */
    SCE_UMD_NOT_READY =	(1 << 3),
    /** The UMD device is ready. */
    SCE_UMD_READY	  =	(1 << 4),
    /** The inserted medium is readable. */
    SCE_UMD_READABLE  =	(1 << 5),
};

/**
 * SCE UMD device power modes. 
 */
enum SceUmdDevicePowerModes {
    SCE_UMD_MODE_POWER_ON  = (1 << 0),
    SCE_UMD_MODE_POWER_CUR = (1 << 1),
};

enum SceUmdMediaTypeFormats {
    /** Unknown format. */
    SCE_UMD_FMT_UNKNOWN	= 0x00000,
    /** Game format. */
    SCE_UMD_FMT_GAME	= 0x00010,
    /** Video format. */
    SCE_UMD_FMT_VIDEO	= 0x00020,
    /** Audio format. */
    SCE_UMD_FMT_AUDIO	= 0x00040,
    /** Cleaning format. */
    SCE_UMD_FMT_CLEAN	= 0x00080,
};

/** UMD file system alias name. */
#define SCE_UMD_ALIAS_NAME			"disc0:"

typedef struct {
    /** The size of the used SceUmdDiscInfo version. uiSize = sizeof(SceUmdDiscInfo). */
	SceSize uiSize;
    /** Defines the media type of the UMD. One of ::SceUmdMediaTypeFormats. */
	u32 uiMediaType;
} SceUmdDiscInfo;


#ifdef	__cplusplus
}
#endif

#endif	/* MEDIAMAN_H */

