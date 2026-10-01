/* Copyright (C) The uOFW team
   See the file COPYING for copying permission.
*/

/*
 * uofw/src/kd/usbacc/usbacc.c
 *
 * sceUSB_Acc_Driver - USB accessory driver.
 *
 * Registers a USB bus driver for PSP accessories (camera, GPS, microphone,
 * etc.). Handles USB control requests (descriptor replies), manages the
 * accessory info block and the registered accessory types.
 */

#include <common_imp.h>
#include <interruptman.h>
#include <sysmem_utils_kernel.h>
#include <sysmem_user.h>
#include <threadman_kernel.h>
#include <usbbus.h>

SCE_MODULE_INFO("sceUSB_Acc_Driver", SCE_MODULE_KERNEL | SCE_MODULE_ATTR_EXCLUSIVE_LOAD
                                              | SCE_MODULE_ATTR_EXCLUSIVE_START, 1, 3);
SCE_SDK_VERSION(SDK_VERSION);

// Error codes are derived from -> https://github.com/xerpi/psp-uvc-usb-video-class/blob/master/include/usb.h
#define SCE_ERROR_USB_INVALID_ARGUMENT        0x80243002
#define SCE_ERROR_USB_DRIVER_NOT_FOUND        0x80243005
#define SCE_ERROR_USB_BUS_DRIVER_NOT_STARTED  0x80243007

/* uOFW note: exact SCE name unknown, value verified from disassembly. */
#define SCE_ERROR_USB_BUS_NOT_READY           0x80243701

/* uOFW note: community USB headers call this PSP_USB_ERROR_ALREADY_DONE, value verified from disassembly. */
#define SCE_ERROR_USB_ALREADY_DONE            0x80243001

/* FPL for building USB descriptors at driver start. */
#define USBACC_FPL_SIZE                       368

/* Size of a received USB setup packet. */
#define USBACC_SETUP_SIZE                     8

/* Exact SCE names unknown for these bus-driver imports (TODO: move to usbbus.h once named). */
s32 sceUsbBus_driver_48CCE3C1(void);
s32 sceUsbBus_driver_7B87815D(void);
s32 sceUsbBus_driver_90B82F55(void *cb);
s32 sceUsbBus_driver_FBA2072B(void);
int sceUsbbdReqRecv(struct UsbdDeviceReq *req);

/* USB device descriptor template (from 6.60 kd/usbacc.prx .rodata). */
static const u8 g_devDescTemplate[20] = {
    0x12, 0x01, 0x00, 0x02, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00,
    0x00, 0x01, 0x00, 0x00
};

/* USB configuration descriptor template (from 6.60 kd/usbacc.prx .rodata). */
static const u8 g_cfgDescTemplate[24] = {
    0x09, 0x02, 0x19, 0x00, 0x01, 0x01, 0x00, 0xC0,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
};

/* USB interface descriptor template (from 6.60 kd/usbacc.prx .rodata). */
static const u8 g_ifDescTemplate[12] = {
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x01, 0x00, 0x00, 0x00
};

/* Second interface descriptor area: 16 meaningful bytes + 32 zero bytes
   (the original copies 48 bytes from here; from 6.60 kd/usbacc.prx .rodata). */
static const u8 g_ifDescTemplate2[48] = {
    0x09, 0x04, 0x00, 0x00, 0x01, 0xFF, 0x00, 0x00,
    0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
};

/* USB endpoint descriptor template (from 6.60 kd/usbacc.prx .rodata). */
static const u8 g_epDescTemplate[32] = {
    0x07, 0x05, 0x81, 0x03, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
};

/* Control-request match table: {bmRequestType, bRequest} pairs (from 6.60 kd/usbacc.prx .rodata). */
typedef struct {
    u8 type;
    u8 req;
    u8 unk2[6];
} UsbAccReqEntry;

static const UsbAccReqEntry g_reqTable[2] = {
    { 0xC1, 0x01, { 0, 0, 0, 0, 0, 0 } },
    { 0x41, 0x01, { 0, 0, 0, 0, 0, 0 } }
};

