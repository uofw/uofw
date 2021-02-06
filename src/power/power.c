/* Copyright (C) The uOFW team
   See the file COPYING for copying permission.
*/

#include <common_imp.h>
#include <idstorage.h>
#include <interruptman.h>
#include <modulemgr_init.h>
#include <power_kernel.h>
#include <syscon.h>
#include <sysmem_kdebug.h>
#include <sysmem_kernel.h>
#include <sysmem_suspend_kernel.h>
#include <sysmem_sysevent.h>
#include <sysmem_sysclib.h>
#include <threadman_kernel.h>

#include "power_int.h"

SCE_MODULE_INFO(
    "scePower_Service", 
    SCE_MODULE_KERNEL | 
    SCE_MODULE_ATTR_CANT_STOP | SCE_MODULE_ATTR_EXCLUSIVE_LOAD | SCE_MODULE_ATTR_EXCLUSIVE_START, 
    1, 13);
SCE_MODULE_BOOTSTART("_scePowerModuleStart");
SCE_MODULE_REBOOT_BEFORE("_scePowerModuleRebootBefore");
SCE_MODULE_REBOOT_PHASE("_scePowerModuleRebootPhase");
SCE_SDK_VERSION(SDK_VERSION);

#define POWER_CALLBACK_TOTAL_SLOTS_KERNEL               (32)
#define POWER_CALLBACK_MAX_SLOT_KERNEL                  (POWER_CALLBACK_TOTAL_SLOTS_KERNEL - 1)
#define POWER_CALLBACK_TOTAL_SLOTS_USER                 (16)
#define POWER_CALLBACK_MAX_SLOT_USER                    (POWER_CALLBACK_TOTAL_SLOTS_USER - 1)

/*
 * The remaining capacity threshold for the PSP battery at which we treat the battery charge state
 * at being low enough that we force a PSP device suspension. In mAh. Given a standard PSP battery
 * with a capacity of 1800 mAh, this threshold is set to 4% of a full charge.
 */
#define BATTERY_FORCE_SUSPEND_CAPACITY_THRESHOLD            72
/* 
 * The remaining capacity threshold for the PSP battery at which we treat the battery as being
 * in the [low battery] state. In mAh. Given a standard PSP battery with a capacity of 1800 mAh,
 * this threshold is set to 12% of a full charge.
 */
#define BATTERY_LOW_CAPACITY_THRESHOLD                      216

/* Defines the initial PLL clock frequency (in MHz) on startup for (game) applications. */
#define PSP_CLOCK_PLL_FREQUENCY_STARTUP_GAME_APP_UPDATER    222
/* Defines the initial CPU clock frequency (in MHz) on startup for (game) applications. */
#define PSP_CLOCK_CPU_FREQUENCY_STARTUP_GAME_APP_UPDATER    222
/* Defines the initial bus clock frequency (in MHz) on startup for (game) applications. */
#define PSP_CLOCK_BUS_FREQUENCY_STARTUP_GAME_APP_UPDATER    111

// TODO: Remove

/** Cancel all PSP Hardware timers. */
#define SCE_KERNEL_POWER_TICK_DEFAULT               (0)     
/** Cancel auto-suspend-related timer. */
#define SCE_KERNEL_POWER_TICK_SUSPENDONLY           (1)
/** Cancel LCD-related timer */
#define SCE_KERNEL_POWER_TICK_LCDONLY               (6)	

/**
 * This structure represents a control block for a registered power callback.
 */
typedef struct  {
    /* The callback ID. */
    SceUID cbid; // 0
    /* 
     * Contains the callback one-off argument (to be passed to the callback function) accumulated across
     * all current callback notifications not yet processed by the system for this power callback.
     * Example for a one-off callback power state arg: SCE_POWER_CALLBACKARG_RESUME_COMP
     */
    s32 delayedCallbacksAccumulatedOneOffArg; // 4
    /*
     * Contains the complete (power state) argument to be passed to the callback function.
     * The (accumulated) one-off callback argument is included as well as the callback argument describing 
     * the general power state (such as whether the POWER switch is currently active or the remaining battery
     * capacity in [%]).
     */
    s32 powerStateCallbackArg; // 8
    /* The callback mode. */
    s32 mode; // 12
} ScePowerCallback;

/**
 * This structure represents the power service's internal control block.
 */
typedef struct {
    ScePowerCallback powerCallback[POWER_CALLBACK_TOTAL_SLOTS_KERNEL]; // 0 - 511
    s32 baryonVersion; // 512
    u32 curPowerStateForCallbackArg; // 516
    u32 callbackArgMask; // 520
    u8 isBatteryLow; // 524
    u8 wlanActivity; // 525
    u8 watchDog; // 526
    u8 isWlanSuppressChargingEnabled; // 527
    u8 backlightMaximumWlanActive; // 528
    /* 
     * The WLAN exclusive PLL clock frequency limit. If WLAN is active, the PLL clock frequency cannot be set
     * to a value which is higher than this clock limit. If WLAN is inactive, this clock limit dictates if the
     * PLL is currently operating at a clock frequency where WLAN can be activated.
     */
    s8 wlanExclusivePllClockLimit; // 529
    u8 ledOffTiming; // 530
    u8 padding[3];
    /* 
     * The initial CPU clock frequency set by the power service when a game, application or updater process
     * has been launched.
     */
    u16 cpuClockInitialFrequencyGameAppUpdater; // 534
    /*
     * The initial bus clock frequency set by the power service when a game, application or updater process
     * has been launched.
     */
    u16 busClockInitialFrequencyGameAppUpdater; // 536
    /*
     * The initial PLL clock frequency set by the power service when a game, application or updater process
     * has been launched.
     */
    u16 pllClockInitialFrequencyGameAppUpdater; // 538
} ScePower; // size = 540

static void _scePowerAcSupplyCallback(s32 enable, void* argp); //sub_0x00000650
static void _scePowerLowBatteryCallback(s32 enable, void* argp); //sub_0x000006C4
static s32 _scePowerSysEventHandler(s32 eventId, char *eventName, void *param, s32 *result); //sub_0x0000071C
static s32 _scePowerInitCallback(void *data, s32 arg, void *opt); //sub_0x0000114C

SceSysEventHandler g_PowerSysEv = 
{
    .size = sizeof(SceSysEventHandler),
    .name = "ScePower",
    .typeMask = SCE_SUSPEND_EVENTS | SCE_RESUME_EVENTS,
    .handler = _scePowerSysEventHandler,
    .gp = 0,
    .busy = SCE_FALSE,
    .next = NULL,
    .reserved = {
        [0] = 0,
        [1] = 0,
        [2] = 0,
        [3] = 0,
        [4] = 0,
        [5] = 0,
        [6] = 0,
        [7] = 0,
        [8] = 0,
    }  
}; // 0x00007040

ScePower g_Power; // 0x00007080

