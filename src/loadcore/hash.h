/* Copyright (C) 2011, 2012 The uOFW team
   See the file COPYING for copying permission.
*/

#ifndef HASH_H
#define	HASH_H

#include "common_header.h"

#ifdef	__cplusplus
extern "C" {
#endif

__inline__ u32 getCyclicPolynomialHash(const char *str, u32 radix, u32 hashTableSize);


#ifdef	__cplusplus
}
#endif

#endif	/* HASH_H */

