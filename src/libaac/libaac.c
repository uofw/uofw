/* Copyright (C) 2011, 2012, 2013 The uOFW team
   See the file COPYING for copying permission.
*/

#include <common_imp.h>
#include <libaac.h>
#include <avcodec_audiocodec.h>
#include <usersystemlib_kernel.h>
#include <threadman_kernel.h>

#define SCE_AAC_MEM_SIZE (1024 * 100) // 0x19000
#define SCE_AAC_DATA_SIZE (0x18F20)

SCE_MODULE_INFO(
    "sceAac_Library",
    SCE_MODULE_ATTR_EXCLUSIVE_LOAD | SCE_MODULE_ATTR_EXCLUSIVE_START,
    1, 1
);
SCE_MODULE_START_THREAD_PARAMETER(3, 0x20, 0x400, 0);
SCE_MODULE_STOP_THREAD_PARAMETER(3, 0x20, 0x400, 0);
SCE_MODULE_BOOTSTART("sceAacStartEntry");
SCE_MODULE_STOP("sceAacEndEntry");
SCE_SDK_VERSION(SDK_VERSION);

void sub_00000000(s32);
void sub_000000F8(s32);
s32 sub_000012B8(s32);
void sub_000013B4(s32);

typedef struct {
    s32 state; // 0
    s32 loopNum; // 4
    s32 reset; // 8
    s32 streamStart; // 12
    s32 streamEnd; // 16
    s32 streamCur; // 20
    void *unk24; // ptr to data src
    s32 unk28;
    s32 unk32;
    void *unk36;
    s32 unk40; // bytes to decode
    s32 streamRemaining;
    void *decBuf; // 48
    s32 decSize; // 52
    s32 curChannel; // 56
    u32 sumDecodedSample; // 60
    s32 sampleRate; // 64
    u8 padding[8];
} SceAacIdInfo; // size 72

typedef struct {
    SceAudiocodecCodec codec; // 0
    SceAacIdInfo info __attribute__((aligned(128))); // 128
    u8 data[SCE_AAC_DATA_SIZE]; // 200
} SceAacId; // size 1024 * 100

static s32 g_poolId = -1; // 1740
static SceAacId *g_pool; // 1750
static s32 g_nbr; // 1754

void sub_00000000(s32 id)
{
    dbg_printf("sub_00000000(id=%d)\n", id);

    SceAacId *p;

    if (g_pool == NULL) {
        return;
    }

    if (g_nbr == 0) {
        return;
    }

    if (id < 0 || id >= g_nbr) {
        return;
    }

    p = g_pool + id * SCE_AAC_MEM_SIZE;

    if (p == NULL) {
        return;
    }

    if (((u32)p->info.unk24 + 2 * p->info.unk28 - (u32)p->codec.inBuf + 1600) < 1536) {
        sub_000000F8(id);
    }

    if (p->info.unk28 < p->info.unk40) {
        return;
    }

    if (p->info.unk36 != (p->info.unk24 + p->info.unk28 * (p->info.unk32 + 1) + 1600)) {
        return;
    }

    p->info.unk32 ^= 1;
    p->info.unk36 = p->info.unk24 + p->info.unk28 * p->info.unk32 + 1600;
}

void sub_000000F8(s32 id) {
    dbg_printf("sub_000000F8(id=%d)\n", id);

    SceAacId *p;
    void *dst;

    if (g_pool == NULL) {
        return;
    }

    if (g_nbr == 0) {
        return;
    }

    if (id < 0 || id >= g_nbr) {
        return;
    }

    p = g_pool + id * SCE_AAC_MEM_SIZE;

    if (p == NULL) {
        return;
    }

    dst = p->info.unk24 - ((u32)p->info.unk24 + (p->info.unk28 << 1) - (u32)p->codec.inBuf);

    // Kernel_Library_1839852A
    sceKernelMemcpy(
        dst,
        p->codec.inBuf,
        (SceSize)((u32)p->info.unk24 + (p->info.unk28 << 1) - (u32)p->codec.inBuf + 1600)
    );

    p->codec.inBuf = dst;
}

