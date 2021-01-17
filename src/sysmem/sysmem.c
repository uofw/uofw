#include <common_imp.h>

#include <sysmem_kdebug.h>
#include <sysmem_kernel.h>
#include <sysmem_suspend_kernel.h>

#include "kdebug.h"
#include "intr.h"
#include "memory.h"
#include "memoryop.h"
#include "misc.h"
#include "partition.h"
#include "suspend.h"
#include "sysevent.h"
#include "uid.h"

#include "sysmem.h"

SCE_MODULE_INFO("sceSystemMemoryManager", SCE_MODULE_KERNEL | SCE_MODULE_ATTR_CANT_STOP | SCE_MODULE_ATTR_EXCLUSIVE_LOAD
                                         | SCE_MODULE_ATTR_EXCLUSIVE_START, 1, 17);
SCE_MODULE_BOOTSTART("SysMemInit");
SCE_SDK_VERSION(SDK_VERSION);

extern SceResidentLibraryEntryTable __library_exports[10];

// 14608?
extern char end;

// 14544
u32 SystemStatus;

// 14548
SceKernelDeci2Ops *g_pDeci2pOps;

// 1454C
SceKernelUsersystemLibWork g_UsersystemLibWork;

// 1455C
SceSysmemMemoryBlock memblk_kernel;

// 14568
SceSysmemMemoryBlock memblk_memman;

// 14574
u32 SysmemRegSave[12];

typedef struct {
    u32 addr; // 0
    u32 size; // 4
    u32 attr; // 8
    SceUID partId; // 12
} SceMemoryProtect;

typedef struct {
    u32 unk0;
    u32 unk4; // 4
    u32 unk8; // 8
    u32 unk12; // 12
    u32 unk16; // 16
    u32 model; // 20
    u32 unk24; // 24
    u32 unk28, unk32, unk36, unk40, unk44, unk48, unk52, unk56;
    u32 dipsLo; // 60
    u32 dipsHi; // 64
    u32 cpTime; // 68
    u32 nBlocks; // 72
    SceMemoryProtect *memBlocks; // 76
    void (*debugPutchar)(short*, int); // 80
    u32 unk84;
    SysMemThreadConfig *threadConf; // 88
    u32 unk92;
    u32 realMemSize; // 96
    u32 randomValue; // 100
} SysMemConfig;

int SysMemInitMain(SysMemConfig *config, SceSysmemMemoryPartition *mainPart, SceSysmemPartTable *table);
int SysMemPostInit(SceSysmemMemoryPartition *mainPart, SceSysmemPartTable *partTable);
int SysMemReInit(SceSysmemMemoryPartition *mainPart);

void SetMemoryPartitionTable(SysMemConfig *config, SceSysmemPartTable *table);
s32 suspendSysmem(int unk __attribute__((unused)), void *param __attribute__((unused)));
s32 resumeSysmem(int unk __attribute__((unused)), void *param __attribute__((unused)));

