//0x000004A8
//ModuleMgrForKernel_2B7FC10D
s32 sceKernelLoadModuleWithApitype2(s32 arg0, const char *path, void *arg2, SceKernelLMOption *option)
{
    s32 oldK1 = pspShiftK1(); //0x4B4
    s32 error = SCE_ERROR_OK;
    SceModuleMgrParam param;

    if (sceKernelIsIntrContext()) //0x4E0
        error = SCE_ERROR_KERNEL_CANNOT_BE_CALLED_FROM_INTERRUPT;
    else if (pspK1IsUserMode()) //0x4EC
        error = SCE_ERROR_KERNEL_ILLEGAL_PERMISSION_CALL;
    else if (path == null) //0x4F8
        error = SCE_ERROR_KERNEL_ILLEGAL_ADDRESS;
    else if (!pspK1PtrIsOk(path)) //0x508
        error = SCE_ERROR_KERNEL_ILLEGAL_ADDRESS;
    else if (SysclibForKernel_B1DC2AE8(path, 37) != 0) //0x63C
        error = SCE_ERROR_KERNEL_UNKOWN_MODULE_FILE;
    else if (option != null && !pspK1StaBufOk(option, 20)) //0x658-0x670
        error = SCE_ERROR_KERNEL_ILLEGAL_ADDRESS;

    if (error < 0){
        pspSetK1(oldK1);
        return error;
    }

    SceUID fid = sceIoOpen(path, 0x04000001, 0777); //0x528
    if (fid < 0){ //0x534
        pspSetK1(oldK1);
        return fid; 
    }

    s32 command;
    switch (arg0){ //0x544-0x560
    case 272:
    case 274:
    case 276:
        command = 0x00208010;
        break;
    default:
        command = 0x00208011;
    }
    
    if (sceIoIoctl(fid, command, null, 0, null, 0) < 0){ //0x580
        pspSetK1(oldK1);
        return SCE_ERROR_KERNEL_PROHIBIT_LOADMODULE_DEVICE;
    }

    //null out param
    param.unk156 = 0; //0x5A4
    param.apiType = arg0; //0x5A8
    param.unk1 = 1; //0x5B4
    param.unk0 = 0; //0x5C0
    param.unk64 = 0; //0x5CC
    param.unk24 = fid; //0x5D8
    param.unk124 = 0; //0x5E0

    if ( sceIoIoctl(fid, 0x00208081, null, 0, null, 0) >= 0) //0x5DC
        param.unk100 = 16;

    error = sub_000075B4(param, option); //0x5F4
    sceIoClose(fid); //0x600
    pspSetK1(oldK1);
    return error;
}

//0x00001688
//ModuleMgrForKernel_CE0A74A5
void sceKernelLoadModuleDisc(const char *path, __attribute__((unused)) s32 flags, SceKernelLMOption *option)
{
    u32 oldK1 = pspShiftK1(); //0x1694
    s32 error = SCE_ERROR_OK;
    SceUID fid;
    SceModuleMgrParam param;

    if (sceKernelIsIntrContext()) //0x17B8
        error = SCE_ERROR_KERNEL_CANNOT_BE_CALLED_FROM_INTERRUPT;
    else if (pspK1IsUserMode()) //0x16C4
        error = SCE_ERROR_KERNEL_ILLEGAL_PERMISSION_CALL;
    else if (path== null) //0x16E8
        error = SCE_ERROR_KERNEL_ILLEGAL_ADDRESS;
    else if (!pspK1PtrIsOk(path)) //0x16E0
        error = SCE_ERROR_KERNEL_ILLEGAL_ADDRESS;
    else if (SysclibForKernel_B1DC2AE8(path, 37) != 0) //0x17F0
        error = SCE_ERROR_KERNEL_UNKNOWN_MODULE_FILE;
    else if (option != null && !pspK1StaBufOk(option, 20)) //0x180C
        error = SCE_ERROR_KERNEL_ILLEGAL_ADDRESS;
    else if (sceKernelGetCompiledSdkVersion() & ~0x1FFFF >= 0x2080000 
                && option->size != 0x20)
        error = SCE_ERROR_KERNEL_ILLEGAL_SIZE;

    if (error < 0){ //0x16EC
        pspSetK1(oldK1);
        return error;
    }

    fid = sceIoOpen(path, 0x04000001, 0777); //0x1700
    if (fid < 0){
        pspSetK1(oldK1);
        return fid;
    }

    if (sceIoIoctl(fid, 0x00208011, null, 0, null, 0) < 0){ //0x1738
        pspSetK1(oldK1);
        return SCE_ERROR_KERNEL_PROHIBIT_LOADMODULE_DEVICE;
    }

    //null out param

    param.apiType = 288; //0x175C
    param.unk1 = 1; //0x176C
    param.unk0 = 0; //0x1774
    param.unk64 = 0; //0x1780
    param.unk24 = fid; //0x1788
    param.unk124 = 0; //0x1790
    if (sceIoIoctl(fid, 0x00208081, null, 0, null, 0) >= 0)
        param.unk100 = 16; //0x1798

    error = sub_000075B4(param, option); //0x17A4
    sceIoClose(fid); //0x17B0
    pspSetK1(oldK1);
    return error;
}

//0x00001858
//ModuleMgrForKernel_CAE8E169
s32 sceKernelLoadModuleDiscUpdater(static char *path, void arg1, SceKernelLMOption *option)
{
    SceModuleMgrParam param;
    SceUID fid;
    s32 error = SCE_ERROR_OK;
    s32 oldK1 = pspShiftK1();

    if (sceKernelIsIntrContext()) //0x1888
        error = SCE_ERROR_KERNEL_CANNOT_BE_CALLED_FROM_INTERRUPT;
    else if (pspK1IsUserMode()) //0x1894
        error = SCE_ERROR_KERNEL_ILLEGAL_PERMISSION_CALL;
    else if (path == null) //0x18A0
        error = SCE_ERROR_KERNEL_ILLEGAL_ADDRESS;
    else if (!pspK1PtrIsOk(path)) //0x18B0
        error = SCE_ERROR_KERNEL_ILLEGAL_ADDRESS;
    else if (strchr(path, '%') != 0) //0x19B8
        error = SCE_ERROR_KERNEL_UNKNOWN_MODULE_FILE;
    else if (option != null && !pspK1StaBufOk(option, 20))
        error = SCE_ERROR_KERNEL_ILLEGAL_ADDRESS;
    else {
        fid = sceIoOpen(path, 0x04000001, 0777); //0x18D0
        if (fid < 0) //0x18DC
            error = (s32)fid;
        else{
            if (sceIoIoctl(fid, 0x00208011, null, 0, null, 0) < 0)
                error = SCE_ERROR_KERNEL_PROHIBIT_LOADMODULE_DEVICE;
            else {
                for (int i = 0; i < sizeof(SceModuleMgrParam) / 4; i ++)
                    ((S32 *)param)[i] = 0;

                param.apiType = API_TYPE_DISC_UPDATER; //0x192C
                param.unk1 = 1; //0x1938
                param.unk0 = 0; //0x1944
                param.unk64 = 0; //0x1950
                param.unk24 = fid; //0x1958
                param.unk124 = 0; //0x1960

                if (sceIoIoctl(fid, 0x00208081, null, 0, null, 0) >= 0)
                    param.unk100 =  16; //0x196C


                error = sub_000075B4(param, option);
            }

            sceIoClose(fid);
        }

    }

    pspSetK1(oldK1);
    return error;

}

//0x00001A28
//ModuleMgrForKernel_2C4F270D
void sceKernelLoadModuleDiscDebug(static char *path, void arg1, SceKernelLMOption *option)
{
    SceModuleMgrParam param;
    SceUID fid;
    s32 error = SCE_ERROR_OK;
    s32 oldK1 = pspShiftK1();

    if (sceKernelIsIntrContext()) //0x1A58
        error = SCE_ERROR_KERNEL_CANNOT_BE_CALLED_FROM_INTERRUPT;
    else if (pspK1IsUserMode()) //0x1A64
        error = SCE_ERROR_KERNEL_ILLEGAL_PERMISSION_CALL;
    else if (path == null) //0x1A70
        error = SCE_ERROR_KERNEL_ILLEGAL_ADDRESS;
    else if (!pspK1PtrIsOk(path)) //0x1A80
        error = SCE_ERROR_KERNEL_ILLEGAL_ADDRESS;
    else if (strchr(path, '%') != 0) //0x1B88
        error = SCE_ERROR_KERNEL_UNKNOWN_MODULE_FILE;
    else if (option != null && !pspK1StaBufOk(option, 20)) //0x1BAC
        error = SCE_ERROR_KERNEL_ILLEGAL_ADDRESS;
    else if (sceKernelGetCompiledSdkVersion() >= 0x2080000 && param.size != 20) //0x1BD4
        error = SCE_ERROR_KERNEL_ILLEGAL_SIZE;
    else {
        fid = sceIoOpen(path, 0x04000001, 0777); //0x1AA0
        if (fid < 0) //0x1AAC
            error = (s32)fid;
        else{
            if (sceIoIoctl(fid, 0x00208011, null, 0, null, 0) < 0)
                error = SCE_ERROR_KERNEL_PROHIBIT_LOADMODULE_DEVICE;
            else {
                for (int i = 0; i < sizeof(SceModuleMgrParam) / 4; i ++)
                    ((S32 *)param)[i] = 0;

                param.apiType = API_TYPE_DISC_DEBUG; //0x1AFC
                param.unk1 = 1; //0x1B08
                param.unk0 = 0; //0x1B14
                param.unk64 = 0; //0x1B20
                param.unk24 = fid; //0x1B28
                param.unk124 = 0; //0x1B30

                if (sceIoIoctl(fid, 0x00208081, null, 0, null, 0) >= 0)
                    param.unk100 =  16; //0x1B3C


                error = sub_000075B4(param, option);
            }

            sceIoClose(fid);
        }

    }

    pspSetK1(oldK1);
    return error;

}

