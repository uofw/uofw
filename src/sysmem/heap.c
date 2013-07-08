#include <sysmem_kdebug.h>
#include <sysmem_kernel.h>
#include <sysmem_sysclib.h>

#include "intr.h"
#include "memory.h"
#include "partition.h"

#include "heap.h"

s32 heap_do_initialize(SceSysmemUidCB *uid, SceSysmemUidCB *uidWithFunc, int funcId, va_list ap);
s32 heap_do_delete(SceSysmemUidCB *uid, SceSysmemUidCB *uidWithFunc, int funcId, va_list ap);
s32 heap_do_alloc(SceSysmemUidCB *uid, SceSysmemUidCB *uidWithFunc, int funcId, va_list ap);
s32 heap_do_free(SceSysmemUidCB *uid, SceSysmemUidCB *uidWithFunc, int funcId, va_list ap);
s32 heap_do_totalfreesize(SceSysmemUidCB *uid, SceSysmemUidCB *uidWithFunc, int funcId, va_list ap);

s32 _TotalFreeSize(SceSysmemHeap *heap);
s32 _DeleteHeap(SceSysmemHeap *heap);

// 145A4
SceSysmemUidCB *g_HeapType;

SceUID sceKernelCreateHeap(SceUID mpid, SceSize size, int flag, const char *name)
{
    int oldIntr = suspendIntr();
    SceSysmemMemoryPartition *part = MpidToCB(mpid);
    SceSysmemUidCB *uid;
    if (part == NULL) {
        // 300C
        resumeIntr(oldIntr);
        return 0x800200D6;
    }
    s32 ret = sceKernelCreateUID(g_HeapType, name, (pspGetK1() >> 31) & 0xFF, &uid);
    if (ret != 0) {
        // 2FFC
        resumeIntr(oldIntr);
        return ret;
    }
    int realSize = 0;
    if ((flag & 1) != 0)
        realSize = (size + 7) & 0xFFFFFFF8;
    SceSysmemHeap *heap = UID_CB_TO_DATA(uid, g_HeapType, SceSysmemHeap);
    SceSysmemPartitionInfo partInfo;
    heap->size = realSize | ((flag >> 1) & 1);
    heap->partId = mpid;
    partInfo.size = 16;
    sceKernelQueryMemoryPartitionInfo(mpid, &partInfo);
    heap->partAddr = partInfo.startAddr;
    heap->partSize = partInfo.memSize;
    // 2F68
    SceSysmemHeapBlock *block = _AllocPartitionMemory(part, ((flag & 2) == 0) ? 0 : 1, size, 0); // 2FF0
    if (block == NULL) {
        // 2FD0
        sceKernelDeleteUID(uid->uid);
        suspendIntr(oldIntr);
        return 0x800200DC;
    }
    block->next = block;
    block->prev = block;
    initheap((SceSysmemLowheap*)(block + 1), size - 8);
    heap->firstBlock = block;
    resumeIntr(oldIntr);
    return uid->uid;
}

void *_AllocHeapMemory(SceSysmemHeap *heap, u32 size, u32 align)
{
    if (size > 0x20000000)
        return 0;
    if (align != 0 && ((align & 3) != 0 || align > 0x80 || ((align - 1) & align) != 0))
        return 0;
    // 30A8
    SceSysmemHeapBlock *block = heap->firstBlock;
    // 30B4
    for (;;) {
        if ((int)block - heap->partAddr >= heap->partSize) {
            if (!sceKernelIsToolMode())
                for (;;) // 321C
                    ;
            Kprintf("Heap memory is in illegal memory partition\n");
            pspBreak(0);
        }
        // 30F4
        void *ret = hmalloc(heap, (SceSysmemLowheap*)(block + 1), size, align);
        if (ret != NULL)
            return ret;
        if (block == heap->firstBlock)
            break;
        block = block->next;
    }
    if (heap->size < 4) // 312C
        return heap;
    u32 newSize = heap->size & 0xFFFFFFFE;
    if (align == 0) {
        // 31FC
        if (newSize - 40 < size) {
            size = UPALIGN8(size);
            newSize = size + 40;
        }
    } else {
        size = UPALIGN8(size);
        // 3160
        int shift = align - (32 % align);
        if ((32 % align) == 0)
            shift = 0;
        if (newSize - 40 < size + shift)
            newSize = size + shift + 40;
    }
    // 317C
    block = _AllocPartitionMemory(MpidToCB(heap->partId), 0, newSize, 0);
    if (block == NULL)
        return 0;
    block->next = block;
    block->prev = block;
    initheap((SceSysmemLowheap*)(block + 1), newSize - 8);
    block->next = heap->firstBlock;
    block->prev = heap->firstBlock->prev;
    heap->firstBlock->prev = block;
    block->prev->next = block;
    return hmalloc(heap, (SceSysmemLowheap*)(block + 1), size, align);
}

