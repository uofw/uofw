/* Copyright (C) 2011 - 2024 The uOFW team
   See the file COPYING for copying permission.
*/

#include <common_imp.h>

#include <interruptman.h>
#include <sysmem_kernel.h>
#include <sysmem_sysclib.h>
#include <iofilemgr_kernel.h>
#include <mediaman_kernel.h>
#include <umd_error.h>
#include <umdman.h>
#include <rtc.h>

SCE_MODULE_INFO("sceIsofs_driver", SCE_MODULE_KERNEL | SCE_MODULE_ATTR_CANT_STOP | SCE_MODULE_ATTR_EXCLUSIVE_LOAD |
                SCE_MODULE_ATTR_EXCLUSIVE_START, 1, 5);
SCE_MODULE_BOOTSTART("isofsInit");
SCE_MODULE_REBOOT_BEFORE("isofsRebootBefore");
SCE_SDK_VERSION(SDK_VERSION);

#define UMD_ID_SIZE 32

#define ISOFS_CACHE_ENTRIES 0x10

#define DEVCTL_GET_UNIT_NUM 0x1e080a8
#define DEVCTL_GET_PVD 0x1e28035
#define DEVCTL_READ_SECTORS 0x1e380c0
#define DEVCTL_GET_SECTOR_SIZE 0x1e280a9
#define DEVCTL_SEEK_DISC_RAW 0x1f100a3
#define DEVCTL_UNKNOWN_1E180D3 0x1e180d3
#define DEVCTL_UNKNOWN_1E38034 0x1e38034
#define DEVCTL_PREPARE_INTO_CACHE 0x1f100a4
#define DEVCTL_PREPARE_INTO_CACHE_GET_STATUS 0x1f300a5
#define DEVCTL_UNKNOWN_1F100A8 0x1f100a8
#define DEVCTL_UNKNOWN_1F100A9 0x1f100a9
#define DEVCTL_GET_TOTAL_NUMBER_OF_SECTORS 0x1f20003
#define DEVCTL_GET_CURRENT_LBN 0x1f20002

/**
 * Types
 */

/** ISO date time as defined in the ISO specs. */
typedef struct __attribute__((packed)) {
    u8 yearsSince1900;
    u8 month;
    u8 day;
    u8 hour;
    u8 minute;
    u8 second;
    u8 offset;
} IsoDirectoryDateTime;

/** ISO directory record as defined in the ISO specs.
 * Directory records are used to describe both files and directories. */
typedef struct __attribute__((packed)) {
    u8 recLen;
    u8 earLen;
    u32 lbn;
    u32 lbnMsb;
    u32 size;
    u32 sizeMsb;
    IsoDirectoryDateTime dateTime;
    u8 flags;
    u8 fileUnitSize;
    u8 interleaveGap;
    u16 volumeSequenceNumber;
    u16 volumeSequenceNumberMsb;
    u8 nameLen;
    char name[];
} IsoDirectory;

/** ISO path record as defined in the ISO specs.
 * Path table is used to speed-up lookup for directories. */
typedef struct __attribute__((packed)) {
    u8 nameLen;
    u8 earLen;
    u32 lbn;
    u16 parentNum;
    char name[];
} IsoPath;

/** Represents parsed ISO path record. Most notably it has fixed size name and few additional fields. */
typedef struct {
    u8 nameLen;
    u8 earLen;
    __attribute__((packed)) u32 lbn; // Only this field is packed, the rest is not to avoid unnecessary unaligned loads.
    u16 parentNum;
    char name[32];
    u32 dirNum;
    u32 dirLevel;
    u32 lbnSize;
    u32 unk34; // Unused
} IsofsPath;

/** Global manager data for this module. */
typedef struct {
    s32 initialized;
    s32 unk4; // Unused
    s32 unk8; // Unused
    s32 maxUnits;
    s32 maxFiles;
    s32 fdwCount;
    SceUID semaId;
    SceUID heapId;
    SceUID fplId;
    s32 driveType;
} IsofsMgr;

typedef struct IsofsFdw IsofsFdw;
typedef struct IsofsFd IsofsFd;
typedef struct IsofsUnit IsofsUnit;

/** Stores information about opened ISO file. */
typedef struct {
    u32 readFlags;
    u8 openFlags; // 3 = raw open, 2 = normal open, 0 = available
    u8 flags;
    s8 usedFlag;
    u8 unk7; // Always zero
    u16 unk8; // Always zero
    s16 unkA; // Unknown, cache related
    u32 lbn;
    u32 unk10; // Always zero
    u32 unk14; // Always zero
    SceOff size;
    SceOff fpos;
    ScePspDateTime dateTime;
    u32 unk38[4]; // Unused
    ScePspDateTime dateTime2;
    SceIoIob *iob;
    IsofsFd *fd;
    char umdId[UMD_ID_SIZE];
} IsofsFile;

typedef struct IsofsDir IsofsDir;

/** Stores information about opened ISO directory. */
struct IsofsDir {
    IsoDirectory *isoDir;
    IsofsPath *paths;
    void *fplData;
    u32 lbnSize;
    u32 dirRecLen;
    u32 lbn;
    char umdId[UMD_ID_SIZE];
    u32 compatFlag;
    IsofsDir *prevDir;
    IsofsDir *nextDir;
};

/** Holds a list of opened file descriptors. A single unit can have more than one list. */
struct IsofsFd {
    u16 unk0; // Unused
    u16 filesCount;
    u32 idx;
    u32 unk8; // Unused
    u32 unkC; // Unused
    SceUID heapId;
    u32 unk14; // Unused
    IsofsFile *files;
    IsofsFdw *fdw;
    IsofsUnit *unit;
};

/** Wraps IsofsFd for unknown reason. */
struct IsofsFdw {
    u32 idx;
    u32 unk4; // Always zero
    IsofsFd fd;
};

/** Represents single device capable of reading ISOs. */
struct IsofsUnit {
    char umdId[UMD_ID_SIZE];
    char blockDev[32];
    SceUID heapId;
    u16 unk44; // Always zero
    s16 unk46; // Always -1
    s16 unitNum;
    u16 unk4a; // Always 1
    u16 mountFlags;
    u16 pathTableEntryCount;
    u8 fdwCount;
    u8 unk51; // Unused
    u16 mountState;
    u32 pathTableSize;
    SceUID semaId;
    u32 pathTableLbn;
    u32 rootDirLbn;
    u32 rootDirSize;
    u32 unkNextLbn;
    u32 lastLbn;
    u32 unkLbnTillEnd;
    u32 totalSectors;
    u32 unkCurrentLbn;
    u32 unk7C; // Unused
    u32 unk80; // Unused
    u32 unk84; // Unused
    u32 unk88; // Unused
    u32 unk8c; // Unused
    IsofsPath *paths;
    IsofsFdw *fdw;
    IsoPath *pathTable;
    u8 *primaryVolumeDescriptor;
    u32 openState;
    u32 currentLbn; // current UMD lbn?
    u32 sectorSize;
    s32 handlerIdx;
};

/** Functions used by ISOFS unit to interface with the actual device reading the ISO. */
typedef struct {
    s32 (*readDirSectors)(IsofsUnit *, s64, s32, void *, s32, u32);
    s32 (*readSectors)(IsofsUnit *, IsofsFile *, s32, void *, s32, u32);
    void *unk8; // Unused
    s32 (*getSectorSize)(IsofsUnit *);
    s32 (*getTotalNumberOfSectors)(IsofsUnit *);
    s32 (*getCurrentLbn)(IsofsUnit *);
    s64 (*seek)(IsofsUnit *, s64);
    s32 (*unk1C)(IsofsUnit *, s32, void *, SceSize, s32, s32);
    s32 (*unk20)();
    s32 (*getUnitNum)(IsofsUnit *, void *, SceSize);
    s32 (*readPvd)(IsofsUnit *, void *, SceSize);
    s32 (*prepareIntoCache)(IsofsUnit *, IsofsFile *, s32, s32);
    s32 (*unk30)(IsofsUnit *, IsofsFile *, s32);
    s32 (*unk34)(IsofsUnit *, IsofsFile *);
    s32 (*unk38)(IsofsUnit *, IsofsFile *);
} IsofsImplFuncs;

/** Definition of device reading the ISO. */
typedef struct {
    char blockDev[32];
    IsofsImplFuncs *funcs;
} IsofsHandler;

/** ISOFS cache entry. This cache doesn't seem to be used so most fields are unknown. */
typedef struct {
    u32 unk0;
    u32 unk4;
    u32 unk8;
    u32 unkC;
    u32 unk10;
    u32 unk14;
    u32 unk18;
    u32 unk1C;
    s8 used;
    s8 unk21;
    s16 unk22;
    u32 unk24;
    u32 unk28;
    void *memory;
} IsofsCacheEntry;

/** Input args for reading with the sceKernelExtendKernelStack function. */
typedef struct {
    IsofsUnit *unit;
    IsofsFile *file;
    u32 flags;
    s32 outsize;
    void *outdata;
    u32 unk14; // Unused, always 0
} IsofsReadSectorsArgs;

/**
 * Declarations
 */

/** module_start function. */
s32 isofsInit();
/** module_reboot_before function. */
s32 isofsRebootBefore();

/** Returns global ISOFS manager. */
IsofsMgr *isofsGetMgr();

/** Initializes all global units. */
s32 isofsInitUnits();
/** Frees resources for all global units. */
s32 isofsFreeUnits();
/** Returns global unit by its number. */
IsofsUnit *isofsGetUnit(s32 unitNum);
/** Clears unit's fields, e.g. after unmount. */
s32 isofsClearUnit(s32 unitNum);

/** Initializes global cache. */
s32 isofsInitCache();
/** Frees global cache. */
s32 isofsFreeCache();
/** Frees global cache data. */
s32 isofsFreeCacheData();

// Driver functions
int isofsDrvInit(SceIoDeviceEntry *entry);
int isofsDrvExit(SceIoDeviceEntry *entry);
int isofsDrvOpen(SceIoIob *iob, char *path, int flags, SceMode mode);
int isofsDrvClose(SceIoIob *iob);
s32 isofsDrvRead(SceIoIob *iob, void *outdata, SceSize size);
SceOff isofsDrvLseek(SceIoIob *iob, SceOff ofs, int whence);
int isofsDrvIoctl(SceIoIob *iob, int cmd, void *indata, SceSize inlen, void *outdata, SceSize outlen);
int isofsDrvDopen(SceIoIob *iob, char *path);
int isofsDrvDclose(SceIoIob *iob);
int isofsDrvDread(SceIoIob *iob, SceIoDirent *dirent);
int isofsDrvGetstat(SceIoIob *iob, char *name, SceIoStat *stat);
int isofsDrvChdir(SceIoIob *iob, char *path);
int isofsDrvMount(SceIoIob *iob, const char *fs, const char *blockDev, int mode, void *outdata, int outlen);
int isofsDrvUmount(SceIoIob *iob, char *blockDev);
int isofsDrvDevctl(SceIoIob *iob, char *dev, int cmd, void *indata, SceSize inlen, void *outdata, SceSize outlen);

/** Opens file from the ISO directory descriptor. */
IsofsFile *isofsOpen(IsofsUnit *unit, SceIoIob *iob, IsoDirectory *isoDir, s32 *outRet);
/** Opens file directly by the specified LBN and size params. */
IsofsFile *isofsOpenRaw(IsofsUnit *unit, SceIoIob *iob, u32 lbn, u32 size, s32 *outRet);
/** Seek to specified position in the file. */
s64 isofsLseek(IsofsFile *file, s64 pos, int whence);
/** Main ioctl implementation. */
s32 isofsIoctl(SceIoIob *iob, s32 cmd, void *indata, SceSize inlen, void *outdata, SceSize outlen);

// Rather simple functions to initialize and free file descriptors related structs.
s32 isofsInitUnitFdw(IsofsUnit *unit, u32 fdwCount, u16 filesCount);
void isofsFreeUnitFdw(IsofsUnit *unit);
s32 isofsInitFdw(IsofsFdw *fdw, u32 idx, u16 filesCount);
void isofsClearFdwFiles(IsofsFdw *fdw, s32 fdwCount);
s32 isofsClearUnitFilesFdw(IsofsUnit *unit, s32 fdwCount);
s32 isofsInitFdFiles(IsofsFd *fd, u16 filesCount);
s32 isofsFreeFdFiles(IsofsFd *fd);
s32 isofsFreeFiles(IsofsFile *files);
s32 isofsClearFile(IsofsFile *file);

/** Set file params from ISO directory records. */
IsofsFile *isofsSetFileFromDir(IsofsFile *file, IsoDirectory *isoDir, SceIoIob *iob, s32 *outRet);
/** Set file params for raw open with provided LBN and size. */
IsofsFile *isofsSetFileFromRaw(IsofsFile *file, SceIoIob *iob, u32 lbn, u32 size, s32 *outRet);
/** Find unused file descriptor for normal open. */
IsofsFile *isofsFindUnusedFile(IsofsUnit *unit, IsoDirectory *isoDir, s32 *outRet);
/** Find unused file descriptor for raw open. */
IsofsFile *isofsFindUnusedFileRaw(IsofsUnit *unit, u32 lbn, s32 *outRet);

/** Select implementation handlers based on the block device name. */
s32 isofsSetBlockDevHandler(IsofsUnit *unit);
/** Clears selected implementation handlers. */
s32 isofsClearBlockDevHandler(IsofsUnit *unit);

/** Initialize unit with data from the ISO's primary volume descriptor (PVD). */
s32 isofsReadPvd(char *blockDev, s32 unitNum);
/** Reads ISO directory into the output buffer. */
int isofsReadDir(IsofsUnit *unit, u32 lbn, void *outdata);
/** Wraps sector reading function for use with sceKernelExtendKernelStack. */
s32 isofsHandlerReadSectorsArgs(void *args);

/** Checks if the path is in the "/sce_lbn" format (used for raw read from the ISO). */
s32 isofsIsSceLbnPath(char *path);
/** Extracts LBN and size from the "/sce_lbn" path. */
s32 isofsParseSceLbnPath(char *path, s32 *outLbn, s32 *outSize);

/** Reads and parses ISO path table to create unit's paths. */
s32 isofsCreatePaths(IsofsUnit *unit);
/** Frees unit paths. */
s32 isofsFreePaths(IsofsUnit *unit);
/** Frees unit paths. (effectively same as isofsFreePaths) */
s32 isofsFreeUnitPaths(IsofsUnit *unit);
/** Reads ISO path table. */
IsoPath *isofsReadPathTable(IsofsUnit *unit, s32 *outRet);
/** Counts entries in the ISO path table. */
s32 isofsCountPathTableEntries(IsoPath *pathTable, s32 pathTableSize, s32 *outRet);
/** Parses entries in the ISO path table and converts them to ISOFS path struct. */
IsofsPath *isofsParsePathTableEntries(IsofsUnit *unit, IsoPath *pathTable, s32 pathTableSize, s32 pathTableEntryCount,
                                      s32 *outRet);
/** Calculate and update LBN size of the path table. */
s32 isofsUpdatePathsLbnSize(IsofsUnit *unit);

/** Check if the unit is already mounted. */
s32 isofsCheckUnitMounted(IsofsUnit *unit, s32 unitNum);
/**
 * Sets some LBN related field in the unit based on the data from device.
 * Those fields aren't read anywhere so the exact purpose is unknown.
 */
s32 isofsUnitSetLbns(char *blockDev, s32 unitNum);
/** Clear global variable with the current directory LBN. */
void isofsClearCurrentDirLbn();
/** Remove directory for the open directories linked list. */
void isofsUnlinkOpenedDir(IsofsDir *dir);
s32 isofsCheckMode(SceMode mode);
/** Checks paths for invalid characters. */
s32 isofsCheckPath(char *name, u32 flag);
/** Find ISO directory entry by name. */
IsoDirectory *isofsFindIsoDirectory(char *path, IsoDirectory *isoDirs, s32 flag, s32 *outRet);
/** Set dirent fields from ISO directory. */
IsoDirectory *isofsSetDirentFromIsoDirectory(IsofsDir *dir, SceIoDirent *dirent, s32 *outRet);
/** Read ISO directory for file open or stat. */
IsoDirectory *isofsReadIsoDir(IsofsUnit *unit, char *path, char *file, u32 flag, s32 *outRet);
/** Find the path record for directory. */
IsofsPath *isofsFindPath(IsofsUnit *unit, char *pathSplit, u32 dirLevel, int pathLen, u32 flag, s32 *outRet);
/** Split path by slash separator. */
char *isofsSplitPath(char *path, u32 *outLen, u32 flag, s32 *outRet);

// Simple functions delegating work to the actual device implementation.
s32 isofsHandlerReadDirSectors(IsofsUnit *unit, s64 lbn, s32 size, void *outdata, s32 outlen, s32 cmd, u32 unkUnused);
s32 isofsHandlerReadSectors(IsofsUnit *unit, IsofsFile *file, s32 size, void *outdata, s32 outlen, u32 unkFlags,
                            s32 unkUnused);
s32 isofsHandlerGetSectorSize(IsofsUnit *unit);
s32 isofsHandlerGetTotalNumberOfSectors(IsofsUnit *unit);
s32 isofsHandlerGetCurrentLbn(IsofsUnit *unit);
s64 isofsHandlerSeek(IsofsUnit *unit, s64 lbn);
s32 isofsHandler_unk1c(IsofsUnit *unit, s32 unk0, void *indata, SceSize inlen, s32 unk1, s32 unk2);
s32 isofsHandlerReadPvd(IsofsUnit *unit, void *indata, SceSize inlen);
s32 isofsHandlerPrepareIntoCache(IsofsUnit *unit, IsofsFile *file, s32 cacheSize, s32 getStatusFlag);
s32 isofsHandler_unk30(IsofsUnit *unit, IsofsFile *file, s32 flag, void *outdata);
s32 isofsHandler_unk34(IsofsUnit *unit, IsofsFile *file);
s32 isofsHandler_unk38(IsofsUnit *unit, IsofsFile *file);

// All IsofsImplFuncs for interacting with the UMD device.
s32 isofsUmdReadDirSectors(IsofsUnit *unit, s64 lbn, s32 size, void *outdata, s32 outlen, u32 cmd);
s32 isofsUmdReadSectors(IsofsUnit *unit, IsofsFile *file, s32 size, void *outdata, s32 outlen, u32 cmd);
s32 isofsUmdGetSectorSize(IsofsUnit *unit);
s32 isofsUmdGetTotalNumberOfSectors(IsofsUnit *unit);
s32 isofsUmdGetCurrentLbn(IsofsUnit *unit);
s64 isofsUmdSeek(IsofsUnit *unit, s64 lbn);
s32 isofsUmd_1e180d3(IsofsUnit *unit, s32 unk, void *indata, SceSize inlen, s32 unk0, s32 unk1);
s32 isofsUmd_unkNotSupported();
s32 isofsUmdGetUnitNum(IsofsUnit *unit, void *indata, SceSize inlen);
s32 isofsUmdReadPvd(IsofsUnit *unit, void *indata, SceSize inlen);
s32 isofsUmdPrepareIntoCache(IsofsUnit *unit, IsofsFile *file, s32 cacheSize, s32 getStatusFlag);
s32 isofsUmd_1f100a6_1f100a7(IsofsUnit *unit, IsofsFile *file, s32 flag);
s32 isofsUmd_1f100a8(IsofsUnit *unit, IsofsFile *file);
s32 isofsUmd_1f100a9(IsofsUnit *unit, IsofsFile *file);

