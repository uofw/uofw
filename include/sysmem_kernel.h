/* Copyright (C) 2011, 2012, 2013 The uOFW team
   See the file COPYING for copying permission.
*/

#ifndef SYSMEM_KERNEL_H
#define SYSMEM_KERNEL_H

#include <stdarg.h>

#include "common_header.h"

#include "sysmem_user.h"
#include "loadcore.h"

typedef struct {
    s32 unk0; //0
    u32 numExportLibs; //4 -- number of sysmem's export libraries - set in SysMemInit (from utopia)
    SceResidentLibraryEntryTable *kernelLibs[8]; //8 --array of sysmem's export tables set in SysMemInit (from utopia)
    u32 loadCoreAddr; // 40 -- allocated in SysMemInit (from utopia)
    u32 numKernelLibs; // 44 -- offset in export_lib at which user libraries begin - set in SysMemInit (from utopia)
    s32 unk48; //48
    SceUID (*AllocPartitionMemory)(s32 mpid, char *name, u32 type, u32 size, u32 addr); // 52
    void * (*GetBlockHeadAddr)(SceUID id); // 56
    s32 (*ResizeMemoryBlock)(SceUID id, s32 leftShift, s32 rightShift); // 60
    SceStubLibraryEntryTable *loadCoreImportTables; //64 -- loadcore stubs - set in kactivate before booting loadcore (from utopia)
    u32 loadCoreImportTablesSize; //68 -- total size of stubs - set in kactivate before booting loadcore (from utopia)
    void *initThreadStack; //72 -- allocated in SysMemInit (from utopia)
    SceLoadCoreExecFileInfo *sysMemExecInfo; //76 -- set in kactivate before booting loadcore (from utopia)
    SceLoadCoreExecFileInfo *loadCoreExecInfo; //80 -- set in kactivate before booting loadcore (from utopia)
    s32 (*CompareSubType)(u32 tag); //84
    u32 (*CompareLatestSubType)(u32 tag); //88
    s32 (*SetMaskFunction)(u32 unk1, vs32 *addr); //92
    void (*Kprintf)(const char *fmt, ...); //96 -- set by sysmem (from utopia)
    s32 (*GetLengthFunction)(u8 *file, u32 size, u32 *newSize); //100 -- set in kactivate before booting loadcore (from utopia)
    s32 (*PrepareGetLengthFunction)(u8 *buf, u32 size); //104
    SceResidentLibraryEntryTable *userLibs[3]; //108 
} SysMemThreadConfig;

/** PSP Hardware models. */
enum ScePspHwModels {
    /** PSP Fat (01g). */
    PSP_1000 = 0,
    /** PSP Slim (02g). */
	PSP_2000 = 1,
    /** PSP Brite (03g). */
	PSP_3000 = 2,
    /** PSP Brite (04g). */
	PSP_4000 = 3,
    /** PSP Go (05g). */
	PSP_GO   = 4,
    /** PSP Brite (07g). */
	PSP_7000 = 6,
    /** PSP Brite (09g). */
	PSP_9000 = 8,
    /** PSP Street E-1000 (11g). */
    PSP_11000 = 10, 
};

/* 
 * Misc
 */

