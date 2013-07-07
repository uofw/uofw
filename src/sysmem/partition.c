#include <sysmem_kdebug.h>
#include <sysmem_kernel.h>

#include "intr.h"
#include "memory.h"

#include "partition.h"

s32 _CreateMemoryPartition(SceSysmemMemoryPartition *part, u32 attr, u32 addr, u32 size);

s32 partition_do_initialize(SceSysmemUidCB *uid, SceSysmemUidCB *uidWithFunc, int funcId, va_list ap);
s32 partition_do_delete(SceSysmemUidCB *uid, SceSysmemUidCB *uidWithFunc, int funcId, va_list ap);
s32 partition_do_resize(SceSysmemUidCB *uid, SceSysmemUidCB *uidWithFunc, int funcId, va_list ap);
s32 partition_do_querypartinfo(SceSysmemUidCB *uid, SceSysmemUidCB *uidWithFunc, int funcId, va_list ap);
s32 partition_do_maxfreememsize(SceSysmemUidCB *uid, SceSysmemUidCB *uidWithFunc, int funcId, va_list ap);

// 145A8
SceSysmemUidCB *g_145A8;

// 145C0
SceSysmemMemInfo g_145C0;

SceSysmemMemoryPartition *MpidToCB(int mpid)
{
    SceSysmemUidCB *uid;
    switch (mpid) // jump table at 0x000134E0
    {
    case 0:
        return NULL;
    case 1:
        // 3D74
        return g_145C0.kernel;
    case 2:
        // 3D80
        return g_145C0.user;
    case 3:
        // 3D8C
        return g_145C0.other1;
    case 4:
        // 3D98
        return g_145C0.other2;
    case 5:
        // 3DA4
        return g_145C0.vshell;
    case 6:
        // 3DB0
        return g_145C0.scUser;
    case 7:
        // 3DBC
        return g_145C0.meUser;
    case 8:
        // 3DC8
        return g_145C0.extScKernel;
    case 9:
        // 3DD4
        return g_145C0.extSc2Kernel;
    case 10:
        // 3DE0
        return g_145C0.extMeKernel;
    case 11:
        // 3DEC
        return g_145C0.extVshell;
    case 12:
        // 3DF8
        return g_145C0.extKernel;
    default:
        // 3E04
        if (sceKernelGetUIDcontrolBlockWithType(mpid, g_145A8, &uid) != 0)
            return 0;
        return UID_CB_TO_DATA(uid, g_145A8, SceSysmemMemoryPartition);
    }
}

s32 CanoniMpid(s32 mpid)
{
    SceSysmemMemoryPartition *part = MpidToCB(mpid);
    if (part == NULL)
        return 0;
    else if (part == g_145C0.kernel)
        return 1;
    else if (part == g_145C0.vshell)
        return 5;
    else if (part == g_145C0.user)
        return 2;
    else if (part == g_145C0.extKernel)
        return 12;
    else if (part == g_145C0.other1)
        return 3;
    else if (part == g_145C0.other2)
        return 4;
    else if (part == g_145C0.scUser)
        return 6;
    else if (part == g_145C0.meUser)
        return 7;
    else if (part == g_145C0.extScKernel)
        return 8;
    else if (part == g_145C0.extSc2Kernel)
        return 9;
    else if (part == g_145C0.extMeKernel)
        return 10;
    else if (part == g_145C0.extVshell)
        return 11;
    else
        return mpid;
}

SceSysmemMemoryPartition *AddrToCB(u32 addr)
{
    SceSysmemMemoryPartition *cur = g_145C0.main;
    addr &= 0x1FFFFFFF;
    // 3F10
    while (cur != NULL) {
        if (addr >= (cur->addr & 0x1FFFFFFF)
         && addr < ((cur->addr + cur->size) & 0x1FFFFFFF))
            return cur;
        // 3F48
        cur = cur->next;
    }
    return NULL;
}

/*
0x000134B0 - D9 D2 10 D3 | 88 46 00 00 | 63 98 08 87 | E0 46 00 00 - .....F..c....F..
0x000134C0 - 36 01 0E BB | BC 47 00 00 | A0 04 BA E6 | C4 47 00 00 - 6....G.......G..
0x000134D0 - 29 1D 7A 6D | CC 47 00 00 | 00 00 00 00 | 00 00 00 00 - ).zm.G..........
*/

// 134B0
SceSysmemUidLookupFunc PartFuncs[] = {
    { 0xD310D2D9, partition_do_initialize },
    { 0x87089863, partition_do_delete },
    { 0xBB0E0136, partition_do_resize },
    { 0xE6BA04A0, partition_do_querypartinfo},
    { 0x6D7A1D29, partition_do_maxfreememsize },
    { 0, NULL }
};

