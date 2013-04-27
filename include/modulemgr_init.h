/* Copyright (C) 2011, 2012, 2013 The uOFW team
   See the file COPYING for copying permission.
*/

#include "common_header.h"

#include "loadcore.h"
#include "loadexec_kernel.h"

/**
 * Application types.
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

enum SceInitApiType {
    SCE_INIT_APITYPE_UNK0x100       = 0x100,
    /* User */
    SCE_INIT_APITYPE_GAME_EBOOT     = 0x110,
    SCE_INIT_APITYPE_GAME_BOOT      = 0x111,
    SCE_INIT_APITYPE_EMU_EBOOT_MS   = 0x112,
    SCE_INIT_APITYPE_EMU_BOOT_MS    = 0x113,
    SCE_INIT_APITYPE_EMU_EBOOT_EF   = 0x114,
    SCE_INIT_APITYPE_EMU_BOOT_EF    = 0x115,
    /* Kernel only */
    SCE_INIT_APITYPE_NPDRM_MS       = 0x116, /* Distributed programs and data through the Playstation Store. */
    SCE_INIT_APITYPE_NPDRM_EF       = 0x118, /* NP-DRM: PlayStation Network Platform Digital Rights Management */
    SCE_INIT_APITYPE_DISC           = 0x120,
    SCE_INIT_APITYPE_DISC_UPDATER   = 0x121,
    SCE_INIT_APITYPE_DISC_DEBUG     = 0x122,
    SCE_INIT_APITYPE_DISC_EMU_MS1   = 0x123, /* np9660 game */
    SCE_INIT_APITYPE_DISC_EMU_MS2   = 0x124,
    SCE_INIT_APITYPE_DISC_EMU_EF1   = 0x125,
    SCE_INIT_APITYPE_DISC_EMU_EF2   = 0x126,
    SCE_INIT_APITYPE_USBWLAN        = 0x130, /* Game sharing */
    SCE_INIT_APITYPE_USBWLAN_DEBUG  = 0x131,
    SCE_INIT_APITYPE_UNK            = 0x132,
    SCE_INIT_APITYPE_UNK_DEBUG      = 0x133,
    SCE_INIT_APITYPE_MS1            = 0x140,
    SCE_INIT_APITYPE_MS2            = 0x141,
    SCE_INIT_APITYPE_MS3            = 0x142,
    SCE_INIT_APITYPE_MS4            = 0x143, /* comic reader */
    SCE_INIT_APITYPE_MS5            = 0x144, /* pops */
    SCE_INIT_APITYPE_MS6            = 0x145,
    SCE_INIT_APITYPE_EF1            = 0x151,
    SCE_INIT_APITYPE_EF2            = 0x152,
    SCE_INIT_APITYPE_EF3            = 0x153,
    SCE_INIT_APITYPE_EF4            = 0x154,
    SCE_INIT_APITYPE_EF5            = 0x155,
    SCE_INIT_APITYPE_EF6            = 0x156,
    SCE_INIT_APITYPE_UNK_GAME1      = 0x160,
    SCE_INIT_APITYPE_UNK_GAME2      = 0x161,
    SCE_INIT_APITYPE_MLNAPP_MS      = 0x170,
    SCE_INIT_APITYPE_MLNAPP_EF      = 0x171,
    SCE_INIT_APITYPE_KERNEL_1       = 0x200,
    SCE_INIT_APITYPE_VSH_1          = 0x210, /* ExitGame */
    SCE_INIT_APITYPE_VSH_2          = 0x220, /* ExitVSH */
    SCE_INIT_APITYPE_KERNEL_REBOOT  = 0x300,
    SCE_INIT_APITYPE_DEBUG          = 0x420  /* doesn't start reboot */
};

/** 
 * This structure represents an Init control block. It holds information about the 
 * currently booted module by Init.
 */
typedef struct {
    /** The API type of the currently loaded module. One of ::SceInitApiType. */
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
    /** The number of power locks used by init. */
    s32 numPowerLocks;
    /** The address of a memory protection block of type ::SCE_PROTECT_INFO_TYPE_PARAM_SFO. */
    void *paramSfoBase; //100
    /** The size of of the memory block pointed to by ::paramSfoBase. */
    SceSize paramSfoSize; //104
    /** Unknown. */
    s32 lptSummary;
    /** Pointer to boot callbacks of modules. */
    SceBootCallback *bootCallbacks1;
    /** Unknown. */
    void *unk116;
    /** Pointer to boot callbacks of modules. */
    SceBootCallback *bootCallbacks2; //120
    /** Unknown. */
    void *unk124;
} SceInit;

/**
 * Get the application type of a module.
 * 
 * @return The application type. One of ::SceApplicationType.
 */
s32 sceKernelApplicationType(void);

/**
 * Get the API type of a module.
 * 
 * @return The API type. One of ::SceInitApiType.
 */
s32 sceKernelInitApitype(void);

s32 sceKernelRegisterChunk(SceUID chunkId, int unk1);
SceUID sceKernelGetChunk(SceUID chunkId);

s32 sceKernelBootFrom(void);
int sceKernelSetInitCallback(void *, int, int);

SceInit *sceKernelQueryInitCB(void); //InitForKernel_040C934B
void *InitForKernel_D83A9BD7(void *);
s32 InitForKernel_9D33A110(void);

