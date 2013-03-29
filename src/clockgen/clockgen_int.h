/* Copyright (C) 2011, 2012, 2013 The uOFW team    
   See the file COPYING for copying permission.
*/

#include <clockgen.h>
#include <common_imp.h>

s32 _sceClockgenModuleStart(SceSize args, void *argp);
s32 _sceClockgenModuleRebootBefore(SceSize args, void *argp);
s32 _sceClockgenSysEventHandler(s32 ev_id, char *ev_name, void *param, s32 *result);
s32 _sceClockgenSetControl1(s32 bus, s32 mode);
s32 _cy27040_write_register(u8 regid, u8 val);