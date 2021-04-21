/* Copyright (C) 2011, 2012 The uOFW team
   See the file COPYING for copying permission.
*/

#include <common_imp.h>

#include "interruptman.h"
#include "lowio_sysreg.h"
#include "modulemgr_init.h"
#include "sysmem_kdebug.h"
#include "sysmem_kernel.h"
#include "sysmem_sysevent.h"
#include "sysmem_sysclib.h"
#include "sysmem_utils_kernel.h"
#include "threadman_kernel.h"

#include "ge_kernel.h"

#include "ge_int.h"

SCE_MODULE_INFO("sceGE_Manager", SCE_MODULE_KERNEL | SCE_MODULE_ATTR_CANT_STOP | SCE_MODULE_ATTR_EXCLUSIVE_LOAD
                                 | SCE_MODULE_ATTR_EXCLUSIVE_START, 1, 11);
SCE_MODULE_BOOTSTART("_sceGeModuleStart");
SCE_MODULE_REBOOT_BEFORE("_sceGeModuleRebootBefore");
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wattribute-alias"
SCE_MODULE_REBOOT_PHASE("_sceGeModuleRebootPhase");
#pragma GCC diagnostic pop
SCE_SDK_VERSION(SDK_VERSION);

// 6640
SceIntrCb g_GeSubIntrFunc = {
    0, 0, NULL, NULL,
    NULL, NULL, NULL, NULL,
    NULL, NULL, NULL
};

// 666C
SceIntrHandler g_GeIntrOpt = { 12, 0x00000020, &g_GeSubIntrFunc };

// 6678
SceKernelDeci2Ops g_Deci2Ops = { 0x3C,
    {
     (void *)sceGeGetReg,
     (void *)sceGeGetCmd,
     (void *)sceGeGetMtx,
     (void *)sceGeSetReg,
     (void *)sceGeSetCmd,
     (void *)sceGeSetMtx,
     (void *)sceGeGetListIdList,
     (void *)sceGeGetList,
     (void *)sceGeGetStack,
     (void *)sceGePutBreakpoint,
     (void *)sceGeGetBreakpoint,
     (void *)sceGeDebugBreak,
     (void *)sceGeDebugContinue,
     (void *)sceGeRegisterLogHandler}
};

// Bitfield containing the GE commands 
char save_regs[] = { // 66B4
    0x07, 0x00, 0xFD, 0xFF,
    0xFF, 0xF1, 0xCF, 0x01,
    0xFC, 0x3F, 0xFB, 0xF9,
    0xFF, 0xFF, 0xFF, 0xFF,
    0xFF, 0xFF, 0xFF, 0xFF,
    0xFF, 0xFF, 0x3F, 0xFF,
    0xEF, 0xFF, 0xFD, 0xFF,
    0xFF, 0x5B, 0x7F, 0x03
};

// 66D4
volatile void *g_pAwRegAdr[32] = {
    &HW_GE_RESET,
    &HW_GE_UNK004,
    &HW_GE_EDRAM_HW_SIZE,
    &HW_GE_EXEC,
    &HW_GE_UNK104,
    &HW_GE_LISTADDR,
    &HW_GE_STALLADDR,
    &HW_GE_RADR1,
    &HW_GE_RADR2,
    &HW_GE_VADR,
    &HW_GE_IADR,
    &HW_GE_OADR,
    &HW_GE_OADR1,
    &HW_GE_OADR2,
    &HW_GE_UNK300,
    &HW_GE_INTERRUPT_TYPE1,
    &HW_GE_INTERRUPT_TYPE2,
    &HW_GE_INTERRUPT_TYPE3,
    &HW_GE_INTERRUPT_TYPE4,
    &HW_GE_EDRAM_ENABLED_SIZE,
    &HW_GE_GEOMETRY_CLOCK,
    &HW_GE_EDRAM_REFRESH_UNK1,
    &HW_GE_EDRAM_UNK10,
    &HW_GE_EDRAM_REFRESH_UNK2,
    &HW_GE_EDRAM_REFRESH_UNK3,
    &HW_GE_EDRAM_UNK40,
    &HW_GE_EDRAM_UNK50,
    &HW_GE_EDRAM_UNK60,
    &HW_GE_EDRAM_ADDR_TRANS_DISABLE,
    &HW_GE_EDRAM_ADDR_TRANS_VALUE,
    &HW_GE_EDRAM_UNK90,
    &HW_GE_EDRAM_UNKA0,
};

// 6754
SadrUpdate sadrupdate_bypass = { "ULJM05086", (void *)0x08992414 };

// 675C
SceGeMatrix mtxtbl[] = {
    {0x2A, 0x00, 0x2B, 0x0C},
    {0x2A, 0x0C, 0x2B, 0x0C},
    {0x2A, 0x18, 0x2B, 0x0C},
    {0x2A, 0x24, 0x2B, 0x0C},
    {0x2A, 0x30, 0x2B, 0x0C},
    {0x2A, 0x3C, 0x2B, 0x0C},
    {0x2A, 0x48, 0x2B, 0x0C},
    {0x2A, 0x54, 0x2B, 0x0C},
    {0x3A, 0x00, 0x3B, 0x0C},
    {0x3C, 0x00, 0x3D, 0x0C},
    {0x3E, 0x00, 0x3F, 0x10},
    {0x40, 0x00, 0x41, 0x0C}
};

// 678C
int stopCmd[] = { GE_MAKE_OP(SCE_GE_CMD_FINISH, 0), GE_MAKE_OP(SCE_GE_CMD_END, 0) };

// 6840
SceSysEventHandler g_GeSysEv =
    { 64, "SceGe", 0x00FFFF00, _sceGeSysEventHandler, 0, 0, NULL, {0, 0, 0, 0,
                                                                   0, 0, 0, 0,
                                                                   0}
};

// 6880
char g_szTbp[] = "RTBP0";

// 6890
SceGeLogHandler g_GeLogHandler;

/* The GE context, saved on reset & suspend (and restored on resume) */
SceGeContext _aw_ctx; // 68C0

// 70C0
int g_uiEdramHwSize;

// 70C4
int g_uiEdramSize;

// 70C8
u16 g_edramAddrTrans;

// 70CC
SceGeQueue g_AwQueue;

// 7500
SceGeQueueSuspendInfo g_GeSuspend;

// 7520
SceGeBpCtrl g_GeDeciBreak;

// 75D0
short g_cbhook;

// 7600
int *g_cmdList;

// 7604
int g_dlMask;

// 7608
void *g_deci2p;

// 7640
SceGeDisplayList g_displayLists[64];

/******************************/

/*
 * Reset the GE. (Only used in sceGeBreak().)
 */
int _sceGeReset()
{
    // Start the GE reset
    int oldIntr = sceKernelCpuSuspendIntr();
    sceSysregAwRegABusClockEnable();
    pspSync();
    HW_GE_EDRAM_UNK10 = 2;
    HW_GE_RESET = 1;
    // 0144
    while ((HW_GE_RESET & 1) != 0)
        ;
    // Save the current GE context
    sceGeSaveContext(&_aw_ctx);
    _aw_ctx.ctx[16] = HW_GE_GEOMETRY_CLOCK;
    sceSysregAwResetEnable();
    sceSysregAwResetDisable();
    // Restart the GE, reinitializing registers
    HW_GE_EDRAM_UNK10 = 0;
    HW_GE_EXEC = 0;
    HW_GE_LISTADDR = 0;
    HW_GE_STALLADDR = 0;
    HW_GE_RADR1 = 0;
    HW_GE_RADR2 = 0;
    HW_GE_VADR = 0;
    HW_GE_IADR = 0;
    HW_GE_OADR = 0;
    HW_GE_OADR1 = 0;
    HW_GE_OADR2 = 0;
    HW_GE_INTERRUPT_TYPE4 = HW_GE_INTERRUPT_TYPE1;
    HW_GE_INTERRUPT_TYPE3 = HW_GE_INTERRUPT_TYPE2;
    HW_GE_INTERRUPT_TYPE2 = HW_GE_INTSIG | HW_GE_INTEND | HW_GE_INTFIN;
    HW_GE_GEOMETRY_CLOCK = _aw_ctx.ctx[16];
    sceSysregSetMasterPriv(64, 1);
    // Restore the GE context
    sceGeRestoreContext(&_aw_ctx);
    sceSysregSetMasterPriv(64, 0);
    sceSysregAwRegABusClockDisable();
    sceKernelCpuResumeIntr(oldIntr);
    return 0;
}

/*
 * Initialize the GE. (Used in module bootstart.)
 */
int sceGeInit()
{
    sceSysregAwResetDisable();
    // Enable the various clocks used by the GE.
    sceSysregAwRegABusClockEnable();
    sceSysregAwRegBBusClockEnable();
    sceSysregAwEdramBusClockEnable();
    // Generate some unique mask used for the display list IDs
    g_dlMask = (HW_GE_CMD(SCE_GE_CMD_VADR) ^ HW_GE_CMD(SCE_GE_CMD_PRIM)
              ^ HW_GE_CMD(SCE_GE_CMD_BEZIER) ^ HW_GE_CMD(SCE_GE_CMD_SPLINE)
              ^ HW_GE_CMD(SCE_GE_CMD_WORLDD)) | 0x80000000;
    // Initialize the GE EDRAM
    sceGeEdramInit();
    // Reset registers & run the initialization display list
    HW_GE_EXEC = 0;
    u32 *dlist = &_aw_ctx.ctx[17];
    HW_GE_LISTADDR = 0;
    HW_GE_STALLADDR = 0;
    u32 *curDl = dlist;
    HW_GE_RADR1 = 0;
    HW_GE_RADR2 = 0;
    HW_GE_VADR = 0;
    HW_GE_IADR = 0;
    HW_GE_OADR = 0;
    HW_GE_OADR1 = 0;
    HW_GE_OADR2 = 0;
    HW_GE_INTERRUPT_TYPE4 = HW_GE_INTERRUPT_TYPE1;
    HW_GE_INTERRUPT_TYPE3 = HW_GE_INTERRUPT_TYPE2;
    HW_GE_INTERRUPT_TYPE2 = HW_GE_INTSIG | HW_GE_INTEND | HW_GE_INTFIN;
    // Reset all the registers which are flagged as being registers which can be saved
    int i;
    for (i = 0; i < 256; i++) {
        if (((save_regs[i / 8] >> (i & 7)) & 1) != 0)
            *(curDl++) = i << 24;
        // 03A0
    }

    // Reset all the bone, world, etc. matrices
    *(curDl++) = GE_MAKE_OP(SCE_GE_CMD_BONEN, 0);
    // 03BC
    for (i = 0; i < 96; i++)
        *(curDl++) = GE_MAKE_OP(SCE_GE_CMD_BONED, 0);

    *(curDl++) = GE_MAKE_OP(SCE_GE_CMD_WORLDN, 0);
    // 03E0
    for (i = 0; i < 12; i++)
        *(curDl++) = GE_MAKE_OP(SCE_GE_CMD_WORLDD, 0);

    *(curDl++) = GE_MAKE_OP(SCE_GE_CMD_VIEWN, 0);
    // 0404
    for (i = 0; i < 12; i++)
        *(curDl++) = GE_MAKE_OP(SCE_GE_CMD_VIEWD, 0);

    *(curDl++) = GE_MAKE_OP(SCE_GE_CMD_PROJN, 0);
    // 0428
    for (i = 0; i < 16; i++)
        *(curDl++) = GE_MAKE_OP(SCE_GE_CMD_PROJD, 0);

    *(curDl++) = GE_MAKE_OP(SCE_GE_CMD_TGENN, 0);
    // 044C
    for (i = 0; i < 12; i++)
        *(curDl++) = GE_MAKE_OP(SCE_GE_CMD_TGEND, 0);
    *(curDl + 0) = GE_MAKE_OP(SCE_GE_CMD_PRIM, 0);
    *(curDl + 1) = GE_MAKE_OP(SCE_GE_CMD_END, 0);
    // Execute the initialization display list
    sceKernelDcacheWritebackInvalidateRange(dlist, 1980);
    HW_GE_INTERRUPT_TYPE3 = HW_GE_INTERRUPT_TYPE2;
    HW_GE_LISTADDR = (int)UCACHED(dlist);
    HW_GE_STALLADDR = 0;
    sceSysregSetMasterPriv(64, 1);
    HW_GE_EXEC = 1;

    // 04B8
    while ((HW_GE_EXEC & 1) != 0)
        ;
    sceSysregSetMasterPriv(64, 0);
    // Re-disable execution for now
    HW_GE_EXEC = 0;
    HW_GE_LISTADDR = 0;
    HW_GE_STALLADDR = 0;
    HW_GE_INTERRUPT_TYPE4 = HW_GE_INTERRUPT_TYPE1;
    HW_GE_INTERRUPT_TYPE2 = HW_GE_INTSIG | HW_GE_INTEND | HW_GE_INTFIN;

    // Initialize the GE interrupt handler
    sceKernelRegisterIntrHandler(SCE_GE_INT, 1, _sceGeInterrupt, 0, &g_GeIntrOpt);

    // 0534
    for (i = 0; i < 16; i++)
        sceKernelSetUserModeIntrHanlerAcceptable(SCE_GE_INT, i, 1);

    sceKernelEnableIntr(SCE_GE_INT);
    sceSysregAwRegABusClockDisable();

    // Register the sysevent handler (for suspend & resume)
    sceKernelRegisterSysEventHandler(&g_GeSysEv);
    // Init the display list queue
    _sceGeQueueInit();
    // Run/enable callbacks used to patch Genso Suikoden I&II
    SceKernelUsersystemLibWork *libWork = sceKernelGetUsersystemLibWork();
    // Ensure callback3 is ran after usersystemlib is loaded
    if (libWork->cmdList == NULL) {
        // 05EC
        sceKernelSetInitCallback(_sceGeInitCallback3, 3, 0);
    } else
        _sceGeInitCallback3(0, 0, 0);

    // 059C
    sceKernelSetInitCallback(_sceGeInitCallback4, 4, 0);
    // Register deci2p (debug) operations
    void *ret = (void *)sceKernelDeci2pReferOperations();
    g_deci2p = ret;
    if (ret != NULL) {
        // 05D4
        int (*func) (SceKernelDeci2Ops *) = (void *)*(int *)(ret + 28);
        func(&g_Deci2Ops);
    }
    return 0;
}

/*
 * Stop the GE. (Used by rebootBefore only.)
 */
int sceGeEnd()
{
    _sceGeQueueEnd();
    // Unregister the system event handler
    sceKernelUnregisterSysEventHandler(&g_GeSysEv);
    // Disable the GE interrupt & subinterrupts
    sceKernelDisableIntr(SCE_GE_INT);
    sceKernelReleaseIntrHandler(SCE_GE_INT);
    // Enable the ABus clock and stop execution
    sceSysregAwRegABusClockEnable();
    HW_GE_EXEC = 0;
    // 0640
    while ((HW_GE_EXEC & 1) != 0)
        ;

    if (g_cmdList != NULL) {
        // Execute a list which just does two empty calls (maybe to ensure we don't stop in the middle of a call?)
        int *cmdOut = &g_cmdList[16];
        cmdOut[0] = GE_MAKE_OP(SCE_GE_CMD_BASE, (((((int)&g_cmdList[20]) >> 24) & 0xF) << 16));
        cmdOut[1] = GE_MAKE_OP(SCE_GE_CMD_OFFSET, 0);
        cmdOut[2] = GE_MAKE_OP(SCE_GE_CMD_CALL, (int)&g_cmdList[20]);
        cmdOut[3] = GE_MAKE_OP(SCE_GE_CMD_END, 0);
        cmdOut[4] = GE_MAKE_OP(SCE_GE_CMD_CALL, (int)&g_cmdList[22]);
        cmdOut[5] = GE_MAKE_OP(SCE_GE_CMD_RET, 0);
        cmdOut[6] = GE_MAKE_OP(SCE_GE_CMD_RET, 0);
        HW_GE_OADR = 0;
        HW_GE_LISTADDR = (int)UCACHED(cmdOut);
        HW_GE_STALLADDR = 0;
        pspSync();
        // Wait for the list to be executed
        HW_GE_EXEC = 1;
        // 06E0
        while ((HW_GE_EXEC & 1) != 0)
            ;
    }
    // Disable the ABus clock again
    sceSysregAwRegABusClockDisable();
    return 0;
}

