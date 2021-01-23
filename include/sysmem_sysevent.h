/* Copyright (C) The uOFW team
   See the file COPYING for copying permission.
*/

#include "common_header.h"

enum SceSysEventTypes {
    SCE_SUSPEND_EVENTS = 0x0000FF00,
    SCE_RESUME_EVENTS = 0x00FF0000,
    SCE_SPEED_CHANGE_EVENTS = 0x01000000
};

/* PSP suspend events */

#define SCE_SYSTEM_SUSPEND_EVENT_QUERY              0x100
#define SCE_SYSTEM_SUSPEND_EVENT_QUERY_DENIED       0x101
#define SCE_SYSTEM_SUSPEND_EVENT_QUERY_GREENLIT     0x102

/* Different phases can be used to prioritize work. Phase2_16 is raised first, Phase2_0 last. */

#define SCE_SYSTEM_SUSPEND_EVENT_PHASE2_0           0x200
#define SCE_SYSTEM_SUSPEND_EVENT_PHASE2_1           0x201
#define SCE_SYSTEM_SUSPEND_EVENT_PHASE2_2           0x202
#define SCE_SYSTEM_SUSPEND_EVENT_PHASE2_3           0x203
#define SCE_SYSTEM_SUSPEND_EVENT_PHASE2_4           0x204
#define SCE_SYSTEM_SUSPEND_EVENT_PHASE2_5           0x205
#define SCE_SYSTEM_SUSPEND_EVENT_PHASE2_6           0x206
#define SCE_SYSTEM_SUSPEND_EVENT_PHASE2_7           0x207
#define SCE_SYSTEM_SUSPEND_EVENT_PHASE2_8           0x208
#define SCE_SYSTEM_SUSPEND_EVENT_PHASE2_9           0x209
#define SCE_SYSTEM_SUSPEND_EVENT_PHASE2_10          0x20A
#define SCE_SYSTEM_SUSPEND_EVENT_PHASE2_11          0x20B
#define SCE_SYSTEM_SUSPEND_EVENT_PHASE2_12          0x20C
#define SCE_SYSTEM_SUSPEND_EVENT_PHASE2_13          0x20D
#define SCE_SYSTEM_SUSPEND_EVENT_PHASE2_14          0x20E
#define SCE_SYSTEM_SUSPEND_EVENT_PHASE2_15          0x20F
#define SCE_SYSTEM_SUSPEND_EVENT_PHASE2_16          0x210

#define SCE_SYSTEM_SUSPEND_EVENT_PHASE1_0           0x400
#define SCE_SYSTEM_SUSPEND_EVENT_PHASE1_1           0x401
#define SCE_SYSTEM_SUSPEND_EVENT_PHASE1_2           0x402

#define SCE_SYSTEM_SUSPEND_EVENT_FREEZE             0x1000

/* Different phases can be used to prioritize work. Phase0_15 is raised first, Phase0_0 last. */

#define SCE_SYSTEM_SUSPEND_EVENT_PHASE0_0           0x4000
#define SCE_SYSTEM_SUSPEND_EVENT_PHASE0_1           0x4001
#define SCE_SYSTEM_SUSPEND_EVENT_PHASE0_2           0x4002
#define SCE_SYSTEM_SUSPEND_EVENT_PHASE0_3           0x4003
#define SCE_SYSTEM_SUSPEND_EVENT_PHASE0_4           0x4004
#define SCE_SYSTEM_SUSPEND_EVENT_PHASE0_5           0x4005
#define SCE_SYSTEM_SUSPEND_EVENT_PHASE0_6           0x4006
#define SCE_SYSTEM_SUSPEND_EVENT_PHASE0_7           0x4007
#define SCE_SYSTEM_SUSPEND_EVENT_PHASE0_8           0x4008
#define SCE_SYSTEM_SUSPEND_EVENT_PHASE0_9           0x4009
#define SCE_SYSTEM_SUSPEND_EVENT_PHASE0_10          0x400A
#define SCE_SYSTEM_SUSPEND_EVENT_PHASE0_11          0x400B
#define SCE_SYSTEM_SUSPEND_EVENT_PHASE0_12          0x400C
#define SCE_SYSTEM_SUSPEND_EVENT_PHASE0_13          0x400D
#define SCE_SYSTEM_SUSPEND_EVENT_PHASE0_14          0x400E
#define SCE_SYSTEM_SUSPEND_EVENT_PHASE0_15          0x400F

/* PSP resume events */

/* Different phases can be used to prioritize work. Phase0_0 is raised first, Phase0_15 last. */

