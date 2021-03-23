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

#define CHKREG_UNK9BC_SIZE    (0x14)
const u8 g_unk9BC[CHKREG_UNK9BC_SIZE] =
{
    0x5F, 0x7C, 0x9B, 0x91,
    0x60, 0xFF, 0xB3, 0xCE,
    0x41, 0x9E, 0xBD, 0x2A,
    0x4E, 0x4B, 0x1B, 0x15,
    0x2C, 0x2A, 0xDF, 0xA0,
}; // 0x000009BC

u32 g_UMDRegionCodeInfoPostIndex; // 0x00000A40
u32 g_isUMDRegionCodesObtained; // 0x00000A44
u32 g_isConsoleIdCertificateObtained; // 0x00000A48

#define CHKREG_ID_STORAGE_UMD_CONFIG_SIZE    (5 * SCE_ID_STORAGE_LEAF_SIZE)
u8 g_idStorageUMDConfig[CHKREG_ID_STORAGE_UMD_CONFIG_SIZE]; // 0x00000A80

SceIdStorageConsoleIdCertificate g_ConsoleIdCertificate; // 0x00001480

SceUID g_semaId; // 0x00001538

u8 g_unk1540[0x38]; // 0x00001540

u8 *g_pIdStorageUMDConfig = (u8 *)&g_idStorageUMDConfig; // 0x00000A00
u8 *g_pWorkBuffer = (u8 *)&g_unk1540; // 0x00000A04

