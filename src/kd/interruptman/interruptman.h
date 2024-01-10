/* Copyright (C) 2011, 2012 The uOFW team
   See the file COPYING for copying permission.
*/

void *mymemset(void *dstVoid, s8 c, s32 n);
s32 SuspendIntc(s32 unk, void *param);
s32 ResumeIntc(s32 unk, void *param);
void InterruptDisableInTable(s32 intrNum);
void sub_29B0(s32 intrNum);
void AllLevelInterruptDisable(s32 intrNum);

