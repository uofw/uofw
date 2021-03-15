/* Copyright (C) The uOFW team
   See the file COPYING for copying permission.
*/

#include <common_imp.h>

#include <chkreg.h>
#include <crypto/kirk.h>
#include <idstorage.h>
#include <memlmd.h>
#include <threadman_kernel.h>

SCE_MODULE_INFO(
    "sceChkreg", 
    SCE_MODULE_KERNEL | SCE_MODULE_KIRK_SEMAPHORE_LIB | 
    SCE_MODULE_ATTR_EXCLUSIVE_LOAD | SCE_MODULE_ATTR_EXCLUSIVE_START, 
    1, 9);

SCE_MODULE_BOOTSTART("_sceChkregInit");
SCE_MODULE_STOP("_sceChkregEnd");

SCE_SDK_VERSION(SDK_VERSION);

// TODO: Cleanup/Verify globals
u32 g_unk;                  // 0x00000B30
u8 g_pscode[KIRK_CERT_LEN]; // 0x00001480
SceUID g_chkregOld_sema;       // 0x00001538
u32 g_unk2;                 // 0x00001540

typedef struct {
    u8 buf[0x200];    // 0x00000A00 - 0x00000BF4
    u8 unk2[0x14];    // 0x000009BC
} g_chkregOld_struct;

g_chkregOld_struct g_chkregOld = { { 0 }, { 0 } };

u32 g_unk1540; // 0x00001540

u8 g_unk1480[184]; // 0x00001480 -- size = 148

typedef struct {
    SceIdStorageLeafUMD1 umd1; // 0
    SceIdStorageLeafUMD2 umd2; // 512
    SceIdStorageLeafUMD3 umd3; // 1024
    SceIdStorageLeafUMD4 umd4; // 1536
    SceIdStorageLeafUMD5 umd5; // 2048
} SceChkregUMDData; // size = 5 * SCE_ID_STORAGE_LEAF_SIZE = 5 * 512 = 2560

typedef struct {
    SceChkregUMDData *pUmdData; // 0
    u8 *pData; // 4
} SceChkreg; // size = 8

SceChkregUMDData g_umdData[5]; // 0x00000A80

SceChkreg g_chkreg = {
    .pUmdData = &g_umdData,
    .pData = &g_unk1540,
}; // 0x00000A00

u8 g_unkA40; // 0x00000A40
u8 g_unkA44; // 0x00000A44
u8 g_unkA48; // 0x00000A48

SceUID g_semaId; // 0x00001538

// Subroutine sub_00000000 - Address 0x00000000
s32 sub_00000000(void) {
    s32 ret = 0;
    u32 error = SCE_ERROR_OUT_OF_MEMORY;

    u32 i = 0;
    for (i = 0; i < 5; i++) {
        ret = sceIdStorageReadLeaf((i + 0x102U) & 0xFFFF, g_chkregOld.buf);
        
        if ((ret < 0) && ((ret = sceIdStorageReadLeaf((i + 0x122) & 0xFFFF, g_chkregOld.buf)) < 0))
            return SCE_ERROR_NOT_FOUND;
    }

    ret = g_unk;
    i = 0;

    while(1) {
        g_unk = g_unk + 2;
        if (ret == 1) {
            ret = ((i << 3 | 4U) + g_unk);
            if (ret == 0) {
                g_chkregOld.buf[0x40] = i + 1;
                g_chkregOld.buf[0x44] = 1;
                error = 0;
            }
            if (ret == 0x70000000) {
                g_chkregOld.buf[0x40] = i + 1;
                g_chkregOld.buf[0x44] = 1;
                error = 0;
            }
        }
        
        if (0xff < i + 1)
            break;
            
        ret = g_unk;
        i = i + 1;
    }
    
    return error;
}

// Subroutine sub_00000128 - Address 0x00000128
s32 sub_00000128(s32 arg0, s32 arg1)
{
    u32 unk1 = 0;
    s32 unk2 = 0;
    u32 unk3 = 0;
    
    if (g_chkregOld.buf[0x40] != 0) {
        unk2 = g_unk;

        while(1) {
            unk1 = unk3 << 3;
            unk3++;
            g_unk += 2;
            
            if ((arg1 == unk2) && (arg0 == (s32)((unk1 | 4) + g_unk)))
                return 1;
            
            if (g_chkregOld.buf[0x40] <= unk3)
                break;
                
            unk2 = g_unk;
        }
    }

    return 0;
}

