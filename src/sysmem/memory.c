s32 sceKernelResizeMemoryBlock(SceUID id, s32 leftShift, s32 rightShift)
{
    u32 rightSegs = 0;
    u32 leftSegs = 0;
    if (leftShift != 0)
        leftSegs = pspMax(leftShift, -leftShift) / 256;
    // 4DFC
    if (rightShift != 0)
        rightSegs = pspMax(rightShift, -rightShift) / 256;
    // 4E20
    if (leftSegs == 0 && rightSegs == 0)
        return 0;
    // 4E5C
    s32 oldIntr = suspendIntr();
    SceSysmemUidCB *uid;
    s32 ret = sceKernelGetUIDcontrolBlockWithType(id, g_13FE0, &uid);
    if (ret != 0) {
        resumeIntr(oldIntr);
        return ret;
    }
    // 4E90
    SceSysmemMemoryBlock *memBlock = (void*)uid + g_13FE0.size * 4;
    SceSysmemMemoryPartition *part = memBlock->part;
    SceSysmemSeg *seg = AddrToSeg(part, memBlock->addr);
    if (seg == NULL)
        goto fail;
    if (seg->used == 0)
        goto fail;
    if (seg->sizeLocked) {
        // 53D4
        resumeIntr(oldIntr);
        return 0x800200DA;
    }
    u32 newLeftOff;
    if (leftShift <= 0) {
        // 53C4
        newLeftOff = seg->offset + leftSegs;
    } else {
        newLeftOff = seg->offset - leftSegs;
    }
    // 4EEC
    u32 newLeftAddr = part->addr + (newLeftOff << 8);
    u32 newRightOff;
    if (rightShift <= 0) {
        // 53B0
        newRightOff = seg->offset + seg->size - rightSegs;
    } else
        newRightOff = seg->offset + seg->size + rightSegs;
    // 4F08
    if (newLeftAddr >= part->addr + (newRightOff << 8))
        goto fail;
    if (newLeftAddr < part->addr)
        goto fail;
    if (newLeftAddr >= part->addr + part->size)
        goto fail;
    SceSysmemCtlBlk *ctlBlk = ((u32)seg >> 8) << 8;
    SceSysmemSeg *prevSeg = &ctlBlk->segs[seg->prev];
    if (seg->prev == 0x3F) {
        // 5394
        prevSeg = NULL;
        if (ctlBlk->prev != NULL)
            prevSeg = &ctlBlk->prev->segs[ctlBlk->lastSeg];
    }
    // 4F60
    SceSysmemSeg *nextSeg = &ctlBlk->segs[seg->next];
    if (seg->next == 0x3F) {
        // 5378
        nextSeg = NULL;
        if (ctlBlk->next != NULL)
            nextSeg = &ctlBlk->next->segs[ctlBlk->firstSeg];
    }
    // 4F84
    if (leftShift > 0) {
        if (prevSeg->used != 0)
            goto fail;
        if (prevSeg->size < leftSegs)
            goto fail;
    }
    // 4FB0
    if (rightShift > 0) {
        if (nextSeg->used != 0)
            goto fail;
        if (nextSeg->size < rightSegs)
            goto fail;
    }
    // 4FF0
    seg->unk8 = 0;
    if (leftSegs != 0) {
        memBlock->addr = newLeftAddr;
        if (leftShift <= 0) {
            // 524C
            memBlock->size -= (leftSegs << 8);
            seg->offset += leftSegs;
            seg->size -= leftSegs;
            if (prevSeg == NULL || prevSeg->used != 0) {
                // 52A8
                SceSysmemSeg *newPrevSeg = &ctlBlk->segs[ctlBlk->freeSeg];
                u32 newPrev = ctlBlk->freeSeg;
                ctlBlk->freeSeg = newPrevSeg->next;
                newPrevSeg->next = (((u32)seg & 0xFF) - 16) / 8;
                newPrevSeg->prev = seg->prev;
                if (newPrevSeg->prev == 0x3F) {
                    // 5370
                    ctlBlk->firstSeg = newPrev;
                } else
                    ctlBlk->segs[seg->prev].next = newPrev;
                // 5320
                seg->prev = newPrev;
                ctlBlk->segCount++;
                newPrevSeg->offset = seg->offset - leftSegs;
                newPrevSeg->size = leftSegs;
                // 523C dup
                updateSmemCtlBlk(part, (u32)newPrevSeg & 0xFFFFFF00);
            } else {
                // 504C dup
                prevSeg->size += leftSegs;
            }
        } else {
            memBlock->size += leftSegs << 8;
            seg->offset -= leftSegs;
            seg->size += leftSegs;
            if (prevSeg->size == leftSegs) {
                // 5224
                _ReturnSegBlankList(prevSeg);
                // 523C dup
                updateSmemCtlBlk(part, (u32)prevSeg & 0xFFFFFF00);
            } else {
                // 504C dup
                prevSeg->size -= leftSegs;
            }
        }
    }
    // 5054
    if (rightSegs != 0) {
        if (rightShift <= 0) {
            // 50EC
            memBlock->size -= rightSegs << 8;
            seg->size -= rightSegs;
            if (nextSeg == NULL || nextSeg->used != 0) {
                // 514C
                SceSysmemSeg *newNextSeg = &ctlBlk->segs[ctlBlk->freeSeg];
                u32 newNext = ctlBlk->freeSeg;
                ctlBlk->freeSeg = newNextSeg->next;
                newNextSeg->next = seg->next;
                newNextSeg->prev = (((u32)seg & 0xFF) - 16) >> 3;
                if (newNextSeg->next == 0x3F)
                    ctlBlk->lastSeg = newNext;
                else
                    ctlBlk->segs[seg->next].prev = newNext;
                // 51D0
                seg->next = newNext;
                ctlBlk->segCount++;
                newNextSeg->offset = seg->offset + seg->size;
                newNextSeg->size = rightSegs;
                updateSmemCtlBlk(part, (u32)newNextSeg & 0xFFFFFF00);
            } else {
                nextSeg->size += rightSegs;
                nextSeg->offset -= rightSegs;
            }
        } else {
            memBlock->size += (rightSegs << 8);
            seg->size += rightSegs;
            if (nextSeg->size != rightSegs) {
                nextSeg->size -= rightSegs;
                nextSeg->offset += rightSegs;
            } else {
                // 50C0
                _ReturnSegBlankList(nextSeg);
                updateSmemCtlBlk(part, (u32)nextSeg & 0xFFFFFF00);
            }
        }
    }
    resumeIntr(oldIntr);
    return 0;

    // 4FE0
fail:
    resumeIntr(oldIntr);
    return 0x800200DB;
}

s32 sceKernelJointMemoryBlock(SceUID id1, SceUID id2);
{
    s32 oldIntr = suspendIntr();
    SceSysmemUidCB *uid1, *uid2;
    s32 ret = sceKernelGetUIDcontrolBlockWithType(id1, g_13FE0, &uid1);
    if (ret != 0)
        goto fail;
    ret = sceKernelGetUIDcontrolBlockWithType(id2, g_13FE0, &uid2);
    if (ret != 0)
        goto fail;
    SceSysmemMemoryBlock *memBlock1 = (void*)uid1 + g_13FE0.size * 4;
    SceSysmemSeg *seg1 = AddrToSeg(memBlock1->part, memBlock1->addr);
    SceSysmemMemoryBlock *memBlock2 = (void*)uid2 + g_13FE0.size * 4;
    SceSysmemSeg *seg2 = AddrToSeg(memBlock2->part, memBlock2->addr);
    if (memBlock1->part != memBlock2->part || seg1->offset + seg1->size != seg2->offset) {
        // 54B8
        resumeIntr(oldIntr);
        return 0x800200E1;
    }
    // 54F0
    if (seg1->sizeLocked || seg2->sizeLocked) {
        // 550C
        resumeIntr(oldIntr);
        return 0x800200DA;
    }
    // 551C
    if (uid1->attr != uid2->attr) {
        // 5674
        resumeIntr(oldIntr);
        return 0x800200E2;
    }
    SceSysmemUidCB *newUid;
    ret = sceKernelCreateUID(g_13FE0, uid1->name, (uid1->attr | (pspGetK1() >> 31)) & 0xFF, &newUid);
    if (ret != 0)
        goto fail;
    // 5560
    seg1->size += seg2->size;
    SceSysmemCtlBlk *ctlBlk = (SceSysmemCtlBlk*)((u32)seg2 & 0xFFFFFF00);
    ctlBlk->unk10--;
    SceSysmemMemoryBlock *newMemBlock = (void*)newUid + g_13FE0.size * 4;
    newMemBlock->addr = memBlock1->addr;
    newMemBlock->size = seg1->size << 8;
    newMemBlock->part = memBlock1->part;
    if (sceKernelIsToolMode() != 0 && sceKernelDipsw(24) == 1) { // 5614
        if (newMemBlock->part == MpidToCB(2)) {
            newMemBlock->size -= 256;
            s32 i;
            // 5658
            for (i = 0; i < 256; i += 4)
                *(int*)(newMemBlock->addr + newMemBlock->size + i) = newMemBlock->addr - 1;
            seg1->unk8 = 1;
        }
    }
    // 55CC
    _ReturnSegBlankList(seg2);
    updateSmemCtlBlk(newMemBlock->part, ctlBlk);
    sceKernelCallUIDObjFunction(uid1, 0x2430FC6B);
    sceKernelCallUIDObjFunction(uid2, 0x2430FC6B);
    resumeIntr(oldIntr);
    return newUid->uid;;

    // 5550
fail:
    resumeIntr(oldIntr);
    return ret;
}

