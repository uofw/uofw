/* Copyright (C) The uOFW team
   See the file COPYING for copying permission.
*/

/*
 * uofw/src/kd/usbcam/usbcam.c
 *
 * sceUSBCam_Driver - USB camera driver.
 *
 * Provides video frames, still capture, and microphone audio to user
 * applications over the USB bus.
 */

#include <common_imp.h>
#include <dmacman.h>
#include <sysmem_kdebug.h>
#include <sysmem_sysclib.h>
#include <sysmem_utils_kernel.h>
#include <threadman_kernel.h>
#include <usbbus.h>

SCE_MODULE_INFO("sceUSBCam_Driver", SCE_MODULE_KERNEL | SCE_MODULE_ATTR_EXCLUSIVE_LOAD
                                              | SCE_MODULE_ATTR_EXCLUSIVE_START, 1, 7);
SCE_SDK_VERSION(SDK_VERSION);

/* sceUSBCam error codes (0x802439xx). Values verified against the 6.60
   disassembly; exact SCE names are unknown and the names/meanings below are
   inferred from usage (cf. usbacc.c pattern). */
#define SCE_ERROR_USBCAM_NOT_SETUP       0x80243901 /* feature not set up (setup flag clear) */
#define SCE_ERROR_USBCAM_NOT_ATTACHED    0x80243902 /* device not attached / accessory auth failed */
#define SCE_ERROR_USBCAM_INVALID_SIZE    0x80243903 /* workarea size/alignment invalid */
#define SCE_ERROR_USBCAM_INVALID_ADDR    0x80243904 /* user pointer failed K1 check */
#define SCE_ERROR_USBCAM_INVALID_RES     0x80243905 /* resolution combo invalid */
#define SCE_ERROR_USBCAM_INVALID_VALUE   0x80243906 /* parameter out of range */
#define SCE_ERROR_USBCAM_INVALID_PARAM   0x80243907 /* null/invalid parameter */
#define SCE_ERROR_USBCAM_NOT_INIT        0x80243908 /* driver not started */
#define SCE_ERROR_USBCAM_BUSY            0x80243909 /* request already in progress */
#define SCE_ERROR_USBCAM_BUF_SMALL       0x8024390A /* user buffer smaller than available data */
#define SCE_ERROR_USBCAM_INVALID_FREQ    0x8024390B /* mic sample rate not supported */
#define SCE_ERROR_USBCAM_INVALID_STATE   0x8024390C /* wrong capture mode/state */
#define SCE_ERROR_USBCAM_UNKNOWN_CMD     0x8024390D /* unknown ioctl/command */
#define SCE_ERROR_USBCAM_NOT_READY       0x8024390E /* poll with nothing pending */
#define SCE_ERROR_USBCAM_NO_CALLBACK     0x8024390F /* no lens callback registered */
#define SCE_ERROR_USBCAM_ALREADY         0x80243910 /* callback already registered */
#define SCE_ERROR_USBCAM_MIC_NOT_SETUP   0x80243911 /* mic workarea not set */
#define SCE_ERROR_USBCAM_MIC_STATE       0x80243912 /* mic wrong mode */
#define SCE_ERROR_USBCAM_REPLY_MISMATCH  0x80243913 /* last control reply tag mismatched (inferred) */

/* Interface descriptor entry: descriptor plus endpoint/class-specific
   descriptor pointers and class-specific total length. */
struct UsbIfDescEntry {
    struct InterfaceDescriptor desc;
    void *epDesc;
    void *csDesc;
    u32 csLen;
};

/* Configuration descriptor entry: descriptor plus interface list. */
struct UsbCfgDescEntry {
    u8 desc[9];
    void *intfList;
    void *unkC;
    u32 unk10;
};

/* Isochronous endpoint entry used by the microphone streaming interface. */
struct UsbMicEpEntry {
    u8 desc[8];
    void *csEpDesc;
    u32 unk8;
    u8 pad[16];
};

/* Object pointed to by ::UsbDriver.intp. */
struct UsbIntpEntry {
    s32 first;
    s32 unk4;
    u32 num;
};

/* Object pointed to by ::UsbDriver.confp (full-speed configuration). */
struct UsbConfBundle {
    void *cfgDesc;
    void *intfList;
    void *ifDesc;
    void *epDesc;
};

/* Device state structs are defined in full next to the .bss instances below. */

/* Callbacks referenced by the driver structures below. */
int videoBusEvent(int arg1 __attribute__((unused)), int arg2 __attribute__((unused)), int arg3 __attribute__((unused)));
int micBusEvent(int arg1 __attribute__((unused)), int arg2 __attribute__((unused)), int arg3 __attribute__((unused)));
int videoDetach(int arg1 __attribute__((unused)), int arg2 __attribute__((unused)), int arg3 __attribute__((unused)));
int micDetach(int arg1 __attribute__((unused)), int arg2 __attribute__((unused)), int arg3 __attribute__((unused)));
int videoRecvCtl(int arg1 __attribute__((unused)), int arg2 __attribute__((unused)),
                 struct DeviceRequest *req __attribute__((unused)));
int videoDriverStart(int size __attribute__((unused)), void *args __attribute__((unused)));
int micDriverStart(int size __attribute__((unused)), void *args __attribute__((unused)));
int videoDriverStop(int size __attribute__((unused)), void *args __attribute__((unused)));
int micDriverStop(int size __attribute__((unused)), void *args __attribute__((unused)));
int videoAttach(int speed __attribute__((unused)), void *arg2 __attribute__((unused)),
                 void *arg3 __attribute__((unused)));
int micAttach(int speed __attribute__((unused)), void *arg2 __attribute__((unused)),
                 void *arg3 __attribute__((unused)));
int videoNullCallback(int arg1 __attribute__((unused)), int arg2 __attribute__((unused)),
                      int arg3 __attribute__((unused)));
int micAccumCallback(int arg1 __attribute__((unused)), int arg2 __attribute__((unused)),
                     int arg3 __attribute__((unused)));

s32 registerMicDriver(int arg0 __attribute__((unused)), int arg1 __attribute__((unused)));

/* sceUsbAcc_internal imports (provided by usbacc's exports). */
s32 sceUsbAccGetInfo(u64 *arg);
s32 sceUsbAccRegisterType(u16 type);
s32 sceUsbAccUnregisterType(u16 type);

/* Not declared in uofw headers. */
int sceUsbbdReqRecv(struct UsbdDeviceReq *req);
int sceKernelCancelSema(SceUID semaid, int signal, int *pcount);

/* Thread entries and request-completion callbacks. */
/* PspUsbCamSetupVideoParam as the kernel reads it: the SDK header stops
   at 48 bytes, but the driver range-checks param..param+52 with
   pspK1StaBufOk and reads a 13th word at +48 (size == 52 variant). */
struct UsbCamSetupVideoParam {
    int size;
    int resolution;
    int framerate;
    int wb;
    int saturation;
    int brightness;
    int contrast;
    int sharpness;
    int effectmode;
    int framesize;
    u32 unk;
    int evlevel;
    u32 unk2;
};

_Static_assert(sizeof(struct UsbCamSetupVideoParam) == 52, "UsbCamSetupVideoParam size");

/* 100-byte block sceUsbCamSetupVideo builds on the stack and passes to
   packVideoConfig; field-for-field PspUsbCamSetupVideoExParam. Field +0 is
   never written here. */
struct UsbCamVideoReq {
    int size;
    u32 unk;
    int resolution;
    int framerate;
    u32 unk2;
    u32 unk3;
    int wb;
    int saturation;
    int brightness;
    int contrast;
    int sharpness;
    u32 unk4;
    u32 unk5;
    u32 unk6[3];
    int effectmode;
    u32 unk7;
    u32 unk8;
    u32 unk9;
    u32 unk10;
    u32 unk11;
    int framesize;
    u32 unk12;
    int evlevel;
};

_Static_assert(sizeof(struct UsbCamVideoReq) == 100, "UsbCamVideoReq size");

/* sceUsbAcc_internal import (NID 0x79A1C743); present in the original's
   import table. uOFW's usbacc.c declares it too, but no uofw header
   does, so it is declared locally here. */
s32 sceUsbAccGetAuthStat(void);

/* packVideoConfig fills out's 32 bytes from req (disassembly 0x4298). */
int packVideoConfig(u8 *out, struct UsbCamVideoReq *req);

/* rodata maps (original module addresses in the comments). */
/* 0x8CB8: bucket map read through a stack copy by encodeSharpness. */
static const u8 s_map8CB8[4] = { 0, 1, 2, 3 };
/* 0x8DE0: width index map, encodeWidthCode and the sceUsbCamSetupVideo tail. */
static const u8 s_widthIdxMap[10] = { 6, 5, 4, 3, 8, 7, 2, 1, 9, 0 };
/* 0x8DEC: separate rodata object with identical contents (original has it
   twice); kept as a second object for byte-exactness. */
static const u8 s_heightIdxMap[10] = { 6, 5, 4, 3, 8, 7, 2, 1, 9, 0 };
/* 0x8DF8: identity {0..7} searched by the sceUsbCamSetupVideo tail. */
static const u8 s_map8DF8[8] = { 0, 1, 2, 3, 4, 5, 6, 7 };
/* 0x8E0C: {0,1,2} searched against g_videoState[0x16]. */
static const u8 s_map8E0C[3] = { 0, 1, 2 };
/* 0x8E28: identity {0..0x10} (clampEvLevel). */
static const u8 s_map8E28[17] = {
    0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16
};
/* 0x8E3C: PspUsbCamResolution -> PspUsbCamResolutionEx, byte pairs,
   only the odd byte of each pair is read (lb at +2*i+1). */
static const s8 s_map8E3C[20] = {
    0, 0, 1, 1, 2, 2, 3, 3, 6, 6, 9, 7, 9, 8, 6, 5, 6, 4, 0, 0
};
/* 0x8E78: 10x10 signed level table for checkEvAllowed (-1 = not usable). */
static const s8 s_map8E78[100] = {
    10, -1, -1, -1, -1, -1, -1, -1, -1, -1,
    11, 10, -1, -1, -1, -1, -1, -1, -1, -1,
    20, 17, 10, -1, -1, -1, -1, -1, -1, -1,
    22, 20, 11, 10, -1, -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
    40, 33, 20, 17, 18, 14, 10, -1, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
    80, 66, 40, 33, 36, 28, 20, 13, 10, 10
};

/* NID 0xEDA8A020, sceUsbBus_driver import; psplibdoc matches the NID to
   sceUsbRestart, but no uofw header declares it, so it is declared
   locally here. */
int sceUsbRestart(int arg);

/* Also declared in include/interruptman.h:150-151; repeated here
   because this file does not include that header. */
s32 sceKernelCpuSuspendIntr(void);
void sceKernelCpuResumeIntr(s32 intr);

/* Forward declarations for functions defined further below. */
s32 resetVideoDefaults(void);
s32 startVideoStream(void);
s32 stopVideoStream(void);
s32 armStillRead(void *buf, int size);
s32 videoWorkerThread(SceSize args, void *argp);
s32 videoCopyWorker(SceSize args, void *argp);
s32 micCopyWorker(SceSize args, void *argp);
void videoEp0Complete(struct UsbdDeviceReq *req);
void videoBulkComplete(struct UsbdDeviceReq *req);
s32 startIsoReceives(void);

/* stopAndDrainStream and pumpVideoFrames are defined below; declared
   here for the caller above. */
s32 stopAndDrainStream(int arg);
s32 pumpVideoFrames(int arg);

/* dmacCopy is defined below; declared here for the first caller. The
   disassembly passes destination and length (two arguments) at 0x3E38. */
void *dmacCopy(void *dst, const void *src, int size);
s32 drainMicBuffer(void *dst, int size);

/* sceUsbAcc_internal import (NID 0x2A100C1F); no uofw header declares it,
   so it is declared locally here. Present in the original's import table. */
s32 sceUsbAccIntrInReq(struct UsbdDeviceReq *req);

/* The value below lives in the sceUsb range (cf. usbacc.c defines): a previous
   accessory interrupt request completed with retcode > 0. Exact SCE name
   unknown, value verified against the disassembly. */
#define SCE_ERROR_USB_INTR_FAILED         0x80243006

/* Forward declarations for functions defined further below. */
/* ============================================================
 * Section: microphone setup and streaming
 * (forward declarations; definitions follow the video/still code)
 * ============================================================ */

s32 commitMicSetup(void *cmd, int flag, void *workarea, int wasize);
s32 sendMicStart(void);
s32 startMicSync(void);
s32 validateMicRead(void *buf, int size);
/* conditionalSwapCopy is defined below its only caller; declared here. */
void *conditionalSwapCopy(void *dst, void *src, int size);

s32 sendMicSetup(void);


/* Mic setup param layouts; the SDK spells them PspUsbCamSetupMicParam /
   PspUsbCamSetupMicExParam. The kernel reads param..param+20 /
   param..param+36. Field +0 is never read here. */
struct UsbCamSetupMicParam {
    int size;
    int alc;
    int gain;
    int noize;
    int freq;
};

_Static_assert(sizeof(struct UsbCamSetupMicParam) == 20, "UsbCamSetupMicParam size");

struct UsbCamSetupMicExParam {
    int size;
    int alc;
    int gain;
    u32 unk2[4];
    int freq;
    int unk3;
};

_Static_assert(sizeof(struct UsbCamSetupMicExParam) == 36, "UsbCamSetupMicExParam size");

s32 resetVideoDefaults(void);
s32 startIsoReceives(void);
s32 videoWorkerThread(SceSize args, void *argp);
s32 videoCopyWorker(SceSize args, void *argp);
s32 micCopyWorker(SceSize args, void *argp);
void videoCmdComplete(struct UsbdDeviceReq *req);
void micEmptyComplete(struct UsbdDeviceReq *req);
void videoIsoComplete(struct UsbdDeviceReq *req);
void videoEp0Complete(struct UsbdDeviceReq *req);
void videoBulkComplete(struct UsbdDeviceReq *req);
void micRecvComplete(struct UsbdDeviceReq *req);
s32 unregisterMicDriver(int arg0 __attribute__((unused)), int arg1 __attribute__((unused)));

/*
 * Video device (USBCamDriver).
 */

/* USB device descriptor (18 bytes) + 2 padding bytes. */
u8 g_videoDevDesc[20] = {
    0x12, 0x01, 0x00, 0x02, 0x00, 0x00, 0x00, 0x40,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00,
    0x00, 0x01, 0x00, 0x00
};

/* Bulk OUT endpoint descriptor (7 bytes) + padding. */
u8 g_videoEpOutDesc[16] = {
    0x07, 0x05, 0x01, 0x02, 0x40, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
};

/* Isochronous endpoint descriptor (7 bytes) + padding. Address byte is
   0x02 exactly as in the original (direction bit clear); kept byte-exact. */
u8 g_videoEpIsoDesc[32] = {
    0x07, 0x05, 0x02, 0x05, 0x80, 0x03, 0x01, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
};

/* Interface 0, alternate setting 0. */
struct UsbIfDescEntry g_videoIfAlt0 = {
    { 0x09, 0x04, 0x00, 0x00, 0x01, 0xFF, 0x00, 0x00, 0x00 },
    g_videoEpOutDesc,
    NULL,
    0
};

/* Interface 0, alternate setting 1 (video streaming). */
struct UsbIfDescEntry g_videoIfAlt1 = {
    { 0x09, 0x04, 0x00, 0x01, 0x01, 0xFF, 0x00, 0x00, 0x00 },
    g_videoEpIsoDesc,
    NULL,
    0
};

/* Padding between the interface entries and the interface list. */
u8 g_videoIfPad[24] __attribute__((section(".data"))) = { 0 };

struct UsbInterfaces g_videoIntfList = {
    { &g_videoIfAlt0.desc, NULL },
    2
};

/* Configuration descriptor (9 bytes) + 3 padding bytes. */
struct UsbCfgDescEntry g_videoCfgDesc = {
    { 0x09, 0x02, 0x29, 0x00, 0x01, 0x01, 0x00, 0xC0, 0x00 },
    &g_videoIntfList,
    NULL,
    0
};

/* String descriptor "USB CAMERA" (bLength 0x16). */
struct StringDescriptor g_videoStrDesc = {
    0x16, 0x03, { 'U', 'S', 'B', ' ', 'C', 'A', 'M', 'E', 'R', 'A' }
};

/* Padding after the string descriptor; aligned(1) keeps it packed at 0xF2 like the original. */
u8 g_videoStrPad[62] __attribute__((section(".data"), aligned(1))) = { 0 };

struct UsbEndpoint g_videoEndpoints[3] = {
    { 0, 0, 0 },
    { 1, 0, 0 },
    { 2, 0, 0 }
};

struct UsbIntpEntry g_videoIntp = { 3, 0, 1 };

struct UsbConfBundle g_videoConfBundle = {
    &g_videoCfgDesc,
    &g_videoIntfList,
    &g_videoIfAlt0,
    g_videoEpOutDesc
};

struct UsbDriver g_videoDriver = {
    .name = "USBCamDriver",
    .endpoints = 3,
    .endp = g_videoEndpoints,
    .intp = (struct UsbInterface *)&g_videoIntp,
    .devp_hi = NULL,
    .confp_hi = NULL,
    .devp = g_videoDevDesc,
    .confp = &g_videoConfBundle,
    .str = &g_videoStrDesc,
    .recvctl = videoRecvCtl,
    .func28 = videoBusEvent,
    .attach = videoAttach,
    .detach = videoDetach,
    .unk34 = (s32)(long)videoNullCallback,
    .start_func = videoDriverStart,
    .stop_func = videoDriverStop,
    .link = NULL
};

/*
 * Microphone device (USBCamMicDriver).
 */

/* USB device descriptor (18 bytes) + 2 padding bytes. */
u8 g_micDevDesc[20] = {
    0x12, 0x01, 0x00, 0x02, 0x00, 0x00, 0x00, 0x40,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00,
    0x00, 0x01, 0x00, 0x00
};

/* Class-specific endpoint descriptor (7 bytes) with 2+3 padding bytes. */
u8 g_micCsEpBlob[12] = {
    0x00, 0x00, 0x07, 0x25, 0x01, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00
};

/* Isochronous IN endpoint entry. */
struct UsbMicEpEntry g_micEpEntry = {
    { 0x09, 0x05, 0x01, 0x05, 0x80, 0x00, 0x01, 0x00 },
    g_micCsEpBlob,
    9,
    { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 }
};

/* Class-specific AC interface descriptors (AudioControl header, terminals, feature unit). */
u8 g_micCsDescs1[32] = {
    0x09, 0x24, 0x01, 0x00, 0x01, 0x1E, 0x00, 0x01,
    0x01, 0x0C, 0x24, 0x02, 0x01, 0x01, 0x01, 0x00,
    0x01, 0x00, 0x00, 0x00, 0x00, 0x09, 0x24, 0x03,
    0x02, 0x01, 0x03, 0x00, 0x01, 0x00, 0x00, 0x00
};

/* Class-specific AS interface descriptors. */
u8 g_micCsDescs2[20] = {
    0x07, 0x24, 0x01, 0x01, 0x01, 0x01, 0x00, 0x0B,
    0x24, 0x02, 0x01, 0x01, 0x02, 0x10, 0x01, 0x44,
    0xAC, 0x00, 0x00, 0x00
};

/* Interface 0, alternate setting 0 (control). bInterfaceNumber is 0x00
   in the original data; the old comment saying "Interface 1" was wrong. */
struct UsbIfDescEntry g_micIfCtrl = {
    { 0x09, 0x04, 0x00, 0x00, 0x00, 0x01, 0x01, 0x00, 0x01 },
    NULL,
    g_micCsDescs1,
    0x1E
};

/* Alternate setting 0 of the streaming interface (bInterfaceNumber 0x01). */
struct UsbIfDescEntry g_micIfAlt1 = {
    { 0x09, 0x04, 0x01, 0x00, 0x00, 0x01, 0x02, 0x00, 0x01 },
    NULL,
    NULL,
    0
};

/* Alternate setting 1 of the streaming interface (isochronous). */
struct UsbIfDescEntry g_micIfAlt2 = {
    { 0x09, 0x04, 0x01, 0x01, 0x01, 0x01, 0x02, 0x00, 0x01 },
    &g_micEpEntry,
    g_micCsDescs2,
    0x12
};

/* Padding between the interface entries and the interface list. */
u8 g_micIfPad[24] __attribute__((section(".data"))) = { 0 };

struct UsbInterfaces g_micIntfList = {
    { &g_micIfCtrl.desc, NULL },
    1
};

struct UsbInterfaces g_micIntfListAlt = {
    { &g_micIfAlt1.desc, NULL },
    2
};

/* Configuration descriptor (9 bytes) + 3 padding bytes. */
struct UsbCfgDescEntry g_micCfgDesc = {
    { 0x09, 0x02, 0x64, 0x00, 0x02, 0x01, 0x00, 0xC0, 0x00 },
    &g_micIntfList,
    NULL,
    0
};

/* String descriptor "USB Accessory Mic" (bLength 0x24). */
struct StringDescriptor g_micStrDesc = {
    0x24, 0x03, { 'U', 'S', 'B', ' ', 'A', 'c', 'c', 'e', 's', 's', 'o', 'r', 'y', ' ', 'M', 'i', 'c' }
};

/* Padding after the string descriptor; aligned(1) keeps it packed at 0x2FA like the original. */
u8 g_micStrPad[62] __attribute__((section(".data"), aligned(1))) = { 0 };

struct UsbEndpoint g_micEndpoints[2] = {
    { 0, 0, 0 },
    { 1, 0, 0 }
};

struct UsbIntpEntry g_micIntp = { -1, 0, 2 };

struct UsbConfBundle g_micConfBundle = {
    &g_micCfgDesc,
    &g_micIntfList,
    &g_micIfCtrl,
    &g_micEpEntry
};

struct UsbDriver g_micDriver = {
    .name = "USBCamMicDriver",
    .endpoints = 2,
    .endp = g_micEndpoints,
    .intp = (struct UsbInterface *)&g_micIntp,
    .devp_hi = NULL,
    .confp_hi = NULL,
    .devp = g_micDevDesc,
    .confp = &g_micConfBundle,
    .str = &g_micStrDesc,
    .recvctl = NULL,
    .func28 = micBusEvent,
    .attach = micAttach,
    .detach = micDetach,
    .unk34 = (s32)(long)micAccumCallback,
    .start_func = micDriverStart,
    .stop_func = micDriverStop,
    .link = NULL
};

/* ============================================================
 * Descriptors and driver registration data (byte-exact blobs) are above;
 * device state follows.
 * ============================================================ */

/*
 * Device states (.bss, laid out as in the original: microphone state at
 * 0x93D0, video state at 0x94FC). Single struct per device; all former
 * offset macros (VIDEO_x / MIC_x) are named fields below (offsets in comments).
 */

/* 32-byte packed video config written by packVideoConfig at +0x0C. */
struct VideoCfg {
    u8 width;       /* +0x0C */
    u8 height;      /* +0x0D */
    u8 framerate;   /* +0x0E */
    u8 unk0F;       /* +0x0F */
    u8 unk10;       /* +0x10 */
    u8 wb;          /* +0x11 */
    u8 unk12;       /* +0x12 */
    u8 unk13;       /* +0x13 */
    u8 unk14;       /* +0x14 */
    u8 unk15;       /* +0x15 */
    u8 antiflicker; /* +0x16 */
    u8 unk17;       /* +0x17 */
    u16 unk18;      /* +0x18 */
    u16 unk1A;      /* +0x1A */
    u16 unk1C;      /* +0x1C */
    u8 effect;      /* +0x1E */
    u8 unk1F;       /* +0x1F */
    u8 res;         /* +0x20 */
    u8 unk21;       /* +0x21 */
    u16 unk22;      /* +0x22 */
    u16 unk24;      /* +0x24 */
    u8 unk26[5];    /* +0x26..0x2A (unobserved) */
    u8 ev;          /* +0x2B */
};

_Static_assert(sizeof(struct VideoCfg) == 32, "VideoCfg size");
_Static_assert(__builtin_offsetof(struct VideoCfg, width) == 0x00, "VideoCfg.width");
_Static_assert(__builtin_offsetof(struct VideoCfg, height) == 0x01, "VideoCfg.height");
_Static_assert(__builtin_offsetof(struct VideoCfg, framerate) == 0x02, "VideoCfg.framerate");
_Static_assert(__builtin_offsetof(struct VideoCfg, unk0F) == 0x03, "VideoCfg.unk0F");
_Static_assert(__builtin_offsetof(struct VideoCfg, unk10) == 0x04, "VideoCfg.unk10");
_Static_assert(__builtin_offsetof(struct VideoCfg, wb) == 0x05, "VideoCfg.wb");
_Static_assert(__builtin_offsetof(struct VideoCfg, unk12) == 0x06, "VideoCfg.unk12");
_Static_assert(__builtin_offsetof(struct VideoCfg, unk13) == 0x07, "VideoCfg.unk13");
_Static_assert(__builtin_offsetof(struct VideoCfg, unk14) == 0x08, "VideoCfg.unk14");
_Static_assert(__builtin_offsetof(struct VideoCfg, unk15) == 0x09, "VideoCfg.unk15");
_Static_assert(__builtin_offsetof(struct VideoCfg, antiflicker) == 0x0A, "VideoCfg.antiflicker");
_Static_assert(__builtin_offsetof(struct VideoCfg, unk17) == 0x0B, "VideoCfg.unk17");
_Static_assert(__builtin_offsetof(struct VideoCfg, unk18) == 0x0C, "VideoCfg.unk18");
_Static_assert(__builtin_offsetof(struct VideoCfg, unk1A) == 0x0E, "VideoCfg.unk1A");
_Static_assert(__builtin_offsetof(struct VideoCfg, unk1C) == 0x10, "VideoCfg.unk1C");
_Static_assert(__builtin_offsetof(struct VideoCfg, effect) == 0x12, "VideoCfg.effect");
_Static_assert(__builtin_offsetof(struct VideoCfg, unk1F) == 0x13, "VideoCfg.unk1F");
_Static_assert(__builtin_offsetof(struct VideoCfg, res) == 0x14, "VideoCfg.res");
_Static_assert(__builtin_offsetof(struct VideoCfg, unk21) == 0x15, "VideoCfg.unk21");
_Static_assert(__builtin_offsetof(struct VideoCfg, unk22) == 0x16, "VideoCfg.unk22");
_Static_assert(__builtin_offsetof(struct VideoCfg, unk24) == 0x18, "VideoCfg.unk24");
_Static_assert(__builtin_offsetof(struct VideoCfg, unk26) == 0x1A, "VideoCfg.unk26");
_Static_assert(__builtin_offsetof(struct VideoCfg, ev) == 0x1F, "VideoCfg.ev");

