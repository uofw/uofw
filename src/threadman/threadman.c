SCE_MODULE_INFO(
        "sceThreadManager",
        SCE_MODULE_KERNEL |
        SCE_MODULE_ATTR_CANT_STOP | SCE_MODULE_ATTR_EXCLUSIVE_LOAD | SCE_MODULE_ATTR_EXCLUSIVE_START,
        1, 8);
// TODO: add SCE_MODULE_BOOTSTART, ...
SCE_SDK_VERSION(SDK_VERSION);


typedef struct {
    int pspCop0State; // 1992
} SceUnkStruct1;

SceUnkStruct1 g_UnkStruct1; // 0x00019F40

// TODO: Reverse function sub_00000080
// Subroutine sub_00000080 - Address 0x00000080
void sub_00000080()
{
    
}

// TODO: Reverse function sub_000001B8
// Subroutine sub_000001B8 - Address 0x000001B8
void sub_000001B8()
{
    
}

// TODO: Reverse function sub_000002C8
// Subroutine sub_000002C8 - Address 0x000002C8
void sub_000002C8()
{
    
}

// TODO: Reverse function sub_000005A4
// Subroutine sub_000005A4 - Address 0x000005A4
void sub_000005A4()
{
    
}

// TODO: Reverse function sub_00000680
// Subroutine sub_00000680 - Address 0x00000680
void sub_00000680()
{
    
}

// TODO: Reverse function sub_00000858
// Subroutine sub_00000858 - Address 0x00000858
void sub_00000858()
{
    
}

// TODO: Reverse function sub_00000890
// Subroutine sub_00000890 - Address 0x00000890
void sub_00000890()
{
    
}

// TODO: Reverse function sub_00000908
// Subroutine sub_00000908 - Address 0x00000908
void sub_00000908()
{
    
}

// TODO: Reverse function sub_00000924
// Subroutine sub_00000924 - Address 0x00000924
void sub_00000924()
{
    
}

// TODO: Reverse function sub_00000A08
// Subroutine sub_00000A08 - Address 0x00000A08
void sub_00000A08()
{
    
}

// TODO: Reverse function sub_00000B08
// Subroutine sub_00000B08 - Address 0x00000B08
void sub_00000B08()
{
    
}

// TODO: Reverse function sub_00000B48
// Subroutine sub_00000B48 - Address 0x00000B48
void sub_00000B48()
{
    
}

// TODO: Reverse function sub_00000BC4
// Subroutine sub_00000BC4 - Address 0x00000BC4
void sub_00000BC4()
{
    
}

// TODO: Reverse function sub_00000C30
// Subroutine sub_00000C30 - Address 0x00000C30
void sub_00000C30()
{
    
}

// TODO: Reverse function sub_00000C50
// Subroutine sub_00000C50 - Address 0x00000C50
void sub_00000C50()
{
    
}

// TODO: Reverse function sub_00000FE4
// Subroutine sub_00000FE4 - Address 0x00000FE4
void sub_00000FE4()
{
    
}

// TODO: Reverse function sceKernelRegisterThreadEventHandler
// Subroutine sceKernelRegisterThreadEventHandler - Address 0x00001140
SceUID sceKernelRegisterThreadEventHandler(const char *name, SceUID threadID, int mask, SceKernelThreadEventHandler handler, void *common)
{
    
}

// TODO: Reverse function sceKernelReleaseThreadEventHandler
// Subroutine sceKernelReleaseThreadEventHandler - Address 0x0000143C
int sceKernelReleaseThreadEventHandler(SceUID uid)
{
    
}

// TODO: Reverse function sub_00001580
// Subroutine sub_00001580 - Address 0x00001580
void sub_00001580()
{
    
}

// TODO: Reverse function sceKernelReferThreadEventHandlerStatus
// Subroutine sceKernelReferThreadEventHandlerStatus - Address 0x00001800
int sceKernelReferThreadEventHandlerStatus(SceUID uid, struct SceKernelThreadEventHandlerInfo *info)
{
    
}

// TODO: Reverse function sceKernelCreateCallback
// Subroutine sceKernelCreateCallback - Address 0x000019D8
int sceKernelCreateCallback(const char *name, SceKernelCallbackFunction func, void *arg)
{
    
}

// TODO: Reverse function sceKernelDeleteCallback
// Subroutine sceKernelDeleteCallback - Address 0x00001B80
int sceKernelDeleteCallback(SceUID cb)
{
    
}

// TODO: Reverse function sceKernelNotifyCallback
// Subroutine sceKernelNotifyCallback - Address 0x00001CC0
int sceKernelNotifyCallback(SceUID cb, int arg2)
{
    
}

// TODO: Reverse function sceKernelCancelCallback
// Subroutine sceKernelCancelCallback - Address 0x00001E74
int sceKernelCancelCallback(SceUID cb)
{
    
}

// TODO: Reverse function sceKernelGetCallbackCount
// Subroutine sceKernelGetCallbackCount - Address 0x00001F78
int sceKernelGetCallbackCount(SceUID cb)
{
    
}

// TODO: Reverse function sceKernelCheckCallback
// Subroutine sceKernelCheckCallback - Address 0x00002078
int sceKernelCheckCallback(void)
{
    
}

// TODO: Reverse function sub_00002170
// Subroutine sub_00002170 - Address 0x00002170
void sub_00002170()
{
    
}

// TODO: Reverse function sceKernelReferCallbackStatus
// Subroutine sceKernelReferCallbackStatus - Address 0x00002384
int sceKernelReferCallbackStatus(SceUID cb, SceKernelCallbackInfo *status)
{
    
}

// TODO: Reverse function _sceKernelReturnFromCallback
// Subroutine _sceKernelReturnFromCallback - Address 0x0000253C
void _sceKernelReturnFromCallback(void)
{
    
}

// TODO: Reverse function sub_00002570
// Subroutine sub_00002570 - Address 0x00002570
void sub_00002570()
{
    
}

// TODO: Reverse function sub_000025FC
// Subroutine sub_000025FC - Address 0x000025FC
void sub_000025FC()
{
    
}

// TODO: Reverse function sub_000026CC
// Subroutine sub_000026CC - Address 0x000026CC
void sub_000026CC()
{
    
}

// TODO: Reverse function ThreadManForUser_8672E3D0
// Subroutine ThreadManForUser_8672E3D0 - Address 0x0000272C
void ThreadManForUser_8672E3D0()
{
    
}

// TODO: Reverse function sub_000027B4
// Subroutine sub_000027B4 - Address 0x000027B4
void sub_000027B4()
{
    
}

