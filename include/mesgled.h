/* Copyright (C) 2022 The uOFW team
   See the file COPYING for copying permission.
*/

#include <common_header.h>

//TODO doc
s32 sceMesgLed_driver_9E3C79D9(u8 *psp, u32 pspSize, u64 *appTicks);
// s32 sceMesgLed_driver_9E3C79D9(u8 *modBuf, u32 arg1, u32 *buf);

/**
 * Decrypt a VSH module (mode 3 DECRYPT_MODE_VSH_MODULE).
 * Supported tags (2g):
 * - 0x380291F0 (type 9, key index 0x5a)
 * - 0x380290F0 (type 9, key index 0x5a)
 * - 0x02000000 (type 8, key index 0x45, 144-bytes seed)
 * @param psp ~PSP encrypted module buffer (MUST be aligned on a 64-bytes boundary).
 * @param pspSize Size of the encrypted PRX module (MUST be >= 0x160 bytes).
 * @param pNewSize Required pointer to receive the plain module size.
 * @return 0 success; < 0 SCE driver error
 */
s32 sceUtilsGetLoadModuleCLength(u8 *psp, u32 pspSize, u32 *pNewSize); // sceMesgLed_driver_5C3A61FE

/**
 * Same as sceUtilsGetLoadModuleCLength() but by polling.
 * @param psp ~PSP encrypted module buffer (MUST be aligned on a 64-bytes boundary).
 * @param pspSize Size of the encrypted PRX module (MUST be >= 0x160 bytes).
 * @param pNewSize Required pointer to receive the plain module size.
 * @return 0 success; < 0 SCE driver error
 */
s32 sceUtilsGetLoadModuleCLengthByPolling(u8 *psp, u32 pspSize, u32 *pNewSize); // sceMesgLed_driver_3783B0AD

/**
 * Decrypt a user module (mode 4 DECRYPT_MODE_USER_MODULE).
 * Supported tags (2g):
 * - 0x457B91F0 (type 9, key index 0x5b)
 * - 0x457B90F0 (type 9, key index 0x5b)
 * - 0x457B8AF0 (type 6, key index 0x5b)
 * - 0x457B80F0 (type 6, key index 0x5b)
 * - 0x457B10F0 (type 2, key index 0x5b)
 * - 0x457B0CF0 (type 2, key index 0x5b)
 * - 0x457B0BF0 (type 2, key index 0x5b)
 * - 0x457B0AF0 (type 2, key index 0x5b)
 * - 0x457B08F0 (type 2, key index 0x5b)
 * - 0x457B06F0 (type 2, key index 0x5b)
 * - 0x457B05F0 (type 2, key index 0x5b)
 * - 0x76202403 (type 2, key index 0x5b)
 * - 0x3ACE4DCE (type 1, key index 0x5b, 144-bytes seed)
 * - 0x03000000 (type 0, key index 0x46, 144-bytes seed)
 * @param psp ~PSP encrypted module buffer (MUST be aligned on a 64-bytes boundary).
 * @param pspSize Size of the encrypted PRX module (MUST be >= 0x160 bytes).
 * @param pNewSize Required pointer to receive the plain module size.
 * @return 0 success; < 0 SCE driver error
 */
s32 sceUtilsGetLoadModuleDLength(u8 *psp, u32 pspSize, u32 *pNewSize); // sceMesgLed_driver_2CB700EC

/**
 * Same as sceUtilsGetLoadModuleDLength() but by polling.
 * @param psp ~PSP encrypted module buffer (MUST be aligned on a 64-bytes boundary).
 * @param pspSize Size of the encrypted PRX module (MUST be >= 0x160 bytes).
 * @param pNewSize Required pointer to receive the plain module size.
 * @return 0 success; < 0 SCE driver error
 */
s32 sceUtilsGetLoadModuleDLengthByPolling(u8 *psp, u32 pspSize, u32 *pNewSize); // sceMesgLed_driver_308D37FF

