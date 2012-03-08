/* Copyright (C) 2011, 2012 The uOFW team
   See the file COPYING for copying permission.
*/

/** 
 * ddrdb.h
 *
 * @author _Felix_
 * @version 6.60
 * 
 * Reverse engineered ddrdb library of the SCE PSP system.
 */

#include <stdlib.h>
#include <pspinit.h>
#include "ddrdb.h"

int sceUtilsBufferCopyWithRange(void *outbuff, int outsize, void *inbuff, int insize, int cmd);

u8 ddrdbBuf[SCE_DDRDB_MAX_BUFFER_SIZE+SCE_DDRDB_DECRYPTED_BUFFER_HEADER_SIZE] __attribute__((aligned(4))); //0x00003D00
const u8 ddrdbBuf2[40] = { 0x44, 0x2E, 0x79, 0xE6, //0
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

SceUID semaId; //0x00004540 

/* Subroutine sceDdrdb_driver_B33ACB44 - Address 0x00002CB0 */
int sceDdrdbDecrypt(u8 buf[], int size) {
    int status;
    int sizeVal;
    int retVal;
    int i;
    
    status = sceKernelWaitSema(semaId, 1, 0); //0x00002CF0
    if (status != 0) { //0x00002CF8
        return SCE_ERROR_SEMAPHORE; //0x00002CDC & 0x00002CE0
    }
    if (size <= SCE_DDRDB_MAX_BUFFER_SIZE) { //0x00002CFC & 0x00002CD0
        if ((size & 0xF) == 0) { //0x00002D04 & 0x00002D08
            //Fill in the header of the buffer to be decrypted.           
            _sw(5, ddrdbBuf); //0x00002D8C
            _sw(0, &ddrdbBuf[4]); //0x00002DA8
            _sw(0, &ddrdbBuf[8]); //0x00002DB0
            _sw(0xB, &ddrdbBuf[12]); //0x00002DA0
            _sw(size, &ddrdbBuf[16]); //0x00002DA4
            
            if (size != 0) { //0x00002D90 & 0x00002DAC
                //0x00002DB4 - x00002DD0
                for (i = 0; i < size; i++) {
                     ddrdbBuf[SCE_DDRDB_DECRYPTED_BUFFER_HEADER_SIZE+i] = buf[i]; //0x00002C50
                }
            }
            sizeVal = size + SCE_DDRDB_DECRYPTED_BUFFER_HEADER_SIZE; //0x00002DD8
            retVal = sceUtilsBufferCopyWithRange(ddrdbBuf, sizeVal, ddrdbBuf, sizeVal, 7); //0x00002DEC
            status = 0x80530300; //0x00002DF0 & 0x00002DF8
            if (retVal == 0) { //0x00002DF4
                if (size > 0) { //0x00002DFC
                    //0x00002E04 - 0x00002E20
                    for (i = 0; i < size; i++) {
                         buf[i] = ddrdbBuf[SCE_DDRDB_DECRYPTED_BUFFER_HEADER_SIZE+i]; //0x00002E20
                    }
                }
                status = 0; //0x00002E20
            }
        }
    }
    else {
        status = 0x80530301; //0x00002D10 & 0x00002D18
        sizeVal = size + 20; //0x00002D14
    }
    if (sizeVal != 0) { //0x00002D1C
        //0x00002D20 - 0x00002D3C
        for (i = 0; i < sizeVal; i++) {
             ddrdbBuf[i] = 0;
        }
    }
    status = sceKernelSignalSema(semaId, 1); //0x00002D44
    return (status != 0) ? SCE_ERROR_SEMAPHORE : 0; //0x00002D4C & 0x00002D50 & 0x00002D54
}



/* Subroutine sceDdrdb_driver_05D50F41 - Address 0x00002B3C */
int sceDdrdbEncrypt(u8 buf[], int size) {
    int status;
    int sizeVal;
    int retVal;
    int i;
    
    status = sceKernelWaitSema(semaId, 1, 0); //0x00002B78
    if (status != 0) {
        return SCE_ERROR_SEMAPHORE; //0x00002B68 & 0x00002B6C
    }
    if (size <= SCE_DDRDB_MAX_BUFFER_SIZE) { //0x00002B84 & 0x00002B88
        if ((size & 0xF) == 0) { //0x00002B8C & 0x00002B90
            //Fill in the header of the buffer to be encrypted.
            _sw(4, ddrdbBuf); //0x00002C10
            _sw(0, &ddrdbBuf[4]); //0x00002C28
            _sw(0, &ddrdbBuf[8]); //0x00002C30
            _sw(0xB, &ddrdbBuf[12]); //0x00002C20
            _sw(size, &ddrdbBuf[16]); //0x00002C24
            
            if (size != 0) { //0x00002C2C
                //0x00002C34 - 0x00002C50
                for (i = 0; i < size; i++) {
                     ddrdbBuf[SCE_DDRDB_ENCRYPTED_BUFFER_HEADER_SIZE +i] = buf[i]; //0x00002C50
                }
            }
            sizeVal = size + SCE_DDRDB_ENCRYPTED_BUFFER_HEADER_SIZE ; //0x00002C58
            retVal = sceUtilsBufferCopyWithRange(ddrdbBuf, sizeVal, ddrdbBuf, sizeVal, 4); //0x00002C6C
            status = 0x80530300;
            if (retVal == 0) { //0x00002C74
                if (retVal < size) { //0x00002C80
                    //0x00002C84 - 0x00002CA4
                    for (i = 0; i < size; i++) {
                         buf[i] = ddrdbBuf[SCE_DDRDB_ENCRYPTED_BUFFER_HEADER_SIZE +i];
                    }
                }
                status = 0; //0x00002CAC
            }
        }
    }
    else {
        status = 0x80530301; //0x00002B98 & 0x00002BA0
        sizeVal = size + 20; //0x00002B9Cf
    }
    if (sizeVal != 0) { //0x00002BA4
        //0x00002BB0 - 0x00002BC4
        for (i = 0; i < sizeVal; i++) {
              ddrdbBuf[i] = 0;
        }
    }
    status = sceKernelSignalSema(semaId, 1);
    return (status != 0) ? SCE_ERROR_SEMAPHORE : 0; //0x00002BD4 - 0x00002BDC
}

/* Subroutine sceDdrdb_driver_40CB752A - Address 0x000029C0 */
int sceDdrdbHash(u8 srcBuf[], int size, u8 hash[SCE_DDRDB_HASH_BUFFER_SIZE]) {
    int status;
    int sizeVal;
    int retVal;
    int i;
    
    status = sceKernelWaitSema(semaId, 1); //0x00002A04
    if (status != 0) { //0x00002A0C
        return SCE_ERROR_SEMAPHORE; //0x000029F4 & 0x000029F8
    }
    sizeVal = size + 4; //0x00002A20
    if (size <= SCE_DDRDB_MAX_BUFFER_SIZE) { //0x00002A1C
        //Fill in the header of the source buffer.
        _sw(size, ddrdbBuf); //0x00002A28 
        
        if (size != 0) { //0x00002A30
            //0x00002A2C - 0x00002A54
            for (i = 0; i < size; i++) {
                 ddrdbBuf[SCE_DDRDB_HASH_BUFFER_HEADER_SIZE+i] = srcBuf[i]; //0x00002A54
            }
        }
        sizeVal = size + SCE_DDRDB_HASH_BUFFER_HEADER_SIZE; //0x00002A5C
        retVal = sceUtilsBufferCopyWithRange(ddrdbBuf, SCE_DDRDB_HASH_BUFFER_SIZE, ddrdbBuf, sizeVal, 11); //0x00002A70
        status = 0x80530300; //0x00002A74 & 0x00002A7C
        if (retVal == 0) { //0x00002A9C
            //0x00002A80 - 0x00002AA0
            for (i = 0; i < SCE_DDRDB_HASH_BUFFER_SIZE; i++) {
                hash[i] = ddrdbBuf[i]; //0x00002A9C
            }
            status = 0; //0x00002AA4
        }
    }
    if (sizeVal < SCE_DDRDB_HASH_BUFFER_SIZE) { //0x00002AAC
        //0x00002AB0 - 0x00002ACC
        for (i = 0; i < SCE_DDRDB_HASH_BUFFER_SIZE; i++) {
             ddrdbBuf[i] = 0; //0x00002ACC
        }
    }
    else {
        if (sizeVal != 0) { //0x00002B14
            //0x00002B1C - 0x00002B30
            for (i = 0; i < sizeVal; i++) {
                ddrdbBuf[i] = 0; //0x00002B30
            }
        }
    }
    status = sceKernelSignalSema(semaId, 1); //0x00002AD4
    return (status != 0) ? SCE_ERROR_SEMAPHORE : 0; //0x00002ADC & 0x00002AE0 & 0x00002AE4
}

/* Subroutine sceDdrdb_driver_F970D54E - Address 0x00002570 */
int sceDdrdbMul1(u8 destBuf[SCE_DDRDB_MUL1_BUFFER_SIZE]) {
    int status = 0x80530300; //0x00002594 & 0x00002598
    int retVal;
    int i;
    
    status = sceKernelWaitSema(semaId, 1); //0x000025A0
    if (status != 0) { //0x000025A8
        return SCE_ERROR_SEMAPHORE; //0x000025B0 & 0x000025B4
    }
    retVal = sceUtilsBufferCopyWithRange(ddrdbBuf, SCE_DDRDB_MUL1_BUFFER_SIZE, NULL, 0, 12); //0x000025E8
    if (retVal == 0) { //0x000025F0
        //0x000025F8 & 0x00002618
        for (i = 0; i < SCE_DDRDB_MUL1_BUFFER_SIZE; i++) {
             destBuf[i] = ddrdbBuf[i]; //0x00002618
        }        
    }
    //0x00002620 - 0x00002638
    for (i = 0; i < SCE_DDRDB_MUL1_BUFFER_SIZE; i++) {
         ddrdbBuf[i] = 0; //0x00002638
    }
    status = sceKernelSignalSema(semaId, 1); //0x00002640
    return (status != 0) ? SCE_ERROR_SEMAPHORE : 0; //0x00002648 & 0x0000264C & 0x00002654
}

/* Subroutine sceDdrdb_driver_EC05300A - Address 0x00002658 */
int sceDdrdbMul2(u8 srcBuf[20], u8 srcBuf1[40], u8 destBuf[SCE_DDRDB_MUL2_DEST_BUFFER_SIZE]) {
    int status;
    int retVal;
    int i;
    
    status = sceKernelWaitSema(semaId, 1); //0x00002698
    if (status != 0) { //0x000026A0
        return SCE_ERROR_SEMAPHORE; //0x0000268C & 0x00002690
    }
    //0x000026A4 - 0x000026C8
    for (i = 0; i < 20; i++) {
         ddrdbBuf[i] = srcBuf[i]; //0x000026C8
    }
    //0x000026CC - 0x000026EC
    for (i = 0; i < 40; i++) {
         ddrdbBuf[20+i] = srcBuf1[i]; //0x000026EC
    }
    retVal = sceUtilsBufferCopyWithRange(ddrdbBuf, SCE_DDRDB_MUL2_DEST_BUFFER_SIZE, ddrdbBuf, SCE_DDRDB_MUL2_SOURCE_BUFFER_SIZE, 13); //0x00002704
    status = 0x80530300; //0x00002708 & 0x00002710
    if (retVal == 0) { //0x0000270C
        //0x00002714 - 0x00002734
        for (i = 0; i < SCE_DDRDB_MUL2_DEST_BUFFER_SIZE; i++) {
             destBuf[i] = ddrdbBuf[i]; //0x00002734
        }      
    }
    //0x00002740 - 0x00002754
    for (i = 0; i < SCE_DDRDB_MUL2_SOURCE_BUFFER_SIZE; i++) {
         ddrdbBuf[i] = 0; //0x00002754
    }
    status = sceKernelSignalSema(semaId, 1); //0x0000275C
    return (status != 0) ? SCE_ERROR_SEMAPHORE : 0; //0x00002764 & 0x00002764 & 0x0000276C
}

/* Subroutine sceDdrdb_driver_E27CE4CB - Address 0x00002358 */
int sceDdrdbSigvry(u8 srcBuf0[40], u8 sha1[20], u8 srcBuf2[40]) {
    int status;
    int i;
    
    status = sceKernelWaitSema(semaId, 1); //0x00002398
    if (status != 0) { //0x000023A0
        return SCE_ERROR_SEMAPHORE; //0x0000238C & 0x00002390
    }
    //0x000023AC - 0x000023C8
    for (i = 0; i < 40; i++) {
         ddrdbBuf[i] = srcBuf0[i]; //0x000023C8
    }
    //0x000023CC - 0x000023EC
    for (i = 0; i < 20; i++) {
         ddrdbBuf[40+i] = sha1[i]; //0x000023EC
    }
    //0x000023F0 - 0x00002410
    for (i = 0; i < 40; i++) {
         ddrdbBuf[60+i] = srcBuf2[i]; //0x00002410
    }
    status = sceUtilsBufferCopyWithRange(NULL, 0, ddrdbBuf, SCE_DDRDB_SIGVRY_BUFFER_SIZE, 17); //0x00002428
    status = (status != 0) ? 0x80530300 : 0; //0x0000242C - 0x00002434
    //0x00002438 - 0x00002450
    for (i = 0; i < SCE_DDRDB_SIGVRY_BUFFER_SIZE; i++) {
         ddrdbBuf[i] = 0; //0x00002450
    }
    status = sceKernelSignalSema(semaId, 1); //0x00002458
    return (status != 0) ? SCE_ERROR_SEMAPHORE : 0; //0x00002460 - 0x00002468
}

/* Subroutine sceDdrdb_driver_370F456A - Address 0x00002494 */
int sceDdrdbCertvry(u8 buf[SCE_DDRDB_CERTVRY_BUFFER_SIZE]) {
    int status;
    int i;
    
    status = sceKernelWaitSema(semaId, 1); //0x000024C4
    if (status != 0) { //0x000024CC
        return SCE_ERROR_SEMAPHORE; //0x000024B8 & 0x000024BC
    }
    //0x000024D4 & 0x000024F4
    for (i = 0; i < SCE_DDRDB_CERTVRY_BUFFER_SIZE; i++) {
         ddrdbBuf[i] = buf[i]; //0x000024F4
    }
    status = sceUtilsBufferCopyWithRange(NULL, 0, ddrdbBuf, SCE_DDRDB_CERTVRY_BUFFER_SIZE, 18); //0x0000250C
    status = (status != 0) ? 0x80530300 : 0; //0x00002510 - 0x00002518
    //0x0000251C - 0x00002534
    for (i = 0; i < SCE_DDRDB_CERTVRY_BUFFER_SIZE; i++) {
         ddrdbBuf[i] = 0; //0x00002534
    }
    status = sceKernelSignalSema(semaId, 1); //0x0000253C
    return (status != 0) ? SCE_ERROR_SEMAPHORE : 0; //0x00002544 - 0x0000254C
}

/* sceDdrdb_driver_B24E1391 - Address 0x00002798 */
int sceDdrdbSiggen(u8 inbuf[32], u8 sha1[20], u8 outbuf[40]) {
    int status;
    int retVal; 
    int i;
    
    status = sceKernelWaitSema(semaId, 1); //0x000027D8
    if (status != 0) { //0x000027E0
        return SCE_ERROR_SEMAPHORE; //0x000027CC & 0x000027E0
    }
    //0x000027E4 - 0x00002808
    for (i = 0; i < 32; i++) {
         ddrdbBuf[i] = inbuf[i]; //0x00002808
    }
    //0x0000280C - 0x0000282C
    for (i = 0; i < 20; i++) {
         ddrdbBuf[i] = sha1[i]; //0x0000282C
    }
    retVal = sceUtilsBufferCopyWithRange(ddrdbBuf, 52, ddrdbBuf, 52, 16); //0x00002844
    status = 0x80530300; //0x00002848 & 0x00002850
    if (retVal == 0) { //0x0000284C
        //0x00002854 - 0x00002874
        for (i = 0; i < 40; i++) {
             outbuf[i] = ddrdbBuf[i]; //0x00002874
        }
        
    }
    //0x00002880 - 0x00002894
    for (i = 0; i < 52; i++) {
         ddrdbBuf[i] = 0; //0x00002894
    }
    status = sceKernelSignalSema(semaId, 1); //0x0000289C
    return (status != 0) ? SCE_ERROR_SEMAPHORE : 0; //0x000028A4 - 0x000028AC
}

/* Subroutine sceDdrdb_driver_B8218473 - Address 0x000028D8 */
int sceDdrdbPrngen(u8 buf[SCE_DDRDB_PRNG_BUFFER_SIZE]) {
    int status = 0x80530300; //0x000028FC & 0x00002900
    int retVal;
    int i;
    
    status = sceKernelWaitSema(semaId, 1); //0x00002908
    if (status != 0) { //0x00002910
        return SCE_ERROR_SEMAPHORE; //0x00002918 & 0x0000291C
    }
    retVal = sceUtilsBufferCopyWithRange(ddrdbBuf, SCE_DDRDB_PRNG_BUFFER_SIZE, NULL, 0, 14); //0x00002950
    if (retVal == 0) { //0x00002958
        //0x00002960 - 0x00002980
        for (i = 0; i < SCE_DDRDB_PRNG_BUFFER_SIZE; i++) {
             buf[i] = ddrdbBuf[i]; //0x00002980
        }     
    }
    //0x00002988 - 0x000029A0
    for (i = 0; i < SCE_DDRDB_PRNG_BUFFER_SIZE; i++) {
         ddrdbBuf[i] = 0; //0x000029A0
    }
    status = sceKernelSignalSema(semaId, 1); //0x000029A8
    return (status != 0) ? SCE_ERROR_SEMAPHORE : 0; //0x000029B0 - 0x000029BC
}

/* Subroutine sceDdrdb_F013F8BF - Address 0x00002E2C */
int sceDdrdb_F013F8BF(u8 srcBuf1[20], u8 srcBuf2[40]) {
    int val;
    int status;
    u32 k1;
    int i;
    
    k1 = pspSdkGetK1();
    pspSdkSetK1(k1 << 11); //0x00002E30
    
    val = srcBuf1 + 20; //0x00002E2C
    val |= srcBuf1; //0x00002E34
    
    if ((val & pspSdkGetK1()) < 0) { //0x00002E3C & 0x00002E68
        pspSdkSetK1(k1); 
        return SCE_ERROR_PRIV_REQUIRED; //0x00002E84 - 0x00002E88
    } 
    status = sceKernelWaitSema(semaId, 1); //0x00002EC4
    if (status != 0) { //0x00002ECC
        pspSdkSetK1(k1);
        return SCE_ERROR_SEMAPHORE; //0x00002EC8 & 0x00002EE0
    }
    //0x00002ED4 - 0x00002F00
    for (i = 0; i < 40; i++) {
         ddrdbBuf[i] = ddrdbBuf2[i]; //0x00002F00
    }
    //0x00002F04 - 0x00002F24
    for (i = 0; i < 20; i++) {
         ddrdbBuf[40+i] = srcBuf1[i]; 
    }
    //0x00002F28 - 0x00002F48
    for (i = 0; i < 40; i++) {
         ddrdbBuf[60+i] = srcBuf2[i]; //0x00002F48
    }
    status = sceUtilsBufferCopyWithRange(NULL, 0, ddrdbBuf, 100, 17); //0x00002F60
    status = (status != 0) ? SCE_ERROR_INVALID_VALUE : 0; //0x00002F64 - 0x00002F6C
    //0x00002F70 - 0x00002F88
    for (i = 0; i < 100; i++) {
         ddrdbBuf[i] = 0; //0x00002F88
    }
    status = sceKernelSignalSema(semaId, 1); //0x00002F90
    pspSdkSetK1(k1);
    return (status != 0) ? SCE_ERROR_SEMAPHORE : 0; //0x00002F98 - 0x00002FA4
}