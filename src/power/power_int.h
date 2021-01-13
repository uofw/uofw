/* Copyright (C) The uOFW team
   See the file COPYING for copying permission.
*/

#ifndef POWER_INT_H
#define POWER_INT_H

#include <common_header.h>

void _scePowerNotifyCallback(s32 clearPowerState, s32 setPowerState, s32 cbOnlyPowerState);

u32 _scePowerSwInit(void);
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

s32 _scePowerFreqInit(void);
u32 _scePowerFreqEnd(void);
u32 _scePowerFreqRebootPhase(u32 arg0);
s32 _scePowerSetClockFrequency(s32 pllFrequency, s32 cpuFrequency, s32 busFrequency);
s32 _scePowerLockPowerFreqMutex(void);
s32 _scePowerUnlockPowerFreqMutex(void);

#endif	/* POWER_INT_H */
