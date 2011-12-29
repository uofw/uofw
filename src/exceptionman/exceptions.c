/* Copyright (C) 2011 The uOFW team
   See the file COPYING for copying permission.
*/

#include "../global.h"

#include "excep.h"
#include "intr.h"
#include "nmi.h"

#include "exceptions.h"

typedef struct SceExceptionHandler
{
    struct SceExceptionHandler *next;
    void *cb;
} SceExceptionHandler;

typedef struct
{
    SceExceptionHandler *hdlr1[32];
    SceExceptionHandler *hdlr2[32];
    SceExceptionHandler *defaultHdlr;
    SceExceptionHandler *curPool;
    SceExceptionHandler pool[32];
} SceExceptions;

PSP_MODULE_INFO("sceExceptionManager", PSP_MODULE_NO_STOP | PSP_MODULE_SINGLE_LOAD | PSP_MODULE_SINGLE_START | PSP_MODULE_KERNEL, 1, 3);
PSP_SDK_VERSION(0x06060010);
PSP_MODULE_BOOTSTART("ExcepManInit");

void build_exectbl(void);
SceExceptionHandler *newExcepCB(void);
void FreeExcepCB(SceExceptionHandler *ex);
void Allocexceppool(void);

int g_0D40;
int g_0D44;

// 0D48 (?)
char g_stack[2108];

// 1580
char g_stackCtx[192];

// 1640
SceExceptions ExcepManCB;

int ExcepManInit(void)
{   
    int oldIntr = suspendIntr();
    int i;
    for (i = 0; i < 32; i++)
        ExcepManCB.hdlr2[i] = NULL;
    ExcepManCB.defaultHdlr = NULL;
    Allocexceppool();
    COP0_CTRL_SET(25, 0);
    sub_0000();
    int cpuid;
    COP0_STATE_GET(cpuid, 22);
    int *src, *srcEnd, *dst;
    if (cpuid == 1)
    {
        // 05A4
        src = (int*)&sub_0130;
        dst = (int*)(0xBFC00000 + (int)ExceptionManagerForKernel_79454858 - (int)sub_0120);
        srcEnd = (int*)&sub_0140;
    }
    else
    {
        dst = (int*)(0xBFC00000);
        src = (int*)&sub_0120;
        srcEnd = (int*)&ExceptionManagerForKernel_79454858;
    }
    // 0550
    // 055C
    while (src != srcEnd)
        *(dst++) = *(src++);
    // 0578
    sceKernelRegisterDefaultExceptionHandler(sub_016C);
    NmiManInit();
    resumeIntr(oldIntr);
    return 0;
}

int sceKernelRegisterPriorityExceptionHandler(int exno, int prio, void (*func)())
{
    int oldIntr = suspendIntr();
    if (*(int*)(func) != 0)
    {
        // 06B0
        resumeIntr(oldIntr);
        return 0x80020034;
    }
    if (exno < 0 || exno >= 32)
    {
        // 069C
        resumeIntr(oldIntr);
        return 0x80020032;
    }
    prio &= 0x3;
    SceExceptionHandler *newEx = newExcepCB();
    SceExceptionHandler *ex = ExcepManCB.hdlr1[exno];
    SceExceptionHandler *lastEx = ex;
    newEx->cb = (void*)(((int)func & 0xFFFFFFFC) | prio);
    // 0640
    while (ex != NULL)
    {
        if (((int)ex->cb & 3) >= prio)
            break;
        lastEx = ex;
        ex = ex->next;
    }
    // (0668)
    newEx->next = ex;
    // 066C
    lastEx->next = newEx;
    build_exectbl();
    resumeIntr(oldIntr);
    return 0;
}

int sceKernelRegisterDefaultExceptionHandler(void *func)
{
    int oldIntr = suspendIntr();
    SceExceptionHandler *handler = func;
    if (handler->next != NULL || (func == sub_016C && ExcepManCB.defaultHdlr != NULL)) // 0734
    {
        // 0744
        resumeIntr(oldIntr);
        return 0x80020034;
    }
    // 0700
    handler->next = ExcepManCB.defaultHdlr;
    ExcepManCB.defaultHdlr = func + 8;
    build_exectbl();
    resumeIntr(oldIntr);
    return 0;
}