typedef struct {
    u32 size; // 0
    u32 flags; // 4
    char str8[16]; // 8
    char str24[11]; // 24
    // padding?
    int unk36;
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

s32 SysMemForKernel_807179E7(char *gameId, int arg1, char *arg2, char *arg3, int arg4, int arg5, char *arg6);
s32 sceKernelCopyGameInfo(SceKernelGameInfo *info);
s32 SysMemForKernel_F3BDB718(char *arg0);
s32 sceKernelGetQTGP2(char *qtgp2);
s32 sceKernelSetQTGP2(char *qtgp2);
s32 sceKernelGetQTGP3(char *qtgp3);
s32 sceKernelSetQTGP3(char *qtgp3);
s32 sceKernelGetAllowReplaceUmd(u32 *allow);
s32 sceKernelSetAllowReplaceUmd(u32 allow);
s32 sceKernelSetUmdCacheOn(u32 umdCacheOn);
s32 SysMemForKernel_40B744A4(u32 unk112);
s32 SysMemForKernel_BFE08689(char *str116);
s32 SysMemForKernel_2A8B8B2D(char *unk204);
SceKernelGameInfo *sceKernelGetGameInfo(void);
u32 sceKernelGetCompiledSdkVersion(void);
s32 sceKernelSetCompiledSdkVersion100(u32 ver);
s32 sceKernelSetCompiledSdkVersion370(u32 ver);
s32 sceKernelSetCompiledSdkVersion380_390(u32 ver);
s32 sceKernelSetCompiledSdkVersion395(u32 ver);
s32 sceKernelSetCompiledSdkVersion410_420(u32 ver);
s32 sceKernelSetCompiledSdkVersion500_550(u32 ver);
s32 sceKernelSetCompiledSdkVersion570(u32 ver);
s32 sceKernelSetCompiledSdkVersion600_620(u32 ver);
s32 sceKernelSetCompiledSdkVersion630_650(u32 ver);
s32 sceKernelSetCompiledSdkVersion660(u32 ver);
s32 sceKernelGetCompilerVersion(void);
s32 sceKernelSetCompilerVersion(s32 version);
s32 sceKernelGetDNAS(void);
s32 sceKernelSetDNAS(s32 dnas);
s32 sceKernelGetInitialRandomValue(void);
s32 SysMemForKernel_A0A9185A(void);
u32 SysMemForKernel_13EE28DA(u32 flag);
u32 sceKernelGetModel(void);
s32 sceKernelSetRebootKernel(s32 (*rebootKernel)());
s32 sceKernelRebootKernel(void *arg);
s32 sceKernelRegisterGetIdFunc(void *func);
s32 sceKernelGetId(const char *path, char *id);

/*
 * Debugging (disabled in release)
 */
s32 sceKernelApiEvaluationInit();
s32 sceKernelRegisterApiEvaluation();
s32 sceKernelApiEvaliationAddData();
s32 sceKernelApiEvaluationReport();
s32 sceKernelSetGcovFunction();
s32 sceKernelCallGcovFunction();
s32 sceKernelSetGprofFunction();
s32 sceKernelCallGprofFunction();
int sceKernelCheckDebugHandler();
s32 SysMemForKernel_7FF2F35A(char *arg);
s32 SysMemForKernel_A03CB480(char *arg);

/*
 * Heap
 */

typedef struct SceSysmemLowheapBlock {
    struct SceSysmemLowheapBlock *next;
    u32 count;
} SceSysmemLowheapBlock;

typedef struct SceSysmemHeapBlock {
    struct SceSysmemHeapBlock *next, *prev; // 0, 4
    // followed by lowheap
} SceSysmemHeapBlock;

SceUID sceKernelCreateHeap(SceUID mpid, SceSize size, int flag, const char *name);

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
    SceSysmemHeapBlock *heaps[];
} SceSysmemHeapInfo;

int sceKernelQueryHeapInfo(SceUID id, SceSysmemHeapInfo *info);

typedef struct SceSysmemLowheapInfoBlock {
    SceSysmemLowheapBlock *block;
    u32 offset;
} SceSysmemLowheapInfoBlock;

typedef struct {
    u32 size; // 0
    u32 heapSize; // 4
    u32 usedSize; // 8
    u32 freeSize; // 12
    u32 maxFreeSize; // 16
    u32 blockCount; // 20
    SceSysmemLowheapInfoBlock infoBlocks[];
} SceSysmemLowheapInfo; // size: 24

s32 sceKernelQueryLowheapInfo(SceSysmemHeapBlock *block, SceSysmemLowheapInfo *info);
int sceKernelDeleteHeap(SceUID id);

typedef struct {
    u32 size; // structure size (probably)
    u32 align;
} SceSysmemHeapAllocOption;

void *sceKernelAllocHeapMemoryWithOption(SceUID id, int size, SceSysmemHeapAllocOption *opt);
void *sceKernelAllocHeapMemory(SceUID id, int size);
s32 sceKernelFreeHeapMemory(SceUID id, void *addr);
s32 sceKernelHeapTotalFreeSize(SceUID id);

/*
 * Main
 */

typedef struct {
    unsigned used : 1; // 0
    unsigned next : 6; /* next index */ // 1
    unsigned offset : 25; /* offset (from the partition start, divided by 0x100) */ // 7
    unsigned isProtected : 1; // 0
    unsigned sizeLocked : 1; // 1
    unsigned prev : 6; // 2
    unsigned checkOverflow : 1; // 8
    unsigned size : 23; /* size (divided by 0x100) */ // 9
} SceSysmemSeg; // size: 8

