/* Copyright (C) 2011, 2012 The uOFW team
   See the file COPYING for copying permission.
*/

/**
 * ctrl.c
 *    Reverse engineered controller libraries of the SCE PSP system.
 * Author: _Felix_
 * Version: 6.60
 *
 */

#include "common.h"

#include <pspmodulemgr.h>
#include "sysmem_sysclib.h"
#include "sysmem_sysevent.h"
#include "systimer.h"

#include "ctrl.h"

/* common defines */

#define USER_MODE                               0
#define KERNEL_MODE                             1

#define FALSE                                   0
#define TRUE                                    1

#define PSP_INIT_KEYCONFIG_UPDATER              0x110 //might be incorrect

/** CTRL defines */

#define CTRL_POLL_MODE_OFF                      0
#define CTRL_POLL_MODE_ON                       1

#define CTRL_INTERNAL_CONTROLLER_BUFFERS        64
#define CTRL_MAX_INTERNAL_CONTROLLER_BUFFER     CTRL_INTERNAL_CONTROLLER_BUFFERS -1

#define CTRL_ANALOG_PAD_DEFAULT_VALUE           128
#define CTRL_ANALOG_PAD_MIN_VALUE               0
#define CTRL_ANALOG_PAD_MAX_VALUE               255

#define CTRL_MAX_EXTRA_SUSPEND_SAMPLES          300

#define CTRL_BUFFER_UPDATE_MIN_CUSTOM_CYCLES    5555
#define CTRL_BUFFER_UPDATE_MAX_CUSTOM_CYCLES    20000

#define CTRL_SAMPLING_MODES                     2
#define CTRL_SAMPLING_MODE_MAX_MODE             CTRL_SAMPLING_MODES -1

#define CTRL_BUTTON_CALLBACK_SLOTS              4
#define CTRL_DATA_EMULATION_SLOTS               4
#define CTRL_BUTTONS_RAPID_FIRE_SLOTS           16

#define CTRL_BUTTON_CALLBACK_MAX_SLOT           CTRL_BUTTON_CALLBACK_SLOTS -1
#define CTRL_DATA_EMULATION_MAX_SLOT            CTRL_DATA_EMULATION_SLOTS -1
#define CTRL_BUTTONS_RAPID_FIRE_MAX_SLOT        CTRL_BUTTONS_RAPID_FIRE_SLOTS -1

#define CTRL_USER_MODE_BUTTONS_DEFAULT          0x3F3F9
#define CTRL_USER_MODE_BUTTONS_EXTENDED         0x3FFFF
#define CTRL_ALL_SUPPORTED_BUTTONS              0x39FFF3F9
//PSP_CTRL_MS | PSP_CTRL_DISC | PSP_CTRL_REMOTE | PSP_CTRL_WLAN_UP | PSP_CTRL_HOLD | ?
#define CTRL_PSP_HARDWARE_IO_BUTTONS            0x3B0E0000


PSP_SDK_VERSION(0x06060010);
PSP_MODULE_INFO("sceController_Service", 0x1007, 1, 1);
PSP_MODULE_BOOTSTART("CtrlInit");


typedef struct _SceCtrlRapidFire {
    /** The pressed-button-range to check for. */
    u32 pressedButtonRange; //0
    /** The button(s) which will fire the pressed/un-pressed period for a button/buttons when being pressed.
     *  The button(s) has/have to be included in the pressed-button-range. *
     */
    u32 reqButtonsEventTrigger; //4
    /** The requested button(s) on which the pressed/un-pressed period (the rapid fire event) will be applied to.
     *  User mode buttons ONLY.
     */
    u32 reqButtons; //8
    /** Apply data for a rapidFire period. Keeps track of the apply mode of the requested button(s) (pressed/un-pressed)
     *  + the amount of left internal controller buffer updates for the current apply mode of the requested button(s).
     */
    u8 eventData; //12
    /** For how many (consecutive) internal controller buffer updates the requested button(s) will be set to ON (pressed). */
    u8 reqButtonsOnTime; //13
    /** For how many (consecutive) internal controller buffer updates the requested button(s) will be set to OFF (un-pressed). */
    u8 reqButtonsOffTime; //14;
    /** For how many (consecutive) internal controller buffer updates the requested button(s) will be set to ON.
     *  It will only be applied for the first ON period of a (not canceled) rapid fire event. */
    u8 reqButtonsEventOnTime; //15;
} SceCtrlRapidFire; //size of SceCtrlRapidFire: 16

typedef struct _SceCtrlEmulatedData {
    /** Emulated analog pad X-axis offset. */
    u8 analogXEmulation; //0
    /** Emulated analog pad Y-axis offset. */
    u8 analogYEmulation; //1
    /** How many internal controller buffer updates the emulated analog pad values will be applied for. */
    u32 intCtrlBufUpdates; //4
    /** Emulated user buttons of ::PspCtrlButtons. You cannot emulate kernel buttons and
     *  the emulated buttons will only be applied for applications running in user mode.
     */
    u32 uModeBtnEmulation; //8
    /** Emulated buttons of ::PspCtrlButtons (you can emulate both user and kernel buttons).
     *  The emulated buttons will only be applied for applications running in kernel mode.
     */
    u32 kModeBtnEmulation; //12
    /** How many internal controller buffer updates the emulated buttons will be applied for. */
    u32 intCtrlBufUpdates2; //16
} SceCtrlEmulatedData; //size of SceCtrlEmulatedData: 20

typedef struct _SceCtrlButtonCallback {
    /** Bitwise OR'ed button values (of ::PspCtrlButtons) which will be checked for being pressed. */
    u32 btnMask; //0
    /** Pointer to a callback function handling the button input effects. */
    SceCtrlCb callbackFunc; //4
    /** The global pointer value of the controller module. */
    u32 gp; //8
    /** An optional pointer being passed as the third argument to the callback function. */
    void *arg; //12
} SceCtrlButtonCallback; //Size of SceCtrlButtonCallback: 16

typedef struct _SceCtrlInternalData {
    /** Button is newly pressed (was not already been pressed). */
    u32 btnMake; //0
    /** Stop of button press. */
    u32 btnBreak; //4
    /** Button is pressed. */
    u32 btnPress; //8
    /** Button is not pressed. */
    u32 btnRelease; //12
    /** Count the internal latch buffer reads. Is set to 0, when the buffer is reseted. */
    u32 readLatchCount; //16
    u32 ctrlBufStartIndex; //20
    u32 ctrlBufIndex; //24
    /** sceCtrlBuf has (indirectly) 64 pointers points to a unique internal controller buffer of type sceCtrlData. */
    void *sceCtrlBuf[3]; //28
} SceCtrlInternalData; //Size of SceCtrlInternalData: 40

typedef struct _SceCtrl {
    SceSysTimerId timerID; //0
    int eventFlag; //4
    u32 btnCycle; //8
    PspCtrlPadInputMode samplingMode[CTRL_SAMPLING_MODES]; //12 -- samplingMode[0] for User mode, samplingMode[1] for Kernel mode -- correct
    u8 unk_Byte_0; //14
    u8 unk_Byte_1; //15
    u8 sysconHwDataTransferBusy; //16
    u8 unk_Byte_7; //17 -- correct
    /** Reserved. */
    u8 resv[2]; //18
    u8 unk_Byte_2; //20
    PspCtrlPadPollMode pollMode; //21
    short int suspendSamples; //22
    int unk_1; //24
    SceSysconPacket sysPacket[2]; //28 -- size of one array member is 96.
    SceCtrlInternalData userModeData; //220
    SceCtrlInternalData kernelModeData; //260
    SceCtrlRapidFire rapidFire[CTRL_BUTTONS_RAPID_FIRE_SLOTS]; //300 - 555
    /** Currently pressed buttons passed to _SceCtrlUpdateButtons(). They are "pure",
     *  as custom settings are applied on them in ::_SceCtrlUpdateButtons. */
    u32 pureButtons; //556
    int unk_array_3[2]; //560 -- previous pressed button?
    u32 prevButtons; //568 -- previously pressed buttons
    /** Currently pressed User buttons. */
    u32 userButtons; //572
    /* Records the possibly user-modified, pressed buttons of the past VBlank interrupt before the current one. */
    u32 prevModifiedButtons; //576
    u8 analogX; //580;
    u8 analogY; //581
    char unk_Byte_9; //582
    char unk_Byte_3; //583
    SceCtrlEmulatedData emulatedData[CTRL_DATA_EMULATION_SLOTS]; //584
    u32 userModeButtons; //664
    /** Button bit mask defining buttons going to be supported (recognized if being pressed or not).
     *  Can be used to ignore buttons (buttons constantly turned off). */
    u32 maskSupportButtons; //668
    /** Button bit mask defining buttons going to be simulated as being pressed. */
    u32 maskSetButtons; //672
    int unk_4; //676
    int unk_5; //680
    int unk_6; //684
    int unk_7; //688
    int unk_8; //692
    int idleReset; //696
    int idleBack; //700
    SceCtrlButtonCallback buttonCallback[CTRL_BUTTON_CALLBACK_SLOTS]; //704
    int unk_9; //768
    ctrlUnkStruct *unk_array2[2]; //772 -- array of functions?
    int unk_array3[2]; //780
} SceCtrl; //size of SceCtrl: 788 (0x314)

static int _sceCtrlSysEventHandler(int ev_id, char* ev_name, void* param, int* result); //0x00000364
static SceUInt _sceCtrlDummyAlarm(void *common); //sub_00001DD8
static int _sceCtrlVblankIntr(int subIntNm, void *arg); //sub_00000440
static int _sceCtrlTimerIntr(int, int, int, int); //sub_00000528
static int _sceCtrlSysconCmdIntr1(SceSysconPacket *sysPacket); //sub_00000610;
static int _sceCtrlSysconCmdIntr2(void); //sub_00001E4C
static int _sceCtrlUpdateButtons(u32 pureButtons, u8 aX, u8 aY); //sub_00000968
static int _sceCtrlReadBuf(SceCtrlDataExt *pad, u8 reqBufReads, int arg3, u8 mode); //sub_00001E70

SceCtrl ctrl; //0x2890
SceKernelDeci2Ops ctrlDeci2Ops = { sizeof(SceKernelDeci2Ops), { (void *)sceCtrlGetSamplingMode, (void *)sceCtrlGetSamplingCycle,
                                                                (void *)sceCtrlPeekBufferPositive, (void *)sceCtrlPeekLatch,
                                                                (void *)sceCtrlSetRapidFire, (void *)sceCtrlClearRapidFire,
                                                                (void *)sceCtrlSetButtonEmulation, (void *)sceCtrlSetAnalogEmulation,
                                                                (void *)sceCtrlExtendInternalCtrlBuffers
                                                              } }; //0x000027F8

SceSysEventHandler ctrlSysEvent = { sizeof(SceSysEventHandler), "SceCtrl", 0x00FFFF00, _sceCtrlSysEventHandler, 0, 0, NULL,
                                    { 0, 0, 0, 0, 0, 0, 0, 0, 0 } }; //0x00002850

SceCtrlData g_2BB0; //0x00002BB0
SceCtrlData g_2FB0; //0x00002FB0


/*
 * Subroutine sceCtrl_driver_121097D5 - Address 0x00000000
 * Exported in sceCtrl_driver
 */
