/* Copyright (C) The uOFW team
   See the file COPYING for copying permission.
*/

#ifndef POWER_KERNEL_H
#define	POWER_KERNEL_H

#include <lowio_sysreg.h>

#include "common_header.h"
#include "power_error.h"

/** @defgroup Kernel Kernel
 *  @ingroup PowerLibrary
 *
 *  The Power service Kernel API.
 *  @{
 */

/* Power service initialization/termination */

/**
 * Initializes the power service.
 * 
 * @return Always SCE_ERROR_OK.
 */
s32 scePowerInit(void);

/**
 * Terminates the power service.
 *
 * @return Always SCE_ERROR_OK.
 */
s32 scePowerEnd(void);

/* Power callbacks */

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
	SCE_POWER_CALLBACKARG_BATTERY_CAP						= 0x0000007F,
	/** Indicates a battery has been equipped.  */
	SCE_POWER_CALLBACKARG_BATTERYEXIST						= 0x00000080,
	/** Indicates the battery is in a low battery state.  */
	SCE_POWER_CALLBACKARG_LOWBATTERY						= 0x00000100,
	/** Indicates power is being supplied from an external power source (AC adapter only). */
	SCE_POWER_CALLBACKARG_POWERONLINE						= 0x00001000,
	/** 
	 * Indicates the PSP's suspend process has begun. This happens for example when 
	 *	- the user quickly presses the HOLD switch 
	 *	- automatic sleep happens to save power
	 *	- forced suspension starts due to battery running out
	 */
	SCE_POWER_CALLBACKARG_SUSPENDING						= 0x00010000,
	/** Indicates the PSP's resume process has started. */
	SCE_POWER_CALLBACKARG_RESUMING							= 0x00020000,
	/** Indicates the PSP's resume process has completed. */
	SCE_POWER_CALLBACKARG_RESUME_COMP						= 0x00040000,
	/** 
	 * Indicates that standby operation has been started by pressing 
	 * and holding the POWER switch.
	 */
	SCE_POWER_CALLBACKARG_STANDINGBY						= 0x00080000,
	/**
	 * Indicates that the user held the PSP's POWER switch at least two seconds
	 * to request entering the standby state.
	 */
	SCE_POWER_CALLBACKARG_POWERSW_STANDBY_REQUESTED			= 0x10000000,
	/**
	 * Indicates that the user held the PSP's POWER switch less than two seconds
	 * to request entering the suspend state.
	 */
	SCE_POWER_CALLBACKARG_POWERSW_SUSPEND_REQUESTED			= 0x20000000,
	/** Indicates HOLD switch has been locked. */
	SCE_POWER_CALLBACKARG_HOLDSW							= 0x40000000,
	/** Indicates POWER switch has been activated. */
	SCE_POWER_CALLBACKARG_POWERSW							= 0x80000000
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
 * power state/battery status/POWER switch state or HOLD switch state occurs.
 * 
 * @param slot The callback registration slot to use. Specify a value between 0 - 31.
 * @param cbid The callback UID obtained by ::sceKernelCreateCallback().
 * 
 * @return SCE_ERROR_OK when @slot was assigned a specific slot number (0 - 31) and registration succeeded.
 * @return A value < 0 on failure.
 * 
 * @remark While user mode applications can specify ::SCE_POWER_CALLBACKSLOT_AUTO for the @p slot to have
 * the power service automatically perform a callback slot assignment, kernel mode applications cannot have 
 * such an operation performed. They will have to explicitly specify a callback slot to use. Specifying
 * ::SCE_POWER_CALLBACKSLOT_AUTO will result in an error.
 * 
 * @par Example:
 * @code
 * 
 * s32 powerCallback(int count, int arg, void *common)
 * {
 *	  if (arg & SCE_POWER_CALLBACKARG_LOWBATTERY)
 *    {
 *        // Do something when the battery is in low battery state.
 *    }
 * }
 *
 * s32 registerPowerCallback()
 * {
 *    SceUID uidCb;
 *    s32 status;
 *
 *    // Create the callback.
 *	  uidCb = sceKernelCreateCallback("powerCb", powerCallback, NULL);
 *
 *    // Register the callback with the power service.
 *	  status = scePowerRegisterCallback(0, uidCb);
 *	  if (status < SCE_ERROR_OK)
 *    {
 *		  // Power callback registration failed.
 *	  }
 * }
 * @endcode
 * 
 * @see ::ScePowerCallback()
 * @see scePowerUnregisterCallback()
 */
s32 scePowerRegisterCallback(s32 slot, SceUID cbid);

/**
 * Cancels a registered power service notification callback.
 * 
 * @param slot The slot number of the callback to unregister. Specify a value between 0 - 31.
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
 * Specify a value between 0 - 31.
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
 * Specify a value between 0 - 31.
 * 
 * @param pMode A pointer to a s32 value which is to receive the callback mode.
 * 
 * @return SCE_ERROR_OK on success, otherwise < 0.
 * 
 * @see ::SCE_POWER_CALLBACK_MODE_CANNOT_REPORT_BUSY_BEING_PROCESSED_STATUS
 * @see ::scePowerSetCallbackMode()
 */
s32 scePowerGetCallbackMode(s32 slot, s32 *pMode);

/**
 * Blocks the ::SCE_POWER_CALLBACKARG_HOLDSW bit from being set in the ::ScePowerCallback
 * callback argument even if the HOLD switch has been locked.
 * 
 * @return Always SCE_ERROR_OK.
 */
s32 scePower_driver_23BDDD8B(void);

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
 * @remark The accuracy is identical to ::scePowerGetPllClockFrequencyInt().
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

