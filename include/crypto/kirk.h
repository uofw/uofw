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

/*!< The length (1472 bit) of the certificate to verify. Used for example for ID Storage keys 0x100 and 0x101. */
#define KIRK_CERT_LEN               (184)

/* Kirk command specific return values */

#define KIRK_OPERATION_SUCCESS      0x0
#define KIRK_NOT_ENABLED            0x1
#define KIRK_INVALID_MODE           0x2
#define KIRK_HEADER_HASH_INVALID    0x3
#define KIRK_DATA_HASH_INVALID      0x4
#define KIRK_SIG_CHECK_INVALID      0x5
#define KIRK_UNK_1                  0x6
#define KIRK_UNK_2                  0x7
#define KIRK_UNK_3                  0x8
#define KIRK_UNK_4                  0x9
#define KIRK_UNK_5                  0xA
#define KIRK_UNK_6                  0xB
#define KIRK_NOT_INITIALIZED        0xC
#define KIRK_INVALID_OPERATION      0xD
#define KIRK_INVALID_SEED_CODE      0xE
#define KIRK_INVALID_SIZE           0xF
#define KIRK_DATA_SIZE_ZERO         0x10


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