int sceCtrlInit(void) {
    int eventId;
    int keyConfig;
    SceSysTimerId timerId;
    u32 supportedUserButtons;
    void (*func)(SceKernelDeci2Ops *);
    int *retPtr;
    int pspModel;

    memset(&ctrl, 0, sizeof(SceCtrl)); //0x00000024

    ctrl.pollMode = PSP_CTRL_POLL_POLLING; //0x00000048
    ctrl.userModeData.sceCtrlBuf[0] = &g_2BB0; //0x00000054 -- size of SceCtrlData is 16 (default) -> required size is 64 * 16 = 0x400.
    ctrl.analogY = CTRL_ANALOG_PAD_DEFAULT_VALUE; //0x00000060
    ctrl.analogX = CTRL_ANALOG_PAD_DEFAULT_VALUE; //0x00000064
    ctrl.kernelModeData.sceCtrlBuf[0] = &g_2FB0; //0x0000006C
    ctrl.unk_1 = -1; //0x00000074

    eventId = sceKernelCreateEventFlag("SceCtrl", 1, 0, NULL); //0x00000070
    ctrl.eventFlag = eventId; //0x0000007C

    timerId = sceSTimerAlloc(); //0x00000078
    if (timerId < 0) { //0x00000080
        return timerId;
    }
    ctrl.timerID = timerId;
    sceSTimerSetPrscl(timerId, 1, 0x30); //0x00000094
    ctrl.unk_Byte_1 = -1; //0x000000A0
    sceKernelRegisterSysEventHandler(&ctrlSysEvent); //0x000000A4
    sceSyscon_driver_B72DDFD2(0); //0x000000AC

    keyConfig = sceKernelApplicationType(); //0x000000B4
    if (keyConfig == PSP_INIT_KEYCONFIG_UPDATER) { //0x000000C0
        supportedUserButtons = CTRL_USER_MODE_BUTTONS_EXTENDED; //0x00000208
    }
    if (keyConfig > PSP_INIT_KEYCONFIG_UPDATER) { //0x000000CC
        supportedUserButtons = CTRL_USER_MODE_BUTTONS_DEFAULT; //0x000000EC
        if (keyConfig == PSP_INIT_KEYCONFIG_POPS) { //0x000000E0
            supportedUserButtons = CTRL_USER_MODE_BUTTONS_EXTENDED; //0x00000208
        }
    }
    if (keyConfig == 0) { //0x000000D4
        supportedUserButtons = CTRL_USER_MODE_BUTTONS_DEFAULT; //0x000000EC
    }
    else { //0x000000DC
         supportedUserButtons = CTRL_USER_MODE_BUTTONS_DEFAULT; //0x000000EC
         if (keyConfig == PSP_INIT_KEYCONFIG_VSH) { //0x000000E0
             supportedUserButtons = CTRL_USER_MODE_BUTTONS_EXTENDED; //0x00000208
         }
    }
    ctrl.userModeButtons = supportedUserButtons; //0x000000F0
    ctrl.maskSetButtons = 0; //0x000000FC
    ctrl.maskSupportButtons = supportedUserButtons; //0x00000104

    pspModel = sceKernelGetModel();
    switch (pspModel) { //0x00000128
        case 4: case 5: case 7: case 9:
            ctrl.unk_4 = CTRL_ALL_SUPPORTED_BUTTONS; //0x000001EC & 0x00000138
            break;
        default: //0x0000010C -> 0x000001F0
            ctrl.unk_4 = 0x1FFF3F9; //0x00000130 & 0x00000138
            break;
    }
    ctrl.unk_5 = CTRL_ALL_SUPPORTED_BUTTONS; //0x00000168
    ctrl.unk_6 = 0xF1F3F9; //0x00000174
    ctrl.unk_7 = 0x390E0000; //0x0000017C
    ctrl.unk_8 = 0; //0x00000180
    ctrl.idleReset = 129; //0x00000188
    ctrl.idleBack = 129; //0x00000158

    sceKernelRegisterSubIntrHandler(PSP_VBLANK_INT, 0x13, _sceCtrlVblankIntr, NULL); //0x00000184
    sceKernelEnableSubIntr(PSP_VBLANK_INT, 0x13); //0x00000190

    //TODO: Reverse the function below.
    retPtr = sceKernelDeci2pReferOperations(); //0x00000198
    if ((retPtr != NULL) && (*retPtr == 48)) { //0x000001A0 & 0x000001AC
         func = (void (*)(SceKernelDeci2Ops *))*(retPtr + 44); //0x000001B0
         func(&ctrlDeci2Ops); //0x000001D8
    }
    return SCE_ERROR_OK;
}

/*
 * Subroutine sceCtrl_driver_1A1A7D40 - Address 0x0000020C
 * Exported in sceCtrl_driver
 */
int sceCtrlEnd(void) {
    SceSysTimerId timerId;
    int sysconStatus;

    sceKernelUnregisterSysEventHandler(&ctrlSysEvent); //0x00000220
    sceSyscon_driver_B72DDFD2(1); //0x00000228
    sceDisplayWaitVblankStart(); //0x00000230

    timerId = ctrl.timerID; //0x0000023C
    ctrl.timerID = -1; //0x00000244
    if (timerId >= 0) { //0x00000248
        sceSTimerStopCount(timerId); //0x00000250
        sceSTimerFree(timerId); //0x00000258
    }
    sceKernelReleaseSubIntrHandler(PSP_VBLANK_INT, 0x13); //0x00000264
    sceKernelDeleteEventFlag(ctrl.eventFlag); //0x0000026C

    //0x00000274
    sysconStatus = ctrl.sysconHwDataTransferBusy;
    sysconStatus |= ctrl.unk_Byte_7;
    sysconStatus |= ctrl.resv[0];
    sysconStatus |= ctrl.resv[1];

    while (sysconStatus != 0) { //0x0000028C
           sceDisplayWaitVblankStart(); //0x00000280
           sysconStatus = ctrl.sysconHwDataTransferBusy;
           sysconStatus |= ctrl.unk_Byte_7;
           sysconStatus |= ctrl.resv[0];
           sysconStatus |= ctrl.resv[1];
    }
    return SCE_ERROR_OK; //0x000002A0
}

/*
 * Subroutine sceCtrl_driver_55497589 - Address 0x00001A50
 * Exported in sceCtrl_driver
 */
int sceCtrlSuspend(void) {
    int cycle;

    cycle = ctrl.btnCycle; //0x00001A58
    if (cycle != 0) { //0x00001A68
        sceSTimerStopCount(ctrl.timerID); //0x00001A88
    }
    else {
        sceKernelDisableSubIntr(PSP_VBLANK_INT, 0x13); //0x00001A70
    }
    return SCE_ERROR_OK;
}

/*
 * Subroutine sceCtrl_driver_33D03FD5 - Address 0x000002AC
 * Exported in sceCtrl_driver
 */
int sceCtrlResume(void) {
    int retVal;
    int prevButtons;

    retVal = sceSyscon_driver_97765E27(); //0x000002B8
    if (retVal != 0) { //0x000002C4
        if (retVal == 1) { //0x00000344
            prevButtons = ctrl.prevButtons; //0x00000350
            prevButtons |= 0x20000000; //0x00000358
            ctrl.prevButtons = prevButtons; //0x00000360
        }
    }
    else {
        prevButtons = ctrl.prevButtons; //0x000002CC
        prevButtons &= 0xDFFFFFFF; //0x000002D0
        ctrl.prevButtons = prevButtons; //0x000002D4
    }
    ctrl.unk_Byte_1 = -1;
    if (ctrl.btnCycle == 0) { //0x000002DC & 0x000002F0
        sceKernelEnableSubIntr(PSP_VBLANK_INT, 0x13); //0x000002F8
    }
    else {
        sceSTimerStartCount(ctrl.timerID); //0x00000318
        sceSTimerSetHandler(ctrl.timerID, ctrl.btnCycle, _sceCtrlTimerIntr, 0); //0x00000330
    }
    return SCE_ERROR_OK;
}

/*
 * Subroutine sceCtrl_driver_F0074903 - Address 0x00001C30
 * Exported in sceCtrl_driver
 */
int sceCtrlSetPollingMode(PspCtrlPadPollMode pollMode) {
    ctrl.pollMode = pollMode; //0x00001C3C
    return SCE_ERROR_OK;
}

/*
 * Subroutine sceCtrl_DA6B76A1 - Address 0x00001330 - Aliases: sceCtrl_driver_F8EC18BD
 * Exported in sceCtrl
 * Exported in sceCtrl_driver
 */
PspCtrlPadInputMode sceCtrlGetSamplingMode(PspCtrlPadInputMode *mode) {
    int oldK1 = pspShiftK1(); //0x00001330
    if (pspK1PtrOk(mode)) { //0x00001348 & 0x00001350
        *mode = ctrl.samplingMode[!pspK1IsUserMode()]; //0x00001358 & 0x0000135C
    }
    pspSetK1(oldK1);
    return SCE_ERROR_OK;
}

/* Subroutine sceCtrl_1F4011E6 - Address 0x000012C8 - Aliases: sceCtrl_driver_F6E94EA3
 * Exported in sceCtrl
 * Exported in sceCtrl_driver
 */
int sceCtrlSetSamplingMode(PspCtrlPadInputMode mode) {
    int suspendFlag;
    int privMode;
    u8 index;
    PspCtrlPadInputMode prevMode;

    if (mode > CTRL_SAMPLING_MODE_MAX_MODE) { //0x000012D0 & 0x000012E4
        return SCE_ERROR_INVALID_MODE;
    }
    suspendFlag = sceKernelCpuSuspendIntr(); //0x000012EC
    privMode = (pspGetK1() >> 20) & 0x1; //0x000012F4
    index = (privMode < 1); //0x000012FC

    prevMode = ctrl.samplingMode[index]; //0x00001308
    ctrl.samplingMode[index] = mode; //0x0000130C

    sceKernelCpuResumeIntr(suspendFlag); //0x00001310:
    return prevMode; //0x00001318
}

/*
 * Subroutine sceCtrl_02BAAD91 - Address 0x00001AB8 - Aliases: sceCtrl_driver_501E0C70
 * Exported in sceCtrl
 * Exported in sceCtrl_driver
 */
int sceCtrlGetSamplingCycle(u32 *cycle) {
    int oldK1;

    oldK1 = pspShiftK1(); //0x00001AB8
    if (pspK1PtrOk(cycle)) { //0x00001ABC && 0x00001AC0
        *cycle = ctrl.btnCycle; //0x00001ACC && 0x00001AD0
    }
    pspSetK1(oldK1);
    return SCE_ERROR_OK;
}

/*
 * Subroutine sceCtrl_6A2774F3 - Address 0x0000136C - Aliases: sceCtrl_driver_83B15A81
 * Exported in sceCtrl
 * Exported in sceCtrl_driver
 */
int sceCtrlSetSamplingCycle(u32 cycle) {
    int suspendFlag;
    u32 prevCycle;
    u32 nCycle;
    int sdkVersion;
    int oldK1;

    oldK1 = pspShiftK1(); // 0x00001378
    suspendFlag = sceKernelCpuSuspendIntr(); //0x00001398

    /* enable the VBlank-Interrupt-Update process and terminate our registered Timer-Update process. */
    if (cycle == 0) { //0x000013B0
        prevCycle = ctrl.btnCycle; //0x00001460
        sceKernelEnableSubIntr(PSP_VBLANK_INT, 0x13); //0x00001464
        ctrl.btnCycle = 0; //0x00001468

        sceSTimerSetHandler(ctrl.timerID, 0, NULL, 0); //0x00001478
        sceSTimerStopCount(ctrl.timerID); //0x00001480
    }
    else if (cycle < CTRL_BUFFER_UPDATE_MIN_CUSTOM_CYCLES || cycle > CTRL_BUFFER_UPDATE_MAX_CUSTOM_CYCLES) { //0x000013B4 & 0x000013B8
        return SCE_ERROR_INVALID_VALUE; //0x0000138C & 0x000013C0
    }
    /* register our new  Timer-Update process and disable the VBlank-Interrupt-Update process. */
    else {
        prevCycle = ctrl.btnCycle; //0x0000140C
        sceSTimerStartCount(ctrl.timerID); //0x00001410
        ctrl.btnCycle = cycle; //0x00001414
        sdkVersion = sceKernelGetCompiledSdkVersion(); //0x00001418

        /* If the PSP Firmware version is >= 2.5, the original cycle is used, otherwise cycle + 1. */
        nCycle = (sdkVersion > 0x204FFFF) ? 0 : 1; //0x00001428 && 0x0000142C
        nCycle += cycle;

        sceSTimerSetHandler(ctrl.timerID, nCycle, _sceCtrlTimerIntr, 0); //0x00001408 & 0x00001444
        sceKernelDisableSubIntr(PSP_VBLANK_INT, 0x13); //0x00001450
    }
    sceKernelCpuResumeIntr(suspendFlag); //0x000013D8
    pspSetK1(oldK1); //0x000013E4
    return prevCycle; //0x000013E0
}

/*
 * Subroutine sceCtrl_687660FA - Address 0x0000170C - Aliases: sceCtrl_driver_E54253E7
 * Exported in sceCtrl
 * Exported in sceCtrl_driver
 */
int sceCtrlGetIdleCancelThreshold(int *idleReset, int *idleBack) {
    int oldK1;
    int suspendFlag;
    int tempIdleBack;
    int tempIdleReset;

    oldK1 = pspShiftK1();
    if (!pspK1PtrOk(idleReset) || !pspK1PtrOk(idleBack)) { //0x00001734 & 0x00001740
        pspSetK1(oldK1);
        return SCE_ERROR_PRIV_REQUIRED; /* protect kernel addresses from user mode. */
    }
    suspendFlag = sceKernelCpuSuspendIntr(); //0x0000176C
    if (idleReset != NULL) { //0x00001774
        tempIdleReset = ctrl.idleReset;
        *idleReset = (tempIdleReset == 129) ? -1 : tempIdleReset; //0x00001788
    }
    if (idleBack != NULL) { //0x00001794
        tempIdleBack = ctrl.idleBack;
        *idleBack = (tempIdleBack == 129) ? -1 : tempIdleBack; //0x000017A4
    }
    sceKernelCpuResumeIntr(suspendFlag); //0x000017B0
    pspSetK1(oldK1); //0x000017B8
    return SCE_ERROR_OK; //0x000017C0
}