struct VideoDescState {
    void *buf;   /* +0x00 */
    int len;     /* +0x04 */
    int cap;     /* +0x08 */
    void *next;  /* +0x0C */
};

/* Video device state (0x1B8 bytes). */
struct VideoState {
    u8 started;         /* +0x00: driver started */
    u8 setupFlags;      /* +0x01: bit0 video setup, bit1 stream setup */
    u8 attached;        /* +0x02: accessory configured */
    u8 altSetting;      /* +0x03: current alt setting */
    u8 mode;            /* +0x04: capture mode */
    u8 aux;             /* +0x05 */
    u8 activeSlot;      /* +0x06: half-buffer being filled */
    u8 peerSlot;        /* +0x07 */
    u32 flags;          /* +0x08 */
    struct VideoCfg cfg;/* +0x0C */
    union {             /* +0x2C..0x43 */
        u32 w[6];
        u8 b[24];
    } cfgTail;          /* [0] observed as frame clamp */
    struct UsbdDeviceReq intrReq;   /* +0x44 (was reqB): accessory intr */
    void *cmdBuf;                   /* +0x6C */
    struct DeviceRequest setup;     /* +0x70 */
    struct UsbdDeviceReq ep0Req;    /* +0x78 (was reqA) */
    void *ep0Buf;                   /* +0xA0 */
    struct UsbdDeviceReq isoReq[2]; /* +0xA4 (was items) */
    void *frameBuf[2];              /* +0xF4 (was frameBufs) */
    struct UsbdDeviceReq bulkReq;   /* +0xFC (was reqC) */
    void *bulkBuf;                  /* +0x124 */
    void *xferHead;                 /* +0x128 (was pad128[0]) */
    void *xferTail;                 /* +0x12C (was pad128[4]) */
    struct VideoDescState desc[2];  /* +0x130 */
    u32 workBase;       /* +0x150: video workarea base */
    u32 workHalf;       /* +0x154 */
    struct {              /* +0x158/+0x160: half-buffer {base,fill} pairs */
        u32 buf;            /* +0x158/+0x160 */
        u32 len;            /* +0x15C/+0x164 */
    } slot[2];
    u32 frameSeq;       /* +0x168 */
    u32 fragHdr;        /* +0x16C (was unk16C) */
    u32 fragTotal;      /* +0x170 (was unk170) */
    u32 stillSize;      /* +0x174 */
    u32 stillAvail;     /* +0x178 */
    u32 stillBuf;       /* +0x17C */
    u32 xferResult;    /* +0x180: completion result of last frame/still transfer */
    u32 xferLength;    /* +0x184: clamped byte count of last transfer */
    sceKernelDmaOperation *dmaOp; /* +0x188 (was unk188) */
    s32 fplId;          /* +0x18C */
    s32 eventflag;      /* +0x190 */
    s32 sema;           /* +0x194 */
    s32 mutex;          /* +0x198 */
    s32 threadVideo;    /* +0x19C (was thread1) */
    s32 threadCopy;     /* +0x1A0 (was thread2) */
    s32 lensCbid;       /* +0x1A4 (was unk1A4) */
    u32 replyMismatch;  /* +0x1A8: last EP0 control-reply tag check nonzero = mismatch */
    u32 readSize;       /* +0x1AC */
    u32 readBuf;        /* +0x1B0 */
    u32 resEx;          /* +0x1B4: Ext resolution / 1280 threshold */
};

_Static_assert(sizeof(struct VideoState) == 0x1B8, "VideoState size");
_Static_assert(__builtin_offsetof(struct VideoState, started) == 0x0, "VideoState.started");
_Static_assert(__builtin_offsetof(struct VideoState, setupFlags) == 0x1, "VideoState.setupFlags");
_Static_assert(__builtin_offsetof(struct VideoState, attached) == 0x2, "VideoState.attached");
_Static_assert(__builtin_offsetof(struct VideoState, altSetting) == 0x3, "VideoState.altSetting");
_Static_assert(__builtin_offsetof(struct VideoState, mode) == 0x4, "VideoState.mode");
_Static_assert(__builtin_offsetof(struct VideoState, aux) == 0x5, "VideoState.aux");
_Static_assert(__builtin_offsetof(struct VideoState, activeSlot) == 0x6, "VideoState.activeSlot");
_Static_assert(__builtin_offsetof(struct VideoState, peerSlot) == 0x7, "VideoState.peerSlot");
_Static_assert(__builtin_offsetof(struct VideoState, flags) == 0x8, "VideoState.flags");
_Static_assert(__builtin_offsetof(struct VideoState, cfg) == 0xC, "VideoState.cfg");
_Static_assert(__builtin_offsetof(struct VideoState, cfgTail) == 0x2C, "VideoState.cfgTail");
_Static_assert(__builtin_offsetof(struct VideoState, intrReq) == 0x44, "VideoState.intrReq");
_Static_assert(__builtin_offsetof(struct VideoState, cmdBuf) == 0x6C, "VideoState.cmdBuf");
_Static_assert(__builtin_offsetof(struct VideoState, setup) == 0x70, "VideoState.setup");
_Static_assert(__builtin_offsetof(struct VideoState, ep0Req) == 0x78, "VideoState.ep0Req");
_Static_assert(__builtin_offsetof(struct VideoState, ep0Buf) == 0xA0, "VideoState.ep0Buf");
_Static_assert(__builtin_offsetof(struct VideoState, isoReq) == 0xA4, "VideoState.isoReq");
_Static_assert(__builtin_offsetof(struct VideoState, frameBuf) == 0xF4, "VideoState.frameBuf");
_Static_assert(__builtin_offsetof(struct VideoState, bulkReq) == 0xFC, "VideoState.bulkReq");
_Static_assert(__builtin_offsetof(struct VideoState, bulkBuf) == 0x124, "VideoState.bulkBuf");
_Static_assert(__builtin_offsetof(struct VideoState, xferHead) == 0x128, "VideoState.xferHead");
_Static_assert(__builtin_offsetof(struct VideoState, xferTail) == 0x12C, "VideoState.xferTail");
_Static_assert(__builtin_offsetof(struct VideoState, desc) == 0x130, "VideoState.desc");
_Static_assert(__builtin_offsetof(struct VideoState, workBase) == 0x150, "VideoState.workBase");
_Static_assert(__builtin_offsetof(struct VideoState, workHalf) == 0x154, "VideoState.workHalf");
_Static_assert(__builtin_offsetof(struct VideoState, slot) == 0x158, "VideoState.slot");
_Static_assert(__builtin_offsetof(struct VideoState, frameSeq) == 0x168, "VideoState.frameSeq");
_Static_assert(__builtin_offsetof(struct VideoState, fragHdr) == 0x16C, "VideoState.fragHdr");
_Static_assert(__builtin_offsetof(struct VideoState, fragTotal) == 0x170, "VideoState.fragTotal");
_Static_assert(__builtin_offsetof(struct VideoState, stillSize) == 0x174, "VideoState.stillSize");
_Static_assert(__builtin_offsetof(struct VideoState, stillAvail) == 0x178, "VideoState.stillAvail");
_Static_assert(__builtin_offsetof(struct VideoState, stillBuf) == 0x17C, "VideoState.stillBuf");
_Static_assert(__builtin_offsetof(struct VideoState, xferResult) == 0x180, "VideoState.xferResult");
_Static_assert(__builtin_offsetof(struct VideoState, xferLength) == 0x184, "VideoState.xferLength");
_Static_assert(__builtin_offsetof(struct VideoState, dmaOp) == 0x188, "VideoState.dmaOp");
_Static_assert(__builtin_offsetof(struct VideoState, fplId) == 0x18C, "VideoState.fplId");
_Static_assert(__builtin_offsetof(struct VideoState, eventflag) == 0x190, "VideoState.eventflag");
_Static_assert(__builtin_offsetof(struct VideoState, sema) == 0x194, "VideoState.sema");
_Static_assert(__builtin_offsetof(struct VideoState, mutex) == 0x198, "VideoState.mutex");
_Static_assert(__builtin_offsetof(struct VideoState, threadVideo) == 0x19C, "VideoState.threadVideo");
_Static_assert(__builtin_offsetof(struct VideoState, threadCopy) == 0x1A0, "VideoState.threadCopy");
_Static_assert(__builtin_offsetof(struct VideoState, lensCbid) == 0x1A4, "VideoState.lensCbid");
_Static_assert(__builtin_offsetof(struct VideoState, replyMismatch) == 0x1A8, "VideoState.replyMismatch");
_Static_assert(__builtin_offsetof(struct VideoState, readSize) == 0x1AC, "VideoState.readSize");
_Static_assert(__builtin_offsetof(struct VideoState, readBuf) == 0x1B0, "VideoState.readBuf");
_Static_assert(__builtin_offsetof(struct VideoState, resEx) == 0x1B4, "VideoState.resEx");

/* Microphone device state (0x12C bytes). +0x08..0x15 is the 14-byte
   setup block (gain at +0x0A); +0x18..0x34 is the ring-buffer state. */
struct MicState {
    u8 started;     /* +0x00 */
    u8 setupFlags;  /* +0x01 */
    u8 attached;    /* +0x02 */
    u8 altSetting;  /* +0x03 */
    u8 mode;        /* +0x04 */
    u8 aux;         /* +0x05 */
    u8 rateFlag;    /* +0x06: nonzero = 132-byte units */
    u8 pad07;
    u16 cmdLo;      /* +0x08 */
    u16 gain;       /* +0x0A */
    u32 cmd0C;      /* +0x0C */
    u32 cmd10;      /* +0x10 */
    u16 cmd14;      /* +0x14 */
    u8 pad16b[2];   /* +0x16 */
    u32 setupOk;    /* +0x18: mic workarea installed */
    u32 bufSize;    /* +0x1C */
    u32 ringCapacity; /* +0x20: slots (132-byte mode) or bytes */
    u32 writePos;   /* +0x24 */
    u32 readPos;    /* +0x28 */
    u32 writeCursor; /* +0x2C: slot index (132-byte mode) or write pointer */
    u32 bufBase;     /* +0x30: ring buffer base */
    u32 status;     /* +0x34 */
    struct UsbdDeviceReq intrReq;   /* +0x38 (was reqD) */
    void *ep0Buf;                   /* +0x60 */
    struct UsbdDeviceReq isoReq[4]; /* +0x64 (was reqs) */
    void *pcmBuf[4];                /* +0x104 (was bufs) */
    s32 eventflag;  /* +0x114 */
    s32 fplId;      /* +0x118 */
    s32 thread;     /* +0x11C */
    u32 readSize;   /* +0x120 */
    u32 readBuf;    /* +0x124 */
    u32 swapMode;   /* +0x128 (was unk128): nonzero = 16-bit-swap copies */
};

_Static_assert(sizeof(struct MicState) == 0x12C, "MicState size");
_Static_assert(__builtin_offsetof(struct MicState, started) == 0x0, "MicState.started");
_Static_assert(__builtin_offsetof(struct MicState, setupFlags) == 0x1, "MicState.setupFlags");
_Static_assert(__builtin_offsetof(struct MicState, attached) == 0x2, "MicState.attached");
_Static_assert(__builtin_offsetof(struct MicState, altSetting) == 0x3, "MicState.altSetting");
_Static_assert(__builtin_offsetof(struct MicState, mode) == 0x4, "MicState.mode");
_Static_assert(__builtin_offsetof(struct MicState, aux) == 0x5, "MicState.aux");
_Static_assert(__builtin_offsetof(struct MicState, rateFlag) == 0x6, "MicState.rateFlag");
_Static_assert(__builtin_offsetof(struct MicState, cmdLo) == 0x8, "MicState.cmdLo");
_Static_assert(__builtin_offsetof(struct MicState, gain) == 0xA, "MicState.gain");
_Static_assert(__builtin_offsetof(struct MicState, cmd0C) == 0xC, "MicState.cmd0C");
_Static_assert(__builtin_offsetof(struct MicState, cmd10) == 0x10, "MicState.cmd10");
_Static_assert(__builtin_offsetof(struct MicState, cmd14) == 0x14, "MicState.cmd14");
_Static_assert(__builtin_offsetof(struct MicState, setupOk) == 0x18, "MicState.setupOk");
_Static_assert(__builtin_offsetof(struct MicState, bufSize) == 0x1C, "MicState.bufSize");
_Static_assert(__builtin_offsetof(struct MicState, ringCapacity) == 0x20, "MicState.ringCapacity");
_Static_assert(__builtin_offsetof(struct MicState, writePos) == 0x24, "MicState.writePos");
_Static_assert(__builtin_offsetof(struct MicState, readPos) == 0x28, "MicState.readPos");
_Static_assert(__builtin_offsetof(struct MicState, writeCursor) == 0x2C, "MicState.writeCursor");
_Static_assert(__builtin_offsetof(struct MicState, bufBase) == 0x30, "MicState.bufBase");
_Static_assert(__builtin_offsetof(struct MicState, status) == 0x34, "MicState.status");
_Static_assert(__builtin_offsetof(struct MicState, intrReq) == 0x38, "MicState.intrReq");
_Static_assert(__builtin_offsetof(struct MicState, ep0Buf) == 0x60, "MicState.ep0Buf");
_Static_assert(__builtin_offsetof(struct MicState, isoReq) == 0x64, "MicState.isoReq");
_Static_assert(__builtin_offsetof(struct MicState, pcmBuf) == 0x104, "MicState.pcmBuf");
_Static_assert(__builtin_offsetof(struct MicState, eventflag) == 0x114, "MicState.eventflag");
_Static_assert(__builtin_offsetof(struct MicState, fplId) == 0x118, "MicState.fplId");
_Static_assert(__builtin_offsetof(struct MicState, thread) == 0x11C, "MicState.thread");
_Static_assert(__builtin_offsetof(struct MicState, readSize) == 0x120, "MicState.readSize");
_Static_assert(__builtin_offsetof(struct MicState, readBuf) == 0x124, "MicState.readBuf");
_Static_assert(__builtin_offsetof(struct MicState, swapMode) == 0x128, "MicState.swapMode");

struct MicState g_micState = { 0 };
struct VideoState g_videoState = { 0 };


/* Byte-offset accessor for the computed-offset merger idioms (the slot,
   slotLen and poff local variables hold struct byte offsets, and poff
   selects the active slot[i] pair). All fixed offsets use named fields. */
static inline u32 *videoWordAt(u32 off)
{
    return (u32 *)((u8 *)&g_videoState + off);
}

/* (bmRequestType, bRequest) pairs, table at 0x8EDC. */
static const u8 g_ctlRequests[6][8] = {
    { 0xC1, 0x03 },
    { 0xC1, 0x06 },
    { 0xC1, 0x08 },
    { 0x41, 0x07 },
    { 0x41, 0x09 },
    { 0x41, 0x0A }
};

/* ============================================================
 * Section: video accessory commands and attach/detach
 * ============================================================ */

/* 0x00000000 sendAccCommand */
int sendAccCommand(int cmd, int arg1, void *buf, int len)
{
    struct VideoState *st = (struct VideoState *)&g_videoState;
    u8 *cb;
    s32 res;

    if (st->attached == 0)
        return SCE_ERROR_USBCAM_NOT_ATTACHED;
    res = sceKernelWaitSema(st->sema, 1, NULL);
    if (res == (s32)0x800201A9)
        return 0;
    if (res < 0)
        return res;
    if (st->intrReq.retcode > 0) {
        sceKernelSignalSema(st->sema, 1);
        return SCE_ERROR_USB_INTR_FAILED;
    }
    cb = st->cmdBuf;
    cb[2] = cmd;
    *(u16 *)cb = arg1;
    cb[3] = len;
    if (buf != NULL)
        memcpy(cb + 4, buf, len);
    return sceUsbAccIntrInReq(&st->intrReq);
}


/* 0x000000F4 sendReverseFlags */
int sendReverseFlags(void *buf)
{
    struct VideoState *st = (struct VideoState *)&g_videoState;
    s32 res;

    res = sceKernelLockMutex(st->mutex, 1, NULL);
    if (res >= 0) {
        res = sendAccCommand(171, 2, buf, 2);
        sceKernelUnlockMutex(st->mutex, 1);
    }
    return res;
}


/* 0x00000170 queryReverseState */
int queryReverseState(void *arg)
{
    struct VideoState *st = (struct VideoState *)&g_videoState;
    u32 bits;
    u32 word;
    u8 *cb;
    s32 res;

    res = sceKernelLockMutex(st->mutex, 1, NULL);
    if ((u32)res - 0x800201A9u < 2)
        return SCE_ERROR_USBCAM_NOT_ATTACHED;
    if (res < 0)
        return res;

    sceKernelClearEventFlag(st->eventflag, 0xFFFEFFFF);
    res = sendAccCommand(43, 2, NULL, 0);
    if (res < 0)
        goto unlock;
    res = sceKernelWaitEventFlag(st->eventflag, 0x10400, 1, &bits, NULL);
    if (res < 0)
        goto unlock;
    if (bits & 0x400) {
        res = SCE_ERROR_USBCAM_NOT_ATTACHED;
        goto unlock;
    }

    cb = st->cmdBuf;
    __builtin_memcpy(arg, cb + 4, 4);
    word = *(u32 *)arg;
    word &= 0x00FFFFFF;
    *(u32 *)arg = word;
    if (word & 0x10000)
        *(u32 *)arg = word & ~0x10000u;
    else
        *(u32 *)arg = word | 0x10000;

    word = *(u32 *)arg;
    if (word & 1)
        st->flags |= 0x200;
    else
        st->flags &= ~0x200u;
    word = *(u32 *)arg;
    if (word & 0x100)
        st->flags |= 0x100;
    else
        st->flags &= ~0x100u;
    word = *(u32 *)arg;
    if (word & 0x10000)
        st->flags |= 0x400;
    else
        st->flags &= ~0x400u;

unlock:
    sceKernelUnlockMutex(st->mutex, 1);
    return res;
}


int videoBusEvent(int arg1 __attribute__((unused)), int arg2, int arg3 __attribute__((unused)))
{
    struct VideoState *st = (struct VideoState *)&g_videoState;
    int ret = 0;

    if (arg1 != 0)
        return ret;
    if (arg2 == 0) {
        if (st->altSetting == 1) {
            if (st->flags & 0x18) {
                sceKernelClearEventFlag(st->eventflag, 0xFFF7FFFF);
                ret = sceKernelSetEventFlag(st->eventflag, 0x100);
                st->flags &= ~0x18;
            } else {
                st->aux = 0;
                sceKernelClearEventFlag(st->eventflag, 0xFFFFFFEF);
                ret = sceKernelSetEventFlag(st->eventflag, 0x100);
            }
        } else {
            ret = sceKernelClearEventFlag(st->eventflag, 0xFFFFCEFF);
            st->fragHdr = 0;
            st->fragTotal = 0;
            if ((u32)st->resEx < 1280) {
                sceKernelDcacheInvalidateRange(st->bulkBuf, 128);
                ret = sceUsbbdReqRecv(&st->bulkReq);
            }
        }
    } else if (arg2 == 1) {
        sceKernelClearEventFlag(st->eventflag, 0xFFFFFEF9);
        if (st->flags & 8) {
            st->fragHdr = 0;
            st->fragTotal = 0;
            sceKernelSetEventFlag(st->eventflag, 0x80000);
        } else {
            sceKernelSetEventFlag(st->eventflag, 0x10);
        }
        ret = startIsoReceives();
    }
    st->altSetting = (u8)arg2;
    return ret;
}

int micBusEvent(int arg1, int arg2, int arg3 __attribute__((unused)))
{
    struct MicState *st = (struct MicState *)&g_micState;
    int ret = 0;
    int i;

    if (arg1 != 1)
        return ret;
    if (arg2 == 0) {
        sceKernelClearEventFlag(st->eventflag, 0xFFFFFFEF);
        ret = sceKernelSetEventFlag(st->eventflag, 32);
        st->mode = 0;
    } else {
        sceKernelClearEventFlag(st->eventflag, 0xFFFFFFDE);
        sceKernelSetEventFlag(st->eventflag, 16);
        for (i = 0; i < 4; i++) {
            memset(st->pcmBuf[i], 0, 256);
            sceKernelDcacheInvalidateRange(st->pcmBuf[i], 256);
            st->isoReq[i].unk1c = (int)&st->isoReq[i + 1];
        }
        st->isoReq[3].unk1c = 0;
        sceUsbbdReqRecv(&st->isoReq[0]);
        ret = 1;
        st->mode = 1;
    }
    st->altSetting = (u8)arg2;
    return ret;
}

int videoDetach(int arg1 __attribute__((unused)), int arg2 __attribute__((unused)), int arg3 __attribute__((unused)))
{
    struct VideoState *st = (struct VideoState *)&g_videoState;
    int i;

    if (st->attached == 0)
        return 0;
    st->mode = 2;
    st->setupFlags = 0;
    st->attached = 0;
    st->altSetting = 0;
    st->flags = 0;
    for (i = 0; i < 3; i++)
        g_videoEndpoints[i].transferred = 0;
    sceKernelClearEventFlag(st->eventflag, 0);
    sceKernelSetEventFlag(st->eventflag, 1024);
    sceKernelCancelSema(st->sema, 1, NULL);
    return sceKernelCancelMutex(st->mutex, 0, NULL);
}

int micDetach(int arg1 __attribute__((unused)), int arg2 __attribute__((unused)), int arg3 __attribute__((unused)))
{
    struct MicState *st = (struct MicState *)&g_micState;
    int i;

    if (st->attached == 0)
        return 0;
    st->mode = 5;
    st->attached = 0;
    st->altSetting = 0;
    for (i = 0; i < 2; i++)
        g_micEndpoints[i].transferred = 0;
    sceKernelClearEventFlag(st->eventflag, 0);
    sceKernelSetEventFlag(st->eventflag, 256);
    st->swapMode = 0;
    /* Original returns 0x10000 (lui residue of the g_micState address
       materialized for the store above), not 0 (verified against the disassembly). */
    return 0;
}

int videoRecvCtl(int arg1 __attribute__((unused)), int arg2, struct DeviceRequest *req)
{
    struct VideoState *st = (struct VideoState *)&g_videoState;
    u8 *blk;
    int res;
    int i;

    st->setup = *req;
    if (arg2 < 0)
        return -1;
    for (i = 0; i < 6; i++) {
        if (g_ctlRequests[i][0] == req->bmRequestType && g_ctlRequests[i][1] == req->bRequest)
            break;
    }
    if (i == 6)
        return -1;
    if ((s8)req->bmRequestType < 0) {
        blk = (u8 *)st->ep0Buf;
        if (req->bRequest == 3) {
            st->ep0Req.data = blk;
            blk[4] = 2;
            blk[5] = 0;
            blk[0] = 0;
            blk[1] = 0;
            blk[2] = 0;
            blk[3] = 0;
            st->ep0Req.size = 6;
        } else if (req->bRequest == 8) {
            st->ep0Req.data = blk;
            blk[0] = 1;
            blk[1] = 0;
            blk[2] = 0;
            blk[3] = 0;
            blk[4] = 0;
            blk[5] = 0;
            blk[6] = 0;
            blk[7] = 0;
            st->ep0Req.size = 8;
        } else {
            return 0;
        }
        sceKernelDcacheWritebackRange(st->ep0Req.data, st->ep0Req.size);
        res = sceUsbbdReqSend(&st->ep0Req);
        if (res < 0)
            Kprintf("%sin %s : Cannot issue send request : 0x%08x\n", "", "DevReqHdlr", res);
        return 0;
    }
    if (req->bRequest == 7 || req->bRequest == 9 ||
        (req->bRequest == 10 && st->setup.wValue == 16)) {
        st->ep0Req.retcode = 0;
        st->ep0Req.size = req->wLength;
        st->ep0Req.data = st->ep0Buf;
        sceKernelDcacheInvalidateRange(st->ep0Req.data, 128);
        sceUsbbdReqRecv(&st->ep0Req);
    }
    return 0;
}