// 070C
// Part of the patching stuff for Genso Suikoden I&II : prepare stuff inside usersystemlib
int _sceGeInitCallback3(void *arg0 __attribute__ ((unused)), s32 arg1 __attribute__ ((unused)), void *arg2 __attribute__ ((unused)))
{
    SceKernelUsersystemLibWork *libWork = sceKernelGetUsersystemLibWork();
    if (libWork->cmdList != NULL) {
        // Put an "empty" display list in libWork->cmdList
        s32 *uncachedDlist = UUNCACHED(libWork->cmdList);
        uncachedDlist[0] = GE_MAKE_OP(SCE_GE_CMD_FINISH, 0);
        uncachedDlist[1] = GE_MAKE_OP(SCE_GE_CMD_END, 0);
        g_cmdList = uncachedDlist;
        _sceGeQueueInitCallback();
    }
    return 0;
}

// Part of the patching stuff for Genso Suikoden I&II : patch the game itself to use sceGeListUpdateStallAddr_lazy
int _sceGeInitCallback4()
{
    SceKernelGameInfo *info = sceKernelGetGameInfo();
    if (info != NULL) {
        u32 syscOp = ALLEGREX_MAKE_SYSCALL(sceKernelQuerySystemCall((void*)sceGeListUpdateStallAddr));
        int oldIntr = sceKernelCpuSuspendIntr();
        if (strcmp(info->gameId, sadrupdate_bypass.name) == 0) {
            u32 *ptr = sadrupdate_bypass.ptr;
            // Replace the syscall to sceGeListUpdateStallAddr by a jump to sceGeListUpdateStallAddr_lazy in usersystemlib
            if (ptr[0] == ALLEGREX_MAKE_JR_RA && ptr[1] == syscOp) {
                // 0804
                ptr[0] = ALLEGREX_MAKE_J(sceKernelGetUsersystemLibWork()->sceGeListUpdateStallAddr_lazy);
                ptr[1] = ALLEGREX_MAKE_NOP;
                pspCache(0x1A, ptr + 0);
                pspCache(0x1A, ptr + 1);
                pspCache(0x08, ptr + 0);
                pspCache(0x08, ptr + 1);
            }
        }
        // 07DC
        sceKernelCpuResumeIntr(oldIntr);
    }
    return 0;
}

/*
 * Read an internal GE register. See SceGeReg's definition for the list.
 */
int sceGeGetReg(SceGeReg regId)
{
    if (regId >= 32)
        return SCE_ERROR_INVALID_INDEX;
    int oldK1 = pspShiftK1();
    int oldIntr = sceKernelCpuSuspendIntr();
    int wasEnabled = sceSysregAwRegABusClockEnable();
    int val = *(int*)g_pAwRegAdr[regId];
    if (!wasEnabled) {
        // 08C8
        sceSysregAwRegABusClockDisable();
    }
    // 089C
    sceKernelCpuResumeIntr(oldIntr);
    pspSetK1(oldK1);
    return val;
}

/*
 * Write to an internal GE register. See SceGeReg's definition for the list.
 */
int sceGeSetReg(SceGeReg regId, u32 value)
{
    if (regId >= 32)
        return SCE_ERROR_INVALID_INDEX;
    // These registers being addresses, they need to be 4-aligned.
    if (regId >= SCE_GE_REG_LISTADDR && regId <= SCE_GE_REG_OADR2 && (value & 3) != 0)
        return SCE_ERROR_INVALID_VALUE;

    // 092C
    int oldK1 = pspShiftK1();
    int oldIntr = sceKernelCpuSuspendIntr();
    int wasEnabled = sceSysregAwRegABusClockEnable();
    int ret;
    if ((HW_GE_EXEC & 1) == 0) {
        // Call particular functions for RADR1 and RADR2 (return address from calls)
        if (regId == SCE_GE_REG_RADR1) {
            // 09EC
            _sceGeSetRegRadr1(value);
        } else if (regId == SCE_GE_REG_RADR2) {
            // 09DC
            _sceGeSetRegRadr2(value);
        }
        // 0974
        *(int*)g_pAwRegAdr[regId] = value;
        ret = 0;
    } else {
        ret = SCE_ERROR_BUSY;
    }
    // 098C
    if (!wasEnabled) {
        // 09CC
        sceSysregAwRegABusClockDisable();
    }
    // 0994
    sceKernelCpuResumeIntr(oldIntr);
    pspSetK1(oldK1);
    return ret;
}

/*
 * Get the value of a command (ie the last command ran for that operation, with its arguments)
 */
int sceGeGetCmd(u32 cmdOff)
{
    if (cmdOff >= 0xFF)
        return SCE_ERROR_INVALID_INDEX;
    int oldK1 = pspShiftK1();
    int oldIntr = sceKernelCpuSuspendIntr();
    int wasEnabled = sceSysregAwRegABusClockEnable();
    int cmd = HW_GE_CMD(cmdOff);
    if (!wasEnabled) {
        // 0A7C
        sceSysregAwRegABusClockDisable();
    }
    // 0A50
    sceKernelCpuResumeIntr(oldIntr);
    pspSetK1(oldK1);
    return cmd;
}

/*
 * Set the value of a command.
 * It is not as easy as reading it, as we need to run a display list setting it,
 * and there is special care to take for some flow-controlling commands.
 */
int sceGeSetCmd(u32 cmdOff, u32 cmd)
{
    if (cmdOff >= 0xFF)
        return SCE_ERROR_INVALID_INDEX;
    int oldK1 = pspShiftK1();
    int oldIntr = sceKernelCpuSuspendIntr();
    int wasEnabled = sceSysregAwRegABusClockEnable();
    int ret = 0;
    // Check if the GE is already busy
    if ((HW_GE_EXEC & 1) != 0) {
        ret = SCE_ERROR_BUSY;
        goto end;
    }
    int oldState = HW_GE_EXEC;
    int listAddr = HW_GE_LISTADDR;
    // For all the branching/jumping functions
    if (cmdOff == SCE_GE_CMD_JUMP || cmdOff == SCE_GE_CMD_BJUMP || cmdOff == SCE_GE_CMD_CALL) {
        // Check if the destination address is valid
        int addr = (((HW_GE_CMD(SCE_GE_CMD_BASE) << 8) & 0xFF000000) | (cmd & 0x00FFFFFF)) + HW_GE_OADR;
        if (!GE_VALID_ADDR(addr)) {
            // 0E68 dup
            ret = SCE_ERROR_INVALID_POINTER;
            goto end;
        } else {
            // 0B88
            if (cmdOff == SCE_GE_CMD_BJUMP) {
                // 0E50
                // If the condition is true, branch, otherwise just remove the corresponding bit for the register
                if ((oldState & 2) == 0)
                    listAddr = addr;
                else {
                    listAddr += 4;
                    oldState &= 0xFFFFFFFD;
                }
            } else if (cmdOff == SCE_GE_CMD_CALL) { // 0DE0
                if ((oldState & 0x200) != 0) { // Double-nested call
                    // 0E48 dup
                    ret = SCE_ERROR_NOT_IMPLEMENTED;
                    goto end;
                } else if ((oldState & 0x100) == 0) { // Not a nested call
                    // 0E24
                    // Set the return address, jump, set the callee flag and set the destination address
                    _sceGeSetRegRadr1(listAddr + 4);
                    listAddr = addr;
                    oldState |= 0x100;
                    // Save caller's OADR
                    HW_GE_OADR1 = HW_GE_OADR;
                } else { // First-level nested call
                    // Same as before but for a nested call
                    _sceGeSetRegRadr2(listAddr + 4);
                    oldState = (oldState & 0xFFFFFEFF) | 0x200;
                    listAddr = addr;
                    HW_GE_OADR2 = HW_GE_OADR;
                }
            } else if (cmdOff == SCE_GE_CMD_JUMP) {
                // Just jump directly
                listAddr = addr;
            }
            // 0BA4 dup
            // Nothing to execute in the display list as we set all the registers appropriately
            cmd = HW_GE_CMD(SCE_GE_CMD_NOP);
            cmdOff = 0;
        }
    } else if (cmdOff == SCE_GE_CMD_RET) {
        // 13E0
        if ((oldState & 0x200) == 0) {
            // 1410
            if ((oldState & 0x100) == 0) {
                // 0E48 dup
                // Cannot return from a non-called code!
                ret = SCE_ERROR_NOT_IMPLEMENTED;
                goto end;
            } else {
                // Jump to the return address and restore OADR, and set to non-called state
                listAddr = HW_GE_RADR1;
                // 1404 dup
                HW_GE_OADR = HW_GE_OADR1;
                oldState &= 0xFFFFFEFF;
            }
        } else { // Nested call
            // Jump to the return address and restore OADR, and set to non-nested call state
            listAddr = HW_GE_RADR2;
            // 1404 dup
            HW_GE_OADR = HW_GE_OADR2;
            oldState = (oldState & 0xFFFFFDFF) | 0x100;
        }
        // 0BA4 dup
        cmd = HW_GE_CMD(SCE_GE_CMD_NOP);
        cmdOff = 0;
    } else if (cmdOff == SCE_GE_CMD_ORIGIN) {
        // 13C8
        // Set the origin address
        HW_GE_OADR = listAddr;
        cmdOff = 0;
        cmd = HW_GE_CMD(SCE_GE_CMD_NOP);
    } else if (cmdOff == SCE_GE_CMD_PRIM || cmdOff == SCE_GE_CMD_BEZIER || cmdOff == SCE_GE_CMD_SPLINE) {
        // Check if the vertex address is valid
        int addr = HW_GE_VADR;
        if (!GE_VALID_ADDR(addr)) {
            ret = SCE_ERROR_INVALID_POINTER;
            goto end;
        }

        // 0F14
        // Check if the index address, if set, is valid
        if (((HW_GE_CMD(SCE_GE_CMD_VTYPE) >> 11) & 3) != 0)
        {
            int addr = HW_GE_IADR;
            if (!GE_VALID_ADDR(addr)) {
                ret = SCE_ERROR_INVALID_POINTER;
                goto end;
            }
        }
        // 0F70
        // Check if the texture address, if set, is valid
        if ((HW_GE_CMD(SCE_GE_CMD_TME) & 1) != 0) {
            int count = (HW_GE_CMD(SCE_GE_CMD_TMODE) >> 16) & 7;
            // 0FC0
            int i;
            for (i = 0; i <= count; i++) {
                int addr = ((HW_GE_CMD(SCE_GE_CMD_TBW0 + i) & 0x00FF0000) << 8) | (HW_GE_CMD(SCE_GE_CMD_TBP0 + i) & 0x00FFFFFF);
                if (!GE_VALID_ADDR(addr)) {
                    // 1028
                    ret = SCE_ERROR_INVALID_POINTER;
                    goto end;
                }
                // 1030
            }
        }
    } else if (cmdOff == SCE_GE_CMD_AP2 && ((cmd >> 21) & 1) != 0) {
        // 12E4
        // For AP2, if texture(s) are set, check if the addresses are valid
        int count = (HW_GE_CMD(SCE_GE_CMD_TMODE) >> 16) & 7;
        // 1328
        int i;
        for (i = 0; i <= count; i++) {
            int addr = ((HW_GE_CMD(SCE_GE_CMD_TBW0 + i) & 0x00FF0000) << 8) | (HW_GE_CMD(SCE_GE_CMD_TBP0 + i) & 0x00FFFFFF);
            if (!GE_VALID_ADDR(addr)) {
                ret = SCE_ERROR_INVALID_POINTER;   // 1390
                goto end;
            }
            // 1398
        }
    } else if (cmdOff == SCE_GE_CMD_CLOAD) {
        // 1240
        // Check if the CLUT address is valid
        int addr = ((HW_GE_CMD(SCE_GE_CMD_CBW) << 8) & 0xFF000000) | (HW_GE_CMD(SCE_GE_CMD_CBP) & 0x00FFFFFF);
        if (!GE_VALID_ADDR(addr)) {
            // 0E68 dup
            ret = SCE_ERROR_INVALID_POINTER;
            goto end;
        }
    } else if (cmdOff == SCE_GE_CMD_XSTART) {
        // Check if the transfer addresses are valid
        int addr1 = ((HW_GE_CMD(SCE_GE_CMD_XBW1) & 0x00FF0000) << 8) | (HW_GE_CMD(SCE_GE_CMD_XBP1) & 0x00FFFFFF);
        int addr2 = ((HW_GE_CMD(SCE_GE_CMD_XBW2) & 0x00FF0000) << 8) | (HW_GE_CMD(SCE_GE_CMD_XBP2) & 0x00FFFFFF);
        if (!GE_VALID_ADDR(addr1) || !GE_VALID_ADDR(addr2)) {
            // 11B8
            ret = SCE_ERROR_INVALID_POINTER;
            goto end;
        }
    }
    // 0BB0
    int cmdBuf[32];
    // 0BB8
    int stallAddr = HW_GE_STALLADDR;
    // Align the command buffer used as a display list
    int *dl = (int *)(((int)cmdBuf | 0x3F) + 1);
    int prevStatus = HW_GE_INTERRUPT_TYPE1;
    // For FINISH & END, just run a FINISH/END sequence with the given command
    if (cmdOff == SCE_GE_CMD_FINISH) {
        // 0DC0
        dl[0] = GE_MAKE_OP(SCE_GE_CMD_FINISH, cmd);
        dl[1] = HW_GE_CMD(SCE_GE_CMD_END);
    } else if (cmdOff == SCE_GE_CMD_END) {
        // 0DA0
        dl[0] = HW_GE_CMD(SCE_GE_CMD_FINISH);
        dl[1] = GE_MAKE_OP(SCE_GE_CMD_END, cmd);
    } else if (cmdOff == SCE_GE_CMD_BASE) {
        // 0D78
        // For BASE, just run the BASE command and FINISH/END
        dl[0] = GE_MAKE_OP(SCE_GE_CMD_BASE, cmd);
        dl[1] = HW_GE_CMD(SCE_GE_CMD_FINISH);
        dl[2] = HW_GE_CMD(SCE_GE_CMD_END);
    } else {
        // For the rest, set the BASE to 0x04000000, run our command, then restore BASE and FINISH/END
        dl[0] = GE_MAKE_OP(SCE_GE_CMD_BASE, 0x00400000 | (HW_GE_CMD(SCE_GE_CMD_BASE) & 0x00FF0000));
        dl[1] = GE_MAKE_OP(cmdOff, cmd);
        dl[2] = HW_GE_CMD(SCE_GE_CMD_BASE);
        dl[3] = HW_GE_CMD(SCE_GE_CMD_FINISH);
        dl[4] = HW_GE_CMD(SCE_GE_CMD_END);
    }
    // 0C44
    pspCache(0x1A, dl);
    if ((pspCop0StateGet(24) & 1) != 0) {
        pspSync();
        pspL2CacheWriteback0(dl, 1);
    }
    // 0C88
    sceSysregSetMasterPriv(64, 1);
    HW_GE_INTERRUPT_TYPE4 = HW_GE_INTFIN;
    HW_GE_LISTADDR = (int)UCACHED(dl);
    HW_GE_STALLADDR = 0;
    pspSync();
    HW_GE_EXEC = oldState | 1;
    // 0CC0
    while ((HW_GE_EXEC & 1) != 0)
        ;
    // 0CD4
    while ((HW_GE_INTERRUPT_TYPE1 & HW_GE_INTFIN) == 0)
        ;
    HW_GE_LISTADDR = listAddr;
    HW_GE_STALLADDR = stallAddr;
    HW_GE_INTERRUPT_TYPE4 = HW_GE_INTERRUPT_TYPE1 ^ prevStatus;
    sceSysregSetMasterPriv(64, 0);

end:
    // 0D1C
    if (!wasEnabled) {
        // 0D68
        sceSysregAwRegABusClockDisable();
    }
    // 0D24
    sceKernelCpuResumeIntr(oldIntr);
    pspSetK1(oldK1);
    return ret;
}

int sceGeGetMtx(int id, int *mtx)
{
    if (id < 0 || id >= 12)
        return SCE_ERROR_INVALID_INDEX;
    int oldK1 = pspShiftK1();
    if (!pspK1PtrOk(mtx)) {
        // 1588
        pspSetK1(oldK1);
        return SCE_ERROR_PRIV_REQUIRED;
    }
    int oldIntr = sceKernelCpuSuspendIntr();
    int wasEnabled = sceSysregAwRegABusClockEnable();
    if (id == SCE_GE_MTX_PROJ) {
        // 1554, 1560
        // 1560
        int i;
        for (i = 0; i < 16; i++) {
            mtx[i] = HW_GE_PROJS[i];
        }
    } else if (id == SCE_GE_MTX_TGEN) {
        // 152C
        int i;
        for (i = 0; i < 12; i++) {
            mtx[i] = HW_GE_TGENS[i];
        }
    } else {
        // 14B0
        int i;
        for (i = 0; i < 12; i++) {
            mtx[i] = HW_GE_BONE(id)[i];
        }
    }
    // 14C8
    if (!wasEnabled) {
        // 1500
        sceSysregAwRegABusClockDisable();
    }
    // 14D0
    sceKernelCpuResumeIntr(oldIntr);
    pspSetK1(oldK1);
    return 0;
}

