/* Copyright (C) 2011, 2012 The uOFW team
   See the file COPYING for copying permission.
*/

#ifndef MODULE_H
#define	MODULE_H

#include <loadcore.h>
#include "interruptController.h"
#include "loadcore_int.h"
#include "nid.h"

#define SCE_MODULE_USER_MODULE  (0x100)

#define SCE_PRIVILEGED_MODULES  (SCE_MODULE_MS | SCE_MODULE_USB_WLAN | SCE_MODULE_APP | SCE_MODULE_VSH | SCE_MODULE_KERNEL)

s32 ModuleServiceInit(void);

#endif	/* MODULE_H */

