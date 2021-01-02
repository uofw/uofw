/* Copyright (C) 2011 - 2016 The uOFW team
   See the file COPYING for copying permission.
*/

/**
 * uofw/include/ms_cm_error.h
 *
 * PSP Memory Stick Class Manager
 */

#ifndef MS_CM_ERROR_H
#define	MS_CM_ERROR_H

/* PSP CM specific error codes. */

#define SCE_MSCM_ERROR_INVALID_PARAM    0x80220081
#define SCE_MSCM_ERROR_NOMEM            0x80220082
#define SCE_MSCM_ERROR_OUT_OF_SERVICE   0x80220083
#define SCE_MSCM_ERROR_NOT_FOUND        0x80220084
#define SCE_MSCM_ERROR_ALREADY          0x80220085
#define SCE_MSCM_ERROR_BUSY             0x80220086
#define SCE_MSCM_ERROR_TIMEOUT          0x80220087
#define SCE_MSCM_ERROR_CRC              0x80220088
#define SCE_MSCM_ERROR_TOE              0x80220089
#define SCE_MSCM_ERROR_DMA              0x8022008A
#define SCE_MSCM_ERROR_CMDNK            0x8022008B
#define SCE_MSCM_ERROR_FLASH            0x8022008C

#endif	/* MS_CM_ERROR_H */

