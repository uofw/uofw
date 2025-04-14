/* Copyright (C) 2011, 2012, 2013 The uOFW team
   See the file COPYING for copying permission.
*/

#include "common_header.h"

/** @defgroup LoadExecForKernel LoadExecForKernel Library
 *  Allows the kernel and VSH to run executables through reboot.bin.
 * @{
 */

#ifndef LOADEXEC_KERNEL_H
#define	LOADEXEC_KERNEL_H

/** Execution parameters for most kernel LoadExec functions. */
typedef struct
{
    /** Size of the structure in bytes. */
    SceSize size;
    /** Size of the arguments string. */
    SceSize args;
    /** Pointer to the arguments strings. */
    void *argp;
    /** The key: "game", "updater", "vsh", "umdemu" or "mlnapp". */
    const char *key;
    /** The size of the vshmain arguments. */
    u32 vshmainArgs;
    /** Vshmain arguments that will be passed to vshmain after the program has exited. */
    void *vshmainArgp;
    /** "/kd/pspbtcnf_game.txt" or "/kd/pspbtcnf.txt" if not supplied (max. 256 chars). */
    char *configFile;
    /** An unknown string (max. 256 chars). */
    char *string;
    /** Unknown flag default value = 0x10000. */
    u32 flags;
    /** The size of the external arguments. */
    u32 extArgs;
    /** "External" arguments only used when calling a function accepting a SceKernelLoadExecVSHParam as an argument. */
    void *extArgp;
    /** Unused. */
    u32 opt11;
} SceKernelLoadExecVSHParam;

/** Types of arguments passed to reboot.bin. */
typedef enum
{
    /** No argument (just used as a default value). */
    SCE_KERNEL_REBOOT_ARGTYPE_NONE     =   0x0,
    /** Used by arguments needed for some api types (USBWLAN, USBWLAN_DEBUG, 'UNK', 'UNK_DEBUG'). */
    SCE_KERNEL_REBOOT_ARGTYPE_KERNEL   =   0x1,
    /** The file path (the one passed to the LoadExec() functions). */
    SCE_KERNEL_REBOOT_ARGTYPE_FILENAME =   0x2,
    /** Vshmain arguments, passed to vshmain after the program has exited. */
    SCE_KERNEL_REBOOT_ARGTYPE_VSHMAIN  =   0x4,
    /** Unknown (not argument made in loadexec has this type). */
    SCE_KERNEL_REBOOT_ARGTYPE_UNKNOWN8 =   0x8,
    /** Game info (as returned by sceKernelGetGameInfo()). */
    SCE_KERNEL_REBOOT_ARGTYPE_GAMEINFO =  0x20,
    /** Used only by emulation api types. */
    SCE_KERNEL_REBOOT_ARGTYPE_EMU      =  0x40,
    /** Used only by NpDrm api types. */
    SCE_KERNEL_REBOOT_ARGTYPE_NPDRM    =  0x80,
    /** Same as SCE_KERNEL_REBOOT_ARGTYPE_KERNEL? */
    SCE_KERNEL_REBOOT_ARGTYPE_DEFAULT  = 0x100,
    /** Used by arguments passed to LoadExec() functions through the extArgs/extArgp fields of the parameters. */
    SCE_KERNEL_REBOOT_ARGTYPE_EXT      = 0x400
} SceKernelRebootArgType;

/**
 * Exit VSH from VSH.
 *
 * @param opt Execution parameters.
 *
 * @return 0 on success.
 */
s32 sceKernelExitVSHVSH(SceKernelLoadExecVSHParam *opt);

/**
 * Invoke the exit callback.
 *
 * @return 0 on success.
 */
s32 sceKernelInvokeExitCallback(void);

/**
 * Check an execution parameters key.
 *
 * @param opt The execution parameters.
 * @param notUpdater If set to 1, will check if key is not "updater"; if set to 0, will check if key is "game", "vsh" or "updater"; otherwise, returns an error.
 *
 * @return 0 if key is inccorrect, 1 if key is correct, < 0 on error.
 */
s32 LoadExecForKernel_BC26BEEF(SceKernelLoadExecVSHParam *opt, s32 notUpdater);

/**
 * Set an argument to send to reboot.bin next time it will be started.
 *
 * @param argp The argument pointer.
 * @param args The argument size.
 * @param argType The argument type.
 *
 * @return 0.
 */
s32 LoadExecForKernel_DBD0CF1B(void *argp, s32 args, SceKernelRebootArgType argType);

