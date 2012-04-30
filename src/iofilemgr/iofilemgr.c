#include "../common/common.h"

#include "../sysmem/sysclib.h"

#include "iofilemgr.h"
#include "stdio.h"

PSP_MODULE_BOOTSTART("IoFileMgrInit");
PSP_MODULE_REBOOT_BEFORE("IoFileMgrRebootBefore");
PSP_MODULE_INFO("sceIOFileManager", 0x1007, 1, 7);
PSP_SDK_VERSION(0x06060010);

typedef struct SceIoDeviceList
{
    struct SceIoDeviceList *next; // 0
    SceIoDeviceArg arg;
} SceIoDeviceList;

typedef struct SceIoAlias
{
    struct SceIoAlias *next; // 0
    SceIoDeviceArg *dev; // 4
    int attr; // 8
    int fsNum; // 12
    char alias[32]; // 16
    char blockDev[32]; // 48
    char fs[32]; // 80
} SceIoAlias; // size: 112

typedef struct SceIoHookList
{
    struct SceIoHookList *next;
    SceIoHookArg arg;
} SceIoHookList;

int deleted_func();
int deleted_func_close();
s64 deleted_func_offt();

// 69B8
SceIoDrvFuncs deleted_dt_func =
{
    deleted_func,
    deleted_func,
    deleted_func,
    deleted_func_close,
    deleted_func,
    deleted_func,
    deleted_func_offt,
    deleted_func,
    deleted_func,
    deleted_func,
    deleted_func,
    deleted_func,
    deleted_func_close,
    deleted_func,
    deleted_func,
    deleted_func,
    deleted_func,
    deleted_func,
    deleted_func,
    deleted_func,
    deleted_func,
    deleted_func
};

// 6A10
SceIoDrv deleted_devtable = { "", 0, 0, "", &deleted_dt_func };

int _nulldev();
s64 _nulldev_offt();
int _nulldev_write(SceIoIob *iob __attribute__((unused)), const char *data, int len);

// 6A24
SceIoDrvFuncs _nullcon_function =
{
    _nulldev,
    _nulldev,
    _nulldev,
    _nulldev,
    _nulldev,
    _nulldev_write,
    _nulldev_offt,
    _nulldev,
    _nulldev,
    _nulldev,
    _nulldev,
    _nulldev,
    _nulldev,
    _nulldev,
    _nulldev,
    _nulldev,
    _nulldev,
    _nulldev,
    _nulldev,
    _nulldev,
    _nulldev,
    _nulldev
};

// 6A7C
SceIoDrv _dummycon_driver = { "dummy_drv_iofile", 0, 0x00000800, "DUMMY_DRV", &_nullcon_function };

int iob_do_initialize(SceSysmemUIDControlBlock *cb, int funcid, void *arg1, void *arg2, void *arg3, void *arg4, void *arg5, void *arg6);
int iob_do_delete(SceSysmemUIDControlBlock *cb, int funcid, void *arg1, void *arg2, void *arg3, void *arg4, void *arg5, void *arg6);

// 6A90
SceSysmemUIDLookupFunc IobFuncs[] =
{
    { 0xD310D2D9, iob_do_initialize },
    { 0x87089863, iob_do_delete },
    { 0, NULL }
};

// 6ADC
int default_thread_priority = -1;

// 6AE0
int g_deleted_error = 0x80020328; // exported as E4D75BC0

// 6AE4
SceIoDeviceArg deleted_device = { &deleted_devtable, NULL, 0 };

// 6B0C
SceIoDeviceList *g_devList;

// 6B10
SceIoAlias *g_aliasList;

// 6B14
SceIoHookList *g_hookList;

// 6B18
int g_pathbufCount;

// 6B1C
SceUID g_heap;

// 6B20
SceUID g_ktls;

// 6B24
int g_iobCount;

// 6B28
SceSysmemUIDControlBlock *g_uid_type;

// 6B2C
SceUID g_UIDs[64];

// 6C2C
char *g_pathbufBuf[2];

int validate_fd(int fd, int arg1, int arg2, int arg3, SceIoIob **outIob);
int alloc_iob(SceIoIob **outIob, int arg1);
int init_iob(SceIoIob *iob, int devType, SceIoDeviceArg *dev, int unk, int fsNum);
int free_iob(SceIoIob *iob);
void *alloc_pathbuf();
void free_pathbuf(void *ptr);
char *parsedev(const char *dev, char *outDev, int *fsNum);
SceIoAlias *sub_35D0(const char *dev);
SceIoAlias *sub_36C4(const char *name);
int sub_375C(const char *path, SceIoDeviceArg **dev, int *fsNum, char **dirNamePtr);
int sub_3778(const char *path, SceIoDeviceArg **dev, int *fsNum, char **dirNamePtr, int userMode);
int strcmp_bs(const char *s1, const char *s2);
int open_main(SceIoIob *iob);
int create_async_thread(SceIoIob *iob);
int iob_power_lock(SceIoIob *iob);
int iob_power_unlock(SceIoIob *iob);
int preobe_fdhook(SceIoIob *iob, char *file, int flags, SceMode mode);
int do_get_async_stat(SceUID fd, SceInt64 *res, int poll, int cb, char *func);
int do_close(SceUID fd, int async, int remove);
int do_open(const char *path, int flags, SceMode mode, int async, int retAddr, int oldK1);
int do_read(SceUID fd, void *data, SceSize size, int async);
int do_write(SceUID fd, const void *data, SceSize size, int async);
SceOff do_lseek(SceUID fd, SceOff offset, int whence, int async);
int do_ioctl(SceUID fd, unsigned int cmd, void *indata, int inlen, void *outdata, int outlen, int async);
int xx_dir(const char *path, SceMode mode, int action);
int xx_stat(const char *file, SceIoStat *stat, int bits, int get);
int do_devctl(const char *dev, unsigned int cmd, void *indata, int inlen, void *outdata, int outlen);
int do_deldrv(SceIoDeviceArg *dev);
int sceIoGetUID(int fd);
void add_device_list(SceIoDeviceList *list);
int delete_device_list(SceIoDeviceList *list);
SceIoDeviceList *alloc_device_list(void);
void free_device_list(SceIoDeviceList *list);
SceIoAlias *alloc_alias_tbl(void);
void free_alias_tbl(SceIoAlias *alias);
void add_alias_tbl(SceIoAlias *alias);
int delete_alias_tbl(SceIoAlias *alias);
SceIoAlias *lookup_alias_tbl(char *drive);
SceIoDeviceList *lookup_device_list(const char *drive);
void free_cwd(void *ktls);
int async_loop(SceSize args, void *argp);

int sceIoChangeAsyncPriority(int fd, int prio)
{
    dbg_printf("Calling %s\n", __FUNCTION__);
    SceIoIob *iob;
    if (prio != -1 && (prio <= 0 || prio >= 127))
        return 0x80020193;
    int oldK1 = pspShiftK1();
    if (pspK1IsUserMode() && prio != -1 && (prio <= 7 || prio >= 120))
    {
        // 0F34
        pspSetK1(oldK1);
        return 0x80020193;
    }
    // 0E8C
    if (fd == -1)
    {
        // 0F20
        pspSetK1(oldK1);
        default_thread_priority = prio;
        return 0;
    }
    int ret = validate_fd(fd, 0, 2, 1, &iob);
    if (ret < 0) {
        pspSetK1(oldK1);
        return ret;
    }
    iob->asyncPrio = prio;
    if (iob->asyncThread == 0) {
        pspSetK1(oldK1);
        return 0;
    }
    if (prio < 0) {
        // 0F04
        prio = sceKernelGetThreadCurrentPriority();
    }
    // 0ECC
    ret = sceKernelChangeThreadPriority(iob->asyncThread, prio);
    if (ret != 0) {
        pspSetK1(oldK1);
        return ret;
    }
    pspSetK1(oldK1);
    return 0;
}

void sceIoCloseAll()
{
    dbg_printf("Calling %s\n", __FUNCTION__);
    SceIoIob *iob;
    int oldK1 = pspShiftK1();
    SceSysmemUIDControlBlock *cur = g_uid_type->parent;
    // 0F74
    while (cur != g_uid_type)
    {
        int ret = validate_fd(cur->UID, 0, 16, 14, &iob);
        if (ret >= 0 && (iob->unk000 & 8) == 0)
        {
            // 0FD4
            if (iob->hook.arg != NULL) {
                // 1020
                iob->hook.arg->hook->funcs->Close(&iob->hook);
            }
            else if (iob->dev != &deleted_device && iob->dev->drv->funcs->IoClose != NULL) {
                // 1010
                iob->dev->drv->funcs->IoClose(iob);
            }

            // 1000
            free_iob(iob);
        }
        cur = g_uid_type->parent;
        // 0FAC
    }

    // 0FB4
    pspSetK1(oldK1);
}

int open_iob(SceIoIob *iob, const char *path, int flags, SceMode mode, int async)
{
    dbg_printf("Calling %s\n", __FUNCTION__);
    int ret;
    SceIoDeviceArg *dev;
    int fsNum;
    char *realPath;
    char *pathbuf = alloc_pathbuf();
    if (pathbuf == NULL)
        return 0x80020190;

    if ((flags & 0x04000000) != 0)
        iob->userLevel = 8;

    if ((flags & 0x04000000) != 0 && iob->userMode != 0) {
        ret = 0x800200D1;
        goto error;
    }

    // 10A8
    realPath = pathbuf;
    ret = sub_3778(path, &dev, &fsNum, &realPath, ((flags >> 26) ^ 1) & 1);
    if (ret < 0)
        goto error;

    ret = init_iob(iob, ret, dev, flags, fsNum);
    if (ret < 0)
        goto error;

    if (dev->drv->funcs->IoOpen == NULL) {
        ret = 0x80020325;
        goto error;
    }

    iob->asyncArgs[2] = flags;
    iob->asyncArgs[1] = (int)realPath;
    iob->asyncArgs[3] = mode;
    iob->asyncArgs[0] = 0;
    if (async == 0) {
        // 11F8
        ret = open_main(iob);
    }
    else
    {
        iob->asyncArgs[0] = (int)pathbuf;
        if (iob->asyncThread != 0)
        {
            ret = create_async_thread(iob);
            if (ret < 0)
                goto error;

            // 1144
            ret = sceKernelPollSema(iob->asyncSema, 1);
            if (ret < 0) {
                ret = 0x80020329;
                goto error;
            }

            if (iob->asyncPrio < 0 && sceKernelGetCompiledSdkVersion() >= 0x04020000)
                sceKernelChangeThreadPriority(iob->asyncThread, 0);
            // 1168
            iob->asyncCmd = 1;
            iob->k1 = pspGetK1();
            ret = sceKernelSetEventFlag(iob->asyncEvFlag, 3);
            // 1180
            if (ret < 0)
                goto error;
            ret = iob->unk040;
            pathbuf = NULL;
        }
    }

    error:
    if (pathbuf != NULL)
        free_pathbuf(pathbuf);
    return ret;
}

int open_main(SceIoIob *iob)
{
    dbg_printf("Calling %s\n", __FUNCTION__);
    if ((iob->unk000 & 0x02000000) != 0) {
        // 12D0
        iob_power_lock(iob);
    }
    // 1230
    int ret;
    if (preobe_fdhook(iob, (char*)iob->asyncArgs[1], iob->asyncArgs[2], iob->asyncArgs[3]) == 0) {
        // 12AC
        ret = iob->dev->drv->funcs->IoOpen(iob, (char*)iob->asyncArgs[1], iob->asyncArgs[2], iob->asyncArgs[3]);
    }
    else
        ret = iob->hook.arg->hook->funcs->Open(&iob->hook, (char*)iob->asyncArgs[1], iob->asyncArgs[2], iob->asyncArgs[3]);
    // 1264
    if (ret >= 0)
        ret = iob->unk040;
    // 1278
    if (iob->asyncArgs[0] != 0)
    {
        // 129C
        free_pathbuf((char*)iob->asyncArgs);
        iob->asyncArgs[0] = 0;
    }
    return ret;
}

