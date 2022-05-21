/* Copyright (C) 2011, 2012, 2013 The uOFW team
   See the file COPYING for copying permission.
*/

#include "common_header.h"

/** @defgroup LoadExecForUser LoadExecForUser Library
 *  Allows the user to run executables through reboot.bin.
 * @{
 */

#ifndef LOADEXEC_USER_H
#define	LOADEXEC_USER_H

/** Execution parameters for user LoadExec functions */
typedef struct {
    /** Size of the structure */
    SceSize size;
    /** Size of the arg string */
    SceSize args;
    /** Pointer to the arg string */
    void *argp;
    /** Encryption key ? */
    const char *key;
} SceKernelLoadExecParam;

/**
 * Update the exit callback status (exported as a variable in the kernel module)
 *
 * @return 0 on success.
 */
s32 LoadExecForUser_362A956B(void);

/**
 * Load an executable.
 *
 * @param file The executable path.
 * @param opt Execution parameters.
 *
 * @return 0 on success.
 */
s32 sceKernelLoadExec(char *file, SceKernelLoadExecParam *opt);

/**
 * Load an executable using npdrm.
 *
 * @param file The executable path.
 * @param opt Execution parameters.
 *
 * @return 0 on success.
 */
s32 LoadExecForUser_8ADA38D3(char *file, SceKernelLoadExecParam *opt);

/**
 * Exit a game with an argument.
 *
 * @param arg An unknown argument.
 *
 * @return 0 on success.
 */
s32 LoadExecForUser_D1FB50DC(void *arg);

/**
 * Register an exit callback, started upon game exit through the "Home" button.
 *
 * @param cbId The callback ID.
 *
 * @return 0 on success.
 */
s32 sceKernelRegisterExitCallback(SceUID cbId);

/**
 * Exit a game (with no status, as opposed to the function's name)
 *
 * @return 0 on success.
 */
s32 sceKernelExitGameWithStatus(void);

/**
 * Exit a game.
 *
 * @return 0 on success.
 */
s32 sceKernelExitGame(void);

#endif	/* LOADEXEC_USER_H */

/** @} */

