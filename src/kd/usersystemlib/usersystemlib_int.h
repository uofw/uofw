/* Copyright (C) 2011, 2012, 2013 The uOFW team    
   See the file COPYING for copying permission.
*/

#include <usersystemlib_kernel.h>

#include <common_imp.h>
#include <sysmem_kernel.h>

typedef struct {
    u32 unk0[48]; // 0
    s32 id; // 192
    u32 unk2; // 196
    u32 stackBottom; // 200
} SceThread;

extern SceGeLazy g_lazy;
extern SceThread *g_thread;
