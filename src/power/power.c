/* Copyright (C) The uOFW team
   See the file COPYING for copying permission.
*/

#include <common_imp.h>
#include <display.h>
#include <ge.h>
#include <interruptman.h>
#include <lowio_sysreg.h>
#include <power_kernel.h>
#include <syscon.h>
#include <sysmem_kdebug.h>
#include <sysmem_kernel.h>
#include <sysmem_suspend_kernel.h>
#include <sysmem_sysevent.h>
#include <threadman_kernel.h>

SCE_MODULE_INFO(
    "scePower_Service", 
    SCE_MODULE_KERNEL | 
    SCE_MODULE_ATTR_CANT_STOP | SCE_MODULE_ATTR_EXCLUSIVE_LOAD | SCE_MODULE_ATTR_EXCLUSIVE_START, 
    1, 13);
SCE_MODULE_BOOTSTART("_scePowerModuleStart");
SCE_MODULE_REBOOT_BEFORE("_scePowerModuleRebootBefore");
SCE_MODULE_REBOOT_PHASE("_scePowerModuleRebootPhase");
SCE_SDK_VERSION(SDK_VERSION);

#define PSP_CLOCK_PLL_FREQUENCY_MIN                     19
#define PSP_CLOCK_PLL_FREQUENCY_MAX                     333
#define PSP_1000_CLOCK_PLL_FREQUENCY_MAX_WLAN_ACTIVE    222

#define PSP_CLOCK_CPU_FREQUENCY_MIN                     1
#define PSP_CLOCK_CPU_FREQUENCY_MAX                     333

#define PSP_CLOCK_BUS_FREQUENCY_MIN                     1
#define PSP_CLOCK_BUS_FREQUENCY_MAX                     167

/* Defines Power service specific lower and upper limits for clock speeds. */

#define POWER_PLL_CLOCK_LIMIT_LOWER         1
#define POWER_PLL_CLOCK_LIMIT_UPPER         333

#define POWER_CPU_CLOCK_LIMIT_LOWER         1
#define POWER_CPU_CLOCK_LIMIT_UPPER         333

#define POWER_BUS_CLOCK_LIMIT_LOWER         24
#define POWER_BUS_CLOCK_LIMIT_UPPER         166

/* Defines the fixed set of PLL clock frequencies supported by the power service. */
#define POWER_PLL_OUT_SELECT_SUPPORTED      (SCE_SYSREG_PLL_OUT_SELECT_37MHz | SCE_SYSREG_PLL_OUT_SELECT_148MHz | \
                                             SCE_SYSREG_PLL_OUT_SELECT_190MHz | SCE_SYSREG_PLL_OUT_SELECT_222MHz | \
                                             SCE_SYSREG_PLL_OUT_SELECT_266MHz | SCE_SYSREG_PLL_OUT_SELECT_333MHz | \
                                             SCE_SYSREG_PLL_OUT_SELECT_19MHz | SCE_SYSREG_PLL_OUT_SELECT_74MHz | \
                                             SCE_SYSREG_PLL_OUT_SELECT_96MHz | SCE_SYSREG_PLL_OUT_SELECT_111MHz | \
                                             SCE_SYSREG_PLL_OUT_SELECT_133MHz | SCE_SYSREG_PLL_OUT_SELECT_166MHz)

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

#define SCE_KERNEL_POWER_VOLATILE_MEM_DEFAULT       (0)

/** Cancel all PSP Hardware timers. */
#define SCE_KERNEL_POWER_TICK_DEFAULT               (0)     
/** Cancel auto-suspend-related timer. */
#define SCE_KERNEL_POWER_TICK_SUSPENDONLY           (1)
/** Cancel LCD-related timer */
#define SCE_KERNEL_POWER_TICK_LCDONLY               (6)	

/* Permit Charge delay in microseconds */
#define POWER_DELAY_PERMIT_CHARGING                 (5 * 1000 * 1000)

/* The (initial) priority of the battery worker thread. */
#define POWER_BATTERY_WORKER_THREAD_PRIO            (64)

/* This constant indicates that there are currently no power switch locks in place. */
#define POWER_SWITCH_EVENT_POWER_SWITCH_UNLOCKED        0x00010000
#define POWER_SWITCH_EVENT_00040000                     0x00040000 // TOOD

typedef struct {
    u32 unk0;
    u32 unk4;
    u32 unk8;
    u32 unk12;
} SceSysEventParam;

typedef struct {
    SceUID cbid; // 0
    s32 unk4; // 4
    s32 powerStatus; //8
    s32 mode; // 12 TODO: Perhaps execution mode, synchron = 1, otherwise asynchron? More data needed...
} ScePowerCallback;