//scePower_driver_9CE06934 - Address 0x00000000
s32 scePowerInit(void)
{
    SceIdStorageLeafBaryon baryonData; // $sp
    SceIdStorageLeafMDdr mDdrData; // $sp + 512
    s32 status;

    u16 lowBatteryCapacity; // $s7
    u16 forceSuspendBatteryCapacity; // $s6

    u32 isUsbChargingSupported; // $fp

    /* Initialize the power switch and clock frequency components of the power service. */  
    _scePowerSwInit(); // 0x00000030
    _scePowerFreqInit(); // 0x00000038
    
    g_Power.baryonVersion = _sceSysconGetBaryonVersion(); // 0x00000040

    /* 
     * Try to read the BARYON specific IdStorage leaf in order to properly initialize the power service and correctly
     * configure the system. For example, we try to obtain settings for
     *  - the initial PLL/CPU/bus clock frequencies to use
     *  - power-service specific clock frequency limits (applied when using Power service APIs to set the clock
     *    frequencies)
     *  - the PLL clock frequency at which WLAN is allowed to operate (depending on the PSP system we are running on)
     *  - whether USB charging is supported (dependent on the PSP system we are running on)
     *  - whether operating WLAN suppresses battery charging (dependent on the PSP system we are running on)
     *  - Voltage values for the TACHYON SoC IC (default & maximum)
     */
    status = sceIdStorageLookup(SCE_ID_STORAGE_LOOKUP_KEY_BARYON, 0, &baryonData, sizeof baryonData); // 0x00000058
    if (status < SCE_ERROR_OK)
    {
        /* 
         * If there was a failure obtaining the requested data from the PSP's IdStorage, we use
         * default values.
         */

        memset(&baryonData, 0, sizeof baryonData); // 0x00000618

        g_Power.wlanExclusivePllClockLimit = SCE_POWER_WLAN_EXCLUSIVE_PLL_CLOCK_LIMIT_222Mhz; // 0x00000624
        g_Power.watchDog = 0; // 0x00000630
        g_Power.isWlanSuppressChargingEnabled = SCE_FALSE; // 0x00000638
        g_Power.backlightMaximumWlanActive = 0; // 0x0000063C
        g_Power.ledOffTiming = SCE_POWER_LED_OFF_TIMING_AUTO; // 0x00000644

        forceSuspendBatteryCapacity = BATTERY_FORCE_SUSPEND_CAPACITY_THRESHOLD; // 0x0000062C
        lowBatteryCapacity = BATTERY_LOW_CAPACITY_THRESHOLD; // 0x00000634

        /* 
         * If the IdStorage is corrupted or the data could not be obtained for other reasons,
         * the force suspend capacity is set to the low battery capacity threshold.
         */
        _scePowerChangeSuspendCap(BATTERY_LOW_CAPACITY_THRESHOLD); // 0x00000640
    }
    else
    {
        /* Store the successfully obtained BARYON ID storage configuration data. */

        g_Power.ledOffTiming = baryonData.ledOffTiming; // 0x0000006C & 0x00000080
        g_Power.watchDog = baryonData.watchDog & 0x7F; // 0x00000088
        g_Power.isWlanSuppressChargingEnabled = baryonData.isWlanSuppressChargingEnabled; // 0x00000070 & 0x0000008C
        g_Power.backlightMaximumWlanActive = baryonData.backlightMaximumWlanActive; // 0x00000074 & 0x00000090

        forceSuspendBatteryCapacity = baryonData.forceSuspendBatteryCapacity; // 0x00000094
        lowBatteryCapacity = baryonData.lowBatteryCapacity; // 0x0000009C

        g_Power.wlanExclusivePllClockLimit = (baryonData.wlanExclusivePllClockLimit >= 0x80) // 0x00000064 & 0x0000007C & 0x00000098
            ? baryonData.wlanExclusivePllClockLimit & 0x7F // 0x00000084 & 0x000000A4
            : 1; // 0x000000A0 & 0x000000A4
    }

    s32 appType = sceKernelApplicationType(); // 0x000000A8
    if (appType == SCE_INIT_APPLICATION_GAME) // 0x000000B4
    {
        /* Initially limit WLAN's PLL coexistence clock frequency to 222MHz when we are a game application. */
        g_Power.wlanExclusivePllClockLimit = SCE_POWER_WLAN_EXCLUSIVE_PLL_CLOCK_LIMIT_222Mhz; // 0x000005FC

        /* 
         * For game applications, the PLL clock frequency can only be set to the following values (in MHz):
         * 199, 222, 266 and 333.
         */
        u32 pllUseMask = SCE_POWER_PLL_USE_MASK_190MHz | SCE_POWER_PLL_USE_MASK_222MHz
            | SCE_POWER_PLL_USE_MASK_266MHz | SCE_POWER_PLL_USE_MASK_333MHz;
        scePowerSetPllUseMask(pllUseMask); // 0x00000600
    }

    /* Determine whether USB charging is supported by the PSP device we are running on. */

    u8 baryonVersionMajor = PSP_SYSCON_BARYON_GET_VERSION_MAJOR(g_Power.baryonVersion);
    u8 baryonVersionMinor = PSP_SYSCON_BARYON_GET_VERSION_MINOR(g_Power.baryonVersion);
    if (baryonVersionMajor == 0x2 
        && baryonVersionMinor >= 0x2 && baryonVersionMinor < 0x6) // 0x000000CC & 0x000005CC & 0x000005D8
    {
        /* Complete PSP 2000 series (02g - TA-085v1 - TA-090v1 (TA-086 excluded as that is 01g)) */
        isUsbChargingSupported = (baryonData.unk53 & 0x2) ? SCE_TRUE : SCE_FALSE; // 0x000005EC
    }
    else if (baryonVersionMajor == 0x2
        && baryonVersionMinor >= 0x6 && baryonVersionMinor < 0x9) // 0x000000E4 & 0x00000598 & 0x000005A8
    {
        /* Partial PSP 3000 series (03g - TA-090v2 - TA-092) */

        isUsbChargingSupported = (baryonData.unk53 & 0x2) ? SCE_TRUE : SCE_FALSE; // 0x000005BC
    }
    else if (baryonVersionMajor == 0x2
        && baryonVersionMinor >= 0xC && baryonVersionMinor < 0xE) // 0x000000FC & 0x0000056C & 0x00000578
    {
        /* Partial PSP 3000 series (04g - TA-093v1 & TA-093v2) */

        isUsbChargingSupported = (baryonData.unk53 & 0x2) ? SCE_TRUE : SCE_FALSE; // 0x0000058C
    }
    else if (baryonVersionMajor == 0x2
        && (baryonVersionMinor == 0xE || baryonVersionMinor == 0xF)) // 0x00000114 & 0x0000053C & 0x00000548
    {
        /* Partial PSP 3000 series (07g & 09g - TA-095v1 & TA-095v2) */

        isUsbChargingSupported = (baryonData.unk53 & 0x2) ? SCE_TRUE : SCE_FALSE; // 0x0000055C
    }
    else if (baryonVersionMajor >= 0x3 && baryonVersionMajor < 0x4) // 0x0000012C
    {
        /* Complete PSP Go N1000 series (05g - TA-091 & TA-094) */

        isUsbChargingSupported = (baryonData.unk53 & 0x4) ? SCE_TRUE : SCE_FALSE; // 0x00000140
    }
    else if (baryonVersionMajor == 0x4) // 0x00000154
    {
        /* Complete PSP Street E1000 series (11g - TA-096 & TA-097) */
        isUsbChargingSupported = (baryonData.unk53 & 0x2) ? SCE_TRUE : SCE_FALSE; // 0x00000528
    }
    else
    {
        /* Complete PSP 1000 series (01g - TA-079v1 - TA-082 & TA-086) */
        isUsbChargingSupported = SCE_FALSE;
    }

    /* Set the TACHYON max voltage & default voltage. */

    s16 tachyonMaxVoltage = -1; // 0x00000160
    s16 tachyonDefaultVoltage = -1; // 0x0000016C

    if (baryonData.tachyonMaxVoltage >= 0x80) // 0x00000168
    {
        tachyonMaxVoltage = (baryonData.tachyonMaxVoltage & 0x7F) << 8; // 0x00000514 & 0x0000051C
    }

    if (baryonData.tachyonDefaultVoltage >= 0x80) // 0x00000178
    {
        tachyonDefaultVoltage = (baryonData.tachyonDefaultVoltage & 0x7F) << 8; // 0x0000017C & 0x00000510
    }
    
    scePowerSetTachyonVoltage(tachyonMaxVoltage, tachyonDefaultVoltage); // 0x00000180

    /* Set power service specific clock frequency limits. */
    
    /* Set power service specific CPU clock frequency limits. */
    s16 cpuClockFreqLowerLimit = baryonData.cpuClockFreqLowerLimit == 0
        ? -1 
        : baryonData.cpuClockFreqLowerLimit; // 0x00000188 & 0x000001A0 & 0x000001B0

    s16 cpuClockFreqUpperLimit = baryonData.cpuClockFreqUpperLimit == 0
        ? -1
        : baryonData.cpuClockFreqUpperLimit; // 0x0000018C & 0x000001A4 

    scePowerLimitScCpuClock(cpuClockFreqLowerLimit, cpuClockFreqUpperLimit); // 0x000001BC

    /* Set power service specific bus clock frequency limits. */
    s16 busClockFreqLowerLimit = baryonData.busClockFreqLowerLimit == 0
        ? -1
        : baryonData.busClockFreqLowerLimit; // 0x00000190 & 0x000001A8 & 0x000001C4

    s16 busClockFreqUpperLimit = baryonData.busClockFreqUpperLimit == 0
        ? -1
        : baryonData.busClockFreqUpperLimit; // 0x00000194 & 0x000001AC & 0x000001CC

    scePowerLimitScBusClock(busClockFreqLowerLimit, busClockFreqUpperLimit); // 0x000001C8

    /* Set power service specific PLL clock frequency limits. */
    s16 pllClockFreqLowerLimit = baryonData.pllClockFreqLowerLimit == 0
        ? -1
        : baryonData.pllClockFreqLowerLimit; // 0x00000198 & 0x000001B8 & 0x000001D0

    s16 pllClockFreqUpperLimit = baryonData.pllClockFreqUpperLimit == 0
        ? -1
        : baryonData.pllClockFreqUpperLimit; // 0x0000019C & 0x000001C0 & 0x000001D8

    scePowerLimitPllClock(pllClockFreqLowerLimit, pllClockFreqUpperLimit); // 0x000001D4

    /* 
     * Obtain the clock frequencies to set on startup (after the Init module finished loading & starting
     * the rest of the kernel).
     */ 

    g_Power.cpuClockInitialFrequencyGameAppUpdater = baryonData.cpuClockInitialFrequencyGameAppUpdater == 0
        ? PSP_CLOCK_CPU_FREQUENCY_STARTUP_GAME_APP_UPDATER
        : baryonData.cpuClockInitialFrequencyGameAppUpdater; // 0x000001F4 & 0x000001F8 & 0x000001FC

    g_Power.busClockInitialFrequencyGameAppUpdater = baryonData.busClockInitialFrequencyGameAppUpdater == 0
        ? PSP_CLOCK_BUS_FREQUENCY_STARTUP_GAME_APP_UPDATER
        : baryonData.busClockInitialFrequencyGameAppUpdater; // 0x00000204 & 0x00000208 & 0x000001EC


    g_Power.pllClockInitialFrequencyGameAppUpdater = baryonData.pllClockInitialFrequencyGameAppUpdater == 0
        ? PSP_CLOCK_PLL_FREQUENCY_STARTUP_GAME_APP_UPDATER
        : baryonData.pllClockInitialFrequencyGameAppUpdater; // 0x00000210 & 0x00000214 & 0x000001F0

    /* Set the WLAN exclusive PLL clock frequency limit. */

    if (pllClockFreqLowerLimit <= 266 && g_Power.pllClockInitialFrequencyGameAppUpdater <= 266
        && (pllClockFreqLowerLimit > 222 || g_Power.pllClockInitialFrequencyGameAppUpdater > 222)) // 0x0000021C & 0x0000022C & 0x000004F0 & 0x000004FC
    {
        g_Power.wlanExclusivePllClockLimit = SCE_POWER_WLAN_EXCLUSIVE_PLL_CLOCK_LIMIT_222Mhz; // 0x000004F4 & 0x00000508
    }
    else
    {
        g_Power.wlanExclusivePllClockLimit = SCE_POWER_WLAN_EXCLUSIVE_PLL_CLOCK_LIMIT_NONE; // 0x00000220 &  0x00000234
    }

    // 0x00000238

    /* Check if boot parameter bit 49 has been set (for the PSP Development Tool). */

    if (sceKernelDipsw(PSP_DIPSW_BIT_GAME_APP_UPDATER_PSP_CLOCK_FREQUENCIES_NO_LIMIT)) // 0x00000238 & 0x00000244
    {
        /* 
         * The bit has been set - we impose no clock frequency limits and allow WLAN to be operated at
         * maximum clock frequencies (independently of the PSP system we are running on).
         */

        g_Power.cpuClockInitialFrequencyGameAppUpdater = PSP_CLOCK_CPU_FREQUENCY_MAX; // 0x000004AC & 0x000004CC
        g_Power.busClockInitialFrequencyGameAppUpdater = PSP_CLOCK_BUS_FREQUENCY_MAX; // 0x000004B4 & 0x000004B8
        g_Power.pllClockInitialFrequencyGameAppUpdater = PSP_CLOCK_PLL_FREQUENCY_MAX; // 0x000004AC & 0x000004C0

        g_Power.wlanExclusivePllClockLimit = SCE_POWER_WLAN_EXCLUSIVE_PLL_CLOCK_LIMIT_NONE; // 0x000004C4

        scePowerLimitScCpuClock(cpuClockFreqLowerLimit, PSP_CLOCK_CPU_FREQUENCY_MAX); // 0x000004C8
        scePowerLimitScCpuClock(busClockFreqLowerLimit, PSP_CLOCK_BUS_FREQUENCY_MAX); // 0x000004D4
        scePowerLimitScCpuClock(pllClockFreqLowerLimit, PSP_CLOCK_PLL_FREQUENCY_MAX); // 0x000004E0
    }

    /*
     * Try to read the DDR memory component's specific IdStorage leaf in order to properly to properly
     * configure the DDR memory compontent. The settings we are interested in are the following:
     *  - Default and maximum voltage of the DDR memory
     *  - Default and maximum strength of the DDR memory
     */
    status = sceIdStorageLookup(SCE_ID_STORAGE_LOOKUP_KEY_MDDR, 0, &mDdrData, sizeof mDdrData); // 0x0000025C
    if (status < SCE_ERROR_OK) // 0x00000270
    {
        /*
         * If there was a failure obtaining the requested data from the PSP's IdStorage, we use
         * default values.
         */
        memset(&mDdrData, 0, sizeof mDdrData); // 0x0000049C
    }

    /* Set DDR memory hardware component voltage and strength values. */

    s16 ddrMaxVoltage = -1; // 0x00000264
    s16 ddrDefaultVoltage = -1; // 0x00000268
    s16 ddrMaxStrength = -1; // 0x0000026C
    s16 ddrDefaultStrength = -1; // 0x00000274

    if (baryonVersionMajor == 0x1) // 0x00000288
    {
        /* Partial PSP-1000 series (TA-082 & TA-086) */

        if (mDdrData.ddrMaxVoltage01g >= 0x80) // 0x00000444
        {
            ddrMaxVoltage = (mDdrData.ddrMaxVoltage01g & 0x7F) << 8; // 0x0000043C & 0x00000448 & 0x00000490
        }

        if (mDdrData.ddrDefaultVoltage01g >= 0x80) // 0x00000454
        {
            ddrDefaultVoltage = (mDdrData.ddrDefaultVoltage01g & 0x7F) << 8; // 0x0000044C & 0x00000458 & 0x00000488
        }

        if (mDdrData.ddrMaxStrength01g >= 0x80) // 0x0000045C & 0x00000464 & 0x0000046C & 0x00000478
        {
            ddrMaxStrength = mDdrData.ddrMaxStrength01g & 0x7F; // 0x00000468
        }

        if (mDdrData.ddrDefaultStrength01g >= 0x80) // 0x00000460 & 0x00000470 & 0x00000474
        {
            ddrDefaultStrength = mDdrData.ddrDefaultStrength01g & 0x7F; // 0x00000480
        }
    }
    else if (baryonVersionMajor == 0x2 || (baryonVersionMajor >= 0x3 && baryonVersionMajor < 0x4)
        || baryonVersionMajor == 0x4) // 0x00000290 & 0x000002A0 & 0x000002AC
    {
        /* PSP 2000 series - PSP E1000 series - all models included. */

        if (mDdrData.ddrMaxVoltage02gAndLater >= 0x80) // 0x000003E0/0x000002A4/0x000002B0 & 0x000003E8
        {
            ddrMaxVoltage = (mDdrData.ddrMaxVoltage02gAndLater & 0x7F) << 8; // 0x000003EC & 0x00000438
        }

        if (mDdrData.ddrDefaultVoltage02gAndLater >= 0x80) // 0x000003F0 & 0x000003F8
        {
            ddrDefaultVoltage = (mDdrData.ddrDefaultVoltage02gAndLater & 0x7F) << 8; // 0x000003FC & 0x00000430
        }

        if (mDdrData.ddrMaxStrength02gAndLater >= 0x80) // 0x00000400 & 0x00000408 & 0x00000414 & 0x00000420
        {
            ddrMaxStrength = mDdrData.ddrMaxStrength02gAndLater & 0x7F; // 0x00000410
        }

        if (mDdrData.ddrDefaultStrength02gAndLater >= 0x80) // 0x00000404 & 0x0000040C & 0x0000041C
        {
            ddrDefaultStrength = mDdrData.ddrDefaultStrength02gAndLater & 0x7F; // 0x00000428
        }
    }

    scePowerSetDdrVoltage(ddrMaxVoltage, ddrDefaultVoltage); // 0x000002B8
    scePowerSetDdrStrength(ddrMaxStrength, ddrDefaultStrength); // 0x000002C8

    /* Set the battery type. */

    u32 batteryType;
    if ((baryonVersionMajor == 0x2 && (baryonVersionMinor == 0x9 || baryonVersionMajor == 0xA))
        || baryonVersionMajor == 0x3 || baryonVersionMajor == 0x4) // 0x000002DC & 0x000002E8 & 0x000002F4 & 0x000002FC
    {
        /* PSP Go + PSP E1000 + unknown PSP hardware (baryonVersion 0x28, 0x29). */
        batteryType = SCE_POWER_BATTERY_TYPE_BATTERY_STATE_MONITORING_NOT_SUPPORTED; // 0x000002EC or 0x000003DC
    }
    else
    {
        /* PSP series PSP-1000, PSP-2000 and PSP-3000. */
        batteryType = SCE_POWER_BATTERY_TYPE_BATTERY_STATE_MONITORING_SUPPORTED; // 0x00000300
    }

    _scePowerBatteryInit(isUsbChargingSupported, batteryType); // 0x00000304

    /* Send the [forceSuspend] and [low battery] capacity threshold values to Syscon. */

    _scePowerBatterySetParam(forceSuspendBatteryCapacity, lowBatteryCapacity); // 0x00000310

    /* Set the low battery state if the battery is low. */

    g_Power.isBatteryLow = sceSysconIsLowBattery(); // 0x00000318
    if (g_Power.isBatteryLow) // 0x00000328
    {
        g_Power.curPowerStateForCallbackArg |= SCE_POWER_CALLBACKARG_LOWBATTERY; // 0x000003C8 - 0x000003D4
    }

    /* Set the power-online state if the PSP system is connected to an AC adapter. */
    if (sceSysconIsAcSupplied()) // 0x00000330
    {
        g_Power.curPowerStateForCallbackArg |= SCE_POWER_CALLBACKARG_POWERONLINE; // 0x00000348
    }

    /* Support all types of power callbacks by default. */
    g_Power.callbackArgMask = 0xFFFFFFFF; //0x00000350

    /* Initialize the Idle timer component of the power service. */
    _scePowerIdleInit(); // 0x0000034C

    /* Register Syscon power state callbacks. */
    sceSysconSetAcSupplyCallback(_scePowerAcSupplyCallback, NULL); // 0x0000035C
    sceSysconSetLowBatteryCallback(_scePowerLowBatteryCallback, NULL); // 0x0000036C

    /* Register a system event handler so that the power service can handle suspend & resume events. */
    sceKernelRegisterSysEventHandler(&g_PowerSysEv); // 0x00000378

    /* 
     * Register an "Init has finished loading and starting the kernel" callback so that we can set the initial
     * clock frequencies for apps and games, for example.
     */
    sceKernelSetInitCallback(_scePowerInitCallback, 2, NULL); // 0x0000038C

    return SCE_ERROR_OK;
}