int sceIoReopen(const char *file, int flags, SceMode mode, int fd)
{
    dbg_printf("Calling %s\n", __FUNCTION__);
    SceIoIob *iob;
    int ra = pspGetRa();
    int oldK1 = pspShiftK1();
    if (!pspK1PtrOk(file))
    {
        // 1434
        pspSetK1(oldK1);
        return 0x800200D3;
    }
    int ret = do_close(fd, 0, 0);
    if (ret < 0)
    {
        // 142C
        pspSetK1(oldK1);
        return ret;
    }
    ret = validate_fd(fd, 0, 2, 3, &iob);
    if (ret < 0) {
        pspSetK1(oldK1);
        return ret;
    }
    iob_power_unlock(iob);
    ret = open_iob(iob, file, flags, mode, 0);
    if (ret < 0)
    {
        // 141C
        free_iob(iob);
        pspSetK1(oldK1);
        return ret;
    }
    if (sceKernelDeci2pReferOperations() != 0)
    {
        if (iob->newPath != NULL) {
            // 140C
            free_pathbuf(iob->newPath);
        }
        // 13A0
        char *path = alloc_pathbuf();
        if (path != NULL) {
            strncpy(path, file, 1023);
            path[1023] = '\0';
        }
        // 13C4
        iob->newPath = path;
    }
    // 13CC
    iob->retAddr = ra;
    iob->curThread = sceKernelGetThreadId();
    pspSetK1(oldK1);
    return ret;
}

SceUID sceIoDopen(const char *dirname)
{
    dbg_printf("Calling %s\n", __FUNCTION__);
    SceIoIob *iob;
    SceIoDeviceArg *dev;
    int fsNum;
    char *dirName;
    int ra = pspGetRa();
    int oldK1 = pspShiftK1();
    if (!pspK1PtrOk(dirname))
    {
        // 15A8
        pspSetK1(oldK1);
        return 0x800200D3;
    }
    char *path = alloc_pathbuf();
    if (path == NULL)
    {
        // 1598
        pspSetK1(oldK1);
        return 0x80020190;
    }
    int ret = alloc_iob(&iob, 1);
    if (ret < 0)
        goto freepath;

    sceKernelRenameUID(sceIoGetUID(ret), dirname);
    if (pspK1IsUserMode()) {
        // 1584
        iob->retAddr = InterruptManagerForKernel_A0F88036();
    }
    else
        iob->retAddr = ra;
    // 14C4
    dirName = path;
    ret = sub_375C(dirname, &dev, &fsNum, &dirName);
    if (ret < 0)
        goto freeiob;

    ret = init_iob(iob, ret, dev, 0x01000008, fsNum);
    if (ret < 0)
        goto freeiob;

    if (dev->drv->funcs->IoDopen == NULL) {
        ret = 0x80020325;
        goto freeiob;
    }

    // 1564
    ret = dev->drv->funcs->IoDopen(iob, dirName);
    if (ret < 0)
        goto freeiob;

    ret = iob->unk040;
    goto freepath;

    freeiob:
    free_iob(iob);

    freepath:
    // 152C
    if (path != NULL)
        free_pathbuf(path);
    pspSetK1(oldK1);
    return ret;
}

int sceIoDread(int fd, SceIoDirent *dir)
{
    dbg_printf("Calling %s\n", __FUNCTION__);
    SceIoIob *iob;
    int oldK1 = pspShiftK1();
    if (!pspK1PtrOk(dir)) {
        pspSetK1(oldK1);
        return 0x800200D3;
    }
    int ret = validate_fd(fd, 8, 4, 0, &iob);
    if (ret < 0) {
        pspSetK1(oldK1);
        return ret;
    }
    if (iob->dev->drv->funcs->IoDread == NULL) {
        pspSetK1(oldK1);
        return 0x80020325;
    }
    // 1650
    ret = iob->dev->drv->funcs->IoDread(iob, dir);
    pspSetK1(oldK1);
    return ret;
}

int sceIoDclose(int fd)
{
    dbg_printf("Calling %s\n", __FUNCTION__);
    SceIoIob *iob;
    int oldK1 = pspShiftK1();
    int ret = validate_fd(fd, 8, 4, 2, &iob);
    if (ret < 0)
    {
        // 1704
        pspSetK1(oldK1);
        return ret;
    }
    ret = 0x80020325;
    if (iob->dev == &deleted_device)
    {
        // 16F0
        free_iob(iob);
        pspSetK1(oldK1);
        return 0;
    }
    if (iob->dev->drv->funcs->IoDclose != NULL)
    {
        // 16DC
        ret = iob->dev->drv->funcs->IoDclose(iob);
        if (ret >= 0)
        {
            // 16F0
            free_iob(iob);
            pspSetK1(oldK1);
            return 0;
        }
    }
    // 16C4
    pspSetK1(oldK1);
    return ret;
}

int sceIoRemove(const char *file)
{
    dbg_printf("Calling %s\n", __FUNCTION__);
    SceIoIob *iob;
    SceIoDeviceArg *dev;
    int fsNum;
    char *realPath;
    int ret;
    int oldK1 = pspShiftK1();
    if (!pspK1PtrOk(file))
    {
        // 181C
        pspSetK1(oldK1);
        return 0x800200D3;
    }
    char *path = alloc_pathbuf();
    if (path == NULL)
    {
        // 180C
        pspSetK1(oldK1);
        return 0x80020190;
    }
    ret = alloc_iob(&iob, 0);
    if (ret < 0)
        goto freepath;

    realPath = path;
    ret = sub_375C(file, &dev, &fsNum, &realPath);
    if (ret < 0)
        goto freeiob;

    ret = init_iob(iob, ret, dev, 0x1000000, fsNum);
    if (ret < 0)
        goto freeiob;

    if (dev->drv->funcs->IoRemove == NULL) {
        ret = 0x80020325;
        goto freeiob;
    }

    // 17F8
    ret = dev->drv->funcs->IoRemove(iob, realPath);

    freeiob:
    // 17BC
    free_iob(iob);

    freepath:
    // 17C4
    if (path != NULL)
        free_pathbuf(path);
    pspSetK1(oldK1);
    return ret;
}

int sceIoRename(const char *oldname, const char *newname)
{
    dbg_printf("Calling %s\n", __FUNCTION__);
    int oldK1 = pspShiftK1();
    if (!pspK1PtrOk(oldname) || !pspK1PtrOk(newname))
    {
        // 1A08
        pspSetK1(oldK1);
        return 0x800200D3;
    }
    char *buf1 = alloc_pathbuf();
    if (buf1 == NULL)
    {
        // 19FC
        pspSetK1(oldK1);
        return 0x80020190;
    }
    char *buf2 = alloc_pathbuf();
    if (buf2 == NULL)
    {
        // 19E4
        free_pathbuf(buf1);
        pspSetK1(oldK1);
        return 0x80020190;
    }
    SceIoIob *iob;
    int ret = alloc_iob(&iob, 0);
    if (ret < 0)
        goto freepath;
    SceIoDeviceArg *olddev;
    int oldfsNum;
    char *oldrealPath = buf1;
    ret = sub_375C(oldname, &olddev, &oldfsNum, &oldrealPath);
    if (ret < 0)
        goto freeiob;
    if (strchr(newname, ':') == 0)
    {
        // 19BC
        if (strlen(newname) >= 1023)
        {
            // 19D8
            ret = 0x8002032D;
            goto freeiob;
        }
    }
    else
    {
        SceIoDeviceArg *newdev;
        int newfsNum;
        char *newrealPath = buf2;
        ret = sub_375C(newname, &newdev, &newfsNum, &newrealPath);
        if (ret < 0) {
            // 19B4
            goto freeiob;
        }
        if (newdev != olddev || newfsNum != oldfsNum) {
            ret = 0x80020322;
            goto freeiob;
        }
    }

    // 1970
    ret = init_iob(iob, ret, olddev, 0x1000000, oldfsNum);
    if (ret < 0)
        goto freeiob;

    ret = 0x80020325;
    if (olddev->drv->funcs->IoRename != NULL)
        ret = olddev->drv->funcs->IoRename(iob, oldrealPath, newname);

    freeiob:
    free_iob(iob);

    freepath:
    if (buf1 != NULL)
        free_pathbuf(buf1);
    // 1934
    if (buf2 != NULL)
        free_pathbuf(buf2);
    pspSetK1(oldK1);
    return ret;
}

int sceIoDevctl(const char *dev, unsigned int cmd, void *indata, int inlen, void *outdata, int outlen)
{
    dbg_printf("Calling %s\n", __FUNCTION__);
    int oldK1 = pspShiftK1();
    if (!pspK1PtrOk(dev) || !pspK1DynBufOk(indata, inlen) || !pspK1DynBufOk(outdata, outlen))
    {
        // 1A9C
        pspSetK1(oldK1);
        return 0x800200D3;
    }
    if (pspK1IsUserMode() && ((cmd >> 15) & 1) != 0)
    {
        // 1A90
        pspSetK1(oldK1);
        return 0x800200D1;
    }
    // 1A74
    int ret = do_devctl(dev, cmd, indata, inlen, outdata, outlen);
    pspSetK1(oldK1);
    return ret;
}

int sceIoAssign(const char *dev, const char *blockDev, const char *fs, int mode, void* unk1, int unk2)
{
    dbg_printf("Calling %s\n", __FUNCTION__);
    char outDev[32];
    char blkAliasName[32];
    char fsAliasName[32];
    int fsNum;
    SceIoIob *iob;
    int oldK1 = pspShiftK1();
    if (!pspK1PtrOk(dev) || !pspK1PtrOk(blockDev) || !pspK1PtrOk(fs) || !pspK1DynBufOk(unk1, unk2))
    {
        // 1FE8
        pspSetK1(oldK1);
        return 0x800200D3;
    }
    if (dev == NULL || (blockDev == NULL && fs != NULL))
    {
        // 1F44
        pspSetK1(oldK1);
        return 0x80020321;
    }
    if (blockDev == NULL && fs == NULL)
        blockDev = "dummy_drv_iofile:";
    if (sub_35D0(dev) != NULL)
    {
        // 1FD8
        pspSetK1(oldK1);
        return 0x80020326;
    }
    if (fs != NULL && (~mode >> 31) != 0)
    {
        // 1F98
        parsedev(fs, outDev, &fsNum);
        SceIoAlias *alias = lookup_alias_tbl(outDev);
        if (alias != NULL) {
            snprintf(fsAliasName, 32, "%s%d:", alias->blockDev, alias->fsNum);
            fs = fsAliasName;
        }
    }
    // 1B78
    SceIoAlias *blkAlias = NULL;
    if (blockDev != NULL && (~mode >> 31) != 0)
    {
        // 1F50
        parsedev(blockDev, outDev, &fsNum);
        blkAlias = lookup_alias_tbl(outDev);
        if (blkAlias != NULL) {
            snprintf(blkAliasName, 32, "%s%d:", blkAlias->blockDev, blkAlias->fsNum);
            blockDev = blkAliasName;
        }
    }
    // 1B8C
    const char *curDev = blockDev;
    int attr = 0x20;
    int fsset = 0;
    if (fs != NULL)
    {
        fsset = 1;
        curDev = fs;
        attr = 0x10;
    }
    // 1BAC
    parsedev(curDev, outDev, &fsNum);
    SceIoDeviceList *list = lookup_device_list(outDev);
    if (list == NULL)
    {
        // 1F44
        pspSetK1(oldK1);
        return 0x80020321;
    }
    if (blkAlias != NULL)
        attr |= 0x100;
    attr |= (((list->arg.drv->dev_type >> 24) & 0xF) << 24) | (((list->arg.drv->dev_type >> 16) & 0xFF) << 16);
    SceIoAlias *newAlias = alloc_alias_tbl();
    if (newAlias == NULL)
    {
        // 1F34
        pspSetK1(oldK1);
        return 0x80020320;
    }
    newAlias->attr = attr;
    newAlias->dev = &list->arg;
    newAlias->fsNum = fsNum;
    // 1F18, 1F20
    while (*dev == ' ')
        dev++;
    // 1C2C
    char *colon = strchr(dev, ':');
    if (colon == NULL)
        goto err_invalid_device;
    if (colon - dev >= 31)
        goto err_invalid_device;
    // 1C8C
    strncpy(newAlias->alias, dev, colon - dev);
    newAlias->alias[colon - dev] = '\0';
    strncpy(newAlias->blockDev, curDev, 31);
    newAlias->blockDev[31] = '\0';
    // 1EFC, 1F04
    while (*blockDev == ' ')
        blockDev++;
    // 1CCC
    colon = strchr(blockDev, ':');
    if (colon == NULL)
        goto err_invalid_device;
    if (colon - blockDev >= 31)
        goto err_invalid_device;

    // 1CEC
    do
    {
        if (colon == blockDev)
            break;
    } while ((look_ctype_table(*(colon--)) & 4) == 0);
    // 1D0C
    strncpy(newAlias->fs, blockDev, colon - blockDev);
    newAlias->fs[colon - blockDev] = '\0';
    if (blkAlias == NULL)
    {
        // 1DE4
        if (strcmp_bs("disc0", newAlias->alias) != 0)
        {
            // 1E24
            if (strcmp_bs("ms0", newAlias->alias) != 0)
            {
                // 1E80
                if (strcmp_bs("flash0", newAlias->alias) == 0
                 || strcmp_bs("flash1", newAlias->alias) == 0
                 || strcmp_bs("flash2", newAlias->alias) == 0
                 || strcmp_bs("flash3", newAlias->alias) == 0) // 1EB0
                {
                    // 1E9C
                    if ((newAlias->attr & 0xFF) != 0x10)
                        goto err_invalid_device;
                    // 1E0C dup
                    if (strcmp_bs("lflash", newAlias->fs) != 0)
                        goto err_invalid_device;
                }
            }
            else
            {
                if ((newAlias->attr & 0xFF) != 0x10)
                    goto err_invalid_device;
                if (strcmp_bs("msstor0p", newAlias->fs) != 0 && strcmp_bs("msstor", newAlias->fs) != 0 && strcmp_bs("eflash0a0f0p", newAlias->fs) != 0)
                    goto err_invalid_device;
            }
        }
        else
        {
            if ((newAlias->attr & 0xFF) != 0x10)
                goto err_invalid_device;
            // 1E0C dup
            if (strcmp_bs("emu", newAlias->fs) != 0)
                goto err_invalid_device;
        }
    }
    // 1D2C
    int ret = alloc_iob(&iob, 0);
    if (ret < 0)
        goto err;
    ret = init_iob(iob, attr, &list->arg, 0x1000000, fsNum);
    if (ret < 0)
        goto err_free_iob;
    if (fsset)
    {
        ret = 0x80020325;
        if (list->arg.drv->funcs->IoMount == NULL)
            goto err_free_iob;
        // 1DA4
        ret = list->arg.drv->funcs->IoMount(iob, fs, blockDev, mode, unk1, unk2);
        if (ret < 0)
            goto err_free_iob;
    }
    // 1DC8
    add_alias_tbl(newAlias);
    free_iob(iob);
    pspSetK1(oldK1);
    return 0;

    err_free_iob:
    free_iob(iob);

    err:
    free_alias_tbl(newAlias);
    pspSetK1(oldK1);
    return ret;

    err_invalid_device:
    free_alias_tbl(newAlias);
    pspSetK1(oldK1);
    return 0x80020321;
}

