/* 
 * ctrl.h
 *    Controller settings of the SCE PSP system.
 * Author: _Felix_
 * Version: 6.39
 */

#ifndef CTRL_H
#define	CTRL_H

#include <stdio.h>
#include <stdlib.h>
#include <pspsysevent.h>
#include <pspsystimer.h>
#include <pspsysmem.h>
#include <pspinit.h>
#include <psppower.h>
#include "../errors.h"

/** Status attributes of a button. Each struct member represents an active button
 *  through its button value (one of ::PspCtrlButtons).
 */
typedef struct _SceCtrlLatch {
    /** Button is newly pressed (was not already been pressed). */
    u32 btnMake; //0
    /** Stop of button press. */
    u32 btnBreak; //4
    /** Button is pressed. */
    u32 btnPress; //8
    /** Button is not pressed. */
    u32 btnRelease; //12
} SceCtrlLatch; //sizeof SceCtrlLatch: 16

/** Required information for a button callback. (Unique to each registered button callback.) */
typedef struct _SceCtrlButtonCallback {
    /** Or'ed button values (of ::PspCtrlButtons) which will be checked for being pressed. */
    u32 btnMask; //0
    /** Pointer to a callback function handling the button input effects. */
    void (*callbackFunc)(int, int, void *); //4
    /** The global pointer ($gp) value of the controller module. */
    u32 gp; //8
    /** An optional pointer, which is passed as the third argument to the callback function. */
    void *arg; //12    
} SceCtrlButtonCallback; //sizeof ButtonCallback: 16

typedef struct _SceCtrlLatchInternal {
    /** Button is newly pressed (was not already been pressed). */
    unsigned int uiMake; //0
    /** Stop of button press. */
    unsigned int uiBreak; //4
    /** Button is pressed. */
    unsigned int uiPress; //8
    /** Button is not pressed. */
    unsigned int uiRelease; //12
    int unk1; //16
    int unk2; //20
    int unk3; //24
    int *unk4[3]; //28
} SceCtrlLatchInternal; //sizeof SceCtrlLatch: 16

/**
 * Enumeration for the digital controller buttons.
 *
 * @note PSP_CTRL_HOME, PSP_CTRL_WLAN_UP, PSP_CTRL_REMOTE, PSP_CTRL_SCREEN, PSP_CTRL_NOTE, PSP_CTRL_VOLUP, PSP_CTRL_VOLDOWN, PSP_CTRL_DISC, 
 *       PSP_CTRL_MS can only be read in kernel mode.
 */
enum PspCtrlButtons {
    /** Select button. */
    PSP_CTRL_SELECT     = 0x1,
    /** Start button. */
    PSP_CTRL_START      = 0x8,
    /** Up D-Pad button. */
    PSP_CTRL_UP         = 0x10,
    /** Right D-Pad button. */
    PSP_CTRL_RIGHT      = 0x20,
    /** Down D-Pad button. */
    PSP_CTRL_DOWN       = 0x40,
    /** Left D-Pad button. */
    PSP_CTRL_LEFT       = 0x80,
    /** Left trigger. */
    PSP_CTRL_LTRIGGER   = 0x100,
    /** Right trigger. */
    PSP_CTRL_RTRIGGER   = 0x200,
    /** Triangle button. */
    PSP_CTRL_TRIANGLE   = 0x1000,
    /** Circle button. */
    PSP_CTRL_CIRCLE     = 0x2000,
    /** Cross button. */
    PSP_CTRL_CROSS      = 0x4000,
    /** Square button. */
    PSP_CTRL_SQUARE     = 0x8000,
    /** Home button. In user mode this bit is set if the exit dialog is visible. */
    PSP_CTRL_HOME       = 0x10000,
    /** Hold button. */
    PSP_CTRL_HOLD       = 0x20000,
    /** Wlan switch up. */
    PSP_CTRL_WLAN_UP    = 0x40000,
    /** Remote hold position. */
    PSP_CTRL_REMOTE     = 0x80000,
    /** Screen button. */
    PSP_CTRL_SCREEN     = 0x400000,
    /** Music Note button. */
    PSP_CTRL_NOTE       = 0x800000,   
    /** Volume up button. */
    PSP_CTRL_VOLUP      = 0x100000,
    /** Volume down button. */
    PSP_CTRL_VOLDOWN    = 0x200000,	
    /** Disc present. */
    PSP_CTRL_DISC       = 0x1000000,
    /** Memory stick present. */
    PSP_CTRL_MS         = 0x2000000,
};

/** Controller input modes. */
enum PspCtrlInputMode {
    /** Digitial input only. No recognizing of analog input. */
    PSP_CTRL_INPUT_NO_ANALOG = 0,         
    /** Recognizing of both digital- and analog input. */
    PSP_CTRL_INPUT_ANALOG = 1,
};

/** Controller (input) poll modes. */
enum PspCtrlPollMode {
    /** No controller input is recognized. */
    PSP_CTRL_POLL_NO_POLLING = 0,
    /** Controller input is recognized. */
    PSP_CTRL_POLL_POLLING = 1,
};

/** Button mask settings. */
enum PspCtrlMaskMode {
    /** Normal button behaviour. No masking. */
    PSP_CTRL_MASK_NO_SETTING = 0,
    /** Mask the defined button bit mask. Every button value "included" in a set button bit mask will be masked (blocked). */
    PSP_CTRL_MASK_BUTTON_MASK = 1,
    /** Should return a button constantly, but seems to mask it only. Implementation error? */
    PSP_CTRL_MASK_BUTTON_SET = 2,
};

