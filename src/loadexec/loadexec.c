/* Copyright (C) 2011, 2012, 2013 The uOFW team
   See the file COPYING for copying permission.
*/

#include <common_imp.h>

#include <iofilemgr_kernel.h>
#include <interruptman.h>
#include <loadcore.h>
#include <modulemgr.h>
#include <modulemgr_init.h>
#include <sysmem_kdebug.h>
#include <sysmem_kernel.h>
#include <sysmem_suspend_kernel.h>
#include <sysmem_sysclib.h>
#include <sysmem_utils_kernel.h>
#include <threadman_kernel.h>

#include <loadexec_kernel.h>
#include <loadexec_user.h>

#include "loadexec_int.h"
#include "reboot.h"

SCE_MODULE_INFO("sceLoadExec", SCE_MODULE_KIRK_MEMLMD_LIB | SCE_MODULE_KERNEL
        | SCE_MODULE_ATTR_EXCLUSIVE_START | SCE_MODULE_ATTR_EXCLUSIVE_LOAD | SCE_MODULE_ATTR_CANT_STOP, 1, 15);
SCE_MODULE_BOOTSTART("LoadExecInit");
SCE_SDK_VERSION(SDK_VERSION);

static char g_encryptedBootPath[]   = "disc0:/PSP_GAME/SYSDIR/EBOOT.BIN"; // 0x3AE4
static char g_unencryptedBootPath[] = "disc0:/PSP_GAME/SYSDIR/BOOT.BIN"; // 0x3B18
static char g_gameStr[] = "game"; // 0x3B38
static char g_umdEmuStr[] = "umdemu"; // 0x3B40
static char g_mlnAppStr[] = "mlnapp"; // 0x3B38
static char g_vshStr[] = "vsh"; // 0x3B40

SceUID g_loadExecMutex; // 0xD3C0
SceUID g_loadExecCb; // 0xD3C4
s32 g_loadExecIsInited; // 0xD3C8
s32 g_suppArgSet; // 0xD3CC
void *g_suppArgp; // 0xD3D0
s32 g_suppArgs; // 0xD3D4
SceKernelRebootArgType g_suppArgType; // 0xD3D8
void (*g_regExitCbCb)(); // 0xD3DC, some unknown callback

char **g_encryptedBootPathPtr = (char**)&g_encryptedBootPath; // 0xD390 [hardcoded??]

s32 *g_unkCbInfo[4]; // 0xD400

s32 LoadExecForUser_362A956B()
{
    s32 oldK1 = pspShiftK1();
    SceUID cbId;
    SceKernelCallbackInfo cbInfo;

    cbId = sceKernelCheckExitCallback();
    cbInfo.size = 56;
    s32 ret = sceKernelReferCallbackStatus(cbId, &cbInfo);
    if (ret < 0) {
        pspSetK1(oldK1);
        return ret;
    }
    s32 *argm8 = (s32*)cbInfo.common - 2;
    if (!pspK1PtrOk(argm8)) {
        pspSetK1(oldK1);
        return SCE_ERROR_KERNEL_ILLEGAL_ADDR;
    }
    s32 pos  = *(s32*)(argm8 + 0);
    s32 *unk = (s32*)*(s32*)(argm8 + 4);
    if ((u32)pos >= 4) {
        pspSetK1(oldK1);
        return SCE_ERROR_KERNEL_ILLEGAL_ARGUMENT;
    }
    if (!pspK1PtrOk(unk))
        return SCE_ERROR_KERNEL_ILLEGAL_ADDR;
    pspSetK1(oldK1);
    if (unk[0] < 12)
        return SCE_ERROR_KERNEL_ILLEGAL_SIZE;
    unk[1] = 0;
    unk[2] = -1;
    g_unkCbInfo[pos] = unk;
    return 0;
}

// 0x09D8
void copyArgsToRebootParam(SceKernelRebootParam *hwOpt, SceKernelLoadExecVSHParam *opt)
{
    if (opt->args != 0) {
        hwOpt->args[hwOpt->curArgs].argp = opt->argp;
        hwOpt->args[hwOpt->curArgs].args = opt->args;
        hwOpt->args[hwOpt->curArgs].type = SCE_KERNEL_REBOOT_ARGTYPE_DEFAULT;
        hwOpt->unk40 = hwOpt->curArgs;
        hwOpt->curArgs++;
    }

    // a3c
    if (opt->vshmainArgs == 0) {
        SceUID blkId = sceKernelGetChunk(0);
        if (blkId > 0) {
            SceKernelSysmemBlockInfo info;
            info.size = 56;
            sceKernelQueryMemoryBlockInfo(blkId, &info);
            hwOpt->args[hwOpt->curArgs].argp = info.unk40;
            hwOpt->args[hwOpt->curArgs].args = info.unk44;
            hwOpt->args[hwOpt->curArgs].type = SCE_KERNEL_REBOOT_ARGTYPE_VSHMAIN;
            hwOpt->curArgs++;
        }
    } else {
        hwOpt->args[hwOpt->curArgs].argp = opt->vshmainArgp;
        hwOpt->args[hwOpt->curArgs].args = opt->vshmainArgs;
        hwOpt->args[hwOpt->curArgs].type = SCE_KERNEL_REBOOT_ARGTYPE_VSHMAIN;
        hwOpt->curArgs++;
    }

    if (opt->extArgs == 0) {
        // b28
        if (sceKernelGetChunk(4) > 0) {
            hwOpt->args[hwOpt->curArgs].argp = InitForKernel_D83A9BD7(&hwOpt->args[hwOpt->curArgs].args);
            hwOpt->args[hwOpt->curArgs].type = SCE_KERNEL_REBOOT_ARGTYPE_EXT;
            hwOpt->curArgs++;
        }
    } else {
        hwOpt->args[hwOpt->curArgs].argp = opt->extArgp;
        hwOpt->args[hwOpt->curArgs].args = opt->extArgs;
        hwOpt->args[hwOpt->curArgs].type = SCE_KERNEL_REBOOT_ARGTYPE_EXT;
        hwOpt->curArgs++;
    }

    SceKernelGameInfo *info = sceKernelGetGameInfo();
    if (info->unk4 != 0) {
        hwOpt->args[hwOpt->curArgs].argp = info;
        hwOpt->args[hwOpt->curArgs].args = sizeof *info;
        hwOpt->args[hwOpt->curArgs].type = SCE_KERNEL_REBOOT_ARGTYPE_GAMEINFO;
        hwOpt->curArgs++;
    }
}

// 0x0BBC
void fixupArgsAddr(SceKernelRebootParam *hwOpt, SceKernelLoadExecVSHParam *opt __attribute__((unused)))
{
    s8 fixId[32];
    s32 fixCount = 0;
    void *addr = (void*)0x8B800000;
    if (sceKernelGetModel() == 0) {
        // E94
        if (sceKernelDipsw(10) == 1) {
            addr = (void*)0x8B800000;
            *(s32*)(0xBC100040) = (*(s32*)(0xBC100040) & 0xFFFFFFFC) | 2;
        } else
            addr = (void*)0x8A000000;
    }
    // C00 / end of E94
    // C04
    // C08
    s32 i, j;
    for (i = 0; i < 32; i++)
        fixId[i] = -1;

    // C4C
    for (i = 0; i < hwOpt->curArgs; i++) {
        SceKernelArgsStor *args = &hwOpt->args[i];
        switch (args->type & 0xFFFF) {
        case SCE_KERNEL_REBOOT_ARGTYPE_GAMEINFO:
        // E5C
        case SCE_KERNEL_REBOOT_ARGTYPE_NPDRM:
        // E7C
        case SCE_KERNEL_REBOOT_ARGTYPE_DEFAULT:
        case SCE_KERNEL_REBOOT_ARGTYPE_EXT:
        //
        case SCE_KERNEL_REBOOT_ARGTYPE_EMU:
        case SCE_KERNEL_REBOOT_ARGTYPE_VSHMAIN:
        // E4C
        case SCE_KERNEL_REBOOT_ARGTYPE_UNKNOWN8:
        case SCE_KERNEL_REBOOT_ARGTYPE_KERNEL:
        case SCE_KERNEL_REBOOT_ARGTYPE_FILENAME:
            // C7C
            // C80
            // C84
            // C88
            for (j = 0; j < 32; j++) {
                s32 mustmove = (s32)UCACHED(hwOpt->args[(u8)fixId[j]].argp) >= (s32)UCACHED(args->argp);
                if (mustmove) {
                    // E20 / E28
                    s32 k;
                    for (k = fixCount; k >= j; k--)
                        fixId[k + 1] = fixId[k];
                }
                if (mustmove || fixId[j] == -1) {
                    // E40
                    fixCount++;
                    fixId[j] = i;
                    break;
                }
            }
            break;
        }

        // CCC
        // CD0
    }

    // CDC / CF8
    for (i = 0; i < fixCount; i++) {
        SceKernelArgsStor *args = &hwOpt->args[(u8)fixId[i]];
        switch (args->type & 0xFFFF) {
        case SCE_KERNEL_REBOOT_ARGTYPE_GAMEINFO:
        // DF4
        case SCE_KERNEL_REBOOT_ARGTYPE_NPDRM:
        case SCE_KERNEL_REBOOT_ARGTYPE_DEFAULT:
        case SCE_KERNEL_REBOOT_ARGTYPE_VSHMAIN:
        case SCE_KERNEL_REBOOT_ARGTYPE_KERNEL:
        case SCE_KERNEL_REBOOT_ARGTYPE_FILENAME:
        // E10
        case SCE_KERNEL_REBOOT_ARGTYPE_EXT:
        // E10
        case SCE_KERNEL_REBOOT_ARGTYPE_EMU:
        // DE4
        case SCE_KERNEL_REBOOT_ARGTYPE_UNKNOWN8:
            // D48 / D4C
            addr -= UPALIGN256(args->args);
            sceKernelMemmove(addr, args->argp, args->args);
            if ((args->type & 0xFFFF) == SCE_KERNEL_REBOOT_ARGTYPE_NPDRM) {
                // DD0
                sceKernelMemset(args->argp, 0, args->args);
            }

            // D74
            args->argp = addr;
            break;
        }

        // D7C
        if ((args->type & 0xFFFF) == SCE_KERNEL_REBOOT_ARGTYPE_UNKNOWN8) {
            // DC8
            hwOpt->unk76 = args->argp;
        }
        // D84
    }
    hwOpt->unk72 = addr;
}

