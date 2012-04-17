/* Copyright (C) 2011, 2012 The uOFW team
   See the file COPYING for copying permission.
*/

#ifndef GLOBAL_H
#define GLOBAL_H

#include "errors.h"

#define PSP_SDK_VERSION(ver) const int module_sdk_version = ver

#define PSP_MODULE_BOOTSTART(name) int module_start(int arglen, void *argp) __attribute__((alias(name))); \
int module_bootstart(int arglen, void *argp) __attribute__((alias(name)))
#define PSP_MODULE_REBOOT_BEFORE(name) int module_reboot_before(void) __attribute__((alias(name)))
#define PSP_MODULE_STOP(name) int module_stop(void) __attribute__((alias(name)))

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

/********************************/

#if 0
#define NULL ((void*)0)
#endif

#define K1 27
#define GP 28
#define SP 29
#define RA 31

#define GET_REG(val, reg) asm("move %0, $%1" : "=r" (val) : "ri" (reg))
#define SET_REG(reg, val) asm("move $%1, %0" : : "r" (val), "ri" (reg))
#define MOV_REG(reg1, reg2) asm("move $%0, $%1" : : "ri" (reg1), "ri" (reg2))

/* TODO: is defining a local variable evil or not? */
#define K1_BACKUP() \
int _oldK1; \
GET_REG(_oldK1, K1); \
int _k1 = _oldK1 << 11; \
SET_REG(K1, _k1)
#define K1_RESET() SET_REG(K1, _oldK1)
#define K1_GET() \
int _k1; \
GET_REG(_k1, K1);
#define K1_GETOLD() _oldK1
#define K1_USER_PTR(ptr) (((s32)(void*)(ptr) & _k1) >= 0)
#define K1_USER_BUF_DYN_SZ(ptr, size) (((((s32)(void*)(ptr) + (s32)size) | (s32)(void*)(ptr) | (s32)size) & _k1) >= 0)
#define K1_USER_BUF_STA_SZ(ptr, size) (((((s32)(void*)(ptr) + (s32)size) | (s32)(void*)(ptr)            ) & _k1) >= 0)
#define K1_USER_MODE() ((_k1 >> 31) == 1)

#define GP_SET(val) MOV_REG(0, GP); SET_REG(GP, val)
#define GP_BACKUP() int _oldGp; GET_REG(_oldGp, GP)
#define GP_RESET() MOV_REG(0, GP); SET_REG(GP, _oldGp)

#define COP0_STATE_GET(val, reg) asm("mfc0 %0, $%1" : "=r" (val) : "ri" (reg))
#define COP0_STATE_SET(reg, val) asm("mtc0 %0, $%1" : : "r" (val), "ri" (reg))
#define COP0_CTRL_GET(val, reg) asm("cfc0 %0, $%1" : "=r" (val) : "ri" (reg))
#define COP0_CTRL_SET(reg, val) asm("ctc0 %0, $%1" : : "r" (val), "ri" (reg))

#define COP0_CTRL_EPC            0
#define COP0_CTRL_STATUS         2
#define COP0_CTRL_CAUSE          3
#define COP0_CTRL_V0             4
#define COP0_CTRL_V1             5
#define COP0_CTRL_EXC_TABLE      8
#define COP0_CTRL_NMI_HANDLER    9
#define COP0_CTRL_SC_TABLE      12
#define COP0_CTRL_IS_INTERRUPT  13
#define COP0_CTRL_SP_KERNEL     14
#define COP0_CTRL_SP_USER       15
#define COP0_CTRL_TCB           16
#define COP0_CTRL_NMI_TABLE     18
#define COP0_CTRL_23            23
#define COP0_CTRL_PROFILER_BASE 25

#define COP0_STATE_COUNT    9
#define COP0_STATE_COMPARE 11
#define COP0_STATE_STATUS  12
#define COP0_STATE_SCCODE  21
#define COP0_STATE_CPUID   22

#define CACHE(x, y) asm("cache %0, 0(%1)" : : "ri" (x), "r" (y))
#define SYNC() asm("sync")

// TODO: use assembly?
#define MAX(x, y) (((x) > (y)) ? (x) : (y))
#define MIN(x, y) (((x) < (y)) ? (x) : (y))

#define UCACHED(ptr) (void*)((u32)(void*)(ptr) & 0x1FFFFFFF)
#define KUNCACHED(ptr) (void*)(0xA0000000 | ((u32)(void*)(ptr) & 0x1FFFFFFF))
#define UUNCACHED(ptr) (void*)(0x40000000 | ((u32)(void*)(ptr) & 0x1FFFFFFF))

#define RESET_VECTOR(info, outAddr, func) \
{ \
    if (*(int*)(info + 4) > 0x2000000) \
        AT_SW((*(int*)(0xBC100040) & 0xFFFFFFFC) | 2, 0xBC100040); \
    else \
        AT_SW((*(int*)(0xBC100040) & 0xFFFFFFFC) | 1, 0xBC100040); \
    memset(0xBFC00000, 0, 0x1000); \
    memcpy(0xBFC00000, &func, &func + sizeof(func)); \
    *(int*)outAddr = 0xBFC00000; \
    int (*_resetVector)(int, int) = 0xBFC00000; \
    _resetVector(*(int*)(info + 0), *(int*)(info + 4)); \
}

#define UPALIGN256(v) (((v) + 0xFF) & 0xFFFFFF00)
#define UPALIGN64(v) (((v) + 0x3F) & 0xFFFFFFC0)

#endif

