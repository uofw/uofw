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
    s32 rsrv[60];
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
    SceHwTimer *regadr;
    /* The interrupt number belonging to this timer. */
    s32 intrcode;
    /* The ID of the timer. */
    s32 timid;
    /* Unknown. */
    u32 prev_tmcy;
    /* Unknown. */
    u32 prev_icnt;
    /* The time-up handler for the timer. */
    SceSTimerCb cb;
    /* The count value for the timer. */
    u32 totalcount;
    /* Pointer to memory common between time-up handler and general routines. */
    void *common;
} SceSTimerInfo;

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
    u32 Control;
    /* The numerator of the timer's prescale. */
    u32 N;
    /* The denominator of the timer's prescale. */
    s32 M;
} SceSTimerReg;

typedef struct {
    SceSTimerInfo SYSTMR[4];
    u32 nextid;
} SceSTimerCB;

// 0b20
SceSTimerCB g_timerCB __attribute__((section(".test.data"))) = {
    .SYSTMR = {
        [0] = {
            .regadr = (void *)HWPTR(HW_TIMER_0),
            .intrcode = SCE_SYSTIMER0_INT,
            .timid = -1,
            .prev_tmcy = 0,
            .prev_icnt = 0,
            .cb = NULL,
            .totalcount = 0,
            .common = NULL,
        },
        [1] = {
            .regadr = (void *)HWPTR(HW_TIMER_1),
            .intrcode = SCE_SYSTIMER1_INT,
            .timid = -1,
            .prev_tmcy = 0,
            .prev_icnt = 0,
            .cb = NULL,
            .totalcount = 0,
            .common = NULL,
        },
        [2] = {
            .regadr = (void *)HWPTR(HW_TIMER_2),
            .intrcode = SCE_SYSTIMER2_INT,
            .timid = -1,
            .prev_tmcy = 0,
            .prev_icnt = 0,
            .cb = NULL,
            .totalcount = 0,
            .common = NULL,
        },
        [3] = {
            .regadr = (void *)HWPTR(HW_TIMER_3),
            .intrcode = SCE_SYSTIMER3_INT,
            .timid = -1,
            .prev_tmcy = 0,
            .prev_icnt = 0,
            .cb = NULL,
            .totalcount = 0,
            .common = NULL,
        },
    },
    /* The base value for a timer ID. */
    .nextid = 0x00352341
};

// 0bb0
/* Collects important data for each individual hardware timer. */
SceSTimerReg STimerRegSave[TIMER_NUM_HW_TIMERS] __attribute__((section(".test.bss")));

 s32 systimerhandler(s32 arg0 __attribute__((unused)), SceSTimerInfo *timer, s32 arg2);
 void _sceSTimerStopCount(SceSTimerInfo *timer);
 s32 _sceSTimerGetCount(SceSTimerInfo *timer);
 s32 suspendSTimer(s32 unk __attribute__((unused)), void *param __attribute__((unused)));
 s32 resumeSTimer(s32 unk __attribute__((unused)), void *param __attribute__((unused)));

// 02e8
s32 SysTimerInit(SceSize argSize __attribute__((unused)), const void *argBlock __attribute__((unused)))
{
    s32 oldIntr = sceKernelCpuSuspendIntr();
    
    s32 i;
    for (i = 0; i < TIMER_NUM_HW_TIMERS; i++) {
        g_timerCB.SYSTMR[i].regadr->timerData = TIMER_SET_MODE(TIMER_MODE_UNKNOWN) | TIMER_SET_COUNT(0);
        g_timerCB.SYSTMR[i].regadr->prsclNumerator = -1;
        g_timerCB.SYSTMR[i].regadr->prsclDenominator = -1;
        //(void)g_timerCB.SYSTMR[i].regadr->unk0;
        g_timerCB.SYSTMR[i].cb = NULL;
        g_timerCB.SYSTMR[i].prev_tmcy = 0;
        g_timerCB.SYSTMR[i].prev_icnt = 0;
        g_timerCB.SYSTMR[i].totalcount = 0;
        g_timerCB.SYSTMR[i].common = NULL;
        g_timerCB.SYSTMR[i].timid = -1;
        sceKernelRegisterIntrHandler(g_timerCB.SYSTMR[i].intrcode, 2, systimerhandler, &g_timerCB.SYSTMR[i], NULL);
    }
    sceKernelRegisterSuspendHandler(10, suspendSTimer, NULL);
    sceKernelRegisterResumeHandler(10, resumeSTimer, NULL);
    
    sceKernelCpuResumeIntr(oldIntr);
    return SCE_ERROR_OK;
}

