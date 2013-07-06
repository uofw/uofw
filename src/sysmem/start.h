#ifndef START_H
#define START_H

typedef struct
{
    u32 h[4];
    u32 pad;
    u16 usRemains;
    u16 usComputed;
    u64 ullTotalLen;
    u8 buf[64];
} SceKernelUtilsMd5Context;

void md5BlockUpdate(SceKernelUtilsMd5Context *ctx, u8 *data);

typedef struct
{
    u32 h[5];
    u16 usRemains;
    u16 usComputed;
    u64 ullTotalLen;
    u8 buf[64];
} SceKernelUtilsSha1Context;

void sha1BlockUpdate(u8 *data, SceKernelUtilsSha1Context *ctx);
void sha1Digest(u8 *data, u32 size, u8 *digest);

typedef struct {
    u32 count;
    u32 state[624];
} SceKernelUtilsMt19937Context;

u32 mt19937UInt(SceKernelUtilsMt19937Context *ctx);

int sceKernelDeflateDecompress(u8 *dest, u32 destSize, const u8 *src, void **unk);

extern int g_13B40;
extern int g_13B44;
extern int g_13B48;
extern int g_13B80;
extern int g_13B84;

#endif