// module_stop
// sceAac_61AA43C9
s32 sceAacEndEntry(void) {
    dbg_printf("sceAacEndEntry()\n");

    s32 i;
    s32 p;
    s32 ret;

    for (i=0, p=0; i<g_nbr; i++, p+=SCE_AAC_MEM_SIZE) {
        if (g_pool == NULL) {
            break;
        }

        if (g_nbr == 0) {
            break;
        }

        if (i<0 || i>=g_nbr) {
            break;
        }

        if (g_pool + p == NULL) {
            continue;
        }

        ret = sceAacExit(i);

        if (ret < 0) {
            return ret;
        }
    }

    // sceAac_23D35CAE
    sceAacTermResource();

    return SCE_ERROR_OK;
}

// sceAac_5CFFC57C
s32 sceAacInitResource(s32 nbr)
{
    dbg_printf("sceAacInitResource(nbr=%d)\n", nbr);

    s32 size;
    s32 i;

    if (g_pool != NULL) {
        return 0x80691504; // SCE_ERROR_AAC_ALREADY_INITIALIZED
    }

    size = nbr * SCE_AAC_MEM_SIZE;

    g_poolId = sceKernelCreateFpl(
        "SceLibAacResouce", // name with a typo
        2, // PSP_MEMORY_PARTITION_USER
        0, // attr
        size, // size of pool
        1, // one block
        NULL // options
    );

    if (g_poolId < 0) {
        return 0x80691501; // SCE_ERROR_AAC_NOT_INITIALIZED
    }

    if (sceKernelAllocateFpl(g_poolId, (void**)&g_pool, NULL) < 0) {
        sceKernelDeleteFpl(g_poolId);

        g_poolId = -1;
        g_pool = NULL;

        return 0x80691501; // SCE_ERROR_AAC_NOT_INITIALIZED
    }

    if (size <= 0) {
        g_nbr = nbr;

        return SCE_ERROR_OK;
    }

    // memset
    for (i=0; i<size; i++) {
        ((u8*)g_pool)[i] = 0;
    }

    return SCE_ERROR_OK;
}

// sceAac_23D35CAE
s32 sceAacTermResource(void)
{
    dbg_printf("sceAacTermResource()\n");

    s32 i;
    s32 offset;
    SceAacId *p;

    if (g_pool == NULL) {
        return SCE_ERROR_OK;
    }

    if (g_nbr == 0) {
        return SCE_ERROR_OK;
    }

    if (g_poolId == -1) {
        return SCE_ERROR_OK;
    }

    for (i=0, offset=0; i<g_nbr; i++, offset+=SCE_AAC_MEM_SIZE) {
        p = NULL;

        if (g_pool != NULL && g_nbr != 0) {
            if (i>=0 && i<g_nbr) {
                p = g_pool + offset;
            }
        }

        if (p->info.state == 1) {
            return 0x80691502; // SCE_ERROR_AAC_NOT_TERMINATED
        }
    }

    // ThreadManForUser_F6414A71
    if (sceKernelFreeFpl(g_poolId, g_pool) < 0) {
        return 0x80691502; // SCE_ERROR_AAC_NOT_TERMINATED
    }

    g_pool = NULL;

    // ThreadManForUser_ED1410E0
    if (sceKernelDeleteFpl(g_poolId) < 0) {
        return 0x80691502; // SCE_ERROR_AAC_NOT_TERMINATED
    }

    g_poolId = -1;

    return SCE_ERROR_OK;
}

