/* Copyright (C) 2011, 2012 The uOFW team
   See the file COPYING for copying permission.
*/

#include "common_header.h"

typedef struct
{
    s32 unk0;
    s32 unk4;
    s32 err; // 8
    s32 edramAddr; // 12
    s32 neededMem; // 16
    s32 unk20;
    void *inBuf; // 24
    s32 unk28;
    void *outBuf; // 32
    s32 unk36;

   /* Note: this part is probably completely different depending on the codec. This should be cleaned up. */
    union {
        struct {
            s8 u40;
            s8 u41;
            s8 u42;
            s8 u43;
        } v8;
        s32 v32;
    } unk40;
    union {
        struct {
            u8 u44;
            s8 u45;
            s8 u46;
            s8 u47;
        } v8;
        struct {
            s16 u44;
            s16 u46;
        } v16;
        s32 v32;
    } unk44;
    s32 unk48;
    s32 unk52;
    s32 unk56;
    s32 unk60;
    s32 unk64;
    s32 unk68;
    s32 unk72;
    s32 unk76;
    s32 unk80;
    s32 unk84;
    s32 unk88;
    s32 unk92;
    s32 unk96;
    s32 unk100;
    void *allocMem; // 104
} SceAudiocodecCodec;

s32 sceAudiocodecCheckNeedMem(SceAudiocodecCodec *info, s32 codec);
s32 sceAudiocodecInit(SceAudiocodecCodec *info, s32 codec);
s32 sceAudiocodec_3DD7EE1A(SceAudiocodecCodec *info, s32 codec);
s32 sceAudiocodecDecode(SceAudiocodecCodec *info, s32 codec);
s32 sceAudiocodecGetInfo(SceAudiocodecCodec *info, s32 codec);
s32 sceAudiocodecAlcExtendParameter(SceAudiocodecCodec *info, s32 codec, s32 *sizeOut);
s32 sceAudiocodecGetEDRAM(SceAudiocodecCodec *info, s32 codec);
s32 sceAudiocodecReleaseEDRAM(SceAudiocodecCodec *info);


