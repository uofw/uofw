/* Copyright (C) 2011, 2012 The uOFW team
   See the file COPYING for copying permission.
*/

#include <common_imp.h>

#include <interruptman.h>
#include <lowio_gpio.h>
#include <lowio_sysreg.h>
#include <sysmem_kdebug.h>
#include <sysmem_sysclib.h>
#include <sysmem_sysevent.h>
#include <sysmem_utils_kernel.h>
#include <threadman_kernel.h>

#include <syscon.h>

#include "syscon.h"

SCE_MODULE_INFO("sceSYSCON_Driver", SCE_MODULE_KERNEL | SCE_MODULE_SINGLE_START | SCE_MODULE_SINGLE_LOAD | SCE_MODULE_NO_STOP, 1, 11);
SCE_MODULE_BOOTSTART("_sceSysconModuleStart");
SCE_MODULE_REBOOT_BEFORE("_sceSysconModuleRebootBefore");
SCE_SDK_VERSION(SDK_VERSION);

typedef struct {
    SceSysconFunc func;
    s32 gp;
    void *argp;
} SceSysconCallback;

typedef enum {
    SYSCON_CB_LOW_BATTERY,
    SYSCON_CB_POWER_SWITCH,
    SYSCON_CB_ALARM,
    SYSCON_CB_AC_SUPPLY,
    SYSCON_CB_HP_CONNECT,
    SYSCON_CB_WLAN_SWITCH,
    SYSCON_CB_HOLD_SWITCH,
    SYSCON_CB_UMD_SWITCH,
    SYSCON_CB_HR_POWER,
    SYSCON_CB_WLAN_POWER,
    SYSCON_CB_GSENSOR,
    UNUSED,
    SYSCON_CB_BT_POWER,
    SYSCON_CB_BT_SWITCH,
    SYSCON_CB_HR_WAKEUP,
    SYSCON_CB_AC_SUPPLY2,
    SYSCON_CB_HR_UNK16,
    SYSCON_CB_HR_UNK17,
    SYSCON_CB_UNK18,
    SYSCON_CB_USB_UNK19,
    SYSCON_CB_COUNT
} SceSysconCallbacks;

typedef struct {
    SceSysconPacket *curPacket;
    SceSysconPacket *packetList[8];
    SceSysconDebugHandlers *dbgHandlers;
    u32 startTime;
    u32 inGpioIntr;
    u32 rebooted;
    s32 packetOff;
    u32 packetStartTimeout;
    u32 packetStartTimeoutIntr;
    s8 hpConnect;
    s8 wlanSwitch;
    s8 btSwitch;
    s8 holdSwitch;
    s8 umdSwitch;
    s8 hrWakeupStatus;
    s8 unk70;
    s8 unk71;
    s8 unk72;
    s8 unk73;
    s8 leptonPower;
    s8 msPower;
    s8 wlanPower;
    s8 hddPower;
    s8 dvePower;
    s8 btPower;
    s8 usbPower;
    s8 tachyonVmePower;
    s8 tachyonAwPower;
    s8 tachyonAvcPower;
    s8 lcdPower;
    s8 hrPower;
    s8 wlanLed;
    s8 wlanOverride;
    s8 btOverride;
    u8 baryonStatus;
    u8 hr;
    u8 baryonStatus2;
    SceSysconCallback callbacks[SYSCON_CB_COUNT];
    s32 baryonVersion;
    s8 timeStampStr[16];
    /* Set to 1 if model is PSP 2k or newer. */
    s32 newModel;
    s32 pommelVersion;
    u64 timeStamp;
    s32 pollingMode;
    s32 retryMode;
    s32 unk376;
    SceUID semaId;
} SceSyscon;

SceSysEventHandler g_SysconEv = { 64, (s8*)"SceSyscon", 0x00FFFF00, _sceSysconSysEventHandler, 0, 0, NULL, { 0, 0, 0, 0, 0, 0, 0, 0, 0 } };

SceSysconPacket g_GetStatus2Cmd;

SceSyscon g_Syscon;

s32 _sceSysconModuleRebootBefore(void)
{
    if (g_Syscon.unk376 != 0) {
        sceSyscon_driver_765775EB(0);
    }
    g_Syscon.rebooted = 1;
    if (sceSysconCmdSync(NULL, 1)) {
        Kprintf("Syscon Cmd remained\n");
        while (sceSysconCmdSync(NULL, 1) != 0)
            sceKernelDelayThread(1000);
    }
    sceSysconEnd();
    return 0;
}

s32 sceSysconInit(void)
{
    sceSysregSpiClkSelect(0, 1);
    sceSysregSpiClkEnable(0);
    sceSysregSpiIoEnable(0);
    HW(0xBE580000) = 0xCF;
    HW(0xBE580004) = 4;
    HW(0xBE580014) = 0;
    HW(0xBE580024) = 0;
    memset(&g_Syscon, 0, 384);
    memset(&g_GetStatus2Cmd, 0, 96);
    g_GetStatus2Cmd.tx[PSP_SYSCON_TX_LEN] = 2;
    g_GetStatus2Cmd.tx[PSP_SYSCON_TX_CMD] = PSP_SYSCON_CMD_GET_STATUS2;
    g_Syscon.retryMode = 1;
    g_Syscon.packetStartTimeoutIntr = 20000;
    g_Syscon.btOverride = -1;
    g_Syscon.hpConnect = -1;
    g_Syscon.wlanSwitch = -1;
    g_Syscon.btSwitch = -1;
    g_Syscon.holdSwitch = -1;
    g_Syscon.umdSwitch = -1;
    g_Syscon.leptonPower = -1;
    g_Syscon.msPower = -1;
    g_Syscon.wlanPower = -1;
    g_Syscon.tachyonVmePower = -1;
    g_Syscon.tachyonAwPower = -1;
    g_Syscon.tachyonAvcPower = -1;
    g_Syscon.lcdPower = -1;
    g_Syscon.hrPower = -1;
    g_Syscon.wlanLed = -1;
    g_Syscon.wlanOverride = -1;
    g_Syscon.packetStartTimeout = 4000;
    g_Syscon.inGpioIntr = 0;
    sceGpioPortClear(8); 
    g_Syscon.startTime = sceKernelGetSystemTimeLow();
    sceGpioSetPortMode(3, 0);
    sceGpioSetPortMode(4, 1);
    sceGpioSetIntrMode(4, 3);
    sceKernelRegisterSubIntrHandler(4, 4, _sceSysconGpioIntr, 0);
    sceKernelEnableSubIntr(4, 4);
    sceKernelRegisterSysEventHandler(&g_SysconEv);
    while (sceSysconGetBaryonVersion(&g_Syscon.baryonVersion) < 0)
        ;
    while (sceSysconGetTimeStamp(g_Syscon.timeStampStr) < 0)
        ;
    u64 num = 0;
    s8 *str = g_Syscon.timeStampStr;
    while (*str != '\0')
        num = num * 10 + *(str++);
    g_Syscon.timeStamp = num;
    u8 baryon = g_Syscon.baryonVersion >> 16;
    if ((baryon & 0xFF) >= 0x30 && (baryon & 0xFF) <= 0x40) {
        g_Syscon.unk72 = -1;
        g_Syscon.unk70 = -1;
        g_Syscon.unk71 = -1;
    } else if ((baryon & 0xF0) == 0x40) {
        g_Syscon.unk72 = -2;
        g_Syscon.unk71 = -1;
        g_Syscon.unk70 = -1;
    } else {
        g_Syscon.unk72 = -2;
        g_Syscon.unk70 = -2;
        g_Syscon.unk71 = -2;
    }
    _sceSysconInitPowerStatus();
    g_Syscon.semaId = sceKernelCreateSema("SceSysconSync0", 1, 0, 1, 0);
    return 0;
}

s32 _sceSysconInitPowerStatus(void)
{
    s32 powerStatus;
    while (sceSysconGetPommelVersion(&g_Syscon.pommelVersion) < 0)
        ;
    while (sceSysconGetPowerStatus(&powerStatus) < 0)
        ;
    u16 baryon = g_Syscon.baryonVersion >> 16;
    if ((baryon & 0xF0) == 0 || (baryon & 0xF0) == 0x10) {
        g_Syscon.leptonPower = (powerStatus >> 9) & 1;
        g_Syscon.msPower = (powerStatus >> 12) & 1;
        g_Syscon.wlanPower = (powerStatus >> 20) & 1;
        if (_sceSysconGetPommelType() == 0x100) {
            g_Syscon.tachyonAwPower = (powerStatus >> 2) & 1;
            g_Syscon.tachyonVmePower = (powerStatus >> 1) & 1;
        } else {
            g_Syscon.tachyonAwPower = 1;
            g_Syscon.tachyonVmePower = 1;
        }
        g_Syscon.tachyonAvcPower = (powerStatus >> 3) & 1;
        g_Syscon.lcdPower = (powerStatus >> 19) & 1;
        g_Syscon.hrPower = (g_Syscon.hr >> 2) & 1;
        g_Syscon.unk73 = 0;
        g_Syscon.hddPower = 0;
        g_Syscon.dvePower = 0;
        g_Syscon.btPower = 0;
        g_Syscon.usbPower = 0;
        g_Syscon.hrWakeupStatus = 0;
    } else {
        g_Syscon.leptonPower = (powerStatus >> 3) & 1;
        g_Syscon.msPower = (powerStatus >> 13) & 1;
        g_Syscon.wlanPower = (powerStatus >> 8) & 1;
        g_Syscon.dvePower = (powerStatus >> 19) & 1;
        g_Syscon.tachyonAvcPower = (powerStatus >> 1) & 1;
        g_Syscon.lcdPower = 1;
        g_Syscon.hrPower = (powerStatus >> 7) & 1;
        g_Syscon.tachyonVmePower = 1;
        g_Syscon.tachyonAwPower = 1;
        if (((baryon & 0xFF) < 0x20 || (baryon & 0xFF) > 0x21)) {
            g_Syscon.usbPower = (powerStatus >> 21) & 1;
        } else
            g_Syscon.usbPower = 0;
        g_Syscon.newModel = 1;
        sceSysconCmdExec(&g_GetStatus2Cmd, 0);
        if (_sceSysconGetPommelType() >= 0x500) {
            g_Syscon.hddPower = (powerStatus >> 18) & 1;
        } else
            g_Syscon.hddPower = 0;
        g_Syscon.unk73 = (g_Syscon.baryonStatus2 >> 4) & 1;
        g_Syscon.btPower = (g_Syscon.baryonStatus2 >> 1) & 1;
        g_Syscon.hrWakeupStatus = (g_Syscon.baryonStatus2 >> 2) & 1;
    }
    return 0;
}

s32 sceSysconEnd(void)
{
    while (g_Syscon.curPacket != NULL)
        sceKernelDelayThread(100000);
    sceKernelUnregisterSysEventHandler(&g_SysconEv);
    sceKernelDisableSubIntr(4, 4);
    sceKernelReleaseSubIntrHandler(4, 4);
    return 0;
}

