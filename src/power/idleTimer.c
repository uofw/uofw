/* Copyright (C) The uOFW team
   See the file COPYING for copying permission.
*/

#include <common_imp.h>
#include <interruptman.h>
#include <power_kernel.h>
#include <syscon.h>
#include <sysmem_suspend_kernel.h>
#include <sysmem_sysclib.h>
#include <threadman_kernel.h>

/* Specifies the maximum available idle timer callback slots in the system. */
#define POWER_IDLE_TIMER_NUM_SLOTS      8

/* Enables the idle timer for the given slot. */
#define POWER_IDLE_TIMER_IS_TIMER_ENABLED(t, i)     (((t) >> i) & 0x1)

/* Enables all idle timers. */
#define POWER_IDLE_TIMER_ENABLE_ALL_TIMERS(t)       ((t) = (t) | (0xFFFFFFFF >> (32 - POWER_IDLE_TIMER_NUM_SLOTS)))

typedef struct {
    /* Indicates whether or not the idle timer has just been resetted. */
    u32 isResetted; // 0
    /* Indicates whether or not the idle timer has reached its specified due time. */
    u32 isDueTimeReached; // 4
    /* 
     * The base time of an idle timer. The difference between the base time and the current system time is the
     * ellapsed time which is then compared to the specified dueTime to determine whether or not the idle
     * timer callback needs to be invoked.
     * 
     * The base time is set to the current system time when the idle timer is resetted (in other words, its
     * timer counted is resetted back to 0).
     */
    u64 baseTime; // 8
    /* Specifies the time offset in microseconds when the callback is to be invoked. */
    u64 dueTime; // 16
    /* Specifies idle timer callback attributes. One or more of ::ScePowerIdleTimerCallbackAttr. */
    u32 attr; // 24
    /* The callback function to be called when the idle timer has reached its due time. */
    ScePowerIdleTimerCallback callback; // 28
    /* Contains the [global pointer] value to use when the idle timer callback is invoked. */
    u32 gp; // 32
    /* A custom argument to be passed to the callback function. */
    void *callbackArg; // 36
} ScePowerIdleTimerCb; // size = 40

typedef struct  {
    /*
     * The current system time set in the power service. Used to determine whether an idle timer has reached
     * its specified due time and that its callback needs to be invoked. It is also used to set an idle timer's
     * base time the timer is resetted.
     */
    u64 curSystemTime; // 0
    /* Holds the enabled status of all idle timers. */
    u32 idleTimerStatus; // 8
    /* Unused. Padding? */
    u32 unk12; // 12
    /* The set of idle timer control blocks. One control block for each idle timer. */
    ScePowerIdleTimerCb idleTimerCb[POWER_IDLE_TIMER_NUM_SLOTS]; // 16
} ScePowerIdle; // size = 336

static s32 _scePowerVblankInterrupt(s32 subIntNm, void *arg);
static s32 GetGp(void);

ScePowerIdle g_PowerIdle; // 0x0000C400

//Subroutine scePower_EFD3C963 - Address 0x00002F94 - Aliases: scePower_driver_0EFEE60E
s32 scePowerTick(s32 tickType)
{
    u32 i;
    for (i = 0; i < POWER_IDLE_TIMER_NUM_SLOTS; i++)
    {
        /*
         * Find the idle timers corresponding to the specified tick type. If ::SCE_KERNEL_POWER_TICK_DEFAULT
         * is specified, we try to reset all idle timers.
         * 
         * In the current implementation only the following timers can be resetted individually:
         *         power tick                     slot
         * 
         * SCE_KERNEL_POWER_TICK_SUSPENDONLY        0
         * SCE_KERNEL_POWER_TICK_LCDONLY           1-2
         * 
         * All timer callbacks registered in the remaining slots (3 to 7) can only be resetted by specifying
         * ::SCE_KERNEL_POWER_TICK_DEFAULT as the tick type.
         * 
         */
        if (!((tickType >> i) & 0x1) && tickType != SCE_KERNEL_POWER_TICK_DEFAULT) // 0x00002FB0 & 0x00002FB8
        {
            continue;
        }

        /* Check if we can cancel the timer. */
        if (g_PowerIdle.idleTimerCb[i].attr & SCE_POWER_IDLE_TIMER_CALLBACK_ATTR_CANNOT_RESET) // 0x00002FC8
        {
            continue;
        }

        /* If the timer is already currently resetted, we are done with it. */
        if (g_PowerIdle.idleTimerCb[i].isResetted) // 0x00002FD4
        {
            continue;
        }

        /* Reset the current idle timer. */
        g_PowerIdle.idleTimerCb[i].isResetted = SCE_TRUE; // 0x00002FEC
        g_PowerIdle.idleTimerCb[i].baseTime = g_PowerIdle.curSystemTime; // 0x00002FF0 & 0x00002FF4
    }

    return SCE_ERROR_OK;
}

