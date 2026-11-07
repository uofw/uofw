/* Copyright (C) 2011, 2012 The uOFW team
   See the file COPYING for copying permission.
*/

#include "common_header.h"

int sceDdrFlush(int);

int sceDdrGetPowerDownCounter(void);
int sceDdrSetPowerDownCounter(s32 counter);

// TODO: unsure about how many arguments (0/1/2 - opinions needed)
s32 sceDdr_driver_0BAAE4C5(void);

s32 sceDdrChangePllClock(u32 pllOutSelect);
