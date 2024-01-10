/* Copyright (C) 2011, 2012 The uOFW team
   See the file COPYING for copying permission.
*/

/*
 * uOFW/trunk/src/loadcore/nid.h
 * 
 * Name Identifier - all NID values used by Loadcore.  NIDs are
 * based on SDK version 6.60.
 */

#ifndef NID_H
#define	NID_H

#ifdef	__cplusplus
extern "C" {
#endif

/* LoadCore exported function NIDs. */
#define NID_SCE_KERNEL_DELETE_MODULE                        0x001B57BB
#define NID_SCE_KERNEL_UNLINK_LIBRARY_ENTRIES               0x0295CFCE
#define NID_SCE_KERNEL_MASK_LIBRARY_ENTRIES                 0x1915737F
#define NID_SCE_KERNEL_LOAD_CORE_LOCK                       0x1999032F
#define NID_SCE_KERNEL_LOAD_EXECUTABLE_OBJECT               0x1C394885
#define NID_SCE_KERNEL_CREATE_MODULE                        0x2C44F793
#define NID_SCE_KERNEL_REGISTER_LIBRARY_FOR_USER            0x2C60CCB8
#define NID_SCE_KERNEL_GET_MODULE_ID_LIST_FOR_KERNEL        0x37E6F41B
#define NID_SCE_KERNEL_GET_MODULE_LIST_WITH_ALLOC           0x3FE631F0
#define NID_SCE_KERNEL_FIND_MODULE_BY_UID                   0x40972E6E
#define NID_SCE_KERNEL_GET_MODULE_GP_BY_ADDRESS_FOR_KERNEL  0x410084F9
#define NID_SCE_KERNEL_PROBE_EXECUTABLE_OBJECT              0x41D10899
#define NID_SCE_KERNEL_REGISTER_LIBRARY                     0x48AF96A9
#define NID_SCE_KERNEL_LOAD_MODULE_BOOT_LOAD_CORE           0x493EE781
#define NID_SCE_KERNEL_CAN_RELEASE_LIBRARY                  0x538129F8
#define NID_SCE_KERNEL_SEGMENT_CHECKSUM                     0x5FDDB07A
#define NID_SCE_KERNEL_QUERY_LOAD_CORE_CB                   0x696594C8
#define NID_SCE_KERNEL_LINK_LIBRARY_ENTRIES_FOR_USER        0x6ECFFFBA
#define NID_SCE_KERNEL_CREATE_ASSIGN_MODULE                 0x84D5C971
#define NID_SCE_KERNEL_LINK_LIBRARY_ENTRIES                 0x8EAE9534
#define NID_SCE_KERNEL_LINK_LIBRARY_ENTRIES_WITH_MODULE     0xA481E30E
#define NID_SCE_KERNEL_RELEASE_MODULE                       0xB17F5075
#define NID_SCE_KERNEL_LOAD_REBOOT_BIN                      0xB27CC244
#define NID_SCE_KERNEL_LOAD_CORE_UNLOCK                     0xB6C037EA
#define NID_SCE_KERNEL_FIND_MODULE_BY_ADDRESS               0xBC99C625
#define NID_SCE_KERNEL_REGISTER_MODULE                      0xBF2E388C
#define NID_SCE_KERNEL_LOAD_CORE_MODE                       0xC8FF5EE5
#define NID_SCE_KERNEL_RELEASE_LIBRARY                      0xCB636A90
#define NID_SCE_KERNEL_GET_MODULE_FROM_UID                  0xCD26E0CA
#define NID_SCE_KERNEL_CHECK_EXEC_FILE                      0xD3353EC4
#define NID_SCE_KERNEL_ASSIGN_MODULE                        0xF3DD4808
#define NID_SCE_KERNEL_FIND_MODULE_BY_NAME                  0xF6B1BF0F
#define NID_SCE_KERNEL_SET_BOOT_CALLBACK_LEVEL              0xF976EF41
#define NID_SCE_KERNEL_CHECK_PSP_CONFIG                     0xFC47F93A

#ifdef	__cplusplus
}
#endif

#endif	/* NID_H */

