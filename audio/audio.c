/**
 * \author artart78
 * \version 6.39 (but only the NIDs changed in 6.60)
 *
 * The audio.prx module RE'ing.
 */

#include "../global.h"

asm(".set noat"); // needed for AUDIO_SET_BUSY()

/** \defgroup Audio Audio Module */

/*@{*/

/* Sets the audio controller busy state.
 * busy: 0 or 1
 */
#define AUDIO_SET_BUSY(busy) asm("lui $at, 0xBE00; sw %0, 0($at)" : : "r" (busy))

/* The audio channel structure. */
typedef struct
{
    /* The channel buffer. */
    void *buf;
    /* The currently unplayed samples count. */
    int curSampleCnt;
    /* The original sample count. */
    u16 sampleCount; // 8
    /* \todo */
    char unk10;
    /* The number of bytes per sample (2 in mono, 4 in stereo). */
    char bytesPerSample; // 11
    /* The left volume. */
    short leftVol;
    /* The right volume. */
    short rightVol;
} SceAudioChannel;

/* The audio controller structure. */
typedef struct
{
    /* TODO */
    u8 buf0[240]; // 0
    /* TODO */
    u8 buf240[272]; // 240
    /* TODO */
    u8 buf512[240]; // 512
    /* TODO */
    u8 buf752[272]; // 752
    /* TODO */
    u32 hwBuf[48]; // 1024
    /* TODO */
    u32 *dmaPtr[3]; // 1216
    /* The audio event flag ID. */
    SceUID evFlagId; // 1228
    /* The audio channels structures. */
    SceAudioChannel chans[8]; // 1232
    /* The channel sample rate. */
    u16 freq; // 1360
    /* The sample rate, converted to be sent to the hardware. */
    u16 hwFreq; // 1362
    /* The audio flags. */
    u8 flags; // 1364
    /* The volume offset. */
    u8 volumeOffset; // 1365
    u8 delayShift; // 1366
    u8 padding1; // 1367
    u16 srcChFreq; // 1368
    u16 srcChSampleCnt; // 1370
    u16 srcVol; // 1372
    u8 numChans; // 1374
    u8 padding2; // 1375
    u32 inputOrigSampleCnt; // 1376
    u32 inputCurSampleCnt; // 1380
    u16 *inputBuf; // 1384
    u8 unkInput0, inputGain, unkInput2, unkInput3, unkInput4, unkInput5; // 1388
    u8 inputHwFreq; // 1394
    u8 inputIsWaiting; // 1395
    u8 inputInited; // 1396
    u8 unkCodecArg; // 1397
    u8 unkCodecRet; // 1398
    u8 unkCodecArgSet; // 1399
    // end: 1400
} SceAudio;

typedef struct
{
    int unk0;
    int gain;
    int unk2;
    int unk3;
    int unk4;
    int unk5;
} SceAudioInputParams;

int audioOutputDmaCb(int unused, int arg1);
int audioOutput(SceAudioChannel *channel, short leftVol, short rightVol, void *buf);
int audioIntrHandler();
int audioEventHandler(int ev_id, char* ev_name, void* param, int* result);
int audioSRCOutputDmaCb(int arg0, int arg1);
int audioInputThread();
int audioInputDmaCb(int arg0, int arg1);

SceAudio g_audio;
char g_audioEventName[] = "SceAudio";
SceSysEventHandler g_audioEvent = {0x40, g_audioEventName, 0x00FFFF00, audioEventHandler, 0, 0, NULL, {0, 0, 0, 0, 0, 0, 0, 0, 0}};

// 0000
/*
 * Update / synchronize the audio buffers to play them.
 * 
 * arg: Set to 1 if called from SRCOutput
 */
void updateAudioBuf(int arg)
{
    int v = arg + 1;
    if (g_audio.flags == 0)
    {
        // 01D0
        AUDIO_SET_BUSY(1);
        sceCodec_driver_B2EF6B19(1);
    }
    // 0038
    int v2 = v | g_audio.flags;
    *(int*)(0xBE000004) = (int)(char)(v2 & 0xFF) ^ v;
    g_audio.flags = v2;
    // 0054
    while ((*(int*)(0xBE00000C) & v) != 0)
        ;
    sceKernelDmaOpQuit(g_audio.dmaPtr[0]);
    *(int*)(0xBE000008) = v ^ 7;
    *(int*)(0xBE00002C) = v;
    *(int*)(0xBE000020) = v;
    v <<= 4;
    // 00A0
    int i;
    for (i = 0; i < 24; i++)
    {
        while ((*(int*)(0xBE000028) & v) == 0)
            ;
        *(int*)(0xBE000060 + (arg << 4)) = 0;
    }
    if (sceKernelDmaOpAssign(g_audio.dmaPtr[arg], 0xFF, 0xFF, (arg * 64 + 320) | 0x0100C801, 0) == 0)
    {
        // 0110
        if (sceKernelDmaOpSetCallback(g_audio.dmaPtr[0], (arg == 0) ? audioOutputDmaCb : audioSRCOutputDmaCb, 0) == 0)
        {
            char shift;
            if (g_audio.hwBuf[arg * 16 + 1] != 0)
                shift = 0;
            else
                shift = 2;
            if (sceKernelDmaOpSetupLink(g_audio.dmaPtr[arg], (arg * 64 + 320) | 0x0100C801, &g_audio.hwBuf[(arg * 4 + shift) * 4]) == 0) {
                sceKernelSetEventFlag(g_audio.evFlagId, 0x20000000 << (arg & 0x1F));
                sceKernelDmaOpEnQueue(g_audio.dmaPtr[arg]);
            }
        }
    }
    // 0180
    // 0184
    *(int*)(0xBE000008) = 7;
    *(int*)(0xBE000004) = (int)g_audio.flags;
    *(int*)(0xBE000010) = g_audio.flags & 3;
    *(int*)(0xBE000024) = (int)g_audio.flags;
    SYNC();
}

// 01EC
/* Update DMA.
 *
 * arg: Memory type: 0 for output, 1 for SRCOutput, 2 for input
 */
int dmaUpdate(int arg)
{
    char v = g_audio.flags & ~(1 << (arg & 0x1F));
    *(int*)(0xBE000004) = v;
    g_audio.flags = v;
    sceKernelDmaOpQuit(g_audio.dmaPtr[arg]);
    sceKernelDmaOpDeQueue(g_audio.dmaPtr[arg]);
    if (arg == 2)
    {
        // 0298
        g_audio.inputInited = 0;
        // 029C
        while ((*(int*)(0xBE00000C) & 4) != 0)
            ;
    }
    // 025C
    if (g_audio.flags == 0)
    {
        // 0284
        AUDIO_SET_BUSY(0);
        sceCodec_driver_B2EF6B19(0);
    }
    return 0;
}

