#include "../global.h"

#define IS_JUMP(op) ((op & 0xFC000000) == 0x0C000000)
#define JUMP(ref, addr) ((ref & 0xF0000000) | ((addr & 0x03FFFFFFF) << 2))

// 21F8
char g_pspbtcnfPath[] = "/kd/pspbtcnf.txt";

char g_220C[] = "SceInitFileName";
char g_221C[] = "SceInitUserParam";
char g_2230[] = "SceInitVSHParam";
char g_2240[] = "SceInitParamsfo";
char g_2250[] = "SceInitDiscImage";

char g_2294[] = "InitThreadEntry";

char g_22A4[] = "backup area";

typedef struct
{
    char *game;
    int **strcpy256;
    int numStrcpy256;
    int **wcscpy256;
    int numWcscpy256;
} SceJumpFixup;

int *g_2840[] = {(int*)0x089C07AC};
int *g_2844[] = {(int*)0x0888EA48, (int*)0x0888EA84, (int*)0x0888EAB4, (int*)0x0888EAF0, (int*)0x0888EB20, (int*)0x0888EB88, (int*)0x0888EBB8};
int *g_2860[] = {(int*)0x089C08F0};
int *g_2864[] = {(int*)0x0888EA40, (int*)0x0888EA7C, (int*)0x0888EAAC, (int*)0x0888EAE8, (int*)0x0888EB18, (int*)0x0888EB80, (int*)0x0888EBB0};
int *g_2880[] = {(int*)0x089C0928};
int *g_2884[] = {(int*)0x0888EA48, (int*)0x0888EA84, (int*)0x0888EAB4, (int*)0x0888EAF0, (int*)0x0888EB20, (int*)0x0888EB88, (int*)0x0888EBB8};

char g_ULUS10141[] = "ULUS10141";
char g_ULES00557[] = "ULES00557";
char g_ULES00558[] = "ULES00558";
char g_ULES00559[] = "ULES00559";
char g_ULES00560[] = "ULES00560";
char g_ULES00561[] = "ULES00561";
char g_ULES00562[] = "ULES00562";
char g_ULAS42082[] = "ULAS42082";
char g_ULKS46066[] = "ULKS46066";
char g_ULJM05213[] = "ULJM05213";

// 22E4
SceJumpFixup jumpFixups[] =
{
    {g_ULUS10141, g_2840, 1, g_2844, 7},
    {g_ULES00557, g_2840, 1, g_2844, 1},
    {g_ULES00558, g_2840, 1, g_2844, 1},
    {g_ULES00559, g_2840, 1, g_2844, 1},
    {g_ULES00560, g_2840, 1, g_2844, 1},
    {g_ULES00561, g_2840, 1, g_2844, 1},
    {g_ULES00562, g_2840, 1, g_2844, 1},
    {g_ULAS42082, g_2880, 1, g_2884, 1},
    {g_ULKS46066, g_2880, 1, g_2884, 1},
    {g_ULJM05213, g_2860, 1, g_2864, 1}
};

typedef struct
{
    int unk_0; // module filename?
    void *buf; // module buffer?
    int size; // PRX size?
    int unk_12;
    int attr;
    int unk_20;
    int argSize;
    int argPartId;
} SceLoadCoreBootModuleInfo;

typedef struct
{
    int addr;
    int size; // 4
    int attr; // 8
    SceUID partId; // 12
    int unk_16;
    int unk_20;
    int unk_24;
} SceLoadCoreProtectInfo;

typedef struct
{ 
    int membase; // 0
    int memsize; // 4
    int loadedMods; // 8
    int numMods; // 12
    SceLoadCoreBootModuleInfo *mods; // 16
    int unk_20;
    char unk_24;
    char unk_25[3]; // ?
    int numProts; // 28
    SceLoadCoreProtectInfo *prots; // 32
    int modProtId;
    int modArgProtId; // 40
    int unk_44;
    int unk_48; // 48
    int unk_52;
    char *configfile; // 56
    int unk_60;
    // the following wasn't in UTOPIA's structure
    int unk_64;
    int unk_68;
    int unk_72;
    int unk_76;
} SceLoadCoreBootInfo;

typedef struct
{
    int addr;
    int size;
} SceResetVectorInfo;

typedef struct
{
    int type;
    int addr;
    int addr2;
    SceResetVectorInfo resetVectorInfo;
    int unk20;
    int hasCfgFile; // 24
    int unk28, unk32, unk...
} SceInit;

SceInit g_init; // 28A0

// -------------------------------

// 0000
/* 'bzero' two times. Used for "reset vector", sets a buffer to 0.
 *
 * buf: The buffer to empty.
 * param: size The size of the buffer.
 */
