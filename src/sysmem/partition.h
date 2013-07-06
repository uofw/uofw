#ifndef PARTITION_H
#define PARTITION_H

#include <sysmem_kernel.h>

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

typedef struct SceSysmemCtlBlk {
    struct SceSysmemCtlBlk *next; // 0
    struct SceSysmemCtlBlk *prev; // 4
    u16 segCount;
    u16 unk10; // used segments?
    u16 freeSeg; // 12 // points to a free segment id
    u8 firstSeg; // 14
    u8 lastSeg; // 15
    SceSysmemSeg segs[30];
} SceSysmemCtlBlk; // size: 0x100 (control blocks are always placed at a 0x100-aligned address)

typedef struct SceSysmemMemoryPartition {
    struct SceSysmemMemoryPartition *next; // 0
    u32 addr; // 4
    u32 size; // 8
    u32 attr; // 12
    SceSysmemCtlBlk *firstCtlBlk; // 16
    SceSysmemCtlBlk *lastCtlBlk; // 20
    s32 unk24;
} SceSysmemMemoryPartition;

SceSysmemMemoryPartition *MpidToCB(int mpid);
SceSysmemSeg *AddrToSeg(SceSysmemMemoryPartition *part, void *addr);
SceSysmemMemoryPartition *AddrToCB(u32 addr);
s32 CanoniMpid(s32 mpid);

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
    SceSysmemMemoryPartition *extKernel; // 56
    u32 realMemSize; // 60
    u32 unk64; // 64
} SceSysmemMemInfo;

typedef struct {
    void *addr;
    u32 size;
    SceSysmemMemoryPartition *part;
} SceSysmemMemoryBlock; // size: 12; allocated space in partition

extern SceSysmemUidCB *g_145A8;
extern SceSysmemMemInfo g_145C0;

#endif

