static inline int pspMax(int a, int b)
{
    int ret;
    asm("max %0, %1, %2" : "=r" (ret) : "r" (a), "r" (b));
    return ret;
}

static inline int pspMin(int a, int b)
{
    int ret;
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

static inline void pspBreak(int op)
{
    asm("break %0" : : "ri" (op));
}

