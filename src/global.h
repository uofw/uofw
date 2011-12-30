/* Copyright (C) 2011 The uOFW team
   See the file COPYING for copying permission.
*/

#ifndef GLOBAL_H
#define GLOBAL_H

#include <stdint.h>

typedef uint64_t u64;
typedef uint32_t u32;
typedef uint16_t u16;
typedef uint8_t  u8;

typedef int64_t s64;
typedef int32_t s32;
typedef int16_t s16;
typedef int8_t  s8;

typedef s64 SceInt64;

typedef s32 SceUID;
typedef s32 SceMode;
typedef u32 SceSize;
typedef SceInt64 SceOff;

typedef struct
{
    unsigned short  modattribute;
    unsigned char   modversion[2];
    char            modname[27];
    char            terminal;
    void *          gp_value;
    void *          ent_top;
    void *          ent_end;
    void *          stub_top;
    void *          stub_end;
} _sceModuleInfo;

typedef const _sceModuleInfo SceModuleInfo;

extern char _gp[];

enum PspModuleInfoAttr
{
    PSP_MODULE_USER         = 0,
    PSP_MODULE_NO_STOP      = 0x0001,
    PSP_MODULE_SINGLE_LOAD  = 0x0002,
    PSP_MODULE_SINGLE_START = 0x0004,
    PSP_MODULE_KERNEL       = 0x1000,
};

/* Declare a module.  This must be specified in the source of a library or executable. */
#define PSP_MODULE_INFO(name, attributes, major_version, minor_version) \
    __asm__ (                                                       \
    "    .set push\n"                                               \
    "    .section .lib.ent.top, \"a\", @progbits\n"                 \
    "    .align 2\n"                                                \
    "    .word 0\n"                                                 \
    "__lib_ent_top:\n"                                              \
    "    .section .lib.ent.btm, \"a\", @progbits\n"                 \
    "    .align 2\n"                                                \
    "__lib_ent_bottom:\n"                                           \
    "    .word 0\n"                                                 \
    "    .section .lib.stub.top, \"a\", @progbits\n"                \
    "    .align 2\n"                                                \
    "    .word 0\n"                                                 \
    "__lib_stub_top:\n"                                             \
    "    .section .lib.stub.btm, \"a\", @progbits\n"                \
    "    .align 2\n"                                                \
    "__lib_stub_bottom:\n"                                          \
    "    .word 0\n"                                                 \
    "    .set pop\n"                                                \
    "    .text\n"                                                   \
    );                                                              \
    extern char __lib_ent_top[], __lib_ent_bottom[];                \
    extern char __lib_stub_top[], __lib_stub_bottom[];              \
    SceModuleInfo module_info                                       \
        __attribute__((section(".rodata.sceModuleInfo"),        \
                   , unused)) = {                \
      attributes, { minor_version, major_version }, name, 0, _gp,  \
      __lib_ent_top, __lib_ent_bottom,                              \
      __lib_stub_top, __lib_stub_bottom                             \
    }

#define PSP_SDK_VERSION(ver) const int syslib_11B97506 = ver

#define PSP_MODULE_BOOTSTART(name) int module_start(int arglen, void *argp) __attribute__((alias(name))); \
int module_bootstart(int arglen, void *argp) __attribute__((alias(name)))

/********** TEMPORARY: it'll be in their corresponding module's header *********/
typedef int (*SceKernelCallbackFunction)(int arg1, int arg2, void *arg);

typedef struct
{
    int size;
    char name[32];
    SceUID threadId;
    SceKernelCallbackFunction callback;
    void *common;
    int notifyCount;
    int notifyArg;
} SceKernelCallbackInfo;

typedef struct
{
    unsigned short  year;
    unsigned short  month;
    unsigned short  day;
    unsigned short  hour;
    unsigned short  minute;
    unsigned short  second;
    unsigned int    microsecond;
} ScePspDateTime;

typedef struct
{
    SceMode         st_mode;
    unsigned int    st_attr;
    SceOff          st_size;
    ScePspDateTime  st_ctime;
    ScePspDateTime  st_atime;
    ScePspDateTime  st_mtime;
    unsigned int    st_private[6];
} SceIoStat;