//0x00001Bf8
void ModuleMgrForKernel_853A6C16(s32 apiType, const u8* path, void arg2, SceKernelLMOption *option))
{
    SceModuleMgrParam param;
    SceUID fid;  
    s32 error = SCE_ERROR_OK;
    s32 oldK1 = pspShiftK1();

    if (sceKernelIsIntrContext()) //0x1C30
        error = SCE_ERROR_KERNEL_CANNOT_BE_CALLED_FROM_INTERRUPT;
    else if (pspK1IsUserMode()) //0x1C3C
        error = SCE_ERROR_KERNEL_ILLEGAL_PERMISSION_CALL;
    else if (path == null) //0x1C48
        error = SCE_ERROR_KERNEL_ILLEGAL_ADDRESS;
    else if (!pspK1PtrIsOk(path)) //0x1C58
        error = SCE_ERROR_KERNEL_ILLEGAL_ADDRESS;
    else if (strchr(path, '%') != 0) //0x1D60
        error = SCE_ERROR_KERNEL_UNKNOWN_MODULE_FILE;
    else if (param != null){
        if (!pspK1StaBufOk(option 20))//0x1D84
            error = SCE_ERROR_KERNEL_ILLEGAL_ADDRESS;
        else if (sceKernelGetCompiledSdkVersion() >= 0x2080000 && option->size != 20) //0x1DC0
            error = SCE_ERROR_KERNEL_ILLEGAL_SIZE;
    }

    if (error == SCE_ERROR_OK){
        error = fid = sceIoOpen(path, 0x0400001, 0777); //0x1C78
        if (error >= 0){ //0x1C84
            error = sceIoIoctl(fid, 0x00208010, null, 0, null, 0); //0x1CA4
            if (error < 0) //0x1CB0
                error = SCE_ERROR_KERNEL_PROHIBIT_LOADMODULE_DEVICE;
            else {
                for (int i = 0; i < 160/4; i++)
                    ((s32*)param)[i] = 0;

                param.apiType = apiType; //0x1CCC
                param.endCMD = MM_COMMAND_1; //0x1CD8
                param.startCMD = MM_COMMAND_0; //0x1CE4
                param.fileBase = null; //0x1CF0
                param.unk24 = fid; //0x1CFC
                param.unk124 = 0; //0x1D04
                if (sceIoIoctl(fid, 0x00208081, null, 0, null, 0)) //0x1D08
                    param.unk100 = 16;
                error = sub_000075B4(param, option); //0x1D18
            }           

            sceIoClose(fid);
        }

    }

    pspSetK1(oldK1);
    return error;

}

//0x00001DD0
void ModuleMgrForKernel_C2A5E6CA(s32 apiType, const u8 *path, void arg1, SceKernelLMOption *option)
{
    SceModuleMgrParam param;
    SceUID fid;
    u8 *localVar;
    s32 error = SCE_ERROR_OK;
    s32 oldK1 = pspShiftK1();

    if (sceKernelIsIntrContext()) //0x1E0C
        error = SCE_ERROR_KERNEL_CANNOT_BE_CALLED_FROM_INTERRUPT;
    else if (pspK1IsUserMode()) //0x1E18
        error = SCE_ERROR_KERNEL_ILLEGAL_PERMISSION_CALL;
    else if (path == null) //0x1E24
        error = SCE_ERROR_KERNEL_ILLEGAL_ADDRESS;
    else if (pspK1PtrIsOk(path)) //0x1E34
        error = SCE_ERROR_KERNEL_ILLGAL_ADDRESS;
    else if (strchr(path, '%') != 0) //0x1F68
        error = SCE_ERROR_KERNEL_UNKNOWN_MODULE_FILE;
    else if (option != null){ //0x1F78
        if (!pspK1StaBufOk(option, 20))
            error = SCE_ERROR_KERNEL_ILLEGAL_ADRESS;
        else if (sceKernelGetCompiledSdkVersion() >= 0x2080000 && option->size != 20)
            error = SCE_ERROR_KERNEL_ILLEGAL_SIZE;
    }

    if (error >= 0){ //0x1E40
        error = fid = sceIoOpen(path, 0x04000001, 0777); //0x1E54
        if (error >= 0){ //0x1E60
            if (sceIoIoctl(fid, 0x00208013, null, 0, null, 0) < 0) //0x1E8C
                error = SCE_ERROR_KERNEL_PROHIBIT_LOADMODULE_DEVICE;
            else {
                memset(localVar, 0, 16); //0x1EA0
                for (int i = 0; i < 160/4; i++)
                    ((s32 *)param)[i] = 0;

                param.apiType = apiType; //0x1EBC
                param.endCMD = MM_COMMAND_1; //0x1EC8
                param.startCMD = MM_COMMAND_0; //0x1ED4
                param.fileBase = null; //0x1EE0
                param.unk24 = fid; //0x1EEC
                param.unk124 = 0; //0x1EF4

                if (sceIoIoctl(fid, 0x00208081, null, 0, null, 0)) //0x1EF8
                    param.unk100 = 16; //0x1F00

                memcpy(&param.unk128, localVar, 16); //0x1F10
                error = sub_000075B4(param, option); //0x1F1C
            }

            sceIoClose(fid);
        }

    }

    pspSetK1(oldK1);
    return error;
}

//0x00001FD8
//ModuleMgrForKernel_FE61F16D
void sceKernelLoadModuleMs1(s32 apiType, const char *path, void arg1, SceKernelLMOption *option)
{
    SceModuleMgrParam param;
    SceUID fid;
    s32 error = SCE_ERROR_OK;
    s32 k1 = pspShiftK1();

    if (sceKernelIsIntrContext()) //0x2010
        error = SCE_ERROR_KERNEL_CANNOT_BE_CALLED_FROM_INTERRUPT; 
    else if (pspK1IsUserMode()) //0x201C
        error = SCE_ERROR_KERNEL_ILLEGAL_PERMISSION_CALL;
    else if (path == null) //0x2028
        error = SCE_ERROR_KERNEL_ILLEGAL_ADDR;
    else if (!pspK1PtrIsOk(path)) //0x2038
        error = SCE_ERROR_KERNEL_ILLEGAL_ADDR;
    else if (strchr(path, '%') != 0) //0x2148
        error = SCE_ERROR_KERNEL_UNKNOWN_MODULE_FILE;
    else if (option != null && pspK1StaBufOk(option, 20)) //0x2164
        error = SCE_ERROR_KERNEL_ILLEGAL_ADDR;
    else if (sceKernelGetCompiledSdkVersion() >= 0x2080000 && option->size != 20) //0x21A0
        error = SCE_ERROR_KERNEL_ILLEGAL_SIZE;


    if (!error){
        fid = sceIoOpen(path, 0x04000001, 0777); //0x2058 
        if (fid < 0) //0x2064
            error = fid;
        else {
            if (sceIoIoctl(fid, 0x00208013, null, 0, null, 0) < 0) //0x2090
                error = SCE_ERROR_KERNEL_PROHIBIT_LOADMODULE_DEVICE;
            else {
                for (int i = 0; i < sizeof(SceModuleMgrParam) / 4; i++)
                    ((void*)param)[i] = 0;

                param.apiType = apiType; //0x20AC
                param.unk1 = 1; //0x20B8
                param.unk0 = 0; //0x20C4
                param.unk64 = 0; //0x20D0
                param.unk24 = fid;
                param.unk124 = 0;
                if (sceIoIoctl(fid, 0x00208081, null, 0, null, 0) >= 0)
                    param.unk100 = 16;

                error = sub_000075B4(param, option);
            }
            SceIoClose(fid);
            
        }
    }

    pspSetK1(oldK1);
    return error;
}  


