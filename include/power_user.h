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
	SCE_POWER_CALLBACKARG_BATTERY_CAP				= 0x0000007F,
	/** Indicates a battery has been equipped.  */
	SCE_POWER_CALLBACKARG_BATTERYEXIST				= 0x00000080,
	/** Indicates the battery is in a low battery state.  */
	SCE_POWER_CALLBACKARG_LOWBATTERY				= 0x00000100,
	/** Indicates power is being supplied from an external power source (AC adapter only). */
	SCE_POWER_CALLBACKARG_POWERONLINE				= 0x00001000,
	/**
	 * Indicates the PSP's suspend process has begun. This happens for example when
	 *	- the user quickly presses the HOLD switch
	 *	- automatic sleep happens to save power
	 *	- forced suspension starts due to battery running out
	 */
	 SCE_POWER_CALLBACKARG_SUSPENDING				= 0x00010000,
	 /** Indicates the PSP's resume process has started. */
	 SCE_POWER_CALLBACKARG_RESUMING					= 0x00020000,
	 /** Indicates the PSP's resume process has completed. */
	 SCE_POWER_CALLBACKARG_RESUME_COMP				= 0x00040000,
	 /**
	  * Indicates that standby operation has been started by pressing
	  * and holding the POWER switch.
	  */
	  SCE_POWER_CALLBACKARG_STANDINGBY				= 0x00080000,
} ScePowerCallbackArg;

/**
 * Power service callback function prototype.
 *
 * @param count Number of times the callback has been called.
 * @param arg The callback argument. Can contain multiple members of ::ScePowerCallbackArg.
 * @param common Custom callback value. Specified by the same parameter in ::sceKernelCreateCallback().
 *
 * @return Always SCE_ERROR_OK.
 * 
 * @see ::scePowerRegisterCallback()
 */
typedef void (*ScePowerCallback)(s32 count, s32 arg, void *common);

/**
 * @brief Registers a power service notification callback.
 * 
 * This function registers a callback that is reported from the power service when a change in the 
 * power state or battery status occurs.
 *
 * @param slot The callback registration slot to use. Specify either a value between 0 - 15
 * or ::SCE_POWER_CALLBACKSLOT_AUTO.
 * @param cbid The callback UID obtained by ::sceKernelCreateCallback().
 *
 * @return SCE_ERROR_OK when @slot was assigned a specific slot number (0 - 15) and registration succeeded.
 * @return 0 - 15 when @slot was set to ::SCE_POWER_CALLBACKSLOT_AUTO and an available slot
 * was found (slot number returned).
 * @return A value < 0 on failure.
 * 
 * @see ::ScePowerCallback()
 * @see scePowerUnregisterCallback()
 */
s32 scePowerRegisterCallback(s32 slot, SceUID cbid);

/**
 * Cancels a registered power service notification callback.
 *
 * @param slot The slot number of the callback to unregister. Specify a value between 0 - 15.
 *
 * @return SCE_ERROR_OK on success, otherwise < 0.
 * 
 * @see ::scePowerRegisterCallback().
 */
s32 scePowerUnregisterCallback(s32 slot);

/**
 * Specifies that a power callback cannot report back to the power service that the system is
 * busy creating the callback notification and calling the callback function.
 * As a consequence, if this callback mode is set, the power service will not wait for callback notification
 * generation being completed by the system when a power state switch occurs (such as a suspend or standby
 * operation). As such, subscribers to power callbacks might not receive a [suspending/standing by]
 * power callback in case the system is delayed from calling the callback function.
 */
#define SCE_POWER_CALLBACK_MODE_CANNOT_REPORT_BUSY_BEING_PROCESSED_STATUS		0

/**
 * Sets the mode for a registered power callback.
 *
 * @param slot The slot number specifying the registered power callback for which a new mode should be set.
 * Specify a value between 0 - 15.
 *
 * @param mode The callback mode. Specify either ::SCE_POWER_CALLBACK_MODE_CANNOT_REPORT_BUSY_BEING_PROCESSED_STATUS
 * or any other valid integer value. If a value other than
 * ::SCE_POWER_CALLBACK_MODE_CANNOT_REPORT_BUSY_BEING_PROCESSED_STATUS is specified, the power service will wait
 * with proceeding a power state switch operation (like suspend/standby) until the system has finished processing
 * the generated callback notification and called the callback function.
 *
 * @return SCE_ERROR_OK on success, otherwise < 0.
 *
 * @remark The default callback mode is ::SCE_POWER_CALLBACK_MODE_CANNOT_REPORT_BUSY_BEING_PROCESSED_STATUS.
 *
 * @see ::SCE_POWER_CALLBACK_MODE_CANNOT_REPORT_BUSY_BEING_PROCESSED_STATUS
 */