// 02B8
/* The audio mixer thread. */
int audioMixerThread()
{
    int sp0[128];
    SceAudio *userAudio = UCACHED(&g_audio);
    memset(sp0, 0, 512);
    SceAudio *uncachedAudio = KUNCACHED(&g_audio);
    u32 *unk = g_audio.dmaPtr[0];
    // 0328
    for (;;)
    {
        int flag = 0;
        if (sceKernelWaitEventFlag(g_audio.evFlagId, 0x20000000, 33, 0, 0) < 0)
        {
            // 04F4
            sceKernelExitThread(0);
            return 0;
        }
        char playedSamples = 0;
        // 035C
        int j;
        for (j = 0; j < 8; j++)
        {
            SceAudioChannel *chan = &g_audio.chans[j];
            short *buf = chan->buf;
            if (buf != NULL)
            {
                playedSamples = MIN(chan->curSampleCnt, 64);
                // 038C
                int i;
                for (i = 0; i < playedSamples * 2; i += 2)
                {
                    sp0[i + 0] += buf[0] * chan->leftVol  / 128;
                    sp0[i + 1] += buf[1] * chan->rightVol / 128;
                    buf += chan->bytesPerSample / 2;
                }
                chan->curSampleCnt -= playedSamples;
                if (chan->curSampleCnt == 0)
                {
                    // 04E0
                    flag |= 1 << (j & 0x1F);
                    buf = NULL;
                }
                // 03D8
                chan->buf = buf;
            }
            // 03DC
        }

        if (playedSamples == 0)
        {
            // 04BC
            int oldIntr = sceKernelCpuSuspendIntr();
            g_audio.flags &= 0xFE;
            sceKernelCpuResumeIntr(oldIntr);
        }
        else
        {
            char shift = g_audio.volumeOffset;
            char unk1 = unk[8] < userAudio->buf0[64];
            int *dstBuf = (int*)(uncachedAudio->buf0 + unk1 * 256);
            // 0408
            int *u32buf = sp0;
            int i;
            for (i = 0; i < 128; i += 2)
            {
                dstBuf[i] = ((MAX(MIN(u32buf[0] >> (shift & 0x1F), 0x7FFF), 0x8000) <<  0) & 0x00FF)
                    | ((MAX(MIN(u32buf[1] >> (shift & 0x1F), 0x7FFF), 0x8000) << 16) & 0xFF00);
                u32buf[0] = 0;
                u32buf[1] = 0;
            }
            *(int*)(uncachedAudio + 1048 +   unk1  * 32) = 0;
            *(int*)(uncachedAudio + 1048 + (!unk1) * 32) = userAudio->hwBuf[unk1 * 2];
            sceDdrFlush(4);
            if ((g_audio.flags & 1) == 0)
            {
                *(int*)(uncachedAudio + 1080) = 0;
                // 0498
                *(int*)(uncachedAudio + 1048) = 0;
                int oldIntr = sceKernelCpuSuspendIntr();
                updateAudioBuf(0);
                sceKernelCpuResumeIntr(oldIntr);
            }
            // 0488
            sceKernelSetEventFlag(g_audio.evFlagId, flag);
        }
    }
}

// 0530
/*
 * The DMA callback for normal output.
 *
 * unused: ?
 * arg1: ?
 */
int audioOutputDmaCb(int unused, int arg1)
{
    sceKernelSetEventFlag(g_audio.evFlagId, 0x20000000);
    if (arg1 != 0) {
        // 056C
        dmaUpdate(0);
    }
    return 0;
}

/**
 * Outputs audio (raw PCM) to channel.
 *
 * @param chanId The channel ID, returned by sceAudioChReserve().
 * @param vol The volume (0 - 0xFFFF).
 * @param buf The PCM buffer to output.
 *
 * @return The sample count in case of success, otherwise less than zero.
 */
int sceAudioOutput(u32 chanId, int vol, void *buf)
{
    if (vol > 0xFFFF)
        return ERROR_AUDIO_INVALID_VOLUME;
    if (chanId >= 8)
        return ERROR_AUDIO_INVALID_CHANNEL;
    K1_BACKUP();
    if (!K1_USER_BUF_STA_SZ(buf, g_audio.chans[chanId].sampleCount * 4)) {
        // 0654
        K1_RESET();
        return ERROR_AUDIO_PRIV_REQUIRED;
    }
    int oldIntr = sceKernelCpuSuspendIntr();
    int ret = audioOutput(&g_audio.chans[chanId], vol, vol, buf);
    sceKernelCpuResumeIntr(oldIntr);
    K1_RESET();
    return ret;
}

/**
 * Outputs audio (raw PCM) to channel and doesn't return until everything has been played.
 *
 * @param chanId The channel ID, returned by sceAudioChReserve().
 * @param vol The volume (0 - 0xFFFF).
 * @param buf The PCM buffer to output.
 *
 * @return The sample count in case of success, otherwise less than zero.
 */
int sceAudioOutputBlocking(u32 chanId, int vol, void *buf)
{
    if (vol >= 0xFFFF)
        return ERROR_AUDIO_INVALID_VOLUME;
    if (chanId >= 8)
        return ERROR_AUDIO_INVALID_CHANNEL;
    SceAudioChannel *chan = &g_audio.chans[chanId];
    K1_BACKUP();
    if (!K1_USER_BUF_STA_SZ(buf, chan->sampleCount * 4))
    {
        // 07D8
        K1_RESET();
        return ERROR_AUDIO_PRIV_REQUIRED;
    }
    int oldIntr = sceKernelCpuSuspendIntr();
    int ret = audioOutput(chan, vol, vol, buf);
    if ((u32)ret == ERROR_AUDIO_CHANNEL_BUSY)
    {
        // 0758
        if (chan->unk10 == 0)
        {
            chan->unk10 = (buf == NULL) ? 0xFF : 1;
            // 077C
            do
            {
                sceKernelCpuResumeIntr(oldIntr);
                ret = sceKernelWaitEventFlag(g_audio.evFlagId, 1 << chanId, 32, 0, 0);
                if (ret < 0) {
                    K1_RESET();
                    return ret;
                }
                oldIntr = sceKernelCpuSuspendIntr();
                ret = audioOutput(chan, vol, vol, buf);
            } while ((u32)ret == ERROR_AUDIO_CHANNEL_BUSY);
            chan->unk10 = 0;
        }
    }
    sceKernelCpuResumeIntr(oldIntr);
    K1_RESET();
    return ret;
}

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
int sceAudioOutputPanned(u32 chanId, int leftVol, int rightVol, void *buf)
{
    if (rightVol > 0xFFFF || leftVol > 0xFFFF)
        return ERROR_AUDIO_INVALID_VOLUME;
    if (chanId >= 8)
        return ERROR_AUDIO_INVALID_CHANNEL;
    SceAudioChannel *chan = &g_audio.chans[chanId];
    K1_BACKUP();
    if (!K1_USER_BUF_STA_SZ(buf, chan->sampleCount * 4))
    {
        // 08D4
        K1_RESET();
        return ERROR_AUDIO_PRIV_REQUIRED;
    }
    int oldIntr = sceKernelCpuSuspendIntr();
    int ret = audioOutput(chan, leftVol, rightVol, buf);
    sceKernelCpuResumeIntr(oldIntr);
    K1_RESET();
    return ret;
}

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
int sceAudioOutputPannedBlocking(u32 chanId, int leftVol, int rightVol, void *buf)
{
    if ((leftVol | rightVol) > 0xFFFF)
        return ERROR_AUDIO_INVALID_VOLUME;
    if (chanId >= 8)
        return ERROR_AUDIO_INVALID_CHANNEL;
    SceAudioChannel *chan = &g_audio.chans[chanId];
    K1_BACKUP();
    if (!K1_USER_BUF_STA_SZ(buf, chan->sampleCount * 4))
    {
        // 0A9C
        K1_RESET();
        return ERROR_AUDIO_PRIV_REQUIRED;
    }
    int oldIntr = sceKernelCpuSuspendIntr();
    int ret = audioOutput(chan, leftVol, rightVol, buf);
    if ((u32)ret == ERROR_AUDIO_CHANNEL_BUSY)
    {
        // 0A08
        if (chan->unk10 == 0)
        {
            chan->unk10 = (buf == NULL) ? 0xFF : 1;
            // 0A38
            do
            {
                sceKernelCpuResumeIntr(oldIntr);
                ret = sceKernelWaitEventFlag(g_audio.evFlagId, 1 << chanId, 32, 0, 0);
                if (ret < 0) {
                    K1_RESET();
                    return ret;
                }
                oldIntr = sceKernelCpuSuspendIntr();
                ret = audioOutput(chan, leftVol, rightVol, buf);
            } while ((u32)ret == ERROR_AUDIO_CHANNEL_BUSY);
            chan->unk10 = 0;
        }
    }
    // 09A8
    sceKernelCpuResumeIntr(oldIntr);
    if (chanId < (u32)(g_audio.delayShift + 5))
        sceKernelDelayThread(3000);
    // 09D0
    K1_RESET();
    return ret;
}

/**
 * Reserves a channel.
 *
 * @param channel The channel ID you want to reserve, or -1 if you want the first free one to be selected.
 * @param sampleCount The number of samples.
 * @param format The audio format (0x10 for MONO, 0 for STEREO)
 *
 * @return The channel ID in case of success, otherwise less than zero.
 */
