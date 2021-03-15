/** Copyright (C) The uOFW team
   See the file COPYING for copying permission.
*/

#include "common_header.h"

#ifndef CHKREG_H
#define CHKREG_H

typedef struct {
	u16 companyCode; // {0, 1}
	u16 productCode;
	u16 productSubCode;
	u16 factoryCode; // = chassis_check >> 2;
} ScePsCode;

s32 sceChkregGetPsCode(u8 *code);

s32 sceChkregCheckRegion(u32 arg0, u32 arg1);

s32 sceChkreg_driver_6894A027(u8 *arg0, s32 arg1);

s32 sceChkreg_driver_7939C851(void);

s32 sceChkreg_driver_9C6E1D34(u8 *arg0, u8 *arg1);

#endif // CHKREG_H
