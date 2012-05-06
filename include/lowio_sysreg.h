/* Copyright (C) 2011, 2012 The uOFW team
   See the file COPYING for copying permission.
*/

#include "common.h"

int sceSysregTopResetEnable(void);
int sceSysregScResetEnable(void);
int sceSysregMeResetEnable(void);
int sceSysregMeResetDisable(void);
int sceSysregAwResetEnable(void);
int sceSysregAwResetDisable(void);
int sceSysregVmeResetEnable(void);
int sceSysregVmeResetDisable(void);
int sceSysregAvcResetEnable(void);
int sceSysregAvcResetDisable(void);
int sceSysregUsbResetEnable(void);
int sceSysregUsbResetDisable(void);
int sceSysregAtaResetEnable(void);
int sceSysregAtaResetDisable(void);
int sceSysregMsifResetEnable(int no);
int sceSysregMsifResetDisable(int no);
int sceSysregKirkResetEnable(void);
int sceSysregKirkResetDisable(void);
int sceSysregAtahddResetEnable(void);
int sceSysregAtahddResetDisable(void);
int sceSysregUsbhostResetEnable(void);
int sceSysregUsbhostResetDisable(void);
int sceSysreg_driver_C6C75585(int no);
int sceSysreg_driver_0995F8F6(int no);
int sceSysreg_driver_72887197(void);
int sceSysreg_driver_32E02FDF(void);
int sceSysreg_driver_73B3E52D(void);
int sceSysregMeBusClockEnable(void);
int sceSysregMeBusClockDisable(void);
int sceSysregAwRegABusClockEnable(void);
int sceSysregAwRegABusClockDisable(void);
int sceSysregAwRegBBusClockEnable(void);
int sceSysregAwRegBBusClockDisable(void);
int sceSysregAwEdramBusClockEnable(void);
int sceSysregAwEdramBusClockDisable(void);
int sceSysregSetMasterPriv(int, int);
int sceSysregSetAwEdramSize(int);
int sceSysregGetTachyonVersion(void);
int sceSysregAudioClkEnable(int);
int sceSysregAudioClkSelect(int, int);
int sceSysregAudioBusClockEnable(int);
int sceSysregAudioIoEnable(int);
int sceSysregAudioIoDisable(int);
int sceSysregAudioClkoutClkSelect(int);
int sceSysregAudioClkoutIoEnable(void);
int sceSysregAudioClkoutIoDisable(void);
int sceSysregIntrEnd(void);
int sceSysregInterruptToOther(void);
int sceSysregPllGetFrequency(void);

