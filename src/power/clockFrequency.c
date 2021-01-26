/* Copyright (C) The uOFW team
   See the file COPYING for copying permission.
*/

#include <common_imp.h>
#include <display.h>
#include <ge.h>
#include <interruptman.h>
#include <lowio_ddr.h>
#include <lowio_sysreg.h>
#include <power_kernel.h>
#include <syscon.h>
#include <sysmem_kdebug.h>
#include <sysmem_kernel.h>
#include <sysmem_suspend_kernel.h>
#include <sysmem_sysclib.h>
#include <sysmem_sysevent.h>
#include <threadman_kernel.h>

#include "power_int.h"

/* clock frequency limits */

#define PSP_1000_CLOCK_PLL_FREQUENCY_MAX_WLAN_ACTIVE    222

/* Defines Power service specific lower and upper limits for clock speeds. */

#define POWER_PLL_CLOCK_LIMIT_LOWER         1
#define POWER_PLL_CLOCK_LIMIT_UPPER         333

#define POWER_CPU_CLOCK_LIMIT_LOWER         1
#define POWER_CPU_CLOCK_LIMIT_UPPER         333

#define POWER_BUS_CLOCK_LIMIT_LOWER         24
#define POWER_BUS_CLOCK_LIMIT_UPPER         166

/* Defines the fixed set of PLL clock frequencies supported by the power service. */
#define POWER_PLL_OUT_SELECT_SUPPORTED      (SCE_SYSREG_PLL_OUT_SELECT_37MHz | SCE_SYSREG_PLL_OUT_SELECT_148MHz | \
                                             SCE_SYSREG_PLL_OUT_SELECT_190MHz | SCE_SYSREG_PLL_OUT_SELECT_222MHz | \
                                             SCE_SYSREG_PLL_OUT_SELECT_266MHz | SCE_SYSREG_PLL_OUT_SELECT_333MHz | \
                                             SCE_SYSREG_PLL_OUT_SELECT_19MHz | SCE_SYSREG_PLL_OUT_SELECT_74MHz | \
                                             SCE_SYSREG_PLL_OUT_SELECT_96MHz | SCE_SYSREG_PLL_OUT_SELECT_111MHz | \
                                             SCE_SYSREG_PLL_OUT_SELECT_133MHz | SCE_SYSREG_PLL_OUT_SELECT_166MHz)

typedef struct
{
    u32 mutexId; //0
    void* pSm1Ops; //4
    u32 pllOutSelect; //8
    s32 pllUseMask; //12
    s32 pllClockFrequencyInt; //16
    s32 cpuClockFrequencyInt; //20
    s32 busClockFrequencyInt; //24
    float pllClockFrequencyFloat; //28
    float cpuClockFrequencyFloat; //32
    float busClockFrequencyFloat; //36
    u32 clkcCpuGearNumerator; //40
    u32 clkcCpuGearDenominator; //44
    u32 clkcBusGearNumerator; //48
    u32 clkcBusGearDenominator; //52
    u32 isTachyonMaxVoltage; // 56
    s16 tachyonMaxVoltage; //60
    s16 tachyonDefaultVoltage; // 62
    u32 isDdrMaxVoltage; // 64
    s16 ddrMaxVoltage; // 68
    s16 ddrDefaultVoltage; // 70
    s32 isDdrMaxStrength; // 72
    s16 ddrMaxStrength; // 76
    s16 ddrDefaultStrength; // 78
    s32 geEdramRefreshMode; //80
    s32 oldGeEdramRefreshMode; //84
    u16 unk88;
    s16 scCpuClockLowerLimit; // 90
    s16 scCpuClockUpperLimit; // 92
    s16 scBusClockLowerLimit; // 94
    s16 scBusClockUpperLimit; // 96
    s16 pllClockLowerLimit; // 98
    s16 pllClockUpperLimit; // 100
    u16 unk102;
} ScePowerFrequency; //size: 104

typedef struct
{
    u32 frequency; // 0
    u32 pllUseMaskBit; // 4
} ScePowerPllConfiguration; //size: 8

#define POWER_PLL_CONFIGURATIONS            (sizeof g_pllSettings / sizeof g_pllSettings[0])
const ScePowerPllConfiguration g_pllSettings[] =
{
    {.frequency = 19, .pllUseMaskBit = SCE_SYSREG_PLL_OUT_SELECT_19MHz }, /* 0.057 (~ 1/18) */
    {.frequency = 37, .pllUseMaskBit = SCE_SYSREG_PLL_OUT_SELECT_37MHz }, /* 0.111 (1/9) */
    {.frequency = 74, .pllUseMaskBit = SCE_SYSREG_PLL_OUT_SELECT_74MHz }, /* 0.222 (2/9)*/
    {.frequency = 96, .pllUseMaskBit = SCE_SYSREG_PLL_OUT_SELECT_96MHz }, /* 0.286 (~5/18) */
    {.frequency = 111, .pllUseMaskBit = SCE_SYSREG_PLL_OUT_SELECT_111MHz }, /* 0.333 (3/9) */
    {.frequency = 133, .pllUseMaskBit = SCE_SYSREG_PLL_OUT_SELECT_133MHz }, /* 0.4 (7/18) */
    {.frequency = 148, .pllUseMaskBit = SCE_SYSREG_PLL_OUT_SELECT_148MHz }, /* 0.444 (4/9)  */
    {.frequency = 166, .pllUseMaskBit = SCE_SYSREG_PLL_OUT_SELECT_166MHz }, /* 0.5 (9/18) */
    {.frequency = 190, .pllUseMaskBit = SCE_SYSREG_PLL_OUT_SELECT_190MHz }, /* 0.571 (5/9) */
    {.frequency = 222, .pllUseMaskBit = SCE_SYSREG_PLL_OUT_SELECT_222MHz }, /* 0.667 (6/9) */
    {.frequency = 266, .pllUseMaskBit = SCE_SYSREG_PLL_OUT_SELECT_266MHz }, /* 0.8 (7/9) */
    {.frequency = 333, .pllUseMaskBit = SCE_SYSREG_PLL_OUT_SELECT_333MHz }, /* 1 (9/9) */
}; //0x00006F70 -- size 96

ScePowerFrequency g_PowerFreq; //0x0000C550