#define SCE_POWER_PLL_USE_MASK_37MHz			(1 << SCE_SYSREG_PLL_OUT_SELECT_37MHz)
#define SCE_POWER_PLL_USE_MASK_148MHz			(1 << SCE_SYSREG_PLL_OUT_SELECT_148MHz)
#define SCE_POWER_PLL_USE_MASK_190MHz			(1 << SCE_SYSREG_PLL_OUT_SELECT_190MHz)
#define SCE_POWER_PLL_USE_MASK_222MHz			(1 << SCE_SYSREG_PLL_OUT_SELECT_222MHz)
#define SCE_POWER_PLL_USE_MASK_266MHz			(1 << SCE_SYSREG_PLL_OUT_SELECT_266MHz)
#define SCE_POWER_PLL_USE_MASK_333MHz			(1 << SCE_SYSREG_PLL_OUT_SELECT_333MHz)
#define SCE_POWER_PLL_USE_MASK_19MHz			(1 << SCE_SYSREG_PLL_OUT_SELECT_19MHz)
#define SCE_POWER_PLL_USE_MASK_74MHz			(1 << SCE_SYSREG_PLL_OUT_SELECT_74MHz)
#define SCE_POWER_PLL_USE_MASK_96MHz			(1 << SCE_SYSREG_PLL_OUT_SELECT_96MHz)
#define SCE_POWER_PLL_USE_MASK_111MHz			(1 << SCE_SYSREG_PLL_OUT_SELECT_111MHz)
#define SCE_POWER_PLL_USE_MASK_133MHz			(1 << SCE_SYSREG_PLL_OUT_SELECT_133MHz)
#define SCE_POWER_PLL_USE_MASK_166MHz			(1 << SCE_SYSREG_PLL_OUT_SELECT_166MHz)

/**
 * @brief Sets a use mask for the PLL clock frequency.
 * 
 * A use mask defines a fixed frequency at which the PLL clock can operate. By setting multiple use masks
 * 'finer' control over the actually set PLL clock frequency can be obtained. When a new PLL clock frequency
 * is specified via the ::scePowerSetClockFrequency() API, the actual frequency the PLL clock will be
 * attempted to be set to depends on the set PLL use masks. The power service will pick the smallest set use mask
 * which represents a clock frequency which is greater than or equal to the specified clock frequency.
 * 
 * @param useMask The mask containing the fixed PLL clock frequencies to consider. Multiple use masks can be
 * combined together by bitwise or'ing.
 * 
 * @par Example: Given the following PLL use mask and custom PLL frequency of 166MHz
 * @code
 * scePowerSetPllUseMask(SCE_POWER_PLL_USE_MASK_148MHz | SCE_POWER_PLL_USE_MASK_190MHz | SCE_POWER_PLL_USE_MASK_333MHz);
 * scePowerSetClockFrequency(166, 166, 83);
 * @endcode
 * the specified PLL frequency will be attempted to be set to 190MHz by the power service.
 * 
 * @return Always SCE_ERROR_OK.
 * 
 * @remark If no valid use mask is set (for example by setting @p useMask to 0) then the power service will
 * attempt to set the PLL clock frequency as high as possible (max 333MHz).
 */
s32 scePowerSetPllUseMask(s32 useMask);

/* Power service exclusive clock frequency limits */

/**
 * Sets a power service specific PLL clock frequency limit.
 * 
 * @param lowerLimit The lower PLL clock frequency limit in MHz. Default is 1MHz.
 * @param upperLimit The upper PLL clock frequency limit in MHz. Default is 333MHz.
 * 
 * @return Always SCE_ERROR_OK.
 * 
 * @remark These PLL clock frequency limits are only applied when attempting to set the
 * PLL clock frequency by calling the ::scePowerSetClockFrequency() API (and its related ones).
 * Changing the PLL clock frequency via other means will ignore these limits.
 */
s32 scePowerLimitPllClock(s16 lowerLimit, s16 upperLimit);

/**
 * Sets a power service specific CPU clock frequency limit.
 *
 * @param lowerLimit The lower CPU clock frequency limit in MHz. Default is 1MHz.
 * @param upperLimit The upper CPU clock frequency limit in MHz. Default is 333MHz.
 *
 * @return Always SCE_ERROR_OK.
 *
 * @remark These CPU clock frequency limits are only applied when attempting to set the
 * CPU clock frequency by calling either the ::scePowerSetCpuClockFrequency() API
 * or the ::scePowerSetClockFrequency() API (and its related ones). 
 * Changing the CPU clock frequency via other means will ignore these limits.
 */
s32 scePowerLimitScCpuClock(s16 lowerLimit, s16 upperLimit);

/**
 * Sets a power service specific bus clock frequency limit.
 *
 * @param lowerLimit The lower bus clock frequency limit in MHz. Default is 24MHz.
 * @param upperLimit The upper bus clock frequency limit in MHz. Default is 166MHz.
 *
 * @return Always SCE_ERROR_OK.
 *
 * @remark These bus clock frequency limits are only applied when attempting to set the
 * bus clock frequency by calling the ::scePowerSetBusClockFrequency() API or the
 * ::scePowerSetClockFrequency() API (and its related ones).
 * Changing the bus clock frequency via other means will ignore these limits.
 */
s32 scePowerLimitScBusClock(s16 lowerLimit, s16 upperLimit);

/* Hardware component power settings */

/**
 * Gets the current Tachyon voltage.
 * 
 * @return The current Tachyon voltage set by the power service.
 */
s16 scePowerGetCurrentTachyonVoltage(void);

/**
 * Gets the possible Tachyon volatge values used by the power service.
 * 
 * @param pMaxVoltage Pointer to the s16 variable which is to receive the maximum Tachyon voltage.
 * @param pDefaultVoltage Pointer to the s16 variable which is to receive the default Tachyon voltage.
 * 
 * @return Always SCE_ERROR_OK.
 */
s32 scePowerGetTachyonVoltage(s16* pMaxVoltage, s16* pDefaultVoltage);

/**
 * Sets the possible Tachyon voltages used by the power service.
 * 
 * @param maxVoltage The voltage used for the Tachyon SoC IC when the PSP's PLL operates at clock frequencies 
 * 266 Mhz or 333 MHz.
 * @param defaultVoltage The voltage used for the Tachyon SoC IC when the PSP's PLL operates at clock 
 * frequencies 222 MHz or below.
 * 
 * @return Always SCE_ERROR_OK.
 */
