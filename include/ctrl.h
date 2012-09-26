/** Copyright (C) 2011, 2012 The uOFW team
   See the file COPYING for copying permission.
*/

#include "common_header.h"

/** @defgroup Controller Controller Module
 *
 * @{	
 */

#ifndef CTRL_H
#define	CTRL_H

/** The callback function used by ::sceCtrlSetSpecialButtonCallback. */
typedef void (*SceKernelButtonCallbackFunction)(u32 curButtons, u32 lastButtons, void *opt);

/** 
 * This structure is for obtaining button data (button/stick information) from the 
 * controller using ::sceCtrlPeekBufferPositive, ::sceCtrlReadBufferNegative and similar 
 * functions.
 */
typedef struct {
    /** 
     * The time, how long the D-Pad & the Analog-Pad have been active. Time unit is in microseconds. 
     * Can be used to get the time period of a button pressing event. 
     */ 
    u32 activeTime;
    /** The currently pressed button. Bitwise OR'ed values of ::SceCtrlPadButtons. */
    u32 buttons;
    /** Analog Stick X-axis offset (0 - 255). Left = 0, Right = 255. */
    u8 aX;
    /** Analog Stick Y-axis offset (0 - 255). Up = 0, Down = 255. */
    u8 aY;
    /** Reserved. Values are normally set to 0. */
    u8 rsrv[6];
} SceCtrlData;

/** 
 * This structure is for obtaining button data (button/stick information) from the 
 * controller using ::sceCtrlPeekBufferPositiveExtra, ::sceCtrlReadBufferNegativeExtra
 * and similar functions.
 */
typedef struct {
    /** 
     * The time, how long the D-Pad & the Analog-Pad have been active. Time unit is in microseconds. 
     * Can be used to get the time period if a button pressing. 
     */ 
    u32 activeTime;
    /** The currently pressed button. Bitwise OR'ed values of ::SceCtrlPadButtons. */
    u32 buttons;
    /** Analog Stick X-axis offset (0 - 255). Left = 0, Right = 255. */
    u8 aX;
    /** Analog Stick Y-axis offset (0 - 255). Up = 0, Down = 255. */
    u8 aY;
    /** Reserved. Values are normally set to 0. */
    u8 rsrv[6];
    /** Unknown. */
    s32 unk1;
    /** Unknown. */
    s32 unk2;
    /** Unknown. */
    s32 unk3;
    /** Unknown. */
    s32 unk4;
    /** Unknown. */
    s32 unk5;
    /** Unknown. */
    s32 unk6;
    /** Unknown. */
    s32 unk7;
    /** Unknown. */
    s32 unk8;
} SceCtrlDataExt;

/** 
 * This structure is for obtaining button status values from the controller using
 * ::sceCtrlPeekLatch and ::sceCtrlReadLatch. Each structure member contains button
 * values of ::SceCtrlPadButtons.
 */
typedef struct {
    /** Button is newly pressed (was not already been pressed). */
    u32 buttonMake;
    /** Stop of button press. It was pressed one frame before the current one. */
    u32 buttonBreak;
    /** Button is pressed. */
    u32 buttonPress;
    /** Button is not pressed. */
    u32 buttonRelease;
} SceCtrlLatch;

/**
 * Unknown structure. 
 */
typedef struct {
    /** Unknown. */
    s32 unk1;
    /** Unknown. */
    s32 (*func)(int);
} SceCtrlUnkStruct;

/**
 * Enumeration for the digital controller buttons in positive logic.
 *
 * @note SCE_CTRL_HOME, SCE_CTRL_WLAN_UP, SCE_CTRL_REMOTE, SCE_CTRL_VOLUP, SCE_CTRL_VOLDOWN, 
 *       SCE_CTRL_SCREEN, SCE_CTRL_NOTE, SCE_CTRL_DISC, SCE_CTRL_MS can only be read in kernel mode.
 */
