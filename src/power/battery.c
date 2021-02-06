/* Copyright (C) The uOFW team
   See the file COPYING for copying permission.
*/

#include <common_imp.h>
#include <interruptman.h>
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

typedef enum {
    POWER_BATTERY_THREAD_OP_START = 0,
    POWER_BATTERY_THREAD_OP_SET_POWER_SUPPLY_STATUS = 1,
    POWER_BATTERY_THREAD_OP_SET_CHARGE_LED_STATUS = 3,
    POWER_BATTERY_THREAD_OP_SET_USB_STATUS = 4,
    POWER_BATTERY_THREAD_OP_SET_REMAIN_CAP = 5,
    POWER_BATTERY_THREAD_OP_SET_FULL_CAP = 6,
    POWER_BATTERY_THREAD_OP_SET_CHARGE_CYCLE = 7,
    POWER_BATTERY_THREAD_OP_SET_LIMIT_TIME = 8,
    POWER_BATTERY_THREAD_OP_SET_BATT_TEMP = 9,
    POWER_BATTERY_THREAD_OP_SET_BATT_ELEC = 10,
    POWER_BATTERY_THREAD_OP_SET_BATT_VOLT = 11,
} PowerBatteryThreadOperation;

typedef struct 
{
    u32 unk0;
    u8 unk4;
    u8 unk5;
    u8 unk6;
    u8 unk7;
} ScePowerSysconSetParamDataTTC; // size: 8

/**
 * This structure represents the power service's internal control block for battery management.
 */
typedef struct 
{
    u32 eventId; // 0
    u32 workerThreadId; // 4
    u32 forceSuspendCapacity; // 8
    u32 lowBatteryCapacity; // 12
    u32 isIdle; // 16
    u32 unk20;
    u32 isUsbChargingSupported; // 24
    u32 isUsbChargingEnabled; // 28
    u32 unk32;
    u32 permitChargingDelayAlarmId; // 36
    u32 unk40; // 40 TODO: Could have something to do with the status of the charge Led (On/Off)
    u32 batteryType; // 44
    u32 unk48;
    PowerBatteryThreadOperation workerThreadNextOp; // 52
    u32 isAcSupplied; // 56
    u32 powerSupplyStatus; // 60 -- TODO: Define macros for possible values
    ScePowerBatteryAvailabilityStatus batteryAvailabilityStatus; // 64
    u32 unk68;
    s32 batteryRemainingCapacity; // 72
    s32 batteryLifePercentage; // 76
    s32 batteryFullCapacity; // 80
    /*
     * Represents the minimum full capacity of a battery. The full capacity of a battery can only be
     * accurately determined with a full charge (where the charge is cutoff at the end of the chargin process).
     * As such, the actual full capacity of a battery might not always be corectly available. In some cases
     * (such as swapping a battery?) the last reported full capacity might be less than the actual full capacity.
     * The [minimumFullCapacity] is thus used to provide a more accurate full capacity value until the battery
     * equipped has been fully charged again and a new full capacity could be ascertained.
     * 
     * The minimum full capacity is determiend by initializing it to the current remaining battery capacity on PSP
     * startup and and then update it to the new reaming capacity value whenever that value is a new maximum
     * during the the use of the PSP system between two cold reboots. For example, this is the case when the battery
     * is being charged when the PSP system is turned on.
     */
    s32 minimumFullCapacity; // 84
    s32 batteryChargeCycle; // 88
    s32 limitTime; // 92
    s32 batteryTemp; // 96
    s32 batteryElec; // 100
    s32 batteryVoltage; // 104
    u32 unk108;
    SceSysconPacket powerBatterySysconPacket; // 112
    ScePowerSysconSetParamDataTTC unk208;
} ScePowerBattery; //size: 216

static inline s32 _scePowerBatteryThreadErrorObtainBattInfo();
static s32 _scePowerBatteryCalcRivisedRcap(void); // 0x00005130
static s32 _scePowerBatteryConvertVoltToRCap(s32 voltage); // 0x00005130
static s32 _scePowerBatterySetTTC(s32 arg0); // 0x000056A4
static s32 _scePowerBatteryInit(u32 isUsbChargingSupported, u32 batteryType); // 0x00005B1C
static s32 _scePowerBatteryDelayedPermitCharging(void* common); // 0x00005EA4
static s32 _scePowerBatterySysconCmdIntr(SceSysconPacket* pSysconPacket, void* param); // 0x00005ED8

ScePowerBattery g_Battery; //0x0000C5B8

// Subroutine sub_00005B1C - Address 0x00005B1C
s32 _scePowerBatteryInit(u32 isUsbChargingSupported, u32 batteryType)
{
    memset(&g_Battery, 0, sizeof(ScePowerBattery)); // 0x00005B50

    g_Battery.permitChargingDelayAlarmId = -1; // 0x00005B74
    g_Battery.isUsbChargingSupported = isUsbChargingSupported; // 0x00005B78
    g_Battery.batteryType = batteryType; // 0x00005B7C
    g_Battery.isAcSupplied = -1; // 0x00005B80
    g_Battery.batteryFullCapacity = -1; // 0x00005B84
    g_Battery.batteryChargeCycle = -1; // 0x00005B8C

    g_Battery.eventId = sceKernelCreateEventFlag("ScePowerBattery", 1, 0, NULL); // 0x00005B88
    g_Battery.workerThreadId = sceKernelCreateThread("ScePowerBattery", _scePowerBatteryThread, POWER_BATTERY_WORKER_THREAD_PRIO,
        2 * SCE_KERNEL_1KiB, SCE_KERNEL_TH_NO_FILLSTACK | 0x1, NULL); // 0x00005BB0

    sceKernelStartThread(g_Battery.workerThreadId, 0, NULL); // 0x00005BC4

    return SCE_ERROR_OK;
}

//Subroutine sub_00004498 - Address 0x00004498
// TODO: Verify function
static s32 _scePowerBatteryEnd(void)
{
    u32 outBits;

    if (g_Battery.permitChargingDelayAlarmId <= 0) { //0x000044C0
        s32 status = sceKernelPollEventFlag(g_Battery.eventId, 0x200, SCE_KERNEL_EW_CLEAR_PAT | SCE_KERNEL_EW_OR, &outBits); //0x00004554
        outBits = ((s32)status < 0) ? 0 : outBits; //0x00004564
    }
    else {
        sceKernelCancelAlarm(g_Battery.permitChargingDelayAlarmId); //0x000044C8
        outBits = 0x200; //0x000044D0
        g_Battery.permitChargingDelayAlarmId = -1;//0x000044DC
    }
    sceKernelClearEventFlag(g_Battery.eventId, ~0xF00); //0x000044E8
    sceKernelSetEventFlag(g_Battery.eventId, 0x80000000); //0x000044F4

    if (outBits & 0x200) //0x00004504
        sceSysconPermitChargeBattery(); //0x00004544

    sceKernelWaitThreadEnd(g_Battery.workerThreadId, 0); //0x00004510
    sceKernelDeleteThread(g_Battery.workerThreadId); //0x00004518
    sceKernelDeleteEventFlag(g_Battery.eventId); //0x00004524

    return SCE_ERROR_OK;
}