#define SCE_SYSTEM_RESUME_EVENT_PHASE0_0            0x10000
#define SCE_SYSTEM_RESUME_EVENT_PHASE0_1            0x10001
#define SCE_SYSTEM_RESUME_EVENT_PHASE0_2            0x10002
#define SCE_SYSTEM_RESUME_EVENT_PHASE0_3            0x10003
#define SCE_SYSTEM_RESUME_EVENT_PHASE0_4            0x10004
#define SCE_SYSTEM_RESUME_EVENT_PHASE0_5            0x10005
#define SCE_SYSTEM_RESUME_EVENT_PHASE0_6            0x10006
#define SCE_SYSTEM_RESUME_EVENT_PHASE0_7            0x10007
#define SCE_SYSTEM_RESUME_EVENT_PHASE0_8            0x10008
#define SCE_SYSTEM_RESUME_EVENT_PHASE0_9            0x10009
#define SCE_SYSTEM_RESUME_EVENT_PHASE0_10           0x1000A
#define SCE_SYSTEM_RESUME_EVENT_PHASE0_11           0x1000B
#define SCE_SYSTEM_RESUME_EVENT_PHASE0_12           0x1000C
#define SCE_SYSTEM_RESUME_EVENT_PHASE0_13           0x1000D
#define SCE_SYSTEM_RESUME_EVENT_PHASE0_14           0x1000E
#define SCE_SYSTEM_RESUME_EVENT_PHASE0_15           0x1000F

#define SCE_SYSTEM_RESUME_EVENT_MELT                0x40000

#define SCE_SYSTEM_RESUME_EVENT_PHASE1_0            0x100000
#define SCE_SYSTEM_RESUME_EVENT_PHASE1_1            0x100001
#define SCE_SYSTEM_RESUME_EVENT_PHASE1_2            0x100002

#define SCE_SYSTEN_RESUME_EVENT_COMPLETED           0x400000  

typedef struct {
    SceSize size; // 0
    u32 isStandbyOrRebootRequested; // 4
    s64 systemTimePreSuspendOp; // 8
    u32 *pWakeUpCondition; // 16
    void *pResumeData; // 20
    u32 unk24; // 24
    u32 unk28; // 28
    u32 unk32; // 32
    u32 unk36; // 36
    u32 unk40; // 40
    u32 unk44; // 44
    u32 unk48; // 48
    u32 unk52; // 52
    u32 unk56; // 56
    u32 unk60; // 60
} SceSysEventSuspendPayload; // size = 64

typedef struct {
    SceSize size; // 0
    u32 sdkVersion; // 4
    u32 unk8; // 8
    u32 unk12; // 12
    u32 unk16; // 16
    u32 unk20; // 20
    u32 unk24; // 24
    u32 unk28; // 28
    s64 systemTimePreSuspendOp; // 32
    s32 pllOutSelect; // 40
    s32 unk44; // 44
    void (*resumePointFunc)(void *); // 48
    SceKernelGameInfo *pSuspendedGameInfo; // 52
    u32 unk56; // 56
    u32 unk60; // 60
    u32 unk64; // 64
    void *pInitParamSfo; // 68
    SceSize paramSfoSize; // 72
} SceSysEventSuspendPayloadResumData; // size = 128

typedef struct {
    SceSize size; // 0;
    u32 unk4; // 4
    s64 systemTimePreSuspendOp; // 8
    s64 unk16; // 16
    u32 unk24; // 24
    u32 unk28; // 28
    u32 unk32; // 32
    u32 unk36; // 36
    u32 unk40; // 40
    u32 unk44; // 44
    u32 unk48; // 48
    u32 unk52; // 52
} SceSysEventResumePayload; // size = 56

typedef struct SceSysEventHandler {
    s32 size;
    char *name;
    s32 typeMask;
    s32 (*handler)(s32 eventId, char* eventName, void *param, s32 *result);
    s32 gp;
    SceBool busy;
    struct SceSysEventHandler *next;
    s32 reserved[9];
} SceSysEventHandler;

s32 sceKernelUnregisterSysEventHandler(SceSysEventHandler *handler);
s32 sceKernelSysEventDispatch(s32 eventTypeMask, s32 eventId, char *eventName, void *param, s32 *result, s32 break_nonzero, 
                              SceSysEventHandler **break_handler);
s32 sceKernelSysEventInit(void);
s32 sceKernelIsRegisterSysEventHandler(SceSysEventHandler *handler);
s32 sceKernelRegisterSysEventHandler(SceSysEventHandler *handler);
SceSysEventHandler *sceKernelReferSysEventHandler(void);