/* USB string descriptor for the accessory (kept in .data like the original). */
static struct StringDescriptor g_strDesc = {
    0x1C, 0x03,
    { 'U', 'S', 'B', ' ', 'A', 'c', 'c', 'e', 's', 's', 'o', 'r', 'y' }
};

// Globals
static struct UsbEndpoint g_endpoints[2] = {
    { 0, 0, 0 },
    { 1, 0, 0 }
};

/* Interface list (kept in .data like the original, whose image carries
   infp[0] = NULL, infp[1] = NULL, num = 1; no code in this module ever
   touches it, so the file values are the runtime values). */
static struct UsbInterfaces g_interfaces __attribute__((section(".data"))) = {
    .infp = { NULL, NULL },
    .num = 1
};

static int recvCtl(int arg1, int arg2, struct DeviceRequest *req);
static int startFunc(int size, void *args);
static int stopFunc(int size, void *args);
static int attachFunc(int speed, void *arg2, void *arg3);
static int detachFunc(int arg1, int arg2, int arg3);
static void recvComplete(struct UsbdDeviceReq *req);
static u8 *setupDescriptors(s32 speed, u8 *devDst, u8 **work, u8 *cfgDst);

struct UsbDriver g_drv = {
    .name = "USBAccBaseDriver",
    .endpoints = 2,
    .endp = g_endpoints,
    .intp = (struct UsbInterface *)&g_interfaces,
    .devp_hi = NULL,
    .confp_hi = NULL,
    .devp = NULL,
    .confp = NULL,
    .str = &g_strDesc,
    .recvctl = recvCtl,
    .func28 = NULL,
    .attach = attachFunc,
    .detach = detachFunc,
    .unk34 = 0,
    .start_func = startFunc,
    .stop_func = stopFunc,
    .link = NULL
};

u64 g_unk0; // last received accessory info
u8 g_setupBuf[USBACC_SETUP_SIZE]; // last received setup packet
struct UsbdDeviceReq g_recvReq; // posted async receive request
void *g_fplBlock; // descriptor scratch block from FPL
SceUID g_fplId; // FPL id
u16 g_type;
u8 g_unk2; // set to 1 when the driver starts, cleared on stop; never read back
u8 g_usbBusDriverStarted;

// Subroutine sceUsbAccGetAuthStat - Address 0x00000000 - Aliases: sceUsbAcc_79A1C743, sceUsbAcc_driver_79A1C743 -- Done
// Exported in sceUsbAcc_internal, sceUsbAcc and sceUsbAcc_driver
/*
 * Returns the USB accessory auth status.
 *
 * Returns 0 on success.
 */
s32 sceUsbAccGetAuthStat(void)
{
    s32 ret;
    u8 started = g_usbBusDriverStarted;
    int intr = sceKernelCpuSuspendIntr();

    if (started) {
        ret = (sceUsbBus_driver_8A3EB5D2(started) == 0) ? SCE_ERROR_USB_BUS_NOT_READY : 0;
    } else {
        ret = SCE_ERROR_USB_BUS_DRIVER_NOT_STARTED;
    }

    sceKernelCpuResumeIntr(intr);
    return ret;
}

// Subroutine sceUsbAccGetInfo - Address 0x00000068 - Aliases: sceUsbAcc_0CD7D4AA, sceUsbAcc_driver_0CD7D4AA -- Done
// Exported in sceUsbAcc_internal, sceUsbAcc and sceUsbAcc_driver
/*
 * Copies the 8-byte accessory info into arg when the caller address is valid.
 *
 * Returns 0 on success.
 */
s32 sceUsbAccGetInfo(u64 *arg)
{
    s32 ret = 0;
    u8 started = g_usbBusDriverStarted;
    int intr = sceKernelCpuSuspendIntr();

    if (started) {
        if (sceUsbBus_driver_8A3EB5D2(intr) != 0) {
            /* The original reads K1 here, masks the caller's range with
               (K1 << 11) and accepts it only while the masked value's sign
               bit is clear. It never shifts K1 itself, it only restores it. */
            u32 k1 = (u32)pspGetK1();

            if (arg == NULL || ((s32)((((u32)arg + 8) | (u32)arg) & (k1 << 11))) < 0) {
                ret = SCE_ERROR_USB_INVALID_ARGUMENT;
            } else {
                /* Byte copy: like the original, this tolerates a pointer
                   that is not word-aligned. */
                u8 *src = (u8 *)&g_unk0;
                u8 *dst = (u8 *)arg;
                s32 i;

                for (i = 0; i < 8; i++)
                    dst[i] = src[i];
            }

            pspSetK1((int)k1);
            sceKernelCpuResumeIntr(intr);
        }
        else {
            sceKernelCpuResumeIntr(intr);
            ret = SCE_ERROR_USB_BUS_NOT_READY;
        }
    }
    else {
        sceKernelCpuResumeIntr(intr);
        ret = SCE_ERROR_USB_BUS_DRIVER_NOT_STARTED;
    }

    return ret;
}

