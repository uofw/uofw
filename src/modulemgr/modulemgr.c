//Modulemgr.c
#include <modulemgr.h>
#include <loadcore.h>
#include "modulemgr.h"

#define SCE_MAGIC 0x4543537E //"~SCE"
#define PSP_MAGIC 0x5053507E //"~PSP"

static typedef struct {
    SceUID thid; //0
    SceUID mutid; //4
    SceUID eventid; //8
    SceUID userThid; //12
    s32 unk16; //16
    s32 apiType; //20
    s32 unk24; //24
    s32 unk28; //28
    void *unkFunc; //32
    SceModule *mod; //36
} ModuleMgrUIDStruct; 

ModuleMgrUIDStruct g_ModuleMgrUIDs; //0x9A20

//0x00000000
static s32 releaseLibraries(SceModule *mod)
{
    s32 error;
    SceResidentLibraryEntryTable *libEnt;
    //Can all lib entries be released?
    if (mod->entSize != 0){ //0x1C
        for (libEnt = mod->entTop; libEnt < mod->entTop + mod->entSize; libEnt += libEnt->len * sizeof(s32)){
            if (libEnt->attribute & 0x8000 == 0){
                error = sceKernelCanReleaseLibrary(libEnt);
                if (error != 0)
                    return error;
            }
        }
    }

    if (mod->stubTop != -1) //0x70
        sceKernelUnLinkLibraryEntries(mod->stubTop, mod->stubSize); //0x80

    sub_00006F80(mod); //0x88
    return 0;
}

//0x000000B0
static s32 releaseModule(sceModule *mod)
{
    u32 status = mod->status & 0xF; //0xCC
    if (status < 2) //0xD4
        return SCE_ERROR_KERNEL_MODULE_CANNOT_REMOVE;
    if (status > 3 && status != 7) //0xDC - 0xE8
        return SCE_ERROR_KERNEL_MODULE_CANNOT_REMOVE;

    sceKernelMemset32(mod->textAddr, 77, (mod->textSize + 3) & ~0x3 ); //0x110
    sceKernelMemset(mod->textAddr + mod->textSize, mod->dataSize + mod->bssSize, -1); //0x130
    sceKernelReleaseModule(mod);

    if (mod->status & 0x1000 == 0) { //0x14C
        sceKernelFreePartitionMemory(mod->memId); //0x168
    }

    sceKernelDeleteModule(mod); //0x158
    return 0;
}

