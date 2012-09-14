/* Copyright (C) 2011, 2012 The uOFW team
   See the file COPYING for copying permission.
*/

#ifndef COMMON_INCLUDED
# error "Only include common_imp.h or common_header.h!"
#endif

#define SCE_MODULE_NAME_LEN         (27)

typedef struct  {
    u16 modAttribute;
    u8 modVersion[2];
    char modName[SCE_MODULE_NAME_LEN];
    s8 terminal;
    void *gpValue;
    void *entTop;
    void *entEnd;
    void *stubTop;
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
    SCE_MODULE_APP                  = 0x0600,
    SCE_MODULE_VSH                  = 0x0800,
    /** Highest permission. */
    SCE_MODULE_KERNEL               = 0x1000,
    /** The module uses KIRK's memlmd resident library. */
    SCE_MODULE_KIRK_MEMLMD_LIB      = 0x2000,
    /** The module uses KIRK's semaphore resident library. */
    SCE_MODULE_KIRK_SEMAPHORE_LIB   = 0x4000,
};

#define SDK_VERSION                     0x06060010
#define SCE_SDK_VERSION(ver)            const int module_sdk_version = ver

#define SCE_MODULE_BOOTSTART(name)      int module_start(int arglen, void *argp) __attribute__((alias(name))); \
                                        int module_bootstart(int arglen, void *argp) __attribute__((alias(name)))

#define SCE_MODULE_REBOOT_BEFORE(name)  int module_reboot_before(void) __attribute__((alias(name)))
#define SCE_MODULE_REBOOT_PHASE(name)   int module_reboot_phase(void) __attribute__((alias(name)))
#define SCE_MODULE_STOP(name)           int module_stop(void) __attribute__((alias(name)))

#define SCE_MODULE_INFO(name, attributes, major_version, minor_version) \
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
        __attribute__((section(".rodata.sceModuleInfo"),            \
                   aligned(16), unused)) = {                        \
      attributes, { minor_version, major_version }, name, 0, _gp,   \
      __lib_ent_top, __lib_ent_bottom,                              \
      __lib_stub_top, __lib_stub_bottom                             \
    }

#define SCE_MODULE_START_THREAD_PARAMETER(numParams, initPriority, stackSize, attr) \
    const SceModuleEntryThread module_start_thread_parameter = { numParams, initPriority, stackSize, attr };

#define SCE_MODULE_STOP_THREAD_PARAMETER(numParams, initPriority, stackSize, attr) \
    const SceModuleEntryThread module_stop_thread_parameter = { numParams, initPriority, stackSize, attr };