enum SceCtrlPadButtons {
    /** Select button. */
    SCE_CTRL_SELECT     = 0x1,
    /** Start button. */
    SCE_CTRL_START      = 0x8,
    /** Up D-Pad button. */
    SCE_CTRL_UP         = 0x10,
    /** Right D-Pad button. */
    SCE_CTRL_RIGHT      = 0x20,
    /** Down D-Pad button. */
    SCE_CTRL_DOWN       = 0x40,
    /** Left D-Pad button. */
    SCE_CTRL_LEFT       = 0x80,
    /** Left trigger. */
    SCE_CTRL_LTRIGGER   = 0x100,
    /** Right trigger. */
    SCE_CTRL_RTRIGGER   = 0x200,
    /** Triangle button. */
    SCE_CTRL_TRIANGLE   = 0x1000,
    /** Circle button. */
    SCE_CTRL_CIRCLE     = 0x2000,
    /** Cross button. */
    SCE_CTRL_CROSS      = 0x4000,
    /** Square button. */
    SCE_CTRL_SQUARE     = 0x8000,
    /** Home button. In user mode this bit is set if the exit dialog is visible.*/
    SCE_CTRL_HOME       = 0x10000,
    /** Hold button. */
    SCE_CTRL_HOLD       = 0x20000,
    /** W-LAN switch up. */
    SCE_CTRL_WLAN_UP    = 0x40000,
    /** Remote hold position. */
    SCE_CTRL_REMOTE     = 0x80000,
    /** Volume up button. */
    SCE_CTRL_VOLUP      = 0x100000,
    /** Volume down button. */
    SCE_CTRL_VOLDOWN    = 0x200000,
    /** Screen button. */
    SCE_CTRL_SCREEN     = 0x400000,
    /** Music Note button. */
    SCE_CTRL_NOTE       = 0x800000,   	
    /** Disc present. */
    SCE_CTRL_DISC       = 0x1000000,
    /** Memory stick present. */
    SCE_CTRL_MS         = 0x2000000,
};

/** Controller input modes. */
enum SceCtrlPadInputMode {
    /** Digital input only. No recognizing of analog input. */
    SCE_CTRL_INPUT_DIGITAL_ONLY = 0,         
    /** Recognizing of both digital and analog input. */
    SCE_CTRL_INPUT_DIGITAL_ANALOG = 1,
};

/** Controller input poll modes. */
enum SceCtrlPadPollMode {
    /** No controller input is recognized. */
    SCE_CTRL_POLL_INACTIVE = 0,
    /** Controller input is recognized. */
    SCE_CTRL_POLL_ACTIVE = 1,
};

/** Button mask settings. */
enum SceCtrlPadButtonMaskMode {
    /** No mask for the specified buttons. Button input is normally recognized. */
    SCE_CTRL_MASK_NO_MASK = 0,
    /** 
     * The specified buttons are ignored, that means even if these buttons are pressed by the user
     * they won't be shown as pressed internally. You can only block user buttons for applications 
     * running in User Mode. 
     */
    SCE_CTRL_MASK_IGNORE_BUTTONS = 1,
    /** 
     * The specified buttons show up as being pressed, even if the user does not press them.
     * You can only turn ON user buttons for applications running in User Mode. 
     */
    SCE_CTRL_MASK_APPLY_BUTTONS = 2,
};

/**
 * Initialize the controller device. Bind the controller driver to the controller device.
 * 
 * @return 0 on success.
 */
s32 sceCtrlInit(void);

/**
 * Terminate the controller device. Unbind the controller driver from the controller device.
 * 
 * @return 0.
 */
s32 sceCtrlEnd(void);

/**
 * Suspend the controller driver and put the controller device into a low-power state.
 * 
 * @return 0.
 */
s32 sceCtrlSuspend(void);

/**
 * Resume the controller driver after and bring the controller device back from a low-power state.
 * 
 * @return 0.
 */
s32 sceCtrlResume(void);

/**
 * Enable/disable controller device input.
 * 
 * @param pollMode One of ::SceCtrlPadPollMode. If set to 0, no button/analog input is recognized.
 *                 Set to 1 to enable button/analog input.
 * 
 * @return 0.
 */
u32 sceCtrlSetPollingMode(u8 pollMode);

/**
 * Get the current controller device input mode.
 * 
 * @param mode Receiving the current controller mode. One of ::SceCtrlPadInputMode.
 * 
 * @return 0.
 */
u32 sceCtrlGetSamplingMode(u8 *mode);

/**
 * Set the controller device input mode.
 * 
 * @param mode The new controller input mode. One of ::SceCtrlPadInputMode.
 * 
 * @return The previous input mode on success.
 */
s32 sceCtrlSetSamplingMode(u8 mode);

