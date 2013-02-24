/* Copyright (C) 2011, 2012 The uOFW team
   See the file COPYING for copying permission.
*/

#include "common_header.h"

#define SCE_KERNEL_LWMUTEX_RECURSIVE	0x200

typedef struct {
	s32 lockCount; // 0
	u32 owner; // 4
	u32 flags; // 8
	u32 unk3; // 12
	u32 id; // 16
} SceLwMutex;

s32 sceKernelCpuSuspendIntr(void);
void sceKernelCpuResumeIntr(s32 intr);
void sceKernelCpuResumeIntrWithSync(s32 intr);
s32 sceKernelIsCpuIntrSuspended(s32 intr);
s32 sceKernelIsCpuIntrEnable(void);

s32 sceKernelGetThreadId(void);
s32 sceKernelCheckThreadStack(void);
s64 sceKernelTryLockLwMutex(SceLwMutex *mutex, u32 count);
s64 sceKernelTryLockLwMutex_600(SceLwMutex *mutex, u32 count);
s32 sceKernelUnlockLwMutex(SceLwMutex *mutex, u32 count);

void *sceKernelMemcpy(void *dst, const void *src, u32 size);
void *sceKernelMemset(void *dst, s32 val, u32 size);

