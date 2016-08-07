/* Copyright (C) 2011 - 2016 The uOFW team
   See the file COPYING for copying permission.
*/

/**
 * uofw/include/net/lib_ssl.h
 *
 * This library is needed to perform HTTPS connections.
 */

#ifndef LIB_SLL_H
#define	LIB_SLL_H

#define SCE_SSL_LEAST_STACK_SIZE        (1*1024)		/* 1KiB */

/* lib_sll specific error codes. */

#define SCE_SSL_ERROR_BEFORE_INIT       0x80435001
#define SCE_SSL_ERROR_ALREADY_INITED    0x80435020
#define SCE_SSL_ERROR_OUT_OF_MEMORY     0x80435022
#define SCE_SSL_ERROR_NOT_FOUND         0x80435025
#define SCE_SSL_ERROR_INVALID_VALUE     0x804351FE
#define SCE_SSL_ERROR_INVALID_FORMAT    0x80435108

#endif	/* LIB_SLL_H */

