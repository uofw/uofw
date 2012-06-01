/* Copyright (C) 2011, 2012 The uOFW team
   See the file COPYING for copying permission.
*/

#include <common.h>

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

/* structure at g_4EB0; size: 384 */
typedef struct {
    SceSysconPacket *curPacket; // 0
    SceSysconPacket *packetList[8]; // 4
    SceSysconDebugHandlers *dbgHandlers; // 36
    u32 startTime; // 40
    u32 inGpioIntr; // 44
    u32 rebooted; // 48
    s32 packetOff; // 52
    u32 packetStartDelay; // 56
    u32 packetStartDelayIntr; // 60
    s8 hpConnect; // 64
    s8 wlanSwitch; // 65
    s8 btSwitch; // 66
    s8 holdSwitch; // 67
    s8 umdSwitch; // 68
    s8 hrWakeupStatus; // 69
    s8 unk70; // 70
    s8 unk71; // 71
    s8 unk72; // 72
    s8 unk73; // 73
    s8 leptonPower; // 74
    s8 msPower; // 75
    s8 wlanPower; // 76
    s8 hddPower; // 77
    s8 dvePower; // 78
    s8 btPower; // 79
    s8 usbPower; // 80
    s8 tachyonVmePower; // 81
    s8 tachyonAwPower; // 82
    s8 tachyonAvcPower; // 83
    s8 lcdPower; // 84
    s8 hrPower; // 85
    s8 wlanLed; // 86
    s8 wlanOverride; // 87
    s8 btOverride; // 88
    u8 baryonStatus; // 89
    u8 hr; // 90
    u8 baryonStatus2; // 91
    SceSysconCallback callbacks[20]; // 92
    s32 baryonVersion; // 332
    s8 timeStampStr[16]; // 336
    s32 unk352; // 352
    s32 pommelVersion; // 356
    u64 timeStamp; // 360
    s32 pollingMode; // 368
    s32 retryMode; // 372
    s32 unk376; // 376
    SceUID semaId; // 380
} SceSyscon;

// 4E10
SceSysEventHandler g_SysconEv = { 64, (s8*)"SceSyscon", 0x00FFFF00, _sceSysconSysEventHandler, 0, 0, NULL, { 0, 0, 0, 0, 0, 0, 0, 0, 0 } };

SceSysconPacket g_4E50;

SceSyscon g_4EB0;

// 0220
s32 _sceSysconModuleRebootBefore(void)
{
    dbg_printf("Run %s\n", __FUNCTION__);
    if (g_4EB0.unk376 != 0) {
        // 02A0
        sceSyscon_driver_765775EB(0);
    }
    g_4EB0.rebooted = 1;
    if (sceSysconCmdSync(NULL, 1)) {
        // 0274
        Kprintf("Syscon Cmd remained\n");
        // 0280
        while (sceSysconCmdSync(NULL, 1) != 0)
            sceKernelDelayThread(1000);
    }
    // 0258
    sceSysconEnd();
    return 0;
}

// 02B0
s32 sceSysconInit(void)
{
    dbg_printf("Run %s\n", __FUNCTION__);
    sceSysregSpiClkSelect(0, 1);
    sceSysregSpiClkEnable(0);
    sceSysregSpiIoEnable(0);
    HW(0xBE580000) = 0xCF;
    HW(0xBE580004) = 4;
    HW(0xBE580014) = 0;
    HW(0xBE580024) = 0;
    memset(&g_4EB0, 0, 384);
    memset(&g_4E50, 0, 96);
    g_4E50.tx[1] = 2;
    g_4E50.tx[0] = 0x10;
    g_4EB0.retryMode = 1;
    g_4EB0.packetStartDelayIntr = 20000;
    g_4EB0.btOverride = -1;
    g_4EB0.hpConnect = -1;
    g_4EB0.wlanSwitch = -1;
    g_4EB0.btSwitch = -1;
    g_4EB0.holdSwitch = -1;
    g_4EB0.umdSwitch = -1;
    g_4EB0.leptonPower = -1;
    g_4EB0.msPower = -1;
    g_4EB0.wlanPower = -1;
    g_4EB0.tachyonVmePower = -1;
    g_4EB0.tachyonAwPower = -1;
    g_4EB0.tachyonAvcPower = -1;
    g_4EB0.lcdPower = -1;
    g_4EB0.hrPower = -1;
    g_4EB0.wlanLed = -1;
    g_4EB0.wlanOverride = -1;
    g_4EB0.packetStartDelay = 4000;
    g_4EB0.inGpioIntr = 0;
    sceGpioPortClear(8); 
    g_4EB0.startTime = sceKernelGetSystemTimeLow();
    sceGpioSetPortMode(3, 0);
    sceGpioSetPortMode(4, 1);
    sceGpioSetIntrMode(4, 3);
    sceKernelRegisterSubIntrHandler(4, 4, _sceSysconGpioIntr, 0);
    sceKernelEnableSubIntr(4, 4);
    sceKernelRegisterSysEventHandler(&g_SysconEv);
    // 040C
    while (sceSysconGetBaryonVersion(&g_4EB0.baryonVersion) < 0)
        ;
    // 041C
    while (sceSysconGetTimeStamp(g_4EB0.timeStampStr) < 0)
        ;
    u64 num = 0;
    s8 *str = g_4EB0.timeStampStr;
    // 0448
    while (*str != '\0')
        num = num * 10 + *(str++);
    // 04A0
    g_4EB0.timeStamp = num;
    u8 baryon = g_4EB0.baryonVersion >> 16;
    if ((baryon & 0xFF) >= 0x30 && (baryon & 0xFF) <= 0x40) {
        // 04D4
        g_4EB0.unk72 = -1;
        g_4EB0.unk70 = -1;
        g_4EB0.unk71 = -1;
    } else if ((baryon & 0xF0) == 0x40) {
        // 0528
        g_4EB0.unk72 = -2;
        g_4EB0.unk71 = -1;
        g_4EB0.unk70 = -1;
    } else {
        // 04D4
        g_4EB0.unk72 = -2;
        g_4EB0.unk70 = -2;
        g_4EB0.unk71 = -2;
    }
    // 04E0
    _sceSysconInitPowerStatus(); // 0540
    g_4EB0.semaId = sceKernelCreateSema("SceSysconSync0", 1, 0, 1, 0);
    return 0;
}

// 0540
s32 _sceSysconInitPowerStatus(void)
{
    dbg_printf("Run %s\n", __FUNCTION__);
    s32 powerStatus;
    // 0554
    while (sceSysconGetPommelVersion(&g_4EB0.pommelVersion) < 0)
        ;
    dbg_puts("1\n");
    // 0564
    while (sceSysconGetPowerStatus(&powerStatus) < 0)
        ;
    dbg_puts("2\n");
    dbg_printf("baryon 0x%08x, pommel 0x%08x\n", g_4EB0.baryonVersion, g_4EB0.pommelVersion);
    u16 baryon = g_4EB0.baryonVersion >> 16;
    if ((baryon & 0xF0) == 0 || (baryon & 0xF0) == 0x10) {
        // 0670
        g_4EB0.leptonPower = (powerStatus >> 9) & 1;
        g_4EB0.msPower = (powerStatus >> 12) & 1;
        g_4EB0.wlanPower = (powerStatus >> 20) & 1;
        if (_sceSysconGetPommelType() == 0x100) {
            // 06E4
            g_4EB0.tachyonAwPower = (powerStatus >> 2) & 1;
            g_4EB0.tachyonVmePower = (powerStatus >> 1) & 1;
        } else {
            g_4EB0.tachyonAwPower = 1;
            g_4EB0.tachyonVmePower = 1;
        }
        // 06A8
        g_4EB0.tachyonAvcPower = (powerStatus >> 3) & 1;
        g_4EB0.lcdPower = (powerStatus >> 19) & 1;
        g_4EB0.hrPower = (g_4EB0.hr >> 2) & 1;
        g_4EB0.unk73 = 0;
        g_4EB0.hddPower = 0;
        g_4EB0.dvePower = 0;
        g_4EB0.btPower = 0;
        g_4EB0.usbPower = 0;
        g_4EB0.hrWakeupStatus = 0;
    } else {
        g_4EB0.leptonPower = (powerStatus >> 3) & 1;
        g_4EB0.msPower = (powerStatus >> 13) & 1;
        g_4EB0.wlanPower = (powerStatus >> 8) & 1;
        g_4EB0.dvePower = (powerStatus >> 19) & 1;
        g_4EB0.tachyonAvcPower = (powerStatus >> 1) & 1;
        g_4EB0.lcdPower = 1;
        g_4EB0.hrPower = (powerStatus >> 7) & 1;
        g_4EB0.tachyonVmePower = 1;
        g_4EB0.tachyonAwPower = 1;
        if (((baryon & 0xFF) < 0x20 || (baryon & 0xFF) > 0x21)) {
            // 0664
            g_4EB0.usbPower = (powerStatus >> 21) & 1;
        } else
            g_4EB0.usbPower = 0;
        // 05EC
        g_4EB0.unk352 = 1;
        sceSysconCmdExec(&g_4E50, 0);
        if (_sceSysconGetPommelType() >= 0x500) {
            // 0658
            g_4EB0.hddPower = (powerStatus >> 18) & 1;
        } else
            g_4EB0.hddPower = 0;
        // 0620
        g_4EB0.unk73 = (g_4EB0.baryonStatus2 >> 4) & 1;
        g_4EB0.btPower = (g_4EB0.baryonStatus2 >> 1) & 1;
        g_4EB0.hrWakeupStatus = (g_4EB0.baryonStatus2 >> 2) & 1;
    }
    // 0640
    return 0;
}

// 06FC
s32 sceSysconEnd(void)
{
    dbg_printf("Run %s\n", __FUNCTION__);
    // 074C, 0750
    while (g_4EB0.curPacket != NULL)
        sceKernelDelayThread(100000);
    // 0718
    sceKernelUnregisterSysEventHandler(&g_SysconEv);
    sceKernelDisableSubIntr(4, 4);
    sceKernelReleaseSubIntrHandler(4, 4);
    return 0;
}

// 076C
s32 sceSysconResume(void *arg0)
{   
    dbg_printf("Run %s\n", __FUNCTION__);
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
    g_4EB0.startTime = sceKernelGetSystemTimeLow();
    g_4EB0.leptonPower = 0;
    g_4EB0.msPower = (*(s32*)(arg0 + 32) >> 21) & 1;
    g_4EB0.lcdPower = 1;
    g_4EB0.wlanPower = (*(s32*)(arg0 + 24) >> 1) & 1;
    g_4EB0.tachyonVmePower = 1;
    g_4EB0.tachyonAwPower = 1;
    g_4EB0.tachyonAvcPower = 1;
    g_4EB0.hrPower = (*(s32*)(arg0 + 24) >> 2) & 1;
    g_4EB0.hpConnect = (*(s32*)(arg0 + 32) >> 15) & 1;
    g_4EB0.wlanSwitch = (~*(s32*)(arg0 + 32) >> 14) & 1;
    g_4EB0.btSwitch = (~*(s32*)(arg0 + 32) >> 14) & 1;
    g_4EB0.holdSwitch = (~*(s32*)(arg0 + 32) >> 13) & 1;
    g_4EB0.umdSwitch = (*(s32*)(arg0 + 32) >> 20) & 1;
    if (g_4EB0.unk70 >= -1)
        g_4EB0.unk70 = (*(s32*)(arg0 + 32) >> 23) & 1;
    // 08A0
    if (g_4EB0.unk71 >= -1)
        g_4EB0.unk71 = *(u8*)(arg0 + 35) & 1;
    // 08C0
    if (g_4EB0.unk72 >= -1)
        g_4EB0.unk72 = (~*(s32*)(arg0 + 32) >> 25) & 1;
    // 08E0
    g_4EB0.baryonStatus2 = *(u8*)(arg0 + 64);
    if (_sceSysconGetPommelType() >= 0x500) {
        // 0934
        g_4EB0.hddPower = (*(s32*)(arg0 + 24) >> 18) & 1;
    }
    else
        g_4EB0.hddPower = 0;
    // 08F8
    g_4EB0.btPower = (g_4EB0.baryonStatus2 >> 1) & 1;
    g_4EB0.hrWakeupStatus = (g_4EB0.baryonStatus2 >> 2) & 1;
    g_4EB0.unk73 = (g_4EB0.baryonStatus2 >> 4) & 1;
    return 0;
}

