#include <common_imp.h>
#include <interruptman.h>
#include <sysmem_utils_kernel.h>
#include <usbbus.h>

SCE_MODULE_INFO("sceUSB_Acc_Driver", SCE_MODULE_KERNEL | SCE_MODULE_ATTR_EXCLUSIVE_LOAD | SCE_MODULE_ATTR_EXCLUSIVE_START, 1, 3);
SCE_SDK_VERSION(SDK_VERSION);

// Error codes are derived from -> https://github.com/xerpi/psp-uvc-usb-video-class/blob/master/include/usb.h
#define SCE_ERROR_USB_INVALID_ARGUMENT        0x80243002
#define SCE_ERROR_USB_DRIVER_NOT_FOUND        0x80243005
#define SCE_ERROR_USB_BUS_DRIVER_NOT_STARTED  0x80243007

// Globals
struct UsbDriver g_drv;     // 0x00000CC4
u8 g_unk0;                  // 0x00000D53
u16 g_type;                 // 0x00000D50
u8 g_unk2;                  // 0x00000D52
struct UsbEndpoint g_endp;  // 0x00000CAC

// Subroutine sceUsbAccGetAuthStat - Address 0x00000000 - Aliases: sceUsbAcc_79A1C743, sceUsbAcc_driver_79A1C743 -- Done
// Exported in sceUsbAcc_internal, sceUsbAcc and sceUsbAcc_driver
s32 sceUsbAccGetAuthStat(void)
{
	int intr = sceKernelCpuSuspendIntr();
	s32 ret = 0;
	
	if (g_unk0) {
		if (sceUsbBus_driver_8A3EB5D2() == 0)
			ret = 0x80243701;
	}
	else
		ret = SCE_ERROR_USB_BUS_DRIVER_NOT_STARTED;
		
	sceKernelCpuResumeIntr(intr);
	return ret;
}

// Subroutine sceUsbAccGetInfo - Address 0x00000068 - Aliases: sceUsbAcc_0CD7D4AA, sceUsbAcc_driver_0CD7D4AA
// Exported in sceUsbAcc_internal, sceUsbAcc and sceUsbAcc_driver
s32 sceUsbAccGetInfo(void)
{
    return 0;
}

// Subroutine sceUsbAcc_internal_2A100C1F - Address 0x00000154 -- TODO: Verify
// Exported in sceUsbAcc_internal
s32 sceUsbAcc_internal_2A100C1F(struct UsbdDeviceReq *req)
{
    s32 ret = SCE_ERROR_USB_BUS_DRIVER_NOT_STARTED;
    u8 *data = req->data;

    if (g_unk0) {
        ret = SCE_ERROR_USB_INVALID_ARGUMENT;
        
        if ((data[3]) < 0x3D) {
            sceKernelDcacheWritebackRange(data, req->size);
            req->endp = &g_endp;
            req->size = data[3] + 4;
            ret = sceUsbbdReqSend(req);
        }
    }
    
    return ret;
}

// Subroutine sceUsbAccRegisterType - Address 0x000004AC -- Done
// Exported in sceUsbAcc_internal
s32 sceUsbAccRegisterType(u16 type)
{
    s32 error = SCE_ERROR_USB_DRIVER_NOT_FOUND;

    if ((g_type & type) == 0) {
        error = 0;
        g_type = g_type | type;
    }
    
    return error;
}

// Subroutine sceUsbAccUnregisterType - Address 0x000004E0 -- Done
// Exported in sceUsbAcc_internal
s32 sceUsbAccUnregisterType(u16 type)
{
    s32 error = SCE_ERROR_USB_DRIVER_NOT_FOUND;
    
    if ((g_type & type) != 0) {
        error = 0;
        g_type = g_type & ~type;
    }
    
    return error;
}

// Subroutine module_start - Address 0x00000518 -- Done
s32 module_start(SceSize args __attribute__((unused)), void *argp __attribute__((unused)))
{
    s32 ret = 0;
    if ((ret = sceUsbbdRegister(&g_drv)) >= 0) {
        g_type = 0;
        g_unk2 = 0;
    }
    
    return (ret < 0)? 1: 0;
}

// Subroutine module_stop - Address 0x00000558 -- Done
s32 module_stop(SceSize args __attribute__((unused)), void *argp __attribute__((unused)))
{
    int ret = sceUsbbdUnregister(&g_drv);
    return (ret < 0)? 1 : 0;
}
