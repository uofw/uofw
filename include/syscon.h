/* Copyright (C) 2011, 2012 The uOFW team
   See the file COPYING for copying permission.
*/

/**
 * @author artart78
 * @version 6.60
 *
 * The syscon.prx module RE'ing.
 */

#include "common_header.h"

/** @defgroup Syscon Syscon Module
 *
 * @{
 */

#define PSP_SYSCON_CMD_NOP                           0x00
#define PSP_SYSCON_CMD_GET_BARYON                    0x01
#define PSP_SYSCON_CMD_GET_DIGITAL_KEY               0x02
#define PSP_SYSCON_CMD_GET_ANALOG                    0x03

#define PSP_SYSCON_CMD_GET_DIGITAL_KEY_ANALOG        0x06
#define PSP_SYSCON_CMD_GET_KERNEL_DIGITAL_KEY        0x07
#define PSP_SYSCON_CMD_GET_KERNEL_DIGITAL_KEY_ANALOG 0x08
#define PSP_SYSCON_CMD_READ_CLOCK                    0x09
#define PSP_SYSCON_CMD_READ_ALARM                    0x0A
#define PSP_SYSCON_CMD_GET_POWER_SUPPLY_STATUS       0x0B
#define PSP_SYSCON_CMD_GET_TACHYON_WDT_STATUS        0x0C
#define PSP_SYSCON_CMD_GET_BATT_VOLT                 0x0D
#define PSP_SYSCON_CMD_GET_WAKE_UP_FACTOR            0x0E
#define PSP_SYSCON_CMD_GET_WAKE_UP_REQ               0x0F
#define PSP_SYSCON_CMD_GET_STATUS2                   0x10
#define PSP_SYSCON_CMD_GET_TIMESTAMP                 0x11
#define PSP_SYSCON_CMD_GET_VIDEO_CABLE               0x12

#define PSP_SYSCON_CMD_WRITE_CLOCK                   0x20
#define PSP_SYSCON_CMD_SET_USB_STATUS                0x21
#define PSP_SYSCON_CMD_WRITE_ALARM                   0x22
#define PSP_SYSCON_CMD_WRITE_SCRATCHPAD              0x23
#define PSP_SYSCON_CMD_READ_SCRATCHPAD               0x24
#define PSP_SYSCON_CMD_SEND_SETPARAM                 0x25
#define PSP_SYSCON_CMD_RECEIVE_SETPARAM              0x26

#define PSP_SYSCON_CMD_CTRL_BT_POWER_UNK1            0x29
#define PSP_SYSCON_CMD_CTRL_BT_POWER_UNK2            0x2A

#define PSP_SYSCON_CMD_CTRL_TACHYON_WDT              0x31
#define PSP_SYSCON_CMD_RESET_DEVICE                  0x32
#define PSP_SYSCON_CMD_CTRL_ANALOG_XY_POLLING        0x33
#define PSP_SYSCON_CMD_CTRL_HR_POWER                 0x34

#define PSP_SYSCON_CMD_GET_BATT_VOLT_AD              0x37

#define PSP_SYSCON_CMD_GET_POMMEL_VERSION            0x40
#define PSP_SYSCON_CMD_GET_POLESTAR_VERSION          0x41
#define PSP_SYSCON_CMD_CTRL_VOLTAGE                  0x42

#define PSP_SYSCON_CMD_CTRL_POWER                    0x45
#define PSP_SYSCON_CMD_GET_POWER_STATUS              0x46
#define PSP_SYSCON_CMD_CTRL_LED                      0x47
#define PSP_SYSCON_CMD_WRITE_POMMEL_REG              0x48
#define PSP_SYSCON_CMD_READ_POMMEL_REG               0x49
#define PSP_SYSCON_CMD_CTRL_HDD_POWER                0x4A
#define PSP_SYSCON_CMD_CTRL_LEPTON_POWER             0x4B
#define PSP_SYSCON_CMD_CTRL_MS_POWER                 0x4C
#define PSP_SYSCON_CMD_CTRL_WLAN_POWER               0x4D
#define PSP_SYSCON_CMD_WRITE_POLESTAR_REG            0x4E
#define PSP_SYSCON_CMD_READ_POLESTAR_REG             0x4F

#define PSP_SYSCON_CMD_CTRL_DVE_POWER                0x52
#define PSP_SYSCON_CMD_CTRL_BT_POWER                 0x53

#define PSP_SYSCON_CMD_CTRL_USB_POWER                0x55
#define PSP_SYSCON_CMD_CTRL_CHARGE                   0x56

