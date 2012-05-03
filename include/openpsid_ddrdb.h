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

#ifndef DDRDB_H
#define	DDRDB_H

#include "../errors.h"

#ifdef	__cplusplus
extern "C" {
#endif

    /** Maximal size of a source buffer in KB. */
    #define SCE_DDRDB_MAX_BUFFER_SIZE                2048
    /** Size of the added header to the buffer to encrypt. */
    #define SCE_DDRDB_ENCRYPTED_BUFFER_HEADER_SIZE   20 
    /** Size of the added header to the buffer to decrypt. */
    #define SCE_DDRDB_DECRYPTED_BUFFER_HEADER_SIZE   20
    /** Size of the added header to the buffer to create a hash for. */
    #define SCE_DDRDB_HASH_BUFFER_HEADER_SIZE        4
    /** Size of the generated hash buffer. */
    #define SCE_DDRDB_HASH_BUFFER_SIZE               20
    /** Size of the generated MUL1 buffer. */
    #define SCE_DDRDB_MUL1_BUFFER_SIZE               60
    /** Size of the generated MUL2 buffer. */
    #define SCE_DDRDB_MUL2_DEST_BUFFER_SIZE          40
    /** Size of source buffer used to generate the MUL2 buffer. */
    #define SCE_DDRDB_MUL2_SOURCE_BUFFER_SIZE        60
    /** Size of source buffer used to check for. */
    #define SCE_DDRDB_SIGVRY_BUFFER_SIZE             100
    /** Size of source buffer used to check for. */
    #define SCE_DDRDB_CERTVRY_BUFFER_SIZE            184
    /** Size of generated Prng buffer. */
    #define SCE_DDRDB_PRNG_BUFFER_SIZE               20

    /**
     * Decrypt a buffer. 
     * 
     * @note  The 20-byte header for buf into sema function:
     *              header[0] = 5;
     *              header[1] = 0;
     *              header[2] = 0;
     *              header[3] = 0xB;
     *              header[4] = size;
     * 
     * @param buf The buffer to decrypt.
     * @param size The size of the buffer. Max size = 2048 Bytes.
     *
     * @return 0 on success, otherwise < 0.
     */
    int sceDdrdbDecrypt(u8 buf[], int size);
    
    /**
     * Encrypt a buffer. 
     * 
     * @note  The 20-byte header for buf into sema function:
     *              header[0] = 4;
     *              header[1] = 0;
     *              header[2] = 0;
     *              header[3] = 0xB;
     *              header[4] = size;
     * 
     * @param buf The buffer to encrypt.
     * @param size The size of the buffer. Max size = 2048 Bytes.
     *
     * @return 0 on success, otherwise < 0.
     */
    int sceDdrdbEncrypt(u8 buf[], int size);
    
    /** 
     * Generate a SHA-1 hash buffer of the source buffer. 
     * 
     * @note The 4-byte header for srcBuf into sema function:
     *              header[0] = size;
     * 
     * @param buf The source buffer to generate the hash from.
     * @param size The size of source buffer. Max size = 2048 Bytes
     * @param hash The destination buffer for the hash. Size = 20.
     *
     * @return 0 on success, otherwise < 0.
     */
    int sceDdrdbHash(u8 srcBuf[], int size, u8 hash[SCE_DDRDB_HASH_BUFFER_SIZE]);
    
    int sceDdrdbMul1(u8 destBuf[SCE_DDRDB_MUL1_BUFFER_SIZE]);
    
    int sceDdrdbMul2(u8 srcBuf[20], u8 srcBuf1[40], u8 destBuf[SCE_DDRDB_MUL2_DEST_BUFFER_SIZE]);
    
    int sceDdrdbSigvry(u8 srcBuf0[40], u8 sha1[20], u8 srcBuf2[40]);
    
    int sceDdrdbCertvry(u8 buf[SCE_DDRDB_CERTVRY_BUFFER_SIZE]);
    
    int sceDdrdbSiggen(u8 inbuf[32], u8 sha1[20], u8 outbuf[40]);
    
    /**
     * Generate a 20-Byte pseudorandom number. 
     * 
     * @note No need to seed it as KIRK is initialized automatically on boot.
     * 
     * @param buf The destination buffer for the pseudorandom number. Size = 20.
     *
     * @return 0 on success, otherwise < 0.
     */
    int sceDdrdbPrngen(u8 buf[SCE_DDRDB_PRNG_BUFFER_SIZE]);
    
    int sceDdrdb_F013F8BF(u8 srcBuf1[20], u8 srcBuf2[40]);



#ifdef	__cplusplus
}
#endif

#endif	/* DDRDB_H */