// TODO: Reverse function sub_0000283C
// Subroutine sub_0000283C - Address 0x0000283C
void sub_0000283C()
{
    
}

// TODO: Reverse function sub_000028C0
// Subroutine sub_000028C0 - Address 0x000028C0
void sub_000028C0()
{
    
}

// TODO: Reverse function sub_00002944
// Subroutine sub_00002944 - Address 0x00002944
void sub_00002944()
{
    
}

// TODO: Reverse function sceKernelSleepThread
// Subroutine sceKernelSleepThread - Address 0x00002DF0
int sceKernelSleepThread(void)
{
    
}

// TODO: Reverse function sceKernelSleepThreadCB
// Subroutine sceKernelSleepThreadCB - Address 0x00002EF0
int sceKernelSleepThreadCB(void)
{
    
}

// TODO: Reverse function sceKernelWakeupThread
// Subroutine sceKernelWakeupThread - Address 0x00003050
int sceKernelWakeupThread(SceUID thid)
{
    
}

// TODO: Reverse function sceKernelCancelWakeupThread
// Subroutine sceKernelCancelWakeupThread - Address 0x00003218
int sceKernelCancelWakeupThread(SceUID thid)
{
    
}

// TODO: Reverse function sceKernelDonateWakeupThread
// Subroutine sceKernelDonateWakeupThread - Address 0x0000333C
void sceKernelDonateWakeupThread()
{
    
}

// TODO: Reverse function sceKernelSuspendThread
// Subroutine sceKernelSuspendThread - Address 0x0000357C
int sceKernelSuspendThread(SceUID thid)
{
    
}

// TODO: Reverse function sceKernelResumeThread
// Subroutine sceKernelResumeThread - Address 0x00003790
int sceKernelResumeThread(SceUID thid)
{
    
}

// TODO: Reverse function sceKernelSuspendAllUserThreads
// Subroutine sceKernelSuspendAllUserThreads - Address 0x00003984
int sceKernelSuspendAllUserThreads(void)
{
    
}

// TODO: Reverse function sceKernelWaitThreadEnd
// Subroutine sceKernelWaitThreadEnd - Address 0x00003AC0
int sceKernelWaitThreadEnd(SceUID thid, SceUInt *timeout)
{
    
}

// TODO: Reverse function sceKernelWaitThreadEndCB
// Subroutine sceKernelWaitThreadEndCB - Address 0x00003AC8
int sceKernelWaitThreadEndCB(SceUID thid, SceUInt *timeout)
{
    
}

// TODO: Reverse function sceKernelDelayThread
// Subroutine sceKernelDelayThread - Address 0x00003D78
int sceKernelDelayThread(SceUInt delay)
{
    
}

// TODO: Reverse function sceKernelDelayThreadCB
// Subroutine sceKernelDelayThreadCB - Address 0x00003D80
int sceKernelDelayThreadCB(SceUInt delay)
{
    
}

// TODO: Reverse function sceKernelDelaySysClockThread
// Subroutine sceKernelDelaySysClockThread - Address 0x00003F9C
int sceKernelDelaySysClockThread(SceKernelSysClock *delay)
{
    
}

// TODO: Reverse function sceKernelDelaySysClockThreadCB
// Subroutine sceKernelDelaySysClockThreadCB - Address 0x00003FA4
int sceKernelDelaySysClockThreadCB(SceKernelSysClock *delay)
{
    
}

// TODO: Reverse function sceKernelCreateSema
// Subroutine sceKernelCreateSema - Address 0x00004248
SceUID sceKernelCreateSema(const char *name, SceUInt attr, int initVal, int maxVal, SceKernelSemaOptParam *option)
{
    
}

// TODO: Reverse function sceKernelDeleteSema
// Subroutine sceKernelDeleteSema - Address 0x000043C4
int sceKernelDeleteSema(SceUID semaid)
{
    
}

// TODO: Reverse function sceKernelSignalSema
// Subroutine sceKernelSignalSema - Address 0x000044D8
int sceKernelSignalSema(SceUID semaid, int signal)
{
    
}

// TODO: Reverse function sceKernelWaitSema
// Subroutine sceKernelWaitSema - Address 0x0000474C
int sceKernelWaitSema(SceUID semaid, int signal, SceUInt *timeout)
{
    
}

// TODO: Reverse function sceKernelWaitSemaCB
// Subroutine sceKernelWaitSemaCB - Address 0x00004754
int sceKernelWaitSemaCB(SceUID semaid, int signal, SceUInt *timeout)
{
    
}

// TODO: Reverse function sceKernelPollSema
// Subroutine sceKernelPollSema - Address 0x00004A34
int sceKernelPollSema(SceUID semaid, int signal)
{
    
}

// TODO: Reverse function sceKernelCancelSema
// Subroutine sceKernelCancelSema - Address 0x00004BCC
void sceKernelCancelSema()
{
    
}

// TODO: Reverse function sub_00004D68
// Subroutine sub_00004D68 - Address 0x00004D68
void sub_00004D68()
{
    
}

// TODO: Reverse function sceKernelReferSemaStatus
// Subroutine sceKernelReferSemaStatus - Address 0x00004DF0
int sceKernelReferSemaStatus(SceUID semaid, SceKernelSemaInfo *info)
{
    
}

// TODO: Reverse function sub_00004FBC
// Subroutine sub_00004FBC - Address 0x00004FBC
void sub_00004FBC()
{
    
}

// TODO: Reverse function sceKernelCreateEventFlag
// Subroutine sceKernelCreateEventFlag - Address 0x00005138
SceUID sceKernelCreateEventFlag(const char *name, int attr, int bits, SceKernelEventFlagOptParam *opt)
{
    
}

// TODO: Reverse function sceKernelDeleteEventFlag
// Subroutine sceKernelDeleteEventFlag - Address 0x000052AC
int sceKernelDeleteEventFlag(int evid)
{
    
}

// TODO: Reverse function sceKernelSetEventFlag
// Subroutine sceKernelSetEventFlag - Address 0x000053C0
int sceKernelSetEventFlag(SceUID evid, u32 bits)
{
    
}

// TODO: Reverse function sceKernelClearEventFlag
// Subroutine sceKernelClearEventFlag - Address 0x00005618
int sceKernelClearEventFlag(SceUID evid, u32 bits)
{
    
}

// TODO: Reverse function sceKernelWaitEventFlag
// Subroutine sceKernelWaitEventFlag - Address 0x00005730
int sceKernelWaitEventFlag(int evid, u32 bits, u32 wait, u32 *outBits, SceUInt *timeout)
{
    
}