int sceGeSetMtx(int id, int *mtx)
{
    int ret;
    if (id < 0 || id >= 12)
        return SCE_ERROR_INVALID_INDEX;
    int oldK1 = pspShiftK1();
    if (!pspK1PtrOk(mtx)) {
        // 16B8
        pspSetK1(oldK1);
        return SCE_ERROR_PRIV_REQUIRED;
    }
    int oldIntr = sceKernelCpuSuspendIntr();
    SceGeMatrix *curMtx = &mtxtbl[id];
    char oldCmd = sceGeGetCmd(curMtx->reg1);
    ret = sceGeSetCmd(curMtx->reg1, curMtx->cmd);
    if (ret >= 0) {
        // 1644
        int i;
        for (i = 0; i < curMtx->size; i++)
            sceGeSetCmd(curMtx->reg2, mtx[i]);

        // 165C
        sceGeSetCmd(curMtx->reg1, oldCmd);
        ret = 0;
    }
    // 1674
    sceKernelCpuResumeIntr(oldIntr);
    pspSetK1(oldK1);
    return ret;
}

int sceGeSaveContext(SceGeContext * ctx)
{
    if (((int)ctx & 3) != 0)
        return SCE_ERROR_INVALID_POINTER;
    int oldK1 = pspShiftK1();
    if (!pspK1StaBufOk(ctx, 2048)) {
        // 1AA0
        pspSetK1(oldK1);
        return SCE_ERROR_PRIV_REQUIRED;
    }
    int oldIntr = sceKernelCpuSuspendIntr();
    int aBusWasEnabled = sceSysregAwRegABusClockEnable();
    if ((HW_GE_EXEC & 1) != 0) {
        // 1A8C
        sceKernelCpuResumeIntr(oldIntr);
        pspSetK1(oldK1);
        return -1;
    }
    u32 *curCmd = &ctx->ctx[17];
    ctx->ctx[0] = HW_GE_EXEC;
    ctx->ctx[1] = HW_GE_LISTADDR;
    ctx->ctx[2] = HW_GE_STALLADDR;
    ctx->ctx[3] = HW_GE_RADR1;
    ctx->ctx[4] = HW_GE_RADR2;
    ctx->ctx[5] = HW_GE_VADR;
    ctx->ctx[6] = HW_GE_IADR;
    ctx->ctx[7] = HW_GE_OADR;
    ctx->ctx[8] = HW_GE_OADR1;
    ctx->ctx[9] = HW_GE_OADR2;

    vs32 *cmds = HWPTR(0xBD400800);
    // 17C8
    int i;
    for (i = 0; i < 256; i++) {
        if (((save_regs[i / 8] >> (i & 7)) & 1) != 0)
            *(curCmd++) = *cmds;
        // 1804
        cmds++;
    }
    int addr = (HW_GE_CMD(SCE_GE_CMD_CBP) & 0x00FFFFFF) | ((HW_GE_CMD(SCE_GE_CMD_CBW) << 8) & 0xFF000000);
    if (GE_VALID_ADDR(addr)) {
        // 1868
        *(curCmd++) = HW_GE_CMD(SCE_GE_CMD_CLOAD);
    }
    // 187C
    *(curCmd++) = GE_MAKE_OP(SCE_GE_CMD_BONEN, 0);
    // 1894
    for (i = 0; i < 96; i++)
        *(curCmd++) = GE_MAKE_OP(SCE_GE_CMD_BONED, HW_GE_BONES[i]);

    *(curCmd++) = GE_MAKE_OP(SCE_GE_CMD_WORLDN, 0);
    for (i = 0; i < 12; i++)
        *(curCmd++) = GE_MAKE_OP(SCE_GE_CMD_WORLDD, HW_GE_WORLDS[i]);

    *(curCmd++) = GE_MAKE_OP(SCE_GE_CMD_VIEWN, 0);
    // 190C
    for (i = 0; i < 12; i++)
        *(curCmd++) = GE_MAKE_OP(SCE_GE_CMD_VIEWD, HW_GE_VIEWS[i]);

    *(curCmd++) = GE_MAKE_OP(SCE_GE_CMD_PROJN, 0);
    // 1948
    for (i = 0; i < 16; i++)
        *(curCmd++) = GE_MAKE_OP(SCE_GE_CMD_PROJD, HW_GE_PROJS[i]);

    *(curCmd++) = GE_MAKE_OP(SCE_GE_CMD_TGENN, 0);
    // 1984
    for (i = 0; i < 12; i++)
        *(curCmd++) = GE_MAKE_OP(SCE_GE_CMD_TGEND, HW_GE_TGENS[i]);

    *(curCmd++) = HW_GE_CMD(SCE_GE_CMD_BONEN);
    *(curCmd++) = HW_GE_CMD(SCE_GE_CMD_WORLDN);
    *(curCmd++) = HW_GE_CMD(SCE_GE_CMD_VIEWN);
    *(curCmd++) = HW_GE_CMD(SCE_GE_CMD_PROJN);
    *(curCmd++) = HW_GE_CMD(SCE_GE_CMD_TGENN);
    *(curCmd++) = GE_MAKE_OP(SCE_GE_CMD_END, 0);
    sceKernelDcacheWritebackInvalidateRange(&ctx->ctx[17], 1980);
    if (!aBusWasEnabled)
        sceSysregAwRegABusClockDisable();   // 1A3C
    // 1A0C
    sceKernelCpuResumeIntr(oldIntr);
    pspSetK1(oldK1);
    return 0;
}

int sceGeRestoreContext(SceGeContext * ctx)
{
    int ret = 0;
    if (((int)ctx & 3) != 0)
        return SCE_ERROR_INVALID_POINTER;
    int oldK1 = pspShiftK1();
    if (!pspK1StaBufOk(ctx, 2048)) {
        // 1C80
        pspSetK1(oldK1);
        return SCE_ERROR_PRIV_REQUIRED;
    }
    int oldIntr = sceKernelCpuSuspendIntr();
    int state = sceSysregAwRegABusClockEnable();
    if ((HW_GE_EXEC & 1) != 0) {
        // 1C68
        sceKernelCpuResumeIntr(oldIntr);
        pspSetK1(oldK1);
        return SCE_ERROR_BUSY;
    }
    int old304 = HW_GE_INTERRUPT_TYPE1;
    int old308 = HW_GE_INTERRUPT_TYPE2;
    HW_GE_INTERRUPT_TYPE3 = old308;
    HW_GE_LISTADDR = (int)UCACHED(&ctx->ctx[17]);
    HW_GE_STALLADDR = 0;
    HW_GE_EXEC = ctx->ctx[0] | 1;
    // 1B64
    while ((HW_GE_EXEC & 1) != 0)
        ;
    int n304 = HW_GE_INTERRUPT_TYPE1;
    HW_GE_INTERRUPT_TYPE4 = (old304 ^ HW_GE_INTERRUPT_TYPE1) & ~(HW_GE_INTFIN | HW_GE_INTSIG);
    HW_GE_INTERRUPT_TYPE2 = old308;
    if ((n304 & 8) != 0)
        ret = -1;
    HW_GE_LISTADDR = ctx->ctx[1];
    HW_GE_STALLADDR = ctx->ctx[2];
    HW_GE_VADR = ctx->ctx[5];
    HW_GE_IADR = ctx->ctx[6];
    HW_GE_OADR = ctx->ctx[7];
    HW_GE_OADR1 = ctx->ctx[8];
    HW_GE_OADR2 = ctx->ctx[9];
    _sceGeSetInternalReg(SCE_GE_INTERNAL_REG_RADR1 | SCE_GE_INTERNAL_REG_RADR2, 0, ctx->ctx[3], ctx->ctx[4]);
    pspSync();
    if (state == 0) {
        // 1C58
        sceSysregAwRegABusClockDisable();
    }
    // 1C24
    sceKernelCpuResumeIntr(oldIntr);
    pspSetK1(oldK1);
    return ret;
}

int _sceGeSetRegRadr1(int arg0)
{
    return _sceGeSetInternalReg(SCE_GE_INTERNAL_REG_RADR1, 0, arg0, 0);
}

int _sceGeSetRegRadr2(int arg0)
{
    return _sceGeSetInternalReg(SCE_GE_INTERNAL_REG_RADR2, 0, 0, arg0);
}

int _sceGeSetInternalReg(int type, int base, int radr1, int radr2)
{
    int *cmdList = g_cmdList;
    if (cmdList == NULL)
        return 0;
    int oldIntr = sceKernelCpuSuspendIntr();
    int old304 = HW_GE_INTERRUPT_TYPE1;
    int old100 = HW_GE_EXEC;
    int old108 = HW_GE_LISTADDR;
    int old10C = HW_GE_STALLADDR;
    int old120 = HW_GE_OADR;
    int old124 = HW_GE_OADR1;
    int old128 = HW_GE_OADR2;
    if ((type & SCE_GE_INTERNAL_REG_BASE_ADDR) == 0)
        base = HW_GE_CMD(SCE_GE_CMD_BASE);
    // 1D74
    cmdList[0] = GE_MAKE_OP(SCE_GE_CMD_FINISH, 0);
    cmdList[1] = GE_MAKE_OP(SCE_GE_CMD_END, 0);
    cmdList += 2;
    cmdList[0] = GE_MAKE_OP(SCE_GE_CMD_END, 0);
    pspSync();
    HW_GE_STALLADDR = 0;
    int *uncachedCmdList = UCACHED(cmdList);
    if ((type & SCE_GE_INTERNAL_REG_RADR1) && radr2 != 0) {
        int *uncachedNewCmdList = UCACHED(radr2 - 4);
        u32 relAddr = (u32) (uncachedCmdList - old120);
        int oldCmd = uncachedNewCmdList[0];
        uncachedNewCmdList[0] = GE_MAKE_OP(SCE_GE_CMD_CALL, relAddr & 0x00FFFFFF);
        pspCache(0x1A, uncachedNewCmdList);
        if ((pspCop0StateGet(24) & 1) != 0) {
            pspSync();
            pspL2CacheWriteback1(uncachedNewCmdList, 0);
        }
        // 1E18
        cmdList[1] = GE_MAKE_OP(SCE_GE_CMD_BASE, (relAddr >> 24) << 16);
        cmdList[2] = GE_MAKE_OP(SCE_GE_CMD_END, 0);
        pspSync();
        HW_GE_LISTADDR = (int)UCACHED(cmdList + 1);
        HW_GE_EXEC = 1;
        // 1E4C
        while ((HW_GE_EXEC & 1) != 0)
            ;
        HW_GE_LISTADDR = (int)UCACHED(uncachedNewCmdList);
        HW_GE_EXEC = 1;
        // 1E74
        while ((HW_GE_EXEC & 1) != 0)
            ;
        uncachedNewCmdList[0] = oldCmd;
        if ((pspCop0StateGet(24) & 1) != 0) {
            pspSync();
            pspL2CacheWriteback1(uncachedNewCmdList, 0);
        }
    }
    // 1ED0
    // 1ED4
    if ((type & SCE_GE_INTERNAL_REG_RADR2) && radr2 != 0) {
        int *uncachedNewCmdList = UCACHED(radr2 - 4);
        u32 relAddr = (u32)(uncachedCmdList - old120);
        int oldCmd = uncachedNewCmdList[0];
        uncachedNewCmdList[0] = GE_MAKE_OP(SCE_GE_CMD_CALL, relAddr & 0x00FFFFFF);
        pspCache(0x1A, uncachedNewCmdList);
        if ((pspCop0StateGet(24) & 1) != 0) {
            pspSync();
            pspL2CacheWriteback1(uncachedNewCmdList, 0);
        }
        // 1F50
        cmdList[1] = GE_MAKE_OP(SCE_GE_CMD_BASE, (relAddr >> 24) << 16);
        cmdList[2] = GE_MAKE_OP(SCE_GE_CMD_END, 0);
        pspSync();
        HW_GE_LISTADDR = (int)UCACHED(cmdList + 1);
        HW_GE_EXEC = 1;

        // 1F88
        while ((HW_GE_EXEC & 1) != 0)
            ;
        HW_GE_LISTADDR = (int)UCACHED(uncachedNewCmdList);
        HW_GE_EXEC = 0x101;

        // 1FB0
        while ((HW_GE_EXEC & 1) != 0)
            ;
        uncachedNewCmdList[0] = oldCmd;
        pspCache(0x1A, uncachedNewCmdList);
        if ((pspCop0StateGet(24) & 1) != 0) {
            pspSync();
            pspL2CacheWriteback1(uncachedNewCmdList, 0);
        }
    }
    cmdList[0] = base;
    // 2010
    cmdList[1] = HW_GE_CMD(SCE_GE_CMD_END);
    HW_GE_LISTADDR = (int)uncachedCmdList;
    pspSync();
    HW_GE_EXEC = old100 | 1;

    // 2034
    while ((HW_GE_EXEC & 1) != 0)
        ;
    HW_GE_LISTADDR = old108;
    HW_GE_STALLADDR = old10C;
    HW_GE_OADR = old120;
    HW_GE_OADR1 = old124;
    HW_GE_OADR2 = old128;

    if ((type & SCE_GE_INTERNAL_REG_RADR1) != 0)
        HW_GE_RADR1 = radr1;
    // 2084
    if ((type & SCE_GE_INTERNAL_REG_RADR2) != 0)
        HW_GE_RADR2 = radr2;

    // 2094
    HW_GE_INTERRUPT_TYPE4 = HW_GE_INTERRUPT_TYPE1 ^ old304;
    sceKernelCpuResumeIntr(oldIntr);
    return 0;
}

int
_sceGeInterrupt(int arg0 __attribute__ ((unused)), int arg1
                __attribute__ ((unused)), int arg2 __attribute__ ((unused)))
{
    int oldIntr = sceKernelCpuSuspendIntr();
    int attr = HW_GE_INTERRUPT_TYPE1;
    int unk1 = HW_GE_UNK004;
    if ((attr & HW_GE_INTERR) != 0) {
        // 2228
        HW_GE_INTERRUPT_TYPE4 = HW_GE_INTERR;
        _sceGeErrorInterrupt(attr, unk1, arg2);
    }
    // 2118
    if ((attr & (HW_GE_INTSIG | HW_GE_INTFIN)) == (HW_GE_INTSIG | HW_GE_INTFIN)) {
        // 2218
        Kprintf("GE INTSIG/INTFIN at the same time\n"); // 0x6324
    }
    // 2128
    if ((attr & HW_GE_INTFIN) == 0) {
        // signal and/or end
        // 21AC
        if ((attr & HW_GE_INTSIG) == 0 && (attr & HW_GE_INTEND) != 0) {   // 2208
            // 21FC dup
            HW_GE_INTERRUPT_TYPE3 = HW_GE_INTEND;
        } else if ((attr & HW_GE_INTSIG) != 0 && (attr & HW_GE_INTEND) == 0) {
            // 21FC dup
            HW_GE_INTERRUPT_TYPE3 = HW_GE_INTSIG;
        } else {
            // 21C0
            while ((HW_GE_EXEC & 1) != 0)
                ;
            HW_GE_INTERRUPT_TYPE4 = HW_GE_INTSIG | HW_GE_INTEND;
            HW_GE_INTERRUPT_TYPE2 = HW_GE_INTSIG | HW_GE_INTEND;
            _sceGeListInterrupt(attr, unk1, arg2);
        }
    } else {
        if ((attr & HW_GE_INTEND) == 0) {
            // 2198
            Kprintf("CMD_FINISH must be used with CMD_END.\n"); // 0x6348
            HW_GE_EXEC = 0;
        }
        // 213C
        while ((HW_GE_EXEC & 1) != 0)
            ;
        HW_GE_INTERRUPT_TYPE4 = HW_GE_INTEND | HW_GE_INTFIN;
        HW_GE_INTERRUPT_TYPE2 = HW_GE_INTEND | HW_GE_INTFIN;
        _sceGeFinishInterrupt(attr, unk1, arg2);
    }

    // 2170
    sceKernelCpuResumeIntrWithSync(oldIntr);
    return -1;
}

