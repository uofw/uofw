/** Copyright (C) 2011 - 2015 The uOFW team
   See the file COPYING for copying permission.
*/

#include "common_header.h"

/** @defgroup Controller Controller Module
 *  Controller (joystick) management.
 * @{	
 */

#ifndef CTRL_H
#define	CTRL_H

/** The callback function used by ::sceCtrlSetSpecialButtonCallback(). */
typedef void (*SceKernelButtonCallbackFunction)(u32 curButtons, u32 lastButtons, void *opt);

/** 
 * This structure is for obtaining button data (button/analog stick information) from the 
 * controller using ::sceCtrlPeekBufferPositive(), ::sceCtrlReadBufferNegative() etc...
 */
typedef struct {
    /** 
     * The time stamp of the time during which sampling was performed. Time unit is microseconds. 
     * Can be used to get the time period of a button pressing event. 
     */ 
    u32 timeStamp;
    /** The currently pressed button. Bitwise OR'ed values of ::SceCtrlPadButtons. */
    u32 buttons;
    /** Analog Stick X-axis offset (0 - 0xFF). Left = 0, Right = 0xFF. */
    u8 aX;
    /** Analog Stick Y-axis offset (0 - 0xFF). Up = 0, Down = 0xFF. */
    u8 aY;
    /** DS3 right analog x-axis. Filled with 0 if input source doesn't allow second analog input. */
    u8 rX;
    /** DS3 right analog y-axis. Filled with 0 if input source doesn't allow second analog input. */
    u8 rY;
    /** Reserved. */
    u8 rsrv[4];
} SceCtrlData;

/** 
 * This structure is for obtaining button data (button/analog stick information) from the 
 * controller using ::sceCtrlPeekBufferPositive2(), ::sceCtrlReadBufferNegative2() etc....
 * In addition to PSP controller state it can contain input state of external input devices 
 * such as a wireless controller.
 */
typedef struct {
    /** 
     * The time stamp of the time during which sampling was performed. Time unit is microseconds. 
     * Can be used to get the time period of a button pressing event.
     */ 
    u32 timeStamp;
    /** The currently pressed button. Bitwise OR'ed values of ::SceCtrlPadButtons. */
    u32 buttons;
    /** Analog Stick X-axis offset (0 - 0xFF). Left = 0, Right = 0xFF. */
    u8 aX;
    /** Analog Stick Y-axis offset (0 - 0xFF). Up = 0, Down = 0xFF. */
    u8 aY;
    /** DS3 right analog x-axis. Filled with 0 if input source doesn't allow second analog input. */
    u8 rX;
    /** DS3 right analog y-axis. Filled with 0 if input source doesn't allow second analog input. */
    u8 rY;
    /** Reserved. */
    u8 rsrv[4];
    /** D-pad pressure sensitivity.
    * Byte 1: D-Pad right.
    * Byte 3: D-Pad left.    
    */
    s32 DPadSenseA;
    /** D-pad pressure sensitivity.
    * Byte 1: D-Pad up.
    * Byte 3: D-Pad down.    
    */
    s32 DPadSenseB;
    /** Gamepad pressure sensitivity.
    * Byte 1: Triangle.
    * Byte 3: Circle.    
    */
    s32 GPadSenseA;
    /** Gamepad pressure sensitivity.
    * Byte 1: Cross.
    * Byte 3: Square.    
    */
    s32 GPadSenseB;
    /** Axis pressure sensitivity.
    * Byte 1: L1.
    * Byte 3: R1.    
    */
    s32 AxisSenseA;
    /** Axis pressure sensitivity.
    * Byte 1: L2.
    * Byte 3: R2.    
    */
    s32 AxisSenseB;
    /** DS3 sixaxis. This is the return value for tilting the x-axis. */
    s32 TiltA;
    /** DS3 sixaxis. This is the return value for tilting the y-axis. */
    s32 TiltB;
} SceCtrlData2;