// 0940
s32 _sceSysconSysEventHandler(s32 ev_id, s8 *ev_name __attribute__((unused)), void* param, s32 *result __attribute__((unused)))
{   
    dbg_printf("Run %s\n", __FUNCTION__);
    switch (ev_id) {
    case 0x402:
        // 09B4
        if (sceSysconCmdSync(NULL, 1) != 0)
            return -1;
        break;

    case 0x4008:
        // 0984
        if (g_4EB0.unk376 != 0) {
            // 09A4
            sceSyscon_driver_765775EB(0);
        }
        // 0994
        sceKernelDisableSubIntr(4, 4);
        break;

    case 0x400F:
        // 0A00
        g_4EB0.pollingMode = 1;
        break;

    case 0x10008:
        // 09F0
        sceSysconResume(*(void**)(param + 4));
        break;

    case 0x1000F:
        g_4EB0.pollingMode = 0;
        break;
    }
    return 0;
}

// 0A10
s32 _sceSysconGpioIntr(s32 subIntr __attribute__((unused)), s32 args __attribute__((unused)), void *argp __attribute__((unused))) // TODO: check
{
    s32 oldIntr = sceKernelCpuSuspendIntr();
    g_4EB0.inGpioIntr = 1;
    SceSysconPacket *packet = g_4EB0.curPacket;
    s32 unkEnable = 0;
    SceSysconCallback *cb;
    g_4EB0.curPacket = NULL;
    if ((packet->status & 0x200) == 0) {
        // 153C
        sceGpioPortClear(8);
    }
    // 0A6C
    u32 startTime = sceKernelGetSystemTimeLow();
    g_4EB0.startTime = startTime;
    sceGpioAcquireIntr(4);
    s8 endFlag = _sceSysconPacketEnd(packet);
    if (endFlag >= 0)
        g_4EB0.hr = endFlag;
    // 0A98
    u8 prev = g_4EB0.baryonStatus;
    if (endFlag < 0) {
        // 1524
        packet->status |= 0x800000;
    } else {
        if (g_4EB0.unk352 != 0)
            endFlag = (endFlag & 0xFFFFFFDF) | (prev & 0x20);
        u8 flagDiff = prev ^ endFlag;
        // 0AC0
        g_4EB0.baryonStatus = endFlag;
        if ((flagDiff & 0x80) != 0) {
            cb = &g_4EB0.callbacks[10];
            if (cb->func != NULL) {
                s32 oldGp = pspSetGp(cb->gp);
                cb->func((endFlag >> 7) & 1, cb->argp);
                pspSetGp(oldGp);
            }
        }
        // 0AF8
        if (g_4EB0.unk352 == 0) {
            // 14E4
            if (((flagDiff | endFlag) & 0x20) != 0) {
                cb = &g_4EB0.callbacks[0];
                if (cb->func != NULL) {
                    s32 oldGp = pspSetGp(cb->gp);
                    cb->func((endFlag >> 5) & 1, cb->argp);
                    pspSetGp(oldGp);
                }
            }
        }
        else if ((g_4EB0.hr & 0x20) != 0 && g_4EB0.pollingMode == 0 && packet->tx[0] != 16)
            unkEnable = 1;
        // 0B38
        if ((flagDiff & 0x10) != 0) {
            cb = &g_4EB0.callbacks[1];
            if (cb->func != NULL) {
                s32 oldGp = pspSetGp(cb->gp);
                cb->func((endFlag >> 4) & 1, cb->argp);
                pspSetGp(oldGp);
            }
        }
        // (0B70)
        // 0B74
        if ((flagDiff & 8) != 0) {
            cb = &g_4EB0.callbacks[2];
            if (cb->func != NULL) {
                s32 oldGp = pspSetGp(cb->gp);
                cb->func(((endFlag ^ 8) >> 3) & 1, cb->argp);
                pspSetGp(oldGp);
            }
        }
        // 0BB4
        if ((flagDiff & 1) != 0) {
            cb = &g_4EB0.callbacks[3];
            if (cb->func != NULL) {
                s32 oldGp = pspSetGp(cb->gp);
                cb->func(endFlag & 1, cb->argp);
                pspSetGp(oldGp);
            }
            // 0BF0
            cb = &g_4EB0.callbacks[15];
            if (cb->func != NULL) {
                s32 oldGp = pspSetGp(cb->gp);
                cb->func(endFlag & 1, cb->argp);
                pspSetGp(oldGp);
            }
        }
        // 0C24
        if ((flagDiff & 2) != 0) {
            cb = &g_4EB0.callbacks[9];
            if (cb->func != NULL) {
                s32 oldGp = pspSetGp(cb->gp);
                cb->func((endFlag >> 1) & 1, cb->argp);
                pspSetGp(oldGp);
            }
        }
        // 0C60
        if ((flagDiff & 4) != 0) {
            cb = &g_4EB0.callbacks[8];
            if (cb->func != NULL) {
                s32 oldGp = pspSetGp(cb->gp);
                cb->func((endFlag >> 2) & 1, cb->argp);
                pspSetGp(oldGp);
            }
        }
        // (0C98)
        // 0C9C
        switch (packet->tx[0]) {
        case 16:
            // 0CC8
            g_4EB0.baryonStatus2 = endFlag;
            if (((g_4EB0.baryonStatus2 ^ packet->rx[3]) & 2) != 0) {
                cb = &g_4EB0.callbacks[12];
                if (cb->func != NULL) {
                    s32 oldGp = pspSetGp(cb->gp);
                    cb->func((endFlag >> 1) & 1, cb->argp);
                    pspSetGp(oldGp);
                }
            }
            // 0D10
            if ((flagDiff & 4) != 0) {
                cb = &g_4EB0.callbacks[14];
                if (cb != NULL) {
                    s32 oldGp = pspSetGp(cb->gp);
                    cb->func((endFlag >> 2) & 1, cb->argp);
                    pspSetGp(oldGp);
                }
            }
            // 0D50
            if ((flagDiff & 8) != 0) {
                s32 enable = 0;
                if ((g_4EB0.baryonStatus & 0x20) != 0 || (endFlag & 8) != 0) {
                    // 0D78
                    enable = 1;
                }
                // 0D80
                cb = &g_4EB0.callbacks[0];
                if (cb->func != NULL) {
                    s32 oldGp = pspSetGp(cb->gp);
                    cb->func(enable, cb->argp);
                    pspSetGp(oldGp);
                }
            }
            // 0DB0
            if ((flagDiff & 0x10) != 0) {
                cb = &g_4EB0.callbacks[19];
                // 0DC8 dup
                if (cb->func != NULL) {
                    s32 oldGp = pspSetGp(cb->gp);
                    // 0DE0 dup
                    cb->func((endFlag >> 4) & 1, cb->argp);
                    pspSetGp(oldGp);
                }
            }
            break;

        case 2: case 6: case 7: case 8: {
            // 1144
            s8 prev = g_4EB0.hpConnect;
            u8 newState = packet->rx[4] >> 7;
            g_4EB0.hpConnect = newState;
            if (prev == -1 || prev != newState) {
                cb = &g_4EB0.callbacks[4];
                if (cb->func != NULL) {
                    s32 oldGp = pspSetGp(cb->gp);
                    cb->func(newState, cb->argp);
                    pspSetGp(oldGp);
                }
            }
            // (11A0)
            // 11A4
            s8 oldState = g_4EB0.wlanSwitch;
            newState = ((packet->rx[4] ^ 0x40) >> 6) & 1;
            if (g_4EB0.wlanOverride >= 0)
                newState = g_4EB0.wlanOverride;
            g_4EB0.wlanSwitch = newState;
            if (oldState == -1 || oldState != newState) {
                cb = &g_4EB0.callbacks[5];
                if (cb->func != NULL) {
                    s32 oldGp = pspSetGp(cb->gp);
                    cb->func(newState, cb->argp);
                    pspSetGp(oldGp);
                }
            }
            // (120C)
            // 1210
            oldState = g_4EB0.btSwitch;
            newState = ((packet->rx[4] ^ 0x40) >> 6) & 1;
            if (g_4EB0.btOverride >= 0)
                newState = g_4EB0.btOverride;
            g_4EB0.btSwitch = newState;
            if (oldState == -1 || oldState != newState) {
                cb = &g_4EB0.callbacks[13];
                if (cb->func != NULL) {
                    s32 oldGp = pspSetGp(cb->gp);
                    cb->func(newState, cb->argp);
                    pspSetGp(oldGp);
                }
            }
            // (1278)
            newState = packet->rx[4];
            // 127C
            oldState = g_4EB0.holdSwitch;
            g_4EB0.holdSwitch = newState;
            if (oldState == -1 || oldState != (((newState ^ 0x20) >> 5) & 1)) {
                cb = &g_4EB0.callbacks[6];
                if (cb->func != NULL) {
                    s32 oldGp = pspSetGp(cb->gp);
                    cb->func(newState, cb->argp);
                    pspSetGp(oldGp);
                }
            }
            // (12D8)
            // 12DC
            if (packet->tx[0] < 7 || packet->tx[0] > 8)
                break;
            oldState = g_4EB0.umdSwitch;
            newState = (packet->rx[5] >> 4) & 1;
            g_4EB0.umdSwitch = newState;
            if (oldState == -1 || oldState != newState) {
                cb = &g_4EB0.callbacks[7];
                if (cb->func != NULL) {
                    s32 oldGp = pspSetGp(cb->gp);
                    cb->func(newState, cb->argp);
                    pspSetGp(oldGp);
                }
            }
            // (134C)
            // 1350
            oldState = g_4EB0.unk70;
            if (oldState >= -1) {
                newState = packet->rx[5] >> 7;
                g_4EB0.unk70 = newState;
                if (oldState == -1 || oldState != newState) {
                    cb = &g_4EB0.callbacks[16];
                    if (cb->func != NULL) {
                        s32 oldGp = pspSetGp(cb->gp);
                        cb->func(newState, cb->argp);
                        pspSetGp(oldGp);
                    }
                }
            }
            // (13B0)
            // 13B4
            oldState = g_4EB0.unk71;
            if (oldState >= -1) {
                newState = packet->rx[6] & 0x1;
                g_4EB0.unk71 = newState;
                if (oldState == -1 || oldState != newState) {
                    cb = &g_4EB0.callbacks[17];
                    if (cb->func != NULL) {
                        s32 oldGp = pspSetGp(cb->gp);
                        cb->func(newState, cb->argp);
                        pspSetGp(oldGp);
                    }
                }
            }
            // (1414)
            // 1418
            oldState = g_4EB0.unk72;
            if (oldState >= -1) {
                newState = ((packet->rx[6] ^ 2) >> 1) & 0x1;
                g_4EB0.unk72 = newState;
                if (oldState == -1 || oldState != newState) {
                    cb = &g_4EB0.callbacks[18];
                    if (cb->func != NULL) {
                        s32 oldGp = pspSetGp(cb->gp);
                        // 0DE0 dup
                        cb->func(newState, cb->argp);
                        pspSetGp(oldGp);
                    }
                }
            }
        }
            break;

        case 11:
            // 1474
            if (g_4EB0.unk352 == 0)
                break;
            if ((packet->rx[3] & 0x10) == 0)
                g_4EB0.baryonStatus &= 0xDF;
            else
                g_4EB0.baryonStatus |= 0x20;
            if ((((prev ^ g_4EB0.baryonStatus) | g_4EB0.baryonStatus) & 0x20) == 0)
                break;
            int enable = 0;
            if ((g_4EB0.baryonStatus & 0x20) != 0 || (g_4EB0.baryonStatus2 & 8) != 0) {
                // 14D0
                enable = 1;
            }
            // 14D8
            cb = &g_4EB0.callbacks[0];
            // 0DC8 dup
            if (cb->func != NULL) {
                s32 oldGp = pspSetGp(cb->gp);
                // 0DE0 dup
                cb->func(enable, cb->argp);
                pspSetGp(oldGp);
            }
            break;

        default:
            break;
        }
    }

    // 0DF0
    // (0DF4)
    // 0DF8
    // 0DFC
    s32 unk0;
    SceUID semaId;
    if ((packet->tx[0] & 0xFF) >= 32 && packet->rx[2] >= 128 && packet->rx[2] < 130 && (packet->status & 0x10) == 0) {
        // 112C
        packet->status |= 0x40000;
        semaId = -1;
        unk0 = 2;
    } else {
        // 0E2C
        s32 off = (packet->tx[0] >> 5) & 0x7;
        if ((packet->tx[0] & 0x80) != 0)
            off = 0;
        g_4EB0.packetList[off] = packet->next;
        packet->status &= 0xFFFBFFFF;
        if (packet->next == NULL)
            g_4EB0.packetList[off + 4] = NULL;
        // 0E64
        packet->status = (packet->status & 0xFFFEFFFF) | 0x80000;
        unk0 = 0;
        packet->next = NULL;
        semaId = packet->semaId;
        if (packet->callback != NULL) {
            // 1038
            s32 oldGp = pspSetGp(packet->gp);
            unk0 = packet->callback(packet, packet->argp);
            pspSetGp(oldGp);
            if (unk0 != 0) {
                if (unk0 == 1) {
                    // 10D0
                    off = (packet->tx[0] >> 5) & 7;
                    if ((packet->tx[0] & 0x80) != 0)
                        off = 0;
                    SceSysconPacket *cur = g_4EB0.packetList[off];
                    if (cur == NULL) {
                        g_4EB0.packetList[off + 4] = packet;
                        // 1120
                        g_4EB0.packetList[off] = packet;
                        packet->next = NULL;
                    } else {
                        g_4EB0.packetList[off] = packet;
                        packet->next = cur;
                    }
                    // 1104
                    semaId = -1;
                    packet->status = (packet->status & 0xFFF7FFFF) | 0x10000;
                } else if (unk0 == 2) {
                    off = (packet->tx[0] >> 5) & 0x7;
                    if ((packet->tx[0] & 0x80) != 0)
                        off = 0;
                    SceSysconPacket *cur = g_4EB0.packetList[off + 4];
                    if (cur != NULL)
                        cur->next = packet;
                    // 10AC
                    semaId = -1;
                    g_4EB0.packetList[off + 4] = packet;
                    packet->status = (packet->status & 0xFFF7FFFF) | 0x10000;
                    packet->next = NULL;
                }
            }
        }
    }
    // 0E8C
    // 0E90
    if (unkEnable && unk0 != 1) {
        SceSysconPacket *cur = g_4EB0.packetList[0];
        if (cur == NULL) {
            // 1028
            g_4EB0.packetList[4] = &g_4E50;
            g_4E50.next = NULL;
            g_4EB0.packetList[0] = &g_4E50;
        } else {
            g_4EB0.packetList[0] = &g_4E50;
            g_4E50.next = cur;
        }
        // 0EC0
    }
    // 0EC4
    SceSysconPacket *cur = g_4EB0.packetList[0];
    if (cur == NULL) {
        // 0F70
        // 0F7C
        s32 i;
        for (i = 0; i < 3; i++) {
            s32 n = g_4EB0.packetOff + i;
            s32 off = n + 1;
            if (off >= 4)
                off = n - 2;
            cur = g_4EB0.packetList[off];
            if (cur != NULL) {
                // 0FDC
                if ((cur->status & 0x40000) != 0 &&
                  startTime - cur->time >= cur->delay &&
                  g_4EB0.retryMode == 0 &&
                  g_4EB0.pollingMode == 0)
                    cur = NULL;
                break;
            }
        }
        // 0FB0
        // 0FB4
        g_4EB0.packetOff++;
        if (g_4EB0.packetOff == 3) {
            // 0FD4
            g_4EB0.packetOff = 0;
        }
        // 0FC4
    }
    if (cur != NULL) {
        // 0ED4
        u32 diffTime = 4;
        if (g_4EB0.pollingMode != 0)
            diffTime = g_4EB0.packetStartDelayIntr;
        // 0EE4
        while (diffTime >= sceKernelGetSystemTimeLow() - startTime)
            ;
        g_4EB0.curPacket = cur;
        _sceSysconPacketStart(cur);
    }
    // 0F08
    if (unk0 == 0 && semaId >= 0) {
        // 0F60
        sceKernelSignalSema(semaId, 1);
    }
    // 0F20
    g_4EB0.inGpioIntr = 0;
    sceKernelCpuResumeIntr(oldIntr);
    return -1;
}