s32 _sceGeSysEventHandler(s32 ev_id, char *ev_name __attribute__((unused)), void *param, s32 *result __attribute__((unused)))
{
    switch (ev_id) {
    case SCE_SYSTEM_SUSPEND_EVENT_PHASE0_5:
        // 2420
        sceSysregAwRegABusClockEnable();
        _sceGeQueueSuspend();
        sceGeSaveContext(&_aw_ctx);
        _aw_ctx.ctx[10] = HW_GE_EDRAM_ADDR_TRANS_DISABLE;
        _aw_ctx.ctx[11] = HW_GE_EDRAM_ADDR_TRANS_VALUE;
        _aw_ctx.ctx[12] = HW_GE_EDRAM_REFRESH_UNK1;
        _aw_ctx.ctx[13] = HW_GE_EDRAM_REFRESH_UNK2;
        _aw_ctx.ctx[14] = HW_GE_EDRAM_REFRESH_UNK3;
        _aw_ctx.ctx[15] = HW_GE_EDRAM_UNK40;
        _aw_ctx.ctx[16] = HW_GE_GEOMETRY_CLOCK;
        break;

    case SCE_SYSTEM_SUSPEND_EVENT_PHASE0_3:
        // 228C
        _sceGeEdramSuspend();
        if (*(int *)(param + 4) != 2) {
            sceSysregAwRegABusClockDisable();
            sceSysregAwRegBBusClockDisable();
            sceSysregAwEdramBusClockDisable();
        }
        break;

    case SCE_SYSTEM_RESUME_EVENT_PHASE0_5:
        // 22C4
        sceSysregAwResetDisable();
        sceSysregAwRegABusClockEnable();
        sceSysregAwRegBBusClockEnable();
        sceSysregAwEdramBusClockEnable();
        sceGeEdramInit();
        _sceGeEdramResume();
        HW_GE_EXEC = 0;
        HW_GE_LISTADDR = 0;
        HW_GE_STALLADDR = 0;
        HW_GE_RADR1 = 0;
        HW_GE_RADR2 = 0;
        HW_GE_VADR = 0;
        HW_GE_IADR = 0;
        HW_GE_OADR = 0;
        HW_GE_OADR1 = 0;
        HW_GE_OADR2 = 0;
        HW_GE_INTERRUPT_TYPE4 = HW_GE_INTERRUPT_TYPE1;
        HW_GE_INTERRUPT_TYPE3 = HW_GE_INTERRUPT_TYPE2;
        HW_GE_INTERRUPT_TYPE2 = HW_GE_INTSIG | HW_GE_INTEND | HW_GE_INTFIN;
        HW_GE_EDRAM_ADDR_TRANS_DISABLE = _aw_ctx.ctx[10];
        HW_GE_EDRAM_ADDR_TRANS_VALUE = _aw_ctx.ctx[11];
        HW_GE_EDRAM_REFRESH_UNK1 = _aw_ctx.ctx[12];
        HW_GE_EDRAM_REFRESH_UNK2 = _aw_ctx.ctx[13];
        HW_GE_EDRAM_REFRESH_UNK3 = _aw_ctx.ctx[14];
        HW_GE_EDRAM_UNK40 = _aw_ctx.ctx[15];
        HW_GE_GEOMETRY_CLOCK = _aw_ctx.ctx[16];
        sceSysregSetMasterPriv(64, 1);
        sceGeRestoreContext(&_aw_ctx);
        sceSysregSetMasterPriv(64, 0);
        _sceGeQueueResume();
        if (_sceGeQueueStatus() == 0)
            sceSysregAwRegABusClockDisable();
        break;
    }
    return 0;
}

int _sceGeModuleStart()
{
    sceGeInit();
    return 0;
}

int _sceGeModuleRebootPhase(s32 arg0 __attribute__((unused)), void *arg1 __attribute__((unused)), s32 arg2 __attribute__((unused)), s32 arg3 __attribute__((unused)))
{
    if (arg0 == 1)
        sceGeBreak(0, NULL);    // 24E4
    return 0;
}

int _sceGeModuleRebootBefore(void *arg0 __attribute__((unused)), s32 arg1 __attribute__((unused)), s32 arg2 __attribute__((unused)), s32 arg3 __attribute__((unused)))
{
    sceGeEnd();
    return 0;
}

int sceGeRegisterLogHandler(SceGeLogHandler handler)
{
    g_GeLogHandler = handler;
    return 0;
}

int sceGeSetGeometryClock(int opt)
{
    int old = HW_GE_GEOMETRY_CLOCK;
    HW_GE_GEOMETRY_CLOCK = opt & 1;
    return old & 1;
}

int _sceGeSetBaseRadr(int arg0, int arg1, int arg2)
{
    return _sceGeSetInternalReg(7, arg0, arg1, arg2);
}

int _sceGeEdramResume()
{
    sceGeEdramSetAddrTranslation(g_edramAddrTrans);
    if (g_uiEdramHwSize == 0x00400000) {
        // 261C
        HW_GE_EDRAM_ENABLED_SIZE = 2;
        sceSysregSetAwEdramSize(1);
    }
    // 25A0
    memcpy(UCACHED(sceGeEdramGetAddr()),
           UCACHED(sceKernelGetAWeDramSaveAddr()), g_uiEdramHwSize);
    sceKernelDcacheWritebackInvalidateAll();
    if (g_uiEdramHwSize == 0x00400000 && g_uiEdramSize == 0x00200000) { // 25F4
        HW_GE_EDRAM_ENABLED_SIZE = 4;
        sceSysregSetAwEdramSize(0);
    }
    return 0;
}

int sceGeEdramInit()
{
    int i = 83;
    // 264C
    while ((i--) != 0)
        ;
    HW_GE_EDRAM_UNK10 = 1;
    // 2660
    while ((HW_GE_EDRAM_UNK10 & 1) != 0)
        ;
    HW_GE_EDRAM_REFRESH_UNK2 = 0x6C4;
    HW_GE_EDRAM_UNK40 = 1;
    HW_GE_EDRAM_UNK90 = 3;
    HW_GE_EDRAM_ENABLED_SIZE = 4;
    if (g_uiEdramHwSize != 0)
        return 0;
    if ((HW_GE_EDRAM_ADDR_TRANS_DISABLE & 1) == 0) {
        // 2758
        g_edramAddrTrans = HW_GE_EDRAM_ADDR_TRANS_VALUE << 1;
    } else
        g_edramAddrTrans = 0;

    // 26C8
    if (sceSysregGetTachyonVersion() > 0x004FFFFF) {
        // 271C
        g_uiEdramSize = 0x00200000;
        g_uiEdramHwSize = 0x00400000;
        if (sceSysregSetAwEdramSize(0) != 0) {
            HW_GE_EDRAM_ENABLED_SIZE = 2;
            sceSysregSetAwEdramSize(1);
            g_uiEdramSize = 0x00400000;
        }
        return 0;
    }
    int size = (HW_GE_EDRAM_HW_SIZE & 0xFFFF) << 10;
    g_uiEdramSize = size;
    g_uiEdramHwSize = size;
    return 0;
}

int sceGeEdramSetRefreshParam(int mode, int arg1, int arg2, int arg3)
{
    int ret = 0;
    int oldIntr = sceKernelCpuSuspendIntr();
    int old44 = HW(0xBC000044);
    HW(0xBC000044) &= 0xFF9BFFFF;
    if (mode != 0) {
        // 2858
        if (mode == 1) {
            HW_GE_EDRAM_REFRESH_UNK1 =
                ((arg3 & 0xF) << 20) | (HW_GE_EDRAM_REFRESH_UNK1 & 0xFF0FFFFF);
            HW_GE_EDRAM_REFRESH_UNK3 = arg2 & 0x3FF;
            HW_GE_EDRAM_REFRESH_UNK2 = arg1 & 0x007FFFFF;
            if ((HW_GE_EDRAM_UNK40 & 2) == 0) {
                // 284C
                HW_GE_EDRAM_UNK40 = 3;
            }
        } else {
            ret = -1;
        }
    } else {
        HW_GE_EDRAM_REFRESH_UNK1 =
            ((arg3 & 0xF) << 20) | (HW_GE_EDRAM_REFRESH_UNK1 & 0xFF0FFFFF);
        HW_GE_EDRAM_REFRESH_UNK3 = arg2 & 0x3FF;
        HW_GE_EDRAM_REFRESH_UNK2 = arg1 & 0x007FFFFF;
        if ((HW_GE_EDRAM_UNK40 & 2) != 0) {
            // 284C dup
            HW_GE_EDRAM_UNK40 = 1;
        }
    }

    // 281C
    HW(0xBC000044) = old44;
    sceKernelCpuResumeIntrWithSync(oldIntr);
    return ret;
}

int sceGeEdramSetSize(int size)
{
    if (size == 0x200000) {
        // 2944
        g_uiEdramSize = size;
        sceGeSetReg(SCE_GE_REG_EDRAM_ENABLED_SIZE, 4);
        // 2934 dup
        sceSysregSetAwEdramSize(0);
    } else if (size == 0x400000) {
        // 28FC
        if (sceSysregGetTachyonVersion() <= 0x4FFFFF)
            return SCE_ERROR_INVALID_SIZE;
        g_uiEdramSize = size;
        sceGeSetReg(SCE_GE_REG_EDRAM_ENABLED_SIZE, 2);
        // 2934 dup
        sceSysregSetAwEdramSize(1);
    } else
        return SCE_ERROR_INVALID_SIZE;

    return 0;
}

int sceGeEdramGetAddr()
{
    return 0x04000000;
}

int sceGeEdramSetAddrTranslation(int arg)
{
    int ret;
    if (arg != 0 && arg != 0x200 && arg != 0x400
        && arg != 0x800 && arg != 0x1000)
        return SCE_ERROR_INVALID_VALUE;

    // 29AC
    int oldIntr = sceKernelCpuSuspendIntr();
    g_edramAddrTrans = arg;
    if (arg == 0) {
        // 2A28
        if ((HW_GE_EDRAM_ADDR_TRANS_DISABLE & 1) == 0) {
            ret = HW_GE_EDRAM_ADDR_TRANS_VALUE << 1;
            HW_GE_EDRAM_ADDR_TRANS_DISABLE = 1;
        } else
            ret = 0;
    } else if ((HW_GE_EDRAM_ADDR_TRANS_DISABLE & 1) == 0) {
        // 2A0C
        ret = HW_GE_EDRAM_ADDR_TRANS_VALUE << 1;
        HW_GE_EDRAM_ADDR_TRANS_VALUE = arg >> 1;
    } else {
        HW_GE_EDRAM_ADDR_TRANS_VALUE = arg >> 1;
        HW_GE_EDRAM_ADDR_TRANS_DISABLE = 0;
        ret = 0;
    }
    sceKernelCpuResumeIntrWithSync(oldIntr);
    return ret;
}

int _sceGeEdramSuspend()
{
    sceKernelDcacheWritebackInvalidateAll();
    if (g_uiEdramHwSize == 0x00400000) {
        // 2ABC
        HW_GE_EDRAM_ENABLED_SIZE = 2;
        sceSysregSetAwEdramSize(1);
    }
    // 2A80
    memcpy(UCACHED(sceKernelGetAWeDramSaveAddr()),
           UCACHED(sceGeEdramGetAddr()), g_uiEdramHwSize);
    return 0;
}

int sceGeEdramGetSize()
{
    return g_uiEdramSize;
}

int sceGeEdramGetHwSize()
{
    return g_uiEdramHwSize;
}

int _sceGeQueueInit()
{
    // 2B10
    int i;
    for (i = 0; i < 64; i++) {
        SceGeDisplayList *cur = &g_displayLists[i];
        cur->next = &g_displayLists[i + 1];
        cur->prev = &g_displayLists[i - 1];
        cur->signal = SCE_GE_DL_SIGNAL_NONE;
        cur->state = SCE_GE_DL_STATE_NONE;
        cur->ctxUpToDate = 0;
    }

    g_displayLists[0].prev = NULL;
    g_displayLists[63].next = NULL;

    g_AwQueue.curRunning = NULL;

    g_AwQueue.free_last = &g_displayLists[63];
    g_AwQueue.free_first = g_displayLists;

    g_AwQueue.active_first = NULL;
    g_AwQueue.active_last = NULL;

    g_AwQueue.drawingEvFlagId = sceKernelCreateEventFlag("SceGeQueue", 0x201, 2, NULL);
    g_AwQueue.listEvFlagIds[0] = sceKernelCreateEventFlag("SceGeQueueId", 0x201, -1, NULL);
    g_AwQueue.listEvFlagIds[1] = sceKernelCreateEventFlag("SceGeQueueId", 0x201, -1, NULL);
    g_AwQueue.syscallId = sceKernelQuerySystemCall((void*)sceGeListUpdateStallAddr);
    SceKernelGameInfo *info = sceKernelGetGameInfo();
    g_AwQueue.patched = 0;
    // Patch for Itadaki Street Portable (missing a cache flush function when enqueuing the first display list)
    if (info != NULL && strcmp(info->gameId, "ULJM05127") == 0) {
        g_AwQueue.patched = 1;
    }
    return 0;
}

int _sceGeQueueSuspend()
{
    if (g_AwQueue.active_first == NULL)
        return 0;

    // 2C5C
    while ((HW_GE_EXEC & 1) != 0) {
        if (HW_GE_LISTADDR == HW_GE_STALLADDR) {
            int oldCmd1, oldCmd2;
            // 2DF8
            g_GeSuspend.status = HW_GE_EXEC | 0x1;
            int *stall = (int *)HW_GE_STALLADDR;
            g_GeSuspend.stallAddr = stall;
            g_GeSuspend.unk0 = HW_GE_UNK004;
            g_GeSuspend.listAddr = HW_GE_LISTADDR;
            g_GeSuspend.intrType = HW_GE_INTERRUPT_TYPE1;
            g_GeSuspend.sigCmd = HW_GE_CMD(SCE_GE_CMD_SIGNAL);
            g_GeSuspend.finCmd = HW_GE_CMD(SCE_GE_CMD_FINISH);
            g_GeSuspend.endCmd = HW_GE_CMD(SCE_GE_CMD_END);
            oldCmd1 = stall[0];
            oldCmd2 = stall[1];
            stall[0] = GE_MAKE_OP(SCE_GE_CMD_FINISH, 0);
            stall[1] = GE_MAKE_OP(SCE_GE_CMD_END, 0);
            pspCache(0x1A, &stall[0]);
            pspCache(0x1A, &stall[1]);
            if ((pspCop0StateGet(24) & 1) != 0) {
                pspSync();
                pspL2CacheWriteback10(stall, 1);
            }
            // 2ECC
            HW_GE_STALLADDR += 8;
            // 2EE0
            while ((HW_GE_EXEC & 1) != 0)
                ;
            if (HW_GE_LISTADDR == (int)g_GeSuspend.stallAddr) {
                // 2F38
                HW_GE_STALLADDR = (int)stall;
                stall[0] = oldCmd1;
                stall[1] = oldCmd2;
                pspCache(0x1A, &stall[0]);
                pspCache(0x1A, &stall[1]);
                break;
            } else {
                // 2F08
                while ((HW_GE_INTERRUPT_TYPE1 & HW_GE_INTFIN) == 0)
                    ;
                stall[0] = oldCmd1;
                stall[1] = oldCmd2;
                pspCache(0x1A, &stall[0]);
                pspCache(0x1A, &stall[1]);
                return 0;
            }
        }
    }

    // 2C88
    if ((HW_GE_INTERRUPT_TYPE1 & HW_GE_INTSIG) == 0 && (g_AwQueue.active_first->signal != SCE_GE_DL_SIGNAL_BREAK || g_AwQueue.isBreak != 0)) // 2DE8
    {
        // 2CB0
        while ((HW_GE_INTERRUPT_TYPE1 & HW_GE_INTFIN) == 0)
            ;
    }
    // 2CC4
    g_GeSuspend.unk0 = HW_GE_UNK004;
    g_GeSuspend.status = HW_GE_EXEC;
    g_GeSuspend.listAddr = HW_GE_LISTADDR;
    g_GeSuspend.stallAddr = (int *)HW_GE_STALLADDR;
    g_GeSuspend.intrType = HW_GE_INTERRUPT_TYPE1;
    g_GeSuspend.sigCmd = HW_GE_CMD(SCE_GE_CMD_SIGNAL);
    g_GeSuspend.finCmd = HW_GE_CMD(SCE_GE_CMD_FINISH);
    g_GeSuspend.endCmd = HW_GE_CMD(SCE_GE_CMD_END);
    sceSysregSetMasterPriv(64, 1);
    int old108 = HW_GE_LISTADDR;
    int old10C = HW_GE_STALLADDR;
    int old304 = HW_GE_INTERRUPT_TYPE1;
    HW_GE_INTERRUPT_TYPE4 = HW_GE_INTFIN;
    HW_GE_LISTADDR = (int)UCACHED(stopCmd);
    HW_GE_STALLADDR = 0;
    HW_GE_EXEC = 1;
    // 2D80
    while ((HW_GE_EXEC & 1) != 0)
        ;
    // 2D94
    while ((HW_GE_INTERRUPT_TYPE1 & HW_GE_INTFIN) == 0)
        ;
    HW_GE_LISTADDR = old108;
    HW_GE_STALLADDR = old10C;
    HW_GE_INTERRUPT_TYPE4 = HW_GE_INTERRUPT_TYPE1 ^ old304;
    sceSysregSetMasterPriv(64, 0);
    return 0;
}