// Subroutine scePower_EDC13FE5 - Address 0x00003008 - Aliases: scePower_driver_DF336CDE
s32 scePowerGetIdleTimer(s32 slot, u64 *pCurEllapsedTime, u64 *pCurDueTime)
{
    s32 oldK1;

    oldK1 = pspShiftK1(); // 0x00003018

    if (slot < 0 || slot >= POWER_IDLE_TIMER_NUM_SLOTS) // 0x0000301C & 0x00003040
    {
        pspSetK1(oldK1);
        return SCE_ERROR_INVALID_INDEX;
    }

    /* Verify that valid memory addresses were specified. */
    if (!pspK1PtrOk(pCurEllapsedTime) || !pspK1PtrOk(pCurDueTime)) // 0x00003054 & 0x0000305C
    {
        pspSetK1(oldK1);
        return SCE_ERROR_PRIV_REQUIRED;
    }

    u64 curSysTime = sceKernelGetSystemTimeWide(); // 0x00003088
    if (pCurEllapsedTime != NULL)
    {     
        *pCurEllapsedTime = curSysTime - g_PowerIdle.idleTimerCb[slot].baseTime; // 0x000030A8 - 0x000030CC
    }

    if (pCurDueTime != NULL) // 0x000030D0
    {
        *pCurDueTime = curSysTime - g_PowerIdle.idleTimerCb[slot].dueTime; // 0x000030D8 - 0x000030F4
    }

    pspSetK1(oldK1);
    return (u32)curSysTime - (u32)g_PowerIdle.idleTimerCb[slot].baseTime;
}

// Subroutine scePower_driver_1BA2FCAE - Address 0x00003100
s32 scePowerSetIdleCallback(s32 slot, u32 attr, u64 dueTime, ScePowerIdleTimerCallback callback, void *common)
{
    s32 intrState;

    if (slot < 0 || slot >= POWER_IDLE_TIMER_NUM_SLOTS) // 0x00003110 & 0x00003148
    {
        return SCE_ERROR_INVALID_INDEX;
    }

    intrState = sceKernelCpuSuspendIntr(); // 0x00003150

    /* If there already is an idle timer callback registered for this slot, we cannot overwrite it. */
    if (callback != NULL && g_PowerIdle.idleTimerCb[slot].callback != NULL) // 0x00003170 & 0x0000317C
    {
        sceKernelCpuResumeIntr(intrState); // 0x0000320C

        /* An idle timer callback already exists in this slot. */
        return SCE_ERROR_ALREADY;
    }

    /* Register the idle timer callback. */

    g_PowerIdle.idleTimerCb[slot].attr = attr; // 0x0000319C
    g_PowerIdle.idleTimerCb[slot].baseTime = sceKernelGetSystemTimeWide(); // 0x000031AC & 0x000031B0
    g_PowerIdle.idleTimerCb[slot].callback = callback; // 0x000031B8
    g_PowerIdle.idleTimerCb[slot].isDueTimeReached = -1; // 0x000031BC
    g_PowerIdle.idleTimerCb[slot].dueTime = dueTime; // 0x000031C0 & 0x000031C4
    g_PowerIdle.idleTimerCb[slot].gp = (u32)&GetGp; // 0x000031C8 -- Note: Yes, this is the address of the function and not its return value...
    g_PowerIdle.idleTimerCb[slot].callbackArg = common; // 0x000031CC
    g_PowerIdle.idleTimerCb[slot].isResetted = SCE_FALSE; // 0x000031D4

    sceKernelCpuResumeIntr(intrState); // 0x000031D0

    return SCE_ERROR_OK;
}