#define PSP_SYSCON_CMD_BATTERY_NOP                   0x60
#define PSP_SYSCON_CMD_BATTERY_GET_STATUS_CAP        0x61
#define PSP_SYSCON_CMD_BATTERY_GET_TEMP              0x62
#define PSP_SYSCON_CMD_BATTERY_GET_VOLT              0x63
#define PSP_SYSCON_CMD_BATTERY_GET_ELEC              0x64
#define PSP_SYSCON_CMD_BATTERY_GET_RCAP              0x65
#define PSP_SYSCON_CMD_BATTERY_GET_CAP               0x66
#define PSP_SYSCON_CMD_BATTERY_GET_FULL_CAP          0x67
#define PSP_SYSCON_CMD_BATTERY_GET_IFC               0x68
#define PSP_SYSCON_CMD_BATTERY_GET_LIMIT_TIME        0x69
#define PSP_SYSCON_CMD_BATTERY_GET_STATUS            0x6A
#define PSP_SYSCON_CMD_BATTERY_GET_CYCLE             0x6B
#define PSP_SYSCON_CMD_BATTERY_GET_SERIAL            0x6C
#define PSP_SYSCON_CMD_BATTERY_GET_INFO              0x6D
#define PSP_SYSCON_CMD_BATTERY_GET_TEMP_AD           0x6E
#define PSP_SYSCON_CMD_BATTERY_GET_VOLT_AD           0x6F
#define PSP_SYSCON_CMD_BATTERY_GET_ELEC_AD           0x70
#define PSP_SYSCON_CMD_BATTERY_GET_TOTAL_ELEC        0x71
#define PSP_SYSCON_CMD_BATTERY_GET_CHARGE_TIME       0x72

#define PSP_SYSCON_TX_CMD (0)
#define PSP_SYSCON_TX_LEN (1)
#define PSP_SYSCON_TX_DATA(i) (2 + (i))

#define PSP_SYSCON_RX_STATUS (0)
#define PSP_SYSCON_RX_LEN (1)
#define PSP_SYSCON_RX_RESPONSE (2)
#define PSP_SYSCON_RX_DATA(i) (3 + (i))

/** A system controller packet, used to run a syscon command. */
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

/** A system controller callback. */
typedef void (*SceSysconFunc)(s32 enable, void *argp);

/** A set of debug handlers for syscon, that you can set in sceSysconSetDebugHandlers(). */
typedef struct {
    /** Structure size (probably, unused). */
    s32 size;
    /** Callback ran right before running a packet, with a pointer to it passed as the first argument. */
    void (*start)(SceSysconPacket *packet);
    /** Callback ran right after finishing running a packet, with a pointer to it passed as the first argument. */
    void (*end)(SceSysconPacket *packet);
} SceSysconDebugHandlers;

/**
 * Initialize the system controller.
 *
 * @return 0.
 */
s32 sceSysconInit(void);

/**
 * End the system controller.
 *
 * @return 0.
 */
s32 sceSysconEnd(void);

/**
 * Resume the system controller.
 *
 * @return 0.
 */
s32 sceSysconResume(void *arg0);

/**
 * Execute synchronously a syscon packet.
 *
 * @param packet The packet to execute. Its tx member needs to be initialized.
 * @param flags The packet flags. Check ScesysconPacketFlags.
 *
 * @return 0 on success.
 */
s32 sceSysconCmdExec(SceSysconPacket *packet, u32 flags);

/**
 * Execute asynchronously a syscon packet.
 *
 * @param packet The packet to execute. Its tx member needs to be initialized.
 * @param flags The packet flags. Check SceSysconPacketFlags.
 * @param callback The packet callback. Check the callback member of SceSysconPacket.
 * @param argp The second argument that will be passed to the callback when executed.
 *
 * @return 0 on success.
 */
s32 sceSysconCmdExecAsync(SceSysconPacket *packet, u32 flags, s32 (*callback)(SceSysconPacket*, void*), void *argp);

/**
 * Cancel a syscon packet which has been added for execution.
 *
 * @param packet The same packet that was passed to sceSysconCmdExec or sceSysconCmdExecAsync.
 *
 * @return 0 on success.
 */
s32 sceSysconCmdCancel(SceSysconPacket *packet);

/**
 * Wait for the currently queued syscon packets to be executed, or check if any are in the queue.
 *
 * @param packet The packet you want to check or wait for, or NULL if you want to check or wait for all the currently running packets.
 * @param noWait Set to 1 if you just want to check the packet status, or 0 if you want the function to return only when the packet queue is empty.
 *
 * @return 1 if packets are still running (and noWait was set to 1), 0 on success, < 0 otherwise.
 */
s32 sceSysconCmdSync(SceSysconPacket *packet, u32 noWait);

/**
 * Set the packet start timeout when waiting for the queue to leave room.
 * After it, packet will be set to current, whatever is the current packet, if polling mode isn't 0 (for interrupts) or if retry mode isn't 0 (for normal sceSysconCmdExec / sceSysconCmdExecAsync).
 *
 * @param timeout The timeout when a packet is ran using sceSysconCmdExec or sceSysconCmdExecAsync.
 * @param intrTimeout The timeout when running the next packet from the queue.
 *
 * @return 0.
 */
s32 sceSyscon_driver_90EAEA2B(u32 timeout, u32 intrTimeout);

/**
 * Get the packet start timeouts (check sceSyscon_driver_90EAEA2B for details).
 *
 * @param timeoutPtr A pointer which will be filled with the packet start timeout if non-NULL.
 * @param intrTimeoutPtr A pointer which will be filled with the interrupt packet start timeout if non-NULL.
 *
 * @return 0.
 */
