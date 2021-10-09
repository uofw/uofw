/* Copyright (C) 2011, 2012 The uOFW team
   See the file COPYING for copying permission.
*/

/*
 * This module is the interface with the GE, or Graphics Engine, PSP's GPU.
 *
 * The GE, also referred as 'Aw' in some function & variable names (this name
 * being undocumented also in official documentations), and as RE+SE-90nm (Surface
 * Engine + Rendering Engine), is a custom ASIC.
 *
 * Apart for some initialization stuff, mainly for the GE's Edram clock etc.,
 * the low-level way to use the GE is very straightforward:
 * 1) build a list of commands to send to the GE, called 'display list'
 * 2) send its address to the GE
 * 3) (optional) set a stall address so that GE's execution stops at a given address
 *    (allows to send partial display lists to the GE)
 * 4) set interrupt handlers for interrupts which will be triggered by the GE when
 *    it encounters certain commands (END, FINISH and SIGNAL)
 *
 * This module is responsible for:
 * - giving a higher-level interface to the GE
 * - handling queues of display lists and execution & drawing synchronization
 * - handling certain SIGNAL instructions to enhance the GE's abilities (for example,
 *   allowing the user to make calls with greater depth)
 * - giving a debugging interface to the developer: breakpoints, step-by-step execution, etc.
 *
 * Note that since even this module is quite low-level — although it's mostly accessible
 * on the user level — most people use the GU user library (provided by Sony), which allows
 * doing most of the drawing oneself, especially considering Sony does not provide a full
 * specification of the GE commands.
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

// Definitions for the GE interrupt handler (32 subintrs max, no callbacks)
SceIntrCb g_GeSubIntrFunc = { // 6640
    0, 0, NULL, NULL,
    NULL, NULL, NULL, NULL,
    NULL, NULL, NULL
};

SceIntrHandler g_GeIntrOpt = { sizeof(SceIntrHandler), 0x00000020, &g_GeSubIntrFunc }; // 666C

// Deci2Ops aka debugging functions accessible through developer tools
SceKernelDeci2Ops g_Deci2Ops = { 0x3C, // 6678
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

// Bitfield containing the GE commands than can be reset just by running the command without an argument
// and reset just by running the command as it was last ran
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

// Hardware addresses behind registers in sceGeGetReg()/sceGeSetReg()
volatile void *g_pAwRegAdr[32] = { // 66D4
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

// Patch address for the Genso Suikoden I&II game (check _sceGeInitCallback3/4 for more info)
SadrUpdate sadrupdate_bypass = { "ULJM05086", (void *)0x08992414 }; // 6754

// Information needed to set the matrices
SceGeMatrix mtxtbl[SCE_GE_MTX_COUNT] = { // 675C
    {SCE_GE_CMD_BONEN, 0x00, SCE_GE_CMD_BONED, 0x0C},
    {SCE_GE_CMD_BONEN, 0x0C, SCE_GE_CMD_BONED, 0x0C},
    {SCE_GE_CMD_BONEN, 0x18, SCE_GE_CMD_BONED, 0x0C},
    {SCE_GE_CMD_BONEN, 0x24, SCE_GE_CMD_BONED, 0x0C},
    {SCE_GE_CMD_BONEN, 0x30, SCE_GE_CMD_BONED, 0x0C},
    {SCE_GE_CMD_BONEN, 0x3C, SCE_GE_CMD_BONED, 0x0C},
    {SCE_GE_CMD_BONEN, 0x48, SCE_GE_CMD_BONED, 0x0C},
    {SCE_GE_CMD_BONEN, 0x54, SCE_GE_CMD_BONED, 0x0C},
    {SCE_GE_CMD_WORLDN, 0x00, SCE_GE_CMD_WORLDD, 0x0C},
    {SCE_GE_CMD_VIEWN, 0x00, SCE_GE_CMD_VIEWD, 0x0C},
    {SCE_GE_CMD_PROJN, 0x00, SCE_GE_CMD_PROJD, 0x10},
    {SCE_GE_CMD_TGENN, 0x00, SCE_GE_CMD_TGEND, 0x0C}
};

// Display list ending sequence
int stopCmd[] = { // 678C
    GE_MAKE_OP(SCE_GE_CMD_FINISH, 0),
    GE_MAKE_OP(SCE_GE_CMD_END, 0)
};

// The GE system event handler (for running _sceGeSysEventHandler() on suspend & resume)
SceSysEventHandler g_GeSysEv = // 6840
    { 64, "SceGe", 0x00FFFF00, _sceGeSysEventHandler, 0, 0, NULL, {0, 0, 0, 0, 
                                                                   0, 0, 0, 0,
                                                                   0}
};

// Buffer used to store the strings RTBP0~7 / OTBP0~7
char g_szTbp[] = "RTBP0"; // 6880

// A Ge logger, which presumably can be set by developers, used to log some Ge events
// (running/ending a display list, signals, ...)
SceGeLogHandler g_GeLogHandler; // 6890

// The GE context, saved on reset & suspend (and restored on resume)
_SceGeContext _aw_ctx; // 68C0

// Set by sceGeEdramInit() and returned by sceGeEdramGetHwSize(), contains the Edram
// hardware size (2MB or 4MB depending on the model)
int g_uiEdramHwSize; // 70C0

// Set by sceGeEdramInit() or sceGeEdramSetSize() and returned by sceGeEdramGetSize(),
// contains the Edram size currently enabled
int g_uiEdramSize; // 70C4

// The current edram memory width
u16 g_edramAddrTrans; // 70C8

// The display list queue
SceGeQueue g_AwQueue; // 70CC

// Saved status of the current queue (on suspend & resuem)
SceGeQueueSuspendInfo g_GeSuspend; // 7500

// Breakpoint information
SceGeBpCtrl g_GeDeciBreak; // 7520

// Bitset: i-th bit is set if i-th callback is set (by sceGeSetCallback())
u16 g_cbhook; // 75D0

// TODO
int *g_cmdList; // 7600

// The mask xor'ed with the display list addresses to give the display list ID
u32 g_dlMask; // 7604

// The deci2p operations set by the developper or devkit (presumably)
SceKernelDeci2Ops *g_deci2p; // 7608

// Buffer of display lists (used as a linked list, so their order in this list is not relevant)
SceGeDisplayList g_displayLists[MAX_COUNT_DL]; // 7640

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
    sceGeSaveContext((SceGeContext*)&_aw_ctx);
    _aw_ctx.geometryClock = HW_GE_GEOMETRY_CLOCK;
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
    HW_GE_GEOMETRY_CLOCK = _aw_ctx.geometryClock;
    sceSysregSetMasterPriv(64, 1);
    // Restore the GE context
    sceGeRestoreContext((SceGeContext*)&_aw_ctx);
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
    u32 *dlist = _aw_ctx.dl;
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
    SceKernelDeci2Ops *ops = sceKernelDeci2pReferOperations();
    g_deci2p = ops;
    if (ops != NULL) {
        // 05D4
        ops->ops[SCE_DECI2OP_GE_SETOPS](&g_Deci2Ops);
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
    // Execute the generated display list
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
    // Wait for the execution to finish
    while ((HW_GE_INTERRUPT_TYPE1 & HW_GE_INTFIN) == 0)
        ;
    // Restore previous status
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
    if (id < 0 || id >= SCE_GE_MTX_COUNT)
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
        // This case includes BONE matrices but also the WORLD and VIEW matrices which
        // follow it and have the same size (while PROJ has a different size and TGEN follows it
        // so can't be included here)
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
    if (id < 0 || id >= SCE_GE_MTX_COUNT)
        return SCE_ERROR_INVALID_INDEX;
    int oldK1 = pspShiftK1();
    if (!pspK1PtrOk(mtx)) {
        // 16B8
        pspSetK1(oldK1);
        return SCE_ERROR_PRIV_REQUIRED;
    }
    int oldIntr = sceKernelCpuSuspendIntr();
    // Get the matrix info
    SceGeMatrix *curMtx = &mtxtbl[id];
    // Run the initialization command (for sending the instruction we're setting the matrix)
    // after having saved its old state
    u8 oldArg = sceGeGetCmd(curMtx->initCmd);
    ret = sceGeSetCmd(curMtx->initCmd, curMtx->initCmdArg);
    if (ret >= 0) {
        // 1644
        // Set the matrix
        int i;
        for (i = 0; i < curMtx->size; i++) {
            sceGeSetCmd(curMtx->setCmd, mtx[i]);
        }

        // 165C
        // Reset the initialization command argument
        sceGeSetCmd(curMtx->initCmd, oldArg);
        ret = 0;
    }
    // 1674
    sceKernelCpuResumeIntr(oldIntr);
    pspSetK1(oldK1);
    return ret;
}

int sceGeSaveContext(SceGeContext * _ctx)
{
    _SceGeContext *ctx = (_SceGeContext*)_ctx;
    if (!ISALIGN4(ctx)) {
        return SCE_ERROR_INVALID_POINTER;
    }
    int oldK1 = pspShiftK1();
    if (!pspK1StaBufOk(ctx, sizeof(*ctx))) {
        // 1AA0
        pspSetK1(oldK1);
        return SCE_ERROR_PRIV_REQUIRED;
    }
    int oldIntr = sceKernelCpuSuspendIntr();
    int aBusWasEnabled = sceSysregAwRegABusClockEnable();
    // If GE is running, abort
    if ((HW_GE_EXEC & 1) != 0) {
        // 1A8C
        sceKernelCpuResumeIntr(oldIntr);
        pspSetK1(oldK1);
        return -1;
    }
    // Save all the main registers for the display list runtime
    ctx->exec = HW_GE_EXEC;
    ctx->ladr = HW_GE_LISTADDR;
    ctx->sadr = HW_GE_STALLADDR;
    ctx->radr1 = HW_GE_RADR1;
    ctx->radr2 = HW_GE_RADR2;
    ctx->vadr = HW_GE_VADR;
    ctx->iadr = HW_GE_IADR;
    ctx->oadr = HW_GE_OADR;
    ctx->oadr1 = HW_GE_OADR1;
    ctx->oadr2 = HW_GE_OADR2;

    // Generate the commands to run on resume
    u32 *curCmd = ctx->dl;
    // 17C8
    // The commands in save_regs can be saved as-is
    int i;
    for (i = 0; i < 256; i++) {
        if (((save_regs[i / 8] >> (i & 7)) & 1) != 0)
            *(curCmd++) = HW_GE_CMD(i);
        // 1804
    }
    // Check the address for CLOAD if a real address was specified by CBP/CBW
    int addr = (HW_GE_CMD(SCE_GE_CMD_CBP) & 0x00FFFFFF) | ((HW_GE_CMD(SCE_GE_CMD_CBW) << 8) & 0xFF000000);
    if (GE_VALID_ADDR(addr)) {
        // 1868
        *(curCmd++) = HW_GE_CMD(SCE_GE_CMD_CLOAD);
    }
    // Save all the matrix statuses
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

    // Reset matrices numbers
    *(curCmd++) = HW_GE_CMD(SCE_GE_CMD_BONEN);
    *(curCmd++) = HW_GE_CMD(SCE_GE_CMD_WORLDN);
    *(curCmd++) = HW_GE_CMD(SCE_GE_CMD_VIEWN);
    *(curCmd++) = HW_GE_CMD(SCE_GE_CMD_PROJN);
    *(curCmd++) = HW_GE_CMD(SCE_GE_CMD_TGENN);
    *(curCmd++) = GE_MAKE_OP(SCE_GE_CMD_END, 0);
    sceKernelDcacheWritebackInvalidateRange(ctx->dl, sizeof(ctx->dl));
    if (!aBusWasEnabled)
        sceSysregAwRegABusClockDisable();   // 1A3C
    // 1A0C
    sceKernelCpuResumeIntr(oldIntr);
    pspSetK1(oldK1);
    return 0;
}

int sceGeRestoreContext(SceGeContext * _ctx)
{
    _SceGeContext *ctx = (_SceGeContext*)_ctx;
    int ret = 0;
    if (!ISALIGN4(ctx)) {
        return SCE_ERROR_INVALID_POINTER;
    }
    int oldK1 = pspShiftK1();
    if (!pspK1StaBufOk(ctx, sizeof(*ctx))) {
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
    // Execute the display list built in sceGeSaveContext()
    int oldIntr1 = HW_GE_INTERRUPT_TYPE1;
    int oldIntr2 = HW_GE_INTERRUPT_TYPE2;
    HW_GE_INTERRUPT_TYPE3 = oldIntr2;
    HW_GE_LISTADDR = (int)UCACHED(ctx->dl);
    HW_GE_STALLADDR = 0;
    HW_GE_EXEC = ctx->exec | 1;
    // 1B64
    while ((HW_GE_EXEC & 1) != 0)
        ;
    // Reset interrupt status and save the registers
    int intrType = HW_GE_INTERRUPT_TYPE1;
    HW_GE_INTERRUPT_TYPE4 = (oldIntr1 ^ HW_GE_INTERRUPT_TYPE1) & ~(HW_GE_INTFIN | HW_GE_INTSIG);
    HW_GE_INTERRUPT_TYPE2 = oldIntr2;
    if ((intrType & HW_GE_INTERR) != 0)
        ret = -1;
    HW_GE_LISTADDR = ctx->ladr;
    HW_GE_STALLADDR = ctx->sadr;
    HW_GE_VADR = ctx->vadr;
    HW_GE_IADR = ctx->iadr;
    HW_GE_OADR = ctx->oadr;
    HW_GE_OADR1 = ctx->oadr1;
    HW_GE_OADR2 = ctx->oadr2;
    _sceGeSetInternalReg(SCE_GE_INTERNAL_REG_RADR1 | SCE_GE_INTERNAL_REG_RADR2, 0, ctx->radr1, ctx->radr2);
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

/*
 * Set the first (ie after the first CALL command) return address register (needs special care compared to the other registers)
 */
