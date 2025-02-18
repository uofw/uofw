/* Copyright (C) 2011 - 2015 The uOFW team
   See the file COPYING for copying permission.
*/

#include <common_imp.h>
#include <iofilemgr_kernel.h>
#include <modulemgr_init.h>
#include <modulemgr_kernel.h>
#include <modulemgr_options.h>
#include <sysmem_kdebug.h>

#include "loadModuleChecks_inline.h"
#include "modulemgr_int.h"
#include "override.h"

static SceUID _loadModuleByBufferID(SceModuleMgrParam *pModParams, const SceKernelLMOption *pOption);
static void _setupForLoadModuleBuffer(SceModuleMgrParam *pModParams, u32 apiType, void *base, 
    SceSize modSize, u32 unk124);

/**
* Load a module by path specifying the api type
*
* @param apiType The api type of the module
* @param path A pointer to a '\0' terminated string containing the path to the module
* @param flag Unused, pass 0
* @param pOption A pointer to a SceKernelLMOption structure, which holds various options about the way to load the module.
*             Pass NULL if you don't want to specify any option.
*
* @return SCE_ERROR_OK on success, < 0 on error.
* @return SCE_ERROR_KERNEL_CANNOT_BE_CALLED_FROM_INTERRUPT if function was called in an interruption.
* @return SCE_ERROR_KERNEL_ILLEGAL_PERMISSION_CALL if function was called from a user context
* @return SCE_ERROR_KERNEL_ILLEGAL_ADDR if the provided pointers are NULL or can't be accessed from the current context.
* @return SCE_ERROR_KERNEL_UNKNOWN_MODULE_FILE if the path contains a '%' (protection against formatted strings attack)
* @return SCE_ERROR_KERNEL_ILLEGAL_SIZE if SdkVersion >= 2.80 and opt->size != sizeof(SceKernelLMOption)
* @return One of the errors of sceIoOpen() if failed
* @return One of the errors of sceIoIoctl() if failed
*/
// Subroutine ModuleMgrForKernel_2B7FC10D - Address 0x000004A8            
SceUID sceKernelLoadModuleForLoadExecForUser(s32 apiType, const char *file, s32 flag,
    const SceKernelLMOption *pOption)
{
    s32 oldK1;
    s32 status;
    s32 ioctlCmd;
    SceUID fd;
    SceModuleMgrParam modParams;

    (void)flag;

    oldK1 = pspShiftK1(); //0x000004B4

    if (sceKernelIsIntrContext()) { //0x000004E0
        pspSetK1(oldK1);
        return SCE_ERROR_KERNEL_CANNOT_BE_CALLED_FROM_INTERRUPT;
    }

    //0x000004EC - 0x000006A8
    if ((status = _checkCallConditionKernel()) < 0 || (status = _checkPathConditions(file)) < 0
            || (status = _checkLMOptionConditions(pOption)) < 0) {
        pspSetK1(oldK1);
        return status;
    }

    fd = sceIoOpen(file, SCE_O_ENCRYPTED | SCE_O_RDONLY, SCE_STM_RWXUGO); //0x00000528
    if (fd < 0) { //0x00000534
        pspSetK1(oldK1);
        return fd;
    }

    switch (apiType) { //0x00000568
    case SCE_EXEC_FILE_APITYPE_GAME_EBOOT:
    case SCE_EXEC_FILE_APITYPE_EMU_EBOOT_MS:
    case SCE_EXEC_FILE_APITYPE_EMU_EBOOT_EF:
        ioctlCmd = 0x208010; // 0x00000568 & 0x0000056C
        break;
    default: // 0x00000540
        ioctlCmd = 0x208011; //0x00000630 & 0x00000638
        break;
    }
    status = sceIoIoctl(fd, ioctlCmd, NULL, 0, NULL, 0); //0x00000580
    if (status < SCE_ERROR_OK) { //0x0000058C
        sceIoClose(fd); //0x00000600
        pspSetK1(oldK1);
        return SCE_ERROR_KERNEL_PROHIBIT_LOADMODULE_DEVICE;
    }

    pspClearMemory32(&modParams, sizeof modParams); //0x000005A0

    modParams.apiType = apiType; // 0x000005A8
    modParams.modeFinish = CMD_RELOCATE_MODULE; // 0x000005B4
    modParams.modeStart = CMD_LOAD_MODULE; // 0x000005C0
    modParams.fileBase = NULL; // 0x000005CC
    modParams.fd = fd; // 0x000005D8
    modParams.unk124 = 0; // 0x000005E0

    status = sceIoIoctl(fd, 0x208081, NULL, 0, NULL, 0); //0x000005DC
    if (status >= SCE_ERROR_OK) //0x000005E4
        modParams.unk100 = 0x10; // 0x000005EC

    status = _loadModuleByBufferID(&modParams, pOption); //0x000005F4

    sceIoClose(fd); //0x00000600
    pspSetK1(oldK1);
    return status;
}

/**
* Load a module by path
*
* @param path A pointer to a '\0' terminated string containing the path to the module
* @param flag Unused, pass 0
* @param pOption A pointer to a SceKernelLMOption structure, which holds various options about the way to load the module. Pass NULL if you don't want to specify any option.
*
* @return SCE_ERROR_OK on success, < 0 on error.
* @return SCE_ERROR_KERNEL_CANNOT_BE_CALLED_FROM_INTERRUPT if function was called in an interruption.
* @return SCE_ERROR_KERNEL_ILLEGAL_PERMISSION_CALL if function was not called from a user context
* @return SCE_ERROR_KERNEL_ILLEGAL_ADDR if the provided pointers are NULL or can't be accessed from the current context.
* @return SCE_ERROR_KERNEL_UNKNOWN_MODULE_FILE if the path contains a '%' (protection against formatted strings attack)
* @return SCE_ERROR_KERNEL_ILLEGAL_SIZE if SdkVersion >= 2.71 and opt->size != sizeof(SceKernelLMOption)
* @return One of the errors of sceIoOpen() if failed
* @return One of the errors of sceIoIoctl() if failed
*/
// Subroutine ModuleMgrForUser_977DE386 - Address 0x000006B8 
SceUID sceKernelLoadModule(const char *path, s32 flag, const SceKernelLMOption *pOption)
{
    s32 oldK1;
    s32 status;
    SceUID fd;
    SceModuleMgrParam modParams;

    (void)flag;

    oldK1 = pspShiftK1(); // 0x000006C4

    // Cannot be called in an interruption
    if (sceKernelIsIntrContext()) { // 0x000006E0
        pspSetK1(oldK1);
        return SCE_ERROR_KERNEL_CANNOT_BE_CALLED_FROM_INTERRUPT;
    }

    //0x000006F8 - 0x0000087C
    if ((status = _checkCallConditionUser()) < 0 || (status = _checkPathConditions(path)) < 0
            || (status = _checkLMOptionConditions(pOption)) < 0) {
        pspSetK1(oldK1);
        return status;
    }

    fd = sceIoOpen(path, SCE_O_ENCRYPTED | SCE_O_RDONLY, SCE_STM_RWXUGO); // 0x00000734
    if (fd < 0) { // 0x00000740
        pspSetK1(oldK1);
        return fd;
    }
#ifndef INSTALLER
    status = sceIoIoctl(fd, 0x208001, NULL, 0, NULL, 0); // 0x00000760
#else
    status = 0;
#endif
    if (status < 0) { // 0x0000076C
        sceIoClose(fd); // 0x000007E0
        pspSetK1(oldK1);
        return SCE_ERROR_KERNEL_PROHIBIT_LOADMODULE_DEVICE;
    }

    pspClearMemory32(&modParams, sizeof modParams); // 0x00000784

    modParams.modeFinish = CMD_RELOCATE_MODULE; // 0x00000790
    modParams.apiType = SCE_EXEC_FILE_APITYPE_MODULE_USER; // 0x0000079C
    modParams.modeStart = CMD_LOAD_MODULE; // 0x000007A8
    modParams.fileBase = NULL; // 0x000007B4
    modParams.fd = fd; // 0x000007BC
    modParams.unk124 = 0; // 0x000007C4
#ifndef INSTALLER
    status = sceIoIoctl(fd, 0x208081, NULL, 0, NULL, 0); // 0x000007C0
#else
    status = 0;
#endif
    if (status >= 0) // 0x000007C8
        modParams.unk100 = 0x10; // 0x000007CC

    status = _loadModuleByBufferID(&modParams, pOption); // 0x000007D4

    sceIoClose(fd); // 0x000007E0
    pspSetK1(oldK1);
    return status;
}

/**
* Load a module by file descriptor
*
* @param inputId The file descriptor that was obtained when opening the module with sceIoOpen()
* @param flag Unused, pass 0
* @param pOption A pointer to a SceKernelLMOption structure, which holds various options about the way to load the module. Pass NULL if you don't want to specify any option.
*
* @return SCE_ERROR_OK on success, < 0 on error.
* @return SCE_ERROR_KERNEL_CANNOT_BE_CALLED_FROM_INTERRUPT if function was called in an interruption.
* @return SCE_ERROR_KERNEL_ILLEGAL_PERMISSION_CALL if function was not called from a user context
* @return SCE_ERROR_KERNEL_ILLEGAL_ADDR if the pointer to SceKernelLMOption can't be accessed from the current context.
* @return SCE_ERROR_KERNEL_ILLEGAL_SIZE if SdkVersion >= 2.80 and opt->size != sizeof(SceKernelLMOption)
* @return One of the errors of sceIoValidateFd() if failed
* @return SCE_ERROR_KERNEL_PROHIBIT_LOADMODULE_DEVICE if sceIoIoctl() failed
*/
// Subroutine ModuleMgrForUser_B7F46618 - Address 0x0000088C
SceUID sceKernelLoadModuleByID(SceUID inputId, s32 flag, const SceKernelLMOption *pOption)
{
    s32 oldK1;
    s32 status;
    SceModuleMgrParam modParams;

    (void)flag;

    oldK1 = pspShiftK1(); //0x00000898

    if (sceKernelIsIntrContext()) { //0x000008C0
        pspSetK1(oldK1);
        return SCE_ERROR_KERNEL_CANNOT_BE_CALLED_FROM_INTERRUPT;
    }

    //0x000008CC - 0x00000944
    if ((status = _checkCallConditionUser()) < 0 || (status = _checkLMOptionConditions(pOption)) < 0) {
        pspSetK1(oldK1);
        return status;
    }

    status = sceIoValidateFd(inputId, 4); //0x00000950
    if (status < SCE_ERROR_OK) { //0x00000958
        pspSetK1(oldK1);
        return status;
    }

    status = sceIoIoctl(inputId, 0x208001, NULL, 0, NULL, 0); // 0x00000978
    if (status < SCE_ERROR_OK) { // 0x00000984
        pspSetK1(oldK1);
        return SCE_ERROR_KERNEL_PROHIBIT_LOADMODULE_DEVICE;
    }

    pspClearMemory32(&modParams, sizeof modParams); //0x0000099C

    modParams.apiType = SCE_EXEC_FILE_APITYPE_MODULE_USER; // 0x000009C8
    modParams.modeFinish = CMD_RELOCATE_MODULE; // 0x000009C4
    modParams.modeStart = CMD_LOAD_MODULE; // 0x000009CC
    modParams.fileBase = NULL; // 0x000005CC
    modParams.fd = inputId; // 0x000005D8
    modParams.unk124 = 0; // 0x000009DC

    status = sceIoIoctl(inputId, 0x208081, NULL, 0, NULL, 0); //0x000009D8
    if (status >= SCE_ERROR_OK) //0x000009E0
        modParams.unk100 = 0x10; // 0x000009E4

    status = _loadModuleByBufferID(&modParams, pOption); //0x000009EC

    pspSetK1(oldK1);
    return status;
}

