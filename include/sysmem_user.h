/* Copyright (C) 2011, 2012, 2013 The uOFW team
   See the file COPYING for copying permission.
*/

#ifndef SYSMEM_USER_H
#define	SYSMEM_USER_H

#include "common_header.h"

#include "sysmem_common.h"

enum SceSysMemPartitionId {
    SCE_KERNEL_UNKNOWN_PARTITION = 0,
    SCE_KERNEL_PRIMARY_KERNEL_PARTITION = 1,
    SCE_KERNEL_PRIMARY_USER_PARTITION = 2,
    SCE_KERNEL_PRIMARY_ME_PARTITION = 3,
    SCE_KERNEL_SECONDARY_KERNEL_PARTITION = 4,
    SCE_KERNEL_SECONDARY_USER_PARTITION = 5,
};

enum SceSysMemBlockType {
    SCE_KERNEL_SMEM_Low = 0,
    SCE_KERNEL_SMEM_High = 1,
    SCE_KERNEL_SMEM_Addr = 2
};

SceUID sceKernelAllocPartitionMemory(SceUID partitionid, const char *name, s32 type, SceSize size, void *addr);
s32 sceKernelFreePartitionMemory(SceUID uid);

void *sceKernelGetBlockHeadAddr(SceUID uid);


#endif	/* SYSMEM_USER_H */