s32 sceKernelSeparateMemoryBlock(SceUID id, u32 cutBefore, u32 size);
{
    s32 oldIntr = suspendIntr();
    SceSysmemUidCB *uid;
    ret = sceKernelGetUIDcontrolBlockWithType(id, g_13FE0, &uid);
    if (ret != 0)
        goto fail;
    SceSysmemMemoryBlock *memBlock = (void*)uid + g_13FE0.size * 4;
    SceSysmemSeg *seg = AddrToSeg(memBlock->part, memBlock->addr);
    SceSysmemCtlBlk *ctlBlk = (SceSysmemCtlBlk*)((u32)seg & 0xFFFFFF00);
    if (seg->sizeLocked) {
        // 5A58
        resumeIntr(oldIntr);
        return 0x800200DA;
    }
    u32 cutSegSize = (size + 0xFF) >> 8;
    if (cutSegSize >= seg->size) {
        // 5A44
        resumeIntr(oldIntr);
        return 0x800200E3;
    }
    SceSysmemUidCB *newUid;
    ret = sceKernelCreateUID(g_13FE0, uid->name, (uid->attr | (pspGetK1() >> 31)) & 0xFF, &newUid);
    if (ret != 0)
        goto fail;
    SceSysmemSeg *freeSeg;
    if (cutBefore) {
        u32 freeId = ctlBlk->freeSeg;
        // 596C
        freeSeg = &ctlBlk->segs[freeId];
        ctlBlk->freeSeg = freeSeg->next;
        freeSeg->next = (((u32)seg & 0xFF) - 16) >> 3;
        freeSeg->prev = seg->prev;
        if (freeSeg->prev == 0x3F) {
            // 5A2C
            ctlBlk->firstSeg = freeId;
        } else
            ctlBlk->segs[seg->prev].next = freeId;
        // 59D8
        seg->prev = freeId;
        ctlBlk->segCount++;
        freeSeg->offset = seg->offset;
        seg->offset = freeSeg->offset + seg->size - cutSegSize;
    } else {
        u32 freeId = ctlBlk->freeSeg;
        SceSysmemSeg *freeSeg = &ctlBlk->segs[freeId];
        ctlBlk->freeSeg = freeSeg->next;
        freeSeg->next = seg->next;
        freeSeg->prev = (((u32)seg & 0xFF) - 16) >> 3;
        if (freeSeg->next == 0x3F)
            ctlBlk->lastseg = freeId;
        else
            ctlBlk->segs[seg->next].prev = freeId;
        // 57DC
        seg->next = freeId;
        ctlBlk->segCount++;
        freeSeg->offset = seg->offset + cutSegSize;
    }
    // 5814
    freeSeg->size = seg->size - cutSegSize;
    freeSeg->used = 1;
    seg->size = cutSegSize;
    seg->unk8 = 0;
    memBlock->addr = memBlock->part->addr + (seg->offset << 8);
    memBlock->size = cutSegSize << 8;
    ctlBlk->unk10++;
    SceSysmemMemoryBlock *newMemBlock = (void*)newUid + g_13FE0.size * 4;
    newMemBlock->addr = memBlock->part->addr + (freeSeg->offset << 8);
    newMemBlock->size = freeSeg->size << 8;
    newMemBlock->part = memBlock->part;
    if (sceKernelIsToolMode() != 0) {
        // 590C
        if (sceKernelDipsw(24) == 1) {
            if (newMemBlock->part == MpidToCB(2)) {
                newMemBlock->size -= 256;
                // 5950
                s32 i;
                for (i = 0; i < 256; i += 4)
                    *(int*)(newMemBlock->addr + newMemBlock->size + i) = newMemBlock->addr - 1;
                freeSeg->unk8 = 1;
            }
        }
    }
    // 58C0
    updateSmemCtlBlk(newMemBlock->part, ctlBlk);
    resumeIntr(oldIntr);
    return newUid->uid;

    // 5A34
fail:
    resumeIntr(oldIntr);
    return ret;
}

s32 sceKernelQueryMemoryInfoForUser(u32 address, SceUID *partitionId, SceUID *memoryBlockId)
{
    s32 oldK1 = pspShiftK1();
    if (!pspK1PtrOk(a0) || !pspK1PtrOk(partitionId) || !pspK1PtrOk(memoryBlockId)) {
        // 5AC0
        pspSetK1(oldK1);
        return 0x800200D1;
    }
    // 5AEC
    s32 oldIntr = suspendIntr();
    s32 partId;
    s32 memBlockId;
    if (memoryBlockId == NULL) {
        // 5C68
        SceSysmemMemoryPartition *part = AddrToCB(address);
        SceSysmemUidCB *curUid = g_145A8->PARENT0;
        partId = -1;
        // 5C88
        while (curUid != g_145A8) {
            if ((void*)curUid + g_145A8->size * 4 == part) {
                // 5CB4
                partId = curUid->uid;
                break;
            }
            curUid = curUid->PARENT0;
        }
        // 5CAC
    } else {
        SceSysmemMemoryPartition *part = AddrToCB(address);
        SceSysmemUidCB *curUid = g_145A8->PARENT0;
        partId = -1;
        // 5B20
        while (curUid != g_145A8) {
            if ((void*)curUid + g_145A8->size * 4 == part) {
                // 5C60
                partId = curUid->uid;
                break;
            }
            curUid = curUid->PARENT0;
        }
        // 5B44
        if (&memBlockId != NULL) {
            curUid = g_13FE0->PARENT0;
            memBlockId = -1;
            // 5B68
            while (curUid != g_13FE0) {
                SceSysmemMemoryBlock *memBlock = (void*)curUid + g_13FE0->size * 4;
                if (address >= memBlock->addr && address < memBlock->addr + memBlock->size) {
                    memBlockId = curUid->uid;
                    break;
                }
                // 5B94
                curUid = curUid->PARENT0;
            }
            // 5BA4
        }
    }
    // 5BA8
    if (partId < 0) {
        // 5C48
        resumeIntr(oldIntr);
        pspSetK1(oldK1);
        return 0x800200D3;
    }
    if (memoryBlockId != NULL) {
        SceSysmemUidCB *uid;
        s32 ret = sceKernelGetUIDcontrolBlockWithType(memBlockId, g_13FE0, &uid);
        if (ret != 0) {
            // 5C34
            resumeIntr(oldIntr);
            pspSetK1(oldK1);
            return ret;
        }
        if (pspK1UserMode() && (uid->attr & 1) == 0) {
            // 5C20
            resumeIntr(oldIntr);
            pspSetK1(oldK1);
            return 0x800200D1;
        }
        *memoryBlockId = memBlockId;
    }
    // 5BF8
    if (partitionId != NULL)
        *partitionId = CanoniMpid(partId);
    // 5C0C
    pspSetK1(oldK1);
    resumeIntr(oldIntr);
    return 0;
}

s32 sceKernelQueryMemoryBlockInfo(SceUID id, SceSysmemMemoryBlockInfo *infoPtr)
{
    SceSysmemMemoryBlockInfo info;
    if (infoPtr == NULL)
        return 0x800200D2;
    if (infoPtr->size != sizeof(SceSysmemMemoryBlockInfo))
        return 0x800200D2;
    s32 oldIntr = suspendIntr();
    SceSysmemUidCB *uid;
    s32 ret = sceKernelGetUIDcontrolBlockWithType(arg0, g_13FE0, &uid);
    if (ret != 0) {
        resumeIntr(oldIntr);
        return ret;
    }
    // 5D4C
    SceSysmemMemoryBlock *memBlock = (void*)uid + g_13FE0->size * 4;
    SceSysmemSeg *seg = AddrToSeg(memBlock->part, memBlock->addr);
    // TODO: check if it uses inline memset
    memset(&info, 0, sizeof info);
    // 5D78
    info.size = sizeof info;
    if (uid->name != 0) {
        // 5E24
        strncpy(info.name, uid->name, 31);
    }
    // 5D9C
    info.attr = uid->attr;
    info.sizeLocked = seg->sizeLocked;
    info.addr = memBlock->addr;
    info.memSize = memBlock->size;
    info.used = seg->used;
    // 5DD8
    // TODO: check if it uses inline memcpy
    memcpy(infoPtr, &info, sizeof info);
    resumeIntr(oldIntr);
    return 0;
}

