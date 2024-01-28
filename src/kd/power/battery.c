/* Copyright (C) The uOFW team
   See the file COPYING for copying permission.
*/

/*
 * uofw/src/power/battery.c
 *
 * This file implements the battery feature area of the power service. It provides public APIs to query the current
 * power supply (is supplied by battery or AC) and battery state (remaining battery capacity, current temperature,
 * voltage, charge cycle,...). Internally, we set up a worker thread which periodically polls the battery to update
 * the collected battery properties.
 * 
 * Starting with the PSP N-1000 (PSP-Go) serie, the battery equipped by default no longer has hardware included to
 * monitor the current battery state. In this case, the power service tries to estimate the current remaining battery
 * capacity based on the current battery voltage.
 * 
 * The battery feature area also provides public APIs to control the charging policies of the PSP system. Battery
 * charging can be allowed/disallowed and whether or not the battery can be charged over USB or not (if the PSP
 * device has a USB charging capability).
 * 
 * Last but not least, battery.c also implements a public API to query whether or not the PSP system should be
 * suspended due to critically low remaining battery capacity.
 */

#include <common_imp.h>
#include <interruptman.h>
#include <loadexec_kernel.h>
#include <power_kernel.h>
#include <syscon.h>
#include <sysmem_sysclib.h>
#include <sysmem_sysevent.h>
#include <threadman_kernel.h>

#include "power_int.h"

/* Permit Charge delay in microseconds */
#define POWER_DELAY_PERMIT_CHARGING                 (5 * 1000 * 1000)

/* The (initial) priority of the battery worker thread. */
#define POWER_BATTERY_WORKER_THREAD_PRIO            64

/*
 * This value is sepcified for the [batteryAvailabilityStatus] member of ScePowerBattery.
 * It represents the current availability status of the battery in the power service.
 */
typedef enum {
    /* No battery is equipped or the power service cannot communicate with the equipped battery. */
    BATTERY_AVAILABILITY_STATUS_BATTERY_NOT_INSTALLED = 0,
    /* The power service is busy getting new battery information after a battery/power state change. */
    BATTERY_AVAILABILITY_STATUS_BATTERY_IS_BEING_DETECTED,
    /* A battery is correctly equipped and its properties (reamining battery life, charging state,...) can be obtained by API. */
    BATTERY_AVAILABILITY_STATUS_BATTERY_AVAILABLE,
} ScePowerBatteryAvailabilityStatus;

/* ScePowerBattery.sysconCmdDisableUsbChargingExecStatus */

#define BATTERY_SYSCON_CMD_DISABLE_USB_CHARGING_EXEC_STATUS_READY    0 /* The command can be executed. */
#define BATTERY_SYSCON_CMD_DISABLE_USB_CHARGING_EXEC_STATUS_BUSY     1 /* The command is currently being executed by SYSCON. */
#define BATTERY_SYSCON_CMD_DISABLE_USB_CHARGING_EXEC_STATUS_DONE     2 /* SYSCON has finished executing the command. */

/* Action flags for the battery worker thread. */

#define BATTERY_EVENT_FORBID_BATTERY_CHARGING                               0x00000100 /* Forbid battery charging. */
#define BATTERY_EVENT_PERMIT_BATTERY_CHARGING                               0x00000200 /* Permit battery charging. */
#define BATTERY_EVENT_UNKNOWN_400                                           0x00000400 /* Unknown. Something to do with disabling USB charging. */
#define BATTERY_EVENT_ENABLE_USB_CHARGING                                   0x00000800 /* Enable USB charging. */
#define BATTERY_EVENT_UPDATE_BATTERY_INFO                                   0x10000000 /* Update the battery control block (remainCap, voltage, temperature,...). */
#define BATTERY_EVENT_RESUME_BATTERY_POLLING_AND_USB_CHARGE_MANAGEMENT      0x20000000 /* Suspend polling the battery for data and USB charge management. */
#define BATTERY_EVENT_SUSPEND_BATTERY_POLLING_AND_USB_CHARGE_MANAGEMENT     0x40000000 /* Resume polling the battery for data and USB charge management. */
#define BATTERY_EVENT_TERMINATION                                           0x80000000 /* Terminate the battery event thread. */

/* Defines the different battery info update operations of the battery worker thread. */
typedef enum {
    POWER_BATTERY_THREAD_OP_START = 0,
    POWER_BATTERY_THREAD_OP_SET_POWER_SUPPLY_STATUS = 1,
    POWER_BATTERY_THREAD_OP_SET_IS_BATTERY_CHARGING_USB = 3,
    POWER_BATTERY_THREAD_OP_SET_USB_STATUS = 4,
    POWER_BATTERY_THREAD_OP_SET_REMAIN_CAP = 5,
    POWER_BATTERY_THREAD_OP_SET_FULL_CAP = 6,
    POWER_BATTERY_THREAD_OP_SET_CHARGE_CYCLE = 7,
    POWER_BATTERY_THREAD_OP_SET_LIMIT_TIME = 8,
    POWER_BATTERY_THREAD_OP_SET_BATT_TEMP = 9,
    POWER_BATTERY_THREAD_OP_SET_BATT_ELEC = 10,
    POWER_BATTERY_THREAD_OP_SET_BATT_VOLT = 11,
} PowerBatteryThreadOperation;

typedef struct {
    u32 unk0;
    u8 unk4;
    u8 unk5;
    u8 unk6;
    u8 unk7;
} ScePowerSysconSetParamDataTTC; // size: 8

/*
 * This structure represents the power service's internal control block for battery management.
 */
typedef struct  {
    SceUID batteryEventFlagId; // 0
    SceUID workerThreadId; // 4
    /* The battery capacity threshold value in mAh at which point the PSP system is automatically suspended. */
    s32 forceSuspendCapacity; // 8
    /*
     * The battery capacity threshold value in mAh at which point the remaining battery lifetime is considered
     * to be short.
     */
    s32 lowBatteryCapacity; // 12
    /*
     * Indicates whether the battery manager is set up to consistently poll the battery for new
     * data (temperatur, remaining capacity,...) and can enable/disable USB charging. For example,
     * if the PSP system enters the suspended power state, the battery manager won't poll (repeatedly)
     * for new battery data.
     */
    u32 isBatteryPollingAndUsbManagementSuspended; // 16
    /* 
     * Represents the total number of [forbid battery charge] requests minus the total number of 
     * [permit battery charge] requests (since cold boot). Consequently, if this number is greather 
     * than 0, there have been more [forbid battery charge] requests than [permit battery charge] requests. 
     * 
     * Note: "0" is its minimum (we do not "ignore" [forbid battery charge] requests).
     */
    u32 batteryForbidChargingNetCounter; // 20
    /* Indicates whether or not battery charging over USB is supported. */
    u32 isUsbChargingSupported; // 24
    /* Indicates whether or not battery charging over USB is currently enabled. */
    u32 isUsbChargingEnabled; // 28
    u32 unk32; // 32 -- TODO: A boolean flag dealing with USB charging.
    SceUID permitChargingDelayAlarmId; // 36
    /* Indicates whether or not the battery is currently charging. */
    u32 isBatteryCharging; // 40
    /* The type of the battery. One of ::ScePowerBatteryType. */
    u32 batteryType; // 44
    /*
     * Indicates whether or not the battery manager is currently polling the battery for data
     * (like remaining capacity, battery temperature or voltage).
     */
    u32 isBatteryInfoUpdateInProgress; // 48
    PowerBatteryThreadOperation workerThreadNextOp; // 52
    /*
     * Indicates whether or not the PSP device is currently connected to an external power source
     * via an AC adapter.
     */
    u32 isAcSupplied; // 56
    /* The current power supply status of the PSP system. */
    u32 powerSupplyStatus; // 60 
    /* The current availibilty status of the battery to the power service. One of ::ScePowerBatteryAvailabilityStatus. */
    u32 batteryAvailabilityStatus; // 64
    /* Unknown. */
    u32 batteryStatus; // 68
    /* The current remaining capacity of the battery in mAh. */
    s32 batteryRemainingCapacity; // 72
    /* The current remaining percentage of battery life relative to a fully charged status. */
    s32 batteryLifePercentage; // 76
    /* The full capacity of the battery in mAh. */
    s32 batteryFullCapacity; // 80
    /*
     * Represents the minimum full capacity of a battery. The full capacity of a battery can only be
     * accurately determined with a full charge (where the charge is cutoff at the end of the chargin process).
     * As such, the actual full capacity of a battery might not always be corectly available. In some cases
     * (such as swapping a battery?) the last reported full capacity might be less than the actual full capacity.
     * The [minimumFullCapacity] is thus used to provide a more accurate full capacity value until the battery
     * equipped has been fully charged again and a new full capacity could be ascertained.
     * 
     * The minimum full capacity is determined by initializing it to the current remaining battery capacity on PSP
     * startup and and then update it to the new reaming capacity value whenever that value is a new maximum
     * during the the use of the PSP system between two cold reboots. For example, this is the case when the battery
     * is being charged when the PSP system is turned on.
     */
    s32 minimumFullCapacity; // 84
    /* The current battery charge cycle count. */
    s32 batteryChargeCycle; // 88
    /*
     * Represents the remaining available minutes of use to the user of the PSP system. If this member
     * is set to a valid value, it will be considered in the calculation of the remaining battery lifetime.
     */
    s32 limitTime; // 92
    /* The current battery temperature in degree Celsius. */
    s32 batteryTemp; // 96
    /* The current electric charge value of the battery. */
    s32 batteryElec; // 100
    /* The current battery voltage in mV. */
    s32 batteryVoltage; // 104
    u32 sysconCmdDisableUsbChargingExecStatus; // 108
    SceSysconPacket powerBatterySysconPacket; // 112
    ScePowerSysconSetParamDataTTC ttcConfig;
} ScePowerBattery; //size: 216

static s32 _scePowerBatteryThread(SceSize args, void *argp);
static inline void _scePowerBatteryThreadErrorObtainBattInfo();
static s32 _scePowerBatteryCalcRivisedRcap(void); // 0x00005130
static s32 _scePowerBatteryConvertVoltToRCap(s32 voltage); // 0x00005130
static s32 _scePowerBatterySetTTC(s32 arg0); // 0x000056A4
static SceUInt _scePowerBatteryDelayedPermitCharging(void* common); // 0x00005EA4
static s32 _scePowerBatterySysconCmdIntr(SceSysconPacket* pSysconPacket, void* param); // 0x00005ED8

ScePowerBattery g_Battery; //0x0000C5B8

// Subroutine sub_00005B1C - Address 0x00005B1C
/* Initialize power service's internal batter manager. */
s32 _scePowerBatteryInit(u32 isUsbChargingSupported, u32 batteryType)
{
    memset(&g_Battery, 0, sizeof(ScePowerBattery)); // 0x00005B50

    g_Battery.permitChargingDelayAlarmId = -1; // 0x00005B74
    g_Battery.isUsbChargingSupported = isUsbChargingSupported; // 0x00005B78
    g_Battery.batteryType = batteryType; // 0x00005B7C
    g_Battery.isAcSupplied = -1; // 0x00005B80
    g_Battery.batteryFullCapacity = -1; // 0x00005B84
    g_Battery.batteryChargeCycle = -1; // 0x00005B8C

    g_Battery.batteryEventFlagId = sceKernelCreateEventFlag("ScePowerBattery", 1, 0, NULL); // 0x00005B88
    g_Battery.workerThreadId = sceKernelCreateThread("ScePowerBattery", _scePowerBatteryThread, POWER_BATTERY_WORKER_THREAD_PRIO,
        2 * SCE_KERNEL_1KiB, SCE_KERNEL_TH_NO_FILLSTACK | 0x1, NULL); // 0x00005BB0

    sceKernelStartThread(g_Battery.workerThreadId, 0, NULL); // 0x00005BC4

    return SCE_ERROR_OK;
}

//Subroutine sub_00004498 - Address 0x00004498
/* Terminate power service's internal battery manager. */
s32 _scePowerBatteryEnd(void)
{
    /* 
     * At the time of power service termination, there might be an active [permit battery charging] request
     * in our battery manager. This request might currently be lined up in our battery worker thread for
     * execution or scheduled to happen after some delay time. However, as we are in the process of shutting
     * down the kernel (anf thus our battery worker thread) we will removed any scheduled/lined up work here 
     * and directly allow battery charging. That way, we won't hold up the termination process waiting for our
     * battery worker thread to have allowed battery charging again.
     */

    u32 eventFlagSetValue;

    /* 
     * If the battery is scheduled to be allowed to charge, we remove this scheduled operation
     * and allow the battery to charge now.
     */
    if (g_Battery.permitChargingDelayAlarmId > 0) // 0x000044C0
    {
        sceKernelCancelAlarm(g_Battery.permitChargingDelayAlarmId); // 0x000044C8
        g_Battery.permitChargingDelayAlarmId = -1; // 0x000044DC

        /* Allow the battery to charge (see code below). */
        eventFlagSetValue = BATTERY_EVENT_PERMIT_BATTERY_CHARGING; // 0x000044D0       
    }
    else
    {
        /* 
         * If a [permit battery charge] operation has been lined up for our battery worker thread,
         * remove it now and directly permit battery charging instead.
         */
        s32 status = sceKernelPollEventFlag(g_Battery.batteryEventFlagId,
            BATTERY_EVENT_PERMIT_BATTERY_CHARGING, SCE_KERNEL_EW_CLEAR_PAT | SCE_KERNEL_EW_OR, &eventFlagSetValue); //0x00004554
        if (status < SCE_ERROR_OK)
        {
            /* There is no [permit battery charge] operation lined up (or an error occured). */
            eventFlagSetValue = 0; // 0x00004564
        }
    }

    /* Remove currently lined up battery charging operations for our battery work thread. */

    u32 battEventFlagClearValue = ~(BATTERY_EVENT_FORBID_BATTERY_CHARGING | BATTERY_EVENT_PERMIT_BATTERY_CHARGING 
        | BATTERY_EVENT_UNKNOWN_400 | BATTERY_EVENT_ENABLE_USB_CHARGING); // 0x000044E4

    sceKernelClearEventFlag(g_Battery.batteryEventFlagId, battEventFlagClearValue); // 0x000044E8

    /* Command our battery worker thread to terminate. */
    sceKernelSetEventFlag(g_Battery.batteryEventFlagId, BATTERY_EVENT_TERMINATION); // 0x000044F4

    /* If the battery was to be allowed to charge, permit charging now. */
    if (eventFlagSetValue & BATTERY_EVENT_PERMIT_BATTERY_CHARGING) // 0x00004504
    {
        sceSysconPermitChargeBattery(); // 0x00004544
    }

    /* Wait for our battery worker thread to end. */
    sceKernelWaitThreadEnd(g_Battery.workerThreadId, NULL); // 0x00004510

    /* Delete worker resources. */

    sceKernelDeleteThread(g_Battery.workerThreadId); // 0x00004518
    sceKernelDeleteEventFlag(g_Battery.batteryEventFlagId); // 0x00004524

    return SCE_ERROR_OK;
}

