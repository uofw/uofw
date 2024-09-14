#include <common_imp.h>

#include <avcodec_audiocodec.h>
#include <usersystemlib_kernel.h>

#include "libatrac3plus.h"

SCE_MODULE_INFO("sceATRAC3plus_Library", SCE_MODULE_ATTR_EXCLUSIVE_LOAD | SCE_MODULE_ATTR_EXCLUSIVE_START, 1, 5);
SCE_MODULE_BOOTSTART("sceAtracStartEntry");
SCE_MODULE_STOP("sceAtracEndEntry");
SCE_SDK_VERSION(0x06060010);
SCE_MODULE_START_THREAD_PARAMETER(3, 0x20, 0x0400, 0);
SCE_MODULE_STOP_THREAD_PARAMETER(3, 0x20, 0x0400, 0);

typedef struct
{
    u16 unk0;
    char unk2;
    char unk3;
} SceAtracUnk;

// 3E74
SceAtracUnk g_3E74[] = {
    { 0x00C0, 0x1, 0xE },
    { 0x0098, 0x1, 0xF },
    { 0x0180, 0x2, 0x4 },
    { 0x0130, 0x2, 0x6 },
    { 0x00C0, 0x2, 0xB }
};

// 3E88
SceAtracUnk g_3E88[] = {
    { 0x00C0, 0x1, 0x0 },
    { 0x1724, 0x0, 0x0 },
    { 0x0118, 0x1, 0x0 },
    { 0x2224, 0x0, 0x0 },
    { 0x0178, 0x1, 0x0 },
    { 0x2E24, 0x0, 0x0 },

    { 0x0230, 0x1, 0x0 },
    { 0x4524, 0x0, 0x0 },
    { 0x02E8, 0x1, 0x0 },
    { 0x5C24, 0x0, 0x0 },

    { 0x0118, 0x2, 0x0 },
    { 0x2228, 0x0, 0x0 },
    { 0x0178, 0x2, 0x0 },
    { 0x2E28, 0x0, 0x0 },

    { 0x0230, 0x2, 0x0 },
    { 0x4528, 0x0, 0x0 },
    { 0x02E8, 0x2, 0x0 },
    { 0x5C28, 0x0, 0x0 },

    { 0x03A8, 0x2, 0x0 },
    { 0x7428, 0x0, 0x0 },
    { 0x0460, 0x2, 0x0 },
    { 0x8B28, 0x0, 0x0 },
    
    { 0x05D0, 0x2, 0x0 },
    { 0xB928, 0x0, 0x0 },
    { 0x0748, 0x2, 0x0 },
    { 0xE828, 0x0, 0x0 },

    { 0x0800, 0x2, 0x0 },
    { 0xFF28, 0x0, 0x0 }
};

// 3EF8
u8 g_at3PlusGUID[] = { 0xBF, 0xAA, 0x23, 0xE9, 0x58, 0xCB, 0x71, 0x44, 
                       0xA1, 0x19, 0xFF, 0xFA, 0x01, 0xE4, 0xCE, 0x62 };

// 3F08
SceAtracUnk g_3F08[] = {
    { 0x0180, 0x4, 0 },
    { 0x0130, 0x6, 0 },
    { 0x00C0, 0xB, 1 },
    { 0x00C0, 0xE, 0 },
    { 0x0098, 0xF, 0 }
};

// 3F80
int g_edramAddr = -1;

// 3FC0
SceAtracId g_atracIds[6] __attribute__((aligned(64)));

// 45C0
int g_needMemAT3;

// 45C4
int g_needMemAT3plus;

int sceAtracReinit(int numAT3Id, int numAT3plusId)
{
    int oldIntr = sceKernelCpuSuspendIntr();
    SceAtracId *curId = &g_atracIds[0];
    // 003C
    int i;
    for (i = 0; i < 6; i++)
    {
        if (curId->info.state > 0)
        {
            // 0188
            sceKernelCpuResumeIntr(oldIntr);
            return SCE_ERROR_BUSY;
        }
        curId++;
    }
    curId = &g_atracIds[0];
    // 0064
    for (i = 0; i < 6; i++) {
        curId->info.state = 0;
        curId++;
    }
    sceKernelCpuResumeIntr(oldIntr);
    if (numAT3Id != 0 || numAT3plusId != 0 || g_edramAddr == -1)
    {
        // 00BC
        if (g_edramAddr == -1)
        {
            // 0170
            int ret = allocEdram();
            if (ret < 0)
                return ret;
        }
        // 00CC
        int count = g_edramAddr;
        // 0110
        curId = &g_atracIds[0];
        int i;
        for (i = 0; i < numAT3Id + numAT3plusId; i++)
        {
            curId->codec.edramAddr = (void *)count;
            curId->codec.unk20 = 1;
            if (i >= numAT3plusId)
            {
                // 0164
                curId->info.codec = 0x1001;
                count += g_needMemAT3;
            }
            else
            {
                curId->codec.unk40.v8.u40 = 40;
                curId->codec.unk40.v8.u41 = 92;
                curId->info.codec = 0x1000;
                count += g_needMemAT3plus;
            }
            // 0130
            if ((u32)(g_edramAddr + 0x19000) < (u32)count)
                return SCE_ERROR_OUT_OF_MEMORY;
            curId->info.state = -1;
            curId++;
        }
        // 015C
        return 0;
    }
    g_edramAddr = -1;
    return sceAudiocodecReleaseEDRAM(&g_atracIds[0].codec);
}

int sceAtracGetAtracID(u32 codecType)
{
    int ret = 0x80630003;
    if (codecType < 0x1000 || codecType > 0x1001)
        return 0x80630004;
    int oldIntr = sceKernelCpuSuspendIntr();
    SceAtracId *curId = &g_atracIds[0];
    // 01EC
    int i;
    for (i = 0; i < 6; i++)
    {
        if (curId->info.codec == codecType && curId->info.state == -1) // 0228
        {
            ret = i;
            curId->info.state = 1;
            break;
        }
        // 01FC
        curId++;
    }
    // 0208
    sceKernelCpuResumeIntr(oldIntr);
    return ret;
}

int sceAtracSetHalfwayBuffer(int atracID, u8 *buffer, u32 readByte, u32 bufferByte)
{
    SceAtracFile info;
    if (atracID < 0 || atracID >= 6 || g_atracIds[atracID].info.state <= 0) {
        // 0320
        return 0x80630005;
    }
    if (bufferByte < readByte)
        return 0x80630013;
    int codec = loadWaveFile(readByte, &info, buffer);
    if (codec < 0)
        return codec;
    if (g_atracIds[atracID].info.codec != codec)
        return 0x80630007;
    // 02F4
    return setHalfwayBuffer(&g_atracIds[atracID], buffer, readByte, bufferByte, &info);
}

int sceAtracSetHalfwayBufferAndGetID(u8 *buffer, u32 readByte, u32 bufferByte)
{
    SceAtracFile info;
    if (bufferByte < readByte)
        return 0x80630013;
    int codec = loadWaveFile(readByte, &info, buffer);
    if (codec < 0)
        return codec;
    int id = sceAtracGetAtracID(codec);
    // 0388
    if (id < 0)
        return id;
    int ret2 = setHalfwayBuffer(&g_atracIds[id], buffer, readByte, bufferByte, &info);
    if (ret2 < 0)
    {
        // 03E0
        if (id >= 0 && id < 6)
        {
            // 03F4
            int oldIntr = sceKernelCpuSuspendIntr();
            if (g_atracIds[id].info.state > 0 && g_atracIds[id].info.state != 16)
                g_atracIds[id].info.state = -1;
            // 0420
            sceKernelCpuResumeIntr(oldIntr);
        }
        return ret2;
    }
    return id;
}

int sceAtracSetMOutHalfwayBuffer(int atracID, u8 *buffer, u32 readByte, u32 bufferByte)
{
    SceAtracFile info;
    if (atracID < 0 || atracID >= 6 || g_atracIds[atracID].info.state <= 0) {
        // 0510
        return 0x80630005;
    }
    if (bufferByte < readByte)
        return 0x80630013;
    int codec = loadWaveFile(readByte, &info, buffer);
    if (codec < 0)
        return codec;
    if (g_atracIds[atracID].info.codec != codec)
        return 0x80630007;
    // 04E4
    return setMOutHalfwayBuffer(&g_atracIds[atracID], buffer, readByte, bufferByte, &info);
}

int sceAtracSetMOutHalfwayBufferAndGetID(u8 *buffer, u32 readByte, u32 bufferByte)
{
    SceAtracFile info;
    if (bufferByte < readByte)
        return 0x80630013;
    int codec = loadWaveFile(readByte, &info, buffer);
    if (codec < 0)
        return codec;
    int id = sceAtracGetAtracID(codec);
    // 0578
    if (id < 0)
        return id;
    int ret = setMOutHalfwayBuffer(&g_atracIds[id], buffer, readByte, bufferByte, &info);
    if (ret < 0)
    {
        // 05D0
        if (id >= 0 && id < 6)
        {
            // 05E4
            int oldIntr = sceKernelCpuSuspendIntr();
            if (g_atracIds[id].info.state > 0 && g_atracIds[id].info.state != 16)
                g_atracIds[id].info.state = -1;
            // 0610
            sceKernelCpuResumeIntr(oldIntr);
        }
        return ret;
    }
    return id;
}

