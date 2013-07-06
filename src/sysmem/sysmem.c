SCE_MODULE_INFO("sceSystemMemoryManager", SCE_MODULE_KERNEL | SCE_MODULE_ATTR_CANT_STOP | SCE_MODULE_ATTR_EXCLUSIVE_LOAD
                                         | SCE_MODULE_ATTR_EXCLUSIVE_START, 1, 17);
SCE_MODULE_BOOTSTART("SysMemInit");
SCE_SDK_VERSION(SDK_VERSION);

// 14548
void *g_deci2p;

int SysMemInit(int args, void *argp)
{
    SceSysmemMemoryPartition mainPart;
    SceSysmemPartTable table;
    sceKernelRegisterDebugPutcharByBootloader(*(int*)(argp + 80));
    DipswInit(*(int*)(argp + 60), *(int*)(argp + 64), *(int*)(argp + 68));
    g_145C0.realMemSize = *(int*)(argp + 96);
    g_model = *(int*)(argp + 20);
    SysMemInitMain(argp, &mainPart, &table);
    InitUid();
    SysMemPostInit(&mainPart, &table);
    sceKernelSuspendInit();
    sceKernelSysEventInit();
    InitGameInfo();
    sub_A1E8();
    // 10B54
    int i;
    for (i = 0; i < *(int*)(argp + 72); i++)
    {
        void *curPtr = *(int*)(argp + 76) + i * 28;
        short type = *(unsigned short*)(curPtr + 8);
        if (type == 32) {
            // 10CFC
            CopyGameInfo(*(int*)(curPtr + 0));
        }
        else if (type == 512)
        {
            // 10CD4
            SceUID id = sceKernelAllocPartitionMemory(1, "SceSysmemProtectSystem", 2, *(int*)(curPtr + 4), *(int*)(curPtr + 0));
            *(int*)(curPtr + 12) = id;
            if (id >= 0)
                *(int*)(curPtr + 8) |= 0x10000;
            // 10CF4
        }
        // 10B78
    }
    // 10B88
    void *ptr = *(int*)(argp + 88);
    g_1453C = -1;
    g_initialRand = *(int*)(argp + 100);
    *(int*)(ptr + 16) = &g_11EE0;
    *(int*)(ptr + 20) = &g_11ED0;
    *(int*)(ptr + 28) = &g_11F18;
    *(int*)(ptr + 44) = 6;
    *(int*)(ptr + 108) = &g_11EA8;
    *(int*)(ptr + 112) = &g_11EF4;
    *(int*)(ptr + 60) = sceKernelResizeMemoryBlock;
    *(int*)(ptr + 24) = &g_11EBC;
    *(int*)(ptr + 8) = &g_11E94;
    *(int*)(ptr + 12) = &g_11F2C;
    *(int*)(ptr + 116) = &g_11F04;
    *(int*)(ptr + 4) = 9;
    *(int*)(ptr + 52) = sceKernelAllocPartitionMemory;
    *(int*)(ptr + 56) = sceKernelGetBlockHeadAddr;
    *(int*)(ptr + 72) = sceKernelGetBlockHeadAddr(sceKernelAllocPartitionMemory(1, "stack:SceSysmemInitialThread", 1, 0x4000, 0)) + 0x4000;
    sceKernelRegisterSuspendHandler(31, &g_11C28, 0);
    sceKernelRegisterResumeHandler(31, &g_11D0C, 0);
    *(int*)(ptr + 96) = Kprintf;
    return 0;
}

void SysMemInitMain(void *arg0, SceSysmemMemoryPartition *mainPart, SceSysmemPartTable *table)
{
    table->unk4 = *(int*)(arg0 + 12);
    table->unk8 = *(int*)(arg0 + 16);
    SetMemoryPartitionTable(arg0, table);
    SceSysmemCtlBlk *ctlBlk = (&g_14608 + 0xFF) & 0xFFFFFF00;
    if (table->memSize >= 0xFFFFFF00)
        table->memSize = 0xFFFFFF00;

    // 10D6C
    MemoryProtectInit(table->other1.size + table->other2.size, table->extSc2Kernel.size + table->extScKernel.size + table->extMeKernel.size);
    g_145C0.memSize = table->memSize;
    g_145C0.unk64 = 0;
    g_145C0.memAddr = *(int*)(arg0 + 4);
    SceSysmemPartInfo *info;
    if (table->unk4 != 3)
        info = &table->other2;
    else
        info = &table->other1;
    mainPart->unk12 = (mainPart->unk12 & 0xFFFFFFF0) | 0xC;
    mainPart->unk24 = 1;
    g_145C0.kernel = mainPart;
    mainPart->firstCtlBlk = ctlBlk;
    mainPart->addr = info->addr;
    mainPart->size = info->size;
    mainPart->lastCtlBlk = ctlBlk;
    if (((info->addr + info->size) & 0x1FFFFFFF) >= (((void*)ctlBlk + 0xFF) & 0x1FFFFFFF)) {
        // 10E38
        return SysMemReInit(mainPart);
    } else
        mainPart->ctlBlk = NULL;
    return 0;
}

