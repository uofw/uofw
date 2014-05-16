#include <dmacman.h>
#include <common_imp.h>
#include <common_asm.h>

#include <interruptman.h>
#include <sysmem_sysclib.h>

#define DMACMAN_VERSION_MAJOR   (1)
#define DMACMAN_VERSION_MINOR   (18)

typedef struct {
    u32 unk0[16];     //0
    SceDmaOp ops[32]; //64
    u32 unk2112;
    u32 unk2116;
    u32 unk2120;      // inUse? bit-vector where 1 means ops[bit] is in use
    SceDmaOp *op2124; // Free list
    SceDmaOp *op2128; // Likely first/head
    SceDmaOp *op2132; // Likely last/tail
    u32 unk2136;
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

s32 _sceDmacManModuleStart(SceSize args __attribute__((unused)), void *argp __attribute__((unused)))
{
    u32 intr;
    s32 err = SCE_ERROR_OK;

    intr = sceKernelCpuSuspendIntr();
    memset(&g_dmacman.ops, 0, 2048);

    g_dmacman.op2124 = &g_dmacman.ops[0];
    for (int i = 0; i < 31; i++) {
        g_dmacman.ops[i].unk0 = &g_dmacman.ops[i+1];
        g_dmacman.ops[i+1].unk4 = &g_dmacman.ops[i];
        g_dmacman.ops[i].unk28 = 0x1000;
        g_dmacman.ops[i].unk24 = 1 << i;
    }
    g_dmacman.ops[31].unk28 = 0x1000;
    g_dmacman.ops[31].unk24 = 1 << i;

    g_dmacman.unk2128 = NULL;
    g_dmacman.unk2132 = NULL;
    g_dmacman.unk2140 = 0;

    sceKernelRegisterSuspendHandler(20, suspendHandler, NULL);
    sceKernelRegisterResumeHandler(20, resumeHandler, NULL);

    err |= sceKernelRegisterIntrHandler(22, 2, interruptHandler, NULL, NULL);
    err |= sceKernelEnableIntr(22);
    err |= sceKernelRegisterIntrHandler(23, 2, interruptHandler, NULL, NULL);
    err |= sceKernelEnableIntr(23);

    if (err) {
        sceKernelReleaseIntrHandler(22);
        sceKernelReleaseIntrHandler(23);
        err = 0x800202BC;
    }

    DmacManForKernel_32757C57(sub_1BF4);

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

//0x300
s32 DmacManForKernel_32757C57(u32 arg0)
{
    g_dmacman.unk2136 = arg0 ? arg0 : 7156;
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
        HW(baseaddr + 34) = (1 << arg1);
    }

    return SCE_ERROR_OK;
}

//0x388
s32 sceKernelDmaOpFree(SceDmaOp *op)
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
            SceDmaOp *unk = g_dmacman.unk2124;
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
s32 sceKernelDmaOpEnQueue(SceDmaOp *op)
{
    u32 intr;
    s32 ret;
    SceDmaOp *cur = op;
    SceDmaOp *tail = op;

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
    sub_14F4();
    ret = SCE_ERROR_OK;

cleanup:
    sceKernelCpuResumeIntr(intr);
    return ret;
}

//0x5D4
s32 sceKernelDmaOpDeQueue(SceDmaOp *op)
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
    SceDmaOp *op = g_dmacman.unk2128;
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
        SceDmaOp *tmp = op->unk0;
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
int sceKernelDmaOpSetCallback(SceDmaOp *op, int (*)(int, int) func, int arg2)
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
s32 sceKernelDmaOpSetupMemcpy(SceDmaOp *op, s32 arg1, s32 arg2, s32 arg3)
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
s32 sceKernelDmaOpSetupNormal(SceDmaOp *op, s32 arg1, s32 arg2, s32 arg3, s32 arg4)
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
u32 sceKernelDmaOpSetupLink(SceDmaOp *op, arg1 command, void *arg2)
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
s32 sceKernelDmaOpSync(SceDmaOp *op, s32 command, u32 *timeout)
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
static void sub_BCC(SceDmaOp *op)
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
s32 sceKernelDmaOpQuit(SceDmaOp *op)
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
s32 sceKernelDmaOpConcatenate(SceDmaOp *op, void *arg1)
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
s32 sceKernelDmaOpAssignMultiple(SceDmaOp **ops, s32 size)
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
        SceDmaOp *op = ops[i];
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

static s32 interruptHandler() { }

//0x14f4
static void sub_14F4() { }

//0x1804
void sceKernelDmaChExclude() { }

//0x18CC
void sceKernelDmaChReserve() { }

//0x1964
u32 *sceKernelDmaOpAlloc() { }

//0x19E8
int sceKernelDmaOpAssign(u32 *arg0, int arg1, int arg2, int arg3, int arg4) { }

//0x1A60
void DmacManForKernel_1FC036B7() { }

//0x1ADC
void sceKernelDmaOnDebugMode() { }

//0x1AE4
static s32 suspendHandler(s32 unk, void *param) { }

//0x1B74
static s32 resumeHandler(s32 unk, void *param) { }

//0x1BF4
static s32 sub_1BF4() { }

//0x1C14
void sub_1C14() { }