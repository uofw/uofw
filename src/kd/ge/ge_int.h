#ifndef GE_INT_H
#define GE_INT_H

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

#define SCE_GE_SIGNAL_ERROR_INVALID_ADDRESS 0
#define SCE_GE_SIGNAL_ERROR_STACK_OVERFLOW 1
#define SCE_GE_SIGNAL_ERROR_STACK_UNDERFLOW 2

// Maximum number of display lists which can be enqueued
#define MAX_COUNT_DL 64

/******************************/

int _sceGeReset();
s32 _sceGeInitCallback3(void *arg0, s32 arg1, void *arg2);
s32 _sceGeInitCallback4();
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
    // Next display list planned to be executed (can be the current one, or not, in case another one was just inserted before the running one stopped)
    SceGeDisplayList *active_first;  //  8
    SceGeDisplayList *active_last;   // 12
    SceGeDisplayList *free_first;    // 16
    SceGeDisplayList *free_last;     // 20
    SceUID drawingEvFlagId;     // 24
    SceUID listEvFlagIds[2];    // 28, 32
    SceGeStack stack[32];       // 36
    // The SDK version, since some variations of the behaviors happened with firmware updates
    int sdkVer;                 // 1060
    // Used by Itadaki Street Portable: add a dcache writeback when enqueueing the firts display list
    int cachePatch;             // 1064
    // Patch for Gensou Suikoden I&II: the syscall number for sceGeListUpdateStallAddr()
    int syscallId;
    // Patch for Gensou Suikoden I&II: the lazy list stall address update data
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

// Structure containing a single breakpoint
typedef struct {
    // The address to break on. Lower bits are set if we want to skip writing the breakpoint on the next occurrence, but are discarded afterwards.
    u32 addr; // 0
    // Number of times we want to break (-1 = infinite)
    int count; // 4
    // The two commands which the SIGNAL/END pair replaced
    int oldCmd1; // 8
    int oldCmd2; // 12
} SceGeBpCmd;

typedef struct {
    // reached a break state (through signal or sceGeBreak())
    int inBreakState;
    // breakpoints are enabled/written in the display list's command list
    int bpSet;
    // Number of breakpoints
    int numBp;
    // Number of additional breakpoints used for step-by-step breaking
    int numStepBp;
    // List of breakpoints
    SceGeBpCmd bpCmds[8];
    // List of additional breakpoints used for step-by-step breaking
    SceGeBpCmd stepBpCmds[2];
} SceGeBpCtrl;

typedef struct {
    char *name;
    u32 *ptr;
} SadrUpdate;

typedef struct {
    u8 initCmd;
    u8 initCmdArg;
    u8 setCmd;
    u8 size;
} SceGeMatrix;

#endif
