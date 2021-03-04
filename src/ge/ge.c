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

char *_dbg_cmds[256] = {
"NOP", // 0x00
"VADR", // 0x01
"IADR", // 0x02
"UNK03",
"PRIM", // 0x04
"BEZIER", // 0x05
"SPLINE", // 0x06
"BBOX", // 0x07
"JUMP", // 0x08
"BJUMP", // 0x09
"CALL", // 0x0A
"RET", // 0x0B
"END", // 0x0C
"UNK0D",
"SIGNAL", // 0x0E
"FINISH", // 0x0F
"BASE", // 0x10
"UNK11",
"VTYPE", // 0x12
"OFFSET", // 0x13
"ORIGIN", // 0x14
"REGION1", // 0x15
"REGION2", // 0x16
"LTE", // 0x17
"LE0", // 0x18
"LE1", // 0x19
"LE2", // 0x1A
"LE3", // 0x1B
"CLE", // 0x1C
"BCE", // 0x1D
"TME", // 0x1E
"FGE", // 0x1F
"DTE", // 0x20
"ABE", // 0x21
"ATE", // 0x22
"ZTE", // 0x23
"STE", // 0x24
"AAE", // 0x25
"PCE", // 0x26
"CTE", // 0x27
"LOE", // 0x28
"UNK29",
"BONEN", // 0x2A
"BONED", // 0x2B
"WEIGHT0", // 0x2C
"WEIGHT1", // 0x2D
"WEIGHT2", // 0x2E
"WEIGHT3", // 0x2F
"WEIGHT4", // 0x30
"WEIGHT5", // 0x31
"WEIGHT6", // 0x32
"WEIGHT7", // 0x33
"UNK34",
"UNK35",
"DIVIDE", // 0x36
"PPM", // 0x37
"PFACE", // 0x38
"UNK39",
"WORLDN", // 0x3A
"WORLDD", // 0x3B
"VIEWN", // 0x3C
"VIEWD", // 0x3D
"PROJN", // 0x3E
"PROJD", // 0x3F
"TGENN", // 0x40
"TGEND", // 0x41
"SX", // 0x42
"SY", // 0x43                                                                                                                                                                           
"SZ", // 0x44                                                                                                                                                                           
"TX", // 0x45
"TY", // 0x46
"TZ", // 0x47
"SU", // 0x48
"SV", // 0x49
"TU", // 0x4A
"TV", // 0x4B
"OFFSETX", // 0x4C
"OFFSETY", // 0x4D
"UNK4E",
"UNK4F",
"SHADE", // 0x50
"NREV", // 0x51
"UNK52",
"MATERIAL", // 0x53
"MEC", // 0x54
"MAC", // 0x55
"MDC", // 0x56
"MSC", // 0x57
"MAA", // 0x58
"UNK59",
"UNK5A",
"MK", // 0x5B
"AC", // 0x5C
"AA", // 0x5D
"LMODE", // 0x5E
"LTYPE0", // 0x5F
"LTYPE1", // 0x60
"LTYPE2", // 0x61
"LTYPE3", // 0x62
"LX0", // 0x63
"LY0", // 0x64
"LZ0", // 0x65
"LX1", // 0x66
"LY1", // 0x67
"LZ1", // 0x68
"LX2", // 0x69
"LY2", // 0x6A
"LZ2", // 0x6B
"LX3", // 0x6C
"LY3", // 0x6D
"LZ3", // 0x6E
"LDX0", // 0x6F
"LDY0", // 0x70
"LDZ0", // 0x71
"LDX1", // 0x72
"LDY1", // 0x73
"LDZ1", // 0x74
"LDX2", // 0x75
"LDY2", // 0x76
"LDZ2", // 0x77
"LDX3", // 0x78
"LDY3", // 0x79
"LDZ3", // 0x7A
"LKA0", // 0x7B
"LKB0", // 0x7C
"LKC0", // 0x7D
"LKA1", // 0x7E
"LKB1", // 0x7F
"LKC1", // 0x80
"LKA2", // 0x81
"LKB2", // 0x82
"LKC2", // 0x83
"LKA3", // 0x84
"LKB3", // 0x85
"LKC3", // 0x86
"LKS0", // 0x87
"LKS1", // 0x88
"LKS2", // 0x89
"LKS3", // 0x8A
"LKO0", // 0x8B
"LKO1", // 0x8C
"LKO2", // 0x8D
"LKO3", // 0x8E
"LAC0", // 0x8F
"LDC0", // 0x90
"LSC0", // 0x91
"LAC1", // 0x92
"LDC1", // 0x93
"LSC1", // 0x94
"LAC2", // 0x95
"LDC2", // 0x96
"LSC2", // 0x97
"LAC3", // 0x98
"LDC3", // 0x99
"LSC3", // 0x9A
"CULL", // 0x9B
"FBP", // 0x9C
"FBW", // 0x9D
"ZBP", // 0x9E
"ZBW", // 0x9F
"TBP0", // 0xA0
"TBP1", // 0xA1
"TBP2", // 0xA2
"TBP3", // 0xA3
"TBP4", // 0xA4
"TBP5", // 0xA5
"TBP6", // 0xA6
"TBP7", // 0xA7
"TBW0", // 0xA8
"TBW1", // 0xA9
"TBW2", // 0xAA
"TBW3", // 0xAB
"TBW4", // 0xAC
"TBW5", // 0xAD
"TBW6", // 0xAE
"TBW7", // 0xAF
"CBP", // 0xB0
"CBW", // 0xB1
"XBP1", // 0xB2
"XBW1", // 0xB3
"XBP2", // 0xB4
"XBW2", // 0xB5
"UNKB6",
"UNKB7",
"TSIZE0", // 0xB8
"TSIZE1", // 0xB9
"TSIZE2", // 0xBA
"TSIZE3", // 0xBB
"TSIZE4", // 0xBC
"TSIZE5", // 0xBD
"TSIZE6", // 0xBE
"TSIZE7", // 0xBF
"TMAP", // 0xC0
"TSHADE", // 0xC1
"TMODE", // 0xC2
"TPF", // 0xC3
"CLOAD", // 0xC4
"CLUT", // 0xC5
"TFILTER", // 0xC6
"TWRAP", // 0xC7
"TLEVEL", // 0xC8
"TFUNC", // 0xC9
"TEC", // 0xCA
"TFLUSH", // 0xCB
"TSYNC", // 0xCC
"FOG1", // 0xCD
"FOG2", // 0xCE
"FC", // 0xCF
"TSLOPE", // 0xD0
"UNKD1",
"FPF", // 0xD2
"CMODE", // 0xD3
"SCISSOR1", // 0xD4
"SCISSOR2", // 0xD5
"MINZ", // 0xD6
"MAXZ", // 0xD7
"CTEST", // 0xD8
"CREF", // 0xD9
"CMSK", // 0xDA
"ATEST", // 0xDB
"STEST", // 0xDC
"SOP", // 0xDD
"ZTEST", // 0xDE
"BLEND", // 0xDF
"FIXA", // 0xE0
"FIXB", // 0xE1
"DITH1", // 0xE2
"DITH2", // 0xE3
"DITH3", // 0xE4
"DITH4", // 0xE5
"LOP", // 0xE6
"ZMSK", // 0xE7
"PMSK1", // 0xE8
"PMSK2", // 0xE9
"XSTART", // 0xEA
"XPOS1", // 0xEB
"XPOS2", // 0xEC
"UNKED",
"XSIZE", // 0xEE
"UNKEF",
"X2", // 0xF0
"Y2", // 0xF1
"Z2", // 0xF2
"S2", // 0xF3
"T2", // 0xF4
"Q2", // 0xF5
"RGB2", // 0xF6
"AP2", // 0xF7
"F2", // 0xF8
"I2", // 0xF9
"UNKFA",
"UNKFB",
"UNKFC",
"UNKFD",
"UNKFE",
"UNKFF",
};

