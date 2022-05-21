#include <common_imp.h>
#include <sysmem_sysevent.h>

#include "intr.h"

// 140F0
SceSysEventHandler *g_pHandlers;

s32 sceKernelUnregisterSysEventHandler(SceSysEventHandler *handler)
{
    int oldIntr = suspendIntr();
    if (handler->busy != 0)
    {
        // C8A8
        resumeIntr(oldIntr);
        return SCE_ERROR_KERNEL_ERROR;
    }
    SceSysEventHandler *cur = g_pHandlers;
    SceSysEventHandler *prev = NULL;
    // C84C
    while (cur != NULL)
    {
        if (cur == handler)
        {
            // C88C
            if (prev == NULL) {
                // C8A0
                g_pHandlers = cur->next;
            }
            else
                prev->next = cur->next;
            break;
        }
        prev = cur;
        cur = cur->next;
    }
    // C864
    resumeIntr(oldIntr);
    if (cur == NULL)
        return SCE_ERROR_KERNEL_HANDLER_NOTFOUND;
    return 0;
}

s32 sceKernelSysEventDispatch(s32 ev_type_mask, s32 ev_id, char* ev_name, void* param, s32* result, s32 break_nonzero, SceSysEventHandler **break_handler)
{
    int oldGp = pspGetGp();
    int ret = 0;
    int oldIntr = suspendIntr();
    SceSysEventHandler *cur = g_pHandlers;
    // C928
    while (cur != NULL)
    {
        if ((cur->typeMask & ev_type_mask) != 0)
        {
            // C984
            cur->busy = 1;
            resumeIntr(oldIntr);
            pspSetGp(cur->gp);
            ret = cur->handler(ev_id, ev_name, param, result);
            oldIntr = suspendIntr();
            cur->busy = 0;
            if (ret < 0 && break_nonzero != 0)
            {
                // C9D8
                if (break_handler != NULL)
                    *break_handler = cur;
                break;
            }
            ret = 0;
        }
        // C934
        cur = cur->next;
    }
    // C940
    resumeIntr(oldIntr);
    pspSetGp(oldGp);
    return ret;
}

s32 sceKernelSysEventInit(void)
{
    g_pHandlers = NULL;
    return 0;
}

s32 sceKernelIsRegisterSysEventHandler(SceSysEventHandler* handler)
{
    int oldIntr = suspendIntr();
    SceSysEventHandler *cur = g_pHandlers;
    // CA24
    while (cur != NULL)
    {
        if (cur == handler)
            break;
        cur = cur->next;
    }
    // CA38
    resumeIntr(oldIntr);
    return (cur != NULL);
}

s32 sceKernelRegisterSysEventHandler(SceSysEventHandler* handler)
{
    int oldIntr1 = suspendIntr();
    int oldIntr2 = suspendIntr();
    SceSysEventHandler *cur = g_pHandlers;
    // CA90
    while (cur != NULL)
    {
        if (cur == handler)
            break;
        cur = cur->next;
    }
    // CAA4
    resumeIntr(oldIntr2);
    if (cur == NULL)
    {
        handler->busy = 0;
        // CAE0
        handler->gp = pspGetGp();
        handler->next = g_pHandlers;
        g_pHandlers = handler;
        resumeIntr(oldIntr1);
        return 0;
    }
    resumeIntr(oldIntr1);
    return SCE_ERROR_KERNEL_HANDLER_ALREADY_EXISTS;
}

SceSysEventHandler *sceKernelReferSysEventHandler(void)
{
    return g_pHandlers;
}

