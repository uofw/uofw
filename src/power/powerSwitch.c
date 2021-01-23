/* Copyright (C) The uOFW team
   See the file COPYING for copying permission.
*/

#include <common_imp.h>
#include <ctrl.h>
#include <display.h>
#include <interruptman.h>
#include <lowio_ddr.h>
#include <lowio_sysreg.h>
#include <modulemgr_init.h>
#include <power_kernel.h>
#include <syscon.h>
#include <sysmem_kdebug.h>
#include <sysmem_kernel.h>
#include <sysmem_suspend_kernel.h>
#include <sysmem_sysclib.h>
#include <sysmem_sysevent.h>
#include <sysmem_utils_kernel.h>
#include <threadman_kernel.h>

#include "power_int.h"

#define POWER_SWITCH_WORKER_THREAD_INIT_PRIO                4

#define POWER_SWITCH_EVENT_POWER_SWITCH_ACTIVE              0x00000001 /* Indicates that the POWER switch is currently active (i.e. pressed by the user). */
#define POWER_SWITCH_EVENT_POWER_SWITCH_INACTIVE            0x00000002 /* Indicates that the POWER switch is currently inactive (i.e. not pressed by the user). */
#define POWER_SWITCH_EVENT_REQUEST_STANDBY                  0x00000010 /* Indicates a standby operation has been requested. */
#define POWER_SWITCH_EVENT_REQUEST_SUSPEND                  0x00000020 /* Indicates a suspend operation has been requested. */
#define POWER_SWITCH_EVENT_REQUEST_SUSPEND_TOUCH_AND_GO     0x00000040 /* Indicates a suspend-touch-and-go operation has been requested. */
#define POWER_SWITCH_EVENT_REQUEST_COLD_RESET               0x00000080 /* Indicates a cold-reset operation has been requested. */
#define POWER_SWITCH_EVENT_100                              0x00000100 /* TODO */
#define POWER_SWITCH_EVENT_POWER_SWITCH_UNLOCKED            0x00010000 /* Indicates that there are currently no power switch locks in place. */
#define POWER_SWITCH_EVENT_OPERATION_RUNNING                0x00020000 /* Indicates that the power manage is currently running a power switch operation. */
#define POWER_SWITCH_EVENT_IDLE                             0x00040000 /* Indicates that the power switch manager is not currently running a power switch oepration. */
#define POWER_SWITCH_EVENT_PROCESSING_TERMINATION           0x80000000 /* Indicates that the power switch manager is shutting down. */

/*
 * Specifies the duration the user has to hold the POWER switch
 * to generate a standby request. Currently two seconds and represented
 * in microseconds.
 */
#define POWER_SWITCH_STANDBY_REQUEST_HOLD_PERIOD            (2 * 1000 * 1000)

/** Defines programmatically generated power switch requests. */
typedef enum
{
    SOFTWARE_POWER_SWITCH_REQUEST_NONE = 0,
    SOFTWARE_POWER_SWITCH_REQUEST_STANDBY,
    SOFTWARE_POWER_SWITCH_REQUEST_SUSPEND,
    SOFTWARE_POWER_SWITCH_REQUEST_SUSPEND_TOUCH_AND_GO,
    SOFTWARE_POWER_SWITCH_REQUEST_COLD_RESET
} ScePowerSoftwarePowerSwitchRequest;

typedef struct 
{
    s32 eventId; //0
    s32 volatileMemorySemaId; //4
    s32 workerThreadId; // 8
    u32 mode; //12
    u32 powerLockForUserShouldForceWait; // 16
    s32 numPowerLocksUser; // 20
    s32 numPowerLocksKernel; // 24
    /* Start address of the PSP's RAM area reserved for volatile memory. */
    u32 volatileMemoryReservedAreaStartAddr; //28
    /* Size of the PSP's RAM area reserved for volatile memory. */
    u32 volatileMemoryReservedAreaSize; // 32
    /* 
     * Indicates whether an application has acquired control over the PSP's RAM area reserved 
     * for volatile memory. 
     */
    u32 isVolatileMemoryReservedAreaInUse; // 36
    ScePowerHardwarePowerSwitchRequest curHardwarePowerSwitchRequest; //40
    ScePowerSoftwarePowerSwitchRequest curSoftwarePowerSwitchRequest; //44
    u32 unk48; //48 TODO: Perhaps a flag indicating whether locking/unlocking is allowed or - more gnerally - standby/suspension/reboot?
    u32 coldResetMode; // 52
    u32 unk56; // 56
    u32 wakeUpCondition; //60
    u32 resumeCount; //64
} ScePowerSwitch; //size: 68

typedef struct
{
    u8 scratchpad[SCE_SCRATCHPAD_SIZE]; // 0 
    u8 hwResetVector[HW_RESET_VECTOR_SIZE]; // 0x4000
    s32 pllOutSelect; // 20480 (0x5000)
    s32 unk20484; // 20484 (0x5004)

    // TODO: Starting here until unk20696 (included) could be a buffer
    // of length 212.

    s32 unk20488; // 20488 (0x5008)
    s32 unk20492; // 20492
    s32 unk20496; // 20496
    s32 unk20504; // 20504
    s32 unk20508; // 20508
    u32 unk20516; // 20516
    // Note: This might not actually be an array but choosing one for convenience here
    // (until more data is available)
    u32 unk20520[44]; // 20520
    u32 unk20696; // 20696
    u32 unk20700; // 20700 (0x50DC)
    // Note: This might not actually be an array but choosing one for convenience here
    // (until more data is available)
    u32 unk20704[8]; // 20704
} ScePowerResume; // Size: 0x5100 - 20736 (TODO: Confirm)

const ScePowerHandlers g_PowerHandler = 
{
    .size = sizeof(ScePowerHandlers),
    .tick = scePowerTick, //0x00002F94
    .lock = scePowerLockForKernel, //0x00001608
    .unlock = scePowerUnlockForKernel, //0x00002C68
    .lockForUser = scePowerLockForUser, //0x00002BF4
    .unlockForUser = scePowerUnlockForUser, //0x00002C24
    .rebootStart = scePowerRebootStart, //0x00002B78
    .memLock = scePowerVolatileMemLock, //0x000012E0
    .memTryLock = scePowerVolatileMemTryLock, //0x00001400
    .memUnlock = scePowerVolatileMemUnlock, //0x00001540
}; //0x00006F48

static s32 _scePowerLock(s32 lockType, s32 isUserLock);
static s32 _scePowerUnlock(s32 lockType, s32 isUserLock);
static s32 _scePowerOffThread(SceSize args, void* argp);
static s32 _scePowerSuspendOperation(s32 mode);
static s32 _scePowerResumePoint(void *pData);
static u32 _scePowerOffCommon(void);
static void _scePowerPowerSwCallback(s32 enable, void* argp);
static void _scePowerHoldSwCallback(s32 enable, void* argp);

ScePowerSwitch g_PowerSwitch; //0x0000729C

ScePowerResume g_Resume; // 0x00007300

//sub_000011A4
/* Initializes the power switch component of the power service. */
s32 _scePowerSwInit(void)
{
    s32 intrState;
    s32 nPowerLock;
    SceSysmemPartitionInfo partitionInfo;

    memset(&g_PowerSwitch, 0, sizeof g_PowerSwitch); // 0x000011C4

    g_PowerSwitch.mode = 2; // 0x000011E8
    g_PowerSwitch.wakeUpCondition = 8; // 0x000011F0

    /* 
     * Initialize the power switch event flag (default: power switch states can happen 
     * and currently no power switch state is being processed).
     */
    g_PowerSwitch.eventId = sceKernelCreateEventFlag("ScePowerSw", SCE_KERNEL_EA_MULTI | 0x1,
        POWER_SWITCH_EVENT_IDLE | POWER_SWITCH_EVENT_POWER_SWITCH_UNLOCKED, NULL); // 0x000011EC & 0x00001210

    /* 
     * Initialize the semaphore lock used to control exclusive access to the RAM area 
     * reserved for volatile memory. We are ready to accept exclusive access requests.
     */
    g_PowerSwitch.volatileMemorySemaId = sceKernelCreateSema("ScePowerVmem", 1, 0, 1, NULL); // 0x0000120C & 0x0000123C

    /* Create and start our worker thread responsible for changing power states. */
    g_PowerSwitch.workerThreadId = sceKernelCreateThread("ScePowerMain", _scePowerOffThread, 
        POWER_SWITCH_WORKER_THREAD_INIT_PRIO, 2 * SCE_KERNEL_1KiB, 
        SCE_KERNEL_TH_NO_FILLSTACK | 0x1, NULL); // 0x00001238 & 0x00001250

    sceKernelStartThread(g_PowerSwitch.workerThreadId, 0, NULL); // 0x0000124C   

    sceSysconSetPowerSwitchCallback(_scePowerPowerSwCallback, NULL); // 0x0000125C
    sceSysconSetHoldSwitchCallback(_scePowerHoldSwCallback, NULL); // 0x0000126C

    partitionInfo.size = sizeof(SceSysmemPartitionInfo); // 0x00001284
    sceKernelQueryMemoryPartitionInfo(SCE_KERNEL_VSHELL_PARTITION, &partitionInfo); // 0x00001280
    g_PowerSwitch.volatileMemoryReservedAreaSize = partitionInfo.memSize; // 0x00001290
    g_PowerSwitch.volatileMemoryReservedAreaStartAddr = partitionInfo.startAddr; // 0x00001298

    intrState = sceKernelCpuSuspendIntr(); // 0x00001294

    scePowerLockForKernel(SCE_KERNEL_POWER_LOCK_DEFAULT); // 0x000012A0

    nPowerLock = sceKernelRegisterPowerHandlers(&g_PowerHandler); // 0x000012AC
    g_PowerSwitch.numPowerLocksKernel += nPowerLock; // 0x000012BC

    sceKernelCpuResumeIntr(intrState); // 0x000012C0

    return SCE_ERROR_OK;
}

//Subroutine scePower_23C31FFE - Address 0x000012E0 - Aliases: scePower_driver_70F42744
s32 scePowerVolatileMemLock(s32 mode, void **ppAddr, SceSize *pSize)
{
    s32 intrState;
    s32 oldK1;
    s32 status;
    u32 startAddr;

    /* We only support SCE_KERNEL_VOLATILE_MEM_DEFAULT for now. */
    if (mode != SCE_KERNEL_VOLATILE_MEM_DEFAULT) // 0x0000130C
    {
        return SCE_ERROR_INVALID_MODE;
    }

    oldK1 = pspShiftK1(); // 0x00001314

    /* 
     * Verify the supplied pointers aren't located in privileged memory when called 
     * by a user application. 
     */
    if (!pspK1PtrOk(ppAddr) || !pspK1PtrOk(pSize)) // 0x00001320 & loc_00001364
    {
        pspSetK1(oldK1);
        return SCE_ERROR_PRIV_REQUIRED;
    }

    /* Block until we can get exclusive control over the RAM area reserved for the volatile memory. */
    status = sceKernelWaitSema(g_PowerSwitch.volatileMemorySemaId, 1, NULL); // 0x00001370

    /* We acquired exclusive access to the memory area. */

    if (ppAddr != NULL) // 0x00001378
    {
        *ppAddr = g_PowerSwitch.volatileMemoryReservedAreaStartAddr; // 0x00001384
    }

    if (pSize != NULL) // 0x00001388
    {
        *pSize = g_PowerSwitch.volatileMemoryReservedAreaSize; // 0x00001394
    }

    intrState = sceKernelCpuSuspendIntr(); // 0x00001398

    /*
     * If the memory area reserved for the volatile memory resides in RAM reserved for the kernel, 
     * we need to change the memory protection for it to make it available to user applications.
     */
    startAddr = g_PowerSwitch.volatileMemoryReservedAreaStartAddr;
    if ((startAddr >= SCE_KERNELSPACE_ADDR_KU0 && startAddr < SCE_USERSPACE_ADDR_KU0)
        || (startAddr >= SCE_KERNELSPACE_ADDR_KU1 && startAddr < SCE_USERSPACE_ADDR_KU1)
        || (startAddr >= SCE_KERNELSPACE_ADDR_K0 && startAddr < SCE_USERSPACE_ADDR_K0)
        || (startAddr >= SCE_KERNELSPACE_ADDR_K1 && startAddr < SCE_USERSPACE_ADDR_K1)) // 0x000013B0 - 0x000013E4
    {
        sceKernelSetDdrMemoryProtection(startAddr, g_PowerSwitch.volatileMemoryReservedAreaSize, 0xF); // 0x000013F0
    }

    g_PowerSwitch.isVolatileMemoryReservedAreaInUse = SCE_TRUE; //0x000013C8

    sceKernelCpuResumeIntrWithSync(intrState); // 0x000013C4

    pspSetK1(oldK1);
    return status;
}

