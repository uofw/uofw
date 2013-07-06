#ifndef HEAP_H
#define HEAP_H

typedef struct {
    u32 addr; // 0 address of this structure minus 1
    u32 size; // 4 size of this structure + the blocks (+ 1 control bit?)
    u32 busyBlocks; // 8
    SceSysmemLowheapBlock *firstBlock; // 12
    // followed by blocks
} SceSysmemLowheap;

typedef struct {
    int size; // 0
    int partId; // 4
    u32 partAddr; // 8
    u32 partSize; // 12
    SceSysmemHeapBlock *firstBlock; // 16
} SceSysmemHeap;

void HeapInit(void);
int _CreateHeap(void *partition, int size, int attr, SceSysmemHeapBlock **out);
void *_AllocHeapMemory(SceSysmemHeap *heap, u32 size, u32 align);
s32 _FreeHeapMemory(SceSysmemHeap *heap, void *addr);

extern SceSysmemUidCB *g_145A4;

#endif

