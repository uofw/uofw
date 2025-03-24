/* Copyright (C) The uOFW team
   See the file COPYING for copying permission.
*/

#ifndef OPENPSID_KERNEL_H
#define OPENPSID_KERNEL_H

#include "common_header.h"

/* Values (PSP product code/sub code, SceConsoleId, ScePsCode) taken from: https://github.com/CelesteBlue-dev/PS-ConsoleId-wiki/blob/master/PS-ConsoleId-wiki.txt */

#define SCE_PSP_PRODUCT_CODE_TEST_PROTOTYPE_TEST_UNIT         0x00 /* Prototype / Test unit */
#define SCE_PSP_PRODUCT_CODE_TOOL_DEVKIT_TOOL_UNIT            0x01 /* Devkit / Development Tool - DEM-1000 & test unit DTP-T1000 */
#define SCE_PSP_PRODUCT_CODE_DEX_TEST_KIT                     0x02 /* TestKit / Testing Kit - Testing Tool DTP-H1500 */
#define SCE_PSP_PRODUCT_CODE_CEX_JAPAN                        0x03 /* Retail Japan */
#define SCE_PSP_PRODUCT_CODE_CEX_NORTH_AMERICA                0x04 /* Retail North America */
#define SCE_PSP_PRODUCT_CODE_CEX_EUROPE_MIDDLE_EAST_AFRICA    0x05 /* Retail Europe/Middle East/Africa */
#define SCE_PSP_PRODUCT_CODE_CEX_KOREA                        0x06 /* Retail South Korea */
#define SCE_PSP_PRODUCT_CODE_CEX_UNITED_KINGDOM               0x07 /* Retail Great Britain/United Kingdom */
#define SCE_PSP_PRODUCT_CODE_CEX_MEXICO_LATIN_AMERICA         0x08 /* Retail Mexico/Latin America */
#define SCE_PSP_PRODUCT_CODE_CEX_AUSTRALIA_NEW_ZEALAND        0x09 /* Retail Australia/New Zealand */
#define SCE_PSP_PRODUCT_CODE_CEX_HONGKONG_SINGAPORE           0x0A /* Retail Hong Kong/Singapore */
#define SCE_PSP_PRODUCT_CODE_CEX_TAIWAN                       0x0B /* Retail Taiwan */
#define SCE_PSP_PRODUCT_CODE_CEX_RUSSIA                       0x0C /* Retail Russia */
#define SCE_PSP_PRODUCT_CODE_CEX_CHINA                        0x0D /* Retail China */

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
 * Specifies if the PSP's factory code is set to diagnosis (i.e. Japan Diagnosis Center 1).
 * In this case, the PsFlags ca be obtained by using the CHKREG module.
 */
#define SCE_PSP_FACTORY_CODE_DIAG    35

/**
 * This structure represents a unique per-console identifier. It contains console specific information and can be used,
 * for example, for DRM purposes and simple PSP hardware model checks.
 *
 * @remark On the PSP, Sony uses the term "PSID" (not to mixup with the term "OpenPSID" which represents a different set of
 * unique identifier bits). On later consoles, like the PS Vita and PS4, Sony uses the term "ConsoleId" for this set of
 * identifier bits. To be consistent within the PS family, we are going with the term "ConsoleId" here, even though APIs like
 * sceOpenPSIDGetPSID() (which returns the ConsoleId) will remain as originally named by Sony.
 */
typedef struct {
	/* Unknown. On retail set to 0. */
	u16 unk0; // 0
	/* Company code. Set to 1. */
	u16 companyCode; // 2
	/* Product code. */
	u16 productCode; // 4
	/* Product sub code. */
	u16 productSubCode; // 6
	/* Upper two bit of PsFlags. */
	u8 psFlagsMajor : 2; // 8
	/* Factory code. */
	u8 factoryCode : 6; // 8
	u8 serialNoMajor : 2; // 9
	/* Lower six bit of the PsFlags. Contain the QA flag, if set. */
	u8 psFlagsMinor : 6; // 9
	u16 searialNoMinor; // 10
	u32 randomStamp; // 12
} SceConsoleId; // size = 16

#endif // OPENPSID_KERNEL_H
