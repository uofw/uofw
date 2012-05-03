#ifndef DEBUG_H
#define DEBUG_H

typedef enum
{
    FB_NONE,
    FB_BASIC,
    FB_AFTER_DISPLAY
} FbMode;

typedef enum
{
    FAT_NONE,
    FAT_BASIC,
    FAT_AFTER_FATFS
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

#endif