/**
 * Decrypt a UMD game executable (mode 9 DECRYPT_MODE_UMD_GAME_EXEC).
 * Supported tags (2g):
 * - 0xD91690F0 (type 9, key index 0x5d)
 * - 0xD91680F0 (type 6, key index 0x5d)
 * - 0xD91610F0 (type 2, key index 0x5d)
 * - 0xD91611F0 (type 2, key index 0x5d)
 * - 0xD9160BF0 (type 2, key index 0x5d)
 * - 0xD9160AF0 (type 2, key index 0x5d)
 * - 0xD91606F0 (type 2, key index 0x5d)
 * - 0xD91605F0 (type 2, key index 0x5d)
 * - 0x8004FD03 (type 2, key index 0x5d)
 * - 0xC0CB167C (type 1, key index 0x5d, 144-bytes seed)
 * - 0x08000000 (type 0, key index 0x4b, 144-bytes seed)
 * @param psp ~PSP encrypted module buffer (MUST be aligned on a 64-bytes boundary).
 * @param pspSize Size of the encrypted PRX module (MUST be >= 0x160 bytes).
 * @param pNewSize Required pointer to receive the plain module size.
 * @return 0 success; < 0 SCE driver error
 */
s32 sceUtilsGetLoadModuleILength(u8 *psp, u32 pspSize, u32 *pNewSize); // sceMesgLed_driver_337D0DD3

/**
 * Same as sceUtilsGetLoadModuleILength() but by polling.
 * @param psp ~PSP encrypted module buffer (MUST be aligned on a 64-bytes boundary).
 * @param pspSize Size of the encrypted PRX module (MUST be >= 0x160 bytes).
 * @param pNewSize Required pointer to receive the plain module size.
 * @return 0 success; < 0 SCE driver error
 */
s32 sceUtilsGetLoadModuleILengthByPolling(u8 *psp, u32 pspSize, u32 *pNewSize); // sceMesgLed_driver_792A6126

/**
 * Decrypt a game sharing executable for retail console (mode 10 DECRYPT_MODE_GAMESHARING_EXEC).
 * Supported tags (2g):
 * - 0x7B0510F0 (type 2, key index 0x5e)
 * - 0x7B0508F0 (type 2, key index 0x5e)
 * - 0x7B0506F0 (type 2, key index 0x5e)
 * - 0x7B0505F0 (type 2, key index 0x5e)
 * - 0x0A35EA03 (type 2, key index 0x5e)
 * - 0xBB67C59F (type 1, key index 0x5e, 144-bytes seed)
 * - 0x09000000 (type 0, key index 0x4c, 144-bytes seed)
 * @param psp ~PSP encrypted module buffer (MUST be aligned on a 64-bytes boundary).
 * @param pspSize Size of the encrypted PRX module (MUST be >= 0x160 bytes).
 * @param pNewSize Required pointer to receive the plain module size.
 * @return 0 success; < 0 SCE driver error
 */
s32 sceUtilsGetLoadModuleJLength(u8 *psp, u32 pspSize, u32 *pNewSize); // sceMesgLed_driver_4EAB9850

/**
 * Same as sceUtilsGetLoadModuleJLength() but by polling.
 * @param psp ~PSP encrypted module buffer (MUST be aligned on a 64-bytes boundary).
 * @param pspSize Size of the encrypted PRX module (MUST be >= 0x160 bytes).
 * @param pNewSize Required pointer to receive the plain module size.
 * @return 0 success; < 0 SCE driver error
 */
s32 sceUtilsGetLoadModuleJLengthByPolling(u8 *psp, u32 pspSize, u32 *pNewSize); // sceMesgLed_driver_4BE02A12

/**
 * Decrypt a game sharing executable for DEVTOOL (mode 11 DECRYPT_MODE_GAMESHARING_EXEC_DEVTOOL).
 * Supported tags (2g):
 * - 0xEFD210F0 (type 2, key index 0x4d)
 * - 0x0A000000 (type 0, key index 0x4d, 144-bytes seed)
 * @param psp ~PSP encrypted module buffer (MUST be aligned on a 64-bytes boundary).
 * @param pspSize Size of the encrypted PRX module (MUST be >= 0x160 bytes).
 * @param pNewSize Required pointer to receive the plain module size.
 * @return 0 success; < 0 SCE driver error
 */
s32 sceUtilsGetLoadModuleKLength(u8 *psp, u32 pspSize, u32 *pNewSize); // sceMesgLed_driver_B2CDAC3F

