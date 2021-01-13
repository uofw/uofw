/* Copyright (C) The uOFW team
   See the file COPYING for copying permission.
*/

#include <common_imp.h>
#include <interruptman.h>
#include <power_kernel.h>
#include <syscon.h>
#include <sysmem_kernel.h>
#include <sysmem_suspend_kernel.h>
#include <sysmem_sysclib.h>
#include <threadman_kernel.h>

#include "power_int.h"

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

#define SCE_KERNEL_POWER_VOLATILE_MEM_DEFAULT       (0) // TODO: Remove

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
    s32 semaId; //4
    s32 threadId; //8
    u32 mode; //12
    u32 unk16; //16
    s32 numPowerLocksUser; // 20
    s32 numPowerLocksKernel; // 24
    u32 startAddr; //28
    u32 memSize; //32
    u32 unk36; //36
    ScePowerHardwarePowerSwitchRequest curHardwarePowerSwitchRequest; //40
    ScePowerSoftwarePowerSwitchRequest curSoftwarePowerSwitchRequest; //44
    u32 unk48; //48 TODO: Perhaps a flag indicating whether locking/unlocking is allowed or - more gnerally - standby/suspension/reboot?
    u32 coldResetMode; // 52
    u32 unk56; // 56
    u32 wakeUpCondition; //60
    u32 resumeCount; //64
} ScePowerSwitch; //size: 68

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
static s32 _scePowerSuspendOperation(u32 arg1);
static s32 _scePowerResumePoint(u32 arg0);
static u32 _scePowerOffCommon(void);
static void _scePowerPowerSwCallback(s32 enable, void* argp);
static void _scePowerHoldSwCallback(s32 enable, void* argp);

ScePowerSwitch g_PowerSwitch; //0x0000729C

//sub_000011A4
// TODO: Verify function
/* Initializes the power switch component of the power service. */
u32 _scePowerSwInit(void)
{
    s32 intrState;
    s32 nPowerLock;
    SceSysmemPartitionInfo partitionInfo;

    memset(&g_PowerSwitch, 0, sizeof g_PowerSwitch); //0x000011C4

    g_PowerSwitch.mode = 2; //0x000011E8
    g_PowerSwitch.wakeUpCondition = 8; //0x000011F0

    g_PowerSwitch.eventId = sceKernelCreateEventFlag("ScePowerSw", SCE_KERNEL_EA_MULTI | 0x1,
        POWER_SWITCH_EVENT_IDLE | POWER_SWITCH_EVENT_POWER_SWITCH_UNLOCKED, NULL); //0x000011EC & 0x00001210
    g_PowerSwitch.semaId = sceKernelCreateSema("ScePowerVmem", 1, 0, 1, NULL); //0x0000120C & 0x0000123C
    g_PowerSwitch.threadId = sceKernelCreateThread("ScePowerMain", _scePowerOffThread, 4, 2048,
        SCE_KERNEL_TH_NO_FILLSTACK | 0x1, NULL); //0x00001238 & 0x00001250

    sceKernelStartThread(g_PowerSwitch.threadId, 0, NULL); //0x0000124C   
    sceSysconSetPowerSwitchCallback(_scePowerPowerSwCallback, NULL); //0x0000125C
    sceSysconSetHoldSwitchCallback(_scePowerHoldSwCallback, NULL); //0x0000126C

    partitionInfo.size = 16; //0x00001284
    sceKernelQueryMemoryPartitionInfo(5, &partitionInfo); //0x00001280
    g_PowerSwitch.memSize = partitionInfo.memSize; //0x00001290
    g_PowerSwitch.startAddr = partitionInfo.startAddr; //0x00001298

    intrState = sceKernelCpuSuspendIntr(); //0x00001294

    scePowerLockForKernel(SCE_KERNEL_POWER_LOCK_DEFAULT); //0x000012A0 -- scePower_driver_6CF50928
    nPowerLock = sceKernelRegisterPowerHandlers(&g_PowerHandler); //0x000012AC
    g_PowerSwitch.numPowerLocksKernel += nPowerLock; //0x000012BC

    sceKernelCpuResumeIntr(intrState); //0x000012C0
    return SCE_ERROR_OK;
}