// Subroutine sub_00004570 - Address 0x00004570 
/* Suspends the power service's internal battery manager. */
s32 _scePowerBatterySuspend(void)
{
    s32 intrState;
    u32 eventFlagBits;

    /* 
     * When the PSP system enters the [suspended] power state, we want to suspend our battery
     * worker thread as well (so that it won't continue polling for updated battery infomration
     * (like remaining capacity, temperature, voltage,...).
     */
    eventFlagBits = BATTERY_EVENT_SUSPEND_BATTERY_POLLING_AND_USB_CHARGE_MANAGEMENT; // 0x00004594

    intrState = sceKernelCpuSuspendIntr(); // 0x00004590

    /* 
     * If a scheduled [permit battery charging] operation is in the system at the time of suspension,
     * cancel it and permit battery charging directly.
     */
    if (g_Battery.permitChargingDelayAlarmId > 0) // 0x000045A0
    {
        sceKernelCancelAlarm(g_Battery.permitChargingDelayAlarmId); // 0x000045A8
        g_Battery.permitChargingDelayAlarmId = -1;

        eventFlagBits |= BATTERY_EVENT_PERMIT_BATTERY_CHARGING;
    }

    /* Permit battery charging in case it is currently forbidden. */
    if (g_Battery.batteryForbidChargingNetCounter != 0) // 0x000045D4
    {
        eventFlagBits |= BATTERY_EVENT_PERMIT_BATTERY_CHARGING;
    }

    /* Disable USB charging (USB charging is not supported when the PSP device is suspended). */
    if (g_Battery.isUsbChargingEnabled) // 0x000045D0
    {
        g_Battery.isUsbChargingEnabled = SCE_FALSE; // 0x000045D8
        eventFlagBits |= BATTERY_EVENT_UNKNOWN_400; // 0x000045DC
    }

    /* Remove lined up battery worker thread resume command (if one exists). */
    sceKernelClearEventFlag(g_Battery.batteryEventFlagId, ~BATTERY_EVENT_RESUME_BATTERY_POLLING_AND_USB_CHARGE_MANAGEMENT); // 0x000045E0

    /* 
     * Command the worker thread to suspend itself and allow battery charging/disable USB charging 
     * if required. 
     */
    sceKernelSetEventFlag(g_Battery.batteryEventFlagId, eventFlagBits); // 0x000045EC

    sceKernelCpuResumeIntr(intrState);
    return SCE_ERROR_OK;
}

// Subroutine sub_00005C18 - Address 0x00005C18
s32 _scePowerBatteryResume(void)
{
    s32 intrState1;
    s32 intrState2;

    intrState1 = sceKernelCpuSuspendIntr(); // 0x00005C2C

    g_Battery.isAcSupplied = -1; // 0x00005C40
    g_Battery.batteryAvailabilityStatus = BATTERY_AVAILABILITY_STATUS_BATTERY_NOT_INSTALLED; // 0x00005C4C
    g_Battery.isBatteryPollingAndUsbManagementSuspended = SCE_FALSE; // 0x00005C4C

    u32 battEventFlagSetValue = g_Battery.batteryForbidChargingNetCounter == 0
        ? BATTERY_EVENT_RESUME_BATTERY_POLLING_AND_USB_CHARGE_MANAGEMENT
        : (BATTERY_EVENT_RESUME_BATTERY_POLLING_AND_USB_CHARGE_MANAGEMENT | BATTERY_EVENT_FORBID_BATTERY_CHARGING);

    sceKernelClearEventFlag(g_Battery.batteryEventFlagId, ~(BATTERY_EVENT_ENABLE_USB_CHARGING | BATTERY_EVENT_FORBID_BATTERY_CHARGING)); // 0x00005C64
    sceKernelSetEventFlag(g_Battery.batteryEventFlagId, battEventFlagSetValue); // 0x00005C70

    intrState2 = sceKernelCpuSuspendIntr(); // 0x00005C78

    /* Poll the battery again and refresh power service's collected battery data. */
    sceKernelSetEventFlag(g_Battery.batteryEventFlagId, BATTERY_EVENT_UPDATE_BATTERY_INFO); // 0x00005C88
    sceKernelClearEventFlag(g_Battery.batteryEventFlagId, ~BATTERY_EVENT_UPDATE_BATTERY_INFO); // 0x00005C98

    sceKernelCpuResumeIntr(intrState2); // 0x00005CA0
    sceKernelCpuResumeIntr(intrState1); // 0x00005CA8

    return SCE_ERROR_OK;
}

// Subroutine sub_00005BF0 - Address 0x00005BF0
/* Sets the battery capacities [forceSuspend] and [low] for the battery manager. */
s32 _scePowerBatterySetParam(s32 forceSuspendCapacity, s32 lowBatteryCapacity)
{
    g_Battery.lowBatteryCapacity = lowBatteryCapacity; // 0x00005BFC
    g_Battery.forceSuspendCapacity = forceSuspendCapacity; // 0x00005C04

    return SCE_ERROR_OK;
}

// Subroutine sub_0000461C - Address 0x0000461C 
/* Called during PSP resume phase [phase0]. Updates our battery status control block. */
s32 _scePowerBatteryUpdatePhase0(ScePowerResumeInfo *pResumeInfo, u32 *pPowerStateForCallbackArg)
{
    u32 powerSupplyStatus;

    powerSupplyStatus = pResumeInfo->powerSupplyStatus;

    g_Battery.limitTime = -1; // 0x00004644
    g_Battery.powerSupplyStatus = powerSupplyStatus; // 0x0000464C
    g_Battery.batteryTemp = -1; // 0x00004650
    g_Battery.batteryElec = -1; // 0x00004654
    g_Battery.batteryVoltage = -1; // 0x0000465C

    /* Check if the PSP system has a battery equipped. */
    if (powerSupplyStatus & SCE_SYSCON_POWER_SUPPLY_STATUS_BATTERY_EQUIPPED) // 0x00004658
    {
        /* A battery is equipped. Update our internal battery status control block. */

        g_Battery.batteryAvailabilityStatus = BATTERY_AVAILABILITY_STATUS_BATTERY_IS_BEING_DETECTED; // 0x0000466C

        /* Check if the type of the equipped battery supports battery state monitoring. */
        if (g_Battery.batteryType == SCE_POWER_BATTERY_TYPE_BATTERY_STATE_MONITORING_SUPPORTED) // 0x00004668
        {
            /* 
             * Battery state monitoring is supported. As such, we have access to battery data like its full &
             * remaining battery capacities and can calculate its remaining battery life (for example in [%]). 
             */

            g_Battery.batteryStatus = pResumeInfo->batteryStatus; // 0x000046A8
            g_Battery.batteryRemainingCapacity = pResumeInfo->batteryRemainCapacity; // 0x000046B0
            g_Battery.minimumFullCapacity = pResumeInfo->batteryRemainCapacity; // 0x000046B8
            g_Battery.batteryChargeCycle = -1; // 0x000046C0
            g_Battery.batteryFullCapacity = pResumeInfo->batteryFullCapacity; // 0x000046C8

            // Note: In earlier versions, this was
            // g_Battery.batteryLifePercentage = _scePowerBatteryConvertVoltToRCap(*(u32*)(arg0 + 44));
            g_Battery.batteryLifePercentage = _scePowerBatteryCalcRivisedRcap(); // 0x000046C4 & 0x000046D0
        }

        /* 
         * Update power service's power callback argument holder so that power callback subscribers can
         * notice that a battery is currently equipped.
         */
        *pPowerStateForCallbackArg &= ~SCE_POWER_CALLBACKARG_BATTERY_CAP; // 0x00004678
        *pPowerStateForCallbackArg |= g_Battery.batteryLifePercentage | SCE_POWER_CALLBACKARG_BATTERYEXIST; // 0x00004684 & 0x00004688
    }
    else
    {
        /* No battery is currently equipped. */

        g_Battery.minimumFullCapacity = -1; // 0x000046D4
        g_Battery.batteryAvailabilityStatus = BATTERY_AVAILABILITY_STATUS_BATTERY_NOT_INSTALLED; // 0x000046D8
        g_Battery.batteryStatus = 0; // 0x000046DC
        g_Battery.batteryRemainingCapacity = -1;
        g_Battery.batteryLifePercentage = -1;
        g_Battery.batteryFullCapacity = -1;
        g_Battery.batteryChargeCycle = -1; // 0x000046EC

        /*
         * Update power service's power callback argument holder so that power callback subscribers can
         * notice that a battery is not currently equipped.
         */
        *pPowerStateForCallbackArg &= ~(SCE_POWER_CALLBACKARG_BATTERY_CAP | SCE_POWER_CALLBACKARG_BATTERYEXIST); // 0x000046F8 & 0x00004688
    }

    return SCE_ERROR_OK;
}

#define BATTERY_EVENT_POLL_NO_WAIT      0
#define BATTERY_EVENT_INDEFINITE_WAIT   (-1)