s32 scePowerSetCallbackMode(s32 slot, s32 mode);

/**
 * Gets the mode for a registered power callback.
 *
 * @param slot The slot number specifying the registered power callback for which the mode should be obtained.
 * Specify a value between 0 - 15.
 *
 * @param pMode A pointer to a s32 value which is to receive the callback mode.
 *
 * @return SCE_ERROR_OK on success, otherwise < 0.
 *
 * @see ::SCE_POWER_CALLBACK_MODE_CANNOT_REPORT_BUSY_BEING_PROCESSED_STATUS
 * @see ::scePowerSetCallbackMode()
 */
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
 * Sets the PLL clock frequency, the CPU clock frequency and the bus clock frequency.
 * The frequencies are specified in MHz.
 *
 * @remark This is an alias for ::scePowerClockFrequency() on firmwares earlier than 2.80.
 *
 * @see ::scePowerClockFrequency() for more details.
 */
s32 scePowerSetClockFrequencyBefore280(s32 pllFrequency, s32 cpuFrequency, s32 busFrequency);

/**
 * Sets the PLL clock frequency, the CPU clock frequency and the bus clock frequency.
 * The frequencies are specified in MHz.
 *
 * @remark This is an alias for ::scePowerClockFrequency() on firmware 2.80.
 *
 * @see ::scePowerClockFrequency() for more details.
 */
s32 scePowerSetClockFrequency280(s32 pllFrequency, s32 cpuFrequency, s32 busFrequency);

/**
 * Sets the PLL clock frequency, the CPU clock frequency and the bus clock frequency.
 * The frequencies are specified in MHz.
 *
 * @remark This is an alias for ::scePowerClockFrequency() on firmware 3.00.
 *
 * @see ::scePowerClockFrequency() for more details.
 */
s32 scePowerSetClockFrequency300(s32 pllFrequency, s32 cpuFrequency, s32 busFrequency);

/**
 * Sets the PLL clock frequency, the CPU clock frequency and the bus clock frequency.
 * The frequencies are specified in MHz.
 *
 * @remark This is an alias for ::scePowerClockFrequency() on firmware 3.50.
 *
 * @see ::scePowerClockFrequency() for more details.
 */
s32 scePowerSetClockFrequency350(s32 pllFrequency, s32 cpuFrequency, s32 busFrequency);

/**
 * Sets the PLL clock frequency, the CPU clock frequency and the bus clock frequency.
 * The frequencies are specified in MHz.
 *
 * @param pllFrequency The new PLL clock frequency. Specify either 190, 222, 266 or 333.
 * @param cpuFrequency The new CPU clock frequency. Specify a value between 1 - 333. Valid values have
 * to be less than or equal to the value specified for @p pllFrequency.
 * @param busFrequency The new bus clock frequency. Must be exactly 1/2 of @p pllFrequency. In case of
 * 333MHZ specified for @p pllFrequency, specify 166MHz.
 *
 * @return SCE_ERROR_OK on success, otherwise < 0.
 */
s32 scePowerSetClockFrequency(s32 pllFrequency, s32 cpuFrequency, s32 busFrequency);

/* Power state switch request functions */