typedef struct {
    union {
        u32 segCount;
        u32 segAddr;
    } info;
    u32 size; // 4
    u32 unused8; // 8
    SceSysmemSeg *curSeg; // 12
} SceSysMemoryInfo;

void sceKernelGetSysMemoryInfo(s32 mpid, u32 needsInit, SceSysMemoryInfo *info);
s32 sceKernelGetSysmemIdList(s32 id, s32 *uids, s32 maxCount, s32 *totalCount);
s32 sceKernelSysMemRealMemorySize(void);
s32 sceKernelSysMemMemSize(void);
s32 sceKernelSysMemMaxFreeMemSize(void);
s32 sceKernelGetMEeDramSaveAddr(void);
s32 sceKernelGetAWeDramSaveAddr(void);
s32 sceKernelGetMEeDramSaveSize(void);
s32 sceKernelGetAWeDramSaveSize(void);
s32 sceKernelDevkitVersion(void);
s32 sceKernelGetSystemStatus(void);
s32 sceKernelSetSystemStatus(s32 newStatus);

typedef struct {
    // Last display list for which a UpdateStallAddr() was run
    s32 dlId;
    // The stall address which was supposed to be set in the last call
    void *stall;
    // Number of times an update has been called on the current dlId
    u32 count;
    // Number of calls to updateStallAddr() required until we really set the address
    u32 max;
} SceGeLazy;

typedef struct {
    u32 size;
    s32 *cmdList;
    s32 (*sceGeListUpdateStallAddr_lazy)(s32 dlId, void *stall);
    SceGeLazy *lazySyncData;
} SceKernelUsersystemLibWork;

s32 sceKernelSetUsersystemLibWork(s32 *cmdList, s32 (*sceGeListUpdateStallAddr_lazy)(s32, void*), SceGeLazy *lazy);
SceKernelUsersystemLibWork *sceKernelGetUsersystemLibWork(void);

/*
 * Memory Block
 */

typedef struct {
    s32 size; // Structure size
} SceSysmemMemoryBlockAllocOption;

SceUID sceKernelAllocMemoryBlock(char *name, u32 type, u32 size, SceSysmemMemoryBlockAllocOption *opt);
s32 sceKernelFreeMemoryBlock(SceUID id);
s32 sceKernelGetMemoryBlockAddr(SceUID id, void **addrPtr);

/*
 * Memory Main
 */

typedef struct {
    u32 size; // 0
    char name[32]; // 4
    u32 attr; // 36
    u32 addr; // 40
    u32 memSize; // 44
    u32 sizeLocked; // 48
    u32 used; // 52
} SceSysmemMemoryBlockInfo;

s32 sceKernelResizeMemoryBlock(SceUID id, s32 leftShift, s32 rightShift);
s32 sceKernelJointMemoryBlock(SceUID id1, SceUID id2);
s32 sceKernelSeparateMemoryBlock(SceUID id, u32 cutBefore, u32 size);
s32 sceKernelQueryMemoryBlockInfo(SceUID id, SceSysmemMemoryBlockInfo *infoPtr);
s32 sceKernelSizeLockMemoryBlock(SceUID id);
s32 sceKernelFreePartitionMemory(SceUID id);
s32 sceKernelQueryMemoryInfo(u32 address, SceUID *partitionId, SceUID *memoryBlockId);
void *sceKernelGetBlockHeadAddr(SceUID id);
u32 SysMemForKernel_CC31DEAD(SceUID id);
void *sceKernelMemset(void *src, s8 c, u32 size);
void *sceKernelMemset32(void *src, s32 c, u32 size);
void *sceKernelMemmove(void *dst, void *src, u32 size);
void *sceKernelMemmoveWithFill(void *dst, void *src, u32 size, s32 fill);
void *sceKernelMemcpy(void *dst, const void *src, u32 n);

/*
 * Memory Operations
 */
void sceKernelMemoryExtendSize(void);
void sceKernelMemoryShrinkSize(void);
u32 sceKernelMemoryOpenSize(void);
void sceKernelMemoryCloseSize(u32 state);
s32 sceKernelSetDdrMemoryProtection(u32 addr, u32 size, u32 set);

/*
 * Partitions
 */

typedef struct {
    SceSize size; // 0
    u32 startAddr; // 4
    u32 memSize; // 8
    u32 attr; // 12
} SceSysmemPartitionInfo;

