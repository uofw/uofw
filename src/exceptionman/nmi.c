/* Copyright (C) 2011, 2012 The uOFW team
   See the file COPYING for copying permission.
*/

#include <common_imp.h>

#include "excep.h"
#include "exceptionman.h"
#include "intr.h"

// 1848
void *g_nmiHandlers[16] = { NULL };

int NmiManInit(void)
{
    dbg_printf("- NmiManInit()\n");
    int oldIntr = suspendIntr();
    int i;
    pspCop0CtrlSet(COP0_CTRL_NMI_TABLE, 0);
    // 0AEC
    for (i = 0; i < 16; i++)
        g_nmiHandlers[i] = NULL;
    nmiInit();
    pspCop0CtrlSet(COP0_CTRL_NMI_TABLE, (int)g_nmiHandlers);
    sceKernelRegisterPriorityExceptionHandler(31, 1, nmiHandler);
    resumeIntr(oldIntr);
    dbg_printf("- NmiManInit ended\n");
    return 0;
}

int sceKernelRegisterNmiHandler(int nmino, void (*func)())
{
    dbg_printf("exec %s\n", __FUNCTION__);
    if (nmino < 0 || nmino >= 17)
        return 0x8002003A;
    int oldIntr = suspendIntr();
    g_nmiHandlers[nmino] = func;
    resumeIntr(oldIntr);
    dbg_printf("(end)\n");
    return 0;
}

int sceKernelReleaseNmiHandler(int nmino)
{
    dbg_printf("exec %s\n", __FUNCTION__);
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
    dbg_printf("(end)\n");
    return ret;
}