//sub_0000353C
/* Initialize the internal power frequency control block. */
s32 _scePowerFreqInit(void)
{
    float pllFrequency;
    float clkcCpuFrequency;
    float clkcBusFrequency;
    s32 tachyonVer;
    u32 fuseConfig;

    memset(&g_PowerFreq, 0, sizeof g_PowerFreq); // 0x0000355C

    g_PowerFreq.pSm1Ops = sceKernelSm1ReferOperations(); // 0x00003564

    // 0x00003580 - 0x00003598
    /* Set power service specific clock frequency limits.  */
    g_PowerFreq.scBusClockLowerLimit = POWER_BUS_CLOCK_LIMIT_LOWER;
    g_PowerFreq.pllClockLowerLimit = POWER_PLL_CLOCK_LIMIT_LOWER;
    g_PowerFreq.pllClockUpperLimit = POWER_PLL_CLOCK_LIMIT_UPPER;
    g_PowerFreq.scCpuClockLowerLimit = POWER_CPU_CLOCK_LIMIT_LOWER;
    g_PowerFreq.scCpuClockUpperLimit = POWER_CPU_CLOCK_LIMIT_UPPER;
    g_PowerFreq.scBusClockUpperLimit = POWER_BUS_CLOCK_LIMIT_UPPER;

    // 0x00003594 - 0x000035A8
    pllFrequency = sceSysregPllGetFrequency();
    g_PowerFreq.pllClockFrequencyFloat = pllFrequency;
    g_PowerFreq.pllClockFrequencyInt = (s32)pllFrequency;

    g_PowerFreq.pllOutSelect = sceSysregPllGetOutSelect(); //0x000035A4 & 0x000035B0

    // 0x000035AC - 0x000035C0
    clkcCpuFrequency = sceClkcGetCpuFrequency();
    g_PowerFreq.pllClockFrequencyFloat = clkcCpuFrequency;
    g_PowerFreq.pllClockFrequencyInt = (s32)clkcCpuFrequency;

    // 0x000035BC - 0x000035DC
    clkcBusFrequency = sceClkcGetBusFrequency();
    g_PowerFreq.busClockFrequencyFloat = clkcBusFrequency;
    g_PowerFreq.busClockFrequencyInt = (s32)clkcBusFrequency;

    g_PowerFreq.pllUseMask = POWER_PLL_OUT_SELECT_SUPPORTED; // 0x000035D0

    if (g_PowerFreq.pllOutSelect == SCE_SYSREG_PLL_OUT_SELECT_333MHz) // 0x000035E0
    {
        g_PowerFreq.isTachyonMaxVoltage = SCE_TRUE; //0x000036B0
        g_PowerFreq.isDdrMaxVoltage = SCE_TRUE;
        g_PowerFreq.isDdrMaxStrength = SCE_TRUE;
    }
    else if (g_PowerFreq.pllOutSelect == SCE_SYSREG_PLL_OUT_SELECT_266MHz) // 0x000035EC
    {
        g_PowerFreq.isTachyonMaxVoltage = SCE_TRUE; // 0x000036A0
        g_PowerFreq.isDdrMaxVoltage = SCE_FALSE;
        g_PowerFreq.isDdrMaxStrength = SCE_FALSE;
    }
    else
    {
        g_PowerFreq.isTachyonMaxVoltage = SCE_FALSE; // 0x000035F8
        g_PowerFreq.isDdrMaxVoltage = SCE_FALSE;
        g_PowerFreq.isDdrMaxStrength = SCE_FALSE;
    }

    // 0x00003608 - 0x00003618
    g_PowerFreq.ddrDefaultStrength = -1;
    g_PowerFreq.ddrMaxVoltage = -1;
    g_PowerFreq.ddrDefaultVoltage = -1;
    g_PowerFreq.ddrMaxStrength = -1;

    tachyonVer = sceSysregGetTachyonVersion(); // 0x00003614
    if (tachyonVer >= 0x00140000) // 0x00003628
    {
        /*
         * PSP Motherboard at least [TA-079 v1]. Every retail PSP model released has a Tachyon SoC IC
         * with at least this version number or higher.
         */

        fuseConfig = (u32)sceSysregGetFuseConfig(); // 0x00003674

        /*
         * (fuseConfig & 0x3800) >> 3) is at most 0x700 -> unk62 has as its minimum 0xB00 - 0x700 = 0x400.
         * g_PowerFreq.tachyonDefaultVoltage is between [0x400, 0xB00] -> [1024 mV, 2816 mV]
         *
         * (~fuseConfig) & 0x700; is at most 0x700.
         * g_PowerFreq.tachyonMaxVoltage is between [0x0, 0x700] -> [0 mV, 1792 mV]
         *
         * TODO: Might expand the comment in the future.
        */
        g_PowerFreq.tachyonDefaultVoltage = 0xB00 - ((fuseConfig & 0x3800) >> 3); // 0x0000367C & 0x00003680 & 0x0000368C
        g_PowerFreq.tachyonMaxVoltage = (~fuseConfig) & 0x700; // 0x00003684 & 0x00003690 & 0x0000369C
    }
    else
    {
        /* Unknown for which PSP hardware this is the case. */

        g_PowerFreq.tachyonDefaultVoltage = 0x400; //0x00003630
        g_PowerFreq.tachyonMaxVoltage = 0; //0x00003634
    }

    scePowerSetGeEdramRefreshMode(1); // 0x0000363C

    g_PowerFreq.mutexId = sceKernelCreateMutex("ScePowerClock", 1, 0, NULL); // 0x00003650

    return SCE_ERROR_OK;
}

//sub_00003FC4
s32 _scePowerFreqEnd(void)
{
    sceKernelDeleteMutex(g_PowerFreq.mutexId); //0x00003FD4
    return SCE_ERROR_OK;
}

//Subroutine scePower_843FBF43 - Address 0x000036BC - Aliases: scePower_driver_53808CBB
s32 scePowerSetCpuClockFrequency(s32 cpuFrequency)
{
    s32 oldK1;
    s32 intrState;
    s32 actCpuFrequency;

    /* The CPU clock frequency cannot be < 1 or higher than the PLL clock frequency. */
    if (cpuFrequency < PSP_CLOCK_CPU_FREQUENCY_MIN || cpuFrequency > g_PowerFreq.pllClockFrequencyInt) //0x000036E8 & 0x000036F8
    {
        return SCE_ERROR_INVALID_VALUE;
    }

    oldK1 = pspShiftK1(); // 0x00003728
    intrState = sceKernelCpuSuspendIntr(); // 0x00003724

    // 0x00003734 - 0x0000373C
    /* Make sure the CPU frequency is inside its limits. */
    actCpuFrequency = pspMax(g_PowerFreq.scCpuClockLowerLimit, pspMin(cpuFrequency, g_PowerFreq.scCpuClockUpperLimit));

    /* Set the CPU clock frequency. */
    sceClkcSetCpuFrequency((float)actCpuFrequency); // 0x00003748

    /* Obtain the actually set bus clock frequency. */
    float cpuFreqFloat = sceClkcGetCpuFrequency(); // 0x00003750
    g_PowerFreq.cpuClockFrequencyFloat = cpuFreqFloat; // 0x0000375C
    g_PowerFreq.cpuClockFrequencyInt = (s32)cpuFreqFloat; // 0x00003768

    sceKernelCpuResumeIntr(intrState); // 0x00003764
    pspSetK1(oldK1); // 0x0000376C

    return SCE_ERROR_OK;
}