int sceAtracSetAA3HalfwayBufferAndGetID(u8 *buffer, u32 readByte, u32 bufferByte, int arg3, int arg4 __attribute__((unused)))
{
    SceAtracFile info;
    if (bufferByte < readByte)
        return 0x80630013;
    int id = openAA3AndGetID(buffer, readByte, arg3, &info);
    if (id < 0)
        return id;
    int ret = setHalfwayBuffer(&g_atracIds[id], buffer, readByte, bufferByte, &info);
    if (ret < 0)
    {
        // 06CC
        if (id >= 0 && id < 6)
        {
            // 06DC
            int oldIntr = sceKernelCpuSuspendIntr();
            if (g_atracIds[id].info.state > 0 && g_atracIds[id].info.state != 16)
                g_atracIds[id].info.state = -1;
            // 0708
            sceKernelCpuResumeIntr(oldIntr);
        }
        return ret;
    }
    return id;
}

int sceAtracDecodeData(int atracID, short *outAddr, u32 *samples, u32 *finishFlag, int *remainFrame)
{
    if (atracID < 0 || atracID >= 6 || g_atracIds[atracID].info.state <= 0) {
        // 0830
        return 0x80630005;
    }
    int ret = isValidState(g_atracIds[atracID].info.state);
    if (ret < 0)
        return ret;
    if (((int)outAddr & 1) != 0)
        return 0x80630014;
    if (g_atracIds[atracID].info.state == 6 && g_atracIds[atracID].info.secondBufferByte == 0) // 081C
        return 0x80630012;
    // 07A4
    // 07BC
    int i;
    for (i = 0; i <= g_atracIds[atracID].info.numFrame; i++)
    {
        ret = decodeFrame(&g_atracIds[atracID], outAddr, samples, finishFlag);
        if (ret != 0) {
            *samples = 0;
            return ret;
        }
    }
    // 07E0
    *remainFrame = getRemainFrame(&g_atracIds[atracID].info);
    return 0;
}

int sceAtracGetRemainFrame(int atracID, int *remainFrame)
{
    if (atracID < 0 || atracID >= 6 || g_atracIds[atracID].info.state <= 0)
        return 0x80630005;
    int ret = isValidState(g_atracIds[atracID].info.state);
    if (ret < 0)
        return ret;
    *remainFrame = getRemainFrame(&g_atracIds[atracID].info);
    return 0;
}

int sceAtracGetStreamDataInfo(int atracID, u8 **writePtr, u32 *writableByte, u32 *readPosition)
{
    if (atracID < 0 || atracID >= 6 || g_atracIds[atracID].info.state <= 0)
        return 0x80630005;
    int ret = isValidState(g_atracIds[atracID].info.state);
    if (ret < 0)
        return ret;
    SceAtracIdInfo *info = &g_atracIds[atracID].info;
    if (info->state == 2)
    {
        // 09A0
        *writableByte = 0;
        *writePtr = info->buffer;
        *readPosition = 0;
    }
    else if (info->state == 3)
    {
        // 0974
        u32 readPos = info->dataOff + info->streamDataByte;
        *writableByte = info->bufferByte - readPos;
        *writePtr = info->buffer + readPos;
        *readPosition = readPos;
    }
    else
    {
        *readPosition = getReadPosition(info);
        *writePtr = getWritePtr(info);
        *writableByte = getWritableByte(info);
    }
    return 0;
}

int sceAtracAddStreamData(int atracID, u32 addByte)
{
    if (atracID < 0 || atracID >= 6 || g_atracIds[atracID].info.state <= 0) {
        // 0A78
        return 0x80630005;
    }
    SceAtracIdInfo *info = &g_atracIds[atracID].info;
    int ret = isValidState(info->state);
    if (ret < 0)
        return ret;
    if (info->state == 2)
        return 0x80630009;
    u32 remainingByte;
    if (info->state == 3)
    {
        // 0A58
        remainingByte = info->dataEnd - info->dataOff - info->streamDataByte;
        if (remainingByte == addByte)
            info->state = 2;
    }
    else
        remainingByte = getWritableByte(info);
    // (0A24)
    // 0A28
    if (remainingByte < addByte)
        return 0x80630018;
    info->streamDataByte += addByte;
    return 0;
}

int sceAtracGetSecondBufferInfo(int atracID, u32 *position, u32 *dataByte)
{
    if (atracID < 0 || atracID >= 6 || g_atracIds[atracID].info.state <= 0)
        return 0x80630005;
    SceAtracIdInfo *info = &g_atracIds[atracID].info;
    int ret = isValidState(info->state);
    if (ret < 0)
        return ret;
    if (info->state != 6)
    {
        *position = 0;
        *dataByte = 0;
        return 0x80630022;
    }
    // 0B14
    u32 pos = getSecondBufPos(info, info->loopEnd) + 1;
    *position = pos;
    *dataByte = info->bufferByte - pos;
    return 0;
}

int sceAtracSetSecondBuffer(int atracID, u8 *secondBuffer, u32 secondBufferByte)
{
    if (atracID < 0 || atracID >= 6 || g_atracIds[atracID].info.state <= 0)
        return 0x80630005;
    SceAtracIdInfo *info = &g_atracIds[atracID].info;
    int ret = isValidState(info->state);
    if (ret < 0)
        return ret;
    u32 secondBufPos = getSecondBufPos(info, info->loopEnd);
    if (secondBufferByte < info->sampleSize * 3 && secondBufferByte < info->dataEnd - secondBufPos - 1)
        return 0x80630011;
    if (info->state != 6)
        return 0x80630022;
    info->secondBuffer = secondBuffer;
    info->secondBufferByte = secondBufferByte;
    info->unk52 = 0;
    return 0;
}

int sceAtracGetNextSample(int atracID, u32 *nextSample)
{
    if (atracID < 0 || atracID >= 6 || g_atracIds[atracID].info.state <= 0)
        return 0x80630005;
    SceAtracIdInfo *info = &g_atracIds[atracID].info;
    int ret = isValidState(info->state);
    if (ret < 0)
        return ret;
    *nextSample = getNextSample(info);
    return 0;
}

int sceAtracGetSoundSample(int atracID, int *endSample, int *loopStartSample, int *loopEndSample)
{
    if (atracID < 0 || atracID >= 6 || g_atracIds[atracID].info.state <= 0)
        return 0x80630005;
    SceAtracIdInfo *info = &g_atracIds[atracID].info;
    int ret = isValidState(info->state);
    if (ret < 0)
        return ret;
    int count = info->samplesPerChan;
    *endSample = info->endSample - count;
    if (info->loopEnd != 0)
    {
        // 0D28
        *loopStartSample = info->loopStart - count;
        *loopEndSample = info->loopEnd - count;
    }
    else {
        *loopStartSample = -1;
        *loopEndSample = -1;
    }
    // 0D04
    return 0;
}

int sceAtracGetNextDecodePosition(int atracID, u32 *samplePosition)
{
    if (atracID < 0 || atracID >= 6 || g_atracIds[atracID].info.state <= 0)
        return 0x80630005;
    SceAtracIdInfo *info = &g_atracIds[atracID].info;
    int ret = isValidState(info->state);
    if (ret < 0)
        return ret;
    if (info->bufferByte - info->curOff < info->sampleSize || info->endSample < info->decodePos)
        return 0x80630024;
    *samplePosition = info->decodePos - info->samplesPerChan;
    return 0;
}

int sceAtracGetLoopStatus(int atracID, int *loopNum, u32 *loopStatus)
{
    if (atracID < 0 || atracID >= 6 || g_atracIds[atracID].info.state <= 0)
        return 0x80630005;
    SceAtracIdInfo *info = &g_atracIds[atracID].info;
    int ret = isValidState(info->state);
    if (ret < 0)
        return ret;
    *loopNum = info->loopNum;
    if (info->loopEnd == 0)
        *loopStatus = 0;
    else if (info->loopNum != 0)
        *loopStatus = 1;
    else if (info->loopEnd < info->decodePos)
        *loopStatus = 0;
    else
        *loopStatus = 1;
    return 0;
}

int sceAtracSetLoopNum(int atracID, int loopNum)
{
    if (atracID < 0 || atracID >= 6 || g_atracIds[atracID].info.state <= 0)
        return 0x80630005;
    SceAtracIdInfo *info = &g_atracIds[atracID].info;
    int ret = isValidState(info->state);
    if (ret < 0)
        return ret;
    if (info->loopEnd == 0 || (info->loopNum == 0 && info->loopEnd < info->decodePos))
        return 0x80630021;
    info->loopNum = loopNum;
    return 0;
}

