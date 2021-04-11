/* Copyright (C) 2011, 2012 The uOFW team
   See the file COPYING for copying permission.
*/

/**
 * @author artart78
 * @version 6.60
 *
 * The ge.prx module RE'ing.
 */

#include "common_header.h"

/** @defgroup GE sceGE_Manager Module
 *
 * @{
 */

/*
 * List of commands accepted by the GE.
 */
#define SCE_GE_CMD_NOP 0x00
#define SCE_GE_CMD_VADR 0x01
#define SCE_GE_CMD_IADR 0x02
#define SCE_GE_CMD_PRIM 0x04
#define SCE_GE_CMD_BEZIER 0x05
#define SCE_GE_CMD_SPLINE 0x06
#define SCE_GE_CMD_BBOX 0x07
#define SCE_GE_CMD_JUMP 0x08
#define SCE_GE_CMD_BJUMP 0x09
#define SCE_GE_CMD_CALL 0x0A
#define SCE_GE_CMD_RET 0x0B
#define SCE_GE_CMD_END 0x0C
#define SCE_GE_CMD_SIGNAL 0x0E
#define SCE_GE_CMD_FINISH 0x0F
#define SCE_GE_CMD_BASE 0x10
#define SCE_GE_CMD_VTYPE 0x12
#define SCE_GE_CMD_OFFSET 0x13
#define SCE_GE_CMD_ORIGIN 0x14
#define SCE_GE_CMD_REGION1 0x15
#define SCE_GE_CMD_REGION2 0x16
#define SCE_GE_CMD_LTE 0x17
#define SCE_GE_CMD_LE0 0x18
#define SCE_GE_CMD_LE1 0x19
#define SCE_GE_CMD_LE2 0x1A
#define SCE_GE_CMD_LE3 0x1B
#define SCE_GE_CMD_CLE 0x1C
#define SCE_GE_CMD_BCE 0x1D
#define SCE_GE_CMD_TME 0x1E
#define SCE_GE_CMD_FGE 0x1F
#define SCE_GE_CMD_DTE 0x20
#define SCE_GE_CMD_ABE 0x21
#define SCE_GE_CMD_ATE 0x22
#define SCE_GE_CMD_ZTE 0x23
#define SCE_GE_CMD_STE 0x24
#define SCE_GE_CMD_AAE 0x25
#define SCE_GE_CMD_PCE 0x26
#define SCE_GE_CMD_CTE 0x27
#define SCE_GE_CMD_LOE 0x28
#define SCE_GE_CMD_BONEN 0x2A
#define SCE_GE_CMD_BONED 0x2B
#define SCE_GE_CMD_WEIGHT0 0x2C
#define SCE_GE_CMD_WEIGHT1 0x2D
#define SCE_GE_CMD_WEIGHT2 0x2E
#define SCE_GE_CMD_WEIGHT3 0x2F
#define SCE_GE_CMD_WEIGHT4 0x30
#define SCE_GE_CMD_WEIGHT5 0x31
#define SCE_GE_CMD_WEIGHT6 0x32
#define SCE_GE_CMD_WEIGHT7 0x33
#define SCE_GE_CMD_DIVIDE 0x36
#define SCE_GE_CMD_PPM 0x37
#define SCE_GE_CMD_PFACE 0x38
#define SCE_GE_CMD_WORLDN 0x3A
#define SCE_GE_CMD_WORLDD 0x3B
#define SCE_GE_CMD_VIEWN 0x3C
#define SCE_GE_CMD_VIEWD 0x3D
#define SCE_GE_CMD_PROJN 0x3E
#define SCE_GE_CMD_PROJD 0x3F
#define SCE_GE_CMD_TGENN 0x40
#define SCE_GE_CMD_TGEND 0x41
#define SCE_GE_CMD_SX 0x42
#define SCE_GE_CMD_SY 0x43
#define SCE_GE_CMD_SZ 0x44
#define SCE_GE_CMD_TX 0x45
#define SCE_GE_CMD_TY 0x46
#define SCE_GE_CMD_TZ 0x47
#define SCE_GE_CMD_SU 0x48
#define SCE_GE_CMD_SV 0x49
#define SCE_GE_CMD_TU 0x4A
#define SCE_GE_CMD_TV 0x4B
#define SCE_GE_CMD_OFFSETX 0x4C
#define SCE_GE_CMD_OFFSETY 0x4D
#define SCE_GE_CMD_SHADE 0x50
#define SCE_GE_CMD_NREV 0x51
#define SCE_GE_CMD_MATERIAL 0x53
#define SCE_GE_CMD_MEC 0x54
#define SCE_GE_CMD_MAC 0x55
#define SCE_GE_CMD_MDC 0x56
#define SCE_GE_CMD_MSC 0x57
#define SCE_GE_CMD_MAA 0x58
#define SCE_GE_CMD_MK 0x5B
#define SCE_GE_CMD_AC 0x5C
#define SCE_GE_CMD_AA 0x5D
#define SCE_GE_CMD_LMODE 0x5E
#define SCE_GE_CMD_LTYPE0 0x5F
#define SCE_GE_CMD_LTYPE1 0x60
#define SCE_GE_CMD_LTYPE2 0x61
#define SCE_GE_CMD_LTYPE3 0x62
#define SCE_GE_CMD_LX0 0x63
#define SCE_GE_CMD_LY0 0x64
#define SCE_GE_CMD_LZ0 0x65
#define SCE_GE_CMD_LX1 0x66
#define SCE_GE_CMD_LY1 0x67
#define SCE_GE_CMD_LZ1 0x68
#define SCE_GE_CMD_LX2 0x69
#define SCE_GE_CMD_LY2 0x6A
#define SCE_GE_CMD_LZ2 0x6B
#define SCE_GE_CMD_LX3 0x6C
#define SCE_GE_CMD_LY3 0x6D
#define SCE_GE_CMD_LZ3 0x6E
#define SCE_GE_CMD_LDX0 0x6F
#define SCE_GE_CMD_LDY0 0x70
#define SCE_GE_CMD_LDZ0 0x71
#define SCE_GE_CMD_LDX1 0x72
#define SCE_GE_CMD_LDY1 0x73
#define SCE_GE_CMD_LDZ1 0x74
#define SCE_GE_CMD_LDX2 0x75
#define SCE_GE_CMD_LDY2 0x76
#define SCE_GE_CMD_LDZ2 0x77
#define SCE_GE_CMD_LDX3 0x78
#define SCE_GE_CMD_LDY3 0x79
#define SCE_GE_CMD_LDZ3 0x7A
#define SCE_GE_CMD_LKA0 0x7B
#define SCE_GE_CMD_LKB0 0x7C
#define SCE_GE_CMD_LKC0 0x7D
#define SCE_GE_CMD_LKA1 0x7E
#define SCE_GE_CMD_LKB1 0x7F
#define SCE_GE_CMD_LKC1 0x80
#define SCE_GE_CMD_LKA2 0x81
#define SCE_GE_CMD_LKB2 0x82
#define SCE_GE_CMD_LKC2 0x83
#define SCE_GE_CMD_LKA3 0x84
#define SCE_GE_CMD_LKB3 0x85
#define SCE_GE_CMD_LKC3 0x86
#define SCE_GE_CMD_LKS0 0x87
#define SCE_GE_CMD_LKS1 0x88
#define SCE_GE_CMD_LKS2 0x89
#define SCE_GE_CMD_LKS3 0x8A
#define SCE_GE_CMD_LKO0 0x8B
#define SCE_GE_CMD_LKO1 0x8C
#define SCE_GE_CMD_LKO2 0x8D
#define SCE_GE_CMD_LKO3 0x8E
#define SCE_GE_CMD_LAC0 0x8F
#define SCE_GE_CMD_LDC0 0x90
#define SCE_GE_CMD_LSC0 0x91
#define SCE_GE_CMD_LAC1 0x92
#define SCE_GE_CMD_LDC1 0x93
#define SCE_GE_CMD_LSC1 0x94
#define SCE_GE_CMD_LAC2 0x95
#define SCE_GE_CMD_LDC2 0x96
#define SCE_GE_CMD_LSC2 0x97
#define SCE_GE_CMD_LAC3 0x98
#define SCE_GE_CMD_LDC3 0x99
#define SCE_GE_CMD_LSC3 0x9A
#define SCE_GE_CMD_CULL 0x9B
#define SCE_GE_CMD_FBP 0x9C
#define SCE_GE_CMD_FBW 0x9D
#define SCE_GE_CMD_ZBP 0x9E
#define SCE_GE_CMD_ZBW 0x9F
#define SCE_GE_CMD_TBP0 0xA0
#define SCE_GE_CMD_TBP1 0xA1
#define SCE_GE_CMD_TBP2 0xA2
#define SCE_GE_CMD_TBP3 0xA3
#define SCE_GE_CMD_TBP4 0xA4
#define SCE_GE_CMD_TBP5 0xA5
#define SCE_GE_CMD_TBP6 0xA6
#define SCE_GE_CMD_TBP7 0xA7
#define SCE_GE_CMD_TBW0 0xA8
#define SCE_GE_CMD_TBW1 0xA9
#define SCE_GE_CMD_TBW2 0xAA
#define SCE_GE_CMD_TBW3 0xAB
#define SCE_GE_CMD_TBW4 0xAC
#define SCE_GE_CMD_TBW5 0xAD
#define SCE_GE_CMD_TBW6 0xAE
#define SCE_GE_CMD_TBW7 0xAF
#define SCE_GE_CMD_CBP 0xB0
#define SCE_GE_CMD_CBW 0xB1
#define SCE_GE_CMD_XBP1 0xB2
#define SCE_GE_CMD_XBW1 0xB3
#define SCE_GE_CMD_XBP2 0xB4
#define SCE_GE_CMD_XBW2 0xB5
#define SCE_GE_CMD_TSIZE0 0xB8
#define SCE_GE_CMD_TSIZE1 0xB9
#define SCE_GE_CMD_TSIZE2 0xBA
#define SCE_GE_CMD_TSIZE3 0xBB
#define SCE_GE_CMD_TSIZE4 0xBC
#define SCE_GE_CMD_TSIZE5 0xBD
#define SCE_GE_CMD_TSIZE6 0xBE
#define SCE_GE_CMD_TSIZE7 0xBF
#define SCE_GE_CMD_TMAP 0xC0
#define SCE_GE_CMD_TSHADE 0xC1
#define SCE_GE_CMD_TMODE 0xC2
#define SCE_GE_CMD_TPF 0xC3
#define SCE_GE_CMD_CLOAD 0xC4
#define SCE_GE_CMD_CLUT 0xC5
#define SCE_GE_CMD_TFILTER 0xC6
#define SCE_GE_CMD_TWRAP 0xC7
#define SCE_GE_CMD_TLEVEL 0xC8
#define SCE_GE_CMD_TFUNC 0xC9
#define SCE_GE_CMD_TEC 0xCA
#define SCE_GE_CMD_TFLUSH 0xCB
#define SCE_GE_CMD_TSYNC 0xCC
#define SCE_GE_CMD_FOG1 0xCD
#define SCE_GE_CMD_FOG2 0xCE
#define SCE_GE_CMD_FC 0xCF
#define SCE_GE_CMD_TSLOPE 0xD0
#define SCE_GE_CMD_FPF 0xD2
#define SCE_GE_CMD_CMODE 0xD3
#define SCE_GE_CMD_SCISSOR1 0xD4
#define SCE_GE_CMD_SCISSOR2 0xD5
#define SCE_GE_CMD_MINZ 0xD6
#define SCE_GE_CMD_MAXZ 0xD7
#define SCE_GE_CMD_CTEST 0xD8
#define SCE_GE_CMD_CREF 0xD9
#define SCE_GE_CMD_CMSK 0xDA
#define SCE_GE_CMD_ATEST 0xDB
#define SCE_GE_CMD_STEST 0xDC
#define SCE_GE_CMD_SOP 0xDD
#define SCE_GE_CMD_ZTEST 0xDE
#define SCE_GE_CMD_BLEND 0xDF
#define SCE_GE_CMD_FIXA 0xE0
#define SCE_GE_CMD_FIXB 0xE1
#define SCE_GE_CMD_DITH1 0xE2
#define SCE_GE_CMD_DITH2 0xE3
#define SCE_GE_CMD_DITH3 0xE4
#define SCE_GE_CMD_DITH4 0xE5
#define SCE_GE_CMD_LOP 0xE6
#define SCE_GE_CMD_ZMSK 0xE7
#define SCE_GE_CMD_PMSK1 0xE8
#define SCE_GE_CMD_PMSK2 0xE9
#define SCE_GE_CMD_XSTART 0xEA
#define SCE_GE_CMD_XPOS1 0xEB
#define SCE_GE_CMD_XPOS2 0xEC
#define SCE_GE_CMD_XSIZE 0xEE
#define SCE_GE_CMD_X2 0xF0
#define SCE_GE_CMD_Y2 0xF1
#define SCE_GE_CMD_Z2 0xF2
#define SCE_GE_CMD_S2 0xF3
#define SCE_GE_CMD_T2 0xF4
#define SCE_GE_CMD_Q2 0xF5
#define SCE_GE_CMD_RGB2 0xF6
#define SCE_GE_CMD_AP2 0xF7
#define SCE_GE_CMD_F2 0xF8
#define SCE_GE_CMD_I2 0xF9