s32 scePowerSetTachyonVoltage(s16 maxVoltage, s16 defaultVoltage);

/**
 * Gets the current DDR memory voltage. 
 * 
 * @return The current DDR memory voltage set by the power service.
 */
s16 scePowerGetCurrentDdrVoltage(void);

/**
 * Gets the possible Tachyon voltage values used by the power service.
 * 
 * @param pMaxVoltage Pointer to the s16 variable which is to receive the maximum DDR memory voltage.
 * @param pDefaultVoltage Pointer to the s16 variable which is to receive the default DDR memory voltage.
 * 
 * @return Always SCE_ERROR_OK.
 */
s32 scePowerGetDdrVoltage(s16* pMaxVoltage, s32* pDefaultVoltage);

/**
 * Sets the possible DDR memory voltages used by the power service.
 * 
 * @param maxVoltage The voltage used for the DDR memory component when the PSP's PLL operates at clock frequency 
 * 333 MHz.
 * @param defaultVoltage The voltage used for the DDR memory component when the PSP's PLL operates at clock 
 * frequencies 266 MHz or below.
 * 
 * @return Always SCE_ERROR_OK.
 */
s32 scePowerSetDdrVoltage(s16 maxVoltage, s32 defaultVoltage);

/**
 * Gets the current DDR memory strength. 
 * 
 * @return The current DDR memory strength set by the power service.
 */
s16 scePowerGetCurrentDdrStrength(void);

/**
 * Gets the possible DDR memory strength values used by the power service.
 * 
 * @param pMaxStrength Pointer to the s16 variable which is to receive the maximum DDR memory strength value.
 * @param pDefaultStrength Pointer to the s16 variable which is to receive the default DDR memory voltage.
 * 
 * @return Always SCE_ERROR_OK.
 */
s32 scePowerGetDdrStrength(s16* pMaxStrength, s16* pDefaultStrength);

/**
 * Sets the possible DDR memory strength values used by the power service.
 * 
 * @param maxStrength The strength value used for the DDR memory component when the PSP's PLL operates at 
 * clock frequency 333 MHz.
 * @param defaultStrength The strength value used for the DDR memory component when the PSP's PLL operates at 
 * clock frequencies 266 MHz and below.
 * 
 * @return Always SCE_ERROR_OK.
 */
s32 scePowerSetDdrStrength(s16 maxStrength, s16 defaultStrength);

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
 * For example, if power switch locks have been put in place (for example by calling ::scePowerLockForKernel())
 * then the standby operation will be delayed until all power switch locks have been removed.
 */
s32 scePowerRequestStandby(void);

/**
 * @brief Requests the PSP system to suspend.
 * 
 * @return Always SCE_ERROR_OK.
 * 
 * @remark This function only generates a request. The actual suspend operation might be delayed.
 * For example, if power switch locks have been put in place (for example by calling ::scePowerLockForKernel())
 * then the suspend operation will be delayed until all power switch locks have been removed.
 */
s32 scePowerRequestSuspend(void);

/**
 * @brief Requests the PSP system to suspend and immediately resume again.
 * 
 * This function generates a request to suspend the PSP system and immediately resume it again without requiring
 * any user input (like manipulating the POWER switch).
 * 
 * @return Always SCE_ERROR_OK.
 * 
 * @remark This function only generates a request. The actual suspend-and-resume operation might be delayed.
 * For example, if power switch locks have been put in place (for example by calling ::scePowerLockForKernel())
 * then this operation will be delayed until all power switch locks have been removed.
 */
s32 scePowerRequestSuspendTouchAndGo(void);

/* This constant defines the default cold reset mode. */
#define SCE_POWER_COLD_RESET_DEFAULT	0

/**
 * @brief Requests the PSP system to do a cold reset.
 * 
 * @param mode Specify either ::SCE_POWER_COLD_RESET_DEFAULT or any other value for a second cold reset
 * execution mode.
 * 
 * @return Always SCE_ERROR_OK.
 * 
 * @remark This function only generates a request. The actual cold-reset operation might be delayed.
 * For example, if power switch locks have been put in place (for example by calling ::scePowerLockForKernel())
 * then the cold-reset operation will be delayed until all power switch locks have been removed.
 */
s32 scePowerRequestColdReset(s32 mode);

/**
 * Gets whether a PSP power switch request (standby/resume/...) is currently registered in the system.
 * 
 * @return SCE_TRUE if a power switch request exists, otherwise SCE_FALSE.
 * 
 * @remark No distinction is made between hardware (POWER switch) generated requests or programmatically 
 * generated requests.
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
 * @brief Notifies the power service that a reboot process has been started.
 *
 * This function creates a usermode power lock and prevents the system from being suspended/
 * going into standby while the reboot process is active.
 * 
 * @param arg0 Unknown. Not used.
 *
 * @return Always SCE_ERROR_OK.
 */
s32 scePowerRebootStart(s32 arg0);

/**
 * Gets the number of times resume processing was performed for the PSP system (between two cold boots).
 *
 * @return The number of times the PSP system was resumed.
 *
 * @remark Power state changes caused by calling ::scePowerRequestSuspendTouchAndGo() are counted as well.
 */
s32 scePowerGetResumeCount(void);

/**
 * Sets the wakeup condition.
 *
 * @param wakeUpCondition Unknown. Used in the suspend & resume process of the system.
 * The default set by the power service is [8].
 * 
 * @return Always SCE_ERROR_OK.
 */
s32 scePowerSetWakeupCondition(u32 wakeUpCondition);

/**
 * Gets the watchdog setting used for a power switch operation.
 * 
 * @return The current watchdog.
 */
u8 scePowerGetWatchDog(void);

/**
 * Defines constants specifying the timing for the PSP's power LED to be turned off when a power state 
 * switch operation (standby/suspend/reboot) is in process.
 */
