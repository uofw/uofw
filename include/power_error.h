/* Copyright (C) 2011 - 2016 The uOFW team
   See the file COPYING for copying permission.
*/

/** @defgroup PowerLibrary Power Library
 *  The Power service API surface.
 */

/**
 * uofw/include/power_error.h
 * 
 * @defgroup ErrorCodes Error Codes
 * @ingroup PowerLibrary
 *
 * Specific error codes for the Power Service. 
 * 
 * @{
 */

#ifndef POWER_ERROR_H
#define	POWER_ERROR_H

/* This constant represents an unknown power service error. */
#define SCE_POWER_ERROR_0010					0x802B0010
/* 
 *This constant indicates that no battery has been equipped or that the power service failed to
 * properly communicate with an installed battery.
 */
#define SCE_POWER_ERROR_NO_BATTERY				0x802B0100
/*
 * This constant indicates that the power service is currently busy detecting the new battery status.
 * This error occurs in a short timeframe after a battery has been removed/equipped or the AC connection
 * status has changed.
 */
#define SCE_POWER_ERROR_DETECTING				0x802B0101
/* This constant indicates that locking volatile memory failed. */
#define SCE_POWER_ERROR_CANNOT_LOCK_VMEM		0x802B0200
/* This constant indicates invalid pre-conditions for an operation. */
#define SCE_POWER_ERROR_BAD_PRECONDITION		0x802B0300

#endif	/* POWER_ERROR_H */

 /** @} */

