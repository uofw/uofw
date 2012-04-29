/* Copyright (C) 2011, 2012 The uOFW team
   See the file COPYING for copying permission.
*/

typedef struct
{   
    // Handler address
    int handler; // 0
    // GP of the module
    int gp; // 4
    // Argument given by sceKernelRegisterSubIntrHandler
    int arg; // 8
    int u12, u16, u20;
    // See disableCb
    int enableCb; // 24
    // Pointer to the callback called by sceKernelDisableSubIntr(), that takes the same arguments as it
    int disableCb; // 28
    // See disableCb
    int suspendCb; // 32
    // See disableCb
    int resumeCb; // 36
    // See disableCb
    int isOccuredCb; // 40
    int u44;
    // Some options
    int v48; // 48
    int u52, u56, u60;
} SubInterrupt; // Size: 64

typedef struct
{   
    int size; // 0
    int u4;
    // Callback called before setting sub interrupt, when registering
    int (*cbRegBefore)(int, int, void*, void*); // 8
    // Callback called after
    int (*cbRegAfter)(int, int, void*, void*); // 12
    // Callback called before resetting handler to 0
    int (*cbRelBefore)(int, int); // 16
    // Callback called after
    int (*cbRelAfter)(int, int); // 20
    int (*cbEnable)(int, int); // 24
    int (*cbDisable)(int, int); // 28
    int (*cbSuspend)(int, int, int*); // 32
    int (*cbResume)(int, int, int); // 36
    int (*cbIsOccured)(int, int); // 40
} SceIntrCb; // Size: 44

// Arg4 in sceKernelRegisterIntrHandler()
typedef struct
{   
    // Handler address, sometimes OR'ed with 2 ?!?
    int handler; // 0
    // GP of the module
    int gp; // 4
    // Argument given by sceKernelRegisterIntrHandler
    int arg; // 8
    int u12, u16, u20, u24, u28, u32, u36;
    // Pointer to sub interrupts
    SubInterrupt *subIntrs; // 40
    // Some value set by sceKernelRegisterIntrHandler, using arg4 SubIntrInfo.callbacks, contains some handlers ran by sceKernelRegisterSubIntrHandler
    SceIntrCb *cb; // 44
    // InterruptManagerForKernel_D01EAA3F changes a bit depending on arg1, sceKernelRegisterIntrHandler changes some also; lower byte is the max number of sub interrupts
    int v48; // 48
    int u52, u56, u60;
} Interrupt; // Size: 64

typedef struct
{
    int size; // must be 12
    int numSubIntrs; // 4
    SceIntrCb *callbacks; // 8
} SubIntrInfo; // Size: 12

typedef struct
{
    int size;
    int attr;
    void *cb;
} SceIntrHandler;

typedef struct CbMap
{
    struct CbMap *next;
    int unk1, unk2, unk3;
    void (*callbacks[64])(void);
} CbMap;

typedef int (*MonitorCb)(int intrNum, int subIntrNum, int, int, int, int, char);

int IntrManInit();
int sceKernelRegisterIntrHandler(int intrNum, int arg1, void *func, int arg3, SceIntrHandler *handler);
int sceKernelSetUserModeIntrHanlerAcceptable(int intrNum, int subIntrNum, int setBit);
int sceKernelReleaseIntrHandler(int intrNum);
int sceKernelSetIntrLevel(int intrNum, int num);
int sceKernelSetIntrLogging(int intrNum, int arg1);
int sceKernelEnableIntr(int intNum);
int sceKernelSuspendIntr(int arg0, int arg1);
int sceKernelResumeIntr(int intrNum, int arg1);
void ReleaseContextHooks();
void InterruptManagerForKernel_E790EAED(int (*arg0)(), int (*arg1)());
int sceKernelCallSubIntrHandler(int intrNum, int subIntrNum, int arg2, int arg3);
int sceKernelGetUserIntrStack();
int sceKernelRegisterSubIntrHandler(int intrNum, int subIntrNum, void *handler, void *arg);
int sceKernelReleaseSubIntrHandler(int intrNum, int subIntrNum);
int sceKernelEnableSubIntr(int intrNum, int subIntrNum);
int sceKernelDisableSubIntr(int intrNum, int subIntrNum);
int sceKernelSuspendSubIntr(int intrNum, int subIntrNum, int *arg2);
int sceKernelResumeSubIntr(int intrNum, int subIntrNum, int arg2);
int sceKernelIsSubInterruptOccured(int intrNum, int subIntrNum);
int sceKernelQueryIntrHandlerInfo(int intrNum, int subIntrNum, int out);
void *mymemset(void *dstVoid, char c, int n);
void InterruptDisableInTable(int intrNum);
void sub_29B0(int intrNum);
void AllLevelInterruptDisable(int intrNum);
int sceKernelSetPrimarySyscallHandler(int arg0, void (*arg1)());
int IntrManTerminate();
void sceKernelCpuEnableIntr();
int InterruptManagerForKernel_6FCBA912(int set);
int sceKernelClearIntrLogging(int intrNum);
int sceKernelIsInterruptOccurred(int intrNum);
int sceKernelDisableIntr(int intrNum);
void RegisterSubIntrruptMonitor(MonitorCb before, MonitorCb after);
void ReleaseSubIntrruptMonitor();
int UnSupportIntr(int intrNum);
int InterruptManagerForKernel_8DFBD787();
int QueryIntrHandlerInfoForUser();
int sceKernelRegisterUserSpaceIntrStack(int addr, int size, int arg2);
int sceKernelGetCpuClockCounter();
u64 sceKernelGetCpuClockCounterWide();
u32 _sceKernelGetCpuClockCounterLow();
int sceKernelRegisterSystemCallTable(CbMap *newMap);
int sceKernelQuerySystemCall(int (*arg)());
void InterruptManagerForKernel_E526B767(int arg);
int SuspendIntc();
int ResumeIntc();