//Subroutine scePower_FA97A599 - Address 0x00001400 - Aliases: scePower_driver_A882AEB7
s32 scePowerVolatileMemTryLock(s32 mode, void **ppAddr, SceSize *pSize)
{
    s32 intrState;
    s32 oldK1;
    s32 status;
    u32 startAddr;

    /* We only support SCE_KERNEL_VOLATILE_MEM_DEFAULT for now. */
    if (mode != SCE_KERNEL_VOLATILE_MEM_DEFAULT) // 0x0000142C
    {
        return SCE_ERROR_INVALID_MODE;
    }      

    oldK1 = pspShiftK1(); // 0x00001434

    /*
     * Verify the supplied pointers aren't located in privileged memory when called
     * by a user application.
     */
    if (!pspK1PtrOk(ppAddr) || !pspK1PtrOk(pSize)) // 0x00001440 & 0x0000144C
    {
        pspSetK1(oldK1);
        return SCE_ERROR_PRIV_REQUIRED;
    }

    /* 
     * Query (without wait) if we can get exclusive access to the RAM area reserved for 
     * the volatile memory. 
     */
    status = sceKernelPollSema(g_PowerSwitch.volatileMemorySemaId, 1); // 0x0000148C
    if (status != SCE_ERROR_OK) // 0x00001494
    {
        /* Exclusive control to the memory area has already been assigned to someone else. */

        pspSetK1(oldK1);
        return (status == SCE_ERROR_KERNEL_SEMA_ZERO) 
            ? SCE_POWER_ERROR_CANNOT_LOCK_VMEM 
            : status; //0x00001524 - 0x0000153C
    }

    /* We acquired exclusive access to the memory area. */

    if (ppAddr != NULL) // 0x0000149C
    {
        *ppAddr = g_PowerSwitch.volatileMemoryReservedAreaStartAddr; // 0x000014A8
    }

    if (pSize != NULL) // 0x000014AC
    {
        *pSize = g_PowerSwitch.volatileMemoryReservedAreaSize; // 0x000014B8
    }     

    intrState = sceKernelCpuSuspendIntr(); //0x000014BC

    /*
     * If the memory area reserved for the volatile memory resides in RAM reserved for the kernel,
     * we need to change the memory protection for it to make it available to user applications.
     */
    startAddr = g_PowerSwitch.volatileMemoryReservedAreaStartAddr;
    if ((startAddr >= SCE_KERNELSPACE_ADDR_KU0 && startAddr < SCE_USERSPACE_ADDR_KU0)
        || (startAddr >= SCE_KERNELSPACE_ADDR_KU1 && startAddr < SCE_USERSPACE_ADDR_KU1)
        || (startAddr >= SCE_KERNELSPACE_ADDR_K0 && startAddr < SCE_USERSPACE_ADDR_K0)
        || (startAddr >= SCE_KERNELSPACE_ADDR_K1 && startAddr < SCE_USERSPACE_ADDR_K1)) // 0x000014CC - 0x000014D8 & 0x000014D8 - 0x00001508
    {
        sceKernelSetDdrMemoryProtection(startAddr, g_PowerSwitch.volatileMemoryReservedAreaSize, 0xF); // 0x00001514
    }

    g_PowerSwitch.isVolatileMemoryReservedAreaInUse = SCE_TRUE; // 0x000014EC

    sceKernelCpuResumeIntrWithSync(intrState); // 0x000014E8

    pspSetK1(oldK1);
    return status;
}

//Subroutine scePower_B3EDD801 - Address 0x00001540 - Aliases: scePower_driver_5978B1C2
s32 scePowerVolatileMemUnlock(s32 mode)
{
    s32 oldK1;
    s32 intrState;
    s32 status;
    u32 startAddr;

    /* We only support SCE_KERNEL_VOLATILE_MEM_DEFAULT for now. */
    if (mode != SCE_KERNEL_VOLATILE_MEM_DEFAULT) // 0x0000156C
    {
        return SCE_ERROR_INVALID_MODE;
    }

    oldK1 = pspShiftK1(); // 0x00001434

    startAddr = g_PowerSwitch.volatileMemoryReservedAreaStartAddr;

    /* Invalidate the cache for the memory area now that we are done using it. */
    sceKernelDcacheWritebackInvalidateRange(startAddr, g_PowerSwitch.volatileMemoryReservedAreaSize); // 0x0000157C

    intrState = sceKernelCpuSuspendIntr(); // 0x00001584

    /*
     * If the memory area reserved for the volatile memory resides in RAM reserved for the kernel,
     * we previously changed its memory protection upon acquiring exclusive control to it. We now 
     * need to restore the original memory protections in place.
     */
    if ((startAddr >= SCE_KERNELSPACE_ADDR_KU0 && startAddr < SCE_USERSPACE_ADDR_KU0)
        || (startAddr >= SCE_KERNELSPACE_ADDR_KU1 && startAddr < SCE_USERSPACE_ADDR_KU1)
        || (startAddr >= SCE_KERNELSPACE_ADDR_K0 && startAddr < SCE_USERSPACE_ADDR_K0)
        || (startAddr >= SCE_KERNELSPACE_ADDR_K1 && startAddr < SCE_USERSPACE_ADDR_K1)) // 0x00001590 - 0x000015AC & 0x000015F0
    {
        sceKernelSetDdrMemoryProtection(startAddr, g_PowerSwitch.volatileMemoryReservedAreaSize, 0); // 0x000015F8
    }

    /* Relinquish exclusive control over the RAM area reversed for volatile memory. */
    status = sceKernelSignalSema(g_PowerSwitch.volatileMemorySemaId, 1); // 0x000015B4

    g_PowerSwitch.isVolatileMemoryReservedAreaInUse = SCE_FALSE; // 0x000015BC

    sceKernelCpuResumeIntrWithSync(intrState); // 0x000014E8

    pspSetK1(oldK1);
    return status;
}

//Subroutine scePower_driver_6CF50928 - Address 0x00001608
s32 scePowerLockForKernel(s32 lockType)
{
    return _scePowerLock(lockType, SCE_FALSE);
}

//Subroutine scePower_D6D016EF - Address 0x00002BF4
s32 scePowerLockForUser(s32 lockType)
{
    s32 oldK1;
    s32 status;

    oldK1 = pspShiftK1();

    status = _scePowerLock(lockType, SCE_TRUE); //0x00002C08

    pspSetK1(oldK1);
    return status;
}

//sub_00001624
static s32 _scePowerLock(s32 lockType, s32 isUserLock)
{
    s32 intrState;

    /* We only support SCE_KERNEL_POWER_LOCK_DEFAULT as the type currently. */
    if (lockType != SCE_KERNEL_POWER_LOCK_DEFAULT) // 0x00001644
    {
        return SCE_ERROR_INVALID_MODE;
    }

    if (g_PowerSwitch.unk48 != 0) // 0x00001658
    {
        return SCE_ERROR_OK;
    }

    intrState = sceKernelCpuSuspendIntr(); // 0x00001680

    /*
     * If no power lock currently exists in the system, we need to update the internal power switch
     * unlocked state.
     */
    if (g_PowerSwitch.numPowerLocksUser == 0 && g_PowerSwitch.numPowerLocksKernel == 0) // 0x0000168C & 0x00001698
    {
        /*
         * Indicate that power switching is now locked. Attempts to suspend/shutdown the system by the user
         * are now blocked until this lock has been cancelled (assuming no other power locks are in place as well).
         */
        sceKernelClearEventFlag(g_PowerSwitch.eventId, ~POWER_SWITCH_EVENT_POWER_SWITCH_UNLOCKED); // 0x0000170C
    }

    if (isUserLock)
    {
        g_PowerSwitch.numPowerLocksUser++; // 0x000016AC - 0x000016BC

        sceKernelCpuResumeIntr(intrState); // 0x000016B8

        /* 
         * No suspend/standby operation currently in progress. Applications can savely proceed with
         * running their power state critical operations.
         */
        if (!g_PowerSwitch.powerLockForUserShouldForceWait) // 0x000016C4
        {
            return SCE_ERROR_OK;
        }

        /*
         * Normally, we return immediately. However, we block the thread when a suspend/standby/reboot process
         * has already been started. We only proceed when the power switch manager is in the Idle state. 
         * This thread block prevents applications from running power-state critical operations which
         * cannot be guaranteed by the system to not be interrupted by power switch events (such as a standby).
         */
        return sceKernelWaitEventFlag(g_PowerSwitch.eventId, POWER_SWITCH_EVENT_IDLE,
            SCE_KERNEL_EW_OR, NULL, NULL); // 0x000016DC
    }
    else
    {
        // loc_000016EC

        g_PowerSwitch.numPowerLocksKernel++; // 0x000016EC - 0x000016FC

        sceKernelCpuResumeIntr(intrState); // 0x000016F8
        return SCE_ERROR_OK;
    }
}

//Subroutine scePower_driver_C3024FE6 - Address 0x00002C68
s32 scePowerUnlockForKernel(s32 lockType)
{
    return _scePowerUnlock(lockType, SCE_FALSE);
}

//Subroutine scePower_CA3D34C1 - Address 0x00002C24
s32 scePowerUnlockForUser(s32 lockType)
{
    s32 oldK1;
    s32 status;

    /* We only support SCE_KERNEL_POWER_LOCK_DEFAULT currently. */
    if (lockType != SCE_KERNEL_POWER_LOCK_DEFAULT) // 0x00002C3C
    {
        return SCE_ERROR_INVALID_MODE;
    }

    oldK1 = pspShiftK1();

    status = _scePowerUnlock(lockType, SCE_TRUE); //0x00002C44

    pspSetK1(oldK1);
    return status;
}

//Subroutine sub_00002E1C - Address 0x00002E1C
static s32 _scePowerUnlock(s32 lockType, s32 isUserLock)
{
    s32 intrState;
    s32 numPowerLocks;

    if (g_PowerSwitch.unk48 != 0) // 0x00002E44
    {
        return SCE_ERROR_OK;
    }

    intrState = sceKernelCpuSuspendIntr(); // 0x00002E6C

    /* Reduce the existing power locks (either user or kernel) by one. */

    if (isUserLock) // 0x00002E74
    {
        numPowerLocks = g_PowerSwitch.numPowerLocksUser;
        if (numPowerLocks > 0) // 0x00002E80
        {
            g_PowerSwitch.numPowerLocksUser = --numPowerLocks; // 0x00002E8C
        }
    }
    else
    {
        numPowerLocks = g_PowerSwitch.numPowerLocksKernel;
        if (numPowerLocks > 0) // 0x00002ECC
        {
            g_PowerSwitch.numPowerLocksKernel = --numPowerLocks; // 0x00002EDC
        }
    }

    // loc_00002E90

    /* Check if we can unlock power switching. */
    if (g_PowerSwitch.numPowerLocksUser == 0 && g_PowerSwitch.numPowerLocksKernel == 0) // 0x00002E94 & 0x00002EA0
    {
        /*
         * No more power locks currently exist in the system, we can now unlock power switching. Any
         * power switching request (like user attempting to suspend the PSP) which was made while the
         * lock was in place will now be processed.
         */
        sceKernelSetEventFlag(g_PowerSwitch.eventId, POWER_SWITCH_EVENT_POWER_SWITCH_UNLOCKED); // 0x00002EB8
    }

    sceKernelCpuResumeIntr(intrState); // 0x00002EA8

    /* Return remaining power locks (>= 0). */
    return numPowerLocks;
}

