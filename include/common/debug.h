/* Copyright (C) 2011, 2012 The uOFW team
   See the file COPYING for copying permission.
*/

#ifndef COMMON_INCLUDED
# error "Only include common_imp.h or common_header.h!"
#endif

typedef enum
{
    FB_NONE,
    FB_HARDWARE,
    FB_AFTER_DISPLAY
} FbMode;

typedef enum
{
    FAT_NONE,
    FAT_NOINIT,
    FAT_HARDWARE,
    FAT_AFTER_SYSCON,
    FAT_AFTER_FATMS
} FatMode;

#ifdef DEBUG
void dbg_init(int eraseLog, FbMode fbMode, FatMode fatMode);
void dbg_printf(const char *format, ...);
void dbg_puts(const char *str);
#else
static inline void dbg_init()
{
}

static inline void dbg_printf()
{
}

static inline void dbg_puts()
{
}
#endif

/* Fills the framebuffer with the (r, g, b) color */
static inline void dbg_fbfill(u8 r, u8 g, u8 b) {
    u32 i;
    for (i = 0; i < 480 * 272 * 2; i++)
        *(int*)(0x44000000 + i * 4) = r | (g << 8) | (b << 16);
}

