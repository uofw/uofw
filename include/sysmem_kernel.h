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
    s32 numExportLibs; //4 -- number of sysmem's export libraries - set in SysMemInit (from utopia)
    SceResidentLibraryEntryTable *exportLib[8]; //8 --array of sysmem's export tables set in SysMemInit (from utopia)
    u32 loadCoreAddr; // 40 -- allocated in SysMemInit (from utopia)
    s32 userLibStart; // 44 -- offset in export_lib at which user libraries begin - set in SysMemInit (from utopia)
    s32 unk48; //48
    s32 unk52; //52
    s32 unk56;//56
    s32 unk60; //60
    SceStubLibraryEntryTable *loadCoreImportTables; //64 -- loadcore stubs - set in kactivate before booting loadcore (from utopia)
    u32 loadCoreImportTablesSize; //68 -- total size of stubs - set in kactivate before booting loadcore (from utopia)
    s32 init_thread_stack; //72 -- allocated in SysMemInit (from utopia)
    SceLoadCoreExecFileInfo *sysMemExecInfo; //76 -- set in kactivate before booting loadcore (from utopia)
    SceLoadCoreExecFileInfo *loadCoreExecInfo; //80 -- set in kactivate before booting loadcore (from utopia)
    s32 (*CompareSubType)(u32 tag); //84
    u32 (*CompareLatestSubType)(u32 tag); //88
    s32 (*SetMaskFunction)(u32 unk1, vs32 *addr); //92
    void (*kprintf_handler)(u8 *format, void *arg1, void *arg2, void *arg3, void *arg4, void *arg5, void *arg6, void *arg7); //96 -- set by sysmem (from utopia)
    s32 (*GetLengthFunction)(u8 *file, u32 size, u32 *newSize); //100 -- set in kactivate before booting loadcore (from utopia)
    s32 (*PrepareGetLengthFunction)(u8 *buf, u32 size); //104
    SceResidentLibraryEntryTable *exportEntryTables[]; //108 
} SysMemThreadConfig;

typedef struct {
    SceSize size;
    u32 startAddr;
    u32 memSize;
    u32 attr;
} SceSysmemPartitionInfo;

s32 sceKernelQueryMemoryPartitionInfo(u32 pid, SceSysmemPartitionInfo *info);

typedef struct {   
    int id;
    int (*func)(void *, int, int funcid, void *args);
} SceSysmemUIDLookupFunction;

typedef struct SceSysmemUIDControlBlock {   
    struct SceSysmemUIDControlBlock *parent; // 0
    struct SceSysmemUIDControlBlock *nextChild; // 4
    struct SceSysmemUIDControlBlock *type; // 8
    SceUID UID; // 12
    char *name; // 16
    unsigned char unk; // 20
    unsigned char size; // size in words
    short attribute; // 22
    struct SceSysmemUIDControlBlock *nextEntry; // 24
    struct SceSysmemUIDControlBlock *inherited; // 28
    SceSysmemUIDLookupFunction *func_table; // 32
} __attribute__((packed)) SceSysmemUIDControlBlock;

typedef struct {   
    int id;
    int (*func)();
} SceSysmemUIDLookupFunc;

typedef struct {
    s32 dlId;
    void *stall;
    u32 count;
    u32 max;
} SceGeLazy;

typedef struct {
    u32 size;
    s32 *cmdList;
    s32 (*sceGeListUpdateStallAddr_lazy)(s32 dlId, void *stall);
    SceGeLazy *lazySyncData;
} SceKernelUsersystemLibWork;

SceSysmemUIDControlBlock *sceKernelCreateUIDtype(const char *name, int attr, SceSysmemUIDLookupFunc *funcs, int unk, SceSysmemUIDControlBlock **type);
SceUID sceKernelCreateUID(SceSysmemUIDControlBlock *type, const char *name, short attr, SceSysmemUIDControlBlock **block);
int sceKernelDeleteUID(SceUID uid);
int sceKernelRenameUID(SceUID uid, const char *name);
int sceKernelGetUIDcontrolBlock(SceUID uid, SceSysmemUIDControlBlock **block);
s32 sceKernelGetUIDcontrolBlockWithType(SceUID uid, SceSysmemUIDControlBlock* type, SceSysmemUIDControlBlock** block);
void sceKernelCallUIDObjCommonFunction(SceSysmemUIDControlBlock *cb, SceSysmemUIDControlBlock *uidWithFunc, s32 funcId, va_list ap);

