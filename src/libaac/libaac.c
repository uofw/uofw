#define SCE_AAC_MEM_SIZE (1024 * 100) // 0x19000

SCE_MODULE_STOP("sceAacEndEntry");

typedef struct {
    s32 unk0;
    s32 unk4;
    s32 unk8;
    s32 unk12;
    s32 unk16;
    s32 unk20; // bufsize
    s32 unk24;
    s32 unk28; // bufsize
    s32 sampleRate; // 32
    s32 unk36;
} SceAacInitArg;

typedef struct {
    s32 init;
    s32 loopNum; // 4
    s32 *unk8; // ptr to data src
    s32 unk12;
    s32 unk16; // stream end offset
    s32 unk20; // bytes read
    s32 *unk24; // ptr to data src
    s32 unk28;
    s32 unk32;
    u32 unk36;
    s32 unk40; // bytes to decode
    s32 unk44;
    s32 *unk48; // bufDec
    s32 unk52; // bufDecSize
    s32 unk56; // even
    s32 unk60;
    s32 sampleRate; // 64
} SceAacIdInfo;

typedef struct {
    SceAudiocodecCodec codec __attribute__((aligned(256))); // size 128
    SceAacIdInfo info __attribute__((aligned(128))); // size 128
} SceAacId; // size 1024 * 100

static s32 g_poolId = -1; // 1740
static SceAacId *g_pool; // 1750
static s32 g_nbr; // 1754