// BD2F1094
s32 sceKernelLoadExec(char *file, SceKernelLoadExecParam *opt)
{
    s32 ret;
    s32 oldK1 = pspShiftK1();
    ret = sceKernelLockMutex(g_loadExecMutex, 1, 0);
    if (ret < 0) {
        pspSetK1(oldK1);
        return ret;
    }
    s32 oldD384 = g_loadExecCb;
    g_loadExecCb = 0;
    g_suppArgSet = 0;
    g_suppArgp = NULL;
    g_suppArgs = 0;
    g_suppArgType = SCE_KERNEL_REBOOT_ARGTYPE_NONE;
    ret = sceKernelBootFrom();
    if (ret >= 48) {
        g_loadExecCb = oldD384;
        sceKernelUnlockMutex(g_loadExecMutex, 1);
        pspSetK1(oldK1);
        return SCE_ERROR_KERNEL_ILLEGAL_PERMISSION_CALL;
    }

    if ((ret == 0 && sceKernelIsToolMode() != 0) || ret == 32) {
        s32 var;
        // FB4
        if (sceKernelGetCompiledSdkVersion() != 0 && sceKernelGetAllowReplaceUmd(&var) == 0 && var != 0) {
            g_loadExecCb = oldD384;
            sceKernelUnlockMutex(g_loadExecMutex, 1);
            pspSetK1(oldK1);
            return SCE_ERROR_KERNEL_ILLEGAL_PERMISSION_CALL;
        }
        // FE0
        if (sceKernelIsIntrContext() != 0) {
            g_loadExecCb = oldD384;
            sceKernelUnlockMutex(g_loadExecMutex, 1);
            pspSetK1(oldK1);
            return SCE_ERROR_KERNEL_CANNOT_BE_CALLED_FROM_INTERRUPT;
        }
        if (pspK1IsUserMode()) {
            g_loadExecCb = oldD384;
            sceKernelUnlockMutex(g_loadExecMutex, 1);
            pspSetK1(oldK1);
            return SCE_ERROR_KERNEL_ILLEGAL_PERMISSION_CALL;
        }
        if (file == NULL || !pspK1PtrOk(file)) {
            g_loadExecCb = oldD384;
            sceKernelUnlockMutex(g_loadExecMutex, 1);
            pspSetK1(oldK1);
            return SCE_ERROR_KERNEL_ILLEGAL_ADDR;
        }
        if (opt != NULL) {
            if (!pspK1StaBufOk(opt, sizeof *opt)) {
                g_loadExecCb = oldD384;
                sceKernelUnlockMutex(g_loadExecMutex, 1);
                pspSetK1(oldK1);
                return SCE_ERROR_KERNEL_ILLEGAL_ADDR;
            }
            if (opt->key != NULL && !pspK1PtrOk(opt->key)) {
                g_loadExecCb = oldD384;
                sceKernelUnlockMutex(g_loadExecMutex, 1);
                pspSetK1(oldK1);
                return SCE_ERROR_KERNEL_ILLEGAL_ADDR;
            }
            // 1058
            if (opt->argp != NULL && !pspK1DynBufOk(opt->argp, opt->args)) {
                g_loadExecCb = oldD384;
                sceKernelUnlockMutex(g_loadExecMutex, 1);
                pspSetK1(oldK1);
                return SCE_ERROR_KERNEL_ILLEGAL_ADDR;
            }
        }
        s32 apiType;
        // 107C
        if (sceKernelGetChunk(3) < 0) {
            // 1220
            if (sceKernelIsDVDMode() == 0 && strcmp(file, g_unencryptedBootPath) == 0) {
                file = *g_encryptedBootPathPtr;
                apiType = SCE_INIT_APITYPE_GAME_BOOT;
            } else
                apiType = SCE_INIT_APITYPE_GAME_EBOOT;
        } else {
            s32 apiType2;
            if (strcmp(file, g_unencryptedBootPath) != 0) {
                // 120C
                apiType = SCE_INIT_APITYPE_EMU_EBOOT_EF;
                apiType2 = SCE_INIT_APITYPE_EMU_EBOOT_MS;
            } else {
                file = *g_encryptedBootPathPtr;
                apiType = SCE_INIT_APITYPE_EMU_BOOT_EF;
                apiType2 = SCE_INIT_APITYPE_EMU_BOOT_MS;
            }
            // 10BC
            if (InitForKernel_9D33A110() != 80) // not EF
                apiType = apiType2;
        }
        // 10C0
        ret = ioctlAndDevctl(file, 0x208810, 0x208010);
        if (ret < 0) {
            g_loadExecCb = oldD384;
            sceKernelUnlockMutex(g_loadExecMutex, 1);
            pspSetK1(oldK1);
            return ret;
        }
        if (sceKernelIsToolMode() != 0) {
            // 11BC
            SceIoStat stat;
            if (sceIoGetstat(file, &stat) >= 0 && stat.st_size > 0x1780000) { // 11EC
                g_loadExecCb = oldD384;
                sceKernelUnlockMutex(g_loadExecMutex, 1);
                pspSetK1(oldK1);
                return SCE_ERROR_KERNEL_ERROR;
            }
        }
        SceKernelLoadExecVSHParam vshParam;
        RunExecParams args;

        // 10F0
        vshParam.size = 48;
        vshParam.vshmainArgs = 0;
        vshParam.vshmainArgp = NULL;
        vshParam.configFile = NULL;
        vshParam.string = NULL;
        vshParam.flags = 0x10000;
        vshParam.extArgs = 0;
        vshParam.extArgp = NULL;
        if (opt == NULL) {
            // 11B0
            vshParam.args = 0;
            vshParam.argp = NULL;
        } else {
            vshParam.args = opt->args;
            vshParam.argp = opt->argp;
        }
        char *name;
        // 112C
        if (apiType == SCE_INIT_APITYPE_EMU_EBOOT_MS || apiType == SCE_INIT_APITYPE_EMU_BOOT_MS || apiType == SCE_INIT_APITYPE_EMU_EBOOT_EF || apiType == SCE_INIT_APITYPE_EMU_BOOT_EF)
            name = g_umdEmuStr;
        else
            name = g_gameStr;
        // 116C
        vshParam.key = name;
        vshParam.opt11 = 0;

        args.apiType = apiType;
        args.args = 0;
        args.argp = file;
        args.vshParam = &vshParam;
        args.opt4 = 0;
        args.npDrm1 = NULL;
        args.npDrm2_1 = 0;
        args.npDrm2_2 = 0;
        ret = runExec(&args);
    } else
        ret = SCE_ERROR_KERNEL_ILLEGAL_PERMISSION_CALL;

    g_loadExecCb = oldD384;
    sceKernelUnlockMutex(g_loadExecMutex, 1);
    pspSetK1(oldK1);
    return ret;
}

