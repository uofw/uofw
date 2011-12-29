/* Copyright (C) 2011 The uOFW team
   See the file COPYING for copying permission.
*/

#ifndef PSPMODULEEXPORT_H
#define PSPMODULEEXPORT_H

/** Structure to hold a single export entry */
struct _PspLibraryEntry {
    const char *    name;
    unsigned short  version;
    unsigned short  attribute;
    unsigned char   entLen;
    unsigned char   varCount;
    unsigned short  funcCount;
    void *          entrytable;
};

#endif