// UMD callbacks management and implementation.
s32 isofsRegisterOnUmdMounted(IsofsUnit *unit);
s32 isofsUmdManUnRegisterInsertEjectUMDCallBack();
s32 isofsUmdMountedCb(int id, IsofsUnit *unit, int unk);

/** Sets params for reading from UMD assuming that current file position is at 0. */
s32 isofsReadParamsZero(IsofsUnit *unit, IsofsFile *file, void *outdata, s32 size, LbnParams *params);
/** Sets params for reading from UMD assuming that current file position is less than sector size. */
s32 isofsReadParamsInitial(IsofsUnit *unit, IsofsFile *file, void *outdata, s32 size, LbnParams *params);
/** Sets params for reading from UMD for other cases without assumptions. */
s32 isofsReadParams(IsofsUnit *unit, IsofsFile *file, void *outdata, s32 size, LbnParams *params);

/**
 * Globals
 */

// 00006d04
SceIoDeviceFunction g_isofsDrvFuncs = {
    isofsDrvInit,
    isofsDrvExit,
    isofsDrvOpen,
    isofsDrvClose,
    isofsDrvRead,
    NULL,
    isofsDrvLseek,
    isofsDrvIoctl,
    NULL,
    NULL,
    NULL,
    isofsDrvDopen,
    isofsDrvDclose,
    isofsDrvDread,
    isofsDrvGetstat,
    NULL,
    NULL,
    isofsDrvChdir,
    isofsDrvMount,
    isofsDrvUmount,
    isofsDrvDevctl,
    NULL
};

// 00006d5c
SceIoDeviceTable g_isofsDrv = {"isofs", 0x10, 0x800, "ISOFS", &g_isofsDrvFuncs};

// 00006d74
IsofsImplFuncs g_isofsUmdFuncs = {
    isofsUmdReadDirSectors,
    isofsUmdReadSectors,
    NULL,
    isofsUmdGetSectorSize,
    isofsUmdGetTotalNumberOfSectors,
    isofsUmdGetCurrentLbn,
    isofsUmdSeek,
    isofsUmd_1e180d3,
    isofsUmd_unkNotSupported,
    isofsUmdGetUnitNum,
    isofsUmdReadPvd,
    isofsUmdPrepareIntoCache,
    isofsUmd_1f100a6_1f100a7,
    isofsUmd_1f100a8,
    isofsUmd_1f100a9
};

// 00006db0
IsofsHandler g_isofsHandlers[] = {
    {{'u', 'm', 'd', '\0'}, &g_isofsUmdFuncs},
    {{'n', 'u', 'l', 'l', '\0'}, NULL},
    {{'\0'}, NULL},
    {{'\0'}, NULL}
};

// 00006e80
IsofsUnit *g_isofsUnits;

// 00006e84
IsofsCacheEntry *g_isofsCache;

// 00006ec0
u8 __attribute__((aligned(64))) g_isofsCurrentDir[4096];

// 00007ec0;
s32 g_isofsCurrentDirLbn;

// 00007ec4
s32 g_isofsMounted2;

// 00007ec8
s32 g_isofsMounted;

// 00007ecc
s32 g_isofsInitialized;

// 00007ed0
IsofsDir *g_isofsOpenDirs;

// 00007ed4
IsofsMgr g_isofsMgr;

/**
 * Implementation
 */

static inline void _setUmdId(char *umdId, int value) {
    s32 *id = (s32 *) umdId;
    id[0] = value;
    id[1] = value;
    id[2] = value;
    id[3] = value;
    id[4] = value;
    id[5] = value;
    id[6] = value;
    id[7] = value;
}

// 00000000
s32 isofsInitUnits() {
    IsofsMgr *mgr = isofsGetMgr();
    sceKernelWaitSema(mgr->semaId, 1, NULL);
    g_isofsUnits = (IsofsUnit *) sceKernelAllocHeapMemory(mgr->heapId, mgr->maxUnits * sizeof(IsofsUnit));
    if (g_isofsUnits == NULL) {
        sceKernelSignalSema(mgr->semaId, 1);
        return SCE_ERROR_ERRNO_NO_MEMORY;
    }
    s32 ret = SCE_ERROR_OK;
    for (s32 i = 0; i < mgr->maxUnits; ++i) {
        IsofsUnit *unit = &g_isofsUnits[i];
        unit->fdwCount = i; // weird
        unit->sectorSize = 0;
        unit->pathTableSize = 0;
        unit->rootDirLbn = 0;
        unit->rootDirSize = 0;
        unit->unkNextLbn = 0;
        unit->lastLbn = 0;
        unit->unkLbnTillEnd = 0;
        unit->pathTableLbn = 0;
        unit->totalSectors = 0;
        unit->unkCurrentLbn = 0;
        unit->unk44 = 0;
        unit->heapId = mgr->heapId;
        unit->unk46 = -1;
        unit->unitNum = -1;
        unit->mountFlags = 0;
        unit->unk4a = 1;
        unit->pathTableEntryCount = 0;
        unit->mountState = 0;
        unit->fdw = NULL;
        unit->paths = NULL;
        unit->pathTable = NULL;
        unit->primaryVolumeDescriptor = NULL;
        unit->openState = 0;
        _setUmdId(unit->umdId, 0);
        if (mgr->fdwCount == 0) {
            pspBreak(SCE_BREAKCODE_DIVZERO);
        }
        ret = isofsInitUnitFdw(unit, mgr->fdwCount, mgr->maxFiles / mgr->fdwCount);
        if (ret != 0) {
            break;
        }
    }
    sceKernelSignalSema(mgr->semaId, 1);
    return ret;
}

// 0000019c
s32 isofsFreeUnits() {
    IsofsMgr *mgr = isofsGetMgr();
    sceKernelWaitSema(mgr->semaId, 1, NULL);
    for (s32 i = 0; i < mgr->maxUnits; ++i) {
        IsofsUnit *unit = &g_isofsUnits[i];
        isofsFreeUnitFdw(unit);
        sceKernelDeleteSema(unit->semaId);
    }
    sceKernelFreeHeapMemory(mgr->heapId, g_isofsUnits);
    g_isofsUnits = NULL;
    sceKernelSignalSema(mgr->semaId, 1);
    return SCE_ERROR_OK;
}

// 00000254
IsofsUnit *isofsGetUnit(s32 unitNum) {
    IsofsMgr *mgr = isofsGetMgr();
    if (((u32) unitNum >> 0x1f | (mgr == NULL)) != 0 || unitNum >= mgr->maxUnits) {
        return NULL;
    }
    return &g_isofsUnits[unitNum];
}

// 000002bc
s32 isofsInitUnitFdw(IsofsUnit *unit, u32 fdwCount, u16 filesCount) {
    unit->fdw = (IsofsFdw *) sceKernelAllocHeapMemory(unit->heapId, fdwCount * sizeof(IsofsFdw));
    if (unit->fdw == NULL) {
        return SCE_ERROR_ERRNO_NO_MEMORY;
    }
    memset(unit->fdw, 0, fdwCount * sizeof(IsofsFdw));
    for (u32 i = 0; i < fdwCount; i++) {
        s32 ret = isofsInitFdw(&unit->fdw[i], i, filesCount);
        if (ret != 0) {
            sceKernelFreeHeapMemory(unit->heapId, unit->fdw);
            return ret;
        }
        unit->fdw[i].idx = i;
        unit->fdw[i].fd.unit = unit;
        unit->fdw[i].unk4 = 0;
    }
    unit->fdwCount = (u8) fdwCount;
    return SCE_ERROR_OK;
}

// 000003c4
void isofsFreeUnitFdw(IsofsUnit *unit) {
    for (u8 i = 0; i < unit->fdwCount; i++) {
        isofsFreeFdFiles(&unit->fdw[i].fd);
    }
    sceKernelFreeHeapMemory(unit->heapId, unit->fdw);
}

// 00000434
s32 isofsClearUnitFilesFdw(IsofsUnit *unit, s32 fdwCount) {
    isofsClearFdwFiles(unit->fdw, fdwCount);
    return SCE_ERROR_OK;
}

// 00000454
s32 isofsInitCache() {
    IsofsMgr *mgr = isofsGetMgr();
    g_isofsCache = (IsofsCacheEntry *) sceKernelAllocHeapMemory(mgr->heapId,
                                                                sizeof(IsofsCacheEntry) * ISOFS_CACHE_ENTRIES);
    if (g_isofsCache == NULL) {
        return SCE_ERROR_ERRNO_NO_MEMORY;
    }
    int fplId = sceKernelCreateFpl("SceIsofsCacheData", 1, 0, 0x1000, 8, NULL);
    if (fplId < 0) {
        sceKernelFreeHeapMemory(mgr->heapId, g_isofsCache);
        return fplId;
    }
    mgr->fplId = fplId;
    for (u32 i = 0; i < ISOFS_CACHE_ENTRIES; i++) {
        IsofsCacheEntry *entry = &g_isofsCache[i];
        entry->used = 0;
        entry->unk0 = 0;
        entry->unk4 = 0;
        entry->unk8 = 0;
        entry->unkC = 0;
        entry->unk10 = 0;
        entry->unk14 = 0;
        entry->unk18 = 0;
        entry->unk1C = 0;
        entry->unk22 = 0;
        entry->unk24 = 0;
        entry->unk28 = 0;
        entry->memory = NULL;
    }
    return SCE_ERROR_OK;
}

// 00000550
s32 isofsFreeCacheData() {
    IsofsMgr *mgr = isofsGetMgr();
    for (u32 i = 0; i < ISOFS_CACHE_ENTRIES; i++) {
        IsofsCacheEntry *entry = &g_isofsCache[i];
        if (entry->used == 1) {
            sceKernelFreeHeapMemory(mgr->fplId, entry->memory);
        }
    }
    return SCE_ERROR_OK;
}

// 000005dc
s32 isofsFreeCache() {
    IsofsMgr *mgr = isofsGetMgr();
    isofsFreeCacheData();
    sceKernelFreeHeapMemory(mgr->heapId, g_isofsCache);
    sceKernelDeleteFpl(mgr->fplId);
    return SCE_ERROR_OK;
}

// 00000620
s32 isofsInitFdw(IsofsFdw *fdw, u32 idx, u16 filesCount) {
    fdw->fd.idx = idx;
    fdw->fd.fdw = fdw;
    fdw->fd.filesCount = filesCount;
    return isofsInitFdFiles(&fdw->fd, filesCount);
}

// 00000650
s32 isofsFreeFdFiles(IsofsFd *fd) {
    s32 ret = isofsFreeFiles(fd->files);
    fd->files = NULL;
    return ret;
}

// 0000067c
void isofsClearFdwFiles(IsofsFdw *fdw, s32 fdwCount) {
    IsofsFile *files = fdw->fd.files;
    for (int i = 0; i < fdwCount / 2; i++) {
        IsofsFile *file = &files[i];
        file->openFlags = 0;
        file->flags = 0;
        file->unk7 = 0;
        file->fd = &fdw->fd;
        file->size = 0;
        file->fpos = 0;
        file->unk8 = 0;
        file->unkA = -1;
        file->readFlags = 0;
        file->lbn = 0;
        file->unk10 = 0;
        file->unk14 = 0;
        file->iob = NULL;
    }
}

// 00000718
s32 isofsInitFdFiles(IsofsFd *fd, u16 filesCount) {
    IsofsMgr *mgr = isofsGetMgr();

    fd->heapId = mgr->heapId;
    fd->files = (IsofsFile *) sceKernelAllocHeapMemory(mgr->heapId, filesCount * sizeof(IsofsFile));
    if (fd->files == NULL) {
        return SCE_ERROR_ERRNO_NO_MEMORY;
    }
    for (u16 i = 0; i < filesCount; i++) {
        IsofsFile *file = &fd->files[i];
        file->openFlags = 0;
        file->flags = 0;
        file->usedFlag = 0;
        file->unk7 = 0;
        file->fd = fd;
        file->size = 0;
        file->fpos = 0;
        file->unk8 = 0;
        file->unkA = -1;
        file->readFlags = 0;
        file->lbn = 0;
        file->unk10 = 0;
        file->unk14 = 0;
        file->iob = NULL;
    }
    return SCE_ERROR_OK;
}

// 000007fc
IsofsFile *isofsOpen(IsofsUnit *unit, SceIoIob *iob, IsoDirectory *isoDir, s32 *outRet) {
    if (unit == NULL || iob == NULL) {
        *outRet = SCE_ERROR_ERRNO_INVALID_ARGUMENT;
        return NULL;
    }
    IsofsFile *file = isofsFindUnusedFile(unit, isoDir, outRet);
    if (file == NULL) {
        isofsClearFile(file);
        return NULL;
    }
    isofsSetFileFromDir(file, isoDir, iob, outRet);
    s32 intr = sceKernelCpuSuspendIntr();
    if (unit->openState == 2) {
        sceKernelCpuResumeIntr(intr);
        isofsClearFile(file);
        return NULL;
    }
    __builtin_memcpy(file->umdId, unit->umdId, UMD_ID_SIZE);
    file->flags = 1;
    sceKernelCpuResumeIntr(intr);
    return file;
}

// 00000924
IsofsFile *isofsOpenRaw(IsofsUnit *unit, SceIoIob *iob, u32 lbn, u32 size, s32 *outRet) {
    if (unit == NULL || iob == NULL) {
        *outRet = SCE_ERROR_ERRNO_INVALID_ARGUMENT;
        return NULL;
    }
    IsofsFile *file = isofsFindUnusedFileRaw(unit, lbn, outRet);
    if (file == NULL) {
        isofsClearFile(file);
        return NULL;
    }
    isofsSetFileFromRaw(file, iob, lbn, size, outRet);
    s32 intr = sceKernelCpuSuspendIntr();
    if (unit->openState == 2) {
        sceKernelCpuResumeIntr(intr);
        isofsClearFile(file);
        return NULL;
    }
    __builtin_memcpy(file->umdId, unit->umdId, UMD_ID_SIZE);
    file->flags = 1;
    sceKernelCpuResumeIntr(intr);
    return file;
}

// 00000a58
IsofsFile *isofsSetFileFromDir(IsofsFile *file, IsoDirectory *isoDir, SceIoIob *iob, s32 *outRet) {
    if (iob == NULL || file == NULL || isoDir == NULL) {
        *outRet = SCE_ERROR_ERRNO_INVALID_ARGUMENT;
        return NULL;
    }
    *outRet = SCE_ERROR_OK;
    file->fpos = 0;
    file->openFlags = 2;
    file->iob = iob;
    file->usedFlag = 1;
    file->readFlags = 0;
    file->lbn = isoDir->lbn;
    file->size = isoDir->size;
    u16 year = isoDir->dateTime.yearsSince1900 + 1900;
    file->dateTime.year = year;
    file->dateTime2.year = year;
    file->dateTime.month = isoDir->dateTime.month;
    file->dateTime2.month = isoDir->dateTime.month;
    file->dateTime.day = isoDir->dateTime.day;
    file->dateTime2.day = isoDir->dateTime.day;
    file->dateTime.hour = isoDir->dateTime.hour;
    file->dateTime2.hour = isoDir->dateTime.hour;
    file->dateTime.minute = isoDir->dateTime.minute;
    file->dateTime2.minute = isoDir->dateTime.minute;
    file->dateTime.second = isoDir->dateTime.second;
    file->dateTime2.second = isoDir->dateTime.second;
    file->dateTime.microsecond = 0;
    file->dateTime2.microsecond = 0;
    return file;
}

// 00000b74
IsofsFile *isofsSetFileFromRaw(IsofsFile *file, SceIoIob *iob, u32 lbn, u32 size, s32 *outRet) {
    if (iob == NULL || file == NULL) {
        *outRet = SCE_ERROR_ERRNO_INVALID_ARGUMENT;
        return NULL;
    }
    *outRet = SCE_ERROR_OK;
    file->iob = iob;
    file->usedFlag = 1;
    file->fpos = 0;
    file->openFlags = 3;
    file->lbn = lbn;
    file->readFlags = 0;
    file->size = size;
    file->dateTime.year = 0;
    file->dateTime2.year = 0;
    file->dateTime.month = 0;
    file->dateTime2.month = 0;
    file->dateTime.day = 0;
    file->dateTime2.day = 0;
    file->dateTime.hour = 0;
    file->dateTime2.hour = 0;
    file->dateTime.minute = 0;
    file->dateTime2.minute = 0;
    file->dateTime.second = 0;
    file->dateTime2.second = 0;
    file->dateTime.microsecond = 0;
    file->dateTime2.microsecond = 0;
    return file;
}

// 00000c20
s32 isofsClearFile(IsofsFile *file) {
    if (file == NULL || file->iob == NULL || file->usedFlag <= 0) {
        return SCE_ERROR_ERRNO_INVALID_ARGUMENT;
    }
    file->size = 0;
    file->fpos = 0;
    file->iob = NULL;
    file->readFlags = 0;
    file->lbn = 0;
    file->flags = 0;
    file->openFlags = 0;
    file->usedFlag = 0;
    file->dateTime.year = 0;
    file->dateTime2.year = 0;
    file->dateTime.month = 0;
    file->dateTime2.month = 0;
    file->dateTime.day = 0;
    file->dateTime2.day = 0;
    file->dateTime.hour = 0;
    file->dateTime2.hour = 0;
    file->dateTime.minute = 0;
    file->dateTime2.minute = 0;
    file->dateTime.second = 0;
    file->dateTime2.second = 0;
    file->dateTime.microsecond = 0;
    file->dateTime2.microsecond = 0;
    file->unkA = -1;
    return SCE_ERROR_OK;
}

// 00000cc0
s32 isofsFreeFiles(IsofsFile *files) {
    // bug, files accessed before null check
    // gcc will optimize away the `files == NULL` check
    IsofsFd *fd = files->fd;
    if (files == NULL || fd == NULL) {
        return SCE_ERROR_ERRNO_INVALID_ARGUMENT;
    }
    return sceKernelFreeHeapMemory(fd->heapId, fd->files);
}

// Not in module. Avoids code duplication.
static inline IsofsFile *_isofsFindUnusedFile(IsofsUnit *unit, u32 lbn, s32 *outRet) {
    *outRet = SCE_ERROR_OK;
    if (unit == NULL) {
        *outRet = SCE_ERROR_ERRNO_DEVICE_NOT_FOUND;
        return NULL;
    }
    if (unit->fdwCount == 1 && unit->totalSectors < lbn) {
        IsofsFd fd = unit->fdw[1].fd;
        u16 filesCount = fd.filesCount;
        for (u16 i = 0; i < filesCount; i++) {
            IsofsFile *file = &fd.files[i];
            if (file->iob == NULL && file->flags == 0) {
                return file;
            }
        }
        *outRet = SCE_ERROR_ERRNO_TOO_MANY_OPEN_SYSTEM_FILES;
        return NULL;
    }

    IsofsFd fd = unit->fdw[0].fd;
    u16 filesCount = fd.filesCount;
    for (u16 i = 0; i < filesCount; i++) {
        IsofsFile *file = &fd.files[i];
        if (file->iob == NULL && file->flags == 0) {
            return file;
        }
    }
    *outRet = SCE_ERROR_ERRNO_TOO_MANY_OPEN_SYSTEM_FILES;
    return NULL;
}