/*
 * Subroutine sceCtrl_A7144800 - Address 0x00001680 - Aliases: sceCtrl_driver_37533267
 * Exported in sceCtrl
 * Exported in sceCtrl_driver
 */
int sceCtrlSetIdleCancelThreshold(int idlereset, int idleback) {
    int suspendFlag;
    if (((idlereset < -1 || idlereset > 128) && idleback < -1) || idleback > 128) { //0x000016A0 & 0x000016B4 & 0x000016C8
        return SCE_ERROR_INVALID_VALUE; //0x000016A8 & 0x000016AC & 0x00001700
    }
    suspendFlag = sceKernelCpuSuspendIntr(); //0x000016D0

    ctrl.idleBack = (idleback == -1) ? 129 : idleback; //0x000016D4 & 0x000016E0
    ctrl.idleReset = (idlereset == -1) ? 129 : idlereset; //0x000016CC & 0x000016EC

    sceKernelCpuResumeIntr(suspendFlag); //0x000016E8
    return SCE_ERROR_OK; //0x000016F0
}

/*
 * Subroutine sceCtrl_AF5960F3 - Address 0x00001C6C - Aliases: sceCtrl_driver_BC8D1A3B
 * Exported in sceCtrl
 * Exported in sceCtrl_driver
 */
short int sceCtrlGetSuspendingExtraSamples(void) {
    short int curSuspendSamples;

    curSuspendSamples = ctrl.suspendSamples;  //0x00001C74
    return curSuspendSamples;
}

/*
 * Subroutine sceCtrl_348D99D4 - Address 0x00001C40 - Aliases: sceCtrl_driver_547F89D3
 * Exported in sceCtrl
 * Exported in sceCtrl_driver
 */
int sceCtrlSetSuspendingExtraSamples(short suspendSamples) {
    short int nSuspendSamples;

    if (suspendSamples > CTRL_MAX_EXTRA_SUSPEND_SAMPLES) { //0x00001C44
        return SCE_ERROR_INVALID_VALUE; //0x00001C48 & 0x00001C54
    }
    nSuspendSamples = (suspendSamples == 1) ? 0 : suspendSamples; //0x00001C40 & 0x00001C4C
    ctrl.suspendSamples = nSuspendSamples;

    return SCE_ERROR_OK;
}

/*
 * Subroutine sceCtrl_driver_E467BEC8 - Address 0x000011F0
 * Exported in sceCtrl_driver
 */
int sceCtrlExtendInternalCtrlBuffers(u8 mode, ctrlUnkStruct *arg2, int arg3) {
    SceUID poolId;
    void *ctrlBuf;

    //0x0000122C
    if (mode < 1 || mode > 2) {
        return SCE_ERROR_INVALID_VALUE;
    }
    if (ctrl.unk_array2[mode] == NULL) { //0x00001238 & 0x00001254
        poolId = sceKernelCreateFpl("SceCtrlBuf", PSP_MEMORY_PARTITION_KERNEL, 0, 2 * sizeof(SceCtrlDataExt) * CTRL_INTERNAL_CONTROLLER_BUFFERS, 1, NULL); //0x00001294
        if (poolId < 0) { //0x000012A4
            return poolId;
        }
        sceKernelTryAllocateFpl(poolId, &ctrlBuf); //0x000012AC
        ctrl.kernelModeData.sceCtrlBuf[0] = (SceCtrlDataExt *)(ctrlBuf + sizeof(SceCtrlDataExt) * CTRL_INTERNAL_CONTROLLER_BUFFERS); //0x000012BC
        ctrl.userModeData.sceCtrlBuf[0] = (SceCtrlDataExt *)ctrlBuf; //0x000012C4
    }
    ctrl.unk_array3[mode - 1] = arg3; //0x00001264
    ctrl.unk_array2[mode - 1] = arg2; //0x0000126C
    return SCE_ERROR_OK;
}

/*
 * Subroutine sceCtrl_B1D0E5CD - Address 0x00001490 - Aliases: sceCtrl_driver_637CB76C
 * Exported in sceCtrl
 * Exported in sceCtrl_driver
 */
int sceCtrlPeekLatch(SceCtrlLatch *latch) {
    SceCtrlInternalData *latchPtr;
    int suspendFlag;
    int oldK1;

    oldK1 = pspShiftK1();
    suspendFlag = sceKernelCpuSuspendIntr(); //0x000014A8

    if (!pspK1PtrOk(latch)) { //0x000014B8
        sceKernelCpuResumeIntr(suspendFlag); //0x00001524
        pspSetK1(oldK1); //0x00001530
        return SCE_ERROR_PRIV_REQUIRED; //0x0000152C & 0x00001538 -- protect kernel address from usermode
    }
    if (!pspK1IsUserMode()) { //0x000014C4
        latchPtr = &ctrl.kernelModeData; //0x00001520
    }
    else {
        latchPtr = &ctrl.userModeData; //0x000014D0
    }
    latch->btnMake = latchPtr->btnMake; //0x000014D4
    latch->btnBreak = latchPtr->btnBreak; //0x000014DC
    latch->btnPress = latchPtr->btnPress; //0x000014E4
    latch->btnRelease = latchPtr->btnRelease; //0x000014EC

    sceKernelCpuResumeIntr(suspendFlag); //0x000014F4
    pspSetK1(oldK1); //0x000014FC
    return latchPtr->readLatchCount; //0x00001500
}

/*
 * Subroutine sceCtrl_0B588501 - Address 0x0000153C - Aliases: sceCtrl_driver_7F7C4E0A
 * Exported in sceCtrl
 * Exported in sceCtrl_driver
 */
int sceCtrlReadLatch(SceCtrlLatch *latch) {
    SceCtrlInternalData *latchPtr;
    int suspendFlag;
    int readLatchCount;
    int oldK1;

    oldK1 = pspShiftK1();
    suspendFlag = sceKernelCpuSuspendIntr(); //0x00001554

    if (!pspK1PtrOk(latch)) { //0x00001564 -- protect kernel address from user mode
        sceKernelCpuResumeIntr(suspendFlag); //0x00001620
        pspSetK1(oldK1); //0x0000162C
        return SCE_ERROR_PRIV_REQUIRED; //0x00001634
    }
    if (!pspK1IsUserMode()) { //0x00001570
        latchPtr = &ctrl.kernelModeData; //0x000015E0
        readLatchCount = latchPtr->readLatchCount; //0x00001614
        latchPtr->readLatchCount = 0; //0x0000161C
    }
    else {
        latchPtr = &ctrl.userModeData; //0x0000157C
        readLatchCount = latchPtr->readLatchCount; //0x000015A0
        latchPtr->readLatchCount = 0; //0x000015A8
    }
    latch->btnMake = latchPtr->btnMake; //0x00001584 & 0x000015E8
    latch->btnBreak = latchPtr->btnBreak; //0x0000158C & 0x000015F0
    latch->btnPress = latchPtr->btnPress; //0x00001594 & 0x000015F8
    latch->btnRelease = latchPtr->btnRelease; //0x0000159C & 0x00001600

    latchPtr->btnMake = 0; //0x000015A4 & 0x00001604
    latchPtr->btnBreak = 0; //0x000015AC & 0x00001608
    latchPtr->btnPress = 0; //0x000015B0 & 0x0000160C
    latchPtr->btnRelease = 0; //0x000015B4 & 0x00001610

    sceKernelCpuResumeIntr(suspendFlag); //0x000015B8
    pspSetK1(oldK1); //0x000015C0
    return readLatchCount; //0x000015C4
}

/*
 * Subroutine sceCtrl_3A622550 - Address 0x00001AE0 - Aliases: sceCtrl_driver_2BA616AF
 * Exported in sceCtrl
 * Exported in sceCtrl_driver
 */
int sceCtrlPeekBufferPositive(SceCtrlData *pad, u8 reqBufReads) {
    int readBuffers;

    readBuffers = _sceCtrlReadBuf((SceCtrlDataExt *)pad, reqBufReads, 0, 0); //0x00001AEC
    return readBuffers;
}

/*
 * Subroutine sceCtrl_C152080A - Address 0x00001B00 - Aliases: sceCtrl_driver_E6085C33
 * Exported in sceCtrl
 * Exported in sceCtrl_driver
 */
int sceCtrlPeekBufferNegative(SceCtrlData *pad, u8 reqBufReads) {
    int readBuffers;

    readBuffers = _sceCtrlReadBuf((SceCtrlDataExt *)pad, reqBufReads, 0, 1); //0x00001B0C
    return readBuffers;
}

/*
 * Subroutine sceCtrl_1F803938 - Address 0x00001B20 - Aliases: sceCtrl_driver_BE30CED0
 * Exported in sceCtrl
 * Exported in sceCtrl_driver
 */
int sceCtrlReadBufferPositive(SceCtrlData *pad, u8 reqBufReads) {
    int readBuffers;

    readBuffers = _sceCtrlReadBuf((SceCtrlDataExt *)pad, reqBufReads, 0, 2); //0x00001B2C
    return readBuffers;
}

/*
 * Subroutine sceCtrl_60B81F86 - Address 0x00001B40 - Aliases: sceCtrl_driver_3A6A612A
 * Exported in sceCtrl
 * Exported in sceCtrl_driver
 */
int sceCtrlReadBufferNegative(SceCtrlData *pad, u8 reqBufReads) {
    int readBuffers;

    readBuffers = _sceCtrlReadBuf((SceCtrlDataExt *)pad, reqBufReads, 0, 3); //0x00001B4C
    return readBuffers;
}

/*
 * Subroutine sceCtrl_5A36B1C2 - Address 0x00001B60 - Aliases: sceCtrl_driver_D4692E77
 * Exported in sceCtrl
 * Exported in sceCtrl_driver
 */
int sceCtrlPeekBufferPositiveExt(int arg1, SceCtrlDataExt *pad, u8 reqBufReads) {
    int readBuffers;

    readBuffers = _sceCtrlReadBuf(pad, reqBufReads, arg1, 4); //0x00001B78
    return readBuffers;
}

/*
 * Subroutine sceCtrl_239A6BA7 - Address 0x00001B8C - Aliases: sceCtrl_driver_41BCD9ED
 * Exported in sceCtrl
 * Exported in sceCtrl_driver
 */
int sceCtrlPeekBufferNegativeExt(int arg1, SceCtrlDataExt *pad, u8 reqBufReads) {
    int readBuffers;

    readBuffers = _sceCtrlReadBuf(pad, reqBufReads, arg1, 5); //0x00001BA4
    return readBuffers;
}

/*
 * Subroutine sceCtrl_1098030B - Address 0x00001BB8 - Aliases: sceCtrl_driver_3BD76EDE
 * Exported in sceCtrl
 * Exported in sceCtrl_driver
 */
int sceCtrlReadBufferPositiveExt(int arg1, SceCtrlDataExt *pad, u8 reqBufReads) {
    int readBuffers;

    readBuffers = _sceCtrlReadBuf(pad, reqBufReads, arg1, 6); //0x00001BD0
    return readBuffers;
}

/*
 * Subroutine sceCtrl_7C3675AB - Address 0x00001BE4 - Aliases: sceCtrl_driver_7ABDEBAA
 * Exported in sceCtrl
 * Exported in sceCtrl_driver
 */
int sceCtrlReadBufferNegativeExt(int arg1, SceCtrlDataExt *pad, u8 reqBufReads) {
    int readBuffers;

    readBuffers = _sceCtrlReadBuf(pad, reqBufReads, arg1, 7); //0x00001BFC
    return readBuffers;
}

/*
 * Subroutine sceCtrl_A68FD260 - Address 0x00001DA8 - Aliases: sceCtrl_driver_994488EC
 * Exported in sceCtrl
 * Exported in sceCtrl_driver
 */
int sceCtrlClearRapidFire(u8 slot) {
    if (slot > CTRL_BUTTONS_RAPID_FIRE_MAX_SLOT) { //0x00001DBC && 0x00001DC0
        return SCE_ERROR_INVALID_INDEX;
    }
    ctrl.rapidFire[slot].pressedButtonRange = 0; //0x00001DC8
    return SCE_ERROR_OK;
}