// Subroutine sceUsbAcc_internal_2A100C1F - Address 0x00000154 -- Done
// Exported in sceUsbAcc_internal
/*
 * Sends an accessory device request after validating and fixing up its size.
 *
 * The original queues the request on g_endpoints[1] (the second registered
 * endpoint, loaded from 0x0CAC in the 6.60 binary), not the first one.
 *
 * Returns 0 on success.
 */
s32 sceUsbAcc_internal_2A100C1F(struct UsbdDeviceReq *req)
{
    s32 ret = 0;
    u8 *data = req->data;

    if (g_usbBusDriverStarted) {
        if ((data[3]) < 0x3D) {
            sceKernelDcacheWritebackRange(data, req->size);
            req->endp = &g_endpoints[1];
            req->size = data[3] + 4;
            ret = sceUsbbdReqSend(req);
        }
        else
            ret = SCE_ERROR_USB_INVALID_ARGUMENT;
    }
    else
        ret = SCE_ERROR_USB_BUS_DRIVER_NOT_STARTED;

    return ret;
}

/*
 * Handles a received USB control request (bmRequestType/bRequest dispatch,
 * descriptor replies, re-arming the receive).
 *
 * Returns 0 on success, -1 when the request is not handled.
 */
static int recvCtl(int arg1 __attribute__((unused)), int arg2, struct DeviceRequest *req)
{
    u8 *setup = g_setupBuf;
    u8 *data;
    s32 i;
    int matched = 0;
    UsbAccReqEntry *entry = (UsbAccReqEntry *)g_reqTable;

    memcpyInline(setup, req, USBACC_SETUP_SIZE);

    if (arg2 < 0)
        return -1;

    for (i = 0; i < 2; i++, entry++) {
        if (setup[0] != entry->type)
            continue;
        if (req->bRequest != entry->req)
            continue;
        matched = 1;
        break;
    }

    if (!matched)
        return -1;

    if ((s8)setup[0] >= 0) {
        /* OUT request: only bRequest 1 re-arms the receive. The check can
           never fail here (both table entries carry bRequest 1 and the
           request already matched one of them), but the original has it. */
        if (req->bRequest != 1)
            return -1;

        data = g_recvReq.data;
        sceKernelDcacheInvalidateRange(data, 64);
        g_recvReq.size = 64;
        sceUsbbdReqRecv(&g_recvReq);
        return 0;
    }

    /* IN request: reply with the request byte, or with 0 when the accessory
       type the host selected is fully registered. */
    if (req->bRequest != 1)
        return -1;

    data = g_recvReq.data;
    if (sceUsbBus_driver_48CCE3C1() == 0) {
        data[0] = req->bRequest;
    } else {
        if ((g_type & 0xFFFF) == (g_type & req->wValue)) {
            data[0] = 0;
            sceUsbBus_driver_FBA2072B();
        } else {
            data[0] = req->bRequest;
        }
    }

    sceKernelDcacheWritebackRange(data, 1);
    g_recvReq.size = 1;
    sceUsbbdReqSend(&g_recvReq);
    return 0;
}

/*
 * Starts the accessory driver: allocates the descriptor scratch block,
 * builds the descriptors and posts the initial receive request.
 *
 * Returns 0 on success, -1 on failure.
 */
