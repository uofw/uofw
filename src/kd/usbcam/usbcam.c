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

/* Microphone device state (0x12C bytes). */
struct MicState {
    u8 unk0;
    u8 pad[0x12B];
};

/* Video device state (0x1B8 bytes). */
struct VideoState {
    u8 unk0;
    u8 pad0[0x18B];
    s32 unk18C;
    s32 unk190;
    s32 unk194;
    s32 unk198;
    s32 unk19C;
    s32 unk1A0;
    u8 pad1[0x10];
    s32 unk1B4;
};

/* Callbacks referenced by the driver structures below. */
int sub_00000320(int arg1 __attribute__((unused)), int arg2 __attribute__((unused)), int arg3 __attribute__((unused)));
int sub_00000460(int arg1 __attribute__((unused)), int arg2 __attribute__((unused)), int arg3 __attribute__((unused)));
int sub_0000056C(int arg1 __attribute__((unused)), int arg2 __attribute__((unused)), int arg3 __attribute__((unused)));
int sub_00000610(int arg1 __attribute__((unused)), int arg2 __attribute__((unused)), int arg3 __attribute__((unused)));
int sub_00000694(int arg1 __attribute__((unused)), int arg2 __attribute__((unused)),
                 struct DeviceRequest *req __attribute__((unused)));
int sub_0000086C(int size __attribute__((unused)), void *args __attribute__((unused)));
int sub_00000C7C(int size __attribute__((unused)), void *args __attribute__((unused)));
int sub_00000ED0(int size __attribute__((unused)), void *args __attribute__((unused)));
int sub_00000FEC(int size __attribute__((unused)), void *args __attribute__((unused)));
int sub_00002F78(int speed __attribute__((unused)), void *arg2 __attribute__((unused)),
                 void *arg3 __attribute__((unused)));
int sub_00002FDC(int speed __attribute__((unused)), void *arg2 __attribute__((unused)),
                 void *arg3 __attribute__((unused)));
int sub_0000307C(int arg1 __attribute__((unused)), int arg2 __attribute__((unused)), int arg3 __attribute__((unused)));
int sub_00003084(int arg1 __attribute__((unused)), int arg2 __attribute__((unused)), int arg3 __attribute__((unused)));

s32 sub_00007C54(int arg0 __attribute__((unused)), int arg1 __attribute__((unused)));

/* sceUsbAcc_internal imports (provided by usbacc's exports). */
s32 sceUsbAccGetInfo(u64 *arg);
s32 sceUsbAccRegisterType(u16 type);
s32 sceUsbAccUnregisterType(u16 type);

/* Not declared in uofw headers. */
int sceUsbbdReqRecv(struct UsbdDeviceReq *req);
int sceKernelCancelSema(SceUID semaid, int signal, int *pcount);

/* Thread entries and request-completion callbacks (later batches). */
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
   sub_00004298; field-for-field PspUsbCamSetupVideoExParam. Field +0 is
   never written here (see batchB1_NOTES.md). */
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

/* sceUsbAcc_internal import (NID 0x79A1C743); the original prx imports
   it (usbcam_imps.txt:144). uOFW's usbacc.c:156 declares it too, no
   uofw header does - same local-decl rule as batchA1. */
s32 sceUsbAccGetAuthStat(void);

/* 0x4298, implemented by a later batch; fills out's 32 bytes from req.
   Replaces gen's `s32 sub_00004298(void)` stub - batchB1_NOTES.md. */
int sub_00004298(u8 *out, struct UsbCamVideoReq *req);

/* rodata maps (original module addresses in the comments). */
/* 0x8CB8: bucket map read through a stack copy by sub_000010B8. */
static const u8 s_map8CB8[4] = { 0, 1, 2, 3 };
/* 0x8DE0: index map, sub_00001110 and the sceUsbCamSetupVideo tail. */
static const u8 s_map8DE0[10] = { 6, 5, 4, 3, 8, 7, 2, 1, 9, 0 };
/* 0x8DEC: separate object, same contents as s_map8DE0 (sub_0000115C). */
static const u8 s_map8DEC[10] = { 6, 5, 4, 3, 8, 7, 2, 1, 9, 0 };
/* 0x8DF8: identity {0..7} searched by the sceUsbCamSetupVideo tail. */
static const u8 s_map8DF8[8] = { 0, 1, 2, 3, 4, 5, 6, 7 };
/* 0x8E0C: {0,1,2} searched against g_videoState[0x16]. */
static const u8 s_map8E0C[3] = { 0, 1, 2 };
/* 0x8E28: identity {0..0x10} (sub_000011A8). */
static const u8 s_map8E28[17] = {
    0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16
};
/* 0x8E3C: PspUsbCamResolution -> PspUsbCamResolutionEx, byte pairs,
   only the odd byte of each pair is read (lb at +2*i+1). */
static const s8 s_map8E3C[20] = {
    0, 0, 1, 1, 2, 2, 3, 3, 6, 6, 9, 7, 9, 8, 6, 5, 6, 4, 0, 0
};
/* 0x8E78: 10x10 signed level table for sub_000011F4 (-1 = not usable). */
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

/* NID 0xEDA8A020, sceUsbBus_driver import (usbcam_imps.txt:137);
   psplibdoc_usb.csv:28/40 matches the NID to sceUsbRestart, no uofw
   header declares it - local decl, prototype guessed (batchC1_NOTES.md). */
int sceUsbRestart(int arg);

/* include/interruptman.h:150-151 verbatim; gen_usbcam.c does not pull
   interruptman.h in, so the fragment declares them itself. Harmless if the
   merge adds the header (identical redeclaration). */
s32 sceKernelCpuSuspendIntr(void);
void sceKernelCpuResumeIntr(s32 intr);

/* batch D5 additions */

/* Redeclared from gen (lines 219-223, 1487 and 2209-2210) so the fragment
   also splices standalone: later functions in this window call the earlier
   ones and sub_00004164 sits at the very end of the window. Identical
   redeclarations are legal C; every prototype below matches gen's. */
s32 sub_00003324(void);
s32 sub_000033E8(void);
s32 sub_000034B8(void);
s32 sub_000035F4(void *buf, int size);
s32 sub_000036B4(SceSize args, void *argp);
s32 sub_00003AA0(SceSize args, void *argp);
s32 sub_00003CD4(SceSize args, void *argp);
void sub_00003E94(struct UsbdDeviceReq *req);
void sub_00003FEC(struct UsbdDeviceReq *req);
s32 sub_00004164(void);

/* gen defines both of these much later than sub_000036B4 (sub_00004C00 at
   line 2946, sub_00004F04 at line 3061) and nothing before them called
   either; the prototypes have to precede that caller. */
s32 sub_00004C00(int arg);
s32 sub_00004F04(int arg);

/* Same situation: gen has no prototype for sub_00004A24 before its
   definition at line 2896, and this window is the first caller. The
   signature below is gen's, verbatim.

   sub_000080F8: the asm at 0x3E38 passes two arguments (destination and
   length); gen's stub was retyped to the two-argument form during the
   batch D5 merge. */
void *sub_00004A24(void *dst, const void *src, int size);
s32 sub_000080F8(void *dst, int size);

/* batch D7 additions */

/* sceUsbAcc_internal import (NID 0x2A100C1F); no uofw header declares it,
   same local-decl rule as sceUsbAccGetAuthStat (gen line 168). The original
   prx imports it (usbcam_imps.txt). */
s32 sceUsbAccIntrInReq(struct UsbdDeviceReq *req);

/* gen defines all five below this window (lines 4118, 4123, 4128, 4133 and
   4161) but never declares them; the earlier functions in this window need
   them. sub_00006DA4 is NOT redeclared here - gen's prototype at line 2575
   must be edited in place (see the merge checklist). */
s32 sub_000078A0(void *cmd, int flag, void *workarea, int wasize);
s32 sub_000079A0(void);
s32 sub_00007A3C(void);
s32 sub_00007AF8(void *buf, int size);
/* batch D6 additions - forward declarations the fragment needs.
   sub_0000808C is defined below its only caller (sub_00007CB0), and gen
   has no prototype for it; splice next to gen's forward-decl anchor
   (s32 sub_00007F0C(void); gen line 270). */
void *sub_0000808C(void *dst, void *src, int size);

s32 sub_00007F0C(void);

/* gen only defines MIC_BYTE / MIC_WORD (lines 553-554); the mic gain slot
   at +0x0A needs the u16 accessor gen has as VIDEO_HALF (line 557). */
#define MIC_HALF(off) (*(u16 *)((u8 *)&g_micState + (off)))

/* Mic counterparts of gen's UsbCamSetupVideoParam (line 119): the SDK spells
   them PspUsbCamSetupMicParam / PspUsbCamSetupMicExParam, which gen does not
   include. The kernel reads param..param+20 / param..param+36. Field +0 is
   never read here. */
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

s32 sub_00003324(void);
s32 sub_00004164(void);
s32 sub_000036B4(SceSize args, void *argp);
s32 sub_00003AA0(SceSize args, void *argp);
s32 sub_00003CD4(SceSize args, void *argp);
void sub_000030A4(struct UsbdDeviceReq *req);
void sub_000030C8(struct UsbdDeviceReq *req);
void sub_000030D0(struct UsbdDeviceReq *req);
void sub_00003E94(struct UsbdDeviceReq *req);
void sub_00003FEC(struct UsbdDeviceReq *req);
void sub_00007CB0(struct UsbdDeviceReq *req);
s32 sub_00007C8C(int arg0 __attribute__((unused)), int arg1 __attribute__((unused)));

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

/* Isochronous IN endpoint descriptor (7 bytes) + padding. */
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
    .recvctl = sub_00000694,
    .func28 = sub_00000320,
    .attach = sub_00002F78,
    .detach = sub_0000056C,
    .unk34 = (s32)(long)sub_0000307C,
    .start_func = sub_0000086C,
    .stop_func = sub_00000ED0,
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

/* Interface 1, alternate setting 0 (control). */
struct UsbIfDescEntry g_micIfCtrl = {
    { 0x09, 0x04, 0x00, 0x00, 0x00, 0x01, 0x01, 0x00, 0x01 },
    NULL,
    g_micCsDescs1,
    0x1E
};

/* Interface 1, alternate setting 0 of the streaming interface. */
struct UsbIfDescEntry g_micIfAlt1 = {
    { 0x09, 0x04, 0x01, 0x00, 0x00, 0x01, 0x02, 0x00, 0x01 },
    NULL,
    NULL,
    0
};

/* Interface 1, alternate setting 1 (isochronous streaming). */
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
    .func28 = sub_00000460,
    .attach = sub_00002FDC,
    .detach = sub_00000610,
    .unk34 = (s32)(long)sub_00003084,
    .start_func = sub_00000C7C,
    .stop_func = sub_00000FEC,
    .link = NULL
};

/*
 * Device states (.bss, laid out as in the original: microphone state at
 * 0x93D0, video state at 0x94FC).
 */
struct MicState g_micState = { 0 };
struct VideoState g_videoState = { 0 };

struct VideoDescState {
    void *unk0;
    int unk4;
    int unk8;
    void *unkC;
};

/* Full view of g_videoState (0x1B8 bytes). Offsets 0x18C..0x1A4 map to
   gen's VideoState fields unk18C..unk1A0 (fplId/eventflag/sema/mutex/
   thread1/thread2) plus unk1A4 here. */
struct VideoStateFull {
    u8 unk0;
    u8 unk1;
    u8 unk2;
    u8 unk3;
    u8 unk4;
    u8 unk5;
    u8 pad06[2];
    u32 unk8;
    u8 pad0C[0x38];
    struct UsbdDeviceReq reqB;
    void *unk6C;
    struct DeviceRequest setup;
    struct UsbdDeviceReq reqA;
    void *unkA0;
    struct UsbdDeviceReq items[2];
    void *frameBufs[2];
    struct UsbdDeviceReq reqC;
    void *unk124;
    u8 pad128[8];
    struct VideoDescState desc[2];
    u8 pad150[0x1C];
    s32 unk16C;
    s32 unk170;
    u8 pad174[0x14];
    sceKernelDmaOperation *unk188;
    s32 fplId;
    s32 eventflag;
    s32 sema;
    s32 mutex;
    s32 thread1;
    s32 thread2;
    s32 unk1A4;
    u8 pad1A8[0xC];
    s32 unk1B4;
};

_Static_assert(sizeof(struct VideoStateFull) == 0x1B8, "VideoStateFull size");

/* Full view of g_micState (0x12C bytes). */
struct MicStateFull {
    u8 unk0;
    u8 unk1;
    u8 unk2;
    u8 unk3;
    u8 unk4;
    u8 unk5;
    u8 pad06[2];
    u32 unk8;
    u32 unkC;
    u32 unk10;
    u16 unk14;
    u8 pad16[0x22];
    struct UsbdDeviceReq reqD;
    void *unk60;
    struct UsbdDeviceReq reqs[4];
    void *bufs[4];
    s32 eventflag;
    s32 fplId;
    s32 thread;
    u8 pad120[8];
    s32 unk128;
};

_Static_assert(sizeof(struct MicStateFull) == 0x12C, "MicStateFull size");

/* (bmRequestType, bRequest) pairs, table at 0x8EDC. */
static const u8 g_ctlRequests[6][8] = {
    { 0xC1, 0x03 },
    { 0xC1, 0x06 },
    { 0xC1, 0x08 },
    { 0x41, 0x07 },
    { 0x41, 0x09 },
    { 0x41, 0x0A }
};

/* State members not named in struct MicState / struct VideoState yet. */
#define MIC_BYTE(off)   (((u8 *)&g_micState)[(off)])
#define MIC_WORD(off)   (*(u32 *)((u8 *)&g_micState + (off)))
#define VIDEO_BYTE(off) (((u8 *)&g_videoState)[(off)])
#define VIDEO_WORD(off) (*(u32 *)((u8 *)&g_videoState + (off)))
#define VIDEO_HALF(off) (*(u16 *)((u8 *)&g_videoState + (off)))

/* 0x00000000 sub_00000000 */
int sub_00000000(int cmd, int arg1, void *buf, int len)
{
    struct VideoStateFull *st = (struct VideoStateFull *)&g_videoState;
    u8 *cb;
    s32 res;

    if (st->unk2 == 0)
        return 0x80243902;
    res = sceKernelWaitSema(st->sema, 1, NULL);
    if (res == (s32)0x800201A9)
        return 0;
    if (res < 0)
        return res;
    if (st->reqB.retcode > 0) {
        sceKernelSignalSema(st->sema, 1);
        return 0x80243006;
    }
    cb = st->unk6C;
    cb[2] = cmd;
    *(u16 *)cb = arg1;
    cb[3] = len;
    if (buf != NULL)
        memcpy(cb + 4, buf, len);
    return sceUsbAccIntrInReq(&st->reqB);
}


/* 0x000000F4 sub_000000F4 */
int sub_000000F4(void *buf)
{
    struct VideoStateFull *st = (struct VideoStateFull *)&g_videoState;
    s32 res;

    res = sceKernelLockMutex(st->mutex, 1, NULL);
    if (res >= 0) {
        res = sub_00000000(171, 2, buf, 2);
        sceKernelUnlockMutex(st->mutex, 1);
    }
    return res;
}


/* 0x00000170 sub_00000170 */
int sub_00000170(void *arg)
{
    struct VideoStateFull *st = (struct VideoStateFull *)&g_videoState;
    u32 bits;
    u32 word;
    u8 *cb;
    s32 res;

    res = sceKernelLockMutex(st->mutex, 1, NULL);
    if ((u32)res - 0x800201A9u < 2)
        return 0x80243902;
    if (res < 0)
        return res;

    sceKernelClearEventFlag(st->eventflag, 0xFFFEFFFF);
    res = sub_00000000(43, 2, NULL, 0);
    if (res < 0)
        goto unlock;
    res = sceKernelWaitEventFlag(st->eventflag, 0x10400, 1, &bits, NULL);
    if (res < 0)
        goto unlock;
    if (bits & 0x400) {
        res = 0x80243902;
        goto unlock;
    }

    cb = st->unk6C;
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
        st->unk8 |= 0x200;
    else
        st->unk8 &= ~0x200u;
    word = *(u32 *)arg;
    if (word & 0x100)
        st->unk8 |= 0x100;
    else
        st->unk8 &= ~0x100u;
    word = *(u32 *)arg;
    if (word & 0x10000)
        st->unk8 |= 0x400;
    else
        st->unk8 &= ~0x400u;

unlock:
    sceKernelUnlockMutex(st->mutex, 1);
    return res;
}


int sub_00000320(int arg1 __attribute__((unused)), int arg2, int arg3 __attribute__((unused)))
{
    struct VideoStateFull *st = (struct VideoStateFull *)&g_videoState;
    int ret = 0;

    if (arg1 != 0)
        return ret;
    if (arg2 == 0) {
        if (st->unk3 == 1) {
            if (st->unk8 & 0x18) {
                sceKernelClearEventFlag(st->eventflag, 0xFFF7FFFF);
                ret = sceKernelSetEventFlag(st->eventflag, 0x100);
                st->unk8 &= ~0x18;
            } else {
                st->unk5 = 0;
                sceKernelClearEventFlag(st->eventflag, 0xFFFFFFEF);
                ret = sceKernelSetEventFlag(st->eventflag, 0x100);
            }
        } else {
            ret = sceKernelClearEventFlag(st->eventflag, 0xFFFFCEFF);
            st->unk16C = 0;
            st->unk170 = 0;
            if ((u32)st->unk1B4 < 1280) {
                sceKernelDcacheInvalidateRange(st->unk124, 128);
                ret = sceUsbbdReqRecv(&st->reqC);
            }
        }
    } else if (arg2 == 1) {
        sceKernelClearEventFlag(st->eventflag, 0xFFFFFEF9);
        if (st->unk8 & 8) {
            st->unk16C = 0;
            st->unk170 = 0;
            sceKernelSetEventFlag(st->eventflag, 0x80000);
        } else {
            sceKernelSetEventFlag(st->eventflag, 0x10);
        }
        ret = sub_00004164();
    }
    st->unk3 = (u8)arg2;
    return ret;
}

