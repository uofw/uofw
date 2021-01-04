/* Copyright (C) The uOFW team
   See the file COPYING for copying permission.
*/

#include <common_imp.h>
#include <power_error.h>
#include <syscon.h>
#include <sysmem_kernel.h>
#include <sysmem_suspend_kernel.h>
#include <sysmem_sysevent.h>

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

#define SCE_POWER_POWER_CALLBACK_TOTAL_SLOTS            (32)
#define SCE_POWER_POWER_CALLBACK_MAX_SLOT               (SCE_POWER_POWER_CALLBACK_TOTAL_SLOTS - 1)
#define SCE_POWER_POWER_CALLBACK_USER_SLOTS             (16)
#define SCE_POWER_POWER_CALLBACK_MAX_USER_SLOT          (SCE_POWER_POWER_CALLBACK_USER_SLOTS - 1)

#define BATTERY_LOW_CAPACITY_VALUE                      (216)
#define BATTERY_REALLY_LOW_CAPACITY_VALUE               (72)

#define SCE_POWER_MAX_BACKLIGHT_LEVEL_POWER_INTERNAL    (3)
#define SCE_POWER_MAX_BACKLIGHT_LEVEL_POWER_EXTERNAL    (4)

#define SCE_POWER_CALLBACKARG_LOW_BATTERY           (0x00000100)
#define SCE_POWER_CALLBACKARG_POWER_ONLINE          (0x00001000)
#define SCE_POWER_CALLBACKARG_SUSPENDING            (0x00010000)
#define SCE_POWER_CALLBACKARG_RESUMING              (0x00020000)
#define SCE_POWER_CALLBACKARG_RESUME_COMP           (0x00040000)
#define SCE_POWER_CALLBACKARG_HOLD_SWITCH           (0x40000000)
#define SCE_POWER_CALLBACKARG_POWER_SWITCH			(0x80000000)

#define SCE_KERNEL_POWER_LOCK_DEFAULT               (0)

#define SCE_KERNEL_POWER_VOLATILE_MEM_DEFAULT       (0)

/** Cancel all PSP Hardware timers. */
#define SCE_KERNEL_POWER_TICK_DEFAULT               (0)     
/** Cancel auto-suspend-related timer. */
#define SCE_KERNEL_POWER_TICK_SUSPENDONLY           (1)
/** Cancel LCD-related timer */
#define SCE_KERNEL_POWER_TICK_LCDONLY               (6)	

typedef struct {
    u32 unk0;
    u32 unk4;
    u32 unk8;
    u32 unk12;
} SceSysEventParam;

typedef struct {
    s32 callbackId;
    s32 unk4;
    s32 powerStatus;
    s32 mode;
} ScePowerCallback;

typedef struct {
    ScePowerCallback powerCallback[SCE_POWER_POWER_CALLBACK_TOTAL_SLOTS]; // 0 - 511
    s32 baryonVersion; //512
    u32 unk516; //516 -- power status?
    u32 unk520; //520
    u8 isBatteryLow; //524
    u8 wlanActivity; //525
    u8 watchDog; //526
    u8 unk527;
    s8 unk528;
    s8 unk529;
    u8 ledOffTiming;
    s16 cpuInitSpeed; //534 -- CPU clock init speed
    s16 busInitSpeed; //536 -- Bus clock init speed
    s16 pllInitSpeed; //538 -- PLL clock init speed (PLL = phase-locked loop ?)
} ScePower;

typedef struct {
    s32 eventId; //0
    s32 semaId; //4
    s32 threadId; //8
    u32 mode; //12
    u32 unk16; //16
    s32 unk20; //20
    s32 numPowerLock; //24
    u32 startAddr; //28
    u32 memSize; //32
    u32 unk36; //36
    u32 unk40; //40
    u32 (*unk44)(u32, u32, u32, u32); //44
    u32 unk48; //48
    u32 wakeUpCondition; //60
    u32 resumeCount; //64
} ScePowerSwitch; //size: 68

typedef struct {
    u32 unk16;
    u32 unk20;
    u32 unk24;
    u32 unk28;
    u32 unk32;
    u32 unk36;
    u32 unk40;
    u32 unk44;
    u32 unk48;
    u32 unk52;
} ScePowerIdleData; //size: 40

typedef struct {
    u32 unk0;
    u32 unk4;
    u32 unk8;
    u32 unk12;
    ScePowerIdleData data[8];
} ScePowerIdle;

typedef struct {
    u32 mutexId; //0
    u32 sm1Ops; //4
    u32 pllOutSelect; //8
    u32 pllUseMask; //12
    u32 pllClockFrequencyInt; //16
    u32 clkcCpuFrequencyInt; //20
    u32 clkcBusFrequencyInt; //24
    u32 pllClockFrequencyFloat; //28
    u32 clkcCpuFrequencyFloat; //32
    u32 clkcBusFrequencyFloat; //36
    u32 clkcCpuGearNumerator; //40
    u32 clkcCpuGearDenominator; //44
    u32 clkcBusGearNumerator; //48
    u32 clkcBusGearDenominator; //52
    u32 unk56;
    s16 tachyonVoltage; //60
    s16 unk62;
    u32 unk64;
    s16 unk68;
    s16 unk70;
    s32 unk72;
    s16 unk76;
    s16 unk78;
    u32 geEdramRefreshMode; //80
    u32 oldGeEdramRefreshMode; //84
    u16 unk88;
    u16 scCpuClockLowerLimit; //90
    u16 scCpuClockUpperLimit; //92
    u16 scBusClockLowerLimit; //94
    u16 scBusClockUpperLimit; //96
    u16 pllClockLowerLimit; //98
    u16 pllClockUpperLimit; //100
    u16 unk102;
} ScePowerFrequency; //size: 104

typedef enum  {
    BATTERY_NOT_INSTALLED = 0,
    BATTERY_IS_BEING_DETECTED = 1,
    BATTERY_AVAILABLE = 2,
} ScePowerBatteryAvailabilityStatus;

typedef struct {
    u32 eventId; // 0
    u32 threadId; // 4
    u32 forceSuspendCapacity; // 8
    u32 lowBatteryCapacity; // 12
    u32 unk16;
    u32 unk20;
    u32 unk24;
    u32 unk28;
    u32 unk32;
    u32 alarmId; // 36
    u32 unk40;
    u32 batteryType; // 44
    u32 unk48;
    u32 unk52;
    u32 unk56;
    u32 unk60;
    ScePowerBatteryAvailabilityStatus batteryAvailabilityStatus; // 64
    u32 unk68;
    u32 unk72;
    u32 batteryLifePercentage; // 76
    u32 batteryFullCapacity; // 80
    u32 unk84;
    u32 unk88;
    u32 unk92;
    u32 unk96;
    u32 unk100;
    u32 unk100;
    s32 batteryVoltage; // 104
    u32 unk108;
    u32 unk112;
    u32 unk116;
    u32 unk120;
    u32 unk124;
    u32 unk128;
    u32 unk132;
    u32 unk136;
    u32 unk140;
    u32 unk144;
    u32 unk148;
    u32 unk152;
    u32 unk156;
    u32 unk160;
    u32 unkl64;
    u32 unk168;
    u32 unk172;
    u32 unk176;
    u32 unk180;
    u32 unk184;
    u32 unk188;
    u32 unk192;
    u32 unk196;
    u32 unk200;
    u32 unk204;
    u32 unk208;
} ScePowerBattery; //size: 212

typedef struct {
    u32 frequency;
    u32 unk4;
} ScePowerPllConfiguration; //size: 96

enum ScePowerWlanActivity {
    SCE_POWER_WLAN_DEACTIVATED = 0,
    SCE_POWER_WLAN_ACTIVATED = 1,
};

static SceSysconFunc _scePowerAcSupplyCallback; //sub_0x00000650
static SceSysconFunc _scePowerLowBatteryCallback; //sub_0x000006C4
static s32 _scePowerSysEventHandler(s32 eventId, char *eventName, void *param, s32 *result); //sub_0x0000071C
static s32 _scePowerInitCallback(); //sub_0x0000114C

static s32 _scePowerBatteryEnd(void); // 0x00004498
static s32 _scePowerBatterySuspend(void); // 0x00004570
static s32 _scePowerBatteryUpdatePhase0(void *arg0, u32 *arg1); // 0x0000461C
static s32 _scePowerBatteryConvertVoltToRCap(void); // 0x00005130
static s32 _scePowerBatteryUpdateAcSupply(void); // 0x0000544C
static s32 _scePowerBatterySetTTC(void); // 0x000056A4
static s32 _scePowerBatteryDelayedPermitCharging(void); // 0x00005EA4
static s32 _scePowerBatterySysconCmdIntr(void); // 0x00005ED8

ScePowerHandlers g_PowerHandler = {
    .size = sizeof(ScePowerHandlers),
    .tick = scePowerTick, //0x00002F94
    .lock = scePowerLockForKernel, //0x00001608
    .unlock = scePowerUnlockForKernel, //0x00002C68
    .lockForUser = scePowerLockForUser, //0x00002BF4
    .unlockForUser = scePowerUnlockForUser, //0x00002C24
    .rebootStart = scePowerRebootStart, //0x00002B78
    .memLock = scePowerVolatileMemLock, //0x000012E0
    .memTryLock = scePowerVolatileMemTryLock, //0x00001400
    .memUnlock = scePowerVolatileMemUnlock, //0x00001540
}; //0x00006F48

SceSysEventHandler g_PowerSysEv = {
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
}; //0x00007040

const ScePowerPllConfiguration g_pllSettings[12]; //0x00006F70
ScePower g_Power; //0x00007080
ScePowerSwitch g_PowerSwitch; //0x0000729C
ScePowerIdle g_PowerIdle; //0x0000C400
ScePowerFrequency g_PowerFreq; //0x0000C550
ScePowerBattery g_Battery; //0x0000C5B8

//scePower_driver_9CE06934 - Address 0x00000000
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
        g_Power.unk529 = 1;
        g_Power.watchDog = 0;
        g_Power.unk527 = 0;
        g_Power.unk528 = 0;
        g_Power.ledOffTiming = 0;
        batteryReallyLowCap = ((u16 *)baryonData)[BARYON_DATA_REALLY_LOW_BATTERY_CAP_SLOT]; //0x0000062C
        batteryLowCap = ((u16 *)baryonData)[BARYON_DATA_LOW_BATTERY_CAP_SLOT]; //0x00000634        
        _scePowerChangeSuspendCap(216); //0x00000640
    }
    else {
        g_Power.ledOffTiming = baryonData[31]; //0x00000080
        g_Power.watchDog = baryonData[24] & 0x7F; //0x00000088
        g_Power.unk527 = baryonData[25]; //0x0000008C
        g_Power.unk528 = baryonData[30]; //0x00000090
        g_Power.unk529 = ((s8)baryonData[52] < 0) ? baryonData[52] & 0x7F : 1; //0x00000098       
        batteryReallyLowCap = BATTERY_REALLY_LOW_CAPACITY_VALUE; //0x00000094
        batteryLowCap = BATTERY_LOW_CAPACITY_VALUE; //0x0000009C
    }
    appType = sceKernelApplicationType(); //0x000000A8
    if (appType == SCE_INIT_APPLICATION_GAME) { //0x000000B4
        g_Power.unk529 = 1;
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
            g_Power.unk529 = 1; //0x00000508
    }
    else {
        g_Power.unk529 = 0; //0x00000220 || 0x00000234
    }   
    status = sceKernelDipsw(49); //0x00000238
    if (status == 1) { //0x00000244
        g_Power.busInitSpeed = 166;
        g_Power.pllInitSpeed = 333; //0x000004C0
        g_Power.unk529 = 0;
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
        g_Power.unk516 |= SCE_POWER_CALLBACKARG_LOW_BATTERY; //0x000003D4
    
    if (sceSysconIsAcSupplied) //0x00000330
        g_Power.unk516 |= SCE_POWER_CALLBACKARG_POWER_ONLINE; //0x00000348
    
    g_Power.unk520 = -1; //0x00000350
    _scePowerIdleInit();//0x0000034C -- sub_000033C4
    
    sceSysconSetAcSupplyCallback(_scePowerAcSupplyCallback, NULL); //0x0000035C
    sceSysconSetLowBatteryCallback(_scePowerLowBatteryCallback, NULL); //0x0000036C
    sceKernelRegisterSysEventHandler(_scePowerSysEventHandler); //0x00000378
    sceKernelSetInitCallback(_scePowerInitCallback, 2, 0); //0x0000038C
    
    return SCE_ERROR_OK;
}