typedef enum {
	/**
	 * This constant defines that the default off timing for the power LED during a 
	 * power state switch is picked. Currently maps to ::SCE_POWER_LED_OFF_TIMING_POWER_OFF_WITH_BUSY_INDICATOR.
	 */
	SCE_POWER_LED_OFF_TIMING_AUTO = 0,
	/** 
	 * This constant defines that the power LED is turned OFF at the beginning of a power state
	 * switch.
	 */
	SCE_POWER_LED_OFF_TIMING_POWER_STATE_SWITCH_START = 1,
	/**
	 * This constant defines that the power LED is turned OFF at the end of a power state switch
	 * (when power is off). Depending on the actual power state switch, the power LED might not be 
	 * turned off at all (like during a reboot).
	 */
	SCE_POWER_LED_OFF_TIMING_POWER_OFF = 2,
	/**
	 * This constant defines that the power LED is turned OFF at the end of a power state switch
	 * (when power is off). Depending on the actual power state switch, the power LED might not be 
	 * turned off at all (like during a reboot). In addition, the power LED flashes during the
	 * power state switch operation if the system is busy preparing for the power state switch. I.e.
	 * in the case of a standby operation, the system might have to wait for all power/volatile memory locks 
	 * to be relased. During this standby operation delay, the power LED will flash repeatedly.
	 */
	SCE_POWER_LED_OFF_TIMING_POWER_OFF_WITH_BUSY_INDICATOR = 3
} ScePowerLedOffTiming;

/**
 * Gets the off timing for the PSP's power LED for a power state switch operation 
 * (like standby/suspend/reboot).
 * 
 * @return The power LED's off timing used by the power service. One of ::ScePowerLedOffTiming.
 */
u8 scePowerGetLedOffTiming(void);

/**
 * This structure contains PSP system information at the time of resumption from the suspend state. It also
 * contains pointers to functions required to complete the resume process.
 */
typedef struct {
	/* Size of the structure. */
	u32 size; // 0
	/* The wakeup factor. */
	u32 wakeUpFactor; // 4
	/* Unknown. */
	u32 wakeUpReq; // 8
	/* Pointer to the sceSysconCmdExec() function. */
	s32 (*pSysconCmdExec)(void *, u32); // 12
	/* PSP Testing Tool dip switches. */
	u32 dipSw; // 16
	/* Pointer to a function to set the PLL clock frequency. */
	s32 (*pChangeClock)(u32); // 20
	/* The BARYON status. */
	u8 baryonStatus; // 24
	/* The BARYON clock. */
	u32 baryonClock; // 28
	/* The BARYON key states. */
	u32 baryonKey; // 32
	/* The power supply status. */
	u32 powerSupplyStatus; // 36
	/* The battery status. */
	u32 batteryStatus; // 40
	/* The remaining battery capacity in mAh. */
	u32 batteryRemainCapacity; // 44
	/* The full battery capacity in mAh. */
	s32 batteryFullCapacity; // 48
	/* Pointer to the sceSysconPowerSuspend() function. */
	s32 (*pSysconPowerSuspend)(u32, u32); // 52
	/* Pointer to the sceSysconPowerStandby() function. */
	s32 (*pSysconPowerStandby)(u32); // 56
	/* Pointer to a function to change the clock voltage. */
	s32 (*pChangeClockVoltage)(s32, s32, s32); // 60
	/* The second BARYON status. */
	u8 baryonStatus2; // 64
	/* Unknown.*/
	s32 unk68; // 68
	/* Pointer to the sceI2cMasterTransmit() function. */
	s32 (*pI2cTransmit)(u32, u8 *, s32); // 72
	/* Pointer to the sceI2cMasterReceive() function. */
	s32 (*pI2cReceive)(u32, u8 *, s32); // 76
	/* Pointer to the sceI2cMasterTransmitReceive() function. */
	s32 (*pI2cTransmitReceive)(u32, u8 *, s32, u32, u8 *, s32); // 80
	u8 remainingData[0x80]; // 84 -- remaining unknowns, for convenience represented as an array for now
} ScePowerResumeInfo; // size = 212

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
 * during a power locked state, if the ::scePowerLockForKernel() function is used carelessly, it can 
 * significantly decrease the usability of the PSP system.
 */
s32 scePowerLockForKernel(s32 lockType);

/**
 * @brief Cancels a power off lock.
 * 
 * This function cancels a power lock that was previously set with the ::scePowerLockForKernel() API.
 * 
 * @param lockType Power processing lock type. Specify ::SCE_KERNEL_POWER_LOCK_DEFAULT.
 * 
 * @return The remaining existing power locks (>= 0).
 */
s32 scePowerUnlockForKernel(s32 lockType);

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
 * @attention When an application has obtained exclusive access, a PSP suspend/standby operation cannot
 * be successfully completed until the application has relinquished its control. See
 * ::scePowerVolatileMemUnlock() for more details.
 * 
 * @see ::scePowerVolatileMemTryLock()
 * @see ::scePowerVolatileMemUnlock()
 */
s32 scePowerVolatileMemLock(s32 mode, void **ppAddr, SceSize *pSize);

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
 * @attention When an application has obtained exclusive access, a PSP suspend/standby operation cannot
 * be successfully completed until the application has relinquished its control. See
 * ::scePowerVolatileMemUnlock() for more details.
 * 
 * @see ::scePowerVolatileMemLock()
 * @see ::scePowerVolatileMemUnlock()
 */
s32 scePowerVolatileMemTryLock(s32 mode, void **ppAddr, SceSize * Size);