int SysMemInit(SceSize argSize __attribute__((unused)), const void *argBlock)
{
    SysMemConfig *config = (SysMemConfig *)argBlock;
    SceSysmemMemoryPartition mainPart;
    SceSysmemPartTable table;
    sceKernelRegisterDebugPutcharByBootloader(config->debugPutchar);
    DipswInit(config->dipsLo, config->dipsHi, config->cpTime);
    g_MemInfo.realMemSize = config->realMemSize;
    gSysmemModel = config->model;
    SysMemInitMain(config, &mainPart, &table);
    InitUid();
    SysMemPostInit(&mainPart, &table);
    sceKernelSuspendInit();
    sceKernelSysEventInit();
    InitGameInfo();
    sub_A1E8();
    // 10B54
    u32 i;
    for (i = 0; i < config->nBlocks; i++) {
        SceMemoryProtect *curBlock = &config->memBlocks[i];
        if ((curBlock->attr & 0xFFFF) == 0x20) {
            // 10CFC
            CopyGameInfo((void *)curBlock->addr);
        } else if ((curBlock->attr & 0xFFFF) == 0x200) {
            // 10CD4
            SceUID id = sceKernelAllocPartitionMemory(1, "SceSysmemProtectSystem", 2, curBlock->size, curBlock->addr);
            curBlock->partId = id;
            if (id >= 0)
                curBlock->attr |= 0x10000;
            // 10CF4
        }
        // 10B78
    }
    // 10B88
    SysMemThreadConfig *threadConf = config->threadConf;
    g_1453C = 0xFFFFFFFF;
    gSysmemRandomValue = config->randomValue;
    threadConf->kernelLibs[2] = &__library_exports[5]; // 0x11EE0 sceSuspendForKernel
    threadConf->kernelLibs[3] = &__library_exports[4]; // 0x11ED0 sceSysEventForKernel
    threadConf->kernelLibs[5] = &__library_exports[8]; // 0x11F18 UtilsForKernel
    threadConf->numKernelLibs = 6;
    threadConf->userLibs[0] = &__library_exports[2]; // 0x11EA8 SysMemUserForUser
    threadConf->userLibs[1] = &__library_exports[6]; // 0x11EF4 sceSuspendForUser
    threadConf->ResizeMemoryBlock = sceKernelResizeMemoryBlock;
    threadConf->kernelLibs[4] = &__library_exports[3]; // 0x11EBC SysclibForKernel
    threadConf->kernelLibs[0] = &__library_exports[1]; // 0x11E94 SysMemForKernel
    threadConf->kernelLibs[1] = &__library_exports[9]; // 0x11F2C KDebugForKernel
    threadConf->userLibs[3] = &__library_exports[7]; // 0x11F04 UtilsForUser
    threadConf->numExportLibs = 9;
    threadConf->AllocPartitionMemory = sceKernelAllocPartitionMemory;
    threadConf->GetBlockHeadAddr = sceKernelGetBlockHeadAddr;
    threadConf->initThreadStack = sceKernelGetBlockHeadAddr(sceKernelAllocPartitionMemory(1, "stack:SceSysmemInitialThread", 1, 0x4000, 0)) + 0x4000;
    sceKernelRegisterSuspendHandler(31, suspendSysmem, 0);
    sceKernelRegisterResumeHandler(31, resumeSysmem, 0);
    threadConf->Kprintf = Kprintf;
    return 0;
}

int SysMemInitMain(SysMemConfig *config, SceSysmemMemoryPartition *mainPart, SceSysmemPartTable *table)
{
    table->unk4 = config->unk12;
    table->unk8 = config->unk16;
    SetMemoryPartitionTable(config, table);
    SceSysmemCtlBlk *ctlBlk = (SceSysmemCtlBlk*)(((u32)&end + 0xFF) & 0xFFFFFF00);
    if (table->memSize >= 0xFFFFFF00)
        table->memSize = 0xFFFFFF00;

    // 10D6C
    MemoryProtectInit(table->other1.size + table->other2.size, table->extSc2Kernel.size + table->extScKernel.size + table->extMeKernel.size);
    g_MemInfo.memSize = table->memSize;
    g_MemInfo.unk64 = 0;
    g_MemInfo.memAddr = config->unk4;
    SceSysmemPartInfo *info;
    if (table->unk4 != 3)
        info = &table->other2;
    else
        info = &table->other1;
    mainPart->attr = (mainPart->attr & 0xFFFFFFF0) | 0xC;
    mainPart->ctlBlkCount = 1;
    g_MemInfo.kernel = mainPart;
    mainPart->firstCtlBlk = ctlBlk;
    mainPart->addr = info->addr;
    mainPart->size = info->size;
    mainPart->lastCtlBlk = ctlBlk;
    if (((info->addr + info->size) & 0x1FFFFFFF) >= (((u32)ctlBlk + 0x100) & 0x1FFFFFFF)) {
        // 10E38
        return SysMemReInit(mainPart);
    } else
        mainPart->firstCtlBlk = NULL;
    return 0;
}

