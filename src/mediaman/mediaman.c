/* Copyright (C) 2011 - 2015 The uOFW team
   See the file COPYING for copying permission.
*/

/*
 * uofw/src/mediaman/mediaman.c
 * 
 * sceUmd_driver - an interface for the UMD Manager module.
 *
 * 1) Assigns the filesystem, the block device and the alias name for the UMD drive.
 * 
 * 2) Provides functions to handle UMD drive media changes.
 *
 */

#include <common_imp.h>
#include <interruptman.h>
#include <iofilemgr_kernel.h>
#include <modulemgr.h>
#include <sysmem_kernel.h>
#include <sysmem_sysclib.h>

#include "mediaman.h"
#include "umd_error.h"

SCE_MODULE_INFO(
	"sceUmd_driver", 
	SCE_MODULE_KERNEL | 
	SCE_MODULE_ATTR_CANT_STOP | SCE_MODULE_ATTR_EXCLUSIVE_LOAD | SCE_MODULE_ATTR_EXCLUSIVE_START, 
	1, 2
);	
									
SCE_MODULE_BOOTSTART("sceUmdModuleStart");
SCE_SDK_VERSION(SDK_VERSION);

#define SCE_UMD_FILE_SYSTEM_NAME_LEN        (32)

#define SCE_UMD_BLOCK_DEVICE_NAME           "umd0:"
#define SCE_UMD_FILE_SYSTEM_DEVICE_NAME     "isofs0:"

#define SCE_UMD_DEVICE_NOT_ASSIGNED         (0)
#define SCE_UMD_DEVICE_ASSIGNED             (2)

#define SCE_UMD_REPLACE_STATUS_PROHIBITED   (1 << 0)
#define SCE_UMD_REPLACE_STATUS_ALLOWED      (1 << 1)

/* Supported UMD drive states which can be waited for. */
#define UMD_SUPPORTED_WAIT_DRIVE_STATES     (SCE_UMD_MEDIA_OUT | SCE_UMD_MEDIA_IN | SCE_UMD_NOT_READY | SCE_UMD_READY \
                                              | SCE_UMD_READABLE)

/* This structure defines a Media Manager Control Block. */
typedef struct {
    u32 umdActivateCallbackGp;
    u32 umdDeactivateCallbackGp;
    u32 umdInfoCallbackGp;
	u32 umdMediaPresentCallbackGp;
    u32 umdReplaceCallbackGp;
    void *umdActivateCallbackParam;
    void *umdDeactivateCallbackParam;
    SceUmdDiscInfo *umdInfoCallbackDiscInfo;
	void *umdMediaPresentCallbackParam;
    u32 unk36;
    s32 (*umdInfoCallback)(SceUmdDiscInfo *info);
	s32 (*umdMediaPresentCallback)(void *param);
    s32 (*umdActivateCallback)(s32 mode, void *param);
    s32 (*umdDeactivateCallback)(s32 mode, void *param);
    s32 (*umdReplaceCallback)(s32 status);
    s32 (*unk60)(void);
} SceMediaMan;

static s32 sub_0000021C(s32 mode);
static s32 sub_00000278(s32 mode);

/* The control block for the media manager. */
SceMediaMan g_mediaMan;

/* Assign status of the UMD device. */
s32 g_isAssigned;

/* The ID of the UMD event flag. */
SceUID g_eventId;

/* The drive state of the UMD disc. One of ::SceUmdDiscStates. */
s32 g_driveState;

/* 
 * The error state of the UMD disc. One of the error codes defined in 
 * include/umd_error.h 
 */
s32 g_errorState;

/* The directory name of the current UMD file system. */
char g_umdFileSystem[SCE_UMD_FILE_SYSTEM_NAME_LEN];

s32 g_umdSuspendResumeMode;

/* The ID of the currently registered custom UMD callback. */
SceUID g_umdCallbackId;

//Subroutine _umdGetDiscInfo - Address 0x00000000
static s32 _umdGetDiscInfo(SceUmdDiscInfo *pDiscInfo)
{
    s32 oldGp;
    s32 status;
    
    if (pDiscInfo == NULL)
        return SCE_ERROR_ERRNO_INVALID_ARGUMENT;
    
    if (g_mediaMan.umdInfoCallback == NULL)
        return SCE_ERROR_UMD_NO_MEDIUM;
    
    oldGp = pspSetGp(g_mediaMan.umdInfoCallbackGp);
    
    status = g_mediaMan.umdInfoCallback(g_mediaMan.umdInfoCallbackDiscInfo);
    
    pspSetGp(oldGp);
    
    pDiscInfo->uiSize = sizeof(SceUmdDiscInfo); //0x00000068
    pDiscInfo->uiMediaType = g_mediaMan.umdInfoCallbackDiscInfo->uiMediaType;
    
    return status;
}

