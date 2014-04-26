/* Copyright (C) 2011, 2012, 2013, 2014 The uOFW team
   See the file COPYING for copying permission.
*/

/*
 * uOFW/trunk/src/idstorage/idstorage.c
 */

#include "idstorage_int.h"
#include <lowio_sysreg.h>
#include <sysmem_sysevent.h>
#include <sysmem_sysclib.h>
#include <sysmem_kdebug.h>
#include <threadman_kernel.h>
#include <usersystemlib_kernel.h>
#include <modulemgr.h>

static s32 _sceIdStorageSysEventHandler(s32 id, char* name, void *param, s32 *res);

SCE_MODULE_INFO(
    "sceIdStorage_Service",
    SCE_MODULE_KERNEL |
    SCE_MODULE_ATTR_CANT_STOP | SCE_MODULE_ATTR_EXCLUSIVE_LOAD | SCE_MODULE_ATTR_EXCLUSIVE_START,
    1, 4
);
SCE_MODULE_BOOTSTART("sceIdStorageModuleStart");
SCE_MODULE_REBOOT_BEFORE("sceIdStorageModuleRebootBefore");
SCE_SDK_VERSION(SDK_VERSION);

typedef struct {
    u16 keyId; // 0
    u8 state; // 2
} SceIdStorageItem;

typedef struct {
    s32 fpl; // 0
    s32 mutex; // 4
    s32 pagesPerBlock; // 8
    s32 unk0; // 12
    u8 formatted; // 16
    u8 readOnly; // 17
    u8 dirty; // 18
    s16 blockPos; // 20
    u32 scramble; // 24
    u16 index[512]; // 28
    SceIdStorageItem store[32]; // 1052
    void *pool; // 1180
} SceIdStorage;

SceSysEventHandler g_sysev = { //0x2040
    .size = 64,
    .name = "SceIdStorage",
    .typeMask = 0x00FFFF00,
    .handler = _sceIdStorageSysEventHandler,
    .gp = 0,
    .busy = 0,
    .next = NULL,
    .reserved = {0}
};
SceIdStorage g_idst; //0x2080

//0x00000000
s32 sceIdStorageModuleStart(SceSize args __attribute__((unused)), void *argp __attribute__((unused)))
{
    sceIdStorageInit();

    return SCE_KERNEL_RESIDENT;
}

//0x00000020
s32 sceIdStorageModuleRebootBefore(SceSize args __attribute__((unused)), void *argp __attribute__((unused)))
{
    sceIdStorageEnd();

    return SCE_KERNEL_STOP_SUCCESS;
}

