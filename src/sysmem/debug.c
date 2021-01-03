#include <sysmem_sysclib.h>

#include "kdebug.h"
#include "partition.h"

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

