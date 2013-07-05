s32 SysMemForKernel_807179E7(char *gameId, int arg1, char *arg2, char *arg3, int arg4, int arg5, char *arg6)
{
    if (gameId == NULL) {
        // 90A4
        g_13FEC.gameId[0] = '\0';
    } else {
        strncpy(g_13FEC.gameId, gameId, 13);
        g_13FEC.gameId[13] = '\0';
    }
    // 8FB0
    g_13FEC.unk84 = arg1;
    if (arg2 == NULL) {
        // 909C
        g_13FEC.str180[0] = '\0';
    } else {
        strncpy(g_13FEC.str180, arg2, 10);
        g_13FEC.str180[10] = '\0';
    }
    // 8FD0
    if (arg3 == NULL) {
        // 9090
        g_13FEC.str196[0] = '\0';
    }
    else {
        strncpy(g_13FEC.str196, arg3, 7);
        g_13FEC.str196[7] = '\0';
    }
    // 8FF0
    g_13FEC.unk212 = arg4;
    g_13FEC.unk216 = arg5;
    if (arg6 == NULL) {
        // 9088
        g_13FEC.str88[0] = '\0';
    } else {
        strncpy(g_13FEC.str88, arg6, 7);
        g_13FEC.str88[7] = '\0';
    }
    // 9014
    if (gameId != NULL || arg1 != 0 || arg2 != NULL || arg3 != NULL || arg6 != NULL) {
        // 907C
        g_13FEC.flags |= 0x100;
    } else
        g_13FEC.flags &= (~0x100);
    // 9044
    return 0;
}

s32 sceKernelCopyGameInfo(SceKernelGameInfo *info)
{
    if (info == NULL)
        return 0x800200D3;
    info->flags = g_13FEC.flags & 0xFFFFFFFD;
    info->size = g_13FEC.size;
    if ((g_13FEC.flags & 1) != 0) {
        // 92A4
        strncpy(info->str8, g_13FEC.str8, 10);
        info->str8[10] = '\0';
    } else
        info->str8[0] = '\0';
    // 9104
    if ((g_13FEC.flags & 4) != 0) {
        // 9290
        memcpy(info->qtgp2, g_13FEC.qtgp2, 8);
    }
    // 9118
    if ((g_13FEC.flags & 8) != 0) {
        // 927C
        memcpy(info->qtgp3, g_13FEC.qtgp3, 16);
    }
    // 9124
    if ((g_13FEC.flags & 0x100) != 0) {
        // 9238
        strncpy(info->gameId, g_13FEC.gameId, 13);
        info->gameId[13] = '\0';
        info->unk84 = g_13FEC.unk84;
        strncpy(info->str180, g_13FEC.str180, 10);
        info->str180[10] = '\0';
        strncpy(info->str88, g_13FEC.str88, 7);
        info->str88[7] = '\0';
    } else {
        info->gameId[0] = '\0';
        info->unk84 = 0;
        info->str180[0] = '\0';
        info->unk88 = 0;
    }
    // 9140
    if ((g_13FEC.flags & 0x200) == 0)
        info->umdCacheOn = 0;
    else
        info->umdCacheOn = g_13FEC.umdCacheOn;
    // 915C
    if ((g_13FEC.flags & 0x20000) == 0)
        info->unk112 = 0;
    else
        info->unk112 = g_13FEC.unk112;
    // 917C
    if ((g_13FEC.status & 0x40000) != 0) {
        // 9224
        strncpy(info->str116, g_13FEC.str116, 64);
    } else
        info->str116[0] = '\0';
    // 9198
    if ((g_13FEC.status & 0x80000) == 0) {
        // 9210
        memset(info->unk204, 0, 8);
    } else
        memcpy(info->unk204, g_13FEC.unk204, 8);
    // 91BC
    if ((g_13FEC.flags & 0x1000) != 0)
        info->sdkVersion = g_13FEC.sdkVersion;
    else
        info->sdkVersion = 0;
    // 91D0
    if ((g_13FEC.flags & 0x2000) != 0)
        info->compilerVersion = g_13FEC.compilerVersion;
    else
        info->compilerVersion = 0;
    // 91EC
    info->dnas = 0;
    return 0;
}