int __attribute__((alias("sceAtracGetBufferInfoForResetting"))) sceAtracGetBufferInfoForReseting(int atracID, u32 sample, SceBufferInfo *bufferInfo);
int sceAtracGetBufferInfoForResetting(int atracID, u32 sample, SceBufferInfo *bufferInfo)
{
    if (atracID < 0 || atracID >= 6 || g_atracIds[atracID].info.state <= 0)
        return 0x80630005;
    SceAtracIdInfo *info = &g_atracIds[atracID].info;
    int ret = isValidState(info->state);
    if (ret < 0)
        return ret;
    if (info->state == 6 && info->secondBufferByte == 0) // 1008
        return 0x80630012;
    // 0FB4
    if (info->endSample < sample + info->samplesPerChan)
        return 0x80630015;
    getBufferInfo(info, sample + info->samplesPerChan, bufferInfo);
    return resetId(&g_atracIds[atracID]);
}

int sceAtracResetPlayPosition(int atracID, u32 sample, u32 writeByteFirstBuf, u32 writeByteSecondBuf)
{
    if (atracID < 0 || atracID >= 6 || g_atracIds[atracID].info.state <= 0)
        return 0x80630005;
    SceAtracIdInfo *info = &g_atracIds[atracID].info;
    int ret = isValidState(info->state);
    if (ret < 0)
        return ret;
    if (info->state == 6 && info->secondBufferByte == 0) // 10F0
        return 0x80630012;
    // 1090
    if (info->endSample < sample + info->samplesPerChan)
        return 0x80630015;
    ret = resetPlayPos(info, sample + info->samplesPerChan, writeByteFirstBuf, writeByteSecondBuf);
    if (ret < 0)
        return ret;
    return resetId(&g_atracIds[atracID]);
}

int sceAtracLowLevelInitDecoder(int atracID, void *arg1)
{
    if (atracID < 0 || atracID >= 6 || g_atracIds[atracID].info.state <= 0)
        return 0x80630005;
    SceAtracIdInfo *info = &g_atracIds[atracID].info;
    int ret = initDecoder(&g_atracIds[atracID], arg1);
    if (ret != 0)
        return ret;
    if (*(int*)(arg1 + 0) == 1 && *(int*)(arg1 + 0) == *(int*)(arg1 + 4)) // 1194
        return sceAudiocodec_3DD7EE1A(&g_atracIds[atracID].codec, info->codec);
    // 1174
    return sceAudiocodecInit(&g_atracIds[atracID].codec, info->codec);
}

int sceAtracLowLevelDecode(int atracID, void *inBuf, int *arg2, void *outBuf, int *arg4)
{
    if (atracID < 0 || atracID >= 6 || g_atracIds[atracID].info.state <= 0)
        return 0x80630005;
    SceAtracIdInfo *info = &g_atracIds[atracID].info;
    if (info->state != 8)
        return 0x80630032;
    if (((int)outBuf & 1) != 0)
        return 0x80630014;
    g_atracIds[atracID].codec.inBuf = inBuf;
    g_atracIds[atracID].codec.outBuf = outBuf;
    if (info->codec == 0x1000) {
        // 1278
        g_atracIds[atracID].codec.unk48 = 0;
    }
    // 1234
    if (sceAudiocodecDecode(&g_atracIds[atracID].codec, info->codec) != 0)
        return 0x80630002;
    *arg2 = g_atracIds[atracID].codec.readSample;
    *arg4 = g_atracIds[atracID].codec.decodedSample;
    return 0;
}

// 1284
int setHalfwayBuffer(SceAtracId *id, u8 *buffer, u32 readByte, u32 bufferByte, SceAtracFile *info)
{
    int ret = setBuffer(id, buffer, readByte, bufferByte, info);
    if (ret < 0)
        return ret;
    ret = sceAudiocodecInit(&id->codec, id->info.codec);
    if (ret == 0)
        ret = resetId(id);
    // 12D8
    if ((id->info.state & 4) != 0) {
        // 130C
        copyBuffer(&id->info, bufferByte);
    }
    return ret;
}

// 131C
int setMOutHalfwayBuffer(SceAtracId *id, u8 *buffer, u32 readByte, u32 bufferByte, SceAtracFile *info)
{
    int ret = setBuffer(id, buffer, readByte, bufferByte, info);
    if (ret < 0)
        return ret;
    ret = 0x80630019;
    if (info->channels == 1)
    {
        // 13B4
        ret = sceAudiocodec_3DD7EE1A(&id->codec, id->info.codec);
        if (ret == 0)
            ret = resetId(id);
    }
    // 136C
    if ((id->info.state & 4) != 0) {
        // 13A4
        copyBuffer(&id->info, bufferByte);
    }
    return ret;
}

// 13D8
int resetId(SceAtracId *info)
{
    u32 finishFlag;
    // 1404
    while (info->info.numFrame != 0)
    {
        int ret = decodeFrame(info, NULL, NULL, &finishFlag);
        if (ret != 0)
            return ret;
    }
    return 0;
}

int sceAtracStartEntry(SceSize argSize __attribute__((unused)), const void *argBlock __attribute__((unused)))
{
    return (sceAtracReinit(2, 2) < 0);
}

int sceAtracEndEntry(SceSize argSize __attribute__((unused)), const void *argBlock __attribute__((unused)))
{
    if (g_edramAddr != -1)
        sceAudiocodecReleaseEDRAM(&g_atracIds[0].codec);
    return 0;
}

int sceAtracReleaseAtracID(int atracID)
{
    int ret = 0x80630005;
    if (atracID < 0 || atracID >= 6)
        return ret;
    // 14CC
    int oldIntr = sceKernelCpuSuspendIntr();
    if (g_atracIds[atracID].info.state > 0 && g_atracIds[atracID].info.state != 16) {
        g_atracIds[atracID].info.state = -1;
        ret = 0;
    }
    // 1510
    sceKernelCpuResumeIntr(oldIntr);
    return ret;
}

int sceAtracSetData(int atracID, u8 *buffer, u32 bufferByte)
{
    return sceAtracSetHalfwayBuffer(atracID, buffer, bufferByte, bufferByte);
}

int sceAtracSetDataAndGetID(u8 *buffer, u32 bufferByte)
{
    return sceAtracSetHalfwayBufferAndGetID(buffer, bufferByte, bufferByte);
}

int sceAtracSetAA3DataAndGetID(u8 *buffer, u32 bufferByte, int arg2, int arg3)
{
    return sceAtracSetAA3HalfwayBufferAndGetID(buffer, bufferByte, bufferByte, arg2, arg3); // unused last argument?
}

int sceAtracIsSecondBufferNeeded(int atracID)
{
    if (atracID < 0 || atracID >= 6 || g_atracIds[atracID].info.state <= 0)
        return 0x80630005;
    int ret = isValidState(g_atracIds[atracID].info.state);
    if (ret < 0)
        return ret;
    return (g_atracIds[atracID].info.state == 6);
}

int sceAtracGetChannel(int atracID, u32 *channel)
{
    if (atracID < 0 || atracID >= 6 || g_atracIds[atracID].info.state <= 0)
        return 0x80630005;
    if (g_atracIds[atracID].info.state == 1)
        return 0x80630010;
    *channel = g_atracIds[atracID].info.numChan;
    return 0;
}

int sceAtracGetOutputChannel(int atracID, u32 *outputChan)
{
    if (atracID < 0 || atracID >= 6 || g_atracIds[atracID].info.state <= 0)
        return 0x80630005;
    if (g_atracIds[atracID].info.state == 1)
        return 0x80630010;
    *outputChan = getOutputChan(&g_atracIds[atracID].info, &g_atracIds[atracID].codec);
    return 0;
}

int sceAtracGetMaxSample(int atracID, u32 *maxSample)
{
    if (atracID < 0 || atracID >= 6 || g_atracIds[atracID].info.state <= 0)
        return 0x80630005;
    if (g_atracIds[atracID].info.state == 1)
        return 0x80630010;
    *maxSample = 0x800 >> (g_atracIds[atracID].info.codec & 1);
    return 0;
}

int sceAtracGetBitrate(int atracID, u32 *outBitrate)
{   
    if (atracID < 0 || atracID >= 6 || g_atracIds[atracID].info.state <= 0)
        return 0x80630005;
    if (g_atracIds[atracID].info.state == 1)
        return 0x80630010;
    SceAtracIdInfo *info = &g_atracIds[atracID].info;
    u32 bitrate = (info->sampleSize * 352800) / 1000;
    if (info->codec == 0x1000)
    {
        // 17A0
        bitrate = ((bitrate >> 11) + 8) & 0xFFFFFFF0;
    }
    else
        bitrate = (bitrate + 511) >> 10;
    // 1790
    *outBitrate = bitrate;
    return 0;
}

int sceAtracGetInternalErrorInfo(int atracID, int *result)
{
    if (atracID < 0 || atracID >= 6 || g_atracIds[atracID].info.state <= 0)
        return 0x80630005;
    if (g_atracIds[atracID].info.state == 1)
        return 0x80630010;
    *result = g_atracIds[atracID].codec.err;
    return 0;
}

