/* Copyright (C) 2011, 2012, 2013 The uOFW team    
   See the file COPYING for copying permission.
*/

#include "usersystemlib_int.h"

#include <interruptman.h>
#include <sysmem_kernel.h>
#include <threadman_user.h>

#define STACK_SIZE 		0x2000
#define CMD_LIST_SIZE 	0x500

SCE_MODULE_INFO(
	"sceKernelLibrary",
    SCE_MODULE_ATTR_CANT_STOP | SCE_MODULE_ATTR_EXCLUSIVE_LOAD | SCE_MODULE_ATTR_EXCLUSIVE_START,
    1, 6
);
SCE_MODULE_START_THREAD_PARAMETER(3, 0x20, 0x400, 0);
SCE_MODULE_BOOTSTART("_UsersystemLibInit");
SCE_SDK_VERSION(SDK_VERSION);

SceGeLazy g_lazy = {
    .dlId = -1, // b80
    .stall = NULL, // b84
    .count = 0, // b88
    .max = 100 // b8c
};
static u8 g_stack[STACK_SIZE]; // bc0
SceThread *g_thread; // 2bc0
static s32 g_cmdList[CMD_LIST_SIZE]; // 2c00

// module_start
s32 _UsersystemLibInit(SceSize argc __attribute__((unused)), void *argp __attribute__((unused)))
{
    // InterruptManager_EEE43F47
    sceKernelRegisterUserSpaceIntrStack(
        (s32)g_stack, // 0xBC0
        STACK_SIZE, // 0x2000
        (s32)&g_thread // 0x2BC0
    );

    // SysMemUserForUser_A6848DF8
    sceKernelSetUsersystemLibWork(
        g_cmdList, // 0x2C00
        sceGe_lazy_31129B95, // 0x140
        &g_lazy // 0xB80
    );

    return SCE_ERROR_OK;
}
