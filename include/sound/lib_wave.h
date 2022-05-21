/* Copyright (C) 2011 - 2016 The uOFW team
   See the file COPYING for copying permission.
*/

/**
* uofw/include/sound/lib_wave.h
*
* lib_wave is a sound library that provides ADPCM (.vag) format fixed-pitch playback \n
* and 44.1 kHz 16-bit linear PCM format sound output functions.
*/

#ifndef LIB_WAVE_H
#define	LIB_WAVE_H

/* lib_wave specific error codes (0x80440000 - 0x8044ffff). */

#define	SCE_WAVE_ERROR_INITFAIL         (0x80440001) 
#define	SCE_WAVE_ERROR_EXITFAIL			(0x80440002)
#define	SCE_WAVE_ERROR_STARTFAIL		(0x80440003)
#define	SCE_WAVE_ERROR_ENDFAIL			(0x80440004)
#define	SCE_WAVE_ERROR_VOICENUM			(0x80440005)
#define	SCE_WAVE_ERROR_BUFFPTR			(0x80440006)
#define	SCE_WAVE_ERROR_SIZE				(0x80440007)
#define	SCE_WAVE_ERROR_MODE				(0x80440008)
#define	SCE_WAVE_ERROR_VOICEPLAY		(0x80440009)
#define	SCE_WAVE_ERROR_VOL				(0x8044000A)
#define	SCE_WAVE_ERROR_VOLPTR			(0x8044000B)
#define	SCE_WAVE_ERROR_RESTPTR			(0x8044000C)
#define	SCE_WAVE_ERROR_LOOPMODE			(0x8044000D)

#define	SCE_WAVE_ERROR_AUDIOCH			(0x80440010)
#define	SCE_WAVE_ERROR_SAMPLE			(0x80440011)
#define	SCE_WAVE_ERROR_THPRIORITY		(0x80440012)

#endif	/* LIB_WAVE_H */

