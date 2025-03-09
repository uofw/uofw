/* Copyright (C) 2024 The uOFW team
See the file COPYING for copying permission.
*/

#ifndef REGISTRY_H
#define REGISTRY_H

#include "common_header.h"

enum RegKeyTypes {
	/** Key is a directory */
	REG_TYPE_DIR = 1,
	/** Key is an integer (4 bytes) */
	REG_TYPE_INT = 2,
	/** Key is a string */
	REG_TYPE_STR = 3,
	/** Key is a binary string */
	REG_TYPE_BIN = 4,
};

struct RegParam {
	unsigned int regtype;     /* 0x0, set to 1 only for system */
	/** Seemingly never used, set to ::SYSTEM_REGISTRY */
	char name[256];        /* 0x4-0x104 */
	/** Length of the name */
	unsigned int namelen;     /* 0x104 */
	/** Unknown, set to 1 */
	unsigned int unk2;     /* 0x108 */
	/** Unknown, set to 1 */
	unsigned int unk3;     /* 0x10C */
};

s32 sceRegOpenRegistry(struct RegParam *reg, int mode, u32 *regHandle);
s32 sceRegOpenCategory(u32 regHandle, const char *dirName, int mode, u32 *catHandle);
s32 sceRegGetKeyInfo(u32 catHandle, const char *keyName, u32 *keyHandle, u32 *type, SceSize *size);
s32 sceRegGetKeyValue(u32 catHandle, u32 keyHandle, void *buf, SceSize size);
s32 sceRegFlushCategory(u32 catHandle);
s32 sceRegCloseCategory(u32 catHandle);
s32 sceRegFlushRegistry(u32 regHandle);
s32 sceRegCloseRegistry(u32 regHandle);

#endif /* REGISTRY_H */
