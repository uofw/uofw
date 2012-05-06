/* Copyright (C) 2011, 2012 The uOFW team
   See the file COPYING for copying permission.
*/

#include "common.h"

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

PSP_SDK_VERSION(0x06060010);
PSP_MODULE_INFO("sceGE_Manager", 0x1007, 1, 11);
PSP_MODULE_BOOTSTART("_sceGeModuleStart");
PSP_MODULE_REBOOT_BEFORE("_sceGeModuleRebootBefore");
PSP_MODULE_REBOOT_PHASE("_sceGeModuleRebootPhase");

/******************************/

int _sceGeReset();
int _sceGeInitCallback3(int arg0, int arg1, int arg2);
int _sceGeInitCallback4();
int _sceGeSetRegRadr1(int arg0);
int _sceGeSetRegRadr2(int arg0);
int _sceGeSetInternalReg(int type, int arg1, int arg2, int arg3);
int _sceGeInterrupt(int arg0, int arg1, int arg2);
int _sceGeSysEventHandler(int ev_id, char* ev_name, void* param, int* result);
int _sceGeModuleStart();
int _sceGeModuleRebootPhase(int unk);
int _sceGeModuleRebootBefore();
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
int _sceGeListEnQueue(void *list, void *stall, int cbid, SceGeListArgs *arg, int head);

/****** Structures *********/

typedef struct
{
    SceGeDisplayList *curRunning;
    int isBreak;
    SceGeDisplayList *cur, *next;
    SceGeDisplayList *first; // 16
    SceGeDisplayList *last; // 20
    SceUID drawingEvFlagId; // 24
    SceUID listEvFlagIds[2]; // 28, 32
    void *stack; // 36
    /* TODO ... */
    int sdkVer; // 1060
    int patched; // 1064
    int syscallId;
    int *lazySyncData;
} SceGeQueue;

typedef struct
{
    int unk0, unk4, unk8;
    int *unk12;
    int unk16, unk20, unk24, unk28;
} SceGeQueueSuspendInfo;

typedef struct
{
    int unk0, unk4, unk8, unk12;
} SceGeBpCmd;

typedef struct
{
    int busy;
    int clear;
    int size;
    int size2;
    SceGeBpCmd cmds[10];
} SceGeBpCtrl;

typedef struct
{
    char *name;
    void *ptr;
} SadrUpdate;

typedef struct
{
    char reg1;
    char cmd;
    char reg2;
    char size;
} SceGeMatrix;

/********* Global values *********/

// 6640
SceIntrCb g_GeSubIntrFunc =
{
    0,    0,    NULL, NULL,
    NULL, NULL, NULL, NULL,
    NULL, NULL, NULL
};

// 666C
SceIntrHandler g_GeIntrOpt = {12, 0x00000020, &g_GeSubIntrFunc};

// 6678
SceKernelDeci2Ops g_Deci2Ops = {0x3C,
{
    (void*)sceGeGetReg,
    (void*)sceGeGetCmd,
    (void*)sceGeGetMtx,
    (void*)sceGeSetReg,
    (void*)sceGeSetCmd,
    (void*)sceGeSetMtx,
    (void*)sceGeGetListIdList,
    (void*)sceGeGetList,
    (void*)sceGeGetStack,
    (void*)sceGePutBreakpoint,
    (void*)sceGeGetBreakpoint,
    (void*)sceGeDebugBreak,
    (void*)sceGeDebugContinue,
    (void*)sceGeRegisterLogHandler
} };