//0x00000650
static void _scePowerAcSupplyCallback(s32 enable, void *argp)
{
    (void)argp;

    /* Generate a new power callback for the AC supply state. */

    if (enable) // 0x00000668
    {
        /* Power is now supplied from an AC adapter. */
        _scePowerNotifyCallback(0, SCE_POWER_CALLBACKARG_POWERONLINE, 0); // 0x0000067C & 0x0000066C & 0x0000065C
    }
    else
    {
        /* Power is no longer supplied from an AC adapter. */
        _scePowerNotifyCallback(SCE_POWER_CALLBACKARG_POWERONLINE, 0, 0); // 0x00000670 - 0x0000067C
    }

    if (PSP_SYSCON_BARYON_GET_VERSION_MAJOR(g_Power.baryonVersion) == 2)
    {
        /* We are running on either a PSP 2000 gen or a PSP 3000 gen. */

        _scePowerBatteryUpdateAcSupply(enable);
    }

    /* Update power service's power battery control block to reflect the AC power supply change. */
    scePowerBatteryUpdateInfo(); // 0x0000069C

    return;
}

//sub_0x000006C4
static void _scePowerLowBatteryCallback(s32 enable, void *argp)
{
    (void)argp;

    /* We only generate a power callback if the low battery state has changed. */
    if (g_Power.isBatteryLow == (u8)enable) // 0x000006D8
    {
        return;
    }

    /* Low battery state has changed. Generate the appropriate power callback. */
    
    g_Power.isBatteryLow = (u8)enable; //0x000006F0

    if (enable) // 0x000006EC
    {
        _scePowerNotifyCallback(0, SCE_POWER_CALLBACKARG_LOWBATTERY, 0); // 0x00000700 & 0x000006E0 & 0x000006E4
    }
    else
    {
        _scePowerNotifyCallback(0, SCE_POWER_CALLBACKARG_LOWBATTERY, 0); // 0x000006F4 - 0x00000700
    }
    
    /* Update power service's power battery control block to reflect the low battery state change. */
    scePowerBatteryUpdateInfo(); // 0x00000708

    return;
}