int sub_00000460(int arg1, int arg2, int arg3 __attribute__((unused)))
{
    struct MicStateFull *st = (struct MicStateFull *)&g_micState;
    int ret = 0;
    int i;

    if (arg1 != 1)
        return ret;
    if (arg2 == 0) {
        sceKernelClearEventFlag(st->eventflag, 0xFFFFFFEF);
        ret = sceKernelSetEventFlag(st->eventflag, 32);
        st->unk4 = 0;
    } else {
        sceKernelClearEventFlag(st->eventflag, 0xFFFFFFDE);
        sceKernelSetEventFlag(st->eventflag, 16);
        for (i = 0; i < 4; i++) {
            memset(st->bufs[i], 0, 256);
            sceKernelDcacheInvalidateRange(st->bufs[i], 256);
            st->reqs[i].unk1c = (int)&st->reqs[i + 1];
        }
        st->reqs[3].unk1c = 0;
        sceUsbbdReqRecv(&st->reqs[0]);
        ret = 1;
        st->unk4 = 1;
    }
    st->unk3 = (u8)arg2;
    return ret;
}

int sub_0000056C(int arg1 __attribute__((unused)), int arg2 __attribute__((unused)), int arg3 __attribute__((unused)))
{
    struct VideoStateFull *st = (struct VideoStateFull *)&g_videoState;
    int i;

    if (st->unk2 == 0)
        return 0;
    st->unk4 = 2;
    st->unk1 = 0;
    st->unk2 = 0;
    st->unk3 = 0;
    st->unk8 = 0;
    for (i = 0; i < 3; i++)
        g_videoEndpoints[i].transferred = 0;
    sceKernelClearEventFlag(st->eventflag, 0);
    sceKernelSetEventFlag(st->eventflag, 1024);
    sceKernelCancelSema(st->sema, 1, NULL);
    return sceKernelCancelMutex(st->mutex, 0, NULL);
}

int sub_00000610(int arg1 __attribute__((unused)), int arg2 __attribute__((unused)), int arg3 __attribute__((unused)))
{
    struct MicStateFull *st = (struct MicStateFull *)&g_micState;
    int i;

    if (st->unk2 == 0)
        return 0;
    st->unk4 = 5;
    st->unk2 = 0;
    st->unk3 = 0;
    for (i = 0; i < 2; i++)
        g_micEndpoints[i].transferred = 0;
    sceKernelClearEventFlag(st->eventflag, 0);
    sceKernelSetEventFlag(st->eventflag, 256);
    st->unk128 = 0;
    /* Original returns 0x10000 (lui residue of the g_micState address
       materialized for the store above), not 0; see batchA1_NOTES.md. */
    return 0;
}

int sub_00000694(int arg1 __attribute__((unused)), int arg2, struct DeviceRequest *req)
{
    struct VideoStateFull *st = (struct VideoStateFull *)&g_videoState;
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
        blk = (u8 *)st->unkA0;
        if (req->bRequest == 3) {
            st->reqA.data = blk;
            blk[4] = 2;
            blk[5] = 0;
            blk[0] = 0;
            blk[1] = 0;
            blk[2] = 0;
            blk[3] = 0;
            st->reqA.size = 6;
        } else if (req->bRequest == 8) {
            st->reqA.data = blk;
            blk[0] = 1;
            blk[1] = 0;
            blk[2] = 0;
            blk[3] = 0;
            blk[4] = 0;
            blk[5] = 0;
            blk[6] = 0;
            blk[7] = 0;
            st->reqA.size = 8;
        } else {
            return 0;
        }
        sceKernelDcacheWritebackRange(st->reqA.data, st->reqA.size);
        res = sceUsbbdReqSend(&st->reqA);
        if (res < 0)
            Kprintf("%sin %s : Cannot issue send request : 0x%08x\n", "", "DevReqHdlr", res);
        return 0;
    }
    if (req->bRequest == 7 || req->bRequest == 9 ||
        (req->bRequest == 10 && st->setup.wValue == 16)) {
        st->reqA.retcode = 0;
        st->reqA.size = req->wLength;
        st->reqA.data = st->unkA0;
        sceKernelDcacheInvalidateRange(st->reqA.data, 128);
        sceUsbbdReqRecv(&st->reqA);
    }
    return 0;
}

int sub_0000086C(int size __attribute__((unused)), void *args __attribute__((unused)))
{
    struct VideoStateFull *st = (struct VideoStateFull *)&g_videoState;
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
    st->unkA0 = block;
    st->frameBufs[0] = (u8 *)block + 128;
    st->frameBufs[1] = (u8 *)block + 1920;
    st->unk6C = (u8 *)block + 3840;
    st->unk124 = (u8 *)block + 3712;
    for (i = 0; i < 2; i++) {
        st->desc[i].unk0 = (u8 *)block + 3904 + i * 896;
        st->desc[i].unkC = (void *)((u8 *)st + 0x140 + i * 16);
        st->desc[i].unk4 = 0;
    }
    st->desc[1].unkC = &st->desc[0];
    res = sceKernelCreateThread("SceUsbCam", sub_000036B4, 17, 1024, 0x100001, NULL);
    st->thread1 = res;
    if (res < 0)
        goto fail;
    res = sceKernelCreateThread("SceUsbCamCopyWorker", sub_00003AA0, 17, 1024, 0x100001, NULL);
    if (res < 0) {
        st->thread2 = -1;
        goto fail;
    }
    st->thread2 = res;
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
    st->reqA.data = st->unkA0;
    st->reqB.data = st->unk6C;
    st->reqC.data = st->unk124;
    st->reqC.endp = &g_videoEndpoints[1];
    st->reqA.unkc = 1;
    st->reqA.func = sub_00003E94;
    st->reqA.retcode = 0;
    st->reqB.func = sub_000030A4;
    st->reqC.size = 64;
    st->reqA.unk1c = 0;
    st->reqA.arg = NULL;
    st->reqA.recvsize = 0;
    st->reqB.retcode = 0;
    st->reqA.size = 64;
    st->reqB.unkc = 0;
    st->reqB.unk1c = 0;
    st->reqB.arg = NULL;
    st->reqB.recvsize = 0;
    st->reqC.func = sub_00003FEC;
    st->reqC.retcode = 0;
    st->reqA.endp = &g_videoEndpoints[0];
    st->reqB.endp = NULL;
    st->reqB.size = 64;
    st->reqC.unkc = 0;
    st->reqC.unk1c = 0;
    st->reqC.arg = NULL;
    st->reqC.recvsize = 0;
    for (i = 0; i < 2; i++) {
        st->items[i].data = st->frameBufs[i];
        st->items[i].endp = &g_videoEndpoints[2];
        st->items[i].size = 896;
        st->items[i].unkc = 1;
        st->items[i].func = sub_000030D0;
        st->items[i].unk1c = 0;
        st->items[i].arg = NULL;
        st->items[i].recvsize = 0;
        st->items[i].retcode = 0;
    }
    if (sceUsbAccRegisterType(2) < 0)
        goto fail;
    st->unk1A4 = -1;
    sceKernelStartThread(st->thread1, 0, NULL);
    sceKernelStartThread(st->thread2, 0, NULL);
    st->unk0 = 1;
    st->unk188 = sceKernelDmaOpAlloc();
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
    if (st->thread2 > 0) {
        sceKernelDeleteThread(st->thread2);
        st->thread2 = -1;
    }
    if (st->thread1 > 0) {
        sceKernelDeleteThread(st->thread1);
        st->thread1 = -1;
        if (st->fplId > 0) {
            sceKernelDeleteFpl(st->fplId);
            st->fplId = -1;
        }
    }
    return -1;
}

int sub_00000C7C(int size __attribute__((unused)), void *args __attribute__((unused)))
{
    struct MicStateFull *st = (struct MicStateFull *)&g_micState;
    void *block;
    int res;
    int i;

    res = sceKernelCreateFpl("SceUsbMic", 1, 256, 1088, 1, NULL);
    st->fplId = res;
    if (res < 0)
        return -1;
    if (sceKernelTryAllocateFpl(st->fplId, &block) < 0)
        goto delFpl;
    st->unk60 = block;
    for (i = 0; i < 4; i++)
        st->bufs[i] = (u8 *)block + 64 + i * 256;
    res = sceKernelCreateThread("SceUsbMicCopyWorker", sub_00003CD4, 16, 1024, 0x100001, NULL);
    st->thread = res;
    if (res < 0)
        goto delFpl;
    res = sceKernelCreateEventFlag("SceUsbMic", 513, 256, NULL);
    st->eventflag = res;
    if (res < 0)
        goto delThread;
    st->reqD.endp = NULL;
    st->reqD.func = sub_000030C8;
    st->reqD.unkc = 0;
    st->reqD.unk1c = 0;
    st->reqD.arg = NULL;
    st->reqD.recvsize = 0;
    st->reqD.retcode = 0;
    st->reqD.data = block;
    st->reqD.size = 64;
    for (i = 0; i < 4; i++) {
        st->reqs[i].data = st->bufs[i];
        st->reqs[i].endp = &g_micEndpoints[1];
        st->reqs[i].size = 128;
        st->reqs[i].unkc = 0;
        st->reqs[i].func = sub_00007CB0;
        st->reqs[i].unk1c = 0;
        st->reqs[i].arg = NULL;
        st->reqs[i].recvsize = 0;
        st->reqs[i].retcode = 0;
    }
    st->unk4 = 5;
    st->unk8 = 0;
    st->unk14 = 0;
    st->unk1 = 0;
    st->unk2 = 0;
    st->unk3 = 0;
    st->unk5 = 0;
    st->unkC = 0;
    st->unk10 = 0;
    for (i = 0; i < 2; i++)
        g_micEndpoints[i].transferred = 0;
    *(u16 *)st->unk60 = 1;
    if (sceUsbAccRegisterType(1) < 0)
        goto delEventFlag;
    st->unk128 = 0;
    st->unk0 = 1;
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

int sub_00000ED0(int size __attribute__((unused)), void *args __attribute__((unused)))
{
    struct VideoStateFull *st = (struct VideoStateFull *)&g_videoState;

    sceUsbAccUnregisterType(2);
    sceKernelSetEventFlag(st->eventflag, 0x4000);
    if (st->thread2 > 0) {
        sceKernelWaitThreadEnd(st->thread2, NULL);
        sceKernelDeleteThread(st->thread2);
        st->thread2 = -1;
    }
    if (st->thread1 > 0) {
        sceKernelWaitThreadEnd(st->thread1, NULL);
        sceKernelDeleteThread(st->thread1);
        st->thread1 = -1;
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
    if (st->unk188 != NULL)
        sceKernelDmaOpFree(st->unk188);
    st->unk0 = 0;
    return 0;
}

int sub_00000FEC(int size __attribute__((unused)), void *args __attribute__((unused)))
{
    struct MicStateFull *st = (struct MicStateFull *)&g_micState;

    sceUsbAccUnregisterType(1);
    sceKernelSetEventFlag(st->eventflag, 512);
    sceKernelWaitThreadEnd(st->thread, NULL);
    sceKernelDeleteThread(st->thread);
    sceKernelDeleteEventFlag(st->eventflag);
    sceKernelDeleteFpl(st->fplId);
    st->unk0 = 0;
    return 0;
}

int sub_00001058(int arg0)
{
    int x = (arg0 + 128) & 0xFF;

    if ((s8)x < 0)
        x = ((~x) | -128) & 0xFF;
    return x;
}

int sub_00001084(int arg0)
{
    if (arg0 < 0)
        return 0;
    if (arg0 >= 256)
        return 5;
    return arg0 / 42;
}

int sub_000010B8(int arg0)
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

int sub_00001110(u8 *arg0)
{
    int i;
    u8 v = *arg0;

    for (i = 0; i < 10; i++) {
        if (s_map8DE0[i] == v)
            break;
    }
    return (i < 10) ? i : 0;
}

int sub_0000115C(u8 *arg0)
{
    int i;
    u8 v = *arg0;

    for (i = 0; i < 10; i++) {
        if (s_map8DEC[i] == v)
            break;
    }
    return (i < 10) ? i : 0;
}

int sub_000011A8(u8 *arg0)
{
    int i;
    u8 v = *arg0;

    for (i = 0; i < 17; i++) {
        if (s_map8E28[i] == v)
            break;
    }
    return (i < 17) ? i : 0;
}

int sub_000011F4(int arg0, int arg1, int arg2)
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
    ret = 0x80243908;
    if (g_videoState.unk0 == 0)
        goto out;
    ret = 0x80243902;
    if (sceUsbAccGetAuthStat() < 0)
        goto out;
    ret = 0x80243907;
    if (param == NULL)
        goto out;
    ret = 0x80243904;
    if (!pspK1StaBufOk(param, 52))
        goto out;
    if (!pspK1DynBufOk(workarea, wasize))
        goto out;
    ret = 0x80243903;
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
        if (s_map8E0C[i] == VIDEO_BYTE(0x16))
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
        ret = 0x80243907;
        goto out;
    }

    ret = sub_00004298((u8 *)out, &req);
    if (ret < 0)
        goto out;

    for (i = 0; i < 8; i++)
        VIDEO_WORD(0x0C + i * 4) = out[i];
    VIDEO_WORD(8) = VIDEO_WORD(8) & ~7u;
    ret = sub_00001110(&VIDEO_BYTE(0x32));
    if (ret < 7) {
        u8 v = VIDEO_BYTE(0x0E);

        for (i = 0; i < 8; i++) {
            if (s_map8DF8[i] == v)
                break;
        }
        if (i == 8)
            i = 7;
        VIDEO_BYTE(0x32) = s_map8DE0[(i < 5) ? 9 : 6];
    }
    ret = 0;
    VIDEO_BYTE(1) |= 1;
    half = (u32)wasize >> 1;
    VIDEO_WORD(0x160) = (u32)workarea + half;
    VIDEO_WORD(0x150) = (u32)workarea;
    VIDEO_WORD(0x154) = half;
    VIDEO_WORD(0x158) = (u32)workarea;
    VIDEO_WORD(0x15C) = 0;
    VIDEO_WORD(0x164) = 0;

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
    ret = 0x80243908;
    if (g_videoState.unk0 == 0)
        goto out;
    ret = 0x80243902;
    if (sceUsbAccGetAuthStat() < 0)
        goto out;
    ret = 0x80243907;
    if (param == NULL)
        goto out;
    ret = 0x80243904;
    if (!pspK1StaBufOk(param, 100))
        goto out;
    if (!pspK1DynBufOk(workarea, wasize))
        goto out;
    ret = 0x80243903;
    if ((wasize & 0x3F) != 0)
        goto out;

    ret = sub_00004298((u8 *)out, param);
    if (ret < 0)
        goto out;

    for (i = 0; i < 8; i++)
        VIDEO_WORD(0x0C + i * 4) = out[i];
    half = (u32)wasize >> 1;
    VIDEO_WORD(0x160) = (u32)workarea + half;
    VIDEO_WORD(8) = VIDEO_WORD(8) & ~7u;
    VIDEO_BYTE(1) |= 1;
    ret = 0;
    VIDEO_WORD(0x150) = (u32)workarea;
    VIDEO_WORD(0x154) = half;
    VIDEO_WORD(0x158) = (u32)workarea;
    VIDEO_WORD(0x15C) = 0;
    VIDEO_WORD(0x164) = 0;

out:
    pspSetK1(oldK1);
    return ret;
}

int sceUsbCamReadVideoFrame(u8 *buf, SceSize size)
{
    int oldK1;
    int ret;

    oldK1 = pspShiftK1();
    ret = 0x80243908;
    if (g_videoState.unk0 == 0)
        goto out;
    ret = 0x80243902;
    if (VIDEO_BYTE(2) == 0)
        goto out;
    if (sceUsbAccGetAuthStat() < 0)
        goto out;
    ret = 0x80243901;
    if ((VIDEO_BYTE(1) & 1) == 0)
        goto out;
    ret = 0x80243907;
    if (buf == NULL)
        goto out;
    ret = 0x80243904;
    if (!pspK1DynBufOk(buf, size))
        goto out;
    ret = 0x8024390C;
    if (VIDEO_BYTE(4) == 0)
        goto out;
    ret = 0x80243909;
    if (VIDEO_WORD(8) & 4)
        goto out;

    VIDEO_WORD(8) |= 4;
    sceKernelClearEventFlag(g_videoState.unk190, ~0x80u);
    VIDEO_WORD(0x1AC) = size;
    VIDEO_WORD(0x1B0) = (u32)buf;
    ret = sceKernelSetEventFlag(g_videoState.unk190, 0x40);

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
    ret = 0x80243908;
    if (g_videoState.unk0 == 0)
        goto out;
    ret = 0x80243902;
    if (VIDEO_BYTE(2) == 0)
        goto out;
    if (sceUsbAccGetAuthStat() < 0)
        goto out;
    ret = 0x80243901;
    if ((VIDEO_BYTE(1) & 1) == 0)
        goto out;
    ret = 0x8024390C;
    if ((VIDEO_WORD(8) & 4) == 0)
        goto out;

    ret = sceKernelWaitEventFlag(g_videoState.unk190, 0x480, 1, &outBits, &timeout);
    if (ret < 0) {
        if ((u32)ret == SCE_ERROR_KERNEL_WAIT_TIMEOUT) {
            sceUsbRestart(1000000);
            ret = 0x80243902;
        }
        goto out;
    }
    VIDEO_WORD(8) &= ~4u;
    if (outBits & 0x400) {
        ret = 0x80243902;
        goto out;
    }
    ret = VIDEO_WORD(0x180);

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
    ret = 0x80243908;
    if (g_videoState.unk0 == 0)
        goto out;
    ret = 0x80243902;
    if (VIDEO_BYTE(2) == 0)
        goto out;
    if (sceUsbAccGetAuthStat() < 0)
        goto out;
    ret = 0x80243901;
    if ((VIDEO_BYTE(1) & 1) == 0)
        goto out;
    ret = 0x8024390C;
    if ((VIDEO_WORD(8) & 4) == 0)
        goto out;

    ret = sceKernelPollEventFlag(g_videoState.unk190, 0x480, 1, &outBits);
    if ((u32)ret == SCE_ERROR_KERNEL_EVENT_FLAG_POLL_FAILED) {
        ret = 0x8024390E;
        goto out;
    }
    if (ret < 0)
        goto out;
    VIDEO_WORD(8) &= ~4u;
    if (outBits & 0x400) {
        ret = 0x80243902;
        goto out;
    }
    ret = VIDEO_WORD(0x180);

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
    ret = 0x80243908;
    if (g_videoState.unk0 == 0)
        goto out;
    ret = 0x80243902;
    if (VIDEO_BYTE(2) == 0)
        goto out;
    if (sceUsbAccGetAuthStat() < 0)
        goto out;
    ret = 0x80243901;
    if ((VIDEO_BYTE(1) & 1) == 0)
        goto out;
    ret = 0x80243907;
    if (buf == NULL)
        goto out;
    ret = 0x80243904;
    if (!pspK1DynBufOk(buf, size))
        goto out;
    ret = 0x80243909;
    if (VIDEO_WORD(8) & 4)
        goto out;

    VIDEO_WORD(8) |= 4;
    sceKernelClearEventFlag(g_videoState.unk190, ~0x80u);
    VIDEO_WORD(0x1B0) = (u32)buf;
    VIDEO_WORD(0x1AC) = size;
    sceKernelSetEventFlag(g_videoState.unk190, 0x40);
    ret = sceKernelWaitEventFlag(g_videoState.unk190, 0x480, 1, &outBits, NULL);
    if (ret < 0)
        goto out;
    VIDEO_WORD(8) &= ~4u;
    if (outBits & 0x400) {
        ret = 0x80243902;
        goto out;
    }
    ret = VIDEO_WORD(0x180);

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
    ret = 0x80243908;
    if (g_videoState.unk0 == 0)
        goto out;
    ret = 0x80243902;
    if (VIDEO_BYTE(2) == 0)
        goto out;
    if (sceUsbAccGetAuthStat() < 0)
        goto out;
    ret = 0x80243901;
    if ((VIDEO_BYTE(1) & 1) == 0)
        goto out;

    intr = sceKernelCpuSuspendIntr();
    ret = VIDEO_WORD(0x184);
    sceKernelCpuResumeIntr(intr);

out:
    pspSetK1(oldK1);
    return ret;
}