// Subroutine sub_0x000046FC - Address 0x000046FC
/* Polls the battery for a fresh set of data and controls battery charging. */
static s32 _scePowerBatteryThread(SceSize args, void* argp)
{
    (void)args;
    (void)argp;

    s32 isUsbChargingEnabled;
    s32 timeout; // $sp + 4
    s32 batteryEventFlagCheckValue; // $s2
    u32 batteryEventFlagSetValue; // $sp

    g_Battery.isBatteryInfoUpdateInProgress = SCE_TRUE; // 0x00004718
    g_Battery.isBatteryPollingAndUsbManagementSuspended = SCE_FALSE; // 0x0000471C

    g_Battery.workerThreadNextOp = POWER_BATTERY_THREAD_OP_START; // 0x00004720

    batteryEventFlagCheckValue = BATTERY_EVENT_SUSPEND_BATTERY_POLLING_AND_USB_CHARGE_MANAGEMENT
        | BATTERY_EVENT_UNKNOWN_400
        | BATTERY_EVENT_PERMIT_BATTERY_CHARGING | BATTERY_EVENT_FORBID_BATTERY_CHARGING; // 0x00004728

    timeout = BATTERY_EVENT_POLL_NO_WAIT; // 0x0000473C

    isUsbChargingEnabled = scePowerGetUsbChargingCapability(); // 0x00004738
    if (isUsbChargingEnabled) // 0x00004740
    {
        u8 version = _sceSysconGetBaryonVersion(); // 0x000050CC
        if (PSP_SYSCON_BARYON_GET_VERSION_MAJOR(version) == 0x2 &&
            PSP_SYSCON_BARYON_GET_VERSION_MINOR(version) >= 0x2 && PSP_SYSCON_BARYON_GET_VERSION_MINOR(version) < 0x6) // 0x000050D0 - 0x00005118
        {
            /* We are running on a PSP-2000 series model. */
            sceSysconReceiveSetParam(SCE_SYSCON_SET_PARAM_POWER_BATTERY_TTC, &g_Battery.ttcConfig); // 0x00005120
        }

        _scePowerBatterySetTTC(1); // 0x000050E4
    }

    s32 status; // $a0 in loc_000047AC
    for (;;)
    {
        /* Poll our event flag to see what operations are currently requested by the power service. */

        /* Reset the result bit pattern. */
        batteryEventFlagSetValue = 0; // 0x0000474C

        if (timeout == BATTERY_EVENT_POLL_NO_WAIT) // 0x0000474C
        {
            sceKernelPollEventFlag(
                g_Battery.batteryEventFlagId, 
                batteryEventFlagCheckValue | BATTERY_EVENT_TERMINATION,
                SCE_KERNEL_EW_OR, &batteryEventFlagSetValue); // 0x000050B4

            status = SCE_ERROR_OK; // 0x000050C0
        }
        else
        {
            SceUInt *pTimeout = (timeout == BATTERY_EVENT_INDEFINITE_WAIT) // 0x00004758  - (0x00004760 - 0x00004770)/0x000050A0
                ? NULL
                : (SceUInt *)&timeout;

            status = sceKernelWaitEventFlag(
                g_Battery.batteryEventFlagId, 
                batteryEventFlagCheckValue | BATTERY_EVENT_TERMINATION | BATTERY_EVENT_UPDATE_BATTERY_INFO,
                SCE_KERNEL_EW_OR, &batteryEventFlagSetValue, pTimeout); // 0x00004774 & 0x00004788

            if (batteryEventFlagSetValue & BATTERY_EVENT_UPDATE_BATTERY_INFO 
                && g_Battery.isBatteryInfoUpdateInProgress == SCE_FALSE) // 0x00004788 & 0x00004794
            {
                g_Battery.isBatteryInfoUpdateInProgress = SCE_TRUE; // 0x000047A0
                g_Battery.workerThreadNextOp = POWER_BATTERY_THREAD_OP_START; // 0x000047A4
            }
        }

        /* Check if we need to terminate the battery worker thread. */
        if ((status < SCE_ERROR_OK && status != (s32)SCE_ERROR_KERNEL_WAIT_TIMEOUT) 
            || batteryEventFlagSetValue & BATTERY_EVENT_TERMINATION) // 0x000047AC - 0x000047C0 & 0x000047C8
        {
            return SCE_ERROR_OK;
        }

        /* Check if we need to permit battery charging. */
        if (batteryEventFlagSetValue & BATTERY_EVENT_PERMIT_BATTERY_CHARGING) // 0x000047D0
        {
            /* Handle [permit battery charge] request. */

            /* Permit battery charging. */
            sceSysconPermitChargeBattery();  // 0x00005028

            /* Remove lined-up [permit battery charge] command now that we've handled it.*/
            sceKernelClearEventFlag(g_Battery.batteryEventFlagId, ~BATTERY_EVENT_PERMIT_BATTERY_CHARGING); // 0x00005034

            if (!(batteryEventFlagSetValue & BATTERY_EVENT_SUSPEND_BATTERY_POLLING_AND_USB_CHARGE_MANAGEMENT)) // 0x00005048
            {
                sceKernelDelayThread(1.5 * 1000 * 1000); // 0x00005054
                continue; // 0x0000505C
            }
        }

        /* 
         * Check if we need to suspend battery polling and USB charge management. For example, this is the case
         * when the PSP system is suspending.
         */
        if (batteryEventFlagSetValue & BATTERY_EVENT_SUSPEND_BATTERY_POLLING_AND_USB_CHARGE_MANAGEMENT) // 0x000047DC
        {
            // loc_00004FF0
            _scePowerBatterySetTTC(1); // 0x00004FF0

            sceKernelClearEventFlag(g_Battery.batteryEventFlagId, ~BATTERY_EVENT_SUSPEND_BATTERY_POLLING_AND_USB_CHARGE_MANAGEMENT); // 0x00005004

            g_Battery.isBatteryPollingAndUsbManagementSuspended = SCE_TRUE; // 0x0000501C
            timeout = BATTERY_EVENT_INDEFINITE_WAIT; // 0x00005024

            batteryEventFlagCheckValue = BATTERY_EVENT_SUSPEND_BATTERY_POLLING_AND_USB_CHARGE_MANAGEMENT
                | BATTERY_EVENT_RESUME_BATTERY_POLLING_AND_USB_CHARGE_MANAGEMENT | BATTERY_EVENT_FORBID_BATTERY_CHARGING; // 0x00005008 & 0x0000500C

            continue; // 0x00005020
        }

        if (batteryEventFlagSetValue & BATTERY_EVENT_RESUME_BATTERY_POLLING_AND_USB_CHARGE_MANAGEMENT) // 0x000047E8
        {
            // loc_00004FC4
            sceKernelClearEventFlag(g_Battery.batteryEventFlagId, ~BATTERY_EVENT_RESUME_BATTERY_POLLING_AND_USB_CHARGE_MANAGEMENT); // 0x00004FD0

            g_Battery.batteryAvailabilityStatus = BATTERY_AVAILABILITY_STATUS_BATTERY_NOT_INSTALLED; // 0x00004FE0
            g_Battery.workerThreadNextOp = POWER_BATTERY_THREAD_OP_START; // 0x00004FE4
            g_Battery.isBatteryPollingAndUsbManagementSuspended = SCE_FALSE; // 0x00004FEC

            batteryEventFlagCheckValue = BATTERY_EVENT_SUSPEND_BATTERY_POLLING_AND_USB_CHARGE_MANAGEMENT
                | BATTERY_EVENT_UNKNOWN_400
                | BATTERY_EVENT_PERMIT_BATTERY_CHARGING | BATTERY_EVENT_FORBID_BATTERY_CHARGING; // 0x00004FDC

            timeout = BATTERY_EVENT_POLL_NO_WAIT; // 0x000048C0

            continue; // 0x000048BC
        }

        /* Check if we need to forbid battery charging. */
        if (batteryEventFlagSetValue & BATTERY_EVENT_FORBID_BATTERY_CHARGING) // 0x000047F0
        {
            /* Handle [forbid battery charge] request. */

            /* Forbid battery charging. */
            sceSysconForbidChargeBattery(); // 0x00004F9C

            /* Remove lined-up [forbid battery charge] command now that we've handled it.*/
            sceKernelClearEventFlag(g_Battery.batteryEventFlagId, ~BATTERY_EVENT_FORBID_BATTERY_CHARGING); // 0x00004FA8

            /* Wait for 1.5 seconds */
            sceKernelDelayThread(1.5 * 1000 * 1000); // 0x00004FB4
        }

        /* If battery polling and USB charge management is suspended, we are done here. */
        if (g_Battery.isBatteryPollingAndUsbManagementSuspended) // 0x00004800
        {
            continue;
        }

        /* Handle USB charging. We check if USB charging needs to be enabled/disabled. */

        u8 isAcSupplied = sceSysconIsAcSupplied(); // 0x00004808 -- $s3

        /* Check if we need to enable USB battery charging. */
        if (batteryEventFlagSetValue & BATTERY_EVENT_ENABLE_USB_CHARGING) // 0x00004818
        {
            /*
             * Only enable USB charging when the PSP system is not currently connected to
             * an external power source via an AC adapter.
             */
            if (isAcSupplied) // 0x00004820
            {
                // loc_00004ECC
                sceSysconSetUSBStatus(SCE_SYSCON_USB_STATUS_CHARGING_DISABLED); // 0x00004ECC

                _scePowerBatterySetTTC(1); // 0x00004ED8 & 0x00004EBC
            }
            else if (g_Battery.sysconCmdDisableUsbChargingExecStatus == BATTERY_SYSCON_CMD_DISABLE_USB_CHARGING_EXEC_STATUS_READY) // 0x0000482C
            {
                sceSysconSetUSBStatus(SCE_SYSCON_USB_STATUS_CHARGING_ENABLED); // 0x00004EA0

                // TODO: Define constant for 1251 battery full capacity
                _scePowerBatterySetTTC((g_Battery.batteryFullCapacity < 1251) ? 1 : 0); // 0x00004EA8 - 0x00004EBC
            }

            sceKernelClearEventFlag(g_Battery.batteryEventFlagId, ~BATTERY_EVENT_ENABLE_USB_CHARGING); // 0x00004838
        }
        else if (batteryEventFlagSetValue & BATTERY_EVENT_UNKNOWN_400) // 0x00004EE0
        {
            g_Battery.unk32 = 0; // 0x00004F78
            sceSysconSetUSBStatus(SCE_SYSCON_USB_STATUS_UNKNOWN_0); // 0x00004F7C

            _scePowerBatterySetTTC(1); // 0x00004F84

            sceKernelClearEventFlag(g_Battery.batteryEventFlagId, ~BATTERY_EVENT_FORBID_BATTERY_CHARGING); // 0x00004F94 & 0x00004838
        }
        else if (g_Battery.sysconCmdDisableUsbChargingExecStatus == BATTERY_SYSCON_CMD_DISABLE_USB_CHARGING_EXEC_STATUS_DONE) // 0x00004EEC
        {
            g_Battery.sysconCmdDisableUsbChargingExecStatus = BATTERY_SYSCON_CMD_DISABLE_USB_CHARGING_EXEC_STATUS_READY; // 0x00004EF0

            // loc_00004F68
            _scePowerBatterySetTTC(1);
        }
        else if (g_Battery.isUsbChargingEnabled && g_Battery.isAcSupplied != isAcSupplied) // 0x00004EF8 & 0x00004F04
        {
            /* 
            * Charging the PSP via an AC adapter (power plug) takes precedence over USB charging. As such, we
            * check if the AC connection status has changed while USB charging is enabled:
            * 
            * PSP power now supplied by an AC adapter: Stop USB charging.
            * PSP power no longer supplied by an AC adapter: Start USB charging.
            */

            if (!isAcSupplied) // 0x00004F0C
            {
                /* Start USB charging. */
                sceSysconSetUSBStatus(SCE_SYSCON_USB_STATUS_CHARGING_ENABLED); // 0x00004F48

                // TODO: Define constant for 1251 (or 1250) battery full capacity
                _scePowerBatterySetTTC((g_Battery.batteryFullCapacity < 1251) ? 1 : 0); // 0x00004F50 - 0x00004F64 & 0x00004F20
            }
            else
            {
                /* Stop USB charging. */
                sceSysconSetUSBStatus(SCE_SYSCON_USB_STATUS_CHARGING_DISABLED); // 0x00004F1C

                _scePowerBatterySetTTC(1); // 0x00004F20
            }
        }
        else if (isAcSupplied && g_Battery.unk32 != 0) // 0x00004F28 & 0x00004F38
        {
            g_Battery.unk32 = 0; // 0x00004F3C
        }

        timeout = 5000000; // 0x00004850

        /* Set the next battery polling operation dependeing on whether a battery is currently equipped or not. */
        if (g_Battery.batteryAvailabilityStatus == BATTERY_AVAILABILITY_STATUS_BATTERY_NOT_INSTALLED) // 0x0000484C
        {
            /* If no battery is currently equipped, skip all the commands which require battery polling. */
            if (g_Battery.workerThreadNextOp > POWER_BATTERY_THREAD_OP_SET_POWER_SUPPLY_STATUS 
                && g_Battery.workerThreadNextOp <= (POWER_BATTERY_THREAD_OP_SET_BATT_VOLT + 1)) // 0x00004854 - 0x00004860
            {
                g_Battery.workerThreadNextOp = POWER_BATTERY_THREAD_OP_START; // 0x00004864
            }
        }
        else if (g_Battery.batteryAvailabilityStatus == BATTERY_AVAILABILITY_STATUS_BATTERY_IS_BEING_DETECTED) // 0x00004E44
        {
            timeout = 20000; // 0x00004E9C 
        }
        else if (g_Battery.batteryAvailabilityStatus == BATTERY_AVAILABILITY_STATUS_BATTERY_AVAILABLE
            && g_Battery.batteryType == SCE_POWER_BATTERY_TYPE_BATTERY_STATE_MONITORING_SUPPORTED) // 0x00004E4C & 0x00004E58
        {
            /* 
             * If the equipped battery supports accurate battery monitoring, we update its remaining battery
             * life (relative to a full charge) and notify any power callback subscribers.
             */
            s32 newBatteryLifePercentage = _scePowerBatteryCalcRivisedRcap(); // 0x00004E60
            if (newBatteryLifePercentage != g_Battery.batteryLifePercentage) // 0x00004E6C
            {
                /* Update the remaining relative battery life. */
                g_Battery.batteryLifePercentage = newBatteryLifePercentage; // 0x00004E74

                /* Notify power callback subscribers about the updated remaining relative battery life. */
                _scePowerNotifyCallback(
                    SCE_POWER_CALLBACKARG_BATTERY_CAP,
                    newBatteryLifePercentage | SCE_POWER_CALLBACKARG_BATTERYEXIST, 
                    0); // 0x00004E7C
            }
        }

        if (g_Battery.isBatteryInfoUpdateInProgress) // 0x00004870
        {
            timeout = 20000; // 0x0000487C
        }

        /* Update the battery AC-supply status. */
        g_Battery.isAcSupplied = isAcSupplied; // 0x00004884

        /* 
         * Execute a battery thread operation. An operation is used to update power service's power supply
         * and battery data. Each operation updates a specific set of power supply/battery data in the
         * power service. These operations are run consequtively in a battery polling round. If the final
         * operation was run, the current polling round is considered to be over. Each polling round starts
         * with the [op_start] operation.
         * 
         * Whether an operation is executed or not depends on if a battery is currently available
         * and the type of the equipped battery. If the battery does not feature battery monitoring
         * capabilities, the following operations won't be run:
         * - Obtain the battery's full capacity
         * - Obtain the battery's charge cycle count
         * - Obtain the battery's limit time
         * - Obtain the battery's temperature
         * - Obtain the battery's electric charge
         * 
         * Note that while the operation to obtain the remaining battery capacity is run for both types of
         * battery, in case of the battery type not supporting monitoring capabilities, only an _estimate_
         * of the remaining capacity derived from the current battery voltage is obtained. See the comments
         * in this specific opeation for more details and drawbacks of that approach.
         * 
         * A polling round also sets the battery availability status and thus the indicator whether applications
         * can poll the power service for battery data or not. If the currently equipped battery is polled for the
         * first time (i.e. it was just recently equipped), the [battery detecting] status is set at the
         * beginning of a polling round. Once the polling round has been successfully completed, we then set
         * the battery availability status to [battery available].
         * 
         * 
         */
        switch (g_Battery.workerThreadNextOp) // 0x0000488C & 0x000048A8
        {
            case POWER_BATTERY_THREAD_OP_START:
            {
                g_Battery.workerThreadNextOp = POWER_BATTERY_THREAD_OP_SET_POWER_SUPPLY_STATUS; // 0x000048B4
                timeout = BATTERY_EVENT_POLL_NO_WAIT; // 0x000048C0

                continue;
            }
            case POWER_BATTERY_THREAD_OP_SET_POWER_SUPPLY_STATUS:
            {
                /* 
                 * If a battery has been newly equipped and the power has not yet polled the battery, the battery
                 * availability status is set to [battery detecting]. The power service will do a full round of
                 * battery polling (running through all applicable operations (depending on the battery type)) and
                 * only once it has completed obtaining all the battery data it can will it set the battery
                 * availability status to [battery available].
                 * 
                 * Data try to be obtained as part of this operation:
                 *  - the current power suppyly status
                 *  - whether or not the battery is currenly charging (only f USB charging is currently disabled)
                 */

                // 0x000048C4
                s32 powerSupplyStatus;
                status = sceSysconGetPowerSupplyStatus(&powerSupplyStatus); // 0x000048C4
                if (status >= SCE_ERROR_OK) // 0x000048CC
                {
                    /* Check if a battery is currently equipped. */
                    g_Battery.powerSupplyStatus = powerSupplyStatus; // 0x000048E4
                    if (!(powerSupplyStatus & SCE_SYSCON_POWER_SUPPLY_STATUS_BATTERY_EQUIPPED)) // 0x000048E0
                    {
                        /*
                         * No battery equipped. Invalidate collected battery data and generate a [battery not equipped]
                         * callback.
                         */
                        _scePowerBatteryThreadErrorObtainBattInfo(); // 00004984 - 0x00004A04

                        /* Since no battery is currently equipped, we are done here. */
                        g_Battery.isBatteryInfoUpdateInProgress = SCE_FALSE; // 0x00004990
                        g_Battery.workerThreadNextOp = POWER_BATTERY_THREAD_OP_START; // 0x00004998
                    }
                    else
                    {
                        /* A battery is currently equipped. */

                        /*
                         * If the power service had yet to poll ("detect") the battery, we set the battery availability status
                         * to [battery detecting] to indicate that the power service is currently busy obtaining battery data.
                         */
                        if (g_Battery.batteryAvailabilityStatus == BATTERY_AVAILABILITY_STATUS_BATTERY_NOT_INSTALLED) // 0x000048EC
                        {
                            g_Battery.batteryAvailabilityStatus = BATTERY_AVAILABILITY_STATUS_BATTERY_IS_BEING_DETECTED; // 0x000048FC
                            timeout = 20000; // 0x00004900
                        }

                        /*
                         * Check if USB charging is enabled. If USB charging is enabled, the battery charging state
                         * is obtained differently than if the battery is charging over an AC adapter.
                         */
                        if (!g_Battery.isUsbChargingEnabled) // 0x00004908
                        {
                            /* As we are not charging over USB, we can ask SYSCON whether the battery is currently charging. */
                            g_Battery.isBatteryCharging = (powerSupplyStatus & SCE_SYSCON_POWER_SUPPLY_STATUS_BATTERY_CHARGING) 
                                ? SCE_TRUE 
                                : SCE_FALSE; // 0x0000490C & 0x00004964 - 0x00004970

                            /* 
                             * Skip the operation to determine whether the battery is charging when USB charging is enabled
                             * as USB charging is disabled.
                             */
                            g_Battery.workerThreadNextOp = POWER_BATTERY_THREAD_OP_SET_REMAIN_CAP; // 0x00004980
                        }
                        else
                        {
                            /* USB charging is enabled: We need to obtain the battery charging status in the next operations. */
                            g_Battery.workerThreadNextOp = POWER_BATTERY_THREAD_OP_SET_IS_BATTERY_CHARGING_USB; // 0x00004914
                        }
                    }
                }

                /* Set content for a LoadExec exported variable. */

                // TODO: uofw's psp-fixup-imports utility currently has issues with processing variable imports.
                // Trying to fix the import for the generated power.elf file results in a segmentation fault.
                // As such, we comment out this code for now. It shouldn't cause a PSP crash, but some functionality
                // might not work as intended.

                //s32 cop0CtrlReg30 = pspCop0CtrlGet(COP0_CTRL_30); // 0x00004918 - $s3
                //if (cop0CtrlReg30 == 0 && g_unkCbInfo[1] != NULL) // 0x0000491C & 0x00004928
                //{
                //    void *pData = (void *)g_unkCbInfo[1];
                //    *(s32 *)(pData + 4) = 1; // 0x00004930
                //    *(s32 *)(pData + 8) = 0; // 0x00004934
                //}
                //
                //s32 cop0CtrlReg31 = pspCop0CtrlGet(COP0_CTRL_31); // 0x00004938 - $s4
                //if (cop0CtrlReg31 == 0 && g_unkCbInfo[0] != NULL) // 0x0000493C & 0x0000494C
                //{
                //    void *pData = (void *)g_unkCbInfo[0];
                //    *(s32 *)(pData + 4) = 1; // 0x00004958
                //    *(s32 *)(pData + 8) = 0; // 0x00004960
                //}

                continue; // 0x0000495C
            }
            case POWER_BATTERY_THREAD_OP_SET_IS_BATTERY_CHARGING_USB:
            {
                /*
                 * Set the battery charging state depdending on polestar's charge LED status. This operation
                 * is only required when the USB charging is enabled.
                 * 
                 */
                // 0x00004A14
                s16 polestarR2Val;
                status = sceSysconReadPolestarReg(2, &polestarR2Val); // 0x00004A18
                if (status < SCE_ERROR_OK) // 0x00004A20
                {
                    g_Battery.isBatteryCharging = -1; // 0x00004A9C
                    g_Battery.workerThreadNextOp = POWER_BATTERY_THREAD_OP_SET_REMAIN_CAP; // 0x00004A98 & 0x00004A7C - 0x00004A84
                }
                else
                {
                    // This code is _scePowerGetPolestar2ChargeLed(), so we are getting the Charge Led on/off status here?
                    g_Battery.isBatteryCharging = (polestarR2Val & 0x100) ? SCE_TRUE : SCE_FALSE; // 0x00004A28 - 0x00004A3C

                    if (polestarR2Val & 0x8000 || g_Battery.powerSupplyStatus & 0x40) // 0x00004A44 & 0x00004A58
                    {
                        g_Battery.workerThreadNextOp = POWER_BATTERY_THREAD_OP_SET_REMAIN_CAP; // 0x00004A84
                    }
                    else
                    {
                        g_Battery.workerThreadNextOp = POWER_BATTERY_THREAD_OP_SET_USB_STATUS; // 0x00004A6C
                        timeout = 5000000; // 0x00004A74
                    }
                }

                continue;
            }
            case POWER_BATTERY_THREAD_OP_SET_USB_STATUS:
            {
                /* Sets the USB status. This operation is only run if USB charging is enabled. */

                s32 powerSupplyStatus; // $sp + 16
                s16 polestarR2Val; // $sp + 20

                // 0x00004AA0 - 0x00004AD4
                if (sceSysconGetPowerSupplyStatus(&powerSupplyStatus) == SCE_ERROR_OK
                    && sceSysconReadPolestarReg(2, &polestarR2Val) == SCE_ERROR_OK
                    && !(polestarR2Val & 0x8000)
                    && !(powerSupplyStatus & 0x40))
                {
                    g_Battery.unk32 = 1; // 0x00004AE4
                    g_Battery.isUsbChargingEnabled = SCE_FALSE; // 0x00004AF0

                    sceSysconSetUSBStatus(SCE_SYSCON_USB_STATUS_UNKNOWN_0); // 0x00004AEC
                    _scePowerBatterySetTTC(1); // 0x00004AF4
                }

                g_Battery.workerThreadNextOp = POWER_BATTERY_THREAD_OP_SET_REMAIN_CAP; // 0x00004A84
                continue; // 0x00004AFC
            }
            case POWER_BATTERY_THREAD_OP_SET_REMAIN_CAP:
            {
                /*
                 * This operation tries to obtain the remaining battery capacity.
                 * 
                 * The remaining battery capacity can only be obtained if the equipped battery supports battery
                 * monitoring. In that case, we can poll the battery and obtain a correct value.
                 * 
                 * If, however, the equipped battery does not have monitoring capabilities, we cannot obtain
                 * its remaining battery capacity. In that case, we try to derive the remaining battery lifetime
                 * (relative to a full charge) by using the current battery voltage. Note though that this approach
                 * is for one less accurate and more importantly, potentially quite problematic. See the detailed
                 * comment below for more context.
                 */
                // 0x00004B04
                if (g_Battery.batteryType == SCE_POWER_BATTERY_TYPE_BATTERY_STATE_MONITORING_SUPPORTED) // 0x00004B0C
                {
                    /* 
                     * The equipped battery supports battery monitoring. We can directly poll the battery for its
                     * currently remaining battery capacity.
                     */

                    u32 batteryStatus;
                    s32 remainCap;
                    status = sceSysconBatteryGetStatusCap(&batteryStatus, &remainCap); // 0x00004B18
                    if (status < SCE_ERROR_OK) // 0x00004B20
                    {
                        /*
                         * There was a failure polling the battery. This could be due to the battery just being
                         * removed by the user. Invalidate collected battery data and generate a [battery not equipped]
                         * callback.
                         */

                        // 0x00004B68 - 0x00004BE0
                        _scePowerBatteryThreadErrorObtainBattInfo();
                    }
                    else
                    {
                        /* Set the remaining battery capacity. */
                        g_Battery.batteryRemainingCapacity = remainCap; // 0x00004B2C
                        g_Battery.batteryStatus = batteryStatus; // 0x00004B3C

                        /* 
                         * If the new remaining battery capacity is greater than the current ascertained
                         * minimum full capacity value, we update the munimum full capacity value. For example,
                         * this is the case if the battery is currently charging and the remaining battery capacity
                         * was smaller than the just obtained new remaining capacity when the PSP was turned on.
                         */
                        if (g_Battery.minimumFullCapacity < remainCap) // 0x00004B38
                        {
                            g_Battery.minimumFullCapacity = remainCap; // 0x00004B4C
                            if (g_Battery.powerSupplyStatus & SCE_SYSCON_POWER_SUPPLY_STATUS_BATTERY_CHARGING)
                            {
                                g_Battery.batteryRemainingCapacity = remainCap--; // 0x00004B54
                            }
                        }

                        // loc_00004B58
                        g_Battery.workerThreadNextOp = POWER_BATTERY_THREAD_OP_SET_FULL_CAP; // 0x00004B60 & 0x00004A84
                    }
                }
                else
                {
                    /* 
                     * The type of the equipped battery does not support broad battery monitoring. For example,
                     * its remaining battery capacity cannot be adequatly measured. We wll proceed with estimating
                     * the remaining battery capacity by deriving it from the battery's current voltage level.
                     * This won't be as accurate as directly measuring the remaining battery capacity and is
                     * also problematic if the estimated battery life is reported back to the UI (for
                     * example) for the user:
                     * 
                     * The problem is that under a higher load, battery voltage drops considerably whereas with the
                     * load removed, battery voltage is stabiizing and might even recover itself to the previous value
                     * before the battery load was applied. In other words, what this means is that if the user
                     * starts a game application (which creates high battery load) the voltage will drop considerably,
                     * If the user then quits their gaming session and puts their PSP system in an idle state the
                     * battery voltage might recover. This could lead to an observation where the PSP system will
                     * indicate 60% remaining battery life when playing a game but then returning to the XMB,
                     * the remaining battery life will jump back up again (to, say, 70%). If the user observes this, 
                     * they might be lead to believe that their battery actually charged itself after quitting the game
                     * while sitting idly in the XMB. Assuming that the PSP device is not currently connected to an
                     * external power source, this is obviously incorrect!
                     */

                    /*
                     * Note: The API to obtain the current battery voltage differs from the one used on PSP hardware
                     * where battery monitoring is supported. Here we call the sceSysconGetBattVolt() function which
                     * is only meant to be called for devices without battery monitoring capabilities. On devices which
                     * have such capabilities, the API sceSysconBatteryGetVolt() is called instead.
                     */

                    // loc_00004BF0
                    s32 battVolt;
                    status = sceSysconGetBattVolt(&battVolt); // 0x00004BF0
                    if (status < SCE_ERROR_OK) // 0x00004BF8
                    {
                        /*
                         * There was a failure polling the battery. This could be due to the battery just being
                         * removed by the user. Invalidate collected battery data and generate a [battery not equipped]
                         * callback.
                         */

                        // 0x00004C40 - 0x00004BE0
                        _scePowerBatteryThreadErrorObtainBattInfo();
                    }
                    else
                    {
                        /* Check if the obtained battery voltage is valid. */
                        if (battVolt != 0) // 0x00004C00
                        {
                            /* We've obtained a valid battery voltage value. */

                            g_Battery.batteryVoltage = battVolt; // 0x00004C08

                            /* 
                             * Estimate the remaining battery life (in [%]) based on the current battery voltage.
                             * Note that this is potentially problematic as stated above.
                             */
                            g_Battery.batteryLifePercentage = _scePowerBatteryConvertVoltToRCap(battVolt); // 0x00004C0C & 0x00004C20

                            /*
                             * Since the battery type does not support battery monitoring, we cannot obtain its
                             * properties like its full capacity, charge cycle or temperature. A such, we are
                             * done with polling the battery for updated values.
                             */

                            if (g_Battery.batteryAvailabilityStatus == BATTERY_AVAILABILITY_STATUS_BATTERY_IS_BEING_DETECTED) // 0x00004C1C
                            {
                                g_Battery.batteryAvailabilityStatus = BATTERY_AVAILABILITY_STATUS_BATTERY_AVAILABLE; // 0x00004C38
                            }

                            g_Battery.isBatteryInfoUpdateInProgress = SCE_FALSE; // 0x00004C24
                            g_Battery.workerThreadNextOp = POWER_BATTERY_THREAD_OP_START; // 0x00004C2C
                        }

                        /* 
                         * If the battery voltage is 0 (which would signal an incorrect voltage value), it
                         * apears we will keep reapeating this step until we have obtained a valid battery
                         * voltage value.
                         */
                    }
                }

                continue;
            }
            case POWER_BATTERY_THREAD_OP_SET_FULL_CAP:
            {
                /*
                 * Try to obtain the full capacity of the battery. This operation is only run when the equipped
                 * battery has battery monitoring capabilities.
                 */

                /* Only try to obtain the full capacity if the power service hasn't already obtained it. */
                if (g_Battery.batteryFullCapacity < 0) // 0x00004CB8
                {
                    /* Poll the battery for its full capacity. */
                    s32 fullCap;
                    status = sceSysconBatteryGetFullCap(&fullCap); // 0x00004CC8
                    if (status < SCE_ERROR_OK) // 0x00004CD0
                    {
                        /*
                         * There was a failure polling the battery. This could be due to the battery just being
                         * removed by the user. Invalidate collected battery data and generate a [battery not equipped]
                         * callback.
                         */

                        // 0x00004C40 - 0x00004BE0
                        _scePowerBatteryThreadErrorObtainBattInfo();
                    }
                    else
                    {
                        /* Battery full capacity was successfully obtained. */
                        g_Battery.workerThreadNextOp = POWER_BATTERY_THREAD_OP_SET_CHARGE_CYCLE; // 0x00004CD8
                        g_Battery.batteryFullCapacity = fullCap; // 0x00004CE4
                    }
                }
                else
                {
                    g_Battery.workerThreadNextOp = POWER_BATTERY_THREAD_OP_SET_CHARGE_CYCLE; // 0x00004CC4
                    timeout = BATTERY_EVENT_POLL_NO_WAIT; // 0x000048BC
                }

                continue;
            }
            case POWER_BATTERY_THREAD_OP_SET_CHARGE_CYCLE:
            {
                /*
                 * Try to obtain the charge cycle count of the battery. This operation is only run when the
                 * equipped battery has battery monitoring capabilities.
                 */

                 /* Only try to obtain the charge cycle count if the power service hasn't already obtained it. */
                if (g_Battery.batteryChargeCycle < 0) // 0x00004CF0
                {
                    /* Poll the battery for its charge cycle count. */
                    s32 chargeCycle;
                    status = sceSysconBatteryGetCycle(&chargeCycle); // 0x00004D00
                    if (status < SCE_ERROR_OK) // 0x00004D08
                    {
                        /*
                         * There was a failure polling the battery. This could be due to the battery just being
                         * removed by the user. Invalidate collected battery data and generate a [battery not equipped]
                         * callback.
                         */

                        // 0x00004C40 - 0x00004BE0
                        _scePowerBatteryThreadErrorObtainBattInfo();
                    }
                    else
                    {
                        /* The battery's charge cycle count was successfully obtained. */
                        g_Battery.workerThreadNextOp = POWER_BATTERY_THREAD_OP_SET_LIMIT_TIME; // 0x00004D10
                        g_Battery.batteryChargeCycle = chargeCycle; // 0x00004D1C
                    }
                }
                else
                {
                    g_Battery.workerThreadNextOp = POWER_BATTERY_THREAD_OP_SET_LIMIT_TIME; // 0x00004CF8 & 0x00004CC4
                    timeout = BATTERY_EVENT_POLL_NO_WAIT; // 0x000048C0
                }

                continue;
            }
            case POWER_BATTERY_THREAD_OP_SET_LIMIT_TIME:
            {
                /*
                 * Try to obtain the charge cycle count of the battery. This operation is only run when the
                 * equipped battery has battery monitoring capabilities.
                 */

                /*
                 * The battery's limit time (which, if set, is considered in computing the remaining battery
                 * lifetime remaining until automatic suspension) is only relevant when the PSP battery is
                 * not currently charging (over an AC adapter).
                 */
                u8 isAcSupplied = sceSysconIsAcSupplied(); // 0x00004D20
                if (!isAcSupplied) // 0x00004D28
                {
                    /*
                     * The battery is currently not charging over and AC adapter. Try to obtain the battery's
                     * limit time.
                     */
                    s32 limitTime;
                    s32 status = sceSysconBatteryGetLimitTime(&limitTime); // 0x00004D44
                    if (status < SCE_ERROR_OK) // 0x00004D4C
                    {
                        /*
                         * There was a failure polling the battery. This could be due to the battery just being
                         * removed by the user. Invalidate collected battery data and generate a [battery not equipped]
                         * callback.
                         */

                        // 0x00004D68 - 0x00004BE0
                        _scePowerBatteryThreadErrorObtainBattInfo();
                    }
                    else
                    {
                        /* The limit time was successfully obtained. */
                        g_Battery.workerThreadNextOp = POWER_BATTERY_THREAD_OP_SET_BATT_TEMP; // 0x00004D58
                        g_Battery.limitTime = limitTime; // 0x00004D64
                    }
                }
                else
                {
                    /* The battery is currently charging. Invalidate the limit time. */
                    g_Battery.workerThreadNextOp = POWER_BATTERY_THREAD_OP_SET_BATT_TEMP; // 0x00004D38
                    g_Battery.limitTime = -1; // 0x00004D40

                    timeout = BATTERY_EVENT_POLL_NO_WAIT; // 0x00004D3C & 0x000048C0
                }

                continue;
            }
            case POWER_BATTERY_THREAD_OP_SET_BATT_TEMP:
            {
                /*
                 * Try to obtain the current temperature of the battery. This operation is only run when the
                 * equipped battery has battery monitoring capabilities.
                 */

                s32 battTemp;
                status = sceSysconBatteryGetTemp(&battTemp); // 0x00004DBC
                if (status < SCE_ERROR_OK) // 0x00004DC4
                {
                    /*
                     * There was a failure polling the battery. This could be due to the battery just being
                     * removed by the user. Invalidate collected battery data and generate a [battery not equipped]
                     * callback.
                     */

                    // 0x00004D68 - 0x00004BE0
                    _scePowerBatteryThreadErrorObtainBattInfo();
                }
                else
                {
                    /* The battery temperature was obtained successfully. */
                    g_Battery.workerThreadNextOp = POWER_BATTERY_THREAD_OP_SET_BATT_ELEC; // 0x00004DD0
                    g_Battery.batteryTemp = battTemp; // 0x00004DDC
                }

                continue;
            }
            case POWER_BATTERY_THREAD_OP_SET_BATT_ELEC:
            {
                /*
                 * Try to obtain the current electric charge of the battery. This operation is only run when the
                 * equipped battery has battery monitoring capabilities.
                 */

                s32 battElec; // $sp + 52
                status = sceSysconBatteryGetElec(&battElec); // 0x00004DE0
                if (status < SCE_ERROR_OK) // 0x00004DE8
                {
                    /*
                     * There was a failure polling the battery. This could be due to the battery just being
                     * removed by the user. Invalidate collected battery data and generate a [battery not equipped]
                     * callback.
                     */

                    // 0x00004D68 - 0x00004BE0
                    _scePowerBatteryThreadErrorObtainBattInfo();
                }
                else
                {
                    /* The electric charge of the battery was successfully obtained. */
                    g_Battery.workerThreadNextOp = POWER_BATTERY_THREAD_OP_SET_BATT_VOLT; // 0x00004DF0
                    g_Battery.batteryElec = battElec; // 0x00004E00
                }

                continue;
            }
            case POWER_BATTERY_THREAD_OP_SET_BATT_VOLT:
            {
                /*
                 * Try to obtain the current voltage of the battery. This operation is only run when the
                 * equipped battery has battery monitoring capabilities.
                 */

                s32 battVolt;
                status = sceSysconBatteryGetVolt(&battVolt); // 0x00004E04
                if (status < SCE_ERROR_OK) // 0x00004E0C
                {
                    /*
                     * There was a failure polling the battery. This could be due to the battery just being
                     * removed by the user. Invalidate collected battery data and generate a [battery not equipped]
                     * callback.
                     */

                    // 0x00004D68 - 0x00004BE0
                    _scePowerBatteryThreadErrorObtainBattInfo();
                }
                else
                {
                    /* The current battery voltage was obtained successfully. */
                    g_Battery.batteryVoltage = battVolt; // 0x00004E24

                    /*
                     * If the battery availability status was set to [battery detecting] at the start of
                     * the battery polling round, we now set it to [battery available] as we have obtained
                     * a fresh set of battery data which can now be polled by applications.
                     */
                    if (g_Battery.batteryAvailabilityStatus == BATTERY_AVAILABILITY_STATUS_BATTERY_IS_BEING_DETECTED) // 0x00004E20
                    {
                        g_Battery.batteryAvailabilityStatus = BATTERY_AVAILABILITY_STATUS_BATTERY_AVAILABLE; // 0x00004E3C
                    }

                    /*
                     * We have reached the end of the final battery polling operation of this round of battery
                     * polling. As such, we are done with this round and prepare execution of the next round.
                     */
                    g_Battery.isBatteryInfoUpdateInProgress = SCE_FALSE; // 0x00004E28
                    g_Battery.workerThreadNextOp = POWER_BATTERY_THREAD_OP_START; // 0x00004E30
                }

                continue;
            }
            default:
                continue; // 0x0000488C
        }
    }
}

