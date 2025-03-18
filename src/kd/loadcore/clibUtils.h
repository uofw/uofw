/* Copyright (C) 2011, 2012 The uOFW team
   See the file COPYING for copying permission.
*/

#ifndef CLIBUTILS_H
#define	CLIBUTILS_H

#include <common_header.h>

/*
 * 
 * Compare the string pointed to by s1 to the string pointed to by s2.
 * 
 * Returns 1, 0 or -1 accordingly as the strong pointed to by s1 is 
 * greater than, equal to, or less then the string pointed to by s2.
 */
s32 strcmp(const char *s1, const char *s2);

/*
 * Copy the first n characters of the array pointed to by src to the 
 * array pointed to by dest. If the array pointed to by src does not 
 * have n characters, NULL characters are copied for the remaining
 * portion. If copying takes place between objects that overlap, the 
 * behavior is undefined. 
 * 
 * Returns the value of dest.
 */
char *strncpy(char *dest, const char *src, s32 n);

/*
 * Compute the length of the string pointed to by s.
 * 
 * Returns the number of characters that preceed the terminating NULL
 * character.
 */
u32 strlen(const char *s);

/**
 * Copy n characters from the object pointed to by src into the object
 * pointed to by s1. If copying takes place between objects that 
 * overlap, the behavior is undefined. When all arguments are 
 * multiples of 4, copying can be executed faster via wmemcpy().
 * 
 * Returns the value of dest.
 */
void *memcpy(void *dest, const void *src, u32 n);



#endif	/* CLIBUTILS_H */

