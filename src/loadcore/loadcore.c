/* Copyright (C) 2011, 2012, 2013 The uOFW team
   See the file COPYING for copying permission.
*/

/*
 * uofw/src/loadcore/loadcore.c
 * 
 * Loadcore - Basic API for the Module Manager and File Loader of the 
 * Program Loader.
 * 
 * The Program Loader handles the loading/relocating of modules and
 * manages modules in memory.  It contains two layers, the Module Manager
 * and the File Loader.  The Module Manager performs linking between
 * modules and registers/deletes resident libraries provided by modules.
 * Its basic API is implemented in loadcore.c.
 * 
 * The File Loader is responsible for loading object files and for
 * registering/deleting modules.  Its basic API is implemented in module.c
 * and loadelf.c
 * 
 * Specific tasks:
 *    1) Continue booting the rest of the system modules.  Every module upto
 *       init.prx (including init) will be booted and its resident libraries
 *       will be registered as well as its stub libraries being linked.  
 *    2) Provide the functions needed to register/delete resident libraries
 *       as well as functions needed to link/unlink stub libraries.
 *    3) Maintain a linked-list of currently registered libraries.
 * 
 */

#include <init.h>
#include <interruptman.h>
#include <loadcore.h>
#include <memlmd.h>
#include <modulemgr.h>
#include <modulemgr_nids.h>
#include <sysmem_kdebug.h>
#include <sysmem_utils_kernel.h>

#include "cache.h"
#include "clibUtils.h"
#include "hash.h"
#include "interruptController.h"
#include "loadcore_int.h"
#include "loadelf.h"
#include "module.h"
#include "nid.h"
#include "systable.h"

#define LOADCORE_VERSION_MINOR                  (17)

SCE_MODULE_INFO("sceLoaderCore", SCE_MODULE_KIRK_MEMLMD_LIB | SCE_MODULE_KERNEL | SCE_MODULE_ATTR_CANT_STOP | 
                                 SCE_MODULE_ATTR_EXCLUSIVE_LOAD | SCE_MODULE_ATTR_EXCLUSIVE_START, 1, 
                                 LOADCORE_VERSION_MINOR);
SCE_MODULE_BOOTSTART("loadCoreInit");
SCE_SDK_VERSION(SDK_VERSION);

#define SVALALIGN4(v)                           ((s32)((((u32)((s32)(v) >> 31)) >> 30) + (v)) >> 2)

#define LOADCORE_CYCLIC_POLYNOMIAL_HASH_RADIAX  (5)

#define RAM_SIZE_32_MB                          (0x2000000)

#define LOADCORE_HEAP_SIZE                      (4096)

#define RESIDENT_LIBRARY_CONTROL_BLOCKS         (30)
#define TOP_RESIDENT_LIBRARY_CONTROL_BLOCK      (RESIDENT_LIBRARY_CONTROL_BLOCKS - 1)

#define STUB_LIBRARY_CONTROL_BLOCKS             (90)
#define TOP_STUB_LIBRARY_CONTROL_BLOCK          (STUB_LIBRARY_CONTROL_BLOCKS - 1)

#define LOADCORE_LIBRARY_NAME                   "LoadCoreForKernel"
#define LOADCORE_EXPORTED_FUNCTIONS             (34)
#define LOADCORE_EXPORT_TABLE_ENTRIES           (LOADCORE_EXPORTED_FUNCTIONS * 2)

#define LOADCORE_HEAP_NAME                      "SceKernelLoadCore"

#define LOADCORE_KERNEL_PRX_BUFFER_NAME         "SceLoadcorePRXKernel"
#define LOADCORE_GZIP_BUFFER_NAME               "SceLoadcoreGzipBuffer"

/* Indicates that the specified system call is not linked. */
#define SYSCALL_NOT_LINKED                      (0x0000054C)

#define MAKE_KERNEL_ADDRESS(addr)               ((addr) | 0x80000000)
#define MAKE_USER_ADDRESS(addr)                 ((addr) & 0x7FFFFFFF)

#define MAKE_SYSCALL(n)                         (0x03FFFFFF & (((u32)(n) << 6) | 0x0000000C))
#define MAKE_JUMP(f)                            (0x08000000 | ((u32)(f)  & 0x0FFFFFFC)) 

/* MIPS ASM NOP instruction. */
#define NOP                                     (0x00000000)
/* MIPS ASM JR $ra assembly code */
#define JR_RA                                   (0x03E00008)

#define SECONDARY_MODULE_ID_START_VALUE         (0x10001)

#define SCE_PRIMARY_SYSCALL_HANDLER_SYSCALL_ID  (0x15)


/*
 * Indicate the privilege-level of a library (stub- and resident 
 * library).  These members are used when a resident library is
 * being registered and when a stub library is being 
 * linked.
 */
enum SceLibPrivilegeLevel {
    /* The library is living in kernel land. */
    LIB_PRIVILEGE_LEVEL_KERNEL = 0,
    /* The library is living in user land. */
    LIB_PRIVILEGE_LEVEL_USER = 1,
};

/*
 * Indicate the status of a resident library registration.  These
 * values are used during the process of registering a resident 
 * library to the system and determine whether the registration can
 * be performed or not.
 */
enum SceLibRegistrationStatus {
    /* The library cannot be registered.  This case occurs when 
     * a library with the same name is already registered and has
     * a different privilege level then the to-be-registered library.
     */
    LIB_REGISTRATION_DENIED = 1,
    /* The library is not registered.  This is the case when a library
     * with the same name is already registered, has the same privilege
     * level, can be updated (attribute SCE_LIB_WEAK_EXPORT set), but
     * has a higher (= newer) version than the to-be-registered library.
     */
    LIB_REGISTRATION_NOT_NEEDED = 2,
    /* The library can be registered. */
    LIB_REGISTRATION_OK = 3
};

/*
 * Specify the performed operation of the aLinkVariableStub() 
 * routine.
 */
enum SceVariableStubLinkOptions {
    /* A variable stub is being linked. */
    VARIABLE_STUB_LINK = 0,
    /* A variable stub is being unlinked. */
    VARIABLE_STUB_UNLINK = 1,
};

/*
 * Special return values of the aLinkLibEntries() routine.
 */
enum SceLibEntryLinkFailures {
    /* A resident library cannot be linked.  This occurs when the resident
     * library has the attribute SCE_LIB_NOLINK_EXPORT. 
     */
    EXPORT_LIB_CANNOT_BE_LINKED = 2,
    /* A kernel resident library cannot be linked to a user stub library
     * when not exporting its functions via system calls. 
     */
    KERNEL_DIRECT_EXPORT_LIB_CANNOT_BE_LINKED_WITH_USER_STUB_LIB = 3,
    /* A user resident library cannot be linked with a kernel stub library. */
    USER_EXPORT_LIB_CANNOT_BE_LINKED_WITH_KERNEL_STUB_LIB = 4,
    /* A kernel resident library exporting its functions via system calls 
     * cannot be linked with a stub library living in kernel land as well.
     */
    KERNEL_SYSCALL_EXPORT_LIB_CANNOT_BE_LINKED_WITH_KERNEL_STUB_LIB = 5
};

/*
 * Specify the performed operation of the find_nid_in_entrytable()
 * routine.
 */
enum SceNidSearchOption {
    /* The routine searches for a function NID. */
    FUNCTION_NID_SEARCH = 0,
    /* The routine searches for a variable NID. */
    VARIABLE_NID_SEARCH = 1,
};

//TOD0: Add member for 0x4.
/*
 * Stub library system status.
 */
enum SceStubLibraryStatus {
    /* The stub library is linked only the first time a resident
     * library is registered. If a newer version of that res. lib
     * is registered, the stub library won't be linked with that one
     * again.
     */
    STUB_LIBRARY_EXCLUSIVE_LINK = 0x1,
    /* The stub library is linked with its corresponding resident
     * library. 
     */
    STUB_LIBRARY_LINKED = 0x2,
};

static s32 aVariableLinkApply(u32 *arg1, u32 exportedVar, u32 linkOption, u32 isUserLib);
static s32 aLinkVariableStub_sub(SceResidentLibrary *lib, SceStubLibraryEntryTable *stubLibEntryTable, u32 linkOption, 
                                 u32 isUserLib);
static s32 search_nid_in_entrytable(SceResidentLibrary *lib, u32 nid, u32 arg2, u32 nidSearchOption);
static void SysBoot(SceLoadCoreBootInfo *bootInfo, SysMemThreadConfig *threadConfig);

static void LoadCoreHeapStatic(void); //0x00002678
static s32 LoadCoreHeapDynamic(void); //0x00002718
static void UpdateCacheAllStatic(void); //0x00002768
static void UpdateCacheAllDynamic(void); //0x0000278C
static void UpdateCacheDataAllStatic(void); //0x000027B0
static void UpdateCacheDataAllDynamic(void); //0x000027CC
static void UpdateCacheRangeStatic(const void *addr __attribute__((unused)), u32 size __attribute__((unused))); //0x000027E8
static void UpdateCacheRangeDynamic(const void *addr, u32 size); //0x0000280C
static void UpdateCacheRangeDataStatic(const void *addr __attribute__((unused)), u32 size __attribute__((unused))); //0x00002848
static void UpdateCacheRangeDataDynamic(const void *addr, u32 size); //0x00002864

static s32 doRegisterLibrary(SceResidentLibraryEntryTable *libEntryTable, u32 isUserLib);
static SceResidentLibrary **FoundLibrary(SceResidentLibraryEntryTable *libEntryTable);
static s32 ReleaseLibEntCB(SceResidentLibrary *lib, SceResidentLibrary **prevLibSlot);
static s32 doLinkLibraryEntries(SceStubLibraryEntryTable *stubLibEntryTable, u32 size, u32 isUserMode, 
                                u32 arg4 __attribute__((unused)));
static s32 UnLinkLibraryEntry(SceStubLibraryEntryTable *stubLibEntryTable);
static s32 aLinkLibEntries(SceStubLibrary *stubLib);
static s32 aLinkClient(SceStubLibrary *stubLib, SceResidentLibrary *lib);
static s32 ProcessModuleExportEnt(SceModule *mod, SceResidentLibraryEntryTable *lib);
static s32 CopyLibEnt(SceResidentLibrary *lib, SceResidentLibraryEntryTable *libEntryTable, u32 isUserLib);
static s32 CopyLibStub(SceStubLibrary *stubLib, SceStubLibraryEntryTable *stubLibEntryTable, u32 isUserLib);

static u32 sceKernelSetGetLengthFunction(s32 (*funcPtr)(u8 *file, u32 size, u32 *newSize)); //sub_0000562C
static u32 sceKernelSetPrepareGetLengthFunction(s32 (*funcPtr)(u8 *buf, u32 size)); //sub_0000563C
static u32 sceKernelSetSetMaskFunction(s32 (*funcPtr)(u32 unk1, vs32 *addr)); //sub_0000564C
static u32 sceKernelSetCompareSubType(s32 (*funcPtr)(u32 tag)); //sub_0000565C
static u32 sceKernelSetCompareLatestSubType(u32 (*funcPtr)(u32 tag)); //sub_0000566C

static __inline__ void StopLoadCore(void);

s32 (*g_loadCoreHeap)(void) = (s32 (*)(void))LoadCoreHeapStatic; //0x000080A0
void (*g_UpdateCacheAll)(void) = UpdateCacheAllStatic; //0x000080A4
void (*g_UpdateCacheDataAll)(void) = UpdateCacheDataAllStatic; //0x000080A8
void (*g_UpdateCacheRange)(const void *addr, u32 size) = UpdateCacheRangeStatic; //0x000080AC
void (*g_UpdateCacheRangeData)(const void *addr, u32 size) = (void (*)(const void *, u32))UpdateCacheRangeDataStatic; //0x000080B0

/*
 * Points to the next free SceResidentLibrary control block.  In the 
 * beginning, it is set to point to the top object of the 
 * g_LoadCoreLibEntries linked list.
 * Whenever a new control block is requested, the block pointed to by 
 * g_FreeLibEnt is returned and g_FreeLibEnt is set to point to the 
 * next free control block.
 * 
 * When freeing a currently used control block, g_FreeLibEnt is set to 
 * point to that specific block.
 */
static SceResidentLibrary *g_FreeLibEnt; //0x000080E0

/*
 * A linked list of resident library control blocks.  If more than 
 * RESIDENT_LIBRARY_CONTROL_BLOCKS are in use, the new blocks are 
 * allocated from the heap and are not inserted into the linked list
 * when being freed.
 */
static SceResidentLibrary g_LoadCoreLibEntries[RESIDENT_LIBRARY_CONTROL_BLOCKS]; //0x00008408

/*
 * A linked list of stub library control blocks. If more than 
 * STUB_LIBRARY_CONTROL_BLOCKS are in use, the new blocks are allocated 
 * from the heap and are not inserted into the linked list when being
 * freed.
 */
static SceStubLibrary g_LoadCoreLibStubs[STUB_LIBRARY_CONTROL_BLOCKS]; //0x00008D68

/*
 * Points to the next free stub library control block.  In the beginning
 * it is set to point to the top object of the g_LoadCoreLibStubs linked
 * list.  Whenever a new control block is requested, the object pointed 
 * to by g_FreeLibStub is returned and g_FreeLibStub is set to point to 
 * the next free control block.
 * 
 * When freeing a currently used control block g_FreeLibStub is set to 
 * point to that specific control block.
 */

s32 g_SyscallIntrRegSave[7]; //0x000080C0
static SceStubLibrary *g_FreeLibStub; //0x000080E4

s32 g_ToolBreakMode; //0x000083C4

/* Pointer to a memory clear function executed in StopLoadCore(). */
static void (*g_MemClearFun)(void *, u32); //0x000083C0
/* The start address of the memory block to clear. */
static void *g_MemBase; //0x000083C8
/* The size of the particular memory block to clear. */
static u32 g_MemSize; //0x000083CD

s32 (*g_getLengthFunc)(u8 *file, u32 size, u32 *newSize); //0x000083D0
s32 (*g_prepareGetLengthFunc)(u8 *buf, u32 size); //0x000083D4
s32 (*g_setMaskFunc)(u32 unk1, vs32 *addr); //0x000083D8
s32 (*g_compareSubType)(u32 tag); //0x000083DC
static u32 (*g_compareLatestSubType)(u32 tag); //0x000083E0

/* Contains the segment start addresses of the currently loaded module. */
void *g_segmentStart[SCE_KERNEL_MAX_MODULE_SEGMENT]; //0x000083E4
/* Contains the segment sizes of the currently loaded module. */
u32 g_segmentSize[SCE_KERNEL_MAX_MODULE_SEGMENT]; //0x000083F4

static u32 g_systemStatus; //0x0000A118

/* Loadcore's internal control block. */
SceLoadCore g_loadCore; //0x0000A11C

//0x00007F2C
const u32 g_hashinfo[8] = {
    0x00000000,
    0x001E0004,
    0x001C0010,
    0x001B0020,
    0x001A0040,
    0x00190080,
    0x00180100,
    0x00170200
};