/**
 * @brief Relinquishes exclusive access to the PSP's RAM area reserved for volatile memory.
 * 
 * This function relinquishes exlusive access to the memory area reserved for saving volatile memory, 
 * which had been previously obtained by using the ::sceKernelVolatileMemLock() or 
 * ::sceKernelVolatileMemTryLock() APIs back to the kernel. The kernel's Utility modules (OSK, game sharing,...)
 * will have access to it again.
 * 
 * @param mode Currently only specify ::SCE_KERNEL_VOLATILE_MEM_DEFAULT.
 * 
 * @return SCE_ERROR_OK on success, otherwise < 0.
 * 
 * @attention Only call this function when you have previously obtained exclusive usage rights (access) for the
 * volatile memory area. If this fucntion is called without having exclusive access at the time, the currently
 * held exclusive access (potentially by another application) will be relinquished and the result of the
 * operation cannot be guaranteed.
 * 
 * @attention When an application has obtained exclusive usage rights, it needs to quickly relinquish these rights
 * when the PSP is suspending/standing by in order for the suspend/standby operation to be completed
 * successfully. A power callback registered with ::scePowerRegisterCallback() can be used for this.
 * @par Example:
 * @code
 * 
 * // Note: Previously registered with scePowerRegisterCallback().
 * int powerCallback(int count, int arg, void *common)
 * {
 *		if (arg & (SCE_POWER_CALLBACKARG_SUSPENDING | SCE_POWER_CALLBACKARG_STANDINGBY))
 *		{
 *			scePowerVolatileMemUnlock(SCE_KERNEL_VOLATILE_MEM_DEFAULT);
 *		}
 * }
 * @endcode
 * 
 * @see ::scePowerVolatileMemLock()
 * @see ::scePowerVolatileMemTryLock()
 * @see ::scePowerRegisterCallback()
 */
s32 scePowerVolatileMemUnlock(s32 mode);

/* WLAN functions */

/**
 * Attempts to activate WLAN.
 * 
 * @remark The PSP system's PLL might operate at a clock frequency where WLAN cannot be used. For example, 
 * the PSP-1000 series cannot use WLAN when it's PLL operates at a clock frequency of either 266 MHz or 333 MHZ.
 * In such a case WLAN activation fails and the PLL clock frequency needs to be reduced first.
 * @remark Battery charging might be suppressed while WLAN is active.
 * 
 * @return SCE_ERROR_OK on success, otherwise < 0.
 */
s32 scePowerWlanActivate(void);

/**
 * Deactivates WLAN. 
 * 
 * @remark If battery charging was disabled previously due to an active WLAN, charging will be enabled again.
 * 
 * @return Always SCE_ERROR_OK.
 */
s32 scePowerWlanDeactivate(void);

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

/* Specifies that WLAN can be operated at the maximum PLL clock frequency. */
#define SCE_POWER_WLAN_EXCLUSIVE_PLL_CLOCK_LIMIT_NONE		0
/** Specifies that WLAN can only be operated at a PLL clock frequency up to 222 MHz. */
#define SCE_POWER_WLAN_EXCLUSIVE_PLL_CLOCK_LIMIT_222Mhz		1
/** Specifies that WLAN can only be operated at a PLL clock frequency up to 266 MHz. */
#define SCE_POWER_WLAN_EXCLUSIVE_PLL_CLOCK_LIMIT_266Mhz		2

/**
 * Sets the clock frequency limit at which WLAN can be operated. If WLAN is active this limits the allowed
 * clock frequencies. If WLAN is OFF, this limit determines whether the PSP currently runs at a clock frequency 
 * slow enough so that WLAN can be successfully activated.
 * 
 * @param clockLimit Specifies the clock frequency limit.
 *
 * @return Always SCE_ERROR_OK.
 */
s32 scePowerSetExclusiveWlan(u8 pllClockLimit);

/**
 * Checks if the PSP system is allowed to operate at the given PLL clock frequency when WLAN is active.
 * 
 * @param pllFrequency The PLL clock frequency to check for.
 * 
 * @return SCE_ERROR_OK if allowed, otherwise < 0.
 */
s32 scePowerCheckWlanCondition(s32 pllFrequency);

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

/**
 * @brief Forbids battery charging.
 * 
 * Thus function forbids battery charging. Note that there might be a slight delay between when
 * this API returns and when the battery will actually no longer be charged.
 *
 * @return Always SCE_ERROR_OK.
 */
s32 scePowerBatteryForbidCharging(void);

/**
 * @brief Permits battery charging.
 * 
 * This function allows the battery to be charged. Note that there might be a slight delay between when
 * this API returns and when the battery will actually be charged again.
 * 
 * @attention This function is to be used in pairs with ::scePowerBatteryForbidCharging(). After
 * battery charging has been forbidden, battery charging will only be permitted again if each
 * ::scePowerBatteryForbidCharging() function call has been paired with a corresponding 
 * ::scePowerBatteryPermitCharging() call. If there are ::scePowerBatteryForbidCharging() calls
 * without a matching ::scePowerBatteryPermitCharging() function call, battery charging will remain
 * forbidden.
 * 
 * @return Always SCE_ERROR_OK.
 * 
 * @see ::scePowerBatteryForbidCharging()
 */
s32 scePowerBatteryPermitCharging(void);

/**
 * @brief Gets the USB charging capability of the PSP device.
 *
 * This function returns whether the PSP device has in-built support for USB charging or not. Typically,
 * starting with the PSP-2000 series, every PSP model has support for USB charging.
 *
 * @return SCE_TRUE if USB charging is supported, SCE_FALSE otherwise.
 */
s32 scePowerGetUsbChargingCapability(void);

/**
 * @brief Enables USB charging.
 * 
 * This function enables USB charging. While this function can be called on any PSP device, USB charging
 * is only actually supported on PSP devices which have in-built hardware support for USB charging.
 * Specifically, calling this API on the PSP-1000 series will result in an error.
 * 
 * Note that charging the battery via an AC adapter takes precedence over USB charging. As such, while USB
 * charging might be enabled, the battery will actually only be charged over USB if the PSP device is not
 * connected to an external power source via an AC adapter.
 * 
 * @return The previous USB charging enabled status on success (0 = disabled/1 = enabled).
 * @return 0 If the power service currently cannot enable USB charging. Since this return value
 * might be the same value returned on success (if USB charging was previously disabled), you can
 * call this API again and check its returns value. If it returns "1", USB charging has been enabled
 * successfully.
 * @return < 0 Error.
 * 
 * @see ::scePowerBatteryDisableUsbCharging()
 */
