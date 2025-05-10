#include <common_imp.h>
#include <modulemgr_kernel.h>
#include <registry.h>
#include <sysmem_kernel.h>
#include <sysmem_sysclib.h>

SCE_MODULE_INFO("sceMlnBridge_MSApp_Driver", SCE_MODULE_KERNEL | SCE_MODULE_ATTR_EXCLUSIVE_START | SCE_MODULE_ATTR_EXCLUSIVE_LOAD, 1, 1);
SCE_MODULE_BOOTSTART("sceMlnBridge_msapp_driver_0ED6A564");
SCE_MODULE_STOP("sceMlnBridge_msapp_driver_C41F1B67");
SCE_SDK_VERSION(SDK_VERSION);

// Headers
extern s32 sceRtcGetCurrentSecureTick(u64 *tick);	//sceRtc_driver_CEEF238F
extern s32 sceDve_driver_253B69B6(u32, u32, u32);

typedef struct {
    const char *path;
    unsigned int args;
    const void *argp;
} ModuleInfo;

// Subroutine sub_00000318 - Address 0x00000318
int sub_00000318(ModuleInfo *moduleInfo) {
    if (moduleInfo == NULL || moduleInfo->path == NULL) {
        return -1;
    }
    
    SceKernelLMOption option = { sizeof(SceKernelLMOption), 0, 0, 0, 0, 0, {0} };
    SceUID modId = sceKernelLoadModuleForKernel(moduleInfo->path, 0, &option);
    if (modId < 0) {
        return modId;
    }
    
    s32 res = 0;
    modId = sceKernelStartModule(modId, moduleInfo->args, moduleInfo->argp, &res, NULL);
    
    if (modId <= 0) {
        sceKernelUnloadModule(modId);
        return modId;
    }
    
    return modId;
}

// Subroutine sub_000003E0 - Address 0x000003E0
u32 sub_000003E0(const char *dirName, const char *keyName, void *data, SceSize len) {
    s32 res = 0;
    struct RegParam regParam = { 0 };
    u32 regHandle = 0, catHandle = 0, keyHandle = 0, type = 0;
    SceSize size = 0;
    
    memset(&regParam, 0, sizeof(regParam));
    strncpy(regParam.name, "/system", 255);
    
    res = sceRegOpenRegistry(&regParam, 2, &regHandle);
    
    if (res == 0) {    
        res = sceRegOpenCategory(regHandle, dirName, 2, &catHandle);
        
        if (res == 0) {
            res = sceRegGetKeyInfo(catHandle, keyName, &keyHandle, &type, &size);
            
            if (((res == 0) && (type == REG_TYPE_BIN)) && (size <= len)) {
                res = sceRegGetKeyValue(catHandle, keyHandle, data, len);
                
                if (res == 0) {
                    sceRegFlushCategory(catHandle);
                    sceRegCloseCategory(catHandle);
                    sceRegFlushRegistry(regHandle);
                    sceRegCloseRegistry(regHandle);
                    return 0;
                }
            }
            
            sceRegCloseCategory(catHandle);
        }
        
        sceRegCloseRegistry(regHandle);
    }
    
    return -1;
}

/*
  Subroutine sceMlnBridge_msapp_D527DEB0 - Address 0x00000000 
  Exported in sceMlnBridge_msapp
 */
s32 sceMlnBridge_msapp_D527DEB0(const char *ptr, s32 size) {
    char data[5];
    s32 res = SCE_ERROR_PRIV_REQUIRED;
    s32 oldK1 = pspShiftK1();

    if ((pspK1DynBufOk(ptr, size)) >= 0) {
        if (size >= 4) {
            // Read password data from registry
            res = sub_000003E0("/CONFIG/SYSTEM/LOCK", "password", data, 5);

            // Compare the first four bytes and result value
            if ((res == 0) && (ptr[0] == data[0]) && (ptr[1] == data[1]) && (ptr[2] == data[2]) && (ptr[3] == data[3])) {
                pspSetK1(oldK1);
                return res;
            }
        }
        else {
            res = SCE_ERROR_INVALID_SIZE; // Handle size error
        }
    }

    pspSetK1(oldK1);
    return res;
}

/*
 Subroutine module_start - Address 0x000000F0 - Aliases: sceMlnBridge_msapp_driver_0ED6A564
 Exported in syslib
 Exported in sceMlnBridge_msapp_driver
 */
