/* Copyright (C) 2011, 2012, 2013 The uOFW team
   See the file COPYING for copying permission.
*/

#ifndef LIBAAC_H
#define LIBAAC_H

#include <common_header.h>

typedef struct {
    s32 streamStart; // 0
    s32 start; // 4
    s32 streamEnd; // 8
    s32 end; // 12
    void *encBuf; // 16
    s32 encSize; // 20
    void *decBuf; // 24
    s32 decSize; // 28
    s32 sampleRate; // 32
    s32 zero; // 36
} SceAacInitArg;

s32 sceAacEndEntry(void);
s32 sceAacInitResource(s32 nbr);
s32 sceAacTermResource(void);
s32 sceAacInit(SceAacInitArg *arg);
s32 sceAac_E955E83A(s32 *sampleRate);
s32 sceAacExit(s32 id);
s32 sceAacDecode(s32 id, void** src);
s32 sceAac_FA01FCB6(s32 id, void *arg1, s32 *arg2, void *arg3, s32 *arg4);
s32 sceAacCheckStreamDataNeeded(s32 id);
s32 sceAacGetInfoToAddStreamData(s32 id, s32 **arg1, s32 *arg2, s32 *arg3);
s32 sceAacNotifyAddStreamData(s32 id, s32 size);
s32 sceAacResetPlayPosition(s32 id);
s32 sceAacSetLoopNum(s32 id, s32 loopNum);
s32 sceAacGetMaxOutputSample(s32 id);
s32 sceAacGetSumDecodedSample(s32 id);
s32 sceAacGetLoopNum(s32 id);
s32 sceAacStartEntry(SceSize argc, void *argp);

#endif /* LIBAAC_H */