s32 scePowerBatteryEnableUsbCharging(void);

/**
 * Disables USB charging. 
 * 
 * This function disables USB charging. While this function can be called on any PSP device, USB charging
 * is only actually supported on PSP devices which have in-built hardware support for USB charging.
 * Specifically, calling this API on the PSP-1000 series will result in an error.
 * 
 * @return SCE_ERROR_OK on success, otherwise < 0.
 * 
 * @see ::scePowerBatteryEnableUsbCharging()
 */
s32 scePowerBatteryDisableUsbCharging(void);

/**
 * Defines constants indicating whether the type of the equipped battery supports battery state monitoring.
 * If battery state monitoring is supported, state information such as the full/remaining battery capacity,
 * or the current battery voltage/temperatur can be obtained through the power service.
 */
typedef enum {
	/**
	 * Describes a battery type which supports battery state monitoring. This type of battery is equipped by
	 * the PSP-1000, PSP-2000 and PSP-3000 models.
	 */
	SCE_POWER_BATTERY_TYPE_BATTERY_STATE_MONITORING_SUPPORTED = 0,
	/**
	 * Describes a battery type which does not supports battery state monitoring. This type of battery is
	 * equipped by the PSP Go and the PSP-E1000 models.
	 */
	 SCE_POWER_BATTERY_TYPE_BATTERY_STATE_MONITORING_NOT_SUPPORTED
} ScePowerBatteryType;

/**
 * @brief Gets the battery type.
 *
 * This function gets the type of the battery equipped in a standard PSP system. The battery type indicates
 * whether battery properties like the remaining battery lifetime/temperature/full capacity can be
 * (accurately) obtained.
 *
 * PSP devices ranging from the PSP-1000 series to the PSP-3000 series can obtain these battery properties,
 * whereas the PSP Go and PSP Street (PSP E-1000) series cannot.
 *
 * Only use this API to determine the battery type. Do not use it for anything else (like determining the PSP
 * hardware model).
 *
 * @return The battery type. One of ::ScePowerBatteryType.
 */
s32 scePowerGetBatteryType(void);

/**
 * @brief Gets the full capacity of the battery.
 *
 * This function gets the full capacity of the equipped battery in mAh.
 *
 * @attention Call this API only on devices which support battery monitoring as indicated by
 * ::scePowerGetBatteryType(). On PSP systems which do not support battery monitoring incorrect values will be
 * returned!
 *
 * @return The full battery capacity on success.
 * @return SCE_POWER_ERROR_NO_BATTERY No battery equipped.
 * @return SCE_POWER_ERROR_DETECTING The power service is busy detecting the new battery status.
 */
s32 scePowerGetBatteryFullCapacity(void);

/**
 * @brief Gets the remaining battery capacity.
 *
 * This function gets the remaining battery capacity in mAh.
 *
 * @attention While this API can be called on any PSP system, a valid remaining battery capacity will
 * only be returned on PSP devices which have battery monitoring capabilities as indicated by
 * ::scePowerGetBatteryType(). Calling this API on PSP systems which do not have such a
 * capability will return an incorrect value!
 *
 * @return The remaining battery capacity on success.
 * @return SCE_POWER_ERROR_NO_BATTERY No battery equipped.
 * @return SCE_POWER_ERROR_DETECTING The power service is busy detecting the new battery status.
 *
 * @remark The correct value may not be returned after a battery is installed until the power service
 * polls and recognizes that the battery has been equipped.
 * 
 * @see ::scePowerGetBatteryLifeTime()
 * @see ::scePowerGetBatteryLifePercent()
 */
s32 scePowerGetBatteryRemainCapacity(void);

/**
 * @brief Gets the estimated continuous remaining battery lifetime.
 *
 * This function gets the estimated continuous remaining battery lifetime in minutes. When the PSP system
 * is connected to an external power source via an AC adapter, 0 is returned since the continuous remaining
 * time cannot be estimated.
 *
 * This function takes the force-suspend capacity threshold value as the baseline for the remaining battery
 * lifetime. If the remaining battery capacity is currently very close to the force-suspend capacity, 0 is
 * returned as well. As such, the returned remaining battery lifetime is the lifetime available to the user before
 * the PSP system is automatically suspended.
 *
 * @attention While this API can be called on any PSP system, a valid estimated remaining battery lifetime will
 * only be returned on PSP devices which have battery monitoring capabilities as indicated by
 * ::scePowerGetBatteryType(). Calling this API on PSP systems which do not have such a
 * capability will return an incorrect value!
 *
 * @return The estimated continuous remaining battery lifetime on success.
 * @return SCE_POWER_ERROR_NO_BATTERY No battery equipped.
 * @return SCE_POWER_ERROR_DETECTING The power service is busy detecting the new battery status.
 * @return < 0 Error.
 *
 * @remark The correct value may not be returned after a battery is installed until the power service
 * polls and recognizes that the battery has been equipped.
 * 
 * @see ::scePowerGetBatteryLifePercent()
 * @see ::scePowerGetBatteryRemainCapacity()
 */
s32 scePowerGetBatteryLifeTime(void);

/**
 * @brief Gets the remaining percentage of battery life relative to the fully charged status.
 *
 * This function gets the remaining battery life as a percentage relative to the fully charged status.
 *
 * This function takes the force-suspend capacity threshold value as the baseline for the remaining battery
 * life. If the remaining battery capacity is currently very close to the force-suspend capacity, 0 is
 * returned. As such, the returned remaining relative battery life is the battery life available to the user
 * before the PSP system is automatically suspended.
 *
 * @attention Call this API only on devices which support battery monitoring as indicated by
 * ::scePowerGetBatteryType(). On PSP systems which do not support battery monitoring incorrect values will be
 * returned!
 *
 * @return The remaining battery life in percent [0-100] on success.
 * @return SCE_POWER_ERROR_NO_BATTERY No battery equipped.
 * @return SCE_POWER_ERROR_DETECTING The power service is busy detecting the new battery status.
 *
 * @remark This status can also be obtained using a power callback. See ::scePowerRegisterCallback() for more
 * details. As mentioned above, correct values are only obtained on PSP devices which have battery
 * monitoring capabilities.
 * 
 * @see ::scePowerGetBatteryLifeTime()
 * @see ::scePowerGetBatteryRemainCapacity()
 */