// TODO: Reverse function sceKernelWaitEventFlagCB
// Subroutine sceKernelWaitEventFlagCB - Address 0x00005738
int sceKernelWaitEventFlagCB(int evid, u32 bits, u32 wait, u32 *outBits, SceUInt *timeout)
{
    
}

// TODO: Reverse function sceKernelPollEventFlag
// Subroutine sceKernelPollEventFlag - Address 0x00005B70
int sceKernelPollEventFlag(int evid, u32 bits, u32 wait, u32 *outBits)
{
    
}

// TODO: Reverse function sceKernelCancelEventFlag
// Subroutine sceKernelCancelEventFlag - Address 0x00005DB4
void sceKernelCancelEventFlag()
{
    
}

// TODO: Reverse function sceKernelReferEventFlagStatus
// Subroutine sceKernelReferEventFlagStatus - Address 0x00005F10
int sceKernelReferEventFlagStatus(SceUID event, SceKernelEventFlagInfo *status)
{
    
}

// TODO: Reverse function sub_000060D4
// Subroutine sub_000060D4 - Address 0x000060D4
void sub_000060D4()
{
    
}

// TODO: Reverse function sub_00006120
// Subroutine sub_00006120 - Address 0x00006120
void sub_00006120()
{
    
}

// TODO: Reverse function sceKernelCreateMutex
// Subroutine sceKernelCreateMutex - Address 0x00006284
void sceKernelCreateMutex()
{
    
}

// TODO: Reverse function sceKernelDeleteMutex
// Subroutine sceKernelDeleteMutex - Address 0x000064F0
void sceKernelDeleteMutex()
{
    
}

// TODO: Reverse function sub_00006604
// Subroutine sub_00006604 - Address 0x00006604
void sub_00006604()
{
    
}

// TODO: Reverse function sub_000066F0
// Subroutine sub_000066F0 - Address 0x000066F0
void sub_000066F0()
{
    
}

// TODO: Reverse function sceKernelUnlockMutex
// Subroutine sceKernelUnlockMutex - Address 0x000067F4
void sceKernelUnlockMutex()
{
    
}

// TODO: Reverse function sceKernelLockMutex
// Subroutine sceKernelLockMutex - Address 0x00006A6C
void sceKernelLockMutex()
{
    
}

// TODO: Reverse function sceKernelLockMutexCB
// Subroutine sceKernelLockMutexCB - Address 0x00006A74
void sceKernelLockMutexCB()
{
    
}

// TODO: Reverse function sceKernelTryLockMutex
// Subroutine sceKernelTryLockMutex - Address 0x00006E54
void sceKernelTryLockMutex()
{
    
}

// TODO: Reverse function sceKernelCancelMutex
// Subroutine sceKernelCancelMutex - Address 0x0000705C
void sceKernelCancelMutex()
{
    
}

// TODO: Reverse function sceKernelReferMutexStatus
// Subroutine sceKernelReferMutexStatus - Address 0x00007258
void sceKernelReferMutexStatus()
{
    
}

// TODO: Reverse function sub_0000743C
// Subroutine sub_0000743C - Address 0x0000743C
void sub_0000743C()
{
    
}

// TODO: Reverse function sub_00007480
// Subroutine sub_00007480 - Address 0x00007480
void sub_00007480()
{
    
}

// TODO: Reverse function sceKernelCreateLwMutex
// Subroutine sceKernelCreateLwMutex - Address 0x000076D8
void sceKernelCreateLwMutex()
{
    
}

// TODO: Reverse function sceKernelDeleteLwMutex
// Subroutine sceKernelDeleteLwMutex - Address 0x00007908
void sceKernelDeleteLwMutex()
{
    
}

// TODO: Reverse function ThreadManForUser_BEED3A47
// Subroutine ThreadManForUser_BEED3A47 - Address 0x00007A5C
void ThreadManForUser_BEED3A47()
{
    
}

// TODO: Reverse function ThreadManForUser_7CFF8CF3
// Subroutine ThreadManForUser_7CFF8CF3 - Address 0x00007CD4
void ThreadManForUser_7CFF8CF3()
{
    
}

// TODO: Reverse function ThreadManForUser_31327F19
// Subroutine ThreadManForUser_31327F19 - Address 0x00007CDC
void ThreadManForUser_31327F19()
{
    
}

// TODO: Reverse function ThreadManForUser_71040D5C
// Subroutine ThreadManForUser_71040D5C - Address 0x00008094
void ThreadManForUser_71040D5C()
{
    
}

// TODO: Reverse function sceKernelReferLwMutexStatusByID
// Subroutine sceKernelReferLwMutexStatusByID - Address 0x000082C0
void sceKernelReferLwMutexStatusByID()
{
    
}

// TODO: Reverse function sub_000084B4
// Subroutine sub_000084B4 - Address 0x000084B4
void sub_000084B4()
{
    
}

// TODO: Reverse function sceKernelCreateMbx
// Subroutine sceKernelCreateMbx - Address 0x00008650
SceUID sceKernelCreateMbx(const char *name, SceUInt attr, SceKernelMbxOptParam *option)
{
    
}

// TODO: Reverse function sceKernelDeleteMbx
// Subroutine sceKernelDeleteMbx - Address 0x000087A8
int sceKernelDeleteMbx(SceUID mbxid)
{
    
}

// TODO: Reverse function sceKernelSendMbx
// Subroutine sceKernelSendMbx - Address 0x000088BC
int sceKernelSendMbx(SceUID mbxid, void *message)
{
    
}

// TODO: Reverse function sceKernelReceiveMbx
// Subroutine sceKernelReceiveMbx - Address 0x00008A68
int sceKernelReceiveMbx(SceUID mbxid, void **pmessage, SceUInt *timeout)
{
    
}

// TODO: Reverse function sceKernelReceiveMbxCB
// Subroutine sceKernelReceiveMbxCB - Address 0x00008A70
int sceKernelReceiveMbxCB(SceUID mbxid, void **pmessage, SceUInt *timeout)
{
    
}

// TODO: Reverse function sceKernelPollMbx
// Subroutine sceKernelPollMbx - Address 0x00008DC0
int sceKernelPollMbx(SceUID mbxid, void **pmessage)
{
    
}

// TODO: Reverse function sceKernelCancelReceiveMbx
// Subroutine sceKernelCancelReceiveMbx - Address 0x00008FF0
int sceKernelCancelReceiveMbx(SceUID mbxid, int *pnum)
{
    
}