//Subroutine sub_00000090 - Address 0x00000090
static s32 _mountUMDDevice(s32 mode, const char *aliasName)
{
    s32 status;
    s32 data;
    s32 len;
    
    if (strcmp(aliasName, g_umdFileSystem) != 0) {
        if (g_isAssigned)
            return SCE_ERROR_ERRNO_DEVICE_BUSY;
            
        status = sceIoAssign(aliasName, SCE_UMD_BLOCK_DEVICE_NAME, SCE_UMD_FILE_SYSTEM_DEVICE_NAME, SCE_MT_RDONLY, 
                &data, sizeof data);
        if (status != SCE_ERROR_OK) {
            g_isAssigned = SCE_UMD_DEVICE_NOT_ASSIGNED;
            return status;
        }
    }
    status = sub_0000021C(mode);
    if (status != SCE_ERROR_OK) {
        g_isAssigned = SCE_UMD_DEVICE_NOT_ASSIGNED;
        return status;
    }
    len = strlen(aliasName);
    memcpy(g_umdFileSystem, aliasName, len);
    
    g_isAssigned = SCE_UMD_DEVICE_ASSIGNED;
    g_umdFileSystem[len] = '\0';
    return SCE_ERROR_OK;
}

//Subroutine sub_0000018C - Address 0x0000018C
static s32 _unmountUMDDevice(s32 mode, const char *aliasName)
{
    s32 status;
    
    status = SCE_ERROR_OK;
    if (g_isAssigned)
        status = sceIoUnassign(aliasName);
    if (status != SCE_ERROR_OK)
        return status;
    
    status = sub_00000278(mode);
    g_isAssigned = SCE_UMD_DEVICE_NOT_ASSIGNED;
    strncpy(g_umdFileSystem, "UMD:", SCE_UMD_FILE_SYSTEM_NAME_LEN); // TODO: ":DMU" ?
    
    return status;
}

//Subroutine sub_0000021C - Address 0x0000021C
static s32 sub_0000021C(s32 mode)
{
    s32 oldGp;
    s32 status;
    
    if (g_mediaMan.umdActivateCallback == NULL)
        return SCE_ERROR_UMD_NO_MEDIUM;
    
    oldGp = pspSetGp(g_mediaMan.umdActivateCallbackGp);
    
    status = g_mediaMan.umdActivateCallback(mode, g_mediaMan.umdActivateCallbackParam);
    
    pspSetGp(oldGp);
    return status;
}

//Subroutine sub_00000278 - Address 0x00000278 
static s32 sub_00000278(s32 mode)
{
    s32 oldGp;
    s32 status;
    
    if (g_mediaMan.umdDeactivateCallback == NULL)
        return SCE_ERROR_UMD_NO_MEDIUM;
    
    oldGp = pspSetGp(g_mediaMan.umdDeactivateCallbackGp);
    
    status = g_mediaMan.umdDeactivateCallback(mode, g_mediaMan.umdDeactivateCallbackParam);
    
    pspSetGp(oldGp);
    return status;
}

//Subroutine sceUmd_040A7090 - Address 0x000002CC
s32 sceUmd_040A7090(s32 errorState)
{
    s32 sdkVersion;
    
    if (errorState == SCE_ERROR_OK)
        return SCE_ERROR_OK;
    
    sdkVersion = sceKernelGetCompiledSdkVersion();
    if (sdkVersion != 0)
        return errorState;
    
    switch (errorState) {
    case SCE_ERROR_ERRNO_NAME_TOO_LONG:
        return SCE_ERROR_ERRNO150_ENAMETOOLONG;
    case SCE_ERROR_ERRNO_EADDRINUSE:
        return SCE_ERROR_ERRNO150_EADDRINUSE;
    case SCE_ERROR_ERRNO_CONNECTION_ABORTED:
        return SCE_ERROR_ERRNO150_ECONNABORTED;
    case SCE_ERROR_ERRNO_ETIMEDOUT:
        return SCE_ERROR_ERRNO150_ETIMEDOUT;
    case SCE_ERROR_ERRNO_NOT_SUPPORTED:
        return SCE_ERROR_ERRNO150_ENOTSUP;
    case SCE_ERROR_ERRNO_ENOMEDIUM:
        return SCE_ERROR_ERRNO150_ENOMEDIUM;
    case SCE_ERROR_ERRNO_WRONG_MEDIUM_TYPE:
        return SCE_ERROR_ERRNO150_EMEDIUMTYPE;
    default:
        return errorState;
    }
}