int SysMemReInit(SceSysmemMemoryPartition *mainPart)
{
    int oldIntr = suspendIntr();
    if (mainPart->firstCtlBlk != NULL) {
        // 10E9C
        g_MemInfo.main = mainPart;
        PartitionInit(mainPart);
        int size = ((u32)mainPart->firstCtlBlk & 0x1FFFFFFF) - (mainPart->addr & 0x1FFFFFFF);
        void *addr = _AllocPartitionMemory(mainPart, 2, size, mainPart->addr);
        if (addr == NULL) {
            // 10F78
            mainPart->firstCtlBlk = NULL;
        } else {
            memblk_kernel.size = (size + 0xFF) & 0xFFFFFF00;
            memblk_kernel.addr = addr;
            addr = _AllocPartitionMemory(mainPart, 2, 0x100, (u32)mainPart->firstCtlBlk);
            if (addr != NULL) {
                if (((u32)addr & 0x1FFFFFFF) == ((u32)mainPart->firstCtlBlk & 0x1FFFFFFF)) {
                    memblk_memman.size = 0x100;
                    memblk_memman.addr = addr;
                    resumeIntr(oldIntr);
                    return mainPart->lastCtlBlk->segs[mainPart->lastCtlBlk->freeSeg].offset << 8;
                }
                // 10F60
                mainPart->firstCtlBlk = NULL;
            }
            // 10F64
            resumeIntr(oldIntr);
            return 0x80020001;
        }
    }
    // 10E78
    resumeIntr(oldIntr);
    return 0;
}