// 00000d04
IsofsFile *isofsFindUnusedFile(IsofsUnit *unit, IsoDirectory *isoDir, s32 *outRet) {
    return _isofsFindUnusedFile(unit, isoDir->lbn, outRet);
}

// 00000e14
IsofsFile *isofsFindUnusedFileRaw(IsofsUnit *unit, u32 lbn, s32 *outRet) {
    return _isofsFindUnusedFile(unit, lbn, outRet);
}

// 00000ef4
IsoDirectory *isofsFindIsoDirectory(char *path, IsoDirectory *isoDirs, s32 flag, s32 *outRet) {
    if (path == NULL || isoDirs == NULL || flag < 1 || outRet == NULL) {
        *outRet = SCE_ERROR_ERRNO_INVALID_ARGUMENT;
        // bug, there is no return here!
    }
    *outRet = SCE_ERROR_ERRNO_FILE_NOT_FOUND;

    char curPath[32];
    __builtin_memset(curPath, 0, 32);

    char *lastSlash = strrchr(path, '/');
    if (lastSlash == NULL) {
        *outRet = SCE_ERROR_ERRNO_INVALID_ARGUMENT;
        return NULL;
    }
    u32 lastSegLen = strlen(lastSlash + 1);
    if (lastSegLen >= 0x20) {
        *outRet = SCE_ERROR_ERRNO_NAME_TOO_LONG;
        return NULL;
    }

    memcpy(curPath, lastSlash + 1, lastSegLen);
    curPath[lastSegLen] = 0;

    u32 offset = 0;
    IsoDirectory *dir = isoDirs;
    while (1) {
        u32 recLen = dir->recLen;
        if (recLen == 0) {
            if (flag < 2) {
                return NULL;
            }
            while (dir->recLen == 0) {
                dir = (IsoDirectory *) (&isoDirs->earLen + offset);
                offset = offset + 1;
            }
            recLen = (u32) dir->recLen;
        }
        if (dir->name[0] != 0 && dir->name[0] != 1) {
            if (strcmp(curPath, dir->name) == 0) {
                break;
            }
        }
        offset = offset + recLen;
        dir = (IsoDirectory *) (&isoDirs->recLen + offset);
        if ((s32) offset > 0x1000) {
            return NULL;
        }
    }
    *outRet = SCE_ERROR_OK;
    return dir;
}

// 00001094
IsofsPath *isofsFindPath(IsofsUnit *unit, char *pathSplit, u32 dirLevel, int pathLen, u32 flag, s32 *outRet) {
    if (outRet == NULL) {
        return NULL;
    }
    if (pathSplit == NULL || pathLen < 1 || ((s32) dirLevel < 1 || (s32) dirLevel > 8)) {
        *outRet = SCE_ERROR_ERRNO_INVALID_ARGUMENT;
        return NULL;
    }
    if (unit == NULL) {
        *outRet = SCE_ERROR_ERRNO_DEVICE_NOT_FOUND;
        return NULL;
    }
    if (unit->paths == NULL || unit->pathTableEntryCount == 0) {
        *outRet = SCE_ERROR_ERRNO_IO_ERROR;
        return NULL;
    }
    char pathStack[32];
    __builtin_memset(pathStack, 0, 32);
    if (dirLevel <= 1) {
        *outRet = SCE_ERROR_ERRNO_FILE_NOT_FOUND;
        return NULL;
    }
    *outRet = SCE_ERROR_ERRNO_FILE_NOT_FOUND;
    u32 curDirNum = 1;
    u32 curDirLevel = 1;
    for (u32 iDirLevel = 2; iDirLevel <= dirLevel; iDirLevel++) {
        u32 outSegLen = 0;
        pathSplit = isofsSplitPath(pathSplit, &outSegLen, flag, outRet);
        if (pathSplit == NULL) {
            return NULL;
        }
        if (outSegLen > 0x1f) {
            *outRet = SCE_ERROR_ERRNO_NAME_TOO_LONG;
            return NULL;
        }
        __builtin_memset(pathStack, 0, 32);
        memcpy(pathStack, pathSplit, outSegLen);
        pathStack[outSegLen] = 0;
        for (s32 iPath = 0; iPath < unit->pathTableEntryCount; iPath++) {
            if (unit->paths[iPath].dirLevel == iDirLevel) {
                if (strcmp(unit->paths[iPath].name, pathStack) == 0) {
                    if (unit->paths[iPath].parentNum == curDirNum) {
                        curDirNum = unit->paths[iPath].dirNum;
                        curDirLevel++;
                        *outRet = SCE_ERROR_OK;
                        if (dirLevel == curDirLevel) {
                            return &unit->paths[curDirNum - 1];
                        }
                        break;
                    }
                }
            }
            *outRet = SCE_ERROR_ERRNO_FILE_NOT_FOUND;
        }
    }
    return NULL;
}

// 000012e4
s32 isofsCreatePaths(IsofsUnit *unit) {
    if (unit == NULL) {
        return SCE_ERROR_ERRNO_INVALID_ARGUMENT;
    }
    s32 outRet;
    IsoPath *pathTable = isofsReadPathTable(unit, &outRet);
    unit->pathTable = pathTable;
    if (pathTable == NULL) {
        return outRet;
    }
    s32 pathTableEntryCount = isofsCountPathTableEntries(pathTable, unit->pathTableSize, &outRet);
    if (pathTableEntryCount > 0) {
        unit->pathTableEntryCount = (u16) pathTableEntryCount;
        IsofsPath *paths = isofsParsePathTableEntries(unit, unit->pathTable, unit->pathTableSize, pathTableEntryCount,
                                                      &outRet);
        unit->paths = paths;
        if (paths != NULL) {
            return isofsUpdatePathsLbnSize(unit);
        }
    }
    isofsFreePaths(unit);
    return outRet;
}

// 00001388
IsoPath *isofsReadPathTable(IsofsUnit *unit, s32 *outRet) {
    if (outRet == NULL) {
        return NULL;
    }
    if (unit == NULL) {
        *outRet = SCE_ERROR_ERRNO_INVALID_ARGUMENT;
        return NULL;
    }
    u32 pathTableSize = unit->pathTableSize;
    if (pathTableSize == 0) {
        *outRet = SCE_ERROR_ERRNO_INVALID_ARGUMENT;
        return NULL;
    }
    u32 sizeToRead = 0x800;
    if (pathTableSize > 0x800 && (sizeToRead = pathTableSize, (pathTableSize & 0x7ff) != 0)) {
        sizeToRead = (pathTableSize & 0xfffff800) + 0x800;
    }
    void *data = sceKernelAllocHeapMemory(unit->heapId, sizeToRead);
    if (data == NULL) {
        *outRet = SCE_ERROR_ERRNO_NO_MEMORY;
        return NULL;
    }
    s32 ret = isofsHandlerReadDirSectors(unit, unit->pathTableLbn, sizeToRead, data, sizeToRead, 6, 0);
    *outRet = ret;
    if (ret >= 0) {
        return data;
    }
    sceKernelFreeHeapMemory(unit->heapId, data);
    return NULL;
}

// 00001474
s32 isofsCountPathTableEntries(IsoPath *pathTable, s32 pathTableSize, s32 *outRet) {
    if (pathTable == NULL || pathTableSize < 9) {
        *outRet = SCE_ERROR_ERRNO_INVALID_ARGUMENT;
        return 0;
    }
    s32 count = 0;
    s32 offset = 0;
    do {
        IsoPath *path = (IsoPath *) ((u8 *) pathTable + offset);
        u8 nameLen = path->nameLen;
        s32 nextOffset = offset + (u32) nameLen;
        if (nameLen < 2 || (offset = nextOffset + 8, (nameLen & 1) != 0)) {
            offset = nextOffset + 9;
        }
        count = count + 1;
    } while (offset < pathTableSize);
    return count;
}

// 000014e0
IsofsPath *isofsParsePathTableEntries(IsofsUnit *unit, IsoPath *pathTable, s32 pathTableSize, s32 pathTableEntryCount,
                                      s32 *outRet) {
    if (outRet == NULL) {
        return NULL;
    }
    if (pathTable == NULL || pathTableEntryCount < 1 || pathTableSize < 10) {
        *outRet = SCE_ERROR_ERRNO_INVALID_ARGUMENT;
        return NULL;
    }
    IsofsPath *paths = sceKernelAllocHeapMemory(unit->heapId, pathTableEntryCount * sizeof(IsofsPath));
    if (paths == NULL) {
        *outRet = SCE_ERROR_ERRNO_NO_MEMORY;
        return NULL;
    }
    memset(paths, 0, pathTableEntryCount * sizeof(IsofsPath));

    s32 isoOffset = 0;
    IsofsPath *currentPath = paths;
    u32 prevParentNum = 1;
    u32 dirNum = 1;
    u32 dirLevel = 1;
    u32 dirLevelNext;
    s32 currentPathIndex = 0;
    u32 isoPathLen;
    const u8 nullByte = 0;
    do {
        IsoPath *isoPath = (IsoPath *) ((u8 *) pathTable + isoOffset);
        u32 nameLen = isoPath->nameLen;
        if ((nameLen < 2 || (isoPathLen = nameLen + 8, (nameLen & 1) != 0)) &&
            (nameLen == 0 || (isoPathLen = nameLen + 9, (nameLen & 1) == 0))) {
            do {
                if (isoPath->nameLen != 0) {
                    break;
                }
                isoOffset = isoOffset + 1;
                isoPath = (IsoPath *) ((u8 *) pathTable + isoOffset);
            } while (isoOffset < pathTableSize);
            nameLen = isoPath->nameLen;
            if ((nameLen < 2 || (isoPathLen = nameLen + 8, (nameLen & 1) != 0)) &&
                (isoPathLen = 0, nameLen != 0) && (isoPathLen = nameLen + 9, (nameLen & 1) == 0)) {
                isoPathLen = 0;
            }
        }

        memcpy(currentPath, isoPath, isoPathLen);
        currentPath->name[isoPathLen - 8] = 0;
        if (currentPathIndex == 0) {
            currentPath->lbnSize = 1;
        } else {
            s32 prevLbn = currentPath[-1].lbn;
            currentPath->lbnSize = 1;
            s32 lbnDiff = currentPath->lbn - prevLbn;
            currentPath[-1].lbnSize = lbnDiff;
            if (lbnDiff == 0) {
                currentPath[-1].lbnSize = 1;
            }
        }

        if (currentPath->parentNum == prevParentNum) {
            if (memcmp(currentPath->name, &nullByte, currentPath->nameLen) == 0) {
                dirLevelNext = dirLevel + 1;
                currentPath->dirLevel = dirLevel;
                currentPath->dirNum = dirNum;
            } else {
                goto hasName;
            }
        } else {
        hasName:
            if (currentPath->parentNum != prevParentNum || memcmp(
                    currentPath->name, &nullByte, currentPath->nameLen) == 0) {
                dirLevelNext = dirLevel;
                if (prevParentNum == 0) {
                    goto next;
                }
                if (paths[currentPath->parentNum - 1].dirLevel != paths[prevParentNum - 1].dirLevel) {
                    dirLevel = dirLevel + 1;
                }
            }
            dirNum = dirNum + 1;
            currentPath->dirNum = dirNum;
            currentPath->dirLevel = dirLevel;
            dirLevelNext = dirLevel;
        }

    next:
        isoOffset = isoOffset + isoPathLen;
        currentPathIndex = currentPathIndex + 1;
        prevParentNum = (u32) currentPath->parentNum;
        currentPath = currentPath + 1;
        dirLevel = dirLevelNext;
        if (pathTableSize <= isoOffset) {
            return paths;
        }
    } while (1);
}

// 0000177c
s32 isofsFreePaths(IsofsUnit *unit) {
    if (unit == NULL) {
        return SCE_ERROR_ERRNO_INVALID_ARGUMENT;
    }
    if (unit->pathTable != NULL) {
        sceKernelFreeHeapMemory(unit->heapId, unit->pathTable);
        unit->pathTable = NULL;
    }
    if (unit->paths != NULL) {
        sceKernelFreeHeapMemory(unit->heapId, unit->paths);
        unit->paths = NULL;
    }
    return SCE_ERROR_OK;
}

// 000017e4
s32 isofsCheckPath(char *name, u32 flag) {
    if (name == NULL) {
        return SCE_ERROR_ERRNO_INVALID_ARGUMENT;
    }
    s32 ret = SCE_ERROR_ERRNO_INVALID_ARGUMENT;
    if (flag >= 4) {
        ret = SCE_ERROR_ERRNO_FILE_NOT_FOUND;
    }
    if (flag < 4) {
        s32 i = 0;
        char c;
        u32 ctype;
        do {
            u32 nameLen = strlen(name);
            c = *(name + i);
            if ((int) nameLen <= i) {
                return SCE_ERROR_OK;
            }
            i++;
            ctype = look_ctype_table(c);
        } while ((ctype & 7) != 0 || c == '_' || c == ';' || c == '/' || (
                     ret = SCE_ERROR_ERRNO_FILE_NOT_FOUND, c == '.'));
    }
    return ret;
}

// 00001934
s32 isofsIsSceLbnPath(char *path) {
    if (path == NULL) {
        return 0;
    }
    char *sep;
    if (path[0] != '/' && (sep = strchr(path, '/'), sep == NULL)) {
        return 2;
    }
    if (strncmp(path, "/sce_lbn", 8) != 0) {
        return 2;
    }
    return 3;
}

// 000019a0
IsoDirectory *isofsReadIsoDir(IsofsUnit *unit, char *path, char *file, u32 flag, s32 *outRet) {
    *outRet = SCE_ERROR_OK;
    if (unit == NULL || path == NULL || file == NULL) {
        *outRet = SCE_ERROR_ERRNO_INVALID_ARGUMENT;
        return NULL;
    }

    // Count how many directory levels are in the path
    char *segPath = path;
    u32 segLen = 0;
    int levels = 1;
    while (1) {
        char *seg = strchr(segPath, '/');
        segPath = seg + 1;
        if (seg == NULL) {
            break;
        }
        levels = levels + 1;
        segLen = strlen(segPath);
    }

    // Construct new path to directory (without the last segment, i.e. file name)
    u32 pathLen = strlen(path);
    char pathStack[256];
    memset(pathStack, 0, 256);
    memcpy(pathStack, path, pathLen - segLen);
    pathStack[pathLen - segLen] = 0;

    // Lookup the directory LBN in path table
    s32 readLbn;
    if (strcmp(pathStack, "/") == 0) {
        readLbn = unit->rootDirLbn;
    } else {
        IsofsPath *foundPath = isofsFindPath(unit, pathStack, levels - 1, strlen(pathStack), flag, outRet);
        if (*outRet != 0) {
            if (*outRet != 0x20) {
                return NULL;
            }
            foundPath = unit->paths;
        }
        readLbn = foundPath->lbn;
    }

    // Check some UMD ids for compatible behaviour
    s32 idMatch = memcmp("ULUS-10013", unit->umdId, 10);
    if (idMatch == 0 || (idMatch = memcmp("ULJM-05006", unit->umdId, 10), idMatch == 0) ||
        (idMatch = memcmp("ULES-00123", unit->umdId, 10), idMatch == 0)) {
        g_isofsCurrentDirLbn = 0;
    }

    s32 openState;
    if (readLbn == g_isofsCurrentDirLbn) {
        // Required LBN is already read into the global
        s32 sdkVer = sceKernelGetCompiledSdkVersion();
        if (sdkVer < 0x3000000) {
            if (memcmp("ULJM-05127", unit, 10) != 0) {
                sceKernelDelayThread(1000);
            }
        }
        openState = unit->openState;
    } else {
        // Read the directory
        s32 ret = isofsReadDir(unit, readLbn, g_isofsCurrentDir);
        if (ret != 0) {
            *outRet = ret;
            return NULL;
        }
        openState = unit->openState;
        g_isofsCurrentDirLbn = readLbn;
    }

    u32 dirSize = ((IsoDirectory *) g_isofsCurrentDir)->size;
    if (openState == 2) {
        g_isofsCurrentDirLbn = 0;
        *outRet = SCE_UMD_ERROR_NOT_DEVICE_READY;
        return NULL;
    }
    IsoDirectory *foundDir;
    u32 noMoreDirSectors = ((IsoDirectory *) g_isofsCurrentDir)->size < 0x1000;
    u32 dirOffset = 0;
    do {
        readLbn = readLbn + 2;
        dirOffset = dirOffset + 0x1000;
        u32 findFlag;
        if (noMoreDirSectors == 0 || (s32) (dirSize - dirOffset) >= 0x1000) {
            findFlag = 2;
        } else {
            memset(g_isofsCurrentDir + 0x200, 0, 0x800);
            findFlag = 1;
        }
        foundDir = isofsFindIsoDirectory(path, (IsoDirectory *) g_isofsCurrentDir, findFlag, outRet);
        if (foundDir != NULL) {
            break;
        }
        idMatch = memcmp("ULUS-10013", unit->umdId, 10);
        if (idMatch == 0 ||
            (idMatch = memcmp("ULJM-05006", unit->umdId, 10), idMatch == 0) ||
            (idMatch = memcmp("ULES-00123", unit->umdId, 10), idMatch == 0)) {
            g_isofsCurrentDirLbn = 0;
        }
        if (readLbn == g_isofsCurrentDirLbn) {
            s32 sdkVer = sceKernelGetCompiledSdkVersion();
            if (sdkVer < 0x3000000) {
                if (memcmp("ULJM-05127", unit, 10) != 0) {
                    sceKernelDelayThread(1000);
                }
            }
            openState = unit->openState;
        } else {
            // Read the directory
            s32 ret = isofsReadDir(unit, readLbn, g_isofsCurrentDir);
            if (ret != 0) {
                *outRet = ret;
                g_isofsCurrentDirLbn = 0;
                return NULL;
            }
            openState = unit->openState;
            g_isofsCurrentDirLbn = readLbn;
        }
        if (openState == 2) {
            *outRet = SCE_UMD_ERROR_NOT_DEVICE_READY;
            g_isofsCurrentDirLbn = 0;
            return NULL;
        }
    } while (dirOffset < dirSize);

    *outRet = SCE_ERROR_OK;
    return foundDir;
}

// 00001dbc
s32 isofsParseSceLbnPath(char *path, s32 *outLbn, s32 *outSize) {
    if (path == NULL || outLbn == NULL || outSize == NULL) {
        return SCE_ERROR_ERRNO_INVALID_ARGUMENT;
    }
    if (strlen(path) > 0x1f) {
        return SCE_ERROR_ERRNO_INVALID_ARGUMENT;
    }
    if (strncmp(path, "/sce_lbn", 8) != 0) {
        return SCE_ERROR_ERRNO_FILE_NOT_FOUND;
    }
    *outLbn = strtol(path + 8, NULL, 0x10);
    char *size = strstr(path, "_size");
    if (size == NULL) {
        return SCE_ERROR_ERRNO_FILE_NOT_FOUND;
    }
    *outSize = strtol(size + 5, NULL, 0x10);
    return SCE_ERROR_OK;
}

