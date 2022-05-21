/* Copyright (C) 2011 - 2016 The uOFW team
   See the file COPYING for copying permission.
*/

/** 
 * uofw/src/openpsid/ddrdb.c
 *
 */

#include <common_imp.h>
#include <crypto/kirk.h>
#include <dnas_error.h>
#include <memlmd.h>
#include <openpsid_ddrdb.h>
#include <threadman_kernel.h>

/* Check if object <o> is a multiple of the AES block length. */
#define IS_AES_BLOCK_LEN_MULTIPLE(o)    (((o) & (KIRK_AES_BLOCK_LEN - 1)) == 0)

/* Work area used to contain the KIRK input/output data. */
u8 g_workData1[SCE_DNAS_USER_DATA_MAX_LEN + sizeof(KirkAESHeader)] __attribute__((aligned(4))); //0x00003D00

const u8 g_pubKeySigForUser[KIRK_ECDSA_PUBLIC_KEY_LEN] = {
    0x44, 0x2E, 0x79, 0xE6, //0
    0x7B, 0xA2, 0xEB, 0x6C, //4                       
    0x4B, 0x37, 0xDF, 0xCA, //8
    0xD8, 0x4F, 0x50, 0x99, //12
    0xEB, 0xDF, 0x0A, 0xE8, //16
    0x73, 0xDE, 0x66, 0x3E, //20
    0x32, 0x8D, 0xE5, 0xFF, //24
    0x65, 0x1C, 0x22, 0x91, //28
    0x8D, 0x03, 0x8C, 0x01, //32
    0xC9, 0xC3, 0x22, 0x38  //36
}; //0x0000394C

/* Pointer to the work area used to hold the KIRK input/output data. */
u8 *g_pWorkData = g_workData1; // 0x00003A80

SceUID g_semaId; //0x00004540 

/* Subroutine sceDdrdb_driver_B33ACB44 - Address 0x00002CB0 */
s32 sceDdrdbDecrypt(u8 *pSrcData, SceSize size) 
{
    s32 status;
    s32 workSize;
    s32 tmpStatus;
    u32 i;
    
    tmpStatus = sceKernelWaitSema(g_semaId, 1, NULL); //0x00002CF0
    if (tmpStatus != SCE_ERROR_OK)
        return SCE_ERROR_SEMAPHORE;

    if (size <= SCE_DNAS_USER_DATA_MAX_LEN && IS_AES_BLOCK_LEN_MULTIPLE(size)) { //0x00002CFC & 0x00002CD0
         // 0x00002D8C - 0x00002DA4

        /* Set up the work area for KIRK. */
        KirkAESHeader *pAesHdr = (KirkAESHeader *)g_pWorkData;
        pAesHdr->mode = 5;
        pAesHdr->unk4 = 0;
        pAesHdr->unk8 = 0;
        pAesHdr->keyIndex = 0xB;
        pAesHdr->dataSize = size;

        /* Copy data to be decrypted into work area. */
        u8 *pData = g_pWorkData + sizeof(KirkAESHeader);
        for (i = 0; i < size; i++) //0x00002DB4 - 0x00002DD0
            pData[i] = pSrcData[i];

        workSize = size + sizeof(KirkAESHeader); //0x00002DD8
        status = SCE_DNAS_ERROR_OPERATION_FAILED; //0x00002DF0 & 0x00002DF8

        /* Decrypt the specified data. */
        tmpStatus = sceUtilsBufferCopyWithRange(g_pWorkData, workSize, g_pWorkData, workSize, KIRK_CMD_DECRYPT_AES_CBC_IV_NONE); //0x00002DEC
        if (tmpStatus == SCE_ERROR_OK) { //0x00002DF4
            
            /* Copy the computed data back into the provided buffer. */
            for (i = 0; i < size; i++) //0x00002E04 - 0x00002E20
                /*
                 * UOFW: Is this correct? We are copying <size> bytes from the beginning of the
                 * the work data, which includes the KIRK AES header. If the header is still 
                 * inside the workData after decryption, we copy wrong data over and miss out
                 * the last <sizeof(KirkAESHeader)> bits of the data to be decrypted.
                 *
                 * Note: sceDdrdbEncrypt() assumes the header is still there and skips it accordingly.
                 */
                pSrcData[i] = g_pWorkData[i]; //0x00002E20

            status = SCE_ERROR_OK; //0x00002E20
        }
    }
    else {
        status = SCE_DNAS_ERROR_INVALID_ARGUMENTS; //0x00002D10 & 0x00002D18

        /* 
         * UOFW: size here is > SCE_DDRDB_MAX_BUFFER_SIZE, so more data gets cleared 
         * below then neccessary. Depending on size, we even clear data which does not 
         * belong to the work area.
         * Another note: Why do we have to clear any data in this case? The work area 
         * wasn't set up. workSize should be 0.
         */
        workSize = size + sizeof(KirkAESHeader); //0x00002D14
    }

    /* Clear the work area. */
    for (i = 0; i < workSize; i++) // 0x00002D20 - 0x00002D3C
        g_pWorkData[i] = 0;

    tmpStatus = sceKernelSignalSema(g_semaId, 1); //0x00002D44
    return (tmpStatus != SCE_ERROR_OK) ? SCE_ERROR_SEMAPHORE : status; //0x00002D4C & 0x00002D50 & 0x00002D54
}

