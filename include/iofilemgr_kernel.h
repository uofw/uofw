/* Copyright (C) 2011, 2012 The uOFW team
   See the file COPYING for copying permission.
*/

#include "common_header.h"

/* flags */
#define	SCE_FREAD       (0x0001)  /* readable */
#define	SCE_FWRITE      (0x0002)  /* writable */
#define	SCE_FNBLOCK     (0x0004)  /*   Reserved: non-blocking reads */
#define	SCE_FDIRO       (0x0008)  /* internal use for dopen */
#define	SCE_FRLOCK      (0x0010)  /*   Reserved: read locked (non-shared) */
#define	SCE_FWLOCK      (0x0020)  /*   Reserved: write locked (non-shared) */
#define	SCE_FAPPEND     (0x0100)  /* append on each write */
#define	SCE_FCREAT      (0x0200)  /* create if nonexistant */
#define	SCE_FTRUNC      (0x0400)  /* truncate to zero length */
#define SCE_EXCL        (0x0800)  /* exclusive create */
#define	SCE_FSCAN       (0x1000)  /*   Reserved: scan type */
#define	SCE_FRCOM       (0x2000)  /*   Reserved: remote command entry */
#define	SCE_FNBUF       (0x4000)  /*   Reserved: no ring buf. and console interrupt */
#define	SCE_FASYNC      (0x8000)  /*   Reserved: asyncronous i/o */
#define SCE_FFDEXCL     (0x01000000)        /* exclusive access */
#define SCE_FPWLOCK     (0x02000000)        /* power control lock */
#define SCE_FGAMEDATA   (0x40000000)

/* Flags for sceIoOpen() */
#define SCE_O_RDONLY    (SCE_FREAD)
#define SCE_O_WRONLY    (SCE_FWRITE)
#define SCE_O_RDWR      (SCE_FREAD|SCE_FWRITE)
#define SCE_O_NBLOCK    (SCE_FNBLOCK) /*   Reserved: Non-Blocking I/O */
#define SCE_O_APPEND    (SCE_FAPPEND) /* append (writes guaranteed at the end) */
#define SCE_O_CREAT     (SCE_FCREAT)  /* open with file create */
#define SCE_O_TRUNC     (SCE_FTRUNC)  /* open with truncation */
#define SCE_O_EXCL	    (SCE_EXCL)	  /* exclusive create */
#define SCE_O_NOBUF     (SCE_FNBUF)	  /*   Reserved: no device buffer and console interrupt */
#define SCE_O_NOWAIT    (SCE_FASYNC)  /*   Reserved: asyncronous i/o */
#define SCE_O_FDEXCL    (SCE_FFDEXCL) /* exclusive access */
#define SCE_O_PWLOCK    (SCE_FPWLOCK) /* power control lock */
#define SCE_O_UNKNOWN0  (0x04000000)
#define SCE_O_FGAMEDATA (SCE_FGAMEDATA)

/** user read/write/execute permission. */
#define SCE_STM_RWXU		00700
/** user read permission. */
#define SCE_STM_RUSR		00400
/** user write permission. */
#define SCE_STM_WUSR		00200
/** user execute permission. */
#define SCE_STM_XUSR		00100

/** group read/write/execute permission. */
#define SCE_STM_RWXG		00070
/** group read permission. */
#define SCE_STM_RGRP		00040
/** group write permission. */
#define SCE_STM_WGRP		00020
/** group execute permission. */
#define SCE_STM_XGRP		00010

/** other read/write/execute permission. */
#define SCE_STM_RWXO		00007
/** other read permission. */
#define SCE_STM_ROTH		00004
/** other write permission. */
#define SCE_STM_WOTH		00002
/** other execute permission. */
#define SCE_STM_XOTH		00001

/** user/group/other - read/write/execute. */
#define SCE_STM_RWXUGO	(SCE_STM_RWXU|SCE_STM_RWXG|SCE_STM_RWXO)
/** user/group/other - read. */
#define SCE_STM_RUGO	(SCE_STM_RUSR|SCE_STM_RGRP|SCE_STM_ROTH)
/** user/group/other - write. */
#define SCE_STM_WUGO	(SCE_STM_WUSR|SCE_STM_WGRP|SCE_STM_WOTH)
/** user/group/other - execute. */
#define SCE_STM_XUGO	(SCE_STM_XUSR|SCE_STM_XGRP|SCE_STM_XOTH)

#define SCE_SEEK_SET    0
#define SCE_SEEK_CUR    1
#define SCE_SEEK_END    2

typedef struct ScePspDateTime {
    u16 year;
    u16 month;
    u16 day;
    u16 hour;
    u16 minute;
    u16 second;
    u32 microsecond;
} ScePspDateTime;