int sceAudioChReserve(int channel, int sampleCount, int format)
{
    K1_BACKUP();
    int oldIntr = sceKernelCpuSuspendIntr();
    if (channel < 0)
    {
        // 0BC0
        // 0BD0
        for (channel = 7; channel >= 0; channel--)
        {
            if (g_audio.chans[channel].sampleCount == 0 && g_audio.chans[channel].buf == NULL)
                break;
            // 0BEC
        }
        // 0BF4
        if (channel < 0)
        {
            sceKernelCpuResumeIntr(oldIntr);
            K1_RESET();
            return ERROR_AUDIO_NO_CHANNELS_AVAILABLE;
        }
    }
    else if (channel >= 8 || g_audio.chans[channel].sampleCount != 0)
    {
        // 0B08
        sceKernelCpuResumeIntr(oldIntr);
        K1_RESET();
        return ERROR_AUDIO_INVALID_CHANNEL;
    }
    // 0B38
    if (sampleCount <= 0 || (sampleCount & 0x3F) != 0 || (u32)sampleCount > 0xFFC00000)
    {
        // 0B58
        sceKernelCpuResumeIntr(oldIntr);
        K1_RESET();
        return ERROR_AUDIO_INVALID_SAMPLECOUNT;
    }
    // 0B6C
    char bytesPerSample = 4;
    if (format != 0)
    {
        bytesPerSample = 2;
        if (format != 0x10)
        {
            // 0BAC
            sceKernelCpuResumeIntr(oldIntr);
            K1_RESET();
            return ERROR_AUDIO_INVALID_FORMAT;
        }
    }
    // 0B8C
    g_audio.chans[channel].bytesPerSample = bytesPerSample;
    g_audio.chans[channel].sampleCount = sampleCount;
    g_audio.chans[channel].leftVol = 0;
    g_audio.chans[channel].rightVol = 0;
    sceKernelCpuResumeIntr(oldIntr);
    K1_RESET();
    return channel;
}

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
int sceAudioOneshotOutput(int chanId, int sampleCount, int fmt, int leftVol, int rightVol, void *buf)
{
    K1_BACKUP();
    int oldIntr = sceKernelCpuSuspendIntr();
    if ((leftVol > 0xFFFF) || (rightVol > 0xFFFF))
    {
        // 0E30
        sceKernelCpuResumeIntr(oldIntr);
        K1_RESET();
        return ERROR_AUDIO_INVALID_VOLUME;
    }
    if (chanId < 0)
    {
        // 0DDC
        // 0DEC
        for (chanId = 7; chanId >= 0; chanId--) // 0E08
            if (g_audio.chans[chanId].sampleCount == 0 && g_audio.chans[chanId].buf == NULL)
                break;
        // 0E10
        if (chanId < 0)
        {
            sceKernelCpuResumeIntr(oldIntr);
            K1_RESET();
            return ERROR_AUDIO_NO_CHANNELS_AVAILABLE;
        }
    }
    else if (chanId >= 8 || g_audio.chans[chanId].sampleCount != 0)
    {
        // 0CA4
        sceKernelCpuResumeIntr(oldIntr);
        K1_RESET();
        return ERROR_AUDIO_INVALID_CHANNEL;
    }
    SceAudioChannel *chan = &g_audio.chans[chanId];
    // 0CE4
    if (sampleCount <= 0)
    {
        // 0DC8
        sceKernelCpuResumeIntr(oldIntr);
        K1_RESET();
        return ERROR_AUDIO_INVALID_SAMPLECOUNT;
    }
    if (!K1_USER_BUF_STA_SZ(buf, chan->sampleCount * 4))
    {
        // 0DB0
        sceKernelCpuResumeIntr(oldIntr);
        K1_RESET();
        return ERROR_AUDIO_PRIV_REQUIRED;
    }
    char bytesPerSample = 4;
    if (fmt != 0)
    {
        bytesPerSample = 2;
        if (fmt != 0x10)
        {
            // 0D98
            sceKernelCpuResumeIntr(oldIntr);
            K1_RESET();
            return ERROR_AUDIO_INVALID_FORMAT;
        }
    }
    // 0D28
    chan->leftVol = leftVol;
    chan->rightVol = rightVol;
    chan->bytesPerSample;
    chan->curSampleCnt = sampleCount;
    chan->buf = buf;
    if (((g_audio.flags & 1) == 0) && (buf != NULL))
    {
        // 0D6C
        int i;
        for (i = 0; i < 128; i++)
            ((int*)KUNCACHED(g_audio.buf0))[i] = 0;
        updateAudioBuf(0);
    }
    // 0D84
    sceKernelCpuResumeIntr(oldIntr);
    K1_RESET();
    return chanId;
}

/**
 * Releases a channel.
 *
 * @param channel The channel ID to release.
 *
 * @return 0 in case of succes, otherwise less than zero.
 */
int sceAudioChRelease(u32 channel)
{
    if (channel >= 8)
        return ERROR_AUDIO_INVALID_CHANNEL;
    K1_BACKUP();
    int oldIntr = sceKernelCpuSuspendIntr();
    SceAudioChannel *chan = &g_audio.chans[channel];
    if (chan->sampleCount == 0)
    {
        // 0EE4
        sceKernelCpuResumeIntr(oldIntr);
        K1_RESET();
        return ERROR_AUDIO_CHANNEL_NOT_RESERVED;
    }
    if (chan->unk10 != 0)
    {
        // 0ECC
        sceKernelCpuResumeIntr(oldIntr);
        K1_RESET();
        return ERROR_AUDIO_CHANNEL_BUSY;
    }
    chan->sampleCount = 0;
    sceKernelCpuResumeIntr(oldIntr);
    K1_RESET();
    return 0;
}

/**
 * Get the number of remaining unplayed samples of a channel.
 *
 * @param chanId The channel ID to check.
 *
 * @return The number of remaining samples.
 */
int sceAudioGetChannelRestLength(u32 chanId)
{
    if (chanId >= 8)
        return ERROR_AUDIO_INVALID_CHANNEL;
    SceAudioChannel *chan = &g_audio.chans[chanId];
    // 0F30
    return ((chan->buf != NULL) ? chan->curSampleCnt : 0) + ((chan->unk10 != 0) ? 0 : chan->sampleCount);
}

/**
 * Set the sample count of a channel.
 *
 * @param chanId The channel ID.
 * @param sampleCount The number of samples.
 *
 * @return 0 on success, otherwise less than zero.
 */
int sceAudioSetChannelDataLen(u32 chanId, int sampleCount)
{
    if (chanId >= 8)
        return ERROR_AUDIO_INVALID_CHANNEL;
    if (sampleCount <= 0 || (sampleCount & 0x3F) != 0 || sampleCount > 0xFFC0)
        return ERROR_AUDIO_INVALID_SAMPLECOUNT;
    // 0FB8
    K1_BACKUP();
    int oldIntr = sceKernelCpuSuspendIntr();
    SceAudioChannel *chan = &g_audio.chans[chanId];
    if (chan->unk10 != 0)
    {
        // 1018
        sceKernelCpuResumeIntr(oldIntr);
        K1_RESET();
        return ERROR_AUDIO_CHANNEL_BUSY;
    }
    if (chan->sampleCount == 0)
    {
        // 1000
        sceKernelCpuResumeIntr(oldIntr);
        K1_RESET();
        return ERROR_AUDIO_CHANNEL_NOT_INIT;
    }
    chan->sampleCount = sampleCount & 0xFFFF;
    K1_RESET();
    return 0;
}

/**
 * Change the volume of a channel.
 *
 * @param chanId The channel ID.
 * @param leftVol The left ear volume.
 * @param rightVol The right ear volume.
 *
 * @return 0 on success, otherwise less than 0
 */
int sceAudioChangeChannelVolume(u32 chanId, int leftVol, int rightVol)
{
    if ((rightVol > 0xFFFF) || (leftVol > 0xFFFF))
        return ERROR_AUDIO_INVALID_VOLUME;
    if (chanId >= 8)
        return ERROR_AUDIO_INVALID_CHANNEL;
    K1_BACKUP();
    int oldIntr = sceKernelCpuSuspendIntr();
    SceAudioChannel *chan = &g_audio.chans[chanId];
    if (leftVol >= 0)
        chan->leftVol = leftVol & 0xFFFF;
    // 10AC
    if (rightVol >= 0)
        chan->rightVol = rightVol & 0xFFFF;
    // 10B4
    sceKernelCpuResumeIntr(oldIntr);
    K1_RESET();
    return 0;
}