void bzero2(int *buf, int size)
{
    int *curBuf = buf;
    // 000C
    do
    {
        *(curBuf++) = 0;
    } while (buf + size != curBuf);
    // 0018 -> WTF?
    curBuf = buf;
    do
    {
        *(curBuf++) = 0;
    } while (buf + size != curBuf);
    BREAK(0);
}

// 0028
void strcpy256(char *dst, char *src)
{
    int i;
    for (i = 255; i >= 0; i--)
    {
        char c = (i == 0) ? '\0' : *dst;
        dst++;
        *src = c;
        if (c == '\0')
            break;
        src++;
    }
}

// 0054
void wcscpy256(short *dst, short *src)
{
    int i;
    for (i = 255; i >= 0; i--)
    {
        short c = (i == 0) ? '\0' : *dst;
        dst++;
        *src = c;
        if (c == '\0')
            break;
        src++;
    }
}

void sub_0080()
{
    void *info = sceKernelGetGameInfo();
    int lastStrcpyOp, lastWcscpyOp;
    lastStrcpyOp = lastWcscpyOp = 0;
    if (info == NULL)
        return;
    // 00C4
    int j;
    for (j = 0; j < 10; j++)
    {
        if (strcmp(info + 68, jumpFixups[j].game) == 0)
        {
            // 00E8
            int i;
            for (i = 0; i < jumpFixups[j].numStrcpy256; i++)
            {
                int *ptr = jumpFixups[j].strcpy256[i];
                if (IS_JUMP(*ptr)) {
                    lastStrcpyOp = JUMP((int)ptr, *ptr);
                    *ptr = JUMP(0, 0x08800000);
                }
                // 0128
            }
            // 0140
            // 0150
            for (i = 0; i < jumpFixups[j].numWcscpy256; i++)
            {
                int *ptr = jumpFixups[j].wcscpy256[i];
                if (IS_JUMP(*ptr)) {
                    lastWcscpyOp = JUMP((int)ptr, *ptr);
                    *ptr = JUMP(0, 0x08800000 + wcscpy256 - strcpy256);
                }
                // 0194
            }
            // 01AC
        }
        // 01B0
    }
    if (lastWcscpyOp != 0 || lastStrcpyOp != 0) {
        // 01D0
        memcpy(0x08800000, strcpy256, (int)&sub_0080 - strcpy256);
    }
    // 01EC
    UtilsForKernel_79D1C3FA();
}

void sub_0218(SceLoadCoreBootInfo *bootInfo)
{
    char *src = bootInfo->configfile;
    if (src == NULL)
    {
        // 024C
        AT_SW(0, &g_init.hasCfgFile);
    }
    else {
        strncpy(&g_28A0 + 28, src, 256);
        AT_SW(1, &g_init.hasCfgFile);
    }
    // 0254
    *(int*)(g_28A0 + 92) = 0x100;
    *(int*)(g_28A0 + 96) = 1;
    AT_SW(-1, &g_28A0 + 296);
    AT_SW(0, &g_28A0 + 300);
    g_init.type = 0;
    *(int*)(g_28A0 + 104) = 0;
    *(int*)(g_28A0 + 108) = 0;
    *(int*)(g_28A0 + 124) = 0;
    AT_SW(32, &g_28A0 + 288);
    g_init.addr = 0;
    g_init.addr2 = 0;
    *(int*)(g_28A0 + 100) = 0;
    *(int*)(g_28A0 + 112) = 0;
    *(int*)(g_28A0 + 116) = 0;
    *(int*)(g_28A0 + 120) = 0;
    // 02BC
    int i;
    for (i = 0; i < 16; i++)
        AT_SBADD(0, &g_28A0 + 304, i);
}

int sub_02E0(int err)
{
    *(int*)(g_28A0 + 12) = 48;
    int blkId = InitForKernel_2C6E9FE9(0);
    void *ptr;
    if (blkId <= 0)
    {
        // 032C
        ptr = g_28A0 + 60;
        *(int*)(ptr + 0) = 32;
        *(int*)(ptr + 4) = 32; 
        *(int*)(ptr + 8) = 0;
        *(int*)(ptr + 16) = 0;
    }
    else
        ptr = sceKernelGetBlockHeadAddr(blkId);
    // 0348
    *(int*)(ptr + 24) = err;
    *(int*)(g_28A0 + 16) = *(int*)(ptr + 0);
    *(int*)(g_28A0 + 36) = g_pspbtcnfPath;
    *(int*)(g_28A0 + 32) = ptr;
    *(int*)(g_28A0 + 40) = 0;
    *(int*)(g_28A0 + 20) = ptr;
    *(int*)(g_28A0 + 28) = *(int*)(ptr + 0);
    g_init.hasCfgFile = 0;
    blkId = InitForKernel_2C6E9FE9(4);
    if (blkId <= 0)
    {
        // 03C0
        *(int*)(g_28A0 + 52) = 0;
        *(int*)(g_28A0 + 48) = 0;
    }
    else {
        *(int*)(g_28A0 + 48) = SysMemForKernel_CC31DEAD(blkId);
        *(int*)(g_28A0 + 52) = sceKernelGetBlockHeadAddr(blkId);
    }
    // 03C8
    sceKernelExitVSHKernel(g_28A0 + 12);
    return 0;
}

