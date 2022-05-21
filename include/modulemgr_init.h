/* Copyright (C) 2011 - 2015 The uOFW team
   See the file COPYING for copying permission.
*/

#include "common_header.h"

#include "loadcore.h"
#include "loadexec_kernel.h"

/** @defgroup ModuleManager Module Manager
 *  Module Management.	
 */

/** @defgroup InitForKernel Init For Kernel
 *  @ingroup ModuleManager
 *  The InitForKernel library.
 * @{
 */

#ifndef MODULEMGR_INIT_H
#define	MODULEMGR_INIT_H

/**
 * The possible boot medium types for an executable.
 */
enum SceBootMediumType {
    /** The executable was booted via Flash 0 (1, 2). */
    SCE_INIT_BOOT_FLASH     = 0,
    /** The executable was booted via a Disc medium. */
    SCE_INIT_BOOT_DISC      = 0x20,
    /** The executable was booted via a Game-sharing medium. */
    SCE_INIT_BOOT_USBWLAN   = 0x30,
    /** The executable was booted via the Memory Stick medium. */
    SCE_INIT_BOOT_MS        = 0x40,
    /** The executable was booted via an unknown medium. */
    SCE_INIT_BOOT_EF        = 0x50,
    /** The executable was booted via Flash 3.*/
    SCE_INIT_BOOT_FLASH3    = 0x80,
};

/**
 * Application types of an executable.
 */
enum SceApplicationType {
    /** The application is a VSH application (i.e. VSH modules). */
    SCE_INIT_APPLICATION_VSH        = 0x100,  
    /** The application is an updater. */
    SCE_INIT_APPLICATION_UPDATER    = 0x110,
    /** The application is a PSP game. */
    SCE_INIT_APPLICATION_GAME       = 0x200,
    /** The application is a Playstation One game. */
    SCE_INIT_APPLICATION_POPS       = 0x300,
    /** The application is a PSP application (i.e. Skype). */
    SCE_INIT_APPLICATION_APP        = 0x400,
};

/**
 * API types of an executable.
 */