SceAtracId *_sceAtracGetContextAddress(int atracID)
{
    if (atracID < 0 || atracID >= 6)
        return 0;
    if (g_atracIds[atracID].info.state <= 0)
        return 0;
    return &g_atracIds[atracID];
}

// 182C
int allocEdram(void)
{
    g_atracIds[0].codec.unk40.v8.u40 = 40;
    g_atracIds[0].codec.unk40.v8.u41 = 92;
    int ret = sceAudiocodecCheckNeedMem(&g_atracIds[0].codec, 0x1000);
    if (ret < 0)
        return ret;
    g_needMemAT3plus = (g_atracIds[0].codec.neededMem + 0x3F) & 0xFFFFFFC0;
    ret = sceAudiocodecCheckNeedMem(&g_atracIds[0].codec, 0x1001);
    if (ret < 0)
        return ret;
    g_needMemAT3 = (g_atracIds[0].codec.neededMem + 0x3F) & 0xFFFFFFC0;
    g_atracIds[0].codec.neededMem = 0x19000;
    ret = sceAudiocodecGetEDRAM(&g_atracIds[0].codec, 0x1001);
    if (ret < 0)
        return ret;
    g_edramAddr = (int)g_atracIds[0].codec.edramAddr;
    return 0;
}

// 18D4
int openAA3AndGetID(u8 *buffer, u32 readByte, int arg2, SceAtracFile *info)
{
    SceAA3File file;
    int codec = parseAA3(readByte, &file, arg2, buffer);
    if (codec < 0)
        return codec;
    int ret = setAtracFileInfo(&file, info);
    if (ret < 0)
        return ret;
    return sceAtracGetAtracID(codec);
}

// 1938
int isValidState(char state)
{
    if (state == 8)
        return 0x80630031;
    if (state == 1)
        return 0x80630010;
    if (state == 16)
        return 0x80630040;
    return 0;
}

// 1970
int getOutputChan(SceAtracIdInfo *info, SceAudiocodecCodec *codec)
{
    if (info->codec == 0x1000) {
        // 199C
        return codec->unk72;
    }
    if (codec->unk40.v32 - 0xeU < 2)
        return codec->unk52;
    return 2;
}

// 19A4
int decodeFrame(SceAtracId *id, short *outAddr, u32 *samples, u32 *finishFlag)
{
    char buf[0x2000] __attribute__((aligned(64)));
    int unk = 0x800 >> (id->info.codec & 1);
    int nextSample = getNextSample(&id->info);
    u32 unk2 = id->info.curOff;
    if (id->info.dataEnd >= id->info.sampleSize + unk2 && id->info.endSample >= id->info.decodePos)
    {
        // 1A74
        // 1A94
        if (((id->info.state & 4) != 0 && id->info.streamDataByte < id->info.sampleSize)
         || (id->info.state == 3 && id->info.dataOff + id->info.streamDataByte < id->info.sampleSize + id->info.curOff)) // 1C34
        {
            // 1C4C
            *finishFlag = 0;
            return 0x80630023;
        }
        // 1AA4
        if ((id->info.state & 4) == 0) {
            // 1AB4
            id->codec.inBuf = id->info.buffer + unk2;
        } else {
            // 1C1C
            char unkFlag = id->info.unk22 & 1;
            unk2 = (unkFlag) ? id->info.unk52 : id->info.unk48;
            u8* infoBuf = (unkFlag) ? id->info.secondBuffer : id->info.buffer;
            // 1AB4
            id->codec.inBuf = infoBuf + unk2;
        }
        // 1AB8
        id->codec.outBuf = outAddr;
        if (nextSample != unk)
            id->codec.outBuf = buf;
        // 1AC4
        int ret = sceAudiocodecDecode(&id->codec, id->info.codec);
        if (ret != 0)
        {
            // 1C0C
            *finishFlag = 0;
            return 0x80630002;
        }
        id->info.curOff += id->info.sampleSize;
        if (id->info.numFrame == 0)
        {
            // 1B20
            *samples = nextSample;
            if (id->info.endSample < id->info.decodePos + nextSample)
                *finishFlag = (id->info.loopNum == 0);
            else
                *finishFlag = 0;
            // 1B44
            if (nextSample != unk && nextSample != 0 && outAddr != NULL) {
                int numChan = getOutputChan(&id->info, &id->codec);
                sceKernelMemcpy(outAddr, buf + ((id->info.decodePos & (0x7FF >> (id->info.codec & 1))) << numChan), nextSample << numChan);
            }
            // 1B98
            id->info.decodePos += nextSample;
            // 1BC8
            if (id->info.loopEnd != 0 && (id->info.loopNum != 0 || id->info.loopEnd >= id->info.decodePos) && id->info.loopEnd < id->info.decodePos) // go back to start of loop
            {
                id->info.curOff = getOffFromSample(&id->info, id->info.loopStart);
                id->info.numFrame = getFrameFromSample(&id->info, id->info.loopStart);
                id->info.decodePos = id->info.loopStart;
                if (id->info.loopNum > 0)
                    id->info.loopNum--;
            }
        }
        else
            id->info.numFrame--;
        // (1AF8)
        // 1AFC
        if ((id->info.state & 4) != 0) {
            // 1B10
            sub_1C54(&id->info);
        }
        return ret;
    }
    // 1A34
    *finishFlag = 1;
    return 0x80630024;
}

void sub_1C54(SceAtracIdInfo *info)
{
    info->streamDataByte -= info->sampleSize;
    if (info->unk22 == 1)
    {
        // 1D48
        u32 unk1 = info->unk52 + info->sampleSize;
        if (info->secondBufferByte >= unk1 + info->sampleSize)
            info->unk52 = unk1;
        else
        {
            info->unk48 = 0;
            info->unk22 = 2;
            info->unk52 = 0;
        }
        return;
    }
    u32 unk2 = info->unk48 + info->sampleSize;
    if (info->bufferByte < unk2 + info->sampleSize)
        unk2 = 0;
    info->unk48 = unk2;
    if (info->loopEnd != 0)
    {
        if (info->loopNum != 0)
            return;
        if (info->loopEnd >= info->decodePos)
            return;
    }
    // 1CC8
    if (info->unk22 != 0 || info->state != 6)
        return;
    // 1CFC
    if (getSecondBufPos(info, info->loopEnd) >= info->curOff)
        return;
    info->unk22 = 1;
    info->streamDataByte = info->secondBufferByte;
    info->unk52 = 0;
    sceKernelMemcpy(info->buffer, info->secondBuffer + info->secondBufferByte - (info->secondBufferByte % info->sampleSize), info->secondBufferByte % info->sampleSize);
}

#define WAVE_CHUNK_ID_DATA 0x61746164 /* "data" */
#define WAVE_CHUNK_ID_SMPL 0x6C706D73 /* "smpl" */
#define WAVE_CHUNK_ID_FACT 0x74636166 /* "fact" */
#define WAVE_CHUNK_ID_FMT  0x20746D66 /* "fmt " */
#define RIFF_MAGIC 0x46464952 /* "RIFF" */
#define WAVE_MAGIC 0x45564157 /* "WAVE" */

