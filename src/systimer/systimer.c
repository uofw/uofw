/* Copyright (C) 2011, 2012, 2013 The uOFW team
   See the file COPYING for copying permission.
*/

/*
 * uOFW/trunk/src/systimer/systimer.c
 * 
 * sceSystimer - a library to manage PSP hardware timers.
 * 
 * The system timer module's main purpose is to provide service functions 
 * for allocating and freeing hardware timers, registering interrupt handlers, 
 * starting and stopping timer counting and reading the counter value of every
 * timer.
 * 
 * There are four hardware timers which can be used by applications.
 * 
 * In addition the user can set the prescale of each timer individually.
 *
 */

#include <common_imp.h>

#include <interruptman.h>
#include <sysmem_suspend_kernel.h>
#include <systimer.h>

SCE_MODULE_INFO("sceSystimer", SCE_MODULE_KERNEL | SCE_MODULE_ATTR_CANT_STOP | SCE_MODULE_ATTR_EXCLUSIVE_LOAD
                             | SCE_MODULE_ATTR_EXCLUSIVE_START, 1, 2);
SCE_MODULE_BOOTSTART("SysTimerInit");
SCE_MODULE_REBOOT_BEFORE("SysTimerEnd");
SCE_SDK_VERSION(SDK_VERSION);

/* The maximum timer for a counter. Approximately 1/10 seconds. */
#define TIMER_MAX_COUNTER_VALUE         4194303

/* The number of hardware timers to use. */
#define TIMER_NUM_HW_TIMERS             4

/* Timer modes */
#define TIMER_MODE_HANDLER_REGISTERED   (1 << 0) /* Indicates that a time-up handler is set for the specific timer. */
#define TIMER_MODE_IN_USE               (1 << 1) /* Indicates that the timer is in use. */
#define TIMER_MODE_UNKNOWN              (1 << 9) /* Unknown timer mode. */
#define TIMER_MODE_MAX                  TIMER_MODE_UNKNOWN /* Max timer mode. Keep this the last entry. */

/* Receive the counter register value. */
#define TIMER_GET_COUNT(data)           ((data) & TIMER_MAX_COUNTER_VALUE)

/* Receive the state of the specific timer. */
#define TIMER_GET_MODE(data)            ((u32)(data) >> 22)

#define TIMER_SET_COUNT(count)          ((count) & TIMER_MAX_COUNTER_VALUE)

#define TIMER_SET_MODE(mode)            ((mode) << 22)

/*
 * This structure represents a hardware timer.
 */
typedef struct {
    /*
     *  31        22 21            0
     * +------------+--------------+
     * | TIMER MODE |  APPLY TIME  |
     * +------------+--------------+
     * 
     * Bit 31 - 22:
     *    The current timer mode.
     * 
     * Bit 21 - 0:
     *    The system time when the timer was stopped. It is overwritten every time 
     *    the timer is stopped.
     */
    s32 timerData; 
    /* Assumed. The base time of the hardware timer. */
    s32 baseTime;
    /* The numerator of the timer's prescale. */
    s32 prsclNumerator;
    /* The denominator of the timer's prescale. */
    s32 prsclDenominator;
    /* Reserved. */
    s32 rsrv[240];
    /*
     *  31        22 21            0
     * +------------+--------------+
     * | TIMER MODE |  APPLY TIME  |
     * +------------+--------------+
     * 
     * Bit 31 - 22:
     *    The current timer mode.
     * 
     * Bit 21 - 0:
     *    The current PSP's system time value. A timer's current count is computed
     *    by subtracting the timer's base time (the init time point) from this value.
     */
    s32 nowData;
} SceHwTimer;

/* This structure represents a timer connected to a hardware timer. */
typedef struct {
    /* The assigned hardware timer. */
    SceHwTimer *hw;
    /* The interrupt number belonging to this timer. */
    s32 intrNum;
    /* The ID of the timer. */
    s32 timerId;
    /* Unknown. */
    s32 unk12;
    /* Unknown. */
    s32 unk16;
    /* The time-up handler for the timer. */
    SceSysTimerCb cb;
    /* The count value for the timer. */
    s32 count; 
    /* Pointer to memory common between time-up handler and general routines. */
    void *common;
} SceSysTimer;

