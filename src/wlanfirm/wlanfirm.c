/* Copyright (C) 2011, 2012, 2013 The uOFW team
   See the file COPYING for copying permission.
*/

#include <common_imp.h>

#define PSP_MODEL_2G

SCE_MODULE_INFO(
#ifdef PSP_MODEL_1G
    "sceWlanFirmMagpie_driver",
#else
    "sceWlanFirmVoyager_driver",
#endif
    SCE_MODULE_KERNEL | SCE_MODULE_ATTR_CANT_STOP |
    SCE_MODULE_ATTR_EXCLUSIVE_LOAD | SCE_MODULE_ATTR_EXCLUSIVE_START,
    1, 2
);
SCE_SDK_VERSION(SDK_VERSION);

#ifdef PSP_MODEL_1G
#include "firmware/magpie.c"
#include "firmware/magpie_helper.c"
#else
#include "firmware/voyager.c"
#include "firmware/voyager_helper.c"
#endif

// TODO proper headers
s32 sceIdStorage_driver_6FE062D1(u16, u32, void*, u32);
void sceWlanDrv_driver_90E5530F(void*, s32, void*, s32);

s32 module_start(SceSize argc __attribute__((unused)), void *argp __attribute__((unused)))
{
    u16 key45;

#ifdef PSP_MODEL_1G
    // sceIdStorageLookup
    sceIdStorage_driver_6FE062D1(0x45, 0, &key45, sizeof(u16));

    if ((key45 & 0xF000) != 0)
    {
        return 1;
    }
#else
    // sceIdStorageLookup
    if (sceIdStorage_driver_6FE062D1(0x45, 0, &key45, sizeof(u16)) < 0) {
        return 1;
    }

    if ((key45 & 0xF000) != 0x1000) {
        return 1;
    }
#endif

    sceWlanDrv_driver_90E5530F(
        g_wlanfirmHelper,
        g_wlanfirmHelperSize,
        g_wlanfirm,
        g_wlanfirmSize
    );

    return 0;
}
