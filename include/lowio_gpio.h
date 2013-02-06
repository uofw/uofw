/* Copyright (C) 2011, 2012, 2013 The uOFW team
   See the file COPYING for copying permission.
*/

/**
 * GPIO Port masks.
 */
enum SceGpioPortMasks {
    SCE_GPIO_MASK_LED_MS = 0x40,
    SCE_GPIO_MASK_LED_WLAN = 0x80,
    SCE_GPIO_MASK_LED_BT = 0x1000000
};

enum SceGpioPortModes {
    /** Memory Stick GPIO port mode. */
    SCE_GPIO_PORT_MODE_MS = 6,
    /** W-LAN GPIO port mode. */
    SCE_GPIO_PORT_MODE_WLAN = 7,
    /** Bluetooth GPIO port mode. */
    SCE_GPIO_PORT_MODE_BT = 24,
};

s32 sceGpioPortSet(s32);
s32 sceGpioPortClear(s32);
s32 sceGpioGetPortMode(s32);
s32 sceGpioSetPortMode(s32, s32);
s32 sceGpioDisableTimerCapture(s32, s32);
s32 sceGpioSetIntrMode(s32, s32);
s32 sceGpioAcquireIntr(s32);
s32 sceGpioQueryIntr(s32);
s32 sceGpioPortRead(void);