typedef struct {
    u32 size;
    u32 unk4;
    u32 unk8;
    u32 unk12;
    u32 unk16;
    u32 unk20;
    u32 unk24;
    u32 unk28;
    u32 unk32;
    u32 unk36;
    void *unk40;
    u32 unk44;
    u32 unk48;
    u32 unk52;
} SceKernelSysmemBlockInfo;

s32 sceKernelQueryMemoryBlockInfo(SceUID id, SceKernelSysmemBlockInfo *info);

SceUID sceKernelCreateHeap(SceUID partitionid, SceSize size, int unk, const char *name);
void *sceKernelAllocHeapMemory(SceUID heapid, SceSize size);
int sceKernelFreeHeapMemory(SceUID heapid, void *block);
int sceKernelDeleteHeap(SceUID heapid);
SceSize sceKernelHeapTotalFreeSize(SceUID heapid);

SceKernelUsersystemLibWork *sceKernelGetUsersystemLibWork(void);
s32 sceKernelSetUsersystemLibWork(s32 *cmdList, s32 (*sceGeListUpdateStallAddr_lazy)(s32, void*), SceGeLazy *lazy);

typedef struct {
    u32 size;
    u32 unk4;
    u32 unk8;
    u32 unk12;
    u32 unk16;
    u32 unk20;
    u32 unk24;
    u32 unk28;
    u32 unk32;
    u32 unk36;
    u32 unk40;
    u32 unk44;
    u32 unk48;
    u32 unk52;
    u32 unk56;
    u32 unk60;
    u32 unk64;
    char gameId[10];
    u16 unk78;
    u32 unk80;
    u32 unk84;
    u32 unk88;
    u32 unk92;
    u32 unk96;
    u32 unk100;
    u32 unk104;
    u32 unk108;
    u32 unk112;
    u32 unk116;
    u32 unk120;
    u32 unk124;
    u32 unk128;
    u32 unk132;
    u32 unk136;
    u32 unk140;
    u32 unk144;
    u32 unk148;
    u32 unk152;
    u32 unk156;
    u32 unk160;
    u32 unk164;
    u32 unk168;
    u32 unk172;
    u32 unk176;
    u32 unk180;
    u32 unk184;
    u32 unk188;
    u32 unk192;
    u32 unk196;
    u32 unk200;
    u32 unk204;
    u32 unk208;
    u32 unk212;
    u32 unk216;
} SceKernelGameInfo;

SceKernelGameInfo *sceKernelGetGameInfo(void);
void *sceKernelGetAWeDramSaveAddr(void);
int sceKernelGetMEeDramSaveAddr(void);
int sceKernelGetMEeDramSaveSize(void);

void *sceKernelMemcpy(void *dst, const void *src, u32 n);
void *sceKernelMemmove(void *dst, const void *src, u32 n);
void *sceKernelMemset32(void *buf, int c, int size);
void *sceKernelMemset(void *buf, int c, u32 size);
void *sceKernelMemmoveWithFill(void *dst, const void *src, SceSize size, int c);

int sceKernelGetCompiledSdkVersion(void);

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

int sceKernelGetModel(void);

s32 sceKernelQueryMemoryInfo(u32 addr, SceUID *partitionId, SceUID *blockId);

s32 sceKernelFillFreeBlock(SceUID partitionId, u32 c);

void sceKernelMemoryExtendSize(void);
void sceKernelMemoryShrinkSize(void);

int sceKernelGetAllowReplaceUmd(int *);

int sceKernelSetRebootKernel(void *);

int sceKernelGetSystemStatus(void);
void sceKernelSetSystemStatus(s32 status);
int sceKernelSetDNAS(int dnas);

int SysMemForKernel_BFE08689(void *buf);

int sceKernelGetInitialRandomValue(void);

int sceKernelSetDdrMemoryProtection(int, int, int);

int sceKernelSysMemRealMemorySize(void);

int SysMemForKernel_40B744A4(int);

#endif