s32 sceKernelQueryMemoryPartitionInfo(s32 mpid, SceSysmemPartitionInfo *info);
u32 sceKernelPartitionMaxFreeMemSize(s32 mpid);
u32 sceKernelPartitionTotalMemSize(s32 mpid);
u32 sceKernelTotalMemSize(void);
u32 sceKernelPartitionTotalFreeMemSize(s32 mpid);
s32 sceKernelFillFreeBlock(s32 mpid, u32 c);
SceUID sceKernelAllocPartitionMemory(s32 mpid, char *name, u32 type, u32 size, u32 addr);

/*
 * UIDs
 */

#define UID_CB_TO_DATA(uid, typeStruct, type) ((type*)((void*)uid + typeStruct->size * 4))
#define UID_DATA_TO_CB(data, typeStruct) ((SceSysmemUidCB*)((void*)data - typeStruct->size * 4))

struct SceSysmemUidLookupFunc;

typedef struct SceSysmemUidCB {
    struct SceSysmemUidCB *PARENT0; // 0
    struct SceSysmemUidCB *nextChild; // 4
    struct SceSysmemUidCB *meta; // 8: the type UID
    SceUID uid; // 12
    char *name; // 16
    u8 childSize; // 20
    u8 size; // 21
    u16 attr; // 22
    union {
        struct SceSysmemUidCB *next; // 24
        s32 numChild; // 24
    } next;
    struct SceSysmemUidCB *PARENT1; // 28
    struct SceSysmemUidLookupFunc *funcTable; // 32
} __attribute__((packed)) SceSysmemUidCB; // size: 36

typedef s32 (*SceSysmemUidFunc)(SceSysmemUidCB *uid, SceSysmemUidCB *uidWithFunc, s32 funcId, va_list ap);

typedef struct SceSysmemUidLookupFunc {
    s32 id;
    SceSysmemUidFunc func;
} SceSysmemUidLookupFunc;

s32 sceKernelCallUIDFunction(SceUID id, int funcId, ...);
s32 sceKernelCallUIDObjFunction(SceSysmemUidCB *uid, s32 funcId, ...);
int sceKernelLookupUIDFunction(SceSysmemUidCB *uid, int id, SceSysmemUidFunc *func, SceSysmemUidCB **parentUidWithFunc);
s32 sceKernelCallUIDObjCommonFunction(SceSysmemUidCB *uid, SceSysmemUidCB *uidWithFunc, s32 funcId, va_list ap);
int sceKernelCreateUIDtypeInherit(const char *parentName, const char *name, int size,
                                  SceSysmemUidLookupFunc *funcTable, SceSysmemUidLookupFunc *metaFuncTable,
                                  SceSysmemUidCB **uidTypeOut);
int sceKernelCreateUID(SceSysmemUidCB *type, const char *name, char k1, SceSysmemUidCB **outUid);
SceUID sceKernelSearchUIDbyName(const char *name, SceUID typeId);
int sceKernelCreateUIDtype(const char *name, int size, SceSysmemUidLookupFunc *funcTable,
                           SceSysmemUidLookupFunc *metaFuncTable, SceSysmemUidCB **uidTypeOut);
s32 sceKernelDeleteUIDtype(SceSysmemUidCB *uid);
s32 sceKernelGetUIDname(SceUID id, s32 len, char *out);
s32 sceKernelRenameUID(SceUID id, const char *name);
s32 sceKernelGetUIDtype(SceUID id);
s32 sceKernelDeleteUID(SceUID id);
s32 sceKernelGetUIDcontrolBlock(SceUID id, SceSysmemUidCB **uidOut);
s32 sceKernelGetUIDcontrolBlockWithType(SceUID id, SceSysmemUidCB *type, SceSysmemUidCB **outUid);
s32 sceKernelIsKindOf(SceSysmemUidCB *uid, SceSysmemUidCB *type);
s32 sceKernelPrintUidListAll(void);

typedef struct {
    SceSysmemUidCB *root; // 0
    SceSysmemUidCB *metaRoot; // 4
    SceSysmemUidCB *basic; // 8
    s32 count; // 12
} SceSysmemUidList;

SceSysmemUidList *sceKernelGetUidmanCB(void);
s32 sceKernelIsHold(SceSysmemUidCB *uid0, SceSysmemUidCB *uid1);
s32 sceKernelHoldUID(SceUID id0, SceUID id1);
s32 sceKernelReleaseUID(SceUID id0, SceUID id1);

#endif