//Subroutine scePower_23C31FFE - Address 0x000010A4 - Aliases: scePower_driver_CE239543
// TODO: Verify function
s32 scePowerVolatileMemLock(s32 mode, void** ptr, s32* size)
{
    s32 intrState;
    s32 oldK1;
    s32 status;
    u32 nBits;

    if (mode != SCE_KERNEL_POWER_VOLATILE_MEM_DEFAULT) //0x0000130C
        return SCE_ERROR_INVALID_MODE;

    oldK1 = pspShiftK1(); //0x00001314

    if (!pspK1PtrOk(ptr) || !pspK1PtrOk(size)) { //0x00001320 & loc_00001364
        pspSetK1(oldK1);
        return SCE_ERROR_PRIV_REQUIRED;
    }

    status = sceKernelWaitSema(g_PowerSwitch.semaId, 1, NULL); //0x00001370
    if (ptr != NULL) //0x00001378
        *ptr = g_PowerSwitch.startAddr; //0x00001384
    if (size != NULL) //0x00001388
        *size = g_PowerSwitch.memSize; //0x00001394

    intrState = sceKernelCpuSuspendIntr(); //0x00001398

    /* test for 1024 byte alignment. */
    if (((g_PowerSwitch.startAddr & 0x1F800000) >> 22) == 0x10) { //0x000013B0
        /* test for 256 byte alignment. */
        nBits = ((g_PowerSwitch.startAddr & 0xE0000000) >> 28); //0x000013B4 & 0x000013DC
        if (((s32)0x35 >> nBits) & 0x1) //0x000013E4
            sceKernelSetDdrMemoryProtection(g_PowerSwitch.startAddr, g_PowerSwitch.memSize, 0xF); //0x000013F0
    }
    g_PowerSwitch.unk36 = 1; //0x000013C8

    sceKernelCpuResumeIntrWithSync(intrState); //0x000013C4
    pspSetK1(oldK1);
    return status;
}

//Subroutine scePower_FA97A599 - Address 0x00001400 - Aliases: scePower_driver_A882AEB7
// TODO: Verify function
s32 scePowerVolatileMemTryLock(s32 mode, void** ptr, s32* size)
{
    s32 intrState;
    s32 oldK1;
    s32 status;
    u32 nBits;

    if (mode != SCE_KERNEL_POWER_VOLATILE_MEM_DEFAULT) //0x0000142C
        return SCE_ERROR_INVALID_MODE;

    oldK1 = pspShiftK1(); //0x00001434

    if (!pspK1PtrOk(ptr) || !pspK1PtrOk(size)) { //0x00001440 & 0x0000144C
        pspSetK1(oldK1);
        return SCE_ERROR_PRIV_REQUIRED;
    }

    status = sceKernelPollSema(g_PowerSwitch.semaId, 1); //0x0000148C
    if (status != SCE_ERROR_OK) { //0x00001494
        pspSetK1(oldK1);
        return (status == SCE_ERROR_KERNEL_SEMA_ZERO) ? SCE_POWER_ERROR_CANNOT_LOCK_VMEM : status; //0x00001524 - 0x0000153C
    }

    if (ptr != NULL) //0x0000149C
        *ptr = g_PowerSwitch.startAddr; //0x000014A8
    if (size != NULL) //0x000014AC
        *size = g_PowerSwitch.memSize; //0x000014B8

    intrState = sceKernelCpuSuspendIntr(); //0x000014BC

    /* test for 1024 byte alignment. */
    if (((g_PowerSwitch.startAddr & 0x1FFFFFC0) >> 6) == 0x10) { //0x000014D4
        /* test for 256 byte alignment. */
        nBits = ((g_PowerSwitch.startAddr & 0xFFFFFFF8) >> 3) & 0x1F; //0x000014D8 & 0x00001500
        if (((s32)0x35 >> nBits) & 0x1) //0x00001508
            sceKernelSetDdrMemoryProtection(g_PowerSwitch.startAddr, g_PowerSwitch.memSize, 0xF); //0x00001514
    }
    g_PowerSwitch.unk36 = 1; //0x000014EC

    sceKernelCpuResumeIntrWithSync(intrState); //0x000014E8
    pspSetK1(oldK1);
    return status;
}

