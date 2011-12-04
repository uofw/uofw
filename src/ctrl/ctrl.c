/**
 * Author: _Felix_
 * Version: 6.39
 *
 * The ctrl.prx module RE'ing.
 */

#include "../ctrl/ctrl.h"

#define PSP_BUTTON_AMOUNT                       16  
#define PSP_INIT_KEYCONFIG_UPDATER              0x110 //might be incorrect

//0x00002890  -- address and structure is likely to change in other firmware revisions
typedef struct _SceCtrl {
    SceSysTimerId timerID; //0
    int eventFlag; //4
    int btn_cycle; //8
    u8 samplingMode[2]; //12 -- two cell array, samplingMode[0] for usermode, samplingMode[1] for kernel mode
    char unk_Byte_0; //14
    char unk_Byte_1; //15
    int unk_0; //16
    u8 unk_Byte_2; //20
    u8 pollMode; //21
    int unk_1; //24
    sceSysconPacket sysPacket; //28
    char unk_Byte_3; //40
    /*...*/
    SceCtrlLatchInternal latch_user; //220
    int latchData_user; //236
    /*...*/
    char unk_Byte_4; //247
    SceCtrlData *(ctrlDataBuf)[64]; //248
    /*...*/
    SceCtrlLatchInternal latch_kernel; //260
    int latchData_kernel; //276
    void *unkAllocData; //288
    /*...*/
    SceCtrlRapidFire rapidFire[16]; //300 - 556 -- 16 cell array
    /*...*/
    int unk_2; //568
    char unk_Byte_5; //580;
    char unk_Byte_6; //581
    /*...*/
    SceCtrlUnk unk_struct; //584 - 4 cell array
    int unk_3; //664
    int mask; //668
    unsigned type; //672
    int unk_4; //676
    int unk_5; //680
    int unk_6; //684
    int unk_7; //688
    int unk_8; //692
    int idleReset; //696
    int idleBack; //700
    SceCtrlButtonCallback buttonCallback[4]; //704   
    int unk_array2[2]; //768 -- 2 size element array
    int unk_array3[2]; //776 -- 2 size element array
} SceCtrl; //sizeof ButtonCtrl: 788 (0x314)

typedef struct _SceCtrlRapidFire {
    /** The button number representing the button for the rapid fire */
    int button; //0
    int unk_1; //4
    int unk_2; //8
    char resv[2]; //12
    u8 unk_3; //14;
    u8 unk_4; //15;
} SceCtrlRapidFire; //sizeof SceCtrlRapidFire: 16

typedef struct _SceCtrlUnk {
    char unk_1; //0
    char unk_2; //1
    char resv[2]; //2
    int unk_3; //4
    int unk_4; //8
    int unk_5; //12
    int unk_6; //16
} SceCtrlUnk; //sizeof SceCtrlUnk: 20

_PspSysEventHandler eventHandler; //0x00002850

typedef UnknownType;

PSP_MODULE_INFO("sceController_Service", 0x1007, 1, 1);

SceCtrl ctrl;

void vblank_handler(int sub, void* arg); //0x00000440
SceUInt SceKernelAlarmHandler(void *); //0x00001DD8
void CmdFunc(); //0x00000610;

/* sub_00000528 */
static int tick_handler() {
    char unk_1 = 7;
    int suspendFlag;
    int retVal;
    
    suspendFlag = sceKernelCpuSuspendIntr(); //0x0000053C
    if (ctrl.btn_cycle != 0) { //0x00000548 && 0x0000054C
        if ((ctrl.unk_0 == 0) && (ctrl.pollMode != 0)) {//0x00000558 && 0x00000564            
            ctrl.unk_0 = 1; //0x000005C8
            if (ctrl.samplingMode[0] != 0) { //0x000005CC
                unk_1 = 8; //0x000005D4
            }
            ctrl.unk_Byte_3 = unk_1; //0x000005D8
            retVal = sceSysconCmdExecAsync(&ctrl.sysPacket, 1, CmdFunc, 0); //0x000005F8
            if (retVal < 0) { 7/0x00000600
                ctrl.unk_0 = 0; //0x00000604
            }
        }
        else {
             sceKernelSetAlarm(700, SceKernelAlarmHandler, NULL); //0x00000578
        }
    }
    if (ctrl.unk_Byte_2 != 0) { //0x00000588
        ctrl.unk_Byte_2 = 0; //0x0000058C
        sceKernelPowerTick(PSP_POWER_TICK_ALL); //0x000005B4       
    }
    
    sceKernelCpuResumeIntr(suspendFlag); //0x00000590
    return -1; //0x000005A8
}

/* Subroutine sceCtrl_driver_5D8CE0B2 - Address 0x00001D04
 * Exported in sceCtrl_driver 
 */
int sceCtrlSetSpecialButtonCallback(int slot, unsigned int btnMask, void (*cb)(int, int, void *), void *arg) {
    
    SceModule *module = NULL;
    int suspendFlag;
    
    //0x00001D14 && 0x00001D34
    if (slot >= 4) {
        return SCE_ERROR_INVALID_INDEX;
    }
    
    //0x00001D3C
    suspendFlag = sceKernelCpuSuspendIntr();
    module = sceKernelFindModuleByName("sceController_Service");
    
    //Intern, the value of no is shifted left 4 bytes to make up the size of ButtonCallback 
    ctrl.buttonCallback[slot].btnMask = btnMask; //0x00001D54 -- 704
    ctrl.buttonCallback[slot].callbackFunc = cb; //0x00001D58 -- 708
    ctrl.buttonCallback[slot].arg = arg; //0x00001D64 -- 716
    ctrl.buttonCallback[slot].gp = module->gp_value; //0x00001D6C -- 712
    
    //0x00001D68        
    sceKernelCpuResumeIntr(suspendFlag); 
    return 0;   
}

