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

int sceAudiocodecCheckNeedMem(u32 *codec_buffer, u32 codec)
{
    if (((0x00220202 >> (((int)codec_buffer >> 27) & 0x1F)) & 1) == 0)
        return 0x807F0002;
    if (codec < 0x1000 || codec >= 0x1006)
        return 0x80000004;
    // 0148
    K1_BACKUP();
    int ret = 0x80000023;
    if (K1_USER_BUF_STA_SZ(codec_buffer, 104))
    {
        *codec_buffer = 0x05100601;
        int id = sceMeAudio_driver_81956A0B(codec, codec_buffer);
        ret = 0;
        if (id != 0)
        {
            // 0188
            if (id < -4 || id >= 0)
                id = 0;
            ret = g_retValues[id + 4];
        }
    }
    K1_RESET();
    return ret;
}

int sceAudiocodecInit(void *codec_buf, int codec)
{   
    if (((0x00220202 >> (((int)codec_buf >> 27) & 0x1F)) & 1) == 0)
        return 0x807F0002;
    if (codec < 0x1000 || codec >= 0x1006)
        return 0x80000004;
    // 0214
    K1_BACKUP();
    if (!K1_USER_BUF_STA_SZ(codec_buf, 104) || !K1_USER_BUF_DYN_SZ(*(int*)(codec_buf + 12), *(int*)(codec_buf + 16))) {
        K1_RESET();
        return 0x80000023;
    }
    // 025C
    if (codec == 0x1002)
    {
        // 02CC
        *(int*)(codec_buf + 56) = 9999;
    }
    // 0264
    if (((0x00220202 >> ((*(int*)(codec_buf + 12) >> 27) & 0x1F)) & 1) != 0) {
        // 02BC
        sceKernelDcacheWritebackInvalidateRange(*(int*)(codec_buf + 12), *(int*)(codec_buf + 16));
    }
    // 027C
    *(int*)(codec_buf + 0) = 0x05100601;
    int id = sceMeAudio_driver_6AD33F60(codec, codec_buf);
    int ret = 0;
    if (id != 0)
    {
        if (id < -4 || id >= 0)
            id = 0;
        ret = g_retValues[id + 4];
    }
    K1_RESET();
    return ret;
}

int sceAudiocodec_3DD7EE1A(void *codec_buf, int codec)
{
    if (((0x00220202 >> (((int)codec_buf >> 27) & 0x1F)) & 1) == 0)
        return 0x807F0002;
    if (codec < 0x1000 || codec >= 0x1006)
        return 0x80000004;
    // 0344
    K1_BACKUP();
    if (!K1_USER_BUF_STA_SZ(codec_buf, 104) || !K1_USER_BUF_DYN_SZ(*(int*)(codec_buf + 12), *(int*)(codec_buf + 16))) {
        K1_RESET();
        return 0x80000023;
    }
    // 038C
    if (codec == 0x1002)
    {
        // 03FC
        *(int*)(codec_buf + 56) = 9999;
    }
    // 0394
    if (((0x00220202 >> ((*(int*)(codec_buf + 12) >> 27) & 0x1F)) & 1) != 0) {
        // 03EC
        sceKernelDcacheWritebackInvalidateRange(*(int*)(codec_buf + 12), *(int*)(codec_buf + 16));
    }
    // 03AC
    *(int*)(codec_buf + 0) = 0x05100601;
    int id = sceMeAudio_driver_B57F033A(codec, codec_buf);
    int ret = 0;
    if (id != 0)
    {
        if (id < -4 || id >= 0)
            id = 0;
        ret = g_retValues[id + 4];
    }
    K1_RESET();
    return ret;
}