/* Subroutine sceDdrdb_driver_05D50F41 - Address 0x00002B3C */
s32 sceDdrdbEncrypt(u8 *pSrcData, SceSize size) 
{
    s32 status;
    SceSize workSize;
    s32 tmpStatus;
    u32 i;
    
    tmpStatus = sceKernelWaitSema(g_semaId, 1, NULL); //0x00002B78
    if (tmpStatus != SCE_ERROR_OK)
        return SCE_ERROR_SEMAPHORE;

    if (size <= SCE_DNAS_USER_DATA_MAX_LEN && IS_AES_BLOCK_LEN_MULTIPLE(size)) { // 0x00002B84 & 0x00002B90
        // 0x00002C10 - 0x00002C24

        /* Set up the work area for KIRK. */
        KirkAESHeader *pAesHdr = (KirkAESHeader *)g_pWorkData;
        pAesHdr->mode = 4;
        pAesHdr->unk4 = 0;
        pAesHdr->unk8 = 0;
        pAesHdr->keyIndex = 0xB;
        pAesHdr->dataSize = size;

        /* Copy data to be encrypted into work area. */
        u8 *pData = g_pWorkData + sizeof(KirkAESHeader);
        for (i = 0; i < size; i++) // 0x00002C34 - 0x00002C50
            pData[i] = pSrcData[i]; //0x00002C50

        workSize = size + sizeof(KirkAESHeader); //0x00002C58
        status = SCE_DNAS_ERROR_OPERATION_FAILED;

        /* Encrypt the specified data. */
        tmpStatus = sceUtilsBufferCopyWithRange(g_pWorkData, workSize, g_pWorkData, workSize, KIRK_CMD_ENCRYPT_AES_CBC_IV_NONE); //0x00002C6C
        if (tmpStatus == SCE_ERROR_OK) { //0x00002C74    

            /* Copy encrypted data into provided buffer. */
            for (i = 0; i < size; i++)
                pSrcData[i] = pData[i];

            status = SCE_ERROR_OK; //0x00002CAC
        }
    }
    else {
        status = SCE_DNAS_ERROR_INVALID_ARGUMENTS; // 0x00002BA0

        /*
        * UOFW: size here is > SCE_DDRDB_MAX_BUFFER_SIZE, so more data gets cleared
        * below then neccessary. Depending on size, we even clear data which does not
        * belong to the work area.
        * Another note: Why do we have to clear any data in this case? The work area
        * wasn't set up. workSize should be 0.
        */
        workSize = size + sizeof(KirkAESHeader); //0x00002B9C
    }
    
    /* Clear the work area. */
    for (i = 0; i < workSize; i++) //0x00002BB0 - 0x00002BC4
        g_pWorkData[i] = 0;

    tmpStatus = sceKernelSignalSema(g_semaId, 1);
    return (tmpStatus != SCE_ERROR_OK) ? SCE_ERROR_SEMAPHORE : status; //0x00002BD4 - 0x00002BDC
}