/**
 * Load an executable from a disc by VSH.
 *
 * @param file The execution path.
 * @param opt Execution parameters.
 *
 * @return 0 on success.
 */
s32 sceKernelLoadExecVSHDisc(char *file, SceKernelLoadExecVSHParam *opt);

/**
 * Load an updater executable from a disc by VSH.
 *
 * @param file The execution path.
 * @param opt Execution parameters.
 *
 * @return 0 on success.
 */
s32 sceKernelLoadExecVSHDiscUpdater(char *file, SceKernelLoadExecVSHParam *opt);

/**
 * Load an executable from a disc in debugging mode by VSH.
 *
 * @param file The execution path.
 * @param opt Execution parameters.
 *
 * @return 0 on success.
 */
s32 sceKernelLoadExecVSHDiscDebug(char *file, SceKernelLoadExecVSHParam *opt);

/**
 * Load an executable from an emulated disc in MS 1 by VSH.
 *
 * @param file The execution path.
 * @param opt Execution parameters.
 *
 * @return 0 on success.
 */
s32 LoadExecForKernel_F9CFCF2F(char *file, SceKernelLoadExecVSHParam *opt);

/**
 * Load an executable from an emulated disc in MS 2 by VSH.
 *
 * @param file The execution path.
 * @param opt Execution parameters.
 *
 * @return 0 on success.
 */
s32 LoadExecForKernel_077BA314(char *file, SceKernelLoadExecVSHParam *opt);

/**
 * Load an executable from an emulated disc in EF 1 by VSH.
 *
 * @param file The execution path.
 * @param opt Execution parameters.
 *
 * @return 0 on success.
 */
s32 LoadExecForKernel_E704ECC3(char *file, SceKernelLoadExecVSHParam *opt);

/**
 * Load an executable from an emulated disc in EF 2 by VSH.
 *
 * @param file The execution path.
 * @param opt Execution parameters.
 *
 * @return 0 on success.
 */
s32 LoadExecForKernel_47A5A49C(char *file, SceKernelLoadExecVSHParam *opt);

/**
 * Load an executable from USB or Wlan by VSH.
 *
 * @param file The execution path.
 * @param opt Execution parameters.
 *
 * @return 0 on success.
 */
s32 sceKernelLoadExecBufferVSHUsbWlan(s32 args, void *argp, SceKernelLoadExecVSHParam *opt);

/**
 * Load an executable from USB or Wlan in debug mode by VSH.
 *
 * @param file The execution path.
 * @param opt Execution parameters.
 *
 * @return 0 on success.
 */
s32 sceKernelLoadExecBufferVSHUsbWlanDebug(s32 args, void *argp, SceKernelLoadExecVSHParam *opt);                                                                                                 

/**
 * Load an executable from ??? by VSH.
 *
 * @param file The execution path.
 * @param opt Execution parameters.
 *
 * @return 0 on success.
 */
s32 LoadExecForKernel_87C3589C(s32 args, void *argp, SceKernelLoadExecVSHParam *opt);                                                                                                             

/**
 * Load an executable from ??? (same as with LoadExecForKernel_87C3589C) in debug mode by VSH.
 *
 * @param file The execution path.
 * @param opt Execution parameters.
 *
 * @return 0 on success.
 */
s32 LoadExecForKernel_7CAFE77F(s32 args, void *argp, SceKernelLoadExecVSHParam *opt);

/**
 * Load an executable from MS 1 by VSH.
 *
 * @param file The execution path.
 * @param opt Execution parameters.
 *
 * @return 0 on success.
 */
s32 sceKernelLoadExecVSHMs1(char *file, SceKernelLoadExecVSHParam *opt);

/**
 * Load an executable from MS 2 by VSH.
 *
 * @param file The execution path.
 * @param opt Execution parameters.
 *
 * @return 0 on success.
 */
s32 sceKernelLoadExecVSHMs2(char *file, SceKernelLoadExecVSHParam *opt);

/**
 * Load an executable from MS 3 by VSH.
 *
 * @param file The execution path.
 * @param opt Execution parameters.
 *
 * @return 0 on success.
 */
s32 sceKernelLoadExecVSHMs3(char *file, SceKernelLoadExecVSHParam *opt);

/**
 * Load an executable from MS 4 by VSH.
 *
 * @param file The execution path.
 * @param opt Execution parameters.
 *
 * @return 0 on success.
 */
s32 sceKernelLoadExecVSHMs4(char *file, SceKernelLoadExecVSHParam *opt);

/**
 * Load an executable from MS 5 by VSH.
 *
 * @param file The execution path.
 * @param opt Execution parameters.
 *
 * @return 0 on success.
 */
