/** Copyright (C) 2011, 2012, 2013 The uOFW team
   See the file COPYING for copying permission.\n
*/

#ifndef LOADCORE_H
#define	LOADCORE_H

#include <memlmd.h>
#include <mesgled.h>
#include "common_imp.h"
#include "threadman_kernel.h"

/** @defgroup Loadcore Loadcore Module
 * Module loader and library import/export manager.
 *
 * @{	
 */

/** The maximum number of segments a module can have. */
#define SCE_KERNEL_MAX_MODULE_SEGMENT           (4)

/** The module will remain in memory and act as a resident library. */
#define SCE_KERNEL_RESIDENT                     (0)

/** The module is not a resident one, meaning it won't stay in memory and act as a resident library. */
#define SCE_KERNEL_NO_RESIDENT                  (1)

/** Library number category minor. */
#define LIBRARY_VERSION_MINOR                   (0)

/** Library number category major. */
#define LIBRARY_VERSION_MAJOR                   (1)

/** Current number category size for libraries. */
#define LIBRARY_VERSION_NUMBER_CATEGORY_SIZE    (2)

/** The length of the old resident library entry table format (without the members unk16 - uk19). */
#define LIBRARY_ENTRY_TABLE_OLD_LEN             (4)

/** The length of the new resident library entry table format (including the members unk16 - unk19). */
#define LIBRARY_ENTRY_TABLE_NEW_LEN             (5)

/** The length of the old stub library entry table format (without the member unk24). */
#define STUB_LIBRARY_ENTRY_TABLE_OLD_LEN        (6)

/** The length of the new stub library entry table format (including the member unk24). */
#define STUB_LIBRARY_ENTRY_TABLE_NEW_LEN        (7)

/** The possible number of libraries with different hash values. */
#define LOADCORE_LIB_HASH_TABLE_SIZE            (128)

/** Indicates that a boot callback function was added to the internal boot callback queue. */
#define SCE_BOOT_CALLBACK_FUNCTION_QUEUED       (1)

/** The protected information block is allocated. */
#define SCE_PROTECT_INFO_STATE_IS_ALLOCATED     (1 << 0)

/** Indicates the type of the protected information block is a file name. */
#define SCE_PROTECT_INFO_TYPE_FILE_NAME         (0x2)

/** Indicates the type of the protected information block is a VSH param. */
#define SCE_PROTECT_INFO_TYPE_VSH_PARAM         (0x4)

/** Indicates the type of the protected information block is a disc image. */
#define SCE_PROTECT_INFO_TYPE_DISC_IMAGE        (0x40)

/** Indicates protected information block belongs to a NPDRM package. */
#define SCE_PROTECT_INFO_TYPE_NPDRM_DATA        (0x80)

/** Indicates the type of the protected information block is a user param. */
#define SCE_PROTECT_INFO_TYPE_USER_PARAM        (0x100)

/** Indicates the type of the protected information block is a param.sfo. */
#define SCE_PROTECT_INFO_TYPE_PARAM_SFO         (0x400)

/** 
 * Get the state of a protected information block. 
 * 
 * @param attr SceLoadCoreProtectInfo.attr
 */
#define GET_PROTECT_INFO_STATE(attr)            ((u32)(attr) >> 16)

/** 
 * Set a new state of a protected information block. 
 * 
 * @param state The new state to set.
 * @param src SceLoadCoreProtectInfo.attr
 */
#define SET_PROTECT_INFO_STATE(state, src)      (((state) << 16) | (src))

/** 
 * Remove a state entry of a protected information block. 
 * 
 * @param state The state entry to remove.
 * @param src SceLoadCoreProtectInfo.attr
 */
#define REMOVE_PROTECT_INFO_STATE(state, src)   ((~((state) << 16)) & (src))

/**
 * Get the type of a protected information block. 
 * 
 * @param attr SceLoadCoreProtectInfo.attr
 */
#define GET_PROTECT_INFO_TYPE(attr)             ((attr) & 0xFFFF)

/**
 * Set the type of a protected information block.
 * 
 * @param type The new type to set.
 * @param src SceLoadCoreProtectInfo.attr
 */
#define SET_PROTECT_INFO_TYPE(type, src)        (((type) & 0xFFFF) | (src))

/** 
 * Executable File Attributes.
 */
enum SceExecFileAttr {
    /** The file is compressed. If SCE_EXEC_FILE_KL4E_COMPRESSED is not set, the file is GZIP compressed. */
    SCE_EXEC_FILE_COMPRESSED        = 0x1,
    /** The file is a static ELF. */
    SCE_EXEC_FILE_ELF               = 0x2,
    /** The file has GZIP overlap. */
    SCE_EXEC_FILE_GZIP_OVERLAP      = 0x8,
    /** The file is KL4E compressed. */
    SCE_EXEC_FILE_KL4E_COMPRESSED   = 0x200
};

/** 
 * Executable File Mode Attributes. 
 */
enum SceExecFileModeAttr {
    /** The file is decrypted. */
    SCE_EXEC_FILE_DECRYPT                   = 0,
    /** The file header is not compressed. */
    SCE_EXEC_FILE_NO_HEADER_COMPRESSION     = 1,
    /** The file is not compressed. */
    SCE_EXEC_FILE_NO_COMPRESSION            = 2,
};

/** 
 * The possible ELF type of an executable. 
 */
enum SceExecFileElfType {
    /** The executable file is not an ELF. Such a file cannot be loaded. */
    SCE_EXEC_FILE_TYPE_INVALID_ELF   = -1,
    /** The executable file is a PRX, a relocatable ELF. */
    SCE_EXEC_FILE_TYPE_PRX           = 1,
    /** The executable file is a PRX, a relocatable ELF. */
    SCE_EXEC_FILE_TYPE_PRX_2         = 2,
    /** The executable file is a static ELF. */
    SCE_EXEC_FILE_TYPE_ELF           = 3,
};