/* Subroutine sceDdrdb_driver_40CB752A - Address 0x000029C0 */
s32 sceDdrdbHash(u8 *pSrcData, SceSize size, u8 *pDigest) 
{
    s32 status;
    s32 tmpStatus;
    SceSize workSize;
    u32 i;
    
    tmpStatus = sceKernelWaitSema(g_semaId, 1, NULL); //0x00002A04
    if (tmpStatus != SCE_ERROR_OK)
        return SCE_ERROR_SEMAPHORE;

    workSize = size + sizeof(KirkSHA1Hdr); //0x00002A20
    if (size <= SCE_DNAS_USER_DATA_MAX_LEN) { //0x00002A1C

        /* Set up the work area for KIRK. */
        KirkSHA1Hdr *pSha1Hdr = (KirkSHA1Hdr *)g_pWorkData;
        pSha1Hdr->dataSize = size; // 0x00002A28 
        
        /* Copy data used for hash computation to work area. */
        u8 *pData = g_pWorkData + sizeof(KirkSHA1Hdr);
        for (i = 0; i < size; i++)
            pData[i] = pSrcData[i]; //0x00002A54

        workSize = size + sizeof(KirkSHA1Hdr); //0x00002A5C
        status = SCE_DNAS_ERROR_OPERATION_FAILED; //0x00002A74 & 0x00002A7C

        /* Compute the SHA-1 hash value of the specified data. */
        tmpStatus = sceUtilsBufferCopyWithRange(g_pWorkData, KIRK_SHA1_DIGEST_LEN, g_pWorkData, workSize, KIRK_CMD_HASH_GEN_SHA1); //0x00002A70
        if (tmpStatus == SCE_ERROR_OK) { //0x00002A9C

            /* Copy hash value into provided buffer. */
            for (i = 0; i < KIRK_SHA1_DIGEST_LEN; i++) //0x00002A80 - 0x00002AA0
                pDigest[i] = g_pWorkData[i];

            status = SCE_ERROR_OK; //0x00002AA4
        }
    }

    /* Clear at least <KIRK_SHA1_DIGEST_LEN> bits of the working area. */
    for (i = 0; i < KIRK_SHA1_DIGEST_LEN; i++) // 0x00002AAC
        g_pWorkData[i] = 0; //0x00002B30

    /* Use casts here to handle potential underflow situation. */
    for (i = 0; (s32)i < (s32)(workSize - KIRK_SHA1_DIGEST_LEN); i++) // 0x00002B1C - 0x00002B30
        g_pWorkData[KIRK_SHA1_DIGEST_LEN + i] = 0;

    tmpStatus = sceKernelSignalSema(g_semaId, 1); //0x00002AD4
    return (tmpStatus != SCE_ERROR_OK) ? SCE_ERROR_SEMAPHORE : tmpStatus; //0x00002ADC & 0x00002AE0 & 0x00002AE4
}

/* Subroutine sceDdrdb_driver_F970D54E - Address 0x00002570 */
s32 sceDdrdbMul1(u8 *pKeyData) 
{
    s32 status;
    s32 tmpStatus;
    SceSize workSize;
    u32 i;
    
    tmpStatus = sceKernelWaitSema(g_semaId, 1, NULL); //0x000025A0
    if (tmpStatus != SCE_ERROR_OK)
        return SCE_ERROR_SEMAPHORE;

    status = SCE_DNAS_ERROR_OPERATION_FAILED; // 0x00002598
    workSize = KIRK_ECDSA_PUBLIC_KEY_LEN + KIRK_ECDSA_PRIVATE_KEY_LEN;

    /* Compute a (public, private) key pair. */
    tmpStatus = sceUtilsBufferCopyWithRange(g_pWorkData, workSize, NULL, 0, KIRK_CMD_KEY_GEN_ECDSA); //0x000025E8
    if (tmpStatus == SCE_ERROR_OK) { // 0x000025F0

        /* Copy the computed key pair into the provided buffer. */
        for (i = 0; i < workSize; i++) // 0x000025F8 & 0x00002618
            pKeyData[i] = g_pWorkData[i]; //0x00002618

        status = SCE_ERROR_OK;
    }

    /* Clear the work area. */
    for (i = 0; i < workSize; i++) // 0x00002620 - 0x00002638
        g_pWorkData[i] = 0;

    tmpStatus = sceKernelSignalSema(g_semaId, 1); //0x00002640
    return (tmpStatus != SCE_ERROR_OK) ? SCE_ERROR_SEMAPHORE : status; //0x00002648 & 0x0000264C & 0x00002654
}