s32 LoadExecForUser_8ADA38D3(char *file, SceKernelLoadExecParam *opt)
{
    RunExecParams args;
    SceKernelLoadExecVSHParam vshParam;
    char unkPtr[16];
    s32 unkPtr2[2];
    SceUID fileId;
    s32 oldD384;

    s32 oldK1 = pspShiftK1();
    s32 ret = sceKernelLockMutex(g_loadExecMutex, 1, 0);
    if (ret < 0) {
        pspSetK1(oldK1);
        return ret;
    }
    oldD384 = g_loadExecCb;
    g_loadExecCb = 0;
    g_suppArgSet = 0;
    g_suppArgp = NULL;
    g_suppArgs = 0;
    g_suppArgType = SCE_KERNEL_REBOOT_ARGTYPE_NONE;

    if (sceKernelIsIntrContext() != 0) {
        g_loadExecCb = oldD384;
        sceKernelUnlockMutex(g_loadExecMutex, 1);
        pspSetK1(oldK1);
        return SCE_ERROR_KERNEL_CANNOT_BE_CALLED_FROM_INTERRUPT;
    }
    if (pspK1IsUserMode()) {
        g_loadExecCb = oldD384;
        sceKernelUnlockMutex(g_loadExecMutex, 1);
        pspSetK1(oldK1);
        return SCE_ERROR_KERNEL_ILLEGAL_PERMISSION_CALL;
    }
    if (file == NULL || !pspK1PtrOk(file)) {
        g_loadExecCb = oldD384;
        sceKernelUnlockMutex(g_loadExecMutex, 1);
        pspSetK1(oldK1);
        return SCE_ERROR_KERNEL_ILLEGAL_ADDR;
    }

    if (opt != NULL) {
        if (!pspK1StaBufOk(opt, 16)) {
            g_loadExecCb = oldD384;
            sceKernelUnlockMutex(g_loadExecMutex, 1);
            pspSetK1(oldK1);
            return SCE_ERROR_KERNEL_ILLEGAL_ADDR;
        }
        if (opt->key != 0 && pspK1PtrOk(opt->key)) {
            g_loadExecCb = oldD384;
            sceKernelUnlockMutex(g_loadExecMutex, 1);
            pspSetK1(oldK1);
            return SCE_ERROR_KERNEL_ILLEGAL_ADDR;
        }

        // 135C
        if (opt->argp != NULL && !pspK1DynBufOk(opt->argp, opt->args)) {
            g_loadExecCb = oldD384;
            sceKernelUnlockMutex(g_loadExecMutex, 1);
            pspSetK1(oldK1);
            return SCE_ERROR_KERNEL_ILLEGAL_ADDR;
        }
    }

    // 1388
    ret = ioctlAndDevctl(file, 0x208813, 0x208013);
    if (ret < 0) {
        g_loadExecCb = oldD384;
        sceKernelUnlockMutex(g_loadExecMutex, 1);
        pspSetK1(oldK1);
        return ret;
    }

    fileId = sceIoOpen(file, 0x4000001, 511);
    if (fileId < 0) {
        g_loadExecCb = oldD384;
        sceKernelUnlockMutex(g_loadExecMutex, 1);
        pspSetK1(oldK1);
        return fileId;
    }
    ret = ModuleMgrForKernel_C3DDABEF(fileId, unkPtr, unkPtr2);
    if (ret < 0) {
        sceIoClose(fileId);
        g_loadExecCb = oldD384;
        sceKernelUnlockMutex(g_loadExecMutex, 1);
        pspSetK1(oldK1);
        return ret;
    }
    vshParam.size = 48;
    vshParam.vshmainArgs = 0;
    vshParam.vshmainArgp = NULL;
    vshParam.configFile = NULL;
    vshParam.string = NULL;
    vshParam.flags = 0x10000;
    vshParam.extArgs = 0;
    vshParam.extArgp = NULL;
    //
    if (opt == NULL) {
        // 1528
        vshParam.args = 0;
        vshParam.argp = 0;
    } else {
        vshParam.args = opt->args;
        vshParam.argp = opt->argp;
    }

    // 1414
    switch (sceKernelInitApitype()) {
    case SCE_INIT_APITYPE_GAME_EBOOT:
    case SCE_INIT_APITYPE_GAME_BOOT:
    case SCE_INIT_APITYPE_DISC:
    case SCE_INIT_APITYPE_DISC_UPDATER:
    case SCE_INIT_APITYPE_DISC_DEBUG:
    case SCE_INIT_APITYPE_UNK_GAME1:
    case SCE_INIT_APITYPE_UNK_GAME2:
        vshParam.key = g_gameStr;
        break;

    case SCE_INIT_APITYPE_EMU_EBOOT_MS:
    case SCE_INIT_APITYPE_EMU_BOOT_MS:
    case SCE_INIT_APITYPE_EMU_EBOOT_EF:
    case SCE_INIT_APITYPE_EMU_BOOT_EF:
    case SCE_INIT_APITYPE_DISC_EMU_MS1:
    case SCE_INIT_APITYPE_DISC_EMU_MS2:
    case SCE_INIT_APITYPE_DISC_EMU_EF1:
    case SCE_INIT_APITYPE_DISC_EMU_EF2:
        vshParam.key = g_umdEmuStr;
        break;

    case SCE_INIT_APITYPE_NPDRM_MS:
    case SCE_INIT_APITYPE_NPDRM_EF:
        if (sceKernelGetChunk(3) < 0)
            vshParam.key = g_gameStr;
        else
            vshParam.key = g_umdEmuStr;

    case SCE_INIT_APITYPE_MLNAPP_MS:
    case SCE_INIT_APITYPE_MLNAPP_EF:
        vshParam.key = g_mlnAppStr;
        break;

    default:
        if (sceKernelDipsw(13) != 1)
            vshParam.key = g_gameStr;
        else
            vshParam.key = g_umdEmuStr;
        break;
    }

    s32 apiType;
    vshParam.opt11 = 0;
    if (InitForKernel_9D33A110() == 80)
        apiType = SCE_INIT_APITYPE_NPDRM_EF;
    else
        apiType = SCE_INIT_APITYPE_NPDRM_MS;

    // 1464
    args.apiType = apiType;
    args.args = 0;
    args.argp = file;
    args.vshParam = &vshParam;
    args.opt4 = 0;
    args.npDrm1 = unkPtr;
    args.npDrm2_1 = unkPtr2[0];
    args.npDrm2_2 = unkPtr2[1];
    ret = runExec(&args);
    sceIoClose(fileId);
    g_loadExecCb = oldD384;
    sceKernelUnlockMutex(g_loadExecMutex, 1);
    pspSetK1(oldK1);
    return ret;
}

s32 LoadExecForUser_D1FB50DC(void *arg)
{
    SceKernelLoadExecVSHParam vshParam;
    RunExecParams args;
    s32 ret, oldVar;

    s32 oldK1 = pspShiftK1();
    ret = sceKernelLockMutex(g_loadExecMutex, 1, 0);
    if (ret < 0) {
        pspSetK1(oldK1);
        return ret;
    }

    oldVar = g_loadExecCb;
    g_loadExecCb = 0;
    if (sceKernelIsIntrContext() == 0) {
        if (pspK1IsUserMode()) {
            args.apiType = SCE_INIT_APITYPE_VSH_1;
            args.args = 0;
            args.argp = NULL;
            args.vshParam = &vshParam;
            args.opt4 = arg;
            args.npDrm1 = NULL;
            args.npDrm2_1 = 0;
            args.npDrm2_2 = 0;

            vshParam.size = sizeof vshParam;
            vshParam.args = 0;
            vshParam.argp = NULL;
            vshParam.key = g_vshStr;
            vshParam.vshmainArgs = 0;
            vshParam.vshmainArgp = NULL;
            vshParam.configFile = NULL;
            vshParam.string = NULL;
            vshParam.flags = 1;
            vshParam.extArgs = 0;
            vshParam.extArgp = NULL;
            vshParam.opt11 = 0;

            ret = runExec(&args);
        } else
            ret = SCE_ERROR_KERNEL_ILLEGAL_PERMISSION_CALL;
    } else
        ret = SCE_ERROR_KERNEL_CANNOT_BE_CALLED_FROM_INTERRUPT;

    g_loadExecCb = oldVar;
    sceKernelUnlockMutex(g_loadExecMutex, 1);
    pspSetK1(oldK1);
    return ret;
}

