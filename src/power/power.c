/* Copyright (C) The uOFW team
   See the file COPYING for copying permission.
*/

#include <common_imp.h>
#include <interruptman.h>
#include <modulemgr_init.h>
#include <power_kernel.h>
#include <syscon.h>
#include <sysmem_kdebug.h>
#include <sysmem_kernel.h>
#include <sysmem_suspend_kernel.h>
#include <sysmem_sysevent.h>
#include <sysmem_sysclib.h>
#include <threadman_kernel.h>

#include "power_int.h"

SCE_MODULE_INFO(
    "scePower_Service", 
    SCE_MODULE_KERNEL | 
    SCE_MODULE_ATTR_CANT_STOP | SCE_MODULE_ATTR_EXCLUSIVE_LOAD | SCE_MODULE_ATTR_EXCLUSIVE_START, 
    1, 13);
SCE_MODULE_BOOTSTART("_scePowerModuleStart");
SCE_MODULE_REBOOT_BEFORE("_scePowerModuleRebootBefore");
SCE_MODULE_REBOOT_PHASE("_scePowerModuleRebootPhase");
SCE_SDK_VERSION(SDK_VERSION);

#define BARYON_DATA_REALLY_LOW_BATTERY_CAP_SLOT         (26)
#define BARYON_DATA_LOW_BATTERY_CAP_SLOT                (28)

#define POWER_CALLBACK_TOTAL_SLOTS_KERNEL               (32)
#define POWER_CALLBACK_MAX_SLOT_KERNEL                  (POWER_CALLBACK_TOTAL_SLOTS_KERNEL - 1)
#define POWER_CALLBACK_TOTAL_SLOTS_USER                 (16)
#define POWER_CALLBACK_MAX_SLOT_USER                    (POWER_CALLBACK_TOTAL_SLOTS_USER - 1)

#define BATTERY_LOW_CAPACITY_VALUE                      (216)
#define BATTERY_REALLY_LOW_CAPACITY_VALUE               (72)

#define SCE_POWER_MAX_BACKLIGHT_LEVEL_POWER_INTERNAL    (3)
#define SCE_POWER_MAX_BACKLIGHT_LEVEL_POWER_EXTERNAL    (4)

/** Cancel all PSP Hardware timers. */
#define SCE_KERNEL_POWER_TICK_DEFAULT               (0)     
/** Cancel auto-suspend-related timer. */
#define SCE_KERNEL_POWER_TICK_SUSPENDONLY           (1)
/** Cancel LCD-related timer */
#define SCE_KERNEL_POWER_TICK_LCDONLY               (6)	

typedef struct 
{
    SceUID cbid; // 0
    s32 customPowerState; // 4
    s32 powerState; //8
    s32 mode; // 12 TODO: Perhaps execution mode, synchron = 1, otherwise asynchron? More data needed...
} ScePowerCallback;

typedef struct 
{
    ScePowerCallback powerCallback[POWER_CALLBACK_TOTAL_SLOTS_KERNEL]; // 0 - 511
    s32 baryonVersion; //512
    u32 curPowerStateForCallback; // 516
    u32 callbackArgMask; //520
    u8 isBatteryLow; //524
    u8 wlanActivity; //525 -- TODO: Perhaps rename to isWlanActivated?
    u8 watchDog; //526
    u8 isWlanSuppressChargingEnabled; // 527
    s8 unk528;
    s8 wlanExclusivePllClockLimit; // 529
    u8 ledOffTiming;
    s16 cpuInitSpeed; //534 -- CPU clock init speed
    s16 busInitSpeed; //536 -- Bus clock init speed
    s16 pllInitSpeed; //538 -- PLL clock init speed (PLL = phase-locked loop ?)
} ScePower;

enum ScePowerWlanActivity 
{
    SCE_POWER_WLAN_DEACTIVATED = 0,
    SCE_POWER_WLAN_ACTIVATED = 1,
};

static void _scePowerAcSupplyCallback(s32 enable, void* argp); //sub_0x00000650
static void _scePowerLowBatteryCallback(s32 enable, void* argp); //sub_0x000006C4
static s32 _scePowerSysEventHandler(s32 eventId, char *eventName, void *param, s32 *result); //sub_0x0000071C
static s32 _scePowerInitCallback(); //sub_0x0000114C

const SceSysEventHandler g_PowerSysEv = 
{
    .size = sizeof(SceSysEventHandler),
    .name = "ScePower",
    .typeMask = SCE_SUSPEND_EVENTS | SCE_RESUME_EVENTS,
    .handler = _scePowerSysEventHandler,
    .gp = 0,
    .busy = SCE_FALSE,
    .next = NULL,
    .reserved = {
        [0] = 0,
        [1] = 0,
        [2] = 0,
        [3] = 0,
        [4] = 0,
        [5] = 0,
        [6] = 0,
        [7] = 0,
        [8] = 0,
    }  
}; // 0x00007040

ScePower g_Power; //0x00007080

