/* Copyright (C) 2011, 2012, 2013 The uOFW team    
   See the file COPYING for copying permission.
*/

/*
 * uOFW/trunk/src/led/led.c
 * 
 * sceLED_Service - a driver for the PSP's hardware LEDs.
 * 
 * The LED driver's main function is to supply users with a precise
 * functionality of changing LED states.  It provides functions to 
 * turn a LED ON/OFF, to setup a blink event for LEDs as well as to
 * register 4 LED functions which can be manually  executed.  In contrast,
 * SYSCON's sceSysconCtrlLed() function only allows the user to turn a LED
 * ON/OFF.
 * 
 * These are the following LEDs which states are controlled by this driver:
 *      - the PSP's W-LAN LED
 *      - the PSP's Memory-Stick LED
 *      - the PSP's Bluetooth LED (only for the PSP_GO and similar
 *                                 PSP hardware models.)
 * 
 * LED state interaction is done every VBlank interrupt, thus approximately
 * 60 times per second.
 * 
 */

#include <common_imp.h>
#include <interruptman.h>
#include <led.h>
#include <lowio_gpio.h>
#include <syscon.h>
#include <sysmem_kernel.h>
#include <sysmem_sysclib.h>
#include <sysmem_sysevent.h>

SCE_MODULE_INFO("sceLED_Service", SCE_MODULE_KERNEL | SCE_MODULE_ATTR_CANT_STOP | SCE_MODULE_ATTR_EXCLUSIVE_LOAD | 
                                  SCE_MODULE_ATTR_EXCLUSIVE_START, 1, 8);
SCE_MODULE_BOOTSTART("_sceLedModuleStart");
SCE_MODULE_REBOOT_BEFORE("_sceLedModuleRebootBefore");
SCE_SDK_VERSION(SDK_VERSION);

/* Highest possible LED mode. */
#define LED_MODE_MAX    (3)

#define LED_CONFIG_SLOTS (4)

/* 
 * Non GO-PSP models have two internal LED classes, 
 * W-LAN LED and Memory-Stick LED.
 */
#define NUM_LED_NON_GO_MODEL    (2)
/* 
 * GO-PSP models have three internal LED classes, 
 * W-LAN LED, Memory-Stick LED and Bluetooth LED.
 */
#define NUM_LED_GO_MODEL        (3)

/* LED display modes. */
enum SceLedDisplayModes {
    /* 
     * The specific LED is on when the corresponding 
     * hardware element is active. 
     */
    LED_ON_ACTIVITY_ON = 0,
    /* 
     * The specific LED is on when the corresponding 
     * hardware element is inactive. 
     */
    LED_ON_ACTIVITY_OFF = 1,
};

typedef struct {
    /* The address of a LED configuration command package. */
    u32 customLedCmd;
    /* The number of executions of a registered LED configuration command. */
    u32 numExecs;
} SceLedControlCmd;

typedef struct {
    /* The GPIO port belonging to the specific LED. */
    u8  gpioPort;
    /* The display mode of an LED. One of ::SceLedDisplayModes. */
    u8  displayMode;
    /* The LED ID. One of ::PspSysconLeds. */
    u8  ledId;
    /* Reserved. */
    u8  padding;
    /* The activity mode of a LED. */
    s32 activityMode;
    /* The blink time interval of a LED. */
    s32 blinkInterval;
    /* The "on" time of a LED during a blink period. */
    s32 onTime;
    /* The "off" time of a LED during a blink period. */
    s32 offTime;
    /* 
     * The duration of a blink period. After the blink period, 
     * the endBlinkState is applied to the specified LED. 
     */
    s32 blinkTime;
    /* The end mode of an LED after the blink time interval. 1 = ON, 0 = OFF. */
    s32 endBlinkState;
    /** The current LED mode. One of ::SceLedModes. */
    s32 curLedMode;
    /* Unknown. */
    s32 unk32;
    /* The current LED configuration command. */
    SceLedConfiguration *config;
    /* The current saved LED configuration command. */
    SceLedConfiguration *savedConfig;
    /* The index into the registered LED configurations. */
    s32 index;
    /* The LED configuration slots. */
    SceLedControlCmd controlCmds[LED_CONFIG_SLOTS];
} SceLed;

static s32 _sceLedSysEventHandler(s32 eventId, char *eventName __attribute__((unused)), void *param, 
                                  s32 *result __attribute__((unused)));