s32 sceSyscon_driver_755CF72B(u32 *timeoutPtr, u32 *intrTimeoutPtr);

/**
 * Suspend the system controller.
 */
s32 sceSysconSuspend(void);

/**
 * Set the debug handlers.
 *
 * @return 0.
 */
s32 sceSysconSetDebugHandlers(SceSysconDebugHandlers *handlers);

/**
 * Set the polling mode.
 *
 * @param pollingMode Should be the same state as the interrupt controller.
 * If set to 0, the module will use semaphores for synchronization, add the "wait" syscon command in the queue, use a constant timeout of 4ms, etc.
 *
 * @return 0.
 */
s32 sceSysconSetPollingMode(u32 pollingMode);

/**
 * Enable retrying packet execution.
 *
 * @param mode If set to 1, the syscon commands which "failed" will be ran again.
 *
 * @return 0.
 */
s32 sceSysconSetAffirmativeRertyMode(u32 mode);

/**
 * Get the baryon version.
 *
 * @return The baryon version.
 */
s32 _sceSysconGetBaryonVersion(void);

/**
 * Get the baryon timestamp.
 *
 * @return The baryon timestamp.
 */
u64 _sceSysconGetBaryonTimeStamp(void);

/**
 * Get the USB power type.
 *
 * @return 0 or 1 (but probably returns 1 on all normal PSPs).
 */
s32 _sceSysconGetUsbPowerType(void);

/**
 * Set the GSensor callback, that will be ran when the GSensor state changes.
 *
 * @param callback The callback function.
 * @param argp The second argument that will be passed to the callback.
 *
 * @return 0.
 */
s32 sceSysconSetGSensorCallback(SceSysconFunc callback, void *argp);

/**
 * Set the low battery callback, that will be ran when the battery is low.
 *
 * @param callback The callback function.
 * @param argp The second argument that will be passed to the callback.
 *
 * @return 0.
 */
s32 sceSysconSetLowBatteryCallback(SceSysconFunc callback, void *argp);

/**
 * Set the power switch callback, that will be ran when the power switch state changes.
 *
 * @param callback The callback function.
 * @param argp The second argument that will be passed to the callback.
 *
 * @return 0.
 */
s32 sceSysconSetPowerSwitchCallback(SceSysconFunc callback, void *argp);

/**
 * Set the alarm callback, that will be ran when the alarm state changes.
 *
 * @param callback The callback function.
 * @param argp The second argument that will be passed to the callback.
 *
 * @return 0.
 */
s32 sceSysconSetAlarmCallback(SceSysconFunc callback, void *argp);

/**
 * Set the Ac supply callback, that will be ran when the PSP Ac power is (dis)connected (probably).
 *
 * @param callback The callback function.
 * @param argp The second argument that will be passed to the callback.
 *
 * @return 0.
 */
s32 sceSysconSetAcSupplyCallback(SceSysconFunc callback, void *argp);

/**
 * Set the second Ac supply callback, that will be ran when the 2nd PSP Ac state changes (?).
 *
 * @param callback The callback function.
 * @param argp The second argument that will be passed to the callback.
 *
 * @return 0.
 */
s32 sceSysconSetAcSupply2Callback(SceSysconFunc callback, void *argp);

/**
 * Set the headphone connect callback, that will be ran when the headphone is (dis)connected.
 *
 * @param callback The callback function.
 * @param argp The second argument that will be passed to the callback.
 *
 * @return 0.
 */
s32 sceSysconSetHPConnectCallback(SceSysconFunc callback, void *argp);

/**
 * Set the HR power callback, that will be ran when the HR power (?) state changes.
 *
 * @param callback The callback function.
 * @param argp The second argument that will be passed to the callback.
 *
 * @return 0.
 */
s32 sceSysconSetHRPowerCallback(SceSysconFunc callback, void *argp);

/**
 * Set the HR wakeup callback, that will be ran when the HR wakeup (?) state changes.
 *
 * @param callback The callback function.
 * @param argp The second argument that will be passed to the callback.
 *
 * @return 0.
 */
s32 sceSysconSetHRWakeupCallback(SceSysconFunc callback, void *argp);

/**
 * Set the wlan switch callback, that will be ran when the wlan switch changes.
 *
 * @param callback The callback function.
 * @param argp The second argument that will be passed to the callback.
 *
 * @return 0.
 */
s32 sceSysconSetWlanSwitchCallback(SceSysconFunc callback, void *argp);

/**
 * Set the wlan power callback, that will be ran when the wlan power state changes.
 *
 * @param callback The callback function.
 * @param argp The second argument that will be passed to the callback.
 *
 * @return 0.
 */
s32 sceSysconSetWlanPowerCallback(SceSysconFunc callback, void *argp);

/**
 * Set the bluetooth switch callback, that will be ran when the bluetooth switch changes.
 *
 * @param callback The callback function.
 * @param argp The second argument that will be passed to the callback.
 *
 * @return 0.
 */
