/* Copyright (C) 2011, 2012 The uOFW team
   See the file COPYING for copying permission.
*/

/*
 * uOFW/trunk/src/loadcore/systable.c
 * 
 * System Call table
 * 
 * A system call table is used to hold the exported functions provided 
 * by resident libraries.  These exported functions have to be exported 
 * by a SCE_LIB_SYSCALL_EXPORT resident library; in other words,
 * a library residing in kernel memory and exporting to stub libraries
 * living in user land. 
 * 
 * Beside filling the system call table with pointers to its exported 
 * functions, a resident library also requests a SceSysCallEntryTable
 * object used to hold the numbers of exported functions and the index into
 * the system call table.  Furthermore, this object is used to set/remove 
 * a protection mask on un-imported system calls.
 * 
 */

#include <interruptman.h>
#include "loadcore_int.h"
#include "systable.h"

#define SYSCALL_TABLE_FUNCTION_TABLE_SIZE       (SYSCALL_TABLE_FUNCTION_TABLE_ENTRIES * sizeof(u32))
#define SYSCALL_TABLE_SIZE                      (SYSCALL_TABLE_FUNCTION_TABLE_SIZE + 16)

#define SYSCALL_TABLE_FUNCTION_TABLE_ENTRIES    (4096)

#define SYSCALL_TABLE_DEFAULT_ENTRIES           (4096)

#define SYSCALL_ENTRY_TABLES                    (60)
#define TOP_SYSCALL_ENTRY_TABLE                 SYSCALL_ENTRY_TABLES - 1

/* 
 * Pointer to the next free SyscallEntryTable object in the
 * initialSysEntTable linked list.  A free SyscallEntryTable object
 * is a table which is not in use and which is not 
 * dependent on a used table, that means, it wasn't allocated by
 * splitting a free block.
 */
static SceSysCallEntryTable *g_pFreeSysEnt; //0x000080E8
/* 
 * Pointer to the top initialSysEntTable linked list object.  It is
 * used to set an entry point in the list when scrolling
 * through it. 
 */
static SceSysCallEntryTable *g_pSysEntControl; // 0x000080EC
/*
 * The linked list with SYSCALL_ENTRY_TABLES SceSysCallEntryTable
 * objects.  Its purpose is to keep track of the exported (system call) 
 * functions of a resident library and whether these functions are 
 * already imported (linked).  When registering a resident library to 
 * the system, these functions are bitwise AND'ed with 0x7FFFFFFF to 
 * prevent that they are called without being imported.  Kernel memory 
 * starts at 0x88000000, thus we mask the addresses so it appears that 
 * they do not live in kernel memory.
 *  
 * Now, if a stub library is linked with a resident library, the specific 
 * list object for that resident library is used to get the addresses
 * of its exported functions.  They are bitwise OR'ed them with 0x80000000 
 * in order to restore the real locations of the functions in kernel memory 
 * again.  This way, they can be called without a problem.
 */
static SceSysCallEntryTable initialSysEntTable[SYSCALL_ENTRY_TABLES]; //0x000080F0

//sub_00000344
/*
 * Initialize the System Call table.  It is initialized the very
 * first time a SCE_LIB_SYSCALL_EXPORT resident library is 
 * registered to the system.  The table makes room for
 * SYSCALL_TABLE_FUNCTION_TABLE_ENTRIES functions to be stored
 * in it.  These function pointer slots are initialized to the 
 * address of the function "UndefSyscall", which simply returns an 
 * error, marking a slot as currently being unused.
 * 
 * In addition, the internal SyscallEntryTable stack is initialized
 * and g_pSysEntControl is set to the top member of it.
 * 
 * @param seed Used to set the start address of the system call table.
 * @param lcSyscallTable Receive a pointer to the allocated system
 *                       call table.
 * 
 * @return 0 on success.
 */
