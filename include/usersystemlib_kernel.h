/* Copyright (C) 2011, 2012 The uOFW team
   See the file COPYING for copying permission.
*/

#include "common_header.h"

#define SCE_KERNEL_LWMUTEX_RECURSIVE	0x200

typedef struct {
	s32 lockCount; // 0
	u32 thid; // 4
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
s64 sceKernelLockLwMutexCB(SceLwMutex *mutex, u32 count);
s64 sceKernelLockLwMutex(SceLwMutex *mutex, u32 count);
s32 sceKernelUnlockLwMutex(SceLwMutex *mutex, u32 count);
s32 Kernel_Library_3AD10D4D(SceLwMutex *mutex);
s32 sceKernelReferLwMutexStatus(SceMutex *mutex, u32 *addr);
void *Kernel_Library_FA835CDE(s32 arg0);

void *sceKernelMemcpy(void *dst, const void *src, u32 size);
void *sceKernelMemset(void *dst, s32 val, u32 size);

