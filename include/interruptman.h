/* Copyright (C) 2011, 2012 The uOFW team
   See the file COPYING for copying permission.
*/

#include "common_header.h"
#include "loadcore.h"

enum SceInterrupts {
    SCE_GPIO_INT = 4,
    SCE_ATA_INT  = 5,
    SCE_UMD_INT  = 6,
    SCE_MSCM0_INT = 7,
    SCE_WLAN_INT  = 8,
    SCE_AUDIO_INT = 10,
    SCE_I2C_INT   = 12,
    SCE_SIRCS_INT = 14,
    SCE_SYSTIMER0_INT = 15,
    SCE_SYSTIMER1_INT = 16,
    SCE_SYSTIMER2_INT = 17,
    SCE_SYSTIMER3_INT = 18,
    SCE_THREAD0_INT   = 19,
    SCE_NAND_INT      = 20,
    SCE_DMACPLUS_INT  = 21,
    SCE_DMA0_INT      = 22,
    SCE_DMA1_INT      = 23,
    SCE_MEMLMD_INT    = 24,
    SCE_GE_INT        = 25,
    SCE_VBLANK_INT = 30,
    SCE_MECODEC_INT  = 31,
    SCE_HPREMOTE_INT = 36,
    SCE_MSCM1_INT    = 60,
    SCE_MSCM2_INT    = 61,
    SCE_THREAD1_INT  = 65,
    SCE_INTERRUPT_INT = 66
};

typedef enum SceKernelIntrVBlankSubIntr {
    SCE_KERNEL_INTR_VBLANK_SUB_INTR_CONTROLLER = 19,
    SCE_KERNEL_INTR_VBLANK_SUB_INTR_POWER      = 26,
};

typedef struct {   
    // Handler address
    s32 handler; // 0
    // GP of the module
    s32 gp; // 4
    // Argument given by sceKernelRegisterSubIntrHandler
    s32 arg; // 8
    s32 u12, u16, u20;
    // See disableCb
    s32 enableCb; // 24
    // Pointer to the callback called by sceKernelDisableSubIntr(), that takes the same arguments as it
    s32 disableCb; // 28
    // See disableCb
    s32 suspendCb; // 32
    // See disableCb
    s32 resumeCb; // 36
    // See disableCb
    s32 isOccuredCb; // 40
    s32 u44;
    // Some options
    s32 v48; // 48
    s32 u52, u56, u60;
} SubInterrupt; // Size: 64

typedef struct {   
    s32 size; // 0
    s32 u4;
    // Callback called before setting sub interrupt, when registering
    s32 (*cbRegBefore)(s32, s32, void*, void*); // 8
    // Callback called after
    s32 (*cbRegAfter)(s32, s32, void*, void*); // 12
    // Callback called before resetting handler to 0
    s32 (*cbRelBefore)(s32, s32); // 16
    // Callback called after
    s32 (*cbRelAfter)(s32, s32); // 20
    s32 (*cbEnable)(s32, s32); // 24
    s32 (*cbDisable)(s32, s32); // 28
    s32 (*cbSuspend)(s32, s32, s32*); // 32
    s32 (*cbResume)(s32, s32, s32); // 36
    s32 (*cbIsOccured)(s32, s32); // 40
} SceIntrCb; // Size: 44

// Arg4 in sceKernelRegisterIntrHandler()
typedef struct {   
    // Handler address, sometimes OR'ed with 2 ?!?
    s32 handler; // 0
    // GP of the module
    s32 gp; // 4
    // Argument given by sceKernelRegisterIntrHandler
    void *arg; // 8
    s32 u12, u16, u20, u24, u28, u32, u36;
    // Pointer to sub interrupts
    SubInterrupt *subIntrs; // 40
    // Some value set by sceKernelRegisterIntrHandler, using arg4 SubIntrInfo.callbacks, contains some handlers ran by sceKernelRegisterSubIntrHandler
    SceIntrCb *cb; // 44
    // InterruptManagerForKernel_D01EAA3F changes a bit depending on arg1, sceKernelRegisterIntrHandler changes some also; lower byte is the max number of sub interrupts
    s32 v48; // 48
    s32 u52, u56, u60;
} Interrupt; // Size: 64