// 08F7166C
s32 sceKernelExitVSHVSH(SceKernelLoadExecVSHParam *opt)
{
    SceKernelLoadExecVSHParam vshParam;
    RunExecParams args;
    s32 oldK1 = pspShiftK1();

    if (sceKernelIsIntrContext() != 0) {
        pspSetK1(oldK1);
        return SCE_ERROR_KERNEL_CANNOT_BE_CALLED_FROM_INTERRUPT;
    }
    if (pspK1IsUserMode()) {
        pspSetK1(oldK1);
        return SCE_ERROR_KERNEL_ILLEGAL_PERMISSION_CALL;
    }

    // 16B0
    if (sceKernelGetUserLevel() != 4) {
        pspSetK1(oldK1);
        return SCE_ERROR_KERNEL_ILLEGAL_PERMISSION_CALL;
    }
    s32 ret = checkVSHParam(opt);
    if (ret < 0) {
        pspSetK1(oldK1);
        return ret;
    }
    vshParam.size = sizeof vshParam;
    if (opt == NULL) {
        // 17B8
        vshParam.args = 0;
        vshParam.argp = NULL;
        vshParam.key = NULL;
        vshParam.vshmainArgs = 0;
        vshParam.vshmainArgp = NULL;
        vshParam.configFile = NULL;
        vshParam.string = NULL;
        vshParam.flags = 0x10000;
        vshParam.extArgs = 0;
        vshParam.extArgp = NULL;
    } else {
        vshParam.args        = opt->args;
        vshParam.argp        = opt->argp;
        vshParam.key         = opt->key;
        vshParam.vshmainArgs = opt->vshmainArgs;
        vshParam.vshmainArgp = opt->vshmainArgp;
        vshParam.configFile  = opt->configFile;
        vshParam.string      = opt->string;
        vshParam.flags       = opt->flags;
        if (opt->size >= sizeof vshParam) {
            // 17A4
            vshParam.extArgs = opt->extArgs;
            vshParam.extArgp = opt->extArgp;
        } else {
            vshParam.extArgs = 0;
            vshParam.extArgp = NULL;
        }
    }

    // 1738
    if (vshParam.key == NULL)
        vshParam.key = g_vshStr;

    // 1754
    vshParam.flags |= 0x10000;
    vshParam.opt11 = 0;

    args.apiType = SCE_INIT_APITYPE_VSH_2;
    args.args = 0;
    args.argp = NULL;
    args.vshParam = &vshParam;
    args.opt4 = 0;
    args.npDrm1 = NULL;
    args.npDrm2_1 = 0;
    args.npDrm2_2 = 0;
    ret = runExec(&args);
    pspSetK1(oldK1);
    return ret;
}

// 1F88A490
s32 sceKernelRegisterExitCallback(SceUID cbId) // alias: 4AC57943 in ForUser
{
    s32 oldK1 = pspShiftK1();
    s32 mtx = sceKernelLockMutex(g_loadExecMutex, 1, 0);
    if (mtx < 0) {
        pspSetK1(oldK1);
        return mtx;
    }
    if (sceKernelGetThreadmanIdType(cbId) == 8) {
        // 188C
        g_loadExecCb = cbId;
        sceKernelUnlockMutex(g_loadExecMutex, 1);
        if (g_regExitCbCb != NULL && sceKernelGetCompiledSdkVersion() == 0) {
            // 18E0
            g_regExitCbCb();
        }

        // 18B4
        if (cbId != 0 && g_loadExecIsInited == 1)
            sceKernelInvokeExitCallback();
        pspSetK1(oldK1);
        return mtx;
    }
    sceKernelUnlockMutex(g_loadExecMutex, 1);
    if (sceKernelGetCompiledSdkVersion() <= 0x30904FF) {
        pspSetK1(oldK1);
        return mtx;
    }
    pspSetK1(oldK1);
    return SCE_ERROR_KERNEL_ILLEGAL_ARGUMENT;
}

// 1F08547A
s32 sceKernelInvokeExitCallback()
{
    s32 ret;
    ret = sceKernelLockMutex(g_loadExecMutex, 1, 0);
    if (ret < 0)
        return ret;

    if (g_loadExecCb != 0 && sceKernelGetThreadmanIdType(g_loadExecCb) != 8)
        g_loadExecCb = 0;

    // 1944
    if (g_loadExecCb != 0) {
        // 1984
        sceKernelPowerRebootStart(0);
        ret = sceKernelNotifyCallback(g_loadExecCb, 0);
    } else {
        g_loadExecIsInited = 1;
        ret = SCE_ERROR_KERNEL_NO_EXIT_CALLBACK;
    }

    // 1960
    sceKernelUnlockMutex(g_loadExecMutex, 1);
    return ret;
}

s32 LoadExecForKernel_BC26BEEF(SceKernelLoadExecVSHParam *opt, s32 arg1)
{
    if (opt == NULL)
        return SCE_ERROR_KERNEL_INVALID_ARGUMENT;
    if (opt->key == NULL)
        return SCE_ERROR_KERNEL_INVALID_ARGUMENT;
    if (arg1 == 0) {
        // 1A34
        if ((strcmp(opt->key, "game") == 0)
         || (strcmp(opt->key, "vsh") == 0)
         || (strcmp(opt->key, "updater") == 0))
            return 1;
        return 0;
    }
    if (arg1 != 1)
        return SCE_ERROR_KERNEL_INVALID_ARGUMENT;
    return strcmp(opt->key, "updater") != 0;
}

s32 LoadExecForKernel_DBD0CF1B(void *argp, s32 args, SceKernelRebootArgType argType)
{
    g_suppArgSet = 1;
    g_suppArgp = argp;
    g_suppArgs = args;
    g_suppArgType = argType;
    return 0;
}

// 2AC9954B
s32 sceKernelExitGameWithStatus()
{
    return LoadExecForUser_D1FB50DC(0);
}

// 05572A5F
s32 sceKernelExitGame()
{
    return LoadExecForUser_D1FB50DC(0);
}

// D8320A28
s32 sceKernelLoadExecVSHDisc(char *file, SceKernelLoadExecVSHParam *opt)
{
    return loadExecVSH(SCE_INIT_APITYPE_DISC, file, opt, 0x10000);
}

// D4B49C4B
s32 sceKernelLoadExecVSHDiscUpdater(char *file, SceKernelLoadExecVSHParam *opt)
{
    return loadExecVSH(SCE_INIT_APITYPE_DISC_UPDATER, file, opt, 0x10000);
}

// 1B305B09
s32 sceKernelLoadExecVSHDiscDebug(char *file, SceKernelLoadExecVSHParam *opt)
{
    if (sceKernelIsToolMode() != 0)
        return loadExecVSH(SCE_INIT_APITYPE_DISC_DEBUG, file, opt, 0x10000);
    return SCE_ERROR_KERNEL_NOT_IMPLEMENTED;
}

s32 LoadExecForKernel_F9CFCF2F(char *file, SceKernelLoadExecVSHParam *opt)
{
    return loadExecVSH(SCE_INIT_APITYPE_DISC_EMU_MS1, file, opt, 0x10000);
}

s32 LoadExecForKernel_077BA314(char *file, SceKernelLoadExecVSHParam *opt)
{
    return loadExecVSH(SCE_INIT_APITYPE_DISC_EMU_MS2, file, opt, 0x10000);
}

s32 LoadExecForKernel_E704ECC3(char *file, SceKernelLoadExecVSHParam *opt)
{
    return loadExecVSH(SCE_INIT_APITYPE_DISC_EMU_EF1, file, opt, 0x10000);
}

s32 LoadExecForKernel_47A5A49C(char *file, SceKernelLoadExecVSHParam *opt)
{
    return loadExecVSH(SCE_INIT_APITYPE_DISC_EMU_EF2, file, opt, 0x10000);
}

// BEF585EC
s32 sceKernelLoadExecBufferVSHUsbWlan(s32 args, void *argp, SceKernelLoadExecVSHParam *opt)
{
    return loadExecVSHWithArgs(SCE_INIT_APITYPE_USBWLAN, args, argp, opt, 0x10000);
}

// 2B8813AF
s32 sceKernelLoadExecBufferVSHUsbWlanDebug(s32 args, void *argp, SceKernelLoadExecVSHParam *opt)
{
    if (sceKernelIsToolMode() != 0)
        return loadExecVSHWithArgs(SCE_INIT_APITYPE_USBWLAN_DEBUG, args, argp, opt, 0x10000);
    return SCE_ERROR_KERNEL_NOT_IMPLEMENTED;
}

s32 LoadExecForKernel_87C3589C(s32 args, void *argp, SceKernelLoadExecVSHParam *opt)
{
    return loadExecVSHWithArgs(SCE_INIT_APITYPE_UNK, args, argp, opt, 0x10000);
}

s32 LoadExecForKernel_7CAFE77F(s32 args, void *argp, SceKernelLoadExecVSHParam *opt)
{
    if (sceKernelIsToolMode() != 0)
        return loadExecVSHWithArgs(SCE_INIT_APITYPE_UNK_DEBUG, args, argp, opt, 0x10000);
    return SCE_ERROR_KERNEL_NOT_IMPLEMENTED;
}

