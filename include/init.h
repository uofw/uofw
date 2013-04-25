/** Copyright (C) 2011, 2012, 2013 The uOFW team
   See the file COPYING for copying permission.\n
*/

#ifndef INIT_H
#define	INIT_H

#include "common_imp.h"

#include "loadcore.h"
#include "loadexec_kernel.h"

/** The internal name of the INIT module. */
#define INIT_MODULE_NAME        "sceInit"

typedef struct {
    s32 apiType; //0
    void *fileModAddr; //4
    u32 discModAddr; //8
    SceKernelLoadExecVSHParam vshParam; //12
    s32 unk60;
    s32 unk64;
    s32 unk68;
    s32 unk72;
    s32 unk76;
    s32 unk80;
    s32 unk84;
    s32 unk88;
    u32 applicationType; //92
    s32 numPowerLocks;
    void *paramSfoBase; //100
    SceSize paramSfoSize; //104
    s32 lptSummary;
    SceBootCallback *bootCallbacks1;
    void *unk116;
    SceBootCallback *bootCallbacks2; //120
    void *unk124;
} SceInit;

#endif	/* INIT_H */