/** 
 * Resident/Stub library attributes. Several members can be bitwise OR'ed together. Every library 
 * needs to have at least one of those attributes. Resident libraries can have the members 
 * SCE_LIB_AUTO_EXPORT, SCE_LIB_WEAK_EXPORT, (SCE_LIB_NOLINK_EXPORT), SCE_LIB_SYSCALL_EXPORT and 
 * SCE_LIB_IS_SYSLIB. Stub libraries can have SCE_LIB_NO_SPECIAL_ATTR or SCE_LIB_WEAK_IMPORT.
 */
enum SceLibAttr {
    /** The library has no special attributes. */
    SCE_LIB_NO_SPECIAL_ATTR = 0x0,
    /** Automatically register the library to the system. */
    SCE_LIB_AUTO_EXPORT = 0x1,
    /** Indicates resident library can be overwritten. */
    SCE_LIB_WEAK_EXPORT = 0x2,
    /** Indicates resident library is NOT being linked. */
    SCE_LIB_NOLINK_EXPORT = 0x4,
    /** Load module that references this library even if this library is not registered. */
    SCE_LIB_WEAK_IMPORT = 0x8,
    /** Indicates the use of the SYSCALL technique for linking. */
    SCE_LIB_SYSCALL_EXPORT = 0x4000,
    /** The library is a system library (a mandatory library for all modules). */
    SCE_LIB_IS_SYSLIB = 0x8000
};

/** Boot Callback function. */
typedef s32 (*SceKernelBootCallbackFunction)(void *data, s32 arg, void *opt);

/** 
 * This structure represents a function stub belonging to same privilege-level linked libraries, 
 * i.e. a kernel resident library linked with a kernel stub library. 
 */
typedef struct {
    /** The call to the imported function via a MIPS ASM Jump instruction. */
    u32 call;
    /** The delay slot belonging to the call, typically a NOP instruction. */
    u32 delaySlot;
} DirectCall;

/** 
 * This structure represents a function stub belonging to different privilege-level linked libraries, 
 * i.e. a kernel resident library linked with a user stub library. 
 */
typedef struct {
    /** The return instruction from the stub. Typically a JR $ra command. */
    u32 returnAddr;
    /** The system call exception used to call the imported function. */
    u32 syscall;
} Syscall;

/**
 * This structure represents an imported function stub.
 */
typedef union {
    /** User/User or Kernel/Kernel function stub. */
    DirectCall dc;
    /** Kernel/User function stub. */
    Syscall sc;
} SceStub;

/**
 * This structure represents an imported variable stub.
 */
typedef struct {
    u32 *addr;
    /** The NID identifying the imported variable. */
    u32 nid;
} SceVariableStub;

/**
 * This structure is used to record the functions a resident library provides to other modules. 
 * This entry table is used to register a resident library to the system. A module can register 
 * multiple libraries and multiple libraries with the same name can be in use simultaneously .
 */
typedef struct {
    /** The name of the library. */
    const char *libName; //0
    /** 
     * The version of the library. It consists of a 'major' and 'minor' field. If you want to 
     * register another version of an already registered resident library, make sure that the new 
     * library has a higher version than all its currently registered versions.
     */
    u8   version[LIBRARY_VERSION_NUMBER_CATEGORY_SIZE]; //4
    /** The library's attributes. One or more of ::SceLibAttr. */
    s16  attribute; //6
    /** 
     * The length of this entry table in 32-Bit words. Set this to "LIBRARY_ENTRY_TABLE_NEW_LEN". 
     * Use this member when you want to iterate through a list of entry tables (size = len * 4).
     */
    u8   len; //8
    /** The number of exported variables by the resident library. */
    u8   vStubCount; //9
    /** The number of exported functions by the resident library. */
    u16   stubCount; //10
    /** 
     * Pointer to an array of NIDs, followed by an array of function- and variable pointers. Each 
     * function-/variable pointer must have a NID value. These arrays are used to correctly perform 
     * linking between a resident library and its corresponding stub libraries.
     */
    u32 *entryTable; //12
    /** Unknown. */
    u16  unk16; //16
    /** Unknown. */
    u8   unk18; //18
    /** Unknown. */
    u8   unk19; //19
} SceResidentLibraryEntryTable;

/**
 * This structure represents the imports, provided by a resident library, that a given module is using. 
 * A module can have multiple stub libraries.
 */
typedef struct {
    /** The name of the library. */
	const char *libName;  //0
    /** 
     * The version of the library. It consists of a 'major' and 'minor' field. The version of a stub 
     * library shouldn't be higher than the version(s) of the corresponding resident library/libraries. 
     * Linking won't be performed in such a case.
     */
	u8 version[LIBRARY_VERSION_NUMBER_CATEGORY_SIZE]; //4
    /** The library's attributes. Can be set to either SCE_LIB_NO_SPECIAL_ATTR or SCE_LIB_WEAK_IMPORT. */
	u16 attribute; //6
    /**
     * The length of this entry table in 32-Bit words. Set this to either "STUB_LIBRARY_ENTRY_TABLE_OLD_LEN" 
     * or "STUB_LIBRARY_ENTRY_TABLE_NEW_LEN". Use this member when  you want to iterate through a 
     * list of entry tables (size = len * 4).
     */ 
	u8 len; //8
    /** The number of imported variables by the stub library. */
	u8 vStubCount; //9
    /** The number of imported functions by the stub library. */
	u16 stubCount; //10
    /** Pointer to an array of NIDs containing the NIDs of the imported functions and variables. */
	u32 *nidTable; //12
    /** Pointer to an array of imported function stubs. */
	SceStub *stubTable; //16
    /** Pointer to an array of imported variable stubs. */
	SceVariableStub *vStubTable; // 20
    /** Unknown. */
    u16 unk24; //24
} SceStubLibraryEntryTable;

