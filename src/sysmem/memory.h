#ifndef MEMORY_H
#define MEMORY_H

#include "heap.h"
#include "partition.h"

void initheap(SceSysmemLowheap *lowh, u32 size);
void *hmalloc(SceSysmemHeap *heap, SceSysmemLowheap *lowh, u32 size, u32 align);
s32 hfree(SceSysmemHeap *heap, SceSysmemLowheap *lowh, void *ptr);
s32 checkheapnouse(SceSysmemLowheap *lowh);
u32 htotalfreesize(SceSysmemLowheap *lowh);
s32 hmaxfreesize(SceSysmemHeap *heap, SceSysmemLowheap *lowh);

void *_AllocPartitionMemory(SceSysmemMemoryPartition *part, int type, int size, int addr);
s32 _FreePartitionMemory(void *addr);
void _QueryMemoryInfo(u32 address, SceUID *partitionId, SceUID *memoryBlockId);

void *_allocSysMemory(SceSysmemMemoryPartition *part, int type, u32 size, u32 addr, SceSysmemCtlBlk **ctlBlkOut);
s32 _freeSysMemory(SceSysmemMemoryPartition *part, void *addr);

void InitSmemCtlBlk(SceSysmemCtlBlk *ctlBlk);
void updateSmemCtlBlk(SceSysmemMemoryPartition *part, SceSysmemCtlBlk *ctlBlk);
void JointSmemCtlBlk(SceSysmemMemoryPartition *part, SceSysmemCtlBlk *ctlBlk);

void _ReturnSegBlankList(SceSysmemSeg *seg);

void *sceKernelFillBlock64(void *dst, s32 c, u32 size);

void MemoryBlockServiceInit(void);
void sceKernelProtectMemoryBlock(SceSysmemMemoryPartition *part, void *addr);

extern SceSysmemUidCB *g_MemBlockType;

#endif