// 00002274
int isofsReadDir(IsofsUnit *unit, u32 lbn, void *outdata) {
    if (unit == NULL) {
        return SCE_ERROR_ERRNO_INVALID_ARGUMENT;
    }
    s32 size = (s32) unit->sectorSize * 2;
    s32 ret = isofsHandlerReadDirSectors(unit, lbn, size, outdata, size, 6, 0);
    if (ret > 0) {
        return SCE_ERROR_OK;
    }
    return ret;
}

// 00001e9c
IsoDirectory *isofsSetDirentFromIsoDirectory(IsofsDir *dir, SceIoDirent *dirent, s32 *outRet) {
    if (dir == NULL) {
        *outRet = SCE_ERROR_ERRNO_INVALID_ARGUMENT;
        return NULL;
    }
    IsoDirectory *isoDir = dir->isoDir;
    if (isoDir == NULL || dirent == NULL) {
        *outRet = SCE_ERROR_ERRNO_INVALID_ARGUMENT;
        return NULL;
    }
    u32 recLen = isoDir->recLen;
    u32 dirRecLen = dir->dirRecLen + recLen;
    dir->dirRecLen = dirRecLen;
    if (recLen == 0) {
        if (dir->lbnSize > 1) {
            u32 dirLbn = (dirRecLen & 0xfffff800) + 0x800;
            dir->lbnSize = dir->lbnSize - 1;
            dir->dirRecLen = dirLbn;
            return (IsoDirectory *) ((u32) isoDir + (dirLbn - dirRecLen));
        }
        *outRet = SCE_ERROR_ERRNO_FILE_NOT_FOUND;
        return NULL;
    }
    if (isoDir->name[0] == 0) {
        dirent->d_name[0] = '.';
        dirent->d_name[1] = 0;
        dir->lbnSize = isoDir->size >> 0xb;
        dirent->d_stat.st_size = isoDir->size;
    }
    memcpy(dirent->d_name, isoDir->name, isoDir->nameLen);
    dirent->d_name[isoDir->nameLen] = 0;
    dirent->d_stat.st_private[0] = isoDir->lbn;
    dirent->d_stat.st_private[1] = 0;
    dirent->d_stat.st_private[2] = 0;
    dirent->d_stat.st_private[3] = 0;
    dirent->d_stat.st_private[4] = 0;
    dirent->d_stat.st_private[5] = 0;
    dirent->d_stat.st_size = isoDir->size;
    dirent->d_stat.st_attr = 0;
    u16 year = isoDir->dateTime.yearsSince1900 + 1900;
    dirent->d_stat.st_atime.year = year;
    dirent->d_stat.st_mtime.year = year;
    dirent->d_stat.st_ctime.year = year;
    u8 isoMonth = isoDir->dateTime.month;
    dirent->d_stat.st_atime.month = isoMonth;
    dirent->d_stat.st_mtime.month = isoMonth;
    dirent->d_stat.st_ctime.month = isoMonth;
    u8 isoDay = isoDir->dateTime.day;
    dirent->d_stat.st_atime.day = isoDay;
    dirent->d_stat.st_mtime.day = isoDay;
    dirent->d_stat.st_ctime.day = isoDay;
    u8 isoHour = isoDir->dateTime.hour;
    dirent->d_stat.st_mtime.hour = isoHour;
    dirent->d_stat.st_atime.hour = isoHour;
    dirent->d_stat.st_ctime.hour = isoHour;
    u8 isoMinute = isoDir->dateTime.minute;
    dirent->d_stat.st_atime.minute = isoMinute;
    dirent->d_stat.st_mtime.minute = isoMinute;
    dirent->d_stat.st_ctime.minute = isoMinute;
    u8 isoSecond = isoDir->dateTime.second;
    dirent->d_stat.st_atime.second = isoSecond;
    dirent->d_stat.st_mtime.second = isoSecond;
    dirent->d_stat.st_ctime.second = isoSecond;
    dirent->d_stat.st_mtime.microsecond = 0;
    dirent->d_stat.st_ctime.microsecond = 0;
    dirent->d_stat.st_atime.microsecond = 0;
    dirent->d_stat.st_mode = 0x16d;
    s32 sdkVer = sceKernelGetCompiledSdkVersion();
    if (sdkVer >= 0x2000011) {
        u32 dateTimeOffset = isoDir->dateTime.offset;
        if (dateTimeOffset != 0) {
            ScePspDateTime time;
            u64 tick;
            s32 ret = sceRtcGetTick((pspTime *) &dirent->d_stat.st_ctime, &tick);
            *outRet = ret;
            if (ret == 0) {
                if (dateTimeOffset - 0xd1 < 0x2f) {
                    u64 numMs = (s64) (s32) (0x100 - dateTimeOffset) * 900000000;
                    sceRtcTickAddTicks(&tick, &tick, numMs);
                } else if (dateTimeOffset - 1 < 0x34) {
                    u64 numMs = (s64) (s32) dateTimeOffset * 900000000;
                    sceRtcTickAddTicks(&tick, &tick, numMs);
                }
                ret = sceRtcSetTick((pspTime *) &time, &tick);
                *outRet = ret;
                if (ret == 0) {
                    dirent->d_stat.st_atime.year = time.year;
                    dirent->d_stat.st_atime.month = time.month;
                    dirent->d_stat.st_atime.day = time.day;
                    dirent->d_stat.st_atime.hour = time.hour;
                    dirent->d_stat.st_atime.minute = time.minute;
                    dirent->d_stat.st_atime.second = time.second;
                    dirent->d_stat.st_atime.microsecond = time.microsecond;
                    dirent->d_stat.st_mtime.year = time.year;
                    dirent->d_stat.st_ctime.year = time.year;
                    dirent->d_stat.st_mtime.month = time.month;
                    dirent->d_stat.st_ctime.month = time.month;
                    dirent->d_stat.st_mtime.day = time.day;
                    dirent->d_stat.st_ctime.day = time.day;
                    dirent->d_stat.st_mtime.hour = time.hour;
                    dirent->d_stat.st_ctime.hour = time.hour;
                    dirent->d_stat.st_mtime.minute = time.minute;
                    dirent->d_stat.st_ctime.minute = time.minute;
                    dirent->d_stat.st_mtime.second = time.second;
                    dirent->d_stat.st_ctime.second = time.second;
                    dirent->d_stat.st_mtime.microsecond = time.microsecond;
                    dirent->d_stat.st_ctime.microsecond = time.microsecond;
                }
            }
        }
    }
    if ((isoDir->flags & 2) == 0) {
        dirent->d_stat.st_mode = dirent->d_stat.st_mode | 0x2000;
    } else {
        dirent->d_stat.st_mode = dirent->d_stat.st_mode | 0x1000;
    }
    isoDir = dir->isoDir;
    *outRet = SCE_ERROR_OK;
    return (IsoDirectory *) (&isoDir->recLen + recLen);
}

// 000022c0
s32 isofsFreeUnitPaths(IsofsUnit *unit) {
    if (unit == NULL) {
        return SCE_ERROR_ERRNO_INVALID_ARGUMENT;
    }
    return isofsFreePaths(unit);
}

// 000022f0
void __attribute__ ((noinline)) isofsClearCurrentDirLbn() {
    g_isofsCurrentDirLbn = 0;
}

// 000022fc
char *isofsSplitPath(char *path, u32 *outLen, u32 flag, s32 *outRet) {
    if (path == NULL || outLen == NULL || outRet == NULL) {
        return NULL;
    }
    char *slash;
    if (flag - 2 < 4) {
        char *lastSlash = strrchr(path, '/');
        if (lastSlash == NULL || strchr(path + 1, '/') == NULL || (slash = strchr(path, '/'), slash == NULL)) {
            *outRet = SCE_ERROR_OK;
            return NULL;
        }
        path = slash + 1;
        char *firstSlash = strchr(path, '/');
        if (firstSlash == NULL) {
            *outLen = strlen(path);
        } else {
            *outLen = (int) firstSlash - (int) path;
        }
    } else {
        if (flag != 1) {
            *outRet = SCE_ERROR_ERRNO_INVALID_ARGUMENT;
            return NULL;
        }
        slash = strchr(path, '/');
        if (slash != NULL) {
            path = slash + 1;
            char *firstSlash = strchr(path, '/');
            if (firstSlash == NULL) {
                *outLen = strlen(path);
            } else {
                *outLen = (int) firstSlash - (int) path;
            }
            return path;
        }
        *outRet = SCE_ERROR_OK;
        *outLen = strlen(path);
    }
    return path;
}

// 0000241c
s32 isofsUpdatePathsLbnSize(IsofsUnit *unit) {
    if (unit == NULL) {
        return SCE_ERROR_ERRNO_INVALID_ARGUMENT;
    }
    u32 rootDirSize = unit->rootDirSize;
    if (rootDirSize < 0x800) {
        unit->paths->lbnSize = 1;
    } else if ((rootDirSize & 0x7ff) == 0) {
        unit->paths->lbnSize = rootDirSize >> 0xb;
    } else {
        unit->paths->lbnSize = (rootDirSize >> 0xb) + 1;
    }
    return SCE_ERROR_OK;
}

// 00002474
int isofsDrvInit(SceIoDeviceEntry *entry) {
    if (entry == NULL) {
        sceKernelSignalSema(g_isofsMgr.semaId, 1);
        return SCE_ERROR_ERRNO_INVALID_ARGUMENT;
    }
    SceUID heapId = sceKernelCreateHeap(1, 0xa000, 1, "SceIsofsRootHeap");
    if (heapId < 0) {
        return heapId;
    }
    g_isofsMgr.heapId = heapId;
    s32 mgrSemaId = sceKernelCreateSema("SceIsofsMgr", 1, 1, 1, NULL);
    if (mgrSemaId > -1) {
        g_isofsMgr.semaId = mgrSemaId;
        s32 ret = isofsInitUnits();
        if (ret == 0 && (ret = sceKernelWaitSema(g_isofsMgr.semaId, 1, NULL), -1 < ret)) {
            for (s32 i = 0; i < g_isofsMgr.maxUnits; ++i) {
                IsofsUnit *unit = isofsGetUnit(i);
                s32 semaId = sceKernelCreateSema("SceIsofs", 1, 1, 1, NULL);
                if (semaId > -1) {
                    unit->semaId = semaId;
                    if (i >= g_isofsMgr.maxUnits) {
                        break;
                    }
                    continue;
                }

                // delete already created
                for (s32 ii = 0; ii < g_isofsMgr.maxUnits; ++ii) {
                    IsofsUnit *unit2 = isofsGetUnit(ii);
                    if (unit2->semaId > -1) {
                        sceKernelDeleteSema(unit2->semaId);
                    }
                }
                sceKernelSignalSema(g_isofsMgr.semaId, 1);
                sceKernelDeleteHeap(g_isofsMgr.heapId);
                sceKernelDeleteSema(g_isofsMgr.semaId);
                return SCE_ERROR_ERRNO_DEVICE_NOT_FOUND;
            }

            IsofsUnit *unit = isofsGetUnit(0);
            entry->d_private = unit;
            if (isofsInitCache() == 0) {
                g_isofsInitialized = 1;
                sceKernelSignalSema(g_isofsMgr.semaId, 1);
                return SCE_ERROR_OK;
            } else {
                sceKernelDeleteSema(g_isofsMgr.semaId);
                sceKernelDeleteHeap(g_isofsMgr.heapId);
                sceKernelSignalSema(g_isofsMgr.semaId, 1);
                return SCE_ERROR_ERRNO_DEVICE_NOT_FOUND;
            }
        }
        sceKernelDeleteSema(g_isofsMgr.semaId);
    }
    sceKernelDeleteHeap(g_isofsMgr.heapId);
    return SCE_ERROR_ERRNO_DEVICE_NOT_FOUND;
}

// 00002680
int isofsDrvMount(SceIoIob *iob, const char *fs, const char *blockDev, int mode, void *outdata, int outlen) {
    if (pspGetK1() <= -1) {
        return SCE_ERROR_PRIV_REQUIRED;
    }
    if (iob == NULL || blockDev == NULL || fs == 0) {
        return SCE_ERROR_ERRNO_INVALID_ARGUMENT;
    }
    if (iob->i_unit < 0 || iob->i_unit >= g_isofsMgr.maxUnits) {
        return SCE_ERROR_ERRNO_INVALID_UNIT_NUM;
    }
    s32 ret = sceKernelWaitSema(g_isofsMgr.semaId, 1, NULL);
    if (ret < 0) {
        return ret;
    }
    s32 outdataRead = 2;
    if (outdata != NULL && outlen > 3) {
        outdataRead = *(s32 *) outdata;
    }
    IsofsUnit *unit = isofsGetUnit(iob->i_unit);
    ret = isofsCheckUnitMounted(unit, iob->i_unit);
    if (ret == 0 && (ret = isofsCheckMode(mode), ret == 0)) {
        u32 blockDevLen = strlen(blockDev);
        u32 devctlRet = 0;
        ret = sceIoDevctl(blockDev, DEVCTL_UNKNOWN_1E38034, (char *) blockDev, blockDevLen, &devctlRet,
                          sizeof(devctlRet));
        if (ret != 0) {
            goto exit;
        }
        unit->unitNum = *(u16 *) &iob->i_unit;
        unit->mountFlags = unit->mountFlags | 0x10;
        unit->unk44 = 0;
        if (blockDevLen >= 0x20) {
            ret = SCE_ERROR_ERRNO_NAME_TOO_LONG;
            goto exit;
        }
        memcpy(unit->blockDev, blockDev, blockDevLen);
        unit->blockDev[blockDevLen] = 0;
        ret = isofsSetBlockDevHandler(unit);
        if (ret < 0) {
            goto exit;
        }
        if (outdataRead == 2) {
            // bug? signal sema not called
            return SCE_ERROR_NOT_SUPPORTED;
        }
        unit->mountState = 1;
        ret = isofsRegisterOnUmdMounted(unit);
        if (ret == 0) {
            return sceKernelSignalSema(g_isofsMgr.semaId, 1);
        }
        isofsClearUnitFilesFdw(unit, g_isofsMgr.maxFiles);
        isofsClearUnit(unit->unitNum);
    }
exit:
    sceKernelSignalSema(g_isofsMgr.semaId, 1);
    return sceUmd_040A7090(ret);
}

// 000028a4
s32 isofsClearUnit(s32 unitNum) {
    if (unitNum < 0) {
        return SCE_ERROR_ERRNO_INVALID_ARGUMENT;
    }
    for (s32 i = 0; i < g_isofsMgr.maxUnits; ++i) {
        IsofsUnit *unit = isofsGetUnit(i);
        if (unit->unitNum == unitNum && (unit->mountFlags & 0x10) != 0) {
            s32 ret = isofsFreeUnitPaths(unit);
            if (ret != 0) {
                return ret;
            }
            unit->pathTableSize = 0;
            unit->rootDirLbn = 0;
            unit->rootDirSize = 0;
            unit->unkNextLbn = 0;
            unit->lastLbn = 0;
            unit->unkLbnTillEnd = 0;
            unit->pathTableLbn = 0;
            unit->totalSectors = 0;
            unit->unkCurrentLbn = 0;
            unit->openState = 0;
            unit->sectorSize = 0;
            _setUmdId(unit->umdId, 0);
            unit->unitNum = -1;
            unit->mountFlags = 0;
            unit->pathTableEntryCount = 0;
            unit->paths = NULL;
            unit->pathTable = NULL;
            unit->primaryVolumeDescriptor = NULL;
            unit->unk44 = 0;
            unit->mountState = 0;
            g_isofsMounted2 = 0;
            return SCE_ERROR_OK;
        }
    }
    return SCE_ERROR_ERRNO_DEVICE_BUSY;
}

// 000029c8
int isofsDrvDopen(SceIoIob *iob, char *path) {
    if (iob == NULL || path == NULL) {
        return SCE_ERROR_ERRNO_INVALID_ARGUMENT;
    }
    u32 pathLen = strlen(path);
    if (pathLen == 0) {
        return SCE_ERROR_ERRNO_INVALID_ARGUMENT;
    }
    if (pathLen + 1 > 0x100) {
        return SCE_ERROR_ERRNO_NAME_TOO_LONG;
    }
    if (g_isofsMgr.maxUnits <= iob->i_unit) {
        return SCE_ERROR_ERRNO_INVALID_UNIT_NUM;
    }
    IsofsUnit *unit = isofsGetUnit(iob->i_unit);
    if (unit == NULL || g_isofsMounted == 0) {
        return SCE_ERROR_ERRNO_DEVICE_NOT_FOUND;
    }
    s32 ret = sceKernelWaitSema(unit->semaId, 1, NULL);
    if (ret < 0) {
        return ret;
    }
    ret = isofsCheckPath(path, 1);
    if (ret == 0) {
        IsofsDir *dir = sceKernelAllocHeapMemory(g_isofsMgr.heapId, sizeof(IsofsDir));
        if (dir == NULL) {
            ret = SCE_ERROR_ERRNO_NO_MEMORY;
        } else {
            dir->lbnSize = 0;
            dir->isoDir = NULL;
            s32 intr = sceKernelCpuSuspendIntr();
            __builtin_memcpy(dir->umdId, unit->umdId, UMD_ID_SIZE);
            dir->compatFlag = 0;
            dir->prevDir = NULL;
            if (g_isofsOpenDirs == NULL) {
                g_isofsOpenDirs = dir;
                dir->nextDir = NULL;
            } else {
                dir->nextDir = g_isofsOpenDirs;
                g_isofsOpenDirs->prevDir = dir;
            }
            sceKernelCpuResumeIntr(intr);
            if (strcmp(path, "/") != 0) {
                char pathStack[257];
                memset(pathStack, 0, 0x100);
                if (pathLen < 0x100) {
                    s32 dirLevel = 1;
                    if (strcmp(path + (pathLen - 1), "/") == 0) {
                        memcpy(pathStack, path, pathLen - 1);
                        pathStack[pathLen] = 0;
                    } else {
                        memcpy(pathStack, path, pathLen);
                        pathStack[pathLen + 1] = 0;
                    }
                    char *pathSeg = pathStack + 1;
                    do {
                        char *pathSep = strchr(pathSeg, '/');
                        if (pathSep == NULL) {
                            break;
                        }
                        dirLevel = dirLevel + 1;
                        pathSeg = pathSep + 1;
                    } while (1);
                    IsofsPath *paths = isofsFindPath(unit, pathStack, dirLevel, strlen(pathStack + 1), 1, &ret);
                    if (paths != NULL) {
                        dir->paths = paths;
                        dir->lbnSize = paths->lbnSize;
                        dir->lbn = paths->lbn;
                        goto found;
                    }
                } else {
                    ret = SCE_ERROR_ERRNO_NAME_TOO_LONG;
                }
                intr = sceKernelCpuSuspendIntr();
                isofsUnlinkOpenedDir(dir);
                sceKernelCpuResumeIntr(intr);
                sceKernelFreeHeapMemory(g_isofsMgr.heapId, dir);
                sceKernelSignalSema(unit->semaId, 1);
                return sceUmd_040A7090(ret);
            }
            dir->paths = unit->paths;
            u32 lbnSize = 1;
            if (0x7ff < unit->rootDirSize && (lbnSize = unit->rootDirSize >> 0xb, (unit->rootDirSize & 0x7ff) != 0)) {
                lbnSize = (unit->rootDirSize >> 0xb) + 1;
            }
            dir->lbnSize = lbnSize;
            dir->lbn = unit->rootDirLbn;
        found:
            dir->dirRecLen = 0;
            dir->fplData = NULL;
            dir->isoDir = NULL;
            if (dir->compatFlag == 0) {
                iob->i_private = dir;
            } else {
                intr = sceKernelCpuSuspendIntr();
                isofsUnlinkOpenedDir(dir);
                sceKernelCpuResumeIntr(intr);
                sceKernelFreeHeapMemory(g_isofsMgr.heapId, dir);
                ret = SCE_UMD_ERROR_NOT_DEVICE_READY;
            }
        }
    }
    sceKernelSignalSema(unit->semaId, 1);
    return sceUmd_040A7090(ret);
}

