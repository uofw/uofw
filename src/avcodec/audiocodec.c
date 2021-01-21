#include <common_header.h>

// retValues
int g_retValues[] = { 0x807F00FC, 0x807F00FD, 0x80000002, 0x807F00FF, 0x807F0001 };

// 49F0
char g_numSamples[][15] = {
    { 0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
      0x08, 0x0A, 0x0C, 0x0E, 0x10, 0x12, 0x14 },
    { 0x00, 0x04, 0x05, 0x06, 0x07, 0x08, 0x0A, 0x0C, 
      0x0E, 0x10, 0x14, 0x18, 0x1C, 0x20, 0x28 }
};

// 4A10
short g_freqs[][4] = {
    { 22050, 24000, 16000, 0 },
    { 44100, 48000, 32000, 0 },
    { 11025, 12000, 8000, 0 }
};

int sceAudiocodecCheckNeedMem(SceAudiocodecCodec *info, int codec)
{
    if (!ADDR_IS_RAM(info))
        return 0x807F0002;
    if (codec < 0x1000 || codec >= 0x1006)
        return SCE_ERROR_NOT_SUPPORTED;
    // 0148
    int oldK1 = pspShiftK1();
    int ret = SCE_ERROR_PRIV_REQUIRED;
    if (pspK1StaBufOk(info, 104))
    {
        info->unk0 = 0x05100601;
        int id = sceMeAudio_driver_81956A0B(codec, info);
        ret = 0;
        if (id != 0)
        {
            // 0188
            if (id < -4 || id >= 0)
                id = 0;
            ret = g_retValues[id + 4];
        }
    }
    pspSetK1(oldK1);
    return ret;
}

int sceAudiocodecInit(SceAudiocodecCodec *info, int codec)
{   
    if (!ADDR_IS_RAM(info))
        return 0x807F0002;
    if (codec < 0x1000 || codec >= 0x1006)
        return SCE_ERROR_NOT_SUPPORTED;
    // 0214
    int oldK1 = pspShiftK1();
    if (!pspK1StaBufOk(info, 104) || !pspK1DynBufOk(info->edramAddr, info->neededMem)) {
        pspSetK1(oldK1);
        return SCE_ERROR_PRIV_REQUIRED;
    }
    // 025C
    if (codec == 0x1002)
    {
        // 02CC
        info->unk56 = 9999;
    }
    // 0264
    if (ADDR_IS_RAM(info->edramAddr))
        // 02BC
        sceKernelDcacheWritebackInvalidateRange(info->edramAddr, info->neededMem);
    }
    // 027C
    info->unk0 = 0x05100601;
    int id = sceMeAudio_driver_6AD33F60(codec, info);
    int ret = 0;
    if (id != 0)
    {
        if (id < -4 || id >= 0)
            id = 0;
        ret = g_retValues[id + 4];
    }
    pspSetK1(oldK1);
    return ret;
}

int sceAudiocodec_3DD7EE1A(SceAudiocodecCodec *info, int codec)
{
    if (!ADDR_IS_RAM(info))
        return 0x807F0002;
    if (codec < 0x1000 || codec >= 0x1006)
        return SCE_ERROR_NOT_SUPPORTED;
    // 0344
    int oldK1 = pspShiftK1();
    if (!pspK1StaBufOk(info, 104) || !pspK1DynBufOk(info->edramAddr, info->neededMem)) {
        pspSetK1(oldK1);
        return SCE_ERROR_PRIV_REQUIRED;
    }
    // 038C
    if (codec == 0x1002)
    {
        // 03FC
        info->unk56 = 9999;
    }
    // 0394
    if (ADDR_IS_RAM(info->edramAddr))
        // 03EC
        sceKernelDcacheWritebackInvalidateRange(info->edramAddr, info->neededMem);
    }
    // 03AC
    info->unk0 = 0x05100601;
    int id = sceMeAudio_driver_B57F033A(codec, info);
    int ret = 0;
    if (id != 0)
    {
        if (id < -4 || id >= 0)
            id = 0;
        ret = g_retValues[id + 4];
    }
    pspSetK1(oldK1);
    return ret;
}