int sceAudiocodecDecode(void *codec_buf, int codec)
{
    if (((0x00220202 >> (((int)codec_buf >> 27) & 0x1F)) & 1) == 0)
        return 0x807F0002;
    if (codec < 0x1000 || codec >= 0x1006)
        return 0x80000004;
    // 0474
    K1_BACKUP();
    if (!K1_USER_BUF_STA_SZ(a0, 104)
     || !K1_USER_BUF_DYN_SZ(*(int*)(a0 + 12), *(int*)(a0 + 16))
     || !K1_USER_BUF_STA_SZ(*(int*)(a0 + 24), 0x10000)
     || !K1_USER_BUF_STA_SZ(*(int*)(a0 + 32), 0x10000)) {
        // (04E0)
        // 04E4
        K1_RESET();
        return 0x80000023;
    }
    // 04F0
    int size;
    if (codec == 0x1002 || codec == 0x1004)
    {
        // 0570
        size = *(int*)(codec_buf + 40);
    }
    else
    {
        size = getBufSize(codec_buf, codec);
        if (size < 0) {
            K1_RESET();
            return size;
        }
    }
    // 0510
    sceKernelDcacheWritebackRange(*(int*)(codec_buf + 24), size);
    size = sub_0A40(codec_buf, codec);
    if (size < 0) {
        K1_RESET();
        return size;
    }
    sceKernelDcacheWritebackInvalidateRange(*(int*)(codec_buf + 32), size);
    int id = sceMeAudio_driver_9A9E21EE(codec, codec_buf);
    int ret = 0;
    if (id != 0)
    {
        if (id < -4 || id >= 0)
            id = 0;
        ret = g_retValues[id + 4];
    }
    K1_RESET();
    return ret;
}

int sceAudiocodecGetInfo(void *codec_buf, int codec)
{
    if (((0x00220202 >> (((int)codec_buf >> 27) & 0x1F)) & 1) == 0)
        return 0x807F0002;
    if (codec < 0x1000 || codec >= 0x1006)
        return 0x80000004;
    // 05D4
    K1_BACKUP();
    if (!K1_USER_BUF_STA_SZ(codec_buf, 104) || !K1_USER_BUF_DYN_SZ(*(int*)(codec_buf + 12), *(int*)(codec_buf + 16)))
    {
        // 060C
        K1_RESET();
        return 0x80000023;
    }
    // 0620
    if ((codec == 0x1002 || codec == 0x1004) && !K1_USER_BUF_DYN_SZ(*(int*)(codec_buf + 24), *(int*)(codec_buf + 40))) {
        K1_RESET();
        return 0x80000023;
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
        K1_RESET();
        return 0;
    }
    // 0684
    *(int*)(codec_buf + 0) = 0x05100601;
    int id = sceMeAudio_driver_C300D466(codec, opt, codec_buf);
    int ret = 0;
    if (id != 0)
    {
        // 06B0
        if (id < -4 || id >= 0)
            id = 0;
        ret = g_retValues[id + 4];
    }
    // 06A8
    K1_RESET();
    return ret;
}

int sceAudiocodecAlcExtendParameter(void *codec_buf, int codec, int *sizeOut)
{   
    if (((0x00220202 >> (((int)codec_buf >> 27) & 0x1F)) & 1) == 0)
        return 0x807F0002;
    if (codec < 0x1000 || codec >= 0x1006)
        return 0x80000004;
    // 0744
    K1_BACKUP();
    if (!K1_USER_BUF_STA_SZ(codec_buf, 104) || !K1_USER_BUF_STA_SZ(sizeOut, 4))
    {
        // 0770
        K1_RESET();
        return 0x80000023;
    }
    // 0780
    int size = sub_0A40(codec_buf, codec);
    if (size < 0) {
        K1_RESET();
        return size;
    }
    *sizeOut = size;
    K1_RESET();
    return 0;
}

