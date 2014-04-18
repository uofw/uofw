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
    s32 fpl; // 0
    s32 mutex; // 4
    s32 pagesPerBlock; // 8
    s32 unk0; // 12
    u8 formatted; // 16
    u8 readOnly; // 17
    u8 unk1[2]; // 18
    s16 block; // 20
    u32 scramble; // 24
    u16 data[512]; // 28
    u16 unk2[64]; // 1052
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
    (void)sceIdStorageInit();

    return SCE_KERNEL_RESIDENT;
}

//0x00000020
s32 sceIdStorageModuleRebootBefore(SceSize args __attribute__((unused)), void *argp __attribute__((unused)))
{
    (void)sceIdStorageEnd();

    return SCE_KERNEL_STOP_SUCCESS;
}

//0x00000040
s32 sceIdStorageInit(void)
{
    u32 digest[5]; //sp+0
    u32 magic[4];   //sp+32
    u8 extra[16]; //sp+48
    u64 fuse;
    u32 block;
    s32 state;
    s32 res;

    memset(&g_idst, 0, sizeof(g_idst));

    /* Generate scramble seed. Used for nand page decryption. */

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

    /* Pages */

    g_idst.pagesPerBlock = sceNandGetPagesPerBlock();

    /* Pool */

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

    /* Mutex */

    // TODO: proper flags
    // 0x200 = SCE_KERNEL_MUTEX_RECURSIVE
    res = sceKernelCreateMutex("SceIdStorage", 0x800 | 0x200 | 0x1, 0, 0);

    if (res < 0) {
        sceKernelDeleteFpl(g_idst.fpl);
        return res;
    }

    g_idst.mutex = res;

    /* Lock nand */

    res = sceNandLock(0);

    if (res < 0) {
        sceKernelDeleteFpl(g_idst.fpl);
        return res;
    }

    g_idst.unk0 = -1;

    /* Find valid idstorage block */

    for (block=0; block<512; block+=g_idst.pagesPerBlock) {
        res = sceNandReadExtraOnly(1536 + block, extra, 1);

        if (res != 0) {
            state = res;
            continue;
        }
        if (extra[5] != 255) {
            state = -1;
            continue;
        }
        if (extra[6] == 201 && extra[7] == 228 && extra[8] == 90 && extra[9] == 165) {
            g_idst.readOnly = 1;
            g_idst.unk0 = 0;
            break;
        }

        state = sceNandCorrectEcc(&extra[4], ((extra[13] << 8) | extra[12]) & 0xFFF);
        if (state == -3) {
            continue;
        }
        if (extra[6] == 115) {
            continue;
        }

        g_idst.unk0 = extra[7];

        if (extra[8] < 2) {
            if (extra[9] >= 2) {
                g_idst.readOnly = 1;
            }
            break;
        }

        state = -1;
    }

    if (g_idst.unk0 == 0) {
        state = -1;
    }

    (void)sceNandSetScramble(g_idst.scramble);

    /* Read idstorage nand block and store it */

    if (state == 0) {
        u32 i;

        for (i=0; i<4; i++) {
            state = sceNandReadPages(1536 + block, &g_idst.data, NULL, 2);

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
        g_idst.block = block;
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
    s32 size = -1;

    for (i=0; i<512; i+=g_idst.pagesPerBlock) {
        for (j=0; j<g_idst.pagesPerBlock; j++) {
            if (g_idst.data[i + j] != 0xFFFF) {
                break;
            }
        }

        /* Block is filled with 0xFFFF, it's free */
        if (j == g_idst.pagesPerBlock) {
            size = 512;
            break;
        }
    }

    if (size < 0) {
        return SCE_ERROR_OUT_OF_MEMORY;
    }

    return size;
}

//sub_00000474
static s32 _sceIdStorageFindPage(u16 data)
{
    s32 i;
    s32 page = -1;

    for (i=0; i<512; i++) {
        if (g_idst.data[i] == data) {
            page = i;
            break;
        }
    }

    if (page < 0) {
        return SCE_ERROR_NOT_FOUND;
    }

    return page;
}

//sub_000004C4
static s32 _sceIdStorage_4C4(s32 arg0)
{
    s32 i;

    for (i=0; i<32; i++) {
        if (g_idst.unk2[2*i] == arg0) {
            if (*(u8*)&g_idst.unk2[2*i+1] != 0) {
                return i;
            }
        }
    }

    return -1;
}

