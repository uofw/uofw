/* Copyright (C) 2011, 2012, 2013 The uOFW team    
   See the file COPYING for copying permission.
*/

#include "usersystemlib_kernel.h"

#include <common_imp.h>
#include <interruptman.h>
#include <sysmem_kernel.h>
#include <threadman_user.h>

SCE_MODULE_INFO("sceKernelLibrary",
    SCE_MODULE_ATTR_CANT_STOP | SCE_MODULE_ATTR_EXCLUSIVE_LOAD |
    SCE_MODULE_ATTR_EXCLUSIVE_START, 1, 6);
SCE_MODULE_START_THREAD_PARAMETER(3, 0x20, 0x400, 0);
SCE_MODULE_BOOTSTART("_UserSystemLibInit");
SCE_SDK_VERSION(SDK_VERSION);

typedef struct {
    u32 unk0[48]; // 0
    s32 id; // 192
    u32 unk2; // 196
    u32 frameSize; // 200
} SceThread;

// .rodata
static const s32 g_userSpaceIntrStackSize = 0x2000; // b68

// .data
static SceGeLazy g_lazy = {
    .dlId = -1, // b80
    .stall = NULL, // b84
    .count = 0, // b88
    .max = 100 // b8c
};

// .bss
static u8 g_userSpaceIntrStack[0x2000]; // bc0
static SceThread *g_thread; // 2bc0
// 2bc4 size 0x3c (padding)
static s32 g_cmdList[0x500]; // 2c00

// module_start
s32 _UserSystemLibInit(SceSize argc __attribute__((unused)), void *argp __attribute__((unused)))
{
    // InterruptManager_EEE43F47
    sceKernelRegisterUserSpaceIntrStack(
        (s32)g_userSpaceIntrStack, // 0xBC0
        g_userSpaceIntrStackSize, // 0x2000
        (s32)&g_thread // 0x2BC0
    );

    // SysMemUserForUser_A6848DF8
    sceKernelSetUsersystemLibWork(
        g_cmdList, // 0x2C00
        sceGe_lazy_31129B95, // 0x140
        &g_lazy // 0xB80
    );

    return SCE_ERROR_OK;
}

s32 sceGe_lazy_31129B95(s32 dlId, void *stall)
{
    s32 ret;

    if (dlId == g_lazy.dlId) {
        g_lazy.stall = stall;

        if (stall != NULL) {
            g_lazy.count++;

            if (g_lazy.count < g_lazy.max) {
                return 0;
            }
        }
    }

    if (g_lazy.dlId >= 0) {
        SceBool idErr = 0;

        do {
            if (pspLl(&g_lazy.dlId) != g_lazy.dlId) {
                idErr = 1;
                break;
            }
        } while (!pspSc(-1, &g_lazy.dlId));

        if (!idErr) {
            sub_00000208(g_lazy.dlId, g_lazy.stall);
        }
    }

    ret = sub_00000208(dlId, stall);

    if (ret < 0) {
        return ret;
    }

    do {
        if (pspLl(&g_lazy.dlId) >= 0) {
            return ret;
        }
    } while (!pspSc(dlId, &g_lazy.dlId));

    g_lazy.stall = stall;
    g_lazy.count = 0;

    return ret;
}

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

    available = pspGetSp() - g_thread->frameSize;

    if (available < 64) {
        // ThreadManForUser_D13BDE95
        return sceKernelCheckThreadStack();
    }

    return available;
}

// Kernel_Library_DC692EE3
s32 sceKernelTryLockLwMutex(SceLwMutex *mutex, s32 count)
{
    // Kernel_Library_37431849
    if (sceKernelTryLockLwMutex_600(mutex, count) != SCE_ERROR_OK) {
        // 0x800201C4
        return SCE_ERROR_KERNEL_MUTEX_LOCKED;
    }

    return SCE_ERROR_OK;
}

// Kernel_Library_37431849
// reference: http://linux.die.net/man/3/pthread_mutex_trylock
s32 sceKernelTryLockLwMutex_600(SceLwMutex *mutex, s32 count)
{
    if (g_thread == NULL) {
        // 0x80020064
        return SCE_ERROR_KERNEL_CANNOT_BE_CALLED_FROM_INTERRUPT;
    }

    if (count <= 0) {
        // 0x800201BD
        return SCE_ERROR_KERNEL_ILLEGAL_COUNT;
    }

    if (mutex->id < 0) {
        // 0x800201CA
        return SCE_ERROR_KERNEL_LWMUTEX_NOT_FOUND;
    }

    if (mutex->thid == g_thread->id) { // loc_00000340
        if (!(mutex->flags & SCE_KERNEL_LWMUTEX_RECURSIVE)) {
            // 0x800201CF
            return SCE_ERROR_KERNEL_LWMUTEX_RECURSIVE_NOT_ALLOWED;
        }

        if (mutex->lockCount + count < 0) {
            // 0x800201CD
            return SCE_ERROR_KERNEL_LWMUTEX_LOCK_OVERFLOW;
        }

        mutex->lockCount += count;

        return SCE_ERROR_OK;
    }

    if (mutex->thid != 0) {
        // 0x800201CB
        return SCE_ERROR_KERNEL_LWMUTEX_LOCKED;
    }

    if (count != 1 && !(mutex->flags & SCE_KERNEL_LWMUTEX_RECURSIVE)) {
        // 0x800201BD
        return SCE_ERROR_KERNEL_ILLEGAL_COUNT;
    }

    do {
        if (pspLl(&mutex->thid) != 0) {
            // 0x800201CB
            return SCE_ERROR_KERNEL_LWMUTEX_LOCKED;
        }
    } while (!pspSc(g_thread->id, &mutex->thid));

    mutex->lockCount = count;

    return SCE_ERROR_OK;
}

