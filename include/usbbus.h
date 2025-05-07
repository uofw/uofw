/* Copyright (C) The uOFW team
   See the file COPYING for copying permission.
*/

/** @defgroup USB USB management
 *  Manages the UMD drive and accessing data on it.
 */

 /** @defgroup USBBD USB Bus Driver
  *  @ingroup USB
  *
  *  The USB bus driver manages the different USB protocol drivers currently
  *  registered with it and communicates with the USB host as a single
  *  USB device.
  * 
  *  Supported USB procotol drivers are as follows:
  * 
  *      • Camera USB
  *        Capture image date from the PSP camera accessory via the USB port
  * 
  *      • GPS USB
  *        Capture position information from the PSP GPS accessory via the USB port
  * 
  *      • Microphone (PCM) USB
  *        Capture PCM data from the PSP microphone accessory via the USB port
  * 
  *      • PS2/PS3 USB
  *        Allows PSP <-> PS2/PS3 data communication via the USB port 
  *
  * @{
  */

#ifndef USBBUS_H
#define USBBUS_H

#include "common_header.h"

/**
 * USB driver endpoint
 */
struct UsbEndpoint {
	/** Endpoint number (must be filled in sequentially) */
	int endpnum; 
	/** Filled in by the bus driver */
	int unk2; 
	/** Filled in by the bus driver */
	int unk3;
};

/**
 * USB Interface descriptor
 */
struct InterfaceDescriptor {
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

/**
 * USB driver interfaces structure
 */
struct UsbInterfaces {
	/** Pointers to the individual interface descriptors */
	struct InterfaceDescriptor *infp[2];
	/** Number of interface descriptors */
	unsigned int num;
};

/**
 * USB string descriptor
 */
struct StringDescriptor {
	unsigned char bLength;
	unsigned char bDescriptorType;
	short bString[32];
} __attribute__((packed));

/**
 * USB EP0 Device Request
 */
struct DeviceRequest {
	unsigned char bmRequestType;
	unsigned char bRequest;
	unsigned short wValue;
	unsigned short wIndex;
	unsigned short wLength;
} __attribute__((packed));

/**
 * USB driver structure used by ::sceUsbbdRegister and ::sceUsbbdUnregister
 */
struct UsbDriver {
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

/**
 * USB device request, used by ::sceUsbbdReqSend and ::sceUsbbdReqRecv.
 */
struct UsbdDeviceReq {
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

/**
 * Register a USB driver.
 *
 * @param drv - Pointer to a filled out USB driver
 *
 * @return 0 on success, < 0 on error.
 */
int sceUsbbdRegister(struct UsbDriver *drv);


/**
 * Unregister a USB driver
 *
 * @param drv - Pointer to a filled out USB driver
 *
 * @return 0 on success, < 0 on error
 */
int sceUsbbdUnregister(struct UsbDriver *drv);

/**
 * Queue a send request (IN from host pov)
 *
 * @param req - Pointer to a filled out UsbdDeviceReq structure.
 *
 * @return 0 on success, < 0 on error
 */
int sceUsbbdReqSend(struct UsbdDeviceReq *req);

int sceUsbBus_driver_8A3EB5D2(void);

#endif // USBBUS_H

/** @} */