int _sceGeQueueResume()
{
    if (g_AwQueue.active_first == NULL)
        return 0;
    // 2F88
    sceSysregSetMasterPriv(64, 1);
    HW_GE_LISTADDR = (int)UCACHED(&g_GeSuspend.sigCmd);
    HW_GE_STALLADDR = 0;
    HW_GE_EXEC = g_GeSuspend.status | 1;
    // 2FC0
    while ((HW_GE_EXEC & 1) != 0)
        ;
    // 2FD4
    while ((HW_GE_INTERRUPT_TYPE1 & HW_GE_INTFIN) == 0)
        ;
    sceSysregSetMasterPriv(64, 0);
    int oldFlag = g_GeSuspend.intrType;
    int flag = 0;
    if ((oldFlag & HW_GE_INTSIG) == 0)
        flag |= HW_GE_INTSIG;
    if ((oldFlag & HW_GE_INTEND) == 0)
        flag |= HW_GE_INTEND;
    if ((oldFlag & HW_GE_INTFIN) == 0)
        flag |= HW_GE_INTFIN;
    HW_GE_INTERRUPT_TYPE4 = flag;
    HW_GE_LISTADDR = g_GeSuspend.listAddr;
    HW_GE_STALLADDR = (int)g_GeSuspend.stallAddr;
    HW_GE_EXEC = g_GeSuspend.status;
    return 0;
}

void
_sceGeFinishInterrupt(int arg0 __attribute__ ((unused)), int arg1
                      __attribute__ ((unused)), int arg2
                      __attribute__ ((unused)))
{
    SceGeDisplayList *dl = g_AwQueue.curRunning;
    g_AwQueue.isBreak = 0;
    g_AwQueue.curRunning = NULL;
    if (dl != NULL) {
        if (dl->signal == SCE_GE_DL_SIGNAL_SYNC) {
            // 351C
            dl->signal = SCE_GE_DL_SIGNAL_NONE;
            g_AwQueue.curRunning = dl;
            HW_GE_EXEC |= 1;
            pspSync();
            return;
        } else if (dl->signal == SCE_GE_DL_SIGNAL_PAUSE) {
            // 3468
            int state = HW_GE_EXEC;
            dl->flags = state;
            dl->list = (int *)HW_GE_LISTADDR;
            dl->radr1 = HW_GE_RADR1;
            dl->radr2 = HW_GE_RADR2;
            dl->oadr = HW_GE_OADR;
            dl->oadr1 = HW_GE_OADR1;
            dl->oadr2 = HW_GE_OADR2;
            dl->base = HW_GE_CMD(SCE_GE_CMD_BASE);
            if ((state & 0x200) == 0) {
                dl->radr2 = 0;
                dl->oadr2 = 0;
                if ((state & 0x100) == 0) {
                    dl->radr1 = 0;
                    dl->oadr1 = 0;
                }
            }
            // 34E8
            if (g_AwQueue.active_first == dl) {
                // 3500
                dl->signal = SCE_GE_DL_SIGNAL_BREAK;
                if (dl->cbId >= 0) {
                    void *list = dl->list;
                    // 3288 dup
                    if (g_AwQueue.sdkVer <= 0x02000010)
                        list = 0;
                    sceKernelCallSubIntrHandler(SCE_GE_INT, dl->cbId * 2, dl->signalData, (int)list);    // call signal CB
                }
                // 32A4
                _sceGeClearBp();
                return;
            }
            dl = NULL;
        }
        // 309C
        if (dl != NULL) {
            if (dl->state == SCE_GE_DL_STATE_RUNNING) {
                // 32DC
                int *cmdList = (int *)HW_GE_LISTADDR;
                u32 lastCmd1 = *(int *)UUNCACHED(cmdList - 2);
                u32 lastCmd2 = *(int *)UUNCACHED(cmdList - 1);
                if ((lastCmd1 >> 24) != SCE_GE_CMD_FINISH || (lastCmd2 >> 24) != SCE_GE_CMD_END) {
                    // 3318
                    Kprintf("_sceGeFinishInterrupt(): illegal finish sequence (MADR=0x%08X)\n", cmdList);   // 6398
                }
                // 3328
                dl->state = SCE_GE_DL_STATE_COMPLETED;
                if (dl->stackOff != 0) {
                    // 343C
                    Kprintf("_sceGeFinishInterrupt(): CALL/RET nesting corrupted\n");   // 63D8
                }
                // 3338
                if (g_GeLogHandler != NULL) {
                    // 3418
                    g_GeLogHandler(SCE_GE_LOG_DL_END, (int)dl ^ g_dlMask,
                                 cmdList, lastCmd1, lastCmd2);
                }
                // 3348
                if (dl->cbId >= 0) {
                    if (g_AwQueue.sdkVer <= 0x02000010)
                        cmdList = NULL;
                    sceKernelCallSubIntrHandler(SCE_GE_INT, dl->cbId * 2 + 1, lastCmd1 & 0xFFFF, (int)cmdList); // call finish CB
                }
                // 337C
                if (dl->ctx != NULL) {
                    // 3408
                    sceGeRestoreContext(dl->ctx);
                }
                // 338C
                if (dl->prev != NULL)
                    dl->prev->next = dl->next;
                // 33A0
                if (dl->next != NULL)
                    dl->next->prev = dl->prev;

                // 33A8
                if (g_AwQueue.active_first == dl)
                    g_AwQueue.active_first = dl->next;

                // 33B8
                if (g_AwQueue.active_last == dl) {
                    // 3400
                    g_AwQueue.active_last = dl->prev;
                }
                // 33C4
                if (g_AwQueue.free_first == NULL) {
                    g_AwQueue.free_first = dl;
                    // 33F8
                    dl->prev = NULL;
                } else {
                    g_AwQueue.free_last->next = dl;
                    dl->prev = g_AwQueue.free_last;
                }

                // 33E0
                dl->state = SCE_GE_DL_STATE_COMPLETED;
                dl->next = NULL;
                g_AwQueue.free_last = dl;
            }
        }
    }

    // 30B0
    SceGeDisplayList *dl2 = g_AwQueue.active_first;
    if (dl2 == NULL) {
        // 32B4
        HW_GE_EXEC = 0;
        pspSync();
        sceSysregAwRegABusClockDisable();
        sceKernelSetEventFlag(g_AwQueue.drawingEvFlagId, 2);
        // 3254
        _sceGeClearBp();
    } else {
        if (dl2->signal == SCE_GE_DL_SIGNAL_PAUSE) {
            // 3264
            dl2->state = SCE_GE_DL_STATE_PAUSED;
            dl2->signal = SCE_GE_DL_SIGNAL_BREAK;
            if (dl2->cbId >= 0) {
                void *list = dl2->list;
                // 3288 dup
                if (g_AwQueue.sdkVer <= 0x02000010)
                    list = NULL;
                sceKernelCallSubIntrHandler(SCE_GE_INT, dl2->cbId * 2,
                                            dl2->signalData, (int)list);
            }
            // 32A4
            _sceGeClearBp();
            return;
        }
        if (dl2->state == SCE_GE_DL_STATE_PAUSED) {
            // 324C
            sceSysregAwRegABusClockDisable();
            // 3254
            _sceGeClearBp();
        } else {
            int *ctx2 = (int *)dl2->ctx;
            dl2->state = SCE_GE_DL_STATE_RUNNING;
            if (ctx2 != NULL && dl2->ctxUpToDate == 0) {
                if (dl == NULL || dl->ctx == NULL) {
                    // 323C
                    sceGeSaveContext(dl2->ctx);
                } else {
                    int *ctx1 = (int *)dl->ctx;

                    // 310C
                    int i;
                    for (i = 0; i < 128; i++) {
                        ctx2[0] = ctx1[0];
                        ctx2[1] = ctx1[1];
                        ctx2[2] = ctx1[2];
                        ctx2[3] = ctx1[3];
                        ctx1 += 4;
                        ctx2 += 4;
                    }
                }
            }
            // (3138)
            // 313C
            dl2->ctxUpToDate = 1;
            HW_GE_EXEC = 0;
            HW_GE_STALLADDR = (int)UCACHED(dl2->stall);
            HW_GE_LISTADDR = (int)UCACHED(dl2->list);
            HW_GE_OADR = dl2->oadr;
            HW_GE_OADR1 = dl2->oadr1;
            HW_GE_OADR2 = dl2->oadr2;
            _sceGeSetBaseRadr(dl2->base, dl2->radr1, dl2->radr2);
            pspSync();
            g_AwQueue.curRunning = dl2;
            HW_GE_EXEC = dl2->flags | 1;
            pspSync();
            if (g_GeLogHandler != 0) {
                // 321C
                g_GeLogHandler(SCE_GE_LOG_DL_RUNNING, (int)dl2 ^ g_dlMask, dl2->list, dl2->stall);
            }
        }
    }

    // 31C8
    if (dl != NULL) {
        u32 idx = (dl - g_displayLists) / sizeof(SceGeDisplayList);
        sceKernelSetEventFlag(g_AwQueue.listEvFlagIds[idx / 32], 1 << (idx % 32));
    }
}

void
_sceGeListInterrupt(int arg0 __attribute__ ((unused)), int arg1
                    __attribute__ ((unused)), int arg2 __attribute__ ((unused)))
{
    SceGeDisplayList *dl = g_AwQueue.active_first;
    if (dl == NULL) {
        // 3C0C
        Kprintf("_sceGeListInterrupt(): unknown interrupt\n");
        return;
    }
    u32 *cmdList = (u32 *)HW_GE_LISTADDR;
    u32 *lastCmdPtr1 = cmdList - 2;
    u32 *lastCmdPtr2 = cmdList - 1;
    u32 lastCmd1 = *(u32 *) UUNCACHED(lastCmdPtr1);
    u32 lastCmd2 = *(u32 *) UUNCACHED(lastCmdPtr2);
    // 35F4
    if ((lastCmd1 >> 24) != SCE_GE_CMD_SIGNAL || (lastCmd2 >> 24) != SCE_GE_CMD_END) {
        Kprintf("_sceGeListInterrupt(): bad signal sequence (MADR=0x%08X)\n", cmdList); // 0x643C
        return;
    }
    if (g_GeLogHandler != NULL) {
        // 3BE8
        g_GeLogHandler(SCE_GE_LOG_DL_SIGNAL, (int)dl ^ g_dlMask, cmdList, lastCmd1, lastCmd2);
    }
    // 3618
    switch ((lastCmd1 >> 16) & 0xFF) {
    case SCE_GE_SIGNAL_HANDLER_SUSPEND:
        // 3670
        if (g_AwQueue.sdkVer <= 0x02000010) {
            dl->state = SCE_GE_DL_STATE_PAUSED;
            cmdList = NULL;
        }
        // 3698
        if (dl->cbId >= 0) {
            sceKernelCallSubIntrHandler(SCE_GE_INT, dl->cbId * 2,
                                        lastCmd1 & 0xFFFF, (int)cmdList);
            pspSync();
        }
        // 36B4
        if (g_AwQueue.sdkVer <= 0x02000010)
            dl->state = lastCmd2 & 0xFF;
        HW_GE_EXEC |= 1;
        break;

    case SCE_GE_SIGNAL_HANDLER_CONTINUE:
        // 3708
        HW_GE_EXEC |= 1;
        pspSync();
        if (dl->cbId >= 0) {
            if (g_AwQueue.sdkVer <= 0x02000010)
                cmdList = NULL;
            sceKernelCallSubIntrHandler(SCE_GE_INT, dl->cbId * 2,
                                        lastCmd1 & 0xFFFF, (int)cmdList);
        }
        break;

    case SCE_GE_SIGNAL_HANDLER_PAUSE:
        dl->state = SCE_GE_DL_STATE_PAUSED;
        dl->signal = lastCmd2 & 0xFF;
        dl->signalData = lastCmd1;
        HW_GE_EXEC |= 1;
        break;

    case SCE_GE_SIGNAL_SYNC:
        // 3994
        dl->signal = SCE_GE_DL_SIGNAL_SYNC;
        HW_GE_EXEC |= 1;
        break;

    case SCE_GE_SIGNAL_CALL:
    case SCE_GE_SIGNAL_RCALL:
    case SCE_GE_SIGNAL_OCALL:
        {
            // 3870
            if (dl->stackOff >= dl->numStacks) {
                // 398C
                _sceGeListError(lastCmd1, SCE_GE_SIGNAL_ERROR_STACK_OVERFLOW);
                break;
            }
            SceGeStack *curStack = &dl->stack[dl->stackOff++];
            curStack->stack[0] = HW_GE_EXEC;
            curStack->stack[1] = HW_GE_LISTADDR;
            curStack->stack[2] = HW_GE_OADR;
            curStack->stack[3] = HW_GE_OADR1;
            curStack->stack[4] = HW_GE_OADR2;
            curStack->stack[5] = HW_GE_RADR1;
            curStack->stack[6] = HW_GE_RADR2;
            curStack->stack[7] = HW_GE_CMD(SCE_GE_CMD_BASE);
            if ((dl->flags & 0x200) == 0) {
                dl->radr2 = 0;
                dl->oadr2 = 0;
                if ((dl->flags & 0x100) == 0) {
                    dl->oadr1 = 0;
                    dl->radr1 = 0;
                }
            }
            // (3924)
            // 3928
            int cmdOff = (lastCmd1 << 16) | (lastCmd2 & 0xFFFF);
            u32 *newCmdList;
            if (((lastCmd1 >> 16) & 0xFF) == SCE_GE_SIGNAL_CALL)
                newCmdList = (u32 *) cmdOff;
            else if (((lastCmd1 >> 16) & 0xFF) == SCE_GE_SIGNAL_RCALL)
                newCmdList = &cmdList[cmdOff / 4 - 2];
            else
                newCmdList = HW_GE_OADR + (u32 *) cmdOff;

            // 395C
            if (((int)newCmdList & 3) != 0) {
                // 397C
                _sceGeListError(lastCmd1, SCE_GE_SIGNAL_ERROR_INVALID_ADDRESS);
            }
            // 396C
            HW_GE_LISTADDR = (int)UCACHED(newCmdList);
            HW_GE_EXEC = 1;
            pspSync();
            break;
        }

    case SCE_GE_SIGNAL_JUMP:
    case SCE_GE_SIGNAL_RJUMP:
    case SCE_GE_SIGNAL_OJUMP:
        {
            // 377C
            int cmdOff = (lastCmd1 << 16) | (lastCmd2 & 0xFFFF);
            u32 *newCmdList;
            if (((lastCmd1 >> 16) & 0xFF) == SCE_GE_SIGNAL_JUMP)
                newCmdList = (u32 *) cmdOff;
            else if (((lastCmd1 >> 16) & 0xFF) == SCE_GE_SIGNAL_RJUMP)
                newCmdList = &cmdList[cmdOff / 4 - 2];
            else
                newCmdList = HW_GE_OADR + (u32 *) cmdOff;

            // 37B4
            if (((int)newCmdList & 3) != 0) {
                // 37D0
                _sceGeListError(lastCmd1, SCE_GE_SIGNAL_ERROR_INVALID_ADDRESS);
            }
            // 37C4
            HW_GE_LISTADDR = (int)UCACHED(newCmdList);
            HW_GE_EXEC |= 1;
            break;
        }

    case SCE_GE_SIGNAL_RET:
        // 37E0
        if (dl->stackOff == 0) {
            _sceGeListError(lastCmd1, SCE_GE_SIGNAL_ERROR_STACK_UNDERFLOW);
            return;
        }
        // 3804
        SceGeStack *curStack = &dl->stack[dl->stackOff--];
        HW_GE_LISTADDR = (int)UCACHED(curStack->stack[1]);
        HW_GE_OADR = curStack->stack[2];
        HW_GE_OADR1 = curStack->stack[3];
        HW_GE_OADR2 = curStack->stack[4];
        _sceGeSetBaseRadr(curStack->stack[7], curStack->stack[5],
                          curStack->stack[6]);
        HW_GE_EXEC = curStack->stack[0] | 1;
        pspSync();
        break;

    case SCE_GE_SIGNAL_RTBP0: case SCE_GE_SIGNAL_RTBP1: case SCE_GE_SIGNAL_RTBP2: case SCE_GE_SIGNAL_RTBP3:
    case SCE_GE_SIGNAL_RTBP4: case SCE_GE_SIGNAL_RTBP5: case SCE_GE_SIGNAL_RTBP6: case SCE_GE_SIGNAL_RTBP7:
    case SCE_GE_SIGNAL_OTBP0: case SCE_GE_SIGNAL_OTBP1: case SCE_GE_SIGNAL_OTBP2: case SCE_GE_SIGNAL_OTBP3:
    case SCE_GE_SIGNAL_OTBP4: case SCE_GE_SIGNAL_OTBP5: case SCE_GE_SIGNAL_OTBP6: case SCE_GE_SIGNAL_OTBP7:
        {
            // 39D0
            int off = (lastCmd1 << 16) | (lastCmd2 & 0xFFFF);
            u32 *newLoc;
            if (((lastCmd1 >> 19) & 1) == 0) // RTBP
                newLoc = lastCmdPtr1 + off;
            else // OTBP
                newLoc = HW_GE_OADR + (u32 *) off;

            // 39F0
            if (((int)newLoc & 0xF) != 0) {
                // 3A50
                _sceGeListError(lastCmd1, SCE_GE_SIGNAL_ERROR_INVALID_ADDRESS);
            }
            // 3A00
            int id = (lastCmd1 >> 16) & 0x7;
            int *uncachedCmdPtr = UUNCACHED(lastCmdPtr1);
            uncachedCmdPtr[0] = GE_MAKE_OP(SCE_GE_CMD_TBP0 + id, (int)newLoc);
            uncachedCmdPtr[1] = GE_MAKE_OP(SCE_GE_CMD_TBW0 + id, ((((int)newLoc >> 24) & 0xF) << 16) | ((lastCmd2 >> 16) & 0xFF));
            // 3A40
            pspSync();
            HW_GE_LISTADDR = (int)lastCmdPtr1;
            HW_GE_EXEC |= 1;
            break;
        }

    case SCE_GE_SIGNAL_RCBP:
    case SCE_GE_SIGNAL_OCBP:
        {
            // 3A80
            int off = (lastCmd1 << 16) | (lastCmd2 & 0xFFFF);
            u32 *newLoc;
            if (((lastCmd1 >> 19) & 1) == 0) // RCBP
                newLoc = lastCmdPtr1 + off;
            else // OCBP
                newLoc = HW_GE_OADR + (u32 *) off;

            // 3AA4
            if (((int)newLoc & 0xF) != 0) {
                // 3AE8
                _sceGeListError(lastCmd1, SCE_GE_SIGNAL_ERROR_INVALID_ADDRESS);
            }
            // 3AB4
            int *uncachedCmdPtr = UUNCACHED(lastCmdPtr1);
            uncachedCmdPtr[0] = GE_MAKE_OP(SCE_GE_CMD_CBP, (int)newLoc);
            uncachedCmdPtr[1] = GE_MAKE_OP(SCE_GE_CMD_CBW, (((int)newLoc >> 24) & 0xF) << 16);
            // 3A40
            pspSync();
            HW_GE_LISTADDR = (int)lastCmdPtr1;
            HW_GE_EXEC |= 1;
            break;
        }

    case SCE_GE_SIGNAL_BREAK1:
    case SCE_GE_SIGNAL_BREAK2:
        {
            // 3B08
            if (g_deci2p == NULL) {
                HW_GE_EXEC |= 1;
                return;
            }
            int opt = 0;
            if (((lastCmd1 >> 16) & 0xFF) == SCE_GE_SIGNAL_BREAK2) {
                // 3B6C
                SceGeBpCmd *bpCmd = &g_GeDeciBreak.cmds[0];
                int i;
                opt = 1;
                for (i = 0; i < g_GeDeciBreak.size; i++) {
                    // 3B90
                    if (UCACHED(bpCmd->addr) == UCACHED(lastCmdPtr1)) {
                        // 3BC0
                        if (bpCmd->count == 0
                            || (bpCmd->count != -1 && (--bpCmd->count) != 0))
                            opt = -1;
                        break;
                    }
                    bpCmd++;
                }
                // 3BB0
                if (opt < 0) {
                    HW_GE_EXEC |= 1;
                    return;
                }
            }
            // 3B28
            HW_GE_LISTADDR = (int)lastCmdPtr1;
            _sceGeClearBp();
            g_GeDeciBreak.busy = 1;
            g_GeDeciBreak.size2 = 0;

            // 3B48
            do {
                int (*func) (int) = (void *)*(int *)(g_deci2p + 32);
                func(opt);
                opt = 0;
            } while (g_GeDeciBreak.busy != 0);
            break;
        }

    default:
        Kprintf("_sceGeListInterrupt(): bad signal command (MADR=0x%08X)\n");   // 0x6478
        break;
    }
}