/**
 * Change the format (mono/stereo) of a channel.
 *
 * @param chanId The channel ID.
 * @param format The audio format (0 for STEREO, 0x10 for MONO).
 *
 * @return 0 on success, otherwise less than 0
 */
int sceAudioChangeChannelConfig(u32 chanId, int format)
{
    if (chanId >= 8)
        return ERROR_AUDIO_INVALID_CHANNEL;
    K1_BACKUP();
    int oldIntr = sceKernelCpuSuspendIntr();
    SceAudioChannel *chan = &g_audio.chans[chanId];
    if (chan->unk10 != 0)
    {
        // 116C
        sceKernelCpuResumeIntr(oldIntr);
        K1_RESET();
        return ERROR_AUDIO_CHANNEL_BUSY;
    }
    // 1164
    if ((SysMemForKernel_F0E0AB7A() > 0x01FFFFFF && (chan->curSampleCnt == 0 || chan->buf == NULL)) || chan->curSampleCnt == 0) // 1214
    {
        // 116C
        sceKernelCpuResumeIntr(oldIntr);
        K1_RESET();
        return ERROR_AUDIO_CHANNEL_BUSY;
    }
    // 11A4
    if (chan->sampleCount == 0)
    {
        // 11FC
        sceKernelCpuResumeIntr(oldIntr);
        K1_RESET();
        return ERROR_AUDIO_CHANNEL_NOT_RESERVED;
    }
    char bytesPerSample = 4;
    if (format != 0)
    {
        bytesPerSample = 2;
        if (format != 0x10)
        {
            // 11E4
            sceKernelCpuResumeIntr(oldIntr);
            K1_RESET();
            return ERROR_AUDIO_INVALID_FORMAT;
        }
    }
    // 11CC
    chan->bytesPerSample = bytesPerSample;
    sceKernelCpuResumeIntr(oldIntr);
    K1_RESET();
    return 0;
}

/**
 * Change SRC output sample count.
 *
 * @param sampleCount The sample count.
 *
 * @return 0 on success, otherwise less than zero.
 */
int sceAudioOutput2ChangeLength(int sampleCount)
{
    if (sampleCount < 17 || sampleCount > 4111)
        return ERROR_AUDIO_INVALID_SAMPLECOUNT;
    int oldIntr = sceKernelCpuSuspendIntr();
    if (g_audio.srcChFreq == 0)
    {
        // 1280
        sceKernelCpuResumeIntr(oldIntr);
        return ERROR_AUDIO_CHANNEL_NOT_RESERVED;
    }
    g_audio.srcChSampleCnt = sampleCount;
    sceKernelCpuResumeIntr(oldIntr);
    return 0;
}

/**
 * Get the number of SRC unplayed samples.
 *
 * @return The number of unplayed samples.
 */
int sceAudioOutput2GetRestSample(void)
{
    int *ptr = KUNCACHED(&g_audio.hwBuf[16]);
    if (g_audio.srcChFreq == 0)
        return ERROR_AUDIO_CHANNEL_NOT_RESERVED;
    // 12C8
    return (ptr[ 2] != 0 ? g_audio.srcChSampleCnt : 0)
        + (ptr[10] != 0 ? g_audio.srcChSampleCnt : 0);
}

/**
 * Get the number of unplayed samples of a channel.
 *
 * @param chanId The channel ID.
 *
 * @return The number of samples in case of success, otherwise less than zero.
 */
int sceAudioGetChannelRestLen(u32 chanId)
{
    if (chanId >= 8)
        return ERROR_AUDIO_INVALID_CHANNEL;
    SceAudioChannel *chan = &g_audio.chans[chanId];
    if (chan->unk10 == 0)
        return chan->curSampleCnt;
    return chan->curSampleCnt + chan->sampleCount;
}

/**
 * Reserve a SRC output.
 *
 * @param sampleCount The number of samples of the output.
 *
 * @return 0 in case of success, otherwise less than zero.
 */
int sceAudioOutput2Reserve(int sampleCount)
{
    return sceAudioSRCChReserve(sampleCount, 44100, 2);
}

/**
 * Output to SRC. This function only returns when all the samples have been played.
 *
 * @param vol The output volume.
 * @param buf The PCM audio buffer.
 *
 * @return The sample count on success, otherwise less than zero.
 */
int sceAudioOutput2OutputBlocking(int vol, void *buf)
{
    return sceAudioSRCOutputBlocking(vol, buf);
}

/**
 * Release SRC output.
 *
 * @return 0 on success, otherwise less than zero.
 */
int sceAudioOutput2Release(void)
{
    return sceAudioSRCChRelease();
}

// 137C
/*
 * Outputs audio to channel.
 *
 * channel: The pointer to the channel.
 * leftVol: The left ear volume.
 * rightVol: The right ear volume.
 * buf: The audio PCM buffer.
 *
 * Returns the number of samples on success, otherwise less than zero.
 */
int audioOutput(SceAudioChannel *channel, short leftVol, short rightVol, void *buf)
{
    if (channel->sampleCount == 0)
        return ERROR_AUDIO_CHANNEL_NOT_INIT;
    if (channel->buf != NULL)
        return ERROR_AUDIO_CHANNEL_BUSY;
    channel->curSampleCnt = channel->sampleCount;
    if (leftVol >= 0)
        channel->leftVol = leftVol;
    // 13BC
    if (rightVol >= 0)
        channel->rightVol = rightVol;
    // 13C4
    channel->buf = buf;
    if (((g_audio.flags & 1) == 1) || ((u32)buf <= 0))
        return channel->sampleCount;
    // 1400
    int i;
    for (i = 0; i < 128; i++)
        ((int*)KUNCACHED(g_audio.buf0))[i] = 0;
    updateAudioBuf(0);
    return channel->sampleCount;
}

/**
 * Sets the audio output frequency.
 *
 * @param freq The frequency (44100 or 48000).
 *
 * @return 0 on success, otherwise less than zero.
 */
int sceAudioSetFrequency(int freq)
{
    if (g_audio.freq == freq)
        return 0;
    int hwFreq;
    if (freq == 44100) {
        // 1510
        hwFreq = 128;
    }
    else if (freq == 48000)
        hwFreq = 256;
    else {
        // 1504
        return ERROR_AUDIO_INVALID_FREQUENCY;
    }
    // 1470
    int oldIntr = sceKernelCpuSuspendIntr();
    AUDIO_SET_BUSY(1);
    g_audio.freq = freq;
    *(int*)(0xBE000004) = 0;
    // 149C
    while (*(int*)(0xBE00000C) != 0)
        ;
    *(int*)(0xBE000040) = 1;
    *(int*)(0xBE000038) = hwFreq;
    *(int*)(0xBE00003C) = hwFreq;
    *(int*)(0xBE000004) = g_audio.flags;
    if (g_audio.flags == 0)
        AUDIO_SET_BUSY(0);
    // 14D0
    sceKernelCpuResumeIntr(oldIntr);
    sceClockgenAudioClkSetFreq(freq);
    sceCodec_driver_431C0C8E(freq);
    return 0;
}

/**
 * Inits audio.
 */
