/* Copyright (C) 2011, 2012 The uOFW team
   See the file COPYING for copying permission.
*/

#include "common.h"

typedef struct {
    s32 size;
    s8 *name;
    s32 type_mask;
    s32 (*handler)(s32 ev_id, s8* ev_name, void* param, s32* result);
    s32 gp;
    s32 busy;
    struct SceSysEventHandler *next;
    s32 reserved[9];
} SceSysEventHandler;

s32 sceKernelUnregisterSysEventHandler(SceSysEventHandler *handler);
s32 sceKernelSysEventDispatch(s32 ev_type_mask, s32 ev_id, s8* ev_name, void* param, s32* result, s32 break_nonzero, SceSysEventHandler **break_handler);
s32 sceKernelSysEventInit(void);
s32 sceKernelIsRegisterSysEventHandler(SceSysEventHandler* handler);
s32 sceKernelRegisterSysEventHandler(SceSysEventHandler* handler);
SceSysEventHandler *sceKernelReferSysEventHandler(void);

