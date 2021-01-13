/* Copyright (C) The uOFW team
   See the file COPYING for copying permission.
*/

#ifndef POWER_USER_H
#define	POWER_USER_H

#include "common_header.h"
#include "power_error.h"

/** @defgroup User User
 *  @ingroup PowerLibrary
 *
 *  The Power service User API.
 * @{
 */


 /* Callbacks */

/**
 * This constant specifies that a search for a slot number from among the empty slots is to be automatically performed
 * when a callback is registered with the ::scePowerRegisterCallback() function.
 */
#define SCE_POWER_CALLBACKSLOT_AUTO					(-1)

 /**
  * Defines constants passed to the ::ScePowerCallback function specifying the reason for the callback.
  */
typedef enum {
	/** These bits represent the remaining battery capacity in [%].*/
	SCE_POWER_CALLBACKARG_BATTERY_CAP = 0x0000007F,
	/** Indicates a battery has been equipped.  */
	SCE_POWER_CALLBACKARG_BATTERYEXIST = 0x00000080,
	/** Indicates the battery is in a low battery state.  */
	SCE_POWER_CALLBACKARG_LOW_BATTERY = 0x00000100,
	/** Indicates power is being supplied from an external power source (AC adapter). */
	SCE_POWER_CALLBACKARG_POWER_ONLINE = 0x00001000,
	/**
	 * Indicates the PSP's suspend process has begun. This happens for example when
	 *	- the user quickly presses the HOLD switch
	 *	- automatic sleep happens to save power
	 *	- forced suspension starts due to battery running out
	 */
	 SCE_POWER_CALLBACKARG_SUSPENDING = 0x00010000,
	 /** Indicates the PSP's resume process has started. */
	 SCE_POWER_CALLBACKARG_RESUMING = 0x00020000,
	 /** Indicates the PSP's resume process has completed. */
	 SCE_POWER_CALLBACKARG_RESUME_COMP = 0x00040000,
	 /**
	  * Indicates that standby operation has been started by pressing
	  * and holding the POWER switch.
	  */
	  SCE_POWER_CALLBACKARG_STANDINGBY = 0x00080000,
} ScePowerCallbackArg;

/**
 * Power service callback function prototype
 *
 * @param count Number of times the callback has been called.
 * @param arg The callback argument. Can contain multiple members of ::ScePowerCallbackArg.
 * @param common Custom callback value. Specified by the same parameter in ::sceKernelCreateCallback().
 *
 * @return Always SCE_ERROR_OK.
 */
typedef void (*ScePowerCallback)(s32 count, s32 arg, void *common);

/**
 * Registers a power service notification callback.
 *
 * @param slot The callback registration slot to use. Specify either a value between 0 - 15 or ::SCE_POWER_CALLBACKSLOT_AUTO.
 * @param cbid The callback UID obtained by ::sceKernelCreateCallback().
 *
 * @return SCE_ERROR_OK when @slot was set to ::SCE_POWER_CALLBACKSLOT_AUTO and registration succeeded.
 * @return 0 - 15 on success when specific slot was used.
 * @return A value < 0 on failure.
 */
s32 scePowerRegisterCallback(s32 slot, SceUID cbid);

/**
 * Cancels a registered power service notification callback.
 *
 * @param slot The slot of the callback to unregister. Specify a value between 0 - 15.
 *
 * @return SCE_ERROR_OK on success, otherwise < 0.
 */
s32 scePowerUnregisterCallback(s32 slot);

s32 scePowerSetCallbackMode(s32 slot, s32 mode);

s32 scePowerGetCallbackMode(s32 slot, s32 *pMode);

/* Clock frequency functions */

/**
 * Gets the CPU clock frequency.
 *
 * @return The CPU clock frequency in MHz.
 */
s32 scePowerGetCpuClockFrequencyInt(void);

/**
 * Gets the CPU clock frequency.
 *
 * @remark The accuracy is identical to ::scePowerGetCpuClockFrequencyInt().
 *
 * @return The CPU clock frequency in MHz.
 */
float scePowerGetCpuClockFrequencyFloat(void);

/**
 * Gets the bus clock frequency.
 *
 * @remark The bus clock frequency always operates at 1/2 the PLL clock frequency.
 *
 * @return The current bus clock frequency in MHz.
 */
s32 scePowerGetBusClockFrequencyInt(void);

/**
 * Gets the bus clock frequency.
 *
 * @remark The bus clock frequency always operates at 1/2 the PLL clock frequency.
 * @remark The accuracy is identical to ::scePowerGetBusClockFrequencyInt().
 *
 * @return The current bus clock frequency in MHz.
 */
