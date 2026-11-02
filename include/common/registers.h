/* Copyright (C) 2011, 2012 The uOFW team
   See the file COPYING for copying permission.
*/

#ifndef COMMON_INCLUDED
# error "Only include common_imp.h or common_header.h!"
#endif

static inline int pspGetK0(void)
{
    int ret;
    asm("move %0, $k0" : "=r" (ret));
    return ret;
}

/*
 * $k1 is a global register variable rather than accessed through volatile inline assembly,
 * so the compiler is free to schedule the $k1 moves (e.g. into branch delay slots), as in
 * the original binaries. Stores to it are never removed.
 */
register int g_k1 __asm__("$27");

#define pspGetK1() (g_k1)

static inline void pspSetK1(int k1)
{
    g_k1 = k1;
}

static inline int pspShiftK1(void)
{
    int oldK1 = pspGetK1();
    pspSetK1(oldK1 << 11);
    return oldK1;
}

static inline int pspK1PtrOk(const void *ptr)
{
    int k1 = pspGetK1();

    return ((k1 & (int)ptr) >= 0);
}

/* Used for buffer with "dynamic" size (specified in the functions' arguments) */
static inline int pspK1DynBufOk(const void *ptr, int size)
{
    return (((((int)ptr + size) | (int)ptr | size) & pspGetK1()) >= 0);
}

/* Used for buffer with "static" size (specified in the function using $k1, like the size of a structure) */
static inline int pspK1StaBufOk(const void *ptr, int size)
{
    return (((((int)ptr + size) | (int)ptr) & pspGetK1()) >= 0);
}

static inline int pspK1IsUserMode(void)
{
    return ((u32)pspGetK1() >> 31);
}

static inline int pspGetGp(void)
{
    int gp;
    asm("move %0, $gp" : "=r" (gp));
    return gp;
}

static inline int pspSetGp(int gp)
{
    int oldGp;
    asm("move %0, $gp" : "=r" (oldGp));
    asm("move $gp, %0" : : "r" (gp));
    return oldGp;
}

static inline int pspGetSp(void)
{
    int sp;
    asm("move %0, $sp" : "=r" (sp));
    return sp;
}

static inline void pspSetSp(int sp)
{
    asm("move $sp, %0" : : "r" (sp));
}

static inline int pspGetRa(void)
{
    int ra;
    asm("move %0, $ra" : "=r" (ra));
    return ra;
}