/**
* Load a module by path specifying a block and an offset
*
* @param path The file descriptor that was obtained when opening the module with sceIoOpen()
* @param block Memory block where the module will be loaded
* @param Aligned-to-64-offset (from the memory block) where the module will be loaded
*
* @return SCE_ERROR_OK on success, < 0 on error.
* @return SCE_ERROR_KERNEL_CANNOT_BE_CALLED_FROM_INTERRUPT if function was called in an interruption.
* @return SCE_ERROR_KERNEL_ILLEGAL_PERMISSION_CALL if function was not called from a user context
* @return SCE_ERROR_KERNEL_ILLEGAL_ADDR if the path pointer is NULL or can't be accessed from the current context, or if the address corresponding to the block can't be accessed from the current context.
* @return SCE_ERROR_KERNEL_UNKNOWN_MODULE_FILE if the path contains a '%' (protection against formatted strings attack)
* @return SCE_ERROR_KERNEL_INVALID_ARGUMENT if the offset is incorrect or not aligned to 64
* @return One of the errors of sceIoOpen() if failed
* @return One of the errors of sceIoValidateFd() if failed
* @return SCE_ERROR_KERNEL_PROHIBIT_LOADMODULE_DEVICE if sceIoIoctl() failed
*/
// Subroutine ModuleMgrForUser_E4C4211C - Address 0x000009FC
SceUID sceKernelLoadModuleWithBlockOffset(const char *path, SceUID blockId, SceOff offset)
{
    s32 oldK1;
    s32 status;
    SceUID fd;
    SceSysmemMemoryBlockInfo blkInfo;
    SceModuleMgrParam modParams;

    oldK1 = pspShiftK1();  //0x00000898

    if (sceKernelIsIntrContext()) { //0x00000A08
        pspSetK1(oldK1);
        return SCE_ERROR_KERNEL_CANNOT_BE_CALLED_FROM_INTERRUPT;
    }

    //0x00000A50 - 0x00000AB8
    if ((status = _checkCallConditionUser()) < 0 || (status = _checkPathConditions(path)) < 0) {
        pspSetK1(oldK1);
        return status;
    }

    blkInfo.size = sizeof(SceSysmemMemoryBlockInfo);
    status = sceKernelQueryMemoryBlockInfo(blockId, &blkInfo); // 0x00000ACC
    if (status < SCE_ERROR_OK) { // 0x00000AD4
        pspSetK1(oldK1);
        return status;
    }
    //0x00000B04 - 0x00000B3C
    status = _checkMemoryBlockInfoConditions(&blkInfo, offset);
    if (status < SCE_ERROR_OK) {
        pspSetK1(oldK1);
        return status;
    }

    fd = sceIoOpen(path, SCE_O_ENCRYPTED | SCE_O_RDONLY, SCE_STM_RWXUGO); //0x00000B5C
    if (fd < 0) { //0x00000B68
        pspSetK1(oldK1);
        return fd;
    }
    status = sceIoIoctl(fd, 0x208001, NULL, 0, NULL, 0); // 0x00000B88
    if (status < SCE_ERROR_OK) { // 0x00000B94
        sceIoClose(fd);
        pspSetK1(oldK1);
        return SCE_ERROR_KERNEL_PROHIBIT_LOADMODULE_DEVICE;
    }

    pspClearMemory32(&modParams, sizeof modParams); //0x00000BAC

    modParams.apiType = SCE_EXEC_FILE_APITYPE_MODULE_USER; // 0x00000BC4
    modParams.modeFinish = CMD_RELOCATE_MODULE; // 0x00000BB8
    modParams.modeStart = CMD_LOAD_MODULE; // 0x00000BD0
    modParams.fileBase = NULL; // 0x00000BDC
    modParams.fd = fd; // 0x00000BE4
    modParams.unk124 = 0; // 0x00000BEC

    status = sceIoIoctl(fd, 0x208081, NULL, 0, NULL, 0); //0x00000BE8
    if (status >= SCE_ERROR_OK) //0x00000BF0
        modParams.unk100 = 0x10; // 0x00000BF4

    // 0x00000BFC
    modParams.externMemBlockIdUser = blockId;
    modParams.memBlockOffset = offset;

    status = _loadModuleByBufferID(&modParams, NULL); //0x00000C08

    sceIoClose(fd); //0x00000C14
    pspSetK1(oldK1);
    return status;
}

/**
* Load a module by file descriptor specifying a block and an offset
*
* @param inputId The file descriptor that was obtained when opening the module with sceIoOpen()
* @param block Memory block where the module will be loaded
* @param Aligned-to-64-offset (from the memory block) where the module will be loaded

* @return SCE_ERROR_OK on success, < 0 on error.
* @return SCE_ERROR_KERNEL_CANNOT_BE_CALLED_FROM_INTERRUPT if function was called in an interruption.
* @return SCE_ERROR_KERNEL_ILLEGAL_PERMISSION_CALL if function was not called from a user context
* @return SCE_ERROR_KERNEL_ILLEGAL_ADDR if the address corresponding to the block can't be accessed from the current context.
* @return SCE_ERROR_KERNEL_INVALID_ARGUMENT if the offset is incorrect or not aligned to 64
* @return One of the errors of sceIoValidateFd() if failed
* @return SCE_ERROR_KERNEL_PROHIBIT_LOADMODULE_DEVICE if sceIoIoctl() failed
*/
// Subroutine ModuleMgrForUser_FBE27467 - Address 0x00000C34
SceUID sceKernelLoadModuleByIDWithBlockOffset(SceUID inputId, SceUID blockId, SceOff offset)
{
    s32 oldK1;
    s32 status;
    SceSysmemMemoryBlockInfo blkInfo;
    SceModuleMgrParam modParams;

    oldK1 = pspShiftK1(); // 0x00000C40

    if (sceKernelIsIntrContext()) { // 0x00000C6C
        pspSetK1(oldK1);
        return SCE_ERROR_KERNEL_CANNOT_BE_CALLED_FROM_INTERRUPT;
    }
    status = _checkCallConditionUser(); // 0x00000C84
    if (status < 0) {
        pspSetK1(oldK1);
        return status;
    }

    blkInfo.size = sizeof(SceSysmemMemoryBlockInfo);
    status = sceKernelQueryMemoryBlockInfo(blockId, &blkInfo); // 0x00000CD0
    if (status < SCE_ERROR_OK) { // 0x00000AD4
        pspSetK1(oldK1);
        return status;
    }
    //0x00000D00 - 0x00000D38
    status = _checkMemoryBlockInfoConditions(&blkInfo, offset);
    if (status < SCE_ERROR_OK) {
        pspSetK1(oldK1);
        return status;
    }

    status = sceIoValidateFd(inputId, 4); //0x00000D50
    if (status < SCE_ERROR_OK) { //0x00000D58
        pspSetK1(oldK1);
        return status;
    }

    status = sceIoIoctl(inputId, 0x208001, NULL, 0, NULL, 0); // 0x00000D78
    if (status < SCE_ERROR_OK) { // 0x00000D84
        pspSetK1(oldK1);
        return SCE_ERROR_KERNEL_PROHIBIT_LOADMODULE_DEVICE;
    }

    pspClearMemory32(&modParams, sizeof modParams); //0x00000D9C

    modParams.apiType = SCE_EXEC_FILE_APITYPE_MODULE_USER; // 0x00000DB4
    modParams.modeFinish = CMD_RELOCATE_MODULE; // 0x00000DA8
    modParams.modeStart = CMD_LOAD_MODULE; // 0x00000DC0
    modParams.fileBase = NULL; // 0x00000DCC
    modParams.fd = inputId; // 0x00000DD4
    modParams.unk124 = 0; // 0x00000DDC

    status = sceIoIoctl(inputId, 0x208081, NULL, 0, NULL, 0); //0x00000DD8
    if (status >= SCE_ERROR_OK) //0x00000DE0
        modParams.unk100 = 0x10; // 0x00000DE4

    // 0x00000DEC
    modParams.externMemBlockIdUser = blockId;
    modParams.memBlockOffset = offset;

    status = _loadModuleByBufferID(&modParams, NULL); //0x00000DF8

    pspSetK1(oldK1);
    return status;
}

/**
* Load a DNAS module by path and secureInstallId
*
* @param path The file descriptor that was obtained when opening the module with sceIoOpen()
* @param secureInstallId A pointer to a secure installation identifier string, which is an encryption key used to decrypt the module, example: "123456789abcdef123456789abcdef12". It is often used in games that encrypt their modules (and other files) to prevent unauthorized access. This makes reverse engineering slightly harder because one needs to find the keys first.
* @param flag Unused, pass 0
* @param opt A pointer to a SceKernelLMOption structure, which holds various options about the way to load the module. Pass NULL if you don't want to specify any option.

* @return SCE_ERROR_OK on success, < 0 on error.
* @return SCE_ERROR_KERNEL_CANNOT_BE_CALLED_FROM_INTERRUPT if function was called in an interruption.
* @return SCE_ERROR_KERNEL_ILLEGAL_PERMISSION_CALL if function was not called from a user context
* @return SCE_ERROR_KERNEL_ILLEGAL_ADDR if the path or secureInstallId or pOption is NULL or can't be accessed from the current context.
* @return SCE_ERROR_KERNEL_UNKNOWN_MODULE_FILE if the path contains a '%' (protection against formatted strings attack)
* @return SCE_ERROR_KERNEL_INVALID_ARGUMENT if the offset is incorrect or not aligned to 64
* @return SCE_ERROR_KERNEL_ILLEGAL_SIZE if SdkVersion >= 2.80 and opt->size != sizeof(SceKernelLMOption)
* @return One of the errors of sceIoOpen() if failed
* @return One of the errors of sceIoIoctl() if failed
* @return SCE_ERROR_KERNEL_PROHIBIT_LOADMODULE_DEVICE if sceIoIoctl() failed
*/
// Subroutine ModuleMgrForUser_FEF27DC1 - Address 0x00000E18
SceUID sceKernelLoadModuleDNAS(const char *path, const char *secureInstallId, s32 flag,
    const SceKernelLMOption *pOption)
{
    s32 oldK1;
    s32 status;
    SceUID fd;
    SceModuleMgrParam modParams;

    (void)flag;

    oldK1 = pspShiftK1(); // 0x00000E24

    // Cannot be called in an interrupt
    if (sceKernelIsIntrContext()) { // 0x00000E48
        pspSetK1(oldK1);
        return SCE_ERROR_KERNEL_CANNOT_BE_CALLED_FROM_INTERRUPT;
    }

    //0x00000E60 - 0x00001050
    if ((status = _checkCallConditionUser()) < 0 || (status = _checkPathConditions(path)) < 0
            || (status = _checkSecureInstalledIdConditions(secureInstallId)) < 0
            || (status = _checkLMOptionConditions(pOption)) < 0) {
        pspSetK1(oldK1);
        return status;
    }

    fd = sceIoOpen(path, SCE_O_FGAMEDATA | SCE_O_ENCRYPTED | SCE_O_RDONLY, SCE_STM_RWXUGO); // 0x00000EB8
    if (fd < 0) { // 0x00000EC4
        pspSetK1(oldK1);
        return fd;
    }

    status = sceIoIoctl(fd, SCE_GAMEDATA_SET_SECURE_INSTALL_ID, (void *)secureInstallId, SCE_SECURE_INSTALL_ID_LEN, NULL, 0); // 0x00000EE4
    if (status < SCE_ERROR_OK) { // 0x00000EEC
        sceIoClose(fd); // 0x00000FDC
        pspSetK1(oldK1);
        return status;
    }

    status = sceIoIoctl(fd, 0x208002, NULL, 0, NULL, 0); // 0x00000F14
    if (status < SCE_ERROR_OK) { // 0x00000F20
        sceIoClose(fd); //0x00000FA8
        pspSetK1(oldK1);
        return SCE_ERROR_KERNEL_PROHIBIT_LOADMODULE_DEVICE;
    }

    pspClearMemory32(&modParams, sizeof modParams); // 0x00000F34

    modParams.apiType = SCE_EXEC_FILE_APITYPE_MODULE_DNAS; // 0x00000F44
    modParams.modeFinish = CMD_RELOCATE_MODULE; // 0x00000F50
    modParams.modeStart = CMD_LOAD_MODULE; // 0x00000F5C
    modParams.fileBase = NULL; // 0x00000F68
    modParams.fd = fd; // 0x00000F70
    modParams.unk124 = 0; // 0x00000F78

    status = sceIoIoctl(fd, 0x208081, NULL, 0, NULL, 0); //0x00000F74
    if (status >= SCE_ERROR_OK) //0x00000F7C
        modParams.unk100 = 0x10; // 0x00000DE4

    memcpy(&modParams.secureInstallId, secureInstallId, SCE_SECURE_INSTALL_ID_LEN); //0x00000F90

    status = _loadModuleByBufferID(&modParams, pOption); // 0x00000F9C

    sceIoClose(fd); // 0x00000FA8
    pspSetK1(oldK1);
    return status;
}

/**
* Load an NPDRM SPRX module, sceNpDrmSetLicenseeKey() needs to be called first in order to set the key
*
* @param path A pointer to a '\0' terminated string containing the path to the module
* @param flag Unused, pass 0
* @param pOption A pointer to a SceKernelLMOption structure, which holds various options about the way to load the module. Pass NULL if you don't want to specify any option.

* @return SCE_ERROR_OK on success, < 0 on error.
* @return SCE_ERROR_KERNEL_CANNOT_BE_CALLED_FROM_INTERRUPT if function was called in an interruption.
* @return SCE_ERROR_KERNEL_ILLEGAL_PERMISSION_CALL if function was not called from a user context
* @return SCE_ERROR_KERNEL_ILLEGAL_ADDR if the path is NULL, or path/pOption can't be accessed from the current context.
* @return SCE_ERROR_KERNEL_UNKNOWN_MODULE_FILE if the path contains a '%' (protection against formatted strings attack)
* @return SCE_ERROR_KERNEL_ILLEGAL_SIZE if SdkVersion >= 2.80 and opt->size != sizeof(SceKernelLMOption)
* @return One of the errors of sceIoOpen() if failed
* @return SCE_ERROR_KERNEL_ERROR If the callback npDrmGetModuleKeyFunction in the g_ModuleManager structure is NULL
* @return One of the errors of sceIoIoctl() if failed
* @return SCE_ERROR_KERNEL_PROHIBIT_LOADMODULE_DEVICE if sceIoIoctl() failed
*
* @see sceNpDrmSetLicenseeKey()
*/
// Subroutine ModuleMgrForUser_F2D8D1B4 - Address 0x00001060 
SceUID sceKernelLoadModuleNpDrm(const char *path, s32 flag, const SceKernelLMOption *pOption)
{
    s32 oldK1;
    SceUID fd;
    s32 status;
    char secureInstallId[SCE_NPDRM_LICENSEE_KEY_LEN];
    SceNpDrm npDrmData;
    SceModuleMgrParam modParams;

    (void)flag;

    oldK1 = pspShiftK1(); // 0x0000106C

    // Cannot be called in an interruption
    if (sceKernelIsIntrContext()) { // 0x00001094
        pspSetK1(oldK1);
        return SCE_ERROR_KERNEL_CANNOT_BE_CALLED_FROM_INTERRUPT;
    }

    //0x000010A0 - 0x0000122C
    if ((status = _checkCallConditionUser()) < 0 || (status = _checkPathConditions(path)) < 0
            || (status = _checkLMOptionConditions(pOption)) < 0) {
        pspSetK1(oldK1);
        return status;
    }

    fd = sceIoOpen(path, SCE_O_ENCRYPTED | SCE_O_RDONLY, SCE_STM_RWXUGO); // 0x000010DC
    if (fd < 0) { // 0x000010E8
        pspSetK1(oldK1);
        return fd;
    }

    if (g_ModuleManager.npDrmGetModuleKeyFunction == NULL) { //0x000010FC
        sceIoClose(fd);
        pspSetK1(oldK1);
        return SCE_ERROR_KERNEL_ERROR;
    }
    status = g_ModuleManager.npDrmGetModuleKeyFunction(fd, &secureInstallId, &npDrmData); // 0x00001110
    if (status < 0) { // 0x00001118
        sceIoClose(fd);
        pspSetK1(oldK1);
        return status;
    }

    sceIoLseek(fd, npDrmData.fileOffset, SCE_SEEK_SET); // 0x0000112C

    status = sceIoIoctl(fd, 0x208002, NULL, 0, NULL, 0); // 0x0000114C
    if (status < SCE_ERROR_OK) { // 0x00001158
        sceIoClose(fd);
        pspSetK1(oldK1);
        return SCE_ERROR_KERNEL_PROHIBIT_LOADMODULE_DEVICE;
    }

    pspClearMemory32(&modParams, sizeof modParams); // 0x0000116C

    modParams.apiType = SCE_EXEC_FILE_APITYPE_MODULE_NPDRM; // 0x0000117C
    modParams.modeFinish = CMD_RELOCATE_MODULE; // 0x00001188
    modParams.modeStart = CMD_LOAD_MODULE; // 0x00001194
    modParams.fileBase = NULL; // 0x000011A0
    modParams.fd = fd; // 0x000011A8
    modParams.unk124 = 0;

    status = sceIoIoctl(fd, 0x208081, NULL, 0, NULL, 0); // 0x000011AC
    if (status >= SCE_ERROR_OK) // 0x000011B4
        modParams.unk100 = 0x10; // 0x000011C0

    memcpy(modParams.secureInstallId, secureInstallId, SCE_SECURE_INSTALL_ID_LEN); //0x000011C8

    status = _loadModuleByBufferID(&modParams, pOption); // 0x000011D4

    sceIoClose(fd);
    pspSetK1(oldK1);
    return status;
}

