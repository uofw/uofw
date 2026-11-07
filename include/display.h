/* Copyright (C) The uOFW team
   See the file COPYING for copying permission.
*/

#include "common_header.h"

/** Framebuffer pixel formats. */
enum PspDisplayPixelFormats {
    /** 16-bit RGB 5:6:5. */
    PSP_DISPLAY_PIXEL_FORMAT_565 = 0,
    /** 16-bit RGBA 5:5:5:1. */
    PSP_DISPLAY_PIXEL_FORMAT_5551,
    /* 16-bit RGBA 4:4:4:4. */
    PSP_DISPLAY_PIXEL_FORMAT_4444,
    /* 32-bit RGBA 8:8:8:8. */
    PSP_DISPLAY_PIXEL_FORMAT_8888
};

/**
 * @brief This function determines whether a VBLANK interval is in progress.
 * 
 * @return SCE_TRUE if a VBLANK is active, otherwise SCE_FALSE.
 */
int sceDisplayIsVblank(void);
int sceDisplayWaitVblankStart(void);

int sceDisplay_driver_CE8A328E(void);

int sceDisplayGetCurrentHcount(void);
int sceDisplayGetAccumulatedHcount(void);
int sceDisplayAdjustAccumulatedHcount(int arg0);

