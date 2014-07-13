# Copyright (C) 2011, 2012 The uOFW team
# See the file COPYING for copying permission.

    .set noreorder

#include "common/pspimport.s"

    IMPORT_START "semaphore",0x00010011
    IMPORT_FUNC  "semaphore",0x4C537C72,sceUtilsBufferCopyWithRange