s32 sceSysconResume(void *arg0)
{   
    sceSysregSpiClkSelect(0, 1);
    sceSysregSpiClkEnable(0);
    sceSysregSpiIoEnable(0);
    HW(0xBE580000) = 0xCF;
    HW(0xBE580004) = 4;
    HW(0xBE580014) = 0;
    HW(0xBE580024) = 0;
    sceGpioSetPortMode(3, 0);
    sceGpioSetPortMode(4, 1);
    sceGpioSetIntrMode(4, 3);
    sceKernelEnableSubIntr(4, 4);
    g_Syscon.startTime = sceKernelGetSystemTimeLow();
    g_Syscon.leptonPower = 0;
    g_Syscon.msPower = (*(s32*)(arg0 + 32) >> 21) & 1;
    g_Syscon.lcdPower = 1;
    g_Syscon.wlanPower = (*(s32*)(arg0 + 24) >> 1) & 1;
    g_Syscon.tachyonVmePower = 1;
    g_Syscon.tachyonAwPower = 1;
    g_Syscon.tachyonAvcPower = 1;
    g_Syscon.hrPower = (*(s32*)(arg0 + 24) >> 2) & 1;
    g_Syscon.hpConnect = (*(s32*)(arg0 + 32) >> 15) & 1;
    g_Syscon.wlanSwitch = (~*(s32*)(arg0 + 32) >> 14) & 1;
    g_Syscon.btSwitch = (~*(s32*)(arg0 + 32) >> 14) & 1;
    g_Syscon.holdSwitch = (~*(s32*)(arg0 + 32) >> 13) & 1;
    g_Syscon.umdSwitch = (*(s32*)(arg0 + 32) >> 20) & 1;
    if (g_Syscon.unk70 >= -1)
        g_Syscon.unk70 = (*(s32*)(arg0 + 32) >> 23) & 1;
    if (g_Syscon.unk71 >= -1)
        g_Syscon.unk71 = *(u8*)(arg0 + 35) & 1;
    if (g_Syscon.unk72 >= -1)
        g_Syscon.unk72 = (~*(s32*)(arg0 + 32) >> 25) & 1;
    g_Syscon.baryonStatus2 = *(u8*)(arg0 + 64);
    if (_sceSysconGetPommelType() >= 0x500) {
        g_Syscon.hddPower = (*(s32*)(arg0 + 24) >> 18) & 1;
    }
    else
        g_Syscon.hddPower = 0;
    g_Syscon.btPower = (g_Syscon.baryonStatus2 >> 1) & 1;
    g_Syscon.hrWakeupStatus = (g_Syscon.baryonStatus2 >> 2) & 1;
    g_Syscon.unk73 = (g_Syscon.baryonStatus2 >> 4) & 1;
    return 0;
}

s32 _sceSysconSysEventHandler(s32 ev_id, s8 *ev_name __attribute__((unused)), void* param, s32 *result __attribute__((unused)))
{   
    switch (ev_id) {
    case 0x402:
        if (sceSysconCmdSync(NULL, 1) != 0)
            return -1;
        break;
    case 0x4008:
        if (g_Syscon.unk376 != 0) {
            sceSyscon_driver_765775EB(0);
        }
        sceKernelDisableSubIntr(4, 4);
        break;
    case 0x400F:
        g_Syscon.pollingMode = 1;
        break;
    case 0x10008:
        sceSysconResume(*(void**)(param + 4));
        break;
    case 0x1000F:
        g_Syscon.pollingMode = 0;
        break;
    }
    return 0;
}

/*
 * This sub interrupt is probably ran at the end of the execution of a syscon packet.
 * It first ends the last executed packet, checks the statuses which changed to run the appropriate callbacks, and runs, if needed, the next packet of the packet list.
 */
s32 _sceSysconGpioIntr(s32 subIntr __attribute__((unused)), s32 args __attribute__((unused)), void *argp __attribute__((unused)))
{
    s32 oldIntr = sceKernelCpuSuspendIntr();
    g_Syscon.inGpioIntr = 1;
    SceSysconPacket *packet = g_Syscon.curPacket;
    s32 unkEnable = 0;
    SceSysconCallback *cb;
    g_Syscon.curPacket = NULL;
    if ((packet->status & 0x200) == 0) {
        sceGpioPortClear(8);
    }
    u32 startTime = sceKernelGetSystemTimeLow();
    g_Syscon.startTime = startTime;
    sceGpioAcquireIntr(4);
    s8 endFlag = _sceSysconPacketEnd(packet);
    if (endFlag >= 0)
        g_Syscon.hr = endFlag;
    u8 prev = g_Syscon.baryonStatus;
    if (endFlag < 0) {
        packet->status |= 0x800000;
    } else {
        if (g_Syscon.newModel)
            endFlag = (endFlag & 0xFFFFFFDF) | (prev & 0x20);
        u8 flagDiff = prev ^ endFlag;
        g_Syscon.baryonStatus = endFlag;
        if ((flagDiff & 0x80) != 0) {
            cb = &g_Syscon.callbacks[SYSCON_CB_GSENSOR];
            if (cb->func != NULL) {
                s32 oldGp = pspSetGp(cb->gp);
                cb->func((endFlag >> 7) & 1, cb->argp);
                pspSetGp(oldGp);
            }
        }
        if (!g_Syscon.newModel) {
            if (((flagDiff | endFlag) & 0x20) != 0) {
                cb = &g_Syscon.callbacks[SYSCON_CB_LOW_BATTERY];
                if (cb->func != NULL) {
                    s32 oldGp = pspSetGp(cb->gp);
                    cb->func((endFlag >> 5) & 1, cb->argp);
                    pspSetGp(oldGp);
                }
            }
        }
        else if ((g_Syscon.hr & 0x20) != 0 && g_Syscon.pollingMode == 0 && packet->tx[PSP_SYSCON_TX_CMD] != PSP_SYSCON_CMD_GET_STATUS2)
            unkEnable = 1;
        if ((flagDiff & 0x10) != 0) {
            cb = &g_Syscon.callbacks[SYSCON_CB_POWER_SWITCH];
            if (cb->func != NULL) {
                s32 oldGp = pspSetGp(cb->gp);
                cb->func((endFlag >> 4) & 1, cb->argp);
                pspSetGp(oldGp);
            }
        }
        if ((flagDiff & 8) != 0) {
            cb = &g_Syscon.callbacks[SYSCON_CB_ALARM];
            if (cb->func != NULL) {
                s32 oldGp = pspSetGp(cb->gp);
                cb->func(((endFlag ^ 8) >> 3) & 1, cb->argp);
                pspSetGp(oldGp);
            }
        }
        if ((flagDiff & 1) != 0) {
            cb = &g_Syscon.callbacks[SYSCON_CB_AC_SUPPLY];
            if (cb->func != NULL) {
                s32 oldGp = pspSetGp(cb->gp);
                cb->func(endFlag & 1, cb->argp);
                pspSetGp(oldGp);
            }
            cb = &g_Syscon.callbacks[SYSCON_CB_AC_SUPPLY2];
            if (cb->func != NULL) {
                s32 oldGp = pspSetGp(cb->gp);
                cb->func(endFlag & 1, cb->argp);
                pspSetGp(oldGp);
            }
        }
        if ((flagDiff & 2) != 0) {
            cb = &g_Syscon.callbacks[SYSCON_CB_WLAN_POWER];
            if (cb->func != NULL) {
                s32 oldGp = pspSetGp(cb->gp);
                cb->func((endFlag >> 1) & 1, cb->argp);
                pspSetGp(oldGp);
            }
        }
        if ((flagDiff & 4) != 0) {
            cb = &g_Syscon.callbacks[SYSCON_CB_HR_POWER];
            if (cb->func != NULL) {
                s32 oldGp = pspSetGp(cb->gp);
                cb->func((endFlag >> 2) & 1, cb->argp);
                pspSetGp(oldGp);
            }
        }
        switch (packet->tx[PSP_SYSCON_TX_CMD]) {
        case PSP_SYSCON_CMD_GET_STATUS2:
            g_Syscon.baryonStatus2 = endFlag;
            if (((g_Syscon.baryonStatus2 ^ packet->rx[PSP_SYSCON_RX_DATA(0)]) & 2) != 0) {
                cb = &g_Syscon.callbacks[SYSCON_CB_BT_POWER];
                if (cb->func != NULL) {
                    s32 oldGp = pspSetGp(cb->gp);
                    cb->func((endFlag >> 1) & 1, cb->argp);
                    pspSetGp(oldGp);
                }
            }
            if ((flagDiff & 4) != 0) {
                cb = &g_Syscon.callbacks[SYSCON_CB_HR_WAKEUP];
                if (cb != NULL) {
                    s32 oldGp = pspSetGp(cb->gp);
                    cb->func((endFlag >> 2) & 1, cb->argp);
                    pspSetGp(oldGp);
                }
            }
            if ((flagDiff & 8) != 0) {
                s32 enable = 0;
                if ((g_Syscon.baryonStatus & 0x20) != 0 || (endFlag & 8) != 0) {
                    enable = 1;
                }
                cb = &g_Syscon.callbacks[SYSCON_CB_LOW_BATTERY];
                if (cb->func != NULL) {
                    s32 oldGp = pspSetGp(cb->gp);
                    cb->func(enable, cb->argp);
                    pspSetGp(oldGp);
                }
            }
            if ((flagDiff & 0x10) != 0) {
                cb = &g_Syscon.callbacks[SYSCON_CB_USB_UNK19];
                if (cb->func != NULL) {
                    s32 oldGp = pspSetGp(cb->gp);
                    cb->func((endFlag >> 4) & 1, cb->argp);
                    pspSetGp(oldGp);
                }
            }
            break;
        case PSP_SYSCON_CMD_GET_DIGITAL_KEY:
        case PSP_SYSCON_CMD_GET_DIGITAL_KEY_ANALOG:
        case PSP_SYSCON_CMD_GET_KERNEL_DIGITAL_KEY:
        case PSP_SYSCON_CMD_GET_KERNEL_DIGITAL_KEY_ANALOG: {
            s8 prev = g_Syscon.hpConnect;
            u8 newState = packet->rx[PSP_SYSCON_RX_DATA(1)] >> 7;
            g_Syscon.hpConnect = newState;
            if (prev == -1 || prev != newState) {
                cb = &g_Syscon.callbacks[SYSCON_CB_HP_CONNECT];
                if (cb->func != NULL) {
                    s32 oldGp = pspSetGp(cb->gp);
                    cb->func(newState, cb->argp);
                    pspSetGp(oldGp);
                }
            }
            s8 oldState = g_Syscon.wlanSwitch;
            newState = ((packet->rx[PSP_SYSCON_RX_DATA(1)] ^ 0x40) >> 6) & 1;
            if (g_Syscon.wlanOverride >= 0)
                newState = g_Syscon.wlanOverride;
            g_Syscon.wlanSwitch = newState;
            if (oldState == -1 || oldState != newState) {
                cb = &g_Syscon.callbacks[SYSCON_CB_WLAN_SWITCH];
                if (cb->func != NULL) {
                    s32 oldGp = pspSetGp(cb->gp);
                    cb->func(newState, cb->argp);
                    pspSetGp(oldGp);
                }
            }
            oldState = g_Syscon.btSwitch;
            newState = ((packet->rx[PSP_SYSCON_RX_DATA(1)] ^ 0x40) >> 6) & 1;
            if (g_Syscon.btOverride >= 0)
                newState = g_Syscon.btOverride;
            g_Syscon.btSwitch = newState;
            if (oldState == -1 || oldState != newState) {
                cb = &g_Syscon.callbacks[SYSCON_CB_BT_SWITCH];
                if (cb->func != NULL) {
                    s32 oldGp = pspSetGp(cb->gp);
                    cb->func(newState, cb->argp);
                    pspSetGp(oldGp);
                }
            }
            newState = ((packet->rx[PSP_SYSCON_RX_DATA(1)] ^ 0x20) >> 5) & 1;
            oldState = g_Syscon.holdSwitch;
            g_Syscon.holdSwitch = newState;
            if (oldState == -1 || oldState != newState) {
                cb = &g_Syscon.callbacks[SYSCON_CB_HOLD_SWITCH];
                if (cb->func != NULL) {
                    s32 oldGp = pspSetGp(cb->gp);
                    cb->func(newState, cb->argp);
                    pspSetGp(oldGp);
                }
            }
            if (packet->tx[PSP_SYSCON_TX_CMD] != PSP_SYSCON_CMD_GET_KERNEL_DIGITAL_KEY &&
              packet->tx[PSP_SYSCON_TX_CMD] != PSP_SYSCON_CMD_GET_KERNEL_DIGITAL_KEY_ANALOG)
                break;
            oldState = g_Syscon.umdSwitch;
            newState = (packet->rx[PSP_SYSCON_RX_DATA(2)] >> 4) & 1;
            g_Syscon.umdSwitch = newState;
            if (oldState == -1 || oldState != newState) {
                cb = &g_Syscon.callbacks[SYSCON_CB_UMD_SWITCH];
                if (cb->func != NULL) {
                    s32 oldGp = pspSetGp(cb->gp);
                    cb->func(newState, cb->argp);
                    pspSetGp(oldGp);
                }
            }
            oldState = g_Syscon.unk70;
            if (oldState >= -1) {
                newState = packet->rx[PSP_SYSCON_RX_DATA(2)] >> 7;
                g_Syscon.unk70 = newState;
                if (oldState == -1 || oldState != newState) {
                    cb = &g_Syscon.callbacks[SYSCON_CB_HR_UNK16];
                    if (cb->func != NULL) {
                        s32 oldGp = pspSetGp(cb->gp);
                        cb->func(newState, cb->argp);
                        pspSetGp(oldGp);
                    }
                }
            }
            oldState = g_Syscon.unk71;
            if (oldState >= -1) {
                newState = packet->rx[PSP_SYSCON_RX_DATA(3)] & 0x1;
                g_Syscon.unk71 = newState;
                if (oldState == -1 || oldState != newState) {
                    cb = &g_Syscon.callbacks[SYSCON_CB_HR_UNK17];
                    if (cb->func != NULL) {
                        s32 oldGp = pspSetGp(cb->gp);
                        cb->func(newState, cb->argp);
                        pspSetGp(oldGp);
                    }
                }
            }
            oldState = g_Syscon.unk72;
            if (oldState >= -1) {
                newState = ((packet->rx[PSP_SYSCON_RX_DATA(3)] ^ 2) >> 1) & 0x1;
                g_Syscon.unk72 = newState;
                if (oldState == -1 || oldState != newState) {
                    cb = &g_Syscon.callbacks[SYSCON_CB_UNK18];
                    if (cb->func != NULL) {
                        s32 oldGp = pspSetGp(cb->gp);
                        cb->func(newState, cb->argp);
                        pspSetGp(oldGp);
                    }
                }
            }
        }
            break;
        case PSP_SYSCON_CMD_GET_POWER_SUPPLY_STATUS:
            if (!g_Syscon.newModel)
                break;
            if ((packet->rx[PSP_SYSCON_RX_DATA(0)] & 0x10) == 0)
                g_Syscon.baryonStatus &= 0xDF;
            else
                g_Syscon.baryonStatus |= 0x20;
            if ((((prev ^ g_Syscon.baryonStatus) | g_Syscon.baryonStatus) & 0x20) == 0)
                break;
            int enable = 0;
            if ((g_Syscon.baryonStatus & 0x20) != 0 || (g_Syscon.baryonStatus2 & 8) != 0) {
                enable = 1;
            }
            cb = &g_Syscon.callbacks[SYSCON_CB_LOW_BATTERY];
            if (cb->func != NULL) {
                s32 oldGp = pspSetGp(cb->gp);
                cb->func(enable, cb->argp);
                pspSetGp(oldGp);
            }
            break;
        default:
            break;
        }
    }

    s32 unk0;
    SceUID semaId;
    if ((packet->tx[PSP_SYSCON_TX_CMD] & 0xFF) >= 32 && packet->rx[PSP_SYSCON_RX_RESPONSE] >= 128 && packet->rx[PSP_SYSCON_RX_RESPONSE] < 130 && (packet->status & 0x10) == 0) {
        packet->status |= 0x40000;
        semaId = -1;
        unk0 = 2;
    } else {
        s32 off = (packet->tx[PSP_SYSCON_TX_CMD] >> 5) & 0x7;
        if ((packet->tx[PSP_SYSCON_TX_CMD] & 0x80) != 0)
            off = 0;
        g_Syscon.packetList[off] = packet->next;
        packet->status &= 0xFFFBFFFF;
        if (packet->next == NULL)
            g_Syscon.packetList[off + 4] = NULL;
        packet->status = (packet->status & 0xFFFEFFFF) | 0x80000;
        unk0 = 0;
        packet->next = NULL;
        semaId = packet->semaId;
        if (packet->callback != NULL) {
            s32 oldGp = pspSetGp(packet->gp);
            unk0 = packet->callback(packet, packet->argp);
            pspSetGp(oldGp);
            if (unk0 != 0) {
                if (unk0 == 1) {
                    off = (packet->tx[PSP_SYSCON_TX_CMD] >> 5) & 7;
                    if ((packet->tx[PSP_SYSCON_TX_CMD] & 0x80) != 0)
                        off = 0;
                    SceSysconPacket *cur = g_Syscon.packetList[off];
                    if (cur == NULL) {
                        g_Syscon.packetList[off + 4] = packet;
                        g_Syscon.packetList[off] = packet;
                        packet->next = NULL;
                    } else {
                        g_Syscon.packetList[off] = packet;
                        packet->next = cur;
                    }
                    semaId = -1;
                    packet->status = (packet->status & 0xFFF7FFFF) | 0x10000;
                } else if (unk0 == 2) {
                    off = (packet->tx[PSP_SYSCON_TX_CMD] >> 5) & 0x7;
                    if ((packet->tx[PSP_SYSCON_TX_CMD] & 0x80) != 0)
                        off = 0;
                    SceSysconPacket *cur = g_Syscon.packetList[off + 4];
                    if (cur != NULL)
                        cur->next = packet;
                    semaId = -1;
                    g_Syscon.packetList[off + 4] = packet;
                    packet->status = (packet->status & 0xFFF7FFFF) | 0x10000;
                    packet->next = NULL;
                }
            }
        }
    }
    if (unkEnable && unk0 != 1) {
        SceSysconPacket *cur = g_Syscon.packetList[0];
        if (cur == NULL) {
            g_Syscon.packetList[4] = &g_GetStatus2Cmd;
            g_GetStatus2Cmd.next = NULL;
            g_Syscon.packetList[0] = &g_GetStatus2Cmd;
        } else {
            g_Syscon.packetList[0] = &g_GetStatus2Cmd;
            g_GetStatus2Cmd.next = cur;
        }
    }
    SceSysconPacket *cur = g_Syscon.packetList[0];
    if (cur == NULL) {
        s32 i;
        for (i = 0; i < 3; i++) {
            s32 n = g_Syscon.packetOff + i;
            s32 off = n + 1;
            if (off >= 4)
                off = n - 2;
            cur = g_Syscon.packetList[off];
            if (cur != NULL) {
                if ((cur->status & 0x40000) != 0 &&
                  startTime - cur->time < cur->delay &&
                  g_Syscon.retryMode == 0 &&
                  g_Syscon.pollingMode == 0)
                    cur = NULL;
                break;
            }
        }
        g_Syscon.packetOff++;
        if (g_Syscon.packetOff == 3) {
            g_Syscon.packetOff = 0;
        }
    }
    if (cur != NULL) {
        u32 diffTime = 4;
        if (g_Syscon.pollingMode != 0)
            diffTime = g_Syscon.packetStartTimeoutIntr;
        while (diffTime >= sceKernelGetSystemTimeLow() - startTime)
            ;
        g_Syscon.curPacket = cur;
        _sceSysconPacketStart(cur);
    }
    if (unk0 == 0 && semaId >= 0) {
        sceKernelSignalSema(semaId, 1);
    }
    g_Syscon.inGpioIntr = 0;
    sceKernelCpuResumeIntr(oldIntr);
    return -1;
}