int SysMemReInit(SceSysmemMemoryPartition *mainPart)
{
    int oldIntr = suspendIntr();
    if (mainPart->firstCtlBlk != NULL) {
        // 10E9C
        g_145C0.main = mainPart;
        PartitionInit(mainPart);
        int size = ((u32)mainPart->firstCtlBlk & 0x1FFFFFFF) - (mainPart->addr & 0x1FFFFFFF);
        int addr = _AllocPartitionMemory(mainPart, 2, size, mainPart->addr);
        if (addr == 0) {
            // 10F78
            mainPart->firstCtlBlk = NULL;
        } else {
            *(int*)(&g_1455C + 4) = (size + 0xFF) & 0xFFFFFF00;
            *(int*)(&g_1455C + 0) = addr;
            addr = _AllocPartitionMemory(mainPart, 2, 0x100, mainPart->firstCtlBlk);
            if (addr != 0) {
                if ((addr & 0x1FFFFFFF) == ((u32)mainPart->firstCtlBlk & 0x1FFFFFFF)) {
                    *(int*)(&g_14568 + 4) = 0x100;
                    *(int*)(&g_14568 + 0) = addr;
                    resumeIntr(oldIntr);
                    return (mainPart->lastCtlBlk[mainPart->lastCtlBlk->freeSeg][0] >> 7) << 8;
                }
                // 10F60
                *(int*)(arg0 + 16) = 0;
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

int SysMemPostInit(void *arg0, SceSysmemPartTable *partTable)
{
    int oldIntr = suspendIntr();
    SceSysmemUidCB *uidKernel, *uidOther, *uidVsh, *uidScUser, *uidMeUser,
                 *uidExtScKernel, *uidExtSc2Kernel, *uidExtMe, *uidExtVsh;
    PartitionServiceInit();
    sceKernelCreateUID(g_145A8, "SceMyKernelPartition", (k1 >> 31) & 0xFF, &uidKernel);
    SceSysmemMemoryPartition *kernelPart = UID_CB_TO_DATA(uid, g_145A8, SceSysmemMemoryPartition);
    SceSysmemMemoryPartition *part;
    *(int*)(kernelPart + 0) = 0;
    *(int*)(kernelPart + 4) = *(int*)(arg0 + 4);
    *(int*)(kernelPart + 8) = *(int*)(arg0 + 8);
    SceSysmemPartInfo *info = &partTable->other2;
    *(int*)(kernelPart + 12) = (*(int*)(kernelPart + 12) & 0xFFFFFFF0) | (*(int*)(arg0 + 12) & 0xF);
    g_145C0.kernel = kernelPart;
    *(int*)(kernelPart + 16) = *(int*)(arg0 + 16);
    *(int*)(kernelPart + 24) = *(int*)(arg0 + 24);
    *(int*)(kernelPart + 20) = *(int*)(arg0 + 16);
    g_145C0.main = kernelPart;
    MemoryBlockServiceInit();
    sceKernelCreateUID(g_MemBlockType, "SceSystemMemoryManager", (k1 >> 31) & 0xFF, &uidKernel);
    SceSysmemMemoryBlock *memBlock = UID_CB_TO_DATA(uidKernel, g_MemBlockType, SceSysmemMemoryBlock);
    *(int*)(&g_1455C + 8) = memBlock;
    *(int*)(memBlock + 0) = *(int*)(&g_1455C + 0);
    *(int*)(memBlock + 4) = *(int*)(&g_1455C + 4);
    *(int*)(memBlock + 8) = *(int*)(&g_1455C + 8);
    sceKernelProtectMemoryBlock(*(int*)(&g_1455C + 8), *(int*)(&g_1455C + 0));
    sceKernelCreateUID(g_MemBlockType, "SceSystemBlock", (k1 >> 31) & 0xFF, &uidKernel);
    memBlock = UID_CB_TO_DATA(uidKernel, g_MemBlockType, SceSysmemMemoryBlock);
    *(int*)(&g_14568 + 8) = memBlock;
    *(int*)(memBlock + 0) = *(int*)(&g_14568 + 0);
    *(int*)(memBlock + 4) = *(int*)(&g_14568 + 4);
    *(int*)(memBlock + 8) = *(int*)(&g_14568 + 8);
    sceKernelProtectMemoryBlock(memBlock, *(int*)(&g_14568 + 0));
    if (*(int*)(arg1 + 4) != 3)
        info = &partTable->other1;
    sceKernelGetUIDcontrolBlockWithType(sceKernelCreateMemoryPartition("SceOtherKernelPartition", 12, info->addr, info->size), g_145A8, &uidOther);
    part = NULL;
    if (info->size != 0)
        part = UID_CB_TO_DATA(uidOther, g_145A8, SceSysmemMemoryPartition);
    // 11160
    if (*(int*)(arg1 + 4) == 3) {
        // 11450
        g_145C0.other2 = part;
        g_145C0.other1 = kernelPart;
    } else {
        g_145C0.other2 = kernelPart;
        g_145C0.other1 = part;
    }
    // 11178
    info = &partTable->vshell;
    sceKernelGetUIDcontrolBlockWithType(sceKernelCreateMemoryPartition("SceVshellPartition", 15, info->addr, info->size), g_145A8, &uidVsh);
    part = NULL;
    if (info->size != 0)
        UID_CB_TO_DATA(uidVsh, g_145A8, SceSysmemMemoryPartition);
    // 111C8
    g_145C0.vshell = part;
    info = &partTable->scUser;
    sceKernelGetUIDcontrolBlockWithType(sceKernelCreateMemoryPartition("SceScUserPartition", 15, info->addr, info->size), g_145A8, &uidScUser);
    part = NULL;
    if (info->size != 0)
        part = UID_CB_TO_DATA(uidScUser, g_145A8, SceSysmemMemoryPartition)
    // 1121C
    g_145C0.scUser = part;
    info = &partTable->meUser;
    sceKernelGetUIDcontrolBlockWithType(sceKernelCreateMemoryPartition("SceMeUserPartition", 15, info->addr, info->size), g_145A8, &uidMeUser);
    part = NULL;
    if (info->size != 0)
        part = UID_CB_TO_DATA(uidMeUser, g_145A8, SceSysmemMemoryPartition);
    // 11270
    g_145C0.meUser = part;
    if (*(int*)(arg1 + 8) == 6) {
        // 11448
        g_145C0.user = g_145C0.scUser;
    } else
        g_145C0.user = g_145C0.meUser;
    // 11288
    info = &partTable->extScKernel;
    sceKernelGetUIDcontrolBlockWithType(sceKernelCreateMemoryPartition("SceExtScKernelPartition", 12, info->addr, info->size), g_145A8, &uidExtScKernel);
    part = NULL;
    if (info->size != 0)
        part = UID_CB_TO_DATA(uidExtScKernel, g_145A8, SceSysmemMemoryPartition);
    // 112DC
    g_145C0.extScKernel = part;
    info = &partTable->extSc2Kernel;
    sceKernelGetUIDcontrolBlockWithType(sceKernelCreateMemoryPartition("SceExtSc2KernelPartition", 12, info->addr, info->size), g_145A8, &uidExtSc2Kernel);
    part = NULL;
    if (info->size != 0)
        part = UID_CB_TO_DATA(uidSc2Kernel, g_145A8, SceSysmemMemoryPartition);
    // 11330
    g_145C0.extSc2Kernel = part;
    info = &partTable->extMeKernel;
    sceKernelGetUIDcontrolBlockWithType(sceKernelCreateMemoryPartition("SceExtMeKernelPartition", 12, info->addr, info->size), g_145A8, &uidExtMeKernel);
    part = NULL;
    if (info->size != 0)
        part = UID_CB_TO_DATA(uidExtMeKernel, g_145A8, SceSysmemMemoryPartition);
    // 11384
    g_145C0.extMeKernel = part;
    if (*(int*)(arg1 + 4) == 3) {
        // 11440
        g_145C0.extKernel = g_145C0.extScKernel;
    } else
        g_145C0.extKernel = g_145C0.extMeKernel;
    // 1139C
    info = &partTable->extVshell;
    if (g_145C0.extKernel == NULL)
        g_145C0.extKernel = g_145C0.kernel;
    // 113B8
    sceKernelGetUIDcontrolBlockWithType(sceKernelCreateMemoryPartition("SceExtVshellPartition", 12, info->addr, info->size), g_145A8, &uidExtVsh);
    part = NULL;
    if (info->size != 0)
        part = UID_CB_TO_DATA(uidExtVsh, g_145A8, SceSysmemMemoryPartition);
    // 11404
    g_145C0.extVshell = part;
    resumeIntr(oldIntr);
    return 0;
}
s32 sceKernelGetSysMemoryInfo(s32 mpid, s32 arg1, arg2)
{
    SceSysmemMemoryPartition *part = MpidToCB(mpid);
    s32 oldIntr = suspendIntr();
    if (arg1 == 0) {
        // 114F4
        a2 = *(int*)(arg2 + 12);
        if (a2 != 0)
        {
            // 11514
            t4 = (*(u32*)(a2 + 0) >> 7) << 8;
            v0 = (*(u32*)(a2 + 4) >> 9) << 8;
            *(int*)(arg2 + 4) = v0;
            *(int*)(arg2 + 0) = part->addr + t4;
            if ((*(u32*)(a2 + 0) & 1) == 0) {
                // 115A8 dup
                *(int*)(arg2 + 4) = v0 | 1;
            } else {
                SceSysmemCtlBlk *curCtlBlk = part->ctlBlk;
                // 11550
                while (curCtlBlk != NULL) {
                    if ((u32)curCtlBlk == part->addr + t4) {
                        // 115A4
                        // 115A8 dup
                        *(int*)(arg2 + 4) |= 2;
                        break;
                    }
                    curCtlBlk = curCtlBlk->next;
                }
            }
            // (11564)
            // 11568
            a0 = (a2 & 0xFFFFFF00);
            v1 = (*(int*)(a2 + 0) >> 1) & 0x3F;
            if (v1 != 0x3F) {
                // 114CC dup
                *(int*)(arg2 + 12) = a0 + v1 * 8 + 16;
            } else if (*(int*)(a0 + 0) != 0) {
                // 11598
                // 114CC dup
                *(int*)(arg2 + 12) = &part->ctlBlk->segs[part->ctlBlk->unk14];
            } else
                *(int*)(arg2 + 12) = 0;
        } else {
            *(int*)(arg2 + 4) = 1;
            *(int*)(arg2 + 0) = part->size;
        }
    } else {
        *(int*)(arg2 + 0) = 0;
        SceSysmemCtlBlk *curCtlBlk = part->ctlBlk;
        if (curCtlBlk != NULL) {
            s32 count = 0;
            // 114A4
            do {
                count += curCtlBlk->segCount;
                curCtlBlk = curCtlBlk->next;
            } while (curCtlBlk != NULL);
            *(int*)(arg2 + 0) = count;
        }
        // 114B8
        *(int*)(arg2 + 4) = part->size;
        // 114CC dup
        *(int*)(arg2 + 12) = &part->ctlBlk->segs[part->ctlBlk->unk14];
    }
    // 114D4
    resumeIntr(oldIntr);
    return;
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
    SceSysmemUidCB *heapUid = g_145A4;
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
    return g_145C0.realMemSize;
}

s32 sceKernelSysMemMemSize(void)
{
    if (MpidtoCB(1)->ctlBlk == NULL)
        return 0;
    return g_145C0.memSize;
}

s32 sceKernelSysMemMaxFreeMemSize(void)
{
    u32 maxSize = 0;
    s32 oldIntr = suspendIntr();
    if (MpidToCB(1)->ctlBlk == NULL) {
        // 117BC
        resumeIntr(oldIntr);
        return 0;
    }
    SceSysmemMemoryPartition *cur = g_145C0.main;
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

int sceKernelDeci2pRegisterOperations(void *op)
{
    g_deci2p = op;
    return 0;
}

void *sceKernelDeci2pReferOperations(void)
{
    return g_deci2p;
}

s32 sceKernelGetMEeDramSaveAddr(void)
{
    if (g_model != 0) {
        // 11894
        if (g_145C0.extVshell == 0) {
            // 118A8
            return g_145C0.vshell->addr + 0x400000;
        } else
            return g_145C0.extVshell->addr;
    } else
        return g_145C0.vshell->addr;
}

s32 sceKernelGetAWeDramSaveAddr(void)
{
    if (g_model != 0) {
        // 118E0
        return g_145C0.vshell->addr;
    } else
        return g_145C0.vshell->addr + 0x200000;
}

s32 sceKernelGetMEeDramSaveSize(void)
{
    if (g_model != 0)
        return 0x400000;
    return 0x200000;
}

s32 sceKernelGetAWeDramSaveSize(void)
{
    if (g_model != 0)
        return 0x400000;
    return 0x200000;
}

s32 sceKernelDevkitVersion(void)
{
    return 0x06060010;
}

s32 sceKernelGetSystemStatus(void)
{
    return g_14544;
}

s32 sceKernelSetSystemStatus(s32 newStatus)
{
    return (g_14544 = newStatus);
}

s32 sceKernelSetUsersystemLibWork(s32 *cmdList, s32 (*sceGeListUpdateStallAddr_lazy)(s32, void*), SceGeLazy *lazy)
{
    s32 oldK1 = pspShiftK1();
    if (!pspK1PtrOk(cmdList) || !pspK1PtrOk(sceGeListUpdateStallAddr_lazy) || !pspK1PtrOk(lazy)) {
        pspSetK1(oldK1);
        return 0x800200D3;
    }
    // 11980
    g_1454C.cmdList = cmdList;
    g_1454C.sceGeListUpdateStallAddr_lazy = sceGeListUpdateStallAddr_lazy;
    g_1454C.lazySyncData = lazy;
    g_1454C.size = sizeof g_1454C;
    pspSetK1(oldK1);
    return 0;
}

SceKernelUsersystemLibWork *sceKernelGetUsersystemLibWork(void)
{
    return g_1454C;
}

void SetMemoryPartitionTable(void *arg0, SceSysmemPartTable *table)
{
    int type;
    if (*(int*)(arg0 + 20) != 0) {
        // 11C08
        table->memSize = 0x04000000;
        type = 2;
        if (*(int*)(arg0 + 24) != 2)
            type = 3;
    } else {
        type = *(int*)(arg0 + 60);
        if ((type & 0x400) == 0 || *(u32*)(arg0 + 96) <= 0x037FFFFF) {
            // 11B9C
            if ((type & 0x1000) != 0 || *(u32*)(arg0 + 96) <= 0x03FFFFFF) {
                type = 0;
                // 11C00
                // 119E0 dup
                table->memSize = 0x02000000;
            }
            table->memSize = 0x04000000;
            switch (*(int*)(arg0 + 24)) { // jump table at 0x13AE8
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
    u32 curAddr = *(int*)(arg0 + 4);
    // 119E4
    table->other1.addr = 0x80000000 | (curAddr & 0x1FFFFFFF);
    // 11A08
    table->other1.size = (type != 2 ? 0x300000 : 0x600000);
    int diff = 0;
    if (*(int*)(arg0 + 8) != 0) {
        diff = *(int*)(arg0 + 8) - table->other1.size;
        table->other1.size = *(int*)(arg0 + 8);
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
    if (type == 3 && (*(int*)(arg0 + 60) & 0x1000) == 0) { // 11B14
        table->extVshell.size = 0x400000;
        table->extVshell.addr = 0x80000000 | (curAddr & 0x1FFFFFFF);
    } else {
        table->extVshell.size = 0;
        // 11B0C
        table->extVshell.addr = 0;
    }
}

s32 suspendSysmem(void)
{
    *(int*)(&g_14574 + 0) = HW(0xBC100040);
    *(int*)(&g_14574 + 4) = HW(0xBC000000) & 0xFEDCEDCF;
    *(int*)(&g_14574 + 8) = HW(0xBC000004) & 0xEDCFCCDD;
    *(int*)(&g_14574 + 12) = HW(0xBC000008);
    *(int*)(&g_14574 + 16) = HW(0xBC00000C);
    *(int*)(&g_14574 + 20) = HW(0xBC000030);
    *(int*)(&g_14574 + 24) = HW(0xBC000034);
    *(int*)(&g_14574 + 28) = HW(0xBC000038);
    *(int*)(&g_14574 + 32) = HW(0xBC00003C);
    *(int*)(&g_14574 + 36) = HW(0xBC000040);
    *(int*)(&g_14574 + 40) = HW(0xBC000044);
    *(int*)(&g_14574 + 44) = HW(0xBC000048);
    return 0;
}

s32 resumeSysmem(void)
{
    HW(0xBC100040) = *(int*)(&g_14574 + 0);
    HW(0xBC000000) = *(int*)(&g_14574 + 4) & 0xCDEFDEFC;
    HW(0xBC000004) = *(int*)(&g_14574 + 8) & 0xDEFCFFEE;
    HW(0xBC000008) = *(int*)(&g_14574 + 12);
    HW(0xBC00000C) = *(int*)(&g_14574 + 16);
    HW(0xBC000030) = *(int*)(&g_14574 + 20);
    HW(0xBC000034) = *(int*)(&g_14574 + 24);
    HW(0xBC000038) = *(int*)(&g_14574 + 28);
    HW(0xBC00003C) = *(int*)(&g_14574 + 32);
    HW(0xBC000040) = *(int*)(&g_14574 + 36);
    HW(0xBC000044) = *(int*)(&g_14574 + 40);
    HW(0xBC000048) = *(int*)(&g_14574 + 44);
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