//0x00008038
const u32 g_hashData[24] = {
    0x2CDEEB73, 
    0xF7E529B9, 
    0xF85FB599,
    0xA8B6B2D3,
    0xA0811E0E,
    0xF91FE045,
    0x51339FF7,
    0xBF9F32B2,
    0x89F0A05E,
    0xF413FE28,
    0x1F24A87E,
    0x7C1ADFE2,
    0x3D3203F8,
    0x4C6E9246,
    0x4DCA0BCB,
    0x71A9322A,
    0x7EAEEDE2,
    0xC7BACA41,
    0xC38D13D2,
    0xE54A1434,
    0x0FC5364D,
    0xF2EA99AA,
    0x7A418AA1,
    0xFEC29EA1,
};

/*
 * Loadcore's exported functions and their NIDs.
 */
const u32 g_loadCoreExportTable[LOADCORE_EXPORT_TABLE_ENTRIES] = {
    NID_SCE_KERNEL_DELETE_MODULE,
    NID_SCE_KERNEL_UNLINK_LIBRARY_ENTRIES,
    NID_SCE_KERNEL_MASK_LIBRARY_ENTRIES,
    NID_SCE_KERNEL_LOAD_CORE_LOCK,
    NID_SCE_KERNEL_LOAD_EXECUTABLE_OBJECT,
    NID_SCE_KERNEL_CREATE_MODULE,
    NID_SCE_KERNEL_REGISTER_LIBRARY_FOR_USER,
    NID_SCE_KERNEL_GET_MODULE_ID_LIST_FOR_KERNEL,
    NID_SCE_KERNEL_GET_MODULE_LIST_WITH_ALLOC,
    NID_SCE_KERNEL_FIND_MODULE_BY_UID,
    NID_SCE_KERNEL_GET_MODULE_GP_BY_ADDRESS_FOR_KERNEL,
    NID_SCE_KERNEL_PROBE_EXECUTABLE_OBJECT,
    NID_SCE_KERNEL_REGISTER_LIBRARY,
    NID_SCE_KERNEL_LOAD_MODULE_BOOT_LOAD_CORE,
    NID_SCE_KERNEL_CAN_RELEASE_LIBRARY,
    NID_SCE_KERNEL_SEGMENT_CHECKSUM,
    NID_SCE_KERNEL_QUERY_LOAD_CORE_CB,
    NID_SCE_KERNEL_LINK_LIBRARY_ENTRIES_FOR_USER,
    NID_SCE_KERNEL_CREATE_ASSIGN_MODULE,
    NID_SCE_KERNEL_LINK_LIBRARY_ENTRIES,
    NID_SCE_KERNEL_LINK_LIBRARY_ENTRIES_WITH_MODULE,
    NID_SCE_KERNEL_RELEASE_MODULE,
    NID_SCE_KERNEL_LOAD_REBOOT_BIN,
    NID_SCE_KERNEL_LOAD_CORE_UNLOCK,
    NID_SCE_KERNEL_FIND_MODULE_BY_ADDRESS,
    NID_SCE_KERNEL_REGISTER_MODULE,
    NID_SCE_KERNEL_LOAD_CORE_MODE,
    NID_SCE_KERNEL_RELEASE_LIBRARY,
    NID_SCE_KERNEL_GET_MODULE_FROM_UID,
    NID_SCE_KERNEL_CHECK_EXEC_FILE,
    NID_SCE_KERNEL_ASSIGN_MODULE,
    NID_SCE_KERNEL_FIND_MODULE_BY_NAME,
    NID_SCE_KERNEL_SET_BOOT_CALLBACK_LEVEL,
    NID_SCE_KERNEL_CHECK_PSP_CONFIG,   
    (u32)sceKernelDeleteModule,
    (u32)sceKernelUnlinkLibraryEntries,
    (u32)sceKernelMaskLibraryEntries,
    (u32)sceKernelLoadCoreLock,
    (u32)sceKernelLoadExecutableObject,
    (u32)sceKernelCreateModule,
    (u32)sceKernelRegisterLibraryForUser,
    (u32)sceKernelGetModuleIdListForKernel,
    (u32)sceKernelGetModuleListWithAlloc,
    (u32)sceKernelFindModuleByUID,
    (u32)sceKernelGetModuleGPByAddressForKernel,
    (u32)sceKernelProbeExecutableObject,
    (u32)sceKernelRegisterLibrary,
    (u32)sceKernelLoadModuleBootLoadCore,
    (u32)sceKernelCanReleaseLibrary,
    (u32)sceKernelSegmentChecksum,
    (u32)sceKernelQueryLoadCoreCB,
    (u32)sceKernelLinkLibraryEntriesForUser,
    (u32)sceKernelCreateAssignModule,
    (u32)sceKernelLinkLibraryEntries,
    (u32)sceKernelLinkLibraryEntriesWithModule,
    (u32)sceKernelReleaseModule,
    (u32)sceKernelLoadRebootBin,
    (u32)sceKernelLoadCoreUnlock,
    (u32)sceKernelFindModuleByAddress,
    (u32)sceKernelRegisterModule,
    (u32)sceKernelLoadCoreMode,
    (u32)sceKernelReleaseLibrary,
    (u32)sceKernelGetModuleFromUID,
    (u32)sceKernelCheckExecFile,
    (u32)sceKernelAssignModule,
    (u32)sceKernelFindModuleByName,
    (u32)sceKernelSetBootCallbackLevel,
    (u32)sceKernelCheckPspConfig,
};

//0x000078E4 -- Note: It is probably declared as "const".
/*
 * Loadcore's kernel mode resident library entry table.
 * It's a kernel-to-kernel export, in other words, only other
 * kernel modules can call these exported functions.  This
 * entry table is registered in the beginning of the LoadCore
 * boot process.
 */
SceResidentLibraryEntryTable loadCoreKernelLib = { 
    .libName = LOADCORE_LIBRARY_NAME,
    .version = {
        [LIBRARY_VERSION_MINOR] = LOADCORE_VERSION_MINOR,
        [LIBRARY_VERSION_MAJOR] = 0,
    },
    .attribute = SCE_LIB_AUTO_EXPORT,
    .len = LIBRARY_ENTRY_TABLE_NEW_LEN,
    .vStubCount = 0,
    .stubCount = LOADCORE_EXPORTED_FUNCTIONS,
    .entryTable = (u32 *)g_loadCoreExportTable,
    .unk16 = 0,
    .unk18 = 2,
    .unk19 = 0,
};

//sub_000000A0
/* Initialize Loadcore's resident library control block linked list. */
static u32 LibEntTableInit(void)
{
    u32 i;
    
    g_LoadCoreLibEntries[0].next = NULL;
    
    for (i = 0; i < TOP_RESIDENT_LIBRARY_CONTROL_BLOCK; i++)
         g_LoadCoreLibEntries[i + 1].next = &g_LoadCoreLibEntries[i];
    
    g_FreeLibEnt = &g_LoadCoreLibEntries[TOP_RESIDENT_LIBRARY_CONTROL_BLOCK];
    return SCE_ERROR_OK;
}

//sub_000000E8
/*
 * Return a requested resident library control block.  If less than 
 * RESIDENT_LIBRARY_CONTROL_BLOCKS blocks are currently being used, 
 * unlink a free block from the list.  Otherwise, allocate a control 
 * block from the heap.
 */
static SceResidentLibrary *NewLibEntCB(void)
{
    SceResidentLibrary *curFreeLibEntry;
    
    curFreeLibEntry = g_FreeLibEnt;
    if (g_FreeLibEnt == NULL)    
        return sceKernelAllocHeapMemory(g_loadCoreHeap(), sizeof(SceResidentLibrary));       

    g_FreeLibEnt = g_FreeLibEnt->next;
    curFreeLibEntry->next = NULL;
        
    return curFreeLibEntry;
}

//sub_00000140
/*
 * Free a used resident library control block. It is called when
 *    a) registering a resident library failed
 *    b) a resident library is being unregistered.
 * 
 * If more than RESIDENT_LIBRARY_CONTROL_BLOCKS blocks are currently 
 * being used, the control block is freed instead of being placed on 
 * the linked list.
 */
static SceResidentLibrary *FreeLibEntCB(SceResidentLibrary *libEntry)
{      
    if (libEntry->libNameInHeap) {
        sceKernelFreeHeapMemory(g_loadCoreHeap(), libEntry->libName); 
        libEntry->libNameInHeap = SCE_FALSE;
    }       
    if (libEntry >= &g_LoadCoreLibEntries[0] && libEntry <= &g_LoadCoreLibEntries[TOP_RESIDENT_LIBRARY_CONTROL_BLOCK]) { //0x00000170 & 0x0000017C    
        libEntry->next = g_FreeLibEnt;
        g_FreeLibEnt = libEntry;
        
        return libEntry;
    }
    sceKernelFreeHeapMemory(g_loadCoreHeap(), libEntry); //0x000001A4     
    return g_FreeLibEnt;
}

//sub_000001F4
/* Initialize Loadcore's stub library control block linked list. */
static u32 LibStubTableInit(void) 
{
    u32 i;   
    
    g_LoadCoreLibStubs[0].next = NULL;    
    for (i = 0; i < TOP_STUB_LIBRARY_CONTROL_BLOCK; i++)    
         g_LoadCoreLibStubs[i + 1].next = &g_LoadCoreLibStubs[i];
    
    g_FreeLibStub = &g_LoadCoreLibStubs[TOP_STUB_LIBRARY_CONTROL_BLOCK];
    return SCE_ERROR_OK;

} 

//sub_0000023C
/*
 * Return a requested stub library control block.  If less than 
 * STUB_LIBRARY_CONTROL_BLOCKS blocks are currently being used, unlink a 
 * free block from the list.  Otherwise, allocate a control block from 
 * the heap.
 */
static SceStubLibrary *NewLibStubCB(void)
{
    SceStubLibrary *curFreeLibStub;
    
    curFreeLibStub = g_FreeLibStub;
    if (g_FreeLibStub == NULL) //0x0000024C
        return sceKernelAllocHeapMemory(g_loadCoreHeap(), sizeof(SceStubLibrary));

    g_FreeLibStub = g_FreeLibStub->next;
    return curFreeLibStub;
}

//sub_00000290
/*
 * Free a used stub library control block. It is called when
 *    a) linking library entries failed
 *    b) the particular stub library to register already exists.
 * 
 * If more than STUB_LIBRARY_CONTROL_BLOCKS blocks are currently being 
 * used, the control block is freed instead of being placed on the
 * linked list.
 */
static SceStubLibrary *FreeLibStubCB(SceStubLibrary *stubLib)
{  
    if (stubLib->libNameInHeap == SCE_TRUE) { //0x000002A8
        sceKernelFreeHeapMemory(g_loadCoreHeap(), stubLib->libName2); //0x0000031C       
        stubLib->libNameInHeap = SCE_FALSE;
    }           
    if (stubLib >= &g_LoadCoreLibStubs[0] && stubLib <= &g_LoadCoreLibStubs[TOP_STUB_LIBRARY_CONTROL_BLOCK]) { //0x000002C0 & 0x000002CC
        stubLib->next = g_FreeLibStub;
        g_FreeLibStub = stubLib;
        
        return stubLib;
    }
    sceKernelFreeHeapMemory(g_loadCoreHeap(), stubLib); //0x00000308       
    return g_FreeLibStub;
}

//sub_0000074C
static s32 aLinkVariableStub(SceResidentLibrary *libEntry, SceStubLibraryEntryTable *stubLibEntryTable, u32 isUserLib)
{   
    return aLinkVariableStub_sub(libEntry, stubLibEntryTable, VARIABLE_STUB_LINK, isUserLib);
}

//sub_0000076C
static s32 aUnLinkVariableStub(SceResidentLibrary *libEntry, SceStubLibraryEntryTable *stubLibEntryTable, u32 isUserLib)
{
    return aLinkVariableStub_sub(libEntry, stubLibEntryTable, VARIABLE_STUB_UNLINK, isUserLib);
}

//sub_0000078C
/* 
 * Link variable stub entries. Linking itself is performed in
 * aVariableLinkApply(). 
 */
static s32 aLinkVariableStub_sub(SceResidentLibrary *lib, SceStubLibraryEntryTable *stubLibEntryTable, u32 linkOption, 
                                 u32 isUserLib)
{
    u32 i;
    u32 j;
    u32 vStubCount;    
    s32 pos;
    
    /* Verify if the stub lib entry table imports variables. */
    if (stubLibEntryTable->len < STUB_LIBRARY_ENTRY_TABLE_OLD_LEN)
        return SCE_ERROR_OK;
    
    if (stubLibEntryTable->len >= STUB_LIBRARY_ENTRY_TABLE_NEW_LEN) //0x000007D8
        vStubCount = (stubLibEntryTable->unk24 < stubLibEntryTable->vStubCount) ? stubLibEntryTable->vStubCount : 
                                                                                  stubLibEntryTable->unk24; //0x00000918
    else
        vStubCount = stubLibEntryTable->vStubCount; //0x000007E0

    if (vStubCount == 0) //0x000007E4
        return SCE_ERROR_OK;
    
    if (lib->vStubCount < vStubCount || stubLibEntryTable->vStubTable == NULL) //0x000007F8 & 0x0000080C
        return SCE_ERROR_KERNEL_ERROR;   
    
    //0x00000814 - 0x000008B4
    for (i = 0; i < 2; i++) {      
         //0x00000828 & 0x00000840 & 0x00000850 - 0x00000898
         for (j = 0; j < vStubCount; j++) {
              pos = search_nid_in_entrytable(lib, stubLibEntryTable->vStubTable[j].nid, 0xFFFFFFFF, VARIABLE_NID_SEARCH); //0x0000085C
              if (pos < 0) //0x00000884
                  return SCE_ERROR_KERNEL_ERROR;
              
              /* Prevent user data to be written at kernel mem address. */
              if (isUserLib && IS_KERNEL_ADDR(stubLibEntryTable->vStubTable[j].addr)) //0x00000890 & 0x00000890
                  return SCE_ERROR_KERNEL_ERROR;
              
              if (i == 1) { //0x00000898
                  aVariableLinkApply(stubLibEntryTable->vStubTable[j].addr, 
                                     lib->entryTable[lib->numExports + lib->stubCount + pos], linkOption, isUserLib); //0x000008FC
              }
         }        
    }
    return SCE_ERROR_OK;
}