//Subroutine scePower_B8D7B3FB - Address 0x00003784 - Aliases: scePower_driver_B71A8B2F
s32 scePowerSetBusClockFrequency(s32 busFrequency)
{
    s32 oldK1;
    s32 intrState;
    s32 status;
    s32 userLevel;
    s32 actBusFrequency;
    s32 halfPllClockFreq;

    // 0x000037B8 - 0x000037C4
    halfPllClockFreq = (g_PowerFreq.pllClockFrequencyInt < 0)
        ? (g_PowerFreq.pllClockFrequencyInt + 1) / 2
        : g_PowerFreq.pllClockFrequencyInt / 2;

    /* The bus clock frequency cannot be < 1 or higher than 1/2 PLL clock frequency. */
    if (busFrequency < PSP_CLOCK_BUS_FREQUENCY_MIN || busFrequency > halfPllClockFreq) // 0x000037B0 & 0x000037CC
    {
        return SCE_ERROR_INVALID_VALUE;
    }

    oldK1 = pspShiftK1(); // 0x000037FC
    intrState = sceKernelCpuSuspendIntr(); // 0x000037F8

    actBusFrequency = busFrequency;

    /*
     * The bus clock frequency is derived from the PLL clock frequency and set to 1/2 of
     * the PLL clock frequency. As such, the specified bus clock frequency is ignored.
     * Going forward, the bus clock frequency can no longer be set directly but can only
     * be set indirectly by setting the PLL frequency.
     */
    userLevel = sceKernelGetUserLevel(); // 0x00003800
    if (userLevel < SCE_USER_LEVEL_VSH) // 0x0000380C
    {
        actBusFrequency = halfPllClockFreq; // 0x00003820
    }

    // 0x00003828 - 0x00003830
    /* Make sure the bus frequency is inside its limits. */
    actBusFrequency = pspMax(g_PowerFreq.scBusClockLowerLimit, pspMin(actBusFrequency, g_PowerFreq.scBusClockUpperLimit));

    // 0x00003838 - 0x00003848
    s32 quarterPllClockFreq = (g_PowerFreq.pllClockFrequencyInt < 0)
        ? (g_PowerFreq.pllClockFrequencyInt + 3) / 4
        : g_PowerFreq.pllClockFrequencyInt / 4;

    actBusFrequency = pspMax(actBusFrequency, quarterPllClockFreq); // 0x0000384C

    /* Set the bus clock frequency. */
    status = sceClkcSetBusFrequency((float)actBusFrequency); // 0x00003854

    /* Obtain the actually set bus clock frequency. */
    float busFrequencyFloat = sceClkcGetBusFrequency(); // 0x0000385C
    g_PowerFreq.busClockFrequencyFloat = busFrequencyFloat; // 0x00003868
    g_PowerFreq.busClockFrequencyInt = (s32)busFrequencyFloat; // 0x00003870

    /* The Graphic Engine's eDRAM operates at the bus clock speed. Refresh it now. */
    scePowerSetGeEdramRefreshMode(scePowerGetGeEdramRefreshMode()); // 0x0000386C & 0x00003874

    sceKernelCpuResumeIntr(intrState); // 0x0000387C
    pspSetK1(oldK1); // 0x00003884

    return status;
}