int sub_03F4()
{
    int partId = InitForKernel_2C6E9FE9();
    if (partId > 0)
    {
        int addr = sceKernelGetBlockHeadAddr(partId);
        int arg = *(int*)(addr + 20);
        if (arg != 0)
            sub_02E0(arg);
    }
    return 0;
}

void sub_0438()
{
    sceKernelSetSystemStatus(0x20000);
    if (*(int*)(g_28A0 + 96) != 0 && sceSuspendForKernel_3AEE7261(0) >= 0)
        *(int*)(g_28A0 + 96)--;
}

int sub_048C(int arg)
{
    int i, unk;
    int *ptr;
    if (arg >= 4)
    {
        // 04DC
        ptr = *(int*)(g_28A0 + 120);
        i = 4;
        unk = 5;
    }
    else
    {
        ptr = *(int*)(g_28A0 + 112);
        i = 0;
        unk = 4;
    }
    int starti = i;
    // 04EC
    // 04F0
    for (; i <= arg; i++)
    {
        int *curPtr = ptr;
        if ((arg == unk - 1) && (arg == i))
        {
            if (arg >= 4)
                *(int*)(g_28A0 + 120) = 0;
            else
                *(int*)(g_28A0 + 112) = 0;
        }
        // 0520
        // 0530
        while (*curPtr != 0)
        {
            int ptr = *curPtr;
            if (starti + (ptr & 3) == i)
            {
                int (*func)(char*, int, int) = (ptr & 0xFFFFFFFC);
                if (i != unk - 1)
                    func((char*)ptr, 1, 0);
                else
                    func((char*)curPtr + 2, 0, 0);
                // 0564
            }
            curPtr += 2;
            // 056C
        }
        // 057C
        if (i == arg - 1 && ptr < curPtr)
        {
            // 0590
            do
            {
                curPtr -= 2;
                if ((*curPtr & 3) + starti >= arg)
                    break;
                *curPtr = 0;
            } while (ptr < curPtr);
            // 05B4
        }
        // 05B8
    }
}

int sub_05F0()
{
    sub_048C(3);
    sub_0080();
    sceKernelSetDNAS(0);
    int oldIntr = sceKernelCpuSuspendIntr();
    if (sceKernelGetModel() >= 3)
    {
        switch (InitForKernel_7233B5BC())
        {
        case 256:
        case 272:
        case 768:
            UtilsForKernel_39FFB756(81);
            break;
    
        case 512:
        default:
            UtilsForKernel_39FFB756(0);
            break;
        }
    }
    // 0684
    sceKernelCpuResumeIntr(oldIntr);
    sub_0438();
    return 0;
}

int sub_06A8(SceLoadCoreBootInfo *bootInfo)
{
    if (g_init.addr2 > 0)
        return 0;
    AT_SW(g_init.addr2 + 1, &g_init.addr2);
    // 06F4
    int i;
    for (i = 0; i < bootInfo->numProts; i++)
    {
        SceLoadCoreProtectInfo *prot = &bootInfo->prots[i];
        if ((prot->attr & 0xFFFF) == 0x200 && (prot->attr & 0x10000) != 0) {
            sceKernelFreePartitionMemory(prot->partId);
            prot->attr &= 0xFFFEFFFF;
        }
        // (0734)
        // 0738
    }
    // 0744
    if (bootInfo->mods == NULL)
        return 0;
    int sp1, sp2[3];
    int ret = sceKernelQueryMemoryInfo(bootInfo->mods, &sp1, sp2);
    if (ret < 0)
        return ret;
    sceKernelFreePartitionMemory(sp2[0]);
    return 0;
}