/**
 * Same as sceUtilsGetLoadModuleKLength() but by polling.
 * @param psp ~PSP encrypted module buffer (MUST be aligned on a 64-bytes boundary).
 * @param pspSize Size of the encrypted PRX module (MUST be >= 0x160 bytes).
 * @param pNewSize Required pointer to receive the plain module size.
 * @return 0 success; < 0 SCE driver error
 */
s32 sceUtilsGetLoadModuleKLengthByPolling(u8 *psp, u32 pspSize, u32 *pNewSize); // sceMesgLed_driver_B5596BE4

/**
 * Decrypt an updater module (mode 12 DECRYPT_MODE_MS_UPDATER).
 * Supported tags (2g):
 * - 0x0B000000 (type 8, key index 0x4e, 144-bytes seed)
 * @param psp ~PSP encrypted module buffer (MUST be aligned on a 64-bytes boundary).
 * @param pspSize Size of the encrypted PRX module (MUST be >= 0x160 bytes).
 * @param pNewSize Required pointer to receive the plain module size.
 * @return 0 success; < 0 SCE driver error
 */
s32 sceUtilsGetLoadModuleLLength(u8 *psp, u32 pspSize, u32 *pNewSize); // sceMesgLed_driver_C79E3488

/**
 * Same as sceUtilsGetLoadModuleLLength() but by polling.
 * @param psp ~PSP encrypted module buffer (MUST be aligned on a 64-bytes boundary).
 * @param pspSize Size of the encrypted PRX module (MUST be >= 0x160 bytes).
 * @param pNewSize Required pointer to receive the plain module size.
 * @return 0 success; < 0 SCE driver error
 */
s32 sceUtilsGetLoadModuleLLengthByPolling(u8 *psp, u32 pspSize, u32 *pNewSize); // sceMesgLed_driver_6BF453D3

/**
 * Decrypt a demo executable (mode 13 DECRYPT_MODE_DEMO_EXEC).
 * Supported tags (2g):
 * - 0xADF310F0 (type 4, key index 0x60)
 * - 0xADF308F0 (type 4, key index 0x60)
 * - 0xADF306F0 (type 4, key index 0x60)
 * - 0xADF305F0 (type 4, key index 0x60)
 * - 0xD67B3303 (type 2, key index 0x60)
 * - 0x7F24BDCD (type 1, key index 0x60, 144-bytes seed)
 * - 0x0C000000 (type 0, key index 0x4f, 144-bytes seed)
 * @param psp ~PSP encrypted module buffer (MUST be aligned on a 64-bytes boundary).
 * @param pspSize Size of the encrypted PRX module (MUST be >= 0x160 bytes).
 * @param pNewSize Required pointer to receive the plain module size.
 * @return 0 success; < 0 SCE driver error
 */
s32 sceUtilsGetLoadModuleMLength(u8 *psp, u32 pspSize, u32 *pNewSize); // sceMesgLed_driver_21AFFAAC

/**
 * Same as sceUtilsGetLoadModuleMLength() but by polling.
 * @param psp ~PSP encrypted module buffer (MUST be aligned on a 64-bytes boundary).
 * @param pspSize Size of the encrypted PRX module (MUST be >= 0x160 bytes).
 * @param pNewSize Required pointer to receive the plain module size.
 * @return 0 success; < 0 SCE driver error
 */
s32 sceUtilsGetLoadModuleMLengthByPolling(u8 *psp, u32 pspSize, u32 *pNewSize); // sceMesgLed_driver_52B6E552

/**
 * Decrypt an application module (mode 14 DECRYPT_MODE_APP_MODULE).
 * Supported tags (2g):
 * - 0x279D10F0 (type 4, key index 0x61)
 * - 0x279D08F0 (type 4, key index 0x61)
 * - 0x279D06F0 (type 4, key index 0x61)
 * - 0x279D05F0 (type 4, key index 0x61)
 * - 0xD66DF703 (type 2, key index 0x61)
 * - 0x1BC8D12B (type 1, key index 0x61, 144-bytes seed)
 * - 0x0D000000 (type 0, key index 0x50, 144-bytes seed)
 * @param psp ~PSP encrypted module buffer (MUST be aligned on a 64-bytes boundary).
 * @param pspSize Size of the encrypted PRX module (MUST be >= 0x160 bytes).
 * @param pNewSize Required pointer to receive the plain module size.
 * @return 0 success; < 0 SCE driver error
 */