//scePower_driver_9CE06934 - Address 0x00000000
// TODO: Verify function
s32 scePowerInit()
{
    u8 baryonData[512];
    u8 powerData[512];
    s32 status;
    s32 appType;
    u16 upperBaryonVer;
    u8 tachyonVoltage1;
    u16 batteryLowCap;
    u16 batteryReallyLowCap;
    u8 tachyonVoltage2;
    s8 ddrVoltage1;
    s8 ddrVoltage2;
    s8 ddrStrength1;
    s8 ddrStrength2;
    u8 fp;         
    s16 minCpuSpeed;
    s16 maxCpuSpeed;
    s16 minBusSpeed;
    s16 maxBusSpeed;
    s16 minPllSpeed;
    s16 maxPllSpeed;
    
    fp = 0;
    
    _scePowerSwInit();
    _scePowerFreqInit();
    
    g_Power.baryonVersion = _sceSysconGetBaryonVersion(); //0x00000040
    status = sceIdStorageLookup(4, 0, baryonData, sizeof baryonData); //0x00000058
    if (status < SCE_ERROR_OK) { //0x00000060
        memset(baryonData, 0, sizeof baryonData); //0x00000618
        g_Power.wlanExclusivePllClockLimit = SCE_POWER_WLAN_EXCLUSIVE_PLL_CLOCK_LIMIT_222Mhz;
        g_Power.watchDog = 0;
        g_Power.isWlanSuppressChargingEnabled = SCE_FALSE;
        g_Power.unk528 = 0;
        g_Power.ledOffTiming = 0;
        batteryReallyLowCap = ((u16 *)baryonData)[BARYON_DATA_REALLY_LOW_BATTERY_CAP_SLOT]; //0x0000062C
        batteryLowCap = ((u16 *)baryonData)[BARYON_DATA_LOW_BATTERY_CAP_SLOT]; //0x00000634        
        _scePowerChangeSuspendCap(216); //0x00000640
    }
    else {
        g_Power.ledOffTiming = baryonData[31]; //0x00000080
        g_Power.watchDog = baryonData[24] & 0x7F; //0x00000088
        g_Power.isWlanSuppressChargingEnabled = baryonData[25]; //0x0000008C
        g_Power.unk528 = baryonData[30]; //0x00000090
        g_Power.wlanExclusivePllClockLimit = ((s8)baryonData[52] < 0) ? baryonData[52] & 0x7F : SCE_POWER_WLAN_EXCLUSIVE_PLL_CLOCK_LIMIT_222Mhz; //0x00000098       
        batteryReallyLowCap = BATTERY_REALLY_LOW_CAPACITY_VALUE; //0x00000094
        batteryLowCap = BATTERY_LOW_CAPACITY_VALUE; //0x0000009C
    }
    appType = sceKernelApplicationType(); //0x000000A8
    if (appType == SCE_INIT_APPLICATION_GAME) { //0x000000B4
        g_Power.wlanExclusivePllClockLimit = SCE_POWER_WLAN_EXCLUSIVE_PLL_CLOCK_LIMIT_222Mhz;
        scePowerSetPllUseMask(60); //0x00000600
    }
    
    upperBaryonVer = g_Power.baryonVersion >> 16;
    if ((upperBaryonVer & 0xF0) == 0x20 && (upperBaryonVer & 0xFF) >= 0x22 && (upperBaryonVer & 0xFF) < 0x26) { //0x000000CC & 0x000005CC & 0x000005D8
        fp = 1;
        if (baryonData[53] & 0x2) { //0x000005E8
            tachyonVoltage1 = -1; //0x00000160
            tachyonVoltage2 = -1; //0x0000016C
            
            if ((s8)baryonData[32] < 0) //0x00000168
                tachyonVoltage1 = (baryonData[32] & 0x7F) << 8; //0x0000051C
            
            if ((s8)baryonData[33] < 0) //0x00000178
                tachyonVoltage2 = (baryonData[33] & 0x7F) << 8; //0x00000510
        }           
    }
    else if ((upperBaryonVer & 0xF0) == 0x20 && (upperBaryonVer & 0xFF) >= 0x26 && (upperBaryonVer & 0xFF) < 0x29) { //0x000000E4 & 0x0000059C & 0x000005A8
        fp = 1; //0x000005BC
        if (baryonData[53] & 0x2) { //0x000005B8
            tachyonVoltage1 = -1; //0x00000160
            tachyonVoltage2 = -1; //0x0000016C
            
            if ((s8)baryonData[32] < 0) //0x00000168
                tachyonVoltage1 = (baryonData[32] & 0x7F) << 8; //0x0000051C
            
            if ((s8)baryonData[33] < 0) //0x00000178
                tachyonVoltage2 = (baryonData[33] & 0x7F) << 8; //0x00000510
        }
    }
    else if ((upperBaryonVer & 0xF0) == 0x20 && (upperBaryonVer & 0xFF) >= 0x2C && (upperBaryonVer & 0xFF) < 0x2E) { //0x000000FC & 0x0000056C & 0x00000578
        fp = 1;
        if (baryonData[53] & 0x2) { //0x00000588
            tachyonVoltage1 = -1; //0x00000160
            tachyonVoltage2 = -1; //0x0000016C
            
            if ((s8)baryonData[32] < 0) //0x00000168
                tachyonVoltage1 = (baryonData[32] & 0x7F) << 8; //0x0000051C
            
            if ((s8)baryonData[33] < 0) //0x00000178
                tachyonVoltage2 = (baryonData[33] & 0x7F) << 8; //0x00000510
        }
    }
    else if ((upperBaryonVer & 0xF0) == 0x20 && (upperBaryonVer & 0xFF) >= 0x2E && (upperBaryonVer & 0xFF) < 0x30) { //0x00000114 & 0x0000053C & 0x00000548
        fp = 1; //0x0000055C
        if (baryonData[53] & 0x2) { //0x00000558
            tachyonVoltage1 = -1; //0x00000160
            tachyonVoltage2 = -1; //0x0000016C
            
            if ((s8)baryonData[32] < 0) //0x00000168
                tachyonVoltage1 = (baryonData[32] & 0x7F) << 8; //0x0000051C
            
            if ((s8)baryonData[33] < 0) //0x00000178
                tachyonVoltage2 = (baryonData[33] & 0x7F) << 8; //0x00000510
        }
    }
    else if (((g_Power.baryonVersion & 0x00FF0000) - 48) < 0x10) { //0x0000012C
        fp = 1; //0x00000140
        if (baryonData[53] & 0x4) { //0x0000013C
            tachyonVoltage1 = -1; //0x00000160
            tachyonVoltage2 = -1; //0x0000016C
            
            if ((s8)baryonData[32] < 0) //0x00000168
                tachyonVoltage1 = (baryonData[32] & 0x7F) << 8; //0x0000051C
            
            if ((s8)baryonData[33] < 0) //0x00000178
                tachyonVoltage2 = (baryonData[33] & 0x7F) << 8; //0x00000510
        }
    }
    else if ((upperBaryonVer & 0xF0) == 0x40) { //0x00000154
        if (baryonData[53] & 0x2) //0x00000528
            fp = 1;
        
        tachyonVoltage1 = -1; //0x00000160
        tachyonVoltage2 = -1; //0x0000016C
            
        if ((s8)baryonData[32] < 0) //0x00000168
            tachyonVoltage1 = (baryonData[32] & 0x7F) << 8; //0x0000051C
            
        if ((s8)baryonData[33] < 0) //0x00000178
            tachyonVoltage2 = (baryonData[33] & 0x7F) << 8; //0x00000510
    }
    else { //0x0000015C
        tachyonVoltage1 = -1; //0x00000160
        tachyonVoltage2 = -1; //0x0000016C
            
        if ((s8)baryonData[32] < 0) //0x00000168
            tachyonVoltage1 = (baryonData[32] & 0x7F) << 8; //0x0000051C
            
        if ((s8)baryonData[33] < 0) //0x00000178
            tachyonVoltage2 = (baryonData[33] & 0x7F) << 8; //0x00000510
    }
    scePowerSetTachyonVoltage(tachyonVoltage1, tachyonVoltage2); //0x00000180 -- scePower_driver_12F8302D
    
    minCpuSpeed = (((s16 *)baryonData)[34] == 0) ? -1 : ((s16 *)baryonData)[34]; //0x000001A0 -- CPU power limit
    maxCpuSpeed = (((s16 *)baryonData)[36] == 0) ? -1 : ((s16 *)baryonData)[36]; //0x000001A4
    scePowerLimitScCpuClock(minCpuSpeed, maxCpuSpeed); //0x000001BC -- scePower_driver_DF904CDE
    
    minBusSpeed = (((s16 *)baryonData)[40] == 0) ? -1 : ((s16 *)baryonData)[40]; //0x000001A8 -- Bus clock limit
    maxBusSpeed = (((s16 *)baryonData)[42] == 0) ? -1 : ((s16 *)baryonData)[42]; //0x000001AC
    scePowerLimitScBusClock(minBusSpeed, maxBusSpeed); //0x000001C8 -- scePower_driver_EEFB2ACF
    
    minPllSpeed = (((s16 *)baryonData)[46] == 0) ? -1 : ((s16 *)baryonData)[46]; //0x000001B8 -- PLL clock limit
    maxPllSpeed = (((s16 *)baryonData)[48] == 0) ? -1 : ((s16 *)baryonData)[48]; //0x000001C0
    scePowerLimitPllClock(minPllSpeed, maxPllSpeed); //0x000001D4 -- scePower_driver_B7000C75
    
    g_Power.busInitSpeed = 111; //0x000001EC
    g_Power.pllInitSpeed = 222; 
    g_Power.cpuInitSpeed = (((u16 *)baryonData)[38] == 0) ? 222 : ((u16 *)baryonData)[38]; //0x000001F4
    g_Power.busInitSpeed = (((u16 *)baryonData)[44] == 0) ? g_Power.busInitSpeed : ((u16 *)baryonData)[44]; //0x00000204
    g_Power.pllInitSpeed = (((u16 *)baryonData)[50] == 0) ? g_Power.pllInitSpeed : ((u16 *)baryonData)[50]; //0x00000218
    
    if (minPllSpeed <= 266 && g_Power.pllInitSpeed < 267) { //0x0000021C & 0x0000022C
        if (minPllSpeed > 222 || g_Power.pllInitSpeed > 222) //0x00000230 & 0x000004F0
            g_Power.wlanExclusivePllClockLimit = SCE_POWER_WLAN_EXCLUSIVE_PLL_CLOCK_LIMIT_222Mhz; //0x00000508
    }
    else {
        g_Power.wlanExclusivePllClockLimit = SCE_POWER_WLAN_EXCLUSIVE_PLL_CLOCK_LIMIT_NONE; //0x00000220 || 0x00000234
    }   
    status = sceKernelDipsw(49); //0x00000238
    if (status == 1) { //0x00000244
        g_Power.busInitSpeed = 166;
        g_Power.pllInitSpeed = 333; //0x000004C0
        g_Power.wlanExclusivePllClockLimit = SCE_POWER_WLAN_EXCLUSIVE_PLL_CLOCK_LIMIT_NONE;
        g_Power.cpuInitSpeed = 333;
        scePowerLimitScCpuClock(minCpuSpeed, 333);
        scePowerLimitScBusClock(minBusSpeed, 166); //0x000004D4
        scePowerLimitPllClock(minPllSpeed, 333); //scePower_driver_B7000C75
    }
    status = sceIdStorageLookup(0x6, 0, powerData, sizeof powerData); //0x0000025C
    ddrVoltage1 = -1; //0x00000264
    ddrVoltage2 = -1; //0x00000268
    ddrStrength1 = -1; //0x0000026C
    ddrStrength2 = -1; //0x00000274
            
    if (status < SCE_ERROR_OK) //0x00000270
        memset(powerData, 0, sizeof powerData);
        
    upperBaryonVer = g_Power.baryonVersion >> 16; //0x0000027C
    if ((upperBaryonVer & 0xF0) == 0x10) { //0x00000288
        if ((s8)powerData[20] < 0) //0x00000444
            ddrVoltage1 = (powerData[20] & 0x7F) << 8; //0x00000490
        
        if ((s8)powerData[19] < 0) //0x00000454
            ddrVoltage2 = (powerData[19] & 0x7F) << 8; //0x00000488
        
        if ((s8)powerData[22] < 0) //0x00000478
            ddrStrength1 = (s8)powerData[22] & 0x7F;
        
        if ((s8)powerData[21] < 0) //0x00000474
            ddrStrength2 = (s8)powerData[21] & 0x7F; //0x00000480
    }
    else if ((upperBaryonVer & 0xF0) == 0x20 || (upperBaryonVer & 0xF0) < 0x40 || powerData[24] == 0x40) { //0x00000290 & 0x000002A0 & 0x000002AC
        if ((s8)powerData[24] < 0) //0x000003E8
            ddrVoltage1 = (powerData[24] 0x7F) << 8; //0x00000438
        
        if ((s8)powerData[23] < 0) //0x000003F8
            ddrVoltage2 = (powerData[23] 0x7F) << 8; //0x00000430
        
        if ((s8)powerData[26] < 0) //0x00000420
            ddrStrength1 = (s8)powerData[26] & 0x7F;
        
        if ((s8)powerData[25] < 0) //0x00000428
            ddrStrength2 = (s8)powerData[25] & 0x7F;
    }
    scePowerSetDdrVoltage(ddrVoltage1, ddrVoltage2); //0x000002B8 -- scePower_driver_018AB235
    scePowerSetDdrStrength(ddrStrength1, ddrStrength2); //0x000002C8 -- scePower_driver_D13377F7
    
    if ((upperBaryonVer & 0xF0) == 0x29 || (upperBaryonVer & 0xF0) == 0x2A || (upperBaryonVer & 0xF0) == 0x30 || 
     (upperBaryonVer & 0xF0) == 0x40) //0x000002DC & 0x000002E8 & 0x000002F4 & 0x000002FC
        a1 = 1;
    else
        a1 = 0;
    
    _scePowerBatteryInit(fp, a1); //0x00000304 -- sub_00005B1C
    _scePowerBatterySetParam(batteryReallyLowCap, batteryLowCap); //0x00000310 -- sub_00005BF0
    
    g_Power.isBatteryLow = sceSysconIsLowBattery(); //0x00000318
    if (g_Power.isBatteryLow == SCE_TRUE) //0x00000328
        g_Power.curPowerStateForCallback |= SCE_POWER_CALLBACKARG_LOW_BATTERY; //0x000003D4
    
    if (sceSysconIsAcSupplied) //0x00000330
        g_Power.curPowerStateForCallback |= SCE_POWER_CALLBACKARG_POWER_ONLINE; //0x00000348
    
    g_Power.callbackArgMask = -1; //0x00000350
    _scePowerIdleInit();//0x0000034C -- sub_000033C4
    
    sceSysconSetAcSupplyCallback(_scePowerAcSupplyCallback, NULL); //0x0000035C
    sceSysconSetLowBatteryCallback(_scePowerLowBatteryCallback, NULL); //0x0000036C
    sceKernelRegisterSysEventHandler(_scePowerSysEventHandler); //0x00000378
    sceKernelSetInitCallback(_scePowerInitCallback, 2, 0); //0x0000038C
    
    return SCE_ERROR_OK;
}

