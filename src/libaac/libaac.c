#define SCE_AAC_MEM_SIZE (1024 * 100) // 0x19000

SCE_MODULE_STOP("sceAacEndEntry");

typedef struct {
    SceBool init;
    u8 unk0[20];
    s32 unk24;
    s32 unk28;
    s32 unk32;
    s32 unk36;
    s32 unk40;
} Unk1;

// size 1024 * 100
typedef struct {
    u8 unk0[24];
    s32 unk24;
    u8 unk28[100];
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

void sub_000000F8(void) {

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

        if (g_pool != 0 && g_nbr != 0) {
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