s32 scePowerGetBatteryLifePercent(void);

/**
 * @brief Gets the current battery voltage.
 *
 * This function gets the current battery voltage in mV (millivolts).
 *
 * @return The current battery voltage on success.
 * @return SCE_POWER_ERROR_NO_BATTERY No battery equipped.
 * @return SCE_POWER_ERROR_DETECTING The power service is busy detecting the new battery status.
 */
s32 scePowerGetBatteryVolt(void);

/**
 * @brief Gets the current temperature of the battery. 
 * 
 * Gets the current battery temperature in degree Celsius.
 * 
 * @attention While this API can be called on any PSP system, a valid temperature value will only be
 * returned on PSP devices which have battery monitoring capabilities as indicated by
 * ::scePowerGetBatteryType(). Calling this API on PSP systems which do not have such a
 * capability will return an error!
 * 
 * @return The current battery temperature on success.
 * @return SCE_POWER_ERROR_NO_BATTERY No battery equipped.
 * @return SCE_POWER_ERROR_DETECTING The power service is busy detecting the new battery status.
 * @return < 0 Error.
 */
s32 scePowerGetBatteryTemp(void);

/**
 * Gets the current electric charge value of the battery.
 * 
 * @param pBatteryElec Pointer to an u32 which is to receive the electric charge value.
 * 
 * @attention Do not specify a NULL pointer as the argument. Otherwise the PSP system will crash.
 * 
 * @return SCE_ERROR_OK on success.
 * @return SCE_POWER_ERROR_NO_BATTERY No battery equipped.
 * @return SCE_POWER_ERROR_DETECTING The power service is busy detecting the new battery status.
 * @return < 0 Error.
 * 
 * @attention While this API can be called on any PSP system, a valid electric charge value will only be
 * returned on PSP devices which have battery monitoring capabilities as indicated by
 * ::scePowerGetBatteryType(). Calling this API on PSP systems which do not have such a
 * capability will return an error!
 */
s32 scePowerGetBatteryElec(u32 *pBatteryElec);

/**
 * @brief Gets the charge cycle status of the battery.
 * 
 * @attention While this API can be called on any PSP system, a valid charge cycle will only be
 * returned on PSP devices which have battery monitoring capabilities as indicated by
 * ::scePowerGetBatteryType(). Calling this API on PSP systems which do not have such a
 * capability will return an error!
 * 
 * @return The current charge cycle count on success.
 * @return SCE_POWER_ERROR_NO_BATTERY No battery equipped.
 * @return SCE_POWER_ERROR_DETECTING The power service is busy detecting the new battery status.
 * @return < 0 Error.
 */
s32 scePowerGetBatteryChargeCycle(void);

/**
 * @brief Gets the suspend-required status.
 *
 * This function gets whether or not the PSP system should be suspended due to very low battery capacity.
 * If the PSP is currently connected to an external power source via an AC adapter, there is no need
 * to suspend the PSP device no matter its current battery capacity.
 *
 * @return SCE_TRUE if the PSP system needs to be suspended, SCE_FALSE otherwise.
 */
s32 scePowerIsSuspendRequired(void);

/**
 * @brief Gets the force-suspend battery capacity threshold value.
 *
 * This function gets the battery capacity value at which the PSP system is force suspended by the system.
 *
 * @return The force-suspend battery capacity threshold value in mAh.
 *
 * @see ::scePowerIsSuspendRequired().
 */
s32 scePowerGetForceSuspendCapacity(void);

/**
 * @brief Gets the low battery status capacity threshold value.
 *
 * This function gets the battery capacity value at which it enters the low battery state.
 *
 * @return The low battery capacity threshold value in mAh.
 *
 * @see ::scePowerIsLowBattery().
 */
s32 scePowerGetLowBatteryCapacity(void);

/**
 * @brief Gets the PSP system chip's temperature.
 *
 * @note As of at least PSP fimrware version 3.50 this API has been "disabled". It will now only
 * return a constant representing an error.
 *
 * @return Always SCE_POWER_ERROR_0010.
 */
s32 scePowerGetInnerTemp(float *pInnerTemp);

/**
 * @brief Requests the power service to poll the battery for new data.
 *
 * This function tells the power service to poll the battery for new data (remaining capacity, voltage,
 * temperature,...) and to refresh its internal battery data.
 *
 * @return Always SCE_ERROR_OK.
 *
 * @remark Typically, you don't have to call this function as the PSP system will consistently poll the battery
 * for new data.
 *
 * @remark There might be a slight delay between returning from this call and when a new set of battery data
 * is available in the power service.
 */
s32 scePowerBatteryUpdateInfo(void);

/* Idle timer */

/**
 * 
 * @brief Cancels an idle timer.
 * 
 * This function cancels an idle timer by resetting its current idle time counter.
 * 
 * In idle state, the system automatically performs power save processing such as turning off the
 * LCD backlight. Using the ::scePowerTick() function to cancel the count value for the idle state prevents
 * the specified power save processing from being performed.
 * 
 * Note that calling this function only resets a timer counter and does not stop the counting itself. As such,
 * to continuously prevent the system from performing power saving operations, call ::scePowerTick() repeatedly,
 * for example once every VBLANK interval.
 * 
 * @param tickType Specify the idle timer you want to reset. Pick one of the following: 
   - ::SCE_KERNEL_POWER_TICK_DEFAULT - Cancels all timers.
   - ::SCE_KERNEL_POWER_TICK_SUSPENDONLY - Cancels only the timer related to automatic suspension.
   - ::SCE_KERNEL_POWER_TICK_LCDONLY - Cancels the timer related to the LCD.
 * 
 * @returns Always SCE_ERROR_OK.
 */
