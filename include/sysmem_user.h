/* Copyright (C) 2011, 2012 The uOFW team
   See the file COPYING for copying permission.
*/

#ifndef SYSMEM_USER_H
#define	SYSMEM_USER_H

#include "common.h"

#ifdef	__cplusplus
extern "C" {
#endif

enum SceSysMemPartitionId {
    SCE_KERNEL_UNKNOWN_PARTITION = 0,
    SCE_KERNEL_PRIMARY_KERNEL_PARTITION = 1,
    SCE_KERNEL_PRIMARY_USER_PARTITION = 2,
};

enum SceSysMemBlockType {
    SCE_KERNEL_SMEM_Low = 0,
    SCE_KERNEL_SMEM_High = 1,
    SCE_KERNEL_SMEM_Addr = 2
};

SceUID sceKernelAllocPartitionMemory(SceUID partitionid, const s8 *name, s32 type, SceSize size, void *addr);
s32 sceKernelFreePartitionMemory(SceUID uid);

s32 sceKernelGetBlockHeadAddr(SceUID uid);


#ifdef	__cplusplus
}
#endif

#endif	/* SYSMEM_USER_H */