/**
* Load a module using the MS API.
*
* @param path Path of the module to load.
* @param flag Unused.
* @param pOption A pointer to a SceKernelLMOption structure, which holds various options about the way to load the module.
*              Pass NULL if you don't want to specify any option. When specifying the SceKernelLMOption structure for
*              the pOpt argument, set the size of the structure in SceKernelLMOption.size.

* @return SCE_ERROR_OK on success, < 0 on error.
*/
// Subroutine ModuleMgrForUser_710F61B5 - Address 0x0000128C
SceUID sceKernelLoadModuleMs(const char *path, s32 flag, const SceKernelLMOption *pOption)
{
    s32 oldK1;
    SceUID fd;
    s32 status;
    SceModuleMgrParam modParams;

    (void)flag;

    oldK1 = pspShiftK1(); // 0x00001298

    // Cannot be called in an interruption
    if (sceKernelIsIntrContext()) { // 0x000012B0
        pspSetK1(oldK1);
        return SCE_ERROR_KERNEL_CANNOT_BE_CALLED_FROM_INTERRUPT;
    }

    if ((status = _checkCallConditionUser()) < 0) { //0x000012C4
        pspSetK1(oldK1);
        return status;
    }

    /* Verify if user can use the MS API. */
    status = sceKernelGetUserLevel();
    if (status != SCE_USER_LEVEL_MS) { //0x00001300
        pspSetK1(oldK1);
        return SCE_ERROR_KERNEL_ILLEGAL_PERMISSION_CALL;
    }

    //0x0000130C - 0x00001468
    if ((status = _checkPathConditions(path)) < 0 || (status = _checkLMOptionConditions(pOption)) < 0) {
        pspSetK1(oldK1);
        return status;
    }

    fd = sceIoOpen(path, SCE_O_ENCRYPTED | SCE_O_RDONLY, SCE_STM_RWXUGO); // 0x0000133C
    if (fd < 0) { // 0x00001348
        pspSetK1(oldK1);
        return fd;
    }

    status = sceIoIoctl(fd, 0x208002, NULL, 0, NULL, 0); // 0x00001368
    if (status < SCE_ERROR_OK) { // 0x00001374
        sceIoClose(fd);
        pspSetK1(oldK1);
        return SCE_ERROR_KERNEL_PROHIBIT_LOADMODULE_DEVICE;
    }

    pspClearMemory32(&modParams, sizeof modParams); // 0x00001388

    modParams.apiType = SCE_EXEC_FILE_APITYPE_MODULE_MS;
    modParams.modeStart = CMD_LOAD_MODULE; // 0x000013B0
    modParams.modeFinish = CMD_RELOCATE_MODULE; // 0x000013A4
    modParams.fileBase = NULL; // 0x000013BC
    modParams.fd = fd; // 0x000013C4
    modParams.unk124 = 0;

    status = sceIoIoctl(fd, 0x208081, NULL, 0, NULL, 0); // 0x000013C8
    if (status >= SCE_ERROR_OK) // 0x000013D0
        modParams.unk100 = 0x10; // 0x000013D8

    status = _loadModuleByBufferID(&modParams, pOption); // 0x000013E0

    sceIoClose(fd);
    pspSetK1(oldK1);
    return status;
}

/**
* Load a module from a buffer, used for external bootable binaries which were sent then booted using the gamesharing API (allow them to load modules that were sent with the executable)
*
* @param bufSize The size of the buffer containing the module
* @param pBuffer The start address of the buffer containing the module
* @param flag Unused, pass 0
* @param pOption A pointer to a SceKernelLMOption structure, which holds various options about the way to load the module. Pass NULL if you don't want to specify any option.

* @return The ID of the module on success, < 0 on error.
* @return SCE_ERROR_KERNEL_CANNOT_BE_CALLED_FROM_INTERRUPT if function was called in an interruption.
* @return SCE_ERROR_KERNEL_ILLEGAL_PERMISSION_CALL if function was not called from a user context.
* @return SCE_ERROR_KERNEL_ILLEGAL_ADDR if pBuffer/pOption can't be accessed from the current context.
* @return SCE_ERROR_KERNEL_ILLEGAL_SIZE if SdkVersion >= 2.80 and opt->size != sizeof(SceKernelLMOption).
* @return One of the errors of sceIoOpen() if failed
* @return SCE_ERROR_KERNEL_ERROR If the callback npDrmGetModuleKeyFunction in the g_ModuleManager structure is NULL
* @return One of the errors of sceIoIoctl() if failed
* @return SCE_ERROR_KERNEL_PROHIBIT_LOADMODULE_DEVICE if sceIoIoctl() failed
*
* @see sceNpDrmSetLicenseeKey()
*/
// Subroutine ModuleMgrForUser_F9275D98 - Address 0x00001478 
SceUID sceKernelLoadModuleBufferUsbWlan(SceSize size, void *base, s32 flag, const SceKernelLMOption *pOption)
{
    s32 oldK1;
    s32 status;
    SceUID fd;
    SceModuleMgrParam modParams;

    (void)flag;

    oldK1 = pspShiftK1(); // 0x00001484

    // Cannot be called in an interruption
    if (sceKernelIsIntrContext()) { // 0x000012B0
        pspSetK1(oldK1);
        return SCE_ERROR_KERNEL_CANNOT_BE_CALLED_FROM_INTERRUPT;
    }

    if ((status = _checkCallConditionUser()) < 0) { // 0x000014B8
        pspSetK1(oldK1);
        return status;
    }

    if (sceKernelGetUserLevel() != SCE_USER_LEVEL_MS && sceKernelGetUserLevel() != SCE_USER_LEVEL_USBWLAN) { // 0x0000150C
        pspSetK1(oldK1);
        return SCE_ERROR_KERNEL_ILLEGAL_PERMISSION_CALL;
    }

    if (!pspK1DynBufOk(base, size)) { // 0x00001528
        pspSetK1(oldK1);
        return SCE_ERROR_KERNEL_ILLEGAL_ADDR;
    }

    status = _checkLMOptionConditions(pOption); //0x00001530 - 0x00001678
    if (status < SCE_ERROR_OK) {
        pspSetK1(oldK1);
        return status;
    }

    // sub_00008568
    if (_CheckOverride(SCE_EXEC_FILE_APITYPE_MODULE_BUFFER_USBWLAN, base, &fd) == SCE_FALSE) {
        // 0x00001608 - 0x00001640
        _setupForLoadModuleBuffer(&modParams, SCE_EXEC_FILE_APITYPE_MODULE_BUFFER_USBWLAN, base, size, 0);

        // sub_000075B4
        status = _loadModuleByBufferID(&modParams, pOption); // 0x0000163C

        pspSetK1(oldK1);
        return status;
    }

    if (fd < 0) {
        // Congrats Sony, that sure is the best way to deal with errors!
        while (1) {}
    }

    pspClearMemory32(&modParams, sizeof modParams); // 0x00001584

    modParams.apiType = SCE_EXEC_FILE_APITYPE_MODULE_BUFFER_USBWLAN;
    modParams.modeFinish = CMD_RELOCATE_MODULE;
    modParams.modeStart = CMD_LOAD_MODULE;
    modParams.fileBase = NULL;
    modParams.fd = fd;
    modParams.unk124 = 0;

    status = sceIoIoctl(fd, 0x208081, NULL, 0, NULL, 0); // 0x000015C4
    if (status >= SCE_ERROR_OK) // 0x000015CC
        modParams.unk100 = 0x10;

    // sub_000075B4
    status = _loadModuleByBufferID(&modParams, pOption); // 0x000015E4

    sceIoClose(fd); // 0x000015F0
    pspSetK1(oldK1);
    return status;
}

// Subroutine ModuleMgrForKernel_CE0A74A5 - Address 0x00001688
SceUID sceKernelLoadModuleForLoadExecVSHDisc(const char *path, s32 flag, const SceKernelLMOption *pOption)
{
    s32 oldK1;
    SceUID fd;
    s32 status;
    SceModuleMgrParam modParams;

    (void)flag;

    oldK1 = pspShiftK1(); // 0x00001694

    // Cannot be called in an interruption
    if (sceKernelIsIntrContext()) { // 0x000016AC
        pspSetK1(oldK1);
        return SCE_ERROR_KERNEL_CANNOT_BE_CALLED_FROM_INTERRUPT;
    }

    if ((status = _checkCallConditionKernel()) < 0) { //0x000016C4
        pspSetK1(oldK1);
        return status;
    }

    //0x000016D0, 0x000017F8 - 0x00001850
    if ((status = _checkPathConditions(path)) < 0 || (status = _checkLMOptionConditions(pOption)) < 0) {
        pspSetK1(oldK1);
        return status;
    }

    fd = sceIoOpen(path, SCE_O_ENCRYPTED | SCE_O_RDONLY, SCE_STM_RWXUGO); // 0x00001700
    if (fd < 0) { // 0x0000170C
        pspSetK1(oldK1);
        return fd;
    }

    status = sceIoIoctl(fd, 0x208011, NULL, 0, NULL, 0); // 0x0000172C
    if (status < SCE_ERROR_OK) { // 0x00001738
        sceIoClose(fd);
        pspSetK1(oldK1);
        return SCE_ERROR_KERNEL_PROHIBIT_LOADMODULE_DEVICE;
    }

    pspClearMemory32(&modParams, sizeof modParams); // 0x0000174C

    modParams.apiType = SCE_EXEC_FILE_APITYPE_DISC;
    modParams.modeStart = CMD_LOAD_MODULE; // 0x00001774
    modParams.modeFinish = CMD_RELOCATE_MODULE; // 0x00001768
    modParams.fileBase = NULL; // 0x00001780
    modParams.fd = fd;
    modParams.unk124 = 0;

    status = sceIoIoctl(fd, 0x208081, NULL, 0, NULL, 0); // 0x0000178C
    if (status >= SCE_ERROR_OK) // 0x00001794
        modParams.unk100 = 0x10; // 0x0000179C

    status = _loadModuleByBufferID(&modParams, pOption); // 0x000017A4

    sceIoClose(fd);
    pspSetK1(oldK1);
    return status;
}

// Subroutine ModuleMgrForKernel_CAE8E169 - Address 0x00001858
SceUID sceKernelLoadModuleForLoadExecVSHDiscUpdater(const char *path, s32 flag, const SceKernelLMOption *pOption)
{
    s32 oldK1;
    SceUID fd;
    s32 status;
    SceModuleMgrParam modParams;

    (void)flag;

    oldK1 = pspShiftK1(); // 0x00001864

    // Cannot be called in an interruption
    if (sceKernelIsIntrContext()) { // 0x0000187C
        pspSetK1(oldK1);
        return SCE_ERROR_KERNEL_CANNOT_BE_CALLED_FROM_INTERRUPT;
    }

    if ((status = _checkCallConditionKernel()) < 0) { //0x00001894
        pspSetK1(oldK1);
        return status;
    }

    //0x000018A0 - 0x000019AC, 0x000019C8 - 0x00001A20
    if ((status = _checkPathConditions(path)) < 0 || (status = _checkLMOptionConditions(pOption)) < 0) {
        pspSetK1(oldK1);
        return status;
    }

    fd = sceIoOpen(path, SCE_O_ENCRYPTED | SCE_O_RDONLY, SCE_STM_RWXUGO); // 0x000018D0
    if (fd < 0) { // 0x000018DC
        pspSetK1(oldK1);
        return fd;
    }

    status = sceIoIoctl(fd, 0x208011, NULL, 0, NULL, 0); // 0x000018FC
    if (status < SCE_ERROR_OK) { // 0x00001908
        sceIoClose(fd);
        pspSetK1(oldK1);
        return SCE_ERROR_KERNEL_PROHIBIT_LOADMODULE_DEVICE;
    }

    pspClearMemory32(&modParams, sizeof modParams); // 0x0000191C

    modParams.apiType = SCE_EXEC_FILE_APITYPE_DISC_UPDATER;
    modParams.modeStart = CMD_LOAD_MODULE; // 0x00001944
    modParams.modeFinish = CMD_RELOCATE_MODULE; // 0x00001938
    modParams.fileBase = NULL; // 0x00001950
    modParams.fd = fd;
    modParams.unk124 = 0;

    status = sceIoIoctl(fd, 0x208081, NULL, 0, NULL, 0); // 0x0000195C
    if (status >= SCE_ERROR_OK) // 0x00001964
        modParams.unk100 = 0x10; // 0x0000196C

    status = _loadModuleByBufferID(&modParams, pOption); // 0x00001980

    sceIoClose(fd);
    pspSetK1(oldK1);
    return status;
}

