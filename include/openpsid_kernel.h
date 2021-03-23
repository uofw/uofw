/* Copyright (C) The uOFW team
   See the file COPYING for copying permission.
*/

#ifndef OPENPSID_KERNEL_H
#define OPENPSID_KERNEL_H

#include "common_header.h"

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
	u8 psFlagsMajor : 2; // 8
	/* Factory code. */
	u8 factoryCode : 6; // 8
	u8 uniqueIdMajor : 2; // 9
	u8 psFlagsMinor : 6; // 9
	u8 uniqueIdMinor[6]; // 10
} SceConsoleId; // size = 16

#endif // OPENPSID_KERNEL_H
