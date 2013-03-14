/* Copyright (C) 2011, 2012 The uOFW team
   See the file COPYING for copying permission.
*/

#include "common_header.h"

#define SCE_KERNEL_LWMUTEX_RECURSIVE	0x200

typedef struct {
	s32 lockCount; // 0
	s32 thid; // 4
	u32 flags; // 8
	s32 unk3; // 12
	s32 id; // 16
} SceLwMutex;

s32 sceKernelCpuSuspendIntr(void);
void sceKernelCpuResumeIntr(s32 intr);
void sceKernelCpuResumeIntrWithSync(s32 intr);
s32 sceKernelIsCpuIntrSuspended(s32 intr);
s32 sceKernelIsCpuIntrEnable(void);

/* When patched by the GE module, syscall to sceGeListUpdateStallAddr */
s32 sub_00000208(s32 dlId, void *stall);

/* sceGeListUpdateStallAddr designed for 'Genso Suikoden I & II' */
s32 sceGe_lazy_31129B95(s32 dlId, void *stall);

s32 sceKernelGetThreadId(void);
s32 sceKernelCheckThreadStack(void);
s32 sceKernelTryLockLwMutex(SceLwMutex *mutex, s32 count);
s32 sceKernelTryLockLwMutex_600(SceLwMutex *mutex, s32 count);
s32 sceKernelLockLwMutexCB(SceLwMutex *mutex, s32 count);
s32 sceKernelLockLwMutex(SceLwMutex *mutex, s32 count);
s32 sceKernelUnlockLwMutex(SceLwMutex *mutex, s32 count);
s32 Kernel_Library_3AD10D4D(SceLwMutex *mutex);
s32 sceKernelReferLwMutexStatus(SceLwMutex *mutex, u32 *addr);
void *Kernel_Library_FA835CDE(s32 arg0);

void *sceKernelMemcpy(void *dst, const void *src, u32 size);
void *sceKernelMemset(void *dst, s32 val, u32 size);