//0x00000040
s32 sceIdStorageInit(void)
{
    u32 digest[5]; //sp+0
    u32 magic[4]; //sp+32
    u8 extra[16]; //sp+48
    u64 fuse;
    u32 pos;
    s32 state;
    s32 res;

    memset(&g_idst, 0, sizeof(g_idst));

    /* Generate scramble seed, used for nand page decryption */

    fuse = sceSysregGetFuseId();
    magic[0] = fuse;
    magic[1] = fuse >> 32;
    magic[2] = fuse << 1;
    magic[3] = 0xD41D8CD9;
    sceKernelUtilsSha1Digest(magic, sizeof(magic), digest);

    if (sceSysregGetTachyonVersion() > 0x004FFFFF) {
        /* PSP Slim and greater */
        g_idst.scramble = (digest[0] ^ digest[3]) + digest[1];
    }
    else {
        /* PSP Fat */
        g_idst.scramble = 0;
    }

    /* 32 pages per block */

    g_idst.pagesPerBlock = sceNandGetPagesPerBlock();

    /* Allocate pool, used to store key contents */

    res = sceKernelCreateFpl("SceIdStorage", 1, 1, 16384, 1, NULL);

    if (res < 0) {
        return res;
    }

    g_idst.fpl = res;

    res = sceKernelAllocateFpl(g_idst.fpl, &g_idst.pool, NULL);

    if (res < 0) {
        sceKernelDeleteFpl(g_idst.fpl);
        return res;
    }

    /* Create mutex */

    // TODO: proper flags
    // 0x200 = SCE_KERNEL_MUTEX_RECURSIVE
    res = sceKernelCreateMutex("SceIdStorage", 0x800 | 0x200 | 0x1, 0, 0);

    if (res < 0) {
        sceKernelDeleteFpl(g_idst.fpl);
        return res;
    }

    g_idst.mutex = res;

    /* Lock nand, disable write access */

    res = sceNandLock(0);

    if (res < 0) {
        sceKernelDeleteFpl(g_idst.fpl);
        return res;
    }

    g_idst.unk0 = -1;

    /* Find valid idstorage index block by reading the spare area */
    /* Reference: http://hitmen.c02.at/files/yapspd/psp_doc/chap19.html#sec19.3 */

    for (pos=0; pos<512; pos+=g_idst.pagesPerBlock) {
        res = sceNandReadExtraOnly(1536 + pos, extra, 1);

        /* Read failed */
        if (res != 0) {
            state = res;
            continue;
        }
        /* Block is not valid */
        if (extra[5] != 255) {
            state = -1;
            continue;
        }
        if (extra[6] == 201 && extra[7] == 228 && extra[8] == 90 && extra[9] == 165) {
            g_idst.readOnly = 1;
            g_idst.unk0 = 0;
            break;
        }

        /* Verify spare aera ECC */
        state = sceNandCorrectEcc(&extra[4], ((extra[13] << 8) | extra[12]) & 0xFFF);
        if (state == -3) {
            continue;
        }
        /* Check for idstorage index */
        if (extra[6] != 115) {
            continue;
        }

        g_idst.unk0 = extra[7];

        /* Second check */
        if (extra[8] < 2) {
            /* Index is read-only */
            if (extra[9] >= 2) {
                g_idst.readOnly = 1;
            }

            /* Valid index found */
            break;
        }

        state = -1;
    }

    if (g_idst.unk0 == 0) {
        state = -1;
    }

    /* Read idstorage index (1024 bytes) and store it */

    sceNandSetScramble(g_idst.scramble);

    if (state == 0) {
        u32 i;

        for (i=0; i<4; i++) {
            state = sceNandReadPages(1536 + pos, &g_idst.index, NULL, 2);

            if (state >= 0) {
                break;
            }
        }
    }

    if (state < 0) {
        Kprintf("ID storage is unformated\n");
        g_idst.formatted = 0;
    }
    else {
        g_idst.formatted = 1;
        g_idst.blockPos = pos;
    }

    sceKernelRegisterSysEventHandler(&g_sysev);
    sceNandUnlock();

    return SCE_ERROR_OK;
}

s32 sceIdStorageEnd(void)
{
    sceIdStorageFlush();
    sceKernelUnregisterSysEventHandler(&g_sysev);
    sceKernelDeleteMutex(g_idst.mutex);
    sceKernelDeleteFpl(g_idst.fpl);

    return SCE_ERROR_OK;
}

static s32 _sceIdStorageSysEventHandler(s32 id, char* name, void *param, s32 *res)
{
    (void)name, (void)param, (void)res;

    if (id == 256) {
        if (sceIdStorageIsDirty()) {
            return SCE_ERROR_BUSY;
        }
    }

    return SCE_ERROR_OK;
}

//sub_000003B0
static s32 _sceIdStorageLockMutex(void)
{
    return sceKernelLockMutex(g_idst.mutex, 1, 0);
}

//sub_000003D8
static s32 _sceIdStorageUnlockMutex(void)
{
    return sceKernelUnlockMutex(g_idst.mutex, 1);
}

//sub_000003FC
static s32 _sceIdStorageFindFreeBlock(void)
{
    s32 i, j;
    s32 pos = -1;

    for (i=0; i<512; i+=g_idst.pagesPerBlock) {
        for (j=0; j<g_idst.pagesPerBlock; j++) {
            if (g_idst.index[i + j] != 0xFFFF) {
                break;
            }
        }

        /* Block is filled with 0xFFFF, it's free */
        if (j == g_idst.pagesPerBlock) {
            pos = i;
            break;
        }
    }

    if (pos < 0) {
        return SCE_ERROR_OUT_OF_MEMORY;
    }

    return pos;
}

//sub_00000474
static s32 _sceIdStorageFindKeyPos(u16 id)
{
    s32 i;
    s32 pos = -1;

    for (i=0; i<512; i++) {
        if (g_idst.index[i] == id) {
            pos = i;
            break;
        }
    }

    if (pos < 0) {
        return SCE_ERROR_NOT_FOUND;
    }

    return pos;
}

//sub_000004C4
static s32 _sceIdStorageKeyStoreFind(u16 id)
{
    s32 i;

    for (i=0; i<32; i++) {
        if (g_idst.store[i].keyId == id && g_idst.store[i].state != 0) {
            return i;
        }
    }

    return -1;
}

