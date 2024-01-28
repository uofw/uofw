#include <common_imp.h>
#include <sysmem_suspend_kernel.h>

#include "intr.h"

typedef struct
{
    s32 (*handler)(s32 unk, void* param);
    int gp;
    void *param;
} SceSuspendHandler;

typedef struct
{
    s32 (*handler)(s32 unk, void* param);
    int gp;
    void *param;
} SceResumeHandler;

// 140F4
int g_iTempPowerLock;
// 140F8
ScePowerHandlers *g_pPowerHandlers;
// 140FC
SceSuspendHandler g_SuspendHandlers[32];
// 1427C
SceResumeHandler g_ResumeHandlers[32];

int sceKernelSuspendInit(void)
{
    int i;
    // CB1C
    for (i = 0; i < 32; i++)
        g_SuspendHandlers[i].handler = NULL;
    // CB38
    for (i = 0; i < 32; i++)
        g_ResumeHandlers[i].handler = NULL;
    return 0;
}

int sceKernelRegisterPowerHandlers(const ScePowerHandlers *handlers)
{
    g_pPowerHandlers = (ScePowerHandlers *)handlers;
    return g_iTempPowerLock;
}

s32 sceKernelPowerLock(s32 unk)
{
    if (g_pPowerHandlers == NULL)
    {
        // CB90
        int oldIntr = suspendIntr();
        g_iTempPowerLock++;
        resumeIntr(oldIntr);
        return 0;
    }
    return g_pPowerHandlers->lock(unk);
}

s32 sceKernelPowerLockForUser(s32 unk)
{
    if (g_pPowerHandlers != NULL)
        return g_pPowerHandlers->lockForUser(unk);
    return 0;
}

s32 sceKernelPowerUnlock(s32 unk)
{
    if (g_pPowerHandlers == NULL)
    {
        // CB90
        int oldIntr = suspendIntr();
        g_iTempPowerLock--;
        resumeIntr(oldIntr);
        return 0;
    }
    return g_pPowerHandlers->unlock(unk);
}

s32 sceKernelPowerUnlockForUser(s32 unk)
{
    if (g_pPowerHandlers != NULL)
        return g_pPowerHandlers->unlockForUser(unk);
    return 0;
}

int sceKernelPowerTick(int unk)
{
    if (g_pPowerHandlers != NULL) {
        // CCA0
        return g_pPowerHandlers->tick(unk);
    }
    return 0;
}

int sceKernelVolatileMemLock(int unk, void **ptr, SceSize *size)
{
    if (g_pPowerHandlers != NULL)
        return g_pPowerHandlers->memLock(unk, ptr, size);
    return -1;
}

int sceKernelVolatileMemTryLock(int unk, void **ptr, SceSize *size)
{
    if (g_pPowerHandlers != NULL)
        return g_pPowerHandlers->memTryLock(unk, ptr, size);
    return -1;
}

int sceKernelVolatileMemUnlock(int unk)
{
    if (g_pPowerHandlers != NULL)
        return g_pPowerHandlers->memUnlock(unk);
    return -1;
}

s32 sceKernelPowerRebootStart(s32 unk)
{
    if (g_pPowerHandlers != NULL)
        return g_pPowerHandlers->rebootStart(unk);
    return 0;
}

s32 sceKernelRegisterSuspendHandler(s32 reg, s32 (*handler)(s32 unk, void *param), void *param)
{
    if (reg < 0 || reg >= 32)
        return -1;
    int oldIntr = suspendIntr();
    SceSuspendHandler *cur = &g_SuspendHandlers[reg];
    cur->handler = handler;
    cur->param = param;
    cur->gp = pspGetGp();
    resumeIntr(oldIntr);
    return 0;
}

s32 sceKernelRegisterResumeHandler(s32 reg, s32 (*handler)(s32, void*), void *param)
{
    if (reg < 0 || reg >= 32)
        return -1;
    int oldIntr = suspendIntr();
    SceResumeHandler *cur = &g_ResumeHandlers[reg];
    cur->handler = handler;
    cur->param = param;
    cur->gp = pspGetGp();
    resumeIntr(oldIntr);
    return 0;
}

int sceKernelDispatchSuspendHandlers(int unk)
{
    int oldGp = pspGetGp();
    SceSuspendHandler *cur = &g_SuspendHandlers[0];
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
    SceResumeHandler *cur = &g_ResumeHandlers[31];
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

