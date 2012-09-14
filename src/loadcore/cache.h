/* Copyright (C) 2011 The uOFW team
   See the file COPYING for copying permission.
*/

#ifndef CACHE_H
#define	CACHE_H

#include "common_header.h"

void sceKernelDcacheWBinvAll(void);
void sceKernelIcacheClearAll(void);

#endif	/* CACHE_H */

