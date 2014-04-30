/** Copyright (C) 2014 The uOFW team
   See the file COPYING for copying permission.
*/

#ifndef IDSTORAGE_H
#define IDSTORAGE_H

#include "common_header.h"

typedef void (*sceIdStorageEnumCB)(u16 id, s32 ppn, void *opt);

s32 sceIdStorageInit(void);
s32 sceIdStorageEnd(void);
s32 sceIdStorageGetLeafSize(void);
s32 sceIdStorageIsFormatted(void);
s32 sceIdStorageIsReadOnly(void);
s32 sceIdStorageIsDirty(void);
s32 sceIdStorageFormat(void);
s32 sceIdStorageUnformat(void);
s32 sceIdStorageReadLeaf(u16 id, void *buf);
s32 sceIdStorageWriteLeaf(u16 id, void *buf);
s32 sceIdStorageFlush(void);
s32 sceIdStorageGetFreeLeaves(void);
s32 sceIdStorageEnumId(sceIdStorageEnumCB cb, void *opt);
s32 sceIdStorageCreateLeaf(u16 id);
s32 sceIdStorageCreateAtomicLeaves(u16 *ids, s32 size);
s32 sceIdStorageDeleteLeaf(u16 id);
s32 sceIdStorageLookup(u16 id, u32 offset, void *buf, u32 len);
s32 sceIdStorageUpdate(u16 id, u32 offset, void *buf, u32 len);

#endif /* IDSTORAGE_H */