s32 _FreeHeapMemory(SceSysmemHeap *heap, void *addr)
{
    SceSysmemHeapBlock *block = heap->firstBlock->next;
    // 3254
    for (;;) {
        if ((u32)block - heap->partAddr >= heap->partSize) {
            if (sceKernelIsToolMode() == 0)
                for (;;) // 3324
                    ;
            Kprintf("Heap memory is in illegal memory partition\n");
            pspBreak(0);
        }
        SceSysmemLowheap *lowh = (SceSysmemLowheap*)(block + 1);
        // 328C
        if (hfree(heap, lowh, addr) == 0) {
            // 32DC
            if (block != heap->firstBlock) {
                if (checkheapnouse(lowh) != 0) {
                    // 3300
                    lowh->addr = 0;
                    block->next->prev = block->prev;
                    block->prev->next = block->next;
                    _FreePartitionMemory(block);
                }
                return 0;
            }
        }
        if (block == heap->firstBlock)
            return 0x800200D3;
        block = block->next;
    }
}

int sceKernelQueryHeapInfo(SceUID id, SceSysmemHeapInfo *info)
{
    if (info->size < 64)
        return 0x800200D2;
    int oldIntr = suspendIntr();
    SceSysmemUidCB *uid;
    int ret = sceKernelGetUIDcontrolBlockWithType(id, g_HeapType, &uid);
    if (ret != 0) {
        // 3530
        resumeIntr(oldIntr);
        return ret;
    }
    SceSysmemHeap *heap = UID_CB_TO_DATA(uid, g_HeapType, SceSysmemHeap);
    // 33B8
    int i;
    for (i = 0; i < 6; i++)
        ((int*)info->name)[i] = 0;
    if (uid->name != NULL) {
        // 3520
        strncpy(info->name, uid->name, 31);
    }
    // 33D0
    info->perm = uid->attr;
    info->heapSize = heap->size & 0xFFFFFFFE;
    if (heap->size >= 4) {
        if ((heap->size & 1) == 0)
            info->attr = 1;
        else
            info->attr = 3;
    } else
        info->attr = 0;
    // 3408
    int totalSize = 0;
    int totalFreeSize = 0;
    SceSysmemHeapBlock *curBlock = heap->firstBlock->next;
    SceSysmemLowheap *cur = (SceSysmemLowheap*)(curBlock + 1);
    int maxFreeSize = 0;
    int numHeaps = 0;
    // 3420
    for (;;)
    {
        if ((u32)curBlock - heap->partAddr >= heap->partSize)
            break;
        numHeaps++;
        totalSize += ((cur->size - 16) & 0xFFFFFFF8) - 8;
        totalFreeSize += htotalfreesize(cur);
        maxFreeSize = pspMax(maxFreeSize, hmaxfreesize(heap, cur));
        if (curBlock == heap->firstBlock)
            break;
        curBlock = curBlock->next;
    }
    // 347C
    info->totalsize = totalSize;
    u32 size = 64;
    info->totalfreesize = totalFreeSize;
    info->maxfreesize = maxFreeSize;
    info->numheaps = numHeaps;
    curBlock = heap->firstBlock->next;
    SceSysmemHeapBlock **heaps = (SceSysmemHeapBlock **)&info->heaps;
    if (info->size >= 68)
    {
        // 34A8
        do
        {
            if ((u32)curBlock - heap->partAddr >= heap->partSize)
                break;
            size += 4;
            *(heaps++) = curBlock;
            if (curBlock == heap->firstBlock)
                break;
            curBlock = curBlock->next;
        } while (size + 4 <= info->size);
    }
    // (34DC)
    info->size = size;
    // 34E0
    resumeIntr(oldIntr);
    return 0;
}