//Subroutine sub_00003898 - Address 0x00003898 
s32 _scePowerSetClockFrequency(s32 pllFrequency, s32 cpuFrequency, s32 busFrequency)
{
    s32 oldK1;
    u32 newPllOutSelect; // $s4
    s32 actPllFrequency; // $s0
    s32 actCpuFrequency; // $s1
    s32 actBusFrequency; // $s2
    s32 status;

    // 0x00003898 - 0x0000391C

    /*
     * The following clock frequency conditions need to be met:
     *      1) PLL clock frequency can only be between [19, 333]
     *      2) CPU clock frequency can only be between [1, 333]
     *      3) Bus clock frequency can only be between [1, 167]
     *
     * In addition, CPU and bus clock frequencies are derived from the PLL clock frequency.
     * As such, we get the following additional conditions to be met:
     *
     *      4) The CPU clock frequency cannot be higher than the PLL clock frequency.
     *      5) The bus clock frequency cannot be higher than 1/2 PLL clock frequency.
     */
    if (pllFrequency < PSP_CLOCK_PLL_FREQUENCY_MIN || pllFrequency > PSP_CLOCK_PLL_FREQUENCY_MAX
        || cpuFrequency < PSP_CLOCK_CPU_FREQUENCY_MIN || cpuFrequency > PSP_CLOCK_CPU_FREQUENCY_MAX
        || busFrequency < PSP_CLOCK_BUS_FREQUENCY_MIN || busFrequency > (PSP_CLOCK_BUS_FREQUENCY_MAX + 1)
        || pllFrequency < cpuFrequency || pllFrequency < 2 * busFrequency)
    {
        return SCE_ERROR_INVALID_VALUE;
    }

    // 0x00003960 - 0x00003970
    /* Adjust the specified PLL clock frequency so that it is in [PllLowerLimit, PllUpperLimit]. */
    actPllFrequency = pspMax(g_PowerFreq.pllClockLowerLimit, pspMin(pllFrequency, g_PowerFreq.pllClockUpperLimit));

    /*
     * The PLL actually can only operate at a fixed set of clock frequencies. For example,
     * clock frequencies 96, 133, 233, 266, 333 (in MHz). It can be configured through setting the
     * [g_PowerFreq.pllUseMask] how many of these clock frequencies will be used to determine
     * the actual PLL clock frequency given the specified input. This works as follows:
     *
     * Given a clock frequency input f, we scan the PLL clock frequency list (sorted in ascended order)
     * for the first fixed frequency f_fixed, so that f <= f_fixed.
     *
     * Example: Given a list of fixed frequencies {74, 166, 190, 224, 333} and input 108 MHz,
     * we will actually attempt to set the PLL clock frequency to 166 MHz.
     */
    newPllOutSelect = 0xFFFFFFFF; // 0x00003984
    u32 i;
    for (int i = 0; i < POWER_PLL_CONFIGURATIONS; i++)
    {
        /* Filter out all PLL settings which do not match out PLL use mask. */
        if (!((1 << g_pllSettings[i].pllUseMaskBit) & g_PowerFreq.pllUseMask)) // 0x000039A0
        {
            continue;
        }

        if (g_pllSettings[i].frequency >= actPllFrequency) // 0x000039B0
        {
            actPllFrequency = g_pllSettings[i].frequency; // 0x000039B0
            newPllOutSelect = g_pllSettings[i].pllUseMaskBit; // 0x00003E58
            break;
        }
    }

    // 0x000039C0

    /*
     * If no fixed PLL clock frequency was found equal to or above the specified frequency,
     * we default to use the highest possible clock frequency -- PSP_CLOCK_PLL_FREQUENCY_MAX (333 MHz).
    */
    if (newPllOutSelect == 0xFFFFFFFF)
    {
        actPllFrequency = PSP_CLOCK_PLL_FREQUENCY_MAX; // 0x000039C4
        newPllOutSelect = SCE_SYSREG_PLL_OUT_SELECT_333MHz; // 0x00003E50
    }

    // 0x000039D4 - 0x000039DC
    actCpuFrequency = pspMax(g_PowerFreq.scCpuClockLowerLimit, pspMin(cpuFrequency, g_PowerFreq.scCpuClockUpperLimit));
    actCpuFrequency = pspMin(actCpuFrequency, actPllFrequency); //0x000039F0

    // 0x000039EC - 0x000039F4
    actBusFrequency = pspMax(g_PowerFreq.scBusClockLowerLimit, pspMin(busFrequency, g_PowerFreq.scBusClockUpperLimit));

    oldK1 = pspShiftK1(); // 0x00003A10

    status = sceKernelLockMutex(g_PowerFreq.mutexId, 1, NULL); //0x00003A18 
    if (status < SCE_ERROR_OK) // //0x00003A20
    {
        pspSetK1(oldK1);
        return status;
    }

    /* WHen WLAN is active, the specified PLL clock frequency might not be applicable. */
    if (scePowerGetWlanActivity() != SCE_POWER_WLAN_ACTIVITY_OFF
        && (status = scePowerCheckWlanCondition(actPllFrequency)) < SCE_ERROR_OK) // 0x00003A28 - 0x00003A44
    {
        /*
         * The specified PLL clock frequency has been set too high. On SDK versions above 2.00, we return an error.
         * On earlier versions, we "auto-correct" the PLL clock frequency to the highest frequency available when
         * WLAN is active.
         */
        u32 sdkVersion = sceKernelGetCompiledSdkVersion(); //0x00003E08
        if (sdkVersion > 0x0200000F) // 0x00003E18
        {
            sceKernelUnlockMutex(g_PowerFreq.mutexId, 1); //0x00003DF4

            pspSetK1(oldK1); //0x00003DFC
            return status; //0x00003E04
        }

        // 0x00003E1C - 0x00003E2C
        actPllFrequency = PSP_1000_CLOCK_PLL_FREQUENCY_MAX_WLAN_ACTIVE;
        actCpuFrequency = PSP_1000_CLOCK_PLL_FREQUENCY_MAX_WLAN_ACTIVE;
        actBusFrequency = PSP_1000_CLOCK_PLL_FREQUENCY_MAX_WLAN_ACTIVE / 2;

        newPllOutSelect = SCE_SYSREG_PLL_OUT_SELECT_222MHz;
    }

    status = sceKernelGetUserLevel(); // 0x00003A4C
    if (status < SCE_USER_LEVEL_VSH) // 0x00003A58
    {
        /*
         * The bus clock frequency is automatically derived from the PLL clock frequency and set to operate at
         * 1/2 of the PLL clock frequency.
         */
        actBusFrequency = (((u32)actPllFrequency >> 31) + actPllFrequency) >> 1; // 0x00003A60 - 0x00003A68
    }

    scePowerLockForKernel(SCE_KERNEL_POWER_LOCK_DEFAULT); //0x00003A6C

    /* Check if we need to actually change the speed the PLL clock is operating on. */
    if (g_PowerFreq.pllOutSelect != newPllOutSelect) // 0x00003A78
    {
        /*
         * The specified PLL clock frequency "does not fit" in the current PLL clock frequency
         * which is chosen out of a fixed set of frequencies (see comment above). Change the
         * PLL clock frequency to the fixed clock frequency we've determined above.
         */

        SceSysEventPllClockFrequencyChangePayload sysEventPllClockFrequencyChangePayload; // $sp
        sysEventPllClockFrequencyChangePayload.unk0 = 8;
        sysEventPllClockFrequencyChangePayload.newPllClockFrequency = actPllFrequency; // 0x00003AA4

        s32 result = 0; // $sp + 16
        SceSysEventHandler* pSysEventBreakHandler = NULL; // $sp + 20

        status = sceKernelSysEventDispatch(SCE_SPEED_CHANGE_EVENTS, SCE_SYSTEM_PLL_CLOCK_FREQUENCY_CHANGE_EVENT_QUERY, "query", 
            &sysEventPllClockFrequencyChangePayload, &result, SCE_TRUE, &pSysEventBreakHandler); // 0x00003AAC

        if (status < SCE_ERROR_OK) // 0x00003AB4
        {
            sceKernelSysEventDispatch(SCE_SPEED_CHANGE_EVENTS, SCE_SYSTEM_PLL_CLOCK_FREQUENCY_CHANGE_EVENT_CANCELLATION, "cancel", 
                &sysEventPllClockFrequencyChangePayload, NULL, SCE_FALSE, NULL); //0x00003DE0

            scePowerUnlockForKernel(SCE_KERNEL_POWER_LOCK_DEFAULT); // 0x00003DE8
            sceKernelUnlockMutex(g_PowerFreq.mutexId, 1); // 0x00003DF4

            pspSetK1(oldK1);
            return status;
        }

        /* Increase the current DDR memory voltage if the PLL clock is set to its maximum frequency. */
        if (newPllOutSelect == SCE_SYSREG_PLL_OUT_SELECT_333MHz && !g_PowerFreq.isDdrMaxVoltage) // 0x00003AC0 && 0x00003D88
        {
            if (g_PowerFreq.ddrMaxVoltage >= 0 && g_PowerFreq.ddrDefaultVoltage != g_PowerFreq.ddrMaxVoltage)
            {
                sceSysconCtrlVoltage(3, g_PowerFreq.ddrMaxVoltage); //0x00003DA8
            }

            g_PowerFreq.isDdrMaxVoltage = SCE_TRUE; // 0x00003DB4
        }

        /*
         * Increase the current Tachyon voltage if the PLL clock is set to operate at a frequency above its
         * default frequency (222 MHz).
         */
        if ((newPllOutSelect == SCE_SYSREG_PLL_OUT_SELECT_266MHz || newPllOutSelect == SCE_SYSREG_PLL_OUT_SELECT_333MHz)
            && !g_PowerFreq.isTachyonMaxVoltage && g_PowerFreq.tachyonMaxVoltage >= 0) // 0x00003AD0 & 0x00003AEC
        {
            sceSysconCtrlTachyonVoltage(g_PowerFreq.tachyonMaxVoltage); // 0x00003AF4
            g_PowerFreq.isTachyonMaxVoltage = SCE_TRUE; // 0x00003B00
        }

        /* Increase the current DDR memory strength if the PLL clock is set to its maximum frequency. */
        if (newPllOutSelect == SCE_SYSREG_PLL_OUT_SELECT_333MHz && !g_PowerFreq.isDdrMaxStrength) // 0x00003B08 & 0x00003D50
        {
            if (g_PowerFreq.ddrMaxStrength >= 0 && g_PowerFreq.ddrMaxStrength != g_PowerFreq.ddrDefaultStrength) // 0x00003D5C & 0x00003D68
            {
                // TODO: still not sure whether arguments are supplied here.
                // The ASM for sceDdr_driver_0BAAE4C5() in lowio looks like this:
                //
                // li         $a0, 32
                // sll        $a2, $a1, 5
                // j          sceDdr_driver_77CD1FB3
                // li         $a1, 2
                //
                // So apparently it takes two arguments, with the first argument being overwritten
                // locally. Yet...looking at the two calls in power, the second argument makes no
                // sense. In the second call at address 0x00003D28, $a1 would be 0x01000020 as this
                // is the value stored into it to provide the second argument to the preceding 
                // sceKernelSysEventDispatch() call.
                sceDdr_driver_0BAAE4C5(); // 0x00003D70
            }

            g_PowerFreq.isDdrMaxStrength = SCE_TRUE;
        }

        sceKernelSysEventDispatch(SCE_SPEED_CHANGE_EVENTS, SCE_SYSTEM_PLL_CLOCK_FREQUENCY_CHANGE_EVENT_START, "start", 
            &sysEventPllClockFrequencyChangePayload, NULL, SCE_FALSE, NULL); // 0x00003B30

        sceDisplayWaitVblankStart(); // 0x00003B38

        s32 intrState = sceKernelCpuSuspendIntr(); // 0x00003B40 -- $s6

        /*
         * Set the ratio used to derive the CPU clock and bus clock frequencies from the PLL clock frequency.
         * Here, the ratio is 511/511 = 1, so the new clock speeds are __roughly__:
         *   CPU clock frequency = PLL clock frequency * [511/511] = PLL clock frequency
         *   Bus clock frequency = (PLL clock frequency / 2) * [511/511] = 1/2 PLL clock frequency
         */
        sceClkcSetCpuGear(511, 511); // 0x00003B50
        sceClkcSetBusGear(511, 511); // 0x00003B5C

        sceKernelSysEventDispatch(SCE_SPEED_CHANGE_EVENTS, SCE_SYSTEM_PLL_CLOCK_FREQUENCY_CHANGE_EVENT_PHASE_0, "phase0", 
            &sysEventPllClockFrequencyChangePayload, NULL, SCE_FALSE, NULL); // 0x00003B84

        if (g_PowerFreq.pSm1Ops != NULL) //0x00003B90
        {
            void (*sm1Op)(s32, s32) = (void (*)(s32, s32)) * (u32*)(g_PowerFreq.pSm1Ops + 44);
            sm1Op(newPllOutSelect, -1); // 0x00003D3C
        }

        /*
         * Actually change the PLL clock frequency now. Set it to the determined fixed frequency
         * (so >= the specified requested frequency).
         */
        sceDdrChangePllClock(newPllOutSelect); // 0x00003B98
        g_PowerFreq.pllOutSelect = newPllOutSelect; // 0x00003BA4

        /* Get the actual PLL clock frequency. */
        float pllFrequency = sceSysregPllGetFrequency(); // 0x00003BA0
        g_PowerFreq.pllClockFrequencyFloat = pllFrequency; // 0x00003BAC
        g_PowerFreq.pllClockFrequencyInt = (s32)pllFrequency; // 0x00003BA8 & 0x00003BB8

        sceKernelSysEventDispatch(SCE_SPEED_CHANGE_EVENTS, SCE_SYSTEM_PLL_CLOCK_FREQUENCY_CHANGE_EVENT_PHASE_1, "phase1", 
            &sysEventPllClockFrequencyChangePayload, NULL, SCE_FALSE, NULL); // 0x00003BD4

        sceKernelCpuResumeIntr(intrState); // 0x00003BDC

        sceKernelSysEventDispatch(SCE_SPEED_CHANGE_EVENTS, SCE_SYSTEM_PLL_CLOCK_FREQUENCY_CHANGE_EVENT_END, "end", 
            &sysEventPllClockFrequencyChangePayload, NULL, SCE_FALSE, NULL); //0x00003C04

        if (newPllOutSelect != SCE_SYSREG_PLL_OUT_SELECT_333MHz && g_PowerFreq.isDdrMaxStrength) // 0x00003C10 & 0x00003C1C
        {
            if (g_PowerFreq.ddrDefaultStrength >= 0 && g_PowerFreq.ddrMaxStrength != g_PowerFreq.ddrDefaultStrength) // 0x00003D14 & 0x00003D20
            {
                // TODO: still not sure whether arguments are supplied here.
                // The ASM for sceDdr_driver_0BAAE4C5() in lowio looks like this:
                //
                // li         $a0, 32
                // sll        $a2, $a1, 5
                // j          sceDdr_driver_77CD1FB3
                // li         $a1, 2
                //
                // So apparently it takes two arguments, with the first argument being overwritten
                // locally. Yet...looking at the two calls in power, the second argument makes no
                // sense. In the second call at address 0x00003D28, $a1 would be 0x01000020 as this
                // is the value stored into it to provide the second argument to the preceding 
                // sceKernelSysEventDispatch() call.
                sceDdr_driver_0BAAE4C5(); // 0x00003D28
            }

            g_PowerFreq.isDdrMaxStrength = SCE_FALSE; // 0x00003D34
        }

        /*
         * Reduce the current Tachyon voltage if the PLL clock is now operating at its default frequency (222 MHz).
         */
        if (newPllOutSelect != SCE_SYSREG_PLL_OUT_SELECT_266MHz && newPllOutSelect != SCE_SYSREG_PLL_OUT_SELECT_333MHz
            && g_PowerFreq.isTachyonMaxVoltage && g_PowerFreq.tachyonDefaultVoltage >= 0) // 0x00003C28 & 0x00003C3C & 0x00003CFC
        {
            sceSysconCtrlTachyonVoltage(g_PowerFreq.tachyonDefaultVoltage); // 0x00003D04
            g_PowerFreq.isTachyonMaxVoltage = SCE_FALSE; // 0x00003D10
        }

        /* Reduce the current DDR memory voltage if the PLL clock is no longer operating at its maximum frequency. */
        if (newPllOutSelect != SCE_SYSREG_PLL_OUT_SELECT_333MHz && g_PowerFreq.isDdrMaxVoltage) // 0x00003C48 & 0x00003C58
        {
            if (g_PowerFreq.ddrDefaultVoltage >= 0 && g_PowerFreq.ddrMaxVoltage != g_PowerFreq.ddrDefaultVoltage) // 0x00003CFC
            {
                // TODO: The first argument might identify the system component for which voltage is to be changed.
                // Here, '3' would mean the DDR memory component. If confirmed, macros should be defined.
                sceSysconCtrlVoltage(3, g_PowerFreq.ddrDefaultVoltage); // 0x00003CE8
            }

            g_PowerFreq.isDdrMaxVoltage = SCE_FALSE; // 0x00003CF8
        }
    }

    // loc_00003C60

    actPllFrequency = (((u32)actPllFrequency >> 31) + actPllFrequency) >> 1; // 0x00003C60 - 0x00003C68
    if (actPllFrequency == actBusFrequency) // 0x00003C6C
    {
        /*
         * Set the ratio used to derive bus clock frequency from the PLL clock frequency.
         * Here, the ratio is 511/511 = 1, so the new clock speeds are __roughly__:
         *   Bus clock frequency = (PLL clock frequency / 2) * [511/511] = 1/2 PLL clock frequency
         */
        sceClkcSetBusGear(511, 511); // 0x00003CA4

        float busFrequencyFloat = sceClkcGetBusFrequency(); // 0x00003CAC
        g_PowerFreq.busClockFrequencyFloat = busFrequencyFloat; // 0x00003CB8
        g_PowerFreq.busClockFrequencyInt = (s32)busFrequencyFloat; // 0x00003CC0

        /* The Graphic Engine's eDRAM operates at the bus clock speed. Refresh it now. */
        scePowerSetGeEdramRefreshMode(scePowerGetGeEdramRefreshMode()); // 0x00003948
    }

    /* Now set the CPU clock frequency. */
    scePowerSetCpuClockFrequency(actCpuFrequency); // 0x00003C7C

    scePowerUnlockForKernel(SCE_KERNEL_POWER_LOCK_DEFAULT); // 0x00003C84
    sceKernelUnlockMutex(g_PowerFreq.mutexId, 1); // 0x00003C90

    pspSetK1(oldK1); // 0x00003C98
    return SCE_ERROR_OK;
}