/** Defines power switch modes. */
typedef enum {
	/**
	 * This constant defines that the PSP system's power state cannot be changed by operating
	 * the POWER switch. The power state can still be changed by calling APIs 
	 * like ::scePowerRequestStandby().
	 */
	SCE_POWER_SWITCH_MODE_HARDWARE_POWER_SWITCH_REQUESTS_NOT_SUPPORTED = 0,
	/**
	 * This constant defines that power state change requests generated by operating the PSP system's
	 * POWER switch will create callbacks applications can receive. The generated callbacks can be
	 * obtained by using the ::scePowerRegisterCallback() API and then check for either the
	 * ::SCE_POWER_CALLBACKARG_POWERSW_STANDBY_REQUESTED bit or the
	 * ::SCE_POWER_CALLBACKARG_POWERSW_SUSPEND_REQUESTED bit being set.
	 */
	SCE_POWER_SWITCH_MODE_HARDWARE_POWER_SWITCH_REQUESTS_CALLBACK_SUPPORTED,
	/**
	 * This constant defines that the PSP system's POWER switch can be used to change the PSP's
	 * power state (to either [suspend] or [standby]).
	 */
	 SCE_POWER_SWITCH_MODE_HARDWARE_POWER_SWITCH_REQUESTS_SUPPORTED
} ScePowerSwitchMode;

/**
 * Sets the power switch mode of the power service.
 *
 * @param mode The new power switch mode. One or more of ::ScePowerSwitchMode.
 *
 * @return Always SCE_ERROR_OK.
 *
 * @see ::scePowerGetPowerSwMode()
 */
s32 scePowerSetPowerSwMode(u32 mode);

/**
 * Gets the power switch mode of the power service.
 *
 * @return The current power switch mode. One or more of ::ScePowerSwitchMode.
 *
 * @remark The default power switch mode set by the power service is ::SCE_POWER_SWITCH_MODE_HARDWARE_POWER_SWITCH_REQUESTS_SUPPORTED.
 *
 * @see ::scePowerSetPowerSwMode()
 */
s32 scePowerGetPowerSwMode(void);

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

/* This constant defines the default cold reset mode. */
#define SCE_POWER_COLD_RESET_DEFAULT	0

/**
 * @brief Requests the PSP system to do a cold reset.
 * 
 * @param mode Only specify ::SCE_POWER_COLD_RESET_DEFAULT.
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

/**
 * @brief Waits until a power switch request registered in the system has been completed.
 *
 * This functions only waits for the completion of a programmatically generated power switch request.
 * Hardware generated power switch requests (user holding the POWER switch) cannot be waited on for
 * completion. If no programmatically generated request has been registered in the system at the time
 * this function is called, it returns immediately.
 *
 * @return SCE_ERROR_OK on success, otherwise < 0.
 */
s32 scePowerWaitRequestCompletion(void);

/* Power state switch misc */

/**
 * Gets the number of times resume processing was performed for the PSP system (between two cold boots).
 *
 * @return The number of times the PSP system was resumed.
 *
 * @remark Power state changes caused by calling ::scePowerRequestSuspendTouchAndGo() are counted as well.
 */
s32 scePowerGetResumeCount(void);

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

/* Volatile memory lock / unlock  */

/**
 * @brief Obtains exclusive access to the PSP's RAM area reserved for volatile memory.
 *
 * This function enables the application to obtain exclusive access to the memory that is normally
 * used for saving the contents of the eDRAM on the PSP system chip during suspend/resume processing.
 * It is a blocking call, meaning that if at the time of the call exclusive access cannot be acquired,
 * the calling thread is waiting until it can acquire exclusive control.
 *
 * @param mode Currently only specify ::SCE_KERNEL_VOLATILE_MEM_DEFAULT.
 * @param ppAddr Pointer to a void* type variable receiving the starting address of the memory area
 * reserved for the volatile memory.
 * @param pSize Pointer to a SceSize variable receiving the size of the memory area reserved for the
 * volatile memory.
 *
 * @return SCE_ERROR_OK if exclusive access was obtained, < 0 on error.
 *
 * @see ::scePowerVolatileMemTryLock()
 * @see ::scePowerVolatileMemUnlock()
 */
s32 scePowerVolatileMemLock(s32 mode, void** ppAddr, SceSize* pSize);

/**
 * @brief Obtains exclusive access to the PSP's RAM area reserved for volatile memory.
 *
 * This function enables the application to to obtain exclusive access to the memory that is normally
 * used for saving the contents of the eDRAM on the PSP system chip during suspend/resume processing.
 * It is not a blocking call, meaning if exclusive access to the memory area cannot be obtained at the
 * time of the call, this function immediately returns.
 *
 * @param mode Currently only specify ::SCE_KERNEL_VOLATILE_MEM_DEFAULT.
 * @param ppAddr Pointer to a void* type variable receiving the starting address of the memory area
 * reserved for the volatile memory.
 * @param pSize Pointer to a SceSize variable receiving the size of the memory area reserved for the
 * volatile memory.
 *
 * @return SCE_ERROR_OK if exclusive access was obtained, < 0 on error.
 *
 * @see ::scePowerVolatileMemLock()
 * @see ::scePowerVolatileMemUnlock()
 */