int sceGeListDeQueue(int dlId)
{
    int ret;
    SceGeDisplayList *dl = (SceGeDisplayList *) (dlId ^ g_dlMask);
    int oldK1 = pspShiftK1();
    if (dl < g_displayLists || dl >= &g_displayLists[64]) {
        // 3D90
        pspSetK1(oldK1);
        return SCE_ERROR_INVALID_ID;
    }
    int oldIntr = sceKernelCpuSuspendIntr();
    if (dl->state != SCE_GE_DL_STATE_NONE) {
        if (dl->ctxUpToDate == 0) {
            if (dl->prev != NULL)
                dl->prev->next = dl->next;
            // 3CAC
            if (dl->next != NULL)
                dl->next->prev = dl->prev;

            // 3CB4
            if (g_AwQueue.active_first == dl)
                g_AwQueue.active_first = dl->next;

            // 3CC8
            if (g_AwQueue.active_last == dl) {
                // 3D88
                g_AwQueue.active_last = dl->prev;
            }
            // 3CD4
            if (g_AwQueue.free_first == NULL) {
                g_AwQueue.free_first = dl;
                // 3D80
                dl->prev = NULL;
            } else {
                g_AwQueue.free_last->next = dl;
                dl->prev = g_AwQueue.free_last;
            }

            // 3CF0
            dl->state = SCE_GE_DL_STATE_NONE;
            dl->next = NULL;
            g_AwQueue.free_last = dl;

            u32 idx = (dl - g_displayLists) / sizeof(SceGeDisplayList);
            sceKernelSetEventFlag(g_AwQueue.listEvFlagIds[idx / 32], 1 << (idx % 32));

            if (g_GeLogHandler != NULL) {
                // 3D70
                g_GeLogHandler(SCE_GE_LOG_DL_DEQUEUED, dlId);
            }
            ret = 0;
        } else
            ret = SCE_ERROR_BUSY;
    } else
        ret = SCE_ERROR_INVALID_ID;

    // 3D3C
    sceKernelCpuResumeIntr(oldIntr);
    pspSetK1(oldK1);
    return ret;
}

SceGeListState sceGeListSync(int dlId, int mode)
{
    int ret;
    SceGeDisplayList *dl = (SceGeDisplayList *) (dlId ^ g_dlMask);
    u32 idx = (dl - g_displayLists) / sizeof(SceGeDisplayList);

    int oldK1 = pspShiftK1();
    if (idx >= 64) {
        // 3EE0
        pspSetK1(oldK1);
        return SCE_ERROR_INVALID_ID;
    }

    if (mode == 0) {
        // 3E84
        int oldIntr = sceKernelCpuSuspendIntr();
        _sceGeListLazyFlush();
        sceKernelCpuResumeIntr(oldIntr);
        ret = sceKernelWaitEventFlag(g_AwQueue.listEvFlagIds[idx / 32], 1 << (idx % 32), 0, 0, 0);
        if (ret >= 0)
            ret = SCE_GE_LIST_COMPLETED;
    } else if (mode == 1) {
        // 3E10
        switch (dl->state) {
        case SCE_GE_DL_STATE_QUEUED:
            if (dl->ctxUpToDate != 0)
                ret = SCE_GE_LIST_PAUSED;
            else
                ret = SCE_GE_LIST_QUEUED;
            break;

        case SCE_GE_DL_STATE_RUNNING:
            // 3E68
            if ((int)dl->stall != HW_GE_LISTADDR)
                ret = SCE_GE_LIST_DRAWING;
            else
                ret = SCE_GE_LIST_STALLING;
            break;

        case SCE_GE_DL_STATE_COMPLETED:
            ret = SCE_GE_LIST_COMPLETED;
            break;

        case SCE_GE_DL_STATE_PAUSED:
            ret = SCE_GE_LIST_PAUSED;
            break;

        default:
            ret = SCE_ERROR_INVALID_ID;
            break;
        }
    } else
        ret = SCE_ERROR_INVALID_MODE;

    // 3DF0
    pspSetK1(oldK1);
    return ret;
}

/**
 * Wait for drawing to end
 */
SceGeListState sceGeDrawSync(int syncType)
{
    int ret;
    int oldK1 = pspShiftK1();
    if (syncType == 0) {
        // 3FA4
        int oldIntr = sceKernelCpuSuspendIntr();
        _sceGeListLazyFlush();
        sceKernelCpuResumeIntr(oldIntr);
        ret = sceKernelWaitEventFlag(g_AwQueue.drawingEvFlagId, 2, 0, 0, 0);
        ret = 0;
        if (ret >= 0) {
            // 3FF4
            int i;
            for (i = 0; i < 64; i++) {
                SceGeDisplayList *curDl = &g_displayLists[i];
                if (curDl->state == SCE_GE_DL_STATE_COMPLETED) {
                    // 4010
                    curDl->state = SCE_GE_DL_STATE_NONE;
                    curDl->ctxUpToDate = 0;
                }
                // 4000
            }
            ret = SCE_GE_LIST_COMPLETED;
        }
    } else if (syncType == 1) {
        // 3F40
        int oldIntr = sceKernelCpuSuspendIntr();
        SceGeDisplayList *dl = g_AwQueue.active_first;
        if (dl == NULL) {
            // 3F9C
            ret = SCE_GE_LIST_COMPLETED;
        } else {
            if (dl->state == SCE_GE_DL_STATE_COMPLETED)
                dl = dl->next;

            // 3F68
            if (dl != NULL) {
                if ((int)dl->stall != HW_GE_LISTADDR)
                    ret = SCE_GE_LIST_DRAWING;
                else
                    ret = SCE_GE_LIST_STALLING;
            } else
                ret = SCE_GE_LIST_COMPLETED;
        }
        // 3F8C
        sceKernelCpuResumeIntr(oldIntr);
    } else
        ret = SCE_ERROR_INVALID_MODE;

    // 3F24
    pspSetK1(oldK1);
    return ret;
}

int sceGeBreak(u32 resetQueue, void *arg1)
{
    int ret;
    if (resetQueue >= 2)
        return SCE_ERROR_INVALID_MODE;
    int oldK1 = pspShiftK1();
    if (!pspK1StaBufOk(arg1, 16)) {
        // 4368
        pspSetK1(oldK1);
        return SCE_ERROR_PRIV_REQUIRED;
    }
    int oldIntr = sceKernelCpuSuspendIntr();
    SceGeDisplayList *dl = g_AwQueue.active_first;
    if (dl != NULL) {
        if (resetQueue) {
            // 42F0
            _sceGeReset();
            // 430C
            int i;
            for (i = 0; i < 64; i++) {
                SceGeDisplayList *cur = &g_displayLists[i];
                cur->next = &g_displayLists[i + 1];
                cur->prev = &g_displayLists[i - 1];
                cur->signal = SCE_GE_DL_SIGNAL_NONE;
                cur->state = SCE_GE_DL_STATE_NONE;
                cur->ctxUpToDate = 0;
            }
            g_AwQueue.free_last = &g_displayLists[63];
            g_AwQueue.curRunning = NULL;
            g_AwQueue.active_last = NULL;
            g_displayLists[0].prev = NULL;
            g_displayLists[63].next = NULL;
            g_AwQueue.free_first = g_displayLists;
            g_AwQueue.active_first = NULL;
            ret = 0;
        } else if (dl->state == SCE_GE_DL_STATE_RUNNING) {
            // 4174
            HW_GE_EXEC = 0;
            pspSync();

            // 4180
            while ((HW_GE_EXEC & 1) != 0)
                ;
            if (g_AwQueue.curRunning != NULL) {
                int *cmdList = (int *)HW_GE_LISTADDR;
                int state = HW_GE_EXEC;
                dl->list = cmdList;
                dl->flags = state;
                dl->stall = (int *)HW_GE_STALLADDR;
                dl->radr1 = HW_GE_RADR1;
                dl->radr2 = HW_GE_RADR2;
                dl->oadr = HW_GE_OADR;
                dl->oadr1 = HW_GE_OADR1;
                dl->oadr2 = HW_GE_OADR2;
                dl->base = HW_GE_CMD(SCE_GE_CMD_BASE);
                if ((state & 0x200) == 0) {
                    dl->radr2 = 0;
                    dl->oadr2 = 0;
                    if ((state & 0x100) == 0) {
                        dl->radr1 = 0;
                        dl->oadr1 = 0;
                    }
                }
                // 4228
                int op = *((char *)UUNCACHED(cmdList - 1) + 3);
                if (op == SCE_GE_CMD_SIGNAL || op == SCE_GE_CMD_FINISH)
                {
                    // 42E8
                    dl->list = cmdList - 1;
                } else if (op == SCE_GE_CMD_END) {
                    // 42E0
                    dl->list = cmdList - 2;
                }
                // 4258
                if (dl->signal == SCE_GE_DL_SIGNAL_SYNC) {
                    // 42D4
                    dl->list += 2;
                }
                // 4268
            }
            // 426C
            dl->state = SCE_GE_DL_STATE_PAUSED;
            dl->signal = SCE_GE_DL_SIGNAL_BREAK;
            HW_GE_STALLADDR = 0;
            HW_GE_LISTADDR = (int)UUNCACHED(g_cmdList);
            HW_GE_INTERRUPT_TYPE4 = HW_GE_INTEND | HW_GE_INTFIN;
            HW_GE_EXEC = 1;
            pspSync();
            g_AwQueue.isBreak = 1;
            g_AwQueue.curRunning = NULL;
            ret = (int)dl ^ g_dlMask;
        } else if (dl->state == SCE_GE_DL_STATE_PAUSED) {
            // 4130
            ret = SCE_ERROR_BUSY;
            if (g_AwQueue.sdkVer > 0x02000010) {
                if (dl->signal == SCE_GE_DL_SIGNAL_PAUSE) {
                    // 4160
                    Kprintf("sceGeBreak(): can't break signal-pausing list\n"); // 64B4
                } else
                    ret = SCE_ERROR_ALREADY;
            }
        } else if (dl->state == SCE_GE_DL_STATE_QUEUED) {
            // 40FC
            dl->state = SCE_GE_DL_STATE_PAUSED;
            ret = (int)dl ^ g_dlMask;
        } else if (g_AwQueue.sdkVer >= 0x02000000)
            ret = SCE_ERROR_NOT_SUPPORTED;
        else
            ret = -1;
    } else
        ret = SCE_ERROR_ALREADY;

    // 410C
    if (ret == 0 && g_GeLogHandler != NULL)
        g_GeLogHandler(SCE_GE_LOG_DL_BREAK, resetQueue);
    sceKernelCpuResumeIntr(oldIntr);
    pspSetK1(oldK1);
    return ret;
}

