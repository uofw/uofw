#include <sysmem_kernel.h>
#include <sysmem_kdebug.h>
#include <sysmem_utils_kernel.h>

#include "intr.h"
#include "partition.h"

// 140D4
u32 mem_extend_nest;

void MemoryProtectInit(u32 size, u32 extSize)
{
    if (sceKernelGetModel() == 0 && sceKernelDipsw(10) == 0 && sceKernelDipsw(12) == 0) // A4B8
        HW(0xBC100040) = (HW(0xBC100040) & 0xFFFFFFFC) | 0x1;
    else {
        // A2E8
        HW(0xBC100040) = (HW(0xBC100040) & 0xFFFFFFFC) | 0x2;
    }
    // A2F8
    if (size > 0x800000)
        size = 0x800000;
    vs32 *ptr = (s32*)0xBC000000;
    // A334
    while (size > 0x001FFFFF) {
        size -= 0x200000;
        *(ptr++) = 0xCCCCCCCC;
    }
    // A348
    if (size != 0) {
        u32 flag = 0xCCCCCCCC;
        // A36C
        s32 i;
        for (i = (size >> 18) * 4; i < 32; i += 4)
            flag |= (0xF << i);

        // A380
        *(ptr++) = flag;
    }
    // A38C
    while (ptr <= (s32*)0xBC00000C) // A3A4
        *(ptr++) = 0xFFFFFFFF;
    // A3B8
    if (extSize == 0) {
        // A4AC
        HW(0xBC000048) = 1;
    } else
        HW(0xBC000048) = 0;
    // A3C8
    s32 state = pspCop0StateGet(COP0_STATE_CPUID);
    if (state != 0) {
        // A424
        HW(0xBC000030) = 0x300;
        HW(0xBC000034) = 0xF00;
        HW(0xBC000038) = 0;
        HW(0xBC000040) = 0;
        HW(0xBC000044) &= 0xFFFFFFE0;
        HW(0xBC000010) = 0xFFFFFFF3;
        HW(0xBC000014) = 0xFFFFFFFF;
        HW(0xBC000018) = 0xFFFFFFFF;
        HW(0xBC00001C) = 0xFFFFFFFF;
        HW(0xBC000020) = 0xFFFFFFFF;
        HW(0xBC000024) = 0xFFFFFFFF;
        HW(0xBC000028) = 0xFFFFFFFF;
        HW(0xBC00002C) = 0xFFFFFFFF;
    } else {
        HW(0xBC000030) = 0;
        HW(0xBC000034) = 0;
        HW(0xBC000038) = 0;
        HW(0xBC00003C) = 0;
        HW(0xBC000040) = 0;
        HW(0xBC000044) &= 0xFFFFFF9F;
    }
}

void sceKernelMemoryExtendSize(void)
{
    if (sceKernelGetModel() != 0)
        return;
    if (g_MemInfo.extSc2Kernel != NULL) {
        // A51C
        s32 oldIntr = suspendIntr();
        if (mem_extend_nest == 0) {
            HW(0xBC100040) = (HW(0xBC100040) & 0xFFFFFFFC) | 2;
            pspSync();
        }
        // A550
        mem_extend_nest++;
        resumeIntr(oldIntr);
    }
}

void sceKernelMemoryShrinkSize(void)
{
    if (sceKernelGetModel() != 0)
        return;
    if (g_MemInfo.extSc2Kernel == NULL)
        return;
    if (mem_extend_nest == 1) {
        // A614
        sceKernelDcacheWritebackInvalidateRange((void *)g_MemInfo.extSc2Kernel->addr, g_MemInfo.extSc2Kernel->size);
    }
    // A5A4
    s32 oldIntr = suspendIntr();
    if ((mem_extend_nest--) == 1) {
        // A5E0
        sceKernelDcacheWritebackInvalidateRange((void *)g_MemInfo.extSc2Kernel->addr, g_MemInfo.extSc2Kernel->size);
        HW(0xBC100040) = (HW(0xBC100040) & 0xFFFFFFFC) | 1;
        pspSync();
    }
    // A5C0
    resumeIntr(oldIntr);
}

u32 sceKernelMemoryOpenSize(void)
{
    u32 oldState = HW(0xBC100040);
    HW(0xBC100040) = (HW(0xBC100040) & 0xFFFFFFFC) | 2;
    return (oldState & 3);
}

void sceKernelMemoryCloseSize(u32 state)
{
    HW(0xBC100040) = (HW(0xBC100040) & 0xFFFFFFFC) | (state & 3);
}

s32 sceKernelSetDdrMemoryProtection(u32 addr, u32 size, u32 set)
{
    // A684
    if (!ADDR_IS_KERNEL_RAM(addr))
        return SCE_ERROR_KERNEL_ERROR;
    if (!ADDR_IS_KERNEL_RAM(addr + size - 1))
        return SCE_ERROR_KERNEL_ERROR;
    u32 numParts = size >> 18;
    u32 firstPart = ((addr & 0x1FFFFFFF) >> 18) & 0x1F;
    u32 i;
    // A6E0
    for (i = 0; i < numParts; i++) {
        u32 curPart = firstPart + i;
        u32 partHi = curPart / 8;
        u32 addr = 0xBC000000 + partHi * 4;
        u32 partLo = (curPart - (partHi * 8)) * 4;
        HW(addr) = (HW(addr) & ~(0xF << partLo)) | (set << partLo);
    }
    return 0;
}

