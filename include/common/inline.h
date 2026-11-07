/* Copyright (C) 2011 - 2015 The uOFW team
   See the file COPYING for copying permission.
*/

#ifndef COMMON_INCLUDED
# error "Only include common_imp.h or common_header.h!"
#endif

inline static s32 pspMax(s32 a, s32 b)
{
    s32 ret;
    asm __volatile__ ("max %0, %1, %2" : "=r" (ret) : "r" (a), "r" (b));
    return ret;
}

inline static s32 pspMin(s32 a, s32 b)
{
    s32 ret;
    asm __volatile__ ("min %0, %1, %2" : "=r" (ret) : "r" (a), "r" (b));
    return ret;
}

/*
 * Count the leading zero bits of 'value' (32 for 0, as the 'clz' instruction).
 */
inline static s32 pspClz(u32 value)
{
#if __GNUC__ >= 14
    return __builtin_clzg(value, 32);
#elif __GNUC__ == 3
    return __builtin_allegrex_clz(value);
#else
    return __builtin_clz(value);
#endif
}

inline static void pspSync(void)
{
    asm __volatile__ ("sync");
}

inline static void pspCache(char op, const void *ptr)
{
    asm __volatile__ ("cache %0, 0(%1)" : : "ri" (op), "r" (ptr));
}

/*
 * BREAK instruction
 *
 *  31  26 25                  6 5    0
 * +------+---------------------+------+
 * |000000|     break code      |001101|
 * +------+---------------------+------+
 */

/* break codes */
#define SCE_BREAKCODE_ZERO			0x00000
#define SCE_BREAKCODE_ONE           0x00001
#define SCE_BREAKCODE_DIVZERO		0x00007 /* Divide by zero check. */

#define MAKE_BREAKCODE_INSTR(op)    ((((op) & 0xFFFFF) << 6) | 0xD)

/*
 * Emit 'break' with the 20-bit break code op. The assembler takes the code as two fields
 * (bits 19..10 and 9..0), so codes above 0x3FF are split.
 */
inline static void pspBreak(s32 op)
{
    asm __volatile__ ("break %0,%1" : : "ri" ((op) >> 10), "ri" ((op) & 0x3FF));
}

inline static void pspHalt(void)
{
    /* The 'HALT' instruction */
    asm __volatile__ (".word 0x70000000");
}

inline static s32 pspMfic(void)
{
    s32 ret;
    asm __volatile__ ("mfic %0, $0" : "=r" (ret));
    return ret;
}

/**
 * Set the interrupt enable flag (the 'mtie' instruction, written 'mtic value, $0'): 0 disables the
 * interrupts, the value returned by pspMfic() restores them.
 */
inline static void pspMtic(s32 value)
{
    asm __volatile__ ("mtic %0, $0" : : "r" (value));
}

inline static s32 pspLl(s32 *ptr)
{
    s32 ret;
    asm __volatile__ ("ll %0, (%1)" : "=r" (ret) : "r" (ptr));
    return ret;
}

inline static s32 pspSc(s32 value, s32 *ptr)
{
    s32 ret = value;
    asm __volatile__ ("sc %0, (%1)" : "=r" (ret) : "r" (ptr));
    return ret;
}

inline static u32 pspWsbw(u32 value)
{
    asm __volatile__ ("wsbw %0, %0" : "=r" (value) : "r" (value));
    return value;
}

