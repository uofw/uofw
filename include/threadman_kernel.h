/* Copyright (C) 2011 - 2015 The uOFW team
   See the file COPYING for copying permission.
*/

#ifndef THREADMAN_KERNEL_H
#define	THREADMAN_KERNEL_H

#include "common_header.h"
#include "threadman_user.h"

/* Threads */

typedef s32 (*SceKernelThreadEntry)(SceSize args, void *argp);

typedef s32 (*SceKernelRebootKernelThreadEntry)(s32 arg1, u32 arg2, s32 arg3, s32 arg4);

typedef struct {
    SceSize     size;
    SceUID      stackMpid;
} SceKernelThreadOptParam;

#define SCE_KERNEL_THREAD_ID_SELF               (0) /* UID representing calling thread. */

/** 
 * thread priority - lower numbers mean higher priority 
 */
#define SCE_KERNEL_INVALID_PRIORITY             (0)
#define SCE_KERNEL_HIGHEST_PRIORITY_KERNEL      (1)
#define SCE_KERNEL_HIGHEST_PRIORITY_USER        (16)
#define SCE_KERNEL_MODULE_INIT_PRIORITY         (32)
#define SCE_KERNEL_LOWEST_PRIORITY_USER         (111)
#define SCE_KERNEL_LOWEST_PRIORITY_KERNEL       (126)

/* thread size */
#define SCE_KERNEL_TH_KERNEL_DEFAULT_STACKSIZE  (SCE_KERNEL_4KiB) /* 4 KB */
#define SCE_KERNEL_TH_USER_DEFAULT_STACKSIZE    (SCE_KERNEL_256KiB) /* 256 KB */

/* thread attributes */
#define SCE_KERNEL_TH_VSH_MODE                  (0xC0000000) /* Thread runs in VSH mode. */
#define SCE_KERNEL_TH_APP_MODE                  (0xB0000000) /* Thread runs in Application mode. */
#define SCE_KERNEL_TH_USB_WLAN_MODE             (0xA0000000) /* Thread runs in USB_WLAN mode. */
#define SCE_KERNEL_TH_MS_MODE                   (0x90000000) /* Thread runs in MS mode. */
#define SCE_KERNEL_TH_USER_MODE                 (0x80000000) /* Thread runs in User mode. */
#define SCE_KERNEL_TH_NO_FILLSTACK              (0x00100000)
#define SCE_KERNEL_TH_CLEAR_STACK               (0x00200000) /* Specifies that thread memory area should be cleared to 0 when deleted. */
#define SCE_KERNEL_TH_LOW_STACK                 (0x00400000) /* Specifies that the stack area is allocated from the lower addresses in memory, not the higher ones. */
#define SCE_KERNEL_TH_UNK_800000                (0x00800000)
#define SCE_KERNEL_TH_USE_VFPU                  (0x00004000) /* Specifies that the VFPU is available. */
#define SCE_KERNEL_TH_NEVERUSE_FPU              (0x00002000)

#define SCE_KERNEL_TH_DEFAULT_ATTR              (0)

#define SCE_KERNEL_AT_THFIFO                    (0x00000000) /* Waiting threads are queued on a FIFO basis. */
#define SCE_KERNEL_AT_THPRI                     (0x00000100) /* Waiting threads are queued based on priority. */

SceUID sceKernelCreateThread(const char *name, SceKernelThreadEntry entry, s32 initPriority,
                             SceSize stackSize, SceUInt attr, SceKernelThreadOptParam *option);
int sceKernelDeleteThread(SceUID thid);
int sceKernelStartThread(SceUID thid, SceSize arglen, void *argp);
int sceKernelSuspendThread(SceUID thid);
int sceKernelExitThread(s32 status);
s32 sceKernelExitDeleteThread(s32 exitStatus);
int sceKernelTerminateDeleteThread(SceUID thid);
int sceKernelDelayThread(SceUInt delay);
int sceKernelChangeThreadPriority(SceUID thid, int priority);
int sceKernelGetThreadCurrentPriority(void);
s32 sceKernelGetThreadId(void);
int sceKernelIsUserModeThread(void);
int sceKernelWaitThreadEnd(SceUID thid, SceUInt *timeout);
int sceKernelWaitThreadEndCB(SceUID thid, SceUInt *timeout);
int sceKernelReleaseWaitThread(SceUID thid);
int sceKernelSuspendAllUserThreads(void);