//Subroutine sceUmd_D2214D75 - Address 0x000003B8
SceUID sceUmdGetUserEventFlagId(void)
{
    return g_eventId;
}

//Subroutine sceUmd_B7BF4C31 - Address 0x000003C4
s32 sceUmdGetDriveStatus(void)
{
    return g_driveState;
}

//Subroutine sceUmd_4A908DDE - Address 0x000003D0
s32 sceUmdGetAssignedFlag(void)
{
    return g_isAssigned;
}

//Subroutine sceUmd_CB297D67 - Address 0x000003DC
void sceUmdSetAssignedFlag(s32 flag)
{
    g_isAssigned = flag;
}

//Subroutine sceUmd_6EDF57F1 - Address 0x000003E8
void sceUmdClearDriveStatus(s32 state)
{
    s32 intrState;
    
    intrState = sceKernelCpuSuspendIntr();
    
    sceKernelClearEventFlag(g_eventId, ~state);
    g_driveState &= (~state);
    
    sceKernelCpuResumeIntr(intrState);
}

//Subroutine sceUmd_982272FE - Address 0x00000444
void sceUmdSetDriveStatus(s32 state)
{
    s32 intrState;
    
    intrState = sceKernelCpuSuspendIntr();
    
    g_driveState |= state;
    sceKernelSetEventFlag(g_eventId, state);
    
    sceKernelCpuResumeIntr(intrState);
}

//Subroutine sceUmd_07E98AF8 - Address 0x0000049C
u32 sceUmdSetErrorStatus(s32 state) //param state: one of SCE_UMD_errors
{
    s32 intrState;
    
    intrState = sceKernelCpuSuspendIntr();
    
    g_errorState = state;
    
    sceKernelCpuResumeIntr(intrState);
    return SCE_ERROR_OK;
}

//Subroutine sceUmd_666580EA - Address 0x000004D0
s32 sceUmdGetErrorStatus(void)
{
    return g_errorState;
}

//Subroutine sub_000004DC - Address 0x000004DC
static s32 sub_000004DC(void)
{
    u32 oldGp;
    s32 status;
    
	if (g_mediaMan.umdMediaPresentCallback == NULL)
        return SCE_ERROR_OK;
    
	oldGp = pspSetGp(g_mediaMan.umdMediaPresentCallbackGp);
    
	status = g_mediaMan.umdMediaPresentCallback(g_mediaMan.umdMediaPresentCallbackParam);
    
    pspSetGp(oldGp);
    return status;
}

//Subroutine sceUmd_48EF868C - Address 0x0000053C
u32 sceUmdRegisterGetUMDInfoCallBack(s32 (*umdInfoCallback)(SceUmdDiscInfo *), SceUmdDiscInfo *pDiscInfo)
{
    s32 intrState;
    
    intrState = sceKernelCpuSuspendIntr();
    
    g_mediaMan.umdInfoCallback = umdInfoCallback;
    g_mediaMan.umdInfoCallbackDiscInfo = pDiscInfo;
    g_mediaMan.umdInfoCallbackGp = pspGetGp();
    
    sceKernelCpuResumeIntr(intrState);
    return SCE_ERROR_OK;
}

//Subroutine sceUmd_A0E8E513 - Address 0x00000590
u32 sceUmdUnRegisterGetUMDInfoCallBack(void)
{
    s32 intrState;
    
    intrState = sceKernelCpuSuspendIntr();
    
    g_mediaMan.umdInfoCallback = NULL;
    g_mediaMan.umdInfoCallbackGp = 0;
    g_mediaMan.umdInfoCallbackDiscInfo = NULL;
    
    sceKernelCpuResumeIntr(intrState);
    return SCE_ERROR_OK;
}

