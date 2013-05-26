#include "../common/common_header.h"

//0x000006B8
//ModuleMgrForUser_977DE386
SceUID sceKernelLoadModule(const char *path, int flags, SceKernelLMOption *option)
{
    SceModuleMgrParam param; //sizeof == 160
    int res;
    int k1 = pspShiftK1();
    s32 error = SCE_ERROR_OK;


    if (sceKernelIsIntrContext()) //0x6EC
        error = SCE_ERROR_KERNEL_CANNOT_BE_CALLED_FROM_INTERRUPT;
    else if (k1 >= 0) //0x6F8
        error = SCE_ERROR_KERNEL_ILLEGAL_PERMISSION_CALL;
    else if (path == null) //0x704
        error = SCE_ERROR_KERNEL_ILLEGAL_ADDR;
    //Disallow kernel address space pointers in syscalls
    else if (path & k1 < 0) //0x718
        error = SCE_ERROR_KERNEL_ILLEGAL_ADDR;

    else if (SysclibForKernel_B1DC2AE8(path) != 0) //0x81C
        error = SCE_ERROR_KERNEL_UNKNOWN_MODULE_FILE;

    else if (option != null){ //0x82C
        if (!pspK1StaBufOk(option, (sizeof(option)))) //0x840
            error = SCE_ERROR_KERNEL_ILLEGAL_ADDR;
        else {
            res = sceKernelGetCompiledSdkVersion(); //0x850
            res &= ~0x1FFFF; //0x85C, check this! 
            if (res < 0x2080000 && option->flags != 20) //0x87C, check this!
                error = SCE_ERROR_KERNEL_ILLEGAL_SIZE;
        }
    }

    if (error < 0){
        pspSetK1(k1);
        return error;
    }

    SceUID uid = sceIoOpen(path, 0x401, 0777); //0x734
    if (uid < 0){ //0x740
        pspSetK1(k1);
        return uid;
    }

    if (sceIoIoctl(uid, 0x80010020, null, 0, null, 0) < 0){ //0x760
        pspSetK1(k1);
        return SCE_ERROR_KERNEL_PROHIBIT_LOADMODULE_DEVICE;
    }

    //null out param 0x780-0x788

    param.unk1 = 1; //0x78C
    param.apiType = 16; //0x79C
    param.unk0 = 0; //0x7A8
    param.unk64 = 0; //0x7B4
    param.unk24 = uid; //0x7BC
    param.unk124 = 0; //0x7C4


    res = sceIoIoctl(uid, 0x00208081, null, 0, null, 0); //0x7C0
    if (res >= 0)
        param.unk100 = 16; //0x7CC

    res = sub_000075B4(param, option); //0x7D4
    sceIoClose(uid); //0x7E0
    pspSetK1(k1);
    return res;
}


//0x0000088C
//ModuleMgrForUser_B7F46618
SceUID sceKernelLoadModuleByID(SceUID fid, int flags, SceKernelLMOption *option)
{
    int res;
    int k1;
    SceModuleMgrParam param;
    k1 = pspShiftK1();

    if (sceKernelIsIntrContext()) //0x8C0
        return SCE_ERROR_KERNEL_CANNOT_BE_CALLED_FROM_INTERRUPT;
    if (k1 >= 0) //0x8CC
        return SCE_ERROR_KERNEL_ILLEGAL_PERMISSION_CALL;
    if (option != null){ //0x8D4
        if (!pspK1StaBufOk(option, (sizeof(option)))) //0x8E8
            return SCE_ERROR_KERNEL_ILLEGAL_ADDR;

        res = sceKernelGetCompiledSdkVersion(); //0x918
        res &= ~0x1FFFF; //0x924, check this! 
        if (res < 0x2080000 && option->flags != 20) //0x944, check this!
            return SCE_ERROR_KERNEL_ILLEGAL_SIZE;
    }

    res = sceIoValidateFd(fid, 4); //0x950
    if (res < 0) //0x958
        return res;

    if (sceIoIoctl(fid, 0x00208001, null, 0, null, 0) < 0) //0x978
        return SCE_ERROR_KERNEL_PROHIBIT_LOADMODULE_DEVICE;

    //null out param 0x998-0x7A0

    param.unk1 = 1; //0x9C4
    param.apiType = 16; //0x9C8
    param.unk0 = 0; //0x9CC
    param.unk64 = 0; //0x9D0
    param.unk24 = fid; //0x9D4
    param.unk124 = 0; //0x9DC


    res = sceIoIoctl(uid, 0x00208081, null, 0, null, 0); //0x9D8
    if (res >= 0)
        param.unk100 = 16; //0x9E0

    res = sub_000075B4(param, option); //0x7D4
    pspSetK1(k1);
    return res;
}

