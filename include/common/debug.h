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
//static inline void dbg_fbfill(u8 r, u8 g, u8 b) {
//    u32 i;
//    for (i = 0; i < 480 * 272 * 2; i++)
//        *(int*)(0x44000000 + i * 4) = r | (g << 8) | (b << 16);
//}

/* Fills the framebuffer with the (r, g, b) color */
static inline void dbg_fbfillCount(u32 count, u8 r, u8 g, u8 b) {
    u32 i;
    for (i = 0 + count * 20; i < 0 + count * 20 + 20; i++)
        *(int*)(0x44000000 + i * 4) = r | (g << 8) | (b << 16);

    for (i = 480 + count * 20; i < 480 + count * 20 + 20; i++)
        *(int*)(0x44000000 + i * 4) = r | (g << 8) | (b << 16);
}

/* Fills the framebuffer with the (r, g, b) color */
static inline void dbg_fbfill(u8 r, u8 g, u8 b) {
    u32 i;
    for (i = 0; i < 20; i++)
        *(int*)(0x44000000 + i * 4) = r | (g << 8) | (b << 16);

    for (i = 480; i < 480 + 20; i++)
        *(int*)(0x44000000 + i * 4) = r | (g << 8) | (b << 16);
}

static inline void dbg_fbfill2(u8 r, u8 g, u8 b) {
    u32 i;
    for (i = 20; i < 40 * 1; i++)
        *(int*)(0x44000000 + i * 4) = r | (g << 8) | (b << 16);

    for (i = 480 + 20; i < 480 + 20 + 20; i++)
        *(int*)(0x44000000 + i * 4) = r | (g << 8) | (b << 16);
}

static inline void dbg_fbfill3(u8 r, u8 g, u8 b) {
    u32 i;
    for (i = 40; i < 60 * 1; i++)
        *(int*)(0x44000000 + i * 4) = r | (g << 8) | (b << 16);

    for (i = 480 + 40; i < 480 + 40 + 20; i++)
        *(int*)(0x44000000 + i * 4) = r | (g << 8) | (b << 16);
}

static inline void dbg_fbfill4(u8 r, u8 g, u8 b) {
    u32 i;
    for (i = 60; i < 80 * 1; i++)
        *(int*)(0x44000000 + i * 4) = r | (g << 8) | (b << 16);

    for (i = 480 + 60; i < 480 + 60 + 20; i++)
        *(int*)(0x44000000 + i * 4) = r | (g << 8) | (b << 16);
}

static inline void dbg_fbfill5(u8 r, u8 g, u8 b) {
    u32 i;
    for (i = 80; i < 100 * 1; i++)
        *(int*)(0x44000000 + i * 4) = r | (g << 8) | (b << 16);

    for (i = 480 + 80; i < 480 + 80 + 20; i++)
        *(int*)(0x44000000 + i * 4) = r | (g << 8) | (b << 16);
}

