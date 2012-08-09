/* Copyright (C) 2011, 2012 The uOFW team
   See the file COPYING for copying permission.
*/

/** 
 * ctrl.h
 *
 * @author _Felix_
 * @version 6.60
 * 
 * Controller libraries of the SCE PSP system.
 */

#include "common.h"

/** @defgroup Controller Controller libraries
 *
 * @{	
 */

#ifndef CTRL_H
#define	CTRL_H

/** The callback function used by ::sceCtrlSetSpecialButtonCallback. */
typedef void (*SceCtrlCb)(u32 currBtns, u32 lastBtns, void *opt);

/** Type definition for improved style. */
typedef u8   SceCtrlPadPollMode;
/** Type definition for improved style. */
typedef u8   SceCtrlPadButtonMaskMode;

/** General information about an internal PSP controller buffer. Including current pressed button(s) and current position 
 *  of the analog controller. 
 */
typedef struct {
    /** The time, how long the D-Pad & the Analog-Pad have been active. Time unit is in microseconds. 
     *  Can be used to get the time period of a button pressing event. 
     */ 
    u32 activeTime; //0
    /** The currently pressed D-Pad button(s). Bitwise OR'ed values of ::sceCtrlPadButtons. */
    u32 buttons; //4
    /** Analog Stick X-axis offset (0 - 255). Left = 0, Right = 255. */
    u8 aX; //8
    /** Analog Stick Y-axis offset (0 - 255). Up = 0, Down = 255. */
    u8 aY; //9
    /** Reserved. Values are normally set to 0. */
    u8 rsrv[6]; //10
} SceCtrlData; //Size of SceCtrlData: 16

/** Extended SceCtrlData structure. */
typedef struct {
    /** The time, how long the D-Pad & the Analog-Pad have been active. Time unit is in microseconds. 
     *  Can be used to get the time period if a button pressing. 
     */ 
    u32 activeTime; //0
    /** The currently pressed D-Pad button(s). Bitwise OR'ed values of ::sceCtrlPadButtons. */
    u32 buttons; //4
    /** Analog Stick X-axis offset (0 - 255). Left = 0, Right = 255. */
    u8 aX; //8
    /** Analog Stick Y-axis offset (0 - 255). Up = 0, Down = 255. */
    u8 aY; //9
    /** Reserved. Values are normally set to 0. */
    u8 rsrv[6]; //10
    /** Unknown. */
    s32 unk1; //16
    /** Unknown. */
    s32 unk2; //20
    /** Unknown. */
    s32 unk3; //24
    /** Unknown. */
    s32 unk4; //28
    /** Unknown. */
    s32 unk5; //32
    /** Unknown. */
    s32 unk6; //36
    /** Unknown. */
    s32 unk7; //40
    /** Unknown. */
    s32 unk8; //44
} SceCtrlDataExt; //Size of SceCtrlDataExt: 48

/** Status attributes of a button. Each structure member represents an active button
 *  through its button value (one of ::sceCtrlPadButtons).
 */
typedef struct {
    /** Button is newly pressed (was not already been pressed). */
    u32 btnMake; //0
    /** Stop of button press. */
    u32 btnBreak; //4
    /** Button is pressed. */
    u32 btnPress; //8
    /** Button is not pressed. */
    u32 btnRelease; //12
} SceCtrlLatch; //Size of SceCtrlLatch: 16

/** Custom function name. */
s32 sceCtrl_5A36B1C2(s32 arg1, SceCtrlDataExt *pad, u8 reqBufReads)__attribute__((alias("sceCtrlPeekBufferPositiveExt")));       
/** Custom function name. */
s32 sceCtrl_239A6BA7(s32 arg1, SceCtrlDataExt *pad, u8 reqBufReads)__attribute__((alias("sceCtrlPeekBufferNegativeExt")));  
/** Custom function name. */
s32 sceCtrl_1098030B(s32 arg1, SceCtrlDataExt *pad, u8 reqBufReads)__attribute__((alias("sceCtrlReadBufferPositiveExt"))); 
/** Custom function name. */
s32 sceCtrl_7C3675AB(s32 arg1, SceCtrlDataExt *pad, u8 reqBufReads)__attribute__((alias("sceCtrlReadBufferNegativeExt"))); 
/** Custom function name. */
s32 sceCtrl_driver_E467BEC8(u8 mode, s32 arg2, s32 arg3)__attribute__((alias("sceCtrlExtendInternalCtrlBuffers")));

