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

typedef struct {
    u8 *pIdStorageUMDConfig; // 0
    u8 *pData; // 4
} SceChkreg; // size = 8

#define CHKREG_ID_STORAGE_UMD_CONFIG_SIZE    (5 * SCE_ID_STORAGE_LEAF_SIZE)

u8 g_idStorageUMDConfig[CHKREG_ID_STORAGE_UMD_CONFIG_SIZE]; // 0x00000A80

SceChkreg g_chkreg = {
    .pIdStorageUMDConfig = &g_idStorageUMDConfig,
    .pData = &g_unk1540,
}; // 0x00000A00

u32 g_UMDRegionCodeInfoPostIndex; // 0x00000A40
u32 g_isUMDRegionCodesObtained; // 0x00000A44
u32 g_isIDPSCertificateObtained; // 0x00000A48

SceIdStorageIDPSCertificate g_IDPSCertificate; // 0x00001480

SceUID g_semaId; // 0x00001538

u32 g_unk1540; // 0x00001540

// Subroutine sub_00000000 - Address 0x00000000
s32 _sceChkregLookupUMDRegionCodeInfo(void)
{
    // 0x00000024 - 0x00000060
    u32 i;
    for (i = 0; i < 5; i++)
    {
        if (sceIdStorageReadLeaf(SCE_ID_STORAGE_LEAF_IDPS_OPEN_PSID_3_UMD_1 + i, &g_chkreg.pIdStorageUMDConfig[i * SCE_ID_STORAGE_LEAF_SIZE]) < SCE_ERROR_OK
            || sceIdStorageReadLeaf(SCE_ID_STORAGE_LEAF_ID_BACKUP_IDPS_OPEN_PSID_3_UMD_1 + i, &g_chkreg.pIdStorageUMDConfig[i * SCE_ID_STORAGE_LEAF_SIZE]) < SCE_ERROR_OK) // 0x0000003C & 0x0000010C
        {
            return SCE_ERROR_NOT_FOUND;
        }
    }

    SceIdStorageUMDRegionCodeInfo *pUMDRegionCodeInfo = (SceIdStorageUMDRegionCodeInfo *)&g_chkreg.pIdStorageUMDConfig[SCE_ID_STORAGE_LEAF_IDPS_OPEN_PSID_3_UMD_1_OFFSET_REGION_CODES];

    // 0x00000064 - 0x00000094
    for (i = 0; i < 256; i++)
    {
        if (pUMDRegionCodeInfo[i].regionCode == 1
            && (pUMDRegionCodeInfo[i].unk4 == 0 || pUMDRegionCodeInfo[i].unk4 == 0x70000000)) // 0x00000084
        {
            // loc_000000B0

            g_UMDRegionCodeInfoPostIndex = i + 1; // 0x000000B4
            g_isUMDRegionCodesObtained = 1; // 0x000000C0

            return SCE_ERROR_OK;
        }
    }

    return SCE_ERROR_OUT_OF_MEMORY;
}

// Subroutine sub_00000128 - Address 0x00000128
s32 _sceChkregCheckRegion(s32 arg0, s32 umdMediaTypeRegionId)
{
    SceIdStorageUMDRegionCodeInfo *pUMDRegionCodeInfo = (SceIdStorageUMDRegionCodeInfo *)&g_chkreg.pIdStorageUMDConfig[SCE_ID_STORAGE_LEAF_IDPS_OPEN_PSID_3_UMD_1_OFFSET_REGION_CODES];

    // 0x00000138 - 0x00000164
    u32 i;
    for (i = 0; i < g_UMDRegionCodeInfoPostIndex; i++)
    {
        if (pUMDRegionCodeInfo[i].regionCode == umdMediaTypeRegionId
            && pUMDRegionCodeInfo[i].unk4 == arg0) // 0x00000160
        {
            return SCE_TRUE;
        }
    }

    return SCE_FALSE;
}

// Subroutine sub_00000190 - Address 0x00000190
s32 _sceChkregLookupIDPSCertificate(void)
{
    /* Obtain an IDPS certificate. */
    if (sceIdStorageLookup(SCE_ID_STORAGE_LEAF_IDPS_OPEN_PSID_1, SCE_ID_STORAGE_LEAF_IDPS_OPEN_PSID_1_OFFSET_IDPS_CERTIFICATE_1, &g_IDPSCertificate, KIRK_CERT_LEN) < SCE_ERROR_OK
        || sceIdStorageLookup(SCE_ID_STORAGE_LEAF_ID_BACKUP_IDPS_OPEN_PSID_1, SCE_ID_STORAGE_LEAF_IDPS_OPEN_PSID_1_OFFSET_IDPS_CERTIFICATE_1, &g_IDPSCertificate, KIRK_CERT_LEN) < SCE_ERROR_OK) // 0x000001B0 & 0x000001F0
    {
        return SCE_ERROR_NOT_FOUND;
    }

    g_isIDPSCertificateObtained = SCE_TRUE; // 0x000001D4

    return SCE_ERROR_OK;
}

// Subroutine sub_0000020C - Address 0x0000020C
s32 _sceChkregVerifyIDPSCertificate(void)
{
    s32 status;

    status = sceUtilsBufferCopyWithRange(NULL, 0, &g_IDPSCertificate, KIRK_CERT_LEN, KIRK_CMD_CERT_VER); // 0x0x00000228

    return (status != SCE_ERROR_OK)
        ? SCE_ERROR_INVALID_FORMAT
        : SCE_ERROR_OK;
}