#define POWER_SWITCH_SUSPEND_OP_STANDBY                     0x100
#define POWER_SWITCH_SUSPEND_OP_SUSPEND                     0x200
#define POWER_SWITCH_SUSPEND_OP_SUSPEND_TOUCH_AND_GO        0x300
#define POWER_SWITCH_SUSPEND_OP_COLD_RESET		            0x400

//0x000017F0
/* Processes power switch requests. */
static s32 _scePowerOffThread(SceSize args, void* argp)
{
    u32 powerSwitchWaitFlags;
    u32 powerSwitchSetFlags; // $sp
    SceUInt waitTimeout; // $sp + 4
    s32 status; // $s0

    (void)args;
    (void)argp;

    for (;;)
    {
        powerSwitchWaitFlags = POWER_SWITCH_EVENT_PROCESSING_TERMINATION | POWER_SWITCH_EVENT_100
            | POWER_SWITCH_EVENT_REQUEST_STANDBY | POWER_SWITCH_EVENT_REQUEST_SUSPEND
            | POWER_SWITCH_EVENT_REQUEST_SUSPEND_TOUCH_AND_GO | POWER_SWITCH_EVENT_REQUEST_COLD_RESET
            | POWER_SWITCH_EVENT_POWER_SWITCH_ACTIVE;

        sceKernelWaitEventFlag(g_PowerSwitch.eventId, powerSwitchWaitFlags, SCE_KERNEL_EW_OR,
            &powerSwitchSetFlags, NULL); // 0x00001844

        if (powerSwitchSetFlags & POWER_SWITCH_EVENT_PROCESSING_TERMINATION) // 0x00001850
        {
            return SCE_ERROR_OK;
        }

        if (powerSwitchSetFlags & POWER_SWITCH_EVENT_POWER_SWITCH_ACTIVE) // 0x00001858
        {
            // loc_000019E8

            /*
             * The POWER switch is currently held by the user. If the user holds it for less than two seconds
             * we let the PSP system enter [suspend] state. If, however, the user holds the POWER switch
             * for at least two seconds, we then let the PSP system enter [standby] mode.
             *
             * Below, we are waiting for at most two seconds to determine if we need to enter either the [suspend]
             * or the [standby] power state.
             */

            powerSwitchWaitFlags = POWER_SWITCH_EVENT_PROCESSING_TERMINATION | POWER_SWITCH_EVENT_100
                | POWER_SWITCH_EVENT_REQUEST_STANDBY | POWER_SWITCH_EVENT_REQUEST_SUSPEND
                | POWER_SWITCH_EVENT_REQUEST_SUSPEND_TOUCH_AND_GO | POWER_SWITCH_EVENT_REQUEST_COLD_RESET
                | POWER_SWITCH_EVENT_POWER_SWITCH_INACTIVE;
            waitTimeout = POWER_SWITCH_STANDBY_REQUEST_HOLD_PERIOD; /* 2 seconds timeout */
            status = sceKernelWaitEventFlag(g_PowerSwitch.eventId, powerSwitchWaitFlags, SCE_KERNEL_EW_OR,
                &powerSwitchSetFlags, &waitTimeout); // 0x00001A04

            if (powerSwitchSetFlags & POWER_SWITCH_EVENT_PROCESSING_TERMINATION) // 0x00001A10
            {
                return SCE_ERROR_OK;
            }

            if (g_PowerSwitch.mode & 0x1) // 0x00001A20
            {
                if (powerSwitchSetFlags & POWER_SWITCH_EVENT_POWER_SWITCH_INACTIVE) // 0x00001A34
                {
                    _scePowerNotifyCallback(0, 0, SCE_POWER_CALLBACKARG_POWER_SWITCH_SUSPEND_REQUESTED);
                }
                else
                {
                    _scePowerNotifyCallback(0, 0, SCE_POWER_CALLBACKARG_POWER_SWITCH_STANDBY_REQUESTED); // 0x00001A3C - 0x00001A48
                }
            }

            // loc_00001A58

            if (g_PowerSwitch.mode & 0x2) // 0x00001A60
            {
                /* Check with power state we need to transition to (suspend or standby). */

                if (powerSwitchSetFlags & POWER_SWITCH_EVENT_POWER_SWITCH_INACTIVE) // 0x00001A6C
                {
                    /*
                     * The user held the POWER switch for less than two seconds thus we now enter the
                     * [suspend] power state.
                     */
                    g_PowerSwitch.curHardwarePowerSwitchRequest = HARDWARE_POWER_SWITCH_REQUEST_SUSPEND; // 0x00001A74
                }
                else if (status == SCE_ERROR_KERNEL_WAIT_TIMEOUT) // 0x00001A84
                {
                    /*
                     * The user kept holding the POWER switch for at least two seconds so we now enter the
                     * [standby] power state.
                     */
                    g_PowerSwitch.curHardwarePowerSwitchRequest = HARDWARE_POWER_SWITCH_REQUEST_STANDBY; // 0x00001A88
                }
            }
        }

        // loc_00001864

        if (powerSwitchSetFlags & POWER_SWITCH_EVENT_REQUEST_STANDBY) // 0x00001864
        {
            g_PowerSwitch.curSoftwarePowerSwitchRequest = SOFTWARE_POWER_SWITCH_REQUEST_STANDBY; // 0x0000186C
        }
        else if (powerSwitchSetFlags & POWER_SWITCH_EVENT_REQUEST_SUSPEND) // 0x00001868 & 0x00001998
        {
            g_PowerSwitch.curSoftwarePowerSwitchRequest = SOFTWARE_POWER_SWITCH_REQUEST_SUSPEND; // 0x000019A0
        }
        else if (powerSwitchSetFlags & POWER_SWITCH_EVENT_REQUEST_SUSPEND_TOUCH_AND_GO) // 0x0000199C & 0x000019AC
        {
            g_PowerSwitch.curSoftwarePowerSwitchRequest = SOFTWARE_POWER_SWITCH_REQUEST_SUSPEND_TOUCH_AND_GO;
        }
        else if (powerSwitchSetFlags & POWER_SWITCH_EVENT_REQUEST_COLD_RESET) // 0x000019B0 & 0x000019C0
        {
            g_PowerSwitch.curSoftwarePowerSwitchRequest = SOFTWARE_POWER_SWITCH_REQUEST_COLD_RESET; // 0x000019C8
        }
        else if (g_PowerSwitch.curHardwarePowerSwitchRequest == HARDWARE_POWER_SWITCH_REQUEST_NONE) // 0x000019D8
        {
            /*
             * There is currently neither an automatically nor manually requested power switch event which needs
             * to be processed. Let's wait again until a power switch event is requested.
             */
            continue;
        }

        // 0x00001874

        /* Process the requested power switch. */

        /*
         * We only proceed with processing the requested power switch when there is no power lock placed
         * in the system. If power locks (one or more) exists, we wait until all of them have been released.
         * We make no distinction between automatically generated power switch requests or manually generated
         * ones. Both types of requests have to wait.
         */
        powerSwitchWaitFlags = POWER_SWITCH_EVENT_PROCESSING_TERMINATION | POWER_SWITCH_EVENT_POWER_SWITCH_UNLOCKED;
        sceKernelWaitEventFlag(g_PowerSwitch.eventId, powerSwitchWaitFlags, SCE_KERNEL_EW_OR,
            &powerSwitchSetFlags, NULL); // 0x00001880

        if (powerSwitchSetFlags & POWER_SWITCH_EVENT_PROCESSING_TERMINATION) // 0x00001890
        {
            return SCE_ERROR_OK;
        }

        if (powerSwitchSetFlags & POWER_SWITCH_EVENT_REQUEST_COLD_RESET) // loc_000018A4
        {
            g_PowerSwitch.curSoftwarePowerSwitchRequest = SOFTWARE_POWER_SWITCH_REQUEST_COLD_RESET; // 0x000018A0
        }

        /*
         * Indicate that the power switch manager is no longer idle (waiting for requests) and
         * is now running a power switch operation.
         */
        sceKernelClearEventFlag(g_PowerSwitch.eventId, ~POWER_SWITCH_EVENT_IDLE); // 0x000018A8
        sceKernelSetEventFlag(g_PowerSwitch.eventId, POWER_SWITCH_EVENT_OPERATION_RUNNING); // 0x000018B8

        if (g_PowerSwitch.unk48 == 0) // 0x000018C4
        {
            if (g_PowerSwitch.curHardwarePowerSwitchRequest == HARDWARE_POWER_SWITCH_REQUEST_STANDBY
                || g_PowerSwitch.curSoftwarePowerSwitchRequest == SOFTWARE_POWER_SWITCH_REQUEST_STANDBY) // 0x000018D4 & 0x000018E0
            {
                _scePowerSuspendOperation(POWER_SWITCH_SUSPEND_OP_STANDBY | 0x1);
            }
            else if (g_PowerSwitch.curHardwarePowerSwitchRequest == HARDWARE_POWER_SWITCH_REQUEST_SUSPEND
                || g_PowerSwitch.curSoftwarePowerSwitchRequest == SOFTWARE_POWER_SWITCH_REQUEST_SUSPEND) // 0x000018F0 & 0x000018FC
            {
                _scePowerSuspendOperation(POWER_SWITCH_SUSPEND_OP_SUSPEND | 0x2); // 0x000018F4 & 0x00001950

                g_PowerSwitch.curHardwarePowerSwitchRequest = HARDWARE_POWER_SWITCH_REQUEST_NONE; // 0x00001958
                g_PowerSwitch.curSoftwarePowerSwitchRequest = SOFTWARE_POWER_SWITCH_REQUEST_NONE; // 0x0000195C
            }
            else if (g_PowerSwitch.curSoftwarePowerSwitchRequest == SOFTWARE_POWER_SWITCH_REQUEST_SUSPEND_TOUCH_AND_GO) // 0x00001908
            {
                _scePowerSuspendOperation(POWER_SWITCH_SUSPEND_OP_SUSPEND_TOUCH_AND_GO | 0x3); // 0x0000190C & 0x00001950

                g_PowerSwitch.curHardwarePowerSwitchRequest = HARDWARE_POWER_SWITCH_REQUEST_NONE; // 0x00001958
                g_PowerSwitch.curSoftwarePowerSwitchRequest = SOFTWARE_POWER_SWITCH_REQUEST_NONE; // 0x0000195C
            }
            else if (g_PowerSwitch.curSoftwarePowerSwitchRequest == SOFTWARE_POWER_SWITCH_REQUEST_COLD_RESET) // 0x00001914
            {
                _scePowerSuspendOperation(POWER_SWITCH_SUSPEND_OP_COLD_RESET | 0x4); // 0x00001918 & 0x00001940
            }
        }

        /*
         * Indicate that the power switch manager has finished running a power switch operation and
         * is now idle again (that is, waiting for a new power switch request to arrive).
         */
        sceKernelClearEventFlag(g_PowerSwitch.eventId, ~POWER_SWITCH_EVENT_OPERATION_RUNNING); // 0x000018A8
        sceKernelSetEventFlag(g_PowerSwitch.eventId, POWER_SWITCH_EVENT_IDLE); // 0x000018B8
    }
}