// 66B4
char save_regs[] =
{
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
int g_pAwRegAdr[] =
{
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
SadrUpdate sadrupdate_bypass = {"ULJM05086", (void*)0x08992414};

// 675C
SceGeMatrix mtxtbl[] =
{
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
int stopCmd[] = {0x0F000000, 0x0C000000};

// 6840
SceSysEventHandler g_GeSysEv = {64, "SceGe", 0x00FFFF00, _sceGeSysEventHandler, 0, 0, NULL, {0, 0, 0, 0, 0, 0, 0, 0, 0}};

// 6880
char g_szTbp[] = "RTBP0";

// 6890
void (*g_logHandler)();

// 68C0
SceGeContext g_context;

// 70C0
int g_edramHwSize;

// 70C4
int g_edramSize;

// 70C8
u16 g_edramAddrTrans;

// 70CC
SceGeQueue g_queue;

// 7500
SceGeQueueSuspendInfo g_queueSuspendInfo;

// 7520
SceGeBpCtrl g_bpCtrl;

// 75D0
int g_cbBits;

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
    dbg_printf("Ran %s\n", __FUNCTION__);
    int oldIntr = sceKernelCpuSuspendIntr();
    sceSysregAwRegABusClockEnable();
    pspSync();
    *(int*)(0xBD500010) = 2;
    *(int*)(0xBD400000) = 1;
    // 0144
    while ((LW(0xBD400000) & 1) != 0)
        ;
    sceGeSaveContext(&g_context);
    g_context.ctx[16] = LW(0xBD400200);
    sceSysregAwResetEnable();
    sceSysregAwResetDisable();
    *(int*)(0xBD500010) = 0;
    *(int*)(0xBD400100) = 0;
    *(int*)(0xBD400108) = 0;
    *(int*)(0xBD40010C) = 0;
    *(int*)(0xBD400110) = 0;
    *(int*)(0xBD400114) = 0;
    *(int*)(0xBD400118) = 0;
    *(int*)(0xBD40011C) = 0;
    *(int*)(0xBD400120) = 0;
    *(int*)(0xBD400124) = 0;
    *(int*)(0xBD400128) = 0;
    *(int*)(0xBD400310) = LW(0xBD400304);
    *(int*)(0xBD40030C) = LW(0xBD400308);
    *(int*)(0xBD400308) = 7;
    *(int*)(0xBD400200) = g_context.ctx[16];
    sceSysregSetMasterPriv(64, 1);
    sceGeRestoreContext(&g_context);
    sceSysregSetMasterPriv(64, 0);
    sceSysregAwRegABusClockDisable();
    sceKernelCpuResumeIntr(oldIntr);
    return 0;
}

int sceGeInit()
{
    dbg_init(1, FB_NONE, FAT_HARDWARE);
    dbg_printf("Ran %s\n", __FUNCTION__);
    sceSysregAwResetDisable();
    sceSysregAwRegABusClockEnable();
    sceSysregAwRegBBusClockEnable();
    sceSysregAwEdramBusClockEnable();
    g_dlMask = (LW(0xBD400804) ^ LW(0xBD400810)
              ^ LW(0xBD400814) ^ LW(0xBD400818)
              ^ LW(0xBD4008EC)) | 0x80000000;
    dbg_printf("%d\n", __LINE__);
    sceGeEdramInit();
    *(int*)(0xBD400100) = 0;
    u32 *dlist = &g_context.ctx[17];
    *(int*)(0xBD400108) = 0;
    *(int*)(0xBD40010C) = 0;
    u32 *curDl = dlist;
    *(int*)(0xBD400110) = 0;
    dbg_printf("%d\n", __LINE__);
    *(int*)(0xBD400114) = 0;
    *(int*)(0xBD400118) = 0;
    *(int*)(0xBD40011C) = 0;
    *(int*)(0xBD400120) = 0;
    *(int*)(0xBD400124) = 0;
    *(int*)(0xBD400128) = 0;
    *(int*)(0xBD400310) = *(int*)(0xBD400304);
    *(int*)(0xBD40030C) = *(int*)(0xBD400308);
    *(int*)(0xBD400308) = 7;
    dbg_printf("%d\n", __LINE__);
    int i;
    for (i = 0; i < 256; i++)
    {
        if (((save_regs[(i + ((u32)(i >> 31) >> 29)) >> 3] >> (i & 7)) & 1) != 0) // list of chars
            *(curDl++) = i << 24;
        // 03A0
    }
    *(curDl++) = 0x2A000000;

    // 03BC
    for (i = 0; i < 96; i++)
        *(curDl++) = 0x2B000000;
    *(curDl++) = 0x3A000000;

    // 03E0
    for (i = 0; i < 12; i++)
        *(curDl++) = 0x3B000000;
    *(curDl++) = 0x3C000000;

    // 0404
    for (i = 0; i < 12; i++)
        *(curDl++) = 0x3D000000;
    *(curDl++) = 0x3E000000;

    // 0428
    for (i = 0; i < 16; i++)
        *(curDl++) = 0x3F000000;
    *(curDl++) = 0x40000000;
    
    // 044C
    for (i = 0; i < 12; i++)
        *(curDl++) = 0x41000000;
    *(curDl + 1) = 0x0C000000;
    *(curDl + 0) = 0x04000000;
    sceKernelDcacheWritebackInvalidateRange(dlist, 1980);
    dbg_printf("%d\n", __LINE__);
    *(int*)(0xBD40030C) = LW(0xBD400308);
    *(int*)(0xBD400108) = (int)UCACHED(dlist);
    *(int*)(0xBD40010C) = 0;
    dbg_printf("%d\n", __LINE__);
    sceSysregSetMasterPriv(64, 1);
    dbg_printf("%d\n", __LINE__);
    *(int*)(0xBD400100) = 1;

    // 04B8
    while ((LW(0xBD400100) & 1) != 0)
        ;
    dbg_printf("%d\n", __LINE__);
    sceSysregSetMasterPriv(64, 0);
    dbg_printf("%d\n", __LINE__);
    *(int*)(0xBD400100) = 0;
    *(int*)(0xBD400108) = 0;
    *(int*)(0xBD40010C) = 0;
    *(int*)(0xBD400310) = LW(0xBD400304);
    *(int*)(0xBD400308) = 7;
    dbg_printf("%d\n", __LINE__);
    sceKernelRegisterIntrHandler(25, 1, _sceGeInterrupt, 0, &g_GeIntrOpt);
    dbg_printf("%d\n", __LINE__);

    // 0534
    for (i = 0; i < 16; i++)
        sceKernelSetUserModeIntrHanlerAcceptable(25, i, 1);
    dbg_printf("%d\n", __LINE__);

    sceKernelEnableIntr(25);
    dbg_printf("%d\n", __LINE__);
    sceSysregAwRegABusClockDisable();
    dbg_printf("%d\n", __LINE__);
    sceKernelRegisterSysEventHandler(&g_GeSysEv);
    dbg_printf("%d\n", __LINE__);
    _sceGeQueueInit();
    dbg_printf("%d\n", __LINE__);
    void *str = (void*)sceKernelGetUsersystemLibWork();
    if (*(int*)(str + 4) == 0) {
        // 05EC
        sceKernelSetInitCallback(_sceGeInitCallback3, 3, 0);
    }
    else
        _sceGeInitCallback3(0, 0, 0);
    dbg_printf("%d\n", __LINE__);

    // 059C
    sceKernelSetInitCallback(_sceGeInitCallback4, 4, 0);
    void *ret = (void*)sceKernelDeci2pReferOperations();
    g_deci2p = ret;
    if (ret != NULL)
    {
        // 05D4
        int (*func)(SceKernelDeci2Ops*) = (void*)*(int*)(ret + 28);
        func(&g_Deci2Ops);
    }
    dbg_printf("%d\n", __LINE__);
    return 0;
}

int sceGeEnd()
{   
    dbg_printf("Ran %s\n", __FUNCTION__);
    _sceGeQueueEnd();
    sceKernelUnregisterSysEventHandler(&g_GeSysEv);
    sceKernelDisableIntr(25);
    sceKernelReleaseIntrHandler(25);
    sceSysregAwRegABusClockEnable();
    *(int*)(0xBD400100) = 0;
    // 0640
    while ((LW(0xBD400100) & 1) != 0)
        ;

    if (g_cmdList != NULL)
    {
        int *cmdOut = &g_cmdList[16];
        cmdOut[0] = 0x10000000 | (((((int)&g_cmdList[20]) >> 24) & 0xF) << 16); // BASE
        cmdOut[1] = 0x13000000; // OFFSET
        cmdOut[2] = 0x0A000000 | (((int)&g_cmdList[20]) & 0x00FFFFFF); // CALL
        cmdOut[3] = 0x0C000000; // END
        cmdOut[4] = 0x0A000000 | (((int)&g_cmdList[22]) & 0x00FFFFFF); // CALL
        cmdOut[5] = 0x0B000000; // RET
        cmdOut[6] = 0x0B000000; // RET
        *(int*)(0xBD400120) = 0;
        *(int*)(0xBD400108) = (int)UCACHED(cmdOut);
        *(int*)(0xBD40010C) = 0;
        pspSync();
        *(int*)(0xBD400100) = 1;
        // 06E0
        while ((LW(0xBD400100) & 1) != 0)
            ;
    }
    sceSysregAwRegABusClockDisable();
    return 0;
}

// 070C
int _sceGeInitCallback3(int arg0 __attribute__((unused)), int arg1 __attribute__((unused)), int arg2 __attribute__((unused)))
{
    dbg_printf("Ran %s\n", __FUNCTION__);
    void *str = (void*)sceKernelGetUsersystemLibWork();
    void *uncached = UUNCACHED(*(int*)(str + 4));
    if (*(int*)(str + 4) != 0)
    {
        *(int*)(uncached + 4) = 0x0C000000;
        *(int*)(uncached + 0) = 0x0F000000;
        g_cmdList = uncached;
        _sceGeQueueInitCallback();
    }
    return 0;
}

int _sceGeInitCallback4()
{
    dbg_printf("Ran %s\n", __FUNCTION__);
    void *str = sceKernelGetGameInfo();
    if (str != NULL)
    {
        int num = ((sceKernelQuerySystemCall(sceGeListUpdateStallAddr) << 6) & 0x03FFFFC0) | 0xC;
        int oldIntr = sceKernelCpuSuspendIntr();
        if (strcmp(str + 68, sadrupdate_bypass.name) == 0)
        {
            void *ptr = sadrupdate_bypass.ptr;
            if (*(int*)(ptr + 0) == 0x03E00008 && *(int*)(ptr + 4) == num)
            {
                // 0804
                *(int*)(ptr + 4) = 0;
                *(int*)(ptr + 0) = ((*(int*)(sceKernelGetUsersystemLibWork() + 8) >> 2) & 0x03FFFFFF) | 0x08000000;
                pspCache(0x1A, ptr + 0);
                pspCache(0x1A, ptr + 4);
                pspCache(0x08, ptr + 0);
                pspCache(0x08, ptr + 4);
            }
        }
        // 07DC
        sceKernelCpuResumeIntr(oldIntr);
    }
    return 0;
}

int sceGeGetReg(u32 regId)
{   
    dbg_printf("Ran %s\n", __FUNCTION__);
    if (regId >= 32)
        return 0x80000102;
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

int sceGeSetReg(u32 regId, u32 value)
{
    dbg_printf("Ran %s\n", __FUNCTION__);
    if (regId >= 32)
        return 0x80000102;
    if (regId >= 5 && regId < 14 && (value & 3) != 0)
        return 0x800001FE;

    // 092C
    int oldK1 = pspShiftK1();
    int oldIntr = sceKernelCpuSuspendIntr();
    int wasEnabled = sceSysregAwRegABusClockEnable();
    int ret;
    if ((*(int*)(0xBD400100) & 1) == 0)
    {
        if (regId == 7) {
            // 09EC
            _sceGeSetRegRadr1(value);
        }
        else if (regId == 8) {
            // 09DC
            _sceGeSetRegRadr2(value);
        }
        // 0974
        *(int*)g_pAwRegAdr[regId] = value;
        ret = 0;
    }
    else
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
    dbg_printf("Ran %s\n", __FUNCTION__);
    if (cmdOff >= 0xFF)
        return 0x80000102;
    int oldK1 = pspShiftK1();
    int oldIntr = sceKernelCpuSuspendIntr();
    int wasEnabled = sceSysregAwRegABusClockEnable();
    int *cmds = (int*)0xBD400800;
    int cmd = cmds[cmdOff];
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
    dbg_printf("Ran %s\n", __FUNCTION__);
    if (cmdOff >= 0xFF)
        return 0x80000102;
    int oldK1 = pspShiftK1();
    int oldIntr = sceKernelCpuSuspendIntr();
    int wasEnabled = sceSysregAwRegABusClockEnable();
    int ret = 0x80000021;
    int old100, old108;
    if ((*(int*)(0xBD400100) & 1) == 0)
    {
        old100 = *(int*)(0xBD400100);
        ret = 0;
        old108 = *(int*)(0xBD400108);
        if (cmdOff < 8 || cmdOff >= 11)
        {
            // 0EB0
            if (cmdOff == 11)
            {
                // 13E0
                if ((old100 & 0x200) == 0)
                {
                    // 1410
                    if ((old100 & 0x100) == 0)
                    {
                        // 0E48 dup
                        ret = 0x80000003;
                    }
                    else
                    {
                        old108 = *(int*)(0xBD400110);
                        // 1404 dup
                        *(int*)(0xBD400120) = *(int*)(0xBD400124);
                        old100 &= 0xFFFFFEFF;
                    }
                }
                else
                {
                    old108 = *(int*)(0xBD400114);
                    // 1404 dup
                    *(int*)(0xBD400120) = *(int*)(0xBD400128);
                    old100 = (old100 & 0xFFFFFDFF) | 0x100;
                }
                // 0BA4 dup
                cmd = *(int*)(0xBD400800);
                cmdOff = 0;
            }
            else if (cmdOff == 20)
            {
                // 13C8
                *(int*)(0xBD400120) = old108;
                cmdOff = 0;
                cmd = *(int*)(0xBD400800);
            }
            else if (cmdOff < 4 || cmdOff >= 7)
            {
                // 10E0
                if ((cmdOff == 247) && ((cmd >> 21) & 1) != 0)
                {
                    // 12E4
                    int count = (*(int*)(0xBD400B08) >> 16) & 7;
                    // 1328
                    int i = 0;
                    do
                    {
                        int flag = ((*(int*)(0xBD400AA0 + i * 4) & 0x00FF0000) << 8) | (*(int*)(0xBD400A80 + i * 4) & 0x00FFFFFF);
                        if (flag < 0 || (
                             (((flag >> 14) & 0x7FFF) != 4 || ((0x35 >> ((flag >> 29) & 7)) & 1) == 0) // 13B8
                          && (((flag >> 23) & 0x003F) != 8 || ((0x35 >> ((flag >> 29) & 7)) & 1) == 0) // 1380, 13A8
                          && (((0x00220202 >> ((flag >> 27) & 0x1F)) & 1) == 0))) // 1388
                            ret = 0x80000103; // 1390
                
                        // 1398
                        i++;
                    } while (i <= count);
                }
                else
                {
                    // 10EC
                    if (cmdOff == 196)
                    {
                        // 1240
                        int flag = ((*(int*)(0xBD400AC4) << 8) & 0xFF000000) | (*(int*)(0xBD400AC0) & 0x00FFFFFF);
                        if (flag < 0 || (
                            (((flag >> 14) & 0x7FFF) != 4 && (0x35 >> ((flag >> 29) & 7) & 1) == 0) // 12C4
                         && (((flag >> 23) & 0x003F) != 8 && (0x35 >> ((flag >> 29) & 7) & 1) == 0) // 127C, 12A4
                         && (((0x00220202 >> ((flag >> 27) & 0x1F)) & 1) == 0))) // 1288
                        {
                            // 0E68 dup
                            ret = 0x80000103;
                        }
                    }
                    else if (cmdOff == 234)
                    {
                        int flag1 = ((*(int*)(0xBD400ACC) & 0x00FF0000) << 8) | (*(int*)(0xBD400AC8) & 0x00FFFFFF);
                        int flag2 = ((*(int*)(0xBD400AD4) & 0x00FF0000) << 8) | (*(int*)(0xBD400AD0) & 0x00FFFFFF);
                        if (flag1 < 0 || (
                            (((flag1 >> 14) & 0x7FFF) != 4 || ((0x35 >> ((flag1 >> 29) & 7)) & 1) == 0) // 1220
                         && (((flag1 >> 23) & 0x003F) != 8 || ((0x35 >> ((flag1 >> 29) & 7)) & 1) == 0) // 1158, 1200
                         && (((0x00220202 >> ((flag1 >> 27) & 0x1F)) & 1) == 0)) // 1164
                         || flag2 < 0 || ( // 1178
                            (((flag2 >> 14) & 0x7FFF) != 4 || ((0x35 >> ((flag2 >> 29) & 7)) & 1) == 0) // 11E0
                         && (((flag2 >> 23) & 0x003F) != 8 || ((0x35 >> ((flag2 >> 29) & 7)) & 1) == 0) // 1194, 11C0
                         && (((0x00220202 >> ((flag2 >> 27) & 0x1F)) & 1) == 0))) // 11A0
                        {
                            // 11B8
                            ret = 0x80000103;
                        }
                    }
                }
            }
            else
            {
                int flag = *(int*)(0xBD400118);
                if (flag < 0 || (
                     (((flag >> 14) & 0x7FFF) != 4 || ((0x35 >> ((flag >> 29) & 7)) & 1) == 0) // 10C0
                  && (((flag >> 23) & 0x003F) == 8 || ((0x35 >> ((flag >> 29) & 7)) & 1) == 0) // 10A0
                  && (((0x00220202 >> ((flag >> 27) & 0x1F)) & 1) == 0))) // 0EFC
                {
                    ret = 0x80000103;
                }
                // 0F14
                if (((*(int*)(0xBD400848) >> 11) & 3) != 0)
                {
                    int flag = *(int*)(0xBD40011C);
                    if (flag < 0 || (
                        (((flag >> 14) & 0x7FFF) != 4 && ((0x35 >> ((flag >> 29) & 7)) & 1) == 0) // 1080
                     && (((flag >> 23) & 0x003F) != 8 && ((0x35 >> ((flag >> 29) & 7)) & 1) == 0) // 0F4C, 1060
                     && (((0x00220202 >> ((flag >> 27) & 0x1F)) & 1) == 0))) // 0F58
                        ret = 0x80000103;
                }
        
                // 0F70
                if ((*(int*)(0xBD400878) & 1) != 0)
                {
                    int count = (*(int*)(0xBD400B08) >> 16) & 7;
                    // 0FC0
                    int i = 0;
                    do
                    {
                        int flag = ((*(int*)(0xBD400AA0 + i * 4) & 0x00FF0000) << 8) | (*(int*)(0xBD400A80 + i * 4) & 0x00FFFFFF);
                        if (flag < 0 || (
                            (((flag >> 14) & 0x7FFF) != 4 || ((0x35 >> ((flag >> 29) & 7)) & 1) == 0) // 1050
                         && (((flag >> 23) & 0x003F) != 8 || ((0x35 >> ((flag >> 29) & 7)) & 1) == 0) // 1018, 1040
                         && (((0x00220202 >> ((flag >> 27) & 0x1F)) & 1)) == 0)) // 1020
                        {
                            // 1028
                            ret = 0x80000103;
                        }
                        i++;
                        // 1030
                    } while (i <= count);
                }
            }
        }
        else
        {
            int flag = (((*(int*)(0xBD400840) << 8) & 0xFF000000) | (cmd & 0x00FFFFFF)) + *(int*)(0xBD400120);
            if (flag < 0 || (
                (((flag >> 14) & 0x7FFF) != 4 || ((0x35 >> ((flag >> 29) & 7)) & 1) == 0) // 0E90
             && (((flag >> 23) & 0x003F) != 8 || ((0x35 >> ((flag >> 29) & 7)) & 1) == 0) // 0B60, 0E70
             && (((0x00220202 >> ((flag >> 27) & 0x1F)) & 1) == 0))) // 0B6C
            {
                // 0E68 dup
                ret = 0x80000103;
            }
            else
            {
                // 0B88
                if (cmdOff == 9)
                {
                    // 0E50
                    if ((old100 & 2) == 0)
                        old108 = flag;
                    else {
                        old108 += 4;
                        old100 &= 0xFFFFFFFD;
                    }
                }
                else if (cmdOff >= 10)
                {
                    // 0DE0
                    if (cmdOff == 10)
                    {
                        if ((old100 & 0x200) != 0)
                        {
                            // 0E48 dup
                            ret = 0x80000003;
                        }
                        else if ((old100 & 0x100) == 0)
                        {
                            // 0E24
                            _sceGeSetRegRadr1(old108 + 4);
                            old108 = flag;
                            old100 |= 0x100;
                            *(int*)(0xBD400124) = *(int*)(0xBD400120);
                        }
                        else
                        {
                            _sceGeSetRegRadr2(old108 + 4);
                            old100 = (old100 & 0xFFFFFEFF) | 0x200;
                            old108 = flag;
                            *(int*)(0xBD400128) = *(int*)(0xBD400120);
                        }
                    }
                }
                else if (cmdOff == 8)
                    old108 = flag;
                // 0BA4 dup
                cmd = *(int*)(0xBD400800);
                cmdOff = 0;
            }
        }
    }
    // 0BB0
    if (ret == 0)
    {
        int buf[32];
        int sp128, sp132;
        // 0BB8
        sp128 = *(int*)(0xBD40010C);
        int *ptr = (int*)(((int)buf | 0x3F) + 1);
        sp132 = *(int*)(0xBD400304);
        if (cmdOff == 15)
        {
            // 0DC0
            ptr[0] = (cmd & 0x00FFFFFF) | 0x0F000000;
            ptr[1] = *(int*)(0xBD400830);
        }
        else if (cmdOff == 12)
        {
            // 0DA0
            ptr[0] = *(int*)(0xBD40083C);
            ptr[1] = 0x0C000000 | (cmd & 0x00FFFFFF);
        }
        else if (cmdOff == 16)
        {
            // 0D78
            ptr[0] = (cmd & 0x00FFFFFF) | 0x10000000; // BASE
            ptr[1] = *(int*)(0xBD40083C);
            ptr[2] = *(int*)(0xBD400830);
        }
        else
        {
            ptr[0] = 0x10400000 | (*(int*)(0xBD400840) & 0x00FF0000); // BASE
            ptr[1] = (cmdOff << 24) | (cmd & 0x00FFFFFF);
            ptr[2] = *(int*)(0xBD400840);
            ptr[3] = *(int*)(0xBD40083C);
            ptr[4] = *(int*)(0xBD400830);
        }
        // 0C44
        pspCache(0x1A, ptr);
        if ((pspCop0StateGet(24) & 1) != 0)
        {
            pspSync();
            *(int*)(0xA7F00000) = ((int)ptr & 0x07FFFFC0) | 0xA0000001;
            (void)*(int*)(0xA7F00000);
        }
        // C88
        sceSysregSetMasterPriv(64, 1);
        *(int*)(0xBD400310) = 4;
        *(int*)(0xBD400108) = (int)UCACHED(ptr);
        *(int*)(0xBD40010C) = 0;
        pspSync();
        *(int*)(0xBD400100) = old100 | 1;
        // 0CC0
        while ((LW(0xBD400100) & 1) != 0)
            ;
        // 0CD4
        while ((LW(0xBD400304) & 4) == 0)
            ;
        *(int*)(0xBD400108) = old108;
        *(int*)(0xBD40010C) = sp128;
        *(int*)(0xBD400310) = *(int*)(0xBD400304) ^ sp132;
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
    dbg_printf("Ran %s\n", __FUNCTION__);
    if (id < 0 || id >= 12)
        return 0x80000102;
    int oldK1 = pspShiftK1();
    if (!pspK1PtrOk(mtx))
    {
        // 1588
        pspSetK1(oldK1);
        return 0x80000023;
    }
    int oldIntr = sceKernelCpuSuspendIntr();
    int wasEnabled = sceSysregAwRegABusClockEnable();
    if (id >= 10)
    {
        // 1510
        if (id == 10)
        {
            // 1554
            // 1560
            int i;
            for (i = 0; i < 16; i++)
                mtx[i] = *(int*)(0xBD400DE0 + i * 4);
        }
        else if (id == 11)
        {
            // 152C
            int i;
            for (i = 0; i < 12; i++)
                mtx[i] = *(int*)(0xBD400E20 + i * 4);
        }
    }
    else
    {
        int *out = (int*)(0xBD400C00 + id * 48);
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
    dbg_printf("Ran %s\n", __FUNCTION__);
    int ret;
    if (id < 0 || id >= 12)
        return 0x80000102;
    int oldK1 = pspShiftK1();
    if (!pspK1PtrOk(mtx))
    {
        // 16B8
        pspSetK1(oldK1);
        return 0x80000023;
    }
    int oldIntr = sceKernelCpuSuspendIntr();
    SceGeMatrix *curMtx = &mtxtbl[id];
    char oldCmd = sceGeGetCmd(curMtx->reg1);
    ret = sceGeSetCmd(curMtx->reg1, curMtx->cmd);
    if (ret >= 0)
    {
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

int sceGeSaveContext(SceGeContext *ctx)
{   
    dbg_printf("Ran %s\n", __FUNCTION__);
    if (((int)ctx & 3) != 0)
        return 0x80000103;
    int oldK1 = pspShiftK1();
    if (!pspK1StaBufOk(ctx, 2048))
    {
        // 1AA0
        pspSetK1(oldK1);
        return 0x80000023;
    }
    int oldIntr = sceKernelCpuSuspendIntr();
    int aBusWasEnabled = sceSysregAwRegABusClockEnable();
    if ((*(int*)(0xBD400100) & 1) != 0)
    {
        // 1A8C
        sceKernelCpuResumeIntr(oldIntr);
        pspSetK1(oldK1);
        return -1;
    }
    u32 *curCmd = &ctx->ctx[17];
    ctx->ctx[0] = *(int*)(0xBD400100);
    ctx->ctx[1] = *(int*)(0xBD400108);
    ctx->ctx[2] = *(int*)(0xBD40010C);
    ctx->ctx[3] = *(int*)(0xBD400110);
    ctx->ctx[4] = *(int*)(0xBD400114);
    ctx->ctx[5] = *(int*)(0xBD400118);
    ctx->ctx[6] = *(int*)(0xBD40011C);
    ctx->ctx[7] = *(int*)(0xBD400120);
    ctx->ctx[8] = *(int*)(0xBD400124);
    ctx->ctx[9] = *(int*)(0xBD400128);
    
    int *cmds = (int*)0xBD400800;
    // 17C8
    int i;
    for (i = 0; i < 256; i++)
    {
        if (((save_regs[(i + ((u32)(i >> 31) >> 29)) >> 3] >> (i & 7)) & 1) != 0)
            *(curCmd++) = *cmds;
        // 1804
        cmds++;
    }
    int flag = (*(int*)(0xBD400AC0) & 0x00FFFFFF) | ((*(int*)(0xBD400AC4) << 8) & 0xFF000000);
    if (flag >= 0
     && ((((flag >> 14) & 0x7FFF) == 4 && ((0x35 >> ((flag >> 29) & 7)) & 1) != 0) // 1A6C
      || (((flag >> 23) & 0x003F) == 8 && ((0x35 >> ((flag >> 29) & 7)) & 1) != 0) // 1A4C
      || (((0x00220202 >> ((flag >> 27) & 0x1F)) & 1) == 0))) // 1854
    {
        // 1868
        *(curCmd++) = *(int*)(0xBD400B10);
    }

    // 187C
    *(curCmd++) = 0x2A000000; // BOFS
    int *bones = (int*)0xBD400C00;
    // 1894
    for (i = 0; i < 96; i++)
        *(curCmd++) = 0x2B000000 | (*(bones++) & 0x00FFFFFF); // BONE

    *(curCmd++) = 0x3A000000; // WMS
    int *worlds = (int*)0xBD400D80;
    for (i = 0; i < 12; i++)
        *(curCmd++) = 0x3B000000 | (*(worlds++) & 0x00FFFFFF); // WORLD

    *(curCmd++) = 0x3C000000; // VMS
    int *views = (int*)0xBD400DB0;
    // 190C
    for (i = 0; i < 12; i++)
        *(curCmd++) = 0x3D000000 | (*(views++) & 0x00FFFFFF); // VIEW

    *(curCmd++) = 0x3E000000; // PMS
    int *projs = (int*)0xBD400DE0;
    // 1948
    for (i = 0; i < 16; i++)
        *(curCmd++) = 0x3F000000 | (*(projs++) & 0x00FFFFFF); // PROJ

    *(curCmd++) = 0x40000000; // TMS
    int *tmtxs = (int*)0xBD400E20;
    // 1984
    for (i = 0; i < 12; i++)
        *(curCmd++) = 0x41000000 | (*(tmtxs++) & 0x00FFFFFF); // TMATRIX

    *(curCmd++) = *(int*)(0xBD4008A8);
    *(curCmd++) = *(int*)(0xBD4008E8);
    *(curCmd++) = *(int*)(0xBD4008F0);
    *(curCmd++) = *(int*)(0xBD4008F8);
    *(curCmd++) = *(int*)(0xBD400900);
    *(curCmd++) = 0x0C000000; // END
    sceKernelDcacheWritebackInvalidateRange(&ctx->ctx[17], 1980);
    if (!aBusWasEnabled)
        sceSysregAwRegABusClockDisable(); // 1A3C
    // 1A0C
    sceKernelCpuResumeIntr(oldIntr);
    pspSetK1(oldK1);
    return 0;
}

int sceGeRestoreContext(SceGeContext *ctx)
{
    dbg_printf("Ran %s\n", __FUNCTION__);
    int ret = 0;
    if (((int)ctx & 3) != 0)
        return 0x80000103;
    int oldK1 = pspShiftK1();
    if (!pspK1StaBufOk(ctx, 2048))
    {
        // 1C80
        pspSetK1(oldK1);
        return 0x80000023;
    }
    int oldIntr = sceKernelCpuSuspendIntr();
    int state = sceSysregAwRegABusClockEnable();
    if ((*(int*)(0xBD400100) & 1) != 0)
    {
        // 1C68
        sceKernelCpuResumeIntr(oldIntr);
        pspSetK1(oldK1);
        return 0x80000021;
    }
    int old304 = *(int*)(0xBD400304);
    int old308 = *(int*)(0xBD400308);
    *(int*)(0xBD40030C) = old308;
    *(int*)(0xBD400108) = (int)UCACHED(&ctx->ctx[17]);
    *(int*)(0xBD40010C) = 0;
    *(int*)(0xBD400100) = ctx->ctx[0] | 1;
    // 1B64
    while ((LW(0xBD400100) & 1) != 0)
        ;
    int n304 = *(int*)(0xBD400304);
    *(int*)(0xBD400310) = (old304 ^ *(int*)(0xBD400304)) & 0xFFFFFFFA;
    *(int*)(0xBD400308) = old308;
    if ((n304 & 8) != 0)
        ret = -1;
    *(int*)(0xBD400108) = ctx->ctx[1];
    *(int*)(0xBD40010C) = ctx->ctx[2];
    *(int*)(0xBD400118) = ctx->ctx[5];
    *(int*)(0xBD40011C) = ctx->ctx[6];
    *(int*)(0xBD400120) = ctx->ctx[7];
    *(int*)(0xBD400124) = ctx->ctx[8];
    *(int*)(0xBD400128) = ctx->ctx[9];
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
    dbg_printf("Ran %s\n", __FUNCTION__);
    return _sceGeSetInternalReg(2, 0, arg0, 0);
}

int _sceGeSetRegRadr2(int arg0)
{
    dbg_printf("Ran %s\n", __FUNCTION__);
    return _sceGeSetInternalReg(4, 0, 0, arg0);
}

int _sceGeSetInternalReg(int type, int arg1, int arg2, int arg3)
{
    dbg_printf("Ran %s\n", __FUNCTION__);
    int *cmdList = g_cmdList;
    if (cmdList == NULL)
        return 0;
    int oldIntr = sceKernelCpuSuspendIntr();
    int old304 = *(int*)(0xBD400304);
    int old100 = *(int*)(0xBD400100);
    int old108 = *(int*)(0xBD400108);
    int old10C = *(int*)(0xBD40010C);
    int old120 = *(int*)(0xBD400120);
    int old124 = *(int*)(0xBD400124);
    int old128 = *(int*)(0xBD400128);
    if ((type & 1) == 0)
        arg1 = *(int*)(0xBD400840);
    // 1D74
    cmdList[0] = 0x0F000000;
    cmdList[1] = 0x0C000000;
    cmdList += 2;
    cmdList[0] = 0x0C000000;
    pspSync();
    *(int*)(0xBD40010C) = 0;
    int *uncachedCmdList = UCACHED(cmdList);
    if (((type >> 1) & 1) && (arg2 != 0))
    {
        int *uncachedNewCmdList = UCACHED(arg2 - 4);
        u32 cmd = (u32)(uncachedCmdList - old120);
        int oldCmd = uncachedNewCmdList[0];
        uncachedNewCmdList[0] = (cmd & 0x00FFFFFF) | 0x0A000000;
        pspCache(0x1A, uncachedNewCmdList);
        if ((pspCop0StateGet(24) & 1) != 0)
        {
            pspSync();
            *(int*)(0xA7F00000) = (((int)uncachedNewCmdList | 0x08000000) & 0x0FFFFFC0) | 0xA0000000;
            (void)*(int*)(0xA7F00000);
        }
        // 1E18
        cmdList[1] = ((cmd >> 24) << 16) | 0x10000000;
        cmdList[2] = 0x0C000000;
        pspSync();
        *(int*)(0xBD400108) = (int)UCACHED(cmdList + 4);
        *(int*)(0xBD400100) = 1;
        // 1E4C
        while ((LW(0xBD400100) & 1) != 0)
            ;
        *(int*)(0xBD400108) = (int)UCACHED(uncachedNewCmdList);
        *(int*)(0xBD400100) = 1;
        // 1E74
        while ((LW(0xBD400100) & 1) != 0)
            ;
        uncachedNewCmdList[0] = oldCmd;
        if ((pspCop0StateGet(24) & 1) != 0)
        {
            pspSync();
            *(int*)(0xA7F00000) = (((int)uncachedNewCmdList | 0x08000000) & 0x0FFFFFC0) | 0xA0000000;
            (void)*(int*)(0xA7F00000);
        }
    }
    // 1ED0
    // 1ED4
    if (((type >> 2) & 1) && (arg3 != 0))
    {
        int *uncachedNewCmdList = UCACHED(arg3 - 4);
        int *cmd = uncachedCmdList - old120;
        int oldCmd = uncachedNewCmdList[0];
        uncachedNewCmdList[0] = ((u32)cmd & 0x00FFFFFF) | 0x0A000000;
        pspCache(0x1A, uncachedNewCmdList);
        if ((pspCop0StateGet(24) & 1) != 0)
        {
            pspSync();
            *(int*)(0xA7F00000) = (((int)uncachedNewCmdList | 0x80000000) & 0x0FFFFFC0) | 0xA0000000;
            (void)*(int*)(0xA7F00000);
        }
        // 1F50
        cmdList[1] = 0x10000000 | (((u32)cmd >> 24) << 16);
        cmdList[2] = 0x0C000000;
        pspSync();
        *(int*)(0xBD400108) = (int)UCACHED(cmdList + 1);
        *(int*)(0xBD400100) = 1;
    
        // 1F88
        while ((LW(0xBD400100) & 1) != 0)
            ;
        *(int*)(0xBD400108) = (int)UCACHED(uncachedNewCmdList);
        *(int*)(0xBD400100) = 0x101;
        
        // 1FB0
        while ((LW(0xBD400100) & 1) != 0)
            ;
        uncachedNewCmdList[0] = oldCmd;
        pspCache(0x1A, uncachedNewCmdList);
        if ((pspCop0StateGet(24) & 1) != 0)
        {
            pspSync();
            *(int*)(0xA7F00000) = (((int)uncachedNewCmdList | 0x08000000) & 0x0FFFFFC0) | 0xA0000000;
            (void)*(int*)(0xA7F00000);
        }
    }
    cmdList[0] = arg1;
    // 2010
    cmdList[1] = *(int*)(0xBD400830);
    *(int*)(0xBD400108) = (int)uncachedCmdList;
    pspSync();
    *(int*)(0xBD400100) = old100 | 1;
    
    // 2034
    while ((LW(0xBD400100) & 1) != 0)
        ;
    *(int*)(0xBD400108) = old108;
    *(int*)(0xBD40010C) = old10C;
    *(int*)(0xBD400120) = old120;
    *(int*)(0xBD400124) = old124;
    *(int*)(0xBD400128) = old128;

    if ((type & 2) != 0)
        *(int*)(0xBD400110) = arg2;
    // 2084
    if ((type & 4) != 0)
        *(int*)(0xBD400114) = arg3;

    // 2094
    *(int*)(0xBD400310) = *(int*)(0xBD400304) ^ old304;
    sceKernelCpuResumeIntr(oldIntr);
    return 0;
}

int _sceGeInterrupt(int arg0 __attribute__((unused)), int arg1 __attribute__((unused)), int arg2 __attribute__((unused)))
{
    dbg_printf("Ran %s\n", __FUNCTION__);
    int oldIntr = sceKernelCpuSuspendIntr();
    int attr = LW(0xBD400304);
    int unk1 = LW(0xBD400004);
    if ((attr & 8) != 0)
    {
        // 2228
        *(int*)(0xBD400310) = 8;
        _sceGeErrorInterrupt(attr, unk1, arg2);
    }
    // 2118
    if ((attr & 5) == 5) {
        // 2218
        Kprintf("GE INTSIG/INTFIN at the same time\n"); // 0x6324
    }

    // 2128
    if ((attr & 4) == 0)
    {
        // 21AC
        if ((attr & 1) == 0 && (attr & 2) != 0) { // 2208
            // 21FC dup
            *(int*)(0xBD40030C) = 2;
        }
        else if ((attr & 1) != 0 && (attr & 2) == 0) {
            // 21FC dup
            *(int*)(0xBD40030C) = 1;
        }
        else
        {
            // 21C0
            while ((LW(0xBD400100) & 1) != 0)
                ;
            *(int*)(0xBD400310) = 3;
            *(int*)(0xBD400308) = 3;
            _sceGeListInterrupt(attr, unk1, arg2);
        }
    }
    else
    {
        if ((attr & 2) == 0) {
            // 2198
            Kprintf("CMD_FINISH must be used with CMD_END.\n"); // 0x6348
            *(int*)(0xBD400100) = 0;
        }

        // 213C
        while ((LW(0xBD400100) & 1) != 0)
            ;
        *(int*)(0xBD400310) = 6;
        *(int*)(0xBD400308) = 6;
        _sceGeFinishInterrupt(attr, unk1, arg2);
    }

    // 2170
    sceKernelCpuResumeIntrWithSync(oldIntr);
    return -1;
}

int _sceGeSysEventHandler(int ev_id, char* ev_name __attribute__((unused)), void* param, int* result __attribute__((unused)))
{
    dbg_printf("Ran %s\n", __FUNCTION__);
    switch (ev_id)
    {
    case 0x4005:
        // 2420
        sceSysregAwRegABusClockEnable();
        _sceGeQueueSuspend();
        sceGeSaveContext(&g_context);
        g_context.ctx[10] = *(int*)(0xBD500070);
        g_context.ctx[11] = *(int*)(0xBD500080);
        g_context.ctx[12] = *(int*)(0xBD500000);
        g_context.ctx[13] = *(int*)(0xBD500020);
        g_context.ctx[14] = *(int*)(0xBD500030);
        g_context.ctx[15] = *(int*)(0xBD500040);
        g_context.ctx[16] = *(int*)(0xBD400200);
        break;

    case 0x10005:
        // 22C4
        sceSysregAwResetDisable();
        sceSysregAwRegABusClockEnable();
        sceSysregAwRegBBusClockEnable();
        sceSysregAwEdramBusClockEnable();
        sceGeEdramInit();
        _sceGeEdramResume();
        *(int*)(0xBD400100) = 0;
        *(int*)(0xBD400108) = 0;
        *(int*)(0xBD40010C) = 0;
        *(int*)(0xBD400110) = 0;
        *(int*)(0xBD400114) = 0;
        *(int*)(0xBD400118) = 0;
        *(int*)(0xBD40011C) = 0;
        *(int*)(0xBD400120) = 0;
        *(int*)(0xBD400124) = 0;
        *(int*)(0xBD400128) = 0;
        *(int*)(0xBD400310) = *(int*)(0xBD400304);
        *(int*)(0xBD40030C) = *(int*)(0xBD400308);
        *(int*)(0xBD400308) = 7;
        *(int*)(0xBD500070) = g_context.ctx[10];
        *(int*)(0xBD500080) = g_context.ctx[11];
        *(int*)(0xBD500000) = g_context.ctx[12];
        *(int*)(0xBD500020) = g_context.ctx[13];
        *(int*)(0xBD500030) = g_context.ctx[14];
        *(int*)(0xBD500040) = g_context.ctx[15];
        *(int*)(0xBD400200) = g_context.ctx[16];
        sceSysregSetMasterPriv(64, 1);
        sceGeRestoreContext(&g_context);
        sceSysregSetMasterPriv(64, 0);
        _sceGeQueueResume();
        if (_sceGeQueueStatus() == 0)
            sceSysregAwRegABusClockDisable();
        break;

    case 0x4003:
        // 228C
        _sceGeEdramSuspend();
        if (*(int*)(param + 4) != 2)
        {
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
    dbg_printf("Ran %s\n", __FUNCTION__);
    sceGeInit();
    return 0;
}

int _sceGeModuleRebootPhase(int unk)
{   
    dbg_printf("Ran %s\n", __FUNCTION__);
    if (unk == 1)
        sceGeBreak(0, NULL); // 24E4
    return 0;
}

int _sceGeModuleRebootBefore()
{
    dbg_printf("Ran %s\n", __FUNCTION__);
    sceGeEnd();
    return 0;
}

int sceGeRegisterLogHandler(void (*handler)())
{
    dbg_printf("Ran %s\n", __FUNCTION__);
    g_logHandler = handler;
    return 0;
}

int sceGeSetGeometryClock(int opt)
{
    dbg_printf("Ran %s\n", __FUNCTION__);
    int old = *(int*)(0xBD400200);
    *(int*)(0xBD400200) = opt & 1;
    return old & 1;
}

int _sceGeSetBaseRadr(int arg0, int arg1, int arg2)
{
    dbg_printf("Ran %s\n", __FUNCTION__);
    return _sceGeSetInternalReg(7, arg0, arg1, arg2);
}

int _sceGeEdramResume()
{
    dbg_printf("Ran %s\n", __FUNCTION__);
    sceGeEdramSetAddrTranslation(g_edramAddrTrans);
    if (g_edramHwSize == 0x00400000)
    {
        // 261C
        *(int*)(0xBD400400) = 2;
        sceSysregSetAwEdramSize(1);
    }
    // 25A0
    memcpy(UCACHED(sceGeEdramGetAddr()), UCACHED(sceKernelGetAWeDramSaveAddr()), g_edramHwSize);
    sceKernelDcacheWritebackInvalidateAll();
    if (g_edramHwSize == 0x00400000 && g_edramSize == 0x00200000) { // 25F4
        *(int*)(0xBD400400) = 4;
        sceSysregSetAwEdramSize(0);
    }
    return 0;
}

int sceGeEdramInit()
{   
    dbg_printf("Ran %s\n", __FUNCTION__);
    int i = 83;
    // 264C
    while ((i--) != 0)
        ;
    *(int*)(0xBD500010) = 1;
    // 2660
    while ((LW(0xBD500010) & 1) != 0)
        ;
    *(int*)(0xBD500020) = 0x6C4;
    *(int*)(0xBD500040) = 1;
    *(int*)(0xBD500090) = 3;
    *(int*)(0xBD400400) = 4;
    if (g_edramHwSize != 0)
        return 0;
    if ((*(int*)(0xBD500070) & 1) == 0)
    {
        // 2758
        g_edramAddrTrans = *(int*)(0xBD500080) << 1;
    }
    else
        g_edramAddrTrans = 0;

    // 26C8
    if (sceSysregGetTachyonVersion() > 0x004FFFFF)
    {
        // 271C
        g_edramSize = 0x00200000;
        g_edramHwSize = 0x00400000;
        if (sceSysregSetAwEdramSize(0) != 0)
        {
            *(int*)(0xBD400400) = 2;
            sceSysregSetAwEdramSize(1);
            g_edramSize = 0x00400000;
        }
        return 0;
    }
    int num = (*(int*)(0xBD400008) & 0xFFFF) << 10;
    g_edramSize = num;
    g_edramHwSize = num;
    return 0;
}

int sceGeEdramSetRefreshParam(int arg0, int arg1, int arg2, int arg3)
{
    dbg_init(0, FB_NONE, FAT_AFTER_SYSCON);
    dbg_printf("Ran %s\n", __FUNCTION__);
    int ret = 0;
    int oldIntr = sceKernelCpuSuspendIntr();
    int old44 = LW(0xBC000044);
    dbg_printf("%d\n", __LINE__);
    *(int*)(0xBC000044) &= 0xFF9BFFFF;
    if (arg0 != 0)
    {
        // 2858
        if (arg0 == 1)
        {
    dbg_printf("%d\n", __LINE__);
            *(int*)(0xBD500000) = ((arg3 & 0xF) << 20) | (LW(0xBD500000) & 0xFF0FFFFF);
            *(int*)(0xBD500030) = arg2 & 0x3FF;
            *(int*)(0xBD500020) = arg1 & 0x007FFFFF;
    dbg_printf("%d\n", __LINE__);
            if ((LW(0xBD500040) & 2) == 0)
            {
    dbg_printf("%d\n", __LINE__);
                // 284C
                *(int*)(0xBD500040) = 3;
    dbg_printf("%d\n", __LINE__);
            }
        }
        else
            ret = -1;
    }
    else
    {
    dbg_printf("%d\n", __LINE__);
        *(int*)(0xBD500000) = ((arg3 & 0xF) << 20) | (LW(0xBD500000) & 0xFF0FFFFF);
        *(int*)(0xBD500030) = arg2 & 0x3FF;
        *(int*)(0xBD500020) = arg1 & 0x007FFFFF;
    dbg_printf("%d\n", __LINE__);
        if ((LW(0xBD500040) & 2) != 0) {
    dbg_printf("%d\n", __LINE__);
            // 284C dup
            *(int*)(0xBD500040) = 1;
    dbg_printf("%d\n", __LINE__);
        }
    dbg_printf("%d\n", __LINE__);
    }

    dbg_printf("%d\n", __LINE__);
    // 281C
    *(int*)(0xBC000044) = old44;
    dbg_printf("%d\n", __LINE__);
    sceKernelCpuResumeIntrWithSync(oldIntr);
    dbg_printf("%d\n", __LINE__);
    dbg_init(0, FB_NONE, FAT_AFTER_FATMS);
    dbg_printf("after fatms?\n");
    return ret;
}

int sceGeEdramSetSize(int size)
{
    dbg_printf("Ran %s\n", __FUNCTION__);
    if (size == 0x200000)
    {
        // 2944
        g_edramSize = size;
        sceGeSetReg(19, 4);
        // 2934 dup
        sceSysregSetAwEdramSize(0);
    }
    else if (size == 0x400000)
    {
        // 28FC
        if (sceSysregGetTachyonVersion() <= 0x4FFFFF)
            return 0x80000104;
        g_edramSize = size;
        sceGeSetReg(19, 2);
        // 2934 dup
        sceSysregSetAwEdramSize(1);
    }
    else
        return 0x80000104;

    return 0;
}

int sceGeEdramGetAddr()
{
    dbg_printf("Ran %s\n", __FUNCTION__);
    return 0x04000000;
}

int sceGeEdramSetAddrTranslation(int arg)
{
    dbg_printf("Ran %s\n", __FUNCTION__);
    int ret;
    if (arg != 0 && arg != 0x200 && arg != 0x400 
      && arg != 0x800 && arg != 0x1000)
        return 0x800001FE;

    // 29AC
    int oldIntr = sceKernelCpuSuspendIntr();
    g_edramAddrTrans = arg;
    if (arg == 0)
    {
        // 2A28
        if ((*(int*)(0xBD500070) & 1) == 0) {
            ret = *(int*)(0xBD500080) << 1;
            *(int*)(0xBD500070) = 1;
        }
        else
            ret = 0;
    }
    else if ((*(int*)(0xBD500070) & 1) == 0)
    {
        // 2A0C
        ret = *(int*)(0xBD500080) << 1;
        *(int*)(0xBD500080) = arg >> 1;
    }
    else
    {
        *(int*)(0xBD500080) = arg >> 1;
        *(int*)(0xBD500070) = 0;
        ret = 0;
    }
    sceKernelCpuResumeIntrWithSync(oldIntr);
    return ret;
}

int _sceGeEdramSuspend()
{
    dbg_printf("Ran %s\n", __FUNCTION__);
    sceKernelDcacheWritebackInvalidateAll();
    if (g_edramHwSize == 0x00400000)
    {
        // 2ABC
        *(int*)(0xBD400400) = 2;
        sceSysregSetAwEdramSize(1);
    }

    // 2A80
    memcpy(UCACHED(sceKernelGetAWeDramSaveAddr()), UCACHED(sceGeEdramGetAddr()), g_edramHwSize);
    return 0;
}

int sceGeEdramGetSize()
{
    dbg_printf("Ran %s\n", __FUNCTION__);
    return g_edramSize;
}

int sceGeEdramGetHwSize()
{
    dbg_printf("Ran %s\n", __FUNCTION__);
    return g_edramHwSize;
}

int _sceGeQueueInit()
{
    dbg_printf("Ran %s\n", __FUNCTION__);
    // 2B10
    int i;
    for (i = 0; i < 64; i++)
    {
        SceGeDisplayList *cur = &g_displayLists[i];
        cur->next = &g_displayLists[i + 1];
        cur->prev = &g_displayLists[i - 1];
        cur->signal = SCE_GE_DL_SIGNAL_NONE;
        cur->state = SCE_GE_DL_STATE_NONE;
        cur->ctxUpToDate = 0;
    }
    g_queue.curRunning = NULL;
    g_queue.last = &g_displayLists[63];
    g_displayLists[0].prev = NULL;
    g_displayLists[63].next = NULL;
    g_queue.first = g_displayLists;
    g_queue.cur = NULL;
    g_queue.next = NULL;
    g_queue.drawingEvFlagId = sceKernelCreateEventFlag("SceGeQueueId", 0x201,  2, NULL);
    g_queue.listEvFlagIds[0] = sceKernelCreateEventFlag("SceGeQueueId", 0x201, -1, NULL);
    g_queue.listEvFlagIds[1] = sceKernelCreateEventFlag("SceGeQueueId", 0x201, -1, NULL);
    g_queue.syscallId = sceKernelQuerySystemCall(sceGeListUpdateStallAddr);
    void *gameInfo = sceKernelGetGameInfo();
    g_queue.patched = 0;
    if (gameInfo != NULL && strcmp(gameInfo + 68, "ULJM05127") == 0)
        g_queue.patched = 1;
    return 0;
}

int _sceGeQueueSuspend()
{
    dbg_printf("Ran %s\n", __FUNCTION__);
    if (g_queue.cur == NULL)
        return 0;

    // 2C5C
    while ((LW(0xBD400100) & 1) != 0)
    {
        if (*(int*)(0xBD400108) == *(int*)(0xBD40010C))
        {
            int oldCmd1, oldCmd2;
            // 2DF8
            g_queueSuspendInfo.unk4 = *(int*)(0xBD400100) | 0x1;
            int *stall = (int*)*(int*)(0xBD40010C);
            g_queueSuspendInfo.unk12 = stall;
            g_queueSuspendInfo.unk0 = *(int*)(0xBD400004);
            g_queueSuspendInfo.unk8 = *(int*)(0xBD400108);
            g_queueSuspendInfo.unk16 = *(int*)(0xBD400304);
            g_queueSuspendInfo.unk20 = *(int*)(0xBD400838);
            g_queueSuspendInfo.unk24 = *(int*)(0xBD40083C);
            g_queueSuspendInfo.unk28 = *(int*)(0xBD400830);
            oldCmd1 = stall[0];
            oldCmd2 = stall[1];
            stall[0] = 0x0F000000;
            stall[1] = 0x0C000000;
            pspCache(0x1A, &stall[0]);
            pspCache(0x1A, &stall[1]);
            if ((pspCop0StateGet(24) & 1) != 0)
            {
                pspSync();
                *(int*)(0xA7F00000) = (((int)stall | 0x08000000) & 0x0FFFFFC0) | 0xA0000001;
                *(int*)(0xA7F00000) = ((int)stall & 0x07FFFFC0) | 0xA0000001;
                (void)*(int*)(0xA7F00000);
            }
            
            // 2ECC
            *(int*)(0xBD40010C) += 8;
            // 2EE0
            while ((LW(0xBD400100) & 1) != 0)
                ;
            if (*(int*)(0xBD400108) == (int)g_queueSuspendInfo.unk12)
            {
                // 2F38
                *(int*)(0xBD40010C) = (int)stall;
                stall[0] = oldCmd1;
                stall[1] = oldCmd2;
                pspCache(0x1A, &stall[0]);
                pspCache(0x1A, &stall[1]);
                break;
            }
            else
            {
                // 2F08
                while ((LW(0xBD400304) & 4) == 0)
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
    if ((*(int*)(0xBD400304) & 1) == 0 && (g_queue.cur->signal != SCE_GE_DL_SIGNAL_BREAK || g_queue.isBreak != 0)) // 2DE8
    {
        // 2CB0
        while ((LW(0xBD400304) & 4) == 0)
            ;
    }

    // 2CC4
    g_queueSuspendInfo.unk0 = *(int*)(0xBD400004);
    g_queueSuspendInfo.unk4 = *(int*)(0xBD400100);
    g_queueSuspendInfo.unk8 = *(int*)(0xBD400108);
    g_queueSuspendInfo.unk12 = (int*)*(int*)(0xBD40010C);
    g_queueSuspendInfo.unk16 = *(int*)(0xBD400304);
    g_queueSuspendInfo.unk20 = *(int*)(0xBD400838);
    g_queueSuspendInfo.unk24 = *(int*)(0xBD40083C);
    g_queueSuspendInfo.unk28 = *(int*)(0xBD400830);
    sceSysregSetMasterPriv(64, 1);
    int old108 = *(int*)(0xBD400108);
    int old10C = *(int*)(0xBD40010C);
    int old304 = *(int*)(0xBD400304);
    *(int*)(0xBD400310) = 4;
    *(int*)(0xBD400108) = (int)UCACHED(stopCmd);
    *(int*)(0xBD40010C) = 0;
    *(int*)(0xBD400100) = 1;
    // 2D80
    while ((LW(0xBD400100) & 1) != 0)
        ;
    // 2D94
    while ((LW(0xBD400304) & 4) == 0)
        ;
    *(int*)(0xBD400108) = old108;
    *(int*)(0xBD40010C) = old10C;
    *(int*)(0xBD400310) = *(int*)(0xBD400304) ^ old304;
    sceSysregSetMasterPriv(64, 0);
    return 0;
}

int _sceGeQueueResume()
{
    dbg_printf("Ran %s\n", __FUNCTION__);
    if (g_queue.cur == NULL)
        return 0;
    // 2F88
    sceSysregSetMasterPriv(64, 1);
    *(int*)(0xBD400108) = (int)UCACHED(&g_queueSuspendInfo.unk20);
    *(int*)(0xBD40010C) = 0;
    *(int*)(0xBD400100) = g_queueSuspendInfo.unk4 | 1;
    // 2FC0
    while ((LW(0xBD400100) & 1) != 0)
        ;
    // 2FD4
    while ((LW(0xBD400304) & 4) == 0)
        ;
    sceSysregSetMasterPriv(64, 0);
    int oldFlag = g_queueSuspendInfo.unk16;
    int flag = 0;
    if ((oldFlag & 1) == 0)
        flag |= 1;
    if ((oldFlag & 2) == 0)
        flag |= 2;
    if ((oldFlag & 4) == 0)
        flag |= 4;
    *(int*)(0xBD400310) = flag;
    *(int*)(0xBD400108) = g_queueSuspendInfo.unk8;
    *(int*)(0xBD40010C) = (int)g_queueSuspendInfo.unk12;
    *(int*)(0xBD400100) = g_queueSuspendInfo.unk4;
    return 0;
}

void _sceGeFinishInterrupt(int arg0 __attribute__((unused)), int arg1 __attribute__((unused)), int arg2 __attribute__((unused)))
{
    dbg_printf("Ran %s\n", __FUNCTION__);
    SceGeDisplayList *dl = g_queue.curRunning;
    g_queue.isBreak = 0;
    g_queue.curRunning = NULL;
    if (dl != NULL)
    {
        if (dl->signal == SCE_GE_DL_SIGNAL_SYNC)
        {
            // 351C
            dl->signal = SCE_GE_DL_SIGNAL_NONE;
            g_queue.curRunning = dl;
            *(int*)(0xBD400100) |= 1;
            pspSync();
            return;
        }
        else if (dl->signal == SCE_GE_DL_SIGNAL_PAUSE)
        {
            // 3468
            int state = *(int*)(0xBD400100);
            dl->flags = state;
            dl->list = (int*)*(int*)(0xBD400108);
            dl->unk28 = *(int*)(0xBD400110);
            dl->unk32 = *(int*)(0xBD400114);
            dl->unk36 = *(int*)(0xBD400120);
            dl->unk40 = *(int*)(0xBD400124);
            dl->unk44 = *(int*)(0xBD400128);
            dl->unk48 = *(int*)(0xBD400840);
            if ((state & 0x200) == 0)
            {
                dl->unk32 = 0;
                dl->unk44 = 0;
                if ((state & 0x100) == 0) {
                    dl->unk28 = 0;
                    dl->unk40 = 0;
                }
            }
    
            // 34E8
            if (g_queue.cur == dl)
            {
                // 3500
                dl->signal = SCE_GE_DL_SIGNAL_BREAK;
                if (dl->cbId >= 0)
                {
                    void *list = dl->list;
                    // 3288 dup
                    if (g_queue.sdkVer <= 0x02000010)
                        list = 0;
                    sceKernelCallSubIntrHandler(25, dl->cbId * 2, dl->unk54, (int)list); // call signal CB
                }
                // 32A4
                _sceGeClearBp();
                return;
            }
            dl = NULL;
        }
        // 309C
        if (dl != NULL)
        {
            if (dl->state == SCE_GE_DL_STATE_RUNNING)
            {
                // 32DC
                int *cmdList = (int*)*(int*)(0xBD400108);
                u32 lastCmd1 = *(int*)UUNCACHED(cmdList - 2);
                u32 lastCmd2 = *(int*)UUNCACHED(cmdList - 1);
                if ((lastCmd1 >> 24) != 0x0F || (lastCmd2 >> 24) != 0x0C) { // FINISH, END
                    // 3318
                    Kprintf("_sceGeFinishInterrupt(): illegal finish sequence (MADR=0x%08X)\n", cmdList); // 6398
                }
                // 3328
                dl->state = SCE_GE_DL_STATE_COMPLETED;
                if (dl->stackOff != 0) {
                    // 343C
                    Kprintf("_sceGeFinishInterrupt(): CALL/RET nesting corrupted\n"); // 63D8
                }
                // 3338
                if (g_logHandler != NULL)
                {
                    // 3418
                    g_logHandler(6, (int)dl ^ g_dlMask, cmdList, lastCmd1, lastCmd2);
                }
                // 3348
                if (dl->cbId >= 0)
                {
                    if (g_queue.sdkVer <= 0x02000010)
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
                if (g_queue.cur == dl)
                    g_queue.cur = dl->next;
        
                // 33B8
                if (g_queue.next == dl) {
                    // 3400
                    g_queue.next = dl->prev;
                }
        
                // 33C4
                if (g_queue.first == NULL)
                {   
                    g_queue.first = dl;
                    // 33F8
                    dl->prev = NULL;
                }
                else {
                    g_queue.last->next = dl;
                    dl->prev = g_queue.last;
                }
        
                // 33E0
                dl->state = SCE_GE_DL_STATE_COMPLETED;
                dl->next = NULL;
                g_queue.last = dl;
            }
        }
    }
    // 30B0
    SceGeDisplayList *dl2 = g_queue.cur;
    if (dl2 == NULL)
    {
        // 32B4
        *(int*)(0xBD400100) = 0;
        pspSync();
        sceSysregAwRegABusClockDisable();
        sceKernelSetEventFlag(g_queue.drawingEvFlagId, 2);
        // 3254
        _sceGeClearBp();
    }
    else
    {
        if (dl2->signal == SCE_GE_DL_SIGNAL_PAUSE)
        {
            // 3264
            dl2->state = SCE_GE_DL_STATE_PAUSED;
            dl2->signal = SCE_GE_DL_SIGNAL_BREAK;
            if (dl2->cbId >= 0)
            {
                void *list = dl2->list;
                // 3288 dup
                if (g_queue.sdkVer <= 0x02000010)
                    list = NULL;
                sceKernelCallSubIntrHandler(25, dl2->cbId * 2, dl2->unk54, (int)list);
            }
            // 32A4
            _sceGeClearBp();
            return;
        }
        if (dl2->state == SCE_GE_DL_STATE_PAUSED)
        {
            // 324C
            sceSysregAwRegABusClockDisable();
            // 3254
            _sceGeClearBp();
        }
        else
        {
            int *ctx2 = (int*)dl2->ctx;
            dl2->state = SCE_GE_DL_STATE_RUNNING;
            if (ctx2 != NULL && dl2->ctxUpToDate == 0)
            {
                int *ctx1 = (int*)dl->ctx;
                if (dl == NULL || dl->ctx == NULL)
                {
                    // 323C
                    sceGeSaveContext(dl2->ctx);
                }
                else
                {
                    // 310C
                    int i;
                    for (i = 0; i < 128; i++)
                    {
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
            *(int*)(0xBD400100) = 0;
            *(int*)(0xBD40010C) = (int)UCACHED(dl2->stall);
            *(int*)(0xBD400108) = (int)UCACHED(dl2->list);
            *(int*)(0xBD400120) = dl2->unk36;
            *(int*)(0xBD400124) = dl2->unk40;
            *(int*)(0xBD400128) = dl2->unk44;
            _sceGeSetBaseRadr(dl2->unk48, dl2->unk28, dl2->unk32);
            pspSync();
            g_queue.curRunning = dl2;
            *(int*)(0xBD400100) = dl2->flags | 1;
            pspSync();
            if (g_logHandler != 0)
            {
                // 321C
                g_logHandler(5, (int)dl2 ^ g_dlMask, dl2->list, dl2->stall);
            }
        }
    }
    
    // 31C8
    if (dl != NULL) {
        u32 off = dl - &g_displayLists[0];
        sceKernelSetEventFlag(g_queue.listEvFlagIds[off >> 11], 1 << ((off >> 6) & 0x1F));
    }
}

void _sceGeListInterrupt(int arg0 __attribute__((unused)), int arg1 __attribute__((unused)), int arg2 __attribute__((unused)))
{
    dbg_printf("Ran %s\n", __FUNCTION__);
    SceGeDisplayList *dl = g_queue.cur;
    if (dl == NULL)
    {
        // 3C0C
        Kprintf("_sceGeListInterrupt(): unknown interrupt\n");
        return;
    }
    u32 *cmdList = (u32*)*(int*)(0xBD400108);
    u32 *lastCmdPtr1 = cmdList - 2;
    u32 *lastCmdPtr2 = cmdList - 1;
    u32 lastCmd1 = *(u32*)UUNCACHED(lastCmdPtr1);
    u32 lastCmd2 = *(u32*)UUNCACHED(lastCmdPtr2);
    // 35F4
    if ((lastCmd1 >> 24) != 0x0E || (lastCmd2 >> 24) != 0x0C) { // SIGNAL, END
        Kprintf("_sceGeListInterrupt(): bad signal sequence (MADR=0x%08X)\n", cmdList); // 0x643C
        return;
    }
    if (g_logHandler != NULL) {
        // 3BE8
        g_logHandler(7, (int)dl ^ g_dlMask, cmdList, lastCmd1, lastCmd2);
    }
    // 3618
    switch ((lastCmd1 >> 16) & 0xFF)
    {
    case 0x1: // HANDLER_SUSPEND
        // 3670
        if (g_queue.sdkVer <= 0x02000010) {
            dl->state = SCE_GE_DL_STATE_PAUSED;
            cmdList = NULL;
        }
        // 3698
        if (dl->cbId >= 0) {
            sceKernelCallSubIntrHandler(25, dl->cbId * 2, lastCmd1 & 0xFFFF, (int)cmdList);
            pspSync();
        }
        // 36B4
        if (g_queue.sdkVer <= 0x02000010)
            dl->state = lastCmd2 & 0xFF;
        *(int*)(0xBD400100) |= 1;
        break;

    case 0x2: // HANDLER_CONTINUE
        // 3708
        *(int*)(0xBD400100) |= 1;
        pspSync();
        if (dl->cbId >= 0)
        {
            if (g_queue.sdkVer <= 0x02000010)
                cmdList = NULL;
            sceKernelCallSubIntrHandler(25, dl->cbId * 2, lastCmd1 & 0xFFFF, (int)cmdList);
        }
        break;

    case 0x3: // HANDLER_PAUSE
        dl->state = SCE_GE_DL_STATE_PAUSED;
        dl->signal = lastCmd2 & 0xFF;
        dl->unk54 = lastCmd1;
        *(int*)(0xBD400100) |= 1;
        break;

    case 0x8: // SYNC
        // 3994
        dl->signal = SCE_GE_DL_SIGNAL_SYNC;
        *(int*)(0xBD400100) |= 1;
        break;

    case 0x11: // CALL
    case 0x14: // RCALL
    case 0x16: // OCALL
    {
        // 3870
        if (dl->stackOff >= dl->numStacks)
        {
            // 398C
            _sceGeListError(lastCmd1, 1);
            break;
        }
        SceGeStack *curStack = &dl->stack[dl->stackOff++];
        curStack->stack[0] = *(int*)(0xBD400100);
        curStack->stack[1] = *(int*)(0xBD400108);
        curStack->stack[2] = *(int*)(0xBD400120);
        curStack->stack[3] = *(int*)(0xBD400124);
        curStack->stack[4] = *(int*)(0xBD400128);
        curStack->stack[5] = *(int*)(0xBD400110);
        curStack->stack[6] = *(int*)(0xBD400114);
        curStack->stack[7] = *(int*)(0xBD400840);
        if ((dl->flags & 0x200) == 0)
        {
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
        if (((lastCmd1 >> 16) & 0xFF) == 0x11) // CALL
            newCmdList = (u32*)cmdOff;
        else if (((lastCmd1 >> 16) & 0xFF) == 0x14) // RCALL
            newCmdList = &cmdList[cmdOff / 4 - 2];
        else // OCALL
            newCmdList = *(int*)(0xBD400120) + (u32*)cmdOff;

        // 395C
        if (((int)newCmdList & 3) != 0)
        {
            // 397C
            _sceGeListError(lastCmd1, 0);
        }
        // 396C
        *(int*)(0xBD400108) = (int)UCACHED(newCmdList);
        *(int*)(0xBD400100) = 1;
        pspSync();
        break;
    }

    case 0x10: // JUMP
    case 0x13: // RJUMP
    case 0x15: // OJUMP
    {
        // 377C
        int cmdOff = (lastCmd1 << 16) | (lastCmd2 & 0xFFFF);
        u32 *newCmdList;
        if (((lastCmd1 >> 16) & 0xFF) == 0x10) // JUMP
            newCmdList = (u32*)cmdOff;
        else if (((lastCmd1 >> 16) & 0xFF) == 0x13) // RJUMP
            newCmdList = &cmdList[cmdOff / 4 - 2];
        else
            newCmdList = *(int*)(0xBD400120) + (u32*)cmdOff; // OJUMP
    
        // 37B4
        if (((int)newCmdList & 3) != 0) {
            // 37D0
            _sceGeListError(lastCmd1, 0);
        }
        // 37C4
        *(int*)(0xBD400108) = (int)UCACHED(newCmdList);
        *(int*)(0xBD400100) |= 1;
        break;
    }
    
    case 0x12: // RETURN
        // 37E0
        if (dl->stackOff == 0) {
            _sceGeListError(lastCmd1, 2);
            return;
        }
    
        // 3804
        SceGeStack *curStack = &dl->stack[dl->stackOff--];
        *(int*)(0xBD400108) = (int)UCACHED(curStack->stack[1]);
        *(int*)(0xBD400120) = curStack->stack[2];
        *(int*)(0xBD400124) = curStack->stack[3];
        *(int*)(0xBD400128) = curStack->stack[4];
        _sceGeSetBaseRadr(curStack->stack[7], curStack->stack[5], curStack->stack[6]);
        *(int*)(0xBD400100) = curStack->stack[0] | 1;
        pspSync();
        break;

    case 0x20: case 0x21: case 0x22: case 0x23: // RTBP0..7
    case 0x24: case 0x25: case 0x26: case 0x27:
    case 0x28: case 0x29: case 0x2A: case 0x2B: // OTBP0..7
    case 0x2C: case 0x2D: case 0x2E: case 0x2F:
    {
        // 39D0
        int off = (lastCmd1 << 16) | (lastCmd2 & 0xFFFF);
        u32 *newLoc;
        if (((lastCmd1 >> 19) & 1) == 0)
            newLoc = lastCmdPtr1 + off;
        else
            newLoc = *(int*)(0xBD400120) + (u32*)off;

        // 39F0
        if (((int)newLoc & 0xF) != 0)
        {
            // 3A50
            _sceGeListError(lastCmd1, 0);
        }
        // 3A00
        int id = (lastCmd1 >> 16) & 0x7;
        int *uncachedCmdPtr = UUNCACHED(lastCmdPtr1);
        uncachedCmdPtr[1] = ((id + 0xA8) << 24) | ((((int)newLoc >> 24) & 0xF) << 16) | ((lastCmd2 >> 16) & 0xFF); // TBW0..7;
        uncachedCmdPtr[0] = ((id + 0xA0) << 24) | ((int)newLoc & 0x00FFFFFF); // TBP0..7;
        // 3A40
        pspSync();
        *(int*)(0xBD400108) = (int)lastCmdPtr1;
        *(int*)(0xBD400100) |= 1;
        break;
    }

    case 0x30: // RCBP
    case 0x38: // OCBP
    {
        // 3A80
        int off = (lastCmd1 << 16) | (lastCmd2 & 0xFFFF);
        u32 *newLoc;
        if (((lastCmd1 >> 19) & 1) == 0) // RCBP
            newLoc = lastCmdPtr1 + off;
        else // OCBP
            newLoc = *(int*)(0xBD400120) + (u32*)off;

        // 3AA4
        if (((int)newLoc & 0xF) != 0)
        {
            // 3AE8
            _sceGeListError(lastCmd1, 0);
        }
        // 3AB4
        int *uncachedCmdPtr = UUNCACHED(lastCmdPtr1);
        uncachedCmdPtr[1] = ((((int)newLoc >> 24) & 0xF) << 16) | 0xB1000000; // CBPH
        uncachedCmdPtr[0] = ((int)newLoc & 0x00FFFFFF) | 0xB0000000; // CBP
        // 3A40
        pspSync();
        *(int*)(0xBD400108) = (int)lastCmdPtr1;
        *(int*)(0xBD400100) |= 1;
        break;
    }

    case 0xF0:
    case 0xFF: // BREAK
    {
        // 3B08
        if (g_deci2p == NULL) {
            *(int*)(0xBD400100) |= 1;
            return;
        }
        int opt = 0;
        if (((lastCmd1 >> 16) & 0xFF) == 0xFF)
        {
            // 3B6C
            SceGeBpCmd *bpCmd = &g_bpCtrl.cmds[0];
            int i;
            opt = 1;
            for (i = 0; i < g_bpCtrl.size; i++)
            {
                // 3B90
                if (UCACHED(bpCmd->unk0) == UCACHED(lastCmdPtr1))
                {
                    // 3BC0
                    if (bpCmd->unk4 == 0 || (bpCmd->unk4 != -1 && (--bpCmd->unk4) != 0))
                        opt = -1;
                    break;
                }
                bpCmd++;
            }
            // 3BB0
            if (opt < 0) {
                *(int*)(0xBD400100) |= 1;
                return;
            }
        }
        // 3B28
        *(int*)(0xBD400108) = (int)lastCmdPtr1;
        _sceGeClearBp();
        g_bpCtrl.busy = 1;
        g_bpCtrl.size2 = 0;

        // 3B48
        do
        {
            int (*func)(int) = (void*)*(int*)(g_deci2p + 32);
            func(opt);
            opt = 0;
        } while (g_bpCtrl.busy != 0);
        break;
    }

    default:
        Kprintf("_sceGeListInterrupt(): bad signal command (MADR=0x%08X)\n"); // 0x6478
        break;
    }
}

int sceGeListDeQueue(int dlId)
{
    dbg_printf("Ran %s\n", __FUNCTION__);
    int ret;
    SceGeDisplayList *dl = (SceGeDisplayList*)(dlId ^ g_dlMask);
    int oldK1 = pspShiftK1();
    if (dl < g_displayLists || dl >= &g_displayLists[64])
    {
        // 3D90
        pspSetK1(oldK1);
        return 0x80000100;
    }
    int oldIntr = sceKernelCpuSuspendIntr();
    if (dl->state != SCE_GE_DL_STATE_NONE)
    {
        if (dl->ctxUpToDate == 0)
        {
            if (dl->prev != NULL)
                dl->prev->next = dl->next;
            // 3CAC
            if (dl->next != NULL)
                dl->next->prev = dl->prev;
        
            // 3CB4
            if (g_queue.cur == dl)
                g_queue.cur = dl->next;

            // 3CC8
            if (g_queue.next == dl) {
                // 3D88
                g_queue.next = dl->prev;
            }
            // 3CD4
            if (g_queue.first == NULL)
            {
                g_queue.first = dl;
                // 3D80
                dl->prev = NULL;
            }
            else {
                g_queue.last->next = dl;
                dl->prev = g_queue.last;
            }

            // 3CF0
            dl->state = SCE_GE_DL_STATE_NONE;
            dl->next = NULL;
            g_queue.last = dl;
            sceKernelSetEventFlag(g_queue.listEvFlagIds[(u32)(dl - g_displayLists) >> 11], 1 << (((dl - g_displayLists) >> 6) & 0x1F));
            if (g_logHandler != NULL) {
                // 3D70
                g_logHandler(1, dlId);
            }
            ret = 0;
        }
        else
            ret = 0x80000021;
    }
    else
        ret = 0x80000100;

    // 3D3C
    sceKernelCpuResumeIntr(oldIntr);
    pspSetK1(oldK1);
    return ret;
}

SceGeListState sceGeListSync(int dlId, int mode)
{
    dbg_printf("Ran %s\n", __FUNCTION__);
    int ret;
    SceGeDisplayList *dl = (SceGeDisplayList*)(dlId ^ g_dlMask);
    u32 off = dl - &g_displayLists[0];
    int oldK1 = pspShiftK1();
    if (off >= 4096)
    {
        // 3EE0
        pspSetK1(oldK1);
        return 0x80000100;
    }

    if (mode == 0)
    {
        // 3E84
        int oldIntr = sceKernelCpuSuspendIntr();
        _sceGeListLazyFlush();
        sceKernelCpuResumeIntr(oldIntr);
        ret = sceKernelWaitEventFlag(g_queue.listEvFlagIds[off / 2048], 1 << ((off / 64) & 0x1F), 0, 0, 0);
        if (ret >= 0)
            ret = SCE_GE_LIST_COMPLETED;
    }
    else if (mode == 1)
    {
        // 3E10
        switch (dl->state)
        {
        case SCE_GE_DL_STATE_QUEUED:
            if (dl->ctxUpToDate != 0)
                ret = SCE_GE_LIST_PAUSED;
            else
                ret = SCE_GE_LIST_QUEUED;
            break;

        case SCE_GE_DL_STATE_RUNNING:
            // 3E68
            if ((int)dl->stall != *(int*)(0xBD400108))
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
    }
    else
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
    dbg_printf("Ran %s\n", __FUNCTION__);
    int ret;
    int oldK1 = pspShiftK1();
    if (syncType == 0)
    {
        // 3FA4
        int oldIntr = sceKernelCpuSuspendIntr();
        _sceGeListLazyFlush();
        sceKernelCpuResumeIntr(oldIntr);
        ret = sceKernelWaitEventFlag(g_queue.drawingEvFlagId, 2, 0, 0, 0);
        if (ret >= 0)
        {
            // 3FF4
            int i;
            for (i = 0; i < 64; i++)
            {
                SceGeDisplayList *curDl = &g_displayLists[i];
                if (curDl->state == SCE_GE_DL_STATE_COMPLETED)
                {
                    // 4010
                    curDl->state = SCE_GE_DL_STATE_NONE;
                    curDl->ctxUpToDate = 0;
                }
                // 4000
            }
            ret = SCE_GE_LIST_COMPLETED;
        }
    }
    else if (syncType == 1)
    {
        // 3F40
        int oldIntr = sceKernelCpuSuspendIntr();
        SceGeDisplayList *dl = g_queue.cur;
        if (dl == NULL)
        {
            // 3F9C
            ret = SCE_GE_LIST_COMPLETED;
        }
        else
        {
            if (dl->state == SCE_GE_DL_STATE_COMPLETED)
                dl = dl->next;

            // 3F68
            if (dl != NULL)
            {
                if ((int)dl->stall != *(int*)(0xBD400108))
                    ret = SCE_GE_LIST_STALLING;
                else
                    ret = SCE_GE_LIST_DRAWING;
            }
            else
                ret = SCE_GE_LIST_COMPLETED;
        }
        // 3F8C
        sceKernelCpuResumeIntr(oldIntr);
    }
    else
        ret = 0x80000107;

    // 3F24
    pspSetK1(oldK1);
    return ret;
}

int sceGeBreak(u32 resetQueue, void *arg1)
{
    dbg_printf("Ran %s\n", __FUNCTION__);
    int ret;
    if (resetQueue >= 2)
        return 0x80000107;
    int oldK1 = pspShiftK1();
    if (!pspK1StaBufOk(arg1, 16))
    {
        // 4368
        pspSetK1(oldK1);
        return 0x80000023;
    }
    int oldIntr = sceKernelCpuSuspendIntr();
    SceGeDisplayList *dl = g_queue.cur;
    if (dl != NULL)
    {
        if (resetQueue)
        {
            // 42F0
            _sceGeReset();
            // 430C
            int i;
            for (i = 0; i < 64; i++)
            {
                SceGeDisplayList *cur = &g_displayLists[i];
                cur->next = &g_displayLists[i + 1];
                cur->prev = &g_displayLists[i - 1];
                cur->signal = SCE_GE_DL_SIGNAL_NONE;
                cur->state = SCE_GE_DL_STATE_NONE;
                cur->ctxUpToDate = 0;
            }
            g_queue.last = &g_displayLists[63];
            g_queue.curRunning = NULL;
            g_queue.next = NULL;
            g_displayLists[0].prev = NULL;
            g_displayLists[63].next = NULL;
            g_queue.first = g_displayLists;
            g_queue.cur = NULL;
            ret = 0;
        }
        else if (dl->state == SCE_GE_DL_STATE_RUNNING)
        {
            // 4174
            *(int*)(0xBD400100) = 0;
            pspSync();
        
            // 4180
            while ((LW(0xBD400100) & 1) != 0)
                ;
            if (g_queue.curRunning != NULL)
            {
                int *cmdList = (int*)*(int*)(0xBD400108);
                int state = *(int*)(0xBD400100);
                dl->list = cmdList;
                dl->flags = state;
                dl->stall = (int*)*(int*)(0xBD40010C);
                dl->unk28 = *(int*)(0xBD400110);
                dl->unk32 = *(int*)(0xBD400114);
                dl->unk36 = *(int*)(0xBD400120);
                dl->unk40 = *(int*)(0xBD400124);
                dl->unk44 = *(int*)(0xBD400128);
                dl->unk48 = *(int*)(0xBD400840);
                if ((state & 0x200) == 0)
                {
                    dl->unk32 = 0;
                    dl->unk44 = 0;
                    if ((state & 0x100) == 0) {
                        dl->unk28 = 0;
                        dl->unk40 = 0;
                    }
                }
            
                // 4228
                int op = *((char*)UUNCACHED(cmdList - 1) + 3);
                if (op == 0x0F || op == 0x0E) // SIGNAL / FINISH
                {
                    // 42E8
                    dl->list = cmdList - 1;
                }
                else if (op == 0x0C) { // END
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
            *(int*)(0xBD40010C) = 0;
            *(int*)(0xBD400108) = (int)UUNCACHED(g_cmdList);
            *(int*)(0xBD400310) = 6;
            *(int*)(0xBD400100) = 1;
            pspSync();
            g_queue.isBreak = 1;
            g_queue.curRunning = NULL;
            ret = (int)dl ^ g_dlMask;
        }
        else if (dl->state == SCE_GE_DL_STATE_PAUSED)
        {
            // 4130
            ret = 0x80000021;
            if (g_queue.sdkVer > 0x02000010)
            {
                if (dl->signal == SCE_GE_DL_SIGNAL_PAUSE) {
                    // 4160
                    Kprintf("sceGeBreak(): can't break signal-pausing list\n"); // 64B4
                }
                else
                    ret = 0x80000020;
            }
        }
        else if (dl->state == SCE_GE_DL_STATE_QUEUED)
        {
            // 40FC
            dl->state = SCE_GE_DL_STATE_PAUSED;
            ret = (int)dl ^ g_dlMask;
        }
        else if (g_queue.sdkVer >= 0x02000000)
            ret = 0x80000004;
        else
            ret = -1;
    }
    else
        ret = 0x80000020;

    // 410C
    if (ret == 0 && g_logHandler != NULL)
        g_logHandler(3, resetQueue);
    sceKernelCpuResumeIntr(oldIntr);
    pspSetK1(oldK1);
    return ret;
}

int sceGeContinue()
{
    dbg_printf("Ran %s\n", __FUNCTION__);
    int oldK1 = pspShiftK1();
    int ret = 0;
    int oldIntr = sceKernelCpuSuspendIntr();
    SceGeDisplayList *dl = g_queue.cur;
    if (dl != NULL)
    {
        if (dl->state == SCE_GE_DL_STATE_PAUSED)
        {
            // 4444
            if (g_queue.isBreak == 0)
            {
                // 4474
                if (dl->signal != SCE_GE_DL_SIGNAL_PAUSE)
                {
                    dl->state = SCE_GE_DL_STATE_NONE;
                    dl->signal = SCE_GE_DL_SIGNAL_NONE;
                    sceSysregAwRegABusClockEnable();
                    // 448C
                    while ((LW(0xBD400100) & 1) != 0)
                        ;
                    *(int*)(0xBD400310) = 6;
                    if (dl->ctx != NULL && dl->ctxUpToDate == 0) {
                        // 4598
                        sceGeSaveContext(dl->ctx);
                    }
                    // 44BC
                    _sceGeWriteBp(dl->list);
                    dl->ctxUpToDate = 1;
                    *(int*)(0xBD400108) = (int)UCACHED(dl->list);
                    *(int*)(0xBD40010C) = (int)UCACHED(dl->stall);
                    *(int*)(0xBD400120) = dl->unk36;
                    *(int*)(0xBD400124) = dl->unk40;
                    *(int*)(0xBD400128) = dl->unk44;
                    _sceGeSetBaseRadr(dl->unk48, dl->unk28, dl->unk32);
                    *(int*)(0xBD400310) = 6;
                    *(int*)(0xBD400100) = dl->flags | 1;
                    pspSync();
                    g_queue.curRunning = dl;
                    sceKernelClearEventFlag(g_queue.drawingEvFlagId, 0xFFFFFFFD);
                    if (g_logHandler != NULL)
                    {
                        g_logHandler(4, 0);
                        if (g_logHandler != NULL)
                            g_logHandler(5, (int)dl ^ g_dlMask, dl->list, dl->stall);
                    }
                }
                else {
                    // 45A8
                    ret = 0x80000021;
                }
            }
            else
            {
                dl->state = SCE_GE_DL_STATE_QUEUED;
                if (g_logHandler != NULL)
                    g_logHandler(4, 0);
            }
        }
        else if (dl->state == SCE_GE_DL_STATE_RUNNING)
        {
            // 4428
            if (g_queue.sdkVer >= 0x02000000)
                ret = 0x80000020;
            else
                ret = -1;
        }
        else
        {
            if (g_queue.sdkVer >= 0x02000000)
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

int sceGeSetCallback(SceGeCallbackData *cb)
{   
    dbg_printf("Ran %s\n", __FUNCTION__);
    int oldK1 = pspShiftK1();
    if (!pspK1PtrOk(cb)
     || !pspK1PtrOk(cb->signal_func)
     || !pspK1PtrOk(cb->finish_func))
    {
        // 4604
        pspSetK1(oldK1);
        return 0x80000023;
    }
    // 4634
    int oldIntr = sceKernelCpuSuspendIntr();

    // 4650
    int i;
    for (i = 0; i < 16; i++)
    {
        if (((g_cbBits >> i) & 1) == 0)
        {
            // 46E4
            g_cbBits |= (1 << i);
            break;
        }
    }
    // 466C
    sceKernelCpuResumeIntr(oldIntr);
    if (i == 16)
    {
        // 46D8
        pspSetK1(oldK1);
        return 0x80000022;
    }

    if (cb->signal_func != NULL)
        sceKernelRegisterSubIntrHandler(25, i * 2 + 0, cb->signal_func, cb->signal_arg);
    // 469C
    if (cb->finish_func != 0)
        sceKernelRegisterSubIntrHandler(25, i * 2 + 1, cb->finish_func, cb->finish_arg);
    // 46B4
    sceKernelEnableSubIntr(25, i * 2 + 0);
    sceKernelEnableSubIntr(25, i * 2 + 1);
    pspSetK1(oldK1);
    return i;
}

int sceGePutBreakpoint(int *inPtr, int size)
{   
    dbg_printf("Ran %s\n", __FUNCTION__);
    if (size >= 9)
        return 0x80000104;
    int oldIntr = sceKernelCpuSuspendIntr();
    int oldK1 = pspShiftK1();
    if (!pspK1DynBufOk(inPtr, size * 8))
    {
        // 4850 dup
        pspSetK1(oldK1);
        sceKernelCpuResumeIntr(oldIntr);
        return 0x80000023;
    }
    // 477C
    int i;
    for (i = 0; i < size; i++)
    {
        int (*func)(int, int, int) = (void*)*(int*)(g_deci2p + 36);
        if (func(inPtr[i * 2], 4, 3) < 0)
        {
            // 4850 dup
            pspSetK1(oldK1);
            sceKernelCpuResumeIntr(oldIntr);
            return 0x80000023;
        }
    }
    // 47A8
    _sceGeClearBp();
    g_bpCtrl.size = size;
    // 47C8
    for (i = 0; i < g_bpCtrl.size; i++) {
        g_bpCtrl.cmds[i].unk0 = inPtr[i * 2 + 0] & 0xFFFFFFFC;
        g_bpCtrl.cmds[i].unk4 = inPtr[i * 2 + 1];
    }
    // 47F4
    if (g_bpCtrl.busy == 0) {
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
    dbg_printf("Ran %s\n", __FUNCTION__);
    int oldIntr = sceKernelCpuSuspendIntr();
    int oldK1 = pspShiftK1();
    if (!pspK1DynBufOk(outPtr, size * 8)
     || !pspK1PtrOk(arg2))
    {
        // 48C0
        pspSetK1(oldK1);
        sceKernelCpuResumeIntr(oldIntr);
        return 0x80000023;
    }
    // 48EC
    SceGeBpCmd *bpCmd = &g_bpCtrl.cmds[0];
    int count = 0;
    // 4904
    int i;
    for (i = 0; i < g_bpCtrl.size; i++)
    {
        if (i < size)
        {
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
        *arg2 = g_bpCtrl.size;
    // 4950
    pspSetK1(oldK1);
    sceKernelCpuResumeIntr(oldIntr);
    return count;
}

int sceGeGetListIdList(int *outPtr, int size, int *totalCountPtr)
{
    dbg_printf("Ran %s\n", __FUNCTION__);
    int oldIntr = sceKernelCpuSuspendIntr();
    int oldK1 = pspShiftK1();
    if (!pspK1DynBufOk(outPtr, size * 4)
     || !pspK1PtrOk(totalCountPtr))
    {
        // 49B8
        pspSetK1(oldK1);
        sceKernelCpuResumeIntr(oldIntr);
        return 0x80000023;
    }
    // 49E8
    SceGeDisplayList *dl = g_queue.cur;
    int storedCount = 0;
    int totalCount = 0;
    // 4A04
    while (dl != NULL)
    {
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

int sceGeGetList(int dlId, SceGeDisplayList *outDl, int *outFlag)
{   
    dbg_printf("Ran %s\n", __FUNCTION__);
    int oldIntr = sceKernelCpuSuspendIntr();
    if (!pspK1PtrOk(outDl) || !pspK1PtrOk(outFlag))
    {
        // 4A8C
        sceKernelCpuResumeIntr(oldIntr);
        return 0x80000023;
    }
    // 4AB8
    SceGeDisplayList *dl = (SceGeDisplayList*)(dlId ^ g_dlMask);
    if (dl < g_displayLists || dl >= &g_displayLists[64])
    {
        // 4B40
        sceKernelCpuResumeIntr(oldIntr);
        return 0x80000100;
    }
    if (outDl != NULL)
    {
        int *outPtr = (int*)outDl;
        int *inPtr = (int*)dl;
        // 4AE8
        do
        {
            outPtr[0] = inPtr[0];
            outPtr[1] = inPtr[1];
            outPtr[2] = inPtr[2];
            outPtr[3] = inPtr[3];
            outPtr += 4;
            inPtr += 4;
        } while (inPtr != (int*)(dl + 64));
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
    dbg_printf("Ran %s\n", __FUNCTION__);
    SceGeDisplayList *dl = g_queue.cur;
    if (dl == NULL)
        return 0;
    if (stackId < 0)
        return dl->stackOff;
    int oldIntr = sceKernelCpuSuspendIntr();
    int oldK1 = pspShiftK1();
    if (stackId >= dl->stackOff)
    {
        // 4C40
        pspSetK1(oldK1);
        sceKernelCpuResumeIntr(oldIntr);
        return 0x80000102;
    }
    if (!pspK1PtrOk(stack))
    {
        // 4C2C
        pspSetK1(oldK1);
        sceKernelCpuResumeIntr(oldIntr);
        return 0x80000023;
    }
    if (stack != NULL)
    {
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
    dbg_printf("Ran %s\n", __FUNCTION__);
    int oldIntr = sceKernelCpuSuspendIntr();
    if (g_bpCtrl.busy != 0)
    {
        // 4CF4
        sceKernelCpuResumeIntr(oldIntr);
        return 0x80000020;
    }
    g_bpCtrl.size2 = 0;
    g_bpCtrl.busy = 1;
    int wasEnabled = sceSysregAwRegABusClockEnable();
    *(int*)(0xBD400100) = 0;
    // 4C9C
    while ((LW(0xBD400100) & 1) != 0)
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
    dbg_printf("Ran %s\n", __FUNCTION__);
    SceGeDisplayList *dl = g_queue.cur;
    if (dl == NULL)
        return 0;
    int oldIntr = sceKernelCpuSuspendIntr();
    if (g_bpCtrl.busy == 0)
    {
        // 50B0
        sceKernelCpuResumeIntr(oldIntr);
        return 0x80000021;
    }
    int wasEnabled = sceSysregAwRegABusClockEnable();
    if ((*(int*)(0xBD400304) & 2) != 0)
    {
        // 5084
        if (!wasEnabled) {
            // 50A0
            sceSysregAwRegABusClockDisable();
        }
        sceKernelCpuResumeIntr(oldIntr);
        return 0x80000021;
    }
    int *cmdPtr = (int*)*(int*)(0xBD400108);
    u32 curCmd = *cmdPtr;
    int hasSignalFF = 0;
    if ((curCmd >> 24) == 0xE && ((curCmd >> 16) & 0xFF) == 0xFF) // 5044 // SIGNAL 0xFF
    {
        cmdPtr += 2;
        *(int*)(0xBD400108) = (int)cmdPtr;
        hasSignalFF = 1;
        if (arg0 == 1)
        {
            if (!wasEnabled)
                sceSysregAwRegABusClockDisable();
            // 4EE8 dup
            sceKernelCpuResumeIntr(oldIntr);
            return 0;
        }
    }
    // (4D80)
    g_bpCtrl.busy = 0;
    // 4D84
    if (arg0 == 0)
    {
        // 4F44 dup
        g_bpCtrl.size2 = 0;
    }
    else
    {
        u32 curCmd = *cmdPtr;
        int *nextCmdPtr1 = cmdPtr + 1;
        int flag = *(int*)(0xBD400100);
        int op = curCmd >> 24;
        if ((op == 8 || op == 9) || (op == 0xA && arg0 == 1)) // JUMP / BJUMP / CALL
        {
            // 4DCC
            if (op != 9 || (flag & 2) == 0) // 5038 // BJUMP
                nextCmdPtr1 = (int*)((((*(int*)(0xBD400840) << 8) & 0xFF000000) | (curCmd & 0x00FFFFFF)) + *(int*)(0xBD400120));
            // 4DFC
        }
        // 4E00
        int *nextCmdPtr2 = cmdPtr + 2;
        if (op == 0xB) // RET
        {
            // 5004
            if ((flag & 0x200) == 0)
            {   
                // 5020
                if ((flag & 0x100) != 0)
                    nextCmdPtr1 = (int*)*(int*)(0xBD400110);
            }
            else
                nextCmdPtr1 = (int*)*(int*)(0xBD400114);
        }
        else
        {
            if (op == 0xF) // FINISH
                nextCmdPtr1 = nextCmdPtr2;
            else if (op == 0xE) // SIGNAL
            {
                // 4F54
                int signalOp = (curCmd >> 16) & 0x000000FF;
                int off = (curCmd << 16) | (*(cmdPtr + 1) & 0xFFFF);
                nextCmdPtr1 = nextCmdPtr2;
                if (signalOp != 0x10 && (signalOp != 0x11 || arg0 != 1))
                {
                    // 4F94
                    if (signalOp == 0x13)
                    {
                        // 4FAC
                        nextCmdPtr1 = cmdPtr + off;
                    }
                    else
                    {
                        if (signalOp != 0x14 || arg0 != 1)
                        {   
                            // 4FB4
                            if (signalOp != 0x15)
                            {
                                if ((signalOp != 0x16 || arg0 != 1) && signalOp == 0x12 && dl->stackOff != 0) // 4FDC
                                    nextCmdPtr1 = (int*)dl->stack[dl->stackOff - 1].stack[1];
                            }
                            else
                            {
                                // 4FCC
                                nextCmdPtr1 = (int*)*(int*)(0xBD400120) + off;
                            }
                         }
                    }
                }
                else {
                    // 4F8C
                    nextCmdPtr1 = (int*)off;
                }
            }
        }
        
        // (4E18)
        // (4E1C)
        // 4E20
        g_bpCtrl.cmds[8].unk4 = 1;
        g_bpCtrl.size2 = 1;
        g_bpCtrl.cmds[8].unk0 = (int)nextCmdPtr1;
        int maxCount = g_bpCtrl.size;
        SceGeBpCmd *bpCmd = &g_bpCtrl.cmds[0];
        // 4E50
        int i;
        for (i = 0; i < maxCount; i++)
        {
            // 4E50
            if (UCACHED(bpCmd->unk0) == UCACHED(nextCmdPtr1))
            {
                // 4F4C
                g_bpCtrl.size2 = 0;
                break;
            }
            bpCmd++;
        }
        // 4E6C
        op = *nextCmdPtr1 >> 24;
        if (op == 0xE && ((*nextCmdPtr1 >> 16) & 0xFF) == 0xFF) { // 4F38
            // 4F44 dup
            g_bpCtrl.size2 = 0;
        }
    }
    // 4E80
    if (hasSignalFF == 0)
    {
        SceGeBpCmd *bpCmd = &g_bpCtrl.cmds[0];
        // 4EA0
        int i;
        for (i = 0; i < g_bpCtrl.size; i++)
        {
            if (UCACHED(bpCmd->unk0) == UCACHED(cmdPtr))
            {
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
    *(int*)(0xBD400100) |= 1;
    sceKernelEnableIntr(25);
    // 4EE8 dup
    sceKernelCpuResumeIntr(oldIntr);
    return 0;
}

int _sceGeQueueInitCallback()
{
    dbg_printf("Ran %s\n", __FUNCTION__);
    void *str = (void*)sceKernelGetUsersystemLibWork();
    if (str != NULL)
    {
        void *str2 = (void*)*(int*)(str + 8);
        *(int*)(str2 + 204) = ((g_queue.syscallId << 6) & 0x03FFFFC0) | 0xC;
        pspCache(0x1A, str2 + 204);
        pspCache(0x08, str2 + 204);
        g_queue.lazySyncData = (int*)*(int*)(str + 12);
    }
    return 0;
}

int _sceGeQueueEnd()
{
    dbg_printf("Ran %s\n", __FUNCTION__);
    sceKernelDeleteEventFlag(g_queue.drawingEvFlagId);
    sceKernelDeleteEventFlag(g_queue.listEvFlagIds[0]);
    sceKernelDeleteEventFlag(g_queue.listEvFlagIds[1]);
    return 0;
}

int _sceGeQueueStatus(void)
{
    dbg_printf("Ran %s\n", __FUNCTION__);
    if (g_queue.cur == NULL)
        return 0;
    return -1;
}

void _sceGeErrorInterrupt(int arg0 __attribute__((unused)), int arg1 __attribute__((unused)), int arg2 __attribute__((unused)))
{
    dbg_printf("Ran %s\n", __FUNCTION__);
}

int sceGeListEnQueue(void *list, void *stall, int cbid, SceGeListArgs *arg)
{
    dbg_printf("Ran %s\n", __FUNCTION__);
    return _sceGeListEnQueue(list, stall, cbid, arg, 0);
}

int sceGeListEnQueueHead(void *list, void *stall, int cbid, SceGeListArgs *arg)
{
    dbg_printf("Ran %s\n", __FUNCTION__);
    return _sceGeListEnQueue(list, stall, cbid, arg, 1);
}

int sceGeUnsetCallback(int cbId)
{   
    dbg_printf("Ran %s\n", __FUNCTION__);
    int oldK1 = pspShiftK1();
    if (cbId < 0 || cbId >= 16)
    {
        // 528C
        pspSetK1(oldK1);
        return 0x80000100;
    }
    int oldIntr = sceKernelCpuSuspendIntr();
    sceKernelDisableSubIntr(25, cbId * 2 + 0);
    sceKernelDisableSubIntr(25, cbId * 2 + 1);
    sceKernelReleaseSubIntrHandler(25, cbId * 2 + 0);
    sceKernelReleaseSubIntrHandler(25, cbId * 2 + 1);
    g_cbBits &= ~(1 << cbId);
    sceKernelCpuResumeIntr(oldIntr);
    pspSetK1(oldK1);
    return 0;
}

void _sceGeListError(u32 cmd, int err) // err: 0 = alignment problem, 1: overflow, 2: underflow
{
    dbg_printf("Ran %s\n", __FUNCTION__);
    int op = (cmd >> 16) & 0xFF;
    int *curAddr = (int*)*(int*)(0xBD400108) - 2;
    char *cmdStr;
    switch (cmd) // 0x6794 jump table
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

    case 0x80:
        // 53A0
        cmdStr = "RCBP";
        break;
    
    case 0xA0:
        // 53D8
        cmdStr = "OCBP";
        break;

    default:
        // 53AC
        if (op >= 0x28)
            g_szTbp[0] = 'O'; // at 6880
        // 53C4
        cmdStr = g_szTbp;
        cmdStr[4] = '0' + (op & 7);
        break;
    }

    // 52E0
    switch (err)
    {
    case 0:
        // 533C
        Kprintf("SCE_GE_SIGNAL_%s address error (MADR=0x%08X)\n", cmdStr, curAddr);
        break;

    case 1:
        // 534C
        Kprintf("SCE_GE_SIGNAL_%s stack overflow (MADR=0x%08X)\n", cmdStr, curAddr);
        break;

    case 2:
        // 5328
        Kprintf("SCE_GE_SIGNAL_%s stack underflow (MADR=0x%08X)\n", cmdStr, curAddr);
        break;
    }
    // 52FC
    if (sceKernelGetCompiledSdkVersion() >= 0x02000000)
        pspBreak(0);
}

void _sceGeWriteBp(int *list)
{
    dbg_printf("Ran %s\n", __FUNCTION__);
    if (g_deci2p == NULL)
        return;
    if (g_bpCtrl.clear != 0)
        return;
    g_bpCtrl.clear = 1;
    int *prevList = list - 1;
    int i;
    for (i = 0; i < g_bpCtrl.size; i++)
    {
        SceGeBpCmd *bpCmd = &g_bpCtrl.cmds[i];
        int *ptr2 = (int*)(bpCmd->unk0 & 0xFFFFFFFC);
        u32 cmd = ptr2[0];
        bpCmd->unk8  = cmd;
        bpCmd->unk12 = ptr2[1];
        if (((bpCmd->unk0 ^ (int)prevList) & 0x1FFFFFFC) != 0)
        {
            // 5574
            if ((bpCmd->unk0 & 3) == 0)
            {
                // 55C4
                if (cmd >> 24 != 0x0C) // END
                    ptr2[0] = 0x0EFF0000; // SIGNAL END
                // 55CC
                ptr2[1] = 0x0C000000; // END
            }
            else
                bpCmd->unk0 = (int)ptr2;
            // 5580
            pspCache(0x1A, &ptr2[0]);
            pspCache(0x1A, &ptr2[1]);
            if ((pspCop0StateGet(24) & 1) != 0)
            {
                pspSync();
                *(int*)(0xA7F00000) = (((int)ptr2 | 0x08000000) & 0x0FFFFFC0) | 0xA0000001;
                (void)*(int*)(0xA7F00000);
            }
        }
        // 5494
    }

    // 54A4
    SceGeBpCmd *bpCmd = &g_bpCtrl.cmds[8];
    
    // 54E0
    for (i = 0; i < g_bpCtrl.size2; i++)
    {
        int *ptr = (int*)bpCmd->unk0;
        bpCmd->unk8 = ptr[0];
        bpCmd->unk12 = ptr[1];
        if ((((int)ptr ^ (int)prevList) & 0x1FFFFFFC) != 0)
        {
            // 5528
            ptr[0] = 0x0EF00000; // SIGNAL BREAK
            ptr[1] = 0x0C000000;
            pspCache(0x1A, &ptr[0]);
            pspCache(0x1A, &ptr[1]);
            if ((pspCop0StateGet(24) & 1) != 0)
            {
                pspSync();
                *(int*)(0xA7F00000) = (((int)ptr | 0x08000000) & 0x0FFFFFC0) | 0xA0000001;
                (void)*(int*)(0xA7F00000);
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
    dbg_printf("Ran %s\n", __FUNCTION__);
    if (g_deci2p == NULL)
        return;
    if (g_bpCtrl.clear == 0)
        return;
    g_bpCtrl.clear = 0;
    // 5624
    int i;
    for (i = 0; i < g_bpCtrl.size2; i++)
    {
        SceGeBpCmd *cmd = &g_bpCtrl.cmds[7 + (g_bpCtrl.size2 - i)];
        int *out = (int*)cmd->unk0;
        out[0] = cmd->unk8;
        out[1] = cmd->unk12;
        pspCache(0x1A, &out[0]);
        pspCache(0x1A, &out[1]);
        if ((pspCop0StateGet(24) & 1) != 0)
        {
            pspSync();
            *(int*)(0xA7F00000) = (((int)out | 0x08000000) & 0x0FFFFFC0) | 0xA0000001;
            (void)*(int*)(0xA7F00000);
        }
        // 5684
    }

    // 5694, 56C4
    for (i = 0; i < g_bpCtrl.size; i++)
    {
        SceGeBpCmd *cmd = &g_bpCtrl.cmds[7 + (g_bpCtrl.size - i)];
        int *out = (int*)(cmd->unk0 & 0xFFFFFFFC);
        out[0] = cmd->unk8;
        out[1] = cmd->unk12;
        pspCache(0x1A, &out[0]);
        pspCache(0x1A, &out[1]);
        if ((pspCop0StateGet(24) & 1) != 0)
        {
            pspSync();
            *(int*)(0xA7F00000) = (((int)out | 0x08000000) & 0x0FFFFFC0) | 0xA0000001;
            (void)*(int*)(0xA7F00000);
        }
        // 5728
    }
    // 5738
    pspSync();
}

void _sceGeListLazyFlush()
{
    dbg_printf("Ran %s\n", __FUNCTION__);
    int *ptr = g_queue.lazySyncData;
    if (ptr != NULL && ptr[0] >= 0)
    {
        ptr[0] = -1;
        ptr[2] = 0;
        sceGeListUpdateStallAddr(ptr[0], (void*)ptr[1]);
    }
}

int _sceGeListEnQueue(void *list, void *stall, int cbid, SceGeListArgs *arg, int head)
{
    dbg_printf("Ran %s\n", __FUNCTION__);
    int oldK1 = pspShiftK1();
    if (!pspK1PtrOk(list)
     || !pspK1PtrOk(stall)
     || !pspK1StaBufOk(arg, 16)) {
        pspSetK1(oldK1);
        return 0x80000023;
    }
    // 5834
    int ver = sceKernelGetCompiledSdkVersion();
    dbg_printf("%d\n", __LINE__);
    g_queue.sdkVer = ver;
    void *stack = &g_queue.stack;
    SceGeContext *ctx = NULL;
    int numStacks = 32;
    int newVer = 1;
    if (ver > 0x01FFFFFF)
    {
        numStacks = 0;
        stack = NULL;
        newVer = 0;
    }
    dbg_printf("%d\n", __LINE__);

    // 5878
    if (arg != NULL)
    {
        ctx = arg->ctx;
        if (!pspK1StaBufOk(ctx, 2048)) {
            pspSetK1(oldK1);
            return 0x80000023;
        }
        if (arg->size >= 16)
        {
            numStacks = arg->numStacks;
            if (numStacks >= 256)
            {
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
    dbg_printf("%d\n", __LINE__);
    // (58DC)
    // 58E0
    if ((((int)list | (int)stall | (int)ctx | (int)stack) & 3) != 0)
    {
        // 5CAC
        pspSetK1(oldK1);
        return 0x80000103;
    }
    dbg_printf("%d\n", __LINE__);
    if (g_queue.patched != 0 && pspK1IsUserMode()) { // 5C94
        g_queue.patched--;
        sceKernelDcacheWritebackAll();
    }
    dbg_printf("%d\n", __LINE__);

    // 58FC
    int oldIntr = sceKernelCpuSuspendIntr();
    dbg_printf("%d\n", __LINE__);
    _sceGeListLazyFlush();
    dbg_printf("%d\n", __LINE__);
    SceGeDisplayList *dl = g_queue.cur;
    dbg_printf("%d\n", __LINE__);
    // 5920
    while (dl != NULL)
    {
        if (UCACHED((int)dl->list ^ (int)list) == NULL)
        {
            // 5C74
            Kprintf("_sceGeListEnQueue(): can't enqueue duplicated addr(MADR=0x%08X)\n", list); // 0x65B8
            if (newVer)
                break;
            sceKernelCpuResumeIntr(oldIntr);
            pspSetK1(oldK1);
            return 0x80000021;
        }
        if (dl->ctxUpToDate && stack != NULL && dl->stack == stack && !newVer) // 5C44
        {
            Kprintf("_sceGeListEnQueue(): can't enqueue duplicated stack(STACK=0x%08X)\n", stack); // 0x65FC
            // 5C60
            sceKernelCpuResumeIntr(oldIntr);
            pspSetK1(oldK1);
            return 0x80000021;
        }
        dl = dl->next;
        // 5954
    }
    dbg_printf("%d\n", __LINE__);

    // 5960
    dl = g_queue.first;
    if (dl == NULL)
    {
        // 5C24
        sceKernelCpuResumeIntr(oldIntr);
        pspSetK1(oldK1);
        return 0x80000022;
    }
    if (dl->next == 0)
    {   
        g_queue.last = NULL;
        // 5C3C
        g_queue.first = NULL;
    }
    else {
        g_queue.first = dl->next;
        dl->next->prev = NULL;
    }

    // 5984
    dl->prev = NULL;
    dl->next = NULL;
    if (dl == NULL)
    {
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
    if (head != 0)
    {
        // 5B8C
        if (g_queue.cur != NULL)
        {                                                                                                                                       
            // 5BE0
            if (g_queue.cur->state != SCE_GE_DL_STATE_PAUSED)
            {
                // 5C0C
                sceKernelCpuResumeIntr(oldIntr);
                pspSetK1(oldK1);
                return 0x800001FE;
            }
            dl->state = SCE_GE_DL_STATE_PAUSED;
            g_queue.cur->state = SCE_GE_DL_STATE_QUEUED;
            dl->prev = NULL;
            g_queue.cur->prev = dl;
            dl->next = g_queue.cur;
        }
        else
        {
            dl->state = SCE_GE_DL_STATE_PAUSED;
            dl->prev = NULL;
            dl->next = NULL;
            g_queue.next = dl;
        }

        // 5BB8
        g_queue.cur = dl;
        if (g_logHandler != NULL)
            g_logHandler(0, dlId, 1, list, stall);
    }
    else if (g_queue.cur == NULL)
    {
        // 5A8C
        dl->state = SCE_GE_DL_STATE_RUNNING;
        dl->ctxUpToDate = 1;
        dl->prev = NULL;
        g_queue.cur = dl;
        g_queue.next = dl;
        sceSysregAwRegABusClockEnable();
        if (ctx != NULL)
            sceGeSaveContext(ctx);

        // 5ABC
        _sceGeWriteBp(list);
        *(int*)(0xBD400100) = 0;
        *(int*)(0xBD400108) = (int)UCACHED(list);
        *(int*)(0xBD40010C) = (int)UCACHED(stall);
        *(int*)(0xBD400120) = 0;
        *(int*)(0xBD400124) = 0;
        *(int*)(0xBD400128) = 0;
        *(int*)(0xBD400110) = 0;
        *(int*)(0xBD400114) = 0;
        pspSync();
        *(int*)(0xBD400100) = 1;
        pspSync();
        g_queue.curRunning = dl;
        sceKernelClearEventFlag(g_queue.drawingEvFlagId, 0xFFFFFFFD);
        if (g_logHandler != NULL)
        {
            g_logHandler(0, dlId, 0, list, stall);
            if (g_logHandler != NULL)
                g_logHandler(5, dlId, list, stall);
        }
    }
    else
    {
        dl->state = SCE_GE_DL_STATE_QUEUED;
        g_queue.next->next = dl;
        dl->prev = g_queue.next;
        g_queue.next = dl;
        if (g_logHandler != NULL) {
            // 5A6C
            g_logHandler(0, dlId, 0, list, stall);
        }
    }

    // 5A28
    u32 off = dl - g_displayLists;
    sceKernelClearEventFlag(g_queue.listEvFlagIds[off / 2048], ~(1 << ((off / 64) & 0x1F)));
    sceKernelCpuResumeIntr(oldIntr);
    pspSetK1(oldK1);
    return dlId;
}

