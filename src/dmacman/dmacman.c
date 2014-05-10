#include <dmacman.h>
#include <common_imp.h>

#define DMACMAN_VERSION_MAJOR   (1)
#define DMACMAN_VERSION_MINOR   (18)

SCE_MODULE_INFO("sceDMAManager", SCE_MODULE_KERNEL | SCE_MODULE_ATTR_EXCLUSIVE_START |
                                 SCE_MODULE_ATTR_EXCLUSIVE_LOAD | SCE_MODULE_ATTR_CANT_STOP, 
                                 DMACMAN_VERSION_MAJOR, DMACMAN_VERSION_MINOR);
SCE_MODULE_BOOTSTART("_sceDmacManModuleStart");
SCE_MODULE_REBOOT_BEFORE("_sceDmacManModuleRebootBefore");
SCE_SDK_VERSION(SDK_VERSION);

s32 _sceDmacManModuleStart(SceSize args __attribute__((unused)), void *argp __attribute__((unused)))
{
    return SCE_ERROR_OK;
}

s32 _sceDmacManModuleRebootBefore(SceSize args __attribute__((unused)), void *argp __attribute__((unused)))
{
    return SCE_ERROR_OK;
}

//0x300
void DmacManForKernel_32757C57() { }

//0x328
void sceKernelDmaSoftRequest() { }

//0x388
int sceKernelDmaOpFree(u32 *arg0) { }

//0x488
int sceKernelDmaOpEnQueue(u32 *arg0) { }

//0x5D4
int sceKernelDmaOpDeQueue(u32 *arg0) { }

//0x6DC
void sceKernelDmaOpAllCancel() { }

//0x798
int sceKernelDmaOpSetCallback(u32* arg0, int (*)(int, int) arg1, int arg2) { }

//0x808
void sceKernelDmaOpSetupMemcpy() { }

//0x8A8
void sceKernelDmaOpSetupNormal() { }

//0x938
int sceKernelDmaOpSetupLink(u32 *arg0, int arg1, u32 *arg2) { }

//0xA64
void sceKernelDmaOpSync() { }

//0xBCC
static void sub_BCC() { }

//0xC70
int sceKernelDmaOpQuit(u32 *arg0) { }

//0xD80
int DmacManForKernel_E18A93A5(void *arg0, void *arg1) { }

//0xFC0
void sceKernelDmaOpAssignMultiple() { }

//0x14f4
static void sub_14F4() { }

//0x1804
void sceKernelDmaChExclude() { }

//0x18CC
void sceKernelDmaChReserve() { }

//0x1964
u32 *sceKernelDmaOpAlloc() { }

//0x19E8
int sceKernelDmaOpAssign(u32 *arg0, int arg1, int arg2, int arg3, int arg4) { }

//0x1A60
void DmacManForKernel_1FC036B7() { }

//0x1ADC
void sceKernelDmaOnDebugMode() { }

//0x1C14
void sub_1C14() { }