/* batch D1 additions */

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
   same 15-int layout as gen's struct UsbCamStillReq (batchC2_NOTES.md maps
   caller field -> UsbCamStillReq field); sceUsbCamSetupStill builds one of
   these on the stack and both entry points hand it to sub_00004680 through
   that type. */
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

/* Helpers defined below this window in gen_usbcam.c; batch D1 sits above
   them (the six stubs are at gen line 1439+), so the prototypes are
   restated here. struct UsbCamStillReq is only forward-declared: gen
   completes it further down (line 1922), so the two sub_00004680 call sites
   cast from the layout-identical UsbCamSetupStillExParam.
   sub_000035F4's gen stub is still `s32 sub_000035F4(void)` - it must be
   retyped in the merge step, see batchD1_NOTES.md checklist item 1. */
struct UsbCamStillReq;
int sub_00004680(u8 *out, struct UsbCamStillReq *req);
s32 sub_00004858(void);
s32 sub_000048DC(int arg);
s32 sub_000035F4(void *buf, int size);

s32 sceUsbCamSetupStill(struct UsbCamSetupStillParam *param)
{
    struct UsbCamSetupStillExParam req;
    u8 out[20];
    int oldK1;
    int ret;
    u8 v;
    int i;

    oldK1 = pspShiftK1();
    ret = 0x80243908;
    if (g_videoState.unk0 == 0)
        goto out;
    ret = 0x80243902;
    if (VIDEO_BYTE(2) == 0)
        goto out;
    if (sceUsbAccGetAuthStat() < 0)
        goto out;
    ret = 0x80243907;
    if (param == NULL)
        goto out;
    ret = 0x80243904;
    if (!pspK1StaBufOk(param, 24))
        goto out;
    ret = 0x80243907;
    if ((u32)(param->size - 20) >= 5)
        goto out;

    v = VIDEO_BYTE(0x0E);
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

    ret = sub_00004680(out, (struct UsbCamStillReq *)&req);
    if (ret < 0)
        goto out;
    __builtin_memcpy(&VIDEO_BYTE(0x2C), out, 20);
    ret = 0;
    VIDEO_BYTE(1) |= 2;
    VIDEO_WORD(8) &= ~8u;

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
    ret = 0x80243908;
    if (g_videoState.unk0 == 0)
        goto out;
    ret = 0x80243902;
    if (VIDEO_BYTE(2) == 0)
        goto out;
    if (sceUsbAccGetAuthStat() < 0)
        goto out;
    ret = 0x80243907;
    if (param == NULL)
        goto out;
    ret = 0x80243904;
    if (!pspK1StaBufOk(param, 60))
        goto out;

    ret = sub_00004680(out, (struct UsbCamStillReq *)param);
    if (ret < 0)
        goto out;
    __builtin_memcpy(&VIDEO_BYTE(0x2C), out, 20);
    ret = 0;
    VIDEO_BYTE(1) |= 2;
    VIDEO_WORD(8) &= ~8u;

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
    ret = 0x80243908;
    if (g_videoState.unk0 == 0)
        goto out;
    ret = 0x80243902;
    if (VIDEO_BYTE(2) == 0)
        goto out;
    if (sceUsbAccGetAuthStat() < 0)
        goto out;
    ret = 0x80243903;
    if (size < 64)
        goto out;
    ret = 0x80243904;
    if (!pspK1DynBufOk(buf, size))
        goto out;
    ret = 0x80243901;
    if ((VIDEO_BYTE(1) & 2) == 0)
        goto out;

    h = sub_0000115C(&VIDEO_BYTE(0x33));
    w = sub_00001110(&VIDEO_BYTE(0x32));
    if (w < h) {
        ret = 0x80243905;
        goto out;
    }

    sceKernelClearEventFlag(g_videoState.unk190, ~0x3000u);
    ret = sub_000035F4(buf, size);

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
    ret = sub_00004858();
    if (ret >= 0)
        ret = sub_000048DC(1);
    pspSetK1(oldK1);
    return ret;
}

/* 0x2138 sceUsbCamStillWaitInputEnd */

s32 sceUsbCamStillWaitInputEnd(void)
{
    int oldK1;
    int ret;

    oldK1 = pspShiftK1();
    ret = sub_00004858();
    if (ret >= 0)
        ret = sub_000048DC(0);
    pspSetK1(oldK1);
    return ret;
}

/* 0x2174 sceUsbCamStillInputBlocking */

s32 sceUsbCamStillInputBlocking(u8 *buf, SceSize size)
{
    int oldK1;
    int ret;

    oldK1 = pspShiftK1();
    ret = 0x80243908;
    if (g_videoState.unk0 == 0)
        goto out;
    ret = 0x80243902;
    if (VIDEO_BYTE(2) == 0)
        goto out;
    if (sceUsbAccGetAuthStat() < 0)
        goto out;
    ret = 0x80243903;
    if (size < 64)
        goto out;
    ret = 0x80243904;
    if (!pspK1DynBufOk(buf, size))
        goto out;
    ret = 0x80243901;
    if ((VIDEO_BYTE(1) & 2) == 0)
        goto out;

    sceKernelClearEventFlag(g_videoState.unk190, ~0x3000u);
    ret = sub_000035F4(buf, size);
    if (ret >= 0)
        ret = sub_000048DC(0);

out:
    pspSetK1(oldK1);
    return ret;
}

/* batch D2 additions */

/* gen defines both of these much later in the file (sub_00004858 at line
   2127, sub_000055AC at line 2667); the prototypes have to precede these
   callers. */
s32 sub_00004858(void);
s32 sub_000055AC(int cmd, int *arg);

s32 sceUsbCamStillGetInputLength(void)
{
    int oldK1;
    s32 ret;

    oldK1 = pspShiftK1();
    ret = 0x80243908;
    if (g_videoState.unk0 == 0)
        goto out;
    ret = 0x80243902;
    if (VIDEO_BYTE(2) == 0)
        goto out;
    if (sceUsbAccGetAuthStat() < 0)
        goto out;
    ret = 0x80243901;
    if ((VIDEO_BYTE(1) & 2) == 0)
        goto out;
    ret = (VIDEO_WORD(0x174) < VIDEO_WORD(0x170)) ? VIDEO_WORD(0x174)
                                                  : VIDEO_WORD(0x170);

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
    ret = sub_00004858();
    if (ret < 0)
        goto out;
    intr = sceKernelCpuSuspendIntr();
    VIDEO_WORD(0x174) = 0;
    VIDEO_WORD(0x178) = 0;
    VIDEO_WORD(0x17C) = 0;
    sceKernelSetEventFlag(VIDEO_WORD(0x190), 0x2000);
    if (VIDEO_WORD(0x1B4) >= 1280)
        VIDEO_WORD(8) |= 0x10;
    VIDEO_WORD(8) = VIDEO_WORD(8) & ~8u;
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
    ret = 0x80243904;
    if (!pspK1StaBufOk(saturation, 4))
        goto out;
    ret = sub_000055AC(0x80000003, saturation);

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
    ret = 0x80243904;
    if (!pspK1StaBufOk(brightness, 4))
        goto out;
    ret = sub_000055AC(0x80000001, brightness);

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
    ret = 0x80243904;
    if (!pspK1StaBufOk(contrast, 4))
        goto out;
    ret = sub_000055AC(0x80000002, contrast);

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
    ret = 0x80243904;
    if (!pspK1StaBufOk(sharpness, 4))
        goto out;
    ret = sub_000055AC(0x80000004, sharpness);

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
    ret = 0x80243904;
    if (!pspK1StaBufOk(zoom, 4))
        goto out;
    ret = sub_000055AC(0x80000005, zoom);

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
    ret = 0x80243904;
    if (!pspK1StaBufOk(antiflicker, 4))
        goto out;
    ret = sub_000055AC(0x80000010, antiflicker);

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
    ret = 0x80243904;
    if (!pspK1StaBufOk(ev, 4))
        goto out;
    ret = sub_000055AC(0x80000014, ev);

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
    ret = 0x80243904;
    if (!pspK1StaBufOk(reverseflags, 4))
        goto out;
    ret = sub_000055AC(0x80000006, reverseflags);

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
    ret = 0x80243904;
    if (!pspK1StaBufOk(effectmode, 4))
        goto out;
    ret = sub_000055AC(0x80000007, effectmode);

out:
    pspSetK1(oldK1);
    return ret;
}

/* 0x2714 sceUsbCam_00631D06 */

s32 sceUsbCam_00631D06(void)
{
    u32 val;
    s32 ret;

    val = VIDEO_WORD(0x1B4);
    if (val == 0) {
        ret = 0x80243902;
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
    res = sceKernelPollEventFlag(VIDEO_WORD(0x190), 0x400, 1, &bits);
    if (res < 0 && res != (s32)0x800201AF) {
        ret = res;
        goto out;
    }
    if (res >= 0 && (bits & 0x400)) {
        ret = 0x80243902;
        goto out;
    }
    if (VIDEO_WORD(8) & 0x2000)
        ret = ((VIDEO_WORD(8) ^ 0x400) >> 10) & 1;
    else
        ret = 0x80243901;

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
        ret = 0x80243905;
        goto out;
    }
    intr = sceKernelCpuSuspendIntr();
    if (VIDEO_WORD(0x1A4) > 0) {
        VIDEO_WORD(0x1A4) = -1;
        ret = 0x80243910;
    } else {
        VIDEO_WORD(0x1A4) = cbid;
    }
    sceKernelCpuResumeIntr(intr);

out:
    pspSetK1(oldK1);
    return ret;
}

s32 module_start(SceSize args __attribute__((unused)), void *argp __attribute__((unused)))
{
    sub_00007C54(0, 0);
    if (sceUsbbdRegister(&g_videoDriver) < 0) {
        return 1;
    }
    g_videoState.unk0 = 0;
    g_videoState.unk1A0 = -1;
    g_videoState.unk1B4 = 0;
    g_videoState.unk18C = -1;
    g_videoState.unk190 = -1;
    g_videoState.unk194 = -1;
    g_videoState.unk19C = -1;
    return 0;
}

s32 module_stop(SceSize args __attribute__((unused)), void *argp __attribute__((unused)))
{
    if (sceUsbbdUnregister(&g_videoDriver) < 0) {
        return 1;
    }
    sub_00007C8C(0, 0);
    return 0;
}

/* batch D3 additions */

/* gen's batch C2 block defines these four maps (and struct UsbCamResEntry +
   s_res8E50, below) only AFTER this window - it was spliced in front of
   sub_00004298 (gen line 2404 ff) while these definitions sit at the top of
   the window (gen line 2023 ff). These are tentative definitions; the real
   initialized definitions later in the same file complete them (legal C in
   both orders, verified with the project's flags). batchD3_NOTES.md merge
   checklist item 3: gen's duplicate struct definition must be deleted. */
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

int sub_00002930(int val)
{
    return s_map8E10[val];
}

/* 0x2944 sub_00002944 */

int sub_00002944(u8 *p)
{
    int i;
    u8 v = *p;

    for (i = 0; i < 7; i++) {
        if (s_map8E10[i] == v)
            break;
    }
    return (i < 7) ? i : 6;
}

/* 0x2988 sub_00002988 */

int sub_00002988(int val)
{
    return s_map8DF8[val];
}

/* 0x299C sub_0000299C */

int sub_0000299C(u8 *p)
{
    int i;
    u8 v = *p;

    for (i = 0; i < 8; i++) {
        if (s_map8DF8[i] == v)
            break;
    }
    return (i < 8) ? i : 7;
}

/* 0x29E0 sub_000029E0 */

int sub_000029E0(int val)
{
    return s_map8E00[val];
}

/* 0x29F4 sub_000029F4 */

int sub_000029F4(u8 *p)
{
    int i;
    u8 v = *p;

    for (i = 0; i < 4; i++) {
        if (s_map8E00[i] == v)
            break;
    }
    return (i < 4) ? i : 3;
}

/* 0x2A38 sub_00002A38 */

int sub_00002A38(int val)
{
    return s_map8E04[val];
}

/* 0x2A4C sub_00002A4C */

int sub_00002A4C(u8 *p)
{
    int i;
    u8 v = *p;

    for (i = 0; i < 4; i++) {
        if (s_map8E04[i] == v)
            break;
    }
    return (i < 4) ? i : 3;
}

/* 0x2A90 sub_00002A90 */

int sub_00002A90(int val)
{
    return s_map8E0C[val];
}

/* 0x2AA4 sub_00002AA4 */

int sub_00002AA4(u8 *p)
{
    int i;
    u8 v = *p;

    for (i = 0; i < 3; i++) {
        if (s_map8E0C[i] == v)
            break;
    }
    return (i < 3) ? i : 2;
}

/* 0x2AE8 sub_00002AE8 */

int sub_00002AE8(int val)
{
    return s_map8E08[val];
}

/* 0x2AFC sub_00002AFC */

int sub_00002AFC(u8 *p)
{
    int i;
    u8 v = *p;

    for (i = 0; i < 3; i++) {
        if (s_map8E08[i] == v)
            break;
    }
    return (i < 3) ? i : 2;
}

/* 0x2B40 sub_00002B40 */

int sub_00002B40(int val, u8 *out0, u8 *out1)
{
    if ((u32)val >= 9)
        return 0x80243905;
    *out0 = 9;
    *out1 = (u8)s_map8E3C[val * 2 + 1];
    return 0;
}

/* 0x2B7C sub_00002B7C */

int sub_00002B7C(int val0, int val1, u8 *out0, u8 *out1)
{
    s32 wdiff = s_res8E50[val0].w - s_res8E50[val1].w;
    s32 hdiff = s_res8E50[val0].h - s_res8E50[val1].h;

    if ((hdiff | wdiff) < 0)
        return 0x80243905;
    *out0 = s_map8DE0[val0];
    *out1 = s_map8DEC[val1];
    return 0;
}

/* 0x2C08 sub_00002C08 */

int sub_00002C08(int val)
{
    return s_map8E28[val];
}

/* batch D4 additions */

/* gen defines both of these much later in the file (sub_000033E8 at line
   2255, sub_000034B8 at line 2260); the prototypes have to precede these
   callers. sub_000055AC is repeated from batch D2 (gen line 1703, already
   above this window) only so the fragment also splices without D2 - an
   identical redeclaration is legal C. */
s32 sub_000033E8(void);
s32 sub_000034B8(void);
s32 sub_000055AC(int cmd, int *arg);

s32 sceUsbCamStartVideo(void)
{
    int oldK1;
    s32 ret;

    oldK1 = pspShiftK1();
    ret = sub_000033E8();
    pspSetK1(oldK1);
    return ret;
}

/* 0x2C48 sceUsbCamStopVideo */

s32 sceUsbCamStopVideo(void)
{
    int oldK1;
    s32 ret;

    oldK1 = pspShiftK1();
    ret = sub_000034B8();
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
    ret = sub_000055AC(3, &val);
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
    ret = sub_000055AC(1, &val);
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
    ret = sub_000055AC(2, &val);
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
    ret = sub_000055AC(4, &val);
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
    ret = sub_000055AC(5, &val);
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
    ret = sub_000055AC(16, &val);
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
    ret = sub_000055AC(6, &val);
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
    ret = sub_000055AC(20, &val);
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
    ret = sub_000055AC(7, &val);
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
    ret = sub_000055AC(9, &val);
    pspSetK1(oldK1);
    return ret;
}

/* 0x2ECC sceUsbCamAutoImageReverseSW */

s32 sceUsbCamAutoImageReverseSW(int on)
{
    if (on != 0)
        VIDEO_WORD(8) |= 0x1000;
    else
        VIDEO_WORD(8) = VIDEO_WORD(8) & ~0x1000u;
    return 0;
}

/* 0x2F00 sceUsbCamGetAutoImageReverseState */

s32 sceUsbCamGetAutoImageReverseState(void)
{
    return (VIDEO_WORD(8) >> 12) & 1;
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
    cbid = VIDEO_WORD(0x1A4);
    ret = 0x8024390F;
    if (cbid >= 0) {
        VIDEO_WORD(0x1A4) = -1;
        ret = cbid;
    }
    sceKernelCpuResumeIntr(intr);
    pspSetK1(oldK1);
    return ret;
}

/* Accessory info blob compared by sub_00002FDC (orig rodata 0x9014). */
static const u8 s_usbAccInfoMagic[8] =
    { 0x4C, 0x05, 0x5B, 0x02, 0x01, 0x10, 0x01, 0x00 };

