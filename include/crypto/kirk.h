/* Copyright (C) 2011 - 2016 The uOFW team
   See the file COPYING for copying permission.
*/

/** 
 * @defgroup Crypto Crypto
 * PSP Cryptography modules.
 */

/**
 * @defgroup KIRK KIRK
 * @ingroup Crypto
 *
 * The hardware crypto engine responsible for almost all aspects of the PSP's security, \n
 * including decryption of EBOOTs & PRX's, savefile and adhoc encryption, and idstorage verification.
 *
 * @{
 */

#ifndef KIRK_H
#define KIRK_H

#include <common_header.h>

/* KIRK commands */

#define KIRK_CMD_ENCRYPT_AES_CBC_IV_NONE        (0x04)
#define KIRK_CMD_ENCRYPT_AES_CBC_IV_FUSE        (0x05)
#define KIRK_CMD_ENCRYPT_AES_CBC_IV_USER        (0x06)
#define KIRK_CMD_DECRYPT_AES_CBC_IV_NONE        (0x07)
#define KIRK_CMD_DECRYPT_AES_CBC_IV_FUSE        (0x08)
#define KIRK_CMD_DECRYPT_AES_CBC_IV_USER        (0x09)
#define KIRK_CMD_HASH_GEN_SHA1                  (0x0B)
#define KIRK_CMD_KEY_GEN_ECDSA                  (0x0C)
#define KIRK_CMD_POINT_MULTIPLICATION_ECDSA     (0x0D)
#define KIRK_CMD_PRN_GEN                        (0x0E)
#define KIRK_CMD_SIG_GEN_ECDSA                  (0x10)
#define KIRK_CMD_SIG_VER_ECDSA                  (0x11)
#define KIRK_CMD_CERT_VER                       (0x12)

/* Specific values for algorithms used by KIRK. */

#define KIRK_SHA1_DIGEST_LEN        (20) /*!< The length (160 bit) of a SHA-1 hash value. */

#define KIRK_PRN_LEN                (20) /*!< The length (160 bit) of a computed pseudo-random number. */

#define KIRK_ECDSA_POINT_LEN        (40) /*!< The length (320 bit) of an elliptic curve point p = (x,y), with len(x) = 160 bit = len(y). */
#define KIRK_ECDSA_PUBLIC_KEY_LEN   (KIRK_ECDSA_POINT_LEN) /*!< The length (320 bit) of the public key for ECDSA. */
#define KIRK_ECDSA_PRIVATE_KEY_LEN  (20) /*!< The length (256 bit) of the private key for ECDSA. */
#define KIRK_ECDSA_SRC_DATA_LEN     (20) /*!< The length (160 bit) of the data to compute the signature for using ECDSA. */
#define KIRK_ECDSA_SIG_LEN          (40) /*!< The length (320 bit) of the signature computed by ECDSA. */

#define KIRK_AES_BLOCK_LEN          (16) /*!< The length (128 bit) of a block to encrypt using AES. */

#define KIRK_CERT_LEN               (184) /*!< The length (1472 bit) of the certification to verify. */


typedef struct {
    SceSize dataSize;
} KirkSHA1Hdr;

typedef struct {
    s32 mode;
    s32 unk4;
    s32 unk8;
    s32 keyIndex;
    SceSize dataSize;
} KirkAESHeader;

#endif /* KIRK_H */

/** @} */