#define SCE_GE_MTX_BONEA 0
#define SCE_GE_MTX_BONEB 1
#define SCE_GE_MTX_BONEC 2
#define SCE_GE_MTX_BONED 3
#define SCE_GE_MTX_BONEE 4
#define SCE_GE_MTX_BONEF 5
#define SCE_GE_MTX_BONEG 6
#define SCE_GE_MTX_BONEH 7
#define SCE_GE_MTX_WORLD 8
#define SCE_GE_MTX_VIEW  9
#define SCE_GE_MTX_PROJ  10
#define SCE_GE_MTX_TGEN  11

/*
 * List of signals which can be sent using the SIGNAL command.
 */
#define SCE_GE_SIGNAL_HANDLER_SUSPEND  0x01
#define SCE_GE_SIGNAL_HANDLER_CONTINUE 0x02
#define SCE_GE_SIGNAL_HANDLER_PAUSE    0x03
#define SCE_GE_SIGNAL_SYNC             0x08
#define SCE_GE_SIGNAL_JUMP             0x10
#define SCE_GE_SIGNAL_CALL             0x11
#define SCE_GE_SIGNAL_RET              0x12
#define SCE_GE_SIGNAL_RJUMP            0x13
#define SCE_GE_SIGNAL_RCALL            0x14
#define SCE_GE_SIGNAL_OJUMP            0x15
#define SCE_GE_SIGNAL_OCALL            0x16