// 4FB44D27
s32 sceKernelLoadExecVSHMs1(char *file, SceKernelLoadExecVSHParam *opt)
{
    return loadExecVSH(SCE_INIT_APITYPE_MS1, file, opt, 0x10000);
}

// D940C83C
s32 sceKernelLoadExecVSHMs2(char *file, SceKernelLoadExecVSHParam *opt)
{
    return loadExecVSH(SCE_INIT_APITYPE_MS2, file, opt, 0x10000);
}

// CC6A47D2
s32 sceKernelLoadExecVSHMs3(char *file, SceKernelLoadExecVSHParam *opt)
{
    return loadExecVSH(SCE_INIT_APITYPE_MS3, file, opt, 0x10000);
}

// 00745486
s32 sceKernelLoadExecVSHMs4(char *file, SceKernelLoadExecVSHParam *opt)
{
    return loadExecVSH(SCE_INIT_APITYPE_MS4, file, opt, 0x10000);
}

// 7CABED9B
s32 sceKernelLoadExecVSHMs5(char *file, SceKernelLoadExecVSHParam *opt)
{
    return loadExecVSH(SCE_INIT_APITYPE_MS5, file, opt, 0x10000);
}

s32 LoadExecForKernel_A6658F10(char *file, SceKernelLoadExecVSHParam *opt)
{
    return loadExecVSH(SCE_INIT_APITYPE_MS6, file, opt, 0x10000);
}

s32 LoadExecForKernel_16A68007(char *file, SceKernelLoadExecVSHParam *opt)
{
    return loadExecVSH(SCE_INIT_APITYPE_EF1, file, opt, 0x10000);
}

s32 LoadExecForKernel_032A7938(char *file, SceKernelLoadExecVSHParam *opt)
{
    return loadExecVSH(SCE_INIT_APITYPE_EF2, file, opt, 0x10000);
}

s32 LoadExecForKernel_40564748(char *file, SceKernelLoadExecVSHParam *opt)
{
    return loadExecVSH(SCE_INIT_APITYPE_EF3, file, opt, 0x10000);
}

s32 LoadExecForKernel_E1972A24(char *file, SceKernelLoadExecVSHParam *opt)
{
    return loadExecVSH(SCE_INIT_APITYPE_EF4, file, opt, 0x10000);
}

s32 LoadExecForKernel_C7C83B1E(char *file, SceKernelLoadExecVSHParam *opt)
{
    return loadExecVSH(SCE_INIT_APITYPE_EF5, file, opt, 0x10000);
}

s32 LoadExecForKernel_8C4679D3(char *file, SceKernelLoadExecVSHParam *opt)
{
    return loadExecVSH(SCE_INIT_APITYPE_EF6, file, opt, 0x10000);
}

s32 LoadExecForKernel_B343FDAB(char *file, SceKernelLoadExecVSHParam *opt)
{
    return loadExecVSH(SCE_INIT_APITYPE_UNK_GAME1, file, opt, 0x10000);
}

s32 LoadExecForKernel_1B8AB02E(char *file, SceKernelLoadExecVSHParam *opt)
{
    return loadExecVSH(SCE_INIT_APITYPE_UNK_GAME2, file, opt, 0x10000);
}

s32 LoadExecForKernel_C11E6DF1(char *file, SceKernelLoadExecVSHParam *opt)
{
    return loadExecVSH(SCE_INIT_APITYPE_MLNAPP_MS, file, opt, 0x10000);
}

s32 LoadExecForKernel_9BD32619(char *file, SceKernelLoadExecVSHParam *opt)
{
    return loadExecVSH(SCE_INIT_APITYPE_MLNAPP_EF, file, opt, 0x10000);
}

// C3474C2A
s32 sceKernelExitVSHKernel(SceKernelLoadExecVSHParam *arg)
{
    return loadExecKernel(SCE_INIT_APITYPE_KERNEL_1, arg);
}

s32 LoadExecForKernel_C540E3B3()
{
    return 0;
}

// 24114598
s32 sceKernelUnregisterExitCallback()
{
    s32 ret = sceKernelLockMutex(g_loadExecMutex, 1, 0);
    if (ret < 0)
        return ret;
    g_loadExecCb = 0;
    sceKernelUnlockMutex(g_loadExecMutex, 1);
    return ret;
}

// B57D0DEC
SceUID sceKernelCheckExitCallback()
{
    return g_loadExecCb;
}

s32 LoadExecForKernel_A5ECA6E3(void (*arg)())
{
    g_regExitCbCb = arg;
    return 0;
}

s32 LoadExecInit()
{
    g_loadExecCb = 0;
    g_loadExecMutex = sceKernelCreateMutex("SceLoadExecMutex", 0x101, 0, 0);
    g_loadExecIsInited = 0;
    g_regExitCbCb = NULL;
    sceKernelSetRebootKernel(rebootKernel);
    return 0;
}

// 0x20FC
s32 runExec(RunExecParams *args)
{
    if (args->apiType != SCE_INIT_APITYPE_KERNEL_REBOOT) {
        /* Run in a thread */
        s32 ret, threadEnd;
        SceKernelThreadOptParam opt;
        SceUID id;
        opt.size = 8;
        opt.stackMpid = 1;
        ret = sceKernelGetModel();
        if (ret != 0 && sceKernelApplicationType() != SCE_INIT_APPLICATION_VSH)
            opt.stackMpid = 8; // not in VSH

        id = sceKernelCreateThread("SceKernelLoadExecThread", (void*)runExecFromThread, 32, 0x8000, 0, &opt);
        if (id < 0)
            return id;
        sceKernelStartThread(id, 40, args);
        threadEnd = sceKernelWaitThreadEnd(id, 0);
        sceKernelExitThread(0);
        return threadEnd;
    }
    return runExecFromThread(40, args);
}

// 0x21E0
s32 ioctlAndDevctl(char *name, s32 devcmd, s32 iocmd)
{
    if (strchr(name, '%') != NULL)
        return SCE_ERROR_KERNEL_ILLEGAL_LOADEXEC_FILENAME;
    char *comma = strchr(name, ':');
    if (comma != NULL) {
        s32 deviceLen = comma - name;
        char device[32];
        if (deviceLen >= 31)
            return SCE_ERROR_KERNEL_ERROR;
        strncpy(device, name, deviceLen + 1);
        device[deviceLen + 1] = '\0';
        s32 ret = sceIoDevctl(device, devcmd, 0, 0, 0, 0);
        // 2260
        if (ret == 0)
            return 0;
    }

    // 226C
    SceUID fileId = sceIoOpen(name, 1, 0x1ff);
    if (fileId < 0)
        return 0;
    s32 ret = sceIoIoctl(fileId, iocmd, 0, 0, 0, 0);
    sceIoClose(fileId);
    if (ret < 0)
        return SCE_ERROR_KERNEL_PROHIBIT_LOADEXEC_DEVICE;
    return 0;
}

// 0x2308
s32 checkVSHParam(SceKernelLoadExecVSHParam *opt)
{
    if (opt != NULL) {
        if (!pspK1StaBufOk(opt, 16))
            return SCE_ERROR_KERNEL_ILLEGAL_ADDR;
        if (opt->key != NULL && !pspK1PtrOk(opt->key))
            return SCE_ERROR_KERNEL_ILLEGAL_ADDR;
        // 2328
        if (opt->configFile != NULL && !pspK1PtrOk(opt->configFile))
            return SCE_ERROR_KERNEL_ILLEGAL_ADDR;
        // 2344
        if (opt->string != NULL && !pspK1PtrOk(opt->string))
            return SCE_ERROR_KERNEL_ILLEGAL_ADDR;
    }
    return 0;
}

