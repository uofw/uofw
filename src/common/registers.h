static inline int pspGetK1(void)
{
    int ret;
    asm("move %0, $k1" : "=r" (ret));
    return ret;
}

static inline void pspSetK1(int k1)
{
    asm("move $k1, %0" : : "r" (k1));
}

static inline int pspShiftK1(void)
{
    int oldK1 = pspGetK1();
    pspSetK1(oldK1 << 11);
    return oldK1;
}

static inline int pspK1PtrOk(const void *ptr)
{
    return (((int)ptr & pspGetK1()) >= 0);
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
    return ((pspGetK1() >> 31) != 0);
}

static inline int pspGetGp(void)
{
    int gp;
    asm("move %0, $gp" : "=r" (gp));
    return gp;
}

static inline void pspSetGp(int gp)
{
    asm("move $zr, $gp");
    asm("move $gp, %0" : : "r" (gp));
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

