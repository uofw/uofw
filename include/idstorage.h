/* Copyright (C) The uOFW team
   See the file COPYING for copying permission.
*/

#ifndef IDSTORAGE_H
#define IDSTORAGE_H

#include "common_header.h"
#include "openpsid_kernel.h"

/* Size of an ID storage leaf. In byte. */
#define SCE_ID_STORAGE_LEAF_SIZE   512

#define SCE_ID_STORAGE_LEAF_CONSOLE_ID_OPEN_PSID_1                    0x100
#define SCE_ID_STORAGE_LEAF_CONSOLE_ID_OPEN_PSID_2                    0x101
#define SCE_ID_STORAGE_LEAF_CONSOLE_ID_OPEN_PSID_3_UMD_1              0x102
#define SCE_ID_STORAGE_LEAF_ID_UMD_2                                  0x103
#define SCE_ID_STORAGE_LEAF_ID_UMD_3                                  0x104
#define SCE_ID_STORAGE_LEAF_ID_UMD_4                                  0x105
#define SCE_ID_STORAGE_LEAF_ID_UMD_5                                  0x106

#define SCE_ID_STORAGE_LEAF_ID_BACKUP_CONSOLE_ID_OPEN_PSID_1          0x120
#define SCE_ID_STORAGE_LEAF_ID_BACKUP_CONSOLE_ID_OPEN_PSID_2          0x121
#define SCE_ID_STORAGE_LEAF_ID_BACKUP_CONSOLE_ID_OPEN_PSID_3_UMD_1    0x122
#define SCE_ID_STORAGE_LEAF_ID_BACKUP_UMD_2                           0x123
#define SCE_ID_STORAGE_LEAF_ID_BACKUP_UMD_3                           0x124
#define SCE_ID_STORAGE_LEAF_ID_BACKUP_UMD_4                           0x125
#define SCE_ID_STORAGE_LEAF_ID_BACKUP_UMD_5                           0x126

/* Offset into the leaf [SCE_ID_STORAGE_LEAF_CONSOLE_ID_OPEN_PSID_1] to the start of the first ConsoleId certificate. */
#define SCE_ID_STORAGE_LEAF_CONSOLE_ID_OPEN_PSID_1_OFFSET_CONSOLE_ID_CERTIFICATE_1    0x38
/* Offset into the leaf [SCE_ID_STORAGE_LEAF_CONSOLE_ID_OPEN_PSID_2] to the start of the OpenPSID certificate. */
#define SCE_ID_STORAGE_LEAF_CONSOLE_ID_OPEN_PSID_2_OFFSET_OPEN_PSID_CERTIFICATE       0x1D0

#define SCE_ID_STORAGE_LEAF_CONSOLE_ID_OPEN_PSID_3_UMD_1_OFFSET_REGION_CODES          0xB0