int sub_0790(SceLoadCoreBootInfo *bootInfo)
{
    int i;
    for (i = 0; i < bootInfo->numProts; i++)
    {
        SceLoadCoreProtectInfo *prot = &bootInfo->prots[i];
        switch (prot->attr & 0xFFFF)
        {
        case 256:
        case 512:
            // (085C)
            // 0860
            if ((prot->addr & 0x10000) != 0) {
                sceKernelFreePartitionMemory(prot->partId);
                prot->attr &= 0xFFFEFFFF;
            }
            break;
    
        case 2:
            // 082C
            if ((prot->addr & 0x10000) != 0)
            {
                sceKernelFreePartitionMemory(prot->partId);
                prot->attr &= 0xFFFEFFFF;
                g_init.addr = 0;
            }
            break;
        }
        // (0880)
        // 0884
    }
    int unk, partId;
    // 0894
    if (bootInfo->prots != NULL)
    {
        int ret = sceKernelQueryMemoryInfo(bootInfo->prots, &unk, &partId);
        if (ret < 0)
            return ret;
        sceKernelFreePartitionMemory(partId);
    }
    // 08C0
    sceKernelQueryMemoryInfo(bootInfo, &unk, &partId);
    sceKernelFreePartitionMemory(partId);
    return 0;
}

void sub_08F8(SceLoadCoreBootInfo *bootInfo)
{
    int sp[16];
    // 092C
    int i;
    for (i = 0; i < bootInfo->numProts; i++)
    {
        SceLoadCoreProtectInfo *info = &bootInfo->prots[i];
        switch (info->attr)
        {
        case 0x2:
            // 09A8
            if (info->size != 0)
            {
                SceUID id = sceKernelAllocPartitionMemory(1, g_220C, 1, info->size, 0);
                if (id > 0)
                {
                    int addr = sceKernelGetBlockHeadAddr(id);
                    memmove(addr, info->addr, info->size);
                    info->addr = addr;
                    info->partId = id;
                    info->attr |= 0x10000;
                }
            }
            // (0A08)
            // 0A0C
            if (i == bootInfo->modProtId)
                g_init.addr = info->addr;
            break;
        case 0x4:
            // 0A8C
            if (info->size != 0)
            {
                SceUID id = sceKernelAllocPartitionMemory(1, g_2230, 1, info->size, 0);
                if (id > 0)
                {
                    sp[0] = 56;
                    sceKernelQueryMemoryBlockInfo(id, sp);
                    int addr = sp[10];
                    memmove(addr, info->addr, info->size);
                    info->addr = addr;
                    info->attr |= 0x10000;
                    info->partId = id;
                    *(int*)(addr + 4) = info->attr;
                    *(int*)(addr + 0) = sp[11];
                    *(int*)(addr + 24) = 0;
                    *(int*)(addr + 20) = 0;
                    sceKernelRegisterChunk(0, id);
                }
            }
            break;
        case 0x40:
            // 0BA0
            if (info->size != 0)
            {
                SceUID id = sceKernelAllocPartitionMemory(1, g_2250, 1, info->size, 0);
                if (id > 0)
                {
                    int addr = sceKernelGetBlockHeadAddr(id);
                    memmove(addr, info->addr, info->size);
                    info->addr = addr;
                    info->attr |= 0x10000;
                    info->partId = id;
                    sceKernelRegisterChunk(3, id);
                    g_init.addr2 = addr;
                }
            }
            break;
        case 0x80:
            // 0C1C
            if (info->size != 0 && *(int*)(info->addr + 0) == info->size)
                memcpy(&g_28A0 + 288, info->addr, 32);
            break;
        case 0x100:
            // 0A28
            if (info->size != 0)
            {
                SceUID id = sceKernelAllocPartitionMemory(1, g_221C, 1, info->size, 0);
                if (id > 0)
                {
                    int addr = sceKernelGetBlockHeadAddr(id);
                    memmove(addr, info->addr, info->size);
                    info->addr = addr;
                    info->attr |= 0x10000;
                    info->partId = id;
                }
            }
            break;
        case 0x400:
            // 0B20
            if (info->size != 0)
            {
                SceUID id = sceKernelAllocPartitionMemory(1, g_2240, 1, info->size, 0);
                if (id > 0)
                {
                    int addr = sceKernelGetBlockHeadAddr(id);
                    memmove(addr, info->addr, info->size);
                    info->addr = addr;
                    info->attr |= 0x10000;
                    info->partId = id;
                    sceKernelRegisterChunk(4, id);
                    *(int*)(g_28A0 + 100) = addr;
                    *(int*)(g_28A0 + 104) = info->size;
                }
            }
            break;
        }
        // (0C48)
        // 0C4C
    }
    // 0C5C
    if (sceKernelGetChunk(0) < 0)
    {
        SceUID id = sceKernelAllocPartitionMemory(1, g_2230, 1, 32, 0);
        if (id > 0)
        {
            sp[0] = 56;
            sceKernelQueryMemoryBlockInfo(id, sp);
            int addr = sp[10];
            *(int*)(addr + 4) = 32;
            *(int*)(addr + 0) = sp[11];
            *(int*)(addr + 24) = 0;
            *(int*)(addr + 8) = 0;
            *(int*)(addr + 12) = 0;
            *(int*)(addr + 16) = 0;
            *(int*)(addr + 20) = 0;
            sceKernelRegisterChunk(0, id);
        }
    }
}