int _sceGeSetRegRadr1(int radr1)
{
    return _sceGeSetInternalReg(SCE_GE_INTERNAL_REG_RADR1, 0, radr1, 0);
}

/*
 * Set the second (ie after the second CALL command) return address register (needs special care compared to the other registers)
 */
int _sceGeSetRegRadr2(int radr2)
{
    return _sceGeSetInternalReg(SCE_GE_INTERNAL_REG_RADR2, 0, 0, radr2);
}

/*
 * Used to safely set some internal registers: BASE, RADR1 and RADR2 (return addresses after a CALL)
 */
int _sceGeSetInternalReg(int type, int base, int radr1, int radr2)
{
    int *cmdList = g_cmdList;
    if (cmdList == NULL)
        return 0;
    int oldIntr = sceKernelCpuSuspendIntr();
    int oldIntrType = HW_GE_INTERRUPT_TYPE1;
    int oldExec = HW_GE_EXEC;
    int oldLadr = HW_GE_LISTADDR;
    int oldSadr = HW_GE_STALLADDR;
    int oldOadr = HW_GE_OADR;
    int oldOadr1 = HW_GE_OADR1;
    int oldOadr2 = HW_GE_OADR2;
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
    if ((type & SCE_GE_INTERNAL_REG_RADR1) && radr1 != 0) {
        // In order to set RADR1, we need to make a CALL from radr1 - 4 after setting BASE correctly
        int *uncachedNewCmdList = UCACHED(radr1 - 4);
        // When using CALL, the OADR is added, so we need to subtract it to jump to the absolute address we want
        u32 relAddr = (u32) (uncachedCmdList - oldOadr);
        // Backup the command at radr1 - 4, and set it to a CALL as wanted
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
        // Execute the first display list: set BASE
        HW_GE_LISTADDR = (int)UCACHED(cmdList + 1);
        HW_GE_EXEC = 1;
        // 1E4C
        while ((HW_GE_EXEC & 1) != 0)
            ;
        // Execute the second display list: make a CALL from the correct place, to cmdList (just an END)
        HW_GE_LISTADDR = (int)UCACHED(uncachedNewCmdList);
        HW_GE_EXEC = 1;
        // 1E74
        while ((HW_GE_EXEC & 1) != 0)
            ;
        // Restore the modified command
        uncachedNewCmdList[0] = oldCmd;
        if ((pspCop0StateGet(24) & 1) != 0) {
            pspSync();
            pspL2CacheWriteback1(uncachedNewCmdList, 0);
        }
    }
    // 1ED0
    // 1ED4
    if ((type & SCE_GE_INTERNAL_REG_RADR2) && radr2 != 0) {
        // This is basically the same as above
        int *uncachedNewCmdList = UCACHED(radr2 - 4);
        u32 relAddr = (u32)(uncachedCmdList - oldOadr);
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
        // Only difference compared to above: we set the execution flag bit 0x100 too so that we're considered to be in a depth 1 call already and set RADR2 instead of RADR1
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
    // Set the display list setting BASE & run it
    cmdList[0] = base;
    // 2010
    cmdList[1] = HW_GE_CMD(SCE_GE_CMD_END);
    HW_GE_LISTADDR = (int)uncachedCmdList;
    pspSync();
    HW_GE_EXEC = oldExec | 1;

    // 2034
    while ((HW_GE_EXEC & 1) != 0)
        ;

    // Restore old status
    HW_GE_LISTADDR = oldLadr;
    HW_GE_STALLADDR = oldSadr;
    HW_GE_OADR = oldOadr;
    HW_GE_OADR1 = oldOadr1;
    HW_GE_OADR2 = oldOadr2;

    // Also set RADR1 & RADR2 by hand (I guess the commands above are not enough?)
    if ((type & SCE_GE_INTERNAL_REG_RADR1) != 0)
        HW_GE_RADR1 = radr1;
    // 2084
    if ((type & SCE_GE_INTERNAL_REG_RADR2) != 0)
        HW_GE_RADR2 = radr2;

    // 2094
    HW_GE_INTERRUPT_TYPE4 = HW_GE_INTERRUPT_TYPE1 ^ oldIntrType;
    sceKernelCpuResumeIntr(oldIntr);
    return 0;
}

/*
 * The GE interrupt handler: interrupts are triggered on SIGNAL, FINISH and END, and errors
 */
int _sceGeInterrupt(int arg0 __attribute__ ((unused)), int arg1
                __attribute__ ((unused)), int arg2 __attribute__ ((unused)))
{
    int oldIntr = sceKernelCpuSuspendIntr();
    int attr = HW_GE_INTERRUPT_TYPE1;
    int unk1 = HW_GE_UNK004;
    // In case an error interrupt was caught, start _sceGeErrorInterrupt
    if ((attr & HW_GE_INTERR) != 0) {
        // 2228
        HW_GE_INTERRUPT_TYPE4 = HW_GE_INTERR;
        _sceGeErrorInterrupt(attr, unk1, arg2);
    }
    // 2118
    // There cannot be both SIGNAL and FINISH at the same time
    if ((attr & (HW_GE_INTSIG | HW_GE_INTFIN)) == (HW_GE_INTSIG | HW_GE_INTFIN)) {
        // 2218
        Kprintf("GE INTSIG/INTFIN at the same time\n"); // 0x6324
    }
    // 2128
    // No FINISH, it it must be a SIGNAL, with or without an END
    if ((attr & HW_GE_INTFIN) == 0) {
        // 21AC
        if ((attr & HW_GE_INTSIG) == 0 && (attr & HW_GE_INTEND) != 0) {   // 2208
            // 21FC dup
            HW_GE_INTERRUPT_TYPE3 = HW_GE_INTEND;
        } else if ((attr & HW_GE_INTSIG) != 0 && (attr & HW_GE_INTEND) == 0) {
            // 21FC dup
            HW_GE_INTERRUPT_TYPE3 = HW_GE_INTSIG;
        } else {
            // 21C0
            // If both SIGNAL and END were received, wait for the end of the execution and start _sceGeListInterrupt()
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
        // If both FINISH and END were received, wait for the end of the execution and start _sceGeFinishInterrupt()
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

/*
 * The GE system event handler, used when suspend and resume are triggered
 */
s32 _sceGeSysEventHandler(s32 ev_id, char *ev_name __attribute__((unused)), void *param, s32 *result __attribute__((unused)))
{
    switch (ev_id) {
    case SCE_SYSTEM_SUSPEND_EVENT_PHASE0_5:
        // 2420
        // Save the context and suspend the queue
        sceSysregAwRegABusClockEnable();
        _sceGeQueueSuspend();
        sceGeSaveContext((SceGeContext*)&_aw_ctx);
        _aw_ctx.edramTransDisable = HW_GE_EDRAM_ADDR_TRANS_DISABLE;
        _aw_ctx.edramTransVal     = HW_GE_EDRAM_ADDR_TRANS_VALUE;
        _aw_ctx.edramRefresh1     = HW_GE_EDRAM_REFRESH_UNK1;
        _aw_ctx.edramRefresh2     = HW_GE_EDRAM_REFRESH_UNK2;
        _aw_ctx.edramRefresh3     = HW_GE_EDRAM_REFRESH_UNK3;
        _aw_ctx.edramUnk40        = HW_GE_EDRAM_UNK40;
        _aw_ctx.geometryClock     = HW_GE_GEOMETRY_CLOCK;
        break;

    case SCE_SYSTEM_SUSPEND_EVENT_PHASE0_3:
        // 228C
        // Suspend the eDram and the GE clocks
        _sceGeEdramSuspend();
        if (((SceSysEventSuspendPayload*)param)->isStandbyOrRebootRequested != 2) { // TODO: find out when this can be 2
            sceSysregAwRegABusClockDisable();
            sceSysregAwRegBBusClockDisable();
            sceSysregAwEdramBusClockDisable();
        }
        break;

    case SCE_SYSTEM_RESUME_EVENT_PHASE0_5:
        // 22C4
        // Enable GE clocks
        sceSysregAwResetDisable();
        sceSysregAwRegABusClockEnable();
        sceSysregAwRegBBusClockEnable();
        sceSysregAwEdramBusClockEnable();
        // Init the eDram
        sceGeEdramInit();
        _sceGeEdramResume();
        // Restore the GE context
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
        HW_GE_EDRAM_ADDR_TRANS_DISABLE = _aw_ctx.edramTransDisable;
        HW_GE_EDRAM_ADDR_TRANS_VALUE   = _aw_ctx.edramTransVal;
        HW_GE_EDRAM_REFRESH_UNK1       = _aw_ctx.edramRefresh1;
        HW_GE_EDRAM_REFRESH_UNK2       = _aw_ctx.edramRefresh2;
        HW_GE_EDRAM_REFRESH_UNK3       = _aw_ctx.edramRefresh3;
        HW_GE_EDRAM_UNK40              = _aw_ctx.edramUnk40;
        HW_GE_GEOMETRY_CLOCK           = _aw_ctx.geometryClock;
        sceSysregSetMasterPriv(64, 1);
        sceGeRestoreContext((SceGeContext*)&_aw_ctx);
        sceSysregSetMasterPriv(64, 0);
        _sceGeQueueResume();
        if (_sceGeQueueStatus() == 0)
            sceSysregAwRegABusClockDisable();
        break;
    }
    return 0;
}

/*
 * The GE module entry point
 */
int _sceGeModuleStart()
{
    sceGeInit();
    return 0;
}

/*
 * The GE module reboot phase: interrupt drawing.
 */
int _sceGeModuleRebootPhase(s32 arg0 __attribute__((unused)), void *arg1 __attribute__((unused)), s32 arg2 __attribute__((unused)), s32 arg3 __attribute__((unused)))
{
    if (arg0 == 1)
        sceGeBreak(0, NULL);    // 24E4
    return 0;
}

/*
 * The GE module just-before reboot function: end GE functionality
 */
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

/*
 * Set three internal registers at the same time: BASE, RADR1 & RADR2 (see _sceGeSetInternalReg() for details)
 */
int _sceGeSetBaseRadr(int base, int radr1, int radr2)
{
    return _sceGeSetInternalReg(SCE_GE_INTERNAL_REG_BASE_ADDR | SCE_GE_INTERNAL_REG_RADR1 | SCE_GE_INTERNAL_REG_RADR2, base, radr1, radr2);
}

/*
 * Resume eDram functionality
 */
int _sceGeEdramResume()
{
    // Reset the addr translation value
    sceGeEdramSetAddrTranslation(g_edramAddrTrans);
    // Reenable additional eDram if it exists, in all cases (probably so the memcpy below goes well)
    if (g_uiEdramHwSize == 0x00400000) {
        // 261C
        HW_GE_EDRAM_ENABLED_SIZE = 2;
        sceSysregSetAwEdramSize(1);
    }
    // 25A0
    // Restore eDram contents
    memcpy(UCACHED(sceGeEdramGetAddr()),
           UCACHED(sceKernelGetAWeDramSaveAddr()), g_uiEdramHwSize);
    sceKernelDcacheWritebackInvalidateAll();
    // Set the enabled size to the requested size
    if (g_uiEdramHwSize == 0x00400000 && g_uiEdramSize == 0x00200000) { // 25F4
        HW_GE_EDRAM_ENABLED_SIZE = 4;
        sceSysregSetAwEdramSize(0);
    }
    return 0;
}

int sceGeEdramInit()
{
    // Sleep for a bit (?)
    int i = 83;
    // 264C
    while ((i--) != 0)
        ;
    // Enable eDram function?
    HW_GE_EDRAM_UNK10 = 1;
    // 2660
    while ((HW_GE_EDRAM_UNK10 & 1) != 0)
        ;
    // Set eDram default parameters
    HW_GE_EDRAM_REFRESH_UNK2 = 0x6C4;
    HW_GE_EDRAM_UNK40 = 1;
    HW_GE_EDRAM_UNK90 = 3;
    HW_GE_EDRAM_ENABLED_SIZE = 4;
    // If g_uiEdramHwSize is already set (eg after resume), stop here
    if (g_uiEdramHwSize != 0)
        return 0;

    // Set the eDram address translation
    if ((HW_GE_EDRAM_ADDR_TRANS_DISABLE & 1) == 0) {
        // 2758
        g_edramAddrTrans = HW_GE_EDRAM_ADDR_TRANS_VALUE << 1;
    } else
        g_edramAddrTrans = 0;

    // 26C8
    // Enable additional eDram if the tachyon version is recent enough
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
    // Otherwise, use the value returned by the hardware
    int size = (HW_GE_EDRAM_HW_SIZE & 0xFFFF) << 10;
    g_uiEdramSize = size;
    g_uiEdramHwSize = size;
    return 0;
}

int sceGeEdramSetRefreshParam(int mode, int arg1, int arg2, int arg3)
{
    int ret = 0;
    int oldIntr = sceKernelCpuSuspendIntr();
    // Disable eDram function while setting internal parameters?
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
        // If the size is set to 2MB, set it directly
        g_uiEdramSize = size;
        sceGeSetReg(SCE_GE_REG_EDRAM_ENABLED_SIZE, 4);
        // 2934 dup
        sceSysregSetAwEdramSize(0);
    } else if (size == 0x400000) {
        // 28FC
        // If the size is set to 4MB, it can only work for more recent PSP models
        if (sceSysregGetTachyonVersion() <= 0x4FFFFF)
            return SCE_ERROR_INVALID_SIZE;
        g_uiEdramSize = size;
        sceGeSetReg(SCE_GE_REG_EDRAM_ENABLED_SIZE, 2);
        // 2934 dup
        sceSysregSetAwEdramSize(1);
    } else {
        // Any value other than exactly 2MiB and 4MiB is rejected
        return SCE_ERROR_INVALID_SIZE;
    }

    return 0;
}

int sceGeEdramGetAddr()
{
    // Easy enough!
    return 0x04000000;
}

int sceGeEdramSetAddrTranslation(int width)
{
    int ret;
    if (width != 0 && width != 0x200 && width != 0x400
        && width != 0x800 && width != 0x1000)
        return SCE_ERROR_INVALID_VALUE;

    // 29AC
    int oldIntr = sceKernelCpuSuspendIntr();
    g_edramAddrTrans = width;
    if (width == 0) {
        // 2A28
        // Width = 0 means disable address translation
        if ((HW_GE_EDRAM_ADDR_TRANS_DISABLE & 1) == 0) {
            ret = HW_GE_EDRAM_ADDR_TRANS_VALUE << 1;
            HW_GE_EDRAM_ADDR_TRANS_DISABLE = 1;
        } else {
            // If it's already disabled, do nothing
            ret = 0;
        }
    } else if ((HW_GE_EDRAM_ADDR_TRANS_DISABLE & 1) == 0) {
        // 2A0C
        // If address translation was already enabled, just set its value
        ret = HW_GE_EDRAM_ADDR_TRANS_VALUE << 1;
        HW_GE_EDRAM_ADDR_TRANS_VALUE = width >> 1;
    } else {
        // If address translation was not enabled, enable it after setting the value
        HW_GE_EDRAM_ADDR_TRANS_VALUE = width >> 1;
        HW_GE_EDRAM_ADDR_TRANS_DISABLE = 0;
        ret = 0;
    }
    sceKernelCpuResumeIntrWithSync(oldIntr);
    return ret;
}

int _sceGeEdramSuspend()
{
    // Flush the dcache
    sceKernelDcacheWritebackInvalidateAll();
    // If the eDram is 4MB big, set the enabled size to 4MB for now to backup everything
    if (g_uiEdramHwSize == 0x00400000) {
        // 2ABC
        HW_GE_EDRAM_ENABLED_SIZE = 2;
        sceSysregSetAwEdramSize(1);
    }
    // 2A80
    // Backup the eDram to a saved memory space on suspend
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
    // Initialize all the (for now, free) display lists
    int i;
    for (i = 0; i < MAX_COUNT_DL; i++) {
        SceGeDisplayList *cur = &g_displayLists[i];
        // The list of free display lists is specified through a double-linked list
        cur->next = &g_displayLists[i + 1];
        cur->prev = &g_displayLists[i - 1];
        cur->signal = SCE_GE_DL_SIGNAL_NONE;
        cur->state = SCE_GE_DL_STATE_NONE;
        cur->isBusy = 0;
    }

    // Set the beginning and the end of the double-linked list
    g_displayLists[0].prev = NULL;
    g_displayLists[63].next = NULL;

    g_AwQueue.curRunning = NULL;

    // All the display lists are free, so set the first & last one accordingly
    g_AwQueue.free_last = &g_displayLists[63];
    g_AwQueue.free_first = g_displayLists;

    // No display list is active for now
    g_AwQueue.active_first = NULL;
    g_AwQueue.active_last = NULL;

    // Create event flags for completed drawing & completed list execution
    g_AwQueue.drawingEvFlagId = sceKernelCreateEventFlag("SceGeQueue", 0x201, 2, NULL);
    g_AwQueue.listEvFlagIds[0] = sceKernelCreateEventFlag("SceGeQueueId", 0x201, -1, NULL);
    g_AwQueue.listEvFlagIds[1] = sceKernelCreateEventFlag("SceGeQueueId", 0x201, -1, NULL);
    // Used for the patching stuff for Genso Suikoden I&II (lazy flush)
    g_AwQueue.syscallId = sceKernelQuerySystemCall((void*)sceGeListUpdateStallAddr);
    SceKernelGameInfo *info = sceKernelGetGameInfo();
    // Patch for Itadaki Street Portable (missing a cache flush function when enqueuing the first display list)
    g_AwQueue.cachePatch = 0;
    if (info != NULL && strcmp(info->gameId, "ULJM05127") == 0) {
        g_AwQueue.cachePatch = 1;
    }
    return 0;
}

/*
 * Suspend the current queue execution
 */
int _sceGeQueueSuspend()
{
    // No queue is active: nothing to do
    if (g_AwQueue.active_first == NULL)
        return 0;

    // 2C5C
    // While the GE is still in execution: see if we catch a stall
    while ((HW_GE_EXEC & 1) != 0) {
        if (HW_GE_LISTADDR == HW_GE_STALLADDR) {
            // A stall was caught
            int oldCmd1, oldCmd2;
            // 2DF8
            // Save the registers to the suspend status
            g_GeSuspend.status = HW_GE_EXEC | 0x1;
            int *stall = (int *)HW_GE_STALLADDR;
            g_GeSuspend.stallAddr = stall;
            g_GeSuspend.unk0 = HW_GE_UNK004;
            g_GeSuspend.listAddr = HW_GE_LISTADDR;
            g_GeSuspend.intrType = HW_GE_INTERRUPT_TYPE1;
            g_GeSuspend.sigCmd = HW_GE_CMD(SCE_GE_CMD_SIGNAL);
            g_GeSuspend.finCmd = HW_GE_CMD(SCE_GE_CMD_FINISH);
            g_GeSuspend.endCmd = HW_GE_CMD(SCE_GE_CMD_END);
            // Replace the commands after the stall to a FINISH/END sequence
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
            // Increase the stall address by 8 so the FINISH/END sequence we added is executed
            HW_GE_STALLADDR += 8;
            // 2EE0
            while ((HW_GE_EXEC & 1) != 0)
                ;
            // First case: list address is still the former stall address, so it didn't execute our code and stopped before
            if (HW_GE_LISTADDR == (int)g_GeSuspend.stallAddr) {
                // 2F38
                // In this case, reset the stall address and act as if it stopped normally (general case below)
                HW_GE_STALLADDR = (int)stall;
                stall[0] = oldCmd1;
                stall[1] = oldCmd2;
                pspCache(0x1A, &stall[0]);
                pspCache(0x1A, &stall[1]);
                break;
            } else {
                // Second case: list address was updated so our code must've been executed, we can leave now
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

    // GE execution ended by itself

    // 2C88
    // If execution didn't end with a SIGNAL, and the current active queue is not in an unhandled BREAK state
    if ((HW_GE_INTERRUPT_TYPE1 & HW_GE_INTSIG) == 0 && (g_AwQueue.active_first->signal != SCE_GE_DL_SIGNAL_BREAK || g_AwQueue.isBreak != 0)) // 2DE8
    {
        // 2CB0
        // Wait for a FINISH command
        while ((HW_GE_INTERRUPT_TYPE1 & HW_GE_INTFIN) == 0)
            ;
    }
    // 2CC4
    // Save all the necessary info for suspending
    g_GeSuspend.unk0 = HW_GE_UNK004;
    g_GeSuspend.status = HW_GE_EXEC;
    g_GeSuspend.listAddr = HW_GE_LISTADDR;
    g_GeSuspend.stallAddr = (int *)HW_GE_STALLADDR;
    g_GeSuspend.intrType = HW_GE_INTERRUPT_TYPE1;
    g_GeSuspend.sigCmd = HW_GE_CMD(SCE_GE_CMD_SIGNAL);
    g_GeSuspend.finCmd = HW_GE_CMD(SCE_GE_CMD_FINISH);
    g_GeSuspend.endCmd = HW_GE_CMD(SCE_GE_CMD_END);
    sceSysregSetMasterPriv(64, 1);
    int oldLadr = HW_GE_LISTADDR;
    int oldSadr = HW_GE_STALLADDR;
    int oldIntrType = HW_GE_INTERRUPT_TYPE1;
    // Execute stopCmd (just a FINISH/END sequence)
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
    HW_GE_LISTADDR = oldLadr;
    HW_GE_STALLADDR = oldSadr;
    HW_GE_INTERRUPT_TYPE4 = HW_GE_INTERRUPT_TYPE1 ^ oldIntrType;
    sceSysregSetMasterPriv(64, 0);
    return 0;
}

/*
 * Resume the current queue execution
 */
int _sceGeQueueResume()
{
    // No queue was running: nothing to do
    if (g_AwQueue.active_first == NULL)
        return 0;

    // 2F88
    sceSysregSetMasterPriv(64, 1);
    // First run the saved SIGNAL/FINISH/END command, to resume in the same signal context
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
    // Flush interrupts?
    int oldFlag = g_GeSuspend.intrType;
    int flag = 0;
    if ((oldFlag & HW_GE_INTSIG) == 0)
        flag |= HW_GE_INTSIG;
    if ((oldFlag & HW_GE_INTEND) == 0)
        flag |= HW_GE_INTEND;
    if ((oldFlag & HW_GE_INTFIN) == 0)
        flag |= HW_GE_INTFIN;
    HW_GE_INTERRUPT_TYPE4 = flag;
    // Restore variables related to the display list execution
    HW_GE_LISTADDR = g_GeSuspend.listAddr;
    HW_GE_STALLADDR = (int)g_GeSuspend.stallAddr;
    HW_GE_EXEC = g_GeSuspend.status;
    return 0;
}

/*
 * This function is run when a display list caught the FINISH/END sequence
 */
void
_sceGeFinishInterrupt(int arg0 __attribute__ ((unused)), int arg1
                      __attribute__ ((unused)), int arg2
                      __attribute__ ((unused)))
{
    SceGeDisplayList *curRunningDl = g_AwQueue.curRunning;
    // Disable break state
    g_AwQueue.isBreak = 0;
    // Unset the running display list
    g_AwQueue.curRunning = NULL;
    if (curRunningDl != NULL) {
        if (curRunningDl->signal == SCE_GE_DL_SIGNAL_SYNC) {
            // If the SYNC signal was set, run pspSync() and resume execution directly
            // 351C
            curRunningDl->signal = SCE_GE_DL_SIGNAL_NONE;
            g_AwQueue.curRunning = curRunningDl;
            HW_GE_EXEC |= 1;
            pspSync();
            return;
        } else if (curRunningDl->signal == SCE_GE_DL_SIGNAL_PAUSE) {
            // If the PAUSE signal was set, store the currectly running status
            // 3468
            int state = HW_GE_EXEC;
            curRunningDl->execState = state;
            curRunningDl->list = (int *)HW_GE_LISTADDR;
            curRunningDl->radr1 = HW_GE_RADR1;
            curRunningDl->radr2 = HW_GE_RADR2;
            curRunningDl->oadr = HW_GE_OADR;
            curRunningDl->oadr1 = HW_GE_OADR1;
            curRunningDl->oadr2 = HW_GE_OADR2;
            curRunningDl->base = HW_GE_CMD(SCE_GE_CMD_BASE);
            if (!(state & HW_GE_EXEC_DEPTH2)) {
                curRunningDl->radr2 = 0;
                curRunningDl->oadr2 = 0;
                if (!(state & HW_GE_EXEC_DEPTH1)) {
                    curRunningDl->radr1 = 0;
                    curRunningDl->oadr1 = 0;
                }
            }
            // 34E8
            // If the display list is the first active one, set the BREAK signal and run the signal callback
            // (otherwise, just execute the next one normally)
            if (g_AwQueue.active_first == curRunningDl) {
                // 3500
                curRunningDl->signal = SCE_GE_DL_SIGNAL_BREAK;
                if (curRunningDl->cbId >= 0) {
                    void *list = curRunningDl->list;
                    // 3288 dup
                    if (g_AwQueue.sdkVer <= 0x02000010)
                        list = 0;
                    sceKernelCallSubIntrHandler(SCE_GE_INT, curRunningDl->cbId * 2, curRunningDl->signalData, (int)list);    // call signal CB
                }
                // 32A4
                _sceGeClearBp();
                return;
            }
            curRunningDl = NULL;
        }
        // 309C
        // Interrupt the current running display list (if not in the PAUSE signal case)
        if (curRunningDl != NULL) {
            // If the display list is running, stop it (otherwise, do nothing)
            if (curRunningDl->state == SCE_GE_DL_STATE_RUNNING) {
                // 32DC
                // Expect a FINISH/END sequence
                int *cmdList = (int *)HW_GE_LISTADDR;
                u32 finishCmd = *(int *)UUNCACHED(cmdList - 2);
                u32 endCmd = *(int *)UUNCACHED(cmdList - 1);
                if ((finishCmd >> 24) != SCE_GE_CMD_FINISH || (endCmd >> 24) != SCE_GE_CMD_END) {
                    // 3318
                    Kprintf("_sceGeFinishInterrupt(): illegal finish sequence (MADR=0x%08X)\n", cmdList);   // 6398
                }
                // 3328
                // Set the DL state to 'completed'
                curRunningDl->state = SCE_GE_DL_STATE_COMPLETED;
                // Expect the CALL/RET nesting to be correct (ie that we returned from all CALLs)
                if (curRunningDl->stackOff != 0) {
                    // 343C
                    Kprintf("_sceGeFinishInterrupt(): CALL/RET nesting corrupted\n");   // 63D8
                }
                // 3338
                if (g_GeLogHandler != NULL) {
                    // 3418
                    g_GeLogHandler(SCE_GE_LOG_DL_END, (int)curRunningDl ^ g_dlMask,
                                 cmdList, finishCmd, endCmd);
                }
                // 3348
                // Call the finish CB
                if (curRunningDl->cbId >= 0) {
                    if (g_AwQueue.sdkVer <= 0x02000010)
                        cmdList = NULL;
                    sceKernelCallSubIntrHandler(SCE_GE_INT, curRunningDl->cbId * 2 + 1, finishCmd & 0xFFFF, (int)cmdList); // call finish CB
                }
                // 337C
                // Restore the last saved context
                if (curRunningDl->ctx != NULL) {
                    // 3408
                    sceGeRestoreContext(curRunningDl->ctx);
                }
                // Remove the current display list from the double-linked queue
                // 338C
                if (curRunningDl->prev != NULL)
                    curRunningDl->prev->next = curRunningDl->next;
                // 33A0
                if (curRunningDl->next != NULL)
                    curRunningDl->next->prev = curRunningDl->prev;

                // Update active_first, active_last, free_first and free_last
                // 33A8
                if (g_AwQueue.active_first == curRunningDl)
                    g_AwQueue.active_first = curRunningDl->next;

                // 33B8
                if (g_AwQueue.active_last == curRunningDl) {
                    // 3400
                    g_AwQueue.active_last = curRunningDl->prev;
                }
                // 33C4
                if (g_AwQueue.free_first == NULL) {
                    g_AwQueue.free_first = curRunningDl;
                    // 33F8
                    curRunningDl->prev = NULL;
                } else {
                    g_AwQueue.free_last->next = curRunningDl;
                    curRunningDl->prev = g_AwQueue.free_last;
                }

                // 33E0
                curRunningDl->state = SCE_GE_DL_STATE_COMPLETED;
                curRunningDl->next = NULL;
                g_AwQueue.free_last = curRunningDl;
            }
        }
    }

    // Start the active_first display list
    // 30B0
    SceGeDisplayList *nextDl = g_AwQueue.active_first;
    if (nextDl == NULL) {
        // There is no active list, stop execution, set the drawing event flag as finished
        // 32B4
        HW_GE_EXEC = 0;
        pspSync();
        sceSysregAwRegABusClockDisable();
        sceKernelSetEventFlag(g_AwQueue.drawingEvFlagId, 2);
        // 3254
        _sceGeClearBp();
    } else {
        // The first active display list received the PAUSE signal, set it in the paused state and set it in BREAK mode
        if (nextDl->signal == SCE_GE_DL_SIGNAL_PAUSE) {
            // 3264
            nextDl->state = SCE_GE_DL_STATE_PAUSED;
            nextDl->signal = SCE_GE_DL_SIGNAL_BREAK;
            // Run the signal CB
            if (nextDl->cbId >= 0) {
                void *list = nextDl->list;
                // 3288 dup
                if (g_AwQueue.sdkVer <= 0x02000010)
                    list = NULL;
                sceKernelCallSubIntrHandler(SCE_GE_INT, nextDl->cbId * 2,
                                            nextDl->signalData, (int)list);
            }
            // 32A4
            _sceGeClearBp();
            return;
        }
        // The display list is in the paused state, stop execution and don't do anything
        if (nextDl->state == SCE_GE_DL_STATE_PAUSED) {
            // 324C
            sceSysregAwRegABusClockDisable();
            // 3254
            _sceGeClearBp();
        } else {
            int *ctx2 = (int *)nextDl->ctx;
            nextDl->state = SCE_GE_DL_STATE_RUNNING;
            // Update the new active display list context to the stopped display list, or save it directly if it doesn't exist
            if (ctx2 != NULL && nextDl->isBusy == 0) {
                if (curRunningDl == NULL || curRunningDl->ctx == NULL) {
                    // 323C
                    sceGeSaveContext(nextDl->ctx);
                } else {
                    // 310C
                    __builtin_memcpy(__builtin_assume_aligned(ctx2, 4), __builtin_assume_aligned(curRunningDl->ctx, 4), sizeof(*curRunningDl->ctx));
                }
            }
            // (3138)
            // 313C
            // Resume execution with the new display list
            nextDl->isBusy = 1;
            HW_GE_EXEC = 0;
            HW_GE_STALLADDR = (int)UCACHED(nextDl->stall);
            HW_GE_LISTADDR = (int)UCACHED(nextDl->list);
            HW_GE_OADR = nextDl->oadr;
            HW_GE_OADR1 = nextDl->oadr1;
            HW_GE_OADR2 = nextDl->oadr2;
            _sceGeSetBaseRadr(nextDl->base, nextDl->radr1, nextDl->radr2);
            pspSync();
            g_AwQueue.curRunning = nextDl;
            HW_GE_EXEC = nextDl->execState | 1;
            pspSync();
            if (g_GeLogHandler != 0) {
                // 321C
                g_GeLogHandler(SCE_GE_LOG_DL_RUNNING, (int)nextDl ^ g_dlMask, nextDl->list, nextDl->stall);
            }
        }
    }

    // 31C8
    // Run the end event flag for the stopped DL
    if (curRunningDl != NULL) {
        u32 idx = (curRunningDl - g_displayLists) / sizeof(SceGeDisplayList);
        sceKernelSetEventFlag(g_AwQueue.listEvFlagIds[idx / 32], 1 << (idx % 32));
    }
}

/*
 * This function is run when the active display list received a SIGNAL/END sequence.
 */
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
    u32 *signalCmdPtr = cmdList - 2;
    u32 *endCmdPtr = cmdList - 1;
    u32 signalCmd = *(u32 *) UUNCACHED(signalCmdPtr);
    u32 endCmd = *(u32 *) UUNCACHED(endCmdPtr);
    // Check that the signal was indeed triggered by a SIGNAL/END sequence
    // 35F4
    if ((signalCmd >> 24) != SCE_GE_CMD_SIGNAL || (endCmd >> 24) != SCE_GE_CMD_END) {
        Kprintf("_sceGeListInterrupt(): bad signal sequence (MADR=0x%08X)\n", cmdList); // 0x643C
        return;
    }
    // Log the SIGNAL
    if (g_GeLogHandler != NULL) {
        // 3BE8
        g_GeLogHandler(SCE_GE_LOG_DL_SIGNAL, (int)dl ^ g_dlMask, cmdList, signalCmd, endCmd);
    }
    // 3618
    switch ((signalCmd >> 16) & 0xFF) {
    case SCE_GE_SIGNAL_HANDLER_SUSPEND:
        // For SDK versions <= 0x02000010, pause the display list
        // 3670
        if (g_AwQueue.sdkVer <= 0x02000010) {
            dl->state = SCE_GE_DL_STATE_PAUSED;
            cmdList = NULL;
        }
        // 3698
        // Run the signal CB
        if (dl->cbId >= 0) {
            sceKernelCallSubIntrHandler(SCE_GE_INT, dl->cbId * 2,
                                        signalCmd & 0xFFFF, (int)cmdList);
            pspSync();
        }
        // 36B4
        // For SDK versions <= 0x02000010, set the DL status to the one specified
        if (g_AwQueue.sdkVer <= 0x02000010)
            dl->state = endCmd & 0xFF;
        // Resume execution
        HW_GE_EXEC |= 1;
        break;

    case SCE_GE_SIGNAL_HANDLER_CONTINUE:
        // 3708
        // Resume execution
        HW_GE_EXEC |= 1;
        pspSync();
        // Call the signal CB
        if (dl->cbId >= 0) {
            if (g_AwQueue.sdkVer <= 0x02000010)
                cmdList = NULL;
            sceKernelCallSubIntrHandler(SCE_GE_INT, dl->cbId * 2,
                                        signalCmd & 0xFFFF, (int)cmdList);
        }
        break;

    case SCE_GE_SIGNAL_HANDLER_PAUSE:
        // Set the display list state to PAUSED and the signal as specified, and resume execution
        dl->state = SCE_GE_DL_STATE_PAUSED;
        dl->signal = endCmd & 0xFF;
        dl->signalData = signalCmd;
        HW_GE_EXEC |= 1;
        break;

    case SCE_GE_SIGNAL_SYNC:
        // 3994
        // Set the display list signal to SYNC and resume execution
        dl->signal = SCE_GE_DL_SIGNAL_SYNC;
        HW_GE_EXEC |= 1;
        break;

    case SCE_GE_SIGNAL_CALL:
    case SCE_GE_SIGNAL_RCALL:
    case SCE_GE_SIGNAL_OCALL:
        {
            // Do a software-side CALL
            // 3870
            if (dl->stackOff >= dl->numStacks) {
                // 398C
                _sceGeListError(signalCmd, SCE_GE_SIGNAL_ERROR_STACK_OVERFLOW);
                break;
            }
            // Save caller status
            SceGeStack *curStack = &dl->stack[dl->stackOff++];
            curStack->stack[0] = HW_GE_EXEC;
            curStack->stack[1] = HW_GE_LISTADDR;
            curStack->stack[2] = HW_GE_OADR;
            curStack->stack[3] = HW_GE_OADR1;
            curStack->stack[4] = HW_GE_OADR2;
            curStack->stack[5] = HW_GE_RADR1;
            curStack->stack[6] = HW_GE_RADR2;
            curStack->stack[7] = HW_GE_CMD(SCE_GE_CMD_BASE);
            if (!(dl->execState & HW_GE_EXEC_DEPTH2)) {
                dl->radr2 = 0;
                dl->oadr2 = 0;
                if (!(dl->execState & HW_GE_EXEC_DEPTH1)) {
                    dl->oadr1 = 0;
                    dl->radr1 = 0;
                }
            }
            // (3924)
            // 3928
            // Compute the call address
            int cmdOff = (signalCmd << 16) | (endCmd & 0xFFFF);
            u32 *newCmdList;
            if (((signalCmd >> 16) & 0xFF) == SCE_GE_SIGNAL_CALL)
                newCmdList = (u32 *) cmdOff;
            else if (((signalCmd >> 16) & 0xFF) == SCE_GE_SIGNAL_RCALL)
                newCmdList = &cmdList[cmdOff / 4 - 2];
            else
                newCmdList = HW_GE_OADR + (u32 *) cmdOff;

            // 395C
            // Check call address alignment
            if (((int)newCmdList & 3) != 0) {
                // 397C
                _sceGeListError(signalCmd, SCE_GE_SIGNAL_ERROR_INVALID_ADDRESS);
            }
            // 396C
            // Jump to call address and resume execution
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
            // Compute the relative jump address from the LSB of the SIGNAL and END commands
            int cmdOff = (signalCmd << 16) | (endCmd & 0xFFFF);
            u32 *newCmdList;
            // Compute address depending on the instruction
            if (((signalCmd >> 16) & 0xFF) == SCE_GE_SIGNAL_JUMP)
                newCmdList = (u32 *) cmdOff;
            else if (((signalCmd >> 16) & 0xFF) == SCE_GE_SIGNAL_RJUMP)
                newCmdList = &cmdList[cmdOff / 4 - 2];
            else
                newCmdList = HW_GE_OADR + (u32 *) cmdOff;

            // 37B4
            // Verify jump address alignment
            if (((int)newCmdList & 3) != 0) {
                // 37D0
                _sceGeListError(signalCmd, SCE_GE_SIGNAL_ERROR_INVALID_ADDRESS);
            }
            // 37C4
            // Jump to the address and resume execution
            HW_GE_LISTADDR = (int)UCACHED(newCmdList);
            HW_GE_EXEC |= 1;
            break;
        }

    case SCE_GE_SIGNAL_RET:
        // Return from a software CALL
        // 37E0
        // Check if there is no underflow (ie more RET than CALLs)
        if (dl->stackOff == 0) {
            _sceGeListError(signalCmd, SCE_GE_SIGNAL_ERROR_STACK_UNDERFLOW);
            return;
        }
        // 3804
        // Reload caller status
        SceGeStack *curStack = &dl->stack[dl->stackOff--];
        HW_GE_LISTADDR = (int)UCACHED(curStack->stack[1]);
        HW_GE_OADR = curStack->stack[2];
        HW_GE_OADR1 = curStack->stack[3];
        HW_GE_OADR2 = curStack->stack[4];
        _sceGeSetBaseRadr(curStack->stack[7], curStack->stack[5],
                          curStack->stack[6]);
        // Resume execution
        HW_GE_EXEC = curStack->stack[0] | 1;
        pspSync();
        break;

    case SCE_GE_SIGNAL_RTBP0: case SCE_GE_SIGNAL_RTBP1: case SCE_GE_SIGNAL_RTBP2: case SCE_GE_SIGNAL_RTBP3:
    case SCE_GE_SIGNAL_RTBP4: case SCE_GE_SIGNAL_RTBP5: case SCE_GE_SIGNAL_RTBP6: case SCE_GE_SIGNAL_RTBP7:
    case SCE_GE_SIGNAL_OTBP0: case SCE_GE_SIGNAL_OTBP1: case SCE_GE_SIGNAL_OTBP2: case SCE_GE_SIGNAL_OTBP3:
    case SCE_GE_SIGNAL_OTBP4: case SCE_GE_SIGNAL_OTBP5: case SCE_GE_SIGNAL_OTBP6: case SCE_GE_SIGNAL_OTBP7:
        {
            // Run TBPn/TBWn using relative addresses with the same address computation as JUMP/CALL
            // 39D0
            int off = (signalCmd << 16) | (endCmd & 0xFFFF);
            u32 *newLoc;
            if (((signalCmd >> 19) & 1) == 0) // RTBP
                newLoc = signalCmdPtr + off;
            else // OTBP
                newLoc = HW_GE_OADR + (u32 *) off;

            // 39F0
            if (((int)newLoc & 0xF) != 0) {
                // 3A50
                _sceGeListError(signalCmd, SCE_GE_SIGNAL_ERROR_INVALID_ADDRESS);
            }
            // 3A00
            int id = (signalCmd >> 16) & 0x7;
            int *uncachedCmdPtr = UUNCACHED(signalCmdPtr);
            uncachedCmdPtr[0] = GE_MAKE_OP(SCE_GE_CMD_TBP0 + id, (int)newLoc);
            uncachedCmdPtr[1] = GE_MAKE_OP(SCE_GE_CMD_TBW0 + id, ((((int)newLoc >> 24) & 0xF) << 16) | ((endCmd >> 16) & 0xFF));
            // 3A40
            pspSync();
            HW_GE_LISTADDR = (int)signalCmdPtr;
            HW_GE_EXEC |= 1;
            break;
        }

    case SCE_GE_SIGNAL_RCBP:
    case SCE_GE_SIGNAL_OCBP:
        {
            // Run CBP/CBW using relative addresses with the same address computation as JUMP/CALL
            // 3A80
            int off = (signalCmd << 16) | (endCmd & 0xFFFF);
            u32 *newLoc;
            if (((signalCmd >> 19) & 1) == 0) // RCBP
                newLoc = signalCmdPtr + off;
            else // OCBP
                newLoc = HW_GE_OADR + (u32 *) off;

            // 3AA4
            if (((int)newLoc & 0xF) != 0) {
                // 3AE8
                _sceGeListError(signalCmd, SCE_GE_SIGNAL_ERROR_INVALID_ADDRESS);
            }
            // 3AB4
            int *uncachedCmdPtr = UUNCACHED(signalCmdPtr);
            uncachedCmdPtr[0] = GE_MAKE_OP(SCE_GE_CMD_CBP, (int)newLoc);
            uncachedCmdPtr[1] = GE_MAKE_OP(SCE_GE_CMD_CBW, (((int)newLoc >> 24) & 0xF) << 16);
            // 3A40
            pspSync();
            HW_GE_LISTADDR = (int)signalCmdPtr;
            HW_GE_EXEC |= 1;
            break;
        }

    case SCE_GE_SIGNAL_BREAK1:
    case SCE_GE_SIGNAL_BREAK2:
        {
            // 3B08
            // If deci2p is not set, resume
            if (g_deci2p == NULL) {
                HW_GE_EXEC |= 1;
                return;
            }
            int opt = 0;
            // Checked if it's a BREAK2 signal, ie break with count
            if (((signalCmd >> 16) & 0xFF) == SCE_GE_SIGNAL_BREAK2) {
                // 3B6C
                SceGeBpCmd *bpCmd = &g_GeDeciBreak.cmds[0];
                int i;
                opt = 1;
                for (i = 0; i < g_GeDeciBreak.size; i++) {
                    // 3B90
                    // Check the matching break instruction
                    if (UCACHED(bpCmd->addr) == UCACHED(signalCmdPtr)) {
                        // 3BC0
                        if (bpCmd->count == 0) {
                            // If the breakpoint count is already 0, don't break
                            opt = -1;
                        } else if (bpCmd->count != -1) {
                            // If the breakpoint count is -1, always break; otherwise, see if it reaches zero after being decreased
                            bpCmd->count--;
                            if (bpCmd->count != 0) {
                                opt = -1;
                            }
                        }
                        break;
                    }
                    bpCmd++;
                }
                // 3BB0
                // If the count is not valid, resume
                if (opt < 0) {
                    HW_GE_EXEC |= 1;
                    return;
                }
            }
            // 3B28
            // Reset the GE list address and clear breakpoints
            HW_GE_LISTADDR = (int)signalCmdPtr;
            _sceGeClearBp();
            g_GeDeciBreak.inBreakState = 1;
            g_GeDeciBreak.size2 = 0;

            // 3B48
            // Wait for the deci2p operation to resume the GE
            do {
                g_deci2p->ops[SCE_DECI2OP_GE_BREAK](opt);
                opt = 0;
            } while (g_GeDeciBreak.inBreakState);
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
    // Get the address of the display list
    SceGeDisplayList *dl = (SceGeDisplayList *) (dlId ^ g_dlMask);
    int oldK1 = pspShiftK1();
    // Check if the pointer is in the correct range
    if (dl < g_displayLists || dl >= &g_displayLists[MAX_COUNT_DL]) {
        // 3D90
        pspSetK1(oldK1);
        return SCE_ERROR_INVALID_ID;
    }
    int oldIntr = sceKernelCpuSuspendIntr();
    // If the display list is in the NONE status, then it's not reserved and is invalid
    if (dl->state == SCE_GE_DL_STATE_NONE) {
        ret = SCE_ERROR_INVALID_ID;
    } else if (dl->isBusy) {
        ret = SCE_ERROR_BUSY;
    } else {
        // Remove the display list from the double-linked queue
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
        // Reset the display list
        dl->state = SCE_GE_DL_STATE_NONE;
        dl->next = NULL;
        g_AwQueue.free_last = dl;

        // Set the event flag corresponding to the display list
        u32 idx = (dl - g_displayLists) / sizeof(SceGeDisplayList);
        sceKernelSetEventFlag(g_AwQueue.listEvFlagIds[idx / 32], 1 << (idx % 32));

        if (g_GeLogHandler != NULL) {
            // 3D70
            g_GeLogHandler(SCE_GE_LOG_DL_DEQUEUED, dlId);
        }
        ret = 0;
    }

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
    // Check if the display list ID is valid
    if (idx >= MAX_COUNT_DL) {
        // 3EE0
        pspSetK1(oldK1);
        return SCE_ERROR_INVALID_ID;
    }

    // Mode = 0: wait until the FINISH/END is received
    if (mode == 0) {
        // 3E84
        int oldIntr = sceKernelCpuSuspendIntr();
        // Run the lazy flush patch
        _sceGeListLazyFlush();
        sceKernelCpuResumeIntr(oldIntr);
        // Wait for the event flag corresponding to the display list
        ret = sceKernelWaitEventFlag(g_AwQueue.listEvFlagIds[idx / 32], 1 << (idx % 32), 0, 0, 0);
        if (ret >= 0) {
            ret = SCE_GE_LIST_COMPLETED;
        }
    } else if (mode == 1) {
        // Mode = 1: poll the current display list status
        // 3E10
        // The state is basically given by the display list state
        switch (dl->state) {
        case SCE_GE_DL_STATE_QUEUED:
            // If the list has been already running and is queued again, return PAUSED anyway
            if (dl->isBusy)
                ret = SCE_GE_LIST_PAUSED;
            else
                ret = SCE_GE_LIST_QUEUED;
            break;

        case SCE_GE_DL_STATE_RUNNING:
            // 3E68
            // If the current address is the stall address, the list must be stalled
            if ((int)dl->stall != HW_GE_LISTADDR)
                ret = SCE_GE_LIST_DRAWING;
            else
                ret = SCE_GE_LIST_STALLING;
            break;

        // The last cases are straightforward
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
    // In this case, wait for the drawing to finish
    if (syncType == 0) {
        // 3FA4
        int oldIntr = sceKernelCpuSuspendIntr();
        _sceGeListLazyFlush();
        sceKernelCpuResumeIntr(oldIntr);
        // Wait for the drawing event flag to be triggered
        ret = sceKernelWaitEventFlag(g_AwQueue.drawingEvFlagId, 2, 0, NULL, NULL);
        if (ret >= 0) {
            // 3FF4
            int i;
            // Reset the state of all the completed display lists
            for (i = 0; i < MAX_COUNT_DL; i++) {
                SceGeDisplayList *curDl = &g_displayLists[i];
                if (curDl->state == SCE_GE_DL_STATE_COMPLETED) {
                    // 4010
                    curDl->state = SCE_GE_DL_STATE_NONE;
                    curDl->isBusy = 0;
                }
                // 4000
            }
            ret = SCE_GE_LIST_COMPLETED;
        }
    } else if (syncType == 1) {
        // Here, just poll the drawing status
        // 3F40
        int oldIntr = sceKernelCpuSuspendIntr();
        // If there is no active display list, drawing must be finished
        SceGeDisplayList *dl = g_AwQueue.active_first;
        if (dl == NULL) {
            // 3F9C
            ret = SCE_GE_LIST_COMPLETED;
        } else {
            // Otherwise, check the first or second display list (in case the first completed but is not removed from the queue yet)
            if (dl->state == SCE_GE_DL_STATE_COMPLETED)
                dl = dl->next;

            // 3F68
            // Return a value depending on the status of this display list
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
    SceGeDisplayList *activeDl = g_AwQueue.active_first;
    if (activeDl != NULL) {
        if (resetQueue) {
            // Reset the whole GE and queue
            // 42F0
            _sceGeReset();
            // 430C
            int i;
            for (i = 0; i < MAX_COUNT_DL; i++) {
                SceGeDisplayList *cur = &g_displayLists[i];
                cur->next = &g_displayLists[i + 1];
                cur->prev = &g_displayLists[i - 1];
                cur->signal = SCE_GE_DL_SIGNAL_NONE;
                cur->state = SCE_GE_DL_STATE_NONE;
                cur->isBusy = 0;
            }
            g_AwQueue.free_last = &g_displayLists[63];
            g_AwQueue.curRunning = NULL;
            g_AwQueue.active_last = NULL;
            g_displayLists[0].prev = NULL;
            g_displayLists[63].next = NULL;
            g_AwQueue.free_first = g_displayLists;
            g_AwQueue.active_first = NULL;
            ret = 0;
        } else if (activeDl->state == SCE_GE_DL_STATE_RUNNING) {
            // 4174
            // Stop GE execution
            HW_GE_EXEC = 0;
            pspSync();

            // 4180
            while ((HW_GE_EXEC & 1) != 0)
                ;
            if (g_AwQueue.curRunning != NULL) {
                int *cmdList = (int *)HW_GE_LISTADDR;
                int state = HW_GE_EXEC;
                activeDl->list = cmdList;
                activeDl->execState = state;
                activeDl->stall = (int *)HW_GE_STALLADDR;
                activeDl->radr1 = HW_GE_RADR1;
                activeDl->radr2 = HW_GE_RADR2;
                activeDl->oadr = HW_GE_OADR;
                activeDl->oadr1 = HW_GE_OADR1;
                activeDl->oadr2 = HW_GE_OADR2;
                activeDl->base = HW_GE_CMD(SCE_GE_CMD_BASE);
                if (!(state & HW_GE_EXEC_DEPTH2)) {
                    activeDl->radr2 = 0;
                    activeDl->oadr2 = 0;
                    if (!(state & HW_GE_EXEC_DEPTH1)) {
                        activeDl->radr1 = 0;
                        activeDl->oadr1 = 0;
                    }
                }
                // 4228
                int op = *((char *)UUNCACHED(cmdList - 1) + 3);
                if (op == SCE_GE_CMD_SIGNAL || op == SCE_GE_CMD_FINISH)
                {
                    // 42E8
                    activeDl->list = cmdList - 1;
                } else if (op == SCE_GE_CMD_END) {
                    // 42E0
                    activeDl->list = cmdList - 2;
                }
                // 4258
                if (activeDl->signal == SCE_GE_DL_SIGNAL_SYNC) {
                    // 42D4
                    activeDl->list += 2;
                }
                // 4268
            }
            // 426C
            activeDl->state = SCE_GE_DL_STATE_PAUSED;
            activeDl->signal = SCE_GE_DL_SIGNAL_BREAK;
            HW_GE_STALLADDR = 0;
            HW_GE_LISTADDR = (int)UUNCACHED(g_cmdList);
            HW_GE_INTERRUPT_TYPE4 = HW_GE_INTEND | HW_GE_INTFIN;
            HW_GE_EXEC = 1;
            pspSync();
            g_AwQueue.isBreak = 1;
            g_AwQueue.curRunning = NULL;
            ret = (int)activeDl ^ g_dlMask;
        } else if (activeDl->state == SCE_GE_DL_STATE_PAUSED) {
            // 4130
            ret = SCE_ERROR_BUSY;
            if (g_AwQueue.sdkVer > 0x02000010) {
                if (activeDl->signal == SCE_GE_DL_SIGNAL_PAUSE) {
                    // 4160
                    Kprintf("sceGeBreak(): can't break signal-pausing list\n"); // 64B4
                } else
                    ret = SCE_ERROR_ALREADY;
            }
        } else if (activeDl->state == SCE_GE_DL_STATE_QUEUED) {
            // 40FC
            activeDl->state = SCE_GE_DL_STATE_PAUSED;
            ret = (int)activeDl ^ g_dlMask;
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
                    if (dl->ctx != NULL && dl->isBusy == 0) {
                        // 4598
                        sceGeSaveContext(dl->ctx);
                    }
                    // 44BC
                    _sceGeWriteBp(dl->list);
                    dl->isBusy = 1;
                    HW_GE_LISTADDR = (int)UCACHED(dl->list);
                    HW_GE_STALLADDR = (int)UCACHED(dl->stall);
                    HW_GE_OADR = dl->oadr;
                    HW_GE_OADR1 = dl->oadr1;
                    HW_GE_OADR2 = dl->oadr2;
                    _sceGeSetBaseRadr(dl->base, dl->radr1, dl->radr2);
                    HW_GE_INTERRUPT_TYPE4 = HW_GE_INTEND | HW_GE_INTFIN;
                    HW_GE_EXEC = dl->execState | HW_GE_EXEC_RUNNING;
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
        if (g_deci2p->ops[SCE_DECI2OP_GE_PUT_BP](bp[i].bpAddr, 4, 3) < 0) {
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
    if (!g_GeDeciBreak.inBreakState) {
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
    if (dl < g_displayLists || dl >= &g_displayLists[MAX_COUNT_DL]) {
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
    if (g_GeDeciBreak.inBreakState) {
        // 4CF4
        sceKernelCpuResumeIntr(oldIntr);
        return SCE_ERROR_ALREADY;
    }
    g_GeDeciBreak.size2 = 0;
    g_GeDeciBreak.inBreakState = 1;
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

// byStep = 0: resume normal operation
// byStep = 1: step-by-step including in the callee
// byStep != {0,1}: step-by-step but stay in the caller if next instruction is a call
int sceGeDebugContinue(int byStep)
{
    SceGeDisplayList *dl = g_AwQueue.active_first;
    if (dl == NULL)
        return 0;
    int oldIntr = sceKernelCpuSuspendIntr();
    if (!g_GeDeciBreak.inBreakState) {
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
        if (byStep == 1) {
            if (!wasEnabled)
                sceSysregAwRegABusClockDisable();
            // 4EE8 dup
            sceKernelCpuResumeIntr(oldIntr);
            return 0;
        }
    }
    // (4D80)
    g_GeDeciBreak.inBreakState = 0;
    // 4D84
    if (!byStep) {
        // 4F44 dup
        g_GeDeciBreak.size2 = 0;
    } else {
        u32 curCmd = *cmdPtr;
        int *nextCmdPtr1 = cmdPtr + 1;
        int flag = HW_GE_EXEC;
        int op = curCmd >> 24;
        // We want to break on the next instruction, so we need to find where it is
        if ((op == SCE_GE_CMD_JUMP || op == SCE_GE_CMD_BJUMP) || (op == SCE_GE_CMD_CALL && byStep == 1))
        {
            // 4DCC
            if (op != SCE_GE_CMD_BJUMP || (flag & 2) == 0) // 5038
                nextCmdPtr1 = (int *)((((HW_GE_CMD(SCE_GE_CMD_BASE) << 8) & 0xFF000000)
                                      | (curCmd & 0x00FFFFFF)) + HW_GE_OADR);
            // 4DFC
        }
        // 4E00
        int *nextCmdPtr2 = cmdPtr + 2;
        if (op == SCE_GE_CMD_RET) {
            // 5004
            if ((flag & 0x200) == 0) {
                // 5020
                if ((flag & 0x100) != 0)
                    nextCmdPtr1 = (int *)HW_GE_RADR1;
            } else
                nextCmdPtr1 = (int *)HW_GE_RADR2;
        } else if (op == SCE_GE_CMD_FINISH) {
            nextCmdPtr1 = nextCmdPtr2;
        } else if (op == SCE_GE_CMD_SIGNAL) {
            // 4F54
            int signalOp = (curCmd >> 16) & 0x000000FF;
            int off = (curCmd << 16) | (*(cmdPtr + 1) & 0xFFFF);
            nextCmdPtr1 = nextCmdPtr2;
            if (signalOp == SCE_GE_SIGNAL_JUMP || (signalOp == SCE_GE_SIGNAL_CALL && byStep == 1)) {
                // 4F8C
                nextCmdPtr1 = (int *)off;
            } else if (signalOp == SCE_GE_SIGNAL_RJUMP || (signalOp == SCE_GE_SIGNAL_RCALL && byStep == 1)) { // 4F94
                // 4FAC
                nextCmdPtr1 = cmdPtr + off;
            } else if (signalOp == SCE_GE_SIGNAL_OJUMP || (signalOp == SCE_GE_SIGNAL_OCALL && byStep == 1)) { // 4FB4
                // 4FCC
                nextCmdPtr1 = (int *)HW_GE_OADR + off;
            } else if (signalOp == SCE_GE_SIGNAL_RET && dl->stackOff != 0) { // 4FDC
                nextCmdPtr1 = (int *)
                    dl->stack[dl->stackOff - 1].stack[1];
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

// Part of the patching stuff for Genso Suikoden I&II : set the callback in usersystemlib to call sceGeListUpdateStallAddr()
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

// Part of the patching stuff for Genso Suikoden I&II : call sceGeListUpdateStallAddr() regularly on some events on the last display list it wanted to update the stall address of
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
    if (g_AwQueue.cachePatch != 0 && pspK1IsUserMode()) {    // 5C94
        g_AwQueue.cachePatch--;
        sceKernelDcacheWritebackAll();
    }

    // 58FC
    int oldIntr = sceKernelCpuSuspendIntr();
    _sceGeListLazyFlush();
    SceGeDisplayList *curDl = g_AwQueue.active_first;
    // 5920
    while (curDl != NULL) {
        if (UCACHED((int)curDl->list) == UCACHED((int)list)) {
            // 5C74
            Kprintf("_sceGeListEnQueue(): can't enqueue duplicated addr(MADR=0x%08X)\n", list); // 0x65B8
            if (oldVer)
                break;
            sceKernelCpuResumeIntr(oldIntr);
            pspSetK1(oldK1);
            return SCE_ERROR_BUSY;
        }
        if (curDl->isBusy && stack != NULL && curDl->stack == stack && !oldVer) { // 5C44
            Kprintf("_sceGeListEnQueue(): can't enqueue duplicated stack(STACK=0x%08X)\n", stack);  // 0x65FC
            // 5C60
            sceKernelCpuResumeIntr(oldIntr);
            pspSetK1(oldK1);
            return SCE_ERROR_BUSY;
        }
        curDl = curDl->next;
        // 5954
    }

    // 5960
    SceGeDisplayList *newDl = g_AwQueue.free_first;
    if (newDl == NULL) {
        // 5C24
        sceKernelCpuResumeIntr(oldIntr);
        pspSetK1(oldK1);
        return SCE_ERROR_OUT_OF_MEMORY;
    }
    if (newDl->next == NULL) {
        g_AwQueue.free_last = NULL;
        // 5C3C
        g_AwQueue.free_first = NULL;
    } else {
        g_AwQueue.free_first = newDl->next;
        newDl->next->prev = NULL;
    }

    // 5984
    newDl->prev = NULL;
    newDl->next = NULL;
    if (newDl == NULL) {
        // 5C24
        sceKernelCpuResumeIntr(oldIntr);
        pspSetK1(oldK1);
        return SCE_ERROR_OUT_OF_MEMORY;
    }
    newDl->isBusy = 0;
    newDl->signal = SCE_GE_DL_SIGNAL_NONE;
    newDl->cbId = pspMax(cbid, -1);
    int newDlId = (int)newDl ^ g_dlMask;
    newDl->numStacks = numStacks;
    newDl->stack = stack;
    newDl->next = NULL;
    newDl->ctx = ctx;
    newDl->execState = 0;
    newDl->list = list;
    newDl->stall = stall;
    newDl->radr1 = 0;
    newDl->radr2 = 0;
    newDl->oadr = 0;
    newDl->oadr1 = 0;
    newDl->oadr2 = 0;
    newDl->base = 0;
    newDl->stackOff = 0;
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
            newDl->state = SCE_GE_DL_STATE_PAUSED;
            g_AwQueue.active_first->state = SCE_GE_DL_STATE_QUEUED;
            newDl->prev = NULL;
            g_AwQueue.active_first->prev = newDl;
            newDl->next = g_AwQueue.active_first;
        } else {
            newDl->state = SCE_GE_DL_STATE_PAUSED;
            newDl->prev = NULL;
            newDl->next = NULL;
            g_AwQueue.active_last = newDl;
        }

        // 5BB8
        g_AwQueue.active_first = newDl;
        if (g_GeLogHandler != NULL)
            g_GeLogHandler(SCE_GE_LOG_DL_ENQUEUED, newDlId, 1, list, stall);
    } else if (g_AwQueue.active_first == NULL) {
        // 5A8C
        newDl->state = SCE_GE_DL_STATE_RUNNING;
        newDl->isBusy = 1;
        newDl->prev = NULL;
        g_AwQueue.active_first = newDl;
        g_AwQueue.active_last = newDl;
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
        g_AwQueue.curRunning = newDl;
        sceKernelClearEventFlag(g_AwQueue.drawingEvFlagId, 0xFFFFFFFD);
        if (g_GeLogHandler != NULL) {
            g_GeLogHandler(SCE_GE_LOG_DL_ENQUEUED, newDlId, 0, list, stall);
            if (g_GeLogHandler != NULL)
                g_GeLogHandler(SCE_GE_LOG_DL_RUNNING, newDlId, list, stall);
        }
    } else {
        newDl->state = SCE_GE_DL_STATE_QUEUED;
        g_AwQueue.active_last->next = newDl;
        newDl->prev = g_AwQueue.active_last;
        g_AwQueue.active_last = newDl;
        if (g_GeLogHandler != NULL) {
            // 5A6C
            g_GeLogHandler(SCE_GE_LOG_DL_ENQUEUED, newDlId, 0, list, stall);
        }
    }

    // 5A28
    // clear event flag for this DL
    u32 idx = (newDl - g_displayLists) / sizeof(SceGeDisplayList);
    sceKernelClearEventFlag(g_AwQueue.listEvFlagIds[idx / 32], ~(1 << (idx % 32)));

    sceKernelCpuResumeIntr(oldIntr);
    pspSetK1(oldK1);
    return newDlId;
}

