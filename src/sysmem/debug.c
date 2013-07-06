#include <sysmem_sysclib.h>

#include "kdebug.h"
#include "partition.h"

char g_140C8[9]; // 140C8

s32 sceKernelApiEvaluationInit()
{
    return 0;
}

s32 sceKernelRegisterApiEvaluation()
{
    return 0x80020001;
}

s32 sceKernelApiEvaliationAddData()
{
    return 0;
}

s32 sceKernelApiEvaluationReport()
{
    return 0;
}

s32 sceKernelSetGcovFunction()
{
    return 0;
}

s32 sceKernelCallGcovFunction()
{
    return 0;
}

s32 sceKernelSetGprofFunction()
{
    return 0;
}

s32 sceKernelCallGprofFunction()
{
    return 0;
}

int sceKernelCheckDebugHandler()
{
    SceSysmemMemoryPartition *part = MpidToCB(3);
    return _CheckDebugHandler((void *)part->addr, part->size);
}

void sub_A1E8(void)
{
    strncpy(g_140C8, "invalid", 8);
    g_140C8[8] = '\0';
}

s32 SysMemForKernel_7FF2F35A(char *arg)
{
    s32 oldK1 = pspShiftK1();
    if (arg == NULL || !pspK1PtrOk(arg)) {
        pspSetK1(oldK1);
        return 0x800200D3;
    }
    // A280
    strncpy(arg, g_140C8, 8);
    arg[8] = '\0';
    pspSetK1(oldK1);
    return 0;
}

s32 SysMemForKernel_A03CB480(char *arg)
{
    strncpy(g_140C8, arg, 8);
    return 0;
}

