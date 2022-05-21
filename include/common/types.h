/* Copyright (C) 2011, 2012, 2013 The uOFW team
   See the file COPYING for copying permission.
*/

#ifndef TYPES_H
#define TYPES_H

#include <stdint.h>

/*
 * Define NULL if not already defined
 */

#ifndef NULL
# define NULL           ((void*)0)
#endif

/*
 * Shorter and more precise type names
 */

/* Unsigned */
typedef uint8_t         u8;
typedef uint16_t        u16;
typedef uint32_t        u32;
typedef uint64_t        u64;

/* Signed */
typedef int8_t          s8;
typedef int16_t         s16;
typedef int32_t         s32;
typedef int64_t         s64;

/* Volatile (should be used for hardware addresses) unsigned */

typedef volatile uint8_t  vu8;
typedef volatile uint16_t vu16;
typedef volatile uint32_t vu32;
typedef volatile uint64_t vu64;

/* Volatile signed */
typedef volatile int8_t   vs8;
typedef volatile int16_t  vs16;
typedef volatile int32_t  vs32;
typedef volatile int64_t  vs64;

/* 
 * Kernel types
 */

/* ID of most kernel objects */
typedef s32             SceUID;
#define SCE_UID_NAME_LEN 31 /* Maximum name length of a kernel object. */

/* Size, unsigned or signed (for memory blocks, etc.) */
typedef u32             SceSize;
typedef s32             SceSSize;

/* Types used by some modules */
typedef u8              SceUChar;
typedef u8              SceUChar8;
typedef u16             SceUShort16;
typedef u32             SceUInt;
typedef u32             SceUInt32;
typedef u64             SceUInt64;
typedef u64             SceULong64;

typedef u8              SceChar8;
typedef u16             SceShort16;
typedef u32             SceInt32;                                                                                                                                                                         
typedef s64             SceInt64;                                                                                                                                                                         
typedef s64             SceLong64;

typedef s32             SceFloat;
typedef s32             SceFloat32;

typedef u16             SceWChar16;
typedef u32             SceWChar32;

#define SCE_FALSE		(0)
#define SCE_TRUE		(1)
typedef s32             SceBool;

typedef void            SceVoid;
typedef void *          ScePVoid;

/* Permission mode when creating a file (in octal, like the chmod function and UNIX command) */
typedef s32             SceMode;
/* An offset inside a file */
typedef SceInt64        SceOff;

#endif

