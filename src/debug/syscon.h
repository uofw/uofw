#ifndef __SYSCON_H__
#define __SYSCON_H__ 

#include <common_imp.h>

/*
	CMD 07,08 : CTRL data

	all bits are active LOW

	+03-06:buttons (cmd 07/08)
	+07   :Analog  (cmd 08)
	+08   :Analog  (cmd 08)
*/
#define SYSCON_CTRL_ALLOW_UP  0x00000001
#define SYSCON_CTRL_ALLOW_RT  0x00000002
#define SYSCON_CTRL_ALLOW_DN  0x00000004
#define SYSCON_CTRL_ALLOW_LT  0x00000008
#define SYSCON_CTRL_TRIANGLE  0x00000010
#define SYSCON_CTRL_CIRCLE    0x00000020
#define SYSCON_CTRL_CROSS     0x00000040
#define SYSCON_CTRL_RECTANGLE 0x00000080
#define SYSCON_CTRL_SELECT    0x00000100
#define SYSCON_CTRL_LTRG      0x00000200
#define SYSCON_CTRL_RTRG      0x00000400
#define SYSCON_CTRL_START     0x00000800
#define SYSCON_CTRL_HOME      0x00001000
#define SYSCON_CTRL_HOLD      0x00002000
#define SYSCON_CTRL_WLAN      0x00004000
#define SYSCON_CTRL_HR_EJ     0x00008000
#define SYSCON_CTRL_VOL_UP    0x00010000
#define SYSCON_CTRL_VOL_DN    0x00020000
#define SYSCON_CTRL_LCD       0x00040000
#define SYSCON_CTRL_NOTE      0x00080000
#define SYSCON_CTRL_UMD_EJCT  0x00100000
#define SYSCON_CTRL_UNKNOWN   0x00200000 /* is not-service mode ? */

/*
	receive +00 : generic status

  bit 0 : AC power input
  bit 1 : sceSysconSetWlanPowerCallback
  bit 2 : sceSysconSetHRPowerCallback
  bit 3 : sceSysconSetAlarmCallback
  bit 4 : power switch on
  bit 5 : sceSysconSetLowBatteryCallback
  bit 6 : ?
  bit 7 : ?
*/
#define SYSCON_STS_AC_ACTIVE   0x01
#define SYSCON_STS_WLAN_POW    0x02
#define SYSCON_STS_HR_POW      0x04
#define SYSCON_STS_ALARM       0x08
#define SYSCON_STS_POWER_SW_ON 0x10
#define SYSCON_STS_LOW_BATTERY 0x20

/*
	receive +02 : response code

	00-7f : response of COMMAND 00-7f
	80-8f : done / error code
*/

// 1.Receive 1st packet of twice command , send same command again
// 2.CHECK SUM error
#define SYSCON_RES_80 0x80

// SYSCON BUSY , send same command again
#define SYSCON_RES_81 0x81

// Done 2nd packet of twice command
#define SYSCON_RES_OK 0x82

// parameter size error
#define SYSCON_RES_83 0x83

// twice send command error
#define SYSCON_RES_86 0x86

/*
	pspSysconCtrlPower device val
*/
#define SYSCON_DEV_ON  (1<<23)
#define SYSCON_DEV_LCD 0x00080000

/*
	func
*/

int pspSyscon_init(void);
int Syscon_cmd(u8 *tx_buf,u8 *rx_buf);

u32 Syscon_wait(u32 usec);

int pspSyscon_tx_noparam(u8 cmd);
int pspSyscon_tx_dword(u32 param,u8 cmd,u8 tx_len);
int pspSyscon_rx_dword(u32 *param,u8 cmd);

/*
SYSCON COMMAND
command code : write size (within cmd and ws) : read size  (without sts,len):
*/

static inline int pspSysconNop(void){ return pspSyscon_tx_noparam(0x00); }								// 00 : 2 : 3
static inline int pspSyscon_driver_Unkonow_7ec5a957(u32 *ptr){ return pspSyscon_rx_dword(ptr,0x01); }	// 01 : 2 : 7
static inline int pspSyscon_driver_Unkonow_34c36ff9(u32 *ptr){ return pspSyscon_rx_dword(ptr,0x02); }	// 02 : 2 : 5
static inline int pspSyscon_driver_Unkonow_3b657a27(u32 *ptr){ return pspSyscon_rx_dword(ptr,0x05); }	// 05 : 2 : 7
//06 :  ? :  ? : (ctrl read?)
static inline int _pspSysconGetCtrl1(u32 *ctrl){ return pspSyscon_rx_dword(ctrl,0x07); }				// 07 : 2 : 7
              int _pspSysconGetCtrl2(u32 *ctrl,u8 *vol1,u8 *vol2);										// 08 : 2 : 9 (ctrl read w. AnaStick)
              