int sceGeContinue()
{
    int oldK1 = pspShiftK1();
    int ret = 0;
    int oldIntr = sceKernelCpuSuspendIntr();
    SceGeDisplayList *dl = g_AwQueue.active_first;
    if (dl != NULL) {
        if (dl->state == SCE_GE_DL_STATE_PAUSED) {
            // 4444
            if (g_AwQueue.isBreak == 0) {
                // 4474
                if (dl->signal != SCE_GE_DL_SIGNAL_PAUSE) {
                    dl->state = SCE_GE_DL_STATE_RUNNING;
                    dl->signal = SCE_GE_DL_SIGNAL_NONE;
                    sceSysregAwRegABusClockEnable();
                    // 448C
                    while ((HW_GE_EXEC & 1) != 0)
                        ;
                    HW_GE_INTERRUPT_TYPE4 = HW_GE_INTEND | HW_GE_INTFIN;
                    if (dl->ctx != NULL && dl->ctxUpToDate == 0) {
                        // 4598
                        sceGeSaveContext(dl->ctx);
                    }
                    // 44BC
                    _sceGeWriteBp(dl->list);
                    dl->ctxUpToDate = 1;
                    HW_GE_LISTADDR = (int)UCACHED(dl->list);
                    HW_GE_STALLADDR = (int)UCACHED(dl->stall);
                    HW_GE_OADR = dl->oadr;
                    HW_GE_OADR1 = dl->oadr1;
                    HW_GE_OADR2 = dl->oadr2;
                    _sceGeSetBaseRadr(dl->base, dl->radr1, dl->radr2);
                    HW_GE_INTERRUPT_TYPE4 = HW_GE_INTEND | HW_GE_INTFIN;
                    HW_GE_EXEC = dl->flags | 1;
                    pspSync();
                    g_AwQueue.curRunning = dl;
                    sceKernelClearEventFlag(g_AwQueue.drawingEvFlagId, ~2);
                    if (g_GeLogHandler != NULL) {
                        g_GeLogHandler(SCE_GE_LOG_DL_CONTINUE, 0);
                        if (g_GeLogHandler != NULL)
                            g_GeLogHandler(SCE_GE_LOG_DL_RUNNING, (int)dl ^ g_dlMask, dl->list, dl->stall);
                    }
                } else {
                    // 45A8
                    ret = SCE_ERROR_BUSY;
                }
            } else {
                dl->state = SCE_GE_DL_STATE_QUEUED;
                if (g_GeLogHandler != NULL)
                    g_GeLogHandler(SCE_GE_LOG_DL_CONTINUE, 0);
            }
        } else if (dl->state == SCE_GE_DL_STATE_RUNNING) {
            // 4428
            if (g_AwQueue.sdkVer >= 0x02000000)
                ret = SCE_ERROR_ALREADY;
            else
                ret = -1;
        } else {
            if (g_AwQueue.sdkVer >= 0x02000000)
                ret = SCE_ERROR_NOT_SUPPORTED;
            else
                ret = -1;
        }

        // 43EC (duplicated above)
    }
    // 43F4
    sceKernelCpuResumeIntr(oldIntr);
    pspSetK1(oldK1);
    return ret;
}

int sceGeSetCallback(SceGeCallbackData * cb)
{
    int oldK1 = pspShiftK1();
    if (!pspK1PtrOk(cb)
        || !pspK1PtrOk(cb->signal_func) || !pspK1PtrOk(cb->finish_func)) {
        // 4604
        pspSetK1(oldK1);
        return SCE_ERROR_PRIV_REQUIRED;
    }
    // 4634
    int oldIntr = sceKernelCpuSuspendIntr();

    // 4650
    int i;
    for (i = 0; i < 16; i++) {
        if (((g_cbhook >> i) & 1) == 0) {
            // 46E4
            g_cbhook |= (1 << i);
            break;
        }
    }
    // 466C
    sceKernelCpuResumeIntr(oldIntr);
    if (i == 16) {
        // 46D8
        pspSetK1(oldK1);
        return SCE_ERROR_OUT_OF_MEMORY;
    }

    if (cb->signal_func != NULL) {
        sceKernelRegisterSubIntrHandler(SCE_GE_INT, i * 2 + 0, cb->signal_func,
                                        cb->signal_arg);
    }
    // 469C
    if (cb->finish_func != NULL) {
        sceKernelRegisterSubIntrHandler(SCE_GE_INT, i * 2 + 1, cb->finish_func,
                                        cb->finish_arg);
    }
    // 46B4
    sceKernelEnableSubIntr(SCE_GE_INT, i * 2 + 0);
    sceKernelEnableSubIntr(SCE_GE_INT, i * 2 + 1);
    pspSetK1(oldK1);
    return i;
}

int sceGePutBreakpoint(SceGeBreakpoint *bp, int size)
{
    if (size > 8)
        return SCE_ERROR_INVALID_SIZE;
    int oldIntr = sceKernelCpuSuspendIntr();
    int oldK1 = pspShiftK1();
    if (!pspK1DynBufOk(bp, size * 8)) {
        // 4850 dup
        pspSetK1(oldK1);
        sceKernelCpuResumeIntr(oldIntr);
        return SCE_ERROR_PRIV_REQUIRED;
    }
    // 477C
    int i;
    for (i = 0; i < size; i++) {
        int (*func) (int, int, int) = (void *)*(int *)(g_deci2p + 36);
        if (func(bp[i].bpAddr, 4, 3) < 0) {
            // 4850 dup
            pspSetK1(oldK1);
            sceKernelCpuResumeIntr(oldIntr);
            return SCE_ERROR_PRIV_REQUIRED;
        }
    }
    // 47A8
    _sceGeClearBp();
    g_GeDeciBreak.size = size;
    // 47C8
    for (i = 0; i < g_GeDeciBreak.size; i++) {
        g_GeDeciBreak.cmds[i].addr = bp[i].bpAddr & 0xFFFFFFFC;
        g_GeDeciBreak.cmds[i].count = bp[i].bpCount;
    }
    // 47F4
    if (g_GeDeciBreak.busy == 0) {
        // 4840
        _sceGeWriteBp(NULL);
    }
    pspSetK1(oldK1);
    // 4804
    sceKernelCpuResumeIntr(oldIntr);
    return 0;
}

int sceGeGetBreakpoint(SceGeBreakpoint *outPtr, int size, int *bpCount)
{
    int oldIntr = sceKernelCpuSuspendIntr();
    int oldK1 = pspShiftK1();
    if (!pspK1DynBufOk(outPtr, size * sizeof(*outPtr)) || !pspK1PtrOk(bpCount)) {
        // 48C0
        pspSetK1(oldK1);
        sceKernelCpuResumeIntr(oldIntr);
        return SCE_ERROR_PRIV_REQUIRED;
    }
    // 48EC
    SceGeBpCmd *bpCmd = &g_GeDeciBreak.cmds[0];
    int count = 0;
    // 4904
    int i;
    for (i = 0; i < g_GeDeciBreak.size; i++) {
        if (i < size) {
            count++;
            outPtr->bpAddr = bpCmd->addr & 0xFFFFFFFC;
            outPtr->bpCount = bpCmd->count;
            outPtr++;
        }
        // 4930
        bpCmd++;
    }
    // 4940
    if (bpCount != NULL)
        *bpCount = g_GeDeciBreak.size;
    // 4950
    pspSetK1(oldK1);
    sceKernelCpuResumeIntr(oldIntr);
    return count;
}

int sceGeGetListIdList(int *outPtr, int size, int *totalCountPtr)
{
    int oldIntr = sceKernelCpuSuspendIntr();
    int oldK1 = pspShiftK1();
    if (!pspK1DynBufOk(outPtr, size * 4) || !pspK1PtrOk(totalCountPtr)) {
        // 49B8
        pspSetK1(oldK1);
        sceKernelCpuResumeIntr(oldIntr);
        return SCE_ERROR_PRIV_REQUIRED;
    }
    // 49E8
    SceGeDisplayList *dl = g_AwQueue.active_first;
    int storedCount = 0;
    int totalCount = 0;
    // 4A04
    while (dl != NULL) {
        totalCount++;
        if (storedCount < size) {
            storedCount++;
            *(outPtr++) = (int)dl ^ g_dlMask;
        }
        // 4A20
        dl = dl->next;
    }
    // 4A2C
    if (totalCountPtr != NULL)
        *totalCountPtr = totalCount;

    // 4A34
    pspSetK1(oldK1);
    sceKernelCpuResumeIntr(oldIntr);
    return storedCount;
}

int sceGeGetList(int dlId, SceGeDisplayList * outDl, int *outFlag)
{
    int oldIntr = sceKernelCpuSuspendIntr();
    if (!pspK1PtrOk(outDl) || !pspK1PtrOk(outFlag)) {
        // 4A8C
        sceKernelCpuResumeIntr(oldIntr);
        return SCE_ERROR_PRIV_REQUIRED;
    }
    // 4AB8
    SceGeDisplayList *dl = (SceGeDisplayList *) (dlId ^ g_dlMask);
    if (dl < g_displayLists || dl >= &g_displayLists[64]) {
        // 4B40
        sceKernelCpuResumeIntr(oldIntr);
        return SCE_ERROR_INVALID_ID;
    }
    if (outDl != NULL) {
        // 4AE8
        __builtin_memcpy(__builtin_assume_aligned(outDl, 4), __builtin_assume_aligned(dl, 4), sizeof(SceGeDisplayList));
    }
    // 4B14
    if (outFlag != NULL)
        *outFlag = (dl->state << 2) | dl->signal;
    // 4B30
    sceKernelCpuResumeIntr(oldIntr);
    return 0;
}

int sceGeGetStack(int stackId, SceGeStack *stack)
{
    SceGeDisplayList *dl = g_AwQueue.active_first;
    if (dl == NULL)
        return 0;
    if (stackId < 0)
        return dl->stackOff;
    int oldIntr = sceKernelCpuSuspendIntr();
    int oldK1 = pspShiftK1();
    if (stackId >= dl->stackOff) {
        // 4C40
        pspSetK1(oldK1);
        sceKernelCpuResumeIntr(oldIntr);
        return SCE_ERROR_INVALID_INDEX;
    }
    if (!pspK1PtrOk(stack)) {
        // 4C2C
        pspSetK1(oldK1);
        sceKernelCpuResumeIntr(oldIntr);
        return SCE_ERROR_PRIV_REQUIRED;
    }
    if (stack != NULL) {
        SceGeStack *inStack = &dl->stack[stackId];
        stack->stack[0] = inStack->stack[0];
        stack->stack[1] = inStack->stack[1];
        stack->stack[2] = inStack->stack[2];
        stack->stack[3] = inStack->stack[3];
        stack->stack[4] = inStack->stack[4];
        stack->stack[5] = inStack->stack[5];
        stack->stack[6] = inStack->stack[6];
        stack->stack[7] = inStack->stack[7];
    }
    // 4C04
    pspSetK1(oldK1);
    sceKernelCpuResumeIntr(oldIntr);
    return dl->stackOff;
}

int sceGeDebugBreak()
{
    int oldIntr = sceKernelCpuSuspendIntr();
    if (g_GeDeciBreak.busy != 0) {
        // 4CF4
        sceKernelCpuResumeIntr(oldIntr);
        return SCE_ERROR_ALREADY;
    }
    g_GeDeciBreak.size2 = 0;
    g_GeDeciBreak.busy = 1;
    int wasEnabled = sceSysregAwRegABusClockEnable();
    HW_GE_EXEC = 0;
    // 4C9C
    while ((HW_GE_EXEC & 1) != 0)
        ;
    if (!wasEnabled) {
        // 4CE4
        sceSysregAwRegABusClockDisable();
    }
    // 4CB8
    _sceGeClearBp();
    sceKernelDisableIntr(SCE_GE_INT);
    sceKernelCpuResumeIntr(oldIntr);
    return 0;
}

int sceGeDebugContinue(int arg0)
{
    SceGeDisplayList *dl = g_AwQueue.active_first;
    if (dl == NULL)
        return 0;
    int oldIntr = sceKernelCpuSuspendIntr();
    if (g_GeDeciBreak.busy == 0) {
        // 50B0
        sceKernelCpuResumeIntr(oldIntr);
        return SCE_ERROR_BUSY;
    }
    int wasEnabled = sceSysregAwRegABusClockEnable();
    if ((HW_GE_INTERRUPT_TYPE1 & HW_GE_INTEND) != 0) {
        // 5084
        if (!wasEnabled) {
            // 50A0
            sceSysregAwRegABusClockDisable();
        }
        sceKernelCpuResumeIntr(oldIntr);
        return SCE_ERROR_BUSY;
    }
    int *cmdPtr = (int *)HW_GE_LISTADDR;
    u32 curCmd = *cmdPtr;
    int hasSignalFF = 0;
    if ((curCmd >> 24) == SCE_GE_CMD_SIGNAL && ((curCmd >> 16) & 0xFF) == SCE_GE_SIGNAL_BREAK2) // 5044
    {
        cmdPtr += 2;
        HW_GE_LISTADDR = (int)cmdPtr;
        hasSignalFF = 1;
        if (arg0 == 1) {
            if (!wasEnabled)
                sceSysregAwRegABusClockDisable();
            // 4EE8 dup
            sceKernelCpuResumeIntr(oldIntr);
            return 0;
        }
    }
    // (4D80)
    g_GeDeciBreak.busy = 0;
    // 4D84
    if (arg0 == 0) {
        // 4F44 dup
        g_GeDeciBreak.size2 = 0;
    } else {
        u32 curCmd = *cmdPtr;
        int *nextCmdPtr1 = cmdPtr + 1;
        int flag = HW_GE_EXEC;
        int op = curCmd >> 24;
        if ((op == SCE_GE_CMD_JUMP || op == SCE_GE_CMD_BJUMP) || (op == SCE_GE_CMD_CALL && arg0 == 1))
        {
            // 4DCC
            if (op != SCE_GE_CMD_BJUMP || (flag & 2) == 0) // 5038
                nextCmdPtr1 = (int *)((((HW_GE_CMD(SCE_GE_CMD_BASE) << 8) & 0xFF000000)
                                      | (curCmd & 0x00FFFFFF)) + HW_GE_OADR);
            // 4DFC
        }
        // 4E00
        int *nextCmdPtr2 = cmdPtr + 2;
        if (op == SCE_GE_CMD_RET)
        {
            // 5004
            if ((flag & 0x200) == 0) {
                // 5020
                if ((flag & 0x100) != 0)
                    nextCmdPtr1 = (int *)HW_GE_RADR1;
            } else
                nextCmdPtr1 = (int *)HW_GE_RADR2;
        } else {
            if (op == SCE_GE_CMD_FINISH)
                nextCmdPtr1 = nextCmdPtr2;
            else if (op == SCE_GE_CMD_SIGNAL)
            {
                // 4F54
                int signalOp = (curCmd >> 16) & 0x000000FF;
                int off = (curCmd << 16) | (*(cmdPtr + 1) & 0xFFFF);
                nextCmdPtr1 = nextCmdPtr2;
                if (signalOp != SCE_GE_SIGNAL_JUMP && (signalOp != SCE_GE_SIGNAL_CALL || arg0 != 1)) {
                    // 4F94
                    if (signalOp == SCE_GE_SIGNAL_RJUMP) {
                        // 4FAC
                        nextCmdPtr1 = cmdPtr + off;
                    } else {
                        if (signalOp != SCE_GE_SIGNAL_RCALL || arg0 != 1) {
                            // 4FB4
                            if (signalOp != SCE_GE_SIGNAL_OJUMP) {
                                if ((signalOp != SCE_GE_SIGNAL_OCALL || arg0 != 1) && signalOp == SCE_GE_SIGNAL_RET && dl->stackOff != 0)   // 4FDC
                                    nextCmdPtr1 = (int *)
                                        dl->stack[dl->stackOff - 1].stack[1];
                            } else {
                                // 4FCC
                                nextCmdPtr1 = (int *)HW_GE_OADR + off;
                            }
                        }
                    }
                } else {
                    // 4F8C
                    nextCmdPtr1 = (int *)off;
                }
            }
        }

        // (4E18)
        // (4E1C)
        // 4E20
        g_GeDeciBreak.size2 = 1;
        g_GeDeciBreak.cmds2[0].count = 1;
        g_GeDeciBreak.cmds2[0].addr = (int)nextCmdPtr1;
        SceGeBpCmd *bpCmd = &g_GeDeciBreak.cmds[0];
        // 4E50
        int i;
        for (i = 0; i < g_GeDeciBreak.size; i++) {
            // 4E50
            if (UCACHED(bpCmd->addr) == UCACHED(nextCmdPtr1)) {
                // 4F4C
                g_GeDeciBreak.size2 = 0;
                break;
            }
            bpCmd++;
        }
        // 4E6C
        if ((*nextCmdPtr1 >> 24) == SCE_GE_CMD_SIGNAL && ((*nextCmdPtr1 >> 16) & 0xFF) == SCE_GE_SIGNAL_BREAK2) { // 4F38
            // 4F44 dup
            g_GeDeciBreak.size2 = 0;
        }
    }
    // 4E80
    if (hasSignalFF == 0) {
        SceGeBpCmd *bpCmd = &g_GeDeciBreak.cmds[0];
        // 4EA0
        int i;
        for (i = 0; i < g_GeDeciBreak.size; i++) {
            if (UCACHED(bpCmd->addr) == UCACHED(cmdPtr)) {
                // 4F10
                bpCmd->addr |= 1;
                if (bpCmd->count != -1 && bpCmd->count != 0) {
                    bpCmd->count--;
                }
            }
            // 4EB4
            bpCmd++;
        }
    }
    // 4EC4
    _sceGeWriteBp(cmdPtr);
    HW_GE_EXEC |= 1;
    sceKernelEnableIntr(SCE_GE_INT);
    // 4EE8 dup
    sceKernelCpuResumeIntr(oldIntr);
    return 0;
}

