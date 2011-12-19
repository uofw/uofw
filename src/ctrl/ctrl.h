/* Copyright (C) 2011 The uOFW team
   See the file COPYING for copying permission.
*/

/** 
 * ctrl.h
 *
 * @author: _Felix_
 * @version: 6.39
 * 
 * Controller library of the SCE PSP system.
 */

/** @defgroup Controller Controller library
 *
 * @{	
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

#define sceCtrlPeekBufferPositiveExt    sceCtrl_5A36B1C2
#define sceCtrlPeekBufferNegativeExt    sceCtrl_239A6BA7
#define sceCtrlReadBufferPositiveExt    sceCtrl_1098030B
#define sceCtrlReadBufferNegativeExt    sceCtrl_7C3675AB
#define sceCtrlExtendInternalCtrlBuf    sceCtrl_driver_65698764

/** General information about an internal PSP controller buffer. Including current pressed button(s) and current position 
 *  of the analog controller. 
 */
typedef struct _SceCtrlData {
    /** The time, how long the D-Pad & the Analog-Pad have been active. Time unit is in microseconds. 
     *  Can be used to get the time period if a button pressing. 
     */ 
    u32 activeTime; //0
    /** The currently pressed D-Pad button(s). Bitwise OR'ed values of :PspCtrlButtons. */
    u32 buttons; //4
    /** Analog Stick X-axis offset (0 - 255). Left = 0, Right = 255. */
    u8 aX; //8
    /** Analog Stick Y-axis offset (0 - 255). Up = 0, Down = 255. */
    u8 aY; //9
    /** Reserved. Values are normally set to 0. */
    u8 rsrv[6]; //10
} SceCtrlData; //Size of SceCtrlData: 16

/** Extended SceCtrlData struct. */
typedef struct _SceCtrlDataExt {
    /** The time, how long the D-Pad & the Analog-Pad have been active. Time unit is in microseconds. 
     *  Can be used to get the time period if a button pressing. 
     */ 
    u32 activeTime; //0
    /** The currently pressed D-Pad button(s). Bitwise OR'ed values of :PspCtrlButtons. */
    u32 buttons; //4
    /** Analog Stick X-axis offset (0 - 255). Left = 0, Right = 255. */
    u8 aX; //8
    /** Analog Stick Y-axis offset (0 - 255). Up = 0, Down = 255. */
    u8 aY; //9
    /** Reserved. Values are normally set to 0. */
    u8 rsrv[6]; //10
    /** Unknown. */
    int unk1; //16
    /** Unknown. */
    int unk2; //20
    /** Unknown. */
    int unk3; //24
    /** Unknown. */
    int unk4; //28
    /** Unknown. */
    int unk5; //32
    /** Unknown. */
    int unk6; //36
    /** Unknown. */
    int unk7; //40
    /** Unknown. */
    int unk8; //44
} SceCtrlDataExt;

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
} SceCtrlLatch; //Size of SceCtrlLatch: 16

typedef struct _SceCtrlLatchInt {
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
    int unk2; //20
    int unk3; //24
    void *sceCtrlBuf[3]; //28 size of SceCtrlDataInt can be either 16 or 48
} SceCtrlLatchInt; //sizeof SceCtrlLatchInt: 

/** Required information for a button callback. Unique to each registered button callback. */
typedef struct _SceCtrlButtonCallback {
    /** Bitwise OR'ed button values (of ::PspCtrlButtons) which will be checked for being pressed. */
    u32 btnMask; //0
    /** Pointer to a callback function handling the button input effects. */
    void (*callbackFunc)(int, int, void *); //4
    /** The global pointer ($gp) value of the controller module. */
    u32 gp; //8
    /** An optional pointer being passed as the third argument to the callback function. */
    void *arg; //12    
} SceCtrlButtonCallback; //Size of SceCtrlButtonCallback: 16

