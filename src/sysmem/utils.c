#include <common_imp.h>

#include <sysmem_sysclib.h>
#include <sysmem_utils_kernel.h>

#include "start.h"

u32 sceKernelUtilsMt19937UInt(SceKernelUtilsMt19937Context *ctx);
const void *sceKernelGzipGetCompressedData(const void *buf);

int sceKernelDcacheInvalidateRangeForUser(const void *p, u32 size)
{
    int ret;
    int oldK1 = pspShiftK1();
    if (!pspK1DynBufOk(p, size))
        ret = SCE_ERROR_KERNEL_ILLEGAL_ADDR;
    else
        ret = sceKernelDcacheInvalidateRange(p, size);
    pspSetK1(oldK1);
    return ret;
}

int UtilsForUser_157A383A(const void *p, u32 size)
{
    int ret;
    int oldK1 = pspShiftK1();
    if (!pspK1DynBufOk(p, size))
        ret = SCE_ERROR_KERNEL_ILLEGAL_ADDR;
    else
        ret = UtilsForKernel_157A383A(p, size);
    pspSetK1(oldK1);
    return ret;
}

int sceKernelDcachePurgeRangeForUser(const void *p, u32 size)
{
    int ret;
    int oldK1 = pspShiftK1();
    if (!pspK1DynBufOk(p, size))
        ret = SCE_ERROR_KERNEL_ILLEGAL_ADDR;
    else
        ret = sceKernelDcachePurgeRange(p, size);
    pspSetK1(oldK1);
    return ret;
}

int sceKernelIcacheInvalidateRangeForUser(const void *p, u32 size)
{
    int ret;
    int oldK1 = pspShiftK1();
    if (!pspK1DynBufOk(p, size))
        ret = SCE_ERROR_KERNEL_ILLEGAL_ADDR;
    else
        ret = sceKernelIcacheInvalidateRange(p, size);
    pspSetK1(oldK1);
    return ret;
}

int UtilsForUser_43C9A8DB(const void *p, u32 size)
{
    int ret;
    int oldK1 = pspShiftK1();
    if (!pspK1DynBufOk(p, size))
        ret = SCE_ERROR_KERNEL_ILLEGAL_ADDR;
    else
        ret = UtilsForKernel_43C9A8DB(p, size);
    pspSetK1(oldK1);
    return ret;
}

int sceKernelUtilsMd5BlockUpdate(SceKernelUtilsMd5Context *ctx, u8 *data, u32 size)
{
    int oldK1 = pspShiftK1();
    if (!pspK1PtrOk(ctx)
     || !pspK1DynBufOk(data, size)
     || ctx == NULL || data == NULL) {
        pspSetK1(oldK1);
        return SCE_ERROR_KERNEL_ILLEGAL_ADDR;
    }
    // ED60
    if (ctx->usComputed != 0)
    {
        pspSetK1(oldK1);
        // EE44
        return -1;
    }
    int remaining = ctx->usRemains;
    if (ctx->usRemains >= 64)
    {
        // EE34
        pspSetK1(oldK1);
        return SCE_ERROR_KERNEL_ILLEGAL_SIZE;
    }
    ctx->ullTotalLen += size;
    int numTotal = ctx->usRemains + size;
    // EDB0
    while (numTotal >= 64)
    {
        if (remaining == 0)
        {
            // EE1C
            data += 64;
            memcpy(ctx->buf, data, 64);
        }
        else
        {
            data += 64 - remaining;
            memcpy(ctx + remaining + 32, data, 64 - remaining);
            remaining = 0;
        }
        // EDD4
        numTotal -= 64;
        md5BlockUpdate(ctx, ctx->buf);
    }
    // EDEC
    if (numTotal != 0) {
        // EE04
        memcpy(ctx + remaining + 32, data, numTotal - remaining);
    }
    ctx->usRemains = numTotal;
    // EDF8
    pspSetK1(oldK1);
    return 0;
}

int sceKernelUtilsMd5BlockResult(SceKernelUtilsMd5Context *ctx, u8 *digest)
{
    u8 buf[64];
    int oldK1 = pspShiftK1();
    if (!pspK1PtrOk(ctx) || !pspK1PtrOk(digest)
     || ctx == NULL || digest == NULL) {
        pspSetK1(oldK1);
        return SCE_ERROR_KERNEL_ILLEGAL_ADDR;
    }
    // EED0
    if (ctx->usComputed == 0)
    {
        short remain = ctx->usRemains;
        if (remain >= 64)
        {
            // EFE4
            pspSetK1(oldK1);
            return SCE_ERROR_KERNEL_ILLEGAL_SIZE;
        }
        memset(buf, 0, 64);
        memcpy(buf, ctx->buf, remain);
        buf[remain] = 0x80;
        if (remain >= 56)
        {
            // EFC0
            md5BlockUpdate(ctx, buf);
            memset(buf, 0, 64);
        }
        // EF1C
        u64 num = ctx->ullTotalLen * 8;
        // EF38
        int i;
        for (i = 0; i < 8; i++) {
            // EF74
            buf[i + 56] = num >> (i * 8);
        }
        md5BlockUpdate(ctx, buf);
        ctx->usComputed = 1;
        memset(ctx->buf, 0, 64);
    }
    // EFA8
    memcpy(digest, ctx, 16);
    pspSetK1(oldK1);
    return 0;
}