void sub_00000000(s32 id)
{
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

    if ((p->info.unk24 + 2 * p->info.unk28 - p->codec.inBuf + 1600) < 1536) {
        sub_000000F8(id, p->info.unk32);
    }

    if (p->info.unk28 < p->info.unk40) {
        return;
    }

    if (p->info.unk36 != (p->info.unk24 + p->info.unk28 * (p->info.unk32 + 1) + 1600) {
        return;
    }

    p->info.unk32 ^= 1;
    p->info.unk36 = p->info.unk24 + p->info.unk28 * p->info.unk32 + 1600;
}

void sub_000000F8(s32 id) {
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

    dst = p->info.unk24 - (p->info.unk24 + (p->info.unk28 << 1) - p->codec.inBuf);

    // Kernel_Library_1839852A
    sceKernelMemcpy(
        dst,
        p->codec.inBuf,
        p->info.unk24 + (p->info.unk28 << 1) - p->codec.inBuf + 1600
    );

    p->codec.inBuf = dst;
}

// module_stop
// sceAac_61AA42C9
s32 sceAacEndEntry(void) {
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

    if (sceKernelAllocateFpl(g_poolId, &g_pool, NULL) < 0) {
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

        if (p->info.init == 1) {
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
    s32 intr;
    s32 i;
    s32 offset;
    SceAacId *p;
    s32 id;
    s32 ret;

    if (arg == NULL) {
        return 0x80691002;
    }

    if (arg->unk16 == 0 || arg->unk24 == 0) {
        return 0x80691002;
    }

    if (arg->unk4 < 0) {
        return 0x80691003;
    }

    if (arg->unk4 >= arg->unk12) {
        if (arg->unk12 != arg->unk4) {
            return 0x80691003;
        }

        if ((u32)arg->unk0 >= (u32)arg->unk8) {
            return 0x80691003;
        }
    }

    if (arg->unk20 < 8192 || arg->unk28 < 8192 || arg->unk36 != 0) {
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

    if (i=0, offset=0; i<g_nbr; i++, offset+=SCE_AAC_MEM_SIZE) {
        p = NULL;

        if (g_pool != NULL && g_nbr != 0) {
            if (i>=0 && i<g_nbr) {
                p = g_pool + offset;
            }
        }

        if (p->info.init == 0) {
            p->info.init = 1;
            id = i;
            break;
        }
    }

    sceKernelCpuResumeIntr(intr);

    if (id < 0) {
        return 0x80691201;
    }

    p->info.loopNum = -1;
    p->info.unk12 = arg->unk0;
    p->info.unk20 = arg->unk0;
    p->info.unk8 = NULL;
    p->info.unk16 = arg->unk8;
    p->info.unk32 = 1;
    p->info.unk28 = (arg->unk20 - 1600 + ((arg->unk20 - 1600) < 0)) >> 1; // wtf?
    p->info.unk44 = arg->unk8 - arg->unk0;
    p->info.unk40 = 0;
    p->info.unk56 = 0;
    p->info.unk36 = arg->unk16 + 1600;
    p->info.unk52 = (arg->unk28 + (arg->unk28 < 0)) >> 1;
    p->info.unk24 = arg->unk16;
    p->info.unk60 = 0;

    p->info.init = 2;

    p->info.sampleRate = arg->sampleRate;
    p->info.unk48 = arg->unk24;

    ret = sub_000012B8();
    if (ret < 0) {
        // sceAac_33B8C009
        sceAacExit(id);

        return ret;
    }

    p->info.init = 3;

    return id;
}

// sceAac_E955E83A
s32 sceAac_E955E83A(s32 *sampleRate)
{
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

    if (i=0, offset=0; i<g_nbr; i++, offset+=SCE_AAC_MEM_SIZE) {
        p = NULL;

        if (g_pool != NULL && g_nbr != 0) {
            if (i>=0 && i<g_nbr) {
                p = g_pool + offset;
            }
        }

        if (p->info.init == 0) {
            p->info.init = 1;
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
    p->info.unk48 = NULL;
    p->info.init = 2;
    p->codec.edramAddr = p + 200;
    p->codec.neededMem = 0x18F20;
    p->codec.unk20 = 1;
    p->codec.unk40 = *sampleRate;
    p->codec.unk45 = 0;
    p->codec.unk44 = 0;

    // sceAudiocodec_5B37EB1D
    ret = sceAudiocodecInit(&p->codec, 0x1003);
    if (ret < 0) {
        // sceAac_33B8C009
        sceAacExit(id);

        return ret;
    }

    p->info.init = 3;

    return id;
}

// sceAac_33B8C009
s32 sceAacExit(s32 id)
{
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

    if (p->info.init > 0) {
        p->info.init = 0;
    }

    sceKernelCpuResumeIntr(intr);

    return SCE_ERROR_OK;
}

// sceAac_7E4CFEE4
s32 sceAacDecode(s32 id, void** src)
{
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

    if (p->info.init < 3) {
        return 0x80691103;
    }

    if (src == NULL) {
        src = p->info.unk8;
    }
    else {
        *src = p->codec.outBuf;
        src = p->info.unk8;
    }

    if (src != NULL) {
        // Kernel_Library_A089ECA4
        sceKernelMemset(p->codec.outBuf, 0, p->info.unk52);

        p->info.unk56 ^= 1;
        p->codec.outBuf = p->info.unk48 + p->info.unk52 * p->info.unk56;

        return 0;
    }

    if ((p->info.unk20 == p->info.unk16 && p->info.unk44 > 0) ||
        (p->info.unk40 >= 1536)) {
        // sceAudiocodec_70A703F8
        if (sceAudiocodecDecode(&p->codec, 0x1003) < 0) {
            sub_000013B4(id);

            return 0x80691401;
        }

        p->info.unk40 -= p->codec.unk28;
        p->codec.inBuf += p->codec.unk28;
        p->info.unk44 -= p->codec.unk28;
        p->codec.outBuf = p->info.unk56 + p->info.unk52 * (p->info.unk56 ^ 1);
        p->info.unk60 += p->codec.unk36;
        p->info.unk56 ^= 1;

        sub_000013B4(id);

        return p->codec.unk36;
    }

    // Kernel_Library_A089ECA4
    sceKernelMemset(p->codec.outBuf, 0, p->info.unk52);

    p->info.unk56 ^= 1;
    p->codec.outBuf = p->info.unk48 + p->info.unk52 * p->info.unk56;

    return p->info.unk52;
}

// sceAac_FA01FCB6
s32 sceAac_FA01FCB6(s32 id, void *arg1, s32 *arg2, void *arg3, s32 *arg4)
{
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

    if (p->info.init < 3) {
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

    *arg2 = p->codec.unk28;
    *arg4 = p->codec.unk36;

    return SCE_ERROR_OK;
}

// sceAac_D7C51541
s32 sceAacCheckStreamDataNeeded(s32 id)
{
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

    if (p->info.init < 2) {
        return 0x80691102;
    }

    if (p->info.unk20 == p->info.unk16) {
        return 0;
    }

    return (p->info.unk36 < (p->info.unk42 + p->info.unk28 * (p->info.unk32 + 1) + 1600));
}

// sceAac_02098C69
s32 sceAacGetInfoToAddStreamData(s32 id, s32 *arg1, s32 *arg2, s32 *arg3)
{
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

    if (p->info.init < 2) {
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
            *arg3 = p->info.unk20;
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

    if (p->info.init < 2) {
        return 0x80691102;
    }

    if (size < 0 &&
        (p->info.unk24 + p->info.unk28 * (p->info.unk32 + 1) + 1600 - p->info.unk36) < size) {
        return 0x80691003;
    }

    if ((p->info.unk16 - p->info.unk20) < size) {
        p->info.unk20 = p->info.unk16;
        p->info.unk40 += p->info.unk16 - p->info.unk20;
        p->info.unk36 += p->info.unk16 - p->info.unk20;
    }
    else {
        p->info.unk40 += size;
        p->info.unk20 += size;
        p->info.unk36 += size;
    }

    sub_00000000(id);

    return SCE_ERROR_OK;
}

// sceAac_D2DA2BBA
s32 sceAacResetPlayPosition(s32 id)
{
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

    if (p->info.init < 3) {
        return 0x80691103;
    }

    p->inBuf = p->info.unk24 + 1600;
    p->info.unk44 = p->info.unk16 - p->info.unk12;
    p->info.unk36 = p->info.unk24 + 1600;
    p->info.unk8 = NULL;
    p->info.unk20 = p->info.unk12;
    p->info.unk40 = 0;
    p->info.unk60 = 0;
    p->info.unk32 = 1;

    return SCE_ERROR_OK;
}

// sceAac_BBDD6403
s32 sceAacSetLoopNum(s32 id, s32 loopNum)
{
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

    if (p->info.init < 2) {
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
    SceAacId *p;
    s32 ret;
    s32 size;

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

    if (p->info.init < 3) {
        return 0x80691103;
    }

    // sceAudiocodec_59176A0F
    ret = sceAudiocodecAlcExtendParameter(&p->codec, 0x1003, &size);
    if (ret < 0) {
        return ret;
    }

    return (u32)size >> 2;
}
