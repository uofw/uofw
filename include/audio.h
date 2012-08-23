/* Copyright (C) 2011, 2012 The uOFW team
   See the file COPYING for copying permission.
*/

/**
 * @author artart78
 * @version 6.60
 *
 * The audio.prx module RE'ing.
 */

#include "common_header.h"

/** @defgroup Audio Audio Module
 *
 * @{
 */

/** The input parameters structure, used by sceAudioInputInitEx() */
typedef struct
{
    /** ? */
    int unk0;
    /** The input gain. */
    int gain;
    /** ? */
    int unk2;
    /** ? */
    int unk3;
    /** ? */
    int unk4;
    /** ? */
    int unk5;
} SceAudioInputParams;

/**
 * Outputs audio (raw PCM) to channel.
 *
 * @param chanId The channel ID, returned by sceAudioChReserve().
 * @param vol The volume (0 - 0xFFFF).
 * @param buf The PCM buffer to output.
 *
 * @return The sample count in case of success, otherwise less than zero.
 */
int sceAudioOutput(u32 chanId, int vol, void *buf);

/**
 * Outputs audio (raw PCM) to channel and doesn't return until everything has been played.
 *
 * @param chanId The channel ID, returned by sceAudioChReserve().
 * @param vol The volume (0 - 0xFFFF).
 * @param buf The PCM buffer to output.
 *
 * @return The sample count in case of success, otherwise less than zero.
 */
int sceAudioOutputBlocking(u32 chanId, int vol, void *buf);

/**
 * Outputs audio (raw PCM) to channel with different left and right volumes.
 *
 * @param chanId The channel ID, returned by sceAudioChReserve().
 * @param leftVol The left volume (0 - 0xFFFF).
 * @param rightVol The right volume (0 - 0xFFFF).
 * @param buf The PCM buffer to output.
 *
 * @return The sample count in case of success, otherwise less than zero.
 */
int sceAudioOutputPanned(u32 chanId, int leftVol, int rightVol, void *buf);

/**
 * Outputs audio (raw PCM) to channel with different left and right volumes. The function doesn't return until the entire buffer has been played.
 *
 * @param chanId The channel ID, returned by sceAudioChReserve().
 * @param leftVol The left volume (0 - 0xFFFF).
 * @param rightVol The right volume (0 - 0xFFFF).
 * @param buf The PCM buffer to output.
 *
 * @return The sample count in case of success, otherwise less than zero.
 */
int sceAudioOutputPannedBlocking(u32 chanId, int leftVol, int rightVol, void *buf);

/**
 * Reserves a channel.
 *
 * @param channel The channel ID you want to reserve, or -1 if you want the first free one to be selected.
 * @param sampleCount The number of samples.
 * @param format The audio format (0x10 for MONO, 0 for STEREO)
 *
 * @return The channel ID in case of success, otherwise less than zero.
 */
int sceAudioChReserve(int channel, int sampleCount, int format);

/**
 * Reserves the channel & outputs in "one shot"
 *
 * @param chanId The channel ID, or -1 if you want the first free channel to be chose.
 * @param sampleCount The number of samples to play.
 * @param fmt The audio format (0x10 for MONO, 0 for STEREO)
 * @param leftVol The left ear volume (0 - 0xFFFF).
 * @param rightVol The right ear volume (0 - 0xFFFF).
 * @param buf The PCM audio buffer.
 *
 * @return The channel ID in case of success, otherwise less than zero.
 */
int sceAudioOneshotOutput(int chanId, int sampleCount, int fmt, int leftVol, int rightVol, void *buf);

/**
 * Releases a channel.
 *
 * @param channel The channel ID to release.
 *
 * @return 0 in case of succes, otherwise less than zero.
 */
int sceAudioChRelease(u32 channel);

/**
 * Get the number of remaining unplayed samples of a channel.
 *
 * @param chanId The channel ID to check.
 *
 * @return The number of remaining samples.
 */
int sceAudioGetChannelRestLength(u32 chanId);

/**
 * Set the sample count of a channel.
 *
 * @param chanId The channel ID.
 * @param sampleCount The number of samples.
 *
 * @return 0 on success, otherwise less than zero.
 */
int sceAudioSetChannelDataLen(u32 chanId, int sampleCount);

/**
 * Change the volume of a channel.
 *
 * @param chanId The channel ID.
 * @param leftVol The left ear volume.
 * @param rightVol The right ear volume.
 *
 * @return 0 on success, otherwise less than 0
 */
int sceAudioChangeChannelVolume(u32 chanId, int leftVol, int rightVol);

/**
 * Change the format (mono/stereo) of a channel.
 *
 * @param chanId The channel ID.
 * @param format The audio format (0 for STEREO, 0x10 for MONO).
 *
 * @return 0 on success, otherwise less than 0
 */
int sceAudioChangeChannelConfig(u32 chanId, int format);