// 154C
s32 sceSysconCmdExec(SceSysconPacket *packet, s32 unk)
{   
    dbg_printf("Run %s\n", __FUNCTION__);
    if (sceKernelIsIntrContext())
        return 0x80000030;

    if (g_4EB0.pollingMode == 0 && pspMfic() == 0)
        return 0x80000031;
    // 159C
    if (g_4EB0.pollingMode != 0 && pspMfic() != 0)
        return 0x80000031;
    // 15BC
    s32 ret = sceSysconCmdExecAsync(packet, unk, 0, 0);
    if (ret < 0)
        return ret;
    dbg_puts("?\n");
    return sceSysconCmdSync(packet, 0);
}

// 1600
s32 sceSysconCmdExecAsync(SceSysconPacket *packet, u32 flags, s32 (*callback)(SceSysconPacket*, void*), void *argp)
{
    dbg_printf("Run %s\n", __FUNCTION__);
    s32 i;
    if (g_4EB0.rebooted != 0) {
        dbg_printf("ended %d\n", __LINE__);
        return 0x80250003;
    }
    if ((flags & 0x100) == 0) {
        u32 hash = 0;
        // 1664
        for (i = 0; i < packet->tx[13]; i++)
            hash += packet->tx[i];
        // 167C
        packet->tx[i] = ~hash;
        // 1698
        for (i++; i < 16; i++)
            packet->tx[i] = -1;
        // 16AC
    }
    // 16B0
    // 16B8
    for (i = 0; i < 16; i++)
        packet->rx[i] = -1;
    s32 oldIntr = sceKernelCpuSuspendIntr();
    packet->status = 0x10000 | (flags & 0xFFFF);
    packet->semaId = -1;
    packet->callback = callback;
    packet->next = NULL;
    packet->gp = pspGetGp();
    u32 off = 0;
    if ((packet->tx[0] & 0x80) == 0)
        off = (packet->tx[0] >> 5) & 7;
    packet->argp = argp;
    if (off != 0) {
        // 1880
        packet->delay = g_4EB0.packetStartDelayIntr;
    }
    else
        packet->delay = 0;
    // 1720
    SceSysconPacket *cur = g_4EB0.packetList[off];
    if (cur == NULL) {
        // 1878
        g_4EB0.packetList[off + 4] = packet;
        // 1754 dup
        g_4EB0.packetList[off] = packet;
    } else if ((flags & 1) == 0) {
        // 186C
        g_4EB0.packetList[off + 4]->next = packet;
        // 1754 dup
        g_4EB0.packetList[off + 4] = packet;
    } else if (((cur->status >> 17) & 3) != 0) {
        SceSysconPacket *prev = cur->next;
        // 1858
        packet->next = cur->next;
        cur->next = packet;
        if (prev == NULL)
            g_4EB0.packetList[off + 4] = packet;
    } else
        packet->next = cur;
    // 1758
    if (g_4EB0.pollingMode != 0) {
        // 1848
        g_4EB0.curPacket = packet;
        // 17F8 dup
        _sceSysconPacketStart(packet);
    } else {
        if (g_4EB0.curPacket != NULL || g_4EB0.inGpioIntr) {
            // 1800 dup
            sceKernelCpuResumeIntr(oldIntr);
            dbg_printf("ended %d [%d, %d]\n", __LINE__, g_4EB0.curPacket != NULL, g_4EB0.inGpioIntr);
            return 0;
        }
        SceSysconPacket *new = g_4EB0.packetList[0];
        if (new != NULL) {
            if (new == packet) {
                // 183C
                new = NULL;
                if ((flags & 2) == 0)
                    new = packet;
            }
        }
        // 1790
        if (off != 0 && (new == NULL || (flags & 1) != 0)) {
            // 17A8
            new = g_4EB0.packetList[off];
            if (new == packet) {
                // 1830
                new = NULL;
                if ((flags & 2) == 0)
                    new = packet;
            }
        }
        // 17BC
        if (new == NULL) {
            // 1800 dup
            sceKernelCpuResumeIntr(oldIntr);
            dbg_printf("ended %d\n", __LINE__);
            return 0;
        }
        u32 timeDiff = 4;
        if (g_4EB0.retryMode != 0)
            timeDiff = g_4EB0.packetStartDelay;
        // 17D4
        // 17D8
        while (timeDiff >= sceKernelGetSystemTimeLow() - g_4EB0.startTime)
            ;
        g_4EB0.curPacket = new;
        // 17F8 dup
        _sceSysconPacketStart(new);
    }
    // 1800 dup
    sceKernelCpuResumeIntr(oldIntr);
    dbg_printf("ended %d\n", __LINE__);
    return 0;
}

