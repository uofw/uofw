/* Copyright (C) 2011 - 2016 The uOFW team
   See the file COPYING for copying permission.
*/

/**
 * uofw/include/graphics/lib_font.h
 *
 * The lib_font library provides font glyph images to an application program.
 *
 */

#ifndef LIB_FONT_H
#define	LIB_FONT_H

#include "../common/errors.h"

/* lib_font specific error codes. */

#define SCE_FONT_NOERROR                (SCE_ERROR_OK)

#define SCE_FONT_ERROR_NOMEMORY         (0x80460001)    /*!< Failed to allocate memory. */ 
#define SCE_FONT_ERROR_LIBID            (0x80460002)    /*!< Invalid library instance. */ 
#define SCE_FONT_ERROR_ARG              (0x80460003)    /*!< Invalid argument. */

#define SCE_FONT_ERROR_NOFILE           (0x80460004)    /*!< No file. */      
#define SCE_FONT_ERROR_FILEOPEN         (0x80460005)    /*!< Failed to open specified file. */
#define SCE_FONT_ERROR_FILECLOSE        (0x80460006)    /*!< Failed to close specified file. */
#define SCE_FONT_ERROR_FILEREAD         (0x80460007)    /*!< Failed to read specified file. */
#define SCE_FONT_ERROR_FILESEEK         (0x80460008)    /*!< Failed to perform file seek operation. */

#define SCE_FONT_ERROR_TOOMANYOPENED    (0x80460009)    /*!< Too many opened fonts. */
#define SCE_FONT_ERROR_ILLEGALVERSION   (0x8046000a)    /*!< Unsupported font version. */
#define SCE_FONT_ERROR_DATAINCONSISTENT (0x8046000b)    /*!< Font data is inconsistent. */ 
#define SCE_FONT_ERROR_EXPIRED          (0x8046000c)    /*!< Usage period expired. */

#define SCE_FONT_ERROR_REGISTRY         (0x8046000d)    /*!< System registry-related error. */

#define SCE_FONT_ERROR_NOSUPPORT        (0x8046000e)    /*!< Not supported. */
#define SCE_FONT_ERR_UNKNOWN            (0x8046ffff)    /*!< Unknown error. */

#endif	/* LIB_FONT_H */