#define SCE_GE_SIGNAL_RTBP0            0x20
#define SCE_GE_SIGNAL_RTBP1            0x21
#define SCE_GE_SIGNAL_RTBP2            0x22
#define SCE_GE_SIGNAL_RTBP3            0x23
#define SCE_GE_SIGNAL_RTBP4            0x24
#define SCE_GE_SIGNAL_RTBP5            0x25
#define SCE_GE_SIGNAL_RTBP6            0x26
#define SCE_GE_SIGNAL_RTBP7            0x27
#define SCE_GE_SIGNAL_OTBP0            0x28
#define SCE_GE_SIGNAL_OTBP1            0x29
#define SCE_GE_SIGNAL_OTBP2            0x2A
#define SCE_GE_SIGNAL_OTBP3            0x2B
#define SCE_GE_SIGNAL_OTBP4            0x2C
#define SCE_GE_SIGNAL_OTBP5            0x2D
#define SCE_GE_SIGNAL_OTBP6            0x2E
#define SCE_GE_SIGNAL_OTBP7            0x2F
#define SCE_GE_SIGNAL_RCBP             0x30
#define SCE_GE_SIGNAL_OCBP             0x38
#define SCE_GE_SIGNAL_BREAK1           0xF0
#define SCE_GE_SIGNAL_BREAK2           0xFF

