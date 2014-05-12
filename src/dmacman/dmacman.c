#include <dmacman.h>
#include <common_imp.h>
#include <common_asm.h>

#include <interruptman.h>
#include <sysmem_sysclib.h>

#define DMACMAN_VERSION_MAJOR   (1)
#define DMACMAN_VERSION_MINOR   (18)

void *g_8256;

SCE_MODULE_INFO("sceDMAManager", SCE_MODULE_KERNEL | SCE_MODULE_ATTR_EXCLUSIVE_START |
                                 SCE_MODULE_ATTR_EXCLUSIVE_LOAD | SCE_MODULE_ATTR_CANT_STOP, 
                                 DMACMAN_VERSION_MAJOR, DMACMAN_VERSION_MINOR);
SCE_MODULE_BOOTSTART("_sceDmacManModuleStart");
SCE_MODULE_REBOOT_BEFORE("_sceDmacManModuleRebootBefore");
SCE_SDK_VERSION(SDK_VERSION);

s32 _sceDmacManModuleStart(SceSize args __attribute__((unused)), void *argp __attribute__((unused)))
{
    return SCE_ERROR_OK;
}

s32 _sceDmacManModuleRebootBefore(SceSize args __attribute__((unused)), void *argp __attribute__((unused)))
{
    return SCE_ERROR_OK;
}

//0x300
s32 DmacManForKernel_32757C57(u32 arg0)
{
    *((int)g_8256 + 2072) = arg0 ? arg0 : 7156;
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
            u32 *unk = *((int)g_8256 + 2060);
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
            *((int)g_8256 + 2060) = op;
            ThreadManForKernel_812346E4(*((int)g_8256 + 2076), op->unk24);
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

    if (!*(g_8256 + 2064))
        *(g_8256 + 2064) = op;

    op->unk0 = 0;
    op->unk4 = *(g_8256 + 2068);
    if (*(g_8256 + 2068)) {
        **(g_8256 + 2068) = op;
    }

    *(g_8256 + 2068) = op;
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
        if (*(g_8256 + 2068) == op) {
            *(g_8256 + 2068) = op->unk4;
        }

        // if op == tail
        //    tail = op->prev;
        if (*(g_8256 + 2064) == op) {
            *(g_8256 + 2064) = op->unk0;
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
    SceDmaOp *op = *(g_8192 + 2128);
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

    op = *(g_8192 + 2128);
    while (op) {
        SceDmaOp *tmp = op->unk0;
        op->unk0 = NULL;
        op->unk4 = NULL;
        op = tmp;
    }

    *(g_8192 + 2128) = NULL; // head, I think.
    *(g_8192 + 2132) = NULL; // tail, I think.
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
u32 sceKernelDmaOpSetupLink(SceDmaOp *op, s32 command, u32 *timeout)
{

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
        return sceKernelWaitEventFlag(*(g_8192 + 2140), op1->unk24, 33, NULL, NULL) ?
            SCE_ERROR_KERNEL_ERROR : SCE_ERROR_OK;
    case 2:
        err = sceKernelWaitEventFlag(*(g_8192 + 2140), op1->unk24, 33, NULL, timeout);
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
    *(g_8192 + 2120) &= ~(1 << unk);
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
int DmacManForKernel_E18A93A5(void *arg0, void *arg1) { }

//0xFC0
void sceKernelDmaOpAssignMultiple() { }

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

//0x1C14
void sub_1C14() { }