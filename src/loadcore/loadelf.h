/* Copyright (C) 2011, 2012 The uOFW team
   See the file COPYING for copying permission.
*/

#ifndef LOADELF_H
#define	LOADELF_H

#include <loadcore.h>
#include <mesgled.h>
#include "elf.h"
#include "loadcore_int.h"
#include "module.h"
    
#define SCE_HEADER_BETA_VERSION     (66)
    
/* For compatibility reasons. Use SCE_MAGIC_LE. */
#define SCE_MAGIC                   (0x7E534345) /* "~SCE" */

#define SCE_MAGIC_LE				(0x4543537E) /* "~SCE" in Little Endian */

#define SCE_HEADER_SIZE             (64)

#define AES_KEY_SIZE                (16)

#define CMAC_KEY_SIZE               (16)

#define CMAC_HEADER_HASH_SIZE       (16)

#define CMAC_DATA_HASH_SIZE         (16)

#define SHA1_HASH_SIZE              (20)

#define KEY_DATA_SIZE               (16)

#define CHECK_SIZE                  (88)

/* 
 * Decryption mode executable file types.
 */
enum SceExecFileDecryptMode {
    /* Not an executable. */
    DECRYPT_MODE_NO_EXEC = 0,
    /* 1.50 Kernel module. */
    DECRYPT_MODE_BOGUS_MODULE = 1,
    DECRYPT_MODE_KERNEL_MODULE = 2,
    DECRYPT_MODE_VSH_MODULE = 3,
    DECRYPT_MODE_USER_MODULE = 4,
    DECRYPT_MODE_UMD_GAME_EXEC = 9,
    DECRYPT_MODE_GAMESHARING_EXEC = 10,
    /* USB/WLAN module. */
    DECRYPT_MODE_UNKNOWN_11 = 11,
    DECRYPT_MODE_MS_UPDATER = 12,
    DECRYPT_MODE_DEMO_EXEC = 13,
    DECRYPT_MODE_APP_MODULE = 14,
    DECRYPT_MODE_UNKNOWN_18 = 18,
    DECRYPT_MODE_UNKNOWN_19 = 19,
    DECRYPT_MODE_POPS_EXEC = 20,
    /* MS module. */
    DECRYPT_MODE_UNKNOWN_21 = 21,
    /* APP module. */
    DECRYPT_MODE_UNKNOWN_22 = 22,
    /* USER module. */
    DECRYPT_MODE_UNKNOWN_23 = 23,
    /* USER module. */
    DECRYPT_MODE_UNKNOWN_25 = 25,
};

typedef struct {
    /* SCE_MAGIC */
    u8 magic[4]; //0
    /* The size of the header. Should be SCE_HEADER_SIZE. */
    u32 size; //4
    /* The version of the SCE Header. */
    u8 hdrVersion; //8
    /* Unknown. */
    u32 unk12; //12
    /* Unknown. */
    u32 unk16; //16
    /* Unknown. */
    u32 unk20; //20
    /* Unknown. */
    u32 unk24; //24
    /* Unknown. */
    u32 unk28; //28
    /* Unknown. */
    u32 unk32; //32
    /* Unknown. */
    u32 unk36; //36
    /* Unknown. */
    u32 unk40; //40
    /* Unknown. */
    u32 unk44; //44
    /* Unknown. */
    u32 unk48; //48
    /* Unknown. */
    u32 unk52; //52
    /* Unknown. */
    u32 unk56; //56
    /* Unknown. */
    u32 unk60; //60   
} SceHeader; //size = SCE_HEADER_SIZE

/*
 * PSP Header - This header is used in encrypted ELF files, as well as 
 * GZIP'ed compressed ones. 
 */
typedef struct {
    /* PSP Header magic */
    u8 magic[4]; //0
    /* The module's attributes. One or more of ::SceModulePrivilegeLevel. */
    u16 modAttribute; //4
    /* The Compression attributes of the module. One of ::SceExecFileAttr. */
    u16 compAttribute; //6
    /* The minor module version. */
    u8 moduleVerLo; //8
    /* The major module version. */
    u8 moduleVerHi; //9
    /* The module's name. */
    s8 modName[SCE_MODULE_NAME_LEN + 1]; //10
    /* Module version. */
    u8 modVersion; //38
    /* The number of segment the module consists of. */
    u8 nSegments; //39
    /* The size of the uncompressed and decrypted module. */
    u32 elfSize; //40
    /* The size of the compressed/encrypted module. */
    u32 pspSize; //44
    /* The entry address of the module. It is the offset from the start of the TEXT segment to the program's entry point. */
    u32 bootEntry; //48
    /* The offset from the start address of the TEXT segment to the SceModuleInfo section. */
    u32 modInfoOffset; //52
    /* The size of the BSS segment. */
    u32 bssSize; //56
    /* An array containing the alignment information of each segment. */
    u16 segAlign[SCE_KERNEL_MAX_MODULE_SEGMENT]; //60
    /* An array containing the start address of each segment. */
    u32 segAddress[SCE_KERNEL_MAX_MODULE_SEGMENT]; //68
    /* An array containing the size of each segment. */
    u32 segSize[SCE_KERNEL_MAX_MODULE_SEGMENT]; //84
    /* Reserved. */
    u32 reserved[5]; //100
    /* The development kit version the module was compiled with. */
    u32 devkitVersion; //120
    /* The decryption mode. One of ::SceExecFileDecryptMode. */
    u8 decryptMode; //124
    /* Reserved. */
    u8 padding; //125
    /* The overlap size. */
    u16 overlapSize; //126
    /* The AES key data. */
    u8 aesKey[AES_KEY_SIZE]; //128
    /* THE CMAC key data. */
    u8 cmacKey[CMAC_KEY_SIZE]; //144
    /* The CMAC header hash. */
    u8 cmacHeaderHash[CMAC_HEADER_HASH_SIZE]; //160
    /* The size of the compressed ELF. */
    u32 compSize; //176
    /* Unknown. */
    u32 unk180; //180
    /* Unknown. */
    u32 unk184; //184
    /* Unknown. */
    u32 unk188; //188
    /* The CMAC Data hash. */
    u8 cmacDataHash[CMAC_DATA_HASH_SIZE]; //192
    /* Tag value. */
    u32 tag; //208
    /* Check. */
    u8 sCheck[CHECK_SIZE]; //212
    /* The SHA-1 hash. */
    u8 sha1Hash[SHA1_HASH_SIZE]; //300
    /* Key data. */
    u8 keyData4[KEY_DATA_SIZE]; //320
} PspHeader; //size == 336


#endif	/* LOADELF_H */