// TODO: Reverse function sceKernelReferMbxStatus
// Subroutine sceKernelReferMbxStatus - Address 0x00009128
int sceKernelReferMbxStatus(SceUID mbxid, SceKernelMbxInfo *info)
{
    
}

// TODO: Reverse function sub_000092F4
// Subroutine sub_000092F4 - Address 0x000092F4
void sub_000092F4()
{
    
}

// TODO: Reverse function sub_00009338
// Subroutine sub_00009338 - Address 0x00009338
void sub_00009338()
{
    
}

// TODO: Reverse function sceKernelCreateMsgPipe
// Subroutine sceKernelCreateMsgPipe - Address 0x00009468
SceUID sceKernelCreateMsgPipe(const char *name, int part, int attr, void *unk1, void *opt)
{
    
}

// TODO: Reverse function sceKernelDeleteMsgPipe
// Subroutine sceKernelDeleteMsgPipe - Address 0x000096BC
int sceKernelDeleteMsgPipe(SceUID uid)
{
    
}

// TODO: Reverse function sceKernelSendMsgPipe
// Subroutine sceKernelSendMsgPipe - Address 0x000097D0
int sceKernelSendMsgPipe(SceUID uid, void *message, unsigned int size, int unk1, void *unk2, unsigned int *timeout)
{
    
}

// TODO: Reverse function sceKernelSendMsgPipeCB
// Subroutine sceKernelSendMsgPipeCB - Address 0x000097D8
int sceKernelSendMsgPipeCB(SceUID uid, void *message, unsigned int size, int unk1, void *unk2, unsigned int *timeout)
{
    
}

// TODO: Reverse function sceKernelTrySendMsgPipe
// Subroutine sceKernelTrySendMsgPipe - Address 0x00009D7C
int sceKernelTrySendMsgPipe(SceUID uid, void *message, unsigned int size, int unk1, void *unk2)
{
    
}

// TODO: Reverse function sceKernelReceiveMsgPipe
// Subroutine sceKernelReceiveMsgPipe - Address 0x0000A1A4
int sceKernelReceiveMsgPipe(SceUID uid, void *message, unsigned int size, int unk1, void *unk2, unsigned int *timeout)
{
    
}

// TODO: Reverse function sceKernelReceiveMsgPipeCB
// Subroutine sceKernelReceiveMsgPipeCB - Address 0x0000A1AC
int sceKernelReceiveMsgPipeCB(SceUID uid, void *message, unsigned int size, int unk1, void *unk2, unsigned int *timeout)
{
    
}

// TODO: Reverse function sceKernelTryReceiveMsgPipe
// Subroutine sceKernelTryReceiveMsgPipe - Address 0x0000A798
int sceKernelTryReceiveMsgPipe(SceUID uid, void *message, unsigned int size, int unk1, void *unk2)
{
    
}

// TODO: Reverse function sceKernelCancelMsgPipe
// Subroutine sceKernelCancelMsgPipe - Address 0x0000AC20
int sceKernelCancelMsgPipe(SceUID uid, int *psend, int *precv)
{
    
}

// TODO: Reverse function sceKernelReferMsgPipeStatus
// Subroutine sceKernelReferMsgPipeStatus - Address 0x0000ADD0
int sceKernelReferMsgPipeStatus(SceUID uid, SceKernelMppInfo *info)
{
    
}

// TODO: Reverse function sub_0000AFC0
// Subroutine sub_0000AFC0 - Address 0x0000AFC0
void sub_0000AFC0()
{
    
}

// TODO: Reverse function sub_0000B094
// Subroutine sub_0000B094 - Address 0x0000B094
void sub_0000B094()
{
    
}

// TODO: Reverse function sub_0000B16C
// Subroutine sub_0000B16C - Address 0x0000B16C
void sub_0000B16C()
{
    
}

// TODO: Reverse function sub_0000B1B4
// Subroutine sub_0000B1B4 - Address 0x0000B1B4
void sub_0000B1B4()
{
    
}

// TODO: Reverse function sceKernelCreateVpl
// Subroutine sceKernelCreateVpl - Address 0x0000B44C
SceUID sceKernelCreateVpl(const char *name, int part, int attr, unsigned int size, struct SceKernelVplOptParam *opt)
{
    
}

// TODO: Reverse function sceKernelDeleteVpl
// Subroutine sceKernelDeleteVpl - Address 0x0000B6C0
int sceKernelDeleteVpl(SceUID uid)
{
    
}

// TODO: Reverse function sceKernelAllocateVpl
// Subroutine sceKernelAllocateVpl - Address 0x0000B7D4
int sceKernelAllocateVpl(SceUID uid, unsigned int size, void **data, unsigned int *timeout)
{
    
}

// TODO: Reverse function sceKernelAllocateVplCB
// Subroutine sceKernelAllocateVplCB - Address 0x0000B7DC
int sceKernelAllocateVplCB(SceUID uid, unsigned int size, void **data, unsigned int *timeout)
{
    
}

// TODO: Reverse function sceKernelTryAllocateVpl
// Subroutine sceKernelTryAllocateVpl - Address 0x0000BB14
int sceKernelTryAllocateVpl(SceUID uid, unsigned int size, void **data)
{
    
}

// TODO: Reverse function sceKernelFreeVpl
// Subroutine sceKernelFreeVpl - Address 0x0000BCB4
int sceKernelFreeVpl(SceUID uid, void *data)
{
    
}

// TODO: Reverse function sceKernelCancelVpl
// Subroutine sceKernelCancelVpl - Address 0x0000BEFC
int sceKernelCancelVpl(SceUID uid, int *pnum)
{
    
}

// TODO: Reverse function sceKernelReferVplStatus
// Subroutine sceKernelReferVplStatus - Address 0x0000C034
int sceKernelReferVplStatus(SceUID uid, SceKernelVplInfo *info)
{
    
}

// TODO: Reverse function sub_0000C1FC
// Subroutine sub_0000C1FC - Address 0x0000C1FC
void sub_0000C1FC()
{
    
}

// TODO: Reverse function sub_0000C244
// Subroutine sub_0000C244 - Address 0x0000C244
void sub_0000C244()
{
    
}

// TODO: Reverse function sceKernelCreateFpl
// Subroutine sceKernelCreateFpl - Address 0x0000C408
int sceKernelCreateFpl(const char *name, int part, int attr, unsigned int size, unsigned int blocks, struct SceKernelFplOptParam *opt)
{
    
}

// TODO: Reverse function sceKernelDeleteFpl
// Subroutine sceKernelDeleteFpl - Address 0x0000C82C
int sceKernelDeleteFpl(SceUID uid)
{
    
}