/*
 * Subroutine sceCtrl_6841BE1A - Address 0x000018DC - Aliases: sceCtrl_driver_89438C13
 * Exported in sceCtrl
 * Exported in sceCtrl_driver
 */
int sceCtrlSetRapidFire(u8 slot, u32 pressedBtnRange, u32 reqBtnsEventTrigger, u32 reqBtn, u8 reqBtnEventOnTime, u8 reqBtnOnTime, u8 reqBtnOffTime) {
    u32 usedButtons;
    u32 kernelButtons;
    int oldK1;
    int suspendFlag;

    if (slot > CTRL_BUTTONS_RAPID_FIRE_MAX_SLOT) { //0x000018FC & 0x0000193C
        return SCE_ERROR_INVALID_INDEX;
    }
    if ((reqBtnEventOnTime | reqBtnOnTime | reqBtnOffTime) > CTRL_MAX_INTERNAL_CONTROLLER_BUFFER) { //0x000018E8 & 0x000018F4 & 0x00001910 & 0x00001954
        return SCE_ERROR_INVALID_VALUE;
    }
    oldK1 = pspShiftK1();

    usedButtons = pressedBtnRange | reqBtnsEventTrigger; //0x0000195C
    usedButtons = usedButtons | reqBtn; //0x00001968
    if (pspK1IsUserMode()) { //0x00001964
        kernelButtons = ~ctrl.userModeButtons; //0x0000197C
        kernelButtons = kernelButtons | PSP_CTRL_HOLD; //0x00001980
        //if the buttons given by the user include kernel buttons or the HOLD button -> return error
        if (kernelButtons & usedButtons) { //0x00001984 & 0x00001988
            pspSetK1(oldK1); //0x00001A0C
            return SCE_ERROR_PRIV_REQUIRED;
        }
    }
    suspendFlag = sceKernelCpuSuspendIntr(); //0x00001990

    ctrl.rapidFire[slot].reqButtonsOnTime = reqBtnOnTime; //0x00001998 & 0x0000199C & 0x000019A0 & 0x000019A4 & 0x000019A8
    ctrl.rapidFire[slot].pressedButtonRange = pressedBtnRange; //0x000019B4
    ctrl.rapidFire[slot].reqButtonsEventTrigger = reqBtnsEventTrigger; //0x000019B8
    ctrl.rapidFire[slot].reqButtons = reqBtn; //0x000019BC
    ctrl.rapidFire[slot].reqButtonsOffTime = reqBtnOffTime; //0x000019C0
    ctrl.rapidFire[slot].reqButtonsEventOnTime = reqBtnEventOnTime; //0x000019B0
    ctrl.rapidFire[slot].eventData = 0; //0x000019C8

    sceKernelCpuResumeIntr(suspendFlag); //0x000019C4
    pspSetK1(oldK1); //0x000019CC
    return SCE_ERROR_OK;
}

/*
 * Subroutine sceCtrl_driver_DB76878D - Address 0x00001CB8
 * Exported in sceCtrl_driver
 */
int sceCtrlSetAnalogEmulation(u8 slot, u8 aXEmu, u8 aYEmu, u32 bufUpdates) {
    if (slot > CTRL_DATA_EMULATION_MAX_SLOT) { //0x00001CE4
        return SCE_ERROR_INVALID_VALUE; //0x00001CD0 & 0x00001CE8
    }

    ctrl.emulatedData[slot].analogXEmulation = aXEmu; //0x00001CF8
    ctrl.emulatedData[slot].analogYEmulation = aYEmu; //0x00001CEC
    ctrl.emulatedData[slot].intCtrlBufUpdates = bufUpdates; //0x00001CF4

    return SCE_ERROR_OK;
}

/*
 * Subroutine sceCtrl_driver_5130DAE3 - Address 0x00001C78
 * Exported in sceCtrl_driver
 */
int sceCtrlSetButtonEmulation(u8 slot, u32 uModeBtnEmu, u32 kModeBtnEmu, u32 bufUpdates) {
    if (slot > CTRL_DATA_EMULATION_MAX_SLOT) { //0x00001C98
        return SCE_ERROR_INVALID_VALUE; //0x00001C90 & 0x00001C9C
    }
    ctrl.emulatedData[slot].uModeBtnEmulation = uModeBtnEmu; //0x00001CA8
    ctrl.emulatedData[slot].kModeBtnEmulation = kModeBtnEmu; //0x00001CAC
    ctrl.emulatedData[slot].intCtrlBufUpdates2 = bufUpdates; //0x00001CA0

    return SCE_ERROR_OK;
}

/* Subroutine sceCtrl_driver_1809B9FC - Address 0x00001878
 * Exported in sceCtrl_driver
 */
PspCtrlPadButtonMaskMode sceCtrlGetButtonIntercept(u32 btnMask) {
    int suspendFlag;
    int curMaskSupBtns;
    int curMaskSetBtns;
    PspCtrlPadButtonMaskMode btnMaskMode = PSP_CTRL_MASK_IGNORE_BUTTON_MASK;

    suspendFlag = sceKernelCpuSuspendIntr(); //0x0000188C
    curMaskSetBtns = ctrl.maskSetButtons; //0x0000189C
    curMaskSupBtns = ctrl.maskSupportButtons; //0x000018A0

    if (curMaskSupBtns & btnMask) { //0x000018AC
        btnMaskMode = (curMaskSetBtns & btnMask) ? PSP_CTRL_MASK_IGNORE_BUTTON_MASK : PSP_CTRL_MASK_DELETE_BUTTON_MASK_SETTING; //0x000018B0 & 0x000018B8
    }

    sceKernelCpuResumeIntr(suspendFlag);
    return btnMaskMode;
}

/* Subroutine sceCtrl_driver_F8346777 - Address 0x000017C4
 * Exported in sceCtrl_driver
 */
PspCtrlPadButtonMaskMode sceCtrlSetButtonIntercept(u32 mask, PspCtrlPadButtonMaskMode buttonMaskMode) {
    int suspendFlag;
    PspCtrlPadButtonMaskMode prevBtnMaskMode = PSP_CTRL_MASK_IGNORE_BUTTON_MASK;

    suspendFlag = sceKernelCpuSuspendIntr(); //0x000017E0

    if (mask & ctrl.maskSupportButtons) { //0x00001800
        prevBtnMaskMode = (mask & ctrl.maskSetButtons) ? PSP_CTRL_MASK_SET_BUTTON_MASK : PSP_CTRL_MASK_DELETE_BUTTON_MASK_SETTING; //0x00001808 & 0x00001810
    }
    if (buttonMaskMode != PSP_CTRL_MASK_DELETE_BUTTON_MASK_SETTING) { //0x00001814
        if (buttonMaskMode == PSP_CTRL_MASK_IGNORE_BUTTON_MASK) { //0x00001818 & 0x00001850
            ctrl.maskSupportButtons &= ~mask; //0x00001854 & 0x00001874
            ctrl.maskSetButtons &= ~mask; //0x00001850 & 0x0000186C
        }
        else if (buttonMaskMode == PSP_CTRL_MASK_SET_BUTTON_MASK) { //0x0000185C
            ctrl.maskSupportButtons |= mask; //0x00001868
            ctrl.maskSetButtons |= mask; //0x00001820
        }
    }
    else {
        ctrl.maskSupportButtons &= ~mask; //0x000017F8 & 0x00001804 & 0x0000181C
        ctrl.maskSetButtons |= mask; //0x000017E4 & 0x000017F0 & 0x00001820
    }

    sceKernelCpuResumeIntr(suspendFlag); //0x0000182C
    return prevBtnMaskMode;
}

/* Subroutine sceCtrl_driver_DF53E160 - Address 0x00001D04
 * Exported in sceCtrl_driver
 */
int sceCtrlSetSpecialButtonCallback(u32 slot, u32 btnMask, SceCtrlCb cb, void *opt) {
    int suspendFlag;

    if (slot > CTRL_BUTTON_CALLBACK_MAX_SLOT) { //0x00001D14 & 0x00001D34
        return SCE_ERROR_INVALID_INDEX;
    }
    suspendFlag = sceKernelCpuSuspendIntr(); //0x00001D3C

    ctrl.buttonCallback[slot].btnMask = btnMask; //0x00001D54
    ctrl.buttonCallback[slot].callbackFunc = cb; //0x00001D58
    ctrl.buttonCallback[slot].arg = opt; //0x00001D64
    ctrl.buttonCallback[slot].gp = pspGetGp(); //0x00001D6C

    sceKernelCpuResumeIntr(suspendFlag); //0x00001D68
    return SCE_ERROR_OK;
}

/*
 * Subroutine sceCtrl_driver_6C86AF22 - Address 0x00001AA8
 * Exported in sceCtrl_driver
 */
int sceCtrl_driver_6C86AF22(int *arg0) {
    ctrl.unk_9 = (int)arg0; //0x00001AB4
    return SCE_ERROR_OK;
}

/*
 * Subroutine sceCtrl_driver_7511CCFE - Address 0x00001638
 * Exported in sceCtrl_driver
 */
int sceCtrlGetIdleCancelKey(int *arg1, int *arg2, int *arg3, int *arg4) {
    if (arg1 != NULL) { //0x00001638
        *arg1 = ctrl.unk_5; //0x00001644
    }
    if (arg2 != NULL) { //0x00001648
        *arg2 = ctrl.unk_6; //0x00001654
    }
    if (arg3 != NULL) { //0x00001658
        *arg3 = ctrl.unk_7; //0x00001664
    }
    if (arg4 != NULL) { //0x00001668
        *arg4 = ctrl.unk_8; //0x00001674
    }
    return SCE_ERROR_OK; //0x0000167C
}

/*
 * sceCtrl_driver_6A1DF4CB - Address 0x00001C10
 * Exported in sceCtrl_driver
 */
int sceCtrlSetIdleCancelKey(int arg1, int arg2, int arg3, int arg4) {
    ctrl.unk_8 = arg4; //0x00001C1C
    ctrl.unk_5 = arg1; //0x00001C20
    ctrl.unk_6 = arg2; //0x00001C24
    ctrl.unk_7 = arg3; //0x00001C2C

    return SCE_ERROR_OK; //0x00001C18
}

/*
 * Subroutine sceCtrl_driver_5886194C - Address 0x00001A98
 * Exported in sceCtrl_driver
 */
int sceCtrl_driver_5886194C(char arg1) {
    ctrl.unk_Byte_3 = arg1; //0x00001AA4
    return SCE_ERROR_OK;
}

/*
 * sceCtrl_driver_365BE224 - Address 0x00001D94
 * Exported in sceCtrl_driver
 */
int sceCtrlUpdateCableTypeReq(void) {
    int sysconStatus;
    int status;

    if (ev_id == 0x402) { //0x0000036C
        //0x0000041C
        sysconStatus = ctrl.sysconHwDataTransferBusy;
        sysconStatus |= ctrl.unk_Byte_7;
        sysconStatus |= ctrl.resv[0];
        sysconStatus |= ctrl.resv[1];
        if (sysconStatus == 0) { //0x00000420
            return SCE_ERROR_OK;
        }
        if (ctrl.unk_1 == 0) { //0x00000430
            return SCE_ERROR_OK;
        }
        return SCE_ERROR_BUSY;
    }
    if (ev_id < 0x403) { //0x00000378
        if (ev_id == 0x400) { //0x00000384
            ctrl.unk_1 = ctrl.suspendSamples; //0x000003AC
        }
        return SCE_ERROR_OK;
    }
    else if (ev_id == 0x400C) { //0x0000037C & 0x000003B0 -- going into sleep mode?
             status = sceCtrlSuspend();
             return status;
    }
    else {  //resuming from sleep mode?
        if (ev_id == 0x1000C) { //0x000003BC
            sceCtrlResume();
            //sceCtrlResume already does that!
            ctrl.unk_1 = -1; //0x000003D8
        }
        return SCE_ERROR_OK;
    }
}

/* sub_00001DD8 */
/**
 * The alarm handler. It updates the internal user and kernel buffers of the ctrl module
 *                    with the buttons placed in the 'pureButton' members.
 *
 * @param common An optional argument passed with the register function.
 *
 * @return 0.
 */
static SceUInt _sceCtrlDummyAlarm(void *common __attribute__((unused))) {
    int suspendFlag;
    u32 pureButtons;

    suspendFlag = sceKernelCpuSuspendIntr(); //0x00001DE8
    pureButtons = ctrl.pureButtons; //0x00001E00
    if (ctrl.unk_1 == 0) { //0x00001DF8
        pureButtons &= 0xFFFE0000; //0x00001E04 -- kernel button values?
    }
    _sceCtrlUpdateButtons(pureButtons, ctrl.analogX, ctrl.analogY); //0x00001E10
    sceKernelSetEventFlag(ctrl.eventFlag, 1); //0x00001E1C
    sceKernelCpuResumeIntr(suspendFlag); //0x00001E24

    return SCE_ERROR_OK;
}

