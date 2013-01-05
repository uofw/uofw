/* Copyright (C) 2011, 2012 The uOFW team
   See the file COPYING for copying permission.
*/

#ifndef SYSMEM_COMMON_H
#define SYSMEM_COMMON_H

/* Not sure where this should be put */

/* Accepts addresses with 3 first bits: 000, 010, 100, 101 (first half byte 0x0, 0x1, 0x4, 0x5, 0x8, 0x9, 0xA, 0xB) */
#define VALID_ADDR_SEGMENT(addr) ((0x35 >> ((addr >> 29) & 7)) & 1)
/* Accepts 0x10000 - 0x14000, with first bits as above */
#define ADDR_IS_SCRATCH(addr) (((addr >> 14) & 0x7FFF) == 4 && VALID_ADDR_SEGMENT(addr))
/* Accepts 0x04000000 - 0x047FFFFF, with first bits as above */
#define ADDR_IS_VRAM(addr) (((addr >> 23) & 0x3F) == 8 && VALID_ADDR_SEGMENT(addr))
/* Accepts 0x08000000 - 0x0FFFFFFF, 0x48000000 - 0x4FFFFFFF, 0x88000000 - 0x8FFFFFFF and 0xA8000000 - 0xAFFFFFFF */
#define ADDR_IS_RAM(addr) ((0x00220202 >> ((addr >> 27) & 0x1F)) & 1)

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

SceUID sceKernelAllocPartitionMemory(SceUID partitionid, const char *name, s32 type, SceSize size, void *addr);
s32 sceKernelFreePartitionMemory(SceUID uid);

#endif