int _allocSysMemory(SceSysmemMemoryPartition *part, int type, u32 size, u32 addr, int *arg4)
{
    u32 numSegs = (size + 0xFF) >> 8;
    if (numSegs == 0)
        return 0;
    if (sceKernelIsToolMode()) {
        // 6868
        if (sceKernelDipsw(24) == 1 && part == MpidToCB(2) &&
            (type != 0 || part->firstCtlBlk->segs[part->firstCtlBlk->firstSeg].used == 1)
            numSegs++;
    }
    // (5E84)
    // 5E88
    switch (type) // jump table at 0x1355C
    {
    case 0: // low
        // 5EAC
        curCtlBlk = part->firstCtlBlk;
        goto alloc_lo;

    case 1: // hi
        // 6158
        curCtlBlk = part->lastCtlBlk;
        goto alloc_hi;

    case 2: // addr
        // 6364
        if (addr < part->addr || addr >= part->addr + part->size) {
            // 638C
            Kprintf("ADDR: 0x%08x doesn't reside in this partition.\n", addr);
            return 0;
        }
        // 63A0
        u32 off = (addr & 0x1FFFFFFF) - (part->addr & 0x1FFFFFFF);
        if ((off & 0xFF) != 0)
        {
            // 6658
            Kprintf("%s: Illegal alloc address or size, offset=0x%x\n", "SCE_KERNEL_SMEM_ADDR", off);
            return 0;
        }
        SceSysmemCtlBlk *curCtlBlk = part->firstCtlBlk;
        u32 blockOff = off >> 8;
        // 63CC
        while (curCtlBlk != NULL) {
            SceSysmemSeg *firstSeg = &curCtlBlk->segs[curCtlBlk->firstSeg];
            SceSysmemSeg *lastSeg = &curCtlBlk->segs[curCtlBlk->lastSeg];
            if (blockOff >= firstSeg->offset && blockOff < lastSeg->offset + lastSeg->size)
                SceSysmemSeg *curSeg = firstSeg;
                u32 i;
                // 642C
                for (i = 0; i < curCtlBlk->segCount; i++) {
                    if (blockOff >= curSeg->offset && blockOff < curSeg->offset + curSeg->size) {
                        // 6488
                        if (curSeg->used) {
                            // 6644
                            Kprintf("ADDR: relevant block is already used, can not alloc\n");
                            Kprintf("ADDR: request nblocks %u ,addr 0x%08x\n", numSegs, addr);
                            return 0;
                        }
                        if (numSegs >= curSeg->size) {
                            // 661C
                            Kprintf("ADDR: relevant block is smaller than requested, can not alloc\n");
                            Kprintf("ADDR: request nblocks %u, addr 0x%08x\n", numSegs, addr);
                            return 0;
                        }
                        goto alloc_addr;
                    }
                    // 6458
                    curSeg = &curCtlBlk->segs[curSeg->next];
                }
            }
            // 6474
            curCtlBlk = curCtlBlk->next;
            // 6478
        }
        return 0;

    case 3: // ?
        // 6668
        SceSysmemCtlBlk *curCtlBlk = part->firstCtlBlk;
        if (addr < 0x101)
            goto alloc_lo;
        // 6680
        while (curCtlBlk != NULL) {
            SceSysmemSeg *curSeg = curCtlBlk->segs[curCtlBlk->firstSeg];
            u32 i = 0, j = 0;
            // 66BC
            while (i < curCtlBlk->segCount && j < curCtlBlk->segCount - curCtlBlk->unk10) {
                if (curSeg->used == 0) {
                    if (curSeg->size >= numSegs) {
                        // 66F8
                        u32 freeSize = (part->addr + (curSeg->offset << 8)) % addr;
                        u32 blockOff = (u32)(addr - freeSize) >> 8;
                        if (freeSize == 0)
                            blockOff = 0;
                        // 670C
                        if (curSeg->size >= numSegs + blockOff) {
                            // 675C
                            blockOff += curSeg->offset;
                            goto alloc_addr;
                        }
                    }
                    j++;
                }
                // 6720
                i++;
                curSeg = &curCtlBlk->segs[curSeg->next];
            }
            // (6748)
            curCtlBlk = curCtlBlk->next;
            // 674C
        }
        return 0;

    case 4: // ?
        // 6768
        SceSysmemCtlBlk *curCtlBlk = part->lastCtlBlk;
        if (addr < 0x101)
            goto alloc_hi;
        // 6780
        while (curCtlBlk != NULL) {
            SceSysmemSeg *curSeg = &curCtlBlk->segs[curCtlBlk->lastSeg];
            u32 i = 0, j = 0;
            // 67BC
            while (i < curCtlBlk->segCount && j < curCtlBlk->segCount - curCtlBlk->unk10) {
                if (curSeg->used == 0) {
                    j++;
                    if (curSeg->size >= numSegs) {
                        u32 endOff = curSeg->offset + curSeg->size;
                        hi = (part->addr + ((endOff - numSegs) << 8)) % addr;
                        // 6804
                        u32 segSize = numSegs + ((u32)hi >> 8);
                        j++;
                        if (curSeg->size >= numSegs + segSize) {
                            // 6858
                            u32 blockOff = endOff - numSegs;
                            goto alloc_addr;
                        }
                    }
                }
                // 681C
                i++;
                curSeg = &curCtlBlk->segs[curSeg->prev];
            }
            // 6844
            curCtlBlk = curCtlBlk->prev;
        }
        return 0;

    default:
        return 0;
    }

alloc_lo:
    // 5EB0
    // 5EBC
    while (curCtlBlk != NULL) {
        SceSysmemSeg *curSeg = &curCtlBlk->segs[curCtlBlk->firstSeg];
        u32 i = 0, j = 0;
        // 5EF8
        while (j < curCtlBlk->segCount - curCtlBlk->unk10 && i < curCtlBlk->segCount) {
            if (curSeg->used == 0) {
                j++;
                if (curSeg->size >= numSegs) {
                    // 5F78
                    curCtlBlk->unk10++;
                    curSeg->used = 1;
                    addr = part->addr + (curSeg->offset << 8);
                    curSeg->unk0_0 = 0;
                    curSeg->sizeLocked = 0;
                    if (sceKernelIsToolMode() && sceKernelDipsw(24) == 1 &&
                        part == MpidToCB(2) && curSeg != &part->firstCtlBlk->segs[part->firstCtlBlk->firstSeg]) // 6108
                        curSeg->unk8 = 1;
                    // 5FAC
                    // 5FB0
                    if (curSeg->size == numSegs)
                        goto alloc_success;
                    goto alloc_enlargeBlock;
                }
            }
            // 5F1C
            i++;
            curSeg = &curCtlBlk->segs[curSeg->next];
        }
        // 5F40
        curCtlBlk = curCtlBlk->next;
    }
    return 0;

alloc_enlargeBlock:
    SceSysmemCtlBlk *ctlBlk = (SceSysmemCtlBlk*)((u32)curSeg & 0xFFFFFF00);
    // 5FC0
    u32 freeSegId = ctlBlk->freeSeg;
    SceSysmemSeg *firstSeg = &ctlBlk->segs[freeSegId];
    ctlBlk->freeSeg = firstSeg->next;
    u32 curSegId = (((u32)curSeg & 0xFF) - 16) / 8;
    firstSeg->next = curSeg->next;
    firstSeg->prev = curSegId;
    if (firstSeg->next == 0x3F)
        ctlBlk->lastSeg = freeSegId;
    else
        ctlBlk->segs[curSeg->next].prev = freeSegId;
    // 603C
    curSeg->next = freeSegId;
    ctlBlk->segCount++;
    firstSeg->offset = curSeg->offset + numSegs;
    firstSeg->size = curSeg->size - numSegs;
    curSeg->size = numSegs;

alloc_success:
    if (sceKernelIsToolMode() && sceKernelDipsw(24) == 1 && part == MpidToCB(2)) { // 60C0
        u32 i;
        s32 *ptr = (s32*)(addr + (numSegs << 8) - 0x100);
        for (i = 0; i < 256; i += 4)
            ptr[i] = addr - 1;
    }
    // 609C
    if (arg4 == 0) {
        // 60B0
        updateSmemCtlBlk(part, curCtlBlk);
    } else
        *arg4 = curCtlBlk;
    return addr;

alloc_hi:
    // 6168
    while (curCtlBlk != NULL) {
        SceSysmemSeg *curSeg = &curCtlBlk->segs[curCtlBlk->lastSeg];
        u32 i = 0, j = 0;
        // 61A4
        while (i < curCtlBlk->segCount && j < curCtlBlk->segCount - curCtlBlk->unk10) {
            if (curSeg->used == 0) {
                j++;
                if (curSeg->size >= numSegs) {
                    // 61FC
                    curCtlBlk->unk10++;
                    curSeg->used = 1;
                    addr = part->addr + ((curSeg->offset + curSeg->size - numSegs) << 8);
                    curSeg->unk0_0 = 0;
                    curSeg->sizeLocked = 0;
                    if (sceKernelIsToolMode() && sceKernelDipsw(24) == 1 && part == MpidToCB(2)) // 6330
                        curSeg->unk8 = 1;
                    // (6238)
                    // 623C
                    if (curSeg->size != numSegs) {
                        SceSysmemCtlBlk *ctlBlk = (SceSysmemCtlBlk*)((u32)curSeg & 0xFFFFFF00);
                        u16 freeSegId = ctlBlk->freeSeg;
                        u32 curSegId = (((u32)curSeg & 0xFF) - 16) / 8;
                        SceSysmemSeg *freeSeg = ctlBlk->segs[freeSegId];
                        ctlBlk->freeSeg = freeSeg->next;
                        freeSeg->next = curSegId;
                        freeSeg->prev = curSeg->prev;
                        if (curSeg->prev == 0x3F) {
                            // 6328
                            ctlBlk->firstSeg = freeSegId;
                        } else
                            ctlBlk->segs[curSeg->prev].next = freeSegId;
                        // 62BC
                        curSeg->prev = freeSegId;
                        ctlBlk->segCount++;
                        freeSeg->offset = curSeg->offset;
                        freeSeg->size = curSeg->size - numSegs;
                        curSeg->size = numSegs;
                        curSeg->offset += curSeg->size - numSegs;
                    }
                    goto alloc_success;
                }
            }
            // 61C0
            i++;
            curSeg = &curCtlBlk->segs[curSeg->prev];
        }
        // 61E8
        curCtlBlk = curCtlBlk->prev;
    }
    return 0;

alloc_addr:
    // 64A0
    addr = part->addr + (blockOff << 8);
    curCtlBlk->unk10++;
    if (curSeg->offset < blockOff)
    {
        SceSysmemCtlBlk *ctlBlk = (SceSysmemCtlBlk*)((u32)curSeg & 0xFFFFFF00);
        u16 freeSegId = ctlBlk->freeSeg;
        SceSysmemSeg *freeSeg = &ctlBlk->segs[freeSegId];
        /* Insert freeSeg before curSeg */
        ctlBlk->freeSeg = freeSeg->next;
        freeSeg->next = (u32)(((u32)curSeg & 0xFF) - 16) / 8;
        freeSeg->prev = curSeg->prev;
        /* If curSeg is the first block (?) */
        if (curSeg->prev == 0x3F) {
            // 6614
            /* First block is now freeSeg */
            ctlBlk->firstSeg = freeSegId;
        } else
            ctlBlk->segs[curSeg->prev].next = freeSegId;
        // 6534
        curSeg->prev = freeSegId;
        ctlBlk->segCount++;
        freeSeg->offset = curSeg->offset;
        freeSeg->size = blockOff - curSeg->offset;
        curSeg->offset = blockOff;
        curSeg->size -= freeSeg->size;
    }
    // 65A0
    curSeg->used = 1;
    curSeg->unk0_0 = 0;
    curSeg->sizeLocked = 0;
    if (sceKernelIsToolMode() && sceKernelDipsw(24) == 1 && part == MpidToCB(2)) // 65E0
        curSeg->unk8 = 1;
    // (65C4)
    // 65C8
    if (numSegs < curSeg->size)
        goto alloc_success;
    goto alloc_enlargeBlock;
}

