/* Copyright (C) 2011, 2012 The uOFW team
   See the file COPYING for copying permission.
*/

#ifndef COMMON_INCLUDED
# error "Only include common_imp.h or common_header.h!"
#endif

static inline s32 pspMax(s32 a, s32 b)
{
    s32 ret;
    asm("max %0, %1, %2" : "=r" (ret) : "r" (a), "r" (b));
    return ret;
}

static inline s32 pspMin(s32 a, s32 b)
{
    s32 ret;
    asm("min %0, %1, %2" : "=r" (ret) : "r" (a), "r" (b));
    return ret;
}

static inline void pspSync(void)
{
    asm("sync");
}

static inline void pspCache(char op, const void *ptr)
{
    asm("cache %0, 0(%1)" : : "ri" (op), "r" (ptr));
}

static inline void pspBreak(s32 op)
{
    asm("break %0" : : "ri" (op));
}

static inline void pspHalt(void)
{
    /* The 'HALT' instruction */
    asm(".word 0x70000000");
}

static inline s32 pspMfic(void)
{
    s32 ret;
    asm("mfic %0, $0" : "=r" (ret));
    return ret;
}

