# Copyright (C) 2011, 2012 The uOFW team
# See the file COPYING for copying permission.

    .set noreorder

#include "common/pspimport.s"

    IMPORT_START "sceChkreg_driver",0x00010011
    IMPORT_FUNC  "sceChkreg_driver", 0x59F8491D, sceChkregGetPsCode
	
	IMPORT_START "SysclibForKernel",0x00010011
	IMPORT_FUNC  "SysclibForKernel", 0xB49A7697, strncpy