// TODO: Reverse function sceKernelAllocateFpl
// Subroutine sceKernelAllocateFpl - Address 0x0000C940
int sceKernelAllocateFpl(SceUID uid, void **data, unsigned int *timeout)
{
    
}

// TODO: Reverse function sceKernelAllocateFplCB
// Subroutine sceKernelAllocateFplCB - Address 0x0000C948
int sceKernelAllocateFplCB(SceUID uid, void **data, unsigned int *timeout)
{
    
}

// TODO: Reverse function sceKernelTryAllocateFpl
// Subroutine sceKernelTryAllocateFpl - Address 0x0000CC1C
int sceKernelTryAllocateFpl(SceUID uid, void **data)
{
    
}

// TODO: Reverse function sceKernelFreeFpl
// Subroutine sceKernelFreeFpl - Address 0x0000CDBC
int sceKernelFreeFpl(SceUID uid, void *data)
{
    
}

// TODO: Reverse function sceKernelCancelFpl
// Subroutine sceKernelCancelFpl - Address 0x0000CFE0
int sceKernelCancelFpl(SceUID uid, int *pnum)
{
    
}

// TODO: Reverse function sceKernelReferFplStatus
// Subroutine sceKernelReferFplStatus - Address 0x0000D118
int sceKernelReferFplStatus(SceUID uid, SceKernelFplInfo *info)
{
    
}

// TODO: Reverse function sub_0000D2E4
// Subroutine sub_0000D2E4 - Address 0x0000D2E4
void sub_0000D2E4()
{
    
}

// TODO: Reverse function sub_0000D338
// Subroutine sub_0000D338 - Address 0x0000D338
void sub_0000D338()
{
    
}

// TODO: Reverse function sub_0000D38C
// Subroutine sub_0000D38C - Address 0x0000D38C
void sub_0000D38C()
{
    
}

// TODO: Reverse function sub_0000D3DC
// Subroutine sub_0000D3DC - Address 0x0000D3DC
void sub_0000D3DC()
{
    
}

// TODO: Reverse function sub_0000D42C
// Subroutine sub_0000D42C - Address 0x0000D42C
void sub_0000D42C()
{
    
}

// TODO: Reverse function sub_0000D474
// Subroutine sub_0000D474 - Address 0x0000D474
void sub_0000D474()
{
    
}

// TODO: Reverse function ThreadManForUser_8DAFF657
// Subroutine ThreadManForUser_8DAFF657 - Address 0x0000D574
void ThreadManForUser_8DAFF657()
{
    
}

// TODO: Reverse function ThreadManForUser_32BF938E
// Subroutine ThreadManForUser_32BF938E - Address 0x0000DA10
void ThreadManForUser_32BF938E()
{
    
}

// TODO: Reverse function ThreadManForUser_65F54FFB
// Subroutine ThreadManForUser_65F54FFB - Address 0x0000DC10
void ThreadManForUser_65F54FFB()
{
    
}

// TODO: Reverse function ThreadManForUser_4A719FB2
// Subroutine ThreadManForUser_4A719FB2 - Address 0x0000DEC8
void ThreadManForUser_4A719FB2()
{
    
}

// TODO: Reverse function sub_0000E0D4
// Subroutine sub_0000E0D4 - Address 0x0000E0D4
void sub_0000E0D4()
{
    
}

// TODO: Reverse function ThreadManForUser_721067F3
// Subroutine ThreadManForUser_721067F3 - Address 0x0000E23C
void ThreadManForUser_721067F3()
{
    
}

// TODO: Reverse function sub_0000E424
// Subroutine sub_0000E424 - Address 0x0000E424
void sub_0000E424()
{
    
}

// TODO: Reverse function sub_0000E4A4
// Subroutine sub_0000E4A4 - Address 0x0000E4A4
void sub_0000E4A4()
{
    
}

// TODO: Reverse function sub_0000E4EC
// Subroutine sub_0000E4EC - Address 0x0000E4EC
void sub_0000E4EC()
{
    
}

// TODO: Reverse function sub_0000E5EC
// Subroutine sub_0000E5EC - Address 0x0000E5EC
void sub_0000E5EC()
{
    
}

// TODO: Reverse function sub_0000EAE0
// Subroutine sub_0000EAE0 - Address 0x0000EAE0
void sub_0000EAE0()
{
    
}

// TODO: Reverse function sub_0000EB7C
// Subroutine sub_0000EB7C - Address 0x0000EB7C
void sub_0000EB7C()
{
    
}

// TODO: Reverse function sceKernelUSec2SysClock
// Subroutine sceKernelUSec2SysClock - Address 0x0000ECB4
int sceKernelUSec2SysClock(unsigned int usec, SceKernelSysClock *clock)
{
    
}

// TODO: Reverse function sub_0000ECE4
// Subroutine sub_0000ECE4 - Address 0x0000ECE4
void sub_0000ECE4()
{
    
}

// TODO: Reverse function sceKernelCancelAlarm
// Subroutine sceKernelCancelAlarm - Address 0x0000ED24
int sceKernelCancelAlarm(SceUID alarmid)
{
    
}

// TODO: Reverse function sub_0000EEB0
// Subroutine sub_0000EEB0 - Address 0x0000EEB0
void sub_0000EEB0()
{
    
}

// TODO: Reverse function sceKernelReferAlarmStatus
// Subroutine sceKernelReferAlarmStatus - Address 0x0000EF24
int sceKernelReferAlarmStatus(SceUID alarmid, SceKernelAlarmInfo *info)
{
    
}

// TODO: Reverse function sceKernelCreateVTimer
// Subroutine sceKernelCreateVTimer - Address 0x0000F0C4
SceUID sceKernelCreateVTimer(const char *name, struct SceKernelVTimerOptParam *opt)
{
    
}

// TODO: Reverse function sceKernelDeleteVTimer
// Subroutine sceKernelDeleteVTimer - Address 0x0000F1D4
int sceKernelDeleteVTimer(SceUID uid)
{
    
}

// TODO: Reverse function sceKernelGetVTimerBase
// Subroutine sceKernelGetVTimerBase - Address 0x0000F2E8
int sceKernelGetVTimerBase(SceUID uid, SceKernelSysClock *base)
{
    
}

// TODO: Reverse function sceKernelGetVTimerBaseWide
// Subroutine sceKernelGetVTimerBaseWide - Address 0x0000F450
SceInt64 sceKernelGetVTimerBaseWide(SceUID uid)
{
    
}