int sceKernelUtilsMd5Digest(u8 *data, u32 size, u8 *digest)
{
    SceKernelUtilsMd5Context ctx;
    int oldK1 = pspShiftK1();
    if (!pspK1DynBufOk(data, size) || !pspK1PtrOk(digest))
    {
        // F04C
        pspSetK1(oldK1);
        return SCE_ERROR_KERNEL_ILLEGAL_ADDR;
    }
    // F068
    if (pspK1PtrOk(&ctx)) // ?!?
    {
        ctx.h[0] = 0x67452301;
        ctx.h[1] = 0xEFCDAB89;
        ctx.h[2] = 0x89BADCFE;
        ctx.h[3] = 0x10325476;
        ctx.ullTotalLen = 0;
        ctx.usRemains = 0;
        ctx.usComputed = 0;
    }
    // F0B8
    sceKernelUtilsMd5BlockUpdate(&ctx, data, size);
    sceKernelUtilsMd5BlockResult(&ctx, digest);
    pspSetK1(oldK1);
    return 0;
}

int sceKernelUtilsMd5BlockInit(SceKernelUtilsMd5Context *ctx)
{
    int oldK1 = pspShiftK1();
    if (!pspK1PtrOk(ctx) || ctx == NULL) {
        pspSetK1(oldK1);
        return SCE_ERROR_KERNEL_ILLEGAL_ADDR;
    }
    ctx->h[0] = 0x67452301;
    ctx->ullTotalLen = 0;
    ctx->h[1] = 0xEFCDAB89;
    ctx->h[2] = 0x98BADCFE;
    ctx->h[3] = 0x10325476;
    ctx->usRemains = 0;
    ctx->usComputed = 0;
    pspSetK1(oldK1);
    return 0;
}

int sceKernelUtilsSha1BlockUpdate(SceKernelUtilsSha1Context *ctx, u8 *data, u32 size)
{
    int buf[16];
    int oldK1 = pspShiftK1();
    if (!pspK1PtrOk(ctx) || !pspK1DynBufOk(data, size) || ctx == NULL || data == NULL)
    {
        // F1B8
        pspSetK1(oldK1);
        return SCE_ERROR_KERNEL_ILLEGAL_ADDR;
    }
    // F1E8
    if (ctx->usComputed != 0)
    {
        pspSetK1(oldK1);
        // F2F8
        return -1;
    }
    u16 remaining = ctx->usRemains;
    if (remaining >= 64)
    {
        // F2E8
        pspSetK1(oldK1);
        return SCE_ERROR_KERNEL_ILLEGAL_SIZE;
    }
    ctx->ullTotalLen += size;
    u16 total = remaining + size;
    // F230
    while (total >= 64)
    {
        if (remaining == 0)
        {
            // F2D4
            memcpy(buf, data, 64);
            data += 64;
        }
        else
        {
            memcpy(buf, ctx->buf, remaining);
            data += 64 - remaining;
            memcpy(buf + remaining, data, 64 - remaining);
            remaining = 0;
        }
        // F264
        // F26C
        int i;
        for (i = 0; i < 16; i++)
            buf[i] = pspWsbw(buf[i]);
        total -= 64;
        sha1BlockUpdate((u8 *)buf, ctx);
    }
    // F2A4
    if (total != 0) {
        // F2BC
        memcpy(&ctx->buf[remaining], data, total - remaining);
    }
    ctx->usRemains = total;
    // F2B0
    pspSetK1(oldK1);
    return 0;
}

