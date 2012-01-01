/* Copyright (C) 2011, 2012 The uOFW team
   See the file COPYING for copying permission.
*/

#include "../global.h"

#include "excep.h"
#include "exceptionman.h"
#include "intr.h"

// 1848
void *g_nmiHandlers[16];

int NmiManInit(void)
{   
    int oldIntr = suspendIntr();
    COP0_CTRL_SET(18, 0);
    // 0AEC
    int i;
    for (i = 0; i < 16; i++)
        g_nmiHandlers[i] = NULL;
    sub_0180();
    COP0_CTRL_SET(18, g_nmiHandlers);
    sceKernelRegisterPriorityExceptionHandler(31, 1, sub_01A4);
    resumeIntr(oldIntr);
    return 0;
}

int sceKernelRegisterNmiHandler(int nmino, void (*func)())
{
    if (nmino < 0 || nmino >= 17)
        return 0x8002003A;
    int oldIntr = suspendIntr();
    g_nmiHandlers[nmino] = func;
    resumeIntr(oldIntr);
    return 0;
}

int sceKernelReleaseNmiHandler(int nmino)
{   
    int ret = 0;
    if (nmino < 0 || nmino >= 17)
        return 0x8002003A;
    int oldIntr = suspendIntr();
    if (g_nmiHandlers[nmino] != NULL)
        g_nmiHandlers[nmino] = NULL;
    else
        ret = 0x80020068;

    // 0BFC
    resumeIntr(oldIntr);
    return ret;
}