s32 sceKernelQueryLowheapInfo(SceSysmemHeapBlock *block, SceSysmemLowheapInfo *info)
{
    u32 maxInfoSize = info->size;
    if (maxInfoSize < 24)
        return 0x800200D2;
    SceSysmemLowheap *lowh = (SceSysmemLowheap *)(block + 1);
    if (lowh->addr != (u32)lowh - 1)
        return 0x800200DE;
    SceSysmemLowheapBlock *cur = (SceSysmemLowheapBlock *)(lowh + 1);
    u32 infoSize = 24;
    SceSysmemLowheapBlock *last = (void*)lowh + lowh->size - 8;
    u32 blockCount = 0;
    u32 usedCount = 0;
    u32 freeCount = 0;
    u32 maxFreeCount = 0;
    SceSysmemLowheapInfoBlock *infoBlock = info->infoBlocks;
    SceSysmemLowheapBlock *first = cur;
    // 35B0
    while (cur != last) {
        if (cur < first || cur > last)
            break;
        if (infoSize + 8 >= maxInfoSize)
            infoBlock = NULL;
        SceSysmemLowheapBlock *block;
        if (cur->next == (void *)lowh) { // used
            // 3670
            block = (SceSysmemLowheapBlock *)((u32)cur | 0x1);
            usedCount += cur->count - 1;
        } else {
            block = cur;
            freeCount += cur->count - 1;
            if (cur->count - 1 > maxFreeCount)
                maxFreeCount = cur->count - 1;
        }
        // 35EC
        blockCount++;
        if (infoBlock != NULL) {
            infoSize += 8;
            infoBlock->block = block;
            infoBlock->offset = (cur->count - 1) * 8;
            infoBlock++;
        }
        // 3618
        cur += cur->count;
    }
    // 3628
    info->size = infoSize;
    info->heapSize = ((lowh->size - 16) & 0xFFFFFFF8) - 8;
    info->usedSize = usedCount << 3;
    info->freeSize = freeCount << 3;
    info->maxFreeSize = maxFreeCount << 3;
    info->blockCount = blockCount;
    return blockCount;
}

// 13480
SceSysmemUidLookupFunc HeapFuncs[] =
{
    { 0xD310D2D9, heap_do_initialize },
    { 0x87089863, heap_do_delete },
    { 0x0DE3B1BD, heap_do_alloc },
    { 0xA9CE362D, heap_do_free },
    { 0x01DB36E1, heap_do_totalfreesize },
    { 0, NULL}
};

void HeapInit(void)
{
    sceKernelCreateUIDtype("SceSysmemHeap", sizeof(SceSysmemHeap), HeapFuncs, NULL, &g_HeapType);
}

int _CreateHeap(void *partition, int size, int attr, SceSysmemHeapBlock **out)
{
    SceSysmemHeapBlock *ptr;
    if ((attr & 2) == 0)
        ptr = _AllocPartitionMemory(partition, 0, size, 0);
    else
        ptr = _AllocPartitionMemory(partition, 1, size, 0);
    // 36F0
    if (ptr == NULL)
        return 0x800200DC;
    *out = ptr;
    ptr->next = ptr;
    ptr->prev = ptr;
    initheap((SceSysmemLowheap*)(ptr + 1), size - 8);
    return 0;
}

int sceKernelDeleteHeap(SceUID id)
{
    int oldIntr = suspendIntr();
    int ret = sceKernelDeleteUID(id);
    resumeIntr(oldIntr);
    return ret;
}

void *sceKernelAllocHeapMemoryWithOption(SceUID id, int size, SceSysmemHeapAllocOption *opt)
{
    int oldIntr = suspendIntr();
    SceSysmemUidCB *uid;
    if (sceKernelGetUIDcontrolBlockWithType(id, g_HeapType, &uid) != 0) {
        // 382C
        resumeIntr(oldIntr);
        return 0;
    }
    // 37F4
    void *ret = _AllocHeapMemory(UID_CB_TO_DATA(uid, g_HeapType, SceSysmemHeap), size, (opt != NULL) ? opt->align : 0);
    resumeIntr(oldIntr);
    return ret;
}

void *sceKernelAllocHeapMemory(SceUID id, int size)
{
    int oldIntr = suspendIntr();
    SceSysmemUidCB *uid;
    if (sceKernelGetUIDcontrolBlockWithType(id, g_HeapType, &uid) != 0) {
        resumeIntr(oldIntr);
        return 0;
    }
    // 38B0
    void *ret = _AllocHeapMemory(UID_CB_TO_DATA(uid, g_HeapType, SceSysmemHeap), size, 0);
    resumeIntr(oldIntr);
    return ret;
}

s32 sceKernelFreeHeapMemory(SceUID id, void *addr)
{
    s32 oldIntr = suspendIntr();
    SceSysmemUidCB *uid;
    s32 ret = sceKernelGetUIDcontrolBlockWithType(id, g_HeapType, &uid);
    if (ret == 0)
        ret = _FreeHeapMemory(UID_CB_TO_DATA(uid, g_HeapType, SceSysmemHeap), addr);
    // 3948
    resumeIntr(oldIntr);
    return ret;
}

