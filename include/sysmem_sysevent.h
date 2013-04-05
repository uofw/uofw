/* Copyright (C) 2011, 2012, 2013 The uOFW team
   See the file COPYING for copying permission.
*/

#include "common_header.h"

enum SceSysEventTypes {
    SCE_SUSPEND_EVENTS = 0x0000FF00,
    SCE_RESUME_EVENTS = 0x00FF0000,
    SCE_SPEED_CHANGE_EVENTS = 0x01000000
};

typedef struct {
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

