/* Copyright (C) 2011, 2012 The uOFW team
   See the file COPYING for copying permission.
*/

#include "common.h"

typedef struct
{
    SceSize size;
    unsigned int startaddr;
    unsigned int memsize;
    unsigned int attr;
} SceSysmemPartitionInfo;

int sceKernelQueryMemoryPartitionInfo(int pid, SceSysmemPartitionInfo *info);
SceUID sceKernelAllocPartitionMemory(SceUID partitionid, const char *name, int type, SceSize size, void *addr);

typedef struct
{   
    int id;
    int (*func)(void *, int, int funcid, void *args);
} SceSysmemUIDLookupFunction;

typedef struct SceSysmemUIDControlBlock
{   
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

typedef struct
{   
    int id;
    int (*func)();
} SceSysmemUIDLookupFunc;

SceSysmemUIDControlBlock *sceKernelCreateUIDtype(const char *name, int attr, SceSysmemUIDLookupFunc *funcs, int unk, SceSysmemUIDControlBlock **type);
SceUID sceKernelCreateUID(SceSysmemUIDControlBlock *type, const char *name, short attr, SceSysmemUIDControlBlock **block);
int sceKernelDeleteUID(SceUID uid);
int sceKernelCallUIDObjCommonFunction(SceSysmemUIDControlBlock *cb, int funcid, void *arg1, void *arg2, void *arg3, void *arg4, void *arg5, void *arg6);
int sceKernelRenameUID(SceUID uid, const char *name);
int sceKernelGetUIDcontrolBlock(SceUID uid, SceSysmemUIDControlBlock **block);
int sceKernelGetUIDcontrolBlockWithType(SceUID uid, SceSysmemUIDControlBlock* type, SceSysmemUIDControlBlock** block);

SceUID sceKernelCreateHeap(SceUID partitionid, SceSize size, int unk, const char *name);
void *sceKernelAllocHeapMemory(SceUID heapid, SceSize size);
int sceKernelFreeHeapMemory(SceUID heapid, void *block);
int sceKernelDeleteHeap(SceUID heapid);
SceSize sceKernelHeapTotalFreeSize(SceUID heapid);

void *sceKernelGetUsersystemLibWork(void);
void *sceKernelGetGameInfo(void);
void *sceKernelGetAWeDramSaveAddr(void);
int sceKernelGetMEeDramSaveAddr(void);
int sceKernelGetMEeDramSaveSize(void);

void *sceKernelMemset32(void *buf, int c, int size);

int sceKernelGetCompiledSdkVersion(void);
int sceKernelGetModel(void);