// Subroutine module_start - Address 0x00000248
s32 _sceChkregInit(SceSize args, void *argp)
{
    (void)args;
    (void)argp;

    // 0x00000258 - 0x0000026C
    u32 i;
    for (i = 0; i < CHKREG_ID_STORAGE_UMD_CONFIG_SIZE; i++)
    {
        ((u8 *)g_chkreg.pIdStorageUMDConfig)[i] = 0;
    }

    // 0x00000270 - 0x0000028C
    for (i = 0; i < sizeof g_IDPSCertificate; i++)
    {
        ((u8 *)&g_IDPSCertificate)[i] = 0;
    }

    g_UMDRegionCodeInfoPostIndex = 0; // 0x0x000002B0
    g_isUMDRegionCodesObtained = SCE_FALSE;
    g_isIDPSCertificateObtained = SCE_FALSE;

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
    for (i = 0; i < CHKREG_ID_STORAGE_UMD_CONFIG_SIZE; i++)
    {
        ((u8 *)g_chkreg.pIdStorageUMDConfig)[i] = 0;
    }

    // 0x00000318 - 0x00000334
    for (i = 0; i < sizeof g_IDPSCertificate; i++)
    {
        ((u8 *)&g_IDPSCertificate)[i] = 0;
    }

    g_UMDRegionCodeInfoPostIndex = 0; // 0x00000348
    g_isUMDRegionCodesObtained = SCE_FALSE;
    g_isIDPSCertificateObtained = SCE_FALSE;

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

// Subroutine sceChkreg_driver_54495B19 - Address 0x00000390
s32 sceChkregCheckRegion(u32 umdMediaType, u32 regionId)
{
    s32 status1;
    s32 status2;

    status1 = SCE_ERROR_SEMAPHORE; // 0x000003BC & 0x000003C8

    /* Allow only one access at a time. */
    status2 = sceKernelWaitSema(g_semaId, 1, NULL); // 0x000003C4
    if (status2 == SCE_ERROR_OK)
    {
        if (g_isUMDRegionCodesObtained || (status1 = _sceChkregLookupUMDRegionCodeInfo()) == SCE_ERROR_OK) // 0x000003D8 & 0x000003E8
        {
            status1 = _sceChkregCheckRegion(0x80000000, umdMediaType | regionId); // 0x000003F4
        }

        /* Release acquired sema resource. */
        status2 = sceKernelSignalSema(g_semaId, 1); // 0x00000404
        if (status2 != SCE_ERROR_OK)
        {
            status1 = SCE_ERROR_SEMAPHORE;
        }
    }

    return status1;
}

// Subroutine sceChkreg_driver_59F8491D - Address 0x00000438
s32 sceChkregGetPsCode(ScePsCode *pPsCode)
{
    s32 status1;
    s32 status2;

    status1 = SCE_ERROR_SEMAPHORE;

    /* Allow only one access at a time. */
    status2 = sceKernelWaitSema(g_semaId, 1, NULL);
    if (status2 == SCE_ERROR_OK)
    {
        if ((g_isIDPSCertificateObtained || (status1 = _sceChkregLookupIDPSCertificate()) == SCE_ERROR_OK)
            && (status1 = _sceChkregVerifyIDPSCertificate()) == SCE_ERROR_OK)
        {
            pPsCode->companyCode = g_IDPSCertificate.idps.companyCode;
            pPsCode->productCode = g_IDPSCertificate.idps.productCode;
            pPsCode->productSubCode = g_IDPSCertificate.idps.productSubCode;
            pPsCode->factoryCode = g_IDPSCertificate.idps.chassisCheck >> 2;
        }

        /* Release acquired sema resource. */
        status2 = sceKernelSignalSema(g_semaId, 1);
        if (status2 != SCE_ERROR_OK)
        {
            status1 = SCE_ERROR_SEMAPHORE;
        }
    }

    return status1;
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

// Subroutine sceChkreg_driver_6894A027 - Address 0x000006B8
s32 sceChkreg_driver_6894A027(u8 *arg0, s32 arg1)
{
    s32 status1;
    s32 status2;

    if (arg1 != 0) // 0x000006E4
    {
        return SCE_ERROR_INVALID_INDEX;
    }

    status1 = SCE_ERROR_SEMAPHORE;

    /* Allow only one access at a time. */
    status2 = sceKernelWaitSema(g_semaId, 1, NULL); // 0x000006FC
    if (status2 == SCE_ERROR_OK) // 0x000006FC
    {
        if ((g_isIDPSCertificateObtained || (status1 = _sceChkregLookupIDPSCertificate()) == SCE_ERROR_OK)
            && (status1 = _sceChkregVerifyIDPSCertificate()) == SCE_ERROR_OK)
        {
            /* 
             * PsCode.factoryCode check:
             * 
             * The test is passed if the PsCode.factoryCode has the following value:
             * 
             *     XXXX XXXX XX10 0011
             * 
             * This means IDPS.chassisCheck needs to be:
             * 
             *     1000 11XX (values 0x8C - 0xF)
             */
            if (((g_IDPSCertificate.idps.chassisCheck >> 2) & 0x3F) == 0x23) // 0x00000748
            {
                // uOFW note: Null check missing for arg0
                *arg0 = (g_IDPSCertificate.idps.chassisCheck << 6) | (g_IDPSCertificate.idps.unk9[0] >> 2);
            }
            else
            {
                status1 = SCE_ERROR_INVALID_VALUE;
            }
        }

        /* Release acquired sema resource. */
        status2 = sceKernelSignalSema(g_semaId, 1); // 0x0000075C
        if (status2 != SCE_ERROR_OK) // 0x0000076C
        {
            status1 = SCE_ERROR_SEMAPHORE;
        }
    }

    return status1;
}

// Subroutine sceChkreg_driver_7939C851 - Address 0x0000079C
s32 sceChkreg_driver_7939C851(void)
{

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