// 1D7C
int loadWaveFile(u32 size, SceAtracFile *info, u8 *in)
{
    u32 curOff = 0;
    info->loopEnd = -1;
    info->samplesPerChan = 0;
    info->loopStart = -1;
    info->factSz = 0;
    info->dataSize = 0;
    int fmt = 0x80630006;
    int multiChan = 0;
    // 1DEC
    for (;;)
    {
        if (size < curOff + 12)
            return 0x80630011;
        if (readWaveData(in, &curOff, 4) != RIFF_MAGIC)
            return 0x80630006;
        int cksize = readWaveData(in, &curOff, 4) - 4;
        int inc = cksize + (cksize & 1);
        if (readWaveData(in, &curOff, 4) == WAVE_MAGIC)
            break;
        if (size < curOff + inc)
            return 0x80630011;
        curOff += inc;
    }
    // 1EAC
    if (curOff + 8 >= size) {
        // 1F58 dup
        return 0x80630011;
    }
    // 1ECC
    do
    {
        int ckid = readWaveData(in, &curOff, 4);
        u32 cksize = readWaveData(in, &curOff, 4);
        cksize = cksize + (cksize & 1);
        if (size < curOff + cksize && ckid != WAVE_CHUNK_ID_DATA)
            return 0x80630011;
        switch (ckid)
        {
            case WAVE_CHUNK_ID_DATA:
                // 22A8
                info->dataSize = cksize;
                info->dataOff = curOff;
                if (info->samplesPerChan == 0)
                {
                    if (fmt != 0x1001)
                        info->samplesPerChan = 2048;
                    else
                        info->samplesPerChan = 1024;
                }
                // 22CC
                if (multiChan && fmt == 0x1000)
                {
                    info->samplesPerChan -= 184;
                    if (info->loopEnd != -1) {
                        info->loopEnd -= 184;
                        info->loopStart -= 184;
                    }
                }
                return fmt;

                // 2160
            case WAVE_CHUNK_ID_SMPL:
                // 2208
                if (info->loopStart < 0)
                {
                    if (cksize < 32)
                        return 0x80630006;
                    curOff += 28;
                    cksize -= 32;
                    if (readWaveData(in, &curOff, 4) != 0)
                    {
                        if (cksize < 20)
                            return 0x80630006;
                        curOff += 12;
                        info->loopStart = readWaveData(in, &curOff, 4);
                        info->loopEnd = readWaveData(in, &curOff, 4);
                        cksize -= 20;
                        if (info->loopStart >= info->loopEnd)
                            return 0x80630008;
                    }
                }
                break;

            case WAVE_CHUNK_ID_FACT:
                if (cksize < 4)
                    return 0x80630006;
                cksize -= 4;
                info->factSz = readWaveData(in, &curOff, 4);
                if (cksize >= 8)
                {
                    // 21D8
                    readWaveData(in, &curOff, 4);
                    multiChan = 1;
                    cksize -= 8;
                    // 21D0 dup
                    info->samplesPerChan = readWaveData(in, &curOff, 4);
                }
                else if (cksize >= 4)
                {
                    // 21BC
                    cksize -= 4;
                    // 21D0 dup
                    info->samplesPerChan = readWaveData(in, &curOff, 4);
                }
                break;

            case WAVE_CHUNK_ID_FMT:
                // 1F64
                if (fmt != (s32)0x80630006)
                    return 0x80630006;
                if (cksize < 32)
                    return 0x80630006;
                short fmtCode = readWaveData(in, &curOff, 2);
                short channels = readWaveData(in, &curOff, 2);
                info->channels = channels;
                if (((channels - 1) & 0xFFFF) >= 2)
                    return 0x80630006;
                if (readWaveData(in, &curOff, 4) != 44100)
                    return 0x80630006;
                curOff += 4; // ignore "data rate"
                short dataBlkSize = readWaveData(in, &curOff, 2);
                info->dataBlkSz = dataBlkSize;
                if ((dataBlkSize & 0xFFFF) == 0)
                    return 0x80630006;
                if (fmtCode == 0x270)
                {
                    // 20C4
                    curOff += 4; // skip bits per sample and cbSize
                    short validBitsPerSample = readWaveData(in, &curOff, 2);
                    if (validBitsPerSample != 1)
                        return 0x80630006;
                    curOff += 4; // skip channel mask
                    short unk = readWaveData(in, &curOff, 2); // err.. it should be GUID
                    info->unk4.v16 = unk;
                    if ((unk & 0xFFFF) != readWaveData(in, &curOff, 2))
                        return 0x80630006;
                    if (readWaveData(in, &curOff, 4) != validBitsPerSample)
                        return 0x80630006;
                    fmt = 0x1001;
                    cksize -= 32;
                }
                else
                {
                    if (fmtCode != (s16)0xFFFE) {
                        // 20B8
                        return 0x80630006;
                    }
                    if (cksize < 52)
                        return 0x80630006;
                    u8 *curIn = in + curOff + 10;
                    u8 *curGUID = g_at3PlusGUID;
                    // 2048
                    int i;
                    // check AT3+ GUID
                    for (i = 16; i != 0; i--)
                    {
                        if (*(curGUID++) != *(curIn++))
                        {
                            // 20A4
                            if (i == 0)
                                break;
                            return 0x80630006;
                        }
                    }
                    // 2064
                    info->unk4.v8[0] = in[curOff + 28];
                    info->unk4.v8[1] = in[curOff + 29];
                    if (((info->unk4.v8[0] >> 2) & 7) != info->channels)
                        return 0x80630006;
                    fmt = 0x1000;
                    cksize -= 52;
                    curOff += 38;
                }
                break;

            default:
                break;
        }
        // 1F38
        // 1F3C
        curOff += cksize;
    } while (size >= curOff && curOff + 8 < size);
    // 1F58 dup
    return 0x80630011;
}

// 2310
int readWaveData(u8 *in, u32 *curOff, int size)
{
    u8 *curStr = in + *curOff + size;
    *curOff = *curOff + size;
    int result = 0;
    // 2324
    do
        result |= *(--curStr) << ((--size) * 8);
    while (size != 0);
    return result;
}

// 2348
int getOffFromSample(SceAtracIdInfo *info, u32 sample)
{
    u32 max = 368;
    if (info->codec == 0x1001)
        max = 69;
    u32 unk = ((sample >> (0x100B - info->codec)) - 1) * info->sampleSize;
    if ((sample & (0x7FF >> (info->codec & 1))) < max && unk != 0)
        unk -= info->sampleSize;
    // 239C
    return unk + info->dataOff;
}

// 23A4
u32 getSecondBufPos(SceAtracIdInfo *info, u32 arg1)
{
    return ((arg1 >> (0x100B - info->codec)) + 1) * info->sampleSize + info->dataOff - 1;
}

// 23D4
int getFrameFromSample(SceAtracIdInfo *info, u32 sample)
{
    u32 max = 368;
    if (info->codec == 0x1001)
        max = 69;
    if ((sample & (0x7FF >> (info->codec & 1))) < max)
        return 2;
    return 1;
}

// 240C
int getNextSample(SceAtracIdInfo *info)
{
    int mask = 0x7FF >> (info->codec & 1);
    u32 unk1 = (info->decodePos & ~mask) + mask;
    u32 unk2 = unk1 - info->endSample;
    if (info->endSample >= unk1)
        unk2 = 0;
    u32 unk3 = (info->decodePos & mask) + unk2;
    u32 unk4 = 0x800 >> (info->codec & 1);
    if (unk3 >= unk4)
        return 0;
    return unk4 - unk3;
}

// 245C
int setBuffer(SceAtracId *id, u8 *buffer, u32 readByte, u32 bufferByte, SceAtracFile *info)
{
    id->info.numChan = info->channels;
    id->info.samplesPerChan = info->samplesPerChan + (id->info.codec == 0x1001 ? 69 : 368);
    id->info.sampleSize = info->dataBlkSz;
    setBufferInfo(&id->info, info->factSz, info->dataSize, info->samplesPerChan, info->loopStart, info->loopEnd);
    id->info.streamDataByte = readByte - info->dataOff;
    id->info.buffer = buffer;
    id->info.curOff = info->dataOff;
    id->info.dataOff = info->dataOff;
    id->info.dataEnd = info->dataSize + info->dataOff; // end
    id->info.unk22 = 0;
    id->info.bufferByte = bufferByte;
    id->info.unk48 = info->dataOff;
    if (id->info.endSample >= id->info.loopEnd && (id->info.endSample >> (0x100B - id->info.codec)) * id->info.sampleSize < info->dataSize)
    {
        // 2584
        int ret = setBufferSize(&id->info, readByte, bufferByte);
        if (ret != 0)
            return ret;
        if (id->info.codec == 0x1001) // AT3
        {
            // 25BC
            SceAtracUnk *cur = &g_3F08[4];
            // 25D4
            int i;
            for (i = 0; i < 5; i++)
            {
                if (cur->unk0 == id->info.sampleSize)
                {
                    // 25F0
                    if (cur->unk3 == info->unk4.v16) {
                        id->codec.unk40.v32 = cur->unk2;
                        return 0;
                    }
                }
                // 25E0
                cur--;
            }
            return 0x80630008;
        }
        id->codec.unk40.v8.u40 = info->unk4.v8[0];
        id->codec.unk48 = 0;
        id->codec.unk40.v8.u41 = info->unk4.v8[1];
        return ret;
    }
    return 0x80630008;
}

// 2608
int initDecoder(SceAtracId *info, void *arg1)
{
    int ret;
    if (info->info.codec == 0x1001)
    {
        // 26AC
        ret = initAT3Decoder(&info->codec, arg1); // AT3
    }
    else
        ret = initAT3plusDecoder(&info->codec, arg1); // AT3+
    // 2634
    if (ret < 0)
        return ret;
    info->info.state = 8;
    info->info.buffer = NULL;
    info->info.numChan = *(u8*)(arg1 + 0);
    info->info.secondBuffer = NULL;
    info->info.secondBufferByte = 0;
    info->info.sampleSize = *(u16*)(arg1 + 8);
    info->info.decodePos = 0;
    info->info.endSample = 0;
    info->info.loopStart = 0;
    info->info.loopEnd = 0;
    info->info.samplesPerChan = 0;
    info->info.numFrame = 0;
    info->info.unk22 = 0;
    info->info.dataOff = 0;
    info->info.curOff = 0;
    info->info.dataEnd = 0;
    info->info.loopNum = 0;
    info->info.streamDataByte = 0;
    info->info.unk48 = 0;
    info->info.unk52 = 0;
    info->info.bufferByte = 0;
    return ret;
}