s32 sceSysconSetBtSwitchCallback(SceSysconFunc callback, void *argp);

/**
 * Set the bluetooth power callback, that will be ran when the bluetooth power state changes.
 *
 * @param callback The callback function.
 * @param argp The second argument that will be passed to the callback.
 *
 * @return 0.
 */
s32 sceSysconSetBtPowerCallback(SceSysconFunc callback, void *argp);

/**
 * Set the hold switch callback, that will be ran when the hold switch state changes.
 *
 * @param callback The callback function.
 * @param argp The second argument that will be passed to the callback.
 *
 * @return 0.
 */
s32 sceSysconSetHoldSwitchCallback(SceSysconFunc callback, void *argp);

/**
 * Set the UMD switch callback, that will be ran when the UMD switch state changes.
 *
 * @param callback The callback function.
 * @param argp The second argument that will be passed to the callback.
 *
 * @return 0.
 */
s32 sceSysconSetUmdSwitchCallback(SceSysconFunc callback, void *argp);

/**
 * Set the (?) callback, that will be ran when the (?) state changes (in PSP street, related to HP remote).
 *
 * @param callback The callback function.
 * @param argp The second argument that will be passed to the callback.
 *
 * @return 0.
 */
s32 sceSyscon_driver_374373A8(SceSysconFunc callback, void *argp);

/**
 * Set the (?) callback, that will be ran when the (?) state changes (in PSP street, related to HP remote).
 *
 * @param callback The callback function.
 * @param argp The second argument that will be passed to the callback.
 *
 * @return 0.
 */
s32 sceSyscon_driver_B761D385(SceSysconFunc callback, void *argp);

/**
 * Set the (?) callback, that will be ran when the (?) state changes (unused).
 *
 * @param callback The callback function.
 * @param argp The second argument that will be passed to the callback.
 *
 * @return 0.
 */
s32 sceSyscon_driver_26307D84(SceSysconFunc callback, void *argp);

/**
 * Set the (?) callback, that will be ran when the (?) state changes (related to USB).
 *
 * @param callback The callback function.
 * @param argp The second argument that will be passed to the callback.
 *
 * @return 0.
 */
s32 sceSyscon_driver_6C388E02(SceSysconFunc callback, void *argp);

/**
 * Get the baryon status (set of flags about the syscon ctrl state).
 *
 * @return The baryon status.
 */
u8 sceSysconGetBaryonStatus(void);

/**
 * Get the "second" baryon status (set of other flags about the syscon ctrl state).
 *
 * @return The second baryon status.
 */
u8 sceSysconGetBaryonStatus2(void);

/**
 * Check if the PSP is falling.
 *
 * @return 1 if it is falling, 0 otherwise.
 */
u8 sceSysconIsFalling(void);

/**
 * Check if the battery is low.
 *
 * @return 1 if it is low, 0 otherwise.
 */
u8 sceSysconIsLowBattery(void);

/**
 * Get the power switch state.
 *
 * @return 1 if it is on, 0 otherwise.
 */
u8 sceSysconGetPowerSwitch(void);

/**
 * Check if the PSP is alarmed.
 *
 * @return 1 if it is alarmed, 0 otherwise.
 */
u8 sceSysconIsAlarmed(void);

/**
 * Check if the Ac is supplying current.
 *
 * @return 1 if it is supplying, 0 otherwise.
 */
u8 sceSysconIsAcSupplied(void);

/**
 * Get the headphone connection.
 *
 * @return 1 if the headphone is connected, 0 otherwise.
 */
s8 sceSysconGetHPConnect(void);

/**
 * Get the wlan switch state.
 *
 * @return 1 if wlan is activated, 0 otherwise.
 */
s8 sceSysconGetWlanSwitch(void);

/**
 * Set a wlan switch override value.
 *
 * @param wlanSwitch The value that will override the default wlan switch value if more than 0, or disable overriding with a less than 0 value.
 *
 * @return The former overriding value.
 */
s8 sceSyscon_driver_0B51E34D(s8 wlanSwitch);

/**
 * Get the bluetooth switch state.
 *
 * @return 1 if bluetooth is activated, 0 otherwise.
 */
s8 sceSysconGetBtSwitch(void);

/**
 * Set a bluetooth switch override value.
 *
 * @param btSwitch The value that will override the default bluetooth switch value if more than 0, or disable overriding with a less than 0 value.
 *
 * @return The former overriding value.
 */
s8 sceSyscon_driver_BADF1260(s8 btSwitch);

/**
 * Get the hold switch state.
 *
 * @return 1 if the hold switch is on, 0 otherwise.
 */
s8 sceSysconGetHoldSwitch(void);

/**
 * Get the UMD switch state.
 *
 * @return 1 if the umd switch is on, 0 otherwise.
 */
s8 sceSysconGetUmdSwitch(void);

/**
 * Get an unknown state (in PSP street, related to HP remote).
 *
 * @return 1 or 0.
 */
s8 sceSyscon_driver_248335CD(void);

/**
 * Get an unknown state (in PSP street, related to HP remote).
 *
 * @return 1 or 0.
 */