SCE_MODULE_INFO("sceGE_Manager", SCE_MODULE_KERNEL | SCE_MODULE_ATTR_CANT_STOP | SCE_MODULE_ATTR_EXCLUSIVE_LOAD
                                 | SCE_MODULE_ATTR_EXCLUSIVE_START, 1, 11);
SCE_MODULE_BOOTSTART("_sceGeModuleStart");
SCE_MODULE_REBOOT_BEFORE("_sceGeModuleRebootBefore");
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wattribute-alias"
SCE_MODULE_REBOOT_PHASE("_sceGeModuleRebootPhase");
#pragma GCC diagnostic pop
SCE_SDK_VERSION(SDK_VERSION);

#define HW_GE_CTRL               HW(0xA7F00000)
#define HW_GE_RESET              HW(0xBD400000)
#define HW_GE_UNK004             HW(0xBD400004)
#define HW_GE_EDRAM_HW_SIZE      HW(0xBD400008)
// RW bit 0x001: 0 = stopped, 1 = running
// R  bit 0x002: 0 = branching condition true, 1 = false
// R  bit 0x100: 1 = is at depth 1 (or 2) of calls
// R  bit 0x200: 1 = is at depth 2 of calls
#define HW_GE_EXEC               HW(0xBD400100)
#define HW_GE_UNK104             HW(0xBD400104)
#define HW_GE_LISTADDR           HW(0xBD400108)
#define HW_GE_STALLADDR          HW(0xBD40010C)
// first return address
#define HW_GE_RADR1              HW(0xBD400110)
// second return address
#define HW_GE_RADR2              HW(0xBD400114)
// address of vertices (for bezier etc)
#define HW_GE_VADR               HW(0xBD400118)
// address of indices (for bezier etc)
#define HW_GE_IADR               HW(0xBD40011C)
// address of the origin (set by ORIGIN, used by some signals)
#define HW_GE_OADR               HW(0xBD400120)
// same, for the first call
#define HW_GE_OADR1              HW(0xBD400124)
// same, for the second call
#define HW_GE_OADR2              HW(0xBD400128)
#define HW_GE_GEOMETRY_CLOCK     HW(0xBD400200)
#define HW_GE_UNK300             HW(0xBD400300)
#define HW_GE_INTERRUPT_TYPE1    HW(0xBD400304)
#define HW_GE_INTERRUPT_TYPE2    HW(0xBD400308)
#define HW_GE_INTERRUPT_TYPE3    HW(0xBD40030C)
#define HW_GE_INTERRUPT_TYPE4    HW(0xBD400310)
#define HW_GE_EDRAM_ENABLED_SIZE HW(0xBD400400)
#define HW_GE_EDRAM_REFRESH_UNK1 HW(0xBD500000)
#define HW_GE_EDRAM_UNK10        HW(0xBD500010)
#define HW_GE_EDRAM_REFRESH_UNK2 HW(0xBD500020)
#define HW_GE_EDRAM_REFRESH_UNK3 HW(0xBD500030)
#define HW_GE_EDRAM_UNK40        HW(0xBD500040)
#define HW_GE_EDRAM_UNK50        HW(0xBD500050)
#define HW_GE_EDRAM_UNK60        HW(0xBD500060)
#define HW_GE_EDRAM_ADDR_TRANS_DISABLE HW(0xBD500070)
#define HW_GE_EDRAM_ADDR_TRANS_VALUE   HW(0xBD500080)
#define HW_GE_EDRAM_UNK90        HW(0xBD500090)
#define HW_GE_EDRAM_UNKA0        HW(0xBD5000A0)
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

