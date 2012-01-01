/* Copyright (C) 2011, 2012 The uOFW team
   See the file COPYING for copying permission.
*/

typedef struct
{   
    // Handler address
    int handler; // 0
    // Unknown result of unknown function LoadCoreForKernel_18CFDAA0(handler_addr);
    int loadCoreRes; // 4
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
    // Unknown result of unknown function LoadCoreForKernel_18CFDAA0(handler_addr);
    int loadCoreRes; // 4
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

// Memory map
// 0x5900 -> 0x595C: options array?
// 0x595C -> 0x5961: ???
// 0x5961          : some option used by InterruptManagerForKernel_352FB341
// 0x5962          : some option used by sceKernelRegisterIntrHandler
// 0x5963 -> 0x5970: ???
// 0x5970 -> 0x5974: stores the top of the user space interrupt stack
// 0x5974 -> 0x5978: stores some unknown option for the userspace interrupt stack
// 0x5978 -> 0x5980: ???
// 0x5980 -> 0x6A80: interruptions array
// 0x6A80 -> 0x6A84: sub interruptions memory pool id