/* Subroutine sceCtrl_driver_33AB5BDB - Address 0x00001878
 * Exported in sceCtrl_driver 
 */
int sceCtrlGetButtonIntercept(u32 mask) {
    
    int suspendFlag;
    int curMask;
    int curType;
    int btnMaskVal = PSP_CTRL_MASK_BUTTON_MASK;
    
    //0x0000188C
    suspendFlag = sceKernelCpuSuspendIntr();    
    curType = ctrl.type; //0x0000189C
    curMask = ctrl.mask; //0x000018A0             
            
    //0x000018AC
    if (mask & curType) {       
        //0x000018B0 && 0x000018B8
        btnMaskVal = (mask & curMask) ?  PSP_CTRL_MASK_BUTTON_SET : PSP_CTRL_MASK_NO_SETTING;
    }
    
    sceKernelCpuResumeIntr(suspendFlag);
    return btnMaskVal;  
}

/* Subroutine sceCtrl_driver_5B15473C - Address 0x000017C4
 * Exported in sceCtrl_driver 
 */
int sceCtrlSetButtonIntercept(u32 mask, u8 ButtonMaskMode) {
    
    int curMask = ctrl.mask;
    int curType = ctrl.type;
    int suspendFlag;    
    int retVal = 1;
    
    //0x000017E0
    suspendFlag = sceKernelCpuSuspendIntr();
    
    if (mask & curType) { //0x00001800        
        retVal = (mask & curMask) ?  2 : 0; //0x00001808 && 0x00001810
    }
    if (ButtonMaskMode != 0) { //0x00001814
        if (ButtonMaskMode == 1) { //0x00001818 && 0x00001850
            curMask = curMask & ~mask; //0x00001854 && 0x0000186C
            curType = curType & ~mask; //0x00001874           
        }
        else if (ButtonMaskMode == 2) { //0x0000185C
                 curMask = curMask | mask; //0x00001868
                 curType = curType | mask; //0x00001820
        }
    }
    else {
         curMask = curMask & ~mask; //0x000017F8 && 0x0000181C
         curType = curType | mask; //0x00001820
    }
    ctrl.mask = curMask; //0x00001828
    ctrl.type = curType; //0x00001830    
    sceKernelCpuResumeIntr(suspendFlag); //0x0000182C
    return retVal;
}

/*
 * Subroutine sceCtrl_DA6B76A1 - Address 0x00001330 - Aliases: sceCtrl_driver_410833B7
 * Exported in sceCtrl
 * Exported in sceCtrl_driver
 */
int sceCtrlGetSamplingMode(int *mode) {
    int privMode;
    int index;
    
    privMode = pspSdkGetK1() << 11; //0x00001330
    index = ((privMode >> 31) < 1); //0x00001338 && 0x00001340
    if ((privMode & mode) >= 0) { //0x00001348 && 0x00001350
        *mode = ctrl.samplingMode[index]; //0x00001358 && 0x0000135C
    }
    return 0; 
}

/* Subroutine sceCtrl_1F4011E6 - Address 0x000012C8 - Aliases: sceCtrl_driver_6CB49301
 * Exported in sceCtrl
 * Exported in sceCtrl_driver
 * 
 * @param mode One of PspCtrlMode.
 * 
 * @return SCE_ERROR_INVALID_MODE if mode >= 2 or the previous sampling mode (one of PspCtrlMode).
 */
int sceCtrlSetSamplingMode(u8 mode) {    
    int suspendFlag;
    int privMode;
    u8 index;
    u8 oldMode;
    
    //0x000012D0 && 0x000012E4
    if (mode >= 2) {
        return SCE_ERROR_INVALID_MODE; 
    }
    suspendFlag = sceKernelCpuSuspendIntr(); //0x000012EC
    privMode = (pspSdkGetK1() >> 20) & 0x1; //0x000012F4
    index = (privMode < 1); //0x000012FC
    
    oldMode = ctrl.samplingMode[index]; //0x00001308
    ctrl.samplingMode[index] = mode; //0x0000130C
    
    sceKernelCpuResumeIntr(suspendFlag); //0x00001310:
    return oldMode; //0x00001318   
}

/*
 * Subroutine sceCtrl_02BAAD91 - Address 0x00001AB8 - Aliases: sceCtrl_driver_4E972A76
 * Exported in sceCtrl
 * Exported in sceCtrl_driver
 */
int sceCtrlGetSamplingCycle(int *cycle) {
    int privMode;
    
    privMode = pspSdkGetK1() << 11; //0x00001AB8
    
    if ((privMode & cycle) >= 0) { //0x00001ABC && 0x00001AC0
        *cycle = ctrl.btn_cycle; //0x00001ACC && 0x00001AD0
    }
    return 0;
}

/*
 * Subroutine sceCtrl_6A2774F3 - Address 0x0000136C - Aliases: sceCtrl_driver_855C255D
 * Exported in sceCtrl
 * Exported in sceCtrl_driver
 */