int videoDriverStart(int size __attribute__((unused)), void *args __attribute__((unused)))
{
    struct VideoState *st = (struct VideoState *)&g_videoState;
    void *block;
    int res;
    int i;

    sceKernelGetSystemTimeLow();
    res = sceKernelCreateFpl("SceUsbCam", 1, 256, 5696, 1, NULL);
    if (res < 0) {
        st->fplId = -1;
        return -1;
    }
    st->fplId = res;
    if (sceKernelTryAllocateFpl(st->fplId, &block) < 0)
        goto fail;
    st->ep0Buf = block;
    st->frameBuf[0] = (u8 *)block + 128;
    st->frameBuf[1] = (u8 *)block + 1920;
    st->cmdBuf = (u8 *)block + 3840;
    st->bulkBuf = (u8 *)block + 3712;
    for (i = 0; i < 2; i++) {
        st->desc[i].buf = (u8 *)block + 3904 + i * 896;
        st->desc[i].next = (void *)((u8 *)st + 0x140 + i * 16);
        st->desc[i].len = 0;
    }
    st->desc[1].next = &st->desc[0];
    res = sceKernelCreateThread("SceUsbCam", videoWorkerThread, 17, 1024, 0x100001, NULL);
    st->threadVideo = res;
    if (res < 0)
        goto fail;
    res = sceKernelCreateThread("SceUsbCamCopyWorker", videoCopyWorker, 17, 1024, 0x100001, NULL);
    if (res < 0) {
        st->threadCopy = -1;
        goto fail;
    }
    st->threadCopy = res;
    res = sceKernelCreateEventFlag("SceUsbCam", 513, 1024, NULL);
    if (res < 0) {
        st->eventflag = -1;
        goto fail;
    }
    st->eventflag = res;
    res = sceKernelCreateSema("SceUsbCamAccIfLock", 0, 1, 1, NULL);
    if (res < 0) {
        st->sema = -1;
        goto fail;
    }
    st->sema = res;
    res = sceKernelCreateMutex("SceUsbCamCmdLock", 256, 0, NULL);
    if (res < 0) {
        st->mutex = -1;
        goto fail;
    }
    st->mutex = res;
    st->ep0Req.data = st->ep0Buf;
    st->intrReq.data = st->cmdBuf;
    st->bulkReq.data = st->bulkBuf;
    st->bulkReq.endp = &g_videoEndpoints[1];
    st->ep0Req.unkc = 1;
    st->ep0Req.func = videoEp0Complete;
    st->ep0Req.retcode = 0;
    st->intrReq.func = videoCmdComplete;
    st->bulkReq.size = 64;
    st->ep0Req.unk1c = 0;
    st->ep0Req.arg = NULL;
    st->ep0Req.recvsize = 0;
    st->intrReq.retcode = 0;
    st->ep0Req.size = 64;
    st->intrReq.unkc = 0;
    st->intrReq.unk1c = 0;
    st->intrReq.arg = NULL;
    st->intrReq.recvsize = 0;
    st->bulkReq.func = videoBulkComplete;
    st->bulkReq.retcode = 0;
    st->ep0Req.endp = &g_videoEndpoints[0];
    st->intrReq.endp = NULL;
    st->intrReq.size = 64;
    st->bulkReq.unkc = 0;
    st->bulkReq.unk1c = 0;
    st->bulkReq.arg = NULL;
    st->bulkReq.recvsize = 0;
    for (i = 0; i < 2; i++) {
        st->isoReq[i].data = st->frameBuf[i];
        st->isoReq[i].endp = &g_videoEndpoints[2];
        st->isoReq[i].size = 896;
        st->isoReq[i].unkc = 1;
        st->isoReq[i].func = videoIsoComplete;
        st->isoReq[i].unk1c = 0;
        st->isoReq[i].arg = NULL;
        st->isoReq[i].recvsize = 0;
        st->isoReq[i].retcode = 0;
    }
    if (sceUsbAccRegisterType(2) < 0)
        goto fail;
    st->lensCbid = -1;
    sceKernelStartThread(st->threadVideo, 0, NULL);
    sceKernelStartThread(st->threadCopy, 0, NULL);
    st->started = 1;
    st->dmaOp = sceKernelDmaOpAlloc();
    return 0;

fail:
    if (st->mutex > 0) {
        sceKernelDeleteMutex(st->mutex);
        st->mutex = -1;
    }
    if (st->sema > 0) {
        sceKernelDeleteSema(st->sema);
        st->sema = -1;
    }
    if (st->eventflag > 0) {
        sceKernelDeleteEventFlag(st->eventflag);
        st->eventflag = -1;
    }
    if (st->threadCopy > 0) {
        sceKernelDeleteThread(st->threadCopy);
        st->threadCopy = -1;
    }
    if (st->threadVideo > 0) {
        sceKernelDeleteThread(st->threadVideo);
        st->threadVideo = -1;
        if (st->fplId > 0) {
            sceKernelDeleteFpl(st->fplId);
            st->fplId = -1;
        }
    }
    return -1;
}

int micDriverStart(int size __attribute__((unused)), void *args __attribute__((unused)))
{
    struct MicState *st = (struct MicState *)&g_micState;
    void *block;
    int res;
    int i;

    res = sceKernelCreateFpl("SceUsbMic", 1, 256, 1088, 1, NULL);
    st->fplId = res;
    if (res < 0)
        return -1;
    if (sceKernelTryAllocateFpl(st->fplId, &block) < 0)
        goto delFpl;
    st->ep0Buf = block;
    for (i = 0; i < 4; i++)
        st->pcmBuf[i] = (u8 *)block + 64 + i * 256;
    res = sceKernelCreateThread("SceUsbMicCopyWorker", micCopyWorker, 16, 1024, 0x100001, NULL);
    st->thread = res;
    if (res < 0)
        goto delFpl;
    res = sceKernelCreateEventFlag("SceUsbMic", 513, 256, NULL);
    st->eventflag = res;
    if (res < 0)
        goto delThread;
    st->intrReq.endp = NULL;
    st->intrReq.func = micEmptyComplete;
    st->intrReq.unkc = 0;
    st->intrReq.unk1c = 0;
    st->intrReq.arg = NULL;
    st->intrReq.recvsize = 0;
    st->intrReq.retcode = 0;
    st->intrReq.data = block;
    st->intrReq.size = 64;
    for (i = 0; i < 4; i++) {
        st->isoReq[i].data = st->pcmBuf[i];
        st->isoReq[i].endp = &g_micEndpoints[1];
        st->isoReq[i].size = 128;
        st->isoReq[i].unkc = 0;
        st->isoReq[i].func = micRecvComplete;
        st->isoReq[i].unk1c = 0;
        st->isoReq[i].arg = NULL;
        st->isoReq[i].recvsize = 0;
        st->isoReq[i].retcode = 0;
    }
    st->mode = 5;
    st->cmdLo = 0;
    st->gain = 0;
    st->cmd14 = 0;
    st->setupFlags = 0;
    st->attached = 0;
    st->altSetting = 0;
    st->aux = 0;
    st->cmd0C = 0;
    st->cmd10 = 0;
    for (i = 0; i < 2; i++)
        g_micEndpoints[i].transferred = 0;
    *(u16 *)st->ep0Buf = 1;
    if (sceUsbAccRegisterType(1) < 0)
        goto delEventFlag;
    st->swapMode = 0;
    st->started = 1;
    if (sceKernelStartThread(st->thread, 0, NULL) == 0)
        return 0;

delEventFlag:
    sceKernelDeleteEventFlag(st->eventflag);
delThread:
    sceKernelDeleteThread(st->thread);
delFpl:
    sceKernelDeleteFpl(st->fplId);
    return -1;
}

int videoDriverStop(int size __attribute__((unused)), void *args __attribute__((unused)))
{
    struct VideoState *st = (struct VideoState *)&g_videoState;

    sceUsbAccUnregisterType(2);
    sceKernelSetEventFlag(st->eventflag, 0x4000);
    if (st->threadCopy > 0) {
        sceKernelWaitThreadEnd(st->threadCopy, NULL);
        sceKernelDeleteThread(st->threadCopy);
        st->threadCopy = -1;
    }
    if (st->threadVideo > 0) {
        sceKernelWaitThreadEnd(st->threadVideo, NULL);
        sceKernelDeleteThread(st->threadVideo);
        st->threadVideo = -1;
    }
    if (st->eventflag > 0) {
        sceKernelDeleteEventFlag(st->eventflag);
        st->eventflag = -1;
    }
    if (st->sema > 0) {
        sceKernelDeleteSema(st->sema);
        st->sema = -1;
    }
    if (st->mutex > 0) {
        sceKernelDeleteMutex(st->mutex);
        st->mutex = -1;
    }
    if (st->fplId > 0) {
        sceKernelDeleteFpl(st->fplId);
        st->fplId = -1;
    }
    if (st->dmaOp != NULL)
        sceKernelDmaOpFree(st->dmaOp);
    st->started = 0;
    return 0;
}

int micDriverStop(int size __attribute__((unused)), void *args __attribute__((unused)))
{
    struct MicState *st = (struct MicState *)&g_micState;

    sceUsbAccUnregisterType(1);
    sceKernelSetEventFlag(st->eventflag, 512);
    sceKernelWaitThreadEnd(st->thread, NULL);
    sceKernelDeleteThread(st->thread);
    sceKernelDeleteEventFlag(st->eventflag);
    sceKernelDeleteFpl(st->fplId);
    st->started = 0;
    return 0;
}

int encodeBrightness(int arg0)
{
    int x = (arg0 + 128) & 0xFF;

    if ((s8)x < 0)
        x = ((~x) | -128) & 0xFF;
    return x;
}

int encodeSaturation(int arg0)
{
    if (arg0 < 0)
        return 0;
    if (arg0 >= 256)
        return 5;
    return arg0 / 42;
}

int encodeSharpness(int arg0)
{
    int idx;

    if (arg0 < 0)
        idx = 0;
    else if (arg0 >= 256)
        idx = 2;
    else
        idx = arg0 / 64;
    return s_map8CB8[idx];
}

int encodeWidthCode(u8 *arg0)
{
    int i;
    u8 v = *arg0;

    for (i = 0; i < 10; i++) {
        if (s_widthIdxMap[i] == v)
            break;
    }
    return (i < 10) ? i : 0;
}

int encodeHeightCode(u8 *arg0)
{
    int i;
    u8 v = *arg0;

    for (i = 0; i < 10; i++) {
        if (s_heightIdxMap[i] == v)
            break;
    }
    return (i < 10) ? i : 0;
}

int clampEvLevel(u8 *arg0)
{
    int i;
    u8 v = *arg0;

    for (i = 0; i < 17; i++) {
        if (s_map8E28[i] == v)
            break;
    }
    return (i < 17) ? i : 0;
}

int checkEvAllowed(int arg0, int arg1, int arg2)
{
    s8 entry;

    if (arg0 >= 10 || arg1 >= 10)
        return 0;
    if (arg2 == 0)
        return 1;
    if ((u32)(arg2 - 10) >= 71)
        return 0;
    entry = s_map8E78[arg0 * 10 + arg1];
    if (entry < 0 || entry < arg2)
        return 0;
    return 1;
}

int sceUsbCamSetupVideo(struct UsbCamSetupVideoParam *param, void *workarea, int wasize)
{
    u32 out[8];
    struct UsbCamVideoReq req;
    int oldK1;
    int ret;
    u32 half;
    int i;

    oldK1 = pspShiftK1();
    ret = SCE_ERROR_USBCAM_NOT_INIT;
    if (g_videoState.started == 0)
        goto out;
    ret = SCE_ERROR_USBCAM_NOT_ATTACHED;
    if (sceUsbAccGetAuthStat() < 0)
        goto out;
    ret = SCE_ERROR_USBCAM_INVALID_PARAM;
    if (param == NULL)
        goto out;
    ret = SCE_ERROR_USBCAM_INVALID_ADDR;
    if (!pspK1StaBufOk(param, 52))
        goto out;
    if (!pspK1DynBufOk(workarea, wasize))
        goto out;
    ret = SCE_ERROR_USBCAM_INVALID_SIZE;
    if ((wasize & 0x3F) != 0)
        goto out;

    req.unk = (param->framerate < 5) ? 9 : 6;
    req.resolution = (s8)s_map8E3C[param->resolution * 2 + 1];
    req.unk2 = 2;
    req.unk3 = 3;
    req.wb = param->wb;
    req.saturation = param->saturation;
    req.brightness = param->brightness;
    req.contrast = param->contrast;
    req.sharpness = param->sharpness;
    req.framerate = param->framerate;
    for (i = 0; i < 3; i++) {
        if (s_map8E0C[i] == g_videoState.cfg.antiflicker)
            break;
    }
    req.unk4 = (i < 3) ? i : 2;
    req.unk5 = 1;
    req.effectmode = param->effectmode;
    req.unk9 = 2;
    req.unk10 = 1000;
    req.unk11 = 500;
    req.framesize = param->framesize;
    req.unk6[0] = 0;
    req.unk6[1] = 0;
    req.unk6[2] = 0;
    req.unk7 = 0;

    switch (param->size) {
    case 40:
        req.evlevel = 8;
        req.unk8 = 10;
        req.unk12 = 0;
        break;
    case 48:
        req.unk12 = param->unk;
        req.evlevel = param->evlevel;
        req.unk8 = 10;
        break;
    case 52:
        req.unk8 = param->unk2;
        req.evlevel = param->evlevel;
        req.unk12 = param->unk;
        break;
    default:
        ret = SCE_ERROR_USBCAM_INVALID_PARAM;
        goto out;
    }

    ret = packVideoConfig((u8 *)out, &req);
    if (ret < 0)
        goto out;

    for (i = 0; i < 8; i++)
        ((u32 *)&g_videoState.cfg)[i] = out[i];
    g_videoState.flags = g_videoState.flags & ~7u;
    ret = encodeWidthCode(&g_videoState.cfgTail.b[6]);
    if (ret < 7) {
        u8 v = g_videoState.cfg.framerate;

        for (i = 0; i < 8; i++) {
            if (s_map8DF8[i] == v)
                break;
        }
        if (i == 8)
            i = 7;
        g_videoState.cfgTail.b[6] = s_widthIdxMap[(i < 5) ? 9 : 6];
    }
    ret = 0;
    g_videoState.setupFlags |= 1;
    half = (u32)wasize >> 1;
    g_videoState.slot[1].buf = (u32)workarea + half;
    g_videoState.workBase = (u32)workarea;
    g_videoState.workHalf = half;
    g_videoState.slot[0].buf = (u32)workarea;
    g_videoState.slot[0].len = 0;
    g_videoState.slot[1].len = 0;

out:
    pspSetK1(oldK1);
    return ret;
}

int sceUsbCamSetupVideoEx(struct UsbCamVideoReq *param, void *workarea, int wasize)
{
    u32 out[8];
    int oldK1;
    int ret;
    u32 half;
    int i;

    oldK1 = pspShiftK1();
    ret = SCE_ERROR_USBCAM_NOT_INIT;
    if (g_videoState.started == 0)
        goto out;
    ret = SCE_ERROR_USBCAM_NOT_ATTACHED;
    if (sceUsbAccGetAuthStat() < 0)
        goto out;
    ret = SCE_ERROR_USBCAM_INVALID_PARAM;
    if (param == NULL)
        goto out;
    ret = SCE_ERROR_USBCAM_INVALID_ADDR;
    if (!pspK1StaBufOk(param, 100))
        goto out;
    if (!pspK1DynBufOk(workarea, wasize))
        goto out;
    ret = SCE_ERROR_USBCAM_INVALID_SIZE;
    if ((wasize & 0x3F) != 0)
        goto out;

    ret = packVideoConfig((u8 *)out, param);
    if (ret < 0)
        goto out;

    for (i = 0; i < 8; i++)
        ((u32 *)&g_videoState.cfg)[i] = out[i];
    half = (u32)wasize >> 1;
    g_videoState.slot[1].buf = (u32)workarea + half;
    g_videoState.flags = g_videoState.flags & ~7u;
    g_videoState.setupFlags |= 1;
    ret = 0;
    g_videoState.workBase = (u32)workarea;
    g_videoState.workHalf = half;
    g_videoState.slot[0].buf = (u32)workarea;
    g_videoState.slot[0].len = 0;
    g_videoState.slot[1].len = 0;

out:
    pspSetK1(oldK1);
    return ret;
}

int sceUsbCamReadVideoFrame(u8 *buf, SceSize size)
{
    int oldK1;
    int ret;

    oldK1 = pspShiftK1();
    ret = SCE_ERROR_USBCAM_NOT_INIT;
    if (g_videoState.started == 0)
        goto out;
    ret = SCE_ERROR_USBCAM_NOT_ATTACHED;
    if (g_videoState.attached == 0)
        goto out;
    if (sceUsbAccGetAuthStat() < 0)
        goto out;
    ret = SCE_ERROR_USBCAM_NOT_SETUP;
    if ((g_videoState.setupFlags & 1) == 0)
        goto out;
    ret = SCE_ERROR_USBCAM_INVALID_PARAM;
    if (buf == NULL)
        goto out;
    ret = SCE_ERROR_USBCAM_INVALID_ADDR;
    if (!pspK1DynBufOk(buf, size))
        goto out;
    ret = SCE_ERROR_USBCAM_INVALID_STATE;
    if (g_videoState.mode == 0)
        goto out;
    ret = SCE_ERROR_USBCAM_BUSY;
    if (g_videoState.flags & 4)
        goto out;

    g_videoState.flags |= 4;
    sceKernelClearEventFlag(g_videoState.eventflag, ~0x80u);
    g_videoState.readSize = size;
    g_videoState.readBuf = (u32)buf;
    ret = sceKernelSetEventFlag(g_videoState.eventflag, 0x40);

out:
    pspSetK1(oldK1);
    return ret;
}

int sceUsbCamWaitReadVideoFrameEnd(void)
{
    u32 outBits = 0;
    SceUInt timeout = 3000000;
    int oldK1;
    int ret;

    oldK1 = pspShiftK1();
    ret = SCE_ERROR_USBCAM_NOT_INIT;
    if (g_videoState.started == 0)
        goto out;
    ret = SCE_ERROR_USBCAM_NOT_ATTACHED;
    if (g_videoState.attached == 0)
        goto out;
    if (sceUsbAccGetAuthStat() < 0)
        goto out;
    ret = SCE_ERROR_USBCAM_NOT_SETUP;
    if ((g_videoState.setupFlags & 1) == 0)
        goto out;
    ret = SCE_ERROR_USBCAM_INVALID_STATE;
    if ((g_videoState.flags & 4) == 0)
        goto out;

    ret = sceKernelWaitEventFlag(g_videoState.eventflag, 0x480, 1, &outBits, &timeout);
    if (ret < 0) {
        if ((u32)ret == SCE_ERROR_KERNEL_WAIT_TIMEOUT) {
            sceUsbRestart(1000000);
            ret = SCE_ERROR_USBCAM_NOT_ATTACHED;
        }
        goto out;
    }
    g_videoState.flags &= ~4u;
    if (outBits & 0x400) {
        ret = SCE_ERROR_USBCAM_NOT_ATTACHED;
        goto out;
    }
    ret = g_videoState.xferResult;

out:
    pspSetK1(oldK1);
    return ret;
}

int sceUsbCamPollReadVideoFrameEnd(void)
{
    u32 outBits = 0;
    int oldK1;
    int ret;

    oldK1 = pspShiftK1();
    ret = SCE_ERROR_USBCAM_NOT_INIT;
    if (g_videoState.started == 0)
        goto out;
    ret = SCE_ERROR_USBCAM_NOT_ATTACHED;
    if (g_videoState.attached == 0)
        goto out;
    if (sceUsbAccGetAuthStat() < 0)
        goto out;
    ret = SCE_ERROR_USBCAM_NOT_SETUP;
    if ((g_videoState.setupFlags & 1) == 0)
        goto out;
    ret = SCE_ERROR_USBCAM_INVALID_STATE;
    if ((g_videoState.flags & 4) == 0)
        goto out;

    ret = sceKernelPollEventFlag(g_videoState.eventflag, 0x480, 1, &outBits);
    if ((u32)ret == SCE_ERROR_KERNEL_EVENT_FLAG_POLL_FAILED) {
        ret = SCE_ERROR_USBCAM_NOT_READY;
        goto out;
    }
    if (ret < 0)
        goto out;
    g_videoState.flags &= ~4u;
    if (outBits & 0x400) {
        ret = SCE_ERROR_USBCAM_NOT_ATTACHED;
        goto out;
    }
    ret = g_videoState.xferResult;

out:
    pspSetK1(oldK1);
    return ret;
}

int sceUsbCamReadVideoFrameBlocking(u8 *buf, SceSize size)
{
    u32 outBits = 0;
    int oldK1;
    int ret;

    oldK1 = pspShiftK1();
    ret = SCE_ERROR_USBCAM_NOT_INIT;
    if (g_videoState.started == 0)
        goto out;
    ret = SCE_ERROR_USBCAM_NOT_ATTACHED;
    if (g_videoState.attached == 0)
        goto out;
    if (sceUsbAccGetAuthStat() < 0)
        goto out;
    ret = SCE_ERROR_USBCAM_NOT_SETUP;
    if ((g_videoState.setupFlags & 1) == 0)
        goto out;
    ret = SCE_ERROR_USBCAM_INVALID_PARAM;
    if (buf == NULL)
        goto out;
    ret = SCE_ERROR_USBCAM_INVALID_ADDR;
    if (!pspK1DynBufOk(buf, size))
        goto out;
    ret = SCE_ERROR_USBCAM_BUSY;
    if (g_videoState.flags & 4)
        goto out;

    g_videoState.flags |= 4;
    sceKernelClearEventFlag(g_videoState.eventflag, ~0x80u);
    g_videoState.readBuf = (u32)buf;
    g_videoState.readSize = size;
    sceKernelSetEventFlag(g_videoState.eventflag, 0x40);
    ret = sceKernelWaitEventFlag(g_videoState.eventflag, 0x480, 1, &outBits, NULL);
    if (ret < 0)
        goto out;
    g_videoState.flags &= ~4u;
    if (outBits & 0x400) {
        ret = SCE_ERROR_USBCAM_NOT_ATTACHED;
        goto out;
    }
    ret = g_videoState.xferResult;

out:
    pspSetK1(oldK1);
    return ret;
}

int sceUsbCamGetReadVideoFrameSize(void)
{
    int oldK1;
    s32 intr;
    int ret;

    oldK1 = pspShiftK1();
    ret = SCE_ERROR_USBCAM_NOT_INIT;
    if (g_videoState.started == 0)
        goto out;
    ret = SCE_ERROR_USBCAM_NOT_ATTACHED;
    if (g_videoState.attached == 0)
        goto out;
    if (sceUsbAccGetAuthStat() < 0)
        goto out;
    ret = SCE_ERROR_USBCAM_NOT_SETUP;
    if ((g_videoState.setupFlags & 1) == 0)
        goto out;

    intr = sceKernelCpuSuspendIntr();
    ret = g_videoState.xferLength;
    sceKernelCpuResumeIntr(intr);

out:
    pspSetK1(oldK1);
    return ret;
}

/* PspUsbCamSetupStillParam (pspusbcam.h:131-145). The driver range-checks
   param..param+24 with pspK1StaBufOk (asm 0x1D34) and accepts size 20..24;
   a 20-byte build has no complevel, so 0x1DEC falls back to 10. */
struct UsbCamSetupStillParam {
    int size;
    int resolution;
    int jpegsize;
    int reverseflags;
    int delay;
    int complevel;
};

_Static_assert(sizeof(struct UsbCamSetupStillParam) == 24, "UsbCamSetupStillParam size");

/* PspUsbCamSetupStillExParam (pspusbcam.h:148-172). Field-for-field the
   same 15-int layout as struct UsbCamStillReq; sceUsbCamSetupStill builds
   one of these on the stack and both entry points hand it to
   packStillConfig through that type. */
struct UsbCamSetupStillExParam {
    int size;
    u32 unk;
    int resolution;
    int jpegsize;
    int complevel;
    u32 unk2;
    u32 unk3;
    int flip;
    int mirror;
    int delay;
    u32 unk4[5];
};

_Static_assert(sizeof(struct UsbCamSetupStillExParam) == 60, "UsbCamSetupStillExParam size");

/* packStillConfig is defined below; struct UsbCamStillReq is
   forward-declared here and completed below. */
struct UsbCamStillReq;
/* ============================================================
 * Section: still capture setup and input
 * ============================================================ */

int packStillConfig(u8 *out, struct UsbCamStillReq *req);
s32 guardStillInput(void);
s32 reapStillInput(int arg);
s32 armStillRead(void *buf, int size);

s32 sceUsbCamSetupStill(struct UsbCamSetupStillParam *param)
{
    struct UsbCamSetupStillExParam req;
    u8 out[20];
    int oldK1;
    int ret;
    u8 v;
    int i;

    oldK1 = pspShiftK1();
    ret = SCE_ERROR_USBCAM_NOT_INIT;
    if (g_videoState.started == 0)
        goto out;
    ret = SCE_ERROR_USBCAM_NOT_ATTACHED;
    if (g_videoState.attached == 0)
        goto out;
    if (sceUsbAccGetAuthStat() < 0)
        goto out;
    ret = SCE_ERROR_USBCAM_INVALID_PARAM;
    if (param == NULL)
        goto out;
    ret = SCE_ERROR_USBCAM_INVALID_ADDR;
    if (!pspK1StaBufOk(param, 24))
        goto out;
    ret = SCE_ERROR_USBCAM_INVALID_PARAM;
    if ((u32)(param->size - 20) >= 5)
        goto out;

    v = g_videoState.cfg.framerate;
    for (i = 0; i < 8; i++) {
        if (s_map8DF8[i] == v)
            break;
    }
    if (i == 8)
        i = 7;
    req.unk = (i < 5) ? 9 : 6;
    req.resolution = s_map8E3C[param->resolution * 2 + 1];
    req.jpegsize = param->jpegsize;
    req.complevel = (param->size < 24) ? 10 : param->complevel;
    req.unk3 = 2;
    req.flip = param->reverseflags & 1;
    req.mirror = (param->reverseflags >> 8) & 1;
    req.delay = param->delay;
    req.unk4[0] = 2;
    req.unk4[1] = 500;
    req.unk4[2] = 250;
    req.unk4[3] = 0;
    req.unk4[4] = 0;

    ret = packStillConfig(out, (struct UsbCamStillReq *)&req);
    if (ret < 0)
        goto out;
    __builtin_memcpy(&g_videoState.cfgTail.b[0], out, 20);
    ret = 0;
    g_videoState.setupFlags |= 2;
    g_videoState.flags &= ~8u;

out:
    pspSetK1(oldK1);
    return ret;
}