int _sceGeQueueInitCallback()
{
    SceKernelUsersystemLibWork *libWork = sceKernelGetUsersystemLibWork();
    if (libWork != NULL) {
        // Pointer to the syscall in sub_00000208 (at 0x20C) in usersystemlib
        u32 *syscPtr = (void*)libWork->sceGeListUpdateStallAddr_lazy + 204; // TODO: replace with a difference of two functions from usersystemlib
        *syscPtr = ALLEGREX_MAKE_SYSCALL(g_AwQueue.syscallId);
        pspCache(0x1A, syscPtr);
        pspCache(0x08, syscPtr);
        g_AwQueue.lazySyncData = libWork->lazySyncData;
    }
    return 0;
}

int _sceGeQueueEnd()
{
    sceKernelDeleteEventFlag(g_AwQueue.drawingEvFlagId);
    sceKernelDeleteEventFlag(g_AwQueue.listEvFlagIds[0]);
    sceKernelDeleteEventFlag(g_AwQueue.listEvFlagIds[1]);
    return 0;
}

int _sceGeQueueStatus(void)
{
    if (g_AwQueue.active_first == NULL)
        return 0;
    return -1;
}

void _sceGeErrorInterrupt(int arg0 __attribute__((unused)), int arg1 __attribute__((unused)), int arg2 __attribute__((unused)))
{
}

int sceGeListEnQueue(void *list, void *stall, int cbid, SceGeListArgs * arg)
{
    return _sceGeListEnQueue(list, stall, cbid, arg, 0);
}

int sceGeListEnQueueHead(void *list, void *stall, int cbid, SceGeListArgs * arg)
{
    return _sceGeListEnQueue(list, stall, cbid, arg, 1);
}

int sceGeUnsetCallback(int cbId)
{
    int oldK1 = pspShiftK1();
    if (cbId < 0 || cbId >= 16) {
        // 528C
        pspSetK1(oldK1);
        return SCE_ERROR_INVALID_ID;
    }
    int oldIntr = sceKernelCpuSuspendIntr();
    sceKernelDisableSubIntr(SCE_GE_INT, cbId * 2 + 0);
    sceKernelDisableSubIntr(SCE_GE_INT, cbId * 2 + 1);
    sceKernelReleaseSubIntrHandler(SCE_GE_INT, cbId * 2 + 0);
    sceKernelReleaseSubIntrHandler(SCE_GE_INT, cbId * 2 + 1);
    g_cbhook &= ~(1 << cbId);
    sceKernelCpuResumeIntr(oldIntr);
    pspSetK1(oldK1);
    return 0;
}

void _sceGeListError(u32 cmd, int err)  // err: 0 = alignment problem, 1: overflow, 2: underflow
{
    int op = (cmd >> 16) & 0xFF;
    int *curAddr = (int *)HW_GE_LISTADDR - 2;
    char *cmdStr;
    switch (cmd)                // 0x6794 jump table
    {
    case SCE_GE_SIGNAL_JUMP:
        // 52D8
        cmdStr = "JUMP";
        break;

    case SCE_GE_SIGNAL_CALL:
        // 5358
        cmdStr = "CALL";
        break;

    case SCE_GE_SIGNAL_RET:
        // 5364
        cmdStr = "RET";
        break;

    case SCE_GE_SIGNAL_RJUMP:
        // 5370
        cmdStr = "RJUMP";
        break;

    case SCE_GE_SIGNAL_RCALL:
        // 537C
        cmdStr = "RCALL";
        break;

    case SCE_GE_SIGNAL_OJUMP:
        // 5388
        cmdStr = "OJUMP";
        break;

    case SCE_GE_SIGNAL_OCALL:
        // 5394
        cmdStr = "OCALL";
        break;

    case SCE_GE_SIGNAL_RCBP:
        // 53A0
        cmdStr = "RCBP";
        break;

    case SCE_GE_SIGNAL_OCBP:
        // 53D8
        cmdStr = "OCBP";
        break;

    default: // RTBP0~7 / OTBP0~7
        // 53AC
        if (op >= SCE_GE_SIGNAL_OTBP0)
            g_szTbp[0] = 'O';   // at 6880
        // 53C4
        cmdStr = g_szTbp;
        cmdStr[4] = '0' + (op & 7);
        break;
    }

    // 52E0
    switch (err) {
    case SCE_GE_SIGNAL_ERROR_INVALID_ADDRESS:
        // 533C
        Kprintf("SCE_GE_SIGNAL_%s address error (MADR=0x%08X)\n",
                cmdStr, curAddr);
        break;

    case SCE_GE_SIGNAL_ERROR_STACK_OVERFLOW:
        // 534C
        Kprintf("SCE_GE_SIGNAL_%s stack overflow (MADR=0x%08X)\n",
                cmdStr, curAddr);
        break;

    case SCE_GE_SIGNAL_ERROR_STACK_UNDERFLOW:
        // 5328
        Kprintf("SCE_GE_SIGNAL_%s stack underflow (MADR=0x%08X)\n",
                cmdStr, curAddr);
        break;
    }
    // 52FC
    if (sceKernelGetCompiledSdkVersion() >= 0x02000000)
        pspBreak(0);
}

void _sceGeWriteBp(int *list)
{
    if (g_deci2p == NULL)
        return;
    if (g_GeDeciBreak.bpSet)
        return;
    g_GeDeciBreak.bpSet = 1;
    int *prevList = list - 1;
    int i;
    for (i = 0; i < g_GeDeciBreak.size; i++) {
        SceGeBpCmd *bpCmd = &g_GeDeciBreak.cmds[i];
        int *ptr2 = (int *)(bpCmd->addr & 0xFFFFFFFC);
        u32 cmd = ptr2[0];
        bpCmd->oldCmd1 = cmd;
        bpCmd->oldCmd2 = ptr2[1];
        if (((bpCmd->addr ^ (int)prevList) & 0x1FFFFFFC) != 0) {
            // 5574
            if ((bpCmd->addr & 3) == 0) {
                // 55C4
                if (cmd >> 24 != SCE_GE_CMD_END)
                    ptr2[0] = GE_MAKE_OP(SCE_GE_CMD_SIGNAL, SCE_GE_SIGNAL_BREAK2 << 24);
                // 55CC
                ptr2[1] = GE_MAKE_OP(SCE_GE_CMD_END, 0);
            } else
                bpCmd->addr = (int)ptr2;
            // 5580
            pspCache(0x1A, &ptr2[0]);
            pspCache(0x1A, &ptr2[1]);
            if ((pspCop0StateGet(24) & 1) != 0) {
                pspSync();
                pspL2CacheWriteback1(ptr2, 1);
            }
        }
        // 5494
    }

    // 54A4
    // 54E0
    for (i = 0; i < g_GeDeciBreak.size2; i++) {
        SceGeBpCmd *bpCmd = &g_GeDeciBreak.cmds2[i];
        int *ptr = (int *)bpCmd->addr;
        bpCmd->oldCmd1 = ptr[0];
        bpCmd->oldCmd2 = ptr[1];
        if ((((int)ptr ^ (int)prevList) & 0x1FFFFFFC) != 0) {
            // 5528
            ptr[0] = GE_MAKE_OP(SCE_GE_CMD_SIGNAL, SCE_GE_SIGNAL_BREAK1 << 24);
            ptr[1] = GE_MAKE_OP(SCE_GE_CMD_END, 0);
            pspCache(0x1A, &ptr[0]);
            pspCache(0x1A, &ptr[1]);
            if ((pspCop0StateGet(24) & 1) != 0) {
                pspSync();
                pspL2CacheWriteback1(ptr, 1);
            }
        }
        // 5504
    }

    // 5514
    pspSync();
}

void _sceGeClearBp()
{
    if (g_deci2p == NULL)
        return;
    if (!g_GeDeciBreak.bpSet)
        return;
    g_GeDeciBreak.bpSet = 0;
    // 5624
    int i;
    for (i = 0; i < g_GeDeciBreak.size2; i++) {
        SceGeBpCmd *cmd = &g_GeDeciBreak.cmds2[g_GeDeciBreak.size2 - i - 1];
        int *out = (int *)cmd->addr;
        out[0] = cmd->oldCmd1;
        out[1] = cmd->oldCmd2;
        pspCache(0x1A, &out[0]);
        pspCache(0x1A, &out[1]);
        if ((pspCop0StateGet(24) & 1) != 0) {
            pspSync();
            pspL2CacheWriteback1(out, 1);
        }
        // 5684
    }

    // 5694, 56C4
    for (i = 0; i < g_GeDeciBreak.size; i++) {
        SceGeBpCmd *cmd = &g_GeDeciBreak.cmds[g_GeDeciBreak.size - i - 1];
        int *out = (int *)(cmd->addr & 0xFFFFFFFC);
        out[0] = cmd->oldCmd1;
        out[1] = cmd->oldCmd2;
        pspCache(0x1A, &out[0]);
        pspCache(0x1A, &out[1]);
        if ((pspCop0StateGet(24) & 1) != 0) {
            pspSync();
            pspL2CacheWriteback1(out, 1);
        }
        // 5728
    }
    // 5738
    pspSync();
}

void _sceGeListLazyFlush()
{
    SceGeLazy *lazy = g_AwQueue.lazySyncData;
    if (lazy != NULL && lazy->dlId >= 0) {
        lazy->max = 0;
        sceGeListUpdateStallAddr(lazy->dlId, lazy->stall);
        lazy->dlId = -1;
    }
}

int _sceGeListEnQueue(void *list, void *stall, int cbid, SceGeListArgs *arg, int head)
{
    int oldK1 = pspShiftK1();
    if (!pspK1PtrOk(list) || !pspK1PtrOk(stall) || !pspK1StaBufOk(arg, 16)) {
        pspSetK1(oldK1);
        return SCE_ERROR_PRIV_REQUIRED;
    }
    // 5834
    int ver = sceKernelGetCompiledSdkVersion();
    g_AwQueue.sdkVer = ver;
    void *stack = &g_AwQueue.stack;
    SceGeContext *ctx = NULL;
    int numStacks = 32;
    // If set to 1, it will ignore duplicates
    int oldVer = 1;
    if (ver > 0x01FFFFFF) {
        numStacks = 0;
        stack = NULL;
        oldVer = 0;
    }

    // 5878
    if (arg != NULL) {
        ctx = arg->ctx;
        if (!pspK1StaBufOk(ctx, 2048)) {
            pspSetK1(oldK1);
            return SCE_ERROR_PRIV_REQUIRED;
        }
        if (arg->size >= 16) {
            numStacks = arg->numStacks;
            if (numStacks >= 256) {
                // 5CBC
                pspSetK1(oldK1);
                return SCE_ERROR_INVALID_SIZE;
            }
            if (!pspK1DynBufOk(arg->stacks, numStacks * 32)) {
                pspSetK1(oldK1);
                return SCE_ERROR_PRIV_REQUIRED;
            }
        }
    }
    // (58DC)
    // 58E0
    if ((((int)list | (int)stall | (int)ctx | (int)stack) & 3) != 0) {
        // 5CAC
        pspSetK1(oldK1);
        return SCE_ERROR_INVALID_POINTER;
    }
    if (g_AwQueue.patched != 0 && pspK1IsUserMode()) {    // 5C94
        g_AwQueue.patched--;
        sceKernelDcacheWritebackAll();
    }

    // 58FC
    int oldIntr = sceKernelCpuSuspendIntr();
    _sceGeListLazyFlush();
    SceGeDisplayList *dl = g_AwQueue.active_first;
    // 5920
    while (dl != NULL) {
        if (UCACHED((int)dl->list ^ (int)list) == NULL) {
            // 5C74
            Kprintf("_sceGeListEnQueue(): can't enqueue duplicated addr(MADR=0x%08X)\n", list); // 0x65B8
            if (oldVer)
                break;
            sceKernelCpuResumeIntr(oldIntr);
            pspSetK1(oldK1);
            return SCE_ERROR_BUSY;
        }
        if (dl->ctxUpToDate && stack != NULL && dl->stack == stack && !oldVer) { // 5C44
            Kprintf("_sceGeListEnQueue(): can't enqueue duplicated stack(STACK=0x%08X)\n", stack);  // 0x65FC
            // 5C60
            sceKernelCpuResumeIntr(oldIntr);
            pspSetK1(oldK1);
            return SCE_ERROR_BUSY;
        }
        dl = dl->next;
        // 5954
    }

    // 5960
    dl = g_AwQueue.free_first;
    if (dl == NULL) {
        // 5C24
        sceKernelCpuResumeIntr(oldIntr);
        pspSetK1(oldK1);
        return SCE_ERROR_OUT_OF_MEMORY;
    }
    if (dl->next == NULL) {
        g_AwQueue.free_last = NULL;
        // 5C3C
        g_AwQueue.free_first = NULL;
    } else {
        g_AwQueue.free_first = dl->next;
        dl->next->prev = NULL;
    }

    // 5984
    dl->prev = NULL;
    dl->next = NULL;
    if (dl == NULL) {
        // 5C24
        sceKernelCpuResumeIntr(oldIntr);
        pspSetK1(oldK1);
        return SCE_ERROR_OUT_OF_MEMORY;
    }
    dl->ctxUpToDate = 0;
    dl->signal = SCE_GE_DL_SIGNAL_NONE;
    dl->cbId = pspMax(cbid, -1);
    int dlId = (int)dl ^ g_dlMask;
    dl->numStacks = numStacks;
    dl->stack = stack;
    dl->next = NULL;
    dl->ctx = ctx;
    dl->flags = 0;
    dl->list = list;
    dl->stall = stall;
    dl->radr1 = 0;
    dl->radr2 = 0;
    dl->oadr = 0;
    dl->oadr1 = 0;
    dl->oadr2 = 0;
    dl->base = 0;
    dl->stackOff = 0;
    if (head != 0) {
        // 5B8C
        if (g_AwQueue.active_first != NULL) {
            // 5BE0
            if (g_AwQueue.active_first->state != SCE_GE_DL_STATE_PAUSED) {
                // 5C0C
                sceKernelCpuResumeIntr(oldIntr);
                pspSetK1(oldK1);
                return SCE_ERROR_INVALID_VALUE;
            }
            dl->state = SCE_GE_DL_STATE_PAUSED;
            g_AwQueue.active_first->state = SCE_GE_DL_STATE_QUEUED;
            dl->prev = NULL;
            g_AwQueue.active_first->prev = dl;
            dl->next = g_AwQueue.active_first;
        } else {
            dl->state = SCE_GE_DL_STATE_PAUSED;
            dl->prev = NULL;
            dl->next = NULL;
            g_AwQueue.active_last = dl;
        }

        // 5BB8
        g_AwQueue.active_first = dl;
        if (g_GeLogHandler != NULL)
            g_GeLogHandler(SCE_GE_LOG_DL_ENQUEUED, dlId, 1, list, stall);
    } else if (g_AwQueue.active_first == NULL) {
        // 5A8C
        dl->state = SCE_GE_DL_STATE_RUNNING;
        dl->ctxUpToDate = 1;
        dl->prev = NULL;
        g_AwQueue.active_first = dl;
        g_AwQueue.active_last = dl;
        sceSysregAwRegABusClockEnable();
        if (ctx != NULL)
            sceGeSaveContext(ctx);

        // 5ABC
        _sceGeWriteBp(list);
        HW_GE_EXEC = 0;
        HW_GE_LISTADDR = (int)UCACHED(list);
        HW_GE_STALLADDR = (int)UCACHED(stall);
        HW_GE_OADR = 0;
        HW_GE_OADR1 = 0;
        HW_GE_OADR2 = 0;
        HW_GE_RADR1 = 0;
        HW_GE_RADR2 = 0;
        pspSync();
        HW_GE_EXEC = 1;
        pspSync();
        g_AwQueue.curRunning = dl;
        sceKernelClearEventFlag(g_AwQueue.drawingEvFlagId, 0xFFFFFFFD);
        if (g_GeLogHandler != NULL) {
            g_GeLogHandler(SCE_GE_LOG_DL_ENQUEUED, dlId, 0, list, stall);
            if (g_GeLogHandler != NULL)
                g_GeLogHandler(SCE_GE_LOG_DL_RUNNING, dlId, list, stall);
        }
    } else {
        dl->state = SCE_GE_DL_STATE_QUEUED;
        g_AwQueue.active_last->next = dl;
        dl->prev = g_AwQueue.active_last;
        g_AwQueue.active_last = dl;
        if (g_GeLogHandler != NULL) {
            // 5A6C
            g_GeLogHandler(SCE_GE_LOG_DL_ENQUEUED, dlId, 0, list, stall);
        }
    }

    // 5A28
    // clear event flag for this DL
    u32 idx = (dl - g_displayLists) / sizeof(SceGeDisplayList);
    sceKernelClearEventFlag(g_AwQueue.listEvFlagIds[idx / 32], ~(1 << (idx % 32)));

    sceKernelCpuResumeIntr(oldIntr);
    pspSetK1(oldK1);
    return dlId;
}