/* Subroutine sceDdrdb_driver_EC05300A - Address 0x00002658 */
s32 sceDdrdbMul2(u8 *pPrivKey, u8 *pBasePoint, u8 *pNewPoint) 
{
    s32 status;
    s32 tmpStatus;
    SceSize workSize;
    u32 i;
    
    tmpStatus = sceKernelWaitSema(g_semaId, 1, NULL); // 0x00002698
    if (tmpStatus != SCE_ERROR_OK)
        return SCE_ERROR_SEMAPHORE;

    /* Set up the work area. */
    for (i = 0; i < KIRK_ECDSA_PRIVATE_KEY_LEN; i++) // 0x000026A4 - 0x000026C8
        g_pWorkData[i] = pPrivKey[i]; //0x000026C8

    for (i = 0; i < KIRK_ECDSA_POINT_LEN; i++) // 0x000026CC - 0x000026EC
        g_pWorkData[KIRK_ECDSA_PRIVATE_KEY_LEN + i] = pBasePoint[i]; //0x000026EC

    status = SCE_DNAS_ERROR_OPERATION_FAILED; //0x00002708 & 0x00002710

    /* Compute a new point on the elliptic curve using the provided key and curve base point. */
    tmpStatus = sceUtilsBufferCopyWithRange(g_pWorkData, KIRK_ECDSA_POINT_LEN,
        g_pWorkData, KIRK_ECDSA_PRIVATE_KEY_LEN, 
        KIRK_CMD_POINT_MULTIPLICATION_ECDSA
        ); //0x00002704
    if (tmpStatus == SCE_ERROR_OK) { //0x0000270C

        /* Copy the computed point into the provided buffer. */
        for (i = 0; i < KIRK_ECDSA_POINT_LEN; i++) //0x00002714 - 0x00002734
            pNewPoint[i] = g_pWorkData[i];

        status = SCE_ERROR_OK;
    }
    workSize = KIRK_ECDSA_PRIVATE_KEY_LEN + KIRK_ECDSA_POINT_LEN;
    
    /* Clear the work area. */
    for (i = 0; i < workSize; i++) //0x00002740 - 0x00002754
        g_pWorkData[i] = 0;

    tmpStatus = sceKernelSignalSema(g_semaId, 1); //0x0000275C
    return (tmpStatus != SCE_ERROR_OK) ? SCE_ERROR_SEMAPHORE : status; //0x00002764 & 0x00002764 & 0x0000276C
}

/* Subroutine sceDdrdb_driver_E27CE4CB - Address 0x00002358 */
s32 sceDdrdbSigvry(u8 *pPubKey, u8 *pData, u8 *pSig) 
{
    s32 status;
    s32 tmpStatus;
    SceSize workSize;
    u32 i;
    
    tmpStatus = sceKernelWaitSema(g_semaId, 1, NULL); // 0x00002398
    if (tmpStatus != SCE_ERROR_OK)
        return SCE_ERROR_SEMAPHORE;

    /* Set up the work area. */
    for (i = 0; i < KIRK_ECDSA_PUBLIC_KEY_LEN; i++) // 0x000023AC - 0x000023C8
        g_pWorkData[i] = pPubKey[i]; // 0x000023C8

    for (i = 0; i < KIRK_ECDSA_SRC_DATA_LEN; i++) // 0x000023CC - 0x000023EC
        g_pWorkData[KIRK_ECDSA_PUBLIC_KEY_LEN + i] = pData[i]; // 0x000023EC

    for (i = 0; i < KIRK_ECDSA_SIG_LEN; i++) // 0x000023CC - 0x000023EC
        g_pWorkData[KIRK_ECDSA_PUBLIC_KEY_LEN + KIRK_ECDSA_SRC_DATA_LEN + i] = pSig[i]; // 0x00002410

    workSize = KIRK_ECDSA_PUBLIC_KEY_LEN + KIRK_ECDSA_SRC_DATA_LEN + KIRK_ECDSA_SIG_LEN;

    /* Verify the provided signature. */
    tmpStatus = sceUtilsBufferCopyWithRange(NULL, 0, g_pWorkData, workSize, KIRK_CMD_SIG_VER_ECDSA); //0x00002428
    status = (tmpStatus != SCE_ERROR_OK) ? SCE_DNAS_ERROR_OPERATION_FAILED : SCE_ERROR_OK; //0x0000242C - 0x00002434

    /* Clear the work area. */
    for (i = 0; i < workSize; i++) //0x00002438 - 0x00002450
        g_pWorkData[i] = 0;

    tmpStatus = sceKernelSignalSema(g_semaId, 1); //0x00002458
    return (tmpStatus != SCE_ERROR_OK) ? SCE_ERROR_SEMAPHORE : status; //0x00002460 - 0x00002468
}