//0x000021B0
//sceKernelLoadModuleMs2 probably
void ModuleMgrForKernel_7BD53193(s32 apiType, const u8 *path, void arg2, SceKernelLMOption *option)
{
    SceUID fid;
    SceModuleMgrParam param;
    s32 error = SCE_ERROR_OK;
    s32 oldK1 = pspShiftK1();

    if (sceKernelIsIntrContext()) //0x21E8
        error = SCE_ERROR_KERNEL_CANNOT_BE_CALLED_FROM_INTERRUPT;
    else if (pspK1IsUserMode()) //0x21F4
        error = SCE_ERROR_KERNEL_ILLEGAL_PERMISSION_CALL;
    else if (path == null) //0x2200
        error = SCE_ERROR_KERNEL_ILLGAL_ADDRESS;
    else if (!pspK1PtrIsOk(path)) //0x2210
        error = SCE_ERROR_KERNEL_ILLEGAL_ADDRESS;
    else if (strchr(path, '%') != 0) //0x2318
        error = SCE_ERROR_KERNEL_UNKNOWN_MODULE_FILE;
    else if (option != null) { //0x2328
        if (!pspK1StaBufOk(option, 20))
            error = SCE_ERROR_KERNEL_ILLEGAL_ADDRESS;
        else if (sceKernelGetCompiledSdkVersion() >= 0x02080000 && option->size != 20)
            error = SCE_ERROR_KERNEL_ILLEGAL_SIZE;
    }

    if (error == SCE_ERROR_OK){ //0x221C
        fid = sceIoOpen(path, 0x04000001, 0777); //0x2230
        if (fid < 0) //0x223C
            error = (s32)fid;
        else {
            error sceIoIoctl(fid, 0x00208013, null, 0, null, 0); //0x225C
            if (error >= 0) { //0x2268
                for (int i = 0; i < 160/4; i++)
                    ((s32 *)param)[i] = 0;

                param.apiType = apiType; //0x2284
                param.endCMD = MM_COMMAND_1; //0x2290
                param.startCMD = MM_COMMAND_0; //0x229C
                param.fileBase = null; //0x22A8
                param.fid = fid; //0x22B4
                param.unk124 = 0; //0x22BC

                if (sceIoIoctl(fid, 0x00208081, null, 0, null, 0) >= 0) //0x22C0
                    param.unk100 = 16;

                error = sub_000075B4(param, option); //0x22D0

            } else 
                error = SCE_ERROR_KERNEL_PROHIBIT_LOADMODULE_DEVICE;

            sceIoClose(fid); //0x22DC
        }

    }
    pspSetK1(oldK1);
    return error;
}

//0x00002388
//ModuleMgrForKernel_D60AB6CC
void sceKernelLoadModuleMs3(s32 apiType, const u8 *path, void arg2, SceKernelLMOption *option)
{
    SceModuleMgrParam param;
    SceUID fid;
    u8 localVar[16];
    s32 error = SCE_ERROR_OK;
    s32 oldK1 = pspShiftK1();

    if (sceKernelIsIntrContext()) //0x23C4
        error = SCE_ERROR_KERNEL_CANNOT_BE_CALLED_FROM_INTERRUPT;
    else if (pspK1IsUserMode()) //0x23D0
        error = SCE_ERROR_KERNEL_ILLEGAL_PERMISSION_CALL;
    else if (path == null) //0x23DC
        error = SCE_ERROR_KERNEL_ILLEGAL_ADDRESS;
    else if (!pspK1PtrIsOk(path)) //0x23EC
        error = SCE_ERROR_KERNEL_ILLEGAL_ADDRESS;
    else if (strchr(path, '%') != 0) //0x2520
        error = SCE_ERROR_KERNEL_UNKNOWN_MODULE_FILE;
    else if (param != null){ //0x2530
        if (!pspK1StaBufOk(param, 20))
            error = SCE_ERROR_KERNEL_ILLEGAL_ADDRESS;
        else if (sceKernelGetCompiledSdkVersion() >= 0x2080000 && param.size != 20) //0x2580
            error = SCE_ERROR_KERNEL_ILLEGAL_SIZE;
    }

    if (error >= 0){
        fid = sceIoOpen(path, 0x04000001, 0777); //0x240C
        if (fid < 0)
            error = fid;
        else { //0x2418
            error = sceIoIoctl(fid, 0x00208013, null, 0, null , 0); //0x2438
            if (error < 0)
                error = SCE_ERROR_KERNEL_PROHIBIT_LOADMODULE_DEVICE;
            else {
                memset(localVar, 0, 16); //0x2458
                for (int i = 0; i < 160/4; i++)
                    ((s32 *)param)[i] = 0;
                param.apiType = apiType; //0x2474
                param.endCMD = MM_COMMAND_1; //0x2480
                param.startCMD = MM_COMMAND_0; //0x248C
                param.fileBase = null; //0x2498
                param.fid = fid; //0x24A4
                param.unk124 = 0; //0x24AC
                if (sceIoIoctl(fid, 0x00208081, null, 0, null, 0) >= 0) //0x24B0
                    param.unk100 = 16; //0x24BC

                memcpy(&param.unk128, localVar, 0); //0x24C8
                error = sub_000075B4(param, option);
            }
            sceIoClose(fid); //0x24E0
        }
    }
    pspSetK1(oldK1);
    return error;
}

//0x00002590
//ModuleMgrForKernel_76F0E956
void sceKernelLoadModuleMs4(s32 apiType, const u8 *path, void arg2, SceKernelLMOption *option)
{
    SceModuleMgrParam param;
    SceUID fid;
    s32 error = SCE_ERROR_OK;
    s32 oldK1 = pspShiftK1();

    if (sceKernelIsIntrContext()) //0x25C8
        error = SCE_ERROR_KERNEL_CANNOT_BE_CALLED_FROM_INTERRUPT;
    else if (pspK1IsUserMode()) //0x25D4
        error = SCE_ERROR_KERNEL_ILLEGAL_PERMISSION_CALL;
    else if (path == null) //0x25E0
        error = SCE_ERROR_KERNEL_ILLGAL_ADDRESS;
    else if (!pspK1PtrIsOk(path)) //0x25F0
        error = SCE_ERROR_KERNEL_ILLEGAL_ADDRESS;
    else if (strchr(path, '37') != 0) //0x26F8
        error = SCE_ERROR_KERNEL_UNKNOWN_MODULE_FILE;
    else if (option != null){ //0x2760
        if (!pspK1StaBufOk(option, 20))
            error = SCE_ERROR_KERNEL_ILLEGAL_ADDRESS;
        else if (sceKernelGetCompiledSdkVersion() >= 0x02080000 && option->size != 20)
            error = SCE_ERROR_KERNEL_ILLEGAL_SIZE;
    }

    if (error = SCE_ERROR_OK){
        fid = sceIoOpen(path, 0x04000001, 0777); //0x2610
        if (fid < 0) //0x261C
            error = fid;
        else {
            error = sceIoIoctl(fid, 0x00208013, null, 0, null, 0); //0x263C
            if (error < 0) //0x2648
                error = SCE_ERROR_KERNEL_PROHIBIT_LOADMODULE_DEVICE;
            else {
                for (int i = 0; i < 160/4, i++)
                    ((s32 *)param)[i] = 0;
                param.apiType = apiType; //0x2664
                param.endCMD = MM_COMMAND_1; //0x2670
                param.startCMD = MM_COMMAND_0; //0x267C
                param.fileBase = null; //0x2688
                param.fid = fid; //0x2694
                param.unk124 = 0; //0x269C
                if (sceIoIoctl(fid, 0x00208081, null, 0, null, 0) >= 0) //0x2698
                    param.unk100 = 16;

                error = sub_000075B4(param, option); //0x26B0
            }

            sceIoClose(fid); //0x26BC

        } 
    }

    pspShiftK1(oldK1);
    return error;
}

//0x00002768
//ModuleMgrForKernel_4E8A2C9D
void sceKernelLoadModuleForLoadExecVSHMs5(s32 apiType, const u8 *path, void arg1, SceKernelLMOption *option)
{

    SceModule *mod;
    SceUID fid;
    u8 localVar[16];
    SceModuleMgrParam param;
    s32 error = SCE_ERROR_OK;
    s32 oldK1 = pspShiftK1();

    if (sceKernelIsIntrContext()) //0x279C
        error = SCE_ERROR_KERNEL_CANNOT_BE_CALLED_FROM_INTERRUPT;
    else if (pspK1IsUserMode()) //0x27B4
        error = SCE_ERROR_KERNEL_ILLEGAL_PERMISSION_CALL;
    else if (path == null) //0x27C0
        error = SCE_ERROR_KERNEL_ILLEGAL_ADDRESS;
    else if (!pspK1PtrIsOk(path)) //0x27D0
        error = SCE_ERROR_KERNEL_ILLEGAL_ADDRESS;
    else if (strchr(path, '%') != 0) //0x2908
        error = SCE_ERROR_KERNEL_UNKNOWN_MODULE_FILE;
    else if (option != null){ //0x2918
        if (!pspK1StaBufOk(option, 20))
            error = SCE_ERROR_KERNEL_ILLEGAL_ADDRESS;
        else if (sceKernelGetCompiledSdkVersion() >= 0x02080000 && option->size != 20)
            error = SCE_ERROR_KERNEL_ILLEGAL_SIZE;
    }

    if (error == SCE_ERROR_OK){
        fid = sceIoOpen(path, 0x04000001, 0777); //0x27F0
        if (fid < 0) //0x27FC
            error =  fid;
        else {
            error = sceIoIoctl(fid, 0x00208013, null, 0, null, 0); //0x2820
            if (error < 0) //0x2828
                error = SCE_ERROR_KERNEL_PROHIBIT_LOADMODULE_DEVICE;
            else {
                error = sceKernelGetId(path, localVar); //0x2838
                if (error >= 0){ //0x2840
                    for (int i = 0; i < 160/4; i++)
                        ((s32 *)param)[i] = 0;

                    param.apiType = apiType; //0x2858
                    param.endCMD = MM_COMMAND_1; //0x286C
                    param.startCMD = MM_COMMAND_0; //0x2878
                    param.fileBase = null; //0x2884
                    param.fid = fid; //0x288C
                    param.unk124 = 0; //0x2894
                    if (sceIoIoctl(fid, 0x00208081, null, 0, null, 0) >= 0)
                        param.unk100 = 16; //0x28A0

                    memcpy(&param.unk128, localVar, 16); //0x28AC
                    error = sub_000075B4(param, option);
                }
            }
            sceIoClose(fid); //0x28C4
        }

    }

    pspSetK1(oldK1);
    return error;
}