s32 sceUtilsGetLoadModuleNLength(u8 *psp, u32 pspSize, u32 *pNewSize); // sceMesgLed_driver_C00DAD75

/**
 * Same as sceUtilsGetLoadModuleNLength() but by polling.
 * @param psp ~PSP encrypted module buffer (MUST be aligned on a 64-bytes boundary).
 * @param pspSize Size of the encrypted PRX module (MUST be >= 0x160 bytes).
 * @param pNewSize Required pointer to receive the plain module size.
 * @return 0 success; < 0 SCE driver error
 */
s32 sceUtilsGetLoadModuleNLengthByPolling(u8 *psp, u32 pspSize, u32 *pNewSize); // sceMesgLed_driver_F8485C9C

/**
 * Decrypt a game patch module for retail console (mode 18 DECRYPT_MODE_MS_GAME_PATCH).
 * Supported tags (2g):
 * - 0xE92410F0 (type 3, key index 0x65, two 16-bytes seeds, secure install ID)
 * - 0xE92408F0 (type 3, key index 0x65, two 16-bytes seeds, secure install ID)
 * - 0x89742B04 (type 3, key index 0x65, two 16-bytes seeds, secure install ID)
 * No longer supported tags:
 * - 0xE92405F0 (see mesg_led_02g FW 2.81)
 * @param psp ~PSP encrypted module buffer (MUST be aligned on a 64-bytes boundary).
 * @param pspSize Size of the encrypted PRX module (MUST be >= 0x160 bytes).
 * @param pNewSize Required pointer to receive the plain module size.
 * @param secureId Secure install ID (16-bytes).
 * @return 0 success; < 0 SCE driver error
 */
s32 sceUtilsGetLoadModuleRLength(u8 *psp, u32 pspSize, u32 *pNewSize, const char *secureId); // sceMesgLed_driver_CED2C075

/**
 * Decrypt a game patch module for DEVTOOL (mode 19 DECRYPT_MODE_MS_GAME_PATCH_DEVTOOL).
 * Supported tags (2g):
 * - 0x692810F0 (type 3, key index 0x66, two 16-bytes seeds, secure install ID)
 * - 0x692808F0 (type 3, key index 0x66, two 16-bytes seeds, secure install ID)
 * - 0xF5F12304 (type 3, key index 0x66, two 16-bytes seeds, secure install ID)
 * @param psp ~PSP encrypted module buffer (MUST be aligned on a 64-bytes boundary).
 * @param pspSize Size of the encrypted PRX module (MUST be >= 0x160 bytes).
 * @param pNewSize Required pointer to receive the plain module size.
 * @param secureId Secure install ID (16-bytes).
 * @return 0 success; < 0 SCE driver error
 */
s32 sceUtilsGetLoadModuleSLength(u8 *psp, u32 pspSize, u32 *pNewSize, const char *secureId); // sceMesgLed_driver_C7D1C16B

/**
 * Decrypt a POPS executable (mode 20 DECRYPT_MODE_POPS_EXEC).
 * Supported tags (2g):
 * - 0x0DAA10F0 (type 5, key index 0x65, two 16-bytes seeds, secure install ID)
 * - 0x0DAA06F0 (type 5, key index 0x65, two 16-bytes seeds, secure install ID)
 * @param psp ~PSP encrypted module buffer (MUST be aligned on a 64-bytes boundary).
 * @param pspSize Size of the encrypted PRX module (MUST be >= 0x160 bytes).
 * @param pNewSize Required pointer to receive the plain module size.
 * @param secureId Secure install ID (16-bytes).
 * @return 0 success; < 0 SCE driver error
 */
s32 sceUtilsGetLoadModuleTLength(u8 *psp, u32 pspSize, u32 *pNewSize, const char *secureId); // sceMesgLed_driver_EBB4613D

