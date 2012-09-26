/* Copyright (C) 2011, 2012 The uOFW team
   See the file COPYING for copying permission.
*/

#include "common_header.h"
#include <loadcore.h>

#ifndef SYSTABLE_H
#define	SYSTABLE_H

/*
 * This structure represents a system call table entry.  It records
 * several important information about such an entry.
 */
typedef struct SceSysCallEntryTable {
    /* Pointer to the next entry table. */
   struct SceSysCallEntryTable *next; //0
   /* Indicates whether the entry is in use or not. */
   u16 inUse; //4
   /* The number of exported functions belonging to that entry. */
   u16 numEntries; //6
   /* The location of the first exported function in the "syscalls" 
    * array of the system call table this entry belongs to. 
    */
   u32 startAddr; // 8
} SceSysCallEntryTable;

s32 SyscallTableInit(u32 seed, SceSyscallTable **syscallTable);

s32 AllocSysTable(u16 numEntries);

SceSysCallEntryTable *FreeSysTable(u32 sysTableEntryAddr);

s32 UndefSyscall(void);


#endif	/* SYSTABLE_H */