/* Delay the suspension thread for 10 milliseconds at the beginning of each suspend query/attempt. */
#define POWER_SUSPEND_OPERATION_DELAY_TIME                              10000     

/* 
 * If during the resume operation system components report they are busy resuming themselves 
 * we wait the sepcified amount of time (in microseconds) until we re-check if all system components
 * have completed resuming themselves.
 */
#define POWER_RESUME_OPERATION_DELAY_TIME                               5000     
/* Print an error message for each 5 seconds a suspend operation in process is blocked from progression. */
#define POWER_SUSPEND_OPERATION_BLOCKED_TRIES_PRINT_ERROR_THRESHOLD     500

//sub_00001AE8
static s32 _scePowerSuspendOperation(s32 mode)
{
    SceSysEventSuspendPayload sysEventSuspendPayload; // $sp
    SceSysEventResumePayload sysEventResumePlayload; // $sp + 64 
    SceSysEventSuspendPayloadResumData sysEventSuspendPayloadResumeData; // $sp + 128

    u32 wakeUpCondition; // sp + 304
    u32 suspendQuerySysEventResult; // sp + 308
    SceSysEventHandler *pSuspendQuerySysEventHandler; // sp + 312
    s32 pspClock; // sp + 316
    u32 suspendPhase1SysEventResult; // sp + 320
    SceSysEventHandler *pSuspendPhase1SysEventHandler; // sp + 324

    SceUID busyPowerCallbackId; // sp + 328
    
    u32 unk332; // sp + 332

    s32 resumePhase1SysEventResult; // sp + 336
    SceSysEventHandler*pResumePhase1SysEventHandler; // sp + 340

    u32 suspendMode; // sp + 344;
    u32 powerLedStatusForFlashing; // sp + 384 -- Seems to track if power LED should be turned on or off

    u32 ledOffTiming; // $s3

    s32 intrState; // $s4
    s32 status;

    suspendMode = mode; // 0x00001AEC

    memset(&sysEventSuspendPayload, 0, sizeof sysEventSuspendPayload); // 0x00001B24
    memset(&sysEventResumePlayload, 0, sizeof sysEventResumePlayload); // 0x00001B38 - 0x00001B70
    memset(&sysEventSuspendPayloadResumeData, 0, sizeof sysEventSuspendPayloadResumeData); // 0x00001B6C

    sysEventSuspendPayload.size = sizeof sysEventSuspendPayload; // 0x00001B88
    sysEventResumePlayload.size = sizeof sysEventResumePlayload; //0x00001B8C
    sysEventSuspendPayloadResumeData.size = sizeof sysEventSuspendPayloadResumeData; // 0x00001B90

    sysEventSuspendPayloadResumeData.compiledSdkVersion = SDK_VERSION; // 0x00001B94
    sysEventSuspendPayloadResumeData.pSuspendedGameInfo = sceKernelGetGameInfo(); // 0x00001B94 & 0x00001BA4
    sysEventSuspendPayloadResumeData.pInitParamSfo = sceKernelInitParamSfo(&sysEventSuspendPayloadResumeData.paramSfoSize); // 0x00001BA0 & 0x00001BC4

    sysEventSuspendPayload.pWakeUpCondition = &wakeUpCondition; // 0x00001BBC
    sysEventSuspendPayload.pResumeData = &sysEventSuspendPayloadResumeData; // 0x00001BC0

    wakeUpCondition = g_PowerSwitch.wakeUpCondition; // 0x00001BCC

    ledOffTiming = scePowerGetLedOffTiming(); // 0x00001BC8 & 0x00001BD0
    if (ledOffTiming == 0) // 0x00001BE8 & 0x00001BD8
    {
        ledOffTiming = 3; // TODO: [3] seems to specify LED should flash during suspend op
    }

    // TODO: Seems to determine the power LED flash rate. Larger values
    // mean longer blink intervals.
    u32 ledFlashInterval = 1; // 0x00001B04 -- $s5
    if (suspendMode & 0xF >= 4) // 0x00001BE4
    {
        // loc_000027DC

        if ((suspendMode & 0xF) == 4) // 0x000027E0 -- Cold reboot (0x404)
        {
            sysEventSuspendPayload.isStandbyOrRebootRequested = SCE_TRUE;
            ledOffTiming = 2; // 0x000027E8 --TODO: [2] seems to indicate power LED is to remain ON for the entire operation
        }
    }
    else if ((suspendMode & 0xF) < 2) // 0x00001BF0
    {
        // loc_000027C8

        if ((suspendMode & 0xF) == 0x1) // 0x000027C8 -- standby (0x101)
        {
            sysEventSuspendPayload.isStandbyOrRebootRequested = SCE_TRUE; // 0x000027D0
            ledFlashInterval = 40; // 0x000027D8
        }
    }
    else
    {
        // 0x00001BF8 -- suspend and suspend-touch-and-go (0x202 & 0x303)

        sysEventSuspendPayload.isStandbyOrRebootRequested = SCE_FALSE; // 0x00001BF8
        ledFlashInterval = 15; // 0x00001BFC
    }

    // loc_00001C00

    intrState = sceKernelCpuSuspendIntr(); // 0x00001C04

    pSuspendQuerySysEventHandler = NULL; // 0x00001C2C
    suspendQuerySysEventResult = 0; // 0x00001C34

    /* 
     * Query the system if the PSP device can be suspended. Systen components can ask for blocking 
     * the power switch operation. 
     */
    status = sceKernelSysEventDispatch(SCE_SUSPEND_EVENTS, SCE_SYSTEM_SUSPEND_EVENT_QUERY, "query",
        &sysEventSuspendPayload, &suspendQuerySysEventResult, SCE_TRUE, &pSuspendQuerySysEventHandler); // 0x00001C30
    if (status < SCE_ERROR_OK) // 0x00001C38
    {
        // loc_00002798

        /* 
         * The PSP device cannot be currently suspended as a system component (which received the above query) 
         * has indicated that it is not yet ready to be suspended. Notify the system that the power switch request
         * has been denied.
         */
        sceKernelSysEventDispatch(SCE_SUSPEND_EVENTS, SCE_SYSTEM_SUSPEND_EVENT_QUERY_DENIED, "query", &sysEventSuspendPayload, NULL, SCE_FALSE, NULL); // 0x000027B0

        sceKernelCpuResumeIntr(intrState); // 0x000027B8
        return status;
    }

    /* The requested PSP device suspension has been greenlit. Notify the system that we will now proceed. */

    status = sceKernelSysEventDispatch(SCE_SUSPEND_EVENTS, SCE_SYSTEM_SUSPEND_EVENT_QUERY_GREENLIT, "query",
        &sysEventSuspendPayload, NULL, SCE_FALSE, NULL); // 0x00001C58

    sceKernelCpuResumeIntr(intrState); // 0x00001C64

    /* Raise [phase 2] suspend operation events. */

    sceKernelSysEventDispatch(SCE_SUSPEND_EVENTS, SCE_SYSTEM_SUSPEND_EVENT_PHASE2_16, "phase2", 
        &sysEventSuspendPayload, NULL, SCE_FALSE, NULL);  // 0x00001C84

    /* 
     * Notify the power-state-change listeners of the impending [suspend] or [standby] operation. 
     * An application, which currently has exclusive access rights to the reserved main memory area
     * for volatile memory, should use these callbacks to quickly release their volatile memory lock.
     * Otherwise the suspend operation in progress won't be able to proceed.
     */

    if (sysEventSuspendPayload.isStandbyOrRebootRequested) // 0x00001C98
    {
        _scePowerNotifyCallback(0, SCE_POWER_CALLBACKARG_STANDINGBY, 0); // 0x00001CA8
    }
    else
    {
        _scePowerNotifyCallback(0, SCE_POWER_CALLBACKARG_SUSPENDING, 0); // 0x00001CA0 & 0x00001CA8
    }

    /* 
     * The power switch [suspend] / [standby] process is is already running and and can no longer be prevented
     * by user mode applications setting a power lock. Yet applications might still call powerLock() APIs while
     * the suspend/standby operation is already running, To prevent them from starting critical operations
     * requiring no power state interruption which cannot be guaranteed by the system to run uninterrupted
     * we block such applications from proceeding with these operations.
     */
    g_PowerSwitch.powerLockForUserShouldForceWait = SCE_TRUE; // 0x00001CBC

    // 0x00001CC0 - 0x00001CE8
    /* Send phase 2 suspend events 0x20F - 0x200 to syscon. */
    s32 suspendEventPhase2Event = SCE_SYSTEM_SUSPEND_EVENT_PHASE2_15;
    do 
    {
        sceKernelSysEventDispatch(SCE_SUSPEND_EVENTS, suspendEventPhase2Event, "phase2", 
            &sysEventSuspendPayload, NULL, SCE_FALSE, NULL);  // 0x00001CE0
    } 
    while (--suspendEventPhase2Event >= SCE_SYSTEM_SUSPEND_EVENT_PHASE2_0);

    if (ledOffTiming == 1) // 0x00001CF4 -- [1] seems to indicate power led is turned OFF the entire suspend op
    {
        // loc_00002788

        /* Turn off the power LED. */
        sceSysconCtrlLED(PSP_SYSCON_LED_POWER, PSP_SYSCON_LED_POWER_STATE_OFF); // 0x00002788
    }

    s32 forceSuspendCapacity = scePowerGetForceSuspendCapacity() & 0xFFFF; // 0x00001CFC

    _scePowerChangeSuspendCap(forceSuspendCapacity); // 0x00001D08

    sceSysconReadClock(&pspClock); // 0x00001D10

    intrState = sceKernelCpuSuspendIntr(); // 0x00001D1C

    /* Send suspend operation [phase 1] event to the system. */

    sceKernelSysEventDispatch(SCE_SUSPEND_EVENTS, SCE_SYSTEM_SUSPEND_EVENT_PHASE1_0, "phase1", &sysEventSuspendPayload, NULL, SCE_FALSE, NULL); // 0x00001D40

    powerLedStatusForFlashing = PSP_SYSCON_LED_POWER_STATE_OFF; // 0x00001D48

    s32 suspendBlockedPrintErrorCounter = 0;

    // TODO: This should be a counter for the number of suspend query options needed
    // until we can proceed with the power switch suspend operation.
    s32 unks0 = 0; // $s0

    s32 watchdog; // $s1 in if (unks0 >= 0) conditional block below
    for (;;)
    {
        pSuspendPhase1SysEventHandler = NULL; // 0x00001D64
        suspendPhase1SysEventResult = 0; // 0x00001D6C

        sceKernelCpuResumeIntr(intrState); // 0x00001D68

        /*
         * Delay the current suspend operation to give applications & system components (modules)
         * the chance to release their suspension blocks or finish their own suspend operations.
         */
        sceKernelDelayThread(POWER_SUSPEND_OPERATION_DELAY_TIME); // 0x00001D70

        if (unks0 >= 0) // 0x00001D78
        {
            unks0++; // 0x00001D84

            // In the ASM, this return value is stored in both $s1 and $v0 and used with both
            // registers in the following code.
            watchdog = scePowerGetWatchDog(); // 0x00001D80 & 0x00001D88
            if (watchdog != 0) // 0x00001D8C
            {
                // The ASM here is most likely a compiler optimization ($s6 = 0x51EB851F-- set in 0x00001D00 & 0x00001D4C)
                //
                // for ((unks0 * $s6) >> 32) to become at least 32 (so that the division result is > 0)
                // unks0 needs to be at least 0x64 (100)
                // from there on, increasing unks0 by a 100 gives us the next increase in tmpA3 by one each time
                //
                // Example:
                // unks0 [0, 100) -> tmpA3 = 0
                // unks0 [100, 200) -> tmpA3 = 1
                // unks0 [200, 300) -> tmpA3 = 2
                // ...
                // 
                // as a result, what we really do here is dividing unks0 by 100.
                //
                // I am leaving the directly translated AMS here for reference:
                //  s32 tmpA3 = ((unks0 * 0x51EB851F) >> 32) >> 5 (shift right is sra - a division by 32);
                //
                s32 tmpA3 = unks0 / 100; // 0x00001D94 & 0x00001D98

                // if unks0 <= 0x7FFFFFFF then (unks0 >> 31) always 0
                // it is only -1 if unks0 is at least 0x80000000 (or < 0).
                //
                // Thus [tmpA1] is effectively [unks0 / 100]. (For unks0 to be large enough to be 
                // >= 0x80000000, we would need (2^31) loop runs, where each loop waits 10 ms.
                // That's highly unlikely to happen.
                s32 tmpA1 = tmpA3 - (s32)(unks0 >> 31); // 0x00001DA0

                if (watchdog < tmpA1) // 0x00001DA8 -- v0 is [watchdog] here from 0x00001D80
                {
                    goto loc_00001DD8;
                }

                // In the ASM, we test if (watchdog ($v0) != 0), yet we only reach this code
                // if watchdog != 0 already...we can directly put a goto here with the branch
                // destination.
                goto loc_00001E08; // 0x00001DB0
            }

            // loc_00001DB8

            // Compiler optimization same as above, we actually divide by a 100 here...
            s32 tmpT3 = unks0 / 100; // 0x00001DB8 & 0x00001DC0

            // Same as earlier: if unks0 <= 0x7FFFFFFF then (unks0 >> 31) always 0
            // it is only -1 if unks0 is at least 0x80000000 (or < 0).
            s32 tmpT2 = tmpT3 - (s32)(unks0 >> 31); // 0x00001DC4
            if (tmpT2 < 21)
            {
                goto loc_00001E08;
            }

        loc_00001DD8:

            if (ledOffTiming == 3) // 0x00001DD8
            {
                // loc_00002778

                /* Turn on Power LED. */
                sceSysconCtrlLED(PSP_SYSCON_LED_POWER, PSP_SYSCON_LED_POWER_STATE_ON); // 0x00002778
            }

            if (watchdog != 0) // 0x00001DE0
            {
                sceSysconCtrlTachyonWDT(5); // 0x00002768
            }

            sceSysconSetAffirmativeRertyMode(1); // 0x00001DE8

            sceCtrlSetPollingMode(SCE_CTRL_POLL_INACTIVE); // 0x00001DF0

            ledOffTiming = 2; // 0x00001DFC

            sceSysconNop(); // 0x00001DF8

            unks0 = -1; // 0x00001E00
        }

    loc_00001E08:

        /*
         * We let the PSP Power LED blink as long as the suspend operation is in process. This process might have to wait
         * for applications to release their registered volatile memory lock or for the system to finish prcessing
         * generated suspend/standby requests.
         */

        if (ledOffTiming == 3) // 0x00001E08 -- $v0 is always 3 here
        {
            if (ledFlashInterval == 0) // 0x00002734
            {
                pspBreak(SCE_BREAKCODE_DIVZERO); // 0x00002738
            }

            s32 tmpT6 = unks0 % ledFlashInterval; // 0x00001E0C & 0x0000273C
            if (tmpT6 == 0) // 0x00002740
            {
                /* Invert the PSP power LED power state (so the user sees the Power LED blinking). */
                s32 nTurnOnLed = powerLedStatusForFlashing ^ PSP_SYSCON_LED_POWER_STATE_ON; // 0x00002748
                sceSysconCtrlLED(PSP_SYSCON_LED_POWER, nTurnOnLed); // 0x00002758
                powerLedStatusForFlashing = nTurnOnLed; // 0x0000275C
            }
        }

        // loc_00001E14

        sceKernelSysEventDispatch(SCE_SUSPEND_EVENTS, SCE_SYSTEM_SUSPEND_EVENT_PHASE1_1, "phase1", &sysEventSuspendPayload, NULL, SCE_FALSE, NULL); // 0x00001E28

        intrState = sceKernelCpuSuspendIntr(); // 0x00001E54

        /*
         * Each system component (modules) might have its own individiual suspend operation.
         * As part of the PSP's suspend process, the power service waits until all system
         * components have indicated they have completed their own suspend operation. Only then does the
         * power service proceed with suspending the system. 
         * 
         * Below we send a [phase1] suspend event to the system which a system component can receive
         * and use to report back to the power service that it has not yet completed its suspend operation.
         */
        status = sceKernelSysEventDispatch(SCE_SUSPEND_EVENTS, SCE_SYSTEM_SUSPEND_EVENT_PHASE1_2, "phase1", &sysEventSuspendPayload,
            &suspendPhase1SysEventResult, SCE_TRUE, &pSuspendPhase1SysEventHandler); // 0x00001E54

        /*
         * Check if a system component has indicated that it is currently busy suspending itself or if
         * other suspension block reasons exist (such as kernel power switch locks or volatile memory locks).
         * 
         * If that it the case we will wait for a bit and then query again if previously existing
         * suspension blocks have been removed while we were waiting.
         */
        if (status < SCE_ERROR_OK) // 0x00001E5C
        {
            // loc_00002700

            /*
             * A system compontent is asking to block the continuation of the suspend operation until
             * it is done cleaning up/suspending.
             */
            if (++suspendBlockedPrintErrorCounter == POWER_SUSPEND_OPERATION_BLOCKED_TRIES_PRINT_ERROR_THRESHOLD) // 00002700 & 0x00002704
            {
                suspendBlockedPrintErrorCounter = 0;
                if (pSuspendPhase1SysEventHandler != NULL) // 0x0000270C
                {
                    Kprintf("%s Busy: 0x%08X, 0x%08X, %08X\n", pSuspendPhase1SysEventHandler->name, status,
                        pSuspendPhase1SysEventHandler, suspendPhase1SysEventResult); // 0x00002724
                }
            }
        }
        else if (g_PowerSwitch.numPowerLocksKernel != 0) // 0x00001E64 - 0x00001E70
        {
            /*
             * We cannot proceed with the suspend operation as long as a power switch lock set up by the system
             * is in place.
             */

            if (++suspendBlockedPrintErrorCounter == POWER_SUSPEND_OPERATION_BLOCKED_TRIES_PRINT_ERROR_THRESHOLD) // 0x00001E78 & 0x00001E7C
            {
                suspendBlockedPrintErrorCounter = 0; // 0x00001E90
                Kprintf("Kernel Power Lock\n"); // 0x00001E8C
            }
        }
        else if (g_PowerSwitch.isVolatileMemoryReservedAreaInUse) // 0x00001EA0
        {
            /*
             * We cannot proceed with the suspend operation as long as an application has the reserved memory area
             * for the volatile memory in use.
             */
            if (++suspendBlockedPrintErrorCounter == POWER_SUSPEND_OPERATION_BLOCKED_TRIES_PRINT_ERROR_THRESHOLD) // 0x00001EAC
            {
                suspendBlockedPrintErrorCounter = 0; // 0x00001E90
                Kprintf("Volatile Memory Lock\n"); // 0x00001E8C
            }
        }
        else if (_scePowerIsCallbackBusy(SCE_POWER_CALLBACKARG_STANDINGBY | SCE_POWER_CALLBACKARG_SUSPENDING, &busyPowerCallbackId)) // 0x00001EC0
        {
            /*
             * Not all previously generated shutodwn/suspend callbacks have yet been processed. Block continuation until
             * every consumer of such callbacks had the chance to handle them.
             */
            if (++suspendBlockedPrintErrorCounter == POWER_SUSPEND_OPERATION_BLOCKED_TRIES_PRINT_ERROR_THRESHOLD) // 0x00001ED0 & 0x00001ED4
            {
                suspendBlockedPrintErrorCounter = 0;
                Kprintf("Power Callback Busy: 0x%08X\n", busyPowerCallbackId); // 0x00001ED8 & 0x000026F0
            }
        }
        else
        {
            /* No blocking condition was met. We can proceed with our suspend operation. */

            // In the ASM, we go back to loc_00001D64 if $s1 < 0. This is only the case if the above 
            // sceKernelSysEventDispatch() call returned an error or _scePowerIsCallbackBusy() returned 
            // a value != 0. Otherwise, if no condition is met, $s1 is the return value of the
            // sceKernelSysEventDispatch() call which is >= 0 then.
            break; // 0x00001EE0
        }
    }

    // 0x00001EE8

    // TODO: uofw: In previous firmwares sceDisplayGetCurrentHcount () was used here.
    // Looking at its ASM, this also seems to return a value related to the HSYNC count value.
    s32 unkHcount = sceDisplay_driver_CE8A328E(); // 0x00001EE8 -- $s5

    s32 accumulatedHcount = sceDisplayGetAccumulatedHcount(); // 0x00001EF0 -- $s6

    s64 systemTime = sceKernelGetSystemTimeWide(); // 0x00001EFC
    sysEventSuspendPayloadResumeData.systemTimePreSuspendOp = systemTime; // 0x00001F1C & 0x00001F20
    sysEventSuspendPayload.systemTimePreSuspendOp = systemTime; // 0x00001F20 - 0x00001F28

    sceKernelSysEventDispatch(SCE_SUSPEND_EVENTS, SCE_SYSTEM_SUSPEND_EVENT_FREEZE, "freeze", &sysEventSuspendPayload,
        NULL, SCE_FALSE, NULL); // 0x00001F2C

    if (ledOffTiming == 3) // 0x00001F34 & 0x00001F00
    {
        // loc_000026CC

        sceSysconSetPollingMode(1); // 0x000026CC
        sceSysconCtrlLED(PSP_SYSCON_LED_POWER, PSP_SYSCON_LED_POWER_STATE_ON); // 0x000026D8
    }

    // 0x00001F40

    /* 
     * Check if no application holds exclusive control over the RAM area 
     * reserved for volatile memory any longer and try to obtain exclusive 
     * access over this memory area. We need access to move the volatile
     * memory to it later in the suspend process.
     */
    status = scePowerVolatileMemTryLock(SCE_KERNEL_VOLATILE_MEM_DEFAULT, NULL, NULL); // 0x00001F44
    if (status < SCE_ERROR_OK) // 0x00001F4C
    {
        /* loop forever */
        for (;;); // 0x000026C4
    }
    
    _scePowerOffCommon(); // 0x00001F54

    // 0x00001F58 - 0x00001F84
    /* Send phase 0 suspend events 0x400F - 0x4000 to syscon. */
    s32 suspendEventPhase0Event = SCE_SYSTEM_SUSPEND_EVENT_PHASE0_15;
    do
    {
        sceKernelSysEventDispatch(SCE_SUSPEND_EVENTS, suspendEventPhase0Event, "phase0",
            &sysEventSuspendPayload, NULL, SCE_FALSE, NULL);  // 0x00001F7C
    } 
    while (--suspendEventPhase0Event >= SCE_SYSTEM_SUSPEND_EVENT_PHASE0_0);

    /* Write back the entire contents of the D-cache to RAM (dirty cache lines only). */
    sceKernelDcacheWritebackAll(); // 0x00001F8C

    s8 scratchPadData[8]; // sp + 256;
    u8 sha1Digest[SCE_KERNEL_UTILS_SHA1_DIGEST_SIZE]; // sp + 272
    sceSysconReadScratchPad(0x10, scratchPadData, sizeof scratchPadData);  // 0x00001FA0

    scratchPadData[5] = 0; // 0x00001FB8
    sceKernelUtilsSha1Digest(scratchPadData, sizeof scratchPadData, sha1Digest); // 0x00001FB4

    u64 fuseId = sceSysregGetFuseId(); // 0x00001FBC

    /* Compute a SHA1 hash unique to each PSP device. */
    ((u32*)sha1Digest)[0] ^= (u32)fuseId; // 0x00001FC4 & 0x00001FD0 & 0x00001FE0
    ((u32*)sha1Digest)[1] ^= (u32)(fuseId >> 32); // 0x00001FC8 & 0x00001FD4 & 0x00001FF0
    sceKernelUtilsSha1Digest(sha1Digest, sizeof sha1Digest, sha1Digest); // 0x00001FEC

    // 0x00002008 & 0x00002010 & 0x00002014 & 0x0000201C
    u32 digestXor = ((u32*)sha1Digest)[0] ^ ((u32*)sha1Digest)[1]
        ^ ((u32*)sha1Digest)[2] ^ ((u32*)sha1Digest)[3] ^ ((u32*)sha1Digest)[4]; // $t4

    sysEventSuspendPayloadResumeData.resumePointFunc = _scePowerResumePoint; // 0x00002038

    unk332 = (((u32)&sysEventSuspendPayloadResumeData) | 0x1) ^ digestXor; // 0x00002044 & 0x00002024 & 0x00002018

    /* Write back to scratchpad. */
    sceSysconWriteScratchPad(12, &unk332, sizeof unk332); // 0x00002040

    s32 pllOutSelect = sceSysregPllGetOutSelect(); // 0x00002048
    g_Resume.pllOutSelect = pllOutSelect; // 0x00002054

    sysEventSuspendPayloadResumeData.pllOutSelect = pllOutSelect; // 0x00002068

    // Reads a value for HW address 0xBD000044
    s32 valBD000044 = sceDdr_driver_00E36648(); // 0x00002050
    g_Resume.unk20484 = valBD000044; // 0x00002058

    sysEventSuspendPayloadResumeData.unk44 = valBD000044; //0x00002060

    _scePowerFreqSuspend();

    /* Copy the Scratchpad content to the power service. */

    // uofw: In the ASM the word alignment for the destination buffer is checked (g_Resume here)
    // and depending on the result of the check either 
    //  - lw/sw are used for copying the memory (word aligned) or
    //  - lwl + lwr /swl + swr are used (if not aligned at a word boundary)
    //
    // No specific check is made on the word-alignment of the source. Instead,
    // it will use the load instructions matching the used store instructions
    // (which are dependent on the destination alignment check).
    //
    // With uofw we want to replicate the original Sony code as much as possible
    // so we will aim to use the same load/store instructions here -- even if 
    // it might not be perfectly optimized (such as no dedicated source buffer alignment 
    // check.
    if (((size_t)g_Resume.scratchpad % 4 == 0) && ((size_t)SCE_SCRATCHPAD_ADDR_K1 % 4 == 0)) // 0x0000203C & 0x00002074
    {
        // 0x00002690 - 0x00002684
        __builtin_memcpy(
            __builtin_assume_aligned(&g_Resume.scratchpad, 4), 
            __builtin_assume_aligned(SCE_SCRATCHPAD_ADDR_K1, 4), 
            SCE_SCRATCHPAD_SIZE);
    }
    else 
    {
        // 0x00002080 - 0x000020C8       
        __builtin_memcpy(g_Resume, SCE_SCRATCHPAD_ADDR_K1, SCE_SCRATCHPAD_SIZE);
    }

    /* Copy the hardware-reset-vector content to the power service. */

    // uofw: The same note as directly above applies here as well.
    if (((size_t)&g_Resume.hwResetVector % 4 == 0) && ((size_t)HW_RESET_VECTOR % 4 == 0)) // 0x000020D8 & 0x000020E0
    {
        // 0x0000265C - 0x00002688
        __builtin_memcpy(
            __builtin_assume_aligned(&g_Resume.hwResetVector, 4),
            __builtin_assume_aligned(HW_RESET_VECTOR, 4),
            HW_RESET_VECTOR_SIZE);
    }
    else
    {
        // 0x000020EC - 0x000020C8       
        __builtin_memcpy(&g_Resume.hwResetVector, HW_RESET_VECTOR, HW_RESET_VECTOR_SIZE);
    }

    // loc_0000213C

    // 0x00002140 - 0x00002164
    //
    // TODO: We should perhaps replace this with an inlined memset
    u32 *powersvcHRVClearArea = (u32 *)&g_Resume.hwResetVector[0x200];
    powersvcHRVClearArea[0] = 0;
    powersvcHRVClearArea[1] = 0;
    powersvcHRVClearArea[2] = 0;
    powersvcHRVClearArea[3] = 0;
    powersvcHRVClearArea[4] = 0;
    powersvcHRVClearArea[5] = 0;
    powersvcHRVClearArea[6] = 0;
    powersvcHRVClearArea[7] = 0;

    sceKernelDispatchSuspendHandlers(0); // 0x00002160 & 0x00002144

    /* 
     * No application code is running any logner at this point. As such, no application
     * can try to acquire a power lock to proceed with stable power state critical operations.
     * 
     * Remove the force-wait now so that when the PSP system resumes, user applications can call
     * powerLock() APIs again and proceed with their critical operations.
     */
    g_PowerSwitch.powerLockForUserShouldForceWait = SCE_FALSE; // 0x0000217C

    if (suspendMode & 0xF00 == POWER_SWITCH_SUSPEND_OP_SUSPEND) // 0x00002180 & 0x00002174
    {
        /* Suspend operation */

        // loc_00002628

        // Get CP0's state register $24.
        s32 cp0SReg24 = UtilsForKernel_A6B0A6B8(); // 0x00002628

        UtilsForKernel_39FFB756(0); // 0x00002634

        /* Save the register state (VFPU/FPU/CP0/MIPS GPR). */
        status = sceSuspendForKernel_67B59042(0); // 0x0000263C
        if (status != 0) // 0x00002644
        {
            // loc_00002554
            UtilsForKernel_39FFB756(cp0SReg24); // 0x00002554
        }
        else
        {
            /* Suspend the system. */
            sceSysconPowerSuspend(wakeUpCondition, 0); // 0x0000264C

            /*
             * Loop forever (until the PSP is actually being suspended at which point no code
             * is no longer run).
             */
            for (;;); // 0x00002654
        }
    }
    else if ((suspendMode & 0xF00) == POWER_SWITCH_SUSPEND_OP_SUSPEND_TOUCH_AND_GO) // 0x00002188 & 0x000024D4 & 0x0000218C
    {
        /* suspend touch and go. */

            // loc_00002530
        int unk = UtilsForKernel_A6B0A6B8(); // 0x00002530
        UtilsForKernel_39FFB756(0); // 0x0000253C

        /* Save the register state (VFPU/FPU/CP0/MIPS GPR). */
        status = sceSuspendForKernel_67B59042(0); // 0x00002544

        if (status != SCE_ERROR_OK) // 0x0000254C
        {
            UtilsForKernel_39FFB756(unk); // 0x00002554
        }
        else
        {
            // loc_00002564

            // TODO: Is that a potential buffer which is cleared here?
            memset(&g_Resume.unk20488, 0, 212); // 0x00002568

           // TODO: add 0 assignments
            // Why are we zeroing them out when the above memset() call already does that job?
            g_Resume.unk20516 = 0; // 0x00002580
            g_Resume.unk20504 = 0; // 0x0000258C
            g_Resume.unk20492 = 0; // 0x00002590
            g_Resume.unk20496 = 0; // 0x00002598

            /* Copy back the saved hardware-reset-vector content. */

            if (((size_t)&g_Resume.hwResetVector % 4 == 0) && ((size_t)HW_RESET_VECTOR % 4 == 0)) // 0x00002574 & 0x0000257C
            {
                // 0x000025A0 - 0x000025EC
                __builtin_memcpy(
                    __builtin_assume_aligned((u32 *)HW_RESET_VECTOR, 4),
                    __builtin_assume_aligned(g_Resume.hwResetVector, 4),
                    HW_RESET_VECTOR_SIZE);
            }
            else
            {
                // 0x000025F4 - 0x0000261C       
                __builtin_memcpy((u32 *)HW_RESET_VECTOR, g_Resume.hwResetVector, HW_RESET_VECTOR_SIZE);
            }

            // loc_000027F4

            g_PowerSwitch.resumeCount++;  // 0x000027F8 & 0x00002808

            /* Load the previously saved register state (VFPU/FPU/CP0/MIPS GPR). */
            sceSuspendForKernel_B2C9640B(1); // 0x00002804

            // Note: In the ASM, an inlined _scePowerResumePoint() directly follows
            // the call of sceSuspendForKernel_B2C9640B(). Dependinding on that function
            // this code might not be executed after all.
            return _scePowerResumePoint(1); // 0x0000280C
        }
    }
    else if ((suspendMode & 0xF00) == POWER_SWITCH_SUSPEND_OP_COLD_RESET) // 0x00002188 & 0x000024DC
    {
        /* cold reset */

        // 0x000024E4

        if (g_PowerSwitch.coldResetMode != 0) // 0x000024E8
        {
            // loc_00002500

            sceSysconCtrlMsPower(1); // 0x00002500

            u32 delayCount = 1 * 1000 * 1000; // 0x00002508 & 0x0000250C
            while (--delayCount != 0) {} // 0x00002510 - 0x00002518

            /* Reset the system. */
            sceSysconResetDevice(1, 2); // 0x00002520

            /*
             * Loop forever (until the PSP is actually being resetted at which point no code
             * is no longer run).
             */
            for (;;); // 0x00002528 & 0x000024F8
        }
        else
        {
            /* Reset the system. */
            sceSysconResetDevice(1, 1); // 0x000024F0

            /*
             * Loop forever (until the PSP is actually being resetted at which point no code
             * is no longer run).
             */
            for (;;); // 0x000024F8
        }
    }
    else if ((suspendMode & 0xF00) == POWER_SWITCH_SUSPEND_OP_STANDBY) // 0x00002194
    {
        /* Standby operation (0x101) */

        // loc_000024C4

        /* Let the system enter standby state.  */
        sceSysconPowerStandby(wakeUpCondition);  // 0x000024C4

        /* 
         * Loop forever (until the PSP is actually in standby at which point no code 
         * is no longer run).
         */
        for (;;); // 0x000024CC
    }

    /* PSP system resume operation in progress here. */

    sceKernelDispatchResumeHandlers(0); // 0x0000219C

    /* Copy back the saved Scratchpad data. */

    // uofw: The same note as directly above applies here as well.
    if (((size_t)g_Resume.scratchpad % 4 == 0) && ((size_t)SCE_SCRATCHPAD_ADDR_K1 % 4 == 0)) // 0x000021A4 & 0x000021A8 & 0x000021B0
    {
        // 0x00002690 - 0x00002684
        __builtin_memcpy(
            __builtin_assume_aligned(SCE_SCRATCHPAD_ADDR_K1, 4),
            __builtin_assume_aligned(&g_Resume.scratchpad, 4),
            SCE_SCRATCHPAD_SIZE);
    }
    else
    {
        // 0x00002490 - 0x000024C0       
        __builtin_memcpy(SCE_SCRATCHPAD_ADDR_K1, &g_Resume.scratchpad, SCE_SCRATCHPAD_SIZE);
    }

    _scePowerFreqResume(0); // 0x0000220C

    sceDdr_driver_E0A39D3E(g_Resume.unk20484); // 0x00002214 & 0x0000221C

    // possible loop ahead
    sysEventResumePlayload.unk4 = &g_Resume.unk20488; // 0x00002248

    // TODO: 0.5 seconds operand (explanation)
    sysEventResumePlayload.unk16 = (g_Resume.unk20516 - pspClock) * 500000; // 0x00002224 & 0x00002234 & 0x00002238 & 0x00002260 & 0x00002264
    sysEventResumePlayload.systemTimePreSuspendOp = sysEventSuspendPayload.systemTimePreSuspendOp; // 0x00002258 & 0x0000225C & 0x00002240 & 0x0000223C

    // 0x00002268 - 0x00002294
    /* Send phase 0 resume events 0x1000 - 0x100F to Syscon. */
    s32 resumeEventPhase0Event = SCE_SYSTEM_RESUME_EVENT_PHASE0_0;
    do
    {
        sceKernelSysEventDispatch(SCE_RESUME_EVENTS, resumeEventPhase0Event, "phase0",
            &sysEventResumePlayload, NULL, SCE_FALSE, NULL);  // 0x00002284
    } 
    while (++resumeEventPhase0Event <= SCE_SYSTEM_RESUME_EVENT_PHASE0_15);

    _scePowerFreqResume(1); // 0x00002298

    /* 
     * We no longer need to store the volatime memory in the RAM area reserved for it on suspension.
     * Relinquish exlusive control now so that user applications can get access to it (one at a time).
     */
    scePowerVolatileMemUnlock(SCE_KERNEL_VOLATILE_MEM_DEFAULT); // 0x000022A4

    /* 
     * Clear all event flags which could have let to this suspend operation. We thus indicate
     * that the requested suspend operation was processed and will now wait for new suspend
     * requests again.
     */
    s32 clearPowerSwitchFlags = POWER_SWITCH_EVENT_100
        | POWER_SWITCH_EVENT_REQUEST_STANDBY | POWER_SWITCH_EVENT_REQUEST_SUSPEND
        | POWER_SWITCH_EVENT_REQUEST_SUSPEND_TOUCH_AND_GO | POWER_SWITCH_EVENT_REQUEST_COLD_RESET
        | POWER_SWITCH_EVENT_POWER_SWITCH_ACTIVE | POWER_SWITCH_EVENT_POWER_SWITCH_INACTIVE;
    sceKernelClearEventFlag(g_PowerSwitch.eventId, clearPowerSwitchFlags); // 0x000022B0

    /* Send callbacks through the system notifying subscribers that we are resuming the system. */
    _scePowerNotifyCallback(SCE_POWER_CALLBACKARG_SUSPENDING, SCE_POWER_CALLBACKARG_RESUMING, 0); // 0x000022C0

    scePowerTick(SCE_KERNEL_POWER_TICK_DEFAULT); // 0x000022C8

    sceSysconSetAffirmativeRertyMode(0); // 0x000022D0

    /* Accept user input again. */
    sceCtrlSetPollingMode(SCE_CTRL_POLL_ACTIVE); // 0x000022D8

    // 0x000022E0 - 0x000022EC
    while (sceDisplayIsVblank() == 0);

    // 0x000022F0 - 0x000022FC
    while (sceDisplayIsVblank() != 0);

    // 0x00002300 - 0x0000230C
    while (sceDisplayGetCurrentHcount() < unkHcount);

    sceDisplayAdjustAccumulatedHcount(accumulatedHcount); // 0x00002314

    /* Send "melt" resume event 0x40000 to Syscon. */
    sceKernelSysEventDispatch(SCE_RESUME_EVENTS, SCE_SYSTEM_RESUME_EVENT_MELT, "melt",
        &sysEventResumePlayload, NULL, SCE_FALSE, NULL);  // 0x00002338

    sceKernelCpuResumeIntr(intrState); // 0x00002340

    s32 lowBattCap = scePowerGetLowBatteryCapacity(); // 0x00002348
    _scePowerChangeSuspendCap(lowBattCap & 0xFFFF); // 0x00002358

    /* Send "phase1" resume event 0x100000 to Syscon. */
    sceKernelSysEventDispatch(SCE_RESUME_EVENTS, SCE_SYSTEM_RESUME_EVENT_PHASE1_0, "phase1",
        &sysEventResumePlayload, NULL, SCE_FALSE, NULL);  // 0x00002378

    u32 resumeBusyPrintInfoCounter = 0; // 0x00002380 -- $s2 
    // 0x00002390 - 0x000023FC
    do 
    {
        resumeBusyPrintInfoCounter++;
        pResumePhase1SysEventHandler = NULL; // 0x000023AC
        resumePhase1SysEventResult = 0; // 0x000023B8

        /* Send "phase1" resume event 0x100001 to Syscon. */
        sceKernelSysEventDispatch(SCE_RESUME_EVENTS, SCE_SYSTEM_RESUME_EVENT_PHASE1_1, "phase1",
            &sysEventResumePlayload, NULL, SCE_FALSE, NULL);  // 0x00002378

        /* Send "phase1" resume event 0x100002 to Syscon. */
        status =  sceKernelSysEventDispatch(SCE_RESUME_EVENTS, SCE_SYSTEM_RESUME_EVENT_PHASE1_2, "phase1",
            &sysEventResumePlayload, &resumePhase1SysEventResult, SCE_TRUE, &pResumePhase1SysEventHandler);  // 0x00002378

        /* 
         * Just as with the suspend operation earlier, each system component (modules) might have their own 
         * resume operation. As part of the PSP's resume process, the power service waits until all system
         * components have indicated they have completed their own resume operation. Only then does the 
         * power service report to the system that we have completed the resume process.
         */

        /* 
         * A system component has reported that it is busy resuming itself. Let's wait some time and retry 
         * until all system components have finished resuming themselves.
         */
        if (status < SCE_ERROR_OK) // 0x000023E0
        {
            if (resumeBusyPrintInfoCounter == POWER_SUSPEND_OPERATION_BLOCKED_TRIES_PRINT_ERROR_THRESHOLD) // 0x000023E8
            {
                resumeBusyPrintInfoCounter = 0; // 0x00002470
                if (pResumePhase1SysEventHandler != NULL) // 0x00002474
                {
                    Kprintf("%s Busy: 0x%08X, 0x%08X, %08X\n", pResumePhase1SysEventHandler->name,
                        status, &pResumePhase1SysEventHandler, resumePhase1SysEventResult); // 0x00002480
                }
                
            }

            /* Wait 5 milliseconds. */
            sceKernelDelayThread(POWER_RESUME_OPERATION_DELAY_TIME); // 0x000023F0
        }
    } 
    while (status < SCE_ERROR_OK);

    // loc_00002400

    /* 
     * All system components have finished resuming themselved. We will now report to the system
     * that the resume process has been completed.
     */
    status = sceKernelSysEventDispatch(SCE_RESUME_EVENTS, SCE_SYSTEN_RESUME_EVENT_COMPLETED, "completed",
        &sysEventResumePlayload, NULL, SCE_FALSE, NULL);  // 0x0000241C

    /* 
     * We finished resuming so we remove this bit from the global power state flag and send out 
     * callbacks with a one-time SCE_POWER_CALLBACKARG_RESUME_COMP notification.
     */
    _scePowerNotifyCallback(SCE_POWER_CALLBACKARG_RESUMING, 0, SCE_POWER_CALLBACKARG_RESUME_COMP); // 0x0000242C

    return SCE_ERROR_OK;
}