/**
 * Decrypt a (POPS DEMO?) executable (mode 21 / 0x15).
 * Supported tags (2g):
 * - 0xE1ED10F0 (type 5, key index 0x66, two 16-bytes seeds, secure install ID)
 * - 0xE1ED06F0 (type 5, key index 0x66, two 16-bytes seeds, secure install ID)
 * @param psp ~PSP encrypted module buffer (MUST be aligned on a 64-bytes boundary).
 * @param pspSize Size of the encrypted PRX module (MUST be >= 0x160 bytes).
 * @param pNewSize Required pointer to receive the plain module size.
 * @param secureId Secure install ID (16-bytes).
 * @return 0 success; < 0 SCE driver error
 */
s32 sceUtilsGetLoadModuleULength(u8 *psp, u32 pspSize, u32 *pNewSize, const char *secureId); // sceMesgLed_driver_66B348B2

/**
 * Decrypt a (application demo?) module (mode 22 / 0x16).
 * Supported tags (2g):
 * - 0x3C2A10F0 (type 2, key index 0x67)
 * - 0x3C2A08F0 (type 2, key index 0x67)
 * @param psp ~PSP encrypted module buffer (MUST be aligned on a 64-bytes boundary).
 * @param pspSize Size of the encrypted PRX module (MUST be >= 0x160 bytes).
 * @param pNewSize Required pointer to receive the plain module size.
 * @return 0 success; < 0 SCE driver error
 */
s32 sceUtilsGetLoadModuleVLength(u8 *psp, u32 pspSize, u32 *pNewSize); // sceMesgLed_driver_B2D95FDF

/**
 * Same as sceUtilsGetLoadModuleVLength() but by polling.
 * @param psp ~PSP encrypted module buffer (MUST be aligned on a 64-bytes boundary).
 * @param pspSize Size of the encrypted PRX module (MUST be >= 0x160 bytes).
 * @param pNewSize Required pointer to receive the plain module size.
 * @return 0 success; < 0 SCE driver error
 */
s32 sceUtilsGetLoadModuleVLengthByPolling(u8 *psp, u32 pspSize, u32 *pNewSize); // sceMesgLed_driver_C9ABD2F2

/**
 * Decrypt a user module (mode 23 / 0x17).
 * Supported tags (2g):
 * - 0x407810F0 (type 5, key index 0x6a, two 16-bytes seeds, secure install ID)
 * @param psp ~PSP encrypted module buffer (MUST be aligned on a 64-bytes boundary).
 * @param pspSize Size of the encrypted PRX module (MUST be >= 0x160 bytes).
 * @param pNewSize Required pointer to receive the plain module size.
 * @param secureId Secure install ID (16-bytes).
 * @return 0 success; < 0 SCE driver error
 */
s32 sceUtilsGetLoadModuleWLength(u8 *psp, u32 pspSize, u32 *pNewSize, const char *secureId); // sceMesgLed_driver_91E0A9AD

/**
 * Decrypt a game patch (PBOOT) module (mode 25 / 0x19).
 * Supported tags (2g):
 * - 0x2E5E90F0 (type 10, key index 0x48, two 16-bytes seeds, secure install ID)
 * - 0x2E5E80F0 (type 7, key index 0x48, two 16-bytes seeds, secure install ID)
 * - 0x2E5E11F0 (type 5, key index 0x48, two 16-bytes seeds, secure install ID)
 * - 0x2E5E10F0 (type 5, key index 0x48, two 16-bytes seeds, secure install ID)
 * @param psp ~PSP encrypted module buffer (MUST be aligned on a 64-bytes boundary).
 * @param pspSize Size of the encrypted PRX module (MUST be >= 0x160 bytes).
 * @param pNewSize Required pointer to receive the plain module size.
 * @param secureId Secure install ID (16-bytes).
 * @return 0 success; < 0 SCE driver error
 */
s32 sceUtilsGetLoadModuleYLength(u8 *psp, u32 pspSize, u32 *pNewSize, const char *secureId); // sceMesgLed_driver_31D6D8AA

/**
 * Decrypt a module.
 * There is no known module thats imports this function, at least in OFW 6.61 (and Go).
 * Supported tags (2g):
 * - 0x628910F0 (type 2, key index 0x47)
 * - 0x04000000 (type 0, key index 0x47, 144-bytes seed)
 * @param psp ~PSP encrypted module buffer (MUST be aligned on a 64-bytes boundary).
 * @param pspSize Size of the encrypted PRX module (MUST be >= 0x160 bytes).
 * @param pNewSize Required pointer to receive the plain module size.
 * @return 0 success; < 0 SCE driver error
 */
