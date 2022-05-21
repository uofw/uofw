/* Copyright (C) 2011 - 2015 The uOFW team
   See the file COPYING for copying permission.
*/

/**
 * @defgroup DNAS_ERROR DNAS Errors
 * @ingroup DNAS
 *
 * uofw/include/dnas_error.h \n
 * Defines error codes specifically for the DNAS facility. \n
 *
 * @{
 */

#ifndef DNAS_ERROR_H
#define	DNAS_ERROR_H

#include "common_header.h"

#ifdef	__cplusplus
extern "C" {
#endif

/*
    DNAS error codes
    SCE_ERROR_FACILITY_DNAS = 0x053
*/

#define SCE_DNAS_ERROR_OPERATION_FAILED         0x80530300 /*!< The requested operation failed. */
#define SCE_DNAS_ERROR_INVALID_ARGUMENTS        0x80530301 /*!< The provided arguments are not correct. */


#ifdef	__cplusplus
}
#endif

#endif	/* DNAS_ERROR_H */

/** @} */