/**
 * This structure represents a boot callback belonging to a module.
 */
typedef struct {
    /** The boot callback function. */
    void *bootCBFunc;
    /** Global pointer value of the module. */
    u32 gp;
} SceBootCallback;

/**
 * This structure represents a stub library control block. This control block is used to manage a 
 * stub library entry table internal.
 */
typedef struct SceStubLibrary {
    /** Unknown. */
    u32 unk0; //0
    /** 
     * A linked list of stub libraries belonging to the same group, i.e. the same resident library.
     */
    struct SceStubLibrary *next; //4
    /** The name of the library. */
    const char *libName; //8
    /** 
     * The version of the library. This member is set by the corresponding stub library entry table. 
     */
    u8 version[LIBRARY_VERSION_NUMBER_CATEGORY_SIZE]; //12
    /** 
     * The library's attributes. This member is set by the corresponding stub library entry table. 
     */
    u16 attribute; //14
    /** The length of the corresponding stub library entry table in 32-Bit words. */
    u8 stubEntryTableLen; //16
    /** 
     * The number of imported variables by the stub library. This member is set by the corresponding 
     * stub library entry table. 
     */
    u8 vStubCount; //17
    /** 
     * The number of imported functions by the stub library. This member is set by the corresponding 
     * stub library entry table.
     */
    u16 stubCount; //18
    /** Pointer to an array of NIDs identifying the imported functions/variables.*/
    u32 *nidTable; //20
    /** Pointer to the imported function stubs. */
    SceStub *stubTable; //24
    /** Pointer to the imported variable stubs. */
    SceVariableStub *vStubTable; //28
    /** Unknown. */
    u16 unk32; //32
    /** Pointer to the corresponding stub library entry table. */
    SceStubLibraryEntryTable *libStubTable; //36
    /** 
     * The current status of a stub library (control block) in memory. One of ::SceStubLibraryStatus.
     */
    u32 status; //40
    /** Indicates whether the stub library lives in User land or Kernel land. */
    u32 isUserLib; //44
    /** The name of the library. */
    char *libName2; //48
    /** Indicates whether the library's name is located in the heap or not. */
    u32 libNameInHeap; //52
} SceStubLibrary; //size = 56

/**
 * This structure represents a resident library control block. This control block is used to manage 
 * a resident library entry table registered to the system.
 */
typedef struct SceResidentLibrary {
    /** Pointer to the next resident library with the same hash value. */
    struct SceResidentLibrary *next; //0
    /** Pointer to the corresponding entry table used to register this library. */
    SceResidentLibraryEntryTable *libEntryTable; //4
    /** 
     * The version of the library. This member is set by the corresponding resident library entry 
     * table. 
     */
    u8 version[LIBRARY_VERSION_NUMBER_CATEGORY_SIZE]; //8
    /** 
     * The library's attributes. This member is set by the corresponding resident library entry table. 
     */
    u16  attribute; //10
    /** 
     * The number of exported functions by the resident library. This member is set by the 
     * corresponding resident library entry table.
     */
    u16 stubCount; //12
    /** 
     * The number of exported variables by the resident library. This member is set by the 
     * corresponding resident library entry table. 
     */
    u16 vStubCount; //14
    /** 
     * The number of total exports by the resident library (the sum of stubCount 
     * and vStubCount). 
     */
    u32 numExports; //16
    /** Unknown. */
    u8 unk20; //20
    /** Unknown. */
    u8 unk21; //21
    /** Unknown. */
    u8 unk22; //22
    /** Unknown. */
    s32 unk24; //24
    /** Unknown. */
    u32 unk28; //28
    /** 
     * Pointer to the resident library's array of NIDs, exported subroutine/variable entries. 
     * Every subroutine/variable entry has a corresponding NID used to identify it. The NID array 
     * comes first, followed by pointers to the exported functions and variables.
     */
    u32 *entryTable; //32
    /** A pointer to the first export entry (either a subroutine or variable). */
    u32 *exportsBaseAddr; //36
    /** The number of exported functions and variables + the number of NIDs used for them. */
    u32 exportsSize; //40
    /** The index into the middle of the exported functions/variables array. */
    u32 midFuncIndex; //44
    /** Unknown. */
    u32 unk48; //48
    /** Pointer to a linked list of corresponding loaded stub libraries. */
    SceStubLibrary *stubLibs; //52
    /** The address of the system call table block belonging to the resident library. */
    u32 sysTableEntry; //56
    /** Indicates whether the resident library lives in User land or Kernel land. */
    u32 isUserLib; //60
    /** Unknown. */
    u32 unk64; //64 
    /** The name of the library. */
    char *libName; //68 
    /** Indicates whether the library's name is located in the heap or not. */
    u32 libNameInHeap; //72
    /**
     * The entry index into Loadcore's system call table for an exported function of the resident 
     * library.
     */
    u16 sysTableEntryStartIndex; //76
    /** 
     * Extra export entries in the system call table entry belonging to the resident library. 
     */
    u16 extraExportEntries; //78
} SceResidentLibrary; //size = 80

/** 
 * This structure represents Protection Information
 */