static inline int pspSysconReadClock(u32 *ptr){ return pspSyscon_rx_dword(ptr,0x09); }					// 09 : 2 : 7
static inline int pspSysconReadAlarm(u32 *ptr){ return pspSyscon_rx_dword(ptr,0x0a); }				// 0A : 2 : 7
static inline int pspSyscon_driver_Unkonow_fc32141a(u32 *ptr){ return pspSyscon_rx_dword(ptr,0x0b); }// 0B : 2 : 7
static inline int pspSyscon_driver_Unkonow_882f0aab(u32 *ptr){ return pspSyscon_rx_dword(ptr,0x0c); }// 0C : 2 : 7
// 0D
static inline int pspSyscon_driver_Unkonow_f775bc34(u32 *ptr){ return pspSyscon_rx_dword(ptr,0x0e); }// 0E : 2 : 7
static inline int pspSyscon_driver_Unkonow_a9aef39f(u32 *ptr){ return pspSyscon_rx_dword(ptr,0x0f); }// 0F : 2 : 7
// 10
static inline int pspSyscon_driver_Unkonow_7bcc5eae(u32 *ptr){ return pspSyscon_rx_dword(ptr,0x11); }// 11 : 2 : ?
static inline int pspSysconMsOn(void) { return pspSyscon_tx_noparam(0x1c); }							// 1C : 2 : 3
static inline int pspSysconMsOff(void) { return pspSyscon_tx_noparam(0x1d); }							// 1D : 2 : ?
static inline int pspSysconWlanOn(void) { return pspSyscon_tx_noparam(0x1e); }							// 1E : 2 : ?
static inline int pspSysconWlanOff(void) { return pspSyscon_tx_noparam(0x1f); }						// 1F : 2 : ?
static inline int pspSysconWriteClock(u32 val){ return pspSyscon_tx_dword(val,0x20,6); }				// 20 : 6 : 3
// 21
static inline int pspSysconWriteAlarm(u32 val){ return pspSyscon_tx_dword(val,0x22,6); }				// 22 : 6 : 3
//23 :  ? :  ? : pspSyscon_driver_Unkonow_65eb6096 (RTC)
//24 :  3 ;  ? : pspSyscon_driver_Unkonow_eb277c88 (RTC)
//25 :  a :  0 : pspSysconSendSetParam
//26 :  2 :  ? : pspSysconReceiveSetParam
//27-30
//31 :  3 :  0 : pspSyscon_driver_Unkonow_2ee82492
static inline int pspSysconResetDevice(u8 val){ return pspSyscon_tx_dword(val,0x32,3); }	// 32 : 3 : 3
static inline int _pspSysconCtrlAStickPower(u8 sw){ return pspSyscon_tx_dword(sw,0x33,3); }	// 33 : 3 : 3 pspSyscon_driver_Unkonow_00e7b6c2(ctrl.prx)
static inline int pspSysconCtrlHRPower(u8 sw){ return pspSyscon_tx_dword(sw,0x34,3); }		// 34 : 3 : 3
static inline void pspSysconPowerStandby(void){ pspSyscon_tx_noparam(0x35); }				// 35 : 2 : 3
static inline void pspSysconPowerSuspend(void){ pspSyscon_tx_noparam(0x36); }				// 36 : 2 : 3

static inline int pspSyscon_driver_Unkonow_e7e87741(u32 *ptr){ return pspSyscon_rx_dword(ptr,0x40); }	// 40 : 2 : 4-7
static inline int pspSyscon_driver_Unkonow_fb148fb6(u32 *ptr){ return pspSyscon_rx_dword(ptr,0x41); }	// 41 : 2 : 4-7
static inline int pspSysconCtrlVoltage(u32 val){ return pspSyscon_tx_dword(val,0x42,5); }		// 42 : 5 : 3
static inline int pspSysconCtrlPower(u32 val){ return pspSyscon_tx_dword(val,0x45,5); }		// 45 : 5 : 3
static inline int pspSysconGetPowerStatus(u32 *sts){ return pspSyscon_rx_dword(sts,0x46); }	// 46 : 2 : ?
              int pspSysconCtrlLED(int sel,int is_on);										// 47 : 3 : 3