static int startFunc(int size __attribute__((unused)), void *args __attribute__((unused)))
{
    /* The attribute value 256 has no named constant in any known PSP SDK or
       uOFW header; it is taken verbatim from the 6.60 disassembly. */
    SceUID fpl = sceKernelCreateFpl("SceUsbAcc", SCE_KERNEL_PRIMARY_KERNEL_PARTITION, 256,
                                    USBACC_FPL_SIZE, 1, NULL);
    if (fpl < 0)
        return -1;

    g_fplId = fpl;

    if (sceKernelTryAllocateFpl(fpl, &g_fplBlock) < 0) {
        sceKernelDeleteFpl(g_fplId);
        return -1;
    }

    {
        u8 *block = g_fplBlock;
        u8 *res;
        u8 *res2;

        /* The 368-byte block holds the 64-byte receive buffer at its start
           (g_recvReq.data), then two 152-byte descriptor builds laid out as
           dev(20) work(16) cfg(24) if1(12) if2(48) ep(32). The first build
           (speed 2) is the high-speed set, the second (speed 1) the
           full-speed set. The original also passes cfgDst again as an
           unused fifth argument. */
        g_drv.devp_hi = block + 64;
        g_drv.confp_hi = block + 84;

        res = setupDescriptors(2, block + 64, (u8 **)(block + 84), block + 100);
        res2 = setupDescriptors(1, res, (u8 **)(res + 20), res + 36);
        (void)res2;
        g_drv.devp = res;
        g_drv.confp = res + 20;
    }

    g_unk0 = 0;
    g_usbBusDriverStarted = 0;

    g_endpoints[0].unk3 = 0;
    g_endpoints[1].unk3 = 0;

    g_recvReq.endp = &g_endpoints[0];
    g_recvReq.data = g_fplBlock;
    g_recvReq.size = 64;
    g_recvReq.unkc = 1;
    g_recvReq.func = recvComplete;
    g_recvReq.recvsize = 0;
    g_recvReq.retcode = 0;
    g_recvReq.unk1c = 0;
    g_recvReq.arg = NULL;
    sceUsbBus_driver_90B82F55(recvComplete);
    g_unk2 = 1;

    return 0;
}

// Subroutine sceUsbAccRegisterType - Address 0x000004AC -- Done
// Exported in sceUsbAcc_internal
/*
 * Registers an accessory type bit.
 *
 * Returns 0 on success.
 */
s32 sceUsbAccRegisterType(u16 type)
{
    u16 cur = g_type;
    s32 ret = 0;

    if ((cur & type) != 0)
        ret = SCE_ERROR_USB_ALREADY_DONE;
    else
        g_type = cur | type;

    return ret;
}

// Subroutine sceUsbAccUnregisterType - Address 0x000004E0 -- Done
// Exported in sceUsbAcc_internal
/*
 * Unregisters an accessory type bit.
 *
 * Returns 0 on success.
 */
s32 sceUsbAccUnregisterType(u16 type)
{
    s32 ret = 0;

    if ((g_type & type) != 0)
        g_type = g_type & ~type;
    else
        ret = SCE_ERROR_USB_DRIVER_NOT_FOUND;

    return ret;
}

// Subroutine module_start - Address 0x00000518 -- Done
/*
 * Registers the accessory USB driver with the USB bus driver.
 *
 * Returns 0 on success.
 */
s32 module_start(SceSize args __attribute__((unused)), void *argp __attribute__((unused)))
{
    if ((sceUsbbdRegister(&g_drv)) >= 0) {
        g_type = 0;
        g_unk2 = 0;
        return 0;
    }

    return 1;
}

// Subroutine module_stop - Address 0x00000558 -- Done
/*
 * Unregisters the accessory USB driver from the USB bus driver.
 *
 * Returns 0 on success.
 */
s32 module_stop(SceSize args __attribute__((unused)), void *argp __attribute__((unused)))
{
    if (sceUsbbdUnregister(&g_drv) >= 0)
        return 0;

    return 1;
}

/*
 * Stops the accessory driver: releases the descriptor scratch block.
 *
 * Like the original, this does not clear g_drv.devp/confp (or the hi
 * variants); they are left pointing into the freed block until the next
 * start.
 *
 * Returns 0 on success.
 */
static int stopFunc(int size __attribute__((unused)), void *args __attribute__((unused)))
{
    sceUsbBus_driver_7B87815D();
    sceKernelDeleteFpl(g_fplId);
    g_unk2 = 0;

    return 0;
}

