/* Copyright (C) 2011, 2012 The uOFW team
   See the file COPYING for copying permission.
*/

#include <common_imp.h>

#include "excep.h"
#include "exceptions.h"
#include "intr.h"
#include "nmi.h"

#include "exceptionman.h"

typedef struct
{
    SceExceptionHandler *hdlr1[32];
    SceExceptionHandler *hdlr2[32];
    SceExceptionHandler *defaultHdlr;
    SceExceptionHandler *curPool;
    SceExceptionHandler pool[32];
} SceExceptions;

SCE_MODULE_INFO("sceExceptionManager", SCE_MODULE_KERNEL | SCE_MODULE_ATTR_CANT_STOP | SCE_MODULE_ATTR_EXCLUSIVE_LOAD
                                       | SCE_MODULE_ATTR_EXCLUSIVE_START, 1, 3);
SCE_MODULE_BOOTSTART("ExcepManInit");
SCE_SDK_VERSION(SDK_VERSION);

int g_0D40 = 0;
int g_0D44 = 0;

// 0D48 (?)
char g_stack[2104] = {0};

// 1580
char g_stackCtx[192] = {0};

// 1640
SceExceptions ExcepManCB = { { NULL }, { NULL }, NULL, NULL, { { NULL, NULL } }};

int ExcepManInit(s32 argc, void *argp)
{
	(void)argc;
	(void)argp;

    dbg_init(1, FB_HARDWARE, FAT_HARDWARE);
    dbg_printf("-- ExcepManInit()\n");
    int oldIntr = suspendIntr();
    int i;
    for (i = 0; i < 480 * 272 * 2; i++) *(int*)(0x44000000 + i * 4) = 0x000000FF;
    for (i = 0; i < 32; i++)
        ExcepManCB.hdlr2[i] = NULL;
    ExcepManCB.defaultHdlr = NULL;
    Allocexceppool();
    pspCop0CtrlSet(25, 0);
    sub_0000();
    int cpuid = pspCop0StateGet(COP0_STATE_CPUID);
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
    dbg_printf("-- init ended\n");
    resumeIntr(oldIntr);
    return 0;
}

int sceKernelRegisterPriorityExceptionHandler(int exno, int prio, void (*func)())
{
    dbg_printf("sceKernelRegisterPriorityExceptionHandler(ex = %d, prio = %d, %08x)\n", exno, prio, func);
    int oldIntr = suspendIntr();
    if (*(int*)(func) != 0)
    {
        // 06B0
        resumeIntr(oldIntr);
        dbg_printf("(ERROR)\n");
        return 0x80020034;
    }
    if (exno < 0 || exno >= 32)
    {
        // 069C
        resumeIntr(oldIntr);
        dbg_printf("(ERROR)\n");
        return 0x80020032;
    }
    prio &= 0x3;
    SceExceptionHandler *newEx = newExcepCB();
    SceExceptionHandler **lastEx = &ExcepManCB.hdlr2[exno];
    SceExceptionHandler *ex = *lastEx;
    newEx->cb = (void*)(((int)func & 0xFFFFFFFC) | prio);
    // 0640
    while (ex != NULL)
    {
        if (((int)ex->cb & 3) >= prio)
            break;
        lastEx = &ex->next;
        ex = ex->next;
    }
    // (0668)
    newEx->next = ex;
    // 066C
    *lastEx = newEx;
    build_exectbl();
    resumeIntr(oldIntr);
    dbg_printf("(end)\n");
    return 0;
}

int sceKernelRegisterDefaultExceptionHandler(void *func)
{
    dbg_printf("sceKernelRegisterDefaultExceptionHandler(%08x)\n", func);
    int oldIntr = suspendIntr();
    SceExceptionHandler *handler = func;
    if (handler->next != NULL || (func == sub_016C && ExcepManCB.defaultHdlr != NULL)) // 0734
    {
        // 0744
        resumeIntr(oldIntr);
        dbg_printf("(ERROR)\n");
        return 0x80020034;
    }
    // 0700
    handler->next = ExcepManCB.defaultHdlr;
    ExcepManCB.defaultHdlr = func + 8;
    build_exectbl();
    resumeIntr(oldIntr);
    dbg_printf("(end)\n");
    return 0;
}

