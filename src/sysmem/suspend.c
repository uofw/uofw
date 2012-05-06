#include "common.h"

typedef struct
{
    int size; // 0
    int (*tick)(int); // 4
    int (*lock)(int); // 8
    int (*unlock)(int); // 12
    int (*lockForUser)(int); // 16
    int (*unlockForUser)(int); // 20
    int (*rebootStart)(int); // 24
    int (*memLock)(int, void**, int*); // 28
    int (*memTryLock)(int, void**, int*); // 32
    int (*memUnlock)(int); // 36
} ScePowerHandlers;

typedef struct
{
    int (*handler)(int unk, void* param);
    int gp;
    void *param;
} SceSuspendHandler;

typedef struct
{
    int (*handler)(int unk, void* param);
    int gp;
    void *param;
} SceResumeHandler;

// 140F4
int g_numPowerLock;
// 140F8
ScePowerHandlers *g_powerHandlers;
// 140FC
SceSuspendHandler g_suspendHandlers[32];
// 1427C
SceResumeHandler g_resumeHandlers[32];

int sceKernelSuspendInit(void)
{
    int i;
    // CB1C
    for (i = 0; i < 32; i++)
        g_suspendHandlers[i].handler = NULL;
    // CB38
    for (i = 0; i < 32; i++)
        g_resumeHandlers[i].handler = NULL;
    return 0;
}

int sceKernelRegisterPowerHandlers(ScePowerHandlers *handlers)
{
    g_powerHandlers = handlers;
    return g_numPowerLock;
}

int sceKernelPowerLock(int unk)
{
    if (g_powerHandlers == NULL)
    {
        // CB90
        int oldIntr = suspendIntr();
        g_numPowerLock++;
        resumeIntr(oldIntr);
        return 0;
    }
    return g_powerHandlers->lock(unk);
}

int sceKernelPowerLockForUser(int unk)
{
    if (g_powerHandlers != NULL)
        return g_powerHandlers->lockForUser(unk);
    return 0;
}

int sceKernelPowerUnlock(int unk)
{
    if (g_powerHandlers == NULL)
    {
        // CB90
        int oldIntr = suspendIntr();
        g_numPowerLock--;
        resumeIntr(oldIntr);
        return 0;
    }
    return g_powerHandlers->unlock(unk);
}

int sceKernelPowerUnlockForUser(int unk)
{
    if (g_powerHandlers != NULL)
        return g_powerHandlers->unlockForUser(unk);
    return 0;
}

int sceKernelPowerTick(int unk)
{
    if (g_powerHandlers != NULL) {
        // CCA0
        return g_powerHandlers->tick(unk);
    }
    return 0;
}

int sceKernelVolatileMemLock(int unk, void **ptr, int *size)
{
    if (g_powerHandlers != NULL)
        return g_powerHandlers->memLock(unk, ptr, size);
    return -1;
}

int sceKernelVolatileMemTryLock(int unk, void **ptr, int *size)
{
    if (g_powerHandlers != NULL)
        return g_powerHandlers->memTryLock(unk, ptr, size);
    return -1;
}

int sceKernelVolatileMemUnlock(int unk)
{
    if (g_powerHandlers != NULL)
        return g_powerHandlers->memUnlock(unk);
    return -1;
}

int sceKernelPowerRebootStart(int unk)
{
    if (g_powerHandlers != NULL)
        return g_powerHandlers->rebootStart(unk);
    return 0;
}

int sceKernelRegisterSuspendHandler(int reg, int (*handler)(int unk, void *param), void *param)
{
    if (reg < 0 || reg >= 32)
        return -1;
    int oldIntr = suspendIntr();
    SceSuspendHandler *cur = &g_suspendHandlers[reg];
    cur->handler = handler;
    cur->param = param;
    cur->gp = pspGetGp();
    resumeIntr(oldIntr);
    return 0;
}

int sceKernelRegisterResumeHandler(int reg, int (*handler)(int, void*), void *param)
{
    if (reg < 0 || reg >= 32)
        return -1;
    int oldIntr = suspendIntr();
    SceResumeHandler *cur = &g_resumeHandlers[reg];
    cur->handler = handler;
    cur->param = param;
    cur->gp = pspGetGp();
    resumeIntr(oldIntr);
    return 0;
}

int sceKernelDispatchSuspendHandlers(int unk)
{
    int oldGp = pspGetGp();
    SceSuspendHandler *cur = &g_suspendHandlers[0];
    int i;
    // CEB0
    for (i = 0; i < 32; i++)
    {
        if (cur->handler != NULL)
        {
            // CEF0
            pspSetGp(cur->gp);
            cur->handler(unk, cur->param);
        }
        // CEC0
        cur++;
    }
    pspSetGp(oldGp);
    return 0;
}

int sceKernelDispatchResumeHandlers(int unk)
{
    int oldGp = pspGetGp();
    SceResumeHandler *cur = &g_resumeHandlers[31];
    // CF40
    int i;
    for (i = 0; i < 32; i++)
    {
        if (cur->handler != NULL)
        {
            // CF80
            pspSetGp(cur->gp);
            cur->handler(unk, cur->param);
        }
        cur--;
    }
    pspSetGp(oldGp);
    return 0;
}