/*
 * Invalidates battery data collected by the power service and notifies power callback subscribers
 * that no battery is currently equipped.
 */
static inline void _scePowerBatteryThreadErrorObtainBattInfo()
{
    if (g_Battery.batteryAvailabilityStatus != BATTERY_AVAILABILITY_STATUS_BATTERY_NOT_INSTALLED) // 0x00004D70
    {
        // 0x00004D78 - 0x00004DB8
        /* Invalidate collected battery data. */
        g_Battery.batteryVoltage = -1;
        g_Battery.batteryRemainingCapacity = -1;
        g_Battery.batteryLifePercentage = -1;
        g_Battery.batteryFullCapacity = -1;
        g_Battery.batteryChargeCycle = -1;
        g_Battery.minimumFullCapacity = -1;
        g_Battery.limitTime = -1;
        g_Battery.batteryTemp = -1;
        g_Battery.batteryElec = -1;
        g_Battery.batteryAvailabilityStatus = BATTERY_AVAILABILITY_STATUS_BATTERY_NOT_INSTALLED;
        g_Battery.isBatteryCharging = SCE_FALSE;
        g_Battery.batteryStatus = 0;

        /* Inform power callback subscribers that no battery is currently eqipped. */
        _scePowerNotifyCallback(SCE_POWER_CALLBACKARG_BATTERYEXIST, 0, 0); // 0x00004DB4 & 0x00004BB0

        s32 intrState = sceKernelCpuSuspendIntr(); // 0x00004BB8

        sceKernelSetEventFlag(g_Battery.batteryEventFlagId, BATTERY_EVENT_UPDATE_BATTERY_INFO); // 0x00004BC8
        sceKernelClearEventFlag(g_Battery.batteryEventFlagId, ~BATTERY_EVENT_UPDATE_BATTERY_INFO); // 0x00004BD8

        sceKernelCpuResumeIntr(intrState); // 0x00004BE0
    }
}