static inline int pspSysconResolverPowerdown(u32 val){ return pspSyscon_tx_dword(val,0x48,5); }// 48 :  5 :  3
static inline int pspSyscon_driver_Unkonow_3de38336(u32 val){ return pspSyscon_tx_dword(val,0x49,3); }// 49 :  3 :  2
static inline int pspSysconGetPowerError(u32 *ptr){ return pspSyscon_rx_dword(ptr,0x4a); }		// 4a : 2 : 7
static inline int pspSysconCtrlLeptonPower(u32 val){ return pspSyscon_tx_dword(val,0x49,3); }	// 4B : 3 : 3
static inline int pspSysconCtrlMsPower(u8 sw){ return pspSyscon_tx_dword(sw,0x4c,3); }			// 4C : 3 : 3
static inline int pspSysconCtrlWlanPower(u32 val){ return pspSyscon_tx_dword(val,0x4d,3); }	// 4d : 3 : 3

// pspSyscon_driver_Unkonow_806d4d6c
// pspSyscon_driver_Unkonow_eab13fbe
// pspSyscon_driver_Unkonow_c5075828
static inline int pspSyscon_CMD4e(u32 val){ return pspSyscon_tx_dword(val,0x4e,5); }			// 4e : 5 : 3

//4f :  3 :  5 :
// pspSyscon_driver_Unkonow_d8471760
// pspSyscon_driver_Unkonow_eab13fbe
// pspSyscon_driver_Unkonow_c5075828

static inline int pspSysconBatteryNop(u32 val){ return pspSyscon_tx_noparam(0x60); }			// 60 : 2 : 3
static inline int pspSysconBatteryGetStatusCap(u32 *ptr){ return pspSyscon_rx_dword(ptr,0x61); }// 61 : 2 : 6
static inline int pspSysconBatteryGetTemp(u32 *ptr){ return pspSyscon_rx_dword(ptr,0x62); }		// 62 : 2 : 4-7
static inline int pspSysconBatteryGetVolt(u32 *ptr){ return pspSyscon_rx_dword(ptr,0x63); }		// 63 : 2 : 4-7
static inline int pspSysconBatteryGetElec(u32 *ptr){ return pspSyscon_rx_dword(ptr,0x64); }		// 64 : 2 : 4-7
static inline int pspSysconBatteryGetRCap(u32 *ptr){ return pspSyscon_rx_dword(ptr,0x65); }		// 65 : 2 : 4-7
static inline int pspSysconBatteryGetCap(u32 *ptr){ return pspSyscon_rx_dword(ptr,0x66); }		// 66 : 2 : 4-7
static inline int pspSysconBatteryGetFullCap(u32 *ptr){ return pspSyscon_rx_dword(ptr,0x67); }	// 67 : 2 : 4-7
static inline int pspSysconBatteryGetIFC(u32 *ptr){ return pspSyscon_rx_dword(ptr,0x68); }		// 68 : 2 : 4-7
static inline int pspSysconBatteryGetLimitTime(u32 *ptr){ return pspSyscon_rx_dword(ptr,0x69); }// 69 : 2 : 4-7
static inline int pspSysconBatteryGetStatus(u32 *ptr){ return pspSyscon_rx_dword(ptr,0x6a);}	// 6a : 2 : 4-7
static inline int pspSysconBatteryGetCycle(u32 *ptr){ return pspSyscon_rx_dword(ptr,0x6b); }	// 6b : 2 : 4-7
static inline int pspSysconBatteryGetSerial(u32 *ptr){ return pspSyscon_rx_dword(ptr,0x6c); }	// 6c : 2 : 4-7
static inline int pspSysconBatteryGetInfo(u32 *ptr){ return pspSyscon_rx_dword(ptr,0x6c); }		// 6d : 2 : 8
static inline int pspSysconBatteryGetTempAD(u32 *ptr){ return pspSyscon_rx_dword(ptr,0x6e);}	// 6e : 2 : 4-7
static inline int pspSysconBatteryGetVoltAD(u32 *ptr){ return pspSyscon_rx_dword(ptr,0x6f);}	// 6f : 2 : 4-7
static inline int pspSysconBatteryGetElecAD(u32 *ptr){ return pspSyscon_rx_dword(ptr,0x70);}	// 70 : 2 : 4-7
static inline int pspSysconBatteryGetTotalElec(u32 *ptr){ return pspSyscon_rx_dword(ptr,0x71);}	// 71 : 2 : 4-7
static inline int pspSysconBatteryGetChargeTime(u32 *ptr){ return pspSyscon_rx_dword(ptr,0x72);}// 72 : 2 : 4-7
// 73 :  5 :  3 : pspSyscon_driver_Unkonow_1165c864
// 74 :  3 :  6?: pspSyscon_driver_Unkonow_68ef0bef
// 7f :  3 :  4 : pspSysconBatteryAuth

#endif
