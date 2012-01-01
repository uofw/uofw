/* Copyright (C) 2011, 2012 The uOFW team
   See the file COPYING for copying permission.
*/

#include <pspidstorage.h>
#include <pspsdk.h>

int g_14C = 0xEA000003; // TODO: unsure
int g_888 = 1;

void module_start()
{
	u16 var;
	sceIdStorageLookup(0x45,0,&var,2);
	if((var[0] & 0xF000) != 0)
	{
	    return 1;
	}
	sceWlanDrv_driver_1747351B(&g_14C,0x73C,&g_888,0x15480);
	return 0;
}