void PartitionServiceInit(void)
{
    sceKernelCreateUIDtype("SceSysmemMemoryPartition", sizeof(SceSysmemMemoryPartition), PartFuncs, 0, &g_145A8);
}

s32 sceKernelCreateMemoryPartition(const char *name, u32 attr, u32 addr, u32 size)
{
    s32 oldIntr = suspendIntr();
    SceSysmemUidCB *uid;
    s32 ret = sceKernelCreateUID(g_145A8, name, (pspGetK1() >> 31) & 0xFF, &uid);
    if (ret != 0) {
        suspendIntr(oldIntr);
        return ret;
    }
    // 402C
    SceSysmemMemoryPartition *part = UID_CB_TO_DATA(uid, g_145A8, SceSysmemMemoryPartition);
    ret = _CreateMemoryPartition(part, attr, addr, size);
    if (ret != 0) {
        // 40E0
        sceKernelDeleteUID(uid->uid);
        resumeIntr(oldIntr);
        return 0x800200D5;
    }
    if (size != 0 && part->size != 0) {
        SceSysmemCtlBlk *ctlBlk = part->firstCtlBlk;
        // 4078
        InitSmemCtlBlk(ctlBlk);
        ctlBlk->segs[0].used = 0;
        ctlBlk->segs[0].next = 0x3F;
        ctlBlk->segs[0].offset = ret;
        ctlBlk->segs[0].unk0_0 = 0;
        ctlBlk->segs[0].sizeLocked = 0;
        ctlBlk->segs[0].prev = 0x3F;
        ctlBlk->segs[0].size = part->size >> 8;
        ctlBlk->firstSeg = 0;
        ctlBlk->lastSeg = 0;
        ctlBlk->segCount = 1;
        ctlBlk->prev = NULL;
        part->unk24 = 1;
        ctlBlk->freeSeg = 1;
        ctlBlk->unk10 = 0;
        ctlBlk->next = NULL;
    }
    // 4064
    resumeIntr(oldIntr);
    return uid->uid;
}

s32 sceKernelQueryMemoryPartitionInfo(s32 mpid, SceSysmemPartitionInfo *info)
{
    SceSysmemMemoryPartition *part = MpidToCB(mpid);
    if (part == NULL || info == NULL)
        return 0x800200D2;
    s32 oldIntr = suspendIntr();
    if (info->size != 16) {
        // 419C
        resumeIntr(oldIntr);
        return 0x800200D2;
    }
    /* Note: there was unused $sp storing here, probably unoptimized crap */
    info->attr = part->attr & 0xF;
    info->startAddr = part->addr;
    info->memSize = part->size;
    resumeIntr(oldIntr);
    return 0;
}

void PartitionInit(SceSysmemMemoryPartition *part)
{
    if (part->size != 0) {
        SceSysmemCtlBlk *ctlBlk = part->firstCtlBlk;
        InitSmemCtlBlk(ctlBlk);
        ctlBlk->segs[0].used = 0;
        ctlBlk->segs[0].next = 0x3F;
        ctlBlk->segs[0].offset = 0;
        ctlBlk->segs[0].unk0_0 = 0;
        ctlBlk->segs[0].sizeLocked = 0;
        ctlBlk->segs[0].prev = 0x3F;
        ctlBlk->segs[0].size = part->size >> 8;
        ctlBlk->firstSeg = 0;
        ctlBlk->lastSeg = 0;
        ctlBlk->segCount = 1;
        ctlBlk->prev = NULL;
        part->unk24 = 1;
        ctlBlk->freeSeg = 1;
        ctlBlk->unk10 = 0;
        ctlBlk->next = NULL;
    }
}

u32 PartitionQueryMaxFreeMemSize(SceSysmemMemoryPartition *part)
{
    SceSysmemCtlBlk *curCtlBlk = part->firstCtlBlk;
    u32 maxSize = 0;
    // 4250
    while (curCtlBlk != NULL) {
        SceSysmemSeg *curSeg = &curCtlBlk->segs[curCtlBlk->firstSeg];
        u32 i;
        // 426C
        for (i = 0; i < curCtlBlk->segCount; i++) {
            if (curSeg->used == 0 && maxSize < curSeg->size)
                maxSize = curSeg->size >> 9;
            // 429C
            curSeg = &curCtlBlk->segs[curSeg->next];
        }
        // 42A4
        curCtlBlk = curCtlBlk->next;
    }
    // 42B0
    return maxSize << 8;
}