int SysMemPostInit(SceSysmemMemoryPartition *mainPart, SceSysmemPartTable *partTable)
{
    int oldIntr = suspendIntr();
    SceSysmemUidCB *uidKernel, *uidOther, *uidVsh, *uidScUser, *uidMeUser,
                 *uidExtScKernel, *uidExtSc2Kernel, *uidExtMeKernel, *uidExtVsh;
    PartitionServiceInit();
    sceKernelCreateUID(g_PartType, "SceMyKernelPartition", (pspGetK1() >> 31) & 0xFF, &uidKernel);
    SceSysmemMemoryPartition *kernelPart = UID_CB_TO_DATA(uidKernel, g_PartType, SceSysmemMemoryPartition);
    SceSysmemMemoryPartition *part;
    kernelPart->next = NULL;
    kernelPart->addr = mainPart->addr;
    kernelPart->size = mainPart->size;
    SceSysmemPartInfo *info = &partTable->other2;
    kernelPart->attr = (kernelPart->attr & 0xFFFFFFF0) | (mainPart->addr & 0xF);
    g_MemInfo.kernel = kernelPart;
    kernelPart->firstCtlBlk = mainPart->firstCtlBlk;
    kernelPart->ctlBlkCount = mainPart->ctlBlkCount;
    kernelPart->lastCtlBlk = mainPart->firstCtlBlk;
    g_MemInfo.main = kernelPart;
    MemoryBlockServiceInit();
    sceKernelCreateUID(g_MemBlockType, "sceSystemMemoryManager", (pspGetK1() >> 31) & 0xFF, &uidKernel);
    SceSysmemMemoryBlock *memBlock = UID_CB_TO_DATA(uidKernel, g_MemBlockType, SceSysmemMemoryBlock);
    memblk_kernel.part = kernelPart;
    memBlock->addr = memblk_kernel.addr;
    memBlock->size = memblk_kernel.size;
    memBlock->part = memblk_kernel.part;
    sceKernelProtectMemoryBlock(memblk_kernel.part, memblk_kernel.addr);
    sceKernelCreateUID(g_MemBlockType, "SceSystemBlock", (pspGetK1() >> 31) & 0xFF, &uidKernel);
    memBlock = UID_CB_TO_DATA(uidKernel, g_MemBlockType, SceSysmemMemoryBlock);
    memblk_memman.part = kernelPart;
    memBlock->addr = memblk_memman.addr;
    memBlock->size = memblk_memman.size;
    memBlock->part = memblk_memman.part;
    sceKernelProtectMemoryBlock(memblk_memman.part, memblk_memman.addr);
    if (partTable->unk4 != 3)
        info = &partTable->other1;
    sceKernelGetUIDcontrolBlockWithType(sceKernelCreateMemoryPartition("SceOtherKernelPartition", 12, info->addr, info->size), g_PartType, &uidOther);
    part = NULL;
    if (info->size != 0)
        part = UID_CB_TO_DATA(uidOther, g_PartType, SceSysmemMemoryPartition);
    // 11160
    if (partTable->unk4 == 3) {
        // 11450
        g_MemInfo.other2 = part;
        g_MemInfo.other1 = kernelPart;
    } else {
        g_MemInfo.other2 = kernelPart;
        g_MemInfo.other1 = part;
    }
    // 11178
    info = &partTable->vshell;
    sceKernelGetUIDcontrolBlockWithType(sceKernelCreateMemoryPartition("SceVshellPartition", 15, info->addr, info->size), g_PartType, &uidVsh);
    part = NULL;
    if (info->size != 0)
        part = UID_CB_TO_DATA(uidVsh, g_PartType, SceSysmemMemoryPartition);
    // 111C8
    g_MemInfo.vshell = part;
    info = &partTable->scUser;
    sceKernelGetUIDcontrolBlockWithType(sceKernelCreateMemoryPartition("SceScUserPartition", 15, info->addr, info->size), g_PartType, &uidScUser);
    part = NULL;
    if (info->size != 0)
        part = UID_CB_TO_DATA(uidScUser, g_PartType, SceSysmemMemoryPartition);
    // 1121C
    g_MemInfo.scUser = part;
    info = &partTable->meUser;
    sceKernelGetUIDcontrolBlockWithType(sceKernelCreateMemoryPartition("SceMeUserPartition", 15, info->addr, info->size), g_PartType, &uidMeUser);
    part = NULL;
    if (info->size != 0)
        part = UID_CB_TO_DATA(uidMeUser, g_PartType, SceSysmemMemoryPartition);
    // 11270
    g_MemInfo.meUser = part;
    if (partTable->unk8 == 6) {
        // 11448
        g_MemInfo.user = g_MemInfo.scUser;
    } else
        g_MemInfo.user = g_MemInfo.meUser;
    // 11288
    info = &partTable->extScKernel;
    sceKernelGetUIDcontrolBlockWithType(sceKernelCreateMemoryPartition("SceExtScKernelPartition", 12, info->addr, info->size), g_PartType, &uidExtScKernel);
    part = NULL;
    if (info->size != 0)
        part = UID_CB_TO_DATA(uidExtScKernel, g_PartType, SceSysmemMemoryPartition);
    // 112DC
    g_MemInfo.extScKernel = part;
    info = &partTable->extSc2Kernel;
    sceKernelGetUIDcontrolBlockWithType(sceKernelCreateMemoryPartition("SceExtSc2KernelPartition", 12, info->addr, info->size), g_PartType, &uidExtSc2Kernel);
    part = NULL;
    if (info->size != 0)
        part = UID_CB_TO_DATA(uidExtSc2Kernel, g_PartType, SceSysmemMemoryPartition);
    // 11330
    g_MemInfo.extSc2Kernel = part;
    info = &partTable->extMeKernel;
    sceKernelGetUIDcontrolBlockWithType(sceKernelCreateMemoryPartition("SceExtMeKernelPartition", 12, info->addr, info->size), g_PartType, &uidExtMeKernel);
    part = NULL;
    if (info->size != 0)
        part = UID_CB_TO_DATA(uidExtMeKernel, g_PartType, SceSysmemMemoryPartition);
    // 11384
    g_MemInfo.extMeKernel = part;
    if (partTable->unk4 == 3) {
        // 11440
        g_MemInfo.extKernel = g_MemInfo.extScKernel;
    } else
        g_MemInfo.extKernel = g_MemInfo.extMeKernel;
    // 1139C
    info = &partTable->extVshell;
    if (g_MemInfo.extKernel == NULL)
        g_MemInfo.extKernel = g_MemInfo.kernel;
    // 113B8
    sceKernelGetUIDcontrolBlockWithType(sceKernelCreateMemoryPartition("SceExtVshellPartition", 12, info->addr, info->size), g_PartType, &uidExtVsh);
    part = NULL;
    if (info->size != 0)
        part = UID_CB_TO_DATA(uidExtVsh, g_PartType, SceSysmemMemoryPartition);
    // 11404
    g_MemInfo.extVshell = part;
    resumeIntr(oldIntr);
    return 0;
}