//0x00002978
void ModuleMgrForKernel_E8422026(s32 apiType, const u8 *path, void arg1, SceKernelLMOption *option)
{
    SceModule *mod;
    SceUID fid;
    SceModuleMgrParam param;
    u8 localVar[16];
    s32 error = SCE_ERROR_OK;
    s32 oldK1 = pspShiftK1();

    if (sceKernelIsIntrContext()) //0x29AC
        error = SCE_ERROR_KERNEL_CANNOT_BE_CALLED_FROM_INTERRUPT;
    else if (pspK1IsUserMode()) //0x29C4
        error = SCE_ERROR_KERNEL_ILLEGAL_PERMISSION_CALL;
    else if (path == null) //0x29D0
        error = SCE_ERROR_KERNEL_ILLEGAL_ADDRESS;
    else if (!pspK1PtrIsOk(path)) //0x29E0
        error = SCE_ERROR_KERNEL_ILLEGAL_ADDRESS;
    else if (strchr(path, '%') != 0) //0x2B18
        error = SCE_ERROR_KERNEL_UNKNOWN_MODULE_FILE;
    else if (option != null){ //0x2B28
        if (!pspK1StaBufOk(param, 20)) //0x2B3C
            error = SCE_ERROR_KERNEL_ILLEGAL_ADDRESS;
        if (sceKernelGetCompiledSdkVersion() >= 0x02080000 && option->size != 20)
            error = SCE_ERROR_KERNEL_ILLEGAL_SIZE;
    }

    if (error == SCE_ERROR_OK) {
        fid = sceIoOpen(path, 0x04000001, 0777); //0x2A00
        if (fid < 0) //0x 2A0C
            error = fid;
        else {
            error = sceIoIoctl(fid, 0x00208013, null, 0, null, 0); //0x2A38
            if (error < 0) //0x2A38
                error = SCE_ERROR_KERNEL_PROHIBIT_LOADMODULE_DEVICE;
            else {
                error = sceKernelGetId(path, localVar); //0x2A48
                if (error >= 0){ //0x2A50
                    for (int i = 0; i < 160/4; i ++)
                        ((s32 *)param)[i] = 0;
                    param.apiType = apiType; //0x2A70
                    param.endCMD = MM_COMMAND_1; //0x2A7C
                    param.startCMD = MM_COMMAND_0; //0x2A88
                    param.fileBase = null; //0x2A94
                    param.fid = fid; //0x2A9C
                    param.unk124 = 0; //0x2AA4
                    if (sceIoIoctl(fid, 0x00208081, null, 0, null, 0) >= 0) //0x2AA8
                        param.unk100 = 16; //0x2AB0
                    memcpy(&param.unk128, localVar, 16); //0x2AC0
                    error = sub_000075B4(param, option); //0x2AC8
                }

            }
            sceIoClose(fid); //0x2AD4
        }
    }

    pspSetK1(oldK1);
    retur error;


}

//0x00002B88
void ModuleMgrForKernel_8DD336D4(s32 apiType, const u8 *path, void arg1, SceKernelLMOption *option)
{
    SceModule *mod;
    SceUID fid;
    SceModuleMgrParam param;
    u8 localVar[16];
    s32 error = SCE_ERROR_OK;
    s32 oldK1 = pspShiftK1();

    if (sceKernelIsIntrContext()) //0x2BC4
        error = SCE_ERROR_KERNEL_CANNOT_BE_CALLED_FROM_INTERRUPT;
    else if (pspK1IsUserMode()) //0x2BD0
        error = SCE_ERROR_KERNEL_ILLEGAL_PERMISSION_CALL;
    else if (path == null) //0x29DC
        error = SCE_ERROR_KERNEL_ILLEGAL_ADDRESS;
    else if (!pspK1PtrIsOk(path)) //0x2BEC
        error = SCE_ERROR_KERNEL_ILLEGAL_ADDRESS;
    else if (strchr(path, '%') != 0) //0x2D20
        error = SCE_ERROR_KERNEL_UNKNOWN_MODULE_FILE;
    else if (option != null){ //0x2D30
        if (!pspK1StaBufOk(param, 20)) //0x2D44
            error = SCE_ERROR_KERNEL_ILLEGAL_ADDRESS;
        if (sceKernelGetCompiledSdkVersion() >= 0x02080000 && option->size != 20)
            error = SCE_ERROR_KERNEL_ILLEGAL_SIZE;
    }

    if (error == SCE_ERROR_OK) {
        fid = sceIoOpen(path, 0x04000001, 0777); //0x2C0C
        if (fid < 0) //0x 2C18
            error = fid;
        else {
            error = sceIoIoctl(fid, 0x00208013, null, 0, null, 0); //0x2C38
            if (error < 0) //0x2C44
                error = SCE_ERROR_KERNEL_PROHIBIT_LOADMODULE_DEVICE;
            else {
                memset(localVar, 0, 16); //0x2C58
                for (int i = 0; i < 160/4; i ++)
                    ((s32 *)param)[i] = 0;

                param.apiType = apiType; //0xCA74
                param.endCMD = MM_COMMAND_1; //0x2C80
                param.startCMD = MM_COMMAND_0; //0x2C8C
                param.fileBase = null; //0x2C98
                param.fid = fid; //0x2CA4
                param.unk124 = 0; //0x2CB4
                if (sceIoIoctl(fid, 0x00208081, null, 0, null, 0) >= 0) //0x2CB0
                    param.unk100 = 16; //0x2CBC
                memcpy(&param.unk128, localVar, 16); //0x2CC8
                error = sub_000075B4(param, option); //0x2CD4
            

            }
            sceIoClose(fid); //0x2CE0
        }
    }

    pspSetK1(oldK1);
    retur error;


}

//0x00002D90
//sceKernelLoadModuleFromFileOffset or something...
s32 ModuleMgrForKernel_30727524(s32 apiType, const u8 *path, SceOff fileOffset, u8 unkBuf[16], void arg2, SceKernelLMOption *option)
{
    SceUID *fid;
    SceModuleMgrParam param;
    s32 error = SCE_ERROR_OK;
    s32 oldK1 = pspShiftK1();

    if (sceKernelIsIntrContext()) //0x2DE0
        error = SCE_ERROR_KERNEL_CANNOT_BE_CALLED_FROM_INTERRUPT;
    else if (pspK1IsUserMode()) //0x2DEC
        error = SCE_ERROR_KERNEL_ILLEGAL_PERMISSION_CALL;
    else if (path == null) //0x2DF8
        error = SCE_ERROR_KERNEL_ILLEGAL_ADDRESS;
    else if (!pspK1PtrIsOk(path)) //0x2E08
        error = SCE_ERROR_KERNEL_ILLEGAL_ADDRESS;
    else if (strchr(path, '%') != 0) //0x2F58
        error = SCE_ERROR_KERNEL_UNKNOWN_MODULE_FILE;
    else if (option != null){ //0x2F68
        if (!pspK1StaBufOk(option, 20)) //0x2F7C
            error = SCE_ERROR_KERNEL_ILLEGAL_ADDRESS;
        else if (sceKernelGetCompiledSdkVersion() & 0xFFFF0000 >= 0x02080000
                && option->size != 20) //0x2FB8
            error = SCE_ERROR_KERNEL_ILLEGAL_SIZE;
    }

    if (error == SCE_ERROR_OK){
        if (!pspK1DynBufOk(arg4, 16)) //0x2E2C
            error = SCE_ERROR_KERNEL_ILLEGAL_ADDRESS;
        else {
            fid = sceIoOpen(path, 0x04000001, 0777); //0x2E40
            if (fid < 0) //0x2E4C
                error = fid;
            else {
                sceIoLseek(fid, fileOffset, SEEK_SET); //0x2E64
                error = sceIoIoctl(fid, 0x00208013, null, 0 ,null, 0); //0x2E84
                if (error < 0) //0x2E8C
                    error = SCE_ERROR_KERNEL_PROHIBIT_LOADMODULE_DEVICE;
                else {
                    for (int i = 0; i < 160/4; i++)
                        ((s32 *)param)[i] = 0;
                    param.apiType = apiType; //0x2EA8
                    param.endCMD = MM_COMMAND_1; //0x2EB4
                    param.startCMD = MM_COMMAND_0; //0x2EC0
                    param.fileBase = null; //0x2ECC
                    param.fid = fid; //0x2ED8
                    param.unk124 = 0; //0x2EE0
                    if (sceIoIoctl(fid, 0x00208081, null, 0, null, 0) >= 0)
                        param.unk100 = 16; //2EEC
                    memcpy(&param.unk128, unkBuf, 16); //0x2EF8
                    error = sub_000075B4(param, option);
                }

                sceIoClose(fid); //0x2F10
            }
        }
    }

    pspSetK1(oldK1);
    return error;
}