/**
 * Get the current update interval of the internal controller data buffers.
 * 
 * @param cycle Receiving the current update interval (in microseconds).
 * 
 * @return 0.
 */
u32 sceCtrlGetSamplingCycle(u32 *cycle);

/**
 * Set the update frequency of the internal controller buffer. Default update interval 
 * is the VBlank interrupt (approximately 60 times per second).
 * 
 * @param cycle The new interval between two samplings of controller attributes in microseconds.
 *              Setting to 0 enables the VBlank-Interrupt-Update process. If you want to set an own
 *              interval for updating the internal controller buffers, cycle has to be greater 5554 
 *              and less than 20001.
 *              This will disable the VBlank-Interrupt-Update process.
 * 
 * @return The previous cycle on success.
 */
s32 sceCtrlSetSamplingCycle(u32 cycle);

/**
 * Obtain the different cancel-idle-timer buttons.
 * 
 * @param oneTimeResetButtons Pointer retrieving the buttons reseting the timer when being pressed a 
 *                            new time (not being pressed immediately before).
 * @param allTimeResetButtons Pointer retrieving the buttons reseting the timer when being pressed.
 * @param oneTimeHoldResetButtons Pointer retrieving the buttons reseting the timer when being pressed 
 *                                a new time (not being pressed immediately before). These buttons are
 *                                checked for when HOLD mode is active.
 * @param allTimeHoldResetButtons Pointer retrieving the buttons reseting the timer when being pressed 
 *                                a new time. These buttons are checked for when HOLD mode is active.
 * 
 * @return 0 on success.
 */
u32 sceCtrlGetIdleCancelKey(u32 *oneTimeResetButtons, u32 *allTimeResetButtons, u32 *oneTimeHoldResetButtons, 
                            u32 *allTimeHoldResetButtons);

/**
 * Specify the buttons which, when being pressed, reset the idle timer. It is satisfying to press only
 * one button of the specified buttons to reset it.
 * 
 * @param oneTimeResetButtons The buttons needed to be pressed to reset the timer. One or more of 
 *                            ::SceCtrlPadButtons. If you keep pressing these buttons after resetting 
 *                            the timer, they will not reset the timer anymore. You will have to 
 *                            release the buttons first, before they can reset it again. In case HOLD 
 *                            mode is active, pressing these buttons will not reset the timer.
 * @param allTimeResetButtons The buttons needed to be pressed to reset the timer. One or more of 
 *                            ::SceCtrlPadButtons. As long as you press one of these buttons, the timer 
 *                            is reset. In case HOLD mode is active, pressing these buttons will not 
 *                            reset the timer.
 * @param oneTimeHoldResetButtons The buttons needed to be pressed to reset the timer when HOLD mode is 
 *                                active. One or more of ::SceCtrlPadButtons. If you keep pressing these 
 *                                buttons after resetting the timer, they will not reset it anymore. You 
 *                                will have to release the buttons first, before they can reset the timer 
 *                                again.
 * @param allTimeHoldResetButtons The buttons needed to be pressed to reset the timer when HOLD mode is 
 *                                active. 
 *                                One or more of ::SceCtrlPadButtons. As long as you press one of these 
 *                                buttons, the timer is reset.
 * 
 * @return 0 on success.
 * 
 * @par Example:
 * @code
 * //Pressing the select will reset the idle timer. No other button will reset it.
 * sceCtrlSetIdleCancelKey(0, SCE_CTRL_SELECT, 0, 0);
 * @endcode
 */
u32 sceCtrlSetIdleCancelKey(u32 oneTimeResetButtons, u32 allTimeResetButtons, u32 oneTimeHoldResetButtons, 
                            u32 allTimeHoldResetButtons);

/**
 * Get the idle timer cancel threshold values for the analog stick.
 *
 * @param iUnHoldThreshold Movement needed by the analog stick to reset the idle timer. Used when 
 *        HOLD mode is inactive. -1 is obtained when the analog stick cannot cancel the idle timer,
 *        otherwise the movement needed by the analog stick to cancel the idle timer.
 * @param iHoldThreshold Movement needed by the analog stick to reset the idle timer. Used when 
 *                       HOLD mode is active. -1 is obtained when the analog stick cannot cancel the 
 *                       idle timer, otherwise the movement needed by the analog stick to cancel the idle timer.
 *
 * @return 0 on success.
 */