struct SceIoDeviceArg;
typedef struct SceIoDeviceArg SceIoDeviceArg;
struct SceIoIob;
typedef struct SceIoIob SceIoIob;

typedef struct
{
    SceMode st_mode;
    u32 st_attr;
    SceOff st_size;
    ScePspDateTime st_ctime;
    ScePspDateTime st_atime;
    ScePspDateTime st_mtime;
    u32 st_private[6];
} SceIoStat;

typedef struct
{
    SceIoStat d_stat;
    char d_name[256];
    void *d_private;
    int dummy;
} SceIoDirent;

typedef struct
{
    int (*IoInit)(SceIoDeviceArg *dev);
    int (*IoExit)(SceIoDeviceArg *dev);
    int (*IoOpen)(SceIoIob *iob, char *file, int flags, SceMode mode);
    int (*IoClose)(SceIoIob *iob);
    int (*IoRead)(SceIoIob *iob, char *data, int len);
    int (*IoWrite)(SceIoIob *iob, const char *data, int len);
    SceOff (*IoLseek)(SceIoIob *iob, SceOff ofs, int whence);
    int (*IoIoctl)(SceIoIob *iob, unsigned int cmd, void *indata, int inlen, void *outdata, int outlen);
    int (*IoRemove)(SceIoIob *iob, const char *name);
    int (*IoMkdir)(SceIoIob *iob, const char *name, SceMode mode);
    int (*IoRmdir)(SceIoIob *iob, const char *name);
    int (*IoDopen)(SceIoIob *iob, const char *dirname);
    int (*IoDclose)(SceIoIob *iob);
    int (*IoDread)(SceIoIob *iob, SceIoDirent *dir);
    int (*IoGetstat)(SceIoIob *iob, const char *file, SceIoStat *stat);
    int (*IoChstat)(SceIoIob *iob, const char *file, SceIoStat *stat, int bits);
    int (*IoRename)(SceIoIob *iob, const char *oldname, const char *newname);
    int (*IoChdir)(SceIoIob *iob, const char *dir);
    int (*IoMount)(SceIoIob *iob, const char *fs, const char *blockDev, int mode, void *unk1, int unk2);
    int (*IoUmount)(SceIoIob *iob, const char *blockDev);
    int (*IoDevctl)(SceIoIob *iob, const char *devname, unsigned int cmd, void *indata, int inlen, void *outdata, int outlen);
    int (*IoCancel)(SceIoIob *iob);
} SceIoDrvFuncs;

typedef struct
{
    const char *name;
    u32 dev_type;
    u32 unk2;
    const char *name2;
    SceIoDrvFuncs *funcs;
} SceIoDrv;

struct SceIoDeviceArg
{
    SceIoDrv *drv;
    void *argp;
    int openedFiles;
};

struct SceIoHookType;
typedef struct SceIoHookType SceIoHookType;

struct SceIoHook;
typedef struct SceIoHook SceIoHook;

typedef struct
{
    void (*Add)(SceIoHookType **hook);
    int unused4;
    int (*Preobe)(SceIoHook *hook, char *file, int flags, SceMode mode);
    int (*Open)(SceIoHook *hook, char *file, int flags, SceMode mode);
    int (*Close)(SceIoHook *hook);
    int (*Read)(SceIoHook *hook, void *data, SceSize size);
    int (*Write)(SceIoHook *hook, const void *data, SceSize size);
    SceOff (*Lseek)(SceIoHook *hook, SceOff ofs, int whence);
    int (*Ioctl)(SceIoHook *iob, unsigned int cmd, void *indata, int inlen, void *outdata, int outlen);
} SceIoHookFuncs;

struct SceIoHookType
{
    char *name;
    int unk4;
    int unk8;
    char *name2;
    SceIoHookFuncs *funcs;
};

typedef struct
{
    int size; // 0
    char name[32]; // 4
    int attribute; // 36
    int unk40; // 40
    const char *drvName; // 44
    int fsNum; // 48
    char *newPath; // 52
    int retAddr; // 56
    int curThread; // 60
    int asyncThread; // 64
    int isAsync; // 68
    int asyncCmd; // 72
    SceIoIob *iob; // 76
    int unk80; // 80
    int unk84; // 84
} SceIoFdDebugInfo;

typedef struct
{
    SceIoHookType *hook;
    void *argp;
} SceIoHookArg;

struct SceIoHook
{
    SceIoHookArg *arg;
    SceIoIob *iob;
    SceIoDrvFuncs *funcs;
};

