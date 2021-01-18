#include <common_imp.h>
#include <interruptman.h>
#include <sysmem_utils_kernel.h>

SCE_MODULE_INFO("sceUSB_Acc_Driver", SCE_MODULE_KERNEL | SCE_MODULE_ATTR_EXCLUSIVE_LOAD | SCE_MODULE_ATTR_EXCLUSIVE_START, 1, 3);
SCE_SDK_VERSION(SDK_VERSION);

// Error codes are derived from -> https://github.com/xerpi/psp-uvc-usb-video-class/blob/master/include/usb.h
#define SCE_ERROR_USB_DRIVER_NOT_FOUND        0x80243005
#define SCE_ERROR_USB_BUS_DRIVER_NOT_STARTED  0x80243007

/** USB driver endpoint */
struct UsbEndpoint
{
	/** Endpoint number (must be filled in sequentially) */
	int endpnum; 
	/** Filled in by the bus driver */
	int unk2; 
	/** Filled in by the bus driver */
	int unk3;
};

/** USB Interface descriptor */
struct InterfaceDescriptor
{
	unsigned char bLength;
	unsigned char bDescriptorType;
	unsigned char bInterfaceNumber;
	unsigned char bAlternateSetting;
	unsigned char bNumEndpoints;
	unsigned char bInterfaceClass;
	unsigned char bInterfaceSubClass;
	unsigned char bInterfaceProtocol;
	unsigned char iInterface;
} __attribute__((packed));

/** USB driver interfaces structure */
struct UsbInterfaces
{
	/** Pointers to the individual interface descriptors */
	struct InterfaceDescriptor *infp[2];
	/** Number of interface descriptors */
	unsigned int num;
};

/** USB string descriptor */
struct StringDescriptor
{
	unsigned char bLength;
	unsigned char bDescriptorType;
	short bString[32];
} __attribute__((packed));

/** USB EP0 Device Request */
struct DeviceRequest
{
	unsigned char bmRequestType;
	unsigned char bRequest;
	unsigned short wValue;
	unsigned short wIndex;
	unsigned short wLength;
} __attribute__((packed));

struct UsbDriver
{
	/** Name of the USB driver */
	const char *name; 
	/** Number of endpoints in this driver (including default control) */
	int endpoints;
	/** List of endpoint structures (used when calling other functions) */
	struct UsbEndpoint *endp; 
	/** Interface list */
	struct UsbInterface *intp;	  
	/** Pointer to hi-speed device descriptor */
	void *devp_hi;  
	/** Pointer to hi-speed device configuration */
	void *confp_hi;	
	/** Pointer to full-speed device descriptor */
	void *devp;     
	/** Pointer to full-speed device configuration */
	void *confp;    
	/** Default String descriptor */
	struct StringDescriptor *str; 
	/** Received a control request arg0 is endpoint, arg1 is possibly data arg2 is data buffer */
	int (*recvctl)(int arg1, int arg2, struct DeviceRequest *req);
	/** Unknown */
	int (*func28)(int arg1, int arg2, int arg3);  
	/** Configuration set (attach) function */
	int (*attach)(int speed, void *arg2, void *arg3); 
	/** Configuration unset (detach) function */
	int (*detach)(int arg1, int arg2, int arg3);
	/** Unknown set to 0 */
	int unk34;
	/** Function called when the driver is started */
	int (*start_func)(int size, void *args);
	/** Function called when the driver is stopped */
	int (*stop_func)(int size, void *args);  
	/** Link to next USB driver in the chain, set to NULL */
	struct UsbDriver *link; 
};

/** USB device request, used by ::sceUsbbdReqSend and ::sceUsbbdReqRecv. */
struct UsbdDeviceReq
{
	/** Pointer to the endpoint to queue request on */
	struct UsbEndpoint *endp; 
	/** Pointer to the data buffer to use in the request */
	void *data;
	/** Size of the data buffer (send == size of data, recv == size of max receive) */
	int  size; 
	/** Unknown */
	int  unkc; 
	/** Pointer to the function to call on completion */
	void *func;
	/** Resultant size (send == size of data sent, recv == size of data received) */
	int  recvsize;
	/** Return code of the request, 0 == success, -3 == cancelled */
	int  retcode; 
	/** Unknown */
	int  unk1c;
	/** A user specified pointer for the device request */
	void *arg;
	/** Link pointer to next request used by the driver, set it to NULL */
	void *link;
};