/** Structure storing a stack (for CALL/RET) */
typedef struct
{
    /** The stack buffer */
    unsigned int stack[8];
} SceGeStack;

/** Structure storing a GE context */
typedef struct
{
    /** The context buffer */
    unsigned int ctx[512];
} SceGeContext;

/** Typedef for a GE callback */
typedef void (*SceGeCallback)(int id, void *arg);

/** Structure to hold the callback data */
typedef struct
{
    /** GE callback for the signal interrupt */
    SceGeCallback signal_func;
    /** GE callback argument for signal interrupt */
    void *signal_arg;
    /** GE callback for the finish interrupt */
    SceGeCallback finish_func;
    /** GE callback argument for finish interrupt */
    void *finish_arg;
} SceGeCallbackData;

/** List of arguments when enqueueing a list */
typedef struct
{
    /** Size of the structure (16) */
    u32 size;
    /** Pointer to a context */
    SceGeContext *ctx;
    /** Number of stacks to use */
    u32 numStacks;
    /** Pointer to the stacks (unused) */
    SceGeStack *stacks;
} SceGeListArgs;

/** State of a display list, returned by sceGeListSync() and sceGeDrawSync() */
typedef enum
{
    /** The list has been completed */
    SCE_GE_LIST_COMPLETED,
    /** The list is queued but not executed yet */
    SCE_GE_LIST_QUEUED,
    /** The list is currently being executed */
    SCE_GE_LIST_DRAWING,
    /** The list was stopped because it encountered stall address */
    SCE_GE_LIST_STALLING,
    /** The list is paused because of a signal */
    SCE_GE_LIST_PAUSED
} SceGeListState;