//Subroutine sceUmd_63517CBA - Address 0x000005CC 
u32 sceUmdRegisterMediaPresentCallBack(s32(*MediaPresentCallback)(void *), void *param)
{
    s32 intrState;
    
    intrState = sceKernelCpuSuspendIntr();
    
	g_mediaMan.umdMediaPresentCallback = MediaPresentCallback;
	g_mediaMan.umdMediaPresentCallbackParam = param;
	g_mediaMan.umdMediaPresentCallbackGp = pspGetGp();
    
    sceKernelCpuResumeIntr(intrState);
    return SCE_ERROR_OK;
}

//Subroutine sceUmd_1471F63D - Address 0x00000620
u32 sceUmdUnRegisterMediaPresentCallBack(void)
{
    s32 intrState;
    
    intrState = sceKernelCpuSuspendIntr();
    
	g_mediaMan.umdMediaPresentCallbackParam = NULL;
	g_mediaMan.umdMediaPresentCallbackGp = 0;
	g_mediaMan.umdMediaPresentCallback = NULL;
    
    sceKernelCpuResumeIntr(intrState);
    return SCE_ERROR_OK;
}

//Subroutine sceUmd_72B1C5B5 - Address 0x0000065C
void sceUmdUnRegisterActivateCallBack(void)
{
    s32 intrState;
    
    intrState = sceKernelCpuSuspendIntr();
    
    g_mediaMan.umdActivateCallback = NULL;
    g_mediaMan.umdActivateCallbackGp = 0;
    g_mediaMan.umdActivateCallbackParam = NULL;
    
    sceKernelCpuResumeIntr(intrState);
}

//Subroutine sceUmd_99CA645A - Address 0x00000694
void sceUmdUnRegisterDeactivateCallBack(void)
{
    s32 intrState;
    
    intrState = sceKernelCpuSuspendIntr();
    
    g_mediaMan.umdDeactivateCallback = NULL;
    g_mediaMan.umdDeactivateCallbackGp = 0;
    g_mediaMan.umdDeactivateCallbackParam = NULL;
    
    sceKernelCpuResumeIntr(intrState);
}

//Subroutine sceUmd_9B0F59CE - Address 0x00000704
u32 sceUmdRegisterActivateCallBack(s32 (*activateCallback)(s32, void *), void *param)
{
    s32 intrState;
    
    intrState = sceKernelCpuSuspendIntr();
    
    g_mediaMan.umdActivateCallback = activateCallback;
    g_mediaMan.umdActivateCallbackParam = param;
    g_mediaMan.umdActivateCallbackGp = pspGetGp();
    
    sceKernelCpuResumeIntr(intrState);
    return SCE_ERROR_OK;
}

//Subroutine sceUmd_D1C80E51 - Address 0x00000720
u32 sceUmdRegisterDeactivateCallBack(s32 (*deactivateCallback)(s32, void *), void *param)
{
    s32 intrState;
    
    intrState = sceKernelCpuSuspendIntr();
    
    g_mediaMan.umdDeactivateCallback = deactivateCallback;
    g_mediaMan.umdDeactivateCallbackParam = param;
    g_mediaMan.umdDeactivateCallbackGp = pspGetGp();
    
    sceKernelCpuResumeIntr(intrState);
    return SCE_ERROR_OK;
}

s32 sceUmdModuleStart(s32 argc, void *argp)
{   
    s32 intrState;

	(void)argc;
	(void)argp;
    
    memset(&g_mediaMan, 0, sizeof g_mediaMan);
    
    // TODO: Check assignment order; might be :DMU
    g_umdFileSystem[0] = 'U';
    g_umdFileSystem[1] = 'M';
    g_umdFileSystem[2] = 'D';
    g_umdFileSystem[3] = ':';
      
    g_eventId = sceKernelCreateEventFlag("SceMediaManUser", SCE_KERNEL_EA_MULTI | 0x1, 0, NULL);
    if (g_eventId < 0) 
        return SCE_KERNEL_NO_RESIDENT;
    
    g_isAssigned = SCE_UMD_DEVICE_NOT_ASSIGNED;
    intrState = sceKernelCpuSuspendIntr();
        
    g_driveState |= SCE_UMD_MEDIA_OUT;
    sceKernelSetEventFlag(g_eventId, 1);
        
    sceKernelCpuResumeIntr(intrState);
    return SCE_KERNEL_RESIDENT;
}