//0x0000280C
static s32 _scePowerResumePoint(void *pData)
{
    if (pData != NULL) // 0x00002810
    {
        /* Copy content of the specified source buffer to the power service. */

        // s32 copySize = *(s32 *)pData; // 0x00002818
        s32 count = 0; // 0x0000282C -- $a0
        u8 *pDst = (u8 *)&g_Resume.unk20488; // 0x00002820
        u8 *pSrc = (u8 *)pData;

        while (count++ < *(s32 *)pData)
        {
            *pDst++ = *pSrc++; // 0x00002830 - 0x0000283C
        }

        // loc_00002850

        g_Resume.unk20508 = 0; // 0x00002858
        if (*(s32 *)(pData + 20) != NULL) // 0x00002860
        {
            /* Restore PLL clock frequency at the time of the supension. */

            void (*func)(s32) = (void (*)(s32)) *(s32 *)(pData + 20);
            func(g_Resume.pllOutSelect); // 0x0000294C
        }
    }

    // loc_00002868

    /* Copy some hardware data first */

    // 0x00002878 - 0x000028B0
    s32 hwData1 = HW(0xBFC00512);
    s32 hwData2 = HW(0xBFC00516);
    s32 hwData3 = HW(0xBFC00520);
    s32 hwData4 = HW(0xBFC00524);
    s32 hwData5 = HW(0xBFC00528);
    s32 hwData6 = HW(0xBFC00532);
    s32 hwData7 = HW(0xBFC00536);
    s32 hwData8 = HW(0xBFC00540);


    /* Write back the saved hardware-reset-vector. */

    // uofw: In the ASM the word alignment for the destination buffer is checked (g_Resume here)
    // and depending on the result of the check either 
    //  - lw/sw are used for copying the memory (word aligned) or
    //  - lwl + lwr /swl + swr are used (if not aligned at a word boundary)
    //
    // No specific check is made on the word-alignment of the source. Instead,
    // it will use the load instructions matching the used store instructions
    // (which are dependent on the destination alignment check).
    //
    // With uofw we want to replicate the original Sony code as much as possible
    // so we will aim to use the same load/store instructions here -- even if 
    // it might not be perfectly optimized (such as no dedicated source buffer alignment 
    // check.
    if (((size_t)g_Resume.hwResetVector % 4 == 0) && ((size_t)HW_RESET_VECTOR % 4 == 0)) // 0x0000203C & 0x00002074
    {
        // 0x00002918 - 0x00002940
        __builtin_memcpy(
            __builtin_assume_aligned((vs32 *)HW_RESET_VECTOR, 4),
            __builtin_assume_aligned(&g_Resume.hwResetVector, 4),
            HW_RESET_VECTOR_SIZE);
    }
    else
    {
        // 0x000028C4 - 0x0000290C       
        __builtin_memcpy((vs32 *)HW_RESET_VECTOR, &g_Resume.hwResetVector, HW_RESET_VECTOR_SIZE);
    }

     // loc_0000295C

    g_PowerSwitch.resumeCount++; // 0x00002960 & 0x00002974

    /*
     * Write back the VFPU's, FPU's and COP's control and status registers from the Sysmem internal
     * mem block.
     */

    // 0x00002964 - 0x000029AC
    HW(0xBFC00512) = hwData1;
    HW(0xBFC00516) = hwData2;
    HW(0xBFC00520) = hwData3;
    HW(0xBFC00524) = hwData4;
    HW(0xBFC00528) = hwData5;
    HW(0xBFC00532) = hwData6;
    HW(0xBFC00536) = hwData7;
    HW(0xBFC00540) = hwData8;

    // Note: This probably won't return here as otherwise no jr $ra exists to execute and 
    // _scePowerOffCommon() would be executed following this call.
    sceSuspendForKernel_B2C9640B(1); // 0x0000296C 
}

