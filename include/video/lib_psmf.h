/* Copyright (C) 2011 - 2016 The uOFW team
   See the file COPYING for copying permission.
*/

/**
 * uofw/include/video/lib_psmf.h
 *
 * lib_psmf is a library which parses the header portion of PSMF files \n
 * and retrieves information on streams. 
 */

#ifndef LIB_PSMF_H
#define	LIB_PSMF_H

/* lib_psmf specific error codes. */

#define SCE_MPEG_ERROR_PSMF_NOT_INITIALIZED     0x80615001
#define SCE_MPEG_ERROR_PSMF_UNMATCHED_VERSION   0x80615002
#define SCE_MPEG_ERROR_PSMF_NOT_FOUND           0x80615025
#define SCE_MPEG_ERROR_PSMF_INVALID_ID          0x80615100
#define SCE_MPEG_ERROR_PSMF_INVALID_VALUE       0x806151FE
#define SCE_MPEG_ERROR_PSMF_INVALID_TIMESTAMP   0x80615500
#define SCE_MPEG_ERROR_PSMF_INVALID_PSMF        0x80615501

#endif	/* LIB_PSMF_H */

