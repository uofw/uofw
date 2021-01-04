/* Copyright (C) 2011 - 2016 The uOFW team
   See the file COPYING for copying permission.
*/

/**
 * uofw/include/power_error.h
 *
 * Specific error codes for the Power Service. 
 */

#ifndef POWER_ERROR_H
#define	POWER_ERROR_H

#define SCE_POWER_ERROR_0010					(0x802B0010)	/*!< Perhaps "not implemented"? */
#define SCE_POWER_ERROR_NO_BATTERY				(0x802B0100)	/*!< No battery is present. */
#define SCE_POWER_ERROR_DETECTING				(0x802B0101)	/*!< Failed to obtain battery information. */
#define SCE_POWER_ERROR_CANNOT_LOCK_VMEM		(0x802B0200)	/*!< Failed to lock volatile memory. */
#define SCE_POWER_ERROR_BAD_PRECONDITION		(0x802B0300)	/*!< Invalid pre-conditions for an operation. */

#endif	/* POWER_ERROR_H */

