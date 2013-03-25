#define SCE_AAC_MEM_SIZE (1024 * 100) // 0x19000

SCE_MODULE_STOP("sceAacEndEntry");

typedef struct {
    s32 init;
    s32 unk4;
    s32 unk8;
    s32 unk12;
    s32 unk16;
    s32 unk20;
    s32 unk24;
    s32 unk28;
    s32 unk32;
    s32 unk36;
    s32 unk40;
    s32 unk44;
    s32 unk48;
    s32 unk52;
    s32 unk56;
    s32 unk60;
    s32 sampleRate; // 64
} Unk1;

// size 1024 * 100
typedef struct {
    s32 unk0;
    s32 unk4;
    s32 unk8;
    s32 unk12;
    s32 unk16;
    s32 unk20;
    s32 unk24;
    s32 unk28;
    s32 unk32;
    s32 unk36;
    s32 unk40;
    s8 unk44;
    s8 unk45;
    u8 undef46[82];
    Unk1 unk128;
} Unk0;

static s32 g_poolId = -1; // 1740
static Unk0 *g_pool; // 1750
static s32 g_nbr; // 1754

void sub_00000000(s32 idx)
{
    Unk0 *p;

    if (g_pool == NULL) {
        return;
    }

    if (g_nbr == 0) {
        return;
    }

    // ((idx < g_nbr) ^ 1) | (idx >> 31)
    if (idx < 0 || idx >= g_nbr) {
        return;
    }

    p = g_pool + idx * SCE_AAC_MEM_SIZE;

    if (p == NULL) {
        return;
    }

    if ((p->unk128.unk24 + 2 * p->unk128.unk28 - p->unk24 + 1600) < 1536) {
        sub_000000F8(idx, p->unk128.unk32);
    }

    if (p->unk128.unk28 < p->unk128.unk40) {
        return;
    }

    if (p->unk128.unk36 != (p->unk128.unk24 + p->unk128.unk28 * (p->unk128.unk32 + 1) + 1600) {
        return;
    }

    p->unk128.unk32 ^= 1;
    p->unk128.unk36 = p->unk128.unk24 + p->unk128.unk28 * p->unk128.unk32 + 1600;
}

void sub_000000F8(s32 idx) {
    Unk0 *p;
    void *dst;

    if (g_pool == NULL) {
        return;
    }

    if (g_nbr == 0) {
        return;
    }

    if (idx < 0 || idx >= g_nbr) {
        return;
    }

    p = g_pool + idx * SCE_AAC_MEM_SIZE;

    if (p == NULL) {
        return;
    }

    dst = p->unk128.unk24 - (p->unk128.unk24 + (p->unk128.unk28 << 1) - p->unk24);

    // Kernel_Library_1839852A
    sceKernelMemcpy(
        dst,
        p->unk24,
        p->unk128.unk24 + (p->unk128.unk28 << 1) - p->unk24 + 1600
    );

    p->unk24 = dst;
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
    Unk0 *p;

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

        if (p->unk128.init == 1) {
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
s32 sceAacInit(s32 *arg0)
{
    s32 intr;
    s32 i;
    s32 offset;
    Unk0 *p;
    s32 id;
    s32 ret;

    if (arg0 == NULL) {
        return 0x80691002;
    }

    if (arg0[4] == 0 || arg0[6] == 0) {
        return 0x80691002;
    }

    if (arg0[1] < 0) {
        return 0x80691003;
    }

    if (arg0[1] >= arg0[3]) {
        if (arg0[3] != arg0[1]) {
            return 0x80691003;
        }

        if ((u32)arg0[0] >= (u32)arg0[2]) {
            return 0x80691003;
        }
    }

    if (arg0[5] < 8192 || arg0[7] < 8192 || arg0[9] != 0) {
        return 0x80691003;
    }

    if (g_pool == NULL || g_nbr == 0) {
        return 0x80691503;
    }

    if (arg0[8] != 24000 && arg0[8] != 32000 && arg0[8] != 44100 && arg0[8] != 48000) {
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

        if (p->unk128.init == 0) {
            p->unk128.init = 1;
            id = i;
            break;
        }
    }

    sceKernelCpuResumeIntr(intr);

    if (id < 0) {
        return 0x80691201;
    }

    p->unk128.unk4 = -1;
    p->unk128.unk12 = arg0[0];
    p->unk128.unk20 = arg0[0];
    p->unk128.unk8 = 0;
    p->unk128.unk16 = arg0[2];
    p->unk128.unk32 = 1;
    p->unk128.unk28 = (arg0[5] - 1600 + ((arg0[5] - 1600) < 0)) >> 1; // wtf?
    p->unk128.unk44 = arg0[2] - arg[0];
    p->unk128.unk40 = 0;
    p->unk128.unk56 = 0;
    p->unk128.unk36 = arg0[4] + 1600;
    p->unk128.unk52 = (arg0[7] + (arg0[7] < 0)) >> 1;
    p->unk128.unk24 = arg0[4];
    p->unk128.unk60 = 0;

    p->unk0 = 2;

    p->unk128.sampleRate = arg0[8];
    p->unk128.unk48 = arg0[6];

    ret = sub_000012B8();
    if (ret < 0) {
        // sceAac_33B8C009
        sceAacExit(id);

        return ret;
    }

    p->unk0 = 3;

    return id;
}

// sceAac_E955E83A
s32 sceAac_E955E83A(s32 *sampleRate)
{
    s32 intr;
    s32 i;
    s32 offset;
    Unk0 *p;
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

        if (p->unk128.init == 0) {
            p->unk128.init = 1;
            id = i;
            break;
        }
    }

    sceKernelCpuResumeIntr(intr);

    if (id < 0) {
        return 0x80691201;
    }

    p->unk128.sampleRate = *sampleRate;
    p->unk128.unk24 = 0;
    p->unk128.unk48 = 0;
    p->unk128.init = 2;
    p->unk12 = p + 200;
    p->unk16 = 0x18F20;
    p->unk20 = 1;
    p->unk40 = *sampleRate;
    p->unk45 = 0;
    p->unk44 = 0;

    // sceAudiocodec_5B37EB1D
    ret = sceAudiocodecInit(p, 0x1003);
    if (ret < 0) {
        // sceAac_33B8C009
        sceAacExit(id);

        return ret;
    }

    p->unk128.init = 3;

    return id;
}

s32 sceAacExit(s32 idx)
{
    Unk0 *p;
    s32 intr;

    if (g_pool == NULL) {
        return 0x80691503;
    }

    if (g_nbr == 0) {
        return 0x80691503;
    }

    if (idx < 0 || idx >= g_nbr) {
        return 0x80691503;
    }

    p = g_pool + idx * SCE_AAC_MEM_SIZE;

    if (p == NULL) {
        return 0x80691503;
    }

    if (idx < 0 || idx >= g_nbr) { // again?
        return 0x80691001;
    }

    intr = sceKernelCpuSuspendIntr();

    if (p->unk128.init > 0) {
        p->unk128.init = 0;
    }

    sceKernelCpuResumeIntr(intr);

    return SCE_ERROR_OK;
}