s32 sceCtrlGetIdleCancelThreshold(s32 *iUnHoldThreshold, s32 *iHoldThreshold);

/**
 * Set analog stick threshold values canceling the idle timer.
 *
 * @param iUnHoldThreshold Movement needed by the analog stick to reset the idle timer. Used when 
 *                         HOLD mode is inactive.
 *                         Set between 1 - 128 to specify the movement on either axis.
 *                         Set to 0 for idle timer to be canceled even if the analog stick is not moved.
 *                         Set to -1 for analog stick not canceling idle timer (although it is moved).
 * @param iHoldThreshold Movement needed by the analog stick to reset the idle timer. Used when 
 *                       HOLD mode is active.
 *                       Set between 1 - 128 to specify the movement on either axis.
 *                       Set to 0 for idle timer to be canceled even if the analog stick is not moved.
 *                       Set to -1 for analog stick not canceling idle timer (although it is moved).
 *
 *
 * @return 0 on success.
 */
s32 sceCtrlSetIdleCancelThreshold(s32 iUnHoldThreshold, s32 iHoldThreshold);

/**
 * Get the number of VBlanks which will be waited for when the PSP device is being suspended.
 * 
 * @return The number of VBlanks to wait for.
 */
s16 sceCtrlGetSuspendingExtraSamples(void);

/**
 * Set a number of VBlanks which will be waited for when the PSP device is being suspended.
 * 
 * @param suspendSamples The number of VBlanks. Between 0 - 300.
 * 
 * @return 0 on success.
 */
s32 sceCtrlSetSuspendingExtraSamples(s16 suspendSamples);

/**
 * Extend the 64 internal controller buffers to represent SceCtrlDataExt structures.
 * By default, an internal controller buffer is equivalent to a SceCtrlData structure. This function 
 * has to be called before using the extended read/peekBuffer functions (only the first time).
 * 
 * @param mode Seems to be an index. Pass either 1 or 2.
 * @param arg2 Unknown. Pointer to a ctrlUnkStruct structure?
 * @param arg3 Unknown.
 * 
 * @return 0 on success.
 */
s32 sceCtrlExtendInternalCtrlBuffers(u8 mode, SceCtrlUnkStruct *arg2, s32 arg3);

/**
 * Obtain button latch data stored in the internal latch controller buffers. The following button 
 * states can be obtained:
 *      Button is pressed, button is not pressed, button has been newly pressed
 *      and button has been newly released. 
 * Once a button has been, for example, pressed, its value is stored into the specific latch member 
 * (btnMake in this case) until you manually reset the specific latch buffer field.
 * 
 * @param latch Pointer to a SceCtrlLatch structure retrieving the current button latch data.
 * 
 * @return The number of reads of the internal latch buffer, without being reset, on success.
 * 
 * @par Example:
 * @code
 * SceCtrlLatch latch;
 * 
 * sceCtrlPeekLatch(&latch);
 * while (1) {
 *        //Cross button pressed
 *        if (latch.btnPress & SCE_CTRL_CROSS) {
 *            //do something
 *        }
 * }
 * @endcode
 * 
 */
s32 sceCtrlPeekLatch(SceCtrlLatch *latch);

/**
 * Obtain button latch data stored in the internal latch controller buffers. The following button 
 * states can be obtained:
 *      Button is pressed, button is not pressed, button has been newly pressed
 *      and button has been newly released. 
 * After the internal latch data has been read, it will be reset to zero again.
 * 
 * @param latch Pointer to a SceCtrlLatch structure retrieving the current button latch data.
 * 
 * @return The number of reads of the internal latch buffer, without being reset, (typically 1) on 
 * success.
 */
s32 sceCtrlReadLatch(SceCtrlLatch *latch);

/**
 * Obtain button data stored in the internal controller buffers. Does not wait for the next 
 * update interval to be performed.  
 * The obtained data will be the latest transfered button data into the internal controller buffers.
 * 
 * @param data Pointer to controller data structure in which button information is stored. The obtained
 *             button data is represented in positive logic.
 * @param nBufs The number of internal controller buffers to read. There are 64 internal controller 
 *              buffers which can be read. Has to be set to a value in the range of 1 - 64.
 * 
 * @return The number of read internal controller buffers on success.
 * 
 * @par Example:
 * @code
 * SceCtrlData data;

 * sceCtrlSetSamplingCycle(0);
 * sceCtrlSetSamplingMode(SCE_CTRL_INPUT_DIGITAL_ANALOG);
 * 
 * while (1) {
 *        sceCtrlPeekBufferPositive(&data, 1); 
 *        //Cross button pressed
 *        if (data.buttons & SCE_CTRL_CROSS) {
 *            //do something
 *        }
 * }
 * @endcode
 */

