/* Copyright (C) 2011, 2012, 2013, 2014 The uOFW team
   See the file COPYING for copying permission.
*/

/**
 * @defgroup UMD_ERROR UMD Errors
 * @ingroup UMDDriveManagement
 * 
 * uofw/include/umd_error.h \n
 *
 * @{
 */

#ifndef UMD_ERROR_H
#define	UMD_ERROR_H

#include "common_header.h"

#ifdef	__cplusplus
extern "C" {
#endif

/*
    UMD error codes
    SCE_ERROR_FACILITY_UMD = 0x021
*/

#define SCE_ERROR_UMD_NOT_READY                                 0x80210001  /** Device not ready. */   
#define SCE_ERROR_UMD_LBA_OUT_OF_RANGE                          0x80210002  /** Logical block addressing out of range. */ 
#define SCE_ERROR_UMD_NO_MEDIUM                                 0x80210003  /** No disc. */
#define SCE_ERROR_UMD_UNKNOWN_MEDIUM                            0x80210004  /** Unknown disc medium. */
#define SCE_ERROR_UMD_HARDWARE_FAILURE                          0x80210005  /** Hardware failure. */
#define SCE_ERROR_UMD_POWER_OFF                                 0x80210006  /** UMD device without power. */
#define SCE_UMD_ERROR_REPLACED                                  0x80210007  /** Media switching prohibited. */
#define SCE_UMD_ERROR_INVALID_LAYOUT                            0x80210008  /** DVD data image layout is invalid. */
#define SCE_UMD_READAHEAD_REQ_FULL                              0x80210009  /** Read ahead requests are full. */
#define SCE_UMD_READAHEAD_NOREQ                                 0x80210010  /** Non-existent read ahead request. */
#define SCE_UMD_READAHEAD_BUSY                                  0x80210011  /** Read ahead request already running. */


#ifdef	__cplusplus
}
#endif

#endif	/* UMD_ERROR_H */

/** @} */

