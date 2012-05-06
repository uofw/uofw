/* Copyright (C) 2011, 2012 The uOFW team
   See the file COPYING for copying permission.
*/

#include "common.h"

int UtilsForKernel_6C6887EE(void *outBuf, int outSize, void *inBuf, void **end);
void sceKernelDcacheInvalidateAll(void);
int  sceKernelDcacheProbe(void *addr);
void sceKernelDcacheWritebackAll(void);
void sceKernelDcacheWritebackInvalidateAll(void);
void sceKernelDcacheWritebackRange(const void *p, unsigned int size);
void sceKernelDcacheWritebackInvalidateRange(const void *p, unsigned int size);
void sceKernelDcacheInvalidateRange(const void *p, unsigned int size);
void sceKernelIcacheInvalidateAll(void);
void sceKernelIcacheInvalidateRange(const void *addr, unsigned int size);
int  sceKernelIcacheProbe(const void *addr);

