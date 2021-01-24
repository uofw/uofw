/* Copyright (C) 2011, 2012 The uOFW team
   See the file COPYING for copying permission.
*/

#include "common_header.h"

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
float sceSysregPllGetFrequency(void);
s32 sceSysregSpiClkSelect(s32, s32);
s32 sceSysregSpiClkEnable(s32);
s32 sceSysregSpiIoEnable(s32);
s32 sceSysregSpiIoDisable(s32);

s32 sceSysregUartIoDisable(s32);

s32 sceSysregGpioIoDisable(s32);



/*
 * The PLL clock frequency is calculated the following way:
 *
 *		pllfreq = (multiplier * basefreq) * pllTable[pllOutSelect],
 * 
 * with basefreq = 37 MHz and multiplier = 9 (by default)
 *
 * <pllOutSelect> is an index into a table defining values which will yield
 * the desired clock frequency value when inserted into the formula above. 
 * 
 * Example: Setting the PLL's out select value to ::SCE_SYSREG_PLL_OUT_SELECT_222MHz
 * the value at pllTable[SCE_SYSREG_PLL_OUT_SELECT_222MHz] will be used in the 
 * PLL frequency calculation, thus we get:
 * 
 *		pllfreq = (37 MHz * 9) * 0.667 = ~222 MHz
 * 
 * Remarks: 
 * 
 * Note that as of at least 6.60, pllTable[pllOutSelect] now
 * returns the final clock frequency:
 * 
 *		pllfreq = pllTable[pllOutSelect] MHz 
 * 
 * In other words,
 * pllTable[SCE_SYSREG_PLL_OUT_SELECT_222MHz] now returns 222 instead of 0.667.
 * 
 * That table can be found in the lowio module at address 0x0000C1C0. It's also
 * worth pointing out that the PLL clock frequencies 19 MHz and 148 MHz don't seem
 * to be available any longer. If we study the pllTable content, we see that it starts
 * with 190 MHz all the way up to the 166 MHz entry (in other words, each index now is 
 * appears to be index - 2 internally).
 */
#define SCE_SYSREG_PLL_OUT_SELECT_37MHz		(0)
#define SCE_SYSREG_PLL_OUT_SELECT_148MHz	(1)
#define SCE_SYSREG_PLL_OUT_SELECT_190MHz	(2)
#define SCE_SYSREG_PLL_OUT_SELECT_222MHz	(3)
#define SCE_SYSREG_PLL_OUT_SELECT_266MHz	(4)
#define SCE_SYSREG_PLL_OUT_SELECT_333MHz	(5)
#define SCE_SYSREG_PLL_OUT_SELECT_19MHz		(8)
#define SCE_SYSREG_PLL_OUT_SELECT_74MHz		(9)
#define SCE_SYSREG_PLL_OUT_SELECT_96MHz		(10)
#define SCE_SYSREG_PLL_OUT_SELECT_111MHz	(11)
#define SCE_SYSREG_PLL_OUT_SELECT_133MHz	(12)
#define SCE_SYSREG_PLL_OUT_SELECT_166MHz	(13)
s32 sceSysregPllGetOutSelect(void);

u64 sceSysregGetFuseId(void);
u32 sceSysregGetFuseConfig(void);