s32 sceResmapPrepare(u8 *psp, u32 pspSize, u32 *pNewSize); // sceResmap_driver_E5659590

/**
 * Same as sceResmapPrepare() but by polling.
 * There is no known module thats imports this function, at least in OFW 6.61 (and Go).
 * @param psp ~PSP encrypted module buffer (MUST be aligned on a 64-bytes boundary).
 * @param pspSize Size of the encrypted PRX module (MUST be >= 0x160 bytes).
 * @param pNewSize Required pointer to receive the plain module size.
 * @return 0 success; < 0 SCE driver error
 */
s32 sceResmapPrepareByPolling(u8 *psp, u32 pspSize, u32 *pNewSize); // sceResmap_driver_4434E59F

/**
 * Decrypt a module.
 * There is no known module thats imports this function, at least in OFW 6.61 (and Go).
 * Supported tags (2g):
 * - 0x8B9B10F0 (type 2, key index 0x48)
 * - 0x05000000 (type 0, key index 0x48)
 * @param psp ~PSP encrypted module buffer (MUST be aligned on a 64-bytes boundary).
 * @param pspSize Size of the encrypted PRX module (MUST be >= 0x160 bytes).
 * @param pNewSize Required pointer to receive the plain module size.
 * @return 0 success; < 0 SCE driver error
 */
s32 sceDbmanSelect(u8 *psp, u32 pspSize, u32 *pNewSize); // sceDbman_driver_B2B8C3F9

/**
 * Same as sceDbmanSelectByPolling() but by polling.
 * There is no known module thats imports this function, at least in OFW 6.61 (and Go).
 * @param psp ~PSP encrypted module buffer (MUST be aligned on a 64-bytes boundary).
 * @param pspSize Size of the encrypted PRX module (MUST be >= 0x160 bytes).
 * @param pNewSize Required pointer to receive the plain module size.
 * @return 0 success; < 0 SCE driver error
 */
s32 sceDbmanSelectByPolling(u8 *psp, u32 pspSize, u32 *pNewSize); // sceDbman_driver_34B53D46

/**
 * Decrypt a module.
 * There is no known module thats imports this function, at least in OFW 6.61 (and Go).
 * Supported tags (2g):
 * - 0x5A5C10F0 (type 2, key index 0x49)
 * - 0xE42C2303 (type 2, key index 0x49)
 * - 0x06000000 (type 0, key index 0x49, 144-bytes seed)
 * @param psp ~PSP encrypted module buffer (MUST be aligned on a 64-bytes boundary).
 * @param pspSize Size of the encrypted PRX module (MUST be >= 0x160 bytes).
 * @param pNewSize Required pointer to receive the plain module size.
 * @return 0 success; < 0 SCE driver error
 */
s32 sceNwman_driver_9555D68D(u8 *psp, u32 pspSize, u32 *pNewSize); // sceNwman_driver_9555D68D

//TODO reverse sceResmgr_driver_8E6C62C8
s32 sceResmgr_driver_8E6C62C8(void *data); // sceResmgr_driver_8E6C62C8

/**
 * Decrypt a module.
 * Supported tags (2g):
 * - 0x0B2B91F0 (type 9, key index 0x5c)
 * @param psp ~PSP encrypted module buffer (MUST be aligned on a 64-bytes boundary).
 * @param pspSize Size of the encrypted PRX module (MUST be >= 0x160 bytes).
 * @param pNewSize Required pointer to receive the plain module size.
 * @return 0 success; < 0 SCE driver error
 */
s32 sceResmgr_driver_9DC14891(u8 *psp, u32 pspSize, u32 *pNewSize); // sceResmgr_driver_9DC14891

/* Same as sceResmgr_driver_8E6C62C8() for user mode. */
s32 sceResmgr_8E6C62C8(void *data); // sceResmgr_8E6C62C8

/* Same as sceResmgr_driver_9DC14891() for user mode.
 * Imported by vsh/module/vshmain.prx module. */
