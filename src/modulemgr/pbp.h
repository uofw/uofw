/* Copyright (C) 2011 - 2015 The uOFW team
   See the file COPYING for copying permission.
*/

#include "common_header.h"

/**
* A .PBP file - such as EBOOT.PBP - is used to distribute PSP applications, i.e. game software.
* It contains a PSP system file (PARAM.SFO), content information files (ICON0.PNG, ICON1.PMF, PIC0.PNG,
* PIC1.PNG, SND0.AT3) and files containing executable and linking information (DATA.PSP, DATA.PSAR).
*
* For more information about the above listed files, check the following document in the uPSPD wiki: TODO
*/

/* Identifies a file as a .PBP file. */
#define PBP_MAGIC       (0x00504250) /* " PBP" */

/*
 * This structure represents the .PBP header.
 *
 * If an application does not have any specified content information files,
 * the members icon0Off to snd0Off can be filled with default values.
 */
typedef struct {
    /* PBP magic value. */
    u8 magic[4]; // 0
    /* PBP header version. */
    u32 version; // 4
    /* The PARAM.SFO file offset in bytes. */
    u32 paramOff; // 8
    /* The ICON0.PNG file offset in bytes. */
    u32 icon0Off; // 12
    /* The ICON1.PMF file offset in bytes. */
    u32 icon1Off; // 16
    /* The PIC0.PNG file offset in bytes. */
    u32 pic0Off; // 20
    /* The PIC1.PNG file offset in bytes. */
    u32 pic1Off; // 24
    /* The SND0.AT3 file offset in bytes. */
    u32 snd0Off; // 28
    /* The DATA.PSP file offset in bytes. */
    u32 dataPSPOff; // 32
    /* The DATA.PSAR file offset in bytes. */
    u32 dataPSAROff; // 36
} PBPHeader;