// Subroutine ModuleMgrForKernel_2C4F270D - Address 0x00001A28
SceUID sceKernelLoadModuleForLoadExecVSHDiscDebug(const char *path, s32 flag, const SceKernelLMOption *pOption)
{
    s32 oldK1;
    SceUID fd;
    s32 status;
    SceModuleMgrParam modParams;

    (void)flag;

    oldK1 = pspShiftK1(); // 0x00001A34

    // Cannot be called in an interruption
    if (sceKernelIsIntrContext()) { // 0x00001A4C
        pspSetK1(oldK1);
        return SCE_ERROR_KERNEL_CANNOT_BE_CALLED_FROM_INTERRUPT;
    }

    if ((status = _checkCallConditionKernel()) < 0) { //0x00001A64
        pspSetK1(oldK1);
        return status;
    }

    //0x00001A70 - 0x00001B94, 0x00001B98 - 0x00001BF0
    if ((status = _checkPathConditions(path)) < 0 || (status = _checkLMOptionConditions(pOption)) < 0) {
        pspSetK1(oldK1);
        return status;
    }

    fd = sceIoOpen(path, SCE_O_ENCRYPTED | SCE_O_RDONLY, SCE_STM_RWXUGO); // 0x00001AA0
    if (fd < 0) { // 0x00001AAC
        pspSetK1(oldK1);
        return fd;
    }

    status = sceIoIoctl(fd, 0x208011, NULL, 0, NULL, 0); // 0x00001ACC
    if (status < SCE_ERROR_OK) { // 0x00001AD8
        sceIoClose(fd);
        pspSetK1(oldK1);
        return SCE_ERROR_KERNEL_PROHIBIT_LOADMODULE_DEVICE;
    }

    pspClearMemory32(&modParams, sizeof modParams); // 0x00001AEC

    modParams.apiType = SCE_EXEC_FILE_APITYPE_DISC_DEBUG;
    modParams.modeStart = CMD_LOAD_MODULE; // 0x00001B14
    modParams.modeFinish = CMD_RELOCATE_MODULE; // 0x00001B08
    modParams.fileBase = NULL; // 0x00001B20
    modParams.fd = fd; //0x00001B28
    modParams.unk124 = 0;

    status = sceIoIoctl(fd, 0x208081, NULL, 0, NULL, 0); // 0x00001B2C
    if (status >= SCE_ERROR_OK) // 0x00001B34
        modParams.unk100 = 0x10; // 0x00001B3C

    status = _loadModuleByBufferID(&modParams, pOption); // 0x00001B44

    sceIoClose(fd);
    pspSetK1(oldK1);
    return status;
}

// Subroutine ModuleMgrForKernel_853A6C16 - Address 0x00001BF8
SceUID sceKernelLoadModuleForLoadExecVSHDiscEmu(s32 apiType, const char *path, s32 flag, 
    const SceKernelLMOption *pOption)
{
    s32 oldK1;
    SceUID fd;
    s32 status;
    SceModuleMgrParam modParams;

    (void)flag;

    oldK1 = pspShiftK1(); // 0x00001C04

    // Cannot be called in an interruption
    if (sceKernelIsIntrContext()) { // 0x00001C24
        pspSetK1(oldK1);
        return SCE_ERROR_KERNEL_CANNOT_BE_CALLED_FROM_INTERRUPT;
    }

    if ((status = _checkCallConditionKernel()) < 0) { //0x00001C3C
        pspSetK1(oldK1);
        return status;
    }

    //0x00001C48 - 0x00001D6C, 0x00001D70 - 0x00001DC8
    if ((status = _checkPathConditions(path)) < 0 || (status = _checkLMOptionConditions(pOption)) < 0) {
        pspSetK1(oldK1);
        return status;
    }

    fd = sceIoOpen(path, SCE_O_ENCRYPTED | SCE_O_RDONLY, SCE_STM_RWXUGO); // 0x00001C78
    if (fd < 0) { // 0x00001C84
        pspSetK1(oldK1);
        return fd;
    }

    status = sceIoIoctl(fd, 0x208010, NULL, 0, NULL, 0); // 0x00001CA4
    if (status < SCE_ERROR_OK) { // 0x00001CB0
        sceIoClose(fd);
        pspSetK1(oldK1);
        return SCE_ERROR_KERNEL_PROHIBIT_LOADMODULE_DEVICE;
    }

    pspClearMemory32(&modParams, sizeof modParams); // 0x00001CC4

    modParams.apiType = apiType; //0x00001CCC
    modParams.modeStart = CMD_LOAD_MODULE; // 0x00001CE4
    modParams.modeFinish = CMD_RELOCATE_MODULE; // 0x00001CD8
    modParams.fileBase = NULL; // 0x00001CF0
    modParams.fd = fd; //0x00001CFC
    modParams.unk124 = 0;

    status = sceIoIoctl(fd, 0x208081, NULL, 0, NULL, 0); // 0x00001D00
    if (status >= SCE_ERROR_OK) // 0x00001D08
        modParams.unk100 = 0x10;

    status = _loadModuleByBufferID(&modParams, pOption); // 0x00001D18

    sceIoClose(fd);
    pspSetK1(oldK1);
    return status;
}

// Subroutine ModuleMgrForKernel_C2A5E6CA - Address 0x00001DD0
SceUID ModuleMgrForKernel_C2A5E6CA(s32 apiType, const char *path, s32 flag, 
    const SceKernelLMOption *pOption)
{
    s32 oldK1;
    SceUID fd;
    s32 status;
    char installId[SCE_SECURE_INSTALL_ID_LEN];
    SceModuleMgrParam modParams;

    (void)flag;

    oldK1 = pspShiftK1(); // 0x00001DDC

    // Cannot be called in an interruption
    if (sceKernelIsIntrContext()) { // 0x00001E00
        pspSetK1(oldK1);
        return SCE_ERROR_KERNEL_CANNOT_BE_CALLED_FROM_INTERRUPT;
    }

    if ((status = _checkCallConditionKernel()) < 0) { //0x00001E18
        pspSetK1(oldK1);
        return status;
    }

    //0x00001E24 - 0x00001F74, 0x00001F78 - 0x00001FD4
    if ((status = _checkPathConditions(path)) < 0 || (status = _checkLMOptionConditions(pOption)) < 0) {
        pspSetK1(oldK1);
        return status;
    }

    fd = sceIoOpen(path, SCE_O_ENCRYPTED | SCE_O_RDONLY, SCE_STM_RWXUGO); // 0x00001E54
    if (fd < 0) { // 0x00001E60
        pspSetK1(oldK1);
        return fd;
    }

    status = sceIoIoctl(fd, 0x208013, NULL, 0, NULL, 0); // 0x00001E7C
    if (status < SCE_ERROR_OK) { // 0x00001E8C
        sceIoClose(fd);
        pspSetK1(oldK1);
        return SCE_ERROR_KERNEL_PROHIBIT_LOADMODULE_DEVICE;
    }

    memset(installId, 0, sizeof installId); //0x00001EA0
    pspClearMemory32(&modParams, sizeof modParams); // 0x00001EB4

    modParams.apiType = apiType; //0x00001EBC
    modParams.modeStart = CMD_LOAD_MODULE; // 0x00001ED4
    modParams.modeFinish = CMD_RELOCATE_MODULE; // 0x00001EC8
    modParams.fileBase = NULL; // 0x00001EE0
    modParams.fd = fd; //0x00001EEC
    modParams.unk124 = 0; //0x00001EF4

    status = sceIoIoctl(fd, 0x208081, NULL, 0, NULL, 0); // 0x00001EF0
    if (status >= SCE_ERROR_OK) // 0x00001EF8
        modParams.unk100 = 0x10;

    memcpy(modParams.secureInstallId, installId, sizeof modParams.secureInstallId); //0x00001F10

    status = _loadModuleByBufferID(&modParams, pOption); // 0x00001F1C

    sceIoClose(fd);
    pspSetK1(oldK1);
    return status;
}

// Subroutine ModuleMgrForKernel_FE61F16D - Address 0x00001FD8
SceUID sceKernelLoadModuleForLoadExecVSHMs1(s32 apiType, const char *path, s32 flag, 
    const SceKernelLMOption *pOption)
{
    s32 oldK1;
    SceUID fd;
    s32 status;
    SceModuleMgrParam modParams;

    (void)flag;

    oldK1 = pspShiftK1(); // 0x00001FE4

    // Cannot be called in an interruption
    if (sceKernelIsIntrContext()) { // 0x00002004
        pspSetK1(oldK1);
        return SCE_ERROR_KERNEL_CANNOT_BE_CALLED_FROM_INTERRUPT;
    }

    if ((status = _checkCallConditionKernel()) < 0) { //0x0000201C
        pspSetK1(oldK1);
        return status;
    }

    //0x00002028 - 0x0000214C, 0x00002150 - 0x000021AC
    if ((status = _checkPathConditions(path)) < 0 || (status = _checkLMOptionConditions(pOption)) < 0) {
        pspSetK1(oldK1);
        return status;
    }

    fd = sceIoOpen(path, SCE_O_ENCRYPTED | SCE_O_RDONLY, SCE_STM_RWXUGO); // 0x00002058
    if (fd < 0) { // 0x00002064
        pspSetK1(oldK1);
        return fd;
    }

    status = sceIoIoctl(fd, 0x208013, NULL, 0, NULL, 0); // 0x00002084
    if (status < SCE_ERROR_OK) { // 0x00002090
        sceIoClose(fd);
        pspSetK1(oldK1);
        return SCE_ERROR_KERNEL_PROHIBIT_LOADMODULE_DEVICE;
    }

    pspClearMemory32(&modParams, sizeof modParams); // 0x000020A4

    modParams.apiType = apiType; //0x000020AC
    modParams.modeStart = CMD_LOAD_MODULE; // 0x000020C4
    modParams.modeFinish = CMD_RELOCATE_MODULE; // 0x000020B8
    modParams.fileBase = NULL; // 0x000020D0
    modParams.fd = fd; //0x000020DC
    modParams.unk124 = 0;

    status = sceIoIoctl(fd, 0x208081, NULL, 0, NULL, 0); // 0x000020E0
    if (status >= SCE_ERROR_OK) // 0x000020E8
        modParams.unk100 = 0x10;

    status = _loadModuleByBufferID(&modParams, pOption); // 0x000020F8

    sceIoClose(fd);
    pspSetK1(oldK1);
    return status;
}

// Subroutine ModuleMgrForKernel_7BD53193 - Address 0x000021B0
SceUID sceKernelLoadModuleForLoadExecVSHMs2(s32 apiType, const char *path, s32 flag, 
    const SceKernelLMOption *pOption)
{
    s32 oldK1;
    SceUID fd;
    s32 status;
    SceModuleMgrParam modParams;

    (void)flag;

    oldK1 = pspShiftK1(); // 0x000021BC

    // Cannot be called in an interruption
    if (sceKernelIsIntrContext()) { // 0x000021DC
        pspSetK1(oldK1);
        return SCE_ERROR_KERNEL_CANNOT_BE_CALLED_FROM_INTERRUPT;
    }

    if ((status = _checkCallConditionKernel()) < 0) { //0x000021F4
        pspSetK1(oldK1);
        return status;
    }

    //0x00002200 - 0x00002324, 0x00002328 - 0x00002384
    if ((status = _checkPathConditions(path)) < 0 || (status = _checkLMOptionConditions(pOption)) < 0) {
        pspSetK1(oldK1);
        return status;
    }

    fd = sceIoOpen(path, SCE_O_ENCRYPTED | SCE_O_RDONLY, SCE_STM_RWXUGO); // 0x00002230
    if (fd < 0) { // 0x0000223C
        pspSetK1(oldK1);
        return fd;
    }

    status = sceIoIoctl(fd, 0x208013, NULL, 0, NULL, 0); // 0x0000225C
    if (status < SCE_ERROR_OK) { // 0x00002268
        sceIoClose(fd);
        pspSetK1(oldK1);
        return SCE_ERROR_KERNEL_PROHIBIT_LOADMODULE_DEVICE;
    }

    pspClearMemory32(&modParams, sizeof modParams); // 0x0000227C

    modParams.apiType = apiType; //0x00002284
    modParams.modeStart = CMD_LOAD_MODULE; // 0x0000229C
    modParams.modeFinish = CMD_RELOCATE_MODULE; // 0x00002290
    modParams.fileBase = NULL; // 0x000022A8
    modParams.fd = fd; //0x000022B4
    modParams.unk124 = 0;

    status = sceIoIoctl(fd, 0x208081, NULL, 0, NULL, 0); // 0x000022D0
    if (status >= 0x0000225C) // 0x000022C0
        modParams.unk100 = 0x10;

    status = _loadModuleByBufferID(&modParams, pOption); // 0x000022D0

    sceIoClose(fd);
    pspSetK1(oldK1);
    return status;
}