//sub_0000091C
/* Link a variable stub. */
static s32 aVariableLinkApply(u32 *arg1, u32 exportedVar, u32 linkOption, u32 isUserLib) 
{
    u32 i;
    u32 j;
    u32 k;
    u32 *addr;
    u32 val2;
    s32 tmp;
    u32 tmp1;
    u32 tmp2;
    
    if (isUserLib && ((s32)(((s32)arg1) & 0xF0000000)) < 0) //0x0000092C & 0x00000938
        return SCE_ERROR_KERNEL_ERROR;
    
    k = 1; //0x00000964
    for (i = 0; arg1[i] != 0; i += k) { //0x00000944
         addr = (u32 *)(((u32)arg1 & 0xF0000000) | ((arg1[i] & 0x03FFFFFF) << 2)); //0x0000097C
         if (arg1[i] >> 26 == 5) { //0x00000978
             val2 = (*addr) << 16; //0x00000A2C
             
             j = i + 1; //0x00000A10
             if (arg1[j] != 0 && (arg1[j] >> 26 == arg1[i] >> 26)) { //0x00000A34
                 while (arg1[++j] != 0 && arg1[++j] >> 26 == 5)
                     k++;
             }
             tmp = *(s16 *)((((u32)arg1) & 0xF0000000) | ((arg1[i] & 0x03FFFFFF) << 2)); //0x00000A48 - 0x00000A5C
             tmp1 = val2 + tmp + exportedVar; //0x00000A58 & 0x00000A64
             tmp2 = val2 + tmp - exportedVar; //0x00000A68
             val2 = (linkOption == VARIABLE_STUB_LINK) ? tmp1 : tmp2; //0x00000A6C            
             
             //0x00000A78 & 0x00000A88 - 0x00000AB8
             for (j = 0; j < k; j++) {
                  tmp = *(u32 *)((arg1[j] & 0x03FFFFFF) << 2) | (((u32)arg1) & 0xF0000000); //0x00000A98
                  arg1[j] = tmp | (((val2 >> 15) + 1) & 0x1FFFE); //0x00000A7C & 0x00000AA4 & 0x00000AAC
             }
         }
         else if ((arg1[i] >> 26) > 5) { //0x00000984
             if ((arg1[i] >> 26) != 6) //0x000009E0
                 continue;
             
             tmp = *(s16 *)addr;
             val2 = (linkOption == VARIABLE_STUB_LINK) ? tmp + exportedVar : tmp - exportedVar; //0x000009FC
             *addr = (val2 & 0xFFFF) | (tmp & 0xFFFF0000); //0x00000A0C & 0x000009D0
         }
         else if ((arg1[i] >> 26) == 2) { //0x0000098C
             *addr = (linkOption == VARIABLE_STUB_LINK) ? *addr + exportedVar : *addr - exportedVar; //0x000009C4
         }
    }
    return SCE_ERROR_OK;
}

//Subroutine module_bootstart - Address 0x00000AF8
/*
 * Initialize Loadcore and continue booting the system.  This process
 * includes several steps:
 *    1) Setup LoadCore's internal buffers:
 *       a) Initialize the linked-list of loaded resident libraries.
 *       b) Initialize the resident libraries/stub libraries free 
 *          stack of pre-allocated buffers.
 * 
 *    2) Register and link libraries:
 *       a) Register Sysmem's/Loadcore's kernel resident libraries.
 *       b) Link Loadcore's stub libraries (only Sysmem export 
 *          references are resolved here).
 * 
 *    3) Register modules:
 *       a) Create the UID type for "module" objects.  This UID type is 
 *          used to create the module UIDs.
 *       b) Create and register the Sysmem and Loadcore modules.
 *    
 *    4) Call SysBoot to boot the rest of the modules (up to "SceInit").
 * 
 * Returns 0 on success.   
 */
s32 loadCoreInit(SceSize argc __attribute__((unused)), void *argp) 
{
    u32 i;
    for (i = 0; i < 480 * 272 * 2; i++) *(int*)(0x44000000 + i * 4) = 0x00FF0000;
    s32 seedPart1;
    s32 status;    
    s32 linkLibsRes;
    SceModule *mod = NULL;
    SceLoadCoreBootInfo *bootInfo = NULL;
    SysMemThreadConfig *sysMemThreadConfig = NULL;         
    
    bootInfo = ((SceLoadCoreBootInfo **)argp)[0];
    sysMemThreadConfig = ((SysMemThreadConfig **)argp)[1]; //0x00000B38
    g_MemSize = bootInfo->memSize; //0x00000B44
    g_MemBase = bootInfo->memBase; //0x00000B48
    
    //0x00000B18 & 0x00000B4C - 0x00000B5C
    for (i = 0; i < LOADCORE_LIB_HASH_TABLE_SIZE; i++)
         g_loadCore.registeredLibs[i] = NULL;
    
    seedPart1 = sysMemThreadConfig->unk48 & 0x1FF; //0x00000B60 & 0x00000B6C
    
    //0x00000B78 - 0x00000BA4
    g_loadCore.sysCallTableSeed = seedPart1 + 0x2000;
    g_loadCore.secModId = SECONDARY_MODULE_ID_START_VALUE;
    g_loadCore.loadCoreHeapId = SCE_KERNEL_VALUE_UNITIALIZED;
    g_loadCore.linkedLoadCoreStubs = SCE_FALSE;
    g_loadCore.sysCallTable = NULL;
    g_loadCore.unk520 = 0;
    g_loadCore.unLinkedStubLibs = NULL;
    g_loadCore.registeredMods = NULL;
    g_loadCore.lastRegMod = NULL;
    g_loadCore.regModCount = 0;
    g_loadCore.bootCallBacks = NULL;
    
    LibEntTableInit();
    LibStubTableInit();   
    
    if (sysMemThreadConfig->GetLengthFunction != NULL) //0x00000BB4
        sceKernelSetGetLengthFunction(sysMemThreadConfig->GetLengthFunction); //0x00000E98
    
    if (sysMemThreadConfig->PrepareGetLengthFunction != NULL) //0x00000BC0
        sceKernelSetPrepareGetLengthFunction(sysMemThreadConfig->PrepareGetLengthFunction); //0x00000E88

    if (sysMemThreadConfig->SetMaskFunction != NULL) //0x00000BCC
        sceKernelSetSetMaskFunction(sysMemThreadConfig->SetMaskFunction); //0x00000E58  

    if (sysMemThreadConfig->CompareSubType != NULL) //0x00000BD8
        sceKernelSetCompareSubType(sysMemThreadConfig->CompareSubType); //0x00000E68

    if (sysMemThreadConfig->CompareLatestSubType != NULL) //0x00000BE4
        sceKernelSetCompareLatestSubType(sysMemThreadConfig->CompareLatestSubType); //0x00000E58

    /* register Sysmem's kernel resident libraries. */             
    for (i = 0; i < sysMemThreadConfig->numKernelLibs; i++) { //0x00000BF0 & 0x00000BFC - 0x00000C24
         if (sceKernelRegisterLibrary(sysMemThreadConfig->kernelLibs[i]) != SCE_ERROR_OK)
             StopLoadCore();
    } 
    status = sceKernelRegisterLibrary(&loadCoreKernelLib); //0x00000C2C
    if (status != SCE_ERROR_OK) //0x00000C34
        StopLoadCore();

    linkLibsRes = sceKernelLinkLibraryEntries(sysMemThreadConfig->loadCoreImportTables, 
                                              sysMemThreadConfig->loadCoreImportTablesSize); //0x00000CD0
    g_loadCore.linkedLoadCoreStubs = SCE_TRUE;
    
    g_UpdateCacheAll = UpdateCacheAllDynamic; //0x00000D1C
    g_UpdateCacheDataAll = UpdateCacheDataAllDynamic; //0x00000D20
    g_loadCoreHeap = LoadCoreHeapDynamic; //0x00000D24
    g_UpdateCacheRange = UpdateCacheRangeDynamic; //0x00000D28
    g_UpdateCacheRangeData = UpdateCacheRangeDataDynamic; //0x00000D30   
    
    ModuleServiceInit(); //0x00000D2C
    
    mod = sceKernelCreateAssignModule(sysMemThreadConfig->sysMemExecInfo); //0x00000D34
    sceKernelRegisterModule(mod); //0x00000D40
    mod->segmentChecksum = sceKernelSegmentChecksum(mod); //0x00000D48 & 0x00000D54    
    mod->status = (mod->status & ~0xF) | MCB_STATUS_STARTED; //0x00000D50 & 0x00000D58 - 0x00000D60
    
    mod = sceKernelCreateAssignModule(sysMemThreadConfig->loadCoreExecInfo); //0x00000D64
    sceKernelRegisterModule(mod); //0x00000D70
    mod->segmentChecksum = sceKernelSegmentChecksum(mod); //0x00000D78 & 0x00000D84   
    mod->status = (mod->status & ~0xF) | MCB_STATUS_STARTED ; //0x00000D80 & 0x00000D8C - 0x00000D94 
    
    SysBoot(bootInfo, sysMemThreadConfig); //0x00000D98
    return linkLibsRes;
}

//sub_00000EA8
/*
 * Find the location of a NID in a resident library's entry table.
 * This routine can perform two operations; finding a function NID
 * and finding a variable NID.  The operation can be specified via
 * "nidSearchOption".
 * 
 * Returns the NID position (greater than 0) on success.
 */
static s32 search_nid_in_entrytable(SceResidentLibrary *lib, u32 nid, u32 arg2, u32 nidSearchOption) 
{
    u16 endNidPos; 
    u16 startIndex;
    u32 nidCurIndex;
    u32 *entryTable;
    u32 *curNidPtr;
    u32 nNids;
    u8 counter;
    u8 unk2;
    s32 i;
    s32 unk6, unk7, unk8, unk9, unk10, unk11, unk12, unk14;    
    
    if (nidSearchOption == FUNCTION_NID_SEARCH) { //0x00000EB0
        endNidPos = lib->stubCount; //0x00000FD0
        unk2 = lib->unk20; //0x00000FD4
        startIndex = 0; //0x00000FD8
        unk7 = 0; //0x00000FE0
    }
    else {
        startIndex = lib->stubCount; //0x00000EB8
        endNidPos = lib->numExports; //0x00000EBC
        unk2 = lib->unk21; //0x00000EC0
        unk7 = lib->unk24; //0x00000EC4
    }
    
    entryTable = (u32 *)lib->entryTable; //0x00000ECC
    unk6 = lib->exportsSize; //0x00000ED0
    counter = lib->unk22; //0x00000EC8
    
    unk8 = (nid >> unk2) + unk7; //0x00000EDC & 0x00000EE0
    unk10 = startIndex << 1; //0x00000EE4
    unk11 = unk8 << 1; //0x00000EE8
    unk12 = -unk10; //0x00000EEC
    nNids = endNidPos - startIndex; //0x00000EF4
    
    //0x00000ED4 - 0x00000F80
    for (i = 0; i < counter; i++) {  
         unk8 = 1 << i; //0x00000EF8
         if ((arg2 & unk8) == 0) { //0x00000F00     
             unk6 += lib->unk48; //0x00000F04 & 0x00000F84 & 0x00000F88
             entryTable += unk6; //0x00000F8C
             unk6 = lib->midFuncIndex; //0x00000F94                       
             continue;
         }
         
         nidCurIndex = startIndex; //0x00000F0C
         if (unk2 != 0) { //0x00000F08 --
             unk8 = (int)(entryTable + unk6); //0x00000F10 - 0x00000F14
             curNidPtr = (u32 *)(unk11 + unk8); //0x00000F14
            
             if (nNids < *curNidPtr) //0x00000F20
                 return SCE_ERROR_KERNEL_ERROR;
             
             unk8 = startIndex + *curNidPtr; //0x00000F28 & 0x00000F2C
             curNidPtr = (u32 *)(unk8 + entryTable); //0x00000F30
            
             if (nid < *curNidPtr) //0x00000F38 & 0x00000F3C
                 return SCE_ERROR_KERNEL_ERROR;
         }
         
         unk14 = unk10 << 1; //0x00000F44
         if (nidCurIndex >= endNidPos) { //0x00000F44 & 0x00000F48      
             unk6 += lib->unk48; //0x00000F04 & 0x00000F84 & 0x00000F88
             entryTable += unk6; //0x00000F8C
             unk6 = lib->midFuncIndex; //0x00000F94                       
             continue;
         }
         curNidPtr = (u32 *)(nidCurIndex << 1); //0x00000F50      
         unk8 = unk14 + unk12; //0x00000F54 
         curNidPtr = entryTable + ((u32)curNidPtr); //0x00000F58
            
         //0x00000F54 - 0x00000F74 
         while (nidCurIndex++ < endNidPos) {
                if (*curNidPtr == nid) { //0x00000F60
                    if (i == 0) //0x00000FB0
                        return nidCurIndex - startIndex; //0x00000FB4

                    unk9 = lib->numExports << 2; //0x00000FBC
                    unk9 = (u32)(entryTable + unk9); //0x00000FC0
                    return *(u16 *)(unk8 + unk9); //0x00000FC4 & 0x00000FCC
                    
                }
                curNidPtr++; //0x00000F64
                unk8 += 2; //0x00000F74
        }
    }
    return SCE_ERROR_KERNEL_ERROR;
}

//Subroutine LoadCoreForKernel_48AF96A9 - Address 0x00000FE4
s32 sceKernelRegisterLibrary(SceResidentLibraryEntryTable *libEntryTable) 
{
    return doRegisterLibrary(libEntryTable, LIB_PRIVILEGE_LEVEL_KERNEL); //0x00000FEC
}

//Subroutine LoadCoreForKernel_538129F8 - Address 0x00001000
s32 sceKernelCanReleaseLibrary(SceResidentLibraryEntryTable *libEntryTable)
{
   SceResidentLibrary *curLib = NULL;
   SceStubLibrary *curStubLib = NULL;
   u32 intrState;   
   u32 index;
   
   intrState = loadCoreCpuSuspendIntr(); //0x00001014 
  
   index = getCyclicPolynomialHash(libEntryTable->libName, LOADCORE_CYCLIC_POLYNOMIAL_HASH_RADIAX, 
                                   LOADCORE_LIB_HASH_TABLE_SIZE);  
   for (curLib = g_loadCore.registeredLibs[index]; curLib; curLib = curLib->next) {
        if (curLib->libEntryTable != libEntryTable) //0x00001090
            continue;
        
        /*
         * Verify if there are currently loaded stub libraries which are
         * dependant on the specified resident library.  If this is the case,
         * the specified library cannot be released.
         */
        for (curStubLib = curLib->stubLibs; curStubLib; curStubLib = curStubLib->next) {           
             if (!(curStubLib->attribute & SCE_LIB_WEAK_IMPORT)) {
                 loadCoreCpuResumeIntr(intrState);
                 return SCE_ERROR_KERNEL_LIBRARY_IN_USE;
             }
        }
   }  
   loadCoreCpuResumeIntr(intrState);
   return SCE_ERROR_OK;
}

//Subroutine LoadCoreForKernel_8EAE9534 - Address 0x00001110 
s32 sceKernelLinkLibraryEntries(SceStubLibraryEntryTable *stubLibEntryTable, u32 size)
{
    return doLinkLibraryEntries(stubLibEntryTable, size, LIB_PRIVILEGE_LEVEL_KERNEL, 0);
}

//Subroutine LoadCoreForKernel_0295CFCE - Address 0x00001130 
s32 sceKernelUnlinkLibraryEntries(SceStubLibraryEntryTable *stubLibEntryTable, u32 size)
{
    void *curStubLibEntryTable;
    
    dbg_printf("Called %s\n", __FUNCTION__);
    
    for (curStubLibEntryTable = stubLibEntryTable; curStubLibEntryTable < ((void *)stubLibEntryTable) + size; 
       curStubLibEntryTable += ((SceStubLibraryEntryTable *)curStubLibEntryTable)->len * sizeof(u32))
         UnLinkLibraryEntry(curStubLibEntryTable);
    
    return SCE_ERROR_OK;
}