float scePowerGetBusClockFrequencyFloat(void);

/**
 * Gets the PLL output clock frequency.
 *
 * @return The current PLL output clock frequency in MHz.
 */
s32 scePowerGetPllClockFrequencyInt(void);

/**
 * Gets the PLL output clock frequency.
 *
 * @return The current PLL output clock frequency in MHz.
 */
float scePowerGetPllClockFrequencyFloat(void);

/**
 * Sets the CPU clock frequency.
 * 
 * @param cpuFrequency The CPU clock frequency in MHz. Can bet to a value in range 1 to 333.
 * Note that it cannot be set to a value higher than the current PLL clock frequency.
 * 
 * @return SCE_ERROR_OK on success, otherwise < 0.
 * 
 * @remark Even in success case, the CPU might not actually operate the specified clock frequency.
 * To obtain the actual frequency in use, call either ::scePowerGetBusClockFrequencyInt() or 
 * ::scePowerGetBusClockFrequencyFloat().
 */
s32 scePowerSetCpuClockFrequency(s32 cpuFrequency);

/**
 * Sets the bus clock frequency.
 *
 * @param busFrequency The bus clock frequency in MHz.
 *
 * @return SCE_ERROR_OK on success, otherwise < 0.
 *
 * @remark This function has been removed from SDK 3.70 and later. If still called,
 * it will now always try to set the bus clock frequency to 1/2 PLL clock frequency.
 */
s32 scePowerSetBusClockFrequency(s32 busFrequency);

/**
 * Sets clock frequencies.
 *
 * @remark This is an alias for ::scePowerClockFrequency() on firmwares earlier than 2.80.
 *
 * @see ::scePowerClockFrequency() for more details.
 *
 */
s32 scePowerSetClockFrequencyBefore280(s32 pllFrequency, s32 cpuFrequency, s32 busFrequency);

/**
 * Sets clock frequencies.
 *
 * @remark This is an alias for ::scePowerClockFrequency() on firmware 2.80.
 *
 * @see ::scePowerClockFrequency() for more details.
 *
 */
s32 scePowerSetClockFrequency280(s32 pllFrequency, s32 cpuFrequency, s32 busFrequency);

/**
 * Sets clock frequencies.
 *
 * @remark This is an alias for ::scePowerClockFrequency() on firmware 3.00.
 *
 * @see ::scePowerClockFrequency() for more details.
 *
 */
s32 scePowerSetClockFrequency300(s32 pllFrequency, s32 cpuFrequency, s32 busFrequency);

/**
 * Sets clock frequencies.
 *
 * @remark This is an alias for ::scePowerClockFrequency() on firmware 3.50.
 *
 * @see ::scePowerClockFrequency() for more details.
 *
 */
s32 scePowerSetClockFrequency350(s32 pllFrequency, s32 cpuFrequency, s32 busFrequency);

/**
 * Sets clock frequencies.
 *
 * @param pllFrequency The PLL clock frequency in MHz. Specify either 190, 222, 266 or 333.
 * @param cpuFrequency The CPU clock frequency in MHz. Specify a value between 1 - 333. Valid values have
 * to be less than or equal to the value specified for @p pllFrequency.
 * @param busFrequency The bus clock frequency in MHz. Must be exactly 1/2 of @p pllFrequency. In case of
 * 333 MHZ specified for @p pllFrequency, specify 166.
 *
 * @return SCE_ERROR_OK on success, otherwise < 0.
 */
s32 scePowerSetClockFrequency(s32 pllFrequency, s32 cpuFrequency, s32 busFrequency);

/* Power switch request functions */

/**
 * @brief Requests the PSP system to go into standby.
 *
 * @return Always SCE_ERROR_OK.
 *
 * @remark This function only generates a request. The actual standby operation might be delayed.
 * For example, if power switch locks have been put in place (for example by calling ::scePowerLockForUser())
 * then the standby operation will be delayed until all power switch locks have been removed.
 */
s32 scePowerRequestStandby(void);

/**
 * @brief Requests the PSP system to suspend.
 *
 * @return Always SCE_ERROR_OK.
 *
 * @remark This function only generates a request. The actual suspend operation might be delayed.
 * For example, if power switch locks have been put in place (for example by calling ::scePowerLockForUser())
 * then the suspend operation will be delayed until all power switch locks have been removed.
 */
s32 scePowerRequestSuspend(void);

s32 scePowerRequestSuspendTouchAndGo(void);