// TODO: Reverse function sceKernelGetVTimerTime
// Subroutine sceKernelGetVTimerTime - Address 0x0000F57C
int sceKernelGetVTimerTime(SceUID uid, SceKernelSysClock *time)
{
    
}

// TODO: Reverse function sceKernelGetVTimerTimeWide
// Subroutine sceKernelGetVTimerTimeWide - Address 0x0000F71C
SceInt64 sceKernelGetVTimerTimeWide(SceUID uid)
{
    
}

// TODO: Reverse function sceKernelSetVTimerTime
// Subroutine sceKernelSetVTimerTime - Address 0x0000F874
int sceKernelSetVTimerTime(SceUID uid, SceKernelSysClock *time)
{
    
}

// TODO: Reverse function sceKernelSetVTimerTimeWide
// Subroutine sceKernelSetVTimerTimeWide - Address 0x0000FB4C
SceInt64 sceKernelSetVTimerTimeWide(SceUID uid, SceInt64 time)
{
    
}

// TODO: Reverse function sceKernelStartVTimer
// Subroutine sceKernelStartVTimer - Address 0x0000FDB4
int sceKernelStartVTimer(SceUID uid)
{
    
}

// TODO: Reverse function sceKernelStopVTimer
// Subroutine sceKernelStopVTimer - Address 0x0000FFBC
int sceKernelStopVTimer(SceUID uid)
{
    
}

// TODO: Reverse function sceKernelSetVTimerHandler
// Subroutine sceKernelSetVTimerHandler - Address 0x00010194
int sceKernelSetVTimerHandler(SceUID uid, SceKernelSysClock *time, SceKernelVTimerHandler handler, void *common)
{
    
}

// TODO: Reverse function sceKernelSetVTimerHandlerWide
// Subroutine sceKernelSetVTimerHandlerWide - Address 0x00010438
int sceKernelSetVTimerHandlerWide(SceUID uid, SceInt64 time, SceKernelVTimerHandlerWide handler, void *common)
{
    
}

// TODO: Reverse function sceKernelCancelVTimerHandler
// Subroutine sceKernelCancelVTimerHandler - Address 0x000106B8
int sceKernelCancelVTimerHandler(SceUID uid)
{
    
}

// TODO: Reverse function sceKernelReferVTimerStatus
// Subroutine sceKernelReferVTimerStatus - Address 0x00010828
int sceKernelReferVTimerStatus(SceUID uid, SceKernelVTimerInfo *info)
{
    
}

// TODO: Reverse function _sceKernelReturnFromTimerHandler
// Subroutine _sceKernelReturnFromTimerHandler - Address 0x00010A60
void _sceKernelReturnFromTimerHandler(void)
{
    
}

// TODO: Reverse function sceKernelUSec2SysClockWide
// Subroutine sceKernelUSec2SysClockWide - Address 0x00010A68
SceInt64 sceKernelUSec2SysClockWide(unsigned int usec)
{
    
}

// TODO: Reverse function sceKernelSysClock2USec
// Subroutine sceKernelSysClock2USec - Address 0x00010A74
int sceKernelSysClock2USec(SceKernelSysClock *clock, unsigned int *low, unsigned int *high)
{
    
}

// TODO: Reverse function sceKernelSysClock2USecWide
// Subroutine sceKernelSysClock2USecWide - Address 0x00010B30
int sceKernelSysClock2USecWide(SceInt64 clock, unsigned *low, unsigned int *high)
{
    
}

// TODO: Reverse function sceKernelGetSystemTime
// Subroutine sceKernelGetSystemTime - Address 0x00010BE0
int sceKernelGetSystemTime(SceKernelSysClock *time)
{
    
}

// TODO: Reverse function sceKernelGetSystemTimeWide
// Subroutine sceKernelGetSystemTimeWide - Address 0x00010C98
SceInt64 sceKernelGetSystemTimeWide(void)
{
    
}

// TODO: Reverse function sceKernelGetSystemTimeLow
// Subroutine sceKernelGetSystemTimeLow - Address 0x00010D28
void sceKernelGetSystemTimeLow()
{
    
}

// TODO: Reverse function sub_00010D34
// Subroutine sub_00010D34 - Address 0x00010D34
void sub_00010D34()
{
    
}

// TODO: Reverse function sub_00010D8C
// Subroutine sub_00010D8C - Address 0x00010D8C
void sub_00010D8C()
{
    
}

// TODO: Reverse function sceKernelSetAlarm
// Subroutine sceKernelSetAlarm - Address 0x00010DF4
SceUID sceKernelSetAlarm(SceUInt clock, SceKernelAlarmHandler handler, void *common)
{
    
}

// TODO: Reverse function sceKernelSetSysClockAlarm
// Subroutine sceKernelSetSysClockAlarm - Address 0x00010EBC
SceUID sceKernelSetSysClockAlarm(SceKernelSysClock *clock, SceKernelAlarmHandler handler, void *common)
{
    
}

// TODO: Reverse function sub_00010FAC
// Subroutine sub_00010FAC - Address 0x00010FAC
void sub_00010FAC()
{
    
}

// TODO: Reverse function sub_00011104
// Subroutine sub_00011104 - Address 0x00011104
void sub_00011104()
{
    
}

// TODO: Reverse function sub_0001121C
// Subroutine sub_0001121C - Address 0x0001121C
void sub_0001121C()
{
    
}

// TODO: Reverse function sub_000113F0
// Subroutine sub_000113F0 - Address 0x000113F0
void sub_000113F0()
{
    
}

// TODO: Reverse function sub_0001147C
// Subroutine sub_0001147C - Address 0x0001147C
void sub_0001147C()
{
    
}

// TODO: Reverse function sub_00011574
// Subroutine sub_00011574 - Address 0x00011574
void sub_00011574()
{
    
}

// TODO: Reverse function sub_000116F4
// Subroutine sub_000116F4 - Address 0x000116F4
void sub_000116F4()
{
    
}

// TODO: Reverse function sceKernelAllocateKTLS
// Subroutine sceKernelAllocateKTLS - Address 0x00011A24
int sceKernelAllocateKTLS(int id, int (*cb)(unsigned int *size, void *arg), void *arg)
{
    
}

// TODO: Reverse function sceKernelFreeKTLS
// Subroutine sceKernelFreeKTLS - Address 0x00011B5C
int sceKernelFreeKTLS(int id)
{
    
}

// TODO: Reverse function sceKernelGetThreadKTLS
// Subroutine sceKernelGetThreadKTLS - Address 0x00011C68
void sceKernelGetThreadKTLS()
{
    
}

// TODO: Reverse function sub_00011E3C
// Subroutine sub_00011E3C - Address 0x00011E3C
void sub_00011E3C()
{
    
}