//0x00000650
// TODO: Verify function
static void _scePowerAcSupplyCallback(s32 enable, void *argp)
{
    u16 type;
    
    if (enable == 0) //0x00000668
        _scePowerNotifyCallback(SCE_POWER_CALLBACKARG_POWER_ONLINE, 0, 0); //0x0000067C -- sub_00000BE0
    else
        _scePowerNotifyCallback(0, SCE_POWER_CALLBACKARG_POWER_ONLINE, 0);
    
    type = g_Power.baryonVersion >> 16;
    if ((type & 0xF0) == 0x20) //0x00000694
        _scePowerBatteryUpdateAcSupply(enable); //0x000006B4 -- sub_0000544C
    
    scePowerBatteryUpdateInfo(); //0x0000069C -- scePower_27F3292C
}

//sub_0x000006C4
// TODO: Verify function
static void _scePowerLowBatteryCallback(s32 enable, void *argp)
{
    if (g_Power.isBatteryLow == enable) //0x000006D8
        return;
    
    g_Power.isBatteryLow = enable; //0x000006F0
    if (enable == 0) //0x000006EC
        _scePowerNotifyCallback(SCE_POWER_CALLBACKARG_LOW_BATTERY, 0, 0); //0x000006F4
    else
        _scePowerNotifyCallback(0, SCE_POWER_CALLBACKARG_LOW_BATTERY, 0);
    
    scePowerBatteryUpdateInfo(); //0x00000708
}