/** State of a display list, internally */
typedef enum
{
    /** No state assigned, the list is empty */
    SCE_GE_DL_STATE_NONE = 0,
    /** The list has been queued */
    SCE_GE_DL_STATE_QUEUED, // 1
    /** The list is being executed */
    SCE_GE_DL_STATE_RUNNING, // 2
    /** The list was completed and will be removed */
    SCE_GE_DL_STATE_COMPLETED, // 3
    /** The list has been paused by a signal */
    SCE_GE_DL_STATE_PAUSED // 4
} SceGeDisplayListState;

/** Signal state of a display list */
typedef enum
{
    /** No signal received */
    SCE_GE_DL_SIGNAL_NONE = 0,
    /** The break signal was received */
    SCE_GE_DL_SIGNAL_BREAK, // 1
    /** The pause signal was received */
    SCE_GE_DL_SIGNAL_PAUSE, // 2
    /** The sync signal was received */
    SCE_GE_DL_SIGNAL_SYNC // 3
} SceGeDisplayListSignal;

/** Structure holding a display list */
typedef struct SceGeDisplayList
{
    /** Next display list of the queue */
    struct SceGeDisplayList *next; // 0
    /** Previous display list */
    struct SceGeDisplayList *prev; // 4
    /** Current display list state */
    u8 state; // SceGeDisplayListSignal / 8
    /** Current display list received signal */
    u8 signal; // SceGeDisplayListSignal / 9
    /** 1 if context is up to date, 0 otherwise */
    u8 ctxUpToDate;
    /* (padding) */
    char unused11;
    /** The display list context */
    SceGeContext *ctx; // 12
    /** The display list flags */
    int flags;
    /** Pointer to the list of commands */
    void *list; // 20
    /** Pointer to the stall address, where the display list will stop being executed */
    void *stall; // 24
    /** Internal data */
    int radr1; // 28
    /** Internal data */
    int radr2; // 32
    /** Internal data */
    int oadr; // 36
    /** Internal data */
    int oadr1; // 40
    /** Internal data */
    int oadr2; // 44
    /** Internal data */
    int base; // 48
    /** The callbacks id set with sceGeSetCallback() */
    short cbId; // 52
    /** Some argument passed to the interrupt handler when calling the subintrs */
    u16 signalData;
    /** The number of stacks of the display list */
    short numStacks; // 56
    /** The offset of the current stack */
    u16 stackOff; // 58
    /** A pointer to the list of stacks */
    SceGeStack *stack; // 60
} SceGeDisplayList; // size: 64