/* 0x1EBC sceUsbCamSetupStillEx */

s32 sceUsbCamSetupStillEx(struct UsbCamSetupStillExParam *param)
{
    u8 out[20];
    int oldK1;
    int ret;

    oldK1 = pspShiftK1();
    ret = SCE_ERROR_USBCAM_NOT_INIT;
    if (g_videoState.started == 0)
        goto out;
    ret = SCE_ERROR_USBCAM_NOT_ATTACHED;
    if (g_videoState.attached == 0)
        goto out;
    if (sceUsbAccGetAuthStat() < 0)
        goto out;
    ret = SCE_ERROR_USBCAM_INVALID_PARAM;
    if (param == NULL)
        goto out;
    ret = SCE_ERROR_USBCAM_INVALID_ADDR;
    if (!pspK1StaBufOk(param, 60))
        goto out;

    ret = packStillConfig(out, (struct UsbCamStillReq *)param);
    if (ret < 0)
        goto out;
    __builtin_memcpy(&g_videoState.cfgTail.b[0], out, 20);
    ret = 0;
    g_videoState.setupFlags |= 2;
    g_videoState.flags &= ~8u;

out:
    pspSetK1(oldK1);
    return ret;
}

/* 0x1FE4 sceUsbCamStillInput */

s32 sceUsbCamStillInput(u8 *buf, SceSize size)
{
    int oldK1;
    int ret;
    int h;
    int w;

    oldK1 = pspShiftK1();
    ret = SCE_ERROR_USBCAM_NOT_INIT;
    if (g_videoState.started == 0)
        goto out;
    ret = SCE_ERROR_USBCAM_NOT_ATTACHED;
    if (g_videoState.attached == 0)
        goto out;
    if (sceUsbAccGetAuthStat() < 0)
        goto out;
    ret = SCE_ERROR_USBCAM_INVALID_SIZE;
    if (size < 64)
        goto out;
    ret = SCE_ERROR_USBCAM_INVALID_ADDR;
    if (!pspK1DynBufOk(buf, size))
        goto out;
    ret = SCE_ERROR_USBCAM_NOT_SETUP;
    if ((g_videoState.setupFlags & 2) == 0)
        goto out;

    h = encodeHeightCode(&g_videoState.cfgTail.b[7]);
    w = encodeWidthCode(&g_videoState.cfgTail.b[6]);
    if (w < h) {
        ret = SCE_ERROR_USBCAM_INVALID_RES;
        goto out;
    }

    sceKernelClearEventFlag(g_videoState.eventflag, ~0x3000u);
    ret = armStillRead(buf, size);

out:
    pspSetK1(oldK1);
    return ret;
}

/* 0x20FC sceUsbCamStillPollInputEnd */

s32 sceUsbCamStillPollInputEnd(void)
{
    int oldK1;
    int ret;

    oldK1 = pspShiftK1();
    ret = guardStillInput();
    if (ret >= 0)
        ret = reapStillInput(1);
    pspSetK1(oldK1);
    return ret;
}

/* 0x2138 sceUsbCamStillWaitInputEnd */

s32 sceUsbCamStillWaitInputEnd(void)
{
    int oldK1;
    int ret;

    oldK1 = pspShiftK1();
    ret = guardStillInput();
    if (ret >= 0)
        ret = reapStillInput(0);
    pspSetK1(oldK1);
    return ret;
}

/* 0x2174 sceUsbCamStillInputBlocking */

s32 sceUsbCamStillInputBlocking(u8 *buf, SceSize size)
{
    int oldK1;
    int ret;

    oldK1 = pspShiftK1();
    ret = SCE_ERROR_USBCAM_NOT_INIT;
    if (g_videoState.started == 0)
        goto out;
    ret = SCE_ERROR_USBCAM_NOT_ATTACHED;
    if (g_videoState.attached == 0)
        goto out;
    if (sceUsbAccGetAuthStat() < 0)
        goto out;
    ret = SCE_ERROR_USBCAM_INVALID_SIZE;
    if (size < 64)
        goto out;
    ret = SCE_ERROR_USBCAM_INVALID_ADDR;
    if (!pspK1DynBufOk(buf, size))
        goto out;
    ret = SCE_ERROR_USBCAM_NOT_SETUP;
    if ((g_videoState.setupFlags & 2) == 0)
        goto out;

    sceKernelClearEventFlag(g_videoState.eventflag, ~0x3000u);
    ret = armStillRead(buf, size);
    if (ret >= 0)
        ret = reapStillInput(0);

out:
    pspSetK1(oldK1);
    return ret;
}

/* guardStillInput and dispatchIoctl are defined below; declared here
   for the callers. */
s32 guardStillInput(void);
s32 dispatchIoctl(int cmd, int *arg);

s32 sceUsbCamStillGetInputLength(void)
{
    int oldK1;
    s32 ret;

    oldK1 = pspShiftK1();
    ret = SCE_ERROR_USBCAM_NOT_INIT;
    if (g_videoState.started == 0)
        goto out;
    ret = SCE_ERROR_USBCAM_NOT_ATTACHED;
    if (g_videoState.attached == 0)
        goto out;
    if (sceUsbAccGetAuthStat() < 0)
        goto out;
    ret = SCE_ERROR_USBCAM_NOT_SETUP;
    if ((g_videoState.setupFlags & 2) == 0)
        goto out;
    ret = (g_videoState.stillSize < g_videoState.fragTotal) ? g_videoState.stillSize
                                                  : g_videoState.fragTotal;

out:
    pspSetK1(oldK1);
    return ret;
}

/* 0x230C sceUsbCamStillCancelInput */

s32 sceUsbCamStillCancelInput(void)
{
    int oldK1;
    s32 ret;
    s32 intr;

    oldK1 = pspShiftK1();
    ret = guardStillInput();
    if (ret < 0)
        goto out;
    intr = sceKernelCpuSuspendIntr();
    g_videoState.stillSize = 0;
    g_videoState.stillAvail = 0;
    g_videoState.stillBuf = 0;
    sceKernelSetEventFlag(g_videoState.eventflag, 0x2000);
    if (g_videoState.resEx >= 1280)
        g_videoState.flags |= 0x10;
    g_videoState.flags = g_videoState.flags & ~8u;
    sceKernelCpuResumeIntr(intr);

out:
    pspSetK1(oldK1);
    return ret;
}

/* 0x23B4 sceUsbCamGetSaturation */

s32 sceUsbCamGetSaturation(int *saturation)
{
    int oldK1;
    s32 ret;

    oldK1 = pspShiftK1();
    ret = SCE_ERROR_USBCAM_INVALID_ADDR;
    if (!pspK1StaBufOk(saturation, 4))
        goto out;
    ret = dispatchIoctl(0x80000003, saturation);

out:
    pspSetK1(oldK1);
    return ret;
}

/* 0x2414 sceUsbCamGetBrightness */

s32 sceUsbCamGetBrightness(int *brightness)
{
    int oldK1;
    s32 ret;

    oldK1 = pspShiftK1();
    ret = SCE_ERROR_USBCAM_INVALID_ADDR;
    if (!pspK1StaBufOk(brightness, 4))
        goto out;
    ret = dispatchIoctl(0x80000001, brightness);

out:
    pspSetK1(oldK1);
    return ret;
}

/* 0x2474 sceUsbCamGetContrast */

s32 sceUsbCamGetContrast(int *contrast)
{
    int oldK1;
    s32 ret;

    oldK1 = pspShiftK1();
    ret = SCE_ERROR_USBCAM_INVALID_ADDR;
    if (!pspK1StaBufOk(contrast, 4))
        goto out;
    ret = dispatchIoctl(0x80000002, contrast);

out:
    pspSetK1(oldK1);
    return ret;
}

/* 0x24D4 sceUsbCamGetSharpness */

s32 sceUsbCamGetSharpness(int *sharpness)
{
    int oldK1;
    s32 ret;

    oldK1 = pspShiftK1();
    ret = SCE_ERROR_USBCAM_INVALID_ADDR;
    if (!pspK1StaBufOk(sharpness, 4))
        goto out;
    ret = dispatchIoctl(0x80000004, sharpness);

out:
    pspSetK1(oldK1);
    return ret;
}

/* 0x2534 sceUsbCamGetZoom */

s32 sceUsbCamGetZoom(int *zoom)
{
    int oldK1;
    s32 ret;

    oldK1 = pspShiftK1();
    ret = SCE_ERROR_USBCAM_INVALID_ADDR;
    if (!pspK1StaBufOk(zoom, 4))
        goto out;
    ret = dispatchIoctl(0x80000005, zoom);

out:
    pspSetK1(oldK1);
    return ret;
}

/* 0x2594 sceUsbCamGetAntiFlicker */

s32 sceUsbCamGetAntiFlicker(int *antiflicker)
{
    int oldK1;
    s32 ret;

    oldK1 = pspShiftK1();
    ret = SCE_ERROR_USBCAM_INVALID_ADDR;
    if (!pspK1StaBufOk(antiflicker, 4))
        goto out;
    ret = dispatchIoctl(0x80000010, antiflicker);

out:
    pspSetK1(oldK1);
    return ret;
}

/* 0x25F4 sceUsbCamGetEvLevel */

s32 sceUsbCamGetEvLevel(int *ev)
{
    int oldK1;
    s32 ret;

    oldK1 = pspShiftK1();
    ret = SCE_ERROR_USBCAM_INVALID_ADDR;
    if (!pspK1StaBufOk(ev, 4))
        goto out;
    ret = dispatchIoctl(0x80000014, ev);

out:
    pspSetK1(oldK1);
    return ret;
}

/* 0x2654 sceUsbCamGetReverseMode */

s32 sceUsbCamGetReverseMode(int *reverseflags)
{
    int oldK1;
    s32 ret;

    oldK1 = pspShiftK1();
    ret = SCE_ERROR_USBCAM_INVALID_ADDR;
    if (!pspK1StaBufOk(reverseflags, 4))
        goto out;
    ret = dispatchIoctl(0x80000006, reverseflags);

out:
    pspSetK1(oldK1);
    return ret;
}

/* 0x26B4 sceUsbCamGetImageEffectMode */

s32 sceUsbCamGetImageEffectMode(int *effectmode)
{
    int oldK1;
    s32 ret;

    oldK1 = pspShiftK1();
    ret = SCE_ERROR_USBCAM_INVALID_ADDR;
    if (!pspK1StaBufOk(effectmode, 4))
        goto out;
    ret = dispatchIoctl(0x80000007, effectmode);

out:
    pspSetK1(oldK1);
    return ret;
}

/* 0x2714 sceUsbCam_00631D06 */

s32 sceUsbCam_00631D06(void)
{
    u32 val;
    s32 ret;

    val = g_videoState.resEx;
    if (val == 0) {
        ret = SCE_ERROR_USBCAM_NOT_ATTACHED;
    } else if (val - 1 < 1279) {
        ret = 1;
    } else {
        /* Unreachable as 0: the asm's movn only clears when val < 1280,
           which the test above already excluded. Kept for the asm. */
        ret = (val < 1280) ? 0 : 2;
    }
    return ret;
}

/* 0x274C sceUsbCamGetLensDirection */

extern __typeof__(sceUsbCam_00631D06) sceUsbCam_driver_00631D06 __attribute__((alias("sceUsbCam_00631D06")));

s32 sceUsbCamGetLensDirection(void)
{
    int oldK1;
    u32 bits;
    s32 res;
    s32 ret;

    oldK1 = pspShiftK1();
    res = sceKernelPollEventFlag(g_videoState.eventflag, 0x400, 1, &bits);
    if (res < 0 && res != (s32)0x800201AF) {
        ret = res;
        goto out;
    }
    if (res >= 0 && (bits & 0x400)) {
        ret = SCE_ERROR_USBCAM_NOT_ATTACHED;
        goto out;
    }
    if (g_videoState.flags & 0x2000)
        ret = ((g_videoState.flags ^ 0x400) >> 10) & 1;
    else
        ret = SCE_ERROR_USBCAM_NOT_SETUP;

out:
    pspSetK1(oldK1);
    return ret;
}

/* 0x27F0 sceUsbCamRegisterLensRotationCallback */

s32 sceUsbCamRegisterLensRotationCallback(SceUID cbid)
{
    int oldK1;
    s32 intr;
    s32 ret;

    oldK1 = pspShiftK1();
    ret = 0;
    if (sceKernelGetThreadmanIdType(cbid) != SCE_KERNEL_TMID_Callback) {
        ret = SCE_ERROR_USBCAM_INVALID_RES;
        goto out;
    }
    intr = sceKernelCpuSuspendIntr();
    if (g_videoState.lensCbid > 0) {
        g_videoState.lensCbid = -1;
        ret = SCE_ERROR_USBCAM_ALREADY;
    } else {
        g_videoState.lensCbid = cbid;
    }
    sceKernelCpuResumeIntr(intr);

out:
    pspSetK1(oldK1);
    return ret;
}

s32 module_start(SceSize args __attribute__((unused)), void *argp __attribute__((unused)))
{
    registerMicDriver(0, 0);
    if (sceUsbbdRegister(&g_videoDriver) < 0) {
        return 1;
    }
    g_videoState.started = 0;
    g_videoState.threadCopy = -1;
    g_videoState.resEx = 0;
    g_videoState.fplId = -1;
    g_videoState.eventflag = -1;
    g_videoState.sema = -1;
    g_videoState.threadVideo = -1;
    return 0;
}

s32 module_stop(SceSize args __attribute__((unused)), void *argp __attribute__((unused)))
{
    if (sceUsbbdUnregister(&g_videoDriver) < 0) {
        return 1;
    }
    unregisterMicDriver(0, 0);
    return 0;
}

/* Map tables completed with initializers later in this file. */
static const u8 s_map8E00[4];
static const u8 s_map8E04[4];
static const u8 s_map8E08[4];
static const u8 s_map8E10[7];

struct UsbCamResEntry {
    u16 w;
    u16 h;
};

_Static_assert(sizeof(struct UsbCamResEntry) == 4, "UsbCamResEntry size");

static const struct UsbCamResEntry s_res8E50[10];

int encodeImageEffect(int val)
{
    return s_map8E10[val];
}

/* 0x2944 decodeImageEffect */

int decodeImageEffect(u8 *p)
{
    int i;
    u8 v = *p;

    for (i = 0; i < 7; i++) {
        if (s_map8E10[i] == v)
            break;
    }
    return (i < 7) ? i : 6;
}

/* 0x2988 encodeFramerate */

int encodeFramerate(int val)
{
    return s_map8DF8[val];
}

/* 0x299C decodeFramerate */

int decodeFramerate(u8 *p)
{
    int i;
    u8 v = *p;

    for (i = 0; i < 8; i++) {
        if (s_map8DF8[i] == v)
            break;
    }
    return (i < 8) ? i : 7;
}

/* 0x29E0 encodeUnk0xE */

int encodeUnk0xE(int val)
{
    return s_map8E00[val];
}

/* 0x29F4 decodeUnk0xE */

int decodeUnk0xE(u8 *p)
{
    int i;
    u8 v = *p;

    for (i = 0; i < 4; i++) {
        if (s_map8E00[i] == v)
            break;
    }
    return (i < 4) ? i : 3;
}

/* 0x2A38 encodeWhiteBalance */

int encodeWhiteBalance(int val)
{
    return s_map8E04[val];
}

/* 0x2A4C decodeWhiteBalance */

int decodeWhiteBalance(u8 *p)
{
    int i;
    u8 v = *p;

    for (i = 0; i < 4; i++) {
        if (s_map8E04[i] == v)
            break;
    }
    return (i < 4) ? i : 3;
}

/* 0x2A90 encodeAntiFlicker */

int encodeAntiFlicker(int val)
{
    return s_map8E0C[val];
}

/* 0x2AA4 decodeAntiFlicker */

int decodeAntiFlicker(u8 *p)
{
    int i;
    u8 v = *p;

    for (i = 0; i < 3; i++) {
        if (s_map8E0C[i] == v)
            break;
    }
    return (i < 3) ? i : 2;
}

/* 0x2AE8 encodeUnk0x13 */

int encodeUnk0x13(int val)
{
    return s_map8E08[val];
}

/* 0x2AFC decodeUnk0x13 */

int decodeUnk0x13(u8 *p)
{
    int i;
    u8 v = *p;

    for (i = 0; i < 3; i++) {
        if (s_map8E08[i] == v)
            break;
    }
    return (i < 3) ? i : 2;
}

/* 0x2B40 encodeResolutionEx */

int encodeResolutionEx(int val, u8 *out0, u8 *out1)
{
    if ((u32)val >= 9)
        return SCE_ERROR_USBCAM_INVALID_RES;
    *out0 = 9;
    *out1 = (u8)s_map8E3C[val * 2 + 1];
    return 0;
}

/* 0x2B7C encodeResolutionPair */

int encodeResolutionPair(int val0, int val1, u8 *out0, u8 *out1)
{
    s32 wdiff = s_res8E50[val0].w - s_res8E50[val1].w;
    s32 hdiff = s_res8E50[val0].h - s_res8E50[val1].h;

    if ((hdiff | wdiff) < 0)
        return SCE_ERROR_USBCAM_INVALID_RES;
    *out0 = s_widthIdxMap[val0];
    *out1 = s_heightIdxMap[val1];
    return 0;
}

/* 0x2C08 encodeEvLevel */

int encodeEvLevel(int val)
{
    return s_map8E28[val];
}

/* startVideoStream, stopVideoStream and dispatchIoctl are defined below;
   declared here for the callers (identical redeclaration is legal C). */
s32 startVideoStream(void);
s32 stopVideoStream(void);
s32 dispatchIoctl(int cmd, int *arg);

s32 sceUsbCamStartVideo(void)
{
    int oldK1;
    s32 ret;

    oldK1 = pspShiftK1();
    ret = startVideoStream();
    pspSetK1(oldK1);
    return ret;
}

/* 0x2C48 sceUsbCamStopVideo */

s32 sceUsbCamStopVideo(void)
{
    int oldK1;
    s32 ret;

    oldK1 = pspShiftK1();
    ret = stopVideoStream();
    pspSetK1(oldK1);
    return ret;
}

/* 0x2C74 sceUsbCamSetSaturation */

s32 sceUsbCamSetSaturation(int saturation)
{
    int oldK1;
    int val = saturation;
    s32 ret;

    oldK1 = pspShiftK1();
    ret = dispatchIoctl(3, &val);
    pspSetK1(oldK1);
    return ret;
}

/* 0x2CB0 sceUsbCamSetBrightness */

s32 sceUsbCamSetBrightness(int brightness)
{
    int oldK1;
    int val = brightness;
    s32 ret;

    oldK1 = pspShiftK1();
    ret = dispatchIoctl(1, &val);
    pspSetK1(oldK1);
    return ret;
}

/* 0x2CEC sceUsbCamSetContrast */

s32 sceUsbCamSetContrast(int contrast)
{
    int oldK1;
    int val = contrast;
    s32 ret;

    oldK1 = pspShiftK1();
    ret = dispatchIoctl(2, &val);
    pspSetK1(oldK1);
    return ret;
}

/* 0x2D28 sceUsbCamSetSharpness */

s32 sceUsbCamSetSharpness(int sharpness)
{
    int oldK1;
    int val = sharpness;
    s32 ret;

    oldK1 = pspShiftK1();
    ret = dispatchIoctl(4, &val);
    pspSetK1(oldK1);
    return ret;
}

/* 0x2D64 sceUsbCamSetZoom */

s32 sceUsbCamSetZoom(int zoom)
{
    int oldK1;
    int val = zoom;
    s32 ret;

    oldK1 = pspShiftK1();
    ret = dispatchIoctl(5, &val);
    pspSetK1(oldK1);
    return ret;
}

/* 0x2DA0 sceUsbCamSetAntiFlicker */

s32 sceUsbCamSetAntiFlicker(int antiflicker)
{
    int oldK1;
    int val = antiflicker;
    s32 ret;

    oldK1 = pspShiftK1();
    ret = dispatchIoctl(16, &val);
    pspSetK1(oldK1);
    return ret;
}

/* 0x2DDC sceUsbCamSetReverseMode */

s32 sceUsbCamSetReverseMode(int reverseflags)
{
    int oldK1;
    int val = reverseflags;
    s32 ret;

    oldK1 = pspShiftK1();
    ret = dispatchIoctl(6, &val);
    pspSetK1(oldK1);
    return ret;
}

/* 0x2E18 sceUsbCamSetEvLevel */

s32 sceUsbCamSetEvLevel(int ev)
{
    int oldK1;
    int val = ev;
    s32 ret;

    oldK1 = pspShiftK1();
    ret = dispatchIoctl(20, &val);
    pspSetK1(oldK1);
    return ret;
}

/* 0x2E54 sceUsbCamSetImageEffectMode */

s32 sceUsbCamSetImageEffectMode(int effectmode)
{
    int oldK1;
    int val = effectmode;
    s32 ret;

    oldK1 = pspShiftK1();
    ret = dispatchIoctl(7, &val);
    pspSetK1(oldK1);
    return ret;
}

/* 0x2E90 sceUsbCamSetResolution */

s32 sceUsbCamSetResolution(int resolution)
{
    int oldK1;
    int val = resolution;
    s32 ret;

    oldK1 = pspShiftK1();
    ret = dispatchIoctl(9, &val);
    pspSetK1(oldK1);
    return ret;
}

/* 0x2ECC sceUsbCamAutoImageReverseSW */

s32 sceUsbCamAutoImageReverseSW(int on)
{
    if (on != 0)
        g_videoState.flags |= 0x1000;
    else
        g_videoState.flags = g_videoState.flags & ~0x1000u;
    return 0;
}

/* 0x2F00 sceUsbCamGetAutoImageReverseState */

s32 sceUsbCamGetAutoImageReverseState(void)
{
    return (g_videoState.flags >> 12) & 1;
}

/* 0x2F10 sceUsbCamUnregisterLensRotationCallback */

s32 sceUsbCamUnregisterLensRotationCallback(void)
{
    int oldK1;
    s32 intr;
    s32 cbid;
    s32 ret;

    oldK1 = pspShiftK1();
    intr = sceKernelCpuSuspendIntr();
    cbid = g_videoState.lensCbid;
    ret = SCE_ERROR_USBCAM_NO_CALLBACK;
    if (cbid >= 0) {
        g_videoState.lensCbid = -1;
        ret = cbid;
    }
    sceKernelCpuResumeIntr(intr);
    pspSetK1(oldK1);
    return ret;
}

/* Accessory info blob compared by micAttach (orig rodata 0x9014). */
static const u8 s_usbAccInfoMagic[8] =
    { 0x4C, 0x05, 0x5B, 0x02, 0x01, 0x10, 0x01, 0x00 };

int videoAttach(int speed __attribute__((unused)), void *arg2 __attribute__((unused)),
                 void *arg3 __attribute__((unused)))
{
    u8 ret = g_videoState.attached;

    if (ret != 0)
        return ret;

    g_videoState.attached = 1;
    g_videoState.mode = 0;
    g_videoState.flags = 0;
    g_videoState.resEx = 0;
    resetVideoDefaults();
    sceKernelClearEventFlag(g_videoState.eventflag, 0);

    return sceKernelSetEventFlag(g_videoState.eventflag, 0x300);
}

int micAttach(int speed __attribute__((unused)), void *arg2 __attribute__((unused)),
                 void *arg3 __attribute__((unused)))
{
    u8 ret = g_micState.attached;
    u64 info;
    int res;

    if (ret != 0)
        return ret;

    g_micState.attached = 1;
    g_micState.mode = 0;
    sceKernelClearEventFlag(g_micState.eventflag, 0);
    sceKernelSetEventFlag(g_micState.eventflag, 0x20);

    res = sceUsbAccGetInfo(&info);
    if (res != 0)
        return res;

    res = memcmp(&info, s_usbAccInfoMagic, 8);
    if (res != 0)
        return res;

    g_micState.swapMode = 1;
    Kprintf("%s16 aligned data swap\n", "usbcammic: ");

    return 0;
}

/* 0x307C videoNullCallback */
int videoNullCallback(int arg1 __attribute__((unused)), int arg2 __attribute__((unused)),
                      int arg3 __attribute__((unused)))
{
    /* Body is a bare "jr $ra"; $v0 is undefined on entry and the caller
       (g_videoDriver.unk34) ignores it - return 0 by convention. */
    return 0;
}


int micAccumCallback(int arg1 __attribute__((unused)), int arg2 __attribute__((unused)),
                     int arg3 __attribute__((unused)))
{
    u8 *p = *(u8 **)arg3;
    u8 *q = *(u8 **)(p + 16);

    q[8] += (u8)g_micIntp.unk4;

    return 0;
}


void videoCmdComplete(struct UsbdDeviceReq *req __attribute__((unused)))
{
    sceKernelSignalSema(g_videoState.sema, 1);
}

/* 0x30C8 micEmptyComplete */
void micEmptyComplete(struct UsbdDeviceReq *req __attribute__((unused)))
{
    /* Body is a bare "jr $ra"; the microphone receive completion callback
       installed as g_micState.reqD.func does nothing. */
}