//sub_0x0000071C
// TODO: Verify function
static s32 _scePowerSysEventHandler(s32 eventId, char *eventName, void *param, s32 *result)
{
    u16 type;
    u32 val;
    s32 sdkVer;

    (void)eventName;
    (void)result;
    
    if (eventId == 0x402) { //0x00000730
        if (_scePowerBatteryIsBusy() == SCE_FALSE) //0x00000898 -- sub_00005C08
            return SCE_ERROR_OK;
        
        return SCE_ERROR_BUSY;
    }
    if (eventId >= 0x403) { //0x0000073C
        if (eventId == 0x10009) { //0x00000784
            type = g_Power.baryonVersion >> 16; //0x000007AC
            if ((type & 0xF0) >= 1 && ((type & 0xF0) ^ 0x10) >= 1) //loc_00000880
                val = (*(u32 *)(*(u32 *)(param + 4)) + 9) & 0x10; //0x00000880 & 0x00000888
            else 
                val = (*(u32 *)(*(u32 *)(param + 4)) + 6) & 0x20; //0x000007CC & 0x000007D0
            
            if (val == 0) //0x000007D4
                g_Power.curPowerStateForCallback &= ~SCE_POWER_CALLBACKARG_LOW_BATTERY; //0x000007D8 & 0x0000087C & 0x000007E4
            else
                g_Power.curPowerStateForCallback |= SCE_POWER_CALLBACKARG_LOW_BATTERY; //0x000007E0 & 0x000007E4
            
            if (((*(u32 *)(*(u32 *)(param + 4)) + 6) & 0x1) == 0) //0x000007F0
                g_Power.curPowerStateForCallback &= SCE_POWER_CALLBACKARG_POWER_ONLINE; //0x0000086C & 0x00000874 & 0x00000800
            else
                g_Power.curPowerStateForCallback |= SCE_POWER_CALLBACKARG_POWER_ONLINE; //0x000007FC & 0x00000800
            
            g_Power.curPowerStateForCallback &= SCE_POWER_CALLBACKARG_HOLD_SWITCH;
            
            sdkVer = sceKernelGetCompiledSdkVersion(); //0x00000810
            if (sdkVer <= 0x06000000 && ((*(u32 *)(*(u32 *)(param + 4)) + 9) & 0x2000) == 0) //0x00000820 & 0x00000830
                g_Power.curPowerStateForCallback |= SCE_POWER_CALLBACKARG_HOLD_SWITCH; //0x00000844
            
            g_Power.curPowerStateForCallback &= SCE_POWER_CALLBACKARG_POWER_SWITCH; //0x00000860
            
            _scePowerBatteryUpdatePhase0(*(u32 *)(param + 4), &g_Power.curPowerStateForCallback); //0x0000085C -- sub_0000461C
            return SCE_ERROR_OK;
        }
        if (eventId == 0x00100000) //0x00000790
            _scePowerBatteryResume(); //0x00000798 -- sub_00005C18
        
        return SCE_ERROR_OK;   
    }
    if (eventId = 0x400) //0x00000748
        _scePowerBatterySuspend(); //0x00000770 -- sub_00004570
    
    return SCE_ERROR_OK;
}