// 0x2384
s32 loadExecVSH(s32 apiType, char *file, SceKernelLoadExecVSHParam *opt, u32 flags)
{
    s32 oldK1 = pspShiftK1();
    if (sceKernelIsIntrContext() == 0) {
        if (!pspK1IsUserMode()) {
            s32 iocmd, devcmd;
            // 23EC
            if (sceKernelGetUserLevel() != 4) {
                pspSetK1(oldK1);
                return SCE_ERROR_KERNEL_ILLEGAL_PERMISSION_CALL;
            }
            if (file == NULL) {
                pspSetK1(oldK1);
                return SCE_ERROR_KERNEL_ILLEGAL_ADDR;
            }
            if (!pspK1PtrOk(file)) {
                pspSetK1(oldK1);
                return SCE_ERROR_KERNEL_ILLEGAL_ADDR;
            }
            if (checkVSHParam(opt) < 0) {
                pspSetK1(oldK1);
                return SCE_ERROR_KERNEL_ILLEGAL_ADDR;
            }
            switch (apiType) {
            case SCE_INIT_APITYPE_DISC:
            case SCE_INIT_APITYPE_DISC_UPDATER:
            case SCE_INIT_APITYPE_DISC_DEBUG:
                devcmd = 0x208811;
                iocmd  = 0x208011;
                break;

            case SCE_INIT_APITYPE_DISC_EMU_MS1:
            case SCE_INIT_APITYPE_DISC_EMU_MS2:
            case SCE_INIT_APITYPE_DISC_EMU_EF1:
            case SCE_INIT_APITYPE_DISC_EMU_EF2:
            case SCE_INIT_APITYPE_MLNAPP_MS:
            case SCE_INIT_APITYPE_MLNAPP_EF:
                devcmd = 0x208814;
                iocmd  = 0x208014;
                break;

            case SCE_INIT_APITYPE_MS1:
            case SCE_INIT_APITYPE_MS2:
            case SCE_INIT_APITYPE_MS4:
            case SCE_INIT_APITYPE_MS5:
            case SCE_INIT_APITYPE_MS6:
            case SCE_INIT_APITYPE_EF1:
            case SCE_INIT_APITYPE_EF2:
            case SCE_INIT_APITYPE_EF4:
            case SCE_INIT_APITYPE_EF5:
            case SCE_INIT_APITYPE_EF6:
            case SCE_INIT_APITYPE_UNK_GAME1:
            case SCE_INIT_APITYPE_UNK_GAME2:
                devcmd = 0x208813;
                iocmd  = 0x208013;
                break;

            default:
                pspSetK1(oldK1);
                return SCE_ERROR_KERNEL_ERROR;
            }

            s32 ret = ioctlAndDevctl(file, devcmd, iocmd);
            if (ret < 0) {
                pspSetK1(oldK1);
                return ret;
            }
            if (apiType == SCE_INIT_APITYPE_DISC_DEBUG) {
                // 24D0
                if (sceKernelIsToolMode() != 0) {
                    SceIoStat stat;
                    if (sceIoGetstat(file, &stat) >= 0) {
                        if (stat.st_size > 0x1780000) {
                            pspSetK1(oldK1);
                            return SCE_ERROR_KERNEL_ERROR;
                        }
                    }
                }
            }
            SceKernelLoadExecVSHParam vshParam;
            RunExecParams args;

            // 2488
            copyVSHParam(&vshParam, flags, opt);
            args.apiType = apiType;
            args.args = 0;
            args.argp = file;
            args.vshParam = &vshParam;
            args.opt4 = 0;
            args.npDrm1 = NULL;
            args.npDrm2_1 = 0;
            args.npDrm2_2 = 0;
            ret = runExec(&args);
            pspSetK1(oldK1);
            return ret;
        } else {
            pspSetK1(oldK1);
            return SCE_ERROR_KERNEL_ILLEGAL_PERMISSION_CALL;
        }
    } else {
        pspSetK1(oldK1);
        return SCE_ERROR_KERNEL_CANNOT_BE_CALLED_FROM_INTERRUPT;
    }
}

// 0x2580
s32 loadExecVSHWithArgs(s32 apiType, s32 args, void *argp, SceKernelLoadExecVSHParam *opt, u32 flags)
{
    s32 oldK1 = pspShiftK1();
    if (sceKernelIsIntrContext() != 0) {
        pspSetK1(oldK1);
        return SCE_ERROR_KERNEL_CANNOT_BE_CALLED_FROM_INTERRUPT;
    }

    if (pspK1IsUserMode()) {
        s32 ret;
        RunExecParams rebootArgs;
        SceKernelLoadExecVSHParam vshArgs;
        // 25F4
        if (sceKernelGetUserLevel() != 4) {
            pspSetK1(oldK1);
            return SCE_ERROR_KERNEL_ILLEGAL_PERMISSION_CALL;
        }
        if (args == 0) {
            pspSetK1(oldK1);
            return SCE_ERROR_KERNEL_ILLEGAL_LOADEXEC_BUFFER;
        }
        if (argp != NULL && !pspK1DynBufOk(argp, args)) {
            pspSetK1(oldK1);
            return SCE_ERROR_KERNEL_ILLEGAL_ADDR;
        }

        // 263C
        ret = checkVSHParam(opt);
        if (ret < 0) {
            pspSetK1(oldK1);
            return ret;
        }
        copyVSHParam(&vshArgs, flags, opt);
        rebootArgs.apiType = apiType;
        rebootArgs.args = args;
        rebootArgs.argp = argp;
        rebootArgs.vshParam = &vshArgs;
        rebootArgs.opt4 = 0;
        rebootArgs.npDrm1 = NULL;
        rebootArgs.npDrm2_1 = 0;
        rebootArgs.npDrm2_2 = 0;
        ret = runExec(&rebootArgs);
        pspSetK1(oldK1);
        return ret;
    } else {
        pspSetK1(oldK1);
        return SCE_ERROR_KERNEL_ILLEGAL_PERMISSION_CALL;
    }
}

// 0x26B0
s32 loadExecKernel(s32 apiType, SceKernelLoadExecVSHParam *opt)
{
    SceKernelLoadExecVSHParam vshParam;
    RunExecParams args;
    s32 ret;
    s32 oldK1 = pspShiftK1();

    if (apiType != SCE_INIT_APITYPE_KERNEL_REBOOT && sceKernelIsIntrContext() != 0) { // 2810
        pspSetK1(oldK1);
        return SCE_ERROR_KERNEL_CANNOT_BE_CALLED_FROM_INTERRUPT;
    }

    // 26C4
    if (pspK1IsUserMode()) {
        pspSetK1(oldK1);
        return SCE_ERROR_KERNEL_ILLEGAL_PERMISSION_CALL;
    }
    ret = checkVSHParam(opt);
    if (ret < 0) {
        pspSetK1(oldK1);
        return ret;
    }
    vshParam.size = sizeof vshParam;
    if (opt == NULL) {
        // 27E0
        vshParam.args = 0;
        vshParam.argp = NULL;
        vshParam.key = NULL;
        vshParam.vshmainArgs = 0;
        vshParam.vshmainArgp = NULL;
        vshParam.configFile = NULL;
        vshParam.string = NULL;
        vshParam.flags = 0x10000;
        vshParam.extArgs = 0;
        vshParam.extArgp = NULL;
    } else {
        vshParam.args = opt->args;
        vshParam.argp = opt->argp;
        vshParam.key = opt->key;
        vshParam.vshmainArgs = opt->vshmainArgs;
        vshParam.vshmainArgp = opt->vshmainArgp;
        vshParam.configFile = opt->configFile;
        vshParam.string = opt->string;
        vshParam.flags = opt->flags;
        if (opt->size >= sizeof vshParam) {
            // 27D4
            vshParam.extArgs = opt->extArgs;
            vshParam.extArgp = opt->extArgp;
        } else {
            vshParam.extArgs = 0;
            vshParam.extArgp = NULL;
        }
    }

    // 2750
    if (vshParam.key == NULL)
        vshParam.key = g_vshStr;

    // 276C
    args.apiType = apiType;
    args.args = 0;
    args.argp = NULL;
    args.vshParam = &vshParam;
    args.opt4 = 0;
    args.npDrm1 = NULL;
    args.npDrm2_1 = 0;
    args.npDrm2_2 = 0;

    vshParam.flags |= 0x10000;
    vshParam.opt11 = 0;
    ret = runExec(&args);
    pspSetK1(oldK1);
    return ret;
}

// 0x2844
s32 rebootKernel(SceKernelLoadExecVSHParam *arg)
{
    return loadExecKernel(SCE_INIT_APITYPE_KERNEL_REBOOT, arg);
}

