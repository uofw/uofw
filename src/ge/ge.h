/* Copyright (C) 2011 The uOFW team
   See the file COPYING for copying permission.
*/

/**
 * @author artart78
 * @version 6.60
 *
 * The ge.prx module RE'ing.
 */

/** @defgroup GE sceGE_Manager Module
 *
 * @{
 */

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
    struct SceGeDisplayList *next;
    /** Previous display list */
    struct SceGeDisplayList *prev;
    /** Current display list state */
    SceGeDisplayListState state;
    /** Current display list received signal */
    SceGeDisplayListSignal signal;
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
    int unk28;
    /** Internal data */
    int unk32;
    /** Internal data */
    int unk36;
    /** Internal data */
    int unk40;
    /** Internal data */
    int unk44;
    /** Internal data */
    int unk48;
    /** The callbacks id set with sceGeSetCallback() */
    short cbId; // 52
    /** Some argument passed to the interrupt handler when calling the subintrs */
    u16 unk54;
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
int sceGeBreak(u32 resetQueue, int arg1);

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