//0x00002FC8
//ModuleMgrForKernel_D5DDAB1F
void sceKernelLoadModuleVSH(const u8 *path, void arg1, SceKernelLMOption *option)
{
    SceModuleMgrParam param;
    SceUID fid;
    s32 oldK1 = pspShiftK1();
    s32 error = SCE_ERROR_OK;

    if (sceKernelIsIntrContext()) //0x2FF8
        error = SCE_ERROR_KERNEL_CANNOT_BE_CALLED_FROM_INTERRUPT;
    else if (!pspK1IsUserMode()) //0x3000
        error = SCE_ERROR_KERNEL_ILLEGAL_PERMISSION_CALL;   
    else if (sceKernelGetUserLevel() != 4) //0x303C
        error = SCE_ERROR_KERNEL_ILLEGAL_PERMISSION_CALL;
    else if (path == null) //0x3048
        error = SCE_ERROR_KERNEL_ILLEGAL_ADDRESS;
    else if (!pspK1PtrIsOk(path)) //0x3168
        error = SCE_ERROR_KERNEL_ILLEGAL_ADDRESS;
    else if (strchr(path, '%') != 0) //0x3174
        error = SCE_ERROR_KERNEL_UNKNOWN_MODULE_FILE;
    else if (option != null){ //0x3184
        if (!pspK1StaBufOk(option, 20)) //0x3198
            error = SCE_ERROR_KERNEL_ILLEGAL_ADDRESS;
        else if (sceKernelGetCompiledSdkVersion() >= 0x02080000 && option->size != 20)
            error = SCE_ERROR_KERNEL_ILLEGAL_SIZE;
    }


    if (error == SCE_ERROR_OK){
        fid = sceIoOpen(path, 0x04000001, 0777); //0x3078
        if (fid < 0) //0x3084
            error = fid;
        else {
            if (sceIoIoctl(fid, 0x00208003, null, 0, null, 0) < 0)
                error = SCE_ERROR_KERNEL_PROHIBIT_LOADMODULE_DEVICE;
            else {
                for (int i = 0; i < 160/4; i++)
                    ((s32 *)param)[i] = 0;
                param.apiType = API_TYPE_VSH; //0x30D4
                param.endCMD = MM_COMMAND_1; //0x30E0
                param.startCMD = MM_COMMAND_0; //0x30EC
                param.fileBase = null; //0x30F5
                param.fid = fid; //0x3100
                param.unk124 = 0; //0x3108
                if (sceIoIoctl(fid, 0x00208081, null, 0, null, 0) >= 0)
                    param.unk100 = 16; //0x3114
                if (sceIoIoctl(fid, 0x00208082, null, 0, null, 0) < 0)
                    param.unk124 = 1; //0x3160
                error = sub_000075B4(param, option);
            }

            sceIoClose(fid);
        }
    }

    pspSetK1(oldK1);
    return error;
    
}

//0x000031E4
//ModuleMgrForKernel_CBA02988
void sceKernelLoadModuleVSHByID(SceUID fid, void arg1, SceKernelLMOption *option)
{
    SceModuleMgrParam param;
    s32 oldK1 = pspShiftK1();
    s32 error = SCE_ERROR_OK;

    if (sceKernelIsIntrContext()) //0x3214
        error = SCE_ERROR_KERNEL_CANNOT_BE_CALLED_FROM_INTERRUPT;
    else if (!pspK1IsUserMode()) //0x321C
        error = SCE_ERROR_KERNEL_ILLEGAL_PERMISSION_CALL;
    else if (sceKernelGetUserLevel() != 4) //0x3258
        error = SCE_ERROR_KERNEL_ILLEGAL_PERMISSION_CALL;
    else if (option != null){ //0x3260
        if (!pspK1StaBufOk(option, 20))
            error = SCE_ERROR_KERNEL_ILLEGAL_ADDRESS;
        else if (sceKernelGetCompiledSdkVersion() >= 0x02080000 && option->size != 20)
            error = SCE_ERROR_KERNEL_ILLEGAL_SIZE;
    }

    if (error == SCE_ERROR_OK){
        error = sceIoValidateFd(fid, 4); //0x32B4
        if (error >= 0){ //0x32BC
            if (sceIoIoctl(fid, 0x00208003, null, 0, null, 0) < 0) //0x32E8
                error = SCE_ERROR_KERNEL_PROHIBIT_LOADMODULE_DEVICE;
            else {
                for (int i = 0; i < 160/4; i++)
                    ((s32 *)param)[i] = 0;
                param.apiType = API_TYPE_VSH; //0x330C
                param.endCMD = MM_COMMAND_1; //0x3318
                param.startCMD = MM_COMMAND_0; //0x3324
                param.fileBase = null; //0x3330
                param.fid = fid; //0x3338
                param.unk124 = 0; //0x3340
                if (sceIoIoctl(fid, 0x00208081, null, 0, null, 0) >= 0)
                    param.unk100 = 16;
                if (sceIoIoctl(fid, 0x00208082, null, 0, null, 0) < 0)
                    param.unk124 = 1;
                error = sub_000075B4(param, option);
            }
        } 
    }

    pspSetK1(oldK1);
    return error;
}

//0x00003394
//ModuleMgrForKernel_939E4270
void sceKernelLoadModule(const u8 *path, void arg1, SceKernelLMOption *option)
{
    SceModuleMgrParam param;
    SceUID fid;
    s32 oldK1 = pspShiftK1();
    s32 error = SCE_ERROR_OK;

    if (sceKernelIsIntrContext()) //0x33C4
        error = SCE_ERROR_KERNEL_CANNOT_BE_CALLED_FROM_INTERRUPT;
    else if (pspK1IsUserMode()) //0x33D0
        error = SCE_ERROR_KERNEL_ILLEGAL_PERMISSION_CALL;   
    else if (path == null) //0x33DC
        error = SCE_ERROR_KERNEL_ILLEGAL_ADDRESS;
    else if (!pspK1PtrIsOk(path)) //0x33EC
        error = SCE_ERROR_KERNEL_ILLEGAL_ADDRESS;
    else if (strchr(path, '%') != 0) //0x3520
        error = SCE_ERROR_KERNEL_UNKNOWN_MODULE_FILE;
    else if (option != null){ //0x3534
        if (!pspK1StaBufOk(option, 20)) //0x3544
            error = SCE_ERROR_KERNEL_ILLEGAL_ADDRESS;
        else if (sceKernelGetCompiledSdkVersion() >= 0x02080000 && option->size != 20)
            error = SCE_ERROR_KERNEL_ILLEGAL_SIZE;
    }


    if (error == SCE_ERROR_OK){
        fid = sceIoOpen(path, 0x04000001, 0777); //0x340C
        if (fid < 0) //0x3418
            error = fid;
        else {
            if (sceIoIoctl(fid, 0x00208006, null, 0, null, 0) < 0)
                error = SCE_ERROR_KERNEL_PROHIBIT_LOADMODULE_DEVICE;
            else {
                for (int i = 0; i < 160/4; i++)
                    ((s32 *)param)[i] = 0;
                param.endCMD = MM_COMMAND_1; //0x3464
                param.apiType = API_TYPE_NONE; //0x3470
                param.startCMD = MM_COMMAND_0; //0x347C
                param.fileBase = null; //0x3488
                param.fid = fid; //0x3490
                param.unk124 = 0; //0x3498
                if (sceIoIoctl(fid, 0x00208081, null, 0, null, 0) >= 0)
                    param.unk100 = 16; //0x34A4
                if (sceIoIoctl(fid, 0x00208082, null, 0, null, 0) < 0)
                    param.unk124 = 1; //0x3510
                error = sub_000075B4(param, option);
            }

            sceIoClose(fid);
        }
    }

    pspSetK1(oldK1);
    return error;
    
}

//0x00003590
//ModuleMgrForKernel_EEC2A745
void sceKernelLoadModuleByID(SceUID fid, void arg1, SceKernelLMOption *option)
{
    SceModuleMgrParam param;
    s32 oldK1 = pspShiftK1();
    s32 error = SCE_ERROR_OK;

    if (sceKernelIsIntrContext()) //0x35C0
        error = SCE_ERROR_KERNEL_CANNOT_BE_CALLED_FROM_INTERRUPT;
    else if (pspK1IsUserMode()) //0x35CC
        error = SCE_ERROR_KERNEL_ILLEGAL_PERMISSION_CALL;
    else if (option != null){ //0x35D4
        if (!pspK1StaBufOk(option, 20))
            error = SCE_ERROR_KERNEL_ILLEGAL_ADDRESS;
        else if (sceKernelGetCompiledSdkVersion() >= 0x02080000 && option->size != 20)
            error = SCE_ERROR_KERNEL_ILLEGAL_SIZE;
    }

    if (error == SCE_ERROR_OK){
        error = sceIoValidateFd(fid, 4); //0x364C
        if (error >= 0){ //0x3654
            if (sceIoIoctl(fid, 0x00208006, null, 0, null, 0) < 0) //0x3680
                error = SCE_ERROR_KERNEL_PROHIBIT_LOADMODULE_DEVICE;
            else {
                for (int i = 0; i < 160/4; i++)
                    ((s32 *)param)[i] = 0;
                param.endCMD = MM_COMMAND_1; //0x36A0
                param.apiType = API_TYPE_NONE; //0x36AC
                param.startCMD = MM_COMMAND_0; //0x36B8
                param.fileBase = null; //0x36C4
                param.fid = fid; //0x36CC
                param.unk124 = 0; //0x36D4
                if (sceIoIoctl(fid, 0x00208081, null, 0, null, 0) >= 0)
                    param.unk100 = 16;
                if (sceIoIoctl(fid, 0x00208082, null, 0, null, 0) < 0)
                    param.unk124 = 1;
                error = sub_000075B4(param, option);
            }
        } 
    }

    pspSetK1(oldK1);
    return error;
}

