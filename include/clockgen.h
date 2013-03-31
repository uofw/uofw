/* Copyright (C) 2011, 2012, 2013 The uOFW team
   See the file COPYING for copying permission.
*/

#ifndef CLOCKGEN_H
#define CLOCKGEN_H

#include <common_header.h>

/** @defgroup Clockgen Clockgen Module
 * Clock generator management.
 * @{
 */

/**
 * Setups the module by retrieving all hardware registers to memory.
 *
 * @return SCE_ERROR_OK, otherwise <0 on error.
 */
s32 sceClockgenSetup();

/**
 * Sets the spectrum spreading mode.
 *
 * @param mode The new spectrum spreading mode. Unknown unit.
 *
 * @return SCE_ERROR_OK, otherwise <0 on error.
 */
s32 sceClockgenSetSpectrumSpreading(s32 mode);

/**
 * Inits the module.
 *
 * Sets up the I2C bus speed, creates the mutex and registers the sysevent handler.
 *
 * @return SCE_ERROR_OK, otherwise <0 on error.
 */
s32 sceClockgenInit();

/**
 * Deinits the module.
 *
 * Deletes the mutex and unregisters the sysevent handler.
 *
 * @return SCE_ERROR_OK.
 */
s32 sceClockgenEnd();

/**
 * Sets the protocol.
 *
 * Changes the behavior of sceClockgenSetup().
 * Non-zero will retrieve the hardware registers with a single command.
 *
 * @param prot Boolean.
 *
 * @return SCE_ERROR_OK.
 */
s32 sceClockgenSetProtocol(u32 prot);

/**
 * Gets the CY27040 chip revision.
 *
 * @return Likely 3, 4, 7, 8, 9, 10 or 15. Another value may indicate that you work at SCE.
 */
s32 sceClockgenGetRevision();

/**
 * Gets the CY27040 hardware register value as stored in memory.
 *
 * @param idx An index where 0 <= idx < 3.
 *
 * @return The register value, otherwise <0.
 */
s32 sceClockgenGetRegValue(u32 idx);

/**
 * Sets the audio clock frequency.
 *
 * @param freq A sample rate frequency in Hz. 44100 or 48000.
 *
 * @return SCE_ERROR_OK, otherwise <0.
 */
s32 sceClockgenAudioClkSetFreq(u32 freq);

/**
 * Enables the audio clock.
 *
 * @return SCE_ERROR_OK, otherwise <0.
 */
s32 sceClockgenAudioClkEnable();

/**
 * Disables the audio clock.
 *
 * @return SCE_ERROR_OK, otherwise <0.
 */
s32 sceClockgenAudioClkDisable();

/**
 * Enables the lepton clock (managing the UMD reader).
 *
 * @return SCE_ERROR_OK, otherwise <0.
 */
s32 sceClockgenLeptonClkEnable();

/**
 * Disables the lepton clock (managing the UMD reader).
 *
 * @return SCE_ERROR_OK, otherwise <0.
 */
s32 sceClockgenLeptonClkDisable();

/** @} */

#endif /* CLOCKGEN_H */