int sceKernelExtendKernelStack(int type, s32 (*cb)(void*), void *arg);

unsigned int sceKernelGetSystemTimeLow(void);

enum SceUserLevel {
    SCE_USER_LEVEL_MS       = 1,
    SCE_USER_LEVEL_USBWLAN  = 2,
    SCE_USER_LEVEL_APP      = 3,
    SCE_USER_LEVEL_VSH      = 4,
};
int sceKernelGetUserLevel(void);

typedef enum {
    SCE_KERNEL_TMID_Thread = 1,
    SCE_KERNEL_TMID_Semaphore = 2,
    SCE_KERNEL_TMID_EventFlag = 3,
    SCE_KERNEL_TMID_Mbox = 4,
    SCE_KERNEL_TMID_Vpl = 5,
    SCE_KERNEL_TMID_Fpl = 6,
    SCE_KERNEL_TMID_Mpipe = 7,
    SCE_KERNEL_TMID_Callback = 8,
    SCE_KERNEL_TMID_ThreadEventHandler = 9,
    SCE_KERNEL_TMID_Alarm = 10,
    SCE_KERNEL_TMID_VTimer = 11,
    SCE_KERNEL_TMID_SleepThread = 64,
    SCE_KERNEL_TMID_DelayThread = 65,
    SCE_KERNEL_TMID_SuspendThread = 66,
    SCE_KERNEL_TMID_DormantThread = 67,
} SceKernelIdListType;

SceKernelIdListType sceKernelGetThreadmanIdType(SceUID uid);

/* Mutexes */

typedef struct {
    SceSize     size;
} SceKernelMutexOptParam;

typedef struct {
    SceSize     size;
    char        name[SCE_UID_NAME_LEN + 1];
    SceUInt     attr;
    s32         initCount;
    s32         currentCount;
    SceUID      currentOwner;
    s32         numWaitThreads;
} SceKernelMutexInfo;

/* Mutex attributes */
#define SCE_KERNEL_MUTEX_ATTR_TH_FIFO       (SCE_KERNEL_AT_THFIFO)
#define SCE_KERNEL_MUTEX_ATTR_TH_PRI        (SCE_KERNEL_AT_THPRI)
#define SCE_KERNEL_MUTEX_ATTR_RECURSIVE     (0x0200) /*Allow recursive locks by threads that own the mutex. */

s32 sceKernelCreateMutex(char *name, s32 attr, s32 initCount, const SceKernelMutexOptParam *pOption);
s32 sceKernelDeleteMutex(SceUID mutexId);
s32 sceKernelLockMutex(SceUID mutexId, s32 lockCount, u32 *pTimeout);
s32 sceKernelLockMutexCB(SceUID mutexId, s32 lockCount, u32 *pTimeout);
s32 sceKernelTryLockMutex(SceUID mutexId, s32 lockCount);
s32 sceKernelUnlockMutex(SceUID mutexId, s32 unlockCount);
s32 sceKernelCancelMutex(SceUID mutexId, s32 newLockCount, s32 *pNumWaitThreads);
s32 sceKernelReferMutexStatus(SceUID mutexId, SceKernelMutexInfo *pInfo);

/* Event flags */

typedef struct {
    SceSize     size;
    char        name[SCE_UID_NAME_LEN + 1];
    SceUInt     attr;
    SceUInt     initPattern;
    SceUInt     currentPattern;
    int         numWaitThreads;
} SceKernelEventFlagInfo;

typedef struct {
    SceSize     size;
} SceKernelEventFlagOptParam;

/* Event flag attributes. */
#define SCE_KERNEL_EA_SINGLE            (0x0000)    /** Multiple thread waits are prohibited. */ 
#define SCE_KERNEL_EA_MULTI             (0x0200)    /** Multiple thread waits are permitted. */