/**
 * Enumeration for the digital controller buttons.
 *
 * @note PSP_CTRL_HOME, PSP_CTRL_WLAN_UP, PSP_CTRL_REMOTE, PSP_CTRL_VOLUP, PSP_CTRL_VOLDOWN, PSP_CTRL_SCREEN, PSP_CTRL_NOTE, PSP_CTRL_DISC, 
 *       PSP_CTRL_MS can only be read in kernel mode.
 */
enum PspCtrlButtons {
    /** Select button. Negative value = 0xFFFFFFFE. */
    PSP_CTRL_SELECT     = 0x1,
    /** Start button. Negative value = 0xFFFFFFF7. */
    PSP_CTRL_START      = 0x8,
    /** Up D-Pad button. Negative value = 0xFFFFFFEF. */
    PSP_CTRL_UP         = 0x10,
    /** Right D-Pad button. Negative value = 0xFFFFFFDF. */
    PSP_CTRL_RIGHT      = 0x20,
    /** Down D-Pad button. Negative value = 0xFFFFFFBF. */
    PSP_CTRL_DOWN       = 0x40,
    /** Left D-Pad button. Negative value = 0xFFFFFF7F. */
    PSP_CTRL_LEFT       = 0x80,
    /** Left trigger. Negative value = 0xFFFFFEFF. */
    PSP_CTRL_LTRIGGER   = 0x100,
    /** Right trigger. Negative value = 0xFFFFFDFF. */
    PSP_CTRL_RTRIGGER   = 0x200,
    /** Triangle button. Negative value = 0xFFFFEFFF. */
    PSP_CTRL_TRIANGLE   = 0x1000,
    /** Circle button. Negative value = 0xFFFFDFFF. */
    PSP_CTRL_CIRCLE     = 0x2000,
    /** Cross button. Negative value = 0xFFFFBFFF. */
    PSP_CTRL_CROSS      = 0x4000,
    /** Square button. Negative value = 0xFFFF7FFF. */
    PSP_CTRL_SQUARE     = 0x8000,
    /** Home button. In user mode this bit is set if the exit dialog is visible.  Negative value = 0xFFFEFFFF. */
    PSP_CTRL_HOME       = 0x10000,
    /** Hold button. Negative value = 0xFFFDFFFF. */
    PSP_CTRL_HOLD       = 0x20000,
    /** Wlan switch up. Negative value = 0xFFFBFFFF. */
    PSP_CTRL_WLAN_UP    = 0x40000,
    /** Remote hold position. Negative value = 0xFFF7FFFF. */
    PSP_CTRL_REMOTE     = 0x80000,
    /** Volume up button. Negative value = 0xFFEFFFFF. */
    PSP_CTRL_VOLUP      = 0x100000,
    /** Volume down button. Negative value = 0xFFBFFFFF. */
    PSP_CTRL_VOLDOWN    = 0x200000,
    /** Screen button. Negative value = 0xFFBFFFFF. */
    PSP_CTRL_SCREEN     = 0x400000,
    /** Music Note button. Negative value = 0xFF7FFFFF. */
    PSP_CTRL_NOTE       = 0x800000,   	
    /** Disc present. Negative value = 0xFEFFFFFF. */
    PSP_CTRL_DISC       = 0x1000000,
    /** Memory stick present. Negative value = 0xFDFFFFFF. */
    PSP_CTRL_MS         = 0x2000000,
};

/** Controller input modes. */
enum PspCtrlInputMode {
    /** Digitial input only. No recognizing of analog input. */
    PSP_CTRL_INPUT_NO_ANALOG = 0,         
    /** Recognizing of both digital and analog input. */
    PSP_CTRL_INPUT_ANALOG = 1,
};

/** Controller input poll-modes. */
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
 * Initialize the controller library.
 * 
 * @return 0 on success, otherwise < 0.
 */
int sceCtrlInit();

/**
 * Terminate the controller library.
 * 
 * @return 0.
 */
int sceCtrlEnd();

/**
 * Suspend the controller library.
 * 
 * @return 0.
 */
int sceCtrlSuspend();