s32 scePowerVolatileMemTryLock(s32 mode, void** ppAddr, SceSize* pSize);

/**
 * @brief Relinquishes exclusive access to the PSP's RAM area reserved for volatile memory.
 *
 * This function relinquishes exlusive access to the memory reserved for saving volatile memory,
 * which had been previously obtained by using the ::sceKernelVolatileMemLock() or
 * ::sceKernelVolatileMemTryLock() APIs back to the kernel.
 *
 * @param mode Currently only specify ::SCE_KERNEL_VOLATILE_MEM_DEFAULT.
 *
 * @return SCE_ERROR_OK on success, otherwise < 0.
 *
 * @see scePowerVolatileMemLock()
 * @see scePowerVolatileMemTryLock()
 */
s32 scePowerVolatileMemUnlock(s32 mode);

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

/** Defines constants specifying whether WLAN is currently in use. */
typedef enum {
	/** This constant specifies that WLAN is currently not in use. */
	SCE_POWER_WLAN_ACTIVITY_OFF = 0,
	/** This constant specifies that WLAN is currently in use. */
	SCE_POWER_WLAN_ACTIVITY_ON
} ScePowerWlanActivity;

/**
 * Gets the current WLAN activity status.
 *
 * @return One of ::ScePowerWlanActivity.
 */
u8 scePowerGetWlanActivity(void);

/* Battery state */

/** Defines constants indicating the battery charging status returned by ::scePowerGetBatteryChargingStatus(). */
typedef enum {
	/* The battery is not charging. */
	SCE_POWER_BATTERY_CHARGING_STATUS_NOT_CHARGING = 0,
	/* The battery is charging. */
	SCE_POWER_BATTERY_CHARGING_STATUS_CHARGING,
	/* The battery is not charging as battery charging is currently forbidden. */
	SCE_POWER_BATTERY_CHARGING_STATUS_CHARGING_FORBIDDEN,
	/* The battery is not charging. Concrete meaning unknown. */
	SCE_POWER_BATTERY_CHARGING_STATUS_NOT_CHARGING_UNKNOWN_STATUS_3
} ScePowerBatteryChargingStatus;

/**
 * @brief Gets the battery charging status.
 *
 * This function gets the current battery charging status. The correct value may not be returned after a
 * battery is installed until the power service polls and recognizes that the battery has been installed.
 * Battery charging may be suppressed (the battery is not charging) while WLAN is in use.
 *
 * @return The current battery charging status on success. One of ::ScePowerBatteryChargingStatus.
 * @return SCE_POWER_ERROR_NO_BATTERY No battery equipped.
 * @return SCE_POWER_ERROR_DETECTING The power service is busy detecting the new battery status.
 *
 * @remark To find out whether battery charging has been completed (= battery is fully charged), you can
 * use this API together with ::sceIsPowerOnline().
 * @par Example:
 * @code
 * if (sceIsPowerOnline()
 *	&& scePowerGetBatteryChargingStatus() == SCE_POWER_BATTERY_CHARGING_STATUS_NOT_CHARGING)
 * {
 *		// battery is fully charged
 * }
 * @endcode
 *
 * @see ::scePowerIsBatteryCharging()
 */
s32 scePowerGetBatteryChargingStatus(void);

s32 scePowerIsSuspendRequired(void);

s32 scePowerGetBatteryRemainCapacity(void);

s32 scePowerGetBatteryLifeTime(void);

s32 scePowerGetBatteryTemp(void);

s32 scePowerGetBatteryElec(u32 *pBatteryElec);

s32 scePowerGetBatteryChargeCycle(void);

s32 scePowerBatteryUpdateInfo(void);

s32 scePowerGetForceSuspendCapacity(void);

s32 scePowerGetLowBatteryCapacity(void);