// 26BC
int initAT3Decoder(SceAudiocodecCodec *codec, void *arg1)
{
    int sp[8];
    int *cur = (int*)&g_3E74[0];
    int *curEnd = (int*)&g_3E74[4];
    int *curSp = sp;
    int *end = curEnd;
    if (((int)g_3E74 & 3) == 0)
    {
        // 27C0
        for (;;)
        {
            curSp[0] = cur[0];
            curSp[1] = cur[1];
            curSp[2] = cur[2];
            curSp[3] = cur[3];
            curSp += 4;
            cur = curEnd;
            if (curEnd == end)
            {
                // 27F4
                *curSp = *curEnd;
                break;
            }
            curEnd += 4;
        }
    }
    else
    {
        // 26F4
        do
        {
            asm("lwl $a0, 3(%0)\n \
                 lwr $a0, 0(%0)\n \
                 lwr $a1, 7(%0)\n \
                 lwr $a1, 4(%0)\n \
                 lwr $a2, 11(%0)\n \
                 lwr $a2, 8(%0)\n \
                 lwr $a3, 15(%0)\n \
                 lwr $a3, 12(%0)\n \
                 swl $a0, 3(%1)\n \
                 swr $a0, 0(%1)\n \
                 swr $a1, 7(%1)\n \
                 swr $a1, 4(%1)\n \
                 swr $a2, 11(%1)\n \
                 swr $a2, 8(%1)\n \
                 swr $a3, 15(%1)\n \
                 swr $a3, 12(%1)" : : "r" (cur), "r" (curSp) : "a0", "a1", "a2", "a3");
            cur += 4;
            curSp += 4;
        } while (cur != end);
        asm("lwl $v1, 3(%0)\n \
             lwr $v1, 0(%0)\n \
             swl $v1, 3(%1)\n \
             swr $v1, 0(%1)" : : "r" (cur), "r" (curSp) : "v1");
    }
    // 2754
    int ret = isValidOpt(*(int*)(arg1 + 0), *(int*)(arg1 + 4));
    if (ret < 0)
        return ret;
    // 2778
    int i;
    SceAtracUnk *unk = &g_3E74[4];
    for (i = 0; i < 5; i++)
    {
        if (unk->unk0 == *(int*)(arg1 + 8) && unk->unk2 == *(int*)(arg1 + 0)) { // 27A0
            codec->unk40.v32 = 0;
            codec->unk40.v8.u40 = unk->unk3;
            return 0;
        }
        // 2784
        unk--;
    }
    return 0x80630001;
}

// 2804
int initAT3plusDecoder(SceAudiocodecCodec *codec, void *arg1)
{
    int sp[28];
    int *cur = (int*)&g_3E88[0];
    int *curSp = sp;
    int *end = (int*)&g_3E88[28];
    if (((int)g_3E88 & 3) == 0)
    {
        // 2918
        do
        {
            curSp[0] = cur[0];
            curSp[1] = cur[1];
            curSp[2] = cur[2];
            curSp[3] = cur[3];
            cur += 4;
            curSp += 4;
        } while (cur != end);
    }
    else
    {
        do
        {
            asm("lwl $a0, 3(%0)\n \
                 lwr $a0, 0(%0)\n \
                 lwr $a1, 7(%0)\n \
                 lwr $a1, 4(%0)\n \
                 lwr $a2, 11(%0)\n \
                 lwr $a2, 8(%0)\n \
                 lwr $a3, 15(%0)\n \
                 lwr $a3, 12(%0)\n \
                 swl $a0, 3(%1)\n \
                 swr $a0, 0(%1)\n \
                 swr $a1, 7(%1)\n \
                 swr $a1, 4(%1)\n \
                 swr $a2, 11(%1)\n \
                 swr $a2, 8(%1)\n \
                 swr $a3, 15(%1)\n \
                 swr $a3, 12(%1)" : : "r" (cur), "r" (curSp) : "a0", "a1", "a2", "a3");
            cur += 4;
            curSp += 4;
        } while (cur != end);
    }
    // 2888
    int ret = isValidOpt(*(int*)(arg1 + 0), *(int*)(arg1 + 4));
    if (ret < 0)
        return ret;
    curSp = &sp[26];
    // 28A8
    int i;
    for (i = 0; i < 14; i++)
    {
        if (*(u16*)(curSp + 0) == *(int*)(arg1 + 8))
        {
            // 28D0
            if (*(u16*)((int)curSp + 2) == *(int*)(arg1 + 0))
            {
                codec->unk40.v8.u41 = *(u8*)((int)curSp + 5);
                codec->unk40.v8.u40 = *(u8*)((int)curSp + 4);
                codec->unk56 = 0;
                codec->unk40.v8.u42 = 0;
                codec->unk40.v8.u43 = 0;
                codec->unk44.v8.u44 = 0;
                codec->unk44.v8.u45 = 0;
                codec->unk44.v8.u46 = 0;
                codec->unk44.v8.u47 = 0;
                codec->unk48 = 0;
                return 0;
            }
        }
        // 28B4
        curSp -= 2;
    }
    return 0x80630001;
}

// 294C
int isValidOpt(int arg0, int arg1)
{
    if ((arg0 != 2 || arg1 != 2) && (arg0 != 1 || arg1 < 1 || arg1 >= 3))
        return 0x80630001;
    return 0;
}

// 298C
void setBufferInfo(SceAtracIdInfo *info, u32 factSize, u32 dataSize, u32 samplesPerChan, int loopStart, int loopEnd)
{
    int add = 368;
    if (info->codec == 0x1001)
        add = 69;
    u32 shift = 0x100B - info->codec;
    u32 samples = samplesPerChan + add;
    if (factSize != 0) {
        // 29FC
        factSize += samples;
    }
    else
        factSize = (dataSize / info->sampleSize) << shift;
    // 29C0
    info->decodePos = samples;
    info->loopNum = 0;
    info->endSample = factSize - 1;
    info->numFrame = samples >> shift;
    if (loopStart < 0)
    {
        // 29F0
        info->loopEnd = 0;
        info->loopStart = 0;
    }
    else {
        info->loopEnd = loopEnd + add;
        info->loopStart = loopStart + add;
    }
}

// 2A04
int setBufferSize(SceAtracIdInfo *info, u32 readByte, u32 bufferByte)
{
    if (bufferByte < info->dataEnd)
    {
        // 2A48
        u32 unk = info->sampleSize * 3;
        if (info->streamDataByte < unk)
            return 0x80630011;
        if (info->loopEnd == 0) {
            info->state = 4;
            return 0;
        }
        if (info->loopEnd == info->endSample) {
            info->state = 5;
            return 0;
        }
        u32 unk2 = getSecondBufPos(info, info->loopEnd) - info->dataOff + 1;
        info->state = 6;
        if (unk2 < info->streamDataByte)
            info->streamDataByte = unk2;
        // 2AAC
        info->unk52 = 0;
        info->secondBuffer = NULL;
        info->secondBufferByte = 0;
    }
    else if (readByte < info->dataEnd)
        info->state = 3;
    else
        info->state = 2;
    return 0;
}

// 2ABC
int getRemainFrame(SceAtracIdInfo *info)
{
    switch (info->state) // jump table at 0x3F1C
    {
    case 0: case 1:
        // 2B00
        return 0;

    case 2:
        // 2AF0
        return -1;

    case 3: {
        // 2B08
        if (info->curOff >= info->dataOff + info->streamDataByte)
            return 0;
        int ret = (info->dataOff + info->streamDataByte - info->curOff) / info->sampleSize - info->numFrame;
        if (ret <= 0)
            ret = 0;
        return ret;
    }

    case 4:
        // 2B4C
        return sub_2DB8(info);

    case 5:
        return sub_2DF8(info);

    case 6:
        // 2B6C
        if (info->loopEnd < info->decodePos) {
            // 2B4C dup
            return sub_2DB8(info);
        }
        // 2B5C dup
        return sub_2DF8(info);

    default:
        return 0;
    }
}

// 2B88
u8 *getWritePtr(SceAtracIdInfo *info)
{
    int count = 0;
    switch (info->state)
    {
    case 4:
        // 2BD8
        if (info->curOff + info->streamDataByte < info->dataEnd)
            count = sub_2FA8(info->unk48, info->bufferByte, info->streamDataByte, info->sampleSize);
        break;

    case 5:
        // 2C38/2C3C dup
        count = sub_2FA8(info->unk48, info->bufferByte, info->streamDataByte, info->sampleSize);
        break;

    case 6:
        // 2C0C
        if (info->loopEnd < info->decodePos && info->unk22 == 1) {
            // 2C4C
            count = sub_2FA8(0, info->bufferByte, sub_31B4(info), info->sampleSize);
        }
        else {
            // 2C38/2C3C dup
            count = sub_2FA8(info->unk48, info->bufferByte, info->streamDataByte, info->sampleSize);
        }
    
    default:
        break;
    }
    return info->buffer + count;
}