// Subroutine sub_00004570 - Address 0x00004570 
// TODO: Verify function
s32 _scePowerBatterySuspend(void)
{
    s32 intrState;
    u32 eventFlagBits;

    eventFlagBits = 0x40000000; // 0x00004594

    intrState = sceKernelCpuSuspendIntr(); // 0x00004590

    if (g_Battery.permitChargingDelayAlarmId > 0) // 0x000045A0
    {
        sceKernelCancelAlarm(g_Battery.permitChargingDelayAlarmId); // 0x000045A8
        g_Battery.permitChargingDelayAlarmId = -1;

        eventFlagBits |= 0x200;
    }

    if (g_Battery.unk20 != 0) // 0x000045D4
    {
        eventFlagBits |= 0x200;
    }

    // 0x000045D0
    if (g_Battery.isUsbChargingEnabled)
    {
        g_Battery.isUsbChargingEnabled = SCE_FALSE; // 0x000045D8
        eventFlagBits |= 0x400; // 0x000045DC
    }

    sceKernelClearEventFlag(g_Battery.eventId, ~0x2000000); // 0x000045E0
    sceKernelSetEventFlag(g_Battery.eventId, eventFlagBits); //0x000045EC

    sceKernelCpuResumeIntr(intrState);
    return SCE_ERROR_OK;
}