static s32 _sceLedVblankInterrupt(s32 subIntNm __attribute__((unused)), void *arg __attribute__((unused)));
static u32 _sceLedSeqExec(SceLed *led);
static u32 _sceLedSuspend(s32 arg0);
static u32 _sceLedResume(s32 arg0);

SceSysEventHandler g_LedSysEv = {
    .size = sizeof(SceSysEventHandler),
    .name = "SceLed",
    .typeMask = SCE_SUSPEND_EVENTS | SCE_RESUME_EVENTS,
    .handler = _sceLedSysEventHandler,
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
};

SceLed g_Led[NUM_LED_GO_MODEL];
u32 g_iMaxLeds;

u32 sceLedInit(void)
{    
    u32 i;
    s32 model;
    u8 btStatus;
    s32 gpioPort;
    u32 val;
    
    memset(&g_Led, 0, sizeof g_Led);
    g_iMaxLeds = NUM_LED_NON_GO_MODEL;
    
    g_Led[PSP_LED_TYPE_MS].gpioPort = SCE_GPIO_PORT_MODE_MS;
    g_Led[PSP_LED_TYPE_MS].ledId = PSP_SYSCON_LED_MS;
    g_Led[PSP_LED_TYPE_WLAN].gpioPort = SCE_GPIO_PORT_MODE_WLAN;
    g_Led[PSP_LED_TYPE_WLAN].ledId = PSP_SYSCON_LED_WLAN;
    
    model = sceKernelGetModel();  
    if (model == PSP_2000 || model == PSP_3000 || model == PSP_GO)
        g_Led[PSP_LED_TYPE_WLAN].displayMode = LED_ON_ACTIVITY_OFF;
    
    if (model == PSP_GO || model == 5 || model == 7 || model == 9) {
        g_Led[PSP_LED_TYPE_BT].gpioPort = SCE_GPIO_PORT_MODE_BT;
        g_Led[PSP_LED_TYPE_BT].ledId = PSP_SYSCON_LED_BT;
        if (model == PSP_GO)
            g_Led[PSP_LED_TYPE_BT].displayMode = LED_ON_ACTIVITY_OFF;
        
        g_iMaxLeds = NUM_LED_GO_MODEL;
    }
    
    for (i = 0; i < g_iMaxLeds; i++) {
        if (sceGpioGetPortMode(g_Led[i].gpioPort) == 2) {
            sceGpioSetPortMode(g_Led[i].gpioPort, 0);
            if (g_Led[i].displayMode == LED_ON_ACTIVITY_ON) {
                sceGpioPortClear(1 << g_Led[i].gpioPort);
                continue;
            }
        }
        else if (i == PSP_LED_TYPE_BT) {
            gpioPort = sceGpioPortRead();
            btStatus = sceSysconGetBtPowerStatus();      
            val = ((gpioPort >> (g_Led[i].gpioPort & 0x1F)) & 0x1) ^ g_Led[i].displayMode;
            val = (btStatus == 0) ? 0 : val;
                
            if (val != 0) {
                sceLedSetMode(PSP_LED_TYPE_BT, SCE_LED_MODE_ON, NULL);
                continue;
            }
            else if (g_Led[i].displayMode == LED_ON_ACTIVITY_ON) {
                sceGpioPortClear(1 << g_Led[i].gpioPort);
                continue;
            }
        }
        else if (g_Led[i].displayMode == LED_ON_ACTIVITY_ON) {
            sceGpioPortClear(1 << g_Led[i].gpioPort);
            continue;                    
        }           
        /*
         * Enable the following LEDs:
         *      1) the Memory-Stick LED
         *      2) the W-LAN LED
         *      3) the Bluetooth LED
         */       
        sceGpioPortSet(1 << g_Led[i].gpioPort);
    }
    
    /* Turn ON all PSP hardware LEDs. */
    for (i = 0; i < g_iMaxLeds; i++)
        sceSysconCtrlLED(g_Led[i].ledId, SCE_LED_MODE_ON);
    
    sceKernelRegisterSubIntrHandler(SCE_VBLANK_INT, 0x19, _sceLedVblankInterrupt, NULL);
    sceKernelEnableSubIntr(SCE_VBLANK_INT, 0x19);
    sceKernelRegisterSysEventHandler(&g_LedSysEv);
    
    return SCE_ERROR_OK;
}