int sub_0CFC(SceLoadCoreBootModuleInfo *mod)
{
    SceSysmemPartitionInfo partInfo;
    sceKernelQueryMemoryPartitionInfo(6, &partInfo);
    return (int)UCACHED(mod->buf + mod->size) >= partInfo.startAddr;
}

int sub_0D4C()
{
    sceKernelFillFreeBlock(1, 0);
    sceKernelFillFreeBlock(5, 0);
    sceSuspendForKernel_A569E425(0);
    int ret = sceKernelFillFreeBlock(2, 0);
    sceKernelMemoryExtendSize();
    sceKernelFillFreeBlock(8, 0);
    sceKernelFillFreeBlock(11, 0);
    sceKernelMemoryShrinkSize();
    sceKernelMemset32(0x80010000, 0, 0x4000);
    return ret;
}

int sub_0DD0(SceLoadCoreBootInfo *bootInfo)
{
    if (KDebugForKernel_FFD2F2B9() == 0 || bootInfo->unk_24 != 0)
        return 0;
    return (InitForKernel_7233B5BC() == 0x200);
}

int sub_0E1C(void *buf)
{
    void *curBuf = buf;
    void *argBuf = buf;
    int type = g_init.type;
    int ret;
    switch (type - 272) // TODO: switch table at 0x23B0 loaded with $AT
    {
    case 0: case 1: case 2: case 3: case 4: case 5:
        // 0E6C
        ret = sceKernelLoadModuleWithApitype2(type, argBuf, 0, 0);
        curBuf = NULL;
        break;
    case 6:
    case 8:
        // 0E84
        ret = ModuleMgrForKernel_30727524(argBuf, *(int*)(&g_28A0 + 296), *(int*)(&g_28A0 + 300), &g_28A0 + 304, 0, 0);
        curBuf = NULL;
        break;
    case 16:
        // 0EB8
        ret = sceKernelLoadModuleDisc(argBuf, 0, 0);
        break;
    case 17:
        // 0F0C
        ret = sceKernelLoadModuleDiscUpdater(type, argBuf, 0, 0);
        loc_0FFC();
        break;
    case 18:
        // 0F24
        if (sceKernelIsToolMode() == 0)
            ret = 0x800200D1;
        else
            ret = sceKernelLoadModuleDiscDebug(argBuf, 0, 0);
        break;
    case 19:
    case 21:
    case 96: case 97:
        // 0ED0
        ret = ModuleMgrForKernel_1B91F6EC(type, argBuf, 0, 0);
        curBuf = g_init.addr2;
        break;
    case 20:
    case 22:
        // 0EE8
        ret = ModuleMgrForKernel_C2A5E6CA(type, argBuf, 0, 0);
        curBuf = g_init.addr2;
        break;
    case 48:
    case 65:
        // 0F4C
        ret = sceKernelLoadModuleMs1(type, argBuf, 0, 0);
        break;
    case 49:
    case 66:
        // 0F64
        ret = sceKernelLoadModuleForLoadExecVSHMs2(type, argBuf, 0, 0);
        break;
    case 50:
    case 67:
        // 0F7C
        ret = sceKernelLoadModuleMs3(type, argBuf, 0, 0);
        break;
    case 51:
    case 68:
        // 0F94
        ret = sceKernelLoadModuleMs4(type, argBuf, 0, 0);
        break;
    case 52:
    case 69:
        // 0FAC
        ret = sceKernelLoadModuleForLoadExecVSHMs5(type, argBuf, 0, 0);
        break;
    case 53:
    case 70:
        // 0FC4
        ret = ModuleMgrForKernel_245B698D(type, argBuf, 0, 0);
        break;
    case 80: case 81:
        // 0FDC
        ret = ModuleMgrForKernel_8DD336D4(type, argBuf, 0, 0);
        break;

    default:
        // 0FF4
        ret = 0x800200D1;
        break;
    }
    // 0FFC
    if (ret >= 0 && curBuf != NULL)
        SysMemForKernel_BFE08689(curBuf);
    return ret;
}