// Subroutine ModuleMgrForKernel_D60AB6CC - Address 0x00002388
SceUID sceKernelLoadModuleForLoadExecVSHMs3(s32 apiType, const char *path, s32 flag, 
    const SceKernelLMOption *pOption)
{
    s32 oldK1;
    SceUID fd;
    s32 status;
    char installId[SCE_SECURE_INSTALL_ID_LEN];
    SceModuleMgrParam modParams;

    (void)flag;

    oldK1 = pspShiftK1(); // 0x00002394

    // Cannot be called in an interruption
    if (sceKernelIsIntrContext()) { // 0x000023B8
        pspSetK1(oldK1);
        return SCE_ERROR_KERNEL_CANNOT_BE_CALLED_FROM_INTERRUPT;
    }

    if ((status = _checkCallConditionKernel()) < 0) { //0x000023D0
        pspSetK1(oldK1);
        return status;
    }

    //0x000023DC - 0x0000252C, 0x00002530 - 0x0000258C
    if ((status = _checkPathConditions(path)) < 0 || (status = _checkLMOptionConditions(pOption)) < 0) {
        pspSetK1(oldK1);
        return status;
    }

    fd = sceIoOpen(path, SCE_O_ENCRYPTED | SCE_O_RDONLY, SCE_STM_RWXUGO); // 0x0000240C
    if (fd < 0) { // 0x00002418
        pspSetK1(oldK1);
        return fd;
    }

    status = sceIoIoctl(fd, 0x208013, NULL, 0, NULL, 0); // 0x00002438
    if (status < SCE_ERROR_OK) { // 0x00002444
        sceIoClose(fd);
        pspSetK1(oldK1);
        return SCE_ERROR_KERNEL_PROHIBIT_LOADMODULE_DEVICE;
    }

    memset(installId, 0, sizeof installId); //0x00002458
    pspClearMemory32(&modParams, sizeof modParams); // 0x0000246C

    modParams.apiType = apiType; //0x00002474
    modParams.modeStart = CMD_LOAD_MODULE; // 0x0000248C
    modParams.modeFinish = CMD_RELOCATE_MODULE; // 0x00002480
    modParams.fileBase = NULL; // 0x00002498
    modParams.fd = fd; //0x000024A4
    modParams.unk124 = 0;

    status = sceIoIoctl(fd, 0x208081, NULL, 0, NULL, 0); // 0x000024A8
    if (status >= SCE_ERROR_OK) // 0x000024B0
        modParams.unk100 = 0x10;

    memcpy(modParams.secureInstallId, installId, sizeof modParams.secureInstallId); //0x000024C8

    status = _loadModuleByBufferID(&modParams, pOption); // 0x000024D4

    sceIoClose(fd);
    pspSetK1(oldK1);
    return status;
}

// Subroutine ModuleMgrForKernel_76F0E956 - Address 0x00002590
SceUID sceKernelLoadModuleForLoadExecVSHMs4(s32 apiType, const char *path, s32 flag, 
    const SceKernelLMOption *pOption)
{
    s32 oldK1;
    SceUID fd;
    s32 status;
    SceModuleMgrParam modParams;

    (void)flag;

    oldK1 = pspShiftK1(); // 0x0000259C

    // Cannot be called in an interruption
    if (sceKernelIsIntrContext()) { // 0x000025BC
        pspSetK1(oldK1);
        return SCE_ERROR_KERNEL_CANNOT_BE_CALLED_FROM_INTERRUPT;
    }

    if ((status = _checkCallConditionKernel()) < 0) { //0x000025D4
        pspSetK1(oldK1);
        return status;
    }

    //0x000025E0 - 0x00002704, 0x00002708 - 0x00002764
    if ((status = _checkPathConditions(path)) < 0 || (status = _checkLMOptionConditions(pOption)) < 0) {
        pspSetK1(oldK1);
        return status;
    }

    fd = sceIoOpen(path, SCE_O_ENCRYPTED | SCE_O_RDONLY, SCE_STM_RWXUGO); // 0x00002610
    if (fd < 0) {
        pspSetK1(oldK1);
        return fd;
    }

    status = sceIoIoctl(fd, 0x208013, NULL, 0, NULL, 0); // 0x0000263C
    if (status < SCE_ERROR_OK) {
        sceIoClose(fd);
        pspSetK1(oldK1);
        return SCE_ERROR_KERNEL_PROHIBIT_LOADMODULE_DEVICE;
    }

    pspClearMemory32(&modParams, sizeof modParams); // 0x0000265C

    modParams.apiType = apiType; //0x00002664
    modParams.modeStart = CMD_LOAD_MODULE; // 0x0000267C
    modParams.modeFinish = CMD_RELOCATE_MODULE; // 0x00002670
    modParams.fileBase = NULL; // 0x00002688
    modParams.fd = fd; //0x00002694
    modParams.unk124 = 0;

    status = sceIoIoctl(fd, 0x208081, NULL, 0, NULL, 0); // 0x00002698
    if (status >= SCE_ERROR_OK)
        modParams.unk100 = 0x10;

    status = _loadModuleByBufferID(&modParams, pOption); // 0x000026B0

    sceIoClose(fd);
    pspSetK1(oldK1);
    return status;
}

// Subroutine ModuleMgrForKernel_4E8A2C9D - Address 0x00002768
SceUID sceKernelLoadModuleForLoadExecVSHMs5(s32 apiType, const char *path, s32 flag, 
    const SceKernelLMOption *pOption)
{
    s32 oldK1;
    SceUID fd;
    s32 status;
    char installId[SCE_SECURE_INSTALL_ID_LEN];
    SceModuleMgrParam modParams;

    (void)flag;

    oldK1 = pspShiftK1(); // 0x00002774

    // Cannot be called in an interruption
    if (sceKernelIsIntrContext()) { // 0x0000279C
        pspSetK1(oldK1);
        return SCE_ERROR_KERNEL_CANNOT_BE_CALLED_FROM_INTERRUPT;
    }

    if ((status = _checkCallConditionKernel()) < 0) { //0x000027B4
        pspSetK1(oldK1);
        return status;
    }

    //0x000027C0 - 0x00002914, 0x00002918 - 0x00002974
    if ((status = _checkPathConditions(path)) < 0 || (status = _checkLMOptionConditions(pOption)) < 0) {
        pspSetK1(oldK1);
        return status;
    }

    fd = sceIoOpen(path, SCE_O_ENCRYPTED | SCE_O_RDONLY, SCE_STM_RWXUGO); // 0x000027F0
    if (fd < 0) {
        pspSetK1(oldK1);
        return fd;
    }

    status = sceIoIoctl(fd, 0x208013, NULL, 0, NULL, 0); // 0x00002820
    if (status < SCE_ERROR_OK) { // 0x00002828
        sceIoClose(fd);
        pspSetK1(oldK1);
        return SCE_ERROR_KERNEL_PROHIBIT_LOADMODULE_DEVICE;
    }

    status = sceKernelGetId(path, installId); //0x00002838
    if (status < SCE_ERROR_OK) {
        sceIoClose(fd);
        pspSetK1(oldK1);
        return status;
    }

    pspClearMemory32(&modParams, sizeof modParams); // 0x00002854

    modParams.apiType = apiType; //0x00002860
    modParams.modeStart = CMD_LOAD_MODULE; // 0x00002878
    modParams.modeFinish = CMD_RELOCATE_MODULE; // 0x0000286C
    modParams.fileBase = NULL; // 0x00002884
    modParams.fd = fd; //0x0000288C
    modParams.unk124 = 0;

    status = sceIoIoctl(fd, 0x208081, NULL, 0, NULL, 0); // 0x00002890
    if (status >= SCE_ERROR_OK) // 0x00002898
        modParams.unk100 = 0x10;

    memcpy(modParams.secureInstallId, installId, sizeof modParams.secureInstallId); //0x000028AC

    status = _loadModuleByBufferID(&modParams, pOption); // 0x000028B8

    sceIoClose(fd);
    pspSetK1(oldK1);
    return status;
}

// Subroutine ModuleMgrForKernel_E8422026 - Address 0x00002978
SceUID sceKernelLoadModuleForLoadExecVSHMs6(s32 apiType, const char *path, s32 flag, 
    const SceKernelLMOption *pOption)
{
    s32 oldK1;
    SceUID fd;
    s32 status;
    char installId[SCE_SECURE_INSTALL_ID_LEN];
    SceModuleMgrParam modParams;

    (void)flag;

    oldK1 = pspShiftK1(); // 0x00002984

    // Cannot be called in an interruption
    if (sceKernelIsIntrContext()) { // 0x000029AC
        pspSetK1(oldK1);
        return SCE_ERROR_KERNEL_CANNOT_BE_CALLED_FROM_INTERRUPT;
    }

    if ((status = _checkCallConditionKernel()) < 0) { //0x000029C4
        pspSetK1(oldK1);
        return status;
    }

    //0x000029D0 - 0x00002B24, 0x00002B28 - 0x00002B84
    if ((status = _checkPathConditions(path)) < 0 || (status = _checkLMOptionConditions(pOption)) < 0) {
        pspSetK1(oldK1);
        return status;
    }

    fd = sceIoOpen(path, SCE_O_ENCRYPTED | SCE_O_RDONLY, SCE_STM_RWXUGO); // 0x00002A00
    if (fd < 0) {
        pspSetK1(oldK1);
        return fd;
    }

    status = sceIoIoctl(fd, 0x208013, NULL, 0, NULL, 0); // 0x00002A30
    if (status < SCE_ERROR_OK) { // 0x00002A38
        sceIoClose(fd);
        pspSetK1(oldK1);
        return SCE_ERROR_KERNEL_PROHIBIT_LOADMODULE_DEVICE;
    }

    status = sceKernelGetId(path, installId); //0x00002A48
    if (status < SCE_ERROR_OK) {
        sceIoClose(fd);
        pspSetK1(oldK1);
        return status;
    }

    pspClearMemory32(&modParams, sizeof modParams); // 0x00002A64

    modParams.apiType = apiType; //0x00002A70
    modParams.modeStart = CMD_LOAD_MODULE; // 0x00002A88
    modParams.modeFinish = CMD_RELOCATE_MODULE; // 0x00002A7C
    modParams.fileBase = NULL; // 0x00002A94
    modParams.fd = fd; //0x00002AA4
    modParams.unk124 = 0;

    status = sceIoIoctl(fd, 0x208081, NULL, 0, NULL, 0); // 0x00002AA0
    if (status >= SCE_ERROR_OK) // 0x00002AA8
        modParams.unk100 = 0x10;

    memcpy(modParams.secureInstallId, installId, sizeof modParams.secureInstallId); //0x00002ABC

    status = _loadModuleByBufferID(&modParams, pOption); // 0x00002AC8

    sceIoClose(fd);
    pspSetK1(oldK1);
    return status;
}

// Subroutine ModuleMgrForKernel_8DD336D4 - Address 0x00002B88
SceUID ModuleMgrForKernel_8DD336D4(s32 apiType, const char *path, s32 flag, 
    const SceKernelLMOption *pOption)
{
    s32 oldK1;
    SceUID fd;
    s32 status;
    char installId[SCE_SECURE_INSTALL_ID_LEN];
    SceModuleMgrParam modParams;

    (void)flag;

    oldK1 = pspShiftK1(); // 0x00002B94

    // Cannot be called in an interruption
    if (sceKernelIsIntrContext()) { // 0x00002BB8
        pspSetK1(oldK1);
        return SCE_ERROR_KERNEL_CANNOT_BE_CALLED_FROM_INTERRUPT;
    }

    if ((status = _checkCallConditionKernel()) < 0) { //0x00002BD0
        pspSetK1(oldK1);
        return status;
    }

    //0x00002BDC - 0x00002D2C, 0x00002D30 - 0x00002D8C
    if ((status = _checkPathConditions(path)) < 0 || (status = _checkLMOptionConditions(pOption)) < 0) {
        pspSetK1(oldK1);
        return status;
    }

    fd = sceIoOpen(path, SCE_O_ENCRYPTED | SCE_O_RDONLY, SCE_STM_RWXUGO); // 0x00002C0C
    if (fd < 0) {
        pspSetK1(oldK1);
        return fd;
    }

    status = sceIoIoctl(fd, 0x208013, NULL, 0, NULL, 0); // 0x00002C38
    if (status < SCE_ERROR_OK) { // 0x00002C44
        sceIoClose(fd);
        pspSetK1(oldK1);
        return SCE_ERROR_KERNEL_PROHIBIT_LOADMODULE_DEVICE;
    }

    memset(installId, 0, sizeof installId); //0x00002C58
    pspClearMemory32(&modParams, sizeof modParams); // 0x00002C6C

    modParams.apiType = apiType; //0x00002C74
    modParams.modeStart = CMD_LOAD_MODULE; // 0x00002C8C
    modParams.modeFinish = CMD_RELOCATE_MODULE; // 0x00002C80
    modParams.fileBase = NULL; // 0x00002C98
    modParams.fd = fd; //0x00002CA4
    modParams.unk124 = 0;

    status = sceIoIoctl(fd, 0x208081, NULL, 0, NULL, 0); // 0x00002CA8
    if (status >= SCE_ERROR_OK) // 0x00002CB0
        modParams.unk100 = 0x10;

    memcpy(modParams.secureInstallId, installId, sizeof modParams.secureInstallId); //0x00002CC8

    status = _loadModuleByBufferID(&modParams, pOption); // 0x00002CD4

    sceIoClose(fd);
    pspSetK1(oldK1);
    return status;
}

