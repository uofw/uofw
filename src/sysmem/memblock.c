SceUID sceKernelAllocMemoryBlock(const char *name, u32 type, u32 size, SceSysmemMemoryBlockAllocOption *opt)
{
    s32 oldK1 = pspShiftK1();
    if (!pspK1PtrOk(name) || !pspK1StaBufOk(a3, size)) {
        pspSetK1(oldK1);
        return 0x800200D3;
    }
    if (opt != NULL && opt->size != 4) {
        pspSetK1(oldK1);
        return 0x800200D2;
    }
    // D088
    if (type >= 2) {
        pspSetK1(oldK1);
        return 0x800200D8;
    }
    SceUID id = sceKernelAllocPartitionMemory(2, name, type, size, 0);
    pspSetK1(oldK1);
    return id;
}

s32 sceKernelFreeMemoryBlock(SceUID id)
{
    s32 oldK1 = pspShiftK1();
    s32 ret = sceKernelFreePartitionMemory(id);
    pspSetK1(oldK1);
    return ret;
}
s32 sceKernelGetMemoryBlockAddr(SceUID id, u32 *addrPtr)
{
    s32 oldK1 = pspShiftK1();
    if (!pspK1StaBufOk(addrPtr, sizeof *addrPtr)) {
        pspSetK1(oldK1);
        return 0x800200D3;
    }
    *addrPtr = sceKernelGetBlockHeadAddr(id);
    pspSetK1(oldK1);
    return 0;
}