u32 sceKernelPartitionMaxFreeMemSize(s32 mpid)
{
    s32 oldIntr = suspendIntr();
    SceSysmemMemoryPartition *part = MpidToCB(mpid);
    SceSysmemCtlBlk *curCtlBlk = part->firstCtlBlk;
    u32 maxSize = 0;
    // 42EC
    while (curCtlBlk != NULL) {
        SceSysmemSeg *curSeg = &curCtlBlk->segs[curCtlBlk->firstSeg];
        u32 i;
        // 4308
        for (i = 0; i < curCtlBlk->segCount; i++) {
            if (curSeg->used == 0 && maxSize < curSeg->size)
                maxSize = curSeg->size;
            // 4338
            curSeg = &curCtlBlk->segs[curSeg->next];
        }
        // 4340
        curCtlBlk = curCtlBlk->next;
    }
    // 434C
    resumeIntr(oldIntr);
    return maxSize << 8;
}

u32 sceKernelPartitionMaxFreeMemSizeForUser(void)
{
    s32 oldK1 = pspShiftK1();
    u32 ret = sceKernelPartitionMaxFreeMemSize(2);
    pspSetK1(oldK1);
    return ret;
}

u32 sceKernelPartitionTotalMemSize(s32 mpid)
{
    SceSysmemMemoryPartition *part = MpidToCB(mpid);
    if (part == NULL)
        return 0x800200D6;
    return part->size;
}

u32 sceKernelTotalMemSize(void)
{
    s32 oldK1 = pspShiftK1();
    SceSysmemMemoryPartition *part = MpidToCB(2);
    pspSetK1(oldK1);
    if (part == NULL)
        return 0x800200D6;
    return part->size;
}

u32 PartitionQueryTotalFreeMemSize(SceSysmemMemoryPartition *part)
{
    SceSysmemCtlBlk *curCtlBlk = part->firstCtlBlk;
    u32 size = 0;
    // 4424
    while (curCtlBlk != NULL) {
        SceSysmemSeg *curSeg = &curCtlBlk->segs[curCtlBlk->firstSeg];
        u32 i;
        // 4440
        for (i = 0; i < curCtlBlk->segCount; i++) {
            if (curSeg->used == 0)
                size += curSeg->size;
            // 446C
            curSeg = &curCtlBlk->segs[curSeg->next];
        }
        // 4474
        curCtlBlk = curCtlBlk->next;
    }
    // 4480
    return size << 8;
}

u32 sceKernelPartitionTotalFreeMemSize(s32 mpid)
{
    s32 oldIntr = suspendIntr();
    SceSysmemCtlBlk *curCtlBlk = MpidToCB(mpid)->firstCtlBlk;
    u32 size = 0;
    // 44BC
    while (curCtlBlk != NULL) {
        SceSysmemSeg *curSeg = &curCtlBlk->segs[curCtlBlk->firstSeg];
        u32 i;
        // 44D8
        for (i = 0; i < curCtlBlk->segCount; i++) {
            if (curSeg->used == 0)
                size += curSeg->size;
            // 4504
            curSeg = &curCtlBlk->segs[curSeg->next];
        }
        curCtlBlk = curCtlBlk->next;
    }
    // 4518
    resumeIntr(oldIntr);
    return size << 8;
}

u32 sceKernelPartitionTotalFreeMemSizeForUser(void)
{
    s32 oldK1 = pspShiftK1();
    u32 ret = sceKernelPartitionTotalFreeMemSize(2);
    pspSetK1(oldK1);
    return ret;
}

s32 sceKernelFillFreeBlock(s32 mpid, u32 c)
{
    SceSysmemMemoryPartition *part = MpidToCB(mpid);
    if (part == NULL)
        return 0x80020001;
    s32 oldIntr = suspendIntr();
    SceSysmemCtlBlk *curCtlBlk = part->firstCtlBlk;
    // 45BC
    while (curCtlBlk != NULL) {
        SceSysmemSeg *curSeg = &curCtlBlk->segs[curCtlBlk->firstSeg];
        u32 i;
        // 45E8
        for (i = 0; i < curCtlBlk->segCount && curCtlBlk->segCount != curCtlBlk->unk10; i++) {
            if (curSeg->used == 0) {
                // 4664
                sceKernelFillBlock64((void*)part->addr + (curSeg->offset << 8), c, curSeg->size << 8);
            }
            // 4600
            curSeg = &curCtlBlk->segs[curSeg->next];
        }
        // 4628
        curCtlBlk = curCtlBlk->next;
        // (462C)
    }
    // 4634
    resumeIntr(oldIntr);
    return 0;
}

