/* Copyright (C) 2011, 2012 The uOFW team
   See the file COPYING for copying permission.
*/

#include <pspidstorage.h>
#include <pspsdk.h>

#include "firmware/magpie.c"
#include "firmware/magpie_helper.c"

s32 module_start()
{
	u16 key45;

	// sceIdStorage_driver_6FE062D1
	sceIdStorageLookup(0x45, 0, &key45, sizeof(u16));

	if((key45 & 0xF000) != 0)
	{
	    return 1;
	}

	sceWlanDrv_driver_1747351B(
		wlanfirmHelper, // 0x14C
		wlanfirmHelperSize, // 1852
		wlanfirm, // 0x888
		wlanfirmSize, // 87168
	);

	return 0;
}