// 0x2864
s32 runExecFromThread(u32 args __attribute__((unused)), RunExecParams *opt) // 2864
{
    NpDrmArg npDrmArg;
    char str1[256], str2[256], str3[256];
    if (opt == NULL)
        return SCE_ERROR_KERNEL_ERROR;
    npDrmArg.size = sizeof npDrmArg;
    npDrmArg.npDrm2_1 = opt->npDrm2_1;
    npDrmArg.npDrm2_2 = opt->npDrm2_2;

    if (opt->npDrm1) {
        memcpy(&npDrmArg.npDrm1, opt->npDrm1, 16);
        memset(opt->npDrm1, 0, 16);
    }

    opt->npDrmArg = &npDrmArg;

    memcpy(&npDrmArg.vshParam, opt->vshParam, sizeof *opt->vshParam);
    opt->opt4 = &npDrmArg.vshParam;

    if (npDrmArg.vshParam.key != NULL) {
        strncpy(str1, npDrmArg.vshParam.key, 256);
        npDrmArg.vshParam.key = str1;
    } else if (npDrmArg.vshParam.configFile == NULL)
        return SCE_ERROR_KERNEL_ERROR;

    if (npDrmArg.vshParam.configFile != NULL) {
        strncpy(str2, npDrmArg.vshParam.configFile, 256);
        npDrmArg.vshParam.configFile = str2;
    }

    if (npDrmArg.vshParam.string != NULL) {
        strncpy(str3, npDrmArg.vshParam.string, 256);
        npDrmArg.vshParam.string = str3;
    }

    sceKernelSetSystemStatus(0x40000);
    if (opt->apiType != SCE_INIT_APITYPE_KERNEL_REBOOT) {
        s32 ret = sceKernelPowerRebootStart(0);
        if (ret < 0)
            return ret;
    }
    return runReboot(opt);
}

// 0x29A4
void copyVSHParam(SceKernelLoadExecVSHParam *dstOpt, u32 flags, SceKernelLoadExecVSHParam *srcOpt)
{
    dstOpt->size = sizeof *dstOpt;
    dstOpt->flags = flags;
    if (srcOpt == NULL) {
        // 2A24
        dstOpt->args = 0;
        dstOpt->argp = NULL;
        dstOpt->key = NULL;
        dstOpt->vshmainArgs = 0;
        dstOpt->vshmainArgp = NULL;
        dstOpt->configFile = NULL;
        dstOpt->string = NULL;
        dstOpt->extArgs = 0;
        dstOpt->extArgp = NULL;
    } else {
        dstOpt->args = srcOpt->args;
        dstOpt->argp = srcOpt->argp;
        dstOpt->key = srcOpt->key;
        dstOpt->vshmainArgs = srcOpt->vshmainArgs;
        dstOpt->vshmainArgp = srcOpt->vshmainArgp;
        dstOpt->configFile = srcOpt->configFile;
        dstOpt->string = srcOpt->string;
        if (srcOpt->size >= sizeof *dstOpt) {
            // 2A10
            dstOpt->extArgs = srcOpt->extArgs;
            dstOpt->extArgp = srcOpt->extArgp;
        } else {
            dstOpt->extArgs = 0;
            dstOpt->extArgp = NULL;
        }
    }

    // 29F0
    if (dstOpt->key == NULL)
        dstOpt->key = g_gameStr;

    // 2A08
    dstOpt->opt11 = 0;
    return;
}