/**
 * Resume the controller library after it has been suspended.
 * 
 * @return 0.
 */
int sceCtrlResume();

/**
 * Register a button callback.
 * 
 * @note In the PSPSDK, this function is defined as sceCtrlRegisterButtonCallback.
 * 
 * @param slot The slot used to register the callback (0-3). Although Sony uses atleast slot 0 and slot 1 of the possible 
 *             4 callback slots in game mode, we can use them all freely as we wish.
 * @param btnMask Bitwise OR'ed button values which will be checked for being pressed.
 * @param cb Pointer to the callback function (int curr, int last, void *arg), which handles button input effects.
 * @param arg Optional user argument. Passed to the callback function as its third argument.
 * 
 * @return 0 on success or < 0, if slot is different than 0-3.
 */
int sceCtrlSetSpecialButtonCallback(int slot, u32 btnMask, void (*cb)(int, int, void *), void *arg);

/**
 * Get the set button mask mode of a button value.
 * 
 * @note In the PSPSDK, this function is defined as sceCtrlGetButtonMask.
 * 
 * @param btnMask The button bit value to check for (one or more buttons of ::PspCtrlButtons).
 * 
 * @return 0 for nothing has been set for this button bit value. Returns 1, if the button bit value is included
 *         in a button bit value which is masked. Returns 2 in the case of the button bit value being set as "button set".
 */
int sceCtrlGetButtonIntercept(u32 btnMask);

/**
 * Set a button mask mode for one or more buttons. You can only mask user mode buttons in user applications.
 * Masking of kernel mode buttons is ignored as well as buttons used in kernel mode applications.
 * 
 * @note In the PSPSDK, this function is defined as sceCtrlSetButtonMask.
 * 
 * @param btnMask The button bit value for which the button mask mode will be applied. 
 *                  One or more buttons of ::PspCtrlButtons.
 * @param btnMaskMode The mask mode. 0 for no masking, 1 for masking, 2 for button setting.
 *                      Note: Set to 2 will only mask button(s), this seems to be an implementation error.  
 * 
 * @return 0, 1 for setting a complete new button bit mask (which is not included in a previously set button bit mask).               
 */
int sceCtrlSetButtonIntercept(u32 btnMask, u8 btnMaskMode);

/**
 * Get the current controller input mode.
 * 
 * @param mode Pointer to int receiving the current controller mode. One of ::PspCtrlInputMode.
 * 
 * @return 0.
 */
int sceCtrlGetSamplingMode(int *mode);

/**
 * Set the controller input mode.
 * 
 * @param mode The new controller input mode. One of ::PspCtrlInputMode.
 * 
 * @return The previous input mode on success, or < 0, if mode is not a value of ::PspCtrlInputMode.
 */
int sceCtrlSetSamplingMode(u8 mode);

/**
 * Get the current cycle specifying the update frequency of the internal ctrl buffer.
 * 
 * @param cycle Pointer to int receiving the current cycle.
 * 
 * @return 0.
 */
int sceCtrlGetSamplingCycle(int *cycle);

/**
 * Set the update frequency of the internal ctrl pad buffer Default update frequency is 60 hz.
 * 
 * @param cycle The new time period between two samplings of controller attributes in microseconds.
 *                Setting to 0 triggers sampling at every VSYNC-event (60 updates/second). If you want to set an own
 *                time period for updating the internal ctrl pad buffer, cycle has to be > 5554 and < 20001.
 * 
 * @return The previous cycle on success, or < 0, if cycle is smaller than 5555 (excluding 0) and greater than 20000.
 */
int sceCtrlSetSamplingCycle(int cycle);

/**
 * Get the idle threshold values.
 *
 * @param idlerest Movement needed by the analog to reset the idle timer.
 * @param idleback Movement needed by the analog to bring the PSP back from an idle state.
 *
 * @return < 0 on error.
 */
int sceCtrlGetIdleCancelThreshold(int *idleReset, int *idleBack);