// 92BC
void CopyGameInfo(SceKernelGameInfo *info)
{
    if (info == NULL)
        return;
    g_13FEC.size = info->size;
    if ((info->flags & 0x1) != 0) {
        // 94E0
        strncpy(g_13FEC.str24, info->str8, 10);
        g_13FEC.str24[10] = '\0';
        g_13FEC.flags |= 0x2;
    }
    // 92F4
    if ((info->flags & 0x4) != 0) {
        if (info->qtgp2 == NULL) {
            // 94C4
            memset(g_13FEC.qtgp2, 0, 8);
            g_13FEC.flags &= (~0x4);
        } else {
            memcpy(g_13FEC.qtgp2, info->qtgp2, 8);
            g_13FEC.flags |= 0x4;
        }
        // 931C
    }
    // 9328
    if ((info->flags & 0x8) != 0) {
        if (info->qtgp3 == NULL) {
            // 949C
            memset(g_13FEC.qtgp3, 0, 16);
            g_13FEC.flags &= (~0x8);
        } else {
            memcpy(g_13FEC.qtgp3, info->qtgp3, 16);
            g_13FEC.flags |= 0x8;
        }
        // 9358
    }
    // 9364
    if ((info->flags & 0x100) != 0) {
        // 9478
        SysMemForKernel_807179E7(info->gameId, info->unk84, info->str180, info->str196, info->unk212, info->unk216, info->str88);
    }
    // 936C
    if ((info->flags & 0x200) != 0) {
        g_13FEC.umdCacheOn = info->umdCacheOn;
        g_13FEC.flags |= 0x200;
    }
    // 9394
    if ((info->flags & 0x20000) != 0) {
        g_13FEC.unk112 = info->unk112;
        g_13FEC.flags |= 0x20000;
    }
    // 93C0
    if ((info->flags & 0x40000) != 0) {
        if (info->str116 == NULL) {
            // 946C
            g_13FEC.str116[0] = '\0';
        } else
            strncpy(g_13FEC.str116, info->str116, 64);
        // 93E4
        g_13FEC.flags |= 0x40000;
    }
    // 9400
    if ((info->flags & 0x80000) != 0) {
        if (info->unk204 == NULL) {
            // 9450
            memset(g_13FEC.unk204, 0, 8);
        } else
            memcpy(g_13FEC.unk204, info->unk204, 8);
        // 9428
        g_13FEC.flags |= 0x80000;
    }
}

void InitGameInfo(void)
{
    g_13FEC.size = 220;
    g_13FEC.str116[0] = '\0';
    g_13FEC.flags = 0;
    g_13FEC.str8[0] = '\0';
    g_13FEC.str24[0] = '\0';
    g_13FEC.allowReplaceUmd = 0;
    g_13FEC.gameId[0] = '\0';
    g_13FEC.unk84 = 0;
    g_13FEC.str88[0] = '\0';
    g_13FEC.umdCacheOn = 0;
    g_13FEC.sdkVersion = 0;
    g_13FEC.compilerVersion = 0;
    g_13FEC.dnas = 0;
    g_13FEC.unk112 = 0;
}
s32 SysMemForKernel_F3BDB718(char *arg0)
{
    if (arg0 == NULL) {
        // 95B4
        memset(g_13FEC.str8, 0, 16);
        g_13FEC.flags &= ~0x1;
    } else {
        strncpy(g_13FEC.str8, arg0, 10);
        g_13FEC.str8[10] = '\0';
        g_13FEC.flags |= 0x1;
    }
    // 9598
    return 0;
}
s32 sceKernelGetQTGP2(char *qtgp2)
{
    if ((g_13FEC.flags & 0x4) == 0)
        return 0x80020001;
    memcpy(qtgp2, g_13FEC.qtgp2, 8);
    return 0;
}

s32 sceKernelSetQTGP2(char *qtgp2)
{
    if (qtgp2 == NULL) {
        // 9670
        memset(g_13FEC.qtgp2, 0, 8);
        g_13FEC.flags &= ~0x4;
    } else {
        memcpy(g_13FEC.qtgp2, qtgp2, 8);
        g_13FEC.flags |= 0x4;
    }
    // 9654
    return 0;
}

s32 sceKernelGetQTGP3(char *qtgp3)
{
    if ((g_13FEC.flags & 0x8) == 0)
        return 0x80020001;
    memcpy(qtgp3, g_13FEC.qtgp3, 16);
    return 0;
}

s32 sceKernelSetQTGP3(char *qtgp3)
{
    if (qtgp3 == NULL) {
        // 972C
        memset(g_13FEC.qtgp3, 0, 16);
        g_13FEC.flags &= ~0x8;
    } else {
        memcpy(g_13FEC.qtgp3, qtgp3, 16);
        g_13FEC.flags |= 0x8;
    }
    return 0;
}

s32 sceKernelGetAllowReplaceUmd(u32 *allow)
{
    if ((g_13FEC.flags & 0x10) == 0)
        return 0x80020001;
    *allow = g_13FEC.allowReplaceUmd;
    return 0;
}