// Subroutine sub_00000190 - Address 0x00000190
s32 sub_00000190(void)
{
    if (sceIdStorageLookup(0x100, 0x38, g_pscode, KIRK_CERT_LEN) < 0) { // Read pscode into g_pscode from key 0x100
        if (sceIdStorageLookup(0x120, 0x38, g_pscode, KIRK_CERT_LEN) < 0) // Read (backup) pscode into g_pscode from key 0x120 if key 0x100 fails
            return SCE_ERROR_NOT_FOUND;
    }
    
    g_chkregOld.buf[0x48] = 1; // pscode found
    return 0;
}

// Subroutine sub_0000020C - Address 0x0000020C
s32 sub_0000020C(void)
{
    if (sceUtilsBufferCopyWithRange(NULL, 0, g_pscode, KIRK_CERT_LEN, KIRK_CMD_CERT_VER) != 0) // Validate g_pscode
        return SCE_ERROR_INVALID_FORMAT;
        
    return 0;
}

// Subroutine module_start - Address 0x00000248
s32 _sceChkregInit(SceSize args, void *argp)
{
    (void)args;
    (void)argp;

    // 0x00000258 - 0x0000026C
    u32 i;
    for (i = 0; i < (sizeof *g_chkreg.pUmdData); i++)
    {
        ((u8 *)g_chkreg.pUmdData)[i] = 0;
    }

    // 0x00000270 - 0x0000028C
    for (i = 0; i < (sizeof g_unk1480 / sizeof g_unk1480[0]); i++)
    {
        g_unk1480[i] = 0;
    }

    g_unkA40 = 0; // 0x0x000002B0
    g_unkA44 = 0;
    g_unkA48 = 0;

    /* Create a module semaphore which acts as a mutex. */
    g_semaId = sceKernelCreateSema("SceChkreg", SCE_KERNEL_SA_THFIFO, 1, 1, NULL); // 0x000002BC

    /* If the semaphore could not be created successfully, unload the module. Otherwise keep it in memory. */
    return (g_semaId < 1)
        ? SCE_KERNEL_NO_RESIDENT
        : SCE_KERNEL_RESIDENT;
}

// Subroutine module_stop - Address 0x000002E0
s32 _sceChkregEnd(SceSize args, void *argp)
{
    (void)args;
    (void)argp;

    s32 status;
    SceUInt timeout;

    timeout = 1 * 1000 * 1000;

    // 0x00000300 - 0x00000314
    u32 i;
    for (i = 0; i < (sizeof *g_chkreg.pUmdData); i++)
    {
        ((u8 *)g_chkreg.pUmdData)[i] = 0;
    }

    // 0x00000318 - 0x00000334
    for (i = 0; i < (sizeof g_unk1480 / sizeof g_unk1480[0]); i++)
    {
        g_unk1480[i] = 0;
    }

    g_unkA40 = 0; // 0x00000348
    g_unkA44 = 0;
    g_unkA48 = 0;

    /* Only stop the module when it is not currently used by other modules. */

    status = sceKernelWaitSema(g_semaId, 1, &timeout); // 0x0x0000035C
    if (status == SCE_ERROR_OK) // 0x0x00000364
    {
        /* Module no longer in use. Stop it. */
        sceKernelDeleteSema(g_semaId); // 0x00000380

        return SCE_KERNEL_STOP_SUCCESS;
    }

    /* Module still in use, don't stop it yet. */

    SCE_KERNEL_STOP_FAIL;
}

// Subroutine sceChkreg_driver_59F8491D - Address 0x00000438
s32 sceChkregGetPsCode(u8 *code)
{
    s32 ret = SCE_ERROR_SEMAPHORE;

    if ((sceKernelWaitSema(g_chkregOld_sema, 1, NULL)) == 0) {
        if (((g_chkregOld.buf[0x48] != 0) || ((ret = sub_00000190()) == 0)) && ((ret = sub_0000020C()) == 0)) {
            code[0] = g_pscode[1];
            code[1] = g_pscode[0];
            code[2] = g_pscode[3];
            code[3] = g_pscode[2];
            code[4] = g_pscode[5];
            code[5] = g_pscode[4];
            code[6] = g_pscode[6] >> 2;
            code[7] = 0;
        }
        
        if ((sceKernelSignalSema(g_chkregOld_sema, 1)) != 0)
            ret = SCE_ERROR_SEMAPHORE;
    }
    
    return ret;
}

