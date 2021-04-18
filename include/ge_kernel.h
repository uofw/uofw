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
#include "ge_user.h"

/** @defgroup GE sceGE_Manager Module
 *
 * @{
 */

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
    u8 state; // SceGeDisplayListState / 8
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
 * Sets a command (?).
 *
 * @param cmdOff The command ID.
 * @param cmd The value to set the command to.
 *
 * @return Zero on success, otherwise less than zero.
 */
int sceGeSetCmd(u32 cmdOff, u32 cmd);

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
 * Gets the EDRAM physical size.
 *
 * @return The EDRAM physical size.
 */
int sceGeEdramGetHwSize();

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

/** @} */