int sceAudioInit()
{
    memset(&g_audio, 0, sizeof(g_audio));
    // 1558
    int i;
    for (i = 0; i < 3; i++)
        g_audio.dmaPtr[i] = sceKernelDmaOpAlloc();
    g_audio.evFlagId = sceKernelCreateEventFlag(0x34E4, 513, 0, 0);
    g_audio.freq = 44100;
    g_audio.volumeOffset = 8;
    i = 0;
    // 15FC
    do
    {
        int shift = ((i + (i >> 31)) >> 1) << 8;
        int *buf = &g_audio.hwBuf[i * 4];
        buf[0] = (int)UCACHED(g_audio.buf0 + shift);
        buf[1] = (int)UCACHED(0xBE000060);
        buf[2] = (int)UCACHED(g_audio.hwBuf[i * 4 + 4]);
        buf[3] = 0x0448903C;
        buf[4] = (int)UCACHED(g_audio.buf240 + shift);
        buf[5] = (int)UCACHED(0xBE000060);
        buf[6] = 0;
        buf[7] = 0x84489004;

        buf[17] = (int)UCACHED(0xBE000070);
        buf[18] = 0;

        buf[21] = (int)UCACHED(0xBE000070);
        buf[22] = 0;

        buf[32] = (int)UCACHED(0xBE000080);
        buf[33] = (int)UCACHED(g_audio.buf512 + shift);
        buf[34] = (int)UCACHED(g_audio.hwBuf[i * 4 + 36]);
        buf[35] = 0x0848903C;
        buf[36] = (int)UCACHED(0xBE000080);
        buf[37] = (int)UCACHED(g_audio.buf752 + shift);
        buf[38] = 0;
        buf[39] = 0x88489004;
        i += 2;
    } while (i < 4);
    sceClockgenAudioClkSetFreq(g_audio.freq);
    audioHwInit();
    sceCodec_driver_431C0C8E(g_audio.freq);
    SceUID id = sceKernelCreateThread("SceAudioMixer" /* 0x34F0 */, audioMixerThread, 5, 0x600, 0x100000, 0);
    if (id < 0 || sceKernelStartThread(id, 0, 0) != 0)
        return 1;
    // 1724
    SceUID id2 = sceKernelCreateThread("SceAudioInput", audioInputThread, 6, 0x400, 0x100000, 0);
    if (id2 < 0 || sceKernelStartThread(id2, 0, 0) != 0)
        return 1;
    sceKernelRegisterSysEventHandler(g_audioEvent);
    sceKernelRegisterIntrHandler(10, 2, audioIntrHandler, 0, 0);
    sceKernelEnableIntr(10);
    sceKernelDcacheWritebackInvalidateRange(g_audio, 1400);
    *(int*)(g_audio.dmaPtr[0] + 8) = UCACHED(&g_audio);
    g_audio.unkCodecArg = 1;
    g_audio.unkCodecArgSet = 0;
    g_audio.inputInited = 0;
    return 0;
}

/**
 * \todo ?
 */
int sceAudioLoopbackTest(int arg0)
{
    if (arg0 == 0)
    {
        // 1854
        g_audio.flags = 0;
        audioHwInit();
        sceCodec_driver_B2EF6B19(0);
    }
    else
    {
        int oldIntr = sceKernelCpuSuspendIntr();
        g_audio.flags = 7;
        AUDIO_SET_BUSY(1);
        sceCodec_driver_B2EF6B19(1);
        *(int*)(0xBE000004) = 7;
        *(int*)(0xBE00002C) = 7;
        *(int*)(0xBE000010) = 4;
        *(int*)(0xBE000008) = 0;
        *(int*)(0xBE000020) = 7;
        *(int*)(0xBE000024) = 0;
        sceKernelCpuResumeIntrWithSync(oldIntr);
    }
    return 0;
}

/**
 * Frees the audio system.
 */
int sceAudioEnd()
{
    sceCodecOutputEnable(0, 0);
    sceCodec_driver_E4D7F914();
    sceKernelUnregisterSysEventHandler(g_audioEvent);
    int oldIntr = sceKernelCpuSuspendIntr();
    sceKernelDeleteEventFlag(g_audio.evFlagId);
    sceKernelReleaseIntrHandler();
    // 18CC
    int i;
    for (i = 0; i < 3; i++) {
        sceKernelDmaOpQuit(g_audio.dmaPtr[i]);
        sceKernelDmaOpFree(g_audio.dmaPtr[i]);
    }
    sceKernelCpuResumeIntr(oldIntr);
    return 0;
}

/**
 * Selects the delaying mode.
 *
 * @param arg If set to 1, channels 0 - 6 will be delayed after sceAudioOutputPannedBlocking, otherwise only channels 0 - 4 will be.
 *
 * @return 0.
 */

int sceAudio_driver_306D18F1(int arg)
{
    g_audio.delayShift = (arg != 0) ? 2 : 0;
    return 0;
}

/**
 * Sets the volume offset/shifting.
 *
 * @param arg The output buffer will be shifted by (arg + 8); default is 8, so arg = 0 is the default value.
 *
 * @return 0.
 */
int sceAudioSetVolumeOffset(int arg)
{
    // 1948
    int ret = sceCodec_driver_F071BF60((arg >= 0) ? arg * 6 : -128);
    if (ret < 0)
        return ret;
    g_audio.volumeOffset = arg + 8;
    return 0;
}

// 1970
/*
 * The audio interrupt handler.
 *
 * Returns -1.
 */
int audioIntrHandler()
{
    int oldIntr = sceKernelCpuSuspendIntr();
    char attr = g_audio.flags;
    char hwAttr = *(int*)(0xBE00001C) & attr;
    if (hwAttr != 0)
    {
        // 1A00
        attr = attr - hwAttr;
        sceKernelSetEventFlag(g_audio.evFlagId, hwAttr << 29);
        if ((hwAttr & 1) != 0)
        {
            // 1A8C
            sceKernelDmaOpQuit(g_audio.dmaPtr[0]);
            *(int*)(g_audio.dmaPtr[0] + 8) = (u32)UCACHED(g_audio.buf240 + 16);
        }
        // 1A1C
        if ((hwAttr & 2) != 0)
        {
            // 1A6C
            sceKernelDmaOpQuit(g_audio.dmaPtr[1]);
            int *ptr = KUNCACHED(&g_audio.hwBuf[16]);
            ptr[2] = 0;
            ptr[10] = 0;
        }
        // 1A24
        if ((hwAttr & 4) != 0)
        {
            // 1A38
            sceKernelDmaOpQuit(g_audio.dmaPtr[2]);
            int *ptr = KUNCACHED(&g_audio.hwBuf[32]);
            ptr[2] = 0;
            ptr[10] = 0;
            sceKernelClearEventFlag(g_audio.evFlagId, 0x7FFFFFFF);
        }
        g_audio.flags = attr;
    }
    // 19B4
    *(int*)(0xBE000024) = attr;
    if (attr == 0)
    {
        // 19EC
        AUDIO_SET_BUSY(0);
        sceCodec_driver_B2EF6B19(0);
    }
    // 19C4
    sceKernelCpuResumeIntrWithSync(oldIntr);
    return -1;
}

// 1AAC
/*
 * (Re)inits the audio hardware & sysreg.
 */
void audioHwInit()
{
    sceSysreg_driver_3A98CABB(0, 1);
    sceSysreg_driver_789597AB(0);
    sceSysreg_driver_8E9E76AE(0);
    sceSysreg_driver_D1999F94(0);
    sceSysreg_driver_9E80B4E2(0);
    sceSysreg_driver_9984A972();
    AUDIO_SET_BUSY(1);
    *(int*)(0xBE000004) = 0;
    // 1B00
    while ((*(int*)(0xBE00000C) & 7) != 0)
        ;
    *(int*)(0xBE00002C) = 7;
    // 1B20
    while ((*(int*)(0xBE000028) & 0x30) != 0x30)
        ;
    *(int*)(0xBE000024) = 0;
    *(int*)(0xBE000020) = 7;
    *(int*)(0xBE000008) = 7;
    *(int*)(0xBE000014) = 0x1208;
    // 1B50
    while ((*(int*)(0xBE000050) & 0x30) != 0)
        ;
    *(int*)(0xBE000050) = 0x8000;
    g_audio.srcVol = 0x0400;
    *(int*)(0xBE000018) = 0;
    if (g_audio.freq == 48000)
    {
        // 1BF0
        *(int*)(0xBE000038) = 0x100;
    }
    else {
        g_audio.freq = 44100;
        *(int*)(0xBE000038) = 0x80;
    }
    // 1BA0
    short v = g_audio.hwFreq;
    if (v == 0) {
        // 1BE0
        *(int*)(0xBE000040) = 5;
    }
    else
    {
        *(int*)(0xBE000040) = 4;
        *(int*)(0xBE000044) = v;
        // 1BBC
        while ((*(int*)(0xBE000040) & 2) != 0)
            ;
    }
    // 1BD0
    AUDIO_SET_BUSY(0);
}

// 1C00
/*
 * The audio event handler.
 *
 * Returns 0.
 */