typedef struct {
    /** Start address of the protected info. */
    u32 addr;
    /** The size of the protected info. */
    SceSize size; // 4
    /**
     *   31         16 15        0\n
     *  +-------------+-----------+\n
     *  |  S T A T E  |  T Y P E  |\n
     *  +-------------+-----------+\n
     * 
     * Bits 31 - 16:
     *      The current state of the protected information.
     * 
     * Bits 15 - 0:
     *      The type of the protected information.
     */
    u32 attr; // 8
    /** The partition ID of the protected info block. */
    SceUID partId; // 12
    /** Unknown. */
    s32 unk16;
    /** Unknown. */
    s32 unk20;
    /** Unknown. */
    s32 unk24;
} SceLoadCoreProtectInfo;

/**
 * This structure is used to boot system modules during the initialization of Loadcore. It represents
 * a module object with all the necessary information needed to boot it.
 */
typedef struct {
    /** The full path (including filename) of the module. */
    u8 *modPath; //0
    /** The buffer with the entire file content. */
    u8 *modBuf; //4
    /** The size of the module. */
    SceSize modSize; //8
    /** Unknown. */
    s32 unk12; //12
    /** Attributes. */
    u32 attr; //16
    /** 
     * Contains the API type of the module prior to the allocation of memory for the module. 
     * Once memory is allocated, ::bootData contains the ID of that memory partition.
     */
    s32 bootData; //20
    /** The size of the arguments passed to the module's entry function? */
    u32 argSize; //24
    /** The partition ID of the arguments passed to the module's entry function? */
    SceUID argPartId; //28
} SceLoadCoreBootModuleInfo;

/**
 * Loadcore Boot Information - Used to boot the system via Loadcore.
 */
typedef struct { 
    /** 
     * Pointer to a memory block which will be cleared in case the system initialization via 
     * Loadcore fails.
     */
    void *memBase; // 0
    /** The size of the memory block to clear. */
    u32 memSize; // 4
    /** Number of modules already loaded during boot process. */
    u32 loadedModules; // 8
    /** Number of modules to boot. */
    u32 numModules; // 12
    /** The modules to boot. */
    SceLoadCoreBootModuleInfo *modules; // 16
     /** Unknown. */
    s32 unk20; //20
     /** Unknown. */
    u8 unk24; //24
    /** Reserved - padding. */
    u8 reserved[3]; // ?
    /** The number of protected (?)modules.*/
    s32 numProtects; // 28
    /** Pointer to the protected (?)modules. */
    SceLoadCoreProtectInfo *protects; // 32
    /** The ID of a protected info. */
    SceUID modProtId;
    /** The ID of a module's arguments? */
    SceUID modArgProtId; // 40
     /** Unknown. */
    s32 unk44;
     /** Unknown. */
    s32 buildVersion;
     /** Unknown. */
    s32 unk52;
    /** The path/name of a boot configuration file. */
    char *configFile; // 56
    /** Unknown. */
    s32 unk60;
    /** Unknown. */
    s32 unk64;
    /** Unknown. */
    s32 unk68;
    /** Unknown. */
    s32 unk72;
    /** Unknown. */
    s32 unk76;
    /** Unknown. */
    u32 unk80;
    /** Unknown. */
    u32 unk84;
    /** Unknown. */
    u32 unk98;
    /** Unknown. */
    u32 unk92;
    /** Unknown. */
    u32 unk96;
    /** Unknown. */
    u32 unk100;
    /** Unknown. */
    u32 unk104;
    /** Unknown. */
    u32 unk108;
    /** Unknown. */
    u32 unk112;
    /** Unknown. */
    u32 unk116;
    /** Unknown. */
    u32 unk120;
    /** Unknown. */
    u32 unk124;
} SceLoadCoreBootInfo; //size = 128

/**
 * This structure represents executable file information used to load the file.
 */
