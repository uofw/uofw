#include <dmacman.h>
#include <common_imp.h>

#include <interruptman.h>
#include <sysmem_sysclib.h>

#define DMACMAN_VERSION_MAJOR   (1)
#define DMACMAN_VERSION_MINOR   (18)


typedef struct {
    u32 unk0[16];     //0 first 8 are mode1, second 8 are mode 2... whatever that means.
    sceKernelDmaOperation ops[32]; //64
    u32 unk2112;      // excluded channels?
    u32 unk2116;      // reserved channels?
    u32 unk2120;      // inUse? bit-vector where 1 means ops[bit] is in use
    sceKernelDmaOperation *op2124; // Free list
    sceKernelDmaOperation *op2128; // Likely first/head
    sceKernelDmaOperation *op2132; // Likely last/tail
    u32 unk2136;      // ddr flush func
    u32 unk2140;      // evid passed to sceKernelWaitEventFlag
} SceDmacMan;

SceDmacMan g_dmacman; //8192

void *g_8256;

SCE_MODULE_INFO("sceDMAManager", SCE_MODULE_KERNEL | SCE_MODULE_ATTR_EXCLUSIVE_START |
                                 SCE_MODULE_ATTR_EXCLUSIVE_LOAD | SCE_MODULE_ATTR_CANT_STOP, 
                                 DMACMAN_VERSION_MAJOR, DMACMAN_VERSION_MINOR);
SCE_MODULE_BOOTSTART("_sceDmacManModuleStart");
SCE_MODULE_REBOOT_BEFORE("_sceDmacManModuleRebootBefore");
SCE_SDK_VERSION(SDK_VERSION);

inline static s32 pspBitrev(s32 a) {
    s32 ret;
    asm __volatile__ ("bitrev %0, %1;" : "=r" (ret) : "r"(a));
    return ret;
}

inline static s32 pspClz(s32 a) {
    s32 ret;
    asm __volatile__ ("clz %0, %1" : "=r"(ret) : "r" (a));
    return ret;
}

inline static s32 lsbPosition(s32 a) {
    return pspClz(pspBitrev(a));
}

s32 _sceDmacManModuleStart(SceSize args __attribute__((unused)), void *argp __attribute__((unused)))
{
    u32 intr;
    s32 err = SCE_ERROR_OK;

    intr = sceKernelCpuSuspendIntr();
    memset(&g_dmacman.ops, 0, 2048);

    g_dmacman.op2124 = &g_dmacman.ops[0];
    for (int i = 0; i < 31; i++) {
        g_dmacman.ops[i].pNext = &g_dmacman.ops[i+1];
        g_dmacman.ops[i+1].pPrev = &g_dmacman.ops[i];
        g_dmacman.ops[i].uiFlag = 0x1000;
        g_dmacman.ops[i].evpat = 1 << i;
    }
    g_dmacman.ops[31].uiFlag = 0x1000;
    g_dmacman.ops[31].evpat = 1 << i;

    g_dmacman.unk2128 = NULL;
    g_dmacman.unk2132 = NULL;
    g_dmacman.unk2140 = 0;

    sceKernelRegisterSuspendHandler(20, suspendHandler, NULL);
    sceKernelRegisterResumeHandler(20, resumeHandler, NULL);

    err |= sceKernelRegisterIntrHandler(22, 2, interruptHandler, 0, NULL);
    err |= sceKernelEnableIntr(22);
    err |= sceKernelRegisterIntrHandler(23, 2, interruptHandler, 1, NULL);
    err |= sceKernelEnableIntr(23);

    if (err) {
        sceKernelReleaseIntrHandler(22);
        sceKernelReleaseIntrHandler(23);
        err = 0x800202BC;
    }

    sceKernelDmaRegisterDdrFlush(_dummyDdrFlush);

    sceKernelCpuResumeIntr(intr);

    g_dmacman.unk2140 = sceKernelCreateEventFlag("SceDmacmanEvflag", 0x200, 0, 0);

    return err;
}