// sceAac_E0C89ACA
s32 sceAacInit(SceAacInitArg *arg)
{
    dbg_printf("sceAacInit(arg=%p)\n", arg);

    s32 intr;
    s32 i;
    s32 offset;
    SceAacId *p;
    s32 id;
    s32 ret;

    if (arg == NULL) {
        return 0x80691002;
    }

    if (arg->encBuf == NULL || arg->decBuf == NULL) {
        return 0x80691002;
    }

    if (arg->start < 0) {
        return 0x80691003;
    }

    if (arg->start >= arg->end) {
        if (arg->end != arg->start) {
            return 0x80691003;
        }

        if ((u32)arg->streamStart >= (u32)arg->streamEnd) {
            return 0x80691003;
        }
    }

    if (arg->encSize < 8192 || arg->decSize < 8192 || arg->zero != 0) {
        return 0x80691003;
    }

    if (g_pool == NULL || g_nbr == 0) {
        return 0x80691503;
    }

    if (arg->sampleRate != 24000 && arg->sampleRate != 32000 &&
        arg->sampleRate != 44100 && arg->sampleRate != 48000) {
        return 0x80691003;
    }

    intr = sceKernelCpuSuspendIntr();
    id = -1;

    for (i=0, offset=0; i<g_nbr; i++, offset+=SCE_AAC_MEM_SIZE) {
        p = NULL;

        if (g_pool != NULL && g_nbr != 0) {
            if (i>=0 && i<g_nbr) {
                p = g_pool + offset;
            }
        }

        if (p->info.state == 0) {
            p->info.state = 1;
            id = i;
            break;
        }
    }

    sceKernelCpuResumeIntr(intr);

    if (id < 0) {
        return 0x80691201;
    }

    p->info.loopNum = -1;
    p->info.streamStart = arg->streamStart;
    p->info.streamCur = arg->streamStart;
    p->info.reset = 0;
    p->info.streamEnd = arg->streamEnd;
    p->info.unk32 = 1;
    p->info.unk28 = (arg->encSize - 1600 + ((arg->encSize - 1600) < 0)) >> 1; // wtf?
    p->streamRemaining = arg->streamEnd - arg->streamStart;
    p->info.unk40 = 0;
    p->info.curChannel = 0;
    p->info.unk36 = arg->encBuf + 1600;
    p->info.decSize = (arg->decSize + (arg->decSize < 0)) >> 1;
    p->info.unk24 = arg->encBuf;
    p->info.sumDecodedSample = 0;

    p->info.state = 2;

    p->info.sampleRate = arg->sampleRate;
    p->info.decBuf = arg->decBuf;

    ret = sub_000012B8(id);
    if (ret < 0) {
        // sceAac_33B8C009
        sceAacExit(id);

        return ret;
    }

    p->info.state = 3;

    return id;
}

// sceAac_E955E83A
s32 sceAac_E955E83A(s32 *sampleRate)
{
    dbg_printf("sceAac_E955E83A(sampleRate=%p)\n", sampleRate);

    s32 intr;
    s32 i;
    s32 offset;
    SceAacId *p;
    s32 id;
    s32 ret;

    if (*sampleRate != 24000 && *sampleRate != 32000 &&
        *sampleRate != 44100 && *sampleRate != 48000) {
        return 0x80691003;
    }

    intr = sceKernelCpuSuspendIntr();
    id = -1;

    for (i=0, offset=0; i<g_nbr; i++, offset+=SCE_AAC_MEM_SIZE) {
        p = NULL;

        if (g_pool != NULL && g_nbr != 0) {
            if (i>=0 && i<g_nbr) {
                p = g_pool + offset;
            }
        }

        if (p->info.state == 0) {
            p->info.state = 1;
            id = i;
            break;
        }
    }

    sceKernelCpuResumeIntr(intr);

    if (id < 0) {
        return 0x80691201;
    }

    p->info.sampleRate = *sampleRate;
    p->info.unk24 = 0;
    p->info.decBuf = NULL;
    p->info.state = 2;
    p->codec.edramAddr = &p->data;
    p->codec.neededMem = SCE_AAC_DATA_SIZE;
    p->codec.unk20 = 1;
    p->codec.sampleRate = *sampleRate;
    p->codec.unk45 = 0;
    p->codec.unk44 = 0;

    // sceAudiocodec_5B37EB1D
    ret = sceAudiocodecInit(&p->codec, 0x1003);
    if (ret < 0) {
        // sceAac_33B8C009
        sceAacExit(id);

        return ret;
    }

    p->info.state = 3;

    return id;
}

// sceAac_33B8C009
s32 sceAacExit(s32 id)
{
    dbg_printf("sceAacExit(id=%d)\n", id);

    SceAacId *p;
    s32 intr;

    if (g_pool == NULL || g_nbr == 0) {
        return 0x80691503;
    }

    if (id < 0 || id >= g_nbr) {
        return 0x80691503;
    }

    p = g_pool + id * SCE_AAC_MEM_SIZE;

    if (p == NULL) {
        return 0x80691503;
    }

    if (id < 0 || id >= g_nbr) { // again?
        return 0x80691001;
    }

    intr = sceKernelCpuSuspendIntr();

    if (p->info.state > 0) {
        p->info.state = 0;
    }

    sceKernelCpuResumeIntr(intr);

    return SCE_ERROR_OK;
}