u32 sceLedEnd(void)
{
    u32  i;
    
    sceKernelUnregisterSysEventHandler(&g_LedSysEv);
    sceKernelReleaseSubIntrHandler(SCE_VBLANK_INT, 0x19);  
    
    /* Disable the PSP hardware LEDs. */
    for (i = 0; i < g_iMaxLeds; i++) {
        if (i == PSP_LED_TYPE_BT) {
            if (g_Led[i].activityMode == SCE_LED_MODE_ON)
                continue;
            else if (g_Led[i].displayMode == LED_ON_ACTIVITY_ON) {
                sceGpioPortClear(1 << g_Led[i].gpioPort);
                continue;
            }
        } else if (g_Led[i].displayMode == LED_ON_ACTIVITY_ON) {
            sceGpioPortClear(1 << g_Led[i].gpioPort);
            continue;
        }       
        sceGpioPortSet(1 << g_Led[i].gpioPort);
    }
    return SCE_ERROR_OK;
}

/*
 * Suspend/resume the LED library according to hardware commands.
 */
static s32 _sceLedSysEventHandler(s32 eventId, char *eventName __attribute__((unused)), void *param, 
                                  s32 *result __attribute__((unused)))
{
    u32 *ptr = NULL;
    
    if (eventId == 0x400A) {
        if (((u32 *)param)[1] == 1)
            _sceLedSuspend(1);
        else
            _sceLedSuspend(0);
    } else if (eventId == 0x100000) {
        ptr = (u32 *)((u32 *)param)[1];
        if (ptr[1] == 0x80000000)
            _sceLedResume(1);
        else
            _sceLedResume(0);
    }
    return SCE_ERROR_OK;
}

/*
 * This function is called every VBlank interrupt, approximately 60 times per second.
 * It is responsible for handling a LED blink event as well as for executing a custom
 * list of LED state functions.
 */
static s32 _sceLedVblankInterrupt(s32 subIntNm __attribute__((unused)), void *arg __attribute__((unused)))
{
    u32 i;
      
    for (i = 0; i < g_iMaxLeds; i++) {
        if (g_Led[i].activityMode == SCE_LED_MODE_SELECTIVE_EXEC) 
            _sceLedSeqExec(&g_Led[i]);
    }
    for (i = 0; i < g_iMaxLeds; i++) {      
       if (g_Led[i].blinkTime <= 0) {
           /* Finish a LED blink event. */
            if (g_Led[i].blinkTime == 0) {
                g_Led[i].blinkInterval = 0;
                g_Led[i].onTime = 0;
                g_Led[i].offTime = 0;
                
                /* Set the LED endBlinkState. */
                if (g_Led[i].endBlinkState != SCE_LED_MODE_OFF)
                    g_Led[i].onTime = 1;
                else
                    g_Led[i].offTime = 1;
            }
       } else {
           g_Led[i].blinkTime--;
       }       
        
       /* Count the LED blink interval time. */
       if ((g_Led[i].blinkInterval + 1) < (g_Led[i].onTime + g_Led[i].offTime))
           g_Led[i].blinkInterval++;
       else
           g_Led[i].blinkInterval = 0;
          
       /* 
        * Set the LEC status depending on the current blink event part(LED ON/OFF) 
        * and the LEDs display mode.
        */
       if (((g_Led[i].blinkInterval < g_Led[i].onTime) && (g_Led[i].displayMode == LED_ON_ACTIVITY_OFF)) 
               || ((g_Led[i].blinkInterval >= g_Led[i].onTime) && (g_Led[i].displayMode == LED_ON_ACTIVITY_ON)))
           /* Disable a LED. */
           sceGpioPortClear(1 << g_Led[i].gpioPort);
       else
           /* Enable a LED. */
          sceGpioPortSet(1 << g_Led[i].gpioPort);
         
    } 
    return -1;
}

/*
 * Execute registered LED configuration commands in sequence.
 */