/**
 * @brief Gets the external power supply connection status.
 *
 * This function checks whether or not power is supplied from an external power source (AC adapter only).
 * Powering the system over USB (USB charging) is not recognized by this API.
 *
 * @return SCE_TRUE if the PSP system is connected to an external power source via an AC adapter, SCE_FALSE
 * otherwise.
 *
 * @remark This status can also be obtained using a power callback. See ::scePowerRegisterCallback() for more
 * details.
 */
s32 scePowerIsPowerOnline(void);

/**
 * @brief Gets the battery equipped status.
 *
 * This function gets whether or not a battery is equipped. A battery is considered to be equipped if the
 * PSP system can correctly communicate with the battery.
 *
 * @return SCE_TRUE If a battery is equipped, otherwise SCE_FALSE.
 *
 * @remark The correct value may not be returned after a battery is installed until the power service
 * polls and recognizes that the battery has been equipped.
 * 
 * @remark This status can also be obtained using a power callback. See ::scePowerRegisterCallback() for more
 * details.
 */
s32 scePowerIsBatteryExist(void);

/**
 * @Brief Gets the battery charging status.
 *
 * This function indicates whether or not the battery is currently charging. If detailed battery charging
 * status information is required, consider using ::scePowerGetBatteryChargingStatus() instead.
 *
 * @return SCE_TRUE if the battery is charging, otherwise SCE_FALSE.
 * @return SCE_POWER_ERROR_NO_BATTERY No battery equipped.
 * @return SCE_POWER_ERROR_DETECTING The power service is busy detecting the new battery status.
 *
 * @remark The correct value may not be returned after a battery is equipped until the power service polls
 * and recognizes that the battery has been equipped. Battery charging may be suppressed (the battery is not
 * charging) while WLAN is in use.
 *
 * @remark To find out whether battery charging has been completed (= battery is fully charged), you can
 * use this API together with ::sceIsPowerOnline().
 * @par Example:
 * @code
 * if (sceIsPowerOnline() && !scePowerIsBatteryCharging())
 * {
 *		// battery is fully charged
 * }
 * @endcode
 *
 * @see ::scePowerGetBatteryChargingStatus()
 */
s32 scePowerIsBatteryCharging(void);

/**
 * @brief Gets the low battery status.
 *
 * This function indicates whether or not the battery is currently in the low battery status. A low battery
 * status means that the remaining battery life is short. When the battery is low, the POWER LED on the PSP
 * system will blink.
 *
 * @return SCE_TRUE if the battery is low, otherwise SCE_FALSE.
 *
 * @remark Whether the battery is currently in the low battery status can also be checked using a power callback.
 * See ::scePowerRegisterCallback() for more details.
 */
s32 scePowerIsLowBattery(void);

s32 scePowerGetBatteryFullCapacity(void);

s32 scePowerGetBatteryLifePercent(void);

s32 scePowerGetBatteryVolt(void);

s32 scePowerGetInnerTemp(void);

/* Misc */

/**
 * Specifies the maximum PSP display backlight level available when the PSP device is not connected to an
 * external power source.
 */
#define SCE_POWER_BACKLIGHT_LEVEL_MAXIMUM_POWER_OFFLINE		3
/**
 * Specifies the maximum PSP display backlight level available when the PSP device is connected to an external
 * power source.
 */
#define SCE_POWER_BACKLIGHT_LEVEL_MAXIMUM_POWER_ONLINE		4

/**
 * @brief Gets the maximum (= brightest) PSP display backlight level currently available.
 *
 * The maximum backlight level is dependent on factors like whether the PSP device is currently
 * connected to an external power source or whether WLAN is currently active. If WLAN is active, the
 * brightest available PSP display backlight level might be less than without WLAN in use.
 *
 * @return If the PSP device is connected to an external power source at the time of this API call, at most
 * ::SCE_POWER_BACKLIGHT_MAXIMUM_POWER_ONLINE is returned.
 * @return If the PSP device is not connected to an external power source at the time of this API call, at most
 * ::SCE_POWER_BACKLIGHT_MAXIMUM_POWER_OFFLINE is returned.
 */
s32 scePowerGetBacklightMaximum(void);


 /** @} */

#endif	/* POWER_USER_H */
