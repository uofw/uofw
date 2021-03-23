/* Copyright (C) The uOFW team
   See the file COPYING for copying permission.
*/

#ifndef CHKREG_H
#define CHKREG_H

#include "common_header.h"

/* Values (PSP product code/sub code, SceConsoleId, ScePsCode) taken from: https://github.com/CelesteBlue-dev/PS-ConsoleId-wiki/blob/master/PS-ConsoleId-wiki.txt */

#define SCE_PSP_PRODUCT_CODE_TEST_PROTOTYPE_TEST_UNIT    0x00 /* Not in use. */
#define SCE_PSP_PRODUCT_CODE_TOOL_DEVKIT_TOOL_UNIT       0x01 /* Development Tool DEM-1000 & test unit DTP-T1000 */
#define SCE_PSP_PRODUCT_CODE_DEX_TEST_KIT                0x02 /* Testing Tool DTP-H1500 */
#define SCE_PRODUCT_CODE_CEX_JAPAN                       0x03 /* Retail Japan */
#define SCE_PRODUCT_CODE_CEX_NORTH_AMERICA               0x04 /* Retail North America */
#define SCE_PRODUCT_CODE_CEX_EUROPE_MIDDLE_EAST_AFRIKA   0x05 /* Retail Europe/Middle East/Afrika */
#define SCE_PRODUCT_CODE_CEX_KOREA                       0x06 /* Retail South Korea */
#define SCE_PRODUCT_CODE_CEX_UNITED_KINGDOM              0x07 /* Retail Great Britain/United Kingdom */
#define SCE_PRODUCT_CODE_CEX_MEXIKO_LATIN_AMERICA        0x08 /* Retail Mexiko/Latin America */
#define SCE_PRODUCT_CODE_CEX_AUSTRALIA_NEW_ZEALAND       0x09 /* Retail Australia/New Zealand */
#define SCE_PRODUCT_CODE_CEX_HONGKONG_SINGAPORE          0x0A /* Retail Hong Kong/Singapore */
#define SCE_PRODUCT_CODE_CEX_TAIWAN                      0x0B /* Retail Taiwan */
#define SCE_PRODUCT_CODE_CEX_RUSSIA                      0x0C /* Retail Russia */
#define SCE_PRODUCT_CODE_CEX_CHINA                       0x0D /* Retail China */

#define SCE_PSP_PRODUCT_SUB_CODE_TA_079_TA_081    0x01 /* PSP-10XX 01g */
#define SCE_PSP_PRODUCT_SUB_CODE_TA_082_TA_086    0x02 /* PSP-10XX 01g */
#define SCE_PSP_PRODUCT_SUB_CODE_TA_085_TA_088    0x03 /* PSP-20XX 02g */
#define SCE_PSP_PRODUCT_SUB_CODE_TA_090_TA_092    0x04 /* PSP-30XX 03g */
#define SCE_PSP_PRODUCT_SUB_CODE_TA_091           0x05 /* PSP-N10XX 05g */
#define SCE_PSP_PRODUCT_SUB_CODE_TA_093           0x06 /* PSP-30XX 04g */
#define SCE_PSP_PRODUCT_SUB_CODE_TA_094           0x07 /* PSP-N10XX 05g - prototype only */
#define SCE_PSP_PRODUCT_SUB_CODE_TA_095           0x08 /* PSP-30XX 07g & 09g */
#define SCE_PSP_PRODUCT_SUB_CODE_TA_096_TA_097    0x09 /* PSP-E10XX 11g */

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