#define SCE_GE_INTERNAL_REG_BASE_ADDR 1
#define SCE_GE_INTERNAL_REG_RADR1 2
#define SCE_GE_INTERNAL_REG_RADR2 4

#define SCE_GE_INTSIG 1
#define SCE_GE_INTEND 2
#define SCE_GE_INTFIN 4
#define SCE_GE_INTERR 8

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
    int unk0;
    int status;
    int listAddr; // 8
    int *stallAddr; // 12
    int intrType; // 16
    int sigCmd;
    int finCmd;
    int endCmd;
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

#define SCE_GE_REG_LISTADDR 5
#define SCE_GE_REG_RADR1 7
#define SCE_GE_REG_RADR2 8
#define SCE_GE_REG_UNK128 13
#define SCE_GE_REG_EDRAM_SIZE 19

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
    HW_GE_EDRAM_UNK10 = 2;
    HW_GE_RESET = 1;
    // 0144
    while ((HW_GE_RESET & 1) != 0)
        ;
    sceGeSaveContext(&_aw_ctx);
    _aw_ctx.ctx[16] = HW_GE_GEOMETRY_CLOCK;
    sceSysregAwResetEnable();
    sceSysregAwResetDisable();
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
    HW_GE_INTERRUPT_TYPE2 = SCE_GE_INTSIG | SCE_GE_INTEND | SCE_GE_INTFIN;
    HW_GE_GEOMETRY_CLOCK = _aw_ctx.ctx[16];
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
    HW_GE_INTERRUPT_TYPE2 = SCE_GE_INTSIG | SCE_GE_INTEND | SCE_GE_INTFIN;
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
    HW_GE_INTERRUPT_TYPE3 = HW_GE_INTERRUPT_TYPE2;
    HW_GE_LISTADDR = (int)UCACHED(dlist);
    HW_GE_STALLADDR = 0;
    sceSysregSetMasterPriv(64, 1);
    HW_GE_EXEC = 1;

    // 04B8
    while ((HW_GE_EXEC & 1) != 0)
        ;
    sceSysregSetMasterPriv(64, 0);
    HW_GE_EXEC = 0;
    HW_GE_LISTADDR = 0;
    HW_GE_STALLADDR = 0;
    HW_GE_INTERRUPT_TYPE4 = HW_GE_INTERRUPT_TYPE1;
    HW_GE_INTERRUPT_TYPE2 = SCE_GE_INTSIG | SCE_GE_INTEND | SCE_GE_INTFIN;
    sceKernelRegisterIntrHandler(SCE_GE_INT, 1, _sceGeInterrupt, 0, &g_GeIntrOpt);

    // 0534
    for (i = 0; i < 16; i++)
        sceKernelSetUserModeIntrHanlerAcceptable(SCE_GE_INT, i, 1);

    sceKernelEnableIntr(SCE_GE_INT);
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
    dbg_printf("sceGeEnd\n");
    _sceGeQueueEnd();
    sceKernelUnregisterSysEventHandler(&g_GeSysEv);
    sceKernelDisableIntr(SCE_GE_INT);
    sceKernelReleaseIntrHandler(SCE_GE_INT);
    sceSysregAwRegABusClockEnable();
    HW_GE_EXEC = 0;
    // 0640
    while ((HW_GE_EXEC & 1) != 0)
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
        HW_GE_OADR = 0;
        HW_GE_LISTADDR = (int)UCACHED(cmdOut);
        HW_GE_STALLADDR = 0;
        pspSync();
        HW_GE_EXEC = 1;
        // 06E0
        while ((HW_GE_EXEC & 1) != 0)
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
    dbg_printf("sceGeGetReg\n");
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