int sub_1038(void *buf, int opt)
{
    int type = g_init.type;
    int ret;
    switch (type)
    {
    case 304: case 306:
        // (10CC)
        // 10D0
        ret = sceKernelLoadModuleBufferForLoadExecBufferVSHUsbWlan(type, buf, 0, 0);
        break;
    case 305: case 307:
        // 10E8
        if (sceKernelIsToolMode() == 0)
            ret = 0x800200D1;
        else
            ret = sceKernelLoadModuleBufferForLoadExecBufferVSHUsbWlanDebug(type, buf, 0, 0);
        break;
    case 512:
        // 111C
        ret = sceKernelLoadModuleBufferForExitVSHKernel(type, 0, 0, opt);
        break;
    case 528:
        // 1148
        ret = sceKernelLoadModuleBufferForExitGame(type, 0, 0, opt);
        break;
    case 544:
        // 1130
        ret = sceKernelLoadModuleBufferForExitVSHVSH(type, 0, 0, opt);
        break;
    case 768:
        // 1160
        ret = sceKernelLoadModuleBufferForRebootKernel(type, 0, 0, opt);
        break;
    default:
        // 1174
        ret = 0x800200D1;
        break;
    }
    // 1178
    SysMemForKernel_BFE08689(0);
    return ret;
}

void sub_1198(char *s, int line) // probably some masked debug thing
{
    int oldIntr = sceKernelCpuSuspendIntr();
    RESET_VECTOR(&g_init.resetVectorInfo, &g_init.addr, bzero2);
    sceKernelCpuResumeIntr(oldIntr);
    for (;;)
        ;
}