s32 SyscallTableInit(u32 seed, SceSyscallTable **syscallTable) 
{
   u32 i;
   s32 status;
   SceSyscallTable *sysCallTable;
     
   sysCallTable = sceKernelAllocHeapMemory(g_loadCoreHeap(), SYSCALL_TABLE_SIZE); 
   if (sysCallTable == NULL)
       return SCE_ERROR_KERNEL_ERROR;

   sysCallTable->next = NULL;
   sysCallTable->seed = seed << 2;
   sysCallTable->funcTableSize = SYSCALL_TABLE_FUNCTION_TABLE_SIZE;
   sysCallTable->tableSize = SYSCALL_TABLE_SIZE;
   
   //0x000003B0 - 0x000003C8
   for (i = 0; i < SYSCALL_TABLE_FUNCTION_TABLE_ENTRIES; i++) 
        sysCallTable->syscalls[i] = (void (*)())UndefSyscall;
    
   //0x000003D0 - 0x000003F8
   initialSysEntTable[0].next = NULL;      
   for (i = 0; i < TOP_SYSCALL_ENTRY_TABLE; i++)
        initialSysEntTable[i + 1].next = &initialSysEntTable[i];
   
   /* 
    * Set g_pFreeSysEnt to point to the top object of the 
    * SYSCALL Entry table stack. 
    */
   g_pFreeSysEnt = &initialSysEntTable[TOP_SYSCALL_ENTRY_TABLE]; //0x0000040C 
   if (g_pFreeSysEnt == NULL) { //0x00000410
       g_pSysEntControl = (SceSysCallEntryTable *)sceKernelAllocHeapMemory(g_loadCoreHeap(), sizeof(SceSysCallEntryTable)); //0x00000498 & 0x000004A4
   }
   else {
       g_pSysEntControl = g_pFreeSysEnt; //0x00000420
       g_pFreeSysEnt = g_pFreeSysEnt->next; //0x0000041C
   }
    
   //0x00000424 - 0x00000440
   /* Initialize the first SYSCALL table entry. */
   g_pSysEntControl->next = NULL; 
   g_pSysEntControl->inUse = FALSE;
   g_pSysEntControl->numEntries = SYSCALL_TABLE_DEFAULT_ENTRIES;
   g_pSysEntControl->startAddr = 0;
   
   status = sceKernelRegisterSystemCallTable(sysCallTable); //0x0000043C
   if (status != SCE_ERROR_OK) { //0x00000444
       sceKernelFreeHeapMemory(g_loadCoreHeap(), sysCallTable); //0x00000478 & 0x00000484
       return status;
   }
   *syscallTable = sysCallTable;
   return SCE_ERROR_OK; 
}

//sub_000004B4
/*
 * Allocate a system-call-entry-table for a resident library
 * exporting its functions via the SYSCALL_EXPORT technique.
 * In order to allocate such a table, we start at the object 
 * pointed to by g_pSysEntControl (normally the top member of 
 * the initialSysEntTable stack).  We scroll through that data 
 * structure until we find an object which is not already 
 * being used and which is big enough to hold the amount of 
 * exported functions.  
 * 
 * @param numEntry The amount of exported functions of a
 *                 resident library.
 * 
 * Returns 0 on success.
 *   
 */
s32 AllocSysTable(u16 numEntries)
{
    SceSysCallEntryTable *sysTableEntry = NULL;
    SceSysCallEntryTable *newSysTableEntry = NULL;
    
    for (sysTableEntry = g_pSysEntControl; sysTableEntry; sysTableEntry = sysTableEntry->next) {
         if (sysTableEntry->inUse)
             continue;
         
         /*
          * Here, the concept "first fit" is used.  We scan the list 
          * for an unused sysEntryTable object big-enough to hold the 
          * amount of exported functions.  If the object is exactly
          * the size requested, its start address will be returned
          * and marked as being in-use.
          * 
          * If the object can hold more than the requested numEntries, 
          * split into two new blocks.  The first block holds the exact 
          * number of requested entries and its start address will be returned. 
          * The second block will be unlinked from the free entry table stack.
          * It holds the remaining entries from the original block ready to be 
          * used by another request. 
          */ 
         if (sysTableEntry->numEntries == numEntries) { //0x000004F4
             sysTableEntry->inUse = TRUE;
             return sysTableEntry->startAddr;
         }
         /* 
          * Example: We want to allocate a new SysEntryTable object capable
          *          of holding 60 exported functions.
          * 
          * State of the list before the object is allocated.
          * 
          * +------------+   +------------+
          * |   A = 100  |-->|   B = 150  |-->...
          * +------------+   +------------+
          */
         if (sysTableEntry->numEntries > numEntries) { //0x000004FC
             if (g_pFreeSysEnt == NULL) {
                 newSysTableEntry = (SceSysCallEntryTable *)sceKernelAllocHeapMemory(g_loadCoreHeap(), sizeof(SceSysCallEntryTable));
             }
             else {
                 newSysTableEntry = g_pFreeSysEnt; //0x00000544
                 g_pFreeSysEnt = g_pFreeSysEnt->next;
             }
             newSysTableEntry->inUse = FALSE; //0x00000550
             newSysTableEntry->startAddr = sysTableEntry->startAddr + numEntries; //0x0000055C
             newSysTableEntry->numEntries = sysTableEntry->numEntries - numEntries;
             newSysTableEntry->next = sysTableEntry->next;
             
             sysTableEntry->inUse = TRUE; //0x00000578
             sysTableEntry->numEntries = numEntries;
             sysTableEntry->next = newSysTableEntry;
             
             /*
              * State of the list after the object is allocated.
              * 
              * +------------+   +------------+   +------------+
              * |###A = 60###|-->|   A' = 40  |-->|   B = 150  |-->...
              * +------------+   +------------+   +------------+
              */ 
             
             return sysTableEntry->startAddr;
         }
    }
    return SCE_ERROR_KERNEL_ERROR;
}