// TODO: Reverse function sceKernelGetKTLS
// Subroutine sceKernelGetKTLS - Address 0x00011F48
void sceKernelGetKTLS()
{
    
}

// TODO: Reverse function module_bootstart
// Subroutine module_bootstart - Address 0x00011F68
s32 module_bootstart(SceSize argc, void *argp)
{
    (void)argc;
    (void)argp;

    sceKernelCpuSuspendIntr();

    SysMemForKernel_22A114DC(&g_UnkStruct1, 0, 2052);

    // The content of coprocessor register $22 of the CP0 is loaded into general register $a3
    g_UnkStruct1.pspCop0State = pspCop0StateGet(22); // 0x00011FC0

    sub_00000C50(...);

    sub_00016B40();
    sub_0000283C(...);
    sub_00004FBC(...);
    sub_00006120(...);
    sub_0000743C(...);
    sub_000084B4(...);
    sub_000092F4(...);
    sub_0000B1B4(...);
    sub_0000C244(...);
    sub_0000D474(...);
    sub_0000E4EC(...);
    sub_0001121C(...);

    // TODO: Finish this function
}

// TODO: Reverse function sub_000126AC
// Subroutine sub_000126AC - Address 0x000126AC
void sub_000126AC()
{
    
}

// TODO: Reverse function sub_000127D0
// Subroutine sub_000127D0 - Address 0x000127D0
void sub_000127D0()
{
    
}

// TODO: Reverse function sub_00012828
// Subroutine sub_00012828 - Address 0x00012828
void sub_00012828()
{
    
}

// TODO: Reverse function sub_00012C04
// Subroutine sub_00012C04 - Address 0x00012C04
void sub_00012C04()
{
    
}

// TODO: Reverse function sceKernelCreateThread
// Subroutine sceKernelCreateThread - Address 0x00012CF8
void sceKernelCreateThread()
{
    
}

// TODO: Reverse function sceKernelDeleteThread
// Subroutine sceKernelDeleteThread - Address 0x000132B4
int sceKernelDeleteThread(SceUID thid)
{
    
}

// TODO: Reverse function sub_000134A8
// Subroutine sub_000134A8 - Address 0x000134A8
void sub_000134A8()
{
    
}

// TODO: Reverse function sceKernelStartThread
// Subroutine sceKernelStartThread - Address 0x0001351C
int sceKernelStartThread(SceUID thid, SceSize arglen, void *argp)
{
    
}

// TODO: Reverse function sceKernelExitThread
// Subroutine sceKernelExitThread - Address 0x000139C8
int sceKernelExitThread(int status)
{
    
}

// TODO: Reverse function sceKernelExitDeleteThread
// Subroutine sceKernelExitDeleteThread - Address 0x00013AC4
int sceKernelExitDeleteThread(int status)
{
    
}

// TODO: Reverse function sceKernelTerminateThread
// Subroutine sceKernelTerminateThread - Address 0x00013BC0
int sceKernelTerminateThread(SceUID thid)
{
    
}

// TODO: Reverse function sceKernelTerminateDeleteThread
// Subroutine sceKernelTerminateDeleteThread - Address 0x00013D38
int sceKernelTerminateDeleteThread(SceUID thid)
{
    
}

// TODO: Reverse function sceKernelSuspendDispatchThread
// Subroutine sceKernelSuspendDispatchThread - Address 0x00014034
int sceKernelSuspendDispatchThread(void)
{
    
}

// TODO: Reverse function sub_00014124
// Subroutine sub_00014124 - Address 0x00014124
void sub_00014124()
{
    
}

// TODO: Reverse function sceKernelResumeDispatchThread
// Subroutine sceKernelResumeDispatchThread - Address 0x00014240
int sceKernelResumeDispatchThread(int state)
{
    
}

// TODO: Reverse function sceKernelChangeCurrentThreadAttr
// Subroutine sceKernelChangeCurrentThreadAttr - Address 0x00014308
int sceKernelChangeCurrentThreadAttr(int unknown, SceUInt attr)
{
    
}

// TODO: Reverse function sub_00014598
// Subroutine sub_00014598 - Address 0x00014598
void sub_00014598()
{
    
}

// TODO: Reverse function sceKernelChangeThreadPriority
// Subroutine sceKernelChangeThreadPriority - Address 0x00014854
int sceKernelChangeThreadPriority(SceUID thid, int priority)
{
    
}

// TODO: Reverse function sceKernelRotateThreadReadyQueue
// Subroutine sceKernelRotateThreadReadyQueue - Address 0x00014A3C
int sceKernelRotateThreadReadyQueue(int priority)
{
    
}

// TODO: Reverse function sceKernelReleaseWaitThread
// Subroutine sceKernelReleaseWaitThread - Address 0x00014C20
int sceKernelReleaseWaitThread(SceUID thid)
{
    
}

// TODO: Reverse function sceKernelGetThreadExitStatus
// Subroutine sceKernelGetThreadExitStatus - Address 0x00014E10
int sceKernelGetThreadExitStatus(SceUID thid)
{
    
}

// TODO: Reverse function ThreadManForUser_A1F78052
// Subroutine ThreadManForUser_A1F78052 - Address 0x00014F10
void ThreadManForUser_A1F78052()
{
    
}

// TODO: Reverse function ThreadManForUser_28BFD974
// Subroutine ThreadManForUser_28BFD974 - Address 0x0001502C
void ThreadManForUser_28BFD974()
{
    
}

// TODO: Reverse function ThreadManForUser_BC80EC7C
// Subroutine ThreadManForUser_BC80EC7C - Address 0x0001512C
void ThreadManForUser_BC80EC7C()
{
    
}

// TODO: Reverse function sceKernelExtendKernelStack
// Subroutine sceKernelExtendKernelStack - Address 0x000152C0
int sceKernelExtendKernelStack(int type, void (*cb)(void*), void *arg)
{
    
}

// TODO: Reverse function sub_000155B8
// Subroutine sub_000155B8 - Address 0x000155B8
void sub_000155B8()
{
    
}

// TODO: Reverse function sub_000156BC
// Subroutine sub_000156BC - Address 0x000156BC
void sub_000156BC()
{
    
}

// TODO: Reverse function sceKernelCheckThreadStack
// Subroutine sceKernelCheckThreadStack - Address 0x000157C8
int sceKernelCheckThreadStack(void)
{
    
}

// TODO: Reverse function sceKernelGetThreadStackFreeSize
// Subroutine sceKernelGetThreadStackFreeSize - Address 0x00015928
int sceKernelGetThreadStackFreeSize(SceUID thid)
{
    
}

