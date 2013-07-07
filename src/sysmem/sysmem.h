#ifndef SYSMEM_H
#define SYSMEM_H

#include <sysmem_kernel.h>

/*

'->' means it uses a chained list ('->' for single-chained, '<->' for double-chained)

                    +-------+
          +---------| Heap  | A heap. Has an UID, name "SceSysmemHeap"
          |         +-------+ The main one is named "SceSystemMemoryManager". Allocated in a partition.
          |             |
          |             v
          |        +---------+   +-------+
          |        |  First  |   |       |
          |  ...<->|  Heap   |<->| Heap  |<->... SceSysmemHeapBlock (note: this may be the "Lowheap" from Sony)
          |        |  Block  |   | Block |       These blocks are created as soon as the others can't hold something else from _AllocHeapMemory()
          |        +---------+   +-------+
          |        | Lowheap |
          |        |  with   | SceSysmemLowheap
          |        | the info|
          |        +---------+
          |        | First   | SceSysmemLowheapBlock
          |    +-->| lowheap | (patterns may be different in complex cases)
          |    |   | block   |
          |    |   +---------+
          |    |        .
          |    |        .
          |    |        .
          |    |   +---------+
          |    |   | Last    |
          |    +-->| lowheap |
          |        | block   |
          |        +---------+
          |
          +---------+ (no pointer here, but partition id + address + size)
                    v
   +-----------+  +-----------+  +-----------+
   |           |  |           |  |           |
...| Partition |->| Partition |->| Partition |... partitions created through sceKernelCreateMemoryPartition() (SceSysmemMemoryPartition)
   |           |  |           |  |           |    Most created by the kernel. Has an UID, name "SceSysmemMemoryPartition"
   +-----------+  +-----------+  +-----------+
                    ^   |
    +---------------+   |
    |                   v
    |          +-----------------+   +---------------+
    |          |     (First)     |   |               |
    |    ...<->|  Control block  |<->| Control block |<-> ... SceSysmemCtlBlk
    |          |   30 segments   |   |  30 segments  |
    |          +-----------------+   +---------------+
    |                   |
    |                   v
    |        +---------------------+
    |        |                     |
    |        | 1 of the 30 segments| SceSysmemSeg
    |        |                     |
    |        +---------------------+
    |
    |       +------------------------+
    |       |                        |
    +-------|        Block           | blocks created through sceKernelAllocPartitionMemory() (SceSysmemMemoryBlock)
            | Only holds size & addr | Has an UID, name "SceSysMemMemoryBlock"
            +------------------------+ Some kind of "interface" to the SceSysmemSeg, without the internal information

*/

typedef struct {
    u32 addr;
    u32 size;
} SceSysmemPartInfo;

typedef struct {
    u32 memSize;
    u32 unk4;
    u32 unk8;
    SceSysmemPartInfo other1; // 12
    SceSysmemPartInfo other2; // 20
    SceSysmemPartInfo vshell; // 28
    SceSysmemPartInfo scUser; // 36
    SceSysmemPartInfo meUser; // 44
    SceSysmemPartInfo extSc2Kernel; // 52
    SceSysmemPartInfo extScKernel; // 60
    SceSysmemPartInfo extMeKernel; // 68
    SceSysmemPartInfo extVshell; // 76
} SceSysmemPartTable;

s32 *wmemset(s32 *src, s32 c, u32 n);

#endif