// 0x00003220
/*
 * Enumerates through all idle timer callback slots and checks which timer callbacks have reached their specified
 * due time. For those timer callbacks, their callback is invoked.
 */
static s32 _scePowerVblankInterrupt(s32 subIntNm, void *arg)
{
    (void)subIntNm;
    (void)arg;

    u8 isAcSupplied;
    s32 oldGp;

    g_PowerIdle.curSystemTime = sceKernelGetSystemTimeWide(); // 0x00003240

    isAcSupplied = sceSysconIsAcSupplied(); // 0x0000325C
    oldGp = pspGetGp(); // 0x00003264

    /* Enumerate through all idle timer callback slots and check which timer callbacks need to be invoked. */
    u32 i;
    for (i = 0; i < POWER_IDLE_TIMER_NUM_SLOTS; i++) // 0x00003274
    {
        /* Check if a idle callback has been registered in the slot. */
        if (g_PowerIdle.idleTimerCb[i].callback == NULL) // 0x00003278
        {
            /* No callback registered. Proceed with the next slot. */
            continue;
        }

        /* Check if the idle timer callback is currently enabled. */
        if (!POWER_IDLE_TIMER_IS_TIMER_ENABLED(g_PowerIdle.idleTimerStatus, i)) // 0x00003290
        {
            /* The idle timer callback has been disabled. Proceed with the next slot. */
            continue;
        }

        /*
         * Check if we need to automatically reset the current idle timer callback when an AC connection
         * is active.
         */
        if (isAcSupplied 
            && !(g_PowerIdle.idleTimerCb[i].attr & SCE_POWER_IDLE_TIMER_CALLBACK_ATTR_NO_AUTO_RESET_ON_AC_CONNECTION)) // 0x000032DC & 0x000032EC
        {
            /* Reset the current idle timer. */
            g_PowerIdle.idleTimerCb[i].isResetted = SCE_TRUE;
            g_PowerIdle.idleTimerCb[i].baseTime = g_PowerIdle.curSystemTime;
        }

        /* 
         * If the idle timer callback has been resetted, remove the reset status now that we again "counting"
         * the time until its specified due time is reached.
        */
        if (g_PowerIdle.idleTimerCb[i].isResetted) // 0x00003304
        {
            g_PowerIdle.idleTimerCb[i].isResetted = SCE_FALSE; // 0x00003314

            /* 
             * Check if the resetted idle timer had previously reached its due time. If it did, we invoke
             * its callback now.
             */
            if (!g_PowerIdle.idleTimerCb[i].isDueTimeReached) // 0x00003310
            {
                continue;
            }

            /* Invoke the callback of the resetted idle timer. */

            g_PowerIdle.idleTimerCb[i].isDueTimeReached = SCE_FALSE; // 0x00003318

            pspSetGp(g_PowerIdle.idleTimerCb[i].gp); // 0x00003324

            /* Invoke the idle timer callback. */
            g_PowerIdle.idleTimerCb[i].callback(i, 0, g_PowerIdle.idleTimerCb[i].callbackArg); // 0x00003334
            continue;
        }

        /*
         * If the timer callback has already reached its due time, we won't invoke its callback and instead
         * proceed with the next idle callback.
         */
        if (g_PowerIdle.idleTimerCb[i].isDueTimeReached == SCE_TRUE) // 0x0000334C
        {
            continue;
        }

        /*
         * Check if enough time has ellapsed so that the idle timer callback has reached/passed its specified
         * due time.
         */
        u64 ellapsedTime = g_PowerIdle.curSystemTime - g_PowerIdle.idleTimerCb[i].baseTime; // 0x00003350 - 0x0000337C & 0x000033B0 - 0x000033C0
        if (ellapsedTime < g_PowerIdle.idleTimerCb[i].dueTime)
        {
            /* The timer callback has not yet reached its due time, proceed with the next idle callback. */
            continue;
        }

        /*
         * The ellapsed time (since the timer was last resetted or registered) has reached/passed the specified
         * due time for the timer callback. We will now invoke the timer callback.
         */

        g_PowerIdle.idleTimerCb[i].isDueTimeReached = SCE_TRUE; // 0x000033C0

        pspSetGp(g_PowerIdle.idleTimerCb[i].gp); // 0x00003324

        /* Invoke the idle timer callback. */
        g_PowerIdle.idleTimerCb[i].callback(i, (u32)ellapsedTime, g_PowerIdle.idleTimerCb[i].callbackArg); // 0x00003334
    }

    pspSetGp(oldGp); // 0x000032AC

    return -1;
}