int sceIoUnassign(const char *dev)
{
    dbg_printf("Calling %s\n", __FUNCTION__);
    SceIoIob *iob;
    int oldK1 = pspShiftK1();
    if (!pspK1PtrOk(dev))
    {
        // 2138
        pspSetK1(oldK1);
        return 0x800200D3;
    }
    if (dev == NULL)
    {
        // 2130
        pspSetK1(oldK1);
        return 0x80020321;
    }
    SceIoAlias *alias = sub_35D0(dev);
    if (alias == NULL)
    {
        // 212C
        pspSetK1(oldK1);
        return 0x80020321;
    }
    int ret = alloc_iob(&iob, 0);
    if (ret < 0)
    {
        // 2124
        pspSetK1(oldK1);
        return ret;
    }
    ret = init_iob(iob, alias->attr, alias->dev, 0x1000000, alias->fsNum);
    if (ret < 0)
        goto error;
    if ((alias->attr & 0xFF) == 0x10)
    {
        // 20DC
        ret = 0x80020325;
        if (alias->dev->drv->funcs->IoUmount == 0)
            goto error;
        // 2108
        ret = alias->dev->drv->funcs->IoUmount(iob, alias->blockDev);
        if (ret < 0)
            goto error;
    }
    // 2080
    if (delete_alias_tbl(alias) != 0)
    {
        // 20C4
        free_iob(iob);
        pspSetK1(oldK1);
        return 0x80020321;
    }
    free_alias_tbl(alias);
    free_iob(iob);
    pspSetK1(oldK1);
    return 0;

    error:
    free_iob(iob);
    pspSetK1(oldK1);
    return ret;
}

int IoFileMgrForKernel_E5323C5B(const char *aliasName, const char *blockDev)
{
    dbg_printf("Calling %s\n", __FUNCTION__);
    int oldK1 = pspShiftK1();
    int fsNum;
    char name[32];
    if (!pspK1PtrOk(aliasName) || !pspK1PtrOk(blockDev))
    {
        // 22C0
        pspSetK1(oldK1);
        return 0x800200D3;
    }
    if (aliasName == NULL)
    {
        // 22A4
        pspSetK1(oldK1);
        return 0x80020321;
    }
    if (sub_36C4(aliasName) != NULL)
    {
        // 22AC
        pspSetK1(oldK1);
        return 0x80020326;
    }
    parsedev(blockDev, name, &fsNum);
    if (lookup_device_list(name) == NULL)
    {
        // 22A4
        pspSetK1(oldK1);
        return 0x80020321;
    }
    SceIoAlias *alias = alloc_alias_tbl();
    if (alias == NULL)
    {
        // 2294
        pspSetK1(oldK1);
        return 0x80020320;
    }
    alias->attr = 0x100;
    alias->dev = NULL;
    alias->fsNum = fsNum;
    // 2278, 2280
    while (*aliasName == ' ')
        aliasName++;
    // 21F0
    char *colon = strchr(aliasName, ':');
    int devLen = colon - aliasName;
    if (colon == NULL || devLen >= 31)
    {
        // 220C
        free_alias_tbl(alias);
        pspSetK1(oldK1);
        return 0x80020321;
    }
    // 223C
    strncpy(alias->alias, aliasName, devLen);
    alias->alias[devLen] = '\0';
    strncpy(alias->blockDev, name, 31);
    alias->blockDev[31] = '\0';
    add_alias_tbl(alias);
    pspSetK1(oldK1);
    return 0;
}

int IoFileMgrForKernel_E972F70B(const char *name)
{
    dbg_printf("Calling %s\n", __FUNCTION__);
    int oldK1 = pspShiftK1();
    if (!pspK1PtrOk(name)) {
        pspSetK1(oldK1);
        return 0x800200D3;
    }
    if (name == NULL) {
        pspSetK1(oldK1);
        return 0x80020321;
    }
    SceIoAlias *alias = sub_36C4(name);
    if (alias == NULL) {
        pspSetK1(oldK1);
        return 0x80020321;
    }
    if (delete_alias_tbl(alias) != 0) {
        pspSetK1(oldK1);
        return 0x80020321;
    }
    free_alias_tbl(alias);
    pspSetK1(oldK1);
    return 0;
}

int sceIoChangeThreadCwd(SceUID threadId, const char *path)
{
    dbg_printf("Calling %s\n", __FUNCTION__);
    SceIoDeviceArg *dev;
    int fsNum;
    char *realPath;
    int oldK1 = pspShiftK1();
    if (!pspK1PtrOk(path))
    {
        // 2494
        pspSetK1(oldK1);
        return 0x800200D3;
    }
    char *buf = alloc_pathbuf();
    if (buf == NULL)
    {
        // 2484
        pspSetK1(oldK1);
        return 0x80020190;
    }
    realPath = buf;
    int ret = sub_375C(path, &dev, &fsNum, &realPath);
    if (ret < 0)
        goto end;
    void *ktls = sceKernelGetThreadKTLS(g_ktls, threadId, 1);
    if (ktls == NULL) {
        ret = 0x80020190;
        goto end;
    }
    if (*(char**)(ktls + 0) != NULL) {
        // 2474
        sceKernelFreeHeapMemory(g_heap, *(char**)(ktls + 0));
    }
    // 2404
    int len = strlen(realPath) + 1;
    char *heap = sceKernelAllocHeapMemory(g_heap, len);
    *(char**)(ktls + 0) = heap;
    if (heap == NULL) {
        ret = 0x80020190;
        goto end;
    }
    memcpy(heap, realPath, len);
    ret = 0;

    end:
    if (buf != NULL)
        free_pathbuf(buf);
    pspSetK1(oldK1);
    return ret;
}

int sceIoCancel(int fd)
{
    dbg_printf("Calling %s\n", __FUNCTION__);
    SceIoIob *iob;
    int oldK1 = pspShiftK1();
    int ret = validate_fd(fd, 0, 8, 1, &iob);
    if (ret < 0)
    {
        // 2588
        pspSetK1(oldK1);
        return ret;
    }
    if (iob->dev->drv->funcs->IoCancel == NULL)
    {
        pspSetK1(oldK1);
        return 0x80020325;
    }
    ret = iob->dev->drv->funcs->IoCancel(iob);
    if (ret < 0)
    {
        // 2588
        pspSetK1(oldK1);
        return ret;
    }
    int oldIntr = sceKernelCpuSuspendIntr();
    if (iob->asyncThread != 0)
    {
        // 2554
        sceKernelTerminateDeleteThread(iob->asyncThread);
        sceKernelDeleteSema(iob->asyncSema);
        sceKernelDeleteEventFlag(iob->asyncEvFlag);
        iob->asyncThread = 0;
        iob->asyncEvFlag = 0;
        iob->asyncSema = 0;
    }
    // 252C
    sceKernelCpuResumeIntr(oldIntr);
    pspSetK1(oldK1);
    return 0;
}

int sceIoGetFdList(SceUID *fds, int numFd, int *count)
{
    dbg_printf("Calling %s\n", __FUNCTION__);
    int oldK1 = pspShiftK1();
    if (!pspK1DynBufOk(fds, numFd * 4) || !pspK1StaBufOk(count, 4))
    {
        // 25F0
        pspSetK1(oldK1);
        return 0x800200D3;
    }
    // 261C
    int stored = 0;
    int oldIntr = sceKernelCpuSuspendIntr();
    SceSysmemUIDControlBlock *cur = g_uid_type->parent;
    int curCount = 0;
    // 2648
    while (cur != g_uid_type)
    {
        u8 perm = cur->attribute;
        if (!pspK1IsUserMode())
            perm = 0xFF;
        if (perm != 0)
        {
            // 2680
            curCount++;
            if (stored < numFd) {
                stored++;
                *(fds++) = ((SceIoIob*)((void*)cur + g_uid_type->size * 4))->unk040;
            }
        }
        // 2658
        cur = cur->parent;
    }
    // 2664
    if (count != NULL)
        *count = curCount;
    // 266C
    sceKernelCpuResumeIntr(oldIntr);
    pspSetK1(oldK1);
    return stored;
}

int sceIoGetFdDebugInfo(int fd, SceIoFdDebugInfo *outInfo)
{
    dbg_printf("Calling %s\n", __FUNCTION__);
    SceIoFdDebugInfo info;
    SceIoIob *iob;
    int oldK1 = pspShiftK1();
    if (!pspK1StaBufOk(outInfo, 88))
    {
        // 2864
        pspSetK1(oldK1);
        return 0x800200D3;
    }
    int oldIntr = sceKernelCpuSuspendIntr();
    int ret = validate_fd(fd, 0, 0xFF, 31, &iob);
    if (ret < 0)
    {
        // 2850
        sceKernelCpuResumeIntr(oldIntr);
        pspSetK1(oldK1);
        return ret;
    }
    // 2718
    int i;
    for (i = 0; i < 88; i++)
        ((int*)&info)[i] = 0;
    info.size = 88;
    char *name = ((SceSysmemUIDControlBlock*)((void*)iob - g_uid_type->size * 4))->name;
    if (name != NULL) {
        // 2840
        strncpy(info.name, name, 31);
    }
    // 2750
    info.attribute = ((SceSysmemUIDControlBlock*)((void*)iob - g_uid_type->size * 4))->attribute;
    info.unk40 = iob->unk000;
    if (iob->dev != NULL)
        info.drvName = iob->dev->drv->name;
    // 2774
    info.fsNum = iob->fsNum;
    int isAsync = 0;
    info.newPath = iob->newPath;
    info.curThread = iob->curThread;
    info.retAddr = iob->retAddr;
    if (iob->asyncThread != 0)
    {
        // 2814
        if (sceKernelPollEventFlag(iob->asyncEvFlag, 1, 1, 0) == 0)
        {
            // 2838
            isAsync = 1;
        }
        else
            isAsync = 0;
    }
    // 27A0
    info.isAsync = isAsync;
    if (isAsync != 0) {
        info.asyncThread = iob->asyncThread;
        info.asyncCmd = iob->asyncCmd;
    }
    // 27B8
    int size = outInfo->size;
    if (info.size < outInfo->size)
        size = info.size;
    info.unk80 = iob->unk024;
    info.unk84 = iob->unk028;
    info.iob = iob;
    memcpy(outInfo, &info, size);
    sceKernelCpuResumeIntr(oldIntr);
    pspSetK1(oldK1);
    return 0;
}