/**
 * Updates the stall address.
 *
 * @param dlId The ID of the display list whose stall address will be modified
 *
 * @return Zero on success, otherwise less than zero.
 */
int sceGeListUpdateStallAddr(int dlId, void *stall);

/**
 * Inits the GE subsystem.
 *
 * @return Zero.
 */
int sceGeInit();

/**
 * Ends the GE subsystem.
 *
 * @return Zer.o
 */
int sceGeEnd();

typedef enum SceGeReg {
    SCE_GE_REG_RESET = 0,
    SCE_GE_REG_UNK004 = 1,
    SCE_GE_REG_EDRAM_HW_SIZE = 2,
    SCE_GE_REG_EXEC = 3,
    SCE_GE_REG_UNK104 = 4,
    SCE_GE_REG_LISTADDR = 5,
    SCE_GE_REG_STALLADDR = 6,
    SCE_GE_REG_RADR1 = 7,
    SCE_GE_REG_RADR2 = 8,
    SCE_GE_REG_VADR = 9,
    SCE_GE_REG_IADR = 10,
    SCE_GE_REG_OADR = 11,
    SCE_GE_REG_OADR1 = 12,
    SCE_GE_REG_OADR2 = 13,
    SCE_GE_REG_UNK300 = 14,
    SCE_GE_REG_INTERRUPT_TYPE1 = 15,
    SCE_GE_REG_INTERRUPT_TYPE2 = 16,
    SCE_GE_REG_INTERRUPT_TYPE3 = 17,
    SCE_GE_REG_INTERRUPT_TYPE4 = 18,
    SCE_GE_REG_EDRAM_ENABLED_SIZE = 19,
    SCE_GE_REG_GEOMETRY_CLOCK = 20,
    SCE_GE_REG_EDRAM_REFRESH_UNK1 = 21,
    SCE_GE_REG_EDRAM_UNK10 = 22,
    SCE_GE_REG_EDRAM_REFRESH_UNK2 = 23,
    SCE_GE_REG_EDRAM_REFRESH_UNK3 = 24,
    SCE_GE_REG_EDRAM_UNK40 = 25,
    SCE_GE_REG_EDRAM_UNK50 = 26,
    SCE_GE_REG_EDRAM_UNK60 = 27,
    SCE_GE_REG_EDRAM_ADDR_TRANS_DISABLE = 28,
    SCE_GE_REG_EDRAM_ADDR_TRANS_VALUE = 29,
    SCE_GE_REG_EDRAM_UNK90 = 30,
    SCE_GE_REG_EDRAM_UNKA0 = 31
} SceGeReg;

/**
 * Gets a GE hardware register.
 *
 * @param regId The register ID.
 *
 * @return The content of the register on success, otherwise less than zero.
 */
int sceGeGetReg(u32 regId);

/**
 * Sets a GE hardware register.
 *
 * @param regId The register ID.
 * @param value The value to set the register to.
 *
 * @return Zero on success, otherwise less than zero.
 */
int sceGeSetReg(u32 regId, u32 value);

/**
 * Gets a command (?).
 *
 * @param cmdOff The command ID.
 *
 * @return The command on success, otherwise less than zero.
 */