/* 0x30D0 videoIsoComplete */
void videoIsoComplete(struct UsbdDeviceReq *req)
{
    struct VideoState *st = (struct VideoState *)&g_videoState;
    struct VideoDescState *node;
    struct VideoDescState *prev;
    struct VideoDescState *start;
    struct VideoDescState *next;
    u8 *src;
    u32 recvsize;
    u32 remaining;
    u32 count;
    u32 len;
    int i;

    if (g_videoState.mode == 2) {
        for (i = 0; i < 2; i++) {
            if (st->isoReq[i].retcode > 0)
                return;
        }
        sceKernelSetEventFlag(st->eventflag, 2);
        return;
    }
    if (req->retcode < 0) {
        if (g_videoState.aux == 1)
            sceKernelSetEventFlag(st->eventflag, 0x8000);
        return;
    }

    node = (struct VideoDescState *)g_videoState.xferHead;
    if (node->cap == 1)
        goto fill;
    prev = node;
    node = node->next;
    if (node == prev)
        goto picked;
    start = prev;
walk:
    if (node->cap != 1)
        goto picked;
    node = node->next;
    if (start != node)
        goto walk;
picked:
    if ((struct VideoDescState *)g_videoState.xferHead == node)
        return;

fill:
    recvsize = (u32)req->recvsize;
    if (recvsize == 0) {
        node->cap = 0;
        goto signal;
    }
    src = (u8 *)req->data;
    remaining = recvsize;
copy:
    len = (u32)node->len;
    count = (896 - len < remaining) ? 896 - len : remaining;
    memcpy((u8 *)node->buf + len, src, count);
    remaining -= count;
    src += count;
    len = (u32)node->len + count;
    node->len = (int)len;
    if (len >= 896)
        goto full;
cont:
    if (remaining != 0)
        goto copy;
tail:
    recvsize = (u32)req->recvsize;
    if (recvsize == 0) {
        node->cap = 0;
        goto signal;
    }
    /* 0x31F0: recvsize == 896 * (((recvsize >> 7) * 0x24924936) >> 32) */
    if (recvsize % 896 != 0)
        goto drain;
    goto modeCheck;

drain:
    node->cap = 0;
signal:
    next = node->next;
    g_videoState.xferHead = next;
    sceKernelSetEventFlag(st->eventflag, 4);
    goto modeCheck;

full:
    node->cap = 0;
    next = node->next;
    g_videoState.xferHead = next;
    sceKernelSetEventFlag(st->eventflag, 4);
    if (next->cap != 1)
        goto tail;
    node = next;
    goto cont;

modeCheck:
    if (g_videoState.mode >= 2)
        return;
    sceKernelDcacheInvalidateRange(req->data, 1792);
    req->unk1c = 0;
    sceUsbbdReqRecv(req);
}

/* 0x3324 resetVideoDefaults */
s32 resetVideoDefaults(void)
{
    u8 *blk = &g_videoState.cfg.width;

    blk[0] = 2;
    blk[1] = 7;
    blk[2] = 6;
    blk[6] = 125;
    blk[7] = 0x80;
    blk[8] = 64;
    blk[11] = 1;
    blk[20] = 10;
    blk[21] = 2;
    blk[22] = 0xE8;
    blk[23] = 3;
    blk[24] = 0xF4;
    blk[25] = 1;
    blk[27] = 0x80;
    blk[53] = 0;
    blk[3] = 2;
    blk[4] = 0;
    blk[5] = 0;
    blk[9] = 0;
    blk[10] = 0;
    blk[12] = 0;
    blk[13] = 0;
    blk[14] = 0;
    blk[15] = 0;
    blk[16] = 0;
    blk[17] = 0;
    blk[18] = 0;
    blk[19] = 0;
    blk[26] = 0;
    blk[28] = 0;
    blk[29] = 0;
    blk[52] = 0;
    return 0;
}


/* 0x33E8 startVideoStream */
s32 startVideoStream(void)
{
    s32 intr;
    s32 ret;

    intr = sceKernelCpuSuspendIntr();
    ret = 0;
    if (g_videoState.started == 0) {
        ret = SCE_ERROR_USBCAM_NOT_INIT;
        goto out;
    }
    if (g_videoState.attached == 0) {
        ret = SCE_ERROR_USBCAM_NOT_ATTACHED;
        goto out;
    }
    if (sceUsbAccGetAuthStat() < 0) {
        ret = SCE_ERROR_USBCAM_NOT_ATTACHED;
        goto out;
    }
    if ((g_videoState.setupFlags & 1) == 0) {
        ret = SCE_ERROR_USBCAM_NOT_SETUP;
        goto out;
    }
    if (g_videoState.mode != 0) {
        ret = SCE_ERROR_USBCAM_BUSY;
        goto out;
    }
    sceKernelClearEventFlag(g_videoState.eventflag, 0xFFFFFFF7);
    sceKernelSetEventFlag(g_videoState.eventflag, 1);
    g_videoState.mode = 1;
out:
    sceKernelCpuResumeIntr(intr);
    return ret;
}


/* 0x34B8 stopVideoStream */
s32 stopVideoStream(void)
{
    u32 bits;
    u32 v;
    s32 intr;
    int mode;

    if (g_videoState.started == 0)
        return SCE_ERROR_USBCAM_NOT_INIT;
    if (g_videoState.attached == 0)
        return SCE_ERROR_USBCAM_NOT_ATTACHED;
    if (sceUsbAccGetAuthStat() < 0)
        return SCE_ERROR_USBCAM_NOT_ATTACHED;
    if ((g_videoState.setupFlags & 1) == 0)
        return SCE_ERROR_USBCAM_NOT_SETUP;
    mode = g_videoState.mode;
    if (mode == 0 || mode == 2)
        return 0;

    intr = sceKernelCpuSuspendIntr();
    if (g_videoState.mode != 3) {
        v = g_videoState.flags & ~4u;
        g_videoState.mode = 2;
        g_videoState.flags = v;
        sceKernelSetEventFlag(g_videoState.eventflag, 4);
    } else {
        g_videoState.flags = g_videoState.flags & ~4u;
    }
    sceKernelCpuResumeIntr(intr);
    sceKernelWaitEventFlag(g_videoState.eventflag, 0x408, 1, &bits, NULL);
    sceKernelWaitEventFlag(g_videoState.eventflag, 0x480, 1, &bits, NULL);
    memset((void *)g_videoState.workBase, 0, g_videoState.workHalf * 2);
    return 0;
}


/* 0x35F4 armStillRead */
s32 armStillRead(void *buf, int size)
{
    s32 intr;
    s32 ret;

    intr = sceKernelCpuSuspendIntr();
    ret = 0;
    if (g_videoState.mode != 0) {
        ret = SCE_ERROR_USBCAM_BUSY;
        goto out;
    }
    if ((g_videoState.flags & 8) != 0) {
        ret = SCE_ERROR_USBCAM_BUSY;
        goto out;
    }
    g_videoState.stillAvail = size;
    g_videoState.stillBuf = (u32)buf;
    g_videoState.stillSize = size;
    if (sceUsbAccGetAuthStat() != 0) {
        ret = SCE_ERROR_USBCAM_NOT_ATTACHED;
        goto out;
    }
    g_videoState.flags = g_videoState.flags | 8;
    sceKernelSetEventFlag(g_videoState.eventflag, 0x800);
out:
    sceKernelCpuResumeIntr(intr);
    return ret;
}



/* 0x36B4 videoWorkerThread */
s32 videoWorkerThread(SceSize args __attribute__((unused)), void *argp __attribute__((unused)))
{
    u8 blk[32];
    u32 bits;
    u32 mask;
    u16 half;
    void *pkt;
    s32 res;
    s32 intr;

loop:
    res = sceKernelWaitEventFlag(g_videoState.eventflag, 0x4200, 1, &bits, NULL);
    if (res < 0)
        goto out;
    if (bits & 0x4000)
        goto out;
    if (queryReverseState(&pkt) < 0)
        goto loop;
    g_videoState.flags |= 0x2000;
    if (g_videoState.resEx == 0) {
        res = dispatchIoctl(0xC0000003, (int *)&g_videoState.resEx);
        if (res < 0)
            goto loop;
    }

    res = sceKernelWaitEventFlag(g_videoState.eventflag, 0x4C01, 1, &bits, NULL);
    if (res < 0)
        goto out;
    if (bits & 0x4000)
        goto out;
    if (bits & 0x400)
        goto loop;

    if (bits & 0x800) {
        if (g_videoState.attached == 0)
            goto loop;
        if (sceUsbAccGetAuthStat() != 0)
            goto waitFrame;
        if (g_videoState.resEx >= 1280) {
            if (g_videoState.cfgTail.b[6] < 2)
                g_videoState.cfgTail.b[6] = 2;
            if (g_videoState.cfgTail.b[7] < 2)
                g_videoState.cfgTail.b[7] = 2;
        }
        if (sceKernelLockMutex(g_videoState.mutex, 1, NULL) == 0) {
            sendAccCommand(3, 2, &g_videoState.cfgTail.b[0], 20);
            sceKernelUnlockMutex(g_videoState.mutex, 1);
        }
        if (g_videoState.resEx < 1280) {
            sceKernelClearEventFlag(g_videoState.eventflag, ~0x800u);
            goto loop;
        }
        goto waitFrame;
    }

    if ((bits & 1) == 0)
        goto waitFrame;
    intr = sceKernelCpuSuspendIntr();
    if (g_videoState.attached == 0) {
        res = -1;
    } else if (sceUsbAccGetAuthStat() != 0) {
        Kprintf("%serror - Not accessory !!", "");
    } else {
        __builtin_memcpy(blk, &g_videoState.cfg.width, 32);
        blk[6] = encodeSaturation(blk[6]);
        blk[7] = encodeBrightness(blk[7]);
        blk[9] = encodeSharpness(blk[9]);
        if (g_videoState.resEx >= 1280) {
            if (blk[0] < 2)
                blk[0] = 2;
            if (blk[1] < 2)
                blk[1] = 2;
        }
        sceKernelCpuResumeIntr(intr);
        res = sceKernelLockMutex(g_videoState.mutex, 1, NULL);
        if (res == 0) {
            sendAccCommand(1, 2, blk, 32);
            res = sceKernelUnlockMutex(g_videoState.mutex, 1);
        }
        intr = sceKernelCpuSuspendIntr();
    }
    sceKernelCpuResumeIntr(intr);
    if (res >= 0)
        goto waitFrame;
    goto loop;

waitFrame:
    mask = (g_videoState.resEx < 1280) ? 0x410 : 0x80410;
    res = sceKernelWaitEventFlag(g_videoState.eventflag, mask, 1, &bits, NULL);
    if (res >= 0 && (bits & 0x400) != 0)
        goto loop;
    if (g_videoState.mode == 2) {
        stopAndDrainStream((g_videoState.flags >> 4) & 1);
    } else {
        if ((bits & 0x80010) != 0 && (g_videoState.flags & 0x1000) != 0) {
            half = (g_videoState.flags & 0x400) ? 0x101 : 0x100;
            sendReverseFlags(&half);
            sceKernelClearEventFlag(g_videoState.eventflag, 0xFFFDFFFFu);
        }
        if (bits & 0x10)
            res = pumpVideoFrames(0);
        else if (bits & 0x80000)
            res = pumpVideoFrames(1);
    }
    if (res >= 0)
        goto loop;

out:
    return 0;
}


/* 0x3AA0 videoCopyWorker */
s32 videoCopyWorker(SceSize args __attribute__((unused)), void *argp __attribute__((unused)))
{
    u32 bits;
    u8 *userBuf;
    u8 *src;
    u32 userSize;
    u32 avail;
    u32 size;
    u32 chunk;
    u32 off;
    s32 status;
    s32 res;
    s32 intr;
    s32 idx;
    s32 peer;

loop:
    res = sceKernelWaitEventFlag(g_videoState.eventflag, 0x4040, 1, &bits, NULL);
    sceKernelClearEventFlag(g_videoState.eventflag, 0xFFFFFFBFu);
    if (res < 0 || (bits & 0x4000) != 0)
        sceKernelSetEventFlag(g_videoState.eventflag, 0x80);
    if (res < 0) {
        g_videoState.xferResult = (u32)res;
        goto loop;
    }
    if ((bits & 0x4000) != 0) {
        g_videoState.xferResult = 0;
        return 0;
    }

    peer = (s8)g_videoState.peerSlot;
    idx = (s8)g_videoState.activeSlot;
    userBuf = (u8 *)g_videoState.readBuf;
    userSize = g_videoState.readSize;
    if (peer == idx)
        goto wait2;
    if (g_videoState.slot[idx].len != 0)
        goto copy;

wait2:
    res = sceKernelWaitEventFlag(g_videoState.eventflag, 0x428, 1, &bits, NULL);
    if (res < 0) {
        g_videoState.xferResult = (u32)res;
        sceKernelSetEventFlag(g_videoState.eventflag, 0x80);
        goto loop;
    }
    if ((bits & 0x400) != 0) {
        g_videoState.xferResult = SCE_ERROR_USBCAM_NOT_ATTACHED;
        sceKernelSetEventFlag(g_videoState.eventflag, 0x80);
        goto loop;
    }
    if ((bits & 8) != 0) {
        g_videoState.xferResult = 0;
        sceKernelSetEventFlag(g_videoState.eventflag, 0x80);
        goto loop;
    }
    idx = (s8)g_videoState.activeSlot;

copy:
    off = 0;
    avail = g_videoState.slot[idx].len;
    size = (userSize < avail) ? userSize : avail;
    if (size != 0) {
        src = (u8 *)g_videoState.slot[idx].buf;
        do {
            chunk = (size < 16380) ? size : 16380;
            dmacCopy(userBuf + off, src + off, (int)chunk);
            size -= chunk;
            off += chunk;
        } while (size != 0);
    }

    intr = sceKernelCpuSuspendIntr();
    idx = (s8)g_videoState.activeSlot;
    avail = g_videoState.slot[idx].len;
    status = (userSize < avail) ? (s32)SCE_ERROR_USBCAM_BUF_SMALL : (s32)avail;
    g_videoState.xferResult = (u32)status;
    idx = (s8)g_videoState.activeSlot;
    g_videoState.slot[idx].len = 0;
    sceKernelClearEventFlag(g_videoState.eventflag, 0xFFFFFFDFu);
    /* 0x3C30 reloads g_videoState.xferResult into $a1 and max()es it with 0;
       interrupts are suspended across the store, so the local is the
       same value. Signed clamp: sceUsbCamGetReadVideoFrameSize reads
       this word back as a byte count (verified against the disassembly). */
    g_videoState.xferLength = (status >= 0) ? (u32)status : 0u;
    sceKernelSetEventFlag(g_videoState.eventflag, 0x80);
    sceKernelCpuResumeIntr(intr);
    goto loop;
}


/* 0x3CD4 micCopyWorker */
s32 micCopyWorker(SceSize args __attribute__((unused)), void *argp __attribute__((unused)))
{
    u32 bits;
    u8 *dst;
    u32 pending;
    u32 limit;
    s32 copied;
    s32 status;
    s32 res;
    s32 intr;
    s32 n;

    g_micState.status = 0;

loop:
    res = sceKernelWaitEventFlag(g_micState.eventflag, 0x204, 1, &bits, NULL);
    if (res < 0)
        return 0;
    if (bits & 0x200)
        return 0;

    dst = (u8 *)g_micState.readBuf;
    pending = g_micState.readSize;
    res = sceKernelWaitEventFlag(g_micState.eventflag, 0x110, 1, &bits, NULL);
    copied = 0;
    status = res;
    if (res < 0)
        goto done;
    if (bits & 0x100) {
        status = SCE_ERROR_USBCAM_NOT_ATTACHED;
        goto done;
    }
    if (dst == NULL)
        goto done;

    limit = (g_micState.rateFlag != 0) ? 132u : 0u;
    if (limit >= pending)
        goto done;

inner:
    res = sceKernelWaitEventFlag(g_micState.eventflag, 0x121, 1, &bits, NULL);
    status = res;
    if (res < 0)
        goto done;
    if (bits & 0x100) {
        status = SCE_ERROR_USBCAM_NOT_ATTACHED;
        goto done;
    }
    if (bits & 1) {
        n = drainMicBuffer(dst + copied, (int)pending);
        if (n < 0) {
            status = n;
            goto done;
        }
        copied += n;
        pending -= n;
        status = copied;
    }
    if (bits & 0x20)
        goto done;
    if (limit < pending)
        goto inner;

done:
    /* 0x3DEC: the status store sits in the SuspendIntr delay slot, i.e.
       it runs before the callee and still with interrupts enabled. */
    g_micState.status = (u32)status;
    intr = sceKernelCpuSuspendIntr();
    if (g_micState.mode == 4)
        g_micState.mode = 1;
    sceKernelCpuResumeIntr(intr);
    g_micState.readBuf = 0;
    g_micState.readSize = 0;
    sceKernelClearEventFlag(g_micState.eventflag, 0xFFFFFFFBu);
    sceKernelSetEventFlag(g_micState.eventflag, 2);
    goto loop;
}


/* 0x3E94 videoEp0Complete */
void videoEp0Complete(struct UsbdDeviceReq *req)
{
    u8 *ptr;
    u32 v;
    s16 half;
    s32 arg;
    s32 cbid;
    int size;

    if (req->retcode != 0)
        return;
    if ((s8)g_videoState.setup.bmRequestType < 0)
        return;

    if (g_videoState.setup.bRequest == 9) {
        ptr = (u8 *)g_videoState.cmdBuf;
        if (ptr[2] != ((u8 *)&g_videoState.setup.wValue)[0]) {
            g_videoState.replyMismatch = 0xFFFFFFFF;
        } else {
            g_videoState.replyMismatch = 0;
            ptr[3] = (u8)req->recvsize;
            memset(ptr + 4, 0, 60);
            size = ((u32)req->recvsize < 61u) ? req->recvsize : 60;
            memcpy(ptr + 4, req->data, size);
        }
        sceKernelSetEventFlag(g_videoState.eventflag, 0x10000);
        return;
    }

    if (g_videoState.setup.bRequest != 10)
        return;
    if (g_videoState.setup.wValue != 16)
        return;

    /* 0x3F08-0x3F18: the first two data bytes are pushed on the stack and
       read back with `lh` - a little-endian sign-extended halfword. */
    half = (s16)((u16)((u8 *)req->data)[0] | ((u16)((u8 *)req->data)[1] << 8));
    v = g_videoState.flags;
    if (half == 1) {
        v &= ~0x400u;
        arg = 1;
    } else {
        v |= 0x400u;
        arg = 0;
    }
    cbid = (s32)g_videoState.lensCbid;
    g_videoState.flags = v;
    if (cbid > 0)
        sceKernelNotifyCallback(cbid, arg);
    if (g_videoState.flags & 0x1000)
        sceKernelSetEventFlag(g_videoState.eventflag, 0x20000);
}


/* 0x3FEC videoBulkComplete */
void videoBulkComplete(struct UsbdDeviceReq *req)
{
    u32 avail;
    u32 left;
    u32 space;
    u32 n;
    u32 done;
    u32 total;

    if ((u32)g_videoState.resEx >= 1280u)
        return;
    if (req->retcode < 0)
        return;

    done = 0;

    if (g_videoState.fragHdr == 0) {
        /* No threshold armed: a 6-byte request stages a new one. */
        if (req->recvsize == 6)
            __builtin_memcpy(&g_videoState.fragHdr, req->data, 4);
        goto rearm;
    }

    avail = g_videoState.stillAvail;
    if (avail == 0)
        goto finish;
    if (g_videoState.stillBuf == 0)
        goto finish;
    left = (u32)req->recvsize;
    if (left == 0)
        goto finish;

    for (;;) {
        avail = g_videoState.stillAvail;
        /* The clamp compares against req->recvsize itself, not against the
           remaining count: `sltu $t7, $v1, $s0` + `movn $s0, $v1, $t7`
           with `$v1` reloaded from 20($s1) at the top of every iteration
           (0x4110, the jump delay slot). */
        n = ((u32)req->recvsize < avail) ? (u32)req->recvsize : avail;
        memcpy((u8 *)g_videoState.stillBuf, (u8 *)req->data + done, n);
        space = g_videoState.stillBuf;
        avail = g_videoState.stillAvail;
        left -= n;
        g_videoState.stillAvail = avail - n;
        done += n;
        /* 0x4108 is the delay slot of the `beqz $s2` exit test, so the
           store runs whether or not the loop is leaving. */
        g_videoState.stillBuf = space + n;
        if (left == 0)
            break;
    }

finish:
    total = g_videoState.fragTotal + (u32)req->recvsize;
    g_videoState.fragTotal = total;
    if (done != 0) {
        if ((done & 0x3F) != 0)
            goto signal;
        if (g_videoState.fragHdr >= total)
            goto rearm;
    }

signal:
    g_videoState.stillBuf = 0;
    sceKernelSetEventFlag(g_videoState.eventflag, 0x1000);
    return;

rearm:
    req->size = 64;
    sceKernelDcacheInvalidateRange(req->data, 128);
    sceUsbbdReqRecv(req);
}

/* 0x4164 startIsoReceives */
s32 startIsoReceives(void)
{
    struct VideoState *st = (struct VideoState *)&g_videoState;
    struct VideoDescState *node;
    s32 res;
    int i;

    g_videoState.frameSeq = 0;
    g_videoState.flags = (g_videoState.flags | 1u) & ~2u;

    for (i = 0; i < 2; i++) {
        st->desc[i].len = 0;
        st->desc[i].cap = 1;
    }

    /* 0x41C8 / 0x41D4: the same node pointer lands in both slots (start
       after current). */
    node = &st->desc[0];
    g_videoState.xferTail = node;
    g_videoState.xferHead = node;

    for (i = 0; i < 2; i++)
        st->slot[i].len = 0;

    g_videoState.activeSlot = 1;
    g_videoState.peerSlot = 0;

    for (i = 0; i < 2; i++) {
        memset(st->frameBuf[i], 0, 1792);
        sceKernelDcacheInvalidateRange(st->frameBuf[i], 1792);
        st->isoReq[i].unk1c = (int)&st->isoReq[i + 1];
    }
    /* 0x4258 sits in the `jal sceUsbbdReqRecv` delay slot, so it runs
       before the call - the same delay-slot pattern the mic path uses. */
    st->isoReq[1].unk1c = 0;

    res = sceUsbbdReqRecv(&st->isoReq[0]);
    if (res < 0 && st->isoReq[0].retcode < 0)
        g_videoState.aux = 1;
    return res;
}


/*
 * sceUsbBus_driver import NID 0xCC57EC9D (psplibdoc_usb.csv:21 names it
 * sceUsbbdReqCancel). No uofw header declares it; the prototype is the
 * sibling of sceUsbbdReqSend (include/usbbus.h:187) and is guessed.
 */
int sceUsbbdReqCancel(struct UsbdDeviceReq *req);

/* Also declared in include/interruptman.h:150-151; repeated here
   because this file does not include that header. */
s32 sceKernelCpuSuspendIntr(void);
void sceKernelCpuResumeIntr(s32 intr);

/* Both are implemented in src/kd/dmacman/dmacman.c but neither is listed
   by include/dmacman.h, so they are re-stated here. */
s32 sceKernelDmaOpSetupMemcpy(sceKernelDmaOperation *op, s32 arg1, s32 arg2, s32 arg3);
s32 sceKernelDmaOpSync(sceKernelDmaOperation *op, s32 command, u32 *timeout);

/* include/lowio_ddr.h:7, include/sysmem_suspend_kernel.h:11/13 and
   include/sysmem_kernel.h:299 verbatim; none of those headers are
   included by this file. */
int sceDdrFlush(int);
s32 sceKernelPowerLock(s32 lockType);
s32 sceKernelPowerUnlock(s32 lockType);
void *sceKernelMemcpy(void *dst, const void *src, u32 n);

/* Helpers and dispatch handlers defined below; the prototypes are
   restated here for the callers above. Every prototype below matches
   its definition. */
s32 execRawCommand(int *arg);

int setResolutionPair(int *arg);
int setSaturation(int *arg);
int getSaturation(int *arg);
int setBrightness(int *arg);
int getBrightness(int *arg);
int setContrast(int *arg);
int getContrast(int *arg);
int setSharpness(int *arg);
int getSharpness(int *arg);
int setZoom(int *arg);
int getZoom(int *arg);
int setReverseMode(int *arg);
int getReverseMode(int *arg);
int setImageEffect(int *arg);
int getImageEffect(int *arg);
int setResolution(int *arg);
int getResolution(int *arg);
int setUnk0xB(int *arg);
int getUnk0xB(int *arg);
int setFramerate(int *arg);
int getFramerate(int *arg);
int setUnk0xE(int *arg);
int getUnk0xE(int *arg);
int setUnk0xF(int *arg);
int getUnk0xF(int *arg);
int setAntiFlicker(int *arg);
int getAntiFlicker(int *arg);
int setUnk0x11(int *arg);
int getUnk0x11(int *arg);
int setUnk0x12(int *arg);
int getUnk0x12(int *arg);
int setUnk0x13(int *arg);
int getUnk0x13(int *arg);
int setEvLevel(int *arg);
int getEvLevel(int *arg);
int setUnk40000001(int *arg);
int getUnk40000001(int *arg);
int setUnk40000002(int *arg);
int getUnk40000002(int *arg);
int getUnk40000003(int *arg);
int setUnk0xA(int *arg);
int getUnk0xA(int *arg);

/* 60-byte block sceUsbCamSetupStill (0x1CAC) and sceUsbCamSetupStillEx
   (0x1EBC) build on the stack and pass to packStillConfig. Field +0 and
   +5 are never read; +44 and +48 are read back as u16. */
struct UsbCamStillReq {
    int unk0;
    int resolution;
    int framesize;
    int jpegsize;
    int complevel;
    int unk5;
    int unk6;
    int reverseVert;
    int reverseHoriz;
    int unk9;
    int unk10;
    int unk11;
    int unk12;
    int unk13;
    int unk14;
};

_Static_assert(sizeof(struct UsbCamStillReq) == 60, "UsbCamStillReq size");

/* 0x8E00, 0x8E04, 0x8E08, 0x8E10, 0x8E18, 0x8E20 and 0x8E24: separate
   one-to-four/six/seven byte index maps read by packVideoConfig and
   packStillConfig. The surrounding objects (0x8DEC, 0x8E0C, 0x8DF8,
   0x8E28) are already defined above. */
