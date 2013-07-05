s32 (*g_rebootKernel)(void *arg); // 13FE4

typedef struct
{
    u32 size; // 0
    u32 flags; // 4
    char str8[16]; // 8
    char str24[11]; // 24
    // padding?
    char qtgp2[8]; // 40
    char qtgp3[16]; // 48
    u32 allowReplaceUmd; // 64
    char gameId[14]; // 68
    // padding?
    u32 unk84; // 84
    char str88[8]; // 88
    u32 umdCacheOn; // 96
    u32 sdkVersion; // 100
    u32 compilerVersion; // 104
    u32 dnas; // 108
    u32 unk112; // 112
    char str116[64]; // 116
    char str180[11]; // 180
    // padding?
    char str196[8]; // 196
    char unk204[8]; // 204
    int unk212; // 212
    int unk216; // 216
} SceKernelGameInfo;

SceKernelGameInfo g_13FEC; // 13FEC

char g_140C8[9]; // 140C8

u32 g_140D4; // 140D4

s32 g_initialRand; // 14538
u32 g_1453C; // 1453C
u32 g_model; // 14540

/*

typedef struct SceSysmemLowheapBlock {
    struct SceSysmemLowheapBlock *next;
    u32 count;
} SceSysmemLowheapBlock;

*/

typedef struct {
    u32 addr; // 0 address of this structure minus 1
    u32 size; // 4 size of this structure + the blocks (+ 1 control bit?)
    u32 busyBlocks; // 8
    SceSysmemLowheapBlock *firstBlock; // 12
    // followed by blocks
} SceSysmemLowheap;

/*

typedef struct SceSysmemHeapBlock {
    struct SceSysmemHeapBlock *next, *prev; // 0, 4
    // followed by lowheap
} SceSysmemHeapBlock;

typedef struct SceSysmemLowheapInfoBlock {
    SceSysmemLowheapBlock *block;
    u32 offset;
} SceSysmemLowheapInfoBlock;

typedef struct {
    u32 size; // 0
    u32 heapSize; // 4
    u32 unkSize1; // 8
    u32 unkSize2; // 12
    u32 maxSize; // 16
    u32 blockCount; // 20
    SceSysmemLowheapInfoBlock infoBlocks[];
} SceSysmemLowheapInfo; // size: 24

*/

typedef struct {
    int size; // 0
    int partId; // 4
    u32 partAddr; // 8
    u32 partSize; // 12
    SceSysmemHeapBlock *firstBlock; // 16
} SceSysmemHeap;

/*
typedef struct {
    u32 size; // 0
    char name[32]; // 4
    int perm; // 36
    int attr; // 40
    int heapSize; // 44
    int totalsize; // 48
    int totalfreesize; // 52
    int maxfreesize; // 56
    int numheaps; // 60
    void *heaps[];
} SceSysmemHeapInfo;
*/

SceSysmemHeap g_140D8;

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
            +------------------------+ Some kind of "interface" to the SceSysmemBlock, without the internal information

*/

typedef struct {
    unsigned used : 1; // 0
    unsigned next : 6; /* next index */ // 1
    unsigned offset : 25; /* offset (from the partition start, divided by 0x100) */ // 7
    unsigned unk0_0 : 1; // 0
    unsigned sizeLocked : 1; // 1
    unsigned prev : 6; // 2
    unsigned unk8 : 1; // 8
    unsigned size : 23; /* size (divided by 0x100) */ // 9
} SceSysmemSeg; // size: 8

typedef struct {
    SceSysmemCtlBlk *next; // 0
    SceSysmemCtlBlk *prev; // 4
    u16 segCount;
    u16 unk10; // used segments?
    u16 freeSeg; // 12 // points to a free segment id
    u8 firstSeg; // 14
    u8 lastSeg; // 15
    SceSysmemBlock segs[30];
} SceSysmemCtlBlk; // size: 0x100 (control blocks are always placed at a 0x100-aligned address)