/*
 * ID Storage layout (WIP):
 * 
 * Each ID storage leaf has a size of 512 bytes. Some leafs are "self-contained", that is, they do not share data with
 * their neighboring leafs. Examples for such leafs are the leafs with IDs 0x4 - 0x8. Other leafs are "continious" leafs,
 * meaning they share data with their neighboring leafs. Examples of such leafs are the leafs 0x100 - 0x102.
 * 
 * 
 *                                .
 *                                .
 *                                .
 * 
 * The leafs 0x100 - 0x102 contain the ConsoleId (termed "PSID" on the PSP) and the OpenPSID. As an additional security feature
 * to prevent successfully tampering them, they are stored inside a certificate, which can be used to verify the ConsoleId and
 * OpenPSID. For more info, see SceIdStorageConsoleIdCertificate and SceIdStorageOpenPSIDCertificate.
 * 
 * These leafs are "continious leafs" and contain 5 ConsoleId certificates (totalling 5 * 184 = 920 bytes)
 * followed by a single OpenPSID certificate (for a total size of 920 + 184 = 1104 bytes). 
 * 
 * Of these 5 certificates, only the first ConsoleId certificate and the OpenPSID certificate are used in the system.
 * It is unknown what the purpose the remaining four ConsoleId certificates is.
 * 
 * +------------------------------------------------------------+----------------
 * |                 unknown data (size: 0x38)                  |                |
 * |------------------------------------------------------------|                |
 * |       SceIdStorageConsoleIdCertificate (size: 0xB8)        |                |
 * |------------------------------------------------------------|            Leaf 0x100
 * |       SceIdStorageConsoleIdCertificate (size: 0xB8)        |                |
 * |------------------------------------------------------------|                |
 * |     SceIdStorageConsoleIdCertificate_part1 (size: 0x58)    |                |
 * |------------------------------------------------------------|----------------
 * |     SceIdStorageConsoleIdCertificate_part2 (size: 0x60)    |                |
 * |------------------------------------------------------------|                |
 * |       SceIdStorageConsoleIdCertificate (size: 0xB8)        |                |
 * |------------------------------------------------------------|            Leaf 0x101
 * |       SceIdStorageConsoleIdCertificate (size: 0xB8)        |                |
 * |------------------------------------------------------------|                |
 * |     SceIdStorageOpenPSIDCertificate_part1 (size: 0x30)     |                |
 * |------------------------------------------------------------|----------------
 * |     SceIdStorageOpenPSIDCertificate_part2 (size: 0x88)     |                |
 * |------------------------------------------------------------|            Leaf 0x102
 * |                 unknown data (size: 0x178)                 |                |
 * |------------------------------------------------------------|----------------
 *                               .
 *                               .
 *                               .
 * 
 * The leafs 0x120 - 0x126 are backups of leafs 0x100 - 0x106. See the description of those leafs for more information.
 * 
 *                               .
 *                               .
 *                               .
 *
 */

/*
 * This structure contains the ConsoleId (termed "PSID" on the PSP) and an ECDSA signature used to verify the correctness of the 
 * ConsoleId.
 * The ConsoleId is used, for example, in PSN DRM, DNAS and system configuration (with its derived PSCode).
 */
typedef struct {
	/* Unique per-console identifier. */
	SceConsoleId consoleId; // 0
	/* Contains the public key of the certificate. No padding. */
	u8 plantextPublicKey[0x28]; // 16
	/* The 'r' part of the ECDSA signature pair (r, s). */
	u8 r[0x14]; // 56
	/* The 's' part of the ECDSA signature pair (r, s). */
	u8 s[0x14]; // 76
	/* The ECDSA public key (can be used to verify ECDSA signature rs). */
	u8 publicKey[0x28]; // 96
	/* Contains the encrypted private key of the certificate (with padding). */
	u8 encPrivateKey[0x20]; // 136
	/* Hash of previous data. */
	u8 hash[0x10]; // 168
} SceIdStorageConsoleIdCertificate; // size = 184

/*
 * This structure contains the OpenPSID (random 16 byte value) and an ECDSA signature used to verify the correctness
 * of the OpenPSID. The OpenPSID value is used, for example, in PSN matchmaking.
 */
typedef struct {
	/* Unique per-console identifier. */
	u8 openPSID[0x10]; // 0
	/* Contains the public key of the certificate. No padding. */
	u8 plantextPublicKey[0x28]; // 16
	/* The 'r' part of the ECDSA signature pair (r, s). */
	u8 r[0x14]; // 56
	/* The 's' part of the ECDSA signature pair (r, s). */
	u8 s[0x14]; // 76
	/* The ECDSA public key (can be used to verify ECDSA signature rs). */
	u8 publicKey[0x28]; // 96
	/* Contains the encrypted private key of the certificate (with padding). */
	u8 encPrivateKey[0x20]; // 136
	/* Hash of previous data. */
	u8 hash[0x10]; // 168
} SceIdStorageOpenPSIDCertificate; // size = 184

typedef struct {
	u32 regionCode; // 0
	u32 unk4; // 4
} SceIdStorageUMDRegionCodeInfo;

s32 sceIdStorageLookup(u16 key, u32 offset, void *pBuf, u32 len);
s32 sceIdStorageReadLeaf(u16 id, void *buf);

#endif // IDSTORAGE_H