// Subroutine ModuleMgrForKernel_30727524 - Address 0x00002D90
SceUID sceKernelLoadModuleForLoadExecNpDrm(s32 apiType, const char *path, SceOff fileOffset, 
    const char *secureInstallId, s32 flag, const SceKernelLMOption *pOption)
{
    s32 oldK1;
    SceUID fd;
    s32 status;
    SceModuleMgrParam modParams;

    (void)flag;

    oldK1 = pspShiftK1(); // 0x00002D9C

    // Cannot be called in an interruption
    if (sceKernelIsIntrContext()) { // 0x00002DD4
        pspSetK1(oldK1);
        return SCE_ERROR_KERNEL_CANNOT_BE_CALLED_FROM_INTERRUPT;
    }

    if ((status = _checkCallConditionKernel()) < 0) { //0x00002DEC
        pspSetK1(oldK1);
        return status;
    }

    //0x00002DF8 - 0x00002F64, 0x00002F68 - 0x00002FC4
    if ((status = _checkPathConditions(path)) < 0 || (status = _checkLMOptionConditions(pOption)) < 0) {
        pspSetK1(oldK1);
        return status;
    }

    if (!pspK1StaBufOk(secureInstallId, SCE_SECURE_INSTALL_ID_LEN)) { //0x00002E2C
        pspSetK1(oldK1);
        return SCE_ERROR_KERNEL_ILLEGAL_ADDR;
    }

    fd = sceIoOpen(path, SCE_O_ENCRYPTED | SCE_O_RDONLY, SCE_STM_RWXUGO); // 0x00002E40
    if (fd < 0) {
        pspSetK1(oldK1);
        return fd;
    }

    sceIoLseek(fd, fileOffset, SCE_SEEK_SET); //0x00002E64

    status = sceIoIoctl(fd, 0x208013, NULL, 0, NULL, 0); // 0x00002E84
    if (status < SCE_ERROR_OK) { // 0x00002E8C
        sceIoClose(fd);
        pspSetK1(oldK1);
        return SCE_ERROR_KERNEL_PROHIBIT_LOADMODULE_DEVICE;
    }

    pspClearMemory32(&modParams, sizeof modParams); // 0x00002EA0

    modParams.apiType = apiType; //0x00002EA8
    modParams.modeStart = CMD_LOAD_MODULE; // 0x00002EC0
    modParams.modeFinish = CMD_RELOCATE_MODULE; // 0x00002EB4
    modParams.fileBase = NULL; // 0x00002ECC
    modParams.fd = fd; //0x00002ED8
    modParams.unk124 = 0;

    status = sceIoIoctl(fd, 0x208081, NULL, 0, NULL, 0); // 0x00002EDC
    if (status >= SCE_ERROR_OK) // 0x00002EE4
        modParams.unk100 = 0x10;

    memcpy(modParams.secureInstallId, secureInstallId, sizeof modParams.secureInstallId); //0x00002EF8

    status = _loadModuleByBufferID(&modParams, pOption); // 0x00002F04

    sceIoClose(fd);
    pspSetK1(oldK1);
    return status;
}

// Subroutine ModuleMgrForKernel_D5DDAB1F - Address 0x00002FC8
SceUID sceKernelLoadModuleVSH(const char *path, s32 flag, const SceKernelLMOption *pOption)
{
    s32 oldK1;
    SceUID fd;
    s32 status;
    SceModuleMgrParam modParams;

    (void)flag;

    oldK1 = pspShiftK1(); // 0x00002FD4

    // Cannot be called in an interruption
    if (sceKernelIsIntrContext()) { // 0x00002FEC
        pspSetK1(oldK1);
        return SCE_ERROR_KERNEL_CANNOT_BE_CALLED_FROM_INTERRUPT;
    }

    if ((status = _checkCallConditionUser()) < 0) { //0x00003000
        pspSetK1(oldK1);
        return status;
    }

    status = sceKernelGetUserLevel(); // 0x00003030
    if (status != SCE_USER_LEVEL_VSH) { //0x00001300
        pspSetK1(oldK1);
        return SCE_ERROR_KERNEL_ILLEGAL_PERMISSION_CALL;
    }

    //0x00003048, 0x00003180; 0x00003184 - 0x000031D8
    if ((status = _checkPathConditions(path)) < 0 || (status = _checkLMOptionConditions(pOption)) < 0) {
        pspSetK1(oldK1);
        return status;
    }

    fd = sceIoOpen(path, SCE_O_ENCRYPTED | SCE_O_RDONLY, SCE_STM_RWXUGO); // 0x00003078
    if (fd < 0) { // 0x00003084
        pspSetK1(oldK1);
        return fd;
    }

    status = sceIoIoctl(fd, 0x208003, NULL, 0, NULL, 0); // 0x000030A4
#ifdef INSTALLER
    status = 0;
#endif
    if (status < SCE_ERROR_OK) { // 0x000030B0
        sceIoClose(fd);
        pspSetK1(oldK1);
        return SCE_ERROR_KERNEL_PROHIBIT_LOADMODULE_DEVICE;
    }

    pspClearMemory32(&modParams, sizeof modParams); // 0x000030C4

    modParams.apiType = SCE_EXEC_FILE_APITYPE_MODULE_VSH; //0x000030D4
    modParams.modeStart = CMD_LOAD_MODULE; // 0x000030EC
    modParams.modeFinish = CMD_RELOCATE_MODULE; // 0x000030E0
    modParams.fileBase = NULL; // 0x000030F8
    modParams.fd = fd; // 0x00003100
    modParams.unk124 = 0;

    status = sceIoIoctl(fd, 0x208081, NULL, 0, NULL, 0); // 0x00003104
#ifdef INSTALLER
    status = 0;
#endif
    if (status >= SCE_ERROR_OK) // 0x0000310C
        modParams.unk100 = 0x10;

    status = sceIoIoctl(fd, 0x208082, NULL, 0, NULL, 0); // 0x00003130
#ifdef INSTALLER
    status = -1;
#endif
    if (status < SCE_ERROR_OK) // 0x00003138
        modParams.unk124 = 1; //0x00003164

    status = _loadModuleByBufferID(&modParams, pOption); // 0x00003144

    sceIoClose(fd);
    pspSetK1(oldK1);
    return status;
}

// Subroutine ModuleMgrForKernel_CBA02988 - Address 0x000031E4
SceUID sceKernelLoadModuleVSHByID(SceUID inputId, s32 flag, const SceKernelLMOption *pOption)
{
    s32 oldK1;
    s32 status;
    SceModuleMgrParam modParams;

    (void)flag;

    oldK1 = pspShiftK1(); // 0x000031F0

    // Cannot be called in an interruption
    if (sceKernelIsIntrContext()) { // 0x00003214
        pspSetK1(oldK1);
        return SCE_ERROR_KERNEL_CANNOT_BE_CALLED_FROM_INTERRUPT;
    }

    if ((status = _checkCallConditionUser()) < 0) { //0x0000321C
        pspSetK1(oldK1);
        return status;
    }

    status = sceKernelGetUserLevel(); // 0x0000324C
    if (status != SCE_USER_LEVEL_VSH) { //0x00003258
        pspSetK1(oldK1);
        return SCE_ERROR_KERNEL_ILLEGAL_PERMISSION_CALL;
    }

    //0x00003260 - 0x000032AC
    if ((status = _checkLMOptionConditions(pOption)) < 0) {
        pspSetK1(oldK1);
        return status;
    }

    status = sceIoValidateFd(inputId, 4); //0x000032B4
    if (status < SCE_ERROR_OK) { //0x000032BC
        pspSetK1(oldK1);
        return status;
    }

    status = sceIoIoctl(inputId, 0x208003, NULL, 0, NULL, 0); // 0x000032DC
#ifdef INSTALLER
    status = 0;
#endif
    if (status < SCE_ERROR_OK) { // 0x000032E8
        pspSetK1(oldK1);
        return SCE_ERROR_KERNEL_PROHIBIT_LOADMODULE_DEVICE;
    }

    pspClearMemory32(&modParams, sizeof modParams); // 0x000032FC

    modParams.apiType = SCE_EXEC_FILE_APITYPE_MODULE_VSH; //0x0000330C
    modParams.modeStart = CMD_LOAD_MODULE; // 0x00003324
    modParams.modeFinish = CMD_RELOCATE_MODULE; // 0x00003318
    modParams.fileBase = NULL; // 0x00003330
    modParams.fd = inputId; // 0x00003338
    modParams.unk124 = 0;

    status = sceIoIoctl(inputId, 0x208081, NULL, 0, NULL, 0); // 0x0000333C
#ifdef INSTALLER
    status = 0;
#endif
    if (status >= SCE_ERROR_OK) // 0x00003344
        modParams.unk100 = 0x10;

    status = sceIoIoctl(inputId, 0x208082, NULL, 0, NULL, 0); // 0x00003368
#ifdef INSTALLER
    status = -1;
#endif
    if (status < SCE_ERROR_OK) // 0x00003390
        modParams.unk124 = 1; //0x00003164

    status = _loadModuleByBufferID(&modParams, pOption); // 0x0000337C

    pspSetK1(oldK1);
    return status;
}

// Subroutine ModuleMgrForKernel_939E4270 - Address 0x00003394
SceUID sceKernelLoadModuleForKernel(const char *path, s32 flag, const SceKernelLMOption *pOption)
{
    s32 oldK1;
    SceUID fd;
    s32 status;
    SceModuleMgrParam modParams;

    (void)flag;

    oldK1 = pspShiftK1(); // 0x000033A0

    // Cannot be called in an interruption
    if (sceKernelIsIntrContext()) { // 0x000033C4
        pspSetK1(oldK1);
        return SCE_ERROR_KERNEL_CANNOT_BE_CALLED_FROM_INTERRUPT;
    }

    //0x000033D0 - 0x000033F0, 0x00003514 - 0x0000358C
    if ((status = _checkCallConditionKernel()) < 0 || (status = _checkPathConditions(path)) < 0
            || (status = _checkLMOptionConditions(pOption)) < 0) {
        pspSetK1(oldK1);
        return status;
    }

    fd = sceIoOpen(path, SCE_O_ENCRYPTED | SCE_O_RDONLY, SCE_STM_RWXUGO); // 0x0000340C
    if (fd < 0) { // 0x00000740
        pspSetK1(oldK1);
        return fd;
    }

    status = sceIoIoctl(fd, 0x208006, NULL, 0, NULL, 0); // 0x00003438
    if (status < SCE_ERROR_OK) { // 0x00003444
        sceIoClose(fd);
        pspSetK1(oldK1);
        return SCE_ERROR_KERNEL_PROHIBIT_LOADMODULE_DEVICE;
    }

    pspClearMemory32(&modParams, sizeof modParams); // 0x00003458

    modParams.apiType = SCE_EXEC_FILE_APITYPE_MODULE_KERNEL; // 0x00003470
    modParams.modeStart = CMD_LOAD_MODULE; // 0x0000347C
    modParams.modeFinish = CMD_RELOCATE_MODULE; // 0x00003464
    modParams.fileBase = NULL; // 0x00003488
    modParams.fd = fd; // 0x00003490
    modParams.unk124 = 0;

    status = sceIoIoctl(fd, 0x208081, NULL, 0, NULL, 0); // 0x00003494
    if (status >= SCE_ERROR_OK) // 0x0000349C
        modParams.unk100 = 0x10; // 0x000034A4

    status = sceIoIoctl(fd, 0x208082, NULL, 0, NULL, 0); // 0x000034C0
    if (status < SCE_ERROR_OK) // 0x000034C8
        modParams.unk124 = 1; //0x00003510

    status = _loadModuleByBufferID(&modParams, pOption); // 0x000034D4

    sceIoClose(fd); // 0x000034E0
    pspSetK1(oldK1);
    return status;
}

// Subroutine ModuleMgrForKernel_EEC2A745 - Address 0x00003590
SceUID sceKernelLoadModuleByIDForKernel(SceUID inputId, s32 flag, const SceKernelLMOption *pOption)
{
    s32 oldK1;
    s32 status;
    SceModuleMgrParam modParams;

    (void)flag;

    oldK1 = pspShiftK1(); //0x0000359C

    if (sceKernelIsIntrContext()) { //0x000035B4
        pspSetK1(oldK1);
        return SCE_ERROR_KERNEL_CANNOT_BE_CALLED_FROM_INTERRUPT;
    }

    //0x000035CC, 0x000035D4 - 0x000035E8, 0x00003614 - 0x00003644
    if ((status = _checkCallConditionKernel()) < 0 || (status = _checkLMOptionConditions(pOption)) < 0) {
        pspSetK1(oldK1);
        return status;
    }

    status = sceIoValidateFd(inputId, 4); //0x0000364C
    if (status < SCE_ERROR_OK) { //0x00003654
        pspSetK1(oldK1);
        return status;
    }

    status = sceIoIoctl(inputId, 0x208006, NULL, 0, NULL, 0); // 0x00003674
    if (status < SCE_ERROR_OK) { // 0x00003680
        pspSetK1(oldK1);
        return SCE_ERROR_KERNEL_PROHIBIT_LOADMODULE_DEVICE;
    }

    pspClearMemory32(&modParams, sizeof modParams); //0x00003694

    modParams.apiType = SCE_EXEC_FILE_APITYPE_MODULE_KERNEL; // 0x000036AC
    modParams.modeFinish = CMD_RELOCATE_MODULE; // 0x000036A0
    modParams.modeStart = CMD_LOAD_MODULE; // 0x000036B8
    modParams.fileBase = NULL; // 0x000036C4
    modParams.fd = inputId; // 0x000036CC
    modParams.unk124 = 0;

    status = sceIoIoctl(inputId, 0x208081, NULL, 0, NULL, 0); //0x000036D0
    if (status >= SCE_ERROR_OK) //0x000036D8
        modParams.unk100 = 0x10; // 0x000036E0

    status = sceIoIoctl(inputId, 0x208082, NULL, 0, NULL, 0); // 0x000036FC
    if (status < SCE_ERROR_OK) // 0x00003704
        modParams.unk124 = 1; //0x00003724

    status = _loadModuleByBufferID(&modParams, pOption); //0x00003710

    pspSetK1(oldK1);
    return status;
}