/** 
 * This structure represents controller button latch data.
 * 
 * With each sampling cycle, the controller service compares the new pressed & released button states
 * with the previously collected pressed button states. This comparison will result in the following possible
 * states for each button:
 *
 *    • [make]: The button has just been pressed with its prior state being the released state. Transition from
 *      'released' state to 'pressed' state.\n
 *    • [press]: The button is currently in the 'pressed' state.\n
 *    • [break]: The button has just been released with its prior state being the 'pressed' state. Transition from
 *      'pressed' state to 'release' state.\n
 *    • [release]: The button is currently in the 'released' state.
 * 
 * It is possible for a button to (briefly) be in two states at the same time. Valid combinations are as follows:
 * 
 *    • [make] & [press]\n 
 *    • [break] & [release]   
 * 
 * In other words, if a button is in the [make] state, then it is also in the [press] state. However, this is not the case
 * for the inverse. A button in the [press] state does not need to be in the [make] state.
 * 
 * These comparison results are stored internally as latch data and can be retrieved using the APIs ::sceCtrlPeekLatch() and 
 * ::sceCtrlReadLatch(). ::SceCtrlPadButtons can be used to find out the state of each button.
 * 
 * @remark The same can be accomplished by using the different sceCtrl[Read/Peek]Buffer[Positive/Negative]() APIs
 * and comparing the currently collected button sampling data with the previously collected one.
 * 
 * @see ::sceCtrlPeekLatch()
 * @see ::sceCtrlReadLatch()
 */
typedef struct {
    /** Button transitioned to pressed state. */
    u32 buttonMake;
    /** Button transitioned to released state. */
    u32 buttonBreak;
    /** Button is in the pressed state. */
    u32 buttonPress;
    /** Button is in the released state. */
    u32 buttonRelease;
} SceCtrlLatch;

/**
 * This structure is used to copy external input data into PSP internal controller buffers. 
 */
typedef struct {
    /** Unknown. Is set to 0xC by Sony. */
    s32 unk1;
    /** 
	 * Pointer to a transfer function to copy input data into a PSP internal controller buffer. 
	 * The function should return a value >= 0 on success, < 0 otherwise.
	 */
	s32 (*copyInputData)(void *pSrc, SceCtrlData2 *pDst);
} SceCtrlInputDataTransferHandler;

/**
 * Enumeration for the digital controller buttons in positive logic.
 *
 * @note SCE_CTRL_INTERCEPTED, SCE_CTRL_WLAN_UP, SCE_CTRL_REMOTE, SCE_CTRL_VOLUP, SCE_CTRL_VOLDOWN, 
 *       SCE_CTRL_SCREEN, SCE_CTRL_NOTE, SCE_CTRL_DISC, SCE_CTRL_MS can only be read in kernel mode.
 */
