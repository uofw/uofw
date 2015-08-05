/* Copyright (C) 2011 - 2015 The uOFW team
   See the file COPYING for copying permission.
*/

#include "common_header.h"

#include "mediaman.h"

/** @defgroup MediamanKernel Mediaman Kernel
 *  @ingroup Mediaman
 * 
 *  Kernel application API.
 * @{
 */

#ifndef MEDIAMAN_KERNEL_H
#define	MEDIAMAN_KERNEL_H

#ifdef	__cplusplus
extern "C" {
#endif

/**
 * Get corresponding 1.50 error code used by the UMD drivers. On 1.50, a bunch of error codes were \n
 * mistakenly returned by the UMD modules.
 * 
 * @param errorState Get the 1.50 version of this error code.
 * 
 * @return The error code used on 1.50.
 */
s32 sceUmd_040A7090(s32 errorState);

SceUID sceUmdGetUserEventFlagId(void);

s32 sceUmdGetDriveStatus(void);

s32 sceUmdGetAssignedFlag(void);

void sceUmdSetAssignedFlag(s32 flag);

void sceUmdClearDriveStatus(s32 state);

void sceUmdSetDriveStatus(s32 state);

u32 sceUmdSetErrorStatus(s32 state);

s32 sceUmdGetErrorStatus(void);

u32 sceUmdRegisterGetUMDInfoCallBack(s32 (*umdInfoCallback)(SceUmdDiscInfo *), SceUmdDiscInfo *pDiscInfo);

u32 sceUmdUnRegisterGetUMDInfoCallBack(void);

u32 sceUmdRegisterMediaPresentCallBack(s32(*MediaPresentCallback)(void *), void *param);

u32 sceUmdUnRegisterMediaPresentCallBack(void);

void sceUmdUnRegisterActivateCallBack(void);

void sceUmdUnRegisterDeactivateCallBack(void);

u32 sceUmdRegisterActivateCallBack(s32 (*activateCallback)(s32, void *), void *param);

u32 sceUmdRegisterDeactivateCallBack(s32 (*deactivateCallback)(s32, void *), void *param);

u32 sceUmdRegisterReplaceCallBack(s32 (*umdReplaceCallback)(s32));

u32 sceUmdUnRegisterReplaceCallBack(void);

u32 sceUmd_76D356F9(s32 (*arg0)(void));
    
/**
 * Activate the UMD drive. This includes assigning the file system, the block device (set to "umd0:")
 * and setting the alias name for the file system access.
 * 
 * @param mode The initial UMD drive power mode. One of ::SceUmdDevicePowerModes.
 * @param aliasName The alias name for the mounted filesystem device name. Pass ::SCE_UMD_ALIAS_NAME.
 * 
 * @return SCE_ERROR_OK on success, otherwise SCE_ERROR_ERRNO_INVALID_ARGUMENT.
 */
s32 sceUmdActivate(s32 mode, const char *aliasName);

/**
 * Deactivate the UMD drive.
 * 
 * @param mode The new UMD drive power mode. One of ::SceUmdDevicePowerModes.
 * @param aliasName aliasName The alias name for the mounted filesystem device name. Pass ::SCE_UMD_ALIAS_NAME.
 * 
 * @return SCE_ERROR_OK on success, otherwise SCE_ERROR_ERRNO_INVALID_ARGUMENT.
 */
s32 sceUmdDeactivate(s32 mode, const char *aliasName);

/**
 * Get the disc information
 * 
 * @param pDiscInfo Pointer to a SceUmdDiscInfo structure to retrieve the disc information.
 * 
 * @return SCE_ERROR_OK on success, otherwise SCE_ERROR_ERRNO_INVALID_ARGUMENT on invalid arguments; 
 *         SCE_ERROR_UMD_NO_MEDIUM if there is no UMD medium inserted.
 */
s32 sceUmdGetDiscInfo(SceUmdDiscInfo *pDiscInfo);

/**
 * Register a callback that is called when the UMD disc state changes. Only exactly one callback can be
 * registered at any given time.
 * 
 * @param callbackId The ID of the callback.
 * 
 * @return SCE_ERROR_OK on success, otherwise SCE_ERROR_ERRNO_INVALID_ARGUMENT (when the passed ID does
 *         not belong to a callback).
 */
s32 sceUmdRegisterUMDCallBack(SceUID callbackId);

/**
 * Unregister a callback that is called when the UMD disc state changes.
 * 
 * @param callbackId The ID of the callback.
 * 
 * @return SCE_ERROR_OK on success, otherwise SCE_ERROR_ERRNO_INVALID_ARGUMENT (when the passed ID does
 *         not match the ID of the previously registered callback).
 */
s32 sceUmdUnRegisterUMDCallBack(SceUID callbackId);

/**
 * Verify the existence of an UMD medium.
 * 
 * @return 0, if no disc is present, a value != 0 indicates disc is present.
 */
s32 sceUmdCheckMedium(void);

/**
 * Wait for a UMD drive status event.
 * 
 * @param umdState The state to wait for until it occurs. One of ::SceUmdDiscStates.
 * 
 * @return SCE_ERROR_OK, otherwise SCE_ERROR_ERRNO_INVALID_ARGUMENT if the given UMD state 
 *         isn't one of the following: SCE_UMD_MEDIA_OUT, SCE_UMD_MEDIA_IN, SCE_UMD_NOT_READY, 
 *         SCE_UMD_READY, SCE_UMD_READABLE
 *         Other errors indicate a thread synchronization error.
 */
s32 sceUmdWaitDriveStat(s32 umdState);

/**
 * Wait for a UMD drive status event.
 * 
 * @param umdState The state to wait for until it occurs. One of ::SceUmdDiscStates.
 * @param timeout Timeout value in microseconds for the wait.
 * 
 * @return SCE_ERROR_OK, otherwise SCE_ERROR_ERRNO_INVALID_ARGUMENT if the given UMD state 
 *         isn't one of the following: SCE_UMD_MEDIA_OUT, SCE_UMD_MEDIA_IN, SCE_UMD_NOT_READY, 
 *         SCE_UMD_READY, SCE_UMD_READABLE. 
 *         Other errors indicate a thread synchronization error.
 */
s32 sceUmdWaitDriveStatWithTimer(u32 umdState, u32 timeout);

/**
 * Wait for a UMD drive status event with callback.
 * 
 * @param umdState The state to wait for until it occurs. One of ::SceUmdDiscStates.
 * @param timeout Timeout value in microseconds for the wait.
 * 
 * @return SCE_ERROR_OK, otherwise SCE_ERROR_ERRNO_INVALID_ARGUMENT if the given UMD state 
 *         isn't one of the following: SCE_UMD_MEDIA_OUT, SCE_UMD_MEDIA_IN, SCE_UMD_NOT_READY, 
 *         SCE_UMD_READY, SCE_UMD_READABLE
 *         Other errors indicate a thread synchronization error.
 */
s32 sceUmdWaitDriveStatCB(u32 umdState, u32 timeout);

/**
 * Cancel a wait for a UMD drive status event.
 * 
 * @return SCE_ERROR_OK on success, otherwise less than 0.
 */
s32 sceUmdCancelWaitDriveStat(void);

/**
 * Prohibit UMD disc being replaced.
 * 
 * @return SCE_ERROR_OK on success, otherwise less than 0.
 */
s32 sceUmdReplaceProhibit(void);

/**
 * Permit UMD disc being replaced.
 * 
 * @return SCE_ERROR_OK on success, otherwise less than 0.
 */
s32 sceUmdReplacePermit(void);

/**
 * Unknown.
 * 
 * @return SCE_ERROR_OK on success, otherwise less than 0.
 */
s32 sceUmdUseUMDInMsUsbWlan(void);

/**
 * Unknown.
 * 
 * @return SCE_ERROR_OK on success, otherwise less than 0.
 */
s32 sceUmdUnuseUMDInMsUsbWlan(void);

#ifdef	__cplusplus
}
#endif

#endif	/* MEDIAMAN_KERNEL_H */

/** @} */