s32 sceKernelLoadExecVSHMs5(char *file, SceKernelLoadExecVSHParam *opt);

/**
 * Load an executable from MS 6 by VSH.
 *
 * @param file The execution path.
 * @param opt Execution parameters.
 *
 * @return 0 on success.
 */
s32 LoadExecForKernel_A6658F10(char *file, SceKernelLoadExecVSHParam *opt);

/**
 * Load an executable from EF 1 by VSH.
 *
 * @param file The execution path.
 * @param opt Execution parameters.
 *
 * @return 0 on success.
 */
s32 LoadExecForKernel_16A68007(char *file, SceKernelLoadExecVSHParam *opt);

/**
 * Load an executable from EF 2 by VSH.
 *
 * @param file The execution path.
 * @param opt Execution parameters.
 *
 * @return 0 on success.
 */
s32 LoadExecForKernel_032A7938(char *file, SceKernelLoadExecVSHParam *opt);

/**
 * Load an executable from EF 3 by VSH.
 *
 * @param file The execution path.
 * @param opt Execution parameters.
 *
 * @return 0 on success.
 */
s32 LoadExecForKernel_40564748(char *file, SceKernelLoadExecVSHParam *opt);

/**
 * Load an executable from EF 4 by VSH.
 *
 * @param file The execution path.
 * @param opt Execution parameters.
 *
 * @return 0 on success.
 */
s32 LoadExecForKernel_E1972A24(char *file, SceKernelLoadExecVSHParam *opt);

/**
 * Load an executable from EF 5 by VSH.
 *
 * @param file The execution path.
 * @param opt Execution parameters.
 *
 * @return 0 on success.
 */
s32 LoadExecForKernel_C7C83B1E(char *file, SceKernelLoadExecVSHParam *opt);

/**
 * Load an executable from EF 6 by VSH.
 *
 * @param file The execution path.
 * @param opt Execution parameters.
 *
 * @return 0 on success.
 */
s32 LoadExecForKernel_8C4679D3(char *file, SceKernelLoadExecVSHParam *opt);

/**
 * Load an executable from ??? 1 (game mode) by VSH.
 *
 * @param file The execution path.
 * @param opt Execution parameters.
 *
 * @return 0 on success.
 */
s32 LoadExecForKernel_B343FDAB(char *file, SceKernelLoadExecVSHParam *opt);

/**
 * Load an executable from ??? 2 (game mode) by VSH.
 *
 * @param file The execution path.
 * @param opt Execution parameters.
 *
 * @return 0 on success.
 */
s32 LoadExecForKernel_1B8AB02E(char *file, SceKernelLoadExecVSHParam *opt);

/**
 * Load an executable of a MLN (PSN) application from MS by VSH.
 *
 * @param file The execution path.
 * @param opt Execution parameters.
 *
 * @return 0 on success.
 */
s32 LoadExecForKernel_C11E6DF1(char *file, SceKernelLoadExecVSHParam *opt);

/**
 * Load an executable of a MLN (PSN) application from EF by VSH.
 *
 * @param file The execution path.
 * @param opt Execution parameters.
 *
 * @return 0 on success.
 */
s32 LoadExecForKernel_9BD32619(char *file, SceKernelLoadExecVSHParam *opt);

/**
 * Exit the VSH from kernel.
 *
 * @param opt Execution parameters.
 *
 * @return 0 on success.
 */
s32 sceKernelExitVSHKernel(SceKernelLoadExecVSHParam *opt);

/**
 * Returns 0. (Probably a disabled debugging function.)
 *
 * @return 0.
 */
s32 LoadExecForKernel_C540E3B3(void);

/**
 * Register an exit callback, started upon game exit through the "Home" button.
 *
 * @param cbId The callback ID.
 *
 * @return 0 on success.
 */
s32 sceKernelRegisterExitCallback(SceUID cbId);

/**
 * Unregister the exit callback.
 *
 * @return 0 on success.
 */
s32 sceKernelUnregisterExitCallback(void);

/**
 * Get the current exit callback.
 *
 * @return The current exit callback ID.
 */
SceUID sceKernelCheckExitCallback(void);

/**
 * Register a function which will be started upon exit callback registering.
 *
 * @param cb The function to register.
 *
 * @return 0.
 */
s32 LoadExecForKernel_A5ECA6E3(void (*cb)());

/* Variable export */

// TODO: As this is an exported variable, we should rename it accordingly.
extern s32 *g_unkCbInfo[4];

#endif	/* LOADEXEC_KERNEL_H */

/** @} */

