/* Copyright (C) 2011 - 2016 The uOFW team
   See the file COPYING for copying permission.
*/

/**
 * uofw/include/sound/lib_sas.h
 *
 * Low-level sound library for the PSPs Software Audio Synthesizer (SAS).
 */

#ifndef LIB_SAS_H
#define	LIB_SAS_H

/* SAS driver specific error codes. */

#define	SCE_SAS_ERROR_ADDRESS			        (0x80420005)

#define	SCE_SAS_ERROR_VOICE_INDEX		        (0x80420010)
#define	SCE_SAS_ERROR_NOISE_CLOCK		        (0x80420011)
#define	SCE_SAS_ERROR_PITCH_VAL			        (0x80420012)
#define	SCE_SAS_ERROR_ADSR_MODE			        (0x80420013)
#define	SCE_SAS_ERROR_ADPCM_SIZE		        (0x80420014)
#define	SCE_SAS_ERROR_LOOP_MODE			        (0x80420015)
#define	SCE_SAS_ERROR_INVALID_STATE		        (0x80420016)
#define	SCE_SAS_ERROR_VOLUME_VAL		        (0x80420018)
#define	SCE_SAS_ERROR_ADSR_VAL			        (0x80420019)
#define	SCE_SAS_ERROR_PCM_SIZE			        (0x8042001a)
#define	SCE_SAS_ERROR_ATRAC3_SIZE		        (0x8042001b)

#define	SCE_SAS_ERROR_FX_TYPE			        (0x80420020)
#define	SCE_SAS_ERROR_FX_FEEDBACK		        (0x80420021)
#define	SCE_SAS_ERROR_FX_DELAY			        (0x80420022)
#define	SCE_SAS_ERROR_FX_VOLUME_VAL		        (0x80420023)
#define	SCE_SAS_ERROR_FX_UNAVAILABLE	        (0x80420024)

#define	SCE_SAS_ERROR_BUSY                      (0x80420030)

#define	SCE_SAS_ERROR_CHANGE_AT3_VOICE          (0x80420040)
#define	SCE_SAS_ERROR_NOT_AT3_VOICE             (0x80420041)
#define SCE_SAS_ERROR_NO_CONCATENATE_SPACE      (0x80420042)

#define	SCE_SAS_ERROR_NOTINIT			        (0x80420100)
#define	SCE_SAS_ERROR_ALRDYINIT			        (0x80420101)
#define SCE_SAS_ERROR_INVALID_ATRAC3	        (0x80420102)
#define SCE_SAS_ERROR_SMALL_ATRAC3_SIZE	        (0x80420103)

#endif	/* LIB_SAS_H */

