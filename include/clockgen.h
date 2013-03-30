/* Copyright (C) 2011, 2012, 2013 The uOFW team
   See the file COPYING for copying permission.
*/

#ifndef CLOCKGEN_H
#define CLOCKGEN_H

#include <common_header.h>

s32 sceClockgenSetup();
s32 sceClockgenSetSpectrumSpreading(s32 arg);
s32 sceClockgenInit();
s32 sceClockgenEnd();
void sceClockgenSetProtocol(u32 prot);
s32 sceClockgenGetRevision();
s32 sceClockgenGetRegValue(u32 idx);
s32 sceClockgenAudioClkSetFreq(u32 freq);
s32 sceClockgenAudioClkEnable();
s32 sceClockgenAudioClkDisable();
s32 sceClockgenLeptonClkEnable();
s32 sceClockgenLeptonClkDisable();

#endif /* CLOCKGEN_H */