// 03d0
s32 SysTimerEnd(void *arg0 __attribute__((unused)), s32 arg1 __attribute__((unused)), s32 arg2 __attribute__((unused)), s32 arg3 __attribute__((unused)))
{
    s32 oldIntr = sceKernelCpuSuspendIntr();
    
    s32 i;
    for (i = 0; i < TIMER_NUM_HW_TIMERS; i++) {
        _sceSTimerStopCount(&g_timerCB.SYSTMR[i]);
        sceKernelReleaseIntrHandler(g_timerCB.SYSTMR[i].intrcode);
    }
    sceKernelCpuResumeIntr(oldIntr);
    return SCE_ERROR_OK;
}

// 00e0
/* Update a timer's counter register value and call the time-up handler. */
s32 systimerhandler(s32 arg0 __attribute__((unused)), SceSTimerInfo *timer, s32 arg2)
{
    if (timer->cb == NULL)
        return -1;
    
    s32 v1 = TIMER_GET_MODE(timer->regadr->timerData);
    s32 v2 = TIMER_GET_COUNT(timer->regadr->timerData);
    if (timer->prev_tmcy != 0) {
        v1--;
        timer->totalcount += timer->prev_tmcy;
    }
    timer->totalcount += v2 * (v1 - timer->prev_icnt);
    timer->prev_tmcy = 0;
    timer->prev_icnt = 0;
    if (timer->cb(timer->timid, timer->totalcount + _sceSTimerGetCount(timer), timer->common, arg2) != -1) {
        _sceSTimerStopCount(timer);
        return -2;
    }
    return -1;
}

// 02b0
/* Stop the hardware timer counting. */
void _sceSTimerStopCount(SceSTimerInfo *timer)
{
    //timer->regadr->timerData = timer->regadr->data & ~(TIMER_MODE_IN_USE | 0x80000000);
    timer->regadr->timerData = TIMER_SET_MODE(TIMER_GET_MODE(timer->regadr->nowData) & ~(TIMER_MODE_IN_USE | TIMER_MODE_UNKNOWN));
    timer->regadr->timerData |= TIMER_SET_COUNT(TIMER_GET_COUNT(timer->regadr->nowData));
}

// 02b0
/* Get the current value of the hardware timer counter register. */
s32 _sceSTimerGetCount(SceSTimerInfo *timer)
{
    //TODO: timer->regadr->ulNowTime - timer->regadr->ulBaseTime ?
    return (TIMER_GET_COUNT(timer->regadr->nowData) - TIMER_GET_COUNT(timer->regadr->baseTime));
}

// 0864
/* 
 * Suspend all timers, save their states and counter value, and prohibit 
 * the timer interrupts.
 */
 s32 suspendSTimer(s32 unk __attribute__((unused)), void *param __attribute__((unused)))
{
    s32 i;
    for (i = 0; i < TIMER_NUM_HW_TIMERS; i++) {
        STimerRegSave[i].Control = g_timerCB.SYSTMR[i].regadr->timerData;
        STimerRegSave[i].N = g_timerCB.SYSTMR[i].regadr->prsclNumerator;
        STimerRegSave[i].M = g_timerCB.SYSTMR[i].regadr->prsclDenominator;
        sceKernelDisableIntr(g_timerCB.SYSTMR[i].intrcode);
    }
    return SCE_ERROR_OK;
}

// 08dc
/* 
 * Resume all timers, set their previous states and counter value and enable 
 * the timer interrupts.
 */
 s32 resumeSTimer(s32 unk __attribute__((unused)), void *param __attribute__((unused)))
{
    s32 i;
    for (i = 0; i < TIMER_NUM_HW_TIMERS; i++) {
        g_timerCB.SYSTMR[i].regadr->prsclDenominator = STimerRegSave[i].M;
        g_timerCB.SYSTMR[i].regadr->prsclNumerator = STimerRegSave[i].N;
        //(void)g_timerCB.SYSTMR[i].regadr->unk0;
        g_timerCB.SYSTMR[i].regadr->timerData = STimerRegSave[i].Control & ~0x80000000;
        sceKernelEnableIntr(g_timerCB.SYSTMR[i].intrcode);
    }
    return SCE_ERROR_OK;
}