static const u8 s_map8E00[4] = { 1, 2, 3, 0 };
static const u8 s_map8E04[4] = { 0, 1, 2, 3 };
static const u8 s_map8E08[4] = { 0, 1, 2, 0 };
static const u8 s_map8E10[7] = { 0, 1, 2, 3, 4, 5, 6 };
static const u8 s_map8E18[4] = { 0, 1, 2, 0 };
static const u8 s_map8E20[4] = { 0, 1, 2, 3 };
static const u8 s_map8E24[4] = { 1, 2, 3, 0 };

/* 0x8E50: ten {width, height} pairs indexed by the 0..9 resolution
   codes packVideoConfig range-checks the caller's frame size against. */
static const struct UsbCamResEntry s_res8E50[10] = {
    { 160, 120 }, { 176, 144 }, { 320, 240 }, { 352, 288 }, { 360, 272 },
    { 480, 272 }, { 640, 480 }, { 1024, 768 }, { 1280, 960 }, { 1280, 1024 }
};

/* 0x4298: pack a 100-byte PspUsbCamSetupVideoExParam into the 32-byte
   command block sceUsbCamSetupVideoEx writes into g_videoState. */

int packVideoConfig(u8 *out, struct UsbCamVideoReq *req)
{
    const struct UsbCamResEntry *cur;
    const struct UsbCamResEntry *sel;
    s32 diffW;
    s32 diffH;
    u32 v;
    int ret;

    ret = SCE_ERROR_USBCAM_INVALID_RES;
    if ((u32)req->unk >= 10)
        goto out;
    if ((s32)req->unk < (s32)req->resolution)
        goto out;
    if ((u32)req->framerate >= 8)
        goto out;
    if ((u32)req->unk2 >= 3)
        goto out;
    if ((u32)req->unk3 >= 4)
        goto out;
    if ((u32)req->wb >= 4)
        goto out;
    ret = SCE_ERROR_USBCAM_INVALID_VALUE;
    if ((u32)req->saturation >= 256)
        goto out;
    if ((u32)req->brightness >= 256)
        goto out;
    if ((u32)req->contrast >= 256)
        goto out;
    if ((u32)req->sharpness >= 256)
        goto out;
    ret = SCE_ERROR_USBCAM_INVALID_RES;
    if ((u32)req->unk4 >= 3)
        goto out;

    sel = &s_res8E50[req->unk];
    cur = &s_res8E50[req->resolution];
    diffW = (s32)sel->w - (s32)cur->w;
    diffH = (s32)sel->h - (s32)cur->h;
    if (diffW < 0 || diffH < 0)
        goto out;
    if (diffW < (s32)req->unk6[1])
        goto out;
    if (diffH < (s32)req->unk6[2])
        goto out;
    if ((u32)req->effectmode >= 7)
        goto out;
    if (checkEvAllowed(req->unk, req->resolution, req->unk8) == 0)
        goto out;
    if ((s32)req->framerate >= 5 && (u32)(req->unk - 7) < 3)
        goto out;
    ret = SCE_ERROR_USBCAM_INVALID_PARAM;
    if ((s32)req->unk9 >= 3)
        goto out;
    if ((u32)req->framesize - 1 > 0x87FF)
        goto out;
    if ((u32)req->unk12 >= 3)
        goto out;

    out[0] = s_widthIdxMap[req->unk];
    out[1] = s_heightIdxMap[req->resolution];
    out[2] = s_map8DF8[req->framerate];
    out[3] = s_map8E08[req->unk2];
    out[4] = s_map8E00[req->unk3];
    out[5] = s_map8E04[req->wb];
    out[6] = (u8)req->saturation;
    out[7] = (u8)req->brightness;
    out[8] = (u8)req->contrast;
    out[9] = (u8)req->sharpness;
    out[10] = s_map8E0C[req->unk4];
    out[11] = (req->unk5 != 0);
    v = (u32)req->unk6[0];
    if (v > 0xFFFF)
        v = 0xFFFF;
    out[12] = (u8)v;
    out[13] = (u8)(v >> 8);
    out[14] = (u8)req->unk6[1];
    out[15] = (u8)((u32)req->unk6[1] >> 8);
    out[16] = (u8)req->unk6[2];
    out[17] = (u8)((u32)req->unk6[2] >> 8);
    out[18] = s_map8E10[req->effectmode];
    out[19] = (req->unk7 != 0);
    out[20] = (u8)req->unk8;
    out[21] = s_map8E18[req->unk9];
    out[22] = (u8)req->unk10;
    out[23] = (u8)((u32)req->unk10 >> 8);
    out[24] = (u8)req->unk11;
    out[25] = (u8)((u32)req->unk11 >> 8);
    out[26] = (u8)req->framesize;
    out[27] = (u8)((u32)req->framesize >> 8);
    out[28] = (u8)((u32)req->framesize >> 16);
    out[29] = (u8)((u32)req->framesize >> 24);
    out[30] = s_map8E24[req->unk12];
    out[31] = s_map8E28[req->evlevel];
    ret = 0;
out:
    return ret;
}

/* 0x4680: pack a 60-byte still-setup block into the 19-byte command
   block sceUsbCamSetupStill writes into g_videoState. */

int packStillConfig(u8 *out, struct UsbCamStillReq *req)
{
    int ret;

    ret = SCE_ERROR_USBCAM_INVALID_PARAM;
    if ((u32)req->jpegsize > 0x80000)
        goto out;
    ret = SCE_ERROR_USBCAM_INVALID_RES;
    if ((s32)req->resolution >= 10)
        goto out;
    if ((s32)req->framesize >= 10)
        goto out;
    if ((s32)req->resolution < (s32)req->framesize)
        goto out;
    if ((s32)req->unk6 >= 3)
        goto out;
    ret = SCE_ERROR_USBCAM_INVALID_PARAM;
    if ((u32)req->complevel - 1 >= 63)
        goto out;
    ret = SCE_ERROR_USBCAM_INVALID_RES;
    if ((u32)req->unk9 >= 4)
        goto out;
    if ((u32)req->unk13 >= 7)
        goto out;
    ret = SCE_ERROR_USBCAM_INVALID_PARAM;
    if ((u32)req->unk14 >= 3)
        goto out;

    out[0] = (u8)req->jpegsize;
    out[1] = (u8)((u32)req->jpegsize >> 8);
    out[2] = (u8)((u32)req->jpegsize >> 16);
    out[3] = (u8)((u32)req->jpegsize >> 24);
    out[4] = (u8)req->complevel;
    out[5] = 1;
    out[6] = s_widthIdxMap[req->resolution];
    out[7] = s_heightIdxMap[req->framesize];
    out[8] = s_map8E08[req->unk6];
    out[9] = (req->reverseVert != 0);
    out[10] = (req->reverseHoriz != 0);
    out[11] = s_map8E20[req->unk9];
    out[12] = s_map8E18[req->unk10];
    out[13] = (u8)req->unk11;
    out[14] = (u8)((u32)req->unk11 >> 8);
    out[15] = (u8)req->unk12;
    out[16] = (u8)((u32)req->unk12 >> 8);
    out[17] = s_map8E10[req->unk13];
    out[18] = s_map8E24[req->unk14];
    ret = 0;
out:
    return ret;
}

/* 0x4858: state/authentication guard shared by the still-input entry
   points (sceUsbCamStillPollInputEnd, sceUsbCamStillWaitInputEnd,
   sceUsbCamGetReadFrameSize). */

s32 guardStillInput(void)
{
    int ret;

    ret = SCE_ERROR_USBCAM_NOT_INIT;
    if (g_videoState.started == 0)
        goto out;
    ret = SCE_ERROR_USBCAM_NOT_ATTACHED;
    if (g_videoState.attached == 0)
        goto out;
    if (sceUsbAccGetAuthStat() < 0)
        goto out;
    ret = SCE_ERROR_USBCAM_NOT_SETUP;
    if ((g_videoState.setupFlags & 2) == 0)
        goto out;
    ret = SCE_ERROR_USBCAM_INVALID_STATE;
    if ((g_videoState.flags & 8) != 0)
        ret = 0;
out:
    return ret;
}

/* 0x48DC: reap the still frame. arg == 0 waits on the video event flag,
   arg != 0 polls it; both then drain the pending request and report the
   byte count that stopAndDrainStream's caller consumed. */

s32 reapStillInput(int arg)
{
    struct VideoState *st;
    u32 bits;
    s32 res;
    s32 ret;

    st = (struct VideoState *)&g_videoState;
    if (arg == 0) {
        res = sceKernelWaitEventFlag(st->eventflag, 0x3400, 1, &bits, NULL);
        if (res < 0)
            return res;
    } else {
        res = sceKernelPollEventFlag(st->eventflag, 0x3400, 1, &bits);
        if (res == (s32)0x800201AF)
            return SCE_ERROR_USBCAM_NOT_READY;
        if (res < 0) {
            g_videoState.flags = g_videoState.flags & ~8u;
            return res;
        }
    }

    if ((bits & 0x2400) != 0) {
        ret = (bits & 0x400) ? SCE_ERROR_USBCAM_NOT_ATTACHED : 0;
        if (g_videoState.resEx < 1280)
            sceUsbbdReqCancel(&st->bulkReq);
    } else if (g_videoState.stillSize < g_videoState.fragTotal) {
        ret = SCE_ERROR_USBCAM_BUF_SMALL;
    } else {
        ret = (g_videoState.stillAvail != 0) ? g_videoState.fragTotal : g_videoState.stillSize;
    }

    if (g_videoState.resEx >= 1280) {
        g_videoState.flags = g_videoState.flags | 0x10;
        sceKernelSetEventFlag(st->eventflag, 0x100000);
    }
    g_videoState.flags = g_videoState.flags & ~8u;
    return ret;
}

/* 0x4A24: memcpy wrapper used by the still-capture drain loop
   (0x3B94-0x3BD8). Falls back to sceKernelMemcpy for addresses outside
   the DMA mask 0x00220202 or when no DMA op has been allocated;
   otherwise it DMAs in <= 16380-byte word-aligned chunks, copies the
   0..3 byte tail in software and returns dst. */

void *dmacCopy(void *dst, const void *src, int size)
{
    sceKernelDmaOperation *op;
    u32 dmacDst;
    u32 dmacSrc;
    u32 aligned;
    int res;

    if (((0x00220202u >> (((u32)dst >> 27) & 0x1F)) & 1) == 0)
        return sceKernelMemcpy(dst, src, (u32)size);
    op = (sceKernelDmaOperation *)g_videoState.dmaOp;
    if (op == NULL)
        return sceKernelMemcpy(dst, src, (u32)size);

    res = sceKernelDmaOpAssign(op, 255, 255, 0, 0);
    if (res < 0)
        Kprintf("%serror - Fail to assign to DMAC. 0x%08x\n", "", res);

    dmacDst = (((u32)dst >> 31) ? 0 : 0x40000000) | ((u32)dst & 0x1FFFFFFF);
    dmacSrc = (((u32)src >> 31) ? 0 : 0x40000000) | ((u32)src & 0x1FFFFFFF);
    aligned = (u32)size & ~3u;
    res = sceKernelDmaOpSetupMemcpy(op, dmacDst, dmacSrc, (u32)size >> 2);
    if (res < 0)
        Kprintf("%serror - Fail to setup DMA 0x%08x\n", "", res);

    sceKernelDcacheInvalidateRange(dst, aligned);
    sceKernelDcacheWritebackInvalidateRange(src, aligned);
    sceKernelPowerLock(0);
    res = sceKernelDmaOpEnQueue(op);
    if (res < 0) {
        memcpy(dst, src, (u32)size);
        goto unlock;
    }
    if ((u32)size & 3)
        memcpy((u8 *)dst + aligned, (const u8 *)src + aligned, (u32)size & 3);
    res = sceKernelDmaOpSync(op, 1, NULL);
    sceDdrFlush(1);
    if (res < 0)
        Kprintf("%serror - Fail to transrate by DMA. 0x%08x\n", "", res);
unlock:
    sceKernelPowerUnlock(0);
    return dst;
}

/* 0x4C00: stop/drain the video stream. arg is the
   (g_videoState[8] >> 4) & 1 flag from the still-capture worker; the
   function tears the event-flag/mutex protocol down, re-arms the two
   frame descriptors and either re-enters the wait loop or returns the
   last threadman status. */

s32 stopAndDrainStream(int arg)
{
    s32 intr;
    s32 ret;
    int done;
    int mode;
    u32 bits;
    int i;

    intr = sceKernelCpuSuspendIntr();
    ret = 0;
    done = 0;
    if (g_videoState.attached == 0) {
        done = 1;
        goto resume;
    }
    if (sceUsbAccGetAuthStat() != 0) {
        sceKernelClearEventFlag(g_videoState.eventflag, (u32)-5);
        g_videoState.desc[0].len = 0;
        g_videoState.desc[0].cap = 1;
        g_videoState.desc[1].len = 0;
        g_videoState.desc[1].cap = 1;
        if (g_videoState.mode == 2) {
            sceKernelClearEventFlag(g_videoState.eventflag, (u32)-2050);
            g_videoState.mode = 0;
            sceKernelSetEventFlag(g_videoState.eventflag, 8);
        } else {
            g_videoState.mode = 0;
        }
        done = 1;
        goto resume;
    }

    sceKernelCpuResumeIntr(intr);
    ret = sceKernelWaitEventFlag(g_videoState.eventflag, 0x402, 1, &bits, NULL);
    if (ret < 0)
        return ret;
    if (arg != 0) {
        ret = sceKernelWaitEventFlag(g_videoState.eventflag, 0x100400, 1, &bits, NULL);
        if (ret < 0)
            goto suspendCheck;
        sceKernelClearEventFlag(g_videoState.eventflag, 0xFFEFFFFFu);
    }
    if ((bits & 0x400) != 0) {
        done = 1;
        goto suspendCheck;
    }
    ret = sceKernelLockMutex(g_videoState.mutex, 1, NULL);
    if (ret != 0)
        goto suspendCheck;
    ret = sendAccCommand((arg != 0) ? 4 : 2, 2, NULL, 0);
    sceKernelUnlockMutex(g_videoState.mutex, 1);
    goto suspendCheck;

suspendCheck:
    intr = sceKernelCpuSuspendIntr();
    if (ret >= 0)
        goto resume;
    sceKernelUnlockMutex(g_videoState.mutex, 1);
    done = 1;
    goto resume;

resume:
    sceKernelCpuResumeIntr(intr);
    if (done)
        return ret;

    ret = sceKernelWaitEventFlag(g_videoState.eventflag, 0x500, 1, &bits, NULL);
    if (ret < 0)
        return ret;
    if ((bits & 0x400) != 0)
        return 0;
    intr = sceKernelCpuSuspendIntr();
    mode = g_videoState.mode;
    if (mode == 2) {
        sceKernelClearEventFlag(g_videoState.eventflag, (u32)-2050);
        g_videoState.mode = 0;
        sceKernelSetEventFlag(g_videoState.eventflag, 8);
        for (i = 0; i < 2; i++)
            g_videoState.slot[i].len = 0;
    } else if (mode == 3) {
        sceKernelClearEventFlag(g_videoState.eventflag, (u32)-2050);
        sceKernelSetEventFlag(g_videoState.eventflag, 8);
        for (i = 0; i < 2; i++)
            g_videoState.slot[i].len = 0;
        g_videoState.xferResult = SCE_ERROR_USBCAM_BUF_SMALL;
        g_videoState.mode = 0;
    } else {
        g_videoState.mode = 0;
    }
    sceKernelCpuResumeIntr(intr);
    sceKernelWaitEventFlag(g_videoState.eventflag, 0x480, 1, &bits, NULL);
    return ret;
}

/* Transfer node of the list rooted at g_videoState.xferTail (g_videoState +
   0x12C): +0 payload pointer, +4 payload length, +8 "busy" flag (1 when
   the node has been consumed), +12 next. Nodes are produced by the USB
   receive path outside this window. */
struct UsbCamXfer {
    u8 *buf;
    int len;
    int flag;
    struct UsbCamXfer *next;
};

/* 0x4F04: frame pump. Sleeps on the video event flag, walks the node list
   at g_videoState.xferTail and merges each node's payload into the active half
   buffer ({st->slot[seb(st->peerSlot)].buf,
   st->slot[seb(st->peerSlot)].len}) while g_videoState.resEx < 1280,
   or into the JPEG pair (g_videoState.stillBuf/g_videoState.fragTotal, reached through
   the slot[i] pair) otherwise. arg == 0 is the half-buffer path,
   arg != 0 the big path. Every exit runs stopAndDrainStream(arg) and returns the
   last sceKernelWaitEventFlag status. */

s32 pumpVideoFrames(int arg)
{
    struct UsbCamXfer *node;
    struct UsbCamXfer *next;
    struct UsbCamXfer *nxt2;
    u32 bits;
    u32 cnt12;
    u32 cnt16;
    u32 cnt20;
    u32 mark;
    u32 poff;
    u32 slot;
    u32 slotLen;
    u32 plen;
    u32 need;
    u32 cap;
    u32 copy;
    u32 limit;
    u32 clamp;
    u32 diff;
    u32 hdr;
    u16 half;
    s32 res;
    s32 intr;
    u8 *buf;
    int i;

    cnt12 = 0;
    cnt16 = 0;
    cnt20 = 0;

wait:
    res = sceKernelWaitEventFlag(g_videoState.eventflag, 0x404, 1, &bits, NULL);
    if (res < 0)
        goto out;
    if ((bits & 0x400) != 0)
        goto out;
    if ((bits & 0x20000) != 0) {
        half = (g_videoState.flags & 0x400) ? 0x101 : 0x100;
        sendReverseFlags(&half);
        sceKernelClearEventFlag(g_videoState.eventflag, 0xFFFDFFFFu);
    }
    if (g_videoState.aux == 1) {
        res = sceKernelWaitEventFlag(g_videoState.eventflag, 0x8400, 1, &bits, NULL);
        if (res < 0)
            goto out;
        if ((bits & 0x400) != 0)
            goto out;
    }
    if (((u32)g_videoState.mode - 2) < 2)
        goto out;

    intr = sceKernelCpuSuspendIntr();
    i = 0;
    node = (struct UsbCamXfer *)g_videoState.xferTail;
    if (node->flag == 0)
        goto found;
walk:
    i = i + 1;
    if ((u32)i >= 2)
        goto restart;
    node = node->next;
    if (node->flag == 0)
        goto found;
    goto walk;

restart:
    sceKernelCpuResumeIntr(intr);
    goto wait;

found:
    poff = 344 + (u32)((s32)(s8)g_videoState.peerSlot * 8);
    sceKernelClearEventFlag(g_videoState.eventflag, (u32)-5);
    if (sceUsbAccGetAuthStat() != 0)
        goto restart;
    i = 0;
frame:
    buf = node->buf;
    mark = (u32)buf[0] | ((u32)buf[1] << 8);
    if (mark == 0xD8FF)
        goto soi;

cont:
    if ((g_videoState.flags & 2) != 0) {
        next = node->next;
        goto nodeDone;
    }
    if (g_videoState.resEx >= 1280)
        goto big;

    cap = g_videoState.workHalf;
    plen = (*videoWordAt(poff + 4));
    need = (u32)node->len;
    copy = ((cap - plen) < need) ? (cap - plen) : need;
    sceKernelMemcpy((u8 *)(*videoWordAt(poff)) + plen, buf, copy);
    plen = plen + need;
    (*videoWordAt(poff + 4)) = plen;
    if (cap < plen) {
        g_videoState.flags = g_videoState.flags | 3;
        (*videoWordAt(poff + 4)) = 0;
        goto restart;
    }
    if (need >= 896) {
        next = node->next;
        goto nodeDone;
    }
    g_videoState.flags = g_videoState.flags | 1;
    if ((s32)(s8)g_videoState.peerSlot == (s32)(s8)g_videoState.activeSlot) {
        (*videoWordAt(poff + 4)) = 0;
        goto nodeTail;
    }
    if (g_videoState.frameSeq == (*videoWordAt(poff + 4)))
        goto other;
    (*videoWordAt(poff + 4)) = 0;

nodeTail:
    next = node->next;
nodeDone:
    node->flag = 1;
    g_videoState.xferTail = next;
    nxt2 = node->next;
    node->len = 0;
    i = i + 1;
    if (nxt2->flag != 0)
        goto restart;
    if ((u32)i >= 2)
        goto restart;
    node = nxt2;
    goto frame;

other:
    if (g_videoState.slot[g_videoState.activeSlot].len != 0) {
        (*videoWordAt(poff + 4)) = 0;
        goto nodeTail;
    }
    g_videoState.activeSlot = g_videoState.activeSlot ^ 1;
    g_videoState.peerSlot = g_videoState.peerSlot ^ 1;
    sceKernelSetEventFlag(g_videoState.eventflag, 32);
    goto nodeTail;

big:
    limit = (arg == 0) ? g_videoState.workHalf : g_videoState.stillSize;
    plen = (*videoWordAt(poff + 4));
    need = (u32)node->len;
    if (limit < (plen + need)) {
        g_videoState.flags = g_videoState.flags | 2;
        goto restart;
    }
    mark = (u32)buf[node->len - 2] | ((u32)buf[node->len - 1] << 8);
    if (mark != 0xD9FF)
        goto common;
    mark = (u32)buf[0] | ((u32)buf[1] << 8);
    if (mark != 0xFEFF || node->len != 16)
        goto common;
    if (arg == 0)
        goto head0;

    __builtin_memcpy(&hdr, buf + 4, 4);
    g_videoState.fragHdr = hdr;
    if (g_videoState.fragHdr == g_videoState.fragTotal)
        goto seg;
    g_videoState.fragTotal = 0;
    g_videoState.fragHdr = 0;
    goto nodeTail;

seg:
    clamp = g_videoState.cfgTail.w[0];
    if (clamp > 0xFC00)
        clamp = 0xFC00;
    diff = (clamp >= g_videoState.fragHdr) ? (clamp - g_videoState.fragHdr)
                                      : (g_videoState.fragHdr - clamp);
    if ((cnt12 + 1) == (u32)buf[9] && g_videoState.fragHdr < (clamp + 1024))
        cnt16 = 1;
    if ((u32)buf[9] == cnt12)
        cnt20 = (cnt20 + 1) & 0xFF;
    else
        cnt20 = 0;
    if (diff < 1024 || cnt16 != 0)
        goto mode2;
    if (cnt20 < 3) {
        cnt12 = (u32)buf[9];
        g_videoState.fragTotal = 0;
        goto nodeTail;
    }
mode2:
    g_videoState.stillBuf = 0;
    sceKernelSetEventFlag(g_videoState.eventflag, 0x1004);
    g_videoState.mode = 2;
    goto restart;

head0:
    __builtin_memcpy(&hdr, buf + 4, 4);
    g_videoState.frameSeq = hdr;
    g_videoState.frameSeq = g_videoState.frameSeq + 12;
    if (g_videoState.frameSeq != (*videoWordAt(poff + 4))) {
        (*videoWordAt(poff + 4)) = 0;
        goto nodeTail;
    }
    if (g_videoState.slot[g_videoState.activeSlot].len != 0) {
        (*videoWordAt(poff + 4)) = 0;
        goto nodeTail;
    }
    hdr = g_videoState.frameSeq;
    __builtin_memcpy(buf + 4, &hdr, 4);
    __builtin_memcpy((u8 *)(*videoWordAt(poff)) + 2, buf, 12);
    g_videoState.activeSlot = g_videoState.peerSlot;
    g_videoState.peerSlot = g_videoState.peerSlot ^ 1;
    sceKernelSetEventFlag(g_videoState.eventflag, 32);
    goto nodeTail;

soi:
    if (g_videoState.resEx >= 1280) {
        (*videoWordAt(poff + 4)) = 0;
        g_videoState.flags = (g_videoState.flags & ~2u) | 1;
        goto cont;
    }
    mark = (u32)buf[2] | ((u32)buf[3] << 8);
    if (mark != 0xFEFF) {
        g_videoState.flags = g_videoState.flags | 2;
        goto cont;
    }
    if ((g_videoState.flags & 1) != 0)
        g_videoState.flags = g_videoState.flags & ~3u;
    __builtin_memcpy(&hdr, buf + 6, 4);
    g_videoState.frameSeq = hdr;
    (*videoWordAt(poff + 4)) = 0;
    if (g_videoState.workHalf < g_videoState.frameSeq)
        g_videoState.flags = g_videoState.flags | 3;
    goto cont;

common:
    /* slot/slotLen are byte offsets into this struct (poff selects the
       active slot[i] pair, else the still assembly area at +0x174/0x17C).
       Kept numeric: the merger intentionally aliases work and still
       buffers, matching the original's computed-offset code. */
    if (arg == 0) {
        slot = poff;
        slotLen = poff + 4;
    } else {
        slot = 380;
        slotLen = 368;
    }
    if ((g_videoState.flags & 1) == 0)
        goto plain;
    if (arg == 0)
        goto split;

    sceKernelMemcpy((u8 *)(*videoWordAt(slot)), buf, (u32)node->len);
    g_videoState.flags = g_videoState.flags & ~1u;
    (*videoWordAt(slotLen)) = (*videoWordAt(slotLen)) + (u32)node->len;
    goto nodeTail;

split:
    sceKernelMemcpy((u8 *)(*videoWordAt(slot)) + 14, buf + 2, (u32)node->len - 2);
    ((u8 *)(*videoWordAt(slot)))[0] = buf[0];
    ((u8 *)(*videoWordAt(slot)))[1] = buf[1];
    (*videoWordAt(slotLen)) = (*videoWordAt(slotLen)) + 12;
    g_videoState.flags = g_videoState.flags & ~1u;
    (*videoWordAt(slotLen)) = (*videoWordAt(slotLen)) + (u32)node->len;
    goto nodeTail;

plain:
    sceKernelMemcpy((u8 *)(*videoWordAt(slot)) + (*videoWordAt(slotLen)), buf,
                    (u32)node->len);
    (*videoWordAt(slotLen)) = (*videoWordAt(slotLen)) + (u32)node->len;
    goto nodeTail;

out:
    stopAndDrainStream(arg);
    return res;
}