//sub_0x0000071C
static s32 _scePowerSysEventHandler(s32 eventId, char *eventName, void *param, s32 *result)
{
    (void)eventName;
    (void)result;

    if (eventId == SCE_SYSTEM_SUSPEND_EVENT_PHASE1_2) // 0x00000730
    {
        /* 
         * Let the system know that the suspend operation should not proceed for now 
         * as the power service is busy handling the suspension request.
         */
        return _scePowerBatteryIsBusy() // 0x0000088C & 0x00000898
            ? SCE_ERROR_BUSY
            : SCE_ERROR_OK;
    }

    // uofw note: The ASM checks "eventId < 0x403" but we can optimize this to "eventId <= 0x401"
    // so we can use macros instead of a magic number
    if (eventId <= SCE_SYSTEM_SUSPEND_EVENT_PHASE1_1) // 0x00000738 & 0x0000073C
    {
        if (eventId == SCE_SYSTEM_SUSPEND_EVENT_PHASE1_0) // 0x00000748
        {
            /* Suspend the battery component of the power service. */
            _scePowerBatterySuspend(); // 0x00000770
        }

        return SCE_ERROR_OK;
    }

    if (eventId == SCE_SYSTEM_RESUME_EVENT_PHASE0_9) // 0x00000784
    {
        SceSysEventResumePowerState *pResumePowerState = ((SceSysEventResumePayload *)param)->pResumePowerState; // 0x000007C8 -- $s1

        u32 isLowBattery;
        s32 baryonVersionMajor = PSP_SYSCON_BARYON_GET_VERSION_MAJOR(g_Power.baryonVersion); // 0x000007AC & 0x000007B0
        if (baryonVersionMajor == 0x0 || baryonVersionMajor == 0x1) // 0x000007B4 - 0x000007C4
        {
            /* PSP 1000 series */

            isLowBattery = pResumePowerState->powerState & SCE_RESUME_POWER_STATE_IS_LOW_BATTERY_01G; // 0x000007C8 - 0x000007D0
        }
        else
        {
            /* PSP 2000 series and newer */

            isLowBattery = pResumePowerState->powerSupplyStatus & SCE_SYSCON_POWER_SUPPLY_STATUS_IS_LOW_BATTERY_02G_AND_LATER; // 0x000007C8 & 0x00000880 - 0x00000888
        }

        /* Update the low battery status for the power callbacks. */

        if (isLowBattery) // 0x000007D4
        {
            g_Power.curPowerStateForCallbackArg |= SCE_POWER_CALLBACKARG_LOWBATTERY; // 0x000007E0
        }
        else
        {
            g_Power.curPowerStateForCallbackArg &= ~SCE_POWER_CALLBACKARG_LOWBATTERY; // 0x00000888 & 0x000007E4
        }

        /* Update the power supply status for the power callbacks. */

        u32 isPowerOnline = pResumePowerState->powerState & SCE_RESUME_POWER_STATE_IS_POWER_ONLINE; // 0x000007E8 - 0x000007EC
        if (isPowerOnline) // 0x000007F0
        {
            g_Power.curPowerStateForCallbackArg |= SCE_POWER_CALLBACKARG_POWERONLINE; // 0x000007FC & 0x00000800
        }
        else
        {
            g_Power.curPowerStateForCallbackArg &= ~SCE_POWER_CALLBACKARG_POWERONLINE; // 0x00000874 & 0x00000800
        }

        /* Update the HOLD switch lock state for the power callbacks. */

        g_Power.curPowerStateForCallbackArg &= ~SCE_POWER_CALLBACKARG_HOLDSW; // 0x0000080C & 0x00000814

        u32 sdkVersion = sceKernelGetCompiledSdkVersion(); //0x00000810
        if (sdkVersion <= 0x06000000
            && !(pResumePowerState->hardwarePeripheralState & SCE_RESUME_HARDWARE_PERIPHERAL_STATE_HOLD_SWITCH_INACTIVE)) // // 0x0000081C - 0x00000830
        {
            g_Power.curPowerStateForCallbackArg |= SCE_POWER_CALLBACKARG_HOLDSW; // 0x0000083C - 0x00000844
        }

        /* Update the POWER switch status for the power callbacks. */
        g_Power.curPowerStateForCallbackArg &= ~SCE_POWER_CALLBACKARG_POWERSW; // 0x00000858 & 0x00000860

        /* 
         * Refresh the power service's internal battery control block with new battery status data 
         * (such as remaining battery lifetime). 
         */
        _scePowerBatteryUpdatePhase0(pResumePowerState, &g_Power.curPowerStateForCallbackArg); // 0x0000085C
    }
    else if (eventId == SCE_SYSTEM_RESUME_EVENT_PHASE1_0) // 0x00000790
    {
        /* Resume the battery component of the power service now. */
        _scePowerBatteryResume(); // 0x00000798
    }

    return SCE_ERROR_OK;
}

