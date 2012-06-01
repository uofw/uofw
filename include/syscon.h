/* Copyright (C) 2011, 2012 The uOFW team
   See the file COPYING for copying permission.
*/

#include "common.h"

/* syscon hardware controller transfer modes. */
#define SYSCON_CTRL_ONLY_DIGITAL_DATA_TRANSFER    7
#define SYSCON_CTRL_ANALOG_DIGITAL_DATA_TRANSFER  8

#define PSP_SYSCON_TX_CMD (0)
#define PSP_SYSCON_TX_LEN (1)
#define PSP_SYSCON_TX_DATA(i) (2 + i)

#define PSP_SYSCON_RX_STATUS (0)
#define PSP_SYSCON_RX_LEN (1)
#define PSP_SYSCON_RX_RESPONSE (2)
#define PSP_SYSCON_RX_DATA(i) (3 + i)

typedef struct SceSysconPacket {
    /** Next packet in the list. */
    struct SceSysconPacket *next;
    /** Status (probably only modified internally) */
    u32 status;
    /** Packet synchronization semaphore ID */
    SceUID semaId;
    /** Transmitted data.
     * First byte is command number,
     * second one is the transmitted data length,
     * the rest is data depending on the command.
     */
    u8 tx[16];
    /** Received data.
     * First byte is status (probably, unused),
     * second one is the received data length,
     * third one is response code (?),
     * the rest is data depending on the command.
     */
    u8 rx[16];
    /** Callback ran after a GPIO interrupt, probably after the packet has been executed. */
    s32 (*callback)(struct SceSysconPacket *, void *argp);
    /** GP value to use in the callback. */
    u32 gp;
    /** Second argument passed to the callback. */
    void *argp;
    /** Current time when the packet was started. */
    u32 time;
    /** Some kind of timeout when running the packet. */
    u32 delay;
    /** Reserved for internal (hardware) use. */
    u8 reserved[32];
} SceSysconPacket;

typedef void (*SceSysconFunc)(s32 enable, void *argp);

typedef struct {
    s32 size; // probably (unused)
    void (*start)(SceSysconPacket *packet);
    void (*end)(SceSysconPacket *packet);
} SceSysconDebugHandlers;