int sceAudiocodecDecode(SceAudiocodecCodec *info, int codec)
{
    if (!ADDR_IS_RAM(info))
        return 0x807F0002;
    if (codec < 0x1000 || codec >= 0x1006)
        return SCE_ERROR_NOT_SUPPORTED;
    // 0474
    int oldK1 = pspShiftK1();
    if (!pspK1StaBufOk(info, 104)
     || !pspK1DynBufOk(info->edramAddr, info->neededMem)
     || !pspK1StaBufOk(info->inBuf, 0x10000)
     || !pspK1StaBufOk(info->outBuf, 0x10000)) {
        // (04E0)
        // 04E4
        pspSetK1(oldK1);
        return SCE_ERROR_PRIV_REQUIRED;
    }
    // 04F0
    int size;
    if (codec == 0x1002 || codec == 0x1004)
    {
        // 0570
        size = *(int*)&info->unk40;
    }
    else
    {
        size = getBufSize(info, codec);
        if (size < 0) {
            pspSetK1(oldK1);
            return size;
        }
    }
    // 0510
    sceKernelDcacheWritebackRange(info->inBuf, size);
    size = sub_0A40(info, codec);
    if (size < 0) {
        pspSetK1(oldK1);
        return size;
    }
    sceKernelDcacheWritebackInvalidateRange(info->outBuf, size);
    int id = sceMeAudio_driver_9A9E21EE(codec, info);
    int ret = 0;
    if (id != 0)
    {
        if (id < -4 || id >= 0)
            id = 0;
        ret = g_retValues[id + 4];
    }
    pspSetK1(oldK1);
    return ret;
}

int sceAudiocodecGetInfo(SceAudiocodecCodec *info, int codec)
{
    if (!ADDR_IS_RAM(info))
        return 0x807F0002;
    if (codec < 0x1000 || codec >= 0x1006)
        return SCE_ERROR_NOT_SUPPORTED;
    // 05D4
    int oldK1 = pspShiftK1();
    if (!pspK1StaBufOk(info, 104) || !pspK1DynBufOk(info->edramAddr, info->neededMem))
    {
        // 060C
        pspSetK1(oldK1);
        return SCE_ERROR_PRIV_REQUIRED;
    }
    // 0620
    if ((codec == 0x1002 || codec == 0x1004) && !pspK1DynBufOk(info->inBuf, (int*)&info->unk40)) {
        pspSetK1(oldK1);
        return SCE_ERROR_PRIV_REQUIRED;
    }
    // 0660
    int opt;
    switch (codec)
    {
    case 0x1002:
    case 0x1004:
        // 06D8
        opt = 2;
        break;

    case 0x1000:
        opt = 4;
        break;
    
    default:
        pspSetK1(oldK1);
        return 0;
    }
    // 0684
    info->unk0 = 0x05100601;
    int id = sceMeAudio_driver_C300D466(codec, opt, info);
    int ret = 0;
    if (id != 0)
    {
        // 06B0
        if (id < -4 || id >= 0)
            id = 0;
        ret = g_retValues[id + 4];
    }
    // 06A8
    pspSetK1(oldK1);
    return ret;
}

int sceAudiocodecAlcExtendParameter(SceAudiocodecCodec *info, int codec, int *sizeOut)
{   
    if (!ADDR_IS_RAM(info))
        return 0x807F0002;
    if (codec < 0x1000 || codec >= 0x1006)
        return SCE_ERROR_NOT_SUPPORTED;
    // 0744
    int oldK1 = pspShiftK1();
    if (!pspK1StaBufOk(info, 104) || !pspK1StaBufOk(sizeOut, 4))
    {
        // 0770
        pspSetK1(oldK1);
        return SCE_ERROR_PRIV_REQUIRED;
    }
    // 0780
    int size = sub_0A40(info, codec);
    if (size < 0) {
        pspSetK1(oldK1);
        return size;
    }
    *sizeOut = size;
    pspSetK1(oldK1);
    return 0;
}

int sceAudiocodecGetEDRAM(SceAudiocodecCodec *info, int codec)
{
    if (!ADDR_IS_RAM(info))
        return 0x807F0002;
    if (codec < 0x1000 || codec >= 0x1006)
        return SCE_ERROR_NOT_SUPPORTED;
    // 0808
    int oldK1 = pspShiftK1();
    int ret = 0;
    if (!pspK1StaBufOk(info, 108)) {
        pspSetK1(oldK1);
        return SCE_ERROR_PRIV_REQUIRED;
    }
    // 0838
    void *alloc = sceMeMalloc((info->neededMem + 0x3F) | 0x3F)
    info->allocMem = alloc;
    if (alloc == NULL || ((int)alloc & 0x1FFFFFFF) > 0x003FFFFF)
    {
        // (087C)
        // 0880
        ret = 0x807F0003;
        sceMeFree(info->allocMem);
        info->allocMem = NULL;
        info->edramAddr = 0;
    }
    else
        info->edramAddr = ((int)alloc + 0x3F) & 0xFFFFFFC0;
    pspSetK1(oldK1);
    return ret;
}