s32 sceSysconCmdExec(SceSysconPacket *packet, u32 flags)
{   
    if (sceKernelIsIntrContext())
        return 0x80000030;

    if (g_Syscon.pollingMode == 0 && pspMfic() == 0)
        return 0x80000031;
    if (g_Syscon.pollingMode != 0 && pspMfic() != 0)
        return 0x80000031;
    s32 ret = sceSysconCmdExecAsync(packet, flags, NULL, NULL);
    if (ret < 0)
        return ret;
    return sceSysconCmdSync(packet, 0);
}

s32 sceSysconCmdExecAsync(SceSysconPacket *packet, u32 flags, s32 (*callback)(SceSysconPacket*, void*), void *argp)
{
    s32 i;
    if (g_Syscon.rebooted != 0)
        return 0x80250003;
    if ((flags & 0x100) == 0) {
        u32 hash = 0;
        for (i = 0; i < packet->tx[PSP_SYSCON_TX_LEN]; i++)
            hash += packet->tx[i];
        packet->tx[i] = ~hash;
        for (i++; i < 16; i++)
            packet->tx[i] = -1;
    }
    for (i = 0; i < 16; i++)
        packet->rx[i] = -1;
    s32 oldIntr = sceKernelCpuSuspendIntr();
    packet->status = 0x10000 | (flags & 0xFFFF);
    packet->semaId = -1;
    packet->callback = callback;
    packet->next = NULL;
    packet->gp = pspGetGp();
    u32 off = 0;
    if ((packet->tx[PSP_SYSCON_TX_CMD] & 0x80) == 0)
        off = (packet->tx[PSP_SYSCON_TX_CMD] >> 5) & 7;
    packet->argp = argp;
    if (off != 0) {
        packet->delay = g_Syscon.packetStartTimeoutIntr;
    }
    else
        packet->delay = 0;
    SceSysconPacket *cur = g_Syscon.packetList[off];
    if (cur == NULL) {
        g_Syscon.packetList[off + 4] = packet;
        g_Syscon.packetList[off] = packet;
    } else if ((flags & 1) == 0) {
        g_Syscon.packetList[off + 4]->next = packet;
        g_Syscon.packetList[off + 4] = packet;
    } else if (((cur->status >> 17) & 3) != 0) {
        SceSysconPacket *prev = cur->next;
        packet->next = cur->next;
        cur->next = packet;
        if (prev == NULL)
            g_Syscon.packetList[off + 4] = packet;
    } else
        packet->next = cur;
    if (g_Syscon.pollingMode != 0) {
        g_Syscon.curPacket = packet;
        _sceSysconPacketStart(packet);
    } else {
        if (g_Syscon.curPacket != NULL || g_Syscon.inGpioIntr) {
            sceKernelCpuResumeIntr(oldIntr);
            return 0;
        }
        SceSysconPacket *new = g_Syscon.packetList[0];
        if (new != NULL) {
            if (new == packet) {
                new = NULL;
                if ((flags & 2) == 0)
                    new = packet;
            }
        }
        if (off != 0 && (new == NULL || (flags & 1) != 0)) {
            new = g_Syscon.packetList[off];
            if (new == packet) {
                new = NULL;
                if ((flags & 2) == 0)
                    new = packet;
            }
        }
        if (new == NULL) {
            sceKernelCpuResumeIntr(oldIntr);
            return 0;
        }
        u32 timeDiff = 4;
        if (g_Syscon.retryMode != 0)
            timeDiff = g_Syscon.packetStartTimeout;
        while (timeDiff >= sceKernelGetSystemTimeLow() - g_Syscon.startTime)
            ;
        g_Syscon.curPacket = new;
        _sceSysconPacketStart(new);
    }
    sceKernelCpuResumeIntr(oldIntr);
    return 0;
}