/* Event flag wait modes. */
#define SCE_KERNEL_EW_AND               (0x00)      /** Wait for all bits in the bit pattern to be set. */
#define SCE_KERNEL_EW_OR                (0x01)      /** Wait for one or more bits in the bit pattern to be set. */
#define SCE_KERNEL_EW_CLEAR_ALL         (0x10)      /** Clear all bits after wait condition is satisfied. */
#define SCE_KERNEL_EW_CLEAR_PAT         (0x20)      /** Clear bits specified by bit pattern after wait condition is satisfied. */
#define SCE_KERNEL_EW_CLEAR             SCE_KERNEL_EW_CLEAR_ALL

SceUID sceKernelCreateEventFlag(const char *name, int attr, int initBits, SceKernelEventFlagOptParam *optParam);
int sceKernelSetEventFlag(SceUID evid, u32 bits);
int sceKernelClearEventFlag(SceUID evid, u32 bits);
int sceKernelPollEventFlag(int evid, u32 bits, u32 wait, u32 *outBits);
int sceKernelWaitEventFlag(int evid, u32 bits, u32 wait, u32 *outBits, SceUInt *timeout);
int sceKernelWaitEventFlagCB(int evid, u32 bits, u32 wait, u32 *outBits, SceUInt *timeout);
int sceKernelCancelEventFlag(SceUID evid, SceUInt setpattern, s32 *numWaitThreads);
int sceKernelDeleteEventFlag(int evid);
int sceKernelReferEventFlagStatus(SceUID event, SceKernelEventFlagInfo *status);

/* MsgPipe */
SceUID sceKernelCreateMsgPipe(const char *name, int part, int attr, void *unk1, void *opt);
int sceKernelDeleteMsgPipe(SceUID uid);
int sceKernelSendMsgPipe(SceUID uid, void *message, unsigned int size, int unk1, void *unk2, unsigned int *timeout);
int sceKernelSendMsgPipeCB(SceUID uid, void *message, unsigned int size, int unk1, void *unk2, unsigned int *timeout);
int sceKernelTrySendMsgPipe(SceUID uid, void *message, unsigned int size, int unk1, void *unk2);
int sceKernelReceiveMsgPipe(SceUID uid, void *message, unsigned int size, int unk1, void *unk2, unsigned int *timeout);
int sceKernelReceiveMsgPipeCB(SceUID uid, void *message, unsigned int size, int unk1, void *unk2, unsigned int *timeout);
int sceKernelTryReceiveMsgPipe(SceUID uid, void *message, unsigned int size, int unk1, void *unk2);
int sceKernelCancelMsgPipe(SceUID uid, int *psend, int *precv);

typedef struct {
    SceSize size;
    char    name[SCE_UID_NAME_LEN + 1];
    SceUInt attr;
    int     bufSize;
    int     freeSize;
    int     numSendWaitThreads;
    int     numReceiveWaitThreads;
} SceKernelMppInfo;
 
int sceKernelReferMsgPipeStatus(SceUID uid, SceKernelMppInfo *info);

/* Semaphores */

typedef struct {
    SceSize     size;
} SceKernelSemaOptParam;

typedef struct {
    SceSize     size;
    char        name[SCE_UID_NAME_LEN + 1];
    SceUInt     attr;
    int         initCount;
    int         currentCount;
    int         maxCount;
    int         numWaitThreads;
} SceKernelSemaInfo;

#define SCE_KERNEL_SA_THFIFO    (0x0000)            /* A FIFO queue is used for the waiting thread */
#define SCE_KERNEL_SA_THPRI     (0x0100)            /* The waiting thread is queued by its thread priority */

SceUID sceKernelCreateSema(const char *name, SceUInt attr, int initVal, int maxVal, SceKernelSemaOptParam *option);
int sceKernelDeleteSema(SceUID semaid);
int sceKernelSignalSema(SceUID semaid, int signal);
int sceKernelWaitSema(SceUID semaid, int signal, SceUInt *timeout);
int sceKernelWaitSemaCB(SceUID semaid, int signal, SceUInt *timeout);
int sceKernelPollSema(SceUID semaid, int signal);
int sceKernelReferSemaStatus(SceUID semaid, SceKernelSemaInfo *info);

