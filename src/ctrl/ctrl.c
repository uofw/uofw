/* Copyright (C) 2011, 2012 The uOFW team
   See the file COPYING for copying permission.
*/

/**
 * ctrl.c
 *    Reverse engineered controller libraries of the SCE PSP system.
 * Author: _Felix_
 * Version: 6.39
 * 
 */

#include "../ctrl/ctrl.h"

#define PSP_BUTTON_AMOUNT                       16  
#define PSP_INIT_KEYCONFIG_UPDATER              0x110 //might be incorrect

#define USER_MODE                               0
#define KERNEL_MODE                             1

#define FALSE                                   0
#define TRUE                                    1

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

#define CTRL_SAMPLING_MODE_MODES                2
#define CTRL_SAMPLING_MODE_MAX_MODE             CTRL_SAMPLING_MODE_MODES -1

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


PSP_MODULE_INFO("sceController_Service", 0x1007, 1, 1);

typedef struct _SceCtrl {
    SceSysTimerId timerID; //0
    int eventFlag; //4
    u32 btnCycle; //8
    pspCtrlPadInputMode samplingMode[CTRL_SAMPLING_MODE_MODES]; //12 -- samplingMode[0] for User mode, samplingMode[1] for Kernel mode
    u8 unk_Byte_0; //14
    u8 unk_Byte_1; //15
    u8 unk_Byte_8; //16
    char unk_Byte_7; //17
    u8 unk_Byte_2; //20
    pspCtrlPadPollMode pollMode; //21
    u16 suspendSamples; //22
    int unk_1; //24
    SceSysconPacket sysPacket[2]; //28 -- size of one array member is 96.
    SceCtrlInternalData userModeData; //220
    SceCtrlInternalData kernelModeData; //260
    SceCtrlRapidFire rapidFire[CTRL_BUTTONS_RAPID_FIRE_SLOTS]; //300 - 555
    /** Currently pressed buttons passed to _SceCtrlUpdateButtons(). They are "pure", 
     *  as custom settings are applied on them in _SceCtrlUpdateButtons(). */
    int pureButtons; //556
    int unk_array_3[2]; //560 -- previous pressed button?
    int prevButtons; //568 -- previously pressed buttons
    /** Currently pressed User buttons. */
    int userButtons; //572
    /* Records the possibly user-modified, pressed buttons of the past VBlank interrupt before the current one. */
    int prevModifiedButtons; //576
    char analogX; //580;
    char analogY; //581
    char unk_Byte_9; //582
    char unk_Byte_3; //583
    SceCtrlEmulatedData emulatedData[CTRL_DATA_EMULATION_SLOTS]; //584
    u32 userModeButtons; //664
    /** Button bit mask defining buttons going to be supported (recognized if being pressed or not). 
     *  Can be used to ignore buttons (buttons constantly turned off). */
    u32 maskSupportButtons; //668
    /** Button bit mask defining buttons going to be simualated as being pressed. */
    u32 maskSetButtons; //672
    int unk_4; //676
    int unk_5; //680
    int unk_6; //684
    int unk_7; //688
    int unk_8; //692
    int idleReset; //696
    int idleBack; //700
    SceCtrlButtonCallback buttonCallback[CTRL_BUTTON_CALLBACK_SLOTS]; //704  
    int unk_array2[2]; //768 -- array of functions?
    int unk_array3[3]; //776
} SceCtrl; //sizeof SceCtrl: 788 (0x314)

typedef struct _SceCtrlRapidFire {
    /** The pressed-button-range to check for. */
    u32 pressedButtonRange; //0
    /** The button(s) which will fire the pressed/unpressed period for a button/buttons when being pressed. 
     *  The button(s) has/have to be included in the pressed-button-range. *
     */
    u32 reqButtonsEventTrigger; //4
    /** The requested button(s) on which the pressed/unpressed period (the rapid fire event) will be applied to. 
     *  User mode buttons ONLY. 
     */
    u32 reqButtons; //8
    /** Apply data for a rapidFire period. Keeps track of the apply mode of the requested button(s) (pressed/unpressed) 
     *  + the amount of left internal controller buffer updates for the current apply mode of the requested button(s). 
     */
    u8 eventData; //12
    /** For how many (consecutive) internal controller buffer updates the requested button(s) will be set to ON (pressed). */
    u8 reqButtonsOnTime; //13
    /** For how many (consecutive) internal controller buffer updates the requested button(s) will be set to OFF (unpressed). */
    u8 reqButtonsOffTime; //14;
    /** For how many (consecutive) internal controller buffer updates the requested button(s) will be set to ON. 
     *  It will only be applied for the first ON period of a (not canceled) rapid fire event. */
    u8 reqButtonsEventOnTime; //15;
} SceCtrlRapidFire; //sizeof SceCtrlRapidFire: 16

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
} SceCtrlEmulatedData; //sizeof SceCtrlEmulatedData: 20

typedef struct _SceCtrlButtonCallback {
    /** Bitwise OR'ed button values (of ::PspCtrlButtons) which will be checked for being pressed. */
    u32 btnMask; //0
    /** Pointer to a callback function handling the button input effects. */
    SceCtrlCb callbackFunc; //4
    /** The global pointer ($gp) value of the controller module. */
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
    /** sceCtrlBuf[0] points to 64 internal ctrl buffers of type sceCtrlData. */
    void *sceCtrlBuf[3]; //28
} SceCtrlInternalData; //sizeof SceCtrlLatchInt: 40

//copied from http://holdpsp.googlecode.com/svn/trunk/sysconhk.h
typedef struct _SceSysconPacket {
    u8 unk00[4]; //0 -- (0x00,0x00,0x00,0x00)
    u8 unk04[2]; //4 -- (arg2)
    u8 status; //6
    u8 unk07; //7 -- (0x00)
    u8 unk08[4]; //8 -- (0xff,0xff,0xff,0xff)
    /** transmit data. */
    u8 tx_cmd; //12 -- command code
    u8 tx_len; //13 -- number of transmit bytes
    u8 tx_data[14]; //14 -- transmit parameters
    /** receive data. */
    u8 rx_sts; //28 --  generic status
    u8 rx_len; //29 --  receive length
    u8 rx_response; //30 --  response code(tx_cmd or status code)
    u8 rx_data[9]; //31 --  receive parameters
    u32 unk28; //40
    /** user callback (when finish an access?) */
    void (*callback)(SceSysconPacket *, u32); //44
    u32	callback_r28; //48
    u32	callback_arg2; //52 -- arg2 of callback (arg4 of sceSycconCmdExec)
    u8 unk38[13]; //56
    u8 old_sts;	//69 -- old rx_sts
    u8 cur_sts;	//70 --  current rx_sts
    u8 unk47[33]; //71
} SceSysconPacket; //sizeof SceSysconPacket: 96

typedef UnknownType;

SceCtrl ctrl; //0x2890
SceKernelDeci2Ops ctrlDeci2Ops; //0x000027F8
_PspSysEventHandler ctrlSysEvent; //0x00002850

static SceUInt _sceCtrlDummyAlarm(void *common); //0x00001DD8
static int _sceCtrlVblankIntr(int subIntNm, void *arg); //0x00000440
static int _sceCtrlTimerIntr(); //sub_00000528
static int _sceCtrlSysconCmdIntr1(); //0x00000610;
static int _sceCtrlSysconCmdIntr2(); //sub_00001E4C
static int _sceCtrlUpdateButtons(u32 pureButtons, u8 aX, u8 aY); //sub_00000968
static int _sceCtrlReadBuf(void *pad, u8 reqBufReads, int arg3, u8 mode); //sub_00001E70


/* sub_00001DD8 */
static SceUInt _sceCtrlDummyAlarm(void *common) {
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
    
    return 0;
}