// Subroutine scePower_04B7766E - Address 0x000008A8 - Aliases: scePower_driver_766CD857
s32 scePowerRegisterCallback(s32 slot, SceUID cbid)
{
    s32 oldK1;
    u32 idType;
    s32 status;
    s32 blockAttr;
    s32 intrState;
    SceSysmemUIDControlBlock *pBlock;
    
    oldK1 = pspShiftK1(); // 0x000008C4

    /* Verify that a valid slot number was specified. */

    if (slot < SCE_POWER_CALLBACKSLOT_AUTO || slot > POWER_CALLBACK_MAX_SLOT_KERNEL)  // 0x000008D4
    {
        pspSetK1(oldK1);
        return SCE_ERROR_INVALID_INDEX;
    }

    if (pspK1IsUserMode() && slot > POWER_CALLBACK_MAX_SLOT_USER) // 0x000008DC & 0x000008E4
    {
        pspSetK1(oldK1);
        return SCE_ERROR_PRIV_REQUIRED;
    }
    
    /* Make sure the specified cbid is actually a valid callback ID.  */
    idType = sceKernelGetThreadmanIdType(cbid); // 0x000008EC
    if (idType != SCE_KERNEL_TMID_Callback) // 0x000008F8
    {
        pspSetK1(oldK1);
        return SCE_ERROR_INVALID_ID;
    }

    /* Verify that no kernel callback was specified when called from user mode.*/
    
    status = sceKernelGetUIDcontrolBlock(cbid, &pBlock); // 0x00000904
    if (status != SCE_ERROR_OK) // 0x0000090C
    {
        pspSetK1(oldK1);
        return SCE_ERROR_PRIV_REQUIRED;
    }

    blockAttr = (pspK1IsUserMode()) ? pBlock->attribute & 0x2 : 2; // 0x00000914 - 0x00000928
    if (blockAttr == 0) // 0x0000092C
    {
        pspSetK1(oldK1);
        return SCE_ERROR_PRIV_REQUIRED;
    }
    
    intrState = sceKernelCpuSuspendIntr(); // 0x00000934

    /* Check if callback slot auto-searching was specified. */
    
    if (slot == SCE_POWER_CALLBACKSLOT_AUTO) // 0x00000940
    {
        if (!pspK1IsUserMode) // 0x000009CC
        {
            /* Don't allow auto slot searching in kernel mode. */

            sceKernelCpuResumeIntr(intrState);

            pspSetK1(oldK1);
            return SCE_ERROR_NOT_SUPPORTED;
        }

        /* Search for the first available callback slot. */

        s32 i;
        for (i = 0; i < POWER_CALLBACK_TOTAL_SLOTS_USER; i++) 
        {
             if (g_Power.powerCallback[i].cbid < 0) // 0x000009F4
             {
                 /* We have found an available callback slot. Let's use it to register the specified callback. */

                 g_Power.powerCallback[i].cbid = cbid; // 0x00000A14
                 g_Power.powerCallback[i].delayedCallbacksAccumulatedOneOffArg = 0; // 0x00000A20
                 g_Power.powerCallback[i].powerStateCallbackArg = g_Power.curPowerStateForCallbackArg; // 0x00000A2C
                 g_Power.powerCallback[i].mode = SCE_POWER_CALLBACK_MODE_CANNOT_REPORT_BUSY_BEING_PROCESSED_STATUS; // 0x00000A28
                 
                 sceKernelNotifyCallback(cbid, g_Power.curPowerStateForCallbackArg & g_Power.callbackArgMask); // 0x000009B8

                 sceKernelCpuResumeIntr(intrState);

                 pspSetK1(oldK1);
                 return i; /* Return the slot used. */
             }             
        }

        /* No available callback slot was found, return with error. */

       sceKernelCpuResumeIntr(intrState);

       pspSetK1(oldK1);
       return SCE_ERROR_OUT_OF_MEMORY;
    } 

    /* A specific callback slot was specified. Check if the requested slot is available. */
    else if (g_Power.powerCallback[slot].cbid < 0) // 0x00000960
    {
        /* Specified callback slot is available, let's use it. */

        g_Power.powerCallback[slot].cbid = cbid; // 0x00000994
        g_Power.powerCallback[slot].delayedCallbacksAccumulatedOneOffArg = 0; // 0x000009A0
        g_Power.powerCallback[slot].powerStateCallbackArg = g_Power.curPowerStateForCallbackArg; // 0x000009A8
        g_Power.powerCallback[slot].mode = SCE_POWER_CALLBACK_MODE_CANNOT_REPORT_BUSY_BEING_PROCESSED_STATUS; // 0x00000A28
        
        sceKernelNotifyCallback(cbid, g_Power.curPowerStateForCallbackArg & g_Power.callbackArgMask); // 0x000009B8

        status = SCE_ERROR_OK; // 0x0000099C
    }
    else
    {
        /* Requested slot already in use, return with error. */
        status = SCE_ERROR_ALREADY;
    }

    sceKernelCpuResumeIntr(intrState);

    pspSetK1(oldK1);
    return status;
}