int sceCtrlSetSamplingCycle(int cycle) {
    int k1;
    int suspendFlag;
    int retVal;
    int sdkVersion;
    int nCycle;
    
    k1 = pspSdkGetK1();
    pspSdkSetK1(k1 << 11); //0x00001378
    
    suspendFlag = sceKernelCpuSuspendIntr(); //0x00001398
    if (cycle == 0) { //0x000013B0
        
        retVal = ctrl.btn_cycle; //0x00001460
        sceKernelEnableSubIntr(PSP_VBLANK_INT, 0x13); //0x00001464
        ctrl.btn_cycle = 0; //0x00001468
        sceSTimerSetHandler(ctrl.timerID, 0, NULL, 0); //0x00001478
        sceSTimerStopCount(ctrl.timerID); //0x00001480            
    }
    else {
        if (cycle < 0x15B3 || cycle > 0x4E21) { //0x000013B4 && 0x000013B8
            retVal = SCE_ERROR_INVALID_VALUE; //0x0000138C && 0x000013C0
        }
        else {
             retVal = ctrl.btn_cycle; //0x0000140C
             sceSTimerStartCount(ctrl.timerID); //0x00001410
             ctrl.btn_cycle = cycle; //0x00001414
             sdkVersion = sceKernelGetCompiledSdkVersion(); //0x00001418
        
             nCycle = ((0x204FFFF < sdkVersion) < 1); //0x00001428 && 0x0000142C
             nCycle = cycle + nCycle; //0x00001434 -- If the Sdk version is < 2.5, the original cycle is used, otherwise original cycle + 1
             sceSTimerSetHandler(ctrl.timerID, nCycle, tick_handler, 0); //0x00001408 && 0x00001444
             sceKernelDisableSubIntr(PSP_VBLANK_INT, 0x13); //0x00001450      
       }
    }
    sceKernelCpuResumeIntr(suspendFlag); //0x000013D8
    pspSdkSetK1(k1); //0x000013E4
    return retVal; //0x000013E0  
}

/*
 * Subroutine sceCtrl_3A622550 - Address 0x00001AE0 - Aliases: sceCtrl_driver_18654FC0
 * Exported in sceCtrl
 * Exported in sceCtrl_driver
 */
int sceCtrlPeekBufferPositive(SceCtrlData *pad, int count) {
    int retVal;
    
    retVal = _sceCtrlReadBuf(pad, count, 0, 0); //0x00001AEC
    return retVal;
}

/*
 * Subroutine sceCtrl_C152080A - Address 0x00001B00 - Aliases: sceCtrl_driver_02DD57CF
 * Exported in sceCtrl
 * Exported in sceCtrl_driver
 */
int sceCtrlPeekBufferNegative(SceCtrlData *pad, int count) {
    int retVal;
    
    retVal = _sceCtrlReadBuf(pad, count, 0, 1); //0x00001B0C
    return retVal;
}

/*
 * Subroutine sceCtrl_1F803938 - Address 0x00001B20 - Aliases: sceCtrl_driver_9F3038AC
 * Exported in sceCtrl
 * Exported in sceCtrl_driver
 */
int sceCtrlReadBufferPositive(SceCtrlData *pad, int count) {
    int retVal;
    
    retVal = _sceCtrlReadBuf(pad, count, 0, 2); //0x00001B2C
    return retVal;
}

/*
 * Subroutine sceCtrl_60B81F86 - Address 0x00001B40 - Aliases: sceCtrl_driver_EB5F1D7A
 * Exported in sceCtrl
 * Exported in sceCtrl_driver
 */
int sceCtrlReadBufferNegative(SceCtrlData *pad, int count) {
    int retVal;
    
    retVal = _sceCtrlReadBuf(pad, count, 0, 3); //0x00001B4C
    return retVal;
}

/*
 * Subroutine sceCtrl_5A36B1C2 - Address 0x00001B60 - Aliases: sceCtrl_driver_1D75C1D4
 * Exported in sceCtrl
 * Exported in sceCtrl_driver
 */
int sceCtrl_5A36B1C2(int arg1, SceCtrlDataExt *pad, int count) {
    int retVal;
    
    retVal = _sceCtrlReadBuf(pad, count, arg1, 4); //0x00001B78
    return retVal;
}

/*
 * Subroutine sceCtrl_239A6BA7 - Address 0x00001B8C - Aliases: sceCtrl_driver_6E552572
 * Exported in sceCtrl
 * Exported in sceCtrl_driver
 */
int sceCtrl_239A6BA7(int arg1, SceCtrlDataExt *pad, int count) {
    int retVal;
    
    retVal = _sceCtrlReadBuf(pad, count, arg1, 5); //0x00001BA4
    return retVal;
}

/*
 * Subroutine sceCtrl_1098030B - Address 0x00001BB8 - Aliases: sceCtrl_driver_16BB4085
 * Exported in sceCtrl
 * Exported in sceCtrl_driver
 */
int sceCtrl_1098030B(int arg1, SceCtrlDataExt *pad, int count) {
    int retVal;
    
    retVal = _sceCtrlReadBuf(pad, count, arg1, 6); //0x00001BD0
    return retVal;
}

/*
 * Subroutine sceCtrl_7C3675AB - Address 0x00001BE4 - Aliases: sceCtrl_driver_4870C6AF
 * Exported in sceCtrl
 * Exported in sceCtrl_driver
 */
int sceCtrl_7C3675AB(int arg1, SceCtrlDataExt *pad, int count) {
    int retVal;
    
    retVal = _sceCtrlReadBuf(pad, count, arg1, 7); //0x00001BFC
    return retVal;
}

/*
 * Subroutine sceCtrl_B1D0E5CD - Address 0x00001490 - Aliases: sceCtrl_driver_6574DC7C
 * Exported in sceCtrl
 * Exported in sceCtrl_driver
 */