// sceAac_7E4CFEE4
s32 sceAacDecode(s32 id, void** src)
{
    dbg_printf("sceAacDecode(id=%d, src=%p)\n", id, src);

    SceAacId *p;

    if (id < 0 || id >= g_nbr) {
        return 0x80691001;
    }

    if (g_pool == NULL || g_nbr == 0) {
        return 0x80691503;
    }

    if (id < 0) { // again?
        return 0x80691503;
    }

    p = g_pool + id * SCE_AAC_MEM_SIZE;

    if (p == NULL) {
        return 0x80691503;
    }

    if (p->info.state < 3) {
        return 0x80691103;
    }

    if (src != NULL) {
        *src = p->codec.outBuf;
    }

    if (p->info.reset) {
        // Kernel_Library_A089ECA4
        sceKernelMemset(p->codec.outBuf, 0, p->info.decSize);

        p->info.curChannel = !p->info.curChannel;
        p->codec.outBuf = p->info.decBuf + p->info.decSize * p->info.curChannel;

        return 0;
    }

    if ((p->info.streamCur == p->info.streamEnd && p->streamRemaining > 0) ||
        (p->info.unk40 >= 1536)) {
        // sceAudiocodec_70A703F8
        if (sceAudiocodecDecode(&p->codec, 0x1003) < 0) {
            sub_000013B4(id);

            return 0x80691401;
        }

        p->info.unk40 -= p->codec.readSample;
        p->codec.inBuf += p->codec.readSample;
        p->streamRemaining -= p->codec.readSample;

        p->codec.outBuf = p->info.decBuf + p->info.decSize * !p->info.curChannel;
        p->info.sumDecodedSample += p->codec.decodedSample;
        p->info.curChannel = !p->info.curChannel;

        sub_000013B4(id);

        return p->codec.decodedSample;
    }

    // Kernel_Library_A089ECA4
    sceKernelMemset(p->codec.outBuf, 0, p->info.decSize);

    p->info.curChannel = !p->info.curChannel;
    p->codec.outBuf = p->info.decBuf + p->info.decSize * p->info.curChannel;

    return p->info.decSize;
}

// sceAac_FA01FCB6
s32 sceAac_FA01FCB6(s32 id, void *arg1, s32 *arg2, void *arg3, s32 *arg4)
{
    dbg_printf("sceAac_FA01FCB6(id=%d, arg1=%p, arg2=%p, arg3=%p, arg4=%p)\n", id, arg1, arg2, arg3, arg4);

    SceAacId *p;

    if (id < 0 || id >= g_nbr) {
        return 0x80691001;
    }

    if (g_pool == NULL || g_nbr == 0 || id < 0) {
        return 0x80691503;
    }

    p = g_pool + id * SCE_AAC_MEM_SIZE;

    if (p == NULL) {
        return 0x80691503;
    }

    if (p->info.state < 3) {
        return 0x80691103;
    }

    if (arg1 == NULL || arg3 == NULL) {
        return 0x80691002;
    }

    p->codec.inBuf = arg1;
    p->codec.outBuf = arg3;

    // sceAudiocodec_70A703F8
    if (sceAudiocodecDecode(&p->codec, 0x1003) != 0) {
        return 0x80691401;
    }

    *arg2 = p->codec.readSample;
    *arg4 = p->codec.decodedSample;

    return SCE_ERROR_OK;
}

// sceAac_D7C51541
s32 sceAacCheckStreamDataNeeded(s32 id)
{
    dbg_printf("sceAacCheckStreamDataNeeded(id=%d)\n", id);

    SceAacId *p;

    if (id < 0 || id >= g_nbr) {
        return 0x80691001;
    }

    if (g_pool == NULL || g_nbr == 0 || id < 0) {
        return 0x80691503;
    }

    p = g_pool + id * SCE_AAC_MEM_SIZE;

    if (p == NULL) {
        return 0x80691503;
    }

    if (p->info.state < 2) {
        return 0x80691102;
    }

    if (p->info.streamCur == p->info.streamEnd) {
        return 0;
    }

    return (p->info.unk36 < (p->info.unk24 + p->info.unk28 * (p->info.unk32 + 1) + 1600));
}

