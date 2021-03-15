/* Copyright (C) The uOFW team
   See the file COPYING for copying permission.
*/

#include "common_header.h"

#ifndef IDSTORAGE_H
#define IDSTORAGE_H

#define SCE_ID_STORAGE_LEAF_SIZE   512

#define SCE_ID_STORAGE_LEAF_UNKNOWN_0x100    0x100
#define SCE_ID_STORAGE_LEAF_ID_UMD_1         0x102
#define SCE_ID_STORAGE_LEAF_ID_UMD_2         0x103
#define SCE_ID_STORAGE_LEAF_ID_UMD_3         0x104
#define SCE_ID_STORAGE_LEAF_ID_UMD_4         0x105
#define SCE_ID_STORAGE_LEAF_ID_UMD_5         0x106

typedef struct {
	u8 unknownData[SCE_ID_STORAGE_LEAF_SIZE];
} SceIdStorageLeafUnknown0x100;

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
