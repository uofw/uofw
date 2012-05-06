/* Copyright (C) 2011, 2012 The uOFW team
   See the file COPYING for copying permission.
*/

#include "common.h"

int sub_01B8(int flag, int enable)
{
    int oldIntr = sceKernelCpuSuspendIntr();
    int reg = LW(0xBC10004C);
    *(int*)(0xBC10004C) = enable ? (reg | flag) : (reg ^ (reg & flag));
    sceKernelCpuResumeIntr(intr);
    return (reg & flag) != 0;
}

/* sceSysreg_driver_47C971B2 */
int sceSysregTopResetEnable(void)
{
    return sub_01B8(1, 1);
}

/* sceSysreg_driver_655C9CFC */
int sceSysregScResetEnable(void)
{
    return sub_01B8(2, 1);
}

/* sceSysreg_driver_457FEBA9 */
int sceSysregMeResetEnable(void)
{
    return sub_01B8(4, 1);
}

/* sceSysreg_driver_48F1C4AD */
int sceSysregMeResetDisable(void)
{
    return sub_01B8(4, 0);
}

/* sceSysreg_driver_66899952 */
int sceSysregAwResetEnable(void)
{
    return sub_01B8(8, 1);
}

/* sceSysreg_driver_AEB8DBD1 */
int sceSysregAwResetDisable(void)
{
    return sub_01B8(8, 0);
}

/* sceSysreg_driver_1A27B224 */
int sceSysregVmeResetEnable(void)
{
    return sub_01B8(0x10, 1);
}

/* sceSysreg_driver_B73D3619 */
int sceSysregVmeResetDisable(void)
{
    return sub_01B8(0x10, 0);
}

/* sceSysreg_driver_0AE8E549 */
int sceSysregAvcResetEnable(void)
{
    return sub_01B8(0x20, 1);
}

/* sceSysreg_driver_55FF02E9 */
int sceSysregAvcResetDisable(void)
{
    return sub_01B8(0x20, 0);
}

/* sceSysreg_driver_30C0A141 */
int sceSysregUsbResetEnable(void)
{
    return sub_01B8(0x40, 1);
}

/* sceSysreg_driver_9306F27B */
int sceSysregUsbResetDisable(void)
{
    return sub_01B8(0x40, 0);
}

/* sceSysreg_driver_64C8E8DD */
int sceSysregAtaResetEnable(void)
{
    return sub_01B8(0x80, 1);
}

/* sceSysreg_driver_8CFD0DCA */
int sceSysregAtaResetDisable(void)
{
    return sub_01B8(0x80, 0);
}

/* sceSysreg_driver_370419AD */
int sceSysregMsifResetEnable(int no)
{
    if (no < 0 || no >= 2)
        return 0x80000102;

    return sub_01B8(0x100 << no, 1);
}

/* sceSysreg_driver_7DD0CBEE */
int sceSysregMsifResetDisable(int no)
{
    if (no < 0 || no >= 2)
        return 0x80000102;

    return sub_01B8(0x100 << no, 0);
}

/* sceSysreg_driver_C1A37B37 */
int sceSysregKirkResetEnable(void)
{
    return sub_01B8(0x400, 1);
}

/* sceSysreg_driver_2F9B03E0 */
int sceSysregKirkResetDisable(void)
{
    return sub_01B8(0x400, 0);
}

/* sceSysreg_driver_866EEB74 */
int sceSysregAtahddResetEnable(void)
{
    return sub_01B8(0x1000, 1);
}

/* sceSysreg_driver_9EB8C49E */
int sceSysregAtahddResetDisable(void)
{
    return sub_01B8(0x1000, 0);
}

/* sceSysreg_driver_4A433DC3 */
int sceSysregUsbhostResetEnable(void)
{
    return sub_01B8(0x2000, 1);
}

/* sceSysreg_driver_9B710D3C */
int sceSysregUsbhostResetDisable(void)
{
    return sub_01B8(0x2000, 0);
}

/* sceSysreg_driver_C6C75585 */
int sceSysreg_driver_C6C75585(int no)
{
    if (no < 0 || no >= 2)
        return 0x80000102;

    return sub_01B8(0x4000 << no, 1);
}

/* sceSysreg_driver_0995F8F6 */
int sceSysreg_driver_0995F8F6(int no)
{
    if (no < 0 || no >= 2)
        return 0x80000102;

    return sub_01B8(0x4000 << no, 0);
}

/* sceSysreg_driver_72887197 */
int sceSysreg_driver_72887197(void)
{
    return sub_01B8(1, 1);
}

/* sceSysreg_driver_32E02FDF */
int sceSysreg_driver_32E02FDF(void)
{
    return sub_01B8(1, 0);
}

/* sceSysreg_driver_73B3E52D */
int sceSysreg_driver_73B3E52D(void)
{
    return (LW(0xBC10004C) >> 16) & 1;
}

int sub_04B8(int flag, int enable)
{
    int oldIntr = sceKernelCpuSuspendIntr();
    int reg = LW(0xBC100050);
    *(int*)(0xBC100050) = enable ? (reg | flag) : (reg ^ (reg & flag));
    sceKernelCpuResumeIntr(intr);
    return ((reg & flag) != 0);
}

/* sceSysreg_driver_38527743 */
int sceSysregMeBusClockEnable(void)
{
    return sub_04B8(1, 1);
}

/* sceSysreg_driver_C4C21CAB */
int sceSysregMeBusClockDisable(void)
{
    return sub_04B8(1, 0);
}

/* sceSysreg_driver_51571E8F */
int sceSysregAwRegABusClockEnable(void)
{
    return sub_04B8(2, 1);
}

/* sceSysreg_driver_52B74976 */
int sceSysregAwRegABusClockDisable(void)
{
    return sub_04B8(2, 0);
}

/* sceSysreg_driver_44277D0D */
int sceSysregAwRegBBusClockEnable(void)
{
    return sub_04B8(4, 1);
}

/* sceSysreg_driver_7E1B1F28 */
int sceSysregAwRegBBusClockDisable(void)
{
    return sub_04B8(4, 0);
}

/* sceSysreg_driver_C2E0E869 */
int sceSysregAwEdramBusClockEnable(void)
{
    return sub_04B8(8, 1);
}

/* sceSysreg_driver_258782A3 */
int sceSysregAwEdramBusClockDisable(void)
{
    return sub_04B8(8, 0);
}

