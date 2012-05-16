/* Copyright (C) 2011, 2012 The uOFW team
   See the file COPYING for copying permission.
*/

#include "common.h"

enum SceApplicationType {
    SCE_INIT_APPLICATION_VSH = 0x100,      
    SCE_INIT_APPLICATION_UPDATER = 0x110, /* Might be incorrect. */
    SCE_INIT_APPLICATION_GAME = 0x200,
    SCE_INIT_APPLICATION_POPS = 0x300,
};

int sceKernelSetInitCallback(void *, int, int);
int sceKernelApplicationType(void);