//Subroutine LoadCoreForKernel_493EE781 - Address 0x00001190
s32 sceKernelLoadModuleBootLoadCore(SceLoadCoreBootModuleInfo *bootModInfo, SceLoadCoreExecFileInfo *execInfo, 
                                    SceUID *modMemId) 
{
    SceUID blockId, tmpBlockId;
    u32 *ptr;
    s32 status;
    u8 *fileBuf = NULL;
    u8 isPrx;
    
    //0x000011C4 -  0x000011CC
    for (ptr = (u32 *)((void *)(execInfo) - sizeof(SceLoadCoreExecFileInfo)); ptr < (u32 *)execInfo; ptr++)
         *ptr = 0;
    
    tmpBlockId = 0;
    
    execInfo->apiType = 0x50; //0x000011DC
    execInfo->isSignChecked = SCE_TRUE;
    execInfo->topAddr = NULL;
    execInfo->modeAttribute = 0;
    
    status = sceKernelCheckExecFile(bootModInfo->modBuf, execInfo); //0x000011F0
    if (status < SCE_ERROR_OK) //0x000011F8
        StopLoadCore();
    
    fileBuf = bootModInfo->modBuf; //0x000011E4
    
    if (execInfo->execAttribute & SCE_EXEC_FILE_COMPRESSED) { //0x00001208
        blockId = sceKernelAllocPartitionMemory(SCE_KERNEL_PRIMARY_KERNEL_PARTITION, LOADCORE_GZIP_BUFFER_NAME, 
                                                SCE_KERNEL_SMEM_High, execInfo->decSize, 0); //0x00001224
        tmpBlockId = blockId;
        execInfo->topAddr = sceKernelGetBlockHeadAddr(blockId); //0x00001230        
        status = sceKernelCheckExecFile(bootModInfo->modBuf, execInfo); //0x00001240
        if (status < SCE_ERROR_OK) //0x00001248
            StopLoadCore();

        execInfo->topAddr = NULL; //0x00001254
        fileBuf = execInfo->fileBase; //0x00001258
    }
    status = sceKernelProbeExecutableObject(fileBuf, execInfo); //0x0000125C
    if (status < SCE_ERROR_OK) //0x00001264
        StopLoadCore();
    
    //0x0000126C - 0x00001290
    switch (execInfo->elfType) {
    case SCE_EXEC_FILE_TYPE_PRX: case SCE_EXEC_FILE_TYPE_PRX_2:
        isPrx = SCE_TRUE; //jumps to 0x00001298
        break;
    default: 
        isPrx = SCE_FALSE;
        break;
    }  
    if (!isPrx)
        StopLoadCore();
    
    if ((execInfo->modInfoAttribute & SCE_MODULE_PRIVILEGE_LEVELS) == SCE_MODULE_KERNEL) { //0x000012A4
        blockId = sceKernelAllocPartitionMemory(SCE_KERNEL_PRIMARY_KERNEL_PARTITION, LOADCORE_KERNEL_PRX_BUFFER_NAME, 
                                                SCE_KERNEL_SMEM_Low, execInfo->largestSegSize, 0); //0x0000134C
        //TODO: Add NULL-pointer check, Sony forgot it.
        *modMemId = blockId;
        execInfo->topAddr = sceKernelGetBlockHeadAddr(blockId); //0x00001360
        status = sceKernelLoadExecutableObject(bootModInfo->modBuf, execInfo); //0x00001368
        if (status < SCE_ERROR_OK)
            StopLoadCore();

        sceKernelMemset32(bootModInfo->modBuf, 0, bootModInfo->modSize); //0x00001380
        if (tmpBlockId <= 0) //0x00001388
            return status;
        
        /* Free allocated SceLoadcoreGzipBuffer. */
        return sceKernelFreePartitionMemory(tmpBlockId); //0x00001390
    }
    StopLoadCore();
    //Note: This return statement isn't included in original loadcore.
    return SCE_KERNEL_VALUE_UNITIALIZED;
}

//Subroutine sub_00001694
/*
 * Boots the rest of the system.  SysBoot is called after the System Memory
 * Manager and Loadcore are booted and it loads/boots the following PRX 
 * modules:
 *    Exception Manager, InterruptionMmanager, Thread Manager, 
 *    Direct Memory Access Controller Manager, System Timer, I/O File Manager,
 *    memlmd_0X, Module Manager, Init
 * 
 * Loading and booting these modules include registering their resident
 * libraries as well as linking their stub libraries with registered 
 * resident libraries.  For each module, its start entry function is called.
 */
static void SysBoot(SceLoadCoreBootInfo *bootInfo, SysMemThreadConfig *threadConfig)
{
    SceModule *mod = NULL;
    SceLoadCoreBootInfo *loadCoreBootInfo = NULL;
    SceLoadCoreBootModuleInfo *loadCoreBootModuleInfo = NULL;
    SceLoadCoreProtectInfo *protectInfo = NULL;
    SceLoadCoreExecFileInfo execInfo;
    SceResidentLibrary *curLib = NULL;
    SceResidentLibrary **prevLibPtr = NULL;
    SceKernelThreadEntry entryPoint;
    SceKernelBootCallbackFunction bootCbFunc;
    SceUID modMemId = 0;
    SceUID memBlockId;
    SceResidentLibraryEntryTable *curExportTable = NULL;
    SceResidentLibraryEntryTable *exportsEnd = NULL;
    u32 intrState;
    s32 status;
    u32 loadedMods;
    s32 i, j;
    u32 size;
    
    /* 
     * Disallow interrupts, set processor mode to kernel mode, set
     * exception level to 0.
     */
    g_systemStatus = pspCop0StateGet(COP0_STATE_STATUS); //0x000016DC
    pspCop0StateSet(COP0_STATE_STATUS, g_systemStatus & ~0x1F); //0x000016E0
    
    /*
     * Allocate a new kernel memory partition for loadCore's Boot 
     * information and fill it with the passed boot information provided
     * by the System Memory Manager module.
     */
    memBlockId = sceKernelAllocPartitionMemory(SCE_KERNEL_PRIMARY_KERNEL_PARTITION, "SceLoadCoreBootInfo", 
                                               SCE_KERNEL_SMEM_High, sizeof(SceLoadCoreBootInfo), 0);
    if (memBlockId < 0) //0x00001704
        StopLoadCore(); 
    
    loadCoreBootInfo = (SceLoadCoreBootInfo *)sceKernelGetBlockHeadAddr(memBlockId); //0x0000170C
   
    //0x0000171C - 0x00001734
    for (i = 0; i < 32; i++)
         ((u32 *)loadCoreBootInfo)[i] = ((u32 *)bootInfo)[i];
   
    /*
     * Create a new kernel memory partition for every module to boot.
     * The partition is used to perform checks on the module before booting
     * it and to provide boot information about the module to the system.
     */
    if (loadCoreBootInfo->numModules > 0) { //0x0000173C
        size = loadCoreBootInfo->numModules * sizeof(SceLoadCoreBootModuleInfo);
        memBlockId = sceKernelAllocPartitionMemory(SCE_KERNEL_PRIMARY_KERNEL_PARTITION, "SceLoadCoreBootModuleInfo", 
                                                   SCE_KERNEL_SMEM_High, size, 0); //0x00001758
        if (memBlockId < 0) //0x00001760
            StopLoadCore();
       
        loadCoreBootModuleInfo = (SceLoadCoreBootModuleInfo *)sceKernelGetBlockHeadAddr(memBlockId); //0x00001768
       
        //0x00001790 - 0x000017A8
        for (i = SVALALIGN4(loadCoreBootInfo->numModules) - 1; i >= 0; i--)
             loadCoreBootModuleInfo[i] = loadCoreBootInfo->modules[i];
       
        loadCoreBootInfo->modules = loadCoreBootModuleInfo; //0x000017AC
    } 
    else {
        loadCoreBootInfo->modules = NULL; //0x00002428
    }   
   
    /* 
     * Create a new kernel memory partition for every "protected information". 
     * Fill the partition with the specific "protect info" provided by the system.
     */
    if (loadCoreBootInfo->numProtects != 0) { //0x000017B4
        size = loadCoreBootInfo->numProtects * sizeof(SceLoadCoreProtectInfo); //0x00002290
        memBlockId = sceKernelAllocPartitionMemory(SCE_KERNEL_PRIMARY_KERNEL_PARTITION, "SceLoadCoreProtectInfo", 
                                                   SCE_KERNEL_SMEM_High, size, 0); //0x000022AC
        if (memBlockId < 0) //0x000022B4
            StopLoadCore();
      
        protectInfo = (SceLoadCoreProtectInfo *)sceKernelGetBlockHeadAddr(memBlockId);
        for (i = loadCoreBootInfo->numProtects - 1; i >= 0; i--)
             protectInfo[i] = loadCoreBootInfo->protects[i];
      
        loadCoreBootInfo->protects = protectInfo;       
    }
   
    /* Create a boot callback info object for every module to be loaded. */
    SceBootCallback bootCallbacks[loadCoreBootInfo->numModules] __attribute__((aligned(16))); //0x000017E0
    bootCallbacks[0].bootCBFunc = NULL; //0x000017D4
    g_loadCore.bootCallBacks = bootCallbacks; //0x000017F4
  
    //0x000017BC & 0x000017DC - 0x000017F0 & 0x00001804 - 0x00001970
    /* Load the specified modules and link them.*/
    for (loadedMods = loadCoreBootInfo->loadedModules; loadedMods < loadCoreBootInfo->numModules; loadedMods++) { 
         if (sceKernelLoadModuleBootLoadCore(&loadCoreBootInfo->modules[loadedMods], &execInfo, &modMemId) < SCE_ERROR_OK) //0x000017F8
             StopLoadCore(); //0x00002204
         
         exportsEnd = execInfo.exportsInfo + (execInfo.exportsSize & ~0x3); //0x00001824
      
         //0x00001850-0x00001870
         /* 
          * Step 2: Register the resident libraries of the recently loaded 
          *         module. 
          */
         for (curExportTable = execInfo.exportsInfo; curExportTable < exportsEnd; 
            curExportTable = (void *)curExportTable + curExportTable->len * sizeof(u32)) {
              if (curExportTable->attribute & SCE_LIB_AUTO_EXPORT) {              
                  if (sceKernelRegisterLibrary(curExportTable) < SCE_ERROR_OK) //0x0000216C
                      StopLoadCore(); //0x0000217C   
              }
         }      
         
         /* 
          * Step 3: Link the stub libraries of the recently loaded module
          *         with the currently registered resident libraries. 
          */
         if (((s32)execInfo.importsInfo) != SCE_KERNEL_VALUE_UNITIALIZED && 
           sceKernelLinkLibraryEntries(execInfo.importsInfo, execInfo.importsSize) < SCE_ERROR_OK)
             StopLoadCore(); //0x000020E0
      
         sceKernelIcacheClearAll(); //0x00001894
         sceKernelDcacheWBinvAll(); //0x000189C

         /* 
          * Step 4: Create a SceModule structure for the recently loaded 
          *         module. 
          */
         mod = sceKernelCreateAssignModule(&execInfo); //0x000018A4
         
         /* 
          * Verify if the SceModule structure could be created.  In case
          * it failed, unlink the module's stub libraries and unregister
          * its loaded resident libraries.
          */
         if (mod == NULL) {
             sceKernelUnlinkLibraryEntries(execInfo.importsInfo, execInfo.importsSize); //0x00001F24
          
             /*
              * In order to unregister the module's resident libraries, we search
              * for its resident libraries entry tables and compare them with the 
              * registered resident libraries.  If they match, we found a library 
              * belonging to the module and can unregister it.
              */
             exportsEnd = (execInfo.exportsSize & ~0x3) + execInfo.exportsInfo; //0x00001F30 - 0x00001F3C         
             //0x00001F44 & 0x00001F60 - 0x00001F78
             for (curExportTable = execInfo.exportsInfo; curExportTable < exportsEnd; 
                curExportTable = (void *)curExportTable + curExportTable->len * sizeof(u32)) {
                  if (curExportTable->attribute & SCE_LIB_AUTO_EXPORT) {                
                      intrState = loadCoreCpuSuspendIntr(); //0x00001F80
                   
                      i = getCyclicPolynomialHash(curExportTable->libName, LOADCORE_CYCLIC_POLYNOMIAL_HASH_RADIAX, 
                                                      LOADCORE_LIB_HASH_TABLE_SIZE);                   
                      for (curLib = g_loadCore.registeredLibs[i], prevLibPtr = &g_loadCore.registeredLibs[i]; curLib; 
                         prevLibPtr = &curLib->next, curLib = curLib->next) { //0x00001FF0 - 0x00002014
                           if (curLib->libEntryTable == curExportTable) //0x00002000
                               break;
                      }
                      if (curLib != NULL && ReleaseLibEntCB(curLib, prevLibPtr) != SCE_ERROR_OK) //0x0000201C
                          StopLoadCore();
                
                      loadCoreCpuResumeIntr(intrState);              
                  }	
             }
        }
        /*
         * Step 5: Register the loaded module and setup its start-, stop-function
         *         (and similar ones).  Boot the module by calling its entry point.
         */ 
        else {
            mod->memId = modMemId; //0x000018C0
            sceKernelRegisterModule(mod); //0x000018BC

            //0x000018C4 - 0x00001904
            exportsEnd = (execInfo.exportsSize & ~0x3) + execInfo.exportsInfo; //0x000018C4 - 0x000018D0
            for (curExportTable = execInfo.exportsInfo; curExportTable < exportsEnd; 
               curExportTable = (void *)curExportTable + curExportTable->len * sizeof(u32)) {
                 if ((curExportTable->attribute & SCE_LIB_IS_SYSLIB) == SCE_LIB_IS_SYSLIB)
                     ProcessModuleExportEnt(mod, curExportTable); //0x00001F14
            }           
            mod->segmentChecksum = sceKernelSegmentChecksum(mod); //0x00001908
            mod->textSegmentChecksum = 0; //0x0000191C
            
            /* Load and boot every system module up to init.prx. */
            if (strcmp(mod->modName, INIT_MODULE_NAME) == 0) //0x00001928
                break;
            
            entryPoint = (SceKernelThreadEntry)execInfo.entryAddr; //0x0000192C
            status = entryPoint(0, NULL); //0x00001934
            mod->status = (mod->status & ~0xF) | MCB_STATUS_STARTED; //0x0000193C - 0x0000194C
            
            if (status < SCE_ERROR_OK) //0x00001948
                StopLoadCore(); //0x00001E88 
            
            /* 
             * The module is not a resident module, that means it won't stay in
             * memory and provide functions/variables to other modules.  To secure
             * successful unloading of the module, we unlink its stub libraries and
             * unregister its resident libraries.
             */
            else if (status == SCE_KERNEL_NO_RESIDENT) { //0x00001954
                sceKernelUnlinkLibraryEntries(execInfo.importsInfo, execInfo.importsSize); //0x00001C90
                
                exportsEnd = (execInfo.exportsSize & ~0x3) + execInfo.exportsInfo; //0x00001C98 - 0x00001CA4
                for (curExportTable = execInfo.exportsInfo; curExportTable < exportsEnd; 
                   curExportTable = (void *)curExportTable + curExportTable->len * sizeof(u32)) {
                     if ((curExportTable->attribute & SCE_LIB_AUTO_EXPORT) == SCE_LIB_AUTO_EXPORT) { //0x00001CBC
                          intrState = loadCoreCpuSuspendIntr(); //0x00001D28
                          
                          i = getCyclicPolynomialHash(curExportTable->libName, LOADCORE_CYCLIC_POLYNOMIAL_HASH_RADIAX, 
                                                          LOADCORE_LIB_HASH_TABLE_SIZE); //0x00001D30 - 0x00001D90                          
                          for (curLib = g_loadCore.registeredLibs[i], prevLibPtr = &g_loadCore.registeredLibs[i]; curLib; 
                             prevLibPtr = &curLib->next, curLib = curLib->next) { //0x00001D98 - 0x00001DB8
                               if (curLib->libEntryTable == curExportTable) //0x00001DA8
                                   break;
                          }
                          if (curLib != NULL && ReleaseLibEntCB(curLib, prevLibPtr) != SCE_ERROR_OK ) //0x00001DC4 & 0x00001DDC
                              StopLoadCore();
                
                          loadCoreCpuResumeIntr(intrState); //0x00001DCC or 0x00001DE8        
                     }
                }
            }
            else 
                mod = NULL; //0x0000195C
        }
    }  
    //0x00001978 - 0x000019B8
    for (i = 0; i < loadCoreBootInfo->numProtects; i++) {
         if ((GET_PROTECT_INFO_TYPE(loadCoreBootInfo->protects[i].attr) == 0x200) 
                 && (GET_PROTECT_INFO_STATE(loadCoreBootInfo->protects[i].attr) & SCE_PROTECT_INFO_STATE_IS_ALLOCATED)) { //0x000019A8 & 0x00001C6C
             sceKernelFreePartitionMemory(loadCoreBootInfo->protects[i].partId); //0x00001C74
             loadCoreBootInfo->protects[i].attr = REMOVE_PROTECT_INFO_STATE(SCE_PROTECT_INFO_STATE_IS_ALLOCATED, 
                                                                             loadCoreBootInfo->protects[i].attr); //0x00001C84
         }
    }
    sceKernelSetGetLengthFunction(NULL); //0x000019BC
    sceKernelSetPrepareGetLengthFunction(NULL); //0x000019C4
    sceKernelSetSetMaskFunction(NULL); //0x000019CC
    
    //0x000019DC - 0x00001A0C
    /* Register the System Memory Manager user mode resident libraries. */
    u32 n;
    for (n = 0; n < threadConfig->numExportLibs - threadConfig->numKernelLibs; n++)
         sceKernelRegisterLibrary(threadConfig->userLibs[n]); //0x000019F0
       
    if (mod == NULL) //0x00001A10
        StopLoadCore(); //0x00001BDC

    //0x00001A18
    loadCoreBootInfo->loadedModules = loadedMods++;
    
    /* 
     * Boot init.prx. This module then initializes the rest of system
     * kernel modules, as well as the VSH/Game modules. 
     */
    entryPoint = (SceKernelThreadEntry)execInfo.entryAddr;    
    status = entryPoint(4, &execInfo); //0x00001A28
    
    mod->status = (mod->status & ~0xF) | MCB_STATUS_STARTED; //0x00001A30 - 0x00001A40
    if (status < SCE_ERROR_OK) 
        StopLoadCore();
    
    /* 
     * Call boot callback functions of the previously registered system 
     * modules.  A system module might won't have registered a boot callback
     * function. 
     */
    //0x00001A44 - 0x00001AA0
    for (i = 0; i < 4; i++) {
        if (i == 3) //0x00001A58
            g_loadCore.bootCallBacks = NULL; //0x00001A5C
        
         //0x00001A60 & 0x00001A78 - 0x00001A90
         for (j = 0; bootCallbacks[j].bootCBFunc != NULL; j++) {  
             if (((s32)(bootCallbacks[j].bootCBFunc) & 3) == i) { //0x00001A7C                   
                 bootCbFunc = (SceKernelBootCallbackFunction)(((u32)bootCallbacks[j].bootCBFunc) & ~0x3); //0x00001B18
                 if (i == 3) //0x00001B1C 
                     bootCbFunc(((void *)&bootCallbacks[j]) + 2, 1, NULL); //0x00001A80 & 0x00001B44    
                 else
                     bootCbFunc(bootCallbacks, 1, NULL); //0x00001B34              
             }
         }
         if (i == 2) { //0x00001A94
             //0x00001AE0 & 00001AF0 - 0x00001B14
             while (j > 0) {
                 j--; //0x00001AF0
                 if (((u32)bootCallbacks[j].bootCBFunc & 3) == 3) //0x00001B00
                     break;
                 
                 bootCallbacks[j].bootCBFunc = NULL; //0x00001B0C
             }
        }
    }   
}