int sceAudiocodecGetEDRAM(void *codec_buf, int codec)
{
    if (((0x00220202 >> (((int)codec_buf >> 27) & 0x1F)) & 1) == 0)
        return 0x807F0002;
    if (codec < 0x1000 || codec >= 0x1006)
        return 0x80000004;
    // 0808
    K1_BACKUP();
    int ret = 0;
    if (!K1_USER_BUF_STA_SZ(codec_buf, 108)) {
        K1_RESET();
        return 0x80000023;
    }
    // 0838
    int cnt = sceMeMalloc((*(int*)(codec_buf + 16) + 0x3F) | 0x3F)
    *(int*)(codec_buf + 104) = cnt;
    if (cnt == 0 || (cnt & 0x1FFFFFFF) > 0x003FFFFF)
    {
        // (087C)
        // 0880
        ret = 0x807F0003;
        sceMeFree(*(int*)(codec_buf + 104));
        *(int*)(codec_buf + 104) = 0;
        *(int*)(codec_buf + 12) = 0;
    }
    else
        *(int*)(codec_buf + 12) = (cnt + 0x3F) & 0xFFFFFFC0;
    K1_RESET();
    return ret;
}

int sceAudiocodecReleaseEDRAM(void *codec_buf)
{   
    K1_BACKUP();
    if (!K1_USER_BUF_STA_SZ(codec_buf, 108) || !K1_USER_BUF_DYN_SZ(*(int*)(codec_buf + 12), *(int*)(codec_buf + 16))) {
        K1_RESET();
        return 0x80000023;
    }
    if (*(int*)(codec_buf + 104) == 0 || *(int*)(codec_buf + 104) & 0x1FFFFFFF > 0x3FFFFF) {
        K1_RESET();
        return 0x807F0004;
    }
    // 0938
    sceMeFree(*(int*)(codec_buf + 104));
    *(int*)(codec_buf + 104) = 0;
    *(int*)(codec_buf + 12) = 0;
    K1_RESET();
    return 0;
}

int getBufSize(void *codec_buf, int codec)
{
    if (codec < 0x1000 || codec >= 0x1006)
        return 0x80000004;

    switch (codec) // jump table at 0x49C0
    {
    case 0x1000:
        // 0990
        if (*(int*)(codec_buf + 48) == 0)
            return *(int*)(codec_buf + 64) + 2;
        return 0x100A;

    case 0x1001:
        // 09B8
        return *(int*)(codec_buf + 48) * 2 + 1;

    case 0x1002:
        // 09C8
        if (*(int*)(codec_buf + 56) == 9999)
        {
            // 09F8
            int ret = sceAudiocodecGetInfo(codec_buf, 0x1002);
            if (ret < 0)
                return ret;
        }
        // 09D8
        // 09E8 dup
        return sub_0B18(*(int*)(codec_buf + 56), *(int*)(codec_buf + 60), *(int*)(codec_buf + 68), *(int*)(codec_buf + 72), codec_buf + 40);

    case 0x1003:
        // 0A10
        if (*(u8*)(codec_buf + 44) == 0)
            return 0x600;
        return 0x609;

    case 0x1004:
        // 0A24
        // 09E8 dup
        return sub_0B18(*(int*)(codec_buf + 48), *(int*)(codec_buf + 52), *(int*)(codec_buf + 60), *(int*)(codec_buf + 64), codec_buf + 40);

    case 0x1005:
        // 0A38
        return 0x420D;
    }
}

int sub_0A40(void *codec_buf, int codec)
{
    if (codec < 0x1000 || codec >= 0x1006)
        return 0x1000;
    switch (codec) // jump table at 0x49D8
    {
    case 0x1000:
        // 0A7C
        if (*(int*)(codec_buf + 56) == 1 && *(int*)(s0 + 72) != *(int*)(s0 + 56)) // 0AA4
            return 0x2000;
        // 0A8C
        return *(int*)(codec_buf + 72) << 12;

    case 0x1001:
        // 0AB4
        return 0x1000;

    case 0x1002:
        // 0ABC
        if (*(int*)(codec_buf + 56) == 9999)
        {
            // 0AE0
            int ret = sceAudiocodecGetInfo(codec_buf, 0x1002);
            if (ret < 0)
                return ret;
        }
        // 0ACC
        // 0AD8 dup
        if (*(int*)(codec_buf + 56) == 1)
            return 0x1200;
        return 0x900;

    case 0x1003:
        // 0AF8
        // 0AD8 dup
        if (*(u8*)(codec_buf + 45) == 0)
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
        size /= 1;
    *sizePtr = size;
    return size;
}