typedef struct {
    s32 size; // must be 12
    s32 numSubIntrs; // 4
    SceIntrCb *callbacks; // 8
} SubIntrInfo; // Size: 12

typedef struct {
    s32 size;
    s32 attr;
    void *cb;
} SceIntrHandler;

typedef s32 (*MonitorCb)(s32 intrNum, s32 subIntrNum, s32, s32, s32, s32, s8);

s32 sceKernelRegisterIntrHandler(s32 intrNum, s32 arg1, void *func, void *arg3, SceIntrHandler *handler);
s32 sceKernelSetUserModeIntrHanlerAcceptable(s32 intrNum, s32 subIntrNum, s32 setBit);
s32 sceKernelReleaseIntrHandler(s32 intrNum);
s32 sceKernelSetIntrLevel(s32 intrNum, s32 num);
s32 sceKernelSetIntrLogging(s32 intrNum, s32 arg1);
s32 sceKernelEnableIntr(s32 intNum);
s32 sceKernelSuspendIntr(s32 arg0, s32 *arg1);
s32 sceKernelResumeIntr(s32 intrNum, s32 arg1);
void ReleaseContextHooks();
void InterruptManagerForKernel_E790EAED(s32 (*arg0)(), s32 (*arg1)());
s32 sceKernelCallSubIntrHandler(s32 intrNum, s32 subIntrNum, s32 arg2, s32 arg3);
s32 sceKernelGetUserIntrStack();
s32 sceKernelRegisterSubIntrHandler(s32 intrNum, s32 subIntrNum, void *handler, void *arg);
s32 sceKernelReleaseSubIntrHandler(s32 intrNum, s32 subIntrNum);
s32 sceKernelEnableSubIntr(s32 intrNum, s32 subIntrNum);
s32 sceKernelDisableSubIntr(s32 intrNum, s32 subIntrNum);
s32 sceKernelSuspendSubIntr(s32 intrNum, s32 subIntrNum, s32 *arg2);
s32 sceKernelResumeSubIntr(s32 intrNum, s32 subIntrNum, s32 arg2);
s32 sceKernelIsSubInterruptOccured(s32 intrNum, s32 subIntrNum);
s32 sceKernelQueryIntrHandlerInfo(s32 intrNum, s32 subIntrNum, s32 out);
s32 sceKernelSetPrimarySyscallHandler(s32 syscallId, void (*syscall)());
void sceKernelCpuEnableIntr();
s32 InterruptManagerForKernel_6FCBA912(s32 set);
s32 sceKernelClearIntrLogging(s32 intrNum);
s32 sceKernelIsInterruptOccurred(s32 intrNum);
s32 sceKernelDisableIntr(s32 intrNum);
void RegisterSubIntrruptMonitor(MonitorCb before, MonitorCb after);
void ReleaseSubIntrruptMonitor();
s32 UnSupportIntr(s32 intrNum);
s32 InterruptManagerForKernel_8DFBD787();
s32 QueryIntrHandlerInfoForUser();
s32 sceKernelRegisterUserSpaceIntrStack(s32 addr, s32 size, s32 arg2);
s32 sceKernelGetCpuClockCounter();
u64 sceKernelGetCpuClockCounterWide();
u32 _sceKernelGetCpuClockCounterLow();
s32 sceKernelRegisterSystemCallTable(SceSyscallTable *newMap);
s32 sceKernelQuerySystemCall(void (*sysc)());
void InterruptManagerForKernel_E526B767(s32 arg);
s32 sceKernelGetSyscallRA(void);
s32 sceKernelCpuSuspendIntr(void);
void sceKernelCpuResumeIntr(s32 intr);
void sceKernelCpuResumeIntrWithSync(s32 intr);
s32 sceKernelIsIntrContext(void);
int sceKernelCallUserIntrHandler(int, int, int, int, int, int);

