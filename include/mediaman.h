/* Copyright (C) 2011, 2012, 2013, 2014 The uOFW team
   See the file COPYING for copying permission.
*/

#include "common_header.h"

/** @defgroup UMDDriveManagment UMD Drive Managment
 *  Manages the UMD drive and accessing data on it.	
 */

/** @defgroup Mediaman Mediaman
 *  @ingroup UMDDriveManagment
 * 
 *  Mediaman enables users to access the UMD drive. The drive can be accessed through files or sectors. \n\n
 * 
 *  To access the UMD drive via a file, the PSP provides the ISO-9660 file system. A file can be accessed \n
 *  by standard I/O functions as in sceIoOpen(), sceIoClose() and sceIoRead(). The general path format is the \n
 *  following: "device alias name" + "unit number" + ":" + "file path name". Here, the device alias name and \n
 *  the unit number are set by the sceUmdActivate() function ("disc0:" being the standard). \n
 *  To open a file, consider this example: \n
 *      sceIoOpen("disc0:PSP_GAME/EXAMPLEDIR/examplefile.exp"); \n
 * 
 *  To access the UMD drive by sectors, the PSP provides a UMD block device driver. You can read the sectors \n
 *  by using standard I/O functions (i.e. sceIoOpen(), sceIoClose() and sceIoRead()). The general path format \n
 *  is the follwoing: "block device name" + "unit number" + ':'. Note that the block device is set by the \n
 *  sceUmdActivate() function to "umd0:". \n
 *  To open a phyisical device as a device file, consider this example: \n
 *      sceIoOpen("UMD0:...."); \n
 *  By default, opening a new physical device sets the first sector to read at sector_0. \n
 * 
 *  A sector contains 2048 bytes and reading is performed sector wise. That is, in sceIoRead() the read size \n
 *  has to be specifized in the number of sectors you want to read. \n
 * 
 * @{
 */

#ifndef MEDIAMAN_H
#define	MEDIAMAN_H

#ifdef	__cplusplus
extern "C" {
#endif
    
/** UMD file system alias name. */
#define SCE_UMD_ALIAS_NAME			"disc0:"

/** LBA raw sector access. */
#define SCE_UMD_LBA_DEVICE_NAME		"umd1:"
    
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
 * UMD drive power modes. 
 */
enum SceUmdDevicePowerModes {
    /** Set the UMD drive's power state to ON. */
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

/** @} */

