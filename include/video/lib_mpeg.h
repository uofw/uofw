/* Copyright (C) 2011 - 2016 The uOFW team
   See the file COPYING for copying permission.
*/

/**
* uofw/include/video/lib_mpeg.h
*
* lib_mpeg is a library that can be used for demultiplexing a PSP Movie Format \n
* (for game) stream (PSMF), and decoding the demultiplexed video and audio streams.
*
* PSMF is based on the MPEG2 program stream, and multiplexes together multiple \n 
* video, audio, and user data streams into a single stream.
*/

#ifndef LIB_MPEG_H
#define	LIB_MPEG_H

#include "common\errors.h"

/* lib_mpeg specific error codes. */

/* for general libmpeg & Demux */

#define SCE_MPEG_ERROR_OK                       SCE_ERROR_OK
#define SCE_MPEG_ERROR_NOT_COMPLETED		    0x80618001
#define SCE_MPEG_ERROR_INVALID_VALUE		    0x806101FE
#define SCE_MPEG_ERROR_UNMATCHED_VERSION	    0x80610002
#define SCE_MPEG_ERROR_INVALID_POINTER		    0x80610103
#define SCE_MPEG_ERROR_OUT_OF_MEMORY		    0x80610022
#define SCE_MPEG_ERROR_NO_RAPI				    0x80618004
#define SCE_MPEG_ERROR_ALREADY_USED			    0x80618005
#define SCE_MPEG_ERROR_INTERNAL				    0x80618006
#define SCE_MPEG_ERROR_ILLEGAL_STREAM		    0x80618007
#define SCE_MPEG_ERROR_INSUFFICIENT_STACKSIZE	0x80618008
#define SCE_MPEG_ERROR_NOT_INITIALIZE		    0x80618009

/* for VIDEO Decoder */

#define SCE_MPEG_ERROR_VIDEO_INVALID_VALUE		0x806201FE
#define SCE_MPEG_ERROR_VIDEO_UNMATCHED_VERSION	0x80620002
#define SCE_MPEG_ERROR_VIDEO_ERROR 				0x80628001
#define SCE_MPEG_ERROR_VIDEO_FATAL 				0x80628002

/* for AUDIO Decoder */

#define SCE_MPEG_ERROR_AUDIO_UNKNOWN_ERROR		0x807F0001
#define SCE_MPEG_ERROR_AUDIO_FATAL 				0x807F00FC
#define SCE_MPEG_ERROR_AUDIO_ERROR 				0x807F00FD
#define SCE_MPEG_ERROR_AUDIO_INVALID_VALUE 		0x807F00FF

#endif	/* LIB_MPEG_H */

