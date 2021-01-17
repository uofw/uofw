/* Copyright (C) 2011, 2012, 2013 The uOFW team    
   See the file COPYING for copying permission.
*/

#include <clockgen.h>
#include <common_imp.h>

/**
 * Setups the module and clock generator.
 *
 * Called when the module is started.
 *
 * @return SCE_ERROR_OK.
 */
static int _sceClockgenModuleStart(int args, void *argp);

/**
 * Deinits the module.
 *
 * Called when the module is stopped.
 *
 * @return SCE_ERROR_OK.
 */
static int _sceClockgenModuleRebootBefore();

/**
 * Event handler function.
 *
 * Called on sleep and resume events.
 *
 * @return SCE_ERROR_OK.
 */
static s32 _sceClockgenSysEventHandler(s32 ev_id, char *ev_name, void *param, s32 *result);

/**
 * Changes the mode of a clock controller.
 *
 * @param bus PSP_CLOCK_AUDIO_FREQ, PSP_CLOCK_LEPTON or PSP_CLOCK_AUDIO
 * @param mode 1 to enable, 0 to disable. Also selects 48kHz or 44.1kHz audio freqs.
 *
 * @return SCE_ERROR_OK, otherwise <0 on error.
 */
static s32 _sceClockgenSetControl1(s32 bus, SceBool mode);

/**
 * Writes to a CY27040 hardware register.
 *
 * @param idx An index where 0 <= idx < 3.
 * @param val The new value of the target register.
 *
 * @return SCE_ERROR_OK, otherwise <0 on error.
 */
static s32 _cy27040_write_register(u8 idx, u8 val);
