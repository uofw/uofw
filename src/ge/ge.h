typedef struct
{
    int unk0, unk4, unk8, unk12, unk16, unk20, unk24, unk28;
} SceGeStack;

typedef struct
{
    int unk0, unk4, unk8, unk12, unk16, unk20, unk24, unk28, unk32, unk36, unk40, unk44, unk48, unk52, unk56, unk60, unk64;
    int cmds[495];
} SceGeContext;

typedef struct SceGeDisplayList
{
    struct SceGeDisplayList *next;
    struct SceGeDisplayList *prev;
    u8 unk8, unk9, unk10;
    char unused11;
    SceGeContext *ctx; // 12
    int unk16;
    void *list; // 20
    void *stall; // 24
    int unk28, unk32, unk36, unk40, unk44, unk48;
    short cbId; // 52
    u16 unk54;
    short stackSize; // 56
    u16 stackOff; // 58
    SceGeStack *stack; // 60
} SceGeDisplayList; // size: 64

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

typedef struct
{
    u32 size;
    SceGeContext *ctx;
    u32 unk8;
    void *unk12;
} SceGeListArgs;

int sceGeListUpdateStallAddr(int dlId, void *stall);
int sceGeInit();
int sceGeEnd();
int sceGeGetReg(u32 regId);
int sceGeSetReg(u32 regId, u32 value);
int sceGeGetCmd(u32 cmdOff);
int sceGeSetCmd(u32 cmdOff, u32 cmd);
int sceGeGetMtx(int id, int *mtx);
int sceGeSetMtx(int id, int *mtx);
int sceGeSaveContext(SceGeContext *ctx);
int sceGeRestoreContext(SceGeContext *ctx);
int sceGeRegisterLogHandler(void (*handler)());
int sceGeSetGeometryClock(int opt);
int sceGeEdramInit();
int sceGeEdramSetRefreshParam(int arg0, int arg1, int arg2, int arg3);
int sceGeEdramSetSize(int size);
int sceGeEdramGetAddr();
int sceGeEdramSetAddrTranslation(int arg);
int sceGeEdramGetSize();
int sceGeEdramGetHwSize();
int sceGeListDeQueue(int dlId);
int sceGeListSync(int dlId, int mode);
int sceGeDrawSync(int syncType);
int sceGeBreak(u32 resetQueue, int arg1);
int sceGeContinue();
int sceGeSetCallback(SceGeCallbackData *cb);
int sceGePutBreakpoint(int *inPtr, int size);
int sceGeGetBreakpoint(int *outPtr, int size, int *arg2);
int sceGeGetListIdList(int *outPtr, int size, int *totalCountPtr);
int sceGeGetList(int dlId, SceGeDisplayList *outDl, int *outFlag);
int sceGeGetStack(int stackId, SceGeStack *stack);
int sceGeListEnQueue(void *list, void *stall, int cbid, SceGeListArgs *arg);
int sceGeListEnQueueHead(void *list, void *stall, int cbid, SceGeListArgs *arg);
int sceGeUnsetCallback(int cbId);