static u32 _sceLedSeqExec(SceLed *led)
{
    u8 cmd;
    
    if (led->unk32 > 0 && --led->unk32 > 0)
        return SCE_ERROR_OK;

    u8 *ptr = (u8 *)led->config;
    while (ptr != NULL) {
        cmd = *ptr++;
        if ((s8)cmd < 0) {
            led->unk32 = cmd & 0x7F;
            led->config = (SceLedConfiguration *)ptr;
            return SCE_ERROR_OK;
        }
        switch(cmd) {
        case 0:
            led->config = NULL;
            return SCE_ERROR_OK;
        case SCE_LED_CMD_SAVE_CMD:
            led->savedConfig = (SceLedConfiguration *)ptr;
            break;   
        case SCE_LED_CMD_EXECUTE_SAVED_CMD:
            ptr = (u8 *)led->savedConfig;
            break;
        case SCE_LED_CMD_REGISTER_CMD:
            if (led->index >= LED_CONFIG_SLOTS) {
                led->config = NULL;
                return SCE_ERROR_OK;
            }           
            led->controlCmds[led->index].numExecs = *ptr++;
            led->controlCmds[led->index].customLedCmd = (u32)ptr;
            led->index++;   
            break;   
        case SCE_LED_CMD_EXECUTE_CMD:
            if (led->index <= 0) {
                led->config = NULL;
                return SCE_ERROR_OK;
            }           
            led->controlCmds[led->index - 1].numExecs--;
            if (led->controlCmds[led->index - 1].numExecs <= 0)
                led->index--;
            else 
                ptr = (u8 *)led->controlCmds[led->index - 1].customLedCmd;
            break;
        case SCE_LED_CMD_TURN_LED_ON:
            led->onTime = 1;
            led->curLedMode = SCE_LED_MODE_ON;
            led->offTime = 0;
            break;   
        case SCE_LED_CMD_TURN_LED_OFF:
            led->offTime = 1;
            led->curLedMode = SCE_LED_MODE_OFF;
            led->onTime = 0;
            break;
        case SCE_LED_CMD_SWITCH_LED_STATE:
            if (led->curLedMode == SCE_LED_MODE_ON) {
                led->onTime = 1;
                led->curLedMode = SCE_LED_MODE_OFF;
                led->offTime = 0;
            } else {
                led->offTime = 1;
                led->curLedMode = SCE_LED_MODE_ON;
                led->onTime = 0;
            }
            break;   
        case SCE_LED_CMD_BLINK_LED:          
            led->curLedMode = SCE_LED_MODE_BLINK;
            led->onTime = *ptr++;
            led->offTime = *ptr++;
            break;   
        default:
            led->config = NULL;
            return SCE_ERROR_OK;
        }       
    }
    led->config = (SceLedConfiguration *)ptr;
    return SCE_ERROR_OK;
}

s32 sceLedSetMode(s32 led, s32 mode, SceLedConfiguration *config)
{
    s32 oldMode;
    
    if (led < 0 || led >= (s32)g_iMaxLeds)
        return SCE_ERROR_INVALID_INDEX;
    
    if (mode < 0 || mode > LED_MODE_MAX)
        return SCE_ERROR_INVALID_MODE;
    
    g_Led[led].blinkTime = -1;
    g_Led[led].config = NULL;
    oldMode = g_Led[led].activityMode;
    g_Led[led].activityMode = mode;
    
    /* Turn a LED ON. */
    if (mode == SCE_LED_MODE_ON) {
        g_Led[led].onTime = 1;
        g_Led[led].offTime = 0;
        g_Led[led].blinkInterval = 0;
    } else if (mode == SCE_LED_MODE_OFF) {
        if (mode != SCE_LED_MODE_OFF)
            return SCE_ERROR_OK;
        
        /* Turn a LED OFF. */
        g_Led[led].offTime = 1;
        g_Led[led].blinkInterval = 0;
        g_Led[led].onTime = 0;
    } else if (mode == SCE_LED_MODE_BLINK) {
        if (oldMode != mode)
            g_Led[led].blinkInterval = 0;
            
        //NOTE: This is uOFW's own code, Sony forgot the null-pointer check
        //      in the original module.
        if (config == NULL)
            return SCE_ERROR_INVALID_POINTER;
        
        /* Setup a LED blink event. */
        g_Led[led].onTime = config->onTime;
        g_Led[led].offTime = config->offTime;
        g_Led[led].blinkTime = config->blinkTime;
        g_Led[led].endBlinkState = config->endBlinkState;
    } else if (mode == SCE_LED_MODE_SELECTIVE_EXEC) {
        if (oldMode != mode)
            g_Led[led].curLedMode = oldMode;
        
        g_Led[led].unk32 = 1;
        g_Led[led].config = config;
        //g_Led[led].index = 0; // <-- Sony code which does not make much sense here.
        g_Led[led].blinkInterval = 0;
        g_Led[led].savedConfig = NULL;
    }  
    return SCE_ERROR_OK;
}

s32 _sceLedModuleStart(SceSize argSize __attribute__((unused)), const void *argBlock __attribute__((unused)))
{
    sceLedInit();
    return SCE_ERROR_OK;
}

s32 _sceLedModuleRebootBefore()
{
    sceLedEnd();
    return SCE_ERROR_OK;
}

/*
 * Turn off all used LEDs and save their state. The LED states 
 * can be restored using _sceLedResume.
 */
