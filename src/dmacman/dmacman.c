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
int sceKernelDmaOpEnQueue(u32 *arg0) { }

//0x5D4
int sceKernelDmaOpDeQueue(u32 *arg0) { }

//0x6DC
void sceKernelDmaOpAllCancel() { }

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
int sceKernelDmaOpSetupLink(u32 *arg0, int arg1, u32 *arg2) { }

//0xA64
void sceKernelDmaOpSync() { }

//0xBCC
static void sub_BCC() { }

//0xC70
int sceKernelDmaOpQuit(u32 *arg0) { }

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