typedef struct {
    /** Unknown. */
    u32 unk0;
    /** The mode attribute of the executable file. One of ::SceExecFileModeAttr. */
    u32 modeAttribute; //4
    /** The API type. */
    u32 apiType; //8
    /** Unknown. */
    u32 unk12;
    /** The size of the executable, including the ~PSP header. */
    SceSize execSize; //16
    /** The maximum size needed for the decompression. */
    SceSize maxAllocSize; //20
    /** The memory ID of the decompression buffer. */
    SceUID decompressionMemId; //24
    /** Pointer to the compressed module data. */
    void *fileBase; //28
    /** Indicates the ELF type of the executable. One of ::SceExecFileElfType. */
    u32 elfType; //32 
    /** The start address of the TEXT segment of the executable in memory. */
    void *topAddr; //36
    /**
     * The entry address of the module. It is the offset from the start of the TEXT segment to the 
     * program's entry point. 
     */
    u32 entryAddr; //40
    /** Unknown. */
    u32 unk44;
    /** 
     * The size of the largest module segment. Should normally be "textSize", but technically can 
     * be any other segment. 
     */
    SceSize largestSegSize; //48
    /** The size of the TEXT segment. */
    SceSize textSize; //52
    /** The size of the DATA segment. */
    SceSize dataSize; //56
    /** The size of the BSS segment. */
    SceSize bssSize; //60
    /** The memory partition of the executable. */
    u32 partitionId; //64
    /** 
     * Indicates whether the executable is a kernel module or not. Set to 1 for kernel module, 
     * 0 for user module. 
     */
    u32 isKernelMod; //68
    /** 
     * Indicates whether the executable is decrypted or not. Set to 1 if it is successfully decrypted, 
     * 0 for encrypted. 
     */
    u32 isDecrypted; //72
    /** The offset from the start address of the TEXT segment to the SceModuleInfo section. */
    u32 moduleInfoOffset; //76
    /** The pointer to the module's SceModuleInfo section. */
    SceModuleInfo *moduleInfo; //80
    /** Indicates whether the module is compressed or not. Set to 1 if it is compressed, otherwise 0.*/
    u32 isCompressed; //84
    /** The module's attributes. One or more of ::SceModuleAttribute and ::SceModulePrivilegeLevel. */
    u16 modInfoAttribute; //88
    /** The attributes of the executable file. One of ::SceExecFileAttr. */
    u16 execAttribute; //90
    /** The size of the decompressed module, including its headers. */
    SceSize decSize; //92
    /** Indicates whether the module is decompressed or not. Set to 1 for decompressed, otherwise 0. */
    u32 isDecompressed; //96
    /** 
     * Indicates whether the module was signChecked or not. Set to 1 for signChecked, otherwise 0. 
     * A signed module has a "mangled" executable header, in other words, the "~PSP" signature can't 
     * be seen. 
     */
    u32 isSignChecked; //100
    /** Unknown. */
    u32 unk104;
    /** The size of the GZIP compression overlap. */
    SceSize overlapSize; //108
    /** Pointer to the first resident library entry table of the module. */
    void *exportsInfo; //112
    /** The size of all resident library entry tables of the module. */
    SceSize exportsSize; //116
    /** Pointer to the first stub library entry table of the module. */
    void *importsInfo; //120
    /** The size of all stub library entry tables of the module. */
    SceSize importsSize; //124
    /** Pointer to the string table section. */
    void *strtabOffset; //128
    /** The number of segments in the executable. */
    u8 numSegments; //132
    /** Reserved. */
    u8 padding[3]; //133
    /** An array containing the start address of each segment. */
    u32 segmentAddr[SCE_KERNEL_MAX_MODULE_SEGMENT]; //136
    /** An array containing the size of each segment. */
    u32 segmentSize[SCE_KERNEL_MAX_MODULE_SEGMENT]; //152
    /** The ID of the ELF memory block containing the TEXT, DATA and BSS segment. */
    SceUID memBlockId; //168
    /** An array containing the alignment information of each segment. */
    u32 segmentAlign[SCE_KERNEL_MAX_MODULE_SEGMENT]; //172
    /** The largest value of the segmentAlign array. */
    u32 maxSegAlign; //188
} SceLoadCoreExecFileInfo;

/** The SceModule structure represents a loaded module in memory. */
typedef struct SceModule {
    /** Pointer to the next registered module. Modules are connected via a linked list. */
	struct SceModule *next; //0
    /** The attributes of a module. One or more of ::SceModuleAttribute and ::SceModulePrivilegeLevel. */
	u16 attribute; //4
    /** 
     * The version of the module. Consists of a major and minor part. There can be several modules 
     * loaded with the same name and version.
     */
	u8 version[MODULE_VERSION_NUMBER_CATEGORY_SIZE]; //6
    /** The module's name. There can be several modules loaded with the same name. */
	char modName[SCE_MODULE_NAME_LEN]; //8
    /** Unknown. */
	u8 terminal; //35
    /** 
     * The status of the module. Contains information whether the module has been started, stopped, 
     * is a user module, etc.
     */
	u16 status; //36
    /** Reserved. */
	u16 padding; //38
    /** A secondary ID for the module. */
	SceUID secId; //40
    /** The module's UID. */
	SceUID modId; //44
    /** The thread ID of a user module. */
	SceUID userModThid; //480
    /** The ID of the memory block belonging to the module. */
	SceUID memId; //52
    /** The ID of the TEXT segment's memory partition. */
	SceUID mpIdText; //56
    /** The ID of the DATA segment's memory partition. */
	SceUID mpIdData; //60
    /** Pointer to the first resident library entry table of the module. */
	void *entTop; //64
    /** The size of all resident library entry tables of the module. */
	SceSize entSize; //68
    /** Pointer to the first stub library entry table of the module. */
	void *stubTop; //72
    /** The size of all stub library entry tables of the module. */
	SceSize stubSize; //76
    /** 
     * A pointer to the (required) module's start entry function. This function is executed during 
     * the module's startup. 
     */
	SceKernelThreadEntry moduleStart; //80
    /** 
     * A pointer to the (required) module's stop entry function. This function is executed during 
     * the module's startup. 
     */
	SceKernelThreadEntry moduleStop; //84
    /** 
     * A pointer to a module's Bootstart entry function. This function is probably executed after 
     * a reboot. 
     */
	SceKernelThreadEntry moduleBootstart; //88
    /** 
     * A pointer to a module's rebootBefore entry function. This function is probably executed 
     * before a reboot. 
     */
	SceKernelThreadEntry moduleRebootBefore; //92
    /** 
     * A pointer to a module's rebootPhase entry function. This function is probably executed 
     * during a reboot. 
     */
	SceKernelThreadEntry moduleRebootPhase; //96
    /** 
     * The entry address of the module. It is the offset from the start of the TEXT segment to the 
     * program's entry point. 
     */
	u32 entryAddr; //100
    /** Contains the offset from the start of the TEXT segment of the program's GP register value. */
	u32 gpValue; //104
    /** The start address of the TEXT segment. */
	u32 textAddr; //108
    /** The size of the TEXT segment. */
	SceSize textSize; //112
    /** The size of the DATA segment. */
	SceSize dataSize; //116
    /** The size of the BSS segment. */
	SceSize bssSize; //120
    /** The number of segments the module consists of. */
	u8 	nSegments; //124
    /** Reserved. */
	u8	padding2[3]; //125
    /** An array containing the start address of each segment. */
	u32 segmentAddr[SCE_KERNEL_MAX_MODULE_SEGMENT]; //128
    /** An array containing the size of each segment. */
	SceSize segmentSize[SCE_KERNEL_MAX_MODULE_SEGMENT]; //144
    /** An array containing the alignment information of each segment. */
    u32 segmentAlign[SCE_KERNEL_MAX_MODULE_SEGMENT]; //160
    /** The priority of the module start thread. */
	s32 moduleStartThreadPriority; //176
    /** The stack size of the module start thread. */
	SceSize moduleStartThreadStacksize; //180
    /** The attributes of the module start thread. */
	SceUInt moduleStartThreadAttr; //184
    /** The priority of the module stop thread. */
	s32 moduleStopThreadPriority; //188
    /** The stack size of the module stop thread. */
	SceSize moduleStopThreadStacksize; //192
    /** The attributes of the module stop thread. */
	SceUInt moduleStopThreadAttr; //196
    /** The priority of the module reboot before thread. */
	s32 moduleRebootBeforeThreadPriority; //200
    /** The stack size of the module reboot before thread. */
	SceSize moduleRebootBeforeThreadStacksize; //204
    /** The attributes of the module reboot before thread. */
	SceUInt moduleRebootBeforeThreadAttr; //208
    /** The value of the coprocessor 0's count register when the module is created. */
	u32 countRegVal; //212
    /** The segment checksum of the module's segments. */
    u32 segmentChecksum; //216
    /** Unknown. */
    u32 unk220; //220
    /** Unknown. */
    u32 unk224; //224
} SceModule; //size = 228