//0x00003728
//ModuleMgrForKernel_D4EE2D26
void sceKernelLoadModuleToBlock(const u8 *path, s32 arg1, s32 *arg2, void arg4, SceKernelLMOption *option)
{
    SceModuleMgrParam param;
    SceUID fid;
    s32 error = SCE_ERROR_OK;
    s32 oldK1 = pspShiftK1();
    s32 *data;

    for (int i = 0; i < 160/4; i++)
        ((s32 *)param)[i] = 0;

    if (sceKernelIsIntrContext()) //0x3780
        error = SCE_ERROR_KERNEL_CANNOT_BE_CALLED_FROM_INTERRUPT;
    else if (pspK1IsUserMode()) //0x378C
        error = SCE_ERROR_KERNEL_ILLEGAL_PERMISSION_CALL;
    else if (path == null) //0x3798
        error = SCE_ERROR_KERNEL_ILLGAL_ADDRESS;
    else if (!pspK1PtrIsOk(path)) //0x37A8
        error = SCE_ERROR_KERNEL_ILLEGAL_ADDRESS;
    else if (strchr(path, '%') != 0) //0x3950
        error = SCE_ERROR_KERNEL_UNKOWN_MODULE_FILE;
    else if (option != null){ //0x3960
        if (!pspK1StaBufOk(option, 20)) //0x3974
            error = SCE_ERROR_KERNEL_ILLEGAL_ADDRESS;
        else if (sceKernelGetCompiledSdkVersion() >= 0x02080000 && option->size != 20)
            error = SCE_ERROR_KERNEL_ILLEGAL_SIZE;
    }

    if (error >= 0) {
        if (arg2 == null) //0x37BC
            error = SCE_ERROR_KERNEL_INVALID_ARGUMENT;
        else if (option != null && option->position >= 2) //037D4
            error = SCE_ERROR_KERNEL_INVALID_ARGUMENT;
        else if (!pspK1StaBufOk(arg2, 4)) //0x37E8
            error = SCE_ERROR_KERNEL_ILLEGAL_ADDRESS;
        else {
            fid = sceIoOpen(path, 0x04000001, 0777); //0x37F8
            if (fid < 0) //0x38F0
                error = fid;
            else {
                if (sceIoIoctl(fid, 0x00208007, null, 0, null, 0) < 0)
                    error = SCE_ERROR_KERNEL_PROHIBIT_LOADMODULE_DEVICE;
                else {
                    for (int i = 0; i < 160/4; i++)
                        ((s32 *)param)[i] = 0;
                    param.apiType = API_TYPE_3; //0x3854
                    param.endCMD = MM_COMMAND_1; //0x3860
                    param.startCMD = MM_COMMAND_0; //0x386C
                    param.fileBase = null; //0x3878
                    param.fid = fid; //0x3880
                    param.unk124 = 0; //0x3888
                    if (sceIoIoctl(fid, 0x00208081, null, 0, null, 0) >= 0)
                        param.unk100 = 16;
                    param.unk104 = arg1; //0x3898
                    param.unk8 = data; //0x38C0
                    if (sceIoIoctl(fid, 0x00208082, null, 0, null, 0) < 0)
                        param.unk124 = 1;
                    error = sub_000075B4(param, option);
                    *arg2 = *data; //0x38E0
                }
            }
            sceIoClose(fid);
        }

    }

    pspSetK1(oldK1);
    return error;
}

//0x000039C0
//ModuleMgrForKernel_F7C7FEBC
void sceKernelLoadModuleBootInitConfig(const u8 *path, void arg1, SceKernelLMOption *option)
{
    SceModuleMgrParam param;
    SceUID fid;
    s32 error = SCE_ERROR_OK;
    s32 oldK1 = pspShiftK1();

    if (KDebugForKernel_ACF427DC() == 0) //0x39F0
        error = SCE_ERROR_KERNEL_ILLEGAL_PERMISSION_CALL;
    else if (sceKernelIsIntrContext()) //0x3A28
        error = SCE_ERROR_KERNEL_CANNOT_BE_CALLED_FROM_INTERRUPT;
    else if (pspK1IsUserMode()) //0x3A34
        error = SCE_ERROR_KERNEL_ILLEGAL_PERMISSION_CALL;
    else if (path == null) //0x3A40
        error = SCE_ERROR_KERNEL_ILLEGAL_ADDRESS;
    else if (!pspK1PtrIsOk(path)) //0x3A50
        error = SCE_ERROR_KERNEL_ILLEGAL_ADDRESS;
    else if (strchr(path, '%') != 0) //0x3B3C
        error = SCE_ERROR_KERNEL_UNKNOWN_MODULE_FILE;
    else if (option != null){ //0x3B4C
        if (!pspK1StaBufOk(option, 20)) //0x3B60
            error = SCE_ERROR_KERNEL_ILLEGAL_ADDRESS;
        else if (sceKernelGetCompiledSdkVersion() >= 0x02080000 && option->size != 20)
            error = SCE_ERROR_KERNEL_ILLEGAL_SIZE;
    }

    if (error == SCE_ERROR_OK){
        fid = sceIoOpen(path, 0x04000001, 0777); //0x3A70
        if (fid < 0)
            error = fid;
        else {
            if (sceIoIoctl(fid, 0x00208009, null, 0, null, 0) < 0) //0x3AAC
                error = SCE_ERROR_KERNEL_PROHIBIT_LOADMODULE_DEVICE;
            else {
                for (int i = 0; i < 160/4; i++)
                    ((s32 *)param)[i] = 0;
                param.apiType = API_TYPE_BOOT_INIT_CONFIG; //0x3ACC
                param.endCMD = MM_COMMAND_1; //0x3AD8
                param.startCMD = MM_COMMAND_0; //0x3AE4
                param.fileBase = null; //0x3AF0
                param.fid = fid; //0x3AF8
                param.unk124 = 0; //0x3B00
                if (sceIoIoctl(fid, 0x00208081, null, 0, null, 0) >= 0)
                    param.unk100 = 16;
                error = sub_000075B4(param, option);
            }
            sceIoClose(fid);
        }
    }

    pspSetK1(oldK1);
    return error;
}

//0x00003BAC
void ModuleMgrForKernel_4493E013(const u8 *path, void arg1, SceKernelLMOption *option)
{
    SceModuleMgrParam param;
    SceUID fid;
    s32 error = SCE_ERROR_OK;
    s32 oldK1 = pspShiftK1();

    if (KDebugForKernel_ACF427DC() == 0) //0x3BD0
        error = SCE_ERROR_KERNEL_ILLEGAL_PERMISSION_CALL;
    else if (sceKernelIsIntrContext()) //0x3C14
        error = SCE_ERROR_KERNEL_CANNOT_BE_CALLED_FROM_INTERRUPT;
    else if (pspK1IsUserMode()) //0x3C20
        error = SCE_ERROR_KERNEL_ILLEGAL_PERMISSION_CALL;
    else if (path == null) //0x3C2C
        error = SCE_ERROR_KERNEL_ILLEGAL_ADDRESS;
    else if (!pspK1PtrIsOk(path)) //0x3C3C
        error = SCE_ERROR_KERNEL_ILLEGAL_ADDRESS;
    else if (strchr(path, '%') != 0) //0x3D28
        error = SCE_ERROR_KERNEL_UNKNOWN_MODULE_FILE;
    else if (option != null){ //0x3D38
        if (!pspK1StaBufOk(option, 20)) //0x3D4C
            error = SCE_ERROR_KERNEL_ILLEGAL_ADDRESS;
        else if (sceKernelGetCompiledSdkVersion() >= 0x02080000 && option->size != 20)
            error = SCE_ERROR_KERNEL_ILLEGAL_SIZE;
    }

    if (error == SCE_ERROR_OK){
        fid = sceIoOpen(path, 0x04000001, 0777); //0x3C5C
        if (fid < 0)
            error = fid;
        else {
            if (sceIoIoctl(fid, 0x0020800C, null, 0, null, 0) < 0) //0x3C88
                error = SCE_ERROR_KERNEL_PROHIBIT_LOADMODULE_DEVICE;
            else {
                for (int i = 0; i < 160/4; i++)
                    ((s32 *)param)[i] = 0;
                param.apiType = API_TYPE_112; //0x3CB8
                param.endCMD = MM_COMMAND_1; //0x3CC4
                param.startCMD = MM_COMMAND_0; //0x3CD0
                param.fileBase = null; //0x3CDC
                param.fid = fid; //0x3CE4
                param.unk124 = 0; //0x3CEC
                if (sceIoIoctl(fid, 0x00208081, null, 0, null, 0) >= 0)
                    param.unk100 = 16;
                error = sub_000075B4(param, option);
            }
            sceIoClose(fid);
        }
    }

    pspSetK1(oldK1);
    return error;
}