s32 _sceDmacManModuleRebootBefore(SceSize args __attribute__((unused)), void *argp __attribute__((unused)))
{
    u32 intr, intr2;
    s32 err = SCE_ERROR_OK;

    intr = sceKernelCpuSuspendIntr();

    for (int j = 0; j < 8; j++) {
        HW(0xBC900100 + j*32) |= 4;
        while (!(HW(0xBC900100 + j*32) & 0x2));
    }

    for (int j = 0; j < 8; j++) {
        HW(0xBCA00100 + j*32) |= 4;
        while (!(HW(0xBCA00100 + j*32) & 0x2));
    }

    sceKernelDmaOpAllCancel();

    HW(0xBC900100 + 48) &= ~0x1;
    HW(0xBC900100 + 8) = 0xFF;
    HW(0xBC900100 + 16) = 0xFF;
    err |= sceKernelReleaseIntrHandler(22);
    intr2 = sceKernelCpuSuspendIntr();
    HW(0xBC100000) &= ~0x20;
    sceKernelCpuResumeIntr(intr2);

    HW(0xBCA00100 + 48) &= ~0x1;
    HW(0xBCA00100 + 8) = 0xFF;
    HW(0xBCA00100 + 16) = 0xFF;
    err |= sceKernelReleaseIntrHandler(23);
    intr2 = sceKernelCpuSuspendIntr();
    HW(0xBC100000) &= ~0x40;
    sceKernelCpuResumeIntr(intr2);

    sceKernelCpuResumeIntr(intr);

    if (err) {
        return 0x800202BC;
    }

    sceKernelDeleteEventFlag(g_dmacman.unk40);
    return SCE_ERROR_OK;
}
s32 sceKernelDmaExit(SceSize args, void *argp) __attribute__((alias ("_sceDmacManModuleRebootBefore")));


//0x300
s32 sceKernelDmaRegisterDdrFlush(void *ddrFlushFunc)
{
    g_dmacman.unk2136 = ddrFlushFunc ? ddrFlushFunc : _dummyDdrFlush;
    return SCE_ERROR_OK;
}

//0x328
void sceKernelDmaSoftRequest(u32 arg0, u32 arg1, u32 arg2, u32 arg3)
{
    u32 baseaddr = arg0? 0xBCA00000 : 0xBC900000;

    // TODO: make cleaner
    if (arg3) {
        if (arg2) {
            HW(baseaddr + 44) = (1 << arg1);
        } else {
            HW(baseaddr + 40) = (1 << arg1);
        }
    } else if (arg2) {
        HW(baseaddr + 32) = (arg2 << arg1);
    } else {
        HW(baseaddr + 34) = (1 << arg1); // XXX
    }

    return SCE_ERROR_OK;
}

//0x388
s32 sceKernelDmaOpFree(sceKernelDmaOperation *op)
{
    s32 ret;
    u32 intr;

    intr = sceKernelCpuSuspendIntr();

    if (op) {
        if (op->unk28 & 0x1000) {
            ret = 0x800202C3;
        } else if (arg0->unk28 & 0x1) {
            ret = 0x800202BE;
        } else if (arg0->unk28 & 0x2) {
            ret = 0x800202C0;
        } else {
            sceKernelDmaOperation *unk = g_dmacman.unk2124;
            op->unk0 = unk;
            op->unk4 = 0;
            op->unk16 = 0;
            op->unk28 = 0x1000;
            op->unk30 = 0xFFFF;
            op->unk32 = 0;
            op->unk36 = 0;
            op->unk40 = 0;
            op->unk44 = 0;
            op->unk48 = 0;
            op->unk56 = 0;
            if (unk) 
                unk->unk4 = op;
            g_dmacman.unk2124 = op;
            sceKernelClearEventFlag(g_dmacman->unk2140, op->unk24);
            ret = SCE_ERROR_OK;
        }

    } else {
        ret = 0x800202CF;
    }

    sceKernelCpuResumeIntr(intr);
    return ret;
}