/**
 * This structure represents a system call table. Such a table takes care of the exported system 
 * calls registered to the system.
 */
typedef struct SceSyscallTable {
    /** Pointer to the next SystemCall table. */
    struct SceSyscallTable *next;
    /** Partly defines the location of the system call table. */
    s32 seed;
    /** Size of the structure (including the syscalls array). */
    s32 funcTableSize;
    /** Size of the syscalls array. */
    s32 tableSize;
    /** Variable-size array containing a list of syscalls. */
    void (*syscalls[])();
} SceSyscallTable;

/** 
 * This structure represents a Loadcore Control Block. It is used keep track of important system
 * information, such as maintaining the list of loaded modules or registered libraries.
 */
typedef struct {
    /** 
     * An array of linked lists of registered resident libraries. The slot used for a library is 
     * computed by a hash algorithm. Libraries with the same hash are stored in the array slot 
     * connected via a linked list.
     */
    SceResidentLibrary *registeredLibs[LOADCORE_LIB_HASH_TABLE_SIZE]; //0
    /** Pointer to Loadcore's system call table object. */
    SceSyscallTable *sysCallTable; //512
    /** The seed of the system call table. Used to locate a table entry. */
    u32 sysCallTableSeed; //516
    /** Unknown. */
    u32 unk520;
    /** A linked list of loaded modules. */
    SceModule *registeredMods; //524
    /** Pointer to the latest loaded module.*/
    SceModule *lastRegMod; //528
    /** The number of currently loaded modules. */
    u32 regModCount; //532
    /** The secondary module ID value assigned to a module during registration. */
    u32 secModId; //536
    /** A linked list of currently unlinked stub libraries living in memory. */
    SceStubLibrary *unLinkedStubLibs; //540
    /** 
     * The ID of Loadcore's heap block. Used to allocate memory from the heap 
     * in Loadcore. 
     */
    SceUID loadCoreHeapId; //548
    /** Indicates whether Loadcore's stub libraries were linked or not.*/
    u32 linkedLoadCoreStubs;
    /** Pointer to Loadcore's control block of boot callbacks. */
    SceBootCallback *bootCallBacks; //556
} SceLoadCore;


/**
 * Register a resident library's entry table in the system. A resident module can register any 
 * number of resident libraries. Note that this function is only meant to register kernel mode 
 * resident libraries. In order to register user mode libraries, use sceKernelRegisterLibraryForUser().
 * 
 * @param libEntryTable Pointer to the resident library's entry table.
 * 
 * @return 0 on success.
 */
s32 sceKernelRegisterLibrary(SceResidentLibraryEntryTable *libEntryTable);

/**
 * Check if a resident library can be released. This check returns "true" when all corresponding stub
 * libraries at the time of the check have one the following status:
 *      a) unlinked
 *      b) have the the attribute SCE_LIB_WEAK_IMPORT (they can exist without the resident library 
 *         being registered).
 * 
 * @param libEntryTable Pointer to the resident library's entry table.
 * 
 * @return 0 indicates the library can be released.
 */
s32 sceKernelCanReleaseLibrary(SceResidentLibraryEntryTable *libEntryTable);

/**
 * Link kernel mode stub libraries with the corresponding registered resident libraries. Note that 
 * this function assumes that the resident libraries linked with reside in kernel memory. Linking 
 * with user mode resident libraries will result in failure.
 * 
 * @param libStubTable Pointer to a stub library's entry table. If you want to link an array of 
 *                     entry tables, make libStubTable a pointer to the first element of that array.
 * @param size         The number of entry tables to link.
 * 
 * @return 0 on success.
 */
s32 sceKernelLinkLibraryEntries(SceStubLibraryEntryTable *libStubTable, u32 size);

/**
 * Unlink stub libraries from their corresponding registered resident libraries. 
 * 
 * @param libStubTable Pointer to a stub library's entry table. If you want to unlink an array of 
 *                     entry tables, make libStubTable a pointer to the first element of that array.
 * @param size The number of entry tables to unlink.
 * @return 
 */
s32 sceKernelUnLinkLibraryEntries(SceStubLibraryEntryTable *libStubTable, u32 size);

