/* Copyright (C) 2011, 2012, 2013 The uOFW team
   See the file COPYING for copying permission.\n
*/

#ifndef LED_H
#define	LED_H

#include "common_header.h"

/** @defgroup LED LED Module
 *  Hardware LED management.
 * @{	
 */

/**
 * The LEDs which can be controlled via ::sceLedSetMode().
 */
enum ScePspLedTypes {
    /** Memory-Stick LED. */
    PSP_LED_TYPE_MS = 0, 
    /** W-LAN LED. */
    PSP_LED_TYPE_WLAN = 1, 
    /** Bluetooth LED. */
    PSP_LED_TYPE_BT = 2
};

/**
 * The possible LED control commands.
 */
enum SceLedModes {
    /** Turn a LED OFF. */
    LED_MODE_OFF = 0,
    /** Turn a LED ON. */
    LED_MODE_ON = 1,
    /** Set a blink event for a LED. */
    LED_MODE_BLINK = 2,
    /** 
     * Register LED configuration commands and execute them.  Its use is not recommended, 
     * as it is still not completely known how that mode works.
     */
    LED_MODE_SELECTIVE_EXEC = 3
};

/** 
 * LED control commands which can be passed via a
 * ::SceLedConfiguration package to the system. 
 */
enum SceLedCmds {
    /** 
     * Save a LED configuration command for later use.  You can only 
     * save one configuration. 
     */
    LED_CMD_SAVE_CMD = 1,
    /** Execute a saved LED configuration command. */
    LED_CMD_EXECUTE_SAVED_CMD = 2,
    /** 
     * Register a LED configuration to be executed manually by the user. 
     * Upto 4 LED configuration can be registered the same time.
     */
    LED_CMD_REGISTER_CMD = 3,
    /** 
     * Execute the recently registered LED configuration. Once it has been executed as 
     * many times the user as the user decided, 
     */
    LED_CMD_EXECUTE_CMD = 4,
    /** Turn ON a specified LED. */
    LED_CMD_TURN_LED_ON = 16,
    /** Turn OFF a specified LED. */
    LED_CMD_TURN_LED_OFF = 17,
    /** Switch the state of an LED. ON -> OFF, or OFF -> ON.*/
    LED_CMD_SWITCH_LED_STATE = 18,
    /** 
     * Setup a blink event for a LED. The settings for the blink event are set via the
     * ::SceLedConfiguration.
     */
    LED_CMD_BLINK_LED = 19,
};

/**
 * This structure represents a LED blink configuration.  It needs to be specified
 * when using ::sceLedSetMode and the LED_MODE_BLINK / LED_MODE_SELECTIVE_EXEC 
 * setting for the LED mode. A blink event contains an ON-/OFF-time for the target 
 * LED, the total time of the event and the final LED state at the end of the event.
 * 
 * Note: When using LED_MODE_BLINK, you don't need to set the first four structure
 *       members.
 */
typedef struct {
    /** This command should be used to register/execute a custom command. */
    u8 selectiveCmd;
    /** The number or executions of the specified <customCmd>.*/
    u8 numExecs;
    /** The custom LED command to execute. One of ::SceLedCmds. */
    u8 customCmd;
    /** Unknown. */
    u8 unk;
    /** The "on" time of a LED during a blink period. */
    s32 onTime; 
    /** The "off" time of a LED during a blink period. */
    s32 offTime; 
    /** The time of a blink period. Set to -1 for an infinite time period. */
    s32 blinkTime; 
    /** The state of a LED at the end of the blink period. */
    s32 endBlinkState; 
} SceLedConfiguration; 

/**
 * Initialize the LED library, enable the PSP's LEDs and turn them ON.
 * 
 * @return 0.
 */
u32 sceLedInit(void);

/**
 * Terminate the LED library and disable the PSP's LEDs.
 * 
 * @return 0.
 */
u32 sceLedEnd(void);

/**
 * Set a LED mode.
 * 
 * @param led The LED to set a mode for. One of ::ScePspLedTypes.
 * @param mode The mode to set for a LED. One of ::SceLedModes.
 * @param config Configuration settings for a LED. Is only used for the ::SceLedModes
 *               LED_MODE_BLINK and LED_MODE_SELECTIVE_EXEC.
 * 
 * @return 0 on success.
 */
s32 sceLedSetMode(s32 led, s32 mode, SceLedConfiguration *config);

#endif	/* LED_H */

/** @} */

