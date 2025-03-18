/* Copyright (C) 2011, 2012, 2013 The uOFW team
See the file COPYING for copying permission.
*/

#ifndef UMDMAN_H
#define UMDMAN_H

#include "common_header.h"

// TODO: unfinished header

typedef struct {
    u32 unk0;
    u32 cmd;
    u32 lbn;
    u32 lbnSize;
    u32 size;
    // how many bytes to read from sectors into 64 bytes aligned output
    u32 byteSizeMiddle;
    // how many bytes to read from the first read sector into not 64 bytes aligned output
    u32 byteSizeFirst;
    // how many bytes to read from the last read sector into not 64 bytes aligned output
    u32 byteSizeLast;
    u32 unk20;
} LbnParams;

typedef struct {
    u32 unk0;
    u32 lbn;
    u32 unk8;
    u32 lbnSize;
} PrepareIntoCacheParams;

s32 sceUmdMan_driver_65E2B3E0();
s32 sceUmdManUnRegisterInsertEjectUMDCallBack(s32 id);
s32 sceUmdManRegisterInsertEjectUMDCallBack(s32 id, void *cb, void *cbArg);

#endif /* UMDMAN_H */