s32 sceSysconCmdCancel(SceSysconPacket *packet)
{
    s32 oldIntr = sceKernelCpuSuspendIntr();
    u32 off = (packet->tx[PSP_SYSCON_TX_CMD] >> 5) & 7;
    if ((packet->tx[PSP_SYSCON_TX_CMD] & 0x80) != 0) {
        off = 0;
    }
    SceSysconPacket *cur = g_Syscon.packetList[off];
    s32 ret = 0x80000025;
    SceSysconPacket *prev = NULL;
    while (cur != NULL) {
        if (cur == packet) {
            ret = 0x80000021;
            if ((cur->status & 0x20000) != 0)
                goto end;
            if (prev == NULL) {
                off = (cur->tx[PSP_SYSCON_TX_CMD] >> 5) & 7;
                if ((cur->tx[PSP_SYSCON_TX_CMD] & 0x80) != 0) {
                    off = 0;
                }
                g_Syscon.packetList[off] = cur->next;
            } else
                prev->next = cur->next;

            ret = 0;
            if (cur->next != NULL)
                goto end;
            off = ((packet->tx[PSP_SYSCON_TX_CMD] >> 5) & 7) + 4;
            if ((packet->tx[PSP_SYSCON_TX_CMD] & 0x80) != 0) {
                off = 4;
            }
            g_Syscon.packetList[off] = prev;
            ret = 0;
            goto end;
        }
        prev = cur;
        cur = cur->next;
    }

end:
    sceKernelCpuResumeIntr(oldIntr);
    return ret;
}

s32 sceSysconCmdSync(SceSysconPacket *packet, u32 noWait)
{
    if (noWait != 0) {
        if (noWait != 1)
            return 0x80000107;
        if (packet == NULL) {
            if (g_Syscon.packetList[0] == NULL &&
              g_Syscon.packetList[1] == NULL &&
              g_Syscon.packetList[2] == NULL &&
              g_Syscon.packetList[3] == NULL)
                return 0;
            return 1;
        }
        if ((packet->status & 0x80000) == 0)
            return 1;
    } else if (g_Syscon.pollingMode == 0) {
        if (sceKernelIsIntrContext() != 0)
            return 0x80000030;
        s32 oldIntr = sceKernelCpuSuspendIntr();
        if (oldIntr == 0) {
            sceKernelCpuResumeIntr(0);
            return 0x80000031;
        }
        if ((packet->status & 0x80000) != 0) {
            sceKernelCpuResumeIntr(oldIntr);
        } else {
            SceUID oldSemaId = g_Syscon.semaId;
            SceUID semaId = oldSemaId;
            if (oldSemaId <= 0) {
                semaId = sceKernelCreateSema("SceSysconSync", 1, 0, 1, 0);
                if (semaId < 0) {
                    sceKernelCpuResumeIntr(oldIntr);
                    return semaId;
                }
            }
            else
                g_Syscon.semaId = 0;
            packet->semaId = semaId;
            sceKernelCpuResumeIntr(oldIntr);
            sceKernelWaitSema(semaId, 1, 0);
            if (oldSemaId == semaId) {
                g_Syscon.semaId = oldSemaId;
            }
            else
                sceKernelDeleteSema(semaId);
        }
    } else {
        while ((packet->status & 0x80000) == 0) {
            while (sceKernelGetSystemTimeLow() - g_Syscon.startTime < 5)
                ;
            while (sceGpioQueryIntr(4) == 0)
                ;
            _sceSysconGpioIntr(4, 0, NULL);
        }
    }
    if ((packet->status & 0xB00000) != 0)
        return 0x80250002;
    if ((packet->rx[PSP_SYSCON_RX_RESPONSE] >> 7) && packet->rx[PSP_SYSCON_RX_RESPONSE] != 0x82)
        return 0x80250000 | packet->rx[PSP_SYSCON_RX_RESPONSE];
    if (packet->tx[PSP_SYSCON_TX_CMD] < 32)
        return 0;
    if ((packet->rx[PSP_SYSCON_RX_RESPONSE] & 0x80) == 0)
        return 0x80250004;
    return 0;
}

s32 _sceSysconCommonRead(s32 *ptr, s32 cmd)
{   
    if (ptr == NULL)
        return 0x80000103;
    SceSysconPacket packet;
    s32 buf[4];
    packet.tx[PSP_SYSCON_TX_LEN] = 2;
    packet.tx[PSP_SYSCON_TX_CMD] = cmd;
    s32 ret = sceSysconCmdExec(&packet, 0);
    if (ret < 0)
        return ret;
    if (packet.rx[PSP_SYSCON_RX_LEN] < 4 || packet.rx[PSP_SYSCON_RX_LEN] >= 8)
        return 0x80250001;
    buf[0] = 0;
    memcpy(buf, &packet.rx[PSP_SYSCON_RX_DATA(0)], packet.rx[PSP_SYSCON_RX_LEN] - 3);
    *ptr = buf[0];
    return 0;
}

s32 _sceSysconModuleStart(s32 argc __attribute__((unused)), void *argp __attribute__((unused)))
{
    sceSysconInit();
    return 0;
}

s32 sceSyscon_driver_90EAEA2B(u32 timeout, u32 intrTimeout)
{
    g_Syscon.packetStartTimeoutIntr = intrTimeout;
    g_Syscon.packetStartTimeout = timeout;
    return 0;
}

s32 sceSyscon_driver_755CF72B(u32 *timeoutPtr, u32 *intrTimeoutPtr)
{
    if (timeoutPtr != NULL)
        *timeoutPtr = g_Syscon.packetStartTimeout;
    if (intrTimeoutPtr != NULL)
        *intrTimeoutPtr = g_Syscon.packetStartTimeoutIntr;
    return 0;
}

s32 sceSysconSuspend(void)
{   
    if (g_Syscon.unk376 != 0) {
        sceSyscon_driver_765775EB(0);
    }
    sceKernelDisableSubIntr(4, 4);
    return 0;
}

s32 sceSysconSetDebugHandlers(SceSysconDebugHandlers *handlers)
{
    g_Syscon.dbgHandlers = handlers;
    return 0;
}

s32 sceSysconSetPollingMode(u32 pollingMode)
{
    g_Syscon.pollingMode = pollingMode;
    return 0;
}

s32 sceSysconSetAffirmativeRertyMode(u32 mode)
{
    g_Syscon.retryMode = mode;
    return 0;
}

s32 _sceSysconGetBaryonVersion(void)
{
    s32 retAddr;
    asm("move %0, $ra" : "=r" (retAddr));
    retAddr &= 0x3FFFFFFF;
    return g_Syscon.baryonVersion;
}

u64 _sceSysconGetBaryonTimeStamp(void)
{
    return g_Syscon.timeStamp;
}

s32 _sceSysconGetPommelVersion(void)
{
    return g_Syscon.pommelVersion;
}

s32 _sceSysconGetUsbPowerType(void)
{   
    u16 type = g_Syscon.baryonVersion >> 16;
    if ((type & 0xF0) != 0 && (type & 0xF0) != 0x10 && ((type & 0xFF) < 0x20 || (type & 0xFF) >= 0x22))
        return 1;
    return 0;
}

s32 sceSysconSetGSensorCallback(SceSysconFunc func, void *argp)
{
    return _sceSysconSetCallback(func, argp, SYSCON_CB_GSENSOR);
}

s32 sceSysconSetLowBatteryCallback(SceSysconFunc func, void *argp)
{
    return _sceSysconSetCallback(func, argp, SYSCON_CB_LOW_BATTERY);
}

s32 sceSysconSetPowerSwitchCallback(SceSysconFunc func, void *argp)
{
    return _sceSysconSetCallback(func, argp, SYSCON_CB_POWER_SWITCH);
}

s32 sceSysconSetAlarmCallback(SceSysconFunc func, void *argp)
{
    return _sceSysconSetCallback(func, argp, SYSCON_CB_ALARM);
}

s32 sceSysconSetAcSupplyCallback(SceSysconFunc func, void *argp)
{
    return _sceSysconSetCallback(func, argp, SYSCON_CB_AC_SUPPLY);
}

s32 sceSysconSetAcSupply2Callback(SceSysconFunc func, void *argp)
{
    return _sceSysconSetCallback(func, argp, SYSCON_CB_AC_SUPPLY2);
}

s32 sceSysconSetHPConnectCallback(SceSysconFunc func, void *argp)
{
    return _sceSysconSetCallback(func, argp, SYSCON_CB_HP_CONNECT);
}

s32 sceSysconSetHRPowerCallback(SceSysconFunc func, void *argp)
{
    return _sceSysconSetCallback(func, argp, SYSCON_CB_HR_POWER);
}

s32 sceSysconSetHRWakeupCallback(SceSysconFunc func, void *argp)
{
    return _sceSysconSetCallback(func, argp, SYSCON_CB_HR_WAKEUP);
}

s32 sceSysconSetWlanSwitchCallback(SceSysconFunc func, void *argp)
{
    return _sceSysconSetCallback(func, argp, SYSCON_CB_WLAN_SWITCH);
}

s32 sceSysconSetWlanPowerCallback(SceSysconFunc func, void *argp)
{
    return _sceSysconSetCallback(func, argp, SYSCON_CB_WLAN_POWER);
}

s32 sceSysconSetBtSwitchCallback(SceSysconFunc func, void *argp)
{
    return _sceSysconSetCallback(func, argp, SYSCON_CB_BT_SWITCH);
}

s32 sceSysconSetBtPowerCallback(SceSysconFunc func, void *argp)
{
    return _sceSysconSetCallback(func, argp, SYSCON_CB_BT_POWER);
}

s32 sceSysconSetHoldSwitchCallback(SceSysconFunc func, void *argp)
{
    return _sceSysconSetCallback(func, argp, SYSCON_CB_HOLD_SWITCH);
}

s32 sceSysconSetUmdSwitchCallback(SceSysconFunc func, void *argp)
{
    return _sceSysconSetCallback(func, argp, SYSCON_CB_UMD_SWITCH);
}

s32 sceSyscon_driver_374373A8(SceSysconFunc func, void *argp)
{
    return _sceSysconSetCallback(func, argp, SYSCON_CB_HR_UNK16);
}

s32 sceSyscon_driver_B761D385(SceSysconFunc func, void *argp)
{
    return _sceSysconSetCallback(func, argp, SYSCON_CB_HR_UNK17);
}

s32 sceSyscon_driver_26307D84(SceSysconFunc func, void *argp)
{
    return _sceSysconSetCallback(func, argp, SYSCON_CB_UNK18);
}

s32 sceSyscon_driver_6C388E02(SceSysconFunc func, void *argp)
{
    return _sceSysconSetCallback(func, argp, SYSCON_CB_USB_UNK19);
}

s32 _sceSysconCommonWrite(u32 val, u32 cmd, u32 size)
{   
    SceSysconPacket packet;
    packet.tx[PSP_SYSCON_TX_CMD] = cmd;
    packet.tx[PSP_SYSCON_TX_LEN] = size;
    packet.tx[PSP_SYSCON_TX_DATA(1)] = (val >> 8);
    packet.tx[PSP_SYSCON_TX_DATA(2)] = (val >> 16);
    packet.tx[PSP_SYSCON_TX_DATA(3)] = (val >> 24);
    packet.tx[PSP_SYSCON_TX_DATA(0)] = val;
    return pspMin(sceSysconCmdExec(&packet, 0), 0);
}