//sub_00000510
static s32 _sceIdStorageKeyStoreInsert(u16 id)
{
    s32 i;

    for (i=0; i<32; i++) {
        if (g_idst.store[i].state == 0) {
            g_idst.store[i].keyId = id;
            g_idst.store[i].state = 1;
            return i;
        }
    }

    return -1;
}

//sub_0000055C
static s32 _sceIdStorageKeyStoreClear(void)
{
    s32 i;

    for (i=0; i<32; i++) {
        g_idst.store[i].state = 0;
    }

    return SCE_ERROR_OK;
}

s32 sceIdStorageGetLeafSize(void)
{
    return 512;
}

s32 sceIdStorageIsFormatted(void)
{
    return g_idst.formatted;
}

s32 sceIdStorageIsReadOnly(void)
{
    return g_idst.readOnly;
}

s32 sceIdStorageIsDirty(void)
{
    s32 i;

    if (g_idst.dirty) {
        return 1;
    }

    for (i=0; i<32; i++) {
        if (g_idst.store[i].state == 3) {
            return 1;
        }
    }

    return 0;
}

s32 sceIdStorageFormat(void)
{
    u8 spare[384]; //sp+0
    s32 res;
    s32 i, j;
    s32 pos;

    res = _sceIdStorageLockMutex();
    if (res < 0) {
        return res;
    }

    res = sceNandLock(1);
    if (res < 0) {
        _sceIdStorageUnlockMutex();
        return res;
    }

    sceKernelPowerLock(0);

    g_idst.dirty = 0;

    for (i=0; i<32; i++) {
        g_idst.store[i].state = 0;
    }

    for (i=0; i<512; i++) {
        g_idst.index[i] = 0xFFFF; // -1
    }

    for (i=0; i<512; i+=g_idst.pagesPerBlock) {
        if (!sceNandIsBadBlock(1536 + i)) {
            res = sceNandTestBlock(1536 + i);

            if (res < 0) {
                sceNandDoMarkAsBadBlock(1536 + i);
            }
            else if (res == 0) {
                continue;
            }
        }

        for (j=0; j<g_idst.pagesPerBlock; j++) {
            g_idst.index[i + j] = 0xFFF0; // -16
        }
    }

    pos = _sceIdStorageFindFreeBlock();
    if (pos < 0) {
        sceKernelPowerUnlock(0);
        sceNandUnlock();
        _sceIdStorageUnlockMutex();
        return pos;
    }

    for (i=0; i<g_idst.pagesPerBlock; i++) {
        g_idst.index[pos + i] = 0xFFF5; // -11
    }

    res = sceNandEraseBlock(1536 + pos);
    if (res < 0) {
        sceKernelPowerUnlock(0);
        sceNandUnlock();
        _sceIdStorageUnlockMutex();
        return res;
    }

    sceNandSetScramble(g_idst.scramble);

    memset(&spare, 0xFF, sizeof(spare));

    for (i=0; i<g_idst.pagesPerBlock; i++) {
        spare[12*i + 2] = 115;
        spare[12*i + 3] = 1;
        spare[12*i + 4] = 1;
        spare[12*i + 5] = 1;
    }

    res = sceNandWritePages(1536 + pos, &g_idst.index, &spare, 2);
    if (res < 0) {
        sceKernelPowerUnlock(0);
        sceNandUnlock();
        _sceIdStorageUnlockMutex();
        return res;
    }

    g_idst.formatted = 1;
    g_idst.unk0 = 1;
    g_idst.blockPos = pos;
    g_idst.readOnly = 0;

    sceKernelPowerUnlock(0);
    sceNandUnlock();
    _sceIdStorageUnlockMutex();

    return SCE_ERROR_OK;
}

s32 sceIdStorageUnformat(void)
{
    s32 res;
    s32 i;

    res = sceNandLock(1);
    if (res < 0) {
        return res;
    }

    sceKernelPowerLock(0);

    for (i=0; i<512; i+=g_idst.pagesPerBlock) {
        sceNandEraseBlock(1536 + i);
    }

    sceKernelPowerUnlock(0);
    sceNandUnlock();

    g_idst.formatted = 0;

    return SCE_ERROR_OK;
}