// scePower_DB9D28DD
s32 scePowerUnregitserCallback(s32 slot) __attribute__((alias("scePowerUnregisterCallback")));

//Subroutine scePower_DB9D28DD - Address 0x00000A64 - Aliases: scePower_DFA8BAF8, scePower_driver_315B8CB6
s32 scePowerUnregisterCallback(s32 slot)
{
    s32 oldK1;
    
    oldK1 = pspShiftK1(); //0x00000A78

    /* Verify that a valid slot number was specified. */

    if (slot < 0 || slot > POWER_CALLBACK_MAX_SLOT_KERNEL) // 0x00000A74
    {
        pspSetK1(oldK1);
        return SCE_ERROR_INVALID_INDEX;
    }

    if (pspK1IsUserMode() && slot > POWER_CALLBACK_MAX_SLOT_USER) // 0x00000A94 & 0x00000AA0
    {
        pspSetK1(oldK1);
        return SCE_ERROR_PRIV_REQUIRED;
    }
    
    pspSetK1(oldK1);
    
    /* Check if the specified slot is already marked as available. */
    if (g_Power.powerCallback[slot].cbid < 0) // 0x00000AB0
    {
        /* The specified slot is not in use currently. */

        return SCE_ERROR_NOT_FOUND;
    }

    /* The specified slot was in use. Mark it as available again. */
    g_Power.powerCallback[slot].cbid = -1; //0x00000ABC

    return SCE_ERROR_OK;
}

//Subroutine scePower_A9D22232 - Address 0x00000AD8 - Aliases: scePower_driver_29E23416
s32 scePowerSetCallbackMode(s32 slot, s32 mode)
{
    s32 oldK1;
    
    oldK1 = pspShiftK1();

    /* Verify that a valid slot number was specified. */

    if (slot < 0 || slot > POWER_CALLBACK_MAX_SLOT_KERNEL) // 0x00000AE8
    {
        pspSetK1(oldK1);
        return SCE_ERROR_INVALID_INDEX;
    }

    if (pspK1IsUserMode && slot > POWER_CALLBACK_MAX_SLOT_USER) // 0x00000B08 & 0x00000B14
    {
        pspSetK1(oldK1);
        return SCE_ERROR_PRIV_REQUIRED;
    }
    
    pspSetK1(oldK1); // 0x00000B18

    /* Verify that the specified callback slot is actually in use. */

    if (g_Power.powerCallback[slot].cbid < 0) // 0x00000B24
    {
        return SCE_ERROR_NOT_FOUND;
    }

    /* Specified slot is in use -> set the new mode for the registered callback. */
    
    g_Power.powerCallback[slot].mode = mode; //0x00000B2C

    return SCE_ERROR_OK;
}

//Subroutine scePower_BAFA3DF0 - Address 0x00000B48 - Aliases: scePower_driver_17EEA285
s32 scePowerGetCallbackMode(s32 slot, s32 *pMode)
{
    s32 oldK1;
    
    oldK1 = pspShiftK1();

    /* Verify that a valid memory address to receive the mode was specified. */
    
    if (!pspK1PtrOk(pMode)) // 0x00000B54
    {
        pspSetK1(oldK1);
        return SCE_ERROR_PRIV_REQUIRED;
    }

    /* Verify that a valid slot number was specified. */

    if (slot < 0 || slot > POWER_CALLBACK_MAX_SLOT_KERNEL) // 0x00000B60
    {
        pspSetK1(oldK1);
        return SCE_ERROR_INVALID_INDEX;
    }

    if (pspK1IsUserMode && slot > POWER_CALLBACK_MAX_SLOT_USER) // 0x00000B68 & 0x00000B7C
    {
        pspSetK1(oldK1);
        return SCE_ERROR_PRIV_REQUIRED;
    }
    
    pspSetK1(oldK1); // 0x00000B78

    /* Verify that the specified callback slot is actually in use. */

    if (g_Power.powerCallback[slot].cbid < 0) // 0x00000B9C
    {
        return SCE_ERROR_NOT_FOUND;
    }

    /* Specified slot is in use -> get the current mode of the registered callback. */
    
    if (pMode != NULL) // 0x00000BA4
    {
        *pMode = g_Power.powerCallback[slot].mode; // 0x00000BB0
    }
    
    return SCE_ERROR_OK;
}

//sub_00000BE0
/* Generate a power callback notification. */
void _scePowerNotifyCallback(s32 clearPowerState, s32 setPowerState, s32 cbOneOffPowerState)
{
    s32 intrState;
    s32 notifyCount;
    
    intrState = sceKernelCpuSuspendIntr(); // 0x00000C0C
    
    // uofw note: Return value not used. 
    sceKernelGetCompiledSdkVersion(); // 0x00000C30

    /* Update power service's global power callback argument holder. */

    g_Power.curPowerStateForCallbackArg = (g_Power.curPowerStateForCallbackArg & ~clearPowerState) 
        | setPowerState; // 0x00000C24 & 0x00000C2C & 0x00000C34

    /* 
     * Iterate through all callback slots and generate a callbacl notification for each registered
     * power callback.
     */
    
    u32 i;
    for (i = 0; i < POWER_CALLBACK_TOTAL_SLOTS_KERNEL; i++)
    {
        /* Skip any callback slot which does not currently contain a registered callback. */
        if (g_Power.powerCallback[i].cbid < 0) // 0x00000C4C
        {
            continue;
        }

        /* Obtain the number of times a callback notification has been delayed for this callback. */
         
         notifyCount = sceKernelGetCallbackCount(g_Power.powerCallback[i].cbid); // 0x00000C54
         if (notifyCount < 0) // 0x00000C5C
         {
             /* 
              * If there was an error obtaining the notification count, we skip this particular registered
              * callback.
              */
             continue;
         }

         /* 
          * If there is currently no delayed notification for this registered callback in the system, we reset 
          * the accumulated one-off callback argument for thsi callback.
          */       
         if (notifyCount == 0) // 0x00000C64
         {
             g_Power.powerCallback[i].delayedCallbacksAccumulatedOneOffArg = 0;
         }
         
         /* 
          * Accumulate the callback one-off argument. This way, if there is a delay generating a callback
          * notification we make sure that the callback notification which _is_ finally being generated
          * contains the sum of all delayed one-off arguments for this specific callback and no
          * one-offargument is actually lost due its original callback notification being delayed.
          */
         g_Power.powerCallback[i].delayedCallbacksAccumulatedOneOffArg |= cbOneOffPowerState; // 0x00000C78

         /* Generate a callback notification with the specified callback arguments. */

         g_Power.powerCallback[i].powerStateCallbackArg = g_Power.powerCallback[i].delayedCallbacksAccumulatedOneOffArg 
             | g_Power.curPowerStateForCallbackArg; // 0x00000C7C

         sceKernelNotifyCallback(g_Power.powerCallback[i].cbid, 
                                 g_Power.powerCallback[i].powerStateCallbackArg & g_Power.callbackArgMask); // 0x00000C84
    }

    sceKernelCpuResumeIntr(intrState); //0x00000C94
}