//sub_000029B8
// TODO: Verify function
static u32 _scePowerOffCommon(void)
{
    s32 usbPowerType;

    sceGpioPortClear(0x2000000); //0x000029C8
    usbPowerType = _sceSysconGetUsbPowerType();
    if (usbPowerType == 0) //0x000029D8
        sceGpioPortClear(0x00800000); //0x00002A94

    //0x000029E0 - 0x00002A00
    u32 i;
    for (i = 0; i < 6; i++) {
        if ((i != 2) && (i != 3)) //0x000029F0
            sceSysregUartIoDisable(i); //0x00002A84
    }
    //0x00002A04 - 0x00002A1C
    for (i = 0; i < 6; i++) {
        if (i != 0) //0x00002A08
            sceSysregSpiIoDisable(i); //0x00002A74
    }
    //0x00002A20 - 0x00002A40
    for (i = 0; i < 32; i++) {
        if ((i != 3) && (i != 4) && (i != 7)) //0x00002A2C
            sceSysreg_driver_15DC34BC(i); //0x00002A64
    }
    return SCE_ERROR_OK;
}

//sub_00002AA4
s32 _scePowerSwEnd(void)
{
    u32 startAddr;

    /* Tell our power switch worker thread to wrap up. */
    sceKernelSetEventFlag(g_PowerSwitch.eventId, POWER_SWITCH_EVENT_PROCESSING_TERMINATION); // 0x00002AC0
    sceKernelWaitThreadEnd(g_PowerSwitch.workerThreadId, NULL); // 0x00002ACC

    /* Unregister callbacks and power handlers registered in the system. */
    sceKernelRegisterPowerHandlers(NULL); // 0x00002AD4
    sceSysconSetPowerSwitchCallback(NULL, NULL); // 0x00002AE0
    sceSysconSetHoldSwitchCallback(NULL, NULL); // 0x00002AEC

    sceKernelDeleteSema(g_PowerSwitch.volatileMemorySemaId); // 0x00002AF4
    sceKernelDeleteEventFlag(g_PowerSwitch.eventId); // 0x00002AFC

    /* 
     * Allow user mode applications access to the memory area reserved for volatile memory
     * if that area is in kernel space (the running power service is responsible for 
     * "locking" access to that memory area to a single request at a time). 
     */
    startAddr = g_PowerSwitch.volatileMemoryReservedAreaStartAddr;
    if ((startAddr >= SCE_KERNELSPACE_ADDR_KU0 && startAddr < SCE_USERSPACE_ADDR_KU0)
        || (startAddr >= SCE_KERNELSPACE_ADDR_KU1 && startAddr < SCE_USERSPACE_ADDR_KU1)
        || (startAddr >= SCE_KERNELSPACE_ADDR_K0 && startAddr < SCE_USERSPACE_ADDR_K0)
        || (startAddr >= SCE_KERNELSPACE_ADDR_K1 && startAddr < SCE_USERSPACE_ADDR_K1)) // 0x00002B08 - 0x00002B20 & 0x00002B40
    {
        sceKernelSetDdrMemoryProtection(startAddr, g_PowerSwitch.volatileMemoryReservedAreaSize, 0xF); // 0x00002B48
    }

    return SCE_ERROR_OK;
}