// 1890
s32 sceSysconCmdCancel(SceSysconPacket *packet)
{
    dbg_printf("Run %s\n", __FUNCTION__);
    s32 oldIntr = sceKernelCpuSuspendIntr();
    u32 off = (packet->tx[0] >> 5) & 7;
    if ((packet->tx[0] & 0x80) != 0) {
        // 19A8
        off = 0;
    }
    // 18C8
    SceSysconPacket *cur = g_4EB0.packetList[off];
    s32 ret = 0x80000025;
    SceSysconPacket *prev = NULL;
    // 18E0
    while (cur != NULL) {
        if (cur == packet) {
            // 1918
            ret = 0x80000021;
            if ((cur->status & 0x20000) != 0)
                goto end;
            if (prev == NULL) {
                // 197C
                off = ((cur->tx[0] >> 5) & 7) * 4;
                if ((cur->tx[0] & 0x80) != 0) {
                    // 19A0
                    off = 0;
                }
                // 1990
                g_4EB0.packetList[off] = cur->next;
            } else
                prev->next = cur->next;

            // 193C
            ret = 0;
            if (cur->next != NULL)
                goto end;
            off = ((packet->tx[0] >> 5) & 7) + 4;
            if ((packet->tx[0] & 0x80) != 0) {
                // 1974
                off = 4;
            }
            // 1964
            g_4EB0.packetList[off] = prev;
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

// 19B0
s32 sceSysconCmdSync(SceSysconPacket *packet, s32 arg1)
{
    dbg_printf("Run %s\n", __FUNCTION__);
    if (arg1 != 0) {
        // 1BAC
        if (arg1 != 1)
            return 0x80000107;
        if (packet == NULL) {
            // 1BDC
            if (g_4EB0.packetList[0] == NULL &&
              g_4EB0.packetList[1] == NULL &&
              g_4EB0.packetList[2] == NULL &&
              g_4EB0.packetList[3] == NULL)
                return 0;
            return 1;
        }
        if ((packet->status & 0x80000) == 0)
            return 1;
    } else if (g_4EB0.pollingMode == 0) {
        // 1AD0
        if (sceKernelIsIntrContext() != 0)
            return 0x80000030;
        s32 oldIntr = sceKernelCpuSuspendIntr();
        if (oldIntr == 0) {
            // 1B98
            sceKernelCpuResumeIntr(0);
            return 0x80000031;
        }
        if ((packet->status & 0x80000) != 0) {
            // 1B88
            sceKernelCpuResumeIntr(oldIntr);
        } else {
            SceUID oldSemaId = g_4EB0.semaId;
            SceUID semaId = oldSemaId;
            if (oldSemaId <= 0) {
                // 1B54
                semaId = sceKernelCreateSema("SceSysconSync", 1, 0, 1, 0);
                if (semaId < 0) {
                    sceKernelCpuResumeIntr(oldIntr);
                    return semaId;
                }
            }
            else
                g_4EB0.semaId = 0;
            // 1B18
            packet->semaId = semaId;
            sceKernelCpuResumeIntr(oldIntr);
            sceKernelWaitSema(semaId, 1, 0);
            if (oldSemaId == semaId) {
                // 1B4C
                g_4EB0.semaId = oldSemaId;
            }
            else
                sceKernelDeleteSema(semaId);
            // 1B44
        }
    } else {
        // 1A04
        while ((packet->status & 0x80000) == 0) {
            while (sceKernelGetSystemTimeLow() - g_4EB0.startTime < 5)
                ;
            // 1A20
            while (sceGpioQueryIntr(4) == 0)
                ;
            _sceSysconGpioIntr(4, 0, NULL);
        }
    }
    // (1A4C)
    // 1A50
    if ((packet->status & 0xB00000) != 0)
        return 0x80250002;
    if ((packet->rx[2] >> 7) && packet->rx[2] != 0x82)
        return 0x80250000 | packet->rx[2];
    if (packet->tx[0] < 32)
        return 0;
    if ((packet->rx[2] & 0x80) == 0)
        return 0x80250004;
    return 0;
}

// 1C1C
s32 _sceSysconCommonRead(s32 *ptr, s32 cmd)
{   
    dbg_printf("Run %s\n", __FUNCTION__);
    if (ptr == NULL)
        return 0x80000103;
    SceSysconPacket packet;
    s32 buf[4];
    packet.tx[1] = 2;
    packet.tx[0] = cmd;
    s32 ret = sceSysconCmdExec(&packet, 0);
    if (ret < 0)
        return ret;
    if (packet.rx[1] < 4 || packet.rx[1] >= 8)
        return 0x80250001;
    buf[0] = 0;
    memcpy(buf, &packet.rx[3], packet.rx[1] - 3);
    *ptr = buf[0];
    return 0;
}

// 1CB0
s32 _sceSysconModuleStart(s32 argc __attribute__((unused)), void *argp __attribute__((unused)))
{
    dbg_init(1, FB_NONE, FAT_HARDWARE);
    dbg_printf("Run %s\n", __FUNCTION__);
    sceSysconInit();
    return 0;
}

// 1CD0
s32 sceSyscon_driver_90EAEA2B(s32 arg0, s32 arg1)
{
    dbg_printf("Run %s\n", __FUNCTION__);
    g_4EB0.packetStartDelayIntr = arg1;
    g_4EB0.packetStartDelay = arg0;
    return 0;
}

// 1CE8
s32 sceSyscon_driver_755CF72B(s32 *arg0, s32 *arg1)
{
    dbg_printf("Run %s\n", __FUNCTION__);
    if (arg0 != NULL)
        *arg0 = g_4EB0.packetStartDelay;
    if (arg1 != NULL)
        *arg1 = g_4EB0.packetStartDelayIntr;
    return 0;
}

// 1D10
s32 sceSysconSuspend(void)
{   
    dbg_printf("Run %s\n", __FUNCTION__);
    if (g_4EB0.unk376 != 0) {
        // 1D44
        sceSyscon_driver_765775EB(0);
    }
    // 1D2C
    sceKernelDisableSubIntr(4, 4);
    return 0;
}

// 1D54
s32 sceSysconSetDebugHandlers(SceSysconDebugHandlers *handlers)
{
    dbg_printf("Run %s\n", __FUNCTION__);
    g_4EB0.dbgHandlers = handlers;
    return 0;
}

// 1D64
s32 sceSysconSetPollingMode(u32 pollingMode)
{
    dbg_printf("Run %s\n", __FUNCTION__);
    g_4EB0.pollingMode = pollingMode;
    return 0;
}

// 1D74
s32 sceSysconSetAffirmativeRertyMode(u32 mode)
{
    dbg_printf("Run %s\n", __FUNCTION__);
    g_4EB0.retryMode = mode;
    return 0;
}

// 1D84
s32 _sceSysconGetBaryonVersion(void)
{
    dbg_printf("Run %s\n", __FUNCTION__);
    return g_4EB0.baryonVersion;
}

// 1D90
u64 _sceSysconGetBaryonTimeStamp(void)
{
    dbg_printf("Run %s\n", __FUNCTION__);
    return g_4EB0.timeStamp;
}

// 1DA4
s32 _sceSysconGetPommelVersion(void)
{
    dbg_printf("Run %s\n", __FUNCTION__);
    return g_4EB0.pommelVersion;
}

// 1DB0
s32 _sceSysconGetUsbPowerType(void)
{   
    dbg_printf("Run %s\n", __FUNCTION__);
    u16 type = g_4EB0.baryonVersion >> 16;
    if ((type & 0xF0) != 0 && (type & 0xF0) != 0x10 && ((type & 0xFF) < 32 || (type & 0xFF) >= 34))
        return 1;
    return 0;
}

// 1DF4
s32 sceSysconSetGSensorCallback(SceSysconFunc func, void *argp)
{
    dbg_printf("Run %s\n", __FUNCTION__);
    return _sceSysconSetCallback(func, argp, 10);
}

// 1E10
s32 sceSysconSetLowBatteryCallback(SceSysconFunc func, void *argp)
{
    dbg_printf("Run %s\n", __FUNCTION__);
    return _sceSysconSetCallback(func, argp, 0);
}

// 1E2C
s32 sceSysconSetPowerSwitchCallback(SceSysconFunc func, void *argp)
{
    dbg_printf("Run %s\n", __FUNCTION__);
    return _sceSysconSetCallback(func, argp, 1);
}

// 1E48
s32 sceSysconSetAlarmCallback(SceSysconFunc func, void *argp)
{
    dbg_printf("Run %s\n", __FUNCTION__);
    return _sceSysconSetCallback(func, argp, 2);
}

// 1E64
s32 sceSysconSetAcSupplyCallback(SceSysconFunc func, void *argp)
{
    dbg_printf("Run %s\n", __FUNCTION__);
    return _sceSysconSetCallback(func, argp, 3);
}

// 1E80
s32 sceSysconSetAcSupply2Callback(SceSysconFunc func, void *argp)
{
    dbg_printf("Run %s\n", __FUNCTION__);
    return _sceSysconSetCallback(func, argp, 15);
}

// 1E9C
s32 sceSysconSetHPConnectCallback(SceSysconFunc func, void *argp)
{
    dbg_printf("Run %s\n", __FUNCTION__);
    return _sceSysconSetCallback(func, argp, 4);
}

// 1EB8
s32 sceSysconSetHRPowerCallback(SceSysconFunc func, void *argp)
{
    dbg_printf("Run %s\n", __FUNCTION__);
    return _sceSysconSetCallback(func, argp, 8);
}

// 1ED4
s32 sceSysconSetHRWakeupCallback(SceSysconFunc func, void *argp)
{
    dbg_printf("Run %s\n", __FUNCTION__);
    return _sceSysconSetCallback(func, argp, 14);
}

// 1EF0
s32 sceSysconSetWlanSwitchCallback(SceSysconFunc func, void *argp)
{
    dbg_printf("Run %s\n", __FUNCTION__);
    return _sceSysconSetCallback(func, argp, 5);
}

// 1F0C
s32 sceSysconSetWlanPowerCallback(SceSysconFunc func, void *argp)
{
    dbg_printf("Run %s\n", __FUNCTION__);
    return _sceSysconSetCallback(func, argp, 9);
}

// 1F28
s32 sceSysconSetBtSwitchCallback(SceSysconFunc func, void *argp)
{
    dbg_printf("Run %s\n", __FUNCTION__);
    return _sceSysconSetCallback(func, argp, 13);
}

// 1F44
s32 sceSysconSetBtPowerCallback(SceSysconFunc func, void *argp)
{
    dbg_printf("Run %s\n", __FUNCTION__);
    return _sceSysconSetCallback(func, argp, 12);
}

// 1F60
s32 sceSysconSetHoldSwitchCallback(SceSysconFunc func, void *argp)
{
    dbg_printf("Run %s\n", __FUNCTION__);
    return _sceSysconSetCallback(func, argp, 6);
}

// 1F7C
s32 sceSysconSetUmdSwitchCallback(SceSysconFunc func, void *argp)
{
    dbg_printf("Run %s\n", __FUNCTION__);
    return _sceSysconSetCallback(func, argp, 7);
}

// 1F98
s32 sceSyscon_driver_374373A8(SceSysconFunc func, void *argp)
{
    dbg_printf("Run %s\n", __FUNCTION__);
    return _sceSysconSetCallback(func, argp, 16);
}

// 1FB4
s32 sceSyscon_driver_B761D385(SceSysconFunc func, void *argp)
{
    dbg_printf("Run %s\n", __FUNCTION__);
    return _sceSysconSetCallback(func, argp, 17);
}

// 1FD0
s32 sceSyscon_driver_26307D84(SceSysconFunc func, void *argp)
{
    dbg_printf("Run %s\n", __FUNCTION__);
    return _sceSysconSetCallback(func, argp, 18);
}

// 1FEC
s32 sceSyscon_driver_6C388E02(SceSysconFunc func, void *argp)
{
    dbg_printf("Run %s\n", __FUNCTION__);
    return _sceSysconSetCallback(func, argp, 19);
}

// 2008
s32 _sceSysconCommonWrite(u32 val, u32 cmd, u32 size)
{   
    dbg_printf("Run %s\n", __FUNCTION__);
    SceSysconPacket packet;
    packet.tx[0] = cmd;
    packet.tx[1] = size;
    packet.tx[3] = (val >> 8);
    packet.tx[4] = (val >> 16);
    packet.tx[5] = (val >> 24);
    packet.tx[2] = val;
    return pspMin(sceSysconCmdExec(&packet, 0), 0);
}

// 205C
u8 sceSysconGetBaryonStatus(void)
{
    dbg_printf("Run %s\n", __FUNCTION__);
    return g_4EB0.baryonStatus;
}

// 2068
u8 sceSysconGetBaryonStatus2(void)
{
    dbg_printf("Run %s\n", __FUNCTION__);
    return g_4EB0.baryonStatus2;
}

// 2074
u8 sceSysconIsFalling(void)
{
    dbg_printf("Run %s\n", __FUNCTION__);
    return (g_4EB0.baryonStatus & 0x80) ? 1 : 0;
}

// 2084
u8 sceSysconIsLowBattery(void)
{
    dbg_printf("Run %s\n", __FUNCTION__);
    return ((g_4EB0.baryonStatus >> 5) & 1) | ((g_4EB0.baryonStatus2 >> 3) & 1);
}

// 20A4
u8 sceSysconGetPowerSwitch(void)
{
    dbg_printf("Run %s\n", __FUNCTION__);
    return (g_4EB0.baryonStatus >> 4) & 1;
}

// 20B4
u8 sceSysconIsAlarmed(void)
{
    dbg_printf("Run %s\n", __FUNCTION__);
    return ((g_4EB0.baryonStatus ^ 8) >> 3) & 1;
}

// 20C8
u8 sceSysconIsAcSupplied(void)
{
    dbg_printf("Run %s\n", __FUNCTION__);
    return g_4EB0.baryonStatus & 1;
}

// 20D8
s8 sceSysconGetHPConnect(void)
{
    dbg_printf("Run %s\n", __FUNCTION__);
    return g_4EB0.hpConnect;
}

// 20E4
s8 sceSysconGetWlanSwitch(void)
{
    dbg_printf("Run %s\n", __FUNCTION__);
    if (g_4EB0.wlanOverride < 0)
        return g_4EB0.wlanSwitch;
    return g_4EB0.wlanOverride;
}

// 2108
s8 sceSyscon_driver_0B51E34D(s8 wlanSwitch)
{
    dbg_printf("Run %s\n", __FUNCTION__);
    s8 prev = g_4EB0.wlanOverride;
    g_4EB0.wlanOverride = wlanSwitch;
    return prev;
}

// 211C
s8 sceSysconGetBtSwitch(void)
{
    dbg_printf("Run %s\n", __FUNCTION__);
    if (g_4EB0.btOverride < 0)
        return g_4EB0.btSwitch;
    return g_4EB0.btOverride;
}

// 2140
s8 sceSyscon_driver_BADF1260(s8 btSwitch)
{
    dbg_printf("Run %s\n", __FUNCTION__);
    s8 prev = g_4EB0.btOverride;
    g_4EB0.btOverride = btSwitch;
    return prev;
}

// 2154
s8 sceSysconGetHoldSwitch(void)
{
    dbg_printf("Run %s\n", __FUNCTION__);
    return g_4EB0.holdSwitch;
}

// 2160
s8 sceSysconGetUmdSwitch(void)
{
    dbg_printf("Run %s\n", __FUNCTION__);
    return g_4EB0.umdSwitch;
}

// 216C
s8 sceSyscon_driver_248335CD(void)
{
    dbg_printf("Run %s\n", __FUNCTION__);
    return g_4EB0.unk70;
}

// 2178
s8 sceSyscon_driver_040982CD(void)
{
    dbg_printf("Run %s\n", __FUNCTION__);
    return g_4EB0.unk71;
}

// 2184
s8 sceSyscon_driver_97765E27(void)
{
    dbg_printf("Run %s\n", __FUNCTION__);
    return g_4EB0.unk72;
}

// 2190
u8 sceSysconGetHRPowerStatus(void)
{
    dbg_printf("Run %s\n", __FUNCTION__);
    return (g_4EB0.baryonStatus >> 2) & 1;
}

// 21A0
u8 sceSysconGetHRWakeupStatus(void)
{
    dbg_printf("Run %s\n", __FUNCTION__);
    return (g_4EB0.baryonStatus2 >> 2) & 1;
}

// 21B0
u8 sceSysconGetWlanPowerStatus(void)
{
    dbg_printf("Run %s\n", __FUNCTION__);
    return (g_4EB0.baryonStatus >> 1) & 1;
}

// 21C0
u8 sceSysconGetBtPowerStatus(void)
{
    dbg_printf("Run %s\n", __FUNCTION__);
    return (g_4EB0.baryonStatus2 >> 1) & 1;
}

// 21D0
u8 sceSyscon_driver_DF20C984(void)
{
    dbg_printf("Run %s\n", __FUNCTION__);
    return (g_4EB0.baryonStatus2 >> 4) & 1;
}

// 21E0
s8 sceSysconGetLeptonPowerCtrl(void)
{
    dbg_printf("Run %s\n", __FUNCTION__);
    return g_4EB0.leptonPower;
}

// 21EC
s8 sceSysconGetMsPowerCtrl(void)
{
    dbg_printf("Run %s\n", __FUNCTION__);
    return g_4EB0.msPower;
}

// 21F8
s8 sceSysconGetWlanPowerCtrl(void)
{
    dbg_printf("Run %s\n", __FUNCTION__);
    return g_4EB0.wlanPower;
}

// 2204
s8 sceSysconGetHddPowerCtrl(void)
{
    dbg_printf("Run %s\n", __FUNCTION__);
    return g_4EB0.hddPower;
}

// 2210
s8 sceSysconGetDvePowerCtrl(void)
{
    dbg_printf("Run %s\n", __FUNCTION__);
    return g_4EB0.dvePower;
}

// 221C
s8 sceSysconGetBtPowerCtrl(void)
{
    dbg_printf("Run %s\n", __FUNCTION__);
    return g_4EB0.btPower;
}

// 2228
s8 sceSysconGetUsbPowerCtrl(void)
{
    dbg_printf("Run %s\n", __FUNCTION__);
    return g_4EB0.usbPower;
}

// 2234
s8 sceSysconGetTachyonVmePowerCtrl(void)
{
    dbg_printf("Run %s\n", __FUNCTION__);
    return g_4EB0.tachyonVmePower;
}

// 2240
s8 sceSysconGetTachyonAwPowerCtrl(void)
{
    dbg_printf("Run %s\n", __FUNCTION__);
    return g_4EB0.tachyonAwPower;
}

// 224C
s8 sceSysconGetTachyonAvcPowerCtrl(void)
{
    dbg_printf("Run %s\n", __FUNCTION__);
    return g_4EB0.tachyonAvcPower;
}

// 2258
s8 sceSysconGetLcdPowerCtrl(void)
{
    dbg_printf("Run %s\n", __FUNCTION__);
    return g_4EB0.lcdPower;
}

// 2264
s8 sceSysconGetHRPowerCtrl(void)
{
    dbg_printf("Run %s\n", __FUNCTION__);
    return g_4EB0.hrPower;
}

// 2270
s8 sceSysconGetWlanLedCtrl(void)
{
    dbg_printf("Run %s\n", __FUNCTION__);
    return g_4EB0.wlanLed;
}

// 227C
s32 _sceSysconSetCallback(SceSysconFunc func, void *argp, s32 cbId)
{   
    dbg_printf("Run %s\n", __FUNCTION__);
    s32 oldIntr = sceKernelCpuSuspendIntr();
    g_4EB0.callbacks[cbId].func = func;
    g_4EB0.callbacks[cbId].argp = argp;
    g_4EB0.callbacks[cbId].gp = pspGetGp();
    sceKernelCpuResumeIntr(oldIntr);
    return 0;
}

// 22F0
s32 _sceSysconPacketStart(SceSysconPacket *packet)
{
    packet->time = sceKernelGetSystemTimeLow();
    if (g_4EB0.dbgHandlers != NULL) {
        // 2420
        g_4EB0.dbgHandlers->start(packet);
    }
    // 231C
    sceGpioPortRead();
    sceGpioPortClear(8);
    // 2340
    while ((HW(0xBE58000C) & 4) != 0)
        HW(0xBE580008);
    // 235C
    HW(0xBE58000C);
    HW(0xBE580020) = 3;
    if ((packet->status & 0x40000) == 0) {
        // 23D8
        s32 i;
        // 23EC
        for (i = 0; i < packet->tx[1] + 1; i += 2) {
            HW(0xBE58000C);
            HW(0xBE580008) = (packet->tx[i] << 8) | packet->tx[i + 1];
        }
    } else {
        HW(0xBE580008) = (packet->tx[0] << 8) | 2;
        HW(0xBE580008) = ((packet->tx[0] + 2) << 8) ^ 0xFFFF;
    }
    // (23A0)
    // 23A4
    HW(0xBE580004) = 6;
    sceGpioPortSet(8);
    packet->status |= 0x20000;
    return 0;
}

// 2434
s32 _sceSysconPacketEnd(SceSysconPacket *packet)
{
    s32 ret = 0;
    s32 i;
    if ((HW(0xBE58000C) & 4) == 0) {
        packet->rx[0] = -1;
        ret = -1;
        packet->status |= 0x100000;
        packet->rx[1] = 0;
        // 2488
        for (i = 0; i < 16; i++)
            asm("\n"); /* Here be dragons. */
    }
    // 2490
    if ((HW(0xBE58000C) & 1) == 0) {
        ret = -1;
        packet->status |= 0x200000;
    }
    // 24B4
    if ((HW(0xBE580018) & 1) != 0) {
        HW(0xBE580020) = 1;
        packet->status |= 0x400000;
    }
    // 24E4
    // 24E8
    for (i = 0; i < 16; i += 2) {
        if ((HW(0xBE58000C) & 4) == 0)
            break;
        u16 v = HW(0xBE580008) & 0xFFFF;
        if (i == 0)
            ret = v >> 8;
        // 2514
        packet->rx[i + 0] = v >> 8;
        packet->rx[i + 1] = v;
    }
    // 2530
    HW(0xBE580004) = 4;
    sceGpioPortClear(8);
    if (g_4EB0.dbgHandlers != NULL) {
        // 25E0
        g_4EB0.dbgHandlers->end(packet);
    }
    // 254C
    if (ret >= 0) {
        u32 hash = 0;
        if (packet->rx[1] < 3) {
            dbg_printf("TOO SHORT\n");
            ret = -2;
        }
        else if (packet->rx[1] < 16) {
            // 2598, 25A8
            for (i = 0; i < packet->rx[1]; i++)
                hash = (hash + packet->rx[i]) & 0xFF;
            // 25C4
            if ((packet->rx[packet->rx[1]] ^ (~hash & 0xFF)) != 0) {
                dbg_printf("INVALID HASH\n");
                ret = -2;
            }
        } else {
            dbg_printf("TOO LONG\n");
            ret = -2;
        }
    }
    else
        dbg_printf("FAIL\n");
    // 2578
    packet->status &= 0xFFFDFFFF;
    return ret;
}

// 25F0
s32 sceSysconGetTimeStamp(s8 *timeStamp)
{   
    dbg_printf("Run %s\n", __FUNCTION__);
    SceSysconPacket packet;
    if (timeStamp == NULL)
        return 0x80000103;
    packet.tx[0] = 0x11;
    packet.tx[1] = 2;
    s32 ret = sceSysconCmdExec(&packet, 0);
    if (ret < 0)
        return ret;
    memcpy(timeStamp, &packet.rx[3], 12); /* inline */
    timeStamp[12] = '\0';
    return 0;
}

// 267C
s32 sceSysconWriteScratchPad(u32 dst, void *src, u32 size)
{   
    dbg_printf("Run %s\n", __FUNCTION__);
    SceSysconPacket packet;
    if ((size < 1 || size >= 3) && size != 4 && size != 8)
        return 0x80000104;
    // 26BC
    if ((dst % size) != 0)
        return 0x80000102;
    if (dst + size >= 33)
        return 0x80000102;
    packet.tx[0] = 0x23;
    packet.tx[1] = size + 3;
    packet.tx[2] = (dst << 2) | ((size / 2) != 4 ? size / 2 : 3);
    memcpy(&packet.tx[3], src, size);
    return pspMin(sceSysconCmdExec(&packet, 0), dst % size);
}

// 274C
s32 sceSysconReadScratchPad(u32 src, void *dst, u32 size)
{
    dbg_printf("Run %s\n", __FUNCTION__);
    SceSysconPacket packet;
    if ((size < 1 || size >= 3) && size != 4 && size != 8)
        return 0x80000104;
    // 2790
    // 27A8
    if ((src % size) != 0)
        return 0x80000102;
    if (src + size > 32)
        return 0x80000102;
    packet.tx[0] = 0x24;
    packet.tx[1] = 3;
    packet.tx[2] = (src << 2) | ((size >> 1) != 4) ? (size >> 1) : 3;
    s32 ret = sceSysconCmdExec(&packet, 0);
    if (ret < 0)
        return ret;
    memcpy(dst, &packet.rx[3], size);
    return 0;
}

// 282C
s32 sceSysconSendSetParam(u32 id, void *param)
{
    dbg_printf("Run %s\n", __FUNCTION__);
    SceSysconPacket packet;
    if (((_sceSysconGetBaryonVersion() >> 16) & 0xFF) < 0x12 &&
      ((_sceSysconGetBaryonVersion() & 0xFF) != 1 ||
      _sceSysconGetBaryonTimeStamp() <= 200509260441)) {
        // 28CC
        if (id != 0)
            return 0x80250011;
        packet.tx[0] = 0x25;
        packet.tx[1] = 10;
        memcpy(&packet.tx[2], param, 8);
    }
    else {
        // 2858
        packet.tx[0] = 0x25;
        packet.tx[1] = 11;
        memcpy(&packet.tx[2], param, 8);
        packet.tx[10] = id;
    }
    // 288C
    return pspMin(sceSysconCmdExec(&packet, 0), 0);
}

// 2944
s32 sceSysconReceiveSetParam(u32 id, void *param)
{   
    dbg_printf("Run %s\n", __FUNCTION__);
    SceSysconPacket packet;
    if (((_sceSysconGetBaryonVersion() >> 16) & 0xFF) < 0x12 &&
      ((_sceSysconGetBaryonVersion() & 0xFF) != 1 ||
      _sceSysconGetBaryonTimeStamp() <= 200509260441)) {
        // 29E8
        if (id != 0)
            return 0x80250011;
        packet.tx[0] = 0x26;
        packet.tx[1] = 2;
    } else {
        // 2970
        packet.tx[0] = 0x26;
        packet.tx[1] = 3;
        packet.tx[2] = id;
    }
    // 2980
    s32 ret = sceSysconCmdExec(&packet, 0);
    if (ret < 0)
        return ret;
    memcpy(param, &packet.rx[3], 8);
    return 0;
}

// 2A40
s32 sceSysconCtrlTachyonWDT(s32 wdt)
{   
    dbg_printf("Run %s\n", __FUNCTION__);
    if (wdt >= 0x80)
        return 0x800001FE;
    return _sceSysconCommonWrite(wdt == 0 ? 0 : wdt | 0x80, 0x31, 3);
}

// 2A88
s32 sceSysconResetDevice(u32 reset, u32 mode)
{
    dbg_printf("Run %s\n", __FUNCTION__);
    if (reset == 1) {
        // 2B24
        if (mode != 1) {
            reset = 0x41;
            if (mode != 2)
                return 0x80000107;
        }
    } else if (mode != 0)
        reset |= 0x80;
    // 2AB4
    s32 ret = _sceSysconCommonWrite(reset, 0x32, 3);
    if (ret < 0)
        return ret;
    if (reset == 4 && mode == 0 && sceSysconGetWlanLedCtrl() == 1)
        sceSysconCtrlLED(1, 1);
    return ret;
}

// 2B40
s32 sceSyscon_driver_12518439(u32 arg0)
{   
    dbg_printf("Run %s\n", __FUNCTION__);
    if (((_sceSysconGetBaryonVersion() >> 16) & 0xF0) >= 0x30) {
        // 2BC8
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
    // 2BA0
    sub_2D08(0x13FC0, 0, 0);
    return 0;
}

// 2BF4
s32 sceSysconPowerSuspend(u32 arg0, u32 arg1)
{   
    dbg_printf("Run %s\n", __FUNCTION__);
    u32 set = 0;
    void *sm1 = sceKernelSm1ReferOperations();
    if (sm1 != NULL) {
        s32 (*func)() = *(void**)(sm1 + 16);
        if (func() == 21325) {
            func = *(void**)(sm1 + 16);
            // 2CF8
            set = (func() < 6);
        }
    }
    // 2C48
    if (((_sceSysconGetBaryonVersion() >> 16) & 0xF0) >= 0x30) {
        // 2CCC
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
    // 2C94
    sub_2D08(0x13FC0, set, arg1);
    return 0;
}

// 2D08
s32 sub_2D08(u32 addr, u32 arg1, u32 arg2)
{   
    dbg_printf("Run %s\n", __FUNCTION__);
    sceKernelCpuSuspendIntr();
    memcpy((void*)0x10000, sub_0000, (void*)_sceSysconModuleRebootBefore - (void*)sub_0000);
    sceKernelDcacheWritebackAll();
    sceKernelIcacheInvalidateAll();
    void (*func)(s32, s32, s32) = (void*)0x10000;
    func(addr, arg1, arg2);
    return 0;
}

// 2D88
void sceSysconNop(void)
{
    dbg_printf("Run %s\n", __FUNCTION__);
    SceSysconPacket packet;
    packet.tx[0] = 0;
    packet.tx[1] = 2;
    sceSysconCmdExec(&packet, 0);
}

// 2DB4
s32 sceSysconGetBaryonVersion(s32 *baryonVersion)
{
    dbg_printf("Run %s\n", __FUNCTION__);
    return _sceSysconCommonRead(baryonVersion, 1);
}

// 2DD0
s32 sceSysconGetGValue(void)
{
    dbg_printf("Run %s\n", __FUNCTION__);
    return 0x80000004;
}

// 2DDC
s32 sceSysconGetPowerSupplyStatus(void *status)
{
    dbg_printf("Run %s\n", __FUNCTION__);
    return _sceSysconCommonRead(status, 0xB);
}

// 2DF8
s32 sceSysconGetFallingDetectTime(void)
{
    dbg_printf("Run %s\n", __FUNCTION__);
    return 0x80000004;
}

// 2E04
s32 sceSysconGetWakeUpFactor(void *factor)
{
    dbg_printf("Run %s\n", __FUNCTION__);
    return _sceSysconCommonRead(factor, 0xE);
}

// 2E20
s32 sceSysconGetWakeUpReq(void *req)
{
    dbg_printf("Run %s\n", __FUNCTION__);
    return _sceSysconCommonRead(req, 0xF);
}

// 2E3C
s32 sceSysconGetVideoCable(s32 *cable)
{
    dbg_printf("Run %s\n", __FUNCTION__);
    u32 ver = _sceSysconGetBaryonVersion() >> 16;
    if ((ver & 0xF0) == 0 || (ver & 0xF0) == 0x10) {
        // 2E9C
        *cable = 0;
        return 0x80000004;
    }
    return _sceSysconCommonRead(cable, 0x12);
}

// 2EA4
s32 sceSysconReadClock(s32 *clock)
{
    dbg_printf("Run %s\n", __FUNCTION__);
    return _sceSysconCommonRead(clock, 9);
}

// 2EC0
s32 sceSysconWriteClock(s32 clock)
{
    dbg_printf("Run %s\n", __FUNCTION__);
    return _sceSysconCommonWrite(clock, 0x20, 6);
}

// 2EE0
s32 sceSysconReadAlarm(s32 *alarm)
{
    dbg_printf("Run %s\n", __FUNCTION__);
    return _sceSysconCommonRead(alarm, 0xA);
}

// 2EFC
s32 sceSysconWriteAlarm(s32 alarm)
{
    dbg_printf("Run %s\n", __FUNCTION__);
    return _sceSysconCommonWrite(alarm, 0x22, 6);
}

// 2F1C
s32 sceSysconSetUSBStatus(u8 status)
{
    dbg_printf("Run %s\n", __FUNCTION__);
    SceSysconPacket packet;
    packet.tx[0] = 0x21;
    packet.tx[1] = 3;
    packet.tx[2] = status;
    return sceSysconCmdExec(&packet, (status & 3) == 0);
}

// 2F58
s32 sceSysconGetTachyonWDTStatus(s32 *status)
{
    dbg_printf("Run %s\n", __FUNCTION__);
    return _sceSysconCommonRead(status, 0xC);
}

// 2F74
s32 sceSysconCtrlAnalogXYPolling(s8 polling)
{
    dbg_printf("Run %s\n", __FUNCTION__);
    return _sceSysconCommonWrite(polling, 0x33, 3);
}

// 2F94
s32 sceSysconCtrlHRPower(s8 power)
{
    dbg_printf("Run %s\n", __FUNCTION__);
    s32 ret = _sceSysconCommonWrite(power, 0x34, 3);
    if (ret >= 0)
        g_4EB0.hrPower = power;
    return ret;
}

// 2FCC
s32 sceSysconCtrlPower(u32 arg0, u32 arg1)
{
    dbg_printf("Run %s\n", __FUNCTION__);
    s32 ret = _sceSysconCommonWrite(((arg1 & 1) << 23) | (arg0 & 0x003FFFFF), 0x45, 5);
    if (ret >= 0)
        _sceSysconGetPommelType();
    return ret;
}

// 301C
s32 sceSysconCtrlLED(u32 arg0, u32 arg1)
{
    dbg_printf("Run %s\n", __FUNCTION__);
    u8 himask, lomask;
    switch (arg0) {
    case 1:
        // 30E0
        himask = 0x80;
        g_4EB0.wlanLed = arg1;
        break;

    case 0:
        himask = 0x40;
        break;

    case 2:
        // 30D8
        himask = 0x20;
        break;

    case 3:
        himask = 0x10;
        if (_sceSysconGetPommelType() < 0x300)
            return 0x80000004;
        break;

    default:
        return 0x80000102;
    }
    // 306C
    lomask = 0;
    if (arg1 != 0) {
        // 30A0
        u32 ver = (_sceSysconGetBaryonVersion() >> 16) & 0xF0;
        lomask = 16;
        if (ver != 0 && ver != 0x10)
            lomask = 1;
    }
    // 3074
    return _sceSysconCommonWrite(himask | lomask, 0x47, 3);
}

// 30F0
s32 sceSysconCtrlDvePower(s8 power)
{
    dbg_printf("Run %s\n", __FUNCTION__);
    u32 type = _sceSysconGetPommelType();
    if (type < 0x300)
        return 0x80000004;
    if (type >= 0x301) {
        // 3140
        s32 ret = _sceSysconCommonWrite(power, 0x52, 3);
        if (ret >= 0)
            g_4EB0.dvePower = power;
        return ret;
    }
    return 0;
}

// 315C
s32 sceSyscon_driver_765775EB(s32 arg0)
{   
    dbg_printf("Run %s\n", __FUNCTION__);
    if (((g_4EB0.baryonVersion >> 16) & 0xFF) >= 0x30 &&
      ((g_4EB0.baryonVersion >> 16) & 0xFF) < 0x32) {
        // 3204
        return sub_406C(arg0);
    }              
    if (arg0 == 0) {
        // 31F4
        if (g_4EB0.unk376 == 0)
            return 0;
    } else if (g_4EB0.unk376 != 0 && sceSysconGetBtPowerStatus() != 0)
        return 0x80250005;
    // (319C)
    // 31A0
    s32 ret = _sceSysconCommonWrite((arg0 & 1) | 0x80, 0x53, 3);
    if (ret >= 0)
        g_4EB0.unk376 = arg0;
    return ret;
}

// 3214
s32 sceSysconCtrlCharge(u8 allow)
{   
    dbg_printf("Run %s\n", __FUNCTION__);
    u8 version = (_sceSysconGetBaryonVersion() >> 16) & 0xF0;
    if (version != 0 && version != 0x10) {
        // 3280
        return _sceSysconCommonWrite(allow, 0x56, 3);
    }
    if (!allow) {          
        // 3270            
        return sub_3360();
    }                  
    return sub_32D8();
}

// 3290
s32 sceSysconCtrlTachyonAvcPower(s8 power)
{
    dbg_printf("Run %s\n", __FUNCTION__);
    s32 ret = sceSysconCtrlPower((_sceSysconGetPommelType() >= 0x300) ? 2 : 8, power);
    if (ret >= 0)
        g_4EB0.tachyonAvcPower = power;
    return ret;
}

// 32D8
s32 sub_32D8(void)
{   
    dbg_printf("Run %s\n", __FUNCTION__);
    SceSysconPacket packet;
    u32 flag;
    packet.tx[0] = 0x4F;
    packet.tx[2] = 2;
    packet.tx[1] = 3;
    s32 ret = sceSysconCmdExec(&packet, 0);
    if (ret >= 0) {
        flag = 0;
        ret = 0;
        memcpy(&flag, &packet.rx[3], 2);
    }
    // 332C
    if (ret >= 0) {
        flag &= 0xFDFF;         
        ret = _sceSysconCommonWrite((flag << 8) | 2, 0x4E, 5);
    }
    return ret;
}

// 3360
s32 sub_3360(void)
{   
    dbg_printf("Run %s\n", __FUNCTION__);
    u32 flag;
    SceSysconPacket packet;
    packet.tx[0] = 0x4F;
    packet.tx[2] = 2;
    packet.tx[1] = 3;
    s32 ret = sceSysconCmdExec(&packet, 0);
    if (ret >= 0) {
        flag = 0;
        ret = 0;
        memcpy(&flag, &packet.rx[3], 2);
    }
    // 33B4       
    if (ret >= 0) {
        flag |= 0x200;                
        ret = _sceSysconCommonWrite(((flag & 0xFFFF) << 8) | 2, 0x4E, 5);
    }
    return ret;
}

// 33E8
s32 sceSysconGetPommelVersion(s32 *pommel)
{
    dbg_printf("Run %s\n", __FUNCTION__);
    return _sceSysconCommonRead(pommel, 0x40);
}

// 3404
s32 sceSyscon_driver_FB148FB6(void *arg0)
{
    dbg_printf("Run %s\n", __FUNCTION__);
    return _sceSysconCommonRead(arg0, 0x41);
}

// 3420
s32 sceSysconCtrlVoltage(s32 arg0, s32 arg1)
{   
    dbg_printf("Run %s\n", __FUNCTION__);
    if (_sceSysconGetPommelType() != 0x100 && arg0 >= 4 && arg0 < 6)
        return 0x80000004;
    // 3470
    return _sceSysconCommonWrite(((arg1 & 0xFFFF) << 8) | (arg0 & 0xFF), 0x42, 5);
}

// 3494
s32 sceSysconGetGSensorVersion(void)
{
    dbg_printf("Run %s\n", __FUNCTION__);
    return 0x80000004;
}

// 34A0
s32 sceSysconCtrlGSensor(void)
{
    dbg_printf("Run %s\n", __FUNCTION__);
    return 0x80000004;
}

// 34AC
s32 sceSysconGetPowerStatus(s32 *status)
{
    dbg_printf("Run %s\n", __FUNCTION__);
    return _sceSysconCommonRead(status, 0x46);
}

// 34C8
s32 sceSysconWritePommelReg(u32 arg0, u32 arg1)
{
    dbg_printf("Run %s\n", __FUNCTION__);
    return _sceSysconCommonWrite(((arg1 & 0xFFFF) << 8) | (arg0 & 0xFF), 0x48, 5);
}

// 34FC
s32 sceSysconReadPommelReg(u8 reg, s32 *value)
{
    dbg_printf("Run %s\n", __FUNCTION__);
    SceSysconPacket packet;
    packet.tx[0] = 0x49;
    packet.tx[1] = 3;
    packet.tx[2] = reg;
    s32 ret = sceSysconCmdExec(&packet, 0);
    if (ret < 0)
        return ret;
    *value = 0;
    memcpy(value, &packet.rx[3], 2);
    return 0;
}

// 3564
s32 sceSysconGetPowerError(s32 *error)
{
    dbg_printf("Run %s\n", __FUNCTION__);
    if (error != NULL)
        *error = 0;
    return 0;
}

// 3574
s32 sceSysconCtrlLeptonPower(s8 power)
{
    dbg_printf("Run %s\n", __FUNCTION__);
    s32 ret = _sceSysconCommonWrite(power, 0x4B, 3);
    if (ret >= 0)
        g_4EB0.leptonPower = power;
    return ret;
}

// 35AC
s32 sceSysconCtrlMsPower(s8 power)
{
    dbg_printf("Run %s\n", __FUNCTION__);
    s32 ret = _sceSysconCommonWrite(power, 0x4C, 3);
    if (ret >= 0)
        g_4EB0.msPower = power;
    return ret;
}

// 35E4
s32 sceSysconCtrlWlanPower(s8 power)
{
    dbg_printf("Run %s\n", __FUNCTION__);
    s32 ret = _sceSysconCommonWrite(power, 0x4D, 3);
    if (ret >= 0) {
        g_4EB0.wlanPower = power;
        if (power != 0 && sceSysconGetWlanLedCtrl())
            sceSysconCtrlLED(1, 1);
    }
    return ret;
}

// 3658
s32 sceSysconCtrlHddPower(s8 power)
{
    dbg_printf("Run %s\n", __FUNCTION__);
    if (_sceSysconGetPommelType() < 0x500)
        return 0x80000004;
    s32 ret = _sceSysconCommonWrite(power, 0x4A, 3);
    if (ret >= 0)
        g_4EB0.hddPower = power;
    return ret;
}

// 36B4
s32 sceSysconCtrlBtPower(s8 power)
{
    dbg_printf("Run %s\n", __FUNCTION__);
    if (g_4EB0.unk376 != 0)
        return 0;
    s32 ret = _sceSysconCommonWrite(power & 1, 0x53, 3);
    if (ret >= 0)
        g_4EB0.btPower = power;
    return ret;
}

// 3714
s32 sceSysconCtrlUsbPower(s8 power)
{   
    dbg_printf("Run %s\n", __FUNCTION__);
    u8 version = _sceSysconGetBaryonVersion() >> 16;
    if ((version & 0xF0) == 0 || (version & 0xF0) == 0x10 || ((version & 0xFF) >= 0x20 && (version & 0xFF) < 0x22))
        return 0x80000004;
    s32 ret = _sceSysconCommonWrite(power, 0x55, 3);
    if (ret >= 0)
        g_4EB0.usbPower = power;
    return ret;
}

// 379C
s32 sceSysconPermitChargeBattery(void)
{
    dbg_printf("Run %s\n", __FUNCTION__);
    return sceSysconCtrlCharge(1);
}

// 37B8
s32 sceSysconForbidChargeBattery(void)
{
    dbg_printf("Run %s\n", __FUNCTION__);
    return sceSysconCtrlCharge(0);
}

// 37D4
s32 sceSysconCtrlTachyonVmePower(s8 power)
{
    dbg_printf("Run %s\n", __FUNCTION__);
    if (_sceSysconGetPommelType() != 0x100)
        return 0x80000004;
    s32 ret = sceSysconCtrlPower(2, power);
    if (ret >= 0)
        g_4EB0.tachyonVmePower = power;
    return ret;
}

// 3830
s32 sceSysconCtrlTachyonAwPower(s8 power)
{
    dbg_printf("Run %s\n", __FUNCTION__);
    if (_sceSysconGetPommelType() != 0x100)
        return 0x80000004;
    s32 ret = sceSysconCtrlPower(4, power);
    if (ret >= 0)
        g_4EB0.tachyonAwPower = power;
    return ret;
}

// 388C
s32 sceSysconCtrlLcdPower(s8 power)
{
    dbg_printf("Run %s\n", __FUNCTION__);
    if (_sceSysconGetPommelType() >= 0x300)
        return 0x80000004;
    s32 ret = sceSysconCtrlPower(0x80000, power);
    if (ret >= 0)
        g_4EB0.lcdPower = power;
    return ret;
}

// 38E4
s32 sceSysconGetGSensorCarib(void)
{
    dbg_printf("Run %s\n", __FUNCTION__);
    return 0x80000004;
}

// 38F0
s32 sceSysconSetGSensorCarib(void)
{
    dbg_printf("Run %s\n", __FUNCTION__);
    return 0x80000004;
}

// 38FC
s32 sceSysconWritePolestarReg(s8 reg, u32 val)
{
    dbg_printf("Run %s\n", __FUNCTION__);
    return _sceSysconCommonWrite(((val & 0xFFFF) << 8) | (reg & 0xFF), 0x4E, 5);
}

// 3930
s32 sceSysconReadPolestarReg(s8 reg, s32 *val)
{
    dbg_printf("Run %s\n", __FUNCTION__);
    SceSysconPacket packet;
    packet.tx[0] = 0x4F;
    packet.tx[1] = 3;
    packet.tx[2] = reg;
    s32 ret = sceSysconCmdExec(&packet, 0);
    if (ret < 0)
        return ret;
    *val = 0;
    memcpy(val, &packet.rx[3], 2);
    return 0;
}

// 3998
s32 sceSysconWriteGSensorReg(void)
{
    dbg_printf("Run %s\n", __FUNCTION__);
    return 0x80000004;
}

// 39A4
s32 sceSysconReadGSensorReg(void)
{
    dbg_printf("Run %s\n", __FUNCTION__);
    return 0x80000004;
}

// 39B0
s32 sceSysconBatteryGetStatusCap(s32 *arg0, s32 *arg1)
{   
    dbg_printf("Run %s\n", __FUNCTION__);
    SceSysconPacket packet;
    packet.tx[0] = 0x61;
    packet.tx[1] = 2;             
    s32 ret = sceSysconCmdExec(&packet, 0);
    if (ret < 0)
        return ret;
    if (arg0 != NULL)
        *arg0 = packet.rx[3];
    // 39F8             
    if (arg1 != NULL)
        *arg1 = (packet.tx[5] << 8) | packet.tx[4];
    // 3A10 
    return 0;
}

// 3A2C
s32 sceSysconBatteryGetInfo(s32 *info)
{
    dbg_printf("Run %s\n", __FUNCTION__);
    SceSysconPacket packet;
    packet.tx[0] = 0x6D;
    packet.tx[1] = 2;
    s32 ret = sceSysconCmdExec(&packet, 0);
    if (ret < 0)
        return ret;
    if (info != NULL) {
        info[3] = packet.rx[7];
        info[0] = (packet.rx[4] << 8) | packet.rx[3];
        info[1] = packet.rx[5];
        info[2] = packet.rx[6];
    }
    return 0;
}

// 3AA8
s32 sceSysconGetBattVolt(s32 *volt)
{   
    dbg_printf("Run %s\n", __FUNCTION__);
    u8 version =_sceSysconGetBaryonVersion() >> 16;
    if ((version & 0xF0) != 0x29 &&
      (version & 0xFF) != 0x2A &&
      (version & 0xF0) != 0x30 &&
      (version & 0xF0) != 0x40)
        return 0x80000004;
    // 3AF8
    s32 val;
    s32 ret = _sceSysconCommonRead(&val, 0xD);
    if (ret < 0)
        return ret;
    *volt = val * 50;
    return 0;
}

// 3B34
s32 sceSysconGetBattVoltAD(s32 *volt1, s32 *volt2)
{
    dbg_printf("Run %s\n", __FUNCTION__);
    u8 version = _sceSysconGetBaryonVersion() >> 16;
    if ((version & 0xF0) != 0x29 &&
      (version & 0xFF) != 0x2A &&
      (version & 0xF0) != 0x30 &&
      (version & 0xF0) != 0x40)
        return 0x80000004;
    // 3B84
    s32 val;
    s32 ret = _sceSysconCommonRead(&val, 0x37);
    if (ret < 0)
        return ret;
    if (volt1 != NULL)
        *volt1 = ((val >> 16) & 0xFF) * 50;
    // 3BB0
    if (volt2 != NULL)
        *volt2 = val & 0xFFFF;
    return 0;
}

// 3BD8
s32 sceSysconBatteryNop(void)
{
    dbg_printf("Run %s\n", __FUNCTION__);
    SceSysconPacket packet;
    packet.tx[0] = 0x60;
    packet.tx[1] = 2;
    return pspMin(sceSysconCmdExec(&packet, 0), 0);
}

// 3C10
s32 sceSysconBatteryGetTemp(s32 *temp)
{
    dbg_printf("Run %s\n", __FUNCTION__);
    return _sceSysconBatteryCommon(0x62, temp);
}

// 3C30
s32 sceSysconBatteryGetVolt(s32 *volt)
{
    dbg_printf("Run %s\n", __FUNCTION__);
    return _sceSysconBatteryCommon(0x63, volt);
}

// 3C50
s32 sceSysconBatteryGetElec(s32 *elec)
{
    dbg_printf("Run %s\n", __FUNCTION__);
    return _sceSysconBatteryCommon(0x64, elec);
}

// 3C70
s32 sceSysconBatteryGetRCap(s32 *rcap)
{
    dbg_printf("Run %s\n", __FUNCTION__);
    return _sceSysconBatteryCommon(0x65, rcap);
}

// 3C90
s32 sceSysconBatteryGetCap(s32 *cap)
{
    dbg_printf("Run %s\n", __FUNCTION__);
    return _sceSysconBatteryCommon(0x66, cap);
}

// 3CB0
s32 sceSysconBatteryGetFullCap(s32 *cap)
{
    dbg_printf("Run %s\n", __FUNCTION__);
    return _sceSysconBatteryCommon(0x67, cap);
}

// 3CD0
s32 sceSysconBatteryGetIFC(s32 *ifc)
{
    dbg_printf("Run %s\n", __FUNCTION__);
    return _sceSysconBatteryCommon(0x68, ifc);
}

// 3CF0
s32 sceSysconBatteryGetLimitTime(s32 *time)
{
    dbg_printf("Run %s\n", __FUNCTION__);
    return _sceSysconBatteryCommon(0x69, time);
}

// 3D10
s32 sceSysconBatteryGetStatus(s32 *status)
{
    dbg_printf("Run %s\n", __FUNCTION__);
    s32 stat;
    s32 ret = _sceSysconBatteryCommon(0x6A, &stat);
    if (ret < 0)
        return ret;
    if (status != NULL)
        *status = stat;
    return 0;
}

// 3D58
s32 sceSysconBatteryGetCycle(s32 *cycle)
{
    dbg_printf("Run %s\n", __FUNCTION__);
    return _sceSysconBatteryCommon(0x6B, cycle);
}

// 3D78
s32 sceSysconBatteryGetSerial(s32 *serial)
{
    dbg_printf("Run %s\n", __FUNCTION__);
    return _sceSysconBatteryCommon(0x6C, serial);
}

// 3D98
s32 sceSysconBatteryGetTempAD(s32 *temp)
{
    dbg_printf("Run %s\n", __FUNCTION__);
    return _sceSysconBatteryCommon(0x6E, temp);
}

// 3DB8
s32 sceSysconBatteryGetVoltAD(s32 *volt)
{
    dbg_printf("Run %s\n", __FUNCTION__);
    return _sceSysconBatteryCommon(0x6F, volt);
}

// 3DD8
s32 sceSysconBatteryGetElecAD(s32 *elec)
{
    dbg_printf("Run %s\n", __FUNCTION__);
    return _sceSysconBatteryCommon(0x70, elec);
}

// 3DF8
s32 sceSysconBatteryGetTotalElec(s32 *elec)
{
    dbg_printf("Run %s\n", __FUNCTION__);
    return _sceSysconBatteryCommon(0x71, elec);
}

// 3E18
s32 sceSysconBatteryGetChargeTime(s32 *time)
{
    dbg_printf("Run %s\n", __FUNCTION__);
    return _sceSysconBatteryCommon(0x72, time);
}

// 3E38
s32 _sceSysconBatteryCommon(u32 cmd, s32 *ptr)
{
    dbg_printf("Run %s\n", __FUNCTION__);
    u32 version = _sceSysconGetBaryonVersion() >> 16;
    if ((version & 0xF0) == 0x29 ||
     (version & 0xFF) == 0x2A ||
     (version & 0xF0) == 0x30 ||
     (version & 0xF0) == 0x40)
        return 0x80000004;
    SceSysconPacket packet;
    packet.tx[0] = cmd;
    packet.tx[1] = 2;
    s32 val;
    s32 ret = sceSysconCmdExec(&packet, 0);
    if (ret < 0)
        return ret;
    if (packet.rx[1] == 4) {
        // 3F54
        val = packet.rx[3];
    } else if (packet.rx[1] == 5) {
        // 3F3C
        val = (s16)((packet.rx[4] << 8) | packet.rx[3]);
    } else if (packet.rx[1] == 6) {
        // 3F18
        val = (((packet.rx[5] << 24) | (packet.rx[4] << 16)) >> 8) | packet.rx[3];
    } else if (packet.rx[1] == 7)
        val = (packet.rx[6] << 24) | (packet.rx[5] << 16) | (packet.rx[4] << 8) | packet.rx[3];
    else
        return 0x80250001;
    // 3EF8
    *ptr = val;
    return 0;
}

// 3F68
s32 sceSysconCtrlTachyonVoltage(s32 voltage)
{   
    dbg_printf("Run %s\n", __FUNCTION__);
    s32 status;
    s32 ret = sceSysconGetPowerStatus(&status);
    if (ret < 0)
        return ret;
    s32 mask = 0;
    u8 ver = (_sceSysconGetBaryonVersion() >> 16) & 0xF0;
    if (ver != 0 && ver != 0x10)
    {  
        // 4058
        if ((status & 2) == 0)
            mask = 2;
    } else {
        mask = 8;
        if ((status & 8) != 0)
            mask = 0;
        if (_sceSysconGetPommelType() == 0x100) {
            // 403C
            if ((status & 4) == 0)
                mask |= 4;
            if ((status & 2) == 0)
                mask |= 2;
        }  
    }              
    // 3FD8        
    if (mask != 0) {             
        // 4024                  
        ret = sceSysconCtrlPower(mask, 1);
        if (ret < 0)
            return ret;
    }   
    // (3FE0)
    // 3FE4                   
    ret = sceSysconCtrlVoltage(1, voltage);
    if (mask != 0) {       
        // 4010            
        sceSysconCtrlPower(mask, 0);
    }
    return ret;
}

// 406C
s32 sub_406C(s32 arg0)
{
    dbg_printf("Run %s\n", __FUNCTION__);
    if (arg0 == 0) {
        // 4134
        if (g_4EB0.unk376 == 0)
            return 0;
    } else {
        if (g_4EB0.unk376 != 0 &&
          sceSysconGetBtPowerStatus() != 0)
            return 0x80250005;
        // 40B4
        s32 ret = sub_4150(0, 150, 150);
        if (ret < 0)
            return ret;
    }
    // 40CC
    SceSysconPacket packet;
    packet.tx[0] = 0x2A;
    packet.tx[1] = 4;
    packet.tx[2] = 1;
    packet.tx[3] = arg0;
    s32 ret = sceSysconCmdExec(&packet, 0);
    if (ret >= 0) {
        g_4EB0.unk376 = arg0;
        if (arg0 != 0)
            return ret;
    }
    // 4104
    sub_4150(0, 0, 0);
    return ret;
}

// 4150
s32 sub_4150(s32 arg0, s32 arg1, s32 arg2)
{
    dbg_printf("Run %s\n", __FUNCTION__);
    SceSysconPacket packet;
    packet.tx[0] = 0x29;
    packet.tx[1] = 7;
    packet.tx[2] = 21;
    packet.tx[3] = 1;
    packet.tx[4] = arg0;
    packet.tx[5] = arg1;
    packet.tx[6] = arg2;
    s32 ret = sceSysconCmdExec(&packet, 0);
    if (ret < 0)
        return ret;
    if (packet.rx[1] != 3)
        return 0x80000104;
    return 0;
}

// 41C4
s32 _sceSysconGetPommelType(void)
{
    dbg_printf("Run %s\n", __FUNCTION__);
    u32 baryon = _sceSysconGetBaryonVersion();
    u8 flag = (u32)baryon >> 16;
    if ((flag & 0xF0) == 0) {
        // 41F4 dup
        if (((baryon >> 8) & 0xFF) != 1)
            return 0x100;
        return 0;
    }
    // 4208
    if ((flag & 0xF0) == 0x10)
        return 512;
    if (((flag & 0xFF) >= 0x20 && (flag & 0xFF) < 0x22) ||
      (((((flag & 0xF0) == 0x20 && (flag & 0xFF) >= 0x22) || (flag & 0xF0) == 0x30 || (flag & 0xF0) == 0x40) &&
      ((flag & 0xF0) == 0x20 && (flag & 0xFF) >= 0x22 && (flag & 0xFF) < 0x26)) ||
      ((flag & 0xFF) == 0x29))) {
        // 442C
        // 41F4 dup
        if (_sceSysconGetPommelVersion() != 0x120)
            return 0x301;
        return 0x300;
    }
    // 4244
    if (((flag & 0xF0) == 0x20 && (flag & 0xFF) >= 0x22) ||
      (flag & 0xF0) == 0x30 ||
      (flag & 0xF0) == 0x40) {
        if (((flag & 0xF0) == 0x20 && (flag & 0xFF) >= 0x26 && (flag & 0xFF) < 0x29) ||
          ((flag & 0xFF) >= 0x30 && (flag & 0xFF) < 0x32))
            return 0x400;
    }
    // 4264
    if (((flag & 0xF0) == 0x20 && (flag & 0xFF) >= 0x22) ||
      (flag & 0xF0) == 0x30 ||
      (flag & 0xF0) == 0x40) {
        if (((flag & 0xF0) == 0x20 && (flag & 0xFF) >= 0x2C && (flag & 0xFF) < 0x2E) ||
          ((flag & 0xFF) >= 0x32 && (flag & 0xFF) < 0x40))
            return 0x500;
    }
    // 4284
    if (((flag & 0xF0) == 0x20 && (flag & 0xFF) >= 0x22) ||
      (flag & 0xF0) == 0x30 ||
      (flag & 0xF0) == 0x40) {
        if ((flag & 0xF0) == 0x20 && (flag & 0xFF) >= 0x2E && (flag & 0xFF) < 0x30)
            return 0x600;
    }
    // (42A0)
    // 42A4
    if (((flag & 0xF0) == 0x20 && (flag & 0xFF) >= 0x22) ||
      (flag & 0xF0) == 0x30 ||
      (flag & 0xF0) == 0x40) {
        // (42C8)
        // 42Cc
        if ((flag & 0xF0) == 0x40)
            return 0x600;
    }
    return -1;
}

// 4474
s32 sceSysconGetDigitalKey(s8 *key)
{
    dbg_printf("Run %s\n", __FUNCTION__);
    SceSysconPacket packet;
    packet.tx[0] = 2;
    packet.tx[1] = 2;
    s32 ret = sceSysconCmdExec(&packet, 0);
    if (ret < 0)
        return ret;
    key[0] = packet.rx[3];
    key[1] = packet.rx[4];
    return 0;
}