//Subroutine scePower_B3EDD801 - Address 0x00001540 - Aliases: scePower_driver_5978B1C2
// TODO: Verify function
s32 scePowerVolatileMemUnlock(s32 mode)
{
    s32 oldK1;
    s32 intrState;
    s32 status;
    u32 nBits;

    if (mode != SCE_KERNEL_POWER_VOLATILE_MEM_DEFAULT)
        return SCE_ERROR_INVALID_MODE;

    oldK1 = pspShiftK1(); //0x00001434

    sceKernelDcacheWritebackInvalidateRange(g_PowerSwitch.startAddr, g_PowerSwitch.memSize); //0x0000157C

    intrState = sceKernelCpuSuspendIntr(); //0x00001584

    /* test for 1024 byte alignment. */
    if (((g_PowerSwitch.startAddr & 0x1FFFFFC0) >> 6) == 0x10) { //0x000015A0 & 0x000015A8
        /* test for 256 byte alignment. */
        nBits = ((g_PowerSwitch.startAddr & 0xFFFFFFF8) >> 3) & 0x1F; //0x00001598 & 0x0000159C & 0x000015AC & 0x000015F0
        if (((s32)0x35 >> nBits) & 0x1) //0x00001508
            sceKernelSetDdrMemoryProtection(g_PowerSwitch.startAddr, g_PowerSwitch.memSize, 0); //0x000015F8
    }
    status = sceKernelSignalSema(g_PowerSwitch.semaId, 1); //0x000015B4
    g_PowerSwitch.unk36 = 0; //0x000015BC

    sceKernelCpuResumeIntrWithSync(intrState); //0x000014E8
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

    /* We only support SCE_KERNEL_POWER_LOCK_DEFAULT currently. */
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

        if (g_PowerSwitch.unk16 == 0) // 0x000016C4
        {
            return SCE_ERROR_OK;
        }

        /*
         * Normally, we return immediately. However, we block the thread when a suspend/standby/reboot process
         * has already been started. We only proceed when the power switch manager is in the Idle state.
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
                _scePowerSuspendOperation(0x101);
            }
            else if (g_PowerSwitch.curHardwarePowerSwitchRequest == HARDWARE_POWER_SWITCH_REQUEST_SUSPEND
                || g_PowerSwitch.curSoftwarePowerSwitchRequest == SOFTWARE_POWER_SWITCH_REQUEST_SUSPEND) // 0x000018F0 & 0x000018FC
            {
                _scePowerSuspendOperation(0x202); // 0x000018F4 & 0x00001950

                g_PowerSwitch.curHardwarePowerSwitchRequest = HARDWARE_POWER_SWITCH_REQUEST_NONE; // 0x00001958
                g_PowerSwitch.curSoftwarePowerSwitchRequest = SOFTWARE_POWER_SWITCH_REQUEST_NONE; // 0x0000195C
            }
            else if (g_PowerSwitch.curSoftwarePowerSwitchRequest == SOFTWARE_POWER_SWITCH_REQUEST_SUSPEND_TOUCH_AND_GO) // 0x00001908
            {
                _scePowerSuspendOperation(0x303); // 0x0000190C & 0x00001950

                g_PowerSwitch.curHardwarePowerSwitchRequest = HARDWARE_POWER_SWITCH_REQUEST_NONE; // 0x00001958
                g_PowerSwitch.curSoftwarePowerSwitchRequest = SOFTWARE_POWER_SWITCH_REQUEST_NONE; // 0x0000195C
            }
            else if (g_PowerSwitch.curSoftwarePowerSwitchRequest == SOFTWARE_POWER_SWITCH_REQUEST_COLD_RESET) // 0x00001914
            {
                _scePowerSuspendOperation(0x404); // 0x00001918 & 0x00001940
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

//sub_00001AE8
// TODO: Verify function
static s32 _scePowerSuspendOperation(u32 arg1)
{
    u8 buf[64];
    u8 buf2[56];
    u8 buf3[128];
    void* unk1; //sp + 200

    memset(buf, 0, sizeof buf); //0x00001B24
    memset(buf2, 0, sizeof buf2); //0x00001B38 - 0x00001B70
    memset(buf3, 0, sizeof buf3); //0x00001B6C

    ((u32*)buf)[0] = sizeof buf; //0x00001B88
    ((u32*)buf2)[0] = sizeof buf2; //0x00001B8C
    ((u32*)buf3)[0] = sizeof buf3; //0x00001B90
    ((u32*)buf3)[1] = 0x6060010; //0x00001B94 -- sdk version?

    ((u32*)buf3)[13] = sceKernelGetUMDData(); //0x00001B94
    InitForKernel_D83A9BD7(unk1); //0x00001BA0

    scePowerGetLedOffTiming(); //0x00001BC8
}

// 0x0000280C
static s32 _scePowerResumePoint(u32 arg0)
{
    // TODO
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
// TODO: Verify function
s32 _scePowerSwEnd(void)
{
    u32 nBits;

    sceKernelSetEventFlag(g_PowerSwitch.eventId, POWER_SWITCH_EVENT_PROCESSING_TERMINATION); //0x00002AC0
    sceKernelWaitThreadEnd(g_PowerSwitch.threadId, 0); //0x00002ACC

    sceKernelRegisterPowerHandlers(NULL); //0x00002AD4
    sceSysconSetPowerSwitchCallback(NULL, NULL); //0x00002AE0
    sceSysconSetHoldSwitchCallback(NULL, NULL); //0x00002AEC

    sceKernelDeleteSema(g_PowerSwitch.semaId); //0x00002AF4
    sceKernelDeleteEventFlag(g_PowerSwitch.eventId); //0x00002AFC


    /* test if address is between 0x087FFFFF and 0x08000000. */
    if (((g_PowerSwitch.startAddr & 0x1F800000) >> 22) == 0x10) { //0x00002B20
        /* Support 0x0..., 0x1..., 0x4..., 0x5..., 0x8..., 0x9..., 0xA..., 0xB...  */
        nBits = ((g_PowerSwitch.startAddr & 0xE0000000) >> 28); //0x00002B0C
        if (((s32)0x35 >> nBits) & 0x1) //0x00002B10 & 0x00002B14
            sceKernelSetDdrMemoryProtection(g_PowerSwitch.startAddr, g_PowerSwitch.memSize, 0xF); //0x000013F0
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
// TODO: Verify function
static void _scePowerPowerSwCallback(s32 enable, void* argp)
{
    (void)argp;

    if (enable != 0) { //0x00002EF4
        sceKernelSetEventFlag(g_PowerSwitch.eventId, POWER_SWITCH_EVENT_POWER_SWITCH_ACTIVE); //0x00002EFC
        sceKernelClearEventFlag(g_PowerSwitch.eventId, ~POWER_SWITCH_EVENT_POWER_SWITCH_INACTIVE); //0x00002F08

        _scePowerNotifyCallback(0, SCE_POWER_CALLBACKARG_POWER_SWITCH, 0); //0x00002F14
    }
    else {
        sceKernelSetEventFlag(g_PowerSwitch.eventId, POWER_SWITCH_EVENT_POWER_SWITCH_INACTIVE); //0x00002F38
        sceKernelClearEventFlag(g_PowerSwitch.eventId, ~POWER_SWITCH_EVENT_POWER_SWITCH_ACTIVE); //0x00002F44

        _scePowerNotifyCallback(SCE_POWER_CALLBACKARG_POWER_SWITCH, 0, 0); //0x00002F4C
    }
}

//0x00002F58
// TODO: Verify function
static void _scePowerHoldSwCallback(s32 enable, void* argp)
{
    (void)argp;

    if (enable != 0) //0x00002F6C
        _scePowerNotifyCallback(0, SCE_POWER_CALLBACKARG_HOLD_SWITCH, 0); //0x00002F64
    else
        _scePowerNotifyCallback(SCE_POWER_CALLBACKARG_HOLD_SWITCH, 0, 0); //0x00002F74
}