typedef struct
{
    int size;
    char* name;
    int type_mask;
    int (*handler)(int ev_id, char* ev_name, void* param, int* result);
    int r28;
    int busy;
    struct SceSysEventHandler *next;
    int reserved[9];
} SceSysEventHandler;

typedef struct
{
    SceSize size;
    u32 startAddr;
    u32 memSize;
    u32 attr;
} SceSysmemPartitionInfo;

typedef struct
{
    int id;
    int (*func)(void *, int, int funcid, void *args);
} SceSysmemUIDLookupFunction;

struct SceSysmemUIDControlBlock
{
    struct SceSysmemUIDControlBlock *parent; // 0
    struct SceSysmemUIDControlBlock *nextChild; // 4
    struct SceSysmemUIDControlBlock *type; // 8
    SceUID id; // 12
    char *name; // 16
    unsigned char unk; // 20
    unsigned char size; // size in words
    short attribute; // 22
    struct SceSysmemUIDControlBlock *nextEntry; // 24
    struct SceSysmemUIDControlBlock *inherited; // 28
    SceSysmemUIDLookupFunction *func_table; // 32
} __attribute__((packed));
typedef struct SceSysmemUIDControlBlock SceSysmemUIDControlBlock;

typedef struct
{
    SceSize size;
    char    name[32];
    u32     attr;
    int     bufSize;
    int     freeSize;
    int     numSendWaitThreads;
    int     numReceiveWaitThreads;
} SceKernelMppInfo;

typedef struct
{
    int size;
    void (*ops[14])();
} SceKernelDeci2Ops;

int sceKernelRegisterSuspendHandler(int no, void *func, int num); // sceSuspendForKernel_91A77137
int sceKernelRegisterResumeHandler(int no, void *func, int num); // sceSuspendForKernel_B43D1A8C
int sceKernelRegisterLibrary(char **name); // LoadCoreForKernel_211FEA3D
int sceKernelGetModuleGPByAddressForKernel(void *func); // LoadCoreForKernel_18CFDAA0

SceUID sceKernelCreateHeap(SceUID partitionid, SceSize size, int unk, const char *name); // SysMemForKernel_AF85EB1B
void *sceKernelAllocHeapMemory(SceUID heapid, SceSize size); // SysMemForKernel_6D161EE2
int sceKernelFreeHeapMemory(SceUID heapid, void *block); // SysMemForKernel_DB836ADB
void *sceKernelGetGameInfo(); // SysMemForKernel_EF29061C

// unsure
int sceCodec_driver_B2EF6B19(int);
int sceCodec_driver_431C0C8E(int freq);
int sceCodecOutputEnable(int, int);
int sceCodec_driver_E4D7F914(void);
int sceCodec_driver_F071BF60(int);
int sceCodec_driver_9681738F(int);
int sceCodec_driver_B0141A1B(char, char, char, char, char, char);
int sceCodec_driver_55F1788B(void);

// unsure
int sceKernelDmaOpQuit(u32*);
int sceKernelDmaOpAssign(u32*, int, int, int, int);
int sceKernelDmaOpSetCallback(u32*, int (*)(int, int), int);
int sceKernelDmaOpSetupLink(u32*, int, u32*);
int sceKernelDmaOpEnQueue(u32*);
int sceKernelDmaOpDeQueue(u32*);
u32 *sceKernelDmaOpAlloc(void);
int sceKernelDmaOpFree(u32*);
/********************************/

#define NULL ((void*)0)

#define K1 27
#define GP 28
#define SP 29

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
#define K1_USER_PTR(ptr) (((u32)(void*)(ptr) & _k1) >= 0)
#define K1_USER_BUF_DYN_SZ(ptr, size) (((((s32)(void*)(ptr) + size) | (s32)(void*)(ptr) | size) & _k1) >= 0)
#define K1_USER_BUF_STA_SZ(ptr, size) (((((s32)(void*)(ptr) + size) | (s32)(void*)(ptr)       ) & _k1) >= 0)
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
#define COP0_CTRL_SC_TABLE      12
#define COP0_CTRL_IS_INTERRUPT  13
#define COP0_CTRL_SP_KERNEL     14
#define COP0_CTRL_SP_USER       15
#define COP0_CTRL_TCB           16
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

#define UPALIGN256(v) ((v + 0xFF) & 0xFFFFFF00)

#endif