//0x00000178
static void sub_00000178(SceSize arglen, void *argp)
{
    SceModuleMgrParam *param = (SceModuleMgrParam *) argp;
    SceModule *newMod;
    void *unkSp; //size = 192;
    s32 error = SCE_ERROR_OK;

    param->mod; //s1
    for (int i = 0; i < 192/4; i++)
        ((s32 *)unkSp)[i] = 0;

    if (param->startCMD < MODULEMGR_NUM_COMMANDS){ //0x1B4
        switch(param->startCMD){
        case MM_COMMAND_0: //0x1D8
            if (param->mod == null){ //0x480
                newMod = sceKernelCreateModule();
                param->mod = newMod;
                if (mod == null)
                    break;
            }
            param->unk16 = unkSp; //0x1E0
            error = sub_00005C4C(param); // 0x1E4
            sceKernelChangeThreadPriority(null, 32); //0x1F8
            if (error < 0){ //0x1FC
                if (param->mod != null)//0x47C
                    sceKernelDeleteModule(param->mod);
                break;
            }

            param->retid = param->mod->modid; //0x20C
            if (param->endCMD == MM_COMMAND_0)
                break;
        case MM_COMMAND_1: //0x21C
            if (param->mod == null){ //0x21C
                newMod = sceKernelCreateModule(); //0x448
                if (newMod == null) //0x454
                    break;
                param->mod = newMod; //0x458
                newMod->status = (newMod->status & ~0xF) | 0x2; //0x470
                sceKernelRegisterModule(newMod);
            }

            if (param->unk16 == 0){ //0x228
                for (int i = 0; i < 192/4; i++)
                    ((s32 *)unkSp)[i] = 0;
                param->unk16 = unkSp;
            }

            error = sub_00006800(param); //0x244
            if (error >= 0){ //0x24C
                param->retid = mod->modid; //0x260
                if (param->endCMD == MM_COMMAND_1)
                    break;
            } else { //0x424
               if (param->mod == null) //0x428
                    break;
                param->retid = unkSp; //0x42C
                sceKernelReleaseModule(param->mod); //0x430
                sceKernelDeleteModule(param->mod); //0x438
                break;
            }

        case MM_COMMAND_MODULE_START: //0x270
            newMod = sceKernelGetModuleFromUID(param->unk52);
            if (newMod == null){ //0x278
                newMod = sceKernelFindModuleByUID(param->unk52); //0x400
                param->retid = SCE_ERROR_KERNEL_UNKNOWN_MODULE; //0x420
                if (newMod == null)
                    return SCE_ERROR_KERNEL_UNKNOWN_MODULE;
            }
            error = startModule(param, newMod, param->unk68, param->unk72, param->unk80); //0x290
            if (error == 0)
                param->retid = unkSp->unk44; //0x3F0
            else if (error == 1)
                param->retid = 0;//0x3E8
            else
                param->retid = error; // 0x2B0

            if (s2 < 0) //0x2B4
                break;

            if (param->endCMD == MM_COMMAND_MODULE_START)
                break;
        case MM_COMMAND_MODULE_STOP: //0x2C8
            if (param->mod == null){ //0x2C8
                newMod = sceKernelGetModuleFromUID(param->unk52);//0x3C8
                if (newMod == null){ //0x2C8
                    param->retid = SCE_ERROR_KERNEL_UNKNOWN_MODULE;
                    return SCE_ERROR_KERNEL_UNKNOWN_MODULE;
                }

                error = stopModule(param, newMod, param->startCMD, param->unk56, param->unk68, param->unk72, param->unk84);
                if (error == 1)
                    param->retid = unkSp->unk68; //0x3B0
                else
                    param->retid = error; //0x308
                if (error < 0)
                    break;
                if (param->endCMD == MM_COMMAND_MODULE_STOP)
                    break;
        case MM_COMMAND_MODULE_UNLOAD: //0x320
            newMod = sceKernelGetModuleFromUID(param->unk52);
            if (newMod != null){ //0x328
                error = releaseModule(newMod);
                if (error >= 0) //0x338
                    param->retid = mod->modid;
                else //0x390
                    param->retid = error;   
            else  //0x39C
                param->retid = SCE_ERROR_KERNEL_UNKNOWN_MODULE;
        }


    if (param->evid != null){ //0x350
        sceKernelChangeThreadPriority(null, 1); //0x374
        sceKernelSetEventFlag(param->evid, 1); //0x380

    return 0;
}

//0x0000501C
int module_reboot_phase(){
    return 0; //0x501C
}

//0x00005024
void module_reboot_before(){
    sceKernelSuspendThread(g_ModuleMgrUIDs.thid); //0x5034
}

//0x00005048
void module_bootstart()
{
    initChunks(); //0x5054
    g_ModuleMgrUIDs.thid = sceKernelCreateThread("SceKernelModmgrWorker", &sub_00000178, 0x20, 0x1000, 0, 0); //0x5078
    g_ModuleMgrUIDs.mutid = sceKernelCreateMutex("SceKernelModmgr", 0, 0, 0); //0x509C
    g_ModuleMgrUIDs.eventid = sceKernelCreateEventFlag("SceKernelModmgr", 0, 0, 0); //50B8
    g_ModuleMgrUIDs.userThid = -1; //0x50DC
    g_ModuleMgrUIDs.unk16 = -1; //0x50D0
    g_ModuleMgrUIDs.apiType = &g_ModuleMgrUIDs.apiType; //0x50D8
    g_ModuleMgrUIDs.unk24 = &g_ModuleMgrUIDs.apiType; //0x50F0
    g_ModuleMgrUIDs.unk32 = 0; //0x50E0
    g_ModuleMgrUIDs.unk36 = 0; //0x50D4

}

//0x00005C4C
//loadModule?
static void sub_00005C4C(SceModuleMgrParam *param)
{
    //param = s6
    //param->fid = s7
    s32 error = SCE_ERROR_OK;
    SceModule *mod;
    SceLoadCoreExecFileInfo *execInfo; //s4
    SceUID lmTmp; //fp
    void *lmTmpHead; //s5
    SceOff fileStart, fileEnd;
    s32 readBytes;

    mod = param->mod; //0x5C7C
    unk->unk288 = 0; //0x5C84
    unk->unk312 = mod; //0x5C90
    if (mod == null) //0x5C8C
        return SCE_ERROR_KERNEL_ERROR;
    if (mod->status & 0xF != MCB_STATUS_NOT_LOADED) //0x5CA0
        return SCE_ERROR_KERNEL_ERROR;
    mod->status = (mod->status & ~ 0xF) | MCB_STATUS_LOADING; //0x6CBC
    execInfo = param->execInfo; //0x5CB8
    for (int i = 0; i < sizeof(SceLoadCoreExecFileInfo) / 4; i++) //0x5CC4
        ((s32)execInfo)[i] = 0;
    offset = sceIoLseek(param->fid, 0, 1); //0x5CE8
    lmTmp = sceKernelAllocPartitionMemory(1, "SceModmgrLMTmp", 1, 512, 0); //0x5D0C
    if (lmTmp < 0) //0x5D18
        return lmTmp;
    lmTmpHead = sceKernelGetBlockHeadAddr(lmTmp); //0x5D24
    

    do {
        readBytes = sceIoRead(param->fid, lmTmpHead, 512); //0x5D34
        if (readBytes < 0){ //0x5D3C
            if (lmTmp >= 0){//0x67AC
                SceMemoryBlockInfo blockInfo;
                blockInfo.size = 56; //0x67C0
                if (sceKernelQueryMemoryBlockInfo(lmTmp, &blockInfo) >= 0){ //0x67C4
                    //unk->unk316 = blockInfo->unk44;
                    sceKernelMemset(blockInfo->unk40, 0, blockInfo->unk44); //0x67E8
                    sceKernelFreePartitionMemory(lmTmp, 0); //0x67F0
                }
            }
            return readBytes; //0x6718
        }
        sceIoLseek(param->fid, offset, 0); //0x5D50
        if (unk->unk228 != 0) //0x5D5C
            break;
        if (param->isSignChecked != 0) //0x5D68
            break;
        if (sub_00008568(unk->unk288, lmTmpHead, unk.unk288) == 0) //0x5D7C
            break;
        if (unk->unk288 < 0) //0x5D84
            for (;;); //0x5D98
        param->isSignChecked = 1; //0x5D94
    } while (true); //0x5D90

    error = sub_00007ED8(param, param->fid, lmTmpHead, unk.unk292, param->apiType); //0x5DB4
    if (error < 0){ //0x5DB8
        //0x673C
    }

    if (error != 0) { //0x5DC0
        fileStart = sceIoLseek(param->fid, 0, SEEK_CUR); //0x5DD0
        sceIoRead(param->fid, lmTmpHead, 512); //0x5DE8
        sceIoLseek(param->fid, fileStart, SEEK_SET); //0x5E04
    }

    execInfo->modeAttribute = 1; //0x5E10
    if (param->isSignChecked & 1 != 0)
        execInfo->isSignChecked = 1; //0x5E20

    error = sceKernelCheckExecFile(execInfo, lmTmpHead); //0x5E34
    if (error < 0){ //0x5E3C
        //0x6200
    }

    if (execInfo->execSize == 0){ //0x5E48
        //0x6670
        fileStart = sceIoLSeek(param->fid, 0, SEEK_CUR); //0x6678
        fileEnd = sceIoLSeek(param->fid, 0, SEEK_END); //0x6694
        sceIoLSeek(param->fid, fileStart, SEEK_SET); //0x66AC
        execInfo->execSize = (s32)(fileEnd - fileStart); //0x66BC
    }

    //0x5E50
    if (execInfo->elfType < 3){ //0x5E58
        //0x64AC
        //s5 = SCE_ERROR_KERNEL_UNSUPPORTED_PRX_TYPE 
        //0x6200
    }

    execInfo->isCompressed = execInfo->execAttribute & 0x1 == 0; //0x5E70
    if (param->unk144 == 0){ //0x5E78
        if (param->unk104 == 0){ //0x6510
            error = sub_00007FD0(param, execInfo); //0x651C
            if (error < 0){
                //s5 = error;
                //a0 = 288(sp)
                //0x6200
            }
        } else {
            //0x6534
            SceMemoryBlockInfo *unkInfo;
            unkInfo->size = 56; //0x653C
            if (sceKernelQueryMemoryBlockInfo(param->unk104, unkInfo) >= 0){ //0x6538
                SceUID memid;
                s32 unk2;
                if (sceKernelQueryMemoryInfo(unkInfo->unk40, &memId, &unk2) >= 0){
                    param->unk112 = memid; //0x6560
                    param->partitionId = unkInfo->unk44; //0x6564
                } 
            }
            //0x6568 
            if (execInfo->isKernelMod == 0){ //0x656C
                SceKernelMemoryPartitionInfo partInfo;
                partInfo.unk0 = 64; //0x6580
                if (sceKernelQueryMemoryPartitionInfo(param->partitionId, &partInfo) < 0 //0x657C
                    || partInfo.unk12 & 3 == 0) { //0x6594
                        //s5 = SCE_ERROR_KERNEL_MODULE_CANNOT_REMOVE
                        //0x6200
                    }
                execInfo->partitionId = param->partitionId; //0x65AC
                if (param->unk40 != 0){ //0x65B4
                    sceKernelMemoryPartitionInfo partInfo2;
                    partInfo2.unk0 = 16;
                    if (sceKernelQueryMemoryPartitionInfo(param->unk40, &partInfo2) < 0 //0x65C4
                        || partInfo2.unk12 & 0x3 == 0){ //0x65E8
                        //s5 = SCE_ERROR_KERNEL_MODULE_CANNOT_REMOVE
                        //0x6200
                    }

                }
                //0x5ED0
            } else {
                //0x65F8
            }
        }
    } else if (execInfo->isKernelMode != 0){ //0x5E84
        //0x6508
        //s5 =SCE_ERROR_KERNEL_PARTITION_MISMATCH
        //0x6200
    } else{
        //0x5E8C
        SceMemoryBlockInfo unkInfo;
        unkInfo->size = 56;//0x5E98
        if (sceKernelQueryMemoryBlockInfo(param->unk144, &unkInfo) >= 0){ //0x5E9C
            SceUID memid;
            s32 unk2;
            if (sceKernelQueryMemoryInfo(unkInfo->unk40, &memId, &unk2) >= 0){
                param->unk112 = memid; //0x5EBC
                param->partitionId = unkInfo->unk44; //0x5EC0
            }   
        }
        execInfo->partitionId = param->partitionId; //0x5EC8
    }

    //0x5ECC




}

//0x00006800
// relocateModule?
static void sub_00006800(SceModuleMgrParam *param)
{
    s32 error;

    SceLoadCoreExecFileInfo *execInfo = param->execInfo;
    execInfo->apiType = param->apiType; //0x6844
    execInfo->unk12 = param->unk100; //0x6858
    if (param->elfType < 1){ //0x6854
        execInfo->modeAttribute = SCE_EXEC_FILE_NO_HEADER_COMPRESSION; //0x6860
        execInfo->fileBase = param->fileBase; //0x6874
        if (param->isSignChecked & 0x1) //0x6870
            execInfo->isSignChecked = 1; //0x6878
        execInfo->unk104 = param.unk124; //0x6880
        if (sceKernelCheckExecFile(param->fileBase, execInfo) >= 0  //0x6884
            && sub_00007FD0(param, param->execInfo) >= 0) { //0x68A0

            if (execInfo->execAttribute & 0x1 == 0){ //0x68B0
                if (execInfo->apiType > 0x41) //0x6F70
                    v0 = 0;
                else if (execInfo->apiType > 0x30) //0x68C8
                    v0 = 1;
                else if (execInfo->apiType == 0x30) //0x68C0
                    v0 = 1//x6F64
                else if (execInfo->apiType == 2) //0x68D4
                    v0 = 1;
                else if (execInfo->apiType == 33) //0x68DC
                    v0 = 1;
                else
                    v0 = 0 //0x68E4

                if (v0 == 0){ //0x68E8
                    //0x6A00
                    if (param->isSignChecked & 0x1) //0x6A08
                        execInfo->isSignChecked = 1;
                    execInfo->unk104 = param.unk128; //0x6A18
                    if ( sceKernelCheckExecFile(param->unk28, execInfo) < 0 //0x6A1C
                        || sub_00007C34(param) < 0 //0x6A34
                        || sceKernelLoadExecutableObject(param->unk28, execInfo) < 0){ //0x6A48
                        //0x6C7C

                    }

                } else {
                    goto 0x68F4

                }
            }
            //a0 = execInfo->largestSegSize;
            //0x68F4

            

            
        }
        //0x6CCC
    
        
    } else {
        //0x6A2C
    }

}

//0x00006F80
static void sub_00006F80(SceModule *mod)
{
    SceResidentLibraryEntryTable libEnt;
    if (mod->entSize != null)
         for(libEnt = mod->entTop;  libEnt < mod->entTop + mod->entSize; libEnt += libEnt->size * sizeof(u32)){
            if (libEnt->attribute & 0x8000 == 0){
                sceKernelReleaseLibrary(libEnt);
            }
        }
    return;
}

//0x00006FF4
static s32 startModule(SceModuleMgrParam *param, SceModule *mod, SceSize arglen, void *argp, s32 *status)
{
    s32 error = SCE_ERROR_OK;

    if (mod->status & 0xF != MCB_STATUS_RELOCATED) //0x7034
        return SCE_ERROR_KERNEL_ERROR;

    mod->status = (mod->status & ~0xF) | MCB_STATUS_STARTING; //0x704C
    error = sub_00008124(param, mod); //0x7048
    if (error < 0){ //0x7050
        mod->status = (mod->status & ~0xF) | 0x3; //0x7138
        return error;
    }

    if (mod->userModThid != -1){ //0x705C
        g_ModuleMgrUIDs->userThid = mod->userModThid; //0x706C
        error = sceKernelStartThread(mod->userModThid, arglen, argp);
        if (error == SCE_ERROR_OK) //0x7080
            error = sceKernelWaitThreadEnd(mod->userModThid, 0); //0x7088
        g_ModuleMgrUIDs->userThid = -1; //0x7090
        sceKernelDeleteThread(mod->userModThid); //0x7094
        if (status != null) //0x70A0
            *status = error;
        if (error == SCE_ERROR_OK){ //0x70A8
            mod->status = (mod->status & ~0xF) | MCB_STATUS_STARTED;
            return SCE_ERROR_OK;
        } else {
            releaseLibraries(mod); //0x70EC
            mod->status = (mod->status & ~0xF) | MCB_STATUS_STOPPED; //0x7104
            releaseModule(mod); //0x7104
            return error;
        }

    }

}

//0x0000713C
static void stopModule(SceModuleMgrParam *param, SceModule *mod, s32 startCMD, void arg3, void arg4, void arg5)
{
    s32 error;
    s32 moduleStopThreadPriority;
    s32 threadStackSize;
    s32 moduleStopThreadAttr;
    switch (mod->status & 0xF){

    case MCB_STATUS_LOADED: //0x71AC
    case MCB_STATUS_RELOCATED: //0x71AC
        return SCE_ERROR_KERNEL_MODULE_NOT_STARTED;

    
    case MCB_STATUS_STARTED: //0x71F4
        if (mod->moduleStop == -1){ //0x71FC
            error = sub_00000000(mod); //0x7408
            if (error >= 0){
                mod->status = (mod->status & ~0xF) | MCB_STATUS_STOPPED; //0x7428
            } else {
                mod->status = (mod->status & ~0xF) | MCB_STATUS_STARTED; //0x73F4
            }
            retrun error;
        }

        moduleStopThreadPriority = arg2->unk28;
        if (moduleStopThreadPriority == 0) //0x7208
            moduleStopThreadPriority = mod->moduleStopThreadPriority = -1? 32:mod->moduleStopThreadPriority;
        threadStackSize = arg2->unk48;
        if (threadStackSize == 0) //0x7224
            threadStackSize = mod->threadStackSize;
        moduleStopThreadAttr = mod->moduleStopThreadAttr == -1? 0:mod->moduleStopThreadAttr;
        if (arg2->unk32 != 0)
            moduleStopThreadAttr |= arg2->unk32 & 0x00F06000;

        moduleStopThreadAttr &= 0xF000000F; //0x726C
        
        if (mod->status & 0x100){
            moduleStartThreadStacksize = moduleStartThreadStacksize == 0? 0x00040000: moduleStartThreadStacksize;
            switch(mod->attribute & 0x1E00){ //0x727C
            case 0x800:
                moduleStopThreadAttr |= 0xC0000000;
                break;
            case 0x600:
                moduleStopThreadAttr |= 0xB0000000;
                break;
            case 0x400:
                moduleStopThreadAttr |= 0xA0000000;
                break;
            case 0x200:
                moduleStopThreadAttr |= 0x90000000;
                break;
            default:
                moduleStopThreadAttr |= 0x80000000;
            }

        } else if (threadStackSize == 0)
            threadStackSize = 4096;

        //072B4



    case MCB_STATUS_STOPPING: //0x74CC
        return SCE_ERROR_KERNEL_MODULE_ALREADY_STOPPING;
    case MCB_STATUS_STOPPED: //0x74D8
        return SCE_ERROR_KERNEL_MODULE_ALREADY_STOPPED;

    case MCB_STATUS_NOT_LOADED: //0x71E8
    case MCB_STATUS_LOADING: //0x71E8
    case MCB_STATUS_STARTING: //0x71E8
    case MCB_STATUS_UNLOADED; //0x71E8
    case default:
        return SCE_ERROR_KERNEL_ERROR;
    }


}

//0x000074E4
//startModuleMgr?
static s32 sub_000074E4(SceModuleMgrParam *param)
{
    s32 error;
    s32 *retid;

    s32 thid = sceKernelGetThreadId(); //0x7500
    if (thid < 0) //0x750C
        return thid;
    if (thid == param->thid){ //0x75A0
       kprintf("module manager busy.\n"); //0x75A0
       return SCE_ERROR_MODULE_MANAGER_BUSY; //0x75AC
    }

    param->retid = retid; // 0x7534
    param->evid = g_ModuleMgrUIDs.evid;
    error = sceKernelLockMutex(g_ModuleMgrUIDs.mutid, 1,0)//0x7538
    if (error < 0)
        return error;
    error = sceKernelStartThread(g_ModuleMgrUIDs.thid, 160, param); //0x7550
    if (error >= 0)
        sceKernelWaitEventFlag(g_ModuleMgrUIDs.eventid, 1, 17, 0, 0); //07570
    sceKernelUnlockMutex(g_ModuleMgrUIDs.mutid, 1);

    return *retid; 
}

//0x000075B4
static s32 sub_000075B4(SceModuleMgrParam *param, SceKernelLMOption *option)
{
    if (option != null){ //0x7608
        param->mpidtext = option->mpidtext; //0x75C8
        param->mpiddata = option->mpiddata; //0x75D0
        param->position = option->position;  //0x75D8
        param->access = option->access;    //0x75E0
    } else {
        param->access = 1; //0x7608
        param->mpidtext = 0; //0x7610
        param->mpiddata = 0; //0x7614
        param->position = 0; //0x761C
    }
    param->unk76 = 0; //0x75E4 
    param->unk80 = 0; //0x75EC
    param->unk96 = 0; //0x75F0  
    param->execInfo = null; //0x75F8

    return sub_000074E4(param);
}

//0x00007620
static lmOptionOk(SceKernelLMOption *option)
{
    if (option == null) //0x763C
        return SCE_ERROR_OK;
    if (!pspK1StaBufOk(option, 20)) //0x7644
        return SCE_ERROR_KERNEL_ILLEGAL_ADDRESS;
    if (sceKernelGetCompiledSdkVersion()  >= 0x2080000 && option->size != 20) //0x7668
        return SCE_ERROR_KERNEL_ILLEGAL_SIZE;

    return SCE_ERROR_OK;
}

//0x00007698
static void initSceModuleMgrParam(SceModuleMgrParam *param, s32 apiType, SceUID fid, SceSize modBufSize, s32 arg4)
{
    for (int i = 0; i < sizeof(param)/4; i++)
        ((s32 *)param)[i] = 0;

    param->unk156 = 0;
    param->isSignChecked = arg4;
    param->apiType = apiType;
    param->endCMD= MM_COMMAND_1;
    param->modBufSize = modBufSize;
    param->fid = fid;
    param->startCMD = MM_COMMAND_0;
    param->fileBase = fid;

}

//0x000076CC
//unloadModule?
static void sub_000076CC(SceUID *thid, s32 ra, SceSize args, void *argp, s32 *status, SceKernelLMOption *option)
{
    SceModuleMgrParam *param;
    s32 error;
    s32 *localStatus;
    SceModule *mod = sceKernelFindModuleByAddress(ra); //0x76FC
    if (mod == null) 
        return SCE_ERROR_KERNEL_MODULE_CANNOT_STOP; //0x770C
    if (mod->attribute & 1) //0x72A0
        return SCE_ERROR_KERNEL_MODULE_CANNOT_STOP;

    for (int i = 0; i < sizeof(param)/4; i++) //0x7730 - 0x7738
        ((s32 *)param)[i] = 0;

    param->startCMD = MM_COMMAND_MODULE_STOP; //0x773c
    param->endCMD= MM_COMMAND_MODULE_UNLOAD; //0x7740
    param->argp = argp; //0x7750
    param->unk52 = mod->modId; //0x7754
    param->args = args;//0x775C
    param->unk56 = mod->modId; //0x7764
    if (status)
        param->status = status;
    else
        param->status = localStatus;
    if (option){ //0x776C
        param->mpidtext = option->mpidtext; //0x7784
        param->mpiddata = option->mpiddata; //0x7788
        param->unk28 = option->flags; //0x778C
        param->unk32 = option->unk16; //0x7790
    } else {
        param->unk44 = 0; //0x7770
        param->unk48 = 0; //0x77D4
        param->unk28 = 0; //0x77D8
        param->unk32 = 0; //0x77E0
    }

    error = sub_000074E4(param);
    if (error < 0)
        return error;
    sceKernelExitDeleteThread(thid); //0x77A4
    return error;
}

//0x000077F0
static void sub_000077F0(SceUID thid, s32 ra, SceSize args, void *argp, s32 *status, SceKernelLMOption *option)
{
    s32 error;
    s32 returnaddr;
    s32 oldk1 = pspShiftK1();

    if (sceKernelIsIntrContext()) //0x783C
        error = SCE_ERROR_KERNEL_CANNOT_BE_CALLED_FROM_INTERRUPT;
    else if (arg3 != null && !pspK1DynBufOk(arg3, arg2)) //0x789C
        error = SCE_ERROR_KERNEL_ILLEGAL_ADDRESS;
    else if (arg4 != null && !pspK1StaBufOk(arg4, 4)) //0x7878
        error = SCE_ERROR_KERNEL_ILLEGAL_ADDRESS;
    else if (option != null){ //0x7880
        if (sceKernelGetCompiledSdkVersion() < 0x2080000 && option->size != 20)
            error = SCE_ERROR_KERNEL_ILLEGAL_SIZE;
        else {
            if ((s32 *)option[16] & 0xFF0F9FFF != 0) //0x7900
                error = sce_Error_KERNEL_ERROR;
        }
    if (!error){
        returnaddr = pspK1IsUserMode()? sceKernelGetSyscallRA():ra; //0x791C
        if (!pspK1PtrOk(ra)) //0x792C
            error = SCE_ERROR_KERNEL_ILLEGAL_ADDRESS;
        else {
            error = sub_000076CC(thid, returnaddr, args, argp, status, option); //0x7950
        }
    }

    pspSetK1(oldk1);
    return error;
}

//0x00007968
static s32 sub_00007968(SceModule *mod, SceResidentLibraryEntryTable *lib)
{


}

//0x00007C34
static void sub_00007C34(SceModuleMgrParam *param)
{
        //TODO VERIFY THIS SUB
    s32 error = SCE_ERROR_OK;
    SceLoadCoreExecFileInfo *execInfo = param->execInfo;
    error = sceKernelProbeExecutableObject(execInfo->modBuf, execInfo); //0x7C5C
    if (error < 0) //0x7C68
        return error; 

    if (execInfo->elfType > 4) //0x7C7C
        return SCE_ERROR_KERNEL_ILLEGAL_OBJECT_FORMAT;

    switch (execInfo->elfType){
    case 0:
    case 1: 
    case 3: //0x7ECC
        return SCE_ERROR_KERNEL_ILLEGAL_OBJECT_FORMAT;
    case 2: //0x7D0C
        if (execInfo->modInfoAttribute & 0x1E00 != 0x1000){ //0x7D18
            PspSysmemPartitionInfo *partitionInfo;
            partitionInfo->size = 16; //0x7D24
            error = sceKernelQueryMemoryPartitionInfo(execInfo->partitionId, partitionInfo); //0x7D28
            if (error < 0){ //0x7D30
                return error
            if (partitionInfo->attr & 0x3 == 0) //0x7D44
                return SCE_ERROR_KERNEL_ILLEGAL_PERMISSION;
            if (execInfo->isCompressed != 1){ //0x7D68
                if (param->position != 2){ //0x7D78
                    u8 type = 0;
                    s32 maxSegAlign = execInfo->maxSegAlign;
                    if (maxSegAlign > 256){ //0x7D8C
                        type = param->position; //0x7DA8
                        maxSegAlign = 0; //0x7DAC
                    } else if (param->position == 0) //0x7D94
                        type == 3; //0x7E08
                    else if (param->position == 1) //0x7D98
                        type == 4; //0x7E00

                    SceUID tmpUID = sceKernelAllocPartitionMemory(execInfo->partitionId, "SceModmgrModuleBlockAuto", type, param->largestSegSize, maxSegAlign); //0x7DB0
                    if (tmpUID < 0) //0x7DBC
                        return (s32)tmpUID; 

                    execInfo->topAddr = sceKernelGetBlockHeadAddr(tmpUID);
                    param->mod->memId = tmpUID;
                    
                } else { //0x7E0C
                    execInfo->topAddr = execInfo->fileBase; //0x7E14
                    execInfo->mod->memId =  execInfo->decompressionMemId; //0x7E1C
                }

                //0x7DD4
                param->mod->mpiddata = param->mod->mpidtext = execInfo->partitionId;
            } else { //0x7E20
                if (param->unk104 != 0 && param->position == 1){
                    execInfo->topAddr = sceKernelGetBlockHeadAddr(param->unk104); //0x7E64
                    execInfo->topAddr += param->unk112 - align(execInfo->largestSegSize,64);
                } else 
                    execInfo->topAddr = execInfo->fileBase;

                param->mod->mpIdText = param->mod->mpIdData = execInfo->decompressionMemId;
            }

            
        } //0x7E88
        break;
        
    case 4: //0x7CA0
        if (execInfo->modInfoAttribute & 0x1E00 == 0x1000) //0x7CB0
            return SCE_ERROR_KERNEL_ERROR;
        if (execInfo->isCompressed == 1){ //0x7CC0
            execInfo->topAddr = execInfo->fileBase; //0x7D00
            param->mod->memId = execInfo->decompressionMemId; //0x7D08 
        } 
        param->mod->mpiddata = param->mod->mpidtext = execInfo->partitionId; //0x7CC8
    }

    if (param->mod->topAddr == null) //0x7DEC
        return SCE_ERROR_KERNEL_NO_MEMORY;
    return error;

}

//0x00007ED8
static void sub_00007ED8(SceModuleMgrParam *param, SceUID fid, void *modBuffer, void arg3 /*out?*/, s32 apiType)
{
    

}

//0x00007FD0
static void sub_00007FD0(SceModuleMgrParam *param, SceLoadCoreExecFileInfo *execInfo)
{


}

//0x00008124
static void sub_00008124(SceModuleMgrParam *param, SceModule *mod)
{
    s32 error;
    error = sub_0000844C(mod); //0x8154
    if (error < 0) //0x837C
        return error;
    if (mod->stubTop != -1){ //0x816C
        if (mod->status & 0x100 != 0){ //0x817C
            error = sceKernelLinkLibraryEntriesWithModule(mod, mod->stubTop, mod->stubSize); //0x8188
        } else {
            error = sceKernelLinkLibraryEntries(mod->stubTop ,mod->stubSize)
        }

        if (error < 0){ //0x8190
            sub_00006F80(mod); //0x8424
            return error;
        }
    }
    

    mod->segmentChecksum = sceKernelSegmentChecksum(mod); //0x8198
    if (mod->unk224 == 0) //0x81A4
        mod->unk220 = 0; //0x841C
    else {
        s32 accum = 0;
        for (s32 i = 0; i < mod->textSize; i += 4){
            accum += mod->segmentAddr[i]; //0x81CC
        }
        mod->unk220 = accum; //0x81D8
    }

    s32 *addr;
    if (mod->moduleStart != -1){ //0x81E4
        addr = mod->moduleStart;
    else if (mod->moduleBootStart != -1)
        addr = mod->moduleBootStart;
    else 
        addr = mod->entryAddr;

    if (addr <= 0 || ~addr <= 0){ //0x81FC
        mod->userModThid = -1; //0x820C
        return SCE_ERROR_OK;
    }

    s32 unks5 = 0;
    if (param->modBuf == null){ //0x823C
        unks5 = ~mod->moduleStartThreadPriority == 0? 32:mod->moduleStartThreadPriority;
    }

    s32 unks2 = 0;
    if (param->mpIdData == null){ //0x8258
        unks2 = ~mod->moduleStartThreadStacksize == 0? 0:mod->moduleStartThreadStacksize;
    }

    //This function makes no fucking sense right now.

}
  
//0x0000844C
static void sub_0000844C(SceModule *mod)
{
    s32 error;
    SceResidentLibraryEntryTable *lib;
    if (mod->entSize == 0) //0x8468
        return SCE_ERROR_OK;
    for (lib = mod->entTop; lib < mod->entTop + mod->entSize; (void *)lib += lib->len * sizeof(s32)){
        if (lib->attribute & 0x1){ //0x848C
            if(mod->status & 0x100) //0x849C
                error = sceKernelRegisterLibraryForUser(lib);
            else
                error = sceKernelRegisterLibrary(lib);
                
            if (error != SCE_ERROR_OK){ //0x84AC
                SceResidentLibraryEntryTable *release;
                for (release = mod->entTop; release < lib; (void *)release += release->len *sizeof(s32)){
                    sceKernelReleaseLibrary(release);
                }
                return error;
            }

        } else if (lib->attribute & 0x8000)
            sub_00007968(mod, lib); //0x8558
    }

    return SCE_ERROR_OK;

}

//0x00008568
static s32 sub_00008568(s32 arg0, s32 *modBuf, SceUID *fid)
{
    void *unk;

    //arg0 is probably loadedFrom
    if (arg0 ^ 0x30 > 0 && arg0 ^ 0x10 > 0) //0x85E4
        return SCE_ERROR_OK;

    if (modBuf[modBuf[0] == SCE_MAGIC? 1:0] != PSP_MAGIC) //CLEAN THIS UP!
        return SCE_ERROR_OK; //0x85DC

    unk = &g_98FC; //0x861C
    for (int i = 0; i < 13; i++){
        if (unk->unk0 > 0 ){ //0x8630
            for (int j = 0; j < unk->unk4; j ++){
                if (memcmp(modBuf + 320, unk->unk8 + j*16,16) == 0){ //0x8668
                    *fid = sceIoOpen(unk->unk0, 0x04000001, 0);
                    return 1;
                }
            }        
        }
        unk += 12;
    }
     
    return 0;
}

//Imports