// Subroutine ModuleMgrForKernel_D4EE2D26 - Address 0x00003728
SceUID sceKernelLoadModuleToBlock(const char *path, SceUID blockId, SceUID *pNewBlockId, s32 flag, 
    const SceKernelLMOption *pOption)
{
    s32 oldK1;
    s32 status;
    SceUID fd;
    SceModuleMgrParam modParams;

    (void)flag;

    oldK1 = pspShiftK1(); //0x0000375C

    if (sceKernelIsIntrContext()) { //0x00003774
        pspSetK1(oldK1);
        return SCE_ERROR_KERNEL_CANNOT_BE_CALLED_FROM_INTERRUPT;
    }

    //0x0000378C, 0x00003798 - 0x000037AC
    if ((status = _checkCallConditionKernel()) < 0 || (status = _checkPathConditions(path)) < 0
            || _checkLMOptionConditions(pOption)) {
        pspSetK1(oldK1);
        return status;
    }

    // 0x000037BC
    if (pNewBlockId == NULL) {
        pspSetK1(oldK1);
        return SCE_ERROR_KERNEL_INVALID_ARGUMENT;
    }

    // 0x000037C4
    if (pOption != NULL && pOption->position != SCE_KERNEL_LM_POS_LOW
            && pOption->position != SCE_KERNEL_LM_POS_HIGH) {
        // UOFW: Missing pspSetK1(oldk1) here
        return SCE_ERROR_KERNEL_INVALID_ARGUMENT;
    }

    // 0x000037E8
    if (!pspK1StaBufOk(pNewBlockId, sizeof *pNewBlockId)) {
        pspSetK1(oldK1);
        return SCE_ERROR_KERNEL_ILLEGAL_ADDR;
    }

    // 0x000037F8
    fd = sceIoOpen(path, SCE_O_ENCRYPTED | SCE_O_RDONLY, SCE_STM_RWXUGO);
    if (fd < 0) { // 0x00003804
        pspSetK1(oldK1);
        return fd;
    }

    status = sceIoIoctl(fd, 0x208007, NULL, 0, NULL, 0); // 0x00003824
    if (status < SCE_ERROR_OK) { // 0x00003830
        sceIoClose(fd);
        pspSetK1(oldK1);
        return SCE_ERROR_KERNEL_PROHIBIT_LOADMODULE_DEVICE;
    }

    pspClearMemory32(&modParams, sizeof modParams); //0x00003844

    modParams.apiType = SCE_EXEC_FILE_APITYPE_MODULE_KERNEL_BLOCK; // 0x00003854
    modParams.modeFinish = CMD_RELOCATE_MODULE; // 0x00003860
    modParams.modeStart = CMD_LOAD_MODULE; // 0x0000386C
    modParams.fileBase = NULL; // 0x00003878
    modParams.fd = fd; // 0x00003880
    modParams.unk124 = 0;

    status = sceIoIoctl(fd, 0x208081, NULL, 0, NULL, 0); //0x00003884
    if (status >= SCE_ERROR_OK) //0x0000388C
        modParams.unk100 = 0x10; // 0x000036E0

    SceUID nBlockId;
    modParams.externMemBlockIdKernel = blockId; // 0x0000389C
    modParams.pNewBlockId = &nBlockId; //0x000038C0

    status = sceIoIoctl(fd, 0x208082, NULL, 0, NULL, 0); // 0x000038BC
    if (status < SCE_ERROR_OK) // 0x000038C4
        modParams.unk124 = 1; //0x00003924

    status = _loadModuleByBufferID(&modParams, pOption); //0x000038D0
    if (status >= SCE_ERROR_OK)
        *pNewBlockId = nBlockId; // 0x000038E4

    sceIoClose(fd);
    pspSetK1(oldK1);
    return status;
}

// Subroutine ModuleMgrForKernel_F7C7FEBC - Address 0x000039C0
SceUID sceKernelLoadModuleBootInitConfig(const char *path, s32 flag, const SceKernelLMOption *pOption)
{
    s32 oldK1;
    s32 status;
    SceUID fd;
    SceModuleMgrParam modParams;

    (void)flag;

    oldK1 = pspShiftK1(); //0x000039CC

    if (!sceKernelIsDevelopmentToolMode()) { // 0x000039E4
        pspSetK1(oldK1);
        return SCE_ERROR_KERNEL_ILLEGAL_PERMISSION_CALL;
    }

    if (sceKernelIsIntrContext()) { // 0x00003A1C
        pspSetK1(oldK1);
        return SCE_ERROR_KERNEL_CANNOT_BE_CALLED_FROM_INTERRUPT;
    }

    //0x00003A24 - 0x00003A5C
    if ((status = _checkCallConditionKernel()) < 0 || (status = _checkLMOptionConditions(pOption)) < 0) {
        pspSetK1(oldK1);
        return status;
    }

    fd = sceIoOpen(path, SCE_O_ENCRYPTED | SCE_O_RDONLY, SCE_STM_RWXUGO); // 0x00003A70
    if (fd < 0) { // 0x00003A7C
        pspSetK1(oldK1);
        return fd;
    }

    status = sceIoIoctl(fd, 0x208009, NULL, 0, NULL, 0); // 0x00003A9C
    if (status < SCE_ERROR_OK) { // 0x00003AA8
        sceIoClose(fd);
        pspSetK1(oldK1);
        return SCE_ERROR_KERNEL_PROHIBIT_LOADMODULE_DEVICE;
    }

    pspClearMemory32(&modParams, sizeof modParams); //0x00003ABC

    modParams.apiType = SCE_EXEC_FILE_APITYPE_MODULE_BOOT_INIT_CONFIG; // 0x00003ACC
    modParams.modeFinish = CMD_RELOCATE_MODULE; // 0x00003AD8
    modParams.modeStart = CMD_LOAD_MODULE; // 0x00003AE4
    modParams.fileBase = NULL; // 0x00003AF0
    modParams.fd = fd; // 0x00003AF8
    modParams.unk124 = 0;

    status = sceIoIoctl(fd, 0x208081, NULL, 0, NULL, 0); //0x00003AFC
    if (status >= SCE_ERROR_OK) //0x00003B04
        modParams.unk100 = 0x10;

    status = _loadModuleByBufferID(&modParams, pOption); //0x00003B14

    sceIoClose(fd);
    pspSetK1(oldK1);
    return status;
}

// Subroutine ModuleMgrForKernel_4493E013 - Address 0x00003BAC
SceUID sceKernelLoadModuleDeci(const char *path, s32 flag, const SceKernelLMOption *pOption)
{
    s32 oldK1;
    s32 status;
    SceUID fd;
    SceModuleMgrParam modParams;

    (void)flag;

    oldK1 = pspShiftK1(); //0x00003BB8

    if (!sceKernelIsDevelopmentToolMode()) { // 0x00003BD0
        pspSetK1(oldK1);
        return SCE_ERROR_KERNEL_ILLEGAL_PERMISSION_CALL;
    }

    if (sceKernelIsIntrContext()) { // 0x00003C08
        pspSetK1(oldK1);
        return SCE_ERROR_KERNEL_CANNOT_BE_CALLED_FROM_INTERRUPT;
    }

    //0x00003C10 - 0x00003C48
    if ((status = _checkCallConditionKernel()) < 0 || (status = _checkLMOptionConditions(pOption)) < 0) {
        pspSetK1(oldK1);
        return status;
    }

    fd = sceIoOpen(path, SCE_O_ENCRYPTED | SCE_O_RDONLY, SCE_STM_RWXUGO); // 0x00003C5C
    if (fd < 0) { // 0x00003C68
        pspSetK1(oldK1);
        return fd;
    }

    status = sceIoIoctl(fd, 0x20800C, NULL, 0, NULL, 0); // 0x00003C88
    if (status < SCE_ERROR_OK) { // 0x00003C94
        sceIoClose(fd);
        pspSetK1(oldK1);
        return SCE_ERROR_KERNEL_PROHIBIT_LOADMODULE_DEVICE;
    }

    pspClearMemory32(&modParams, sizeof modParams); //0x00003CA8

    modParams.apiType = SCE_EXEC_FILE_APITYPE_MODULE_DECI; // 0x00003CB8
    modParams.modeFinish = CMD_RELOCATE_MODULE; // 0x00003CC4
    modParams.modeStart = CMD_LOAD_MODULE; // 0x00003CD0
    modParams.fileBase = NULL; // 0x00003CDC
    modParams.fd = fd; // 0x00003CE4
    modParams.unk124 = 0;

    status = sceIoIoctl(fd, 0x208081, NULL, 0, NULL, 0); //0x00003CE8
    if (status >= SCE_ERROR_OK) //0x00003CF0
        modParams.unk100 = 0x10;

    status = _loadModuleByBufferID(&modParams, pOption); //0x00003D00

    sceIoClose(fd);
    pspSetK1(oldK1);
    return status;
}

// Subroutine ModuleMgrForKernel_61E3EC69 - Address 0x000050FC
SceUID sceKernelLoadModuleBufferForExitGame(void *base, s32 flag, const SceKernelLMOption *pOption, s32 opt)
{
    s32 oldK1;
    s32 status;
    SceModuleMgrParam modParams;

    (void)flag;
    (void)opt;

    oldK1 = pspShiftK1(); // 0x00005108

    // Cannot be called in an interruption
    if (sceKernelIsIntrContext()) { // 0x0000511C
        pspSetK1(oldK1);
        return SCE_ERROR_KERNEL_CANNOT_BE_CALLED_FROM_INTERRUPT;
    }

    // 0x00005138
    if ((status = _checkCallConditionKernel()) < 0) {
        pspSetK1(oldK1);
        return status;
    }
    //0x00005134 - 0x0000514C
    if (!pspK1PtrOk(base)) {
        pspSetK1(oldK1);
        return SCE_ERROR_KERNEL_ILLEGAL_ADDR;
    }

    status = _checkLMOptionConditions(pOption); //0x00005170
    if (status < 0) {
        pspSetK1(oldK1);
        return status;
    }

    _setupForLoadModuleBuffer(&modParams, SCE_EXEC_FILE_APITYPE_VSH_1, base, 0, 1); // 0x00005190
    status = _loadModuleByBufferID(&modParams, pOption); //0x0000519C

    pspSetK1(oldK1);
    return status;
}

// Subroutine ModuleMgrForUser_1196472E - Address 0x000051AC
SceUID sceKernelLoadModuleBufferMs(SceSize size, void *base, s32 flag, const SceKernelLMOption *pOption)
{
    s32 oldK1;
    s32 status;
    SceModuleMgrParam modParams;

    (void)flag;

    oldK1 = pspShiftK1(); // 0x000051B8

    // Cannot be called in an interruption
    if (sceKernelIsIntrContext()) { // 0x000051D4
        pspSetK1(oldK1);
        return SCE_ERROR_KERNEL_CANNOT_BE_CALLED_FROM_INTERRUPT;
    }

    if ((status = _checkCallConditionUser()) < 0) { // 0x000051E8
        pspSetK1(oldK1);
        return status;
    }

    /* Check for MS API */
    if (sceKernelGetUserLevel() != SCE_USER_LEVEL_MS && sceKernelGetUserLevel() != SCE_USER_LEVEL_APP) { // 0x00005220 & 0x0000522C
        pspSetK1(oldK1);
        return SCE_ERROR_KERNEL_ILLEGAL_PERMISSION_CALL;
    }

    if (!pspK1DynBufOk(base, size)) { // 0x00005258
        pspSetK1(oldK1);
        return SCE_ERROR_KERNEL_ILLEGAL_ADDR;
    }

    status = _checkLMOptionConditions(pOption); //0x00005260
    if (status < 0) { // 0x0000526C
        pspSetK1(oldK1);
        return status;
    }

    _setupForLoadModuleBuffer(&modParams, SCE_EXEC_FILE_APITYPE_MODULE_BUFFER_MS, base, size, 0); // 0x00005280
    status = _loadModuleByBufferID(&modParams, pOption); // 0x00005368

    pspSetK1(oldK1);
    return status;
}

// Subroutine ModuleMgrForUser_24EC0641 - Address 0x0000529C
SceUID sceKernelLoadModuleBufferApp(SceSize size, void *base, s32 flag, const SceKernelLMOption *pOption)
{
    s32 oldK1;
    s32 status;
    SceModuleMgrParam modParams;

    (void)flag;

    oldK1 = pspShiftK1(); // 0x000052A8

    // Cannot be called in an interruption
    if (sceKernelIsIntrContext()) { // 0x000052C4
        pspSetK1(oldK1);
        return SCE_ERROR_KERNEL_CANNOT_BE_CALLED_FROM_INTERRUPT;
    }

    if ((status = _checkCallConditionUser()) < 0) { // 0x000052D8
        pspSetK1(oldK1);
        return status;
    }

    /* Check for APP API */
    if (sceKernelGetUserLevel() != SCE_USER_LEVEL_APP) { // 0x00005308
        pspSetK1(oldK1);
        return SCE_ERROR_KERNEL_ILLEGAL_PERMISSION_CALL;
    }

    if (!pspK1DynBufOk(base, size)) { // 0x00005334
        pspSetK1(oldK1);
        return SCE_ERROR_KERNEL_ILLEGAL_ADDR;
    }

    status = _checkLMOptionConditions(pOption); // 0x0000533C
    if (status < 0) { // 0x00005348
        pspSetK1(oldK1);
        return status;
    }

    _setupForLoadModuleBuffer(&modParams, SCE_EXEC_FILE_APITYPE_MODULE_BUFFER_APP, base, size, 0); // 0x0000535C
    status = _loadModuleByBufferID(&modParams, pOption); //0x00005368

    pspSetK1(oldK1);
    return status;
}

