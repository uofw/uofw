/* Copyright (C) 2011, 2012, 2013, 2014 The uOFW team
   See the file COPYING for copying permission.
*/

#include "common_header.h"
#include "mediaman.h"

#ifndef MEDIAMAN_USER_H
#define	MEDIAMAN_USER_H

#ifdef	__cplusplus
extern "C" {
#endif

/**
 * Activate the media mananger driver.
 * 
 * @param mode The initial UMD device power mode. One of ::SceUmdDevicePowerModes.
 * @param aliasName The alias name for the mounted filesystem device name.
 * 
 * @return SCE_ERROR_OK on success, otherwise SCE_ERROR_ERRNO_INVALID_ARGUMENT.
 */
s32 sceUmdActivate(s32 mode, const char *aliasName);

/**
 * Deactivate the media manager driver.
 * 
 * @param mode The new UMD device power mode? One of ::SceUmdDevicePowerModes
 * @param aliasName aliasName The alias name for the mounted filesystem device name.
 * 
 * @return SCE_ERROR_OK on success, otherwise SCE_ERROR_ERRNO_INVALID_ARGUMENT.
 */
s32 sceUmdDeactivate(s32 mode, const char *aliasName);

/**
 * Get the disc information.
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
 * @return SCE_ERROR_OK, otherwise SCE_ERROR_ERRNO_INVALID_ARGUMENT if the given umd state 
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
 * @return SCE_ERROR_OK, otherwise SCE_ERROR_ERRNO_INVALID_ARGUMENT if the given umd state 
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
 * @return SCE_ERROR_OK, otherwise SCE_ERROR_ERRNO_INVALID_ARGUMENT if the given umd state 
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
 * Get the current UMD drive status.
 * 
 * @return The UMD drive status. One of ::SceUmdDiscStates.
 */
s32 sceUmdGetDriveStat(void);

/**
 * Get the current UMD error status.
 * 
 * @return The UMD error status. One of the error codes defined in include/umd_error.h 
 */
s32 sceUmdGetErrorStat(void);

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

#endif	/* MEDIAMAN_USER_H */