//0x000046E4
//ModuleMgrForKernel_CC873DFA
s32 sceKernelRebootBeforeForUser(void *arg0)
{

    u8 localVar[16];
    SceUID modList;
    SceUID *modIDs
    SceModule *mod;
    s32 numMods, 
    s32 priority;
    s32 stacksize;
    u32 attr;
    PspSysmemPartitionInfo memPartInfo;
    PspSysmemMemoryBlockInfo memBlockInfo;
    SceKernelThreadOptParam threadOptParam;
    s32 oldGp = pspGetGp();
    s32 error = SCE_ERROR_OK;

    sceKernelLockMutex(g_ModuleMgrUIDS.mutid, 1, 0); //0x4724
    memcpy(localVar, arg0, 16); //0x4734
    localVar[0] = 16; //0x4748
    modList = sceKernelGetModuleListWithAlloc(&numMods); //0x4744
    if (modList < 0) //0x474C
        return (s32)modIDs;
    modIDs = (SceUID *)sceKernelGetBlockHeadAddr(modList);
    for (int i = 0; i < numMods; i++){
        mod = sceKernelFindModuleByUID(modIDs[i]); //0x4774
        if (mod != null && 
            mod->moduleRebootBefore != -1 &&
            mod->status & 0xF = MCB_STATUS_STARTED &&
            mod->status & 0x100 != 0){ //0x477C

            //0x4830 Thread priority/stack size/attr stuff :/
            priority = mod->moduleRebootBeforeThreadPriority;
            stackSize = mod->moduleRebootBeforeThreadStackSize;
            attr = mod->moduleRebootBeforeThreadAttr;

            priority = priority == -1 ? 32 : priority; //0x4864
            stackSize = stackSize == -1 ? 4096 : stackSize; //0x4870
            attr = attr == -1 ? 0 : attr; //0x4874
            switch (mod->attribute & 0x1E00){
            case 0x800: 
                attr |= 0xC0000000;
                break;
            case 0x600:
                attr |= 0xB0000000;
                break;
            case 0x400:
                attr |= 0xA0000000;
                break;
            case 0x200:
                attr |= 0x90000000:
                break;
            default:
                attr |= 0x80000000;
            }

            threadOptParam.size = 8; //0x48A8
            threadOptParam.stackMpid = mod->mpIdData; //0x0x48CC
            memInfo->size = 16; //0x48BC
            if (sceKernelQueryMemoryPartitionInfo(mod->mpIdData, &memPartInfo) < 0
                || memPartInfo.attr & 0x3 == 0)
                threadOptParam.stackMpid = 2;
            pspSetGp(mod->gpValue); //0x4900
            mod->userModThid = sceKernelCreateThread("SceModmgrRebootBefore", mod->moduleRebootBefore, priority, stackSize, attr, &threadOptParam); //0x491C
            pspSetGp(oldGp); //0x4928
            if (sceKernelStartThread(mod->userModThid, 16, sp) == 0) //0x493C
                sceKernelWaitThreadEnd(mod->userModThid, 0); //0x49AC
            sceKernelDeleteThread(mod->usermodThid); //0x4944
            mod->userModThid = -1; //0x4950
            if (KDebugForKernel_47570AC5() == 0) //0x4954
                continue;
            else if (KDebugForKernel_86010FCB(25) == 1) //0x4968
                continue;
            else if (sceKernelGetCompiledSdkVersion() <= 0x03030000)
                continue;
            else if (sceKernelSegmentChecksum(mod) == mod->segmentChecksum)
                continue;
            else {
                pspBreak(0);
                continue;
            }
        }
    }


    //0x47B0
    memBlockInfo.size = 56; //0x47C0
    error = sceKernelQueryMemoryBlockInfo(modIDs, &memBlockInfo); //0x47BC
    if (error >= 0){
        sceKernelMemset(memBlockInfo->unk40, 0, memBlockInfo->unk44); //0x47F0
        error = sceKernelFreePartitionMemory(modIDs); //0x47F8
    }

    return error;
}

//0x000049BC
//ModuleMgrForKernel_9B7102E2
void sceKernelRebootPhaseForKernel(void arg0, void arg1, void arg2)
{
    SceUID uid;
    SceUID *modUIDS;
    SceModule *mod;
    s32 modCount;
    void *unk;
    s32 error = SCE_ERROR_OK;
    uid = sceKernelGetModuleListWithAlloc(&modCount); //0x49F8
    if (uid >= 0){ //0x4A00
        modUIDS = (SceUID *)sceKernelGetBlockHeadAddr(uid); //0x4A08
        for (int i = 0; i < modCount; i++){
            mod = sceKernelFindModuleByUID(modUIDS[i]); //0x4A34
            if (mod != null){ //0x4A3C
                if (mod->moduleRebootPhase != -1){
                    if (mod->status & 0xF == 0x5 && mod->status & 0x100 == 0){
                        mod->moduleRebootPhase(arg0, arg1, arg2);
                        if (KDebugForKernel_47570AC5() >= 0){ //0x4B14
                            if (KDebugForKernel_86010FCB() == 1)
                                continue;
                            else if (sceKernelGetCompiledSdkVersion() >= 0x03030000)
                                continue;
                            else if (sceKernelSegmentChecksum(mod) == mod->segmentChecksum)
                                continue;
                            else {
                                pspBreak(0);
                                continue;
                            }
                            
                        }
                    }
                }
            }
        }

        unk->unk0 = 56; //size?
        error = sceKernelQueryMemoryBlockInfo(uid, unk);
        if (error >= 0){
            sceKernelMemset(unk->unk40, unk->unk44, 56);
            sceKernelFreePartitionMemory(uid);
            error = SCE_ERROR_OK;
        }



    } else 
        error = (s32)uid;

    return error;

}

//0x00004B6C
//ModuleMgrForKernel_5FC3B3DA
void sceKernelRebootBeforeForKernel(void arg0, void arg1, void arg2, void arg3)
{
    s32 modCount;
    s32 error = SCE_ERROR_OK;
    SceUID uid;
    SceUID *modUIDs;
    SceModule *mod;
    uid = sceKernelGetModuleListWithAlloc(&modCount); //0x4BA8
    if (uid >= 0){ //0x4BB0
        modUIDS = (SceUID *)sceKernelGetBlockHeadAddr(uid); //0x4BB8
        for (int i = 0; i < modCount; i++){
            mod = sceKernelFindModuleByUID(modUIDS[i]);
            if (mod->rebootBefore != -1){ //0x4BF8
                if (mod->status & 0xF == 5 && mod->status & 0x100 == 0)
                    mod->rebootBefore(arg0, arg1, arg2, arg3);
            }
        }
    } else
        error = (s32)uid;
    return error;
}

//0x000050FC
//ModuleMgrForKernel_61E3EC69
void sceKernelLoadModuleBufferForExitGame(SceUID bufid, void arg1, SceKernelLMOption *option)
{
    s32 error = SCE_ERROR_OK;
    SceModuleMgrParam param;
    s32 oldK1 = pspShiftK1();

    if (sceKernelIsIntrContext()) //0x5128
        error = SCE_ERROR_KERNEL_CANNOT_BE_CALLED_FROM_INTERRUPT;
    else if (pspK1IsUserMode()) //0x5138
        error = SCE_ERROR_KERNEL_ILLEGAL_PERMISSION_CALL;
    else if (!pspK1PtrIsOk(arg0)) //0x5148
        error = SCE_ERROR_KERNEL_ILLEGAL_ADDRESS;
    else {
        error = sub_00007620(option); //0x5170
        if (error >= 0){ //0x517C
            initSceModuleMgrParam(param, API_TYPE_EXIT_GAME, bufid, MM_COMMAND_0, MM_COMMAND_1); //0x5190
            error sub_000075B4(param, option); //0x519C
        }
    }

    pspSetK1(oldK1);
    return error;
}

//0x00005378
//ModuleMgrForKernel_2F3F9B6A
void sceKernelLoadModuleBufferForLoadExecBufferVSHUsbWlan(s32 apiType, const char *path, void arg1, SceKernelLMOption *option)
{
    SceModuleMgrParam param;
    s32 error = SCE_ERROR_OK;
    s32 oldK1 = pspShiftK1();

    if (sceKernelIsIntrContext()) //0x53AC
        error = SCE_ERROR_KERNEL_CANNOT_BE_CALLED_FROM_INTERRUPT;
    else if(pspK1IsUserMode())
        error = SCE_ERROR_KERNEL_ILLEGAL_PERMISSION_CALL;
    else if(!pspK1PtrIsOk(path))
        error = SCE_ERROR_KERNEL_ILLEGAL_ADDRESS;
    else if ((error = sub_00007620(option)) >= 0){
        initSceModuleMgrParam(param, apiType, path, 0, 0); //path??? Should be an SceUID???
        error = sub_000075B4(param, option);   
    }

    pspSetK1(oldK1);
    return error;
}

//0x00005434
//ModuleMgrForKernel_C13E2DE5
void sceKernelLoadModuleBufferForLoadExecBufferVSHUsbWlanDebug(s32 apiType, const char *path, void arg1, SceKernelLMOption *option)
{
    SceModuleMgrParam param;
    s32 error = SCE_ERROR_OK;
    s32 oldK1 = pspShiftK1();

    if (sceKernelIsIntrContext()) //0x5468
        error = SCE_ERROR_KERNEL_CANNOT_BE_CALLED_FROM_INTERRUPT;
    else if(pspK1IsUserMode()) //0x5478
        error = SCE_ERROR_KERNEL_ILLEGAL_PERMISSION_CALL;
    else if(!pspK1PtrIsOk(path)) //5488
        error = SCE_ERROR_KERNEL_ILLEGAL_ADDRESS;
    else if ((error = sub_00007620(option)) >= 0){
        initSceModuleMgrParam(param, apiType, path, 0, 0); //path??? Should be an SceUID???
        error = sub_000075B4(param, option);   
    }

    pspSetK1(oldK1);
    return error;
}