/**
 * Load a module. This function is used to boot modules during the start of Loadcore. In order for 
 * a module to be loaded, it has to be a kernel module.
 * 
 * @param bootModInfo Pointer to module information (including the file content of the module, 
 *                    its size,...) used to boot the module.
 * @param execInfo Pointer an allocated execInfo structure used to handle load-checks against the 
 *                 program module.
 *                 Furthermore, it collects various information about the module, such as its elfType, 
 *                 its segments (.text, .data, .bss), the locations of its exported functions.
 * @param modMemId The memory id of the allocated kernelPRX memory block used for the program module 
 *                 sections. The memory block specified by the ID holds the .text segment of the module. 
 * 
 * @return 0 on success.
 */
s32 sceKernelLoadModuleBootLoadCore(SceLoadCoreBootModuleInfo *bootModInfo, SceLoadCoreExecFileInfo *execInfo, 
                                    SceUID *modMemId);

/**
 * Save interrupts state and disable all interrupts.
 * 
 * @return The current state of the interrupt controller. Use sceKernelLoadCoreUnlock() to return 
 *         to that state.
 */
s32 sceKernelLoadCoreLock(void);

/**
 * Return interrupt state.
 * 
 * @param intrState The state acquired by sceKernelLoadCoreLock().
 */
void sceKernelLoadCoreUnlock(s32 intrState);

/**
 * Register a user mode resident library's entry table in the system. A resident module can register 
 * any number of resident libraries. In order to register kernel mode libraries, use 
 * sceKernelRegisterLibrary().
 * 
 * Restrictions on user mode resident libraries:
 *    1) The resident library has to live in user memory.
 *    2) Functions cannot be exported via the SYSCALL technique.
 *    3) The resident library cannot be linked with stub libraries living in kernel memory.
 * 
 * @param libEntryTable Pointer to the resident library's entry table.
 * 
 * @return 0 on success.
 */
s32 sceKernelRegisterLibraryForUser(SceResidentLibraryEntryTable *libEntryTable);

/**
 * Delete a registered resident library from the system. Deletion cannot be performed if there are 
 * loaded modules using the resident library. These modules must be deleted first.
 * 
 * @param libEntryTable Pointer to the resident library's entry table.
 * 
 * @return 0 on success.
 */
s32 sceKernelReleaseLibrary(SceResidentLibraryEntryTable *libEntryTable);

/**
 * 
 * @param libStubTable Pointer to a stub library's entry table. If you want to link an array of entry 
 *                     tables, make libStubTable a pointer to the first element of that array.
 * @param size The number of entry tables to link.
 * 
 * @return 0 on success.
 */
s32 sceKernelLinkLibraryEntriesForUser(SceStubLibraryEntryTable *libStubTable, u32 size);

/**
 * 
 * @param mod Pointer to a module. Should not be NULL. The module seems not to be used for anything 
 *            useful.
 * @param libStubTable Pointer to a stub library's entry table. If you want to link an array of 
 *                     entry tables, make libStubTable a pointer to the first element of that array.
 * @param size The number of entry tables to link.
 * 
 * @return 0 on success.
 */
s32 sceKernelLinkLibraryEntriesWithModule(SceModule *mod, SceStubLibraryEntryTable *libStubTable, u32 size);

/**
 * Does nothing but a simple return.
 * 
 * @return 0.
 */
u32 sceKernelMaskLibraryEntries(void);

/**
 * Get Loadcore's control block. The block takes care of the registered libraries, the unlinked 
 * stub libraries living in memory and the currently loaded modules.
 * 
 * @return A pointer to Loadcore's internal control block.
 */
SceLoadCore *sceKernelQueryLoadCoreCB(void);

/**
 * Set a boot callback.  Call this function during a module boot process.
 * 
 * @param bootCBFunc The boot callback function to execute once the important system modules 
 *                   (up to init.prx) have been booted.
 * @param flag Defines the execute order of the callbacks. Pass 0 for earliest execution, 3 for latest.
 *             1 and 2 are between these two.
 * @param status The returned status of bootCBFunc in case it was executed directly.
 * 
 * @return 0 for directly executing the boot callback function. 1 indicates boot callback function 
 *           was enqueued into other existing boot callbacks and will be called after init.prx got 
 *           booted. 
 */
s32 sceKernelSetBootCallbackLevel(SceKernelBootCallbackFunction bootCBFunc, u32 flag, s32 *status);

/**
 * Does nothing but a simple return, probably a debug function.
 * 
 * @return 0.
 */
u32 sceKernelLoadCoreMode(void);

/**
 * Check and decrypt a PSP configuration file.
 * 
 * @param file The configuration file (i.e. pspbtcnf.bin) to decrypt.
 * @param size The size of the file.
 * 
 * @return The size of the decrypted file on success.
 */
s32 sceKernelCheckPspConfig(u8 *file, u32 size);

/**
 * Decrypt and load a reboot file used to boot the system. Reboot.bin is only used for kernel reboots 
 * (warm reboots only).
 * 
 * @param file The reboot file to use (i.e. reboot.bin).
 * @param size The size of the file
 * 
 * @return 0 on success.
 */
s32 sceKernelLoadRebootBin(u8 *file, u32 size);

/**
 * Compute a checksum of every segment of a module.
 * 
 * @param mod The module to create the checksum for.
 * 
 * @return The checksum. Shouldn't be 0.
 */
s32 sceKernelSegmentChecksum(SceModule *mod);

/**
 * Check an executable file. This contains scanning its ELF header and ~PSP header (if it has one) 
 * and filling the execInfo structure with basic information, like the ELF type, segment information, 
 * the size of the executable. The file is also uncompressed, if it was compressed before.
 * 
 * @param buf Pointer to the file's contents.
 * @param execInfo Pointer to the executionInfo belonging to that executable.
 * 
 * @return 0 on success.
 */
