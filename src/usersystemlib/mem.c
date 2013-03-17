/* Copyright (C) 2011, 2012, 2013 The uOFW team    
   See the file COPYING for copying permission.
*/

#include "usersystemlib_int.h"

// FIXME: naive, not reversed!
void *sceKernelMemcpy(void *dst, const void *src, SceSize size)
{
    u8 *dst8 = (u8*)dst;
    u8 *src8 = (u8*)src;

    while (size--) {
        *(dst8++) = *(src8++);
    }
    
    return dst;
}

// FIXME: naive, not reversed!
void *sceKernelMemset(void *dst, s32 val, SceSize size)
{
    u8 *dst8 = (u8*)dst;

    while (size--) {
        *(dst8++) = (u8)val;
    }

    return dst;
}