//Subroutine scePower_driver_100A77A1 - Address 0x00003E64
s32 scePowerSetGeEdramRefreshMode(s32 geEdramRefreshMode)
{
    s32 refreshParam1; // $a1
    s32 refreshParam2; // $a2
    s32 refreshParam3; // $a3
    s32 intrState;
    s32 status;

    intrState = sceKernelCpuSuspendIntr(); // 0x00003E78

    refreshParam1 = 1;
    refreshParam2 = 8;
    refreshParam3 = 6;

    if (geEdramRefreshMode == 0) // 0x00003E8C
    {
        // 0x00003E94
        if (g_PowerFreq.busClockFrequencyInt < 75) // 0x00003EA0
        {
            // loc_00003F0C

            if (g_PowerFreq.busClockFrequencyInt < 50) // 0x00003EA4 & 0x00003F0C
            {
                // loc_00003F40

                if (g_PowerFreq.busClockFrequencyInt < 25) // 0x00003F10 & 0x00003F40
                {
                    // loc_00003F84

                    refreshParam3 = 1; // 0x00003F90
                    refreshParam2 = 6; // 0x00003F94

                    // 0x00003F84 - 0x00003FAC
                    // TODO: What kind of (compiler) arithmetic optimization am I looking at?
                    s32 tmp = ((g_PowerFreq.busClockFrequencyInt * 8000) - 75); // $t4
                    s32 tmp2 = (tmp * 0xBFA02FE9) >> 32; // $t3

                    refreshParam1 = (tmp + tmp2) / 128 - (tmp >> 31);
                }
                else
                {
                    refreshParam3 = 2; // 0x00003F58
                    refreshParam2 = 3; // 0x00003F5C

                    // 0x00003F48 - 0x00003F78
                    // TODO: What kind of (compiler) arithmetic optimization am I looking at?
                    s32 tmp = ((g_PowerFreq.busClockFrequencyInt * 8000) - 75); // $a1
                    s32 tmp2 = (tmp * 0xBFA02FE9) >> 32; // $t9

                    refreshParam1 = (tmp + tmp2) / 256 - (tmp >> 31);
                }
            }
            else
            {
                refreshParam3 = 3; // 0x00003F1C
                refreshParam2 = 2; // 0x00003F20

                // 0x00003F14 - 0x00003F3C
                s32 tmp = ((g_PowerFreq.busClockFrequencyInt * 8000) - 75);
                refreshParam1 = (tmp < 0)
                    ? (tmp + 511) / 512 /* Presumably something to do with overflow */
                    : tmp / 512;
            }
        }
        else
        {
            refreshParam2 = 1; // 0x00003EB0

            // 0x00003EAC - 0x00003EC8
            s32 tmp = ((g_PowerFreq.busClockFrequencyInt * 8000) - 75);
            refreshParam1 = (tmp < 0)
                ? (tmp + 1023) / 1024 /* Presumably something to do with overflow */
                : tmp / 1024;
        }
    }

    // loc_00003ECC

    status = sceGeEdramSetRefreshParam(geEdramRefreshMode, refreshParam1, refreshParam2, refreshParam3); // 0x00003ECC

    sceKernelCpuResumeIntr(intrState); //0x00003ED8

    if (status < SCE_ERROR_OK)
    {
        return status;
    }

    g_PowerFreq.geEdramRefreshMode = geEdramRefreshMode; // 0x00003EEC

    return SCE_ERROR_OK;
}