s32 sceSysconInit(void);
s32 sceSysconEnd(void);
s32 sceSysconResume(void *arg0);
s32 sceSysconCmdExec(SceSysconPacket *packet, s32 unk);
s32 sceSysconCmdExecAsync(SceSysconPacket *packet, u32 flags, s32 (*callback)(SceSysconPacket*, void*), void *argp);
s32 sceSysconCmdCancel(SceSysconPacket *packet);
s32 sceSysconCmdSync(SceSysconPacket *packet, s32 arg1);
s32 sceSyscon_driver_90EAEA2B(s32 arg0, s32 arg1);
s32 sceSyscon_driver_755CF72B(s32 *arg0, s32 *arg1);
s32 sceSysconSuspend(void);
s32 sceSysconSetDebugHandlers(SceSysconDebugHandlers *handlers);
s32 sceSysconSetPollingMode(u32 pollingMode);
s32 sceSysconSetAffirmativeRertyMode(u32 mode);
s32 _sceSysconGetBaryonVersion(void);
u64 _sceSysconGetBaryonTimeStamp(void);
s32 _sceSysconGetUsbPowerType(void);
s32 sceSysconSetGSensorCallback(SceSysconFunc func, void *argp);
s32 sceSysconSetLowBatteryCallback(SceSysconFunc func, void *argp);
s32 sceSysconSetPowerSwitchCallback(SceSysconFunc func, void *argp);
s32 sceSysconSetAlarmCallback(SceSysconFunc func, void *argp);
s32 sceSysconSetAcSupplyCallback(SceSysconFunc func, void *argp);
s32 sceSysconSetAcSupply2Callback(SceSysconFunc func, void *argp);
s32 sceSysconSetHPConnectCallback(SceSysconFunc func, void *argp);
s32 sceSysconSetHRPowerCallback(SceSysconFunc func, void *argp);
s32 sceSysconSetHRWakeupCallback(SceSysconFunc func, void *argp);
s32 sceSysconSetWlanSwitchCallback(SceSysconFunc func, void *argp);
s32 sceSysconSetWlanPowerCallback(SceSysconFunc func, void *argp);
s32 sceSysconSetBtSwitchCallback(SceSysconFunc func, void *argp);
s32 sceSysconSetBtPowerCallback(SceSysconFunc func, void *argp);
s32 sceSysconSetHoldSwitchCallback(SceSysconFunc func, void *argp);
s32 sceSysconSetUmdSwitchCallback(SceSysconFunc func, void *argp);
s32 sceSyscon_driver_374373A8(SceSysconFunc func, void *argp);
s32 sceSyscon_driver_B761D385(SceSysconFunc func, void *argp);
s32 sceSyscon_driver_26307D84(SceSysconFunc func, void *argp);
s32 sceSyscon_driver_6C388E02(SceSysconFunc func, void *argp);
u8 sceSysconGetBaryonStatus(void);
u8 sceSysconGetBaryonStatus2(void);
u8 sceSysconIsFalling(void);
u8 sceSysconIsLowBattery(void);
u8 sceSysconGetPowerSwitch(void);
u8 sceSysconIsAlarmed(void);
u8 sceSyscon_driver_A3406117(void);
s8 sceSysconGetHPConnect(void);
s8 sceSysconGetWlanSwitch(void);
s8 sceSyscon_driver_0B51E34D(s8 wlanSwitch);
s8 sceSysconGetBtSwitch(void);
s8 sceSyscon_driver_BADF1260(s8 btSwitch);
s8 sceSysconGetHoldSwitch(void);
s8 sceSysconGetUmdSwitch(void);
s8 sceSyscon_driver_248335CD(void);
s8 sceSyscon_driver_040982CD(void);
s8 sceSyscon_driver_97765E27(void);
u8 sceSysconGetHRPowerStatus(void);
u8 sceSysconGetHRWakeupStatus(void);
u8 sceSysconGetWlanPowerStatus(void);
u8 sceSysconGetBtPowerStatus(void);
u8 sceSyscon_driver_DF20C984(void);
s8 sceSysconGetLeptonPowerCtrl(void);
s8 sceSysconGetMsPowerCtrl(void);
s8 sceSysconGetWlanPowerCtrl(void);
s8 sceSysconGetHddPowerCtrl(void);
s8 sceSysconGetDvePowerCtrl(void);
s8 sceSysconGetBtPowerCtrl(void);
s8 sceSysconGetUsbPowerCtrl(void);
s8 sceSysconGetTachyonVmePowerCtrl(void);
s8 sceSysconGetTachyonAwPowerCtrl(void);
s8 sceSysconGetTachyonAvcPowerCtrl(void);
s8 sceSysconGetLcdPowerCtrl(void);
s8 sceSysconGetHRPowerCtrl(void);
s8 sceSysconGetWlanLedCtrl(void);
s32 sceSysconGetTimeStamp(s8 *timeStamp);
s32 sceSysconWriteScratchPad(u32 dst, void *src, u32 size);
s32 sceSysconReadScratchPad(u32 src, void *dst, u32 size);
s32 sceSysconSendSetParam(u32 id, void *param);
s32 sceSysconReceiveSetParam(u32 id, void *param);
s32 sceSysconCtrlTachyonWDT(s32 wdt);
s32 sceSysconResetDevice(u32 reset, u32 mode);
s32 sceSyscon_driver_12518439(u32 arg0);
s32 sceSysconPowerSuspend(u32 arg0, u32 arg1);
void sceSysconNop(void);
s32 sceSysconGetBaryonVersion(s32 *baryonVersion);
s32 sceSysconGetGValue(void);
s32 sceSysconGetPowerSupplyStatus(void *status);
s32 sceSysconGetFallingDetectTime(void);
s32 sceSysconGetWakeUpFactor(void *factor);
s32 sceSysconGetWakeUpReq(void *req);
s32 sceSysconGetVideoCable(s32 *cable);
s32 sceSysconReadClock(s32 *clock);
s32 sceSysconWriteClock(s32 clock);
s32 sceSysconReadAlarm(s32 *alarm);
s32 sceSysconWriteAlarm(s32 alarm);
s32 sceSysconSetUSBStatus(u8 status);
s32 sceSysconGetTachyonWDTStatus(s32 *status);
s32 sceSysconCtrlAnalogXYPolling(s8 polling);
s32 sceSysconCtrlHRPower(s8 power);
s32 sceSysconCtrlPower(u32 arg0, u32 arg1);
s32 sceSysconCtrlLED(u32 arg0, u32 arg1);
s32 sceSysconCtrlDvePower(s8 power);
s32 sceSyscon_driver_765775EB(s32 arg0);
s32 sceSysconCtrlCharge(u8 allow);
s32 sceSysconCtrlTachyonAvcPower(s8 power);
s32 sceSysconGetPommelVersion(s32 *pommel);
s32 sceSyscon_driver_FB148FB6(void *arg0);
s32 sceSysconCtrlVoltage(s32 arg0, s32 arg1);
s32 sceSysconGetGSensorVersion(void);
s32 sceSysconCtrlGSensor(void);
s32 sceSysconGetPowerStatus(s32 *status);
s32 sceSysconWritePommelReg(u32 arg0, u32 arg1);
s32 sceSysconReadPommelReg(u8 reg, s32 *value);
s32 sceSysconGetPowerError(s32 *error);
s32 sceSysconCtrlLeptonPower(s8 power);
s32 sceSysconCtrlMsPower(s8 power);
s32 sceSysconCtrlWlanPower(s8 power);
s32 sceSysconCtrlHddPower(s8 power);
s32 sceSysconCtrlBtPower(s8 power);
s32 sceSysconCtrlUsbPower(s8 power);
s32 sceSysconPermitChargeBattery(void);
s32 sceSysconForbidChargeBattery(void);
s32 sceSysconCtrlTachyonVmePower(s8 power);
s32 sceSysconCtrlTachyonAwPower(s8 power);
s32 sceSysconCtrlLcdPower(s8 power);
s32 sceSysconGetGSensorCarib(void);
s32 sceSysconSetGSensorCarib(void);
s32 sceSysconWritePolestarReg(s8 reg, u32 val);
s32 sceSysconReadPolestarReg(s8 reg, s32 *val);
s32 sceSysconWriteGSensorReg(void);
s32 sceSysconReadGSensorReg(void);
s32 sceSysconBatteryGetStatusCap(s32 *arg0, s32 *arg1);
s32 sceSysconBatteryGetInfo(s32 *info);
s32 sceSysconGetBattVolt(s32 *volt);
s32 sceSysconGetBattVoltAD(s32 *volt1, s32 *volt2);
s32 sceSysconBatteryNop(void);
s32 sceSysconBatteryGetTemp(s32 *temp);
s32 sceSysconBatteryGetVolt(s32 *volt);
s32 sceSysconBatteryGetElec(s32 *elec);
s32 sceSysconBatteryGetRCap(s32 *rcap);
s32 sceSysconBatteryGetCap(s32 *cap);
s32 sceSysconBatteryGetFullCap(s32 *cap);
s32 sceSysconBatteryGetIFC(s32 *ifc);
s32 sceSysconBatteryGetLimitTime(s32 *time);
s32 sceSysconBatteryGetStatus(s32 *status);
s32 sceSysconBatteryGetCycle(s32 *cycle);
s32 sceSysconBatteryGetSerial(s32 *serial);
s32 sceSysconBatteryGetTempAD(s32 *temp);
s32 sceSysconBatteryGetVoltAD(s32 *volt);
s32 sceSysconBatteryGetElecAD(s32 *elec);
s32 sceSysconBatteryGetTotalElec(s32 *elec);
s32 sceSysconBatteryGetChargeTime(s32 *time);
s32 sceSysconCtrlTachyonVoltage(s32 voltage);
s32 sceSysconGetDigitalKey(s8 *key);

