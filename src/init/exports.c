#include <pspmoduleexport.h>
#define NULL ((void *) 0)

extern char module_bootstart;
extern char module_info;
extern char module_sdk_version;
static const unsigned int __syslib_exports[6] __attribute__((section(".rodata.sceResident"))) = {
	0xD3744BE0,
	0xF01D73A7,
	0x11B97506,
	(unsigned int) &module_bootstart,
	(unsigned int) &module_info,
	(unsigned int) &module_sdk_version,
};

const struct SceLibraryEntry __library_exports[1] __attribute__((section(".lib.ent"), used)) = {
	{ NULL, 0x0000, 0x8000, 4, 2, 1, &__syslib_exports },
};