/**
 * @brief Requests the PSP system to do a cold reset.
 * 
 * @param mode Unknown. Only specify 0.
 *
 * @return SCE_ERROR_OK on successful request generation, otherwise < 0.
 *
 * @remark This function only generates a request. The actual cold-reset operation might be delayed.
 * For example, if power switch locks have been put in place (for example by calling ::scePowerLockForUser())
 * then the cold-reset operation will be delayed until all power switch locks have been removed.
 */
s32 scePowerRequestColdReset(s32 mode);

/**
 * Gets whether a PSP power switch request (standby/resume/...) is currently registered in the system.
 *
 * @return SCE_TRUE if a power switch request exists, otherwise SCE_FALSE.
 */
s32 scePowerIsRequest(void);

/**
 * Defines constants representing the power switch requests which can be generated by the user via
 * the PSP hardware (POWER switch).
 */
typedef enum {
	/**
	 * Defines that no power switch generated by the user holding the POWER switch is currently
	 * registered in the system.
	 */
	HARDWARE_POWER_SWITCH_REQUEST_NONE = 0,
	/**
	 * Defines a standby power switch request generated by the user holding the POWER switch
	 * for less than two seconds.
	 */
	 HARDWARE_POWER_SWITCH_REQUEST_STANDBY,
	 /**
	  * Defines a standby power switch request generated by the user holding the POWER switch
	  * for less than two seconds.
	  */
	  HARDWARE_POWER_SWITCH_REQUEST_SUSPEND,
} ScePowerHardwarePowerSwitchRequest;

/**
 * @brief Cancels an existing power switch request.
 *
 * This function only cancels power switch requests which were generated by the user
 * holding the POWER switch (thus generating either a suspend or standby request). Programmatically
 * generated power switch requests cannot be canceled.
 *
 * @return The canceled power switch request. A value of ::ScePowerHardwarePowerSwitchRequest.
 */
s32 scePowerCancelRequest(void);

/* Power switch manipulation lock / unlock */

/**
 * @brief Locks Power switch manipulation (sets a power off lock).
 *
 * This function delays a power off request until the lock is canceled. It is used in a critical section
 * to protect timing when a power interruption would cause a problem. For example, it might be used to prevent
 * a power interruption while data is being written to the Memory Stick.
 *
 * @param lockType The power processing lock type. Specify ::SCE_KERNEL_POWER_LOCK_DEFAULT.
 *
 * @return SCE_ERROR_OK on success, otherwise < 0.
 *
 * @remark Since the user will not be able to turn the power off by operating the POWER/HOLD switch
 * during a power locked state, if the ::scePowerLockForUser() function is used carelessly, it can
 * significantly decrease the usability of the PSP system.
 * 
 * @remark Normally, ::scePowerLockForUser() returns immediately. However, if a suspend/standby/reboot operation
 * is already running at the time the lock is being set, the calling thread may be blocked.
 */
s32 scePowerLockForUser(s32 lockType);

/**
 * @brief Cancels a power off lock.
 *
 * This function cancels a power lock that was previously set with the ::scePowerLockForUser() API.
 *
 * @param lockType Power processing lock type. Specify ::SCE_KERNEL_POWER_LOCK_DEFAULT.
 *
 * @return The remaining existing power locks (>= 0).
 * 
 * @remark Normally, ::scePowerUnlockForUser() returns immediately. However, if a suspend/standby/reboot operation
 * is already running at the time the lock is being set, the calling thread may be blocked.
 */
s32 scePowerUnlockForUser(s32 lockType);

/* WLAN functions */

/** Specifies a device type which permits a maximum PLL clock frequency of 222 MHz when WLAN is active. */
#define SCE_POWER_WLAN_COEXISTENCE_CLOCK_222MHz		0
/** Specifies a device type which supports PLL clock frequencies of 266 MHz and 333 MHz when WLAN is active. */
#define SCE_POWER_WLAN_COEXISTENCE_CLOCK_333MHz		1

/**
 * Gets the maximum clock frequency allowed when WLAN is active.
 *
 * @remark For the PSP-100X, the PLL clock frequency cannot be set to 266 MHz or 333 MHz while WLAN is actice.
 * However, for the PSP-2000 and later devices, the PLL clock frequency can be set to 266 MHz or 333 MHz while
 * WLAN is active.
 *
 * @return ::SCE_POWER_WLAN_COEXISTENCE_CLOCK_222MHz or ::SCE_POWER_WLAN_COEXISTENCE_CLOCK_333MHz on success,
 * otherwise < 0.
 */
s32 scePowerCheckWlanCoexistenceClock(void);

u8 scePowerGetWlanActivity(void);


 /** @} */

#endif	/* POWER_USER_H */