int sub_00002F78(int speed __attribute__((unused)), void *arg2 __attribute__((unused)),
                 void *arg3 __attribute__((unused)))
{
    u8 ret = VIDEO_BYTE(2);

    if (ret != 0)
        return ret;

    VIDEO_BYTE(2) = 1;
    VIDEO_BYTE(4) = 0;
    VIDEO_WORD(8) = 0;
    g_videoState.unk1B4 = 0;
    sub_00003324();
    sceKernelClearEventFlag(g_videoState.unk190, 0);

    return sceKernelSetEventFlag(g_videoState.unk190, 0x300);
}

int sub_00002FDC(int speed __attribute__((unused)), void *arg2 __attribute__((unused)),
                 void *arg3 __attribute__((unused)))
{
    u8 ret = MIC_BYTE(2);
    u64 info;
    int res;

    if (ret != 0)
        return ret;

    MIC_BYTE(2) = 1;
    MIC_BYTE(4) = 0;
    sceKernelClearEventFlag(MIC_WORD(0x114), 0);
    sceKernelSetEventFlag(MIC_WORD(0x114), 0x20);

    res = sceUsbAccGetInfo(&info);
    if (res != 0)
        return res;

    res = memcmp(&info, s_usbAccInfoMagic, 8);
    if (res != 0)
        return res;

    MIC_WORD(0x128) = 1;
    Kprintf("%s16 aligned data swap\n", "usbcammic: ");

    return 0;
}

/* 0x307C sub_0000307C */
int sub_0000307C(int arg1 __attribute__((unused)), int arg2 __attribute__((unused)), int arg3 __attribute__((unused)))
{
    /* Body is a bare "jr $ra"; $v0 is undefined on entry and the caller
       (g_videoDriver.unk34) ignores it - return 0 by convention. */
    return 0;
}


int sub_00003084(int arg1 __attribute__((unused)), int arg2 __attribute__((unused)), int arg3 __attribute__((unused)))
{
    u8 *p = *(u8 **)arg3;
    u8 *q = *(u8 **)(p + 16);

    q[8] += (u8)g_micIntp.unk4;

    return 0;
}


void sub_000030A4(struct UsbdDeviceReq *req __attribute__((unused)))
{
    sceKernelSignalSema(g_videoState.unk194, 1);
}

/* 0x30C8 sub_000030C8 */
void sub_000030C8(struct UsbdDeviceReq *req __attribute__((unused)))
{
    /* Body is a bare "jr $ra"; the microphone receive completion callback
       installed as g_micState.reqD.func does nothing. */
}


/* 0x30D0 sub_000030D0 */
void sub_000030D0(struct UsbdDeviceReq *req)
{
    struct VideoStateFull *st = (struct VideoStateFull *)&g_videoState;
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

    if (VIDEO_BYTE(4) == 2) {
        for (i = 0; i < 2; i++) {
            if (st->items[i].retcode > 0)
                return;
        }
        sceKernelSetEventFlag(st->eventflag, 2);
        return;
    }
    if (req->retcode < 0) {
        if (VIDEO_BYTE(5) == 1)
            sceKernelSetEventFlag(st->eventflag, 0x8000);
        return;
    }

    node = (struct VideoDescState *)VIDEO_WORD(296);
    if (node->unk8 == 1)
        goto fill;
    prev = node;
    node = node->unkC;
    if (node == prev)
        goto picked;
    start = prev;
walk:
    if (node->unk8 != 1)
        goto picked;
    node = node->unkC;
    if (start != node)
        goto walk;
picked:
    if ((struct VideoDescState *)VIDEO_WORD(296) == node)
        return;

fill:
    recvsize = (u32)req->recvsize;
    if (recvsize == 0) {
        node->unk8 = 0;
        goto signal;
    }
    src = (u8 *)req->data;
    remaining = recvsize;
copy:
    len = (u32)node->unk4;
    count = (896 - len < remaining) ? 896 - len : remaining;
    memcpy((u8 *)node->unk0 + len, src, count);
    remaining -= count;
    src += count;
    len = (u32)node->unk4 + count;
    node->unk4 = (int)len;
    if (len >= 896)
        goto full;
cont:
    if (remaining != 0)
        goto copy;
tail:
    recvsize = (u32)req->recvsize;
    if (recvsize == 0) {
        node->unk8 = 0;
        goto signal;
    }
    /* 0x31F0: recvsize == 896 * (((recvsize >> 7) * 0x24924936) >> 32) */
    if (recvsize % 896 != 0)
        goto drain;
    goto modeCheck;

drain:
    node->unk8 = 0;
signal:
    next = node->unkC;
    VIDEO_WORD(296) = (u32)next;
    sceKernelSetEventFlag(st->eventflag, 4);
    goto modeCheck;

full:
    node->unk8 = 0;
    next = node->unkC;
    VIDEO_WORD(296) = (u32)next;
    sceKernelSetEventFlag(st->eventflag, 4);
    if (next->unk8 != 1)
        goto tail;
    node = next;
    goto cont;

modeCheck:
    if (VIDEO_BYTE(4) >= 2)
        return;
    sceKernelDcacheInvalidateRange(req->data, 1792);
    req->unk1c = 0;
    sceUsbbdReqRecv(req);
}

/* 0x3324 sub_00003324 */
s32 sub_00003324(void)
{
    u8 *blk = &VIDEO_BYTE(0x0C);

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


/* 0x33E8 sub_000033E8 */
s32 sub_000033E8(void)
{
    s32 intr;
    s32 ret;

    intr = sceKernelCpuSuspendIntr();
    ret = 0;
    if (VIDEO_BYTE(0) == 0) {
        ret = 0x80243908;
        goto out;
    }
    if (VIDEO_BYTE(2) == 0) {
        ret = 0x80243902;
        goto out;
    }
    if (sceUsbAccGetAuthStat() < 0) {
        ret = 0x80243902;
        goto out;
    }
    if ((VIDEO_BYTE(1) & 1) == 0) {
        ret = 0x80243901;
        goto out;
    }
    if (VIDEO_BYTE(4) != 0) {
        ret = 0x80243909;
        goto out;
    }
    sceKernelClearEventFlag(VIDEO_WORD(400), 0xFFFFFFF7);
    sceKernelSetEventFlag(VIDEO_WORD(400), 1);
    VIDEO_BYTE(4) = 1;
out:
    sceKernelCpuResumeIntr(intr);
    return ret;
}


/* 0x34B8 sub_000034B8 */
s32 sub_000034B8(void)
{
    u32 bits;
    u32 v;
    s32 intr;
    int mode;

    if (VIDEO_BYTE(0) == 0)
        return 0x80243908;
    if (VIDEO_BYTE(2) == 0)
        return 0x80243902;
    if (sceUsbAccGetAuthStat() < 0)
        return 0x80243902;
    if ((VIDEO_BYTE(1) & 1) == 0)
        return 0x80243901;
    mode = VIDEO_BYTE(4);
    if (mode == 0 || mode == 2)
        return 0;

    intr = sceKernelCpuSuspendIntr();
    if (VIDEO_BYTE(4) != 3) {
        v = VIDEO_WORD(8) & ~4u;
        VIDEO_BYTE(4) = 2;
        VIDEO_WORD(8) = v;
        sceKernelSetEventFlag(VIDEO_WORD(400), 4);
    } else {
        VIDEO_WORD(8) = VIDEO_WORD(8) & ~4u;
    }
    sceKernelCpuResumeIntr(intr);
    sceKernelWaitEventFlag(VIDEO_WORD(400), 0x408, 1, &bits, NULL);
    sceKernelWaitEventFlag(VIDEO_WORD(400), 0x480, 1, &bits, NULL);
    memset((void *)VIDEO_WORD(0x150), 0, VIDEO_WORD(0x154) * 2);
    return 0;
}


/* 0x35F4 sub_000035F4 */
s32 sub_000035F4(void *buf, int size)
{
    s32 intr;
    s32 ret;

    intr = sceKernelCpuSuspendIntr();
    ret = 0;
    if (VIDEO_BYTE(4) != 0) {
        ret = 0x80243909;
        goto out;
    }
    if ((VIDEO_WORD(8) & 8) != 0) {
        ret = 0x80243909;
        goto out;
    }
    VIDEO_WORD(376) = size;
    VIDEO_WORD(380) = (u32)buf;
    VIDEO_WORD(372) = size;
    if (sceUsbAccGetAuthStat() != 0) {
        ret = 0x80243902;
        goto out;
    }
    VIDEO_WORD(8) = VIDEO_WORD(8) | 8;
    sceKernelSetEventFlag(VIDEO_WORD(400), 0x800);
out:
    sceKernelCpuResumeIntr(intr);
    return ret;
}



/* 0x36B4 sub_000036B4 */
s32 sub_000036B4(SceSize args __attribute__((unused)), void *argp __attribute__((unused)))
{
    u8 blk[32];
    u32 bits;
    u32 mask;
    u16 half;
    void *pkt;
    s32 res;
    s32 intr;

loop:
    res = sceKernelWaitEventFlag(VIDEO_WORD(400), 0x4200, 1, &bits, NULL);
    if (res < 0)
        goto out;
    if (bits & 0x4000)
        goto out;
    if (sub_00000170(&pkt) < 0)
        goto loop;
    VIDEO_WORD(8) |= 0x2000;
    if (VIDEO_WORD(436) == 0) {
        res = sub_000055AC(0xC0000003, (int *)&VIDEO_WORD(436));
        if (res < 0)
            goto loop;
    }

    res = sceKernelWaitEventFlag(VIDEO_WORD(400), 0x4C01, 1, &bits, NULL);
    if (res < 0)
        goto out;
    if (bits & 0x4000)
        goto out;
    if (bits & 0x400)
        goto loop;

    if (bits & 0x800) {
        if (VIDEO_BYTE(2) == 0)
            goto loop;
        if (sceUsbAccGetAuthStat() != 0)
            goto waitFrame;
        if (VIDEO_WORD(436) >= 1280) {
            if (VIDEO_BYTE(0x32) < 2)
                VIDEO_BYTE(0x32) = 2;
            if (VIDEO_BYTE(0x33) < 2)
                VIDEO_BYTE(0x33) = 2;
        }
        if (sceKernelLockMutex(VIDEO_WORD(408), 1, NULL) == 0) {
            sub_00000000(3, 2, &VIDEO_BYTE(0x2C), 20);
            sceKernelUnlockMutex(VIDEO_WORD(408), 1);
        }
        if (VIDEO_WORD(436) < 1280) {
            sceKernelClearEventFlag(VIDEO_WORD(400), ~0x800u);
            goto loop;
        }
        goto waitFrame;
    }

    if ((bits & 1) == 0)
        goto waitFrame;
    intr = sceKernelCpuSuspendIntr();
    if (VIDEO_BYTE(2) == 0) {
        res = -1;
    } else if (sceUsbAccGetAuthStat() != 0) {
        Kprintf("%serror - Not accessory !!", "");
    } else {
        __builtin_memcpy(blk, &VIDEO_BYTE(0x0C), 32);
        blk[6] = sub_00001084(blk[6]);
        blk[7] = sub_00001058(blk[7]);
        blk[9] = sub_000010B8(blk[9]);
        if (VIDEO_WORD(436) >= 1280) {
            if (blk[0] < 2)
                blk[0] = 2;
            if (blk[1] < 2)
                blk[1] = 2;
        }
        sceKernelCpuResumeIntr(intr);
        res = sceKernelLockMutex(VIDEO_WORD(408), 1, NULL);
        if (res == 0) {
            sub_00000000(1, 2, blk, 32);
            res = sceKernelUnlockMutex(VIDEO_WORD(408), 1);
        }
        intr = sceKernelCpuSuspendIntr();
    }
    sceKernelCpuResumeIntr(intr);
    if (res >= 0)
        goto waitFrame;
    goto loop;

waitFrame:
    mask = (VIDEO_WORD(436) < 1280) ? 0x410 : 0x80410;
    res = sceKernelWaitEventFlag(VIDEO_WORD(400), mask, 1, &bits, NULL);
    if (res >= 0 && (bits & 0x400) != 0)
        goto loop;
    if (VIDEO_BYTE(4) == 2) {
        sub_00004C00((VIDEO_WORD(8) >> 4) & 1);
    } else {
        if ((bits & 0x80010) != 0 && (VIDEO_WORD(8) & 0x1000) != 0) {
            half = (VIDEO_WORD(8) & 0x400) ? 0x101 : 0x100;
            sub_000000F4(&half);
            sceKernelClearEventFlag(VIDEO_WORD(400), 0xFFFDFFFFu);
        }
        if (bits & 0x10)
            res = sub_00004F04(0);
        else if (bits & 0x80000)
            res = sub_00004F04(1);
    }
    if (res >= 0)
        goto loop;

out:
    return 0;
}


/* 0x3AA0 sub_00003AA0 */
s32 sub_00003AA0(SceSize args __attribute__((unused)), void *argp __attribute__((unused)))
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
    res = sceKernelWaitEventFlag(VIDEO_WORD(400), 0x4040, 1, &bits, NULL);
    sceKernelClearEventFlag(VIDEO_WORD(400), 0xFFFFFFBFu);
    if (res < 0 || (bits & 0x4000) != 0)
        sceKernelSetEventFlag(VIDEO_WORD(400), 0x80);
    if (res < 0) {
        VIDEO_WORD(384) = (u32)res;
        goto loop;
    }
    if ((bits & 0x4000) != 0) {
        VIDEO_WORD(384) = 0;
        return 0;
    }

    peer = (s8)VIDEO_BYTE(7);
    idx = (s8)VIDEO_BYTE(6);
    userBuf = (u8 *)VIDEO_WORD(432);
    userSize = VIDEO_WORD(428);
    if (peer == idx)
        goto wait2;
    if (VIDEO_WORD(348 + 8 * idx) != 0)
        goto copy;

wait2:
    res = sceKernelWaitEventFlag(VIDEO_WORD(400), 0x428, 1, &bits, NULL);
    if (res < 0) {
        VIDEO_WORD(384) = (u32)res;
        sceKernelSetEventFlag(VIDEO_WORD(400), 0x80);
        goto loop;
    }
    if ((bits & 0x400) != 0) {
        VIDEO_WORD(384) = 0x80243902;
        sceKernelSetEventFlag(VIDEO_WORD(400), 0x80);
        goto loop;
    }
    if ((bits & 8) != 0) {
        VIDEO_WORD(384) = 0;
        sceKernelSetEventFlag(VIDEO_WORD(400), 0x80);
        goto loop;
    }
    idx = (s8)VIDEO_BYTE(6);

copy:
    off = 0;
    avail = VIDEO_WORD(348 + 8 * idx);
    size = (userSize < avail) ? userSize : avail;
    if (size != 0) {
        src = (u8 *)VIDEO_WORD(344 + 8 * idx);
        do {
            chunk = (size < 16380) ? size : 16380;
            sub_00004A24(userBuf + off, src + off, (int)chunk);
            size -= chunk;
            off += chunk;
        } while (size != 0);
    }

    intr = sceKernelCpuSuspendIntr();
    idx = (s8)VIDEO_BYTE(6);
    avail = VIDEO_WORD(348 + 8 * idx);
    status = (userSize < avail) ? (s32)0x8024390A : (s32)avail;
    VIDEO_WORD(384) = (u32)status;
    idx = (s8)VIDEO_BYTE(6);
    VIDEO_WORD(348 + 8 * idx) = 0;
    sceKernelClearEventFlag(VIDEO_WORD(400), 0xFFFFFFDFu);
    /* 0x3C30 reloads VIDEO_WORD(384) into $a1 and max()es it with 0;
       interrupts are suspended across the store, so the local is the
       same value. Signed clamp: sceUsbCamGetReadVideoFrameSize reads
       this word back as a byte count (batchC1_NOTES 381-390). */
    VIDEO_WORD(388) = (status >= 0) ? (u32)status : 0u;
    sceKernelSetEventFlag(VIDEO_WORD(400), 0x80);
    sceKernelCpuResumeIntr(intr);
    goto loop;
}


/* 0x3CD4 sub_00003CD4 */
s32 sub_00003CD4(SceSize args __attribute__((unused)), void *argp __attribute__((unused)))
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

    MIC_WORD(0x34) = 0;

loop:
    res = sceKernelWaitEventFlag(MIC_WORD(0x114), 0x204, 1, &bits, NULL);
    if (res < 0)
        return 0;
    if (bits & 0x200)
        return 0;

    dst = (u8 *)MIC_WORD(0x124);
    pending = MIC_WORD(0x120);
    res = sceKernelWaitEventFlag(MIC_WORD(0x114), 0x110, 1, &bits, NULL);
    copied = 0;
    status = res;
    if (res < 0)
        goto done;
    if (bits & 0x100) {
        status = 0x80243902;
        goto done;
    }
    if (dst == NULL)
        goto done;

    limit = (MIC_BYTE(6) != 0) ? 132u : 0u;
    if (limit >= pending)
        goto done;

inner:
    res = sceKernelWaitEventFlag(MIC_WORD(0x114), 0x121, 1, &bits, NULL);
    status = res;
    if (res < 0)
        goto done;
    if (bits & 0x100) {
        status = 0x80243902;
        goto done;
    }
    if (bits & 1) {
        n = sub_000080F8(dst + copied, (int)pending);
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
    MIC_WORD(0x34) = (u32)status;
    intr = sceKernelCpuSuspendIntr();
    if (MIC_BYTE(4) == 4)
        MIC_BYTE(4) = 1;
    sceKernelCpuResumeIntr(intr);
    MIC_WORD(0x124) = 0;
    MIC_WORD(0x120) = 0;
    sceKernelClearEventFlag(MIC_WORD(0x114), 0xFFFFFFFBu);
    sceKernelSetEventFlag(MIC_WORD(0x114), 2);
    goto loop;
}