//0x00000650
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
        if (eventId == 0x1009) { //0x00000784
            type = g_Power.baryonVersion >> 16; //0x000007AC
            if ((type & 0xF0) >= 1 && ((type & 0xF0) ^ 0x10) >= 1) //loc_00000880
                val = (*(u32 *)(*(u32 *)(param + 4)) + 9) & 0x10; //0x00000880 & 0x00000888
            else 
                val = (*(u32 *)(*(u32 *)(param + 4)) + 6) & 0x20; //0x000007CC & 0x000007D0
            
            if (val == 0) //0x000007D4
                g_Power.unk516 &= ~SCE_POWER_CALLBACKARG_LOW_BATTERY; //0x000007D8 & 0x0000087C & 0x000007E4
            else
                g_Power.unk516 |= SCE_POWER_CALLBACKARG_LOW_BATTERY; //0x000007E0 & 0x000007E4
            
            if (((*(u32 *)(*(u32 *)(param + 4)) + 6) & 0x1) == 0) //0x000007F0
                g_Power.unk516 &= SCE_POWER_CALLBACKARG_POWER_ONLINE; //0x0000086C & 0x00000874 & 0x00000800
            else
                g_Power.unk516 |= SCE_POWER_CALLBACKARG_POWER_ONLINE; //0x000007FC & 0x00000800
            
            g_Power.unk516 &= SCE_POWER_CALLBACKARG_HOLD_SWITCH;
            
            sdkVer = sceKernelGetCompiledSdkVersion(); //0x00000810
            if (sdkVer <= 0x06000000 && ((*(u32 *)(*(u32 *)(param + 4)) + 9) & 0x2000) == 0) //0x00000820 & 0x00000830
                g_Power.unk516 |= SCE_POWER_CALLBACKARG_HOLD_SWITCH; //0x00000844
            
            g_Power.unk516 &= SCE_POWER_CALLBACKARG_POWER_SWITCH; //0x00000860
            
            _scePowerBatteryUpdatePhase0(*(u32 *)(param + 4), &g_Power.unk516); //0x0000085C -- sub_0000461C
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

//Subroutine scePower_04B7766E - Address 0x000008A8 - Aliases: scePower_driver_766CD857
s32 scePowerRegisterCallback(s32 slot, SceUID cbid)
{
    s32 oldK1;
    u32 idType;
    s32 status;
    s32 blockAttr;
    s32 intrState;
    s32 i;
    SceSysmemUIDControlBlock *block;
    
    oldK1 = pspShiftK1(); //0x000008C4
    if (slot < -1 || slot > SCE_POWER_POWER_CALLBACK_MAX_SLOT) { //0x000008D4
        pspSetK1(oldK1);
        return SCE_ERROR_INVALID_INDEX;
    }
    if (pspK1IsUserMode() && slot > SCE_POWER_POWER_CALLBACK_MAX_USER_SLOT) { //0x000008DC & 0x000008E4
        pspSetK1(oldK1);
        return SCE_ERROR_PRIV_REQUIRED;
    }
    
    idType = sceKernelGetThreadmanIdType(cbid); //0x000008EC
    if (idType != SCE_KERNEL_TMID_Callback) { //0x000008F8
        pspSetK1(oldK1);
        return SCE_ERROR_INVALID_ID;
    }
    
    status = sceKernelGetUIDcontrolBlock(cbid, &block); //0x00000904
    if (status != SCE_ERROR_OK) { //0x0000090C
        pspSetK1(oldK1);
        return SCE_ERROR_PRIV_REQUIRED;
    }
    blockAttr = (pspK1IsUserMode()) ? block->attribute & 0x2 : 2; //0x00000914 - 0x00000928
    if (blockAttr == 0) { //0x0000092C
        pspSetK1(oldK1);
        return SCE_ERROR_PRIV_REQUIRED;
    }
    
    intrState = sceKernelCpuSuspendIntr(); //0x00000934
    
    if (slot == -1) { //0x00000940
        if (!pspK1IsUserMode) { //0x000009CC
            sceKernelCpuResumeIntr(intrState);
            pspSetK1(oldK1);
            return SCE_ERROR_NOT_SUPPORTED;
        }
        for (i = 0; i < SCE_POWER_POWER_CALLBACK_USER_SLOTS; i++) {
             if (g_Power.powerCallback[i].callbackId < 0) { //0x000009F4
                 g_Power.powerCallback[i].callbackId = cbid; //0x00000A14
                 g_Power.powerCallback[i].unk4 = 0; //0x00000A20
                 g_Power.powerCallback[i].powerStatus = g_Power.unk516; //0x00000A2C
                 g_Power.powerCallback[i].mode  = 0; //0x00000A28
                 
                 sceKernelNotifyCallback(cbid, g_Power.unk516 & g_Power.unk520); //0x000009B8
                 sceKernelCpuResumeIntr(intrState);
                 pspSetK1(oldK1);
                 return i;
             }             
       }
       sceKernelCpuResumeIntr(intrState);
       pspSetK1(oldK1);
       return i;
    }
    if (g_Power.powerCallback[slot].callbackId < 0) { //0x00000960
        g_Power.powerCallback[slot].callbackId = cbid; //0x00000994
        g_Power.powerCallback[slot].unk4 = 0; //0x000009A0
        g_Power.powerCallback[slot].powerStatus = g_Power.unk516; //0x000009A8
        g_Power.powerCallback[slot].mode  = 0; //0x00000A28
        
        sceKernelNotifyCallback(cbid, g_Power.unk516 & g_Power.unk520); //0x000009B8
    }
    sceKernelCpuResumeIntr(intrState);
    pspSetK1(oldK1);
    return SCE_ERROR_OK;
}

//Subroutine scePower_DB9D28DD - Address 0x00000A64 - Aliases: scePower_DFA8BAF8, scePower_driver_315B8CB6
s32 scePowerUnregisterCallback(s32 slot)
{
    s32 oldK1;
    
    oldK1 = pspShiftK1(); //0x00000A78
    if (slot < 0 || slot > SCE_POWER_POWER_CALLBACK_MAX_SLOT) { //0x00000A74
        pspSetK1(oldK1);
        return SCE_ERROR_INVALID_INDEX;
    }
    if (pspK1IsUserMode() && slot > SCE_POWER_POWER_CALLBACK_MAX_USER_SLOT) { //0x00000A94 & 0x00000AA0
        pspSetK1(oldK1);
        return SCE_ERROR_PRIV_REQUIRED;
    }
    
    pspSetK1(oldK1);
    if (g_Power.powerCallback[slot].callbackId < 0) //0x00000AB0
        return SCE_ERROR_NOT_FOUND;
    
    g_Power.powerCallback[slot].callbackId = -1; //0x00000ABC
    return SCE_ERROR_OK;
}

//Subroutine scePower_A9D22232 - Address 0x00000AD8 - Aliases: scePower_driver_29E23416
s32 scePowerSetCallbackMode(s32 slot, s32 mode)
{
    s32 oldK1;
    
    oldK1 = pspShiftK1();
    if (slot < 0 || slot > SCE_POWER_POWER_CALLBACK_MAX_SLOT) { //0x00000AE8
        pspSetK1(oldK1);
        return SCE_ERROR_INVALID_INDEX;
    }
    if (pspK1IsUserMode && slot > SCE_POWER_POWER_CALLBACK_MAX_USER_SLOT) { //0x00000B08 & 0x00000B14
        pspSetK1(oldK1);
        return SCE_ERROR_PRIV_REQUIRED;
    }
    
    pspSetK1(oldK1); //0x00000B18
    if (g_Power.powerCallback[slot].callbackId < 0) //0x00000B24
        return SCE_ERROR_NOT_FOUND;
    
    g_Power.powerCallback[slot].mode = mode; //0x00000B2C
    return SCE_ERROR_OK;
}

//Subroutine scePower_BAFA3DF0 - Address 0x00000B48 - Aliases: scePower_driver_17EEA285
s32 scePowerGetCallbackMode(s32 slot, s32 *mode)
{
    s32 oldK1;
    
    oldK1 = pspShiftK1();
    
    if (!pspK1PtrOk(mode)) { //0x00000B54
        pspSetK1(oldK1);
        return SCE_ERROR_PRIV_REQUIRED;
    }
    if (slot < 0 || slot > SCE_POWER_POWER_CALLBACK_MAX_SLOT) { //0x00000B60
        pspSetK1(oldK1);
        return SCE_ERROR_INVALID_INDEX;
    }
    if (pspK1IsUserMode && slot > SCE_POWER_POWER_CALLBACK_MAX_USER_SLOT) { //0x00000B68 & 0x00000B7C
        pspSetK1(oldK1);
        return SCE_ERROR_PRIV_REQUIRED;
    }
    
    pspSetK1(oldK1); //0x00000B78
    if (g_Power.powerCallback[slot].callbackId < 0) //0x00000B9C
        return SCE_ERROR_NOT_FOUND;
    
    if (mode != NULL) //0x00000BA4
        *mode = g_Power.powerCallback[slot].mode; //0x00000BB0
    
    return SCE_ERROR_OK;
}

//sub_00000BE0
static void _scePowerNotifyCallback(s32 deleteCallbackFlag, s32 applyCallbackFlag, s32 arg3)
{
    s32 intrState;
    s32 sdkVer;
    s32 notifyCount;
    u32 i;
    
    intrState = sceKernelCpuSuspendIntr(); //0x00000C0C
    
    sdkVer = sceKernelGetCompiledSdkVersion();
    g_Power.unk516 = (g_Power.unk516 & ~deleteCallbackFlag) | applyCallbackFlag; //0x00000C24 & 0x00000C2C & 0x00000C34
    
    for (i = 0; i < SCE_POWER_POWER_CALLBACK_TOTAL_SLOTS; i++) {
         if (g_Power.powerCallback[i].callbackId < 0) //0x00000C4C
             continue;
         
         notifyCount = sceKernelGetCallbackCount(g_Power.powerCallback[i].callbackId); //0x00000C54
         if (notifyCount < 0) //0x00000C5C
             continue;
         
         if (notifyCount == 0) //0x00000C64
             g_Power.powerCallback[i].unk4 = 0;
         
         g_Power.powerCallback[i].unk4 |= arg3; //0x00000C78
         g_Power.powerCallback[i].powerStatus = g_Power.powerCallback[i].unk4 | g_Power.unk516; //0x00000C7C
         sceKernelNotifyCallback(g_Power.powerCallback[i].callbackId, 
                                 g_Power.powerCallback[i].powerStatus & g_Power.unk520); //0x00000C84
    }
    sceKernelCpuResumeIntr(intrState); //0x00000C94
}

//sub_00000CC4
static void _scePowerIsCallbackBusy(u32 callbackFlag, u32 *callbackId)
{
    s32 intrState;
    s32 notifyCount;
    u32 i;
    
    intrState = sceKernelCpuSuspendIntr(); //0x00000CEC
    
    for (i = 0; i < SCE_POWER_POWER_CALLBACK_TOTAL_SLOTS; i++) {
         if (g_Power.powerCallback[i].callbackId < 0) //0x00000D04
             continue;
         
         if ((g_Power.powerCallback[i].powerStatus & callbackFlag) && g_Power.powerCallback[i].mode != 0) { //0x00000D14 & 0x00000D58
             notifyCount = sceKernelGetCallbackCount(g_Power.powerCallback[i].callbackId); //0x00000D60
             if (notifyCount <= 0) //0x00000D68
                 continue;
             
             if (callbackId != NULL)
                 *callbackId = g_Power.powerCallback[i].callbackId; //0x00000D74
             
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
    s32 cpuFreq;
    s32 busFreq;
    
    sub_00003FEC(); //0x00000D94 -- sub_00003FEC -- lock mutex
    
    pllFreq = scePowerGetPllClockFrequencyInt(); //0x00000D9C -- scePower_34F9C463
    cpuFreq = scePowerGetCpuClockFrequencyInt(); //0x00000DA4 -- scePower_FDB5BFE9
    busFreq = scePowerGetBusClockFrequencyInt(); //0x00000DAC -- scePower_478FE6F5
   
    if ((g_Power.unk529 == 1 && pllFreq > 222) || (g_Power.unk529 == 2 && pllFreq > 266)) { //0x00000DBC - 0x00000DEC
        sub_00004014(); //0x00000DF4 -- unlock mutex
        return SCE_POWER_ERROR_BAD_PRECONDITION;
    }
    g_Power.wlanActivity = SCE_POWER_WLAN_ACTIVATED; //0x00000E1C
    sub_00004014(); //0x00000E18
    
    if (g_Power.unk527) //0x00000E24
        scePowerBatteryForbidCharging(); //0x00000E34 -- scePower_driver_10CE273F
    
    return SCE_ERROR_OK;
}

//Subroutine scePower_442BFBAC - Address 0x00000E44 - Aliases: scePower_driver_2509FF3B
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
s32 _scePowerModuleStart(s32 argc, void *argp)
{
    (void)argc;
    (void)argp;
    
    memset(&g_Power, 0, sizeof g_Power); //0x00000EA8
    
    //0x00000EB0 - 0x00000EC4
    u32 i;
    for (i = 0; i < SCE_POWER_POWER_CALLBACK_TOTAL_SLOTS; i++)
         g_Power.powerCallback[i].callbackId = -1;
    
    scePowerInit(); //scePower_driver_9CE06934
    return SCE_ERROR_OK;
    
}

//Subroutine syslib_ADF12745 - Address 0x00000EE4
s32 _scePowerModuleRebootPhase(s32 arg1)
{
    _scePowerFreqRebootPhase(arg1); //0x00000EF0 -- sub_00004038
    
    if (arg1 == 1) //0x00000F04
        scePowerSetClockFrequency(333, 333, 166); //0x00000F20 -- sub_00003898
    
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
u32 scePower_driver_23BDDD8B(void)
{
    g_Power.unk520 &= ~SCE_POWER_CALLBACKARG_HOLD_SWITCH; //0x00001038 
    return SCE_ERROR_OK;
}

//Subroutine scePower_A85880D0 - Address 0x00001044 - Aliases: scePower_driver_693F6CF0
u32 scePower_A85880D0(void)
{
    u32 pspModel;
    
    pspModel = sceKernelGetModel(); //0x0000104C
    if (pspModel == PSP_1000) { //0x00001054
        return (sceKernelDipsw(11) != 1) ? 0 : 1; //0x00001070 & 0x0000107C
    }
    return 1;
}

//Subroutine scePower_driver_114B75AB - Address 0x0000108C
u32 scePowerSetExclusiveWlan(u8 arg1) 
{
    g_Power.unk529 = arg1;
    return SCE_ERROR_OK; //0x00001098
}

//Subroutine scePower_driver_E52B4362 - Address 0x0000109C
s32 scePowerCheckWlanCondition(u32 freq)
{
    if ((g_Power.unk529 == 1 && freq > 222) || (g_Power.unk529 == 2 && freq > 266)) //0x000010A0 - 0x000010D4
        return SCE_POWER_ERROR_BAD_PRECONDITION;
    
    return SCE_ERROR_OK;
}

//Subroutine scePower_driver_8C6BEFD9 - Address 0x000010EC
u32 scePowerWlanDeactivate(void)
{
    g_Power.wlanActivity = SCE_POWER_WLAN_DEACTIVATED; //0x00001104
    if (g_Power.unk527 != 0)
        scePowerBatteryPermitCharging(); //0x00001118 -- scePower_driver_EF751B4A
   
    return SCE_ERROR_OK;
}

//Subroutine scePower_driver_4E32E9B8 - Address 0x00001128
u8 scePowerGetWatchDog(void)
{
    return g_Power.watchDog; //0x00001130
}

//Subroutine scePower_2B51FE2F - Address 0x00001134 - Aliases: scePower_driver_CE2032CD
u8 scePowerGetWlanActivity(void)
{
    return g_Power.wlanActivity; //0x0000113C
}

//Subroutine scePower_driver_C463E7F2 - Address 0x00001140
u8 scePowerGetLedOffTiming(void)
{
    return g_Power.ledOffTiming; //0x00001148
}

//0x0000114C
static u32 _scePowerInitCallback(void)
{
    s32 appType;
    
    appType = sceKernelApplicationType(); //0x00001154           
    if (appType != SCE_INIT_APPLICATION_VSH && appType != SCE_INIT_APPLICATION_POPS) //0x0000115C - 0x00001174
        scePowerSetClockFrequency(g_Power.pllInitSpeed, g_Power.cpuInitSpeed, g_Power.busInitSpeed); //0x00001194 -- sub_00003898
    
    return SCE_ERROR_OK;
}

//sub_000011A4
static u32 _scePowerSwInit(void)
{
    s32 intrState;
    s32 nPowerLock;
    SceSysmemPartitionInfo partitionInfo;
    
    memset(&g_PowerSwitch, 0, sizeof g_PowerSwitch); //0x000011C4
    
    g_PowerSwitch.mode = 2; //0x000011E8
    g_PowerSwitch.wakeUpCondition = 8; //0x000011F0
    
    g_PowerSwitch.eventId = sceKernelCreateEventFlag("ScePowerSw", SCE_KERNEL_EA_MULTI | 0x1, 0x50000, NULL); //0x000011EC & 0x00001210
    g_PowerSwitch.semaId = sceKernelCreateSema("ScePowerVmem", 1, 0, 1, NULL); //0x0000120C & 0x0000123C
    g_PowerSwitch.threadId = sceKernelCreateThread("ScePowerMain", _scePowerOffThread, 4, 2048, 
                                                   SCE_KERNEL_TH_NO_FILLSTACK | 0x1, NULL); //0x00001238 & 0x00001250
    
    sceKernelStartThread(g_PowerSwitch.threadId, 0, NULL); //0x0000124C   
    sceSysconSetPowerSwitchCallback(_scePowerPowerSwCallback, NULL); //0x0000125C
    sceSysconSetHoldSwitchCallback(_scePowerHoldSwCallback, NULL); //0x0000126C
    
    partitionInfo.size = 16; //0x00001284
    sceKernelQueryMemoryPartitionInfo(5, &partitionInfo); //0x00001280
    g_PowerSwitch.memSize = partitionInfo.memSize; //0x00001290
    g_PowerSwitch.startAddr = partitionInfo.startAddr; //0x00001298
    
    intrState = sceKernelCpuSuspendIntr(); //0x00001294
    
    scePowerLockForKernel(SCE_KERNEL_POWER_LOCK_DEFAULT); //0x000012A0 -- scePower_driver_6CF50928
    nPowerLock = sceKernelRegisterPowerHandlers(&g_PowerHandler); //0x000012AC
    g_PowerSwitch.numPowerLock += nPowerLock; //0x000012BC
    
    sceKernelCpuResumeIntr(intrState); //0x000012C0
    return SCE_ERROR_OK;
}

//Subroutine scePower_23C31FFE - Address 0x000010A4 - Aliases: scePower_driver_CE239543
s32 scePowerVolatileMemLock(s32 mode, void **ptr, s32 *size)
{
    s32 intrState;
    s32 oldK1;
    s32 status;
    u32 nBits;
    
    if (mode != SCE_KERNEL_POWER_VOLATILE_MEM_DEFAULT) //0x0000130C
        return SCE_ERROR_INVALID_MODE;
    
    oldK1 = pspShiftK1(); //0x00001314
    
    if (!pspK1PtrOk(ptr) || !pspK1PtrOk(size)) { //0x00001320 & loc_00001364
        pspSetK1(oldK1);
        return SCE_ERROR_PRIV_REQUIRED;
    }
    
    status = sceKernelWaitSema(g_PowerSwitch.semaId, 1, NULL); //0x00001370
    if (ptr != NULL) //0x00001378
        *ptr = g_PowerSwitch.startAddr; //0x00001384
    if (size != NULL) //0x00001388
        *size = g_PowerSwitch.memSize; //0x00001394
    
    intrState = sceKernelCpuSuspendIntr(); //0x00001398
    
    /* test for 1024 byte alignment. */
    if (((g_PowerSwitch.startAddr & 0x1F800000) >> 22) == 0x10) { //0x000013B0
        /* test for 256 byte alignment. */
        nBits = ((g_PowerSwitch.startAddr & 0xE0000000) >> 28); //0x000013B4 & 0x000013DC
        if (((s32)0x35 >> nBits) & 0x1) //0x000013E4
            sceKernelSetDdrMemoryProtection(g_PowerSwitch.startAddr, g_PowerSwitch.memSize, 0xF); //0x000013F0
    }
    g_PowerSwitch.unk36 = 1; //0x000013C8
    
    sceKernelCpuResumeIntrWithSync(intrState); //0x000013C4
    pspSetK1(oldK1);
    return status;
}

//Subroutine scePower_FA97A599 - Address 0x00001400 - Aliases: scePower_driver_A882AEB7
s32 scePowerVolatileMemTryLock(s32 mode, void **ptr, s32 *size)
{
    s32 intrState;
    s32 oldK1;
    s32 status;
    u32 nBits;
    
    if (mode != SCE_KERNEL_POWER_VOLATILE_MEM_DEFAULT) //0x0000142C
        return SCE_ERROR_INVALID_MODE; 
    
    oldK1 = pspShiftK1(); //0x00001434
    
    if (!pspK1PtrOk(ptr) || !pspK1PtrOk(size)) { //0x00001440 & 0x0000144C
        pspSetK1(oldK1);
        return SCE_ERROR_PRIV_REQUIRED;
    }
    
    status = sceKernelPollSema(g_PowerSwitch.semaId, 1); //0x0000148C
    if (status != SCE_ERROR_OK) { //0x00001494
        pspSetK1(oldK1);
        return (status == SCE_ERROR_KERNEL_SEMA_ZERO) ? SCE_POWER_ERROR_CANNOT_LOCK_VMEM : status; //0x00001524 - 0x0000153C
    }
    
    if (ptr != NULL) //0x0000149C
        *ptr = g_PowerSwitch.startAddr; //0x000014A8
    if (size != NULL) //0x000014AC
        *size = g_PowerSwitch.memSize; //0x000014B8
    
    intrState = sceKernelCpuSuspendIntr(); //0x000014BC
    
    /* test for 1024 byte alignment. */
    if (((g_PowerSwitch.startAddr & 0x1FFFFFC0) >> 6) == 0x10) { //0x000014D4
        /* test for 256 byte alignment. */
        nBits = ((g_PowerSwitch.startAddr & 0xFFFFFFF8) >> 3) & 0x1F; //0x000014D8 & 0x00001500
        if (((s32)0x35 >> nBits) & 0x1) //0x00001508
            sceKernelSetDdrMemoryProtection(g_PowerSwitch.startAddr, g_PowerSwitch.memSize, 0xF); //0x00001514
    }
    g_PowerSwitch.unk36 = 1; //0x000014EC
    
    sceKernelCpuResumeIntrWithSync(intrState); //0x000014E8
    pspSetK1(oldK1);
    return status;
}

//Subroutine scePower_B3EDD801 - Address 0x00001540 - Aliases: scePower_driver_5978B1C2
s32 scePowerVolatileMemUnlock(s32 mode)
{
    s32 oldK1;
    s32 intrState;
    s32 status;
    u32 nBits;
    
    if (mode != SCE_KERNEL_POWER_VOLATILE_MEM_DEFAULT)
        return SCE_ERROR_INVALID_MODE;
    
    oldK1 = pspShiftK1(); //0x00001434
    
    sceKernelDcacheWritebackInvalidateRange(g_PowerSwitch.startAddr, g_PowerSwitch.memSize); //0x0000157C
    
    intrState = sceKernelCpuSuspendIntr(); //0x00001584
    
    /* test for 1024 byte alignment. */
    if (((g_PowerSwitch.startAddr & 0x1FFFFFC0) >> 6) == 0x10) { //0x000015A0 & 0x000015A8
        /* test for 256 byte alignment. */
        nBits = ((g_PowerSwitch.startAddr & 0xFFFFFFF8) >> 3) & 0x1F; //0x00001598 & 0x0000159C & 0x000015AC & 0x000015F0
        if (((s32)0x35 >> nBits) & 0x1) //0x00001508
            sceKernelSetDdrMemoryProtection(g_PowerSwitch.startAddr, g_PowerSwitch.memSize, 0); //0x000015F8
    }
    status = sceKernelSignalSema(g_PowerSwitch.semaId, 1); //0x000015B4
    g_PowerSwitch.unk36 = 0; //0x000015BC
    
    sceKernelCpuResumeIntrWithSync(intrState); //0x000014E8
    pspSetK1(oldK1);
    return status;
}

//Subroutine scePower_driver_6CF50928 - Address 0x00001608
void scePowerLockForKernel(u32 lockType)
{
    _scePowerLock(lockType, 0); //0x00001610
}

//sub_00001624
void _scePowerLock(u32 lockType, u32 arg2)
{
    s32 intrState;
    
    if (lockType != SCE_KERNEL_POWER_LOCK_DEFAULT) //0x00001644
        return SCE_ERROR_INVALID_MODE;
    
    if (g_PowerSwitch.unk48 != 0) //0x00001658
        return SCE_ERROR_OK;
    
    intrState = sceKernelCpuSuspendIntr(); //0x00001680
    
    if (g_PowerSwitch.unk20 == 0 && g_PowerSwitch.numPowerLock == 0) //0x00001698
        sceKernelClearEventFlag(g_PowerSwitch.eventId, ~0x10000); //0x0000170C
    
    if (arg2 == 0) { //0x000016A0
        g_PowerSwitch.numPowerLock += 1; //0x000016F4
        sceKernelCpuResumeIntr(intrState);
        return SCE_ERROR_OK;
    }
    g_PowerSwitch.unk20 += 1; //0x000016BC
    
    sceKernelCpuResumeIntr(intrState);
    if (g_PowerSwitch.unk16 == 0) //0x000016C4
        return SCE_ERROR_OK;
    
    return sceKernelWaitEventFlag(g_PowerSwitch.eventId, 0x40000, SCE_KERNEL_EW_OR, NULL, NULL); //0x000016DC
}

//Subroutine scePower_3951AF53 - Address 0x0000171C - Aliases: scePower_driver_3300D85A
s32 scePowerWaitRequestCompletion(void)
{
    s32 oldK1;
    u32 resultBits;
    s32 status;
    
    oldK1 = pspShiftK1(); //0x00001748
    
    status = sceKernelPollEventFlag(g_PowerSwitch.eventId, 0xFFFFFFFF, SCE_KERNEL_EW_OR, &resultBits);
    if (status < SCE_ERROR_OK && status != SCE_ERROR_KERNEL_EVENT_FLAG_POLL_FAILED) { //0x0000174C & 0x00001764
        pspSetK1(oldK1); //0x000017EC
        return status;
    }
    if ((resultBits & 0x1F0) == 0) { //0x0000177C
        pspSetK1(oldK1); //0x000017EC
        return SCE_ERROR_OK;
    }
    
    if ((resultBits & 0x20000) == 0) { //0x00001780& 0x00001790
        status = sceKernelWaitEventFlag(g_PowerSwitch.eventId, 0x20000, SCE_KERNEL_EW_OR, &resultBits, NULL); //0x00001798
        if (status < SCE_ERROR_OK) { //0x000017A0
            pspSetK1(oldK1); //0x000017EC
            return status;
        }
    }
    status = sceKernelWaitEventFlag(g_PowerSwitch.eventId, 0x40000, SCE_KERNEL_EW_OR, &resultBits, NULL); //0x000017B8
    pspSetK1(oldK1); //0x000017C8
    return (status < SCE_ERROR_OK) ? status : SCE_ERROR_OK; //0x000017C4
}

//0x000017F0
static s32 _scePowerOffThread(void)
{
    s32 status;
    u32 resultBits; //sp
    u32 timeOut; //sp + 4
    
start:    
    sceKernelWaitEventFlag(g_PowerSwitch.eventId, 0x800001F1, SCE_KERNEL_EW_OR, &resultBits, NULL); //0x00001844
    if ((s32)resultBits < 0) //0x00001850
        return SCE_ERROR_OK;
    
    if (resultBits & 0x1) { //0x00001858
        timeOut = 0x1E8480; //0x00001A08 -- 2 seconds
        status = sceKernelWaitEventFlag(g_PowerSwitch.eventId, 0x800001F2, SCE_KERNEL_EW_OR, &resultBits, &timeOut); //0x00001A04
        if ((s32)resultBits < 0) //0x00001A10
            return SCE_ERROR_OK;
        
        if (g_PowerSwitch.mode != 0) { //0x00001A20
            if ((resultBits & 0x2) == 0) //0x00001A34
                _scePowerNotifyCallback(0, 0, 0x10000000); //0x00001A444
            else 
                _scePowerNotifyCallback(0, 0, 0x20000000); //0x00001A38
        }
        if (g_PowerSwitch.mode & 0x2) { //0x00001A60
            if ((resultBits & 0x2) == 0) { //0x00001A6C
                if (status == SCE_ERROR_KERNEL_WAIT_TIMEOUT) //0x00001A84
                    g_PowerSwitch.unk40 = 1; //0x00001A88
            }
            else {
                g_PowerSwitch.unk40 = 2; //0x00001A74
            }
        }
    }
    if ((resultBits & 0x10) == 0) { //0x00001864
        if (resultBits & 0x20) //0x00001998
            g_PowerSwitch.unk44 = 2; //0x000019A0
        else if (resultBits & 0x40) //0x000019AC
            g_PowerSwitch.unk44 = 3; //0x000019B4
        else if (resultBits & 0x80) //0x000019C0
            g_PowerSwitch.unk44 = 4; //0x000019C8
        else if (g_PowerSwitch.unk40 == 0) //0x000019D8
            goto start;
    }
    sceKernelWaitEventFlag(g_PowerSwitch.eventId, 0x80010000, 1, &resultBits, NULL); //0x00001880
    if ((s32)resultBits < 0) //0x00001890
        return SCE_ERROR_OK;
    if (resultBits & 0x80) //0x0000189C
        g_PowerSwitch.unk44 = 4; //0x000018A0
    
    sceKernelClearEventFlag(g_PowerSwitch.eventId, ~0x40000); //0x000018A8
    sceKernelSetEventFlag(g_PowerSwitch.eventId, 0x20000); //0x000018B8
    
    if (g_PowerSwitch.unk48 == 0) { //0x000018C4
        if (g_PowerSwitch.unk40 == 1 || g_PowerSwitch.unk44 == 1) //0x000018D4 & 0x000018E0
            _scePowerSuspendOperation(0x101); //0x00001940
        else if (g_PowerSwitch.unk40 == 2 || g_PowerSwitch.unk44 == 2) {//0x000018F0 & 0x000018FC
            _scePowerSuspendOperation(0x202); //0x00001940
            g_PowerSwitch.unk40 = 0; //0x00001958
            g_PowerSwitch.unk44 = 0; //0x0000195C
        }
        else if (g_PowerSwitch.unk44 == 3) { //0x00001908
            _scePowerSuspendOperation(0x303); //0x00001940
            g_PowerSwitch.unk40 = 0; //0x00001958
            g_PowerSwitch.unk44 = 0; //0x0000195C
        }
        else if (g_PowerSwitch.unk44 == 4) //0x00001914
            _scePowerSuspendOperation(0x404); //0x00001940
    }
    
    sceKernelClearEventFlag(g_PowerSwitch.eventId, ~0x20000); //0x00001924
    sceKernelSetEventFlag(g_PowerSwitch.eventId, 0x40000); //0x00001930
    goto start; //0x00001938
}

//sub_00001A94
static s32 _scePowerChangeSuspendCap(u32 newSuspendCap)
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

//sub_00001AE8
static s32 _scePowerSuspendOperation(u32 arg1)
{
    u8 buf[64];
    u8 buf2[56];
    u8 buf3[128];
    void *unk1; //sp + 200
    
    memset(buf, 0, sizeof buf); //0x00001B24
    memset(buf2, 0, sizeof buf2); //0x00001B38 - 0x00001B70
    memset(buf3, 0, sizeof buf3); //0x00001B6C
    
    ((u32 *)buf)[0] = sizeof buf; //0x00001B88
    ((u32 *)buf2)[0] = sizeof buf2; //0x00001B8C
    ((u32 *)buf3)[0] = sizeof buf3; //0x00001B90
    ((u32 *)buf3)[1] = 0x6060010; //0x00001B94 -- sdk version?
    
    ((u32 *)buf3)[13] = sceKernelGetUMDData(); //0x00001B94
    InitForKernel_D83A9BD7(unk1); //0x00001BA0
    
    scePowerGetLedOffTiming(); //0x00001BC8
}

//sub_000029B8
static u32 _scePowerOffCommon(void)
{
    s32 usbPowerType;
    
    sceGpioPortClear(0x2000000); //0x000029C8
    usbPowerType = _sceSysconGetUsbPowerType();
    if (usbPowerType == 0) //0x000029D8
        sceGpioPortClear(0x00800000); //0x00002A94
    
    //0x000029E0 - 0x00002A00
    u32 i;
    for (i = 0; i < 6; i++) {
         if ((i != 2) && (i != 3)) //0x000029F0
             sceSysregUartIoDisable(i); //0x00002A84
    }
    //0x00002A04 - 0x00002A1C
    for (i = 0; i < 6; i++) {
         if (i != 0) //0x00002A08
             sceSysregSpiIoDisable(i); //0x00002A74
    }
    //0x00002A20 - 0x00002A40
    for (i = 0; i < 32; i++) {
         if ((i != 3) && (i != 4) && (i != 7)) //0x00002A2C
             sceSysreg_driver_15DC34BC(i); //0x00002A64
    }
    return SCE_ERROR_OK;
}

//sub_00002AA4
static s32 _scePowerSwEnd(void)
{
    u32 nBits;
    
    sceKernelSetEventFlag(g_PowerSwitch.eventId, 0x80000000); //0x00002AC0
    sceKernelWaitThreadEnd(g_PowerSwitch.threadId, 0); //0x00002ACC
    
    sceKernelRegisterPowerHandlers(NULL); //0x00002AD4
    sceSysconSetPowerSwitchCallback(NULL, NULL); //0x00002AE0
    sceSysconSetHoldSwitchCallback(NULL, NULL); //0x00002AEC
    
    sceKernelDeleteSema(g_PowerSwitch.semaId); //0x00002AF4
    sceKernelDeleteEventFlag(g_PowerSwitch.eventId); //0x00002AFC
    
    
    /* test if address is between 0x087FFFFF and 0x08000000. */
    if (((g_PowerSwitch.startAddr & 0x1F800000) >> 22) == 0x10) { //0x00002B20
        /* Support 0x0..., 0x1..., 0x4..., 0x5..., 0x8..., 0x9..., 0xA..., 0xB...  */
        nBits = ((g_PowerSwitch.startAddr & 0xE0000000) >> 28); //0x00002B0C
        if (((s32)0x35 >> nBits) & 0x1) //0x00002B10 & 0x00002B14
            sceKernelSetDdrMemoryProtection(g_PowerSwitch.startAddr, g_PowerSwitch.memSize, 0xF); //0x000013F0
    }
    return SCE_ERROR_OK;
}

//Subroutine scePower_0CD21B1F - Address 0x00002B58 - Aliases: scePower_driver_73785D34
u32 scePowerSetPowerSwMode(u32 mode)
{
    g_PowerSwitch.mode = mode & 0x3;
    return SCE_ERROR_OK;
}

//Subroutine scePower_165CE085 - Address 0x00002B6C - Aliases: scePower_driver_E11CDFFA
u32 scePowerGetPowerSwMode(void)
{
    return g_PowerSwitch.mode;
}

//Subroutine scePower_driver_1EC2D4E4 - Address 0x00002B78
u32 scePowerRebootStart(void)
{
    s32 oldK1;
    s32 intrState;
    
    oldK1 = pspShiftK1(); 
    
    _scePowerLock(SCE_KERNEL_POWER_LOCK_DEFAULT, 1); //0x00002B98
    
    pspSetK1(oldK1); //0x00002BB0
    intrState = sceKernelCpuSuspendIntr(); //0x00002BAC
    
    sceKernelClearEventFlag(g_PowerSwitch.eventId, ~0x10000); //0x00002BC0
    sceKernelSetEventFlag(g_PowerSwitch.eventId, 0x00040000); //0x00002BCC
    
    sceKernelCpuResumeIntr(intrState); //0x00002BD4
    return SCE_ERROR_OK;
}

//Subroutine scePower_D6D016EF - Address 0x00002BF4
void scePowerLockForUser(u32 lockType) 
{
    s32 oldK1;
    
    oldK1 = pspShiftK1();
    
    _scePowerLock(lockType, 1); //0x00002C08
    
    pspSetK1(oldK1);
}

//Subroutine scePower_CA3D34C1 - Address 0x00002C24
s32 scePowerUnlockForUser(u32 mode)
{
    s32 oldK1;
    s32 status;
    
    if (mode != 0) //0x00002C3C
        return SCE_ERROR_INVALID_MODE;
    
    oldK1 = pspShiftK1();
    
    status = _scePowerUnlock(mode, 1); //0x00002C44
    
    pspSetK1(oldK1);
    return status;
}

//Subroutine scePower_driver_C3024FE6 - Address 0x00002C68
s32 scePowerUnlockForKernel(u32 mode)
{
    return _scePowerUnlock(mode, 0); //0x00002C70
}

//Subroutine scePower_7FA406DD - Address 0x00002C84 - Aliases: scePower_driver_566B8353
s32 scePowerIsRequest(void)
{
    return (0 < (g_PowerSwitch.unk40 | g_PowerSwitch.unk44));
}

//Subroutine scePower_DB62C9CF - Address 0x00002CA0 - Aliases: scePower_driver_DB62C9CF
u32 scePowerCancelRequest(void)
{
    s32 intrState;
    u32 oldData;
    
    intrState = sceKernelCpuSuspendIntr();//0x00002CA8
    
    oldData = g_PowerSwitch.unk40; //0x00002CB8
    g_PowerSwitch.unk40 = 0;
    
    sceKernelCpuResumeIntr(intrState);
    return oldData;
}

//Subroutine scePower_2B7C7CF4 - Address 0x00002CDC - Aliases: scePower_driver_9B44CFD9
u32 scePowerRequestStandby(void)
{
    s32 oldK1;
    
    oldK1 = pspShiftK1();
    
    sceKernelSetEventFlag(g_PowerSwitch.eventId, 0x10); //0x00002CF8
    
    pspSetK1(oldK1);
    return SCE_ERROR_OK;
}

//Subroutine scePower_AC32C9CC - Address 0x00002D18 - Aliases: scePower_driver_5C1333B7
u32 scePowerRequestSuspend(void)
{
    s32 oldK1;
    
    oldK1 = pspShiftK1();
    
    sceKernelSetEventFlag(g_PowerSwitch.eventId, 0x20); //0x00002D34
    
    pspSetK1(oldK1);
    return SCE_ERROR_OK;
}

//Subroutine scePower_driver_D79B0122 - Address 0x00002D54
u32 scePower_driver_D79B0122(void)
{
    return SCE_ERROR_OK;
}

//Subroutine scePower_2875994B - Address 0x00002D5C - Aliases: scePower_driver_D1FFF513
u32 scePowerRequestSuspendTouchAndGo(void)
{
    s32 oldK1;
    
    oldK1 = pspShiftK1();
    
    sceKernelSetEventFlag(g_PowerSwitch.eventId, 0x40); //0x00002D78
    
    pspSetK1(oldK1);
    return SCE_ERROR_OK;
}

//Subroutine scePower_0442D852 - Address 0x00002D98 - Aliases: scePower_driver_9DAF25A0
s32 scePowerRequestColdReset(u32 mode)
{
    s32 oldK1;
    
    oldK1 = pspShiftK1();
    
    if (pspK1IsUserMode() && mode != 0) //0x00002DBC & 0x00002DC8
        pspSetK1(oldK1);
        return SCE_ERROR_INVALID_MODE;
    
    sceKernelSetEventFlag(g_PowerSwitch.eventId, 0x80); //0x00002DD4
    
    pspSetK1(oldK1);
    return SCE_ERROR_OK;
}

//Subroutine scePower_0074EF9B - Address 0x00002DFC - Aliases: scePower_driver_B45C9066
u32 scePowerGetResumeCount(void)
{
    return g_PowerSwitch.resumeCount; 
}

//Subroutine scePower_driver_BA566CD0 - Address 0x00002E0C
u32 scePowerSetWakeupCondition(u32 wakeUpCondition)
{
    g_PowerSwitch.wakeUpCondition = wakeUpCondition;
    return SCE_ERROR_OK;
}

//Subroutine sub_00002E1C - Address 0x00002E1C
s32 _scePowerUnlock(u32 mode, u32 arg1)
{
    s32 intrState;
    s32 numPowerLock;
    
    if (g_PowerSwitch.unk48 != 0)
        return SCE_ERROR_OK;
    
    intrState = sceKernelCpuSuspendIntr(); //0x00002E6C
    
    if (arg1 != 0) { //0x00002E74
        numPowerLock = g_PowerSwitch.numPowerLock;
        if (numPowerLock > 0) //0x00002ECC
            g_PowerSwitch.numPowerLock = --numPowerLock; //0x00002EDC
    }
    else {
        numPowerLock = g_PowerSwitch.unk20;
        if (numPowerLock > 0) //0x00002E80
            g_PowerSwitch.unk20 = --numPowerLock; //0x00002E8C
    }
    
    if (g_PowerSwitch.unk20 == 0 && g_PowerSwitch.numPowerLock == 0) //0x00002E94 & 0x00002EA0
        sceKernelSetEventFlag(g_PowerSwitch.eventId, 0x10000); //0x00002EB8
    
    sceKernelCpuResumeIntr(intrState); //0x00002EA8
    return numPowerLock;
}

//0x00002EE0
static void _scePowerPowerSwCallback(s32 enable, void *argp)
{
    (void)argp;

    if (enable != 0) { //0x00002EF4
        sceKernelSetEventFlag(g_PowerSwitch.eventId, 1); //0x00002EFC
        sceKernelClearEventFlag(g_PowerSwitch.eventId, ~0x2); //0x00002F08
        _scePowerNotifyCallback(0, SCE_POWER_CALLBACKARG_POWER_SWITCH, 0); //0x00002F14
    } else {
        sceKernelSetEventFlag(g_PowerSwitch.eventId, 2); //0x00002F38
        sceKernelClearEventFlag(g_PowerSwitch.eventId, ~0x1); //0x00002F44
        _scePowerNotifyCallback(SCE_POWER_CALLBACKARG_POWER_SWITCH, 0, 0); //0x00002F4C
    }
}

//0x00002F58
static void _scePowerHoldSwCallback(s32 enable, void *argp)
{
    (void)argp;

    if (enable != 0) //0x00002F6C
        _scePowerNotifyCallback(0, SCE_POWER_CALLBACKARG_HOLD_SWITCH, 0); //0x00002F64
    else 
        _scePowerNotifyCallback(SCE_POWER_CALLBACKARG_HOLD_SWITCH, 0, 0); //0x00002F74
}

//Subroutine scePower_EFD3C963 - Address 0x00002F94 - Aliases: scePower_driver_0EFEE60E
u32 scePowerTick(u32 tickType)
{
    u32 i;
    
    //0x00002F9C - 0x00002FF8
    for (i = 0; i < 8; i++) {
         if ((((s32)tickType >> i) & 0x1) == 0 && (tickType != 0 || g_PowerIdle.data[i].unk40 & 0x200)) //0x00002FB0 & 0x00002FB8 & 0x00002FC8
             continue;
         
         if (g_PowerIdle.data[i].unk16 != 0) //0x00002FD4
             continue;
         
         g_PowerIdle.data[i].unk16 = 1;
         g_PowerIdle.data[i].unk24 = g_PowerIdle.unk0; //0x00002FF0
         g_PowerIdle.data[i].unk28 = g_PowerIdle.unk4; //0x00002FF4
         
    }
    return SCE_ERROR_OK;
}

//Subroutine scePower_EDC13FE5 - Address 0x00003008 - Aliases: scePower_driver_DF336CDE
u32 scePowerGetIdleTimer(u32 slot, SceKernelSysClock *sysClock, u32 *arg2) 
{
    s32 oldK1;
    s64 sysTime;
    
    oldK1 = pspShiftK1(); //0x00003018
    
    if (slot >= 8) { //0x00003040
        pspSetK1(oldK1);
        return SCE_ERROR_INVALID_INDEX;
    }  
    if (!pspK1PtrOk(sysClock) || !pspK1PtrOk(arg2)) { //0x00003054 & 0x0000305C
        pspSetK1(oldK1);
        return SCE_ERROR_PRIV_REQUIRED;
    }
    
    sysTime = sceKernelGetSystemTimeWide(); //0x00003088
    if (sysClock != NULL) { //0x000030C0
        sysClock->low = (u32)sysTime - g_PowerIdle.data[slot].unk24; //0x000030BC
        sysClock->hi = ((u32)(sysTime >> 32) - g_PowerIdle.data[slot].unk28) - ((u32)sysTime < g_PowerIdle.data[slot].unk24); //0x000030CC & 0x000030C4 & 0x000030B8
    }
    if (arg2 != NULL) { //0x000030D0
        *arg2 = g_PowerIdle.data[slot].unk32 - ((u32)sysTime - g_PowerIdle.data[slot].unk24); //0x000030E8
        *(u32 *)(arg2 + 4) = (g_PowerIdle.data[slot].unk36 - (((u32)(sysTime >> 32) - g_PowerIdle.data[slot].unk28) - 
                                                        ((u32)sysTime < g_PowerIdle.data[slot].unk24))) - 
                              (g_PowerIdle.data[slot].unk32 < (u32)sysTime - g_PowerIdle.data[slot].unk24);
    }
    return (u32)sysTime - g_PowerIdle.data[slot].unk24;
}

//Subroutine scePower_driver_1BA2FCAE - Address 0x00003100
u32 scePowerSetIdleCallback(u32 slot, u32 arg1, u32 arg2, u32 arg3, u32 arg4, u32 arg5)
{
    s32 intrState;
    s64 sysTime;
    
    if (slot >= 8) //0x00003148
        return SCE_ERROR_INVALID_INDEX;
    
    intrState = sceKernelCpuSuspendIntr(); //0x00003150
    
    if (arg3 != 0 && g_PowerIdle.data[slot].unk44 != 0) { //0x00003170 & 0x0000317C
        sceKernelCpuResumeIntr(intrState);
        return SCE_ERROR_ALREADY;
    }
    sysTime = sceKernelGetSystemTimeWide(); //0x00003198
    g_PowerIdle.data[slot].unk40 = arg1; //0x0000319C
    g_PowerIdle.data[slot].unk44 = arg4; //0x000031B8
    g_PowerIdle.data[slot].unk24 = (u32)sysTime; //0x000031AC
    g_PowerIdle.data[slot].unk28 = (u32)(sysTime >> 32); //0x000031B0
    g_PowerIdle.data[slot].unk16 = 0; //0x000031D4
    g_PowerIdle.data[slot].unk20 = -1; //0x000031BC
    g_PowerIdle.data[slot].unk32 = arg2; //0x000031C0
    g_PowerIdle.data[slot].unk36 = arg3; //0x000031C4
    g_PowerIdle.data[slot].unk48 = GetGp(); //0x000031C8
    g_PowerIdle.data[slot].unk52 = arg5; //0x000031CC
    
    sceKernelCpuResumeIntr(intrState); //0x000031D0
    return SCE_ERROR_OK;
}

//0x00003220
static s32 _scePowerVblankInterrupt(s32 subIntNm, void *arg)
{
    s64 sysTime;
    u8 isAcSupplied;
    s32 gp;
    u32 i;
    u32 data;
    u32 diff;
    void (*func)(u32, u32, u32, u32);

    (void)subIntNm;
    (void)arg;
    
    sysTime = sceKernelGetSystemTimeWide(); //0x00003240
    g_PowerIdle.unk0 = (u32)sysTime; //0x00003258
    g_PowerIdle.unk4 = (u32)(sysTime >> 32);
    
    isAcSupplied = sceSysconIsAcSupplied(); //0x0000325C
    gp = pspGetGp(); //0x00003264
    
    //0x0000326C
    for (i = 0; i < 8; i++) {
        if (g_PowerIdle.data[i].unk44 == 0) //0x00003278
            continue;
        
        if ((g_PowerIdle.unk8 & (1 << i)) == 0) //0x00003288 & 0x00003290
            continue;
        
        if (isAcSupplied && ((g_PowerIdle.data[i].unk40 & 0x100) == 0)) { //0x000032DC & 0x000032EC
            g_PowerIdle.data[i].unk16 = 1; //0x000032F4
            g_PowerIdle.data[i].unk24 = (u32)sysTime;
            g_PowerIdle.data[i].unk16 (u32)(sysTime >> 32);           
        }
        if (g_PowerIdle.data[i].unk16 != 0) { //0x00003304
            g_PowerIdle.data[i].unk16 = 0; //0x00003314
            if (g_PowerIdle.data[i].unk20 == 0) //0x00003310
                continue;
            
            g_PowerIdle.data[i].unk20 = 0; //0x00003318
            pspSetGp(g_PowerIdle.data[i].unk48); //0x00003324
            func = g_PowerIdle.data[i].unk44;
            func(i, 0, g_PowerIdle.data[i].unk52, &g_PowerIdle.data[i].unk16); //0x00003334
            continue;
        }
        if (g_PowerIdle.data[i].unk20 == 1) //0x0000334C
            continue;
        
        data = ((u32)(sysTime >> 32) - g_PowerIdle.data[i].unk28) - (g_PowerIdle.data[i].unk24 < (u32)sysTime); //0x00003360 & 0x00003364 & 0x00003368
        if (data < g_PowerIdle.data[i].unk36) //0x0000336C & 0x00003370
            continue;
        
        diff = (u32)sysTime - g_PowerIdle.data[i].unk24; //0x00003374
        if ((data == g_PowerIdle.data[i].unk36) && (diff < g_PowerIdle.data[i].unk32)) //0x00003378 & 0x000033B4
            continue;
        
        g_PowerIdle.data[i].unk20 = 1; //0x00003380
        
        pspSetGp(g_PowerIdle.data[i].unk48); //0x00003390
        func = g_PowerIdle.data[i].unk44; //0x00003398
        func(i, diff, g_PowerIdle.data[i].unk52, &g_PowerIdle.data[i].unk16); //0x000033A0
    }
    return -1;
}

//sub_000033C4
static u32 _scePowerIdleInit(void)
{
    u64 sysTime;
    
    memset(&g_PowerIdle, 0, sizeof g_PowerIdle); //0x000033E0
    sysTime = sceKernelGetSystemTimeWide(); //0x000033E8
    
    g_PowerIdle.unk0 = (u32)sysTime; //0x00003408
    g_PowerIdle.unk4 = (u32)(sysTime >> 32);
    g_PowerIdle.unk8 = 255;
    
    sceKernelRegisterSubIntrHandler(SCE_VBLANK_INT, 0x1A, _scePowerVblankInterrupt, NULL); //0x0000340C
    sceKernelEnableSubIntr(SCE_VBLANK_INT, 0x1A); //0x0000341C
    return SCE_ERROR_OK;
}

//sub_00003438
static u32 _scePowerIdleEnd(void)
{
    sceKernelReleaseSubIntrHandler(SCE_VBLANK_INT, 0x1A); //0x00003444
    return SCE_ERROR_OK;
}

//Subroutine scePower_7F30B3B1 - Address 0x000030F0 - Aliases: scePower_driver_E660E488
u32 scePowerIdleTimerEnable(u32 slot)
{
    s32 intrState;
    u32 data;
    
    if (slot >= 8) //0x00003474
        return SCE_ERROR_INVALID_INDEX;
    
    intrState = sceKernelCpuSuspendIntr(); //0x0000347C
    
    data = g_PowerIdle.unk8;
    g_PowerIdle.unk8 |= (1 << slot); //0x0000348C - 0x0000349C
    
    sceKernelCpuResumeIntr(intrState);
    return (data >> slot) & 0x1;
}

//Subroutine scePower_972CE941 - Address 0x000034C8 - Aliases: scePower_driver_961A06A5
u32 scePowerIdleTimerDisable(u32 slot)
{
    s32 intrState;
    u32 data;
    
    if (slot >= 8) //0x000034D0
        return SCE_ERROR_INVALID_INDEX;
    
    intrState = sceKernelCpuSuspendIntr; //0x000034E8
    
    data = g_PowerIdle.unk8; //0x000034F8
    g_PowerIdle.unk8 &= (~1 << slot); //0x00003508
    
    sceKernelCpuResumeIntr(intrState);
    return (data >> slot) & 0x1; //0x000031A4
}

//0x00003534
static u32 GetGp(void)
{
   return pspGetGp();
}

//sub_0000353C
static u32 _scePowerFreqInit(void) 
{
    float pllFrequency;
    float clkcCpuFrequency;
    float clkcBusFrequency;
    s32 tachyonVer;
    u32 fuseConfig;
    
    memset(&g_PowerFreq, 0, sizeof g_PowerFreq); //0x0000355C
    
    g_PowerFreq.sm1Ops = sceKernelSm1ReferOperations(); //0x00003564
    g_PowerFreq.scBusClockLowerLimit = 24; //0x00003580
    g_PowerFreq.pllClockLowerLimit = 1; //0x00003584
    g_PowerFreq.pllClockUpperLimit = 333; //0x00003588
    g_PowerFreq.scCpuClockLowerLimit = 1; //0x0000358C
    g_PowerFreq.scCpuClockUpperLimit = 333; //0x00003590
    g_PowerFreq.scBusClockUpperLimit = 166; //0x00003598
    
    pllFrequency = sceSysregPllGetFrequency();
    g_PowerFreq.pllClockFrequencyFloat = pllFrequency; //0x000035A0
    g_PowerFreq.pllClockFrequencyInt = (s32)pllFrequency; //0x000035A8
    
    g_PowerFreq.pllOutSelect = sceSysregPllGetOutSelect(); //0x000035A4 & 0x000035B0
    
    clkcCpuFrequency = sceClkcGetCpuFrequency(); //0x000035AC
    g_PowerFreq.pllClockFrequencyFloat = clkcCpuFrequency; //0x000035B8
    g_PowerFreq.pllClockFrequencyInt = (s32)clkcCpuFrequency; //0x000035C0
    
    clkcBusFrequency = sceClkcGetBusFrequency(); //0x000035BC
    g_PowerFreq.clkcBusFrequencyFloat = clkcBusFrequency; //0x000035E4
    g_PowerFreq.clkcBusFrequencyInt = (s32)clkcBusFrequency; //0x000035DC
    
    g_PowerFreq.pllUseMask = 0x3F3F; //0x000035D0
    
    if (g_PowerFreq.pllOutSelect == 5) { //0x000035E0
        g_PowerFreq.unk56 = 1; //0x000036B0
        g_PowerFreq.unk64 = 1;
        g_PowerFreq.unk72 = 1;
    } else if (g_PowerFreq.pllOutSelect == 4) { //0x000035EC
        g_PowerFreq.unk56 = 1; //0x000036A0
        g_PowerFreq.unk64 = 0;
        g_PowerFreq.unk72 = 0;
    } else {
        g_PowerFreq.unk56 = 0; //0x000035F8
        g_PowerFreq.unk64 = 0;
        g_PowerFreq.unk72 = 0;
    }
    g_PowerFreq.unk78 = -1; //0x00003608
    g_PowerFreq.unk68 = -1;
    g_PowerFreq.unk70 = -1;
    g_PowerFreq.unk76 = -1;
    
    tachyonVer = sceSysregGetTachyonVersion(); //0x00003614
    if (tachyonVer >= 0x00140000) { //0x00003628 -- PSP Motherboard at least "TA-079 v1"
        fuseConfig = sceSysregGetFuseConfig(); //0x00003674
        g_PowerFreq.unk62 = 0xB00 - ((fuseConfig & 0x3800) >> 3); //0x0000367C & 0x00003680 & 0x0000368C
        g_PowerFreq.unk60 = (~fuseConfig) & 0x700; //0x00003684 & 0x00003690 & 0x0000369C
    } else {
        g_PowerFreq.unk62 = 0x400; //0x00003630
        g_PowerFreq.unk60 = 0; //0x00003634
    }
    
    scePowerSetGeEdramRefreshMode(1); //0x0000363C
    g_PowerFreq.mutexId = sceKernelCreateMutex("ScePowerClock", 1, 0, NULL); //0x00003650
    return SCE_ERROR_OK;
}

//Subroutine scePower_843FBF43 - Address 0x00003350 - Aliases: scePower_driver_BD02C252
s32 scePowerSetCpuClockFrequency(s32 cpuFrequency)
{   
    s32 oldK1;
    s32 intrState;
    s32 cpuFreq;
     
    if (cpuFrequency <= 0 || cpuFrequency <= g_PowerFreq.pllClockFrequencyInt) //0x000036E8 & 0x000036F8
        return SCE_ERROR_INVALID_VALUE;
    
    oldK1 = pspShiftK1(); //0x00003728
    intrState = sceKernelCpuSuspendIntr(); //0x00003724
    
    if (cpuFrequency >= g_PowerFreq.scCpuClockLowerLimit) //0x00003734
        cpuFreq = pspMin(cpuFrequency, g_PowerFreq.scCpuClockUpperLimit); //0x00003780
    else 
        cpuFreq = g_PowerFreq.scCpuClockLowerLimit; //0x0000373C
    
    sceClkcSetCpuFrequency((float)cpuFreq); //0x00003748
    float cpuFreqFloat = sceClkcGetCpuFrequency(); //0x00003750
    g_PowerFreq.clkcCpuFrequencyFloat = cpuFreqFloat; //0x0000375C
    g_PowerFreq.clkcCpuFrequencyInt = (s32)cpuFreqFloat; //0x00003768
    
    sceKernelCpuResumeIntr(intrState); //0x00003764
    pspSetK1(oldK1); //0x0000376C
    return SCE_ERROR_OK;
}

//Subroutine scePower_B8D7B3FB - Address 0x00003784 - Aliases: scePower_driver_B71A8B2F
s32 scePowerSetBusClockFrequency(s32 busFrequency)
{
    s32 oldK1;
    s32 intrState;
    s32 status;
    s32 pllClockFreq;
    s32 userLevel;
    
    pllClockFreq = ((s32)(g_PowerFreq.pllClockFrequencyInt + (g_PowerFreq.pllClockFrequencyInt >> 31))) >> 1;
    if (busFrequency <= 0 || busFrequency <= pllClockFreq) //0x000037B0 & 0x000037CC
        return SCE_ERROR_INVALID_VALUE;
    
    oldK1 = pspShiftK1(); //0x000037FC
    intrState = sceKernelCpuSuspendIntr(); //0x000037F8
    
    userLevel = sceKernelGetUserLevel(); //0x00003800
    if (userLevel < 4) //0x0000380C
        busFrequency = pllClockFreq; //0x00003820
    
    if (busFrequency >= g_PowerFreq.scBusClockLowerLimit) //0x00003828
        busFrequency = pspMin(busFrequency, g_PowerFreq.scBusClockUpperLimit); //0x00003894
    else 
        busFrequency = g_PowerFreq.scBusClockLowerLimit; //0x00003830
    
    pllClockFreq = ((s32)(((u32)((s32)g_PowerFreq.pllClockFrequencyInt >> 31)) >> 30 + g_PowerFreq.pllClockFrequencyInt)) >> 2;
    busFrequency = pspMax(busFrequency, pllClockFreq); //0x0000384C
    status = sceClkcSetBusFrequency((float)busFrequency); //0x00003854
    
    float busFrequencyFloat = sceClkcGetBusFrequency(); //0x0000385C
    g_PowerFreq.clkcBusFrequencyFloat = busFrequencyFloat; //0x00003868
    g_PowerFreq.clkcBusFrequencyInt = (s32)busFrequencyFloat; //0x00003870
    
    scePowerSetGeEdramRefreshMode(scePowerGetGeEdramRefreshMode()); //0x0000386C & 0x00003874
    
    sceKernelCpuResumeIntr(intrState); //0x0000387C
    pspSetK1(oldK1); //0x00003884
    return status;
}

//Subroutine sub_00003898 - Address 0x00003898 
u32 scePowerSetClockFrequency(s32 pllFrequency, s32 cpuFrequency, s32 busFrequency) 
{
    s32 oldK1;
    u32 frequency; 
    u32 unk1;
    s32 status;
    
    if ((pllFrequency < 19 || pllFrequency > 333) || cpuFrequency < 1) //0x000038A4 & 0x000038A8 & 0x000038BC & 0x000038E0
        return SCE_ERROR_INVALID_VALUE;
    
    if (cpuFrequency > 333 || busFrequency < 1) //0x000038E8 - 0x000038FC
        return SCE_ERROR_INVALID_VALUE;
    
    if (busFrequency > 167 || pllFrequency < cpuFrequency) //0x00003904 & 0x00003910
        return SCE_ERROR_INVALID_VALUE;
    
    if (pllFrequency < (busFrequency * 2)) //0x00003918
        return SCE_ERROR_INVALID_VALUE;
    
    if (pllFrequency >= g_PowerFreq.pllClockLowerLimit) //0x00003968
        frequency = pspMin(pllFrequency, g_PowerFreq.pllClockUpperLimit); //loc_00003E5C
    else 
        frequency = g_PowerFreq.pllClockLowerLimit; //0x00003970
    
    //0x00003980 - 0x000039BC
    unk1 = -1; //0x00003984
    u32 i;
    for (i = 0; i < 12; i++) {
        if (((g_pllSettings[i].unk4 << 1) & g_PowerFreq.pllUseMask) == 0) //0x000039A0
            continue;
        
        if (g_pllSettings[i].frequency >= frequency) { //0x000039B0
            frequency = g_pllSettings[i].frequency; //0x000039B4
            unk1 = g_pllSettings[i].unk4; //0x00003E58
            break;
        }
    }
    if (unk1 < 0) { //0x000039C0
        frequency = 333; //0x000039C4
        unk1 = 5; //0x00003E50
    }
    
    if (cpuFrequency >= g_PowerFreq.scCpuClockLowerLimit) //0x000039D4
        cpuFrequency = pspMin(cpuFrequency, g_PowerFreq.scCpuClockUpperLimit); //0x00003E48
    else 
        cpuFrequency = g_PowerFreq.scCpuClockLowerLimit; //0x000039DC
    
    cpuFrequency = pspMin(cpuFrequency, frequency); //0x000039F0
    
    if (busFrequency >= g_PowerFreq.scBusClockLowerLimit) //0x000039EC
        busFrequency = pspMin(busFrequency, g_PowerFreq.scBusClockUpperLimit); //0x00003E40
    else 
        busFrequency = g_PowerFreq.scBusClockLowerLimit; //0x000039F4
    
    oldK1 = pspShiftK1(); //0x00003A10
    
    status = sceKernelLockMutex(g_PowerFreq.mutexId, 1, NULL); //0x00003A18 
    if (status < SCE_ERROR_OK) { //0x00003A20
        pspSetK1(oldK1);
        return status;
    }
    
    status = scePowerGetWlanActivity(); //0x00003A28
    if (status == SCE_POWER_WLAN_ACTIVATED) { //0x00003A30
         status = scePowerCheckWlanCondition(frequency); //0x00003A3C
         if (status < SCE_ERROR_OK) { //0x00003A44
             u32 sdkVersion = sceKernelGetCompiledSdkVersion(); //0x00003E08
             if (sdkVersion > 0x0200000F) { //0x00003E18
                 sceKernelUnlockMutex(g_PowerFreq.mutexId, 1); //0x00003DF4
                 
                 pspSetK1(oldK1); //0x00003DFC
                 return status; //0x00003E04
             }
             frequency = 222; //0x00003E1C
             cpuFrequency = 222; //0x00003E24
             busFrequency = 111; //0x00003E2C
             unk1 = 4; //0x00003E20
         }
    }
    status = sceKernelGetUserLevel(); //0x00003A4C
    if (status < 4) //0x00003A58
        busFrequency = ((s32)((frequency >> 31) + frequency)) >> 31; //0x00003A60 - 0x00003A68
    
    scePowerLockForKernel(0); //0x00003A6C
    
    if (g_PowerFreq.pllOutSelect != unk1) { //0x00003A78
        SceSysEventParam sysEventParam;
        sysEventParam.unk0 = 8;
        sysEventParam.unk4 = frequency;
        s32 result = 0;
        SceSysEventHandler *sysEventHandler = NULL;
        status = sceKernelSysEventDispatch(SCE_SPEED_CHANGE_EVENTS, 0x01000000, "query", &sysEventParam, &result, 1, &sysEventHandler); //0x00003AAC
        if (status < SCE_ERROR_OK) { //0x00003AB4
            sceKernelSysEventDispatch(SCE_SPEED_CHANGE_EVENTS, 0x01000001, "cancel", &sysEventParam, NULL, 0, NULL); //0x00003DE0
            scePowerUnlockForKernel(0); //0x00003DE8
            sceKernelUnlockMutex(g_PowerFreq.mutexId, 1); //0x00003DF4
            pspSetK1(oldK1);
            return status;
        }
        
        if (unk1 == 5 && g_PowerFreq.unk64 == 0) { //0x00003AC0 & 0x00003D88
            if (g_PowerFreq.unk68 >= 0 && g_PowerFreq.unk70 != g_PowerFreq.unk68) //0x00003D94 & 0x00003DA0
                sceSysconCtrlVoltage(3, g_PowerFreq.unk68); //0x00003DA8
            g_PowerFreq.unk64 = 1; //0x00003DB4
        }
        if (unk1 >= 6 && g_PowerFreq.unk56 == 0 && g_PowerFreq.tachyonVoltage >= 0) { //0x00003AD0 & 0x00003AE0 & 0x00003AEC
            sceSysconCtrlTachyonVoltage(g_PowerFreq.tachyonVoltage); //0x00003AF4
            g_PowerFreq.unk56 = 1; //0x00003B00
        }
        if (unk1 == 5 && g_PowerFreq.unk72 == 0) { //0x00003B08 & 0x00003D50
            if (g_PowerFreq.unk76 >= 0 && g_PowerFreq.unk76 != g_PowerFreq.unk78) //0x00003D5C & 0x00003D68
                sceDdr_driver_0BAAE4C5(); //0x00003D70 -- TODO: Check back for required arguments.
            g_PowerFreq.unk72 = 1; //0x00003D84
        }
        sceKernelSysEventDispatch(SCE_SPEED_CHANGE_EVENTS, 0x01000002, "start", &sysEventParam, NULL, 0, NULL); //0x00003B30
        sceDisplayWaitVblankStart(); //0x00003B38
    
        u32 intrState = sceKernelCpuSuspendIntr(); //0x00003B40
    
        sceClkcSetBusGear(511, 511); //0x00003B50
        sceSysreg_driver_63E1EE9C(511, 511); //0x00003B5C
    
        sceKernelSysEventDispatch(SCE_SPEED_CHANGE_EVENTS, 0x01000010, "phase0", &sysEventParam, NULL, 0, NULL); //0x00003B84
        if (g_PowerFreq.sm1Ops != NULL) { //0x00003B90
            void (*func)(u32 unk1, u32 unk2) = (*func)(u32, u32)*(u32 *)(((void *)(g_PowerFreq.sm1Ops)) + 48);
            func(unk1, -1); //0x00003B94 & 0x00003D3C
        }
        sceDdrChangePllClock(unk1); //0x00003B98
    
        float pllFrequency = sceSysregPllGetFrequency(); //0x00003BA0
        g_PowerFreq.pllClockFrequencyFloat = pllFrequency(); //0x00003BAC
        g_PowerFreq.pllClockFrequencyInt = (u32)pllFrequency; //0x00003BB8
    
        sceKernelSysEventDispatch(SCE_SPEED_CHANGE_EVENTS, 0x01000011, "phase1", &sysEventParam, NULL, 0, NULL); //0x00003BD4
    
        sceKernelCpuResumeIntr(intrState); //0x00003BDC
        
        sceKernelSysEventDispatch(SCE_SPEED_CHANGE_EVENTS, 0x01000020, "end", &sysEventParam, NULL, 0, NULL); //0x00003C04
        
        if (unk1 != 5 && g_PowerFreq.unk72 == 1) { //0x00003C10 & 0x00003C1C
            if (g_PowerFreq.unk78 >= 0 && g_PowerFreq.unk78 != g_PowerFreq.unk76) //0x00003D14 & 0x00003D20
                sceDdr_driver_0BAAE4C5(); //0x00003D28 -- TODO: Check for arguments.
            g_PowerFreq.unk72 = 0; //0x00003D34
        }
        if (unk1 >= 6 && g_PowerFreq.unk56 == 1 && g_PowerFreq.unk62 >= 0) { //0x00003C28 & 0x00003C3C & 0x00003CFC
            sceSysconCtrlTachyonVoltage(g_PowerFreq.unk62); //0x00003D04
            g_PowerFreq.unk56 = 0; //0x00003D10
        }
        if (unk1 != 5 && g_PowerFreq.unk64 == 1) { //0x00003C48 & 0x00003C58
            if (g_PowerFreq.unk70 >= 0 && g_PowerFreq.unk70 != g_PowerFreq.unk68) //0x00003CD4 & 0x00003CE0
                sceSysconCtrlVoltage(1, g_PowerFreq.unk70); //0x00003CE8
            g_PowerFreq.unk64 = 0; //0x00003CF8
        }
    }
    s32 tmpFrequency = ((s32)((frequency >> 31) + frequency)) >> 1; //	0x00003C68
    if (tmpFrequency == busFrequency) { //0x00003C6C
        sceClkcSetBusGear(511, 511); //0x00003CA4
            
        float busFrequencyFloat = sceClkcGetBusFrequency(); //0x00003CAC
        g_PowerFreq.clkcBusFrequencyFloat = busFrequencyFloat; //0x00003CB8
        g_PowerFreq.clkcBusFrequencyInt = (u32)busFrequencyFloat; //0x00003CC0
            
        scePowerSetGeEdramRefreshMode(scePowerGetGeEdramRefreshMode()); //0x00003948
    } else {
        scePowerSetBusClockFrequency(busFrequency); //0x00003C74
    }
    scePowerSetCpuClockFrequency(cpuFrequency); //0x00003C7C
        
    scePowerUnlockForKernel(0); //0x00003C84
    sceKernelUnlockMutex(g_PowerFreq.mutexId, 1); //0x00003C90
    pspSetK1(oldK1); //0x00003C98
    return SCE_ERROR_OK;
}

//Subroutine scePower_driver_100A77A1 - Address 0x00003E64
u32 scePowerSetGeEdramRefreshMode(u32 geEdramRefreshMode)
{
    s32 intrState;
    s64 res;
    s32 resLow;
    s32 sign;
    s32 resHi;
    
    u32 unk1;
    u32 unk2;
    
    intrState = sceKernelCpuSuspendIntr(); //0x00003E78
    
    res = 1; //0x00003E88
    unk1 = 8;
    unk2 = 6; //0x00003E84
    if (geEdramRefreshMode == 1) { //0x00003E8C
        if (g_PowerFreq.clkcBusFrequencyInt < 75) { //0x00003EA0
            if (g_PowerFreq.clkcBusFrequencyInt < 50) { //0x00003F0C
                if (g_PowerFreq.clkcBusFrequencyInt < 25) { //0x00003F40
                    res = g_PowerFreq.clkcBusFrequencyInt * 32768; //0x00003F84
                    unk2 = 1; //0x00003F90
                    unk1 = 6; //0x00003F94
                    
                    resLow = (((s32)res) - 75) * (-1080021015); //0x00003FA0
                    sign = (resLow >> 31); //0x00003FA4
                    resHi = resLow + ((res >> 32) & 0xFFFFFFFF); //0x00003FAC
                    resHi = (resHi >> 7); //0x00003FB4
                } else { //0x00003F48
                    res = g_PowerFreq.clkcBusFrequencyInt * 32768; //0x00003F4C
                    unk2 = 2; //0x00003F58
                    unk1 = 3;
                    
                    resLow = (((s32)res) - 75) * (-1080021015); //0x00003F68
                    sign = (resLow >> 31); //0x00003F6C
                    resHi = resLow + ((res >> 32) & 0xFFFFFFFF); //0x00003F74
                    resHi = (resHi >> 8); //0x00003F78
                }
                res = resHi - sign; //0x00003F80
            } else { //0x00003F14
                res = g_PowerFreq.clkcBusFrequencyInt * 32768; //0x00003F18
                unk2 = 3; //0x00003F1C
                unk1 = 2;
                    
                resLow = ((s32)res) - 75; //0x00003F28
                resHi = ((resLow >> 31) << 23) + resLow; //0x00003F34
                res = (resHi >> 9); //0x00003F3C   
            }
        } else { //0x00003EA8
            res = g_PowerFreq.clkcBusFrequencyInt * 32768; //0x00003EAC
            unk1 = 1; //0x00003EB0
                    
            resLow = ((s32)res) - 75; //0x00003EB8
            resHi = ((resLow >> 31) << 22) + resLow; //0x00003EC4
            res = (resHi >> 10); //0x00003EC8
        }
        res = sceGeEdramSetRefreshParam(geEdramRefreshMode, res, unk1, unk2); //0x00003ECC
        
        sceKernelCpuResumeIntr(intrState); //0x00003ED8
        
        if (res < SCE_ERROR_OK) //0x00003EE0
            return res;
        g_PowerFreq.geEdramRefreshMode = geEdramRefreshMode; //0x00003EEC
        return SCE_ERROR_OK;
    }
}

//Subroutine scePower_driver_C520F5DC - Address 0x00003FB8
u32 scePowerGetGeEdramRefreshMode(void)
{
    return g_PowerFreq.geEdramRefreshMode;
}

//sub_00003FC4
static u32 _scePowerFreqEnd(void)
{
    sceKernelDeleteMutex(g_PowerFreq.mutexId); //0x00003FD4
    return SCE_ERROR_OK;
}

//sub_00003FEC
static u32 sub_00003FEC(void)
{
   return sceKernelLockMutex(g_PowerFreq.mutexId, 1, NULL); //0x00004000
}

//sub_00004014
static u32 sub_00004014(void)
{
    return sceKernelUnlockMutex(g_PowerFreq.mutexId, 1); //0x00004024
}

//sub_00004038
static u32 _scePowerFreqRebootPhase(u32 arg0)
{
    switch (arg0) {
    case 1: //0x00004044
        sceKernelUnlockMutex(g_PowerFreq.mutexId, 1); //0x00004080
        return SCE_ERROR_OK;
    case 2: //0x00004050
        sceKernelLockMutex(g_PowerFreq.mutexId, 1, NULL); //0x0000406C
        return SCE_ERROR_OK;
    default:
        return SCE_ERROR_OK;
    }
}

//sub_00004090
static u32 _scePowerFreqSuspend(void)
{
    sceClkcGetCpuGear(&g_PowerFreq.clkcCpuGearNumerator, &g_PowerFreq.clkcCpuGearDenominator); //0x000040A8
    sceClkcGetBusGear(&g_PowerFreq.clkcBusGearNumerator, &g_PowerFreq.clkcBusGearDenominator); //0x000040B4
    
    g_PowerFreq.oldGeEdramRefreshMode = g_PowerFreq.geEdramRefreshMode; //0x000040CC
    scePowerSetGeEdramRefreshMode(g_PowerFreq.geEdramRefreshMode); //0x000040C8
    return SCE_ERROR_OK;
}

//sub_000040E4
static u32 _scePowerFreqResume(u32 arg0)
{
    switch (arg0) {
    case 0: //0x000040F4
        sceClkcSetCpuGear(g_PowerFreq.clkcCpuGearNumerator, g_PowerFreq.clkcCpuGearDenominator); //0x00004130
        sceClkcSetBusGear(g_PowerFreq.clkcBusGearNumerator, g_PowerFreq.clkcBusGearDenominator); //0x0000413C
        return SCE_ERROR_OK;
    case 1: //0x00004100
        scePowerSetGeEdramRefreshMode(g_PowerFreq.oldGeEdramRefreshMode); //0x0000411C
        return SCE_ERROR_OK;
    default:
        return SCE_ERROR_OK;
    }
}

//Subroutine scePower_driver_D7DD9D38 - Address 0x0000414C 
s16 scePowerGetCurrentTachyonVoltage(void)
{
    if (g_PowerFreq.unk56 == 0) //0x00004158
        return g_PowerFreq.unk62; //0x00004164
    return g_PowerFreq.tachyonVoltage; //0x0000416C
}

//Subroutine scePower_driver_BADA8332 - Address 0x00004170
u32 scePowerGetTachyonVoltage(u32 *arg0, u32 *arg1)
{
    if (arg0 != NULL)
        *arg0 = g_PowerFreq.tachyonVoltage;
    if (arg1 != NULL)
        *arg1 = g_PowerFreq.unk62;
    
    return SCE_ERROR_OK;
}

//Subroutine scePower_driver_12F8302D - Address 0x00004198
u32 scePowerSetTachyonVoltage(s32 arg0, s32 arg1)
{
    if (arg0 != -1) //0x0000419C
        g_PowerFreq.tachyonVoltage = arg0;
    if (arg1 != -1) //0x000041A8
        g_PowerFreq.unk62 = arg1;
    
    return SCE_ERROR_OK;
}

//Subroutine scePower_driver_9127E5B2 - Address 0x000041BC
u32 scePowerGetCurrentDdrVoltage(void)
{
    if (g_PowerFreq.unk64 != 0) //0x000041C8
        return g_PowerFreq.unk68;
    return g_PowerFreq.unk70; //0x000041DC
}

//Subroutine scePower_driver_75906F9A - Address 0x000041E0 
u32 scePowerGetDdrVoltage(s32 *arg0, s32 *arg1)
{
    if (arg0 != NULL) //0x000041E0
        *arg0 = g_PowerFreq.unk68;
    if (arg1 != NULL) //0x000041F0
        *arg1 = g_PowerFreq.unk70;
    
    return SCE_ERROR_OK;
}

//Subroutine scePower_driver_018AB235 - Address 0x00004208 
u32 scePowerSetDdrVoltage(s32 arg0, s32 arg1)
{
    if (arg0 != -1) //0x0000420C
        g_PowerFreq.unk68 = arg0;
    if (arg1 != -1) //0x00004218
        g_PowerFreq.unk70 = arg1;
    
    return SCE_ERROR_OK;
}

//Subroutine scePower_driver_0655D7C3 - Address 0x0000422C 
u32 scePowerGetCurrentDdrStrength()
{
    if (g_PowerFreq.unk72 != 0) //0x00004238
        return g_PowerFreq.unk76;
    return g_PowerFreq.unk78;
}

//Subroutine scePower_driver_16F965C9 - Address 0x00004250 
u32 scePowerGetDdrStrength(s32 *arg0, s32 *arg1) 
{
    if (arg0 != NULL) //0x00004250
        *arg0 = g_PowerFreq.unk76;
    if (arg1 != NULL) //0x00004260
        *arg1 = g_PowerFreq.unk78;
    
    return SCE_ERROR_OK;
}

//Subroutine scePower_driver_D13377F7 - Address 0x00004278
u32 scePowerSetDdrStrength(s32 arg0, s32 arg1)
{
    if (arg0 != -1) //0x0000427C
        g_PowerFreq.unk76 = arg0;
    if (arg1 != -1) //0x00004288
        g_PowerFreq.unk78 = arg1;
}

//Subroutine scePower_driver_DF904CDE - Address 0x0000429C 
u32 scePowerLimitScCpuClock(s32 lowerLimit, s32 upperLimit)
{
    if (lowerLimit != -1) //0x000042A0
        g_PowerFreq.scCpuClockLowerLimit = lowerLimit;
    if (upperLimit != -1) //0x000042AC
        g_PowerFreq.scCpuClockUpperLimit = upperLimit;
    
    return SCE_ERROR_OK;
}

//Subroutine scePower_driver_EEFB2ACF - Address 0x000042C0
u32 scePowerLimitScBusClock(s32 lowerLimit, s32 upperLimit)
{
    if (lowerLimit != -1) //0x000042C4
        g_PowerFreq.scBusClockLowerLimit = lowerLimit;
    if (upperLimit != -1) //0x000042D0
        g_PowerFreq.scBusClockUpperLimit = upperLimit;
    
    return SCE_ERROR_OK;
}

//Subroutine scePower_driver_B7000C75 - Address 0x000042E4
u32 scePowerLimitPllClock(s32 lowerLimit, s32 upperLimit)
{
    if (lowerLimit != -1) //0x000042E8
        g_PowerFreq.pllClockLowerLimit = lowerLimit;
    if (upperLimit != -1) //0x000042F4
        g_PowerFreq.pllClockUpperLimit = upperLimit;
    
    return SCE_ERROR_OK;
}

//Subroutine scePower_driver_13D7CCE4 - Address 0x00004308
u32 scePowerSetPllUseMask(u32 useMask)
{
    g_PowerFreq.pllUseMask = useMask;
    return SCE_ERROR_OK;
}

//Subroutine scePower_FDB5BFE9 - Address 0x00004318 - Aliases: scePower_FEE03A2F, scePower_driver_FDB5BFE9
u32 scePowerGetCpuClockFrequencyInt(void)
{
    return g_PowerFreq.clkcCpuFrequencyInt;
}

//Subroutine scePower_B1A52C83 - Address 0x00004324 - Aliases: scePower_driver_DC4395E2
u32 scePowerGetCpuClockFrequencyFloat(void)
{
    return g_PowerFreq.clkcCpuFrequencyFloat;
}

//Subroutine scePower_478FE6F5 - Address 0x00004330 - Aliases: scePower_BD681969, scePower_driver_04711DFB
u32 scePowerGetBusClockFrequencyInt(void)
{
    return g_PowerFreq.clkcBusFrequencyInt;
}

//Subroutine scePower_9BADB3EB - Address 0x0000433C - Aliases: scePower_driver_1FF8DA3B
u32 scePowerGetBusClockFrequencyFloat(void) 
{
    return g_PowerFreq.clkcBusFrequencyFloat;
}

//Subroutine scePower_34F9C463 - Address 0x00004348 - Aliases: scePower_driver_67BD889B
u32 scePowerGetPllClockFrequencyInt(void)
{
    return g_PowerFreq.pllClockFrequencyInt;
}

//Subroutine scePower_EA382A27 - Address 0x00004354 - Aliases: scePower_driver_BA8CBCBF
u32 scePowerGetPllClockFrequencyFloat(void)
{
    return g_PowerFreq.pllClockFrequencyFloat;
}

//Subroutine scePower_737486F2 - Address 0x00004360
s32 scePowerSetClockFrequencyBefore280(s32 pllFrequency, s32 cpuFrequency, s32 busFrequency) 
{
    if (g_PowerFreq.sm1Ops != NULL)
        sceKernelDelayThread(60000000); //0x000043BC
    return scePowerSetClockFrequency(pllFrequency, cpuFrequency, busFrequency); //0x0000439C
}

//Subroutine scePower_A4E93389 - Address 0x000043CC
s32 scePowerSetClockFrequency280(s32 pllFrequency, s32 cpuFrequency, s32 busFrequency)
{
    return scePowerSetClockFrequency(pllFrequency, cpuFrequency, busFrequency);
}

//Subroutine scePower_545A7F3C - Address 0x000043E8
s32 scePowerSetClockFrequency300(s32 pllFrequency, s32 cpuFrequency, s32 busFrequency)
{
    return scePowerSetClockFrequency(pllFrequency, cpuFrequency, busFrequency);
}

//Subroutine scePower_EBD177D6 - Address 0x00004404 - Aliases: scePower_driver_EBD177D6
s32 scePowerSetClockFrequency350(s32 pllFrequency, s32 cpuFrequency, s32 busFrequency)
{
    return scePowerSetClockFrequency(pllFrequency, cpuFrequency, busFrequency);
}

//Subroutine scePower_469989AD - Address 0x00004420 - Aliases: scePower_driver_469989AD
s32 scePower_469989AD(s32 pllFrequency, s32 cpuFrequency, s32 busFrequency)
{
    if (sceKernelGetModel() != PSP_1000 || (sceKernelDipsw(11) == 1)) //0x0000443C & 0x00004444 & 0x0000444C & 0x00004458
        scePowerSetExclusiveWlan(0); //0x00004488

    return scePowerSetClockFrequency(pllFrequency, cpuFrequency, busFrequency); //0x00004468
}

//Subroutine sub_00004498 - Address 0x00004498
static s32 _scePowerBatteryEnd(void)
{
    u32 outBits;
    
    if (g_Battery.alarmId <= 0) { //0x000044C0
        s32 status = sceKernelPollEventFlag(g_Battery.eventId, 0x200, SCE_KERNEL_EW_CLEAR_PAT | SCE_KERNEL_EW_OR, &outBits); //0x00004554
        outBits = ((s32)status < 0) ? 0 : outBits; //0x00004564
    } else {
        sceKernelCancelAlarm(g_Battery.alarmId); //0x000044C8
        outBits = 0x200; //0x000044D0
        g_Battery.alarmId = -1;//0x000044DC
    }
    sceKernelClearEventFlag(g_Battery.eventId, ~0xF00); //0x000044E8
    sceKernelSetEventFlag(g_Battery.eventId, 0x80000000); //0x000044F4 -- TODO: 0x80000000 == SCE_POWER_CALLBACKARG_POWER_SWITCH?
    
    if (outBits & 0x200) //0x00004504
        sceSysconPermitChargeBattery(); //0x00004544
    
    sceKernelWaitThreadEnd(g_Battery.threadId, 0); //0x00004510
    sceKernelDeleteThread(g_Battery.threadId); //0x00004518
    sceKernelDeleteEventFlag(g_Battery.eventId); //0x00004524
    
    return SCE_ERROR_OK; 
}

// Subroutine sub_00004570 - Address 0x00004570 
static s32 _scePowerBatterySuspend(void)
{
    s32 intrState;
    u32 eventFlagBits;

    eventFlagBits = 0x40000000; // 0x00004594

    intrState = sceKernelCpuSuspendIntr(); // 0x00004590

    if (g_Battery.alarmId > 0) // 0x000045A0
    {
        sceKernelCancelAlarm(g_Battery.alarmId); // 0x000045A8
        g_Battery.alarmId = -1;

        eventFlagBits |= 0x200;
    }

    if (g_Battery.unk20 != 0) // 0x000045D4
    {
        eventFlagBits |= 0x200;
    }

    // 0x000045D0
    if (g_Battery.unk28 != 0)
    {
        g_Battery.unk28 = 0; // 0x000045D8
        eventFlagBits |= 0x400; // 0x000045DC
    }

    sceKernelClearEventFlag(g_Battery.eventId, ~0x2000000); // 0x000045E0
    sceKernelSetEventFlag(g_Battery.eventId, eventFlagBits); //0x000045EC

    sceKernelCpuResumeIntr(intrState);
    return SCE_ERROR_OK;
}

#include <ctrl.h>

// Subroutine sub_0000461C - Address 0x0000461C 
static s32 _scePowerBatteryUpdatePhase0(void *arg0, u32 *arg1)
{
    u32 val1;
    u32 val2;

    val1 = *(u32 *)(arg0 + 36);

    g_Battery.unk92 = -1; // 0x00004644
    g_Battery.unk60 = val1; // 0x0000464C
    g_Battery.unk96 = -1; // 0x00004650
    g_Battery.unk100 = -1; // 0x00004654
    g_Battery.batteryVoltage = -1; // 0x0000465C

    if (val1 & 0x2) // 0x00004658
    {
        g_Battery.batteryAvailabilityStatus = BATTERY_IS_BEING_DETECTED; // 0x0000466C
        if (g_Battery.batteryType == 0) // 0x00004668
        {
            g_Battery.unk68 = *(u32*)(arg0 + 40); // 0x000046A8
            g_Battery.unk72 = *(u32*)(arg0 + 44); // 0x000046B0
            g_Battery.unk84 = *(u32*)(arg0 + 44); // 0x000046B8
            g_Battery.unk88 = -1; // 0x000046C0
            g_Battery.batteryFullCapacity = *(u32*)(arg0 + 48); // 0x000046C8

            // Note: In earlier versions, this was
            // g_Battery.batteryLifePercentage = _scePowerBatteryConvertVoltToRCap();
            g_Battery.batteryLifePercentage = _scePowerBatteryCalcRivisedRcap(); // 0x000046C4 & 0x000046D0
        }

        *arg1 &= ~0x7F; // 0x00004678
        val2 = *arg1 | g_Battery.batteryLifePercentage | 0x80; // 0x00004684
    }
    else
    {
        g_Battery.unk84 = -1; // 0x000046D4
        g_Battery.batteryAvailabilityStatus = BATTERY_NOT_INSTALLED; // 0x000046D8
        g_Battery.unk68 = 0; // 0x000046DC
        g_Battery.unk72 = -1;
        g_Battery.batteryLifePercentage = -1;
        g_Battery.batteryFullCapacity = -1;
        g_Battery.unk88 = -1; // 0x000046EC

        val2 = *arg1 & ~0xFF; // 0x000046F8
    }

    *arg1 = val2; // 0x00004688
    return SCE_ERROR_OK;
}

// Subroutine sub_0x000046FC - Address 0x000046FC
static s32 _scePowerBatteryThread(void)
{

}

// Subroutine sub_00005130 - Address 0x00005130
// Note: Yep, that's the name used in 5.00, might have been corrected since.
static s32 _scePowerBatteryCalcRivisedRcap(void)
{

}

 // Subroutine sub_0000524C - Address 0x0000524C
static s32 _scePowerBatteryConvertVoltToRCap(void)
{

}

// TODO:
// In earlier versions, there was an internal _scePowerGetPolestar2ChargeLed() function here
// This function calls sceSyscon_driver_D8471760 - sceSysconReadPolestarReg 
// perhaps it has been inlined? It is called in _scePowerBatteryThread().

// Subroutine scePower_driver_5F5006D2 - Address 0x000052C8
s32 scePowerGetUsbChargingCapability(void)
{

}

// Subroutine scePower_driver_10CE273F - Address 0x000052D4
s32 scePowerBatteryForbidCharging(void)
{

}

// Subroutine scePower_driver_EF751B4A - Address 0x00005394 
s32 scePowerBatteryPermitCharging(void)
{

}

// Subroutine sub_0000544C - Address 0x0000544C
// TODO: Set param list
static s32 _scePowerBatteryUpdateAcSupply(void)
{

}

// Subroutine scePower_driver_72D1B53A - Address 0x000054E0
s32 scePowerBatteryEnableUsbCharging(void)
{

}

// Subroutine scePower_driver_7EAA4247 - Address 0x00005590
s32 scePowerBatteryDisableUsbCharging(void)
{

}

// Subroutine sub_000056A4 - Address 0x000056A4
// TODO: Check parameter list
static s32 _scePowerBatterySetTTC(void)
{

}

// Subroutine scePower_B4432BC8 - Address 0x00005774 - Aliases: scePower_driver_67492C52
s32 scePowerGetBatteryChargingStatus(void)
{

}

 // Subroutine scePower_78A1A796 - Address 0x0000582C - Aliases: scePower_driver_88C79735
s32 scePowerIsSuspendRequired(void)
{

}

// Subroutine scePower_94F5A53F - Address 0x000058DC - Aliases: scePower_driver_41ADFF48
s32 scePowerGetBatteryRemainCapacity(void)
{

}

// Subroutine scePower_8EFB3FA2 - Address 0x00005910 - Aliases: scePower_driver_C79F9157
s32 scePowerGetBatteryLifeTime(void)
{

}

// Subroutine scePower_28E12023 - Address 0x00005A30 - Aliases: scePower_driver_40870DAC
s32 scePowerGetBatteryTemp(void)
{

}

// Subroutine scePower_862AE1A6 - Address 0x00005A74 - Aliases: scePower_driver_993B8C4A
s32 scePowerGetBatteryElec(void)
{

}

// Subroutine scePower_CB49F5CE - Address 0x00005AD8 - Aliases: scePower_driver_8432901E
s32 scePowerGetBatteryChargeCycle(void)
{

}

// Subroutine sub_00005B1C - Address 0x00005B1C
// TODO: check parameters
static s32 _scePowerBatteryInit(void)
{

}

// Subroutine sub_00005BF0 - Address 0x00005BF0
// TODO: check parameters
static s32 _scePowerBatterySetParam(void)
{

}

// Subroutine sub_00005C08 - Address 0x00005C08
static s32 _scePowerBatteryIsBusy(void)
{

}

// Subroutine sub_00005C18 - Address 0x00005C18
static s32 _scePowerBatteryResume(void)
{

}

// Subroutine scePower_27F3292C - Address 0x00005CCC - Aliases: scePower_driver_0DA940D2
s32 scePowerBatteryUpdateInfo(void)
{
    s32 intrState;

    intrState = sceKernelCpuSuspendIntr(); // 0x00005CDC

    sceKernelSetEventFlag(g_Battery.eventId, 0x10000000); // 0x00005CEC
    sceKernelClearEventFlag(g_Battery.eventId, ~0x10000000); // 0x00005CFC

    sceKernelCpuResumeIntr(intrState); // 0x00005D04
    return SCE_ERROR_OK;
}

// Subroutine scePower_E8E4E204 - Address 0x00005D24 - Aliases: scePower_driver_A641CF3F
s32 scePowerGetForceSuspendCapacity(void)
{
    return (s32)g_Battery.forceSuspendCapacity;
}

// Subroutine scePower_B999184C - Address 0x00005D30 - Aliases: scePower_driver_7B908CAA
s32 scePowerGetLowBatteryCapacity(void)
{
    return (s32)g_Battery.lowBatteryCapacity;
}

// Subroutine scePower_87440F5E - Address 0x00005D3C - Aliases: scePower_driver_872F4ECE
s32 scePowerIsPowerOnline(void)
{
    s32 status;
    s32 oldK1;

    oldK1 = pspShiftK1();

    status = sceSysconIsAcSupplied(); // 0x00005D4C

    pspSetK1(oldK1);
    return (s32)status;
}

// Subroutine scePower_0AFD0D8B - Address 0x00005D68 - Aliases: scePower_driver_8C873AA7
s32 scePowerIsBatteryExist(void)
{
    return (s32)(g_Battery.batteryAvailabilityStatus != BATTERY_NOT_INSTALLED);
}

// Subroutine scePower_1E490401 - Address 0x00005D78 - Aliases: scePower_driver_7A9EA6DE
s32 scePowerIsBatteryCharging(void)
{
    s32 status;

    status = scePowerGetBatteryChargingStatus(); // 0x00005D80

    if (status == 2 || status == 3) // 0x00005D88 & 0x00005D90
    {
        status = SCE_FALSE; // 0x00005D94
    }

    return status;
}

// Subroutine scePower_D3075926 - Address 0x00005DA0 - Aliases: scePower_driver_FA651CE1
s32 scePowerIsLowBattery(void)
{
    s32 status;
    s32 oldK1;

    status = SCE_ERROR_OK;
    oldK1 = pspShiftK1(); // 0x00005DC4

    if (g_Battery.batteryAvailabilityStatus == BATTERY_AVAILABLE) // 0x00005DC0
    {
        status = sceSysconIsLowBattery(); // 0x00005DE0
    }

    pspSetK1(oldK1);
    return status;
}

// Subroutine scePower_driver_071160B1 - Address 0x00005DF0
s32 scePowerGetBatteryType(void)
{
    return (s32)g_Battery.batteryType;
}

// Subroutine scePower_FD18A0FF - Address 0x00005DFC - Aliases: scePower_driver_003B1E03
s32 scePowerGetBatteryFullCapacity(void)
{
    if (g_Battery.batteryAvailabilityStatus == BATTERY_NOT_INSTALLED)
    {
        return SCE_POWER_ERROR_NO_BATTERY;
    }

    if (g_Battery.batteryAvailabilityStatus == BATTERY_IS_BEING_DETECTED)
    {
        return SCE_POWER_ERROR_DETECTING;
    }

    return g_Battery.batteryFullCapacity;
}

// Subroutine scePower_2085D15D - Address 0x00005E30 - Aliases: scePower_driver_31AEA94C
s32 scePowerGetBatteryLifePercent(void)
{
    if (g_Battery.batteryAvailabilityStatus == BATTERY_NOT_INSTALLED)
    {
        return SCE_POWER_ERROR_NO_BATTERY;
    }

    if (g_Battery.batteryAvailabilityStatus == BATTERY_IS_BEING_DETECTED)
    {
        return SCE_POWER_ERROR_DETECTING;
    }

    return g_Battery.batteryLifePercentage;
}

// Subroutine scePower_483CE86B - Address 0x00005E64 - Aliases: scePower_driver_F7DE0E81
s32 scePowerGetBatteryVolt(void)
{
    if (g_Battery.batteryAvailabilityStatus == BATTERY_NOT_INSTALLED)
    {
        return SCE_POWER_ERROR_NO_BATTERY;
    }

    if (g_Battery.batteryAvailabilityStatus == BATTERY_IS_BEING_DETECTED)
    {
        return SCE_POWER_ERROR_DETECTING;
    }

    return g_Battery.batteryVoltage;
}

// Subroutine scePower_23436A4A - Address 0x00005E98 - Aliases: scePower_driver_C730F432
s32 scePowerGetInnerTemp(void)
{
    return SCE_POWER_ERROR_0010;
}

// Subroutine sub_0x00005EA4 - Address 0x00005EA4
static s32 _scePowerBatteryDelayedPermitCharging(void)
{
    g_Battery.alarmId = -1;
    sceKernelSetEventFlag(g_Battery.eventId, 0x200);

    return SCE_ERROR_OK;
}

// Subroutine sub_0x00005ED8 - Address 0x00005ED8
static s32 _scePowerBatterySysconCmdIntr(void)
{
    g_Battery.unk108 = 2;
    return SCE_ERROR_OK;
}