// 0000
s32 sceSTimerAlloc(void)
{
    if (sceKernelIsIntrContext())
        return SCE_ERROR_KERNEL_CANNOT_BE_CALLED_FROM_INTERRUPT;
    
    s32 oldIntr = sceKernelCpuSuspendIntr();
    
    s32 i;
    for (i = 0; i < TIMER_NUM_HW_TIMERS; i++) {
        if (g_timerCB.SYSTMR[i].timid == -1) {
            g_timerCB.SYSTMR[i].cb = NULL;
            //g_timerCB.SYSTMR[i].regadr->timerData = 0x80000000;
            g_timerCB.SYSTMR[i].regadr->timerData = TIMER_SET_MODE(TIMER_MODE_UNKNOWN) | TIMER_SET_COUNT(0);
            g_timerCB.SYSTMR[i].regadr->prsclNumerator = -1;
            g_timerCB.SYSTMR[i].regadr->prsclDenominator = -1;
            //(void)g_timerCB.SYSTMR[i].regadr->unk0;
            g_timerCB.SYSTMR[i].prev_tmcy = 0;
            g_timerCB.SYSTMR[i].prev_icnt = 0;
            g_timerCB.SYSTMR[i].totalcount = 0;
            g_timerCB.SYSTMR[i].common = NULL;
            g_timerCB.SYSTMR[i].timid = ((g_timerCB.nextid << 2) | i) & 0x7FFFFFFF;
            g_timerCB.nextid += 7;
            
            sceKernelCpuResumeIntr(oldIntr);
            return g_timerCB.SYSTMR[i].timid;
        }
    }
    sceKernelCpuResumeIntr(oldIntr);
    return SCE_ERROR_KERNEL_NO_TIMER;
}

// 043c
s32 sceSTimerFree(s32 timerId)
{
    SceSTimerInfo *timer = &g_timerCB.SYSTMR[timerId & 3];
    if (sceKernelIsIntrContext())
        return SCE_ERROR_KERNEL_CANNOT_BE_CALLED_FROM_INTERRUPT;
    
    if (timer->timid != timerId)
        return SCE_ERROR_KERNEL_ILLEGAL_TIMER_ID;
    
    s32 oldIntr = sceKernelCpuSuspendIntr();
    
    _sceSTimerStopCount(timer);
    sceKernelDisableIntr(timer->intrcode);
    //timer->regadr->timerData = 0x80000000;
    timer->regadr->timerData = TIMER_SET_MODE(TIMER_MODE_UNKNOWN) | TIMER_SET_COUNT(0);
    timer->regadr->prsclNumerator = -1;
    timer->regadr->prsclDenominator = -1;
    timer->cb = NULL;
    //(void)timer->regadr->unk0;
    timer->common = NULL;
    timer->timid = -1;
    timer->prev_tmcy = 0;
    timer->prev_icnt = 0;
    timer->totalcount = 0;
    
    sceKernelCpuResumeIntr(oldIntr);
    return SCE_ERROR_OK;
}

// 05d4
s32 sceSTimerStartCount(s32 timerId)
{
    SceSTimerInfo *timer = &g_timerCB.SYSTMR[timerId & 3];
    if (timer->timid != timerId)
        return SCE_ERROR_KERNEL_ILLEGAL_TIMER_ID;
    
    s32 oldIntr = sceKernelCpuSuspendIntr();
    
    if (TIMER_GET_MODE(timer->regadr->nowData) & TIMER_MODE_IN_USE) {
        sceKernelCpuResumeIntr(oldIntr);
        return SCE_ERROR_KERNEL_TIMER_BUSY;
    }
    timer->regadr->timerData = TIMER_SET_MODE(TIMER_GET_MODE(timer->regadr->nowData) | TIMER_MODE_IN_USE);
    timer->regadr->timerData |= TIMER_SET_COUNT(0);
    
    sceKernelCpuResumeIntr(oldIntr);
    return SCE_ERROR_OK;
}

// 0738
s32 sceSTimerGetCount(s32 timerId, s32 *count)
{
    SceSTimerInfo *timer = &g_timerCB.SYSTMR[timerId & 3];
    if (timer->timid != timerId)
        return SCE_ERROR_KERNEL_ILLEGAL_TIMER_ID;
    
    s32 oldIntr = sceKernelCpuSuspendIntr();
    
    *count = _sceSTimerGetCount(timer);
    
    sceKernelCpuResumeIntr(oldIntr);
    return SCE_ERROR_OK;
}

// 06cc
s32 sceSTimerResetCount(s32 timerId)
{
    SceSTimerInfo *timer = &g_timerCB.SYSTMR[timerId & 3];
    if (timer->timid != timerId)
        return SCE_ERROR_KERNEL_ILLEGAL_TIMER_ID;
    
    s32 oldIntr = sceKernelCpuSuspendIntr();
    
    //timer->regadr->timerData |= 0x80000000;
    timer->regadr->timerData |= TIMER_SET_MODE(TIMER_MODE_UNKNOWN);
    timer->totalcount = 0;
    
    sceKernelCpuResumeIntr(oldIntr);
    return SCE_ERROR_OK;
}

