/* Copyright (C) 2011, 2012 The uOFW team
   See the file COPYING for copying permission.
*/

#include "common_header.h"

s32 sceKernelCpuSuspendIntr(void);
void sceKernelCpuResumeIntr(s32 intr);
void sceKernelCpuResumeIntrWithSync(s32 intr);
s32 sceKernelIsCpuIntrSuspended(s32 intr);
s32 sceKernelIsCpuIntrEnable(void);

void *sceKernelMemcpy(void *dst, const void *src, u32 n);
void *sceKernelMemset(void *s, s32 c, u32 n);