s32 FreeUsedSeg(SceSysmemMemoryPartition *part, SceSysmemSeg *seg)
{
    SceSysmemCtlBlk *ctlBlk = ((u32)seg >> 8) << 8;
    if (ctlBlk->segCount < 2)
        return 0x80020001;
    ctlBlk->unk10--;
    seg->unk0_0 = 0;
    seg->sizeLocked = 0;
    seg->unk8 = 0;
    seg->used = 0;
    SceSysmemSeg *nextSeg = &ctlBlk->segs[seg->next];
    if (seg->next == 0x3F) {
        // 6B5C
        nextSeg = NULL;
        if (ctlBlk->next != 0)
            nextSeg = &ctlBlk->segs[ctlBlk->next->firstSeg];
    }
    // 693C
    SceSysmemSeg *prevSeg = &ctlBlk->segs[seg->prev];
    if (seg->prev == 0x3F) {
        // 6B40
        prevSeg = NULL;
        if (ctlBlk->prev != NULL)
            prevSeg = &ctlBlk->segs[seg->lastSeg];
    }
    // 6960
    if (prevSeg != NULL && nextSeg != NULL)
    {
        // 6A1C
        if (prevSeg->used != 0 && nextSeg->used != 0)
            return 0;
        // 6A3C
        if (prevSeg->used == 0 || nextSeg->used != 0) {
            // 6A9C
            if (prevSeg->used == 0 && nextSeg->used != 0) {
                // 69C4 dup
                prevSeg->size = nextSeg->size + seg->size;
                // 69DC dup
                _ReturnSegBlankList(seg);
            } else {
                // 6AB8
                if (seg->next == 0x3F) {
                    // 6AF8
                    nextSeg->offset = prevSeg->offset;
                    nextSeg->size += prevSeg->size + seg->size;
                    _ReturnSegBlankList(prevSeg);
                    // 69DC dup
                    _ReturnSegBlankList(seg);
                } else {
                    prevSeg->size += seg->size + nextSeg->size;
                    _ReturnSegBlankList(seg);
                    // 69DC dup
                    _ReturnSegBlankList(nextSeg);
                }
            }
        } else if (seg->next != 0x3F) {
            // 69FC dup
            seg->size += nextSeg->size;
            // 69DC dup
            _ReturnSegBlankList(nextSeg);
        } else {
            nextSeg->offset = seg->offset;
            nextSeg->size += seg->size;
            // 69DC dup
            _ReturnSegBlankList(seg);
        }
    } else if (prevSeg == NULL) {
        // 69EC
        if (nextSeg->used != 0)
            return 0;
        // 69FC dup
        seg->size += nextSeg->size;
        // 69DC dup
        _ReturnSegBlankList(nextSeg);
    } else if (nextSeg == NULL) {
        // 69B0
        if (prevSeg->used != 0)
            return 0;
        // 69C4 dup
        prevSeg->size += seg->size;
        // 69DC dup
        _ReturnSegBlankList(seg);
    }
    // 6984
    updateSmemCtlBlk(part, ctlBlk);
    return 0;
}

s32 SeparateSmemCtlBlk(SceSysmemMemoryPartition *part, SceSysmemCtlBlk *ctlBlk)
{
    SceSysmemCtlBlk *allocedCtlBlk;
    SceSysmemMemoryPartition *kernel = MpidToCB(1);
    SceSysmemCtlBlk *newCtlBlk = _allocSysMemory(kernel, 3, 256, 256, *allocedCtlBlk);
    if (newCtlBlk == NULL)
        return;
    // 6BD8
    // TODO: check if it uses inline memset
    memset(newCtlBlk, 0, 256);
    SceSysmemSeg *curSeg = &ctlBlk->segs[0];
    // 6BEC
    s32 i;
    for (i = 0; i < 30; i++) {
        curSeg->next = i + 1;
        curSeg++;
    }
    newCtlBlk->lastSeg = 0x3F;
    u16 newSegCount = (((ctlBlk->segCount * 2) / 3) >> 1) & 0xFFFF;
    newCtlBlk->firstSeg = 0;
    newCtlBlk->freeSeg = newSegCount;
    newCtlBlk->lastSeg = newSegCount - 1;
    curSeg = &newCtlBlk->segs[newSegCount];
    newCtlBlk->segs[newSegCount & 0xFF].next = 0x3F;
    s32 i = newSegCount;
    SceSysmemSeg *lastSeg = &ctlBlk->segs[ctlBlk->lastSeg];
    // 6C78
    while (i > 0) {
        if (lastSeg->used != 0) {
            ctlBlk->unk10--;
            newCtlBlk->unk10++;
        }
        // 6CA0
        curSeg->used = lastSeg->used;
        curSeg->unk0_0 = lastSeg->unk0_0;
        curSeg->sizeLocked = lastSeg->sizeLocked;
        curSeg->unk8 = lastSeg->unk8;
        curSeg->offset = lastSeg->offset;
        curSeg->size = lastSeg->size;
        _ReturnSegBlankList(lastSeg);
        curSeg->prev = i - 2;
        i--;
        curSeg--;
        lastSeg = &ctlBlk->segs[ctlBlk->lastSeg];
    }
    // 6D34
    newCtlBlk->prev = ctlBlk;
    newCtlBlk->segs[0].next = 0x3F;
    newCtlBlk->next = ctlBlk->next;
    if (ctlBlk->next == NULL)
        part->lastCtlBlk = newCtlBlk;
    else
        ctlBlk->next->prev = newCtlBlk;
    // 6D5C
    newCtlBlk->segCount = newSegCount;
    part->unk24++;
    ctlBlk->next = newCtlBlk;
    if (allocedCtlBlk == NULL || allocedCtlBlk == ctlBlk)
        return;
    if (allocedCtlBlk->segCount >= 27) {
        // 6DB8
        SeparateSmemCtlBlk(kernel);
    }
}

void updateSmemCtlBlk(SceSysmemMemoryPartition *part, SceSysmemCtlBlk *ctlBlk)
{
    if (part->unk24 != 1 && ctlBlk->segCount < 6) {
        // 6E34
        JointSmemCtlBlk(part, ctlBlk);
    }

    if (ctlBlk->segCount >= 27) {
        // 6E24
        SeparateSmemCtlBlk(part, ctlBlk);
    }
}

void MemoryBlockServiceInit(void)
{
    sceKernelCreateUIDtype("SceSysMemMemoryBlock", 12, &g_13514, 0, &g_13FE0);
}

void InitSmemCtlBlk(SceSysmemCtlBlk *ctlBlk)
{
    int i;
    // 6E84
    memset(ctlBlk, 0, 256);
    // 6E98
    for (i = 0; i < 30; i++)
        ctlBlk->segs[i].next = i + 1;
    ctlBlk->lastSeg = 0x3F;
    ctlBlk->firstSeg = 0x3F;
}

int _AllocPartitionMemory(SceSysmemMemoryPartition *part, int type, int size, int addr)
{
    if (type == 2)
    {
        // 6EF4
        int startAddr = addr & 0xFFFFFF00;
        int endAddr = (addr + size + 0xFF) & 0xFFFFFF00;
        if ((addr & 0xFF) != 0) {
            size = endAddr - startAddr;
            addr = startAddr;
        }
    }
    // 6EE0
    return _allocSysMemory(part, type, size, addr, NULL);
}

s32 sceKernelSizeLockMemoryBlock(SceUID id)
{
    s32 oldIntr = suspendIntr();
    SceSysmemUidCB *uid;
    s32 ret = sceKernelGetUIDcontrolBlockWithType(id, g_13FE0, &uid);
    if (ret != 0) {
        resumeIntr(oldIntr);
        return ret;
    }
    // 6F7C
    SceSysmemMemoryBlock *memBlock = (void*)uid + g_13FE0.size * 4;
    SceSysmemSeg *seg = AddrToSeg(memBlock->part, memBlock->addr);
    seg->sizeLocked = 1;
    resumeIntr(oldIntr);
    return 0;
}

s32 _FreePartitionMemory(u32 addr)
{
    SceSysmemMemoryPartition *part = AddrToCB(addr);
    if (part == MpidToCB(1)) {
        // 7004
        SceSysmemCtlBlk *curCtlBlk = part->firstCtlBlk;
        // 7010
        while (curCtlBlk != NULL) {
            if ((void*)addr == curCtlBlk)
                return 0x80020001;
            curCtlBlk = curCtlBlk->next;
        }
    }
    // 6FE8
    return _freeSysMemory(part, addr);
}

s32 sceKernelFreePartitionMemory(SceUID id)
{
    SceSysmemUidCB *uid;
    s32 oldIntr = suspendIntr();
    s32 ret = sceKernelGetUIDcontrolBlockWithType(id, g_13FE0, &uid);
    if (ret == 0)
        ret = sceKernelDeleteUID(id);
    // 707C
    resumeIntr(oldIntr);
    return ret;
}

s32 sceKernelFreePartitionMemoryForUser(SceUID id)
{
    s32 oldK1 = pspShiftK1();
    s32 oldIntr = suspendIntr();
    SceSysmemUidCB *uid;
    s32 ret = sceKernelGetUIDcontrolBlockWithType(id, g_13FE0, &uid);
    if (ret == 0) {
        if (pspK1IsUserMode() && (uid->attr & 0x10) == 0) {
            // 7148
            resumeIntr(oldIntr);
            pspSetK1(oldK1);
            return 0x800200D1;
        }
        ret = sceKernelDeleteUID(id);
    }
    // 711C
    resumeIntr(oldIntr);
    pspSetK1(oldK1);
    return ret;
}

