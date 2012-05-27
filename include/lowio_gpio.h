/* Copyright (C) 2011, 2012 The uOFW team
   See the file COPYING for copying permission.
*/

s32 sceGpioPortSet(s32);
s32 sceGpioPortClear(s32);
s32 sceGpioSetPortMode(s32, s32);
s32 sceGpioDisableTimerCapture(s32, s32);
s32 sceGpioSetIntrMode(s32, s32);
s32 sceGpioAcquireIntr(s32);
s32 sceGpioQueryIntr(s32);
s32 sceGpioPortRead(void);

