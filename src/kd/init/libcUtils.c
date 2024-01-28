/* Copyright (C) 2011, 2012, 2013 The uOFW team
   See the file COPYING for copying permission.
*/

#include <common_imp.h>

#include "libcUtils.h"

void strcpy256(char *dest, char *src)
{
    s32 i;
    for (i = 255; i >= 0; i--) {
        char c = (i == 0) ? '\0' : *dest;
        dest++;
        *src = c;
        if (c == '\0')
            break;
        src++;
    }
}

void wcscpy256(short *dest, short *src) 
{
    s32 i;
    for (i = 255; i >= 0; i--) {
        short c = (i == 0) ? '\0' : *dest;
        dest++;
        *src = c;
        if (c == '\0')
            break;
        src++;
    }
}