s32 sceResmgr_9DC14891(u8 *psp, u32 pspSize, u32 *pNewSize); // sceResmgr_9DC14891

/**
 * Decrypt a module.
 * There is no known module thats imports this function, at least in OFW 6.61 (and Go).
 * Supported tags (2g):
 * - 0xD82310F0 (type 2, key index 0x51)
 * - 0x63BAB403 (type 2, key index 0x51)
 * - 0x0E000000 (type 0, key index 0x51, 144-bytes seed)
 * @param psp ~PSP encrypted module buffer (MUST be aligned on a 64-bytes boundary).
 * @param pspSize Size of the encrypted PRX module (MUST be >= 0x160 bytes).
 * @param pNewSize Required pointer to receive the plain module size.
 * @return 0 success; < 0 SCE driver error
 */
s32 sceMesgd_driver_102DC8AF(u8 *psp, u32 pspSize, u32 *pNewSize); // sceMesgd_driver_102DC8AF

/**
 * Same as sceMesgd_driver_102DC8AF() but by polling.
 * There is no known module thats imports this function, at least in OFW 6.61 (and Go).
 * @param psp ~PSP encrypted module buffer (MUST be aligned on a 64-bytes boundary).
 * @param pspSize Size of the encrypted PRX module (MUST be >= 0x160 bytes).
 * @param pNewSize Required pointer to receive the plain module size.
 * @return 0 success; < 0 SCE driver error
 */
s32 sceMesgd_driver_ADD0CB66(u8 *psp, u32 pspSize, u32 *pNewSize); // sceMesgd_driver_ADD0CB66

/**
 * Decrypt a module.
 * Imported by kd/me_wrapper.prx module.
 * Supported tags (2g):
 * - 0xD13B10F0 (type 2, key index 0x52)
 * - 0xD13B08F0 (type 2, key index 0x52)
 * - 0xD13B06F0 (type 2, key index 0x52)
 * - 0xD13B05F0 (type 2, key index 0x52)
 * - 0x1B11FD03 (type 2, key index 0x52)
 * - 0x862648D1 (type 1, key index 0x52, 144-bytes seed)
 * - 0x0F000000 (type 0, key index 0x52, 144-bytes seed)
 * @param psp ~PSP encrypted module buffer (MUST be aligned on a 64-bytes boundary).
 * @param pspSize Size of the encrypted PRX module (MUST be >= 0x160 bytes).
 * @param pNewSize Required pointer to receive the plain module size.
 * @return 0 success; < 0 SCE driver error
 */
s32 sceWmd_driver_7A0E484C(u8 *psp, u32 pspSize, u32 *pNewSize); // sceWmd_driver_7A0E484C

/**
 * Same as sceWmd_driver_7A0E484C() but by polling.
 * There is no known module thats imports this function, at least in OFW 6.61 (and Go).
 * @param psp ~PSP encrypted module buffer (MUST be aligned on a 64-bytes boundary).
 * @param pspSize Size of the encrypted PRX module (MUST be >= 0x160 bytes).
 * @param pNewSize Required pointer to receive the plain module size.
 * @return 0 success; < 0 SCE driver error
 */
s32 sceWmd_driver_B7CE9041(u8 *psp, u32 pspSize, u32 *pNewSize); // sceWmd_driver_B7CE9041

/**
 * Generate 20 bytes of random data.
 * This is a simple wrapper for kirk command 14.
 * There is no known module thats imports this function, at least in OFW 6.61 (and Go).
 * @param out Buffer to receive the random bytes.
 * @return 0 success; < 0 SCE driver error
 */
s32 sceDbsvrGetData(u8 out[20]); // sceDbsvr_driver_94561901