// sceAac_02098C69
s32 sceAacGetInfoToAddStreamData(s32 id, s32 **arg1, s32 *arg2, s32 *arg3)
{
    dbg_printf("sceAacGetInfoToAddStreamData(id=%d, arg1=%p, arg2=%p, arg3=%p)\n", id, arg1, arg2, arg3);

    SceAacId *p;

    if (id < 0 || id >= g_nbr) {
        return 0x80691001;
    }

    if (g_pool == NULL || g_nbr == 0 || id < 0) {
        return 0x80691503;
    }

    p = g_pool + id * SCE_AAC_MEM_SIZE;

    if (p == NULL) {
        return 0x80691503;
    }

    if (p->info.state < 2) {
        return 0x80691102;
    }

    // sceAac_D7C51541
    if (!sceAacCheckStreamDataNeeded(id)) {
        if (arg3 != NULL) {
            *arg3 = 0;
        }

        if (arg1 != NULL) {
            *arg1 = 0;
        }

        if (arg2 != NULL) {
            *arg2 = 0;
        }
    }
    else {
        if (arg3 != NULL) {
            *arg3 = p->info.streamCur;
        }

        if (arg1 != NULL) {
            *arg1 = p->info.unk36;
        }

        if (arg2 != NULL) {
            *arg2 = (p->info.unk24 + p->info.unk28 * (p->info.unk32 + 1) + 1600) - p->info.unk36;
        }
    }

    return SCE_ERROR_OK;
}

// sceAac_AC6DCBE3
s32 sceAacNotifyAddStreamData(s32 id, s32 size)
{
    dbg_printf("sceAacNotifyAddStreamData(id=%d, size=%d)\n", id, size);

    SceAacId *p;
    s32 remaining;

    if (id < 0 || id >= g_nbr) {
        return 0x80691001;
    }

    if (g_pool == NULL || g_nbr == 0 || id < 0) {
        return 0x80691503;
    }

    p = g_pool + id * SCE_AAC_MEM_SIZE;

    if (p == NULL) {
        return 0x80691503;
    }

    if (p->info.state < 2) {
        return 0x80691102;
    }

    if (size < 0 &&
        (p->info.unk24 + p->info.unk28 * (p->info.unk32 + 1) + 1600 - p->info.unk36) < size) {
        return 0x80691003;
    }

    remaining = p->info.streamEnd - p->info.streamCur;

    if (remaining < size) {
        p->info.streamCur = p->info.streamEnd;
        p->info.unk40 += remaining;
        p->info.unk36 += remaining;
    }
    else {
        p->info.unk40 += size;
        p->info.streamCur += size;
        p->info.unk36 += size;
    }

    sub_00000000(id);

    return SCE_ERROR_OK;
}

// sceAac_D2DA2BBA
s32 sceAacResetPlayPosition(s32 id)
{
    dbg_printf("sceAacResetPlayPosition(id=%d)\n", id);

    SceAacId *p;

    if (g_pool == NULL || g_nbr == 0) {
        return 0x80691503;
    }

    if (id < 0 || id >= g_nbr) {
        return 0x80691503;
    }

    p = g_pool + id * SCE_AAC_MEM_SIZE;

    if (g_pool == NULL) {
        return 0x80691503;
    }

    /* Never reached? */
    if (id < 0 || id >= g_nbr) {
        return 0x80691001;
    }

    if (p->info.state < 3) {
        return 0x80691103;
    }

    p->codec.inBuf = p->info.unk24 + 1600;
    p->streamRemaining = p->info.streamEnd - p->info.streamStart;
    p->info.unk36 = p->info.unk24 + 1600;
    p->info.reset = 0;
    p->info.streamCur = p->info.streamStart;
    p->info.unk40 = 0;
    p->info.sumDecodedSample = 0;
    p->info.unk32 = 1;

    return SCE_ERROR_OK;
}

// sceAac_BBDD6403
s32 sceAacSetLoopNum(s32 id, s32 loopNum)
{
    dbg_printf("sceAacSetLoopNum(id=%d, loopNum=%d)\n", id, loopNum);

    SceAacId *p;

    if (id < 0 || id >= g_nbr) {
        return 0x80691001;
    }

    if (g_pool == NULL || g_nbr == 0 || id < 0) {
        return 0x80691503;
    }

    p = g_pool + id * SCE_AAC_MEM_SIZE;

    if (p == NULL) {
        return 0x80691503;
    }

    if (p->info.state < 2) {
        return 0x80691102;
    }

    if (loopNum < 0) {
        p->info.loopNum = -1;
    }
    else {
        p->info.loopNum = loopNum;
    }

    return SCE_ERROR_OK;
}