//Subroutine scePower_driver_C520F5DC - Address 0x00003FB8
s32 scePowerGetGeEdramRefreshMode(void)
{
    return g_PowerFreq.geEdramRefreshMode;
}

//sub_00003FEC
s32 _scePowerLockPowerFreqMutex(void)
{
    return sceKernelLockMutex(g_PowerFreq.mutexId, 1, NULL); //0x00004000
}

//sub_00004014
s32 _scePowerUnlockPowerFreqMutex(void)
{
    return sceKernelUnlockMutex(g_PowerFreq.mutexId, 1); //0x00004024
}

//sub_00004038
s32 _scePowerFreqRebootPhase(s32 arg0)
{
    // TODO: arg0 seems to be a phase index (like rebootPhase_1 / rebootPhase_2)

    if (arg0 == 1) // 0x00004044
    {
        sceKernelUnlockMutex(g_PowerFreq.mutexId, 1); // 0x00004080
    }
    else if (arg0 == 2) // 0x00004050
    {
        sceKernelLockMutex(g_PowerFreq.mutexId, 1, NULL); // 0x0000406C
    }

    return SCE_ERROR_OK;
}

//sub_00004090
s32 _scePowerFreqSuspend(void)
{
    sceClkcGetCpuGear(&g_PowerFreq.clkcCpuGearNumerator, &g_PowerFreq.clkcCpuGearDenominator); // 0x000040A8
    sceClkcGetBusGear(&g_PowerFreq.clkcBusGearNumerator, &g_PowerFreq.clkcBusGearDenominator); // 0x000040B4

    g_PowerFreq.oldGeEdramRefreshMode = g_PowerFreq.geEdramRefreshMode; // 0x000040CC
    scePowerSetGeEdramRefreshMode(0); // 0x000040C8

    return SCE_ERROR_OK;
}