s8 sceSyscon_driver_040982CD(void);

/**
 * Get an unknown state (related to input).
 *
 * @return 1 or 0.
 */
s8 sceSyscon_driver_97765E27(void);

/**
 * Get the HP remote power status.
 *
 * @return 1 if the HP remote is powered, 0 otherwise.
 */
u8 sceSysconGetHRPowerStatus(void);

/**
 * Get the headphone remote wakeup status.
 *
 * @return 1 if it woke up (unsure), 0 otherwise.
 */
u8 sceSysconGetHRWakeupStatus(void);

/**
 * Get the wlan power status.
 *
 * @return 1 if the power is on, 0 otherwise.
 */
u8 sceSysconGetWlanPowerStatus(void);

/**
 * Get the bluetooth power status.
 *
 * @return 1 if the power is on, 0 otherwise.
 */
u8 sceSysconGetBtPowerStatus(void);

/**
 * Get an unknown state (USB-related).
 *
 * @return 1 or 0.
 */
u8 sceSyscon_driver_DF20C984(void);

/**
 * Get the UMD drive power control state.
 *
 * @return 1 if powered, 0 otherwise.
 */
s8 sceSysconGetLeptonPowerCtrl(void);

/**
 * Get the Memory Stick power control state.
 *
 * @return 1 if powered, 0 otherwise.
 */
s8 sceSysconGetMsPowerCtrl(void);

/**
 * Get the wlan power control state.
 *
 * @return 1 if powered, 0 otherwise.
 */
s8 sceSysconGetWlanPowerCtrl(void);

/**
 * Get the HDD power control state.
 *
 * @return 1 if powered, 0 otherwise.
 */
s8 sceSysconGetHddPowerCtrl(void);

/**
 * Get the DVE (video out) power control state.
 *
 * @return 1 if powered, 0 otherwise.
 */
s8 sceSysconGetDvePowerCtrl(void);

/**
 * Get the bluetooth power control state.
 *
 * @return 1 if powered, 0 otherwise.
 */
s8 sceSysconGetBtPowerCtrl(void);

/**
 * Get the USB power control state.
 *
 * @return 1 if powered, 0 otherwise.
 */
s8 sceSysconGetUsbPowerCtrl(void);

/**
 * Get the Tachyon VME power control state.
 *
 * @return 1 if powered, 0 otherwise.
 */
s8 sceSysconGetTachyonVmePowerCtrl(void);

/**
 * Get the Tachyon AW power control state.
 *
 * @return 1 if powered, 0 otherwise.
 */
s8 sceSysconGetTachyonAwPowerCtrl(void);

/**
 * Get the Tachyon AVC power control state.
 *
 * @return 1 if powered, 0 otherwise.
 */
s8 sceSysconGetTachyonAvcPowerCtrl(void);

/**
 * Get the LCD screen power control state.
 *
 * @return 1 if powered, 0 otherwise.
 */
s8 sceSysconGetLcdPowerCtrl(void);

/**
 * Get the headphone remote power control state.
 *
 * @return 1 if powered, 0 otherwise.
 */
s8 sceSysconGetHRPowerCtrl(void);

/**
 * Get the wlan LED control state.
 *
 * @return 1 if on, 0 otherwise.
 */
s8 sceSysconGetWlanLedCtrl(void);

/**
 * Get the baryon timestamp string.
 *
 * @param timeStamp A pointer to a string at least 12 bytes long.
 *
 * @return 0 on success.
 */
s32 sceSysconGetTimeStamp(s8 *timeStamp);

/**
 * Write data to the scratchpad.
 *
 * @param dst The scratchpad address to write to.
 * @param src A pointer to the data to copy to the scratchpad.
 * @param size The size of the data to copy.
 *
 * @return 0 on success.
 */
s32 sceSysconWriteScratchPad(u32 dst, void *src, u32 size);

/**
 * Read data from the scratchpad.
 *
 * @param src The scratchpad address to read from.
 * @param dst A pointer where will be copied the read data.
 * @param size The size of the data to read from the scratchpad.
 *
 * @return 0 on success.
 */
s32 sceSysconReadScratchPad(u32 src, void *dst, u32 size);

/**
 * Set a parameter (used by power).
 *
 * @param id The parameter ID (?).
 * @param param Pointer to a buffer (length 8) the parameter will be set to.
 *
 * @return 0 on success.
 */
s32 sceSysconSendSetParam(u32 id, void *param);

/**
 * Receive a parameter (used by power).
 *
 * @param id The parameter ID (?).
 * @param param Pointer to a buffer (length 8) where will be copied the parameter.
 *
 * @return 0 on success.
 */
s32 sceSysconReceiveSetParam(u32 id, void *param);

/**
 * Set the tachyon watchdog timer.
 *
 * @param wdt The timer value (0 - 0x7F).
 *
 * @return 0 on success.
 */
s32 sceSysconCtrlTachyonWDT(s32 wdt);