// sceAac_6DC7758A
s32 sceAacGetMaxOutputSample(s32 id)
{
    dbg_printf("sceAacGetMaxOutputSample(id=%d)\n", id);

    SceAacId *p;
    s32 ret;
    u32 maxOutputSample;

    if (id < 0 || id >= g_nbr) {
        return 0x80691001;
    }

    if (g_pool == NULL || g_nbr == 0 || id < 0) {
        return 0x80691503;
    }

    p = g_pool + id * SCE_AAC_MEM_SIZE;

    if (p == NULL) {
        return 0x80691503;
    }

    if (p->info.state < 3) {
        return 0x80691103;
    }

    // sceAudiocodec_59176A0F
    ret = sceAudiocodecAlcExtendParameter(&p->codec, 0x1003, (s32*)&maxOutputSample);
    if (ret < 0) {
        return ret;
    }

    return maxOutputSample >> 2;
}

// sceAac_506BF66C
s32 sceAacGetSumDecodedSample(s32 id)
{
    dbg_printf("sceAacGetSumDecodedSample(id=%d)\n", id);

    SceAacId *p;

    if (g_pool == NULL || g_nbr == 0) {
        return 0x80691503;
    }

    if (id < 0 || id >= g_nbr) {
        return 0x80691503;
    }

    p = g_pool + id * SCE_AAC_MEM_SIZE;

    if (p == NULL) {
        return 0x80691503;
    }

    if (id < 0 || id >= g_nbr) {
        return 0x80691001;
    }

    if (p->info.state < 2) {
        return 0x80691102;
    }

    return p->info.sumDecodedSample >> 2;
}

// sceAac_523347D9
s32 sceAacGetLoopNum(s32 id)
{
    dbg_printf("sceAacGetLoopNum(id=%d)\n", id);

    SceAacId *p;

    if (id < 0 || id >= g_nbr) {
        return 0x80691001;
    }

    if (g_pool == NULL || g_nbr == 0 || id < 0) {
        return 0x80691503;
    }

    p = g_pool + id * SCE_AAC_MEM_SIZE;

    if (p == NULL) {
        return 0x80691503;
    }

    if (p->info.state < 2) {
        return 0x80691102;
    }

    return p->info.loopNum;
}

// module_start
// sceAac_6C05813B
s32 sceAacStartEntry(SceSize argc __attribute__((unused)), void *argp __attribute__((unused)))
{
    dbg_init(1, FB_HARDWARE, FAT_NONE);
    dbg_printf("sceAacStartEntry()\n");

    return SCE_ERROR_OK;
}

s32 sub_000012B8(s32 id)
{
    dbg_printf("sub_000012B8(id=%d)\n", id);

    SceAacId *p;
    s32 ret;

    if (g_pool == NULL || g_nbr == 0) {
        return 0x80691503;
    }

    if (id < 0 || id >= g_nbr) {
        return 0x80691503;
    }

    p = g_pool + id * SCE_AAC_MEM_SIZE;

    if (p == NULL) {
        return 0x80691503;
    }

    p->codec.neededMem = SCE_AAC_DATA_SIZE;
    p->codec.unk20 = 1;
    p->codec.edramAddr = p->data;
    p->codec.sampleRate = p->info.sampleRate;
    p->codec.unk45 = 0;
    p->codec.unk44 = 0;

    ret = sceAudiocodecInit(&p->codec, 0x1003);
    if (ret < 0) {
        return ret;
    }

    p->codec.inBuf = p->info.unk24 + 1600;
    p->codec.outBuf = p->info.decBuf + p->info.decSize * p->info.curChannel;

    // sceAudiocodec_8ACA11D5
    ret = sceAudiocodecGetInfo(&p->codec, 0x1003);
    if (ret < 0) {
        return ret;
    }

    return SCE_ERROR_OK;
}

void sub_000013B4(s32 id)
{
    dbg_printf("sub_000013B4(id=%d)\n", id);

    SceAacId *p;

    if (g_pool == NULL || g_nbr == 0) {
        return;
    }

    if (id < 0 || id >= g_nbr) {
        return;
    }

    p = g_pool + id * SCE_AAC_MEM_SIZE;

    if (p == NULL) {
        return;
    }

    if (p->info.streamCur == p->info.streamEnd && p->streamRemaining <= 0) {
        if (p->info.loopNum > 0) {
            p->info.reset = 1;
        }
        else {
            p->info.loopNum--;

            sceAacResetPlayPosition(id);
        }
    }

    sub_00000000(id);
}