u8 sceSysconGetBaryonStatus(void)
{
    return g_Syscon.baryonStatus;
}

u8 sceSysconGetBaryonStatus2(void)
{
    return g_Syscon.baryonStatus2;
}

u8 sceSysconIsFalling(void)
{
    return (g_Syscon.baryonStatus & 0x80) ? 1 : 0;
}

u8 sceSysconIsLowBattery(void)
{
    return ((g_Syscon.baryonStatus >> 5) & 1) | ((g_Syscon.baryonStatus2 >> 3) & 1);
}

u8 sceSysconGetPowerSwitch(void)
{
    return (g_Syscon.baryonStatus >> 4) & 1;
}

u8 sceSysconIsAlarmed(void)
{
    return ((g_Syscon.baryonStatus ^ 8) >> 3) & 1;
}

u8 sceSysconIsAcSupplied(void)
{
    return g_Syscon.baryonStatus & 1;
}

s8 sceSysconGetHPConnect(void)
{
    return g_Syscon.hpConnect;
}

s8 sceSysconGetWlanSwitch(void)
{
    if (g_Syscon.wlanOverride < 0)
        return g_Syscon.wlanSwitch;
    return g_Syscon.wlanOverride;
}

s8 sceSyscon_driver_0B51E34D(s8 wlanSwitch)
{
    s8 prev = g_Syscon.wlanOverride;
    g_Syscon.wlanOverride = wlanSwitch;
    return prev;
}

s8 sceSysconGetBtSwitch(void)
{
    if (g_Syscon.btOverride < 0)
        return g_Syscon.btSwitch;
    return g_Syscon.btOverride;
}

s8 sceSyscon_driver_BADF1260(s8 btSwitch)
{
    s8 prev = g_Syscon.btOverride;
    g_Syscon.btOverride = btSwitch;
    return prev;
}

s8 sceSysconGetHoldSwitch(void)
{
    return g_Syscon.holdSwitch;
}

s8 sceSysconGetUmdSwitch(void)
{
    return g_Syscon.umdSwitch;
}

s8 sceSyscon_driver_248335CD(void)
{
    return g_Syscon.unk70;
}

s8 sceSyscon_driver_040982CD(void)
{
    return g_Syscon.unk71;
}

s8 sceSyscon_driver_97765E27(void)
{
    return g_Syscon.unk72;
}

u8 sceSysconGetHRPowerStatus(void)
{
    return (g_Syscon.baryonStatus >> 2) & 1;
}

u8 sceSysconGetHRWakeupStatus(void)
{
    return (g_Syscon.baryonStatus2 >> 2) & 1;
}

u8 sceSysconGetWlanPowerStatus(void)
{
    return (g_Syscon.baryonStatus >> 1) & 1;
}

u8 sceSysconGetBtPowerStatus(void)
{
    return (g_Syscon.baryonStatus2 >> 1) & 1;
}

u8 sceSyscon_driver_DF20C984(void)
{
    return (g_Syscon.baryonStatus2 >> 4) & 1;
}

s8 sceSysconGetLeptonPowerCtrl(void)
{
    return g_Syscon.leptonPower;
}

s8 sceSysconGetMsPowerCtrl(void)
{
    return g_Syscon.msPower;
}

s8 sceSysconGetWlanPowerCtrl(void)
{
    return g_Syscon.wlanPower;
}

s8 sceSysconGetHddPowerCtrl(void)
{
    return g_Syscon.hddPower;
}

s8 sceSysconGetDvePowerCtrl(void)
{
    return g_Syscon.dvePower;
}

s8 sceSysconGetBtPowerCtrl(void)
{
    return g_Syscon.btPower;
}

s8 sceSysconGetUsbPowerCtrl(void)
{
    return g_Syscon.usbPower;
}

s8 sceSysconGetTachyonVmePowerCtrl(void)
{
    return g_Syscon.tachyonVmePower;
}

s8 sceSysconGetTachyonAwPowerCtrl(void)
{
    return g_Syscon.tachyonAwPower;
}

s8 sceSysconGetTachyonAvcPowerCtrl(void)
{
    return g_Syscon.tachyonAvcPower;
}

s8 sceSysconGetLcdPowerCtrl(void)
{
    return g_Syscon.lcdPower;
}

s8 sceSysconGetHRPowerCtrl(void)
{
    return g_Syscon.hrPower;
}

s8 sceSysconGetWlanLedCtrl(void)
{
    return g_Syscon.wlanLed;
}

s32 _sceSysconSetCallback(SceSysconFunc func, void *argp, s32 cbId)
{   
    s32 oldIntr = sceKernelCpuSuspendIntr();
    g_Syscon.callbacks[cbId].func = func;
    g_Syscon.callbacks[cbId].argp = argp;
    g_Syscon.callbacks[cbId].gp = pspGetGp();
    sceKernelCpuResumeIntr(oldIntr);
    return 0;
}

s32 _sceSysconPacketStart(SceSysconPacket *packet)
{
    packet->time = sceKernelGetSystemTimeLow();
    if (g_Syscon.dbgHandlers != NULL) {
        g_Syscon.dbgHandlers->start(packet);
    }
    sceGpioPortRead();
    sceGpioPortClear(8);
    while ((HW(0xBE58000C) & 4) != 0)
        HW(0xBE580008);
    HW(0xBE58000C);
    HW(0xBE580020) = 3;
    if ((packet->status & 0x40000) == 0) {
        s32 i;
        for (i = 0; i < packet->tx[PSP_SYSCON_TX_LEN] + 1; i += 2) {
            HW(0xBE58000C);
            HW(0xBE580008) = (packet->tx[i] << 8) | packet->tx[i + 1];
        }
    } else {
        HW(0xBE580008) = (packet->tx[PSP_SYSCON_TX_CMD] << 8) | 2;
        HW(0xBE580008) = ((packet->tx[PSP_SYSCON_TX_CMD] + 2) << 8) ^ 0xFFFF;
    }
    HW(0xBE580004) = 6;
    sceGpioPortSet(8);
    packet->status |= 0x20000;
    return 0;
}

s32 _sceSysconPacketEnd(SceSysconPacket *packet)
{
    s32 ret = 0;
    s32 i;
    if ((HW(0xBE58000C) & 4) == 0) {
        packet->rx[PSP_SYSCON_RX_STATUS] = -1;
        ret = -1;
        packet->status |= 0x100000;
        packet->rx[PSP_SYSCON_RX_LEN] = 0;
        for (i = 0; i < 16; i++)
            asm("\n"); /* Here be dragons. */
    }
    if ((HW(0xBE58000C) & 1) == 0) {
        ret = -1;
        packet->status |= 0x200000;
    }
    if ((HW(0xBE580018) & 1) != 0) {
        HW(0xBE580020) = 1;
        packet->status |= 0x400000;
    }
    for (i = 0; i < 16; i += 2) {
        if ((HW(0xBE58000C) & 4) == 0)
            break;
        u16 v = HW(0xBE580008) & 0xFFFF;
        if (i == 0)
            ret = v >> 8;
        packet->rx[i + 0] = v >> 8;
        packet->rx[i + 1] = v;
    }
    HW(0xBE580004) = 4;
    sceGpioPortClear(8);
    if (g_Syscon.dbgHandlers != NULL) {
        g_Syscon.dbgHandlers->end(packet);
    }
    if (ret >= 0) {
        u32 hash = 0;
        if (packet->rx[PSP_SYSCON_RX_LEN] < 3) /* Received data is too short */
            ret = -2;
        else if (packet->rx[PSP_SYSCON_RX_LEN] < 16) {
            for (i = 0; i < packet->rx[PSP_SYSCON_RX_LEN]; i++)
                hash = (hash + packet->rx[i]) & 0xFF;
            if ((packet->rx[packet->rx[PSP_SYSCON_RX_LEN]] ^ (~hash & 0xFF)) != 0) /* Wrong hash */
                ret = -2;
        } else /* Received data is too long */
            ret = -2;
    }
    packet->status &= 0xFFFDFFFF;
    return ret;
}

s32 sceSysconGetTimeStamp(s8 *timeStamp)
{   
    SceSysconPacket packet;
    if (timeStamp == NULL)
        return 0x80000103;
    packet.tx[PSP_SYSCON_TX_CMD] = PSP_SYSCON_CMD_GET_TIMESTAMP;
    packet.tx[PSP_SYSCON_TX_LEN] = 2;
    s32 ret = sceSysconCmdExec(&packet, 0);
    if (ret < 0)
        return ret;
    memcpy(timeStamp, &packet.rx[PSP_SYSCON_RX_DATA(0)], 12); /* inline */
    timeStamp[12] = '\0';
    return 0;
}

s32 sceSysconWriteScratchPad(u32 dst, void *src, u32 size)
{   
    SceSysconPacket packet;
    if ((size < 1 || size >= 3) && size != 4 && size != 8)
        return 0x80000104;
    if ((dst % size) != 0)
        return 0x80000102;
    if (dst + size >= 33)
        return 0x80000102;
    packet.tx[PSP_SYSCON_TX_CMD] = PSP_SYSCON_CMD_WRITE_SCRATCHPAD;
    packet.tx[PSP_SYSCON_TX_LEN] = size + 3;
    packet.tx[PSP_SYSCON_TX_DATA(0)] = (dst << 2) | ((size / 2) != 4 ? size / 2 : 3);
    memcpy(&packet.tx[PSP_SYSCON_TX_DATA(1)], src, size);
    return pspMin(sceSysconCmdExec(&packet, 0), dst % size);
}

s32 sceSysconReadScratchPad(u32 src, void *dst, u32 size)
{
    SceSysconPacket packet;
    if ((size < 1 || size >= 3) && size != 4 && size != 8)
        return 0x80000104;
    if ((src % size) != 0)
        return 0x80000102;
    if (src + size > 32)
        return 0x80000102;
    packet.tx[PSP_SYSCON_TX_CMD] = PSP_SYSCON_CMD_READ_SCRATCHPAD;
    packet.tx[PSP_SYSCON_TX_LEN] = 3;
    packet.tx[PSP_SYSCON_TX_DATA(0)] = (src << 2) | ((size >> 1) != 4) ? (size >> 1) : 3;
    s32 ret = sceSysconCmdExec(&packet, 0);
    if (ret < 0)
        return ret;
    memcpy(dst, &packet.rx[PSP_SYSCON_RX_DATA(0)], size);
    return 0;
}

s32 sceSysconSendSetParam(u32 id, void *param)
{
    SceSysconPacket packet;
    if (((_sceSysconGetBaryonVersion() >> 16) & 0xFF) < 0x12 &&
      ((_sceSysconGetBaryonVersion() & 0xFF) != 1 ||
      _sceSysconGetBaryonTimeStamp() <= 200509260441)) {
        if (id != 0)
            return 0x80250011;
        packet.tx[PSP_SYSCON_TX_CMD] = PSP_SYSCON_CMD_SEND_SETPARAM;
        packet.tx[PSP_SYSCON_TX_LEN] = 10;
        memcpy(&packet.tx[PSP_SYSCON_TX_DATA(0)], param, 8);
    }
    else {
        packet.tx[PSP_SYSCON_TX_CMD] = PSP_SYSCON_CMD_SEND_SETPARAM;
        packet.tx[PSP_SYSCON_TX_LEN] = 11;
        memcpy(&packet.tx[PSP_SYSCON_TX_DATA(0)], param, 8);
        packet.tx[PSP_SYSCON_TX_DATA(8)] = id;
    }
    return pspMin(sceSysconCmdExec(&packet, 0), 0);
}