s32 sceCtrlPeekBufferPositive(SceCtrlData *data, u8 nBufs);

/**
 * Obtain button data stored in the internal controller buffers. Does not wait for the next 
 * update interval to be performed. 
 * The obtained data will be the latest transfered button data into the internal controller buffers.
 * 
 * @param data Pointer to controller data structure in which button information is stored. The obtained
 *             button data is represented in negative logic.
 * @param nBufs The number of internal controller buffers to read. There are 64 internal controller 
 *              buffers which can be read. Has to be set to a value in the range of 1 - 64.
 * 
 * @return The number of read internal controller buffers on success.
 * 
 * @par Example:
 * @code
 * SceCtrlData data;

 * sceCtrlSetSamplingCycle(0);
 * sceCtrlSetSamplingMode(SCE_CTRL_INPUT_DIGITAL_ANALOG);
 * 
 * while (1) {
 *        sceCtrlPeekBufferNegative(&data, 1); 
 *        //Cross button pressed
 *        if (data.buttons & ~SCE_CTRL_CROSS) {
 *            //do something
 *        }
 * }
 * @endcode
 */
s32 sceCtrlPeekBufferNegative(SceCtrlData *data, u8 nBufs);

/**
 * Obtain button data stored in the internal controller buffers. Waits for the next update interval
 * before obtaining the data. The read data is the newest transfered data into the internal controller 
 * buffers.
 * 
 * @param data Pointer to controller data structure in which button information is stored. The obtained
 *             button data is represented in positive logic. 
 * @param nBufs The number of internal controller buffers to read. There are 64 internal controller 
 *              buffers which can be read. Has to be set to a value in the range of 1 - 64.
 * 
 * @return The number of read internal controller buffers on success.
 */
s32 sceCtrlReadBufferPositive(SceCtrlData *data, u8 nBufs);

/**
 * Obtain button data stored in the internal controller buffers. Waits for the next update interval
 * before obtaining the data. The read data is the newest transfered data into the internal controller 
 * buffers.
 * 
 * @param data Pointer to controller data structure in which button information is stored. The obtained
 *             button data is represented in negative logic.
 * @param nBufs The number of internal controller buffers to read. There are 64 internal controller 
 *              buffers which can be read. Has to be set to a value in the range of 1 - 64.
 * 
 * @return The number of read internal controller buffers on success.
 */
s32 sceCtrlReadBufferNegative(SceCtrlData *data, u8 nBufs);

/**
 * Extended ::sceCtrlPeekBufferPositive. See description for more info.
 * You need to call ::SceCtrlExtendInternalCtrlBuffers before use.
 * 
 * @param arg1 Pass 1 or 2.
 * @param data Pointer to controller data structure in which button information is stored. The obtained
 *             button data is represented in positive logic.
 * @param nBufs The number of internal controller buffers to read. There are 64 internal controller 
 *              buffers which can be read. Has to be set to a value in the range of 1 - 64.
 * 
 * @return The number of read internal controller buffers on success.
 */
s32 sceCtrlPeekBufferPositiveExtra(s32 arg1, SceCtrlDataExt *data, u8 nBufs);

/**
 * Extended ::sceCtrlPeekBufferNegative. See description for more info. 
 * You need to call ::sceCtrlExtendInternalCtrlBuffers before use.
 * 
 * @param arg1 Unknown. Pass 1 or 2.
 * @param data Pointer to controller data structure in which button information is stored. The obtained
 *             button data is represented in negative logic.
 * @param nBufs The number of internal controller buffers to read. There are 64 internal controller 
 *              buffers which can be read. Has to be set to a value in the range of 1 - 64.
 * 
 * @return The number of read internal controller buffers on success.
 */
s32 sceCtrlPeekBufferNegativeExtra(s32 arg1, SceCtrlDataExt *data, u8 nBufs);