/* 0x3E94 sub_00003E94 */
void sub_00003E94(struct UsbdDeviceReq *req)
{
    u8 *ptr;
    u32 v;
    s16 half;
    s32 arg;
    s32 cbid;
    int size;

    if (req->retcode != 0)
        return;
    if ((s8)VIDEO_BYTE(0x70) < 0)
        return;

    if (VIDEO_BYTE(0x71) == 9) {
        ptr = (u8 *)VIDEO_WORD(0x6C);
        if (ptr[2] != VIDEO_BYTE(0x72)) {
            VIDEO_WORD(424) = 0xFFFFFFFF;
        } else {
            VIDEO_WORD(424) = 0;
            ptr[3] = (u8)req->recvsize;
            memset(ptr + 4, 0, 60);
            size = ((u32)req->recvsize < 61u) ? req->recvsize : 60;
            memcpy(ptr + 4, req->data, size);
        }
        sceKernelSetEventFlag(VIDEO_WORD(400), 0x10000);
        return;
    }

    if (VIDEO_BYTE(0x71) != 10)
        return;
    if (VIDEO_HALF(0x72) != 16)
        return;

    /* 0x3F08-0x3F18: the first two data bytes are pushed on the stack and
       read back with `lh` - a little-endian sign-extended halfword. */
    half = (s16)((u16)((u8 *)req->data)[0] | ((u16)((u8 *)req->data)[1] << 8));
    v = VIDEO_WORD(8);
    if (half == 1) {
        v &= ~0x400u;
        arg = 1;
    } else {
        v |= 0x400u;
        arg = 0;
    }
    cbid = (s32)VIDEO_WORD(420);
    VIDEO_WORD(8) = v;
    if (cbid > 0)
        sceKernelNotifyCallback(cbid, arg);
    if (VIDEO_WORD(8) & 0x1000)
        sceKernelSetEventFlag(VIDEO_WORD(400), 0x20000);
}


/* 0x3FEC sub_00003FEC */
void sub_00003FEC(struct UsbdDeviceReq *req)
{
    u32 avail;
    u32 left;
    u32 space;
    u32 n;
    u32 done;
    u32 total;

    if ((u32)VIDEO_WORD(436) >= 1280u)
        return;
    if (req->retcode < 0)
        return;

    done = 0;

    if (VIDEO_WORD(364) == 0) {
        /* No threshold armed: a 6-byte request stages a new one. */
        if (req->recvsize == 6)
            __builtin_memcpy(&VIDEO_WORD(364), req->data, 4);
        goto rearm;
    }

    avail = VIDEO_WORD(376);
    if (avail == 0)
        goto finish;
    if (VIDEO_WORD(380) == 0)
        goto finish;
    left = (u32)req->recvsize;
    if (left == 0)
        goto finish;

    for (;;) {
        avail = VIDEO_WORD(376);
        /* The clamp compares against req->recvsize itself, not against the
           remaining count: `sltu $t7, $v1, $s0` + `movn $s0, $v1, $t7`
           with `$v1` reloaded from 20($s1) at the top of every iteration
           (0x4110, the jump delay slot). */
        n = ((u32)req->recvsize < avail) ? (u32)req->recvsize : avail;
        memcpy((u8 *)VIDEO_WORD(380), (u8 *)req->data + done, n);
        space = VIDEO_WORD(380);
        avail = VIDEO_WORD(376);
        left -= n;
        VIDEO_WORD(376) = avail - n;
        done += n;
        /* 0x4108 is the delay slot of the `beqz $s2` exit test, so the
           store runs whether or not the loop is leaving. */
        VIDEO_WORD(380) = space + n;
        if (left == 0)
            break;
    }

finish:
    total = VIDEO_WORD(368) + (u32)req->recvsize;
    VIDEO_WORD(368) = total;
    if (done != 0) {
        if ((done & 0x3F) != 0)
            goto signal;
        if (VIDEO_WORD(364) >= total)
            goto rearm;
    }

signal:
    VIDEO_WORD(380) = 0;
    sceKernelSetEventFlag(VIDEO_WORD(400), 0x1000);
    return;

rearm:
    req->size = 64;
    sceKernelDcacheInvalidateRange(req->data, 128);
    sceUsbbdReqRecv(req);
}

/* 0x4164 sub_00004164 */
s32 sub_00004164(void)
{
    struct VideoStateFull *st = (struct VideoStateFull *)&g_videoState;
    struct VideoDescState *node;
    s32 res;
    int i;

    VIDEO_WORD(360) = 0;
    VIDEO_WORD(8) = (VIDEO_WORD(8) | 1u) & ~2u;

    for (i = 0; i < 2; i++) {
        st->desc[i].unk4 = 0;
        st->desc[i].unk8 = 1;
    }

    /* 0x41C8 / 0x41D4: the same node pointer lands in both slots (start
       after current). */
    node = &st->desc[0];
    VIDEO_WORD(300) = (u32)node;
    VIDEO_WORD(296) = (u32)node;

    for (i = 0; i < 2; i++)
        VIDEO_WORD(348 + 8 * i) = 0;

    VIDEO_BYTE(6) = 1;
    VIDEO_BYTE(7) = 0;

    for (i = 0; i < 2; i++) {
        memset(st->frameBufs[i], 0, 1792);
        sceKernelDcacheInvalidateRange(st->frameBufs[i], 1792);
        st->items[i].unk1c = (int)&st->items[i + 1];
    }
    /* 0x4258 sits in the `jal sceUsbbdReqRecv` delay slot, so it runs
       before the call - exactly like gen's mic loop at line 636. */
    st->items[1].unk1c = 0;

    res = sceUsbbdReqRecv(&st->items[0]);
    if (res < 0 && st->items[0].retcode < 0)
        VIDEO_BYTE(5) = 1;
    return res;
}


/*
 * sceUsbBus_driver import NID 0xCC57EC9D (psplibdoc_usb.csv:21 names it
 * sceUsbbdReqCancel). No uofw header declares it; the prototype is the
 * sibling of sceUsbbdReqSend (include/usbbus.h:187) and is guessed.
 * batchC2_NOTES.md.
 */
int sceUsbbdReqCancel(struct UsbdDeviceReq *req);

/* include/interruptman.h:150-151 verbatim; gen_usbcam.c does not pull
   interruptman.h in, so the fragment declares them itself. Identical to
   the declarations in batchC1.c and batchC2_NOTES.md. */
s32 sceKernelCpuSuspendIntr(void);
void sceKernelCpuResumeIntr(s32 intr);

/* src/kd/dmacman/dmacman.c:375/463 - both are implemented in uOFW but
   neither is listed by include/dmacman.h, so the fragment re-states the
   definitions. Harmless redeclaration if the merge adds them. */
s32 sceKernelDmaOpSetupMemcpy(sceKernelDmaOperation *op, s32 arg1, s32 arg2, s32 arg3);
s32 sceKernelDmaOpSync(sceKernelDmaOperation *op, s32 command, u32 *timeout);

/* include/lowio_ddr.h:7, include/sysmem_suspend_kernel.h:11/13 and
   include/sysmem_kernel.h:299 verbatim; none of those headers are
   reachable from gen_usbcam.c's include set. */
int sceDdrFlush(int);
s32 sceKernelPowerLock(s32 lockType);
s32 sceKernelPowerUnlock(s32 lockType);
void *sceKernelMemcpy(void *dst, const void *src, u32 n);

/* Helpers and dispatch handlers defined after this window in
   gen_usbcam.c (batchB2.c at 0x5870-0x6DA4); batchC2 sits above them,
   so the prototypes are restated here. Every prototype below matches
   gen_usbcam.c's definition exactly. */
s32 sub_00006DA4(int *arg);

int sub_00005870(int *arg);
int sub_00005948(int *arg);
int sub_000059BC(int *arg);
int sub_000059D0(int *arg);
int sub_00005A88(int *arg);
int sub_00005A9C(int *arg);
int sub_00005B4C(int *arg);
int sub_00005B60(int *arg);
int sub_00005C18(int *arg);
int sub_00005C2C(int *arg);
int sub_00005D44(int *arg);
int sub_00005D58(int *arg);
int sub_00005D7C(int *arg);
int sub_00005D98(int *arg);
int sub_00005E28(int *arg);
int sub_00005E5C(int *arg);
int sub_00005F00(int *arg);
int sub_00005F50(int *arg);
int sub_00006020(int *arg);
int sub_00006054(int *arg);
int sub_00006100(int *arg);
int sub_00006134(int *arg);
int sub_000061E0(int *arg);
int sub_00006214(int *arg);
int sub_000062C0(int *arg);
int sub_000062F4(int *arg);
int sub_000063A0(int *arg);
int sub_000063D4(int *arg);
int sub_000064FC(int *arg);
int sub_00006560(int *arg);
int sub_000065F0(int *arg);
int sub_00006604(int *arg);
int sub_000066B0(int *arg);
int sub_000066E4(int *arg);
int sub_00006790(int *arg);
int sub_000067C4(int *arg);
int sub_000067FC(int *arg);
int sub_0000694C(int *arg);
int sub_00006984(int *arg);
int sub_00006AD4(int *arg);
int sub_00006C0C(int *arg);
int sub_00006D3C(int *arg);

/* 60-byte block sceUsbCamSetupStill (0x1CAC) and sceUsbCamSetupStillEx
   (0x1EBC) build on the stack and pass to sub_00004680. Field +0 and
   +5 are never read; +44 and +48 are read back as u16. batchC2_NOTES.md
   carries the caller-to-field mapping. */
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
   one-to-four/six/seven byte index maps read by sub_00004298 and
   sub_00004680. The surrounding objects (0x8DEC, 0x8E0C, 0x8DF8,
   0x8E28) already live in gen_usbcam.c. */
static const u8 s_map8E00[4] = { 1, 2, 3, 0 };
static const u8 s_map8E04[4] = { 0, 1, 2, 3 };
static const u8 s_map8E08[4] = { 0, 1, 2, 0 };
static const u8 s_map8E10[7] = { 0, 1, 2, 3, 4, 5, 6 };
static const u8 s_map8E18[4] = { 0, 1, 2, 0 };
static const u8 s_map8E20[4] = { 0, 1, 2, 3 };
static const u8 s_map8E24[4] = { 1, 2, 3, 0 };

/* 0x8E50: ten {width, height} pairs indexed by the 0..9 resolution
   codes sub_00004298 range-checks the caller's frame size against. */
static const struct UsbCamResEntry s_res8E50[10] = {
    { 160, 120 }, { 176, 144 }, { 320, 240 }, { 352, 288 }, { 360, 272 },
    { 480, 272 }, { 640, 480 }, { 1024, 768 }, { 1280, 960 }, { 1280, 1024 }
};

/* 0x4298: pack a 100-byte PspUsbCamSetupVideoExParam into the 32-byte
   command block sceUsbCamSetupVideoEx writes into g_videoState. */