// 	00002d4c
int isofsDrvDread(SceIoIob *iob, SceIoDirent *dirent) {
    if (iob == NULL || iob->i_private == NULL) {
        return SCE_ERROR_ERRNO_INVALID_ARGUMENT;
    }
    IsofsUnit *unit = isofsGetUnit(iob->i_unit);
    if (unit == NULL || dirent == NULL) {
        return SCE_ERROR_ERRNO_INVALID_ARGUMENT;
    }
    s32 ret = sceKernelWaitSema(unit->semaId, 1, NULL);
    if (ret < 0) {
        return ret;
    }
    IsoDirectory *data = NULL;
    IsofsDir *dir = iob->i_private;
    if (dir->compatFlag == 0) {
        memset(dirent, 0, 0x160);
        if (dir->dirRecLen != 0) {
            u32 lba;
            u32 dirLbn;
        doLoop:
            do {
                IsoDirectory *stat = dir->isoDir;
            doNext:
                if (stat->recLen != 0) {
                    if (stat->nameLen == 1 && (stat[1].recLen == 0 || stat[1].recLen == 1)) {
                        stat = isofsSetDirentFromIsoDirectory(dir, dirent, &ret);
                        if (stat != NULL) {
                            dir->isoDir = stat;
                            ret = 1;
                            goto doLoop;
                        }
                        dir->isoDir = NULL;
                        ret = 0;
                        goto exit;
                    }
                    if (stat->recLen == 0) {
                        lba = dir->lbnSize;
                        goto recLenZero;
                    }
                setFromIso:
                    stat = isofsSetDirentFromIsoDirectory(dir, dirent, &ret);
                    if (stat == NULL) {
                        if (ret == (s32) SCE_ERROR_ERRNO_FILE_NOT_FOUND) {
                            ret = 0;
                        }
                        dir->isoDir = NULL;
                        goto exit;
                    }
                    dir->isoDir = stat;
                    ret = 1;
                    if (dir->lbnSize == 0) {
                        goto exit;
                    }
                    if (dir->dirRecLen == 0) {
                        goto exit;
                    }
                    if ((dir->dirRecLen & 0xfff) != 0) {
                        goto exit;
                    }
                    dir->lbn = dir->lbn + 2;
                    memset(dir->fplData, 0, 0x1000);
                    ret = isofsReadDir(unit, dir->lbn, dir->fplData);
                    if (ret != 0) {
                        dir->lbn = dir->lbn - 2;
                        goto exit;
                    }
                    if (dir->compatFlag != 0) {
                        dirLbn = dir->lbn;
                        ret = SCE_ERROR_ERRNO_INVALID_FILE_DESCRIPTOR;
                        dir->lbn = dirLbn - 2;
                        goto exit;
                    }
                    ret = 1;
                    dir->isoDir = (IsoDirectory *) dir->fplData;
                    goto exit;
                }
                lba = dir->lbnSize;
            recLenZero:
                if (lba < 2) {
                    goto setFromIso;
                }
                stat = isofsSetDirentFromIsoDirectory(dir, dirent, &ret);
                if (stat == NULL) {
                    dir->isoDir = NULL;
                    ret = 0;
                    goto exit;
                }
                dir->isoDir = stat;
                ret = 1;
                if (dir->lbnSize == 0) {
                    goto doLoop;
                }
                if (dir->dirRecLen == 0) {
                    stat = dir->isoDir;
                    goto doNext;
                }
                if ((dir->dirRecLen & 0xfff) != 0) {
                    stat = dir->isoDir;
                    goto doNext;
                }
                dir->lbn = dir->lbn + 2;
                memset(dir->fplData, 0, 0x1000);
                ret = isofsReadDir(unit, dir->lbn, dir->fplData);
                if (ret != 0) {
                    dir->lbn = dir->lbn - 2;
                    goto exit;
                }
                if (dir->compatFlag != 0) {
                    dirLbn = dir->lbn;
                    ret = SCE_ERROR_ERRNO_INVALID_FILE_DESCRIPTOR;
                    dir->lbn = dirLbn - 2;
                    goto exit;
                }
                ret = 1;
                dir->isoDir = (IsoDirectory *) dir->fplData;
            } while (1);
        }
        ret = sceKernelTryAllocateFpl(g_isofsMgr.fplId, (void **) &data);
        if (ret < 0) {
            goto exit;
        }
        dir->fplData = data;
        if (dir->dirRecLen != 0) {
            goto doLoop;
        }
        ret = isofsReadDir(unit, dir->lbn, data);
        if (ret != 0) {
            goto exit;
        }
        if (dir->compatFlag == 0) {
            dir->isoDir = data;
            goto doLoop;
        }
    }
    ret = SCE_ERROR_ERRNO_INVALID_FILE_DESCRIPTOR;
exit:
    sceKernelSignalSema(unit->semaId, 1);
    return sceUmd_040A7090(ret);
}

// 00003084
int isofsDrvClose(SceIoIob *iob) {
    if (iob == NULL) {
        return SCE_ERROR_ERRNO_INVALID_ARGUMENT;
    }
    IsofsUnit *unit = isofsGetUnit(iob->i_unit);
    IsofsFile *file = iob->i_private;
    if (unit == NULL || file == NULL) {
        return SCE_ERROR_ERRNO_INVALID_ARGUMENT;
    }
    s32 ret = sceKernelWaitSema(unit->semaId, 1, NULL);
    if (ret < 0) {
        return ret;
    }
    ret = isofsClearFile(file);
    if (ret == 0) {
        iob->i_private = NULL;
    }
    sceKernelSignalSema(unit->semaId, 1);
    return ret;
}

// 00003134
int isofsDrvDclose(SceIoIob *iob) {
    if (iob == NULL) {
        return SCE_ERROR_ERRNO_INVALID_ARGUMENT;
    }
    IsofsUnit *unit = isofsGetUnit(iob->i_unit);
    s32 ret = sceKernelWaitSema(unit->semaId, 1, NULL);
    if (ret < 0) {
        return ret;
    }
    IsofsDir *dir = iob->i_private;
    s32 intr = sceKernelCpuSuspendIntr();
    isofsUnlinkOpenedDir(dir);
    sceKernelCpuResumeIntr(intr);
    sceKernelFreeFpl(g_isofsMgr.fplId, dir->fplData);
    sceKernelFreeHeapMemory(g_isofsMgr.heapId, dir);
    iob->i_private = NULL;
    sceKernelSignalSema(unit->semaId, 1);
    return ret;
}

// 000031f4
int isofsDrvUmount(SceIoIob *iob, __attribute__((unused)) char *blockDev) {
    if (iob == NULL) {
        return SCE_ERROR_ERRNO_INVALID_ARGUMENT;
    }
    IsofsUnit *unit = isofsGetUnit(iob->i_unit);
    s32 ret = sceKernelWaitSema(g_isofsMgr.semaId, 1, NULL);
    if (ret < 0) {
        return ret;
    }
    s32 intr = sceKernelCpuSuspendIntr();
    ret = isofsUmdManUnRegisterInsertEjectUMDCallBack();
    if (ret != 0) {
        goto exit;
    }
    isofsClearBlockDevHandler(unit);
    ret = isofsClearUnitFilesFdw(unit, g_isofsMgr.maxFiles);
    if (ret != 0) {
        goto exit;
    }
    ret = isofsClearUnit(unit->unitNum);
    g_isofsMounted = 0;
    sceIoTerminateFd("isofs");
exit:
    sceKernelCpuResumeIntr(intr);
    sceKernelSignalSema(g_isofsMgr.semaId, 1);
    return ret;
}

// 000032d8
int isofsDrvExit(SceIoDeviceEntry *entry) {
    if (entry == NULL) {
        return SCE_ERROR_ERRNO_INVALID_ARGUMENT;
    }
    s32 ret = isofsFreeUnits();
    if (ret != 0) {
        return ret;
    }
    ret = isofsFreeCache();
    if (ret != 0) {
        return ret;
    }
    sceKernelDeleteHeap(g_isofsMgr.heapId);
    sceKernelDeleteSema(g_isofsMgr.semaId);
    g_isofsInitialized = 0;
    return SCE_ERROR_OK;
}

// 00003348
int isofsDrvGetstat(SceIoIob *iob, char *name, SceIoStat *stat) {
    if (iob == NULL || name == NULL) {
        return SCE_ERROR_ERRNO_INVALID_ARGUMENT;
    }
    if (strlen(name) + 1 > 0x100 || g_isofsMgr.maxUnits <= iob->i_unit) {
        return SCE_ERROR_ERRNO_INVALID_ARGUMENT;
    }
    IsofsUnit *unit = isofsGetUnit(iob->i_unit);
    s32 ret = sceKernelWaitSema(unit->semaId, 1, NULL);
    if (ret < 0) {
        return ret;
    }
    ret = isofsCheckPath(name, 2);
    s32 sceLbnRet = 0;
    if (ret != 0 || (sceLbnRet = isofsIsSceLbnPath(name), sceLbnRet != 2)) {
        sceKernelSignalSema(unit->semaId, 1);
        return sceUmd_040A7090(ret);
    }
    char *lastSlash = strrchr(name, '/');
    if (lastSlash == NULL) {
        ret = SCE_ERROR_ERRNO_INVALID_ARGUMENT;
        sceKernelSignalSema(unit->semaId, 1);
        return sceUmd_040A7090(ret);
    }
    IsoDirectory *isoDir = isofsReadIsoDir(unit, name, lastSlash + 1, 2, &ret);
    if (isoDir == NULL) {
        ret = SCE_ERROR_ERRNO_FILE_NOT_FOUND;
        sceKernelSignalSema(unit->semaId, 1);
        return sceUmd_040A7090(ret);
    }
    stat->st_private[0] = isoDir->lbn;
    stat->st_private[1] = 0;
    stat->st_private[2] = 0;
    stat->st_private[3] = 0;
    stat->st_private[4] = 0;
    stat->st_private[5] = 0;
    stat->st_mode = 0x16d;
    stat->st_size = isoDir->size;
    u16 year = isoDir->dateTime.yearsSince1900 + 1900;
    stat->st_ctime.year = year;
    stat->st_mtime.year = year;
    stat->st_atime.year = year;
    stat->st_ctime.month = (u16) isoDir->dateTime.month;
    stat->st_mtime.month = (u16) isoDir->dateTime.month;
    stat->st_atime.month = (u16) isoDir->dateTime.month;
    stat->st_ctime.day = (u16) isoDir->dateTime.day;
    stat->st_mtime.day = (u16) isoDir->dateTime.day;
    stat->st_atime.day = (u16) isoDir->dateTime.day;
    stat->st_ctime.hour = (u16) isoDir->dateTime.hour;
    stat->st_mtime.hour = (u16) isoDir->dateTime.hour;
    stat->st_atime.hour = (u16) isoDir->dateTime.hour;
    stat->st_ctime.minute = (u16) isoDir->dateTime.minute;
    stat->st_mtime.minute = (u16) isoDir->dateTime.minute;
    stat->st_atime.minute = (u16) isoDir->dateTime.minute;
    stat->st_ctime.second = (u16) isoDir->dateTime.second;
    stat->st_mtime.second = (u16) isoDir->dateTime.second;
    stat->st_atime.second = (u16) isoDir->dateTime.second;
    stat->st_ctime.microsecond = 0;
    stat->st_mtime.microsecond = 0;
    stat->st_atime.microsecond = 0;
    s32 sdkVer = sceKernelGetCompiledSdkVersion();
    if (sdkVer >= 0x2000011) {
        u32 dateTimeOffset = isoDir->dateTime.offset;
        u64 tick = 0;
        if (dateTimeOffset != 0 && (ret = sceRtcGetTick((pspTime *) &stat->st_ctime, &tick), ret == 0)) {
            if (dateTimeOffset - 0xd1 < 0x2f) {
                u64 numMs = (s64) (s32) (0x100 - dateTimeOffset) * 900000000;
                sceRtcTickAddTicks(&tick, &tick, numMs);
            } else if (dateTimeOffset - 1 < 0x34) {
                u64 numMs = (s64) (s32) dateTimeOffset * 900000000;
                sceRtcTickAddTicks(&tick, &tick, numMs);
            }
            ScePspDateTime time;
            ret = sceRtcSetTick((pspTime *) &time, &tick);
            if (ret == 0) {
                stat->st_atime.year = time.year;
                stat->st_atime.month = time.month;
                stat->st_atime.day = time.day;
                stat->st_atime.hour = time.hour;
                stat->st_atime.minute = time.minute;
                stat->st_atime.second = time.second;
                stat->st_atime.microsecond = time.microsecond;
                stat->st_mtime.year = time.year;
                stat->st_ctime.year = time.year;
                stat->st_mtime.month = time.month;
                stat->st_ctime.month = time.month;
                stat->st_mtime.day = time.day;
                stat->st_ctime.day = time.day;
                stat->st_mtime.hour = time.hour;
                stat->st_ctime.hour = time.hour;
                stat->st_mtime.minute = time.minute;
                stat->st_ctime.minute = time.minute;
                stat->st_mtime.second = time.second;
                stat->st_ctime.second = time.second;
                stat->st_mtime.microsecond = time.microsecond;
                stat->st_ctime.microsecond = time.microsecond;
            }
        }
    }
    if ((isoDir->flags & 2) == 0) {
        stat->st_mode = stat->st_mode | 0x2000;
    } else {
        stat->st_mode = stat->st_mode | 0x1000;
    }
    sceKernelSignalSema(unit->semaId, 1);
    return sceUmd_040A7090(ret);
}

// 000036e0
s32 isofsCheckUnitMounted(IsofsUnit *unit, s32 unitNum) {
    if (unit->primaryVolumeDescriptor != NULL || unit->unitNum == unitNum || unit->unk44 != 0
        || (unit->mountFlags & 0x10) != 0) {
        return SCE_ERROR_ERRNO_DEVICE_BUSY;
    }
    return SCE_ERROR_OK;
}

// 00003720
s32 __attribute__ ((noinline)) isofsCheckMode(SceMode mode) {
    if ((mode & 0xf02U) != 0) {
        return SCE_ERROR_ERRNO_INVALID_FLAG;
    }
    return SCE_ERROR_OK;
}

// 00003734
int isofsDrvOpen(SceIoIob *iob, char *path, int flags, __attribute__((unused)) SceMode mode) {
    if (iob == NULL || path == NULL) {
        return SCE_ERROR_ERRNO_INVALID_ARGUMENT;
    }
    u32 pathLen = strlen(path);
    if ((int) (pathLen + 1) > 0x100) {
        return SCE_ERROR_ERRNO_NAME_TOO_LONG;
    }
    if (g_isofsMgr.maxUnits <= iob->i_unit) {
        return SCE_ERROR_ERRNO_INVALID_ARGUMENT;
    }
    IsofsUnit *unit = isofsGetUnit(iob->i_unit);
    if (unit == NULL || g_isofsMounted == 0) {
        return SCE_ERROR_ERRNO_DEVICE_NOT_FOUND;
    }
    s32 ret = sceKernelWaitSema(unit->semaId, 1, NULL);
    if (ret < 0) {
        return ret;
    }
    ret = isofsCheckMode(flags); // bug? should probably pass mode instead
    if (ret == 0 && (ret = isofsCheckPath(path, 2), ret == 0)) {
        IsofsFile *file;
        unit->openState = 1;
        s32 openMode = isofsIsSceLbnPath(path);
        if (openMode == 2) {
            char *pathSep = strrchr(path, '/');
            if (pathSep == NULL) {
                ret = SCE_ERROR_ERRNO_INVALID_ARGUMENT;
            } else {
                pathLen = strlen(pathSep + 1);
                if (pathLen < 0x20) {
                    IsoDirectory *isoDir = isofsReadIsoDir(unit, path, pathSep + 1, 2, &ret);
                    if (isoDir == NULL || ret != 0) {
                        if (ret == (s32) SCE_UMD_ERROR_NOT_DEVICE_READY || ret == (s32) SCE_UMD_ERROR_NO_MEDIUM) {
                            unit->openState = 0;
                            sceKernelSignalSema(unit->semaId, 1);
                            return sceUmd_040A7090(ret);
                        }
                        ret = SCE_ERROR_ERRNO_FILE_NOT_FOUND;
                    } else {
                        file = isofsOpen(unit, iob, isoDir, &ret);
                        if (file != NULL) {
                            if ((flags & 0x4000) != 0) {
                                file->readFlags = file->readFlags | 8;
                            }
                            iob->i_private = file;
                            iob->i_fpos = 0;
                            unit->openState = 0;
                            sceKernelSignalSema(unit->semaId, 1);
                            return sceUmd_040A7090(ret);
                        }
                    }
                } else {
                    ret = SCE_ERROR_ERRNO_NAME_TOO_LONG;
                }
            }
        } else if (openMode == 3) {
            s32 lbn = 0;
            s32 size = 0;
            pathLen = strlen(path);
            if (pathLen > 0xff) {
                ret = SCE_ERROR_ERRNO_NAME_TOO_LONG;
                unit->openState = 0;
                sceKernelSignalSema(unit->semaId, 1);
                return sceUmd_040A7090(ret);
            }
            ret = isofsParseSceLbnPath(path, &lbn, &size);
            if (ret == 0) {
                u32 currentLbn = unit->currentLbn;
                if (currentLbn < (u32) lbn) {
                    ret = SCE_ERROR_ERRNO_FILE_NOT_FOUND;
                    unit->openState = 0;
                    sceKernelSignalSema(unit->semaId, 1);
                    return sceUmd_040A7090(ret);
                }
                u32 lbnSize = 1;
                if (size >= 0x800) {
                    lbnSize = (size >> 0xb) + 1;
                    if ((size & 0x7ff) == 0) {
                        lbnSize = size >> 0xb;
                    }
                }
                if (currentLbn < lbn + lbnSize - 1) {
                    size = (currentLbn - lbn + 1) * 0x800;
                }
                file = isofsOpenRaw(unit, iob, lbn, size, &ret);
                if (file != NULL) {
                    if ((flags & 0x4000) != 0) {
                        file->readFlags = file->readFlags | 8;
                    }
                    iob->i_private = file;
                    iob->i_fpos = 0;
                    unit->openState = 0;
                    sceKernelSignalSema(unit->semaId, 1);
                    return sceUmd_040A7090(ret);
                }
            }
        } else {
            ret = SCE_ERROR_ERRNO_FILE_NOT_FOUND;
        }
    }

    unit->openState = 0;
    sceKernelSignalSema(unit->semaId, 1);
    return sceUmd_040A7090(ret);
}