int sceKernelReleaseExceptionHandler(int exno, void (*func)())
{
    int oldIntr = suspendIntr();
    if (exno < 0 || exno >= 32)
    {
        // 080C
        resumeIntr(oldIntr);
        return 0x80020032;
    }
    SceExceptionHandler *lastEx = ExcepManCB.hdlr1[exno];
    SceExceptionHandler *ex = lastEx->next;
    // 07A0
    while (ex != NULL)
    {
        if ((void*)((int)ex->cb & 0xFFFFFFFC) == func)
        {  
            // 07E8
            *(int*)(ex->cb) = 0;
            lastEx->next = ex->next;
            FreeExcepCB(ex);
            build_exectbl();
            resumeIntr(oldIntr);
            return 0;
        }
        lastEx = ex;
        ex = ex->next;
    }
    // 07C0
    resumeIntr(oldIntr);
    return 0x80020033;
}

int sceKernelReleaseDefaultExceptionHandler(void (*func)())
{
    int oldIntr = suspendIntr();
    void (*actualFunc)() = func + 8;
    SceExceptionHandler *releaseEx = (SceExceptionHandler*)func;
    SceExceptionHandler *ex = (SceExceptionHandler*)func;
    if (func != sub_016C)
    {
        SceExceptionHandler *next = ExcepManCB.defaultHdlr->next;
        // 085C
        while (next != NULL)
        {
            void (*nextFunc) = ex->next;
            if (nextFunc == actualFunc)
            {
                // 0898
                ex->next = releaseEx->next;
                releaseEx->next = 0;
                build_exectbl();
                resumeIntr(oldIntr);
                return 0;
            }
            ex = nextFunc - 8;
            next = ex->next;
        }
    }
    // 0874
    resumeIntr(oldIntr);
    return 0x80020033;
}

void build_exectbl(void)
{
    // 08C8
    int i;
    for (i = 0; i < 32; i++)
    {
        SceExceptionHandler *hdlr = ExcepManCB.hdlr2[i];
        if (hdlr != NULL)
        {  
            // 08DC
            while (hdlr->next != NULL) {
                *(int*)((int)hdlr->cb & 0xFFFFFFFC) = ((int)hdlr->next->cb & 0xFFFFFFFC) + 8;
                hdlr = hdlr->next;
            }
            // 0908
            *(void**)((int)hdlr->cb & 0xFFFFFFFC) = ExcepManCB.defaultHdlr;
        }
        // 0918
    }
    int op = *(int*)(sub_0068);
    // 0948
    for (i = 0; i < 32; i++)
    {
        SceExceptionHandler *hdlr2 = ExcepManCB.hdlr2[i];
        if (hdlr2 != NULL) {
            // 0994
            hdlr2 = (void*)(((int)hdlr2->cb & 0xFFFFFFFC) + 8);
        }
        else
            hdlr2 = ExcepManCB.defaultHdlr;

        // 0958
        ExcepManCB.hdlr1[i] = hdlr2;
        if (i == 8)
        {
            // 097C
            op = (((int)ExcepManCB.hdlr1[8] >> 2) & 0x03FFFFFF) + 0x08000000;
            CACHE(0x1A, &sub_0068);
        }
        // 0964
    }
    *(int*)(sub_0068) = op;
}

SceExceptionHandler *newExcepCB(void)
{
    SceExceptionHandler *excep = ExcepManCB.curPool;
    if (excep != NULL)
        ExcepManCB.curPool = ExcepManCB.curPool->next;
    return excep;
}

void FreeExcepCB(SceExceptionHandler *ex)
{
    ex->next = ExcepManCB.curPool;
    ExcepManCB.curPool = ex;
}

void Allocexceppool(void)
{
    ExcepManCB.curPool = &ExcepManCB.pool[0];
    // 09F4
    int i;
    for (i = 0; i < 31; i++)
        ExcepManCB.pool[i].next = &ExcepManCB.pool[i + 1];
    ExcepManCB.hdlr2[30] = NULL;
}

int sceKernelRegisterExceptionHandler(int exno, void (*func)())
{
    return sceKernelRegisterPriorityExceptionHandler(exno, 2, func);
}

SceExceptionHandler *sceKernelGetActiveDefaultExceptionHandler(void)
{
    if ((void*)ExcepManCB.defaultHdlr == sub_016C)
        return NULL;
    return ExcepManCB.defaultHdlr;
}