/**
 * Register a button callback.
 * 
 * @param slot - The slot used to register the callback (0-3). Although Sony uses atleast slot 0 and slot 1 of the possible 
 *               4 callback slots in game mode, we can use them all freely as we wish.
 * @param btnMask - Or'ed button values which will be checked for being pressed.
 * @param cb - Pointer to the callback function (int curr, int last, void *arg), which handles button input effects.
 * @param arg - Optional user argument. Passed to the callback function.
 * 
 * @return 0 on success or < 0, if slot is another value than one of 0-3.
 */
int sceCtrlRegisterButtonCallback(int slot, u32 btnMask, void (*cb)(int, int, void *), void *arg);

/**
 * Get the set button mask mode of a button value.
 * 
 * @param btnMask - The button bit value to check for (one or more buttons of ::PspCtrlButtons).
 * 
 * @return 0 for nothing has been set for this button bit value. Returns 1, if the button bit value is included
 *         in a button bit value which is masked. Returns 2 in the case of the button bit value being set as "button set".
 */
int sceCtrlGetButtonMask(u32 btnMask);

/**
 * Set a button mask mode for one or more buttons. You can only mask user-mode buttons in user applications.
 * Masking of kernel-mode buttons is ignored as well as buttons usedin kernel-mode applications.
 * 
 * @param btnMask - The button bit value for which the button mask mode will be applied. 
 *                  One or more buttons of ::PspCtrlButtons.
 * @param btnMaskMode - The mask mode. 0 for no masking, 1 for masking, 2 for button setting.
 *                      Note: Set to 2 will only mask button(s), this seems to be an implementation error.  
 * 
 * @return 0, 1 for setting a complete new button bit mask (which is not included in a previously set button bit mask).               
 */
int sceCtrlSetButtonMask(u32 btnMask, u8 btnMaskMode);

/**
 * Get the current controller-input mode.
 * 
 * @param mode Pointer to int receiving the current controller mode.
 * 
 * @return 0.
 */
int sceCtrlGetSamplingMode(int *mode);

/**
 * Set the controller-input mode.
 * 
 * @param mode - The new controller-input mode. One of ::PspCtrlInputMode.
 * 
 * @return The previous input mode on success, or < 0, if mode is not a value of ::PspCtrlInputMode.
 */
int sceCtrlSetSamplingMode(u8 mode);

/**
 * Get the current cycle specifying the update frequency of the internal ctrl buffer.
 * 
 * @param cycle - The current cycle.
 * 
 * @return 0.
 */
int sceCtrlGetSamplingCycle(int *cycle);

/**
 * Set the update frequency of the internal ctrl pad buffer (i.e. SceCtrlData (internal), SceCtrlLatch (internal)).
 * 
 * @param cycle - The new time period between two samplings of controller attributes in microseconds.
 *                Setting to 0 triggers sampling at every VSYNC-event (60 updates/second). If you want to set an own
 *                time period for updating the internal ctrl pad buffer, cycle has to be > 5554 and < 20001.
 * 
 * @return The previous cycle on success, or < 0, if cycle is smaller than 5555 (excluding 0) and greater than 20000.
 */
int sceCtrlSetSamplingCycle(int cycle);

/**
 * Get the idle threshold values.
 *
 * @param idlerest - Movement needed by the analog to reset the idle timer.
 * @param idleback - Movement needed by the analog to bring the PSP back from an idle state.
 *
 * @return < 0 on error.
 */
int sceCtrlGetIdleCancelThreshold(int *idleReset, int *idleBack);

/**
 * Set analog threshold relating to the idle timer.
 *
 * @param idlereset - Movement needed by the analog to reset the idle timer.
 * @param idleback - Movement needed by the analog to bring the PSP back from an idle state.
 *
 * Set to -1 for analog to not cancel idle timer.
 * Set to 0 for idle timer to be cancelled even if the analog is not moved.
 * Set between 1 - 128 to specify the movement on either axis needed by the analog to fire the event.
 *
 * @return < 0 on error.
 */
int sceCtrlSetIdleCancelThreshold(int idlereset, int idleback);

/**
 * Enable/disable controller input. Set to PSP_CTRL_POLL_MODE_POLLING by Sony when initiating the controller.
 * 
 * @param pollMode - One of ::PspCtrlPollMode. If set to 0, no button/analog input is recognized.
 *                   Set to 1 to enable button/analog input.
 * 
 * @return 0.
 */
int sceCtrlSetPollingMode(u8 pollMode);

/**
 * Read the current internal SceCtrlLatch buffer. The following button states are delivered:
 *                                                Button is pressed, button is not pressed, button has been newly pressed
 *                                                and button has been newly released. 
 * Once a button has been i.e. pressed, its value is stored in the specific internal latch buffer member (uiMake in this case)
 * until you manually reset the specific latch buffer field.
 * 
 * @param latch - Pointer to a SceCtrlLatch struct retrieving the current latch_data.
 * 
 * @return > 0 on success, < 0 on error.
 */
int sceCtrlPeekLatch(SceCtrlLatch *latch);

/**
 * Read the current internal SceCtrlLatch buffer and reset the buffer afterwards. The following button states are delivered:
 *                                                Button is pressed, button is not pressed, button has been newly pressed
 *                                                and button has been newly released. 
 * After the internal latch buffer has been read, it will be cleaned (all members will be reset to zero)
 * 
 * @param latch - Pointer to a SceCtrlLatch struct retrieving the current latch_data.
 * 
 * @return > 0 on success, < 0 on error.
 */
int sceCtrlReadLatch(SceCtrlLatch *latch);

#endif	/* CTRL_H */