// 1240
int InitThreadEntry(int argSize, int args[2])
{
    char foundMod = 0;
    if (argSize < 8)
        return;
    SceLoadCoreBootInfo *bootInfo = args[1];
    sceKernelWaitThreadEnd(args[0]);
    if (KDebugForKernel_ACF427DC() != 0)
        printf("devkit version 0x%08x\n", 0x06060000);
    // 12B4
    if (bootInfo->unk_48 != 0)
        printf("build version 0x%08x\n");
    // 12CC
    int sp;
    // TODO: $sp is modified but $fp is backup'ed from $sp and $fp is used as the stack
    GET_REG(sp, SP);
    SET_REG(SP, (sp = sp - ALIGN_16_UP(bootInfo->numMods * sizeof(SceLoadCoreBootModuleInfo))));
    *(int*)sp = 0;
    *(int*)(g_28A0 + 112) = sp;
    *(int*)(g_28A0 + 116) = sp;
    SET_REG(SP, (sp = sp - ALIGN_16_UP(bootInfo->numMods * sizeof(SceLoadCoreBootModuleInfo))));
    *(int*)sp = 0;
    *(int*)(g_28A0 + 120) = sp;
    *(int*)(g_28A0 + 108) = bootInfo->unk_76;
    *(int*)(g_28A0 + 124) = bootInfo->unk_24;
    if (bootInfo->unk_24 == 0)
    {
        // 1380
        SceLoadCoreBootModuleInfo *mod = &bootInfo->mods[bootInfo->numMods - 1];
        if ((mod->attr & 2) != 0)
            g_init.type = mod->unk_20;
        // 13AC
        if (KDebugForKernel_ACF427DC() == 0)
        {
            // 13EC
            *(int*)(g_28A0 + 92) = 256;
        }
        else if (sceKernelDipsw(29) != 1)
            *(int*)(g_28A0 + 92) = 512; // 13E4 dup
        else {
            // 13D8/13E4 dup
            *(int*)(g_28A0 + 92) = 256;
        }
    }
    else
    {
        if (bootInfo->unk_24 >= 3)
        {
            // 13FC
            *(int*)(g_28A0 + 92) = 256;
            // 18EC dup
            sub_1198(g_2294, 2028); // this function never returns
        }
        g_init.type = bootInfo->mods[bootInfo->numMods - 1].unk_20;
        switch (bootInfo->mods[bootInfo->numMods - 1].unk_20 - 272) // TODO: jump table loaded using $AT??
        {
        case 0: case 1: case 2: case 3: case 4: case 5: case 6:
        case 8:
        case 16:
        case 18: case 19: case 20: case 21: case 22:
        case 32: case 33: case 34: case 35:
        case 49: case 50:
        case 53:
        case 66: case 67:
        case 70:
        case 80:
        case 96: case 97:
            // 13E0
            *(int*)(g_28A0 + 92) = 512;
            break;

        case 17:
        case 48:
        case 65:
            // 1368
            *(int*)(g_28A0 + 92) = 272;
            break;

        case 51:
        case 68:
            // 1370
            *(int*)(g_28A0 + 92) = 1024;
            break;

        case 52:
        case 69:
            // 1378
            *(int*)(g_28A0 + 92) = 768;
            break;

        default:
            // 13D8 dup
            *(int*)(g_28A0 + 92) = 256;
            break;
        }
    }
    // 1410
    int v = g_init.type - 276;
    // TODO: jump table using $AT??
    if (v ==  0 || v ==  1 || v ==  4 || v == 17
     || v == 30 || v == 31 || v == 62 || v == 63
     || v == 64 || v == 65 || v == 66 || v == 93)
    {
        // 1440
        SysMemForKernel_40B744A4(127);
    }
    else
    {
        // 1448
        SysMemForKernel_40B744A4(0);
    }
    // 144C
    if (bootInfo->loadedMods < bootInfo->numMods)
    {
        SceSysmemPartitionInfo partInfo;
        partInfo.size = 16;
        int curMod = bootInfo->numMods;
        sceKernelQueryMemoryPartitionInfo(5, &partInfo);
        int i;
        // 1498
        for (i = bootInfo->loadedMods; i < bootInfo->numMods; i++)
        {
            if ((bootInfo->mods[i].attr & 1) != 0) {
                curMod = i;
                break;
            }
        }
        // 14C0
        void *buf = UCACHED(partInfo.startAddr);
        // 14D8
        for (i = curMod; i < bootInfo->numMods; i++)
        {
            SceLoadCoreBootModuleInfo *mod = &bootInfo->mods[i];
            if (sub_0CFC(mod))
            {
                sceKernelMemmove(buf, mod->buf, mod->size);
                mod->buf = buf;
                buf = ALIGN_64_UP(buf + mod->size);
            }
        }
        // 152C
        for (i = bootInfo->numMods - 1; i >= 0; i--)
        {
            SceLoadCoreBootModuleInfo *mod = &bootInfo->mods[i];
            if ((mod->attr & 2) != 0)
            {
                int protId = bootInfo->modProtId;
                foundMod = 1;
                if (protId != -1)
                {
                    SceLoadCoreProtectInfo *prot = &bootInfo->prots[protId];
                    mod->size = prot->size;
                    mod->buf = prot->addr;
                    if ((prot->attr & 2) != 0)
                    {
                        mod->attr |= 4;
                        g_init.addr = prot->addr;
                    }
                }
                // 15AC
                protId = bootInfo->modArgProtId;
                if (protId != -1)
                {
                    SceLoadCoreProtectInfo *prot = &bootInfo->prots[protId];
                    mod->argPartId = prot->partId;
                    mod->argSize   = prot->size;
                }
            }
            // 15DC
            if (sub_0CFC(mod) == 0)
                mod->unk_20 = 0;
            else
            {
                SceUID id = sceKernelAllocPartitionMemory(2, g_22A4, 1, mod->size, 0);
                int addr = sceKernelGetBlockHeadAddr(id);
                sceKernelMemmoveWithFill(addr, mod->buf, mod->size, 0);
                mod->unk_20 = id;
                mod->buf = addr;
            }
            // 1634
        }
        // 1648
        for (i = bootInfo->loadedMods; i < bootInfo->numMods; i++)
        {
            if (i == curMod)
            {
                if (*(int*)(g_28A0 + 92) != 512)
                {
                    // trollol my name is Sony and I love making my code look like enigmas; "encrypted" string for "flash2:/opnssmp.bin"
                    int str[5] = {0x8C9E9399, 0xD0C5CD97, 0x8C918F90, 0xD18F928C, 0xFF91969D};
                    int j;
                    for (j = 0; j < 20; j++)
                        ((char*)str)[j] = ~((char*)str)[j];
                    int argp = *(int*)(sceKernelGetGameInfo() + 216);
                    int status;
                    SceIoStat stat;
                    if (argp != 0 && sceIoGetstat(str, &stat) >= 0)
                    {
                        char started = 0;
                        SceUID id = sceKernelLoadModule(str, 0, 0);
                        if (id >= 0)
                        {
                            if (sceKernelStartModule(id, 4, &argp, &status, 0) >= 0)
                                started = 1;
                            else
                                sceKernelUnloadModule(id);
                        }
                        if (!started) {
                            // 173C
                            sceIoRemove(str);
                        }
                    }
                }
            }
            // 1744
            // 1748
            SceLoadCoreBootModuleInfo *mod = &bootInfo->mods[i];
            if (mod->unk_20 != 0)
                sceKernelFreePartitionMemory(mod->unk_20);
            // 176C
            SceUID modId;
            if ((mod->attr & 2) == 0)
            {
                // 1824
                char unkStruct[18]; // TODO
                ((int *)unkStruct)[ 0] = 20;
                ((int *)unkStruct)[ 2] = (mod->attr >> 16) & 0xFF;
                ((char*)unkStruct)[17] = 1;
                ((int *)unkStruct)[ 1] = (mod->attr >> 16) & 0xFF;
                ((char*)unkStruct)[16] = 0;
                if ((mod->attr & 4) == 0)
                {
                    // 1868
                    modId = ModuleMgrForKernel_EF7A7F02(mod->size, mod->buf, 0, unkStruct, (mod->attr >> 8) & 1);
                    if (modId > 0)
                        sceKernelMemset32(mod->buf, 0, mod->size);
                }
                else
                    modId = ModuleMgrForKernel_25E1F458(mod->buf, 0);
            }
            else
            {
                SceLoadCoreBootModuleInfo *mod2 = *(int*)(&g_28A0 + 320);
                memmove(mod2, mod, 32);
                mod = mod2;
                sub_06A8(bootInfo);
                sub_03F4();
                if (sub_0DD0(bootInfo) == 0)
                {
                    if (((g_28A0 + 320 + 16) & 4) != 0)
                        modId = sub_0E1C(g_28A0 + 320 + 4);
                    else
                    {
                        // 17DC
                        modId = sub_1038(g_28A0 + 320 + 4, (mod->attr >> 8) & 1);
                        if (modId > 0)
                            sceKernelMemset32(g_28A0 + 320 + 4, 0, g_28A0 + 320 + 8);
                    }
                    // 180C
                    if (modId < 0)
                        sub_02E0(modId);
                }
                else
                    modId = 0x80020001;
            }
            // (1898)
            // 189C
            if ((mod->attr & 2) != 0) {
                sub_0D4C();
                sub_05F0();
            }
            // 18B8
            if (modId >= 0)
            {
                int ret, status;
                // 18F4
                if (mod->argPartId == 0 || mod->argSize == 0) {
                    // 1920
                    ret = sceKernelStartModule(modId, 0, 0, &status, 0);
                }
                else
                    ret = sceKernelStartModule(modId, mod->argSize, sceKernelGetBlockHeadAddr(mod->argPartId), &status, 0);
                // 1928
                if (ret < 0 && (mod->attr & 2) != 0)
                    sub_02E0(ret);
                // 1954
            }
            else if ((mod->attr & 2) == 0 || sub_0DD0(bootInfo) == 0)
            {
                // 18E0
                // 18EC dup
                sub_1198(g_2294, 2313); // this function never returns
            }
            // 1958
            if (mod->argPartId != 0)
                sceKernelFreePartitionMemory(mod->argPartId);
            // 196C
            bootInfo->loadedMods++;
            if ((mod->attr & 2) != 0)
                break;
        }
    }
    // 1994
    sub_06A8(bootInfo);
    sub_0790(bootInfo);
    if (foundMod == 0)
        sub_0D4C();
    // 19B4
    printf("Loading all modules ... Ready\n");
    if (sceKernelGetSystemStatus() == 0)
        sub_05F0();
    // 19D8
    if (foundMod == 1)
        sub_048C(4);
    // 19E8
    int *ptr = KDebugForKernel_B7251823();
    if (ptr != NULL)
    {
        void (*func)(int) = *(int*)(ptr + 8);
        func(2);
    }
    // 1A08
    sceKernelStopUnloadSelfModuleWithStatusKernel(1, 0, 0, 0, 0);
}

