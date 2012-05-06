/* Copyright (C) 2011, 2012 The uOFW team
   See the file COPYING for copying permission.
*/

#include "common.h"

static int g_0000C820;
static int g_0000C83C;

/* sceGpio_driver_317D9D2C */
int
sceGpioSetPortMode(u32 port, int mode)
{
    u32 intr;
    int r;
    u32 s3, s4, t5, t7;

    if (port >= 0x20)
        return 0x80000102; /* bad port */

    switch (mode) {
    case 1:
        s4 = 0;
        s3 = 1;
        break;

    case 0:
        s4 = 1;
        s3 = 0;
        break;

    case 2:
        s4 = 0;
        s3 = 0;
        break;

    default:
        return 0x80000107; /* bad mode */
    }

    /* loc_00002EB0 */
    intr = sceKernelCpuSuspendIntr();
    r = 0;

    /* 0x00002ED0 */
    if ((g_0000C820 & (1 << port)) == 0) {
        r = 1;
        if ((g_0000C83C & (1 << port)) == 0)
            r = 2;
    }

    /* loc_00002EEC */
    t7 = (g_0000C820 & (~(1 << port))) | (s4 << port);
    t5 = (g_0000C83C & (~(1 << port))) | (s3 << port);

    g_0000C820 = t7;
    _sw(0xBE240000, t7);
    g_0000C83C = t5;
    _sw(0xBE240040, t5);

    if (mode != 2)
        sceSysregGpioIoEnable(port);
    else
        sceSysregGpioIoDisable(port);

    sceKernelCpuResumeIntr(intr);

    return r;
}