// Function prototypes
s32 sceUsbAcc_79A1C743(void)        __attribute__((alias("sceUsbAcc_internal_79A1C743")));
s32 sceUsbAcc_driver_79A1C743(void) __attribute__((alias("sceUsbAcc_internal_79A1C743")));
s32 sceUsbAcc_0CD7D4AA(void)        __attribute__((alias("sceUsbAcc_internal_0CD7D4AA")));
s32 sceUsbAcc_driver_0CD7D4AA(void) __attribute__((alias("sceUsbAcc_internal_0CD7D4AA")));

int sceUsbBus_driver_B1644BE7(struct UsbDriver *drv);     // sceUsbbdRegister
int sceUsbBus_driver_C1E2A540(struct UsbDriver *drv);     // sceUsbbdUnregister
int sceUsbBus_driver_23E51D8F(struct UsbdDeviceReq *req); // sceUsbbdReqSend
int sceUsbBus_driver_8A3EB5D2(void);

// Globals
struct UsbDriver g_drv;     // 0x00000CC4
u8 unk0;                    // 0x00000D53
u16 unk1;                   // 0x00000D60
u8  unk2;                   // 0x00000D62
struct UsbdDeviceReq g_req; // 0x00000CAC

// Subroutine sceUsbAcc_internal_79A1C743 - Address 0x00000000 - Aliases: sceUsbAcc_79A1C743, sceUsbAcc_driver_79A1C743 -- sceUsbAccGetAuthStat
// Exported in sceUsbAcc_internal, sceUsbAcc and sceUsbAcc_driver
s32 sceUsbAcc_internal_79A1C743(void)
{
    int intr = sceKernelCpuSuspendIntr();
    s32 ret = SCE_ERROR_USB_BUS_DRIVER_NOT_STARTED;
    
    if (unk0) {
        ret = 0;
        if (sceUsbBus_driver_8A3EB5D2() == 0)
            ret = 0x80243701;
    }
    
    sceKernelCpuResumeIntr(intr);
    return ret;
}

// Subroutine sceUsbAcc_internal_0CD7D4AA - Address 0x00000068 - Aliases: sceUsbAcc_0CD7D4AA, sceUsbAcc_driver_0CD7D4AA
// Exported in sceUsbAcc_internal, sceUsbAcc and sceUsbAcc_driver
s32 sceUsbAcc_internal_0CD7D4AA(void)
{
    return 0;
}

// Subroutine sceUsbAcc_internal_2A100C1F - Address 0x00000154
// Exported in sceUsbAcc_internal
s32 sceUsbAcc_internal_2A100C1F(void)
{
    return 0;
}

// Subroutine sceUsbAcc_internal_2E251404 - Address 0x000004AC -- sceUsbAccRegisterType
// Exported in sceUsbAcc_internal
s32 sceUsbAcc_internal_2E251404(u16 type)
{
    s32 error = SCE_ERROR_USB_DRIVER_NOT_FOUND;

    if ((unk1 & type) == 0) {
        error = 0;
        unk1 = unk1 | type;
    }
    
    return error;
}

// Subroutine sceUsbAcc_internal_18B04C82 - Address 0x000004E0 -- sceUsbAccUnregisterType
// Exported in sceUsbAcc_internal
s32 sceUsbAcc_internal_18B04C82(u16 type)
{
    s32 error = SCE_ERROR_USB_DRIVER_NOT_FOUND;
    
    if ((unk1 & type) != 0) {
        error = 0;
        unk1 = unk1 & ~type;
    }
    
    return error;
}

// Subroutine module_start - Address 0x00000518
s32 module_start(SceSize args __attribute__((unused)), void *argp __attribute__((unused)))
{
    s32 ret = 0;
    if ((ret = sceUsbBus_driver_B1644BE7(&g_drv)) >= 0) {
        unk1 = 0;
        unk2 = 0;
    }
    
    return (ret < 0)? 1: 0;
}

// Subroutine module_stop - Address 0x00000558
s32 module_stop(SceSize args __attribute__((unused)), void *argp __attribute__((unused)))
{
    int ret = sceUsbBus_driver_C1E2A540(&g_drv);
    return (ret < 0)? 1 : 0;
}