// Subroutine scePower_04B7766E - Address 0x000008A8 - Aliases: scePower_driver_766CD857
s32 scePowerRegisterCallback(s32 slot, SceUID cbid)
{
    s32 oldK1;
    u32 idType;
    s32 status;
    s32 blockAttr;
    s32 intrState;
    SceSysmemUIDControlBlock *pBlock;
    
    oldK1 = pspShiftK1(); //0x000008C4

    if (slot < -1 || slot > POWER_CALLBACK_MAX_SLOT_KERNEL) { //0x000008D4
        pspSetK1(oldK1);
        return SCE_ERROR_INVALID_INDEX;
    }

    if (pspK1IsUserMode() && slot > POWER_CALLBACK_MAX_SLOT_USER) { //0x000008DC & 0x000008E4
        pspSetK1(oldK1);
        return SCE_ERROR_PRIV_REQUIRED;
    }
    
    /* Make sure the specified cbid is actually a valid callback ID.  */
    idType = sceKernelGetThreadmanIdType(cbid); //0x000008EC
    if (idType != SCE_KERNEL_TMID_Callback) { //0x000008F8
        pspSetK1(oldK1);
        return SCE_ERROR_INVALID_ID;
    }

    /* Verify that no kernel callback was specified when called from user mode.*/
    
    status = sceKernelGetUIDcontrolBlock(cbid, &pBlock); //0x00000904
    if (status != SCE_ERROR_OK) { //0x0000090C
        pspSetK1(oldK1);
        return SCE_ERROR_PRIV_REQUIRED;
    }

    blockAttr = (pspK1IsUserMode()) ? pBlock->attribute & 0x2 : 2; // 0x00000914 - 0x00000928
    if (blockAttr == 0) { //0x0000092C
        pspSetK1(oldK1);
        return SCE_ERROR_PRIV_REQUIRED;
    }
    
    intrState = sceKernelCpuSuspendIntr(); //0x00000934
    
    if (slot == SCE_POWER_CALLBACKSLOT_AUTO) { //0x00000940
        if (!pspK1IsUserMode) { //0x000009CC
            /* Don't allow auto slot searching in kernel mode. */
            sceKernelCpuResumeIntr(intrState);

            pspSetK1(oldK1);
            return SCE_ERROR_NOT_SUPPORTED;
        }

        s32 i;
        for (i = 0; i < POWER_CALLBACK_TOTAL_SLOTS_USER; i++) {
             if (g_Power.powerCallback[i].cbid < 0) { //0x000009F4
                 g_Power.powerCallback[i].cbid = cbid; //0x00000A14
                 g_Power.powerCallback[i].customPowerState = 0; //0x00000A20
                 g_Power.powerCallback[i].powerState = g_Power.curPowerStateForCallback; //0x00000A2C
                 g_Power.powerCallback[i].mode  = 0; //0x00000A28
                 
                 sceKernelNotifyCallback(cbid, g_Power.curPowerStateForCallback & g_Power.callbackArgMask); //0x000009B8

                 sceKernelCpuResumeIntr(intrState);
                 pspSetK1(oldK1);
                 return i; /* Return the slot used. */
             }             
        }

        /* No empty slot was found, return with error. */

       sceKernelCpuResumeIntr(intrState);
       pspSetK1(oldK1);
       return SCE_ERROR_OUT_OF_MEMORY;
    } 
    /* Check if the requested callback slot is available. */
    else if (g_Power.powerCallback[slot].cbid < 0) { //0x00000960
        /* Callback slot is free, let's use it. */

        g_Power.powerCallback[slot].cbid = cbid; //0x00000994
        g_Power.powerCallback[slot].customPowerState = 0; //0x000009A0
        g_Power.powerCallback[slot].powerState = g_Power.curPowerStateForCallback; //0x000009A8
        g_Power.powerCallback[slot].mode  = 0; //0x00000A28
        
        sceKernelNotifyCallback(cbid, g_Power.curPowerStateForCallback & g_Power.callbackArgMask); //0x000009B8

        status = SCE_ERROR_OK; // 0x0000099C
    }
    else
    {
        /* Requested slot already in use, return with error. */
        status = SCE_ERROR_ALREADY;
    }

    sceKernelCpuResumeIntr(intrState);

    pspSetK1(oldK1);
    return status;
}