/**
 * Extended ::sceCtrlReadBufferPositive. See description for more info.
 * You need to call ::sceCtrlExtendInternalCtrlBuffers before use.
 * 
 * @param arg1 Pass 1 or 2.
 * @param data Pointer to controller data structure in which button information is stored. The obtained
 *             button data is represented in positive logic.
 * @param nBufs The number of internal controller buffers to read. There are 64 internal controller 
 *              buffers which can be read. Has to be set to a value in the range of 1 - 64.
 * 
 * @return The number of read internal controller buffers on success.
 */
s32 sceCtrlReadBufferPositiveExtra(s32 arg1, SceCtrlDataExt *data, u8 nBufs);

/**
 * Extended ::sceCtrlReadBufferNegative. See description for more info.
 * You need to call ::sceCtrlExtendInternalCtrlBuffers before use.
 * 
 * @param arg1 Pass 1 or 2.
 * @param data Pointer to controller data structure in which button information is stored. The obtained
 *             button data is represented in negative logic.
 * @param nBufs The number of internal controller buffers to read. There are 64 internal controller 
 *              buffers which can be read. Has to be set to a value in the range of 1 - 64.
 * 
 * @return The number of read internal controller buffers on success.
 */
s32 sceCtrlReadBufferNegativeExtra(s32 arg1, SceCtrlDataExt *data, u8 nBufs);

/**
 * Disable a rapid-fire button event.
 * 
 * @param slot The slot of the event to clear. Between 0 - 15.
 * 
 * @return 0 on success
 */
s32 sceCtrlClearRapidFire(u8 slot);

/**
 * Specify a rapid-fire event for one or more buttons.
 * 
 * @param slot The slot used to set the custom values. Between 0 - 15. Multiple slots can be used.
 * @param eventSupportButtons The buttons which potentially can trigger the rapid fire event. This
 *                            usage is restricted for now. In order for the "eventTriggerButtons"
 *                            to trigger the event, they need to be included in these buttons. 
 *                            One or more buttons of ::SceCtrlPadButtons. 
 * @param eventTriggerButtons The buttons which will start the rapid fire event for the specified 
 *                            buttons when being pressed.
 * @param buttons The buttons on which the rapid fire event will be applied to. User mode 
 *                buttons only.
 * @param eventOnTime The number of consecutive internal controller buffer updates the buttons will 
 *                    be set to ON.  It will only be applied for the first ON period of a (not canceled) 
 *                    rapid fire event. This "ON-time" will only be applied in the beginning of every new 
 *                    fired event (not being fired immediately before). Set to 0 - 63.
 * @param buttonsOnTime      The number of consecutive internal controller buffer updates the 
 *                           buttons will be set to ON (pressed).  This "ON-time" is set after 
 *                           eventOnTime was applied and the reqButton was turned off. It will be 
 *                           applied for as long as the same rapid fire event is called without a 
 *                           break (i.e. pressing of a different PSP button). Set to 0 - 63. 
 *                           If set to 0, the reqButton will be turned ON for one internal controller 
 *                           buffer update. 
 * @param buttonsOffTime     The number of consecutive internal controller buffer updates the 
 *                           buttons will be set to ON. This "OFF-time" is set after eventOnTime was 
 *                           applied. It will be applied as long as the same rapid fire event is called 
 *                           without a break (i.e. the pressing of a different PSP button). Set to 0 - 63. 
 *                           If  set to 0, the reqButton will be turned OFF for 64 consecutive internal 
 *                           controller buffer updates.
 * 
 * @return 0 on success.
 * 
 * @par Example:
 * @code
 * //A rapid fire event for the R-button while the D-Pad-Up button is being pressed.
 * //R-button will be turned ON and OFF for 64 internal controller buffer updates in both cases 
 * //(as long as D-Pad-Up is pressed).
 * sceCtrlSetRapidFire(0, 0xFF, SCE_CTRL_UP, SCE_CTRL_RTRIGGER, 63, 63, 63);
 * 
 * //A rapid fire event for the R-button while the D-Pad-Up button is being pressed.
 * //R-button will be turned OFF and ON for 40 internal controller buffer updates in both cases 
 * //(as long as D-Pad-Up is pressed).
 * sceCtrlSetRapidFire(0, SCE_CTRL_UP, SCE_CTRL_UP, SCE_CTRL_RTRIGGER, 0, 40, 40);
 * @endcode
 * 
 */