//Subroutine scePower_0CD21B1F - Address 0x00002B58 - Aliases: scePower_driver_73785D34
// TODO: Verify function
u32 scePowerSetPowerSwMode(u32 mode)
{
    g_PowerSwitch.mode = mode & 0x3;
    return SCE_ERROR_OK;
}

//Subroutine scePower_165CE085 - Address 0x00002B6C - Aliases: scePower_driver_E11CDFFA
// TODO: Verify function
u32 scePowerGetPowerSwMode(void)
{
    return g_PowerSwitch.mode;
}

//Subroutine scePower_driver_1EC2D4E4 - Address 0x00002B78
// TODO: Verify function
u32 scePowerRebootStart(void)
{
    s32 oldK1;
    s32 intrState;

    oldK1 = pspShiftK1();

    _scePowerLock(SCE_KERNEL_POWER_LOCK_DEFAULT, SCE_TRUE); //0x00002B98

    pspSetK1(oldK1); //0x00002BB0
    intrState = sceKernelCpuSuspendIntr(); //0x00002BAC

    sceKernelClearEventFlag(g_PowerSwitch.eventId, ~POWER_SWITCH_EVENT_POWER_SWITCH_UNLOCKED); //0x00002BC0
    sceKernelSetEventFlag(g_PowerSwitch.eventId, POWER_SWITCH_EVENT_IDLE); //0x00002BCC

    sceKernelCpuResumeIntr(intrState); //0x00002BD4
    return SCE_ERROR_OK;
}