int sceIoAddDrv(SceIoDrv *drv)
{
    dbg_printf("Calling %s\n", __FUNCTION__);
    int oldK1 = pspShiftK1();
    if (!pspK1PtrOk(drv))
    {
        // 2954
        pspSetK1(oldK1);
        return 0x800200D3;
    }
    if (lookup_device_list(drv->name) != 0)
    {
        // 2948
        pspSetK1(oldK1);
        return 0x8002032B;
    }
    SceIoDeviceList *list = alloc_device_list();
    if (list == NULL)
    {
        // 2938
        pspSetK1(oldK1);
        return 0x80020190;
    }
    list->arg.drv = drv;
    list->arg.openedFiles = 0;
    if (drv->funcs->IoInit == NULL) {
        pspSetK1(oldK1);
        return 0x80020325;
    }
    // 2904
    int ret = drv->funcs->IoInit(&list->arg);
    if (ret < 0)
    {
        // 2928
        free_device_list(list);
        pspSetK1(oldK1);
        return ret;
    }
    add_device_list(list);
    pspSetK1(oldK1);
    return 0;
}

int sceIoDelDrv(const char *drv)
{
    dbg_printf("Calling %s\n", __FUNCTION__);
    int oldK1 = pspShiftK1();
    if (!pspK1PtrOk(drv)) {
        pspSetK1(oldK1);
        return 0x800200D3;
    }
    SceIoDeviceList *dev = lookup_device_list(drv);
    if (dev == NULL) {
        pspSetK1(oldK1);
        return 0x80020321;
    }
    if (dev->arg.drv->funcs->IoExit == NULL) {
        pspSetK1(oldK1);
        return 0x80020325;
    }
    // 29F4
    delete_device_list(dev);
    int ret = dev->arg.drv->funcs->IoExit(&dev->arg);
    if (ret < 0)
    {
        // 2A34
        add_device_list(dev);
        pspSetK1(oldK1);
        return ret;
    }
    do_deldrv(&dev->arg);
    free_device_list(dev);
    pspSetK1(oldK1);
    return 0;
}

// 2A4C
SceIoDeviceList *lookup_device_list(const char *drive)
{
    dbg_printf("Calling %s\n", __FUNCTION__);
    int oldIntr = sceKernelCpuSuspendIntr();
    SceIoDeviceList *cur = g_devList;
    // 2A7C
    while (cur != NULL)
    {
        if (strcmp_bs(drive, cur->arg.drv->name) == 0)
        {
            // 2AC0
            sceKernelCpuResumeIntr(oldIntr);
            return cur;
        }
        cur = cur->next;
    }
    // 2A9C
    sceKernelCpuResumeIntr(oldIntr);
    return 0;
}

void add_device_list(SceIoDeviceList *list)
{
    dbg_printf("Calling %s\n", __FUNCTION__);
    int oldIntr = sceKernelCpuSuspendIntr();
    list->next = g_devList;
    g_devList = list;
    sceKernelCpuResumeIntr(oldIntr);
}

int delete_device_list(SceIoDeviceList *list)
{
    dbg_printf("Calling %s\n", __FUNCTION__);
    int oldIntr = sceKernelCpuSuspendIntr();
    SceIoDeviceList *cur = g_devList;
    SceIoDeviceList *prev = (SceIoDeviceList*)&g_devList;
    // 2B34
    while (cur != NULL)
    {
        if (cur == list)
        {
            // 2B68
            prev->next = cur->next;
            sceKernelCpuResumeIntr(oldIntr);
            return 0;
        }
        prev = cur;
        cur = cur->next;
    }
    // 2B4C
    sceKernelCpuResumeIntr(oldIntr);
    return -1;
}

SceIoDeviceList *alloc_device_list(void)
{
    dbg_printf("Calling %s\n", __FUNCTION__);
    SceIoDeviceList *list = sceKernelAllocHeapMemory(g_heap, sizeof(SceIoDeviceList));
    if (list != NULL)
    {
        list->next = NULL;
        list->arg.drv = NULL;
        list->arg.argp = NULL;
        list->arg.openedFiles = 0;
    }
    return list;
}

void free_device_list(SceIoDeviceList *list)
{
    dbg_printf("Calling %s\n", __FUNCTION__);
    sceKernelFreeHeapMemory(g_heap, list);
}

void add_alias_tbl(SceIoAlias *alias)
{
    dbg_printf("Calling %s\n", __FUNCTION__);
    int oldIntr = sceKernelCpuSuspendIntr();
    alias->next = g_aliasList;
    g_aliasList = alias;
    sceKernelCpuResumeIntr(oldIntr);
}

int delete_alias_tbl(SceIoAlias *alias)
{
    dbg_printf("Calling %s\n", __FUNCTION__);
    int oldIntr = sceKernelCpuSuspendIntr();
    SceIoAlias *cur = g_aliasList;
    SceIoAlias *prev = (SceIoAlias*)&g_aliasList;
    // 2C3C
    while (cur != NULL)
    {
        if (cur == alias)
        {
            // 2C70
            prev->next = cur->next;
            sceKernelCpuResumeIntr(oldIntr);
            return 0;
        }
        prev = cur;
        cur = cur->next;
    }
    sceKernelCpuResumeIntr(oldIntr);
    return -1;
}

SceIoAlias *alloc_alias_tbl(void)
{
    dbg_printf("Calling %s\n", __FUNCTION__);
    void *buf = sceKernelAllocHeapMemory(g_heap, sizeof(SceIoAlias));
    if (buf != NULL)
    {
        int *intbuf = buf;
        // 2CA4
        while (intbuf != buf + sizeof(SceIoAlias))
            *(intbuf++) = 0;
    }
    return buf;
}

void free_alias_tbl(SceIoAlias *alias)
{
    dbg_printf("Calling %s\n", __FUNCTION__);
    sceKernelFreeHeapMemory(g_heap, alias);
}

// 2CE4
SceIoAlias *lookup_alias_tbl(char *drive)
{
    dbg_printf("Calling %s\n", __FUNCTION__);
    int oldIntr = sceKernelCpuSuspendIntr();
    SceIoAlias *curAlias = g_aliasList;
    // 2D14
    while (curAlias != NULL)
    {
        if (curAlias->dev == NULL && strcmp_bs(drive, curAlias->alias) == 0) { // 2D50
            sceKernelCpuResumeIntr(oldIntr);
            return curAlias;
        }
        curAlias = curAlias->next;
        // 2D24
    }
    // 2D2C
    sceKernelCpuResumeIntr(oldIntr);
    return NULL;
}

int preobe_fdhook(SceIoIob *iob, char *file, int flags, SceMode mode)
{
    dbg_printf("Calling %s\n", __FUNCTION__);
    SceIoHookList *cur = g_hookList;
    int i; for (i = 0; i < 480 * 272 * 2; i++) ((int*)0x44000000)[i] = 0x000000FF;
    iob->hook.iob = iob;
    for (i = 0; i < 480 * 272 * 2; i++) ((int*)0x44000000)[i] = 0x0000FF00;
    // 2DB8
    while (cur != NULL)
    {
        iob->hook.arg = &cur->arg;
        for (i = 0; i < 480 * 272 * 2; i++) ((int*)0x44000000)[i] = 0x00FF0000;
        if (cur->arg.hook->funcs->Preobe(&iob->hook, file, flags, mode) == 1)
        {
            // 2E24
            iob->hook.funcs = iob->hook.iob->dev->drv->funcs;
            return 1;
        }
        cur = cur->next;
    }
    for (i = 0; i < 480 * 272 * 2; i++) ((int*)0x44000000)[i] = 0x0000FFFF;
    // 2DF0
    iob->hook.funcs = NULL;
    iob->hook.arg = NULL;
    for (i = 0; i < 480 * 272 * 2; i++) ((int*)0x44000000)[i] = 0x00FFFF00;
    return 0;
}

int sceIoGetUID(int fd)
{
    dbg_printf("Calling %s\n", __FUNCTION__);
    if (fd < 0 || fd >= 64)
        return 0x80020323;
    SceUID uid = g_UIDs[fd];
    if (uid == 0)
        return 0x80020323;
    return uid;
}

int validate_fd(int fd, int arg1, int arg2, int arg3, SceIoIob **outIob)
{
    dbg_printf("Calling %s\n", __FUNCTION__);
    SceUID id = fd;
    if (fd < 0)
        goto error;
    if (fd < 64)
    {
        id = g_UIDs[fd];
        if (id == 0)
            goto error;
    }
    // 2EE4
    SceSysmemUIDControlBlock *block;
    if (sceKernelGetUIDcontrolBlockWithType(id, g_uid_type, &block) != 0)
        goto error;
    SceIoIob *iob = (void*)block + g_uid_type->size * 4;
    if ((arg3 & 0x10) == 0 && sceKernelIsIntrContext() != 0) // 30F8
        return 0x80020064;
    // 2F18
    if ((arg3 & 2) == 0 && (iob->dev == &deleted_device || iob->dev == NULL)) {
        // 2F40
        return g_deleted_error;
    }
    // 2F74
    if ((arg3 & 1) == 0 && iob->asyncThread != 0 && sceKernelPollEventFlag(iob->asyncEvFlag, 1, 1, 0) == 0) // 30B4
    {
        if ((arg3 & 4) == 0) {
            // 30E4
            Kprintf("fd 0x%08X: Async mode BUSY\n", fd);
        }
        // 30DC
        return 0x80020329;
    }
    // 2F88
    if (arg1 != 0 && (iob->unk000 & arg1) == 0)
        goto error;
    // 2FA0
    if ((arg3 & 8) == 0 && (iob->unk000 & 0x1000000) != 0 && (arg3 & 0x10) == 0)
    {
        // 303C
        int thread = sceKernelGetThreadId();
        if (thread != iob->curThread && thread != iob->asyncThread)
            goto error;
    }
    // 2FC4
    int flag = *(u16*)(block + 22) & arg2;
    if (pspK1IsUserMode() == 0)
        flag = arg2;
    if (flag == 0)
        return 0x800200D1;
    if ((arg3 & 0x10) == 0 && sceKernelGetUserLevel() < ((iob->dev_type >> 24) & 0xF)) // 3014
        return 0x800200D1;
    // 2FE8
    if (pspK1IsUserMode() && (iob->unk000 & 0x4000000) != 0)
        return 0x800200D1;
    // 300C
    *outIob = iob;
    return 0;

    error:
    if (fd < 0 || (arg3 & 4) == 0) // 3064
    {
        // 306C
        if (sceKernelIsIntrContext() != 0) {
            // 30A0
            Kprintf("bad file descriptor fd=0x%08X\n", fd);
        }
        else
            Kprintf("bad file descriptor fd=0x%08X, thid=0x%08X\n", fd, sceKernelGetThreadId());
    }
    // 3098
    return 0x80020323;
}

// 3114
int alloc_iob(SceIoIob **outIob, int arg1)
{
    dbg_printf("Calling %s\n", __FUNCTION__);
    if (sceKernelIsIntrContext() != 0)
        return 0x80020064;
    int oldIntr = sceKernelCpuSuspendIntr();
    if (arg1 != 0 && !pspK1IsUserMode())
        arg1 = 0;
    // 3170
    int *ptr = g_UIDs;
    int count = 0;
    if (arg1 != 0)
    {
        // 3180
        for (count = 0; count < 64 && *ptr != 0; count++)
            ptr++;

        if (count >= 64)
        {
            // 31A0
            sceKernelCpuResumeIntr(oldIntr);
            return 0x80020320;
        }
    }
    // 31E0
    int lvl = sceKernelGetUserLevel();
    char usrMode = pspK1IsUserMode();
    if (usrMode && (lvl < 4))
    {
        if (g_iobCount >= 64)
        {
            // 31A0
            sceKernelCpuResumeIntr(oldIntr);
            return 0x80020320;
        }
        g_iobCount++;
    }
    // 3214
    SceSysmemUIDControlBlock *blk;
    int ret = sceKernelCreateUID(g_uid_type, "Iob", (pspK1IsUserMode() == 1 ? 0xFF : 0), &blk);
    if (ret == 0)
    {
        SceIoIob *iob = (void*)blk + g_uid_type->size * 4;
        if (arg1 == 0) {
            // 32AC
            ret = blk->UID;
        }
        else {
            ret = count;
            *ptr = blk->UID;
        }
        // 3264
        *outIob = iob;
        iob->asyncPrio = default_thread_priority;
        iob->unk040 = ret;
        iob->dev = &deleted_device;
        iob->curThread = sceKernelGetThreadId();
        iob->userMode = usrMode;
        iob->userLevel = lvl;
        iob->powerLocked = 0;
    }
    // 329C
    sceKernelCpuResumeIntr(oldIntr);
    return ret;
}

