/* This file is temporary: all the prototypes will be in the modules' headers */

#include <pspintrman.h>
#include <pspintrman_kernel.h>
#include <pspsysmem.h>
#include <pspthreadman.h>
#include <pspthreadman_kernel.h>
#include <psputils.h>

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

int sceKernelDeci2pRegisterOperations(void *op);
void *sceKernelDeci2pReferOperations();

int sceKernelRenameUID(SceUID uid, const char *name);

int InterruptManagerForKernel_A0F88036(void);

// unsure
int sceKernelDmaOpQuit(u32*);
int sceKernelDmaOpAssign(u32*, int, int, int, int);
int sceKernelDmaOpSetCallback(u32*, int (*)(int, int), int);
int sceKernelDmaOpSetupLink(u32*, int, u32*);
int sceKernelDmaOpEnQueue(u32*);
int sceKernelDmaOpDeQueue(u32*);
u32 *sceKernelDmaOpAlloc(void);
int sceKernelDmaOpFree(u32*);

int sceDdrFlush(int);

int sceClockgenAudioClkSetFreq(int);

int sceSysregAudioClkEnable(int);
int sceSysregAudioClkSelect(int, int);

int sceSysregAudioBusClockEnable(int);

int sceSysregAudioIoEnable(int);
int sceSysregAudioIoDisable(int);

int sceSysregAudioClkoutClkSelect(int);
int sceSysregAudioClkoutIoEnable(void);
int sceSysregAudioClkoutIoDisable();

int DmacManForKernel_E18A93A5(void*, void*);

SceUID sceKernelCreateHeap(SceUID partitionid, SceSize size, int unk, const char *name);
void *sceKernelAllocHeapMemory(SceUID heapid, SceSize size);
int sceKernelFreeHeapMemory(SceUID heapid, void *block);
int sceKernelDeleteHeap(SceUID heapid);
SceSize sceKernelHeapTotalFreeSize(SceUID heapid);

typedef struct
{
    int id;
    int (*func)();
} SceSysmemUIDLookupFunc;

int sceKernelGetUIDcontrolBlockWithType(SceUID uid, SceSysmemUIDControlBlock *type, SceSysmemUIDControlBlock **block);
int sceKernelGetUIDcontrolBlock(SceUID uid, SceSysmemUIDControlBlock **block);
SceSysmemUIDControlBlock *sceKernelCreateUIDtype(const char *name, int attr, SceSysmemUIDLookupFunc *funcs, int unk, SceSysmemUIDControlBlock **type);
SceUID sceKernelCreateUID(SceSysmemUIDControlBlock *type, const char *name, short attr, SceSysmemUIDControlBlock **block);
int sceKernelDeleteUID(SceUID uid);
int sceKernelCallUIDObjCommonFunction(SceSysmemUIDControlBlock *cb, int funcid, void *arg1, void *arg2, void *arg3, void *arg4, void *arg5, void *arg6);

void Kprintf(const char *format, ...);
int sceKernelGetUserLevel(void);

int sceKernelDipsw(int);

int sceKernelDebugWrite(SceUID fd, const void *data, SceSize size);
int sceKernelDebugRead(SceUID fd, const void *data, SceSize size);
int sceKernelDebugEcho(void);

int sceKernelPowerLock(int);
int sceKernelPowerLockForUser(int);
int sceKernelPowerUnlock(int);
int sceKernelPowerUnlockForUser(int);