void sceKernelGetSysMemoryInfo(s32 mpid, u32 needsInit, SceSysMemoryInfo *info)
{
    SceSysmemMemoryPartition *part = MpidToCB(mpid);
    s32 oldIntr = suspendIntr();
    if (!needsInit) {
        // 114F4
        SceSysmemSeg *seg = info->curSeg;
        if (seg != NULL) {
            // 11514
            u32 off = seg->offset << 8;
            u32 size = seg->size << 8;
            info->size = size;
            info->info.segAddr = part->addr + off;
            if (seg->used == 0) {
                // 115A8 dup
                info->size |= 1;
            } else {
                SceSysmemCtlBlk *curCtlBlk = part->firstCtlBlk;
                // 11550
                while (curCtlBlk != NULL) {
                    if ((u32)curCtlBlk == part->addr + off) {
                        // 115A4
                        // 115A8 dup
                        info->size |= 2;
                        break;
                    }
                    curCtlBlk = curCtlBlk->next;
                }
            }
            // (11564)
            // 11568
            SceSysmemCtlBlk *ctlBlk = (SceSysmemCtlBlk *)((u32)seg & 0xFFFFFF00);
            if (seg->next != 0x3F) {
                // 114CC dup
                info->curSeg = &ctlBlk->segs[seg->next];
            } else if (ctlBlk->next != NULL) {
                // 11598
                // 114CC dup
                info->curSeg = &part->firstCtlBlk->segs[part->firstCtlBlk->firstSeg];
            } else
                info->curSeg = NULL;
        } else {
            info->size = 1;
            info->info.segAddr = part->addr;
        }
    } else {
        info->info.segCount = 0;
        SceSysmemCtlBlk *curCtlBlk = part->firstCtlBlk;
        if (curCtlBlk != NULL) {
            s32 count = 0;
            // 114A4
            do {
                count += curCtlBlk->segCount;
                curCtlBlk = curCtlBlk->next;
            } while (curCtlBlk != NULL);
            info->info.segCount = count;
        }
        // 114B8
        info->size = part->size;
        // 114CC dup
        info->curSeg = &part->firstCtlBlk->segs[part->firstCtlBlk->firstSeg];
    }
    // 114D4
    resumeIntr(oldIntr);
}

s32 sceKernelGetSysmemIdList(s32 id, s32 *uids, s32 maxCount, s32 *totalCount)
{
    switch (id) {
    case 0:
    case 1:
    case 2:
        break;
    default:
        // 115FC
        return 0x800201BB;
    }
    // 11624
    SceSysmemUidCB *heapUid = g_HeapType;
    // 11628
    s32 oldK1 = pspShiftK1();
    if (!pspK1DynBufOk(uids, maxCount * 4) || !pspK1StaBufOk(totalCount, 4)) {
        // 11660
        pspSetK1(oldK1);
        return 0x800200D3;
    }
    // 11670
    s32 storedIdCount = 0;
    s32 oldIntr = suspendIntr();
    SceSysmemUidCB *curUid = heapUid->PARENT0;
    s32 idCount = 0;
    if (curUid != heapUid) {
        // 11690
        do {
            u8 size = curUid->size;
            if (!pspK1IsUserMode())
                size = 0xFF;
            if (size != 0) {
                // 116C8
                idCount++;
                if (storedIdCount < maxCount) {
                    storedIdCount++;
                    *(uids++) = curUid->uid;
                }
            }
            // 116A0
            curUid = curUid->PARENT0;
        } while (curUid != heapUid);
    }
    // 116AC
    if (totalCount != NULL)
        *totalCount = idCount;
    // 116B4
    resumeIntr(oldIntr);
    pspSetK1(oldK1);
    return storedIdCount;
}

s32 sceKernelSysMemRealMemorySize(void)
{
    return g_MemInfo.realMemSize;
}

s32 sceKernelSysMemMemSize(void)
{
    if (MpidToCB(1)->firstCtlBlk == NULL)
        return 0;
    return g_MemInfo.memSize;
}

s32 sceKernelSysMemMaxFreeMemSize(void)
{
    u32 maxSize = 0;
    s32 oldIntr = suspendIntr();
    if (MpidToCB(1)->firstCtlBlk == NULL) {
        // 117BC
        resumeIntr(oldIntr);
        return 0;
    }
    SceSysmemMemoryPartition *cur = g_MemInfo.main;
    // 11780
    while (cur != NULL) {
        u32 curMax = PartitionQueryMaxFreeMemSize(cur);
        if (curMax > maxSize)
            maxSize = curMax;
        cur = cur->next;
    }
    // 11798
    resumeIntr(oldIntr);
    return maxSize;
}

s32 sceKernelSysMemTotalFreeMemSize(void)
{
    s32 totalSize = 0;
    s32 oldIntr = suspendIntr();
    if (MpidToCB(1)->firstCtlBlk == NULL) {
        // 11844
        resumeIntr(oldIntr);
        return 0;
    }
    SceSysmemMemoryPartition *cur = g_MemInfo.main;
    // 1180C
    while (cur != NULL) {
        totalSize += PartitionQueryTotalFreeMemSize(cur);
        cur = cur->next;
    }
    // 11820
    resumeIntr(oldIntr);
    return totalSize;
}

