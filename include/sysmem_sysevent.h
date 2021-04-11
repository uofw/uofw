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

#define SCE_SYSTEM_SUSPEND_EVENT_QUERY              0x00000100 /* Cancel request can be sent. */
#define SCE_SYSTEM_SUSPEND_EVENT_CANCELLATION       0x00000101
#define SCE_SYSTEM_SUSPEND_EVENT_START              0x00000102

/* Different phases can be used to prioritize work/set up data. Phase2_16 is raised first, Phase2_0 last. */

#define SCE_SYSTEM_SUSPEND_EVENT_PHASE2_0           0x00000200
#define SCE_SYSTEM_SUSPEND_EVENT_PHASE2_1           0x00000201
#define SCE_SYSTEM_SUSPEND_EVENT_PHASE2_2           0x00000202
#define SCE_SYSTEM_SUSPEND_EVENT_PHASE2_3           0x00000203
#define SCE_SYSTEM_SUSPEND_EVENT_PHASE2_4           0x00000204
#define SCE_SYSTEM_SUSPEND_EVENT_PHASE2_5           0x00000205
#define SCE_SYSTEM_SUSPEND_EVENT_PHASE2_6           0x00000206
#define SCE_SYSTEM_SUSPEND_EVENT_PHASE2_7           0x00000207
#define SCE_SYSTEM_SUSPEND_EVENT_PHASE2_8           0x00000208
#define SCE_SYSTEM_SUSPEND_EVENT_PHASE2_9           0x00000209
#define SCE_SYSTEM_SUSPEND_EVENT_PHASE2_10          0x0000020A
#define SCE_SYSTEM_SUSPEND_EVENT_PHASE2_11          0x0000020B
#define SCE_SYSTEM_SUSPEND_EVENT_PHASE2_12          0x0000020C
#define SCE_SYSTEM_SUSPEND_EVENT_PHASE2_13          0x0000020D
#define SCE_SYSTEM_SUSPEND_EVENT_PHASE2_14          0x0000020E
#define SCE_SYSTEM_SUSPEND_EVENT_PHASE2_15          0x0000020F
#define SCE_SYSTEM_SUSPEND_EVENT_PHASE2_16          0x00000210

#define SCE_SYSTEM_SUSPEND_EVENT_PHASE1_0           0x00000400
#define SCE_SYSTEM_SUSPEND_EVENT_PHASE1_1           0x00000401
#define SCE_SYSTEM_SUSPEND_EVENT_PHASE1_2           0x00000402 /* Cancel request can be sent. */

#define SCE_SYSTEM_SUSPEND_EVENT_FREEZE             0x00001000

/* Different phases can be used to prioritize work/set up data. Phase0_15 is raised first, Phase0_0 last. */

#define SCE_SYSTEM_SUSPEND_EVENT_PHASE0_0           0x00004000
#define SCE_SYSTEM_SUSPEND_EVENT_PHASE0_1           0x00004001
#define SCE_SYSTEM_SUSPEND_EVENT_PHASE0_2           0x00004002
#define SCE_SYSTEM_SUSPEND_EVENT_PHASE0_3           0x00004003
#define SCE_SYSTEM_SUSPEND_EVENT_PHASE0_4           0x00004004
#define SCE_SYSTEM_SUSPEND_EVENT_PHASE0_5           0x00004005
#define SCE_SYSTEM_SUSPEND_EVENT_PHASE0_6           0x00004006
#define SCE_SYSTEM_SUSPEND_EVENT_PHASE0_7           0x00004007
#define SCE_SYSTEM_SUSPEND_EVENT_PHASE0_8           0x00004008
#define SCE_SYSTEM_SUSPEND_EVENT_PHASE0_9           0x00004009
#define SCE_SYSTEM_SUSPEND_EVENT_PHASE0_10          0x0000400A
#define SCE_SYSTEM_SUSPEND_EVENT_PHASE0_11          0x0000400B
#define SCE_SYSTEM_SUSPEND_EVENT_PHASE0_12          0x0000400C
#define SCE_SYSTEM_SUSPEND_EVENT_PHASE0_13          0x0000400D
#define SCE_SYSTEM_SUSPEND_EVENT_PHASE0_14          0x0000400E
#define SCE_SYSTEM_SUSPEND_EVENT_PHASE0_15          0x0000400F

/* PSP resume events */

/* Different phases can be used to prioritize work/set up data. Phase0_0 is raised first, Phase0_15 last. */

#define SCE_SYSTEM_RESUME_EVENT_PHASE0_0            0x00010000
#define SCE_SYSTEM_RESUME_EVENT_PHASE0_1            0x00010001
#define SCE_SYSTEM_RESUME_EVENT_PHASE0_2            0x00010002
#define SCE_SYSTEM_RESUME_EVENT_PHASE0_3            0x00010003
#define SCE_SYSTEM_RESUME_EVENT_PHASE0_4            0x00010004
#define SCE_SYSTEM_RESUME_EVENT_PHASE0_5            0x00010005
#define SCE_SYSTEM_RESUME_EVENT_PHASE0_6            0x00010006
#define SCE_SYSTEM_RESUME_EVENT_PHASE0_7            0x00010007
#define SCE_SYSTEM_RESUME_EVENT_PHASE0_8            0x00010008
#define SCE_SYSTEM_RESUME_EVENT_PHASE0_9            0x00010009
#define SCE_SYSTEM_RESUME_EVENT_PHASE0_10           0x0001000A
#define SCE_SYSTEM_RESUME_EVENT_PHASE0_11           0x0001000B
#define SCE_SYSTEM_RESUME_EVENT_PHASE0_12           0x0001000C
#define SCE_SYSTEM_RESUME_EVENT_PHASE0_13           0x0001000D
#define SCE_SYSTEM_RESUME_EVENT_PHASE0_14           0x0001000E
#define SCE_SYSTEM_RESUME_EVENT_PHASE0_15           0x0001000F

#define SCE_SYSTEM_RESUME_EVENT_MELT                0x00040000

#define SCE_SYSTEM_RESUME_EVENT_PHASE1_0            0x00100000
#define SCE_SYSTEM_RESUME_EVENT_PHASE1_1            0x00100001
#define SCE_SYSTEM_RESUME_EVENT_PHASE1_2            0x00100002 /* Cancel request can be sent. */

#define SCE_SYSTEM_RESUME_EVENT_COMPLETED           0x00400000  

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