// Subroutine sceChkreg_driver_54495B19 - Address 0x00000390
s32 sceChkregCheckRegion(u32 arg0, u32 arg1)
{
    s32 ret = 0;
    
    if (sceKernelWaitSema(g_chkregOld_sema, 1, NULL) == 0) {
        if ((g_chkregOld.buf[0x44] != 0) || ((ret = sub_00000000()) == 0))
            ret = sub_00000128(0x80000000, (arg0 | arg1));
        
        if (sceKernelSignalSema(g_chkregOld_sema, 1) != 0)
            ret = SCE_ERROR_SEMAPHORE;
    }

    return ret;
}

// Subroutine sceChkreg_driver_6894A027 - Address 0x000006B8 -- Done
s32 sceChkreg_driver_6894A027(u8 *arg0, s32 arg1) {
    s32 ret = SCE_ERROR_INVALID_INDEX;

    if (arg1 == 0) {
        ret = SCE_ERROR_SEMAPHORE;

        if ((sceKernelWaitSema(g_chkregOld_sema, 1, NULL)) == 0) {
            if (((g_chkregOld.buf[0x48] != 0) || ((ret = sub_00000190()) == 0)) && ((ret = sub_0000020C()) == 0)) {
                if ((((u32)g_pscode[6] << 0x18) >> 0x1a) == 0x23)
                    *arg0 = g_pscode[6] << 6 | g_pscode[7] >> 2;
                else
                    ret = SCE_ERROR_INVALID_VALUE;
            }
            
            if ((sceKernelSignalSema(g_chkregOld_sema, 1)) != 0)
                ret = SCE_ERROR_SEMAPHORE;
        }
    }
    
    return ret;
}

// Subroutine sceChkreg_driver_7939C851 - Address 0x0000079C
s32 sceChkreg_driver_7939C851(void) {
    s32 ret = 0;
    u8 code[4];
    u16 unk = 0;
    
    // TODO: Fix this, it doesn't seem right
    if ((ret = sceChkreg_driver_59F8491D(code)) == 0) {
        ret = 0;
        
        switch(unk) {
            case 0:
                ret = 0;
                break;
            
            case 1:
            case 2:
                ret = 1;
                break;
                
            case 3:
                ret = 2;
                break;
                
            case 4:
            case 6:
            case 8:
                ret = 3;
                break;
                
            case 5:
            case 7:
            case 9:
                ret = 5;
        }
    }
    
    return ret;
}

// Subroutine sceChkreg_driver_9C6E1D34 - Address 0x0000051C -- Done
s32 sceChkreg_driver_9C6E1D34(u8 *arg0, u8 *arg1) {
    s32 ret = 0;
    s32 error = SCE_ERROR_SEMAPHORE;

    if ((ret = sceKernelWaitSema(g_chkregOld_sema, 1, 0)) == 0) {
        g_unk2 = 0x34;
        u32 i = 0;
        
        for (i = 0; i < 0x14; i++)
            g_chkregOld.buf[0x8 + i] = g_chkregOld.unk2[i]; // 0xA00 + 0x8 = 0xA08
        
        for (i = 0; i < 0x10; i++)
            g_chkregOld.buf[0x1C + i] = (arg0[i] + 0xD4); // 0xA08 + 0x14 = 0xA1C 
        
        for (i = 0; i < 0x10; i++)
            g_chkregOld.buf[0x2C + i] = (arg0[i] + 0x140); // 0xA1C + 0x10 = 0xA2C
            
        error = 0;

        if ((ret = sceUtilsBufferCopyWithRange(g_chkregOld.buf, 0x38, g_chkregOld.buf, 0x38, 0xB)) == 0) {
            for (i = 0; i < 0x10; i++)
                g_chkregOld.buf[i] = arg1[i];
            
            error = 0;
        }
        else {
            if (ret < 0)
                error = SCE_ERROR_BUSY;
            else {
                error = SCE_ERROR_NOT_INITIALIZED;
                if (ret != 0xC)
                    error = SCE_ERROR_NOT_SUPPORTED;
            }
        }

        for (i = 0; i < 0x38; i++)
            g_chkregOld.buf[i] = 0;
        
        if ((ret = sceKernelSignalSema(g_chkregOld_sema, 1)) != 0)
            error = SCE_ERROR_SEMAPHORE;
    }

    return error;
}