/**
 * Authenticate, encrypt, and re-sign part of the given ~PSP header.
 * For retail console.
 * psheet is the only module that calls this function.
 *
 * The following members of the ~PSP header are used:
 * aes_key, cmac_key, cmac_header_hash, cmac_data_hash, sha1_hash
 * Only the first 0x10 bytes of sha1_hash are used.
 *
 * This internally calls kirk command 2 which:
 * 1) checks the given data authenticity (CMAC check).
 * 2) encrypts the members data with a key unique to this PSP device.
 * 3) re-signs (compute new CMAC) the encrypted data.
 *
 * This function will fail when the aforementioned members data is changed.
 * There is currently no way to compute the CMAC hash of kirk 2 input data, 
 * as we don't have the CMAC private key.
 *
 * Supported tags (2g):
 * - 0xE92410F0 (type 3, key index 0x65, two 16-bytes seeds, secure install ID)
 * - 0xE92408F0 (type 3, key index 0x65, two 16-bytes seeds, secure install ID)
 * - 0x89742B04 (type 3, key index 0x65, two 16-bytes seeds, secure install ID)
 *
 * @param psp ~PSP module header structure (MUST be aligned on a 64-bytes boundary).
 * @param pspSize Size of the ~PSP header (MUST be >= 0x160 bytes).
 * @param secureId Secure install ID (16-bytes).
 * 
 * @return 0 success; 
 *         -202 invalid size; 
 *         -203 invalid address alignment; 
 *         < 0 SCE driver error
 */
s32 sceUtilsPrepareGetLoadModuleRLength(u8 *psp, u32 pspSize, const char *secureId); // sceMesgIns_driver_D062B635

/**
 * Same as sceUtilsPrepareGetLoadModuleRLength() but for DEVTOOL tags.
 * 
 * Supported tags (2g):
 * - 0x692810F0 (type 3, key index 0x66, two 16-bytes seeds, secure install ID)
 * - 0x692808F0 (type 3, key index 0x66, two 16-bytes seeds, secure install ID)
 * - 0xF5F12304 (type 3, key index 0x66, two 16-bytes seeds, secure install ID)
 * 
 * @param psp ~PSP module header structure (MUST be aligned on a 64-bytes boundary).
 * @param pspSize Size of the ~PSP header (MUST be >= 0x160 bytes).
 * @param secureId Secure install ID (16-bytes).
 * 
 * @return 0 success; 
 *         -202 invalid size; 
 *         -203 invalid address alignment; 
 *         < 0 SCE driver error
 */
s32 sceUtilsPrepareGetLoadModuleSLength(u8 *psp, u32 pspSize, const char *secureId); // sceMesgIns_driver_4A03F940

/**
 * Decrypt a module.
 * There is no known module thats imports this function, at least in OFW 6.61 (and Go).
 * Supported tags (2g):
 * - 0x2FD313F0 (type 5, key index 0x47, two 16-bytes seeds, secure install ID)
 * @param psp ~PSP encrypted module buffer (MUST be aligned on a 64-bytes boundary).
 * @param pspSize Size of the encrypted PRX module (MUST be >= 0x160 bytes).
 * @param pNewSize Required pointer to receive the plain module size.
 * @param secureId Secure install ID (16-bytes).
 * @return 0 success; < 0 SCE driver error
 */
s32 scePauth_driver_98B83B5D(u8 *psp, u32 pspSize, u32 *pNewSize, const char *secureId); // scePauth_driver_98B83B5D

/**
 * Decrypt a module.
 * There is no known module thats imports this function, at least in OFW 6.61 (and Go).
 * Supported tags (2g):
 * - 0x2FD312F0 (type 5, key index 0x47, two 16-bytes seeds, secure install ID)
 * - 0x2FD311F0 (type 5, key index 0x47, two 16-bytes seeds, secure install ID)
 * @param psp ~PSP encrypted module buffer (MUST be aligned on a 64-bytes boundary).
 * @param pspSize Size of the encrypted PRX module (MUST be >= 0x160 bytes).
 * @param pNewSize Required pointer to receive the plain module size.
 * @param secureId Secure install ID (16-bytes).
 * @return 0 success; < 0 SCE driver error
 */
s32 scePauth_driver_F7AA47F6(u8 *psp, u32 pspSize, u32 *pNewSize, const char *secureId); // scePauth_driver_F7AA47F6

/* Same as scePauth_driver_98B83B5D() / scePauth_driver_F7AA47F6() for user mode. */
s32 scePauth_98B83B5D(u8 *psp, u32 pspSize, u32 *pNewSize, const char *secureId); // scePauth_98B83B5D
s32 scePauth_F7AA47F6(u8 *psp, u32 pspSize, u32 *pNewSize, const char *secureId); // scePauth_F7AA47F6