// Subroutine sub_00005C08 - Address 0x00005C08
s32 _scePowerBatteryIsBusy(void)
{
    return !g_Battery.isBatteryPollingAndUsbManagementSuspended;
}

// Subroutine sub_00005130 - Address 0x00005130
// Note: Yep, that's the name used in 5.00, might have been corrected since.
/* Gets the  remaining percentage of battery life relative to the fully charged status (0-100). */
static s32 _scePowerBatteryCalcRivisedRcap(void)
{
    s32 fsCap;
    s32 rCap;
    s32 lCap;
    s32 fCap;
    s32 fCapLimit;

    fCap = g_Battery.batteryFullCapacity;
    fCapLimit = fCap * 90 / 100; // 0x0000513C - 0x00005164 -- Note: We could write 0.9 as 90 / 100 instead.
    if (fCapLimit < g_Battery.minimumFullCapacity) // 0x0000516C
    {
        fCapLimit = g_Battery.minimumFullCapacity * 95 / 100; // 0x00005174 - 0x00005194
    }

    rCap = g_Battery.batteryRemainingCapacity;
    fsCap = g_Battery.forceSuspendCapacity;
    lCap = g_Battery.lowBatteryCapacity;

    /*
    * If the remaining battery capacity is below the capacity threshold when the PSP is force suspended,
    * this remaining capacity is not available to the user as a means to run their PSP system. As such,
    * we return 0 here as the effective relatively remaining capacity.
    */
    if (rCap <= fsCap) // 0x000051A0
    {
        return 0;
    }

    /* Check if the remaining capacity is not larger than the low-battery capacity. */
    if (rCap <= lCap) // 0x000051B0
    {
        /*
         * When rCap is in (fsCap, lCap] we use the following formula to compute the relative remaining usable cap:
         *
         *    rCap - fsCap       lCap
         *  ---------------- x --------
         *    lCap - fsCap       fCap
         *
         * We thus first compute the value of the remaining usable cap relative to the max remaining usable cap
         * possible here (remember: rCap <= lCap). Once we have this ratio (which is in (0, 1]), we multiply it
         * with the ratio of lCap to fCup (i.e. lCap is 20% of fCap relatively). As such, our end value is in
         * the following range:
         *
         *   (0, lCap / fCap]
         *
         * In other words, if rCap == lCap, then the remaining usable capacity relative to full capacity is [lCap / fCap].
         * Otherwise, the remaining usable capacity is < [lCap / fCap].
         *
         *
         * Why are we using lCap here? If rCap is == lCap, then we get the correct
         * relative value for rCap / fCap (i.e. if lCap=rCap=400mAh and fCap=1800mAh then we return 22mAh).  From there, we
         * get smaller values until we reach [0.XYZ....]. rCap might still be > fsCap (like rCap=85mAh and fsCap=80mAh) but
         * at that point the PSP is about to force suspend so the effectively remaining _relative_ capacity for running the PSP
         * is 0.XYZ which is rounded to 0.
         */

         /*
          * Implementation details: We slightly change the order of operations here to make sure that when we devide two values
          * our numerator is always >= the denominator so we don't have to deal with floating point values in (0, 1). Instead,
          * we only use integer arithmetic here. Below you can see the out commented implementation if floating point arithmetic
          * were to be used.
          */

          //float actUsableRCapMaxUsableRCapRatio = (remainCap - forceSuspendCap) / (lowCap - forceSuspendCap);
          //float actUsableRCapFullCapRatio = actUsableRCapMaxUsableRCapRatio * (lowCap / fullCap);

          ///* Make sure return value is in [0, 100] since we return the relatively remaining capacity in percent. */
          //return pspMax(0, pspMin(actUsableRCapFullCapRatio * 100, 100));

        if ((lCap - fsCap) == 0) // 0x000051C8
        {
            pspBreak(SCE_BREAKCODE_DIVZERO); // 0x000051CC
        }

        u32 relRCap = ((rCap - fsCap) * lCap) / (lCap - fsCap); // 0x000051B8 & 0x000051BC & 0x000051C0 & 0x000051D4
        relRCap = (relRCap * 100) / fCap; // 0x000051DC & 0x000051E4

        return pspMax(0, pspMin(relRCap, 100));
    }
    else
    {
        /*
         * The remaining battery capacity is above the force suspend threshold and larger than the
         * low-battery capacity.
         */

         // TODO: Not exactly sure yet what fCapLimit is about...
        if (fCapLimit == 0) // 0x0000520C
        {
            pspBreak(SCE_BREAKCODE_DIVZERO); // 0x00005210
        }

        u32 relRCap = (rCap * 100) / fCapLimit; // 0x00005208 & 0x00005214 & 0x00005220
        u32 lowerBound = (lCap * 100) / fCap; // 0x00005218 & 0x0000521C & 0x00005228 & 0x0000522C

        // effectively val2 = pspMax(val2, lowerBound);
        if (relRCap < lowerBound) // 0x00005234
        {
            if (fCap == 0) // 0x51000001
            {
                pspBreak(SCE_BREAKCODE_DIVZERO); // 0x00005240
            }
            relRCap = lowerBound; // 0x00005244 & 0x000051E8
        }

        return pspMax(0, pspMin(relRCap, 100)); // 0x00005244 & 0x000051EC & 0x00005204 & 0x000051F4
    }
}

