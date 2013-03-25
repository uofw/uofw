/* Copyright (C) 2011, 2012, 2013 The uOFW team
   See the file COPYING for copying permission.
*/

#include "common_header.h"

/**
 * The time-up handler used by ::sceSTimerSetHandler(). \n
 * When the hardware timer counter register matches the comparison value
 * that was set by ::sceSTimerSetHandler(), the time-up handler is called.
 * 
 * When the value returned by the time-up handler is not equal to -1, timer \n
 * counting is stopped and the timer is set to "not in use".
 * 
 * The following arguments don't need to be used in the time-up routine.
 * 
 * @param timerID The ID of the timer this time-up handler belongs to.
 * @param count Counter value.
 * @param common Pass the common argument specified in ::sceSTimerSetHandler().
 * @param unk Unknown.
 * 
 * Typically return -1.
 */
typedef s32 (*SceSysTimerCb)(s32 timerId, s32 count, void *common, s32 unk);

/**
 * Obtain a hardware timer. Cannot be called from an interrupt handler. 
 * 
 * @return The timer id (greater than or equal to 0) on success.
 */
s32 sceSTimerAlloc(void);

/**
 * Return the hardware timer that was obtained  by ::sceSTimerAlloc().\n
 * Cannot be called from an interrupt handler.
 * 
 * @param timerId The ID of the timer to return.
 * 
 * @return SCE_ERROR_OK on success.
 */
s32 sceSTimerFree(s32 timerId);

/**
 * Start hardware timer counting. The timer is set to "in use" state.
 * 
 * @param timerId The ID of the timer to start counting.
 * 
 * @return SCE_ERROR_OK on success.
 */
s32 sceSTimerStartCount(s32 timerId);

/**
 * Read the current value of hardware timer's counter register.
 * 
 * @param timerId The ID of the timer to obtain the timer counter value from.
 * @param count A pointer where the obtained counter value will be stored into.
 * 
 * @return SCE_ERROR_OK on success.
 */
s32 sceSTimerGetCount(s32 timerId, s32 *count);

/**
 * Reset the hardware timer's counter register to 0.
 * 
 * @param timerId The ID of the timer to reset its counter.
 * 
 * @return SCE_ERROR_OK on success.
 */
s32 sceSTimerResetCount(s32 timerId);

/**
 * Stop the hardware timer counting and set the timer's state to "not in use".
 * 
 * @param timerId The ID of the timer to stop its counting.
 * 
 * @return SCE_ERROR_OK on success.
 */
s32 sceSTimerStopCount(s32 timerId);

/**
 * Set the prescale of a hardware timer. It can be only set on timers which are in the "not in use" state.\n
 * The input signal is divided into the resulting ratio. The ratio has to be greater than 1/11.
 * 
 * @param timerId The ID of the timer to set the prescale.
 * @param numerator The numerator of the prescale. Must not be 0.
 * @param denominator The denumerator of the prescale. Must not be 0.
 * 
 * @return SCE_ERROR_OK on success.
 */
s32 sceSTimerSetPrscl(s32 timerId, s32 numerator, s32 denominator);

/**
 * Set the comparison value and the time-up handler of the hardware timer counter register.\n
 * The hardware timer counter register begins counting up from zero. If the counter register matches \n
 * the comparison value, an interrupt occurs, the counter register is returned to zero, and counting \n
 * continues. The time-up handler is called via this interrupt.
 * 
 * @param timerId The ID of the timer to set the compare value and time-up handler.
 * @param compareValue The count comparison value. Should not be greater 4194303 (~1/10 seconds) and less than 0.
 * @param timeUpHandler Specify the time-up handler that is called when count matches the comparison value.\n
 *                      Pass null to delete a registered time-up handler. This will also stop the hardware timer \n
 *                      counting. 
 * @param common Pointer to memory common between time-up handler and general routines.
 * 
 * @return SCE_ERROR_OK on success. 
 */
s32 sceSTimerSetHandler(s32 timerId, s32 compareValue, SceSysTimerCb timeUpHandler, void *common);

/**
 * Unknown purpose.
 * 
 * @param timerId The ID of the timer.
 * @param arg1 Unknown. Should not be less than 0 and greater 4194304.
 * 
 * @return SCE_ERROR_OK on success.
 */
s32 sceSTimerSetTMCY(s32 timerId, s32 arg1);

