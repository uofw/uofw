/* Copyright (C) 2011, 2012, 2013 The uOFW team
   See the file COPYING for copying permission.
*/

#ifndef LOADEXEC_H
#define	LOADEXEC_H

/** Structure for LoadExecVSH* functions */
typedef struct
{
    /** Size of the structure in bytes */
    SceSize size;
    /** Size of the arguments string */
    SceSize args;
    /** Pointer to the arguments strings */
    void *argp;
    /** The key, usually "game", "updater" or "vsh" */
    const char *key;
    /** The size of the vshmain arguments */
    u32 vshmainArgs;
    /** vshmain arguments that will be passed to vshmain after the program has exited */
    void *vshmainArgp;
    /** "/kd/pspbtcnf_game.txt" or "/kd/pspbtcnf.txt" if not supplied (max. 256 chars) */
    char *configFile;
    /** An unknown string (max. 256 chars) probably used in 2nd stage of loadexec */
    u32 unk4;
    /** unknown flag default value = 0x10000 */
    u32 flags;
    u32 unkArgs;
    void *unkArgp;
    u32 opt11;
} SceKernelLoadExecVSHParam;

#endif	/* LOADEXEC_H */