int sceKernelReleaseExceptionHandler(int exno, void (*func)())
{
    dbg_printf("exec %s\n", __FUNCTION__);
    int oldIntr = suspendIntr();
    if (exno < 0 || exno >= 32)
    {
        // 080C
        resumeIntr(oldIntr);
        dbg_printf("(ERROR)\n");
        return 0x80020032;
    }
    SceExceptionHandler *lastEx = ExcepManCB.hdlr2[exno];
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
            dbg_printf("(end)\n");
            return 0;
        }
        lastEx = ex;
        ex = ex->next;
    }
    // 07C0
    resumeIntr(oldIntr);
    dbg_printf("(ERROR)\n");
    return 0x80020033;
}

int sceKernelReleaseDefaultExceptionHandler(void (*func)())
{
    dbg_printf("exec %s\n", __FUNCTION__);
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
                dbg_printf("(end)\n");
                return 0;
            }
            ex = nextFunc - 8;
            next = ex->next;
        }
    }
    // 0874
    resumeIntr(oldIntr);
    dbg_printf("(ERROR)\n");
    return 0x80020033;
}

void build_exectbl(void)
{
    dbg_printf("exec %s\n", __FUNCTION__);
    // 08C8
    int i;
    for (i = 0; i < 32; i++)
    {
        SceExceptionHandler *hdlr = ExcepManCB.hdlr2[i];
        if (hdlr != NULL)
        {  
    		dbg_printf("== listing handlers for excep %d ==\n", i);
            // 08DC
            while (hdlr->next != NULL) {
                dbg_printf("cb %08x, prio %d\n", (int)hdlr->cb & 0xFFFFFFFC, (int)hdlr->cb & 3);
                *(int*)((int)hdlr->cb & 0xFFFFFFFC) = ((int)hdlr->next->cb & 0xFFFFFFFC) + 8;
                hdlr = hdlr->next;
            }
            dbg_printf("cb %08x, prio %d\n", (int)hdlr->cb & 0xFFFFFFFC, (int)hdlr->cb & 3);
            dbg_printf("== done ==\n");
            // 0908
            *(void**)((int)hdlr->cb & 0xFFFFFFFC) = ExcepManCB.defaultHdlr;
        }
        // 0918
    }
    u32 op = syscallHandler;
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
            pspCache(0x1A, &syscallHandler);
        }
        // 0964
    }
    syscallHandler = op;
}

SceExceptionHandler *newExcepCB(void)
{
    dbg_printf("exec %s\n", __FUNCTION__);
    SceExceptionHandler *excep = ExcepManCB.curPool;
    if (excep != NULL)
        ExcepManCB.curPool = ExcepManCB.curPool->next;
    return excep;
}

void FreeExcepCB(SceExceptionHandler *ex)
{
    dbg_printf("exec %s\n", __FUNCTION__);
    ex->next = ExcepManCB.curPool;
    ExcepManCB.curPool = ex;
}

void Allocexceppool(void)
{
    ExcepManCB.curPool = &ExcepManCB.pool[0];
    dbg_printf("Allocexceppool()\n");
    // 09F4
    int i;
    for (i = 0; i < 31; i++)
        ExcepManCB.pool[i].next = &ExcepManCB.pool[i + 1];
    ExcepManCB.hdlr2[30] = NULL;
}

int sceKernelRegisterExceptionHandler(int exno, void (*func)())
{
    dbg_printf("exec %s\n", __FUNCTION__);
    return sceKernelRegisterPriorityExceptionHandler(exno, 2, func);
}

SceExceptionHandler *sceKernelGetActiveDefaultExceptionHandler(void)
{
    dbg_printf("exec %s\n", __FUNCTION__);
    if ((void*)ExcepManCB.defaultHdlr == sub_016C)
        return NULL;
    return ExcepManCB.defaultHdlr;
}