/* Subroutine sceDdrdb_driver_370F456A - Address 0x00002494 */
s32 sceDdrdbCertvry(u8 *pCert) 
{
    s32 status;
    s32 tmpStatus;
    u32 i;
    
    status = sceKernelWaitSema(g_semaId, 1, NULL); //0x000024C4
    if (status != SCE_ERROR_OK)
        return SCE_ERROR_SEMAPHORE; 

    /* Set up the work area. */
    for (i = 0; i < KIRK_CERT_LEN; i++) //0x000024D4 & 0x000024F4
        g_pWorkData[i] = pCert[i]; //0x000024F4

    /* Verify the provided certificate. */
    tmpStatus = sceUtilsBufferCopyWithRange(NULL, 0, g_pWorkData, KIRK_CERT_LEN, KIRK_CMD_CERT_VER); //0x0000250C
    status = (tmpStatus != SCE_ERROR_OK) ? SCE_DNAS_ERROR_OPERATION_FAILED : SCE_ERROR_OK; //0x00002510 - 0x00002518

    /* Clear the work area. */
    for (i = 0; i < KIRK_CERT_LEN; i++) //0x0000251C - 0x00002534
        g_pWorkData[i] = 0;

    tmpStatus = sceKernelSignalSema(g_semaId, 1); //0x0000253C
    return (tmpStatus != SCE_ERROR_OK) ? SCE_ERROR_SEMAPHORE : status;
}

/* sceDdrdb_driver_B24E1391 - Address 0x00002798 */
s32 sceDdrdbSiggen(u8 *pPrivKey, u8 *pSrcData, u8 *pSig) 
{
    s32 status;
    s32 tmpStatus; 
    SceSize workSize;
    u32 i;
    
    tmpStatus = sceKernelWaitSema(g_semaId, 1, NULL); //0x000027D8
    if (tmpStatus != SCE_ERROR_OK)
        return SCE_ERROR_SEMAPHORE;
    
    /* Setup the work area. */
    for (i = 0; i < 32; i++) //0x000027E4 - 0x00002808
        g_pWorkData[i] = pPrivKey[i];
    
    for (i = 0; i < KIRK_ECDSA_SRC_DATA_LEN; i++) //0x0000280C - 0x0000282C
        g_pWorkData[i] = pSrcData[i];

    workSize = 32 + KIRK_ECDSA_SRC_DATA_LEN;
    status = SCE_DNAS_ERROR_OPERATION_FAILED; //0x00002848 & 0x00002850

    /* Compute the signature for the provided data. */
    tmpStatus = sceUtilsBufferCopyWithRange(g_pWorkData, workSize, g_pWorkData, workSize, KIRK_CMD_SIG_GEN_ECDSA); //0x00002844
    if (tmpStatus == SCE_ERROR_OK) { //0x0000284C

        /* Copy the computed signature into the provided buffer. */
        for (i = 0; i < KIRK_ECDSA_SIG_LEN; i++) //0x00002854 - 0x00002874
            pSig[i] = g_pWorkData[i]; //0x00002874

        status = SCE_ERROR_OK;
    }
    
    /* Clear the work area. */
    for (i = 0; i < workSize; i++) //0x00002880 - 0x00002894
        g_pWorkData[i] = 0;

    tmpStatus = sceKernelSignalSema(g_semaId, 1); //0x0000289C
    return (tmpStatus != SCE_ERROR_OK) ? SCE_ERROR_SEMAPHORE : status; //0x000028A4 - 0x000028AC
}