//sub_000040E4
s32 _scePowerFreqResume(u32 resumeStep)
{
    if (resumeStep == 0) // 0x000040F4
    {
        sceClkcSetCpuGear(g_PowerFreq.clkcCpuGearNumerator, g_PowerFreq.clkcCpuGearDenominator); // 0x00004130
        sceClkcSetBusGear(g_PowerFreq.clkcBusGearNumerator, g_PowerFreq.clkcBusGearDenominator); // 0x0000413C
    }
    else if (resumeStep == 1) // 0x00004100
    {
        scePowerSetGeEdramRefreshMode(g_PowerFreq.oldGeEdramRefreshMode); // 0x0000411C
    }

    return SCE_ERROR_OK;
}

//Subroutine scePower_driver_D7DD9D38 - Address 0x0000414C 
s16 scePowerGetCurrentTachyonVoltage(void)
{
    return (g_PowerFreq.isTachyonMaxVoltage)
        ? g_PowerFreq.tachyonMaxVoltage
        : g_PowerFreq.tachyonDefaultVoltage;
}

//Subroutine scePower_driver_BADA8332 - Address 0x00004170
s32 scePowerGetTachyonVoltage(s16* pMaxVoltage, s16* pDefaultVoltage)
{
    if (pMaxVoltage != NULL)
        *pMaxVoltage = g_PowerFreq.tachyonMaxVoltage;

    if (pDefaultVoltage != NULL)
        *pDefaultVoltage = g_PowerFreq.tachyonDefaultVoltage;

    return SCE_ERROR_OK;
}

//Subroutine scePower_driver_12F8302D - Address 0x00004198
s32 scePowerSetTachyonVoltage(s16 maxVoltage, s16 defaultVoltage)
{
    if (maxVoltage != -1) //0x0000419C
        g_PowerFreq.tachyonMaxVoltage = maxVoltage;

    if (defaultVoltage != -1) //0x000041A8
        g_PowerFreq.tachyonDefaultVoltage = defaultVoltage;

    return SCE_ERROR_OK;
}

//Subroutine scePower_driver_9127E5B2 - Address 0x000041BC
s16 scePowerGetCurrentDdrVoltage(void)
{
    return (g_PowerFreq.isDdrMaxVoltage)
        ? g_PowerFreq.ddrMaxVoltage
        : g_PowerFreq.ddrDefaultVoltage;
}

//Subroutine scePower_driver_75906F9A - Address 0x000041E0 
s32 scePowerGetDdrVoltage(s16* pMaxVoltage, s32* pDefaultVoltage)
{
    if (pMaxVoltage != NULL) //0x000041E0
        *pMaxVoltage = g_PowerFreq.ddrMaxVoltage;

    if (pDefaultVoltage != NULL) //0x000041F0
        *pDefaultVoltage = g_PowerFreq.ddrDefaultVoltage;

    return SCE_ERROR_OK;
}

//Subroutine scePower_driver_018AB235 - Address 0x00004208 
s32 scePowerSetDdrVoltage(s16 maxVoltage, s32 defaultVoltage)
{
    if (maxVoltage != -1) //0x0000420C
        g_PowerFreq.ddrMaxVoltage = maxVoltage;

    if (defaultVoltage != -1) //0x00004218
        g_PowerFreq.ddrDefaultVoltage = defaultVoltage;

    return SCE_ERROR_OK;
}

//Subroutine scePower_driver_0655D7C3 - Address 0x0000422C 
s16 scePowerGetCurrentDdrStrength(void)
{
    return (g_PowerFreq.isDdrMaxStrength)
        ? g_PowerFreq.ddrMaxStrength
        : g_PowerFreq.ddrDefaultStrength;
}

//Subroutine scePower_driver_16F965C9 - Address 0x00004250 
s32 scePowerGetDdrStrength(s16* pMaxStrength, s16* pDefaultStrength)
{
    if (pMaxStrength != NULL) //0x00004250
        *pMaxStrength = g_PowerFreq.ddrMaxStrength;

    if (pDefaultStrength != NULL) //0x00004260
        *pDefaultStrength = g_PowerFreq.ddrDefaultStrength;

    return SCE_ERROR_OK;
}

//Subroutine scePower_driver_D13377F7 - Address 0x00004278
s32 scePowerSetDdrStrength(s16 maxStrength, s16 defaultStrength)
{
    if (maxStrength != -1) // 0x0000427C
        g_PowerFreq.ddrMaxStrength = maxStrength;

    if (defaultStrength != -1) //0x00004288
        g_PowerFreq.ddrDefaultStrength = defaultStrength;

    return SCE_ERROR_OK;
}

