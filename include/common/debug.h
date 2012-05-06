/* Copyright (C) 2011, 2012 The uOFW team
   See the file COPYING for copying permission.
*/

#ifndef COMMON_H
# error "Only include common.h!"
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
    FAT_HARDWARE,
    FAT_AFTER_SYSCON,
    FAT_AFTER_FATMS
} FatMode;

#ifdef DEBUG
void dbg_init(int eraseLog, FbMode fbMode, FatMode fatMode);
void dbg_printf(const char *format, ...);
#else
static inline void dbg_init()
{
}

static inline void dbg_printf()
{
}
#endif