// scePower_DB9D28DD
s32 scePowerUnregitserCallback(s32 slot) __attribute__((alias("scePowerUnregisterCallback")));

//Subroutine scePower_DB9D28DD - Address 0x00000A64 - Aliases: scePower_DFA8BAF8, scePower_driver_315B8CB6
s32 scePowerUnregisterCallback(s32 slot)
{
    s32 oldK1;
    
    oldK1 = pspShiftK1(); //0x00000A78

    if (slot < 0 || slot > POWER_CALLBACK_MAX_SLOT_KERNEL) { //0x00000A74
        pspSetK1(oldK1);
        return SCE_ERROR_INVALID_INDEX;
    }

    if (pspK1IsUserMode() && slot > POWER_CALLBACK_MAX_SLOT_USER) { //0x00000A94 & 0x00000AA0
        pspSetK1(oldK1);
        return SCE_ERROR_PRIV_REQUIRED;
    }
    
    pspSetK1(oldK1);

    if (g_Power.powerCallback[slot].cbid < 0) //0x00000AB0
        return SCE_ERROR_NOT_FOUND;
    
    g_Power.powerCallback[slot].cbid = -1; //0x00000ABC
    return SCE_ERROR_OK;
}

//Subroutine scePower_A9D22232 - Address 0x00000AD8 - Aliases: scePower_driver_29E23416
s32 scePowerSetCallbackMode(s32 slot, s32 mode)
{
    s32 oldK1;
    
    oldK1 = pspShiftK1();

    if (slot < 0 || slot > POWER_CALLBACK_MAX_SLOT_KERNEL) { //0x00000AE8
        pspSetK1(oldK1);
        return SCE_ERROR_INVALID_INDEX;
    }

    if (pspK1IsUserMode && slot > POWER_CALLBACK_MAX_SLOT_USER) { //0x00000B08 & 0x00000B14
        pspSetK1(oldK1);
        return SCE_ERROR_PRIV_REQUIRED;
    }
    
    pspSetK1(oldK1); //0x00000B18

    if (g_Power.powerCallback[slot].cbid < 0) //0x00000B24
        return SCE_ERROR_NOT_FOUND;
    
    g_Power.powerCallback[slot].mode = mode; //0x00000B2C
    return SCE_ERROR_OK;
}

//Subroutine scePower_BAFA3DF0 - Address 0x00000B48 - Aliases: scePower_driver_17EEA285
s32 scePowerGetCallbackMode(s32 slot, s32 *pMode)
{
    s32 oldK1;
    
    oldK1 = pspShiftK1();
    
    if (!pspK1PtrOk(pMode)) { //0x00000B54
        pspSetK1(oldK1);
        return SCE_ERROR_PRIV_REQUIRED;
    }

    if (slot < 0 || slot > POWER_CALLBACK_MAX_SLOT_KERNEL) { //0x00000B60
        pspSetK1(oldK1);
        return SCE_ERROR_INVALID_INDEX;
    }

    if (pspK1IsUserMode && slot > POWER_CALLBACK_MAX_SLOT_USER) { //0x00000B68 & 0x00000B7C
        pspSetK1(oldK1);
        return SCE_ERROR_PRIV_REQUIRED;
    }
    
    pspSetK1(oldK1); //0x00000B78

    if (g_Power.powerCallback[slot].cbid < 0) //0x00000B9C
        return SCE_ERROR_NOT_FOUND;
    
    if (pMode != NULL) //0x00000BA4
        *pMode = g_Power.powerCallback[slot].mode; //0x00000BB0
    
    return SCE_ERROR_OK;
}

//sub_00000BE0
void _scePowerNotifyCallback(s32 clearPowerState, s32 setPowerState, s32 cbOnlyPowerState)
{
    s32 intrState;
    s32 notifyCount;
    
    intrState = sceKernelCpuSuspendIntr(); //0x00000C0C
    
    // uofw note: Return value not used. 
    sceKernelGetCompiledSdkVersion(); // 0x00000C30

    g_Power.curPowerStateForCallback = (g_Power.curPowerStateForCallback & ~clearPowerState) | setPowerState; //0x00000C24 & 0x00000C2C & 0x00000C34
    
    u32 i;
    for (i = 0; i < POWER_CALLBACK_TOTAL_SLOTS_KERNEL; i++) {
         if (g_Power.powerCallback[i].cbid < 0) //0x00000C4C
             continue;
         
         notifyCount = sceKernelGetCallbackCount(g_Power.powerCallback[i].cbid); //0x00000C54
         if (notifyCount < 0) //0x00000C5C
             continue;
         
         if (notifyCount == 0) //0x00000C64
             g_Power.powerCallback[i].customPowerState = 0;
         
         g_Power.powerCallback[i].customPowerState |= cbOnlyPowerState; //0x00000C78
         g_Power.powerCallback[i].powerState = g_Power.powerCallback[i].customPowerState | g_Power.curPowerStateForCallback; //0x00000C7C
         sceKernelNotifyCallback(g_Power.powerCallback[i].cbid, 
                                 g_Power.powerCallback[i].powerState & g_Power.callbackArgMask); //0x00000C84
    }

    sceKernelCpuResumeIntr(intrState); //0x00000C94
}