s32 partition_do_initialize(SceSysmemUidCB *uid, SceSysmemUidCB *uidWithFunc, int funcId, va_list ap)
{
    sceKernelCallUIDObjCommonFunction(uid, uidWithFunc, funcId, ap);
    SceSysmemMemoryPartition *part = UID_CB_TO_DATA(uid, g_145A8, SceSysmemMemoryPartition);
    part->size = 0;
    part->attr &= 0xFFFFFFF0;
    part->firstCtlBlk = NULL;
    part->next = NULL;
    part->addr = 0;
    return uid->uid;
}

s32 partition_do_delete(SceSysmemUidCB *uid, SceSysmemUidCB *uidWithFunc, int funcId, va_list ap)
{
    SceSysmemMemoryPartition *part = UID_CB_TO_DATA(uid, g_145A8, SceSysmemMemoryPartition);
    if (part == MpidToCB(1))
        return 0x800200D7;
    if (part->firstCtlBlk->unk10 != 0)
        return 0x800200D7;
    SceSysmemMemoryPartition *cur = g_145C0.main;
    // 4750
    for (;;) {
        if (cur == NULL)
            return 0x80020001;
        if (cur->next == part)
            break;
        cur = cur->next;
    }
    // 476C
    cur->next = part->next;
    _FreePartitionMemory(part->firstCtlBlk);
    sceKernelCallUIDObjCommonFunction(uid, uidWithFunc, funcId, ap);
    return uid->uid;
}

s32 partition_do_resize(SceSysmemUidCB *uid, SceSysmemUidCB *uidWithFunc __attribute__((unused)), int funcId __attribute__((unused)), va_list ap __attribute__((unused)))
{
    return uid->uid;
}

s32 partition_do_querypartinfo(SceSysmemUidCB *uid, SceSysmemUidCB *uidWithFunc __attribute__((unused)), int funcId __attribute__((unused)), va_list ap __attribute__((unused)))
{
    return uid->uid;
}

s32 partition_do_maxfreememsize(SceSysmemUidCB *uid, SceSysmemUidCB *uidWithFunc __attribute__((unused)), int funcId __attribute__((unused)), va_list ap)
{
    s32 *size = va_arg(ap, s32*);
    SceSysmemMemoryPartition *part = UID_CB_TO_DATA(uid, g_145A8, SceSysmemMemoryPartition);
    SceSysmemCtlBlk *curCtlBlk = part->firstCtlBlk;
    u32 maxSize = 0;
    // 47F4
    while (curCtlBlk != NULL) {
        SceSysmemSeg *curSeg = &curCtlBlk->segs[curCtlBlk->firstSeg];
        u32 i;
        // 4810
        for (i = 0; i < curCtlBlk->segCount; i++) {
            if (curSeg->used == 0 && maxSize < curSeg->size)
                maxSize = curSeg->size;
            // 4840
            curSeg = &curCtlBlk->segs[curSeg->next];
        }
        // 4848
        curCtlBlk = curCtlBlk->next;
    }
    // 4854
    *size = maxSize << 8;
    return uid->uid;
}

s32 _CreateMemoryPartition(SceSysmemMemoryPartition *part, u32 attr, u32 addr, u32 size)
{
    u32 physAddr = addr & 0x1FFFFFFF;
    u32 startAddr = g_145C0.memAddr & 0x1FFFFFFF;
    if (part->size != 0) {
        if (physAddr < startAddr || physAddr + size > startAddr + g_145C0.memSize)
            return 0x800200D4;
        SceSysmemMemoryPartition *cur = g_145C0.main;
        // 48D4
        // 48E0
        while (cur != NULL) {
            u32 curAddr = cur->addr & 0x1FFFFFFF;
            if ((physAddr > curAddr && physAddr < curAddr + cur->size) || // start of the partition in another partition
                (physAddr + size > curAddr && physAddr + size < curAddr + cur->size)) // end of the partition in another partition
                return 0x800200D5;
            cur = cur->next;
            // 4924
        }
    }
    // 492C
    // 4930
    part->addr = addr;
    part->attr = (part->attr & 0xFFFFFFF0) | (attr & 0xF);
    part->next = NULL;
    part->size = size;
    part->unk24 = 0;
    if (size != 0) {
        SceSysmemMemoryPartition *prev = g_145C0.main;
        SceSysmemMemoryPartition *cur = prev->next;
        while (cur != NULL) { // 4960
            prev = cur;
            cur = cur->next;
        }
        prev->next = part;
    }
    // 4974
    SceSysmemCtlBlk *ctlBlk = sceKernelGetBlockHeadAddr(sceKernelAllocPartitionMemory(1, "SceSystemBlock", 3, 0xFF, 0xFF));
    part->lastCtlBlk = ctlBlk;
    part->firstCtlBlk = ctlBlk;
    return 0;
}