int sceCtrlPeekLatch(SceCtrlLatch *latch) {
    int k1;
    int suspendFlag;
    SceCtrlLatch *latchPtr;
    
    k1 = pspSdkGetK1();
    pspSdkSetK1(k1 << 11); //0x0000149C  
    suspendFlag = sceKernelCpuSuspendIntr(); //0x000014A8
    
    if ((pspSdkGetK1() & latch) < 0) { //0x000014B8
        sceKernelCpuResumeIntr(suspendFlag); //0x00001524
        pspSdkSetK1(k1); //0x00001530
        return SCE_ERROR_PRIV_REQUIRED; //0x0000152C && 0x00001538 -- protect kernel address from usermode
    }
    if (pspSdkGetK1() >= 0) { //0x000014C4
        latchPtr = &ctrl.latch_kernel; //0x00001520
    }
    else {
         latchPtr = &ctrl.latch_user; //0x000014D0
    }
    latch->btnMake = latchPtr->btnMake; //0x000014D4
    latch->btnBreak = latchPtr->btnBreak; //0x000014DC
    latch->btnPress = latchPtr->btnPress; //0x000014E4
    latch->btnRelease = latchPtr->btnRelease; //0x000014EC
    
    sceKernelCpuResumeIntr(suspendFlag); //0x000014F4
    pspSdkSetK1(k1); //0x000014FC
    return *(latchPtr + 0x16); //0x00001500
}

/*
 * Subroutine sceCtrl_0B588501 - Address 0x0000153C - Aliases: sceCtrl_driver_D883CAF9
 * Exported in sceCtrl
 * Exported in sceCtrl_driver
 */
int sceCtrlReadLatch(SceCtrlLatch *latch) {
    int k1;
    int suspendFlag;
    SceCtrlLatch *latchPtr;
    int retVal;
    
    k1 = pspSdkGetK1(); //0x00001540
    pspSdkSetK1(k1 << 11); //0x00001548
    suspendFlag = sceKernelCpuSuspendIntr(); //0x00001554
    
    if ((pspSdkGetK1() & latch) < 0) { //0x00001564 -- protect kernel address from usermode     
        sceKernelCpuResumeIntr(suspendFlag); //0x00001620
        pspSdkSetK1(k1); //0x0000162C
        return SCE_ERROR_PRIV_REQUIRED; //0x00001634
    }
    if (pspSdkGetK1() >= 0) { //0x00001570
        latchPtr = &ctrl.latch_kernel; //0x000015E0       
        retVal = ctrl.latchData_kernel; //0x00001614
        ctrl.latchData_kernel = 0; //0x0000161C
    }
    else {
         latchPtr = &ctrl.latch_user; //0x0000157C
         retVal = ctrl.latchData_user; //0x000015A0
         ctrl.latchData_user = 0; //0x000015A8
    }
    latch->btnMake = latchPtr->btnMake; //0x00001584 || 0x000015E8
    latch->btnBreak = latchPtr->btnBreak; //0x0000158C || 0x000015F0
    latch->btnPress = latchPtr->btnPress; //0x00001594 || 0x000015F8
    latch->btnRelease = latchPtr->btnRelease; //0x0000159C || 0x00001600
    
    latchPtr->btnMake = 0; //0x000015A4 || 0x00001604
    latchPtr->btnBreak = 0; //0x000015AC || 0x00001608
    latchPtr->btnPress = 0; //0x000015B0 || 0x0000160C
    latchPtr->btnRelease = 0; //0x000015B4 || 0x00001610
    
    sceKernelCpuResumeIntr(suspendFlag); //0x000015B8
    pspSdkSetK1(k1); //0x000015C0
    return retVal; //0x000015C4
} 

/*
 * Subroutine sceCtrl_A7144800 - Address 0x00001680 - Aliases: sceCtrl_driver_84CEAE74
 * Exported in sceCtrl
 * Exported in sceCtrl_driver
 * 
 * original ASM code (idlereset check)
 */
/* int sceCtrlSetIdleCancelThreshold(int idlereset, int idleback) {
    int inValid_reset;
    int inValid_back;
    int cancelIdleTimer_reset;
    int cancelIdleTimer_back;
    int suspendFlag;
    
    
    inValid_reset = ((idlereset + 1) < 130); //0x00001688
    inValid_back = (idleback < -1); //0x0000168C
    
    inValid_reset = inValid_reset ^ 1; //0x00001698
    
    if ((inValid_back | inValid_reset) != 0) { //0x000016A0 && 0x000016B4
        return SCE_ERROR_INVALID_VALUE; //0x000016A8 && 0x000016AC &&0x00001700
    }
    cancelIdleTimer_reset = ~(idlereset | 0); //0x000016C0
    cancelIdleTimer_back = ~(idleback | 0); //0x000016C4
    
    if (idleback > 128) { //0x000016C8
        return SCE_ERROR_INVALID_VALUE; //0x000016A8 && 0x000016AC &&0x00001700
    }
    suspendFlag = sceKernelCpuSuspendIntr(); //0x000016D0
    
    ctrl.idleBack = (cancelIdleTimer_back == 0) ? 129 : idleback; //0x000016D4 && 0x000016E0
    ctrl.idleReset = (cancelIdleTimer_reset == 0) ? 129 : idlereset; //0x000016CC && 0x000016EC
    
    sceKernelCpuResumeIntr(suspendFlag); //0x000016E8
    return 0; //0x000016F0
} */

/*
 * Subroutine sceCtrl_A7144800 - Address 0x00001680 - Aliases: sceCtrl_driver_84CEAE74
 * Exported in sceCtrl
 * Exported in sceCtrl_driver
 */