/**
 * Reset the device.
 *
 * @param reset The reset value, passed to the syscon.
 * @param mode The resetting mode (?).
 * 
 * @return 0 on success.
 */
s32 sceSysconResetDevice(u32 reset, u32 mode);

/**
 * (? related to power, looks a bit like sceSysconPowerSuspend)
 *
 * @param arg0 Unknown.
 *
 * @return 0.
 */
s32 sceSyscon_driver_12518439(u32 arg0);

/**
 * Suspend the PSP power.
 *
 * @param arg0 Unknown.
 * @param arg1 Unknown.
 *
 * @return 0.
 */
s32 sceSysconPowerSuspend(u32 arg0, u32 arg1);

/**
 * Send a command to the syscon doing nothing.
 *
 * @return 0 on success.
 */
s32 sceSysconNop(void);

/**
 * Get the baryon version from the syscon.
 *
 * @param baryonVersion Pointer to a s32 where the baryon version will be stored.
 *
 * @return 0 on success.
 */
s32 sceSysconGetBaryonVersion(s32 *baryonVersion);

/**
 * Debugging function, disactivated in production.
 *
 * @return < 0.
 */
s32 sceSysconGetGValue(void);

/**
 * Get the power supply status.
 *
 * @param status Pointer to a s32 where the power supply status will be stored.
 *
 * @return 0 on success.
 */
s32 sceSysconGetPowerSupplyStatus(s32 *status);

/**
 * Debugging function, disactivated in production.
 *
 * @return < 0.
 */
s32 sceSysconGetFallingDetectTime(void);

/**
 * Get the wake up factor (?).
 *
 * @param factor Pointer to a buffer where the factor will be stored.
 *
 * @return 0 on success.
 */
s32 sceSysconGetWakeUpFactor(void *factor);

/**
 * Get the wake up req (?).
 *
 * @param req Pointer to a buffer where the req will be stored.
 *
 * @return 0 on success.
 */
s32 sceSysconGetWakeUpReq(void *req);

/**
 * Get the video cable.
 *
 * @param cable Pointer to a s32 where the cable info will be stored.
 *
 * @return 0 on success.
 */
s32 sceSysconGetVideoCable(s32 *cable);

/**
 * Read the PSP clock.
 *
 * @param clock Pointer to a s32 where the clock will be stored.
 *
 * @return 0 on success.
 */
s32 sceSysconReadClock(s32 *clock);

/**
 * Write the PSP clock.
 *
 * @param clock The clock value to set the PSP clock to.
 *
 * @return 0 on success.
 */
s32 sceSysconWriteClock(s32 clock);

/**
 * Read the PSP alarm.
 *
 * @param alarm Pointer to a s32 where the alarm will be stored.
 *
 * @return 0 on success.
 */
s32 sceSysconReadAlarm(s32 *alarm);

/**
 * Set the PSP alarm.
 *
 * @param alarm The alarm value to set the PSP alarm to.
 *
 * @return 0 on success.
 */
s32 sceSysconWriteAlarm(s32 alarm);

/**
 * Set the USB status.
 *
 * @param status The new USB status.
 *
 * @return 0 on success.
 */
s32 sceSysconSetUSBStatus(u8 status);

/**
 * Get the Tachyon watchdog timer status.
 *
 * @param status Pointer to a s32 where the watchdog timer status will be stored.
 *
 * @return 0 on success.
 */
s32 sceSysconGetTachyonWDTStatus(s32 *status);

/**
 * Set the analog XY polling control.
 * 
 * @param polling The new polling value.
 *
 * @return 0 on success.
 */
s32 sceSysconCtrlAnalogXYPolling(s8 polling);

/**
 * Set the HR power control.
 *
 * @param power The new power control.
 *
 * @return 0 on success.
 */
s32 sceSysconCtrlHRPower(s8 power);

/**
 * Set the power control.
 *
 * @param arg0 Unknown.
 * @param arg1 Unknown.
 *
 * @return 0 on success.
 */
s32 sceSysconCtrlPower(u32 arg0, u32 arg1);

/**
 * Turn a LED on or off.
 *
 * @param led The LED id (0, 1, 2 or 3).
 * @param set Set this value to 1 if you want the LED to turn on, or 0 if you want it to turn off.
 *
 * @return 0 on success.
 */
s32 sceSysconCtrlLED(u32 led, u32 set);

/**
 * Set the Dve power control.
 *
 * @param power The new dve power control.
 *
 * @return 0 on success.
 */
s32 sceSysconCtrlDvePower(s8 power);

/**
 * Set an unknown value.
 *
 * @param arg0 Always set to 0 when the function is called.
 *
 * @return 0 on success.
 */
s32 sceSyscon_driver_765775EB(s32 arg0);

/**
 * Allow or disallow charge.
 *
 * @param allow Set to 1 if you want charge to be allowed, 0 otherwise.
 *
 * @return 0 on success.
 */
s32 sceSysconCtrlCharge(u8 allow);

/**
 * Set the tachyon AVC power control.
 *
 * @param power The new AVC power control.
 *
 * @return 0 on success.
 */
s32 sceSysconCtrlTachyonAvcPower(s8 power);

