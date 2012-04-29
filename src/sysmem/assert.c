#include "../common/common.h"

// 143FC
void (*g_assertHandler)(int);

void sceKernelRegisterAssertHandler(void (*func)(int))
{
    g_assertHandler = func;
}

void sceKernelAssert(int test, int lvl)
{
    if (test)
        return;
    // CFD0
    if (sceKernelDipsw(8) == 0)
    {
        // D00C
        Kprintf("assertion ignore (level %d)\n", lvl);
        return;
    }
    void (*assertFunc)(int) = g_assertHandler;
    if (assertFunc == NULL)
    {  
        // CFFC
        Kprintf("There is no assert handler, stop\n");
        for (;;) // D004
            ;
    }
    assertFunc(0);
}