/* sub_00000440 */
/**
 * This function is called whenever the VBlank interrupt is enabled and when that interrupt occurs (60 times/second).
 *
 * @param subIntNm The sub interrupt handler number passed via the register function.
 * @param arg An argument passed to the interrupt handler via the register function.
 *
 * @return -1.
 */
static int _sceCtrlVblankIntr(int subIntNm __attribute__((unused)), void *arg __attribute__((unused))) {
    int suspendFlag;
    int retVal;

    suspendFlag = sceKernelCpuSuspendIntr(); //0x00000454
    if (ctrl.btnCycle == 0) { //0x00000464
        if (ctrl.sysconHwDataTransferBusy == 0) { //0x00000470
            if (ctrl.pollMode != PSP_CTRL_POLL_NO_POLLING) { //0x0000047C
                ctrl.sysconHwDataTransferBusy = 1; //0x000004E0

                if ((ctrl.samplingMode[USER_MODE] | ctrl.samplingMode[KERNEL_MODE]) == PSP_CTRL_INPUT_DIGITAL_ONLY) { //0x000004E4
                    ctrl.sysPacket[0].tx_cmd = SYSCON_CTRL_TRANSFER_DATA_DIGITAL_ONLY; //0x000004E8
                }
                else {
                    ctrl.sysPacket[0].tx_cmd = SYSCON_CTRL_TRANSFER_DATA_DIGITAL_ANALOG; //0x000004EC
                }
                ctrl.sysPacket[0].tx_len = 2; //0x00000514

                retVal = sceSysconCmdExecAsync(&ctrl.sysPacket[0], 1, _sceCtrlSysconCmdIntr1, 0); //0x00000510
                if (retVal < 0) { //0x00000518
                    ctrl.sysconHwDataTransferBusy = 0; //0x0000051C
                }
            }
            else {
                //register an alarm occurring every 700 micro seconds
                sceKernelSetAlarm(700, _sceCtrlDummyAlarm, NULL); //0x00000490
            }
        }
        else {
            sceKernelSetAlarm(700, _sceCtrlDummyAlarm, NULL); //0x00000490
        }
    }
    if (ctrl.unk_Byte_2 != 0) { //0x000004A0
        ctrl.unk_Byte_2 = 0; //0x000004A4
        sceKernelPowerTick(SCE_KERNEL_POWER_TICK_DEFAULT); //0x000004CC
    }

    sceKernelCpuResumeIntr(suspendFlag); //0x000004A8
    return -1;
}

/* sub_00000528 */
/**
 * The custom timer interrupt handler. Will be executed every 'ctrl.cycle' micro seconds.
 * Updates the 'pureButton' SceCtrl structure field via a SYSCON hardware transfer or uses the current value of that field
 * to update the internal SceCtrl data buffers.
 *
 * @return -1.
 */
static int _sceCtrlTimerIntr(int unused0 __attribute__((unused)), int unused1 __attribute__((unused)), int unused2 __attribute__((unused)), int unused3 __attribute__((unused))) {
    u8 sysconReqCtrlData;
    int suspendFlag;
    int retVal;

    sysconReqCtrlData = SYSCON_CTRL_TRANSFER_DATA_DIGITAL_ONLY;
    suspendFlag = sceKernelCpuSuspendIntr(); //0x0000053C

    if (ctrl.btnCycle != 0) { //0x00000548 & 0x0000054C
        if ((ctrl.sysconHwDataTransferBusy == 0) && (ctrl.pollMode != CTRL_POLL_MODE_OFF)) { //0x00000558 & 0x00000564
            ctrl.sysconHwDataTransferBusy = 1; //0x000005C8

            if (ctrl.samplingMode[USER_MODE] != PSP_CTRL_INPUT_DIGITAL_ONLY) { //0x000005CC
                sysconReqCtrlData = SYSCON_CTRL_TRANSFER_DATA_DIGITAL_ANALOG; //0x000005D4
            }
            ctrl.sysPacket[0].tx_cmd = sysconReqCtrlData; //0x000005D8

            retVal = sceSysconCmdExecAsync(&ctrl.sysPacket[0], 1, _sceCtrlSysconCmdIntr1, 0); //0x000005F8
            if (retVal < 0) { //0x00000600
                ctrl.sysconHwDataTransferBusy = 0; //0x00000604
            }
        }
        else {
             sceKernelSetAlarm(700, _sceCtrlDummyAlarm, NULL); //0x00000578
        }
    }
    if (ctrl.unk_Byte_2 != 0) { //0x00000588
        ctrl.unk_Byte_2 = 0; //0x0000058C
        sceKernelPowerTick(SCE_KERNEL_POWER_TICK_DEFAULT); //0x000005B4
    }

    sceKernelCpuResumeIntr(suspendFlag); //0x00000590
    return -1; //0x000005A8
}

/* sub_00000610 */
/**
 * This function receives a SYSCON packet with the transferred button/analog data from the hardware registers.
 * It updates the 'pureButtons' SceCtrl structure field as well as the analog data and the internal controller data buffers.
 *
 * @param sysPacket The SYSCON packet with the new button/analog data.
 *
 * @return 0.
 */
static int _sceCtrlSysconCmdIntr1(SceSysconPacket *sysPacket) {
    u32 pureButtons;
    int suspendFlag;
    int res;
    u32 nButtons;
    u32 tmpButtons;
    u8 analogX;
    u8 analogY;
    u8 idleVal;
    u8 checkVal;

    ctrl.sysconHwDataTransferBusy = 0; //0x00000644
    if (ctrl.unk_1 > 0) { //0x00000640
        ctrl.unk_1--; //0x00000648
    }

    if (ctrl.unk_1 == 0) { //0x00000650
        suspendFlag = sceKernelCpuSuspendIntr(); //0x00000918
        if (ctrl.unk_1 == 0) { //0x00000924
            pureButtons = ctrl.pureButtons & 0xFFFE0000; //0x00000930
        }
        else {
            pureButtons = ctrl.pureButtons; //0x00000964
        }
        _sceCtrlUpdateButtons(pureButtons, ctrl.analogX, ctrl.analogY); //0x0000093C
        sceKernelSetEventFlag(ctrl.eventFlag, 1); //0x00000948
        sceKernelCpuResumeIntr(suspendFlag); //0x00000950

        return SCE_ERROR_OK; //0x00000820
    }
    //TODO: Reverse of sceSysconCmdExecAsync to get structure members!
    else {
        tmpButtons = ctrl.pureButtons; //0x00000674 & 0x000008A8
        if (sysPacket->tx_cmd != 2 && sysPacket->tx_cmd != 6) { //...0x00000670
            nButtons = tmpButtons;
            if (sysPacket->tx_cmd >= 7 && sysPacket->tx_cmd <= 8) { //0x000008A4
                nButtons = (((sysPacket->rx_data[3] & 3) << 28) 
                          | ((sysPacket->rx_data[2] & 0xBF) << 20)
                          | ((sysPacket->rx_data[1] & 0xF0) << 12)
                          | ((sysPacket->rx_data[0] & 0xFF) << 8)
                          | ((sysPacket->rx_data[1] & 6) << 7)
                          | ((sysPacket->rx_data[0] & 0xF) << 4)
                          | (sysPacket->rx_data[1] & 9)) 
                    ^ 0x20F7F3F9; //0x00000914
            }
        }
        else {
            nButtons = ((((sysPacket->rx_data[1] & 0xF0) << 12) // 0x00000678...
                       | ((sysPacket->rx_data[0] & 0xF0) << 8)
                       | ((sysPacket->rx_data[1] & 0x6) << 7)
                       | ((sysPacket->rx_data[0] & 0xF) << 4)
                       | (sysPacket->rx_data[1] & 0x9)) 
                    ^ 0x7F3F9) 
                    | (ctrl.pureButtons & 0xFFF00000); //0x000006C8
        }
        ctrl.pureButtons = nButtons; //0x000006D8

        //analogPadValues passed from hw-Regs via syscon to ctrl
        if (sysPacket->tx_cmd == 3) { //0x000006D4
            analogY = sysPacket->rx_data[1]; //0x00000890
            analogX = sysPacket->rx_data[0]; //0x00000898
        }
        else if (sysPacket->tx_cmd == 6) { //0x000006E0
            analogY = sysPacket->rx_data[3]; //0x00000884
            analogX = sysPacket->rx_data[2]; //0x0000088C
        }
        else if (sysPacket->tx_cmd == 8) {
            analogY = sysPacket->rx_data[5]; //0x000006EC
            analogX = sysPacket->rx_data[4]; //0x0000088C
        }
        else {
            analogY = ctrl.analogY; //0x000006F0
            analogX = ctrl.analogX; //0x000006F4
        }
        ctrl.analogX = analogX; //0x000006FC
        ctrl.analogY = analogY; //0x00000708
        _sceCtrlUpdateButtons(nButtons, analogX, analogY); //0x0000070C

        if (sysPacket->tx_data[6] == 0) { //0x00000718
            int unk1, unk2;
            tmpButtons ^= nButtons; //0x0000072C
            if ((nButtons & 0x20000000) == 0) { //0x00000728
                unk1 = ctrl.unk_5; //0x0000086C
                unk2 = ctrl.unk_6; //0x00000870
                idleVal = ctrl.idleReset; //0x00000878
            }
            else {
                unk1 = ctrl.unk_7; //0x00000730
                unk2 = ctrl.unk_8; //0x00000734
                idleVal = ctrl.idleBack; //0x00000738
            }
            unk1 &= tmpButtons; //0x0000073C
            unk2 &= nButtons; //0x00000740
            unk1 = unk1 | unk2; //0x00000744

            checkVal = unk1 & ctrl.unk_4; //0x00000750
            if (checkVal == 0 && ctrl.samplingMode[USER_MODE] == PSP_CTRL_INPUT_DIGITAL_ANALOG) { //0x00000754 & 0x00000760
                int xMove = pspMax(analogX - 128, -analogX + 128) & 0xFF; //0x00000764 & 0x00000770 & 0x00000774
                int yMove = pspMax(analogY - 128, -analogY + 128) & 0xFF; //0x0000076C & 0x00000778 & 0x00000780 & 0x00000788
                xMove = (xMove == 127) ? 128 : xMove; //0x00000774 & 0x0000078C
                yMove = (yMove == 127) ? 128 : yMove; //0x0000079C

                if (xMove >= idleVal || yMove >= idleVal) { //0x00000798 & 0x000007A4
                    checkVal = 1; //0x000007AC
                }
            }
            if (checkVal != 0) { //0x000007B0
                ctrl.unk_Byte_2 = 1; //0x000007BC
            }
        }
        u8 sampling = (ctrl.samplingMode[USER_MODE] | ctrl.samplingMode[KERNEL_MODE]) != 0; //0x000007C8 & 0x000007D4
        sampling = (ctrl.unk_Byte_0 != 0) ? (sampling | 2) : sampling; //0x000007D8 & 0x000007DC

        if (sampling != ctrl.unk_Byte_1 && ctrl.unk_Byte_7 == 0) { //0x000007E0 & 0x000007EC
            ctrl.unk_Byte_7 = 1; //0x00000834
            ctrl.sysPacket[1].tx_cmd = 51; //0x0000082C & 0x00000840
            ctrl.sysPacket[1].tx_len = 3; //0x00000830 & 0x0000084C
            ctrl.sysPacket[1].tx_data[0] = sampling; //0x00000858

            res = sceSysconCmdExecAsync(&ctrl.sysPacket[1], 0, _sceCtrlSysconCmdIntr2, 0); //0x00000854
            if (res < 0) { //0x0000085C
                ctrl.unk_Byte_7 = 0; //0x00000860
            }
        }
        sceKernelSetEventFlag(ctrl.eventFlag, 1); //0x000007FC
        return SCE_ERROR_OK;
    }
}

/* sub_00001E4C */
static int _sceCtrlSysconCmdIntr2(void) {
    ctrl.unk_Byte_0 = 0; //0x00001E5C
    ctrl.unk_Byte_1 = ctrl.sysPacket[1].tx_data[0] & 0x1; //0x00001E64
    ctrl.unk_Byte_7 = 0; //0x00001E6C

    return SCE_ERROR_OK;
}

