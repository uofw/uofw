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

#include "ge.h"

SCE_MODULE_INFO("sceGE_Manager", SCE_MODULE_KERNEL | SCE_MODULE_ATTR_CANT_STOP | SCE_MODULE_ATTR_EXCLUSIVE_LOAD
                                 | SCE_MODULE_ATTR_EXCLUSIVE_START, 1, 11);
SCE_MODULE_BOOTSTART("_sceGeModuleStart");
SCE_MODULE_REBOOT_BEFORE("_sceGeModuleRebootBefore");
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wattribute-alias"
SCE_MODULE_REBOOT_PHASE("_sceGeModuleRebootPhase");
#pragma GCC diagnostic pop
SCE_SDK_VERSION(SDK_VERSION);

#define HW_GE_RESET     HW(0xBD400000)
#define HW_GE_EDRAMSIZE HW(0xBD400008)
#define HW_GE_DLIST     HW(0xBD400108)
#define HW_GE_STALLADDR HW(0xBD40010C)
#define HW_GE_CMD(i)    HW(0xBD400800 + i * 4)
#define HW_GE_BONES     ((vs32*)HWPTR(0xBD400C00))
#define HW_GE_BONE(i)   ((vs32*)HWPTR(0xBD400C00 + i * 48))
#define HW_GE_WORLDS    ((vs32*)HWPTR(0xBD400D80))
#define HW_GE_VIEWS     ((vs32*)HWPTR(0xBD400DB0))
#define HW_GE_PROJS     ((vs32*)HWPTR(0xBD400DE0))
#define HW_GE_TGENS     ((vs32*)HWPTR(0xBD400E20))

#define GE_SIGNAL_HANDLER_SUSPEND  0x01
#define GE_SIGNAL_HANDLER_CONTINUE 0x02
#define GE_SIGNAL_HANDLER_PAUSE    0x03
#define GE_SIGNAL_SYNC             0x08
#define GE_SIGNAL_JUMP             0x10
#define GE_SIGNAL_CALL             0x11
#define GE_SIGNAL_RET              0x12
#define GE_SIGNAL_RJUMP            0x13
#define GE_SIGNAL_RCALL            0x14
#define GE_SIGNAL_OJUMP            0x15
#define GE_SIGNAL_OCALL            0x16

#define GE_SIGNAL_RTBP0            0x20
#define GE_SIGNAL_RTBP1            0x21
#define GE_SIGNAL_RTBP2            0x22
#define GE_SIGNAL_RTBP3            0x23
#define GE_SIGNAL_RTBP4            0x24
#define GE_SIGNAL_RTBP5            0x25
#define GE_SIGNAL_RTBP6            0x26
#define GE_SIGNAL_RTBP7            0x27
#define GE_SIGNAL_OTBP0            0x28
#define GE_SIGNAL_OTBP1            0x29
#define GE_SIGNAL_OTBP2            0x2A
#define GE_SIGNAL_OTBP3            0x2B
#define GE_SIGNAL_OTBP4            0x2C
#define GE_SIGNAL_OTBP5            0x2D
#define GE_SIGNAL_OTBP6            0x2E
#define GE_SIGNAL_OTBP7            0x2F
#define GE_SIGNAL_RCBP             0x30
#define GE_SIGNAL_OCBP             0x38
#define GE_SIGNAL_BREAK1           0xF0
#define GE_SIGNAL_BREAK2           0xFF

#define GE_MAKE_OP(cmd, arg) (((cmd) << 24) | ((arg) & 0x00FFFFFF))
#define GE_VALID_ADDR(addr) ((int)(addr) >= 0 && \
         (ADDR_IS_SCRATCH(addr) || ADDR_IS_VRAM(addr) || ADDR_IS_RAM(addr)))

#define MAKE_SYSCALL(n)            (0x03FFFFFF & (((u32)(n) << 6) | 0x0000000C))
#define MAKE_JUMP(f)               (0x08000000 | ((u32)(f)  & 0x0FFFFFFC)) 
#define JR_RA                      (0x03E00008)
#define NOP                        (0)

/******************************/

int _sceGeReset();
int _sceGeInitCallback3(void *arg0, s32 arg1, void *arg2);
int _sceGeInitCallback4();
int _sceGeSetRegRadr1(int arg0);
int _sceGeSetRegRadr2(int arg0);
int _sceGeSetInternalReg(int type, int arg1, int arg2, int arg3);
int _sceGeInterrupt(int arg0, int arg1, int arg2);
s32 _sceGeSysEventHandler(s32 ev_id, char *ev_name, void *param, s32 *result);
int _sceGeModuleStart();
int _sceGeModuleRebootPhase(s32 arg0 __attribute__((unused)), void *arg1 __attribute__((unused)), s32 arg2 __attribute__((unused)), s32 arg3 __attribute__((unused)));
int _sceGeModuleRebootBefore(void *arg0 __attribute__((unused)), s32 arg1 __attribute__((unused)), s32 arg2 __attribute__((unused)), s32 arg3 __attribute__((unused)));
int _sceGeSetBaseRadr(int arg0, int arg1, int arg2);
int _sceGeEdramResume();
int _sceGeEdramSuspend();
int _sceGeQueueInit();
int _sceGeQueueSuspend();
int _sceGeQueueResume();
void _sceGeFinishInterrupt(int arg0, int arg1, int arg2);
void _sceGeListInterrupt(int arg0, int arg1, int arg2);
int sceGeDebugBreak();
int sceGeDebugContinue(int arg0);
int _sceGeQueueInitCallback();
int _sceGeQueueEnd();
int _sceGeQueueStatus(void);
void _sceGeErrorInterrupt(int arg0, int arg1, int arg2);
void _sceGeListError(u32 cmd, int err);
void _sceGeWriteBp(int *list);
void _sceGeClearBp();
void _sceGeListLazyFlush();
int _sceGeListEnQueue(void *list, void *stall, int cbid, SceGeListArgs * arg,
                      int head);

/****** Structures *********/

typedef struct {
    SceGeDisplayList *curRunning;
    int isBreak;
    SceGeDisplayList *active_first;  //  8
    SceGeDisplayList *active_last;   // 12
    SceGeDisplayList *free_first;    // 16
    SceGeDisplayList *free_last;     // 20
    SceUID drawingEvFlagId;     // 24
    SceUID listEvFlagIds[2];    // 28, 32
    SceGeStack stack[32];       // 36
    int sdkVer;                 // 1060
    int patched;                // 1064
    int syscallId;
    SceGeLazy *lazySyncData;
} SceGeQueue;

typedef struct {
    int unk0, unk4, unk8;
    int *unk12;
    int unk16, unk20, unk24, unk28;
} SceGeQueueSuspendInfo;

typedef struct {
    int unk0, unk4, unk8, unk12;
} SceGeBpCmd;

typedef struct {
    int busy;
    int clear;
    int size;
    int size2;
    SceGeBpCmd cmds[10];
} SceGeBpCtrl;

typedef struct {
    char *name;
    u32 *ptr;
} SadrUpdate;

typedef struct {
    char reg1;
    char cmd;
    char reg2;
    char size;
} SceGeMatrix;

/********* Global values *********/

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

