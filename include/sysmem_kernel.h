/* Copyright (C) 2011, 2012 The uOFW team
   See the file COPYING for copying permission.
*/

#include "common_header.h"
#include <loadcore.h>

typedef struct {
  int unk_0; //0
  int num_export_libs; //4 //number of sysmem's export libraries - set in SysMemInit (from utopia)
  SceResidentLibraryEntry *export_lib[8]; //8 //array of sysmem's export tables set in SysMemInit (from utopia)
  int loadcore_addr; // 40//allocated in SysMemInit (from utopia)
  int user_lib_start; // 44 //offset in export_lib at which user libraries begin - set in SysMemInit (from utopia)
  int unk48; //48
  int unk52; //52
  int unk56;//56
  int unk60; //60
  SceLibraryStubTable *lc_stub; //64 //loadcore stubs - set in kactivate before booting loadcore (from utopia)
  int lc_stubsize; //68 //total size of stubs - set in kactivate before booting loadcore (from utopia)
  int init_thread_stack; //72 //allocated in SysMemInit (from utopia)
  SceLoadCoreExecFileInfo *sysmem_execinfo; //76 //set in kactivate before booting loadcore (from utopia)
  SceLoadCoreExecFileInfo *lc_execinfo; //80 //set in kactivate before booting loadcore (from utopia)
  int (*CompareSubType)(void); //84 //TODO is the arg really void?
  int (*CompreLatestSubType)(u32 val); //88
  s32 (*SetMaskFunction)(u32 unk1, u32 *addr); //92
  void (*kprintf_handler)(char *format, void *arg1, void *arg2, void *arg3, void *arg4, void *arg5, void *arg6, void *arg7); //96 //set by sysmem (from utopia)
  s32 (*GetLengthFunction)(s8 *file, u32 size, u32 *newSize); //100 //set in kactivate before booting loadcore (from utopia)
  s32 (*PrepareGetLengthFunction)(void); //104
  SceResidentLibraryEntry *export; //108 
} SysMemThreadConfig;

typedef struct {
    SceSize size;
    unsigned int startaddr;
    unsigned int memsize;
    unsigned int attr;
} SceSysmemPartitionInfo;

int sceKernelQueryMemoryPartitionInfo(int pid, SceSysmemPartitionInfo *info);

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

SceSysmemUIDControlBlock *sceKernelCreateUIDType(const char *name, int attr, SceSysmemUIDLookupFunc *funcs, int unk, SceSysmemUIDControlBlock **type);
SceUID sceKernelCreateUID(SceSysmemUIDControlBlock *type, const char *name, short attr, SceSysmemUIDControlBlock **block);
int sceKernelDeleteUID(SceUID uid);
int sceKernelCallUIDObjCommonFunction(SceSysmemUIDControlBlock *cb, int funcid, void *arg1, void *arg2, void *arg3, void *arg4, void *arg5, void *arg6);
int sceKernelRenameUID(SceUID uid, const char *name);
int sceKernelGetUIDcontrolBlock(SceUID uid, SceSysmemUIDControlBlock **block);
int sceKernelGetUIDcontrolBlockWithType(SceUID uid, SceSysmemUIDControlBlock* type, SceSysmemUIDControlBlock** block);
void SysMemForKernel_235C2646(SceSysmemUIDControlBlock *cb);

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

void *sceKernelMemcpy(void *dst, const void *src, u32 n);
void *sceKernelMemmove(void *dst, const void *src, u32 n);
void *sceKernelMemset32(void *buf, int c, int size);
void *sceKernelMemset(void *buf, int c, u32 size);

int sceKernelGetCompiledSdkVersion(void);
int sceKernelGetModel(void);

s32 SysMemForKernel_C4EEAF20(u32 unk0, s8 *buf);

