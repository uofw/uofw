/* Copyright (C) 2011, 2012 The uOFW team
   See the file COPYING for copying permission.
*/

#ifndef USERSYSTEMLIB_KERNEL_H
#define USERSYSTEMLIB_KERNEL_H

#include "common_header.h"

#include <threadman_user.h>

/* Interrupts */

/**
 * Suspends all interrupts.
 *
 * @return Current state of the interrupt controller.
 */
s32 sceKernelCpuSuspendIntr(void);

/**
 * Resumes all interrupts.
 *
 * @param intr A state returned by sceKernelCpuSuspendIntr().
 */
void sceKernelCpuResumeIntr(s32 intr);

/**
 * Resumes all interrupts, with synchronization.
 *
 * @param intr A state returned by sceKernelCpuSuspendIntr().
 */
void sceKernelCpuResumeIntrWithSync(s32 intr);

/**
 * Checks if interrupts are suspended.
 *
 * @param intr A state returned by sceKernelCpuSuspendIntr().
 *
 * @return 1 if the state indicates that interrupts are suspended, otherwise 0. 
 */
s32 sceKernelIsCpuIntrSuspended(s32 intr);

/**
 * Checks if interrupts are enabled.
 *
 * @return 1 if interrupts are enabled, otherwise 0.
 */
s32 sceKernelIsCpuIntrEnable(void);


/* Lazy */

/**
 * Calls sceGeListUpdateStallAddr.
 *
 * This is NOT an exported function because it is only used by sceGe_lazy_31129B95().
 * Its syscall instruction is patched at runtime by the GE module.
 */
s32 sub_00000208(s32 dlId, void *stall);

/**
 * Updates the stall address.
 *
 * Calls sceGeListUpdateStallAddr with additional checks, limiting its call once out of 64 times.
 * This function is only used by 'Genso Suikoden I & II'.
 *
 * @param dlId The ID of the display list which stall address will be modified.
 * @param stall A new stall address.
 *
 * @return SCE_ERROR_OK on success, otherwise <0 on error.
 */
s32 sceGe_lazy_31129B95(s32 dlId, void *stall);


/* Thread */

/**
 * Gets the current thread ID.
 *
 * @return The current thread ID, otherwise <0 on error.
 */
s32 sceKernelGetThreadId(void);

/**
 * Checks how much stack space is left for the current thread.
 *
 * @return The current thread's stack space available, otherwise <0 on error.
 */
s32 sceKernelCheckThreadStack(void);

/**
 * Unknown. FIXME
 *
 * @param arg0 Unknown.
 *
 * @return A pointer, otherwise NULL on error.
 */
void *Kernel_Library_FA835CDE(s32 arg0);


/* Lightweight Mutex */

/**
 * Tries to lock a lightweight mutex.
 *
 * This function is non-blocking.
 * If the mutex is flagged as recursive, count can be >1.
 * For more information, see http://linux.die.net/man/3/pthread_mutex_trylock
 *
 * @param mutex Pointer to a lightweight mutex structure.
 * @param count The lock counter increment.
 *
 * @return SCE_ERROR_OK on success, otherwise SCE_ERROR_KERNEL_LWMUTEX_LOCKED on error.
 */
s32 sceKernelTryLockLwMutex(SceLwMutex *mutex, s32 count);

/**
 * Tries to lock a lightweight mutex (6.xx version).
 *
 * This function is non-blocking. It could return SCE_ERROR_KERNEL_LWMUTEX_LOCKED.
 * Unlike sceKernelTryLockLwMutex(), the error codes are more informative.
 * If the mutex is flagged as recursive, count can be >1.
 * For more information, see http://linux.die.net/man/3/pthread_mutex_trylock
 *
 * @param mutex Pointer to a lightweight mutex structure.
 * @param count The lock counter increment.
 *
 * @return SCE_ERROR_OK on success, otherwise <0 on error.
 */
s32 sceKernelTryLockLwMutex_600(SceLwMutex *mutex, s32 count);

/**
 * Locks a lightweight mutex (callback).
 *
 * This function can be blocking if the mutex is already locked.
 * If the mutex is flagged as recursive, count can be >1.
 * For more information, see http://linux.die.net/man/3/pthread_mutex_lock
 *
 * @param mutex Pointer to a lightweight mutex structure.
 * @param count The lock counter increment.
 *
 * @return SCE_ERROR_OK on success, otherwise <0 on error.
 */
s32 sceKernelLockLwMutexCB(SceLwMutex *mutex, s32 count);

/**
 * Locks a lightweight mutex.
 *
 * This function can be blocking if the mutex is already locked.
 * If the mutex is flagged as recursive, count can be >1.
 * For more information, see http://linux.die.net/man/3/pthread_mutex_lock
 *
 * @param mutex Pointer to a lightweight mutex structure.
 * @param count The lock counter increment.
 *
 * @return SCE_ERROR_OK on success, otherwise <0 on error.
 */
s32 sceKernelLockLwMutex(SceLwMutex *mutex, s32 count);

/**
 * Unlocks a lightweight mutex.
 *
 * This function is non-blocking.
 * If the mutex is flagged as recursive, count can be >1.
 * For more information, see http://linux.die.net/man/3/pthread_mutex_unlock
 *
 * @param mutex Pointer to a lightweight mutex structure.
 * @param count The lock counter decrement.
 *
 * @return SCE_ERROR_OK on success, otherwise <0 on error.
 */
s32 sceKernelUnlockLwMutex(SceLwMutex *mutex, s32 count);

/**
 * Gets the lock count of a lightweight mutex.
 *
 * @param mutex Pointer to a lightweight mutex structure.
 *
 * @return The mutex's lock count, otherwise <0 on error.
 */
s32 Kernel_Library_3AD10D4D(SceLwMutex *mutex);

/**
 * Refers the lightweight mutex's status. FIXME
 *
 * @param mutex Pointer to a lightweight mutex structure.
 * @param addr Unknown.
 *
 * @return Unknown, may be SCE_ERROR_OK on success and <0 on error.
 */
s32 sceKernelReferLwMutexStatus(SceLwMutex *mutex, u32 *addr);

/* Memory */

/**
 * Copies a block of memory.
 *
 * See http://www.cplusplus.com/reference/cstring/memcpy/
 *
 * @param dst Pointer to the memory block where the content will be copied.
 * @param src Pointer to the data source.
 * @param size Number of bytes to copy.
 *
 * @return Value of dst.
 */
void *sceKernelMemcpy(void *dst, const void *src, SceSize size);

/**
 * Fills a block of memory.
 *
 * See http://www.cplusplus.com/reference/cstring/memset/
 *
 * @param dst Pointer to the memory block to fill.
 * @param val Value to be set, casted to the u8 type.
 * @param size Number of bytes to be set.
 *
 * @return Value of dst.
 */
void *sceKernelMemset(void *dst, s32 val, SceSize size);

#endif /* USERSYSTEMLIB_KERNEL_H */