/* sub_00000968 */
static int _sceCtrlUpdateButtons(u32 pureButtons, u8 aX, u8 aY) {
    SceCtrlData *ctrlUserBuf;
    SceCtrlData *ctrlKernelBuf;
    SceCtrlDataExt *ctrlKernelBufExt;
    SceCtrlDataExt *ctrlUserBufExt;
    u32 sysTimeLow;
    u32 buttons;
    u32 curButtons;
    u32 prevButtons;
    u32 pressedButtons;
    u32 index;
    int tempIndex;
    int tempIndex2;
    u8 analogX;
    u8 analogY;
    u8 tempAnalogX;
    u8 tempAnalogX2;
    u8 tempAnalogY;
    u8 tempAnalogY2;
    int (*func)(int);
    int ret;
    int res;
    int storeData;
    u32 btnMask;
    /** User mode buttons being pressed. */
    u32 pressedUserButtons;
    /** Newly pressed buttons, were not pressed before immediately. */
    u32 newPressedButtons;
    /** Newly unpressed buttons, were pressed before immediately. */
    u32 newUnpressedButtons;
    /** Pure pressed buttons, which already went through modification process. */
    u32 updatedButtons;
    u32 updatedButtons2;
    /** Records buttons which are currently not being pressed. */
    u32 unpressedButtons;
    /** 0 = HOLD mode on, 1 = HOLD mode off. */
    u32 holdButton;
    /** Merge of pressed buttons during the last VBlank event and the current one.
     *  Buttons, which have been pressed both the last VBlank event and the current one are turned OFF. */
    u32 mergedButtons;
    u32 uModeBtnEmulation;
    u32 uModeBtnEmulationAll;
    u32 kModeBtnEmulation;
    u32 kModeBtnEmulationAll;
    u32 intCtrlBufUpdates;
    u8 ctrlBufferUpdatesLeft;
    /** The apply mode for the requested button(s) of a rapid fire event. 0 = reqButton will be set ON the next buffer update,
     *  2 = reqButton(s) will be set ON, Other values = reqButton(s) will be set OFF. */
    u8 reqButtonEventMode;
    u8 rapidFireEventData;
    SceCtrlCb btnCbFunc;
    u32 gp; /* global pointer */
    _Bool check = 0;

    sysTimeLow = sceKernelGetSystemTimeLow(); //0x000009A0

    //User buffer
    index = ctrl.userModeData.ctrlBufIndex; //0x000009B4 & 0x000009C4
    ctrlUserBuf = ((SceCtrlData *)ctrl.userModeData.sceCtrlBuf[0] + index); //0x000009CC
    ctrlUserBuf->activeTime = sysTimeLow; //0x000009D0

    //Kernel buffer
    index = ctrl.kernelModeData.ctrlBufIndex; //0x000009C0 & 0x000009D4
    ctrlKernelBuf = ((SceCtrlData *)ctrl.kernelModeData.sceCtrlBuf[0] + index); //0x000009D8
    ctrlUserBuf->activeTime = sysTimeLow; //x000009E8

    analogX = aX; //0x0000097C
    analogY = aY; //0x00000974

    int i;
    for (i = 0; i < CTRL_DATA_EMULATION_SLOTS; i++) { //0x00000A08 & 0x000009F0
        if (ctrl.emulatedData[i].intCtrlBufUpdates > 0) { //0x000009F4
            ctrl.emulatedData[i].intCtrlBufUpdates--; //0x000009F8
            analogX = ctrl.emulatedData[i].analogXEmulation; //0x00000A00
            analogY = ctrl.emulatedData[i].analogYEmulation; //0x00000A04
        }
    }
    ctrlUserBuf->aX = analogX; //0x00000A10
    ctrlUserBuf->aY = analogY; //0x00000A1C
    ctrlKernelBuf->aX = analogX; //0x00000A20
    ctrlKernelBuf->aY = analogY; //0x00000A24

    //User Mode
    if (ctrl.samplingMode[USER_MODE] == PSP_CTRL_INPUT_DIGITAL_ONLY) { //0x00000A2C
        ctrlUserBuf->aY = CTRL_ANALOG_PAD_DEFAULT_VALUE; //0x00000A38
        ctrlUserBuf->aX = CTRL_ANALOG_PAD_DEFAULT_VALUE; //0x00000A3C
    }
    //Kernel Mode
    if (ctrl.samplingMode[KERNEL_MODE] == PSP_CTRL_INPUT_DIGITAL_ONLY) { //0x00000A44
        ctrlKernelBuf->aY = CTRL_ANALOG_PAD_DEFAULT_VALUE; //0x00000A50
        ctrlKernelBuf->aX = CTRL_ANALOG_PAD_DEFAULT_VALUE; //0x00000A54
    }
    tempAnalogX = 0;
    //0x00000A6C && 0x00000A7C && 0x00000A88
    for (i = 0; i < (int)sizeof ctrlUserBuf->rsrv; i++) {
         ctrlUserBuf->rsrv[i] = 0;
    }
    tempAnalogY = 0;
    //0x00000A8C && 0x00000AA0 && 0x00000AA4
    for (i = 0; i < (int)sizeof ctrlKernelBuf->rsrv; i++) {
         ctrlKernelBuf->rsrv[i] = 0;
    }
    //0x00000AB8 && 0x00000AC0
    for (i = 0; i < 2; i++) {
         if (ctrl.unk_array2[i+1] != NULL) { //0x00000AAC
             ctrlKernelBufExt = (SceCtrlDataExt *)ctrl.kernelModeData.sceCtrlBuf[i+1]; //0x00000AB0 && 0x00000ABC
             ctrlKernelBufExt = ctrlKernelBufExt + sizeof(SceCtrlDataExt); //0x00000F48
             ctrlKernelBufExt->activeTime = sysTimeLow; //0x00000F68

             //check that back
             func = (ctrl.unk_array2[i+1]->func); //0x00000F5C
             ret = func(ctrl.unk_array3[i+1]); //0x00000F60
             if (ret < 0) { //0x00000F78
                 //0x000011AC && 0x000011B8
                 int j;
                 for (j = 0; j < (int)sizeof ctrlKernelBufExt->rsrv; j++) {
                     ctrlKernelBufExt->rsrv[j] = 0;
                 }
                 ctrlKernelBufExt->activeTime = 0; //0x000011B0
                 ctrlKernelBufExt->buttons = 0; //0x000011B4
                 ctrlKernelBufExt->unk1 = 0; //0x000011BC
                 ctrlKernelBufExt->unk2 = 0; //0x000011C0
                 ctrlKernelBufExt->unk3 = 0;
                 ctrlKernelBufExt->unk4 = 0;
                 ctrlKernelBufExt->unk5 = 0;
                 ctrlKernelBufExt->unk6 = 0;
                 ctrlKernelBufExt->unk7 = 0;
                 ctrlKernelBufExt->unk8 = 0; //0x000011D8
                 ctrlKernelBufExt->aX = CTRL_ANALOG_PAD_DEFAULT_VALUE; //0x00000A80 && 0x000011DC
                 ctrlKernelBufExt->aY = CTRL_ANALOG_PAD_DEFAULT_VALUE; //0x000011E0
                 ctrlKernelBufExt->rsrv[0] = -128; //0x000011E4
                 ctrlKernelBufExt->rsrv[1] = -128; //0x000011EC
             }
             else {
                 if (ctrl.unk_Byte_2 == 0) { //0x00000F8C
                     res = ctrlKernelBufExt->buttons ^ ctrl.unk_array_3[i]; //0x00000F9C
                     res = res | ctrlKernelBufExt->buttons; //0x00000FA0
                     res &= 0x1FFFF;//0x00000FA4 -- User mode buttons?
                     ctrl.unk_array_3[i] = ctrlKernelBufExt->buttons; //0x00000FAC
                     if (res != 0) { //0x00000FA8
                         ctrl.unk_Byte_2 = 1; //0x00000FBC
                     }
                 }
                 if ((ctrl.unk_Byte_2 == 0) && (ctrl.samplingMode[USER_MODE] == PSP_CTRL_INPUT_DIGITAL_ANALOG)) { //0x00000FCC &&  0x00000FD4
                     analogX = ctrlKernelBufExt->aX - 128; //0x00000FE0 & 0x00000FEC
                     analogY = ctrlKernelBufExt->aY - 128; //0x00000FE4 & 0x00000FF0
                     tempAnalogX = -analogX; //0x00000FF4
                     tempAnalogY = -analogY; //0x00000FF8
                     int moveX = pspMax(analogX, tempAnalogX); //0x00001004
                     tempAnalogX = moveX;
                     int moveY = pspMax(analogY, tempAnalogY); //0x00001008
                     tempAnalogY = moveY;

                     int minIdleReset = (ctrl.idleReset < 37) ? 37 : ctrl.idleReset; //0x0000100C & 0x00001020
                     moveX = (tempAnalogX == 127) ? 128 : moveX; //0x00001014 & 0x00001024
                     moveY = (tempAnalogY == 127) ? 128 : moveY; //0x00001018 & 0x00001030

                     int moveX2 = pspMax(ctrlKernelBufExt->rsrv[0] - 128, -ctrlKernelBufExt->rsrv[0] + 128); //0x00000FEC & 0x00001028 & 0x00001044
                     int moveY2 = pspMax(ctrlKernelBufExt->rsrv[1] - 128, -ctrlKernelBufExt->rsrv[1] + 128); //0x00000102C & 0x00001048
                     moveX2 = (moveX2 == 127) ? 128 : moveX2; //0x00001054 & 0x00001068
                     moveY2 = (moveY2 == 127) ? 128 : moveY2; //0x00001058 & 0x00001060

                     storeData = 1;
                     if (moveY < minIdleReset && moveX < minIdleReset //0x00001034 & 0x00001038 & 0x0000104C & 0x00001050 & 0x0000105C & 0x00001064
                      && moveY2 < minIdleReset && moveX2 < minIdleReset) { //0x0000106C - 0x00001080
                         storeData = 0;
                     }
                     //0x00001088
                     if (storeData) {
                         ctrl.unk_Byte_2 = 1; //0x0000108C
                     }
                 }
             }
             //*** start of code: 0x00001094 -- 0x000010D0 ***
             ctrlUserBufExt = ((SceCtrlDataExt **)ctrl.userModeData.sceCtrlBuf[1])[i]; //0x00000FD0 | 0x00000FDC | 0x00001090 & 0x000010A0 -> check?
             ctrlUserBufExt->activeTime = ctrlKernelBufExt->activeTime; //0x000010A8 & 0x000010B8
             ctrlUserBufExt->buttons = ctrlKernelBufExt->buttons; //0x000010AC & 0x000010C4
             ctrlUserBufExt->aX = ctrlKernelBufExt->aX; //0x000010B0 & 0x000010C8
             ctrlUserBufExt->aY = ctrlKernelBufExt->aY; //0x000010B0 & 0x000010C8

             //0x000010B0 & 0x000010C8 & 0x000010B4 & 0x000010D0
             int j;
             for (j = 0; j < (int)sizeof ctrlUserBufExt->rsrv; j++) {
                  ctrlUserBufExt->rsrv[j] = ctrlKernelBufExt->rsrv[j];
             }
             ctrlUserBufExt->unk1 = ctrlKernelBufExt->unk1;
             ctrlUserBufExt->unk2 = ctrlKernelBufExt->unk2;
             ctrlUserBufExt->unk3 = ctrlKernelBufExt->unk3;
             ctrlUserBufExt->unk4 = ctrlKernelBufExt->unk4;
             ctrlUserBufExt->unk5 = ctrlKernelBufExt->unk5;
             ctrlUserBufExt->unk6 = ctrlKernelBufExt->unk6;
             ctrlUserBufExt->unk7 = ctrlKernelBufExt->unk7;
             ctrlUserBufExt->unk8 = ctrlKernelBufExt->unk8;
             //*** end of code: 0x00001094 -- 0x000010D0 ***

             buttons = ctrlUserBufExt->buttons; //0x000010D4
             ctrlUserBufExt->buttons = buttons & 0xFFFEFFFF; //0x000010E0 && 0x000010E4
             ctrlUserBufExt->buttons = ctrlUserBufExt->buttons & ctrl.maskSupportButtons; //0x000010F0
             ctrlUserBufExt->buttons = ctrlUserBufExt->buttons | ctrl.maskSetButtons; //0x000010F0

             if ((ctrl.unk_9 >> i) == 1) { //0x00001100 & 0x00001104 & 0x00001108 && 0x0000110C
                 buttons = ctrlKernelBufExt->buttons; //0x00001114
                 ctrl.emulatedData[i+1].intCtrlBufUpdates2 = 1; //0x0000111C
                 //Why that?
                 kModeBtnEmulation = (buttons & 0xFFFFF0FF) //0x00001134
                                   | (((buttons & 0x500) != 0) << 8) //0x00001120 & 0x00001128 & 0x00001138
                                   | (((buttons & 0xA00) != 0) << 9); //0x00001124 & 0x00001130 & 0x00001140 & 0x0000113C & 0x00001144
                 uModeBtnEmulation = kModeBtnEmulation & ctrl.userModeButtons; //0x00001148
                 ctrl.emulatedData[i+1].uModeBtnEmulation = uModeBtnEmulation; //0x0000114C
                 ctrl.emulatedData[i+1].kModeBtnEmulation = kModeBtnEmulation; //0x00001150

                 analogX = ctrlKernelBufExt->aX - 128; //0x00001154 & 0x00001158
                 tempAnalogX = -analogX; //0x0000115C
                 analogX = pspMax(analogX, tempAnalogX); //0x00001160
                 analogY = ctrlKernelBufExt->aY; //0x0000116C
                 if (analogX < 38
                    && pspMax((analogY & 0xFF) - 128, -(analogY & 0xFF) + 128) >= 38) { //0x00001164 & 0x00001168 & 0x00001170 & 0x00001174 & 0x00001178 & 0x00001180 & 0x00001184
                     check = 1; //0x00001194
                     tempAnalogY = analogY & 0xFF; //0x00001190
                     tempAnalogY -= 128; //0x000011A0
                     tempAnalogX -= 128; //0x000011A8
                 }
             }
         }
    }
    //0x00000AC8
    if ((tempAnalogX | tempAnalogY) != 0) { //0x00000ACC
        tempAnalogX2 = analogX - 65; //0x00000AD4
        tempAnalogX2 = tempAnalogX + analogX; //0x00000AE0
        storeData = 1;
        if (tempAnalogX2 < 127) { //0x00000AD8 & 0x00000ADC
            tempAnalogY2 = analogY - 65; //0x00000AE4
            analogX = 255; //0x00000AF0
            if (tempAnalogY2 < 127) { //0x00000AE8 & 0x00000AEC
                analogX = CTRL_ANALOG_PAD_DEFAULT_VALUE; //0x00000AF4
                analogY = CTRL_ANALOG_PAD_DEFAULT_VALUE; //0x00000AF8
                tempAnalogX2 = tempAnalogX + analogX; //0x00000AFC
            }
            else {
                storeData = 0; /* simple check to not break the ASM code. */
            }
        }
        if (storeData) {
            analogX = CTRL_ANALOG_PAD_MAX_VALUE; //0x00000B00
        }
        tempAnalogY += analogY; //0x00000B04
        tempAnalogX = pspMin(tempAnalogX2, analogX); //0x00000B0C
        if ((char)tempAnalogX2 < CTRL_ANALOG_PAD_MIN_VALUE) { //0x00000B08
            tempAnalogX = CTRL_ANALOG_PAD_MIN_VALUE; //0x00000F38
        }
        analogY = CTRL_ANALOG_PAD_MAX_VALUE; //0x00000B10
        tempAnalogY2 = pspMin(tempAnalogY, analogY); //0x00000B18
        if ((char)tempAnalogY < CTRL_ANALOG_PAD_MIN_VALUE) { //0x00000B14
            tempAnalogY2 = CTRL_ANALOG_PAD_MIN_VALUE; //0x00000F30
        }

        //User Mode -- analog input mode
        if (ctrl.samplingMode[USER_MODE] == PSP_CTRL_INPUT_DIGITAL_ANALOG) { //0x00000B28
            ctrlUserBuf->aX = tempAnalogX; //0x00000B30
            ctrlUserBuf->aY = tempAnalogY2; //0x00000B34
        }
        //Kernel Mode -- analog input mode
        if (ctrl.samplingMode[KERNEL_MODE] == PSP_CTRL_INPUT_DIGITAL_ANALOG) { //0x00000B38
            ctrlKernelBuf->aX = tempAnalogX; //0x00000B44
            ctrlKernelBuf->aY = tempAnalogY2; //0x00000B48
        }
    }
    //0x00000B4C
    index = ctrl.userModeData.ctrlBufIndex + 1; //0x00000AD0 & 0x00000B40 -- the index is stored into $s1.
    tempIndex = (int)index >> 31; //0x00000B4C
    tempIndex = tempIndex >> 26; //0x00000B54
    tempIndex = index + tempIndex; //0x00000B5C
    tempIndex &= 0xFFFFFFC0; //0x00000B64
    index = index - tempIndex; //0x00000B68
    ctrl.userModeData.ctrlBufIndex = index; //0x00000B70
    if (index == ctrl.userModeData.ctrlBufStartIndex) { //0x00000B6C
        tempIndex = index++; //0x00000F0C
        tempIndex2 = tempIndex >> 31; //0x00000F10
        tempIndex2 = tempIndex2 >> 26; //0x00000F14
        tempIndex2 += tempIndex; //0x00000F14
        tempIndex2 &= 0xFFFFFFC0; //0x00000F1C
        tempIndex -= tempIndex2; //0x00000F20
        ctrl.userModeData.ctrlBufStartIndex = tempIndex; //0x00000F28
    }
    index = ctrl.kernelModeData.ctrlBufIndex +1; //0x000009C0 & 0x00000B74 -- the index is stored into $s3.
    tempIndex = (int)index >> 31; //0x00000B78
    tempIndex = tempIndex >> 26; //0x00000B7C
    tempIndex = index + tempIndex; //0x00000B80
    tempIndex &= 0xFFFFFFC0; //0x00000B88
    index = index - tempIndex; //0x00000B8C
    ctrl.kernelModeData.ctrlBufIndex = index; //0x00000B94
    if (index == ctrl.kernelModeData.ctrlBufStartIndex) { //0x00000B90
        tempIndex = index++; //0x00000EEC
        tempIndex2 = tempIndex >> 31; //0x00000EF0
        tempIndex2 = tempIndex2 >> 26; //0x00000EF4
        tempIndex2 += tempIndex; //0x00000EF8
        tempIndex2 &= 0xFFFFFFC0; //0x00000EFC
        tempIndex -= tempIndex2; //0x00000F00
        ctrl.kernelModeData.ctrlBufStartIndex = tempIndex; //0x00000F08
    }
    //if HOLD mode is active
    updatedButtons = pureButtons;
    if (pureButtons & PSP_CTRL_HOLD) { //0x00000BA4
        updatedButtons &= CTRL_PSP_HARDWARE_IO_BUTTONS; //0x00000BB0 -- only I/O Buttons permitted?
    }
    //0x00000BB8 & 0x00000BC0 & 0x00000BC8
    for (i = 0; i < CTRL_DATA_EMULATION_SLOTS; i++) {
         updatedButtons = updatedButtons | ctrl.emulatedData[i].kModeBtnEmulation; //0x00000BBC & 0x00000BCC
    }
    ctrlKernelBuf->buttons = updatedButtons; //0x00000BD0

    unpressedButtons = ~updatedButtons; //0x00000BD4
    mergedButtons = ctrl.prevModifiedButtons ^ updatedButtons; //0x00000BFC

    pressedButtons = ctrl.kernelModeData.btnPress | updatedButtons; //0x00000C00
    newPressedButtons = mergedButtons & updatedButtons; //0x00000C04
    newUnpressedButtons = mergedButtons & unpressedButtons; //0x00000C08
    ctrl.prevModifiedButtons = updatedButtons; //0x00000C0C
    updatedButtons = pureButtons & ctrl.maskSupportButtons; //0x00000C10

    newPressedButtons = ctrl.kernelModeData.btnMake | newPressedButtons; //0x00000C14
    newUnpressedButtons = ctrl.kernelModeData.btnBreak | newUnpressedButtons; //0x00000C18
    unpressedButtons = ctrl.kernelModeData.btnRelease | unpressedButtons; //0x00000C1C

    ctrl.kernelModeData.btnPress = pressedButtons; //0x00000C28
    ctrl.kernelModeData.btnRelease = unpressedButtons; //0x00000C2C
    ctrl.kernelModeData.btnMake = newPressedButtons; //0x00000C30
    ctrl.kernelModeData.btnBreak = newUnpressedButtons; //0x00000C34
    ctrl.kernelModeData.readLatchCount++; //0x00000C3C

    //HOLD mode active
    if (updatedButtons & PSP_CTRL_HOLD) { //0x00000C38
        updatedButtons = updatedButtons & (PSP_CTRL_HOLD | PSP_CTRL_HOME); //0x00000C44
    }
    uModeBtnEmulationAll = 0; //0x00000C50
    kModeBtnEmulationAll = 0; //0x00000C54
    for (i = 0; i < CTRL_DATA_EMULATION_SLOTS; i++) {
         uModeBtnEmulation = ctrl.emulatedData[i].uModeBtnEmulation; //0x00000C5C
         kModeBtnEmulation = ctrl.emulatedData[i].kModeBtnEmulation; //0x00000C60
         intCtrlBufUpdates = ctrl.emulatedData[i].intCtrlBufUpdates2; //0x00000C64

         uModeBtnEmulationAll |= uModeBtnEmulation; //0x00000C6C
         kModeBtnEmulationAll |= kModeBtnEmulation; //0x00000C78
         if (intCtrlBufUpdates > 0) { //0x00000C74
             if (--intCtrlBufUpdates == 0) { //0x00000C7C
                 ctrl.emulatedData[i].uModeBtnEmulation = 0; //0x00000C84
                 ctrl.emulatedData[i].kModeBtnEmulation = 0; //0x00000C88
             }
             ctrl.emulatedData[i].intCtrlBufUpdates2 = intCtrlBufUpdates; //0x00000C80
         }
    }
    updatedButtons |= uModeBtnEmulationAll; //0x00000C94
    gp = pspGetGp(); //0x00000C98

    curButtons = kModeBtnEmulationAll | pureButtons; //0x00000CA8
    prevButtons = ctrl.prevButtons; //0x00000CAC
    ctrl.prevButtons = curButtons; //0x00000CB0
    pressedButtons = curButtons ^ prevButtons; //0x00000CB8

    //0x00000CE0 & 0x00000CE4
    for (i = 0; i < CTRL_BUTTON_CALLBACK_SLOTS; i++) {
         btnMask = ctrl.buttonCallback[i].btnMask; //0x00000CC0
         btnMask &= pressedButtons; //0x00000CC4
         //If a "callback" button has been pressed
         if (btnMask != 0) { //0x00000CC8
             if (ctrl.buttonCallback[i].callbackFunc != NULL) { //0x00000CD4
                 //0x00000EC8: useless
                 pspSetGp(ctrl.buttonCallback[i].gp); //0x00000ECC
                 btnCbFunc = ctrl.buttonCallback[i].callbackFunc; //0x00000ED0
                 btnCbFunc(curButtons, prevButtons, ctrl.buttonCallback[i].arg); // 0x00000EDC
             }
         }
    }

    pspSetGp(gp); //0x00000CEC

    for (i = CTRL_BUTTONS_RAPID_FIRE_MAX_SLOT; i >= 0; i--) {
        if (ctrl.rapidFire[i].pressedButtonRange != 0) { //0x00000D0C
            pressedButtons = updatedButtons & ctrl.rapidFire[i].pressedButtonRange; //0x00000D18
            if (pressedButtons == ctrl.rapidFire[i].reqButtonsEventTrigger) { //0x00000D1C
                ctrlBufferUpdatesLeft = ctrl.rapidFire[i].eventData & 0x3F; //0x00000E64
                ctrlBufferUpdatesLeft--; //0x00000E68
                reqButtonEventMode = ctrl.rapidFire[i].eventData >> 6; //0x00000E6C
                ctrlBufferUpdatesLeft &= 0xFF; //0x00000E74
                if (reqButtonEventMode != 0) { //0x00000E70
                    if (reqButtonEventMode == 2) { //0x00000E90
                        updatedButtons = updatedButtons | ctrl.rapidFire[i].reqButtons; //0x00000EC4
                    }
                    else {
                        updatedButtons = updatedButtons & ~ctrl.rapidFire[i].reqButtons; //0x00000E9C & 0x00000EA0
                    }
                    if (ctrlBufferUpdatesLeft == 0) { //0x00000EA4
                        //Switch event mode. If current event mode = 2 -> 3 (ignore reqButton(s)), otherwise 3 -> 2 (set reqButton(s)).
                        reqButtonEventMode = reqButtonEventMode ^ 0x1; //0x00000EAC
                        ctrlBufferUpdatesLeft = *(u8 *)(&ctrl.rapidFire[i] + (reqButtonEventMode & 0x1) + 13); //0x00000EB0 & 0x00000EB4 & 0x00000EBC
                    }
                }
                else {
                    ctrlBufferUpdatesLeft = ctrl.rapidFire[i].reqButtonsEventOnTime; //0x00000E78
                    reqButtonEventMode = 2; //0x00000E7C
                }

                reqButtonEventMode = reqButtonEventMode << 6; //0x00000E80
                rapidFireEventData = reqButtonEventMode | ctrlBufferUpdatesLeft; //0x00000E84
                ctrl.rapidFire[i].eventData = rapidFireEventData; //0x00000E8C
            }
            else {
                 ctrl.rapidFire[i].eventData = 0; //0x00000D24
            }
        }
    }
    //0x00000D34
    updatedButtons &= ctrl.maskSupportButtons; //0x00000D48
    updatedButtons |= ctrl.maskSetButtons; //0x00000D4C
    //HOLD mode active?
    if (updatedButtons & PSP_CTRL_HOLD) { //0x00000D50 & 0x00000D54
        if (check == 0) { //0x00000D60
            if (uModeBtnEmulationAll != 0) { //0x00000E44
                updatedButtons2 = updatedButtons & 0xFFFEFFFF; //0x00000E54
                pressedUserButtons = updatedButtons & ctrl.userModeButtons; //0x00000E58
                updatedButtons = (pressedUserButtons != 0) ? updatedButtons2 : updatedButtons; //0x00000E60
            }
        }
        else {
            updatedButtons &= 0xFFFEFFFF; //0x00000D68
        }
    }
    mergedButtons = updatedButtons ^ ctrl.userButtons; //0x00000D78
    if (updatedButtons & PSP_CTRL_HOLD) { //0x00000D74
        updatedButtons = updatedButtons & (PSP_CTRL_HOLD | PSP_CTRL_HOME); //0x00000D80
        mergedButtons = updatedButtons ^ ctrl.userButtons; //0x00000D84
    }
    holdButton = mergedButtons & PSP_CTRL_HOLD; //0x00000D90
    ctrlUserBuf->buttons = updatedButtons; //0x00000D94
    ctrl.userButtons = updatedButtons; //0x00000D9C
    if (holdButton) { //0x00000D98
        ctrl.unk_Byte_9 = ctrl.unk_Byte_3; //0x00000DA4
    }
    if (ctrl.unk_Byte_9 != 0) { //0x00000DAC
        updatedButtons &= PSP_CTRL_HOME; //0x00000DBC
        ctrl.unk_Byte_9--; //0x00000DC0
        mergedButtons = updatedButtons ^ ctrl.userButtons; //0x00000DC4
    }
    unpressedButtons = ~updatedButtons; //0x00000DD4
    newUnpressedButtons = unpressedButtons & mergedButtons; //0x00000DDC
    updatedButtons &= mergedButtons; //0x00000DE4

    pressedButtons = ctrl.userModeData.btnPress | updatedButtons; //0x00000DC8 & 0x00000DEC

    ctrl.userModeData.btnRelease = ctrl.userModeData.btnRelease | unpressedButtons; //0x00000DD4 & 0x00000DF0 & 0x00000DF4
    ctrl.userModeData.btnBreak = ctrl.userModeData.btnBreak | newUnpressedButtons; //0x00000DE8 & 0x00000E00
    ctrl.userModeData.btnPress = ctrl.userModeData.btnPress | pressedButtons; //0x00000E34
    ctrl.userModeData.btnMake = ctrl.userModeData.btnMake | mergedButtons; //0x00000E38
    ctrl.userModeData.readLatchCount++; //0x00000DD8 & 0x00000DF8 & 0x00000E2C

    return SCE_ERROR_OK; //0x00000E30
}