#define BATTERY_VOLTAGE_REMAINING_RELATIVE_BATTERY_CAPACITY_0_THRESHOLD_1   3100 /* 3.10V */
#define BATTERY_VOLTAGE_REMAINING_RELATIVE_BATTERY_CAPACITY_0_THRESHOLD_2   3300 /* 3.30V */
#define BATTERY_VOLTAGE_REMAINING_RELATIVE_BATTERY_CAPACITY_0_THRESHOLD_3   3400 /* 3.40V */

#define BATTERY_VOLTAGE_REMAINING_RELATIVE_BATTERY_CAPACITY_2_THRESHOLD     3500 /* 3.50V */

#define BATTERY_VOLTAGE_REMAINING_RELATIVE_BATTERY_CAPACITY_4_THRESHOLD     3600 /* 3.60V */

#define BATTERY_VOLTAGE_REMAINING_RELATIVE_BATTERY_CAPACITY_12_THRESHOLD    3700 /* 3.70V */

#define BATTERY_VOLTAGE_REMAINING_RELATIVE_BATTERY_CAPACITY_30_THRESHOLD    3800 /* 3.80V */

#define BATTERY_VOLTAGE_REMAINING_RELATIVE_BATTERY_CAPACITY_60_THRESHOLD    4000 /* 4.00V */

#define BATTERY_VOLTAGE_REMAINING_RELATIVE_BATTERY_CAPACITY_90_THRESHOLD_1  4050 /* 4.05V */
#define BATTERY_VOLTAGE_REMAINING_RELATIVE_BATTERY_CAPACITY_90_THRESHOLD_2  4150 /* 4.15V */

// Subroutine sub_0000524C - Address 0x0000524C
/* 
 * Convert battery voltage to remaining battery capacity (relative). The conversion operates on
 * hardcoded values and lacks some accuracy.
 */
static s32 _scePowerBatteryConvertVoltToRCap(s32 voltage)
{
    /* 
     * This function returns an estimated remaining battery capacity related to a full charge
     * based on the specified battery voltage. It is used to for PSP devices which are equipped
     * with a battery which does not have battery monitoring capabilties.
     * 
     * The maximum charge voltage of such a battery (like the Li-ion battery equipped in the PSP Go)
     * is 4.25 V or 4250 mV. For the battery manufactured specifically for the PSP, the following
     * relation between current battery voltage and remaining battery capacity is used:
     * 
     * 100% --- >= 4.15V
     * 90%  --- >= 4.00V
     * 60%  --- >= 3.80V
     * 30%  --- >= 3.70V
     * 12%  --- >= 3.60V
     *  4%  --- >= 3.50V
     *  2%  --- >= 3.40V
     */

    /* If an invalid voltage is specified, we return 0. */
    if (voltage <= 0)
    {
        return 0;
    }

    /* Battery voltage threshold values for 0% remaining battery capacity. */
    if (voltage < BATTERY_VOLTAGE_REMAINING_RELATIVE_BATTERY_CAPACITY_0_THRESHOLD_1 
        || voltage < BATTERY_VOLTAGE_REMAINING_RELATIVE_BATTERY_CAPACITY_0_THRESHOLD_2
        || voltage < BATTERY_VOLTAGE_REMAINING_RELATIVE_BATTERY_CAPACITY_0_THRESHOLD_3) // 0x0000524C - 0x00005268
    {
        return 0;
    }

    /* Battery voltage threshold value for 2% remaining battery capacity. */
    if (voltage < BATTERY_VOLTAGE_REMAINING_RELATIVE_BATTERY_CAPACITY_2_THRESHOLD) // 0x00005270
    {
        return 2;
    }

    /* Battery voltage threshold value for 4% remaining battery capacity. */
    if (voltage < BATTERY_VOLTAGE_REMAINING_RELATIVE_BATTERY_CAPACITY_4_THRESHOLD) // 0x0000527C
    {
        return 4;
    }
    
    /* Battery voltage threshold value for 12% remaining battery capacity. */
    if (voltage < BATTERY_VOLTAGE_REMAINING_RELATIVE_BATTERY_CAPACITY_12_THRESHOLD) // 0x00005288
    {
        return 12;
    }

    /* Battery voltage threshold value for 30% remaining battery capacity. */
    if (voltage < BATTERY_VOLTAGE_REMAINING_RELATIVE_BATTERY_CAPACITY_30_THRESHOLD) // 0x00005294
    {
        return 30;
    }

    /* Battery voltage threshold value for 60% remaining battery capacity. */
    if (voltage < BATTERY_VOLTAGE_REMAINING_RELATIVE_BATTERY_CAPACITY_60_THRESHOLD) // 0x000052A0
    {
        return 60;
    }

    /* Battery voltage threshold values for 90% remaining battery capacity. */
    if (voltage < BATTERY_VOLTAGE_REMAINING_RELATIVE_BATTERY_CAPACITY_90_THRESHOLD_1 
        || voltage < BATTERY_VOLTAGE_REMAINING_RELATIVE_BATTERY_CAPACITY_90_THRESHOLD_2) // 0x000052AC
    {
        return 90;
    }

    /* Battery voltage is high enough for 100% remaining battery capacity. */
    return 100;
}

// Subroutine scePower_driver_10CE273F - Address 0x000052D4
s32 scePowerBatteryForbidCharging(void)
{
    s32 intrState1;
    s32 intrState2;

    intrState1 = sceKernelCpuSuspendIntr();  // 0x000052EC

    /* 
     * Check if battery charging is currently permitted. We only need to proceed if battery
     * charging is currenly permitted.
     */
    if (g_Battery.batteryForbidChargingNetCounter == 0) // 0x000052F8
    {
        /* 
         * The battery can currently be charged. Check if a scheduled [permit battery charging] operation
         * is currently in the system. If it is, cancel that operation.
         */
        if (g_Battery.permitChargingDelayAlarmId > 0) // 0x00005304
        {
            sceKernelCancelAlarm(g_Battery.permitChargingDelayAlarmId); // 0x0000530C
            g_Battery.permitChargingDelayAlarmId = 0; // 0x00005318
        }

        /* Remove a lined up [permit battery charge] in the battery worker thread if once exists. */
        sceKernelClearEventFlag(g_Battery.batteryEventFlagId, ~BATTERY_EVENT_PERMIT_BATTERY_CHARGING); // 0x00005320

        /* Comamnd the battery worker thread to forbid battery charging. */
        sceKernelSetEventFlag(g_Battery.batteryEventFlagId, BATTERY_EVENT_FORBID_BATTERY_CHARGING); // 0x0000532C
    }

    /* Increase the net counter. */
    g_Battery.batteryForbidChargingNetCounter++; // 0x00005344

    intrState2 = sceKernelCpuSuspendIntr(); // 0x00005340

    /* Command the battery worker thread to update battery info (poll the battery). */
    sceKernelSetEventFlag(g_Battery.batteryEventFlagId, BATTERY_EVENT_UPDATE_BATTERY_INFO); // 0x00005350
    sceKernelClearEventFlag(g_Battery.batteryEventFlagId, ~BATTERY_EVENT_UPDATE_BATTERY_INFO); // 0x00005360

    sceKernelCpuResumeIntr(intrState2); // 0x00005368
    sceKernelCpuResumeIntr(intrState1); // 0x00005370

    return SCE_ERROR_OK;
}

// Subroutine scePower_driver_EF751B4A - Address 0x00005394 
s32 scePowerBatteryPermitCharging(void)
{
    s32 intrState1;
    s32 intrState2;

    intrState1 = sceKernelCpuSuspendIntr(); // 0x000053AC

    /*
     * Check if battery charging is currently forbidden. We only need to proceed if battery
     * charging is currently forbidden.
     */
    if (g_Battery.batteryForbidChargingNetCounter > 0)
    {
        /* The battery is currently not allowed to charge. */

        /* Decrease the net counter. */
        g_Battery.batteryForbidChargingNetCounter--; // 0x000053CC

        /* 
         * If a [permit charge] request has been sent as often as a [forbid charge] request, we will now
         * proceed with allowing the battery to charge again.
         */
        if (g_Battery.batteryForbidChargingNetCounter == 0) // 0x000053C8
        {
            sceKernelClearEventFlag(g_Battery.batteryEventFlagId, ~BATTERY_EVENT_FORBID_BATTERY_CHARGING); // 0x00005424
            g_Battery.permitChargingDelayAlarmId = sceKernelSetAlarm(POWER_DELAY_PERMIT_CHARGING, _scePowerBatteryDelayedPermitCharging, NULL); // 0x0000543C
        }
    }

    intrState2 = sceKernelCpuSuspendIntr(); // 0x000053D0

    /* Command the battery worker thread to poll the battery. */
    sceKernelSetEventFlag(g_Battery.batteryEventFlagId, BATTERY_EVENT_UPDATE_BATTERY_INFO); // 0x000053E0
    sceKernelClearEventFlag(g_Battery.batteryEventFlagId, ~BATTERY_EVENT_UPDATE_BATTERY_INFO); // 0x000053F0

    sceKernelCpuResumeIntr(intrState2); // 0x000053F8
    sceKernelCpuResumeIntr(intrState1); // 0x00005400

    return SCE_ERROR_OK;
}

// Subroutine sub_0000544C - Address 0x0000544C
s32 _scePowerBatteryUpdateAcSupply(s32 isAcSupplied)
{
    s32 intrState;

    if (!isAcSupplied) // 0x00005464
    {
        return SCE_ERROR_OK;
    }

    intrState = sceKernelCpuSuspendIntr(); // 0x00005488

    if (g_Battery.batteryForbidChargingNetCounter > 0) // 0x00005494
    {
        if (g_Battery.permitChargingDelayAlarmId > 0) // 0x000054A0
        {
            sceKernelCancelAlarm(g_Battery.permitChargingDelayAlarmId); // 0x000054AC
            g_Battery.permitChargingDelayAlarmId = -1; // 0x000054B4
        }

        sceKernelClearEventFlag(g_Battery.batteryEventFlagId, ~BATTERY_EVENT_PERMIT_BATTERY_CHARGING); // 0x000054BC
        sceKernelSetEventFlag(g_Battery.batteryEventFlagId, BATTERY_EVENT_FORBID_BATTERY_CHARGING);  // 0x000054C8
    }

    sceKernelCpuResumeIntr(intrState);
    return SCE_ERROR_OK;
}