s32 sceKernelSetAllowReplaceUmd(u32 allow)
{
    g_13FEC.allowReplaceUmd = allow;
    g_13FEC.flags |= 0x10;
    return 0;
}

s32 sceKernelSetUmdCacheOn(u32 umdCacheOn)
{
    g_13FEC.umdCacheOn = umdCacheOn;
    g_13FEC.flags |= 0x200;
    return 0;
}

s32 SysMemForKernel_40B744A4(u32 unk112)
{
    g_13FEC.unk112 = unk112;
    g_13FEC.flags |= 0x20000;
    return 0;
}

s32 SysMemForKernel_BFE08689(char *str116)
{
    if (str116 == NULL)
        g_13FEC.str116[0] = '\0';
    else
        strncpy(g_13FEC.str116, str116, 64);
    // 980C
    g_13FEC.status |= 0x40000;
    return 0;
}

s32 SysMemForKernel_2A8B8B2D(char *unk204)
{
    if (unk204 == NULL) {
        // 9888
        memset(g_13FEC.unk204, 0, 8);
    } else
        memcpy(g_13FEC.unk204, unk204, 8);
    // 9864
    g_13FEC.flags |= 0x80000;
    return 0;
}

SceKernelGameInfo sceKernelGetGameInfo(void)
{
    return g_13FEC;
}

u32 sceKernelGetCompiledSdkVersion(void)
{
    if ((g_13FEC.flags & 0x1000) == 0)
        return 0;
    return g_13FEC.sdkVersion;
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
        g_13FEC.flags |= 0x1000;
        pspSetK1(oldK1);
        return 0;

    default:
        s32 oldIntr = suspendIntr();
        HW(0xBC100040) = (HW(0xBC100040) & 0xFFFFFFFC) | 1;
        sceKernelMemset32((void *)HWPTR(HW_RESET_VECTOR), 0, HW_RESET_VECTOR_SIZE);
        resumeIntr(oldIntr);
        // 09968
        for (;;)
            ;
    }
}

s32 sceKernelSetCompiledSdkVersion370(u32 ver)
{
    s32 oldK1 = pspShiftK1();
    if ((ver & 0xFFFF0000) == 0x03070000) {
        g_13FEC.flags |= 0x1000;
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
      ((ver & 0xFFFF0000) == 0x03090000 && (ver & 0x0000FF00) == 0))
        g_13FEC.flags |= 0x1000;
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
        g_13FEC.flags |= 0x1000;
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
        g_13FEC.flags |= 0x1000;
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
        g_13FEC.flags |= 0x1000;
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
        g_13FEC.flags |= 0x1000;
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
        g_13FEC.flags |= 0x1000;
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
        g_13FEC.flags |= 0x1000;
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
        g_13FEC.flags |= 0x1000;
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
    if ((g_13FEC.flags & 0x2000) == 0)
        return 0;
    return g_13FEC.compilerVersion;
}

s32 sceKernelSetCompilerVersion(s32 version)
{
    g_13FEC.compilerVersion = version;
    return 0;
}

s32 sceKernelGetDNAS(void)
{
    if ((g_13FEC.flags & 0x10000) == 0)
        return 0;
    return g_13FEC.dnas;
}

s32 sceKernelSetDNAS(s32 dnas)
{
    g_13FEC.dnas = dnas;
    return 0;
}

s32 sceKernelGetInitialRandomValue(void)
{
    return g_initialRand;
}

s32 SysMemForKernel_A0A9185A(void)
{
    if (g_1453C != 0)
        return 0x80020001;
    return 0;
}

u32 SysMemForKernel_13EE28DA(u32 flag)
{
    return (g_1453C &= flag);
}

u32 sceKernelGetModel(void)
{
    return g_model;
}

s32 sceKernelSetRebootKernel(s32 (*rebootKernel)(void*))
{
    g_rebootKernel = rebootKernel;
    return 0;
}

s32 sceKernelRebootKernel(void *arg)
{
    if (g_rebootKernel == NULL)
        return 0x80020001;
    return g_rebootKernel(arg);
}

s32 sceKernelRegisterGetIdFunc(void *func)
{
    if (g_13FE8 != NULL)
        return 0x80020001;
    g_13FE8 = func;
    return 0;
}

s32 sceKernelGetId()
{
    if (g_13FE8 == NULL || g_13FE8 == (void*)-1)
        return 0x80020001;
    // A164
    s32 ret = g_13FE8();
    g_13FE8 = (void*)-1;
    return ret;
}