//sub_00000CC4
s32 _scePowerIsCallbackBusy(u32 cbFlag, SceUID *pCbid)
{
    s32 intrState;
    s32 notifyCount;
    
    intrState = sceKernelCpuSuspendIntr(); //0x00000CEC
    
    u32 i;
    for (i = 0; i < POWER_CALLBACK_TOTAL_SLOTS_KERNEL; i++) {
         if (g_Power.powerCallback[i].cbid < 0) //0x00000D04
             continue;
         
         if ((g_Power.powerCallback[i].powerState & cbFlag) && g_Power.powerCallback[i].mode != 0) { //0x00000D14 & 0x00000D58
             notifyCount = sceKernelGetCallbackCount(g_Power.powerCallback[i].cbid); //0x00000D60
             if (notifyCount <= 0) //0x00000D68
                 continue;
             
             /* 
              * notifyCount > 0: Callback notification has been delayed at least once with the callback 
              * not being called yet.  Return the ID of the busy callback.
              */
             if (pCbid != NULL)
                 *pCbid = g_Power.powerCallback[i].cbid; //0x00000D74
             
             sceKernelCpuResumeIntr(intrState); //0x00000D78
             return SCE_TRUE; //0x00000D84
         }
    }

    sceKernelCpuResumeIntr(intrState); //0x00000D28
    return SCE_FALSE; //0x00000D30
}

//Subroutine scePower_driver_2638EF48 - Address 0x00000D88
s32 scePowerWlanActivate(void)
{
    s32 pllFreq;
    
    _scePowerLockPowerFreqMutex(); // 0x00000D94
    
    pllFreq = scePowerGetPllClockFrequencyInt(); // 0x00000D9C

    /* 
     * uofw note: No longer needed since the CPU clock and the bus clock are now derived from the PLL clock. 
     * Sony appears to have kept them in the source though (ignoring return values) so we do so as well.
     */
    scePowerGetCpuClockFrequencyInt(); // 0x00000DA4
    scePowerGetBusClockFrequencyInt(); // 0x00000DAC
   
    /* Check if the PSP currenty runs at a clock frequency where WLAN cannot be used.  */
    if ((g_Power.wlanExclusivePllClockLimit == SCE_POWER_WLAN_EXCLUSIVE_PLL_CLOCK_LIMIT_222Mhz && pllFreq > 222) 
        || (g_Power.wlanExclusivePllClockLimit == SCE_POWER_WLAN_EXCLUSIVE_PLL_CLOCK_LIMIT_266Mhz && pllFreq > 266)) //0x00000DBC - 0x00000DEC
    {
        /* 
         * The PSP is currently operating at a clock frequency where WLAN cannot be activated. In this case, 
         * the clock frequency needs to be reduced first.
         */
        _scePowerUnlockPowerFreqMutex();
        return SCE_POWER_ERROR_BAD_PRECONDITION;
    }

    g_Power.wlanActivity = SCE_POWER_WLAN_ACTIVATED; // 0x00000E1C

    _scePowerUnlockPowerFreqMutex(); //0x00000E18
    
    /* Suppress battery charging while WLAN is on. */
    if (g_Power.isWlanSuppressChargingEnabled) //0x00000E24
        scePowerBatteryForbidCharging(); //0x00000E34
    
    return SCE_ERROR_OK;
}

//Subroutine scePower_driver_8C6BEFD9 - Address 0x000010EC
s32 scePowerWlanDeactivate(void)
{
    g_Power.wlanActivity = SCE_POWER_WLAN_DEACTIVATED; // 0x00001104

    /* Allow battery charging again now that WLAN is turned off. */
    if (g_Power.isWlanSuppressChargingEnabled)
        scePowerBatteryPermitCharging(); // 0x00001118

    return SCE_ERROR_OK;
}

//Subroutine scePower_A85880D0 - Address 0x00001044 - Aliases: scePower_driver_693F6CF0
s32 scePowerCheckWlanCoexistenceClock(void)
{
    /*
     * Determine the maximum allowed clock frequencies when WLAN is active based on the hardware
     * we are running on (i.e. PSP-100X only has limited clock speed when WLAN is active).
     */

     // 0x0000104C - 0x0000107C
    return (sceKernelGetModel() == PSP_1000)
        ? (sceKernelDipsw(PSP_DIPSW_BIT_PLL_WLAN_COEXISTENCY_CLOCK) != PSP_DIPSW_PLL_WLAN_COEXISTENCY_CLOCK_333MHz)
            ? SCE_POWER_WLAN_COEXISTENCE_CLOCK_222MHz /* Device runs as a PSP 1000 */
            : SCE_POWER_WLAN_COEXISTENCE_CLOCK_333MHz /* Device runs as a PSP 2000+ (set in development tool) */
        : SCE_POWER_WLAN_COEXISTENCE_CLOCK_333MHz; /* PSP-2000 and later support 333 MHz clock frequency with WLAN. */
}

//Subroutine scePower_driver_114B75AB - Address 0x0000108C
s32 scePowerSetExclusiveWlan(u8 pllClockLimit)
{
    g_Power.wlanExclusivePllClockLimit = pllClockLimit;
    return SCE_ERROR_OK;
}

//Subroutine scePower_driver_E52B4362 - Address 0x0000109C
s32 scePowerCheckWlanCondition(s32 pllFrequency)
{
    if ((g_Power.wlanExclusivePllClockLimit == SCE_POWER_WLAN_EXCLUSIVE_PLL_CLOCK_LIMIT_222Mhz && pllFrequency > 222)
        || (g_Power.wlanExclusivePllClockLimit == SCE_POWER_WLAN_EXCLUSIVE_PLL_CLOCK_LIMIT_266Mhz && pllFrequency > 266)) // 0x000010A0 - 0x000010D4
    {
        return SCE_POWER_ERROR_BAD_PRECONDITION;
    }

    return SCE_ERROR_OK;
}