// 66B4
char save_regs[] = {
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
int g_pAwRegAdr[] = {
    0xBD400000, 0xBD400004, 0xBD400008, 0xBD400100,
    0xBD400104, 0xBD400108, 0xBD40010C, 0xBD400110,
    0xBD400114, 0xBD400118, 0xBD40011C, 0xBD400120,
    0xBD400124, 0xBD400128, 0xBD400300, 0xBD400304,
    0xBD400308, 0xBD40030C, 0xBD400310, 0xBD400400,
    0xBD400200, 0xBD500000, 0xBD500010, 0xBD500020,
    0xBD500030, 0xBD500040, 0xBD500050, 0xBD500060,
    0xBD500070, 0xBD500080, 0xBD500090, 0xBD5000A0
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
void (*g_GeLogHandler) ();

// 68C0
SceGeContext _aw_ctx;

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
int g_cbhook;

// 7600
int *g_cmdList;

// 7604
int g_dlMask;

// 7608
void *g_deci2p;

// 7640
SceGeDisplayList g_displayLists[64];

/******************************/

int _sceGeReset()
{
    int oldIntr = sceKernelCpuSuspendIntr();
    sceSysregAwRegABusClockEnable();
    pspSync();
    HW(0xBD500010) = 2;
    HW_GE_RESET = 1;
    // 0144
    while ((HW_GE_RESET & 1) != 0)
        ;
    sceGeSaveContext(&_aw_ctx);
    _aw_ctx.ctx[16] = HW(0xBD400200);
    sceSysregAwResetEnable();
    sceSysregAwResetDisable();
    HW(0xBD500010) = 0;
    HW(0xBD400100) = 0;
    HW_GE_DLIST = 0;
    HW_GE_STALLADDR = 0;
    HW(0xBD400110) = 0;
    HW(0xBD400114) = 0;
    HW(0xBD400118) = 0;
    HW(0xBD40011C) = 0;
    HW(0xBD400120) = 0;
    HW(0xBD400124) = 0;
    HW(0xBD400128) = 0;
    HW(0xBD400310) = HW(0xBD400304);
    HW(0xBD40030C) = HW(0xBD400308);
    HW(0xBD400308) = 7;
    HW(0xBD400200) = _aw_ctx.ctx[16];
    sceSysregSetMasterPriv(64, 1);
    sceGeRestoreContext(&_aw_ctx);
    sceSysregSetMasterPriv(64, 0);
    sceSysregAwRegABusClockDisable();
    sceKernelCpuResumeIntr(oldIntr);
    return 0;
}

int sceGeInit()
{
    sceSysregAwResetDisable();
    sceSysregAwRegABusClockEnable();
    sceSysregAwRegBBusClockEnable();
    sceSysregAwEdramBusClockEnable();
    g_dlMask = (HW_GE_CMD(SCE_GE_CMD_VADR) ^ HW_GE_CMD(SCE_GE_CMD_PRIM)
              ^ HW_GE_CMD(SCE_GE_CMD_BEZIER) ^ HW_GE_CMD(SCE_GE_CMD_SPLINE)
              ^ HW_GE_CMD(SCE_GE_CMD_WORLDD)) | 0x80000000;
    sceGeEdramInit();
    HW(0xBD400100) = 0;
    u32 *dlist = &_aw_ctx.ctx[17];
    HW_GE_DLIST = 0;
    HW_GE_STALLADDR = 0;
    u32 *curDl = dlist;
    HW(0xBD400110) = 0;
    HW(0xBD400114) = 0;
    HW(0xBD400118) = 0;
    HW(0xBD40011C) = 0;
    HW(0xBD400120) = 0;
    HW(0xBD400124) = 0;
    HW(0xBD400128) = 0;
    HW(0xBD400310) = HW(0xBD400304);
    HW(0xBD40030C) = HW(0xBD400308);
    HW(0xBD400308) = 7;
    int i;
    for (i = 0; i < 256; i++) {
        if (((save_regs[i / 8] >> (i & 7)) & 1) != 0)
            *(curDl++) = i << 24;
        // 03A0
    }

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
    sceKernelDcacheWritebackInvalidateRange(dlist, 1980);
    HW(0xBD40030C) = HW(0xBD400308);
    HW_GE_DLIST = (int)UCACHED(dlist);
    HW_GE_STALLADDR = 0;
    sceSysregSetMasterPriv(64, 1);
    HW(0xBD400100) = 1;

    // 04B8
    while ((HW(0xBD400100) & 1) != 0)
        ;
    sceSysregSetMasterPriv(64, 0);
    HW(0xBD400100) = 0;
    HW_GE_DLIST = 0;
    HW_GE_STALLADDR = 0;
    HW(0xBD400310) = HW(0xBD400304);
    HW(0xBD400308) = 7;
    sceKernelRegisterIntrHandler(25, 1, _sceGeInterrupt, 0, &g_GeIntrOpt);

    // 0534
    for (i = 0; i < 16; i++)
        sceKernelSetUserModeIntrHanlerAcceptable(25, i, 1);

    sceKernelEnableIntr(25);
    sceSysregAwRegABusClockDisable();
    sceKernelRegisterSysEventHandler(&g_GeSysEv);
    _sceGeQueueInit();
    SceKernelUsersystemLibWork *libWork = sceKernelGetUsersystemLibWork();
    if (libWork->cmdList == NULL) {
        // 05EC
        sceKernelSetInitCallback(_sceGeInitCallback3, 3, 0);
    } else
        _sceGeInitCallback3(0, 0, 0);

    // 059C
    sceKernelSetInitCallback(_sceGeInitCallback4, 4, 0);
    void *ret = (void *)sceKernelDeci2pReferOperations();
    g_deci2p = ret;
    if (ret != NULL) {
        // 05D4
        int (*func) (SceKernelDeci2Ops *) = (void *)*(int *)(ret + 28);
        func(&g_Deci2Ops);
    }
    return 0;
}

int sceGeEnd()
{
    _sceGeQueueEnd();
    sceKernelUnregisterSysEventHandler(&g_GeSysEv);
    sceKernelDisableIntr(25);
    sceKernelReleaseIntrHandler(25);
    sceSysregAwRegABusClockEnable();
    HW(0xBD400100) = 0;
    // 0640
    while ((HW(0xBD400100) & 1) != 0)
        ;

    if (g_cmdList != NULL) {
        int *cmdOut = &g_cmdList[16];
        cmdOut[0] = GE_MAKE_OP(SCE_GE_CMD_BASE, (((((int)&g_cmdList[20]) >> 24) & 0xF) << 16));
        cmdOut[1] = GE_MAKE_OP(SCE_GE_CMD_OFFSET, 0);
        cmdOut[2] = GE_MAKE_OP(SCE_GE_CMD_CALL, (int)&g_cmdList[20]);
        cmdOut[3] = GE_MAKE_OP(SCE_GE_CMD_END, 0);
        cmdOut[4] = GE_MAKE_OP(SCE_GE_CMD_CALL, (int)&g_cmdList[22]);
        cmdOut[5] = GE_MAKE_OP(SCE_GE_CMD_RET, 0);
        cmdOut[6] = GE_MAKE_OP(SCE_GE_CMD_RET, 0);
        HW(0xBD400120) = 0;
        HW_GE_DLIST = (int)UCACHED(cmdOut);
        HW_GE_STALLADDR = 0;
        pspSync();
        HW(0xBD400100) = 1;
        // 06E0
        while ((HW(0xBD400100) & 1) != 0)
            ;
    }
    sceSysregAwRegABusClockDisable();
    return 0;
}

// 070C
int _sceGeInitCallback3(void *arg0 __attribute__ ((unused)), s32 arg1 __attribute__ ((unused)), void *arg2 __attribute__ ((unused)))
{
    SceKernelUsersystemLibWork *libWork = sceKernelGetUsersystemLibWork();
    if (libWork->cmdList != NULL) {
        s32 *uncachedDlist = UUNCACHED(libWork->cmdList);
        uncachedDlist[0] = GE_MAKE_OP(SCE_GE_CMD_FINISH, 0);
        uncachedDlist[1] = GE_MAKE_OP(SCE_GE_CMD_END, 0);
        g_cmdList = uncachedDlist;
        _sceGeQueueInitCallback();
    }
    return 0;
}

int _sceGeInitCallback4()
{
    SceKernelGameInfo *info = sceKernelGetGameInfo();
    if (info != NULL) {
        u32 syscOp = MAKE_SYSCALL(sceKernelQuerySystemCall((void*)sceGeListUpdateStallAddr));
        int oldIntr = sceKernelCpuSuspendIntr();
        if (strcmp(info->gameId, sadrupdate_bypass.name) == 0) {
            u32 *ptr = sadrupdate_bypass.ptr;
            if (ptr[0] == JR_RA && ptr[1] == syscOp) {
                // 0804
                ptr[0] = MAKE_JUMP(sceKernelGetUsersystemLibWork()->sceGeListUpdateStallAddr_lazy);
                ptr[1] = NOP;
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

int sceGeGetReg(u32 regId)
{
    if (regId >= 32)
        return 0x80000102;
    int oldK1 = pspShiftK1();
    int oldIntr = sceKernelCpuSuspendIntr();
    int wasEnabled = sceSysregAwRegABusClockEnable();
    int val = *(int *)g_pAwRegAdr[regId];
    if (!wasEnabled) {
        // 08C8
        sceSysregAwRegABusClockDisable();
    }
    // 089C
    sceKernelCpuResumeIntr(oldIntr);
    pspSetK1(oldK1);
    return val;
}

int sceGeSetReg(u32 regId, u32 value)
{
    if (regId >= 32)
        return 0x80000102;
    if (regId >= 5 && regId < 14 && (value & 3) != 0)
        return 0x800001FE;

    // 092C
    int oldK1 = pspShiftK1();
    int oldIntr = sceKernelCpuSuspendIntr();
    int wasEnabled = sceSysregAwRegABusClockEnable();
    int ret;
    if ((HW(0xBD400100) & 1) == 0) {
        if (regId == 7) {
            // 09EC
            _sceGeSetRegRadr1(value);
        } else if (regId == 8) {
            // 09DC
            _sceGeSetRegRadr2(value);
        }
        // 0974
        *(int *)g_pAwRegAdr[regId] = value;
        ret = 0;
    } else
        ret = 0x80000021;
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

int sceGeGetCmd(u32 cmdOff)
{
    if (cmdOff >= 0xFF)
        return 0x80000102;
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

int sceGeSetCmd(u32 cmdOff, u32 cmd)
{
    if (cmdOff >= 0xFF)
        return 0x80000102;
    int oldK1 = pspShiftK1();
    int oldIntr = sceKernelCpuSuspendIntr();
    int wasEnabled = sceSysregAwRegABusClockEnable();
    int ret = 0x80000021;
    int old100, old108;
    if ((HW(0xBD400100) & 1) == 0) {
        old100 = HW(0xBD400100);
        ret = 0;
        old108 = HW_GE_DLIST;
        if (cmdOff == SCE_GE_CMD_JUMP || cmdOff == SCE_GE_CMD_BJUMP || cmdOff == SCE_GE_CMD_CALL) {
            int addr = (((HW_GE_CMD(SCE_GE_CMD_BASE) << 8) & 0xFF000000) | (cmd & 0x00FFFFFF)) + HW(0xBD400120);
            if (!GE_VALID_ADDR(addr)) {
                // 0E68 dup
                ret = 0x80000103;
            } else {
                // 0B88
                if (cmdOff == 9) {
                    // 0E50
                    if ((old100 & 2) == 0)
                        old108 = addr;
                    else {
                        old108 += 4;
                        old100 &= 0xFFFFFFFD;
                    }
                } else if (cmdOff >= 10) {
                    // 0DE0
                    if (cmdOff == 10) {
                        if ((old100 & 0x200) != 0) {
                            // 0E48 dup
                            ret = 0x80000003;
                        } else if ((old100 & 0x100) == 0) {
                            // 0E24
                            _sceGeSetRegRadr1(old108 + 4);
                            old108 = addr;
                            old100 |= 0x100;
                            HW(0xBD400124) = HW(0xBD400120);
                        } else {
                            _sceGeSetRegRadr2(old108 + 4);
                            old100 = (old100 & 0xFFFFFEFF) | 0x200;
                            old108 = addr;
                            HW(0xBD400128) = HW(0xBD400120);
                        }
                    }
                } else if (cmdOff == 8)
                    old108 = addr;
                // 0BA4 dup
                cmd = HW_GE_CMD(SCE_GE_CMD_NOP);
                cmdOff = 0;
            }
        } else if (cmdOff == SCE_GE_CMD_RET) {
            // 13E0
            if ((old100 & 0x200) == 0) {
                // 1410
                if ((old100 & 0x100) == 0) {
                    // 0E48 dup
                    ret = 0x80000003;
                } else {
                    old108 = HW(0xBD400110);
                    // 1404 dup
                    HW(0xBD400120) = HW(0xBD400124);
                    old100 &= 0xFFFFFEFF;
                }
            } else {
                old108 = HW(0xBD400114);
                // 1404 dup
                HW(0xBD400120) = HW(0xBD400128);
                old100 = (old100 & 0xFFFFFDFF) | 0x100;
            }
            // 0BA4 dup
            cmd = HW_GE_CMD(SCE_GE_CMD_NOP);
            cmdOff = 0;
        } else if (cmdOff == SCE_GE_CMD_ORIGIN) {
            // 13C8
            HW(0xBD400120) = old108;
            cmdOff = 0;
            cmd = HW_GE_CMD(SCE_GE_CMD_NOP);
        } else if (cmdOff == SCE_GE_CMD_PRIM || cmdOff == SCE_GE_CMD_BEZIER || cmdOff == SCE_GE_CMD_SPLINE) {
            int addr = HW(0xBD400118);
            if (!GE_VALID_ADDR(addr))
                ret = 0x80000103;

            // 0F14
            if (((HW_GE_CMD(SCE_GE_CMD_VTYPE) >> 11) & 3) != 0)
            {
                int addr = HW(0xBD40011C);
                if (!GE_VALID_ADDR(addr))
                    ret = 0x80000103;
            }
            // 0F70
            if ((HW_GE_CMD(SCE_GE_CMD_TME) & 1) != 0) {
                int count = (HW_GE_CMD(SCE_GE_CMD_TMODE) >> 16) & 7;
                // 0FC0
                int i = 0;
                do {
                    int addr = ((HW_GE_CMD(SCE_GE_CMD_TBW0 + i) & 0x00FF0000) << 8) | (HW_GE_CMD(SCE_GE_CMD_TBP0 + i) & 0x00FFFFFF);
                    if (!GE_VALID_ADDR(addr)) {
                        // 1028
                        ret = 0x80000103;
                    }
                    i++;
                    // 1030
                } while (i <= count);
            }
        } else if (cmdOff == SCE_GE_CMD_AP2 && ((cmd >> 21) & 1) != 0) {
            // 12E4
            int count = (HW_GE_CMD(SCE_GE_CMD_TMODE) >> 16) & 7;
            // 1328
            int i = 0;
            do {
                int addr = ((HW_GE_CMD(SCE_GE_CMD_TBW0 + i) & 0x00FF0000) << 8) | (HW_GE_CMD(SCE_GE_CMD_TBP0 + i) & 0x00FFFFFF);
                if (!GE_VALID_ADDR(addr))
                    ret = 0x80000103;   // 1390

                // 1398
                i++;
            } while (i <= count);
        } else if (cmdOff == SCE_GE_CMD_CLOAD) {
            // 1240
            int addr = ((HW_GE_CMD(SCE_GE_CMD_CBW) << 8) & 0xFF000000) | (HW_GE_CMD(SCE_GE_CMD_CBP) & 0x00FFFFFF);
            if (!GE_VALID_ADDR(addr)) {
                // 0E68 dup
                ret = 0x80000103;
            }
        } else if (cmdOff == SCE_GE_CMD_XSTART) {
            int addr1 = ((HW_GE_CMD(SCE_GE_CMD_XBW1) & 0x00FF0000) << 8) | (HW_GE_CMD(SCE_GE_CMD_XBP1) & 0x00FFFFFF);
            int addr2 = ((HW_GE_CMD(SCE_GE_CMD_XBW2) & 0x00FF0000) << 8) | (HW_GE_CMD(SCE_GE_CMD_XBP2) & 0x00FFFFFF);
            if (!GE_VALID_ADDR(addr1) || !GE_VALID_ADDR(addr2)) {
                // 11B8
                ret = 0x80000103;
            }
        }
    }
    // 0BB0
    if (ret == 0) {
        int buf[32];
        int sp128, sp132;
        // 0BB8
        sp128 = HW_GE_STALLADDR;
        int *ptr = (int *)(((int)buf | 0x3F) + 1);
        sp132 = HW(0xBD400304);
        if (cmdOff == SCE_GE_CMD_FINISH) {
            // 0DC0
            ptr[0] = GE_MAKE_OP(SCE_GE_CMD_FINISH, cmd);
            ptr[1] = HW_GE_CMD(SCE_GE_CMD_END);
        } else if (cmdOff == SCE_GE_CMD_END) {
            // 0DA0
            ptr[0] = HW_GE_CMD(SCE_GE_CMD_FINISH);
            ptr[1] = GE_MAKE_OP(SCE_GE_CMD_END, cmd);
        } else if (cmdOff == SCE_GE_CMD_BASE) {
            // 0D78
            ptr[0] = GE_MAKE_OP(SCE_GE_CMD_BASE, cmd);
            ptr[1] = HW_GE_CMD(SCE_GE_CMD_FINISH);
            ptr[2] = HW_GE_CMD(SCE_GE_CMD_END);
        } else {
            ptr[0] = GE_MAKE_OP(SCE_GE_CMD_BASE, 0x00400000 | (HW_GE_CMD(SCE_GE_CMD_BASE) & 0x00FF0000));
            ptr[1] = GE_MAKE_OP(cmdOff, cmd);
            ptr[2] = HW_GE_CMD(SCE_GE_CMD_BASE);
            ptr[3] = HW_GE_CMD(SCE_GE_CMD_FINISH);
            ptr[4] = HW_GE_CMD(SCE_GE_CMD_END);
        }
        // 0C44
        pspCache(0x1A, ptr);
        if ((pspCop0StateGet(24) & 1) != 0) {
            pspSync();
            *(int *)(0xA7F00000) = ((int)ptr & 0x07FFFFC0) | 0xA0000001;
            (void)*(int *)(0xA7F00000);
        }
        // 0C88
        sceSysregSetMasterPriv(64, 1);
        HW(0xBD400310) = 4;
        HW_GE_DLIST = (int)UCACHED(ptr);
        HW_GE_STALLADDR = 0;
        pspSync();
        HW(0xBD400100) = old100 | 1;
        // 0CC0
        while ((HW(0xBD400100) & 1) != 0)
            ;
        // 0CD4
        while ((HW(0xBD400304) & 4) == 0)
            ;
        HW_GE_DLIST = old108;
        HW_GE_STALLADDR = sp128;
        HW(0xBD400310) = HW(0xBD400304) ^ sp132;
        sceSysregSetMasterPriv(64, 0);
    }
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
        return 0x80000102;
    int oldK1 = pspShiftK1();
    if (!pspK1PtrOk(mtx)) {
        // 1588
        pspSetK1(oldK1);
        return 0x80000023;
    }
    int oldIntr = sceKernelCpuSuspendIntr();
    int wasEnabled = sceSysregAwRegABusClockEnable();
    if (id == SCE_GE_MTX_PROJ) {
        // 1554, 1560
        // 1560
        int i;
        for (i = 0; i < 16; i++)
            mtx[i] = HW(0xBD400DE0 + i * 4);
    } else if (id == SCE_GE_MTX_TGEN) {
        // 152C
        int i;
        for (i = 0; i < 12; i++)
            mtx[i] = HW(0xBD400E20 + i * 4);
    } else {
        vs32 *out = HW_GE_BONE(id);
        // 14B0
        int i;
        for (i = 0; i < 12; i++)
            mtx[i] = out[i];
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
        return 0x80000102;
    int oldK1 = pspShiftK1();
    if (!pspK1PtrOk(mtx)) {
        // 16B8
        pspSetK1(oldK1);
        return 0x80000023;
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
        return 0x80000103;
    int oldK1 = pspShiftK1();
    if (!pspK1StaBufOk(ctx, 2048)) {
        // 1AA0
        pspSetK1(oldK1);
        return 0x80000023;
    }
    int oldIntr = sceKernelCpuSuspendIntr();
    int aBusWasEnabled = sceSysregAwRegABusClockEnable();
    if ((HW(0xBD400100) & 1) != 0) {
        // 1A8C
        sceKernelCpuResumeIntr(oldIntr);
        pspSetK1(oldK1);
        return -1;
    }
    u32 *curCmd = &ctx->ctx[17];
    ctx->ctx[0] = HW(0xBD400100);
    ctx->ctx[1] = HW_GE_DLIST;
    ctx->ctx[2] = HW_GE_STALLADDR;
    ctx->ctx[3] = HW(0xBD400110);
    ctx->ctx[4] = HW(0xBD400114);
    ctx->ctx[5] = HW(0xBD400118);
    ctx->ctx[6] = HW(0xBD40011C);
    ctx->ctx[7] = HW(0xBD400120);
    ctx->ctx[8] = HW(0xBD400124);
    ctx->ctx[9] = HW(0xBD400128);

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
    vs32 *bones = HW_GE_BONES;
    // 1894
    for (i = 0; i < 96; i++)
        *(curCmd++) = GE_MAKE_OP(SCE_GE_CMD_BONED, *(bones++));

    *(curCmd++) = GE_MAKE_OP(SCE_GE_CMD_WORLDN, 0);
    vs32 *worlds = HW_GE_WORLDS;
    for (i = 0; i < 12; i++)
        *(curCmd++) = GE_MAKE_OP(SCE_GE_CMD_WORLDD, *(worlds++));

    *(curCmd++) = GE_MAKE_OP(SCE_GE_CMD_VIEWN, 0);
    vs32 *views = HW_GE_VIEWS;
    // 190C
    for (i = 0; i < 12; i++)
        *(curCmd++) = GE_MAKE_OP(SCE_GE_CMD_VIEWD, *(views++));

    *(curCmd++) = GE_MAKE_OP(SCE_GE_CMD_PROJN, 0);
    vs32 *projs = HW_GE_PROJS;
    // 1948
    for (i = 0; i < 16; i++)
        *(curCmd++) = GE_MAKE_OP(SCE_GE_CMD_PROJD, *(projs++));

    *(curCmd++) = GE_MAKE_OP(SCE_GE_CMD_TGENN, 0);
    vs32 *tmtxs = HW_GE_TGENS;
    // 1984
    for (i = 0; i < 12; i++)
        *(curCmd++) = GE_MAKE_OP(SCE_GE_CMD_TGEND, *(tmtxs++));

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
        return 0x80000103;
    int oldK1 = pspShiftK1();
    if (!pspK1StaBufOk(ctx, 2048)) {
        // 1C80
        pspSetK1(oldK1);
        return 0x80000023;
    }
    int oldIntr = sceKernelCpuSuspendIntr();
    int state = sceSysregAwRegABusClockEnable();
    if ((HW(0xBD400100) & 1) != 0) {
        // 1C68
        sceKernelCpuResumeIntr(oldIntr);
        pspSetK1(oldK1);
        return 0x80000021;
    }
    int old304 = HW(0xBD400304);
    int old308 = HW(0xBD400308);
    HW(0xBD40030C) = old308;
    HW_GE_DLIST = (int)UCACHED(&ctx->ctx[17]);
    HW_GE_STALLADDR = 0;
    HW(0xBD400100) = ctx->ctx[0] | 1;
    // 1B64
    while ((HW(0xBD400100) & 1) != 0)
        ;
    int n304 = HW(0xBD400304);
    HW(0xBD400310) = (old304 ^ HW(0xBD400304)) & 0xFFFFFFFA;
    HW(0xBD400308) = old308;
    if ((n304 & 8) != 0)
        ret = -1;
    HW_GE_DLIST = ctx->ctx[1];
    HW_GE_STALLADDR = ctx->ctx[2];
    HW(0xBD400118) = ctx->ctx[5];
    HW(0xBD40011C) = ctx->ctx[6];
    HW(0xBD400120) = ctx->ctx[7];
    HW(0xBD400124) = ctx->ctx[8];
    HW(0xBD400128) = ctx->ctx[9];
    _sceGeSetInternalReg(6, 0, ctx->ctx[3], ctx->ctx[4]);
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
    return _sceGeSetInternalReg(2, 0, arg0, 0);
}

int _sceGeSetRegRadr2(int arg0)
{
    return _sceGeSetInternalReg(4, 0, 0, arg0);
}

int _sceGeSetInternalReg(int type, int arg1, int arg2, int arg3)
{
    int *cmdList = g_cmdList;
    if (cmdList == NULL)
        return 0;
    int oldIntr = sceKernelCpuSuspendIntr();
    int old304 = HW(0xBD400304);
    int old100 = HW(0xBD400100);
    int old108 = HW_GE_DLIST;
    int old10C = HW_GE_STALLADDR;
    int old120 = HW(0xBD400120);
    int old124 = HW(0xBD400124);
    int old128 = HW(0xBD400128);
    if ((type & 1) == 0)
        arg1 = HW_GE_CMD(SCE_GE_CMD_BASE);
    // 1D74
    cmdList[0] = 0x0F000000;
    cmdList[1] = 0x0C000000;
    cmdList += 2;
    cmdList[0] = 0x0C000000;
    pspSync();
    HW_GE_STALLADDR = 0;
    int *uncachedCmdList = UCACHED(cmdList);
    if (((type >> 1) & 1) && (arg2 != 0)) {
        int *uncachedNewCmdList = UCACHED(arg2 - 4);
        u32 cmd = (u32) (uncachedCmdList - old120);
        int oldCmd = uncachedNewCmdList[0];
        uncachedNewCmdList[0] = (cmd & 0x00FFFFFF) | 0x0A000000;
        pspCache(0x1A, uncachedNewCmdList);
        if ((pspCop0StateGet(24) & 1) != 0) {
            pspSync();
            *(int *)(0xA7F00000) =
                (((int)uncachedNewCmdList | 0x08000000) &
                 0x0FFFFFC0) | 0xA0000000;
            (void)*(int *)(0xA7F00000);
        }
        // 1E18
        cmdList[1] = ((cmd >> 24) << 16) | 0x10000000;
        cmdList[2] = 0x0C000000;
        pspSync();
        HW_GE_DLIST = (int)UCACHED(cmdList + 4);
        HW(0xBD400100) = 1;
        // 1E4C
        while ((HW(0xBD400100) & 1) != 0)
            ;
        HW_GE_DLIST = (int)UCACHED(uncachedNewCmdList);
        HW(0xBD400100) = 1;
        // 1E74
        while ((HW(0xBD400100) & 1) != 0)
            ;
        uncachedNewCmdList[0] = oldCmd;
        if ((pspCop0StateGet(24) & 1) != 0) {
            pspSync();
            *(int *)(0xA7F00000) =
                (((int)uncachedNewCmdList | 0x08000000) &
                 0x0FFFFFC0) | 0xA0000000;
            (void)*(int *)(0xA7F00000);
        }
    }
    // 1ED0
    // 1ED4
    if (((type >> 2) & 1) && (arg3 != 0)) {
        int *uncachedNewCmdList = UCACHED(arg3 - 4);
        int *cmd = uncachedCmdList - old120;
        int oldCmd = uncachedNewCmdList[0];
        uncachedNewCmdList[0] = ((u32) cmd & 0x00FFFFFF) | 0x0A000000;
        pspCache(0x1A, uncachedNewCmdList);
        if ((pspCop0StateGet(24) & 1) != 0) {
            pspSync();
            *(int *)(0xA7F00000) =
                (((int)uncachedNewCmdList | 0x80000000) &
                 0x0FFFFFC0) | 0xA0000000;
            (void)*(int *)(0xA7F00000);
        }
        // 1F50
        cmdList[1] = 0x10000000 | (((u32) cmd >> 24) << 16);
        cmdList[2] = 0x0C000000;
        pspSync();
        HW_GE_DLIST = (int)UCACHED(cmdList + 1);
        HW(0xBD400100) = 1;

        // 1F88
        while ((HW(0xBD400100) & 1) != 0)
            ;
        HW_GE_DLIST = (int)UCACHED(uncachedNewCmdList);
        HW(0xBD400100) = 0x101;

        // 1FB0
        while ((HW(0xBD400100) & 1) != 0)
            ;
        uncachedNewCmdList[0] = oldCmd;
        pspCache(0x1A, uncachedNewCmdList);
        if ((pspCop0StateGet(24) & 1) != 0) {
            pspSync();
            *(int *)(0xA7F00000) =
                (((int)uncachedNewCmdList | 0x08000000) &
                 0x0FFFFFC0) | 0xA0000000;
            (void)*(int *)(0xA7F00000);
        }
    }
    cmdList[0] = arg1;
    // 2010
    cmdList[1] = HW_GE_CMD(SCE_GE_CMD_END);
    HW_GE_DLIST = (int)uncachedCmdList;
    pspSync();
    HW(0xBD400100) = old100 | 1;

    // 2034
    while ((HW(0xBD400100) & 1) != 0)
        ;
    HW_GE_DLIST = old108;
    HW_GE_STALLADDR = old10C;
    HW(0xBD400120) = old120;
    HW(0xBD400124) = old124;
    HW(0xBD400128) = old128;

    if ((type & 2) != 0)
        HW(0xBD400110) = arg2;
    // 2084
    if ((type & 4) != 0)
        HW(0xBD400114) = arg3;

    // 2094
    HW(0xBD400310) = HW(0xBD400304) ^ old304;
    sceKernelCpuResumeIntr(oldIntr);
    return 0;
}

int
_sceGeInterrupt(int arg0 __attribute__ ((unused)), int arg1
                __attribute__ ((unused)), int arg2 __attribute__ ((unused)))
{
    int oldIntr = sceKernelCpuSuspendIntr();
    int attr = HW(0xBD400304);
    int unk1 = HW(0xBD400004);
    if ((attr & 8) != 0) {
        // 2228
        HW(0xBD400310) = 8;
        _sceGeErrorInterrupt(attr, unk1, arg2);
    }
    // 2118
    if ((attr & 5) == 5) {
        // 2218
        Kprintf("GE INTSIG/INTFIN at the same time\n"); // 0x6324
    }
    // 2128
    if ((attr & 4) == 0) {
        // 21AC
        if ((attr & 1) == 0 && (attr & 2) != 0) {   // 2208
            // 21FC dup
            HW(0xBD40030C) = 2;
        } else if ((attr & 1) != 0 && (attr & 2) == 0) {
            // 21FC dup
            HW(0xBD40030C) = 1;
        } else {
            // 21C0
            while ((HW(0xBD400100) & 1) != 0)
                ;
            HW(0xBD400310) = 3;
            HW(0xBD400308) = 3;
            _sceGeListInterrupt(attr, unk1, arg2);
        }
    } else {
        if ((attr & 2) == 0) {
            // 2198
            Kprintf("CMD_FINISH must be used with CMD_END.\n"); // 0x6348
            HW(0xBD400100) = 0;
        }
        // 213C
        while ((HW(0xBD400100) & 1) != 0)
            ;
        HW(0xBD400310) = 6;
        HW(0xBD400308) = 6;
        _sceGeFinishInterrupt(attr, unk1, arg2);
    }

    // 2170
    sceKernelCpuResumeIntrWithSync(oldIntr);
    return -1;
}

s32 _sceGeSysEventHandler(s32 ev_id, char *ev_name __attribute__((unused)), void *param, s32 *result __attribute__((unused)))
{
    switch (ev_id) {
    case 0x4005:
        // 2420
        sceSysregAwRegABusClockEnable();
        _sceGeQueueSuspend();
        sceGeSaveContext(&_aw_ctx);
        _aw_ctx.ctx[10] = HW(0xBD500070);
        _aw_ctx.ctx[11] = HW(0xBD500080);
        _aw_ctx.ctx[12] = HW(0xBD500000);
        _aw_ctx.ctx[13] = HW(0xBD500020);
        _aw_ctx.ctx[14] = HW(0xBD500030);
        _aw_ctx.ctx[15] = HW(0xBD500040);
        _aw_ctx.ctx[16] = HW(0xBD400200);
        break;

    case 0x10005:
        // 22C4
        sceSysregAwResetDisable();
        sceSysregAwRegABusClockEnable();
        sceSysregAwRegBBusClockEnable();
        sceSysregAwEdramBusClockEnable();
        sceGeEdramInit();
        _sceGeEdramResume();
        HW(0xBD400100) = 0;
        HW_GE_DLIST = 0;
        HW_GE_STALLADDR = 0;
        HW(0xBD400110) = 0;
        HW(0xBD400114) = 0;
        HW(0xBD400118) = 0;
        HW(0xBD40011C) = 0;
        HW(0xBD400120) = 0;
        HW(0xBD400124) = 0;
        HW(0xBD400128) = 0;
        HW(0xBD400310) = HW(0xBD400304);
        HW(0xBD40030C) = HW(0xBD400308);
        HW(0xBD400308) = 7;
        HW(0xBD500070) = _aw_ctx.ctx[10];
        HW(0xBD500080) = _aw_ctx.ctx[11];
        HW(0xBD500000) = _aw_ctx.ctx[12];
        HW(0xBD500020) = _aw_ctx.ctx[13];
        HW(0xBD500030) = _aw_ctx.ctx[14];
        HW(0xBD500040) = _aw_ctx.ctx[15];
        HW(0xBD400200) = _aw_ctx.ctx[16];
        sceSysregSetMasterPriv(64, 1);
        sceGeRestoreContext(&_aw_ctx);
        sceSysregSetMasterPriv(64, 0);
        _sceGeQueueResume();
        if (_sceGeQueueStatus() == 0)
            sceSysregAwRegABusClockDisable();
        break;

    case 0x4003:
        // 228C
        _sceGeEdramSuspend();
        if (*(int *)(param + 4) != 2) {
            sceSysregAwRegABusClockDisable();
            sceSysregAwRegBBusClockDisable();
            sceSysregAwEdramBusClockDisable();
        }
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

int sceGeRegisterLogHandler(void (*handler) ())
{
    g_GeLogHandler = handler;
    return 0;
}

int sceGeSetGeometryClock(int opt)
{
    int old = HW(0xBD400200);
    HW(0xBD400200) = opt & 1;
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
        HW(0xBD400400) = 2;
        sceSysregSetAwEdramSize(1);
    }
    // 25A0
    memcpy(UCACHED(sceGeEdramGetAddr()),
           UCACHED(sceKernelGetAWeDramSaveAddr()), g_uiEdramHwSize);
    sceKernelDcacheWritebackInvalidateAll();
    if (g_uiEdramHwSize == 0x00400000 && g_uiEdramSize == 0x00200000) { // 25F4
        HW(0xBD400400) = 4;
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
    HW(0xBD500010) = 1;
    // 2660
    while ((HW(0xBD500010) & 1) != 0)
        ;
    HW(0xBD500020) = 0x6C4;
    HW(0xBD500040) = 1;
    HW(0xBD500090) = 3;
    HW(0xBD400400) = 4;
    if (g_uiEdramHwSize != 0)
        return 0;
    if ((HW(0xBD500070) & 1) == 0) {
        // 2758
        g_edramAddrTrans = HW(0xBD500080) << 1;
    } else
        g_edramAddrTrans = 0;

    // 26C8
    if (sceSysregGetTachyonVersion() > 0x004FFFFF) {
        // 271C
        g_uiEdramSize = 0x00200000;
        g_uiEdramHwSize = 0x00400000;
        if (sceSysregSetAwEdramSize(0) != 0) {
            HW(0xBD400400) = 2;
            sceSysregSetAwEdramSize(1);
            g_uiEdramSize = 0x00400000;
        }
        return 0;
    }
    int size = (HW_GE_EDRAMSIZE & 0xFFFF) << 10;
    g_uiEdramSize = size;
    g_uiEdramHwSize = size;
    return 0;
}

int sceGeEdramSetRefreshParam(int arg0, int arg1, int arg2, int arg3)
{
    int ret = 0;
    int oldIntr = sceKernelCpuSuspendIntr();
    int old44 = HW(0xBC000044);
    HW(0xBC000044) &= 0xFF9BFFFF;
    if (arg0 != 0) {
        // 2858
        if (arg0 == 1) {
            HW(0xBD500000) =
                ((arg3 & 0xF) << 20) | (HW(0xBD500000) & 0xFF0FFFFF);
            HW(0xBD500030) = arg2 & 0x3FF;
            HW(0xBD500020) = arg1 & 0x007FFFFF;
            if ((HW(0xBD500040) & 2) == 0) {
                // 284C
                HW(0xBD500040) = 3;
            }
        } else
            ret = -1;
    } else {
        HW(0xBD500000) =
            ((arg3 & 0xF) << 20) | (HW(0xBD500000) & 0xFF0FFFFF);
        HW(0xBD500030) = arg2 & 0x3FF;
        HW(0xBD500020) = arg1 & 0x007FFFFF;
        if ((HW(0xBD500040) & 2) != 0) {
            // 284C dup
            HW(0xBD500040) = 1;
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
        sceGeSetReg(19, 4);
        // 2934 dup
        sceSysregSetAwEdramSize(0);
    } else if (size == 0x400000) {
        // 28FC
        if (sceSysregGetTachyonVersion() <= 0x4FFFFF)
            return 0x80000104;
        g_uiEdramSize = size;
        sceGeSetReg(19, 2);
        // 2934 dup
        sceSysregSetAwEdramSize(1);
    } else
        return 0x80000104;

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
        return 0x800001FE;

    // 29AC
    int oldIntr = sceKernelCpuSuspendIntr();
    g_edramAddrTrans = arg;
    if (arg == 0) {
        // 2A28
        if ((HW(0xBD500070) & 1) == 0) {
            ret = HW(0xBD500080) << 1;
            HW(0xBD500070) = 1;
        } else
            ret = 0;
    } else if ((HW(0xBD500070) & 1) == 0) {
        // 2A0C
        ret = HW(0xBD500080) << 1;
        HW(0xBD500080) = arg >> 1;
    } else {
        HW(0xBD500080) = arg >> 1;
        HW(0xBD500070) = 0;
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
        HW(0xBD400400) = 2;
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

    g_AwQueue.drawingEvFlagId = sceKernelCreateEventFlag("SceGeQueueId", 0x201, 2, NULL);
    g_AwQueue.listEvFlagIds[0] = sceKernelCreateEventFlag("SceGeQueueId", 0x201, -1, NULL);
    g_AwQueue.listEvFlagIds[1] = sceKernelCreateEventFlag("SceGeQueueId", 0x201, -1, NULL);
    g_AwQueue.syscallId = sceKernelQuerySystemCall((void*)sceGeListUpdateStallAddr);
    SceKernelGameInfo *info = sceKernelGetGameInfo();
    g_AwQueue.patched = 0;
    if (info != NULL && strcmp(info->gameId, "ULJM05127") == 0)
        g_AwQueue.patched = 1;
    return 0;
}

int _sceGeQueueSuspend()
{
    if (g_AwQueue.active_first == NULL)
        return 0;

    // 2C5C
    while ((HW(0xBD400100) & 1) != 0) {
        if (HW_GE_DLIST == HW_GE_STALLADDR) {
            int oldCmd1, oldCmd2;
            // 2DF8
            g_GeSuspend.unk4 = HW(0xBD400100) | 0x1;
            int *stall = (int *)HW_GE_STALLADDR;
            g_GeSuspend.unk12 = stall;
            g_GeSuspend.unk0 = HW(0xBD400004);
            g_GeSuspend.unk8 = HW_GE_DLIST;
            g_GeSuspend.unk16 = HW(0xBD400304);
            g_GeSuspend.unk20 = HW_GE_CMD(SCE_GE_CMD_SIGNAL);
            g_GeSuspend.unk24 = HW_GE_CMD(SCE_GE_CMD_FINISH);
            g_GeSuspend.unk28 = HW_GE_CMD(SCE_GE_CMD_END);
            oldCmd1 = stall[0];
            oldCmd2 = stall[1];
            stall[0] = GE_MAKE_OP(SCE_GE_CMD_FINISH, 0);
            stall[1] = GE_MAKE_OP(SCE_GE_CMD_END, 0);
            pspCache(0x1A, &stall[0]);
            pspCache(0x1A, &stall[1]);
            if ((pspCop0StateGet(24) & 1) != 0) {
                pspSync();
                *(int *)(0xA7F00000) =
                    (((int)stall | 0x08000000) & 0x0FFFFFC0) | 0xA0000001;
                *(int *)(0xA7F00000) = ((int)stall & 0x07FFFFC0) | 0xA0000001;
                (void)*(int *)(0xA7F00000);
            }
            // 2ECC
            HW_GE_STALLADDR += 8;
            // 2EE0
            while ((HW(0xBD400100) & 1) != 0)
                ;
            if (HW_GE_DLIST == (int)g_GeSuspend.unk12) {
                // 2F38
                HW_GE_STALLADDR = (int)stall;
                stall[0] = oldCmd1;
                stall[1] = oldCmd2;
                pspCache(0x1A, &stall[0]);
                pspCache(0x1A, &stall[1]);
                break;
            } else {
                // 2F08
                while ((HW(0xBD400304) & 4) == 0)
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
    if ((HW(0xBD400304) & 1) == 0 && (g_AwQueue.active_first->signal != SCE_GE_DL_SIGNAL_BREAK || g_AwQueue.isBreak != 0)) // 2DE8
    {
        // 2CB0
        while ((HW(0xBD400304) & 4) == 0)
            ;
    }
    // 2CC4
    g_GeSuspend.unk0 = HW(0xBD400004);
    g_GeSuspend.unk4 = HW(0xBD400100);
    g_GeSuspend.unk8 = HW_GE_DLIST;
    g_GeSuspend.unk12 = (int *)HW_GE_STALLADDR;
    g_GeSuspend.unk16 = HW(0xBD400304);
    g_GeSuspend.unk20 = HW_GE_CMD(SCE_GE_CMD_SIGNAL);
    g_GeSuspend.unk24 = HW_GE_CMD(SCE_GE_CMD_FINISH);
    g_GeSuspend.unk28 = HW_GE_CMD(SCE_GE_CMD_END);
    sceSysregSetMasterPriv(64, 1);
    int old108 = HW_GE_DLIST;
    int old10C = HW_GE_STALLADDR;
    int old304 = HW(0xBD400304);
    HW(0xBD400310) = 4;
    HW_GE_DLIST = (int)UCACHED(stopCmd);
    HW_GE_STALLADDR = 0;
    HW(0xBD400100) = 1;
    // 2D80
    while ((HW(0xBD400100) & 1) != 0)
        ;
    // 2D94
    while ((HW(0xBD400304) & 4) == 0)
        ;
    HW_GE_DLIST = old108;
    HW_GE_STALLADDR = old10C;
    HW(0xBD400310) = HW(0xBD400304) ^ old304;
    sceSysregSetMasterPriv(64, 0);
    return 0;
}

int _sceGeQueueResume()
{
    if (g_AwQueue.active_first == NULL)
        return 0;
    // 2F88
    sceSysregSetMasterPriv(64, 1);
    HW_GE_DLIST = (int)UCACHED(&g_GeSuspend.unk20);
    HW_GE_STALLADDR = 0;
    HW(0xBD400100) = g_GeSuspend.unk4 | 1;
    // 2FC0
    while ((HW(0xBD400100) & 1) != 0)
        ;
    // 2FD4
    while ((HW(0xBD400304) & 4) == 0)
        ;
    sceSysregSetMasterPriv(64, 0);
    int oldFlag = g_GeSuspend.unk16;
    int flag = 0;
    if ((oldFlag & 1) == 0)
        flag |= 1;
    if ((oldFlag & 2) == 0)
        flag |= 2;
    if ((oldFlag & 4) == 0)
        flag |= 4;
    HW(0xBD400310) = flag;
    HW_GE_DLIST = g_GeSuspend.unk8;
    HW_GE_STALLADDR = (int)g_GeSuspend.unk12;
    HW(0xBD400100) = g_GeSuspend.unk4;
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
            HW(0xBD400100) |= 1;
            pspSync();
            return;
        } else if (dl->signal == SCE_GE_DL_SIGNAL_PAUSE) {
            // 3468
            int state = HW(0xBD400100);
            dl->flags = state;
            dl->list = (int *)HW_GE_DLIST;
            dl->unk28 = HW(0xBD400110);
            dl->unk32 = HW(0xBD400114);
            dl->unk36 = HW(0xBD400120);
            dl->unk40 = HW(0xBD400124);
            dl->unk44 = HW(0xBD400128);
            dl->unk48 = HW_GE_CMD(SCE_GE_CMD_BASE);
            if ((state & 0x200) == 0) {
                dl->unk32 = 0;
                dl->unk44 = 0;
                if ((state & 0x100) == 0) {
                    dl->unk28 = 0;
                    dl->unk40 = 0;
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
                    sceKernelCallSubIntrHandler(25, dl->cbId * 2, dl->unk54, (int)list);    // call signal CB
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
                int *cmdList = (int *)HW_GE_DLIST;
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
                    g_GeLogHandler(6, (int)dl ^ g_dlMask,
                                 cmdList, lastCmd1, lastCmd2);
                }
                // 3348
                if (dl->cbId >= 0) {
                    if (g_AwQueue.sdkVer <= 0x02000010)
                        cmdList = NULL;
                    sceKernelCallSubIntrHandler(25, dl->cbId * 2 + 1, lastCmd1 & 0xFFFF, (int)cmdList); // call finish CB
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
        HW(0xBD400100) = 0;
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
                sceKernelCallSubIntrHandler(25, dl2->cbId * 2,
                                            dl2->unk54, (int)list);
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
            HW(0xBD400100) = 0;
            HW_GE_STALLADDR = (int)UCACHED(dl2->stall);
            HW_GE_DLIST = (int)UCACHED(dl2->list);
            HW(0xBD400120) = dl2->unk36;
            HW(0xBD400124) = dl2->unk40;
            HW(0xBD400128) = dl2->unk44;
            _sceGeSetBaseRadr(dl2->unk48, dl2->unk28, dl2->unk32);
            pspSync();
            g_AwQueue.curRunning = dl2;
            HW(0xBD400100) = dl2->flags | 1;
            pspSync();
            if (g_GeLogHandler != 0) {
                // 321C
                g_GeLogHandler(5, (int)dl2 ^ g_dlMask, dl2->list, dl2->stall);
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
    u32 *cmdList = (u32 *)HW_GE_DLIST;
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
        g_GeLogHandler(7, (int)dl ^ g_dlMask, cmdList, lastCmd1, lastCmd2);
    }
    // 3618
    switch ((lastCmd1 >> 16) & 0xFF) {
    case GE_SIGNAL_HANDLER_SUSPEND:
        // 3670
        if (g_AwQueue.sdkVer <= 0x02000010) {
            dl->state = SCE_GE_DL_STATE_PAUSED;
            cmdList = NULL;
        }
        // 3698
        if (dl->cbId >= 0) {
            sceKernelCallSubIntrHandler(25, dl->cbId * 2,
                                        lastCmd1 & 0xFFFF, (int)cmdList);
            pspSync();
        }
        // 36B4
        if (g_AwQueue.sdkVer <= 0x02000010)
            dl->state = lastCmd2 & 0xFF;
        HW(0xBD400100) |= 1;
        break;

    case GE_SIGNAL_HANDLER_CONTINUE:
        // 3708
        HW(0xBD400100) |= 1;
        pspSync();
        if (dl->cbId >= 0) {
            if (g_AwQueue.sdkVer <= 0x02000010)
                cmdList = NULL;
            sceKernelCallSubIntrHandler(25, dl->cbId * 2,
                                        lastCmd1 & 0xFFFF, (int)cmdList);
        }
        break;

    case GE_SIGNAL_HANDLER_PAUSE:
        dl->state = SCE_GE_DL_STATE_PAUSED;
        dl->signal = lastCmd2 & 0xFF;
        dl->unk54 = lastCmd1;
        HW(0xBD400100) |= 1;
        break;

    case GE_SIGNAL_SYNC:
        // 3994
        dl->signal = SCE_GE_DL_SIGNAL_SYNC;
        HW(0xBD400100) |= 1;
        break;

    case GE_SIGNAL_CALL:
    case GE_SIGNAL_RCALL:
    case GE_SIGNAL_OCALL:
        {
            // 3870
            if (dl->stackOff >= dl->numStacks) {
                // 398C
                _sceGeListError(lastCmd1, 1);
                break;
            }
            SceGeStack *curStack = &dl->stack[dl->stackOff++];
            curStack->stack[0] = HW(0xBD400100);
            curStack->stack[1] = HW_GE_DLIST;
            curStack->stack[2] = HW(0xBD400120);
            curStack->stack[3] = HW(0xBD400124);
            curStack->stack[4] = HW(0xBD400128);
            curStack->stack[5] = HW(0xBD400110);
            curStack->stack[6] = HW(0xBD400114);
            curStack->stack[7] = HW_GE_CMD(SCE_GE_CMD_BASE);
            if ((dl->flags & 0x200) == 0) {
                dl->unk32 = 0;
                dl->unk44 = 0;
                if ((dl->flags & 0x100) == 0) {
                    dl->unk40 = 0;
                    dl->unk28 = 0;
                }
            }
            // (3924)
            // 3928
            int cmdOff = (lastCmd1 << 16) | (lastCmd2 & 0xFFFF);
            u32 *newCmdList;
            if (((lastCmd1 >> 16) & 0xFF) == GE_SIGNAL_CALL)
                newCmdList = (u32 *) cmdOff;
            else if (((lastCmd1 >> 16) & 0xFF) == GE_SIGNAL_RCALL)
                newCmdList = &cmdList[cmdOff / 4 - 2];
            else
                newCmdList = HW(0xBD400120) + (u32 *) cmdOff;

            // 395C
            if (((int)newCmdList & 3) != 0) {
                // 397C
                _sceGeListError(lastCmd1, 0);
            }
            // 396C
            HW_GE_DLIST = (int)UCACHED(newCmdList);
            HW(0xBD400100) = 1;
            pspSync();
            break;
        }

    case GE_SIGNAL_JUMP:
    case GE_SIGNAL_RJUMP:
    case GE_SIGNAL_OJUMP:
        {
            // 377C
            int cmdOff = (lastCmd1 << 16) | (lastCmd2 & 0xFFFF);
            u32 *newCmdList;
            if (((lastCmd1 >> 16) & 0xFF) == GE_SIGNAL_JUMP)
                newCmdList = (u32 *) cmdOff;
            else if (((lastCmd1 >> 16) & 0xFF) == GE_SIGNAL_RJUMP)
                newCmdList = &cmdList[cmdOff / 4 - 2];
            else
                newCmdList = HW(0xBD400120) + (u32 *) cmdOff;

            // 37B4
            if (((int)newCmdList & 3) != 0) {
                // 37D0
                _sceGeListError(lastCmd1, 0);
            }
            // 37C4
            HW_GE_DLIST = (int)UCACHED(newCmdList);
            HW(0xBD400100) |= 1;
            break;
        }

    case GE_SIGNAL_RET:
        // 37E0
        if (dl->stackOff == 0) {
            _sceGeListError(lastCmd1, 2);
            return;
        }
        // 3804
        SceGeStack *curStack = &dl->stack[dl->stackOff--];
        HW_GE_DLIST = (int)UCACHED(curStack->stack[1]);
        HW(0xBD400120) = curStack->stack[2];
        HW(0xBD400124) = curStack->stack[3];
        HW(0xBD400128) = curStack->stack[4];
        _sceGeSetBaseRadr(curStack->stack[7], curStack->stack[5],
                          curStack->stack[6]);
        HW(0xBD400100) = curStack->stack[0] | 1;
        pspSync();
        break;

    case GE_SIGNAL_RTBP0: case GE_SIGNAL_RTBP1: case GE_SIGNAL_RTBP2: case GE_SIGNAL_RTBP3:
    case GE_SIGNAL_RTBP4: case GE_SIGNAL_RTBP5: case GE_SIGNAL_RTBP6: case GE_SIGNAL_RTBP7:
    case GE_SIGNAL_OTBP0: case GE_SIGNAL_OTBP1: case GE_SIGNAL_OTBP2: case GE_SIGNAL_OTBP3:
    case GE_SIGNAL_OTBP4: case GE_SIGNAL_OTBP5: case GE_SIGNAL_OTBP6: case GE_SIGNAL_OTBP7:
        {
            // 39D0
            int off = (lastCmd1 << 16) | (lastCmd2 & 0xFFFF);
            u32 *newLoc;
            if (((lastCmd1 >> 19) & 1) == 0) // RTBP
                newLoc = lastCmdPtr1 + off;
            else // OTBP
                newLoc = HW(0xBD400120) + (u32 *) off;

            // 39F0
            if (((int)newLoc & 0xF) != 0) {
                // 3A50
                _sceGeListError(lastCmd1, 0);
            }
            // 3A00
            int id = (lastCmd1 >> 16) & 0x7;
            int *uncachedCmdPtr = UUNCACHED(lastCmdPtr1);
            uncachedCmdPtr[0] = GE_MAKE_OP(SCE_GE_CMD_TBP0 + id, (int)newLoc);
            uncachedCmdPtr[1] = GE_MAKE_OP(SCE_GE_CMD_TBW0 + id, ((((int)newLoc >> 24) & 0xF) << 16) | ((lastCmd2 >> 16) & 0xFF));
            // 3A40
            pspSync();
            HW_GE_DLIST = (int)lastCmdPtr1;
            HW(0xBD400100) |= 1;
            break;
        }

    case GE_SIGNAL_RCBP:
    case GE_SIGNAL_OCBP:
        {
            // 3A80
            int off = (lastCmd1 << 16) | (lastCmd2 & 0xFFFF);
            u32 *newLoc;
            if (((lastCmd1 >> 19) & 1) == 0) // RCBP
                newLoc = lastCmdPtr1 + off;
            else // OCBP
                newLoc = HW(0xBD400120) + (u32 *) off;

            // 3AA4
            if (((int)newLoc & 0xF) != 0) {
                // 3AE8
                _sceGeListError(lastCmd1, 0);
            }
            // 3AB4
            int *uncachedCmdPtr = UUNCACHED(lastCmdPtr1);
            uncachedCmdPtr[0] = GE_MAKE_OP(SCE_GE_CMD_CBP, (int)newLoc);
            uncachedCmdPtr[1] = GE_MAKE_OP(SCE_GE_CMD_CBW, (((int)newLoc >> 24) & 0xF) << 16);
            // 3A40
            pspSync();
            HW_GE_DLIST = (int)lastCmdPtr1;
            HW(0xBD400100) |= 1;
            break;
        }

    case GE_SIGNAL_BREAK1:
    case GE_SIGNAL_BREAK2:
        {
            // 3B08
            if (g_deci2p == NULL) {
                HW(0xBD400100) |= 1;
                return;
            }
            int opt = 0;
            if (((lastCmd1 >> 16) & 0xFF) == 0xFF) {
                // 3B6C
                SceGeBpCmd *bpCmd = &g_GeDeciBreak.cmds[0];
                int i;
                opt = 1;
                for (i = 0; i < g_GeDeciBreak.size; i++) {
                    // 3B90
                    if (UCACHED(bpCmd->unk0) == UCACHED(lastCmdPtr1)) {
                        // 3BC0
                        if (bpCmd->unk4 == 0
                            || (bpCmd->unk4 != -1 && (--bpCmd->unk4) != 0))
                            opt = -1;
                        break;
                    }
                    bpCmd++;
                }
                // 3BB0
                if (opt < 0) {
                    HW(0xBD400100) |= 1;
                    return;
                }
            }
            // 3B28
            HW_GE_DLIST = (int)lastCmdPtr1;
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
        return 0x80000100;
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
                g_GeLogHandler(1, dlId);
            }
            ret = 0;
        } else
            ret = 0x80000021;
    } else
        ret = 0x80000100;

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
        return 0x80000100;
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
            if ((int)dl->stall != HW_GE_DLIST)
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
            ret = 0x80000100;
            break;
        }
    } else
        ret = 0x80000107;

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
                if ((int)dl->stall != HW_GE_DLIST)
                    ret = SCE_GE_LIST_DRAWING;
                else
                    ret = SCE_GE_LIST_STALLING;
            } else
                ret = SCE_GE_LIST_COMPLETED;
        }
        // 3F8C
        sceKernelCpuResumeIntr(oldIntr);
    } else
        ret = 0x80000107;

    // 3F24
    pspSetK1(oldK1);
    return ret;
}

int sceGeBreak(u32 resetQueue, void *arg1)
{
    int ret;
    if (resetQueue >= 2)
        return 0x80000107;
    int oldK1 = pspShiftK1();
    if (!pspK1StaBufOk(arg1, 16)) {
        // 4368
        pspSetK1(oldK1);
        return 0x80000023;
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
            HW(0xBD400100) = 0;
            pspSync();

            // 4180
            while ((HW(0xBD400100) & 1) != 0)
                ;
            if (g_AwQueue.curRunning != NULL) {
                int *cmdList = (int *)HW_GE_DLIST;
                int state = HW(0xBD400100);
                dl->list = cmdList;
                dl->flags = state;
                dl->stall = (int *)HW_GE_STALLADDR;
                dl->unk28 = HW(0xBD400110);
                dl->unk32 = HW(0xBD400114);
                dl->unk36 = HW(0xBD400120);
                dl->unk40 = HW(0xBD400124);
                dl->unk44 = HW(0xBD400128);
                dl->unk48 = HW_GE_CMD(SCE_GE_CMD_BASE);
                if ((state & 0x200) == 0) {
                    dl->unk32 = 0;
                    dl->unk44 = 0;
                    if ((state & 0x100) == 0) {
                        dl->unk28 = 0;
                        dl->unk40 = 0;
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
            HW_GE_DLIST = (int)UUNCACHED(g_cmdList);
            HW(0xBD400310) = 6;
            HW(0xBD400100) = 1;
            pspSync();
            g_AwQueue.isBreak = 1;
            g_AwQueue.curRunning = NULL;
            ret = (int)dl ^ g_dlMask;
        } else if (dl->state == SCE_GE_DL_STATE_PAUSED) {
            // 4130
            ret = 0x80000021;
            if (g_AwQueue.sdkVer > 0x02000010) {
                if (dl->signal == SCE_GE_DL_SIGNAL_PAUSE) {
                    // 4160
                    Kprintf("sceGeBreak(): can't break signal-pausing list\n"); // 64B4
                } else
                    ret = 0x80000020;
            }
        } else if (dl->state == SCE_GE_DL_STATE_QUEUED) {
            // 40FC
            dl->state = SCE_GE_DL_STATE_PAUSED;
            ret = (int)dl ^ g_dlMask;
        } else if (g_AwQueue.sdkVer >= 0x02000000)
            ret = 0x80000004;
        else
            ret = -1;
    } else
        ret = 0x80000020;

    // 410C
    if (ret == 0 && g_GeLogHandler != NULL)
        g_GeLogHandler(3, resetQueue);
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
                    while ((HW(0xBD400100) & 1) != 0)
                        ;
                    HW(0xBD400310) = 6;
                    if (dl->ctx != NULL && dl->ctxUpToDate == 0) {
                        // 4598
                        sceGeSaveContext(dl->ctx);
                    }
                    // 44BC
                    _sceGeWriteBp(dl->list);
                    dl->ctxUpToDate = 1;
                    HW_GE_DLIST = (int)UCACHED(dl->list);
                    HW_GE_STALLADDR = (int)UCACHED(dl->stall);
                    HW(0xBD400120) = dl->unk36;
                    HW(0xBD400124) = dl->unk40;
                    HW(0xBD400128) = dl->unk44;
                    _sceGeSetBaseRadr(dl->unk48, dl->unk28, dl->unk32);
                    HW(0xBD400310) = 6;
                    HW(0xBD400100) = dl->flags | 1;
                    pspSync();
                    g_AwQueue.curRunning = dl;
                    sceKernelClearEventFlag(g_AwQueue.drawingEvFlagId, 0xFFFFFFFD);
                    if (g_GeLogHandler != NULL) {
                        g_GeLogHandler(4, 0);
                        if (g_GeLogHandler != NULL)
                            g_GeLogHandler(5, (int)dl ^ g_dlMask, dl->list, dl->stall);
                    }
                } else {
                    // 45A8
                    ret = 0x80000021;
                }
            } else {
                dl->state = SCE_GE_DL_STATE_QUEUED;
                if (g_GeLogHandler != NULL)
                    g_GeLogHandler(4, 0);
            }
        } else if (dl->state == SCE_GE_DL_STATE_RUNNING) {
            // 4428
            if (g_AwQueue.sdkVer >= 0x02000000)
                ret = 0x80000020;
            else
                ret = -1;
        } else {
            if (g_AwQueue.sdkVer >= 0x02000000)
                ret = 0x80000004;
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
        return 0x80000023;
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
        return 0x80000022;
    }

    if (cb->signal_func != NULL)
        sceKernelRegisterSubIntrHandler(25, i * 2 + 0, cb->signal_func,
                                        cb->signal_arg);
    // 469C
    if (cb->finish_func != 0)
        sceKernelRegisterSubIntrHandler(25, i * 2 + 1, cb->finish_func,
                                        cb->finish_arg);
    // 46B4
    sceKernelEnableSubIntr(25, i * 2 + 0);
    sceKernelEnableSubIntr(25, i * 2 + 1);
    pspSetK1(oldK1);
    return i;
}

int sceGePutBreakpoint(int *inPtr, int size)
{
    if (size >= 9)
        return 0x80000104;
    int oldIntr = sceKernelCpuSuspendIntr();
    int oldK1 = pspShiftK1();
    if (!pspK1DynBufOk(inPtr, size * 8)) {
        // 4850 dup
        pspSetK1(oldK1);
        sceKernelCpuResumeIntr(oldIntr);
        return 0x80000023;
    }
    // 477C
    int i;
    for (i = 0; i < size; i++) {
        int (*func) (int, int, int) = (void *)*(int *)(g_deci2p + 36);
        if (func(inPtr[i * 2], 4, 3) < 0) {
            // 4850 dup
            pspSetK1(oldK1);
            sceKernelCpuResumeIntr(oldIntr);
            return 0x80000023;
        }
    }
    // 47A8
    _sceGeClearBp();
    g_GeDeciBreak.size = size;
    // 47C8
    for (i = 0; i < g_GeDeciBreak.size; i++) {
        g_GeDeciBreak.cmds[i].unk0 = inPtr[i * 2 + 0] & 0xFFFFFFFC;
        g_GeDeciBreak.cmds[i].unk4 = inPtr[i * 2 + 1];
    }
    // 47F4
    if (g_GeDeciBreak.busy == 0) {
        // 4840
        _sceGeWriteBp(0);
    }
    pspSetK1(oldK1);
    // 4804
    sceKernelCpuResumeIntr(oldIntr);
    return 0;
}

int sceGeGetBreakpoint(int *outPtr, int size, int *arg2)
{
    int oldIntr = sceKernelCpuSuspendIntr();
    int oldK1 = pspShiftK1();
    if (!pspK1DynBufOk(outPtr, size * 8) || !pspK1PtrOk(arg2)) {
        // 48C0
        pspSetK1(oldK1);
        sceKernelCpuResumeIntr(oldIntr);
        return 0x80000023;
    }
    // 48EC
    SceGeBpCmd *bpCmd = &g_GeDeciBreak.cmds[0];
    int count = 0;
    // 4904
    int i;
    for (i = 0; i < g_GeDeciBreak.size; i++) {
        if (i < size) {
            count++;
            outPtr[0] = bpCmd->unk0 & 0xFFFFFFFC;
            outPtr[1] = bpCmd->unk4;
            outPtr += 2;
        }
        // 4930
        bpCmd++;
    }
    // 4940
    if (arg2 != 0)
        *arg2 = g_GeDeciBreak.size;
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
        return 0x80000023;
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
        return 0x80000023;
    }
    // 4AB8
    SceGeDisplayList *dl = (SceGeDisplayList *) (dlId ^ g_dlMask);
    if (dl < g_displayLists || dl >= &g_displayLists[64]) {
        // 4B40
        sceKernelCpuResumeIntr(oldIntr);
        return 0x80000100;
    }
    if (outDl != NULL) {
        int *outPtr = (int *)outDl;
        int *inPtr = (int *)dl;
        // 4AE8
        do {
            outPtr[0] = inPtr[0];
            outPtr[1] = inPtr[1];
            outPtr[2] = inPtr[2];
            outPtr[3] = inPtr[3];
            outPtr += 4;
            inPtr += 4;
        } while (inPtr != (int *)(dl + 64));
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
        return 0x80000102;
    }
    if (!pspK1PtrOk(stack)) {
        // 4C2C
        pspSetK1(oldK1);
        sceKernelCpuResumeIntr(oldIntr);
        return 0x80000023;
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
        return 0x80000020;
    }
    g_GeDeciBreak.size2 = 0;
    g_GeDeciBreak.busy = 1;
    int wasEnabled = sceSysregAwRegABusClockEnable();
    HW(0xBD400100) = 0;
    // 4C9C
    while ((HW(0xBD400100) & 1) != 0)
        ;
    if (!wasEnabled) {
        // 4CE4
        sceSysregAwRegABusClockDisable();
    }
    // 4CB8
    _sceGeClearBp();
    sceKernelDisableIntr(25);
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
        return 0x80000021;
    }
    int wasEnabled = sceSysregAwRegABusClockEnable();
    if ((HW(0xBD400304) & 2) != 0) {
        // 5084
        if (!wasEnabled) {
            // 50A0
            sceSysregAwRegABusClockDisable();
        }
        sceKernelCpuResumeIntr(oldIntr);
        return 0x80000021;
    }
    int *cmdPtr = (int *)HW_GE_DLIST;
    u32 curCmd = *cmdPtr;
    int hasSignalFF = 0;
    if ((curCmd >> 24) == SCE_GE_CMD_SIGNAL && ((curCmd >> 16) & 0xFF) == GE_SIGNAL_BREAK2) // 5044
    {
        cmdPtr += 2;
        HW_GE_DLIST = (int)cmdPtr;
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
        int flag = HW(0xBD400100);
        int op = curCmd >> 24;
        if ((op == SCE_GE_CMD_JUMP || op == SCE_GE_CMD_BJUMP) || (op == SCE_GE_CMD_CALL && arg0 == 1))
        {
            // 4DCC
            if (op != SCE_GE_CMD_BJUMP || (flag & 2) == 0) // 5038
                nextCmdPtr1 = (int *)((((HW_GE_CMD(SCE_GE_CMD_BASE) << 8) & 0xFF000000)
                                      | (curCmd & 0x00FFFFFF)) + HW(0xBD400120));
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
                    nextCmdPtr1 = (int *)HW(0xBD400110);
            } else
                nextCmdPtr1 = (int *)HW(0xBD400114);
        } else {
            if (op == SCE_GE_CMD_FINISH)
                nextCmdPtr1 = nextCmdPtr2;
            else if (op == SCE_GE_CMD_SIGNAL)
            {
                // 4F54
                int signalOp = (curCmd >> 16) & 0x000000FF;
                int off = (curCmd << 16) | (*(cmdPtr + 1) & 0xFFFF);
                nextCmdPtr1 = nextCmdPtr2;
                if (signalOp != 0x10 && (signalOp != 0x11 || arg0 != 1)) {
                    // 4F94
                    if (signalOp == 0x13) {
                        // 4FAC
                        nextCmdPtr1 = cmdPtr + off;
                    } else {
                        if (signalOp != 0x14 || arg0 != 1) {
                            // 4FB4
                            if (signalOp != 0x15) {
                                if ((signalOp != 0x16 || arg0 != 1) && signalOp == 0x12 && dl->stackOff != 0)   // 4FDC
                                    nextCmdPtr1 = (int *)
                                        dl->stack[dl->stackOff - 1].stack[1];
                            } else {
                                // 4FCC
                                nextCmdPtr1 = (int *)HW(0xBD400120) + off;
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
        g_GeDeciBreak.cmds[8].unk4 = 1;
        g_GeDeciBreak.size2 = 1;
        g_GeDeciBreak.cmds[8].unk0 = (int)nextCmdPtr1;
        int maxCount = g_GeDeciBreak.size;
        SceGeBpCmd *bpCmd = &g_GeDeciBreak.cmds[0];
        // 4E50
        int i;
        for (i = 0; i < maxCount; i++) {
            // 4E50
            if (UCACHED(bpCmd->unk0) == UCACHED(nextCmdPtr1)) {
                // 4F4C
                g_GeDeciBreak.size2 = 0;
                break;
            }
            bpCmd++;
        }
        // 4E6C
        op = *nextCmdPtr1 >> 24;
        if (op == SCE_GE_CMD_SIGNAL && ((*nextCmdPtr1 >> 16) & 0xFF) == 0xFF) {   // 4F38
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
            if (UCACHED(bpCmd->unk0) == UCACHED(cmdPtr)) {
                // 4F10
                bpCmd->unk0 |= 1;
                if (~bpCmd->unk4 != 0 && bpCmd->unk4 != 0)
                    bpCmd->unk4--;
            }
            // 4EB4
            bpCmd++;
        }
    }
    // 4EC4
    _sceGeWriteBp(cmdPtr);
    HW(0xBD400100) |= 1;
    sceKernelEnableIntr(25);
    // 4EE8 dup
    sceKernelCpuResumeIntr(oldIntr);
    return 0;
}

int _sceGeQueueInitCallback()
{
    SceKernelUsersystemLibWork *libWork = sceKernelGetUsersystemLibWork();
    if (libWork != NULL) {
        u32 *syscPtr = (void*)libWork->sceGeListUpdateStallAddr_lazy + 204; // TODO: replace with a difference of two functions from usersystemlib
        *syscPtr = MAKE_SYSCALL(g_AwQueue.syscallId);
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
        return 0x80000100;
    }
    int oldIntr = sceKernelCpuSuspendIntr();
    sceKernelDisableSubIntr(25, cbId * 2 + 0);
    sceKernelDisableSubIntr(25, cbId * 2 + 1);
    sceKernelReleaseSubIntrHandler(25, cbId * 2 + 0);
    sceKernelReleaseSubIntrHandler(25, cbId * 2 + 1);
    g_cbhook &= ~(1 << cbId);
    sceKernelCpuResumeIntr(oldIntr);
    pspSetK1(oldK1);
    return 0;
}

void _sceGeListError(u32 cmd, int err)  // err: 0 = alignment problem, 1: overflow, 2: underflow
{
    int op = (cmd >> 16) & 0xFF;
    int *curAddr = (int *)HW_GE_DLIST - 2;
    char *cmdStr;
    switch (cmd)                // 0x6794 jump table
    {
    case 0x10:
        // 52D8
        cmdStr = "JUMP";
        break;

    case 0x11:
        // 5358
        cmdStr = "CALL";
        break;

    case 0x12:
        // 5364
        cmdStr = "RET";
        break;

    case 0x13:
        // 5370
        cmdStr = "RJUMP";
        break;

    case 0x14:
        // 537C
        cmdStr = "RCALL";
        break;

    case 0x15:
        // 5388
        cmdStr = "OJUMP";
        break;

    case 0x16:
        // 5394
        cmdStr = "OCALL";
        break;

    case 0x30:
        // 53A0
        cmdStr = "RCBP";
        break;

    case 0x38:
        // 53D8
        cmdStr = "OCBP";
        break;

    default:
        // 53AC
        if (op >= 0x28)
            g_szTbp[0] = 'O';   // at 6880
        // 53C4
        cmdStr = g_szTbp;
        cmdStr[4] = '0' + (op & 7);
        break;
    }

    // 52E0
    switch (err) {
    case 0:
        // 533C
        Kprintf("SCE_GE_SIGNAL_%s address error (MADR=0x%08X)\n",
                cmdStr, curAddr);
        break;

    case 1:
        // 534C
        Kprintf("SCE_GE_SIGNAL_%s stack overflow (MADR=0x%08X)\n",
                cmdStr, curAddr);
        break;

    case 2:
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
    if (g_GeDeciBreak.clear != 0)
        return;
    g_GeDeciBreak.clear = 1;
    int *prevList = list - 1;
    int i;
    for (i = 0; i < g_GeDeciBreak.size; i++) {
        SceGeBpCmd *bpCmd = &g_GeDeciBreak.cmds[i];
        int *ptr2 = (int *)(bpCmd->unk0 & 0xFFFFFFFC);
        u32 cmd = ptr2[0];
        bpCmd->unk8 = cmd;
        bpCmd->unk12 = ptr2[1];
        if (((bpCmd->unk0 ^ (int)prevList) & 0x1FFFFFFC) != 0) {
            // 5574
            if ((bpCmd->unk0 & 3) == 0) {
                // 55C4
                if (cmd >> 24 != SCE_GE_CMD_END)
                    ptr2[0] = GE_MAKE_OP(SCE_GE_CMD_SIGNAL, GE_SIGNAL_BREAK2 << 24);
                // 55CC
                ptr2[1] = GE_MAKE_OP(SCE_GE_CMD_END, 0);
            } else
                bpCmd->unk0 = (int)ptr2;
            // 5580
            pspCache(0x1A, &ptr2[0]);
            pspCache(0x1A, &ptr2[1]);
            if ((pspCop0StateGet(24) & 1) != 0) {
                pspSync();
                *(int *)(0xA7F00000) =
                    (((int)ptr2 | 0x08000000) & 0x0FFFFFC0) | 0xA0000001;
                (void)*(int *)(0xA7F00000);
            }
        }
        // 5494
    }

    // 54A4
    SceGeBpCmd *bpCmd = &g_GeDeciBreak.cmds[8];

    // 54E0
    for (i = 0; i < g_GeDeciBreak.size2; i++) {
        int *ptr = (int *)bpCmd->unk0;
        bpCmd->unk8 = ptr[0];
        bpCmd->unk12 = ptr[1];
        if ((((int)ptr ^ (int)prevList) & 0x1FFFFFFC) != 0) {
            // 5528
            ptr[0] = GE_MAKE_OP(SCE_GE_CMD_SIGNAL, GE_SIGNAL_BREAK1 << 24);
            ptr[1] = GE_MAKE_OP(SCE_GE_CMD_END, 0);
            pspCache(0x1A, &ptr[0]);
            pspCache(0x1A, &ptr[1]);
            if ((pspCop0StateGet(24) & 1) != 0) {
                pspSync();
                *(int *)(0xA7F00000) =
                    (((int)ptr | 0x08000000) & 0x0FFFFFC0) | 0xA0000001;
                (void)*(int *)(0xA7F00000);
            }
        }
        // 5504
        bpCmd++;
    }

    // 5514
    pspSync();
}

void _sceGeClearBp()
{
    if (g_deci2p == NULL)
        return;
    if (g_GeDeciBreak.clear == 0)
        return;
    g_GeDeciBreak.clear = 0;
    // 5624
    int i;
    for (i = 0; i < g_GeDeciBreak.size2; i++) {
        SceGeBpCmd *cmd = &g_GeDeciBreak.cmds[7 + (g_GeDeciBreak.size2 - i)];
        int *out = (int *)cmd->unk0;
        out[0] = cmd->unk8;
        out[1] = cmd->unk12;
        pspCache(0x1A, &out[0]);
        pspCache(0x1A, &out[1]);
        if ((pspCop0StateGet(24) & 1) != 0) {
            pspSync();
            *(int *)(0xA7F00000) =
                (((int)out | 0x08000000) & 0x0FFFFFC0) | 0xA0000001;
            (void)*(int *)(0xA7F00000);
        }
        // 5684
    }

    // 5694, 56C4
    for (i = 0; i < g_GeDeciBreak.size; i++) {
        SceGeBpCmd *cmd = &g_GeDeciBreak.cmds[7 + (g_GeDeciBreak.size - i)];
        int *out = (int *)(cmd->unk0 & 0xFFFFFFFC);
        out[0] = cmd->unk8;
        out[1] = cmd->unk12;
        pspCache(0x1A, &out[0]);
        pspCache(0x1A, &out[1]);
        if ((pspCop0StateGet(24) & 1) != 0) {
            pspSync();
            *(int *)(0xA7F00000) =
                (((int)out | 0x08000000) & 0x0FFFFFC0) | 0xA0000001;
            (void)*(int *)(0xA7F00000);
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
        return 0x80000023;
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
            return 0x80000023;
        }
        if (arg->size >= 16) {
            numStacks = arg->numStacks;
            if (numStacks >= 256) {
                // 5CBC
                pspSetK1(oldK1);
                return 0x80000104;
            }
            if (!pspK1DynBufOk(arg->stacks, numStacks * 32)) {
                pspSetK1(oldK1);
                return 0x80000023;
            }
        }
    }
    // (58DC)
    // 58E0
    if ((((int)list | (int)stall | (int)ctx | (int)stack) & 3) != 0) {
        // 5CAC
        pspSetK1(oldK1);
        return 0x80000103;
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
            return 0x80000021;
        }
        if (dl->ctxUpToDate && stack != NULL && dl->stack == stack && !oldVer) { // 5C44
            Kprintf("_sceGeListEnQueue(): can't enqueue duplicated stack(STACK=0x%08X)\n", stack);  // 0x65FC
            // 5C60
            sceKernelCpuResumeIntr(oldIntr);
            pspSetK1(oldK1);
            return 0x80000021;
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
        return 0x80000022;
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
        return 0x80000022;
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
    dl->unk28 = 0;
    dl->unk32 = 0;
    dl->unk36 = 0;
    dl->unk40 = 0;
    dl->unk44 = 0;
    dl->unk48 = 0;
    dl->stackOff = 0;
    if (head != 0) {
        // 5B8C
        if (g_AwQueue.active_first != NULL) {
            // 5BE0
            if (g_AwQueue.active_first->state != SCE_GE_DL_STATE_PAUSED) {
                // 5C0C
                sceKernelCpuResumeIntr(oldIntr);
                pspSetK1(oldK1);
                return 0x800001FE;
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
            g_GeLogHandler(0, dlId, 1, list, stall);
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
        HW(0xBD400100) = 0;
        HW_GE_DLIST = (int)UCACHED(list);
        HW_GE_STALLADDR = (int)UCACHED(stall);
        HW(0xBD400120) = 0;
        HW(0xBD400124) = 0;
        HW(0xBD400128) = 0;
        HW(0xBD400110) = 0;
        HW(0xBD400114) = 0;
        pspSync();
        HW(0xBD400100) = 1;
        pspSync();
        g_AwQueue.curRunning = dl;
        sceKernelClearEventFlag(g_AwQueue.drawingEvFlagId, 0xFFFFFFFD);
        if (g_GeLogHandler != NULL) {
            g_GeLogHandler(0, dlId, 0, list, stall);
            if (g_GeLogHandler != NULL)
                g_GeLogHandler(5, dlId, list, stall);
        }
    } else {
        dl->state = SCE_GE_DL_STATE_QUEUED;
        g_AwQueue.active_last->next = dl;
        dl->prev = g_AwQueue.active_last;
        g_AwQueue.active_last = dl;
        if (g_GeLogHandler != NULL) {
            // 5A6C
            g_GeLogHandler(0, dlId, 0, list, stall);
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

