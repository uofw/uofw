/* Copyright (C) 2011 - 2016 The uOFW team
   See the file COPYING for copying permission.
*/

/**
 * uofw/include/net/lib_http.h
 *
 */

#ifndef LIB_HTTP_H
#define	LIB_HTTP_H

/* lib_http specific error codes. */

#define SCE_HTTP_ERROR_BEFORE_INIT              0x80431001
#define SCE_HTTP_ERROR_NOT_SUPPORTED            0x80431004
#define SCE_HTTP_ERROR_ALREADY_INITED           0x80431020
#define SCE_HTTP_ERROR_BUSY                     0x80431021
#define SCE_HTTP_ERROR_OUT_OF_MEMORY            0x80431022
#define SCE_HTTP_ERROR_NOT_FOUND                0x80431025
#define SCE_HTTP_ERROR_INSUFFICIENT_HEAPSIZE    0x80431077
#define SCE_HTTP_ERROR_BEFORE_COOKIE_LOAD       0x80431078
#define SCE_HTTP_ERROR_INVALID_ID               0x80431100
#define SCE_HTTP_ERROR_OUT_OF_SIZE              0x80431104
#define SCE_HTTP_INVALID_VALUE                  0x804311FE

#endif	/* LIB_HTTP_H */