//Subroutine sceUmd_3748C4DB - Address 0x00000838
u32 sceUmdRegisterReplaceCallBack(s32 (*umdReplaceCallback)(s32))
{
    s32 intrState;
    
    intrState = sceKernelCpuSuspendIntr();
       
    g_mediaMan.umdReplaceCallback = umdReplaceCallback;
    g_mediaMan.umdReplaceCallbackGp = pspGetGp();
       
    sceKernelCpuResumeIntr(intrState);       
    return SCE_ERROR_OK;
}

//Subroutine sceUmd_0E3F8ED9 - Address 0x0000087C
u32 sceUmdUnRegisterReplaceCallBack(void)
{
    s32 intrState;
    
    intrState = sceKernelCpuSuspendIntr();
       
    g_mediaMan.umdReplaceCallback = NULL;
    g_mediaMan.umdReplaceCallbackGp = 0;
       
    sceKernelCpuResumeIntr(intrState);       
    return SCE_ERROR_OK;
}

//Subroutine sceUmd_76D356F9 - Address 0x000008B4
u32 sceUmd_76D356F9(s32 (*arg0)(void))
{
    s32 intrState;
    
    intrState = sceKernelCpuSuspendIntr();
       
    g_mediaMan.unk60 = arg0;
    g_mediaMan.unk36 = pspGetGp();
       
    sceKernelCpuResumeIntr(intrState);
    return SCE_ERROR_OK;
}

static s32 sub_0000094C(void)
{
    s32 status;
   
    if (g_mediaMan.unk60 == NULL)
        return SCE_ERROR_UMD_NO_MEDIUM;
 
    s32 oldGp = pspSetGp(g_mediaMan.unk36);
       
    status = g_mediaMan.unk60();
       
    pspSetGp(oldGp);
    return status;
}

//Subroutine sceUmd_C6183D47 - Address 0x00000A0C - Aliases: sceUmdUser_C6183D47
s32 sceUmdActivate(s32 mode, const char *aliasName)
{
    s32 oldK1;
    s32 status;
    
    if ((mode != SCE_UMD_MODE_POWER_ON && mode != SCE_UMD_MODE_POWER_CUR) || aliasName == NULL 
            || strncmp(aliasName, SCE_UMD_ALIAS_NAME, strlen(SCE_UMD_ALIAS_NAME) + 1) != 0)
        return SCE_ERROR_ERRNO_INVALID_ARGUMENT;
       
    oldK1 = pspShiftK1();   
    if (pspK1PtrOk(aliasName) < 0) {
        pspSetK1(oldK1);
        return SCE_ERROR_ERRNO_INVALID_ARGUMENT;
    }
    
    status = _mountUMDDevice(mode, aliasName);
    sceUmdSetErrorStatus(status);
       
    pspSetK1(oldK1);   
    return SCE_ERROR_OK;
}

// Subroutine sceUmd_E83742BA - Address 0x00000AC8 - Aliases: sceUmdUser_E83742BA
s32 sceUmdDeactivate(s32 mode, const char *aliasName)
{
    s32 oldK1;
    s32 status;
    
    if (mode < 0 || mode > 18 || (mode == SCE_UMD_MODE_POWER_CUR && aliasName == NULL))
        return SCE_ERROR_ERRNO_INVALID_ARGUMENT;
       
    oldK1 = pspShiftK1();
    if (pspK1PtrOk(aliasName) < 0) {
        pspSetK1(oldK1);
        return SCE_ERROR_ERRNO_INVALID_ARGUMENT;       
    }
 
    status = _unmountUMDDevice(mode, aliasName);
    status = sceUmdSetErrorStatus(status);
       
    pspSetK1(oldK1);   
    return status;
}

//Subroutine sceUmd_BA3D2A5F - Address 0x00000D24 - Aliases: sceUmdUser_340B7686
s32 sceUmdGetDiscInfo(SceUmdDiscInfo *pDiscInfo)
{
    s32 oldK1;
    s32 status;
       
    oldK1 = pspShiftK1();
        
    if (pDiscInfo == NULL || pspK1PtrOk(pDiscInfo) < 0 || pspK1StaBufOk(pDiscInfo, sizeof(SceUmdDiscInfo) - sizeof(u32)) < 0 
            || pDiscInfo->uiSize != sizeof(SceUmdDiscInfo)) {
        pspSetK1(oldK1);
        return SCE_ERROR_ERRNO_INVALID_ARGUMENT;
    }      
    status = _umdGetDiscInfo(pDiscInfo);
    
    pspSetK1(oldK1);
    return status;
}