/* Subroutine sub_00001E70 - Address 0x00001E70 */
static int _sceCtrlReadBuf(SceCtrlDataExt *pad, u8 reqBufReads, int arg3, u8 mode) {
    SceCtrlInternalData *intDataPtr;
    SceCtrlDataExt *ctrlBuf;
    int oldK1;
    int i; /* Used as a counter variable in for-loops for storing data into a SceCtrlData member. */
    u32 buttons;
    char bufIndex;
    /* Number of read internal controller buffers. */
    int readIntBufs;
    char startBufIndex;
    int res;
    int suspendFlag;

    if (reqBufReads >= CTRL_INTERNAL_CONTROLLER_BUFFERS) { //0x00001E74 & 0x00001E84 & 0x00001EB4
        return SCE_ERROR_INVALID_SIZE;
    }
    if (arg3 > 2) { //0x00001EC0 & 0x00001EC4
        return SCE_ERROR_INVALID_VALUE;
    }
    if ((arg3 != 0) && (mode & 2)) { //0x00001ECC & 0x00001ED8
        return SCE_ERROR_NOT_SUPPORTED;
    }
    oldK1 = pspShiftK1();

    if (!pspK1PtrOk(pad)) { //0x00001EF0 -- protect kernel address from user mode
        pspSetK1(oldK1);
        return SCE_ERROR_PRIV_REQUIRED;
    }
    if (pspK1IsUserMode()) { //0x00001EFC -- user mode
        intDataPtr = &ctrl.userModeData; //0x00001F00
    }
    else { //kernel mode
        intDataPtr = &ctrl.kernelModeData; //0x00001F08
    }
    if (arg3 != 0) { //0x00001F0C
        if (intDataPtr->sceCtrlBuf[arg3] == NULL) { //0x00001F18 & 0x00001F1C & 0x00001F20
            return SCE_ERROR_NOT_SUPPORTED;
        }
    }
    //waiting for the VSYNC callback or your custom timer when using "read"
    if (mode & 2) { //0x00001F10 && 0x00001F28
        res = sceKernelWaitEventFlag(ctrl.timerID, 1, PSP_EVENT_WAITOR, NULL, NULL); //0x00001F44
        if (res < 0) { //0x00001F4C
            pspSetK1(oldK1); //0x00001F50
            return res;
        }
        suspendFlag = sceKernelCpuSuspendIntr(); //0x00001F54
        sceKernelClearEventFlag(ctrl.timerID, 0xFFFFFFFE); //0x00001F64

        startBufIndex = intDataPtr->ctrlBufStartIndex; //0x00001F70
        readIntBufs = intDataPtr->ctrlBufIndex - startBufIndex; //0x00001F74
        if (readIntBufs < 0) { //0x00001F7C
            readIntBufs += 64; //0x00001F78 & 0x00001F80
        }
        intDataPtr->ctrlBufStartIndex = intDataPtr->ctrlBufIndex; //0x00001F8C
        if (reqBufReads < readIntBufs) { //0x00001F88
            startBufIndex = intDataPtr->ctrlBufIndex - reqBufReads; //0x00001F90
            startBufIndex = (startBufIndex < 0) ? startBufIndex + CTRL_INTERNAL_CONTROLLER_BUFFERS : startBufIndex; //0x00001F98 & 0x00001F9C
            readIntBufs = reqBufReads;
        }
    }
    else {
        suspendFlag = sceKernelCpuSuspendIntr(); //0x0000216C
        bufIndex = intDataPtr->ctrlBufIndex; //0x00002174

        startBufIndex = bufIndex; //0x00002184
        if (reqBufReads < 64) { //0x00002178
            startBufIndex = bufIndex - reqBufReads; //0x0000218C
            startBufIndex = (startBufIndex < 0) ? startBufIndex + CTRL_INTERNAL_CONTROLLER_BUFFERS : startBufIndex; //0x00001F98 & 0x00001F9C
            readIntBufs = reqBufReads; //0x00001FA0
        }
    }
    if (arg3 != 0) { //0x00001FA4
        ctrlBuf = (SceCtrlDataExt *)intDataPtr->sceCtrlBuf[arg3] + startBufIndex; //0x00002160
    }
    else {
         ctrlBuf = (SceCtrlDataExt *)((SceCtrlData *)intDataPtr->sceCtrlBuf[0] + startBufIndex); //0x00001FB4
    }
    if (readIntBufs < 0) { //0x00001FB8
        sceKernelCpuResumeIntr(suspendFlag); //0x000020A8
        pspSetK1(oldK1); //0x000020B0
        return readIntBufs;
    }
    /* read internal data buffers. */
    while (reqBufReads-- > 0) { //0x0000209C & 0x000020A0
           pad->activeTime = ctrlBuf->activeTime; //0x00001FE4 & 0x00001FF0

           buttons = ctrlBuf->buttons; //0x00001FE8
           if (pspK1IsUserMode()) { //0x00001FEC
               buttons &= ctrl.userModeButtons; //0x00001FF8
           }
           /* The current button value(s) will be turned negative, if this function is called through read/peekBufferNegative. */
           pad->buttons = (mode & 1) ? ~buttons : buttons; //0x00002004 & 0x00002004 & 0x0000200C

           pad->aX = ctrlBuf->aX; //0x00002010
           pad->aY = ctrlBuf->aY; //0x00002018

           if (mode < 4) { //0x00001FC8 & 0x00002014
               for (i = 0; i < (int)sizeof pad->rsrv; i++) { //0x00002130 - 0x00002138
                    pad->rsrv[i] = 0;
               }
               pad = (SceCtrlDataExt *)((SceCtrlData *)pad + 1); //0x00002140
           }
           /* read data into the extended parts of the original SceCtrlData structure. */
           if (mode >= 4) {
               pad->rsrv[2] = 0; //0x0000201C
               pad->rsrv[3] = 0; //0x00002024
               if (arg3 == 0) { //0x00002020
                   pad->rsrv[0] = -128; //0x00002100
                   pad->rsrv[1] = -128; //0x00002104
                   pad->rsrv[4] = 0; //0x00002108
                   pad->rsrv[5] = 0; //0x00002108
                   pad->unk1 = 0; //0x0000210C
                   pad->unk2 = 0; //0x00002110
                   pad->unk3 = 0; //0x00002114
                   pad->unk4 = 0; //0x00002118
                   pad->unk5 = 0; //0x0000211C
                   pad->unk6 = 0; //0x00002120
                   pad->unk7 = 0; //0x00002124
                   pad->unk8 = 0; //0x0000212C
               }
               else {
                   pad->rsrv[0] = ctrlBuf->rsrv[0]; //0x00002034
                   pad->rsrv[1] = ctrlBuf->rsrv[1]; //0x00002038
                   pad->rsrv[4] = ctrlBuf->rsrv[4]; //0x0000203C
                   pad->rsrv[5] = ctrlBuf->rsrv[5]; //""
                   pad->unk1 = ctrlBuf->unk1; //0x00002044
                   pad->unk2 = ctrlBuf->unk2; //...
                   pad->unk3 = ctrlBuf->unk3;
                   pad->unk4 = ctrlBuf->unk4;
                   pad->unk5 = ctrlBuf->unk5;
                   pad->unk6 = ctrlBuf->unk6;
                   pad->unk7 = ctrlBuf->unk7; //...
                   pad->unk8 = ctrlBuf->unk8; //0x0000207C
               }
               pad = pad + 1; //0x00002080
           }
           startBufIndex++; //0x0000208C
           if (arg3 == 0) { //0x00002098
               ctrlBuf = (SceCtrlDataExt *)((SceCtrlData *)ctrlBuf + startBufIndex); //0x00002084 & 0x00002090
           }
           else {
               ctrlBuf = (SceCtrlDataExt *)ctrlBuf + startBufIndex; //0x00002088 & 0x00002098
           }
           if (startBufIndex == CTRL_INTERNAL_CONTROLLER_BUFFERS) { //0x00002094
               startBufIndex = 0; //0x000020EC

               if (arg3 != 0) { //0x000020E8
                   ctrlBuf = (SceCtrlDataExt *)intDataPtr->sceCtrlBuf[0]; //0x000020F4
               }
               else {
                   ctrlBuf = (SceCtrlDataExt *)intDataPtr->sceCtrlBuf[arg3]; //0x00001FC4 & 0x000020FC
               }
           }
    }

    sceKernelCpuResumeIntr(suspendFlag); //0x000020A8
    pspSetK1(oldK1); //0x000020B0
    return readIntBufs;
}

/*
 * Subroutine module_start - Address 0x00001A10
 * Exported in syslib
 */
int CtrlInit(void) {
    sceCtrlInit();
    return SCE_ERROR_OK;
}

/*
 * Subroutine module_reboot_before - Address 0x00001A30
 * Exported in syslib
 */
int module_reboot_before(void) {
    sceCtrlEnd();
    return SCE_ERROR_OK;
}
