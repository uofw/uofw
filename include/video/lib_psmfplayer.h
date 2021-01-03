/* Copyright (C) 2011 - 2016 The uOFW team
   See the file COPYING for copying permission.
*/

/**
 * uofw/include/video/lib_psmfplayer.h
 *
 * lib_psmfplayer is a library that can be used to easily implement PSMF stream playback.
 * 
 */

#ifndef LIB_PSMFPLAYER_H
#define	LIB_PSMFPLAYER_H

/* lib_psmfplayer specific error codes. */

#define SCE_MPEG_ERROR_PSMFPLAYER_NOT_INITIALIZED       0x80616001
#define SCE_MPEG_ERROR_PSMFPLAYER_UNMATCHED_VERSION     0x80616002
#define SCE_MPEG_ERROR_PSMFPLAYER_NOT_SUPPORTED         0x80616003
#define SCE_MPEG_ERROR_PSMFPLAYER_BUSY                  0x80616004
#define SCE_MPEG_ERROR_PSMFPLAYER_OUT_OF_MEMORY         0x80616005
#define SCE_MPEG_ERROR_PSMFPLAYER_INVALID_ID            0x80616006
#define SCE_MPEG_ERROR_PSMFPLAYER_INVALID_COMMAND       0x80616007
#define SCE_MPEG_ERROR_PSMFPLAYER_INVALID_VALUE         0x80616008
#define SCE_MPEG_ERROR_PSMFPLAYER_TOO_BIG_OFFSET        0x80616009
#define SCE_MPEG_ERROR_PSMFPLAYER_FAILED_READ_HEADER    0x8061600a
#define SCE_MPEG_ERROR_PSMFPLAYER_FATAL                 0x8061600b
#define SCE_MPEG_ERROR_PSMFPLAYER_NODATA                0x8061600c
#define SCE_MPEG_ERROR_PSMFPLAYER_INVALID_PSMF          0x8061600d
#define SCE_MPEG_ERROR_PSMFPLAYER_ABORTED               0x8061600e

#endif	/* LIB_PSMFPLAYER_H */

