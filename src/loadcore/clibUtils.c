/* Copyright (C) 2011, 2012 The uOFW team
   See the file COPYING for copying permission.
*/

#include "clibUtils.h"

static void *wmemcpy(u32 *dest, const u32 *src, u32 n);

//sub_000074CC
s32 strcmp(const char *s1, const char *s2)
{   
    if (s1 != NULL && s2 != NULL) {   
        while (*s1++ == *s2++) {
               if (*(s1 - 1) == '\0')
                   return 0;
        }
        return *(s1 - 1) - *(s2 - 1);
    }
    if (s1 == s2)           
        return 0;
    
    if (s1 != NULL)
        return 1;
    
    return -1;
}

//sub_0000754C
char *strncpy(char *dest, const char *src, s32 n) 
{
    char *tmpDest;
    int i;
    
    if (dest == NULL || src == NULL)
        return 0;
    
    tmpDest = dest;
    for (i = 0; i < n; i++) {
         if ((*tmpDest++ = *src++) == '\0') {
             while (++i < n)
                    *tmpDest++ = '\0';
             break;
         }
    }
    return dest;
}

//sub_000075C8
u32 strlen(const char *s) 
{
    u32 len;
    
    len = 0;
    if (s == NULL)
        return len;
    
    while (*s++ != '\0')
           len++;
    
    return len;
}

//sub_000075FC
void *memcpy(void *dest, const void *src, u32 n)
{
    u8 *tmpDest;
       
    if (dest == NULL)
        return NULL;
    
    /* Check if word-aligned (minimum alignment). */
    if ((((u32)dest | (u32)src | n) & 0x3) == 0)
        return wmemcpy(dest, src, n);
    
    /* Byte alignment. */
    tmpDest = dest;   
    while (n-- != 0)
           *tmpDest++ = *(u8 *)src++;
    
    return dest;
}

/*
 * Copy n characters from the object pointed to by src into the object
 * pointed to by s1. Note that n has to be a multiple of 4.
 * 
 * Returns the value of dest.
 */
static void *wmemcpy(u32 *dest, const u32 *src, u32 n)
{
    u32 *tmpDest;
    
    /* Alignment has to be at least 4 byte. */
    n &= ~0x3;
    if (n == 0)
        return dest;
    
    tmpDest = dest;
    /* 4-byte alignment check. */
    if (n & 0xC) {
        while (((void *)dest + (n & 0xC)) != tmpDest)
               *tmpDest++ = *src++;
        
        if (((void *)dest + n) == tmpDest)
            return dest;
    }
    /* 16-byte alignment. */
    while ((((void *)dest) + (n & 0xC)) != tmpDest) {
           tmpDest[0] = src[0];
           tmpDest[1] = src[1];
           tmpDest[2] = src[2];
           tmpDest[3] = src[3];
           tmpDest++;
           src++;
    }
    return dest; 
}