s32 sceMlnBridge_msapp_driver_0ED6A564(SceSize args __attribute__((unused)), const void *argp __attribute__((unused))) {
    return SCE_ERROR_OK;
}

/*
 Subroutine module_stop - Address 0x000000F8 - Aliases: sceMlnBridge_msapp_driver_C41F1B67
 Exported in syslib
 Exported in sceMlnBridge_msapp_driver
 */
s32 sceMlnBridge_msapp_driver_C41F1B67(SceSize args __attribute__((unused)), const void *argp __attribute__((unused))) {
    return SCE_ERROR_OK;
}

/*
 Subroutine sceMlnBridge_msapp_3811BA77 - Address 0x00000100
 Exported in sceMlnBridge_msapp
 */
s32 sceMlnBridge_msapp_3811BA77(u64 *tick) {
    s32 res = SCE_ERROR_PRIV_REQUIRED;
    s32 oldK1 = pspShiftK1();
    
    //0x128
    if (pspK1StaBufOk(tick, 8) >= 0) {
        res = sceRtcGetCurrentSecureTick(tick);
    }
    
    //0x138
    pspSetK1(oldK1);
    return res;
}

/*
 Subroutine sceMlnBridge_msapp_F02B9478 - Address 0x0000014C 
 Exported in sceMlnBridge_msapp
 0x00000814: "flash0:/vsh/module/mlncmn.prx"
 0x00000834: "flash0:/vsh/module/mcore.prx"
 */
s32 sceMlnBridge_msapp_F02B9478(u32 arg0) {
    s32 res = -1;
    ModuleInfo modules[2] = {
        { "flash0:/vsh/module/mlncmn.prx", 0, NULL },
        { "flash0:/vsh/module/mcore.prx", 0, NULL}
    };
    
    s32 oldK1 = pspGetK1();
    
    //0x1B4
    if (arg0 < 4) {
        oldK1 = pspShiftK1();
        res = sub_00000318(&modules[arg0]);
        pspSetK1(oldK1);
    }
    
    //0x1C8
    return res;
}

/*
 ok
 Subroutine sceMlnBridge_msapp_CC6037D7 - Address 0x000001D8 
 Exported in sceMlnBridge_msapp
 "flash0:/kd/np_commerce2_store.prx" ref
 */
s32 sceMlnBridge_msapp_CC6037D7(void) {
    s32 res = 0;
    ModuleInfo module = {
        "flash0:/kd/np_commerce2_store.prx",
        0,
        NULL
    };
    
    s32 oldK1 = pspShiftK1();
    res = sub_00000318(&module);
    pspSetK1(oldK1);
    return res;
}

/*
 Subroutine sceMlnBridge_msapp_494B3B0B - Address 0x00000220 
 Exported in sceMlnBridge_msapp
 */
s32 sceMlnBridge_msapp_494B3B0B(void) {
    s32 oldK1 = pspShiftK1();
    s32 model = sceKernelGetModel();
    s32 res = 0;
    
    if (model != PSP_1000 && model != PSP_11000) { // Checking if model is a PSP Phat or PSP Street
        res = sceDve_driver_253B69B6(-1, 0, 3); // This function does not exist for PSP 1000's also 4th argument is 0
        pspSetK1(oldK1);
        return model;
    }
    
    pspSetK1(oldK1);
    return res;
}
 
/*
 Subroutine sceMlnBridge_msapp_0398DEFF - Address 0x00000284  
 Exported in sceMlnBridge_msapp
*/
s32 sceMlnBridge_msapp_0398DEFF(void) {
    s32 oldK1 = pspShiftK1();
    s32 res = sceKernelGetModel();
    pspSetK1(oldK1);
    return res != PSP_1000; // Checking if model is not PSP_1000
}

/*
 Subroutine sceMlnBridge_msapp_7AD66017 - Address 0x000002B4 
 Exported in sceMlnBridge_msapp
*/
s32 sceMlnBridge_msapp_7AD66017(void) {
    s32 oldK1 = pspShiftK1();
    u32 model = sceKernelGetModel();
    pspSetK1(oldK1);
    s32 res = 0;
    
    if (model == PSP_GO || model == 5 || model == 7 || model == 9) { // Checking if model is PSP GO
        res = 1;
    }
    
    return res;
}