//Subroutine sceUmd_5EBB491F - Address 0x00000C80 - Aliases: sceUmdUser_AEE7404D
s32 sceUmdRegisterUMDCallBack(SceUID callbackId)
{
    s32 oldK1;
    s32 intrState;
    
    oldK1 = pspShiftK1();
       
    if (sceKernelGetThreadmanIdType(callbackId) != SCE_KERNEL_TMID_Callback) {
        pspSetK1(oldK1);
        return SCE_ERROR_ERRNO_INVALID_ARGUMENT;
    }
    intrState = sceKernelCpuSuspendIntr();
    
    g_umdCallbackId = callbackId;
    
    sceKernelCpuResumeIntr(intrState);
    pspSetK1(oldK1);      
    return SCE_ERROR_OK;
}

//Subroutine sceUmd_598EC4DC - Address 0x000009A0 - Aliases: sceUmdUser_BD2BDE07
s32 sceUmdUnRegisterUMDCallBack(SceUID callbackId)
{   
    s32 oldK1 = pspShiftK1();
       
    if (callbackId != g_umdCallbackId) {
        pspSetK1(oldK1);
        return SCE_ERROR_ERRNO_INVALID_ARGUMENT;
    }
       
    g_umdCallbackId = 0;
       
    /* Only support PSP SDK versions >= 3.00 */
    if (sceKernelGetCompiledSdkVersion() >= 0x03000000) {
        pspSetK1(oldK1);
        return SCE_ERROR_OK;
    }
    pspSetK1(oldK1);
    return SCE_ERROR_ERRNO_INVALID_ARGUMENT;
}

//Subroutine sceUmd_A9B5B972 - Address 0x00000DA8 - Aliases: sceUmdUser_46EBB729
s32 sceUmdCheckMedium(void)
{
    s32 oldK1;
    s32 status;
    
    oldK1 = pspShiftK1();
    
    status = sub_000004DC();
       
    pspSetK1(oldK1);
    return status;
}

//Subroutine sceUmd_8EF08FCE - Address 0x00000DD4 - Aliases: sceUmdUser_8EF08FCE
s32 sceUmdWaitDriveStat(s32 umdState)
{
    s32 oldK1;
    SceUID eventId;
    u32 resultBits;
    s32 status;
        
    oldK1 = pspShiftK1();
    eventId = sceUmdGetUserEventFlagId();
        
    if (!(umdState & UMD_SUPPORTED_WAIT_DRIVE_STATES))
        return SCE_ERROR_ERRNO_INVALID_ARGUMENT; 
       
    status = sceKernelWaitEventFlag(eventId, umdState, SCE_KERNEL_EW_OR, &resultBits, NULL);
       
    pspSetK1(oldK1);
    return status;
}
 
//Subroutine sceUmd_8DCFBA06 - Address 0x00000B50 - Aliases: sceUmdUser_56202973
s32 sceUmdWaitDriveStatWithTimer(u32 umdState, u32 timeout)
{
    s32 oldK1;
    SceUID eventId;
    SceUInt *pTimeout;
    u32 resultBits;
    s32 status;
        
    oldK1 = pspShiftK1();
    eventId = sceUmdGetUserEventFlagId();
        
    if (!(umdState & UMD_SUPPORTED_WAIT_DRIVE_STATES))
        return SCE_ERROR_ERRNO_INVALID_ARGUMENT;
       
    if (timeout != 0)
        pTimeout = &timeout;
    else
        pTimeout = NULL;
       
    status = sceKernelWaitEventFlag(eventId, umdState, SCE_KERNEL_EW_OR, &resultBits, pTimeout);
    
    pspSetK1(oldK1);
    return status;
}

//Subroutine sceUmd_98AFBD10 - Address 0x00000BE8 - Aliases: sceUmdUser_4A9E5E29
s32 sceUmdWaitDriveStatCB(u32 umdState, u32 timeout)
{
    s32 oldK1;
    SceUID eventId;
    SceUInt *pTimeout;
    u32 outBits;
    s32 status;
        
    oldK1 = pspShiftK1();
    eventId = sceUmdGetUserEventFlagId();
        
    if (!(umdState & UMD_SUPPORTED_WAIT_DRIVE_STATES))
        return SCE_ERROR_ERRNO_INVALID_ARGUMENT;
       
    if (timeout != 0)
        pTimeout = &timeout;
    else
        pTimeout = NULL;
   
    status = sceKernelWaitEventFlagCB(eventId, umdState, SCE_KERNEL_EW_OR, &outBits, pTimeout);
    pspSetK1(oldK1);
    return status;
}