s32 sceKernelQueryMemoryInfo(u32 address, SceUID *partitionId, SceUID *memoryBlockId)
{
    s32 oldIntr = suspendIntr();
    _QueryMemoryInfo(address, partitionId, memoryBlockId);
    resumeIntr(oldIntr);
    return 0;
}

u32 sceKernelGetBlockHeadAddr(SceUID id)
{
    s32 oldIntr = suspendIntr();
    SceSysmemUidCB *uid;
    if (sceKernelGetUIDcontrolBlockWithType(id, g_13FE0, &uid) != 0) {
        // 721C
        resumeIntr(oldIntr);
        return 0;
    }
    u32 addr = (SceSysmemMemoryBlock *)((void*)uid + g_13FE0->size * 4)->addr;
    // 721C
    resumeIntr(oldIntr);
    return addr;
}

u32 SysMemForKernel_CC31DEAD(SceUID id)
{
    s32 oldIntr = suspendIntr();
    SceSysmemUidCB *uid;
    if (sceKernelGetUIDcontrolBlockWithType(id, g_13FE0, &uid) != 0) {
        resumeIntr(oldIntr);
        return 0;
    }
    u32 size = (SceSysmemMemoryBlock *)((void*)uid + g_13FE0->size * 4)->size;
    resumeIntr(oldIntr);
    return size;
}

u32 sceKernelGetBlockHeadAddrForUser(SceUID id)
{
    s32 oldK1 = pspShiftK1();
    s32 oldIntr = suspendIntr();
    SceSysmemUidCB *uid;
    if (sceKernelGetUIDcontrolBlockWithType(id, g_13FE0, &uid) != 0
     || (pspK1IsUserMode() && (uid->attr & 1) == 0)) {
        // 735C
        resumeIntr(oldIntr);
        pspSetK1(oldK1);
        return 0;
    }
    resumeIntr(oldIntr);
    u32 addr = (SceSysmemMemoryBlock *)((void*)uid + g_13FE0->size * 4)->addr;
    pspSetK1(oldK1);
    return addr;
}

void sceKernelProtectMemoryBlock(SceSysmemMemoryPartition *part)
{
    AddrToSeg(part, part->addr)->unk0_0 = 1;
}

s32 _freeSysMemory(SceSysmemMemoryPartition *part, u32 addr)
{
    if ((addr & 0xFF) != 0)
        return -1;
    s32 offset = (addr & 0x1FFFFFFF) - (part->addr & 0x1FFFFFFF);
    s32 segOff = offset / 256;
    SceSysmemCtlBlk *curCtlBlk = part->firstCtlBlk;
    // 73F8
    while (curCtlBlk != NULL) {
        SceSysmemSeg *curSeg = &curCtlBlk->segs[curCtlBlk->firstSeg];
        if (segOff >= curSeg->offset && segOff <= curCtlBlk->segs[curSeg->lastSeg].offset) {
            // 7448
            s32 i;
            for (i = 0; i < curCtlBlk->segCount; i++) {
                if (curSeg->size != 0) {
                    // 75F0
                    if (curSeg->offset == segOff) {
                        // 74B8
                        if (curSeg->used == 0)
                            return 0x80020001;
                        if (curSeg->unk0_0 != 0) {
                            // 7570
                            SceSysmemUidCB *curUid = g_13FE0->PARENT0;
                            SceUID foundId = -1;
                            // 758C
                            while (curUid != g_13FE0) {
                                SceSysmemMemoryBlock *memBlock = (void*)curUid + g_13FE0->size * 4;
                                if (addr >= memBlock->addr && addr < memBlock->addr + memBlock->size) {
                                    foundId = curUid->uid;
                                    break;
                                }
                                // 75B8
                                curUid = curUid->PARENT0;
                            }
                            // 75C8
                            char name[32];
                            sceKernelGetUIDname(foundId, name, 32);
                            AddrToSeg(MpidToCB(1), addr);
                            return 0x80020001;
                        }
                        if (curSeg->unk8 != 0) {
                            u32 *curPtr = (u32*)((void*)addr + (curSeg->size - 1) * 256);
                            // 74EC
                            s32 i;
                            for (i = 0; i < 64; i++) {
                                if (*curPtr != addr - 1) {
                                    // 751C
                                    Kprintf("\n<< Memory block overflow detected >>\n");
                                    Kprintf(" Overflowed block: 0x%08x - 0x%08x\n", addr, addr + (curSeg->size - 1) * 256 - 1);
                                    Kprintf(" (address 0x%08x): expected value is 0x%08x, but actual value is 0x%08x\n", curPtr, addr - 1, *curPtr);
                                    BREAK(0);
                                    break;
                                }
                                curPtr++;
                            }
                        }
                        // 7508
                        return FreeUsedSeg(part, curSeg);
                    }
                }
                // 746C
                curSeg = &curCtlBlk->segs[curSeg->next];
            }
        }
        // 7480
        curCtlBlk = curCtlBlk->next;
    }
    return 0x80020001;
}

s32 block_do_initialize(SceSysmemUidCB *uid, SceSysmemUidCB *uidWithFunc, int funcId, va_list ap)
{
    sceKernelCallUIDObjCommonFunction(uid, uidWithFunc, funcId, ap);
    SceSysmemMemoryBlock *memBlock = (void*)uid + g_13FE0->size * 4;
    memBlock->size = 0;
    memBlock->part = NULL;
    memBlock->addr = 0;
    return uid->uid;
}

s32 block_do_delete(SceSysmemUidCB *uid, SceSysmemUidCB *uidWithFunc, int funcId, va_list ap)
{
    u32 addr = (SceSysmemMemoryBlock *)((void*)uid + g_13FE0->size * 4)->addr;
    if (addr != 0) {
        SceSysmemMemoryPartition *part = AddrToCB(addr);
        if (part == MpidToCB(1))
        {
            SceSysmemCtlBlk *curCtlBlk = part->firstCtlBlk;
            // 76FC
            // 7704
            while (curCtlBlk != NULL) {
                if (addr == (u32)curCtlBlk) {
                    // 7720
                    return 0x80020001;
                }
                curCtlBlk = curCtlBlk->next;
            }
        }
        // 76AC
        s32 ret = _freeSysMemory(part, addr);
        if (ret != 0)
            return ret;
    }
    // 76E0
    sceKernelCallUIDObjCommonFunction(uid, uidWithFunc, funcId, ap);
    return uid->uid;
}

s32 block_do_delete_super(SceSysmemUidCB *uid, SceSysmemUidCB *uidWithFunc, int funcId, va_list ap)
{
    sceKernelCallUIDObjCommonFunction(uid, uidWithFunc, 0x87089863, ap);
    return uid->uid;
}

s32 block_do_resize(SceSysmemUidCB *uid, SceSysmemUidCB *uidWithFunc, int funcId, va_list ap)
{
    s32 ret = sceKernelResizeMemoryBlock(uid->uid, va_arg(ap, s32), va_arg(ap, s32));
    if (ret != 0)
        return ret;
    return uid->uid;
}

s32 block_do_sizelock(SceSysmemUidCB *uid, SceSysmemUidCB *uidWithFunc, int funcId, va_list ap)
{
    s32 oldIntr = suspendIntr();
    SceSysmemUidCB *uid2;
    s32 ret = sceKernelGetUIDcontrolBlockWithType(uid->uid, g_13FE0, &uid2);
    if (ret != 0) {
        resumeIntr(oldIntr);
        return ret;
    }
    // 7818
    SceSysmemMemoryBlock *memBlock = (void*)uid2 + g_13FE0->size * 4;
    AddrToSeg(memBlock->part, memBlock->addr)->sizeLocked = 1;
    resumeIntr(oldIntr);
    return uid->uid;
}

s32 block_do_querymeminfo(SceSysmemUidCB *uid, SceSysmemUidCB *uidWithFunc, int funcId, va_list ap)
{
    _QueryMemoryInfo(va_arg(ap, u32), va_arg(ap, SceUID*), va_arg(ap, SceUID*));
    return uid->uid;
}

s32 block_do_queryblkinfo(SceSysmemUidCB *uid, SceSysmemUidCB *uidWithFunc, int funcId, va_list ap)
{
    s32 ret = sceKernelQueryMemoryBlockInfo(uid->uid, va_arg(ap, SceSysmemMemoryBlockInfo *));
    if (ret != 0)
        return ret;
    return uid->uid;
}

s32 block_do_getheadaddr(SceSysmemUidCB *uid, SceSysmemUidCB *uidWithFunc, int funcId, va_list ap)
{
    u32 *addrPtr = va_arg(ap, u32 *);
    SceSysmemUidCB *uid2;
    u32 addr = 0;
    if (sceKernelGetUIDcontrolBlockWithType(uid->uid, g_13FE0, &uid2) == 0)
        addr = (SceSysmemMemoryBlock *)((void*)uid2 + g_13FE0->size * 4)->addr;
    // 791C
    *addrPtr = addr;
    return uid->uid;
}

s32 _ReturnSegBlankList(SceSysmemSeg *seg)
{
    SceSysmemCtlBlk *ctlBlk = (SceSysmemCtlBlk *)((u32)seg & 0xFFFFFF00);
    u32 segId = (((u32)seg & 0xFF) - 16) >> 3;
    if (ctlBlk->segCount < 2)
        return;
    if (seg->next == 0x3F) {
        // 7A10
        ctlBlk->segs[seg->prev].next = seg->next;
        ctlBlk->lastSeg = seg->prev;
    } else if (seg->prev == 0x3F) {
        // 79F4
        ctlBlk->segs[seg->next].prev = seg->prev;
        ctlBlk->firstSeg = seg->next;
    } else {
        ctlBlk->segs[seg->prev].next = seg->next;
        ctlBlk->segs[seg->next].prev = seg->prev;
    }
    // 79C4
    // TODO: verify that it uses inline memset
    memset(seg, 0, 8);
    seg->next = ctlBlk->freeSeg;
    ctlBlk->freeSeg = segId;
    ctlBlk->segCount--;
}