SceSysmemSeg *AddrToSeg(SceSysmemMemoryPartition *part, void *addr)
{
    s32 blockOff = ((u32)addr & 0x1FFFFFFF) - (part->addr & 0x1FFFFFFF);
    if ((u32)addr < part->addr || (u32)addr > part->addr + part->size)
        return NULL;
    SceSysmemCtlBlk *curCtlBlk = part->firstCtlBlk;
    // 49FC
    while (curCtlBlk != NULL) {
        SceSysmemSeg *curSeg = &curCtlBlk->segs[curCtlBlk->firstSeg];
        SceSysmemSeg *lastSeg = &curCtlBlk->segs[curCtlBlk->lastSeg];
        if (blockOff >= (s32)curSeg->offset && blockOff < (s32)lastSeg->offset + lastSeg->size) {
            u32 i;
            for (i = 0; i < curCtlBlk->segCount; i++) { // 4A58
                if (blockOff >= (s32)curSeg->offset && blockOff < (s32)curSeg->offset + curSeg->size)
                    return curSeg;
                // 4A94
                curSeg = &curCtlBlk->segs[curSeg->next];
            }
        }
        // (4A9C)
        curCtlBlk = curCtlBlk->next;
        // 4AA0
    }
    return NULL;
}

SceUID sceKernelAllocPartitionMemory(s32 mpid, char *name, u32 type, u32 size, u32 addr)
{
    if (type > 4)
        return 0x800200D8;
    if ((type == 3 || type == 4) &&
        (addr == 0 || (addr & (addr - 1)) != 0))
        return 0x800200E4;
    // 4B64
    s32 oldIntr = suspendIntr();
    SceSysmemMemoryPartition *part = MpidToCB(mpid);
    if (part == NULL) {
        // 4CAC
        resumeIntr(oldIntr);
        return 0x800200D6;
    }
    u8 flag = 0xFF;
    if (!pspK1IsUserMode())
        flag = 0;
    SceSysmemUidCB *uid;
    s32 ret = sceKernelCreateUID(g_MemBlockType, name, (flag | pspK1IsUserMode()) & 0xFF, &uid);
    if (ret != 0) {
        // 4C9C
        resumeIntr(oldIntr);
        return ret;
    }
    SceSysmemMemoryBlock *memBlock = UID_CB_TO_DATA(uid, g_MemBlockType, SceSysmemMemoryBlock);
    void *outAddr;
    if (type == 2 && (addr & 0xFF) != 0) { // 4C70
        outAddr = _allocSysMemory(part, type, ((addr + size + 0xFF) & 0xFFFFFF00) - (addr & 0xFFFFFF00), addr & 0xFFFFFF00, 0);
    } else
        outAddr = _allocSysMemory(part, type, size, addr, 0);
    // 4BDC
    if (outAddr == NULL) {
        // 4C14
        Kprintf("system memory allocation failed\n");
        Kprintf("\tmpid 0x%08x, name [%s], request size 0x%x\n", mpid, name, size);
        Kprintf("\tmax size 0x%x\n", PartitionQueryMaxFreeMemSize(part));
        sceKernelDeleteUID(uid->uid);
        resumeIntr(oldIntr);
        return 0x800200D9;
    }
    memBlock->size = (size + 0xFF) & 0xFFFFFF00;
    memBlock->part = part;
    memBlock->addr = outAddr;
    resumeIntr(oldIntr);
    return uid->uid;
}

SceUID sceKernelAllocPartitionMemoryForUser(s32 mpid, char *name, u32 type, u32 size, u32 addr)
{
    SceSysmemPartitionInfo info;
    s32 oldK1 = pspShiftK1();
    info.size = sizeof(SceSysmemPartitionInfo);
    s32 ret = sceKernelQueryMemoryPartitionInfo(mpid, &info);
    if (ret < 0) {
        pspSetK1(oldK1);
        return ret;
    }
    if ((info.attr & 2) == 0) {
        pspSetK1(oldK1);
        return 0x800200D6;
    }
    if (!pspK1PtrOk(name) || (type == 2 && !pspK1PtrOk((void *)addr))) { // 4D8C
        // 4D94
        pspSetK1(oldK1);
        return 0x800200D3;
    }
    // 4D54
    ret = sceKernelAllocPartitionMemory(mpid, name, type, size, addr);
    pspSetK1(oldK1);
    return ret;
}