//0x488
s32 sceKernelDmaOpEnQueue(sceKernelDmaOperation *op)
{
    u32 intr;
    s32 ret;
    sceKernelDmaOperation *cur = op;
    sceKernelDmaOperation *tail = op;

    if (!op)
        return 0x800202CF;
    if (op->unk8)
        return 0x800202CD;

    intr = sceKernelCpuSuspendIntr();
    for (; cur; cur = cur->next) {
        if (cur->unk28 & 0x1) {
            ret = 0x800202BE;
            goto cleanup;
        } else if (cur->unk28 & 0x1000) {
            ret =  0x800202C3;
            goto cleanup;
        } else if (!(cur->unk28 & 0x100)) {
            ret = 0x800202C1;
            goto cleanup;
        } else if (cur->unk28 & 0x2) {
            ret = 0x800202C0;
            goto cleanup;
        } else if (cur->unk28 & 0x70) {
            ret = 0x800202C4;
            goto cleanup;
        }
        prev = cur;
    }

    // This seems redundant
    if (!g_dmacman.unk2128)
        g_dmacman.unk2128 = op;

    op->unk0 = 0;
    op->unk4 = g_dmacman.unk2132;
    if (g_dmacman.unk2132) {
        g_dmacman.unk2132->unk0 = op;
    }

    g_dmacman.unk2128 = op;
    op->unk28 |= 1;
    _sceKernelDmaSchedule();
    ret = SCE_ERROR_OK;

cleanup:
    sceKernelCpuResumeIntr(intr);
    return ret;
}

//0x5D4
s32 sceKernelDmaOpDeQueue(sceKernelDmaOperation *op)
{
    u32 intr;
    s32 ret;

    intr = sceKernelCpuSuspendIntr();
    if (!op) {
        ret = 0x800202CF
    } else if (op->unk28 & 0x1000) {
        ret = 0x800202C3;
    } else if (!(op->unk28 & 0x100)) {
        ret = 0x800202C1;
    } else if (op->unk28 & 0x2) {
        ret = 0x800202C0;
    } else if (!(op->unk28) & 0x1) {
        ret = 0x800202BF;
    } else {

        // I'm not sure which is next and which is prev.

        // if op->next
        //    op->next->prev = op->prev
        if (op->unk4) {
            op->unk4->unk0 = op->unk0;
        }

        // if op->prev
        //    op->prev->next = op->next
        if (op->unk0) {
            op->unk0->unk4 = op->unk4;
        }

        // if op == head
        //    head = op->next
        if (g_dmacman.unk2132 == op) {
            g_dmacman.unk2132 = op->unk4;
        }

        // if op == tail
        //    tail = op->prev;
        if (g_dmacman.unk2128 == op) {
            g_dmacman.unk2128 = op->unk0;
        }

        op->unk0 = NULL;
        op->unk28 &= ~0x1;
        ret = SCE_ERROR_OK;
    }

    sceKernelCpuResumeIntr(intr);
    return ret;
}

//0x6DC
s32 sceKernelDmaOpAllCancel()
{
    u32 intr;
    s32 ret;
    sceKernelDmaOperation *op = g_dmacman.unk2128;
    if (!op) {
        return 0x800202BF;
    }

    intr = sceKernelCpuSuspendIntr();
    for (int i = 0; i < 16; i++) {
        if (*(g_8192 + i*4)) {
            sceKernelCpuResumeIntr(intr);
            return 0x800202C0;
        }
    }

    op = g_dmacman.unk2128;
    while (op) {
        sceKernelDmaOperation *tmp = op->unk0;
        op->unk0 = NULL;
        op->unk4 = NULL;
        op = tmp;
    }

    g_dmacman.unk2128 = NULL; // head, I think.
    g_dmacman.unk2132 = NULL; // tail, I think.
    sceKernelCpuResumeIntr(intr);
    return SCE_ERROR_OK;
}

//0x798
int sceKernelDmaOpSetCallback(sceKernelDmaOperation *op, int (*)(int, int) func, int arg2)
{
    if (!op) 
        return 0x800202CF;
    if (op->unk28 & 0x1000)
        return 0x800202C3;
    if (!(op->unk28 & 0x800))
        return 0x800202C1;
    if (op->unk28 & 0x2)
        return 0x800202C0;
    if (op->unk28 & 0x70)
        return 0x800202C4;

    op->unk20 = arg2;
    op->callback = func;
    return SCE_ERROR_OK;

}

//0x808
s32 sceKernelDmaOpSetupMemcpy(sceKernelDmaOperation *op, s32 arg1, s32 arg2, s32 arg3)
{
    if (!op)
        return 0x800202CF;
    if (op->unk28 & 0x1000)
        return 0x800202C3;
    if (!(op->unk28 & 0x100))
        return 0x800202C1;
    if (op->unk28 & 0x2)
        return 0x800202C0;
    if (op->unk28 & 0x70)
        return 0x800202C4;
    if (arg3 < 0x1000) {
        op->unk32 = arg2;
        op->unk36 = arg1;
        op->unk40 = 0;
        op->unk44 = arg3 & 0x8C4BF000;
        op->unk48 |= 0xC001;
    }

    return SCE_ERROR_OK;
}