static u32 _sceLedSuspend(s32 mode)
{
    u32 i;
    s32 status;
    
    sceKernelDisableSubIntr(SCE_VBLANK_INT, 0x19);
    
    if (mode == 0) {
        sceGpioPortClear(SCE_GPIO_MASK_LED_MS);
        sceGpioSetPortMode(SCE_GPIO_PORT_MODE_MS, 2);
        sceSysconCtrlLED(PSP_SYSCON_LED_WLAN, SCE_LED_MODE_OFF);
        
        status = sceSysconGetWlanPowerCtrl();       
        if (status == 0) {
            sceGpioPortClear(SCE_GPIO_MASK_LED_WLAN);
            sceGpioSetPortMode(SCE_GPIO_PORT_MODE_MS, 2); //Note: Shouldn't it be SCE_GPIO_PORT_MODE_WLAN?
        } else {
            if (g_Led[PSP_LED_TYPE_WLAN].displayMode == LED_ON_ACTIVITY_OFF)
                sceGpioPortClear(1 << g_Led[PSP_LED_TYPE_WLAN].gpioPort);
            else
                sceGpioPortSet(1 << g_Led[PSP_LED_TYPE_WLAN].gpioPort);
        }
        
        if (g_Led[PSP_LED_TYPE_BT].ledId == PSP_SYSCON_LED_MS)
            return SCE_ERROR_OK;
            
        sceSysconCtrlLED(PSP_SYSCON_LED_BT, SCE_LED_MODE_OFF);
        
        status = sceSysconGetBtPowerCtrl();        
        if (status == 0) {
            sceGpioPortClear(SCE_GPIO_MASK_LED_BT);
            sceGpioSetPortMode(SCE_GPIO_PORT_MODE_BT, 2);     
            return SCE_ERROR_OK;
        }
        
        if (g_Led[PSP_LED_TYPE_BT].displayMode == LED_ON_ACTIVITY_OFF) {
            sceGpioPortClear(1 << g_Led[PSP_LED_TYPE_BT].gpioPort);
            return SCE_ERROR_OK;
        }      
        sceGpioPortSet(1 << g_Led[PSP_LED_TYPE_BT].gpioPort);
        return SCE_ERROR_OK;
    }
    else {  
        for (i = 0; i < g_iMaxLeds; i++) {
            if (i == PSP_LED_TYPE_BT) {
                if (g_Led[i].activityMode == SCE_LED_MODE_ON)
                    continue;
                
                if (g_Led[i].displayMode == LED_ON_ACTIVITY_ON) {
                    sceGpioPortClear(1 << g_Led[i].gpioPort);
                    continue;
                }
            } else if (g_Led[i].displayMode == LED_ON_ACTIVITY_ON) {
                sceGpioPortClear(1 << g_Led[i].gpioPort);
                continue;
            }
            sceGpioPortSet(1 << g_Led[i].gpioPort);
        }
    }
    return SCE_ERROR_OK;
}

/*
 * Recover the LED states before the LEDs were suspended.
 */
static u32 _sceLedResume(s32 arg0)
{
    u32 i;
    u32 val;
    s32 gpioPort;
    u8 btStatus;
    
    for (i = 0; i < g_iMaxLeds; i++) {
        if (arg0 != 0 && i == PSP_LED_TYPE_BT) { 
            gpioPort = sceGpioPortRead();
            btStatus = sceSysconGetBtPowerStatus();            
            val = ((gpioPort >> (g_Led[i].gpioPort & 0x1F)) & 0x1) ^ g_Led[i].displayMode;
            val = (btStatus == 0) ? 0 : val;
                
            if (val != 0)
                sceLedSetMode(PSP_LED_TYPE_BT, SCE_LED_MODE_ON, NULL);
            else
                sceLedSetMode(PSP_LED_TYPE_BT, SCE_LED_MODE_OFF, NULL);
                
            sceGpioSetPortMode(g_Led[i].gpioPort, 0);
            sceSysconCtrlLED(g_Led[i].ledId, SCE_LED_MODE_ON);          
            continue;
        }     
        if (g_Led[i].displayMode == LED_ON_ACTIVITY_ON)
            sceGpioPortClear(1 << g_Led[i].gpioPort);
        else
            sceGpioPortSet(1 << g_Led[i].gpioPort);
        
        sceGpioSetPortMode(g_Led[i].gpioPort, 0);
        sceSysconCtrlLED(g_Led[i].ledId, SCE_LED_MODE_ON);
    }
    sceKernelEnableSubIntr(SCE_VBLANK_INT, 0x19);
    return SCE_ERROR_OK;
}
