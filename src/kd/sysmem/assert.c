#include <sysmem_kdebug.h>

// 143FC
void (*assert_handler)(int);

void sceKernelRegisterAssertHandler(void (*func)(int))
{
    assert_handler = func;
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
    void (*assertFunc)(int) = assert_handler;
    if (assertFunc == NULL)
    {
        // CFFC
        Kprintf("There is no assert handler, stop\n");
        for (;;) // D004
            ;
    }
    assertFunc(0);
}