/* 
 * This structure saves important timer data when a timer is suspended, 
 * so this data can be re-set when the timer is resumed.
 */
typedef struct {
    /*
     *  31        22 21            0
     * +------------+--------------+
     * | TIMER MODE |  APPLY TIME  |
     * +------------+--------------+
     * 
     * Bit 31 - 22:
     *    The saved timer mode.
     * 
     * Bit 21 - 0:
     *    The saved system time when the timer was stopped.  It is set when the 
     *    corresponding timer is suspended.
     */
    s32 sTimerData;
    /* The numerator of the timer's prescale. */
    s32 sPrsclNumerator; 
    /* The denominator of the timer's prescale. */
    s32 sPrsclDenominator;
} SceSysTimerSave;

SceSysTimer timers[TIMER_NUM_HW_TIMERS] = {
    [0] = {
        .hw = (void *)HWPTR(HW_TIMER_0),
        .intrNum = SCE_SYSTIMER0_INT,
        .timerId = -1,
        .unk12 = 0,
        .unk16 = 0,
        .cb = NULL,
        .count = 0,
        .common = NULL,
    },
    [1] = {
        .hw = (void *)HWPTR(HW_TIMER_1),
        .intrNum = SCE_SYSTIMER1_INT,
        .timerId = -1,
        .unk12 = 0,
        .unk16 = 0,
        .cb = NULL,
        .count = 0,
        .common = NULL,
    },
    [2] = {
        .hw = (void *)HWPTR(HW_TIMER_2),
        .intrNum = SCE_SYSTIMER2_INT,
        .timerId = -1,
        .unk12 = 0,
        .unk16 = 0,
        .cb = NULL,
        .count = 0,
        .common = NULL,
    },
    [3] = {
        .hw = (void *)HWPTR(HW_TIMER_3),
        .intrNum = SCE_SYSTIMER3_INT,
        .timerId = -1,
        .unk12 = 0,
        .unk16 = 0,
        .cb = NULL,
        .count = 0,
        .common = NULL,
    },
};

/* The base value for a timer ID. */
s32 baseTimerId = 0x00352341;

/* Collects important data for each individual hardware timer. */
SceSysTimerSave STimerRegSave[TIMER_NUM_HW_TIMERS];

static s32 systimerhandler(s32 arg0 __attribute__((unused)), SceSysTimer *timer, s32 arg2);
static void _sceSTimerStopCount(SceSysTimer *timer);
static s32 _sceSTimerGetCount(SceSysTimer *timer);
static s32 suspendSTimer(s32 unk __attribute__((unused)), void *param __attribute__((unused)));
static s32 resumeSTimer(s32 unk __attribute__((unused)), void *param __attribute__((unused)));

s32 SysTimerInit(s32 argc __attribute__((unused)), void *argp __attribute__((unused)))
{
    s32 oldIntr = sceKernelCpuSuspendIntr();
    
    s32 i;
    for (i = 0; i < TIMER_NUM_HW_TIMERS; i++) {
        timers[i].hw->timerData = TIMER_SET_MODE(TIMER_MODE_UNKNOWN) | TIMER_SET_COUNT(0);
        timers[i].hw->prsclNumerator = -1;
        timers[i].hw->prsclDenominator = -1;
        //(void)timers[i].hw->unk0;
        timers[i].cb = NULL;
        timers[i].unk12 = 0;
        timers[i].unk16 = 0;
        timers[i].count = 0;
        timers[i].common = NULL;
        timers[i].timerId = -1;
        sceKernelRegisterIntrHandler(timers[i].intrNum, 2, systimerhandler, &timers[i], NULL);
    }
    sceKernelRegisterSuspendHandler(10, suspendSTimer, NULL);
    sceKernelRegisterResumeHandler(10, resumeSTimer, NULL);
    
    sceKernelCpuResumeIntr(oldIntr);
    return SCE_ERROR_OK;
}