//Subroutine LoadCoreForKernel_1999032F - Address 0x000024C0
s32 sceKernelLoadCoreLock(void)
{
    return loadCoreCpuSuspendIntr();
}

//Subroutine LoadCoreForKernel_B6C037EA - Address 0x000024DC
void sceKernelLoadCoreUnlock(s32 intrState)
{
    loadCoreCpuResumeIntr(intrState);
}

//Subroutine LoadCoreForKernel_2C60CCB8 - Address 0x000024F8
s32 sceKernelRegisterLibraryForUser(SceResidentLibraryEntryTable *libEntryTable)
{
    return doRegisterLibrary(libEntryTable, LIB_PRIVILEGE_LEVEL_USER);
}

//Subroutine LoadCoreForKernel_CB636A90 - Address 0x00002514
s32 sceKernelReleaseLibrary(SceResidentLibraryEntryTable *libEntryTable)
{    
    s32 intrState;
    s32 status;
    SceResidentLibrary *lib;
    SceResidentLibrary **prevLibSlotPtr;
    
    intrState = loadCoreCpuSuspendIntr();
    
    /* NOTE: FoundLibrary can return a NULL pointer, thus checking should be added. */
    prevLibSlotPtr = FoundLibrary(libEntryTable); //0x00002530
    lib = *prevLibSlotPtr;
    if (lib != NULL) { //0x00002540
        status = ReleaseLibEntCB(lib, prevLibSlotPtr); //0x00002568
        loadCoreCpuResumeIntr(intrState); //0x00002574
        return status;
    }
    
    loadCoreCpuResumeIntr(intrState);
    return SCE_ERROR_OK;  
}

//Subroutine LoadCoreForKernel_6ECFFFBA - Address 0x00002584
s32 sceKernelLinkLibraryEntriesForUser(SceStubLibraryEntryTable *stubLibEntryTable, u32 size)
{
    return doLinkLibraryEntries(stubLibEntryTable, size, LIB_PRIVILEGE_LEVEL_USER, 0);
}

//Subroutine LoadCoreForKernel_A481E30E - Address 0x000025A4
s32 sceKernelLinkLibraryEntriesWithModule(SceModule *mod, SceStubLibraryEntryTable *stubLibEntryTable, u32 size)
{
    return doLinkLibraryEntries(stubLibEntryTable, size, LIB_PRIVILEGE_LEVEL_USER, mod->countRegVal);
}

//Subroutine LoadCoreForKernel_1915737F - Address 0x000025CC
u32 sceKernelMaskLibraryEntries(void)
{
    return SCE_ERROR_OK;
}

//Subroutine LoadCoreForKernel_696594C8 - Address 0x000025D4
SceLoadCore *sceKernelQueryLoadCoreCB(void)
{
    return &g_loadCore;
}

//Subroutine LoadCoreForKernel_F976EF41 - Address 0x000025E0
s32 sceKernelSetBootCallbackLevel(SceKernelBootCallbackFunction bootCBFunc, u32 flag, s32 *status)
{
    if (g_loadCore.bootCallBacks != NULL) { //0x00002614
        g_loadCore.bootCallBacks->bootCBFunc = bootCBFunc + (flag & 3); //0x00002650
        g_loadCore.bootCallBacks->gp = sceKernelGetModuleGPByAddressForKernel((u32)bootCBFunc); //0x0000264C & 0x00002658
        g_loadCore.bootCallBacks += 1; //0x00002664
        g_loadCore.bootCallBacks->bootCBFunc = NULL; //0x0000266C
        
        return SCE_BOOT_CALLBACK_FUNCTION_QUEUED;
    }    
    s32 result = bootCBFunc((void*)1, 0, NULL); //0x0000261C     
    if (status != NULL) //0x00002624
        *status = result; //0x0000262C
     
    return SCE_ERROR_OK;   
}

//Subroutine LoadCoreForKernel_C8FF5EE5 - Address 0x00002670
u32 sceKernelLoadCoreMode(void)
{
    return SCE_ERROR_OK;
}

//sub_00002678
static void LoadCoreHeapStatic(void)
{
    StopLoadCore();
}

//sub_00002718
static s32 LoadCoreHeapDynamic(void)
{
    if (g_loadCore.loadCoreHeapId <= 0)
        g_loadCore.loadCoreHeapId = sceKernelCreateHeap(SCE_KERNEL_PRIMARY_KERNEL_PARTITION, LOADCORE_HEAP_SIZE, 1, 
                                                        LOADCORE_HEAP_NAME); //0x00002758
    return g_loadCore.loadCoreHeapId;
}

//sub_00002768 
static void UpdateCacheAllStatic(void)
{
    sceKernelIcacheClearAll(); //0x00002770 - sub_0000748C
    sceKernelDcacheWBinvAll(); //0x00002778 - sub_0000744C
    
}

//sub_0000278C
static void UpdateCacheAllDynamic(void)
{
    sceKernelIcacheInvalidateAll(); //0x00002794
    sceKernelDcacheWritebackInvalidateAll(); //0x0000279C
}

//sub_000027B0
static void UpdateCacheDataAllStatic(void)
{
    sceKernelDcacheWBinvAll(); //0x000027B8 - sub_0000744C
}

//sub_000027CC
static void UpdateCacheDataAllDynamic(void)
{
    sceKernelDcacheWritebackInvalidateAll(); //0x000027D4
}

//sub_000027E8
static void UpdateCacheRangeStatic(const void *addr __attribute__((unused)), u32 size __attribute__((unused)))
{
    sceKernelIcacheClearAll(); //0x000027F0 - sub_0000748C
    sceKernelDcacheWBinvAll(); //0x000027F8 - sub_0000744C
}

//sub_0000280C
static void UpdateCacheRangeDynamic(const void *addr, u32 size)
{
    sceKernelIcacheInvalidateRange(addr, size); //0x00002820
    sceKernelDcacheWritebackInvalidateRange(addr, size); //0x0000282C
}

//sub_00002848
static void UpdateCacheRangeDataStatic(const void *addr __attribute__((unused)), u32 size __attribute__((unused)))
{
    sceKernelDcacheWBinvAll(); //0x00002850 - sub_0000744C
}

//sub_00002864
static void UpdateCacheRangeDataDynamic(const void *addr, u32 size) 
{
    sceKernelDcacheWritebackInvalidateRange(addr, size);
}

//sub_00002880
/*
 * Register a resident library to the system.  When registering a 
 * library, several steps are performed.  
 *   1) We check if earlier versions of that library are already 
 *      registered and if we are allowed to register a newer library 
 *      version.
 *   2) In case the library exports its functions as syscalls,
 *      Loadcore's syscall table is updated and filled with masked
 *      addresses of the exported functions.
 *   3) We search for all unlinked stub libraries corresponding
 *      to the resident library and link them with the resident library.
 *      In case the library already has older versions registered in the
 *      system, update their stub libraries as well if they are not set 
 *      with the STUB_LIBRARY_EXCLUSIVE attribute.
 *   4) Insert the created resident library into a linked list
 *      of libraries having the same hash value.
 * 
 * Returns 0 on success.
 */