//Subroutine scePower_driver_DF904CDE - Address 0x0000429C 
s32 scePowerLimitScCpuClock(s16 lowerLimit, s16 upperLimit)
{
    if (lowerLimit != -1) // 0x000042A0
    {
        g_PowerFreq.scCpuClockLowerLimit = lowerLimit; // 0x000042A8
    }

    if (upperLimit != -1) // 0x000042AC
    {
        g_PowerFreq.scCpuClockUpperLimit = upperLimit; // 0x000042B4
    }

    return SCE_ERROR_OK;
}

//Subroutine scePower_driver_EEFB2ACF - Address 0x000042C0
s32 scePowerLimitScBusClock(s16 lowerLimit, s16 upperLimit)
{
    if (lowerLimit != -1) // 0x000042C4
    {
        g_PowerFreq.scBusClockLowerLimit = lowerLimit; // 0x000042CC
    }

    if (upperLimit != -1) // 0x000042D0
    {
        g_PowerFreq.scBusClockUpperLimit = upperLimit; // 0x000042D8
    }

    return SCE_ERROR_OK;
}

//Subroutine scePower_driver_B7000C75 - Address 0x000042E4
s32 scePowerLimitPllClock(s16 lowerLimit, s16 upperLimit)
{
    if (lowerLimit != -1) // 0x000042E8
    {
        g_PowerFreq.pllClockLowerLimit = lowerLimit; // 0x000042F0
    }

    if (upperLimit != -1) // 0x000042F4
    {
        g_PowerFreq.pllClockUpperLimit = upperLimit; // 0x000042FC
    }

    return SCE_ERROR_OK;
}

//Subroutine scePower_driver_13D7CCE4 - Address 0x00004308
s32 scePowerSetPllUseMask(s32 useMask)
{
    g_PowerFreq.pllUseMask = useMask;
    return SCE_ERROR_OK;
}

// scePower_FEE03A2F
s32 scePowerGetCpuClockFrequency(void) __attribute__((alias("scePowerGetCpuClockFrequencyInt")));

//Subroutine scePower_FDB5BFE9 - Address 0x00004318 - Aliases: scePower_FEE03A2F, scePower_driver_FDB5BFE9
s32 scePowerGetCpuClockFrequencyInt(void)
{
    return g_PowerFreq.cpuClockFrequencyInt;
}

//Subroutine scePower_B1A52C83 - Address 0x00004324 - Aliases: scePower_driver_DC4395E2
float scePowerGetCpuClockFrequencyFloat(void)
{
    return g_PowerFreq.cpuClockFrequencyFloat;
}

// scePower_478FE6F5
s32 scePowerGetBusClockFrequency(void) __attribute__((alias("scePowerGetBusClockFrequencyInt")));

//Subroutine scePower_478FE6F5 - Address 0x00004330 - Aliases: scePower_BD681969, scePower_driver_04711DFB
s32 scePowerGetBusClockFrequencyInt(void)
{
    return g_PowerFreq.busClockFrequencyInt;
}

//Subroutine scePower_9BADB3EB - Address 0x0000433C - Aliases: scePower_driver_1FF8DA3B
float scePowerGetBusClockFrequencyFloat(void)
{
    return g_PowerFreq.busClockFrequencyFloat;
}

//Subroutine scePower_34F9C463 - Address 0x00004348 - Aliases: scePower_driver_67BD889B
s32 scePowerGetPllClockFrequencyInt(void)
{
    return g_PowerFreq.pllClockFrequencyInt;
}

//Subroutine scePower_EA382A27 - Address 0x00004354 - Aliases: scePower_driver_BA8CBCBF
float scePowerGetPllClockFrequencyFloat(void)
{
    return g_PowerFreq.pllClockFrequencyFloat;
}

//Subroutine scePower_737486F2 - Address 0x00004360
s32 scePowerSetClockFrequencyBefore280(s32 pllFrequency, s32 cpuFrequency, s32 busFrequency)
{
    if (g_PowerFreq.pSm1Ops != NULL)
        sceKernelDelayThread(60000000); //0x000043BC

    return _scePowerSetClockFrequency(pllFrequency, cpuFrequency, busFrequency); //0x0000439C
}

//Subroutine scePower_A4E93389 - Address 0x000043CC
s32 scePowerSetClockFrequency280(s32 pllFrequency, s32 cpuFrequency, s32 busFrequency)
{
    return _scePowerSetClockFrequency(pllFrequency, cpuFrequency, busFrequency);
}

//Subroutine scePower_545A7F3C - Address 0x000043E8
s32 scePowerSetClockFrequency300(s32 pllFrequency, s32 cpuFrequency, s32 busFrequency)
{
    return _scePowerSetClockFrequency(pllFrequency, cpuFrequency, busFrequency);
}

//Subroutine scePower_EBD177D6 - Address 0x00004404 - Aliases: scePower_driver_EBD177D6
s32 scePowerSetClockFrequency350(s32 pllFrequency, s32 cpuFrequency, s32 busFrequency)
{
    return _scePowerSetClockFrequency(pllFrequency, cpuFrequency, busFrequency);
}

//Subroutine scePower_469989AD - Address 0x00004420 - Aliases: scePower_driver_469989AD
s32 scePowerSetClockFrequency(s32 pllFrequency, s32 cpuFrequency, s32 busFrequency)
{
    /*
     * Check if the device calling this function is a PSP-100X or a development tool (DTP-T1000) configured
     * to operate like a PSP-100X (by limiting the PLL's WLAN coexistency clock).
     */
    if (sceKernelGetModel() != PSP_1000
        || (sceKernelDipsw(PSP_DIPSW_BIT_PLL_WLAN_COEXISTENCY_CLOCK) == PSP_DIPSW_PLL_WLAN_COEXISTENCY_CLOCK_333MHz)) //0x0000443C & 0x00004444 & 0x0000444C & 0x00004458
    {
        /*
         * If we run on a PSP-2000 device or later (or set the boot parameter of the dev tool accordingly)
         * WLAN can be used without limiting the clock frequencies.
         */
        scePowerSetExclusiveWlan(SCE_POWER_WLAN_EXCLUSIVE_PLL_CLOCK_LIMIT_NONE); //0x00004488
    }

    return _scePowerSetClockFrequency(pllFrequency, cpuFrequency, busFrequency); //0x00004468
}