s32 sceKernelHeapTotalFreeSize(SceUID id)
{
    s32 oldIntr = suspendIntr();
    SceSysmemUidCB *uid;
    s32 ret = sceKernelGetUIDcontrolBlockWithType(id, g_HeapType, &uid);
    if (ret == 0)
        ret = _TotalFreeSize(UID_CB_TO_DATA(uid, g_HeapType, SceSysmemHeap));
    // 39D0
    resumeIntr(oldIntr);
    return ret;
}

s32 heap_do_initialize(SceSysmemUidCB *uid, SceSysmemUidCB *uidWithFunc, int funcId, va_list ap)
{
    sceKernelCallUIDObjCommonFunction(uid, uidWithFunc, funcId, ap);
    SceSysmemHeap *heap = UID_CB_TO_DATA(uid, g_HeapType, SceSysmemHeap);
    heap->size = 0;
    heap->partId = 0;
    heap->firstBlock = NULL;
    return uid->uid;
}

s32 heap_do_delete(SceSysmemUidCB *uid, SceSysmemUidCB *uidWithFunc, int funcId, va_list ap)
{
    SceSysmemHeap *heap = UID_CB_TO_DATA(uid, g_HeapType, SceSysmemHeap);
    if (heap->firstBlock != NULL)
        _DeleteHeap(heap);
    sceKernelCallUIDObjCommonFunction(uid, uidWithFunc, funcId, ap);
    return uid->uid;
}

s32 heap_do_alloc(SceSysmemUidCB *uid, SceSysmemUidCB *uidWithFunc __attribute__((unused)), int funcId __attribute__((unused)), va_list ap)
{
    u32 size = va_arg(ap, u32);
    SceSysmemHeapAllocOption *opt = va_arg(ap, SceSysmemHeapAllocOption *);
    void **retPtr = va_arg(ap, void**);
    SceSysmemHeap *heap = UID_CB_TO_DATA(uid, g_HeapType, SceSysmemHeap);
    // 3B10
    *retPtr = _AllocHeapMemory(heap, size, (opt != NULL) ? opt->align : 0);
    return uid->uid;
}

s32 heap_do_free(SceSysmemUidCB *uid, SceSysmemUidCB *uidWithFunc __attribute__((unused)), int funcId __attribute__((unused)), va_list ap)
{
    s32 ret = _FreeHeapMemory(UID_CB_TO_DATA(uid, g_HeapType, SceSysmemHeap), va_arg(ap, void*));
    if (ret != 0)
        return ret;
    return uid->uid;
}

s32 heap_do_totalfreesize(SceSysmemUidCB *uid, SceSysmemUidCB *uidWithFunc __attribute__((unused)), int funcId __attribute__((unused)), va_list ap)
{
    *va_arg(ap, s32*) = _TotalFreeSize(UID_CB_TO_DATA(uid, g_HeapType, SceSysmemHeap));
    return uid->uid;
}

s32 _DeleteHeap(SceSysmemHeap *heap)
{
    SceSysmemHeapBlock *firstBlock = heap->firstBlock;
    SceSysmemHeapBlock *cur = firstBlock->next;
    if (cur != firstBlock) {
        // 3C04
        do {
            if ((u32)cur - heap->partAddr >= heap->partSize) {
                if (sceKernelIsToolMode() == 0)
                    for (;;) // 3C7C
                        ;
                Kprintf("Heap memory is in illegal memory partition\n");
                pspBreak(0);
            }
            // 3C38
            ((SceSysmemLowheap*)(cur + 1))->addr = 0;
            _FreePartitionMemory(cur);
            cur = cur->next;
        } while (cur != firstBlock);
    }
    // 3C50
    ((SceSysmemLowheap*)(firstBlock + 1))->addr = 0;
    return _FreePartitionMemory(firstBlock);
}

s32 _TotalFreeSize(SceSysmemHeap *heap)
{
    s32 size = 0;
    SceSysmemHeapBlock *cur = heap->firstBlock;
    for (;;) {
        if ((u32)cur - heap->partAddr >= heap->partSize) {
            if (sceKernelIsToolMode() == 0)
                for (;;) // 3D20
                    ;
            Kprintf("Heap memory is in illegal memory partition\n");
            pspBreak(0);
        }
        // 3CE4
        size += htotalfreesize((SceSysmemLowheap*)(cur + 1));
        if (cur == heap->firstBlock)
            return size;
        cur = cur->next;
    }
}