//sub_000005BC
/*
 * FreeSysTable
 * 
 * Free a system-call-entry-table of a resident library.  In order
 * to free such a table, we start at the object pointed to by 
 * g_pSysEntControl (normally the top member of the initialSysEntTable 
 * stack).  We scroll through that data structure until the location 
 * of the to-be-freed table is found and integrate it properly into 
 * the list again, merging it with another free neighbor table if
 * needed.
 * 
 * @param sysTableEntryAddr The address of an allocated SceSysCallEntryTable 
 *                          object.
 * 
 * Returns the address of the freed table on success.
 */
SceSysCallEntryTable *FreeSysTable(u32 sysTableEntryAddr) 
{
    SceSysCallEntryTable *curSysTableEntry = NULL;
    SceSysCallEntryTable *prevSysTableEntry = NULL;
    SceSysCallEntryTable *nextSysTableEntry = NULL;
    
    if (g_pSysEntControl == NULL) //0x000005D4
        return NULL;

    //0x000005DC - 0x000005F4
    /* Search for the location of the to-be-freed table. */
    for (curSysTableEntry = g_pSysEntControl; curSysTableEntry != NULL && 
       curSysTableEntry->startAddr != sysTableEntryAddr; curSysTableEntry = curSysTableEntry->next)
         prevSysTableEntry = curSysTableEntry; 

    if (curSysTableEntry == NULL || curSysTableEntry->inUse == FALSE) //0x00000614 & 0x00000624
        return NULL;

    /* Once found, mark it as being unused. */
    curSysTableEntry->inUse = FALSE; //0x00000630 & 0x00000638     
    
    /*
     * If the previous system-call-entry-table is currently being 
     * unused, merge these two tables together.  Integrate the freed 
     * table back into the list of currently free system call 
     * entry-tables if it was unlinked from it during allocation process.
     */
    if (prevSysTableEntry != NULL && prevSysTableEntry->inUse == FALSE) { //0x00000634        
        prevSysTableEntry->numEntries += curSysTableEntry->numEntries; //0x0000065C & 0x00000664
        prevSysTableEntry->next = curSysTableEntry->next; //0x0000066C 
        
        if (curSysTableEntry >= &initialSysEntTable[0] && curSysTableEntry <= &initialSysEntTable[TOP_SYSCALL_ENTRY_TABLE]) { //0x00000668 & 0x00000678
            curSysTableEntry->next = g_pFreeSysEnt; //0x00000688
            g_pFreeSysEnt = curSysTableEntry; //0x0000068C
            curSysTableEntry = prevSysTableEntry; //0x00000690             
        } 
        else {
            sceKernelFreeHeapMemory(g_loadCoreHeap(), curSysTableEntry); //0x00000730
            curSysTableEntry = prevSysTableEntry;
        }
    }
    /*
     * If both the previous table and the next table are used,
     * or the specified table is the last element and its 
     * previous table is being used, don't merge it.
     */
    if (curSysTableEntry->next == NULL || curSysTableEntry->next->inUse) //0x00000698 - 0x000006A4
        return curSysTableEntry;
    
    /*
     * Here, we merge the upper (next) free system call table with the 
     * specified table by the user.
     */
    nextSysTableEntry = curSysTableEntry->next; //0x00000694
    curSysTableEntry->numEntries += nextSysTableEntry->numEntries; //0x000006BC & 0x000006C4      
    curSysTableEntry->next = nextSysTableEntry->next; //0x000006CC
        
    if (nextSysTableEntry >= &initialSysEntTable[0] && nextSysTableEntry <= &initialSysEntTable[TOP_SYSCALL_ENTRY_TABLE]) { //0x000006C8 & 0x000006D8
        nextSysTableEntry->next = g_pFreeSysEnt; //0x000006E8
        g_pFreeSysEnt = nextSysTableEntry;   
    }
    else {
        sceKernelFreeHeapMemory(g_loadCoreHeap(), nextSysTableEntry); //0x0000070C
    }
    return curSysTableEntry;
}

//sub_0x00000740
/*
 * The system call table's exported function slots are initialized 
 * with a pointer to this function.  Calling a function slot 
 * which wasn't set with a pointer to an exported function of 
 * a resident library thus results in a call to this function.
 */
s32 UndefSyscall(void) 
{
    return SCE_ERROR_KERNEL_ERROR;
}
