/* Copyright (C) 2011, 2012, 2013 The uOFW team    
   See the file COPYING for copying permission.
*/

#include "usersystemlib_kernel.h"

#include <common_imp.h>
#include <interruptman.h>

SCE_MODULE_INFO("sceKernelLibrary",
    SCE_MODULE_KERNEL | SCE_MODULE_ATTR_CANT_STOP |
    SCE_MODULE_ATTR_EXCLUSIVE_LOAD | SCE_MODULE_ATTR_EXCLUSIVE_START, 1, 1);
SCE_MODULE_BOOTSTART("_UserSystemLibInit");

// .rodata
static const s32 g_userSpaceIntrStackSize = 0x2000; // b68

// .data
static s32 g_b80 = -1; // b80

// .bss
static u8 g_userSpaceIntrStack[0x2000]; // bc0
static s32 *g_2bc0; // 2bc0
static u8 g_unk_2bc4[0x3C]; // 2bc4
static u8 g_2c00[0x80]; // 2c00
static s32 g_thid; // 2c80
static u8 g_unk_2c84[4]; // 2c84
static void *g_sp; // 2c88

// module_start
s32 _UserSystemLibInit(SceSize argc __attribute__((unused)), void *argp)
{
    // InterruptManager_EEE43F47
    sceKernelRegisterUserSpaceIntrStack(
        (s32)g_userSpaceIntrStack, // 0xBC0
        g_userSpaceIntrStackSize, // 0x2000
        (s32)&g_2bc0 // 0x2BC0
    );

    // SysMemUserForUser_A6848DF8
    sceKernelSetUsersystemLibWork(
        g_2c00, // 0x2C00
        0x140,
        &g_b80 // 0xB80
    );

    return SCE_ERROR_OK;
}

// Kernel_Library_293B45B8
s32 sceKernelGetThreadId(void)
{
    if (g_2bc0 == NULL) { // 0x2BC0
        // 0x80020064
        return SCE_ERROR_KERNEL_CANNOT_BE_CALLED_FROM_INTERRUPT;
    }

    return g_thid; // 0x2BC0 + 192
}

// Kernel_Library_D13BDE95
s32 sceKernelCheckThreadStack(void)
{
    u32 size;

    size = pspGetSp() - g_sp; // 0x2BC0 + 200

    if (g_2bc0 == NULL || size < 64) { // 0x2BC0
        // ThreadManForUser_D13BDE95
        return sceKernelCheckThreadStack();
    }

    return size;
}

// Kernel_Library_DC692EE3
s64 sceKernelTryLockLwMutex(SceLwMutex *mutex, u32 count)
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
s64 sceKernelTryLockLwMutex_600(SceLwMutex *mutex, u32 count)
{
    u32 tmpCount;
    u32 tmpOwner;

    if (g_2bc0 == NULL) {
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

    if (mutex->owner == g_2bc0[48]) { // loc_00000340
        if (!(mutex->flags & SCE_KERNEL_LWMUTEX_RECURSIVE) {
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

    if (mutex->owner != 0) {
        // 0x800201CB
        return (1 << 32) | SCE_ERROR_KERNEL_LWMUTEX_LOCKED;
    }

    if (mutex->flags & SCE_KERNEL_LWMUTEX_RECURSIVE) {
        tmpCount = 0;
    } else {
        tmpCount = count ^ 1;
    }

    if (tmpCount != 0) {
        // 0x800201BD
        return SCE_ERROR_KERNEL_ILLEGAL_COUNT;
    }

    do { // loc_00000320
        /* begin atomic RMW */
        asm __volatile__(
            "ll %0, (%1)"
            : "=r" (tmpOwner)
            : "r" (&mutex->owner)
        );

        if (tmpOwner != 0) {
            // 0x800201CB
            return (1 << 32) | SCE_ERROR_KERNEL_LWMUTEX_LOCKED;
        }

        tmpOwner = g_2bc0[48];

        /* end atomic RMW */
        /* if an atomic update as occured, %0 will be set to 1 */
        asm __volatile__(
            "sc %0, (%1)"
            : "=r" (tmpOwner)
            : "r" (&mutex->owner)
        );
    } while (tmpOwner == 0);

    mutex->lockCount = count;

    return SCE_ERROR_OK;
}

s32 sceKernelLockLwMutexCB(void)
{
    return SCE_ERROR_OK;
}

s32 sceKernelLockLwMutex(void)
{
    return SCE_ERROR_OK;
}

// Kernel_Library_15B6446B
// reference: http://linux.die.net/man/3/pthread_mutex_unlock
s32 sceKernelUnlockLwMutex(SceLwMutex *mutex, u32 count)
{
    u32 tmpCount;
    u32 tmpOwner;

    if (g_2bc0 == NULL) {
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

    if (mutex->flags & SCE_KERNEL_LWMUTEX_RECURSIVE) {
        tmpCount = 0;
    } else {
        tmpCount = count ^ 1;
    }

    if (tmpCount != 0) {
        // 0x800201BD
        return SCE_ERROR_KERNEL_ILLEGAL_COUNT;
    }

    if (mutex->owner != g_2bc0[48]) {
        return SCE_ERROR_KERNEL_LWMUTEX_UNLOCKED;
    }

    tmpCount = mutex->lockCount;

    if (mutex->lockCount - count > 0) {
        mutex->lockCount -= count;
        return SCE_ERROR_OK;
    }

    // loc_00000408

    if (mutex->lockCount - count < 0) {
        return SCE_ERROR_KERNEL_LWMUTEX_UNLOCK_UNDERFLOW;
    }

    mutex->lockCount = 0;

    /* begin atomic RMW */
    asm __volatile__(
        "ll %0, (%1)"
        : "=r" (tmpOwner)
        : "r" (&mutex->unk3)
    );

    if (tmpOwner != 0) {
        mutex->lockCount = tmpCount;
        return ThreadManForUser_BEED3A47(mutex, count);
    }

    tmpOwner = 0;

    /* end atomic RMW */
    /* if an atomic update as occured, %0 will be set to 1 */
    asm __volatile__(
        "sc %0, (%1)"
        : "=r" (tmpOwner)
        : "r" (&mutex->owner)
    );

    if (tmpOwner == 0) {
        mutex->lockCount = tmpCount;
        return ThreadManForUser_BEED3A47(mutex, count);
    }

    return SCE_ERROR_OK;
}

s32 Kernel_Library_3AD10D4D(void)
{
    return SCE_ERROR_OK;
}

s32 sceKernelReferLwMutexStatus(SceMutex *mutex, u32 *addr)
{
    // ThreadManForUser_4C145944
    return sceKernelReferLwMutexStatusByID(mutex->id, addr);
}

s32 Kernel_Library_F1835CDE(void)
{
    return SCE_ERROR_OK;
}

void *sceKernelMemcpy(void *dst, const void *src, u32 size)
{
    return NULL;
}

void *sceKernelMemset(void *dst, s32 val, u32 size)
{
    return NULL;
}
