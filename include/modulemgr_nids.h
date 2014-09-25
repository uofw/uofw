/* Copyright (C) 2011 - 2014 The uOFW team
   See the file COPYING for copying permission.
*/

#ifndef MODULEMGR_NIDS_H
#define	MODULEMGR_NIDS_H

#ifdef	__cplusplus
extern "C" {
#endif

#define NID_MODULE_BOOTSTART                                0xD3744BE0
#define NID_MODULE_REBOOT_PHASE                             0xADF12745
#define NID_MODULE_REBOOT_BEFORE                            0x2F064FA6
#define NID_MODULE_REBOOT_BEFORE_THREAD_PARAM               0xF4F4299D
#define NID_MODULE_START                                    0xD632ACDB
#define NID_MODULE_START_THREAD_PARAM                       0x0F7C276C
#define NID_MODULE_STOP                                     0xCEE8593C
#define NID_MODULE_STOP_THREAD_PARAM                        0xCF0CC697
#define NID_MODULE_INFO                                     0xF01D73A7
#define NID_MODULE_SDK_VERSION                              0x11B97506


#ifdef	__cplusplus
}
#endif

#endif	/* MODULEMGR_NIDS_H */

