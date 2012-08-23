/* Copyright (C) 2011, 2012 The uOFW team
   See the file COPYING for copying permission.
*/

#ifndef COMMON_INCLUDED
# error "Only include common_imp.h or common_header.h!"
#endif

typedef struct  {
    u16 modattribute;
    u8 modversion[2];
    s8 modname[27];
    s8 terminal;
    void *gp_value;
    void *ent_top;
    void *ent_end;
    void *stub_top;
    void *stub_end;
} SceModuleInfo;

typedef struct {
    u32 unk0;
    u32 unk4;
    u32 unk8;
    u32 unk12;
} SceThreadParameter;

extern char _gp[];

/* Module attributes. */
enum SceModuleInfoAttr {
    SCE_MODULE_USER         = 0,
    SCE_MODULE_NO_STOP      = 0x0001,
    SCE_MODULE_SINGLE_LOAD  = 0x0002,
    SCE_MODULE_SINGLE_START = 0x0004,
    SCE_MODULE_VSH          = 0x0800,
    SCE_MODULE_KERNEL       = 0x1000,
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

#define SCE_MODULE_START_THREAD_PARAMETER(unk1, unk2, unk3, unk4) \
    const SceThreadParameter module_start_thread_parameter = { unk1, unk2, unk3, unk4 };

#define SCE_MODULE_STOP_THREAD_PARAMETER(unk1, unk2, unk3, unk4) \
    const SceThreadParameter module_stop_thread_parameter = { unk1, unk2, unk3, unk4 };

