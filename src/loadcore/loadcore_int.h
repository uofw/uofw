/* Copyright (C) 2011, 2012, 2013 The uOFW team
   See the file COPYING for copying permission.
*/


#ifndef LOADCORE_INT_H
#define	LOADCORE_INT_H

#include <loadcore.h>
#include <sysmem_kernel.h>
#include <sysmem_user.h>
#include "systable.h"
    
#define LOADCORE_ERROR                      (-1)

/* For compatibility reasons. Use PSP_MAGIC_LE. */
#define PSP_MAGIC                           (0x7E505350)  /* "~PSP" */

/* The magic number identifying a file as a PSP object file. */
#define PSP_MAGIC_LE						(0x5053507E)  /* "~PSP" in Little Endian. */

/* 
 * A test if a specified address is a kernel address. If the sign-bit
 * is set to 1 (example: 0x80000000) then the address is in kernel 
 * memory. In two's complement, a value with the sign-bit set is always
 * a negative number.
 */
#define IS_KERNEL_ADDR(addr)                ((s32)(addr) < 0)
    

extern s32 (*g_loadCoreHeap)(void);
extern s32 (*g_getLengthFunc)(u8 *file, u32 size, u32 *newSize);
extern s32 (*g_prepareGetLengthFunc)(u8 *buf, u32 size);
extern s32 (*g_setMaskFunc)(u32 unk1, vs32 *addr);
extern s32 (*g_compareSubType)(u32 tag);

extern SceLoadCore g_loadCore;  
extern const u32 g_hashData[];
extern void *g_segmentStart[];
extern u32 g_segmentSize[];
    
void loadCoreClearMem(u32 *baseAddr, u32 size);
    
s32 CheckLatestSubType(u8 *file);
    
s32 sceLoadCorePrimarySyscallHandler(void);

#endif	/* LOADCORE_INT_H */