//0x000009FC
void ModuleMgrForUser_E4C4211C(const u8 *path, SceUID blockId, void arg2, s32 arg3)
{
    SceModuleMgrParam param;
    SceUID *fid;
    SceKernelSysmemBlockInfo *blockInfo;
    s32 error = SCE_ERROR_OK;
    s32 oldK1 = pspShiftK1();

    blockInfo->size = 56;

    if (sceKernelIsIntrContext()) //0xA44
        error = SCE_ERROR_KERNEL_CANNOT_BE_CALLED_FROM_INTERRUPT;
    else if (!pspK1IsUserMode()) //0xA50
        error = SCE_ERROR_KERNEL_ILLEGAL_PERMISSION_CALL;   
    else if (path == null) //0xA5C
        error = SCE_ERROR_KERNEL_ILLEGAL_ADDRESS;
    else if (!pspK1Ptrok(path)) //0xA6C
        error = SCE_ERROR_KERNEL_ILLEGAL_ADDRESS;
    else if (strchr(path, '%') != 0) //0xAB8
        error = SCE_ERROR_KERNEL_UNKNOWN_MODULE_FILE;
    else {
        error = SceKernelSysmemBlockInfo(blockId, blockInfo); //0xACC
        if (error >= 0){ //0xAD4
            if (!pspK1DynBufOk(blockInfo->unk44, blockInfo->unk40)) //0xB04
                error = SCE_ERROR_KERNEL_ILLEGAL_ADDRESS;
            else if (arg3 > 0) //0xB14
                error = SCE_ERROR_KERNEL_INVALID_ARGUMENT;
            else if (arg3 == 0 && blockInfo->unk44 < arg2) //0xB1C
                error = SCE_ERROR_KERNEL_INVALID_ARGUMENT;
            else if (arg2 & 0 | arg3 & 0x3F == 0) //0xBC3
                error = SCE_ERROR_KERNEL_INVALID_ARGUMENT;
            else {
                fid = sceIoOpen(path, 0x04000001, 0777); //0xB5C
                if (fid < 0) //0xB68
                    error = (s32)fid;
                else {
                    error = sceIoIoctl(fid, 0x00208001, null, 0, null, 0); //0xB88
                    if (error < 0) //0xB94
                        error = SCE_ERROR_KERNEL_PROHIBIT_LOADMODULE_DEVICE;
                    else {
                        for (int i = 0; i < 192/4; i++)
                            ((s32 *)param)[i] = 0;
                        param.endCMD = MM_COMMAND_1; //0xBB8
                        param.apiType = 16; //0xBC4
                        param.startCMD = MM_COMMAND_0; //0xBD0
                        param.fileBase = null; //0xBDC
                        param.fid = fid; //0xBE4
                        param.unk124 = 0; //0xBEC
                        if (sceIoIoctl(fid, 0x00208081, null, 0, null, 0) >= 0)
                            param.unk100 = 16;//0xBF4
                        param.unk144 = blockId; //0xBFC
                        param.unk152 = arg2; //0xC00
                        param.unk156 = arg3; //0xC04
                        error = sub_000075B4(param, null); //0xC08
                    }

                    sceIoClose(fid); //0xC14
                }
            }
        }
    }

    pspSetK1(oldK1);
    return error;
}   

//0x00000C34
void ModuleMgrForUser_FBE27467(SceUID fid, s32 arg1, s32 arg2, s32 arg3)
{
    SceModuleMgrParam param;
    void *unk;
    unk->unk0 = 56; //0xCCC
    s32 error = SCE_ERROR_OK;
    s32 oldK1 = pspShiftK1();

    if (sceKernelIsIntrContext()) //0xC78
        error = SCE_ERROR_KERNEL_CANNOT_BE_CALLED_FROM_INTERRUPT;
    else if (pspK1IsUserMode()) //0xC84
        error = SCE_ERROR_KERNEL_ILLEGAL_PERMISSION_CALL;
    else if (sceKernelQueryMemoryBlockInfo(arg1, unk) >= 0) //0xCE4
        && !pspK1DynBufOk(unk->unk40, unk->unk44)) //0xD00
                error = SCE_ERROR_KERNEL_ILLEGAL_SIZE;
    else if (arg3 >= 0) //0xD10
        error = SCE_ERROR_KERNEL_INVALID_ARGUMENT;
    else if (arg2 & 0x3F | arg3 & 0)
        error = SCE_ERROR_KERNEL_INVALID_ARGUMENT;
    else {
        error = sceIoValidateFd(fid,4); //0xD50
        if (!error){
            if (sceIoIoctl(fid, 0x00208001, null, 0, null, 0) < 0)
                error = SCE_ERROR_KERNEL_PROHIBIT_LOADMODULE_DEVICE;
            if (!error){
                for (int i = 0; i < sizeof(SceModuleMgrParam)/4; i++)
                    ((s32*)param)[i] = 0;
                param.unk156 = 0; //0xDA0
                param.unk1 = 1; //0xDA8
                param.apiType = 16; //0xDB4
                param.unk0 = 0; //0xDC0
                param.unk64 = 0; //0xDCC
                param.unk24 = fid; //0xDD4
                param.unk124 = 0; //0xDDC
                if (sceIoIoctl(fid, 0x00208081, null, 0, null, 0) >= 0)
                    param.unk100 = 16;

                param.unk144 = arg1;
                param.unk152 = arg2;
                param.unk156 = arg3;

                error = sub_000075B4(param, null);
            }
        }
    }

    pspSetK1(oldK1);
    return error;
}

