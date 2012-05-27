/* Copyright (C) 2011, 2012 The uOFW team
   See the file COPYING for copying permission.
*/

#include "common.h"

s32 sceSysregTopResetEnable(void);
s32 sceSysregScResetEnable(void);
s32 sceSysregMeResetEnable(void);
s32 sceSysregMeResetDisable(void);
s32 sceSysregAwResetEnable(void);
s32 sceSysregAwResetDisable(void);
s32 sceSysregVmeResetEnable(void);
s32 sceSysregVmeResetDisable(void);
s32 sceSysregAvcResetEnable(void);
s32 sceSysregAvcResetDisable(void);
s32 sceSysregUsbResetEnable(void);
s32 sceSysregUsbResetDisable(void);
s32 sceSysregAtaResetEnable(void);
s32 sceSysregAtaResetDisable(void);
s32 sceSysregMsifResetEnable(s32 no);
s32 sceSysregMsifResetDisable(s32 no);
s32 sceSysregKirkResetEnable(void);
s32 sceSysregKirkResetDisable(void);
s32 sceSysregAtahddResetEnable(void);
s32 sceSysregAtahddResetDisable(void);
s32 sceSysregUsbhostResetEnable(void);
s32 sceSysregUsbhostResetDisable(void);
s32 sceSysreg_driver_C6C75585(s32 no);
s32 sceSysreg_driver_0995F8F6(s32 no);
s32 sceSysreg_driver_72887197(void);
s32 sceSysreg_driver_32E02FDF(void);
s32 sceSysreg_driver_73B3E52D(void);
s32 sceSysregMeBusClockEnable(void);
s32 sceSysregMeBusClockDisable(void);
s32 sceSysregAwRegABusClockEnable(void);
s32 sceSysregAwRegABusClockDisable(void);
s32 sceSysregAwRegBBusClockEnable(void);
s32 sceSysregAwRegBBusClockDisable(void);
s32 sceSysregAwEdramBusClockEnable(void);
s32 sceSysregAwEdramBusClockDisable(void);
s32 sceSysregSetMasterPriv(s32, s32);
s32 sceSysregSetAwEdramSize(s32);
s32 sceSysregGetTachyonVersion(void);
s32 sceSysregAudioClkEnable(s32);
s32 sceSysregAudioClkSelect(s32, s32);
s32 sceSysregAudioBusClockEnable(s32);
s32 sceSysregAudioIoEnable(s32);
s32 sceSysregAudioIoDisable(s32);
s32 sceSysregAudioClkoutClkEnable(void);
s32 sceSysregAudioClkoutClkDisable(void);
s32 sceSysregAudioClkoutClkSelect(s32);
s32 sceSysregAudioClkoutIoEnable(void);
s32 sceSysregAudioClkoutIoDisable(void);
s32 sceSysregIntrEnd(void);
s32 sceSysregInterruptToOther(void);
s32 sceSysregPllGetFrequency(void);
s32 sceSysregSpiClkSelect(s32, s32);
s32 sceSysregSpiClkEnable(s32);
s32 sceSysregSpiIoEnable(s32);