int sceGeSetReg(u32 regId, u32 value)
{
    dbg_printf("sceGeSetReg\n");
    if (regId >= 32)
        return SCE_ERROR_INVALID_INDEX;
    if (regId >= SCE_GE_REG_LISTADDR && regId <= SCE_GE_REG_UNK128 && (value & 3) != 0)
        return SCE_ERROR_INVALID_VALUE;

    // 092C
    int oldK1 = pspShiftK1();
    int oldIntr = sceKernelCpuSuspendIntr();
    int wasEnabled = sceSysregAwRegABusClockEnable();
    int ret;
    if ((HW_GE_EXEC & 1) == 0) {
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

int sceGeGetCmd(u32 cmdOff)
{
    dbg_printf("sceGeGetCmd\n");
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

int sceGeSetCmd(u32 cmdOff, u32 cmd)
{
    dbg_printf("sceGeSetCmd\n");
    if (cmdOff >= 0xFF)
        return SCE_ERROR_INVALID_INDEX;
    int oldK1 = pspShiftK1();
    int oldIntr = sceKernelCpuSuspendIntr();
    int wasEnabled = sceSysregAwRegABusClockEnable();
    int ret = 0;
    if ((HW_GE_EXEC & 1) != 0) {
        ret = SCE_ERROR_BUSY;
        goto end;
    }
    int oldState = HW_GE_EXEC;
    int listAddr = HW_GE_LISTADDR;
    if (cmdOff == SCE_GE_CMD_JUMP || cmdOff == SCE_GE_CMD_BJUMP || cmdOff == SCE_GE_CMD_CALL) {
        int addr = (((HW_GE_CMD(SCE_GE_CMD_BASE) << 8) & 0xFF000000) | (cmd & 0x00FFFFFF)) + HW_GE_OADR;
        if (!GE_VALID_ADDR(addr)) {
            // 0E68 dup
            ret = SCE_ERROR_INVALID_POINTER;
            goto end;
        } else {
            // 0B88
            if (cmdOff == SCE_GE_CMD_BJUMP) {
                // 0E50
                if ((oldState & 2) == 0)
                    listAddr = addr;
                else {
                    listAddr += 4;
                    oldState &= 0xFFFFFFFD;
                }
            } else if (cmdOff == SCE_GE_CMD_CALL) { // 0DE0
                if ((oldState & 0x200) != 0) {
                    // 0E48 dup
                    ret = SCE_ERROR_NOT_IMPLEMENTED;
                    goto end;
                } else if ((oldState & 0x100) == 0) {
                    // 0E24
                    _sceGeSetRegRadr1(listAddr + 4);
                    listAddr = addr;
                    oldState |= 0x100;
                    HW_GE_OADR1 = HW_GE_OADR;
                } else {
                    _sceGeSetRegRadr2(listAddr + 4);
                    oldState = (oldState & 0xFFFFFEFF) | 0x200;
                    listAddr = addr;
                    HW_GE_OADR2 = HW_GE_OADR;
                }
            } else if (cmdOff == SCE_GE_CMD_JUMP) {
                listAddr = addr;
            }
            // 0BA4 dup
            cmd = HW_GE_CMD(SCE_GE_CMD_NOP);
            cmdOff = 0;
        }
    } else if (cmdOff == SCE_GE_CMD_RET) {
        // 13E0
        if ((oldState & 0x200) == 0) {
            // 1410
            if ((oldState & 0x100) == 0) {
                // 0E48 dup
                ret = SCE_ERROR_NOT_IMPLEMENTED;
                goto end;
            } else {
                listAddr = HW_GE_RADR1;
                // 1404 dup
                HW_GE_OADR = HW_GE_OADR1;
                oldState &= 0xFFFFFEFF;
            }
        } else {
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
        HW_GE_OADR = listAddr;
        cmdOff = 0;
        cmd = HW_GE_CMD(SCE_GE_CMD_NOP);
    } else if (cmdOff == SCE_GE_CMD_PRIM || cmdOff == SCE_GE_CMD_BEZIER || cmdOff == SCE_GE_CMD_SPLINE) {
        int addr = HW_GE_VADR;
        if (!GE_VALID_ADDR(addr)) {
            ret = SCE_ERROR_INVALID_POINTER;
            goto end;
        }

        // 0F14
        if (((HW_GE_CMD(SCE_GE_CMD_VTYPE) >> 11) & 3) != 0)
        {
            int addr = HW_GE_IADR;
            if (!GE_VALID_ADDR(addr)) {
                ret = SCE_ERROR_INVALID_POINTER;
                goto end;
            }
        }
        // 0F70
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
        int addr = ((HW_GE_CMD(SCE_GE_CMD_CBW) << 8) & 0xFF000000) | (HW_GE_CMD(SCE_GE_CMD_CBP) & 0x00FFFFFF);
        if (!GE_VALID_ADDR(addr)) {
            // 0E68 dup
            ret = SCE_ERROR_INVALID_POINTER;
            goto end;
        }
    } else if (cmdOff == SCE_GE_CMD_XSTART) {
        int addr1 = ((HW_GE_CMD(SCE_GE_CMD_XBW1) & 0x00FF0000) << 8) | (HW_GE_CMD(SCE_GE_CMD_XBP1) & 0x00FFFFFF);
        int addr2 = ((HW_GE_CMD(SCE_GE_CMD_XBW2) & 0x00FF0000) << 8) | (HW_GE_CMD(SCE_GE_CMD_XBP2) & 0x00FFFFFF);
        if (!GE_VALID_ADDR(addr1) || !GE_VALID_ADDR(addr2)) {
            // 11B8
            ret = SCE_ERROR_INVALID_POINTER;
            goto end;
        }
    }
    // 0BB0
    int buf[32];
    // 0BB8
    int stallAddr = HW_GE_STALLADDR;
    int *ptr = (int *)(((int)buf | 0x3F) + 1);
    int prevStatus = HW_GE_INTERRUPT_TYPE1;
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
        HW_GE_CTRL = ((int)ptr & 0x07FFFFC0) | 0xA0000001;
        HW_GE_CTRL;
    }
    // 0C88
    sceSysregSetMasterPriv(64, 1);
    HW_GE_INTERRUPT_TYPE4 = SCE_GE_INTFIN;
    HW_GE_LISTADDR = (int)UCACHED(ptr);
    HW_GE_STALLADDR = 0;
    pspSync();
    HW_GE_EXEC = oldState | 1;
    // 0CC0
    while ((HW_GE_EXEC & 1) != 0)
        ;
    // 0CD4
    while ((HW_GE_INTERRUPT_TYPE1 & SCE_GE_INTFIN) == 0)
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
    dbg_printf("sceGeGetMtx\n");
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
    dbg_printf("sceGeSetMtx\n");
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
    dbg_printf("sceGeSaveContext\n");
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
    dbg_printf("sceGeRestoreContext\n");
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
    HW_GE_INTERRUPT_TYPE4 = (old304 ^ HW_GE_INTERRUPT_TYPE1) & ~(SCE_GE_INTFIN | SCE_GE_INTSIG);
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
            HW_GE_CTRL =
                (((int)uncachedNewCmdList | 0x08000000) &
                 0x0FFFFFC0) | 0xA0000000;
            HW_GE_CTRL;
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
            HW_GE_CTRL =
                (((int)uncachedNewCmdList | 0x08000000) &
                 0x0FFFFFC0) | 0xA0000000;
            HW_GE_CTRL;
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
            HW_GE_CTRL =
                (((int)uncachedNewCmdList | 0x08000000) &
                 0x0FFFFFC0) | 0xA0000000;
            HW_GE_CTRL;
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
            HW_GE_CTRL =
                (((int)uncachedNewCmdList | 0x08000000) &
                 0x0FFFFFC0) | 0xA0000000;
            HW_GE_CTRL;
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
    if ((attr & SCE_GE_INTERR) != 0) {
        // 2228
        HW_GE_INTERRUPT_TYPE4 = 8;
        _sceGeErrorInterrupt(attr, unk1, arg2);
    }
    // 2118
    if ((attr & (SCE_GE_INTSIG | SCE_GE_INTFIN)) == (SCE_GE_INTSIG | SCE_GE_INTFIN)) {
        // 2218
        Kprintf("GE INTSIG/INTFIN at the same time\n"); // 0x6324
    }
    // 2128
    if ((attr & SCE_GE_INTFIN) == 0) {
        // signal and/or end
        // 21AC
        if ((attr & SCE_GE_INTSIG) == 0 && (attr & SCE_GE_INTEND) != 0) {   // 2208
            // 21FC dup
            HW_GE_INTERRUPT_TYPE3 = SCE_GE_INTEND;
        } else if ((attr & SCE_GE_INTSIG) != 0 && (attr & SCE_GE_INTEND) == 0) {
            // 21FC dup
            HW_GE_INTERRUPT_TYPE3 = SCE_GE_INTSIG;
        } else {
            // 21C0
            while ((HW_GE_EXEC & 1) != 0)
                ;
            HW_GE_INTERRUPT_TYPE4 = SCE_GE_INTSIG | SCE_GE_INTEND;
            HW_GE_INTERRUPT_TYPE2 = SCE_GE_INTSIG | SCE_GE_INTEND;
            _sceGeListInterrupt(attr, unk1, arg2);
        }
    } else {
        if ((attr & SCE_GE_INTEND) == 0) {
            // 2198
            Kprintf("CMD_FINISH must be used with CMD_END.\n"); // 0x6348
            HW_GE_EXEC = 0;
        }
        // 213C
        while ((HW_GE_EXEC & 1) != 0)
            ;
        HW_GE_INTERRUPT_TYPE4 = SCE_GE_INTEND | SCE_GE_INTFIN;
        HW_GE_INTERRUPT_TYPE2 = SCE_GE_INTEND | SCE_GE_INTFIN;
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
        HW_GE_INTERRUPT_TYPE2 = SCE_GE_INTSIG | SCE_GE_INTEND | SCE_GE_INTFIN;
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
    dbg_init(1, FB_NONE, FAT_HARDWARE);
    dbg_printf("Doing init\n");
    sceGeInit();
    dbg_printf("Init ok\n");
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
        sceGeSetReg(SCE_GE_REG_EDRAM_SIZE, 4);
        // 2934 dup
        sceSysregSetAwEdramSize(0);
    } else if (size == 0x400000) {
        // 28FC
        if (sceSysregGetTachyonVersion() <= 0x4FFFFF)
            return SCE_ERROR_INVALID_SIZE;
        g_uiEdramSize = size;
        sceGeSetReg(SCE_GE_REG_EDRAM_SIZE, 2);
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
                HW_GE_CTRL =
                    (((int)stall | 0x08000000) & 0x0FFFFFC0) | 0xA0000001;
                HW_GE_CTRL = ((int)stall & 0x07FFFFC0) | 0xA0000001;
                HW_GE_CTRL;
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
                while ((HW_GE_INTERRUPT_TYPE1 & SCE_GE_INTFIN) == 0)
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
    if ((HW_GE_INTERRUPT_TYPE1 & SCE_GE_INTSIG) == 0 && (g_AwQueue.active_first->signal != SCE_GE_DL_SIGNAL_BREAK || g_AwQueue.isBreak != 0)) // 2DE8
    {
        // 2CB0
        while ((HW_GE_INTERRUPT_TYPE1 & SCE_GE_INTFIN) == 0)
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
    HW_GE_INTERRUPT_TYPE4 = SCE_GE_INTFIN;
    HW_GE_LISTADDR = (int)UCACHED(stopCmd);
    HW_GE_STALLADDR = 0;
    HW_GE_EXEC = 1;
    // 2D80
    while ((HW_GE_EXEC & 1) != 0)
        ;
    // 2D94
    while ((HW_GE_INTERRUPT_TYPE1 & SCE_GE_INTFIN) == 0)
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
    while ((HW_GE_INTERRUPT_TYPE1 & SCE_GE_INTFIN) == 0)
        ;
    sceSysregSetMasterPriv(64, 0);
    int oldFlag = g_GeSuspend.intrType;
    int flag = 0;
    if ((oldFlag & SCE_GE_INTSIG) == 0)
        flag |= SCE_GE_INTSIG;
    if ((oldFlag & SCE_GE_INTEND) == 0)
        flag |= SCE_GE_INTEND;
    if ((oldFlag & SCE_GE_INTFIN) == 0)
        flag |= SCE_GE_INTFIN;
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
    //dbg_printf("_sceGeFinishInterrupt\n");
    SceGeDisplayList *dl = g_AwQueue.curRunning;
    g_AwQueue.isBreak = 0;
    g_AwQueue.curRunning = NULL;
    if (dl != NULL) {
        //dbg_printf("_sceGeFinishInterrupt: cur running != NULL\n");
        if (dl->signal == SCE_GE_DL_SIGNAL_SYNC) {
            //dbg_printf("_sceGeFinishInterrupt: cur running = SYNC\n");
            // 351C
            dl->signal = SCE_GE_DL_SIGNAL_NONE;
            g_AwQueue.curRunning = dl;
            HW_GE_EXEC |= 1;
            pspSync();
            return;
        } else if (dl->signal == SCE_GE_DL_SIGNAL_PAUSE) {
            //dbg_printf("_sceGeFinishInterrupt: cur running = PAUSE\n");
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
                //dbg_printf("_sceGeFinishInterrupt: cur running = still running\n");
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
                    g_GeLogHandler(6, (int)dl ^ g_dlMask,
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
        //dbg_printf("_sceGeFinishInterrupt: active first = NULL\n");
        // 32B4
        HW_GE_EXEC = 0;
        pspSync();
        sceSysregAwRegABusClockDisable();
        sceKernelSetEventFlag(g_AwQueue.drawingEvFlagId, 2);
        // 3254
        _sceGeClearBp();
    } else {
        //dbg_printf("_sceGeFinishInterrupt: active first != NULL\n");
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
                g_GeLogHandler(5, (int)dl2 ^ g_dlMask, dl2->list, dl2->stall);
            }
        }
    }

    // 31C8
    if (dl != NULL) {
        u32 idx = (dl - g_displayLists) / sizeof(SceGeDisplayList);
        //dbg_printf("_sceGeFinishInterrupt: signal %d\n", idx);
        sceKernelSetEventFlag(g_AwQueue.listEvFlagIds[idx / 32], 1 << (idx % 32));
    }
    //dbg_printf("_sceGeFinishInterrupt: end\n");
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
            sceKernelCallSubIntrHandler(SCE_GE_INT, dl->cbId * 2,
                                        lastCmd1 & 0xFFFF, (int)cmdList);
            pspSync();
        }
        // 36B4
        if (g_AwQueue.sdkVer <= 0x02000010)
            dl->state = lastCmd2 & 0xFF;
        HW_GE_EXEC |= 1;
        break;

    case GE_SIGNAL_HANDLER_CONTINUE:
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

    case GE_SIGNAL_HANDLER_PAUSE:
        dl->state = SCE_GE_DL_STATE_PAUSED;
        dl->signal = lastCmd2 & 0xFF;
        dl->signalData = lastCmd1;
        HW_GE_EXEC |= 1;
        break;

    case GE_SIGNAL_SYNC:
        // 3994
        dl->signal = SCE_GE_DL_SIGNAL_SYNC;
        HW_GE_EXEC |= 1;
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
            if (((lastCmd1 >> 16) & 0xFF) == GE_SIGNAL_CALL)
                newCmdList = (u32 *) cmdOff;
            else if (((lastCmd1 >> 16) & 0xFF) == GE_SIGNAL_RCALL)
                newCmdList = &cmdList[cmdOff / 4 - 2];
            else
                newCmdList = HW_GE_OADR + (u32 *) cmdOff;

            // 395C
            if (((int)newCmdList & 3) != 0) {
                // 397C
                _sceGeListError(lastCmd1, 0);
            }
            // 396C
            HW_GE_LISTADDR = (int)UCACHED(newCmdList);
            HW_GE_EXEC = 1;
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
                newCmdList = HW_GE_OADR + (u32 *) cmdOff;

            // 37B4
            if (((int)newCmdList & 3) != 0) {
                // 37D0
                _sceGeListError(lastCmd1, 0);
            }
            // 37C4
            HW_GE_LISTADDR = (int)UCACHED(newCmdList);
            HW_GE_EXEC |= 1;
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
        HW_GE_LISTADDR = (int)UCACHED(curStack->stack[1]);
        HW_GE_OADR = curStack->stack[2];
        HW_GE_OADR1 = curStack->stack[3];
        HW_GE_OADR2 = curStack->stack[4];
        _sceGeSetBaseRadr(curStack->stack[7], curStack->stack[5],
                          curStack->stack[6]);
        HW_GE_EXEC = curStack->stack[0] | 1;
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
                newLoc = HW_GE_OADR + (u32 *) off;

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
            HW_GE_LISTADDR = (int)lastCmdPtr1;
            HW_GE_EXEC |= 1;
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
                newLoc = HW_GE_OADR + (u32 *) off;

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
            HW_GE_LISTADDR = (int)lastCmdPtr1;
            HW_GE_EXEC |= 1;
            break;
        }

    case GE_SIGNAL_BREAK1:
    case GE_SIGNAL_BREAK2:
        {
            // 3B08
            if (g_deci2p == NULL) {
                HW_GE_EXEC |= 1;
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
    dbg_printf("sceGeListDeQueue(%08x)\n", dlId);
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
                g_GeLogHandler(1, dlId);
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
    dbg_printf("sceGeListSync(%08x, %d)\n", dlId, mode);
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
    dbg_printf(" -> sceGeListSync(%08x, %d) = %08x\n", dlId, mode, ret);
    return ret;
}

/**
 * Wait for drawing to end
 */
SceGeListState sceGeDrawSync(int syncType)
{
    dbg_printf("sceGeDrawSync(%d)\n", syncType);
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

    dbg_printf("-> sceGeDrawSync(%d) = %08x\n", syncType, ret);
    // 3F24
    pspSetK1(oldK1);
    return ret;
}

int sceGeBreak(u32 resetQueue, void *arg1)
{
    dbg_printf("sceGeBreak\n");
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
            HW_GE_INTERRUPT_TYPE4 = SCE_GE_INTEND | SCE_GE_INTFIN;
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
        g_GeLogHandler(3, resetQueue);
    sceKernelCpuResumeIntr(oldIntr);
    pspSetK1(oldK1);
    return ret;
}

int sceGeContinue()
{
    dbg_printf("sceGeContinue\n");
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
                    HW_GE_INTERRUPT_TYPE4 = SCE_GE_INTEND | SCE_GE_INTFIN;
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
                    HW_GE_INTERRUPT_TYPE4 = SCE_GE_INTEND | SCE_GE_INTFIN;
                    HW_GE_EXEC = dl->flags | 1;
                    pspSync();
                    g_AwQueue.curRunning = dl;
                    sceKernelClearEventFlag(g_AwQueue.drawingEvFlagId, ~2);
                    if (g_GeLogHandler != NULL) {
                        g_GeLogHandler(4, 0);
                        if (g_GeLogHandler != NULL)
                            g_GeLogHandler(5, (int)dl ^ g_dlMask, dl->list, dl->stall);
                    }
                } else {
                    // 45A8
                    ret = SCE_ERROR_BUSY;
                }
            } else {
                dl->state = SCE_GE_DL_STATE_QUEUED;
                if (g_GeLogHandler != NULL)
                    g_GeLogHandler(4, 0);
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
    dbg_printf("sceSetCallback(%08x)\n", (u32)cb);
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

int sceGePutBreakpoint(int *inPtr, int size)
{
    dbg_printf("sceGePutBreakpoint\n");
    if (size >= 9)
        return SCE_ERROR_INVALID_SIZE;
    int oldIntr = sceKernelCpuSuspendIntr();
    int oldK1 = pspShiftK1();
    if (!pspK1DynBufOk(inPtr, size * 8)) {
        // 4850 dup
        pspSetK1(oldK1);
        sceKernelCpuResumeIntr(oldIntr);
        return SCE_ERROR_PRIV_REQUIRED;
    }
    // 477C
    int i;
    for (i = 0; i < size; i++) {
        int (*func) (int, int, int) = (void *)*(int *)(g_deci2p + 36);
        if (func(inPtr[i * 2], 4, 3) < 0) {
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
    dbg_printf("sceGeGetBreakpoint\n");
    int oldIntr = sceKernelCpuSuspendIntr();
    int oldK1 = pspShiftK1();
    if (!pspK1DynBufOk(outPtr, size * 8) || !pspK1PtrOk(arg2)) {
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
    dbg_printf("sceGeListIdList\n");
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
    dbg_printf("sceGeGetList\n");
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
    dbg_printf("sceGeGetStack\n");
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
    dbg_printf("sceGeDebugBreak\n");
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
    dbg_printf("sceGeDebugContinue\n");
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
    if ((HW_GE_INTERRUPT_TYPE1 & SCE_GE_INTEND) != 0) {
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
    if ((curCmd >> 24) == SCE_GE_CMD_SIGNAL && ((curCmd >> 16) & 0xFF) == GE_SIGNAL_BREAK2) // 5044
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
    dbg_printf("sceGeUnsetCallback(%08x)\n", cbId);
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
                HW_GE_CTRL =
                    (((int)ptr2 | 0x08000000) & 0x0FFFFFC0) | 0xA0000001;
                HW_GE_CTRL;
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
                HW_GE_CTRL =
                    (((int)ptr | 0x08000000) & 0x0FFFFFC0) | 0xA0000001;
                HW_GE_CTRL;
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
            HW_GE_CTRL =
                (((int)out | 0x08000000) & 0x0FFFFFC0) | 0xA0000001;
            HW_GE_CTRL;
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
            HW_GE_CTRL =
                (((int)out | 0x08000000) & 0x0FFFFFC0) | 0xA0000001;
            HW_GE_CTRL;
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

u32 _tmp_base = 0;
void print_list(void *list, void *stall)
{
    u32 *curCmd;
    for (curCmd = list; (void*)curCmd < stall || stall == NULL; curCmd++) {
        dbg_printf("- cmd %08x [%s %06x]\n", *curCmd, _dbg_cmds[*curCmd >> 24], *curCmd & 0xFFFFFF);
        if (*curCmd >> 24 == 0x10) { // base
            _tmp_base = *curCmd & 0xFFFFFF;
        }
        if (*curCmd >> 24 == 0x0A) { // call
            void *sublist = (void*)((_tmp_base << 8) | (*curCmd & 0xFFFFFF));
            dbg_printf("==sublist beg at %08x==\n", sublist);
            print_list(sublist, NULL);
            dbg_printf("==sublist end==\n");
        }
        if (*curCmd >> 24 == 0x0C) {
            break;
        }
        if (*curCmd >> 24 == 0x0B) {
            break;
        }
    }
}

int _sceGeListEnQueue(void *list, void *stall, int cbid, SceGeListArgs *arg, int head)
{
    dbg_printf("Enqueue list %08x, stall %08x, cbid %d, args %08x, head %d\n", list, stall, cbid, arg, head);
    print_list(list, stall);
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
    dbg_printf("Enqueued with dlId %08x [list=%d]\n", dlId, idx);
    return dlId;
}

