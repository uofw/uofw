/* Copyright (C) 2011, 2012, 2013 The uOFW team    
   See the file COPYING for copying permission.
*/

#include "usersystemlib_int.h"
#include <threadman_user.h>

// Kernel_Library_293B45B8
s32 sceKernelGetThreadId(void)
{
    if (g_thread == NULL) {
        // 0x80020064
        return SCE_ERROR_KERNEL_CANNOT_BE_CALLED_FROM_INTERRUPT;
    }

    return g_thread->id;
}

// Kernel_Library_D13BDE95
s32 _sceKernelCheckThreadStack(void)
{
    s32 available;

    if (g_thread == NULL) {
        // ThreadManForUser_D13BDE95
        return sceKernelCheckThreadStack();
    }

    available = pspGetSp() - g_thread->stackBottom;

    if (available < 64) {
        // ThreadManForUser_D13BDE95
        return sceKernelCheckThreadStack();
    }

    return available;
}

// Kernel_Library_FA835CDE
void *sceKernelGetTlsAddr(SceUID uid)
{
    void *addr;
    s32 *k0;
    s32 res;

    // SceThread* ?
    k0 = (s32*)pspGetK0();

    if (k0 == NULL) {
        return NULL;
    }

    if (!sceKernelIsCpuIntrEnable()) {
        return NULL;
    }

    // SceThread.unk2 ?
    if (k0[49] != 0) {
        return NULL;
    }

    if (uid < 0) {
        return NULL;
    }

    // range k0[16-31]
    addr = (void*)k0[((uid >> 3) & 0xF) + 16];

    if (addr == NULL) { // loc_000004FC
        res = _sceKernelAllocateTlspl(uid, &addr, 0);

        if (res < 0) {
            return NULL;
        }
    }

    return addr;
}