/**
 * Set analog threshold relating to the idle timer.
 *
 * @param idlereset Movement needed by the analog to reset the idle timer.
 * @param idleback Movement needed by the analog to bring the PSP back from an idle state.
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
 * @param pollMode One of ::PspCtrlPollMode. If set to 0, no button/analog input is recognized.
 *                 Set to 1 to enable button/analog input.
 * 
 * @return 0.
 */
int sceCtrlSetPollingMode(u8 pollMode);

/**
 * Read the current internal latch buffer. The following button states are delivered:
 *                                         Button is pressed, button is not pressed, button has been newly pressed
 *                                         and button has been newly released. 
 * Once a button has been i.e. pressed, its value is stored in the specific internal latch buffer member (uiMake in this case)
 * until you manually reset the specific latch buffer field.
 * 
 * @param latch Pointer to a SceCtrlLatch struct retrieving the current internal latch buffer.
 * 
 * @return The amount of reads of the internal latch buffer without being reseted, or < 0 on error.
 */
int sceCtrlPeekLatch(SceCtrlLatch *latch);

/**
 * Read the current internal latch buffer and reset the buffer afterwards. The following button states are delivered:
 *                                                Button is pressed, button is not pressed, button has been newly pressed
 *                                                and button has been newly released. 
 * After the internal latch buffer has been read, it will be cleaned (all members will be reset to zero).
 * 
 * @param latch Pointer to a SceCtrlLatch struct retrieving the current internal latch buffer.
 * 
 * @return The amount of reads of the internal latch buffer without being reseted (typically 1), or < 0 on error.
 */
int sceCtrlReadLatch(SceCtrlLatch *latch);

/**
 * Read the current internal ctrl buffer. Does not wait for the next VBlank.
 * 
 * @param pad Pointer to a SceCtrlData struct retrieving the current internal ctrl buffer.
 * @param count The number of internal buffers to read. There are 64 internal ctrl buffers which can be read.
 *              Has to be set to a value between 1 and 64 (including the bounds).
 * 
 * @return The amount of read internal ctrl buffers, or < 0 on error.
 */
int sceCtrlPeekBufferPositive(SceCtrlData *pad, u8 count);

/**
 * Read the current internal SceCtrlData buffer. Does not wait for the next VBlank.
 * 
 * @param pad Pointer to a SceCtrlData struct retrieving the current internal ctrl buffer. Here, the button values are turned off,
 *            this means the button value for PSP_CTRL_CROSS is 0xBFFF instead of 0x4000. 
 *            Check ::PspCtrlButtons for the negative active values of the buttons. If no button is active, the internal
 *            button value is 0xFFFFFFFF.
 * @param count The number of internal buffers to read. There are 64 internal ctrl buffers which can be read.
 *              Has to be set to a value between 1 and 64 (including the bounds).
 * 
 * @return The amount of read internal ctrl buffers, or < 0 on error.
 */
int sceCtrlPeekBufferNegative(SceCtrlData *pad, u8 count);

/**
 * Read the current internal SceCtrlData buffer. By default, the internal ctrl buffer will be read after every VSYNC period (60 times/sec).
 * You can set your own update timer by using sceCtrlSetSamplingCycle.
 * 
 * @param pad Pointer to a SceCtrlData struct retrieving the current internal button buffer. 
 * @param count The number of internal buffers to read. There are 64 internal ctrl buffers which can be read.
 *              Has to be set to a value between 1 and 64 (including the bounds).
 * 
 * @return < 0 on error, otherwise 1.
 */
int sceCtrlReadBufferPositive(SceCtrlData *pad, u8 count);