s32 sceSysconReceiveSetParam(u32 id, void *param)
{   
    SceSysconPacket packet;
    if (((_sceSysconGetBaryonVersion() >> 16) & 0xFF) < 0x12 &&
      ((_sceSysconGetBaryonVersion() & 0xFF) != 1 ||
      _sceSysconGetBaryonTimeStamp() <= 200509260441)) {
        if (id != 0)
            return 0x80250011;
        packet.tx[PSP_SYSCON_TX_CMD] = PSP_SYSCON_CMD_RECEIVE_SETPARAM;
        packet.tx[PSP_SYSCON_TX_LEN] = 2;
    } else {
        packet.tx[PSP_SYSCON_TX_CMD] = PSP_SYSCON_CMD_RECEIVE_SETPARAM;
        packet.tx[PSP_SYSCON_TX_LEN] = 3;
        packet.tx[PSP_SYSCON_TX_DATA(0)] = id;
    }
    s32 ret = sceSysconCmdExec(&packet, 0);
    if (ret < 0)
        return ret;
    memcpy(param, &packet.rx[PSP_SYSCON_RX_DATA(0)], 8);
    return 0;
}

s32 sceSysconCtrlTachyonWDT(s32 wdt)
{   
    if (wdt >= 0x80)
        return 0x800001FE;
    return _sceSysconCommonWrite(wdt == 0 ? 0 : wdt | 0x80, PSP_SYSCON_CMD_CTRL_TACHYON_WDT, 3);
}

s32 sceSysconResetDevice(u32 reset, u32 mode)
{
    if (reset == 1) {
        if (mode != 1) {
            reset = 0x41;
            if (mode != 2)
                return 0x80000107;
        }
    } else if (mode != 0)
        reset |= 0x80;
    s32 ret = _sceSysconCommonWrite(reset, PSP_SYSCON_CMD_RESET_DEVICE, 3);
    if (ret < 0)
        return ret;
    if (reset == 4 && mode == 0 && sceSysconGetWlanLedCtrl() == 1)
        sceSysconCtrlLED(1, 1);
    return ret;
}

s32 sceSyscon_driver_12518439(u32 arg0)
{   
    if (((_sceSysconGetBaryonVersion() >> 16) & 0xF0) >= 0x30) {
        *(s8*)(0x13FC0 + 0) = 0x35;
        *(s8*)(0x13FC0 + 5) = -1;
        *(s8*)(0x13FC0 + 1) = 4;
        *(s8*)(0x13FC0 + 4) = ~(arg0 + (arg0 >> 8) + 57);
        *(s8*)(0x13FC0 + 2) = arg0;
        *(s8*)(0x13FC0 + 3) = (arg0 >> 8);
    } else {
        *(s8*)(0x13FC0 + 0) = 0x35;
        *(s8*)(0x13FC0 + 3) = -1;
        *(s8*)(0x13FC0 + 1) = 2;
        *(s8*)(0x13FC0 + 2) = -56;
    }
    sub_2D08(0x13FC0, 0, 0);
    return 0;
}

s32 sceSysconPowerSuspend(u32 arg0, u32 arg1)
{   
    u32 set = 0;
    void *sm1 = sceKernelSm1ReferOperations();
    if (sm1 != NULL) {
        s32 (*func)() = *(void**)(sm1 + 16);
        if (func() == 21325) {
            func = *(void**)(sm1 + 16);
            set = (func() < 6);
        }
    }
    if (((_sceSysconGetBaryonVersion() >> 16) & 0xF0) >= 0x30) {
        *(s8*)(0x13FC0 + 0) = 0x36;
        *(s8*)(0x13FC0 + 5) = -1;
        *(s8*)(0x13FC0 + 1) = 4;
        *(s8*)(0x13FC0 + 4) = ~(arg0 + (arg0 >> 8) + 58);
        *(s8*)(0x13FC0 + 2) = arg0;
        *(s8*)(0x13FC0 + 3) = arg0 >> 8;
    } else {
        *(s8*)(0x13FC0 + 0) = 0x36;
        *(s8*)(0x13FC0 + 3) = ~(arg0 + 57);
        *(s8*)(0x13FC0 + 1) = 3;
        *(s8*)(0x13FC0 + 2) = arg0;
    }
    sub_2D08(0x13FC0, set, arg1);
    return 0;
}

s32 sub_2D08(u32 addr, u32 arg1, u32 arg2)
{   
    sceKernelCpuSuspendIntr();
    memcpy((void*)0x10000, sub_0000, (void*)_sceSysconModuleRebootBefore - (void*)sub_0000);
    sceKernelDcacheWritebackAll();
    sceKernelIcacheInvalidateAll();
    void (*func)(s32, s32, s32) = (void*)0x10000;
    func(addr, arg1, arg2);
    return 0;
}

s32 sceSysconNop(void)
{
    SceSysconPacket packet;
    packet.tx[PSP_SYSCON_TX_CMD] = PSP_SYSCON_CMD_NOP;
    packet.tx[PSP_SYSCON_TX_LEN] = 2;
    return sceSysconCmdExec(&packet, 0);
}

s32 sceSysconGetBaryonVersion(s32 *baryonVersion)
{
    return _sceSysconCommonRead(baryonVersion, PSP_SYSCON_CMD_GET_BARYON);
}

s32 sceSysconGetGValue(void)
{
    return 0x80000004;
}

s32 sceSysconGetPowerSupplyStatus(s32 *status)
{
    return _sceSysconCommonRead(status, PSP_SYSCON_CMD_GET_POWER_SUPPLY_STATUS);
}

s32 sceSysconGetFallingDetectTime(void)
{
    return 0x80000004;
}

s32 sceSysconGetWakeUpFactor(void *factor)
{
    return _sceSysconCommonRead(factor, PSP_SYSCON_CMD_GET_WAKE_UP_FACTOR);
}

s32 sceSysconGetWakeUpReq(void *req)
{
    return _sceSysconCommonRead(req, PSP_SYSCON_CMD_GET_WAKE_UP_REQ);
}

s32 sceSysconGetVideoCable(s32 *cable)
{
    u32 ver = _sceSysconGetBaryonVersion() >> 16;
    if ((ver & 0xF0) == 0 || (ver & 0xF0) == 0x10) {
        *cable = 0;
        return 0x80000004;
    }
    return _sceSysconCommonRead(cable, PSP_SYSCON_CMD_GET_VIDEO_CABLE);
}

s32 sceSysconReadClock(s32 *clock)
{
    return _sceSysconCommonRead(clock, PSP_SYSCON_CMD_READ_CLOCK);
}

s32 sceSysconWriteClock(s32 clock)
{
    return _sceSysconCommonWrite(clock, PSP_SYSCON_CMD_WRITE_CLOCK, 6);
}

s32 sceSysconReadAlarm(s32 *alarm)
{
    return _sceSysconCommonRead(alarm, PSP_SYSCON_CMD_READ_ALARM);
}

s32 sceSysconWriteAlarm(s32 alarm)
{
    return _sceSysconCommonWrite(alarm, PSP_SYSCON_CMD_WRITE_ALARM, 6);
}

s32 sceSysconSetUSBStatus(u8 status)
{
    SceSysconPacket packet;
    packet.tx[PSP_SYSCON_TX_CMD] = PSP_SYSCON_CMD_SET_USB_STATUS;
    packet.tx[PSP_SYSCON_TX_LEN] = 3;
    packet.tx[PSP_SYSCON_TX_DATA(0)] = status;
    return sceSysconCmdExec(&packet, (status & 3) == 0);
}

s32 sceSysconGetTachyonWDTStatus(s32 *status)
{
    return _sceSysconCommonRead(status, PSP_SYSCON_CMD_GET_TACHYON_WDT_STATUS);
}

s32 sceSysconCtrlAnalogXYPolling(s8 polling)
{
    return _sceSysconCommonWrite(polling, PSP_SYSCON_CMD_CTRL_ANALOG_XY_POLLING, 3);
}

s32 sceSysconCtrlHRPower(s8 power)
{
    s32 ret = _sceSysconCommonWrite(power, PSP_SYSCON_CMD_CTRL_HR_POWER, 3);
    if (ret >= 0)
        g_Syscon.hrPower = power;
    return ret;
}

s32 sceSysconCtrlPower(u32 arg0, u32 arg1)
{
    s32 ret = _sceSysconCommonWrite(((arg1 & 1) << 23) | (arg0 & 0x003FFFFF), PSP_SYSCON_CMD_CTRL_POWER, 5);
    if (ret >= 0)
        _sceSysconGetPommelType();
    return ret;
}

s32 sceSysconCtrlLED(u32 led, u32 set)
{
    u8 ledMask, setMask;
    switch (led) {
    case 1:
        ledMask = 0x80;
        g_Syscon.wlanLed = set;
        break;
    case 0:
        ledMask = 0x40;
        break;
    case 2:
        ledMask = 0x20;
        break;
    case 3:
        ledMask = 0x10;
        if (_sceSysconGetPommelType() < 0x300)
            return 0x80000004;
        break;
    default:
        return 0x80000102;
    }
    setMask = 0;
    if (set != 0) {
        u32 ver = (_sceSysconGetBaryonVersion() >> 16) & 0xF0;
        setMask = 0x10;
        if (ver != 0 && ver != 0x10)
            setMask = 1;
    }
    return _sceSysconCommonWrite(ledMask | setMask, PSP_SYSCON_CMD_CTRL_LED, 3);
}

s32 sceSysconCtrlDvePower(s8 power)
{
    u32 type = _sceSysconGetPommelType();
    if (type < 0x300)
        return 0x80000004;
    if (type >= 0x301) {
        s32 ret = _sceSysconCommonWrite(power, PSP_SYSCON_CMD_CTRL_DVE_POWER, 3);
        if (ret >= 0)
            g_Syscon.dvePower = power;
        return ret;
    }
    return 0;
}

s32 sceSyscon_driver_765775EB(s32 arg0)
{   
    if (((g_Syscon.baryonVersion >> 16) & 0xFF) >= 0x30 &&
      ((g_Syscon.baryonVersion >> 16) & 0xFF) < 0x32) {
        return sub_406C(arg0);
    }              
    if (arg0 == 0) {
        if (g_Syscon.unk376 == 0)
            return 0;
    } else if (g_Syscon.unk376 != 0 && sceSysconGetBtPowerStatus() != 0)
        return 0x80250005;
    s32 ret = _sceSysconCommonWrite((arg0 & 1) | 0x80, PSP_SYSCON_CMD_CTRL_BT_POWER, 3);
    if (ret >= 0)
        g_Syscon.unk376 = arg0;
    return ret;
}

s32 sceSysconCtrlCharge(u8 allow)
{   
    u8 version = (_sceSysconGetBaryonVersion() >> 16) & 0xF0;
    if (version != 0 && version != 0x10) {
        return _sceSysconCommonWrite(allow, PSP_SYSCON_CMD_CTRL_CHARGE, 3);
    }
    if (!allow) {          
        return sub_3360();
    }                  
    return sub_32D8();
}

s32 sceSysconCtrlTachyonAvcPower(s8 power)
{
    s32 ret = sceSysconCtrlPower((_sceSysconGetPommelType() >= 0x300) ? 2 : 8, power);
    if (ret >= 0)
        g_Syscon.tachyonAvcPower = power;
    return ret;
}