//0x000054F0
//ModuleMgrForKernel_C6DE0B9C
void sceKernelLoadModuleBufferVSH(void *arg0, void arg1, void arg2, SceKernelLMOption *option)
{

    //NEEDS VERIFICATION!
    //Is arg0 or arg1 the pointer?
    SceModuleMgrParam param;
    s32 error = SCE_ERROR_OK;
    s32 oldK1 = pspShiftK1();

    if (sceKernelIsIntrContext()) //0x5524
        error = SCE_ERROR_KERNEL_CANNOT_BE_CALLED_FROM_INTERRUPT;
    else if (!pspK1IsUserMode())
        error = SCE_ERROR_KERNEL_ILLEGAL_PERMISSION_CALL; //0x552C
    else if (sceKernelGetUserLevel() != 4)            
        error = SCE_ERROR_KERNEL_ILLEGAL_PERMISSION_CALL; //0x5568
    else if (!pspK1DynBufOk(arg0, arg1)) //0x5588
        error = SCE_ERROR_KERNEL_ILLEGAL_ADDRESSl
    else if ((error = sub_00007620(option)) >= 0){ //0x559C
        initSceModuleMgrParam(param, API_TYPE_VSH_BUFFER, arg1, arg0, 0);
        error = sub_000075B4(param, option);
    }

    pspSetK1(oldK1);
    return error;
}

//0x000055CC
//ModuleMgrForKernel_9236B422
void sceKernelLoadModuleBufferForExitVSHVSH(SceUID *bufid, void *arg1, SceKernelLMOption *option)
{
    SceModuleMgrParam param;
    s32 oldK1 = pspShiftK1();
    s32 error = SCE_ERROR_OK;

    if (sceKernelIsIntrContext())
        error = SCE_ERROR_KERNEL_CANNOT_BE_CALLED_FROM_INTERRUPT;
    else if (pspK1IsUserMode())
        error = SCE_ERROR_KERNEL_ILLEGAL_PERMISSION_CALL;
    else if (!pspK1PtrIsOk(bufid))
        error = SCE_ERROR_KERNEL_ILLEGAL_ADDRESS;
    else {
        error = sub_00007620();
        if (error >= 0){
            initSceModuleMgrParam(param, API_TYPE_EXIT_VSH_VSH, bufid, 0, 1);
            error = sub_000075B4(param, option);
        }
    }

    pspSetK1(oldK1);
    return error;
}

//0x0000567C
//ModuleMgrForKernel_4E62C48A
void sceKernelLoadModuleBuffer(SceUID *bufid, void *arg1, SceKernelLMOption *option)
{
    SceModuleMgrParam param;
    s32 oldK1 = pspShiftK1();
    s32 error = SCE_ERROR_OK;

    if (sceKernelIsIntrContext())
        error = SCE_ERROR_KERNEL_CANNOT_BE_CALLED_FROM_INTERRUPT;
    else if (pspK1IsUserMode())
        error = SCE_ERROR_KERNEL_ILLEGAL_PERMISSION_CALL;
    else if (!pspK1PtrIsOk(bufid))
        error = SCE_ERROR_KERNEL_ILLEGAL_ADDRESS;
    else {
        error = sub_00007620();
        if (error >= 0){
            initSceModuleMgrParam(param, API_TYPE_2, bufid, 0, 1);
            error = sub_000075B4(param, option);
        }
    }

    pspSetK1(oldK1);
    return error;

}

//0x00005744
//ModuleMgrForKernel_253AA17C
void sceKernelLoadModuleBufferForExitVSHKernel(SceUID *bufid, void *arg1, SceKernelLMOption *option)
{
    SceModuleMgrParam param;
    s32 oldK1 = pspShiftK1();
    s32 error = SCE_ERROR_OK;

    if (sceKernelIsIntrContext())
        error = SCE_ERROR_KERNEL_CANNOT_BE_CALLED_FROM_INTERRUPT;
    else if (pspK1IsUserMode())
        error = SCE_ERROR_KERNEL_ILLEGAL_PERMISSION_CALL;
    else if (!pspK1PtrIsOk(bufid))
        error = SCE_ERROR_KERNEL_ILLEGAL_ADDRESS;
    else {
        error = sub_00007620();
        if (error >= 0){
            initSceModuleMgrParam(param, API_TYPE_EXIT_VSH_KERNEL, bufid, 0, 1);
            error = sub_000075B4(param, option);
        }
    }

    pspSetK1(oldK1);
    return error;

}

//0x000057F4
//ModuleMgrForKernel_4E38EA1D
void sceKernelLoadModuleBufferForRebootKernel(SceUID *bufid, void *arg1, SceKernelLMOption *option)
{
    SceModuleMgrParam param;
    s32 oldK1 = pspShiftK1();
    s32 error = SCE_ERROR_OK;

    if (sceKernelIsIntrContext())
        error = SCE_ERROR_KERNEL_CANNOT_BE_CALLED_FROM_INTERRUPT;
    else if (pspK1IsUserMode())
        error = SCE_ERROR_KERNEL_ILLEGAL_PERMISSION_CALL;
    else if (!pspK1PtrIsOk(bufid))
        error = SCE_ERROR_KERNEL_ILLEGAL_ADDRESS;
    else {
        error = sub_00007620();
        if (error >= 0){
            initSceModuleMgrParam(param, 0x300, bufid, 0, 1);
            error = sub_000075B4(param, option);
        }
    }

    pspSetK1(oldK1);
    return error;

}

//0x000058A4
void ModuleMgrForKernel_955D6CB2()
{
    return SCE_ERROR_KERNEL_ILLEGAL_PERMISSION_CALL;
}

//0x000058B0
void ModuleMgrForKernel_1CF0B794(SceUID *bufid, void *arg1, SceKernelLMOption *option)
{
    SceModuleMgrParam param;
    s32 oldK1 = pspShiftK1();
    s32 error = SCE_ERROR_OK;

    if (sceKernelIsIntrContext())
        error = SCE_ERROR_KERNEL_CANNOT_BE_CALLED_FROM_INTERRUPT;
    else if (pspK1IsUserMode())
        error = SCE_ERROR_KERNEL_ILLEGAL_PERMISSION_CALL;
    else if (!pspK1PtrIsOk(bufid))
        error = SCE_ERROR_KERNEL_ILLEGAL_ADDRESS;
    else {
        error = sub_00007620(option);
        if (error >= 0){
            initSceModuleMgrParam(param, API_TYPE_81, bufid, 0, 1);
            error = sub_000075B4(param, option);
        }
    }

    pspSetK1(oldK1);
    return error;  
}

//0x00005978
//ModuleMgrForKernel_5FC32087
void sceKernelLoadModuleByIDBootInitConfig()
{
    return SCE_ERROR_KERNEL_ILLEGAL_PERMISSION_CALL;
}

//0x00005984
//void ModuleMgrForKernel_E8B9D19D
void sceKernelLoadModuleBufferBootInitConfig()
{
    return SCE_ERROR_KERNEL_ILLEGAL_PERMISSION_CALL;
}

//0x00005A14
//ModuleMgrForKernel_EE6E8F49
s32 sceKernelStopUnloadSelfModuleWithStatusKernel(void *arg0, void *arg1, void *arg2, void *arg3, void *arg4)
{
    return sub_000077F0(arg0, pspGetRa(), arg1, arg2, arg3, arg4);
}

//0x00005A80
//ModuleMgrForKernel_D86DD11B
SceUID sceKernelSearchModuleByName(const char *name)
{
    s32 k1 = pspShiftK1();
    s32 error;
    if (!pspK1PtrIsOk(name)) //0x5AA0
        error = SCE_ERROR_KERNEL_ILLEGAL_ADDRESS;
    else {
        SceModule *mod = sceKernelFindModuleByName(name);
        if (mod){
            pspSetK1(k1);
            return mod->modId;
        }
        error = SCE_ERROR_KERNEL_UNKNOWN_MODULE;
    }

    pspSetK1(k1);
    return error;
}

//0x00005AE0
//ModuleMgrForKernel_12F99392
SceUID sceKernelSearchModuleByAddress(s32 address)
{
    SceModule *mod = sceKernelFindModuleByAddress(address);
    if (mod)
        return mod->modId;
    return SCE_ERROR_KERNEL_UNKNOWN_MODULE;
}

//0x00005B6C
s32 ModuleMgrForKernel_A40EC254(void *func)
{
    g_ModuleMgrUIDS->unkFunc = func;
    return SCE_ERROR_OK;
}

//x00005B7C
s32 ModuleMgrForKernel_C3DDABEF(void *arg0, void *arg1)
{
    s32 error = SCE_ERROR_OK;

    if (arg1 == null || arg2 = null) //0x5B88
        return SCE_ERROR_KERNEL_ILLEGAL_ADDR;
    if (g_ModuleMgrUIDS->unkFunc == null)
        return SCE_ERROR_KERNEL_ERROR;

    error = g_ModuleMgrUIDS->unkFunc(arg0, arg1); //Args?
    if (error < 0)
        return error;
    return SCE_ERROR_OK; //g_9A40 may return > 0, cannot return error variable
}

//0x00005BD0
//ModuleMgrForKernel_1CFFC5DE
s32 sceKernelModuleMgrMode()
{
    return 0;
}