/**
 * Get the pommel version.
 *
 * @param pommel Pointer to a s32 where the pommel version will be stored.
 *
 * @return 0 on success.
 */
s32 sceSysconGetPommelVersion(s32 *pommel);

/**
 * Get the polestar version.
 *
 * @param polestar Pointer to a s32 where the polestar version will be stored.
 *
 * @return 0 on success.
 */
s32 sceSysconGetPolestarVersion(s32 *polestar);

/**
 * Set the voltage.
 *
 * @param arg0 Unknown.
 * @param arg1 Unknown.
 *
 * @return 0 on success.
 */
s32 sceSysconCtrlVoltage(s32 arg0, s32 arg1);

/**
 * Debugging function, disabled for production.
 *
 * @return < 0.
 */
s32 sceSysconGetGSensorVersion(void);

/**
 * Debugging function, disabled for production.
 *
 * @return < 0.
 */
s32 sceSysconCtrlGSensor(void);

/**
 * Get the power status.
 *
 * @param status Pointer to a s32 where the power status will be stored.
 *
 * @return 0 on success.
 */
s32 sceSysconGetPowerStatus(s32 *status);

/**
 * Write a value to a pommel register.
 *
 * @param reg The register id to write the value to.
 * @param value The value to write to the register.
 *
 * @return 0 on success.
 */
s32 sceSysconWritePommelReg(u8 reg, s16 value);

/**
 * Read a value from a pommel register.
 *
 * @param reg The register id to read the value from.
 * @param value Pointer to a s16 where the contents of the register will be stored.
 *
 * @return 0 on success.
 */
s32 sceSysconReadPommelReg(u8 reg, s16 *value);

/**
 * Get the power error (function seems disabled).
 *
 * @param error Pointer to a s32 where 0 will be stored, if non-NULL.
 *
 * @return 0.
 */
s32 sceSysconGetPowerError(s32 *error);

/**
 * Set the lepton power.
 *
 * @param power The new power value.
 *
 * @return 0 on success.
 */
s32 sceSysconCtrlLeptonPower(s8 power);

/**
 * Set the memory stick power.
 *
 * @param power The new power value.
 *
 * @return 0 on success.
 */
s32 sceSysconCtrlMsPower(s8 power);

/**
 * Set the wlan power.
 *
 * @param power The new power value.
 *
 * @return 0 on success.
 */
s32 sceSysconCtrlWlanPower(s8 power);

/**
 * Set the HDD power.
 *
 * @param power The new power value.
 *
 * @return 0 on success.
 */
s32 sceSysconCtrlHddPower(s8 power);

/**
 * Set the bluetooth power.
 *
 * @param power The new power value.
 *
 * @return 0 on success.
 */
s32 sceSysconCtrlBtPower(s8 power);

/**
 * Set the USB power.
 *
 * @param power The new power value.
 *
 * @return 0 on success.
 */
s32 sceSysconCtrlUsbPower(s8 power);

/**
 * Permit the battery charge.
 *
 * @return 0 on success.
 */
s32 sceSysconPermitChargeBattery(void);

/**
 * Forbid the battery charge.
 *
 * @return 0 on success.
 */
s32 sceSysconForbidChargeBattery(void);

/**
 * Set the tachyon VME power.
 *
 * @param power The new power value.
 *
 * @return 0 on success.
 */
s32 sceSysconCtrlTachyonVmePower(s8 power);

/**
 * Set the tachyon AW power.
 *
 * @param power The new power value.
 *
 * @return 0 on success.
 */
s32 sceSysconCtrlTachyonAwPower(s8 power);

/**
 * Set the LCD power.
 *
 * @param power The new power value.
 *
 * @return 0 on success.
 */
s32 sceSysconCtrlLcdPower(s8 power);

/**
 * Debugging function, disabled for production.
 *
 * @return < 0.
 */
s32 sceSysconGetGSensorCarib(void);

/**
 * Debugging function, disabled for production.
 *
 * @return < 0.
 */
s32 sceSysconSetGSensorCarib(void);

/**
 * Write a value to a polestar register.
 *
 * @param reg The register to write the value to.
 * @param val The value that will be stored in the register.
 *
 * @return 0 on success.
 */
s32 sceSysconWritePolestarReg(u8 reg, s16 val);

/**
 * Read a value from a polestar register.
 *
 * @param reg The register to read the value from.
 * @param val Pointer to a s16 where will be stored the read content of the register.
 *
 * @return 0 on sucess.
 */
s32 sceSysconReadPolestarReg(u8 reg, s16 *val);

/**
 * Debugging function, disabled for production.
 *
 * @return < 0.
 */
s32 sceSysconWriteGSensorReg(void);

/**
 * Debugging function, disabled for production.
 *
 * @return < 0.
 */
s32 sceSysconReadGSensorReg(void);

/**
 * Get the battery status cap.
 *
 * @param arg0 Pointer to an unknown s32 where a value will be stored.
 * @param arg1 Pointer to an unknown s32 where a value will be stored.
 *
 * @return 0 on success.
 */