s32 sub_32D8(void)
{   
    SceSysconPacket packet;
    u32 flag;
    packet.tx[PSP_SYSCON_TX_CMD] = PSP_SYSCON_CMD_READ_POLESTAR_REG;
    packet.tx[PSP_SYSCON_TX_DATA(0)] = 2;
    packet.tx[PSP_SYSCON_TX_LEN] = 3;
    s32 ret = sceSysconCmdExec(&packet, 0);
    if (ret >= 0) {
        flag = 0;
        ret = 0;
        memcpy(&flag, &packet.rx[PSP_SYSCON_RX_DATA(0)], 2);
    }
    if (ret >= 0) {
        flag &= 0xFDFF;         
        ret = _sceSysconCommonWrite((flag << 8) | 2, PSP_SYSCON_CMD_WRITE_POLESTAR_REG, 5);
    }
    return ret;
}

s32 sub_3360(void)
{   
    u32 flag;
    SceSysconPacket packet;
    packet.tx[PSP_SYSCON_TX_CMD] = PSP_SYSCON_CMD_READ_POLESTAR_REG;
    packet.tx[PSP_SYSCON_TX_DATA(0)] = 2;
    packet.tx[PSP_SYSCON_TX_LEN] = 3;
    s32 ret = sceSysconCmdExec(&packet, 0);
    if (ret >= 0) {
        flag = 0;
        ret = 0;
        memcpy(&flag, &packet.rx[PSP_SYSCON_RX_DATA(0)], 2);
    }
    if (ret >= 0) {
        flag |= 0x200;                
        ret = _sceSysconCommonWrite(((flag & 0xFFFF) << 8) | 2, PSP_SYSCON_CMD_WRITE_POLESTAR_REG, 5);
    }
    return ret;
}

s32 sceSysconGetPommelVersion(s32 *pommel)
{
    return _sceSysconCommonRead(pommel, PSP_SYSCON_CMD_GET_POMMEL_VERSION);
}

s32 sceSysconGetPolestarVersion(s32 *polestar)
{
    return _sceSysconCommonRead(polestar, PSP_SYSCON_CMD_GET_POLESTAR_VERSION);
}

s32 sceSysconCtrlVoltage(s32 arg0, s32 arg1)
{   
    if (_sceSysconGetPommelType() != 0x100 && arg0 >= 4 && arg0 < 6)
        return 0x80000004;
    return _sceSysconCommonWrite(((arg1 & 0xFFFF) << 8) | (arg0 & 0xFF), PSP_SYSCON_CMD_CTRL_VOLTAGE, 5);
}

s32 sceSysconGetGSensorVersion(void)
{
    return 0x80000004;
}

s32 sceSysconCtrlGSensor(void)
{
    return 0x80000004;
}

s32 sceSysconGetPowerStatus(s32 *status)
{
    return _sceSysconCommonRead(status, PSP_SYSCON_CMD_GET_POWER_STATUS);
}

s32 sceSysconWritePommelReg(u8 reg, s16 value)
{
    return _sceSysconCommonWrite(((value & 0xFFFF) << 8) | (reg & 0xFF), PSP_SYSCON_CMD_WRITE_POMMEL_REG, 5);
}

s32 sceSysconReadPommelReg(u8 reg, s16 *value)
{
    SceSysconPacket packet;
    packet.tx[PSP_SYSCON_TX_CMD] = PSP_SYSCON_CMD_READ_POMMEL_REG;
    packet.tx[PSP_SYSCON_TX_LEN] = 3;
    packet.tx[PSP_SYSCON_TX_DATA(0)] = reg;
    s32 ret = sceSysconCmdExec(&packet, 0);
    if (ret < 0)
        return ret;
    *value = 0;
    memcpy(value, &packet.rx[PSP_SYSCON_RX_DATA(0)], 2);
    return 0;
}

s32 sceSysconGetPowerError(s32 *error)
{
    if (error != NULL)
        *error = 0;
    return 0;
}

s32 sceSysconCtrlLeptonPower(s8 power)
{
    s32 ret = _sceSysconCommonWrite(power, PSP_SYSCON_CMD_CTRL_LEPTON_POWER, 3);
    if (ret >= 0)
        g_Syscon.leptonPower = power;
    return ret;
}

s32 sceSysconCtrlMsPower(s8 power)
{
    s32 ret = _sceSysconCommonWrite(power, PSP_SYSCON_CMD_CTRL_MS_POWER, 3);
    if (ret >= 0)
        g_Syscon.msPower = power;
    return ret;
}

s32 sceSysconCtrlWlanPower(s8 power)
{
    s32 ret = _sceSysconCommonWrite(power, PSP_SYSCON_CMD_CTRL_WLAN_POWER, 3);
    if (ret >= 0) {
        g_Syscon.wlanPower = power;
        if (power != 0 && sceSysconGetWlanLedCtrl())
            sceSysconCtrlLED(1, 1);
    }
    return ret;
}

s32 sceSysconCtrlHddPower(s8 power)
{
    if (_sceSysconGetPommelType() < 0x500)
        return 0x80000004;
    s32 ret = _sceSysconCommonWrite(power, PSP_SYSCON_CMD_CTRL_HDD_POWER, 3);
    if (ret >= 0)
        g_Syscon.hddPower = power;
    return ret;
}

s32 sceSysconCtrlBtPower(s8 power)
{
    if (g_Syscon.unk376 != 0)
        return 0;
    s32 ret = _sceSysconCommonWrite(power & 1, PSP_SYSCON_CMD_CTRL_BT_POWER, 3);
    if (ret >= 0)
        g_Syscon.btPower = power;
    return ret;
}

s32 sceSysconCtrlUsbPower(s8 power)
{   
    u8 version = _sceSysconGetBaryonVersion() >> 16;
    if ((version & 0xF0) == 0 || (version & 0xF0) == 0x10 || ((version & 0xFF) >= 0x20 && (version & 0xFF) < 0x22))
        return 0x80000004;
    s32 ret = _sceSysconCommonWrite(power, PSP_SYSCON_CMD_CTRL_USB_POWER, 3);
    if (ret >= 0)
        g_Syscon.usbPower = power;
    return ret;
}

s32 sceSysconPermitChargeBattery(void)
{
    return sceSysconCtrlCharge(1);
}

s32 sceSysconForbidChargeBattery(void)
{
    return sceSysconCtrlCharge(0);
}

s32 sceSysconCtrlTachyonVmePower(s8 power)
{
    if (_sceSysconGetPommelType() != 0x100)
        return 0x80000004;
    s32 ret = sceSysconCtrlPower(2, power);
    if (ret >= 0)
        g_Syscon.tachyonVmePower = power;
    return ret;
}

s32 sceSysconCtrlTachyonAwPower(s8 power)
{
    if (_sceSysconGetPommelType() != 0x100)
        return 0x80000004;
    s32 ret = sceSysconCtrlPower(4, power);
    if (ret >= 0)
        g_Syscon.tachyonAwPower = power;
    return ret;
}

s32 sceSysconCtrlLcdPower(s8 power)
{
    if (_sceSysconGetPommelType() >= 0x300)
        return 0x80000004;
    s32 ret = sceSysconCtrlPower(0x80000, power);
    if (ret >= 0)
        g_Syscon.lcdPower = power;
    return ret;
}

s32 sceSysconGetGSensorCarib(void)
{
    return 0x80000004;
}

s32 sceSysconSetGSensorCarib(void)
{
    return 0x80000004;
}

s32 sceSysconWritePolestarReg(u8 reg, s16 val)
{
    return _sceSysconCommonWrite(((val & 0xFFFF) << 8) | (reg & 0xFF), PSP_SYSCON_CMD_WRITE_POLESTAR_REG, 5);
}

s32 sceSysconReadPolestarReg(u8 reg, s16 *val)
{
    SceSysconPacket packet;
    packet.tx[PSP_SYSCON_TX_CMD] = PSP_SYSCON_CMD_READ_POLESTAR_REG;
    packet.tx[PSP_SYSCON_TX_LEN] = 3;
    packet.tx[PSP_SYSCON_TX_DATA(0)] = reg;
    s32 ret = sceSysconCmdExec(&packet, 0);
    if (ret < 0)
        return ret;
    *val = 0;
    memcpy(val, &packet.rx[PSP_SYSCON_RX_DATA(0)], 2);
    return 0;
}

s32 sceSysconWriteGSensorReg(void)
{
    return 0x80000004;
}

s32 sceSysconReadGSensorReg(void)
{
    return 0x80000004;
}

s32 sceSysconBatteryGetStatusCap(s32 *arg0, s32 *arg1)
{   
    SceSysconPacket packet;
    packet.tx[PSP_SYSCON_TX_CMD] = PSP_SYSCON_CMD_BATTERY_GET_STATUS_CAP;
    packet.tx[PSP_SYSCON_TX_LEN] = 2;             
    s32 ret = sceSysconCmdExec(&packet, 0);
    if (ret < 0)
        return ret;
    if (arg0 != NULL)
        *arg0 = packet.rx[PSP_SYSCON_RX_DATA(0)];
    if (arg1 != NULL)
        *arg1 = (packet.tx[PSP_SYSCON_TX_DATA(3)] << 8) | packet.tx[PSP_SYSCON_TX_DATA(2)];
    return 0;
}

s32 sceSysconBatteryGetInfo(s32 *info)
{
    SceSysconPacket packet;
    packet.tx[PSP_SYSCON_TX_CMD] = PSP_SYSCON_CMD_BATTERY_GET_INFO;
    packet.tx[PSP_SYSCON_TX_LEN] = 2;
    s32 ret = sceSysconCmdExec(&packet, 0);
    if (ret < 0)
        return ret;
    if (info != NULL) {
        info[3] = packet.rx[PSP_SYSCON_RX_DATA(4)];
        info[0] = (packet.rx[PSP_SYSCON_RX_DATA(1)] << 8) | packet.rx[PSP_SYSCON_RX_DATA(0)];
        info[1] = packet.rx[PSP_SYSCON_RX_DATA(2)];
        info[2] = packet.rx[PSP_SYSCON_RX_DATA(3)];
    }
    return 0;
}

s32 sceSysconGetBattVolt(s32 *volt)
{   
    u8 version =_sceSysconGetBaryonVersion() >> 16;
    if ((version & 0xF0) != 0x29 &&
      (version & 0xFF) != 0x2A &&
      (version & 0xF0) != 0x30 &&
      (version & 0xF0) != 0x40)
        return 0x80000004;
    s32 val;
    s32 ret = _sceSysconCommonRead(&val, PSP_SYSCON_CMD_GET_BATT_VOLT);
    if (ret < 0)
        return ret;
    *volt = val * 50;
    return 0;
}

s32 sceSysconGetBattVoltAD(s32 *volt1, s32 *volt2)
{
    u8 version = _sceSysconGetBaryonVersion() >> 16;
    if ((version & 0xF0) != 0x29 &&
      (version & 0xFF) != 0x2A &&
      (version & 0xF0) != 0x30 &&
      (version & 0xF0) != 0x40)
        return 0x80000004;
    s32 val;
    s32 ret = _sceSysconCommonRead(&val, PSP_SYSCON_CMD_GET_BATT_VOLT_AD);
    if (ret < 0)
        return ret;
    if (volt1 != NULL)
        *volt1 = ((val >> 16) & 0xFF) * 50;
    if (volt2 != NULL)
        *volt2 = val & 0xFFFF;
    return 0;
}

