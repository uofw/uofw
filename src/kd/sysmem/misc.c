#include <sysmem_kernel.h>
#include <sysmem_sysclib.h>

#include "intr.h"

// 13FE4
s32 (*gRebootFunc)(void *arg);

// 13FE8
s32(*gGetIdFunc)(const char *path, char *id);

// 13FEC
SceGameInfo SystemGameInfo;

// 140C8
char g_140C8[9];

// 14538
s32 gSysmemRandomValue;

// 1453C
u32 g_1453C;

// 14540
u32 gSysmemModel;

void sub_A1E8(void)
{
    strncpy(g_140C8, "invalid", 8);
    g_140C8[8] = '\0';
}

s32 SysMemUserForUser_945E45DA(char *arg)
{
    s32 oldK1 = pspShiftK1();
    if (arg == NULL || !pspK1PtrOk(arg)) {
        pspSetK1(oldK1);
        return SCE_ERROR_KERNEL_ILLEGAL_ADDR;
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

s32 SysMemForKernel_807179E7(char *gameId, int arg1, char *arg2, char *arg3, int arg4, int arg5, char *arg6)
{
    if (gameId == NULL) {
        // 90A4
        SystemGameInfo.param_product_string[0] = '\0';
    } else {
        strncpy((char*)SystemGameInfo.param_product_string, gameId, 13);
        SystemGameInfo.param_product_string[13] = '\0';
    }
    // 8FB0
    SystemGameInfo.param_parental = arg1;
    if (arg2 == NULL) {
        // 909C
        SystemGameInfo.param_gamedata_id[0] = '\0';
    } else {
        strncpy(SystemGameInfo.param_gamedata_id, arg2, 10);
        SystemGameInfo.param_gamedata_id[10] = '\0';
    }
    // 8FD0
    if (arg3 == NULL) {
        // 9090
        SystemGameInfo.param_app_ver[0] = '\0';
    }
    else {
        strncpy(SystemGameInfo.param_app_ver, arg3, 7);
        SystemGameInfo.param_app_ver[7] = '\0';
    }
    // 8FF0
    SystemGameInfo.param_bootable = arg4;
    SystemGameInfo.param_opnssmp_ver = arg5;
    if (arg6 == NULL) {
        // 9088
        SystemGameInfo.vsh_version[0] = '\0';
    } else {
        strncpy(SystemGameInfo.vsh_version, arg6, 7);
        SystemGameInfo.vsh_version[7] = '\0';
    }
    // 9014
    if (gameId != NULL || arg1 != 0 || arg2 != NULL || arg3 != NULL || arg6 != NULL) {
        // 907C
        SystemGameInfo.flags |= 0x100;
    } else
        SystemGameInfo.flags &= (~0x100);
    // 9044
    return 0;
}

s32 sceKernelCopyGameInfo(SceGameInfo *info)
{
    if (info == NULL)
        return SCE_ERROR_KERNEL_ILLEGAL_ADDR;
    info->flags = SystemGameInfo.flags & 0xFFFFFFFD;
    info->size = SystemGameInfo.size;
    if ((SystemGameInfo.flags & 1) != 0) {
        // 92A4
        strncpy((char*)info->umd_data_string, (char*)SystemGameInfo.umd_data_string, 10);
        info->umd_data_string[10] = '\0';
    } else
        info->umd_data_string[0] = '\0';
    // 9104
    if ((SystemGameInfo.flags & 4) != 0) {
        // 9290
        memcpy(info->QTGP2, SystemGameInfo.QTGP2, 8);
    }
    // 9118
    if ((SystemGameInfo.flags & 8) != 0) {
        // 927C
        memcpy(info->QTGP3, SystemGameInfo.QTGP3, 16);
    }
    // 9124
    if ((SystemGameInfo.flags & 0x100) != 0) {
        // 9238
        strncpy((char*)info->param_product_string, (char*)SystemGameInfo.param_product_string, 13);
        info->param_product_string[13] = '\0';
        info->param_parental = SystemGameInfo.param_parental;
        strncpy(info->param_gamedata_id, SystemGameInfo.param_gamedata_id, 10);
        info->param_gamedata_id[10] = '\0';
        strncpy(info->vsh_version, SystemGameInfo.vsh_version, 7);
        info->vsh_version[7] = '\0';
    } else {
        info->param_product_string[0] = '\0';
        info->param_parental = 0;
        info->param_gamedata_id[0] = '\0';
        info->vsh_version[0] = '\0';
    }
    // 9140
    if ((SystemGameInfo.flags & 0x200) == 0)
        info->umd_cache_on = 0;
    else
        info->umd_cache_on = SystemGameInfo.umd_cache_on;
    // 915C
    if ((SystemGameInfo.flags & 0x20000) == 0)
        info->utility_location = 0;
    else
        info->utility_location = SystemGameInfo.utility_location;
    // 917C
    if ((SystemGameInfo.flags & 0x40000) != 0) {
        // 9224
        strncpy(info->vsh_bootfilename, SystemGameInfo.vsh_bootfilename, 64);
    } else
        info->vsh_bootfilename[0] = '\0';
    // 9198
    if ((SystemGameInfo.flags & 0x80000) == 0) {
        // 9210
        memset(info->subscription_validity, 0, 8);
    } else
        memcpy(info->subscription_validity, SystemGameInfo.subscription_validity, 8);
    // 91BC
    if ((SystemGameInfo.flags & 0x1000) != 0)
        info->compiled_sdk_version = SystemGameInfo.compiled_sdk_version;
    else
        info->compiled_sdk_version = 0;
    // 91D0
    if ((SystemGameInfo.flags & 0x2000) != 0)
        info->compiler_version = SystemGameInfo.compiler_version;
    else
        info->compiler_version = 0;
    // 91EC
    info->DNAS = 0;
    return 0;
}

// 92BC
void CopyGameInfo(SceGameInfo *info)
{
    if (info == NULL)
        return;
    SystemGameInfo.size = info->size;
    if ((info->flags & 0x1) != 0) {
        // 94E0
        strncpy((char*)SystemGameInfo.expect_umd_data, (char*)info->umd_data_string, 10);
        SystemGameInfo.expect_umd_data[10] = '\0';
        SystemGameInfo.flags |= 0x2;
    }
    // 92F4
    if ((info->flags & 0x4) != 0) {
        if (&info->QTGP2 == NULL) {
            // 94C4
            memset(SystemGameInfo.QTGP2, 0, 8);
            SystemGameInfo.flags &= (~0x4);
        } else {
            memcpy(SystemGameInfo.QTGP2, info->QTGP2, 8);
            SystemGameInfo.flags |= 0x4;
        }
        // 931C
    }
    // 9328
    if ((info->flags & 0x8) != 0) {
        if (&info->QTGP3 == NULL) {
            // 949C
            memset(SystemGameInfo.QTGP3, 0, 16);
            SystemGameInfo.flags &= (~0x8);
        } else {
            memcpy(SystemGameInfo.QTGP3, info->QTGP3, 16);
            SystemGameInfo.flags |= 0x8;
        }
        // 9358
    }
    // 9364
    if ((info->flags & 0x100) != 0) {
        // 9478
        SysMemForKernel_807179E7((char*)info->param_product_string, info->param_parental, info->param_gamedata_id, info->param_app_ver, info->param_bootable, info->param_opnssmp_ver, info->vsh_version);
    }
    // 936C
    if ((info->flags & 0x200) != 0) {
        SystemGameInfo.umd_cache_on = info->umd_cache_on;
        SystemGameInfo.flags |= 0x200;
    }
    // 9394
    if ((info->flags & 0x20000) != 0) {
        SystemGameInfo.utility_location = info->utility_location;
        SystemGameInfo.flags |= 0x20000;
    }
    // 93C0
    if ((info->flags & 0x40000) != 0) {
        if (info->vsh_bootfilename == NULL) {
            // 946C
            SystemGameInfo.vsh_bootfilename[0] = '\0';
        } else
            strncpy(SystemGameInfo.vsh_bootfilename, info->vsh_bootfilename, 64);
        // 93E4
        SystemGameInfo.flags |= 0x40000;
    }
    // 9400
    if ((info->flags & 0x80000) != 0) {
        if (info->subscription_validity == NULL) {
            // 9450
            memset(SystemGameInfo.subscription_validity, 0, 8);
        } else
            memcpy(SystemGameInfo.subscription_validity, info->subscription_validity, 8);
        // 9428
        SystemGameInfo.flags |= 0x80000;
    }
}

void InitGameInfo(void)
{
    SystemGameInfo.size = sizeof SystemGameInfo;
    SystemGameInfo.vsh_bootfilename[0] = '\0';
    SystemGameInfo.flags = 0;
    SystemGameInfo.umd_data_string[0] = '\0';
    SystemGameInfo.expect_umd_data[0] = '\0';
    SystemGameInfo.allow_replace_umd = 0;
    SystemGameInfo.param_product_string[0] = '\0';
    SystemGameInfo.param_parental = 0;
    SystemGameInfo.vsh_version[0] = '\0';
    SystemGameInfo.umd_cache_on = 0;
    SystemGameInfo.compiled_sdk_version = 0;
    SystemGameInfo.compiler_version = 0;
    SystemGameInfo.DNAS = 0;
    SystemGameInfo.utility_location = 0;
}

s32 SysMemForKernel_F3BDB718(char *arg0)
{
    if (arg0 == NULL) {
        // 95B4
        memset(SystemGameInfo.umd_data_string, 0, 16);
        SystemGameInfo.flags &= ~0x1;
    } else {
        strncpy((char*)SystemGameInfo.umd_data_string, arg0, 10);
        SystemGameInfo.umd_data_string[10] = '\0';
        SystemGameInfo.flags |= 0x1;
    }
    // 9598
    return 0;
}

s32 sceKernelGetQTGP2(char *qtgp2)
{
    if ((SystemGameInfo.flags & 0x4) == 0)
        return SCE_ERROR_KERNEL_ERROR;
    memcpy(qtgp2, SystemGameInfo.QTGP2, 8);
    return 0;
}

s32 sceKernelSetQTGP2(char *qtgp2)
{
    if (qtgp2 == NULL) {
        // 9670
        memset(SystemGameInfo.QTGP2, 0, 8);
        SystemGameInfo.flags &= ~0x4;
    } else {
        memcpy(SystemGameInfo.QTGP2, qtgp2, 8);
        SystemGameInfo.flags |= 0x4;
    }
    // 9654
    return 0;
}

s32 sceKernelGetQTGP3(char *qtgp3)
{
    if ((SystemGameInfo.flags & 0x8) == 0)
        return SCE_ERROR_KERNEL_ERROR;
    memcpy(qtgp3, SystemGameInfo.QTGP3, 16);
    return 0;
}

s32 sceKernelSetQTGP3(char *qtgp3)
{
    if (qtgp3 == NULL) {
        // 972C
        memset(SystemGameInfo.QTGP3, 0, 16);
        SystemGameInfo.flags &= ~0x8;
    } else {
        memcpy(SystemGameInfo.QTGP3, qtgp3, 16);
        SystemGameInfo.flags |= 0x8;
    }
    return 0;
}

s32 sceKernelGetAllowReplaceUmd(u32 *allow)
{
    if ((SystemGameInfo.flags & 0x10) == 0)
        return SCE_ERROR_KERNEL_ERROR;
    *allow = SystemGameInfo.allow_replace_umd;
    return 0;
}

s32 sceKernelSetAllowReplaceUmd(u32 allow)
{
    SystemGameInfo.allow_replace_umd = allow;
    SystemGameInfo.flags |= 0x10;
    return 0;
}

s32 sceKernelSetUmdCacheOn(u32 umdCacheOn)
{
    SystemGameInfo.umd_cache_on = umdCacheOn;
    SystemGameInfo.flags |= 0x200;
    return 0;
}

s32 SysMemForKernel_40B744A4(u32 utility_location)
{
    SystemGameInfo.utility_location = utility_location;
    SystemGameInfo.flags |= 0x20000;
    return 0;
}

s32 SysMemForKernel_BFE08689(char *vsh_bootfilename)
{
    if (vsh_bootfilename == NULL)
        SystemGameInfo.vsh_bootfilename[0] = '\0';
    else
        strncpy(SystemGameInfo.vsh_bootfilename, vsh_bootfilename, 64);
    // 980C
    SystemGameInfo.flags |= 0x40000;
    return 0;
}

s32 SysMemForKernel_2A8B8B2D(char *subscription_validity)
{
    if (subscription_validity == NULL) {
        // 9888
        memset(SystemGameInfo.subscription_validity, 0, 8);
    } else
        memcpy(SystemGameInfo.subscription_validity, subscription_validity, 8);
    // 9864
    SystemGameInfo.flags |= 0x80000;
    return 0;
}

SceGameInfo *sceKernelGetGameInfo(void)
{
    return &SystemGameInfo;
}

u32 sceKernelGetCompiledSdkVersion(void)
{
    if ((SystemGameInfo.flags & 0x1000) == 0)
        return 0;
    return SystemGameInfo.compiled_sdk_version;
}

s32 sceKernelSetCompiledSdkVersion100(u32 ver)
{
    s32 oldK1 = pspShiftK1();
    switch (ver & 0xFFFF0000)
    {
    case 0x01000000:
    case 0x01050000:
    case 0x02000000:
    case 0x02050000:
    case 0x02060000:
    case 0x02070000:
    case 0x02080000:
    case 0x03000000:
    case 0x03010000:
    case 0x03030000:
    case 0x03040000:
    case 0x03050000:
    case 0x03060000:
        SystemGameInfo.flags |= 0x1000;
        pspSetK1(oldK1);
        return 0;

    default: {
        s32 oldIntr = suspendIntr();
        HW(0xBC100040) = (HW(0xBC100040) & 0xFFFFFFFC) | 1;
        sceKernelMemset32((void *)HWPTR(HW_RESET_VECTOR), 0, HW_RESET_VECTOR_SIZE);
        resumeIntr(oldIntr);
        // 09968
        for (;;)
            ;
        }
    }
}

s32 sceKernelSetCompiledSdkVersion370(u32 ver)
{
    s32 oldK1 = pspShiftK1();
    if ((ver & 0xFFFF0000) == 0x03070000) {
        SystemGameInfo.flags |= 0x1000;
        pspSetK1(oldK1);
        return 0;
    }
    s32 oldIntr = suspendIntr();
    HW(0xBC100040) = (HW(0xBC100040) & 0xFFFFFFFC) | 1;
    sceKernelMemset32((void *)HWPTR(HW_RESET_VECTOR), 0, HW_RESET_VECTOR_SIZE);
    resumeIntr(oldIntr);
    // 9A54
    for (;;)
        ;
}

s32 sceKernelSetCompiledSdkVersion380_390(u32 ver)
{
    s32 oldK1 = pspShiftK1();
    if ((ver & 0xFFFF0000) == 0x03080000 ||
      ((ver & 0xFFFF0000) == 0x03090000 && (ver & 0x0000FF00) == 0)) {
        SystemGameInfo.flags |= 0x1000;
        pspSetK1(oldK1);
        return 0;
    }
    s32 oldIntr = suspendIntr();
    HW(0xBC100040) = (HW(0xBC100040) & 0xFFFFFFFC) | 1;
    sceKernelMemset32((void *)HWPTR(HW_RESET_VECTOR), 0, HW_RESET_VECTOR_SIZE);
    resumeIntr(oldIntr);
    // 9AF8
    for (;;)
        ;
}

s32 sceKernelSetCompiledSdkVersion395(u32 ver)
{
    s32 oldK1 = pspShiftK1();
    switch (ver & 0xFFFFFF00)
    {
    case 0x03090500:
    case 0x03090600:
    case 0x04000000:
    case 0x04000100:
    case 0x04000500:
        SystemGameInfo.flags |= 0x1000;
        pspSetK1(oldK1);
        return 0;
    }
    s32 oldIntr = suspendIntr();
    HW(0xBC100040) = (HW(0xBC100040) & 0xFFFFFFFC) | 1;
    sceKernelMemset32((void *)HWPTR(HW_RESET_VECTOR), 0, HW_RESET_VECTOR_SIZE);
    resumeIntr(oldIntr);
    // 9AF8
    for (;;)
        ;
}

s32 sceKernelSetCompiledSdkVersion410_420(u32 ver)
{
    s32 oldK1 = pspShiftK1();
    switch (ver & 0xFFFF0000)
    {
    case 0x04010000:
    case 0x04020000:
        SystemGameInfo.flags |= 0x1000;
        pspSetK1(oldK1);
        return 0;
    }
    s32 oldIntr = suspendIntr();
    HW(0xBC100040) = (HW(0xBC100040) & 0xFFFFFFFC) | 1;
    sceKernelMemset32((void *)HWPTR(HW_RESET_VECTOR), 0, HW_RESET_VECTOR_SIZE);
    resumeIntr(oldIntr);
    // 9C7C
    for (;;)
        ;
}

s32 sceKernelSetCompiledSdkVersion500_550(u32 ver)
{
    s32 oldK1 = pspShiftK1();
    switch (ver & 0xFFFF0000)
    {
    case 0x05000000:
    case 0x05050000:
        SystemGameInfo.flags |= 0x1000;
        pspSetK1(oldK1);
        return 0;
    }
    s32 oldIntr = suspendIntr();
    HW(0xBC100040) = (HW(0xBC100040) & 0xFFFFFFFC) | 1;
    sceKernelMemset32((void *)HWPTR(HW_RESET_VECTOR), 0, HW_RESET_VECTOR_SIZE);
    resumeIntr(oldIntr);
    for (;;)
        ;
}

s32 sceKernelSetCompiledSdkVersion570(u32 ver)
{
    s32 oldK1 = pspShiftK1();
    if ((ver & 0xFFFF0000) == 0x05070000) {
        SystemGameInfo.flags |= 0x1000;
        pspSetK1(oldK1);
        return 0;
    }
    s32 oldIntr = suspendIntr();
    HW(0xBC100040) = (HW(0xBC100040) & 0xFFFFFFFC) | 1;
    sceKernelMemset32((void *)HWPTR(HW_RESET_VECTOR), 0, HW_RESET_VECTOR_SIZE);
    resumeIntr(oldIntr);
    for (;;)
        ;
}

s32 sceKernelSetCompiledSdkVersion600_620(u32 ver)
{
    s32 oldK1 = pspShiftK1();
    switch (ver & 0xFFFF0000)
    {
    case 0x06000000:
    case 0x06010000:
    case 0x06020000:
        SystemGameInfo.flags |= 0x1000;
        pspSetK1(oldK1);
        return 0;
    }
    s32 oldIntr = suspendIntr();
    HW(0xBC100040) = (HW(0xBC100040) & 0xFFFFFFFC) | 1;
    sceKernelMemset32((void *)HWPTR(HW_RESET_VECTOR), 0, HW_RESET_VECTOR_SIZE);
    resumeIntr(oldIntr);
    for (;;)
        ;
}

s32 sceKernelSetCompiledSdkVersion630_650(u32 ver)
{
    s32 oldK1 = pspShiftK1();
    switch (ver & 0xFFFF0000)
    {
    case 0x06030000:
    case 0x06040000:
    case 0x06050000:
        SystemGameInfo.flags |= 0x1000;
        pspSetK1(oldK1);
        return 0;
    }
    s32 oldIntr = suspendIntr();
    HW(0xBC100040) = (HW(0xBC100040) & 0xFFFFFFFC) | 1;
    sceKernelMemset32((void *)HWPTR(HW_RESET_VECTOR), 0, HW_RESET_VECTOR_SIZE);
    resumeIntr(oldIntr);
    for (;;)
        ;
}

s32 sceKernelSetCompiledSdkVersion660(u32 ver)
{
    s32 oldK1 = pspShiftK1();
    if ((ver & 0xFFFF0000) == 0x06060000) {
        SystemGameInfo.flags |= 0x1000;
        pspSetK1(oldK1);
        return 0;
    }
    s32 oldIntr = suspendIntr();
    HW(0xBC100040) = (HW(0xBC100040) & 0xFFFFFFFC) | 1;
    sceKernelMemset32((void *)HWPTR(HW_RESET_VECTOR), 0, HW_RESET_VECTOR_SIZE);
    resumeIntr(oldIntr);
    for (;;)
        ;
}

s32 sceKernelGetCompilerVersion(void)
{
    if ((SystemGameInfo.flags & 0x2000) == 0)
        return 0;
    return SystemGameInfo.compiler_version;
}

s32 sceKernelSetCompilerVersion(s32 version)
{
    SystemGameInfo.compiler_version = version;
    return 0;
}

s32 sceKernelGetDNAS(void)
{
    if ((SystemGameInfo.flags & 0x10000) == 0)
        return 0;
    return SystemGameInfo.DNAS;
}

s32 sceKernelSetDNAS(s32 DNAS)
{
    SystemGameInfo.DNAS = DNAS;
    return 0;
}

s32 sceKernelGetInitialRandomValue(void)
{
    return gSysmemRandomValue;
}

s32 SysMemUserForUser_D8DE5C1E(void)
{
    if (g_1453C != 0)
        return SCE_ERROR_KERNEL_ERROR;
    return 0;
}

u32 SysMemForKernel_13EE28DA(u32 flag)
{
    return (g_1453C &= flag);
}

u32 sceKernelGetModel(void)
{
    return gSysmemModel;
}

s32 sceKernelSetRebootKernel(s32 (*rebootKernel)(void*))
{
    gRebootFunc = rebootKernel;
    return 0;
}

s32 sceKernelRebootKernel(void *arg)
{
    if (gRebootFunc == NULL)
        return SCE_ERROR_KERNEL_ERROR;
    return gRebootFunc(arg);
}

s32 sceKernelRegisterGetIdFunc(void *func)
{
    if (gGetIdFunc != NULL)
        return SCE_ERROR_KERNEL_ERROR;
    gGetIdFunc = func;
    return 0;
}

s32 sceKernelGetId(const char *path, char *id)
{
    if (gGetIdFunc == NULL || gGetIdFunc == (void*)-1)
        return SCE_ERROR_KERNEL_ERROR;
    // A164
    s32 ret = gGetIdFunc(path, id);
    gGetIdFunc = (void*)-1;
    return ret;
}