enum SceCtrlPadButtons {
    /** Select button. */
    SCE_CTRL_SELECT         = 0x00000001,
    /** DS3 L3 button. */
    SCE_CTRL_L3             = 0x00000002,
    /** DS3 R3 button. */
    SCE_CTRL_R3             = 0x00000004,
    /** Start button. */
    SCE_CTRL_START          = 0x00000008,
    /** Up D-Pad button. */
    SCE_CTRL_UP             = 0x00000010,
    /** Right D-Pad button. */
    SCE_CTRL_RIGHT          = 0x00000020,
    /** Down D-Pad button. */
    SCE_CTRL_DOWN           = 0x00000040,
    /** Left D-Pad button. */
    SCE_CTRL_LEFT           = 0x00000080,
    /** Left trigger. This accounts for the DS3 L2 trigger as well. */
    SCE_CTRL_LTRIGGER       = 0x00000100,
    /** Right trigger. This accounts for the DS3 R2 trigger as well. */
    SCE_CTRL_RTRIGGER       = 0x00000200,
    /** DS3 L1 trigger. */
    SCE_CTRL_L1TRIGGER      = 0x00000400,
    /** DS3 R1 trigger. */
    SCE_CTRL_R1TRIGGER      = 0x00000800,
    /** Triangle button. */
    SCE_CTRL_TRIANGLE       = 0x00001000,
    /** Circle button. */
    SCE_CTRL_CIRCLE         = 0x00002000,
    /** Cross button. */
    SCE_CTRL_CROSS          = 0x00004000,
    /** Square button. */
    SCE_CTRL_SQUARE         = 0x00008000,
    /** 
     * If this bit is set, then controller input is being intercepted by the 
     * system software or another application.  For example, this is the case 
     * when the PSP's HOME menu is being shown. 
     */
    SCE_CTRL_INTERCEPTED    = 0x00010000,
    /** Hold button. */
    SCE_CTRL_HOLD           = 0x00020000,
    /** W-LAN switch up. */
    SCE_CTRL_WLAN_UP        = 0x00040000,
    /** Remote hold position. */
    SCE_CTRL_REMOTE         = 0x00080000,
    /** Volume up button. */
    SCE_CTRL_VOLUP          = 0x00100000,
    /** Volume down button. */
    SCE_CTRL_VOLDOWN        = 0x00200000,
    /** Screen button. */
    SCE_CTRL_SCREEN         = 0x00400000,
    /** Music Note button. */
    SCE_CTRL_NOTE           = 0x00800000,   	
    /** Disc present. */
    SCE_CTRL_DISC           = 0x01000000,
    /** Memory stick present. */
    SCE_CTRL_MS             = 0x02000000,
    SCE_CTRL_UNK_10000000   = 0x10000000,
    SCE_CTRL_UNK_20000000   = 0x20000000,
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

/** Specifies the type of input data to be obtained. */
enum SceCtrlPort {
    /* Input is only obtained from the PSP. */
    SCE_CRTL_PORT_PSP = 0,
    /* Input is obtained from the PSP and a connected DualShock3 controller. */
    SCE_CTRL_PORT_DS3 = 1,
    /* Input is obtained from the PSP and an unknown connected external device. */
    SCE_CTRL_PORT_UNKNOWN_2 = 2
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

/* The center position of the analog stick on both axes. */
#define SCE_CTRL_ANALOG_PAD_CENTER_VALUE            0x80

/**
 * Initializes the controller device. Bind the controller driver to the controller device.
 * 
 * @return 0 on success.
 */
s32 sceCtrlInit(void);

/**
 * Terminates the controller device. Unbind the controller driver from the controller device.
 * 
 * @return 0.
 */
s32 sceCtrlEnd(void);

/**
 * Suspends the controller driver and put the controller device into a low-power state.
 * 
 * @return 0.
 */
s32 sceCtrlSuspend(void);

/**
 * Resumes the controller driver after and bring the controller device back from a low-power state.
 * 
 * @return 0.
 */
s32 sceCtrlResume(void);

/**
 * Enables/disables controller device input.
 * 
 * @param pollMode One of ::SceCtrlPadPollMode. If set to 0, no button/analog input is recognized.
 *                 Set to 1 to enable button/analog input.
 * 
 * @return 0.
 */
u32 sceCtrlSetPollingMode(u8 pollMode);

/**
 * Gets the current controller device input mode.
 * 
 * @param pMode Pointer to an u8 variable which is to receive the current controller mode. One of ::SceCtrlPadInputMode.
 * 
 * @return 0.
 */
u32 sceCtrlGetSamplingMode(u8 *pMode);

/**
 * Sets the controller device input mode.
 * 
 * @param mode The new controller input mode. One of ::SceCtrlPadInputMode.
 * 
 * @return The previous input mode on success.
 */
s32 sceCtrlSetSamplingMode(u8 mode);

/**
 * Gets the current update interval of the internal controller data buffers.
 * 
 * @param pCycle Pointer to an u32 variable which is to receive the current update interval (in microseconds).
 * 
 * @return 0.
 */
u32 sceCtrlGetSamplingCycle(u32 *pCycle);

/**
 * Sets the update frequency of the internal controller buffer. Default update interval 
 * is the VBlank interrupt (approximately 60 times per second).
 * 
 * @param cycle The new interval between two samplings of controller attributes in microseconds.
 *              Setting to 0 enables the VBlank-Interrupt-Update process. If you want to set an own
 *              interval for updating the internal controller buffers, cycle has to in the range of 
 *              5555 - 20000 (the range from about 180 Hz to 50 Hz).
 *              This will disable the VBlank-Interrupt-Update process.
 * 
 * @return The previous cycle on success.
 */
s32 sceCtrlSetSamplingCycle(u32 cycle);

/**
 * Obtains the different cancel-idle-timer buttons.
 * 
 * @param pResetButtonsMake Pointer to an u32 variable which is to receive the buttons resetting the idle timer when
 * being pressed a new time (not already being pressed). Not used when the system is in HOLD mode.
 * @param pResetButtonsPress Pointer to an u32 variable which is to receive the buttons resetting the idle timer when
 * being pressed. Not used when the system is in HOLD mode.
 * @param pResetButtonsMakeHoldMode Pointer to an u32 variable which is to receive the buttons resetting the idle timer when
 * being pressed a new time (not already being pressed). Used when the system is in HOLD mode.
 * @param pResetButtonsPressHoldMode Pointer to an u32 variable which is to receive the buttons resetting the idle timer when
 * being pressed a new time. Used when the system is in HOLD mode.
 * 
 * @return 0 on success.
 */
u32 sceCtrlGetIdleCancelKey(u32 *pResetButtonsMake, u32 *pResetButtonsPress, u32 *pResetButtonsMakeHoldMode, 
                            u32 *pResetButtonsPressHoldMode);

/**
 * @brief Specifies the buttons which, when being pressed, reset the idle timer. 
 * 
 * This function lets you specify buttons which reset the idle timer. If multiple buttons are specified the timer can be
 * resetted by simply pressing one of the specified buttons. A button combination is also possible.
 * 
 * @param resetButtonsMake Buttons which will reset the timer if pressed. One or more of ::SceCtrlPadButtons.
 * If you keep pressing one or more of these buttons after resetting the timer, they will not reset the timer anymore.
 * You will have to release the buttons first, before they can reset it again. In case HOLD mode is active, pressing
 * these buttons will not reset the timer.
 * @param resetButtonsPress Buttons which will reset the timer if pressed. One or more of ::SceCtrlPadButtons.
 * As long as you press one of these buttons, the timer is resetted. In case HOLD mode is active, pressing these buttons
 * will not reset the timer.
 * @param resetButtonsMakeHoldMode Buttons which will reset the timer if pressed while HOLD mode is active. One or
 * more of ::SceCtrlPadButtons. If you keep pressing these buttons after resetting the timer, they will not reset it
 * anymore. You will have to release the buttons first, before they can reset the timer again.
 * @param resetButtonsPressHoldMode Buttons which will reset the timer if pressed while HOLD mode is active. One or
 * more of ::SceCtrlPadButtons. As long as you press one of these buttons, the timer is resetted.
 * 
 * @return 0 on success.
 * 
 * @par Example:
 * @code
 * // Pressing the START button will reset the idle timer. Button needs to be released and pressed again to reset the
 * // idle timer another time. HOLD mode has to be inactive.
 * // Pressing the SELECT button will reset the idle timer. The button can be kept pressed (held down) to keep resetting
 * // the idle timer. HOLD mode has to be inactive.
 * sceCtrlSetIdleCancelKey(SCE_CTRL_START, SCE_CTRL_SELECT, 0, 0);
 * @endcode
 */
u32 sceCtrlSetIdleCancelKey(u32 resetButtonsMake, u32 resetButtonsPress, u32 resetButtonsMakeHoldMode,
                            u32 resetButtonsPressHoldMode);

/* This constant defines that analog stick movement does not cancel the idle timer. */
#define SCE_CTRL_IDLE_CANCEL_TRESHOLD_NO_CANCELLATION    (-1)

/**
 * Gets the idle timer cancel movement threshold values for the analog stick.
 *
 * @param pUnHoldThreshold Pointer to a s32 variable which is to receive the treshold value when HOLD mode is inactive.
 * @param pHoldThreshold Pointer to a s32 variable which is to receive the treshold value when HOLD mode is active.
 *
 * @return 0 on success.
 */
s32 sceCtrlGetIdleCancelThreshold(s32 *pUnHoldThreshold, s32 *pHoldThreshold);

/**
 * Sets analog stick movement threshold values for cancelling the idle timer. In case SCE_CTRL_INPUT_DIGITAL_ONLY is set as the 
 * input mode for the controller, analog stick movements will not result in cancelling the idle timer.
 *
 * @param unHoldThreshold Movement needed by the analog stick to reset the idle timer. Used when 
 * HOLD mode is inactive.
 * Set between 1 - 128 to specify the movement on either axis.
 * Set to 0 for idle timer to be canceled even if the analog stick is not moved (that is, the idle timer itself stops running).
 * Specify SCE_CTRL_IDLE_CANCEL_TRESHOLD_NO_CANCELLATION for analog stick movement to not cancel the idle timer.
 * @param holdThreshold Movement needed by the analog stick to reset the idle timer. Used when HOLD mode is active.
 * Set between 1 - 128 to specify the movement on either axis.
 * Set to 0 for idle timer to be canceled even if the analog stick is not moved (that is, the idle timer itself stops running).
 * Specify SCE_CTRL_IDLE_CANCEL_TRESHOLD_NO_CANCELLATION for analog stick movement to not cancel the idle timer.
 *
 * @return 0 on success.
 */
s32 sceCtrlSetIdleCancelThreshold(s32 unHoldThreshold, s32 holdThreshold);

/**
 * Gets the number of VBlanks which will be waited for when the PSP device is being suspended.
 * 
 * @return The number of VBlanks to wait for.
 */
s16 sceCtrlGetSuspendingExtraSamples(void);

/**
 * Sets a number of VBlanks which will be waited for when the PSP device is being suspended.
 * 
 * @param suspendSamples The number of VBlanks. Between 0 - 300.
 * 
 * @return 0 on success.
 */
s32 sceCtrlSetSuspendingExtraSamples(s16 suspendSamples);

/**
 * Sets up internal controller buffers to receive external input data. Each input mode has its own
 * set of buffers. These buffers are of type ::SceCtrlData2. 
 * Note: This function has to be called initially in order to obtain external input data via the corresponding 
 * Peek/Read functions.
 * 
 * @param externalPort Pass a valid element of ::SceCtrlPort (either 1 or 2).
 * @param transferHandler Pointer to a SceCtrlInputDataTransferHandler containing a function to copy the @p inputSource
 *						into the PSP's controller buffers.
 * @param inputSource Pointer to buffer containing the Controller input data to copy to the PSP's 
 *					  controller buffers. It is passed as the source argument to the given transfer function.
 * 
 * @return 0 on success.
 */
s32 sceCtrl_driver_E467BEC8(u8 externalPort, SceCtrlInputDataTransferHandler *transferHandler, void *inputSource);

/**
 * @brief Gets the latch data.
 *
 * This function reads the latch data collected by the controller service. At each sampling
 * interval, the controller service compares the new pressed/released button states with the previously sampled pressed
 * button states and stores that comparison as latch data.
 *
 * Compared to ::sceCtrlReadLatch(), calling this API will not result in clearing the internal latch data. As such,
 * the data returned is the accumulated latch data since the last time ::sceCtrlReadLatch() was called. Consequently,
 * the returned data should not be relied on whether a button is currently in a pressed or released state.
 *
 * @param pLatch Pointer to a ::SceCtrlLatch variable which is to receive the accumulated button latch data.
 *
 * @return On success, the number of times the controller service performed sampling since the last time
 * ::sceCtrlReadLatch() was called.
 * @return < 0 on error.
 * 
 * @see ::SceCtrlLatch
 * @see ::sceCtrlReadLatch()
 */
s32 sceCtrlPeekLatch(SceCtrlLatch *pLatch);

/**
 * @brief Gets the latch data. 
 * 
 * This function reads the most recent latch data collected by the controller service. At each sampling
 * interval, the controller service compares the new pressed/released button states with the previously sampled pressed
 * button states and stores that comparison as latch data.
 * 
 * Compared to ::sceCtrlPeekLatch(), calling this API will result in clearing the internal latch data. As such,
 * calling code might have to explicitly wait for the controller service to update its collected latch data.
 * 
 * @param pLatch Pointer to a ::SceCtrlLatch variable which is to receive the current button latch data.
 * 
 * @return On success, the number of times the controller service performed sampling since the last time
 * ::sceCtrlReadLatch() was called.
 * @return < 0 on error.
 * 
 * @par Example:
 * @code
 * SceCtrlLatch latchData;
 * 
 * while (1) {
 *     // Obtain latch data
 *     sceCtrlReadLatch(&latchData);
 * 
 *     if (latchData.buttonMake & SCE_CTRL_CROSS)
 *     {
 *         // The Cross button has just been pressed (transition from 'released' state to 'pressed' state)
 *     }
 * 
 *     if (latchData.buttonPress & SCE_CTRL_SQUARE)
 *     {
 *         // The Square button is currently in the 'pressed' state
 *     }
 * 
 *    if (latchData.buttonBreak & SCE_CTRL_TRIANGLE)
 *    {
 *        // The Triangle button has just been released (transition from 'pressed' state to 'released' state)
 *    }
 * 
 *    if (latchData.buttonRelease & SCE_CTRL_CIRCLE)
 *    {
 *        // The Circle button is currently in the 'released' state
 *    }
 * 
 *    // As we clear the internal latch data with the ReadLatch() call, we can explicitly wait for the VBLANK interval
 *    // to give the controller service the time it needs to collect new latch data again. This guarantees the next call
 *    // to sceCtrlReadLatch() will return collected data again.
 *    //
 *    // Note: The sceCtrlReadBuffer*() APIs are implicitly waiting for a VBLANK interval if necessary.
 *    sceDisplayWaitVBlank();
 * }
 * @endcode
 * 
 * @see ::SceCtrlLatch
 * @see ::sceCtrlPeekLatch()
 */
s32 sceCtrlReadLatch(SceCtrlLatch *pLatch);

/**
 * @brief Retrieves controller state data by polling (positive logic).
 *
 * This function obtains button data stored in the internal controller buffers. Does not wait for the next 
 * update interval to be performed.  
 * The obtained data will be the latest transfered button data into the internal controller buffers.
 * 
 * @param pData Pointer to one or more ::SceCtrlData variables to receive controller state data. The obtained
 * button data is represented in positive logic.
 * @param nBufs The number of internal controller buffers to read. There are 64 internal controller
 * buffers which can be read. Has to be set to a value in the range of 1 - 64.
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
 *        // Cross button pressed
 *        if (data.buttons & SCE_CTRL_CROSS) {
 *            // do something
 *        }
 * }
 * @endcode
 */

s32 sceCtrlPeekBufferPositive(SceCtrlData *pData, u8 nBufs);

/**
 * @brief Retrieves controller state data by polling (negative logic).
 * 
 * This fucntion obtains button data stored in the internal controller buffers. Does not wait for the next 
 * update interval to be performed. 
 * The obtained data will be the latest transfered button data into the internal controller buffers.
 * 
 * @param pData Pointer to one or more ::SceCtrlData variables to receive controller state data. The obtained
 * button data is represented in negative logic.
 * @param nBufs The number of internal controller buffers to read. There are 64 internal controller
 * buffers which can be read. Has to be set to a value in the range of 1 - 64.
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
 *        // Cross button pressed
 *        if (data.buttons & ~SCE_CTRL_CROSS) {
 *            // do something
 *        }
 * }
 * @endcode
 */
s32 sceCtrlPeekBufferNegative(SceCtrlData *pData, u8 nBufs);

/**
 * @brief Retrieves controller state data by blocking (positive logic).
 * 
 * This function obtains button data stored in the internal controller buffers. Waits for the next update interval
 * before obtaining the data. The read data is the newest transfered data into the internal controller 
 * buffers.
 * 
 * @param pData Pointer to one or more ::SceCtrlData variables to receive controller state data. The obtained
 * data is represented in positive logic. 
 * @param nBufs The number of internal controller buffers to read. There are 64 internal controller
 * buffers which can be read. Has to be set to a value in the range of 1 - 64.
 * 
 * @return The number of read internal controller buffers on success.
 */
s32 sceCtrlReadBufferPositive(SceCtrlData *pData, u8 nBufs);

/**
 * @brief Retrieves controller state data by blocking (negative logic).
 * 
 * This function obtains button data stored in the internal controller buffers. Waits for the next update interval
 * before obtaining the data. The read data is the newest transfered data into the internal controller 
 * buffers.
 * 
 * @param pData Pointer to one or more ::SceCtrlData variables to receive controller state data. The obtained
 * data is represented in negative logic.
 * @param nBufs The number of internal controller buffers to read. There are 64 internal controller
 * buffers which can be read. Has to be set to a value in the range of 1 - 64.
 * 
 * @return The number of read internal controller buffers on success.
 */
s32 sceCtrlReadBufferNegative(SceCtrlData *pData, u8 nBufs);

/**
 * @brief Retrieves controller state data by polling (positive logic) with support for wireless controllers.
 *
 * This function Obtains button data stored in the internal controller buffers. Waits for the next update interval
 * before obtaining the data. The read data is the newest transfered data into the internal controller 
 * buffers and can contain input state provided by external input devices such as a wireless controller.
 * 
 * @remark You need to call ::sceCtrl_driver_E467BEC8() before initial use of this API or its related ones.
 * 
 * @param port Pass a valid element of ::SceCtrlPort (either 1 or 2).
 * @param pData Pointer to one or more ::SceCtrlData2 variables to receive controller state data. The obtained
 * button data is represented in positive logic.
 * @param nBufs The number of internal controller buffers to read. There are 64 internal controller
 * buffers which can be read. Has to be set to a value in the range of 1 - 64.
 * 
 * @return The number of read internal controller buffers on success.
 */
s32 sceCtrlPeekBufferPositive2(u32 port, SceCtrlData2 *pData, u8 nBufs);

/**
 * @brief Retrieves controller state data by polling (negative logic) with support for wireless controllers.
 *
 * This function obtains button data stored in the internal controller buffers. Waits for the next update interval
 * before obtaining the data. The read data is the newest transfered data into the internal controller
 * buffers and can contain input state provided by external input devices such as a wireless controller.
 *
 * @remark You need to call ::sceCtrl_driver_E467BEC8() before initial use of this API or its related ones.
 * 
 * @param port Pass a valid element of ::SceCtrlPort (either 1 or 2).
 * @param pData Pointer to one or more ::SceCtrlData2 variables to receive controller state data. The obtained
 * button data is represented in negative logic.
 * @param nBufs The number of internal controller buffers to read. There are 64 internal controller
 * buffers which can be read. Has to be set to a value in the range of 1 - 64.
 * 
 * @return The number of read internal controller buffers on success.
 */
s32 sceCtrlPeekBufferNegative2(u32 port, SceCtrlData2 *pData, u8 nBufs);

/**
 * @brief Retrieves controller state data by blocking (positive logic) with support for wireless controllers.
 *
 * This function obtains button data stored in the internal controller buffers. Waits for the next update interval
 * before obtaining the data. The read data is the newest transfered data into the internal controller
 * buffers and can contain input state provided by external input devices such as a wireless controller.
 *
 * @remark You need to call ::sceCtrl_driver_E467BEC8() before initial use of this API or its related ones.
 * 
 * @param port Pass a valid element of ::SceCtrlPort (either 1 or 2).
 * @param pData Pointer to one or more ::SceCtrlData2 variables to receive controller state data. The obtained
 * button data is represented in positive logic.
 * @param nBufs The number of internal controller buffers to read. There are 64 internal controller
 * buffers which can be read. Has to be set to a value in the range of 1 - 64.
 * 
 * @return The number of read internal controller buffers on success.
 */
s32 sceCtrlReadBufferPositive2(u32 port, SceCtrlData2 *pData, u8 nBufs);

/**
 * @brief Retrieves controller state data by blocking (negative logic) with support for wireless controllers.
 *
 * This function obtains button data stored in the internal controller buffers. Waits for the next update interval
 * before obtaining the data. The read data is the newest transfered data into the internal controller
 * buffers and can contain input state provided by external input devices such as a wireless controller.
 *
 * @remark You need to call ::sceCtrl_driver_E467BEC8() before initial use of this API or its related ones.
 * 
 * @param port Pass a valid element of ::SceCtrlPort (either 1 or 2).
 * @param pData Pointer to one or more ::SceCtrlData2 variables to receive controller state data. The obtained
 * button data is represented in negative logic.
 * @param nBufs The number of internal controller buffers to read. There are 64 internal controller
 * buffers which can be read. Has to be set to a value in the range of 1 - 64.
 * 
 * @return The number of read internal controller buffers on success.
 */
s32 sceCtrlReadBufferNegative2(u32 port, SceCtrlData2 *pData, u8 nBufs);

/**
 * Disables a rapid-fire button event.
 * 
 * @param slot The slot of the event to clear. Between 0 - 15.
 * 
 * @return 0 on success.
 */
s32 sceCtrlClearRapidFire(u8 slot);

/**
 * Specifies a rapid-fire event for one or more buttons.
 * 
 * @param slot      The slot used to set the custom values. Between 0 - 15. Up to 16 slots can be used.
 * @param uiMask    Comparison mask of the button operation for rapid-fire trigger. In order for the @p uiTrigger buttons
 *                            to trigger the event, they need to be included in these buttons. 
 *                            One or more buttons of ::SceCtrlPadButtons. 
 * @param uiTrigger The buttons which will start the rapid fire event for the specified 
 *                  @p uiTarget buttons when being pressed.
 * @param uiTarget  The buttons for which the rapid-fire event will be applied to. User mode 
 *                  buttons only. @p uiMake and @p uiBreak define the rapid-fire cycle.
 * @param uiDelay   Dead time of rapid-fire trigger (sampling count). Specifies the rapid-fire start timing. 
 *                  It will only be applied for the first ON period of a (not cancelled) rapid-fire event.  
 *                  Set to 0 - 63.
 * @param uiMake    The press time for the @p uiTarget buttons.  This "ON-time" is set after 
 *                  @p uiDelay was applied and the @p uiTrigger buttons were turned OFF. It will be 
 *                  applied for as long as the same rapid fire event is called without a 
 *                  break (i.e. pressing of a different PSP button). Set to 0 - 63. 
 *                  If set to 0, the @p uiTarget button(s) will be turned ON for one sampling count.
 * @param uiBreak   The release time for @p uiTarget buttons. This "OFF-time" is set after @p uiDelay was 
 *                  applied. It will be applied as long as the same rapid fire event is called 
 *                  without a break (i.e. the pressing of a different PSP button). Set to 0 - 63. 
 *                  If  set to 0, the @p uiTarget button will be turned OFF for 64 consecutive sampling counts.
 * 
 * @return 0 on success.
 * 
 * @par Example:
 * @code
 * // A rapid fire event for the R-button while the D-Pad-Up button is being pressed.
 * // R-button will be turned ON and OFF for 64 internal controller buffer updates in both cases 
 * // (as long as D-Pad-Up is pressed).
 * sceCtrlSetRapidFire(0, SCE_CTRL_UP, SCE_CTRL_UP, SCE_CTRL_RTRIGGER, 63, 63, 63);
 * 
 * // A rapid fire event for the R-button while the D-Pad-Up button is being pressed.
 * // R-button will be turned OFF and ON for 40 internal controller buffer updates in both cases 
 * // (as long as D-Pad-Up is pressed).
 * sceCtrlSetRapidFire(0, SCE_CTRL_UP, SCE_CTRL_UP, SCE_CTRL_RTRIGGER, 0, 40, 40);
 * @endcode
 */
s32 sceCtrlSetRapidFire(u8 slot, u32 uiMask, u32 uiTrigger, u32 uiTarget, u8 uiDelay, 
                        u8 uiMake, u8 uiBreak);

/**
 * Emulates values for the analog pad's X- and Y-axis.
 * 
 * @param slot The slot used to set the custom values. Between 0 - 3. If multiple slots are used, 
 *             their settings are combined.
 * @param aX New emulated value for the X-axis. Between 0 - 0xFF.
 * @param aY New emulate value for the Y-axis. Between 0 - 0xFF.
 * @param uiMake Specifies the duration of the emulation. Meassured in sampling counts.
 * 
 * @return 0 on success.
 */
s32 sceCtrlSetAnalogEmulation(u8 slot, u8 aX, u8 aY, u32 uiMake);

/**
 * Emulates buttons for the digital pad.
 * 
 * @param slot The slot used to set the custom values. Between 0 - 3. If multiple slots are used, 
 *             their settings are combined.
 * @param userButtons Emulated user buttons of ::SceCtrlPadButtons. You cannot emulate kernel 
 *                    buttons and the emulated buttons will only be applied for applications 
 *                    running in user mode. 
 * @param kernelButtons Emulated buttons of ::SceCtrlPadButtons (you can emulate both user and 
 *                      kernel buttons). The emulated buttons will only be applied for applications 
 *                      running in kernel mode.
 * @param uiMake Specifies the duration of the emulation. Meassured in sampling counts.
 * 
 * @return 0 on success.
 */
s32 sceCtrlSetButtonEmulation(u8 slot, u32 userButtons, u32 kernelButtons, u32 uiMake);

/**
 * Gets the button mask settings applied to PSP buttons.
 *
 * @param buttons The buttons to check for. One or more buttons of ::SceCtrlPadButtons.
 *
 * @return The button mask mode for the given buttons. One of ::SceCtrlPadButtonMaskMode.
 */
u32 sceCtrlGetButtonIntercept(u32 buttons);

/**
 * Sets a button mask mode for one or more buttons. You can only mask user mode buttons in user applications.
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
 * // Block user mode buttons for User mode applications
 * sceCtrlSetButtonIntercept(0xFFFF, SCE_CTRL_MASK_IGNORE_BUTTONS);
 * // Do something
 * 
 * // Remove block from user mode buttons for User mode applications
 * sceCtrlSetButtonIntercept(0xFFFF, SCE_CTRL_MASK_NO_MASK);
 * @endcode
 */
u32 sceCtrlSetButtonIntercept(u32 buttons, u32 buttonMaskMode);

/**
 * Registers a button callback.
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
 * Unknown. 
 * 
 * @param arg1 Unknown argument.
 * 
 * @return 0.
 */
u32 sceCtrl_driver_6C86AF22(s32 arg1);

/**
 * Unknown.
 * 
 * @param arg1 Unknown argument.
 * 
 * @return 0.
 */
u32 sceCtrl_driver_5886194C(s8 arg1);

/**
 * Unknown.
 * 
 * @return 0.
 */
u32 sceCtrlUpdateCableTypeReq(void);

#endif	/* CTRL_H */

/** @} */