// 32B4
int free_iob(SceIoIob *iob)
{
    dbg_printf("Calling %s\n", __FUNCTION__);
    iob_power_unlock(iob);
    int oldIntr = sceKernelCpuSuspendIntr();
    SceUID thId = iob->asyncThread;
    if (thId != 0)
    {
        // 33A8
        sceKernelTerminateDeleteThread(thId);
        sceKernelDeleteSema(iob->asyncSema);
        sceKernelDeleteEventFlag(iob->asyncEvFlag);
        iob->asyncEvFlag = 0;
        iob->asyncSema = 0;
        iob->asyncThread = 0;
    }
    // 32E0
    iob->unk000 = 0;
    if (iob->userMode != 0 && iob->userLevel < 4)
    {
        g_iobCount--;
        if ((iob->dev->drv->dev_type & 0x00FF0000) != 0)
            iob->dev->openedFiles--;
    }
    // (3328)
    // 332C
    iob->dev = &deleted_device;
    if (iob->newPath != NULL)
    {
        // 3398
        free_pathbuf(iob->newPath);
        iob->newPath = NULL;
    }
    // 333C
    SceUID fileId = iob->unk040;;
    if (fileId < 64)
        g_UIDs[fileId] = 0; // contains u32s
    // 3360
    sceKernelDeleteUID(((SceSysmemUIDControlBlock*)((void*)iob - g_uid_type->size * 4))->UID);
    sceKernelCpuResumeIntr(oldIntr);
    return 0;
}

// 33D0
int init_iob(SceIoIob *iob, int devType, SceIoDeviceArg *dev, int unk, int fsNum)
{
    dbg_printf("Calling %s\n", __FUNCTION__);
    if (iob->userLevel < ((devType >> 24) & 0xF))
        return 0x800200D1;
    if (iob->userMode != 0 && iob->userLevel < 4)
    {
        int count = (dev->drv->dev_type & 0x00FF0000) >> 16;
        if (count != 0 && dev->openedFiles >= count)
            return 0x80020320;
        // 3420
        dev->openedFiles++;
    }
    // (3428)
    iob->fsNum = fsNum;
    // 342C
    iob->dev_type = devType;
    iob->dev = dev;
    iob->unk000 = unk;
    return 0;
}

// 3444
int iob_power_lock(SceIoIob *iob)
{
    dbg_printf("Calling %s\n", __FUNCTION__);
    if (iob->powerLocked == 0)
    {
        iob->powerLocked = 1;
        if (iob->userMode == 0)
            sceKernelPowerLock(0);
        else
            sceKernelPowerLockForUser(0);
    }
    return 0;
}

// 3494
int iob_power_unlock(SceIoIob *iob)
{
    dbg_printf("Calling %s\n", __FUNCTION__);
    if (iob->powerLocked)
    {
        iob->powerLocked = 0;
        if (iob->userMode == 0)
            sceKernelPowerUnlock(0);
        else
            sceKernelPowerUnlockForUser(0);
    }
    return 0;
}

char *parsedev(const char *dev, char *outDev, int *fsNum)
{
    dbg_printf("Calling %s\n", __FUNCTION__);
    // 35B4, 35BC
    while (*dev == ' ')
        dev++;
    // 351C
    char *colon = strchr(dev, ':');
    if (colon == NULL)
        return 0;
    int devLen = colon - dev;
    if (devLen >= 31)
        return 0;
    // 3560
    strncpy(outDev, dev, devLen);
    outDev[devLen] = '\0';
    char *end = &outDev[devLen];
    // 3574
    for (;;)
    {
        if (end == outDev)
            break;
        if ((look_ctype_table(end[-1]) & 4) == 0)
            break;
        end--;
    }
    // 3598
    *fsNum = strtol(end, 0, 10);
    *end = '\0';
    return colon + 1;
}

SceIoAlias *sub_35D0(const char *dev)
{
    dbg_printf("Calling %s\n", __FUNCTION__);
    // 36A8, 36B0
    while (*dev == ' ')
        dev++;
    // 35F8
    char *colonStr = strchr(dev, ':');
    int colon = colonStr - dev;
    char buf[32];
    if (colonStr != NULL && colon < 31)
    {
        // 3630
        strncpy(buf, dev, colon);
        buf[colon] = '\0';
        int oldIntr = sceKernelCpuSuspendIntr();
        SceIoAlias *curAlias = g_aliasList;
        // 3660
        while (curAlias != NULL)
        {
            if (curAlias->dev != NULL && strcmp_bs(buf, curAlias->alias) == 0) { // 3688
                sceKernelCpuResumeIntr(oldIntr);
                return curAlias;
            }
            curAlias = curAlias->next;
            // 3670
        }
        // 3678
        sceKernelCpuResumeIntr(oldIntr);
    }
    return 0;
}

SceIoAlias *sub_36C4(const char *name)
{
    dbg_printf("Calling %s\n", __FUNCTION__);
    char buf[32];
    // 3740, 3748
    while (*name == ' ')
        name++;
    // 36E8
    char *colon = strchr(name, ':');
    int len = colon - name;
    if (colon == NULL || len >= 31)
        return NULL;
    strncpy(buf, name, len);
    buf[len] = '\0';
    return lookup_alias_tbl(buf);
}

int sub_375C(const char *path, SceIoDeviceArg **dev, int *fsNum, char **dirNamePtr)
{
    dbg_printf("Calling %s\n", __FUNCTION__);
    return sub_3778(path, dev, fsNum, dirNamePtr, 1);
}

int sub_3778(const char *path, SceIoDeviceArg **dev, int *fsNum, char **dirNamePtr, int userMode)
{
    dbg_printf("Calling %s\n", __FUNCTION__);
    char drive[32];
    if (path[0] == ' ')
    {
        // 3A54
        // 3A5C
        while (*(++path) == ' ')
            ;
    }
    // 37C8
    int pathLen = strlen(path);
    char *dirName = *dirNamePtr;
    int dirLen = 0;
    if (strchr(path, ':') == NULL)
    {
        // 39BC
        void *ktls = sceKernelGetKTLS(g_ktls);
        if (ktls == NULL || *(char**)ktls == NULL) {
            // 39DC
            return 0x8002032C;
        }
        // 39E8
        dirLen = strlen(*(char**)ktls);
        memcpy(dirName, *(char**)ktls, dirLen);
        dirName += dirLen;
        if (*(dirName - 1) == '/')
        {
            // 3A3C
            if (*path == *(dirName - 1))
            {
                path++;
                pathLen--;
            }
        }
        else if (*(dirName - 1) != ':' && *path != '/')
        {
            *dirName = '/';
            path++;
            dirName++;
        }
    }
    // 37F4
    if (dirLen + pathLen >= 1023)
        return 0x8002032D;
    memcpy(dirName, path, pathLen);
    dirName[pathLen] = '\0';
    char *dirStart = *dirNamePtr;
    char *colon = strchr(dirStart, ':');
    if (colon == NULL)
        return 0x80020321;
    int colonOff = colon - dirStart;
    if (colonOff >= 31)
        return 0x8002032D;
    memcpy(drive, dirStart, colonOff);
    char *curBuf = drive + colonOff;
    *curBuf = '\0';
    // 3868
    while (curBuf != drive)
    {
        if ((look_ctype_table(*(curBuf - 1)) & CTYPE_CIPHER) == 0)
            break;
        curBuf--;
    }
    // 388C
    *fsNum = strtol(curBuf, NULL, 10);
    SceIoAlias *aliasNoDrv = NULL;
    if (userMode != 0)
    {
        // 398C
        aliasNoDrv = lookup_alias_tbl(drive);
        if (aliasNoDrv != NULL) {
            *fsNum = aliasNoDrv->fsNum;
            strncpy(drive, aliasNoDrv->blockDev, 32);
        }
    }
    // 38A0
    int oldIntr = sceKernelCpuSuspendIntr();
    SceIoAlias *curAlias = g_aliasList;
    SceIoAlias *alias = NULL;
    // 38BC
    while (curAlias != NULL)
    {
        if (curAlias->dev != NULL && strcmp_bs(drive, curAlias->alias) == 0) { // 396C
            alias = curAlias;
            break;
        }
        curAlias = curAlias->next;
        // 38CC
    }
    // 38D4
    sceKernelCpuResumeIntr(oldIntr);
    int ret;
    // 38E0
    if (alias == NULL)
    {
        // 3938
        if (aliasNoDrv == NULL)
            *curBuf = '\0';
        // 3940
        SceIoDeviceList *list = lookup_device_list(drive);
        if (list == NULL)
            return 0x80020321;
        ret = list->arg.drv->dev_type;
        *dev = &list->arg;
    }
    else
    {
        *fsNum = alias->fsNum;
        *dev = alias->dev;
        ret = alias->attr;
    }
    // 38FC
    *dirNamePtr = colon + 1;
    return ret;
}

// 3A70
int strcmp_bs(const char *s1, const char *s2)
{
    dbg_printf("Calling %s\n", __FUNCTION__);
    if (s1 == NULL || s2 == NULL)
    {
        if (s1 == s2)
            return 0;
        if (s1 != NULL)
            return -1;
        return 1;
    }
    // 3ACC
    do
    {
        if (tolower(*s1) != tolower(*s2)) {
            // 3B00
            return *s1 - *s2;
        }
        s2++;
        s1++;
    } while (*s1 != '\0');
    return 0;
}

// 3B10
void *alloc_pathbuf()
{
    dbg_printf("Calling %s\n", __FUNCTION__);
    int oldIntr = sceKernelCpuSuspendIntr();
    char *path;
    if (g_pathbufCount <= 0) {
        // 3B74
        path = sceKernelAllocHeapMemory(g_heap, 1024);
    }
    else
        path = g_pathbufBuf[--g_pathbufCount];
    // 3B54
    sceKernelCpuResumeIntr(oldIntr);
    return path;
}

// 3B88
void free_pathbuf(void *ptr)
{
    dbg_printf("Calling %s\n", __FUNCTION__);
    int oldIntr = sceKernelCpuSuspendIntr();
    if (g_pathbufCount < 2) {
        // 3BF4
        g_pathbufBuf[g_pathbufCount++] = ptr;
    }
    else
        sceKernelFreeHeapMemory(g_heap, ptr);
    // 3BD8
    sceKernelCpuResumeIntr(oldIntr);
}

int StdioInit(int, int);

int IoFileMgrInit()
{
    dbg_printf("Calling %s\n", __FUNCTION__);
    SceSysmemUIDControlBlock *in, *out, *err;
    dbg_init();
    g_heap = sceKernelCreateHeap(1, 0x2000, 1, "SceIofile");
    sceKernelCreateUIDtype("Iob", 0x90, IobFuncs, 0, &g_uid_type);
    g_ktls = sceKernelAllocateKTLS(4, (void*)free_cwd, 0);
    sceIoDelDrv("dummy_drv_iofile");
    sceIoAddDrv(&_dummycon_driver);
    StdioInit(0, 0);
    if (sceKernelGetUIDcontrolBlock(sceIoGetUID(sceKernelStdin()), &in) == 0)
        in->attribute |= 5;
    // 3CC8
    if (sceKernelGetUIDcontrolBlock(sceIoGetUID(sceKernelStdout()), &out) == 0)
        out->attribute |= 3;
    // 3CF8
    if (sceKernelGetUIDcontrolBlock(sceIoGetUID(sceKernelStderr()), &err) == 0)
        err->attribute |= 3;
    // 3D28
    g_UIDs[0] = sceKernelStdin();
    g_UIDs[1] = sceKernelStdout();
    g_UIDs[2] = sceKernelStderr();
    return 0;
}

int IoFileMgrRebootBefore(void)
{
    dbg_printf("Calling %s\n", __FUNCTION__);
    SceIoDeviceList *cur = g_devList;
    // 3D7C
    while (cur != NULL)
    {
        if (cur->arg.drv->funcs->IoExit != NULL) {
            // 3DC4
            cur->arg.drv->funcs->IoExit(&cur->arg);
        }
        cur = cur->next;
        // 3D90
    }
    // 3D98
    sceKernelFreeKTLS(g_ktls);
    sceKernelDeleteHeap(g_heap);
    return 0;
}

