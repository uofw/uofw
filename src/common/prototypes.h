/* This file is temporary: all the prototypes will be in the modules' headers */

#include <pspdisplay.h>
#include <pspinit.h>
#include <pspintrman.h>
#include <pspintrman_kernel.h>
#include <pspsysevent.h>
#include <pspsysmem.h>
#include <pspsysmem_kernel.h>
#include <pspsystimer.h>
#include <pspthreadman.h>
#include <pspthreadman_kernel.h>
#include <psputils.h>

/* Defines */

/** The PSP SDK defines this as PSP_POWER_TICK_ALL. Cancels all timers. */
#define SCE_KERNEL_POWER_TICK_DEFAULT           0 

/* syscon hardware controller transfer modes. */
#define SYSCON_CTRL_TRANSFER_DATA_DIGITAL_ONLY    7
#define SYSCON_CTRL_TRANSFER_DATA_DIGITAL_ANALOG  8


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
    int size;
    int (*ops[])();
} SceKernelDeci2Ops;

typedef struct _SceSysconPacket SceSysconPacket;

//copied from http://holdpsp.googlecode.com/svn/trunk/sysconhk.h
struct _SceSysconPacket {
    u8 unk00[4]; //0 -- (0x00,0x00,0x00,0x00)
    u8 unk04[2]; //4 -- (arg2)
    u8 status; //6
    u8 unk07; //7 -- (0x00)
    u8 unk08[4]; //8 -- (0xff,0xff,0xff,0xff)
    /** transmit data. */
    u8 tx_cmd; //12 -- command code
    u8 tx_len; //13 -- number of transmit bytes
    u8 tx_data[14]; //14 -- transmit parameters
    /** receive data. */
    u8 rx_sts; //28 --  generic status
    u8 rx_len; //29 --  receive length
    u8 rx_response; //30 --  response code(tx_cmd or status code)
    u8 rx_data[9]; //31 --  receive parameters
    u32 unk28; //40
    /** user callback (when finish an access?) */
    void (*callback)(SceSysconPacket *, u32); //44
    u32	callback_r28; //48
    u32	callback_arg2; //52 -- arg2 of callback (arg4 of sceSycconCmdExec)
    u8 unk38[13]; //56
    u8 old_sts;	//69 -- old rx_sts
    u8 cur_sts;	//70 --  current rx_sts
    u8 unk47[33]; //71
}; //size of SceSysconPacket: 96

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
int sceSysregAudioClkoutIoDisable(void);

int sceSysregAwRegABusClockEnable(void);
int sceSysregAwRegABusClockDisable(void);
int sceSysregAwRegBBusClockEnable(void);
int sceSysregAwRegBBusClockDisable(void);
int sceSysregAwEdramBusClockEnable(void);
int sceSysregAwEdramBusClockDisable(void);
int sceSysregAwResetEnable(void);
int sceSysregAwResetDisable(void);

int sceSysregSetMasterPriv(int, int);
int sceSysregSetAwEdramSize(int);

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

//int sceKernelGetUIDcontrolBlockWithType(SceUID uid, SceSysmemUIDControlBlock *type, SceSysmemUIDControlBlock **block);
//int sceKernelGetUIDcontrolBlock(SceUID uid, SceSysmemUIDControlBlock **block);
SceSysmemUIDControlBlock *sceKernelCreateUIDtype(const char *name, int attr, SceSysmemUIDLookupFunc *funcs, int unk, SceSysmemUIDControlBlock **type);
SceUID sceKernelCreateUID(SceSysmemUIDControlBlock *type, const char *name, short attr, SceSysmemUIDControlBlock **block);
int sceKernelDeleteUID(SceUID uid);
int sceKernelCallUIDObjCommonFunction(SceSysmemUIDControlBlock *cb, int funcid, void *arg1, void *arg2, void *arg3, void *arg4, void *arg5, void *arg6);

void *sceKernelGetUsersystemLibWork(void);
void *sceKernelGetGameInfo(void);
void *sceKernelGetAWeDramSaveAddr(void);
int sceSysregGetTachyonVersion(void);

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
int sceKernelPowerTick(int unk);

int sceKernelSetInitCallback(void *, int, int);

int sceKernelApplicationType();

void sceSyscon_driver_B72DDFD2(int);
int sceSyscon_driver_97765E27();
int sceSysconCmdExecAsync(SceSysconPacket *, int, int (*)(), int);

int sceSTimerSetPrscl(int timerId, int arg1, int arg2);

