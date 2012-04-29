#include "../common/common.h"

// 140F0
SceSysEventHandler *g_sysEvHandlers;

int sceKernelUnregisterSysEventHandler(SceSysEventHandler *handler)
{
    int oldIntr = suspendIntr();
    if (handler->busy != 0)
    {
        // C8A8
        resumeIntr(oldIntr);
        return 0x80020001;
    }
    SceSysEventHandler *cur = g_sysEvHandlers;
    SceSysEventHandler *prev = NULL;
    // C84C
    while (cur != NULL)
    {
        if (cur == handler)
        {
            // C88C
            if (prev == NULL) {
                // C8A0
                g_sysEvHandlers = cur->next;
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
        return 0x80020068;
    return 0;
}

int sceKernelSysEventDispatch(int ev_type_mask, int ev_id, char* ev_name, void* param, int* result, int break_nonzero, SceSysEventHandler **break_handler)
{
    int oldGp = pspGetGp();
    int ret = 0;
    int oldIntr = suspendIntr();
    SceSysEventHandler *cur = g_sysEvHandlers;
    // C928
    while (cur != NULL)
    {
        if ((cur->type_mask & ev_type_mask) != 0)
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

int sceKernelSysEventInit(void)
{
    g_sysEvHandlers = NULL;
    return 0;
}

int sceKernelIsRegisterSysEventHandler(SceSysEventHandler* handler)
{
    int oldIntr = suspendIntr();
    SceSysEventHandler *cur = g_sysEvHandlers;
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

int sceKernelRegisterSysEventHandler(SceSysEventHandler* handler)
{
    int oldIntr1 = suspendIntr();
    int oldIntr2 = suspendIntr();
    SceSysEventHandler *cur = g_sysEvHandlers;
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
        handler->next = g_sysEvHandlers;
        g_sysEvHandlers = handler;
        resumeIntr(oldIntr1);
        return 0;
    }
    resumeIntr(oldIntr1);
    return 0x80020067;
}

SceSysEventHandler *sceKernelReferSysEventHandler(void)
{
    return g_sysEvHandlers;
}