/* KTLS */
int sceKernelAllocateKTLS(int id, int (*cb)(unsigned int *size, void *arg), void *arg);
int sceKernelFreeKTLS(int id);
void *sceKernelGetKTLS(int id);
void *sceKernelGetThreadKTLS(int id, SceUID thid, int mode);

/* Alarms. */

typedef SceUInt (*SceKernelAlarmHandler)(void *common);

typedef struct {
	SceUInt32   low;
	SceUInt32   hi;
} SceKernelSysClock;

typedef struct {
	SceSize size;
	SceKernelSysClock schedule;
	SceKernelAlarmHandler handler;
	void *common;
} SceKernelAlarmInfo;

SceUID sceKernelSetAlarm(SceUInt clock, SceKernelAlarmHandler handler, void *common);
SceUID sceKernelSetSysClockAlarm(SceKernelSysClock *clock, SceKernelAlarmHandler handler, void *common);
int sceKernelCancelAlarm(SceUID alarmid);
int sceKernelReferAlarmStatus(SceUID alarmid, SceKernelAlarmInfo *info);

/* Callbacks */
typedef s32 (*SceKernelCallbackFunction)(s32 arg1, s32 arg2, void *arg);

typedef struct {
    SceSize size;
    char name[SCE_UID_NAME_LEN + 1];
    SceUID threadId;
    SceKernelCallbackFunction callback;
    void *common;
    s32 notifyCount;
    s32 notifyArg;
} SceKernelCallbackInfo;

SceUID sceKernelCreateCallback(const char* name, SceKernelCallbackFunction callback, void* common);
int sceKernelDeleteCallback(SceUID cbid);
int sceKernelNotifyCallback(SceUID cb, int arg2);
int sceKernelCancelCallback(SceUID cbid);
int sceKernelGetCallbackCount(SceUID cbid);
int sceKernelCheckCallback(void);
int sceKernelReferCallbackStatus(SceUID cb, SceKernelCallbackInfo *status);

/* VPL Functions */

typedef struct {
    SceSize 	size;
} SceKernelVplOptParam;


SceUID sceKernelCreateVpl(const char *name, int part, int attr, unsigned int size, SceKernelVplOptParam *opt);

int sceKernelDeleteVpl(SceUID uid);
int sceKernelAllocateVpl(SceUID uid, unsigned int size, void **data, unsigned int *timeout);
int sceKernelAllocateVplCB(SceUID uid, unsigned int size, void **data, unsigned int *timeout);
int sceKernelTryAllocateVpl(SceUID uid, unsigned int size, void **data);
int sceKernelFreeVpl(SceUID uid, void *data);
int sceKernelCancelVpl(SceUID uid, int *pnum);

typedef struct {
	SceSize 	size;
    char 	name[SCE_UID_NAME_LEN + 1];
	SceUInt 	attr;
	int 	poolSize;
	int 	freeSize;
	int 	numWaitThreads;
} SceKernelVplInfo;

int sceKernelReferVplStatus(SceUID uid, SceKernelVplInfo *info);

/* FPL Functions */

typedef struct {
    SceSize 	size;
} SceKernelFplOptParam;

int sceKernelCreateFpl(const char *name, int part, int attr, unsigned int size, unsigned int blocks, SceKernelFplOptParam *opt);
int sceKernelDeleteFpl(SceUID uid);
int sceKernelAllocateFpl(SceUID uid, void **data, unsigned int *timeout);
int sceKernelAllocateFplCB(SceUID uid, void **data, unsigned int *timeout);
int sceKernelTryAllocateFpl(SceUID uid, void **data);
int sceKernelFreeFpl(SceUID uid, void *data);
int sceKernelCancelFpl(SceUID uid, int *pnum);

typedef struct {
    SceSize 	size;
    char 	name[SCE_UID_NAME_LEN + 1];
    SceUInt 	attr;
    int 	blockSize;
    int 	numBlocks;
    int 	freeBlocks;
    int 	numWaitThreads;
} SceKernelFplInfo;

int sceKernelReferFplStatus(SceUID uid, SceKernelFplInfo *info);

s64 sceKernelGetSystemTimeWide(void);

#endif /* THREADMAN_KERNEL_H */