//0x8A8
s32 sceKernelDmaOpSetupNormal(sceKernelDmaOperation *op, s32 arg1, s32 arg2, s32 arg3, s32 arg4)
{
    if (!op)
        return 0x800202CF;
    if (op->unk28 & 0x1000)
        return 0x800202C3;
    if (!(op->unk28 & 0x100))
        return 0x800202C1;
    if (op->unk28 & 0x2)
        return 0x800202C0;
    if (op->unk28 & 0x70)
        return 0x800202C4;

    op->unk32 = arg3;
    op->unk36 = arg2;
    op->unk40 = 0;
    op->unk44 = arg4 | 0x80000000;
    op->unk48 |= (arg1 | 0xC001);


    return SCE_ERROR_OK;
}

//0x938
u32 sceKernelDmaOpSetupLink(sceKernelDmaOperation *op, arg1 command, void *arg2)
{
    void *unk0;
    void *unk1;

    if (!op)
        return 0x800202CF;
    if (op->unk28 & 0x1172 != 0x100) {
        if (op->unk28 & 0x1000)
            return 0x800202C3;
        if (op->unk28 & 0x1172 != 0)
            return 0x800202C1;
        if (op->unk28 & 0x2)
            return 0x800202C0; // unreachable?
        return 0x800202C4;
    }

    op->unk48 |= (arg1 | 0xC001);
    if (!(arg2 & 0x3)) {
        for (unk0 = KUNCACHED(arg2); unk0->unk8; unk0 = KUNCACHED(unk->unk8));
        op->unk60 = unk;
        unk0->unk12 |= 0x80000000
        unk1 = KUNCACHED(arg2);
    } else {
        unk1 = KCACHED(arg2 & 0xFFFFFFF8);
    }

    op->unk56 = unk1;
    op->unk32 = unk1->unk0;
    op->unk36 = unk1->unk4;
    op->unk40 = unk1->unk8;
    op->unk44 = unk1->unk12;
    if (!(arg2 & 0x3)) {
        g_dmacman.unk2136(4);
    }

    return SCE_ERROR_OK;
}

//0xA64
s32 sceKernelDmaOpSync(sceKernelDmaOperation *op, s32 command, u32 *timeout)
{
    s32 err;

    if (!op)
        return 0x800202CF;
    if (op->unk28 & 0x1000)
        return 0x800202C3;
    if (!(op->unk28 & 0x100))
        return 0x800202C1;
    if (op->unk28 & 0x20)
        return 0x800202C6;

    switch (command) {
    case 0: {
        if (op->unk28 & 0x40)
            return SCE_ERROR_OK;
        if (op->unk28 & 0x10)
            return 0x800202C7;
        if (op->unk28 & 0x1)
            return 0x800202BE;
        if (op->unk28 & 0x2)
            return 0x800202C0;
        return 0x800202BF
    }
    case 1:
        return sceKernelWaitEventFlag(g_dmacman.unk2140, op1->unk24, 33, NULL, NULL) ?
            SCE_ERROR_KERNEL_ERROR : SCE_ERROR_OK;
    case 2:
        err = sceKernelWaitEventFlag(g_dmacman.unk2140, op1->unk24, 33, NULL, timeout);
        if (err == SCE_ERROR_OK)
            return SCE_ERROR_OK;
        if (err == SCE_ERROR_KERNEL_WAIT_TIMEOUT) {
            sub_BCC(op1);
            return 0x800202C2;
        }
        // intentional fall through
    default:
        return SCE_ERROR_KERNEL_ERROR;
    }
}

//0xBCC doDmaOpQuit?
static void sub_BCC(sceKernelDmaOperation *op)
{
    s32 unk = op->unk30;
    s32 unk1 = (unk & 7) << 5;
    s32 base = (unk >> 3 == 0) ? 0xBCA00000 : 0xBC900000;
    s32 address = unk1 + base + 0x100;

    HW(address) &= ~0x1;

    // Wait for the write to go through
    while (HW(address) & 0x1);

    op->unk28 = 0x20;
    op->unk30 = 0xFFFF;
    g_dmacman.unk2120 &= ~(1 << unk);
    *(g_8192 + unk1 * 4) = 0;

    HW(base + 8) = 1 << unk1;
    HW(base + 16) = 1 << unk1;

}

