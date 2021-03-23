/* Copyright (C) The uOFW team
   See the file COPYING for copying permission.
*/

#ifndef CHKREG_H
#define CHKREG_H

#include "common_header.h"

/**
 * This structure contains console specific information. It is a subset of the the ::SceConsoleId.
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

s32 sceChkregGetPsCode(ScePsCode *pPsCode);

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
#define SCE_PSP_REGION_UNKNOWN_15                   0xF

s32 sceChkregCheckRegion(u32 umdMediaType, u32 regionId);

s32 sceChkreg_driver_9C6E1D34(const u8 *arg0, u8 *arg1);

/* QA flag */
#define SCE_CHKREG_PS_FLAGS_QAF      0x00000001
#define SCE_CHKREG_PS_FLAGS_UNK2F    0x00000002

#define SCE_CHKREG_PS_FLAGS_INDEX_DEFAULT    0

s32 sceChkregGetPsFlags(u8 *pPsFlags, s32 index);

#define SCE_CHKREG_PSP_MODEL_UNKNOWN_SERIES        0
#define SCE_CHKREG_PSP_MODEL_1000_SERIES           1
#define SCE_CHKREG_PSP_MODEL_2000_SERIES           2
#define SCE_CHKREG_PSP_MODEL_3000_SERIES           3
#define SCE_CHKREG_PSP_MODEL_N1000_E1000_SERIES    5

s32 sceChkregGetPspModel(void);

#endif // CHKREG_H