int sceKernelUtilsSha1BlockResult(SceKernelUtilsSha1Context *ctx, u8 *digest)
{
    int buf[16];
    int oldK1 = pspShiftK1();
    if (!pspK1PtrOk(ctx) || !pspK1PtrOk(digest) || ctx == NULL || digest == NULL)
    {
        // F358
        pspSetK1(oldK1);
        return SCE_ERROR_KERNEL_ILLEGAL_ADDR;
    }
    // F384
    if (ctx->usComputed == 0)
    {
        int i;
        if (ctx->usRemains >= 64)
        {
            // F500
            pspSetK1(oldK1);
            return SCE_ERROR_KERNEL_ILLEGAL_SIZE;
        }
        memset(buf, 0, 64);
        memcpy(buf, ctx->buf, ctx->usRemains);
        buf[ctx->usRemains] = 0x80;
        if (ctx->usRemains >= 56)
        {
            // F3D8
            for (i = 0; i < 16; i++)
                buf[i] = pspWsbw(buf[i]);
            sha1BlockUpdate((u8 *)buf, ctx);
            memset(buf, 0, 64);
        }
        // F410
        for (i = 0; i < 8; i++)
            ((char*)buf)[i + 56] = ctx->ullTotalLen >> (56 - i * 8);
        // F480
        for (i = 0; i < 16; i++)
            buf[i] = pspWsbw(buf[i]);
        sha1BlockUpdate((u8 *)buf, ctx);
        // F4B0
        for (i = 0; i < 5; i++)
            ctx->h[i] = pspWsbw(ctx->h[i]);
        ctx->usComputed = 1;
        memset(ctx->buf, 0, 64);
    }
    // F4E8
    memcpy(digest, ctx->h, 20);
    pspSetK1(oldK1);
    return 0;
}

int sceKernelUtilsSha1Digest(u8 *data, u32 size, u8 *digest)
{
    int oldK1 = pspShiftK1();
    if (!pspK1PtrOk(digest) || !pspK1DynBufOk(data, size))
    {
        // F550
        pspSetK1(oldK1);
        return SCE_ERROR_KERNEL_ILLEGAL_ADDR;
    }
    sha1Digest(data, size, digest);
    pspSetK1(oldK1);
    return 0;
}

int sceKernelUtilsSha1BlockInit(SceKernelUtilsSha1Context *ctx)
{
    int oldK1 = pspShiftK1();
    if (!pspK1PtrOk(ctx) || ctx == NULL)
    {
        // F5F8
        pspSetK1(oldK1);
        return SCE_ERROR_KERNEL_ILLEGAL_ADDR;
    }
    ctx->ullTotalLen = 0;
    ctx->h[0] = 0x67452301;
    ctx->h[1] = 0xEFCDAB89;
    ctx->h[2] = 0x98BADCFE;
    ctx->h[3] = 0x10325476;
    ctx->h[4] = 0xC3D2E1F0;
    ctx->usRemains = 0;
    ctx->usComputed = 0;
    pspSetK1(oldK1);
    return 0;
}

int sceKernelUtilsMt19937Init(SceKernelUtilsMt19937Context *ctx, u32 seed)
{
    int oldK1 = pspShiftK1();
    if (!pspK1PtrOk(ctx))
    {
        // F6A8
        pspSetK1(oldK1);
        return SCE_ERROR_KERNEL_ILLEGAL_ADDR;
    }
    ctx->state[0] = seed;
    // F640
    int i;
    for (i = 1; i < 624; i++)
        ctx->state[i] = (ctx->state[i - 1] ^ (ctx->state[i - 1] >> 30)) * 0x6C078965 + 1;
    ctx->count = 0;
    // F678
    for (i = 0; i < 624; i++)
        sceKernelUtilsMt19937UInt(ctx);
    pspSetK1(oldK1);
    return 0;
}

u32 sceKernelUtilsMt19937UInt(SceKernelUtilsMt19937Context *ctx)
{
    int ret;
    int oldK1 = pspShiftK1();
    if (!pspK1PtrOk(ctx))
        ret = SCE_ERROR_KERNEL_ILLEGAL_ADDR;
    else
        ret = mt19937UInt(ctx);
    pspSetK1(oldK1);
    return ret;
}

typedef int clock_t;
typedef int time_t;

struct timeval;
struct timezone;

typedef clock_t (*ClockHandler)(void);
typedef time_t (*TimeHandler)(time_t *t);
typedef int (*GettimeofdayHandler)(struct timeval *tp, struct timezone *tzp);
typedef int (*RtcTickHandler)(u64 *tick);

// 14400
ClockHandler g_pFuncClock;
// 14404
TimeHandler g_pFuncTime;
// 14408
GettimeofdayHandler g_pFuncGettimeofday;
// 1440C
RtcTickHandler g_pFuncGetTick;

int sceKernelRegisterRtcFunc(ClockHandler clock, TimeHandler time, GettimeofdayHandler gettimeofday, RtcTickHandler rtctick)
{
    g_pFuncClock = clock;
    g_pFuncTime = time;
    g_pFuncGettimeofday = gettimeofday;
    g_pFuncGetTick = rtctick;
    return 0;
}

int sceKernelReleaseRtcFunc(void)
{
    g_pFuncClock = NULL;
    g_pFuncTime = NULL;
    g_pFuncGettimeofday = NULL;
    g_pFuncGetTick = NULL;
    return 0;
}