//0xC70
s32 sceKernelDmaOpQuit(sceKernelDmaOperation *op)
{
    u32 intr;
    s32 ret = SCE_ERROR_OK;

    intr = sceKernelCpuSuspendIntr();
    if (!op)
        ret = 0x800202CF;
    else if (op->unk28 & 0x1000)
        ret = 0x800202C3;
    else if (!(op->unk28 & 0x100))
        ret = 0x800202C1;
    else if (op->unk28 & 0x40)
        ret = 0x800202BF;
    else if (op->unk28 & 0x1)
        ret = 0x800202BE;
    else if (op->unk28 & 0x10)
        ret = 0x800202C7;
    else if (op->unk28 & 0x20)
        ret = 0x800202C6;
    else if (!(op->unk28 & 0x2))
        ret = SCE_ERROR_KERNEL_ERROR;
    else
        sub_BCC(op);

    return ret;
}

//0xD80
s32 sceKernelDmaOpConcatenate(sceKernelDmaOperation *op, void *arg1)
{
    void *unk;
    u32 intr;
    s32 ret;
    s32 addr;

    u32 addr16;

    // s1 = op
    // s2 = arg1
    if (!op)
        return 0x800202CF;

    for (unk = KUNCACHED(arg1); unk->unk8; unk = KUNCACHED(unk->unk8));
    unk->unk12 |= 0x80000000; // unk->unk12 = KCACHED(unk->unk12)?

    intr = sceKernelCpuSuspendIntr();

    if (op->unk28 & 0x1170 != 0x100) {
        if (op->unk28 & 0x1000)
            ret = 0x800202C3;
        else if (!(op->unk28 & 0x100))
            ret = 0x800202C1;
        else
            ret = 0x800202C4;
        goto cleanup;
    }

    if (!(op->unk28 & 0x2)) {
        if (op->unk28 & 0x1) {
            op->unk60->unk8 = arg1;
            op->unk60 = unk;
            ret = SCE_ERROR_OK;
        } else
            ret = SCE_ERROR_KERNEL_ERROR;
        goto cleanup;
    }

    addr = (op->unk30 / 8) ? 0xBCA00100 : 0xBC900100;
    addr16 = HW(addr + 16) | 4;
    HW(addr + 16) = addr 16;
    if (!HW(addr + 8)) {
        s32 mask = addr16 & 0x1003800;
        if (mask ^ 0x1000000 || mask ^ 0x1001000 || mask == 0x1003000) {
            ret = 0x800202BC;
            goto cleanup;
        }

        while (!(HW(addr + 16) & 0x2));

        if (!(HW(addr + 12) & 0xFFF)) {
            ret = 0x800202C4;
            goto cleanup;
        }

        HW(addr + 16) &= ~0x1;
        while(HW(addr + 16) & 0x1);

        HW(addr + 8) = arg1;
        HW(addr + 16) |= 1;
    } else {
        op->unk60->unk8 = arg1;
    }

    op->unk60 = unk;
    HW(addr + 16) &= 0xFFF00001;
    g_dmacman->unk2136(4);
    ret = SCE_ERROR_OK;

cleanup:
    sceKernelCpuSuspendIntr(intr);
    return ret;
}

//0xFC0
s32 sceKernelDmaOpAssignMultiple(sceKernelDmaOperation **ops, s32 size)
{
    u32 intr;
    u32 unk0 = size;

    if (size > 16)
        return 0x800202BD;

    for (int i = 0; i < 16; i++) {
        if (ops[0]->unk52 >> i & 1)
            unk0--;
    }

    if (unk0 < 0)
        return 0x800202BD;

    for (int i = size - 1; i; i--) {
        sceKernelDmaOperation *op = ops[i];
        if (!op)
            return 0x800202CF;
        if (op->unk28 & 0x1000)
            return 0x800202C3;
        if (!(op->unk28 & 0x100))
            return 0x800202C1;
        if (op->unk28 & 0x1)
            return 0x800202BE;
        if (op->unk28 & 0x2)
            return 0x800202C0;
        if (op->unk28 & 0x70)
            return 0x800202C4;
    }

    intr = sceKernelCpuSuspendIntr();
    for (int i = 0; i < size - 1; i++) {
        ops[i]->unk12 = ops[i+1];
        ops[i+1]->unk8 = ops[i];
        ops[i]->unk0 = 0;
        ops[i]->unk4 = 0;
        ops[i+1]->unk52 = ops[i]->unk52;
    }
    ops[0]->unk54 = size;
    sceKernelCpuResumeIntr(intr);
    return SCE_ERROR_OK;
}