// 0664
s32 sceSTimerStopCount(s32 timerId)
{
    SceSTimerInfo *timer = &g_timerCB.SYSTMR[timerId & 3];
    if (timer->timid != timerId)
        return SCE_ERROR_KERNEL_ILLEGAL_TIMER_ID;
    
    s32 oldIntr = sceKernelCpuSuspendIntr();
    
    _sceSTimerStopCount(timer);
    
    sceKernelCpuResumeIntr(oldIntr);
    return SCE_ERROR_OK;
}

// 04f8
s32 sceSTimerSetPrscl(s32 timerId, s32 numerator, s32 denominator)
{
    SceSTimerInfo *timer = &g_timerCB.SYSTMR[timerId & 3];
    if (timer->timid != timerId)
        return SCE_ERROR_KERNEL_ILLEGAL_TIMER_ID;
    
    if (numerator == 0 || denominator == 0)
        return SCE_ERROR_KERNEL_ILLEGAL_PRESCALE;
    
    if ((denominator / numerator) < 12)
        return SCE_ERROR_KERNEL_ILLEGAL_PRESCALE;
    
    s32 oldIntr = sceKernelCpuSuspendIntr();
    
    if (TIMER_GET_MODE(timer->regadr->nowData) & TIMER_MODE_IN_USE) {
        sceKernelCpuResumeIntr(oldIntr);
        return SCE_ERROR_KERNEL_TIMER_BUSY;
    }
    timer->regadr->prsclNumerator = numerator;
    timer->regadr->prsclDenominator = denominator;
    
    sceKernelCpuResumeIntr(oldIntr);
    return SCE_ERROR_OK;
}

// 01b4
s32 sceSTimerSetHandler(s32 timerId, s32 compareValue, SceSTimerCb timeUpHandler, void *common)
{
    SceSTimerInfo *timer = &g_timerCB.SYSTMR[timerId & 3];
    if (timer->timid != timerId)
        return SCE_ERROR_KERNEL_ILLEGAL_TIMER_ID;
    
    s32 oldIntr = sceKernelCpuSuspendIntr();
    
    if ((u32)compareValue > TIMER_MAX_COUNTER_VALUE)
        compareValue = TIMER_MAX_COUNTER_VALUE;
    
    if (timeUpHandler == NULL) {
        timer->cb = NULL;
        timer->totalcount = 0;
        timer->common = NULL;
        _sceSTimerStopCount(timer);
        sceKernelDisableIntr(timer->intrcode);
    } else {
        timer->cb = timeUpHandler;
        timer->common = common;
        //timer->regadr->nowData & ~TIMER_MAX_COUNTER_VALUE
        timer->regadr->timerData = TIMER_SET_MODE(TIMER_GET_MODE(timer->regadr->nowData)) | TIMER_SET_COUNT(0);
        timer->regadr->timerData |= compareValue;
        timer->regadr->timerData = timer->regadr->nowData | TIMER_SET_MODE(TIMER_MODE_UNKNOWN | TIMER_MODE_HANDLER_REGISTERED);
        sceKernelEnableIntr(timer->intrcode);
    }
    sceKernelCpuResumeIntr(oldIntr);
    return SCE_ERROR_OK;
}

// 07b0
s32 sceSTimerSetTMCY(s32 timerId, s32 arg1)
{
    SceSTimerInfo *timer = &g_timerCB.SYSTMR[timerId & 3];
    if (timer->timid != timerId)
        return SCE_ERROR_KERNEL_ILLEGAL_TIMER_ID;
    
    s32 oldIntr = sceKernelCpuSuspendIntr();
    
    s32 val = TIMER_GET_COUNT(timer->regadr->nowData);
    s32 val2 = TIMER_GET_MODE(timer->regadr->nowData);
    u32 prev_icnt = timer->prev_icnt;
    timer->prev_tmcy = val;
    timer->prev_icnt = val2;
    timer->totalcount += val * (val2 - prev_icnt);
    
    u32 mode = TIMER_GET_MODE(timer->regadr->nowData);
    if ((arg1 - 1) > TIMER_MAX_COUNTER_VALUE)
        timer->regadr->timerData = TIMER_SET_MODE(mode) | TIMER_SET_COUNT(TIMER_MAX_COUNTER_VALUE);
    else
        timer->regadr->timerData = TIMER_SET_MODE(mode) | TIMER_SET_COUNT(arg1 - 1);
    
    sceKernelCpuResumeIntr(oldIntr);
    return SCE_ERROR_OK;
}