enum SceFileExecApiType {
    SCE_EXEC_FILE_APITYPE_MODULE_KERNEL                  = 0x000,
    SCE_EXEC_FILE_APITYPE_MODULE_BUFFER_KERNEL           = 0x002,
    SCE_EXEC_FILE_APITYPE_MODULE_KERNEL_BLOCK            = 0x003,
    SCE_EXEC_FILE_APITYPE_MODULE_USER                    = 0x010,
    SCE_EXEC_FILE_APITYPE_MODULE_MS                      = 0x011,
    SCE_EXEC_FILE_APITYPE_MODULE_DNAS                    = 0x013,
    SCE_EXEC_FILE_APITYPE_MODULE_NPDRM                   = 0x014,
    SCE_EXEC_FILE_APITYPE_MODULE_VSH                     = 0x020,
    SCE_EXEC_FILE_APITYPE_MODULE_BUFFER_VSH              = 0x021,
    SCE_EXEC_FILE_APITYPE_MODULE_BUFFER_USBWLAN          = 0x030,
    SCE_EXEC_FILE_APITYPE_MODULE_BUFFER_MS               = 0x042,
    SCE_EXEC_FILE_APITYPE_MODULE_BUFFER_APP              = 0x043,
    SCE_EXEC_FILE_APITYPE_MODULE_BUFFER_BOOT_INIT_BTCNF  = 0x051,
    SCE_EXEC_FILE_APITYPE_MODULE_BOOT_INIT_CONFIG        = 0x052,
    SCE_EXEC_FILE_APITYPE_MODULE_DECI                    = 0x070,
    /** Unknown. */
    SCE_EXEC_FILE_APITYPE_UNK100                         = 0x100,
    /** GAME EBOOT. */
    SCE_EXEC_FILE_APITYPE_GAME_EBOOT                     = 0x110,
    /** GAME BOOT. */
    SCE_EXEC_FILE_APITYPE_GAME_BOOT                      = 0x111,
    /** Emulated EBOOT Memory-Stick. */
    SCE_EXEC_FILE_APITYPE_EMU_EBOOT_MS                   = 0x112,
    /** Emulated BOOT Memory-Stick. */
    SCE_EXEC_FILE_APITYPE_EMU_BOOT_MS                    = 0x113,
    /** Emulated EBOOT EF. */
    SCE_EXEC_FILE_APITYPE_EMU_EBOOT_EF                   = 0x114,
    /** Emulated BOOT EF. */
    SCE_EXEC_FILE_APITYPE_EMU_BOOT_EF                    = 0x115,
    /** NP-DRM Memory-Stick. */
    SCE_EXEC_FILE_APITYPE_NPDRM_MS                       = 0x116, /* Distributed programs and data through the Playstation Store. */
    /** Unknown. */
    SCE_EXEC_FILE_APITYPE_UNK117                         = 0x117,
    /** NP-DRM EF. */
    SCE_EXEC_FILE_APITYPE_NPDRM_EF                       = 0x118, /* NP-DRM: PlayStation Network Platform Digital Rights Management */
    /** Unknown. */
    SCE_EXEC_FILE_APITYPE_UNK119                         = 0x119,
    /** Executable on a disc. */
    SCE_EXEC_FILE_APITYPE_DISC                           = 0x120,
    /** Updater executable on a disc.*/
    SCE_EXEC_FILE_APITYPE_DISC_UPDATER                   = 0x121,
    /** Disc debugger. */
    SCE_EXEC_FILE_APITYPE_DISC_DEBUG                     = 0x122,
    /** NP-9660 game. */
    SCE_EXEC_FILE_APITYPE_DISC_EMU_MS1                   = 0x123,
    /** Unknown. */
    SCE_EXEC_FILE_APITYPE_DISC_EMU_MS2                   = 0x124,
    /** Unknown. */
    SCE_EXEC_FILE_APITYPE_DISC_EMU_EF1                   = 0x125,
    /** Unknown. */
    SCE_EXEC_FILE_APITYPE_DISC_EMU_EF2                   = 0x126,
    /** Game-sharing executable. */
    SCE_EXEC_FILE_APITYPE_USBWLAN                        = 0x130,
    /** Unknown. */
    SCE_EXEC_FILE_APITYPE_USBWLAN_DEBUG                  = 0x131,
    /** Unknown. */
    SCE_EXEC_FILE_APITYPE_UNK132                         = 0x132,
    /** Unknown. */
    SCE_EXEC_FILE_APITYPE_UNK133                         = 0x133,
    /** Unknown. */
    SCE_EXEC_FILE_APITYPE_MS1                            = 0x140,
    /** Unknown. */
    SCE_EXEC_FILE_APITYPE_MS2                            = 0x141,
    /** Unknown. */
    SCE_EXEC_FILE_APITYPE_MS3                            = 0x142,
    /** Applications (i.e. Comic Reader) */
    SCE_EXEC_FILE_APITYPE_MS4                            = 0x143,
    /** Playstation One executable. */
    SCE_EXEC_FILE_APITYPE_MS5                            = 0x144,
    /** Unknown. */
    SCE_EXEC_FILE_APITYPE_MS6                            = 0x145,
    /** Unknown. */
    SCE_EXEC_FILE_APITYPE_EF1                            = 0x151,
    /** Unknown. */
    SCE_EXEC_FILE_APITYPE_EF2                            = 0x152,
    /** Unknown. */
    SCE_EXEC_FILE_APITYPE_EF3                            = 0x153,
    /** Unknown. */
    SCE_EXEC_FILE_APITYPE_EF4                            = 0x154,
    /** Unknown. */
    SCE_EXEC_FILE_APITYPE_EF5                            = 0x155,
    /** Unknown. */
    SCE_EXEC_FILE_APITYPE_EF6                            = 0x156,
    /** Unknown. */
    SCE_EXEC_FILE_APITYPE_UNK160                         = 0x160,
    /** Unknown. */
    SCE_EXEC_FILE_APITYPE_UNK161                         = 0x161,
    /** Unknown. */
    SCE_EXEC_FILE_APITYPE_MLNAPP_MS                      = 0x170,
    /** Unknown. */
    SCE_EXEC_FILE_APITYPE_MLNAPP_EF                      = 0x171,
    /** Unknown. */
    SCE_EXEC_FILE_APITYPE_KERNEL_1                       = 0x200,
    /** Exit Game. */
    SCE_EXEC_FILE_APITYPE_VSH_1                          = 0x210,
    /** Exit VSH. */
    SCE_EXEC_FILE_APITYPE_VSH_2                          = 0x220,
    /** Kernel reboot. */
    SCE_EXEC_FILE_APITYPE_KERNEL_REBOOT                  = 0x300,
    /** Debug. */
    SCE_EXEC_FILE_APITYPE_DEBUG                          = 0x420  /* doesn't start reboot */
};

/** 
 * This structure represents an Init control block. It holds information about the 
 * currently booted module by Init.
 */
typedef struct {
    /** The API type of the currently loaded module. One of ::SceFileExecApiType. */
    s32 apiType; //0
    /** The address of a memory protection block of type ::SCE_PROTECT_INFO_TYPE_FILE_NAME. */
    void *fileModAddr; //4
    /** The address of a memory protection block of type ::SCE_PROTECT_INFO_TYPE_DISC_IMAGE. */
    void *discModAddr; //8
    /** VSH parameters. Used to reboot the kernel. */
    SceKernelLoadExecVSHParam vshParam; //12
    /** Unknown. */
    s32 unk60;
    /** Unknown. */
    s32 unk64;
    /** Unknown. */
    s32 unk68;
    /** Unknown. */
    s32 unk72;
    /** Unknown. */
    s32 unk76;
    /** Unknown. */
    s32 unk80;
    /** Unknown. */
    s32 unk84;
    /** Unknown. */
    s32 unk88;
    /** The application type of the currently loaded module. One of ::SceApplicationType. */
    u32 applicationType; //92
    /** The number of power locks used by Init. */
    s32 numPowerLocks; //96
    /** The address of a memory protection block of type ::SCE_PROTECT_INFO_TYPE_PARAM_SFO. */
    void *paramSfoBase; //100
    /** The size of of the memory block pointed to by ::paramSfoBase. */
    SceSize paramSfoSize; //104
    /** Unknown. */
    s32 lptSummary; //108
    /** Pointer to boot callbacks of modules. */
    SceBootCallback *bootCallbacks1; //112
    /** The current boot callback 1 slot used to hold the registered boot callback. */
    SceBootCallback *curBootCallback1; //116
    /** Pointer to boot callbacks of modules. */
    SceBootCallback *bootCallbacks2; //120
    /** The current boot callback 2 slot used to hold the registered boot callback. */
    SceBootCallback *curBootCallback2; //124
} SceInit;