/* The module start function. */
int module_bootstart(int unused, SceLoadCoreBootInfo *bootInfo)
{
    AT_SW(bootInfo->membase, &g_init.resetVectorInfo.addr);
    AT_SW(bootInfo->memsize, &g_init.resetVectorInfo.size);
    AT_SW(InitForKernel_040C934B(), g_28A0);
    sub_0218(bootInfo);
    int threadId = sceKernelGetThreadId();
    if (threadId < 0)
        return threadId;
    sub_08F8(bootInfo);
    SceLoadCoreBootModuleInfo *mods = bootInfo->mods;
    if (bootInfo->unk_24 == 0 && (u32)mods->buf <= 0x88FFFFFF)
    {
        void *minStart = (void*)-1;
        void *maxEnd = NULL;
        int i;
        // 1AE4
        for (i = 0; i < bootInfo->numMods; i++)
        {
            void *start = mods[i].buf;
            void *end = start + mods[i].size;
            if (maxEnd < end)
                maxEnd = end;
            if (start < minStart)
                minStart = start;
        }
        // 1B0C
        AT_ADDI(minStart, 0x77A00000);
        sceKernelMemset32(0x88600000, 0, minStart);
        sceKernelMemset32(maxEnd, 0, 0x88800000 - (int)maxEnd);
    }
    else {
        // 1B38
        sceKernelMemset32(0x88600000, 0, 0x200000);
    }
    // 1B40
    SceUID id = sceKernelCreateThread("SceKernelInitThread", InitThreadEntry, 32, 0x4000, 0, 0);
    int sp[2];
    sp[0] = threadId;
    sp[1] = bootInfo;
    return sceKernelStartThread(id, 8, sp);
}