// sub_000033C4
s32 _scePowerIdleInit(void)
{
    memset(&g_PowerIdle, 0, sizeof g_PowerIdle); // 0x000033E0

    g_PowerIdle.curSystemTime = sceKernelGetSystemTimeWide(); // 0x000033E8 & 0x00003408 & 0x0000340C

    /* Enable all timers by default. */
    POWER_IDLE_TIMER_ENABLE_ALL_TIMERS(g_PowerIdle.idleTimerStatus); // 0x000033F8

    /* Register and enable the VBLANK interrupt for our idle timer manager. */
    sceKernelRegisterSubIntrHandler(SCE_VBLANK_INT, SCE_KERNEL_INTR_VBLANK_SUB_INTR_POWER, _scePowerVblankInterrupt, NULL); // 0x0000340C
    sceKernelEnableSubIntr(SCE_VBLANK_INT, SCE_KERNEL_INTR_VBLANK_SUB_INTR_POWER); // 0x0000341C

    return SCE_ERROR_OK;
}

// sub_00003438
s32 _scePowerIdleEnd(void)
{
    sceKernelReleaseSubIntrHandler(SCE_VBLANK_INT, SCE_KERNEL_INTR_VBLANK_SUB_INTR_POWER); // 0x00003444

    return SCE_ERROR_OK;
}

// Subroutine scePower_7F30B3B1 - Address 0x0000345C - Aliases: scePower_driver_1E3B1FAE
s32 scePowerIdleTimerEnable(s32 slot)
{
    s32 intrState;
    u32 prevIdleTimerStatus;

    if (slot < 0 || slot >= POWER_IDLE_TIMER_NUM_SLOTS) // 0x00003464 & 0x00003474
    {
        return SCE_ERROR_INVALID_INDEX;
    }

    intrState = sceKernelCpuSuspendIntr(); // 0x0000347C

    prevIdleTimerStatus = g_PowerIdle.idleTimerStatus; // 0x0000348C

    /* Enable the specified idle timer. */
    g_PowerIdle.idleTimerStatus |= (1 << slot); // 0x00003490 - 0x0000349C

    sceKernelCpuResumeIntr(intrState); // 0x000034A8

     /* Return the previous enabled status for the specified idle timer. */
    return POWER_IDLE_TIMER_IS_TIMER_ENABLED(prevIdleTimerStatus, slot); // 0x000034A0 & 0x000034A4 & 0x000034B0 & 0x000034BC
}

// Subroutine scePower_972CE941 - Address 0x000034C8 - Aliases: scePower_driver_961A06A5
s32 scePowerIdleTimerDisable(s32 slot)
{
    s32 intrState;
    u32 prevIdleTimerStatus;

    if (slot < 0 || slot >= POWER_IDLE_TIMER_NUM_SLOTS)
    {
        return SCE_ERROR_INVALID_INDEX;
    }

    intrState = sceKernelCpuSuspendIntr(); // 0x000034E8

    prevIdleTimerStatus = g_PowerIdle.idleTimerStatus; // 0x000034F8

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wshift-negative-value"

    /*
     * We disable all registered idle timers in range [0..slot]. This differs with the
     * scePowerIdleTimerEnable() implementation which only enables the specified idle timer.
     */
    g_PowerIdle.idleTimerStatus &= (~0x1 << slot); // 0x000034FC - 0x00003508

#pragma GCC diagnostic pop

    sceKernelCpuResumeIntr(intrState); // 0x00003514

    /* Return the previous enabled status for the specified idle timer. */
    return POWER_IDLE_TIMER_IS_TIMER_ENABLED(prevIdleTimerStatus, slot); // 0x0000350C & 0x00003510 & 0x0000351C & 0x00003528
}

// 0x00003534
static s32 GetGp(void)
{
    return pspGetGp();
}