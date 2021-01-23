/* Copyright (C) The uOFW team
   See the file COPYING for copying permission.
*/

#include "common_header.h"

int UtilsForKernel_6C6887EE(void *outBuf, int outSize, void *inBuf, void **end);
void sceKernelDcacheInvalidateAll(void);
int  sceKernelDcacheProbe(void *addr);
int  sceKernelDcachePurgeRange(const void *p, u32 size);
void sceKernelDcacheWritebackAll(void);
void sceKernelDcacheWritebackInvalidateAll(void);
void sceKernelDcacheWritebackRange(const void *p, unsigned int size);
void sceKernelDcacheWritebackInvalidateRange(const void *p, unsigned int size);
int sceKernelDcacheInvalidateRange(const void *p, unsigned int size);
int UtilsForKernel_157A383A(const void *p, unsigned int size);
void sceKernelIcacheInvalidateAll(void);
int  sceKernelIcacheInvalidateRange(const void *addr, unsigned int size);
int  sceKernelIcacheProbe(const void *addr);
int UtilsForKernel_43C9A8DB(const void *p, u32 size);

int sceKernelRtcGetTick(u64 *tick);

#define SCE_KERNEL_UTILS_SHA1_DIGEST_SIZE		20
int sceKernelUtilsSha1Digest(u8 *data, u32 size, u8 *digest);

#define SCE_KERNEL_UTILS_MD5_DIGEST_SIZE		16
int sceKernelUtilsMd5Digest(u8 *data, u32 size, u8 *digest);

int sceKernelGzipDecompress(u8 *dest, u32 destSize, const void *src, u32 *unk);

int UtilsForKernel_39FFB756(int);

int UtilsForKernel_79D1C3FA(void);

int UtilsForKernel_A6B0A6B8(void);
