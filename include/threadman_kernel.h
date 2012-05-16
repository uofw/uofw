/* Copyright (C) 2011, 2012 The uOFW team
   See the file COPYING for copying permission.
*/

#include "common.h"

#define SCE_MEMORY_PARTITION_KERNEL  1

/* Threads */

typedef int (*SceKernelThreadEntry)(SceSize args, void *argp);

typedef struct
{
    SceSize     size;
    SceUID      stackMpid;
} SceKernelThreadOptParam;

SceUID sceKernelCreateThread(const char *name, SceKernelThreadEntry entry, int initPriority,
                             int stackSize, SceUInt attr, SceKernelThreadOptParam *option);
int sceKernelDeleteThread(SceUID thid);
int sceKernelStartThread(SceUID thid, SceSize arglen, void *argp);
int sceKernelExitThread(int status);
int sceKernelTerminateDeleteThread(SceUID thid);
int sceKernelDelayThread(SceUInt delay);
int sceKernelChangeThreadPriority(SceUID thid, int priority);
int sceKernelGetThreadCurrentPriority(void);
int sceKernelGetThreadId(void);
int sceKernelIsUserModeThread(void);

unsigned int sceKernelGetSystemTimeLow(void);
int sceKernelGetUserLevel(void);

typedef enum
{
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

int sceKernelCreateMutex(char *, int, int, int);
int sceKernelTryLockMutex(int, int);
int sceKernelLockMutex(int, int, int);
int sceKernelUnlockMutex(int, int);
int sceKernelDeleteMutex(int);

/* Event flags */

enum SceEventFlagWaitTypes
{
    /** Wait for all bits in the pattern to be set */
    SCE_EVENT_WAITAND = 0,
    /** Wait for one or more bits in the pattern to be set */
    SCE_EVENT_WAITOR = 1,
    /** Clear the wait pattern when it matches */
    SCE_EVENT_WAITCLEAR = 0x20
};

typedef struct
{
    SceSize     size;
    char        name[32];
    SceUInt     attr;
    SceUInt     initPattern;
    SceUInt     currentPattern;
    int         numWaitThreads;
} SceKernelEventFlagInfo;

typedef struct {
    SceSize     size;
} SceKernelEventFlagOptParam;

SceUID sceKernelCreateEventFlag(const char *name, int attr, int bits, SceKernelEventFlagOptParam *opt);
int sceKernelSetEventFlag(SceUID evid, u32 bits);
int sceKernelClearEventFlag(SceUID evid, u32 bits);
int sceKernelPollEventFlag(int evid, u32 bits, u32 wait, u32 *outBits);
int sceKernelWaitEventFlag(int evid, u32 bits, u32 wait, u32 *outBits, SceUInt *timeout);
int sceKernelWaitEventFlagCB(int evid, u32 bits, u32 wait, u32 *outBits, SceUInt *timeout);
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
        char    name[32];
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

typedef struct
{
    SceSize     size;
    char        name[32];
    SceUInt     attr;
    int         initCount;
    int         currentCount;
    int         maxCount;
    int         numWaitThreads;
} SceKernelSemaInfo;

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
int sceKernelNotifyCallback(SceUID cb, int arg2);

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
	char 	name[32];
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
    char 	name[32];
    SceUInt 	attr;
    int 	blockSize;
    int 	numBlocks;
    int 	freeBlocks;
    int 	numWaitThreads;
} SceKernelFplInfo;

int sceKernelReferFplStatus(SceUID uid, SceKernelFplInfo *info);