typedef struct SceSysmemMemoryPartition {
    SceSysmemMemoryPartition *next; // 0
    u32 addr; // 4
    u32 size; // 8
    s32 unk12;
    SceSysmemCtlBlk *firstCtlBlk; // 16
    SceSysmemCtlBlk *lastCtlBlk; // 20
    s32 unk24;
} SceSysmemMemoryPartition;

/*
typedef s32 (*SceSysmemUidFunc)(SceSysmemUidCB *uid, SceSysmemUidCB *uidWithFunc, s32 funcId, va_list ap);

typedef struct {
    s32 id;
    SceSysmemUidFunc func;
} SceSysmemUidLookupFunc;

typedef struct {
    SceSysmemUidCB *PARENT0; // 0
    SceSysmemUidCB *nextChild; // 4
    SceSysmemUidCB *meta; // 8: the type UID
    u32 uid; // 12
    char *name; // 16
    u8 childSize; // 20
    u8 size; // 21
    u16 attr; // 22
    union {
        SceSysmemUidCB *next; // 24
        s32 numChild; // 24
    } next;
    SceSysmemUidCB *PARENT1; // 28
    SceSysmemUidLookupFunc *funcTable; // 32
} SceSysmemUidCB; // size: 36

typedef struct {
    SceSysmemUidCB *root; // 0
    SceSysmemUidCB *metaRoot; // 4
    SceSysmemUidCB *basic; // 8
    s32 count; // 12
} SceSysmemUidList;
*/

SceSysmemUidList g_uidTypeList; // 145B0

typedef struct {
    u32 addr;
    u32 size;
} SceSysmemPartInfo;

typedef struct SceSysmemHoldElem {
    struct SceSysmemHoldElem *next; // 0
    struct SceSysmemHoldElem *prev; // 4
} SceSysmemHoldElem;

typedef struct {
    SceSysmemHoldElem unk0; // 0
    SceSysmemHoldElem unk8; // 8
    u32 count1; // 16
    u32 count2; // 20
} SceSysmemHoldHead; // size: 24

typedef struct {
    SceSysmemHoldElem unk0; // 0
    SceSysmemHoldElem unk8; // 8
    SceSysmemUidCB *uid1; // 16
    SceSysmemUidCB *uid0; // 20
} SceSysmemHoldElement;

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

typedef struct {
    u32 memAddr; // 0
    u32 memSize; // 4
    SceSysmemMemoryPartition *main; // 8
    SceSysmemMemoryPartition *other1; // 12
    SceSysmemMemoryPartition *other2; // 16
    SceSysmemMemoryPartition *vshell; // 20
    SceSysmemMemoryPartition *scUser; // 24
    SceSysmemMemoryPartition *meUser; // 28
    SceSysmemMemoryPartition *extScKernel; // 32
    SceSysmemMemoryPartition *extSc2Kernel; // 36
    SceSysmemMemoryPartition *extMeKernel; // 40
    SceSysmemMemoryPartition *extVshell; // 44
    SceSysmemMemoryPartition *kernel; // 48
    SceSysmemMemoryPartition *user; // 52
    SceSysmemmemoryPartition *extKernel; // 56
    u32 realMemSize; // 60
    u32 unk64; // 64
} SceSysmemMemInfo;

/*
typedef struct {
    u32 size; // structure size
    void *unk4;
    void *unk8;
    void *unk12;
} SceUsersystemLibWork;
*/

SceUsersystemLibWork g_1454C;

SceSysmemMemInfo g_145C0;

typedef struct {
    u32 addr;
    u32 size;
    SceSysmemMemoryPartition *part;
} SceSysmemMemoryBlock; // size: 12; allocated space in partition

/*
typedef struct {
    u32 size; // 0
    char name[32]; // 4
    u32 attr; // 36
    u32 addr; // 40
    u32 memSize; // 44
    u32 sizeLocked; // 48
    u32 used; // 52
} SceSysmemMemoryBlockInfo;
*/

typedef struct {
    u32 size; // structure size (probably)
    u32 addr;
} SceSysmemHeapAllocOption;