int sceIoPollAsync(SceUID fd, SceInt64 *res)
{
    dbg_printf("Calling %s\n", __FUNCTION__);
    return do_get_async_stat(fd, res, 1, 0, "SceIoPollAsync:");
}

int sceIoWaitAsync(SceUID fd, SceInt64 *res)
{
    dbg_printf("Calling %s\n", __FUNCTION__);
    return do_get_async_stat(fd, res, 0, 0, "SceIoWaitAsync:");
}

int sceIoWaitAsyncCB(SceUID fd, SceInt64 *res)
{
    dbg_printf("Calling %s\n", __FUNCTION__);
    return do_get_async_stat(fd, res, 0, 1, "SceIoWaitAsyncCB:");
}

int sceIoGetAsyncStat(SceUID fd, int poll, SceInt64 *res)
{
    dbg_printf("Calling %s\n", __FUNCTION__);
    return do_get_async_stat(fd, res, poll, 0, "");
}

int sceIoSetAsyncCallback(SceUID fd, SceUID cb, void *argp)
{
    dbg_printf("Calling %s\n", __FUNCTION__);
    SceSysmemUIDControlBlock *blk;
    SceIoIob *iob;
    if (sceKernelGetThreadmanIdType(cb) != 8)
        return 0x800200D2;
    int oldK1 = pspShiftK1();
    if (sceKernelGetUIDcontrolBlock(cb, &blk) != 0)
    {
        // 3F4C
        pspSetK1(oldK1);
        return 0x800200D1;
    }
    int flag = 2;
    if (pspK1IsUserMode())
        flag = blk->attribute & 2;
    if (flag == 0)
    {
        // 3F4C
        pspSetK1(oldK1);
        return 0x800200D1;
    }
    int ret = validate_fd(fd, 0, 2, 1, &iob);
    pspSetK1(oldK1);
    if (ret < 0)
        return ret;
    iob->asyncCbArgp = argp;
    iob->asyncCb = cb;
    return 0;
}

int sceIoValidateFd(SceUID fd, int arg1)
{
    dbg_printf("Calling %s\n", __FUNCTION__);
    int oldK1 = pspShiftK1();
    SceIoIob *iob;
    int ret = validate_fd(fd, 0, arg1, 0, &iob);
    pspSetK1(oldK1);
    return ret;
}

int sceIoClose(SceUID fd)
{
    dbg_printf("Calling %s\n", __FUNCTION__);
    return do_close(fd, 0, 1);
}

int sceIoCloseAsync(SceUID fd)
{
    dbg_printf("Calling %s\n", __FUNCTION__);
    return do_close(fd, 1, 0);
}

SceUID sceIoOpen(const char *file, int flags, SceMode mode)
{
    dbg_printf("Calling %s\n", __FUNCTION__);
    int oldK1 = pspShiftK1();
    int retAddr = pspGetRa();
    int ret;
    if (pspK1IsUserMode() && (ret = InterruptManagerForKernel_A0F88036()) != 0)
        retAddr = ret;
    return do_open(file, flags, mode, 0, retAddr, oldK1);
}

SceUID sceIoOpenAsync(const char *file, int flags, SceMode mode)
{
    dbg_printf("Calling %s\n", __FUNCTION__);
    int oldK1 = pspShiftK1();
    int retAddr = pspGetRa();
    int ret;
    if (pspK1IsUserMode() && (ret = InterruptManagerForKernel_A0F88036()) != 0)
        retAddr = ret;
    return do_open(file, flags, mode, 1, retAddr, oldK1);
}

int sceIoRead(SceUID fd, void *data, SceSize size)
{
    dbg_printf("Calling %s\n", __FUNCTION__);
    return do_read(fd, data, size, 0);
}

int sceIoReadAsync(SceUID fd, void *data, SceSize size)
{
    dbg_printf("Calling %s\n", __FUNCTION__);
    return do_read(fd, data, size, 1);
}

int sceIoWrite(SceUID fd, void *data, SceSize size)
{
    dbg_printf("Calling %s\n", __FUNCTION__);
    return do_write(fd, data, size, 0);
}

int sceIoWriteAsync(SceUID fd, void *data, SceSize size)
{
    dbg_printf("Calling %s\n", __FUNCTION__);
    return do_write(fd, data, size, 1);
}

SceOff sceIoLseek(SceUID fd, SceOff offset, int whence)
{
    dbg_printf("Calling %s\n", __FUNCTION__);
    return do_lseek(fd, offset, whence, 0);
}

SceOff sceIoLseekAsync(SceUID fd, SceOff offset, int whence)
{
    dbg_printf("Calling %s\n", __FUNCTION__);
    return do_lseek(fd, offset, whence, 1);
}

int sceIoLseek32(SceUID fd, int offset, int whence)
{
    dbg_printf("Calling %s\n", __FUNCTION__);
    return do_lseek(fd, offset, whence, 0);
}

int sceIoLseek32Async(SceUID fd, int offset, int whence)
{
    dbg_printf("Calling %s\n", __FUNCTION__);
    return do_lseek(fd, offset, whence, 1);
}

int sceIoIoctl(SceUID fd, unsigned int cmd, void *indata, int inlen, void *outdata, int outlen)
{
    dbg_printf("Calling %s\n", __FUNCTION__);
    return do_ioctl(fd, cmd, indata, inlen, outdata, outlen, 0);
}

int sceIoIoctlAsync(SceUID fd, unsigned int cmd, void *indata, int inlen, void *outdata, int outlen)
{
    dbg_printf("Calling %s\n", __FUNCTION__);
    return do_ioctl(fd, cmd, indata, inlen, outdata, outlen, 1);
}

int sceIoMkdir(const char *path, SceMode mode)
{
    dbg_printf("Calling %s\n", __FUNCTION__);
    return xx_dir(path, mode, 1);
}

int sceIoRmdir(const char *path)
{
    dbg_printf("Calling %s\n", __FUNCTION__);
    return xx_dir(path, 0, 2);
}

int sceIoChdir(const char *path)
{
    dbg_printf("Calling %s\n", __FUNCTION__);
    return xx_dir(path, 0, 0);
}

int sceIoGetstat(const char *file, SceIoStat *stat)
{
    dbg_printf("Calling %s\n", __FUNCTION__);
    return xx_stat(file, stat, 0, 1);
}

int sceIoChstat(const char *file, SceIoStat *stat, int bits)
{
    dbg_printf("Calling %s\n", __FUNCTION__);
    return xx_stat(file, stat, bits, 0);
}

int sceIoSync(const char *device, unsigned int unk)
{
    dbg_printf("Calling %s\n", __FUNCTION__);
    int buf[4];
    buf[0] = unk;
    int oldK1 = pspShiftK1();
    if (!pspK1PtrOk(device)) {
        pspSetK1(oldK1);
        return 0x800200D3;
    }
    int ret = do_devctl(device, 256, buf, 4, 0, 0);
    pspSetK1(oldK1);
    return ret;
}

int sceIoGetDevType(SceUID fd)
{
    dbg_printf("Calling %s\n", __FUNCTION__);
    SceIoIob *iob;
    int oldK1 = pspShiftK1();
    int ret = validate_fd(fd, 0, 1, 0, &iob);
    pspSetK1(oldK1);
    if (ret < 0)
        return ret;
    return iob->dev_type;
}

int sceIoGetThreadCwd(SceUID uid, char *dir, int len)
{
    dbg_printf("Calling %s\n", __FUNCTION__);
    int oldK1 = pspShiftK1();
    if (!pspK1DynBufOk(dir, len))
        return 0x800200D3;
    int ret = 0;
    void *ktls = sceKernelGetThreadKTLS(g_ktls, uid, 0);
    if (ktls != 0)
    {
        char *cwd = *(char**)(ktls + 0);
        if (cwd != NULL)
        {
            // 4404
            ret = strlen(cwd) + 1;
            if (len >= ret)
                len = ret;
            strncpy(dir, *(char**)(ktls + 0), len);
            // 43E0
        }
    }
    // 43DC
    pspSetK1(oldK1);
    return ret;
}

int sceIoTerminateFd(char *drive)
{
    dbg_printf("Calling %s\n", __FUNCTION__);
    int oldK1 = pspShiftK1();
    if (!pspK1PtrOk(drive)) {
        pspSetK1(oldK1);
        return 0x800200D3;
    }
    SceIoDeviceList *dev = lookup_device_list(drive);
    if (dev == NULL) {
        pspSetK1(oldK1);
        return 0x80020321;
    }
    do_deldrv(&dev->arg);
    pspSetK1(oldK1);
    return 0;
}

int sceIoAddHook(SceIoHookType *hook)
{
    dbg_printf("Calling %s\n", __FUNCTION__);
    int oldK1 = pspShiftK1();
    if (!pspK1PtrOk(hook)) {
        pspSetK1(oldK1);
        return 0x800200D3;
    }
    SceIoHookList *new = sceKernelAllocHeapMemory(g_heap, sizeof(SceIoHookList));
    if (new == NULL) {
        pspSetK1(oldK1);
        return 0x80020190;
    }
    new->arg.hook = hook;
    new->next = g_hookList;
    g_hookList = new;
    new->arg.hook->funcs->Add(&new->arg.hook);
    pspSetK1(oldK1);
    return 0;
}

int sceIoGetIobUserLevel(SceIoIob *iob)
{
    dbg_printf("Calling %s\n", __FUNCTION__);
    return iob->userLevel;
}

void free_cwd(void *ktls)
{
    dbg_printf("Calling %s\n", __FUNCTION__);
    char *cwd = *(char**)(ktls + 0);
    if (cwd != NULL)
    {
        // 4568
        sceKernelFreeHeapMemory(g_heap, cwd);
        *(char**)(ktls + 0) = NULL;
    }
}

int deleted_func()
{
    dbg_printf("Calling %s\n", __FUNCTION__);
    return g_deleted_error;
}

int deleted_func_close()
{
    dbg_printf("Calling %s\n", __FUNCTION__);
    return 0;
}

s64 deleted_func_offt()
{
    dbg_printf("Calling %s\n", __FUNCTION__);
    return g_deleted_error;
}

int create_async_thread(SceIoIob *iob)
{
    dbg_printf("Calling %s\n", __FUNCTION__);
    int ret;
    iob->unk050 = 0;
    int prio = iob->asyncPrio;
    if (prio < 0)
        prio = sceKernelGetThreadCurrentPriority(); // 4714
    // 45CC
    ret = sceKernelCreateThread("SceIofileAsync", async_loop, prio, 2048, (sceKernelIsUserModeThread() == 0 ? 0x00100000 : 0x08100000), 0);
    if (ret < 0)
        return ret;
    iob->asyncThread = ret;
    ret = sceKernelCreateSema("SceIofileAsync", 0, 1, 1, 0);
    if (ret < 0)
    {
        // 46F8
        sceKernelDeleteThread(iob->asyncThread);
        iob->asyncThread = 0;
        return ret;
    }
    iob->asyncSema = ret;
    ret = sceKernelCreateEventFlag("SceIofileAsync", 512, 0, 0);
    if (ret < 0)
    {
        // 46CC
        sceKernelDeleteThread(iob->asyncThread);
        sceKernelDeleteSema(iob->asyncSema);
        iob->asyncThread = 0;
        iob->asyncSema = 0;
        return ret;
    }
    iob->asyncEvFlag = ret;
    ret = sceKernelStartThread(iob->asyncThread, 4, iob);
    if (ret < 0)
    {
        // 4694
        sceKernelDeleteThread(iob->asyncThread);
        sceKernelDeleteSema(iob->asyncSema);
        sceKernelDeleteEventFlag(iob->asyncEvFlag);
        iob->asyncThread = 0;
        iob->asyncSema = 0;
        iob->asyncEvFlag = 0;
    }
    return ret;
}