int sceCtrlSetIdleCancelThreshold(int idlereset, int idleback) {
    int suspendFlag;
    
    if ((idlereset < -1 && idlereset > 128) && (idleback < -1 && idleback > 128)) { //0x000016A0 && 0x000016B4 && 0x000016C8
        return SCE_ERROR_INVALID_VALUE; //0x000016A8 && 0x000016AC && 0x00001700
    }       
    suspendFlag = sceKernelCpuSuspendIntr(); //0x000016D0
    
    ctrl.idleBack = (idleback == -1) ? 129 : idleback; //0x000016D4 && 0x000016E0
    ctrl.idleReset = (idlereset == -1) ? 129 : idlereset; //0x000016CC && 0x000016EC
    
    sceKernelCpuResumeIntr(suspendFlag); //0x000016E8
    return 0; //0x000016F0
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
    
    if (((idleReset & privMode) < 0) || ((privMode & idleBack) < 0)) { //0x00001734 && 0x00001740
        pspSdkSetK1(k1);
        return SCE_ERROR_PRIV_REQUIRED; /* protect kernel addresses from usermode */
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
 * Subroutine sceCtrl_6841BE1A - Address 0x000018DC - Aliases: sceCtrl_driver_E96A4D84
 * Exported in sceCtrl
 * Exported in sceCtrl_driver
 */
int sceCtrlSetRapidFire(int key, int arg2, int arg3, int arg4, char arg5, char arg6, char arg7) {
    int unk_1;
    int unk_2;
    int unk_3;
    int unk_4;
    int k1;
    int suspendFlag;
    
    unk_1 = arg5 | arg6; //0x000018E8
    unk_2 = unk_1 | arg7; //0x000018F4
    if (key > 15) { //0x000018FC && 0x0000193C
        return SCE_ERROR_INVALID_INDEX;
    }
    if ((arg2 -1) > 62) { //0x00001910 && 0x00001954
        return SCE_ERROR_INVALID_VALUE;
    }
    k1 = pspSdkGetK1(); //0x0000194C
    pspSdkSetK1(k1 << 11); // 0x00001960
    
    unk_3 = arg2 | arg3; //0x0000195C    
    unk_3 = unk_3 | arg4; //0x00001968
    if (k1 < 0) { //0x00001964
        unk_4 = ~ctrl.unk_3; //0x0000197C
        unk_4 = unk_4 | 0x20000; //0x00001980
        unk_4 = unk_4 & unk_3; //0x00001984
        if (unk_4 != 0) { //0x00001988
            pspSdkSetK1(k1); //0x00001A0C
            return SCE_ERROR_PRIV_REQUIRED;
        }
    }
    suspendFlag = sceKernelCpuSuspendIntr(); //0x00001990
    
    ctrl.rapidFire[key].button = arg2; //0x000019B4
    ctrl.rapidFire[key].unk_1 = arg3; //0x000019B8
    ctrl.rapidFire[key].unk_2 = arg4; //0x000019BC
    ctrl.rapidFire[key].unk_3 = arg7; //0x000019C0
    ctrl.rapidFire[key].unk_4 = arg5; //0x000019B0
    ctrl.unkAllocData[key][24] = 0; //0x000019C8
    ctrl.unkAllocData[key][25] = arg6; //0x000019A8
    
    sceKernelCpuResumeIntr(suspendFlag); //0x000019C4
    pspSdkSetK1(k1); //0x000019CC
    return 0;
}

/*
 * Subroutine sceCtrl_A68FD260 - Address 0x00001DA8 - Aliases: sceCtrl_driver_FAF675CB
 * Exported in sceCtrl
 * Exported in sceCtrl_driver
 */
int sceCtrlClearRapidFire(int key) {
    if (key > 15) { //0x00001DBC && 0x00001DC0
        return SCE_ERROR_INVALID_INDEX;
    }
    ctrl.rapidFire[key].button = 0; //0x00001DC8
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
 * Subroutine sceCtrl_driver_1CDEDDBC - Address 0x00001A50 
 * Exported in sceCtrl_driver
 */
int sceCtrlSuspend() {  
    int cycle;
    
    cycle = ctrl.btn_cycle; //0x00001A58
    if (cycle != 0) { //0x00001A68
        sceSTimerStopCount(ctrl.timerID); //0x00001A88
    }
    else {
         sceKernelDisableSubIntr(PSP_VBLANK_INT, 0x13); //0x00001A70 
    }
    return 0;       
}

/*
 * Subroutine sceCtrl_driver_65698764 - Address 0x000011F0 
 * Exported in sceCtrl_driver
 */
int sceCtrl_driver_65698764(u8 mode, int arg2, int arg3) {
    int unk_1;
    SceUID poolId;
    void *arg;
    UnknownType *buffer;    
    
    //0x0000122C
    if (mode > 2) {
        return SCE_ERROR_INVALID_VALUE;
    }
    unk_1 = ctrl.unk_array2[mode]; //0x00001238
    if (unk_1 == 0) { //0x00001254
        poolId = sceKernelCreateFpl("SceCtrlBuf", PSP_MEMORY_PARTITION_KERNEL, 0, 0x1800, 1, NULL); //0x00001294
        if (poolId < 0) { //0x000012A4
            return poolId;
        }
        sceKernelTryAllocateFpl(poolId, &arg); //0x000012AC
        buffer = (UnknownType *)arg; //0x000012B4
        ctrl.unkAllocData = buffer + 0xC00; //0x000012BC -- 0xC00 is the size of 64 * sizeof(sceCtrlData), where the size of sceCtrlData is 48.
        ctrl.ctrlDataBuf = (SceCtrl[64] *)buffer; //0x000012C4
    }
    ctrl.unk_array3[mode] = arg3; //0x00001264
    ctrl.unk_array2[mode] = arg2; //0x0000126C
    return 0;
}

/*
 * Subroutine sceCtrl_driver_196CF2F4 - Address 0x00001C30 
 * Exported in sceCtrl_driver
 * checked
 */
int sceCtrlSetPollingMode(char pollMode) {
    ctrl.pollMode = pollMode; //0x00001C3C
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
    ctrl.unk_Byte_4 = arg1; //0x00001AA4
    return 0;
}

/*
 * Subroutine sceCtrl_348D99D4 - Address 0x00001C40 - Aliases: sceCtrl_driver_53E67075
 * Exported in sceCtrl
 * Exported in sceCtrl_driver
 * checked
 */
int sceCtrlSetSuspendingExtraSamples(u16 arg1) {
    u16 unk_1;
    
    if (arg1 >= 301) { //0x00001C44
        return SCE_ERROR_INVALID_VALUE; //0x00001C48 && 0x00001C54
    }
    unk_1 = ((arg1 ^ 0x1) == 0) ? 0 : arg1; //0x00001C40 && 0x00001C4C
    ctrl.unk_0 = unk_1;
}

/*
 * Subroutine sceCtrl_AF5960F3 - Address 0x00001C6C - Aliases: sceCtrl_driver_525D27AC
 * Exported in sceCtrl
 * Exported in sceCtrl_driver
 * checked
 */
u16 sceCtrlGetSuspendingExtraSamples() {
    u16 retVal;
    
    retVal = ctrl.unk_0;  //0x00001C74
    return retVal;
}

/*
 * Subroutine sceCtrl_driver_094EF1BB - Address 0x00001C78 
 * Exported in sceCtrl_driver
 */
int sceCtrl_driver_094EF1BB(int index, int arg2, int arg3, int arg4) {
    
    if (index >= 4) { //0x00001C98
        return SCE_ERROR_INVALID_VALUE; //0x00001C90 && 0x00001C9C
    }
    ctrl.unk_struct[index].unk_4 = arg2; //0x00001CA8
    ctrl.unk_struct[index].unk_5 = arg3; //0x00001CAC
    ctrl.unk_struct[index].unk_6 = arg4; //0x00001CA0
    
    return 0;
}

/*
 * Subroutine sceCtrl_driver_A759DB6A - Address 0x00001CB8
 * Exported in sceCtrl_driver
 */
int sceCtrl_driver_A759DB6A(int index, int arg1, int arg2, int arg3) {
    
    if (index >= 4) { //0x00001CE4
        return SCE_ERROR_INVALID_VALUE; //0x00001CD0 && 0x00001CE8
    }
    arg1 &= 0xFF; //0x00001CDC
    arg2 &= 0xFF; //0x00001CE0
    
    ctrl.unk_struct[index].unk_1 = arg1; //0x00001CF8
    ctrl.unk_struct[index].unk_2 = arg2; //0x00001CEC
    ctrl.unk_struct[index].unk_3 = arg3; //0x00001CF4
    
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
 * Subroutine sub_00001E70 - Address 0x00001E70
 */
static int _sceCtrlReadBuf(void *pad, int count, int arg3, int mode) {
    SceCtrlLatch *latchPtr;
    int k1;
    int privMode;
    int offset;
    int index = arg3;
    int unk1;
    int res;
    SceCtrlDataExt ctrlBuf;
    int suspendFlag;
    
    
    if ((count -1) >= 64) { //0x00001E74 && 0x00001E84 && 0x00001EB4
        return SCE_ERROR_INVALID_SIZE;
    }
    if (arg3 >= 3) { //0x00001EC0 && 0x00001EC4
        return SCE_ERROR_INVALID_VALUE;
    }
    if ((arg3 != 0) && (mode & 2)) { //0x00001ECC && 0x00001ED8
        return SCE_ERROR_NOT_SUPPORTED; 
    }
    k1 = pspSdkGetK1();
    privMode = k1 << 11;
    pspSdkSetK1(privMode); //0x00001EF4
    
    if ((privMode & pad) < 0) { //0x00001EF0 -- protect kernel addresses from usermode
        pspSdkSetK1(k1);
        return SCE_ERROR_PRIV_REQUIRED;
    }
    if (privMode < 0) { //0x00001EFC -- user mode
        latchPtr = &ctrl.latch_user; //0x00001F00
    }
    else { //kernel mode
        latchPtr = &ctrl.latch_kernel; //0x00001F08
    }
    if (arg3 != 0) { //0x00001F0C
        offset = (arg3 << 2) + 28; //0x00001F14
        if (*(latchPtr + offset) == NULL) { //0x00001F18 && 0x00001F1C && 0x00001F20
            return SCE_ERROR_NOT_SUPPORTED;
        }
    }
    //waiting for the VSNYCH callback or your custom timer when using "read"
    if (mode & 2) { //0x00001F10 && 0x00001F28
        res = sceKernelWaitEventFlag(ctrl.timerID, 1, PSP_EVENT_WAITOR, NULL, NULL); //0x00001F44
        if (res < 0) { //0x00001F4C
            pspSdkSetK1(k1); //0x00001F50
            return res;
        }
        suspendFlag = sceKernelCpuSuspendIntr(); //0x00001F54
        sceKernelClearEventFlag(ctrl.timerID, 0xFFFFFFFE); //0x00001F64
        
        unk1 = ctrl.latch_kernel.unk2 //0x00001F70
        res = ctrl.latch_kernel.unk3 - unk1; //0x00001F74
        if (res < 0) { //0x00001F7C 
            res += 64; //0x00001F80
        }
        ctrl.latch_kernel.unk2 = ctrl.latch_kernel.unk3; //0x00001F8C
        if (res >= count) { //0x00001F88
            unk1 = ctrl.latch_kernel.unk3 - count; //0x00001F90
            unk1 = (unk1 < 0) ? unk1 + 64 : unk1; //0x00001F98 && 0x00001F9C
            res = count; //0x00001FA0
        }
    }
    else {
        suspendFlag = sceKernelCpuSuspendIntr(); //0x0000216C
        res = ctrl.latch_kernel.unk3; //0x00002174
        
        unk1 = res; //0x00002184
        if (count < 64) { //0x00002178
            unk1 = res - count; //0x0000218C
            unk1 = (unk1 < 0) ? unk1 + 64 : unk1; //0x00001F98 && 0x00001F9C
            res = count; //0x00001FA0
        }   
    }
    index = unk1;
        if (arg3 != 0) { //0x00001FA4          
            //res2 = ctrl.latch_kernel.unk4[arg3] + ((unk1 + (unk1 << 1)) << 4); //0x00002160 -- shift value == 48
            ctrlBuf = ctrl.latch_kernel.ctrlDataBuf[index]; //0x00002160 -- shift value == 48 (array element size == 48)
        }
        else {
            //res2 = ctrl.latch_kernel.unk4[0] + (unk1 << 4); //0x00001FB4 -- shift value == 16
            ctrlBuf = ctrl.latch_kernel.ctrlDataBuf[index]; //0x00001FB4 -- shift value == 16 (array element size == 16)
        }
        if (res < 0) { //0x00001FB8
            sceKernelCpuResumeIntr(suspendFlag); //0x000020A8
            pspSdkSetK1(k1); //0x000020B0
            return res;
        }
    
    while (count-- > 0) {
        ((SceCtrlData *)pad)->activeTime = ctrlBuf.activeTime; //0x00001FE4 && 0x00001FF0
        if (pspSdkGetK1() >> 31) { //0x00001FEC
            ctrlBuf.buttons &= ctrl.unk_3; //0x00001FF8
        }
        //here, the current active button value(s) will be turned negative, if this function is called through read/peekBufferNegative
        ((SceCtrlData *)pad)->buttons = (mode & 1) ? ~ctrlBuf.buttons : ctrlBuf.buttons; //0x00002004 && 0x00002004 && 0x0000200C
        ((SceCtrlData *)pad)->aX = ctrlBuf.aX; //0x00002010
        ((SceCtrlData *)pad)->aY = ctrlBuf.aY; //0x00002018
        
        if ((mode & 4) == 0) { //if (mode < 4) -- 0x00001FC8 && 0x00002014
            ((SceCtrlData *)pad)->rsrv[0] = 0; //0x00002130
            ((SceCtrlData *)pad)->rsrv[1] = 0; 
            ((SceCtrlData *)pad)->rsrv[2] = 0;
            ((SceCtrlData *)pad)->rsrv[3] = 0;
            ((SceCtrlData *)pad)->rsrv[4] = 0;
            ((SceCtrlData *)pad)->rsrv[5] = 0; //0x00002138
            pad = pad + 16; //0x00002140 -- size of sceCtrlData
        }
        else { //if (mode >= 4)
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
                ((SceCtrlDataExt *)pad)->rsrv[0] = ctrlBuf.rsrv[0]; //0x00002034
                ((SceCtrlDataExt *)pad)->rsrv[1] = ctrlBuf.rsrv[1]; //0x00002038
                ((SceCtrlDataExt *)pad)->rsrv[4] = ctrlBuf.rsrv[4]; //0x0000203C
                ((SceCtrlDataExt *)pad)->rsrv[5] = ctrlBuf.rsrv[5]; //""
                ((SceCtrlDataExt *)pad)->unk1 = ctrlBuf.unk1; //0x00002044
                ((SceCtrlDataExt *)pad)->unk2 = ctrlBuf.unk2; //...
                ((SceCtrlDataExt *)pad)->unk3 = ctrlBuf.unk3;
                ((SceCtrlDataExt *)pad)->unk4 = ctrlBuf.unk4;
                ((SceCtrlDataExt *)pad)->unk5 = ctrlBuf.unk5;
                ((SceCtrlDataExt *)pad)->unk6 = ctrlBuf.unk6;
                ((SceCtrlDataExt *)pad)->unk7 = ctrlBuf.unk7; //...
                ((SceCtrlDataExt *)pad)->unk8 = ctrlBuf.unk8; //0x0000207C              
            }
            pad = pad + 48; //0x00002080 -- size of SceCtrlDataExt
        }
        //index++; //0x00002084 && 0x00002088 && 0x00002090 && 0x00002098
        unk1++; //0x0000208C
        ctrlBuf = ctrl.latch_kernel[0].ctrlDataBuf[unk1];
        if (unk1 == 64) { //0x00002094
            unk1 = 0; //0x000020EC
            if (arg3 != 0) {
                ctrlBuf = ctrl.latch_kernel[0].ctrlDataBuf[unk1]; //0x000020F4
            }
            else {
                ctrlBuf = ctrl.latch_kernel[arg3].ctrlDataBuf; //0x00001FC4 && 0x000020FC
            }
        }
    }
    sceKernelCpuResumeIntr(suspendFlag); //0x000020A8
    pspSdkSetK1(k1); //0x000020B0
    return res;
}

/*
 * Subroutine sceCtrl_driver_8543BB3B - Address 0x000002AC 
 * Exported in sceCtrl_driver
 */
int sceCtrlResume() {
    int res;
    int unk_1;
    
    res = sceSyscon_driver_A59F82EB(); //0x000002B8
    if (res != 0) { //0x000002C4
        if (res == 1) { //0x00000344
            unk_1 = ctrl.unk_2; //0x00000350
            unk_1 |= 0x20000000; //0x00000358
            ctrl.unk_2 = unk_1; //0x00000360
        }
    }
    else {
         unk_1 = ctrl.unk_2; //0x000002CC
         unk_1 &= 0xDFFFFFFF; //0x000002D0
         ctrl.unk_2 = unk_1; //0x000002D4
    }
    ctrl.unk_Byte_1 = -1;
    if (ctrl.btn_cycle == 0) { //0x000002DC && 0x000002F0
        sceKernelEnableSubIntr(PSP_VBLANK_INT, 0x13); //0x000002F8
    }
    else {
         sceSTimerStartCount(ctrl.timerID); //0x00000318
         sceSTimerSetHandler(ctrl.timerID, ctrl.btn_cycle, tick_handler, 0); //0x00000330
    }
    return 0;
} 

/*
 * Subroutine sceCtrl_driver_08DE9E04 - Address 0x0000020C
 * Exported in sceCtrl_driver
 */
int sceCtrlEnd() {
    SceSysTimerId timerId;
    
    sceKernelUnregisterSysEventHandler(&eventHandler); //0x00000220
    sceSyscon_driver_C325BF4B(1); //0x00000228
    sceDisplayWaitVblankStart(); //0x00000230
    
    timerId = ctrl.timerID; //0x0000023C
    ctrl.timerID = -1; //0x00000244
    if (timerId >= 0) { //0x00000248
        sceSTimerStopCount(timerId); //0x00000250
        sceSTimerFree(timerId); //0x00000258
    }
    sceKernelReleaseSubIntrHandler(PSP_VBLANK_INT, 013); //0x00000264
    sceKernelDeleteEventFlag(ctrl.eventFlag); //0x0000026C
    
    while (ctrl.unk_0 < 0) { //0x0000028C
           sceDisplayWaitVblankStart(); //0x00000280
    }
    return 0; //0x000002A0
}

/*
 * Subroutine sceCtrl_driver_4FAA342D - Address 0x00000000
 * Exported in sceCtrl_driver
 */
int sceCtrlInit() {
    int eventId;
    int keyConfig;
    SceSysTimerId timerId;
    int unk_1;
    int unk_2;
    void (*func)(void *);
    UnknownType *retPtr;
    int pspModel;
    
    memset(ctrl, 0, sizeof(SceCtrl));  //0x00000024 -- SysclibForKernel_10F3BB61
    ctrl.pollMode = 1; //0x00000048
    ctrl.ctrlDataBuf = (SceCtrlData[64] *)0x00002BB0; //0x00000054 -- where sizeof SceCtrlData is 16 (default) - thus the required size is 0x400.
    ctrl.unk_Byte_6 = -128; //0x00000060
    ctrl.unk_Byte_5 = -128; //0x00000064
    ctrl.unkAllocData = 0x00002FB0; //0x0000006C
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
    sceKernelRegisterSysEventHandler(&eventHandler); //0x000000A4
    sceSyscon_driver_C325BF4B(0); //0x000000AC
    
    keyConfig = sceKernelInitKeyConfig(); //0x000000B4
    if (keyConfig == PSP_INIT_KEYCONFIG_UPDATER) { //0x000000C0
        unk_1 = 0x3FFFF; //0x00000208
    }
    if (keyConfig > 0x111) { //0x000000CC
        unk_1 = 0x3F3F9; //0x000000EC -- might be or'ed button values
        if (keyConfig == PSP_INIT_KEYCONFIG_POPS) { //0x000000E0
            unk_1 = 0x3FFFF; //0x00000208
        }
    }
    if (keyConfig == 0) { //0x000000D4
        unk_1 = 0x3F3F9; //0x000000EC
    }
    else { //0x000000DC
         unk_1 = 0x3F3F9; //0x000000EC
         if (keyConfig == PSP_INIT_KEYCONFIG_VSH) { //0x000000E0
             unk_1 = 0x3FFFF; //0x00000208
         }
    }
    ctrl.unk_3 = unk_1; //0x000000F0
    ctrl.type = 0; //0x000000FC
    ctrl.mask = unk_1; //0x00000104
    
    pspModel = sceKernelGetModel();
    if (pspModel < 10) { //0x0000010C
        switch (pspModel) { //0x00000128
            case 0: case 1: case 2: case 3: case 6: case 8:
                unk_2 = 0x1FFF3F9; //0x00000130 && 0x00000138               
                break;
            case 4: case 5: case 7: case 9:
                unk_2 = 0x39FFF3F9; //0x000001EC && 0x00000138
                break;
        }
        ctrl.unk_4 = unk_2; //0x0000013C
    }
    else { //0x000001F0
        ctrl.unk_4 = 0x1FFF3F9; //0x00000110 && 0x000001F0
    }    
    ctrl.unk_5 = 0x39FFF3F9; //0x00000168
    ctrl.unk_6 = 0xF1F3F9; //0x00000174
    ctrl.unk_7 = 0x390E0000; //0x0000017C
    ctrl.unk_8 = 0; //0x00000180
    ctrl.idleReset = 129; //0x00000188
    ctrl.idleBack = 129; //0x00000158
    
    sceKernelRegisterSubIntrHandler(PSP_VBLANK_INT, 0x13, vblank_handler, NULL); //0x00000184
    sceKernelEnableSubIntr(PSP_VBLANK_INT, 0x13); //0x00000190
    
    retPtr = KDebugForKernel_9975EF98(); /* function needs to be reversed */
    if ((retPtr != NULL) && (*retPtr == 0x30)) { //0x000001A0 && 0x000001AC
         func = *(retPtr + 0x2C); //0x000001B0
         func(0x000027F8); //0x000001D8            
    }
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