s32 sceSysconBatteryNop(void)
{
    SceSysconPacket packet;
    packet.tx[PSP_SYSCON_TX_CMD] = PSP_SYSCON_CMD_BATTERY_NOP;
    packet.tx[PSP_SYSCON_TX_LEN] = 2;
    return pspMin(sceSysconCmdExec(&packet, 0), 0);
}

s32 sceSysconBatteryGetTemp(s32 *temp)
{
    return _sceSysconBatteryCommon(PSP_SYSCON_CMD_BATTERY_GET_TEMP, temp);
}

s32 sceSysconBatteryGetVolt(s32 *volt)
{
    return _sceSysconBatteryCommon(PSP_SYSCON_CMD_BATTERY_GET_VOLT, volt);
}

s32 sceSysconBatteryGetElec(s32 *elec)
{
    return _sceSysconBatteryCommon(PSP_SYSCON_CMD_BATTERY_GET_ELEC, elec);
}

s32 sceSysconBatteryGetRCap(s32 *rcap)
{
    return _sceSysconBatteryCommon(PSP_SYSCON_CMD_BATTERY_GET_RCAP, rcap);
}

s32 sceSysconBatteryGetCap(s32 *cap)
{
    return _sceSysconBatteryCommon(PSP_SYSCON_CMD_BATTERY_GET_CAP, cap);
}

s32 sceSysconBatteryGetFullCap(s32 *cap)
{
    return _sceSysconBatteryCommon(PSP_SYSCON_CMD_BATTERY_GET_FULL_CAP, cap);
}

s32 sceSysconBatteryGetIFC(s32 *ifc)
{
    return _sceSysconBatteryCommon(PSP_SYSCON_CMD_BATTERY_GET_IFC, ifc);
}

s32 sceSysconBatteryGetLimitTime(s32 *time)
{
    return _sceSysconBatteryCommon(PSP_SYSCON_CMD_BATTERY_GET_LIMIT_TIME, time);
}

s32 sceSysconBatteryGetStatus(s32 *status)
{
    s32 stat;
    s32 ret = _sceSysconBatteryCommon(PSP_SYSCON_CMD_BATTERY_GET_STATUS, &stat);
    if (ret < 0)
        return ret;
    if (status != NULL)
        *status = stat;
    return 0;
}

s32 sceSysconBatteryGetCycle(s32 *cycle)
{
    return _sceSysconBatteryCommon(PSP_SYSCON_CMD_BATTERY_GET_CYCLE, cycle);
}

s32 sceSysconBatteryGetSerial(s32 *serial)
{
    return _sceSysconBatteryCommon(PSP_SYSCON_CMD_BATTERY_GET_SERIAL, serial);
}

s32 sceSysconBatteryGetTempAD(s32 *temp)
{
    return _sceSysconBatteryCommon(PSP_SYSCON_CMD_BATTERY_GET_TEMP_AD, temp);
}

s32 sceSysconBatteryGetVoltAD(s32 *volt)
{
    return _sceSysconBatteryCommon(PSP_SYSCON_CMD_BATTERY_GET_VOLT_AD, volt);
}

s32 sceSysconBatteryGetElecAD(s32 *elec)
{
    return _sceSysconBatteryCommon(PSP_SYSCON_CMD_BATTERY_GET_ELEC_AD, elec);
}

s32 sceSysconBatteryGetTotalElec(s32 *elec)
{
    return _sceSysconBatteryCommon(PSP_SYSCON_CMD_BATTERY_GET_TOTAL_ELEC, elec);
}

s32 sceSysconBatteryGetChargeTime(s32 *time)
{
    return _sceSysconBatteryCommon(PSP_SYSCON_CMD_BATTERY_GET_CHARGE_TIME, time);
}

s32 _sceSysconBatteryCommon(u32 cmd, s32 *ptr)
{
    u32 version = _sceSysconGetBaryonVersion() >> 16;
    if ((version & 0xF0) == 0x29 ||
     (version & 0xFF) == 0x2A ||
     (version & 0xF0) == 0x30 ||
     (version & 0xF0) == 0x40)
        return 0x80000004;
    SceSysconPacket packet;
    packet.tx[PSP_SYSCON_TX_CMD] = cmd;
    packet.tx[PSP_SYSCON_TX_LEN] = 2;
    s32 val;
    s32 ret = sceSysconCmdExec(&packet, 0);
    if (ret < 0)
        return ret;
    if (packet.rx[PSP_SYSCON_RX_LEN] == 4) {
        val = packet.rx[PSP_SYSCON_RX_DATA(0)];
    } else if (packet.rx[PSP_SYSCON_RX_LEN] == 5) {
        val = (s16)((packet.rx[PSP_SYSCON_RX_DATA(1)] << 8) | packet.rx[PSP_SYSCON_RX_DATA(0)]);
    } else if (packet.rx[PSP_SYSCON_RX_LEN] == 6) {
        val = (((packet.rx[PSP_SYSCON_RX_DATA(2)] << 24) | (packet.rx[PSP_SYSCON_RX_DATA(1)] << 16)) >> 8) | packet.rx[PSP_SYSCON_RX_DATA(0)];
    } else if (packet.rx[PSP_SYSCON_RX_LEN] == 7)
        val = (packet.rx[PSP_SYSCON_RX_DATA(3)] << 24) | (packet.rx[PSP_SYSCON_RX_DATA(2)] << 16) | (packet.rx[PSP_SYSCON_RX_DATA(1)] << 8) | packet.rx[PSP_SYSCON_RX_DATA(0)];
    else
        return 0x80250001;
    *ptr = val;
    return 0;
}

s32 sceSysconCtrlTachyonVoltage(s32 voltage)
{   
    s32 status;
    s32 ret = sceSysconGetPowerStatus(&status);
    if (ret < 0)
        return ret;
    s32 mask = 0;
    u8 ver = (_sceSysconGetBaryonVersion() >> 16) & 0xF0;
    if (ver != 0 && ver != 0x10) {
        if ((status & 2) == 0)
            mask = 2;
    } else {
        mask = 8;
        if ((status & 8) != 0)
            mask = 0;
        if (_sceSysconGetPommelType() == 0x100) {
            if ((status & 4) == 0)
                mask |= 4;
            if ((status & 2) == 0)
                mask |= 2;
        }  
    }              
    if (mask != 0) {             
        ret = sceSysconCtrlPower(mask, 1);
        if (ret < 0)
            return ret;
    }   
    ret = sceSysconCtrlVoltage(1, voltage);
    if (mask != 0) {       
        sceSysconCtrlPower(mask, 0);
    }
    return ret;
}

s32 sub_406C(s32 arg0)
{
    if (arg0 == 0) {
        if (g_Syscon.unk376 == 0)
            return 0;
    } else {
        if (g_Syscon.unk376 != 0 &&
          sceSysconGetBtPowerStatus() != 0)
            return 0x80250005;
        s32 ret = sub_4150(0, 150, 150);
        if (ret < 0)
            return ret;
    }
    SceSysconPacket packet;
    packet.tx[PSP_SYSCON_TX_CMD] = PSP_SYSCON_CMD_CTRL_BT_POWER_UNK2;
    packet.tx[PSP_SYSCON_TX_LEN] = 4;
    packet.tx[PSP_SYSCON_TX_DATA(0)] = 1;
    packet.tx[PSP_SYSCON_TX_DATA(1)] = arg0;
    s32 ret = sceSysconCmdExec(&packet, 0);
    if (ret >= 0) {
        g_Syscon.unk376 = arg0;
        if (arg0 != 0)
            return ret;
    }
    sub_4150(0, 0, 0);
    return ret;
}

s32 sub_4150(s32 arg0, s32 arg1, s32 arg2)
{
    SceSysconPacket packet;
    packet.tx[PSP_SYSCON_TX_CMD] = PSP_SYSCON_CMD_CTRL_BT_POWER_UNK1;
    packet.tx[PSP_SYSCON_TX_LEN] = 7;
    packet.tx[PSP_SYSCON_TX_DATA(0)] = 21;
    packet.tx[PSP_SYSCON_TX_DATA(1)] = 1;
    packet.tx[PSP_SYSCON_TX_DATA(2)] = arg0;
    packet.tx[PSP_SYSCON_TX_DATA(3)] = arg1;
    packet.tx[PSP_SYSCON_TX_DATA(4)] = arg2;
    s32 ret = sceSysconCmdExec(&packet, 0);
    if (ret < 0)
        return ret;
    if (packet.rx[PSP_SYSCON_RX_LEN] != 3)
        return 0x80000104;
    return 0;
}

s32 _sceSysconGetPommelType(void)
{
    u32 baryon = _sceSysconGetBaryonVersion();
    u8 flag = (u32)baryon >> 16;
    if ((flag & 0xF0) == 0) {
        if (((baryon >> 8) & 0xFF) != 1)
            return 0x100;
        return 0;
    }
    if ((flag & 0xF0) == 0x10)
        return 0x200;
    if (((flag & 0xFF) >= 0x20 && (flag & 0xFF) < 0x22) ||
      (((((flag & 0xF0) == 0x20 && (flag & 0xFF) >= 0x22) || (flag & 0xF0) == 0x30 || (flag & 0xF0) == 0x40) &&
      ((flag & 0xF0) == 0x20 && (flag & 0xFF) >= 0x22 && (flag & 0xFF) < 0x26)) ||
      ((flag & 0xFF) == 0x29))) {
        if (_sceSysconGetPommelVersion() != 0x120)
            return 0x301;
        return 0x300;
    }
    if (((flag & 0xF0) == 0x20 && (flag & 0xFF) >= 0x22) ||
      (flag & 0xF0) == 0x30 ||
      (flag & 0xF0) == 0x40) {
        if (((flag & 0xF0) == 0x20 && (flag & 0xFF) >= 0x26 && (flag & 0xFF) < 0x29) ||
          ((flag & 0xFF) >= 0x30 && (flag & 0xFF) < 0x32))
            return 0x400;
    }
    if (((flag & 0xF0) == 0x20 && (flag & 0xFF) >= 0x22) ||
      (flag & 0xF0) == 0x30 ||
      (flag & 0xF0) == 0x40) {
        if (((flag & 0xF0) == 0x20 && (flag & 0xFF) >= 0x2C && (flag & 0xFF) < 0x2E) ||
          ((flag & 0xFF) >= 0x32 && (flag & 0xFF) < 0x40))
            return 0x500;
    }
    if (((flag & 0xF0) == 0x20 && (flag & 0xFF) >= 0x22) ||
      (flag & 0xF0) == 0x30 ||
      (flag & 0xF0) == 0x40) {
        if ((flag & 0xF0) == 0x20 && (flag & 0xFF) >= 0x2E && (flag & 0xFF) < 0x30)
            return 0x600;
    }
    if (((flag & 0xF0) == 0x20 && (flag & 0xFF) >= 0x22) ||
      (flag & 0xF0) == 0x30 ||
      (flag & 0xF0) == 0x40) {
        if ((flag & 0xF0) == 0x40)
            return 0x600;
    }
    return -1;
}

s32 sceSysconGetDigitalKey(s8 *key)
{
    SceSysconPacket packet;
    packet.tx[PSP_SYSCON_TX_CMD] = PSP_SYSCON_CMD_GET_DIGITAL_KEY;
    packet.tx[PSP_SYSCON_TX_LEN] = 2;
    s32 ret = sceSysconCmdExec(&packet, 0);
    if (ret < 0)
        return ret;
    key[0] = packet.rx[PSP_SYSCON_RX_DATA(0)];
    key[1] = packet.rx[PSP_SYSCON_RX_DATA(1)];
    return 0;
}