s32 SysTimerEnd()
{
    s32 oldIntr = sceKernelCpuSuspendIntr();
    
    s32 i;
    for (i = 0; i < TIMER_NUM_HW_TIMERS; i++) {
        _sceSTimerStopCount(&timers[i]);
        sceKernelReleaseIntrHandler(timers[i].intrNum);
    }
    sceKernelCpuResumeIntr(oldIntr);
    return SCE_ERROR_OK;
}

/* Update a timer's counter register value and call the time-up handler. */
static s32 systimerhandler(s32 arg0 __attribute__((unused)), SceSysTimer *timer, s32 arg2)
{
    if (timer->cb == NULL)
        return -1;
    
    s32 v1 = TIMER_GET_MODE(timer->hw->timerData);
    s32 v2 = TIMER_GET_COUNT(timer->hw->timerData);
    if (timer->unk12 != 0) {
        v1--;
        timer->count += timer->unk12;
    }
    timer->count += v2 * (v1 - timer->unk16);
    timer->unk12 = 0;
    timer->unk16 = 0;
    if (timer->cb(timer->timerId, timer->count + _sceSTimerGetCount(timer), timer->common, arg2) != -1) {
        _sceSTimerStopCount(timer);
        return -2;
    }
    return -1;
}

/* Stop the hardware timer counting. */
static void _sceSTimerStopCount(SceSysTimer *timer)
{
    //timer->hw->timerData = timer->hw->data & ~(TIMER_MODE_IN_USE | 0x80000000);
    timer->hw->timerData = TIMER_SET_MODE(TIMER_GET_MODE(timer->hw->nowData) & ~(TIMER_MODE_IN_USE | TIMER_MODE_UNKNOWN));
    timer->hw->timerData |= TIMER_SET_COUNT(TIMER_GET_COUNT(timer->hw->nowData));
}
/* Get the current value of the hardware timer counter register. */
static s32 _sceSTimerGetCount(SceSysTimer *timer)
{
    //TODO: timer->hw->ulNowTime - timer->hw->ulBaseTime ?
    return (TIMER_GET_COUNT(timer->hw->nowData) - TIMER_GET_COUNT(timer->hw->baseTime));
}

/* 
 * Suspend all timers, save their states and counter value, and prohibit 
 * the timer interrupts.
 */
static s32 suspendSTimer(s32 unk __attribute__((unused)), void *param __attribute__((unused)))
{
    s32 i;
    for (i = 0; i < TIMER_NUM_HW_TIMERS; i++) {
        STimerRegSave[i].sTimerData = timers[i].hw->timerData;
        STimerRegSave[i].sPrsclNumerator = timers[i].hw->prsclNumerator;
        STimerRegSave[i].sPrsclDenominator = timers[i].hw->prsclDenominator;
        sceKernelDisableIntr(timers[i].intrNum);
    }
    return SCE_ERROR_OK;
}

/* 
 * Resume all timers, set their previous states and counter value and enable 
 * the timer interrupts.
 */
static s32 resumeSTimer(s32 unk __attribute__((unused)), void *param __attribute__((unused)))
{
    s32 i;
    for (i = 0; i < TIMER_NUM_HW_TIMERS; i++) {
        timers[i].hw->prsclDenominator = STimerRegSave[i].sPrsclDenominator;
        timers[i].hw->prsclNumerator = STimerRegSave[i].sPrsclNumerator;
        //(void)timers[i].hw->unk0;
        timers[i].hw->timerData = STimerRegSave[i].sTimerData & ~0x80000000;
        sceKernelEnableIntr(timers[i].intrNum);
    }
    return SCE_ERROR_OK;
}