s32 sceIdStorageReadLeaf(u16 id, void *buf)
{
    u8 extra[16]; //sp+0
    s32 res;
    s32 i;
    s32 pos;
    s32 idx;

    if (id > 0xFFEF) {
        return SCE_ERROR_INVALID_ID;
    }

    if (!sceIdStorageIsFormatted()) {
        return SCE_ERROR_INVALID_FORMAT;
    }

    res = _sceIdStorageLockMutex();
    if (res < 0) {
        return res;
    }

    pos = _sceIdStorageFindKeyPos(id);
    if (pos < 0) {
        _sceIdStorageUnlockMutex();
        return pos;
    }

    idx = _sceIdStorageKeyStoreFind(pos);
    if (idx < 0) {
        idx = _sceIdStorageKeyStoreInsert(pos);
        if (idx < 0) {
            res = sceIdStorageFlush();
            if (res < 0) {
                _sceIdStorageUnlockMutex();
                return res;
            }

            pos = _sceIdStorageFindKeyPos(id);
            if (pos < 0) {
                _sceIdStorageUnlockMutex();
                return pos;
            }

            idx = _sceIdStorageKeyStoreInsert(pos);
        }

        for (i=0; i<4; i++) {
            res = sceNandLock(0);
            if (res < 0) {
                _sceIdStorageUnlockMutex();
                return res;
            }

            sceNandSetScramble(g_idst.scramble);

            res = sceNandReadPagesRawExtra(pos, g_idst.pool + 512*idx, &extra, 1);
            sceNandUnlock();
            if (res >= 0) {
                break;
            }

            /* Page read failed on last attempt */
            if (i == 3) {
                g_idst.store[idx].state = 0;
                _sceIdStorageUnlockMutex();
                return res;
            }
        }

        g_idst.store[idx].state = 2;
    }

    /* NOTE: asm code seems to contain inlined memcpy */
    memcpy(buf, g_idst.pool + 512*idx, 512);

    _sceIdStorageUnlockMutex();

    return SCE_ERROR_OK;
}

s32 sceIdStorageWriteLeaf(u16 id, void *buf)
{
    s32 res;
    s32 pos;
    s32 idx;

    if (id > 0xFFEF) {
        return SCE_ERROR_INVALID_ID;
    }

    if (!sceIdStorageIsFormatted()) {
        return SCE_ERROR_INVALID_FORMAT;
    }

    if (sceIdStorageIsReadOnly()) {
        return -1;
    }

    res = _sceIdStorageLockMutex();
    if (res < 0) {
        return res;
    }

    pos = _sceIdStorageFindKeyPos(id);
    if (pos < 0) {
        _sceIdStorageUnlockMutex();
        return pos;
    }

    idx = _sceIdStorageKeyStoreFind(pos);
    if (idx < 0) {
        idx = _sceIdStorageKeyStoreInsert(pos);
        if (idx < 0) {
            res = sceIdStorageFlush();
            if (res < 0) {
                _sceIdStorageUnlockMutex();
                return res;
            }

            pos = _sceIdStorageFindKeyPos(id);
            if (pos < 0) {
                _sceIdStorageUnlockMutex();
                return pos;
            }

            idx = _sceIdStorageKeyStoreInsert(pos);
        }
    }

    /* NOTE: asm code seems to contain inlined memcpy */
    memcpy(g_idst.pool + 512*idx, buf, 512);

    g_idst.store[idx].state = 3;
    _sceIdStorageUnlockMutex();

    return SCE_ERROR_OK;
}

s32 sceIdStorageFlush(void)
{
    s32 res;

    if (!sceIdStorageIsFormatted()) {
        return SCE_ERROR_INVALID_FORMAT;
    }

    res = _sceIdStorageLockMutex();
    if (res < 0) {
        return res;
    }

    if (sceIdStorageIsDirty()) {
        // TODO: reverse associated func
        res = sceKernelExtendKernelStack(0x8000, (void*)0xCC8, NULL);
        if (res < 0) {
            return res;
        }
    }

    _sceIdStorageKeyStoreClear();
    _sceIdStorageUnlockMutex();

    return SCE_ERROR_OK;
}

s32 sceIdStorageGetFreeLeaves(void)
{
    s32 intr;
    s32 i;
    s32 count = 0;

    if (!sceIdStorageIsFormatted()) {
        return SCE_ERROR_INVALID_FORMAT;
    }

    intr = sceKernelCpuSuspendIntr();

    for (i=0; i<512; i++) {
        if (g_idst.index[i] == 0xFFFF) {
            count++;
        }
    }

    sceKernelCpuResumeIntr(intr);

    return count;
}