int audioEventHandler(int ev_id, char* ev_name, void* param, int* result)
{
    switch (ev_id)
    {
    case 0x1000F:
        // 1D38
        audioHwInit();
        break;
    case 0x100000:
        // 1CE8
        sceCodec_driver_431C0C8E(g_audio.freq);
        if (g_audio.inputInited != 0)
        {
            sceCodec_driver_9681738F(g_audio.unkCodecArg);
            sceCodec_driver_B0141A1B(g_audio.unkInput0, g_audio.inputGain, g_audio.unkInput2, g_audio.unkInput3, g_audio.unkInput4, g_audio.unkInput5);
        }
        break;
    case 0x1000:
        {
        // 1C4C
        // 1C58
        int i;
        for (i = 0; i < 3; i++) {
            sceKernelDmaOpQuit(g_audio.dmaPtr[i]);
            sceKernelDmaOpDeQueue(g_audio.dmaPtr[i]);
        }
        int *ptr = KUNCACHED(&g_audio.hwBuf[16]);
        ptr[2] = 0;
        ptr[10] = 0;
        AUDIO_SET_BUSY(1);
        *(int*)(0xBE000004) = 1;
        sceKernelSetEventFlag(g_audio.evFlagId, (g_audio.inputCurSampleCnt != 0) ? 0x60000000 : 0xE0000000);
        g_audio.flags = 0;
        *(int*)(0xBE000004) = 0;
        *(int*)(0xBE000008) = 0;
        SYNC();
        sceSysregAudioIoDisable(0);
        sceSysregAudioClkoutIoDisable();
        break;
        }
    }
    return 0;
}

// 1D48
/*
 * The DMA callback for SRC output.
 */
int audioSRCOutputDmaCb(int arg0, int arg1)
{
    if (arg1 != 0)
    {
        // 1DD0
        dmaUpdate(1);
        return -1;
    }
    int shift = (*(int*)(arg0 + 40) + 0x7FFFFBB0 - (u32)&g_audio < 32) ? 8 : 0; // ?
    int *ptr1 = KUNCACHED(&g_audio.hwBuf[18 + shift]);
    *ptr1 = 0;
    int *ptr2 = UUNCACHED(&g_audio.hwBuf[26]);
    if (*(int*)(arg0 + 40) == 0)
        *ptr2 = 0;
    // 1DB8
    sceKernelSetEventFlag(g_audio.evFlagId);
    return 0;
}

/**
 * Reserves a SRC output.
 *
 * @param sampleCount The number of samples.
 * @param freq The output frequency.
 * @param numChans The number of output "channels" (only 2 is accepted, for stereo)
 *
 * @return 0 on success, otherwise less than zero.
 */
int sceAudioSRCChReserve(int sampleCount, int freq, int numChans)
{
    if (numChans != 4 && numChans != 2)
        return ERROR_SIZE;
    if (numChans == 4)
        return 0x80000003;
    int totalSamples = sampleCount * numChans;
    int wut = (int)(totalSamples + ((u32)totalSamples >> 31)) >> 1;
    if (wut < 17 || wut > 4111)
        return ERROR_SIZE;
    int hwFreq;
    if (freq == 0 || g_audio.freq == freq)
    {
        // 2044
        freq = g_audio.freq;
        hwFreq = 0;
    }
    else
    {
        if (g_audio.freq == 48000)
        {
            // 2030
            *(int*)(0xBE000038) = 128;
            *(int*)(0xBE00003C) = 128;
        }

        // 1E84
        switch (freq)
        {
        case 8000:
            hwFreq = 1;
            break;
        case 11025:
            hwFreq = 2;
            break;
        case 12000:
            hwFreq = 4;
            break;
        case 16000: // 2028
            hwFreq = 8;
            break;
        case 22050:
            hwFreq = 16;
            break;
        case 24000: // 2020
            hwFreq = 32;
            break;
        case 32000:
            hwFreq = 64;
            break;
        case 48000:
            hwFreq = 256;
            break;
        default:
            // 1EB4
            return ERROR_AUDIO_INVALID_FREQUENCY;
        }
    }
    // 1EDC
    int oldIntr = sceKernelCpuSuspendIntr();
    if (g_audio.srcChFreq != 0)
    {
        // 1FBC
        sceKernelCpuResumeIntr(oldIntr);
        return 0x80268002;
    }
    g_audio.numChans = numChans << 1;
    g_audio.srcChSampleCnt = sampleCount;
    g_audio.srcChFreq = freq;
    AUDIO_SET_BUSY(1);
    if ((g_audio.flags & 2) != 0)
    {
        *(int*)(0xBE000004) = (int)(g_audio.flags ^ 2);
        // 1F30
        while ((*(int*)(0xBE00000C) & 2) != 0)
            ;
    }
    // 1F44
    g_audio.hwFreq = hwFreq;
    if (hwFreq == 0) {
        // 1FA8
        *(int*)(0xBE000040) = 5;
    }
    else
    {
        *(int*)(0xBE000040) = 4;
        *(int*)(0xBE000044) = hwFreq;
        // 1F5C
        while ((*(int*)(0xBE000040) & 2) != 0)
            ;
    }
    // 1F70
    *(int*)(0xBE00002C) = 2;
    if (g_audio.flags != 0)
    {
        // 1F9C
        *(int*)(0xBE000004) = g_audio.flags;
    }
    else
        AUDIO_SET_BUSY(0);
    // 1F8C
    sceKernelCpuResumeIntr(oldIntr);
    return 0;
}

/**
 * Releases a SRC output.
 *
 * @return 0 on success, otherwise less than zero.
 */
int sceAudioSRCChRelease(void)
{
    int oldIntr = sceKernelCpuSuspendIntr();
    int *ptr = KUNCACHED(&g_audio.hwBuf[16]);
    if (g_audio.srcChFreq == 0) {
        sceKernelCpuResumeIntr(oldIntr);
        return ERROR_AUDIO_CHANNEL_NOT_RESERVED;
    }
    // 20C0
    if ((ptr[2] | ptr[10]) != 0) {
        sceKernelCpuResumeIntr(oldIntr);
        return 0x80268002;
    }
    // 20DC
    sceKernelSetEventFlag(g_audio.evFlagId, 0x40000000);
    g_audio.srcChFreq = 0;
    sceKernelCpuResumeIntr(oldIntr);
    return 0;
}

/**
 * Outputs to SRC. The functions returns only when all the buffer has been played.
 *
 * @param vol The output volume (0 - 0xFFFF).
 * @param buf The audio PCM buffer.
 *
 * @return The sample count on success, otherwise less than zero.
 */
int sceAudioSRCOutputBlocking(int vol, void *buf)
{
    if (vol > 0xFFFFF)
        return ERROR_AUDIO_INVALID_VOLUME;
    K1_BACKUP();
    if (!K1_USER_BUF_STA_SZ(buf, g_audio.srcChSampleCnt * g_audio.numChans))
    {
        // 224C
        K1_RESET();
        return ERROR_AUDIO_PRIV_REQUIRED;
    }
    int oldIntr = sceKernelCpuSuspendIntr();
    int ret = audioSRCOutput(vol, buf);
    sceKernelCpuResumeIntr(oldIntr);
    if (ret > 0 || (ret == 0 && (g_audio.flags & 2) != 0)) // 222C
    {
        // 2190
        int ret2 = sceKernelWaitEVentFlag(g_audio.evFlagId, 0x40000000, 32, 0, 0);
        ret = (ret2 < 0) ? ret : ret2;
    }
    // 21AC
    int ret2 = sceCodec_driver_55F1788B();
    if (ret2 == 0) {
        K1_RESET();
        return 0;
    }
    if (g_audio.freq == ret2) {
        K1_RESET();
        return ret2;
    }
    sceAudioSetFrequency(ret2);
    oldIntr = sceKernerCpuSuspendIntr();
    short freq = g_audio.srcChFreq;
    g_audio.srcChFreq = 0;
    sceAudioSRCChReserve(g_audio.srcChSampleCnt, freq, g_audio.numChans >> 1);
    sceKernelCpuResumeIntr(oldIntr);
    K1_RESET();
    return ret;
}

/*
 * Outputs audio to SRC output.
 *
 * vol: The volume (0 - 0xFFFF)
 * buf: The audio PCM buffer.
 *
 * Returns the sample count on success, otherwise less than zero.
 */
