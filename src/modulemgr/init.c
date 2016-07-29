/* Copyright (C) 2011 - 2015 The uOFW team
   See the file COPYING for copying permission.
*/

#include <common_imp.h>

//#include <loadcore.h>
#include <modulemgr_init.h>
#include <sysmem_sysclib.h>

/*
 * uofw/src/modulemgr/init.c
 * 
 * init - InitForKernel library
 * 
 * Its purpose is to provide information about an executable which is
 * being launched, or which was recently launched, by the system. The 
 * information includes the API type of the executable, its application
 * type, its file name, and the boot medium which was used to boot the 
 * executable.
 * 
 * While originally placed inside the Init module, this library was later
 * removed from it and implemented in the module manager module instead. 
 * This is done because InitForKernel is a resident library whereas the Init
 * module isn't a resident module. To guarantee the availability of the 
 * exported InitForKernel functions, this library was moved to module manager, 
 * a resident module.
 */

SceInit g_init; //0x000099A0

u32 sceKernelBootFrom(void)
{
    switch (g_init.apiType) {
    case SCE_EXEC_FILE_APITYPE_GAME_EBOOT: //0x00004D20
    case SCE_EXEC_FILE_APITYPE_GAME_BOOT:
    case SCE_EXEC_FILE_APITYPE_EMU_EBOOT_MS:
    case SCE_EXEC_FILE_APITYPE_EMU_BOOT_MS:
    case SCE_EXEC_FILE_APITYPE_EMU_EBOOT_EF:
    case SCE_EXEC_FILE_APITYPE_EMU_BOOT_EF:
    case SCE_EXEC_FILE_APITYPE_NPDRM_MS:
    case SCE_EXEC_FILE_APITYPE_UNK117:
    case SCE_EXEC_FILE_APITYPE_NPDRM_EF:
    case SCE_EXEC_FILE_APITYPE_UNK119:
    case SCE_EXEC_FILE_APITYPE_DISC:
    case SCE_EXEC_FILE_APITYPE_DISC_UPDATER:
    case SCE_EXEC_FILE_APITYPE_DISC_DEBUG:
    case SCE_EXEC_FILE_APITYPE_DISC_EMU_MS1:
    case SCE_EXEC_FILE_APITYPE_DISC_EMU_MS2:
    case SCE_EXEC_FILE_APITYPE_DISC_EMU_EF1:
    case SCE_EXEC_FILE_APITYPE_DISC_EMU_EF2:
    case SCE_EXEC_FILE_APITYPE_UNK160: //0x00004DD4
    case SCE_EXEC_FILE_APITYPE_UNK161:
    case SCE_EXEC_FILE_APITYPE_MLNAPP_MS: //0x00004DC8
    case SCE_EXEC_FILE_APITYPE_MLNAPP_EF:
        return SCE_INIT_BOOT_DISC; //0x00004D20 | 0x00004D38
        
    case SCE_EXEC_FILE_APITYPE_USBWLAN:
    case SCE_EXEC_FILE_APITYPE_USBWLAN_DEBUG:
    case SCE_EXEC_FILE_APITYPE_UNK132:
    case SCE_EXEC_FILE_APITYPE_UNK133:
        return SCE_INIT_BOOT_USBWLAN; //0x00004CFC
        
    case SCE_EXEC_FILE_APITYPE_MS1:
    case SCE_EXEC_FILE_APITYPE_MS2:
    case SCE_EXEC_FILE_APITYPE_MS3:
    case SCE_EXEC_FILE_APITYPE_MS5:
    case SCE_EXEC_FILE_APITYPE_MS6:
        return SCE_INIT_BOOT_MS; //0x00004CE8 | 0x00004D50
        
    case SCE_EXEC_FILE_APITYPE_MS4:
        if (strncmp(sceKernelInitFileName(), "flash3:", strlen("flash3:")) == 0) // 0x00004D58 & 0x00004D6C & 0x00004D7C
            return SCE_INIT_BOOT_FLASH3;
        return SCE_INIT_BOOT_MS;
        
    case SCE_EXEC_FILE_APITYPE_EF1:
    case SCE_EXEC_FILE_APITYPE_EF2:
    case SCE_EXEC_FILE_APITYPE_EF3:
    case SCE_EXEC_FILE_APITYPE_EF4:
    case SCE_EXEC_FILE_APITYPE_EF5:
    case SCE_EXEC_FILE_APITYPE_EF6:
        return SCE_INIT_BOOT_EF; //0x00004D98
        
    default:
        return SCE_INIT_BOOT_FLASH;  
    }
}