/**
 * Enumeration for the digital controller buttons.
 *
 * @note PSP_CTRL_HOME, PSP_CTRL_WLAN_UP, PSP_CTRL_REMOTE, PSP_CTRL_VOLUP, PSP_CTRL_VOLDOWN, PSP_CTRL_SCREEN, PSP_CTRL_NOTE, PSP_CTRL_DISC, 
 *       PSP_CTRL_MS can only be read in kernel mode.
 */
enum sceCtrlPadButtons {
    /** Select button. Negative value = 0xFFFFFFFE. */
    SCE_CTRL_SELECT     = 0x1,
    /** Start button. Negative value = 0xFFFFFFF7. */
    SCE_CTRL_START      = 0x8,
    /** Up D-Pad button. Negative value = 0xFFFFFFEF. */
    SCE_CTRL_UP         = 0x10,
    /** Right D-Pad button. Negative value = 0xFFFFFFDF. */
    SCE_CTRL_RIGHT      = 0x20,
    /** Down D-Pad button. Negative value = 0xFFFFFFBF. */
    SCE_CTRL_DOWN       = 0x40,
    /** Left D-Pad button. Negative value = 0xFFFFFF7F. */
    SCE_CTRL_LEFT       = 0x80,
    /** Left trigger. Negative value = 0xFFFFFEFF. */
    SCE_CTRL_LTRIGGER   = 0x100,
    /** Right trigger. Negative value = 0xFFFFFDFF. */
    SCE_CTRL_RTRIGGER   = 0x200,
    /** Triangle button. Negative value = 0xFFFFEFFF. */
    SCE_CTRL_TRIANGLE   = 0x1000,
    /** Circle button. Negative value = 0xFFFFDFFF. */
    SCE_CTRL_CIRCLE     = 0x2000,
    /** Cross button. Negative value = 0xFFFFBFFF. */
    SCE_CTRL_CROSS      = 0x4000,
    /** Square button. Negative value = 0xFFFF7FFF. */
    SCE_CTRL_SQUARE     = 0x8000,
    /** Home button. In user mode this bit is set if the exit dialog is visible.  Negative value = 0xFFFEFFFF. */
    SCE_CTRL_HOME       = 0x10000,
    /** Hold button. Negative value = 0xFFFDFFFF. */
    SCE_CTRL_HOLD       = 0x20000,
    /** W-LAN switch up. Negative value = 0xFFFBFFFF. */
    SCE_CTRL_WLAN_UP    = 0x40000,
    /** Remote hold position. Negative value = 0xFFF7FFFF. */
    SCE_CTRL_REMOTE     = 0x80000,
    /** Volume up button. Negative value = 0xFFEFFFFF. */
    SCE_CTRL_VOLUP      = 0x100000,
    /** Volume down button. Negative value = 0xFFBFFFFF. */
    SCE_CTRL_VOLDOWN    = 0x200000,
    /** Screen button. Negative value = 0xFFBFFFFF. */
    SCE_CTRL_SCREEN     = 0x400000,
    /** Music Note button. Negative value = 0xFF7FFFFF. */
    SCE_CTRL_NOTE       = 0x800000,   	
    /** Disc present. Negative value = 0xFEFFFFFF. */
    SCE_CTRL_DISC       = 0x1000000,
    /** Memory stick present. Negative value = 0xFDFFFFFF. */
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
enum sceCtrlPadPollMode {
    /** No controller input is recognized. */
    SCE_CTRL_POLL_KILLED = 0,
    /** Controller input is recognized. */
    SCE_CTRL_POLL_RUNNING = 1,
};

/** Button mask settings. */
enum sceCtrlPadButtonMaskMode {
    /** Remove any masks involving the specified buttons. */
    SCE_CTRL_MASK_NO_MASK = 0,
    /** Block the specified buttons (their button status (pressed/un-pressed) won't be recognized). 
     *  You can only block user buttons for applications running in User Mode. */
    SCE_CTRL_MASK_IGNORE_BUTTONS = 1,
    /** Set the specified buttons (they will be simulated as being pressed). 
     *  You can only set user buttons for applications running in User Mode. */
    SCE_CTRL_MASK_SET_BUTTONS = 2,
};

/**
 * Initialize the controller device. Bind the controller driver to the controller device.
 * 
 * @return 0 on success, otherwise < 0.
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
 * Resume the controller driver after and bring the controller device 
 * back from a low-power state.
 * 
 * @return 0.
 */
s32 sceCtrlResume(void);

/**
 * Enable/disable controller device input.
 * 
 * @param pollMode One of ::sceCtrlPadPollMode. If set to 0, no button/analog input is recognized.
 *                 Set to 1 to enable button/analog input.
 * 
 * @return 0.
 */
s32 sceCtrlSetPollingMode(SceCtrlPadPollMode pollMode);

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
 * @return The previous input mode on success, otherwise less than 0.
 */
s32 sceCtrlSetSamplingMode(u8 mode);

/**
 * Get the current cycle specifying the update frequency of the internal controller data buffers.
 * 
 * @param cycle Receiving the current cycle.
 * 
 * @return 0.
 */
s32 sceCtrlGetSamplingCycle(u32 *cycle);

/**
 * Set the update frequency of the internal controller buffer. Default update frequency is the VBlank-Interrupt (60 times/sec).
 * 
 * @param cycle The new time period between two samplings of controller attributes in microseconds.
 *                Setting to 0 enables the VBlank-Interrupt-Update process. If you want to set an own
 *                time period for updating the internal controller buffer(s), cycle has to be greater 5554 
 *                and less than 20001.
 *                This will disable the VBlank-Interrupt-Update process.
 * 
 * @return The previous cycle on success, otherwise less than 0.
 */
s32 sceCtrlSetSamplingCycle(u32 cycle);

/**
 * Get the idle threshold values.
 *
 * @param idlerest Movement needed by the analog to reset the idle timer.
 * @param idleback Movement needed by the analog to bring the PSP back from an idle state.
 *
 * @return 0 on success, otherwise less than 0.
 */
s32 sceCtrlGetIdleCancelThreshold(s32 *idleReset, s32 *idleBack);

/**
 * Set analog threshold relating to the idle timer.
 *
 * @param idlereset Movement needed by the analog to reset the idle timer.
 * @param idleback Movement needed by the analog to bring the PSP back from an idle state.
 *
 * Set to -1 for analog to not cancel idle timer.
 * Set to 0 for idle timer to be canceled even if the analog is not moved.
 * Set between 1 - 128 to specify the movement on either axis needed by the analog to fire the event.
 *
 * @return 0 on success, otherwise less than 0.
 */
s32 sceCtrlSetIdleCancelThreshold(s32 idlereset, s32 idleback);

/**
 * Get the number of VBlanks for which will be waited when terminating the controller library.
 * 
 * @return The number of VBlanks.
 */
s16 sceCtrlGetSuspendingExtraSamples(void);

/**
 * Set a number of VBlanks for which will be waited when terminating the controller library.
 * 
 * @param suspendSamples The number of VBlanks. Between 0 - 300.
 * 
 * @return 0 on success, otherwise less than 0.
 */
s32 sceCtrlSetSuspendingExtraSamples(s16 suspendSamples);

typedef struct _ctrlUnkStruct {
    s32 unk1;
    s32 (*func)(int);
} ctrlUnkStruct;

/**
 * Extend the 64 internal controller buffers to represent SceCtrlDataExt structures.
 * By default, an internal controller buffer is equivalent to a SceCtrlData structure. This function has to be called before using
 * the extended read/peekBuffer functions.
 * 
 * @param mode Seems to be an index. Pass either 1 or 2.
 * @param arg2 Pointer to a ctrlUnkStruct structure?
 * @param arg3 Unknown.
 * 
 * @return 0 on success, otherwise less than 0.
 */
s32 sceCtrlExtendInternalCtrlBuffers(u8 mode, ctrlUnkStruct *arg2, s32 arg3);

/**
 * Read the current internal latch buffer. The following button states are delivered:
 *                                         Button is pressed, button is not pressed, button has been newly pressed
 *                                         and button has been newly released. 
 * Once a button has been i.e. pressed, its value is stored into the specific internal latch buffer member (uiMake in this case)
 * until you manually reset the specific latch buffer field.
 * 
 * @param latch Pointer to a SceCtrlLatch structure retrieving the current internal latch buffer.
 * 
 * @return The amount of reads of the internal latch buffer, without being reseted, on success, otherwise less than 0.
 * 
 * @par Example:
 * @code
 * SceCtrlLatch latch_data;
 * 
 * sceCtrlPeekLatch(&latch_data);
 * while(1) {
 *           //Cross button pressed
 *           if (latch_data.btnPress & PSP_CTRL_CROSS) {
 *               //do something
 *           }
 * }
 * @endcode
 * 
 */
s32 sceCtrlPeekLatch(SceCtrlLatch *latch);

/**
 * Read the current internal latch buffer and reset the buffer afterwards. The following button states are delivered:
 *                                                Button is pressed, button is not pressed, button has been newly pressed
 *                                                and button has been newly released. 
 * After the internal latch buffer has been read, it will be cleaned (all members will be reset to zero).
 * 
 * @param latch Pointer to a SceCtrlLatch structure retrieving the current internal latch buffer.
 * 
 * @return The amount of reads of the internal latch buffer without being reseted (typically 1) on success, otherwise less than 0.
 */
s32 sceCtrlReadLatch(SceCtrlLatch *latch);

/**
 * Read the current internal controller buffer. Does not wait for the next VBlank.
 * 
 * @param pad Pointer to a SceCtrlData structure retrieving the current internal controller buffer.
 * @param count The number of internal buffers to read. There are 64 internal controller buffers which can be read.
 *              Has to be set to a value in the range of 1 - 64.
 * 
 * @return The amount of read internal controller buffers, otherwise less than 0.
 * 
 * @par Example:
 * @code
 * SceCtrlData pad;

 * sceCtrlSetSamplingCycle(0);
 * sceCtrlSetSamplingMode(1);
 * 
 * while(1) {
 *           sceCtrlPeekBufferPositive(&pad, 1); 
 *           //Cross button pressed
 *           if (pad.Buttons & PSP_CTRL_CROSS) {
 *               //do something
 *           }
 * }
 * @endcode
 */
s32 sceCtrlPeekBufferPositive(SceCtrlData *pad, u8 reqBufReads);

/**
 * Read the current internal SceCtrlData buffer. Does not wait for the next VBlank.
 * 
 * @param pad Pointer to a SceCtrlData structure retrieving the current internal controller buffer. Negative button values have to be used. 
 *            Check ::sceCtrlPadButtons for the negative active values of the buttons. If no button is active, the internal
 *            button value is 0xFFFFFFFF.
 * @param reqBufReads The number of internal buffers to read. There are 64 internal controller buffers which can be read.
 *                    Has to be set to a value in the range of 1 - 64.
 * 
 * @return The amount of read internal controller buffers, otherwise less than 0.
 * 
 * @par Example:
 * @code
 * SceCtrlData pad;

 * sceCtrlSetSamplingCycle(0);
 * sceCtrlSetSamplingMode(1);
 * 
 * while(1) {
 *           sceCtrlPeekBufferNegative(&pad, 1); 
 *           //Cross button pressed
 *           if (pad.Buttons & ~PSP_CTRL_CROSS) {
 *               //do something
 *           }
 * }
 * @endcode
 */
s32 sceCtrlPeekBufferNegative(SceCtrlData *pad, u8 reqBufReads);

/**
 * Read the current internal SceCtrlData buffer. By default, the internal controller buffer will be read after every VSYNC period (60 times/sec).
 * You can set your own update timer by calling ::sceCtrlSetSamplingCycle.
 * 
 * @param pad Pointer to a SceCtrlData structure retrieving the current internal button buffer. 
 * @param reqBufReads The number of internal buffers to read. There are 64 internal controller buffers which can be read.
 *                    Has to be set to a value in the range of 1 - 64.
 * 
 * @return The amount of read internal controller buffers on success, otherwise less than 0.
 */
s32 sceCtrlReadBufferPositive(SceCtrlData *pad, u8 reqBufReads);

/**
 * Read the current internal SceCtrlData buffer. By default, the internal controller buffer will be read after every VSYNC period (60 times/sec).
 * You can set your own update time by calling ::sceCtrlSetSamplingCycle.
 * 
 * @param pad Pointer to a SceCtrlData structure retrieving the current internal controller buffer. Negative button values have to be used. 
 *            Check ::sceCtrlPadButtons for the negative active values of the buttons. If no button is active, the internal
 *            button value is 0xFFFFFFFF.
 * @param reqBufReads The number of internal buffers to read. There are 64 internal controller buffers which can be read.
 *                    Has to be set to a value in the range of 1 - 64.
 * 
 * @return The amount of read internal controller buffers on success, otherwise less than 0.
 */
s32 sceCtrlReadBufferNegative(SceCtrlData *pad, u8 reqBufReads);

/**
 * Extended ::sceCtrlPeekBufferPositive. See description for more info.
 * You need to call ::sceCtrlExtendInternalCtrlBuffers before use.
 * 
 * @param arg1 Pass 1 or 2.
 * @param padExt Pointer to a SceCtrlData structure retrieving the current internal controller buffer.
 * @param reqBufReads. Number of requested reads of the internal controller buffers. Has to be set to a value in the range of 1 - 64.
 * 
 * @return The amount of read internal controller buffers on success, otherwise less than 0.
 */
s32 sceCtrlPeekBufferPositiveExt(s32 arg1, SceCtrlDataExt *padExt, u8 reqBufReads);

/**
 * Extended ::sceCtrlPeekBufferNegative. See description for more info. 
 * You need to call ::sceCtrlExtendInternalCtrlBuffers before use.
 * 
 * @param arg1 Unknown. Pass 1 or 2.
 * @param padExt Pointer to a SceCtrlData structure retrieving the current internal controller buffer.
 * @param reqBufReads. Number of requested reads of the internal controller buffers. Has to be set to a value in the range of 1 - 64.
 * 
 * @return The amount of read internal controller buffers, otherwise less than 0.
 */
s32 sceCtrlPeekBufferNegativeExt(s32 arg1, SceCtrlDataExt *padExt, u8 reqBufReads);

/**
 * Extended ::sceCtrlReadBufferPositive. See description for more info.
 * You need to call ::sceCtrlExtendInternalCtrlBuffers before use.
 * 
 * @param arg1 Pass 1 or 2.
 * @param padExt Pointer to a SceCtrlData structure retrieving the current internal controller buffer.
 * @param reqBufReads. Number of requested reads of the internal controller buffers. Has to be set to a value in the range of 1 - 64.
 * 
 * @return The amount of read internal controller buffers, otherwise less than 0.
 */
s32 sceCtrlReadBufferPositiveExt(s32 arg1, SceCtrlDataExt *padExt, u8 reqBufReads);

/**
 * Extended ::sceCtrlReadBufferNegative. See description for more info.
 * You need to call ::sceCtrlExtendInternalCtrlBuffers before use.
 * 
 * @param arg1 Pass 1 or 2.
 * @param padExt Pointer to a SceCtrlData structure retrieving the current internal controller buffer.
 * @param reqBufReads. Number of requested reads of the internal controller buffers. Has to be set to a value in the range of 1 - 64.
 * 
 * @return The amount of read internal controller buffers, otherwise less than 0.
 */
s32 sceCtrlReadBufferNegativeExt(s32 arg1, SceCtrlDataExt *padExt, u8 reqBufReads);

/**
 * Clear a rapid-fire-button slot.
 * 
 * @param slot The slot to clear. Between 0 - 15.
 * 
 * @return 0 on success, otherwise less than 0.
 */
s32 sceCtrlClearRapidFire(u8 slot);

/**
 * Set a pressed/un-pressed period for a button.
 * 
 * @param slot The slot used to set the custom values. Between 0 - 15. Multiple slots can be used.
 * @param pressedBtnRange The pressed-button-range to check for. One or more buttons of ::sceCtrlPadButtons. 
 * @param reqBtnsEventTrigger The button(s) which will fire the pressed/un-pressed period for a button/buttons when being pressed. 
 *        Has to be included in pressedBtnRange.
 * @param reqBtn The requested button(s) on which the pressed/un-pressed period (the rapid fire event) will be applied to. 
 *               User mode buttons ONLY.
 * @param reqBtnEventOnTime For how many (consecutive) internal controller buffer updates the requested button(s) will be set to ON (pressed).
 *                          This "ON-time" will only be applied in the beginning of every new fired event 
 *                          (not being fired immediately before). Set to 0 - 63.
 * @param reqBtnOnTime      For how many (consecutive) internal controller buffer updates the requested button(s) will be set to ON (pressed). 
 *                          This "ON-time" is set after reqBtnEventOnTime was applied and the reqButton was turned off. 
 *                          It will be applied for as long as the same rapid fire event is called without a break
 *                          (i.e. the pressing of a different PSP button). Set to 0 - 63. If set to 0, the reqButton will be turned ON
 *                          for 1 internal controller buffer update. 
 * @param reqBtnOffTime     For how many (consecutive) internal controller buffer updates the requested button(s) will be set to OFF (un-pressed). 
 *                          This "OFF-time" is set after reqBtnEventOnTime was applied. 
 *                          It will be applied for as long as the same rapid fire event is called without a break
 *                          (i.e. the pressing of a different PSP button). Set to 0 - 63. If  set to 0, the reqButton will be turned OFF
 *                          for 64 (consecutive) internal controller buffer updates.
 * 
 * @Note Please choose values for reqBtnEventOnTime, reqBtnOnTime and reqBtnOffTime which are, when being bitwise OR'ed together, < 64.
 *       Otherwise, you will get an error. 
 * 
 * @return 0 on success, otherwise less than 0.
 * 
 * @par Example:
 * @code
 * //A rapid fire event for the R-button while the D-Pad-Up button is being pressed.
 * //R-button will be turned ON and OFF for 64 internal ctrl buffer updates in both cases (as long as D-Pad-Up is pressed).
 * sceCtrlSetRapidFire(0, 0xFF, PSP_CTRL_UP, PSP_CTRL_RTRIGGER, 63, 63, 63);
 * 
 * //A rapid fire event for the R-button while the D-Pad-Up button is being pressed.
 * //R-button will be turned OFF and ON for 40 internal ctrl buffer updates in both cases (as long as D-Pad-Up is pressed).
 * sceCtrlSetRapidFire(0, 0xFF, PSP_CTRL_UP, PSP_CTRL_RTRIGGER, 0, 40, 40);
 * @endcode
 * 
 */
s32 sceCtrlSetRapidFire(u8 slot, u32 pressedBtnRange, u32 reqBtnsEventTrigger, u32 reqBtn, u8 reqBtnEventOnTime, u8 reqBtnOnTime, u8 reqBtnOffTime);

/**
 * Emulate values for the analog pad's X- and Y-axis.
 * 
 * @param slot The slot used to set the custom values. Between 0 - 3. If multiple slots are used, their settings are bitwise OR'ed together.
 * @param aXEmu New emulated value for the X-axis. Between 0 - 255.
 * @param aYEmu New emulate value for the Y-axis. Between 0 - 255.
 * @param bufUpdates Specifies for how many updates of the internal controller buffers the custom values will be applied for.
 * 
 * @return 0 on success, otherwise less than 0.
 */
s32 sceCtrlSetAnalogEmulation(u8 slot, u8 aXEmu, u8 aYEmu, u32 bufUpdates);

/**
 * Emulate buttons for the digital pad.
 * 
 * @param slot The slot used to set the custom values. Between 0 - 3. If multiple slots are used, their settings are bitwise OR'ed together.
 * @param uModeBtnEmu Emulated user buttons of ::sceCtrlPadButtons. You cannot emulate kernel buttons and 
 *                    the emulated buttons will only be applied for applications running in user mode. 
 * @param kModeBtnEmu Emulated buttons of ::sceCtrlPadButtons (you can emulate both user and kernel buttons).  
 *                    The emulated buttons will only be applied for applications running in kernel mode.
 * @param bufUpdates Specifies for how many updates of the internal controller buffers the custom values will be applied for.
 * 
 * @return 0 on success, otherwise less than 0.
 */
s32 sceCtrlSetButtonEmulation(u8 slot, u32 uModeBtnEmu, u32 kModeBtnEmu, u32 bufUpdates);

/**
 * Get the button mask mode of one or more bitwise OR'ed PSP buttons.
 * 
 * @note In the PSPSDK, this function is defined as sceCtrlGetButtonMask.
 * 
 * @param buttons The button(s) to check for (one or more buttons of ::sceCtrlPadButtons)
 *                an existing button mask mode applying to it/them.
 * 
 * @return The button mask mode for the given button(s). One of ::sceCtrlPadButtonMaskMode. 
 *         SCE_CTRL_MASK_DELETE_BUTTON_MASK_SETTING (0), if btnMask (or parts of it) is/are included in the currently set button mask.
 *         SCE_CTRL_MASK_IGNORE_BUTTON_MASK (1), if btnMask is not included in the current button mask.
 *         SCE_CTRL_MASK_SET_BUTTON_MASK (2), if btnMask (or parts of it) are set to ON by the current set button mask.
 */
SceCtrlPadButtonMaskMode sceCtrlGetButtonIntercept(u32 buttons);

/**
 * Set a button mask mode for one or more buttons. You can only mask user mode buttons in user applications.
 * Masking of kernel mode buttons is ignored as well as buttons used in kernel mode applications.
 * 
 * @note In the PSPSDK, this function is defined as sceCtrlSetButtonMask.
 * 
 * @param buttons The button value for which the button mask mode will be applied for. 
 *                One or more buttons of ::sceCtrlPadButtons.
 * @param btnMaskMode Specifies the mask mode of the button mask. One of ::sceCtrlPadButtonMaskMode.
 *  
 *        btnMaskMode = SCE_CTRL_MASK_IGNORE_BUTTONS:
 *              Delete the specified buttons from the supported buttons 
 *              and remove them from the 'set to ON' buttons.
 *        btnMaskMode = SCE_CTRL_MASK_SET_BUTTONS:
 *              Add the specified buttons to both the supported buttons
 *              and to the 'set to ON' buttons.
 *        btnMaskMode = SCE_CTRL_MASK_NO_MASK:
 *              Add the specified buttons to the supported buttons 
 *              and remove them from the 'set to ON' buttons.
 * 
 * @return The button mask mode regarding the new btnMask compared with the previously set btnMask.   
 *         SCE_CTRL_MASK_DELETE_BUTTON_MASK_SETTING (0) for the new btnMask (or parts of it) already being supported by the previously set btnMask,
 *         SCE_CTRL_MASK_IGNORE_BUTTON_MASK (1) for the new btnMask already being ignored by the previously btnMask or
 *         SCE_CTRL_MASK_SET_BUTTON_MASK (2) for the new btnMask (or parts of being) being already being set to ON
 *         by the previously set btnMask.
 * 
 * @par Example:
 * @code
 * //Block user mode buttons for User mode applications
 * sceCtrlSetButtonIntercept(0xFFFF, 1);
 * //Do something
 * 
 * //Remove block from user mode buttons for User mode applications
 * sceCtrlSetButtonIntercept(0xFFFF, 0);
 * @endcode
 */
SceCtrlPadButtonMaskMode sceCtrlSetButtonIntercept(u32 buttons, SceCtrlPadButtonMaskMode btnMaskMode);

/**
 * Register a button callback.
 * 
 * @note In the PSPSDK, this function is defined as sceCtrlRegisterButtonCallback.
 * 
 * @param slot The slot used to register the callback (0-3).
 * @param btnMask Bitwise OR'ed button values which will be checked for being pressed. One or more buttons of ::sceCtrlPadButtons.
 * @param cbFunc Pointer to the callback function (s32 currBtns, s32 lastBtns, void *opt), which handles button input effects.
 * @param opt Optional user argument. Passed to the callback function as its third argument.
 * 
 * @return 0 on success, otherwise less than 0.
 */
s32 sceCtrlSetSpecialButtonCallback(u32 slot, u32 btnMask, SceCtrlCb cbFunc, void *opt);

#endif	/* CTRL_H */

/** @} */