// Subroutine sub_00000000 - Address 0x00000000
s32 _sceChkregLookupUMDRegionCodeInfo(void)
{
    // 0x00000024 - 0x00000060
    u32 i;
    for (i = 0; i < 5; i++)
    {
        if (sceIdStorageReadLeaf(SCE_ID_STORAGE_LEAF_CONSOLE_ID_OPEN_PSID_3_UMD_1 + i, &g_pIdStorageUMDConfig[i * SCE_ID_STORAGE_LEAF_SIZE]) < SCE_ERROR_OK
            || sceIdStorageReadLeaf(SCE_ID_STORAGE_LEAF_ID_BACKUP_CONSOLE_ID_OPEN_PSID_3_UMD_1 + i, &g_pIdStorageUMDConfig[i * SCE_ID_STORAGE_LEAF_SIZE]) < SCE_ERROR_OK) // 0x0000003C & 0x0000010C
        {
            return SCE_ERROR_NOT_FOUND;
        }
    }

    SceIdStorageUMDRegionCodeInfo *pUMDRegionCodeInfo = (SceIdStorageUMDRegionCodeInfo *)&g_pIdStorageUMDConfig[SCE_ID_STORAGE_LEAF_CONSOLE_ID_OPEN_PSID_3_UMD_1_OFFSET_REGION_CODES];

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
s32 _sceChkregCheckRegion(u32 arg0, u32 umdMediaTypeRegionId)
{
    SceIdStorageUMDRegionCodeInfo *pUMDRegionCodeInfo = (SceIdStorageUMDRegionCodeInfo *)&g_pIdStorageUMDConfig[SCE_ID_STORAGE_LEAF_CONSOLE_ID_OPEN_PSID_3_UMD_1_OFFSET_REGION_CODES];

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
s32 _sceChkregLookupConsoleIdCertificate(void)
{
    /* Obtain a ConsoleId certificate. */
    if (sceIdStorageLookup(SCE_ID_STORAGE_LEAF_CONSOLE_ID_OPEN_PSID_1, SCE_ID_STORAGE_LEAF_CONSOLE_ID_OPEN_PSID_1_OFFSET_IDPS_CERTIFICATE_1, &g_ConsoleIdCertificate, KIRK_CERT_LEN) < SCE_ERROR_OK
        || sceIdStorageLookup(SCE_ID_STORAGE_LEAF_ID_BACKUP_CONSOLE_ID_OPEN_PSID_1, SCE_ID_STORAGE_LEAF_CONSOLE_ID_OPEN_PSID_1_OFFSET_IDPS_CERTIFICATE_1, &g_ConsoleIdCertificate, KIRK_CERT_LEN) < SCE_ERROR_OK) // 0x000001B0 & 0x000001F0
    {
        return SCE_ERROR_NOT_FOUND;
    }

    g_isConsoleIdCertificateObtained = SCE_TRUE; // 0x000001D4

    return SCE_ERROR_OK;
}

// Subroutine sub_0000020C - Address 0x0000020C
s32 _sceChkregVerifyConsoleIdCertificate(void)
{
    s32 status;

    status = sceUtilsBufferCopyWithRange(NULL, 0, (u8 *)&g_ConsoleIdCertificate, KIRK_CERT_LEN, KIRK_CMD_CERT_VER); // 0x0x00000228

    return (status != SCE_ERROR_OK)
        ? SCE_ERROR_INVALID_FORMAT
        : SCE_ERROR_OK;
}

// Subroutine module_start - Address 0x00000248
s32 _sceChkregInit(SceSize args, const void *argp)
{
    (void)args;
    (void)argp;

    // 0x00000258 - 0x0000026C
    u32 i;
    for (i = 0; i < CHKREG_ID_STORAGE_UMD_CONFIG_SIZE; i++)
    {
        g_pIdStorageUMDConfig[i] = 0;
    }

    // 0x00000270 - 0x0000028C
    for (i = 0; i < sizeof g_ConsoleIdCertificate; i++)
    {
        ((u8 *)&g_ConsoleIdCertificate)[i] = 0;
    }

    g_UMDRegionCodeInfoPostIndex = 0; // 0x0x000002B0
    g_isUMDRegionCodesObtained = SCE_FALSE;
    g_isConsoleIdCertificateObtained = SCE_FALSE;

    /* Create a module semaphore which acts as a mutex. */
    g_semaId = sceKernelCreateSema("SceChkreg", SCE_KERNEL_SA_THFIFO, 1, 1, NULL); // 0x000002BC

    /* If the semaphore could not be created successfully, unload the module. Otherwise keep it in memory. */
    return (g_semaId < 1)
        ? SCE_KERNEL_NO_RESIDENT
        : SCE_KERNEL_RESIDENT;
}

// Subroutine module_stop - Address 0x000002E0
s32 _sceChkregEnd(SceSize args, const void *argp)
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
        g_pIdStorageUMDConfig[i] = 0;
    }

    // 0x00000318 - 0x00000334
    for (i = 0; i < sizeof g_ConsoleIdCertificate; i++)
    {
        ((u8 *)&g_ConsoleIdCertificate)[i] = 0;
    }

    g_UMDRegionCodeInfoPostIndex = 0; // 0x00000348
    g_isUMDRegionCodesObtained = SCE_FALSE;
    g_isConsoleIdCertificateObtained = SCE_FALSE;

    /* Only stop the module when it is not currently used by other modules. */

    status = sceKernelWaitSema(g_semaId, 1, &timeout); // 0x0x0000035C
    if (status == SCE_ERROR_OK) // 0x0x00000364
    {
        /* Module no longer in use. Stop it. */
        sceKernelDeleteSema(g_semaId); // 0x00000380

        return SCE_KERNEL_STOP_SUCCESS;
    }

    /* Module still in use, don't stop it yet. */

    return SCE_KERNEL_STOP_FAIL;
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
        if ((g_isConsoleIdCertificateObtained || (status1 = _sceChkregLookupConsoleIdCertificate()) == SCE_ERROR_OK)
            && (status1 = _sceChkregVerifyConsoleIdCertificate()) == SCE_ERROR_OK)
        {
            pPsCode->companyCode = g_ConsoleIdCertificate.consoleId.companyCode;
            pPsCode->productCode = g_ConsoleIdCertificate.consoleId.productCode;
            pPsCode->productSubCode = g_ConsoleIdCertificate.consoleId.productSubCode;
            pPsCode->factoryCode = g_ConsoleIdCertificate.consoleId.factoryCode;
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

// Subroutine sceChkreg_driver_9C6E1D34 - Address 0x0000051C
s32 sceChkreg_driver_9C6E1D34(const u8 *arg0, u8 *arg1)
{
    s32 status1;
    s32 status2;

    status1 = SCE_ERROR_SEMAPHORE;

    /* Allow only one access at a time. */
    status2 = sceKernelWaitSema(g_semaId, 1, NULL); // 0x00000554
    if (status2 == SCE_ERROR_OK) // 0x0000055C
    {
        u8 *pWorkBuffer = g_pWorkBuffer;

        ((u32 *)pWorkBuffer)[0] = 0x34; // 0x00000570 -- TODO: Length of specific data to hash?

        /* "Prefix" specified input data with 0x14 bytes of Chkreg specific data in the work buffer. */

        // 0x00000574 - 0x0000059C
        u32 i;
        for (i = 0; i < CHKREG_UNK9BC_SIZE; i++)
        {
            pWorkBuffer[0x4 + i] = g_unk9BC[i];
        }

        /* Copy 0x20 bytes of input data to work buffer. */

        // 0x000005A0 - 0x000005C0
        for (i = 0; i < 0x10; i++)
        {
            pWorkBuffer[0x4 + CHKREG_UNK9BC_SIZE + i] = arg0[0xD4 + i];
        }

        // 0x000005C4 - 0x000005E4
        for (i = 0; i < 0x10; i++)
        {
            pWorkBuffer[0x4 + CHKREG_UNK9BC_SIZE + 0x10 + i] = arg0[0x140 + i];
        }

        /* Compute SHA1 hash. */
        status1 = sceUtilsBufferCopyWithRange(pWorkBuffer, 0x38, pWorkBuffer, 0x38, KIRK_CMD_HASH_GEN_SHA1); // 0x000005F8
        if (status1 == SCE_ERROR_OK) // 0x00000604
        {
            /* Copy first 16 byte of computed hash to target buffer. */
            for (i = 0; i < 0x10; i++)
            {
                arg1[i] = pWorkBuffer[i];
            }

            status1 = SCE_ERROR_OK;
        }
        else if (status1 < SCE_ERROR_OK) // 0x0000060C
        {
            status1 = SCE_ERROR_BUSY;
        }
        else
        {
            // 0x00000614 - 0x00000628
            status1 = (status1 != KIRK_NOT_INITIALIZED)
                ? SCE_ERROR_NOT_SUPPORTED
                : SCE_ERROR_NOT_INITIALIZED;
        }

        /* Clear work buffer. */

        // 0x0000062C - 0x00000644
        for (i = 0; i < 0x38; i++)
        {
            pWorkBuffer[i] = 0;
        }

        /* Release acquired sema resource. */
        status2 = sceKernelSignalSema(g_semaId, 1); // 0x0000064C
        if (status2 != SCE_ERROR_OK)
        {
            status1 = SCE_ERROR_SEMAPHORE;
        }
    }

    return status1;
}

// Subroutine sceChkreg_driver_6894A027 - Address 0x000006B8
s32 sceChkregGetPsFlags(u8 *pPsFlags, s32 index)
{
    s32 status1;
    s32 status2;

    if (index != SCE_CHKREG_PS_FLAGS_INDEX_DEFAULT) // 0x000006E4
    {
        return SCE_ERROR_INVALID_INDEX;
    }

    status1 = SCE_ERROR_SEMAPHORE;

    /* Allow only one access at a time. */
    status2 = sceKernelWaitSema(g_semaId, 1, NULL); // 0x000006FC
    if (status2 == SCE_ERROR_OK) // 0x000006FC
    {
        if ((g_isConsoleIdCertificateObtained || (status1 = _sceChkregLookupConsoleIdCertificate()) == SCE_ERROR_OK)
            && (status1 = _sceChkregVerifyConsoleIdCertificate()) == SCE_ERROR_OK)
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
            if (g_ConsoleIdCertificate.consoleId.factoryCode == 0x23) // 0x00000748
            {
                // uOFW note: Null check missing for arg0
                *pPsFlags = (g_ConsoleIdCertificate.consoleId.psFlagsMajor << 6) | (g_ConsoleIdCertificate.consoleId.psFlagsMinor);
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
s32 sceChkregGetPspModel(void)
{
    s32 status;
    ScePsCode psCode;

    status = sceChkregGetPsCode(&psCode); // 0x000007A4
    if (status != SCE_ERROR_OK) // 0x000007AC
    {
        return status;
    }

    switch (psCode.productSubCode)
    {
        case SCE_PSP_PRODUCT_SUB_CODE_TA_079_TA_081:
        case SCE_PSP_PRODUCT_SUB_CODE_TA_082_TA_086:
            return SCE_CHKREG_PSP_MODEL_1000_SERIES;
        case SCE_PSP_PRODUCT_SUB_CODE_TA_085_TA_088:
            return SCE_CHKREG_PSP_MODEL_2000_SERIES;
        case SCE_PSP_PRODUCT_SUB_CODE_TA_090_TA_092:
        case SCE_PSP_PRODUCT_SUB_CODE_TA_093:
        case SCE_PSP_PRODUCT_SUB_CODE_TA_095:
            return SCE_CHKREG_PSP_MODEL_3000_SERIES;
        case SCE_PSP_PRODUCT_SUB_CODE_TA_091:
        case SCE_PSP_PRODUCT_SUB_CODE_TA_094:
        case SCE_PSP_PRODUCT_SUB_CODE_TA_096_TA_097:
            // uOFW note: Before the release of the PSP E-1000, '4' was returned for the PSP N-1000 series.
            // In 6.60, however, both the PSP N-1000 and PSP E-1000 series are grouped together as value '5'.
            // Why is that?
            return SCE_CHKREG_PSP_MODEL_N1000_E1000_SERIES;
        default:
            return SCE_CHKREG_PSP_MODEL_UNKNOWN_SERIES;
    }
}
