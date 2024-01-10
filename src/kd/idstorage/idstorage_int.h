/* Copyright (C) 2011, 2012, 2013, 2014 The uOFW team
   See the file COPYING for copying permission.
*/

#include <idstorage.h>

static s32 _sceIdStorageSysEventHandler(s32 id, char* name, void *param, s32 *res);
static s32 _sceIdStorageLockMutex(void);
static s32 _sceIdStorageUnlockMutex(void);
static s32 _sceIdStorageFindFreeBlock(void);
static s32 _sceIdStorageFindKeyPos(u16 id);
static s32 _sceIdStorageKeyStoreFind(u16 id);
static s32 _sceIdStorageKeyStoreInsert(u16 id);
static s32 _sceIdStorageKeyStoreClear(void);
static s32 _sceIdStorageFlushCB(void *arg);

