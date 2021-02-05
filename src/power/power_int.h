/* Copyright (C) The uOFW team
   See the file COPYING for copying permission.
*/

#ifndef POWER_INT_H
#define POWER_INT_H

#include <common_header.h>

/* clock frequency limits */

#define PSP_CLOCK_PLL_FREQUENCY_MIN                     19
#define PSP_CLOCK_PLL_FREQUENCY_MAX                     333

#define PSP_CLOCK_CPU_FREQUENCY_MIN                     1
#define PSP_CLOCK_CPU_FREQUENCY_MAX                     333

#define PSP_CLOCK_BUS_FREQUENCY_MIN                     1
#define PSP_CLOCK_BUS_FREQUENCY_MAX                     166

void _scePowerNotifyCallback(s32 clearPowerState, s32 setPowerState, s32 cbOneOffPowerState);
s32 _scePowerIsCallbackBusy(s32 cbArgFlag, SceUID *pCbid);

s32 _scePowerSwInit(void);
s32 _scePowerSwEnd(void);

u32 _scePowerIdleInit(void);
u32 _scePowerIdleEnd(void);

s32 _scePowerBatteryInit(u32 isUsbChargingSupported, u32 batteryType);
s32 _scePowerBatteryEnd(void);
s32 _scePowerBatterySuspend(void);
s32 _scePowerBatteryResume(void);
s32 _scePowerBatteryUpdatePhase0(void* arg0, u32* arg1);
s32 _scePowerBatteryUpdateAcSupply(s32 enable);
s32 _scePowerBatterySetParam(s32 forceSuspendCapacity, s32 lowBatteryCapacity);
s32 _scePowerChangeSuspendCap(u32 newSuspendCap);
s32 _scePowerBatteryIsBusy(void);

s32 _scePowerFreqInit(void);
s32 _scePowerFreqEnd(void);
s32 _scePowerFreqSuspend(void);
s32 _scePowerFreqResume(u32 resumeStep);
s32 _scePowerFreqRebootPhase(s32 arg0);
s32 _scePowerSetClockFrequency(s32 pllFrequency, s32 cpuFrequency, s32 busFrequency);
s32 _scePowerLockPowerFreqMutex(void);
s32 _scePowerUnlockPowerFreqMutex(void);

#endif	/* POWER_INT_H */