// TODO: Reverse function sceKernelGetThreadKernelStackFreeSize
// Subroutine sceKernelGetThreadKernelStackFreeSize - Address 0x00015A8C
int sceKernelGetThreadKernelStackFreeSize(SceUID thid)
{
    
}

// TODO: Reverse function sceKernelReferThreadStatus
// Subroutine sceKernelReferThreadStatus - Address 0x00015C04
int sceKernelReferThreadStatus(SceUID thid, SceKernelThreadInfo *info)
{
    
}

// TODO: Reverse function sceKernelReferThreadRunStatus
// Subroutine sceKernelReferThreadRunStatus - Address 0x00015F50
int sceKernelReferThreadRunStatus(SceUID thid, SceKernelThreadRunStatus *status)
{
    
}

// TODO: Reverse function ThreadManForKernel_2D69D086
// Subroutine ThreadManForKernel_2D69D086 - Address 0x000161EC
int ThreadManForKernel_2D69D086(SceUID uid, SceKernelThreadKInfo *info)
{
    
}

// TODO: Reverse function sceKernelGetThreadmanIdList
// Subroutine sceKernelGetThreadmanIdList - Address 0x000163E0
int sceKernelGetThreadmanIdList(enum SceKernelIdListType type, SceUID *readbuf, int readbufsize, int *idcount)
{
    
}

// TODO: Reverse function sceKernelGetThreadmanIdType
// Subroutine sceKernelGetThreadmanIdType - Address 0x00016784
void sceKernelGetThreadmanIdType()
{
    
}

// TODO: Reverse function ThreadManForKernel_C9BBE9A2
// Subroutine ThreadManForKernel_C9BBE9A2 - Address 0x0001696C
void ThreadManForKernel_C9BBE9A2()
{
    
}

// TODO: Reverse function ThreadManForKernel_D366D35A
// Subroutine ThreadManForKernel_D366D35A - Address 0x00016B1C
void ThreadManForKernel_D366D35A()
{
    
}

// TODO: Reverse function sub_00016B40
// Subroutine sub_00016B40 - Address 0x00016B40
void sub_00016B40(void)
{
    
}

// TODO: Reverse function module_reboot_before
// Subroutine module_reboot_before - Address 0x00016B84
void module_reboot_before()
{
    
}

// TODO: Reverse function ThreadManForKernel_B50F4E46
// Subroutine ThreadManForKernel_B50F4E46 - Address 0x00016BD8
void ThreadManForKernel_B50F4E46()
{
    
}

// TODO: Reverse function sub_00016BF4
// Subroutine sub_00016BF4 - Address 0x00016BF4
void sub_00016BF4()
{
    
}

// TODO: Reverse function sub_00016C48
// Subroutine sub_00016C48 - Address 0x00016C48
void sub_00016C48()
{
    
}

// TODO: Reverse function _sceKernelExitThread
// Subroutine _sceKernelExitThread - Address 0x00016D34
void _sceKernelExitThread(void)
{
    
}

// TODO: Reverse function sceKernelGetThreadId
// Subroutine sceKernelGetThreadId - Address 0x00016DAC
int sceKernelGetThreadId(void)
{
    
}

// TODO: Reverse function sceKernelGetThreadCurrentPriority
// Subroutine sceKernelGetThreadCurrentPriority - Address 0x00016E24
int sceKernelGetThreadCurrentPriority(void)
{
    
}

// TODO: Reverse function sceKernelGetUserLevel
// Subroutine sceKernelGetUserLevel - Address 0x00016E9C
int sceKernelGetUserLevel(void)
{
    
}

// TODO: Reverse function sceKernelIsUserModeThread
// Subroutine sceKernelIsUserModeThread - Address 0x00016F24
int sceKernelIsUserModeThread(void)
{
    
}

// TODO: Reverse function sub_00016FA4
// Subroutine sub_00016FA4 - Address 0x00016FA4
void sub_00016FA4()
{
    
}

// TODO: Reverse function sceKernelCheckThreadKernelStack
// Subroutine sceKernelCheckThreadKernelStack - Address 0x00017044
int sceKernelCheckThreadKernelStack(void)
{
    
}

// TODO: Reverse function sceKernelGetSystemStatusFlag
// Subroutine sceKernelGetSystemStatusFlag - Address 0x0001712C
void sceKernelGetSystemStatusFlag()
{
    
}

// TODO: Reverse function ThreadManForKernel_80CA77BA
// Subroutine ThreadManForKernel_80CA77BA - Address 0x00017138
void ThreadManForKernel_80CA77BA()
{
    
}

// TODO: Reverse function sceKernelReferSystemStatus
// Subroutine sceKernelReferSystemStatus - Address 0x00017148
int sceKernelReferSystemStatus(SceKernelSystemStatus *status)
{
    
}

// TODO: Reverse function sceKernelReferThreadProfiler
// Subroutine sceKernelReferThreadProfiler - Address 0x00017230
void sceKernelReferThreadProfiler()
{
    
}

// TODO: Reverse function sceKernelReferGlobalProfiler
// Subroutine sceKernelReferGlobalProfiler - Address 0x0001727C
void sceKernelReferGlobalProfiler()
{
    
}

// TODO: Reverse function sub_000172F8
// Subroutine sub_000172F8 - Address 0x000172F8
void sub_000172F8()
{
    
}

// TODO: Reverse function sub_0001752C
// Subroutine sub_0001752C - Address 0x0001752C
void sub_0001752C()
{
    
}

// TODO: Reverse function sub_000177C4
// Subroutine sub_000177C4 - Address 0x000177C4
void sub_000177C4()
{
    
}

// TODO: Reverse function sub_00017844
// Subroutine sub_00017844 - Address 0x00017844
void sub_00017844()
{
    
}

// TODO: Reverse function sub_000178C0
// Subroutine sub_000178C0 - Address 0x000178C0
void sub_000178C0()
{
    
}

// TODO: Reverse function sub_00017A14
// Subroutine sub_00017A14 - Address 0x00017A14
void sub_00017A14()
{
    
}

// TODO: Reverse function sub_00017EB4
// Subroutine sub_00017EB4 - Address 0x00017EB4
void sub_00017EB4()
{
    
}

// TODO: Reverse function sub_000180AC
// Subroutine sub_000180AC - Address 0x000180AC
void sub_000180AC()
{
    
}

// TODO: Reverse function sub_000183EC
// Subroutine sub_000183EC - Address 0x000183EC
void sub_000183EC()
{
    
}