/* sub_00000440 */
static int _sceCtrlVblankIntr(int subIntNm, void *arg) {
    int suspendFlag;
    int retVal;
    
    suspendFlag = sceKernelCpuSuspendIntr(); //0x00000454
    if (ctrl.btnCycle == 0) { //0x00000464
        if (ctrl.unk_Byte_8 == 0) { //0x00000470
            if (ctrl.pollMode != PSP_CTRL_POLL_NO_POLLING) { //0x0000047C
                ctrl.unk_Byte_8 = 1; //0x000004E0
                if ((ctrl.samplingMode[USER_MODE] | ctrl.samplingMode[KERNEL_MODE]) == PSP_CTRL_INPUT_DIGITAL_ONLY) { //0x000004E4
                    ctrl.sysPacket[0].tx_cmd = 7; //0x000004E8
                }
                else {
                    ctrl.sysPacket[0].tx_cmd = 8; //0x000004EC
                }
                ctrl.sysPacket[0].tx_len = 2; //0x00000514
                retVal = sceSysconCmdExecAsync(&ctrl.sysPacket[0], 1, _sceCtrlSysconCmdIntr1, 0); //0x00000510
                if (retVal < 0) { //0x00000518
                    ctrl.unk_Byte_8 = 0; //0x0000051C
                }
            }
            else {
                sceKernelSetAlarm(700, _sceCtrlDummyAlarm, NULL); //0x00000490
            }
        }
        else {
            sceKernelSetAlarm(700, _sceCtrlDummyAlarm, NULL); //0x00000490
        }
    }
    if (ctrl.unk_Byte_2 != 0) { //0x000004A0
        ctrl.unk_Byte_2 = 0; //0x000004A4
        sceKernelPowerTick(PSP_POWER_TICK_ALL); //0x000004CC             
    }
    
    sceKernelCpuResumeIntr(suspendFlag); //0x000004A8
    return -1;
}

/* sub_00000528 */
static int _sceCtrlTimerIntr() {
    char unk_1 = 7;
    int suspendFlag;
    int retVal;
    
    suspendFlag = sceKernelCpuSuspendIntr(); //0x0000053C
    if (ctrl.btnCycle != 0) { //0x00000548 & 0x0000054C
        if ((ctrl.suspendSamples == 0) && (ctrl.pollMode != CTRL_POLL_MODE_OFF)) { //0x00000558 & 0x00000564            
            ctrl.suspendSamples = 1; //0x000005C8
            if (ctrl.samplingMode[USER_MODE] != PSP_CTRL_INPUT_DIGITAL_ONLY) { //0x000005CC
                unk_1 = 8; //0x000005D4
            }
            ctrl.sysPacket[0].tx_cmd = unk_1; //0x000005D8
            retVal = sceSysconCmdExecAsync(&ctrl.sysPacket[0], 1, _sceCtrlSysconCmdIntr1, 0); //0x000005F8
            if (retVal < 0) { //0x00000600
                ctrl.suspendSamples = 0; //0x00000604
            }
        }
        else {
             sceKernelSetAlarm(700, _sceCtrlDummyAlarm, NULL); //0x00000578
        }
    }
    if (ctrl.unk_Byte_2 != 0) { //0x00000588
        ctrl.unk_Byte_2 = 0; //0x0000058C
        sceKernelPowerTick(PSP_POWER_TICK_ALL); //0x000005B4       
    }
    
    sceKernelCpuResumeIntr(suspendFlag); //0x00000590
    return -1; //0x000005A8
}

/* sub_00000610 */
static int _sceCtrlSysconCmdIntr1() {
    u32 pureButtons;
    int suspendFlag;
    
    ctrl.unk_Byte_8 = 0; //0x00000644
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
        
        return 0; //0x00000820
    }
    //TODO: Reversing of sceSysconCmdExecAsync to be able to continue here!
    else {
        return 0;
    } 
}