/* ============================================================
 * Section: ioctl dispatch (size table + command table)
 * ============================================================ */

/* 0x55AC: shared ioctl front end for every sceUsbCamIoctl entry point.
   Guards the driver state, special-cases cmd 0x8001, then linearly scans
   the 22-entry dispatch table at 0x8F0C comparing cmd with its sign bit
   stripped. cmd >= 0 selects the setter (+4), cmd < 0 the getter (+8). */
struct UsbCamCmdEntry {
    u32 cmd;
    int (*setter)(int *);
    int (*getter)(int *);
};

static const struct UsbCamCmdEntry s_cmd8F0C[22] = {
    { 0x00000003, setSaturation, getSaturation },
    { 0x00000001, setBrightness, getBrightness },
    { 0x00000002, setContrast, getContrast },
    { 0x00000004, setSharpness, getSharpness },
    { 0x00000005, setZoom, getZoom },
    { 0x00000006, setReverseMode, getReverseMode },
    { 0x00000007, setImageEffect, getImageEffect },
    { 0x00000009, setResolution, NULL },
    { 0x40000001, setUnk40000001, getUnk40000001 },
    { 0x40000002, setUnk40000002, getUnk40000002 },
    { 0x40000003, NULL, getUnk40000003 },
    { 0x0000000A, setUnk0xA, getUnk0xA },
    { 0x0000000B, setUnk0xB, getUnk0xB },
    { 0x0000000C, setResolutionPair, getResolution },
    { 0x0000000D, setFramerate, getFramerate },
    { 0x0000000E, setUnk0xE, getUnk0xE },
    { 0x0000000F, setUnk0xF, getUnk0xF },
    { 0x00000010, setAntiFlicker, getAntiFlicker },
    { 0x00000011, setUnk0x11, getUnk0x11 },
    { 0x00000012, setUnk0x12, getUnk0x12 },
    { 0x00000013, setUnk0x13, getUnk0x13 },
    { 0x00000014, setEvLevel, getEvLevel }
};

s32 dispatchIoctl(int cmd, int *arg)
{
    int (*fn)(int *);
    u32 key;
    int i;

    if (g_videoState.started == 0)
        return SCE_ERROR_USBCAM_NOT_INIT;
    if (g_videoState.attached == 0)
        return SCE_ERROR_USBCAM_NOT_ATTACHED;
    if (sceUsbAccGetAuthStat() < 0)
        return SCE_ERROR_USBCAM_NOT_ATTACHED;
    if (cmd == 0x8001)
        return execRawCommand(arg);

    key = (u32)cmd & 0x7FFFFFFF;
    for (i = 0; i < 22; i++) {
        if (s_cmd8F0C[i].cmd != key)
            continue;
        fn = (cmd < 0) ? s_cmd8F0C[i].getter : s_cmd8F0C[i].setter;
        if (fn == NULL)
            return SCE_ERROR_USBCAM_UNKNOWN_CMD;
        return fn(arg);
    }
    return SCE_ERROR_USBCAM_UNKNOWN_CMD;
}

/* ioctl argument sizes. Derived from the original's nested range ladder;
   gaps with no row (e.g. cmd 8, 0x15, 0x80000008/09) fall through to
   UNKNOWN_CMD, matching the original. */
struct IoctlSizeEntry {
    u32 lo;
    u32 hi;
    int size;
};

static const struct IoctlSizeEntry s_ioctlSizes[] = {
    { 0x00000001, 0x00000007, 4 },
    { 0x00000009, 0x00000009, 4 },
    { 0x0000000A, 0x0000000A, 12 },
    { 0x0000000B, 0x0000000C, 8 },
    { 0x0000000D, 0x00000010, 4 },
    { 0x00000011, 0x00000011, 8 },
    { 0x00000012, 0x00000014, 4 },
    { 0x00008001, 0x00008001, 144 },
    { 0x40000001, 0x40000002, 8 },
    { 0x80000001, 0x80000007, 4 },
    { 0x8000000A, 0x8000000A, 12 },
    { 0x8000000B, 0x8000000C, 8 },
    { 0x8000000D, 0x80000010, 4 },
    { 0x80000011, 0x80000011, 8 },
    { 0x80000012, 0x80000014, 4 },
    { 0xC0000001, 0xC0000002, 8 },
    { 0xC0000003, 0xC0000003, 4 },
};

static int ioctlArgSize(int cmd, int *size)
{
    u32 i;
    for (i = 0; i < sizeof(s_ioctlSizes) / sizeof(s_ioctlSizes[0]); i++) {
        if ((u32)cmd >= s_ioctlSizes[i].lo && (u32)cmd <= s_ioctlSizes[i].hi) {
            *size = s_ioctlSizes[i].size;
            return 0;
        }
    }
    return SCE_ERROR_USBCAM_UNKNOWN_CMD;
}

/* 0x56B0 sceUsbCamIoctl - validates cmd/arg buffer size, K1-checks the
   user pointer, then hands off to dispatchIoctl (auth, 0x8001, table walk). */
s32 sceUsbCamIoctl(int cmd, int *arg)
{
    int oldK1;
    s32 ret;
    int size = 0;

    oldK1 = pspShiftK1();
    ret = SCE_ERROR_USBCAM_INVALID_PARAM;
    if (arg == NULL)
        goto out;

    ret = ioctlArgSize(cmd, &size);
    if (ret < 0)
        goto out;

    ret = SCE_ERROR_USBCAM_INVALID_ADDR;
    if (!pspK1StaBufOk(arg, size))
        goto out;
    ret = dispatchIoctl(cmd, arg);
out:
    pspSetK1(oldK1);
    return ret;
}


/* Helpers defined below; every prototype below matches its definition. */
int sendAccCommand(int cmd, int arg1, void *buf, int len);
int sendReverseFlags(void *buf);
int queryReverseState(void *arg);
int encodeBrightness(int val);
int encodeSaturation(int val);
int encodeSharpness(int val);
int encodeWidthCode(u8 *p);
int encodeHeightCode(u8 *p);
int clampEvLevel(u8 *p);
int checkEvAllowed(int curW, int curH, int val);
int encodeImageEffect(int val);
int decodeImageEffect(u8 *p);
int encodeFramerate(int val);
int decodeFramerate(u8 *p);
int encodeUnk0xE(int val);
int decodeUnk0xE(u8 *p);
int encodeWhiteBalance(int val);
int decodeWhiteBalance(u8 *p);
int encodeAntiFlicker(int val);
int decodeAntiFlicker(u8 *p);
int encodeUnk0x13(int val);
int decodeUnk0x13(u8 *p);
int encodeResolutionEx(int val, u8 *out0, u8 *out1);
int encodeResolutionPair(int val0, int val1, u8 *out0, u8 *out1);
int encodeEvLevel(int val);

/* Mode code table at .rodata 0x8E1C; values are the identity map, but
   the original indexes it on both the set and the get path. */
static const u8 s_set10Codes[3] = { 0x00, 0x01, 0x02 };

int setResolutionPair(int *arg)
{
    u8 buf[4] = { 0 };
    int res;

    if (arg[0] >= 10)
        return SCE_ERROR_USBCAM_INVALID_RES;
    if (arg[1] >= 10)
        return SCE_ERROR_USBCAM_INVALID_RES;
    res = encodeResolutionPair(arg[0], arg[1], buf, buf + 1);
    if (res < 0)
        return res;
    res = sceKernelLockMutex(g_videoState.mutex, 1, NULL);
    if (res != 0)
        return res;
    res = sendAccCommand(160, 2, buf, 2);
    sceKernelUnlockMutex(g_videoState.mutex, 1);
    if (res != 0)
        return res;
    g_videoState.cfg.width = buf[0];
    g_videoState.cfg.height = buf[1];
    return 0;
}

int setSaturation(int *arg)
{
    u8 buf[4];
    int res;

    if ((u32)*arg >= 256)
        return SCE_ERROR_USBCAM_INVALID_VALUE;
    buf[0] = encodeSaturation(*arg);
    res = sendAccCommand(164, 2, buf, 1);
    if (res != 0)
        return res;
    g_videoState.cfg.unk12 = *arg;
    return 0;
}

int getSaturation(int *arg)
{
    *arg = g_videoState.cfg.unk12;
    return 0;
}

int setBrightness(int *arg)
{
    u8 buf[4];
    int res;

    if ((u32)*arg >= 256)
        return SCE_ERROR_USBCAM_INVALID_VALUE;
    buf[0] = encodeBrightness(*arg);
    res = sceKernelLockMutex(g_videoState.mutex, 1, NULL);
    if (res != 0)
        return res;
    res = sendAccCommand(165, 2, buf, 1);
    sceKernelUnlockMutex(g_videoState.mutex, 1);
    if (res != 0)
        return res;
    g_videoState.cfg.unk13 = *arg;
    return 0;
}

int getBrightness(int *arg)
{
    *arg = g_videoState.cfg.unk13;
    return 0;
}

int setContrast(int *arg)
{
    u8 buf[4];
    int res;

    if ((u32)*arg >= 256)
        return SCE_ERROR_USBCAM_INVALID_VALUE;
    buf[0] = *arg;
    res = sceKernelLockMutex(g_videoState.mutex, 1, NULL);
    if (res != 0)
        return res;
    res = sendAccCommand(166, 2, buf, 1);
    sceKernelUnlockMutex(g_videoState.mutex, 1);
    if (res != 0)
        return res;
    g_videoState.cfg.unk14 = *arg;
    return 0;
}

int getContrast(int *arg)
{
    *arg = g_videoState.cfg.unk14;
    return 0;
}

int setSharpness(int *arg)
{
    u8 buf[4];
    int res;

    if ((u32)*arg >= 256)
        return SCE_ERROR_USBCAM_INVALID_VALUE;
    buf[0] = encodeSharpness(*arg);
    res = sceKernelLockMutex(g_videoState.mutex, 1, NULL);
    if (res != 0)
        return res;
    res = sendAccCommand(167, 2, buf, 1);
    sceKernelUnlockMutex(g_videoState.mutex, 1);
    if (res != 0)
        return res;
    g_videoState.cfg.unk15 = *arg;
    return 0;
}

int getSharpness(int *arg)
{
    *arg = g_videoState.cfg.unk15;
    return 0;
}

int setZoom(int *arg)
{
    u8 buf[4];
    int curW;
    int curH;
    int res;
    int val;

    val = *arg;
    if (val != 0) {
        if ((u32)(val - 10) >= 71)
            return SCE_ERROR_USBCAM_INVALID_RES;
        curW = encodeWidthCode(&g_videoState.cfg.width);
        curH = encodeHeightCode(&g_videoState.cfg.height);
        if (checkEvAllowed(curW, curH, val) == 0)
            return SCE_ERROR_USBCAM_INVALID_RES;
        if (g_videoState.cfg.framerate >= 5 && (u32)(curW - 7) < 3)
            return SCE_ERROR_USBCAM_INVALID_RES;
    }
    buf[0] = *arg;
    res = sceKernelLockMutex(g_videoState.mutex, 1, NULL);
    if (res != 0)
        return res;
    res = sendAccCommand(5, 2, buf, 1);
    sceKernelUnlockMutex(g_videoState.mutex, 1);
    if (res != 0)
        return res;
    g_videoState.cfg.res = buf[0];
    return 0;
}

int getZoom(int *arg)
{
    *arg = g_videoState.cfg.res;
    return 0;
}

int setReverseMode(int *arg)
{
    u16 buf;

    buf = *(u16 *)arg;
    return sendReverseFlags(&buf);
}

int getReverseMode(int *arg)
{
    return queryReverseState(arg);
}

int setImageEffect(int *arg)
{
    u8 buf[4];
    int res;

    buf[0] = encodeImageEffect(*arg);
    res = sceKernelLockMutex(g_videoState.mutex, 1, NULL);
    if (res != 0)
        return res;
    res = sendAccCommand(172, 2, buf, 1);
    sceKernelUnlockMutex(g_videoState.mutex, 1);
    if (res != 0)
        return res;
    g_videoState.cfg.effect = buf[0];
    return 0;
}

int getImageEffect(int *arg)
{
    *arg = decodeImageEffect(&g_videoState.cfg.effect);
    return 0;
}

int setResolution(int *arg)
{
    u8 buf[4] = { 0 };
    int res;

    res = encodeResolutionEx(*arg, buf, buf + 1);
    if (res < 0)
        return res;
    res = sceKernelLockMutex(g_videoState.mutex, 1, NULL);
    if (res != 0)
        return res;
    res = sendAccCommand(160, 2, buf, 2);
    sceKernelUnlockMutex(g_videoState.mutex, 1);
    if (res != 0)
        return res;
    g_videoState.cfg.width = buf[0];
    g_videoState.cfg.height = buf[1];
    return 0;
}

int getResolution(int *arg)
{
    arg[0] = encodeWidthCode(&g_videoState.cfg.width);
    arg[1] = encodeHeightCode(&g_videoState.cfg.height);
    return 0;
}

int setUnk0xB(int *arg)
{
    u32 buf[2];
    int res;

    buf[0] = (*arg == 1) | (*(u16 *)(arg + 1) << 8);
    res = sceKernelLockMutex(g_videoState.mutex, 1, NULL);
    if (res != 0)
        return res;
    res = sendAccCommand(168, 2, buf, 3);
    sceKernelUnlockMutex(g_videoState.mutex, 1);
    if (res != 0)
        return res;
    g_videoState.cfg.unk17 = buf[0];
    g_videoState.cfg.unk18 = buf[0] >> 8;
    return 0;
}

int getUnk0xB(int *arg)
{
    if (g_videoState.cfg.unk17 == 1) {
        *arg = 1;
        *(u16 *)(arg + 1) = 0;
    } else {
        *arg = 0;
        *(u16 *)(arg + 1) = g_videoState.cfg.unk18;
    }
    return 0;
}

int setFramerate(int *arg)
{
    u8 buf[4];
    int res;

    if ((u32)*arg >= 8)
        return SCE_ERROR_USBCAM_INVALID_RES;
    buf[0] = encodeFramerate(*arg);
    res = sceKernelLockMutex(g_videoState.mutex, 1, NULL);
    if (res != 0)
        return res;
    res = sendAccCommand(161, 2, buf, 1);
    sceKernelUnlockMutex(g_videoState.mutex, 1);
    if (res != 0)
        return res;
    g_videoState.cfg.framerate = buf[0];
    return 0;
}

int getFramerate(int *arg)
{
    *arg = decodeFramerate(&g_videoState.cfg.framerate);
    return 0;
}

int setUnk0xE(int *arg)
{
    u8 buf[4];
    int res;

    if ((u32)*arg >= 4)
        return SCE_ERROR_USBCAM_INVALID_RES;
    buf[0] = encodeUnk0xE(*arg);
    res = sceKernelLockMutex(g_videoState.mutex, 1, NULL);
    if (res != 0)
        return res;
    res = sendAccCommand(162, 2, buf, 1);
    sceKernelUnlockMutex(g_videoState.mutex, 1);
    if (res != 0)
        return res;
    g_videoState.cfg.unk10 = buf[0];
    return 0;
}

int getUnk0xE(int *arg)
{
    *arg = decodeUnk0xE(&g_videoState.cfg.unk10);
    return 0;
}

int setUnk0xF(int *arg)
{
    u8 buf[4];
    int res;

    if ((u32)*arg >= 4)
        return SCE_ERROR_USBCAM_INVALID_RES;
    buf[0] = encodeWhiteBalance(*arg);
    res = sceKernelLockMutex(g_videoState.mutex, 1, NULL);
    if (res != 0)
        return res;
    res = sendAccCommand(163, 2, buf, 1);
    sceKernelUnlockMutex(g_videoState.mutex, 1);
    if (res != 0)
        return res;
    g_videoState.cfg.wb = buf[0];
    return 0;
}

int getUnk0xF(int *arg)
{
    *arg = decodeWhiteBalance(&g_videoState.cfg.wb);
    return 0;
}

int setAntiFlicker(int *arg)
{
    u8 buf[4];
    int res;

    if ((u32)*arg >= 3)
        return SCE_ERROR_USBCAM_INVALID_RES;
    buf[0] = encodeAntiFlicker(*arg);
    res = sceKernelLockMutex(g_videoState.mutex, 1, NULL);
    if (res != 0)
        return res;
    res = sendAccCommand(169, 2, buf, 1);
    sceKernelUnlockMutex(g_videoState.mutex, 1);
    if (res != 0)
        return res;
    g_videoState.cfg.antiflicker = buf[0];
    return 0;
}

int getAntiFlicker(int *arg)
{
    *arg = decodeAntiFlicker(&g_videoState.cfg.antiflicker);
    return 0;
}

int setUnk0x11(int *arg)
{
    u16 buf[2];
    int res;

    if ((u32)arg[0] + 0x8013 > 0x10026)
        return SCE_ERROR_USBCAM_INVALID_PARAM;
    if (arg[1] < -32787 || arg[1] > 32787)
        return SCE_ERROR_USBCAM_INVALID_PARAM;
    if (arg[0] < 0) {
        buf[0] = -arg[0];
        arg[0] = (arg[0] | 0x8000) + 1;
    } else {
        buf[0] = arg[0];
    }
    if (arg[1] < 0) {
        buf[1] = -arg[1];
        arg[1] = (arg[1] | 0x8000) + 1;
    } else {
        buf[1] = arg[1];
    }
    res = sceKernelLockMutex(g_videoState.mutex, 1, NULL);
    if (res != 0)
        return res;
    res = sendAccCommand(170, 2, buf, 4);
    sceKernelUnlockMutex(g_videoState.mutex, 1);
    if (res != 0)
        return res;
    g_videoState.cfg.unk1A = *(u16 *)arg;
    g_videoState.cfg.unk1C = *(u16 *)(arg + 1);
    return 0;
}

int getUnk0x11(int *arg)
{
    u16 v;

    v = g_videoState.cfg.unk1A;
    arg[0] = (v & 0x8000) ? 1 - (v & 0x7FFF) : v;
    v = g_videoState.cfg.unk1C;
    arg[1] = (v & 0x8000) ? 1 - (v & 0x7FFF) : v;
    return 0;
}

int setUnk0x12(int *arg)
{
    u8 buf[4];
    int res;

    buf[0] = (*arg != 0);
    res = sceKernelLockMutex(g_videoState.mutex, 1, NULL);
    if (res != 0)
        return res;
    res = sendAccCommand(173, 2, buf, 1);
    sceKernelUnlockMutex(g_videoState.mutex, 1);
    if (res != 0)
        return res;
    g_videoState.cfg.unk1F = buf[0];
    return 0;
}

int getUnk0x12(int *arg)
{
    *arg = g_videoState.cfg.unk1F;
    return 0;
}

int setUnk0x13(int *arg)
{
    u8 buf[4];
    int res;

    if ((u32)*arg >= 3)
        return SCE_ERROR_USBCAM_INVALID_RES;
    buf[0] = encodeUnk0x13(*arg);
    res = sceKernelLockMutex(g_videoState.mutex, 1, NULL);
    if (res != 0)
        return res;
    res = sendAccCommand(174, 2, buf, 1);
    sceKernelUnlockMutex(g_videoState.mutex, 1);
    if (res != 0)
        return res;
    g_videoState.cfg.unk0F = buf[0];
    return 0;
}

int getUnk0x13(int *arg)
{
    *arg = decodeUnk0x13(&g_videoState.cfg.unk0F);
    return 0;
}

int setEvLevel(int *arg)
{
    u8 buf[4];
    int res;

    if ((u32)*arg >= 17)
        return SCE_ERROR_USBCAM_INVALID_RES;
    buf[0] = encodeEvLevel(*arg);
    res = sceKernelLockMutex(g_videoState.mutex, 1, NULL);
    if (res != 0)
        return res;
    res = sendAccCommand(175, 2, buf, 1);
    sceKernelUnlockMutex(g_videoState.mutex, 1);
    if (res != 0)
        return res;
    g_videoState.cfg.ev = buf[0];
    return 0;
}

int getEvLevel(int *arg)
{
    *arg = clampEvLevel(&g_videoState.cfg.ev);
    return 0;
}

int setUnk40000001(int *arg)
{
    u8 buf[4];

    *(u16 *)buf = *(u16 *)arg;
    buf[2] = *((u8 *)arg + 4);
    return sendAccCommand(68, 3, buf, 3);
}

int getUnk40000001(int *arg)
{
    u8 buf[4];
    u32 outBits;
    u8 *bufPtr;
    int res;

    res = sceKernelLockMutex(g_videoState.mutex, 1, NULL);
    if ((u32)res - 0x800201A9u < 2)
        return SCE_ERROR_USBCAM_NOT_ATTACHED;
    if (res < 0)
        return res;
    *(u16 *)buf = *(u16 *)arg;
    sceKernelClearEventFlag(g_videoState.eventflag, 0xFFFEFFFF);
    res = sendAccCommand(67, 3, buf, 2);
    if (res < 0)
        goto unlock;
    res = sceKernelWaitEventFlag(g_videoState.eventflag, 0x400, 1, &outBits, NULL);
    if (res < 0) {
        Kprintf("%serror - at waiting for event at line %d): 0x%08x\n", "", 793, outBits);
        goto unlock;
    }
    if (outBits & 0x400) {
        res = SCE_ERROR_USBCAM_NOT_ATTACHED;
        goto unlock;
    }
    if (g_videoState.replyMismatch != 0) {
        res = SCE_ERROR_USBCAM_REPLY_MISMATCH;
        goto unlock;
    }
    arg[1] = 0;
    bufPtr = (u8 *)g_videoState.cmdBuf;
    memcpy((u8 *)arg + 4, bufPtr + 4, bufPtr[3]);
unlock:
    sceKernelUnlockMutex(g_videoState.mutex, 1);
    return res;
}

int setUnk40000002(int *arg)
{
    u8 buf[4];

    buf[0] = *(u8 *)arg;
    buf[1] = *((u8 *)arg + 4);
    return sendAccCommand(65, 2, buf, 2);
}

int getUnk40000002(int *arg)
{
    u8 buf[4];
    u32 outBits;
    u8 *bufPtr;
    int res;

    res = sceKernelLockMutex(g_videoState.mutex, 1, NULL);
    if ((u32)res - 0x800201A9u < 2)
        return SCE_ERROR_USBCAM_NOT_ATTACHED;
    if (res < 0)
        return res;
    buf[0] = *(u8 *)arg;
    sceKernelClearEventFlag(g_videoState.eventflag, 0xFFFEFFFF);
    res = sendAccCommand(64, 2, buf, 1);
    if (res < 0)
        goto unlock;
    res = sceKernelWaitEventFlag(g_videoState.eventflag, 0x400, 1, &outBits, NULL);
    if (res < 0) {
        Kprintf("%serror - at waiting for event at line %d): 0x%08x\n", "", 866, outBits);
        goto unlock;
    }
    if (outBits & 0x400) {
        res = SCE_ERROR_USBCAM_NOT_ATTACHED;
        goto unlock;
    }
    if (g_videoState.replyMismatch != 0) {
        res = SCE_ERROR_USBCAM_REPLY_MISMATCH;
        goto unlock;
    }
    arg[1] = 0;
    bufPtr = (u8 *)g_videoState.cmdBuf;
    memcpy((u8 *)arg + 4, bufPtr + 4, bufPtr[3]);
unlock:
    sceKernelUnlockMutex(g_videoState.mutex, 1);
    return res;
}

int getUnk40000003(int *arg)
{
    u32 outBits;
    u8 *bufPtr;
    int res;

    res = sceKernelLockMutex(g_videoState.mutex, 1, NULL);
    if ((u32)res - 0x800201A9u < 2)
        return SCE_ERROR_USBCAM_NOT_ATTACHED;
    if (res < 0)
        return res;
    sceKernelClearEventFlag(g_videoState.eventflag, 0xFFFEFFFF);
    res = sendAccCommand(9, 3, NULL, 0);
    if (res < 0)
        goto unlock;
    res = sceKernelWaitEventFlag(g_videoState.eventflag, 0x400, 1, &outBits, NULL);
    if (res < 0) {
        Kprintf("%serror - at waiting for event at line %d): 0x%08x\n", "", 917, outBits);
        goto unlock;
    }
    if (outBits & 0x400) {
        res = SCE_ERROR_USBCAM_NOT_ATTACHED;
        goto unlock;
    }
    bufPtr = (u8 *)g_videoState.cmdBuf;
    *arg = *(u16 *)(bufPtr + 4);
unlock:
    sceKernelUnlockMutex(g_videoState.mutex, 1);
    return res;
}

int setUnk0xA(int *arg)
{
    u32 buf[2];
    u16 v1;
    u16 v2;
    int res;

    if ((u32)arg[0] >= 3)
        return SCE_ERROR_USBCAM_INVALID_RES;
    if ((u32)arg[1] > 0xFFFF)
        return SCE_ERROR_USBCAM_INVALID_PARAM;
    if ((u32)arg[2] > 0xFFFF)
        return SCE_ERROR_USBCAM_INVALID_PARAM;
    v1 = *(u16 *)(arg + 1);
    v2 = *(u16 *)(arg + 2);
    buf[0] = s_set10Codes[arg[0]] | ((u32)v1 << 8) | ((u32)v2 << 24);
    buf[1] = v2 >> 8;
    res = sceKernelLockMutex(g_videoState.mutex, 1, NULL);
    if (res != 0)
        return res;
    res = sendAccCommand(6, 2, buf, 5);
    sceKernelUnlockMutex(g_videoState.mutex, 1);
    if (res != 0)
        return res;
    g_videoState.cfg.unk21 = buf[0];
    g_videoState.cfg.unk22 = buf[0] >> 8;
    g_videoState.cfg.unk24 = (buf[0] >> 24) | ((buf[1] & 0xFF) << 8);
    return 0;
}