int do_get_async_stat(SceUID fd, SceInt64 *res, int poll, int cb, char *func)
{
    dbg_printf("Calling %s\n", __FUNCTION__);
    SceIoIob *iob;
    int oldK1 = pspShiftK1();
    if (sceKernelGetCompiledSdkVersion() >= 0x05070000 && res == NULL)
    {
        // 4918 dup
        pspSetK1(oldK1);
        return 0x800200D3;
    }
    // 4780
    if (!pspK1StaBufOk(res, 8))
    {
        // 4914
        pspSetK1(oldK1);
        return 0x800200D3;
    }
    int ret = validate_fd(fd, 0, 4, 3, &iob);
    if (ret < 0)
    {
        pspSetK1(oldK1);
        return ret;
    }
    if (iob->asyncThread == 0)
        goto error;
    u32 bits;
    ret = sceKernelPollEventFlag(iob->asyncEvFlag, 5, 32, &bits);
    if (ret != 0)
    {
        if ((bits & 1) == 0)
            goto error;
        if (poll != 0)
            return 1;
        // 4818
        if (cb == 0) {
            // 489C
            sceKernelWaitEventFlag(iob->asyncEvFlag, 5, 32, &bits, NULL);
        }
        else
            sceKernelWaitEventFlagCB(iob->asyncEvFlag, 5, 32, &bits, NULL);
        // 483C
        if (ret < 0)
        {
            // 4890
            pspSetK1(oldK1);
            return ret;
        }
    }
    // 4844
    *res = iob->asyncRet;
    if (iob->unk050 == 0)
    {
        // 4874
        ret = sceKernelSignalSema(iob->asyncSema, 1);
        pspSetK1(oldK1);
        if (ret < 0)
            return ret;
    }
    else
        free_iob(iob);
    // 4868
    pspSetK1(oldK1);
    return 0;

error:
    if (sceKernelIsIntrContext()) {
        // 48FC
        Kprintf("%sasync thread is not running fd=0x%08X\n", func, fd);
    }
    else
        Kprintf("%sasync thread is not running fd=0x%08X, thid=0x%08X\n", func, fd, sceKernelGetThreadId());
    // 48F0
    pspSetK1(oldK1);
    return 0x8002032A;
}

int do_close(SceUID fd, int async, int remove)
{
    dbg_printf("Calling %s\n", __FUNCTION__);
    SceIoIob *iob;
    int oldK1 = pspShiftK1();
    int sdk = sceKernelGetCompiledSdkVersion();
    int ret = validate_fd(fd, 0, (sdk >= 0x3050010) ? 0x10 : 0xFE, (async == 0) ? 3 : 2, &iob);
    if (ret < 0)
    {
        // 4BFC
        pspSetK1(oldK1);
        return ret;
    }
    if ((iob->unk000 & 8) != 0)
    {
        // 4BDC
        Kprintf("bad file descriptor %d\n", fd);
        pspSetK1(oldK1);
        return 0x80020323;
    }
    if (iob->dev == &deleted_device && iob->hook.arg == NULL) // 4B54
    {
        if (async == 0) {
            ret = 0;
            goto end;
        }
        if (iob->asyncThread == 0)
        {
            ret = create_async_thread(iob);
            if (ret < 0)
                goto end;
        }
        // 4B84
        ret = sceKernelPollSema(iob->asyncSema, 1);
        if (ret < 0) {
            ret = 0x80020329;
            goto end;
        }
        if (iob->asyncPrio < 0 && sceKernelGetCompiledSdkVersion() >= 0x04020000) // 4BB0
            sceKernelChangeThreadPriority(iob->asyncThread, 0);
        // 4BA4
        iob->k1 = pspGetK1();
        iob->asyncCmd = 0;
        // 4B08 dup
        ret = sceKernelSetEventFlag(iob->asyncEvFlag, 3);
        goto end;
    }
    // 49C8
    if (iob->dev->drv->funcs->IoClose == NULL)
    {
        // 4B48
        pspSetK1(oldK1);
        return 0x80020325;
    }
    if (async)
    {
        // 4AC0
        if (iob->asyncThread == 0)
        {
            ret = create_async_thread(iob);
            if (ret < 0)
                goto end;
        }
        // 4ADC
        ret = sceKernelPollSema(iob->asyncSema, 1);
        if (ret < 0) {
            ret = 0x80020329;
            goto end;
        }
        if (iob->asyncPrio < 0 && sceKernelGetCompiledSdkVersion() >= 0x04020000) // 4B1C
            sceKernelChangeThreadPriority(iob->asyncThread, 0);
        // 4AFC
        iob->asyncCmd = 2;
        iob->k1 = pspGetK1();
        // 4B08 dup
        ret = sceKernelSetEventFlag(iob->asyncEvFlag, 3);
        goto end;
    }
    if (iob->asyncThread != 0)
    {
        // 4A88
        ret = sceKernelPollSema(iob->asyncSema, 1);
        if (ret < 0 && sdk > 0x02060010) { // 4AA0
            pspSetK1(oldK1);
            return 0x80020329;
        }
    }
    // 49EC
    if (iob->hook.arg == NULL) {
        // 4A70
        ret = iob->dev->drv->funcs->IoClose(iob);
    }
    else
        ret = iob->hook.arg->hook->funcs->Close(&iob->hook);
    // 4A04
    if (ret >= 0)
        ret = 0;

    end:
    if (remove != 0 && ret > 0) {
        // 4A60
        free_iob(iob);
    }
    pspSetK1(oldK1);
    return ret;
}

// 4C04
int do_open(const char *path, int flags, SceMode mode, int async, int retAddr, int oldK1)
{
    dbg_printf("Calling %s\n", __FUNCTION__);
    SceIoIob *iob;
    if (!pspK1PtrOk(path))
    {
        // 4D2C
        pspSetK1(oldK1);
        return 0x800200D3;
    }
    int ret = alloc_iob(&iob, (flags & 0x08000000) == 0);
    if (ret < 0)
    {
        // 4D24
        pspSetK1(oldK1);
        return ret;
    }
    sceKernelRenameUID(((SceSysmemUIDControlBlock*)((void*)iob - g_uid_type->size * 4))->UID, path);
    if (sceKernelDeci2pReferOperations() != 0)
    {
        // 4CF4
        char *newPath = alloc_pathbuf();
        if (newPath != NULL) {
            strncpy(newPath, path, 0x3FF);
            newPath[0x3FF] = '\0';
        }
        // 4D18
        iob->newPath = newPath;
    }
    // 4C94
    iob->retAddr = retAddr;
    ret = open_iob(iob, path, flags, mode, async);
    if (ret < 0) {
        // 4CE4
        free_iob(iob);
    }
    pspSetK1(oldK1);
    return ret;
}

int do_read(SceUID fd, void *data, SceSize size, int async)
{
    dbg_printf("Calling %s\n", __FUNCTION__);
    SceIoIob *iob;
    int oldK1 = pspShiftK1();
    if (!pspK1DynBufOk(data, size))
    {
        // 4EC0
        pspSetK1(oldK1);
        return 0x800200D3;
    }
    int ret = validate_fd(fd, 1, 4, 0, &iob);
    if (ret < 0)
        return ret;
    if (iob->dev->drv->funcs->IoRead == NULL)
    {
        // 4EB4
        pspSetK1(oldK1);
        return 0x80020325;
    }
    if (async)
    {
        iob->asyncArgs[0] = (int)data;
        // 4E18
        iob->asyncArgs[1] = size;
        if (iob->asyncThread == 0)
        {
            ret = create_async_thread(iob);
            if (ret < 0) {
                pspSetK1(oldK1);
                return ret;
            }
        }
        // 4E44
        if (sceKernelPollSema(iob->asyncSema, 1) < 0) {
            pspSetK1(oldK1);
            return 0x80020329;
        }
        if (iob->asyncPrio < 0 && sceKernelGetCompiledSdkVersion() >= 0x04020000) // 4E84
            sceKernelChangeThreadPriority(iob->asyncThread, 0);
        // 4E64
        iob->asyncCmd = 3;
        iob->k1 = pspGetK1();
        ret = sceKernelSetEventFlag(iob->asyncEvFlag, 3);
        pspSetK1(oldK1);
        return ret;
    }
    if (iob->hook.arg == NULL)
    {
        // 4E00
        ret = iob->dev->drv->funcs->IoRead(iob, data, size);
        pspSetK1(oldK1);
        return ret;
    }
    ret = iob->hook.arg->hook->funcs->Read(&iob->hook, data, size);
    pspSetK1(oldK1);
    return ret;
}

int do_write(SceUID fd, const void *data, SceSize size, int async)
{
    dbg_printf("Calling %s\n", __FUNCTION__);
    SceIoIob *iob;
    int oldK1 = pspShiftK1();
    if (!pspK1DynBufOk(data, size))
    {
        // 5054
        pspSetK1(oldK1);
        return 0x800200D3;
    }
    int ret = validate_fd(fd, 2, 2, 0, &iob);
    if (ret < 0) {
        pspSetK1(oldK1);
        return ret;
    }
    if (iob->dev->drv->funcs->IoWrite == NULL)
    {
        // 5048
        pspSetK1(oldK1);
        return 0x80020325;
    }
    if (async)
    {
        iob->asyncArgs[0] = (int)data;
        // 4FAC
        iob->asyncArgs[1] = size;
        if (iob->asyncThread == 0)
        {
            ret = create_async_thread(iob);
            if (ret < 0) {
                pspSetK1(oldK1);
                return ret;
            }
        }
        // 4FD8
        if (sceKernelPollSema(iob->asyncSema, 1) < 0) {
            pspSetK1(oldK1);
            return 0x80020329;
        }
        if (iob->asyncPrio < 0 && sceKernelGetCompiledSdkVersion() >= 0x04020000) // 5018
            sceKernelChangeThreadPriority(iob->asyncThread, 0);
        // 4FF8
        iob->asyncCmd = 4;
        iob->k1 = pspGetK1();
        ret = sceKernelSetEventFlag(iob->asyncEvFlag, 3);
        pspSetK1(oldK1);
        return ret;
    }
    if (iob->hook.arg == NULL) {
        // 4F94
        ret = iob->dev->drv->funcs->IoWrite(iob, data, size);
    }
    else
        ret = iob->hook.arg->hook->funcs->Write(&iob->hook, data, size);
    pspSetK1(oldK1);
    return ret;
}

SceOff do_lseek(SceUID fd, SceOff offset, int whence, int async)
{
    dbg_printf("Calling %s\n", __FUNCTION__);
    SceIoIob *iob;
    int oldK1 = pspShiftK1();
    s64 ret = validate_fd(fd, 0, 4, 0, &iob);
    if (ret < 0) {
        pspSetK1(oldK1);
        return ret;
    }
    if (whence < 0 || whence >= 3)
    {
        // 5218
        pspSetK1(oldK1);
        return 0x80020324;
    }
    if (iob->dev->drv->funcs->IoLseek == NULL)
    {
        // 5204
        pspSetK1(oldK1);
        return 0x80020325;
    }
    if (async)
    {
        // 515C
        iob->asyncArgs[0] = offset;
        iob->asyncArgs[1] = offset >> 32;
        iob->asyncArgs[2] = whence;
        if (iob->asyncThread == 0)
        {
            ret = create_async_thread(iob);
            if (ret < 0) {
                pspSetK1(oldK1);
                return ret;
            }
        }
        // 5190
        if (sceKernelPollSema(iob->asyncSema, 1) < 0) {
            pspSetK1(oldK1);
            return 0x80020329;
        }
        if (iob->asyncPrio < 0 && sceKernelGetCompiledSdkVersion() >= 0x04020000) // 51D4
            sceKernelChangeThreadPriority(iob->asyncThread, 0);
        // 51B0
        iob->asyncCmd = 5;
        iob->k1 = pspGetK1();
        ret = sceKernelSetEventFlag(iob->asyncEvFlag, 3);
        // 51C8
        pspSetK1(oldK1);
        return ret;
    }
    if (iob->hook.arg == NULL) {
        // 5140
        ret = iob->dev->drv->funcs->IoLseek(iob, offset, whence);
    }
    else
        ret = iob->hook.arg->hook->funcs->Lseek(&iob->hook, offset, whence);
    // 5114
    pspSetK1(oldK1);
    return ret;
}