// 2C68
u32 getReadPosition(SceAtracIdInfo *info)
{
    switch (info->state)
    {
    case 4:
        // 2CB8
        return sub_2FEC(info->dataEnd, info->curOff, info->streamDataByte, info->bufferByte);

    case 6:
        // 2CD4
        if (info->loopEnd < info->decodePos)
        {
            // 2D2C
            int ret = sub_31B4(info);
            return sub_2FEC(info->dataEnd, info->curOff + info->streamDataByte - ret, ret, info->bufferByte);
        }
        // fall thru
    case 5:
        // 2CF0
        return sub_3004(info->curOff, info->streamDataByte, getOffFromSample(info, info->loopStart), getSecondBufPos(info, info->loopEnd), info->bufferByte);
    }
    return 0;
}

// 2D54
u32 getWritableByte(SceAtracIdInfo *info)
{
    // 2D84
    u32 num1 = sub_316C(info->unk22 != 1 ? info->unk48 : 0, info->bufferByte, sub_31B4(info), info->sampleSize);
    u32 num2 = sub_3048(info);
    if (num2 >= num1)
        return num1;
    return num2;
}

int sub_2DB8(SceAtracIdInfo *info)
{
    if (info->streamDataByte >= info->dataEnd - info->curOff)
        return -2;
    return pspMax(info->streamDataByte / info->sampleSize - info->numFrame, 0);
}

int sub_2DF8(SceAtracIdInfo *info)
{
    int ret = sub_2EA4(info);
    if (info->loopNum < 0)
        return ret;
    u32 ret2 = getSecondBufPos(info, info->loopEnd);
    u32 sum = info->curOff + info->streamDataByte;
    if (sum >= ret2 + 1 && (s32)(sum - (ret2 + 1)) / (s32)(ret2 - getOffFromSample(info, info->loopStart) + 1) >= info->loopNum) // 2E78
        ret = -3;
    // 2E58
    return ret;
}

int sub_2EA4(SceAtracIdInfo *info)
{
    u32 ret = getSecondBufPos(info, info->loopEnd);
    u32 sum = info->curOff + info->streamDataByte;
    u32 sum2 = sum - ret - 1;
    int count;
    if (ret + 1 < sum)
    {
        // 2F28
        u32 ret2 = getFrameFromSample(info, info->loopStart);
        u32 unk = ret - getOffFromSample(info, info->loopStart) + 1;
        count = (ret - info->curOff + 1) / info->sampleSize + (sum2 / unk) * (unk / info->sampleSize - ret2);
        if (ret2 * info->sampleSize < sum2 % unk)
            count += (sum2 % unk) / info->sampleSize - ret2;
    }
    else
        count = info->streamDataByte / info->sampleSize;
    // 2EFC
    return pspMax(count - info->numFrame, 0);
}

int sub_2FA8(u32 arg0, u32 arg1, u32 arg2, u32 arg3)
{
    u32 num = arg0 + arg2;
    if (arg0 < arg1)
        arg1 = (((arg1 - arg0) / arg3) * arg3) + arg0;
    // 2FCC
    if (num < arg1)
        return num;
    if (arg1 >= arg2)
        return num - arg1;
    return 0;
}

u32 sub_2FEC(u32 arg0, u32 arg1, u32 arg2, u32 arg3)
{
    u32 num = arg1 + arg2;
    if (num < arg0 && arg2 < arg3)
        return num;
    return 0;
}

u32 sub_3004(u32 arg0, u32 arg1, u32 arg2, u32 arg3, u32 arg4)
{
    if (arg1 >= arg4)
        return 0;
    u32 unk = arg0 + arg1;
    if (unk < arg3 + 1)
        return unk;
    return arg2 + (unk - arg3 - 1) % (arg3 - arg2 + 1);
}

u32 sub_3048(SceAtracIdInfo *info)
{
    char state = info->state;
    if (state == 6)
    {
        // 30A8
        state = 5;
        if (info->loopEnd < info->decodePos)
            state = 4;
    }
    // 306C
    if (state == 4) {
        // 3098
        return sub_30C0(info);
    }
    if (state == 5) {
        // 3088
        return sub_30E0(info);
    }
    return 0;
}

u32 sub_30C0(SceAtracIdInfo *info)
{
    if (info->curOff + info->streamDataByte < info->dataEnd)
        return info->dataEnd - (info->curOff + info->streamDataByte);
    return 0;
}

u32 sub_30E0(SceAtracIdInfo *info)
{
    u32 ret = getSecondBufPos(info, info->loopEnd);
    u32 sum = info->curOff + info->streamDataByte;
    if (sum >= ret + 1)
    {
        // 3138
        u32 sum2 = ret - getOffFromSample(info, info->loopStart) + 1;
        return sum2 - (info->curOff + info->streamDataByte - ret - 1) % sum2;
    }
    return ret - sum + 1;
}

u32 sub_316C(u32 arg0, u32 arg1, u32 arg2, u32 arg3)
{
    if (arg0 < arg1)
        arg1 = ((arg1 - arg0) / arg3) * arg3 + arg0;
    // 3190
    if (arg0 + arg2 < arg1)
        return arg1 - (arg0 + arg2);
    if (arg1 >= arg2)
        return arg1 - arg2;
    return 0;
}

u32 sub_31B4(SceAtracIdInfo *info)
{
    if (info->loopEnd < info->decodePos && info->unk22 == 1)
    {
        u32 num1 = info->unk52;
        // 31E4
        u32 num2 = info->secondBufferByte;
        if (num1 < num2)
            num2 = ((num2 - num1) / info->sampleSize) * info->sampleSize + num1;
        // 320C
        if (num2 < num1)
            return 0;
        if (num2 - num1 >= info->streamDataByte)
            return 0;
        return info->streamDataByte - (num2 - num1);
    }
    // 31D8
    return info->streamDataByte;
}

u32 sub_3230(u32 arg0, u32 arg1, u32 arg2)
{
    if (arg0 >= arg2)
        return arg2;
    return ((arg2 - arg0) / arg1) * arg1 + arg0;
}

// 325C
void copyBuffer(SceAtracIdInfo *info, u32 bufferByte)
{
    u32 num1 = info->bufferByte;
    u32 num2 = info->dataOff;
    if (num2 < num1)
        num1 = ((num1 - num2) / info->sampleSize) * info->sampleSize + num2;
    // 3290
    u32 count = bufferByte - num1;
    if (count <= 0)
        return;
    sceKernelMemcpy(info->buffer, info->buffer + num1, count);
}

// 32B4
void getBufferInfo(SceAtracIdInfo *info, u32 sample, SceBufferInfo *bufferInfo)
{
    switch (info->state) // jump table at 0x3F38
    {
    case 3: {
        // 3308
        u32 num1 = info->dataOff + info->streamDataByte;
        u32 num2 = info->dataOff + ((sample >> (0x100B - info->codec)) + 1) * info->sampleSize;
        bufferInfo->writePositionFirstBuf = info->buffer + num1;
        bufferInfo->writableByteFirstBuf = info->dataEnd - num1;
        bufferInfo->readPositionFirstBuf = num1;
        if (num2 >= num1) {
            // 339C dup
            bufferInfo->minWriteByteFirstBuf = num2 - num1;
        }
        else
            bufferInfo->minWriteByteFirstBuf = 0;
    }
        break;

    case 4: case 5: {
        // 33BC
        u32 num1 = sub_3230(0, info->sampleSize, info->bufferByte);
        bufferInfo->writePositionFirstBuf = info->buffer;
        u32 ret = getOffFromSample(info, sample);
        bufferInfo->readPositionFirstBuf = ret;
        u32 num2 = info->dataEnd - ret;
        if (num2 >= num1)
            num2 = num1;
        bufferInfo->writableByteFirstBuf = num2;
        bufferInfo->minWriteByteFirstBuf = (getFrameFromSample(info, sample) + 1) * info->sampleSize;
    }
        break;

    case 6: {
        // 3420
        u32 ret1 = getOffFromSample(info, sample);
        u32 ret2 = getSecondBufPos(info, info->loopEnd);
        u32 ret3 = sub_3230(0, info->sampleSize, info->bufferByte);
        u32 mult = (getFrameFromSample(info, sample) + 1) * info->sampleSize;
        if (ret1 < ret2)
        {
            u32 diff = ret2 - ret1 + 1;
            if (diff < ret3)
                ret3 = diff;
            if (diff >= mult)
                diff = mult;
            bufferInfo->writePositionFirstBuf = info->buffer;
            bufferInfo->writableByteFirstBuf = ret3;
            bufferInfo->minWriteByteFirstBuf = diff;
            bufferInfo->readPositionFirstBuf = ret1;
            break;
        }
        // 34A8
        u32 num = ret2 + sub_3230(0, info->sampleSize, info->secondBufferByte);
        if (ret1 >= num)
        {
            bufferInfo->readPositionFirstBuf = ret1;
            // 351C
            bufferInfo->minWriteByteFirstBuf = mult;
            num = info->dataEnd - ret1;
            if (num >= ret3)
                num = ret3;
            bufferInfo->writePositionFirstBuf = info->buffer;
            bufferInfo->writableByteFirstBuf = num;
            break;
        }
        else if (ret2 + info->secondBufferByte + 1 < info->dataEnd)
        {
            num++;
            bufferInfo->readPositionFirstBuf = num;
            ret1 += mult;
            u32 num2 = info->dataEnd - num;
            if (num2 >= ret3)
                num2 = ret3;
            bufferInfo->writePositionFirstBuf = info->buffer;
            bufferInfo->writableByteFirstBuf = num2;
            if (ret1 >= num) {
                // 339C dup
                bufferInfo->minWriteByteFirstBuf = ret1 - num;
            }
            else
                bufferInfo->minWriteByteFirstBuf = 0;
            break;
        }
    }
        // fall thru

    default:
        // 33A4
        // 33A8
        bufferInfo->writableByteFirstBuf = 0;
        bufferInfo->writePositionFirstBuf = info->buffer;
        bufferInfo->minWriteByteFirstBuf = 0;
        bufferInfo->readPositionFirstBuf = 0;
        break;
    }
    // 3364
    bufferInfo->readPositionSecondBuf = 0;
    bufferInfo->writePositionSecondBuf = info->buffer;
    bufferInfo->writableByteSecondBuf = 0;
    bufferInfo->minWriteByteSecondBuf = 0;
}