// 0x2A64
s32 runReboot(RunExecParams *opt)
{
    SceKernelRebootArgType argType = SCE_KERNEL_REBOOT_ARGTYPE_NONE;
    SceKernelSysmemBlockInfo blkInfo;
    switch (opt->apiType) {
    case SCE_INIT_APITYPE_DEBUG:

    case SCE_INIT_APITYPE_MLNAPP_MS:
    case SCE_INIT_APITYPE_MLNAPP_EF:

    case SCE_INIT_APITYPE_EF4:
    case SCE_INIT_APITYPE_EF5:
    case SCE_INIT_APITYPE_EF6:

    case SCE_INIT_APITYPE_UNK_GAME1:
    case SCE_INIT_APITYPE_UNK_GAME2:

    case SCE_INIT_APITYPE_EF1:
    case SCE_INIT_APITYPE_EF2:

    case SCE_INIT_APITYPE_MS4:
    case SCE_INIT_APITYPE_MS5:
    case SCE_INIT_APITYPE_MS6:

    case SCE_INIT_APITYPE_MS1:
    case SCE_INIT_APITYPE_MS2:

    case SCE_INIT_APITYPE_USBWLAN:
    case SCE_INIT_APITYPE_USBWLAN_DEBUG:
    case SCE_INIT_APITYPE_UNK:
    case SCE_INIT_APITYPE_UNK_DEBUG:

    case SCE_INIT_APITYPE_DISC:
    case SCE_INIT_APITYPE_DISC_UPDATER:

    case SCE_INIT_APITYPE_DISC_DEBUG:
    case SCE_INIT_APITYPE_DISC_EMU_MS1:
    case SCE_INIT_APITYPE_DISC_EMU_MS2:
    case SCE_INIT_APITYPE_DISC_EMU_EF1:
    case SCE_INIT_APITYPE_DISC_EMU_EF2:

    case SCE_INIT_APITYPE_NPDRM_EF:

    case SCE_INIT_APITYPE_UNK0x100:

    case SCE_INIT_APITYPE_GAME_EBOOT:
    case SCE_INIT_APITYPE_GAME_BOOT:
    case SCE_INIT_APITYPE_EMU_EBOOT_MS:
    case SCE_INIT_APITYPE_EMU_BOOT_MS:
    case SCE_INIT_APITYPE_EMU_EBOOT_EF:
    case SCE_INIT_APITYPE_EMU_BOOT_EF:
    case SCE_INIT_APITYPE_NPDRM_MS:
        // (2AE4)
        argType = SCE_KERNEL_REBOOT_ARGTYPE_FILENAME;
        break;
    case SCE_INIT_APITYPE_VSH_2:
    case SCE_INIT_APITYPE_KERNEL_REBOOT:
    case SCE_INIT_APITYPE_KERNEL_1:
    case SCE_INIT_APITYPE_VSH_1:
        // 32CC
        argType = SCE_KERNEL_REBOOT_ARGTYPE_KERNEL;
        break;
    default:
        return SCE_ERROR_KERNEL_ERROR;
    }

    // 2AE8
    s32 rand = sceKernelGetInitialRandomValue();
    if (opt->apiType != SCE_INIT_APITYPE_KERNEL_REBOOT) {
        // 3218
        s32 ret = sceKernelRebootBeforeForUser(opt->vshParam);
        if (ret < 0)
            return ret;
    // 2AF8
        // 31F8
        ret = sceKernelRebootPhaseForKernel(3, opt->vshParam, 0, 0);
        if (ret < 0)
            return ret;
    // 2B00
        // 31D8
        ret = sceKernelRebootPhaseForKernel(2, opt->vshParam, 0, 0);
        if (ret < 0)
            return ret;
    }
    // 2B0C
    if (opt->apiType != SCE_INIT_APITYPE_KERNEL_REBOOT) {
        // 31C8
        sceKernelSuspendAllUserThreads();
    }

    // 2B1C
    sceKernelSetSystemStatus(0x40020);
    if (argType == SCE_KERNEL_REBOOT_ARGTYPE_KERNEL) {
        // 3194
        if (opt->vshParam->vshmainArgs == 0) {
            SceUID id = sceKernelGetChunk(0);
            if (id > 0) {
                blkInfo.size = 56;
                sceKernelQueryMemoryBlockInfo(id, &blkInfo);
                opt->vshParam->vshmainArgp = blkInfo.unk40;
            }
        }
    }

    // 2B30
    if (opt->apiType != SCE_INIT_APITYPE_KERNEL_REBOOT) {
        // 3174
        s32 ret = sceKernelRebootPhaseForKernel(1, opt->vshParam, 0, 0);
        if (ret < 0)
            return ret;
    // 2B38
        // 315C
        sceKernelRebootBeforeForKernel(opt->vshParam, 0, 0, 0);
    }

    // 2B40
    s32 ret = sceKernelLoadRebootBin(g_reboot, sizeof g_reboot);
    if (ret != 0) {
        sceKernelCpuSuspendIntr();
        *(s32*)(0xBC100040) = (*(s32*)(0xBC100040) & 0xFFFFFFFC) | 1;
        sceKernelMemset((void*)0x88600000, 0, 0x200000);
        sceKernelMemset(g_reboot, 0, sizeof g_reboot);
        memset((void*)0xBFC00000, 0, 4096);
        for (;;)
            ;
    }

    // 2BC4
    pspCop0StateSet(COP0_STATE_STATUS, pspCop0StateGet(COP0_STATE_STATUS) & 0xFFF7FFE0);
    // Skipped useless part

    // 2C58
    SceKernelRebootParam *hwOpt = makeRebootParam();
    hwOpt->unk24 |= argType;
    if (argType == SCE_KERNEL_REBOOT_ARGTYPE_FILENAME) {
        // 2F4C
        if (opt->args != 0) {
            // 3134
            hwOpt->args[0].argp = opt->argp;
            hwOpt->args[0].args = opt->args;
            hwOpt->args[0].type = SCE_KERNEL_REBOOT_ARGTYPE_KERNEL;
            hwOpt->curArgs = 1;
            hwOpt->unk36 = 0;
            // 2FFC
            copyArgsToRebootParam(hwOpt, opt->vshParam);
        } else if (opt->argp != NULL) {
            if ((opt->apiType == SCE_INIT_APITYPE_DISC_EMU_MS1) || (opt->apiType == SCE_INIT_APITYPE_DISC_EMU_MS2)
             || (opt->apiType == SCE_INIT_APITYPE_DISC_EMU_EF1) || (opt->apiType == SCE_INIT_APITYPE_DISC_EMU_EF2)
             || (opt->apiType == SCE_INIT_APITYPE_MLNAPP_MS) || (opt->apiType == SCE_INIT_APITYPE_MLNAPP_EF)) {
                // 2FAC
                // 2FB0
                hwOpt->args[0].argp = opt->vshParam->argp;
                hwOpt->args[0].args = strlen(opt->vshParam->argp) + 1;
                hwOpt->args[0].type = SCE_KERNEL_REBOOT_ARGTYPE_FILENAME;
                hwOpt->args[1].argp = opt->argp;
                hwOpt->args[1].args = strlen(opt->argp) + 1;
                hwOpt->args[1].type = SCE_KERNEL_REBOOT_ARGTYPE_EMU;
                hwOpt->curArgs = 2;
                hwOpt->unk36 = 0;
            } else {
                // 300C
                hwOpt->args[0].argp = opt->argp;
                hwOpt->args[0].args = strlen(opt->argp) + 1;
                hwOpt->args[0].type = argType;
                hwOpt->curArgs = 1;
                hwOpt->unk36 = 0;
                if (opt->apiType == SCE_INIT_APITYPE_EMU_EBOOT_MS || opt->apiType == SCE_INIT_APITYPE_EMU_BOOT_MS
                 || opt->apiType == SCE_INIT_APITYPE_EMU_EBOOT_EF || opt->apiType == SCE_INIT_APITYPE_EMU_BOOT_EF) {
                    // 3064
                    SceUID id = sceKernelGetChunk(3);
                    ret = id;
                    if (id >= 0) {
                        void *addr = sceKernelGetBlockHeadAddr(id);
                        hwOpt->args[1].argp = addr;
                        hwOpt->args[1].args = strlen(addr) + 1;
                        hwOpt->args[1].type = SCE_KERNEL_REBOOT_ARGTYPE_EMU;
                        hwOpt->curArgs++;
                    }
                }
                // 30AC
                if ((opt->apiType == SCE_INIT_APITYPE_NPDRM_MS) || (opt->apiType == SCE_INIT_APITYPE_NPDRM_EF)) {
                    hwOpt->args[1].argp = opt->npDrmArg;
                    hwOpt->args[1].args = sizeof *opt->npDrmArg;
                    hwOpt->args[1].type = SCE_KERNEL_REBOOT_ARGTYPE_NPDRM;
                    hwOpt->curArgs++;
                    SceUID id = sceKernelGetChunk(3);
                    ret = id;
                    if (id >= 0) {
                        void *addr = sceKernelGetBlockHeadAddr(id);
                        hwOpt->args[2].argp = addr;
                        hwOpt->args[2].args = strlen(addr) + 1;
                        hwOpt->args[2].type = SCE_KERNEL_REBOOT_ARGTYPE_EMU;
                        hwOpt->curArgs++;
                    }
                }
            }
            // 2FF4
            // 2FF8
            // 2FFC
            copyArgsToRebootParam(hwOpt, opt->vshParam);
        }
    } else if (argType == SCE_KERNEL_REBOOT_ARGTYPE_KERNEL) {
        // 2E38
        s32 *ptr = NULL;
        if (opt->vshParam->vshmainArgs == 0) {
            // 2ED4
            SceUID id = sceKernelGetChunk(0);
            if (id > 0) {
                blkInfo.size = 56;
                sceKernelQueryMemoryBlockInfo(id, &blkInfo);
                hwOpt->args[hwOpt->curArgs].argp = blkInfo.unk40;
                hwOpt->args[hwOpt->curArgs].args = blkInfo.unk44;
                hwOpt->args[hwOpt->curArgs].type = SCE_KERNEL_REBOOT_ARGTYPE_DEFAULT;
                hwOpt->unk40 = hwOpt->curArgs;
                hwOpt->curArgs++;
                ptr = hwOpt->args[hwOpt->curArgs].argp;
                ptr[0] = hwOpt->args[hwOpt->curArgs].args;
                ptr[1] = 32;
            }
        } else {
            hwOpt->args[hwOpt->curArgs].argp = opt->vshParam->vshmainArgp;
            hwOpt->args[hwOpt->curArgs].args = opt->vshParam->vshmainArgs;
            hwOpt->args[hwOpt->curArgs].type = SCE_KERNEL_REBOOT_ARGTYPE_DEFAULT;
            hwOpt->unk40 = hwOpt->curArgs;
            hwOpt->curArgs++;
            ptr = opt->vshParam->vshmainArgp;
            ptr[0] = opt->vshParam->vshmainArgs;
            ptr[1] = 32;
        }

        // 2E94
        ptr[2] = (s32)opt->opt4;
        if (opt->opt4 == 0) {
            // 2EB4
            s32 v;
            if ((v = ptr[6]) == 0)
                if ((v = ptr[5]) == 0)
                    v = ptr[4];
            // 2ECC
            ptr[3] = v;
        } else
            ptr[3] = 0;

        // 2EA4
        ptr[6] = 0;
        ptr[4] = 0;
        ptr[5] = 0;
    }

    // 2C84
    if (g_suppArgSet) {
        hwOpt->args[hwOpt->curArgs].argp = g_suppArgp;
        hwOpt->args[hwOpt->curArgs].args = g_suppArgs;
        hwOpt->args[hwOpt->curArgs].type = g_suppArgType;
        hwOpt->curArgs++;
    }

    // 2CE0
    if ((opt->vshParam->flags & 0x10000) == 0)
        *(s32*)(hwOpt->startAddr + hwOpt->unk12 * 32 - 12) = opt->apiType;

    // 2D04
    fixupArgsAddr(hwOpt, opt->vshParam);
    sceKernelSetDdrMemoryProtection(0x88400000, 0x400000, 12);
    if (opt->apiType == SCE_INIT_APITYPE_DEBUG)
        return ret;
    sceKernelMemset((void*)0x88600000, 0, 0x200000);
    ret = decodeKL4E((void*)0x88600000, 0x200000, (void*)g_reboot + 4, 0);
    if (ret < 0) {
        // 2DD0
        sceKernelCpuSuspendIntr();
        *(s32*)(0xBC100040) = (*(s32*)(0xBC100040) & 0xFFFFFFFC) | 1;
        sceKernelMemset((void*)0x88600000, 0, 0x200000);
        sceKernelMemset(g_reboot, 0, sizeof g_reboot);
        memset((void*)0xBFC00000, 0, 4096);
        for (;;)
            ;
    }
    sceKernelDcacheWritebackAll();
    sceKernelIcacheInvalidateAll();
    UtilsForKernel_39FFB756(0);
    Kprintf("***** reboot start *****\n");
    Kprintf("\n\n\n");
    s32 (*reboot)(SceKernelRebootParam *, SceKernelLoadExecVSHParam *, s32, s32) = (void*)0x88600000;
    reboot(hwOpt, opt->vshParam, opt->apiType, rand);
    return ret;
}

// 0x32FC
SceKernelRebootParam *makeRebootParam()
{
    s32 base, startAddr, args, endAddr;
    SceKernelRebootParam *opt;
    if (sceKernelGetModel() != 0)
        base = 0x8B800000;
    else
        base = 0x88400000;

    opt       = (SceKernelRebootParam*)UPALIGN64(base + 0x10); // Size 0x1000
    startAddr = UPALIGN64((s32)opt  + 0x01000); // Size 0x1c000
    args      = UPALIGN64(startAddr + 0x1c000); // Size 0x380 (32 arguments max)
    endAddr   = UPALIGN64(args      + 0x00380);

    sceKernelMemset((void*)base, 0, endAddr - base);

    opt->startAddr = (void*)startAddr;
    opt->args      = (void*)args;
    opt->endAddr   = (void*)endAddr;
    opt->curArgs   = 0;
    opt->unk24     = 0;
    opt->model     = sceKernelGetModel();
    opt->unk60     = 0;
    opt->dipswLo   = sceKernelDipswLow32();
    opt->dipswHi   = sceKernelDipswHigh32();
    opt->unk48     = 0;

    if (sceKernelDipsw(30) != 1)
        opt->dipswHi &= 0xFAFFFFFF;

    opt->addr = (void*)0x88000000;
    opt->memorySize = sceKernelSysMemRealMemorySize();
    opt->unk40 = -1;
    opt->unk36 = -1;
    opt->cpTime = sceKernelDipswCpTime();
    return opt;
}

