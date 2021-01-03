/* Copyright (C) 2011, 2012 The uOFW team
   See the file COPYING for copying permission.
*/

#ifndef _CHNNLSV_H
#define _CHNNLSV_H

typedef struct {
	u32 mode;
	u32 unk4;
	u8 data[16];
} SceSdCtx1;

typedef struct {
	u32 mode;
	u8 data[16];
	u8 key[16];
	u32 size;
} SceSdCtx2;

typedef struct {
	u8 kirk_buf1[0x14];	//0x00001A80
	u8 kirk_buf2[0x82C];	//0x00001A94
	u8 kirk_buf3[0x14];	//0x000022C0
	u8 kirk_buf4[0x800];	//0x000022D4
	SceUID sema1;		//0x00002AD4
	SceUID sema2;		//0x00002AD8
} g_chnnlsv_struct;

g_chnnlsv_struct g_chnnlsv = {
	{0},
	{0},
	{0},
	{0},
	0,
	0
};

//0x0000198C
const u8 SceSdKey1[16] = {
	0xFA, 0xAA, 0x50, 0xEC, 0x2F, 0xDE, 0x54, 0x93, 0xAD, 0x14, 0xB2, 0xCE, 0xA5, 0x30, 0x05, 0xDF
};

//0x0000199C
const u8 SceSdKey2[16] = {
	0x36, 0xA5, 0x3E, 0xAC, 0xC5, 0x26, 0x9E, 0xA3, 0x83, 0xD9, 0xEC, 0x25, 0x6C, 0x48, 0x48, 0x72
};

//0x000019AC
const u8 SceSdKey3[16] = {
	0xD8, 0xC0, 0xB0, 0xF3, 0x3E, 0x6B, 0x76, 0x85, 0xFD, 0xFB, 0x4D, 0x7D, 0x45, 0x1E, 0x92, 0x03
};

//0x000019BC
const u8 SceSdKey4[16] = {
	0xCB, 0x15, 0xF4, 0x07, 0xF9, 0x6A, 0x52, 0x3C, 0x04, 0xB9, 0xB2, 0xEE, 0x5C, 0x53, 0xFA, 0x86
};

//0x000019CC
const u8 SceSdKey5[16] = {
	0x70, 0x44, 0xA3, 0xAE, 0xEF, 0x5D, 0xA5, 0xF2, 0x85, 0x7F, 0xF2, 0xD6, 0x94, 0xF5, 0x36, 0x3B
};

//0x000019DC
const u8 SceSdKey6[16] = {
	0xEC, 0x6D, 0x29, 0x59, 0x26, 0x35, 0xA5, 0x7F, 0x97, 0x2A, 0x0D, 0xBC, 0xA3, 0x26, 0x33, 0x00
};

s32 sceUtilsBufferCopyWithRange(void* outBuf, s32 outsize, void* inBuf, s32 insize, s32 cmd);
s32 _SdCrypt(void *buf, u8 *data, u32 size, u8 *data2, u8 *unk4, u32 mode);
s32 _kirk4(void *buf, u32 size, u32 scramble_code);
s32 _kirk7(void *buf, u32 size, u32 scramble_code);
s32 _kirk5(void *buf, u32 size);
s32 _kirk8(void *buf, u32 size);
s32 _kirk4Xor(void *buf, u32 size, u8 *data, u32 scramble_code);
s32 _kirk7Xor(void *buf, u32 size, u8 *data, u32 scramble_code);
s32 _kirk5Xor(void *buf, u32 size, u8 *data);
s32 _kirk8Xor(void *buf, u32 size, u8 *data);
s32 _kirk14(void *buf);

#define SCE_CHNNLSV_ERROR_ILLEGAL_SIZE                          0xFFFFFBFE
#define SCE_CHNNLSV_ERROR_ILLEGAL_ALIGNMENT_SIZE                0xFFFFFBFF
#define SCE_CHNNLSV_ERROR_KIRK_14_ERROR                         0xFFFFFEFB
#define SCE_CHNNLSV_ERROR_SEMA_ERROR                            0xFFFFFEFC
#define SCE_CHNNLSV_ERROR_ILLEGAL_ADDR                          0xFFFFFEFD
#define SCE_CHNNLSV_ERROR_KIRK_IV_FUSE_ERROR                    0xFFFFFEFE
#define SCE_CHNNLSV_ERROR_KIRK_IV_ERROR                         0xFFFFFEFF

#endif /* _CHNNLSV_H */