int sceKernelDeci2pRegisterOperations(SceKernelDeci2Ops *ops)
{
    g_pDeci2pOps = ops;
    return 0;
}

SceKernelDeci2Ops *sceKernelDeci2pReferOperations(void)
{
    return g_pDeci2pOps;
}

s32 sceKernelGetMEeDramSaveAddr(void)
{
    if (gSysmemModel != 0) {
        // 11894
        if (g_MemInfo.extVshell == 0) {
            // 118A8
            return g_MemInfo.vshell->addr + 0x400000;
        } else
            return g_MemInfo.extVshell->addr;
    } else
        return g_MemInfo.vshell->addr;
}

s32 sceKernelGetAWeDramSaveAddr(void)
{
    if (gSysmemModel != 0) {
        // 118E0
        return g_MemInfo.vshell->addr;
    } else
        return g_MemInfo.vshell->addr + 0x200000;
}

s32 sceKernelGetMEeDramSaveSize(void)
{
    if (gSysmemModel != 0)
        return 0x400000;
    return 0x200000;
}

s32 sceKernelGetAWeDramSaveSize(void)
{
    if (gSysmemModel != 0)
        return 0x400000;
    return 0x200000;
}

s32 sceKernelDevkitVersion(void)
{
    return 0x06060010;
}

s32 sceKernelGetSystemStatus(void)
{
    return SystemStatus;
}

s32 sceKernelSetSystemStatus(s32 newStatus)
{
    return (SystemStatus = newStatus);
}

s32 sceKernelSetUsersystemLibWork(s32 *cmdList, s32 (*sceGeListUpdateStallAddr_lazy)(s32, void*), SceGeLazy *lazy)
{
    s32 oldK1 = pspShiftK1();
    if (!pspK1PtrOk(cmdList) || !pspK1PtrOk(sceGeListUpdateStallAddr_lazy) || !pspK1PtrOk(lazy)) {
        pspSetK1(oldK1);
        return 0x800200D3;
    }
    // 11980
    g_UsersystemLibWork.cmdList = cmdList;
    g_UsersystemLibWork.sceGeListUpdateStallAddr_lazy = sceGeListUpdateStallAddr_lazy;
    g_UsersystemLibWork.lazySyncData = lazy;
    g_UsersystemLibWork.size = sizeof g_UsersystemLibWork;
    pspSetK1(oldK1);
    return 0;
}

SceKernelUsersystemLibWork *sceKernelGetUsersystemLibWork(void)
{
    return &g_UsersystemLibWork;
}