// 00003a1c
s32 isofsDrvRead(SceIoIob *iob, void *outdata, SceSize size) {
    if (iob == NULL || outdata == 0) {
        return SCE_ERROR_ERRNO_INVALID_ARGUMENT;
    }
    IsofsUnit *unit = isofsGetUnit(iob->i_unit);
    IsofsFile *file = iob->i_private;
    if (unit == NULL || file == NULL) {
        return SCE_ERROR_ERRNO_INVALID_ARGUMENT;
    }

    s32 ret = sceKernelWaitSema(unit->semaId, 1, NULL);
    if (ret < 0) {
        return ret;
    }
    if ((file->flags & 8) != 0) {
        ret = SCE_ERROR_ERRNO_INVALID_FILE_DESCRIPTOR;
        goto exit;
    }
    IsofsReadSectorsArgs readArgs;
    readArgs.flags = file->readFlags;
    readArgs.unk14 = 0;
    readArgs.unit = unit;
    readArgs.file = file;
    readArgs.outsize = size;
    readArgs.outdata = outdata;
    ret = sceKernelExtendKernelStack(0x1800, isofsHandlerReadSectorsArgs, &readArgs);
    if ((file->flags & 8) != 0) {
        ret = SCE_ERROR_ERRNO_INVALID_FILE_DESCRIPTOR;
        goto exit;
    }
    if (ret <= -1) {
        goto exit;
    }
    file->fpos += ret;
    iob->i_fpos += ret;
    u32 fposHi = *(u32 *) ((u32) &file->fpos + 4);
    u32 sizeHi = *(u32 *) ((u32) &file->size + 4);
    if (fposHi <= sizeHi) {
        if (fposHi != sizeHi) {
            goto exit;
        }
        if (*(u32 *) &file->fpos <= *(u32 *) &file->size) {
            goto exit;
        }
    }
    file->fpos = file->size;
    iob->i_fpos = file->size;
exit:
    sceKernelSignalSema(unit->semaId, 1);
    return sceUmd_040A7090(ret);
}

// 00003bd4
SceOff isofsDrvLseek(SceIoIob *iob, SceOff ofs, int whence) {
    if (iob == NULL) {
        return (s32) SCE_ERROR_ERRNO_INVALID_ARGUMENT;
    }
    IsofsUnit *unit = isofsGetUnit(iob->i_unit);
    IsofsFile *file = iob->i_private;
    if (unit == NULL || file == NULL) {
        return (s32) SCE_ERROR_ERRNO_INVALID_ARGUMENT;
    }
    s32 ret = sceKernelWaitSema(unit->semaId, 1, NULL);
    if (ret >> 0x1f < 0) {
        return ret;
    }
    if ((file->flags & 8) != 0) {
        sceKernelSignalSema(unit->semaId, 1);
        return (s32) SCE_ERROR_ERRNO_INVALID_FILE_DESCRIPTOR;
    }
    if ((iob->i_flgs & 8U) != 0) {
        sceKernelSignalSema(unit->semaId, 1);
        return (s32) SCE_ERROR_ERRNO_IS_DIRECTORY;
    }
    SceOff seekRet = isofsLseek(file, ofs, whence);
    if (seekRet > -1) {
        iob->i_fpos = seekRet;
    }
    sceKernelSignalSema(unit->semaId, 1);
    return seekRet;
}

// 00003d0c
int isofsDrvIoctl(SceIoIob *iob, int cmd, void *indata, SceSize inlen, void *outdata, SceSize outlen) {
    if (iob == NULL || 0 >= cmd) {
        return SCE_ERROR_ERRNO_INVALID_ARGUMENT;
    }
    IsofsUnit *unit = isofsGetUnit(iob->i_unit);
    if (unit == NULL || g_isofsMounted == 0) {
        return SCE_ERROR_ERRNO_DEVICE_NOT_FOUND;
    }
    IsofsFile *file = iob->i_private;
    if (file == NULL) {
        return SCE_ERROR_ERRNO_INVALID_ARGUMENT;
    }
    s32 ret = sceKernelWaitSema(unit->semaId, 1, NULL);
    if (ret < 0) {
        return ret;
    }
    if ((file->flags & 8) != 0) {
        ret = SCE_ERROR_ERRNO_INVALID_FILE_DESCRIPTOR;
        goto exit;
    }
    ret = isofsIoctl(iob, cmd, indata, inlen, outdata, outlen);
    if ((file->flags & 8) != 0) {
        ret = SCE_ERROR_ERRNO_INVALID_FILE_DESCRIPTOR;
    }
exit:
    sceKernelSignalSema(unit->semaId, 1);
    return sceUmd_040A7090(ret);
}

// 00003e4c
int isofsDrvDevctl(SceIoIob *iob, char *dev, int cmd, void *indata, SceSize inlen, void *outdata, SceSize outlen) {
    if (iob == NULL || dev == NULL || cmd <= 1) {
        return SCE_ERROR_ERRNO_INVALID_ARGUMENT;
    }
    IsofsUnit *unit = isofsGetUnit(iob->i_unit);
    if (unit == NULL || g_isofsMounted == 0) {
        return SCE_ERROR_ERRNO_DEVICE_NOT_FOUND;
    }
    u32 devLen = strlen(dev);
    if (devLen >= 0x20) {
        return SCE_ERROR_ERRNO_INVALID_ARGUMENT;
    }
    s32 ret = sceKernelWaitSema(unit->semaId, 1, NULL);
    if (ret < 0) {
        return ret;
    }
    ret = 0;
    if (cmd > 0x208811) {
        ret = sceIoDevctl(dev, cmd, indata, inlen, outdata, outlen);
    }
    sceKernelSignalSema(unit->semaId, 1);
    return sceUmd_040A7090(ret);
}

// 00003f8c
s32 isofsUmdMountedCb(int id, IsofsUnit *unit, int unk) {
    s32 ret = SCE_ERROR_ERRNO_INVALID_ARGUMENT;
    if (id == 0x20 && (ret = SCE_ERROR_ERRNO_INVALID_ARGUMENT, unit != NULL) &&
        (ret = sceKernelWaitSema(g_isofsMgr.semaId, 1, NULL), -1 < ret)) {
        if (unit->unitNum < 0) {
            sceKernelSignalSema(g_isofsMgr.semaId, 1);
            ret = SCE_ERROR_ERRNO_INVALID_ARGUMENT;
        } else if (g_isofsMounted == 0 || unk != 0) {
            isofsClearCurrentDirLbn();
            while (1) {
                ret = isofsUnitSetLbns(unit->blockDev, unit->unitNum);
                if (ret == SCE_ERROR_OK) {
                    break;
                }
                // Not sure what's going on here
                u32 ret2 = ret + 0x7fdeef60;
                if (ret + 0x7fdeffffU < 3 || ret == (s32) 0xc0210004 || ret == (s32) SCE_ERROR_ERRNO_INVALID_ARGUMENT ||
                    (ret = SCE_UMD_ERROR_NO_MEDIUM, ret2 < 2)) {
                    sceKernelSignalSema(g_isofsMgr.semaId, 1);
                    return ret;
                }
            }
            ret = isofsReadPvd(unit->blockDev, unit->unitNum);
            if (ret >= 0) {
                goto tryInitPaths;
            }
            sceKernelSignalSema(g_isofsMgr.semaId, 1);
        } else {
            sceKernelSignalSema(g_isofsMgr.semaId, 1);
            ret = 0;
        }
    }
    return ret;

tryInitPaths:
    isofsFreeUnitPaths(unit);
    ret = isofsCreatePaths(unit);
    if (ret != 0) {
        u32 ret2 = ret + 0x7fdeef60U;
        if (ret + 0x7fdeffffU < 2 || ret == (s32) SCE_ERROR_ERRNO_INVALID_ARGUMENT ||
            (ret == (s32) SCE_ERROR_ERRNO_NO_MEMORY || ret == (s32) SCE_UMD_ERROR_NO_MEDIUM) ||
            (ret == (s32) 0xc0210004 || (ret = (s32) SCE_UMD_ERROR_NO_MEDIUM, ret2 < 2))) {
            sceKernelSignalSema(g_isofsMgr.semaId, 1);
            return ret;
        }
        goto tryInitPaths;
    }
    unit->mountState = 2;
    if (g_isofsMounted2 == 0) {
        g_isofsMounted2 = 1;
    } else {
        sceUmdSetDriveStatus(4);
        s32 intr = sceKernelCpuSuspendIntr();
        IsofsDir *openDirs = g_isofsOpenDirs;
        if (unit->openState == 1) {
            unit->openState = 2;
            openDirs = g_isofsOpenDirs;
        }
        for (; openDirs != NULL; openDirs = openDirs->nextDir) {
            openDirs->compatFlag = (u32) (memcmp(openDirs->umdId, unit->umdId, UMD_ID_SIZE) != 0);
        }
        for (s32 iFdw = 0; iFdw < 2; iFdw++) {
            for (s32 iFile = 0; iFile < unit->fdw[iFdw].fd.filesCount; iFile++) {
                IsofsFile *file = &unit->fdw[iFdw].fd.files[iFile];
                if (file->flags != 0) {
                    volatile s32 matches = memcmp(file->umdId, unit->umdId, UMD_ID_SIZE);
                    if (matches == 0) {
                        file->flags = file->flags & 0xf7;
                    } else {
                        file->flags = file->flags | 8;
                    }
                }
            }
        }
        sceKernelCpuResumeIntr(intr);
    }
    g_isofsMounted = 1;
    sceKernelSignalSema(g_isofsMgr.semaId, 1);
    return ret;
}

// 000042c8
s32 isofsInit() {
    s32 driveType = sceUmdMan_driver_65E2B3E0();
    if (driveType == 0) {
        g_isofsMgr.driveType = 1;
    } else {
        g_isofsMgr.driveType = 2;
    }
    g_isofsMgr.fdwCount = 2;
    g_isofsMgr.maxUnits = 1;
    g_isofsMgr.maxFiles = 32;
    g_isofsMgr.initialized = 1;
    return sceIoAddDrv(&g_isofsDrv) < 0;
}

// 00004330
s32 isofsRebootBefore() {
    for (s32 i = 0; i < g_isofsMgr.maxUnits; i++) {
        IsofsUnit *unit = isofsGetUnit(i);
        if (unit != NULL && unit->semaId >= 0) {
            sceKernelDeleteSema(unit->semaId);
        }
    }
    if (g_isofsMgr.semaId >= 0) {
        sceKernelDeleteSema(g_isofsMgr.semaId);
    }
    return SCE_ERROR_OK;
}

// 000043c8
IsofsMgr * __attribute__ ((noinline)) isofsGetMgr() {
    return &g_isofsMgr;
}

// 000043d4
s32 isofsHandlerReadSectorsArgs(void *args) {
    IsofsReadSectorsArgs *a = args;
    if (a == NULL) {
        return SCE_ERROR_ERRNO_INVALID_ARGUMENT;
    }
    return isofsHandlerReadSectors(a->unit, a->file, a->outsize, a->outdata, a->outsize, a->flags, 0);
}

// 00004418
int isofsDrvChdir(__attribute__((unused)) SceIoIob *iob, __attribute__((unused)) char *path) {
    return SCE_ERROR_OK;
}

// 00004420
void isofsUnlinkOpenedDir(IsofsDir *dir) {
    IsofsDir *prev = dir->prevDir;
    IsofsDir *next = dir->nextDir;
    if (prev == NULL) {
        g_isofsOpenDirs = next;
        if (next != NULL) {
            next->prevDir = NULL;
        }
    } else {
        prev->nextDir = next;
        if (next != NULL) {
            next->prevDir = prev;
        }
    }
}

// 00004454
s32 isofsReadPvd(char *blockDev, s32 unitNum) {
    if ((unitNum >> 0x1f | (u32) (blockDev == NULL)) != 0) {
        return SCE_ERROR_ERRNO_INVALID_ARGUMENT;
    }
    IsofsUnit *unit = isofsGetUnit(unitNum);
    if (unit == NULL) {
        return SCE_ERROR_ERRNO_DEVICE_NOT_FOUND;
    }
    unit->unitNum = (s16) unitNum;
    return isofsHandlerReadPvd(unit, NULL, 0);
}

// 000044bc
s32 isofsUnitSetLbns(char *blockDev, s32 unitNum) {
    if (((u32) (blockDev == NULL) | unitNum >> 0x1f) != 0) {
        return SCE_ERROR_ERRNO_INVALID_ARGUMENT;
    }
    IsofsUnit *unit = isofsGetUnit(unitNum);
    s32 ret = SCE_ERROR_ERRNO_INVALID_ARGUMENT;
    if (unit != NULL && (ret = isofsHandlerGetSectorSize(unit), ret > -1) &&
        (ret = isofsHandlerGetTotalNumberOfSectors(unit), ret > -1)) {
        unit->lastLbn = unit->totalSectors + 1;
        ret = isofsHandlerGetCurrentLbn(unit);
        if (ret > -1) {
            unit->unkLbnTillEnd = unit->currentLbn - unit->lastLbn + 1;
            unit->unkNextLbn = unit->currentLbn + 1;
            unit->unkCurrentLbn = unit->currentLbn;
        }
    }
    return ret;
}

// 00004574
s32 isofsIoctl(SceIoIob *iob, s32 cmd, void *indata, SceSize inlen, void *outdata, SceSize outlen) {
    if (iob == NULL || cmd < 1) {
        return SCE_ERROR_ERRNO_INVALID_ARGUMENT;
    }
    IsofsUnit *unit = isofsGetUnit(iob->i_unit);
    IsofsFile *file = iob->i_private;
    if (unit == NULL || file == NULL) {
        return SCE_ERROR_ERRNO_INVALID_ARGUMENT;
    }

    if (cmd == 0x101000a) {
        if (indata == NULL || inlen < 4) {
            return SCE_ERROR_ERRNO_INVALID_ARGUMENT;
        }
        return isofsHandlerPrepareIntoCache(unit, file, *(s32 *) indata, 1);
    }
    if (cmd < 0x101000b) {
        if (cmd < 0x208083) {
            if (cmd > 0x208080) {
                return SCE_ERROR_OK;
            }
            if (cmd == 0x20800c) {
                return SCE_ERROR_OK;
            }
            if (cmd < 0x20800d) {
                if (cmd == 0x208001) {
                    return SCE_ERROR_OK;
                }
                if (cmd == 0x208006) {
                    return SCE_ERROR_OK;
                }
            } else if (cmd - 0x208010U < 2) {
                return SCE_ERROR_OK;
            }
        } else {
            if (cmd == 0x100000e) {
                return isofsHandler_unk38(unit, file);
            }
            if (cmd < 0x100000f) {
                if (cmd == 0x100000d) {
                    return isofsHandler_unk34(unit, file);
                }
            } else {
                if (cmd == 0x1010005) {
                    if (file == NULL) {
                        return SCE_ERROR_ERRNO_INVALID_FILE_DESCRIPTOR;
                    }
                    if (indata == NULL || inlen < 4) {
                        return SCE_ERROR_ERRNO_INVALID_ARGUMENT;
                    }
                    SceOff pos = isofsLseek(file, *(s64 *) indata, *(s32 *) ((s32) indata + 0xc));
                    if ((s32) pos < 0) {
                        return pos;
                    }
                    u32 unk = 0;
                    if (pos >= 0x800) {
                        s64 unk1 = pos + (u64) ((u32) (pos >> 0x3f) >> 0x15);
                        unk = (u32) unk1 >> 0xb | (s32) ((u64) unk1 >> 0x20) * 0x200000;
                    }
                    iob->i_fpos = pos;
                    return isofsHandlerSeek(unit, file->lbn + unk);
                }
                if (cmd == 0x1010009) {
                    if (indata == NULL || inlen < 4) {
                        return SCE_ERROR_ERRNO_INVALID_ARGUMENT;
                    }
                    return isofsHandlerPrepareIntoCache(unit, file, *(s32 *) indata, 0);
                }
            }
        }
    } else {
        if (cmd == 0x1020006) {
            if (outdata == NULL || outlen < 4) {
                return SCE_ERROR_ERRNO_INVALID_ARGUMENT;
            }
            if (file == NULL) {
                return SCE_ERROR_ERRNO_INVALID_FILE_DESCRIPTOR;
            }
            *(u32 *) outdata = file->lbn;
            return SCE_ERROR_OK;
        }
        if (cmd < 0x1020007) {
            if (cmd == 0x1020002) {
                if (outdata == NULL) {
                    return SCE_ERROR_ERRNO_INVALID_ARGUMENT;
                }
                if (unit->pathTableSize <= outlen) {
                    memcpy(outdata, unit->pathTable, unit->pathTableSize);
                    return SCE_ERROR_OK;
                }
                return SCE_ERROR_ERRNO_INVALID_ARGUMENT;
            }
            if (cmd > 0x1020002) {
                u32 size = 0;
                if (cmd == 0x1020003) {
                    size = 0x800;
                    if (outdata == NULL || outlen < 4) {
                        return SCE_ERROR_ERRNO_INVALID_ARGUMENT;
                    }
                } else {
                    if (cmd != 0x1020004) {
                        return SCE_ERROR_ERRNO_NOT_SUPPORTED;
                    }
                    if (outlen < 4) {
                        return SCE_ERROR_ERRNO_INVALID_ARGUMENT;
                    }
                    size = *(u32 *) &file->fpos;
                }
                *(u32 *) outdata = size;
                return SCE_ERROR_OK;
            }
            if (cmd == 0x101000b) {
                return isofsHandler_unk30(unit, file, 0, NULL);
            }
            if (cmd == 0x1020001) {
                if (outdata == NULL || outlen < 0x800) {
                    return SCE_ERROR_ERRNO_INVALID_ARGUMENT;
                }
                u8 *pvd = unit->primaryVolumeDescriptor;
                if ((((u32) pvd | (u32) outdata) & 3) != 0) {
                    __builtin_memcpy(outdata, unit->primaryVolumeDescriptor, 0x800);
                } else {
                    // Faster copy to aligned pointer
                    u32 *pvd32 = (u32 *) pvd;
                    u32 *pvdEnd = pvd32 + 0x200;
                    u32 *out32 = outdata;
                    do {
                        u32 d0 = *pvd32;
                        u32 d1 = *(pvd32 + 1);
                        u32 d2 = *(pvd32 + 2);
                        u32 d3 = *(pvd32 + 3);
                        *out32 = d0;
                        *(out32 + 1) = d1;
                        *(out32 + 2) = d2;
                        *(out32 + 3) = d3;
                        pvd32 += 4;
                        out32 += 4;
                    } while (pvd32 != pvdEnd);
                }
                return SCE_ERROR_OK;
            }
        } else {
            if (cmd == 0x103000c) {
                if (outdata == NULL || outlen < 4) {
                    return SCE_ERROR_ERRNO_INVALID_ARGUMENT;
                }
                return isofsHandler_unk30(unit, file, 1, outdata);
            }
            if (cmd < 0x103000d) {
                if (cmd == 0x1020007) {
                    if (outdata == NULL || outlen < 8) {
                        return SCE_ERROR_ERRNO_INVALID_ARGUMENT;
                    }
                    if (file == NULL) {
                        return SCE_ERROR_ERRNO_INVALID_FILE_DESCRIPTOR;
                    }
                    *(SceOff *) outdata = file->size;
                    return SCE_ERROR_OK;
                }
                if (cmd == 0x1030008) {
                    if (file == NULL) {
                        return SCE_ERROR_ERRNO_INVALID_FILE_DESCRIPTOR;
                    }
                    if (indata == NULL || inlen < 4) {
                        return SCE_ERROR_ERRNO_INVALID_ARGUMENT;
                    }
                    if (outdata == NULL) {
                        return SCE_ERROR_ERRNO_INVALID_ARGUMENT;
                    }
                    IsofsReadSectorsArgs args;
                    args.unit = unit;
                    args.file = file;
                    args.flags = file->readFlags | 1;
                    args.unk14 = 0;
                    args.outdata = outdata;
                    args.outsize = *(s32 *) indata;
                    if (outlen < (u32) args.outsize) {
                        return SCE_ERROR_ERRNO_INVALID_ARGUMENT;
                    }
                    s32 ret = sceKernelExtendKernelStack(0x2000, isofsHandlerReadSectorsArgs, &args);
                    if ((file->flags & 8) != 0) {
                        return SCE_ERROR_ERRNO_INVALID_FILE_DESCRIPTOR;
                    }
                    if (ret < 0) {
                        return ret;
                    }
                    file->fpos += ret;
                    iob->i_fpos += ret;
                    u32 fposHi = *(u32 *) ((u32) &file->fpos + 4);
                    u32 sizeHi = *(u32 *) ((u32) &file->size + 4);
                    if (fposHi <= sizeHi) {
                        if (fposHi != sizeHi) {
                            return ret;
                        }
                        if (*(u32 *) &file->fpos <= *(u32 *) &file->size) {
                            return ret;
                        }
                    }
                    file->fpos = file->size;
                    iob->i_fpos = file->size;
                    return ret;
                }
            } else {
                if (cmd == 0x1118011) {
                    if (indata == NULL) {
                        return SCE_ERROR_ERRNO_INVALID_ARGUMENT;
                    }
                    return isofsHandler_unk1c(unit, 1, indata, inlen, 0, 0);
                }
                if (cmd == 0x1138001) {
                    if (indata == NULL || 3 < inlen) {
                        return SCE_ERROR_ERRNO_INVALID_ARGUMENT;
                    }
                    if (outdata != NULL) {
                        IsofsReadSectorsArgs args;
                        args.unit = unit;
                        args.file = file;
                        args.flags = 6;
                        args.unk14 = 0;
                        args.outdata = outdata;
                        args.outsize = *(s32 *) indata;
                        if (outlen < (u32) args.outsize) {
                            return sceKernelExtendKernelStack(0x1800, isofsHandlerReadSectorsArgs, &unit);
                        }
                        return SCE_ERROR_ERRNO_INVALID_ARGUMENT;
                    }
                    return SCE_ERROR_ERRNO_INVALID_ARGUMENT;
                }
            }
        }
    }
    return SCE_ERROR_ERRNO_NOT_SUPPORTED;
}