/* Subroutine sceDdrdb_driver_B8218473 - Address 0x000028D8 */
s32 sceDdrdbPrngen(u8 *pDstData) 
{
    s32 status;
    s32 tmpStatus;
    u32 i;

    status = SCE_DNAS_ERROR_OPERATION_FAILED; //0x000028FC
    
    status = sceKernelWaitSema(g_semaId, 1, NULL); //0x00002908
    if (status != SCE_ERROR_OK)
        return SCE_ERROR_SEMAPHORE;

    /* Compute a pseudorandom number. */
    tmpStatus = sceUtilsBufferCopyWithRange(g_pWorkData, KIRK_PRN_LEN, NULL, 0, KIRK_CMD_PRN_GEN); //0x00002950
    if (tmpStatus == SCE_ERROR_OK) { //0x00002958

        /* Copy computed number into provided buffer. */
        for (i = 0; i < KIRK_PRN_LEN; i++) //0x00002960 - 0x00002980
            pDstData[i] = g_pWorkData[i]; //0x00002980

        status = SCE_ERROR_OK;
    }
    
    /* Clear the work area. */
    for (i = 0; i < KIRK_PRN_LEN; i++) // 0x00002988 - 0x000029A0
        g_pWorkData[i] = 0; 

    tmpStatus = sceKernelSignalSema(g_semaId, 1); //0x000029A8
    return (tmpStatus != SCE_ERROR_OK) ? SCE_ERROR_SEMAPHORE : status; //0x000029B0 - 0x000029BC
}

/* Subroutine sceDdrdb_F013F8BF - Address 0x00002E2C */
// Name: sceDdrdbSigvryForUser ?
s32 sceDdrdb_F013F8BF(u8 *pData, u8 *pSig) 
{
    s32 status;
    s32 tmpStatus;
    SceSize workSize;
    s32 oldK1;
    u32 i;
    
    oldK1 = pspShiftK1();

    /* Check if the provided buffers are located in userland. */
    if (!pspK1StaBufOk(pData, KIRK_SHA1_DIGEST_LEN) || !pspK1StaBufOk(pSig, KIRK_ECDSA_SIG_LEN)) { // 0x00002E68 & 0x00002E7C
        pspSetK1(oldK1);
        return SCE_ERROR_PRIV_REQUIRED;
    }
         
    tmpStatus = sceKernelWaitSema(g_semaId, 1, NULL); //0x00002EC4
    if (tmpStatus != SCE_ERROR_OK) {
        pspSetK1(oldK1);
        return SCE_ERROR_SEMAPHORE;
    }

    /* Set up the work area. */
    for (i = 0; i < KIRK_ECDSA_PUBLIC_KEY_LEN; i++) // 0x00002ED4 - 0x00002F00
        g_pWorkData[i] = g_pubKeySigForUser[i]; // 0x00002F00

    for (i = 0; i < KIRK_SHA1_DIGEST_LEN; i++) //0x00002F04 - 0x00002F24
        g_pWorkData[KIRK_ECDSA_PUBLIC_KEY_LEN + i] = pData[i];

    for (i = 0; i < KIRK_ECDSA_SIG_LEN; i++) //0x00002F28 - 0x00002F48
        g_pWorkData[KIRK_ECDSA_PUBLIC_KEY_LEN + KIRK_SHA1_DIGEST_LEN + i] = pSig[i]; //0x00002F48

    workSize = KIRK_ECDSA_PUBLIC_KEY_LEN + KIRK_SHA1_DIGEST_LEN + KIRK_ECDSA_SIG_LEN;

    /* Verify the provided signature. */
    tmpStatus = sceUtilsBufferCopyWithRange(NULL, 0, g_pWorkData, workSize, KIRK_CMD_SIG_VER_ECDSA); //0x00002F60
    status = (tmpStatus != SCE_ERROR_OK) ? SCE_ERROR_INVALID_VALUE : SCE_ERROR_OK; //0x00002F64 - 0x00002F6C
    
    /* Clear the work area. */
    for (i = 0; i < workSize; i++) //0x00002F70 - 0x00002F88
        g_pWorkData[i] = 0; //0x00002F88

    tmpStatus = sceKernelSignalSema(g_semaId, 1); //0x00002F90

    pspSetK1(oldK1);
    return (tmpStatus != SCE_ERROR_OK) ? SCE_ERROR_SEMAPHORE : status; //0x00002F98 - 0x00002FA4
}