int sub_00004298(u8 *out, struct UsbCamVideoReq *req)
{
    const struct UsbCamResEntry *cur;
    const struct UsbCamResEntry *sel;
    s32 diffW;
    s32 diffH;
    u32 v;
    int ret;

    ret = 0x80243905;
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
    ret = 0x80243906;
    if ((u32)req->saturation >= 256)
        goto out;
    if ((u32)req->brightness >= 256)
        goto out;
    if ((u32)req->contrast >= 256)
        goto out;
    if ((u32)req->sharpness >= 256)
        goto out;
    ret = 0x80243905;
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
    if (sub_000011F4(req->unk, req->resolution, req->unk8) == 0)
        goto out;
    if ((s32)req->framerate >= 5 && (u32)(req->unk - 7) < 3)
        goto out;
    ret = 0x80243907;
    if ((s32)req->unk9 >= 3)
        goto out;
    if ((u32)req->framesize - 1 > 0x87FF)
        goto out;
    if ((u32)req->unk12 >= 3)
        goto out;

    out[0] = s_map8DE0[req->unk];
    out[1] = s_map8DEC[req->resolution];
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

int sub_00004680(u8 *out, struct UsbCamStillReq *req)
{
    int ret;

    ret = 0x80243907;
    if ((u32)req->jpegsize > 0x80000)
        goto out;
    ret = 0x80243905;
    if ((s32)req->resolution >= 10)
        goto out;
    if ((s32)req->framesize >= 10)
        goto out;
    if ((s32)req->resolution < (s32)req->framesize)
        goto out;
    if ((s32)req->unk6 >= 3)
        goto out;
    ret = 0x80243907;
    if ((u32)req->complevel - 1 >= 63)
        goto out;
    ret = 0x80243905;
    if ((u32)req->unk9 >= 4)
        goto out;
    if ((u32)req->unk13 >= 7)
        goto out;
    ret = 0x80243907;
    if ((u32)req->unk14 >= 3)
        goto out;

    out[0] = (u8)req->jpegsize;
    out[1] = (u8)((u32)req->jpegsize >> 8);
    out[2] = (u8)((u32)req->jpegsize >> 16);
    out[3] = (u8)((u32)req->jpegsize >> 24);
    out[4] = (u8)req->complevel;
    out[5] = 1;
    out[6] = s_map8DE0[req->resolution];
    out[7] = s_map8DEC[req->framesize];
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

s32 sub_00004858(void)
{
    int ret;

    ret = 0x80243908;
    if (g_videoState.unk0 == 0)
        goto out;
    ret = 0x80243902;
    if (VIDEO_BYTE(2) == 0)
        goto out;
    if (sceUsbAccGetAuthStat() < 0)
        goto out;
    ret = 0x80243901;
    if ((VIDEO_BYTE(1) & 2) == 0)
        goto out;
    ret = 0x8024390C;
    if ((VIDEO_WORD(8) & 8) != 0)
        ret = 0;
out:
    return ret;
}

/* 0x48DC: reap the still frame. arg == 0 waits on the video event flag,
   arg != 0 polls it; both then drain the pending request and report the
   byte count that sub_00004C00's caller consumed. */

s32 sub_000048DC(int arg)
{
    struct VideoStateFull *st;
    u32 bits;
    s32 res;
    s32 ret;

    st = (struct VideoStateFull *)&g_videoState;
    if (arg == 0) {
        res = sceKernelWaitEventFlag(st->eventflag, 0x3400, 1, &bits, NULL);
        if (res < 0)
            return res;
    } else {
        res = sceKernelPollEventFlag(st->eventflag, 0x3400, 1, &bits);
        if (res == (s32)0x800201AF)
            return 0x8024390E;
        if (res < 0) {
            VIDEO_WORD(8) = VIDEO_WORD(8) & ~8u;
            return res;
        }
    }

    if ((bits & 0x2400) != 0) {
        ret = (bits & 0x400) ? 0x80243902 : 0;
        if (VIDEO_WORD(436) < 1280)
            sceUsbbdReqCancel(&st->reqC);
    } else if (VIDEO_WORD(372) < VIDEO_WORD(368)) {
        ret = 0x8024390A;
    } else {
        ret = (VIDEO_WORD(376) != 0) ? VIDEO_WORD(368) : VIDEO_WORD(372);
    }

    if (VIDEO_WORD(436) >= 1280) {
        VIDEO_WORD(8) = VIDEO_WORD(8) | 0x10;
        sceKernelSetEventFlag(st->eventflag, 0x100000);
    }
    VIDEO_WORD(8) = VIDEO_WORD(8) & ~8u;
    return ret;
}

/* 0x4A24: memcpy wrapper used by the still-capture drain loop
   (0x3B94-0x3BD8). Falls back to sceKernelMemcpy for addresses outside
   the DMA mask 0x00220202 or when no DMA op has been allocated;
   otherwise it DMAs in <= 16380-byte word-aligned chunks, copies the
   0..3 byte tail in software and returns dst. */

void *sub_00004A24(void *dst, const void *src, int size)
{
    sceKernelDmaOperation *op;
    u32 dmacDst;
    u32 dmacSrc;
    u32 aligned;
    int res;

    if (((0x00220202u >> (((u32)dst >> 27) & 0x1F)) & 1) == 0)
        return sceKernelMemcpy(dst, src, (u32)size);
    op = (sceKernelDmaOperation *)VIDEO_WORD(392);
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

s32 sub_00004C00(int arg)
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
    if (VIDEO_BYTE(2) == 0) {
        done = 1;
        goto resume;
    }
    if (sceUsbAccGetAuthStat() != 0) {
        sceKernelClearEventFlag(VIDEO_WORD(400), (u32)-5);
        VIDEO_WORD(0x134) = 0;
        VIDEO_WORD(0x138) = 1;
        VIDEO_WORD(0x144) = 0;
        VIDEO_WORD(0x148) = 1;
        if (VIDEO_BYTE(4) == 2) {
            sceKernelClearEventFlag(VIDEO_WORD(400), (u32)-2050);
            VIDEO_BYTE(4) = 0;
            sceKernelSetEventFlag(VIDEO_WORD(400), 8);
        } else {
            VIDEO_BYTE(4) = 0;
        }
        done = 1;
        goto resume;
    }

    sceKernelCpuResumeIntr(intr);
    ret = sceKernelWaitEventFlag(VIDEO_WORD(400), 0x402, 1, &bits, NULL);
    if (ret < 0)
        return ret;
    if (arg != 0) {
        ret = sceKernelWaitEventFlag(VIDEO_WORD(400), 0x100400, 1, &bits, NULL);
        if (ret < 0)
            goto suspendCheck;
        sceKernelClearEventFlag(VIDEO_WORD(400), 0xFFEFFFFFu);
    }
    if ((bits & 0x400) != 0) {
        done = 1;
        goto suspendCheck;
    }
    ret = sceKernelLockMutex(VIDEO_WORD(408), 1, NULL);
    if (ret != 0)
        goto suspendCheck;
    ret = sub_00000000((arg != 0) ? 4 : 2, 2, NULL, 0);
    sceKernelUnlockMutex(VIDEO_WORD(408), 1);
    goto suspendCheck;

suspendCheck:
    intr = sceKernelCpuSuspendIntr();
    if (ret >= 0)
        goto resume;
    sceKernelUnlockMutex(VIDEO_WORD(408), 1);
    done = 1;
    goto resume;

resume:
    sceKernelCpuResumeIntr(intr);
    if (done)
        return ret;

    ret = sceKernelWaitEventFlag(VIDEO_WORD(400), 0x500, 1, &bits, NULL);
    if (ret < 0)
        return ret;
    if ((bits & 0x400) != 0)
        return 0;
    intr = sceKernelCpuSuspendIntr();
    mode = VIDEO_BYTE(4);
    if (mode == 2) {
        sceKernelClearEventFlag(VIDEO_WORD(400), (u32)-2050);
        VIDEO_BYTE(4) = 0;
        sceKernelSetEventFlag(VIDEO_WORD(400), 8);
        for (i = 0; i < 2; i++)
            VIDEO_WORD(348 + i * 8) = 0;
    } else if (mode == 3) {
        sceKernelClearEventFlag(VIDEO_WORD(400), (u32)-2050);
        sceKernelSetEventFlag(VIDEO_WORD(400), 8);
        for (i = 0; i < 2; i++)
            VIDEO_WORD(348 + i * 8) = 0;
        VIDEO_WORD(384) = 0x8024390A;
        VIDEO_BYTE(4) = 0;
    } else {
        VIDEO_BYTE(4) = 0;
    }
    sceKernelCpuResumeIntr(intr);
    sceKernelWaitEventFlag(VIDEO_WORD(400), 0x480, 1, &bits, NULL);
    return ret;
}

/* Transfer node of the list rooted at VIDEO_WORD(300) (g_videoState +
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
   at VIDEO_WORD(300) and merges each node's payload into the active half
   buffer ({VIDEO_WORD(344 + 8 * seb(VIDEO_BYTE(7))),
   VIDEO_WORD(348 + 8 * seb(VIDEO_BYTE(7)))}) while VIDEO_WORD(436) < 1280,
   or into the JPEG pair (VIDEO_WORD(380)/VIDEO_WORD(368), reached through
   the slot/slotLen pair) otherwise. arg == 0 is the half-buffer path,
   arg != 0 the big path. Every exit runs sub_00004C00(arg) and returns the
   last sceKernelWaitEventFlag status. */

s32 sub_00004F04(int arg)
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
    res = sceKernelWaitEventFlag(VIDEO_WORD(400), 0x404, 1, &bits, NULL);
    if (res < 0)
        goto out;
    if ((bits & 0x400) != 0)
        goto out;
    if ((bits & 0x20000) != 0) {
        half = (VIDEO_WORD(8) & 0x400) ? 0x101 : 0x100;
        sub_000000F4(&half);
        sceKernelClearEventFlag(VIDEO_WORD(400), 0xFFFDFFFFu);
    }
    if (VIDEO_BYTE(5) == 1) {
        res = sceKernelWaitEventFlag(VIDEO_WORD(400), 0x8400, 1, &bits, NULL);
        if (res < 0)
            goto out;
        if ((bits & 0x400) != 0)
            goto out;
    }
    if (((u32)VIDEO_BYTE(4) - 2) < 2)
        goto out;

    intr = sceKernelCpuSuspendIntr();
    i = 0;
    node = (struct UsbCamXfer *)VIDEO_WORD(300);
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
    poff = 344 + (u32)((s32)(s8)VIDEO_BYTE(7) * 8);
    sceKernelClearEventFlag(VIDEO_WORD(400), (u32)-5);
    if (sceUsbAccGetAuthStat() != 0)
        goto restart;
    i = 0;
frame:
    buf = node->buf;
    mark = (u32)buf[0] | ((u32)buf[1] << 8);
    if (mark == 0xD8FF)
        goto soi;

cont:
    if ((VIDEO_WORD(8) & 2) != 0) {
        next = node->next;
        goto nodeDone;
    }
    if (VIDEO_WORD(436) >= 1280)
        goto big;

    cap = VIDEO_WORD(340);
    plen = VIDEO_WORD(poff + 4);
    need = (u32)node->len;
    copy = ((cap - plen) < need) ? (cap - plen) : need;
    sceKernelMemcpy((u8 *)VIDEO_WORD(poff) + plen, buf, copy);
    plen = plen + need;
    VIDEO_WORD(poff + 4) = plen;
    if (cap < plen) {
        VIDEO_WORD(8) = VIDEO_WORD(8) | 3;
        VIDEO_WORD(poff + 4) = 0;
        goto restart;
    }
    if (need >= 896) {
        next = node->next;
        goto nodeDone;
    }
    VIDEO_WORD(8) = VIDEO_WORD(8) | 1;
    if ((s32)(s8)VIDEO_BYTE(7) == (s32)(s8)VIDEO_BYTE(6)) {
        VIDEO_WORD(poff + 4) = 0;
        goto nodeTail;
    }
    if (VIDEO_WORD(360) == VIDEO_WORD(poff + 4))
        goto other;
    VIDEO_WORD(poff + 4) = 0;

nodeTail:
    next = node->next;
nodeDone:
    node->flag = 1;
    VIDEO_WORD(300) = (u32)next;
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
    if (VIDEO_WORD(348 + (u32)((s32)(s8)VIDEO_BYTE(6) * 8)) != 0) {
        VIDEO_WORD(poff + 4) = 0;
        goto nodeTail;
    }
    VIDEO_BYTE(6) = VIDEO_BYTE(6) ^ 1;
    VIDEO_BYTE(7) = VIDEO_BYTE(7) ^ 1;
    sceKernelSetEventFlag(VIDEO_WORD(400), 32);
    goto nodeTail;

big:
    limit = (arg == 0) ? VIDEO_WORD(340) : VIDEO_WORD(372);
    plen = VIDEO_WORD(poff + 4);
    need = (u32)node->len;
    if (limit < (plen + need)) {
        VIDEO_WORD(8) = VIDEO_WORD(8) | 2;
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
    VIDEO_WORD(364) = hdr;
    if (VIDEO_WORD(364) == VIDEO_WORD(368))
        goto seg;
    VIDEO_WORD(368) = 0;
    VIDEO_WORD(364) = 0;
    goto nodeTail;

seg:
    clamp = VIDEO_WORD(44);
    if (clamp > 0xFC00)
        clamp = 0xFC00;
    diff = (clamp >= VIDEO_WORD(364)) ? (clamp - VIDEO_WORD(364))
                                      : (VIDEO_WORD(364) - clamp);
    if ((cnt12 + 1) == (u32)buf[9] && VIDEO_WORD(364) < (clamp + 1024))
        cnt16 = 1;
    if ((u32)buf[9] == cnt12)
        cnt20 = (cnt20 + 1) & 0xFF;
    else
        cnt20 = 0;
    if (diff < 1024 || cnt16 != 0)
        goto mode2;
    if (cnt20 < 3) {
        cnt12 = (u32)buf[9];
        VIDEO_WORD(368) = 0;
        goto nodeTail;
    }
mode2:
    VIDEO_WORD(380) = 0;
    sceKernelSetEventFlag(VIDEO_WORD(400), 0x1004);
    VIDEO_BYTE(4) = 2;
    goto restart;

head0:
    __builtin_memcpy(&hdr, buf + 4, 4);
    VIDEO_WORD(360) = hdr;
    VIDEO_WORD(360) = VIDEO_WORD(360) + 12;
    if (VIDEO_WORD(360) != VIDEO_WORD(poff + 4)) {
        VIDEO_WORD(poff + 4) = 0;
        goto nodeTail;
    }
    if (VIDEO_WORD(348 + (u32)((s32)(s8)VIDEO_BYTE(6) * 8)) != 0) {
        VIDEO_WORD(poff + 4) = 0;
        goto nodeTail;
    }
    hdr = VIDEO_WORD(360);
    __builtin_memcpy(buf + 4, &hdr, 4);
    __builtin_memcpy((u8 *)VIDEO_WORD(poff) + 2, buf, 12);
    VIDEO_BYTE(6) = VIDEO_BYTE(7);
    VIDEO_BYTE(7) = VIDEO_BYTE(7) ^ 1;
    sceKernelSetEventFlag(VIDEO_WORD(400), 32);
    goto nodeTail;

soi:
    if (VIDEO_WORD(436) >= 1280) {
        VIDEO_WORD(poff + 4) = 0;
        VIDEO_WORD(8) = (VIDEO_WORD(8) & ~2u) | 1;
        goto cont;
    }
    mark = (u32)buf[2] | ((u32)buf[3] << 8);
    if (mark != 0xFEFF) {
        VIDEO_WORD(8) = VIDEO_WORD(8) | 2;
        goto cont;
    }
    if ((VIDEO_WORD(8) & 1) != 0)
        VIDEO_WORD(8) = VIDEO_WORD(8) & ~3u;
    __builtin_memcpy(&hdr, buf + 6, 4);
    VIDEO_WORD(360) = hdr;
    VIDEO_WORD(poff + 4) = 0;
    if (VIDEO_WORD(340) < VIDEO_WORD(360))
        VIDEO_WORD(8) = VIDEO_WORD(8) | 3;
    goto cont;

common:
    if (arg == 0) {
        slot = poff;
        slotLen = poff + 4;
    } else {
        slot = 380;
        slotLen = 368;
    }
    if ((VIDEO_WORD(8) & 1) == 0)
        goto plain;
    if (arg == 0)
        goto split;

    sceKernelMemcpy((u8 *)VIDEO_WORD(slot), buf, (u32)node->len);
    VIDEO_WORD(8) = VIDEO_WORD(8) & ~1u;
    VIDEO_WORD(slotLen) = VIDEO_WORD(slotLen) + (u32)node->len;
    goto nodeTail;

split:
    sceKernelMemcpy((u8 *)VIDEO_WORD(slot) + 14, buf + 2, (u32)node->len - 2);
    ((u8 *)VIDEO_WORD(slot))[0] = buf[0];
    ((u8 *)VIDEO_WORD(slot))[1] = buf[1];
    VIDEO_WORD(slotLen) = VIDEO_WORD(slotLen) + 12;
    VIDEO_WORD(8) = VIDEO_WORD(8) & ~1u;
    VIDEO_WORD(slotLen) = VIDEO_WORD(slotLen) + (u32)node->len;
    goto nodeTail;

plain:
    sceKernelMemcpy((u8 *)VIDEO_WORD(slot) + VIDEO_WORD(slotLen), buf,
                    (u32)node->len);
    VIDEO_WORD(slotLen) = VIDEO_WORD(slotLen) + (u32)node->len;
    goto nodeTail;

out:
    sub_00004C00(arg);
    return res;
}

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
    { 0x00000003, sub_00005948, sub_000059BC },
    { 0x00000001, sub_000059D0, sub_00005A88 },
    { 0x00000002, sub_00005A9C, sub_00005B4C },
    { 0x00000004, sub_00005B60, sub_00005C18 },
    { 0x00000005, sub_00005C2C, sub_00005D44 },
    { 0x00000006, sub_00005D58, sub_00005D7C },
    { 0x00000007, sub_00005D98, sub_00005E28 },
    { 0x00000009, sub_00005E5C, NULL },
    { 0x40000001, sub_000067C4, sub_000067FC },
    { 0x40000002, sub_0000694C, sub_00006984 },
    { 0x40000003, NULL, sub_00006AD4 },
    { 0x0000000A, sub_00006C0C, sub_00006D3C },
    { 0x0000000B, sub_00005F50, sub_00006020 },
    { 0x0000000C, sub_00005870, sub_00005F00 },
    { 0x0000000D, sub_00006054, sub_00006100 },
    { 0x0000000E, sub_00006134, sub_000061E0 },
    { 0x0000000F, sub_00006214, sub_000062C0 },
    { 0x00000010, sub_000062F4, sub_000063A0 },
    { 0x00000011, sub_000063D4, sub_000064FC },
    { 0x00000012, sub_00006560, sub_000065F0 },
    { 0x00000013, sub_00006604, sub_000066B0 },
    { 0x00000014, sub_000066E4, sub_00006790 }
};

s32 sub_000055AC(int cmd, int *arg)
{
    int (*fn)(int *);
    u32 key;
    int i;

    if (VIDEO_BYTE(0) == 0)
        return 0x80243908;
    if (VIDEO_BYTE(2) == 0)
        return 0x80243902;
    if (sceUsbAccGetAuthStat() < 0)
        return 0x80243902;
    if (cmd == 0x8001)
        return sub_00006DA4(arg);

    key = (u32)cmd & 0x7FFFFFFF;
    for (i = 0; i < 22; i++) {
        if (s_cmd8F0C[i].cmd != key)
            continue;
        fn = (cmd < 0) ? s_cmd8F0C[i].getter : s_cmd8F0C[i].setter;
        if (fn == NULL)
            return 0x8024390D;
        return fn(arg);
    }
    return 0x8024390D;
}

/* 0x56B0 sceUsbCamIoctl - validates cmd/arg buffer size, K1-checks the
   user pointer, then hands off to sub_000055AC (auth, 0x8001, table walk). */
s32 sceUsbCamIoctl(int cmd, int *arg)
{
    int oldK1;
    s32 ret;
    int size;

    oldK1 = pspShiftK1();
    ret = 0x80243907;
    if (arg == NULL)
        goto out;

    if ((u32)cmd > 0x40000002u) {
        if ((u32)cmd <= 0x80000010u) {
            if ((u32)cmd < 0x8000000Du) {
                if ((u32)cmd == 0x8000000Au) {
                    size = 12;
                } else if ((u32)cmd > 0x8000000Au) {
                    size = 8;
                } else if ((u32)cmd + 0x7FFFFFFFu < 7u) {
                    size = 4;
                } else {
                    ret = 0x8024390D;
                    goto out;
                }
            } else {
                size = 4;
            }
        } else if ((u32)cmd <= 0x80000014u) {
            size = ((u32)cmd < 0x80000012u) ? 8 : 4;
        } else if ((u32)cmd < 0xC0000001u) {
            ret = 0x8024390D;
            goto out;
        } else if ((u32)cmd <= 0xC0000002u) {
            size = 8;
        } else if ((u32)cmd == 0xC0000003u) {
            size = 4;
        } else {
            ret = 0x8024390D;
            goto out;
        }
    } else if ((u32)cmd >= 0x40000001u) {
        size = 8;
    } else if ((u32)cmd < 13u) {
        if ((u32)cmd < 11u) {
            if (cmd == 9) {
                size = 4;
            } else if ((u32)cmd < 10u) {
                if ((u32)(cmd - 1) < 7u)
                    size = 4;
                else {
                    ret = 0x8024390D;
                    goto out;
                }
            } else {
                size = 12;
            }
        } else {
            size = 8;
        }
    } else if (cmd == 17) {
        size = 8;
    } else if ((u32)cmd < 17u) {
        size = 4;
    } else if ((u32)cmd < 21u) {
        size = 4;
    } else if (cmd == 0x8001) {
        size = 144;
    } else {
        ret = 0x8024390D;
        goto out;
    }

    ret = 0x80243904;
    if (!pspK1StaBufOk(arg, size))
        goto out;
    ret = sub_000055AC(cmd, arg);
out:
    pspSetK1(oldK1);
    return ret;
}


/* Helpers implemented outside this window. gen_usbcam.c still defines
   them as "s32 sub_0000XXXX(void)" stubs; those stubs must be removed
   (or replaced with these prototypes) when batchB2.c is merged. Every
   later batch must reuse these exact prototypes. */
int sub_00000000(int cmd, int arg1, void *buf, int len);
int sub_000000F4(void *buf);
int sub_00000170(void *arg);
int sub_00001058(int val);
int sub_00001084(int val);
int sub_000010B8(int val);
int sub_00001110(u8 *p);
int sub_0000115C(u8 *p);
int sub_000011A8(u8 *p);
int sub_000011F4(int curW, int curH, int val);
int sub_00002930(int val);
int sub_00002944(u8 *p);
int sub_00002988(int val);
int sub_0000299C(u8 *p);
int sub_000029E0(int val);
int sub_000029F4(u8 *p);
int sub_00002A38(int val);
int sub_00002A4C(u8 *p);
int sub_00002A90(int val);
int sub_00002AA4(u8 *p);
int sub_00002AE8(int val);
int sub_00002AFC(u8 *p);
int sub_00002B40(int val, u8 *out0, u8 *out1);
int sub_00002B7C(int val0, int val1, u8 *out0, u8 *out1);
int sub_00002C08(int val);

/* Mode code table at .rodata 0x8E1C; values are the identity map, but
   the original indexes it on both the set and the get path. */
static const u8 s_set10Codes[3] = { 0x00, 0x01, 0x02 };

int sub_00005870(int *arg)
{
    u8 buf[4] = { 0 };
    int res;

    if (arg[0] >= 10)
        return 0x80243905;
    if (arg[1] >= 10)
        return 0x80243905;
    res = sub_00002B7C(arg[0], arg[1], buf, buf + 1);
    if (res < 0)
        return res;
    res = sceKernelLockMutex(VIDEO_WORD(0x198), 1, NULL);
    if (res != 0)
        return res;
    res = sub_00000000(160, 2, buf, 2);
    sceKernelUnlockMutex(VIDEO_WORD(0x198), 1);
    if (res != 0)
        return res;
    VIDEO_BYTE(12) = buf[0];
    VIDEO_BYTE(13) = buf[1];
    return 0;
}

int sub_00005948(int *arg)
{
    u8 buf[4];
    int res;

    if ((u32)*arg >= 256)
        return 0x80243906;
    buf[0] = sub_00001084(*arg);
    res = sub_00000000(164, 2, buf, 1);
    if (res != 0)
        return res;
    VIDEO_BYTE(18) = *arg;
    return 0;
}

int sub_000059BC(int *arg)
{
    *arg = VIDEO_BYTE(18);
    return 0;
}

int sub_000059D0(int *arg)
{
    u8 buf[4];
    int res;

    if ((u32)*arg >= 256)
        return 0x80243906;
    buf[0] = sub_00001058(*arg);
    res = sceKernelLockMutex(VIDEO_WORD(0x198), 1, NULL);
    if (res != 0)
        return res;
    res = sub_00000000(165, 2, buf, 1);
    sceKernelUnlockMutex(VIDEO_WORD(0x198), 1);
    if (res != 0)
        return res;
    VIDEO_BYTE(19) = *arg;
    return 0;
}

int sub_00005A88(int *arg)
{
    *arg = VIDEO_BYTE(19);
    return 0;
}

int sub_00005A9C(int *arg)
{
    u8 buf[4];
    int res;

    if ((u32)*arg >= 256)
        return 0x80243906;
    buf[0] = *arg;
    res = sceKernelLockMutex(VIDEO_WORD(0x198), 1, NULL);
    if (res != 0)
        return res;
    res = sub_00000000(166, 2, buf, 1);
    sceKernelUnlockMutex(VIDEO_WORD(0x198), 1);
    if (res != 0)
        return res;
    VIDEO_BYTE(20) = *arg;
    return 0;
}

int sub_00005B4C(int *arg)
{
    *arg = VIDEO_BYTE(20);
    return 0;
}

int sub_00005B60(int *arg)
{
    u8 buf[4];
    int res;

    if ((u32)*arg >= 256)
        return 0x80243906;
    buf[0] = sub_000010B8(*arg);
    res = sceKernelLockMutex(VIDEO_WORD(0x198), 1, NULL);
    if (res != 0)
        return res;
    res = sub_00000000(167, 2, buf, 1);
    sceKernelUnlockMutex(VIDEO_WORD(0x198), 1);
    if (res != 0)
        return res;
    VIDEO_BYTE(21) = *arg;
    return 0;
}

int sub_00005C18(int *arg)
{
    *arg = VIDEO_BYTE(21);
    return 0;
}

int sub_00005C2C(int *arg)
{
    u8 buf[4];
    int curW;
    int curH;
    int res;
    int val;

    val = *arg;
    if (val != 0) {
        if ((u32)(val - 10) >= 71)
            return 0x80243905;
        curW = sub_00001110(&VIDEO_BYTE(12));
        curH = sub_0000115C(&VIDEO_BYTE(13));
        if (sub_000011F4(curW, curH, val) == 0)
            return 0x80243905;
        if (VIDEO_BYTE(14) >= 5 && (u32)(curW - 7) < 3)
            return 0x80243905;
    }
    buf[0] = *arg;
    res = sceKernelLockMutex(VIDEO_WORD(0x198), 1, NULL);
    if (res != 0)
        return res;
    res = sub_00000000(5, 2, buf, 1);
    sceKernelUnlockMutex(VIDEO_WORD(0x198), 1);
    if (res != 0)
        return res;
    VIDEO_BYTE(32) = buf[0];
    return 0;
}

int sub_00005D44(int *arg)
{
    *arg = VIDEO_BYTE(32);
    return 0;
}

int sub_00005D58(int *arg)
{
    u16 buf;

    buf = *(u16 *)arg;
    return sub_000000F4(&buf);
}

int sub_00005D7C(int *arg)
{
    return sub_00000170(arg);
}

int sub_00005D98(int *arg)
{
    u8 buf[4];
    int res;

    buf[0] = sub_00002930(*arg);
    res = sceKernelLockMutex(VIDEO_WORD(0x198), 1, NULL);
    if (res != 0)
        return res;
    res = sub_00000000(172, 2, buf, 1);
    sceKernelUnlockMutex(VIDEO_WORD(0x198), 1);
    if (res != 0)
        return res;
    VIDEO_BYTE(30) = buf[0];
    return 0;
}

int sub_00005E28(int *arg)
{
    *arg = sub_00002944(&VIDEO_BYTE(30));
    return 0;
}

int sub_00005E5C(int *arg)
{
    u8 buf[4] = { 0 };
    int res;

    res = sub_00002B40(*arg, buf, buf + 1);
    if (res < 0)
        return res;
    res = sceKernelLockMutex(VIDEO_WORD(0x198), 1, NULL);
    if (res != 0)
        return res;
    res = sub_00000000(160, 2, buf, 2);
    sceKernelUnlockMutex(VIDEO_WORD(0x198), 1);
    if (res != 0)
        return res;
    VIDEO_BYTE(12) = buf[0];
    VIDEO_BYTE(13) = buf[1];
    return 0;
}

int sub_00005F00(int *arg)
{
    arg[0] = sub_00001110(&VIDEO_BYTE(12));
    arg[1] = sub_0000115C(&VIDEO_BYTE(13));
    return 0;
}

int sub_00005F50(int *arg)
{
    u32 buf[2];
    int res;

    buf[0] = (*arg == 1) | (*(u16 *)(arg + 1) << 8);
    res = sceKernelLockMutex(VIDEO_WORD(0x198), 1, NULL);
    if (res != 0)
        return res;
    res = sub_00000000(168, 2, buf, 3);
    sceKernelUnlockMutex(VIDEO_WORD(0x198), 1);
    if (res != 0)
        return res;
    VIDEO_BYTE(23) = buf[0];
    VIDEO_HALF(24) = buf[0] >> 8;
    return 0;
}

int sub_00006020(int *arg)
{
    if (VIDEO_BYTE(23) == 1) {
        *arg = 1;
        *(u16 *)(arg + 1) = 0;
    } else {
        *arg = 0;
        *(u16 *)(arg + 1) = VIDEO_HALF(24);
    }
    return 0;
}

int sub_00006054(int *arg)
{
    u8 buf[4];
    int res;

    if ((u32)*arg >= 8)
        return 0x80243905;
    buf[0] = sub_00002988(*arg);
    res = sceKernelLockMutex(VIDEO_WORD(0x198), 1, NULL);
    if (res != 0)
        return res;
    res = sub_00000000(161, 2, buf, 1);
    sceKernelUnlockMutex(VIDEO_WORD(0x198), 1);
    if (res != 0)
        return res;
    VIDEO_BYTE(14) = buf[0];
    return 0;
}

int sub_00006100(int *arg)
{
    *arg = sub_0000299C(&VIDEO_BYTE(14));
    return 0;
}

int sub_00006134(int *arg)
{
    u8 buf[4];
    int res;

    if ((u32)*arg >= 4)
        return 0x80243905;
    buf[0] = sub_000029E0(*arg);
    res = sceKernelLockMutex(VIDEO_WORD(0x198), 1, NULL);
    if (res != 0)
        return res;
    res = sub_00000000(162, 2, buf, 1);
    sceKernelUnlockMutex(VIDEO_WORD(0x198), 1);
    if (res != 0)
        return res;
    VIDEO_BYTE(16) = buf[0];
    return 0;
}

int sub_000061E0(int *arg)
{
    *arg = sub_000029F4(&VIDEO_BYTE(16));
    return 0;
}

int sub_00006214(int *arg)
{
    u8 buf[4];
    int res;

    if ((u32)*arg >= 4)
        return 0x80243905;
    buf[0] = sub_00002A38(*arg);
    res = sceKernelLockMutex(VIDEO_WORD(0x198), 1, NULL);
    if (res != 0)
        return res;
    res = sub_00000000(163, 2, buf, 1);
    sceKernelUnlockMutex(VIDEO_WORD(0x198), 1);
    if (res != 0)
        return res;
    VIDEO_BYTE(17) = buf[0];
    return 0;
}

int sub_000062C0(int *arg)
{
    *arg = sub_00002A4C(&VIDEO_BYTE(17));
    return 0;
}

int sub_000062F4(int *arg)
{
    u8 buf[4];
    int res;

    if ((u32)*arg >= 3)
        return 0x80243905;
    buf[0] = sub_00002A90(*arg);
    res = sceKernelLockMutex(VIDEO_WORD(0x198), 1, NULL);
    if (res != 0)
        return res;
    res = sub_00000000(169, 2, buf, 1);
    sceKernelUnlockMutex(VIDEO_WORD(0x198), 1);
    if (res != 0)
        return res;
    VIDEO_BYTE(22) = buf[0];
    return 0;
}

int sub_000063A0(int *arg)
{
    *arg = sub_00002AA4(&VIDEO_BYTE(22));
    return 0;
}

int sub_000063D4(int *arg)
{
    u16 buf[2];
    int res;

    if ((u32)arg[0] + 0x8013 > 0x10026)
        return 0x80243907;
    if (arg[1] < -32787 || arg[1] > 32787)
        return 0x80243907;
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
    res = sceKernelLockMutex(VIDEO_WORD(0x198), 1, NULL);
    if (res != 0)
        return res;
    res = sub_00000000(170, 2, buf, 4);
    sceKernelUnlockMutex(VIDEO_WORD(0x198), 1);
    if (res != 0)
        return res;
    VIDEO_HALF(26) = *(u16 *)arg;
    VIDEO_HALF(28) = *(u16 *)(arg + 1);
    return 0;
}

int sub_000064FC(int *arg)
{
    u16 v;

    v = VIDEO_HALF(26);
    arg[0] = (v & 0x8000) ? 1 - (v & 0x7FFF) : v;
    v = VIDEO_HALF(28);
    arg[1] = (v & 0x8000) ? 1 - (v & 0x7FFF) : v;
    return 0;
}

int sub_00006560(int *arg)
{
    u8 buf[4];
    int res;

    buf[0] = (*arg != 0);
    res = sceKernelLockMutex(VIDEO_WORD(0x198), 1, NULL);
    if (res != 0)
        return res;
    res = sub_00000000(173, 2, buf, 1);
    sceKernelUnlockMutex(VIDEO_WORD(0x198), 1);
    if (res != 0)
        return res;
    VIDEO_BYTE(31) = buf[0];
    return 0;
}

int sub_000065F0(int *arg)
{
    *arg = VIDEO_BYTE(31);
    return 0;
}

int sub_00006604(int *arg)
{
    u8 buf[4];
    int res;

    if ((u32)*arg >= 3)
        return 0x80243905;
    buf[0] = sub_00002AE8(*arg);
    res = sceKernelLockMutex(VIDEO_WORD(0x198), 1, NULL);
    if (res != 0)
        return res;
    res = sub_00000000(174, 2, buf, 1);
    sceKernelUnlockMutex(VIDEO_WORD(0x198), 1);
    if (res != 0)
        return res;
    VIDEO_BYTE(15) = buf[0];
    return 0;
}

int sub_000066B0(int *arg)
{
    *arg = sub_00002AFC(&VIDEO_BYTE(15));
    return 0;
}

int sub_000066E4(int *arg)
{
    u8 buf[4];
    int res;

    if ((u32)*arg >= 17)
        return 0x80243905;
    buf[0] = sub_00002C08(*arg);
    res = sceKernelLockMutex(VIDEO_WORD(0x198), 1, NULL);
    if (res != 0)
        return res;
    res = sub_00000000(175, 2, buf, 1);
    sceKernelUnlockMutex(VIDEO_WORD(0x198), 1);
    if (res != 0)
        return res;
    VIDEO_BYTE(43) = buf[0];
    return 0;
}

int sub_00006790(int *arg)
{
    *arg = sub_000011A8(&VIDEO_BYTE(43));
    return 0;
}

int sub_000067C4(int *arg)
{
    u8 buf[4];

    *(u16 *)buf = *(u16 *)arg;
    buf[2] = *((u8 *)arg + 4);
    return sub_00000000(68, 3, buf, 3);
}

int sub_000067FC(int *arg)
{
    u8 buf[4];
    u32 outBits;
    u8 *bufPtr;
    int res;

    res = sceKernelLockMutex(VIDEO_WORD(0x198), 1, NULL);
    if ((u32)res - 0x800201A9u < 2)
        return 0x80243902;
    if (res < 0)
        return res;
    *(u16 *)buf = *(u16 *)arg;
    sceKernelClearEventFlag(VIDEO_WORD(0x190), 0xFFFEFFFF);
    res = sub_00000000(67, 3, buf, 2);
    if (res < 0)
        goto unlock;
    res = sceKernelWaitEventFlag(VIDEO_WORD(0x190), 0x400, 1, &outBits, NULL);
    if (res < 0) {
        Kprintf("%serror - at waiting for event at line %d): 0x%08x\n", "", 793, outBits);
        goto unlock;
    }
    if (outBits & 0x400) {
        res = 0x80243902;
        goto unlock;
    }
    if (VIDEO_WORD(0x1A8) != 0) {
        res = 0x80243913;
        goto unlock;
    }
    arg[1] = 0;
    bufPtr = (u8 *)VIDEO_WORD(0x6C);
    memcpy((u8 *)arg + 4, bufPtr + 4, bufPtr[3]);
unlock:
    sceKernelUnlockMutex(VIDEO_WORD(0x198), 1);
    return res;
}

int sub_0000694C(int *arg)
{
    u8 buf[4];

    buf[0] = *(u8 *)arg;
    buf[1] = *((u8 *)arg + 4);
    return sub_00000000(65, 2, buf, 2);
}

int sub_00006984(int *arg)
{
    u8 buf[4];
    u32 outBits;
    u8 *bufPtr;
    int res;

    res = sceKernelLockMutex(VIDEO_WORD(0x198), 1, NULL);
    if ((u32)res - 0x800201A9u < 2)
        return 0x80243902;
    if (res < 0)
        return res;
    buf[0] = *(u8 *)arg;
    sceKernelClearEventFlag(VIDEO_WORD(0x190), 0xFFFEFFFF);
    res = sub_00000000(64, 2, buf, 1);
    if (res < 0)
        goto unlock;
    res = sceKernelWaitEventFlag(VIDEO_WORD(0x190), 0x400, 1, &outBits, NULL);
    if (res < 0) {
        Kprintf("%serror - at waiting for event at line %d): 0x%08x\n", "", 866, outBits);
        goto unlock;
    }
    if (outBits & 0x400) {
        res = 0x80243902;
        goto unlock;
    }
    if (VIDEO_WORD(0x1A8) != 0) {
        res = 0x80243913;
        goto unlock;
    }
    arg[1] = 0;
    bufPtr = (u8 *)VIDEO_WORD(0x6C);
    memcpy((u8 *)arg + 4, bufPtr + 4, bufPtr[3]);
unlock:
    sceKernelUnlockMutex(VIDEO_WORD(0x198), 1);
    return res;
}

int sub_00006AD4(int *arg)
{
    u32 outBits;
    u8 *bufPtr;
    int res;

    res = sceKernelLockMutex(VIDEO_WORD(0x198), 1, NULL);
    if ((u32)res - 0x800201A9u < 2)
        return 0x80243902;
    if (res < 0)
        return res;
    sceKernelClearEventFlag(VIDEO_WORD(0x190), 0xFFFEFFFF);
    res = sub_00000000(9, 3, NULL, 0);
    if (res < 0)
        goto unlock;
    res = sceKernelWaitEventFlag(VIDEO_WORD(0x190), 0x400, 1, &outBits, NULL);
    if (res < 0) {
        Kprintf("%serror - at waiting for event at line %d): 0x%08x\n", "", 917, outBits);
        goto unlock;
    }
    if (outBits & 0x400) {
        res = 0x80243902;
        goto unlock;
    }
    bufPtr = (u8 *)VIDEO_WORD(0x6C);
    *arg = *(u16 *)(bufPtr + 4);
unlock:
    sceKernelUnlockMutex(VIDEO_WORD(0x198), 1);
    return res;
}

int sub_00006C0C(int *arg)
{
    u32 buf[2];
    u16 v1;
    u16 v2;
    int res;

    if ((u32)arg[0] >= 3)
        return 0x80243905;
    if ((u32)arg[1] > 0xFFFF)
        return 0x80243907;
    if ((u32)arg[2] > 0xFFFF)
        return 0x80243907;
    v1 = *(u16 *)(arg + 1);
    v2 = *(u16 *)(arg + 2);
    buf[0] = s_set10Codes[arg[0]] | ((u32)v1 << 8) | ((u32)v2 << 24);
    buf[1] = v2 >> 8;
    res = sceKernelLockMutex(VIDEO_WORD(0x198), 1, NULL);
    if (res != 0)
        return res;
    res = sub_00000000(6, 2, buf, 5);
    sceKernelUnlockMutex(VIDEO_WORD(0x198), 1);
    if (res != 0)
        return res;
    VIDEO_BYTE(33) = buf[0];
    VIDEO_HALF(34) = buf[0] >> 8;
    VIDEO_HALF(36) = (buf[0] >> 24) | ((buf[1] & 0xFF) << 8);
    return 0;
}

int sub_00006D3C(int *arg)
{
    int i;

    for (i = 0; i < 3; i++) {
        if (VIDEO_BYTE(33) == s_set10Codes[i])
            break;
    }
    if (i == 3)
        return 0x80243905;
    arg[0] = i;
    arg[1] = VIDEO_HALF(34);
    arg[2] = VIDEO_HALF(36);
    return 0;
}

/* 0x6DA4 sub_00006DA4 */
s32 sub_00006DA4(int *arg)
{
    u32 outBits;
    int res;

    res = sceKernelLockMutex(VIDEO_WORD(0x198), 1, NULL);
    if ((u32)res - 0x800201A9u < 2)
        return 0x80243902;
    if (res < 0)
        return res;
    sceKernelClearEventFlag(VIDEO_WORD(0x190), 0xFFFEFFFF);
    res = sub_00000000(arg[1], arg[2], &arg[4], arg[3]);
    if (res < 0)
        goto unlock;
    if (arg[0] == 0)
        goto unlock;
    res = sceKernelWaitEventFlag(VIDEO_WORD(0x190), 0x10400, 1, &outBits, NULL);
    if (res < 0) {
        Kprintf("%serror - at waiting for event at line %d): 0x%08x\n", "", 1030, outBits);
        goto unlock;
    }
    if (outBits & 0x400) {
        res = 0x80243902;
        goto unlock;
    }
    __builtin_memcpy((u8 *)arg + 4, (u8 *)VIDEO_WORD(0x6C) + 4, 64);
unlock:
    sceKernelUnlockMutex(VIDEO_WORD(0x198), 1);
    return res;
}


/* 0x6F78 sceUsbCamSetupMicEx */
int sceUsbCamSetupMicEx(struct UsbCamSetupMicExParam *param, void *workarea, int wasize)
{
    u16 cmd[7];
    int oldK1;
    int ret;

    if (MIC_BYTE(0) == 0)
        return 0x80243908;
    if (MIC_BYTE(2) == 0)
        return 0x80243902;
    if (sceUsbAccGetAuthStat() < 0)
        return 0x80243902;
    if ((u32)wasize < 264)
        return 0x8024390A;
    if (param == NULL)
        return 0x80243904;

    oldK1 = pspShiftK1();
    ret = 0x80243904;
    if (!pspK1StaBufOk(param, 36))
        goto out;
    if (workarea == NULL)
        goto out;
    if (!pspK1DynBufOk(workarea, wasize))
        goto out;
    /* Original falls straight into the epilogue here without pspSetK1. */
    if (param->freq != 48000 && param->freq != 44100 &&
        param->freq != 22050 && param->freq != 11025)
        return 0x8024390B;

    cmd[0] = (u16)param->alc;
    cmd[1] = (u16)param->gain;
    cmd[2] = (u16)param->unk2[0];
    cmd[3] = (u16)param->unk2[1];
    cmd[4] = (u16)param->unk2[2];
    cmd[5] = (u16)param->unk2[3];
    cmd[6] = (u16)(param->freq / 1000);
    ret = sub_000078A0(cmd, param->unk3, workarea, wasize);
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
    ret = 0x80243908;
    if (MIC_BYTE(0) == 0)
        goto out;
    ret = 0x80243902;
    if (MIC_BYTE(2) == 0)
        goto out;
    if (sceUsbAccGetAuthStat() < 0)
        goto out;
    ret = 0x8024390A;
    if ((u32)wasize < 264)
        goto out;
    ret = 0x80243904;
    if (param == NULL)
        goto out;
    if (!pspK1StaBufOk(param, 20))
        goto out;
    if (workarea == NULL)
        goto out;
    if (!pspK1DynBufOk(workarea, wasize))
        goto out;
    ret = 0x8024390B;
    if (param->freq != 44100 && param->freq != 22050 && param->freq != 11025)
        goto out;

    cmd[0] = (u16)param->alc;
    cmd[1] = (u16)param->gain;
    cmd[2] = (u16)param->noize;
    cmd[3] = 0;
    cmd[4] = 3;
    cmd[5] = 2;
    cmd[6] = (u16)(param->freq / 1000);
    ret = sub_000078A0(cmd, 0, workarea, wasize);
out:
    pspSetK1(oldK1);
    return ret;
}


/* 0x7278 sceUsbCamStopMic */
s32 sceUsbCamStopMic(void)
{
    int oldK1;
    s32 ret;

    if (MIC_BYTE(0) == 0)
        return 0x80243908;
    if (MIC_BYTE(2) == 0)
        return 0x80243902;
    if (sceUsbAccGetAuthStat() < 0)
        return 0x80243902;
    if (MIC_BYTE(1) == 0)
        return 0x80243901;
    oldK1 = pspShiftK1();
    ret = sub_000079A0();
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
    ret = 0x80243908;
    if (MIC_BYTE(0) == 0)
        goto out;
    if (MIC_BYTE(5) != 0) {
        ret = sceKernelWaitEventFlag(MIC_WORD(0x114), 0x500, 1, &outBits, NULL);
        if (ret < 0)
            goto out;
        if (outBits & 0x100) {
            ret = 0x80243902;
            goto out;
        }
        sceKernelClearEventFlag(MIC_WORD(0x114), 0xFBFF);
    }
    intr = sceKernelCpuSuspendIntr();
    ret = sub_00007AF8(buf, size);
    if (ret < 0)
        goto resume;
    if (MIC_BYTE(4) == 4) {
        ret = 0x80243909;
        goto resume;
    }
    sceKernelClearEventFlag(MIC_WORD(0x114), 0xFFFFFFFD);
    MIC_WORD(0x120) = size;
    MIC_WORD(0x124) = (u32)buf;
    MIC_BYTE(4) = 4;
    ret = sceKernelSetEventFlag(MIC_WORD(0x114), 4);
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
    ret = 0x80243908;
    if (MIC_BYTE(0) == 0)
        goto out;
    if (MIC_BYTE(5) != 0) {
        ret = sceKernelWaitEventFlag(MIC_WORD(0x114), 0x500, 1, &outBits, NULL);
        if (ret < 0)
            goto out;
        if (outBits & 0x100) {
            ret = 0x80243902;
            goto out;
        }
        sceKernelClearEventFlag(MIC_WORD(0x114), 0xFBFF);
    }
    intr = sceKernelCpuSuspendIntr();
    ret = sub_00007AF8(buf, size);
    if (ret < 0)
        goto resume;
    if (MIC_BYTE(4) == 4) {
        /* Original jumps straight to the epilogue: sceKernelCpuResumeIntr
           is skipped and interrupts stay disabled. */
        ret = 0x80243909;
        goto out;
    }
    sceKernelClearEventFlag(MIC_WORD(0x114), 0xFFFFFFFD);
    MIC_WORD(0x120) = size;
    MIC_WORD(0x124) = (u32)buf;
    MIC_BYTE(4) = 4;
    ret = sceKernelSetEventFlag(MIC_WORD(0x114), 4);
resume:
    sceKernelCpuResumeIntr(intr);
    if (ret < 0)
        goto out;
    ret = sceKernelWaitEventFlag(MIC_WORD(0x114), 0x102, 1, &outBits, NULL);
    if (ret < 0)
        goto out;
    if (outBits & 0x100) {
        ret = 0x80243902;
        goto out;
    }
    ret = MIC_WORD(0x34);
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
    ret = 0x80243908;
    if (MIC_BYTE(0) == 0)
        goto out;
    ret = 0x80243902;
    if (MIC_BYTE(2) == 0)
        goto out;
    if (sceUsbAccGetAuthStat() < 0)
        goto out;
    ret = 0x80243901;
    if (MIC_BYTE(1) == 0)
        goto out;

    ret = sceKernelWaitEventFlag(MIC_WORD(0x114), 0x102, 1, &outBits, NULL);
    if (ret < 0)
        goto out;
    if (outBits & 0x100) {
        ret = 0x80243902;
        goto out;
    }
    ret = MIC_WORD(0x34);
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
    ret = 0x80243908;
    if (MIC_BYTE(0) == 0)
        goto out;
    ret = 0x80243902;
    if (MIC_BYTE(2) == 0)
        goto out;
    if (sceUsbAccGetAuthStat() < 0)
        goto out;
    ret = 0x80243901;
    if (MIC_BYTE(1) == 0)
        goto out;

    ret = sceKernelPollEventFlag(MIC_WORD(0x114), 0x102, 1, &outBits);
    if (ret == (s32)0x800201AF) {
        ret = 0x8024390E;
        goto out;
    }
    if (ret < 0)
        goto out;
    if (outBits & 0x100) {
        ret = 0x80243902;
        goto out;
    }
    ret = MIC_WORD(0x34);
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
    ret = 0x80243908;
    if (MIC_BYTE(0) == 0)
        goto out;
    ret = 0x80243902;
    if (MIC_BYTE(2) == 0)
        goto out;
    if (sceUsbAccGetAuthStat() < 0)
        goto out;
    ret = 0x80243901;
    if (MIC_BYTE(1) == 0)
        goto out;
    ret = MIC_WORD(0x34);
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
    MIC_BYTE(5) = 1;
    ret = 0;
    sceKernelClearEventFlag(MIC_WORD(0x114), 0xFBFF);

    ret = 0x80243908;
    if (MIC_BYTE(0) == 0)
        goto out;
    ret = 0x80243902;
    if (MIC_BYTE(2) == 0)
        goto out;
    if (sceUsbAccGetAuthStat() < 0)
        goto out;
    ret = 0x80243901;
    if (MIC_BYTE(1) == 0)
        goto out;

    ret = 0;
    if ((s16)MIC_HALF(0x0A) != (s16)gain) {
        MIC_HALF(0x0A) = (u16)gain;
        ret = sub_00007A3C();
        MIC_BYTE(5) = 0;
        sceKernelSetEventFlag(MIC_WORD(0x114), 0x400);
    }
out:
    pspSetK1(oldK1);
    return ret;
}


/* 0x78A0 sub_000078A0 */
s32 sub_000078A0(void *cmd, int flag, void *workarea, int wasize)
{
    int intr;

    intr = sceKernelCpuSuspendIntr();
    __builtin_memcpy(&MIC_BYTE(8), cmd, 14);
    MIC_WORD(0x1C) = wasize;
    MIC_WORD(0x18) = (u32)workarea;
    MIC_BYTE(6) = (flag != 0);
    if (flag != 0) {
        MIC_WORD(0x30) = (u32)workarea;
        MIC_WORD(0x24) = 0;
        MIC_WORD(0x28) = 0;
        MIC_WORD(0x2C) = 0;
        MIC_WORD(0x20) = (u32)wasize / 132;
    } else {
        MIC_WORD(0x30) = (u32)workarea;
        MIC_WORD(0x20) = wasize;
        MIC_WORD(0x24) = 0;
        MIC_WORD(0x28) = (u32)workarea;
        MIC_WORD(0x2C) = (u32)workarea;
    }
    sceKernelCpuResumeIntr(intr);
    MIC_BYTE(1) = 1;
    return 0;
}


/* 0x79A0 sub_000079A0 */
s32 sub_000079A0(void)
{
    struct MicStateFull *st = (struct MicStateFull *)&g_micState;
    int intr;
    s32 ret;

    intr = sceKernelCpuSuspendIntr();
    if (sceUsbAccGetAuthStat() != 0) {
        ret = 0x80243902;
        goto out;
    }
    if (st->reqD.retcode > 0) {
        ret = 0x80243006;
        goto out;
    }
    ((u8 *)st->unk60)[2] = 2;
    ((u8 *)st->unk60)[3] = 0;
    ret = sceUsbAccIntrInReq(&st->reqD);
    if (ret < 0)
        goto out;
    st->unk4 = 2;
    ret = 0;
out:
    sceKernelCpuResumeIntr(intr);
    return ret;
}


/* 0x7A3C sub_00007A3C */
s32 sub_00007A3C(void)
{
    u32 outBits;
    s32 ret;

    if (MIC_BYTE(4) == 4) {
        ret = sceKernelWaitEventFlag(MIC_WORD(0x114), 0x102, 1, &outBits, NULL);
        if (ret < 0)
            return ret;
        if (outBits & 0x100)
            return 0x80243902;
    }
    sub_000079A0();
    ret = sceKernelWaitEventFlag(MIC_WORD(0x114), 0x120, 1, &outBits, NULL);
    if (ret < 0)
        return ret;
    if (outBits & 0x100)
        return 0x80243902;
    return sub_00007F0C();
}


/* 0x7AF8 sub_00007AF8 */
s32 sub_00007AF8(void *buf, int size)
{
    if (MIC_BYTE(0) == 0)
        return 0x80243908;
    if (MIC_BYTE(2) == 0)
        return 0x80243902;
    if (sceUsbAccGetAuthStat() < 0)
        return 0x80243902;
    if (MIC_BYTE(1) == 0)
        return 0x80243901;
    if (MIC_BYTE(4) == 0 || MIC_BYTE(4) == 2)
        return 0x80243912;
    if (buf == NULL)
        return 0x80243907;
    if (!pspK1DynBufOk(buf, size))
        return 0x80243904;
    if (MIC_BYTE(6) != 0) {
        if ((u32)size < 132)
            return 0x80243903;
    } else {
        if ((u32)size < 2)
            return 0x80243903;
        if (size & 1)
            return 0x80243903;
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
    if (MIC_BYTE(6) != 0) {
        MIC_WORD(0x2C) = 0;
        MIC_WORD(0x24) = 0;
        MIC_WORD(0x28) = 0;
    } else {
        buf = MIC_WORD(0x30);
        MIC_WORD(0x24) = 0;
        MIC_WORD(0x28) = buf;
        MIC_WORD(0x2C) = buf;
    }
    ret = sub_00007F0C();
    pspSetK1(oldK1);
    return ret;
}


s32 sub_00007C54(int arg0 __attribute__((unused)), int arg1 __attribute__((unused)))
{
    if (sceUsbbdRegister(&g_micDriver) < 0) {
        return 1;
    }
    g_micState.unk0 = 0;
    return 0;
}

s32 sub_00007C8C(int arg0 __attribute__((unused)), int arg1 __attribute__((unused)))
{
    return sceUsbbdUnregister(&g_micDriver) < 0;
}


/* 0x7CB0 sub_00007CB0 - mic endpoint receive completion callback
   (installed as g_micState.reqs[i].func, gen line 993). */
void sub_00007CB0(struct UsbdDeviceReq *req)
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
    if (MIC_BYTE(4) == 0 || MIC_BYTE(4) == 2)
        return;

    if (MIC_WORD(0x18) == 0) {
        if (MIC_BYTE(4) == 1 || MIC_BYTE(4) == 4) {
            sceKernelDcacheInvalidateRange(req->data, 256);
            sceUsbbdReqRecv(req);
        }
        return;
    }

    recvsize = (u32)req->recvsize;
    if (MIC_BYTE(6) != 0) {
        blk = (u8 *)(MIC_WORD(0x30) + MIC_WORD(0x2C) * 132);
        payload = (u16)(recvsize - 2);
        blk[0] = ((u8 *)req->data)[0];
        blk[1] = ((u8 *)req->data)[1];
        *(u16 *)(blk + 2) = payload;
        memcpy(blk + 4, (u8 *)req->data + 2, payload);
        memset(blk + 4 + payload, 0, 128 - payload);

        MIC_WORD(0x2C) = MIC_WORD(0x2C) + 1;
        if (MIC_WORD(0x2C) >= MIC_WORD(0x20))
            MIC_WORD(0x2C) = 0;
        if (MIC_WORD(0x24) >= MIC_WORD(0x20)) {
            MIC_WORD(0x28) = MIC_WORD(0x2C);
            avail = MIC_WORD(0x24);
        } else {
            MIC_WORD(0x24) = MIC_WORD(0x24) + 1;
            avail = MIC_WORD(0x24);
        }
    } else {
        writePtr = MIC_WORD(0x2C);
        start = MIC_WORD(0x30);
        end = start + (MIC_WORD(0x20) & ~1u);
        reqEnd = writePtr + (recvsize & ~1u);

        if (end < reqEnd) {
            first = reqEnd - end;
            sub_0000808C((void *)writePtr, req->data, (int)(recvsize - first));
            sub_0000808C((void *)MIC_WORD(0x30),
                         (u8 *)req->data + (recvsize - first), (int)first);
            newWrite = MIC_WORD(0x30) + (first & ~1u);
        } else {
            sub_0000808C((void *)writePtr, req->data, (int)recvsize);
            newWrite = writePtr + (recvsize & ~1u);
        }

        MIC_WORD(0x24) = MIC_WORD(0x24) + recvsize;
        if (MIC_WORD(0x20) < MIC_WORD(0x24))
            MIC_WORD(0x24) = MIC_WORD(0x20);
        MIC_WORD(0x2C) = newWrite;
        if (MIC_WORD(0x20) == MIC_WORD(0x24))
            MIC_WORD(0x28) = MIC_WORD(0x2C);
        avail = MIC_WORD(0x24);
    }

    if (avail != 0)
        sceKernelSetEventFlag(MIC_WORD(0x114), 1);
    if (MIC_BYTE(4) == 1 || MIC_BYTE(4) == 4) {
        sceKernelDcacheInvalidateRange(req->data, 256);
        sceUsbbdReqRecv(req);
    }
}

/* 0x7F0C sub_00007F0C - arm the mic receive path: guards under
   cpu-suspend, build the setup packet in unk60, submit reqD. */
s32 sub_00007F0C(void)
{
    struct MicStateFull *st = (struct MicStateFull *)&g_micState;
    s32 intr;
    s32 ret;
    u16 buf[8];
    void *src;
    int breq;
    int len;
    int i;

    intr = sceKernelCpuSuspendIntr();
    ret = 0x80243908;
    if (st->unk0 == 0)
        goto out;
    ret = 0x80243902;
    if (st->unk2 == 0)
        goto out;
    if (sceUsbAccGetAuthStat() < 0)
        goto out;
    ret = 0x80243901;
    if (st->unk1 == 0)
        goto out;
    ret = 0x80243911;
    if (MIC_WORD(0x18) == 0)
        goto out;
    ret = 0x80243909;
    if (st->unk4 != 0)
        goto out;
    ret = 0x80243006;
    if (st->reqD.retcode > 0)
        goto out;

    if (MIC_BYTE(6) != 0) {
        for (i = 0; i < 7; i++)
            buf[i] = ((u16 *)&st->unk8)[i];
        ((u8 *)buf)[14] = 1;
        src = buf;
        breq = 3;
        len = 15;
    } else {
        src = &st->unk8;
        breq = 1;
        len = 14;
    }

    ((u8 *)st->unk60)[2] = breq;
    ((u8 *)st->unk60)[3] = len;
    memcpy((u8 *)st->unk60 + 4, src, len);
    ret = sceUsbAccIntrInReq(&st->reqD);
    if (ret < 0)
        goto out;
    st->unk4 = 3;
    st->unk3 = 1;
    ret = 0;
out:
    sceKernelCpuResumeIntr(intr);
    return ret;
}


/* 0x808C sub_0000808C - copy helper: plain memcpy, or per-16-bit-swap
   copy when g_micState.unk128 ("16 aligned data swap") is set. */
void *sub_0000808C(void *dst, void *src, int size)
{
    u32 *d = (u32 *)dst;
    u32 *s = (u32 *)src;
    u32 n;
    u32 i;

    if (MIC_WORD(0x128) == 0) {
        memcpy(dst, src, size);
    } else {
        n = (u32)size >> 2;
        for (i = 0; i < n; i++)
            d[i] = ((s[i] >> 8) & 0x00FF00FFu) | ((s[i] << 8) & 0xFF00FF00u);
    }
    return dst;
}


/* 0x80F8 sub_000080F8 - drain the mic buffer into dst under cpu-suspend.
   Last function in the driver's .text (0x80F8-0x8334). */
s32 sub_000080F8(void *dst, int size)
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

    if (MIC_BYTE(6) != 0) {
        n = (u32)size / 132;
        if (n == 0) {
            ret = 0;
            goto out;
        }
        if (MIC_WORD(0x24) < n)
            n = MIC_WORD(0x24);
        d = dst;
        for (i = 0; i < n; i++) {
            __builtin_memcpy(d, (void *)(MIC_WORD(0x30) + MIC_WORD(0x28) * 132),
                             132);
            d += 132;
            idx = MIC_WORD(0x28) + 1;
            MIC_WORD(0x28) = (idx < MIC_WORD(0x20)) ? idx : 0;
            MIC_WORD(0x24) = MIC_WORD(0x24) - 1;
        }
        if (MIC_WORD(0x24) == 0)
            sceKernelClearEventFlag(MIC_WORD(0x114), -2);
        ret = n * 132;
    } else {
        avail = MIC_WORD(0x24);
        n = (u32)size;
        if (avail < n)
            n = avail;
        rptr = MIC_WORD(0x28);
        limit = MIC_WORD(0x30) + (MIC_WORD(0x20) & ~1u);
        dptr = (u32)dst;
        half = n >> 1;
        for (i = 0; i < half; i++) {
            *(u16 *)dptr = *(u16 *)rptr;
            rptr += 2;
            if (rptr >= limit)
                rptr = MIC_WORD(0x30);
            dptr += 2;
        }
        MIC_WORD(0x28) = rptr;
        MIC_WORD(0x24) = avail - n;
        if (avail - n == 0)
            sceKernelClearEventFlag(MIC_WORD(0x114), -2);
        ret = n;
    }
out:
    sceKernelCpuResumeIntr(intr);
    return ret;
}

