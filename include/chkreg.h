/* Copyright (C) The uOFW team
   See the file COPYING for copying permission.
*/

#ifndef CHKREG_H
#define CHKREG_H

#include "common_header.h"

/**
 * This structure contains console specific information. It is a subset of the ::SceConsoleId.
 * Check <openpsid_kernel.h> for possible member values.
 */
typedef struct {
	/* Company code. Set to 1. */
	u16 companyCode; // 0
	/* Product code. */
	u16 productCode; // 2
	/* Product sub code. */
	u16 productSubCode; // 4
	/* Factory code. */
	u16 factoryCode; // 6
} ScePsCode; // size = 8

/**
 * Gets the PSP system's Ps code.
 * 
 * @param pPsCode Pointer to a ScePsCode variable which is to receive the Ps code. 
 * 
 * @return 0 on success, otherwise < 0.
 */
s32 sceChkregGetPsCode(ScePsCode *pPsCode);

#define SCE_CHKREG_UMD_MEDIA_TYPE_GAME     0x00
#define SCE_CHKREG_UMD_MEDIA_TYPE_VIDEO    0x20
#define SCE_CHKREG_UMD_MEDIA_TYPE_AUDIO    0x40

#define SCE_PSP_REGION_JAPAN                        0x0
#define SCE_PSP_REGION_NORTH_AMERICA                0x1
#define SCE_PSP_REGION_EUROPE_MIDDLE_EAST_AFRICA    0x2
#define SCE_PSP_REGION_KOREA                        0x3
#define SCE_PSP_REGION_UK_IRELAND                   0x4
#define SCE_PSP_REGION_MEXICO                       0x5
#define SCE_PSP_REGION_AUSTRALIA_NEW_ZEALAND        0x6
#define SCE_PSP_REGION_HONGKONG_SINGAPORE           0x7
#define SCE_PSP_REGION_TAIWAN                       0x8
#define SCE_PSP_REGION_RUSSIA                       0x9
#define SCE_PSP_REGION_CHINA                        0xA
#define SCE_PSP_REGION_UNKNOWN_15                   0xF /* Perhaps all regions to test?*/

/**
 * Checks if the UMD with the given media type can be started for the specified region on the PSP system.
 * 
 * @param umdMediaType The type of the UMD medium to check for. Typically GAME, VIDEO or AUDIO.
 * @param regionId The region to check for.
 * 
 * @return SCE_TRUE if the UMD with the given media type and the specified region can be played on the PSP system,
 * SCE_FALSE otherwise.
 * @return < on error.
 */
s32 sceChkregCheckRegion(u32 umdMediaType, u32 regionId);

s32 sceChkreg_driver_9C6E1D34(const u8 *arg0, u8 *arg1);

/* QA flag. */
#define SCE_CHKREG_PS_FLAGS_QAF      0x00000001
#define SCE_CHKREG_PS_FLAGS_UNK2F    0x00000002

#define SCE_CHKREG_PS_FLAGS_INDEX_DEFAULT    0

/**
 * @brief Gets the PSP system's Ps flags. 
 * 
 * The Ps flags can be used to check for special hardware configurations like whether the QA flag is set. If, for example,
 * the QA flag is set, this will enable a special menu point in the XMB called "Debug Settings" which can be used to change
 * PSP system parameters (such as the available RAM (32MB/64MB), the WLAN coexistency max clock frequency or whether the
 * Cross or Circle button acts as the [enter] button).
 * 
 * The Ps flags can only be successfully obtained on certain PSP systems (such as test/development hardware). On retail
 * PSP systems, an error is returned.
 * 
 * @param pPsFlags Pointer to an u8 variable which is to receive the Ps flags.
 * @param index Specify ::SCE_CHKREG_PS_FLAGS_INDEX_DEFAULT.
 * 
 * @return 0 on success.
 * @return SCE_ERROR_INVALID_VALUE The PSP system is not allowed to obtain its Ps flags. This is typically the case for
 * retail PSP systems.
 * @return < 0 on error.
 */
s32 sceChkregGetPsFlags(u8 *pPsFlags, s32 index);

/** The PSP model belongs to a PSP series which could not be identified. */
#define SCE_CHKREG_PSP_MODEL_UNKNOWN_SERIES        0
/** The PSP model belongs to the PSP-1000 series. */
#define SCE_CHKREG_PSP_MODEL_1000_SERIES           1
/** The PSP model belongs to the PSP-2000 series. */
#define SCE_CHKREG_PSP_MODEL_2000_SERIES           2
/** The PSP model belongs to the PSP-3000 series. */
#define SCE_CHKREG_PSP_MODEL_3000_SERIES           3
/** The PSP model belongs to either the PSP-N1000 series or the PSP-E1000 series. */
#define SCE_CHKREG_PSP_MODEL_N1000_E1000_SERIES    5

/**
 * Gets the PSP series this PSP model belongs to.
 *
 * @return The PSP series on success, otherwise < 0.
 */
s32 sceChkregGetPspModel(void);

#endif // CHKREG_H