static s32 doRegisterLibrary(SceResidentLibraryEntryTable *libEntryTable, u32 isUserLib) 
{
    SceResidentLibrary *nLib = NULL;
    SceResidentLibrary *curLib = NULL;
    SceStubLibrary *curStubLib = NULL;
    SceStubLibrary *updatableStubLib = NULL;
    SceStubLibrary *nextStubLib = NULL;
    SceStubLibrary **stubLibPtr = NULL;
    SceStubLibrary *curUnlinkedStubLib = NULL;
    SceStubLibrary *prevUnlinkedStubLib = NULL;
    SceStubLibrary *nextUnlinkedStubLib = NULL;
    s32 status;
    u32 i;
    u32 index;  
    s32 intrState;
    s32 versionDiff;
    u32 libRegStatus;
    u32 random;
    s32 startAddr;
    u32 funcAddr;
    
    if (libEntryTable == NULL) //0x000028B8
        return SCE_ERROR_KERNEL_ILLEGAL_LIBRARY_HEADER;

    /*
     * Check for a privilege-level mismatch.  A user mode library cannot 
     * export its functions as syscalls.  
     */
    if (isUserLib && (libEntryTable->attribute & SCE_LIB_SYSCALL_EXPORT)) //0x000028C0 - 0x000028D4
        return SCE_ERROR_KERNEL_ILLEGAL_PERMISSION;
    
    intrState = loadCoreCpuSuspendIntr(); //0x000028D8
       
    nLib = NewLibEntCB(); //0x000028E0
    status = CopyLibEnt(nLib, libEntryTable, isUserLib); //0x000028F4
    if (status != SCE_ERROR_OK) { //0x000028FC
        FreeLibEntCB(nLib); //0x00002DC0       
        loadCoreCpuResumeIntr(intrState); //0x00002DC8
        return SCE_ERROR_KERNEL_ERROR;
    }
    
    index = getCyclicPolynomialHash(libEntryTable->libName, LOADCORE_CYCLIC_POLYNOMIAL_HASH_RADIAX, 
                                    LOADCORE_LIB_HASH_TABLE_SIZE);   
    //0x00002968 & 0x000029E4
    for (curLib = g_loadCore.registeredLibs[index]; curLib != NULL; curLib = curLib->next) {
         
        /*
         * Check if the library registration is needed.
         * Step 1: Check if a version of the to-be-registered library is
         *         already registered.
         */
         if (strcmp(nLib->libName, curLib->libName) != 0) //0x00002980
             continue; 
         
         /*
          * Step 2: Check if the privilege level differs for the 
          *         to-be-registered library and its already registered library
          *         version.
          * A library cannot have two or more instances running
          * in different privilege levels.
          */
         libRegStatus = LIB_REGISTRATION_DENIED;
         if (nLib->isUserLib == curLib->isUserLib) { //0x00002990
             //0x00002D94
             if (curLib->attribute & SCE_LIB_WEAK_EXPORT) {
                 /*
                  * Step 3: Register the library if it has a newer version than the
                  *         currently registered libraries.
                  */
                 versionDiff = ((nLib->version[1] << 8) | nLib->version[0]) - ((curLib->version[1] << 8) | curLib->version[0]); //0x00002DB0
                 libRegStatus = (versionDiff > 0) ? LIB_REGISTRATION_OK : LIB_REGISTRATION_NOT_NEEDED; //0x00002DBC                                 
             }               
         }
         if (libRegStatus == LIB_REGISTRATION_NOT_NEEDED) { //0x00002998
             FreeLibEntCB(nLib); //0x00002D84
             loadCoreCpuResumeIntr(intrState);               
             return SCE_ERROR_OK;
         } 
         if (libRegStatus != LIB_REGISTRATION_OK) { //0x000029AC
             FreeLibEntCB(nLib); //0x00002D68
             loadCoreCpuResumeIntr(intrState); 
             return SCE_ERROR_KERNEL_LIBRARY_ALREADY_EXISTS;
         }
         
         //0x000029B0 - 0x000029DC                                
         stubLibPtr = &curLib->stubLibs; //0x000029B0
         for (curStubLib = curLib->stubLibs, curLib->stubLibs = NULL; curStubLib; curStubLib = nextStubLib) {
             nextStubLib = curStubLib->next; //0x000029CC
              if ((curStubLib->status & STUB_LIBRARY_EXCLUSIVE_LINK) == 0) { //0x000029C8
                   curStubLib->next = updatableStubLib; //0x000029D0
                   updatableStubLib = curStubLib; //0x000029D4
              }
               else {
                  *stubLibPtr = curStubLib; //0x00002D58
                  stubLibPtr = &curStubLib->next; //0x00002D5C
                  curStubLib->next = NULL;
               }             
         }        
    }
    
    if (nLib->attribute & SCE_LIB_SYSCALL_EXPORT) { //0x000029F4
        /*
         * Step 1: Initialize Loadcore's SYSCALL table if not already being 
         *         created.
         */
        if (g_loadCore.sysCallTable == NULL) { //0x00002A08
            status = SyscallTableInit(g_loadCore.sysCallTableSeed, &g_loadCore.sysCallTable); //0x00002A10
            if (status < SCE_ERROR_OK) { //0x00002A18
                FreeLibEntCB(nLib); //0x00002D40
                loadCoreCpuResumeIntr(intrState);
                return status;
            }
        }
        /*
         * Step 2: Allocate a SYSCALL table entry for the exported functions.
         *         The exported entries are added to a random number between 
         *         1 - 4, representing the number of entries to be allocated.
         * 
         *         The "random" number is read from coprocessor 0's Count register 
         *         (reg number 9) which is a timer being increased at a fixed rate.
         */        
        random = pspCop0StateGet(COP0_STATE_COUNT); //0x00002A30
        nLib->extraExportEntries = (random & 0x3) + 1; 
        startAddr = AllocSysTable(nLib->stubCount + nLib->extraExportEntries); //0x00002A44
        nLib->sysTableEntry = g_loadCore.sysCallTableSeed + startAddr;
        
        random = pspCop0StateGet(COP0_STATE_COUNT); //0x00002A5C
        if (nLib->stubCount == 0) //0x00002A68
            nLib->sysTableEntryStartIndex = 0;
        else
            nLib->sysTableEntryStartIndex = random % nLib->stubCount; //0x00002A70 & 0x00002A80
        
        /* Error allocating SYSCALL table. */
        if (startAddr < 0) { //0x00002A84
            FreeLibEntCB(nLib); //0x00002D30
            loadCoreCpuResumeIntr(intrState);
            return startAddr;
        }
        
        /*
         * Step 3: Fill in the SYSCALL table with the addresses of the exported 
         *         functions.  Every function address (a kernel address) is masked
         *         to a user mode address to signalize that the particular function
         *         is not linked and then stored into a table slot.
         *         Every reserved table slot which has no equivalent exported function 
         *         is filled with a function causing the system to freeze.
         */
        //0x00002A98 - 0x00002B1C        
        for (i = 0; i < (nLib->stubCount + nLib->extraExportEntries); i++) {
             if (i < nLib->stubCount) //0x00002AC4
                 funcAddr = MAKE_USER_ADDRESS(nLib->entryTable[nLib->stubCount + nLib->vStubCount + i]); //0x00002AE4
             else
                 funcAddr = (u32)sub_00003D84;
                
             if ((nLib->sysTableEntryStartIndex + i) >= (nLib->stubCount + nLib->extraExportEntries)) //0x00002AF8
                 index = (nLib->sysTableEntryStartIndex + i) % (nLib->stubCount + nLib->extraExportEntries); //0x00002B0C
             else
                index = nLib->sysTableEntryStartIndex + i; //0x00002AFC

             g_loadCore.sysCallTable->syscalls[index + startAddr] = (void *)funcAddr; //0x00002B20
        }
    }
        
    //0x00002B2C - 0x00002B74
    /*
     * Find all unlinked loaded stub libraries dependant on the
     * to-be-registered library.  Unlink them from the linked list
     * containing unlinked stub libraries living in memory and create a 
     * new linked list of all these stub libraries going to be linked 
     * with the resident library to resolve external references.  Integrate
     * an unlinked stub library into the newly created linked list.
     */
    for (curUnlinkedStubLib = g_loadCore.unLinkedStubLibs; curUnlinkedStubLib; prevUnlinkedStubLib = curUnlinkedStubLib, 
       curUnlinkedStubLib = nextUnlinkedStubLib) {
         if (strcmp(curUnlinkedStubLib->libName, nLib->libName) != 0) { //0x00002B40
             nextUnlinkedStubLib = curUnlinkedStubLib->next;
             continue;
         }
         if (nLib->isUserLib && !curUnlinkedStubLib->isUserLib) { //0x00002B4C & 0x00002B58
             nextUnlinkedStubLib = curUnlinkedStubLib->next;
             continue;
         }                            
         prevUnlinkedStubLib->next = curUnlinkedStubLib->next;
         nextUnlinkedStubLib = prevUnlinkedStubLib->next;         
         
         curUnlinkedStubLib->next = updatableStubLib;
         updatableStubLib = curUnlinkedStubLib;    
    }         
        
    nLib->stubLibs = NULL;
    //0x00002B7C - 0x00002BE0 
    /*
     * Link the stub libraries of the newly created linked list with the
     * resident library.
     */
    for (; updatableStubLib; updatableStubLib = updatableStubLib->next) {
         aLinkClient(updatableStubLib, nLib); //0x00002B9C
         if (updatableStubLib->vStubCount == 0) //0x00002BA8
             g_UpdateCacheRange(updatableStubLib->stubTable, updatableStubLib->stubCount << 3); //0x00002D18
         else
             g_UpdateCacheAll(); //0x00002BB4

         if (strcmp(MEMLMD_MODULE_NAME, updatableStubLib->libName) == 0) { //0x00002BC4
             g_ToolBreakMode = sceKernelDipsw(30); //0x00002CA4
             if (g_ToolBreakMode != 0 && sceKernelDipsw(21) == 1) //0x00002CB0 & 0x00002CF4                
                 g_ToolBreakMode = 0;
                    
             sceKernelSetPrimarySyscallHandler(SCE_PRIMARY_SYSCALL_HANDLER_SYSCALL_ID, (void*)sceLoadCorePrimarySyscallHandler); //0x00002CBC
             sceKernelSetGetLengthFunction(NULL);
             sceKernelSetPrepareGetLengthFunction(NULL);
             sceKernelSetSetMaskFunction(NULL);
             sceKernelSetCompareSubType(NULL);
             sceKernelSetCompareLatestSubType(NULL);
         }
         updatableStubLib->next = nLib->stubLibs; //0x00002BD8
         nLib->stubLibs = updatableStubLib;
    }
    nLib->attribute &= ~STUB_LIBRARY_LINKED; //0x00002BF4
    
    /* 
     * Add the created resident library into a linked list of libraries 
     * having the same hash value.
     */
    index = getCyclicPolynomialHash(libEntryTable->libName, LOADCORE_CYCLIC_POLYNOMIAL_HASH_RADIAX, 
                                    LOADCORE_LIB_HASH_TABLE_SIZE);    
    nLib->next = g_loadCore.registeredLibs[index];
    g_loadCore.registeredLibs[index] = nLib;
    
    loadCoreCpuResumeIntr(intrState);   
    return SCE_ERROR_OK;
}

//Subroutine sub_00002DDC - Address 0x00002DDC
/*
 * Delete a registered resident library from the system. 
 * The corresponding stub libraries' table entries with the
 * attribute SCE_LIB_WEAK_IMPORT are unlinked.  The allocated 
 * SceResidentLibrary object to hold the resident library's entry 
 * table is freed/deallocated.  Note that releasing a resident 
 * library will fail if there are active stub libraries not using
 * the SCE_LIB_WEAK_IMPORT attribute. 
 * 
 * The function stubs of the corresponding stub libraries in memory
 * will be changed to:
 *    stubAddr_0: SYSCALL_NOT_LINKED # 0x0000054C
 *    stubAddr_4: NOP # 0x00000000
 * 
 * Returns 0 on success. 
 */
static s32 ReleaseLibEntCB(SceResidentLibrary *lib, SceResidentLibrary **prevLibSlot)
{
    u32 i;
    SceStubLibrary *curStubLib = NULL;
    SceStubLibrary *nextStubLib = NULL;
    SceStubLibrary **stubLibPtr = NULL;
    
    stubLibPtr = &lib->stubLibs; //0x00002E18

    if (lib->stubLibs != NULL) { //0x00002E08
        for (curStubLib = *stubLibPtr; curStubLib; curStubLib = *stubLibPtr) { //0x00002E24 - 0x00002EC0  
             if (!(curStubLib->attribute & SCE_LIB_WEAK_IMPORT)) //0x00002E2C
                 stubLibPtr = &curStubLib->next; //0x00002E30
                 continue;
            
             nextStubLib = curStubLib->next; //0x00002E38
             curStubLib->next = g_loadCore.unLinkedStubLibs; //0x00002E3C
             g_loadCore.unLinkedStubLibs = curStubLib; //0x00002E44

             //0x00002E58 - 0x00002E70
             for (i = 0; i < curStubLib->stubCount; i++) {
                  curStubLib->stubTable[i].sc.returnAddr = SYSCALL_NOT_LINKED;
                  curStubLib->stubTable[i].sc.syscall = NOP;
             }
             curStubLib->status &= ~STUB_LIBRARY_LINKED; //0x00002E84
             g_UpdateCacheRange(curStubLib->stubTable, curStubLib->stubCount * sizeof(SceStub)); //0x00002E8C
             
             aUnLinkVariableStub(lib, (SceStubLibraryEntryTable *)curStubLib->libName, curStubLib->isUserLib); //0x00002EA0
             if (curStubLib->vStubCount != 0) //0x00002EB0
                 g_UpdateCacheAll(); //0x00002FBC
           
             *stubLibPtr = nextStubLib; //0x00002EB8
        }
        /* 
         * Verify if there are still stub libraries linked with specified
         * resident library.  If this is the case, the library cannot be 
         * released.
         */
        if (lib->stubLibs != NULL) //0x00002ED0
            return SCE_ERROR_KERNEL_LIBRARY_IN_USE;    
    }
    curStubLib = lib->stubLibs; //0x00002EE0
    
    *prevLibSlot = lib->next; //0x00002EE8
    lib->stubLibs = NULL; //0x00002EEC
    lib->next = NULL; //0x00002EF4
     
    if (lib->attribute & SCE_LIB_SYSCALL_EXPORT) //0x00002EF0
        FreeSysTable(lib->sysTableEntry - g_loadCore.sysCallTableSeed); //0x00002FAC
    
    FreeLibEntCB(lib); //0x00002F00
    
    /* Note: curLibStub should always be NULL here. */
    for (; curStubLib; curStubLib = curStubLib->next) {
         if (aLinkLibEntries(curStubLib) == SCE_ERROR_OK) //0x00002F18
             continue;
         
         //0x00002F20 - 0x00002F44
         for (i = 0; i < curStubLib->stubCount; i++) {
              curStubLib->stubTable[i].dc.call = JR_RA;
              curStubLib->stubTable[i].dc.delaySlot = NOP;
         }
         curStubLib->next = g_loadCore.unLinkedStubLibs->next;
         curStubLib->status = (curStubLib->status & ~STUB_LIBRARY_LINKED) | 0x4; //0x00002F64
         g_loadCore.unLinkedStubLibs->next = curStubLib; //0x00002F68      
    }
    return SCE_ERROR_OK;
}

//sub_00002FCC
/* Find a registered resident library.  This functions returns
 * the address of a member of loadCore's linked-list of registered 
 * resident libraries.  The address is
 *    a) the address of the slot of the previous library or
 *    b) the address of the first slot.  This is the case when
 *       the library searched for is the first one in the linked-list.    
 */
static SceResidentLibrary **FoundLibrary(SceResidentLibraryEntryTable *libEntryTable)
{
    u32 index;
    SceResidentLibrary *curLib = NULL; 
    SceResidentLibrary **prevLib = NULL;
    
    index = getCyclicPolynomialHash(libEntryTable->libName, LOADCORE_CYCLIC_POLYNOMIAL_HASH_RADIAX, 
                                                            LOADCORE_LIB_HASH_TABLE_SIZE);
    for (curLib = g_loadCore.registeredLibs[index], prevLib = &g_loadCore.registeredLibs[index]; curLib; 
       prevLib = &curLib->next, curLib = curLib->next) {
         if (curLib->libEntryTable == libEntryTable) 
             return prevLib;
    }    
    return NULL;
}

//sub_00003080
/*
 * Link stub library entry tables with their corresponding resident libraries.
 * 
 * Returns 0 on success.
 */
static s32 doLinkLibraryEntries(SceStubLibraryEntryTable *stubLibEntryTable, u32 size, u32 isUserMode, 
                                u32 arg4 __attribute__((unused)))
{
    u32 i;
    s32 status;
    u32 dupStubLib; /* SCE_TRUE = stub library duplicated */
    void *curStubTable;
    void *stubTableMemRange;
    SceStubLibrary *stubLib;
    SceStubLibrary *curUnlinkedStubLib;
    
    stubTableMemRange = stubLibEntryTable + size; //0x00003088
    for (curStubTable = stubLibEntryTable; curStubTable < stubTableMemRange; 
       curStubTable += ((SceStubLibraryEntryTable *)curStubTable)->len * sizeof(u32)) {       
         stubLib = NewLibStubCB(); //0x000030D8
         CopyLibStub(stubLib, curStubTable, isUserMode); //0x000030EC
         if (CopyLibStub(stubLib, curStubTable, isUserMode) != SCE_ERROR_OK) { //0x000030F4 
             FreeLibStubCB(stubLib); //0x00003230
             return SCE_ERROR_KERNEL_ERROR; 
         }
         
         /* libStub is not free. */
         if (stubLib->status & (STUB_LIBRARY_LINKED | STUB_LIBRARY_EXCLUSIVE_LINK | 0x4)) //0x00003104
             continue;
         
         status = aLinkLibEntries(stubLib); //0x00003158
         if (status == SCE_ERROR_OK) //0x00003160
             continue;
         
         /*
          * The stub library cannot be linked with the resident library
          * if that lib does not export functions/variables.  If the resident
          * lib does exporting, "status" contains another error, indicating
          * that linking failed.  If the stub library has the attribute 
          * SCE_LIB_WEAK_IMPORT, we ignore that linking failure and proceed.
          */
         if (status == EXPORT_LIB_CANNOT_BE_LINKED || !(stubLib->attribute & SCE_LIB_WEAK_IMPORT)) { //0x00003168 & 0x00003178
             sceKernelUnlinkLibraryEntries(stubLibEntryTable, size); //0x00003214
             FreeLibStubCB(stubLib); //0x0000321C
             return SCE_ERROR_KERNEL_LIBRARY_NOT_FOUND;
         }
         
         //0x00003180 - 0x000031A4
         /*
          * Verify if the current stub library, which failed at being linked, 
          * is already placed into the loadCore's linked-list of unlinked
          * stub libraries.  If this is true, we free the current stub library's
          * instance.  Otherwise, it is added to the linked-list.
          */
         dupStubLib = SCE_FALSE;
         for (curUnlinkedStubLib = g_loadCore.unLinkedStubLibs; curUnlinkedStubLib; 
            curUnlinkedStubLib = curUnlinkedStubLib->next) {              
              if (curUnlinkedStubLib->libStubTable == stubLib->libStubTable) { //0x00003194
                  FreeLibStubCB(stubLib); //0x00003204
                  dupStubLib = SCE_TRUE; //0x0000320C
              }
         }
         if (dupStubLib == SCE_FALSE) {
             stubLib->next = g_loadCore.unLinkedStubLibs; //0x000031B4
             g_loadCore.unLinkedStubLibs = stubLib; //0x000031BC
             
             //0x000031B8 - 0x000031D8
             for (i = 0; i < stubLib->stubCount; i++) {
                  stubLib->stubTable[i].sc.returnAddr = SYSCALL_NOT_LINKED;
                  stubLib->stubTable[i].sc.syscall = NOP;
             }
             
             stubLib->status &= ~STUB_LIBRARY_LINKED; //0x000031EC
             g_UpdateCacheRange(stubLib->stubTable, stubLib->stubCount * sizeof(SceStub)); //0x000031F4
         }
    }
    return SCE_ERROR_OK;
}          

