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

/* 
 * This structure represents a unique per-console identifier. This is also named "PSID" on the PSP
 * (not to mixup with the term "OpenPSID" which also used for a different set of identifier bytes).
 */
typedef struct {
	/* Unknown. On retail set to 0. */
	u16 unk0; // 0
	/* Company code. Set to 1. */
	u16 companyCcode; // 2
	/* Product code. */
	u16 productCode; // 4
	/* Product sub code. */
	u16 productSubCode; // 6
	/* Chassis check. */
	u8 chassisCheck; // 8
	u8 unk9[7]; // 9
} SceConsoleId; // size = 16

typedef struct {
	/* Company code. Set to 1. */
	u16 companyCode; // 0
	/* Product code. */
	u16 productCode; // 2
	/* Product sub code. */
	u16 productSubCode; // 4
	u16 factoryCode; // 6
} ScePsCode; // size = 8

s32 sceChkregGetPsCode(ScePsCode *pPsCode);

s32 sceChkregCheckRegion(u32 arg0, u32 arg1);

s32 sceChkreg_driver_6894A027(u8 *arg0, s32 arg1);

s32 sceChkreg_driver_7939C851(void);

s32 sceChkreg_driver_9C6E1D34(u8 *arg0, u8 *arg1);

#endif // CHKREG_H