//Subroutine InitForKernel_9D33A110 - Address 0x00004DDC
u32 InitForKernel_9D33A110(void)
{
    switch (g_init.apiType) {
    case SCE_EXEC_FILE_APITYPE_GAME_EBOOT:
    case SCE_EXEC_FILE_APITYPE_GAME_BOOT:  
    case SCE_EXEC_FILE_APITYPE_DISC:
    case SCE_EXEC_FILE_APITYPE_DISC_UPDATER:
    case SCE_EXEC_FILE_APITYPE_DISC_DEBUG: //0x00004E70
        return SCE_INIT_BOOT_DISC; //0x00004E2C
        
    case SCE_EXEC_FILE_APITYPE_USBWLAN: //0x00004E9C
    case SCE_EXEC_FILE_APITYPE_USBWLAN_DEBUG:
    case SCE_EXEC_FILE_APITYPE_UNK132:
    case SCE_EXEC_FILE_APITYPE_UNK133:
        return SCE_INIT_BOOT_USBWLAN;
        
    case SCE_EXEC_FILE_APITYPE_EMU_EBOOT_MS:
    case SCE_EXEC_FILE_APITYPE_EMU_BOOT_MS:
    case SCE_EXEC_FILE_APITYPE_NPDRM_MS: //0x00004E64
    case SCE_EXEC_FILE_APITYPE_UNK117:
    case SCE_EXEC_FILE_APITYPE_DISC_EMU_MS1: //0x00004DF8
    case SCE_EXEC_FILE_APITYPE_DISC_EMU_MS2:
    case SCE_EXEC_FILE_APITYPE_MS1: //0x00004EC4
    case SCE_EXEC_FILE_APITYPE_MS2:
    case SCE_EXEC_FILE_APITYPE_MS3:
    case SCE_EXEC_FILE_APITYPE_MS5: //0x00004EEC
    case SCE_EXEC_FILE_APITYPE_MS6:
    case SCE_EXEC_FILE_APITYPE_MLNAPP_MS: //0x00004F10
        return SCE_INIT_BOOT_MS; //0x00004E30
        
    case SCE_EXEC_FILE_APITYPE_MS4: 
        if (strncmp(sceKernelInitFileName(), "flash3:", strlen("flash3:")) == 0) // 0x00004F14 & 0x00004F28 & 0x00004F38
            return SCE_INIT_BOOT_FLASH3;
        return SCE_INIT_BOOT_MS;
        
    case SCE_EXEC_FILE_APITYPE_EMU_EBOOT_EF:
    case SCE_EXEC_FILE_APITYPE_EMU_BOOT_EF:
    case SCE_EXEC_FILE_APITYPE_NPDRM_EF:
    case SCE_EXEC_FILE_APITYPE_UNK119:
    case SCE_EXEC_FILE_APITYPE_DISC_EMU_EF1: //0x00004EB0
    case SCE_EXEC_FILE_APITYPE_DISC_EMU_EF2:
    case SCE_EXEC_FILE_APITYPE_EF1: //0x00004ED4
    case SCE_EXEC_FILE_APITYPE_EF2:
    case SCE_EXEC_FILE_APITYPE_EF3:
    case SCE_EXEC_FILE_APITYPE_EF4:
    case SCE_EXEC_FILE_APITYPE_EF5:
    case SCE_EXEC_FILE_APITYPE_EF6:
    case SCE_EXEC_FILE_APITYPE_MLNAPP_EF: //0x00004EF0
        return SCE_INIT_BOOT_EF; //0x00004E10
        
    default:
        return SCE_INIT_BOOT_FLASH;
    }
}

char *sceKernelInitFileName(void)
{
    return (char *)g_init.fileModAddr;
}

u32 sceKernelSetInitCallback(SceKernelBootCallbackFunction bootCBFunc, u32 flag, s32 *pStatus)
{
    s32 result;
    SceBootCallback *curBootCallback;
    
    if (flag < 4) { //0x00004F64
        if (g_init.bootCallbacks1 == NULL) { //0x00004F78
            result = bootCBFunc((void *)1, 0, NULL); //0x00004FC8
            if (pStatus != NULL) //0x00004FD0
                *pStatus = result;
            return SCE_ERROR_OK;
        }
        curBootCallback = g_init.curBootCallback1; //0x00004F80
        curBootCallback->bootCBFunc = (flag & 0x3) + bootCBFunc; //0x00004F90
        g_init.curBootCallback1 += 1; //0x00004FA0     
        g_init.curBootCallback1->bootCBFunc = NULL; //0x00004FA8
    } else {
        if (g_init.bootCallbacks2 == NULL) { //0x00004FEC
            result = bootCBFunc((void *)1, 0, NULL); //0x00004FC8
            if (pStatus != NULL) //0x00004FD0
                *pStatus = result;
            return SCE_ERROR_OK;
        }
        curBootCallback = g_init.curBootCallback2; //0x00004FF4
        curBootCallback->bootCBFunc = (flag & 0x3) + bootCBFunc; //0x00005004
        g_init.curBootCallback2 += 1; //0x00005010
        g_init.curBootCallback2->bootCBFunc = NULL; //0x00004FA8
    }
    curBootCallback->gp = sceKernelGetModuleGPByAddressForKernel((u32)bootCBFunc); //0x00004F8C & 0x00004FA4
    return SCE_BOOT_CALLBACK_FUNCTION_QUEUED;
}

u32 sceKernelStartIntrLogging(void)
{
    return SCE_ERROR_OK;
}

u32 sceKernelShowIntrHandlerInfo(void)
{
    return SCE_ERROR_OK;
}

u32 sceKernelShowIntrMaskTime(void)
{
    return SCE_ERROR_OK;
}

SceInit *sceKernelQueryInitCB(void)
{
    return &g_init;
}

s32 sceKernelInitApitype(void)
{
    return g_init.apiType;
}

s32 sceKernelApplicationType(void)
{
    return g_init.applicationType;
}

void *sceKernelInitDiscImage(void)
{
    return g_init.discModAddr;
}

s32 sceKernelInitLptSummary(void)
{
    return g_init.lptSummary;
}

void *sceKernelInitParamSfo(SceSize *pSize)
{
    if (pSize != NULL)
        *pSize = g_init.paramSfoSize;
    return g_init.paramSfoBase;
}

