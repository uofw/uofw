/* Copyright (C) 2011 - 2016 The uOFW team
   See the file COPYING for copying permission.
*/

/** @defgroup DNAS Dynamic Network Authentication System
 *  An authentication system used for online authentication \n
 *  (for example with a game server).
 */

/** 
 * @defgroup OpenPSID OpenPSID
 * @ingroup DNAS
 *
 * @{
 */

#ifndef OPENPSID_DDRDB_H
#define	OPENPSID_DDRDB_H

#include "common_header.h"

#ifdef	__cplusplus
extern "C" {
#endif

    /** Maximum size of a source buffer in byte. */
    #define SCE_DNAS_USER_DATA_MAX_LEN      2048

    /**
     * Decrypt the provided data. The data has to be AES encrypted. 
     *
     * @note The used key is provided by the PSP.
     * 
     * @param pSrcData Pointer to data to decrypt. The decrypted data will be written \n
     *                 back into this buffer.
     * @param size The size of the data to decrypt. The size needs to be a multiple of ::KIRK_AES_BLOCK_LEN. \n
                   Max size: ::SCE_DNAS_USER_DATA_MAX_LEN.
     *
     * @return 0 on success, otherwise < 0.
     */
    s32 sceDdrdbDecrypt(u8 *pSrcData, SceSize size);
    
    /**
     * Encrypt the provided data. It will be encrypted using AES.
     *
     * @note The used key is provided by the PSP.
     * 
     * @param pSrcData Pointer to data to encrypt. The encrypted data will be written 
     *                 back into this buffer.
     * @param size The size of the data to encrypt. The size needs to be a multiple of ::KIRK_AES_BLOCK_LEN. \n
                   Max size: ::SCE_DNAS_USER_DATA_MAX_LEN.
     *
     * @return 0 on success, otherwise < 0.
     */
    s32 sceDdrdbEncrypt(u8 *pSrcData, SceSize size);
    
    /** 
     * Generate a SHA-1 hash value of the provided data.
     * 
     * @param pSrcData Pointer to data to generate the hash for.
     * @param size The size of the source data. Max size: ::SCE_DNAS_USER_DATA_MAX_LEN.
     * @param pDigest Pointer to buffer receiving the hash. Size: ::KIRK_SHA1_DIGEST_LEN.
     *
     * @return 0 on success, otherwise < 0.
     */
    s32 sceDdrdbHash(u8 *pSrcData, SceSize size, u8 *pDigest);
    
    /**
     * Generate a new (public,private) key pair to use with ECDSA.
     *
     * @param pKeyData Pointer to buffer receiving the computed key pair. \n
     *                 The first ::KIRK_ECDSA_PRIVATE_KEY_LEN byte will contain the private key. \n
     *                 The rest of the bytes will contain the public key (elliptic curve) point p = (x,y), \n
     *                 with the x-value being first. Both coordinates have size ::KIRK_ECDSA_POINT_LEN / 2.
     *
     * @return 0 on success, otherwise < 0.
     */
    s32 sceDdrdbMul1(u8 *pKeyData);

    /**
     *
     * Compute a new elliptic curve point by multiplying the provided private key with the \n
     * provided base point of the elliptic curve.
     *
     * @param pPrivKey Pointer to the private key of a (public,private) key pair usable for ECDSA.
     *                 
     * @param pBasePoint Pointer to a base point of the elliptic curve. Point size: ::KIRK_ECDSA_POINT_LEN
     * @param pNewPoint Pointer to a buffer receiving the new curve point. Buffer size: ::KIRK_ECDSA_POINT_LEN
     *
     * @return 0 on success, otherwise < 0.
     */
    s32 sceDdrdbMul2(u8 *pPrivKey, u8 *pBasePoint, u8 *pNewPoint);
    
    /**
     * Verify if the provided signature is valid for the specified data given the public key. 
     *
     * @note The ECDSA algorithm is used to verify a signature.
     *
     * @param pPubKey The public key used for validating the (data,signature) pair. \n
     *                Size has to be ::KIRK_ECDSA_PUBLIC_KEY_LEN.
     * @param pData Pointer to data the signature has to be verified for. \n
                    Data length: ::KIRK_ECDSA_SRC_DATA_LEN \n
     * @param pSig Pointer to the signature to verify. Signature length: ::KIRK_ECDSA_SIG_LEN
     *
     * @return 0 on success, otherwise < 0.
     */
    s32 sceDdrdbSigvry(u8 *pPubKey, u8 *pData, u8 *pSig);
    
    /**
     * Verify a certificate.
     *
     * @param pCert Pointer to the certificate to verify. Certificate length: ::KIRK_CERT_LEN.
     *
     * @return 0 on success, otherwise < 0.
     */
    s32 sceDdrdbCertvry(u8 *pCert);
    
    /**
     * Generate a valid signature for the specified data using the specified private key.
     *
     * @note The ECDSA algorithm is used to generate a signature.
     *
     * @param pPrivKey Pointer to the private key used to generate the signature. \n
     *                 CONFIRM: The key has to be AES encrypted before.
     * @param pData Pointer to data a signature has to be computed for. Data length: ::KIRK_ECDSA_SRC_DATA_LEN
     * @param pSig Pointer to a buffer receiving the signature. Signature length: ::KIRK_ECDSA_SIG_LEN
     *
     * @return 0 on success, otherwise < 0.
     */
    s32 sceDdrdbSiggen(u8 *pPrivKey, u8 *pSrcData, u8 *pSig);
    
    /**
     * Generate a ::KIRK_PRN_LEN large pseudorandom number (PRN). 
     * 
     * @note The seed is automatically set by the system software.
     * 
     * @param pDstData Pointer to buffer receiving the PRN. Size has to be ::KIRK_PRN_LEN.
     *
     * @return 0 on success, otherwise < 0.
     */
    s32 sceDdrdbPrngen(u8 *pDstData);
    
    /**
     * Verify if the provided signature is valid for the specified data. The public key\n
     * is provided by the system software.
     *
     * @note The ECDSA algorithm is used to verify a signature.
     *
     * @param pData Pointer to data the signature has to be verified for. \n
     *              Data length: ::KIRK_ECDSA_SRC_DATA_LEN.
     * @param pSig Pointer to the signature to verify. Signature length: ::KIRK_ECDSA_SIG_LEN.
     *
     * @return 0 on success, otherwise < 0.
     */
    s32 sceDdrdb_F013F8BF(u8 *pData, u8 *pSig);

#ifdef	__cplusplus
}
#endif

#endif	/* OPENPSID_DDRDB_H */

/** @} */