//0x1148
static s32 interruptHandler(s32 arg0 __attribute__((unused)), s32 arg1)
{
    u32 intr = sceKernelCpuSuspendIntr();
    u32 addr = arg1? 0xBCA00000 : 0xBC900000;

    u32 unk0, unk1, unk3 = 0, unk4;

    unk0 = HW(addr + 4);
    unk1 = HW(addr + 12);
    unk2 = (unk1 | unk2) & 0xFF;
    HW(addr + 8) = 0;

    for (int i = 0; unk2 > (1 << i); i++) {
        if (unk2 & (i << i)) {
            if (!g_dmacman.unk0[i + 8*arg1]) {
                // 14CC
                if (g_dmacman.unk2132 & (1 << i))
                    Kprintf("Fatal error: Stray interrupt occurred\n");
            } else {
                sceKernelDmaOperation *op = g_dmacman.unk0[i + 8*arg1];
                op->unk32 = HW(addr + 0x100);
                op->unk36 = HW(addr + 0x100 + 4);
                op->unk40 = HW(addr + 0x100 + 8);
                op->unk44 = HW(addr + 0x100 + 12);
                op->unk48 = HW(addr + 0x100 + 16);
                if (unk0 & 1 << i) {
                    if (!HW(addr + 0x100 + 8) && ! (HW(addr + 0x100 + 16) & 0x1)) {
                        g_dmacman.unk2120 &= ~(1 << i);
                        unk3 |= op->unk24;
                        op->unk28 |= 0x40;
                        op->unk28 &= ~0x2;
                        op->unk30 = 0xFFFF;
                        g_dmacman.unk0[i + 8*arg1] = NULL;
                    }
                    HW(addr + 16) = (1 << i);
                    unk4 = 0;
                } else {
                    // 0x1438
                    if (HW(addr + 0x100 + 8) && (HW(addr + 0x100 + 16) & 0x1)) {
                        sub_BCC(op);
                    }
                    g_dmacman.unk2120 &= ~(1 << i);
                    g_dmacman.unk0[i + 8*arg1] = NULL;
                    unk3 |= op->unk24;
                    op->unk28 |= 0x40;
                    op->unk28 &= ~0x2;
                    op->unk30 = 0xFFFF;
                    HW(addr + 16) = (1 << i);
                    unk4 = -1
                }
                // 0x139C
                if (op->unk16 && op->unk16(op, unk4, i + 8*arg1, op->unk20) == 1) {
                    if (op->unk56) {
                        op->unk32 = op->unk56->unk0;
                        op->unk36 = op->unk56->unk4;
                        op->unk40 = op->unk56->unk8;
                        op->unk46 = op->unk56->unk12;
                    }
                    if (!g_dmacman.unk2128)
                        g_dmacman.unk2128 = op;
                    op->unk0 = NULL;
                    if (g_dmacman.unk2132) {
                        g_dmacman.unk2132->unk0 = op;
                    }
                    op->unk4 = g_dmacman.unk2132;
                    g_dmacman.unk2132 = op;
                    op->unk28 |= 0x1;
                }
            }
        }
    }
    // 11D4
    sceKernelSetEventFlag(g_dmacman.unk2140, unk3);
    if (!g_dmacman.unk2128)
        _sceKernelDmaSchedule();
    if ((g_dmacman.unk2120 | g_dmacman.unk2112) & 0xFF) {
        u32 intr2 = sceKernelCpuSuspendIntr();
        HW(0xBC100000) &= ~(1 << 6);
        sceKernelCpuResumeIntr(intr2);
    }
    if ((g_dmacman.unk2120 | g_dmacman.unk2112) & 0xFF00) {
        u32 intr2 = sceKernelCpuSuspendIntr();
        HW(0xBC100000) &= ~(1 << 5);
        sceKernelCpuResumeIntr(intr2);
    }
    sceKernelCpuResumeIntr(intr);
    return -1;

}