// Subroutine ModuleMgrForKernel_2F3F9B6A - Address 0x00005378
SceUID sceKernelLoadModuleBufferForLoadExecBufferVSHUsbWlan(s32 apiType, void *base, s32 flag, 
    const SceKernelLMOption *pOption)
{
    s32 oldK1;
    s32 status;
    SceModuleMgrParam modParams;

    (void)flag;

    oldK1 = pspShiftK1(); // 0x00005384

    // Cannot be called in an interruption
    if (sceKernelIsIntrContext()) { // 0x000053A0
        pspSetK1(oldK1);
        return SCE_ERROR_KERNEL_CANNOT_BE_CALLED_FROM_INTERRUPT;
    }

    // 0x000053BC
    if ((status = _checkCallConditionKernel()) < 0) {
        pspSetK1(oldK1);
        return status;
    }
    // 0x000053CC
    if (!pspK1PtrOk(base)) {
        pspSetK1(oldK1);
        return SCE_ERROR_KERNEL_ILLEGAL_ADDR;
    }

    status = _checkLMOptionConditions(pOption); // 0x000053F8
    if (status < 0) {
        pspSetK1(oldK1);
        return status;
    }

    _setupForLoadModuleBuffer(&modParams, apiType, base, 0, 0); // 0x00005418
    status = _loadModuleByBufferID(&modParams, pOption); // 0x00005424

    pspSetK1(oldK1);
    return status;
}

// Subroutine ModuleMgrForKernel_C13E2DE5 - Address 0x00005434
SceUID sceKernelLoadModuleBufferForLoadExecBufferVSHUsbWlanDebug(s32 apiType, void *base, s32 flag, 
    const SceKernelLMOption *pOption)
{
    s32 oldK1;
    s32 status;
    SceModuleMgrParam modParams;

    (void)flag;

    oldK1 = pspShiftK1(); // 0x00005440

    // Cannot be called in an interruption
    if (sceKernelIsIntrContext()) { // 0x0000545C
        pspSetK1(oldK1);
        return SCE_ERROR_KERNEL_CANNOT_BE_CALLED_FROM_INTERRUPT;
    }

    // 0x00005478
    if ((status = _checkCallConditionKernel()) < 0) {
        pspSetK1(oldK1);
        return status;
    }
    // 0x00005488
    if (!pspK1PtrOk(base)) {
        pspSetK1(oldK1);
        return SCE_ERROR_KERNEL_ILLEGAL_ADDR;
    }

    status = _checkLMOptionConditions(pOption); // 0x000054B4
    if (status < 0) {
        pspSetK1(oldK1);
        return status;
    }

    _setupForLoadModuleBuffer(&modParams, apiType, base, 0, 0); // 0x000054D4
    status = _loadModuleByBufferID(&modParams, pOption); // 0x000054E0

    pspSetK1(oldK1);
    return status;
}

// Subroutine ModuleMgrForKernel_C6DE0B9C - Address 0x000054F0
SceUID sceKernelLoadModuleBufferVSH(SceSize size, void *base, s32 flag, 
    const SceKernelLMOption *pOption)
{
    s32 oldK1;
    s32 status;
    SceModuleMgrParam modParams;

    (void)flag;

    oldK1 = pspShiftK1(); // 0x000054FC

    // Cannot be called in an interruption
    if (sceKernelIsIntrContext()) { // 0x00005518
        pspSetK1(oldK1);
        return SCE_ERROR_KERNEL_CANNOT_BE_CALLED_FROM_INTERRUPT;
    }

    if ((status = _checkCallConditionUser()) < 0) { // 0x0000552C
        pspSetK1(oldK1);
        return status;
    }

    /* Check for VSH API */
    if (sceKernelGetUserLevel() != SCE_USER_LEVEL_VSH) { // 0x0000555C
        pspSetK1(oldK1);
        return SCE_ERROR_KERNEL_ILLEGAL_PERMISSION_CALL;
    }

    if (!pspK1DynBufOk(base, size)) { // 0x00005588
        pspSetK1(oldK1);
        return SCE_ERROR_KERNEL_ILLEGAL_ADDR;
    }

    status = _checkLMOptionConditions(pOption); // 0x00005590
    if (status < 0) { // 0x0000559C
        pspSetK1(oldK1);
        return status;
    }

    _setupForLoadModuleBuffer(&modParams, SCE_EXEC_FILE_APITYPE_MODULE_BUFFER_VSH, base, size, 0); // 0x000055B0
    status = _loadModuleByBufferID(&modParams, pOption); // 0x000055BC

    pspSetK1(oldK1);
    return status;
}

// Subroutine ModuleMgrForKernel_9236B422 - Address 0x000055CC
SceUID sceKernelLoadModuleBufferForExitVSHVSH(void *base, s32 flag, const SceKernelLMOption *pOption, s32 opt)
{
    s32 oldK1;
    s32 status;
    SceModuleMgrParam modParams;

    (void)flag;
    (void)opt;

    oldK1 = pspShiftK1(); // 0x000055D8

    // Cannot be called in an interruption
    if (sceKernelIsIntrContext()) { // 0x000055EC
        pspSetK1(oldK1);
        return SCE_ERROR_KERNEL_CANNOT_BE_CALLED_FROM_INTERRUPT;
    }

    // 0x00005608
    if ((status = _checkCallConditionKernel()) < 0) {
        pspSetK1(oldK1);
        return status;
    }

    // 0x00005618
    if (!pspK1PtrOk(base)) {
        pspSetK1(oldK1);
        return SCE_ERROR_KERNEL_ILLEGAL_ADDR;
    }

    status = _checkLMOptionConditions(pOption); // 0x00005640
    if (status < 0) {
        pspSetK1(oldK1);
        return status;
    }

    _setupForLoadModuleBuffer(&modParams, SCE_EXEC_FILE_APITYPE_VSH_2, base, 0, 1); // 0x00005660
    status = _loadModuleByBufferID(&modParams, pOption); //0x0000566C

    pspSetK1(oldK1);
    return status;
}

// Subroutine ModuleMgrForKernel_4E62C48A - Address 0x0000567C
SceUID sceKernelLoadModuleBufferForKernel(SceSize size, void *base, s32 flag, 
    const SceKernelLMOption *pOption)
{
    s32 oldK1;
    s32 status;
    SceModuleMgrParam modParams;

    (void)flag;

    oldK1 = pspShiftK1(); // 0x00005688

    // Cannot be called in an interruption
    if (sceKernelIsIntrContext()) { // 0x000056A4
        pspSetK1(oldK1);
        return SCE_ERROR_KERNEL_CANNOT_BE_CALLED_FROM_INTERRUPT;
    }

    if ((status = _checkCallConditionKernel()) < 0) { // 0x000056CC
        pspSetK1(oldK1);
        return status;
    }

    if (!pspK1DynBufOk(base, size)) { // 0x000056DC
        pspSetK1(oldK1);
        return SCE_ERROR_KERNEL_ILLEGAL_ADDR;
    }

    status = _checkLMOptionConditions(pOption); // 0x00005708
    if (status < 0) { // 0x00005714
        pspSetK1(oldK1);
        return status;
    }

    _setupForLoadModuleBuffer(&modParams, SCE_EXEC_FILE_APITYPE_MODULE_BUFFER_KERNEL, base, size, 0); // 0x00005728
    status = _loadModuleByBufferID(&modParams, pOption); // 0x00005734

    pspSetK1(oldK1);
    return status;
}

// Subroutine ModuleMgrForKernel_253AA17C - Address 0x00005744
SceUID sceKernelLoadModuleBufferForExitVSHKernel(void *base, s32 flag, const SceKernelLMOption *pOption, s32 opt)
{
    s32 oldK1;
    s32 status;
    SceModuleMgrParam modParams;

    (void)flag;
    (void)opt;

    oldK1 = pspShiftK1(); // 0x00005750

    // Cannot be called in an interruption
    if (sceKernelIsIntrContext()) { // 0x00005764
        pspSetK1(oldK1);
        return SCE_ERROR_KERNEL_CANNOT_BE_CALLED_FROM_INTERRUPT;
    }

    // 0x00005780
    if ((status = _checkCallConditionKernel()) < 0) {
        pspSetK1(oldK1);
        return status;
    }

    // 0x00005790
    if (!pspK1PtrOk(base)) {
        pspSetK1(oldK1);
        return SCE_ERROR_KERNEL_ILLEGAL_ADDR;
    }

    status = _checkLMOptionConditions(pOption); // 0x000057B8
    if (status < 0) {
        pspSetK1(oldK1);
        return status;
    }

    _setupForLoadModuleBuffer(&modParams, SCE_EXEC_FILE_APITYPE_KERNEL_1, base, 0, 1); // 0x000057D8
    status = _loadModuleByBufferID(&modParams, pOption); //0x000057E4

    pspSetK1(oldK1);
    return status;
}

// Subroutine ModuleMgrForKernel_4E38EA1D - Address 0x000057F4
SceUID sceKernelLoadModuleBufferForRebootKernel(void *base, s32 flag, 
    const SceKernelLMOption *pOption, s32 opt)
{
    s32 oldK1;
    s32 status;
    SceModuleMgrParam modParams;

    (void)flag;
    (void)opt;

    oldK1 = pspShiftK1(); // 0x00005800

    // Cannot be called in an interruption
    if (sceKernelIsIntrContext()) { // 0x00005814
        pspSetK1(oldK1);
        return SCE_ERROR_KERNEL_CANNOT_BE_CALLED_FROM_INTERRUPT;
    }

    // 0x00005830
    if ((status = _checkCallConditionKernel()) < 0) {
        pspSetK1(oldK1);
        return status;
    }

    // 0x00005840
    if (!pspK1PtrOk(base)) {
        pspSetK1(oldK1);
        return SCE_ERROR_KERNEL_ILLEGAL_ADDR;
    }

    status = _checkLMOptionConditions(pOption); // 0x00005868
    if (status < 0) {
        pspSetK1(oldK1);
        return status;
    }

    _setupForLoadModuleBuffer(&modParams, SCE_EXEC_FILE_APITYPE_KERNEL_REBOOT, base, 0, 1); // 0x00005888
    status = _loadModuleByBufferID(&modParams, pOption); //0x00005894

    pspSetK1(oldK1);
    return status;
}

// Subroutine ModuleMgrForKernel_955D6CB2 - Address 0x000058A4
// TODO: Check back function name
s32 sceKernelLoadModuleBootInitBtcnf(void *base, s32 flag, const SceKernelLMOption *pOption)
{
    (void)base;
    (void)flag;
    (void)pOption;

    return SCE_ERROR_KERNEL_ILLEGAL_PERMISSION_CALL;
}

// Subroutine ModuleMgrForKernel_1CF0B794 - Address 0x000058B0
SceUID sceKernelLoadModuleBufferBootInitBtcnf(SceSize size, void *base, s32 flag, 
    const SceKernelLMOption *pOption, s32 opt)
{
    s32 oldK1;
    s32 status;
    SceModuleMgrParam modParams;

    (void)flag;
    (void)opt;

    oldK1 = pspShiftK1(); // 0x000058BC

    // Cannot be called in an interruption
    if (sceKernelIsIntrContext()) { // 0x000058D8
        pspSetK1(oldK1);
        return SCE_ERROR_KERNEL_CANNOT_BE_CALLED_FROM_INTERRUPT;
    }

    if ((status = _checkCallConditionKernel()) < 0) { // 0x00005900
        pspSetK1(oldK1);
        return status;
    }

    if (!pspK1DynBufOk(base, size)) { // 0x00005910
        pspSetK1(oldK1);
        return SCE_ERROR_KERNEL_ILLEGAL_ADDR;
    }

    status = _checkLMOptionConditions(pOption); // 0x0000593C
    if (status < 0) { // 0x00005714
        pspSetK1(oldK1);
        return status;
    }

    _setupForLoadModuleBuffer(&modParams, SCE_EXEC_FILE_APITYPE_MODULE_BUFFER_BOOT_INIT_BTCNF, base, size, 1); // 0x0000595C

    status = _loadModuleByBufferID(&modParams, pOption); // 0x00005968

    pspSetK1(oldK1);
    return status;
}

// Subroutine ModuleMgrForKernel_5FC32087 - Address 0x00005978
s32 sceKernelLoadModuleByIDBootInitConfig(void)
{
    return SCE_ERROR_KERNEL_ILLEGAL_PERMISSION_CALL;
}

// Subroutine ModuleMgrForKernel_E8B9D19D - Address 0x00005984
s32 sceKernelLoadModuleBufferBootInitConfig(void)
{
    return SCE_ERROR_KERNEL_ILLEGAL_PERMISSION_CALL;
}

// Subroutine sub_000075B4 - Address 0x000075B4 
static SceUID _loadModuleByBufferID(SceModuleMgrParam *pModParams, const SceKernelLMOption *pOption)
{
    if (pOption == NULL) { // 0x000075BC
        pModParams->access = SCE_KERNEL_LM_ACCESS_NOSEEK;
        pModParams->mpIdText = SCE_KERNEL_UNKNOWN_PARTITION;
        pModParams->mpIdData = SCE_KERNEL_UNKNOWN_PARTITION;
        pModParams->position = SCE_KERNEL_LM_POS_LOW;
    } else {
        pModParams->mpIdText = pOption->mpIdText;
        pModParams->mpIdData = pOption->mpIdData;
        pModParams->position = pOption->position;
        pModParams->access = pOption->access;
    }

    // 0x000075E4
    pModParams->unk76 = 0;
    pModParams->unk80 = 0;
    pModParams->unk96 = 0;
    pModParams->pExecInfo = NULL;

    return _start_exe_thread(pModParams); // 0x000075F4
}

// Subroutine sub_00007698 - Address 0x00007698
static void _setupForLoadModuleBuffer(SceModuleMgrParam *pModParams, u32 apiType, void *base, SceSize modSize, u32 unk124)
{
    pspClearMemory32(pModParams, sizeof *pModParams); // 0x000076A0

    pModParams->unk124 = unk124; // 0x000076AC
    pModParams->apiType = apiType; // 0x000076B0
    pModParams->modeStart = CMD_RELOCATE_MODULE; // 0x000076C0
    pModParams->modeFinish = CMD_RELOCATE_MODULE; // 0x000076B4
    pModParams->modSize = modSize; // 0x000076B8
    pModParams->fd = (SceUID)base; // 0x000076BC
    pModParams->fileBase = base; // 0x000076C8
}