//sub_00000CC4
/* 
 * Determines whether the system is currently busy generating a callback notification for a registered power
 * callback which callback argument contains the specified [cbArgFlag] value. Only power callbacks with their
 * callback mode set to a value other than ::SCE_POWER_CALLBACK_MODE_CANNOT_REPORT_BUSY_BEING_PROCESSED_STATUS
 * will be considered.
 */
s32 _scePowerIsCallbackBusy(s32 cbArgFlag, SceUID *pCbid)
{
    s32 intrState;
    s32 notifyCount;
    
    intrState = sceKernelCpuSuspendIntr(); // 0x00000CEC

    /* 
     * Iterate through all callback slots to find a callback for which the generation of a callback notification
     * containing the specified callback argument is currently delayed in the system.
     */
    
    u32 i;
    for (i = 0; i < POWER_CALLBACK_TOTAL_SLOTS_KERNEL; i++) 
    {
        /* Check if current callback slot contains a registered power callback. */
        if (g_Power.powerCallback[i].cbid < 0)
        {
            /* Specific callback slot is empty -> proceed with next callback slot. */
            continue;
        }
        
        /*
         * Check if the callback argument sent to the registered callback contains the specified value
         * and if the callback mode has been set to report a system delay in generating a callback
         * notification.
         */

        if (g_Power.powerCallback[i].powerStateCallbackArg & cbArgFlag
            && g_Power.powerCallback[i].mode != SCE_POWER_CALLBACK_MODE_CANNOT_REPORT_BUSY_BEING_PROCESSED_STATUS) // 0x00000D14 & 0x00000D58
        {
            /* Check if there currently is a delay in the system generating notifications for this callback. */
            notifyCount = sceKernelGetCallbackCount(g_Power.powerCallback[i].cbid); // 0x00000D60
            if (notifyCount <= 0) // 0x00000D68
            {
                /* 
                 * No delayed notification for this callback currently in the system. Proceed with the next
                 * callabck slot.
                 */
                continue;
            }
             
            /* 
             * Callback notification has been delayed at least once with the callback function
             * not being called yet. Return the ID of the callback busy being processed by the system.
             */
            if (pCbid != NULL)
            {
                *pCbid = g_Power.powerCallback[i].cbid; // 0x00000D74
            }
             
            sceKernelCpuResumeIntr(intrState); // 0x00000D78
            return SCE_TRUE; // 0x00000D84
        }
    }

    /* No delayed notification found for any registered callback. */

    sceKernelCpuResumeIntr(intrState); // 0x00000D28
    return SCE_FALSE; // 0x00000D30
}

//Subroutine scePower_driver_2638EF48 - Address 0x00000D88
s32 scePowerWlanActivate(void)
{
    s32 pllFreq;
    
    _scePowerLockPowerFreqMutex(); // 0x00000D94
    
    pllFreq = scePowerGetPllClockFrequencyInt(); // 0x00000D9C

    /* 
     * uofw note: No longer needed since the CPU clock and the bus clock are now derived from the PLL clock. 
     * Sony appears to have kept them in the source though (ignoring return values) so we do so as well.
     */
    scePowerGetCpuClockFrequencyInt(); // 0x00000DA4
    scePowerGetBusClockFrequencyInt(); // 0x00000DAC
   
    /* Check if the PSP currenty runs at a clock frequency where WLAN cannot be used.  */
    if ((g_Power.wlanExclusivePllClockLimit == SCE_POWER_WLAN_EXCLUSIVE_PLL_CLOCK_LIMIT_222Mhz && pllFreq > 222) 
        || (g_Power.wlanExclusivePllClockLimit == SCE_POWER_WLAN_EXCLUSIVE_PLL_CLOCK_LIMIT_266Mhz && pllFreq > 266)) //0x00000DBC - 0x00000DEC
    {
        /* 
         * The PSP is currently operating at a clock frequency where WLAN cannot be activated. In this case, 
         * the clock frequency needs to be reduced first.
         */
        _scePowerUnlockPowerFreqMutex();
        return SCE_POWER_ERROR_BAD_PRECONDITION;
    }

    g_Power.wlanActivity = SCE_POWER_WLAN_ACTIVITY_ON; // 0x00000E1C

    _scePowerUnlockPowerFreqMutex(); //0x00000E18
    
    /* Suppress battery charging while WLAN is on. */
    if (g_Power.isWlanSuppressChargingEnabled) //0x00000E24
        scePowerBatteryForbidCharging(); //0x00000E34
    
    return SCE_ERROR_OK;
}

//Subroutine scePower_driver_8C6BEFD9 - Address 0x000010EC
s32 scePowerWlanDeactivate(void)
{
    g_Power.wlanActivity = SCE_POWER_WLAN_ACTIVITY_OFF; // 0x00001104

    /* Allow battery charging again now that WLAN is turned off. */
    if (g_Power.isWlanSuppressChargingEnabled)
        scePowerBatteryPermitCharging(); // 0x00001118

    return SCE_ERROR_OK;
}

//Subroutine scePower_A85880D0 - Address 0x00001044 - Aliases: scePower_driver_693F6CF0
s32 scePowerCheckWlanCoexistenceClock(void)
{
    /*
     * Determine the maximum allowed clock frequencies when WLAN is active based on the hardware
     * we are running on (i.e. PSP-100X only has limited clock speed when WLAN is active).
     */

     // 0x0000104C - 0x0000107C
    return (sceKernelGetModel() == PSP_1000)
        ? (sceKernelDipsw(PSP_DIPSW_BIT_PLL_WLAN_COEXISTENCY_CLOCK) != PSP_DIPSW_PLL_WLAN_COEXISTENCY_CLOCK_333MHz)
            ? SCE_POWER_WLAN_COEXISTENCE_CLOCK_222MHz /* Device runs as a PSP 1000 */
            : SCE_POWER_WLAN_COEXISTENCE_CLOCK_333MHz /* Device runs as a PSP 2000+ (set in development tool) */
        : SCE_POWER_WLAN_COEXISTENCE_CLOCK_333MHz; /* PSP-2000 and later support 333 MHz clock frequency with WLAN. */
}

//Subroutine scePower_driver_114B75AB - Address 0x0000108C
s32 scePowerSetExclusiveWlan(u8 pllClockLimit)
{
    g_Power.wlanExclusivePllClockLimit = pllClockLimit;
    return SCE_ERROR_OK;
}