//0x00000E18
void ModuleMgrForUser_FEF27DC1(const char *path, u8 buffer[16], void arg1, SceKernelLMOption *option)
{
    SceUID fid;
    SceModuleMgrParam param;
    s32 error = SCE_ERROR_OK;
    s32 oldK1 = pspShiftK1();

    if (pspKernelIsIntrContext()) //0xE54
        error = SCE_ERROR_KERNEL_CANNOT_BE_CALLED_FROM_INTERRUPT;
    else if (!pspK1IsUserMode()) //0xE60
        error = SCE_ERROR_KERNEL_ILLEGAL_PERMISSION_CALL;
    else if (buffer == null) //0xE6C
        error = SCE_ERROR_KERNEL_ILLEGAL_ADDRESS;
    else if (!pspK1StaBufOk()) //0xE84
        error = SCE_ERROR_KERNEL_ILLEGAL_ADDRESS;
    else if (path == null) //0xE8C
        error = SCE_ERROR_KERNEL_ILLEGAL_ADDRESS;
    else if (!strchr(path, '%') == 0) //0xFF4
        error = SCE_ERROR_KERNEL_UNKNOWN_MODULE_FILE;
    else if (option != null){ //0x1004
        if (!pspK1StaBufOk(option, 20)) //0x1018
            error = SCE_ERROR_KERNEL_ILLEGAL_ADDRESS;
        else if (sceKernelGetCompiledSdkVersion() >= 0x02080000 && option->size != 20)
            error = SCE_ERROR_KERNEL_ILLEGAL_SIZE;
    }

    if (error == SCE_ERROR_OK){
        fid = sceIoOpen(fid, 0x44000001, 0777); //0xEB8
        if (fid < 0) //0xEC4
            error = fid;
        else {
            error = sceIoIoctl(fid, 0x04100001, buffer, 16, null, 0); //0xEE4
            if (error >= 0){
                if (sceIoIoctl(fid, 0x00208002, null, 0, null, 0) < 0) //0xF20
                    error = SCE_ERROR_KERNEL_PROHIBIT_LOADMODULE_DEVICE;
                else {
                    for (int i = 0; i < 160/4; i++)
                        ((s32 *)param)[i] = 0;
                    param.apiType = API_TYPE_19; //0xF44
                    param.endCMD = MM_COMMAND_1; //0xF50
                    param.startCMD = MM_COMMAND_0; //0xF5C
                    param.fileBase = null; //0xF68
                    param.fid = fid; //0xF70
                    param.unk124 = 0; //0xF7C
                    if (sceIoIoctl(fid, 0x00208001, null, 0, null, 0) >= 0)
                        param.unk100 = 16;
                    memcpy(&param.unk128, buffer, 16); //0xF90
                    error = sub_000075B4(param, option);
                }
            }
            sceIoClose(fid);
        }
    }

    pspSetK1(oldK1);
    return error;
}

//0x00001060
void ModuleMgrForUser_F2D8D1B4(const u8 *path, void arg1, SceKernelLMOption *option)
{
    SceUID fid;
    s8 buffer[16];
    SceOffset fileOffset;
    SceModuleMgrParam param;
    s32 error = SCE_ERROR_OK;
    s32 oldK1 = pspShiftK1();

    if (sceKernelIsIntrContext()) //0x1094
        error = SCE_ERROR_KERNEL_CANNOT_BE_CALLED_FROM_INTERRUPT;
    else if (!pspK1IsUserMode()) //0x10A0
        error = SCE_ERROR_KERNEL_ILLEGAL_PERMISSION_CALL;
    else if (path == null) //0x10AC
        error = SCE_ERROR_KERNEL_ILLEGAL_ADDRESS;
    else if (!pspK1PtrOk(path)) //0x10BC
        error = SCE_ERROR_KERNEL_ILLEGAL_ADDRESS;
    else if (strchr(path, '%') != 0) //0x1214
        error = SCE_ERROR_KERNEL_UNKNOWN_MODULE_FILE;
    else if (option != null) { //0x122C
        if (!pspK1StaBufOk(option, 20)) //0x1240
            error = SCE_ERROR_KERNEL_ILLEGAL_ADDRESS;
        else if (sceKernelGetCompiledSdkVersion() >= 0x02080000 && option->size != 20)
            error = SCE_ERROR_KERNEL_ILLEGAL_SIZE;
    }

    if (error == SCE_ERROR_OK){
        fid = sceIoOpen(path, 0x04000001, 0777); //0x10DC
        if (fid < 0) //0x10E8
            error = fid;
        else {
            if (g_ModuleMgrUIDs.unkFunc == null) //0x10FC
                error = SCE_ERROR_KERNEL_ERROR;
            else {
                error = g_ModuleMgrUIDs.unkFunc(fid, buffer, &fileOffset)
                if (error >= 0){ //0x1118
                    sceIoLseek(fid, fileOffset, SEEK_SET); //0x112C
                    if (sceIoIoctl(fid, 0x00208002, null, 0, null, 0) < 0)
                        error = SCE_ERROR_KERNEL_PROHIBIT_LOADMODULE_DEVICE;
                    else {
                        for (int i = 0; i < 160/4; i++)
                            ((s32 *)param)[i] = 0;
                        param.apiType = API_TYPE_VSH; //0x117C
                        param.endCMD = MM_COMMAND_1; //0x1188
                        param.startCMD = MM_COMMAND_0; //0x1194
                        param.fileBase = null; //0x11A0
                        param.fid = fid; //0x11A8
                        param.unk124 = 0; //0x11B8
                        if (sceIoIoctl(fid, 0x00208081, null, 0, null, 0) >= 0) //0x11B4
                            param.unk100 =16; //0x11BC
                        memcpy(&param.unk128, buffer, 16); //0x11C8
                        error = sub_000075B4(param, option);
                    }
                }
            }
            sceIoClose(fid);
        }
    }

    pspSetK1(oldK1);
    return error;
}