//0x14f4
static void _sceKernelDmaSchedule()
{
    sceKernelDmaOperation *t1 = g_dmacman.unk2128;
    u32 t2 = g_dmacman.unk2120 & g_dmacman.unk2112;
    u32 a3 = t2 ^ 0xFFFF;

    for (; t1 && a3; t1 = t1->unk0) {
        if (t1->unk12 && !t1->unk8) {
            u32 t0 = t1->unk52 & ~t2;
            u32 a1 = t1->unk54;
            for (int i = 0; i < 16; i++) {
                a1 -= (t0 >> i) & 0x1;
            }
            if (a1 < 0) {
                if (t1->unk0) {
                    sceKernelDmaOperation *a0 = t1->unk0;
                    a0->unk4 = t1->unk4;
                    t1->unk4 = a0;
                    t1->unk0 = a0->unk0;
                    if (a0->unk4)
                        a0->unk4->unk0 = a0;
                    if (!a0->unk0)
                        g_dmacman.unk2132 = t1;
                    else
                        t1->unk0->unk4 = t1;
                    a0->unk0 = t0;
                }
                continue;
            }

            // 1618
            for (sceKernelDmaOperation *op = t1, int j = 0; op; j++) {
                if (t0 & (1 << j)) {
                    g_dmacman.unk0[j]->unk30 = j;
                    op = op->unk12;
                    t2 |= (1 << j);
                }
            }
        }

        for (sceKernelDmaOperation *a2 = t1; a2; a2 = a2->unk12)
            // 1644
            if (a2->unk30 < 0) {
                u32 pos = lsbPosition(~t2 & a2->unk52);
                if (pos >= 16) {
                    break;
                }
                g_dmacman.unk0[pos] = t1;
                t1->unk30 = pos;
                t2 |= pos;
            }

            // 1674
            if (!HW(0xBC100080) & 0x20 << (t1->unk30 / 8)) {
                HW(0xBC100080) |= 0x20 << (t1->unk30 / 8);
                u32 base = t1->unk30/8 ? 0xBCA00000 : 0xBC900000;
                HW(base + 48) |= 1;
                HW(base + 52) = 0;

                for (int i = 8; i > 0; i--) {
                    HW(base + 0x100 + 32*i) = 0;
                    HW(base + 0x104 + 32*i) = 0;
                    HW(base + 0x108 + 32*i) = 0;
                    HW(base + 0x10C + 32*i) = 0;
                    HW(base + 0x110 + 32*i) = 0;
                }

                HW(base + 8) = 0xFF;
                HW(base + 16) = 0xFF;
            }
            // 16EC
            HW(base + 0x100 + (a2->unk30 & 7) * 32) = t1->unk32;
            HW(base + 0x104 + (a2->unk30 & 7) * 32) = t1->unk36;
            HW(base + 0x108 + (a2->unk30 & 7) * 32) = t1->unk40;
            HW(base + 0x10C + (a2->unk30 & 7) * 32) = t1->unk44;
            HW(base + 0x110 + (a2->unk30 & 7) * 32) = t1->unk48;
            a2->unk28 |= 2;
            if (a2->unk8 && !a2->unk12) {
                break;
            } else {
                a2->unk28 &= ~0x1;
                if (a2 == g_dmacman.unk2132)
                    g_dmacman.unk2132 = a2->unk4;
                if (a2 == g_dmacman.unk2128)
                    g_dmacman.unk2128 = a2->unk0;
                if (a2->unk4) {
                    a2->unk4->unk0 = a2->unk0;
                    a2->unk4 = NULL;
                }
                if (a2->unk0) {
                    a2->unk0->unk4 = a2->unk4;
                    a2->unk0 = NULL;
                }
            }
        }
    }

    // 15E8
    g_dmacman.unk2120 = ~g_dmacman.unk2112 & t2;
    return;
}

//0x1804
s32 sceKernelDmaChExclude(u32 ch, u32 arg1)
{
    u32 intr;
    u32 ret;

    if (ch > 16)
        return 0x800202CF;

    intr = sceKernelCpuSuspendIntr();
    if (g_dmacman.unk2116 & (1 << ch)) {
        ret = 0x800202C8;
    } else {
        if (!arg2) {
            g_dmacman.unk2112 &= ~(1 << ch);
        } else {
            g_dmacman.unk2112 |= (1 << ch);
            _sceKernelDmacClkEnable(ch / 8);
        }
        ret = SCE_ERROR_OK;
    }
    sceKernelCpuResumeIntr(intr);
    return ret;
}