// Kernel_Library_1FC64E09
s32 sceKernelLockLwMutexCB(SceLwMutex *mutex, s32 count)
{
    s32 ret;

    if (g_thread == NULL) {
        // 0x80020064
        return SCE_ERROR_KERNEL_CANNOT_BE_CALLED_FROM_INTERRUPT;
    }

    if (!sceKernelIsCpuIntrEnable() || g_thread->unk2 != 0) {
        // 0x800201A7
        return SCE_ERROR_KERNEL_WAIT_CAN_NOT_WAIT;
    }

    ret = sceKernelTryLockLwMutex_600(mutex, count);

    /* mutex already locked, block until it is available */
    if ((u32)ret == SCE_ERROR_KERNEL_LWMUTEX_LOCKED) {
        // ThreadManForUser_31327F19
        return _sceKernelLockLwMutexCB(mutex, count);
    }

    return ret;
}

// Kernel_Library_BEA46419
s32 sceKernelLockLwMutex(SceLwMutex *mutex, s32 count)
{
    s32 ret;

    if (g_thread == NULL) {
        // 0x80020064
        return SCE_ERROR_KERNEL_CANNOT_BE_CALLED_FROM_INTERRUPT;
    }

    if (!sceKernelIsCpuIntrEnable() || g_thread->unk2 != 0) {
        // 0x800201A7
        return SCE_ERROR_KERNEL_WAIT_CAN_NOT_WAIT;
    }

    ret = sceKernelTryLockLwMutex_600(mutex, count);

    /* mutex already locked, block until it is available */
    if ((u32)ret == SCE_ERROR_KERNEL_LWMUTEX_LOCKED) {
        // ThreadManForUser_7CFF8CF3
        return _sceKernelLockLwMutex(mutex, count);
    }

    return ret;
}

// Kernel_Library_15B6446B
// reference: http://linux.die.net/man/3/pthread_mutex_unlock
s32 sceKernelUnlockLwMutex(SceLwMutex *mutex, s32 count)
{
    s32 prevCount;

    if (g_thread == NULL) {
        // 0x80020064
        return SCE_ERROR_KERNEL_CANNOT_BE_CALLED_FROM_INTERRUPT;
    }

    if (count <= 0) {
        // 0x800201BD
        return SCE_ERROR_KERNEL_ILLEGAL_COUNT;
    }

    if (mutex->id < 0) {
        // 0x800201CA
        return SCE_ERROR_KERNEL_LWMUTEX_NOT_FOUND;
    }

    if (count != 1 && !(mutex->flags & SCE_KERNEL_LWMUTEX_RECURSIVE)) {
        // 0x800201BD
        return SCE_ERROR_KERNEL_ILLEGAL_COUNT;
    }

    if (mutex->thid != g_thread->id) {
        return SCE_ERROR_KERNEL_LWMUTEX_UNLOCKED;
    }

    prevCount = mutex->lockCount;

    if (mutex->lockCount - count > 0) {
        mutex->lockCount -= count;
        return SCE_ERROR_OK;
    }

    // loc_00000408

    if (mutex->lockCount - count < 0) {
        return SCE_ERROR_KERNEL_LWMUTEX_UNLOCK_UNDERFLOW;
    }

    mutex->lockCount = 0;

    if (pspLl(&mutex->unk3) != 0) {
        mutex->lockCount = prevCount;
        // ThreadManForUser_BEED3A47
        return _sceKernelUnlockLwMutex(mutex, count);
    }

    if (pspSc(0, &mutex->thid) == 0) {
        mutex->lockCount = prevCount;
        // ThreadManForUser_BEED3A47
        return _sceKernelUnlockLwMutex(mutex, count);
    }

    return SCE_ERROR_OK;
}

// Kernel_Library_3AD10D4D
s32 Kernel_Library_3AD10D4D(SceLwMutex *mutex)
{
    if (g_thread == NULL) {
        // 0x80020064
        return SCE_ERROR_KERNEL_CANNOT_BE_CALLED_FROM_INTERRUPT;
    }

    if (mutex->id < 0) {
        // 0x800201CA
        return SCE_ERROR_KERNEL_LWMUTEX_NOT_FOUND;
    }

    if ((mutex->thid == 0 && g_thread->id != 0) ||
        (mutex->thid != 0 && g_thread->id == 0)) {
        return 0;
    }

    return mutex->lockCount;
}

// Kernel_Library_C1734599
s32 sceKernelReferLwMutexStatus(SceLwMutex *mutex, u32 *addr)
{
    // ThreadManForUser_4C145944
    return sceKernelReferLwMutexStatusByID(mutex->id, addr);
}

// Kernel_Library_FA835CDE
void *Kernel_Library_FA835CDE(s32 arg0)
{
    void *ptr;
    s32 *k0;

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

    if (arg0 < 0) {
        return NULL;
    }

    // range k0[16-31]
    ptr = (void*)k0[((arg0 >> 3) & 0xF) + 16];

    // this is why I think that a pointer is returned
    if (ptr == NULL) { // loc_000004FC
        s32 ret;

        ret = ThreadManForUser_65F54FFB(arg0, &ptr, 0);

        if (ret < 0) {
            return NULL;
        }
    }

    return ptr;
}

// FIXME: naive, not reversed!
void *sceKernelMemcpy(void *dst, const void *src, SceSize size)
{
    u8 *dst8 = (u8*)dst;
    u8 *src8 = (u8*)src;

    while (size--) {
        *(dst8++) = *(src8++);
    }
    
    return dst;
}

// FIXME: naive, not reversed!
void *sceKernelMemset(void *dst, s32 val, SceSize size)
{
    u8 *dst8 = (u8*)dst;

    while (size--) {
        *(dst8++) = (u8)val;
    }

    return dst;
}
