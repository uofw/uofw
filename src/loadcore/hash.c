/* Copyright (C) 2011, 2012 The uOFW team
   See the file COPYING for copying permission.
*/

#include "clibUtils.h"
#include "hash.h"

/*
 * Computes the hash
 *  $H(S) = qx^{rn} + \sum\limits_{i=1}^n x^{r(n-i)}s_{i-1}$
 *  or recursively: $H_n(S) = x^rH_{n-1}+s_{n-1}$
 *  with $H_0 = q$
 * @param str - The character array to hash
 * @param radix - 
 * @param hashTableSize - the size of the corresponding hash table
 * 
 * Returns the index in the hash table where the array can be found
 */
u32 getCyclicPolynomialHash(const char *str, u32 radix, u32 hashTableSize)
{
    u32 len;
    u32 hash;
    u32 addressMask;
    u32 index;
    u32 i;
    
    len = strlen(str);
    hash = len;
    addressMask = hashTableSize - 1;
    /* Computes sum from i = 0 to len of x^(r(n-i))*toHash[i] */
    for (i = 0; i < len; i++) {
         hash = (hash << radix | hash >> (8 * sizeof(u32) - radix)); //x^r * hash
         hash ^= str[i]; //hash + toHash[i]
    }
    hash ^= (hash >> 8) ^ (hash >> 16) ^ (hash >> 24);
    index = hash & addressMask;
    
    return index;
}