/*
 * Completion callback for the posted receive request: stores newly
 * received accessory info.
 */
static void recvComplete(struct UsbdDeviceReq *req)
{
    if (req->retcode != 0)
        return;

    if ((s8)g_setupBuf[0] < 0)
        return;

    /* Only an OUT request with bRequest 1 carries a new accessory info
       block (the original copies when setupBuf[1] == 1). */
    if (g_setupBuf[1] == 1) {
        u8 *data = req->data;
        u64 *dst = &g_unk0;
        s32 i;

        for (i = 0; i < 8; i++)
            ((u8 *)dst)[i] = data[i];
    }
}

/*
 * Marks the driver as attached.
 *
 * Returns the previous attached state: 0 when the driver was not attached
 * (in which case it has now been marked as attached), 1 when it already
 * was.
 */
static int attachFunc(int speed __attribute__((unused)), void *arg2 __attribute__((unused)),
                      void *arg3 __attribute__((unused)))
{
    u8 ret = g_usbBusDriverStarted;

    if (ret == 0)
        g_usbBusDriverStarted = 1;

    return ret;
}

/*
 * Marks the driver as detached and clears the runtime state.
 *
 * The original returns early, without touching anything, when the driver
 * was never attached.
 *
 * Returns 0 on success (the original leaves an undefined value in v0).
 */
static int detachFunc(int arg1 __attribute__((unused)), int arg2 __attribute__((unused)),
                      int arg3 __attribute__((unused)))
{
    if (g_usbBusDriverStarted == 0)
        return 0;

    g_usbBusDriverStarted = 0;
    g_unk0 = 0;
    g_endpoints[0].unk3 = 0;
    g_endpoints[1].unk3 = 0;

    return 0;
}

/*
 * Builds one USB descriptor set into the FPL scratch block.
 *
 * devDst receives the 20-byte device descriptor, work receives the four
 * resulting descriptors as { cfg, if1, if2, ep }, and cfgDst receives the
 * configuration descriptor followed by both interface descriptors and the
 * endpoint descriptor:
 *
 *   cfgDst+0   cfg  (24)   cfgDst+24  if1 (12)
 *   cfgDst+36  if2  (48)   cfgDst+84  ep  (32)
 *
 * The original also passes cfgDst a fifth time as an unused stack
 * argument; nothing reads it, so it is not reproduced here.
 *
 * Returns cfgDst + 116 (one byte past the endpoint descriptor).
 */
static u8 *setupDescriptors(s32 speed, u8 *devDst, u8 **work, u8 *cfgDst)
{
    u8 *if1Dst = cfgDst + 24;
    u8 *if2Dst = cfgDst + 36;
    u8 *epDst = cfgDst + 84;

    work[0] = cfgDst;
    work[1] = if1Dst;
    work[2] = if2Dst;
    work[3] = epDst;

    /* Copy the 20-byte device descriptor template. */
    memcpyInline(devDst, g_devDescTemplate, sizeof(g_devDescTemplate));

    /* Copy the 24-byte configuration descriptor template. */
    memcpyInline(cfgDst, g_cfgDescTemplate, sizeof(g_cfgDescTemplate));

    /* Copy the 12-byte interface descriptor template. */
    memcpyInline(if1Dst, g_ifDescTemplate, sizeof(g_ifDescTemplate));

    /* Copy the 48-byte second descriptor area. */
    memcpyInline(if2Dst, g_ifDescTemplate2, 48);

    /* Copy the 32-byte endpoint descriptor template. */
    memcpyInline(epDst, g_epDescTemplate, sizeof(g_epDescTemplate));

    /* Link the descriptor chain: cfg -> if1 -> if2 -> ep. */
    *(u8 **)(cfgDst + 12) = if1Dst;
    *(u8 **)if1Dst = if2Dst;
    *(u8 **)(if2Dst + 12) = epDst;

    /* Patch the fields the original sets at run time. The endpoint's
       wMaxPacketSize is 64 for both speeds; only bInterval differs. */
    devDst[7] = 64;
    *(u16 *)(epDst + 4) = 64;
    epDst[6] = (speed == 2) ? 7 : 8;

    return epDst + 32;
}