int getUnk0xA(int *arg)
{
    int i;

    for (i = 0; i < 3; i++) {
        if (g_videoState.cfg.unk21 == s_set10Codes[i])
            break;
    }
    if (i == 3)
        return SCE_ERROR_USBCAM_INVALID_RES;
    arg[0] = i;
    arg[1] = g_videoState.cfg.unk22;
    arg[2] = g_videoState.cfg.unk24;
    return 0;
}

/* 0x6DA4 execRawCommand */
s32 execRawCommand(int *arg)
{
    u32 outBits;
    int res;

    res = sceKernelLockMutex(g_videoState.mutex, 1, NULL);
    if ((u32)res - 0x800201A9u < 2)
        return SCE_ERROR_USBCAM_NOT_ATTACHED;
    if (res < 0)
        return res;
    sceKernelClearEventFlag(g_videoState.eventflag, 0xFFFEFFFF);
    res = sendAccCommand(arg[1], arg[2], &arg[4], arg[3]);
    if (res < 0)
        goto unlock;
    if (arg[0] == 0)
        goto unlock;
    res = sceKernelWaitEventFlag(g_videoState.eventflag, 0x10400, 1, &outBits, NULL);
    if (res < 0) {
        Kprintf("%serror - at waiting for event at line %d): 0x%08x\n", "", 1030, outBits);
        goto unlock;
    }
    if (outBits & 0x400) {
        res = SCE_ERROR_USBCAM_NOT_ATTACHED;
        goto unlock;
    }
    __builtin_memcpy((u8 *)arg + 4, (u8 *)g_videoState.cmdBuf + 4, 64);
unlock:
    sceKernelUnlockMutex(g_videoState.mutex, 1);
    return res;
}


/* 0x6F78 sceUsbCamSetupMicEx */
int sceUsbCamSetupMicEx(struct UsbCamSetupMicExParam *param, void *workarea, int wasize)
{
    u16 cmd[7];
    int oldK1;
    int ret;

    if (g_micState.started == 0)
        return SCE_ERROR_USBCAM_NOT_INIT;
    if (g_micState.attached == 0)
        return SCE_ERROR_USBCAM_NOT_ATTACHED;
    if (sceUsbAccGetAuthStat() < 0)
        return SCE_ERROR_USBCAM_NOT_ATTACHED;
    if ((u32)wasize < 264)
        return SCE_ERROR_USBCAM_BUF_SMALL;
    if (param == NULL)
        return SCE_ERROR_USBCAM_INVALID_ADDR;

    oldK1 = pspShiftK1();
    ret = SCE_ERROR_USBCAM_INVALID_ADDR;
    if (!pspK1StaBufOk(param, 36))
        goto out;
    if (workarea == NULL)
        goto out;
    if (!pspK1DynBufOk(workarea, wasize))
        goto out;
    /* Original falls straight into the epilogue here without pspSetK1. */
    if (param->freq != 48000 && param->freq != 44100 &&
        param->freq != 22050 && param->freq != 11025)
        return SCE_ERROR_USBCAM_INVALID_FREQ;

    cmd[0] = (u16)param->alc;
    cmd[1] = (u16)param->gain;
    cmd[2] = (u16)param->unk2[0];
    cmd[3] = (u16)param->unk2[1];
    cmd[4] = (u16)param->unk2[2];
    cmd[5] = (u16)param->unk2[3];
    cmd[6] = (u16)(param->freq / 1000);
    ret = commitMicSetup(cmd, param->unk3, workarea, wasize);
out:
    pspSetK1(oldK1);
    return ret;
}


/* 0x7100 sceUsbCamSetupMic */
int sceUsbCamSetupMic(struct UsbCamSetupMicParam *param, void *workarea, int wasize)
{
    u16 cmd[7];
    int oldK1;
    int ret;

    oldK1 = pspShiftK1();
    ret = SCE_ERROR_USBCAM_NOT_INIT;
    if (g_micState.started == 0)
        goto out;
    ret = SCE_ERROR_USBCAM_NOT_ATTACHED;
    if (g_micState.attached == 0)
        goto out;
    if (sceUsbAccGetAuthStat() < 0)
        goto out;
    ret = SCE_ERROR_USBCAM_BUF_SMALL;
    if ((u32)wasize < 264)
        goto out;
    ret = SCE_ERROR_USBCAM_INVALID_ADDR;
    if (param == NULL)
        goto out;
    if (!pspK1StaBufOk(param, 20))
        goto out;
    if (workarea == NULL)
        goto out;
    if (!pspK1DynBufOk(workarea, wasize))
        goto out;
    ret = SCE_ERROR_USBCAM_INVALID_FREQ;
    if (param->freq != 44100 && param->freq != 22050 && param->freq != 11025)
        goto out;

    cmd[0] = (u16)param->alc;
    cmd[1] = (u16)param->gain;
    cmd[2] = (u16)param->noize;
    cmd[3] = 0;
    cmd[4] = 3;
    cmd[5] = 2;
    cmd[6] = (u16)(param->freq / 1000);
    ret = commitMicSetup(cmd, 0, workarea, wasize);
out:
    pspSetK1(oldK1);
    return ret;
}


/* 0x7278 sceUsbCamStopMic */
s32 sceUsbCamStopMic(void)
{
    int oldK1;
    s32 ret;

    if (g_micState.started == 0)
        return SCE_ERROR_USBCAM_NOT_INIT;
    if (g_micState.attached == 0)
        return SCE_ERROR_USBCAM_NOT_ATTACHED;
    if (sceUsbAccGetAuthStat() < 0)
        return SCE_ERROR_USBCAM_NOT_ATTACHED;
    if (g_micState.setupFlags == 0)
        return SCE_ERROR_USBCAM_NOT_SETUP;
    oldK1 = pspShiftK1();
    /* uOFW note: the original stops the mic by re-issuing the start
       transfer; verified from the disassembly. */
    ret = sendMicStart();
    pspSetK1(oldK1);
    return ret;
}


/* 0x72F8 sceUsbCamReadMic */
int sceUsbCamReadMic(u8 *buf, SceSize size)
{
    u32 outBits;
    int oldK1;
    int ret;
    int intr;

    oldK1 = pspShiftK1();
    ret = SCE_ERROR_USBCAM_NOT_INIT;
    if (g_micState.started == 0)
        goto out;
    if (g_micState.aux != 0) {
        ret = sceKernelWaitEventFlag(g_micState.eventflag, 0x500, 1, &outBits, NULL);
        if (ret < 0)
            goto out;
        if (outBits & 0x100) {
            ret = SCE_ERROR_USBCAM_NOT_ATTACHED;
            goto out;
        }
        sceKernelClearEventFlag(g_micState.eventflag, 0xFBFF);
    }
    intr = sceKernelCpuSuspendIntr();
    ret = validateMicRead(buf, size);
    if (ret < 0)
        goto resume;
    if (g_micState.mode == 4) {
        ret = SCE_ERROR_USBCAM_BUSY;
        goto resume;
    }
    sceKernelClearEventFlag(g_micState.eventflag, 0xFFFFFFFD);
    g_micState.readSize = size;
    g_micState.readBuf = (u32)buf;
    g_micState.mode = 4;
    ret = sceKernelSetEventFlag(g_micState.eventflag, 4);
resume:
    sceKernelCpuResumeIntr(intr);
out:
    pspSetK1(oldK1);
    return ret;
}


/* 0x7428 sceUsbCamReadMicBlocking */
int sceUsbCamReadMicBlocking(u8 *buf, SceSize size)
{
    u32 outBits;
    int oldK1;
    int ret;
    int intr;

    oldK1 = pspShiftK1();
    ret = SCE_ERROR_USBCAM_NOT_INIT;
    if (g_micState.started == 0)
        goto out;
    if (g_micState.aux != 0) {
        ret = sceKernelWaitEventFlag(g_micState.eventflag, 0x500, 1, &outBits, NULL);
        if (ret < 0)
            goto out;
        if (outBits & 0x100) {
            ret = SCE_ERROR_USBCAM_NOT_ATTACHED;
            goto out;
        }
        sceKernelClearEventFlag(g_micState.eventflag, 0xFBFF);
    }
    intr = sceKernelCpuSuspendIntr();
    ret = validateMicRead(buf, size);
    if (ret < 0)
        goto resume;
    if (g_micState.mode == 4) {
        /* Original jumps straight to the epilogue: sceKernelCpuResumeIntr
           is skipped and interrupts stay disabled. */
        ret = SCE_ERROR_USBCAM_BUSY;
        goto out;
    }
    sceKernelClearEventFlag(g_micState.eventflag, 0xFFFFFFFD);
    g_micState.readSize = size;
    g_micState.readBuf = (u32)buf;
    g_micState.mode = 4;
    ret = sceKernelSetEventFlag(g_micState.eventflag, 4);
resume:
    sceKernelCpuResumeIntr(intr);
    if (ret < 0)
        goto out;
    ret = sceKernelWaitEventFlag(g_micState.eventflag, 0x102, 1, &outBits, NULL);
    if (ret < 0)
        goto out;
    if (outBits & 0x100) {
        ret = SCE_ERROR_USBCAM_NOT_ATTACHED;
        goto out;
    }
    ret = g_micState.status;
out:
    pspSetK1(oldK1);
    return ret;
}


/* 0x75A0 sceUsbCamWaitReadMicEnd */
int sceUsbCamWaitReadMicEnd(void)
{
    u32 outBits;
    int oldK1;
    int ret;

    oldK1 = pspShiftK1();
    ret = SCE_ERROR_USBCAM_NOT_INIT;
    if (g_micState.started == 0)
        goto out;
    ret = SCE_ERROR_USBCAM_NOT_ATTACHED;
    if (g_micState.attached == 0)
        goto out;
    if (sceUsbAccGetAuthStat() < 0)
        goto out;
    ret = SCE_ERROR_USBCAM_NOT_SETUP;
    if (g_micState.setupFlags == 0)
        goto out;

    ret = sceKernelWaitEventFlag(g_micState.eventflag, 0x102, 1, &outBits, NULL);
    if (ret < 0)
        goto out;
    if (outBits & 0x100) {
        ret = SCE_ERROR_USBCAM_NOT_ATTACHED;
        goto out;
    }
    ret = g_micState.status;
out:
    pspSetK1(oldK1);
    return ret;
}


/* 0x765C sceUsbCamPollReadMicEnd */
int sceUsbCamPollReadMicEnd(void)
{
    u32 outBits;
    int oldK1;
    int ret;

    oldK1 = pspShiftK1();
    ret = SCE_ERROR_USBCAM_NOT_INIT;
    if (g_micState.started == 0)
        goto out;
    ret = SCE_ERROR_USBCAM_NOT_ATTACHED;
    if (g_micState.attached == 0)
        goto out;
    if (sceUsbAccGetAuthStat() < 0)
        goto out;
    ret = SCE_ERROR_USBCAM_NOT_SETUP;
    if (g_micState.setupFlags == 0)
        goto out;

    ret = sceKernelPollEventFlag(g_micState.eventflag, 0x102, 1, &outBits);
    if (ret == (s32)0x800201AF) {
        ret = SCE_ERROR_USBCAM_NOT_READY;
        goto out;
    }
    if (ret < 0)
        goto out;
    if (outBits & 0x100) {
        ret = SCE_ERROR_USBCAM_NOT_ATTACHED;
        goto out;
    }
    ret = g_micState.status;
out:
    pspSetK1(oldK1);
    return ret;
}


/* 0x772C sceUsbCamGetMicDataLength */
int sceUsbCamGetMicDataLength(void)
{
    int oldK1;
    int ret;

    oldK1 = pspShiftK1();
    ret = SCE_ERROR_USBCAM_NOT_INIT;
    if (g_micState.started == 0)
        goto out;
    ret = SCE_ERROR_USBCAM_NOT_ATTACHED;
    if (g_micState.attached == 0)
        goto out;
    if (sceUsbAccGetAuthStat() < 0)
        goto out;
    ret = SCE_ERROR_USBCAM_NOT_SETUP;
    if (g_micState.setupFlags == 0)
        goto out;
    ret = g_micState.status;
out:
    pspSetK1(oldK1);
    return ret;
}


/* 0x77B4 sceUsbCamSetMicGain */
s32 sceUsbCamSetMicGain(int gain)
{
    int oldK1;
    s32 ret;

    oldK1 = pspShiftK1();
    g_micState.aux = 1;
    ret = 0;
    sceKernelClearEventFlag(g_micState.eventflag, 0xFBFF);

    ret = SCE_ERROR_USBCAM_NOT_INIT;
    if (g_micState.started == 0)
        goto out;
    ret = SCE_ERROR_USBCAM_NOT_ATTACHED;
    if (g_micState.attached == 0)
        goto out;
    if (sceUsbAccGetAuthStat() < 0)
        goto out;
    ret = SCE_ERROR_USBCAM_NOT_SETUP;
    if (g_micState.setupFlags == 0)
        goto out;

    ret = 0;
    if ((s16)g_micState.gain != (s16)gain) {
        g_micState.gain = (u16)gain;
        ret = startMicSync();
        g_micState.aux = 0;
        sceKernelSetEventFlag(g_micState.eventflag, 0x400);
    }
out:
    pspSetK1(oldK1);
    return ret;
}


/* 0x78A0 commitMicSetup */
s32 commitMicSetup(void *cmd, int flag, void *workarea, int wasize)
{
    int intr;

    intr = sceKernelCpuSuspendIntr();
    __builtin_memcpy(&g_micState.cmdLo, cmd, 14);
    g_micState.bufSize = wasize;
    g_micState.setupOk = (u32)workarea;
    g_micState.rateFlag = (flag != 0);
    if (flag != 0) {
        g_micState.bufBase = (u32)workarea;
        g_micState.writePos = 0;
        g_micState.readPos = 0;
        g_micState.writeCursor = 0;
        g_micState.ringCapacity = (u32)wasize / 132;
    } else {
        g_micState.bufBase = (u32)workarea;
        g_micState.ringCapacity = wasize;
        g_micState.writePos = 0;
        g_micState.readPos = (u32)workarea;
        g_micState.writeCursor = (u32)workarea;
    }
    sceKernelCpuResumeIntr(intr);
    g_micState.setupFlags = 1;
    return 0;
}


/* 0x79A0 sendMicStart */
s32 sendMicStart(void)
{
    struct MicState *st = (struct MicState *)&g_micState;
    int intr;
    s32 ret;

    intr = sceKernelCpuSuspendIntr();
    if (sceUsbAccGetAuthStat() != 0) {
        ret = SCE_ERROR_USBCAM_NOT_ATTACHED;
        goto out;
    }
    if (st->intrReq.retcode > 0) {
        ret = SCE_ERROR_USB_INTR_FAILED;
        goto out;
    }
    ((u8 *)st->ep0Buf)[2] = 2;
    ((u8 *)st->ep0Buf)[3] = 0;
    ret = sceUsbAccIntrInReq(&st->intrReq);
    if (ret < 0)
        goto out;
    st->mode = 2;
    ret = 0;
out:
    sceKernelCpuResumeIntr(intr);
    return ret;
}


/* 0x7A3C startMicSync */
s32 startMicSync(void)
{
    u32 outBits;
    s32 ret;

    if (g_micState.mode == 4) {
        ret = sceKernelWaitEventFlag(g_micState.eventflag, 0x102, 1, &outBits, NULL);
        if (ret < 0)
            return ret;
        if (outBits & 0x100)
            return SCE_ERROR_USBCAM_NOT_ATTACHED;
    }
    sendMicStart();
    ret = sceKernelWaitEventFlag(g_micState.eventflag, 0x120, 1, &outBits, NULL);
    if (ret < 0)
        return ret;
    if (outBits & 0x100)
        return SCE_ERROR_USBCAM_NOT_ATTACHED;
    return sendMicSetup();
}


/* 0x7AF8 validateMicRead */
s32 validateMicRead(void *buf, int size)
{
    if (g_micState.started == 0)
        return SCE_ERROR_USBCAM_NOT_INIT;
    if (g_micState.attached == 0)
        return SCE_ERROR_USBCAM_NOT_ATTACHED;
    if (sceUsbAccGetAuthStat() < 0)
        return SCE_ERROR_USBCAM_NOT_ATTACHED;
    if (g_micState.setupFlags == 0)
        return SCE_ERROR_USBCAM_NOT_SETUP;
    if (g_micState.mode == 0 || g_micState.mode == 2)
        return SCE_ERROR_USBCAM_MIC_STATE;
    if (buf == NULL)
        return SCE_ERROR_USBCAM_INVALID_PARAM;
    if (!pspK1DynBufOk(buf, size))
        return SCE_ERROR_USBCAM_INVALID_ADDR;
    if (g_micState.rateFlag != 0) {
        if ((u32)size < 132)
            return SCE_ERROR_USBCAM_INVALID_SIZE;
    } else {
        if ((u32)size < 2)
            return SCE_ERROR_USBCAM_INVALID_SIZE;
        if (size & 1)
            return SCE_ERROR_USBCAM_INVALID_SIZE;
    }
    return 0;
}


/* 0x7BF4 sceUsbCamStartMic */
s32 sceUsbCamStartMic(void)
{
    int oldK1;
    int ret;
    u32 buf;

    oldK1 = pspShiftK1();
    if (g_micState.rateFlag != 0) {
        g_micState.writeCursor = 0;
        g_micState.writePos = 0;
        g_micState.readPos = 0;
    } else {
        buf = g_micState.bufBase;
        g_micState.writePos = 0;
        g_micState.readPos = buf;
        g_micState.writeCursor = buf;
    }
    ret = sendMicSetup();
    pspSetK1(oldK1);
    return ret;
}


s32 registerMicDriver(int arg0 __attribute__((unused)), int arg1 __attribute__((unused)))
{
    if (sceUsbbdRegister(&g_micDriver) < 0) {
        return 1;
    }
    g_micState.started = 0;
    return 0;
}

s32 unregisterMicDriver(int arg0 __attribute__((unused)), int arg1 __attribute__((unused)))
{
    return sceUsbbdUnregister(&g_micDriver) < 0;
}


/* micRecvComplete - mic endpoint receive completion callback
   (installed as g_micState.reqs[i].func). */
void micRecvComplete(struct UsbdDeviceReq *req)
{
    u32 writePtr;
    u32 start;
    u32 end;
    u32 reqEnd;
    u32 newWrite;
    u32 first;
    u32 avail;
    u32 recvsize;
    u8 *blk;
    u16 payload;

    if (req->retcode < 0)
        return;
    if (g_micState.mode == 0 || g_micState.mode == 2)
        return;

    if (g_micState.setupOk == 0) {
        if (g_micState.mode == 1 || g_micState.mode == 4) {
            sceKernelDcacheInvalidateRange(req->data, 256);
            sceUsbbdReqRecv(req);
        }
        return;
    }

    recvsize = (u32)req->recvsize;
    if (g_micState.rateFlag != 0) {
        blk = (u8 *)(g_micState.bufBase + g_micState.writeCursor * 132);
        payload = (u16)(recvsize - 2);
        blk[0] = ((u8 *)req->data)[0];
        blk[1] = ((u8 *)req->data)[1];
        *(u16 *)(blk + 2) = payload;
        memcpy(blk + 4, (u8 *)req->data + 2, payload);
        memset(blk + 4 + payload, 0, 128 - payload);

        g_micState.writeCursor = g_micState.writeCursor + 1;
        if (g_micState.writeCursor >= g_micState.ringCapacity)
            g_micState.writeCursor = 0;
        if (g_micState.writePos >= g_micState.ringCapacity) {
            g_micState.readPos = g_micState.writeCursor;
            avail = g_micState.writePos;
        } else {
            g_micState.writePos = g_micState.writePos + 1;
            avail = g_micState.writePos;
        }
    } else {
        writePtr = g_micState.writeCursor;
        start = g_micState.bufBase;
        end = start + (g_micState.ringCapacity & ~1u);
        reqEnd = writePtr + (recvsize & ~1u);

        if (end < reqEnd) {
            first = reqEnd - end;
            conditionalSwapCopy((void *)writePtr, req->data, (int)(recvsize - first));
            conditionalSwapCopy((void *)g_micState.bufBase,
                         (u8 *)req->data + (recvsize - first), (int)first);
            newWrite = g_micState.bufBase + (first & ~1u);
        } else {
            conditionalSwapCopy((void *)writePtr, req->data, (int)recvsize);
            newWrite = writePtr + (recvsize & ~1u);
        }

        g_micState.writePos = g_micState.writePos + recvsize;
        if (g_micState.ringCapacity < g_micState.writePos)
            g_micState.writePos = g_micState.ringCapacity;
        g_micState.writeCursor = newWrite;
        if (g_micState.ringCapacity == g_micState.writePos)
            g_micState.readPos = g_micState.writeCursor;
        avail = g_micState.writePos;
    }

    if (avail != 0)
        sceKernelSetEventFlag(g_micState.eventflag, 1);
    if (g_micState.mode == 1 || g_micState.mode == 4) {
        sceKernelDcacheInvalidateRange(req->data, 256);
        sceUsbbdReqRecv(req);
    }
}

/* 0x7F0C sendMicSetup - arm the mic receive path: guards under
   cpu-suspend, build the setup packet in unk60, submit reqD. */
s32 sendMicSetup(void)
{
    struct MicState *st = (struct MicState *)&g_micState;
    s32 intr;
    s32 ret;
    u16 buf[8];
    void *src;
    int breq;
    int len;
    int i;

    intr = sceKernelCpuSuspendIntr();
    ret = SCE_ERROR_USBCAM_NOT_INIT;
    if (st->started == 0)
        goto out;
    ret = SCE_ERROR_USBCAM_NOT_ATTACHED;
    if (st->attached == 0)
        goto out;
    if (sceUsbAccGetAuthStat() < 0)
        goto out;
    ret = SCE_ERROR_USBCAM_NOT_SETUP;
    if (st->setupFlags == 0)
        goto out;
    ret = SCE_ERROR_USBCAM_MIC_NOT_SETUP;
    if (g_micState.setupOk == 0)
        goto out;
    ret = SCE_ERROR_USBCAM_BUSY;
    if (st->mode != 0)
        goto out;
    ret = SCE_ERROR_USB_INTR_FAILED;
    if (st->intrReq.retcode > 0)
        goto out;

    if (g_micState.rateFlag != 0) {
        for (i = 0; i < 7; i++)
            buf[i] = ((u16 *)&st->cmdLo)[i];
        ((u8 *)buf)[14] = 1;
        src = buf;
        breq = 3;
        len = 15;
    } else {
        src = &st->cmdLo;
        breq = 1;
        len = 14;
    }

    ((u8 *)st->ep0Buf)[2] = breq;
    ((u8 *)st->ep0Buf)[3] = len;
    memcpy((u8 *)st->ep0Buf + 4, src, len);
    ret = sceUsbAccIntrInReq(&st->intrReq);
    if (ret < 0)
        goto out;
    st->mode = 3;
    st->altSetting = 1;
    ret = 0;
out:
    sceKernelCpuResumeIntr(intr);
    return ret;
}


/* 0x808C conditionalSwapCopy - copy helper: plain memcpy, or per-16-bit-swap
   copy when g_micState.swapMode is set (original comment: "16 aligned data swap"). */
void *conditionalSwapCopy(void *dst, void *src, int size)
{
    u32 *d = (u32 *)dst;
    u32 *s = (u32 *)src;
    u32 n;
    u32 i;

    if (g_micState.swapMode == 0) {
        memcpy(dst, src, size);
    } else {
        n = (u32)size >> 2;
        for (i = 0; i < n; i++)
            d[i] = ((s[i] >> 8) & 0x00FF00FFu) | ((s[i] << 8) & 0xFF00FF00u);
    }
    return dst;
}


/* 0x80F8 drainMicBuffer - drain the mic buffer into dst under cpu-suspend.
   Last function in the driver's .text (0x80F8-0x8334). */
s32 drainMicBuffer(void *dst, int size)
{
    s32 intr;
    s32 ret;
    u32 n;
    u32 avail;
    u32 i;
    u32 idx;
    u32 half;
    u32 limit;
    u32 rptr;
    u32 dptr;
    u8 *d;

    intr = sceKernelCpuSuspendIntr();

    if (g_micState.rateFlag != 0) {
        n = (u32)size / 132;
        if (n == 0) {
            ret = 0;
            goto out;
        }
        if (g_micState.writePos < n)
            n = g_micState.writePos;
        d = dst;
        for (i = 0; i < n; i++) {
            __builtin_memcpy(d, (void *)(g_micState.bufBase + g_micState.readPos * 132),
                             132);
            d += 132;
            idx = g_micState.readPos + 1;
            g_micState.readPos = (idx < g_micState.ringCapacity) ? idx : 0;
            g_micState.writePos = g_micState.writePos - 1;
        }
        if (g_micState.writePos == 0)
            sceKernelClearEventFlag(g_micState.eventflag, -2);
        ret = n * 132;
    } else {
        avail = g_micState.writePos;
        n = (u32)size;
        if (avail < n)
            n = avail;
        rptr = g_micState.readPos;
        limit = g_micState.bufBase + (g_micState.ringCapacity & ~1u);
        dptr = (u32)dst;
        half = n >> 1;
        for (i = 0; i < half; i++) {
            *(u16 *)dptr = *(u16 *)rptr;
            rptr += 2;
            if (rptr >= limit)
                rptr = g_micState.bufBase;
            dptr += 2;
        }
        g_micState.readPos = rptr;
        g_micState.writePos = avail - n;
        if (avail - n == 0)
            sceKernelClearEventFlag(g_micState.eventflag, -2);
        ret = n;
    }
out:
    sceKernelCpuResumeIntr(intr);
    return ret;
}