/**
 * Get the boot medium of the executable calling this function.
 * 
 * @return The boot medium type. One of ::SceBootMediumType.
 */
u32 sceKernelBootFrom(void);

/**
 * Get the boot medium of the executable calling this function. For PSP-GO only?
 * 
 * @return The boot medium type. One of ::SceBootMediumType.
 */
u32 InitForKernel_9D33A110(void);

/**
 * Get the application type of a module.
 * 
 * @return The application type. One of ::SceApplicationType.
 */
s32 sceKernelApplicationType(void);

/**
 * Get the API type of a module.
 * 
 * @return The API type. One of ::SceFileExecApiType.
 */
s32 sceKernelInitApitype(void);

/**
 * Set a boot callback.  Call this function during a module boot process.
 * 
 * @param bootCBFunc The boot callback function to execute once the module has been loaded by the Init 
 *                   module.
 * @param flag Defines the execute order of the callbacks. Pass 0 for earliest execution, 3 for latest.
 *             1 and 2 are between these two. Pass 4 - 7 for execution after Init loaded all modules, again
 *             4 is earliest, 7 is latest. 
 * @param pStatus The returned status of bootCBFunc in case it was executed directly.
 * 
 * @return SCE_ERROR_OK for directly executing the boot callback function. SCE_BOOT_CALLBACK_FUNCTION_QUEUED 
 *         indicates boot callback function was enqueued into other existing boot callbacks and will be called 
 *         when Init boots the rest of the system modules.
 */
u32 sceKernelSetInitCallback(SceKernelBootCallbackFunction bootCBFunc, u32 flag, s32 *pStatus);

/**
 * Disabled debug function.
 * 
 * @return SCE_ERROR_OK.
 */
u32 sceKernelStartIntrLogging(void);

/**
 * Disabled debug function.
 * 
 * @return SCE_ERROR_OK.
 */
u32 sceKernelShowIntrHandlerInfo(void);

/**
 * Disabled debug function.
 * 
 * @return SCE_ERROR_OK.
 */
u32 sceKernelShowIntrMaskTime(void);

/**
 * Retrieve Init's internal control block. This control block manages execution details of an
 * executable, like its API type, its boot medium and its application type.
 * 
 * @return A pointer to Init's internal control block.
 */
SceInit *sceKernelQueryInitCB(void);

/**
 * Get the file name of the currently booted executable.
 * 
 * @return The file name.
 */
char *sceKernelInitFileName(void);

/**
 * Get the disc image of the currently booted executable.
 * 
 * @return The disc image. Return null if there is no disc image.
 */
void *sceKernelInitDiscImage(void);

/**
 * Get information about a paramSfo block of a module to boot.
 * 
 * @param pSize The size of the paramSfo block.
 * 
 * @return A pointer to the head address of the paramSfo block. 
 */
void *sceKernelInitParamSfo(SceSize *pSize);

/**
 * Get the LPT summary. Unknown.
 * 
 * @return The LPT summary of the system.
 */
s32 sceKernelInitLptSummary(void);

/**
 * Get a chunk's memory block ID.
 * 
 * @param chunkId The ID of the chunk which memory block ID you want to receive.
 *                Between 0 - 15.
 * 
 * @return The memory block ID on success (greater than or equal to 0) or 
 *         SCE_ERROR_KERNEL_ILLEGAL_CHUNK_ID.
 */
SceUID sceKernelGetChunk(s32 chunkId);

/**
 * Register a chunk in the system.
 * 
 * @param chunkId The ID of the chunk to hold the memory block ID. Between 0 - 15.
 * @param blockId The memory block ID to register.
 * 
 * @return The blockId stored into the chunk on success, otherwise 
 *         SCE_ERROR_KERNEL_ILLEGAL_CHUNK_ID.
 */
SceUID sceKernelRegisterChunk(s32 chunkId, SceUID blockId);

/**
 * Release a used chunk.
 * 
 * @param chunkId The ID of the chunk to release. Between 0 -15.
 * 
 * @return The new value of the chunk, typically -1, on success, otherwise 
 *         SCE_ERROR_KERNEL_ILLEGAL_CHUNK_ID.
 */
s32 sceKernelReleaseChunk(SceUID chunkId);

#endif	/* MODULEMGR_INIT_H */

/** @} */

