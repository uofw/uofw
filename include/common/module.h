/* Copyright (C) 2011 - 2015 The uOFW team
   See the file COPYING for copying permission.
*/

#ifndef COMMON_INCLUDED
# error "Only include common_imp.h or common_header.h!"
#endif

#include "../pspmoduleexport.h"

/** The maximum length of a module name. */
#define SCE_MODULE_NAME_LEN                     (27)

/** SceModuleInfo.modVersion */
#define MODULE_VERSION_MINOR                    (0)
#define MODULE_VERSION_MAJOR                    (1)
#define MODULE_VERSION_NUMBER_CATEGORY_SIZE     (2) /* current number category size */

/** Module Info section data. */
typedef struct  {
    /** 
     * The attributes of a module. Bitwise OR'ed values from ::SceModuleAttribute 
     * and ::SceModulePrivilegeLevel. 
     */
    u16 modAttribute;
    /** The module version. Contains two number categories, minor, major. */
    u8 modVersion[MODULE_VERSION_NUMBER_CATEGORY_SIZE];
    /** The name of the module. */
    char modName[SCE_MODULE_NAME_LEN];
    /** String terminator (always '\0'). */
    s8 terminal;
    /** The global pointer of the module. */
    void *gpValue;
    /** 
     * Pointer to the first resident library entry table of the module. 
     * This section is known as ".lib.ent". 
     */
    void *entTop;
    /** 
     * Pointer to the last line of the .lib.ent section. This line is always 0 and 
     * is known as ".lib.ent.btm". 
     */
    void *entEnd;
    /** 
     * Pointer to the first stub library entry table of the module. 
     * This section is known as "lib.stub". 
     */
    void *stubTop;
    /** 
     * Pointer to the last line of the lib.stub section. This line is always 0 and 
     * is known as ".lib.stub.btm". 
     */
    void *stubEnd;
} SceModuleInfo;

/* 
 * Entry thread structure - an entry thread is used for executing the 
 * module entry functions. 
 */
typedef struct {
    /* The number of entry thread parameters, typically 3. */
    u32 numParams;
    /* The initial priority of the entry thread. */
    u32 initPriority;
    /* The stack size of the entry thread. */
    u32 stackSize;
    /* The attributes of the entry thread. */
    u32 attr;  
} SceModuleEntryThread;

extern char _gp[];

/** 
 * Module type attributes. 
 */
enum SceModuleAttribute {
    /** No module attributes. */
    SCE_MODULE_ATTR_NONE             = 0x0000,
    /** Resident module - stays in memory. You cannot unload such a module. */
    SCE_MODULE_ATTR_CANT_STOP        = 0x0001,
    /** 
     * Only one instance of the module (one version) can be loaded into the system. If you want to load another 
     * version of that module, you have to delete the loaded version first.
     */
    SCE_MODULE_ATTR_EXCLUSIVE_LOAD   = 0x0002,
    /** 
     * Only one instance of the module (one version) can be started. If you want to start another 
     * version of that module, you have to stop the currently running version first.
     */
    SCE_MODULE_ATTR_EXCLUSIVE_START  = 0x0004,
};

/** 
 * Module Privilege Levels - These levels define the permissions a 
 * module can have.
 */
enum SceModulePrivilegeLevel {
    /** Lowest permission. */
    SCE_MODULE_USER                 = 0x0000,
    /** POPS/Demo. */
    SCE_MODULE_MS                   = 0x0200,
    /** Module Gamesharing. */
    SCE_MODULE_USB_WLAN             = 0x0400,
    /** Application module. */
    SCE_MODULE_APP                  = 0x0600,
    /** VSH module. */
    SCE_MODULE_VSH                  = 0x0800,
    /** Highest permission. */
    SCE_MODULE_KERNEL               = 0x1000,
    /** The module uses KIRK's memlmd resident library. */
    SCE_MODULE_KIRK_MEMLMD_LIB      = 0x2000,
    /** The module uses KIRK's semaphore resident library. */
    SCE_MODULE_KIRK_SEMAPHORE_LIB   = 0x4000,
};