//Subroutine scePower_2B51FE2F - Address 0x00001134 - Aliases: scePower_driver_CE2032CD
// TODO: Verify function
u8 scePowerGetWlanActivity(void)
{
    return g_Power.wlanActivity; //0x0000113C
}

//Subroutine scePower_442BFBAC - Address 0x00000E44 - Aliases: scePower_driver_2509FF3B
// TODO: Verify function
u32 scePowerGetBacklightMaximum(void)
{
    u32 backlightMax;
    
    backlightMax = (scePowerIsPowerOnline() == SCE_TRUE) 
        ? SCE_POWER_MAX_BACKLIGHT_LEVEL_POWER_EXTERNAL 
        : SCE_POWER_MAX_BACKLIGHT_LEVEL_POWER_INTERNAL; //0x00000E4C & 0x00000E68

    if (g_Power.unk528 != 0)
        backlightMax = (g_Power.wlanActivity == SCE_POWER_WLAN_ACTIVATED) ? pspMin(backlightMax, g_Power.unk528) : backlightMax; //0x00000E70 & 0x00000E78
    
    return backlightMax;
}

//Subroutine module_start - Address 0x00000E8C
// TODO: Verify function
s32 _scePowerModuleStart(s32 argc, void *argp)
{
    (void)argc;
    (void)argp;
    
    memset(&g_Power, 0, sizeof g_Power); //0x00000EA8
    
    //0x00000EB0 - 0x00000EC4
    u32 i;
    for (i = 0; i < POWER_CALLBACK_TOTAL_SLOTS_KERNEL; i++)
         g_Power.powerCallback[i].cbid = -1;
    
    scePowerInit(); //scePower_driver_9CE06934
    return SCE_ERROR_OK;
    
}

//Subroutine syslib_ADF12745 - Address 0x00000EE4
// TODO: Verify function
s32 _scePowerModuleRebootPhase(s32 arg1)
{
    _scePowerFreqRebootPhase(arg1); //0x00000EF0 -- sub_00004038
    
    if (arg1 == 1) //0x00000F04
        _scePowerSetClockFrequency(333, 333, 166); //0x00000F20 -- sub_00003898
    
    return SCE_ERROR_OK;
}

//Subroutine module_reboot_before - Address 0x00000F30
s32 _scePowerModuleRebootBefore(u32 *arg1)
{
    u32 *ptr;
    
    ptr = *(arg1 + 11); //0x00000F3C
    if (ptr != NULL) {
        *(ptr + 5) = scePowerGetBatteryType(); //0x00000FA0 -- scePower_driver_071160B1
        scePowerGetTachyonVoltage(ptr + 6, ptr + 7); //0x00000FB0 -- scePower_driver_BADA8332
        scePowerGetDdrStrength(ptr + 8, ptr + 9); //0x00000FBC -- scePower_driver_16F965C9
    }
    return scePowerEnd();
}

//Subroutine scePower_driver_AD5BB433 - Address 0x00000FCC
// TODO: Verify function
s32 scePowerEnd(void)
{
    sceKernelUnregisterSysEventHandler(&g_PowerSysEv); //0x00000FD8
    sceSysconSetAcSupplyCallback(NULL, NULL); //0x00000FE4
    sceSysconSetLowBatteryCallback(NULL, NULL); //0x00000FF0
    
    _scePowerIdleEnd(); //0x00000FF8 -- sub_00003438
    _scePowerFreqEnd(); //0x00001000 -- sub_00003FC4
    _scePowerBatteryEnd(); //0x00001008 -- sub_00004498
    _scePowerSwEnd(); //0x00001010 -- sub_00002AA4
    
    return SCE_ERROR_OK;
}

//Subroutine scePower_driver_23BDDD8B - Address 0x00001028
// TODO: Verify function
u32 scePower_driver_23BDDD8B(void)
{
    g_Power.callbackArgMask &= ~SCE_POWER_CALLBACKARG_HOLD_SWITCH; //0x00001038 
    return SCE_ERROR_OK;
}

//Subroutine scePower_driver_4E32E9B8 - Address 0x00001128
// TODO: Verify function
u8 scePowerGetWatchDog(void)
{
    return g_Power.watchDog; //0x00001130
}

//Subroutine scePower_driver_C463E7F2 - Address 0x00001140
// TODO: Verify function
u8 scePowerGetLedOffTiming(void)
{
    return g_Power.ledOffTiming; //0x00001148
}

//0x0000114C
// TODO: Verify function
static u32 _scePowerInitCallback(void)
{
    s32 appType;
    
    appType = sceKernelApplicationType(); //0x00001154           
    if (appType != SCE_INIT_APPLICATION_VSH && appType != SCE_INIT_APPLICATION_POPS) //0x0000115C - 0x00001174
        _scePowerSetClockFrequency(g_Power.pllInitSpeed, g_Power.cpuInitSpeed, g_Power.busInitSpeed); //0x00001194 -- sub_00003898
    
    return SCE_ERROR_OK;
}

//sub_00001A94
// TODO: Verify function
s32 _scePowerChangeSuspendCap(u32 newSuspendCap)
{
    u32 param;
    s32 status;
    
    status = sceSysconReceiveSetParam(0, &param); //0x00001AAC
    if (status < SCE_ERROR_OK) //0x00001ABC
        return status;
    
    *((u8 *)param + 1) = (u8)((newSuspendCap & 0xFFFF) >> 8); //0x00001AB0 & 0x00001AB4 & 0x00001AC4
    *(u8 *)param = (u8)(newSuspendCap & 0xFFFF); //0x00001AB0 & 0x00001ACC
    
    status = sceSysconSendSetParam(0, &param); //0x00001AC8
    return (status < SCE_ERROR_OK) ? status : SCE_ERROR_OK; //0x00001AD4
}

//0x00003534
// TODO: Verify function -- i.e. delete
static u32 GetGp(void)
{
   return pspGetGp();
}