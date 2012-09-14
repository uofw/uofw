/* Copyright (C) 2011, 2012 The uOFW team
   See the file COPYING for copying permission.
*/

#ifndef SYSMEM_USER_H
#define	SYSMEM_USER_H

#include "common_header.h"

#include "sysmem_common.h"

#ifdef	__cplusplus
extern "C" {
#endif

void *sceKernelGetBlockHeadAddr(SceUID uid);


#ifdef	__cplusplus
}
#endif

#endif	/* SYSMEM_USER_H */