s32 sceCtrlSetRapidFire(u8 slot, u32 eventSupportButtons, u32 eventTriggerButtons, u32 buttons, u8 eventOnTime, 
                        u8 buttonsOnTime, u8 buttonsOffTime);

/**
 * Emulate values for the analog pad's X- and Y-axis.
 * 
 * @param slot The slot used to set the custom values. Between 0 - 3. If multiple slots are used, 
 *             their settings are combined.
 * @param aX New emulated value for the X-axis. Between 0 - 255.
 * @param aY New emulate value for the Y-axis. Between 0 - 255.
 * @param bufUpdates Specifies for how many updates of the internal controller buffers the emulation 
 *                   data will be applied for.
 * 
 * @return 0 on success.
 */
s32 sceCtrlSetAnalogEmulation(u8 slot, u8 aX, u8 aY, u32 bufUpdates);

/**
 * Emulate buttons for the digital pad.
 * 
 * @param slot The slot used to set the custom values. Between 0 - 3. If multiple slots are used, 
 *             their settings are combined.
 * @param userButtons Emulated user buttons of ::SceCtrlPadButtons. You cannot emulate kernel 
 *                    buttons and the emulated buttons will only be applied for applications 
 *                    running in user mode. 
 * @param kernelButtons Emulated buttons of ::SceCtrlPadButtons (you can emulate both user and 
 *                      kernel buttons). The emulated buttons will only be applied for applications 
 *                      running in kernel mode.
 * @param bufUpdates Specifies for how many updates of the internal controller buffers the emulation 
 *                   data will be applied for.
 * 
 * @return 0 on success.
 */
s32 sceCtrlSetButtonEmulation(u8 slot, u32 userButtons, u32 kernelButtons, u32 bufUpdates);

/**
 * Get the button mask settings applied to PSP buttons.
 * 
 * @param buttons The buttons to check for. One or more buttons of ::SceCtrlPadButtons.
 * 
 * @return The button mask mode for the given buttons. One of ::SceCtrlPadButtonMaskMode. 
 */
u32 sceCtrlGetButtonIntercept(u32 buttons);

/**
 * Set a button mask mode for one or more buttons. You can only mask user mode buttons in user applications.
 * Masking of kernel mode buttons is ignored as well as buttons used in kernel mode applications.
 * 
 * @param buttons The button value for which the button mask mode will be applied for. 
 *                One or more buttons of ::SceCtrlPadButtons.
 * @param buttonMaskMode Specifies the type of the button mask. One of ::SceCtrlPadButtonMaskMode.
 * 
 * @return The previous button mask type for the given buttons. One of ::SceCtrlPadButtonMaskMode.
 * 
 * @par Example:
 * @code
 * //Block user mode buttons for User mode applications
 * sceCtrlSetButtonIntercept(0xFFFF, SCE_CTRL_MASK_IGNORE_BUTTONS);
 * //Do something
 * 
 * //Remove block from user mode buttons for User mode applications
 * sceCtrlSetButtonIntercept(0xFFFF, SCE_CTRL_MASK_NO_MASK);
 * @endcode
 */
u32 sceCtrlSetButtonIntercept(u32 buttons, u32 buttonMaskMode);

/**
 * Register a button callback.
 * 
 * @param slot The slot used to register the callback. Between 0 - 3.
 * @param buttonMask Bitwise OR'ed button values which will be checked for being pressed. One or more 
 *                   buttons of ::SceCtrlPadButtons.
 * @param callback Pointer to the callback function handling the button callbacks.
 * @param opt Optional user argument. Passed to the callback function as its third argument.
 * 
 * @return 0 on success.
 */
s32 sceCtrlSetSpecialButtonCallback(u32 slot, u32 buttonMask, SceKernelButtonCallbackFunction callback, void *opt);

/**
 * Unknown purpose. 
 * 
 * @param arg1 Unknown argument.
 * 
 * @return 0.
 */
u32 sceCtrl_driver_6C86AF22(s32 arg1);

/**
 * Unknown purpose.
 * 
 * @param arg1 Unknown argument.
 * 
 * @return 0.
 */
u32 sceCtrl_driver_5886194C(s8 arg1);

/**
 * Unknown purpose.
 * 
 * @return 0.
 */
u32 sceCtrlUpdateCableTypeReq(void);

#endif	/* CTRL_H */

/** @} */