int audioSRCOutput(int vol, void *buf)
{
    int *ptr = KUNCACHED(&g_audio.hwBuf[16]);
    if (g_audio.srcChFreq == 0)
        return ERROR_AUDIO_CHANNEL_NOT_RESERVED;
    if (ptr[2] != 0)
    {
        if (ptr[10] != 0)
            return ERROR_AUDIO_CHANNEL_BUSY;
        ptr += 8;
    }
    // 22C4
    if (buf == NULL)
        return 0;
    int numSamples = g_audio.srcChSampleCnt * g_audio.numChans;
    if (((0xD3 >> (((u32)buf >> 29) & 7)) & 1) != 0)
    {
        // 2440
        sceKernelDcacheWritebackRange(buf, numSamples);
    }
    // 2300
    int samplesHi = (numSamples > 16396) ? (numSamples - 16380) : 16;
    int diff = numSamples - samplesHi;
    int k1;
    GET_REG(k1, K1);
    void *newBuf = (void*)((((k1 >> 31) << 30) & 0xE0000000) | (u32)UCACHED(buf));
    ptr[3] = (diff      >> 2) | 0x04489000;
    ptr[2] = ((int)(void*)ptr & 0x1FFFFFFF) + 16;
    ptr[4] = (u32)newBuf + diff;
    ptr[7] = (samplesHi >> 2) | 0x84489000;
    ptr[0] = (u32)newBuf;
    ptr[6] = 0;
    int wait;
    if ((g_audio.flags & 2) == 0)
    {
        // 2430
        updateAudioBuf(1);
        wait = 1;
    }
    else
    {
        *(int*)(((int)(void*)ptr ^ 0x20) + 24) = (u32)ptr;
        wait = 0;
        sceDdrFlush(4);
    }
    // 2384
    int volHi = vol >> 5;
    if (g_audio.srcVol != volHi)
    {
        // 23C4
        int oldTime = sceKernelGetSystemTimeLow();
        g_audio.srcVol = volHi;
        if (wait)
        {
            // 23FC
            while (sceKernelGetSystemTimeLow() - oldTime < 25)
                ;
            // 2414
            while ((*(int*)(0xBE00000C) & 2) == 0)
                ;
        }
        // 23D8
        while ((*(int*)(0xBE000050) & 0x10000) != 0)
            ;
        *(int*)(0xBE000050) = volHi;
    }
    // 239C
    return g_audio.srcChSampleCnt;
}

// 2454
/*
 * The audio input thread.
 *
 * Returns 0.
 */
int audioInputThread()
{
    // 2478
    for (;;)
    {
        // 247C
        if (sceKernelWaitEventFlag(g_audio.evFlagId, 0x80000000, 33, 0, 0) < 0)
        {
            // 26CC
            sceKernelExitThread(0);
            return 0;
        }
        int *ptr1 = g_audio.dmaPtr[2];
        int curSampleCount = g_audio.inputCurSampleCnt;
        int unk = (ptr1[9] < g_audio.buf752[0x80000000 + 16]); // yes, it's correct, it reverses the kernel mode
        short *uncached1 = KUNCACHED(&g_audio.buf512[unk << 8]);
        char unk2 = g_audio.inputHwFreq;
        short *ptr3 = g_audio.inputBuf;
        if (curSampleCount == 0)
        {
            // 2648
            if (g_audio.inputIsWaiting == 0 && g_audio.unkCodecArgSet == 0)
            {
                int oldIntr = sceKernelCpuSuspendIntr();
                *(int*)(0xBE000004) = (int)(char)(g_audio.flags & 0xFB);
                g_audio.flags &= 0xFB;
                // 268C
                while ((*(int*)(0xBE00000C) & 4) != 0)
                    ;
                sceKernelDmaOpQuit(g_audio.dmaPtr[2]);
                *(int*)(0xBE000008) = g_audio.flags;
                sceKernelCpuResumeIntr(oldIntr);
                sceCodec_driver_E4D7F914();
            }
        }
        else
        {
            if (ptr3 != NULL)
            {
                uncached1 = (g_audio.unkCodecRet != 0) ? uncached1 : uncached1 + 2;
                // 24F4
                int i;
                for (i = 128; i > 0; i -= unk2)
                {
                    int v = uncached1[0];
                    if (unk2 >= 4)
                    {
                        v += uncached1[2];
                        if (unk2 >= 8)
                            v += uncached1[4] + uncached1[6];
                    }
                    // 251C
                    *(ptr3++) = v >> ((unk2 >> 2) & 0x1F);
                    uncached1 += unk2;
                }
                g_audio.inputBuf = ptr3;
            }
            // 2538
            curSampleCount -= (0x40 >> ((unk2 >> 2) & 0x1F));
            if (curSampleCount <= 0)
            {
                // 2630
                curSampleCount = 0;
                sceKernelSetEventFlag(g_audio.evFlagId, 0x100);
                g_audio.inputBuf = NULL;
            }
            // 254C
            g_audio.inputCurSampleCnt = curSampleCount;
            int oldIntr = sceKernelCpuSuspendIntr();
            if (g_audio.inputInited != 0)
            {
                // 257C
                int shift = unk * 8;
                int *uncached2 = KUNCACHED(&g_audio.hwBuf[34 + shift]);
                int *uncached3 = KUNCACHED(&g_audio.hwBuf[38 + shift]);
                *uncached3 = 0;
                *uncached2 = UCACHED(&g_audio.hwBuf[36 + shift]);
                if (DmacManForKernel_E18A93A5(ptr1, UCACHED(&g_audio.hwBuf[32 + shift])) < 0)
                {
                    *(int*)(0xBE000004) = (int)(char)(g_audio.flags & 0xFB);
                    g_audio.flags &= 0xFB;
                    SYNC();
                    // 2600
                    while ((*(int*)(0xBE00000C) & 4) != 0)
                        ;
                    sceKernelDmaOpQuit(g_audio.dmaPtr[2]);
                    ptr1[2] = g_audio.flags;
                    audioInputSetup();
                }
            }
            else
                g_audio.inputCurSampleCnt = 0;
            // 256C
            sceKernelCpuResumeIntr(oldIntr);
        }
    }
}

/**
 * Waits for the input to end.
 *
 * @return 0 on success, otherwise less than 0.
 */
int sceAudioWaitInputEnd()
{
    int ret = 0;
    if (g_audio.inputOrigSampleCnt == 0)
        return ERROR_AUDIO_CHANNEL_NOT_INIT;
    int oldIntr = sceKernelCpuSuspendIntr();
    if (g_audio.inputIsWaiting != 0)
    {
        // 27AC
        sceKernelCpuResumeIntr(oldIntr);
        return ERROR_AUDIO_INPUT_BUSY;
    }
    g_audio.inputIsWaiting = 1;
    sceKernelCpuResumeIntr(oldIntr);
    K1_BACKUP();
    // 275C
    while (g_audio.inputCurSampleCnt != 0)
        ret = sceKernelWaitEventFlag(g_audio.evFlagId, 0x100, 0x20, 0, 0);
    // 2780
    g_audio.inputIsWaiting = 0;
    K1_RESET();
    return ret;
}

/**
 * Inits audio input.
 *
 * @param param The structure containing the input parameters.
 *
 * @return 0 on success, otherwise less than zero.
 */
int sceAudioInputInitEx(SceAudioInputParams *param)
{
    K1_BACKUP();
    if (!K1_USER_BUF_STA_SZ(param, 24)) {
        K1_RESET();
        return ERROR_AUDIO_PRIV_REQUIRED;
    }
    int ret = audioInputInit(param->unk0, param->gain, param->unk2, param->unk3, param->unk4, param->unk5);
    K1_RESET();
    return ret;
}

/**
 * Inits audio input (probably deprecated, has less parameters than sceAudioInputInitEx()).
 *
 * @param arg0 \todo ?
 * @param gain The input gain.
 * @param arg2 \todo ?
 *
 * @return 0 on success, otherwise less than zero.
 */
int sceAudioInputInit(int arg0, int gain, int arg2)
{
    K1_BACKUP();
    int ret = audioInputInit(arg0, gain, arg2, 0, 3, 2);
    K1_RESET();
    return ret;
}

/**
 * Waits for the input to end, and store input.
 *
 * @param sampleCount The number of samples to read.
 * @param freq The input frequency.
 * @param buf The audio PCM input buffer.
 *
 * @return The number of played samples on success, otherwise less than zero.
 */
int sceAudioInputBlocking(int sampleCount, int freq, void *buf)
{
    sceAudioWaitInputEnd();
    return audioInput(sampleCount, freq, buf);
}

