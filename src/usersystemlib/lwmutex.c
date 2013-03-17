/* Copyright (C) 2011, 2012, 2013 The uOFW team    
   See the file COPYING for copying permission.
*/

#include "usersystemlib_int.h"

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