//sub_00003244
/*
 * Unlink a stub library from its corresponding resident library.
 * In addition, we remove the stub library from the system.
 * 
 * The function stubs (in memory) of the stub library will be 
 * changed to:
 *    stubAddr_0: JR $ra # 0x03E00008 - a simple return to the caller
 *    stubAddr_4: NOP # 0x00000000 - delay slot
 * 
 * Returns 0 on success.
 */
static s32 UnLinkLibraryEntry(SceStubLibraryEntryTable *stubLibEntryTable)
{
    u32 i;
    u32 stubLibUnlinked;
    SceResidentLibrary *curLib;
    SceStubLibrary *curStubLib;
    SceStubLibrary *firstStubLib;
    SceStubLibrary *curStubLib2;
    SceStubLibrary *prevStubLib;
    SceStubLibrary *curUnlinkedStubLib;
    SceStubLibrary **prevUnlinkedStubLibPtr;    
    
    i = getCyclicPolynomialHash(stubLibEntryTable->libName, LOADCORE_CYCLIC_POLYNOMIAL_HASH_RADIAX, 
                                LOADCORE_LIB_HASH_TABLE_SIZE);   
    //0x000032BC - 0x000032F0
    for (curLib = g_loadCore.registeredLibs[i]; curLib; curLib = curLib->next) {
         //0x000032D0 - 0x000032E4
         for (curStubLib = curLib->stubLibs, firstStubLib = curStubLib; curStubLib; curStubLib = curStubLib->next) {
              if (curStubLib->libStubTable != stubLibEntryTable) //0x000032D4
                  continue;
              
              /* 
               * In case the found registered stub library is the first member 
               * of the linked-list of stub libraries of the corresponding 
               * resident library, we can simply overwrite the stub library with
               * a pointer to the next stub library.  Otherwise, we have to find
               * the previous stub library in the linked-list and overwrite its
               * "next" pointer to point to the next stub library of the found
               * stub library.
               */
              if (firstStubLib == curStubLib) { //0x00003384
                  curLib->stubLibs = curStubLib->next; //0x0000340C
              }
              else {
                  stubLibUnlinked = SCE_FALSE;
                  //0x0000338C - 0x000033AC
                  for (curStubLib2 = firstStubLib->next, prevStubLib = firstStubLib; curStubLib2; 
                     prevStubLib = curStubLib2, curStubLib2 = curStubLib2->next) {
                       if (curStubLib2 == curStubLib) { //0x0000339C
                           prevStubLib->next = curStubLib->next; //0x000033BC
                           curStubLib->next = NULL; //0x000033C0
                           stubLibUnlinked = SCE_TRUE;
                           break;
                       }
                  }
                  if (stubLibUnlinked == SCE_FALSE)
                      return SCE_ERROR_KERNEL_ERROR; //0x000033B4
              }
              curStubLib->status &= ~(STUB_LIBRARY_LINKED | STUB_LIBRARY_EXCLUSIVE_LINK | 0x4); //0x000033CC
              //0x000033D0 - 0x000033F4
              for (i = 0; i < curStubLib->stubCount; i++) {
                   curStubLib->stubTable[i].dc.call = JR_RA; //0x000033E8
                   curStubLib->stubTable[i].dc.delaySlot = NOP; //0x000033EC
              }
              FreeLibStubCB(curStubLib); //0x000033F8
              return SCE_ERROR_OK;
         }
    }   
    //0x000032F8 - 0x0000331C
    /*
     * We will only go into this block when there is no corresponding
     * resident library for the to-be-unlinked stub library.  We search
     * for our stub library in LoadCore's linked-list of unlinked stub 
     * libraries and remove our stub lib from that list and freeing it.
     */
    for (curUnlinkedStubLib = g_loadCore.unLinkedStubLibs, prevUnlinkedStubLibPtr = &g_loadCore.unLinkedStubLibs; 
       curUnlinkedStubLib; prevUnlinkedStubLibPtr = &curUnlinkedStubLib->next, curUnlinkedStubLib = curUnlinkedStubLib->next) {
        if (curUnlinkedStubLib->libStubTable != stubLibEntryTable) //0x00003308
            continue;
        
        curUnlinkedStubLib->status &= ~(STUB_LIBRARY_LINKED | STUB_LIBRARY_EXCLUSIVE_LINK | 0x4);
        *prevUnlinkedStubLibPtr = curUnlinkedStubLib->next; //0x0000334C
        
        //0x000033D0 - 0x000033F4
        for (i = 0; i < curUnlinkedStubLib->stubCount; i++) {
             curUnlinkedStubLib->stubTable[i].dc.call = JR_RA; //0x000033E8
             curUnlinkedStubLib->stubTable[i].dc.delaySlot = NOP; //0x000033EC
        }       
        FreeLibStubCB(curUnlinkedStubLib); //0x000033F8   
    }
    return SCE_ERROR_OK;         
}

//sub_00003410
/*
 * Update module export entries via an entry table of a resident library.
 * 
 * Returns 0.
 */
static s32 ProcessModuleExportEnt(SceModule *mod, SceResidentLibraryEntryTable *lib)
{
    u32 i;
    SceModuleEntryThread *entryThread;
    
    //0x00003410 - 0x0000348C
    for (i = 0; i < lib->stubCount; i++) {        
         switch (lib->entryTable[i]) { 
         case NID_MODULE_REBOOT_PHASE: //0x00003450
             mod->moduleRebootPhase = (SceKernelRebootKernelThreadEntry)lib->entryTable[lib->vStubCount + lib->stubCount + i]; //0x00003678
             break;
         case NID_MODULE_BOOTSTART: //0x000035DC
             mod->moduleBootstart = (SceKernelThreadEntry)lib->entryTable[lib->vStubCount + lib->stubCount + i]; //0x00003658
             break;
         case NID_MODULE_START: //0x00003614
             mod->moduleStart = (SceKernelThreadEntry)lib->entryTable[lib->vStubCount + lib->stubCount + i]; //0x00003658
             break;
         case NID_MODULE_STOP: //0x000035EC
             mod->moduleStop = (SceKernelThreadEntry)lib->entryTable[lib->vStubCount + lib->stubCount + i]; //0x00003610
             break;
         case NID_MODULE_REBOOT_BEFORE: //0x00003478
             mod->moduleRebootBefore = (SceKernelRebootKernelThreadEntry)lib->entryTable[lib->vStubCount + lib->stubCount + i]; //0x000035D4
             break;
         }         
    }
    //0x00003490 - 0x000034F8
    for (i = 0; i < lib->vStubCount; i++) {
         switch (lib->entryTable[lib->vStubCount + i]) {
         case NID_MODULE_STOP_THREAD_PARAM: //0x000034D0
             entryThread = (SceModuleEntryThread *)lib->entryTable[2 * lib->stubCount + lib->vStubCount + i];
             mod->moduleStopThreadPriority = entryThread->initPriority; //0x000035A4
             mod->moduleStopThreadStacksize = entryThread->stackSize; //0x000035AC
             mod->moduleStopThreadAttr = entryThread->attr; //0x000035B8
             break;
         case NID_MODULE_REBOOT_BEFORE_THREAD_PARAM: //0x00003544
             entryThread = (SceModuleEntryThread *)lib->entryTable[2 * lib->stubCount + lib->vStubCount + i];
             mod->moduleRebootBeforeThreadPriority = entryThread->initPriority; //0x0000356C
             mod->moduleRebootBeforeThreadStacksize = entryThread->stackSize; //0x00003574
             mod->moduleRebootBeforeThreadAttr = entryThread->attr; //0x00003580
             break;               
         case NID_MODULE_START_THREAD_PARAM: //0x000034E0
             entryThread = (SceModuleEntryThread *)lib->entryTable[2 * lib->stubCount + lib->vStubCount + i];
             mod->moduleStartThreadPriority = entryThread->initPriority; //0x00003520
             mod->moduleStartThreadStacksize = entryThread->stackSize; //0x00003528
             mod->moduleStartThreadAttr = entryThread->attr; //0x00003530
             break;
        }
    }
    return SCE_ERROR_OK;
}

//sub_000039B4
/*
 * Link function/variable stubs of a stub library with the
 * corresponding exports of a registered resident library.
 * Exported kernel functions to user stub libraries will be linked
 * as SYSCALLS.  Same privilege-level exports, such as Kernel-Kernel 
 * or User-User are linked via direct jumps.
 * 
 * User to User export/Kernel to Kernel export:
 *    stubAddr_0: J $exportedFuncAddr
 *    stubAddr_4: NOP # 0x00000000
 * 
 * Kernel to User export:
 *    stubAddr_0: JR $ra # 0x03E00008
 *    stubAddr_4: SYSCALL_X # X is the number of the syscall.
 * 
 * Function stub has no corresponding export function:
 *    stubAddr_0: SYSCALL_NOT_LINKED # 0x0000054C
 *    stubAddr_4: NOP # 0x00000000
 */
static s32 aLinkClient(SceStubLibrary *stubLib, SceResidentLibrary *lib)
{   
    u32 i;
    u32 j;
    s32 pos;
    u32 status;
    u32 *syscalls;
    
    //0x00003A0C - 0x00003ADC
    for (i = 0; i < stubLib->stubCount; i++) {
         pos = search_nid_in_entrytable(lib, stubLib->nidTable[i], 0xFFFFFFFF, FUNCTION_NID_SEARCH); //0x00003A44
         if (pos < 0) { //0x00003A4C
             stubLib->stubTable[i].sc.syscall = NOP; //0x00003BAC
             stubLib->stubTable[i].sc.returnAddr = SYSCALL_NOT_LINKED; //0x00003BB4
             continue;
         }
         if (!(lib->attribute & SCE_LIB_SYSCALL_EXPORT)) { //0x00003A64
             stubLib->stubTable[i].dc.call = MAKE_JUMP(lib->entryTable[lib->vStubCount + lib->stubCount + pos]); //0x00003B98
             continue;
         }
         if (lib->stubCount == 0) { //0x00003A70
             j = lib->sysTableEntry + pos; //0x00003B68
         }
         else {
             j = (lib->sysTableEntryStartIndex + pos) % (lib->stubCount + lib->extraExportEntries); //0x00003A88 & 0x00003A98
             j += lib->sysTableEntry; //0x00003A9C
         }
         syscalls = (u32 *)g_loadCore.sysCallTable->syscalls - g_loadCore.sysCallTableSeed; //0x00003A10
         if ((s32)syscalls[j] >= 0) //0x00003AAC
             syscalls[j] = MAKE_KERNEL_ADDRESS(syscalls[j]); //0x00003AB0
         
         stubLib->stubTable[i].sc.returnAddr = JR_RA; //0x00003AC8
         stubLib->stubTable[i].sc.syscall = MAKE_SYSCALL(j); //0x00003AC4
    }
    if (!(lib->attribute & SCE_LIB_SYSCALL_EXPORT)) { //0x00003AE8
        status = aLinkVariableStub(lib, (SceStubLibraryEntryTable *)&stubLib->libName, stubLib->isUserLib); //0x00003B38
        if (status != 0) //0x00003B44
            g_UpdateCacheAll(); //0x00003B54
    }
    stubLib->status = (stubLib->status & ~0x4) | STUB_LIBRARY_LINKED; //0x00003B04
    return SCE_ERROR_OK;
}

//sub_00003BB8
/*
 * Dynamically link a stub library with its corresponding registered
 * resident library.  A stub library has to meet a few conditions in
 * order to be linked successfully.
 * 
 * 1) The stub library's version has to be less than or equal
 *    to the version of the resident library.
 * 
 * 2) The resident library cannot have the SCE_LIB_NOLINK_EXPORT
 *    attribute set.
 * 
 * 3) In case the resident library is a kernel library and the stub
 *    library is a user library, the resident library has to export
 *    its functions via syscalls.  That said, the resident library
 *    must have the SCE_LIB_SYSCALL_EXPORT attribute set.
 * 
 * 4) In case both the resident library and the stub library linked with
 *    are kernel libraries, the resident library cannot have the attribute
 *    SCE_LIB_SYSCALL_EXPORT set.
 * 
 * 5) In case the resident library is a user library, the stub library 
 *    has to be a user library as well.
 * 
 * Returns 0 on success.
 */
static s32 aLinkLibEntries(SceStubLibrary *stubLib)
{
    u32 i;
    s32 status;
    SceResidentLibrary *curLib;
    SceStubLibrary *curStubLib;
    
    status = SCE_ERROR_KERNEL_LIBRARY_NOT_FOUND;
    
    i = getCyclicPolynomialHash(stubLib->libName, LOADCORE_CYCLIC_POLYNOMIAL_HASH_RADIAX, 
                                                  LOADCORE_LIB_HASH_TABLE_SIZE); 
    //0x00003C48 - 0x00003C5C
    for (curLib = g_loadCore.registeredLibs[i]; curLib; curLib = curLib->next) {
         if (curLib->attribute & SCE_LIB_AUTO_EXPORT) //0x00003C4C
             continue;
         
         if (strcmp(stubLib->libName, curLib->libName) != 0) //0x00003C88
             continue;
         
         if ((((curLib->version[1] << 8) | curLib->version[0]) - ((stubLib->version[1] << 8) | stubLib->version[0])) < 0) //0x00003C9C
             continue;
                 
         status = EXPORT_LIB_CANNOT_BE_LINKED; //0x00003CB4
         if (curLib->attribute & SCE_LIB_NOLINK_EXPORT) //0x00003CB0
             continue;
         
         if (!curLib->isUserLib) { //0x00003CBC
             if (stubLib->isUserLib) { //0x00003D50
                 status = KERNEL_DIRECT_EXPORT_LIB_CANNOT_BE_LINKED_WITH_USER_STUB_LIB; //0x00003D5C
                 if (!(curLib->attribute & SCE_LIB_SYSCALL_EXPORT)) //0x00003D58
                     continue;
             }
             if (!curLib->isUserLib) { //0x00003D60
                 if (!stubLib->isUserLib) { //0x00003D6C
                     status = KERNEL_SYSCALL_EXPORT_LIB_CANNOT_BE_LINKED_WITH_KERNEL_STUB_LIB; //0x00003D78
                     if (curLib->attribute & SCE_LIB_SYSCALL_EXPORT) //0x00003D74
                         continue;
                 }
             }
         }
         else { //0x00004800
             status = USER_EXPORT_LIB_CANNOT_BE_LINKED_WITH_KERNEL_STUB_LIB;
             if (!stubLib->isUserLib) //0x00003CC8
                 continue;
         }
         
         //0x00003CD0 - 0x00003CF4
         /*
          * Check if the specified stub library has already been linked.
          * If that is not the case, proceed with linking.
          */
         for (curStubLib = curLib->stubLibs; curStubLib; curStubLib = curStubLib->next) {
              if (curStubLib->libStubTable == stubLib->libStubTable) //0x00003CE4
                  return SCE_ERROR_OK;
         }
         aLinkClient(stubLib, curLib); //0x00003CFC
         if (stubLib->vStubCount == 0) //0x00003D08
             g_UpdateCacheRangeData(stubLib->stubTable, stubLib->stubCount * sizeof(SceStub)); //0x00003D40         
         else
             g_UpdateCacheDataAll(); //0x00003D18

         stubLib->next = curLib->stubLibs; //0x00003D28
         curLib->stubLibs = stubLib; //0x00003D2C
         
         return SCE_ERROR_OK;
    }  
    return status;
}