s32 sceIdStorageEnumId(void (*cb)(u16 id, s32 ppn, void *opt), void *opt)
{
    s32 res;
    s32 i;

    res = _sceIdStorageLockMutex();
    if (res < 0) {
        return res;
    }

    for (i=0; i<512; i++) {
        if (g_idst.index[i] <= 0xFFEF) {
            cb(g_idst.index[i], 1536 + i, opt);
        }
    }

    _sceIdStorageUnlockMutex();

    return SCE_ERROR_OK;
}

s32 sceIdStorageCreateLeaf(u16 id)
{
    s32 i, j;
    s32 intr;
    s32 pos;
    s32 count;
    s32 min;

    if (!sceIdStorageIsFormatted()) {
        return SCE_ERROR_INVALID_FORMAT;
    }

    if (sceIdStorageIsReadOnly()) {
        return -1;
    }

    if (id > 0xFFEF) {
        return SCE_ERROR_INVALID_ID;
    }

    intr = sceKernelCpuSuspendIntr();

    /* Check if the key is already in the index */
    if (_sceIdStorageFindKeyPos(id) >= 0) {
        sceKernelCpuResumeIntr(intr);
        return SCE_ERROR_ALREADY;
    }

    /* Attempt chaining */
    if (id != 0) {
        pos = _sceIdStorageFindKeyPos(id-1);

        if (0 <= pos && pos < 511) {
            if (g_idst.index[pos+1] == 0xFFFF) {
                g_idst.index[pos+1] = id;
                g_idst.dirty = 1;
                sceKernelCpuResumeIntr(intr);
                return SCE_ERROR_OK;
            }
        }
    }

    min = g_idst.pagesPerBlock + 1;
    pos = -1;

    /* Find the most fully used block with at least one unused page */
    for (i=0; i<512; i+=g_idst.pagesPerBlock) {
        count = 0;

        for (j=0; j<g_idst.pagesPerBlock; j++) {
            if (g_idst.index[i + j] == 0xFFFF) {
                count++;
            }
        }

        if (0 < count && count < min) {
            min = count;
            pos = i;
        }
    }

    /* No suitable block found */
    if (pos < 0) {
        sceKernelCpuResumeIntr(intr);
        return SCE_ERROR_OUT_OF_MEMORY;
    }

    /* Reserve first unused page of the block */
    for (i=0; i<g_idst.pagesPerBlock; i++) {
        if (g_idst.index[pos + i] == 0xFFFF) {
            g_idst.index[pos + i] = id;
            break;
        }
    }

    g_idst.dirty = 1;
    sceKernelCpuResumeIntr(intr);

    return SCE_ERROR_OK;
}

s32 sceIdStorageCreateAtomicLeaves(u16 *ids, s32 size)
{
    s32 i, j, k;
    s32 intr;
    s32 count;

    if (!sceIdStorageIsFormatted()) {
        return SCE_ERROR_INVALID_FORMAT;
    }

    if (sceIdStorageIsReadOnly()) {
        return -1;
    }

    if (size <= 0 || g_idst.pagesPerBlock < size) {
        return SCE_ERROR_INVALID_SIZE;
    }

    intr = sceKernelCpuSuspendIntr();

    /* Verify set of keys */
    for (i=0; i<size; i++) {
        if (ids[i] > 0xFFEF) {
            sceKernelCpuResumeIntr(intr);
            return SCE_ERROR_INVALID_ID;
        }

        if (_sceIdStorageFindKeyPos(ids[i]) >= 0) {
            sceKernelCpuResumeIntr(intr);
            return SCE_ERROR_ALREADY;
        }
    }

    for (i=0; i<512; i+=g_idst.pagesPerBlock) {
        /* Check against index if the block contains enough unused pages */
        count = 0;

        for (j=0; j<g_idst.pagesPerBlock; j++) {
            if (g_idst.index[i + j] == 0xFFFF) {
                count++;
            }
        }

        if (count < size) {
            continue;
        }

        /* Reserve early unused pages of the block with set of keys */
        k = 0;

        for (j=0; j<g_idst.pagesPerBlock; j++) {
            if (g_idst.index[i + j] == 0xFFFF) {
                g_idst.index[i + j] = ids[k++];

                if (k == size) {
                    break;
                }
            }
        }
    }

    g_idst.dirty = 1;
    sceKernelCpuResumeIntr(intr);

    return SCE_ERROR_OK;
}