int sceGeGetCmd(u32 cmdOff);

/**
 * Sets a command (?).
 *
 * @param cmdOff The command ID.
 * @param cmd The value to set the command to.
 *
 * @return Zero on success, otherwise less than zero.
 */
int sceGeSetCmd(u32 cmdOff, u32 cmd);

/**
 * Gets a matrix.
 *
 * @param id The matrix ID (0 - 11)
 * @param mtx A buffer to store the matrix.
 *
 * @return Zero on success, otherwise less than zero.
 */
int sceGeGetMtx(int id, int *mtx);

/**
 * Sets a matrix.
 *
 * @param id The matrix ID (0 - 11)
 * @param mtx The buffer storing the matrix.
 *
 * @return Zero on success, otherwise less than zero.
 */
int sceGeSetMtx(int id, int *mtx);

/**
 * Saves the current GE context into a structure.
 *
 * @param ctx The structure to save the GE context in.
 *
 * @return Zero on success, otherwise less than zero.
 */
int sceGeSaveContext(SceGeContext *ctx);

/**
 * Restores a context from a structure.
 *
 * @param ctx The structure to load the GE context from.
 *
 * @return Zero on success, otherwise less than zero.
 */
int sceGeRestoreContext(SceGeContext *ctx);

/**
 * Registers a logging handler.
 *
 * @param handler The handler function.
 *
 * @return Zero.
 */
int sceGeRegisterLogHandler(void (*handler)());

/**
 * Sets or unsets the geometry clock.
 *
 * @param opt The value whose first bit enables or disables the geometry clock.
 *
 * @return The old state.
 */
int sceGeSetGeometryClock(int opt);

/**
 * Inits the EDRAM memory.
 *
 * @return Zero.
 */
int sceGeEdramInit();

/**
 * Sets the EDRAM refresh parameters.
 *
 * @param arg0 Unknown (0 or 1).
 * @param arg1 Unknown (0 to 0x7FFFFF).
 * @param arg2 Unknown (0 to 0x3FF).
 * @param arg3 Unknown (0 to 0xF).
 *
 * @return Zero on success, otherwise less than zero.
 */
int sceGeEdramSetRefreshParam(int arg0, int arg1, int arg2, int arg3);

/**
 * Sets the EDRAM size.
 *
 * @param size The size (0x200000 or 0x400000).
 *
 * @return Zero on success, otherwise less than zero.
 */
int sceGeEdramSetSize(int size);

/**
 * Gets the EDRAM address.
 *
 * @return 0x04000000.
 */
int sceGeEdramGetAddr();

/**
 * Sets the EDRAM address translation.
 *
 * @param arg The memory width (0, 0x200, 0x400, 0x800 or 0x1000).
 *
 * @return The previous memory width.
 */
int sceGeEdramSetAddrTranslation(int arg);

/**
 * Gets the EDRAM size, set with sceGeEdramSetSize().
 *
 * @return The EDRAM size.
 */
int sceGeEdramGetSize();

/**
 * Gets the EDRAM physical size.
 *
 * @return The EDRAM physical size.
 */
int sceGeEdramGetHwSize();

/**
 * Dequeues a list.
 *
 * @param dlId The ID of the display list to dequeue.
 *
 * @return Zero on success, otherwise less than zero.
 */
int sceGeListDeQueue(int dlId);

/**
 * Peeks a list state, or waits for it to be completed.
 *
 * @param dlId The ID of the display list to check.
 * @param mode 0 to wait for the display list to be completed, or 1 to check its current state.
 *
 * @return On success, SCE_GE_LIST_COMPLETED if mode is 0, one of the values of SceGeListState if mode is 1, and otherwise, less than zero.
 */
SceGeListState sceGeListSync(int dlId, int mode);

/**
 * Peeks a drawing state, or waits for the drawing to be completed.
 *
 * @param syncType 0 to wait for the drawing to be completed, or to check its current state.
 *
 * @return On success, SCE_GE_LIST_COMPLETED if mode is 0, SCE_GE_LIST_STALLING, SCE_GE_LIST_DRAWING or SCE_GE_LIST_COMPLETED if mode is 1, and otherwise, less than zero.
 */