clock_t sceKernelLibcClock(void)
{
    if (g_pFuncClock == NULL)
        return 0;
    int oldK1 = pspShiftK1();
    clock_t ret = g_pFuncClock();
    pspSetK1(oldK1);
    return ret;
}

time_t sceKernelLibcTime(time_t *t)
{
    time_t ret;
    if (g_pFuncTime == NULL)
        return 0;
    int oldK1 = pspShiftK1();
    if (!pspK1PtrOk(t))
        ret = 0;
    else
        ret = g_pFuncTime(t);
    pspSetK1(oldK1);
    return ret;
}

int sceKernelLibcGettimeofday(struct timeval *tp, struct timezone *tzp)
{
    int ret;
    if (g_pFuncGettimeofday == NULL)
        return 1;
    int oldK1 = pspShiftK1();
    if (!pspK1PtrOk(tp) || !pspK1PtrOk(tzp))
        ret = 1;
    else
        ret = g_pFuncGettimeofday(tp, tzp);
    pspSetK1(oldK1);
    return ret;
}

int sceKernelRtcGetTick(u64 *tick)
{
    if (g_pFuncGetTick == NULL)
        return 0x80000001;
    return g_pFuncGetTick(tick);
}

int sceKernelGzipDecompress(u8 *dest, u32 destSize, const void *src, u32 *crc32)
{
    const void *buf = sceKernelGzipGetCompressedData(src);
    void *next;
    struct {
        u32 crc32;
        u32 inSize;
    } info;

    if (buf == NULL)
        return SCE_ERROR_KERNEL_ERROR;
    if (((char*)src)[2] != 8)
        return SCE_ERROR_NOT_SUPPORTED;
    int ret = sceKernelDeflateDecompress(dest, destSize, buf, &next);
    if (ret < 0)
        return ret;
    memcpy(&info, next, 8);
    if ((u32)ret != info.inSize)
        return SCE_ERROR_INVALID_FORMAT;
    if (crc32 != NULL)
        *crc32 = info.crc32;
    return ret;
}

int sceKernelGzipGetInfo(const void *buf, const void **extra, const char **name, const char **comment, u16 *crc16, const void **compressedData)
{
    const char *curBuf = buf + 10;
    u8 header[10];
    if (extra != NULL)
        *extra = NULL;
    if (name != NULL)
        *name = NULL;
    if (comment != NULL)
        *comment = NULL;
    if (crc16 != NULL)
        *crc16 = 0;
    if (compressedData != NULL)
        *compressedData = NULL;
    // F9AC
    if (buf == NULL)
        return SCE_ERROR_INVALID_POINTER;
    memcpy(header, buf, 10);
    if (header[0] != 0x1F || header[1] != 0x8B)
        return SCE_ERROR_INVALID_FORMAT;
    // FA14
    char flag = header[3];
    if ((flag & 4) != 0) // FEXTRA
    {
        if (extra != NULL)
            *extra = curBuf;
        // FA38
        curBuf += (curBuf[1] << 8) | curBuf[0];
    }
    // FA40
    if ((flag & 8) != 0) // FNAME
    {
        if (name != NULL)
            *name = curBuf;
        // FA50
        // FA5C
        while (*curBuf != 0)
            curBuf++;
        // FA68
    }
    // FA6C
    if ((flag & 0x10) != 0) // FCOMMENT
    {
        if (comment != NULL)
            *comment = curBuf;
        // FA7C
        // FA88
        while (*curBuf != 0)
            curBuf++;
        // FA94
    }
    // FA98
    if ((flag & 2) != 0) // FHCRC
    {
        if (crc16 != NULL)
            *crc16 = (curBuf[1] << 8) | curBuf[0];
        curBuf += 2;
    }
    // FAC0
    if (compressedData != NULL)
        *compressedData = curBuf;
    // FAC8
    return 0;
}

const void *sceKernelGzipGetCompressedData(const void *buf)
{
    const void *data;
    if (sceKernelGzipGetInfo(buf, NULL, NULL, NULL, NULL, &data) < 0)
        return NULL;
    return data;
}

int sceKernelGzipIsValid(const void *buf)
{
    if (((u8*)buf)[0] == 0x1F && ((u8*)buf)[1] == 0x8B)
        return 1;
    return 0;
}

const char *sceKernelGzipGetName(const void *buf)
{
    const char *name;
    if (sceKernelGzipGetInfo(buf, NULL, &name, NULL, NULL, NULL) < 0)
        return NULL;
    return name;
}

const char *sceKernelGzipGetComment(const void *buf)
{
    const char *comment;
    if (sceKernelGzipGetInfo(buf, NULL, NULL, &comment, NULL, NULL) < 0)
        return NULL;
    return comment;
}