//Subroutine sceUmd_18E225C8 - Address 0x00000E38 - Aliases: sceUmdUser_6AF9B50A
s32 sceUmdCancelWaitDriveStat(void)
{
    s32 oldK1;
    SceUID eventId;
    s32 driveState;
    s32 status;
    
    eventId = sceUmdGetUserEventFlagId();
    oldK1 = pspShiftK1();
       
    driveState = sceUmdGetDriveStatus();
       
    status = sceKernelCancelEventFlag(eventId, driveState, NULL);
       
    pspSetK1(oldK1);
    return status;
}

//Subroutine sceUmdUser_6B4A146C - Address 0x00000E84
s32 sceUmdGetDriveStat(void)
{
    return sceUmdGetDriveStatus();
}

//Subroutine sceUmdUser_20628E6F - Address 0x00000EA0
s32 sceUmdGetErrorStat(void)
{
    s32 oldK1;
    s32 status;
    
    oldK1 = pspShiftK1();
       
    status = sceUmdGetErrorStatus();
    status = sceUmd_040A7090(status);
       
    pspSetK1(oldK1);
    return status;
}

//Subroutine sceUmd_A55109DD - Address 0x00000D9C
SceUID sceUmdGetDetectUMDCallBackId(void)
{   
    return g_umdCallbackId;
}

//Subroutine sceUmd_060E934D - Address 0x00000CF0
void sceUmdSetDetectUMDCallBackId(SceUID callbackId)
{
    s32 intrState;
    
    intrState = sceKernelCpuSuspendIntr();
    
    g_umdCallbackId = callbackId;
    
    sceKernelCpuResumeIntr(intrState);
}

//Subroutine sceUmd_899B5C41 - Address 0x00000ED4
s32 sceUmdGetSuspendResumeMode(void)
{
    return g_umdSuspendResumeMode;
}

//Subroutine sceUmd_816E656B - Address 0x00000EE0
void sceUmdSetSuspendResumeMode(s32 mode)
{   
    g_umdSuspendResumeMode = mode;
}

/* param replaceStatus: 1 prohibited, 2 allowed */
static s32 _setUmdReplaceStatus(u32 replaceStatus)
{
    s32 status;
   
    if (g_mediaMan.umdReplaceCallback == NULL)
        return SCE_ERROR_UMD_NO_MEDIUM;
 
    s32 oldGp = pspSetGp(g_mediaMan.umdReplaceCallbackGp);
       
    status = g_mediaMan.umdReplaceCallback(replaceStatus);
       
    pspSetGp(oldGp);
    return status;
}

//Subroutine sceUmd_4F017CDE - Address 0x00000EEC - Aliases: sceUmdUser_87533940
s32 sceUmdReplaceProhibit(void)
{
    s32 oldK1;
    s32 status;
    
    oldK1 = pspShiftK1();
    
    status = _setUmdReplaceStatus(SCE_UMD_REPLACE_STATUS_PROHIBITED);
    
    pspSetK1(oldK1);
    return status;
}

//Subroutine sceUmd_0B14CE61 - Address 0x00000F1C - Aliases: sceUmdUser_CBE9F02A
s32 sceUmdReplacePermit(void)
{
    s32 oldK1;
    s32 status;
    
    oldK1 = pspShiftK1();
    
    status = _setUmdReplaceStatus(SCE_UMD_REPLACE_STATUS_ALLOWED);
    
    pspSetK1(oldK1);
    return status;
}

//Subroutine sceUmd_AD1444AB - Address 0x00000F4C - Aliases: sceUmdUser_B103FA38
s32 sceUmdUseUMDInMsUsbWlan(void)
{
    s32 oldK1;
    s32 status;
    
    oldK1 = pspShiftK1();
    
    status = sub_0000094C();
    
    pspSetK1(oldK1);
    return status;       
}

//Subroutine sceUmd_F0C51280 - Address 0x00000F78 - Aliases: sceUmdUser_14C6C45C
s32 sceUmdUnuseUMDInMsUsbWlan(void)
{
    s32 oldK1;
    s32 status;
    
    oldK1 = pspShiftK1();
    
    status = _setUmdReplaceStatus(SCE_UMD_REPLACE_STATUS_PROHIBITED);
    
    pspSetK1(oldK1);
    return status;
}