s32 scePowerTick(s32 tickType);

/**
 * Defines attributes for an idle timer callback to be specified when registering the callback using
 * ::scePowerSetIdleCallback(). Multiple attributes can be specified.
 */
typedef enum {
	/*
	 * Specifies that the idle timer's time counter is not automatically resetted constantly when the
	 * PSP system is connected to an external power source via an AC adapter.
	 */
	SCE_POWER_IDLE_TIMER_CALLBACK_ATTR_NO_AUTO_RESET_ON_AC_CONNECTION	= 0x100,
	/* Specifies that the idle timer's time counter cannot be resetted using the ::scePowerTick() API. */
	SCE_POWER_IDLE_TIMER_CALLBACK_ATTR_CANNOT_RESET						= 0x200,
} ScePowerIdleTimerCallbackAttr;

/**
 * Power service idle timer callback function prototype.
 *
 * @param slot The slot registration number of the invoked idle timer callback.
 * @param ellapsedTime The low-order 32 bit value of the ellapsed time since the idle timer callback
 * was last resetted or registered.
 * @param common Custom callback value. Specified by the same parameter in ::scePowerSetIdleCallback().
 *
 * @see ::scePowerSetIdleCallback()
 */
typedef void (*ScePowerIdleTimerCallback)(s32 slot, u32 ellapsedTime, void *common);

/**
 * @brief Gets the timing state for a registered idle timer callback.
 * 
 * @param slot The registered idle timer callback the timing state should be obtained for. Specify value in
 * range 0 to 7.
 * @param pCurEllapsedTime Pointer to an u64 which is to retrieve the passed time in microseconds since the
 * timer was last resetted.
 * @param pCurDueTime Pointer to an u64 which is to retrieve the remaining time in microseconds until the
 * idle timer callback will be called.
 * 
 * @return The low-order 32-bit offset between the current system time and the idle timer callback's base time.
 * 
 * @see ::scePowerSetIdleCallback()
 */
s32 scePowerGetIdleTimer(s32 slot, u64 *pCurEllapsedTime, u64 *pCurDueTime);

/**
 * @brief Registers an idle timer callback in the system.
 * 
 * This function registers an idle timer callback in the system.
 * 
 * A timer callback cannot overwrite an already registered idle timer callback in the same slot. If you want
 * to register a new timer callback when all slots are already in use, you first need to unregister an existing
 * idle timer callback and then use the freed slot to register your new idle timer callback.
 * 
 * @note If the idle timer for the given slot has been previously disabled by calling ::scePowerIdleTimerDisable()
 * you need to enable the idler timer for this slot again using ::scePowerIdleTimerEnable().
 *
 * @param slot The callback registration slot. Specify value in range of 0 to 7.
 * @param attr The callback attributes. One or more of ::ScePowerIdleTimerCallbackAttr.
 * @param dueTime The amount of (idle) time to pass until the callback is invoked. In microseconds.
 * @param callback Pointer to the callback function. Specify [null] if you want to unregister an idle timer
 * callback.
 * @param common Argument to be passed to the callback function.
 * 
 * @return SCE_ERROR_OK on success, otherwise < 0.
 * 
 * @remark The specified callback might not just be invoked when the specified due time is reached. For example,
 * when a timer, which had previously reached its due time, has been resetted by calling ::scePowerTick(), the
 * system invokes its callback. However, the ellapsedTime argument of the callback will be set to 0 in this case
 * so you can differentiate between this case and when the callback is invoked due to idle timer reaching its
 * due time (where the ellapsedTime argument is set to a value greater than 0).
 * 
 * @see ::scePowerGetIdleTimer()
 */
s32 scePowerSetIdleCallback(s32 slot, u32 attr, u64 dueTime, ScePowerIdleTimerCallback callback, void *common);

/**
 * @brief Enables an idle timer.
 * 
 * This function enables an idle timer. If an idle callback is registered for this idle timer with
 * ::scePowerSetIdleCallback(), its callback function is invoked when the idle timer counter has
 * reached/passed the specified due time for the idle timer callback.
 * 
 * @param slot The callback registration slot. Specify value in range of 0 to 7.
 * 
 * @return The previous enabled state of the idle timer on success, otherwise < 0.
 * 
 * @see ::scePowerIdleTimerDisable()
 */
s32 scePowerIdleTimerEnable(s32 slot);

/**
 * @brief Disables an idle timer.
 *
 * This function disables an idle timer. If an idle callback is registered for this idle timer with
 * ::scePowerSetIdleCallback(), its callback function is no longer invoked when the idle timer counter
 * has reached/passed the specified due time for the idle timer callback.
 * 
 * In the current implementation, disabling an idle timer will also disable all idle timers for the slots
 * 0 to (slots - 1). Other than the idle timer assigned to the first slot, no other idle timer can be
 * individually disabled.
 *
 * @param slot The callback registration slot. Specify value in range of 0 to 7.
 *
 * @return The previous enabled state of the idle timer on success, otherwise < 0.
 * 
 * @see ::scePowerIdleTimerEnable()
 */
s32 scePowerIdleTimerDisable(s32 slot);

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

/**
 * Gets the Grahpic Engine's current eDRAM refresh mode.
 * 
 * @return The refresh mode.
 */
s32 scePowerGetGeEdramRefreshMode(void);

/**
 * Sets the Graphic Engine's eDRAM refresh mode.
 * 
 * @param geEdramRefreshMode The refresh mode. Pass either 0 or 1.
 * 
 * @return SCE_ERROR_OK on success, otherwise < 0.
 */
s32 scePowerSetGeEdramRefreshMode(s32 geEdramRefreshMode);

/* Disabled functions */

/**
 * Dummy function. Does nothing.
 * 
 * @return Always SCE_ERROR_OK.
 */
s32 scePower_driver_D79B0122(void);

 /** @} */

#endif	/* POWER_KERNEL_H */
