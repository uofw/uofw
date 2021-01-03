/* Copyright (C) 2011, 2012, 2013 The uOFW team
   See the file COPYING for copying permission.
*/

#ifndef THREADMAN_USER_H
#define THREADMAN_USER_H

#include "common_header.h"

/* LwMutex */

enum SceKernelLwMutexTypes {
    SCE_KERNEL_LWMUTEX_RECURSIVE = 0x200
};

typedef struct {
	s32 lockCount; // 0
	s32 thid; // 4
	u32 flags; // 8
	s32 unk3; // 12
	s32 id; // 16
} SceLwMutex;

s32 _sceKernelLockLwMutexCB(SceLwMutex *mutex, s32 count);
s32 _sceKernelLockLwMutex(SceLwMutex *mutex, s32 count);
s32 _sceKernelUnlockLwMutex(SceLwMutex *mutex, s32 count);
s32 sceKernelReferLwMutexStatusByID(s32 id, u32 *addr);
s32 _sceKernelAllocateTlspl(s32, void*, s32);

#endif /* THREADMAN_USER_H */