s32 sceSysconBatteryGetStatusCap(s32 *arg0, s32 *arg1);

/**
 * Get the battery info.
 *
 * @param info Pointer to a s32 where the battery info will be stored.
 *
 * @return 0 on success.
 */
s32 sceSysconBatteryGetInfo(s32 *info);

/**
 * Get the battery voltage.
 *
 * @param volt Pointer to a s32 where the voltage will be stored.
 *
 * @return 0 on success.
 */
s32 sceSysconGetBattVolt(s32 *volt);

/**
 * Get the battery analog-digital voltage (?).
 *
 * @param volt1 Pointer to a s32 where an unknown value will be stored.
 * @param volt2 Pointer to a s32 where an unknown value will be stored.
 *
 * @return 0 on success.
 */
s32 sceSysconGetBattVoltAD(s32 *volt1, s32 *volt2);

/**
 * Send a command to the battery doing nothing.
 *
 * @return 0 on success.
 */
s32 sceSysconBatteryNop(void);

/**
 * Get the battery temperature.
 *
 * @param temp Pointer to a s32 where the temperature will be stored.
 *
 * @return 0 on success.
 */
s32 sceSysconBatteryGetTemp(s32 *temp);

/**
 * Get the battery voltage.
 *
 * @param volt Pointer to a s32 where the voltage will be stored.
 *
 * @return 0 on success.
 */
s32 sceSysconBatteryGetVolt(s32 *volt);

/**
 * Get the battery electric charge.
 *
 * @param elec Pointer to a s32 where the charge will be stored.
 *
 * @return 0 on success.
 */
s32 sceSysconBatteryGetElec(s32 *elec);

/**
 * Get the battery remaining capacity.
 *
 * @param rcap Pointer to a s32 where the capacity will be stored.
 *
 * @return 0 on success.
 */
s32 sceSysconBatteryGetRCap(s32 *rcap);

/**
 * Get the battery charged capacity.
 *
 * @param cap Pointer to a s32 where the capacity will be stored.
 *
 * @return 0 on success.
 */
s32 sceSysconBatteryGetCap(s32 *cap);

/**
 * Get the battery full capacity.
 *
 * @param cap Pointer to a s32 where the capacity will be stored.
 *
 * @return 0 on success.
 */
s32 sceSysconBatteryGetFullCap(s32 *cap);

/**
 * Get the battery IFC (Integrated Fire Control?).
 *
 * @param ifc Pointer to a s32 where the IFC will be stored.
 *
 * @return 0 on success.
 */
s32 sceSysconBatteryGetIFC(s32 *ifc);

/**
 * Get the battery limit time.
 *
 * @param time Pointer to a s32 where the limit time will be stored.
 *
 * @return 0 on success.
 */
s32 sceSysconBatteryGetLimitTime(s32 *time);

/**
 * Get the battery status.
 *
 * @param status Pointer to a s32 where the status will be stored.
 *
 * @return 0 on success.
 */
s32 sceSysconBatteryGetStatus(s32 *status);

/**
 * Get the battery cycle.
 *
 * @param cycle Pointer to a s32 where the cycle will be stored.
 *
 * @return 0 on success.
 */
s32 sceSysconBatteryGetCycle(s32 *cycle);

/**
 * Get the battery serial.
 *
 * @param serial Pointer to a s32 where the serial will be stored.
 *
 * @return 0 on success.
 */
s32 sceSysconBatteryGetSerial(s32 *serial);

/**
 * Get the battery analog-digital temperature.
 *
 * @param temp Pointer to a s32 where the temperature will be stored.
 *
 * @return 0 on success.
 */
s32 sceSysconBatteryGetTempAD(s32 *temp);

/**
 * Get the battery analog-digital voltage.
 *
 * @param volt Pointer to a s32 where the voltage will be stored.
 *
 * @return 0 on success.
 */
s32 sceSysconBatteryGetVoltAD(s32 *volt);

/**
 * Get the battery analog-digital electric current.
 *
 * @param elec Pointer to a s32 where the electric current will be stored.
 *
 * @return 0 on success.
 */
s32 sceSysconBatteryGetElecAD(s32 *elec);

/**
 * Get the battery total electric current.
 *
 * @param elec Pointer to a s32 where the total current will be stored.
 *
 * @return 0 on success.
 */
s32 sceSysconBatteryGetTotalElec(s32 *elec);

/**
 * Get the battery charge time.
 *
 * @param time Pointer to a s32 where the charge time will be stored.
 *
 * @return 0 on success.
 */
s32 sceSysconBatteryGetChargeTime(s32 *time);

/**
 * Set the tachyon voltage.
 *
 * @param voltage The new voltage.
 *
 * @return 0 on success.
 */
s32 sceSysconCtrlTachyonVoltage(s32 voltage);

/**
 * Get the pressed user keys.
 *
 * @param key Pointer to a 2-byte buffer where the pressed user keys will be stored.
 *
 * @return 0 on success.
 */
s32 sceSysconGetDigitalKey(s8 *key);

/** @} */