// 3540
int resetPlayPos(SceAtracIdInfo *info, u32 sample, u32 writeByteFirstBuf, u32 writeByteSecondBuf)
{
    SceBufferInfo bufferInfo;
    getBufferInfo(info, sample, &bufferInfo);
    if (writeByteFirstBuf < bufferInfo.minWriteByteFirstBuf || bufferInfo.writableByteFirstBuf < writeByteFirstBuf) {
        // 358C
        return 0x80630016;
    }
    // 35B0
    if (writeByteSecondBuf < bufferInfo.minWriteByteSecondBuf || bufferInfo.writableByteSecondBuf < writeByteSecondBuf) {
        // 35C8
        return 0x80630017;
    }
    info->decodePos = sample;
    // 35D4
    info->numFrame = getFrameFromSample(info, sample);
    info->loopNum = 0;
    info->curOff = getOffFromSample(info, sample);
    switch (info->state)
    {
    case 6: {
        // 3660
        u32 ret1 = getOffFromSample(info, sample);
        u32 ret2 = getSecondBufPos(info, info->loopEnd);
        if (ret1 < ret2)
        {
                // 3650
            info->streamDataByte = writeByteFirstBuf;
            // 3654 dup
            info->unk22 = 0;
            // 3658 dup
            info->unk48 = 0;
        }
        else
        {
            u32 num = ret2 + sub_3230(0, info->sampleSize, info->secondBufferByte);
            if (ret1 >= num)
            {
                // 36DC
                info->streamDataByte = writeByteFirstBuf;
                info->unk22 = 2;
                // 3658 dup
                info->unk48 = 0;
            }
            else
            {
                info->streamDataByte = num - ret1 + writeByteFirstBuf + 1;
                info->unk22 = 1;
                info->unk52 = ret1 - ret2 - 1;
            }
        }
    }
        break;

    case 4: case 5:
        info->streamDataByte = writeByteFirstBuf;
        // 3654/3658 dup
        info->unk22 = 0;
        info->unk48 = 0;
        break;

    case 3:
        // 3624
        info->unk22 = 0;
        info->streamDataByte += writeByteFirstBuf;
        if (info->dataOff + info->streamDataByte >= info->dataEnd)
            info->state = 2;
        break;
    }
    // 361C
    return 0;
}

// 36E8
int parseAA3(u32 readByte, SceAA3File *aa3, int arg2, u8 *buffer)
{
    u32 curOff = 0;
    if (readByte < 3)
        return 0x80631004;
    int magic = sub_3B14(buffer, &curOff, readByte);
    if (magic != 0x656133 && magic != 0x494433) // 3ae / 3AE
    {
        // 3910
        aa3->unk40 = 0;
        aa3->dataOff = 0;
    }
    else
    {
        if (readByte - 3 < curOff)
            return 0x80631004;
        if (buffer[curOff] != 3 || buffer[curOff + 1] != 0)
            return 0x80631002;
        // 37B0
        curOff = curOff + 3;
        aa3->unk40 = sub_3B54(buffer, &curOff, readByte);
        if (aa3->unk40 == -1)
            return 0x80631003;
        curOff += aa3->unk40;
        if (readByte - 1 < curOff)
            return 0x80631004;
        if (buffer[curOff] == 0)
            curOff += 16;
        aa3->dataOff = curOff;
    }
    // 3810
    if (readByte - 34 < curOff)
        return 0x80631004;
    if (sub_3B14(buffer, &curOff, readByte) != 0x454133) // "3AE"
        return 0x80631003;
    curOff++;
    aa3->unk44 = sub_3AA0(buffer, &curOff, readByte);
    if (aa3->unk44 == -1)
        return 0x80631003;
    if (sub_3AA0(buffer, &curOff, readByte) != 0xFFFF)
        return 0x80631003;
    curOff += 24;
    aa3->unk6 = *(u8*)(buffer + curOff);
    int codec;
    if (*(u8*)(buffer + curOff) == 0)
        codec = 0x1001;
    else if (*(u8*)(buffer + curOff) == 1)
        codec = 0x1000;
    else
        return 0x80631004;
    // 38D8
    aa3->unk32 = sub_3A18(buffer, &curOff, readByte);
    int sum = aa3->dataOff + aa3->unk44;
    aa3->dataSize = arg2 - sum;
    aa3->dataOff = sum;
    return codec;
}

int setAtracFileInfo(SceAA3File *aa3, SceAtracFile *info)
{
    info->loopEnd = -1;
    info->loopStart = -1;
    info->dataSize = aa3->dataSize;
    info->dataOff = aa3->dataOff;
    if (aa3->unk6 != 0)
    {
        // 39A8
        if (aa3->unk6 != 1)
            return 0;
        info->samplesPerChan = 2048;
        if ((aa3->unk32 & 0x1C00) != 0x800)
            return 0x80631005;
        info->channels = 2;
        if ((aa3->unk32 & 0xE000) != 0x2000)
            return 0x80631005;
        info->factSz = 0;
        info->dataBlkSz = ((aa3->unk32 & 0x3FF) << 3) + 8;
        info->unk4.v8[0] = ((aa3->unk32 >> 8) & 3) | 0x28;
        info->unk4.v8[1] = aa3->unk32 & 0xFF;
    }
    else
    {
        info->samplesPerChan = 1024;
        info->channels = 2;
        if ((aa3->unk32 & 0xE000) != 0x2000)
            return 0x80631005;
        info->dataBlkSz = (aa3->unk32 & 0x3FF) << 3;
        if ((aa3->unk32 & 0x20000) == 0) {
            // 39A0
            info->unk4.v16 = 0;
        }
        else
            info->unk4.v16 = 1;
        // 3990
        info->factSz = 0;
    }
    // 3994
    return 0;
}

int sub_3A18(u8 *buffer, u32 *curOff, u32 arg2)
{
    int result = 0;
    if (arg2 < *curOff + 4)
    {
        // 3A64
        int i;
        for (i = 0; i < 4; i++)
        {
            u8 num = 0;
            if (arg2 >= *curOff + i + 1)
                num = buffer[*curOff + i];
            // 3A8C
            result |= num << (i * 8);
        }
        *curOff = arg2;
    }
    else
    {
        result = (buffer[*curOff + 0] << 24)
               | (buffer[*curOff + 1] << 16)
               | (buffer[*curOff + 2] <<  8)
               | (buffer[*curOff + 3] <<  0);
        *curOff += 4;
    }
    return result;
}

u16 sub_3AA0(u8 *buffer, u32 *curOff, u32 readByte)
{
    u16 result = 0;
    if (readByte < *curOff + 2)
    {
        // 3AD8
        int i;
        for (i = 0; i < 2; i++)
        {
            u8 num = 0;
            if (readByte >= *curOff + i + 1)
                num = buffer[*curOff + i];
            // 3B00
            result |= num << (i * 8);
        }
        *curOff = readByte;
    }
    else {
        result = (buffer[*curOff + 0] << 8) | buffer[*curOff + 1];
        *curOff += 2;
    }
    return result;
}

int sub_3B14(u8 *buffer, u32 *curOff, u32 readByte)
{
    if (readByte < *curOff + 3)
        return 0xFF000000;
    int result = (buffer[*curOff] << 16) | (buffer[*curOff + 1] << 8) | buffer[*curOff + 2];
    *curOff += 3;
    return result;
}

int sub_3B54(u8 *buffer, u32 *curOff, u32 readByte)
{
    if (readByte < *curOff + 4)
        return -1;
    int result = ((buffer[*curOff + 0] & 0x7F) << 21)
               | ((buffer[*curOff + 1] & 0x7F) << 14)
               | ((buffer[*curOff + 2] & 0x7F) <<  7)
               | ((buffer[*curOff + 3] & 0x7F) <<  0);
    *curOff += 4;
    return result;
}