s32 sceKernelCheckExecFile(u8 *buf, SceLoadCoreExecFileInfo *execInfo);

/**
 * Probe an executable file. This contains calculating the sizes for the three segments TEXT, DATA 
 * and BSS, filling the execInfo structure with information about the location and sizes of the 
 * resident/stub library entry tables.
 * Furthermore, it is checked whether the executable has valid API type or not.
 * 
 * @param buf Pointer to the file's contents.
 * @param execInfo Pointer to the executionInfo belonging to that executable.
 * 
 * @return 0 on success.
 */
s32 sceKernelProbeExecutableObject(u8 *buf, SceLoadCoreExecFileInfo *execInfo);

/**
 * Load an executable file. This contains allocating s memory block containing the three segments 
 * TEXT, DATA and BSS (in case the executable consists of only these three sections). 
 * Furthermore, relocation of the executable file, if needed, is also taken care off.
 * 
 * @param buf Pointer to the file's contents.
 * @param execInfo Pointer to the executionInfo belonging to that executable.
 * 
 * @return 0 on success.
 */
s32 sceKernelLoadExecutableObject(u8 *buf, SceLoadCoreExecFileInfo *execInfo);

/**
 * Allocate memory for a new SceModule structure and fill it with default values. This function is 
 * called during the loading process of a module.
 * 
 * @return A pointer to the allocated SceModule structure on success, otherwise NULL.
 */
SceModule *sceKernelCreateModule(void);

/**
 * Assign a module and check if it can be loaded, is a valid module and copy the moduleInfo section 
 * of the execution file over to the SceModule structure.
 * 
 * @param mod The module to receive the moduleInfo section data based on the provided execution file 
 *            information.
 * @param execFileInfo The execution file information used to copy over the moduleInfo section for 
 *        the specified module.
 * 
 * @return 0 on success.
 */
s32 sceKernelAssignModule(SceModule *mod, SceLoadCoreExecFileInfo *execFileInfo);

/**
 * Unlink a module from the internal loaded-modules-linked-list. The module has to be stopped before.
 * 
 * @param mod The module to release.
 * 
 * @return 0 on success.
 */
s32 sceKernelReleaseModule(SceModule *mod);

/**
 * Receive a list of UIDs of loaded modules.
 * 
 * @param modIdList Pointer to a SceUID array which will receive the UIDs of the loaded modules.
 * @param size Size of modIdList. Specifies the number of entries that can be stored into modIdList.
 * @param modCount A pointer which will receive the total number of loaded modules.
 * @param userModsOnly Set to 1 to only receive UIDs from user mode modules. Set to 0 to receive UIDs 
 *                     from all loaded modules.
 * 
 * @return 0 on success.
 */
s32 sceKernelGetModuleIdListForKernel(SceUID *modIdList, u32 size, u32 *modCount, u32 userModsOnly);

/**
 * Get a loaded module from its UID.
 * 
 * @param uid The UID (of a module) to check for.
 * 
 * @return Pointer to the found SceModule structure on success, otherwise NULL.
 */
SceModule *sceKernelGetModuleFromUID(SceUID uid);

/**
 * Delete a module from the system. The module has to be stopped and released before.
 * 
 * @param mod The module to delete.
 * 
 * @return 0 on success.
 */
s32 sceKernelDeleteModule(SceModule *mod);

/**
 * Create and assign a module. It provides the same result as a sceKernelCreateModule() call 
 * followed by a sceKernelAssignModule() call.
 * 
 * @param execFileInfo The execution file information used to copy over the moduleInfo section for 
 * the specified module.
 * 
 * @return Pointer to the created SceModule structure on success, otherwise NULL.
 */
SceModule *sceKernelCreateAssignModule(SceLoadCoreExecFileInfo *execFileInfo);

/**
 * Register a module in the system and link it into the internal loaded-modules-linked-list.
 * 
 * @param mod The module to register.
 * 
 * @return 0.
 */
s32 sceKernelRegisterModule(SceModule *mod);

/**
 * Find a loaded module by its name. If more than one module with the same name is loaded, return 
 * the module which was loaded last.
 * 
 * @param name The name of the module to find. 
 * 
 * @return Pointer to the found SceModule structure on success, otherwise NULL.
 */
SceModule *sceKernelFindModuleByName(const char *name);

/**
 * Find a loaded module containing the specified address.
 * 
 * @param addr Memory address belonging to the module, i.e. the address of a function/global variable 
 *             within the module.
 * 
 * @return Pointer to the found SceModule structure on success, otherwise NULL.
 */
SceModule *sceKernelFindModuleByAddress(u32 addr);

/**
 * Get the global pointer value of a module.
 * 
 * @param addr Memory address belonging to the module, i.e. the address of a function/global variable 
 *             within the module.
 * 
 * @return The global pointer value (greater than 0) of the found module on success.
 */
s32 sceKernelGetModuleGPByAddressForKernel(u32 addr);

/**
 * Find a loaded module by its UID.
 * 
 * @param uid The UID of the module to find.
 * 
 * @return Pointer to the found SceModule structure on success, otherwise NULL.
 */
SceModule *sceKernelFindModuleByUID(SceUID uid);

/**
 * Receive a list of UIDs of all loaded modules.
 * 
 * @param modCount A pointer which will receive the total number of loaded modules.
 * 
 * @return The UID of the allocated array containing UIDs of the loaded modules on success. It should 
 *        be greater than 0.
 */
SceUID sceKernelGetModuleListWithAlloc(u32 *modCount);

#endif	/** LOADCORE_H */

/** @} */