s32 _QueryMemoryInfo(u32 address, SceUID *partitionId, SceUID *memoryBlockId)
{
    if (partitionId != NULL) {
        SceSysmemMemoryPartition *part = AddrToCB(address);
        SceSysmemUidCB *curUid = g_145A8->PARENT0;
        SceUID foundId = -1;
        // 7A80
        while (curUid != g_145A8) {
            if ((SceSysmemMemoryPartition *)((void*)uid + g_145A8->size * 4) == part) {
                // 7B24
                foundId = curUid->uid;
                break;
            }
            curUid = curUid->PARENT0;
        }
        // 7AA4
        *partitionId = foundId;
    }
    // 7AA8
    if (memoryBlockId != NULL) {
        SceSysmemUidCB *curUid = g_13FE0->PARENT0;
        SceUID foundId = -1;
        // 7ACC
        while (curUid != g_13FE0) {
            SceSysmemMemoryBlock *memBlock = (SceSysmemMemoryBlock *)((void*)curUid + g_13FE0->size * 4);
            if (address >= memBlock->addr && address < memBlock->addr + memBlock->size) {
                foundId = curUid->uid;
                break;
            }
            // 7AF8
            curUid = curUid->PARENT0;
        }
    }
    // 7B08
    *memoryBlockId = foundId;
}

s32 JointSmemCtlBlk(SceSysmemPartition *part, SceSysmemCtlBlk *ctlBlk)
{
    SceSysmemMemoryPartition *kernelPart = MpidToCB(1);
    SceSysmemCtlBlk *nextCtlBlk = ctlBlk->next;
    SceSysmemCtlBlk *prevCtlBlk = ctlBlk->prev;
    if (part->unk24 == 1)
        return;
    if (prevCtlBlk != NULL && (nextCtlBlk == NULL || nextCtlBlk->segCount >= prevCtlBlk->segCount)) {
        // 7EF4
        if (prevCtlBlk->segCount < 16) {
            // 8090
            SceSysmemSeg *curSrcSeg = &ctlBlk->segs[ctlBlk->firstSeg];
            SceSysmemSeg *prevDstSeg = &prevCtlBlk->segs[prevCtlBlk->lastSeg];
            s32 i;
            // 80BC
            for (i = 0; i < ctlBlock->segCount; i++) {
                SceSysmemCtlBlk *newPrevCtlBlk = (SceSysmemCtlBlk *)((u32)prevDstSeg & 0xFFFFFF00);
                u32 newId = newPrevCtlBlk->freeSeg;
                SceSysmemSeg *curDstSeg = &newPrevCtlBlk->segs[newId];
                newPrevCtlBlk->freeSeg = curDstSeg->next;
                curDstSeg->next = prevDstSeg->next;
                curDstSeg->prev = (((u32)prevDstSeg & 0xFF) - 16) >> 3;
                if (curDstSeg->next == 0x3F)
                    newPrevCtlBlk->lastSeg = newId;
                else
                    newPrevCtlBlk->segs[prevDstSeg->next].prev = newId;
                // 813C
                prevDstSeg->next = newId;

                newPrevCtlBlk->segCount++;
                curDstSeg->used = curSrcSeg->used;
                curDstSeg->unk0_0 = curSrcSeg->unk0_0;
                curDstSeg->sizeLocked = curSrcSeg->sizeLocked;;
                curDstSeg->unk8 = curSrcSeg->unk8;
                curDstSeg->offset = curSrcSeg->offset;
                curDstSeg->size = curSrcSeg->size;
                curSrcSeg = &ctlBlk->segs[curSrcSeg->next];
                prevDstSeg = curDstSeg;
            }
            // 81E8
            prevCtlBlk->next = ctlBlk->next;
            prevCtlBlk->unk10 += ctlBlk->unk10;
            if (nextCtlBlk != NULL)
                nextCtlBlk->prev = prevCtlBlk;
            // 8204
            _freeSysMemory(kernelPart, ctlBlk);
            part->unk24--;
            if (part->lastCtlBlk == ctlBlk)
                part->lastCtlBlk = prevCtlBlk;
        } else {
            u16 joinCount = ((prevCtlBlk->segCount * 2) / 3) >> 1;
            SceSysmemSeg *curSrcSeg = &prevCtlBlk->segs[prevCtlBlk->last];
            SceSysmemSeg *prevDstSeg = &ctlBlk->segs[ctlBlk->firstSeg];
            // 7F44
            s32 i;
            for (i = 0; i < joinCount; i++) {
                SceSysmemCtlBlk *newPrevCtlBlk = (SceSysmemCtlBlk *)((u32)prevDstSeg & 0xFFFFFF00);
                u32 newId = newPrevCtlBlk->freeSeg;
                SceSysmemSeg *curDstSeg = &newPrevCtlBlk->segs[newId];
                newPrevCtlBlk->freeSeg = curDstSeg->next;
                curDstSeg->next = (((u32)prevDstSeg & 0xFF) - 16) >> 3;
                curDstSeg->prev = prevDstSeg->prev;
                if (curDstSeg->prev == 0x3F) {
                    // 8088
                    newPrevCtlBlk->firstSeg = newId;
                } else
                    newPrevCtlBlk->segs[prevDstSeg->prev].next = newId;
                // 7BF8
                prevDstSeg->prev = newId;
                newPrevCtlBlk->segCount++;
                if (curSrcSeg->used != 0) {
                    ctlBlk->unk10++;
                    prevCtlBlk->unk10--;
                }
                // 8000
                curDstSeg->used = curSrcSeg->used;
                curDstSeg->unk0_0 = curSrcSeg->unk0_0;
                curDstSeg->sizeLocked = curSrcSeg->sizeLocked;
                curDstSeg->unk8 = curSrcSeg->unk4;
                curDstSeg->offset = curSrcSeg->offset;
                curDstSeg->size = curSrcSeg->size;
                _ReturnSegBlankList(curSrcSeg);
                curSrcSeg = &prevCtlBlk->segs[prevCtlBlk->lastSeg];
                prevDstSeg = curDstSeg;
            }
        }
    } else {
        // 7B9C
        if (nextCtlBlk->segCount < 16) {
            // 7D58
            SceSysmemSeg *curSrcSeg = &ctlBlk->segs[ctlBlk->lastSeg];
            SceSysmemSeg *prevDstSeg = &nextCtlBlk->segs[nextCtlBlk->firstSeg];
            // 7D84
            s32 i;
            for (i = 0; i < ctlBlk->segCount; i++) {
                SceSysmemCtlBlk *newPrevCtlBlk = (SceSysmemCtlBlk *)((u32)prevDstSeg & 0xFFFFFF00);
                u32 newId = newPrevCtlBlk->freeSeg;
                SceSysmemSeg *curDstSeg = &newPrevCtlBlk->segs[newId];
                newPrevCtlBlk->freeSeg = curDstSeg->next;
                curDstSeg->next = ((u32)prevDstSeg & 0xFF - 16) >> 3;
                curDstSeg->prev = prevDstSeg->prev;
                if (curDstSeg->prev == 0x3F) {
                    // 7EEC
                    newPrevCtlBlk->firstSeg = newId;
                } else
                    newPrevCtlBlk->segs[prevDstSeg->prev].next = newId;
                // 7DF8
                prevDstSeg->prev = newId;
                newPrevCtlBlk->segCount++;
                curDstSeg->used = curSrcSeg->used;
                curDstSeg->unk0_0 = curSrcSeg->unk0_0;
                curDstSeg->sizeLocked = curSrcSeg->sizeLocked;
                curDstSeg->unk8 = curSrcSeg->unk8;
                curDstSeg->offset = curSrcSeg->offset;
                curDstSeg->size = curSrcSeg->size;
                curSrcSeg = &ctlBlk->segs[curSrcSeg->prev];
                prevDstSeg = curDstSeg;
            }
            // 7EA8
            nextCtlBlk->prev = ctlBlk->prev;
            nextCtlBlk->unk10 += ctlBlk->unk10;
            if (prevCtlBlk != NULL)
                prevCtlBlk->next = nextCtlBlk;
            // 7EC4
            part->unk24--;
            _freeSysMemory(kernelPart, ctlBlk);
            if (part->firstCtlBlk == ctlBlk)
                part->firstCtlBlk = nextCtlBlk;
        } else {
            u16 joinCount = ((nextCtlBlk->segCount * 2) / 3) >> 1;
            SceSysmemSeg *curSrcSeg = &nextCtlBlk->segs[nextCtlBlk->firstSeg];
            SceSysmemSeg *prevDstSeg = &ctlBlk[ctlBlk->lastSeg];
            // 7BEC
            s32 i;
            for (i = 0; i < joinCount; i++) {
                SceSysmemCtlBlk *newPrevCtlBlk = (SceSysmemCtlBlk *)((u32)prevDstSeg & 0xFFFFFF00);
                u32 newId = newPrevCtlBlk->freeSeg;
                SceSysmemSeg *curDstSeg = &newPrevCtlBlk->segs[newId];
                newPrevCtlBlk->freeSeg = curDstSeg->next;
                curDstSeg->next = prevDstSeg->next;
                curDstSeg->prev = (((u32)prevDstSeg & 0xFF) - 16) >> 3;
                if (curDstSeg->next == 0x3F)
                    newPrevCtlBlk->lastSeg = newId;
                else
                    newPrevCtlBlk->segs[prevDstSeg->next].prev = newId;
                // 7C6C
                prevDstSeg->next = newId;
                newPrevCtlBlk->segCount++;
                if (curSrcSeg->used != 0) {
                    ctlBlk->unk10++;
                    nextCtlBlk->unk10--;
                }
                // 7CB4
                curDstSeg->used = curSrcSeg->used;
                curDstSeg->unk0_0 = curSrcSeg->unk0_0;
                curDstSeg->sizeLocked = curSrcSeg->sizeLocked;
                curDstSeg->unk8 = curSrcSeg->unk8;
                curDstSeg->offset = curSrcSeg->offset;
                curDstSeg->size = curSrcSeg->size;
                _ReturnSegBlankList(curSrcSeg);
                curSrcSeg = &nextCtlBlk->segs[nextCtlBlk->firstSeg];
                prevDstSeg = curDstSeg;
            }
        }
    }
}