SceGeListState sceGeDrawSync(int syncType);

/**
 * Stop the GE drawing.
 *
 * @param resetQueue If not set to 0, the display list queue will be emptied.
 * @param arg1 Unused pointer.
 *
 * @return The stopped display list ID on success, otherwise less than zero.
 */
int sceGeBreak(u32 resetQueue, void *arg1);

/**
 * Continues the queue execution after a sceGeBreak().
 *
 * @return Zero on success, otherwise less than zero.
 */
int sceGeContinue();

/**
 * Sets GE finish/signal callbacks.
 *
 * @param cb The GE callbacks parameters.
 *
 * @return The callbacks id on success, otherwise less than zero.
 */
int sceGeSetCallback(SceGeCallbackData *cb);

/**
 * Puts a breakpoint (used for debugging).
 *
 * @param inPtr A list of breakpoints, each one using 2 ints: one for the breakpoint address, and another one for the number of stops to do at the specified address.
 * @param size The number of breakpoints to set.
 *
 * @return Zero on success, otherwise less than zero.
 */
int sceGePutBreakpoint(int *inPtr, int size);

/**
 * Gets a breakpoint.
 *
 * @param outPtr The list of breakpoints (check sceGePutBreakpoint()).
 * @param size The number of breakpoints to read.
 * @param arg2 A pointer where will be stored the total number of breakpoints.
 *
 * @return The number of stored breakpoints on success, otherwise less than zero.
 */
int sceGeGetBreakpoint(int *outPtr, int size, int *arg2);

/**
 * Gets a list of the IDs of the display lists currently being in the queue.
 *
 * @param outPtr A buffer that will store the display lists' ID.
 * @param size The number of IDs to store.
 * @param totalCountPtr A point where will be stored the total number of display lists.
 *
 * @return The number of stored list IDs on success, otherwise less than zero.
 */
int sceGeGetListIdList(int *outPtr, int size, int *totalCountPtr);

/**
 * Gets a display list from its ID.
 *
 * @param dlId The display list ID.
 * @param outDl A pointer where the display list will be stored.
 * @param outFlag A pointer where will be stored (outDl->state << 2) | outDl->signal.
 *
 * @return Zero on success, otherwise less than zero.
 */
int sceGeGetList(int dlId, SceGeDisplayList *outDl, int *outFlag);

/**
 * Gets a stack from the current display list, using its ID.
 *
 * @param stackId The stack ID.
 * @param stack A pointer where the stack will be stored.
 *
 * @return The current stack of the display list on success, otherwise less than zero.
 */
int sceGeGetStack(int stackId, SceGeStack *stack);

/**
 * Enqueues a display list at the end of the queue.
 *
 * @param list A pointer to the list of commands.
 * @param stall The address where the display list will stall.
 * @param cbid The callback ID, returned by sceGeSetCallback(), of the callbacks to be used.
 * @param arg A structure storing arguments about the list.
 *
 * @return The display list ID on success, otherwise less than zero.
 */
int sceGeListEnQueue(void *list, void *stall, int cbid, SceGeListArgs *arg);

/**
 * Enqueues a display list as the next display list that will be executed.
 *
 * @param list A pointer to the list of commands.
 * @param stall The address where the display list will stall.
 * @param cbid The callback ID, returned by sceGeSetCallback(), of the callbacks to be used.
 * @param arg A structure storing arguments about the list.
 *
 * @return The display list ID on success, otherwise less than zero.
 */
int sceGeListEnQueueHead(void *list, void *stall, int cbid, SceGeListArgs *arg);

/**
 * Unsets GE callbacks.
 *
 * @param cbId The ID of the callbacks to unset.
 *
 * @return Zero on success, otherwise less than zero.
 */
int sceGeUnsetCallback(int cbId);

/** @} */