// Subroutine scePower_driver_5F5006D2 - Address 0x000052C8
s32 scePowerGetUsbChargingCapability(void)
{
    return (s32)g_Battery.isUsbChargingSupported;
}

// Subroutine scePower_driver_72D1B53A - Address 0x000054E0
s32 scePowerBatteryEnableUsbCharging(void)
{
    s32 prevIsUsbEnabled;
    s32 intrState;

    /* Only enable USB charging on PSP devices which actually support USB charging. */
    if (!g_Battery.isUsbChargingSupported) // 0x00005508
    {
        return SCE_ERROR_NOT_SUPPORTED;
    }

    /* Check if our battery worker thread can currently enable USB charging. */
    if (g_Battery.isBatteryPollingAndUsbManagementSuspended) // 0x00005514
    {
        return SCE_ERROR_OK;
    }

    intrState = sceKernelCpuSuspendIntr(); // 0x0000553C

    /* Obtain the previous status. */
    prevIsUsbEnabled = g_Battery.isUsbChargingEnabled;

    /* Only proceed if USB charging is not already enabled. */
    if (!g_Battery.isUsbChargingEnabled) // 0x00005548
    {
        g_Battery.isUsbChargingEnabled = SCE_TRUE; // 0x00005568

        /* 
         * Charging via an AC adapter takes precedence over USB charging. As such, only proceed with
         * actually enabling USB charging when the PSP system is not currently connected to an external
         * power source via an AC adapter.
         */
        if (!sceSysconIsAcSupplied()) // 0x00005564
        {
            /* Line up an [enable USB charging] command for our battery worker thread to execute. */
            sceKernelSetEventFlag(g_Battery.batteryEventFlagId, BATTERY_EVENT_ENABLE_USB_CHARGING); // 0x00005574

            sceKernelClearEventFlag(g_Battery.batteryEventFlagId, ~BATTERY_EVENT_UNKNOWN_400); // 0x00005580
        }
    }

    sceKernelCpuResumeIntr(intrState); // 0x00005550
    return prevIsUsbEnabled;
}

// Subroutine scePower_driver_7EAA4247 - Address 0x00005590
s32 scePowerBatteryDisableUsbCharging(void)
{
    s32 status;
    s32 intrState1;
    s32 intrState2;

    /* Only disable USB charging on PSP devices which actually support USB charging. */
    if (!g_Battery.isUsbChargingSupported) // 0x000055B4
    {
        return SCE_ERROR_NOT_SUPPORTED;
    }

    intrState1 = sceKernelCpuSuspendIntr(); // 0x000055BC

    /* Only proceed if USB charging is currently enabled. */
    if (g_Battery.isUsbChargingEnabled) // 0x000055C8
    {
        g_Battery.isUsbChargingEnabled = SCE_FALSE; // 0x000055E8

        /* Check if SYSCON is already disabling USB charging right now. */
        if (g_Battery.sysconCmdDisableUsbChargingExecStatus == BATTERY_SYSCON_CMD_DISABLE_USB_CHARGING_EXEC_STATUS_READY) // 0x000055EC
        {
            /* SYSCON is not currently disabling USB charging -> tell it to disable USB charging now. */
            g_Battery.powerBatterySysconPacket.tx[PSP_SYSCON_TX_CMD] = PSP_SYSCON_CMD_SET_USB_STATUS; // 0x0000567C
            g_Battery.powerBatterySysconPacket.tx[PSP_SYSCON_TX_LEN] = 3; // 0x00005680
            g_Battery.powerBatterySysconPacket.tx[PSP_SYSCON_TX_DATA(0)] = SCE_SYSCON_USB_STATUS_CHARGING_DISABLED; // 0x00005688

            status = sceSysconCmdExecAsync(&g_Battery.powerBatterySysconPacket, 1, _scePowerBatterySysconCmdIntr, NULL); // 0x00005684
            if (status < SCE_ERROR_OK) // 0x0000568C
            {
                /* Our command to disable USB charging was not started being executed successfully. */
                sceKernelSetEventFlag(g_Battery.batteryEventFlagId, BATTERY_EVENT_UNKNOWN_400); // 0x00005660
            }
            else
            {
                /* SYSCON successfully started executing the command to disable USB charging. */
                g_Battery.sysconCmdDisableUsbChargingExecStatus = BATTERY_SYSCON_CMD_DISABLE_USB_CHARGING_EXEC_STATUS_BUSY; // 0x00005698
                g_Battery.unk32 = 0; // 0x000056A0
            }
        }
        else
        {
            /* SYSCON is currently busy disabling USB charging (from a previous call). */
            sceKernelSetEventFlag(g_Battery.batteryEventFlagId, BATTERY_EVENT_UNKNOWN_400); // 0x00005660
        }

        /* Remove a lined-up [enable USB charging] command from our battery worker thread (if one exists). */
        sceKernelClearEventFlag(g_Battery.batteryEventFlagId, ~BATTERY_EVENT_ENABLE_USB_CHARGING); // 0x000055FC
    }

    intrState2 = sceKernelCpuSuspendIntr(); // 0x00005604

    /* Request the battery worker thread to update the battery info. */
    sceKernelSetEventFlag(g_Battery.batteryEventFlagId, BATTERY_EVENT_UPDATE_BATTERY_INFO);  // 0x00005614
    sceKernelClearEventFlag(g_Battery.batteryEventFlagId, ~BATTERY_EVENT_UPDATE_BATTERY_INFO); // 0x00005624

    sceKernelCpuResumeIntr(intrState2); // 0x0000562C
    sceKernelCpuResumeIntr(intrState1); // 0x00005634

    return SCE_ERROR_OK;
}

// Subroutine scePower_B4432BC8 - Address 0x00005774 - Aliases: scePower_driver_67492C52
s32 scePowerGetBatteryChargingStatus(void)
{
    s32 oldK1;
    s32 batteryChargingStatus;

    oldK1 = pspShiftK1(); // 0x000057A0

    /* If no battery is equipped we return with an error. */
    if (g_Battery.batteryAvailabilityStatus == BATTERY_AVAILABILITY_STATUS_BATTERY_NOT_INSTALLED) // 0x0000579C
    {
        pspSetK1(oldK1);
        return SCE_POWER_ERROR_NO_BATTERY;
    }

    /* If the power service is currently busy detecting the battery status we return with an error. */
    if (g_Battery.batteryAvailabilityStatus == BATTERY_AVAILABILITY_STATUS_BATTERY_IS_BEING_DETECTED) // 0x000057AC
    {
        pspSetK1(oldK1);
        return SCE_POWER_ERROR_DETECTING;
    }

    /* 
     * Each PSP system cna be charged by connecting it to an external power source with an AC adapter.
     * In addition, starting with the PSP-2000 series, a PSP device can also be charged over the USB cable.
     */

    /* Check if the PSP system is connected to an external power source with an AC adapter. */
    if (sceSysconIsAcSupplied()) // 0x000057BC
    {
        /* We are connected via an AC adapter. Check if battery charging is forbidden. */
        if (g_Battery.batteryForbidChargingNetCounter != 0) // 0x000057C8
        {
            /* Battery charging is forbidden. */
            batteryChargingStatus = SCE_POWER_BATTERY_CHARGING_STATUS_CHARGING_FORBIDDEN; // 0x000057CC
        }
        else
        {
            /* Battery charging is not forbidden. Obtain the battery charging status. */
            batteryChargingStatus = g_Battery.powerSupplyStatus & SCE_SYSCON_POWER_SUPPLY_STATUS_BATTERY_CHARGING; // 0x000057D0 & 0x000057D4
        }
    }
    else
    {
        /* The PSP device is not connected via an AC adapter to an external power source. */

        // TODO: Figure out what is being checked here.
        if (g_Battery.unk32 != 0) // 0x000057F4
        {
            batteryChargingStatus = SCE_POWER_BATTERY_CHARGING_STATUS_NOT_CHARGING_UNKNOWN_STATUS_3; // 0x000057F8
        }
        else if (!g_Battery.isUsbChargingEnabled || !(g_Battery.powerSupplyStatus & 0x40) || !g_Battery.isBatteryCharging) // 0x00005800 & 0x00005810 & 0x0000581C
        {
            /* If the PSP system is not charging over USB, the battery is not charging. */
            batteryChargingStatus = SCE_POWER_BATTERY_CHARGING_STATUS_NOT_CHARGING; // 0x00005804
        }
        else
        {
            /* The battery is charging over USB. */
            batteryChargingStatus = SCE_POWER_BATTERY_CHARGING_STATUS_CHARGING; // 0x00005820
        }
    }

    pspSetK1(oldK1);
    return batteryChargingStatus;
}

/*
 * Defines the battery voltage threshold value at which the PSP system should automatically suspend.
 * This voltage value currently maps to a remaining relative battery capacity of 0%.
 */
#define BATTERY_VOLTAGE_FORCE_SUSPEND_THRESHOLD     BATTERY_VOLTAGE_REMAINING_RELATIVE_BATTERY_CAPACITY_0_THRESHOLD_3

// Subroutine scePower_78A1A796 - Address 0x0000582C - Aliases: scePower_driver_88C79735
s32 scePowerIsSuspendRequired(void)
{
    s32 isSuspendRequired;
    s32 oldK1;

    oldK1 = pspShiftK1(); // 0x00005838

    /* 
     * No suspension required if the PSP system is currently connected to an external power source
     * with an AC adapter.
     */
    if (sceSysconIsAcSupplied()) // 0x00005850
    {
        pspSetK1(oldK1);
        return SCE_FALSE; // 0x000058D8
    }

    /* Check if the equipped battery supports battery monitoring. */
    if (g_Battery.batteryType == SCE_POWER_BATTERY_TYPE_BATTERY_STATE_MONITORING_SUPPORTED) // 0x0000585C
    {
        /*
         * As the equipped battery supports battery monitoring, we can obtain its currently remaining battery
         * capacity and use that to derive whether or not PSP system suspension is required.
         */
        s32 remainingCapacity = scePowerGetBatteryRemainCapacity(); // 0x000058B8
        if (remainingCapacity <= 0) // 0x000058C0
        {
            /* If the remaining battery capacity could not be estimated, no suspension is required. */
            isSuspendRequired = SCE_FALSE; // 0x000058C4
        }
        else
        {
            /* 
             * If the remaining battery capacity is less than the specified force-suspend capacity, suspending the
             * PSP system is required.
             */
            isSuspendRequired = remainingCapacity < g_Battery.forceSuspendCapacity; // 0x000058C8 & 0x000058D0
        }
    }

    if (g_Battery.batteryType == SCE_POWER_BATTERY_TYPE_BATTERY_STATE_MONITORING_NOT_SUPPORTED) // 0x00005864
    {
        /* 
         * If the equipped battery does not support battery monitoring, we need to obtain the suspension-required
         * status by asking SYSCON or by estimating based on the current battery voltage.
         */

        u8 baryonStatus2 = sceSysconGetBaryonStatus2(); // 0x0000588C
        if (baryonStatus2 & SCE_SYSCON_BARYON_STATUS2_IS_LOW_BATTERY) // 0x00005898
        {
            isSuspendRequired = SCE_TRUE; // 0x00005890
        }
        else
        {
            /* Estimate the need to suspend the PSP system based on the current battery voltage. */
            isSuspendRequired = g_Battery.batteryVoltage >= 0 
                && g_Battery.batteryVoltage < BATTERY_VOLTAGE_FORCE_SUSPEND_THRESHOLD; // 0x0000589C - 0x000058B4
        }
    }

    pspSetK1(oldK1);

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wmaybe-uninitialized"

    // uofw note: if g_Battery.batteryType is neither 0 or 1, isSuspendRequired is unitialized.
    // Normally, it should only be set to either 0 or 1 though.
    return isSuspendRequired;

#pragma GCC diagnostic pop
}

// Subroutine scePower_94F5A53F - Address 0x000058DC - Aliases: scePower_driver_41ADFF48
s32 scePowerGetBatteryRemainCapacity(void)
{
    if (g_Battery.batteryAvailabilityStatus == BATTERY_AVAILABILITY_STATUS_BATTERY_NOT_INSTALLED) // 0x000058EC
    {
        return SCE_POWER_ERROR_NO_BATTERY;
    }

    if (g_Battery.batteryAvailabilityStatus == BATTERY_AVAILABILITY_STATUS_BATTERY_IS_BEING_DETECTED) // 0x000058FC
    {
        return SCE_POWER_ERROR_DETECTING;
    }

    return g_Battery.batteryRemainingCapacity; // 0x00005904
}