s32 sceSTimerAlloc(void)
{
    if (sceKernelIsIntrContext())
        return SCE_ERROR_KERNEL_CANNOT_BE_CALLED_FROM_INTERRUPT;
    
    s32 oldIntr = sceKernelCpuSuspendIntr();
    
    s32 i;
    for (i = 0; i < TIMER_NUM_HW_TIMERS; i++) {
        if (timers[i].timerId == -1) {
            timers[i].cb = NULL;
            //timers[i].hw->timerData = 0x80000000;
            timers[i].hw->timerData = TIMER_SET_MODE(TIMER_MODE_UNKNOWN) | TIMER_SET_COUNT(0);
            timers[i].hw->prsclNumerator = -1;
            timers[i].hw->prsclDenominator = -1;
            //(void)timers[i].hw->unk0;
            timers[i].unk12 = 0;
            timers[i].unk16 = 0;
            timers[i].count = 0;
            timers[i].common = NULL;
            timers[i].timerId = ((baseTimerId << 2) | i) & 0x7FFFFFFF;
            baseTimerId += 7;
            
            sceKernelCpuResumeIntr(oldIntr);
            return timers[i].timerId;
        }
    }
    sceKernelCpuResumeIntr(oldIntr);
    return SCE_ERROR_KERNEL_NO_TIMER;
}

s32 sceSTimerFree(s32 timerId)
{
    SceSysTimer *timer = &timers[timerId & 3];
    if (sceKernelIsIntrContext())
        return SCE_ERROR_KERNEL_CANNOT_BE_CALLED_FROM_INTERRUPT;
    
    if (timer->timerId != timerId)
        return SCE_ERROR_KERNEL_ILLEGAL_TIMER_ID;
    
    s32 oldIntr = sceKernelCpuSuspendIntr();
    
    _sceSTimerStopCount(timer);
    sceKernelDisableIntr(timer->intrNum);
    //timer->hw->timerData = 0x80000000;
    timer->hw->timerData = TIMER_SET_MODE(TIMER_MODE_UNKNOWN) | TIMER_SET_COUNT(0);
    timer->hw->prsclNumerator = -1;
    timer->hw->prsclDenominator = -1;
    timer->cb = NULL;
    //(void)timer->hw->unk0;
    timer->common = NULL;
    timer->timerId = -1;
    timer->unk12 = 0;
    timer->unk16 = 0;
    timer->count = 0;
    
    sceKernelCpuResumeIntr(oldIntr);
    return SCE_ERROR_OK;
}

s32 sceSTimerStartCount(s32 timerId)
{
    SceSysTimer *timer = &timers[timerId & 3];
    if (timer->timerId != timerId)
        return SCE_ERROR_KERNEL_ILLEGAL_TIMER_ID;
    
    s32 oldIntr = sceKernelCpuSuspendIntr();
    
    if (TIMER_GET_MODE(timer->hw->nowData) & TIMER_MODE_IN_USE) {
        sceKernelCpuResumeIntr(oldIntr);
        return SCE_ERROR_KERNEL_TIMER_BUSY;
    }
    timer->hw->timerData = TIMER_SET_MODE(TIMER_GET_MODE(timer->hw->nowData) | TIMER_MODE_IN_USE);
    timer->hw->timerData |= TIMER_SET_COUNT(0);
    
    sceKernelCpuResumeIntr(oldIntr);
    return SCE_ERROR_OK;
}

s32 sceSTimerGetCount(s32 timerId, s32 *count)
{
    SceSysTimer *timer = &timers[timerId & 3];
    if (timer->timerId != timerId)
        return SCE_ERROR_KERNEL_ILLEGAL_TIMER_ID;
    
    s32 oldIntr = sceKernelCpuSuspendIntr();
    
    *count = _sceSTimerGetCount(timer);
    
    sceKernelCpuResumeIntr(oldIntr);
    return SCE_ERROR_OK;
}

s32 sceSTimerResetCount(s32 timerId)
{
    SceSysTimer *timer = &timers[timerId & 3];
    if (timer->timerId != timerId)
        return SCE_ERROR_KERNEL_ILLEGAL_TIMER_ID;
    
    s32 oldIntr = sceKernelCpuSuspendIntr();
    
    //timer->hw->timerData |= 0x80000000;
    timer->hw->timerData |= TIMER_SET_MODE(TIMER_MODE_UNKNOWN);
    timer->count = 0;
    
    sceKernelCpuResumeIntr(oldIntr);
    return SCE_ERROR_OK;
}

