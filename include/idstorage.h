/* Copyright (C) The uOFW team
   See the file COPYING for copying permission.
*/

#ifndef IDSTORAGE_H
#define IDSTORAGE_H

#include "common_header.h"
#include "chkreg.h"

#define SCE_ID_STORAGE_LEAF_SIZE   512

#define SCE_ID_STORAGE_LEAF_ID_CONSOLE_ID           0x100
#define SCE_ID_STORAGE_LEAF_ID_UMD_1                0x102
#define SCE_ID_STORAGE_LEAF_ID_UMD_2                0x103
#define SCE_ID_STORAGE_LEAF_ID_UMD_3                0x104
#define SCE_ID_STORAGE_LEAF_ID_UMD_4                0x105
#define SCE_ID_STORAGE_LEAF_ID_UMD_5                0x106

#define SCE_ID_STORAGE_LEAF_ID_BACKUP_CONSOLE_ID    0x120

#define SCE_ID_STORAGE_LEAF_CONSOLE_ID_OFFSET_BLOCK_CONSOLE_ID_CERTIFICATE    56

/*
 * This structure contains the consoleId ("PSID") and other unknown data, plus a signature
 * of the consoleId + other data (168 byte in total) used to verify the correctness of the consoleId.
 */
typedef struct {
	/* Unique per-console identifier. */
	SceConsoleId consoleId; // 0
	u8 unknowData[152]; // 16
	/* Signature of consoleId + unknownData used to verify its correctness. */
	u8 hash[16]; // 168
} SceIdStorageLeafConsoleIdBlockConsoleIdCertificate; // size = 184

typedef struct {
	/* Unknown data. */
	u8 unknownData[56]; // 0
	SceIdStorageLeafConsoleIdBlockConsoleIdCertificate blockConsoleIdCertificate; // 56
	/* Unknown. */
	u8 unknownData[SCE_ID_STORAGE_LEAF_SIZE - (56 + sizeof(SceIdStorageLeafConsoleIdBlockConsoleIdCertificate))]; // 240
} SceIdStorageLeafConsoleId;

typedef struct {
	u8 unknownData[SCE_ID_STORAGE_LEAF_SIZE];
} SceIdStorageLeafUMD1;

typedef struct {
	u8 unknownData[SCE_ID_STORAGE_LEAF_SIZE];
} SceIdStorageLeafUMD2;

typedef struct {
	u8 unknownData[SCE_ID_STORAGE_LEAF_SIZE];
} SceIdStorageLeafUMD3;

typedef struct {
	u8 unknownData[SCE_ID_STORAGE_LEAF_SIZE];
} SceIdStorageLeafUMD4;

typedef struct {
	u8 unknownData[SCE_ID_STORAGE_LEAF_SIZE];
} SceIdStorageLeafUMD5;

s32 sceIdStorageLookup(u16 key, u32 offset, void *pBuf, u32 len);
s32 sceIdStorageReadLeaf(u16 id, void *buf);

#endif // IDSTORAGE_H
