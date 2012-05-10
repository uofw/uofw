/* Copyright (C) 2011, 2012 The uOFW team
   See the file COPYING for copying permission.
*/

#include "common.h"

enum PSPApplicationType
{
	PSP_INIT_ALLPICATION_VSH = 0x100,
	PSP_INIT_ALLPICATION_GAME = 0x200,
	PSP_INIT_ALLPICATION_POPS = 0x300,
};

int sceKernelSetInitCallback(void *, int, int);
int sceKernelApplicationType(void);