struct SceIoIob
{
    int unk000; // some ID
    int fsNum; // 4
    SceIoDeviceArg *dev; // 8
    int dev_type; // 12
    int unk016; // 16
    int unk020; // 20
    int unk024; // 24
    int unk028; // 28
    int unk032; // 32
    int unk036; // 36
    int unk040; // 40
    SceUID curThread; // 44
    char userMode; // 48
    char powerLocked; // 49
    char unk050;
    char asyncPrio; // 51
    SceUID asyncThread; // 52
    SceUID asyncSema; // 56
    SceUID asyncEvFlag; // 60
    SceUID asyncCb; // 64
    void *asyncCbArgp; // 68
    int unused72; // 72
    int k1; // 76
    s64 asyncRet; // 80
    int asyncArgs[6]; // 88
    int asyncCmd; // 112
    int userLevel; // 116
    SceIoHook hook; // 120
    int unk132; // 132
    char *newPath; // 136
    int retAddr; // 140
};

int sceIoChangeAsyncPriority(int fd, int prio);
void sceIoCloseAll();
int sceIoReopen(const char *file, int flags, SceMode mode, int fd);
SceUID sceIoDopen(const char *dirname);
int sceIoDread(int fd, SceIoDirent *dir);
int sceIoDclose(int fd);
int sceIoRemove(const char *file);
int sceIoRename(const char *oldname, const char *newname);
int sceIoDevctl(const char *dev, unsigned int cmd, void *indata, int inlen, void *outdata, int outlen);

/* IO-Assign mount mode flags. */
#define SCE_MT_RDWR	          0x00 /** Mount as read/write enabled. */
#define SCE_MT_RDONLY	      0x01 /** Mount as read-only. */
#define SCE_MT_ROBUST	      0x02 /** Mount in ROBUST mode. */
#define SCE_MT_ERRCHECK       0x04 /** Set an error if there is anythign abnormal in the file system when mounting. */

int sceIoAssign(const char *dev, const char *blockDev, const char *fs, int mode, void* unk1, int unk2);
int sceIoUnassign(const char *dev);
int sceIoChangeThreadCwd(SceUID threadId, const char *path);
int sceIoCancel(int fd);
int sceIoGetFdList(SceUID *fds, int numFd, int *count);
int sceIoGetFdDebugInfo(int fd, SceIoFdDebugInfo *outInfo);
int sceIoAddDrv(SceIoDrv *drv);
int sceIoDelDrv(const char *drv);
int sceIoGetUID(int fd);
int sceIoPollAsync(SceUID fd, SceInt64 *res);
int sceIoWaitAsync(SceUID fd, SceInt64 *res);
int sceIoWaitAsyncCB(SceUID fd, SceInt64 *res);
int sceIoGetAsyncStat(SceUID fd, int poll, SceInt64 *res);
int sceIoSetAsyncCallback(SceUID fd, SceUID cb, void *argp);
int sceIoValidateFd(SceUID fd, int arg1);
int sceIoClose(SceUID fd);
int sceIoCloseAsync(SceUID fd);
SceUID sceIoOpen(const char *file, int flags, SceMode mode);
SceUID sceIoOpenAsync(const char *file, int flags, SceMode mode);
int sceIoRead(SceUID fd, void *data, SceSize size);
int sceIoReadAsync(SceUID fd, void *data, SceSize size);
int sceIoWrite(SceUID fd, const void *data, SceSize size);
int sceIoWriteAsync(SceUID fd, const void *data, SceSize size);
SceOff sceIoLseek(SceUID fd, SceOff offset, int whence);
SceOff sceIoLseekAsync(SceUID fd, SceOff offset, int whence);
int sceIoLseek32(SceUID fd, int offset, int whence);
int sceIoLseek32Async(SceUID fd, int offset, int whence);
int sceIoIoctl(SceUID fd, unsigned int cmd, void *indata, int inlen, void *outdata, int outlen);
int sceIoIoctlAsync(SceUID fd, unsigned int cmd, void *indata, int inlen, void *outdata, int outlen);
int sceIoMkdir(const char *path, SceMode mode);
int sceIoRmdir(const char *path);
int sceIoChdir(const char *path);
int sceIoGetstat(const char *file, SceIoStat *stat);
int sceIoChstat(const char *file, SceIoStat *stat, int bits);
int sceIoSync(const char *device, unsigned int unk);
int sceIoGetDevType(SceUID fd);
int sceIoGetThreadCwd(SceUID uid, char *dir, int len);
int sceIoTerminateFd(char *drive);
int sceIoAddHook(SceIoHookType *hook);
int sceIoGetIobUserLevel(SceIoIob *iob);