#define SCE_MODULE_PRIVILEGE_LEVELS     (SCE_MODULE_MS | SCE_MODULE_USB_WLAN | SCE_MODULE_APP | SCE_MODULE_VSH | SCE_MODULE_KERNEL)

#define SCE_MODINFO_SECTION_NAME        ".rodata.sceModuleInfo"

#define SDK_VERSION                     0x06060010 /** Release X.Y.Z -> 0xXXYYZZZZ */
#define SCE_SDK_VERSION(ver)            const int module_sdk_version = ver

/**
 * module START thread return value
 */
#define SCE_KERNEL_START_SUCCESS        (0) /** The module could be started successfully. */
#define SCE_KERNEL_START_FAIL           (1) /** The module could not be started successfully. */

#define SCE_KERNEL_RESIDENT             (SCE_KERNEL_START_SUCCESS) /** After executing its start function, the module will remain in memory (resident library). */
#define SCE_KERNEL_NO_RESIDENT          (SCE_KERNEL_START_FAIL) /** The module will be unloaded after executing its start function. */

/**
 * module STOP thread return value
 */
#define SCE_KERNEL_STOP_SUCCESS         (0) /** The module could be stopped successfully. */
#define SCE_KERNEL_STOP_FAIL            (1) /** The module could not be stopped successfully. */

/**
 * Module entry functions.
 */
#define SCE_MODULE_BOOTSTART(name)      int module_start(SceSize argSize, const void *argBlock) __attribute__((alias(name))); \
                                        int module_bootstart(SceSize argSize, const void *argBlock) __attribute__((alias(name)))

#define SCE_MODULE_REBOOT_BEFORE(name)  int module_reboot_before(void) __attribute__((alias(name)))
#define SCE_MODULE_REBOOT_PHASE(name)   int module_reboot_phase(void) __attribute__((alias(name)))
#define SCE_MODULE_STOP(name)           int module_stop(void) __attribute__((alias(name)))

#define SCE_MODULE_INFO(name, attributes, majorVersion, minorVersion) \
    __asm__ (                                                       \
    "    .set push\n"                                               \
    "    .section .lib.ent.top, \"a\", @progbits\n"                 \
    "    .align 2\n"                                                \
    "    .word 0\n"                                                 \
    "__lib_ent_top:\n"                                              \
    "    .section .lib.ent.btm, \"a\", @progbits\n"                 \
    "    .align 2\n"                                                \
    "__lib_ent_bottom:\n"                                           \
    "    .word 0\n"                                                 \
    "    .section .lib.stub.top, \"a\", @progbits\n"                \
    "    .align 2\n"                                                \
    "    .word 0\n"                                                 \
    "__lib_stub_top:\n"                                             \
    "    .section .lib.stub.btm, \"a\", @progbits\n"                \
    "    .align 2\n"                                                \
    "__lib_stub_bottom:\n"                                          \
    "    .word 0\n"                                                 \
    "    .set pop\n"                                                \
    "    .text\n"                                                   \
    );                                                              \
    extern char __lib_ent_top[], __lib_ent_bottom[];                \
    extern char __lib_stub_top[], __lib_stub_bottom[];              \
    const SceModuleInfo module_info                                 \
        __attribute__((section(SCE_MODINFO_SECTION_NAME),            \
                   aligned(16), unused)) = {                        \
      attributes, { minorVersion, majorVersion }, name, 0, _gp,   \
      __lib_ent_top, __lib_ent_bottom,                              \
      __lib_stub_top, __lib_stub_bottom                             \
    }

#define SCE_MODULE_START_THREAD_PARAMETER(numParams, initPriority, stackSize, attr) \
    const SceModuleEntryThread module_start_thread_parameter = { numParams, initPriority, stackSize, attr };

#define SCE_MODULE_STOP_THREAD_PARAMETER(numParams, initPriority, stackSize, attr) \
    const SceModuleEntryThread module_stop_thread_parameter = { numParams, initPriority, stackSize, attr };