typedef struct {
    ScePowerCallback powerCallback[POWER_CALLBACK_TOTAL_SLOTS_KERNEL]; // 0 - 511
    s32 baryonVersion; //512
    u32 unk516; //516 -- power status?
    u32 callbackArgMask; //520
    u8 isBatteryLow; //524
    u8 wlanActivity; //525 -- TODO: Perhaps rename to isWlanActivated?
    u8 watchDog; //526
    u8 isWlanSuppressChargingEnabled; // 527
    s8 unk528;
    s8 wlanExclusiveClockLimit; // 529
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
    s32 numPowerLocksUser; // 20
    s32 numPowerLocksKernel; // 24
    u32 startAddr; //28
    u32 memSize; //32
    u32 unk36; //36
    u32 unk40; //40
    u32 (*unk44)(u32, u32, u32, u32); //44
    u32 unk48; //48 TODO: Perhaps a flag indicating whether locking/unlocking is allowed or - more gnerally - standby/suspension/reboot?
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
    void *pSm1Ops; //4
    u32 pllOutSelect; //8
    s32 pllUseMask; //12
    s32 pllClockFrequencyInt; //16
    s32 cpuClockFrequencyInt; //20
    s32 busClockFrequencyInt; //24
    float pllClockFrequencyFloat; //28
    float cpuClockFrequencyFloat; //32
    float busClockFrequencyFloat; //36
    u32 clkcCpuGearNumerator; //40
    u32 clkcCpuGearDenominator; //44
    u32 clkcBusGearNumerator; //48
    u32 clkcBusGearDenominator; //52
    u32 isTachyonMaxVoltage; // 56
    s16 tachyonMaxVoltage; //60
    s16 tachyonDefaultVoltage; // 62
    u32 isDdrMaxVoltage; // 64
    s16 ddrMaxVoltage; // 68
    s16 ddrDefaultVoltage; // 70
    s32 isDdrMaxStrength; // 72
    s16 ddrMaxStrength; // 76
    s16 ddrDefaultStrength; // 78
    s32 geEdramRefreshMode; //80
    s32 oldGeEdramRefreshMode; //84
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

typedef enum {
    POWER_BATTERY_THREAD_OP_START = 0,
    POWER_BATTERY_THREAD_OP_SET_POWER_SUPPLY_STATUS = 1,
    POWER_BATTERY_THREAD_OP_SET_CHARGE_LED_STATUS = 3,
    POWER_BATTERY_THREAD_OP_SET_USB_STATUS = 4,
    POWER_BATTERY_THREAD_OP_SET_REMAIN_CAP = 5,
    POWER_BATTERY_THREAD_OP_SET_FULL_CAP = 6, 
    POWER_BATTERY_THREAD_OP_SET_CHARGE_CYCLE = 7,
    POWER_BATTERY_THREAD_OP_SET_LIMIT_TIME = 8,
    POWER_BATTERY_THREAD_OP_SET_BATT_TEMP = 9,
    POWER_BATTERY_THREAD_OP_SET_BATT_ELEC = 10,
    POWER_BATTERY_THREAD_OP_SET_BATT_VOLT = 11,
} PowerBatteryThreadOperation;

typedef struct {
    u32 unk0;
    u8 unk4;
    u8 unk5;
    u8 unk6;
    u8 unk7;
} ScePowerSysconSetParamDataTTC; // size: 8

typedef struct {
    u32 eventId; // 0
    u32 workerThreadId; // 4
    u32 forceSuspendCapacity; // 8
    u32 lowBatteryCapacity; // 12
    u32 isIdle; // 16
    u32 unk20;
    u32 isUsbChargingSupported; // 24
    u32 isUsbChargingEnabled; // 28
    u32 unk32;
    u32 permitChargingDelayAlarmId; // 36
    u32 unk40; // 40 TODO: Could have something to do with the status of the charge Led (On/Off)
    u32 batteryType; // 44
    u32 unk48;
    PowerBatteryThreadOperation workerThreadNextOp; // 52
    u32 isAcSupplied; // 56
    u32 powerSupplyStatus; // 60 -- TODO: Define constants for possible values
    ScePowerBatteryAvailabilityStatus batteryAvailabilityStatus; // 64
    u32 unk68;
    s32 batteryRemainingCapacity; // 72
    s32 batteryLifePercentage; // 76
    s32 batteryFullCapacity; // 80
    s32 unk84;
    s32 batteryChargeCycle; // 88
    s32 limitTime; // 92
    s32 batteryTemp; // 96
    s32 batteryElec; // 100
    s32 batteryVoltage; // 104
    u32 unk108;
    SceSysconPacket powerBatterySysconPacket; // 112
    ScePowerSysconSetParamDataTTC unk208;
} ScePowerBattery; //size: 216

typedef struct {
    u32 frequency; // 0
    u32 pllUseMaskBit; // 4
} ScePowerPllConfiguration; //size: 8

enum ScePowerWlanActivity {
    SCE_POWER_WLAN_DEACTIVATED = 0,
    SCE_POWER_WLAN_ACTIVATED = 1,
};

static SceSysconFunc _scePowerAcSupplyCallback; //sub_0x00000650
static SceSysconFunc _scePowerLowBatteryCallback; //sub_0x000006C4
static s32 _scePowerSysEventHandler(s32 eventId, char *eventName, void *param, s32 *result); //sub_0x0000071C
static void _scePowerNotifyCallback(s32 deleteCbFlag, s32 applyCbFlag, s32 arg2); // sub_00000BE0
static s32 _scePowerIsCallbackBusy(u32 cbFlag, SceUID* pCbid); // sub_00000CC4
static s32 _scePowerInitCallback(); //sub_0x0000114C

static s32 _scePowerLock(s32 lockType, s32 isUserLock); // 00001624
static s32 _scePowerUnlock(s32 lockType, s32 isUserLock); // 0x00002E1C

static s32 _scePowerFreqInit(void); // 0x0000353C

static s32 _scePowerSetClockFrequency(s32 pllFrequency, s32 cpuFrequency, s32 busFrequency); // 0x00003898

static s32 _scePowerLockPowerFreqMutex(void); // sub_00003FEC
static s32 _scePowerUnlockPowerFreqMutex(void); // sub_00004014

static s32 _scePowerBatteryEnd(void); // 0x00004498
static s32 _scePowerBatterySuspend(void); // 0x00004570
static s32 _scePowerBatteryUpdatePhase0(void *arg0, u32 *arg1); // 0x0000461C
static s32 _scePowerBatteryThread(SceSize args, void* argp); // 0x000046FC
static inline s32 _scePowerBatteryThreadErrorObtainBattInfo();
static s32 _scePowerBatteryCalcRivisedRcap(void); // 0x00005130
static s32 _scePowerBatteryConvertVoltToRCap(s32 voltage); // 0x00005130
static s32 _scePowerBatteryUpdateAcSupply(s32 enable); // 0x0000544C
static s32 _scePowerBatterySetTTC(s32 arg0); // 0x000056A4
static s32 _scePowerBatteryInit(u32 isUsbChargingSupported, u32 batteryType); // 0x00005B1C
static s32 _scePowerBatterySetParam(s32 forceSuspendCapacity, s32 lowBatteryCapacity); // 0x00005BF0
static s32 _scePowerBatteryIsBusy(void); // 0x00005C08
static s32 _scePowerBatteryResume(void); // 0x00005C18
static s32 _scePowerBatteryDelayedPermitCharging(void* common); // 0x00005EA4
static s32 _scePowerBatterySysconCmdIntr(SceSysconPacket *pSysconPacket, void *param); // 0x00005ED8

const ScePowerHandlers g_PowerHandler = {
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

const SceSysEventHandler g_PowerSysEv = {
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

#define POWER_PLL_CONFIGURATIONS            (sizeof g_pllSettings / sizeof g_pllSettings[0])
const ScePowerPllConfiguration g_pllSettings[] = {
    { .frequency = 19, .pllUseMaskBit = SCE_SYSREG_PLL_OUT_SELECT_19MHz }, /* 0.057 (~ 1/18) */
    { .frequency = 37, .pllUseMaskBit = SCE_SYSREG_PLL_OUT_SELECT_37MHz }, /* 0.111 (1/9) */
    { .frequency = 74, .pllUseMaskBit = SCE_SYSREG_PLL_OUT_SELECT_74MHz }, /* 0.222 (2/9)*/
    { .frequency = 96, .pllUseMaskBit = SCE_SYSREG_PLL_OUT_SELECT_96MHz }, /* 0.286 (~5/18) */
    { .frequency = 111, .pllUseMaskBit = SCE_SYSREG_PLL_OUT_SELECT_111MHz }, /* 0.333 (3/9) */
    { .frequency = 133, .pllUseMaskBit = SCE_SYSREG_PLL_OUT_SELECT_133MHz }, /* 0.4 (7/18) */
    { .frequency = 148, .pllUseMaskBit = SCE_SYSREG_PLL_OUT_SELECT_148MHz }, /* 0.444 (4/9)  */
    { .frequency = 166, .pllUseMaskBit = SCE_SYSREG_PLL_OUT_SELECT_166MHz }, /* 0.5 (9/18) */
    { .frequency = 190, .pllUseMaskBit = SCE_SYSREG_PLL_OUT_SELECT_190MHz }, /* 0.571 (5/9) */
    { .frequency = 222, .pllUseMaskBit = SCE_SYSREG_PLL_OUT_SELECT_222MHz }, /* 0.667 (6/9) */
    { .frequency = 266, .pllUseMaskBit = SCE_SYSREG_PLL_OUT_SELECT_266MHz }, /* 0.8 (7/9) */
    { .frequency = 333, .pllUseMaskBit = SCE_SYSREG_PLL_OUT_SELECT_333MHz }, /* 1 (9/9) */
}; //0x00006F70 -- size 96


ScePower g_Power; //0x00007080
ScePowerSwitch g_PowerSwitch; //0x0000729C
ScePowerIdle g_PowerIdle; //0x0000C400
ScePowerFrequency g_PowerFreq; //0x0000C550
ScePowerBattery g_Battery; //0x0000C5B8

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
        g_Power.wlanExclusiveClockLimit = SCE_POWER_WLAN_EXCLUSIVE_CLOCK_LIMIT_222Mhz;
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
        g_Power.wlanExclusiveClockLimit = ((s8)baryonData[52] < 0) ? baryonData[52] & 0x7F : SCE_POWER_WLAN_EXCLUSIVE_CLOCK_LIMIT_222Mhz; //0x00000098       
        batteryReallyLowCap = BATTERY_REALLY_LOW_CAPACITY_VALUE; //0x00000094
        batteryLowCap = BATTERY_LOW_CAPACITY_VALUE; //0x0000009C
    }
    appType = sceKernelApplicationType(); //0x000000A8
    if (appType == SCE_INIT_APPLICATION_GAME) { //0x000000B4
        g_Power.wlanExclusiveClockLimit = SCE_POWER_WLAN_EXCLUSIVE_CLOCK_LIMIT_222Mhz;
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
            g_Power.wlanExclusiveClockLimit = SCE_POWER_WLAN_EXCLUSIVE_CLOCK_LIMIT_222Mhz; //0x00000508
    }
    else {
        g_Power.wlanExclusiveClockLimit = SCE_POWER_WLAN_EXCLUSIVE_CLOCK_LIMIT_NONE; //0x00000220 || 0x00000234
    }   
    status = sceKernelDipsw(49); //0x00000238
    if (status == 1) { //0x00000244
        g_Power.busInitSpeed = 166;
        g_Power.pllInitSpeed = 333; //0x000004C0
        g_Power.wlanExclusiveClockLimit = SCE_POWER_WLAN_EXCLUSIVE_CLOCK_LIMIT_NONE;
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
                 g_Power.powerCallback[i].unk4 = 0; //0x00000A20
                 g_Power.powerCallback[i].powerStatus = g_Power.unk516; //0x00000A2C
                 g_Power.powerCallback[i].mode  = 0; //0x00000A28
                 
                 sceKernelNotifyCallback(cbid, g_Power.unk516 & g_Power.callbackArgMask); //0x000009B8

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
        g_Power.powerCallback[slot].unk4 = 0; //0x000009A0
        g_Power.powerCallback[slot].powerStatus = g_Power.unk516; //0x000009A8
        g_Power.powerCallback[slot].mode  = 0; //0x00000A28
        
        sceKernelNotifyCallback(cbid, g_Power.unk516 & g_Power.callbackArgMask); //0x000009B8

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
static void _scePowerNotifyCallback(s32 deleteCbFlag, s32 applyCbFlag, s32 arg2)
{
    s32 intrState;
    s32 notifyCount;
    
    intrState = sceKernelCpuSuspendIntr(); //0x00000C0C
    
    // uofw note: Return value not used. 
    sceKernelGetCompiledSdkVersion(); // 0x00000C30

    g_Power.unk516 = (g_Power.unk516 & ~deleteCbFlag) | applyCbFlag; //0x00000C24 & 0x00000C2C & 0x00000C34
    
    u32 i;
    for (i = 0; i < POWER_CALLBACK_TOTAL_SLOTS_KERNEL; i++) {
         if (g_Power.powerCallback[i].cbid < 0) //0x00000C4C
             continue;
         
         notifyCount = sceKernelGetCallbackCount(g_Power.powerCallback[i].cbid); //0x00000C54
         if (notifyCount < 0) //0x00000C5C
             continue;
         
         if (notifyCount == 0) //0x00000C64
             g_Power.powerCallback[i].unk4 = 0;
         
         g_Power.powerCallback[i].unk4 |= arg2; //0x00000C78
         g_Power.powerCallback[i].powerStatus = g_Power.powerCallback[i].unk4 | g_Power.unk516; //0x00000C7C
         sceKernelNotifyCallback(g_Power.powerCallback[i].cbid, 
                                 g_Power.powerCallback[i].powerStatus & g_Power.callbackArgMask); //0x00000C84
    }

    sceKernelCpuResumeIntr(intrState); //0x00000C94
}

//sub_00000CC4
static s32 _scePowerIsCallbackBusy(u32 cbFlag, SceUID *pCbid)
{
    s32 intrState;
    s32 notifyCount;
    
    intrState = sceKernelCpuSuspendIntr(); //0x00000CEC
    
    u32 i;
    for (i = 0; i < POWER_CALLBACK_TOTAL_SLOTS_KERNEL; i++) {
         if (g_Power.powerCallback[i].cbid < 0) //0x00000D04
             continue;
         
         if ((g_Power.powerCallback[i].powerStatus & cbFlag) && g_Power.powerCallback[i].mode != 0) { //0x00000D14 & 0x00000D58
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
   
    if ((g_Power.wlanExclusiveClockLimit == SCE_POWER_WLAN_EXCLUSIVE_CLOCK_LIMIT_222Mhz && pllFreq > 222) 
        || (g_Power.wlanExclusiveClockLimit == SCE_POWER_WLAN_EXCLUSIVE_CLOCK_LIMIT_266Mhz && pllFreq > 266)) //0x00000DBC - 0x00000DEC
    {
        _scePowerUnlockPowerFreqMutex();
        return SCE_POWER_ERROR_BAD_PRECONDITION;
    }

    g_Power.wlanActivity = SCE_POWER_WLAN_ACTIVATED; // 0x00000E1C

    _scePowerUnlockPowerFreqMutex(); //0x00000E18
    
    if (g_Power.isWlanSuppressChargingEnabled) //0x00000E24
        scePowerBatteryForbidCharging(); //0x00000E34
    
    return SCE_ERROR_OK;
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
// TODO: Verify function
s32 scePowerSetExclusiveWlan(u8 clockLimit) 
{
    g_Power.wlanExclusiveClockLimit = clockLimit;
    return SCE_ERROR_OK; //0x00001098
}

//Subroutine scePower_driver_E52B4362 - Address 0x0000109C
// TODO: Verify function
s32 scePowerCheckWlanCondition(u32 freq)
{
    if ((g_Power.wlanExclusiveClockLimit == SCE_POWER_WLAN_EXCLUSIVE_CLOCK_LIMIT_222Mhz && freq > 222) 
        || (g_Power.wlanExclusiveClockLimit == SCE_POWER_WLAN_EXCLUSIVE_CLOCK_LIMIT_266Mhz && freq > 266)) //0x000010A0 - 0x000010D4
        return SCE_POWER_ERROR_BAD_PRECONDITION;
    
    return SCE_ERROR_OK;
}

//Subroutine scePower_driver_8C6BEFD9 - Address 0x000010EC
s32 scePowerWlanDeactivate(void)
{
    g_Power.wlanActivity = SCE_POWER_WLAN_DEACTIVATED; // 0x00001104

    if (g_Power.isWlanSuppressChargingEnabled)
        scePowerBatteryPermitCharging(); // 0x00001118
   
    return SCE_ERROR_OK;
}

//Subroutine scePower_driver_4E32E9B8 - Address 0x00001128
// TODO: Verify function
u8 scePowerGetWatchDog(void)
{
    return g_Power.watchDog; //0x00001130
}

//Subroutine scePower_2B51FE2F - Address 0x00001134 - Aliases: scePower_driver_CE2032CD
// TODO: Verify function
u8 scePowerGetWlanActivity(void)
{
    return g_Power.wlanActivity; //0x0000113C
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

//sub_000011A4
// TODO: Verify function
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
    g_PowerSwitch.numPowerLocksKernel += nPowerLock; //0x000012BC
    
    sceKernelCpuResumeIntr(intrState); //0x000012C0
    return SCE_ERROR_OK;
}

//Subroutine scePower_23C31FFE - Address 0x000010A4 - Aliases: scePower_driver_CE239543
// TODO: Verify function
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
// TODO: Verify function
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
// TODO: Verify function
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
s32 scePowerLockForKernel(s32 lockType)
{
    return _scePowerLock(lockType, SCE_FALSE);
}

//Subroutine scePower_D6D016EF - Address 0x00002BF4
s32 scePowerLockForUser(s32 lockType)
{
    s32 oldK1;
    s32 status;

    oldK1 = pspShiftK1();

    status = _scePowerLock(lockType, SCE_TRUE); //0x00002C08

    pspSetK1(oldK1);
    return status;
}

//sub_00001624
static s32 _scePowerLock(s32 lockType, s32 isUserLock)
{
    s32 intrState;

    /* We only support SCE_KERNEL_POWER_LOCK_DEFAULT currently. */
    if (lockType != SCE_KERNEL_POWER_LOCK_DEFAULT) // 0x00001644
    {
        return SCE_ERROR_INVALID_MODE;
    }

    if (g_PowerSwitch.unk48 != 0) // 0x00001658
    {
        return SCE_ERROR_OK;
    }

    intrState = sceKernelCpuSuspendIntr(); // 0x00001680

    /* 
     * If no power lock currently exists in the system, we need to update the internal power switch 
     * unlocked state.
     */
    if (g_PowerSwitch.numPowerLocksUser == 0 && g_PowerSwitch.numPowerLocksKernel == 0) // 0x0000168C & 0x00001698
    {
        /* 
         * Indicate that power switching is now locked. Attempts to suspend/shutdown the system by the user
         * are now blocked until this lock has been cancelled (assuming no other power locks are in place as well).
         */
        sceKernelClearEventFlag(g_PowerSwitch.eventId, ~POWER_SWITCH_EVENT_POWER_SWITCH_UNLOCKED); // 0x0000170C
    }

    if (isUserLock)
    {
        g_PowerSwitch.numPowerLocksUser++; // 0x000016AC - 0x000016BC

        sceKernelCpuResumeIntr(intrState); // 0x000016B8

        if (g_PowerSwitch.unk16 == 0) // 0x000016C4
        {
            return SCE_ERROR_OK;
        }

        /* 
         * Normally, we return immediately. However, we block the thread when a suspend/standby/reboot process
         * has already been started.
         */
        return sceKernelWaitEventFlag(g_PowerSwitch.eventId, POWER_SWITCH_EVENT_00040000,
            SCE_KERNEL_EW_OR, NULL, NULL); // 0x000016DC
    }
    else
    {
        // loc_000016EC

        g_PowerSwitch.numPowerLocksKernel++; // 0x000016EC - 0x000016FC

        sceKernelCpuResumeIntr(intrState); // 0x000016F8
        return SCE_ERROR_OK;
    }
}

//Subroutine scePower_driver_C3024FE6 - Address 0x00002C68
s32 scePowerUnlockForKernel(s32 lockType)
{
    return _scePowerUnlock(lockType, SCE_FALSE);
}

//Subroutine scePower_CA3D34C1 - Address 0x00002C24
s32 scePowerUnlockForUser(s32 lockType)
{
    s32 oldK1;
    s32 status;

    /* We only support SCE_KERNEL_POWER_LOCK_DEFAULT currently. */
    if (lockType != SCE_KERNEL_POWER_LOCK_DEFAULT) // 0x00002C3C
    {
        return SCE_ERROR_INVALID_MODE;
    }
        
    oldK1 = pspShiftK1();

    status = _scePowerUnlock(lockType, SCE_TRUE); //0x00002C44

    pspSetK1(oldK1);
    return status;
}

//Subroutine sub_00002E1C - Address 0x00002E1C
static s32 _scePowerUnlock(s32 lockType, s32 isUserLock)
{
    s32 intrState;
    s32 numPowerLocks;

    if (g_PowerSwitch.unk48 != 0) // 0x00002E44
    {
        return SCE_ERROR_OK;
    }

    intrState = sceKernelCpuSuspendIntr(); // 0x00002E6C

    /* Reduce the existing power locks (either user or kernel) by one. */

    if (isUserLock) // 0x00002E74
    {
        numPowerLocks = g_PowerSwitch.numPowerLocksUser;
        if (numPowerLocks > 0) // 0x00002E80
        {
            g_PowerSwitch.numPowerLocksUser = --numPowerLocks; // 0x00002E8C
        }
    }
    else
    {
        numPowerLocks = g_PowerSwitch.numPowerLocksKernel;
        if (numPowerLocks > 0) // 0x00002ECC
        {
            g_PowerSwitch.numPowerLocksKernel = --numPowerLocks; // 0x00002EDC
        }
    }

    // loc_00002E90

    /* Check if we can unlock power switching. */
    if (g_PowerSwitch.numPowerLocksUser == 0 && g_PowerSwitch.numPowerLocksKernel == 0) // 0x00002E94 & 0x00002EA0
    {
        /* 
         * No more power locks currently exist in the system, we can now unlock power switching. Any 
         * power switching request (like user attempting to suspend the PSP) which was made while the 
         * lock was in place will now be processed. 
         */
        sceKernelSetEventFlag(g_PowerSwitch.eventId, POWER_SWITCH_EVENT_POWER_SWITCH_UNLOCKED); // 0x00002EB8
    }

    sceKernelCpuResumeIntr(intrState); // 0x00002EA8

    /* Return remaining power locks (>= 0). */
    return numPowerLocks; 
}

//Subroutine scePower_3951AF53 - Address 0x0000171C - Aliases: scePower_driver_3300D85A
// TODO: Verify function
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
// TODO: Verify function
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
    sceKernelWaitEventFlag(g_PowerSwitch.eventId, 0x80000000 | POWER_SWITCH_EVENT_POWER_SWITCH_UNLOCKED, 
        1, &resultBits, NULL); //0x00001880
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
// TODO: Verify function
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
// TODO: Verify function
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
// TODO: Verify function
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
// TODO: Verify function
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
// TODO: Verify function
u32 scePowerSetPowerSwMode(u32 mode)
{
    g_PowerSwitch.mode = mode & 0x3;
    return SCE_ERROR_OK;
}

//Subroutine scePower_165CE085 - Address 0x00002B6C - Aliases: scePower_driver_E11CDFFA
// TODO: Verify function
u32 scePowerGetPowerSwMode(void)
{
    return g_PowerSwitch.mode;
}

//Subroutine scePower_driver_1EC2D4E4 - Address 0x00002B78
// TODO: Verify function
u32 scePowerRebootStart(void)
{
    s32 oldK1;
    s32 intrState;
    
    oldK1 = pspShiftK1(); 
    
    _scePowerLock(SCE_KERNEL_POWER_LOCK_DEFAULT, SCE_TRUE); //0x00002B98
    
    pspSetK1(oldK1); //0x00002BB0
    intrState = sceKernelCpuSuspendIntr(); //0x00002BAC
    
    sceKernelClearEventFlag(g_PowerSwitch.eventId, ~POWER_SWITCH_EVENT_POWER_SWITCH_UNLOCKED); //0x00002BC0
    sceKernelSetEventFlag(g_PowerSwitch.eventId, 0x00040000); //0x00002BCC
    
    sceKernelCpuResumeIntr(intrState); //0x00002BD4
    return SCE_ERROR_OK;
}

//Subroutine scePower_7FA406DD - Address 0x00002C84 - Aliases: scePower_driver_566B8353
// TODO: Verify function
s32 scePowerIsRequest(void)
{
    return (0 < (g_PowerSwitch.unk40 | g_PowerSwitch.unk44));
}

//Subroutine scePower_DB62C9CF - Address 0x00002CA0 - Aliases: scePower_driver_DB62C9CF
// TODO: Verify function
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
// TODO: Verify function
u32 scePowerRequestStandby(void)
{
    s32 oldK1;
    
    oldK1 = pspShiftK1();
    
    sceKernelSetEventFlag(g_PowerSwitch.eventId, 0x10); //0x00002CF8
    
    pspSetK1(oldK1);
    return SCE_ERROR_OK;
}

//Subroutine scePower_AC32C9CC - Address 0x00002D18 - Aliases: scePower_driver_5C1333B7
// TODO: Verify function
u32 scePowerRequestSuspend(void)
{
    s32 oldK1;
    
    oldK1 = pspShiftK1();
    
    sceKernelSetEventFlag(g_PowerSwitch.eventId, 0x20); //0x00002D34
    
    pspSetK1(oldK1);
    return SCE_ERROR_OK;
}

//Subroutine scePower_driver_D79B0122 - Address 0x00002D54
// TODO: Verify function
u32 scePower_driver_D79B0122(void)
{
    return SCE_ERROR_OK;
}

//Subroutine scePower_2875994B - Address 0x00002D5C - Aliases: scePower_driver_D1FFF513
// TODO: Verify function
u32 scePowerRequestSuspendTouchAndGo(void)
{
    s32 oldK1;
    
    oldK1 = pspShiftK1();
    
    sceKernelSetEventFlag(g_PowerSwitch.eventId, 0x40); //0x00002D78
    
    pspSetK1(oldK1);
    return SCE_ERROR_OK;
}

//Subroutine scePower_0442D852 - Address 0x00002D98 - Aliases: scePower_driver_9DAF25A0
// TODO: Verify function
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
// TODO: Verify function
u32 scePowerGetResumeCount(void)
{
    return g_PowerSwitch.resumeCount; 
}

//Subroutine scePower_driver_BA566CD0 - Address 0x00002E0C
// TODO: Verify function
u32 scePowerSetWakeupCondition(u32 wakeUpCondition)
{
    g_PowerSwitch.wakeUpCondition = wakeUpCondition;
    return SCE_ERROR_OK;
}

//0x00002EE0
// TODO: Verify function
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
// TODO: Verify function
static void _scePowerHoldSwCallback(s32 enable, void *argp)
{
    (void)argp;

    if (enable != 0) //0x00002F6C
        _scePowerNotifyCallback(0, SCE_POWER_CALLBACKARG_HOLD_SWITCH, 0); //0x00002F64
    else 
        _scePowerNotifyCallback(SCE_POWER_CALLBACKARG_HOLD_SWITCH, 0, 0); //0x00002F74
}

//Subroutine scePower_EFD3C963 - Address 0x00002F94 - Aliases: scePower_driver_0EFEE60E
// TODO: Verify function
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
// TODO: Verify function
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
// TODO: Verify function
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
// TODO: Verify function
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
// TODO: Verify function
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
// TODO: Verify function
static u32 _scePowerIdleEnd(void)
{
    sceKernelReleaseSubIntrHandler(SCE_VBLANK_INT, 0x1A); //0x00003444
    return SCE_ERROR_OK;
}

//Subroutine scePower_7F30B3B1 - Address 0x000030F0 - Aliases: scePower_driver_E660E488
// TODO: Verify function
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
// TODO: Verify function
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
// TODO: Verify function -- i.e. delete
static u32 GetGp(void)
{
   return pspGetGp();
}

//sub_0000353C
/* Initialize the internal power frequency control block. */
static s32 _scePowerFreqInit(void) 
{
    float pllFrequency;
    float clkcCpuFrequency;
    float clkcBusFrequency;
    s32 tachyonVer;
    u32 fuseConfig;
    
    memset(&g_PowerFreq, 0, sizeof g_PowerFreq); // 0x0000355C
    
    g_PowerFreq.pSm1Ops = sceKernelSm1ReferOperations(); // 0x00003564

    // 0x00003580 - 0x00003598
    /* Set power service specific clock frequency limits.  */
    g_PowerFreq.scBusClockLowerLimit = POWER_BUS_CLOCK_LIMIT_LOWER;
    g_PowerFreq.pllClockLowerLimit = POWER_PLL_CLOCK_LIMIT_LOWER;
    g_PowerFreq.pllClockUpperLimit = POWER_PLL_CLOCK_LIMIT_UPPER;
    g_PowerFreq.scCpuClockLowerLimit = POWER_CPU_CLOCK_LIMIT_LOWER;
    g_PowerFreq.scCpuClockUpperLimit = POWER_CPU_CLOCK_LIMIT_UPPER;
    g_PowerFreq.scBusClockUpperLimit = POWER_BUS_CLOCK_LIMIT_UPPER;
    
    // 0x00003594 - 0x000035A8
    pllFrequency = sceSysregPllGetFrequency();
    g_PowerFreq.pllClockFrequencyFloat = pllFrequency;
    g_PowerFreq.pllClockFrequencyInt = (s32)pllFrequency;
    
    g_PowerFreq.pllOutSelect = sceSysregPllGetOutSelect(); //0x000035A4 & 0x000035B0
    
    // 0x000035AC - 0x000035C0
    clkcCpuFrequency = sceClkcGetCpuFrequency();
    g_PowerFreq.pllClockFrequencyFloat = clkcCpuFrequency;
    g_PowerFreq.pllClockFrequencyInt = (s32)clkcCpuFrequency;
    
    // 0x000035BC - 0x000035DC
    clkcBusFrequency = sceClkcGetBusFrequency();
    g_PowerFreq.busClockFrequencyFloat = clkcBusFrequency;
    g_PowerFreq.busClockFrequencyInt = (s32)clkcBusFrequency;
    
    g_PowerFreq.pllUseMask = POWER_PLL_OUT_SELECT_SUPPORTED; // 0x000035D0
    
    if (g_PowerFreq.pllOutSelect == SCE_SYSREG_PLL_OUT_SELECT_333MHz) // 0x000035E0
    {
        g_PowerFreq.isTachyonMaxVoltage = SCE_TRUE; //0x000036B0
        g_PowerFreq.isDdrMaxVoltage = SCE_TRUE;
        g_PowerFreq.isDdrMaxStrength = SCE_TRUE;
    } 
    else if (g_PowerFreq.pllOutSelect == SCE_SYSREG_PLL_OUT_SELECT_266MHz) // 0x000035EC
    { 
        g_PowerFreq.isTachyonMaxVoltage = SCE_TRUE; // 0x000036A0
        g_PowerFreq.isDdrMaxVoltage = SCE_FALSE;
        g_PowerFreq.isDdrMaxStrength = SCE_FALSE;
    } 
    else 
    {
        g_PowerFreq.isTachyonMaxVoltage = SCE_FALSE; // 0x000035F8
        g_PowerFreq.isDdrMaxVoltage = SCE_FALSE;
        g_PowerFreq.isDdrMaxStrength = SCE_FALSE;
    }

    // 0x00003608 - 0x00003618
    g_PowerFreq.ddrDefaultStrength = -1;
    g_PowerFreq.ddrMaxVoltage = -1;
    g_PowerFreq.ddrDefaultVoltage = -1;
    g_PowerFreq.ddrMaxStrength = -1;
    
    tachyonVer = sceSysregGetTachyonVersion(); // 0x00003614
    if (tachyonVer >= 0x00140000) // 0x00003628
    {
        /* 
         * PSP Motherboard at least [TA-079 v1]. Every retail PSP model released has a Tachyon SoC IC 
         * with at least this version number or higher.
         */

        fuseConfig = (u32)sceSysregGetFuseConfig(); // 0x00003674

        /* 
         * (fuseConfig & 0x3800) >> 3) is at most 0x700 -> unk62 has as its minimum 0xB00 - 0x700 = 0x400.
         * g_PowerFreq.tachyonDefaultVoltage is between [0x400, 0xB00] -> [1024 mV, 2816 mV] 
         * 
         * (~fuseConfig) & 0x700; is at most 0x700. 
         * g_PowerFreq.tachyonMaxVoltage is between [0x0, 0x700] -> [0 mV, 1792 mV]
         * 
         * TODO: Might expand the comment in the future.
        */
        g_PowerFreq.tachyonDefaultVoltage = 0xB00 - ((fuseConfig & 0x3800) >> 3); // 0x0000367C & 0x00003680 & 0x0000368C
        g_PowerFreq.tachyonMaxVoltage = (~fuseConfig) & 0x700; // 0x00003684 & 0x00003690 & 0x0000369C
    } 
    else 
    {
        /* Unknown for which PSP hardware this is the case. */

        g_PowerFreq.tachyonDefaultVoltage = 0x400; //0x00003630
        g_PowerFreq.tachyonMaxVoltage = 0; //0x00003634
    }
    
    scePowerSetGeEdramRefreshMode(1); // 0x0000363C

    g_PowerFreq.mutexId = sceKernelCreateMutex("ScePowerClock", 1, 0, NULL); // 0x00003650

    return SCE_ERROR_OK;
}

//Subroutine scePower_843FBF43 - Address 0x00003350 - Aliases: scePower_driver_BD02C252
s32 scePowerSetCpuClockFrequency(s32 cpuFrequency)
{   
    s32 oldK1;
    s32 intrState;
    s32 actCpuFrequency;

    /* The CPU clock frequency cannot be < 1 or higher than the PLL clock frequency. */
    if (cpuFrequency < PSP_CLOCK_CPU_FREQUENCY_MIN || cpuFrequency > g_PowerFreq.pllClockFrequencyInt) //0x000036E8 & 0x000036F8
    {
        return SCE_ERROR_INVALID_VALUE;
    }

    oldK1 = pspShiftK1(); // 0x00003728
    intrState = sceKernelCpuSuspendIntr(); // 0x00003724

    // 0x00003734 - 0x0000373C
    /* Make sure the CPU frequency is inside its limits. */
    actCpuFrequency = pspMax(g_PowerFreq.scCpuClockLowerLimit, pspMin(cpuFrequency, g_PowerFreq.scCpuClockUpperLimit));

    /* Set the CPU clock frequency. */
    sceClkcSetCpuFrequency((float)actCpuFrequency); // 0x00003748

    /* Obtain the actually set bus clock frequency. */
    float cpuFreqFloat = sceClkcGetCpuFrequency(); // 0x00003750
    g_PowerFreq.cpuClockFrequencyFloat = cpuFreqFloat; // 0x0000375C
    g_PowerFreq.cpuClockFrequencyInt = (s32)cpuFreqFloat; // 0x00003768
    
    sceKernelCpuResumeIntr(intrState); // 0x00003764
    pspSetK1(oldK1); // 0x0000376C

    return SCE_ERROR_OK;
}

//Subroutine scePower_B8D7B3FB - Address 0x00003784 - Aliases: scePower_driver_B71A8B2F
s32 scePowerSetBusClockFrequency(s32 busFrequency)
{
    s32 oldK1;
    s32 intrState;
    s32 status;
    s32 userLevel;
    s32 actBusFrequency;
    s32 halfPllClockFreq;

    // 0x000037B8 - 0x000037C4
    halfPllClockFreq = (g_PowerFreq.pllClockFrequencyInt < 0)
        ? (g_PowerFreq.pllClockFrequencyInt + 1) / 2
        : g_PowerFreq.pllClockFrequencyInt / 2;

    /* The bus clock frequency cannot be < 1 or higher than 1/2 PLL clock frequency. */
    if (busFrequency < PSP_CLOCK_BUS_FREQUENCY_MIN || busFrequency > halfPllClockFreq) // 0x000037B0 & 0x000037CC
    {
        return SCE_ERROR_INVALID_VALUE;
    }

    oldK1 = pspShiftK1(); // 0x000037FC
    intrState = sceKernelCpuSuspendIntr(); // 0x000037F8

    actBusFrequency = busFrequency;

    /* 
     * The bus clock frequency is derived from the PLL clock frequency and set to 1/2 of  
     * the PLL clock frequency. As such, the specified bus clock frequency is ignored.
     * Going forward, the bus clock frequency can no longer be set directly but can only
     * be set indirectly by setting the PLL frequency.
     */
    userLevel = sceKernelGetUserLevel(); // 0x00003800
    if (userLevel < SCE_USER_LEVEL_VSH) // 0x0000380C
    {
        actBusFrequency = halfPllClockFreq; // 0x00003820
    }

    // 0x00003828 - 0x00003830
    /* Make sure the bus frequency is inside its limits. */
    actBusFrequency = pspMax(g_PowerFreq.scBusClockLowerLimit, pspMin(actBusFrequency, g_PowerFreq.scBusClockUpperLimit));

    // 0x00003838 - 0x00003848
    s32 quarterPllClockFreq = (g_PowerFreq.pllClockFrequencyInt < 0)
        ? (g_PowerFreq.pllClockFrequencyInt + 3) / 4
        : g_PowerFreq.pllClockFrequencyInt / 4;

    actBusFrequency = pspMax(actBusFrequency, quarterPllClockFreq); // 0x0000384C

    /* Set the bus clock frequency. */
    status = sceClkcSetBusFrequency((float)actBusFrequency); // 0x00003854

    /* Obtain the actually set bus clock frequency. */
    float busFrequencyFloat = sceClkcGetBusFrequency(); // 0x0000385C
    g_PowerFreq.busClockFrequencyFloat = busFrequencyFloat; // 0x00003868
    g_PowerFreq.busClockFrequencyInt = (s32)busFrequencyFloat; // 0x00003870
    
    /* The Graphic Engine's eDRAM operates at the bus clock speed. Refresh it now. */
    scePowerSetGeEdramRefreshMode(scePowerGetGeEdramRefreshMode()); // 0x0000386C & 0x00003874
    
    sceKernelCpuResumeIntr(intrState); // 0x0000387C
    pspSetK1(oldK1); // 0x00003884

    return status;
}


//Subroutine sub_00003898 - Address 0x00003898 
static s32 _scePowerSetClockFrequency(s32 pllFrequency, s32 cpuFrequency, s32 busFrequency) 
{
    s32 oldK1;
    u32 newPllOutSelect; // $s4
    s32 actPllFrequency; // $s0
    s32 actCpuFrequency; // $s1
    s32 actBusFrequency; // $s2
    s32 status;

    // 0x00003898 - 0x0000391C

    /*
     * The following clock frequency conditions need to be met:
     *      1) PLL clock frequency can only be between [19, 333]
     *      2) CPU clock frequency can only be between [1, 333]
     *      3) Bus clock frequency can only be between [1, 167]
     * 
     * In addition, CPU and bus clock frequencies are derived from the PLL clock frequency.
     * As such, we get the following additional conditions to be met:
     * 
     *      4) The CPU clock frequency cannot be higher than the PLL clock frequency.
     *      5) The bus clock frequency cannot be higher than 1/2 PLL clock frequency.
     */
    if (pllFrequency < PSP_CLOCK_PLL_FREQUENCY_MIN || pllFrequency > PSP_CLOCK_PLL_FREQUENCY_MAX
        || cpuFrequency < PSP_CLOCK_CPU_FREQUENCY_MIN || cpuFrequency > PSP_CLOCK_CPU_FREQUENCY_MAX
        || busFrequency < PSP_CLOCK_BUS_FREQUENCY_MIN || busFrequency > PSP_CLOCK_BUS_FREQUENCY_MAX
        || pllFrequency < cpuFrequency || pllFrequency < 2 * busFrequency)
    {
        return SCE_ERROR_INVALID_VALUE;
    }

    // 0x00003960 - 0x00003970
    /* Adjust the specified PLL clock frequency so that it is in [PllLowerLimit, PllUpperLimit]. */
    actPllFrequency = pspMax(g_PowerFreq.pllClockLowerLimit, pspMin(pllFrequency, g_PowerFreq.pllClockUpperLimit));

    /* 
     * The PLL actually can only operate at a fixed set of clock frequencies. For example, 
     * clock frequencies 96, 133, 233, 266, 333 (in MHz). It can be configured through setting the 
     * [g_PowerFreq.pllUseMask] how many of these clock frequencies will be used to determine
     * the actual PLL clock frequency given the specified input. This works as follows:
     * 
     * Given a clock frequency input f, we scan the PLL clock frequency list (sorted in ascended order)
     * for the first fixed frequency f_fixed, so that f <= f_fixed.
     * 
     * Example: Given a list of fixed frequencies {74, 166, 190, 224, 333} and input 108 MHz,
     * we will actually attempt to set the PLL clock frequency to 166 MHz.
     */
    newPllOutSelect = 0xFFFFFFFF; // 0x00003984
    u32 i;
    for (int i = 0; i < POWER_PLL_CONFIGURATIONS; i++)
    {
        /* Filter out all PLL settings which do not match out PLL use mask. */
        if (!((1 << g_pllSettings[i].pllUseMaskBit) & g_PowerFreq.pllUseMask)) // 0x000039A0
        {
            continue;
        }

        if (g_pllSettings[i].frequency >= actPllFrequency) // 0x000039B0
        {
            actPllFrequency = g_pllSettings[i].frequency; // 0x000039B0
            newPllOutSelect = g_pllSettings[i].pllUseMaskBit; // 0x00003E58
            break;
        }
    }

    // 0x000039C0

    /* 
     * If no fixed PLL clock frequency was found equal to or above the specified frequency,
     * we default to use the highest possible clock frequency -- PSP_CLOCK_PLL_FREQUENCY_MAX (333 MHz).
    */
    if (newPllOutSelect == 0xFFFFFFFF)
    {
        actPllFrequency = PSP_CLOCK_PLL_FREQUENCY_MAX; // 0x000039C4
        newPllOutSelect = SCE_SYSREG_PLL_OUT_SELECT_333MHz; // 0x00003E50
    }

    // 0x000039D4 - 0x000039DC
    actCpuFrequency = pspMax(g_PowerFreq.scCpuClockLowerLimit, pspMin(cpuFrequency, g_PowerFreq.scCpuClockUpperLimit));
    actCpuFrequency = pspMin(actCpuFrequency, actPllFrequency); //0x000039F0
    
    // 0x000039EC - 0x000039F4
    actBusFrequency = pspMax(g_PowerFreq.scBusClockLowerLimit, pspMin(busFrequency, g_PowerFreq.scBusClockUpperLimit));
    
    oldK1 = pspShiftK1(); // 0x00003A10
    
    status = sceKernelLockMutex(g_PowerFreq.mutexId, 1, NULL); //0x00003A18 
    if (status < SCE_ERROR_OK) // //0x00003A20
    {
        pspSetK1(oldK1);
        return status;
    }

    /* WHen WLAN is active, the specified PLL clock frequency might not be applicable. */
    if (scePowerGetWlanActivity() != SCE_POWER_WLAN_DEACTIVATED
        && (status = scePowerCheckWlanCondition(actPllFrequency)) < SCE_ERROR_OK) // 0x00003A28 - 0x00003A44
    {
        /* 
         * The specified PLL clock frequency has been set too high. On SDK versions above 2.00, we return an error.
         * On earlier versions, we "auto-correct" the PLL clock frequency to the highest frequency available when 
         * WLAN is active.
         */
        u32 sdkVersion = sceKernelGetCompiledSdkVersion(); //0x00003E08
        if (sdkVersion > 0x0200000F) // 0x00003E18
        {
            sceKernelUnlockMutex(g_PowerFreq.mutexId, 1); //0x00003DF4

            pspSetK1(oldK1); //0x00003DFC
            return status; //0x00003E04
        }

        // 0x00003E1C - 0x00003E2C
        actPllFrequency = PSP_1000_CLOCK_PLL_FREQUENCY_MAX_WLAN_ACTIVE;
        actCpuFrequency = PSP_1000_CLOCK_PLL_FREQUENCY_MAX_WLAN_ACTIVE;
        actBusFrequency = PSP_1000_CLOCK_PLL_FREQUENCY_MAX_WLAN_ACTIVE / 2;

        newPllOutSelect = SCE_SYSREG_PLL_OUT_SELECT_222MHz;
    }

    status = sceKernelGetUserLevel(); // 0x00003A4C
    if (status < SCE_USER_LEVEL_VSH) // 0x00003A58
    {
        /* 
         * The bus clock frequency is automatically derived from the PLL clock frequency and set to operate at 
         * 1/2 of the PLL clock frequency. 
         */
        actBusFrequency = (((u32)actPllFrequency >> 31) + actPllFrequency) >> 1; // 0x00003A60 - 0x00003A68
    }

    scePowerLockForKernel(SCE_KERNEL_POWER_LOCK_DEFAULT); //0x00003A6C

    /* Check if we need to actually change the speed the PLL clock is operating on. */
    if (g_PowerFreq.pllOutSelect != newPllOutSelect) // 0x00003A78
    {
        /* 
         * The specified PLL clock frequency "does not fit" in the current PLL clock frequency 
         * which is chosen out of a fixed set of frequencies (see comment above). Change the 
         * PLL clock frequency to the fixed clock frequency we've determined above.
         */

        SceSysEventParam sysEventParam; // $sp
        sysEventParam.unk0 = 8;
        sysEventParam.unk4 = actPllFrequency; // 0x00003AA4
        s32 result = 0; // $sp + 16
        SceSysEventHandler *pSysEventBreakHandler = NULL; // $sp + 20

        status = sceKernelSysEventDispatch(SCE_SPEED_CHANGE_EVENTS, 0x01000000, "query", &sysEventParam, 
            &result, 1, &pSysEventBreakHandler); // 0x00003AAC

        if (status < SCE_ERROR_OK) // 0x00003AB4
        {
            sceKernelSysEventDispatch(SCE_SPEED_CHANGE_EVENTS, 0x01000001, "cancel", &sysEventParam, 
                NULL, 0, NULL); //0x00003DE0

            scePowerUnlockForKernel(SCE_KERNEL_POWER_LOCK_DEFAULT); // 0x00003DE8
            sceKernelUnlockMutex(g_PowerFreq.mutexId, 1); // 0x00003DF4

            pspSetK1(oldK1);
            return status;
        }

        /* Increase the current DDR memory voltage if the PLL clock is set to its maximum frequency. */
        if (newPllOutSelect == SCE_SYSREG_PLL_OUT_SELECT_333MHz && !g_PowerFreq.isDdrMaxVoltage) // 0x00003AC0 && 0x00003D88
        {
            if (g_PowerFreq.ddrMaxVoltage >= 0 && g_PowerFreq.ddrDefaultVoltage != g_PowerFreq.ddrMaxVoltage)
            {
                sceSysconCtrlVoltage(3, g_PowerFreq.ddrMaxVoltage); //0x00003DA8
            }

            g_PowerFreq.isDdrMaxVoltage = SCE_TRUE; // 0x00003DB4
        }

        /* 
         * Increase the current Tachyon voltage if the PLL clock is set to operate at a frequency above its
         * default frequency (222 MHz). 
         */
        if ((newPllOutSelect == SCE_SYSREG_PLL_OUT_SELECT_266MHz || newPllOutSelect == SCE_SYSREG_PLL_OUT_SELECT_333MHz) 
            && !g_PowerFreq.isTachyonMaxVoltage && g_PowerFreq.tachyonMaxVoltage >= 0) // 0x00003AD0 & 0x00003AEC
        {
            sceSysconCtrlTachyonVoltage(g_PowerFreq.tachyonMaxVoltage); // 0x00003AF4
            g_PowerFreq.isTachyonMaxVoltage = SCE_TRUE; // 0x00003B00
        }

        /* Increase the current DDR memory strength if the PLL clock is set to its maximum frequency. */
        if (newPllOutSelect == SCE_SYSREG_PLL_OUT_SELECT_333MHz && !g_PowerFreq.isDdrMaxStrength) // 0x00003B08 & 0x00003D50
        {
            if (g_PowerFreq.ddrMaxStrength >= 0 && g_PowerFreq.ddrMaxStrength != g_PowerFreq.ddrDefaultStrength) // 0x00003D5C & 0x00003D68
            {
                // TODO: still not sure whether arguments are supplied here.
                // The ASM for sceDdr_driver_0BAAE4C5() in lowio looks like this:
                //
                // li         $a0, 32
                // sll        $a2, $a1, 5
                // j          sceDdr_driver_77CD1FB3
                // li         $a1, 2
                //
                // So apparently it takes two arguments, with the first argument being overwritten
                // locally. Yet...looking at the two calls in power, the second argument makes no
                // sense. In the second call at address 0x00003D28, $a1 would be 0x01000020 as this
                // is the value stored into it to provide the second argument to the preceding 
                // sceKernelSysEventDispatch() call.
                sceDdr_driver_0BAAE4C5(); // 0x00003D70
            }

            g_PowerFreq.isDdrMaxStrength = SCE_TRUE;
        }

        sceKernelSysEventDispatch(SCE_SPEED_CHANGE_EVENTS, 0x01000002, "start", &sysEventParam, 
            NULL, 0, NULL); // 0x00003B30

        sceDisplayWaitVblankStart(); // 0x00003B38

        s32 intrState = sceKernelCpuSuspendIntr(); // 0x00003B40 -- $s6

        /* 
         * Set the ratio used to derive the CPU clock and bus clock frequencies from the PLL clock frequency.
         * Here, the ratio is 511/511 = 1, so the new clock speeds are __roughly__:
         *   CPU clock frequency = PLL clock frequency * [511/511] = PLL clock frequency
         *   Bus clock frequency = (PLL clock frequency / 2) * [511/511] = 1/2 PLL clock frequency
         */
        sceClkcSetCpuGear(511, 511); // 0x00003B50
        sceClkcSetBusGear(511, 511); // 0x00003B5C

        sceKernelSysEventDispatch(SCE_SPEED_CHANGE_EVENTS, 0x01000010, "phase0", &sysEventParam, 
            NULL, 0, NULL); // 0x00003B84

        if (g_PowerFreq.pSm1Ops != NULL) //0x00003B90
        {
            void (*sm1Op)(s32, s32) = (void (*)(s32, s32))*(u32*)(g_PowerFreq.pSm1Ops + 44);
            sm1Op(newPllOutSelect, -1); // 0x00003D3C
        }

        /* 
         * Actually change the PLL clock frequency now. Set it to the determined fixed frequency
         * (so >= the specified requested frequency).
         */
        sceDdrChangePllClock(newPllOutSelect); // 0x00003B98
        g_PowerFreq.pllOutSelect = newPllOutSelect; // 0x00003BA4

        /* Get the actual PLL clock frequency. */
        float pllFrequency = sceSysregPllGetFrequency(); // 0x00003BA0
        g_PowerFreq.pllClockFrequencyFloat = pllFrequency; // 0x00003BAC
        g_PowerFreq.pllClockFrequencyInt = (s32)pllFrequency; // 0x00003BA8 & 0x00003BB8

        sceKernelSysEventDispatch(SCE_SPEED_CHANGE_EVENTS, 0x01000011, "phase1", &sysEventParam, 
            NULL, 0, NULL); // 0x00003BD4

        sceKernelCpuResumeIntr(intrState); // 0x00003BDC

        sceKernelSysEventDispatch(SCE_SPEED_CHANGE_EVENTS, 0x01000020, "end", &sysEventParam, 
            NULL, 0, NULL); //0x00003C04

        if (newPllOutSelect != SCE_SYSREG_PLL_OUT_SELECT_333MHz && g_PowerFreq.isDdrMaxStrength) // 0x00003C10 & 0x00003C1C
        {
            if (g_PowerFreq.ddrDefaultStrength >= 0 && g_PowerFreq.ddrMaxStrength != g_PowerFreq.ddrDefaultStrength) // 0x00003D14 & 0x00003D20
            {
                // TODO: still not sure whether arguments are supplied here.
                // The ASM for sceDdr_driver_0BAAE4C5() in lowio looks like this:
                //
                // li         $a0, 32
                // sll        $a2, $a1, 5
                // j          sceDdr_driver_77CD1FB3
                // li         $a1, 2
                //
                // So apparently it takes two arguments, with the first argument being overwritten
                // locally. Yet...looking at the two calls in power, the second argument makes no
                // sense. In the second call at address 0x00003D28, $a1 would be 0x01000020 as this
                // is the value stored into it to provide the second argument to the preceding 
                // sceKernelSysEventDispatch() call.
                sceDdr_driver_0BAAE4C5(); // 0x00003D28
            }
            
            g_PowerFreq.isDdrMaxStrength = SCE_FALSE; // 0x00003D34
        }

        /*
         * Reduce the current Tachyon voltage if the PLL clock is now operating at its default frequency (222 MHz).
         */
        if (newPllOutSelect != SCE_SYSREG_PLL_OUT_SELECT_266MHz && newPllOutSelect != SCE_SYSREG_PLL_OUT_SELECT_333MHz 
            && g_PowerFreq.isTachyonMaxVoltage && g_PowerFreq.tachyonDefaultVoltage >= 0) // 0x00003C28 & 0x00003C3C & 0x00003CFC
        {
            sceSysconCtrlTachyonVoltage(g_PowerFreq.tachyonDefaultVoltage); // 0x00003D04
            g_PowerFreq.isTachyonMaxVoltage = SCE_FALSE; // 0x00003D10
        }

        /* Reduce the current DDR memory voltage if the PLL clock is no longer operating at its maximum frequency. */
        if (newPllOutSelect != SCE_SYSREG_PLL_OUT_SELECT_333MHz && g_PowerFreq.isDdrMaxVoltage) // 0x00003C48 & 0x00003C58
        {
            if (g_PowerFreq.ddrDefaultVoltage >= 0 && g_PowerFreq.ddrMaxVoltage != g_PowerFreq.ddrDefaultVoltage) // 0x00003CFC
            {
                // TODO: The first argument might identify the system component for which voltage is to be changed.
                // Here, '3' would mean the DDR memory component. If confirmed, macros should be defined.
                sceSysconCtrlVoltage(3, g_PowerFreq.ddrDefaultVoltage); // 0x00003CE8
            }

            g_PowerFreq.isDdrMaxVoltage = SCE_FALSE; // 0x00003CF8
        }
    }

    // loc_00003C60

    actPllFrequency = (((u32)actPllFrequency >> 31) + actPllFrequency) >> 1; // 0x00003C60 - 0x00003C68
    if (actPllFrequency == actBusFrequency) // 0x00003C6C
    {
        /*
         * Set the ratio used to derive bus clock frequency from the PLL clock frequency.
         * Here, the ratio is 511/511 = 1, so the new clock speeds are __roughly__:
         *   Bus clock frequency = (PLL clock frequency / 2) * [511/511] = 1/2 PLL clock frequency
         */
        sceClkcSetBusGear(511, 511); // 0x00003CA4

        float busFrequencyFloat = sceClkcGetBusFrequency(); // 0x00003CAC
        g_PowerFreq.busClockFrequencyFloat = busFrequencyFloat; // 0x00003CB8
        g_PowerFreq.busClockFrequencyInt = (s32)busFrequencyFloat; // 0x00003CC0

        /* The Graphic Engine's eDRAM operates at the bus clock speed. Refresh it now. */
        scePowerSetGeEdramRefreshMode(scePowerGetGeEdramRefreshMode()); // 0x00003948
    }

    /* Now set the CPU clock frequency. */
    scePowerSetCpuClockFrequency(actCpuFrequency); // 0x00003C7C

    scePowerUnlockForKernel(SCE_KERNEL_POWER_LOCK_DEFAULT); // 0x00003C84
    sceKernelUnlockMutex(g_PowerFreq.mutexId, 1); // 0x00003C90

    pspSetK1(oldK1); // 0x00003C98
    return SCE_ERROR_OK;
}

//Subroutine scePower_driver_100A77A1 - Address 0x00003E64
s32 scePowerSetGeEdramRefreshMode(s32 geEdramRefreshMode)
{
    s32 refreshParam1; // $a1
    s32 refreshParam2; // $a2
    s32 refreshParam3; // $a3
    s32 intrState;
    s32 status;

    intrState = sceKernelCpuSuspendIntr(); // 0x00003E78

    refreshParam1 = 1;
    refreshParam2 = 8;
    refreshParam3 = 6;

    if (geEdramRefreshMode == 0) // 0x00003E8C
    {
        // 0x00003E94
        if (g_PowerFreq.busClockFrequencyInt < 75) // 0x00003EA0
        {
            // loc_00003F0C

            if (g_PowerFreq.busClockFrequencyInt < 50) // 0x00003EA4 & 0x00003F0C
            {
                // loc_00003F40

                if (g_PowerFreq.busClockFrequencyInt < 25) // 0x00003F10 & 0x00003F40
                {
                    // loc_00003F84

                    refreshParam3 = 1; // 0x00003F90
                    refreshParam2 = 6; // 0x00003F94

                    // 0x00003F84 - 0x00003FAC
                    // TODO: What kind of (compiler) arithmetic optimization am I looking at?
                    s32 tmp = ((g_PowerFreq.busClockFrequencyInt * 8000) - 75); // $t4
                    s32 tmp2 = (tmp * 0xBFA02FE9) >> 32; // $t3

                    refreshParam1 = (tmp + tmp2) / 128 - (tmp >> 31);
                }
                else
                {
                    refreshParam3 = 2; // 0x00003F58
                    refreshParam2 = 3; // 0x00003F5C

                    // 0x00003F48 - 0x00003F78
                    // TODO: What kind of (compiler) arithmetic optimization am I looking at?
                    s32 tmp = ((g_PowerFreq.busClockFrequencyInt * 8000) - 75); // $a1
                    s32 tmp2 = (tmp * 0xBFA02FE9) >> 32; // $t9

                    refreshParam1 = (tmp + tmp2) / 256 - (tmp >> 31);
                }
            }
            else
            {
                refreshParam3 = 3; // 0x00003F1C
                refreshParam2 = 2; // 0x00003F20

                // 0x00003F14 - 0x00003F3C
                s32 tmp = ((g_PowerFreq.busClockFrequencyInt * 8000) - 75);
                refreshParam1 = (tmp < 0)
                    ? (tmp + 511) / 512 /* Presumably something to do with overflow */
                    : tmp / 512;
            }
        }
        else
        {
            refreshParam2 = 1; // 0x00003EB0

            // 0x00003EAC - 0x00003EC8
            s32 tmp = ((g_PowerFreq.busClockFrequencyInt * 8000) - 75);
            refreshParam1 = (tmp < 0)
                ? (tmp + 1023) / 1024 /* Presumably something to do with overflow */
                : tmp / 1024;
        }
    }

    // loc_00003ECC

    status = sceGeEdramSetRefreshParam(geEdramRefreshMode, refreshParam1, refreshParam2, refreshParam3); // 0x00003ECC

    sceKernelCpuResumeIntr(intrState); //0x00003ED8

    if (status < SCE_ERROR_OK)
    {
        return status;
    }

    g_PowerFreq.geEdramRefreshMode = geEdramRefreshMode; // 0x00003EEC

    return SCE_ERROR_OK;
}

//Subroutine scePower_driver_C520F5DC - Address 0x00003FB8
s32 scePowerGetGeEdramRefreshMode(void)
{
    return g_PowerFreq.geEdramRefreshMode;
}

//sub_00003FC4
// TODO: Verify function
static u32 _scePowerFreqEnd(void)
{
    sceKernelDeleteMutex(g_PowerFreq.mutexId); //0x00003FD4
    return SCE_ERROR_OK;
}

//sub_00003FEC
static s32 _scePowerLockPowerFreqMutex(void)
{
   return sceKernelLockMutex(g_PowerFreq.mutexId, 1, NULL); //0x00004000
}

//sub_00004014
static s32 _scePowerUnlockPowerFreqMutex(void)
{
    return sceKernelUnlockMutex(g_PowerFreq.mutexId, 1); //0x00004024
}

//sub_00004038
// TODO: Verify function
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
// TODO: Verify function
static u32 _scePowerFreqSuspend(void)
{
    sceClkcGetCpuGear(&g_PowerFreq.clkcCpuGearNumerator, &g_PowerFreq.clkcCpuGearDenominator); //0x000040A8
    sceClkcGetBusGear(&g_PowerFreq.clkcBusGearNumerator, &g_PowerFreq.clkcBusGearDenominator); //0x000040B4
    
    g_PowerFreq.oldGeEdramRefreshMode = g_PowerFreq.geEdramRefreshMode; //0x000040CC
    scePowerSetGeEdramRefreshMode(g_PowerFreq.geEdramRefreshMode); //0x000040C8
    return SCE_ERROR_OK;
}

//sub_000040E4
// TODO: Verify function
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
    return (g_PowerFreq.isTachyonMaxVoltage)
        ? g_PowerFreq.tachyonMaxVoltage
        : g_PowerFreq.tachyonDefaultVoltage;
}

//Subroutine scePower_driver_BADA8332 - Address 0x00004170
s32 scePowerGetTachyonVoltage(s16 *pMaxVoltage, s16 *pDefaultVoltage)
{
    if (pMaxVoltage != NULL)
        *pMaxVoltage = g_PowerFreq.tachyonMaxVoltage;

    if (pDefaultVoltage != NULL)
        *pDefaultVoltage = g_PowerFreq.tachyonDefaultVoltage;
    
    return SCE_ERROR_OK;
}

//Subroutine scePower_driver_12F8302D - Address 0x00004198
s32 scePowerSetTachyonVoltage(s16 maxVoltage, s16 defaultVoltage)
{
    if (maxVoltage != -1) //0x0000419C
        g_PowerFreq.tachyonMaxVoltage = maxVoltage;

    if (defaultVoltage != -1) //0x000041A8
        g_PowerFreq.tachyonDefaultVoltage = defaultVoltage;
    
    return SCE_ERROR_OK;
}

//Subroutine scePower_driver_9127E5B2 - Address 0x000041BC
s16 scePowerGetCurrentDdrVoltage(void)
{
    return (g_PowerFreq.isDdrMaxVoltage)
        ? g_PowerFreq.ddrMaxVoltage
        : g_PowerFreq.ddrDefaultVoltage;
}

//Subroutine scePower_driver_75906F9A - Address 0x000041E0 
s32 scePowerGetDdrVoltage(s16 *pMaxVoltage, s32 *pDefaultVoltage)
{
    if (pMaxVoltage != NULL) //0x000041E0
        *pMaxVoltage = g_PowerFreq.ddrMaxVoltage;

    if (pDefaultVoltage != NULL) //0x000041F0
        *pDefaultVoltage = g_PowerFreq.ddrDefaultVoltage;
    
    return SCE_ERROR_OK;
}

//Subroutine scePower_driver_018AB235 - Address 0x00004208 
s32 scePowerSetDdrVoltage(s16 maxVoltage, s32 defaultVoltage)
{
    if (maxVoltage != -1) //0x0000420C
        g_PowerFreq.ddrMaxVoltage = maxVoltage;

    if (defaultVoltage != -1) //0x00004218
        g_PowerFreq.ddrDefaultVoltage = defaultVoltage;
    
    return SCE_ERROR_OK;
}

//Subroutine scePower_driver_0655D7C3 - Address 0x0000422C 
s16 scePowerGetCurrentDdrStrength(void)
{
    return (g_PowerFreq.isDdrMaxStrength)
        ? g_PowerFreq.ddrMaxStrength
        : g_PowerFreq.ddrDefaultStrength;
}

//Subroutine scePower_driver_16F965C9 - Address 0x00004250 
s32 scePowerGetDdrStrength(s16 *pMaxStrength, s16 *pDefaultStrength) 
{
    if (pMaxStrength != NULL) //0x00004250
        *pMaxStrength = g_PowerFreq.ddrMaxStrength;

    if (pDefaultStrength != NULL) //0x00004260
        *pDefaultStrength = g_PowerFreq.ddrDefaultStrength;
    
    return SCE_ERROR_OK;
}

//Subroutine scePower_driver_D13377F7 - Address 0x00004278
s32 scePowerSetDdrStrength(s16 maxStrength, s16 defaultStrength)
{
    if (maxStrength != -1) // 0x0000427C
        g_PowerFreq.ddrMaxStrength = maxStrength;

    if (defaultStrength != -1) //0x00004288
        g_PowerFreq.ddrDefaultStrength = defaultStrength;

    return SCE_ERROR_OK;
}

//Subroutine scePower_driver_DF904CDE - Address 0x0000429C 
// TODO: Verify function
u32 scePowerLimitScCpuClock(s32 lowerLimit, s32 upperLimit)
{
    if (lowerLimit != -1) //0x000042A0
        g_PowerFreq.scCpuClockLowerLimit = lowerLimit;
    if (upperLimit != -1) //0x000042AC
        g_PowerFreq.scCpuClockUpperLimit = upperLimit;
    
    return SCE_ERROR_OK;
}

//Subroutine scePower_driver_EEFB2ACF - Address 0x000042C0
// TODO: Verify function
u32 scePowerLimitScBusClock(s32 lowerLimit, s32 upperLimit)
{
    if (lowerLimit != -1) //0x000042C4
        g_PowerFreq.scBusClockLowerLimit = lowerLimit;
    if (upperLimit != -1) //0x000042D0
        g_PowerFreq.scBusClockUpperLimit = upperLimit;
    
    return SCE_ERROR_OK;
}

//Subroutine scePower_driver_B7000C75 - Address 0x000042E4
// TODO: Verify function
u32 scePowerLimitPllClock(s32 lowerLimit, s32 upperLimit)
{
    if (lowerLimit != -1) //0x000042E8
        g_PowerFreq.pllClockLowerLimit = lowerLimit;
    if (upperLimit != -1) //0x000042F4
        g_PowerFreq.pllClockUpperLimit = upperLimit;
    
    return SCE_ERROR_OK;
}

//Subroutine scePower_driver_13D7CCE4 - Address 0x00004308
// TODO: Verify function
s32 scePowerSetPllUseMask(s32 useMask)
{
    g_PowerFreq.pllUseMask = useMask;
    return SCE_ERROR_OK;
}

//Subroutine scePower_FDB5BFE9 - Address 0x00004318 - Aliases: scePower_FEE03A2F, scePower_driver_FDB5BFE9
s32 scePowerGetCpuClockFrequencyInt(void)
{
    return g_PowerFreq.cpuClockFrequencyInt;
}

//Subroutine scePower_B1A52C83 - Address 0x00004324 - Aliases: scePower_driver_DC4395E2
float scePowerGetCpuClockFrequencyFloat(void)
{
    return g_PowerFreq.cpuClockFrequencyFloat;
}

//Subroutine scePower_478FE6F5 - Address 0x00004330 - Aliases: scePower_BD681969, scePower_driver_04711DFB
s32 scePowerGetBusClockFrequencyInt(void)
{
    return g_PowerFreq.busClockFrequencyInt;
}

//Subroutine scePower_9BADB3EB - Address 0x0000433C - Aliases: scePower_driver_1FF8DA3B
float scePowerGetBusClockFrequencyFloat(void) 
{
    return g_PowerFreq.busClockFrequencyFloat;
}

//Subroutine scePower_34F9C463 - Address 0x00004348 - Aliases: scePower_driver_67BD889B
s32 scePowerGetPllClockFrequencyInt(void)
{
    return g_PowerFreq.pllClockFrequencyInt;
}

//Subroutine scePower_EA382A27 - Address 0x00004354 - Aliases: scePower_driver_BA8CBCBF
float scePowerGetPllClockFrequencyFloat(void)
{
    return g_PowerFreq.pllClockFrequencyFloat;
}

//Subroutine scePower_737486F2 - Address 0x00004360
s32 scePowerSetClockFrequencyBefore280(s32 pllFrequency, s32 cpuFrequency, s32 busFrequency) 
{
    if (g_PowerFreq.sm1Ops != NULL)
        sceKernelDelayThread(60000000); //0x000043BC

    return _scePowerSetClockFrequency(pllFrequency, cpuFrequency, busFrequency); //0x0000439C
}

//Subroutine scePower_A4E93389 - Address 0x000043CC
s32 scePowerSetClockFrequency280(s32 pllFrequency, s32 cpuFrequency, s32 busFrequency)
{
    return _scePowerSetClockFrequency(pllFrequency, cpuFrequency, busFrequency);
}

//Subroutine scePower_545A7F3C - Address 0x000043E8
s32 scePowerSetClockFrequency300(s32 pllFrequency, s32 cpuFrequency, s32 busFrequency)
{
    return _scePowerSetClockFrequency(pllFrequency, cpuFrequency, busFrequency);
}

//Subroutine scePower_EBD177D6 - Address 0x00004404 - Aliases: scePower_driver_EBD177D6
s32 scePowerSetClockFrequency350(s32 pllFrequency, s32 cpuFrequency, s32 busFrequency)
{
    return _scePowerSetClockFrequency(pllFrequency, cpuFrequency, busFrequency);
}

//Subroutine scePower_469989AD - Address 0x00004420 - Aliases: scePower_driver_469989AD
s32 scePowerSetClockFrequency(s32 pllFrequency, s32 cpuFrequency, s32 busFrequency)
{
    /* 
     * Check if the device calling this function is a PSP-100X or a a development tool (DTP-T1000) configured 
     * to operate as a PSP-100X.
     */
    if (sceKernelGetModel() != PSP_1000
        || (sceKernelDipsw(PSP_DIPSW_BIT_PLL_WLAN_COEXISTENCY_CLOCK) == PSP_DIPSW_PLL_WLAN_COEXISTENCY_CLOCK_333MHz)) //0x0000443C & 0x00004444 & 0x0000444C & 0x00004458
    {
        /* If we run as a PSP-2000 device or later, WLAN can be used without limiting the clock frequencies. */
        scePowerSetExclusiveWlan(0); //0x00004488
    }

    return _scePowerSetClockFrequency(pllFrequency, cpuFrequency, busFrequency); //0x00004468
}

//Subroutine sub_00004498 - Address 0x00004498
// TODO: Verify function
static s32 _scePowerBatteryEnd(void)
{
    u32 outBits;
    
    if (g_Battery.permitChargingDelayAlarmId <= 0) { //0x000044C0
        s32 status = sceKernelPollEventFlag(g_Battery.eventId, 0x200, SCE_KERNEL_EW_CLEAR_PAT | SCE_KERNEL_EW_OR, &outBits); //0x00004554
        outBits = ((s32)status < 0) ? 0 : outBits; //0x00004564
    } else {
        sceKernelCancelAlarm(g_Battery.permitChargingDelayAlarmId); //0x000044C8
        outBits = 0x200; //0x000044D0
        g_Battery.permitChargingDelayAlarmId = -1;//0x000044DC
    }
    sceKernelClearEventFlag(g_Battery.eventId, ~0xF00); //0x000044E8
    sceKernelSetEventFlag(g_Battery.eventId, 0x80000000); //0x000044F4 -- TODO: 0x80000000 == SCE_POWER_CALLBACKARG_POWER_SWITCH?
    
    if (outBits & 0x200) //0x00004504
        sceSysconPermitChargeBattery(); //0x00004544
    
    sceKernelWaitThreadEnd(g_Battery.workerThreadId, 0); //0x00004510
    sceKernelDeleteThread(g_Battery.workerThreadId); //0x00004518
    sceKernelDeleteEventFlag(g_Battery.eventId); //0x00004524
    
    return SCE_ERROR_OK; 
}

// Subroutine sub_00004570 - Address 0x00004570 
// TODO: Verify function
static s32 _scePowerBatterySuspend(void)
{
    s32 intrState;
    u32 eventFlagBits;

    eventFlagBits = 0x40000000; // 0x00004594

    intrState = sceKernelCpuSuspendIntr(); // 0x00004590

    if (g_Battery.permitChargingDelayAlarmId > 0) // 0x000045A0
    {
        sceKernelCancelAlarm(g_Battery.permitChargingDelayAlarmId); // 0x000045A8
        g_Battery.permitChargingDelayAlarmId = -1;

        eventFlagBits |= 0x200;
    }

    if (g_Battery.unk20 != 0) // 0x000045D4
    {
        eventFlagBits |= 0x200;
    }

    // 0x000045D0
    if (g_Battery.isUsbChargingEnabled)
    {
        g_Battery.isUsbChargingEnabled = SCE_FALSE; // 0x000045D8
        eventFlagBits |= 0x400; // 0x000045DC
    }

    sceKernelClearEventFlag(g_Battery.eventId, ~0x2000000); // 0x000045E0
    sceKernelSetEventFlag(g_Battery.eventId, eventFlagBits); //0x000045EC

    sceKernelCpuResumeIntr(intrState);
    return SCE_ERROR_OK;
}

// Subroutine sub_0000461C - Address 0x0000461C 
// TODO: Verify function
static s32 _scePowerBatteryUpdatePhase0(void *arg0, u32 *arg1)
{
    u32 val1;
    u32 val2;

    val1 = *(u32 *)(arg0 + 36);

    g_Battery.limitTime = -1; // 0x00004644
    g_Battery.powerSupplyStatus = val1; // 0x0000464C
    g_Battery.batteryTemp = -1; // 0x00004650
    g_Battery.batteryElec = -1; // 0x00004654
    g_Battery.batteryVoltage = -1; // 0x0000465C

    if (val1 & 0x2) // 0x00004658
    {
        g_Battery.batteryAvailabilityStatus = BATTERY_IS_BEING_DETECTED; // 0x0000466C
        if (g_Battery.batteryType == 0) // 0x00004668
        {
            g_Battery.unk68 = *(u32*)(arg0 + 40); // 0x000046A8
            g_Battery.batteryRemainingCapacity = *(u32*)(arg0 + 44); // 0x000046B0
            g_Battery.unk84 = *(u32*)(arg0 + 44); // 0x000046B8
            g_Battery.batteryChargeCycle = -1; // 0x000046C0
            g_Battery.batteryFullCapacity = *(u32*)(arg0 + 48); // 0x000046C8

            // Note: In earlier versions, this was
            // g_Battery.batteryLifePercentage = _scePowerBatteryConvertVoltToRCap(*(u32*)(arg0 + 44));
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
        g_Battery.batteryRemainingCapacity = -1;
        g_Battery.batteryLifePercentage = -1;
        g_Battery.batteryFullCapacity = -1;
        g_Battery.batteryChargeCycle = -1; // 0x000046EC

        val2 = *arg1 & ~0xFF; // 0x000046F8
    }

    *arg1 = val2; // 0x00004688
    return SCE_ERROR_OK;
}

// Subroutine sub_0x000046FC - Address 0x000046FC
static s32 _scePowerBatteryThread(SceSize args, void *argp)
{
    s32 isUsbChargingEnabled;
    s32 timeout; // $sp + 4
    s32 batteryEventCheckFlags; // $s2

    u32 batteryEventFlag; // $sp

    g_Battery.unk48 = 1; // 0x00004718
    g_Battery.isIdle = SCE_FALSE; // 0x0000471C
    g_Battery.workerThreadNextOp = POWER_BATTERY_THREAD_OP_START; // 0x00004720

    batteryEventCheckFlags = 0x40000700; // 0x00004728
    timeout = 0; // 0x0000473C

    isUsbChargingEnabled = scePowerGetUsbChargingCapability(); // 0x00004738
    if (isUsbChargingEnabled) // 0x00004740
    {
        u8 version = _sceSysconGetBaryonVersion(); // 0x000050CC
        if (PSP_SYSCON_BARYON_GET_VERSION_MAJOR(version) == 2 &&
            PSP_SYSCON_BARYON_GET_VERSION_MINOR(version) >= 2 && SP_SYSCON_BARYON_GET_VERSION_MINOR(version) < 6) // 0x000050D0 - 0x00005118
        {
            /* We are running on a PSP-2000 series model. */
            sceSysconReceiveSetParam(4, &g_Battery.unk208); // 0x00005120
        }

        _scePowerBatterySetTTC(1); // 0x000050E4
    }

    s32 status; // $a0 in loc_000047AC
    for (;;)
    {
        batteryEventFlag = 0; // 0x0000474C
        if (timeout == 0) // 0x0000474C
        {
            sceKernelPollEventFlag(g_Battery.eventId, batteryEventCheckFlags /* 0x40000700 */ | 0x80000000, SCE_KERNEL_EW_OR, &batteryEventFlag); // 0x000050B4

            status = SCE_ERROR_OK; // 0x000050C0
        }
        else
        {
            s32* pTimeout;
            if (timeout == -1) // 0x00004758
                pTimeout = NULL; // 0x000050A0
            else
                pTimeout = &timeout; // 0x00004760 - 0x00004770

            status = sceKernelWaitEventFlag(g_Battery.eventId, batteryEventCheckFlags | 0x90000000, SCE_KERNEL_EW_OR, &batteryEventFlag, pTimeout); // 0x00004774 & 0x00004788

            if (batteryEventFlag & 0x10000000 && g_Battery.unk48 == 0) // 0x00004788 & 0x00004794
            {
                g_Battery.unk48 = 1; // 0x000047A0
                g_Battery.workerThreadNextOp = POWER_BATTERY_THREAD_OP_START; // 0x000047A4
            }
        }

        if (status == SCE_ERROR_KERNEL_WAIT_TIMEOUT
            || batteryEventFlag & 0x80000000) // 0x000047AC - 0x000047C0 & 0x000047C8
        {
            return SCE_ERROR_OK;
        }

        if (batteryEventFlag & 0x200) // 0x000047D0
        {
            sceSysconPermitChargeBattery();  // 0x00005028

            sceKernelClearEventFlag(g_Battery.eventId, ~0x200); // 0x00005034

            if (!(batteryEventFlag & 0x40000000)) // // 0x00005048
            {
                sceKernelDelayThread(1.5 * 1000 * 1000); // 0x00005054
                continue; // 0x0000505C
            }
        }

        if (batteryEventFlag & 0x40000000) // 0x000047DC
        {
            // loc_00004FF0
            _scePowerBatterySetTTC(1); // 0x00004FF0

            sceKernelClearEventFlag(g_Battery.eventId, ~0x40000000); // 0x00005004

            g_Battery.isIdle = SCE_TRUE; // 0x0000501C
            timeout = -1; // 0x00005024
            batteryEventCheckFlags = 0x60000100; // 0x00005008 & 0x0000500C

            continue; // 0x00005020
        }

        if (batteryEventFlag & 0x20000000) // 0x000047E8
        {
            // loc_00004FC4
            sceKernelClearEventFlag(g_Battery.eventId, ~0x20000000); // 0x00004FD0

            g_Battery.batteryAvailabilityStatus = BATTERY_NOT_INSTALLED; // 0x00004FE0
            g_Battery.workerThreadNextOp = POWER_BATTERY_THREAD_OP_START; // 0x00004FE4
            g_Battery.isIdle = SCE_FALSE; // 0x00004FEC

            batteryEventCheckFlags = 0x40000700; // 0x00004FDC
            timeout = 0; // 0x000048C0

            continue; // 0x000048BC
        }

        if (batteryEventFlag & 0x100) // 0x000047F0
        {
            // loc_00004F9C
            sceSysconForbidChargeBattery(); // 0x00004F9C

            sceKernelClearEventFlag(g_Battery.eventId, ~0x100); // 0x00004FA8

            /* Wait for 1.5 seconds */
            sceKernelDelayThread(1.5 * 1000 * 1000); // 0x00004FB4
        }

        // loc_00004800

        if (g_Battery.isIdle) // 0x00004800
        {
            continue;
        }

        u8 isAcSupplied = sceSysconIsAcSupplied(); // 0x00004808 -- $s3

        if (batteryEventFlag & 0x800) // 0x00004818
        {
            if (isAcSupplied) // 0x00004820
            {
                // loc_00004ECC
                sceSysconSetUSBStatus(4); // 0x00004ECC

                _scePowerBatterySetTTC(1); // 0x00004ED8 & 0x00004EBC
            }
            else if (g_Battery.unk108 == 0) // 0x0000482C
            {
                sceSysconSetUSBStatus(6); // 0x00004EA0

                // TODO: Define constant for 1251 battery full capacity
                _scePowerBatterySetTTC((g_Battery.batteryFullCapacity < 1251) ? 1 : 0); // 0x00004EA8 - 0x00004EBC
            }

            // loc_00004834
            sceKernelClearEventFlag(g_Battery.eventId, ~0x800); // 0x00004838
        }
        else if (batteryEventFlag & 0x400) // 0x00004EE0
        {
            // loc_00004EDC
            g_Battery.unk32 = 0; // 0x00004F78
            sceSysconSetUSBStatus(0); // 0x00004F7C

            _scePowerBatterySetTTC(1); // 0x00004F84

            sceKernelClearEventFlag(g_Battery.eventId, ~0x100); // 0x00004F94 & 0x00004838
        }
        else if (g_Battery.unk108 == 2) // 0x00004EEC
        {
            g_Battery.unk108 = 0; // 0x00004EF0

            // loc_00004F68
            _scePowerBatterySetTTC(1);
        }
        else if (g_Battery.isUsbChargingEnabled && g_Battery.isAcSupplied != isAcSupplied) // 0x00004EF8 & 0x00004F04
        {
            // 0x00004F0C
            if (!isAcSupplied) // 0x00004F0C
            {
                sceSysconSetUSBStatus(6); // 0x00004F48

                // TODO: Define constant for 1251 battery full capacity
                _scePowerBatterySetTTC((g_Battery.batteryFullCapacity < 1251) ? 1 : 0); // 0x00004F50 - 0x00004F64 & 0x00004F20
            }
            else 
            {
                // 0x00004F1C
                sceSysconSetUSBStatus(4);

                _scePowerBatterySetTTC(1); // 0x00004F20
            }
        }
        else if (isAcSupplied && g_Battery.unk32 != 0) // 0x00004F28 & 0x00004F38
        {
            g_Battery.unk32 = 0; // 0x00004F3C
        }

        // loc_00004840

        timeout = 5000000; // 0x00004850

        if (g_Battery.batteryAvailabilityStatus == BATTERY_NOT_INSTALLED) // 0x0000484C
        {
            if (g_Battery.workerThreadNextOp >= 2 && g_Battery.workerThreadNextOp <= 12) // 0x00004854 - 0x00004860
            {
                g_Battery.workerThreadNextOp = POWER_BATTERY_THREAD_OP_START; // 0x00004864
            }
        }
        else if (g_Battery.batteryAvailabilityStatus == BATTERY_IS_BEING_DETECTED) // 0x00004E44
        {
            // loc_00004E94
            timeout = 20000; // 0x00004E9C 

        }
        else if (g_Battery.batteryAvailabilityStatus == BATTERY_AVAILABLE 
            && g_Battery.batteryType == 0) // 0x00004E4C & 0x00004E58
        {
            s32 remainCapacity =_scePowerBatteryCalcRivisedRcap(); // 0x00004E60
            if (remainCapacity != g_Battery.batteryLifePercentage) // 0x00004E6C
            {
                g_Battery.batteryLifePercentage = remainCapacity; // 0x00004E74
                _scePowerNotifyCallback(SCE_POWER_CALLBACKARG_BATTERY_CAP, 
                    remainCapacity | SCE_POWER_CALLBACKARG_BATTERYEXIST, 0); // 0x00004E7C
            }
        }

        // potentially continuing at loc_0000486C / loc_00004870?
        if (g_Battery.unk48 != 0) // 0x00004870
        {
            timeout = 20000; // 0x0000487C
        }

        g_Battery.isAcSupplied = isAcSupplied; // 0x00004884

        switch (g_Battery.workerThreadNextOp) // 0x0000488C & 0x000048A8
        {
            case POWER_BATTERY_THREAD_OP_START:
            {
                g_Battery.workerThreadNextOp = POWER_BATTERY_THREAD_OP_SET_POWER_SUPPLY_STATUS; // 0x000048B4
                timeout = 0; // 0x000048C0

                continue;
            }
            case POWER_BATTERY_THREAD_OP_SET_POWER_SUPPLY_STATUS:
            {
                // 0x000048C4
                s32 powerSupplyStatus;
                status = sceSysconGetPowerSupplyStatus(&powerSupplyStatus); // 0x000048C4
                if (status >= SCE_ERROR_OK) // 0x000048CC
                {
                    g_Battery.powerSupplyStatus = powerSupplyStatus; // 0x000048E4
                    if (!(powerSupplyStatus & 0x2)) // 0x000048E0
                    {
                        _scePowerBatteryThreadErrorObtainBattInfo(); // 00004984 - 0x00004A04

                        g_Battery.unk48 = 0; // 0x00004990
                        g_Battery.workerThreadNextOp = POWER_BATTERY_THREAD_OP_START; // 0x00004998
                    }
                    else
                    {
                        if (g_Battery.batteryAvailabilityStatus == BATTERY_NOT_INSTALLED) // 0x000048EC
                        {
                            g_Battery.batteryAvailabilityStatus == BATTERY_IS_BEING_DETECTED; // 0x000048FC
                            timeout = 20000; // 0x00004900
                        }

                        // loc_00004908

                        if (!g_Battery.isUsbChargingEnabled) // 0x00004908
                        {
                            g_Battery.unk40 = (powerSupplyStatus & 0x80) ? 1 : 0; // 0x0000490C & 0x00004964 - 0x00004970
                            g_Battery.workerThreadNextOp = POWER_BATTERY_THREAD_OP_SET_REMAIN_CAP; // 0x00004980
                        }
                        else
                        {
                            g_Battery.workerThreadNextOp = POWER_BATTERY_THREAD_OP_SET_CHARGE_LED_STATUS; // 0x00004914
                        }
                    }
                }

                // loc_00004918
                //
                // uofw note: I am putting my faith in prxtool here that this will indeed work :p
                s32 cop0CtrlReg30 = pspCop0CtrlGet(COP0_CTRL_30); // 0x00004918 - $s3
                if (cop0CtrlReg30 == 0 && *((s32*)0x00000004) != 0) // 0x0000491C & 0x00004928
                {
                    void* pData = *(s32*)0x00000004;
                    *(s32*)(pData + 4) = 1; // 0x00004930
                    *(s32*)(pData + 8) = 0; // 0x00004934
                }

                s32 cop0CtrlReg31 = pspCop0CtrlGet(COP0_CTRL_31); // 0x00004938 - $s4
                if (cop0CtrlReg31 == 0 && *((s32*)0x00000000) != 0) // 0x0000493C & 0x0000494C
                {
                    void* pData = *(s32*)0x00000000;
                    *(s32*)(pData + 4) = 1; // 0x00004958
                    *(s32*)(pData + 8) = 0; // 0x00004960
                }

                continue; // 0x0000495C
            }
            case POWER_BATTERY_THREAD_OP_SET_CHARGE_LED_STATUS:
            {
                // 0x00004A14
                s16 polestarR2Val;
                status = sceSysconReadPolestarReg(2, &polestarR2Val); // 0x00004A18
                if (status < SCE_ERROR_OK) // 0x00004A20
                {
                    // loc_00004A90
                    g_Battery.unk40 = -1; // 0x00004A9C
                    g_Battery.workerThreadNextOp = POWER_BATTERY_THREAD_OP_SET_REMAIN_CAP; // 0x00004A98 & 0x00004A7C - 0x00004A84
                }
                else
                {
                    // This code is _scePowerGetPolestar2ChargeLed(), so g_Battery.unk40 something with charge Led?
                    g_Battery.unk40 = (polestarR2Val & 0x100) ? 1 : 0; // 0x00004A28 - 0x00004A3C

                    if (polestarR2Val & 0x8000 || g_Battery.powerSupplyStatus & 0x40) // 0x00004A44 & 0x00004A58
                    {
                        // loc_00004A7C
                        g_Battery.workerThreadNextOp = POWER_BATTERY_THREAD_OP_SET_REMAIN_CAP; // 0x00004A84
                    }
                    else
                    {
                        g_Battery.workerThreadNextOp = POWER_BATTERY_THREAD_OP_SET_USB_STATUS; // 0x00004A6C
                        timeout = 5000000; // 0x00004A74
                    }
                }

                continue;
            }
            case POWER_BATTERY_THREAD_OP_SET_USB_STATUS:
            {                
                s32 powerSupplyStatus;
                s16 polestarR2Val;
                // 0x00004AA0 - 0x00004AD4
                if (sceSysconGetPowerSupplyStatus(&powerSupplyStatus) < SCE_ERROR_OK
                    || sceSysconReadPolestarReg(2, &polestarR2Val) < SCE_ERROR_OK
                    || polestarR2Val & 0x8000
                    || powerSupplyStatus & 0x40)
                {
                    g_Battery.workerThreadNextOp = POWER_BATTERY_THREAD_OP_SET_REMAIN_CAP; // 0x00004A84
                    continue;
                }

                g_Battery.unk32 = 1; // 0x00004AE4
                g_Battery.isUsbChargingEnabled = SCE_FALSE; // 0x00004AF0

                sceSysconSetUSBStatus(0); // 0x00004AEC
                _scePowerBatterySetTTC(1); // 0x00004AF4

                continue; // 0x00004AFC
            }
            case POWER_BATTERY_THREAD_OP_SET_REMAIN_CAP:
            {
                // 0x00004B04
                if (g_Battery.batteryType == 0) // 0x00004B0C
                {
                    s32 batStat1;
                    s32 remainCap;
                    status = sceSysconBatteryGetStatusCap(&batStat1, &remainCap); // 0x00004B18
                    if (status < SCE_ERROR_OK) // 0x00004B20
                    {
                        // 0x00004B68 - 0x00004BE0
                        _scePowerBatteryThreadErrorObtainBattInfo();
                    }
                    else
                    {
                        g_Battery.batteryRemainingCapacity = remainCap; // 0x00004B2C
                        g_Battery.unk68 = batStat1; // 0x00004B3C

                        if (g_Battery.unk84 < remainCap) // 0x00004B38
                        {
                            g_Battery.unk84 = remainCap; // 0x00004B4C
                            if (g_Battery.powerSupplyStatus & 0x80)
                            {
                                g_Battery.batteryRemainingCapacity = remainCap--; // 0x00004B54
                            }
                        }

                        // loc_00004B58
                        g_Battery.workerThreadNextOp = POWER_BATTERY_THREAD_OP_SET_FULL_CAP; // 0x00004B60 & 0x00004A84
                    }
                }
                else
                {
                    // loc_00004BF0
                    s32 battVolt;
                    status = sceSysconGetBattVolt(&battVolt); // 0x00004BF0
                    if (status < SCE_ERROR_OK) // 0x00004BF8
                    {
                        // 0x00004C40 - 0x00004BE0
                        _scePowerBatteryThreadErrorObtainBattInfo();
                    }
                    else
                    {
                        if (battVolt != 0) // 0x00004C00
                        {
                            g_Battery.batteryVoltage = battVolt; // 0x00004C08
                            g_Battery.batteryLifePercentage = _scePowerBatteryConvertVoltToRCap(battVolt); // 0x00004C0C & 0x00004C20

                            if (g_Battery.batteryAvailabilityStatus == BATTERY_IS_BEING_DETECTED) // 0x00004C1C
                            {
                                g_Battery.batteryAvailabilityStatus = BATTERY_AVAILABLE; // 0x00004C38
                            }

                            g_Battery.unk48 = 0;
                            g_Battery.workerThreadNextOp = POWER_BATTERY_THREAD_OP_START;
                        }
                    }
                }

                continue;
            }
            case POWER_BATTERY_THREAD_OP_SET_FULL_CAP:
            {
                // 0x00004CAC

                if (g_Battery.batteryFullCapacity < 0) // 0x00004CB8
                {
                    s32 fullCap;
                    status = sceSysconBatteryGetFullCap(&fullCap); // 0x00004CC8
                    if (status < SCE_ERROR_OK) // 0x00004CD0
                    {
                        // 0x00004C40 - 0x00004BE0
                        _scePowerBatteryThreadErrorObtainBattInfo();
                    }
                    else
                    {
                        g_Battery.workerThreadNextOp = POWER_BATTERY_THREAD_OP_SET_CHARGE_CYCLE; // 0x00004CD8
                        g_Battery.batteryFullCapacity = fullCap; // 0x00004CE4
                    }
                }
                else
                {
                    g_Battery.workerThreadNextOp = POWER_BATTERY_THREAD_OP_SET_CHARGE_CYCLE; // 0x00004CC4
                    timeout = 0; // 0x000048BC
                }

                continue;
            }         
            case POWER_BATTERY_THREAD_OP_SET_CHARGE_CYCLE:
            {
                // 0x00004CE8
                if (g_Battery.batteryChargeCycle < 0) // 0x00004CF0
                {
                    // loc_00004D00
                    s32 chargeCycle;
                    status = sceSysconBatteryGetCycle(&chargeCycle); // 0x00004D00
                    if (status < SCE_ERROR_OK) // 0x00004D08
                    {
                        // 0x00004C40 - 0x00004BE0
                        _scePowerBatteryThreadErrorObtainBattInfo();
                    }
                    else
                    {
                        g_Battery.workerThreadNextOp = POWER_BATTERY_THREAD_OP_SET_LIMIT_TIME; // 0x00004D10
                        g_Battery.batteryChargeCycle = chargeCycle; // 0x00004D1C
                    }
                }
                else
                {
                    g_Battery.workerThreadNextOp = POWER_BATTERY_THREAD_OP_SET_LIMIT_TIME; // 0x00004CF8 & 0x00004CC4
                    timeout = 0; // 0x000048C0
                }

                continue;
            }            
            case POWER_BATTERY_THREAD_OP_SET_LIMIT_TIME:
            {
                // 0x00004D20
                u8 isAcSupplied = sceSysconIsAcSupplied(); // 0x00004D20
                if (!isAcSupplied) // 0x00004D28
                {
                    // loc_00004D44
                    s32 limitTime;
                    s32 status = sceSysconBatteryGetLimitTime(&limitTime); // 0x00004D44
                    if (status < SCE_ERROR_OK) // 0x00004D4C
                    {
                        // 0x00004D68 - 0x00004BE0
                        _scePowerBatteryThreadErrorObtainBattInfo();
                    }
                    else
                    {
                        g_Battery.workerThreadNextOp = POWER_BATTERY_THREAD_OP_SET_BATT_TEMP; // 0x00004D58
                        g_Battery.limitTime = limitTime; // 0x00004D64
                    }
                }
                else
                {
                    g_Battery.workerThreadNextOp = POWER_BATTERY_THREAD_OP_SET_BATT_TEMP; // 0x00004D38
                    g_Battery.limitTime = -1; // 0x00004D40
                    
                    timeout = 0; // 0x00004D3C & 0x000048C0
                }

                continue;
            }
            case POWER_BATTERY_THREAD_OP_SET_BATT_TEMP:
            {
                // 0x00004DBC
                s32 battTemp;
                status = sceSysconBatteryGetTemp(&battTemp); // 0x00004DBC
                if (status < SCE_ERROR_OK) // 0x00004DC4
                {
                    // 0x00004D68 - 0x00004BE0
                    _scePowerBatteryThreadErrorObtainBattInfo();
                }
                else
                {
                    g_Battery.workerThreadNextOp = POWER_BATTERY_THREAD_OP_SET_BATT_ELEC; // 0x00004DD0
                    g_Battery.batteryTemp = battTemp; // 0x00004DDC
                }

                continue;
            }                
            case POWER_BATTERY_THREAD_OP_SET_BATT_ELEC:
            {
                // 0x00004DE0
                s32 battElec;
                status = sceSysconBatteryGetElec(&battElec); // 0x00004DE0
                if (status < SCE_ERROR_OK) // 0x00004DE8
                {
                    // 0x00004D68 - 0x00004BE0
                    _scePowerBatteryThreadErrorObtainBattInfo();
                }
                else
                {
                    g_Battery.workerThreadNextOp = POWER_BATTERY_THREAD_OP_SET_BATT_VOLT; // 0x00004DF0
                    g_Battery.batteryElec = battElec; // 0x00004E00
                }

                continue;
            }
            case POWER_BATTERY_THREAD_OP_SET_BATT_VOLT:
            {
                // 0x00004E04
                s32 battVolt;
                status = sceSysconBatteryGetVolt(&battVolt); // 0x00004E04
                if (status < SCE_ERROR_OK) // 0x00004E0C
                {
                    // 0x00004D68 - 0x00004BE0
                    _scePowerBatteryThreadErrorObtainBattInfo();
                }
                else
                {
                    g_Battery.batteryVoltage = battVolt; // 0x00004E24
                    if (g_Battery.batteryAvailabilityStatus == BATTERY_IS_BEING_DETECTED) // 0x00004E20
                    {
                        g_Battery.batteryAvailabilityStatus = BATTERY_AVAILABLE; // 0x00004E3C
                    }

                    g_Battery.unk48 = 0; // 0x00004E28
                    g_Battery.workerThreadNextOp = POWER_BATTERY_THREAD_OP_START; // 0x00004E30
                }

                continue;
            }
            default:
                continue; // 0x0000488C
        }
    }
}

static inline s32 _scePowerBatteryThreadErrorObtainBattInfo()
{
    if (g_Battery.batteryAvailabilityStatus != BATTERY_NOT_INSTALLED) // 0x00004D70
    {
        // 0x00004D78 - 0x00004DB8
        g_Battery.batteryVoltage = -1;
        g_Battery.batteryRemainingCapacity = -1;
        g_Battery.batteryLifePercentage = -1;
        g_Battery.batteryFullCapacity = -1;
        g_Battery.batteryChargeCycle = -1;
        g_Battery.unk84 = -1;
        g_Battery.limitTime = -1;
        g_Battery.batteryTemp = -1;
        g_Battery.batteryElec = -1;
        g_Battery.batteryAvailabilityStatus = BATTERY_NOT_INSTALLED;
        g_Battery.unk40 = 0;
        g_Battery.unk68 = 0;

        _scePowerNotifyCallback(SCE_POWER_CALLBACKARG_BATTERYEXIST, 0, 0); // 0x00004DB4 & 0x00004BB0

        s32 intrState = sceKernelCpuSuspendIntr(); // 0x00004BB8

        sceKernelSetEventFlag(g_Battery.eventId, 0x10000000); // 0x00004BC8
        sceKernelClearEventFlag(g_Battery.eventId, ~0x10000000); // 0x00004BD8

        sceKernelCpuResumeIntr(intrState); // 0x00004BE0
    }
}

// Subroutine sub_00005130 - Address 0x00005130
// Note: Yep, that's the name used in 5.00, might have been corrected since.
static s32 _scePowerBatteryCalcRivisedRcap(void)
{
    s32 fsCap;
    s32 rCap;
    s32 lCap;
    s32 fCap;
    s32 fCapLimit;

    fCap = g_Battery.batteryFullCapacity;
    fCapLimit = fCap * 0.9; // 0x0000513C - 0x00005164
    if (fCapLimit < g_Battery.unk84) // 0x0000516C
    {
        fCapLimit = g_Battery.unk84 * 0.95; // 0x00005174 - 0x00005194
    }

    rCap = g_Battery.batteryRemainingCapacity;
    fsCap = g_Battery.forceSuspendCapacity;
    lCap = g_Battery.lowBatteryCapacity;

    /* 
    * If the remaining battery capacity is below the capacity threshold when the PSP is force suspended,
    * this remaining capacity is not available to the user as a means to run their PSP system. As such, 
    * we return 0 here as the effective relatively remaining capacity.
    */
    if (rCap <= fsCap) // 0x000051A0
    {
        return 0;
    }

    /* Check if the remaining capacity is no larger than the low-battery capacity. */
    if (rCap <= lCap) // 0x000051B0
    {
        /* 
         * When rCap is in (fsCap, lCap] we use the following formula to compute the relative remaining usable cap:
         * 
         *    rCap - fsCap       lCap
         *  ---------------- x --------
         *    lCap - fsCap       fCap
         * 
         * We thus first compute the value of the remaining usable cap relative to the max remaining usable cap 
         * possible here (remember: rCap <= lCap). Once we have this ratio (which is in (0, 1]), we multiply it 
         * with the ratio of lCap to fCup (i.e. lCap is 20% of fCap relatively). As such, our end value is in 
         * the following range:
         * 
         *   (0, lCap / fCap]
         * 
         * In other words, if rCap == lCap, then the remaining usable capacity relative to full capacity is [lCap / fCap].
         * Otherwise, the remaining usable capacity is < [lCap / fCap].
         * 
         * 
         * Why are we using lCap here? If rCap is == lCap, then we get the correct
         * relative value for rCap / fCap (i.e. if lCap=rCap=400mAh and fCap=1800mAh then we return 22mAh).  From there, we 
         * get smaller values until we reach [0.XYZ....]. rCap might still be > fsCap (like rCap=85mAh and fsCap=80mAh) but 
         * at that point the PSP is about to force suspend so the effectively remaining _relative_ capacity for running the PSP 
         * is 0.XYZ which is rounded to 0.
         */

        /*
         * Implementation details: We slightly change the order of operations here to make sure that when we devide two values
         * our numerator is always >= the denominator so we don't have to deal with floating point values in (0, 1). Instead,
         * we only use integer arithmetic here. Below you can see the out commented implementation if floating point arithmetic
         * were to be used.
         */

         //float actUsableRCapMaxUsableRCapRatio = (remainCap - forceSuspendCap) / (lowCap - forceSuspendCap);
         //float actUsableRCapFullCapRatio = actUsableRCapMaxUsableRCapRatio * (lowCap / fullCap);

         ///* Make sure return value is in [0, 100] since we return the relatively remaining capacity in percent. */
         //return pspMax(0, pspMin(actUsableRCapFullCapRatio * 100, 100));

        if ((lCap - fsCap) == 0) // 0x000051C8
        {
            pspBreak(SCE_BREAKCODE_DIVZERO); // 0x000051CC
        }

        u32 relRCap = ((rCap - fsCap) * lCap) / (lCap - fsCap); // 0x000051B8 & 0x000051BC & 0x000051C0 & 0x000051D4
        relRCap = (relRCap * 100) / fCap; // 0x000051DC & 0x000051E4

        return pspMax(0, pspMin(relRCap, 100));
    }
    else
    {
        /*
         * The remaining battery capacity is above the force suspend threshold and larger than the
         * low-battery capacity.
         */

        // TODO: Not exactly sure yet what fCapLimit is exactly about...
        if (fCapLimit == 0) // 0x0000520C
        {
            pspBreak(SCE_BREAKCODE_DIVZERO); // 0x00005210
        }

        u32 relRCap = (rCap * 100) / fCapLimit; // 0x00005208 & 0x00005214 & 0x00005220
        u32 lowerBound = (lCap * 100) / fCap; // 0x00005218 & 0x0000521C & 0x00005228 & 0x0000522C

        // effectively val2 = pspMax(val2, lowerBound);
        if (relRCap < lowerBound) // 0x00005234
        {
            if (fCap == 0) // 0x51000001
            {
                pspBreak(SCE_BREAKCODE_DIVZERO); // 0x00005240
            }
            relRCap = lowerBound; // 0x00005244 & 0x000051E8
        }

        return pspMax(0, pspMin(relRCap, 100)); // 0x00005244 & 0x000051EC & 0x00005204 & 0x000051F4
    }
}

// Subroutine sub_0000524C - Address 0x0000524C
//
// TODO: 
//  - This function has been changed since 5.00. While it still most likely returns the remaining battery capacity
//    its input have changed. As such, this function name might no longer be correct
//  - Define constants for these values
//  - Might these be different values depending on the PSP model used (thus different battery)?
/* Convert battery voltage to remaining battery capacity (relative) */
static s32 _scePowerBatteryConvertVoltToRCap(s32 voltage)
{
    if (voltage <= 0 || voltage < 3100 || voltage < 3300 || voltage < 3400) // 0x0000524C - 0x00005268
    {
        return 0;
    }

    if (voltage < 3500) // 0x00005270
    {
        return 2;
    }

    if (voltage < 3600) // 0x0000527C
    {
        return 4;
    }

    if (voltage < 3700) // 0x00005288
    {
        return 12;
    }

    if (voltage < 3800) // 0x00005294
    {
        return 30;
    }

    if (voltage < 4000) // 0x000052A0
    {
        return 60;
    }

    if (voltage < 4050 || voltage < 4150) // 0x000052AC
    {
        return 90;
    }

    return 100;
}

// Subroutine scePower_driver_5F5006D2 - Address 0x000052C8
s32 scePowerGetUsbChargingCapability(void)
{
    return (s32)g_Battery.isUsbChargingSupported;
}

// Subroutine scePower_driver_10CE273F - Address 0x000052D4
s32 scePowerBatteryForbidCharging(void)
{
    s32 intrState1;
    s32 intrState2;

    intrState1 = sceKernelCpuSuspendIntr();  // 0x000052EC

    if (g_Battery.unk20 == 0) // 0x000052F8
    {
        if (g_Battery.permitChargingDelayAlarmId > 0) // 0x00005304
        {
            sceKernelCancelAlarm(g_Battery.permitChargingDelayAlarmId); // 0x0000530C
            g_Battery.permitChargingDelayAlarmId = 0; // 0x00005318
        }

        sceKernelClearEventFlag(g_Battery.eventId, ~0x200); // 0x00005320
        sceKernelSetEventFlag(g_Battery.eventId, 0x100); // 0x0000532C
    }

    g_Battery.unk20++; // 0x00005344

    intrState2 = sceKernelCpuSuspendIntr(); // 0x00005340

    sceKernelSetEventFlag(g_Battery.eventId, 0x10000000); // 0x00005350
    sceKernelClearEventFlag(g_Battery.eventId, ~0x10000000); // 0x00005360

    sceKernelCpuResumeIntr(intrState2); // 0x00005368
    sceKernelCpuResumeIntr(intrState1); // 0x00005370

    return SCE_ERROR_OK;
}

// Subroutine scePower_driver_EF751B4A - Address 0x00005394 
s32 scePowerBatteryPermitCharging(void)
{
    s32 intrState1;
    s32 intrState2;

    intrState1 = sceKernelCpuSuspendIntr(); // 0x000053AC

    if (g_Battery.unk20 > 0)
    {
        g_Battery.unk20--; // 0x000053CC
        if (g_Battery.unk20 == 0) // 0x000053C8
        {
            sceKernelClearEventFlag(g_Battery.eventId, ~0x100); // 0x00005424
            g_Battery.permitChargingDelayAlarmId = sceKernelSetAlarm(POWER_DELAY_PERMIT_CHARGING, _scePowerBatteryDelayedPermitCharging, NULL); // 0x0000543C
        }
    }

    intrState2 = sceKernelCpuSuspendIntr(); // 0x000053D0

    sceKernelSetEventFlag(g_Battery.eventId, 0x10000000); // 0x000053E0
    sceKernelClearEventFlag(g_Battery.eventId, ~0x10000000); // 0x000053F0

    sceKernelCpuResumeIntr(intrState2); // 0x000053F8
    sceKernelCpuResumeIntr(intrState1); // 0x00005400

    return SCE_ERROR_OK;
}

// Subroutine sub_0000544C - Address 0x0000544C
static s32 _scePowerBatteryUpdateAcSupply(s32 enable)
{
    s32 intrState;

    if (!enable) // 0x00005464
    {
        return SCE_ERROR_OK;
    }

    intrState = sceKernelCpuSuspendIntr(); // 0x00005488

    if (g_Battery.unk20 > 0) // 0x00005494
    {
        if (g_Battery.permitChargingDelayAlarmId > 0) // 0x000054A0
        {
            sceKernelCancelAlarm(g_Battery.permitChargingDelayAlarmId); // 0x000054AC
            g_Battery.permitChargingDelayAlarmId = -1; // 0x000054B4
        }

        sceKernelClearEventFlag(g_Battery.eventId, ~0x200); // 0x000054BC
        sceKernelSetEventFlag(g_Battery.eventId, 0x100);  // 0x000054C8
    }

    sceKernelCpuResumeIntr(intrState);
    return SCE_ERROR_OK;
}

// Subroutine scePower_driver_72D1B53A - Address 0x000054E0
s32 scePowerBatteryEnableUsbCharging(void)
{
    s32 prevIsUsbEnabled;
    s32 intrState;

    if (!g_Battery.isUsbChargingSupported) // 0x00005508
    {
        return SCE_ERROR_NOT_SUPPORTED;
    }

    if (g_Battery.isIdle) // 0x00005514
    {
        return SCE_ERROR_OK;
    }

    intrState = sceKernelCpuSuspendIntr(); // 0x0000553C

    prevIsUsbEnabled = g_Battery.isUsbChargingEnabled;
    if (!g_Battery.isUsbChargingEnabled) // 0x00005548
    {
        g_Battery.isUsbChargingEnabled = SCE_TRUE; // 0x00005568

        if (!sceSysconIsAcSupplied()) // 0x00005564
        {
            sceKernelSetEventFlag(g_Battery.eventId, 0x800); // 0x00005574
            sceKernelClearEventFlag(g_Battery.eventId, ~0x400); // 0x00005580
        }
    }

    sceKernelCpuResumeIntr(intrState); // 0x00005550
    return prevIsUsbEnabled;
}

// Subroutine scePower_driver_7EAA4247 - Address 0x00005590
s32 scePowerBatteryDisableUsbCharging(void)
{
    s32 status;
    s32 intrState1;
    s32 intrState2;

    if (!g_Battery.isUsbChargingSupported) // 0x000055B4
    {
        return SCE_ERROR_NOT_SUPPORTED;
    }

    intrState1 = sceKernelCpuSuspendIntr(); // 0x000055BC

    if (g_Battery.isUsbChargingEnabled) // 0x000055C8
    {
        g_Battery.isUsbChargingEnabled = SCE_FALSE; // 0x000055E8

        if (g_Battery.unk108 == 0) // 0x000055EC
        {
            g_Battery.powerBatterySysconPacket.tx[0] = PSP_SYSCON_CMD_SET_USB_STATUS; // 0x0000567C
            g_Battery.powerBatterySysconPacket.tx[1] = 3; // 0x00005680
            g_Battery.powerBatterySysconPacket.tx[2] = 4; // 0x00005688

            status = sceSysconCmdExecAsync(&g_Battery.powerBatterySysconPacket, 1, _scePowerBatterySysconCmdIntr, NULL); // 0x00005684
            if (status < SCE_ERROR_OK) // 0x0000568C
            {
                sceKernelSetEventFlag(g_Battery.eventId, 0x400); // 0x00005660
            }
            else
            {
                g_Battery.unk108 = 1; // 0x00005698
                g_Battery.unk32 = 0; // 0x000056A0
            }
        }
        else
        {
            sceKernelSetEventFlag(g_Battery.eventId, 0x400); // 0x00005660
        }

        sceKernelClearEventFlag(g_Battery.eventId, ~0x800); // 0x000055FC
    }

    intrState2 = sceKernelCpuSuspendIntr(); // 0x00005604

    sceKernelSetEventFlag(g_Battery.eventId, 0x10000000);  // 0x00005614
    sceKernelClearEventFlag(g_Battery.eventId, ~0x10000000); // 0x00005624

    sceKernelCpuResumeIntr(intrState2); // 0x0000562C
    sceKernelCpuResumeIntr(intrState1); // 0x00005634

    return SCE_ERROR_OK;
}

// Subroutine sub_000056A4 - Address 0x000056A4
static s32 _scePowerBatterySetTTC(s32 arg0)
{
    s32 status;
    s32 intrState;

    if (!scePowerGetUsbChargingCapability()) // 0x000056B8
    {
        return SCE_ERROR_OK;
    }

    // 0x000056D4 - 0x0000571C

    /* Ony proceed if we are running on a PSP with a supported Baryon version (PS-2000 series). */
    u8 version = _sceSysconGetBaryonVersion();
    if (PSP_SYSCON_BARYON_GET_VERSION_MAJOR(version) != 2 &&
        PSP_SYSCON_BARYON_GET_VERSION_MINOR(version) < 2 && SP_SYSCON_BARYON_GET_VERSION_MINOR(version) > 6)
    {
        return SCE_ERROR_OK;
    }

    intrState = sceKernelCpuSuspendIntr(); // 0x00005720

    if (arg0 == 0) // 0x00005734
    {
        g_Battery.unk208.unk7 = (g_Battery.unk208.unk7 & 0xF8) | 0x4; // 0x00005738 & 0x00005768 - 0x00005770, 0x0000574C
    }
    else
    {
        g_Battery.unk208.unk7 = (g_Battery.unk208.unk7 & 0xF8) | 0x5; // 0x00005738 - 0x00005740, 0x0000574C
    }

    sceKernelCpuResumeIntr(intrState); // 0x00005748

    status = sceSysconSendSetParam(4, &g_Battery.unk208); // 0x00005758
    return status;
}

// Subroutine scePower_B4432BC8 - Address 0x00005774 - Aliases: scePower_driver_67492C52
//
// TODO: Figure out meaning behind different batteryChargingStatus values (0 - 3) 
// Looking at scePowerIsBatteryCharging() it appears [1] means battery is charging, and [0, 2, 3] mean 
// battery is not charging
s32 scePowerGetBatteryChargingStatus(void)
{
    s32 oldK1;
    s32 batteryChargingStatus;

    oldK1 = pspShiftK1(); // 0x000057A0

    if (g_Battery.batteryAvailabilityStatus == BATTERY_NOT_INSTALLED) // 0x0000579C
    {
        pspSetK1(oldK1);
        return SCE_POWER_ERROR_NO_BATTERY;
    }

    if (g_Battery.batteryAvailabilityStatus == BATTERY_IS_BEING_DETECTED) // 0x000057AC
    {
        pspSetK1(oldK1);
        return SCE_POWER_ERROR_DETECTING;
    }

    if (sceSysconIsAcSupplied()) // 0x000057BC
    {
        // TODO: Battery charging may be suppressed (the battery is not charging) while the WLAN is in use
        // Could g_Battery.unk20 here be an indicator for WlanActive?

        // Another case is where we are connected to an external power source and the battery is already fully charged.
        // In this case, we report the battery as not charging and this info could be accessed in the else part below (?)

        if (g_Battery.unk20 != 0) // 0x000057C8
        {
            // battery not charging
            batteryChargingStatus = 2; // 0x000057CC
        }
        else
        {
            // If bit 8 (pos 7) is set, then battery is being charged
            batteryChargingStatus =  g_Battery.powerSupplyStatus >> 7 & 0x1; // 0x000057D0 & 0x000057D4
        }
    }
    else
    {
        if (g_Battery.unk32 != 0) // 0x000057F4
        {
            // Apparently battery not charging
            batteryChargingStatus = 3; // 0x000057F8
        }
        else if (!g_Battery.isUsbChargingEnabled || !(g_Battery.powerSupplyStatus & 0x40) || g_Battery.unk40 == 0) // 0x00005800 & 0x00005810 & 0x0000581C
        {
            // Aparently battery not charging
            batteryChargingStatus = 0; // 0x00005804
        }
        else
        {
            // Apparently battery charging, why is that? We are not connected to an external power source
            // Could this be USB charging and sceSysconIsAcSupplied() does not supply that info (only traditional PSP AC)?
            batteryChargingStatus = 1; // 0x00005820
        }
    }

    pspSetK1(oldK1);
    return batteryChargingStatus;
}

 // Subroutine scePower_78A1A796 - Address 0x0000582C - Aliases: scePower_driver_88C79735
s32 scePowerIsSuspendRequired(void)
{
    s32 isSuspendRequired;
    s32 oldK1;

    oldK1 = pspShiftK1(); // 0x00005838

    if (sceSysconIsAcSupplied()) // 0x00005850
    {
        pspSetK1(oldK1);
        return SCE_FALSE; // 0x000058D8
    }

    if (g_Battery.batteryType == 0) // 0x0000585C
    {
        s32 remainingCapacity = scePowerGetBatteryRemainCapacity(); // 0x000058B8
        if (remainingCapacity <= 0) // 0x000058C0
        {
            isSuspendRequired = SCE_FALSE; // 0x000058C4
        }
        else
        {
            isSuspendRequired = remainingCapacity < g_Battery.forceSuspendCapacity; // 0x000058C8 & 0x000058D0
        }
    }

    if (g_Battery.batteryType == 1) // 0x00005864
    {
        u8 baryonStatus2 = sceSysconGetBaryonStatus2(); // 0x0000588C
        if (baryonStatus2 & 0x8) // 0x00005898
        {
            isSuspendRequired = SCE_TRUE; // 0x00005890
        }
        else
        {
            // TODO: Define constant for 3400 batteryVoltage
            isSuspendRequired = g_Battery.batteryVoltage >= 0 && g_Battery.batteryVoltage < 3400; // 0x0000589C - 0x000058B4
        }
    }

    pspSetK1(oldK1);

    // uofw note: if g_Battery.batteryType is neither 0 or 1, isSuspendRequired is unitialized.
    // That said, perhaps Sony made sure g_Battery.batteryType is only ever either 0 or 1.
    return isSuspendRequired;
}

// Subroutine scePower_94F5A53F - Address 0x000058DC - Aliases: scePower_driver_41ADFF48
s32 scePowerGetBatteryRemainCapacity(void)
{
    if (g_Battery.batteryAvailabilityStatus == BATTERY_NOT_INSTALLED) // 0x000058EC
    {
        return SCE_POWER_ERROR_NO_BATTERY;
    }

    if (g_Battery.batteryAvailabilityStatus == BATTERY_IS_BEING_DETECTED) // 0x000058FC
    {
        return SCE_POWER_ERROR_DETECTING;
    }

    return g_Battery.batteryRemainingCapacity; // 0x00005904
}

// Subroutine scePower_8EFB3FA2 - Address 0x00005910 - Aliases: scePower_driver_C79F9157
s32 scePowerGetBatteryLifeTime(void)
{
    s32 status;
    s32 intrState;

    // If an external power supply is available, return 0 since we cannot estimate the remaining batter life.
    if (sceSysconIsAcSupplied()) // 0x00005920 & 0x00005928
    {
        return 0;
    }

    if (g_Battery.batteryType != 0) // 0x0000593C
    {
        // uofw note: Yes, Sony is returning an unitialized local variable here
        // Presumably status should have been set to SCE_ERROR_NOT_SUPPORTED before 
        // returning here.
        return status;
    }

    s32 remainingCapacity = scePowerGetBatteryRemainCapacity(); // 0x00005944
    if (remainingCapacity < 0) // 0x00005950
    {
        return remainingCapacity; // 0x00005954
    }

    if (!g_Battery.isUsbChargingEnabled && g_Battery.batteryElec >= 0) // 0x0000596C & 0x000059EC
    {
        intrState = sceKernelCpuSuspendIntr(); // 0x000059F4

        sceKernelSetEventFlag(g_Battery.eventId, 0x10000000); // 0x00005A04
        sceKernelClearEventFlag(g_Battery.eventId, ~0x10000000); // 0x00005A14

        sceKernelCpuResumeIntr(intrState);
        return SCE_POWER_ERROR_DETECTING;
    }

    if (g_Battery.batteryElec < 0) // 0x00005978
    {
        s32 remainingCapacityForRunning = (remainingCapacity - g_Battery.forceSuspendCapacity); // 0x000059A8 & 0x000059AC

        if (g_Battery.batteryElec == -1) // 0x000059C0
        {
            pspBreak(SCE_BREAKCODE_DIVZERO); // 0x000059C4
        }

        // Convert remaining capacity (which is (presumably - based on PS Vita SDK doc) in mAh) into minutes.
        // rough formula to use here:
        //    (mAh)/(Amps*1000)*60 = (minutes). 
        s32 cvtRemainingTime = remainingCapacityForRunning * 60 / ~g_Battery.batteryElec; // 0x000059B0 & 0x000059B4 & 0x000059B4

        if (g_Battery.limitTime >= 0) // 0x000059CC & 0x000059D
        {
            cvtRemainingTime = pspMin(cvtRemainingTime, g_Battery.limitTime); // 0x000059D8
        }

        return pspMax(cvtRemainingTime, 1); // 0x000059E0
    }

    return 0;
}

// Subroutine scePower_28E12023 - Address 0x00005A30 - Aliases: scePower_driver_40870DAC
s32 scePowerGetBatteryTemp(void)
{
    if (g_Battery.batteryType != 0) // 0x00005A40
    {
        return SCE_ERROR_NOT_SUPPORTED;
    }

    if (g_Battery.batteryAvailabilityStatus == BATTERY_NOT_INSTALLED) // 0x00005A50
    {
        return SCE_POWER_ERROR_NO_BATTERY;
    }

    if (g_Battery.batteryAvailabilityStatus == BATTERY_IS_BEING_DETECTED) // 0x00005A60
    {
        return SCE_POWER_ERROR_DETECTING;
    }

    return g_Battery.batteryTemp; // 0x00005A68
}

// Subroutine scePower_862AE1A6 - Address 0x00005A74 - Aliases: scePower_driver_993B8C4A
s32 scePowerGetBatteryElec(u32 *pBatteryElec)
{
    s32 oldK1;

    oldK1 = pspShiftK1(); // 0x00005A80

    if (g_Battery.batteryType != 0) // 0x00005A90
    {
        return SCE_ERROR_NOT_SUPPORTED;
    }

    if (!pspK1PtrOk(pBatteryElec)) // 0x00005A98
    {
        return SCE_ERROR_PRIV_REQUIRED;
    }

    if (g_Battery.batteryAvailabilityStatus == BATTERY_NOT_INSTALLED) // 0x00005AA8
    {
        return SCE_POWER_ERROR_NO_BATTERY;
    }

    if (g_Battery.batteryAvailabilityStatus == BATTERY_IS_BEING_DETECTED) // 0x00005AB8
    {
        return SCE_POWER_ERROR_DETECTING;
    }

    // uOFW note: Missing null check
    *pBatteryElec = g_Battery.batteryElec; // 0x00005AC8

    pspSetK1(oldK1);
    return SCE_ERROR_OK;
}

// Subroutine scePower_CB49F5CE - Address 0x00005AD8 - Aliases: scePower_driver_8432901E
s32 scePowerGetBatteryChargeCycle(void)
{
    if (g_Battery.batteryType != 0) // 0x00005AE8
    {
        return SCE_ERROR_NOT_SUPPORTED;
    }

    if (g_Battery.batteryAvailabilityStatus == BATTERY_NOT_INSTALLED) // 0x00005AF8
    {
        return SCE_POWER_ERROR_NO_BATTERY;
    }

    if (g_Battery.batteryAvailabilityStatus == BATTERY_IS_BEING_DETECTED) // 0x00005B08
    {
        return SCE_POWER_ERROR_DETECTING;
    }

    return g_Battery.batteryChargeCycle; // 0x00005B10
}

// Subroutine sub_00005B1C - Address 0x00005B1C
static s32 _scePowerBatteryInit(u32 isUsbChargingSupported, u32 batteryType)
{
    memset(&g_Battery, 0, sizeof(ScePowerBattery)); // 0x00005B50

    g_Battery.permitChargingDelayAlarmId = 1; // 0x00005B74
    g_Battery.isUsbChargingSupported = isUsbChargingSupported; // 0x00005B78
    g_Battery.batteryType = batteryType; // 0x00005B7C
    g_Battery.isAcSupplied = -1; // 0x00005B80
    g_Battery.batteryFullCapacity = -1; // 0x00005B84
    g_Battery.batteryChargeCycle = -1; // 0x00005B8C

    g_Battery.eventId = sceKernelCreateEventFlag("ScePowerBattery", 1, 0, NULL); // 0x00005B88
    g_Battery.workerThreadId = sceKernelCreateThread("ScePowerBattery", _scePowerBatteryThread, POWER_BATTERY_WORKER_THREAD_PRIO, 
        2 * SCE_KERNEL_1KiB, SCE_KERNEL_TH_NO_FILLSTACK | 0x1, NULL); // 0x00005BB0

    sceKernelStartThread(g_Battery.workerThreadId, 0, NULL); // 0x00005BC4

    return SCE_ERROR_OK;
}

// Subroutine sub_00005BF0 - Address 0x00005BF0
static s32 _scePowerBatterySetParam(s32 forceSuspendCapacity, s32 lowBatteryCapacity)
{
    g_Battery.lowBatteryCapacity = lowBatteryCapacity; // 0x00005BFC
    g_Battery.forceSuspendCapacity = forceSuspendCapacity; // 0x00005C04

    return SCE_ERROR_OK;
}

// Subroutine sub_00005C08 - Address 0x00005C08
static s32 _scePowerBatteryIsBusy(void)
{
    return !g_Battery.isIdle;
}

// Subroutine sub_00005C18 - Address 0x00005C18
static s32 _scePowerBatteryResume(void)
{
    s32 intrState1;
    s32 intrState2;

    intrState1 = sceKernelCpuSuspendIntr(); // 0x00005C2C

    g_Battery.isAcSupplied = -1; // 0x00005C40
    g_Battery.batteryAvailabilityStatus = BATTERY_NOT_INSTALLED; // 0x00005C4C
    g_Battery.isIdle = SCE_FALSE; // 0x00005C4C

    sceKernelClearEventFlag(g_Battery.eventId, ~0x900); // 0x00005C64
    sceKernelSetEventFlag(g_Battery.eventId, g_Battery.unk20 == 0 ? 0x20000000 : 0x20000100); // 0x00005C70

    intrState2 = sceKernelCpuSuspendIntr(); // 0x00005C78

    sceKernelSetEventFlag(g_Battery.eventId, 0x10000000); // 0x00005C88
    sceKernelClearEventFlag(g_Battery.eventId, ~0x10000000); // 0x00005C98

    sceKernelCpuResumeIntr(intrState2); // 0x00005CA0
    sceKernelCpuResumeIntr(intrState1); // 0x00005CA8

    return SCE_ERROR_OK;
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
static s32 _scePowerBatteryDelayedPermitCharging(void *common)
{
    (void)common; 

    g_Battery.permitChargingDelayAlarmId = -1;
    sceKernelSetEventFlag(g_Battery.eventId, 0x200);

    return 0; /* Delete this alarm handler. */
}

// Subroutine sub_0x00005ED8 - Address 0x00005ED8
static s32 _scePowerBatterySysconCmdIntr(SceSysconPacket *pSysconPacket, void *param)
{
    (void)pSysconPacket;
    (void)param;

    g_Battery.unk108 = 2;
    return SCE_ERROR_OK;
}