// Subroutine scePower_8EFB3FA2 - Address 0x00005910 - Aliases: scePower_driver_C79F9157
s32 scePowerGetBatteryLifeTime(void)
{
    s32 status;
    s32 intrState;

    /* If an external power supply is available, return 0 since we cannot estimate the remaining battery life. */
    if (sceSysconIsAcSupplied()) // 0x00005920 & 0x00005928
    {
        return 0;
    }

    if (g_Battery.batteryType != SCE_POWER_BATTERY_TYPE_BATTERY_STATE_MONITORING_SUPPORTED) // 0x0000593C
    {
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wmaybe-uninitialized"

        // uofw note: Yes, Sony is returning an unitialized local variable here
        // Presumably status should have been set to SCE_ERROR_NOT_SUPPORTED before 
        // returning here.
        return status;

#pragma GCC diagnostic pop
    }

    /* Get the remaining battery capacity. */
    s32 remainingCapacity = scePowerGetBatteryRemainCapacity(); // 0x00005944
    if (remainingCapacity < 0) // 0x00005950
    {
        return remainingCapacity; // 0x00005954
    }

    if (!g_Battery.isUsbChargingEnabled && g_Battery.batteryElec >= 0) // 0x0000596C & 0x000059EC
    {
        intrState = sceKernelCpuSuspendIntr(); // 0x000059F4

        sceKernelSetEventFlag(g_Battery.batteryEventFlagId, BATTERY_EVENT_UPDATE_BATTERY_INFO); // 0x00005A04
        sceKernelClearEventFlag(g_Battery.batteryEventFlagId, ~BATTERY_EVENT_UPDATE_BATTERY_INFO); // 0x00005A14

        sceKernelCpuResumeIntr(intrState);
        return SCE_POWER_ERROR_DETECTING;
    }

    if (g_Battery.batteryElec < 0) // 0x00005978
    {
        /* Get the remaining battery capacity actually available to the user. */
        s32 remainingCapacityForRunning = (remainingCapacity - g_Battery.forceSuspendCapacity); // 0x000059A8 & 0x000059AC

        if (g_Battery.batteryElec == -1) // 0x000059C0
        {
            pspBreak(SCE_BREAKCODE_DIVZERO); // 0x000059C4
        }

        /* 
         * Convert remaining capacity in mAh into minutes. Rough formula to use here:
         * (mAh)/(Amps*1000)*60 = (minutes).
         */
        s32 cvtRemainingTime = (remainingCapacityForRunning * 60) / ~g_Battery.batteryElec; // 0x000059B0 & 0x000059B4 & 0x000059B4

        /* Consider the explicitely set limit time. */
        if (g_Battery.limitTime >= 0) // 0x000059CC & 0x000059D
        {
            cvtRemainingTime = pspMin(cvtRemainingTime, g_Battery.limitTime); // 0x000059D8
        }

        return pspMax(cvtRemainingTime, 1); // 0x000059E0
    }

    return 0; /* Remaining battery lifetime cannot be estimated. */
}

// Subroutine scePower_28E12023 - Address 0x00005A30 - Aliases: scePower_driver_40870DAC
s32 scePowerGetBatteryTemp(void)
{
    if (g_Battery.batteryType != SCE_POWER_BATTERY_TYPE_BATTERY_STATE_MONITORING_SUPPORTED) // 0x00005A40
    {
        return SCE_ERROR_NOT_SUPPORTED;
    }

    if (g_Battery.batteryAvailabilityStatus == BATTERY_AVAILABILITY_STATUS_BATTERY_NOT_INSTALLED) // 0x00005A50
    {
        return SCE_POWER_ERROR_NO_BATTERY;
    }

    if (g_Battery.batteryAvailabilityStatus == BATTERY_AVAILABILITY_STATUS_BATTERY_IS_BEING_DETECTED) // 0x00005A60
    {
        return SCE_POWER_ERROR_DETECTING;
    }

    return g_Battery.batteryTemp; // 0x00005A68
}

// Subroutine scePower_862AE1A6 - Address 0x00005A74 - Aliases: scePower_driver_993B8C4A
s32 scePowerGetBatteryElec(u32 *pBatteryElec)
{
    s32 oldK1;

    oldK1 = pspShiftK1(); // 0x00005A80

    if (g_Battery.batteryType != SCE_POWER_BATTERY_TYPE_BATTERY_STATE_MONITORING_SUPPORTED) // 0x00005A90
    {
        return SCE_ERROR_NOT_SUPPORTED;
    }

    /* Verify that a valid memory address was specified. */
    if (!pspK1PtrOk(pBatteryElec)) // 0x00005A98
    {
        return SCE_ERROR_PRIV_REQUIRED;
    }

    if (g_Battery.batteryAvailabilityStatus == BATTERY_AVAILABILITY_STATUS_BATTERY_NOT_INSTALLED) // 0x00005AA8
    {
        return SCE_POWER_ERROR_NO_BATTERY;
    }

    if (g_Battery.batteryAvailabilityStatus == BATTERY_AVAILABILITY_STATUS_BATTERY_IS_BEING_DETECTED) // 0x00005AB8
    {
        return SCE_POWER_ERROR_DETECTING;
    }

    // uOFW note: Missing null check
    *pBatteryElec = g_Battery.batteryElec; // 0x00005AC8

    pspSetK1(oldK1);
    return SCE_ERROR_OK;
}

// Subroutine scePower_CB49F5CE - Address 0x00005AD8 - Aliases: scePower_driver_8432901E
s32 scePowerGetBatteryChargeCycle(void)
{
    if (g_Battery.batteryType != SCE_POWER_BATTERY_TYPE_BATTERY_STATE_MONITORING_SUPPORTED) // 0x00005AE8
    {
        return SCE_ERROR_NOT_SUPPORTED;
    }

    if (g_Battery.batteryAvailabilityStatus == BATTERY_AVAILABILITY_STATUS_BATTERY_NOT_INSTALLED) // 0x00005AF8
    {
        return SCE_POWER_ERROR_NO_BATTERY;
    }

    if (g_Battery.batteryAvailabilityStatus == BATTERY_AVAILABILITY_STATUS_BATTERY_IS_BEING_DETECTED) // 0x00005B08
    {
        return SCE_POWER_ERROR_DETECTING;
    }

    return g_Battery.batteryChargeCycle; // 0x00005B10
}

// Subroutine scePower_27F3292C - Address 0x00005CCC - Aliases: scePower_driver_0DA940D2
s32 scePowerBatteryUpdateInfo(void)
{
    s32 intrState;

    intrState = sceKernelCpuSuspendIntr(); // 0x00005CDC

    /* Poll the battery again and refresh power service's collected battery data. */
    sceKernelSetEventFlag(g_Battery.batteryEventFlagId, BATTERY_EVENT_UPDATE_BATTERY_INFO); // 0x00005CEC
    sceKernelClearEventFlag(g_Battery.batteryEventFlagId, ~BATTERY_EVENT_UPDATE_BATTERY_INFO); // 0x00005CFC

    sceKernelCpuResumeIntr(intrState); // 0x00005D04
    return SCE_ERROR_OK;
}

// Subroutine scePower_E8E4E204 - Address 0x00005D24 - Aliases: scePower_driver_A641CF3F
s32 scePowerGetForceSuspendCapacity(void)
{
    return (s32)g_Battery.forceSuspendCapacity;
}

// Subroutine scePower_B999184C - Address 0x00005D30 - Aliases: scePower_driver_7B908CAA
s32 scePowerGetLowBatteryCapacity(void)
{
    return (s32)g_Battery.lowBatteryCapacity;
}

// Subroutine scePower_87440F5E - Address 0x00005D3C - Aliases: scePower_driver_872F4ECE
s32 scePowerIsPowerOnline(void)
{
    s32 status;
    s32 oldK1;

    oldK1 = pspShiftK1();

    status = sceSysconIsAcSupplied(); // 0x00005D4C

    pspSetK1(oldK1);
    return (s32)status;
}

// Subroutine scePower_0AFD0D8B - Address 0x00005D68 - Aliases: scePower_driver_8C873AA7
s32 scePowerIsBatteryExist(void)
{
    return (s32)(g_Battery.batteryAvailabilityStatus != BATTERY_AVAILABILITY_STATUS_BATTERY_NOT_INSTALLED);
}

// Subroutine scePower_1E490401 - Address 0x00005D78 - Aliases: scePower_driver_7A9EA6DE
s32 scePowerIsBatteryCharging(void)
{
    s32 status;

    status = scePowerGetBatteryChargingStatus(); // 0x00005D80

    if (status == SCE_POWER_BATTERY_CHARGING_STATUS_CHARGING_FORBIDDEN 
        || status == SCE_POWER_BATTERY_CHARGING_STATUS_NOT_CHARGING_UNKNOWN_STATUS_3) // 0x00005D88 & 0x00005D90
    {
        status = SCE_FALSE; // 0x00005D94
    }

    return status;
}

// Subroutine scePower_D3075926 - Address 0x00005DA0 - Aliases: scePower_driver_FA651CE1
s32 scePowerIsLowBattery(void)
{
    s32 status;
    s32 oldK1;

    status = SCE_FALSE;
    oldK1 = pspShiftK1(); // 0x00005DC4

    if (g_Battery.batteryAvailabilityStatus == BATTERY_AVAILABILITY_STATUS_BATTERY_AVAILABLE) // 0x00005DC0
    {
        status = sceSysconIsLowBattery(); // 0x00005DE0
    }

    pspSetK1(oldK1);
    return status;
}

// Subroutine scePower_driver_071160B1 - Address 0x00005DF0
s32 scePowerGetBatteryType(void)
{
    return (s32)g_Battery.batteryType;
}

// Subroutine scePower_FD18A0FF - Address 0x00005DFC - Aliases: scePower_driver_003B1E03
s32 scePowerGetBatteryFullCapacity(void)
{
    if (g_Battery.batteryAvailabilityStatus == BATTERY_AVAILABILITY_STATUS_BATTERY_NOT_INSTALLED)
    {
        return SCE_POWER_ERROR_NO_BATTERY;
    }

    if (g_Battery.batteryAvailabilityStatus == BATTERY_AVAILABILITY_STATUS_BATTERY_IS_BEING_DETECTED)
    {
        return SCE_POWER_ERROR_DETECTING;
    }

    return g_Battery.batteryFullCapacity;
}

// Subroutine scePower_2085D15D - Address 0x00005E30 - Aliases: scePower_driver_31AEA94C
s32 scePowerGetBatteryLifePercent(void)
{
    if (g_Battery.batteryAvailabilityStatus == BATTERY_AVAILABILITY_STATUS_BATTERY_NOT_INSTALLED)
    {
        return SCE_POWER_ERROR_NO_BATTERY;
    }

    if (g_Battery.batteryAvailabilityStatus == BATTERY_AVAILABILITY_STATUS_BATTERY_IS_BEING_DETECTED)
    {
        return SCE_POWER_ERROR_DETECTING;
    }

    return g_Battery.batteryLifePercentage;
}

// Subroutine scePower_483CE86B - Address 0x00005E64 - Aliases: scePower_driver_F7DE0E81
s32 scePowerGetBatteryVolt(void)
{
    if (g_Battery.batteryAvailabilityStatus == BATTERY_AVAILABILITY_STATUS_BATTERY_NOT_INSTALLED)
    {
        return SCE_POWER_ERROR_NO_BATTERY;
    }

    if (g_Battery.batteryAvailabilityStatus == BATTERY_AVAILABILITY_STATUS_BATTERY_IS_BEING_DETECTED)
    {
        return SCE_POWER_ERROR_DETECTING;
    }

    return g_Battery.batteryVoltage;
}

// Subroutine scePower_23436A4A - Address 0x00005E98 - Aliases: scePower_driver_C730F432
s32 scePowerGetInnerTemp(float *pInnerTemp)
{
    (void)pInnerTemp;

    /* In earlier firmwares (like 1.50) this API apparently returned the PSP system chip's temperature. */

    return SCE_POWER_ERROR_0010;
}

// Subroutine sub_000056A4 - Address 0x000056A4
/* Sets the charging rate (0 = limited rate, 1 = full rate) (?) - only used on the PSP-2000 series. */
static s32 _scePowerBatterySetTTC(s32 arg0)
{
    s32 status;
    s32 intrState;

    if (!scePowerGetUsbChargingCapability()) // 0x000056B8
    {
        return SCE_ERROR_OK;
    }

    /* Only proceed if we are running on a PSP with a supported Baryon version (PSP-2000 series). */
    u8 version = _sceSysconGetBaryonVersion();
    u8 baryonMinVersion = PSP_SYSCON_BARYON_GET_VERSION_MINOR(version);
    if (PSP_SYSCON_BARYON_GET_VERSION_MAJOR(version) != 0x2 ||
        baryonMinVersion < 0x2 || baryonMinVersion >= 0x6) // 0x000056D4 - 0x0000571C
    {
        return SCE_ERROR_OK;
    }

    intrState = sceKernelCpuSuspendIntr(); // 0x00005720

    if (arg0 == 0) // 0x00005734
    {
        g_Battery.ttcConfig.unk7 = (g_Battery.ttcConfig.unk7 & 0xF8) | 0x4; // 0x00005738 & 0x00005768 - 0x00005770, 0x0000574C
    }
    else
    {
        g_Battery.ttcConfig.unk7 = (g_Battery.ttcConfig.unk7 & 0xF8) | 0x5; // 0x00005738 - 0x00005740, 0x0000574C
    }

    sceKernelCpuResumeIntr(intrState); // 0x00005748

    status = sceSysconSendSetParam(SCE_SYSCON_SET_PARAM_POWER_BATTERY_TTC, &g_Battery.ttcConfig); // 0x00005758
    return status;
}

// Subroutine sub_0x00005EA4 - Address 0x00005EA4
static SceUInt _scePowerBatteryDelayedPermitCharging(void *common)
{
    (void)common;

    g_Battery.permitChargingDelayAlarmId = -1;
    sceKernelSetEventFlag(g_Battery.batteryEventFlagId, BATTERY_EVENT_PERMIT_BATTERY_CHARGING);

    return 0; /* Delete this alarm handler. */
}

// Subroutine sub_0x00005ED8 - Address 0x00005ED8
static s32 _scePowerBatterySysconCmdIntr(SceSysconPacket *pSysconPacket, void *param)
{
    (void)pSysconPacket;
    (void)param;

    /* SYSCON has finished disabling USB charging. */
    g_Battery.sysconCmdDisableUsbChargingExecStatus = BATTERY_SYSCON_CMD_DISABLE_USB_CHARGING_EXEC_STATUS_DONE;
    return 0;
}