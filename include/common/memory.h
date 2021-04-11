/* Copyright (C) 2011, 2012, 2013 The uOFW team
   See the file COPYING for copying permission.
*/

#ifndef COMMON_INCLUDED
# error "Only include common_imp.h or common_header.h!"
#endif

/* Segment base addresses and sizes */
#define KU0_BASE                        0x00000000  /* cached - user/supervisor/kernel */
#define KU1_BASE                        0x40000000  /* uncached - user/supervisor/kernel */

#define K0_BASE                         0x80000000  /* cached - kernel */
#define K0_SIZE                         0x20000000  /* 512 MB */
#define K1_BASE                         0xA0000000  /* uncached - kernel */
#define K1_SIZE                         0x20000000  /* 512 MB */
#define K2_BASE                         0xC0000000  /* cached - supervisor/kernel */
#define K2_SIZE                         0x20000000  /* 512 MB */
#define K3_BASE                         0xE0000000  /* cached - kernel */
#define K3_SIZE                         0x20000000  /* 512 MB */

/* Scratchpad segment base address and size */
#define SCE_SCRATCHPAD_ADDR		0x00010000  /* Physical memory */
#define SCE_SCRATCHPAD_ADDR_KU0         0x00010000  /* KU segment 0 (cached) */
#define SCE_SCRATCHPAD_ADDR_KU1         0x40010000  /* KU segment 1 (uncached) */
#define SCE_SCRATCHPAD_ADDR_K0          0x80010000  /* K0 segment (cached) */
#define SCE_SCRATCHPAD_SIZE		0x00004000  /* 16 KB */

#define REBOOT_BASE_ADDR_K0             0x88600000  /* K0 segment (cached) */

/* Userspace memory base address and size */
#define SCE_USERSPACE_ADDR_KU0          0x08800000  /* KU segment 0 (cached) */
#define SCE_USERSPACE_ADDR_KU1          0x48800000  /* KU segment 1 (uncached) */
#define SCE_USERSPACE_ADDR_K0           0x88800000  /* K0 segment (cached) */
#define SCE_USERSPACE_ADDR_K1           0xA8800000  /* K1 segment (uncached) */
#define SCE_USERSPACE_SIZE              0x01800000  /* 24 MB */

#define SCE_USERSPACE_GAME_ADDR_K0      0x88900000  /* K0 segment (chached) */

#define UCACHED(ptr)    (void *)((u32)(void *)(ptr) & 0x1FFFFFFF)                /* KU0 - cached. */
#define KCACHED(ptr)    (void *)(K0_BASE | ((u32)(void *)(ptr) & 0x1FFFFFFF))    /* K0 - cached */
#define KUNCACHED(ptr)  (void *)(K1_BASE | ((u32)(void *)(ptr) & 0x1FFFFFFF))    /* K1 - uncached */
#define UUNCACHED(ptr)  (void *)(KU1_BASE | ((u32)(void *)(ptr) & 0x1FFFFFFF))   /* KU1 - uncached */

/* Alignment */
#define UPALIGN256(v)   (((v) + 0xFF) & 0xFFFFFF00)
#define UPALIGN64(v)    (((v) + 0x3F) & 0xFFFFFFC0)
#define UPALIGN16(v)    (((v) + 0xF) & 0xFFFFFFF0)
#define UPALIGN8(v)     (((v) + 0x7) & 0xFFFFFFF8)
#define UPALIGN4(v)     (((v) + 0x3) & 0xFFFFFFFC)

/* Clear memory partitioned in 1-Byte blocks. */
static inline void pspClearMemory8(void *ptr, int size) {
    int i;
    for (i = 0; i < size; i++)
         ((u8 *)ptr)[i] = 0;
}

/* Clear memory partitioned in 2-Byte blocks. */
static inline void pspClearMemory16(void *ptr, int size) {
    int i;
    for (i = 0; i < (int)(size / sizeof(u16)); i++)
         ((u16 *)ptr)[i] = 0;
}

/* Clear memory partitioned in 4-Byte blocks. */
static inline void pspClearMemory32(void *ptr, int size) {
    int i;
    for (i = 0; i < (int)(size / sizeof(u32)); i++)
         ((u32 *)ptr)[i] = 0;
}

// TODO: Remove size handling in above's clear  functions.
//       Replace instances of above functions with this one.
static inline void pspClearMemory(void *ptr, int size) {
    if (size % 4 == 0)
        pspClearMemory32(ptr, size / 4);
    else if (size % 2 == 0)
        pspClearMemory16(ptr, size / 2);
    else
        pspClearMemory8(ptr, size);
}

/* If we believe in the sysmem NIDs, 04g+ seem to have a "L2" cache
 * we can send commands to through this address */
#define L2_CACHE_CMD (vu32*)0xA7F00000

static inline void pspL2CacheWriteback0(void *ptr, u8 align) {
    *L2_CACHE_CMD = 0xA0000000 | ((u32)ptr & 0x07FFFFC0) | align;
    *L2_CACHE_CMD;
}

static inline void pspL2CacheWriteback1(void *ptr, u8 align) {
    *L2_CACHE_CMD = 0xA0000000 | 0x08000000 | ((u32)ptr & 0x07FFFFC0) | align;
    *L2_CACHE_CMD;
}

static inline void pspL2CacheWriteback10(void *ptr, u8 align) {
    *L2_CACHE_CMD = 0xA0000000 | 0x08000000 | ((u32)ptr & 0x07FFFFC0) | align;
    *L2_CACHE_CMD = 0xA0000000 | ((u32)ptr & 0x07FFFFC0) | align;
    *L2_CACHE_CMD;
}