s32 sceSTimerStopCount(s32 timerId)
{
    SceSysTimer *timer = &timers[timerId & 3];
    if (timer->timerId != timerId)
        return SCE_ERROR_KERNEL_ILLEGAL_TIMER_ID;
    
    s32 oldIntr = sceKernelCpuSuspendIntr();
    
    _sceSTimerStopCount(timer);
    
    sceKernelCpuResumeIntr(oldIntr);
    return SCE_ERROR_OK;
}

s32 sceSTimerSetPrscl(s32 timerId, s32 numerator, s32 denominator)
{
    SceSysTimer *timer = &timers[timerId & 3];
    if (timer->timerId != timerId)
        return SCE_ERROR_KERNEL_ILLEGAL_TIMER_ID;
    
    if (numerator == 0 || denominator == 0)
        return SCE_ERROR_KERNEL_ILLEGAL_PRESCALE;
    
    if ((denominator / numerator) < 12)
        return SCE_ERROR_KERNEL_ILLEGAL_PRESCALE;
    
    s32 oldIntr = sceKernelCpuSuspendIntr();
    
    if (TIMER_GET_MODE(timer->hw->nowData) & TIMER_MODE_IN_USE) {
        sceKernelCpuResumeIntr(oldIntr);
        return SCE_ERROR_KERNEL_TIMER_BUSY;
    }
    timer->hw->prsclNumerator = numerator;
    timer->hw->prsclDenominator = denominator;
    
    sceKernelCpuResumeIntr(oldIntr);
    return SCE_ERROR_OK;
}

s32 sceSTimerSetHandler(s32 timerId, s32 compareValue, SceSysTimerCb timeUpHandler, void *common)
{
    SceSysTimer *timer = &timers[timerId & 3];
    if (timer->timerId != timerId)
        return SCE_ERROR_KERNEL_ILLEGAL_TIMER_ID;
    
    s32 oldIntr = sceKernelCpuSuspendIntr();
    
    if ((u32)compareValue > TIMER_MAX_COUNTER_VALUE)
        compareValue = TIMER_MAX_COUNTER_VALUE;
    
    if (timeUpHandler == NULL) {
        timer->cb = NULL;
        timer->count = 0;
        timer->common = NULL;
        _sceSTimerStopCount(timer);
        sceKernelDisableIntr(timer->intrNum);
    } else {
        timer->cb = timeUpHandler;
        timer->common = common;
        //timer->hw->nowData & ~TIMER_MAX_COUNTER_VALUE
        timer->hw->timerData = TIMER_SET_MODE(TIMER_GET_MODE(timer->hw->nowData)) | TIMER_SET_COUNT(0);
        timer->hw->timerData |= compareValue;
        timer->hw->timerData = timer->hw->nowData | TIMER_SET_MODE(TIMER_MODE_UNKNOWN | TIMER_MODE_HANDLER_REGISTERED);
        sceKernelEnableIntr(timer->intrNum);
    }
    sceKernelCpuResumeIntr(oldIntr);
    return SCE_ERROR_OK;
}

s32 sceSTimerSetTMCY(s32 timerId, s32 arg1)
{
    SceSysTimer *timer = &timers[timerId & 3];
    if (timer->timerId != timerId)
        return SCE_ERROR_KERNEL_ILLEGAL_TIMER_ID;
    
    s32 oldIntr = sceKernelCpuSuspendIntr();
    
    s32 val = TIMER_GET_COUNT(timer->hw->nowData);
    s32 val2 = TIMER_GET_MODE(timer->hw->nowData);
    timer->unk12 = val;
    timer->unk16 = val2;
    timer->count += val * (val2 - timer->unk16);
    
    u32 mode = TIMER_GET_MODE(timer->hw->nowData);
    if ((arg1 - 1) > TIMER_MAX_COUNTER_VALUE)
        timer->hw->timerData = TIMER_SET_MODE(mode) | TIMER_SET_COUNT(TIMER_MAX_COUNTER_VALUE);
    else
        timer->hw->timerData = TIMER_SET_MODE(mode) | TIMER_SET_COUNT(arg1 - 1);
    
    sceKernelCpuResumeIntr(oldIntr);
    return SCE_ERROR_OK;
}