//0x18CC
void sceKernelDmaChReserve(u32 ch, u32 arg1)
{
    u32 intr;
    u32 ret;

    intr = sceKernelCpuSuspendIntr();
    if (g_dmacman.unk2112 & (1 << ch)) {
        ret = 0x800202C9;
    } else {
        if (arg1 < 0) {
            g_dmacman.unk2116 &= ~(1 << ch);
        } else {
            g_dmacman.unk2116 |= (1 << ch);
        }
        ret = SCE_ERROR_OK;
    }
    sceKernelCpuResumeIntr(intr);
    return ret;
}

//0x1964
sceKernelDmaOperation *sceKernelDmaOpAlloc()
{
    u32 intr = sceKernelCpuSuspendIntr();
    sceKernelDmaOperation *op = g_dmacman.unk2124;
    if (op) {
        if (op->unk28 & 0x1000) {
            g_dmacman.unk2124 = op->unk4;
            op->unk52 = 0xFFFF;
            op->unk16 = 0;
            op->unk30 = 0xFFFF;
            op->unk12 = NULL;
            op->unk8 = NULL;
        } else {
            op = NULL;
        }
    }
    sceKernelCpuResumeIntr(intr);
    return op;
}

//0x19E8
s32 sceKernelDmaOpAssign(sceKernelDmaOperation *op, int arg1, int arg2, int arg3, int arg4)
{
    if (!op)
        return 0x800202CF;
    if (op->unk28 & 0x1000)
        return 0x800202C3;
    if (op->unk28 & 0x1)
        return 0x800202BE;
    if (op->unk28 & 0x2)
        return 0x800202C0;

    op->unk48 = arg3;
    op->unk52 = (arg1 & 0xFF) | (arg2 & 0xFF) << 8;
    op->unk28 = 0x100;
    return SCE_ERROR_OK;
}

//0x1A60
s32 sceKernelDmaOpLLIConcatenate(sceKernelDmaOperation *op1, sceKernelDmaOperation *op2)
{
    sceKernelDmaOperation *head;

    op1->unk60->unk8 = op2;
    g_dmacman.unk2136(4);
    for (*head = KUNCACHED(op2); head->unk8; head = KUNCACHED(head->unk8));
    op1->unk60 = head;
    return SCE_ERROR_OK;
}

//0x1ADC
s32 sceKernelDmaOnDebugMode()
{
    return SCE_ERROR_OK;
}

//0x1AE4
static s32 suspendHandler(s32 unk __attribute__((unused)), void *param __attribute__((unused)))
{
    for (int i = 0; i < 16; i++) {
        if (g_dmacman.unk0[i])
            sub_BCC(g_dmacman.unk0[i]);
    }

    g_dmacman.unk2120 = 0;
    sceKernelDmaOpAllCancel();
    sceKernelDisableIntr(22);
    sceKernelDisableIntr(23);
    return SCE_ERROR_OK;
}

//0x1B74
static s32 resumeHandler(s32 unk __attribute__((unused)), void *param __attribute__((unused)));
{
    if (g_dmacman.unk2112 & 0xFF00) 
        _sceKernelDmacClkEnable(1);
    if (g_dmacman.unk2112 & 0xFF)
        _sceKernelDmacClkEnable(0);

    sceKernelEnableIntr(22);
    sceKernelEnableIntr(23);
    return SCE_ERROR_OK;
}

//0x1BF4 dummyFlush
static s32 _dummyDdrFlush()
{
    pspSync();
    u32 ra = pspGetRa();
    *((ra >> 31 + 2) << 29); // Will this just get optimized out?
    return SCE_ERROR_OK;
}

//0x1C14
static void _sceKernelDmacClkEnable(s32 arg0)
{
    u32 addr = arg0 ? 0xBCA00000 : 0xBC900000;
    if (HW(0xBC100010) & (0x20 << arg0))
        return;
    HW(0xBC100010) |= (0x20 << arg0);
    HW(addr + 48) |= 0x1;
    HW(addr + 52) = 0;
    for (int i = 0; i < 8; i++) {
        HW(addr + 32*i + 256) = 0;
        HW(addr + 32*i + 260) = 0;
        HW(addr + 32*i + 264) = 0;
        HW(addr + 32*i + 268) = 0;
        HW(addr + 32*i + 272) = 0;
    }
    HW(addr + 8) = 0x100;
    HW(addr + 16) = 0x100;
}