/* Allocates blocks in this format:

+----------------------------------------------------------------------------+
|                                                                            |
|     Free block (must be 'align'-aligned, otherwise won't be used)          |
|                                                                            |
+----------------------------------------------------------------------------+

+------------+-------------------------+-------------------------------------+
|            |                         |                                     |
| Free space |     Allocated space     | Padding blocks (so the end of the   |
|            | ('size' upaligned to 8) | allocated space is 'align'-aligned) |
+------------+-------------------------+-------------------------------------+

Note: if allocation fails, results are probably unknown. */

void *hmalloc(SceSysmemHeap *heap, SceSysmemLowheap *lowh, u32 size, u32 align)
{
    if (lowh == NULL)
        return NULL;
    if (lowh->addr != (u32)lowh - 1)
        return NULL;
    // 82B0
    if (size < 8)
        size = 8;
    u32 paddingBlocks;
    u32 allocBlocks = (size / 8) + 1;
    if ((size & 7) != 0)
        allocBlocks = (size / 8) + 2;
    if ((u32)lowh->firstBlock - heap->partAddr >= heap->partSize) {
        // 8444
        if (sceKernelIsToolMode() == 0) {
            // 8454
            for (;;)
                ;
        }
        // 845C
        Kprintf("Heap memory is in illegal memory partition\n");
        pspBreak(0);
    }
    // 82E8
    SceSysmemLowheapBlock *curBlock = lowh->firstBlock->next;
    SceSysmemLowheapBlock *prevBlock = lowh->firstBlock;
    SceSysmemLowheapBlock *prevFreeBlock;
    u32 reservedBlockCount;
    // 82F8
    for (;;)
    {
        if ((u32)curBlock - heap->partAddr >= heap->partSize) {
            // 8418
            if (sceKernelIsToolMode() == 0) {
                // 843C
                for (;;)
                    ;
            }
            Kprintf("Heap memory is in illegal memory partition\n");
            pspBreak(0);
        }
        // 830C
        if (curBlock->count >= allocBlocks) {
            prevFreeBlock = NULL;
            if (curBlock->count == allocBlocks) {
                // 83EC
                if (align == 0 || ((u32)(curBlock + 1) % align) == 0) { // 8400
                    // 840C
                    prevBlock->next = curBlock->next;
                    goto end_noresize;
                }
            } else {
                paddingBlocks = 0;
                SceSysmemLowheapBlock *firstAllocBlock = curBlock + curBlock->count - allocBlocks + 1;
                if (align == 0 || ((u32)firstAllocBlock % align) == 0) // 8344
                    // 83E4
                    reservedBlockCount = allocBlocks;
                    goto end_resize;
                }
                paddingBlocks = ((u32)firstAllocBlock % align) >> 3;
                reservedBlockCount = allocBlocks + paddingBlocks;
                if (curBlock->count >= reservedBlockCount)
                    break;
            }
        }
        // 8364
        if (curBlock == lowh->firstBlock)
            return NULL;
        prevBlock = curBlock;
        curBlock = curBlock->next;
    }
    // 8380
    prevFreeBlock = curBlock;
    if (curBlock->count == reservedBlockCount) {
        // 83D4
        prevFreeBlock = prevBlock;
        prevBlock->next = curBlock->next;
    }
    // 8388
end_resize:
    curBlock->count -= reservedBlockCount;
    curBlock += curBlock->count;
    curBlock->count = allocBlocks;
    if (paddingBlocks != 0) {
        SceSysmemLowheapBlock *paddingBlock = curBlock + allocBlocks;
        paddingBlock->count = paddingBlocks;
        paddingBlock->next = prevFreeBlock->next;
        prevFreeBlock->next = paddingBlock;
    }
    // 83B8
end_noresize:
    lowh->firstBlock = prevBlock;
    curBlock->next = lowh;
    lowh->busyBlocks += curBlock->count;
    return curBlock + 1;
}

s32 hfree(SceSysmemHeap *heap, SceSysmemLowheap *lowh, void *ptr)
{
    SceSysmemLowheapBlock *allocatedBlock = (SceSysmemLowheapBlock *)(ptr - 8);
    if (lowh == NULL || lowh->addr != (u32)lowh - 1)
        return -4;
    // 84DC
    if (allocatedBlock < (void*)lowh + sizeof *lowh || allocatedBlock >= (void*)lowh + lowh->size)
        return -2;
    if (ptr == NULL)
        return -1;
    if (allocatedBlock->next != lowh)
        return -1;
    // 8518
    if (allocatedBlock->count <= 0)
        return -1;
    SceSysmemLowheapBlock *curBlock = lowh->firstBlock;
    // 8534, 86C4
    while (curBlock >= allocatedBlock || allocatedBlock >= curBlock->next) {
        if ((void*)curBlock - heap->partAddr >= heap->partSize) {
            // 8698
            if (sceKernelIsToolMode() == 0) {
                // 86BC
                for (;;)
                    ;
            }
            Kprintf("Heap memory is in illegal memory partition\n");
            pspBreak(0);
        }
        // 854C
        if (allocatedBlock == curBlock)
            return -3;
        if (curBlock >= curBlock->next && (allocatedBlock < curBlock->next || curBlock < allocatedBlock))
            break;
        // 8570
        curBlock = curBlock->next;
    }
    // 8594
    if (curBlock->next > allocatedBlock && curBlock->next < allocatedBlock + allocatedBlock->count)
        return -3;
    // 85B8
    if (allocatedBlock > curBlock && allocatedBlock < curBlock + curBlock->count)
        return -3;
    // 85DC
    lowh->busyBlocks -= allocatedBlock->count;
    if ((void*)curBlock->next - heap->partAddr >= heap->partSize) {
        // 866C
        if (sceKernelIsToolMode() == 0) {
            // 867C
            for (;;)
                ;
        }
        // 8684
        Kprintf("Heap memory is in illegal memory partition\n");
        pspBreak(0);
    }
    // 8604
    if (allocatedBlock + allocatedBlock->count == curBlock->next)
    {
        // 8654
        if (curBlock->next->count <= 0)
            allocatedBlock->next = curBlock->next;
        else {
            allocatedBlock->count += curBlock->next->count;
            allocatedBlock->next = curBlock->next->next;
        }
    }
    else
        allocatedBlock->next = curBlock->next;
    // 861C
    curBlock->next = allocatedBlock;
    if (allocatedBlock == curBlock + curBlock->count) {
        // 863C
        curBlock->count += allocatedBlock->count;
        curBlock->next = allocatedBlock->next;
    }
    // 8630
    lowh->firstBlock = curBlock;
    return 0;
}

s32 initheap(SceSysmemLowheap *lowh, u32 size)
{
    u32 count = (size - 16) / 8;
    SceSysmemLowheapBlock *start = (SceSysmemLowheapBlock*)(lowh + 1);
    SceSysmemLowheapBlock *end = start + count - 1; // TODO: correct only if size of block is 8 (needs check)
    if (lowh != NULL && size >= 41) {
        lowh->addr = (u32)lowh - 1;
        lowh->size = size;
        lowh->firstBlock = start;
        lowh->busyBlocks = 0;
        start->next = end;
        start->count = count - 1;
        end->next = start;
        end->count = 0;
    }
}

s32 checkheapnouse(SceKernelLowHeap *lowh)
{
    if (lowh == NULL || lowh->addr != (u32)lowh - 1)
        return 0;
    return (lowh->busyBlocks == 0);
}

u32 htotalfreesize(SceSysmemLowheap *lowh)
{
    u32 numBlocks = (lowh->size - 16) / 8;
    u32 freeBlocks = numBlocks - lowh->busyBlocks;
    return (freeBlocks - 1) * 8;
}

s32 hmaxfreesize(SceSysmemHeap *heap, SceSysmemLowHeap *lowh);
{
    SceSysmemLowheapBlock *firstBlock = lowh->firstBlock;
    u32 maxfreeblocks = 0;
    if (firstBlock - heap->partAddr >= heap->partSize)
        return -1;
    SceSysmemLowheapBlock *curBlock = firstBlock->next;
    // 879C
    for (;;) {
        if (curBlock - heap->partAddr >= heap->partSize)
            return -1;
        if (curBlock->count > maxfreeblocks)
            maxfreeblocks = curBlock->count;
        if (curBlock == firstBlock)
            return (maxfreeblocks - 1) * 8;
        curBlock = curBlock->next;
    }
}

int memset(void *s, int c, int n)
{
    if (s != NULL)
        return sceKernelMemset(s, c, n);
    return s;
}

s32 sceKernelMemset(s8 *src, s8 c, u32 size)
{
    s8 *end = src + size;
    if (size >= 8) {
        // 8824
        u32 value = c | (c << 8) | (c << 12) | (c << 16);
        s8 *alignedSrc = (s8 *)(((u32)src + 3) & 0xFFFFFFFC);
        if (alignedSrc != src) {
            asm("swr %0, 0(%1)" : : "r" (value), "r" (src));
            size -= alignedSrc - src;
        }
        // 8844
        if (((u32)end & 3) != 0)
            asm("swl %0, -1(%1)" : : "r" (value), "r" (end));
        // 8854
        sceKernelMemset32(alignedSrc, value, size);
    } else {
        s8 *cur = src;
        // 8804
        while (cur != end)
            *(cur++) = c;
    }
    return src;
}