// Subroutine sub_0000461C - Address 0x0000461C 
/* Called during PSP resume phase [phase0]. Updates our battery status control block. */
s32 _scePowerBatteryUpdatePhase0(SceSysEventResumePowerState *pResumePowerState, u32 *pPowerStateForCallbackArg)
{
    u32 powerSupplyStatus;

    powerSupplyStatus = pResumePowerState->powerSupplyStatus;

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

            g_Battery.unk68 = pResumePowerState->unk40; // 0x000046A8
            g_Battery.batteryRemainingCapacity = pResumePowerState->batteryRemainCapacity; // 0x000046B0
            g_Battery.minimumFullCapacity = pResumePowerState->batteryRemainCapacity; // 0x000046B8
            g_Battery.batteryChargeCycle = -1; // 0x000046C0
            g_Battery.batteryFullCapacity = pResumePowerState->batteryFullCapacity; // 0x000046C8

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
        g_Battery.unk68 = 0; // 0x000046DC
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

// Subroutine sub_0x000046FC - Address 0x000046FC
// TODO: Verify
static s32 _scePowerBatteryThread(SceSize args, void* argp)
{
    (void)args;
    (void)argp;

    s32 isUsbChargingEnabled;
    s32 timeout; // $sp + 4
    s32 batteryEventCheckFlags; // $s2

    u32 batteryEventFlag; // $sp

    g_Battery.unk48 = 1; // 0x00004718
    g_Battery.isIdle = SCE_FALSE; // 0x0000471C
    g_Battery.workerThreadNextOp = POWER_BATTERY_THREAD_OP_START; // 0x00004720

    batteryEventCheckFlags = 0x40000700; // 0x00004728
    timeout = 0; // 0x0000473C

    isUsbChargingEnabled = scePowerGetUsbChargingCapability(); // 0x00004738
    if (isUsbChargingEnabled) // 0x00004740
    {
        u8 version = _sceSysconGetBaryonVersion(); // 0x000050CC
        if (PSP_SYSCON_BARYON_GET_VERSION_MAJOR(version) == 2 &&
            PSP_SYSCON_BARYON_GET_VERSION_MINOR(version) >= 2 && PSP_SYSCON_BARYON_GET_VERSION_MINOR(version) < 6) // 0x000050D0 - 0x00005118
        {
            /* We are running on a PSP-2000 series model. */
            sceSysconReceiveSetParam(4, &g_Battery.unk208); // 0x00005120
        }

        _scePowerBatterySetTTC(1); // 0x000050E4
    }

    s32 status; // $a0 in loc_000047AC
    for (;;)
    {
        batteryEventFlag = 0; // 0x0000474C
        if (timeout == 0) // 0x0000474C
        {
            sceKernelPollEventFlag(g_Battery.eventId, batteryEventCheckFlags /* 0x40000700 */ | 0x80000000, SCE_KERNEL_EW_OR, &batteryEventFlag); // 0x000050B4

            status = SCE_ERROR_OK; // 0x000050C0
        }
        else
        {
            s32* pTimeout;
            if (timeout == -1) // 0x00004758
                pTimeout = NULL; // 0x000050A0
            else
                pTimeout = &timeout; // 0x00004760 - 0x00004770

            status = sceKernelWaitEventFlag(g_Battery.eventId, batteryEventCheckFlags | 0x90000000, SCE_KERNEL_EW_OR, &batteryEventFlag, pTimeout); // 0x00004774 & 0x00004788

            if (batteryEventFlag & 0x10000000 && g_Battery.unk48 == 0) // 0x00004788 & 0x00004794
            {
                g_Battery.unk48 = 1; // 0x000047A0
                g_Battery.workerThreadNextOp = POWER_BATTERY_THREAD_OP_START; // 0x000047A4
            }
        }

        if (status == SCE_ERROR_KERNEL_WAIT_TIMEOUT
            || batteryEventFlag & 0x80000000) // 0x000047AC - 0x000047C0 & 0x000047C8
        {
            return SCE_ERROR_OK;
        }

        if (batteryEventFlag & 0x200) // 0x000047D0
        {
            sceSysconPermitChargeBattery();  // 0x00005028

            sceKernelClearEventFlag(g_Battery.eventId, ~0x200); // 0x00005034

            if (!(batteryEventFlag & 0x40000000)) // // 0x00005048
            {
                sceKernelDelayThread(1.5 * 1000 * 1000); // 0x00005054
                continue; // 0x0000505C
            }
        }

        if (batteryEventFlag & 0x40000000) // 0x000047DC
        {
            // loc_00004FF0
            _scePowerBatterySetTTC(1); // 0x00004FF0

            sceKernelClearEventFlag(g_Battery.eventId, ~0x40000000); // 0x00005004

            g_Battery.isIdle = SCE_TRUE; // 0x0000501C
            timeout = -1; // 0x00005024
            batteryEventCheckFlags = 0x60000100; // 0x00005008 & 0x0000500C

            continue; // 0x00005020
        }

        if (batteryEventFlag & 0x20000000) // 0x000047E8
        {
            // loc_00004FC4
            sceKernelClearEventFlag(g_Battery.eventId, ~0x20000000); // 0x00004FD0

            g_Battery.batteryAvailabilityStatus = BATTERY_AVAILABILITY_STATUS_BATTERY_NOT_INSTALLED; // 0x00004FE0
            g_Battery.workerThreadNextOp = POWER_BATTERY_THREAD_OP_START; // 0x00004FE4
            g_Battery.isIdle = SCE_FALSE; // 0x00004FEC

            batteryEventCheckFlags = 0x40000700; // 0x00004FDC
            timeout = 0; // 0x000048C0

            continue; // 0x000048BC
        }

        if (batteryEventFlag & 0x100) // 0x000047F0
        {
            // loc_00004F9C
            sceSysconForbidChargeBattery(); // 0x00004F9C

            sceKernelClearEventFlag(g_Battery.eventId, ~0x100); // 0x00004FA8

            /* Wait for 1.5 seconds */
            sceKernelDelayThread(1.5 * 1000 * 1000); // 0x00004FB4
        }

        // loc_00004800

        if (g_Battery.isIdle) // 0x00004800
        {
            continue;
        }

        u8 isAcSupplied = sceSysconIsAcSupplied(); // 0x00004808 -- $s3

        if (batteryEventFlag & 0x800) // 0x00004818
        {
            if (isAcSupplied) // 0x00004820
            {
                // loc_00004ECC
                sceSysconSetUSBStatus(4); // 0x00004ECC

                _scePowerBatterySetTTC(1); // 0x00004ED8 & 0x00004EBC
            }
            else if (g_Battery.unk108 == 0) // 0x0000482C
            {
                sceSysconSetUSBStatus(6); // 0x00004EA0

                // TODO: Define constant for 1251 battery full capacity
                _scePowerBatterySetTTC((g_Battery.batteryFullCapacity < 1251) ? 1 : 0); // 0x00004EA8 - 0x00004EBC
            }

            // loc_00004834
            sceKernelClearEventFlag(g_Battery.eventId, ~0x800); // 0x00004838
        }
        else if (batteryEventFlag & 0x400) // 0x00004EE0
        {
            // loc_00004EDC
            g_Battery.unk32 = 0; // 0x00004F78
            sceSysconSetUSBStatus(0); // 0x00004F7C

            _scePowerBatterySetTTC(1); // 0x00004F84

            sceKernelClearEventFlag(g_Battery.eventId, ~0x100); // 0x00004F94 & 0x00004838
        }
        else if (g_Battery.unk108 == 2) // 0x00004EEC
        {
            g_Battery.unk108 = 0; // 0x00004EF0

            // loc_00004F68
            _scePowerBatterySetTTC(1);
        }
        else if (g_Battery.isUsbChargingEnabled && g_Battery.isAcSupplied != isAcSupplied) // 0x00004EF8 & 0x00004F04
        {
            // 0x00004F0C
            if (!isAcSupplied) // 0x00004F0C
            {
                sceSysconSetUSBStatus(6); // 0x00004F48

                // TODO: Define constant for 1251 battery full capacity
                _scePowerBatterySetTTC((g_Battery.batteryFullCapacity < 1251) ? 1 : 0); // 0x00004F50 - 0x00004F64 & 0x00004F20
            }
            else
            {
                // 0x00004F1C
                sceSysconSetUSBStatus(4);

                _scePowerBatterySetTTC(1); // 0x00004F20
            }
        }
        else if (isAcSupplied && g_Battery.unk32 != 0) // 0x00004F28 & 0x00004F38
        {
            g_Battery.unk32 = 0; // 0x00004F3C
        }

        // loc_00004840

        timeout = 5000000; // 0x00004850

        if (g_Battery.batteryAvailabilityStatus == BATTERY_AVAILABILITY_STATUS_BATTERY_NOT_INSTALLED) // 0x0000484C
        {
            if (g_Battery.workerThreadNextOp >= 2 && g_Battery.workerThreadNextOp <= 12) // 0x00004854 - 0x00004860
            {
                g_Battery.workerThreadNextOp = POWER_BATTERY_THREAD_OP_START; // 0x00004864
            }
        }
        else if (g_Battery.batteryAvailabilityStatus == BATTERY_AVAILABILITY_STATUS_BATTERY_IS_BEING_DETECTED) // 0x00004E44
        {
            // loc_00004E94
            timeout = 20000; // 0x00004E9C 
        }
        else if (g_Battery.batteryAvailabilityStatus == BATTERY_AVAILABILITY_STATUS_BATTERY_AVAILABLE
            && g_Battery.batteryType == 0) // 0x00004E4C & 0x00004E58
        {
            s32 remainCapacity = _scePowerBatteryCalcRivisedRcap(); // 0x00004E60
            if (remainCapacity != g_Battery.batteryLifePercentage) // 0x00004E6C
            {
                g_Battery.batteryLifePercentage = remainCapacity; // 0x00004E74
                _scePowerNotifyCallback(SCE_POWER_CALLBACKARG_BATTERY_CAP,
                    remainCapacity | SCE_POWER_CALLBACKARG_BATTERYEXIST, 0); // 0x00004E7C
            }
        }

        // potentially continuing at loc_0000486C / loc_00004870?
        if (g_Battery.unk48 != 0) // 0x00004870
        {
            timeout = 20000; // 0x0000487C
        }

        g_Battery.isAcSupplied = isAcSupplied; // 0x00004884

        switch (g_Battery.workerThreadNextOp) // 0x0000488C & 0x000048A8
        {
            case POWER_BATTERY_THREAD_OP_START:
            {
                g_Battery.workerThreadNextOp = POWER_BATTERY_THREAD_OP_SET_POWER_SUPPLY_STATUS; // 0x000048B4
                timeout = 0; // 0x000048C0

                continue;
            }
            case POWER_BATTERY_THREAD_OP_SET_POWER_SUPPLY_STATUS:
            {
                // 0x000048C4
                s32 powerSupplyStatus;
                status = sceSysconGetPowerSupplyStatus(&powerSupplyStatus); // 0x000048C4
                if (status >= SCE_ERROR_OK) // 0x000048CC
                {
                    g_Battery.powerSupplyStatus = powerSupplyStatus; // 0x000048E4
                    if (!(powerSupplyStatus & SCE_SYSCON_POWER_SUPPLY_STATUS_BATTERY_EQUIPPED)) // 0x000048E0
                    {
                        _scePowerBatteryThreadErrorObtainBattInfo(); // 00004984 - 0x00004A04

                        g_Battery.unk48 = 0; // 0x00004990
                        g_Battery.workerThreadNextOp = POWER_BATTERY_THREAD_OP_START; // 0x00004998
                    }
                    else
                    {
                        if (g_Battery.batteryAvailabilityStatus == BATTERY_AVAILABILITY_STATUS_BATTERY_NOT_INSTALLED) // 0x000048EC
                        {
                            g_Battery.batteryAvailabilityStatus = BATTERY_AVAILABILITY_STATUS_BATTERY_IS_BEING_DETECTED; // 0x000048FC
                            timeout = 20000; // 0x00004900
                        }

                        // loc_00004908

                        if (!g_Battery.isUsbChargingEnabled) // 0x00004908
                        {
                            g_Battery.unk40 = (powerSupplyStatus & 0x80) ? 1 : 0; // 0x0000490C & 0x00004964 - 0x00004970
                            g_Battery.workerThreadNextOp = POWER_BATTERY_THREAD_OP_SET_REMAIN_CAP; // 0x00004980
                        }
                        else
                        {
                            g_Battery.workerThreadNextOp = POWER_BATTERY_THREAD_OP_SET_CHARGE_LED_STATUS; // 0x00004914
                        }
                    }
                }

                // loc_00004918
                //
                // uofw note: I am putting my faith in prxtool here that this will indeed work :p
                s32 cop0CtrlReg30 = pspCop0CtrlGet(COP0_CTRL_30); // 0x00004918 - $s3
                if (cop0CtrlReg30 == 0 && *((s32*)0x00000004) != 0) // 0x0000491C & 0x00004928
                {
                    void* pData = *(s32*)0x00000004;
                    *(s32*)(pData + 4) = 1; // 0x00004930
                    *(s32*)(pData + 8) = 0; // 0x00004934
                }

                s32 cop0CtrlReg31 = pspCop0CtrlGet(COP0_CTRL_31); // 0x00004938 - $s4
                if (cop0CtrlReg31 == 0 && *((s32*)0x00000000) != 0) // 0x0000493C & 0x0000494C
                {
                    void* pData = *(s32*)0x00000000;
                    *(s32*)(pData + 4) = 1; // 0x00004958
                    *(s32*)(pData + 8) = 0; // 0x00004960
                }

                continue; // 0x0000495C
            }
            case POWER_BATTERY_THREAD_OP_SET_CHARGE_LED_STATUS:
            {
                // 0x00004A14
                s16 polestarR2Val;
                status = sceSysconReadPolestarReg(2, &polestarR2Val); // 0x00004A18
                if (status < SCE_ERROR_OK) // 0x00004A20
                {
                    // loc_00004A90
                    g_Battery.unk40 = -1; // 0x00004A9C
                    g_Battery.workerThreadNextOp = POWER_BATTERY_THREAD_OP_SET_REMAIN_CAP; // 0x00004A98 & 0x00004A7C - 0x00004A84
                }
                else
                {
                    // This code is _scePowerGetPolestar2ChargeLed(), so g_Battery.unk40 something with charge Led?
                    g_Battery.unk40 = (polestarR2Val & 0x100) ? 1 : 0; // 0x00004A28 - 0x00004A3C

                    if (polestarR2Val & 0x8000 || g_Battery.powerSupplyStatus & 0x40) // 0x00004A44 & 0x00004A58
                    {
                        // loc_00004A7C
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
                s32 powerSupplyStatus;
                s16 polestarR2Val;
                // 0x00004AA0 - 0x00004AD4
                if (sceSysconGetPowerSupplyStatus(&powerSupplyStatus) < SCE_ERROR_OK
                    || sceSysconReadPolestarReg(2, &polestarR2Val) < SCE_ERROR_OK
                    || polestarR2Val & 0x8000
                    || powerSupplyStatus & 0x40)
                {
                    g_Battery.workerThreadNextOp = POWER_BATTERY_THREAD_OP_SET_REMAIN_CAP; // 0x00004A84
                    continue;
                }

                g_Battery.unk32 = 1; // 0x00004AE4
                g_Battery.isUsbChargingEnabled = SCE_FALSE; // 0x00004AF0

                sceSysconSetUSBStatus(0); // 0x00004AEC
                _scePowerBatterySetTTC(1); // 0x00004AF4

                continue; // 0x00004AFC
            }
            case POWER_BATTERY_THREAD_OP_SET_REMAIN_CAP:
            {
                // 0x00004B04
                if (g_Battery.batteryType == 0) // 0x00004B0C
                {
                    s32 batStat1;
                    s32 remainCap;
                    status = sceSysconBatteryGetStatusCap(&batStat1, &remainCap); // 0x00004B18
                    if (status < SCE_ERROR_OK) // 0x00004B20
                    {
                        // 0x00004B68 - 0x00004BE0
                        _scePowerBatteryThreadErrorObtainBattInfo();
                    }
                    else
                    {
                        g_Battery.batteryRemainingCapacity = remainCap; // 0x00004B2C
                        g_Battery.unk68 = batStat1; // 0x00004B3C

                        /* 
                         * If the new remaining battery capacity is greater than the current ascertained
                         * minimum full capacity value, we update the munimum full capacity value. For example,
                         * this is the case if the battery is currently charging and the remaining battery capacity
                         * was smaller than the just obtained new remainign capacity when the PSP was turned on.
                         */
                        if (g_Battery.minimumFullCapacity < remainCap) // 0x00004B38
                        {
                            g_Battery.minimumFullCapacity = remainCap; // 0x00004B4C
                            if (g_Battery.powerSupplyStatus & 0x80)
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
                    // loc_00004BF0
                    s32 battVolt;
                    status = sceSysconGetBattVolt(&battVolt); // 0x00004BF0
                    if (status < SCE_ERROR_OK) // 0x00004BF8
                    {
                        // 0x00004C40 - 0x00004BE0
                        _scePowerBatteryThreadErrorObtainBattInfo();
                    }
                    else
                    {
                        if (battVolt != 0) // 0x00004C00
                        {
                            g_Battery.batteryVoltage = battVolt; // 0x00004C08
                            g_Battery.batteryLifePercentage = _scePowerBatteryConvertVoltToRCap(battVolt); // 0x00004C0C & 0x00004C20

                            if (g_Battery.batteryAvailabilityStatus == BATTERY_AVAILABILITY_STATUS_BATTERY_IS_BEING_DETECTED) // 0x00004C1C
                            {
                                g_Battery.batteryAvailabilityStatus = BATTERY_AVAILABILITY_STATUS_BATTERY_AVAILABLE; // 0x00004C38
                            }

                            g_Battery.unk48 = 0;
                            g_Battery.workerThreadNextOp = POWER_BATTERY_THREAD_OP_START;
                        }
                    }
                }

                continue;
            }
            case POWER_BATTERY_THREAD_OP_SET_FULL_CAP:
            {
                // 0x00004CAC

                if (g_Battery.batteryFullCapacity < 0) // 0x00004CB8
                {
                    s32 fullCap;
                    status = sceSysconBatteryGetFullCap(&fullCap); // 0x00004CC8
                    if (status < SCE_ERROR_OK) // 0x00004CD0
                    {
                        // 0x00004C40 - 0x00004BE0
                        _scePowerBatteryThreadErrorObtainBattInfo();
                    }
                    else
                    {
                        g_Battery.workerThreadNextOp = POWER_BATTERY_THREAD_OP_SET_CHARGE_CYCLE; // 0x00004CD8
                        g_Battery.batteryFullCapacity = fullCap; // 0x00004CE4
                    }
                }
                else
                {
                    g_Battery.workerThreadNextOp = POWER_BATTERY_THREAD_OP_SET_CHARGE_CYCLE; // 0x00004CC4
                    timeout = 0; // 0x000048BC
                }

                continue;
            }
            case POWER_BATTERY_THREAD_OP_SET_CHARGE_CYCLE:
            {
                // 0x00004CE8
                if (g_Battery.batteryChargeCycle < 0) // 0x00004CF0
                {
                    // loc_00004D00
                    s32 chargeCycle;
                    status = sceSysconBatteryGetCycle(&chargeCycle); // 0x00004D00
                    if (status < SCE_ERROR_OK) // 0x00004D08
                    {
                        // 0x00004C40 - 0x00004BE0
                        _scePowerBatteryThreadErrorObtainBattInfo();
                    }
                    else
                    {
                        g_Battery.workerThreadNextOp = POWER_BATTERY_THREAD_OP_SET_LIMIT_TIME; // 0x00004D10
                        g_Battery.batteryChargeCycle = chargeCycle; // 0x00004D1C
                    }
                }
                else
                {
                    g_Battery.workerThreadNextOp = POWER_BATTERY_THREAD_OP_SET_LIMIT_TIME; // 0x00004CF8 & 0x00004CC4
                    timeout = 0; // 0x000048C0
                }

                continue;
            }
            case POWER_BATTERY_THREAD_OP_SET_LIMIT_TIME:
            {
                // 0x00004D20
                u8 isAcSupplied = sceSysconIsAcSupplied(); // 0x00004D20
                if (!isAcSupplied) // 0x00004D28
                {
                    // loc_00004D44
                    s32 limitTime;
                    s32 status = sceSysconBatteryGetLimitTime(&limitTime); // 0x00004D44
                    if (status < SCE_ERROR_OK) // 0x00004D4C
                    {
                        // 0x00004D68 - 0x00004BE0
                        _scePowerBatteryThreadErrorObtainBattInfo();
                    }
                    else
                    {
                        g_Battery.workerThreadNextOp = POWER_BATTERY_THREAD_OP_SET_BATT_TEMP; // 0x00004D58
                        g_Battery.limitTime = limitTime; // 0x00004D64
                    }
                }
                else
                {
                    g_Battery.workerThreadNextOp = POWER_BATTERY_THREAD_OP_SET_BATT_TEMP; // 0x00004D38
                    g_Battery.limitTime = -1; // 0x00004D40

                    timeout = 0; // 0x00004D3C & 0x000048C0
                }

                continue;
            }
            case POWER_BATTERY_THREAD_OP_SET_BATT_TEMP:
            {
                // 0x00004DBC
                s32 battTemp;
                status = sceSysconBatteryGetTemp(&battTemp); // 0x00004DBC
                if (status < SCE_ERROR_OK) // 0x00004DC4
                {
                    // 0x00004D68 - 0x00004BE0
                    _scePowerBatteryThreadErrorObtainBattInfo();
                }
                else
                {
                    g_Battery.workerThreadNextOp = POWER_BATTERY_THREAD_OP_SET_BATT_ELEC; // 0x00004DD0
                    g_Battery.batteryTemp = battTemp; // 0x00004DDC
                }

                continue;
            }
            case POWER_BATTERY_THREAD_OP_SET_BATT_ELEC:
            {
                // 0x00004DE0
                s32 battElec;
                status = sceSysconBatteryGetElec(&battElec); // 0x00004DE0
                if (status < SCE_ERROR_OK) // 0x00004DE8
                {
                    // 0x00004D68 - 0x00004BE0
                    _scePowerBatteryThreadErrorObtainBattInfo();
                }
                else
                {
                    g_Battery.workerThreadNextOp = POWER_BATTERY_THREAD_OP_SET_BATT_VOLT; // 0x00004DF0
                    g_Battery.batteryElec = battElec; // 0x00004E00
                }

                continue;
            }
            case POWER_BATTERY_THREAD_OP_SET_BATT_VOLT:
            {
                // 0x00004E04
                s32 battVolt;
                status = sceSysconBatteryGetVolt(&battVolt); // 0x00004E04
                if (status < SCE_ERROR_OK) // 0x00004E0C
                {
                    // 0x00004D68 - 0x00004BE0
                    _scePowerBatteryThreadErrorObtainBattInfo();
                }
                else
                {
                    g_Battery.batteryVoltage = battVolt; // 0x00004E24
                    if (g_Battery.batteryAvailabilityStatus == BATTERY_AVAILABILITY_STATUS_BATTERY_IS_BEING_DETECTED) // 0x00004E20
                    {
                        g_Battery.batteryAvailabilityStatus = BATTERY_AVAILABILITY_STATUS_BATTERY_AVAILABLE; // 0x00004E3C
                    }

                    g_Battery.unk48 = 0; // 0x00004E28
                    g_Battery.workerThreadNextOp = POWER_BATTERY_THREAD_OP_START; // 0x00004E30
                }

                continue;
            }
            default:
                continue; // 0x0000488C
        }
    }
}

static inline s32 _scePowerBatteryThreadErrorObtainBattInfo()
{
    if (g_Battery.batteryAvailabilityStatus != BATTERY_AVAILABILITY_STATUS_BATTERY_NOT_INSTALLED) // 0x00004D70
    {
        // 0x00004D78 - 0x00004DB8
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
        g_Battery.unk40 = 0;
        g_Battery.unk68 = 0;

        _scePowerNotifyCallback(SCE_POWER_CALLBACKARG_BATTERYEXIST, 0, 0); // 0x00004DB4 & 0x00004BB0

        s32 intrState = sceKernelCpuSuspendIntr(); // 0x00004BB8

        sceKernelSetEventFlag(g_Battery.eventId, 0x10000000); // 0x00004BC8
        sceKernelClearEventFlag(g_Battery.eventId, ~0x10000000); // 0x00004BD8

        sceKernelCpuResumeIntr(intrState); // 0x00004BE0
    }
}

// Subroutine sub_00005130 - Address 0x00005130
// Note: Yep, that's the name used in 5.00, might have been corrected since.
// TODO: Verify correctness (for example using a small test function)
static s32 _scePowerBatteryCalcRivisedRcap(void)
{
    s32 fsCap;
    s32 rCap;
    s32 lCap;
    s32 fCap;
    s32 fCapLimit;

    fCap = g_Battery.batteryFullCapacity;
    fCapLimit = fCap * 0.9; // 0x0000513C - 0x00005164
    if (fCapLimit < g_Battery.minimumFullCapacity) // 0x0000516C
    {
        fCapLimit = g_Battery.minimumFullCapacity * 0.95; // 0x00005174 - 0x00005194
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

    /* Check if the remaining capacity is no larger than the low-battery capacity. */
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

         // TODO: Not exactly sure yet what fCapLimit is exactly about...
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

// Subroutine sub_0000524C - Address 0x0000524C
//
// TODO: 
//  - This function has been changed since 5.00. While it still most likely returns the remaining battery capacity
//    its input have changed. As such, this function name might no longer be correct
//  - Define constants for these values
//  - Might these be different values depending on the PSP model used (thus different battery)?
/* Convert battery voltage to remaining battery capacity (relative) */
static s32 _scePowerBatteryConvertVoltToRCap(s32 voltage)
{
    if (voltage <= 0 || voltage < 3100 || voltage < 3300 || voltage < 3400) // 0x0000524C - 0x00005268
    {
        return 0;
    }

    if (voltage < 3500) // 0x00005270
    {
        return 2;
    }

    if (voltage < 3600) // 0x0000527C
    {
        return 4;
    }

    if (voltage < 3700) // 0x00005288
    {
        return 12;
    }

    if (voltage < 3800) // 0x00005294
    {
        return 30;
    }

    if (voltage < 4000) // 0x000052A0
    {
        return 60;
    }

    if (voltage < 4050 || voltage < 4150) // 0x000052AC
    {
        return 90;
    }

    return 100;
}

// Subroutine scePower_driver_5F5006D2 - Address 0x000052C8
// TODO: Write documentation
s32 scePowerGetUsbChargingCapability(void)
{
    return (s32)g_Battery.isUsbChargingSupported;
}

// Subroutine scePower_driver_10CE273F - Address 0x000052D4
// TODO: Write documentation
s32 scePowerBatteryForbidCharging(void)
{
    s32 intrState1;
    s32 intrState2;

    intrState1 = sceKernelCpuSuspendIntr();  // 0x000052EC

    if (g_Battery.unk20 == 0) // 0x000052F8
    {
        if (g_Battery.permitChargingDelayAlarmId > 0) // 0x00005304
        {
            sceKernelCancelAlarm(g_Battery.permitChargingDelayAlarmId); // 0x0000530C
            g_Battery.permitChargingDelayAlarmId = 0; // 0x00005318
        }

        sceKernelClearEventFlag(g_Battery.eventId, ~0x200); // 0x00005320
        sceKernelSetEventFlag(g_Battery.eventId, 0x100); // 0x0000532C
    }

    g_Battery.unk20++; // 0x00005344

    intrState2 = sceKernelCpuSuspendIntr(); // 0x00005340

    sceKernelSetEventFlag(g_Battery.eventId, 0x10000000); // 0x00005350
    sceKernelClearEventFlag(g_Battery.eventId, ~0x10000000); // 0x00005360

    sceKernelCpuResumeIntr(intrState2); // 0x00005368
    sceKernelCpuResumeIntr(intrState1); // 0x00005370

    return SCE_ERROR_OK;
}

// Subroutine scePower_driver_EF751B4A - Address 0x00005394 
// TODO: Write documentation
s32 scePowerBatteryPermitCharging(void)
{
    s32 intrState1;
    s32 intrState2;

    intrState1 = sceKernelCpuSuspendIntr(); // 0x000053AC

    if (g_Battery.unk20 > 0)
    {
        g_Battery.unk20--; // 0x000053CC
        if (g_Battery.unk20 == 0) // 0x000053C8
        {
            sceKernelClearEventFlag(g_Battery.eventId, ~0x100); // 0x00005424
            g_Battery.permitChargingDelayAlarmId = sceKernelSetAlarm(POWER_DELAY_PERMIT_CHARGING, _scePowerBatteryDelayedPermitCharging, NULL); // 0x0000543C
        }
    }

    intrState2 = sceKernelCpuSuspendIntr(); // 0x000053D0

    sceKernelSetEventFlag(g_Battery.eventId, 0x10000000); // 0x000053E0
    sceKernelClearEventFlag(g_Battery.eventId, ~0x10000000); // 0x000053F0

    sceKernelCpuResumeIntr(intrState2); // 0x000053F8
    sceKernelCpuResumeIntr(intrState1); // 0x00005400

    return SCE_ERROR_OK;
}

// Subroutine sub_0000544C - Address 0x0000544C
// TODO: Write documentation
s32 _scePowerBatteryUpdateAcSupply(s32 enable)
{
    s32 intrState;

    if (!enable) // 0x00005464
    {
        return SCE_ERROR_OK;
    }

    intrState = sceKernelCpuSuspendIntr(); // 0x00005488

    if (g_Battery.unk20 > 0) // 0x00005494
    {
        if (g_Battery.permitChargingDelayAlarmId > 0) // 0x000054A0
        {
            sceKernelCancelAlarm(g_Battery.permitChargingDelayAlarmId); // 0x000054AC
            g_Battery.permitChargingDelayAlarmId = -1; // 0x000054B4
        }

        sceKernelClearEventFlag(g_Battery.eventId, ~0x200); // 0x000054BC
        sceKernelSetEventFlag(g_Battery.eventId, 0x100);  // 0x000054C8
    }

    sceKernelCpuResumeIntr(intrState);
    return SCE_ERROR_OK;
}

// Subroutine scePower_driver_72D1B53A - Address 0x000054E0
// TODO: Write documentation
s32 scePowerBatteryEnableUsbCharging(void)
{
    s32 prevIsUsbEnabled;
    s32 intrState;

    if (!g_Battery.isUsbChargingSupported) // 0x00005508
    {
        return SCE_ERROR_NOT_SUPPORTED;
    }

    if (g_Battery.isIdle) // 0x00005514
    {
        return SCE_ERROR_OK;
    }

    intrState = sceKernelCpuSuspendIntr(); // 0x0000553C

    prevIsUsbEnabled = g_Battery.isUsbChargingEnabled;
    if (!g_Battery.isUsbChargingEnabled) // 0x00005548
    {
        g_Battery.isUsbChargingEnabled = SCE_TRUE; // 0x00005568

        if (!sceSysconIsAcSupplied()) // 0x00005564
        {
            sceKernelSetEventFlag(g_Battery.eventId, 0x800); // 0x00005574
            sceKernelClearEventFlag(g_Battery.eventId, ~0x400); // 0x00005580
        }
    }

    sceKernelCpuResumeIntr(intrState); // 0x00005550
    return prevIsUsbEnabled;
}

// Subroutine scePower_driver_7EAA4247 - Address 0x00005590
// TODO: Write documentation
s32 scePowerBatteryDisableUsbCharging(void)
{
    s32 status;
    s32 intrState1;
    s32 intrState2;

    if (!g_Battery.isUsbChargingSupported) // 0x000055B4
    {
        return SCE_ERROR_NOT_SUPPORTED;
    }

    intrState1 = sceKernelCpuSuspendIntr(); // 0x000055BC

    if (g_Battery.isUsbChargingEnabled) // 0x000055C8
    {
        g_Battery.isUsbChargingEnabled = SCE_FALSE; // 0x000055E8

        if (g_Battery.unk108 == 0) // 0x000055EC
        {
            g_Battery.powerBatterySysconPacket.tx[0] = PSP_SYSCON_CMD_SET_USB_STATUS; // 0x0000567C
            g_Battery.powerBatterySysconPacket.tx[1] = 3; // 0x00005680
            g_Battery.powerBatterySysconPacket.tx[2] = 4; // 0x00005688

            status = sceSysconCmdExecAsync(&g_Battery.powerBatterySysconPacket, 1, _scePowerBatterySysconCmdIntr, NULL); // 0x00005684
            if (status < SCE_ERROR_OK) // 0x0000568C
            {
                sceKernelSetEventFlag(g_Battery.eventId, 0x400); // 0x00005660
            }
            else
            {
                g_Battery.unk108 = 1; // 0x00005698
                g_Battery.unk32 = 0; // 0x000056A0
            }
        }
        else
        {
            sceKernelSetEventFlag(g_Battery.eventId, 0x400); // 0x00005660
        }

        sceKernelClearEventFlag(g_Battery.eventId, ~0x800); // 0x000055FC
    }

    intrState2 = sceKernelCpuSuspendIntr(); // 0x00005604

    sceKernelSetEventFlag(g_Battery.eventId, 0x10000000);  // 0x00005614
    sceKernelClearEventFlag(g_Battery.eventId, ~0x10000000); // 0x00005624

    sceKernelCpuResumeIntr(intrState2); // 0x0000562C
    sceKernelCpuResumeIntr(intrState1); // 0x00005634

    return SCE_ERROR_OK;
}

// Subroutine sub_000056A4 - Address 0x000056A4
static s32 _scePowerBatterySetTTC(s32 arg0)
{
    s32 status;
    s32 intrState;

    if (!scePowerGetUsbChargingCapability()) // 0x000056B8
    {
        return SCE_ERROR_OK;
    }

    // 0x000056D4 - 0x0000571C

    /* Ony proceed if we are running on a PSP with a supported Baryon version (PS-2000 series). */
    u8 version = _sceSysconGetBaryonVersion();
    if (PSP_SYSCON_BARYON_GET_VERSION_MAJOR(version) != 2 &&
        PSP_SYSCON_BARYON_GET_VERSION_MINOR(version) < 2 && PSP_SYSCON_BARYON_GET_VERSION_MINOR(version) > 6)
    {
        return SCE_ERROR_OK;
    }

    intrState = sceKernelCpuSuspendIntr(); // 0x00005720

    if (arg0 == 0) // 0x00005734
    {
        g_Battery.unk208.unk7 = (g_Battery.unk208.unk7 & 0xF8) | 0x4; // 0x00005738 & 0x00005768 - 0x00005770, 0x0000574C
    }
    else
    {
        g_Battery.unk208.unk7 = (g_Battery.unk208.unk7 & 0xF8) | 0x5; // 0x00005738 - 0x00005740, 0x0000574C
    }

    sceKernelCpuResumeIntr(intrState); // 0x00005748

    status = sceSysconSendSetParam(4, &g_Battery.unk208); // 0x00005758
    return status;
}

// Subroutine scePower_B4432BC8 - Address 0x00005774 - Aliases: scePower_driver_67492C52
//
// TODO: Figure out meaning behind different batteryChargingStatus values (0 - 3) 
// Looking at scePowerIsBatteryCharging() it appears [1] means battery is charging, and [0, 2, 3] mean 
// battery is not charging
// TODO: Write documentation
s32 scePowerGetBatteryChargingStatus(void)
{
    s32 oldK1;
    s32 batteryChargingStatus;

    oldK1 = pspShiftK1(); // 0x000057A0

    if (g_Battery.batteryAvailabilityStatus == BATTERY_AVAILABILITY_STATUS_BATTERY_NOT_INSTALLED) // 0x0000579C
    {
        pspSetK1(oldK1);
        return SCE_POWER_ERROR_NO_BATTERY;
    }

    if (g_Battery.batteryAvailabilityStatus == BATTERY_AVAILABILITY_STATUS_BATTERY_IS_BEING_DETECTED) // 0x000057AC
    {
        pspSetK1(oldK1);
        return SCE_POWER_ERROR_DETECTING;
    }

    if (sceSysconIsAcSupplied()) // 0x000057BC
    {
        // TODO: Battery charging may be suppressed (the battery is not charging) while the WLAN is in use
        // Could g_Battery.unk20 here be an indicator for WlanActive?

        // Another case is where we are connected to an external power source and the battery is already fully charged.
        // In this case, we report the battery as not charging and this info could be accessed in the else part below (?)

        if (g_Battery.unk20 != 0) // 0x000057C8
        {
            // battery not charging
            batteryChargingStatus = 2; // 0x000057CC
        }
        else
        {
            // If bit 8 (pos 7) is set, then battery is being charged
            batteryChargingStatus = g_Battery.powerSupplyStatus >> 7 & 0x1; // 0x000057D0 & 0x000057D4
        }
    }
    else
    {
        if (g_Battery.unk32 != 0) // 0x000057F4
        {
            // Apparently battery not charging
            batteryChargingStatus = 3; // 0x000057F8
        }
        else if (!g_Battery.isUsbChargingEnabled || !(g_Battery.powerSupplyStatus & 0x40) || g_Battery.unk40 == 0) // 0x00005800 & 0x00005810 & 0x0000581C
        {
            // Aparently battery not charging
            batteryChargingStatus = 0; // 0x00005804
        }
        else
        {
            // Apparently battery charging, why is that? We are not connected to an external power source
            // Could this be USB charging and sceSysconIsAcSupplied() does not supply that info (only traditional PSP AC)?
            batteryChargingStatus = 1; // 0x00005820
        }
    }

    pspSetK1(oldK1);
    return batteryChargingStatus;
}

// Subroutine scePower_78A1A796 - Address 0x0000582C - Aliases: scePower_driver_88C79735
// TODO: Write documentation
s32 scePowerIsSuspendRequired(void)
{
    s32 isSuspendRequired;
    s32 oldK1;

    oldK1 = pspShiftK1(); // 0x00005838

    if (sceSysconIsAcSupplied()) // 0x00005850
    {
        pspSetK1(oldK1);
        return SCE_FALSE; // 0x000058D8
    }

    if (g_Battery.batteryType == 0) // 0x0000585C
    {
        s32 remainingCapacity = scePowerGetBatteryRemainCapacity(); // 0x000058B8
        if (remainingCapacity <= 0) // 0x000058C0
        {
            isSuspendRequired = SCE_FALSE; // 0x000058C4
        }
        else
        {
            isSuspendRequired = remainingCapacity < g_Battery.forceSuspendCapacity; // 0x000058C8 & 0x000058D0
        }
    }

    if (g_Battery.batteryType == 1) // 0x00005864
    {
        u8 baryonStatus2 = sceSysconGetBaryonStatus2(); // 0x0000588C
        if (baryonStatus2 & 0x8) // 0x00005898
        {
            isSuspendRequired = SCE_TRUE; // 0x00005890
        }
        else
        {
            // TODO: Define constant for 3400 batteryVoltage
            isSuspendRequired = g_Battery.batteryVoltage >= 0 && g_Battery.batteryVoltage < 3400; // 0x0000589C - 0x000058B4
        }
    }

    pspSetK1(oldK1);

    // uofw note: if g_Battery.batteryType is neither 0 or 1, isSuspendRequired is unitialized.
    // That said, perhaps Sony made sure g_Battery.batteryType is only ever either 0 or 1.
    return isSuspendRequired;
}

// Subroutine scePower_94F5A53F - Address 0x000058DC - Aliases: scePower_driver_41ADFF48
// TODO: Write documentation
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
// TODO: Write documentation
s32 scePowerGetBatteryLifeTime(void)
{
    s32 status;
    s32 intrState;

    // If an external power supply is available, return 0 since we cannot estimate the remaining batter life.
    if (sceSysconIsAcSupplied()) // 0x00005920 & 0x00005928
    {
        return 0;
    }

    if (g_Battery.batteryType != 0) // 0x0000593C
    {
        // uofw note: Yes, Sony is returning an unitialized local variable here
        // Presumably status should have been set to SCE_ERROR_NOT_SUPPORTED before 
        // returning here.
        return status;
    }

    s32 remainingCapacity = scePowerGetBatteryRemainCapacity(); // 0x00005944
    if (remainingCapacity < 0) // 0x00005950
    {
        return remainingCapacity; // 0x00005954
    }

    if (!g_Battery.isUsbChargingEnabled && g_Battery.batteryElec >= 0) // 0x0000596C & 0x000059EC
    {
        intrState = sceKernelCpuSuspendIntr(); // 0x000059F4

        sceKernelSetEventFlag(g_Battery.eventId, 0x10000000); // 0x00005A04
        sceKernelClearEventFlag(g_Battery.eventId, ~0x10000000); // 0x00005A14

        sceKernelCpuResumeIntr(intrState);
        return SCE_POWER_ERROR_DETECTING;
    }

    if (g_Battery.batteryElec < 0) // 0x00005978
    {
        s32 remainingCapacityForRunning = (remainingCapacity - g_Battery.forceSuspendCapacity); // 0x000059A8 & 0x000059AC

        if (g_Battery.batteryElec == -1) // 0x000059C0
        {
            pspBreak(SCE_BREAKCODE_DIVZERO); // 0x000059C4
        }

        // Convert remaining capacity (which is (presumably - based on PS Vita SDK doc) in mAh) into minutes.
        // rough formula to use here:
        //    (mAh)/(Amps*1000)*60 = (minutes). 
        s32 cvtRemainingTime = remainingCapacityForRunning * 60 / ~g_Battery.batteryElec; // 0x000059B0 & 0x000059B4 & 0x000059B4

        if (g_Battery.limitTime >= 0) // 0x000059CC & 0x000059D
        {
            cvtRemainingTime = pspMin(cvtRemainingTime, g_Battery.limitTime); // 0x000059D8
        }

        return pspMax(cvtRemainingTime, 1); // 0x000059E0
    }

    return 0;
}

// Subroutine scePower_28E12023 - Address 0x00005A30 - Aliases: scePower_driver_40870DAC
// TODO: Write documentation
s32 scePowerGetBatteryTemp(void)
{
    if (g_Battery.batteryType != 0) // 0x00005A40
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
// TODO: Write documentation
s32 scePowerGetBatteryElec(u32 *pBatteryElec)
{
    s32 oldK1;

    oldK1 = pspShiftK1(); // 0x00005A80

    if (g_Battery.batteryType != 0) // 0x00005A90
    {
        return SCE_ERROR_NOT_SUPPORTED;
    }

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
// TODO: Write documentation
s32 scePowerGetBatteryChargeCycle(void)
{
    if (g_Battery.batteryType != 0) // 0x00005AE8
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

// Subroutine sub_00005BF0 - Address 0x00005BF0
s32 _scePowerBatterySetParam(s32 forceSuspendCapacity, s32 lowBatteryCapacity)
{
    g_Battery.lowBatteryCapacity = lowBatteryCapacity; // 0x00005BFC
    g_Battery.forceSuspendCapacity = forceSuspendCapacity; // 0x00005C04

    return SCE_ERROR_OK;
}

// Subroutine sub_00005C08 - Address 0x00005C08
s32 _scePowerBatteryIsBusy(void)
{
    return !g_Battery.isIdle;
}

// Subroutine sub_00005C18 - Address 0x00005C18
static s32 _scePowerBatteryResume(void)
{
    s32 intrState1;
    s32 intrState2;

    intrState1 = sceKernelCpuSuspendIntr(); // 0x00005C2C

    g_Battery.isAcSupplied = -1; // 0x00005C40
    g_Battery.batteryAvailabilityStatus = BATTERY_AVAILABILITY_STATUS_BATTERY_NOT_INSTALLED; // 0x00005C4C
    g_Battery.isIdle = SCE_FALSE; // 0x00005C4C

    sceKernelClearEventFlag(g_Battery.eventId, ~0x900); // 0x00005C64
    sceKernelSetEventFlag(g_Battery.eventId, g_Battery.unk20 == 0 ? 0x20000000 : 0x20000100); // 0x00005C70

    intrState2 = sceKernelCpuSuspendIntr(); // 0x00005C78

    sceKernelSetEventFlag(g_Battery.eventId, 0x10000000); // 0x00005C88
    sceKernelClearEventFlag(g_Battery.eventId, ~0x10000000); // 0x00005C98

    sceKernelCpuResumeIntr(intrState2); // 0x00005CA0
    sceKernelCpuResumeIntr(intrState1); // 0x00005CA8

    return SCE_ERROR_OK;
}

// Subroutine scePower_27F3292C - Address 0x00005CCC - Aliases: scePower_driver_0DA940D2
// TODO: Write documentation
s32 scePowerBatteryUpdateInfo(void)
{
    s32 intrState;

    intrState = sceKernelCpuSuspendIntr(); // 0x00005CDC

    sceKernelSetEventFlag(g_Battery.eventId, 0x10000000); // 0x00005CEC
    sceKernelClearEventFlag(g_Battery.eventId, ~0x10000000); // 0x00005CFC

    sceKernelCpuResumeIntr(intrState); // 0x00005D04
    return SCE_ERROR_OK;
}

// Subroutine scePower_E8E4E204 - Address 0x00005D24 - Aliases: scePower_driver_A641CF3F
// TODO: Write documentation
s32 scePowerGetForceSuspendCapacity(void)
{
    return (s32)g_Battery.forceSuspendCapacity;
}

// Subroutine scePower_B999184C - Address 0x00005D30 - Aliases: scePower_driver_7B908CAA
// TODO: Write documentation
s32 scePowerGetLowBatteryCapacity(void)
{
    return (s32)g_Battery.lowBatteryCapacity;
}

// Subroutine scePower_87440F5E - Address 0x00005D3C - Aliases: scePower_driver_872F4ECE
// TODO: Write documentation
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
// TODO: Write documentation
s32 scePowerIsBatteryExist(void)
{
    return (s32)(g_Battery.batteryAvailabilityStatus != BATTERY_AVAILABILITY_STATUS_BATTERY_NOT_INSTALLED);
}

// Subroutine scePower_1E490401 - Address 0x00005D78 - Aliases: scePower_driver_7A9EA6DE
// TODO: Write documentation
s32 scePowerIsBatteryCharging(void)
{
    s32 status;

    status = scePowerGetBatteryChargingStatus(); // 0x00005D80

    if (status == 2 || status == 3) // 0x00005D88 & 0x00005D90
    {
        status = SCE_FALSE; // 0x00005D94
    }

    return status;
}

// Subroutine scePower_D3075926 - Address 0x00005DA0 - Aliases: scePower_driver_FA651CE1
// TODO: Write documentation
s32 scePowerIsLowBattery(void)
{
    s32 status;
    s32 oldK1;

    status = SCE_ERROR_OK;
    oldK1 = pspShiftK1(); // 0x00005DC4

    if (g_Battery.batteryAvailabilityStatus == BATTERY_AVAILABILITY_STATUS_BATTERY_AVAILABLE) // 0x00005DC0
    {
        status = sceSysconIsLowBattery(); // 0x00005DE0
    }

    pspSetK1(oldK1);
    return status;
}

// Subroutine scePower_driver_071160B1 - Address 0x00005DF0
// TODO: Write documentation
s32 scePowerGetBatteryType(void)
{
    return (s32)g_Battery.batteryType;
}

// Subroutine scePower_FD18A0FF - Address 0x00005DFC - Aliases: scePower_driver_003B1E03
// TODO: Write documentation
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
// TODO: Write documentation
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
// TODO: Write documentation
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
// TODO: Write documentation
s32 scePowerGetInnerTemp(void)
{
    return SCE_POWER_ERROR_0010;
}

// Subroutine sub_0x00005EA4 - Address 0x00005EA4
static s32 _scePowerBatteryDelayedPermitCharging(void *common)
{
    (void)common;

    g_Battery.permitChargingDelayAlarmId = -1;
    sceKernelSetEventFlag(g_Battery.eventId, 0x200);

    return 0; /* Delete this alarm handler. */
}

// Subroutine sub_0x00005ED8 - Address 0x00005ED8
static s32 _scePowerBatterySysconCmdIntr(SceSysconPacket *pSysconPacket, void *param)
{
    (void)pSysconPacket;
    (void)param;

    g_Battery.unk108 = 2;
    return SCE_ERROR_OK;
}