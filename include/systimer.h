/* Copyright (C) 2011, 2012 The uOFW team
   See the file COPYING for copying permission.
*/

#include "common.h"

typedef int SceSysTimerId;
typedef int (*SceSysTimerCb)(int, int, int, int);

int sceSTimerSetPrscl(SceSysTimerId timerId, int arg1, int arg2);
int sceSTimerAlloc(void);
int sceSTimerSetHandler(SceSysTimerId timerId, int arg1, SceSysTimerCb arg2, int arg3);
int sceSTimerStartCount(SceSysTimerId timerId);
int sceSTimerGetCount(SceSysTimerId timerId, int *arg1);
int sceSTimerResetCount(SceSysTimerId timerId);
int sceSTimerSetTMCY(SceSysTimerId timerId, int arg1);
int sceSTimerStopCount(SceSysTimerId timerId);
int sceSTimerFree(SceSysTimerId timerId);