s32 *sceKernelMemset32(s32 *src, s32 c, u32 size)
{
    size = size & 0xFFFFFFFC;
    if (((u32)src & 3) != 0)
        return NULL;
    if (((0x2C >> (((u32)src >> 29) & 0x7)) & 1) != 0) { // accepts first 4 bits 0x4, 0x5, 0x6, 0x7, 0xA, 0xB
        // 8964
        return wmemset(src, c, size);
    }
    if (size < 257) {
        // 8964
        return wmemset(src, c, size);
    }
    s32 *alignedSrc = (s32 *)(((u32)src + 0x3F) & 0xFFFFFFC0);
    s32 *end = (void*)src + size;
    if (alignedSrc != src)
        wmemset(src, c, alignedSrc - src);
    // 88C8
    if (((u32)end & 0x3F) != 0) {
        // 8954
        end = wmemset((void*)end - ((u32)end & 0x3F), c, size);
    }
    // 88D4
    s32 *curSrc = alignedSrc;
    // 88E0
    while (curSrc != end) {
        pspCache(0x18, curSrc);
        *(curSrc +  0) = *(curSrc +  1) = *(curSrc +  2) = *(curSrc +  3) = c;
        *(curSrc +  4) = *(curSrc +  5) = *(curSrc +  6) = *(curSrc +  7) = c;
        *(curSrc +  8) = *(curSrc +  9) = *(curSrc + 10) = *(curSrc + 11) = c;
        *(curSrc + 12) = *(curSrc + 13) = *(curSrc + 14) = *(curSrc + 15) = c;
        curSrc += 16;
    }
    // 8930
    return src;
}

void *sceKernelMemmove(void *dst, void *src, u32 size)
{
    void *realDst = (void*)((u32)dst & 0x1FFFFFFF);
    void *realSrc = (void*)((u32)src & 0x1FFFFFFF);
    if (realDst == realSrc)
        return dst;
    u32 dist = realDst - realSrc;
    if (((0x2C >> (((u32)dst >> 29) & 7)) & 1) != 0) {
        // 8B0C
        return memmove(dst, src, size);
    }
    if (dist + 63 < 126) {
        // 8B0C
        return memmove(dst, src, size);
    }
    if (realDst < realSrc) {
        // 8B1C
        return sceKernelMemcpy(dst, src, size);
    }
    u32 dstAlign = (-(u32)dst) & 0x3F;
    if (dstAlign >= size) {
        // 8B1C
        return sceKernelMemcpy(dst, src, size);
    }
    if ((dist & 3) != 0) {
        // 8B0C
        return memmove(dst, src, size);
    }
    void *dstEnd = dst + size;
    u32 dstEndAlign = (u32)dstEnd & 0x3F;
    if (dstEndAlign != 0) {
        size -= dstEndAlign;
        // 8AF8
        memmove(dst + size, src + size, dstEndAlign);
    }
    size -= dstAlign;
    // 8A04
    if (size != 0) {
        void *alignedDst = dst + dstAlign;
        s32 *curDst = (void*)alignedDst + size - 64;
        s32 *curSrc = (void*)src + dstAlign + size - 64;
        // 8A28
        while (curDst >= alignedDst) {
            pspCache(0x18, curDst);
            curDst[ 0] = curSrc[ 0];
            curDst[ 1] = curSrc[ 1];
            curDst[ 2] = curSrc[ 2];
            curDst[ 3] = curSrc[ 3];
            curDst[ 4] = curSrc[ 4];
            curDst[ 5] = curSrc[ 5];
            curDst[ 6] = curSrc[ 6];
            curDst[ 7] = curSrc[ 7];
            curDst[ 8] = curSrc[ 8];
            curDst[ 9] = curSrc[ 9];
            curDst[10] = curSrc[10];
            curDst[11] = curSrc[11];
            curDst[12] = curSrc[12];
            curDst[13] = curSrc[13];
            curDst[14] = curSrc[14];
            curDst[15] = curSrc[15];
            curDst -= 16;
            curSrc -= 16;
        }
    }
    // 8ABC
    if (dstAlign != 0) {
        // 8AE4
        memmove(dst, src, dstAlign);
    }
    return dst;
}

void *sceKernelMemmoveWithFill(void *dst, void *src, u32 size, s32 fill)
{
    void *realDst = (void*)((u32)dst & 0x1FFFFFFF);
    void *realSrc = (void*)((u32)src & 0x1FFFFFFF);
    if (realDst == realSrc)
        return dst;
    u32 dist = realDst - realSrc;
    if ((dist & 3) != 0) {
        // 8ED0
        return memmove(dst, src, size);
    }
    if (((0x2C >> (((u32)dst >> 29) & 7)) & 1) != 0) {
        // 8ED0
        return memmove(dst, src, size);
    }
    if (dist + 63 < 126) {
        // 8ED0
        return memmove(dst, src, size);
    }
    u32 dstAlign = (((u32)dst / 64) * 64) - (u32)dst;
    void *dstEnd = dst + size;
    if (dstAlign >= size) {
        // 8EC0
        memmove(dst, src, size);
        return dst;
    }
    u32 dstEndAlign = (u32)dstEnd & 0x3F;
    if (realDst >= realSrc) {
        // 8D5C
        void *srcEnd = src + size;
        if (dstEndAlign != 0) {
            // 8E94
            memmove(dstEnd - dstEndAlign, srcEnd - dstEndAlign, dstEndAlign);
            sceKernelMemset(srcEnd - dstEndAlign, fill, dstEndAlign);
        }
        // 8D68
        if (size != dstAlign + dstEndAlign) {
            u32 alignedSize = size - dstAlign - dstEndAlign;
            s32 *curDst = dst + dstAlign + alignedSize - 64;
            s32 *curSrc = src + dstAlign + alignedSize - 64;
            // 8D94
            while (curDst >= dst + dstAlign) {
                pspCache(0x18, curDst);
                curDst[ 0] = curSrc[ 0];
                curDst[ 1] = curSrc[ 1];
                curDst[ 2] = curSrc[ 2];
                curDst[ 3] = curSrc[ 3];
                curDst[ 4] = curSrc[ 4];
                curDst[ 5] = curSrc[ 5];
                curDst[ 6] = curSrc[ 6];
                curDst[ 7] = curSrc[ 7];
                curDst[ 8] = curSrc[ 8];
                curDst[ 9] = curSrc[ 9];
                curDst[10] = curSrc[10];
                curDst[11] = curSrc[11];
                curDst[12] = curSrc[12];
                curDst[13] = curSrc[13];
                curDst[14] = curSrc[14];
                curDst[15] = curSrc[15];
                pspCache(0x18, curSrc);
                curSrc[ 0] = curSrc[ 1] = curSrc[ 2] = curSrc[ 3] = fill;
                curSrc[ 4] = curSrc[ 5] = curSrc[ 6] = curSrc[ 7] = fill;
                curSrc[ 8] = curSrc[ 9] = curSrc[10] = curSrc[11] = fill;
                curSrc[12] = curSrc[13] = curSrc[14] = curSrc[15] = fill;
                curDst -= 16;
                curSrc -= 16;
            }
        }
        // 8E6C
        if (dstAlign == 0)
            return dst;
        memmove(dst, src, dstAlign);
        sceKernelMemset(src, fill, dstAlign);
    } else {
        if (dstAlign != 0) {
            // 8D3C
            memmove(dst, src, dstAlign);
            sceKernelMemset(src, fill, dstAlign);
        }
        // 8BDC
        if (size != dstAlign + dstEndAlign) {
            s32 *curDst = dst + dstAlign;
            s32 *curSrc = src + dstAlign;
            // 8BFC
            while (curDst < dst + size - dstEndAlign) {
                pspCache(0x18, curDst);
                curDst[ 0] = curSrc[ 0];
                curDst[ 1] = curSrc[ 1];
                curDst[ 2] = curSrc[ 2];
                curDst[ 3] = curSrc[ 3];
                curDst[ 4] = curSrc[ 4];
                curDst[ 5] = curSrc[ 5];
                curDst[ 6] = curSrc[ 6];
                curDst[ 7] = curSrc[ 7];
                curDst[ 8] = curSrc[ 8];
                curDst[ 9] = curSrc[ 9];
                curDst[10] = curSrc[10];
                curDst[11] = curSrc[11];
                curDst[12] = curSrc[12];
                curDst[13] = curSrc[13];
                curDst[14] = curSrc[14];
                curDst[15] = curSrc[15];
                pspCache(0x18, curSrc);
                curSrc[ 0] = curSrc[ 1] = curSrc[ 2] = curSrc[ 3] = fill;
                curSrc[ 4] = curSrc[ 5] = curSrc[ 6] = curSrc[ 7] = fill;
                curSrc[ 8] = curSrc[ 9] = curSrc[10] = curSrc[11] = fill;
                curSrc[12] = curSrc[13] = curSrc[14] = curSrc[15] = fill;
                curDst += 16;
                curSrc += 16;
            }
        }
        // 8CD4
        if (dstEndAlign != 0) {
            // 8D0C
            size += src - dstEndAlign;
            memmove(dstEnd - dstEndAlign, size, dstEndAlign);
            sceKernelMemset(size, fill, dstEndAlign);
        }
    }
    return dst;
}

s32 *sceKernelFillBlock64(void *dst, s32 c, u32 size)
{
    void *end = dst + size;
    s32 *curDst = dst;
    // 8EEC
    while (curDst != end) {
        pspCache(0x18, curDst);
        curDst[ 0] = curDst[ 1] = curDst[ 2] = curDst[ 3] = c;
        curDst[ 4] = curDst[ 5] = curDst[ 6] = curDst[ 7] = c;
        curDst[ 8] = curDst[ 9] = curDst[10] = curDst[11] = c;
        curDst[12] = curDst[13] = curDst[14] = curDst[15] = c;
        curDst += 16;
    }
    return dst;
}