int sceAudiocodecReleaseEDRAM(SceAudiocodecInfo *info)
{   
    int oldK1 = pspShiftK1();
    if (!pspK1StaBufOk(info, 108) || !pspK1DynBufOk(info->edramAddr, info->neededMem)) {
        pspSetK1(oldK1);
        return SCE_ERROR_PRIV_REQUIRED;
    }
    if (info->allocMem == NULL || ((int)info->allocMem & 0x1FFFFFFF) > 0x3FFFFF) {
        pspSetK1(oldK1);
        return 0x807F0004;
    }
    // 0938
    sceMeFree(info->allocMem);
    info->allocMem = NULL;
    info->edramAddr = 0;
    pspSetK1(oldK1);
    return 0;
}

int getBufSize(SceAudiocodecCodec *info, int codec)
{
    if (codec < 0x1000 || codec >= 0x1006)
        return SCE_ERROR_NOT_SUPPORTED;

    switch (codec) // jump table at 0x49C0
    {
    case 0x1000:
        // 0990
        if (info->unk48 == 0)
            return info->unk64 + 2;
        return 0x100A;

    case 0x1001:
        // 09B8
        return info->unk48 * 2 + 1;

    case 0x1002:
        // 09C8
        if (info->unk56 == 9999)
        {
            // 09F8
            int ret = sceAudiocodecGetInfo(info, 0x1002);
            if (ret < 0)
                return ret;
        }
        // 09D8
        // 09E8 dup
        return sub_0B18(info->unk56, info->unk60, info->unk68, info->unk72, &info->unk40);

    case 0x1003:
        // 0A10
        if (info->unk44 == 0)
            return 0x600;
        return 0x609;

    case 0x1004:
        // 0A24
        // 09E8 dup
        return sub_0B18(info->unk48, info->unk52, info->unk60, info->unk64, &info->unk40);

    case 0x1005:
        // 0A38
        return 0x420D;
    }
}

int sub_0A40(SceAudiocodecCodec *info, int codec)
{
    if (codec < 0x1000 || codec >= 0x1006)
        return 0x1000;
    switch (codec) // jump table at 0x49D8
    {
    case 0x1000:
        // 0A7C
        if (info->unk56 == 1 && info->unk72 != info->unk56)
            return 0x2000;
        // 0A8C
        return info->unk72 << 12;

    case 0x1001:
        // 0AB4
        return 0x1000;

    case 0x1002:
        // 0ABC
        if (info->unk56 == 9999)
        {
            // 0AE0
            int ret = sceAudiocodecGetInfo(info, 0x1002);
            if (ret < 0)
                return ret;
        }
        // 0ACC
        // 0AD8 dup
        if (info->unk56 == 1)
            return 0x1200;
        return 0x900;

    case 0x1003:
        // 0AF8
        // 0AD8 dup
        if (info->unk45 == 0)
            return 0x1000;
        return 0x2000;

    case 0x1004:
        // 0B08
        return 0x1200;

    case 0x1005:
        // 0B10
        return 0x2F00;
    }
}

int sub_0B18(int type, int arg1, int sampleType, int freqType, int *sizePtr)
{
    char samples = 0;
    if (type >= 0 && type < 3 && sampleType >= 0 && sampleType < 15)
        samples = g_numSamples[type & 1][sampleType];
    // 0B7C
    short freq = 0;
    int size = 0x7C9;
    if (type >= 0 && type < 3 && freqType >= 0 && freqType < 4)
        freq = g_freqs[type][freqType];
    // 0BAC
    if (samples != 0 && freq != 0 && arg1 >= 1 && arg1 < 3)
    {
        size = ((samples * 144) / freq) + 1;
        if (size > 0x7C9)
            size = 0x7C9;
    }
    // 0BE4
    // 0BE8
    if (type != 1)
        size /= 2;
    *sizePtr = size;
    return size;
}

