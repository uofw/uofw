/* Copyright (C) The uOFW team
   See the file COPYING for copying permission.
*/

#ifndef POWERSWITCH_H
#define POWERSWITCH_H

#include <common_header.h>

// TODO: I need theese two fucntions in src/power/powerSwitch.c alongside power_kernel.h. However, I can't
// include both power_kernel.h and power_user.h in that file, so I am using this header
// here as a workaround.

s32 scePowerLockForUser(s32 lockType);

s32 scePowerUnlockForUser(s32 lockType);

#endif // POWERSWITCH_H