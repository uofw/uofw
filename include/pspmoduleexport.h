/* Copyright (C) 2011, 2012 The uOFW team
   See the file COPYING for copying permission.
*/

#ifndef PSPMODULEEXPORT_H
#define PSPMODULEEXPORT_H

/** Structure to hold a single export entry */
struct SceLibraryEntry {
    const char *    name;
    unsigned short  version;
    unsigned short  attribute;
    unsigned char   entLen;
    unsigned char   varCount;
    unsigned short  funcCount;
    const void *    entrytable;
};

/**
 * An export entry of 5 words (entLen 5), used by the original modules for their large libraries. Bits 16-19 of
 * extra (k) give the size of the NID index following the export table: 2^(k+2) halfwords, entry i being the
 * number of exported functions whose NID has its top k+2 bits below i.
 */
typedef struct {
    struct SceLibraryEntry entry;
    unsigned int    extra;
} SceLibraryEntryExtra;

#endif