void SetMemoryPartitionTable(SysMemConfig *config, SceSysmemPartTable *table)
{
    int type;
    if (config->model != 0) {
        // 11C08
        table->memSize = 0x04000000;
        type = 2;
        if (config->unk24 != 2)
            type = 3;
    } else {
        if ((config->dipsLo & 0x400) == 0 || config->realMemSize <= 0x037FFFFF) {
            // 11B9C
            if ((config->dipsLo & 0x1000) != 0 || config->realMemSize <= 0x03FFFFFF) {
                type = 0;
                // 11C00
                // 119E0 dup
                table->memSize = 0x02000000;
            }
            table->memSize = 0x04000000;
            switch (config->unk24) { // jump table at 0x13AE8
            case 2:
            case 3:
                // 11BF0
                type = 0;
                break;
            default:
                // 11BF8
                type = 3;
                break;
            }
        } else {
            type = 1;
            // 119E0 dup
            table->memSize = 0x03800000;
        }
    }
    u32 curAddr = config->unk4;
    // 119E4
    table->other1.addr = 0x80000000 | (curAddr & 0x1FFFFFFF);
    // 11A08
    table->other1.size = (type != 2 ? 0x300000 : 0x600000);
    int diff = 0;
    if (config->unk8 != 0) {
        diff = config->unk8 - table->other1.size;
        table->other1.size = config->unk8;
    }

    curAddr += table->other1.size;
    table->other2.addr = 0x80000000 | (curAddr & 0x1FFFFFFF);
    // 11A4C
    table->other2.size = (type != 2 ? 0x100000 : 0x200000);
    curAddr += table->other2.size;
    table->vshell.addr = curAddr & 0x1FFFFFFF;
    if (type == 2) {
        // 11B94
        table->vshell.size = 0;
    } else
        table->vshell.size = 0x400000;

    // 11A70
    curAddr += table->vshell.size;
    table->scUser.addr = curAddr & 0x1FFFFFFF;
    int size;
    switch (type)
    {
    case 1:
    case 2: // 11B8C
        size = 0x3000000;
        break;
    default:
        size = 0x1800000;
        break;
    }
    // 11AA0
    table->scUser.size = size;
    table->meUser.addr = 0;
    curAddr += table->scUser.size - diff;
    table->scUser.size -= diff;
    table->meUser.size = 0;
    if (type == 2) {
        // 11B74
        table->vshell.addr = curAddr & 0x1FFFFFFF;
        curAddr += 0x800000;
        table->vshell.size = 0x800000;
    }
    // 11AC4
    if (type == 3) {
        // 11B5C
        table->extSc2Kernel.addr = 0x80000000 | (curAddr & 0x1FFFFFFF);
        table->extSc2Kernel.size = 0x1800000;
    } else {
        table->extSc2Kernel.addr = 0;
        table->extSc2Kernel.size = 0;
    }
    // 11AD8
    curAddr += table->extSc2Kernel.size;
    if (type == 3) {
        // 11B40
        table->extScKernel.addr = 0x80000000 | (curAddr & 0x1FFFFFFF);
        table->extScKernel.size = 0x400000;
    } else {
        table->extScKernel.addr = 0;
        table->extScKernel.size = 0;
    }
    // 11AF0
    table->extMeKernel.addr = 0;
    curAddr += table->extScKernel.size;
    table->extMeKernel.size = 0;
    if (type == 3 && (config->dipsLo & 0x1000) == 0) { // 11B14
        table->extVshell.size = 0x400000;
        table->extVshell.addr = 0x80000000 | (curAddr & 0x1FFFFFFF);
    } else {
        table->extVshell.size = 0;
        // 11B0C
        table->extVshell.addr = 0;
    }
}

s32 suspendSysmem(int unk __attribute__((unused)), void *param __attribute__((unused)))
{
    SysmemRegSave[0]  = HW(0xBC100040);
    SysmemRegSave[1]  = HW(0xBC000000) & 0xFEDCEDCF;
    SysmemRegSave[2]  = HW(0xBC000004) & 0xEDCFCCDD;
    SysmemRegSave[3]  = HW(0xBC000008);
    SysmemRegSave[4]  = HW(0xBC00000C);
    SysmemRegSave[5]  = HW(0xBC000030);
    SysmemRegSave[6]  = HW(0xBC000034);
    SysmemRegSave[7]  = HW(0xBC000038);
    SysmemRegSave[8]  = HW(0xBC00003C);
    SysmemRegSave[9]  = HW(0xBC000040);
    SysmemRegSave[10] = HW(0xBC000044);
    SysmemRegSave[11] = HW(0xBC000048);
    return 0;
}

s32 resumeSysmem(int unk __attribute__((unused)), void *param __attribute__((unused)))
{
    HW(0xBC100040) = SysmemRegSave[0];
    HW(0xBC000000) = SysmemRegSave[1] & 0xCDEFDEFC;
    HW(0xBC000004) = SysmemRegSave[2] & 0xDEFCFFEE;
    HW(0xBC000008) = SysmemRegSave[3];
    HW(0xBC00000C) = SysmemRegSave[4];
    HW(0xBC000030) = SysmemRegSave[5];
    HW(0xBC000034) = SysmemRegSave[6];
    HW(0xBC000038) = SysmemRegSave[7];
    HW(0xBC00003C) = SysmemRegSave[8];
    HW(0xBC000040) = SysmemRegSave[9];
    HW(0xBC000044) = SysmemRegSave[10];
    HW(0xBC000048) = SysmemRegSave[11];
    return 0;
}

s32 *wmemset(s32 *src, s32 c, u32 n)
{
    n = n & 0xFFFFFFFC;
    if (n != 0) {
        s32 *end = (void*)src + n;
        s32 *cur = src;
        if ((n & 4) != 0) {
            *cur = c;
            cur++;
        }
        // 11E14
        while (end != cur) {
            *(cur + 0) = c;
            *(cur + 1) = c;
            cur += 2;
        }
    }
    return src;
}