//Subroutine scePower_driver_E52B4362 - Address 0x0000109C
s32 scePowerCheckWlanCondition(s32 pllFrequency)
{
    if ((g_Power.wlanExclusivePllClockLimit == SCE_POWER_WLAN_EXCLUSIVE_PLL_CLOCK_LIMIT_222Mhz && pllFrequency > 222)
        || (g_Power.wlanExclusivePllClockLimit == SCE_POWER_WLAN_EXCLUSIVE_PLL_CLOCK_LIMIT_266Mhz && pllFrequency > 266)) // 0x000010A0 - 0x000010D4
    {
        return SCE_POWER_ERROR_BAD_PRECONDITION;
    }

    return SCE_ERROR_OK;
}

//Subroutine scePower_2B51FE2F - Address 0x00001134 - Aliases: scePower_driver_CE2032CD
u8 scePowerGetWlanActivity(void)
{
    return g_Power.wlanActivity;
}

//Subroutine scePower_442BFBAC - Address 0x00000E44 - Aliases: scePower_driver_2509FF3B
s32 scePowerGetBacklightMaximum(void)
{
    s32 backlightMax;

    /* Get the maximum display backlight level currently available. */
    
    backlightMax = (scePowerIsPowerOnline() == SCE_TRUE) 
        ? SCE_POWER_BACKLIGHT_LEVEL_MAXIMUM_POWER_ONLINE
        : SCE_POWER_BACKLIGHT_LEVEL_MAXIMUM_POWER_OFFLINE; // 0x00000E4C & 0x00000E60 - 0x00000E68

    /* 
     * There might be a separate maximum backlight level specified for the display while WLAN is active.
     * Check if that is the case and if it needs to be applied (if WLAN is currently active).
     */
    if (g_Power.backlightMaximumWlanActive != 0 && 
        g_Power.wlanActivity != SCE_POWER_WLAN_ACTIVITY_OFF) // 0x00000E6C & 0x00000E78
    {
        backlightMax = pspMin(backlightMax, g_Power.backlightMaximumWlanActive); // 0x00000E70
    }

    return backlightMax;
}

//Subroutine module_start - Address 0x00000E8C
s32 _scePowerModuleStart(SceSize argSize, const void *argBlock)
{
    (void)argSize;
    (void)argBlock;
    
    memset(&g_Power, 0, sizeof g_Power); // 0x00000EA8
    
    //0x00000EB0 - 0x00000EC4
    u32 i;
    for (i = 0; i < POWER_CALLBACK_TOTAL_SLOTS_KERNEL; i++)
    {
        g_Power.powerCallback[i].cbid = -1; // 0x00000EBC
    }
         
    scePowerInit(); // 0x00000EC8

    return SCE_ERROR_OK;
}

//Subroutine syslib_ADF12745 - Address 0x00000EE4
s32 _scePowerModuleRebootPhase(s32 arg1, void *arg2, s32 arg3, s32 arg4)
{
    (void)arg2;
    (void)arg3;
    (void)arg4;

    _scePowerFreqRebootPhase(arg1); // 0x00000EF0
    
    if (arg1 == 1) // 0x00000F04
    {
        _scePowerSetClockFrequency(
            PSP_CLOCK_PLL_FREQUENCY_MAX, 
            PSP_CLOCK_CPU_FREQUENCY_MAX, 
            PSP_CLOCK_BUS_FREQUENCY_MAX); // 0x00000F20
    }

    return SCE_ERROR_OK;
}

//Subroutine module_reboot_before - Address 0x00000F30
s32 _scePowerModuleRebootBefore(void *arg0, s32 arg1, s32 arg2, s32 arg3)
{
    s32 *pBuf;

    pBuf = *(s32 *)(arg0 + 44); // 0x00000F3C
    if (pBuf != NULL) // 0x00000F40
    {
        pBuf[5] = scePowerGetBatteryType(); // 0x00000FA0 & 0x00000FA8

        scePowerGetTachyonVoltage((s16 *)&pBuf[6], (s16 *)&pBuf[7]); // 0x00000FB0
        scePowerGetDdrStrength((s16 *)&pBuf[8], (s16 *)&pBuf[9]); // 0x00000FBC
    }

    // uofw note: Below is basically a scePowerEnd() call.

    sceKernelUnregisterSysEventHandler(&g_PowerSysEv); // 0x00000F4C

    sceSysconSetAcSupplyCallback(NULL, NULL); // 0x00000F58
    sceSysconSetLowBatteryCallback(NULL, NULL); // 0x00000F64

    _scePowerIdleEnd(); // 0x00000F6C
    _scePowerFreqEnd(); // 0x00000F74
    _scePowerBatteryEnd(); // 0x00000F7C
    _scePowerSwEnd(); // 0x00000F84 

    return SCE_ERROR_OK;
}

//Subroutine scePower_driver_AD5BB433 - Address 0x00000FCC
s32 scePowerEnd(void)
{
    sceKernelUnregisterSysEventHandler((SceSysEventHandler *)&g_PowerSysEv); // 0x00000FD8

    sceSysconSetAcSupplyCallback(NULL, NULL); // 0x00000FE4
    sceSysconSetLowBatteryCallback(NULL, NULL); // 0x00000FF0
    
    _scePowerIdleEnd(); // 0x00000FF8
    _scePowerFreqEnd(); // 0x00001000
    _scePowerBatteryEnd(); // 0x00001008
    _scePowerSwEnd(); // 0x00001010
    
    return SCE_ERROR_OK;
}

//Subroutine scePower_driver_23BDDD8B - Address 0x00001028
s32 scePower_driver_23BDDD8B(void)
{
    g_Power.callbackArgMask &= ~SCE_POWER_CALLBACKARG_HOLDSW; // 0x00001038 
    return SCE_ERROR_OK;
}

//Subroutine scePower_driver_4E32E9B8 - Address 0x00001128
u8 scePowerGetWatchDog(void)
{
    return g_Power.watchDog; //0x00001130
}

//Subroutine scePower_driver_C463E7F2 - Address 0x00001140
u8 scePowerGetLedOffTiming(void)
{
    return g_Power.ledOffTiming; // 0x00001148
}

//0x0000114C
/*
 * Called after the power service was loaded and started by the Init module
 * (or Init has finished loading and starting the remaining kernel/VSH modules). 
 */
static s32 _scePowerInitCallback(void *data, s32 arg, void *opt)
{
    s32 appType;

    (void)data;
    (void)arg;
    (void)opt;
    
    appType = sceKernelApplicationType(); // 0x00001154   
    if (appType != SCE_INIT_APPLICATION_VSH && appType != SCE_INIT_APPLICATION_POPS) // 0x0000115C - 0x00001174
    {
        /* Set the initial clock frequencies. */
        _scePowerSetClockFrequency(g_Power.pllClockInitialFrequencyGameAppUpdater, g_Power.cpuClockInitialFrequencyGameAppUpdater, g_Power.busClockInitialFrequencyGameAppUpdater); // 0x00001194
    } 
    
    return SCE_ERROR_OK;
}

//sub_00001A94
/* Sends the new battery suspend capacity to Syscon. */
s32 _scePowerChangeSuspendCap(u32 newSuspendCap)
{
    u32 param[4];
    s32 status;
    
    status = sceSysconReceiveSetParam(0, &param); // 0x00001AAC
    if (status < SCE_ERROR_OK) // 0x00001ABC
    {
        return status;
    }

    /* Store the new suspend capacity in the first two bytes of our syscon param. */
    
    *((u8 *)param + 1) = (u8)((newSuspendCap & 0xFFFF) >> 8); // 0x00001AB0 & 0x00001AB4 & 0x00001AC4
    *(u8 *)param = (u8)(newSuspendCap & 0xFFFF); // 0x00001AB0 & 0x00001ACC

    /* Send the new battery suspend capacity to Syscon. */
    status = sceSysconSendSetParam(0, &param); // 0x00001AC8
    return (status < SCE_ERROR_OK) 
        ? status 
        : SCE_ERROR_OK;
}