// 00004c8c
SceOff isofsLseek(IsofsFile *file, SceOff pos, int whence) {
    if (file == NULL) {
        return (s32) SCE_ERROR_ERRNO_INVALID_ARGUMENT;
    }
    SceOff newPos;
    if (whence == SCE_SEEK_SET) {
        newPos = pos;
    } else if (whence == SCE_SEEK_CUR) {
        newPos = file->fpos + pos;
    } else if (whence == SCE_SEEK_END) {
        newPos = file->size + pos;
    } else {
        return (s32) SCE_ERROR_ERRNO_INVALID_ARGUMENT;
    }
    if (newPos > file->size || newPos < 0) {
        return (s32) SCE_ERROR_ERRNO_IO_ERROR;
    }
    file->fpos = newPos;
    return newPos;
}

// 00004d84
s32 isofsRegisterOnUmdMounted(IsofsUnit *unit) {
    s32 ret = sceUmdManRegisterInsertEjectUMDCallBack(0x20, isofsUmdMountedCb, unit);
    if (ret > 0) {
        return SCE_ERROR_OK;
    }
    return ret;
}

// 00004db4
s32 __attribute__ ((noinline)) isofsUmdManUnRegisterInsertEjectUMDCallBack() {
    sceUmdManUnRegisterInsertEjectUMDCallBack(0x20);
    return SCE_ERROR_OK;
}

// 00004dd4
s32 isofsSetBlockDevHandler(IsofsUnit *unit) {
    if (unit == NULL || unit == (IsofsUnit *) 0xffffffe0) {
        return SCE_ERROR_ERRNO_INVALID_ARGUMENT;
    }
    char *blockDev = unit->blockDev;
    s32 blockDevLen = strlen(blockDev);
    if (blockDevLen > 0x100) {
        return SCE_ERROR_ERRNO_NAME_TOO_LONG;
    }
    int n1 = 0;
    if (blockDevLen > 0) {
        do {
            u32 ctype = look_ctype_table(unit->blockDev[n1]);
            if ((ctype & 8) == 0) {
                break;
            }
            n1 = n1 + 1;
            blockDev = blockDev + 1;
        } while (n1 < blockDevLen);
    }
    int n2 = 0;
    if (blockDevLen - n1 > 0) {
        do {
            char *blockDevOff = blockDev + n2;
            u32 ctype = look_ctype_table(*blockDevOff);
            if ((ctype & 3) == 0) {
                break;
            }
            n2 = n2 + 1;
        } while (n2 < blockDevLen - n1);
    }
    for (int i = 0; i < 4; i++) {
        if (strncmp(blockDev, g_isofsHandlers[i].blockDev, n2) == 0) {
            unit->handlerIdx = i;
            return SCE_ERROR_OK;
        }
    }
    return SCE_ERROR_ERRNO_DEVICE_NOT_FOUND;
}

// 00005048
s32 isofsUmd_1f100a6_1f100a7(IsofsUnit *unit, IsofsFile *file, s32 flag) {
    if (unit == NULL || file == NULL) {
        return SCE_ERROR_ERRNO_INVALID_ARGUMENT;
    }
    s32 indata = file->unkA;
    u32 cmd = 0x1f300a7;
    if (flag == 0) {
        cmd = 0x1f100a6;
    }
    return sceIoDevctl(unit->blockDev, cmd, &indata, sizeof(indata), NULL, 0);
}

// 00004f04
s32 isofsUmdPrepareIntoCache(IsofsUnit *unit, IsofsFile *file, s32 cacheSize, s32 getStatusFlag) {
    if (unit == NULL || file == NULL || cacheSize < 1) {
        return SCE_ERROR_ERRNO_INVALID_ARGUMENT;
    }
    s32 sectorSize = unit->sectorSize;
    PrepareIntoCacheParams params;
    params.unk0 = 0;
    params.unk8 = 0;
    params.lbnSize = 0;
    u64 posLbn = __udivdi3(file->fpos, sectorSize);
    params.lbn = file->lbn + (int) posLbn;
    params.lbnSize = 1;
    if (cacheSize > sectorSize) {
        params.lbnSize = cacheSize / sectorSize;
        if (sectorSize == 0) {
            pspBreak(SCE_BREAKCODE_DIVZERO);
        }
        if (cacheSize % sectorSize != 0) {
            params.lbnSize = params.lbnSize + 1;
        }
        if (sectorSize == 0) {
            pspBreak(SCE_BREAKCODE_DIVZERO);
        }
    }

    if (getStatusFlag == 0) {
        return sceIoDevctl(unit->blockDev, DEVCTL_PREPARE_INTO_CACHE, &params, sizeof(PrepareIntoCacheParams),NULL, 0);
    }
    u16 statusOut[2];
    statusOut[0] = 0;
    statusOut[1] = 0;
    s32 ret = sceIoDevctl(unit->blockDev, DEVCTL_PREPARE_INTO_CACHE_GET_STATUS, &params, sizeof(PrepareIntoCacheParams),
                          statusOut, 4);
    if (ret >= 0) {
        file->unkA = statusOut[0];
    }
    return ret;
}

// 000050c4
s32 isofsUmdReadDirSectors(IsofsUnit *unit, s64 lbn, s32 size, void *outdata, s32 outlen, u32 cmd) {
    if (unit == NULL || outdata == NULL || outlen < size) {
        return SCE_ERROR_ERRNO_INVALID_ARGUMENT;
    }

    LbnParams params;
    params.lbn = (u32) lbn;
    params.lbnSize = 0;

    u32 lbnSize;
    const u32 sectorSize = unit->sectorSize;
    if ((int) sectorSize < size) {
        lbnSize = size / (int) sectorSize;
        if (sectorSize == 0) {
            pspBreak(SCE_BREAKCODE_DIVZERO);
        }
        if (size % (int) sectorSize != 0) {
            // incomplete params setup, won't this always fail?
            goto doRead;
        }
        params.byteSizeMiddle = size;
        if (sectorSize == 0) {
            pspBreak(SCE_BREAKCODE_DIVZERO);
        }
    } else {
        lbnSize = 1;
        params.byteSizeMiddle = sectorSize;
    }

    params.byteSizeLast = 0;
    params.byteSizeFirst = 0;
    params.lbnSize = lbnSize;
    params.size = params.byteSizeMiddle;
doRead:
    params.unk0 = (u32) ((cmd & 1) != 0);
    params.cmd = (u32) ((cmd & 2) != 0);
    if ((cmd & 4) != 0) {
        params.cmd = params.cmd | 2;
    }
    if ((cmd & 8) != 0) {
        params.cmd = params.cmd | 0xf0;
    }
    return sceIoDevctl(unit->blockDev, DEVCTL_READ_SECTORS, &params, sizeof(LbnParams), outdata, params.size);
}

// 000051d8
s32 isofsUmdReadSectors(IsofsUnit *unit, IsofsFile *file, s32 size, void *outdata, s32 outlen, u32 cmd) {
    if (unit == NULL || file == NULL || outdata == NULL || outlen < size) {
        return SCE_ERROR_ERRNO_INVALID_ARGUMENT;
    }
    if (size == 0) {
        return SCE_ERROR_OK;
    }

    LbnParams params;
    params.unk0 = (u32) ((cmd & 1) != 0);
    params.cmd = (u32) ((cmd & 2) != 0);
    params.lbn = 0;
    params.lbnSize = 0;
    params.size = 0;
    params.byteSizeMiddle = 0;
    params.byteSizeFirst = 0;
    params.byteSizeLast = 0;
    params.unk20 = 0;
    if ((cmd & 4) != 0) {
        params.cmd = params.cmd | 2;
    }
    if ((cmd & 8) != 0) {
        params.cmd = params.cmd | 0xf0;
    }

    const u32 fposHi = *(u32 *) ((u32) &file->fpos + 4);
    const u32 fpos = *(u32 *) &file->fpos;
    const u32 sizeHi = *(u32 *) ((u32) &file->size + 4);
    const u32 sizeLo = *(u32 *) &file->size;

    const u32 readEnd = fpos + size;
    const u32 unk = *(int *) ((int) &file->fpos + 4) + (size >> 0x1f) + (u32) (readEnd < (u32) size);
    if (sizeHi < unk || (sizeHi == unk && sizeLo < readEnd)) {
        size = (s32) (sizeLo - fpos);
    }
    if (size == 0) {
        return SCE_ERROR_OK;
    }

    s32 ret;
    if ((fpos | fposHi) == 0) {
        ret = isofsReadParamsZero(unit, file, outdata, size, &params);
    } else if (fposHi == 0 && fpos < unit->sectorSize) {
        ret = isofsReadParamsInitial(unit, file, outdata, size, &params);
    } else {
        volatile u32 sectorSize = unit->sectorSize; // force not to optimize the following if to match asm
        if ((fpos | fposHi) == 0 || (fposHi == 0 && fpos < sectorSize)) {
            return SCE_ERROR_ERRNO_INVALID_ARGUMENT;
        }
        ret = isofsReadParams(unit, file, outdata, size, &params);
    }
    if (ret != SCE_ERROR_OK) {
        return ret;
    }
    ret = sceIoDevctl(unit->blockDev, DEVCTL_READ_SECTORS, &params, sizeof(LbnParams), outdata, size);
    if (ret < 0) {
        return ret;
    }
    return size;
}

// 000053fc
s32 isofsReadParamsZero(IsofsUnit *unit, IsofsFile *file, void *outdata, s32 size, LbnParams *params) {
    if (unit == NULL || file == NULL || params == NULL || size == 0) {
        return SCE_ERROR_ERRNO_INVALID_ARGUMENT;
    }
    const s32 sectorSize = unit->sectorSize;
    const u32 outdataEnd = (u32) outdata + size;
    // read smaller than sector size
    if (size <= sectorSize) {
        params->lbn = file->lbn;
        params->lbnSize = 1;
        params->size = size;
        params->byteSizeMiddle = 0;
        params->byteSizeFirst = 0;
        params->byteSizeLast = size;
        return SCE_ERROR_OK;
    }

    // read smaller than size of two sectors, outdata is NOT aligned to 64 bytes
    if (size <= sectorSize * 2 && ((u32) outdata & 0x3f) != 0) {
        params->lbn = file->lbn;
        params->lbnSize = 2;
        params->size = size;
        params->byteSizeMiddle = 0;
        params->byteSizeFirst = sectorSize;
        params->byteSizeLast = size - sectorSize;
        return SCE_ERROR_OK;
    }

    // read smaller than size of two sectors, outdata is aligned to 64 bytes
    if (size <= sectorSize * 2) {
        params->lbn = file->lbn;
        params->lbnSize = 2;
        params->size = size;
        params->byteSizeMiddle = sectorSize;
        params->byteSizeFirst = 0;
        params->byteSizeLast = size - sectorSize;
        return SCE_ERROR_OK;
    }

    // read larger than two sectors, outdata start NOT aligned, end NOT aligned
    if (size > sectorSize * 2) {
        if (sectorSize == 0) {
            pspBreak(SCE_BREAKCODE_DIVZERO);
        }
        if (size % sectorSize == 0 && ((u32) outdata & 0x3f) != 0 && (outdataEnd & 0xffffffc0) != outdataEnd) {
            params->lbn = file->lbn;
            if (sectorSize == 0) {
                pspBreak(SCE_BREAKCODE_DIVZERO);
            }
            params->lbnSize = size / sectorSize;
            params->size = size;
            params->byteSizeMiddle = size - sectorSize * 2;
            params->byteSizeFirst = sectorSize;
            params->byteSizeLast = sectorSize;
            return SCE_ERROR_OK;
        }
    }

    // read larger than two sectors, outdata start aligned, end NOT aligned
    if (size > sectorSize * 2) {
        if (sectorSize == 0) {
            pspBreak(SCE_BREAKCODE_DIVZERO);
        }
        if (size % sectorSize == 0 && (outdataEnd & 0xffffffc0) != outdataEnd) {
            params->lbn = file->lbn;
            if (sectorSize == 0) {
                pspBreak(SCE_BREAKCODE_DIVZERO);
            }
            params->lbnSize = size / sectorSize;
            params->size = size;
            params->byteSizeMiddle = size - sectorSize;
            params->byteSizeFirst = 0;
            params->byteSizeLast = sectorSize;
            return SCE_ERROR_OK;
        }
    }

    // read larger than two sectors, outdata start NOT aligned, end aligned
    if (size > sectorSize * 2) {
        if (sectorSize == 0) {
            pspBreak(SCE_BREAKCODE_DIVZERO);
        }
        if (size % sectorSize == 0 && ((u32) outdata & 0x3f) != 0) {
            params->lbn = file->lbn;
            if (sectorSize == 0) {
                pspBreak(SCE_BREAKCODE_DIVZERO);
            }
            params->lbnSize = size / sectorSize;
            params->size = size;
            params->byteSizeMiddle = size - sectorSize;
            params->byteSizeFirst = sectorSize;
            params->byteSizeLast = 0;
            return SCE_ERROR_OK;
        }
    }

    // read larger than two sectors, outdata start and end aligned
    if (size > sectorSize * 2) {
        if (sectorSize == 0) {
            pspBreak(SCE_BREAKCODE_DIVZERO);
        }
        if (size % sectorSize == 0) {
            params->lbn = file->lbn;
            if (sectorSize == 0) {
                pspBreak(SCE_BREAKCODE_DIVZERO);
            }
            params->lbnSize = size / sectorSize;
            params->size = size;
            params->byteSizeMiddle = size;
            params->byteSizeFirst = 0;
            params->byteSizeLast = 0;
            return SCE_ERROR_OK;
        }
    }

    // read larger than two sectors, size is not divisible by the sector size, outdata start NOT aligned
    if (size > sectorSize * 2) {
        if (sectorSize == 0) {
            pspBreak(SCE_BREAKCODE_DIVZERO);
        }
        if (size % sectorSize != 0 && ((u32) outdata & 0x3f) != 0) {
            u32 diff = (size - sectorSize) % sectorSize;
            if (sectorSize == 0) {
                pspBreak(SCE_BREAKCODE_DIVZERO);
            }
            params->lbn = file->lbn;
            params->lbnSize = size / sectorSize + 1;
            params->size = size;
            params->byteSizeMiddle = size - sectorSize - diff;
            params->byteSizeFirst = sectorSize;
            params->byteSizeLast = diff;
            return SCE_ERROR_OK;
        }
    }

    // read larger than two sectors, size is not divisible by the sector size
    if (size > sectorSize * 2) {
        u32 bytesOver = size % sectorSize;
        if (sectorSize == 0) {
            pspBreak(SCE_BREAKCODE_DIVZERO);
        }
        if (bytesOver == 0) {
            return SCE_ERROR_ERRNO_INVALID_ARGUMENT;
        }
        if (sectorSize == 0) {
            pspBreak(SCE_BREAKCODE_DIVZERO);
        }
        params->lbn = file->lbn;
        params->lbnSize = size / sectorSize + 1;
        params->size = size;
        params->byteSizeMiddle = size - bytesOver;
        params->byteSizeFirst = 0;
        params->byteSizeLast = bytesOver;
        return SCE_ERROR_OK;
    }

    return SCE_ERROR_ERRNO_INVALID_ARGUMENT;
}

