/* Copyright (C) 2011, 2012 The uOFW team
   See the file COPYING for copying permission.
*/

#include "common.h"

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

int sceDisplayWaitVblankStart(void);