int do_ioctl(SceUID fd, unsigned int cmd, void *indata, int inlen, void *outdata, int outlen, int async)
{
    dbg_printf("Calling %s\n", __FUNCTION__);
    SceIoIob *iob;
    int oldK1 = pspShiftK1();
    if (!pspK1DynBufOk(indata, inlen) || !pspK1DynBufOk(outdata, outlen))
    {
        // 5438
        pspSetK1(oldK1);
        return 0x800200D3;
    }
    if (inlen < 0 && ((cmd >> 15) & 1) != 0)
    {
        // 542C
        pspSetK1(oldK1);
        return 0x800200D1;
    }
    // 52B0
    int ret = validate_fd(fd, 0, 2, 0, &iob);
    if (ret < 0) {
        pspSetK1(oldK1);
        return ret;
    }
    if (iob->dev->drv->funcs->IoIoctl == NULL) {
        pspSetK1(oldK1);
        return 0x80020325;
    }
    if (async)
    {
        iob->asyncArgs[0] = cmd;
        // 536C
        iob->asyncArgs[1] = (int)indata;
        iob->asyncArgs[2] = inlen;
        iob->asyncArgs[3] = (int)outdata;
        iob->asyncArgs[4] = outlen;
        if (iob->asyncThread == 0)
        {
            ret = create_async_thread(iob);
            if (ret < 0) {
                pspSetK1(oldK1);
                return ret;
            }
        }
        // 53B0
        if (sceKernelPollSema(iob->asyncSema, 1) < 0) {
            pspSetK1(oldK1);
            return 0x80020329;
        }
        if (iob->asyncPrio < 0 && sceKernelGetCompiledSdkVersion() >= 0x04020000) // 53F0
            sceKernelChangeThreadPriority(iob->asyncThread, 0);
        // 53D0
        iob->asyncCmd = 6;
        iob->k1 = pspGetK1();
        ret = sceKernelSetEventFlag(iob->asyncEvFlag, 3);
        pspSetK1(oldK1);
        return ret;
    }
    if (iob->hook.arg == NULL) {
        // 5348
        ret = iob->dev->drv->funcs->IoIoctl(iob, cmd, indata, inlen, outdata, outlen);
    }
    else
        ret = iob->hook.arg->hook->funcs->Ioctl(&iob->hook, cmd, indata, inlen, outdata, outlen);
    // 531C
    pspSetK1(oldK1);
    return ret;
}

// 5448
int xx_dir(const char *path, SceMode mode, int action) // action: 0 = chdir, 1 = mkdir, 2 = rmdir
{
    dbg_printf("Calling %s\n", __FUNCTION__);
    int ret;
    char str[32];
    SceIoDeviceArg *dev;
    int fsNum;
    char *dirName;
    SceIoIob *iob;
    int oldK1 = pspShiftK1();
    if (!pspK1PtrOk(path))
    {
        // 5684
        pspSetK1(oldK1);
        return 0x800200D3;
    }
    char *pathBuf = alloc_pathbuf();
    if (pathBuf == NULL)
    {
        // 5674
        pspSetK1(oldK1);
        return 0x80020190;
    }
    ret = alloc_iob(&iob, 0);
    if (ret < 0)
        goto freepath;

    dirName = pathBuf;
    ret = sub_375C(path, &dev, &fsNum, &dirName);
    if (ret < 0)
        goto freeiob;

    ret = init_iob(iob, ret, dev, 0x1000000, fsNum);
    if (ret < 0)
        goto freeiob;

    switch (action)
    {
    case 1: // mkdir
        // 5640
        if (dev->drv->funcs->IoMkdir != NULL)
            ret = dev->drv->funcs->IoMkdir(iob, dirName, mode);
        else
            ret = 0x80020325;
        break;
    case 0: // chdir
        // 5580
        snprintf(str, 32, "%s%d:", dev->drv->name, fsNum);
        char **ptr = sceKernelGetKTLS(g_ktls);
        if (ptr == NULL) {
            ret = 0x80020190;
            goto freeiob;
        }

        if (*ptr != NULL) {
            // 5630
            sceKernelFreeHeapMemory(g_heap, *ptr);
        }
        // 55D4
        int size = strlen(dirName) + strlen(str) + 1;
        char *curPath = sceKernelAllocHeapMemory(g_heap, size);
        *ptr = curPath;
        if (curPath != NULL) {
            snprintf(curPath, size, "%s%s", str, dirName);
            ret = 0;
        }
        else
            ret = 0x80020190;
        break;
    case 2: // rmdir
        // 5554
        if (dev->drv->funcs->IoRmdir != NULL)
            ret = dev->drv->funcs->IoRmdir(iob, pathBuf);
        else
            ret = 0x80020325;
        break;
    default:
        // 550C
        ret = 0x80020325;
        break;
    }

    freeiob:
    // 5510
    free_iob(iob);

    freepath:
    // 5518
    if (pathBuf != NULL)
        free_pathbuf(pathBuf);
    pspSetK1(oldK1);
    return ret;
}

int xx_stat(const char *file, SceIoStat *stat, int bits, int get)
{
    dbg_printf("Calling %s\n", __FUNCTION__);
    SceIoIob *iob;
    SceIoDeviceArg *dev;
    int fsNum;
    char *dirNamePtr;
    int oldK1 = pspShiftK1();
    if (!pspK1PtrOk(file) || !pspK1PtrOk(stat))
    {
        // 5824
        pspSetK1(oldK1);
        return 0x800200D3;
    }
    char *buf = alloc_pathbuf();
    if (buf == NULL)
    {
        // 5814
        pspSetK1(oldK1);
        return 0x80020190;
    }
    int ret = alloc_iob(&iob, 0);
    if (ret < 0)
        goto end;
    dirNamePtr = buf;
    ret = sub_375C(file, &dev, &fsNum, &dirNamePtr);
    if (ret < 0)
        goto freeiob;
    ret = init_iob(iob, ret, dev, 0x1000000, fsNum);
    if (ret < 0)
        goto freeiob;
    if (get == 0)
    {
        // 57E0
        ret = 0x80020325;
        if (dev->drv->funcs->IoChstat != NULL)
            ret = dev->drv->funcs->IoChstat(iob, dirNamePtr, stat, bits);
    }
    else if (get == 1)
    {
        // 57AC
        ret = 0x80020325;
        if (dev->drv->funcs->IoGetstat != NULL)
            ret = dev->drv->funcs->IoGetstat(iob, dirNamePtr, stat);
    }
    else
        ret = 0x80020325;

    freeiob:
    free_iob(iob);

    end:
    if (buf != NULL)
        free_pathbuf(buf);
    pspSetK1(oldK1);
    return ret;
}

int do_devctl(const char *dev, unsigned int cmd, void *indata, int inlen, void *outdata, int outlen)
{
    dbg_printf("Calling %s\n", __FUNCTION__);
    SceIoIob *iob;
    SceIoDeviceArg *arg;
    int fsNum;
    char *path;
    char *buf = alloc_pathbuf();
    if (buf == NULL)
        return 0x80020190;
    int ret = alloc_iob(&iob, 0);
    if (ret < 0)
        goto end;
    path = buf;
    ret = sub_3778(dev, &arg, &fsNum, &path, iob->userMode);
    if (ret < 0)
        goto freeiob;
    ret = init_iob(iob, ret, arg, 0x1000000, fsNum);
    if (ret < 0)
        goto freeiob;
    ret = 0x80020325;
    if (arg->drv->funcs->IoDevctl != NULL) {
        // 5948
        ret = arg->drv->funcs->IoDevctl(iob, path, cmd, indata, inlen, outdata, outlen);
    }

    freeiob:
    free_iob(iob);

    end:
    if (buf != NULL)
        free_pathbuf(buf);
    return ret;
}

int do_deldrv(SceIoDeviceArg *dev)
{
    dbg_printf("Calling %s\n", __FUNCTION__);
    int oldIntr = sceKernelCpuSuspendIntr();
    SceSysmemUIDControlBlock *cur = g_uid_type->parent;
    // 59C4
    while (cur != g_uid_type)
    {
        SceIoIob *iob;
        if (validate_fd(cur->UID, 0, 0xFF, 13, &iob) == 0)
        {
            if (iob->dev == dev)
            {
                // 5A30
                if (iob->hook.arg != NULL)
                    iob->hook.funcs = &deleted_dt_func;
                // 5A3C
                iob->dev = &deleted_device;
                if (iob->userMode != 0 && iob->userLevel < 4 && (dev->drv->dev_type & 0xFF0000) != 0)
                    dev->openedFiles--;
            }
        }
        // 59F0
        cur = cur->parent;
        // 59F4
    }
    // 59FC
    sceKernelCpuResumeIntr(oldIntr);
    return 0;
}

int _nulldev()
{
    dbg_printf("Calling %s\n", __FUNCTION__);
    return 0;
}

s64 _nulldev_offt()
{
    dbg_printf("Calling %s\n", __FUNCTION__);
    return 0;
}

int _nulldev_write(SceIoIob *iob __attribute__((unused)), const char *data, int len)
{
    dbg_printf("Calling %s\n", __FUNCTION__);
    if (sceKernelDipsw(59) == 1) {
        // 5ADC
        sceKernelDebugWrite(0, data, len);
    }
    return len;
}

int iob_do_initialize(SceSysmemUIDControlBlock *cb, int funcid, void *arg1, void *arg2, void *arg3, void *arg4, void *arg5, void *arg6)
{
    dbg_printf("Calling %s\n", __FUNCTION__);
    sceKernelCallUIDObjCommonFunction(cb, funcid, arg1, arg2, arg3, arg4, arg5, arg6);
    return cb->UID;
}

int iob_do_delete(SceSysmemUIDControlBlock *cb, int funcid, void *arg1, void *arg2, void *arg3, void *arg4, void *arg5, void *arg6)
{
    dbg_printf("Calling %s\n", __FUNCTION__);
    sceKernelCallUIDObjCommonFunction(cb, funcid, arg1, arg2, arg3, arg4, arg5, arg6);
    return 0;
}

int async_loop(SceSize args __attribute__((unused)), void *argp)
{
    dbg_printf("Calling %s\n", __FUNCTION__);
    SceIoIob *iob = *(SceIoIob**)argp;
    do
    {
        u32 bits;
        if (sceKernelWaitEventFlag(iob->asyncEvFlag, 2, 33, &bits, 0) != 0)
            return 0;
        s64 ret = 0x80020323;
        pspSetK1(iob->k1);
        switch (iob->asyncCmd)
        {
        case 0:
            // 5C0C
            iob->unk050 = 1;
            ret = 0;
            break;

        case 1:
            // 5BA4
            ret = open_main(iob);
            if (ret < 0)
                iob->unk050 = 1;
            break;

        case 2:
            // 5C24
            if (iob->hook.arg == NULL) {
                // 5C5C
                ret = iob->dev->drv->funcs->IoClose(iob);
            }
            else
                ret = iob->hook.arg->hook->funcs->Close(&iob->hook);
            // 5C40
            if (ret >= 0)
                iob->unk050 = 1;
            break;

        case 3:
            // 5C70
            if (iob->hook.arg == NULL) {
                // 5CA8
                ret = iob->dev->drv->funcs->IoRead(iob, (void*)iob->asyncArgs[0], iob->asyncArgs[1]);
            }
            else
                ret = iob->hook.arg->hook->funcs->Read(&iob->hook, (void*)iob->asyncArgs[0], iob->asyncArgs[1]);
            break;

        case 4:
            // 5CC4
            if (iob->hook.arg == NULL) {
                // 5CEC
                ret = iob->dev->drv->funcs->IoWrite(iob, (void*)iob->asyncArgs[0], iob->asyncArgs[1]);
            }
            else
                ret = iob->hook.arg->hook->funcs->Write(&iob->hook, (void*)iob->asyncArgs[0], iob->asyncArgs[1]);
            break;

        case 5:
            // 5D04
            if (iob->hook.arg == NULL) {
                // 5D40
                ret = iob->dev->drv->funcs->IoLseek(iob, ((s64)iob->asyncArgs[1] << 32) | iob->asyncArgs[0], iob->asyncArgs[2]);
            }
            else
                ret = iob->hook.arg->hook->funcs->Lseek(&iob->hook, ((s64)iob->asyncArgs[1] << 32) | iob->asyncArgs[0], iob->asyncArgs[2]);
            // 5D2C
            break;

        case 6:
            // 5D60
            if (iob->hook.arg == NULL) {
                // 5DA0
                ret = iob->dev->drv->funcs->IoIoctl(iob, iob->asyncArgs[0], (void*)iob->asyncArgs[1], iob->asyncArgs[2], (void*)iob->asyncArgs[3], iob->asyncArgs[4]);
            }
            else
                ret = iob->hook.arg->hook->funcs->Ioctl(&iob->hook, iob->asyncArgs[0], (void*)iob->asyncArgs[1], iob->asyncArgs[2], (void*)iob->asyncArgs[3], iob->asyncArgs[4]);
            // 5D90
            break;

        default:
            break;
        }
        // (5BB8)
        // 5BBC
        iob->asyncRet = ret;
        if (iob->asyncCb > 0)
            sceKernelNotifyCallback(iob->asyncCb, (int)iob->asyncCbArgp);
        // 5BD8
    } while (sceKernelSetEventFlag(iob->asyncEvFlag, 4) == 0);
    return 0;
}