/**
 * Store input.
 *
 * @param unk1 \todo ?
 * @param gain The input gain.
 * @param unk2 \todo ?
 *
 * @return The number of played samples on success, otherwise less than zero.
 */
int sceAudioInput(int unk1, int gain, int unk2)
{
    return audioInput(unk1, gain, unk2);
}

/**
 * Get the number of samples read from input.
 *
 * @return The sample count.
 */
int sceAudioGetInputLength()
{
    int origSampleCount = g_audio.inputOrigSampleCnt;
    if (origSampleCount == 0)
        return ERROR_AUDIO_CHANNEL_NOT_INIT;
    return origSampleCount - g_audio.inputCurSampleCnt;
}

/**
 * Checks if the input has ended.
 *
 * @return 1 if the input is still running, otherwise 0.
 */
int sceAudioPollInputEnd()
{
    if (g_audio.inputInited == 0)
        return 0x80000001;
    if (g_audio.inputOrigSampleCnt == 0)
        return 0;
    return (g_audio.inputCurSampleCnt > 0);
}

/**
 * \todo ?
 */
int sceAudio_driver_37660887(int arg)
{
    if (g_audio.unkCodecArg != arg) {
        g_audio.unkCodecArgSet = 1;
        g_audio.unkCodecArg = arg;
    }
    return 0;
}

// 2948
/*
 * Setups audio input hardware.
 *
 * Returns 0 on success, otherwise less than zero.
 */
int audioInputSetup()
{
    sceKernelDmaOpQuit(g_audio.dmaPtr[2]);
    int ret = sceKernelDmaOpAssign(g_audio.dmaPtr[2], 0xFF, 0xFF, 0xD00F, 0);
    if (ret != 0)
        return ret;
    // 299C
    ret = sceKernelDmaOpSetCallback(g_audio.dmaPtr[2], audioInputDmaCb, 0);
    if (ret != 0)
        return ret;
    int *ptr = KUNCACHED(&g_audio.hwBuf[32]);
    // 29C8
    int i;
    for (i = 0; i < 3; i++)
        ptr[i * 4 + 2] = UCACHED(&g_audio.hwBuf[36 + i * 4]);
    ptr[14] = 0;
    ret = sceKernelDmaOpSetupLink(g_audio.dmaPtr[2], 0xD00F, &g_audio.hwBuf[32]);
    if (ret != 0)
        return ret;
    char flags = g_audio.flags;
    if (flags == 0)
    {
        // 2A68
        AUDIO_SET_BUSY(1);
        sceCodec_driver_B2EF6B19(1);
    }
    // 2A10
    *(int*)(0xBE000024) = (char)((flags | 4) & 0xFF) - 4;
    g_audio.flags = flags | 4;
    *(int*)(0xBE00002C) = 4;
    *(int*)(0xBE000020) = 4;
    SYNC();
    ret = sceKernelDmaOpEnQueue(g_audio.dmaPtr[2]);
    *(int*)(0xBE000008) = g_audio.flags;
    *(int*)(0xBE000004) = g_audio.flags | 1;
    *(int*)(0xBE000024) = g_audio.flags;
    SYNC();
    return ret;
}

// 2A80
/*
 * Stores input.
 *
 * sampleCount: The number of samples to store.
 * freq: The audio input frequency.
 * buf: The audio PCM buffer.
 *
 * Returns the number of played samples in case of success, otherwise less than zero.
 */
int audioInput(int sampleCount, int freq, void *buf)
{
    int ret = 0;
    if (g_audio.inputInited == 0)
        return 0x80000001;
    if (sampleCount <= 0 || (sampleCount & 0x3F) != 0) {
        // 2AD8
        return ERROR_AUDIO_INVALID_SAMPLECOUNT;
    }
    int hwFreq;
    // 2B08
    switch (freq)
    {
    case 11025:
        hwFreq = 8;
        break;
    case 22050:
        // 2C90
        hwFreq = 4;
        break;
    case 44100:
        // 2C80
        hwFreq = 2;
        break;
    default:
        // 2B24
        return ERROR_AUDIO_INVALID_FREQUENCY;
    }
    // 2B30
    if (buf == NULL)
    {
        // 2C6C
        g_audio.inputOrigSampleCnt = 0;
        g_audio.inputBuf = NULL;
        return 0;
    }
    K1_BACKUP();
    if (!K1_USER_BUF_DYN_SZ(buf, sampleCount * 2))
    {
        // 2C5C
        K1_RESET();
        return ERROR_AUDIO_PRIV_REQUIRED;
    }
    int oldIntr = sceKernelCpuSuspendIntr();
    if (g_audio.inputCurSampleCnt != 0)
    {
        // 2C44
        sceKernelCpuResumeIntr(oldIntr);
        K1_RESET();
        return ERROR_AUDIO_INPUT_BUSY;
    }
    g_audio.inputBuf = buf;
    g_audio.inputHwFreq = hwFreq;
    g_audio.inputOrigSampleCnt = sampleCount;
    g_audio.inputCurSampleCnt = sampleCount;
    sceKernelCpuResumeIntr(oldIntr);
    if ((g_audio.flags & 4) == 0)
    {
        // 2C04
        ret = sceCodec_driver_9681738F(g_audio.unkCodecArg);
        if (ret < 0) {
            K1_RESET();
            return ret;
        }
        int oldIntr = sceKernelCpuSuspendIntr();
        ret = audioInputSetup();
        sceKernelCpuResumeIntr(oldIntr);
        if (ret < 0) {
            K1_RESET();
            return ret;
        }
    }
    // 2B90
    if (g_audio.unkCodecArgSet != 0)
    {
        // 2BB0
        int ret = sceCodec_driver_9681738F(g_audio.unkCodecArg);
        if (ret >= 0)
        {
            g_audio.unkCodecRet = ret & 1;
            if ((ret & 4) != 0)
                *(int*)(0xBE0000D0) = (ret >> 1) & 1;
        }
        // 2BD8
        ret = sceCodec_driver_B0141A1B(g_audio.unkInput0, g_audio.inputGain, g_audio.unkInput2, g_audio.unkInput3, g_audio.unkInput4, g_audio.unkInput5);
        g_audio.unkCodecArgSet = 0;
    }
    // 2B9C
    if (ret >= 0)
        ret = sampleCount;
    K1_RESET();
    return ret;
}

// 2C98
/*
 * Inits audio input.
 *
 * arg0: TODO ?
 * gain: The audio input gain.
 * arg2: ?
 * arg3: ?
 * arg4: ?
 * arg5: ?
 *
 * Returns 0 on success, otherwise less than zero.
 */
int audioInputInit(int arg0, int gain, int arg2, int arg3, int arg4, int arg5)
{
    g_audio.unkInput0 = MIN(MAX(arg0, -29),  0);
    g_audio.inputGain = MIN(MAX(gain, -18), 30);
    g_audio.unkInput2 = MIN(MAX(arg2, -76),  0);
    g_audio.unkInput3 = MIN(MAX(arg3,   0), 15);
    g_audio.unkInput4 = MIN(MAX(arg4,   0), 10);
    g_audio.unkInput5 = MIN(MAX(arg5,   0), 10);
    int ret = sceCodec_driver_9681738F(g_audio.unkCodecArg);
    if (ret < 0)
        return ret;
    g_audio.unkCodecRet = ret & 1;
    sceCodec_driver_B0141A1B(g_audio.unkInput0, g_audio.inputGain, g_audio.unkInput2, g_audio.unkInput3, g_audio.unkInput4, g_audio.unkInput5);
    if (ret < 0)
        return ret;
    g_audio.inputInited = 1;
    return 0;
}

/*
 * The DMA callback for input.
 */
int audioInputDmaCb(int arg0, int arg1)
{
    if (arg1 != 0)
    {
        // 2DD0
        dmaUpdate(2);
        return -1;
    }
    int *ptr = KUNCACHED(&g_audio.hwBuf[34] + ((*(int*)(arg0 + 40) + 0x80000000 - (u32)(&g_audio.hwBuf[36]) < 32) << 5));
    *ptr = 0;
    sceKernelSetEventFlag(g_audio.evFlagId, 0x80000000);
    return 0;
}

/** @} */