//0x0000128C
//ModuleMgrForUser_710F61B5
SceUID sceKernelLoadModuleMs(const char *path, int flags, SceKernelLMOption *option)
{
    SceUID fid;
    SceModuleMgrParam param;
    s32 error = SCE_ERROR_OK;
    s32 oldK1 = pspShiftK1();

    if (sceKernelIsIntrContext()) //0x12BC
        error = SCE_ERROR_KERNEL_CANNOT_BE_CALLED_FROM_INTERRUPT;
    else if (!pspK1IsUserMode()) //0x12C4
        error = SCE_ERROR_KERNEL_ILLEGAL_PERMISSION_CALL;
    else if (sceKernelGetUserLevel() != 1) //0x1300
        error = SCE_ERROR_KERNEL_ILLEGAL_PERMISSION_CALL;
    else if (path == null) //0x130C
        error = SCE_ERROR_KERNEL_ILLEGAL_ADDRESS;
    else if (!pspK1Ptrok(path)) //0x131C
        error = SCE_ERROR_KERNEL_ILLEGAL_ADDRESS;
    else if (strchr(path, '%') != 0) //0x1408
        error = SCE_ERROR_KERNEL_UNKNOWN_MODULE_FILE;
    else if (option != null){ //0x1418
        if (!pspK1StaBufOk(option, 20)) //0x142C
            error = SCE_ERROR_KERNEL_ILLEGAL_ADDRESS;
        else if (sceKernelGetCompiledSdkVersion() >= 0x02080000 && option->size != 20)
            error = SCE_ERROR_KERNEL_ILLEGAL_SIZE;
    }

    if (error == SCE_ERROR_OK){
        fid = sceIoOpen(path, 0x04000001, 0777); //0x133C
        if (fid < 0)
            error = fid;
        else {
            if (sceIoIoctl(fid, 0x00208002, null, 0, null, 0) < 0) //0x1374
                error = SCE_ERROR_KERNEL_PROHIBIT_LOADMODULE_DEVICE;
            else {
                for (int i =0; i < 160/4; i++)
                    ((s32 *)param)[i] = 0;
                param.apiType = API_TYPE_MS; //0x1398
                param.endCMD = MM_COMMAND_1; //0x13A4
                param.startCMD = MM_COMMAND_0; //0x13B0
                param.fileBase = null; //0x13BC
                param.fid = fid; //0x13C4
                param.unk124 = 0; //0x13CC
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

//0x00001478
//ModuleMgrForUser_F9275D98()
SceUID sceKernelLoadModuleBufferUsbWlan(SceSize bufsize, void *modBuf, int flags, SceKernelLMOption *option)
{
    SceUID fid;
    SceModuleMgrParam param;
    s32 error = SCE_ERROR_OK;
    s32 oldK1 = pspShiftK1();

    if (sceKernelIsIntrContext()) //0x14B0
        error = SCE_ERROR_KERNEL_CANNOT_BE_CALLED_FROM_INTERRUPT;
    else if (!pspK1IsUserMode()) //0x14B8
        error = SCE_ERROR_KERNEL_ILLEGAL_PERMISSION_CALL;
    else if (sceKernelGetUserLevel() != 1 && sceKernelGetUserLevel() != 2)
        error = SCE_ILLE+SCE_ERROR_KERNEL_ILLEGAL_PERMISSION_CALL;
    else if (!pspK1DynBufOk(buf, bufsize)) //0x1528
        error = SCE_ERROR_KERNEL_ILLEGAL_ADDRESS;
    else if (option != null){ //0x1530
        if (!pspK1StaBufOk(option, 20))
            error = SCE_ERROR_KERNEL_ILLEGAL_ADDRESS;
        else if (sceKernelGetCompiledSdkVersion() >= 0x02080000 && option->size != 20)
            error = SCE_ERROR_KERNEL_ILLEGAL_SIZE;
    }

    if (error == SCE_ERROR_OK){
         //0x1558
        if (sub_00008568(0x30, modBuf, &fid) != 0){ //0x1564
            if (fid < 0) //0x1570
                for(;;); //0x1600
            for (int i = 0; i < 160/4; i++)
                ((s32 *)param)[i] = 0;
            param.apiType = API_TYPE_BUFFER_USB_WLAN; //01594
            param.endCMD = MM_COMMAND_1; //0x15A0
            param.startCMD = MM_COMMAND_0; //0x15AC
            param.fileBase = null; //0x15B8
            param.fid = fid; //0x15C0
            param.unk124 = 0; //0x15C8
            if (sceIoIoctl(fid, 0x00208081, null, 0, null, 0) >= 0)
                param.unk100 = 16;
            param.unk124 = 1; //0x15EC
            error = sub_000075B4(param, option); //0x15E4
            sceIoClose(fid);

        } else { //0x1608
            for (int i = 0; i < 160/4; i++)
                ((s32 *)param)[i] = 0;
            param.apiType = API_TYPE_BUFFER_USB_WLAN; //0x161C
            param.endCMD = MM_COMMAND_1; //0x1628
            param.modBufSize = bufSize; //0x162C
            param.fid = modBuf; //01630
            param.unk124 = 0; //0x1634
            param.startCMD = MM_COMMAND_0; //0x1638
            param.fileBase = modBuf;
            error = sub_000075B4(param, option)
        }
    }

    pspSetK1(oldK1);
    return error;

}

//0x00003D98
//ModuleMgrForUser_50F0C1EC
s32 sceKernelStartModule(SceUID modid, SceSize argsize, void *argp, int *status, SceKernelSMOption *option)
{
    SceModuleMgrParam param;
    s32 oldK1 = pspShiftK1();
    s32 error = SCE_ERROR_OK;

    if (pspKernelIsIntrContext()) //0x3DDC
        error = SCE_ERROR_KERNEL_CANNOT_BE_CALLED_FROM_INTERRUPT;
    else if (argp != null && !pspK1DynBufOk(argp, argsize))
        error = SCE_ERROR_KERNEL_ILLEGAL_ADDRESS;
    else if (status != null && !pspK1Ptrok(status))
        error = SCE_ERROR_KERNEL_ILLEGAL_ADDRESS;
    else if (option != null){
        if (!pspK1StaBufOk(option, 20))
            error = SCE_ERROR_KERNEL_ILLEGAL_ADDRESS;
        else if (sceKernelGetCompiledSdkVersion() >= 0x02080000 && option->size != 20)
            error = SCE_ERROR_KERNEL_ILLEGAL_SIZE;
        else if (((s32 *)option)[4] & 0xFF0F9FFF != 0)
            error = SCE_ERROR_KERNEL_ERROR;
    }

    if (error == SCE_ERROR_OK){
        for (int i = 0; i < 160/4; i ++)
            ((s32 *)param)[i] = 0;
        param.endCMD = MM_COMMAND_MODULE_START; //0x3EC8
        param.modid = modid; //0x3ECC
        param.args = argsize; //0x3ED0
        param.argp = argp; //0x3ED4
        param.status = status; //0x3ED8
        param.startCMD = MM_COMMAND_MODULE_START; //0x3
        if (option != null){
            param.mpidtext = option->mpidtext; //0x3EF4
            param.mpiddata = option->mpiddata; //0x3EF8
            param.flags = option->flags; //0x3EFC
            ((s32 *)param)[8] = ((s32 *)option)[4]; //0x3F00 
        } else {
            param.mpidtext = 0; //0x3F14
            param.mpiddata = 0; //0x3F18
            param.flags = 0; //0x3F1C
            ((s32 *)param)[8] = 0; //0x3F24
        }
        error = sub_000074E4(param);
    }

    pspSetK1(oldK1);
    return error;
}

//0x00003F28
//ModuleMgrForUser_D1FF982A
s32 sceKernelStopModule(SceUID modid, SceSize argsize, void *argp, int *status, SceKernelSMOption *option)
{
    SceModule *sourceMod;
    SceModule *targetMod;
    SceModuleMgrParam param;
    s32 oldK1 = pspShiftK1();
    s32 error = SCE_ERROR_OK;

    if (pspKernelIsIntrContext()) //0x3F74
        error = SCE_ERROR_KERNEL_CANNOT_BE_CALLED_FROM_INTERRUPT;
    else if (argp != null && !pspK1DynBufOk(argp, argsize))
        error = SCE_ERROR_KERNEL_ILLEGAL_ADDRESS;
    else if (status != null && !pspK1Ptrok(status))
        error = SCE_ERROR_KERNEL_ILLEGAL_ADDRESS;
    else if (option != null){
        if (!pspK1StaBufOk(option, 20))
            error = SCE_ERROR_KERNEL_ILLEGAL_ADDRESS;
        else if (sceKernelGetCompiledSdkVersion() >= 0x02080000 && option->size != 20)
            error = SCE_ERROR_KERNEL_ILLEGAL_SIZE;
        else if (((s32 *)option)[4] & 0xFF0F9FFF != 0)
            error = SCE_ERROR_KERNEL_ERROR;
    }

    if (error == SCE_ERROR_OK){
        sourceMod = sceKernelFindModuleByAddress(pspGetRa()); //0x4054
        if (sourceMod == null)
            error = SCE_ERROR_KERNEL_MODULE_CANNOT_STOP;
        else {
            for (int i = 0; i < 160/4; i ++)
                ((s32 *)param)[i] = 0;

            targetMod = sceKernelGetModuleFromUID(modid); //0x4080
            param.mod = targetMod; //0x408C
            if (targetMod != null && targetMod->attribute & 0x1 != 0) //0x4088
                error = SCE_ERROR_KERNEL_MODULE_CANNOT_STOP;
            else {
                param.endCMD = MM_COMMAND_MODULE_STOP; //0x40A8
                param.modid = modid; //0x40AC
                param.startCMD = MM_COMMAND_MODULE_STOP; //0x40B0
                param.argp = argp; //0x40B8
                param.memid = sourceMod->memid; //0x40BC
                param.args = argsize; //0x40C0
                param.status = status; //0x40C4

                if (option != null){
                    param.mpidtext = option->mpidtext; //0x40DC
                    param.mpiddata = option->mpiddata; //0x40E0
                    param.flags = option->flags; //0x40E4
                    ((s32 *)param)[8] = ((s32 *)option)[4]; //0x40E8
                } else {
                    param.mpidtext = 0; //0x40FC
                    param.mpiddata = 0; //0x4a00
                    param.flags = 0; //0x4104
                    ((s32 *)param)[8] = 0; //0x410C
                }
                error = sub_000074E4(param);
            }
        }
    }

    pspSetK1(oldK1);
    return error;       
}

//0x00004110
//ModuleMgrForUser_D675EBB8
void sceKernelSelfStopUnloadModule(void arg0, void arg1, void arg2)
{
    s32 oldK1 = pspShiftK1();
    s32 error = SCE_ERROR_OK;
    if (pspKernelIsIntrContext()) //0x414C
        error = SCE_ERROR_KERNEL_CANNOT_BE_CALLED_FROM_INTERRUPT;
    else if (arg0 != null && pspK1DynBufOk(arg0, arg1)) //0x416C
        error = SCE_ERROR_KERNEL_ILLEGAL_ADDRESS;
    
    if (error >=0){}
        if (pspK1IsUserMode()) //0x4174
            error = InterruptManagerForKernel_A0F88036();
        if (!pspK1Ptrok(pspGetRa()))
            error = SCE_ERROR_KERNEL_ILLEGAL_ADDRESS;
        else {
            error = sub_000076CC(arg0, pspGetRA(), arg1, arg2, 0, 0);
        }
    }

    pspSetK1(oldK1);
    return error;


}

//0x000041E8
//ModuleMgrForUser_644395E2
int sceKernelGetModuleIdList(SceUID *readbuf, s32 readbufsize, int *idcount)
{
    s32 oldK1 = pspShiftK1();
    if (readbuf == null) //0x4200
        error = SCE_ERROR_KERNEL_ILLEGAL_ADDRESS;
    else if (readbufsize == 0) //0x4220
        error = SCE_ERROR_KERNEL_ILLEGAL_ADDRESS;
    else if (!pspK1DynBufOk(readbuf, readbufsize)) //0x4238
        error = SCE_ERROR_KERNEL_ILLEGAL_ADDRESS;
    else if (!pspK1DynBufOk(idcount, 4)) //0x4244
        error = SCE_ERROR_KERNEL_ILLEGAL_ADDRESS;
    else 
        error = sceKernelGetModuleIdListForKernel(readbuf, readbufsize, idcount);

    pspSetK1(oldK1);
    return error;

}

//0x00004270
//ModuleMgrForUser_748CBED9
s32 sceKernelQueryModuleInfo(SceUID modid, SceKernelModuleInfo *info)
{
    s32 oldK1 = pspShiftK1();
    s32 error = SCE_ERROR_OK;
    s32 intrState;
    SceModule *mod;

    if (sceKernelIsIntrContext()) //0x42A4
        error = SCE_ERROR_KERNEL_CANNOT_BE_CALLED_FROM_INTERRUPT;
    else if (info == null) //0x42AC
        error = SCE_ERROR_KERNEL_ILLEGAL_ADDRESS;
    else if (!pspK1StaBufOk(info, 96)) //0x42BC
        error = SCE_ERROR_KERNEL_ILLEGAL_ADDRESS;
    else if (sceKernelGetCompiledSdkVersion() >= 0x02080000
        && ((info->size ^ 0x60 >= 0) && (info->size ^ 0x40 >= 0)))
        error = SCE_ERROR_KERNEL_ILLEGAL_SIZE;
    else {
        intrState = sceKernelLoadCoreLock(); //0x4334
        mod = sceKernelFindModuleByUID(modid);
        if (mod == null)
            error = SCE_ERROR_KERNEL_UNKNOWN_MODULE;
        else {
            if (pspK1IsUserMode()){ //0x4350
                if (mod->status & 0x100 == 0) //0x4360
                    error = SCE_ERROR_KERNEL_CANNOT_GET_MODULE_INFO;
                if (!((sceKernelGetUserLevel() == 2 && mod->attribute & 0x1E00 == 0x400) ||
                      (sceKernelGetUserLevel() == 1 && mod->attribute & 0x1E00 == 0x200) ||
                      (sceKernelGetUserLevel() == 3 && mod->attribute & 0x1E00 == 0x600) ||
                      (mod->attribute & 0x1E00 == 0)))
                            error = SCE_ERROR_KERNEL_CANNOT_GET_MODULE_INFO;
            }

            if (error == SCE_ERROR_OK){
                if (mod->status & 0xF >= 8) //0x43C0
                    error = SCE_ERROR_KERNEL_CANNOT_GET_MODULE_INFO;
                else {
                    info->nSegments = mod->nSegments; //0x43CC
                    for (int i = 0; i < SCE_KERNEL_MAX_MODULE_SEGMENTS; i++){
                        info->segmentAddr[i] = mod->segmentAddr[i];
                        info->segmentSize[i] = mod->segmentSize[i];
                    }
                    info->entryAddr = mod->entryAddr; //0x4404
                    info->gpValue = mod->gpValue; //0x440C
                    info->textAddr = mod->textAddr; //0x4414
                    info->textSize = mod->textSize; //0x401C
                    info->dataSize = mod->dataSize; //0x4024
                    info->bssSize = mod->bssSize; //0x4430
                    if (info->size == 96){ //0x442C
                        info->attribute = mod->attribute & 0x0F; //0x4454
                        info->version[0]= mod->version[0]; //0x4460
                        info->version[1] = mod->version[1]; //0x446C
                        strncpy(info.modName, mod.modName, SCE_MODULE_NAME_LEN); //0x4468
                        info->terminal = mod->terminal; //0x4478
                    }
                }
            }
        }

        sceKernelLoadCoreUnlock(intrState);
    }

    pspSetK1(oldK1);
    return error;
}

//0x000044EC
//ModuleMgrForUser_F0A26395
SceUID sceKernelGetModuleId()
{
    s32 ra;
    SceModule *mod;
    s32 intrState;
    s32 error = SCE_ERROR_OK;
    s32 oldK1 = pspShiftK1();

    ra = pspK1IsUserMode()? sceKernelGetSyscallRA():pspGetRa();
    if (sceKernelIsIntrContext()) //0x450C
        error = SCE_ERROR_KERNEL_CANNOT_BE_CALLED_FROM_INTERRUPT;
    if (!pspk1PtrOk(ra))
        error = SCE_ERROR_KERNEL_ILLEGAL_ADDRESS;
    else {
        intrState = sceKernelLoadCoreLock();
        mod = sceKernelFindModuleByAddress(ra);
        if (mod != null)
            error = SCE_ERROR_KERNEL_ERROR;
        else
            error = (s32)mod->modid;
        sceKernelLoadCoreUnlock(intrState);
    }

    pspShiftK1(oldK1);
    return (SceUID)error;
}

//0x00004598
//ModuleMgrForUser_D8B73127
SceUID sceKernelGetModuleIdByAddress(s32 addr)
{
    s32 error;
    s32 intrState;
    SceModule *mod;
    s32 oldK1 = pspShiftK1();

    if (sceKernelIsIntrContext()) //45C4
        error = SCE_ERROR_KERNEL_CANNOT_BE_CALLED_FROM_INTERRUPT;
    else if (!pspk1PtrOk(addr)) //0x45D0
        error = SCE_ERROR_KERNEL_ILLEGAL_ADDRESS;
    else {
        intrState = sceKernelLoadCoreLock(); //0x45D8
        mod = sceKernelFindModuleByAddress(addr); //0x45E4
        if (mod == null)
            error = SCE_ERROR_KERNEL_UNKNOWN_MODULE;
        else 
            error = (s32) mod->modid;
        sceKernelLoadCoreUnlock(intrState);
    }

    return (SceUID)error;
}
//0x00004628
//ModuleMgrForUser_D2FBC957
s32 sceKernelGetModuleGPByAddress(s32 addr, s32 *gp)
{
    SceModule *mod;
    s32 intrState;
    s32 error = SCE_ERROR_OK;
    s32 oldK1 = pspShiftK1();

    if (!pspk1PtrOk(addr))
        error = SCE_ERROR_KERNEL_ILLEGAL_ADDRESS;
    else if (gp == null)
        error = SCE_ERROR_KERNEL_ILLEGAL_ADDRESS;
    else if (!pspk1PtrOk(gp))
        error = SCE_ERROR_KERNEL_ILLEGAL_ADDRESS;
    else {
        intrState = sceKernelLoadCorelock();
        mod = sceKernelFindModuleByAddress(arg0);
        if (mod == null)
            error = SCE_ERROR_KERNEL_UNKNOWN_MODULE;
        else
            *gp = mod->gpAddress;

        sceKernelLoadCoreUnlock(intrState);
    }
    return error;
}

//0x000051AC
void ModuleMgrForUser_1196472E(void *modBuf, SceSize modBufSize, void arg2, SceKernelLMOption *option)
{
    SceModuleMgrParam param;
    s32 error = SCE_ERROR_OK;
    s32 oldK1 = pspShiftK1();

    if (sceKernelIsIntrContext()) //0x51E0
        error = SCE_ERROR_KERNEL_CANNOT_BE_CALLED_FROM_INTERRUPT;
    else if (!pspK1IsUserMode()) //0x5218
        error = SCE_ERROR_KERNEL_ILLEGAL_PERMISSION_CALL;
    else if (sceKernelGetUserLevel() != 1 && sceKernelGetUserLevel() != 2)
        error = SCE_ERROR_KERNEL_ILLEGAL_PERMISSION_CALL;
    else if (!pspK1DynBufOk(arg1, arg0)) //
        error = SCE_ERROR_KERNEL_ILLEGAL_ADDRESS;
    else 
        error = lmOptionOk(option);

    if (error == SCE_ERROR_OK){
        initSceModuleMgrParam(param, API_TYPE_66, modBuf, modBufSize, 0); //0x5280
        error = sub_000075B4(param, option);
    }

    pspSetK1(oldK1);
    return error;
}

//0x0000529C
void ModuleMgrForUser_24EC0641(void *modBuf, SceSize modBufSize, void arg2, SceKernelLMOption *option)
{
    SceModuleMgrParam param;
    s32 error = SCE_ERROR_OK;
    s32 oldK1 = pspShiftK1();

    if (sceKernelIsIntrContext()) //0x52D0
        error = SCE_ERROR_KERNEL_CANNOT_BE_CALLED_FROM_INTERRUPT;
    else if (!pspK1IsUserMode()) //0x52D8
        error = SCE_ERROR_KERNEL_ILLEGAL_PERMISSION_CALL;
    else if (sceKernelGetUserLevel() != 3)
        error = SCE_ERROR_KERNEL_ILLEGAL_PERMISSION_CALL;
    else if (!pspK1DynBufOk(arg1, arg0)) //
        error = SCE_ERROR_KERNEL_ILLEGAL_ADDRESS;
    else 
        error = lmOptionOk(option);

    if (error == SCE_ERROR_OK){
        initSceModuleMgrParam(param, API_TYPE_67, modBuf, modBufSize, 0); //0x5280
        error = sub_000075B4(param, option);
    }

    pspSetK1(oldK1);
    return error;
}

//0x00005990
//ModuleMgrForUser_2E0911AA
int sceKernelUnloadModule(SceUID modid)
{
    s32 error = SCE_ERROR_OK;
    SceModuleMgrParam param;
    s32 oldK1 = pspShiftK1();
    if (sceKernelIsIntrContext()) //0x59B4
        error = SCE_ERROR_KERNEL_CANNOT_BE_CALLED_FROM_INTERRUPT;
    else {
        for (int i = 0; i < sizeof(SceModuleMgrParam) / 4; i ++)
            ((s32 *)param)[i] = 0;
        param.stopCMD = MM_COMMAND_MODULE_UNLOAD; //0x59D8
        param.startCMD = MM_COMMAND_MODULE_UNLOAD; //0x59DC
        param.unk52 = modid; //0x59E0
        param.unk68 = 0; //0x59E4
        param.unk72 = 0; //0x59E8
        param.unk84 = 0; //0x59F4
        error = sub_000074E4(param);
    }

    pspSetK1(oldK1);
    return error;
}

//0x00005A4C
//void ModuleMgrForUser_CC1D3699
void sceKernelStopUnloadSelfModule(SceSize argsize, void *argp, int *status, SceKernelSMOption *option)
{
    return sub_000077F0(0, pspGetRa(), argsize, argp, status, option);
}

//0x00005B10
//sceKernelModuleTextChecksumIsValid or something
void ModuleMgrForUser_CDE1C1FE()
{
    s32 oldK1 = pspShiftK1();
    SceModule *mod = g_ModuleMgrUIDs.mod;
    u32 checkSum = 0;
    if (mod == null)
        return 1;
    for (int i = 0; i < mod->textSize / sizeof(s32); i++)
        checkSum += mod->segmentAddr[0][i];

    return (checkSum ^ mod->unk220) == 0;
}