//Subroutine LoadCoreForKernel_FC47F93A - Address 0x00003D90
s32 sceKernelCheckPspConfig(u8 *file, u32 size) 
{
    s32 status;
    u32 newSize;
    PspHeader *pspHdr = NULL;
    
    pspHdr = (PspHeader *)file;
            
    if (((pspHdr->magic[0] << 24) | (pspHdr->magic[1] << 16) | (pspHdr->magic[2] << 8) | pspHdr->magic[3]) != PSP_MAGIC) //0x00003DB8
        return SCE_KERNEL_VALUE_UNITIALIZED;
    
    if (g_prepareGetLengthFunc != NULL && g_prepareGetLengthFunc(file, size) != 0) //0x00003DE8 & 0x00003DF8
        return SCE_KERNEL_VALUE_UNITIALIZED;
            
    status = CheckLatestSubType(file); //0x00003E00
    if (status == 0) //0x00003E08
        return SCE_KERNEL_VALUE_UNITIALIZED;
    
    if (g_setMaskFunc == NULL) //0x00003E1C
        return SCE_ERROR_KERNEL_LIBRARY_IS_NOT_LINKED;
    
    status = g_setMaskFunc(0, HWPTR(0xBFC00200)); //0x00003E2C
    if (status != 0) //0x00003E34
        return SCE_KERNEL_VALUE_UNITIALIZED;
    
    if (g_getLengthFunc == NULL) { //0x00003E44
        /* Decrypt a module. */
        status = memlmd_EF73E85B(file, size, &newSize); //0x00003E70
    }
    else
        status = g_getLengthFunc(file, size, &newSize); //0x00003E50
    
    if (status != 0)
        return SCE_KERNEL_VALUE_UNITIALIZED;
    
    return newSize;
}

//Subroutine sub_00003E80 - Address 0x00003E80
s32 CheckLatestSubType(u8 *file)
{
    u32 subType;
    s32 status;
    u8 *tag;
    
    tag = (u8 *)&((PspHeader *)file)->tag; 
    subType = tag[0] << 24 | tag[1] << 16 | tag[2] << 8 | tag[3]; //0x00003E90 - 0x00003EB8   
    if (g_compareLatestSubType == NULL) //0x00003EC4
        status = memlmd_9D36A439(subType); //0x00003ED8
    else
        status = g_compareLatestSubType(subType); //0x00003EC4
   
    return status;
}

//Subroutine LoadCoreForKernel_B27CC244 - Address 0x00003EE8
s32 sceKernelLoadRebootBin(u8 *file, u32 size) 
{
    u32 newSize;
    s32 status;
    SceSysmemPartitionInfo partInfo;   
    
    partInfo.size = sizeof(SceSysmemPartitionInfo); //0x00003F10
    status = sceKernelQueryMemoryPartitionInfo(SCE_KERNEL_PRIMARY_KERNEL_PARTITION, &partInfo); //0x00003F0C
    if (status < SCE_ERROR_OK)
        return SCE_KERNEL_VALUE_UNITIALIZED; //0x00003F14
    
    if ((u32)file < partInfo.startAddr) //0x00003F24
        return SCE_KERNEL_VALUE_UNITIALIZED;
    
    if ((partInfo.startAddr + partInfo.memSize) < (u32)(file + size)) //0x00003F38
        return SCE_KERNEL_VALUE_UNITIALIZED;
    
    status = CheckLatestSubType(file); //0x00003F58
    if (status == 0) //0x00003F64
        return SCE_KERNEL_VALUE_UNITIALIZED;
    
    status = memlmd_F26A33C3(0, HWPTR(0xBFC00200)); //0x00003F70
    if (status != SCE_ERROR_OK) //0x00003F84
        return SCE_KERNEL_VALUE_UNITIALIZED;
    
    status = memlmd_CF03556B(file, size, &newSize); //0x00003F8C
    if (status != SCE_ERROR_OK) //0x00003F94
        return status;
    
    return memlmd_2F3D7E2D(); //0x00003FA4
}

//sub_0000367C
/* 
 * Copy a resident library's entry table to the corresponding
 * resident library object registered to the system.  This library
 * is then inserted into a linked-list of registered resident
 * libraries with the same hash value.
 * 
 * Returns 0 on success.
 */
static s32 CopyLibEnt(SceResidentLibrary *lib, SceResidentLibraryEntryTable *libEntryTable, u32 isUserLib) 
{
    u32 *addr;
    u32 *addr2;
    s32 len;
    u32 exports;
    
    /* Protect Kernel memory from User Mode. */
    if (isUserLib && (IS_KERNEL_ADDR(libEntryTable) || IS_KERNEL_ADDR(libEntryTable->libName) || 
      IS_KERNEL_ADDR(libEntryTable->entryTable))) //0x00003698        
        return SCE_KERNEL_VALUE_UNITIALIZED;
      
    addr = (u32 *)g_hashinfo;
    addr2 = (u32 *)g_hashinfo;
    
    lib->isUserLib = isUserLib;
    lib->libEntryTable = libEntryTable; //0x000036E4
    
    lib->version[LIBRARY_VERSION_MINOR] = libEntryTable->version[LIBRARY_VERSION_MINOR];
    lib->version[LIBRARY_VERSION_MAJOR] = libEntryTable->version[LIBRARY_VERSION_MAJOR];
    
    lib->attribute = libEntryTable->attribute; //0x000036F4
    lib->stubCount = libEntryTable->stubCount;
    
    if (libEntryTable->len < LIBRARY_ENTRY_TABLE_NEW_LEN) { //0x0000370C
        lib->unk22 = 1; //0x0000371C
        lib->vStubCount = libEntryTable->vStubCount;
    }
    else {        
        lib->vStubCount = libEntryTable->unk16; //0x00003804
        lib->vStubCount = (libEntryTable->unk16 < libEntryTable->vStubCount) 
                           ? libEntryTable->vStubCount : libEntryTable->unk16; //0x00003820
        lib->unk22 = libEntryTable->unk19 + 1; 
        
        addr += libEntryTable->unk18 & 0xF;
        addr2 += libEntryTable->unk18 >> 4; //0x0000384C
    }
    //0x00003724
    lib->unk20 = *(u8 *)(addr + 2);
    lib->unk21 = *(u8 *)(addr2 + 2); //0x00003740
    
    exports = lib->stubCount + lib->vStubCount; //0x00003744
    lib->exportsBaseAddr = &libEntryTable->entryTable[exports]; 
    lib->unk24 = *(s16 *)addr; 
    lib->unk48 = (*(s16 *)addr + *(s16 *)addr2) << 1;
    lib->exportsSize = exports << 1;
    lib->midFuncIndex = exports + ((exports + 1) >> 1); //0x00003784
    lib->numExports = exports;
    lib->entryTable = libEntryTable->entryTable;
    lib->unk28 = *(s16 *)addr2;
    lib->unk64 = 0;
    
    /*
     * The corresponding resident library object is living in kernel land,
     * thus we simply copy the entry table's libName pointer over, if the
     * entry table resides in kernel land as well.  That way, we avoid
     * using a memcpy over the libName.
     */
    if (IS_KERNEL_ADDR(libEntryTable->libName)) { //0x00003794
        lib->libName = (char *)libEntryTable->libName; //0x0000379C
        lib->libNameInHeap = SCE_FALSE;
        lib->sysTableEntryStartIndex = 0;     
        return SCE_ERROR_OK;
    }
    
    len = strlen(libEntryTable->libName); //0x000037B0
    lib->libName = sceKernelAllocHeapMemory(g_loadCoreHeap(), len + 1); 
    if (lib->libName == NULL)
        return SCE_ERROR_KERNEL_ERROR;

    memcpy(lib->libName, libEntryTable->libName, len + 1); //0x000037F4
    lib->libNameInHeap = SCE_TRUE;
    lib->sysTableEntryStartIndex = 0;

    return SCE_ERROR_OK;
}

//sub_00003850
/*
 * Copy a stub library's entry table to the corresponding
 * stub library object registered to the system.  In case
 * linking this stub library is successful, it is then inserted 
 * into a linked-list of registered stub libraries belonging to 
 * the same resident library.
 * 
 * Returns 0 on success. 
 */
static s32 CopyLibStub(SceStubLibrary *stubLib, SceStubLibraryEntryTable *stubLibEntryTable, u32 isUserLib)
{       
    u32 len; 
    
    /* Protect Kernel memory from User Mode. */   
    if (isUserLib && (IS_KERNEL_ADDR(stubLibEntryTable) || IS_KERNEL_ADDR(stubLibEntryTable->libName) || 
      IS_KERNEL_ADDR(stubLibEntryTable->nidTable) || IS_KERNEL_ADDR(stubLibEntryTable->stubTable) || 
      IS_KERNEL_ADDR(stubLibEntryTable->vStubTable))) //0x0000386C
        return SCE_KERNEL_VALUE_UNITIALIZED;     
    
    stubLib->isUserLib = isUserLib;
    stubLib->libStubTable = stubLibEntryTable;
    stubLib->stubEntryTableLen = stubLibEntryTable->len; //0x000038D0
    stubLib->libName = stubLibEntryTable->libName;
    stubLib->version[LIBRARY_VERSION_MINOR] = stubLibEntryTable->version[LIBRARY_VERSION_MINOR];
    stubLib->version[LIBRARY_VERSION_MAJOR] = stubLibEntryTable->version[LIBRARY_VERSION_MAJOR];  
    
    stubLib->attribute = stubLibEntryTable->attribute;
    stubLib->vStubCount = stubLibEntryTable->vStubCount;
    stubLib->stubCount = stubLibEntryTable->stubCount;
    stubLib->nidTable = stubLibEntryTable->nidTable;
    stubLib->stubTable = stubLibEntryTable->stubTable;
    
    if (stubLibEntryTable->len < STUB_LIBRARY_ENTRY_TABLE_OLD_LEN)        
        stubLib->vStubTable = NULL; //0x0000399C
    else      
       stubLib->vStubTable = stubLibEntryTable->vStubTable; //0x00003910
        
    //0x00003918
    if (stubLibEntryTable->len < STUB_LIBRARY_ENTRY_TABLE_NEW_LEN)
        stubLib->unk32 = 0;       
    else
       stubLib->unk32 = stubLibEntryTable->unk24;
        
    //0x00003928
    stubLib->status = 0;
    
    /*
     * The corresponding stub library object is living in kernel land,
     * thus we simply copy the entry table's libName pointer over, if the
     * entry table resides in kernel land as well.  That way, we avoid
     * using a memcpy over the libName.
     */
    if (IS_KERNEL_ADDR(stubLibEntryTable->libName)) {        
        stubLib->libName2 = (char *)stubLibEntryTable->libName; //0x00003930
        stubLib->libNameInHeap = SCE_FALSE;
        return SCE_ERROR_OK;
    }
    
    //0x00003944
    len = strlen(stubLibEntryTable->libName);           
    stubLib->libName2 = sceKernelAllocHeapMemory(g_loadCoreHeap(), len + 1);            
    if (stubLib->libName == NULL) 
        return SCE_KERNEL_VALUE_UNITIALIZED;
            
    memcpy(stubLib->libName2, stubLibEntryTable->libName, len + 1);
    stubLib->libNameInHeap = SCE_TRUE;
    return SCE_ERROR_OK;                        
}

//sub_0000562C
static u32 sceKernelSetGetLengthFunction(s32 (*funcPtr)(u8 *file, u32 size, u32 *newSize))
{
    g_getLengthFunc = funcPtr;
    return SCE_ERROR_OK;
}
//sub_0000563C
static u32 sceKernelSetPrepareGetLengthFunction(s32 (*funcPtr)(u8 *buf, u32 size))
{
    g_prepareGetLengthFunc = funcPtr;
    return SCE_ERROR_OK;
}

//sub_0000564C
static u32 sceKernelSetSetMaskFunction(s32 (*funcPtr)(u32 unk1, vs32 *addr))
{
    g_setMaskFunc = funcPtr;
    return SCE_ERROR_OK;
}

//sub_0000565C
static u32 sceKernelSetCompareSubType(s32 (*funcPtr)(u32 tag))
{
   g_compareSubType = funcPtr;
   return SCE_ERROR_OK;
}

static u32 sceKernelSetCompareLatestSubType(u32 (*funcPtr)(u32 tag))
{
    g_compareLatestSubType = funcPtr;
    return SCE_ERROR_OK;
}

//LoadCoreForKernel_5FDDB07A - Address 0x0000567C
s32 sceKernelSegmentChecksum(SceModule *mod)
{
    u32 i;
    u32 checkSum;
    
    //0x00005684 - 0x000056AC
    for (i = 0, checkSum = 0; i < 16; i++) { //0x000056A0
         checkSum += mod->segmentAddr[i];
         if (mod->segmentAddr[i] == JR_RA)
             break;
    }
    return checkSum;
}

/* Stop the system initialization. */
static __inline__ void StopLoadCore(void)
{
    s32 intrState;
    u32 hwRamType;
    
    intrState = loadCoreCpuSuspendIntr();
        
    hwRamType = HW(HW_RAM_SIZE) & ~(RAM_TYPE_32_MB | RAM_TYPE_64_MB); //0x00001584
    hwRamType |= (g_MemSize > RAM_SIZE_32_MB) ? RAM_TYPE_64_MB : RAM_TYPE_32_MB; //0x00001598          
    HW(HW_RAM_SIZE) = hwRamType; //0x000015B4
            
    sceKernelMemset32((void *)HWPTR(HW_RESET_VECTOR), 0, HW_RESET_VECTOR_SIZE); //0x000015B8
    memcpy((void *)HWPTR(HW_RESET_VECTOR), loadCoreClearMem, 0x28); //0x000015D4
    g_MemClearFun = (void (*)(void *, u32))HWPTR(HW_RESET_VECTOR); //0x000015F0
    g_MemClearFun(g_MemBase, g_MemSize); //0x000015EC
        
    loadCoreCpuResumeIntr(intrState); //0x000015F4
            
     while(1)
         ;
}