// 000057c8
s32 isofsReadParamsInitial(IsofsUnit *unit, IsofsFile *file, __attribute__((unused)) void *outdata, s32 size,
                           LbnParams *params) {
    if (unit == NULL || file == NULL || params == NULL || size == 0) {
        return SCE_ERROR_ERRNO_INVALID_ARGUMENT;
    }

    const u32 usize = size;
    const s32 sizeMsb = (s32) usize >> 0x1f;
    const u32 sectorSize = unit->sectorSize;
    const u32 fposHi = *(u32 *) ((u32) &file->fpos + 4);
    const u32 fpos = *(u32 *) &file->fpos;
    const u32 readEnd = *(s32 *) &file->fpos + usize;
    const u32 byteSizeFirst = sectorSize - fpos;

    const s32 unk0 = *(s32 *) ((s32) &file->fpos + 4) + sizeMsb + (u32) (readEnd < usize);
    const s32 unk1 = unk0 != 0 || readEnd > sectorSize;
    const s32 unk2 = unk0 != 0 || readEnd > sectorSize * 2;
    const s32 unk3 = unk0 == 0 && readEnd <= sectorSize * 2;

    // read one sector
    if (fposHi == 0 && fpos < sectorSize && unk0 == 0 && readEnd <= sectorSize) {
        params->lbn = file->lbn;
        params->lbnSize = 1;
        params->size = usize;
        params->byteSizeMiddle = 0;
        params->byteSizeFirst = fpos;
        params->byteSizeLast = 0;
        return SCE_ERROR_OK;
    }

    // read two sectors
    if (fposHi == 0 && fpos < sectorSize) {
        if (unk1 && unk3) {
            params->lbn = file->lbn;
            params->lbnSize = 2;
            params->size = usize;
            params->byteSizeMiddle = 0;
            params->byteSizeFirst = byteSizeFirst;
            params->byteSizeLast = usize - byteSizeFirst;
            return SCE_ERROR_OK;
        }
    }

    // read larger than two sectors, size is divisible by the sector size
    if (fposHi == 0 && fpos < sectorSize) {
        if (unk1 && unk2) {
            u32 byteSizeMiddle = usize - byteSizeFirst;
            if (sectorSize == 0) {
                pspBreak(SCE_BREAKCODE_DIVZERO);
            }
            if (byteSizeMiddle % sectorSize == 0) {
                params->lbn = file->lbn;
                if (sectorSize == 0) {
                    pspBreak(SCE_BREAKCODE_DIVZERO);
                }
                params->lbnSize = byteSizeMiddle / sectorSize + 1;
                params->size = usize;
                params->byteSizeMiddle = byteSizeMiddle;
                params->byteSizeFirst = byteSizeFirst;
                params->byteSizeLast = 0;
                return SCE_ERROR_OK;
            }
        }
    }

    // read larger than two sectors, size is not divisible by the sector size
    if (fposHi == 0 && fpos < sectorSize) {
        if (unk1 && unk2) {
            u32 byteSizeLast = (usize - byteSizeFirst) % sectorSize;
            if (sectorSize == 0) {
                pspBreak(SCE_BREAKCODE_DIVZERO);
            }
            if (byteSizeLast != 0) {
                if (sectorSize == 0) {
                    pspBreak(SCE_BREAKCODE_DIVZERO);
                }
                u32 byteSizeMiddle = usize - (byteSizeLast + byteSizeFirst);
                params->lbn = file->lbn;
                params->lbnSize = byteSizeMiddle / sectorSize + 2;
                params->size = usize;
                params->byteSizeMiddle = byteSizeMiddle;
                params->byteSizeFirst = byteSizeFirst;
                params->byteSizeLast = byteSizeLast;
                return SCE_ERROR_OK;
            }
        }
    }

    return SCE_ERROR_ERRNO_INVALID_ARGUMENT;
}

// 00005b14
s32 isofsReadParams(IsofsUnit *unit, IsofsFile *file, void *outdata, s32 size, LbnParams *params) {
    if (unit == NULL || file == NULL || params == NULL) {
        return SCE_ERROR_ERRNO_INVALID_ARGUMENT;
    }
    if (size == 0) {
        return SCE_ERROR_OK;
    }

    const u32 sectorSize = unit->sectorSize;
    const s64 sectorSizeS64 = (s32) sectorSize;
    const u32 outdataEnd = (u32) outdata + size;
    const u32 fposSectors = __udivdi3(file->fpos, sectorSizeS64);
    const u32 fposRemBytes = __umoddi3(file->fpos, sectorSizeS64);

    if (fposRemBytes != 0) {
        const u32 byteSizeFirst = sectorSize - fposRemBytes;
        const u32 byteSizeLast = size - byteSizeFirst;
        const u32 remainingBytes = byteSizeLast % sectorSize;
        if (sectorSize == 0) {
            pspBreak(SCE_BREAKCODE_DIVZERO);
        }
        if (sectorSize < fposRemBytes + size) {
            if (remainingBytes != 0) {
                if (byteSizeLast <= sectorSize && byteSizeFirst != 0) {
                    params->lbn = file->lbn + fposSectors;
                    params->lbnSize = 2;
                    params->size = size;
                    params->byteSizeMiddle = 0;
                    params->byteSizeFirst = byteSizeFirst;
                    params->byteSizeLast = byteSizeLast;
                    return SCE_ERROR_OK;
                }

                if (sectorSize == 0) {
                    pspBreak(SCE_BREAKCODE_DIVZERO);
                }
                params->lbn = file->lbn + fposSectors;
                params->lbnSize = (byteSizeLast - remainingBytes) / sectorSize + 2;
                params->size = size;
                params->byteSizeMiddle = byteSizeLast - remainingBytes;
                params->byteSizeFirst = byteSizeFirst;
                params->byteSizeLast = remainingBytes;
                return SCE_ERROR_OK;
            }

            if (sectorSize == 0) {
                pspBreak(SCE_BREAKCODE_DIVZERO);
            }
            params->lbn = file->lbn + fposSectors;
            params->lbnSize = byteSizeLast / sectorSize + 1;
            params->size = size;
            params->byteSizeMiddle = byteSizeLast;
            params->byteSizeFirst = byteSizeFirst;
        } else {
            params->lbn = file->lbn + fposSectors;
            params->lbnSize = 1;
            params->size = size;
            params->byteSizeMiddle = 0;
            params->byteSizeFirst = fposRemBytes;
        }
        params->byteSizeLast = 0;
        return SCE_ERROR_OK;
    }

    if (size <= (s32) sectorSize) {
        params->lbn = file->lbn + fposSectors;
        params->lbnSize = 1;
        params->size = size;
        params->byteSizeMiddle = 0;
        params->byteSizeFirst = 0;
        params->byteSizeLast = size;
        return SCE_ERROR_OK;
    }

    if (sectorSize == 0) {
        pspBreak(SCE_BREAKCODE_DIVZERO);
    }
    if (size % (s32) sectorSize == 0 && ((u32) outdata & 0x3f) != 0 && (outdataEnd & 0xffffffc0) != outdataEnd) {
        params->lbn = file->lbn + fposSectors;
        if (unit->sectorSize == 0) {
            pspBreak(SCE_BREAKCODE_DIVZERO);
        }
        params->lbnSize = size / (s32) unit->sectorSize;
        params->size = size;
        params->byteSizeMiddle = size + unit->sectorSize * -2;
        params->byteSizeFirst = unit->sectorSize;
        params->byteSizeLast = unit->sectorSize;
        return SCE_ERROR_OK;
    }

    if ((s32) sectorSize < size) {
        if (sectorSize == 0) {
            pspBreak(SCE_BREAKCODE_DIVZERO);
        }
        if (size % (s32) sectorSize == 0 && ((u32) outdata & 0x3f) != 0) {
            params->lbn = file->lbn + fposSectors;
            if (unit->sectorSize == 0) {
                pspBreak(SCE_BREAKCODE_DIVZERO);
            }
            params->lbnSize = size / (s32) unit->sectorSize;
            params->size = size;
            params->byteSizeMiddle = size - unit->sectorSize;
            params->byteSizeFirst = unit->sectorSize;
            params->byteSizeLast = 0;
            return SCE_ERROR_OK;
        }
    }

    if ((s32) sectorSize < size) {
        if (sectorSize == 0) {
            pspBreak(SCE_BREAKCODE_DIVZERO);
        }
        if (size % (s32) sectorSize == 0 && (outdataEnd & 0xffffffc0) != outdataEnd) {
            params->lbn = file->lbn + fposSectors;
            if (unit->sectorSize == 0) {
                pspBreak(SCE_BREAKCODE_DIVZERO);
            }
            params->lbnSize = size / (s32) unit->sectorSize;
            params->size = size;
            params->byteSizeMiddle = size - unit->sectorSize;
            params->byteSizeFirst = 0;
            params->byteSizeLast = unit->sectorSize;
            return SCE_ERROR_OK;
        }
    }

    if ((s32) sectorSize < size) {
        if (sectorSize == 0) {
            pspBreak(SCE_BREAKCODE_DIVZERO);
        }
        if (size % (s32) sectorSize == 0) {
            params->lbn = file->lbn + fposSectors;
            if (unit->sectorSize == 0) {
                pspBreak(SCE_BREAKCODE_DIVZERO);
            }
            params->lbnSize = size / (s32) unit->sectorSize;
            params->size = size;
            params->byteSizeMiddle = size;
            params->byteSizeFirst = 0;
            params->byteSizeLast = 0;
            return SCE_ERROR_OK;
        }
    }

    if ((s32) sectorSize < size) {
        u32 byteSizeLast = size % (s32) sectorSize;
        if (sectorSize == 0) {
            pspBreak(SCE_BREAKCODE_DIVZERO);
        }
        if (byteSizeLast != 0 && ((u32) outdata & 0x3f) != 0) {
            if (sectorSize == 0) {
                pspBreak(SCE_BREAKCODE_DIVZERO);
            }
            params->lbn = file->lbn + fposSectors;
            params->lbnSize = size / (s32) sectorSize + 1;
            params->size = size;
            params->byteSizeMiddle = size - unit->sectorSize - byteSizeLast;
            params->byteSizeFirst = unit->sectorSize;
            params->byteSizeLast = byteSizeLast;
            return SCE_ERROR_OK;
        }
    }

    if (size <= (s32) sectorSize) {
        return SCE_ERROR_OK;
    }
    u32 byteSizeLast = size % (s32) sectorSize;
    if (sectorSize == 0) {
        pspBreak(SCE_BREAKCODE_DIVZERO);
    }
    if (byteSizeLast == 0) {
        return SCE_ERROR_OK;
    }
    if (sectorSize == 0) {
        pspBreak(SCE_BREAKCODE_DIVZERO);
    }
    params->lbn = file->lbn + fposSectors;
    params->lbnSize = size / (s32) sectorSize + 1;
    params->size = size;
    params->byteSizeMiddle = size - byteSizeLast;
    params->byteSizeFirst = 0;
    params->byteSizeLast = byteSizeLast;
    return SCE_ERROR_OK;
}

// 00005fcc
s32 isofsUmd_1e180d3(IsofsUnit *unit, s32 unk, void *indata, SceSize inlen, __attribute__((unused)) s32 unk0,
                     __attribute__((unused)) s32 unk1) {
    if (unit == NULL || indata == NULL) {
        return SCE_ERROR_ERRNO_INVALID_ARGUMENT;
    }
    if (unk != 1) {
        return SCE_ERROR_OK;
    }
    if (inlen <= 0x17) {
        return SCE_ERROR_ERRNO_INVALID_ARGUMENT;
    }
    return sceIoDevctl(unit->blockDev, DEVCTL_UNKNOWN_1E180D3, indata, 0x18, NULL, 0);
}

// 00006040
s32 isofsClearBlockDevHandler(IsofsUnit *unit) {
    if (unit == NULL) {
        return SCE_ERROR_ERRNO_INVALID_ARGUMENT;
    }
    unit->handlerIdx = -1;
    return SCE_ERROR_OK;
}

// 00006060
s32 isofsHandlerReadDirSectors(IsofsUnit *unit, s64 lbn, s32 size, void *outdata, s32 outlen, s32 cmd,
                               __attribute__((unused)) u32 unkUnused) {
    if (unit == NULL || outdata == NULL) {
        return SCE_ERROR_ERRNO_INVALID_ARGUMENT;
    }
    return g_isofsHandlers[unit->handlerIdx].funcs->readDirSectors(unit, lbn, size, outdata, outlen, cmd);
}

// 000060c0
s32 isofsHandlerReadSectors(IsofsUnit *unit, IsofsFile *file, s32 size, void *outdata, s32 outlen, u32 unkFlags,
                            __attribute__((unused)) s32 unkUnused) {
    if (unit == NULL || file == NULL || outdata == NULL) {
        return SCE_ERROR_ERRNO_INVALID_ARGUMENT;
    }
    return g_isofsHandlers[unit->handlerIdx].funcs->readSectors(unit, file, size, outdata, outlen, unkFlags);
}

// 00006124
s32 isofsHandlerGetCurrentLbn(IsofsUnit *unit) {
    if (unit == NULL) {
        return SCE_ERROR_ERRNO_INVALID_ARGUMENT;
    }
    return g_isofsHandlers[unit->handlerIdx].funcs->getCurrentLbn(unit);
}

// 00006170
s32 isofsHandlerGetSectorSize(IsofsUnit *unit) {
    if (unit == NULL) {
        return SCE_ERROR_ERRNO_INVALID_ARGUMENT;
    }
    return g_isofsHandlers[unit->handlerIdx].funcs->getSectorSize(unit);
}

// 000061bc
s32 isofsHandlerGetTotalNumberOfSectors(IsofsUnit *unit) {
    if (unit == NULL) {
        return SCE_ERROR_ERRNO_INVALID_ARGUMENT;
    }
    return g_isofsHandlers[unit->handlerIdx].funcs->getTotalNumberOfSectors(unit);
}

// 00006208
s64 isofsHandlerSeek(IsofsUnit *unit, s64 lbn) {
    if (unit == NULL) {
        return SCE_ERROR_ERRNO_INVALID_ARGUMENT;
    }
    return g_isofsHandlers[unit->handlerIdx].funcs->seek(unit, lbn);
}

// 00006254
s32 isofsHandler_unk1c(IsofsUnit *unit, s32 unk0, void *indata, SceSize inlen, s32 unk1, s32 unk2) {
    if (unit == NULL) {
        return SCE_ERROR_ERRNO_INVALID_ARGUMENT;
    }
    return g_isofsHandlers[unit->handlerIdx].funcs->unk1C(unit, unk0, indata, inlen, unk1, unk2);
}

// 000062a0
s32 isofsHandlerReadPvd(IsofsUnit *unit, void *indata, SceSize inlen) {
    if (unit == NULL) {
        return SCE_ERROR_ERRNO_INVALID_ARGUMENT;
    }
    return g_isofsHandlers[unit->handlerIdx].funcs->readPvd(unit, indata, inlen);
}

// 000062ec
s32 isofsHandlerPrepareIntoCache(IsofsUnit *unit, IsofsFile *file, s32 cacheSize, s32 getStatusFlag) {
    if (unit == NULL || file == NULL) {
        return SCE_ERROR_ERRNO_INVALID_ARGUMENT;
    }
    return g_isofsHandlers[unit->handlerIdx].funcs->prepareIntoCache(unit, file, cacheSize, getStatusFlag);
}

// 00006344
s32 isofsHandler_unk30(IsofsUnit *unit, IsofsFile *file, s32 flag, __attribute__((unused)) void *outdata) {
    if (unit == NULL || file == NULL) {
        return SCE_ERROR_ERRNO_INVALID_ARGUMENT;
    }
    return g_isofsHandlers[unit->handlerIdx].funcs->unk30(unit, file, flag);
}

// 0000639c
s32 isofsHandler_unk34(IsofsUnit *unit, IsofsFile *file) {
    if (unit == NULL || file == NULL) {
        return SCE_ERROR_ERRNO_INVALID_ARGUMENT;
    }
    return g_isofsHandlers[unit->handlerIdx].funcs->unk34(unit, file);
}

// 000063f4
s32 isofsHandler_unk38(IsofsUnit *unit, IsofsFile *file) {
    if (unit == NULL || file == NULL) {
        return SCE_ERROR_ERRNO_INVALID_ARGUMENT;
    }
    return g_isofsHandlers[unit->handlerIdx].funcs->unk38(unit, file);
}

// 0000644c
s32 isofsUmdGetSectorSize(IsofsUnit *unit) {
    if (unit == NULL) {
        return SCE_ERROR_ERRNO_INVALID_ARGUMENT;
    }
    return sceIoDevctl(unit->blockDev, DEVCTL_GET_SECTOR_SIZE, NULL, 0, &unit->sectorSize,
                       sizeof(unit->sectorSize));
}

// 0000649c
s32 isofsUmdGetTotalNumberOfSectors(IsofsUnit *unit) {
    if (unit == NULL) {
        return SCE_ERROR_ERRNO_INVALID_ARGUMENT;
    }
    return sceIoDevctl(unit->blockDev, DEVCTL_GET_TOTAL_NUMBER_OF_SECTORS, NULL, 0, &unit->totalSectors,
                       sizeof(unit->totalSectors));
}

// 000064ec
s32 isofsUmdGetCurrentLbn(IsofsUnit *unit) {
    if (unit == NULL) {
        return SCE_ERROR_ERRNO_INVALID_ARGUMENT;
    }
    return sceIoDevctl(unit->blockDev, DEVCTL_GET_CURRENT_LBN, NULL, 0, &unit->currentLbn,
                       sizeof(unit->currentLbn));
}

// 0000653c
s32 isofsUmd_unkNotSupported() {
    return SCE_ERROR_ERRNO_NOT_SUPPORTED;
}

// 00006548
s32 isofsUmdReadPvd(IsofsUnit *unit, void *indata, SceSize inlen) {
    u8 *outdata;
    sceIoDevctl(unit->blockDev, DEVCTL_GET_PVD, indata, inlen, &outdata, sizeof(outdata));
    unit->primaryVolumeDescriptor = outdata;
    __builtin_memcpy(&unit->rootDirLbn, outdata + 0x9e, sizeof(unit->rootDirLbn));
    __builtin_memcpy(&unit->rootDirSize, outdata + 0xa6, sizeof(unit->rootDirSize));
    __builtin_memcpy(&unit->pathTableSize, outdata + 0x84, sizeof(unit->pathTableSize));
    __builtin_memcpy(&unit->pathTableLbn, outdata + 0x8c, sizeof(unit->pathTableLbn));
    __builtin_memcpy(unit->umdId, outdata + 0x373, UMD_ID_SIZE);
    return SCE_ERROR_OK;
}

// 000066c4
s64 isofsUmdSeek(IsofsUnit *unit, s64 lbn) {
    if (unit == NULL) {
        return SCE_ERROR_ERRNO_INVALID_ARGUMENT;
    }
    return sceIoDevctl(unit->blockDev, DEVCTL_SEEK_DISC_RAW, &lbn, sizeof(lbn), NULL, 0);
}

// 00006734
s32 isofsUmdGetUnitNum(IsofsUnit *unit, void *indata, SceSize inlen) {
    if (unit == NULL) {
        return SCE_ERROR_ERRNO_INVALID_ARGUMENT;
    }
    return sceIoDevctl(unit->blockDev, DEVCTL_GET_UNIT_NUM, indata, inlen, &unit->unitNum, sizeof(unit->unitNum));
}

//00006788
s32 isofsUmd_1f100a8(IsofsUnit *unit, IsofsFile *file) {
    if (unit == NULL) {
        return SCE_ERROR_ERRNO_INVALID_ARGUMENT;
    }
    s32 indata = file->unkA;
    return sceIoDevctl(unit->blockDev, DEVCTL_UNKNOWN_1F100A8, &indata, sizeof(indata), NULL, 0);
}

// 000067ec
s32 isofsUmd_1f100a9(IsofsUnit *unit, IsofsFile *file) {
    if (unit == NULL) {
        return SCE_ERROR_ERRNO_INVALID_ARGUMENT;
    }
    s32 indata = file->unkA;
    return sceIoDevctl(unit->blockDev, DEVCTL_UNKNOWN_1F100A9, &indata, sizeof(indata), NULL, 0);
}