/**
 * Change SRC output sample count.
 *
 * @param sampleCount The sample count.
 *
 * @return 0 on success, otherwise less than zero.
 */
int sceAudioOutput2ChangeLength(int sampleCount);

/**
 * Get the number of SRC unplayed samples.
 *
 * @return The number of unplayed samples.
 */
int sceAudioOutput2GetRestSample(void);

/**
 * Get the number of unplayed samples of a channel.
 *
 * @param chanId The channel ID.
 *
 * @return The number of samples in case of success, otherwise less than zero.                                                              
 */
int sceAudioGetChannelRestLen(u32 chanId);

/**
 * Reserve a SRC output.
 *
 * @param sampleCount The number of samples of the output.
 *
 * @return 0 in case of success, otherwise less than zero.
 */
int sceAudioOutput2Reserve(int sampleCount);

/**
 * Output to SRC. This function only returns when all the samples have been played.
 *
 * @param vol The output volume.
 * @param buf The PCM audio buffer.
 *
 * @return The sample count on success, otherwise less than zero.
 */
int sceAudioOutput2OutputBlocking(int vol, void *buf);

/**
 * Release SRC output.
 *
 * @return 0 on success, otherwise less than zero.
 */
int sceAudioOutput2Release(void);

/**
 * Sets the audio output frequency.
 *
 * @param freq The frequency (44100 or 48000).
 *
 * @return 0 on success, otherwise less than zero.
 */
int sceAudioSetFrequency(int freq);

/**
 * Inits audio.
 */
int sceAudioInit();

/**
 * \todo ?
 */
int sceAudioLoopbackTest(int arg0);

/**
 * Frees the audio system.
 */
int sceAudioEnd();

/**
 * Selects the delaying mode.
 *
 * @param arg If set to 1, channels 0 - 6 will be delayed after sceAudioOutputPannedBlocking, otherwise only channels 0 - 4 will be.
 *
 * @return 0.
 */
int sceAudio_driver_FF298CE7(int arg);

/**
 * Sets the volume offset/shifting.
 *
 * @param arg The output buffer will be shifted by (arg + 8); default is 8, so arg = 0 is the default value.
 *
 * @return 0.
 */
int sceAudioSetVolumeOffset(int arg);

/**
 * Reserves a SRC output.
 *
 * @param sampleCount The number of samples.
 * @param freq The output frequency.
 * @param numChans The number of output "channels" (only 2 is accepted, for stereo)
 *
 * @return 0 on success, otherwise less than zero.
 */
int sceAudioSRCChReserve(int sampleCount, int freq, int numChans);

/**
 * Releases a SRC output.
 *
 * @return 0 on success, otherwise less than zero.
 */
int sceAudioSRCChRelease(void);

/**
 * Outputs to SRC. The functions returns only when all the buffer has been played.
 *
 * @param vol The output volume (0 - 0xFFFF).
 * @param buf The audio PCM buffer.
 *                                                                                                                                             
 * @return The sample count on success, otherwise less than zero.                                                                              
 */
int sceAudioSRCOutputBlocking(int vol, void *buf);

/**
 * Waits for the input to end.
 *
 * @return 0 on success, otherwise less than 0.
 */
int sceAudioWaitInputEnd();

/**
 * Inits audio input.                                                                                                                          
 *                                                                                                                                             
 * @param param The structure containing the input parameters.
 *
 * @return 0 on success, otherwise less than zero.
 */
int sceAudioInputInitEx(SceAudioInputParams *param);

/**
 * Inits audio input (probably deprecated, has less parameters than sceAudioInputInitEx()).
 *
 * @param arg0 \todo ?
 * @param gain The input gain.
 * @param arg2 \todo ?
 *
 * @return 0 on success, otherwise less than zero.
 */
int sceAudioInputInit(int arg0, int gain, int arg2);

/**
 * Waits for the input to end, and store input.
 *
 * @param sampleCount The number of samples to read.
 * @param freq The input frequency.
 * @param buf The audio PCM input buffer.
 *
 * @return The number of played samples on success, otherwise less than zero.
 */
int sceAudioInputBlocking(int sampleCount, int freq, void *buf);

/**
 * Store input.
 *
 * @param sampleCount The number of samples to read.
 * @param freq The input frequency.
 * @param buf The audio PCM input buffer.
 *
 * @return The number of played samples on success, otherwise less than zero.
 */
int sceAudioInput(int sampleCount, int freq, void *buf);

/**
 * Get the number of samples read from input.
 *
 * @return The sample count.
 */
int sceAudioGetInputLength();

/**
 * Checks if the input has ended.
 *
 * @return 1 if the input is still running, otherwise 0.
 */
int sceAudioPollInputEnd();

/**
 * \todo ?
 */
int sceAudio_driver_5182B550(int arg);

/** @} */