/* sub_00001E4C */
static int _sceCtrlSysconCmdIntr2() {
    ctrl.unk_Byte_0 = 0; //0x00001E5C   
    ctrl.unk_Byte_1 = ctrl.sysPacket[1].tx_data[0] & 0x1; //0x00001E64
    ctrl.unk_Byte_7 = 0; //0x00001E6C
    
    return 0;
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
    void (*func)(void *);
    int ret;
    int res;
    int res2;
    int res3;
    int storeData;
    int temp1;
    int temp2;
    int temp3;
    int unk1;
    int unk2;
    int unk3;
    u32 btnMask;
    /** User mode buttons being pressed. */
    u32 pressedUserButtons; 
    /** Newly pressed buttons, were not pressed immediatley before. */
    u32 newPressedButtons; 
    /** Newly unpressed buttons, were pressed immediatley before. */
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
    u32 gp_Val;
    
    _Bool check = 0;
    
    analogX = aX & 0xFF; //0x00000974
    analogY = aY & 0xFF; //0x0000097C
    
    sysTimeLow = sceKernelGetSystemTimeLow(); //0x000009A0
    
    //User buffer
    index = ctrl.userModeData.ctrlBufIndex; //0x000009B4 && 0x000009C4 
    ctrlUserBuf = (SceCtrlData *)(ctrl.userModeData.sceCtrlBuf[0] + index * sizeof(SceCtrlData)); //0x000009CC
    ctrlUserBuf->activeTime = sysTimeLow; //0x000009D0
    
    //Kernel buffer
    index = ctrl.kernelModeData.ctrlBufIndex; //0x000009C0 && 0x000009D4
    ctrlKernelBuf = (SceCtrlData *)(ctrl.kernelModeData.sceCtrlBuf[0] + index * sizeof(SceCtrlData)); //0x000009D8
    ctrlUserBuf->activeTime = sysTimeLow; //x000009E8
    
    int i;
    for (i = 0; 0 < 4; i++) { //0x00000A08 && 0x000009F0
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
    if (ctrl.samplingMode[USER_MODE] == PSP_CTRL_INPUT_DIGITAL_ONLY) { //0x00000A2C -- if only the Digital input is to be recongnized
        ctrlUserBuf->aY = CTRL_ANALOG_PAD_DEFAULT_VALUE; //0x00000A38
        ctrlUserBuf->aX = CTRL_ANALOG_PAD_DEFAULT_VALUE; //0x00000A3C       
    }
    //Kernel Mode
    if (ctrl.samplingMode[KERNEL_MODE] == PSP_CTRL_INPUT_DIGITAL_ONLY) { //0x00000A44 -- if only the Digital input is to be recongnized
             ctrlKernelBuf->aY = CTRL_ANALOG_PAD_DEFAULT_VALUE; //0x00000A50
             ctrlKernelBuf->aX = CTRL_ANALOG_PAD_DEFAULT_VALUE; //0x00000A54
    }
    //0x00000A6C && 0x00000A7C && 0x00000A88
    for (i = 0; i < 6; i++) {
         ctrlUserBuf->rsrv[i] = 0;
    }
    //0x00000A8C && 0x00000AA0 && 0x00000AA4
    for (i = 0; i < 6; i++) {
         ctrlKernelBuf->rsrv[i] = 0;
    }
    //0x00000AB8 && 0x00000AC0
    for (i = 0; i < 2; i++) {
         if (ctrl.unk_array2[i+1] != 0) { //0x00000AAC 
             ctrlKernelBufExt = (SceCtrlDataExt *)ctrl.kernelModeData.sceCtrlBuf[i+1]; //0x00000AB0 && 0x00000ABC
             ctrlKernelBufExt = ctrlKernelBufExt + sizeof(SceCtrlDataExt); //0x00000F48
             
             func = *((ctrl.unk_array2[i+1]) +0x4); //0x00000F5C
             ret = func();
             ctrlKernelBufExt->activeTime = sysTimeLow; //0x00000F68
             func(ctrl.unk_array3[i+1]); //0x00000F64
             if (ret < 0) { //0x00000F78
                 //0x000011AC && 0x000011B8
                 int j;
                 for (j = 0; j < 6; j++) {
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
                     res &=0x1FFFF;//0x00000FA4 -- User mode buttons?
                     ctrl.unk_array_3[i] = ctrlKernelBufExt->buttons; //0x00000FAC
                     if (res != 0) { //0x00000FA8
                         ctrl.unk_Byte_2 = 1; //0x00000FBC
                     }
                 }
                 ctrlUserBufExt = (SceCtrlDataExt *)ctrl.userModeData.sceCtrlBuf[i+1]; //0x00000FD0
                 if ((ctrl.unk_Byte_2 == 0) && (ctrl.samplingMode[USER_MODE] == PSP_CTRL_INPUT_DIGITAL_ANALOG)) { //0x00000FCC &&  0x00000FD4
                     analogX = ctrlKernelBufExt->aX; //0x00000FE0
                     analogY = ctrlKernelBufExt->aY; //0x00000FE4
                     analogX -= 128; //0x00000FEC
                     analogY -= 128; //0x00000FF0
                     tempAnalogX = -analogX; //0x00000FF4
                     tempAnalogY = -analogY; //0x00000FF8
                     temp2 = MAX(analogX, tempAnalogX); //0x00001004
                     tempAnalogX = temp2;
                     temp3 = MAX(analogY, tempAnalogY); //0x00001008
                     tempAnalogY = temp3;
                     
                     res = (ctrl.idleReset < 37); //0x0000100C
                     tempAnalogX ^= 0x7F; //0x00001014
                     tempAnalogY ^= 0x7F; //0x00001018
                     temp1 = (res != 0) ? 37 : ctrl.idleReset; //0x00001020
                     temp2 = (tempAnalogX == 0) ? 128 : temp2; //0x00001024
                     unk1 = ctrlKernelBufExt->rsrv[0]; //0x00000FEC
                     unk2 = ctrlKernelBufExt->rsrv[1]; //0x00000FF0
                     unk1 -= 128; //0x00001028
                     unk2 -= 128; //0x0000102C
                     temp3 = (tempAnalogY == 0) ? 128 : temp3; //0x00001030
                     res = (temp3 < temp1); //0x00001034
                     res2 = (temp2 < temp1); //0x00001038
                     unk1 = MAX(unk1, (-unk1)); //0x00001044
                     unk2 = MAX(unk2, (-unk2)); //0x00001048
                     res2 ^= 0x1; //0x0000104C
                     res ^= 0x1; //0x00001050
                     res3 = res2 | res; //0x0000105C
                     unk2 = ((unk2 ^ 0x7F) == 0) ? 128 : unk2; //0x00001058 && 0x00001060
                     unk1 = ((unk1 ^ 0x7F) == 0) ? 128 : unk1; //0x00001054 && 0x00001068
                     
                     storeData = 1;
                     if (res3 == 0) { //0x00001064
                         res2 = (unk2 < temp1); //0x0000106C
                         unk2 = (unk1 < temp1); //0x00001070
                         unk2 ^= 0x1; //0x00001074
                         res2 ^= 0x1; //0x00001078
                         if ((res2 | unk2) == 0) { //0x00001080
                             storeData = 0;
                         } 
                     }
                     //0x00001088
                     if (storeData) {
                         ctrl.unk_Byte_2 = 1; //0x0000108C
                     }   
                 }
             }
             //*** start of code: 0x00001094 -- 0x000010D0 ***
             ctrlUserBufExt = (SceCtrlDataExt *)ctrlUserBufExt + sizeof(SceCtrlDataExt); //0x000010A0
             ctrlUserBufExt->activeTime = ctrlKernelBufExt->activeTime; //0x000010A8 && 0x000010B8
             ctrlUserBufExt->buttons = ctrlKernelBufExt->buttons; //0x000010AC && 0x000010C4
             ctrlUserBufExt->aX = ctrlKernelBufExt->aX; //0x000010B0 && 0x000010C8
             ctrlUserBufExt->aY = ctrlKernelBufExt->aY; //0x000010B0 && 0x000010C8
             //0x000010B0 && 0x000010C8 && 0x000010B4 && 0x000010D0
             int j;
             for (j = 0; j < 6; j++) {
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
             
             unk1 = ctrl.unk_array2[0]; //0x00001100
             unk1 = unk1 >> i; //0x00001104
             if ((unk1 & 0x1) != 0) { //0x00001108 && 0x0000110C
                 buttons = ctrlKernelBufExt->buttons; //0x00001114
                 ctrl.emulatedData[i+1].intCtrlBufUpdates2 = 1; //0x0000111C
                 //What is that!? Makes no sence
                 unk1 = buttons & 0x500; //0x00001120
                 unk2 = buttons & 0xA00; //0x00001124
                 unk1 = (0 < unk1); //0x00001128
                 unk2 = (0 < unk2); //0x00001130
                 buttons = buttons & 0xFFFFF0FF; //0x00001134
                 unk1 = unk1 << 8; //0x00001138
                 unk3 = buttons | unk1; //0x0000113C
                 unk2 = unk2 << 9; //0x00001140
                 kModeBtnEmulation = unk3 | unk2; //0x00001144
                 uModeBtnEmulation = kModeBtnEmulation & ctrl.userModeButtons; //0x00001148
                 ctrl.emulatedData[i+1].uModeBtnEmulation = uModeBtnEmulation; //0x0000114C
                 ctrl.emulatedData[i+1].kModeBtnEmulation = kModeBtnEmulation; //0x00001150
                 
                 analogX = ctrlKernelBufExt->aX; //0x00001154
                 analogX -= 128; //0x00001158
                 tempAnalogX = -analogX; //0x0000115C
                 analogX = MAX(analogX, tempAnalogX); //0x00001160
                 res = (analogX < 38); //0x00001164
                 analogY = ctrlKernelBufExt->aY; //0x0000116C
                 if (res != 0) { //0x00001168
                     res = res & 0xFF; //0x00001170
                     res -= 128; //0x00001174
                     res2 = -res; //0x00001178
                     res2 = MAX(res, res2); //0x00001178
                     res = (res2 < 38); //0x00001180
                     if (res == 0) { //0x00001184
                         check = 1; //0x00001194
                         tempAnalogY = analogY & 0xFF; //0x00001190
                         tempAnalogY -= 128; //0x000011A0
                         tempAnalogX -= 128; //0x000011A8
                         
                     }
                 }
             }
         }
    }
    //0x00000AC8
    if ((tempAnalogX | tempAnalogY) != 0) { //0x00000ACC
        tempAnalogX2 = analogX - 65; //0x00000AD4
        res = (tempAnalogX2 < 127); //0x00000AD8
        tempAnalogX2 = tempAnalogX + analogX; //0x00000AE0
        storeData = 1; 
        if (res != 0) { //0x00000ADC
            tempAnalogY2 = analogY - 65; //0x00000AE4
            res = (tempAnalogY2 < 127); //0x00000AE8
            analogX = 255; //0x00000AF0
            if (res != 0) { //0x00000AEC
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
        tempAnalogX = MIN(tempAnalogX2, analogX); //0x00000B0C
        if (tempAnalogX2 < CTRL_ANALOG_PAD_MIN_VALUE) { //0x00000B08
            tempAnalogX = CTRL_ANALOG_PAD_MIN_VALUE; //0x00000F38 
        }
        analogY = CTRL_ANALOG_PAD_MAX_VALUE; //0x00000B10
        tempAnalogY2 = MIN(tempAnalogY, analogY); //0x00000B18
        if (tempAnalogY < CTRL_ANALOG_PAD_MIN_VALUE) { //0x00000B14
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
    tempIndex = index >> 31; //0x00000B4C
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
    tempIndex = index >> 31; //0x00000B78
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
    if (pureButtons & PSP_CTRL_HOLD) { //0x00000BA4
        updatedButtons = pureButtons & CTRL_PSP_HARDWARE_IO_BUTTONS; //0x00000BB0 -- only I/O Buttons permitted?
    }
    //0x00000BB8 & 0x00000BC0 & 0x00000BC8
    for (i = 0; i < 3; i++) {
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
                gp_Val = GP_BACKUP(); //0x00000EC8
                GP_SET(ctrl.buttonCallback[i].gp); //0x00000ECC
                btnCbFunc = ctrl.buttonCallback[i].callbackFunc; //0x00000ED0
                btnCbFunc(curButtons, prevButtons, ctrl.buttonCallback[i].arg); // 0x00000EDC
            }
        }
    }
    
    GP_SET(gp_Val); //0x00000CEC
    
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
    updatedButtons = updatedButtons & ctrl.maskSupportButtons; //0x00000D48
    updatedButtons = updatedButtons | ctrl.maskSetButtons; //0x00000D4C
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
        ctrl.unk_Byte_9 = ctrl.unk_Byte_3; //0x00000DA4 -- set ctrl.unk_Byte_9 to 0?
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
    
    return 0; //0x00000E30
}

/* Subroutine sub_00001E70 - Address 0x00001E70 */
static int _sceCtrlReadBuf(void *pad, u8 reqBufReads, int arg3, u8 mode) {
    SceCtrlInternalData *intDataPtr;
    void *ctrlBuf;
    int k1;
    int privMode;
    int i; /* Used as a counter variable in for-loops for storing data into a SceCtrlData member. */
    u32 buttons;
    char bufIndex;
    /* Number of internal ctrl buffers read. */
    int readIntBufs; 
    char startBufIndex;
    int res;  
    int suspendFlag;
         
    if (reqBufReads > CTRL_INTERNAL_CONTROLLER_BUFFERS) { //0x00001E74 & 0x00001E84 & 0x00001EB4
        return SCE_ERROR_INVALID_SIZE;
    }
    if (arg3 >= 3) { //0x00001EC0 & 0x00001EC4
        return SCE_ERROR_INVALID_VALUE;
    }
    if ((arg3 != 0) && (mode & 2)) { //0x00001ECC & 0x00001ED8
        return SCE_ERROR_NOT_SUPPORTED; 
    }
    k1 = pspSdkGetK1();
    privMode = k1 << 11;
    pspSdkSetK1(privMode); //0x00001EF4
    
    if ((privMode & pad) < 0) { //0x00001EF0 -- protect kernel address from user mode
        pspSdkSetK1(k1);
        return SCE_ERROR_PRIV_REQUIRED;
    }
    if (privMode < 0) { //0x00001EFC -- user mode
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
    //waiting for the VSNYC callback or your custom timer when using "read"
    if (mode & 2) { //0x00001F10 && 0x00001F28
        res = sceKernelWaitEventFlag(ctrl.timerID, 1, PSP_EVENT_WAITOR, NULL, NULL); //0x00001F44
        if (res < 0) { //0x00001F4C
            pspSdkSetK1(k1); //0x00001F50
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
        ctrlBuf = intDataPtr->sceCtrlBuf[arg3] + startBufIndex * sizeof(SceCtrlDataExt); //0x00002160
    }
    else {
         ctrlBuf = intDataPtr->sceCtrlBuf[0] + startBufIndex * sizeof(SceCtrlData); //0x00001FB4
    }
    if (readIntBufs < 0) { //0x00001FB8
        sceKernelCpuResumeIntr(suspendFlag); //0x000020A8
        pspSdkSetK1(k1); //0x000020B0
        return readIntBufs;
    }  
    while (reqBufReads-- > 0) { //0x0000209C & 0x000020A0
           /* Default SceCtrlData structure is used. */
           ((SceCtrlData *)pad)->activeTime = ((SceCtrlData *)ctrlBuf)->activeTime; //0x00001FE4 & 0x00001FF0
         
           buttons = ((SceCtrlData *)ctrlBuf)->buttons; //0x00001FE8
           if (pspSdkGetK1() >> 31) { //0x00001FEC          
               buttons &= ctrl.userModeButtons; //0x00001FF8 -- Save only user mode buttons.
           }
           /* The current button value(s) will be turned negative, if this function is called through read/peekBufferNegative. */
           ((SceCtrlData *)pad)->buttons = (mode & 1) ? ~buttons : buttons; //0x00002004 & 0x00002004 & 0x0000200C
         
           ((SceCtrlData *)pad)->aX = ((SceCtrlData *)ctrlBuf)->aX; //0x00002010
           ((SceCtrlData *)pad)->aY = ((SceCtrlData *)ctrlBuf)->aY; //0x00002018
        
           if ((mode & 4) == 0) { //if (mode < 4) -- 0x00001FC8 & 0x00002014
               for (i = 0; i < 6; i++) { //0x00002130 - 0x00002138
                    ((SceCtrlData *)pad)->rsrv[i] = 0;
               }
               pad = pad + sizeof(SceCtrlData); //0x00002140
           }
           else { //if (mode >= 4)
               /* Extended SceCtrlData structure is used. */
               ((SceCtrlDataExt *)pad)->activeTime = ((SceCtrlDataExt *)ctrlBuf)->activeTime; //0x00001FE4 & 0x00001FF0
             
               buttons = ((SceCtrlData *)ctrlBuf)->buttons; //0x00001FE8
               if (pspSdkGetK1() >> 31) { //0x00001FEC                 
                   buttons &= ctrl.userModeButtons; //0x00001FF8 -- Save only user mode buttons
               }
               /* The current button value(s) will be turned negative, if this function is called through read/peekBufferNegative. */
               ((SceCtrlDataExt *)pad)->buttons = (mode & 1) ? ~buttons : buttons; //0x00002004 & 0x00002004 & 0x0000200C
             
               ((SceCtrlDataExt *)pad)->aX = ((SceCtrlDataExt *)ctrlBuf)->aX; //0x00002010
               ((SceCtrlDataExt *)pad)->aY = ((SceCtrlDataExt *)ctrlBuf)->aY; //0x00002018           
               ((SceCtrlDataExt *)pad)->rsrv[2] = 0; //0x0000201C
               ((SceCtrlDataExt *)pad)->rsrv[3] = 0; //0x00002024
               if (arg3 == 0) { //0x00002020
                   ((SceCtrlDataExt *)pad)->rsrv[0] = -128; //0x00002100
                   ((SceCtrlDataExt *)pad)->rsrv[1] = -128; //0x00002104
                   ((SceCtrlDataExt *)pad)->rsrv[4] = 0; //0x00002108
                   ((SceCtrlDataExt *)pad)->rsrv[5] = 0; //0x00002108
                   ((SceCtrlDataExt *)pad)->unk1 = 0; //0x0000210C
                   ((SceCtrlDataExt *)pad)->unk2 = 0; //0x00002110
                   ((SceCtrlDataExt *)pad)->unk3 = 0; //0x00002114
                   ((SceCtrlDataExt *)pad)->unk4 = 0; //0x00002118
                   ((SceCtrlDataExt *)pad)->unk5 = 0; //0x0000211C
                   ((SceCtrlDataExt *)pad)->unk6 = 0; //0x00002120
                   ((SceCtrlDataExt *)pad)->unk7 = 0; //0x00002124
                   ((SceCtrlDataExt *)pad)->unk8 = 0; //0x0000212C
               }
               else {
                   ((SceCtrlDataExt *)pad)->rsrv[0] = ((SceCtrlDataExt *)ctrlBuf)->rsrv[0]; //0x00002034
                   ((SceCtrlDataExt *)pad)->rsrv[1] = ((SceCtrlDataExt *)ctrlBuf)->rsrv[1]; //0x00002038
                   ((SceCtrlDataExt *)pad)->rsrv[4] = ((SceCtrlDataExt *)ctrlBuf)->rsrv[4]; //0x0000203C
                   ((SceCtrlDataExt *)pad)->rsrv[5] = ((SceCtrlDataExt *)ctrlBuf)->rsrv[5]; //""
                   ((SceCtrlDataExt *)pad)->unk1 = ((SceCtrlDataExt *)ctrlBuf)->unk1; //0x00002044
                   ((SceCtrlDataExt *)pad)->unk2 = ((SceCtrlDataExt *)ctrlBuf)->unk2; //...
                   ((SceCtrlDataExt *)pad)->unk3 = ((SceCtrlDataExt *)ctrlBuf)->unk3;
                   ((SceCtrlDataExt *)pad)->unk4 = ((SceCtrlDataExt *)ctrlBuf)->unk4;
                   ((SceCtrlDataExt *)pad)->unk5 = ((SceCtrlDataExt *)ctrlBuf)->unk5;
                   ((SceCtrlDataExt *)pad)->unk6 = ((SceCtrlDataExt *)ctrlBuf)->unk6;
                   ((SceCtrlDataExt *)pad)->unk7 = ((SceCtrlDataExt *)ctrlBuf)->unk7; //...
                   ((SceCtrlDataExt *)pad)->unk8 = ((SceCtrlDataExt *)ctrlBuf)->unk8; //0x0000207C              
               }
               pad = pad + sizeof(SceCtrlDataExt); //0x00002080
           }
           startBufIndex++; //0x0000208C        
           if (arg3 == 0) { //0x00002098 
               ctrlBuf = ctrlBuf + startBufIndex * sizeof(SceCtrlData); //0x00002084 & 0x00002090
           }
           else {
               ctrlBuf = ctrlBuf + startBufIndex * sizeof(SceCtrlDataExt); //0x00002088 & 0x00002098
           }
           if (startBufIndex == CTRL_INTERNAL_CONTROLLER_BUFFERS) { //0x00002094
               startBufIndex = 0; //0x000020EC
               if (arg3 != 0) { //0x000020E8
                   ctrlBuf = intDataPtr->sceCtrlBuf[0]; //0x000020F4
               }
               else {
                   ctrlBuf = intDataPtr->sceCtrlBuf[arg3]; //0x00001FC4 && 0x000020FC
               }
           }
    }
 
    sceKernelCpuResumeIntr(suspendFlag); //0x000020A8
    pspSdkSetK1(k1); //0x000020B0
    return readIntBufs;
}

/*
 * Subroutine sceCtrl_driver_4FAA342D - Address 0x00000000
 * Exported in sceCtrl_driver
 */
int sceCtrlInit() {
    int eventId;
    int keyConfig;
    SceSysTimerId timerId;
    u32 supportedUserButtons;
    int unk_2;
    void (*func)(SceKernelDeci2Ops *);
    UnknownType *retPtr;
    int pspModel;
    
    memset(ctrl, 0, sizeof(SceCtrl)); //0x00000024
    ctrl.pollMode = PSP_CTRL_POLL_POLLING; //0x00000048
    ctrl.userModeData.sceCtrlBuf = (void *)0x00002BB0; //0x00000054 -- size of SceCtrlData is 16 (default) -> required size is 0x400.
    //Default values of the analog pad.
    ctrl.analogY = CTRL_ANALOG_PAD_DEFAULT_VALUE; //0x00000060
    ctrl.analogX = CTRL_ANALOG_PAD_DEFAULT_VALUE; //0x00000064
    ctrl.kernelModeData.sceCtrlBuf = (void *)0x00002FB0; //0x0000006C
    ctrl.unk_1 = -1; //0x00000074
    
    eventId = sceKernelCreateEventFlag("SceCtrl", 1, 0, NULL); //0x00000070
    ctrl.eventFlag = eventId; //0x0000007C
    
    timerId = sceSTimerAlloc(); //0x00000078
    if (timerId < 0) { //0x00000080
        return timerId;
    }
    ctrl.timerID = timerId;
    SysTimerForKernel_B53534B4(timerId, 1, 0x30); //0x00000094
    ctrl.unk_Byte_1 = -1; //0x000000A0
    sceKernelRegisterSysEventHandler(&ctrlSysEvent); //0x000000A4
    sceSyscon_driver_C325BF4B(0); //0x000000AC
    
    keyConfig = sceKernelInitKeyConfig(); //0x000000B4
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
    if (pspModel < 10) { //0x0000010C
        switch (pspModel) { //0x00000128
            case 0: case 1: case 2: case 3: case 6: case 8:
                unk_2 = 0x1FFF3F9; //0x00000130 & 0x00000138               
                break;
            case 4: case 5: case 7: case 9:
                unk_2 = CTRL_ALL_SUPPORTED_BUTTONS; //0x000001EC & 0x00000138
                break;
        }
        ctrl.unk_4 = unk_2; //0x0000013C
    }
    else { //0x000001F0
        ctrl.unk_4 = 0x1FFF3F9; //0x00000110 & 0x000001F0
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
    retPtr = sceKernelDeci2pReferOperations(); 
    if ((retPtr != NULL) && (*retPtr == 48)) { //0x000001A0 & 0x000001AC
         func = *(retPtr + 0x2C)(SceKernelDeci2Ops *); //0x000001B0
         func(&ctrlDeci2Ops); //0x000001D8            
    }
    return 0;
}

/*
 * Subroutine sceCtrl_driver_08DE9E04 - Address 0x0000020C
 * Exported in sceCtrl_driver
 */
int sceCtrlEnd() {
    SceSysTimerId timerId;
    
    sceKernelUnregisterSysEventHandler(&ctrlSysEvent); //0x00000220
    sceSyscon_driver_C325BF4B(1); //0x00000228
    sceDisplayWaitVblankStart(); //0x00000230
    
    timerId = ctrl.timerID; //0x0000023C
    ctrl.timerID = -1; //0x00000244
    if (timerId >= 0) { //0x00000248
        sceSTimerStopCount(timerId); //0x00000250
        sceSTimerFree(timerId); //0x00000258
    }
    sceKernelReleaseSubIntrHandler(PSP_VBLANK_INT, 0x13); //0x00000264
    sceKernelDeleteEventFlag(ctrl.eventFlag); //0x0000026C
    
    while (ctrl.suspendSamples != 0) { //0x0000028C
           sceDisplayWaitVblankStart(); //0x00000280
    }
    return 0; //0x000002A0
}

/*
 * Subroutine sceCtrl_driver_1CDEDDBC - Address 0x00001A50 
 * Exported in sceCtrl_driver
 */
int sceCtrlSuspend() {  
    int cycle;
    
    cycle = ctrl.btnCycle; //0x00001A58
    if (cycle != 0) { //0x00001A68
        sceSTimerStopCount(ctrl.timerID); //0x00001A88
    }
    else {
         sceKernelDisableSubIntr(PSP_VBLANK_INT, 0x13); //0x00001A70 
    }
    return 0;       
}

/*
 * Subroutine sceCtrl_driver_8543BB3B - Address 0x000002AC 
 * Exported in sceCtrl_driver
 */
int sceCtrlResume() {
    int retVal;
    int prevButtons;
    
    retVal = sceSyscon_driver_A59F82EB(); //0x000002B8
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
    return 0;
} 

/*
 * Subroutine sceCtrl_driver_196CF2F4 - Address 0x00001C30 
 * Exported in sceCtrl_driver
 */
int sceCtrlSetPollingMode(pspCtrlPadPollMode pollMode) {
    ctrl.pollMode = pollMode; //0x00001C3C
    return 0;
}

/*
 * Subroutine sceCtrl_DA6B76A1 - Address 0x00001330 - Aliases: sceCtrl_driver_410833B7
 * Exported in sceCtrl
 * Exported in sceCtrl_driver
 */
pspCtrlPadInputMode sceCtrlGetSamplingMode(pspCtrlPadInputMode *mode) {
    int privMode;
    int index;
    
    privMode = pspSdkGetK1() << 11; //0x00001330
    index = ((privMode >> 31) < 1); //0x00001338 & 0x00001340
    if ((privMode & mode) >= 0) { //0x00001348 & 0x00001350
        *mode = ctrl.samplingMode[index]; //0x00001358 & 0x0000135C
    }
    return 0; 
}

/* Subroutine sceCtrl_1F4011E6 - Address 0x000012C8 - Aliases: sceCtrl_driver_6CB49301
 * Exported in sceCtrl
 * Exported in sceCtrl_driver
 */
pspCtrlPadInputMode sceCtrlSetSamplingMode(pspCtrlPadInputMode mode) {    
    int suspendFlag;
    int privMode;
    u8 index;
    pspCtrlPadInputMode prevMode;
    
    if (mode > CTRL_SAMPLING_MODE_MAX_MODE) { //0x000012D0 & 0x000012E4
        return SCE_ERROR_INVALID_MODE; 
    }
    suspendFlag = sceKernelCpuSuspendIntr(); //0x000012EC
    privMode = (pspSdkGetK1() >> 20) & 0x1; //0x000012F4
    index = (privMode < 1); //0x000012FC
    
    prevMode = ctrl.samplingMode[index]; //0x00001308
    ctrl.samplingMode[index] = mode; //0x0000130C
    
    sceKernelCpuResumeIntr(suspendFlag); //0x00001310:
    return prevMode; //0x00001318   
}

/*
 * Subroutine sceCtrl_02BAAD91 - Address 0x00001AB8 - Aliases: sceCtrl_driver_4E972A76
 * Exported in sceCtrl
 * Exported in sceCtrl_driver
 */
int sceCtrlGetSamplingCycle(u32 *cycle) {
    int privMode;
    
    privMode = pspSdkGetK1() << 11; //0x00001AB8   
    if ((privMode & cycle) >= 0) { //0x00001ABC && 0x00001AC0
        *cycle = ctrl.btnCycle; //0x00001ACC && 0x00001AD0
    }
    return 0;
}

/*
 * Subroutine sceCtrl_6A2774F3 - Address 0x0000136C - Aliases: sceCtrl_driver_855C255D
 * Exported in sceCtrl
 * Exported in sceCtrl_driver
 */
int sceCtrlSetSamplingCycle(u32 cycle) {
    int k1;
    int suspendFlag;
    int prevCycle;
    int sdkVersion;
    u32 nCycle;
    
    k1 = pspSdkGetK1();
    pspSdkSetK1(k1 << 11); //0x00001378
    
    suspendFlag = sceKernelCpuSuspendIntr(); //0x00001398
    if (cycle == 0) { //0x000013B0        
        prevCycle = ctrl.btnCycle; //0x00001460
        sceKernelEnableSubIntr(PSP_VBLANK_INT, 0x13); //0x00001464
        ctrl.btnCycle = 0; //0x00001468
        
        sceSTimerSetHandler(ctrl.timerID, 0, NULL, 0); //0x00001478
        sceSTimerStopCount(ctrl.timerID); //0x00001480            
    }
    else {
        if (cycle < CTRL_BUFFER_UPDATE_MIN_CUSTOM_CYCLES || cycle > CTRL_BUFFER_UPDATE_MAX_CUSTOM_CYCLES) { //0x000013B4 & 0x000013B8
            return SCE_ERROR_INVALID_VALUE; //0x0000138C & 0x000013C0
        }
        else {
             prevCycle = ctrl.btnCycle; //0x0000140C
             sceSTimerStartCount(ctrl.timerID); //0x00001410
             ctrl.btnCycle = cycle; //0x00001414
             sdkVersion = sceKernelGetCompiledSdkVersion(); //0x00001418
        
             nCycle = ((0x204FFFF < sdkVersion) < 1); //0x00001428 && 0x0000142C
             nCycle += cycle; //0x00001434 -- If the PSP Firmware version is < 2.5, the original cycle is used, otherwise original cycle + 1
             sceSTimerSetHandler(ctrl.timerID, nCycle, _sceCtrlTimerIntr, 0); //0x00001408 && 0x00001444
             sceKernelDisableSubIntr(PSP_VBLANK_INT, 0x13); //0x00001450      
       }
    }
    sceKernelCpuResumeIntr(suspendFlag); //0x000013D8
    pspSdkSetK1(k1); //0x000013E4
    return prevCycle; //0x000013E0  
}

/*
 * Subroutine sceCtrl_687660FA - Address 0x0000170C - Aliases: sceCtrl_driver_390D1A49
 * Exported in sceCtrl
 * Exported in sceCtrl_driver
 */
int sceCtrlGetIdleCancelThreshold(int *idleReset, int *idleBack) {
    int k1;
    int privMode;
    int suspendFlag;
    int tempIdleBack;
    int tempIdleReset;   
    
    privMode = pspSdkGetK1() << 11; //0x00001710    
    idleReset = idleReset | privMode; //0x0000171C
    k1 = pspSdkSetK1(privMode); //0x00001728
    
    if (((idleReset & privMode) < 0) || ((privMode & idleBack) < 0)) { //0x00001734 & 0x00001740
        pspSdkSetK1(k1);
        return SCE_ERROR_PRIV_REQUIRED; /* protect kernel addresses from user mode. */
    }   
    suspendFlag = sceKernelCpuSuspendIntr(); //0x0000176C  
    if (idleReset != NULL) { //0x00001774
        tempIdleReset = ctrl.idleReset;
        *idleReset = (tempIdleReset == 129) ? -1 : idleReset; //0x00001788
    }
    if (idleBack != NULL) { //0x00001794
        tempIdleBack = ctrl.idleBack;
        *idleBack = (tempIdleBack == 129) ? -1 : tempIdleBack; //0x000017A4
    }
    sceKernelCpuResumeIntr(suspendFlag); //0x000017B0
    pspSdkSetK1(k1); //0x000017B8
    return 0; //0x000017C0
}

/*
 * Subroutine sceCtrl_A7144800 - Address 0x00001680 - Aliases: sceCtrl_driver_84CEAE74
 * Exported in sceCtrl
 * Exported in sceCtrl_driver
 */
int sceCtrlSetIdleCancelThreshold(int idlereset, int idleback) {
    int suspendFlag;
    
    if ((idlereset < -1 && idlereset > 128) && (idleback < -1 && idleback > 128)) { //0x000016A0 & 0x000016B4 & 0x000016C8
        return SCE_ERROR_INVALID_VALUE; //0x000016A8 & 0x000016AC & 0x00001700
    }       
    suspendFlag = sceKernelCpuSuspendIntr(); //0x000016D0
    
    ctrl.idleBack = (idleback == -1) ? 129 : idleback; //0x000016D4 & 0x000016E0
    ctrl.idleReset = (idlereset == -1) ? 129 : idlereset; //0x000016CC & 0x000016EC
    
    sceKernelCpuResumeIntr(suspendFlag); //0x000016E8
    return 0; //0x000016F0
}

/*
 * Subroutine sceCtrl_AF5960F3 - Address 0x00001C6C - Aliases: sceCtrl_driver_525D27AC
 * Exported in sceCtrl
 * Exported in sceCtrl_driver
 */
u16 sceCtrlGetSuspendingExtraSamples() {
    u16 curSuspendSamples;
    
    curSuspendSamples = ctrl.suspendSamples;  //0x00001C74
    return curSuspendSamples;
}

/*
 * Subroutine sceCtrl_348D99D4 - Address 0x00001C40 - Aliases: sceCtrl_driver_53E67075
 * Exported in sceCtrl
 * Exported in sceCtrl_driver
 */
int sceCtrlSetSuspendingExtraSamples(u16 suspendSamples) {
    u16 nSuspendSamples;
    
    if (suspendSamples > CTRL_MAX_EXTRA_SUSPEND_SAMPLES) { //0x00001C44
        return SCE_ERROR_INVALID_VALUE; //0x00001C48 & 0x00001C54
    }
    nSuspendSamples = ((suspendSamples ^ 0x1) == 0) ? 0 : suspendSamples; //0x00001C40 & 0x00001C4C
    ctrl.suspendSamples = nSuspendSamples;
    
    return 0;
}

/*
 * Subroutine sceCtrl_driver_65698764 - Address 0x000011F0 
 * Exported in sceCtrl_driver
 */
int sceCtrlExtendInternalCtrlBuffers(u8 mode, int arg2, int arg3) {
    int isExtended;
    SceUID poolId;
    void *ctrlBuf;    
    
    //0x0000122C
    if (mode > 2) {
        return SCE_ERROR_INVALID_VALUE;
    }
    isExtended = ctrl.unk_array2[mode]; //0x00001238
    if (!isExtended) { //0x00001254
        poolId = sceKernelCreateFpl("SceCtrlBuf", PSP_MEMORY_PARTITION_KERNEL, 0, 2 * sizeof(SceCtrlDataExt) * CTRL_INTERNAL_CONTROLLER_BUFFERS, 1, NULL); //0x00001294
        if (poolId < 0) { //0x000012A4
            return poolId;
        }
        sceKernelTryAllocateFpl(poolId, &ctrlBuf); //0x000012AC
        ctrl.kernelModeData.sceCtrlBuf = (void *)ctrlBuf + sizeof(SceCtrlDataExt) * CTRL_INTERNAL_CONTROLLER_BUFFERS; //0x000012BC
        ctrl.userModeData.sceCtrlBuf = (void *)ctrlBuf; //0x000012C4
    }
    ctrl.unk_array3[mode] = arg3; //0x00001264
    ctrl.unk_array2[mode] = arg2; //0x0000126C
    return 0;
}

/*
 * Subroutine sceCtrl_B1D0E5CD - Address 0x00001490 - Aliases: sceCtrl_driver_6574DC7C
 * Exported in sceCtrl
 * Exported in sceCtrl_driver
 */
int sceCtrlPeekLatch(SceCtrlLatch *latch) {
    int k1;
    int suspendFlag;
    SceCtrlInternalData *latchPtr;
    
    k1 = pspSdkGetK1();
    pspSdkSetK1(k1 << 11); //0x0000149C  
    suspendFlag = sceKernelCpuSuspendIntr(); //0x000014A8
    
    if ((pspSdkGetK1() & latch) < 0) { //0x000014B8
        sceKernelCpuResumeIntr(suspendFlag); //0x00001524
        pspSdkSetK1(k1); //0x00001530
        return SCE_ERROR_PRIV_REQUIRED; //0x0000152C & 0x00001538 -- protect kernel address from usermode
    }
    if (pspSdkGetK1() >= 0) { //0x000014C4
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
    pspSdkSetK1(k1); //0x000014FC
    return latchPtr->readLatchCount; //0x00001500
}

/*
 * Subroutine sceCtrl_0B588501 - Address 0x0000153C - Aliases: sceCtrl_driver_D883CAF9
 * Exported in sceCtrl
 * Exported in sceCtrl_driver
 */
int sceCtrlReadLatch(SceCtrlLatch *latch) {
    int k1;
    int suspendFlag;
    SceCtrlInternalData *latchPtr;
    int readLatchCount;
    
    k1 = pspSdkGetK1(); //0x00001540
    pspSdkSetK1(k1 << 11); //0x00001548
    suspendFlag = sceKernelCpuSuspendIntr(); //0x00001554
    
    if ((pspSdkGetK1() & latch) < 0) { //0x00001564 -- protect kernel address from usermode     
        sceKernelCpuResumeIntr(suspendFlag); //0x00001620
        pspSdkSetK1(k1); //0x0000162C
        return SCE_ERROR_PRIV_REQUIRED; //0x00001634
    }
    if (pspSdkGetK1() >= 0) { //0x00001570
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
    pspSdkSetK1(k1); //0x000015C0
    return readLatchCount; //0x000015C4
} 

/*
 * Subroutine sceCtrl_3A622550 - Address 0x00001AE0 - Aliases: sceCtrl_driver_18654FC0
 * Exported in sceCtrl
 * Exported in sceCtrl_driver
 */
int sceCtrlPeekBufferPositive(SceCtrlData *pad, u8 reqBufReads) {
    int readBuffers;
    
    readBuffers = _sceCtrlReadBuf(pad, reqBufReads, 0, 0); //0x00001AEC
    return readBuffers;
}

/*
 * Subroutine sceCtrl_C152080A - Address 0x00001B00 - Aliases: sceCtrl_driver_02DD57CF
 * Exported in sceCtrl
 * Exported in sceCtrl_driver
 */
int sceCtrlPeekBufferNegative(SceCtrlData *pad, u8 reqBufReads) {
    int readBuffers;
    
    readBuffers = _sceCtrlReadBuf(pad, reqBufReads, 0, 1); //0x00001B0C
    return readBuffers;
}

/*
 * Subroutine sceCtrl_1F803938 - Address 0x00001B20 - Aliases: sceCtrl_driver_9F3038AC
 * Exported in sceCtrl
 * Exported in sceCtrl_driver
 */
int sceCtrlReadBufferPositive(SceCtrlData *pad, u8 reqBufReads) {
    int readBuffers;
    
    readBuffers = _sceCtrlReadBuf(pad, reqBufReads, 0, 2); //0x00001B2C
    return readBuffers;
}

/*
 * Subroutine sceCtrl_60B81F86 - Address 0x00001B40 - Aliases: sceCtrl_driver_EB5F1D7A
 * Exported in sceCtrl
 * Exported in sceCtrl_driver
 */
int sceCtrlReadBufferNegative(SceCtrlData *pad, u8 reqBufReads) {
    int readBuffers;
    
    readBuffers = _sceCtrlReadBuf(pad, reqBufReads, 0, 3); //0x00001B4C
    return readBuffers;
}

/*
 * Subroutine sceCtrl_5A36B1C2 - Address 0x00001B60 - Aliases: sceCtrl_driver_1D75C1D4
 * Exported in sceCtrl
 * Exported in sceCtrl_driver
 */
int sceCtrlPeekBufferPositiveExt(int arg1, SceCtrlDataExt *pad, u8 reqBufReads) {
    int readBuffers;
    
    readBuffers = _sceCtrlReadBuf(pad, reqBufReads, arg1, 4); //0x00001B78
    return readBuffers;
}

/*
 * Subroutine sceCtrl_239A6BA7 - Address 0x00001B8C - Aliases: sceCtrl_driver_6E552572
 * Exported in sceCtrl
 * Exported in sceCtrl_driver
 */
int sceCtrlPeekBufferNegativeExt(int arg1, SceCtrlDataExt *pad, u8 reqBufReads) {
    int readBuffers;
    
    readBuffers = _sceCtrlReadBuf(pad, reqBufReads, arg1, 5); //0x00001BA4
    return readBuffers;
}

/*
 * Subroutine sceCtrl_1098030B - Address 0x00001BB8 - Aliases: sceCtrl_driver_16BB4085
 * Exported in sceCtrl
 * Exported in sceCtrl_driver
 */
int sceCtrlReadBufferPositiveExt(int arg1, SceCtrlDataExt *pad, u8 reqBufReads) {
    int readBuffers;
    
    readBuffers = _sceCtrlReadBuf(pad, reqBufReads, arg1, 6); //0x00001BD0
    return readBuffers;
}

/*
 * Subroutine sceCtrl_7C3675AB - Address 0x00001BE4 - Aliases: sceCtrl_driver_4870C6AF
 * Exported in sceCtrl
 * Exported in sceCtrl_driver
 */
int sceCtrlReadBufferNegativeExt(int arg1, SceCtrlDataExt *pad, u8 reqBufReads) {
    int readBuffers;
    
    readBuffers = _sceCtrlReadBuf(pad, reqBufReads, arg1, 7); //0x00001BFC
    return readBuffers;
}

/*
 * Subroutine sceCtrl_A68FD260 - Address 0x00001DA8 - Aliases: sceCtrl_driver_FAF675CB
 * Exported in sceCtrl
 * Exported in sceCtrl_driver
 */
int sceCtrlClearRapidFire(u8 slot) {
    if (slot > CTRL_BUTTONS_RAPID_FIRE_MAX_SLOT) { //0x00001DBC && 0x00001DC0
        return SCE_ERROR_INVALID_INDEX;
    }
    ctrl.rapidFire[slot].pressedButtonRange = 0; //0x00001DC8
    return 0;
}

/*
 * Subroutine sceCtrl_6841BE1A - Address 0x000018DC - Aliases: sceCtrl_driver_E96A4D84
 * Exported in sceCtrl
 * Exported in sceCtrl_driver
 */
int sceCtrlSetRapidFire(u8 slot, u32 pressedBtnRange, u32 reqBtnsEventTrigger, u32 reqBtn, u8 reqBtnEventOnTime, u8 reqBtnOnTime, u8 reqBtnOffTime) {
    u32 usedButtons;
    u32 kernelButtons;
    int k1;
    int suspendFlag;
    
    if (slot > CTRL_BUTTONS_RAPID_FIRE_MAX_SLOT) { //0x000018FC & 0x0000193C
        return SCE_ERROR_INVALID_INDEX;
    }
    if ((reqBtnEventOnTime | reqBtnOnTime | reqBtnOffTime) > CTRL_MAX_INTERNAL_CONTROLLER_BUFFER) { //0x000018E8 & 0x000018F4 & 0x00001910 & 0x00001954
        return SCE_ERROR_INVALID_VALUE;
    }
    k1 = pspSdkGetK1(); //0x0000194C
    pspSdkSetK1(k1 << 11); // 0x00001960
    
    usedButtons = pressedBtnRange | reqBtnsEventTrigger; //0x0000195C    
    usedButtons = usedButtons | reqBtn; //0x00001968
    if (k1 < 0) { //0x00001964
        kernelButtons = ~ctrl.userModeButtons; //0x0000197C
        kernelButtons = kernelButtons | PSP_CTRL_HOLD; //0x00001980
        //if the buttons given by the user include kernel buttons or the HOLD button -> return error
        if (kernelButtons & usedButtons) { //0x00001984 & 0x00001988
            pspSdkSetK1(k1); //0x00001A0C
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
    pspSdkSetK1(k1); //0x000019CC
    return 0;
}

/*
 * Subroutine sceCtrl_driver_A759DB6A - Address 0x00001CB8
 * Exported in sceCtrl_driver
 */
int sceCtrlSetAnalogEmulation(u8 slot, u8 aXEmu, u8 aYEmu, u32 bufUpdates) {
    
    if (slot > CTRL_DATA_EMULATION_MAX_SLOT) { //0x00001CE4
        return SCE_ERROR_INVALID_VALUE; //0x00001CD0 & 0x00001CE8
    }
    aXEmu &= 0xFF; //0x00001CDC
    aYEmu &= 0xFF; //0x00001CE0
    
    ctrl.emulatedData[slot].analogXEmulation = aXEmu; //0x00001CF8
    ctrl.emulatedData[slot].analogYEmulation = aYEmu; //0x00001CEC
    ctrl.emulatedData[slot].intCtrlBufUpdates = bufUpdates; //0x00001CF4
    
    return 0;
}

/*
 * Subroutine sceCtrl_driver_094EF1BB - Address 0x00001C78 
 * Exported in sceCtrl_driver
 */
int sceCtrlSetButtonEmulation(u8 slot, u32 uModeBtnEmu, u32 kModeBtnEmu, u32 bufUpdates) {
    
    if (slot > CTRL_DATA_EMULATION_MAX_SLOT) { //0x00001C98
        return SCE_ERROR_INVALID_VALUE; //0x00001C90 & 0x00001C9C
    }
    ctrl.emulatedData[slot].uModeBtnEmulation = uModeBtnEmu; //0x00001CA8
    ctrl.emulatedData[slot].kModeBtnEmulation = kModeBtnEmu; //0x00001CAC
    ctrl.emulatedData[slot].intCtrlBufUpdates2 = bufUpdates; //0x00001CA0
    
    return 0;
}


/* Subroutine sceCtrl_driver_33AB5BDB - Address 0x00001878
 * Exported in sceCtrl_driver 
 */
pspCtrlPadButtonMaskMode sceCtrlGetButtonIntercept(u32 btnMask) {   
    int suspendFlag;
    int curMaskSupBtns;
    int curMaskSetBtns;
    pspCtrlPadButtonMaskMode btnMaskMode = PSP_CTRL_MASK_IGNORE_BUTTON_MASK;
       
    suspendFlag = sceKernelCpuSuspendIntr(); //0x0000188C
    curMaskSetBtns = ctrl.maskSetButtons; //0x0000189C
    curMaskSupBtns = ctrl.maskSupportButtons; //0x000018A0             
              
    if (curMaskSupBtns & btnMask) { //0x000018AC          
        btnMaskMode = (curMaskSetBtns & btnMask) ? PSP_CTRL_MASK_IGNORE_BUTTON_MASK : PSP_CTRL_MASK_DELETE_BUTTON_MASK_SETTING; //0x000018B0 & 0x000018B8
    }
    
    sceKernelCpuResumeIntr(suspendFlag);
    return btnMaskMode;  
}

/* Subroutine sceCtrl_driver_5B15473C - Address 0x000017C4
 * Exported in sceCtrl_driver 
 */
pspCtrlPadButtonMaskMode sceCtrlSetButtonIntercept(u32 mask, pspCtrlPadButtonMaskMode buttonMaskMode) {   
    int curMaskSupBtns = ctrl.maskSupportButtons;
    int newMaskSupBtns;
    int curMaskSetBtns = ctrl.maskSetButtons;
    int newMaskSetBtns;
    int suspendFlag;    
    pspCtrlPadButtonMaskMode prevBtnMaskMode = PSP_CTRL_MASK_IGNORE_BUTTON_MASK;
       
    suspendFlag = sceKernelCpuSuspendIntr(); //0x000017E0
    
    if (mask & curMaskSupBtns) { //0x00001800        
        prevBtnMaskMode = (mask & curMaskSetBtns) ?  PSP_CTRL_MASK_SET_BUTTON_MASK : PSP_CTRL_MASK_DELETE_BUTTON_MASK_SETTING; //0x00001808 & 0x00001810
    }
    if (buttonMaskMode != PSP_CTRL_MASK_DELETE_BUTTON_MASK_SETTING) { //0x00001814
        if (buttonMaskMode == PSP_CTRL_MASK_IGNORE_BUTTON_MASK) { //0x00001818 & 0x00001850
            newMaskSupBtns = curMaskSupBtns & ~mask; //0x00001854 & 0x00001874
            newMaskSetBtns = curMaskSetBtns & ~mask; //0x00001850 & 0x0000186C          
        }
        else if (buttonMaskMode == PSP_CTRL_MASK_SET_BUTTON_MASK) { //0x0000185C
                 newMaskSupBtns = curMaskSupBtns | mask; //0x00001868
                 newMaskSetBtns = curMaskSetBtns | mask; //0x00001820
        }
    }
    else {
         newMaskSetBtns = curMaskSetBtns & ~mask; //0x000017F8 & 0x00001804 & 0x0000181C
         newMaskSupBtns = curMaskSupBtns | mask; //0x000017E4 & 0x000017F0 & 0x00001820
    }
    ctrl.maskSupportButtons = newMaskSupBtns; //0x00001828
    ctrl.maskSetButtons = newMaskSetBtns; //0x00001830 
    
    sceKernelCpuResumeIntr(suspendFlag); //0x0000182C
    return prevBtnMaskMode;
}

/* Subroutine sceCtrl_driver_5D8CE0B2 - Address 0x00001D04
 * Exported in sceCtrl_driver 
 */
int sceCtrlSetSpecialButtonCallback(u32 slot, u32 btnMask, SceCtrlCb cb, void *arg) {   
    int suspendFlag;
     
    if (slot > CTRL_BUTTON_CALLBACK_MAX_SLOT) { //0x00001D14 & 0x00001D34
        return SCE_ERROR_INVALID_INDEX;
    }      
    suspendFlag = sceKernelCpuSuspendIntr(); //0x00001D3C
    
    //collect information for a button callback
    ctrl.buttonCallback[slot].btnMask = btnMask; //0x00001D54
    ctrl.buttonCallback[slot].callbackFunc = cb; //0x00001D58
    ctrl.buttonCallback[slot].arg = arg; //0x00001D64
    ctrl.buttonCallback[slot].gp = GP_BACKUP(); //0x00001D6C
            
    sceKernelCpuResumeIntr(suspendFlag); //0x00001D68 
    return 0;   
}

/*
 * Subroutine sceCtrl_driver_DEFAD580 - Address 0x00001AA8 
 * Exported in sceCtrl_driver
 */
int sceCtrl_driver_DEFAD580(int arg1) {
    ctrl.unk_array2[0] = arg1; //0x00001AB4
    return 0;
}

/*
 * Subroutine sceCtrl_driver_BD119FAB - Address 0x00001638
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
    return 0; //0x0000167C
}

/*
 * sceCtrl_driver_0D627B90 - Address 0x00001C10
 * Exported in sceCtrl_driver
 */
int sceCtrlSetIdleCancelKey(int arg1, int arg2, int arg3, int arg4) {
    ctrl.unk_8 = arg4; //0x00001C1C
    ctrl.unk_5 = arg1; //0x00001C20
    ctrl.unk_6 = arg2; //0x00001C24
    ctrl.unk_7 = arg3; //0x00001C2C
    
    return 0; //0x00001C18
}

/*
 * Subroutine sceCtrl_driver_BEF3B4C9 - Address 0x00001A98
 * Exported in sceCtrl_driver
 */
int sceCtrl_driver_BEF3B4C9(char arg1) {
    ctrl.unk_Byte_3 = arg1; //0x00001AA4
    return 0;
}

/*
 * sceCtrl_driver_5F20A0F0 - Address 0x00001D94 
 * Exported in sceCtrl_driver
 */
int sceCtrlUpdateCableTypeReq(char arg1) {
    ctrl.unk_Byte_0 = arg1; //0x00001D9C
    return 0;
}

/*
 * Subroutine module_start - Address 0x00001A10 
 * Exported in syslib
 */
int module_start(SceSize args, void *argp) {    
    sceCtrlInit();
    return 0;
}

/*
 * Subroutine module_reboot_before - Address 0x00001A30
 * Exported in syslib
 */
int module_reboot_before(SceSize args, void *argp) {
    sceCtrlEnd();
    return 0;
}