/**
 * Read the current internal SceCtrlData buffer. By default, the internal ctrl buffer will be read after every VSYNC period (60 times/sec).
 * You can set your own update time by using sceCtrlSetSamplingCycle.
 * 
 * @param pad Pointer to a SceCtrlData struct retrieving the current internal ctrl buffer. Here, the button values are turned off,
 *            this means the button value for  PSP_CTRL_CROSS is 0xBFFF instead of 0x4000. 
 *            Check ::PspCtrlButtons for the negative active values of the buttons. If no button is active, the internal
 *            button value is 0xFFFFFFFF.
 * @param count The number of internal buffers to read. There are 64 internal ctrl buffers which can be read.
 *              Has to be set to a value between 1 and 64 (including the bounds).
 * 
 * @return < 0 on error, otherwise 1.
 */
int sceCtrlReadBufferNegative(SceCtrlData *pad, u8 count);

/**
 * Extended sceCtrlPeekBufferPositive(see description for more info).
 * You need to call sceCtrlExtendInternalCtrlBuf() before use.
 * 
 * @param arg1 Pass 1 or 2.
 * @param padExt Pointer to a SceCtrlData struct retrieving the current internal ctrl buffer.
 * @param count Has to be set to a value between 1 and 64 (including the bounds).
 * 
 * @return The amount of read internal ctrl buffers, or < 0 on error.
 */
int sceCtrlPeekBufferPositiveExt(int arg1, SceCtrlDataExt *padExt, u8 count);

/**
 * Extended sceCtrlPeekBufferNegative (see description for more info). 
 * You need to call sceCtrlExtendInternalCtrlBuf() before use.
 * 
 * @param arg1 Pass 1 or 2.
 * @param padExt Pointer to a SceCtrlData struct retrieving the current internal ctrl buffer.
 * @param count Has to be set to a value between 1 and 64 (including the bounds).
 * 
 * @return The amount of read internal ctrl buffers, or < 0 on error.
 */
int sceCtrlPeekBufferNegativeExt(int arg1, SceCtrlDataExt *padExt, u8 count);

/**
 * Extended sceCtrlReadBufferPositive (see description for more info).
 * You need to call sceCtrlExtendInternalCtrlBuf() before use.
 * 
 * @param arg1 Pass 1 or 2.
 * @param padExt Pointer to a SceCtrlData struct retrieving the current internal ctrl buffer.
 * @param count Has to be set to a value between 1 and 64 (including the bounds).
 * 
 * @return < 0 on error, otherwise 1.
 */
int sceCtrlReadBufferPositiveExt(int arg1, SceCtrlDataExt *padExt, u8 count);

/**
 * Extended sceCtrlReadBufferNegative (see description for more info).
 * You need to call sceCtrlExtendInternalCtrlBuf() before use.
 * 
 * @param arg1 Pass 1 or 2.
 * @param padExt Pointer to a SceCtrlData struct retrieving the current internal ctrl buffer.
 * @param count Has to be set to a value between 1 and 64 (including the bounds).
 * 
 * @return < 0 on error, otherwise 1.
 */
int sceCtrlReadBufferNegativeExt(int arg1, SceCtrlDataExt *padExt, u8 count);

/**
 * Extend the 64 internal controller buffers. Every ctrl buffer now represents a SceCtrlDataExt struct.
 * By default, an internal controller is equivalent to a SceCtrlData struct. This function has to be called before using
 * the extended read/peekBuffer functions.
 * 
 * @param mode Seems to be an index. Pass either 1 or 2.
 * @param arg2 Unknown, pass 1.
 * @param arg3 Unknown, pass 1.
 * 
 * @return 0 on succss, otherwise < 0 on error.
 */
int sceCtrlExtendInternalCtrlBuf(u8 mode, int arg2, int arg3);

/**
 * Set a number of VBlanks for which will be waited when terminating the controller library.
 * 
 * @param suspendSamples The number of VBlanks. Between 0 - 300.
 * 
 * @return 0 on success, < 0 if suspendSamples is not between 0 - 300.
 */
int sceCtrlSetSuspendingExtraSamples(u16 suspendSamples);

/**
 * Get the number of VBlanks for which will be waited when terminating the controller library.
 * 
 * @return The number of VBlanks.
 */
u16 sceCtrlGetSuspendingExtraSamples();

#endif	/* CTRL_H */

/** @} */