//Subroutine scePower_7FA406DD - Address 0x00002C84 - Aliases: scePower_driver_566B8353
s32 scePowerIsRequest(void)
{
    return g_PowerSwitch.curHardwarePowerSwitchRequest != HARDWARE_POWER_SWITCH_REQUEST_NONE
        || g_PowerSwitch.curSoftwarePowerSwitchRequest != SOFTWARE_POWER_SWITCH_REQUEST_NONE;
}

//Subroutine scePower_DB62C9CF - Address 0x00002CA0 - Aliases: scePower_driver_DB62C9CF
s32 scePowerCancelRequest(void)
{
    s32 intrState;
    ScePowerHardwarePowerSwitchRequest canceledRequest;

    intrState = sceKernelCpuSuspendIntr(); // 0x00002CA8

    canceledRequest = g_PowerSwitch.curHardwarePowerSwitchRequest; // 0x00002CB8
    g_PowerSwitch.curHardwarePowerSwitchRequest = HARDWARE_POWER_SWITCH_REQUEST_NONE;

    sceKernelCpuResumeIntr(intrState);
    return (s32)canceledRequest;
}

//Subroutine scePower_3951AF53 - Address 0x0000171C - Aliases: scePower_driver_3300D85A
s32 scePowerWaitRequestCompletion(void)
{
    s32 oldK1;
    s32 status;
    u32 powerSwitchRequestFlags;
    u32 powerSwitchSetFlags;

    oldK1 = pspShiftK1(); // 0x00001748

    status = sceKernelPollEventFlag(g_PowerSwitch.eventId, 0xFFFFFFFF, SCE_KERNEL_EW_OR, &powerSwitchSetFlags); // 0x00001744
    if (status < SCE_ERROR_OK && status != SCE_ERROR_KERNEL_EVENT_FLAG_POLL_FAILED) // 0x0000174C & 0x00001764
    {
        pspSetK1(oldK1); // 0x000017EC
        return status;
    }

    powerSwitchRequestFlags = POWER_SWITCH_EVENT_REQUEST_STANDBY | POWER_SWITCH_EVENT_REQUEST_SUSPEND
        | POWER_SWITCH_EVENT_REQUEST_SUSPEND_TOUCH_AND_GO | POWER_SWITCH_EVENT_REQUEST_COLD_RESET
        | POWER_SWITCH_EVENT_100;

    /* Check if a progammatically generated power switch request is currently in the system. */
    if (!(powerSwitchSetFlags & powerSwitchRequestFlags)) // 0x0000177C
    {
        /* No programmatically generated requests are in the system so we have nothing to wait on. */

        pspSetK1(oldK1); // 0x000017EC
        return SCE_ERROR_OK;
    }

    /* Check if the registered power switch request is already being processed. */
    if (!(powerSwitchRequestFlags & POWER_SWITCH_EVENT_OPERATION_RUNNING)) // 0x00001790
    {
        /* The registered request is not yet being processed. Wait until the system begins prcessing it. */
        status = sceKernelWaitEventFlag(g_PowerSwitch.eventId, POWER_SWITCH_EVENT_OPERATION_RUNNING, 
            SCE_KERNEL_EW_OR, &powerSwitchSetFlags, NULL); // 0x00001798

        if (status < SCE_ERROR_OK) // 0x000017A0
        {
            pspSetK1(oldK1); // 0x000017EC
            return status;
        }
    }

    /* 
     * The registered power switch request is currently being processed. Wait until the system 
     * has completed processing it.
     */
    status = sceKernelWaitEventFlag(g_PowerSwitch.eventId, POWER_SWITCH_EVENT_IDLE, SCE_KERNEL_EW_OR,
        &powerSwitchSetFlags, NULL); // 0x000017B8

    /* Registered power switch request has been fully processed. We can return now. */

    pspSetK1(oldK1);
    return (status < SCE_ERROR_OK) ? status : SCE_ERROR_OK; // 0x000017C4
}

//Subroutine scePower_2B7C7CF4 - Address 0x00002CDC - Aliases: scePower_driver_9B44CFD9
s32 scePowerRequestStandby(void)
{
    s32 oldK1;

    oldK1 = pspShiftK1();

    sceKernelSetEventFlag(g_PowerSwitch.eventId, POWER_SWITCH_EVENT_REQUEST_STANDBY); // 0x00002CF8

    pspSetK1(oldK1);
    return SCE_ERROR_OK;
}

//Subroutine scePower_AC32C9CC - Address 0x00002D18 - Aliases: scePower_driver_5C1333B7
s32 scePowerRequestSuspend(void)
{
    s32 oldK1;

    oldK1 = pspShiftK1();

    sceKernelSetEventFlag(g_PowerSwitch.eventId, POWER_SWITCH_EVENT_REQUEST_SUSPEND); // 0x00002D34

    pspSetK1(oldK1);
    return SCE_ERROR_OK;
}

//Subroutine scePower_driver_D79B0122 - Address 0x00002D54
s32 scePower_driver_D79B0122(void)
{
    return SCE_ERROR_OK;
}

//Subroutine scePower_2875994B - Address 0x00002D5C - Aliases: scePower_driver_D1FFF513
s32 scePowerRequestSuspendTouchAndGo(void)
{
    s32 oldK1;

    oldK1 = pspShiftK1();

    sceKernelSetEventFlag(g_PowerSwitch.eventId, POWER_SWITCH_EVENT_REQUEST_SUSPEND_TOUCH_AND_GO); // 0x00002D78

    pspSetK1(oldK1);
    return SCE_ERROR_OK;
}

//Subroutine scePower_0442D852 - Address 0x00002D98 - Aliases: scePower_driver_9DAF25A0
s32 scePowerRequestColdReset(s32 mode)
{
    s32 oldK1;

    oldK1 = pspShiftK1();

    if (pspK1IsUserMode() && mode != 0) // 0x00002DBC & 0x00002DC8
    {
        pspSetK1(oldK1);
        return SCE_ERROR_INVALID_MODE;
    }

    g_PowerSwitch.coldResetMode = mode;

    sceKernelSetEventFlag(g_PowerSwitch.eventId, POWER_SWITCH_EVENT_REQUEST_COLD_RESET); // 0x00002DD4

    pspSetK1(oldK1);
    return SCE_ERROR_OK;
}

//Subroutine scePower_0074EF9B - Address 0x00002DFC - Aliases: scePower_driver_B45C9066
// TODO: Verify function
u32 scePowerGetResumeCount(void)
{
    return g_PowerSwitch.resumeCount;
}

//Subroutine scePower_driver_BA566CD0 - Address 0x00002E0C
// TODO: Verify function
u32 scePowerSetWakeupCondition(u32 wakeUpCondition)
{
    g_PowerSwitch.wakeUpCondition = wakeUpCondition;
    return SCE_ERROR_OK;
}

//0x00002EE0
/* 
 * Listens to POWER switch changes (sent from Syscon) and reports a power service callback. 
 * Notifies the power service that a suspend/standby operation is in the process of being requested.
 */
static void _scePowerPowerSwCallback(s32 enable, void* argp)
{
    (void)argp;

    if (enable) // 0x00002EF4
    {
        sceKernelSetEventFlag(g_PowerSwitch.eventId, POWER_SWITCH_EVENT_POWER_SWITCH_ACTIVE); // 0x00002EFC
        sceKernelClearEventFlag(g_PowerSwitch.eventId, ~POWER_SWITCH_EVENT_POWER_SWITCH_INACTIVE); // 0x00002F08

        _scePowerNotifyCallback(0, SCE_POWER_CALLBACKARG_POWER_SWITCH, 0); // 0x00002F14
    }
    else 
    {
        sceKernelSetEventFlag(g_PowerSwitch.eventId, POWER_SWITCH_EVENT_POWER_SWITCH_INACTIVE); // 0x00002F38
        sceKernelClearEventFlag(g_PowerSwitch.eventId, ~POWER_SWITCH_EVENT_POWER_SWITCH_ACTIVE); // 0x00002F44

        _scePowerNotifyCallback(SCE_POWER_CALLBACKARG_POWER_SWITCH, 0, 0); // 0x00002F4C
    }
}

//0x00002F58
/* Listens to HOLD switch changes (sent from Syscon) and reports a power service callback. */
static void _scePowerHoldSwCallback(s32 enable, void* argp)
{
    (void)argp;

    if (enable) // 0x00002F6C
        _scePowerNotifyCallback(0, SCE_POWER_CALLBACKARG_HOLD_SWITCH, 0); // 0x00002F64
    else
        _scePowerNotifyCallback(SCE_POWER_CALLBACKARG_HOLD_SWITCH, 0, 0); // 0x00002F74
}