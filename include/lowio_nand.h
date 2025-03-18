/* Copyright (C(); 2014 The uOFW team
   See the file COPYING for copying permission.
*/

#ifndef LOWIO_NAND_H
#define LOWIO_NAND_H

#include "common_header.h"

typedef struct {
	u8	user_ecc[3]; //0
	u8	reserved0; //3
	u8	block_fmt; //4
	u8	block_stat; //5
	union {
		u16 lbn;
		struct {
			u8 idx;
			u8 ver;
		} IdStorId;
	};
	union {
		u32 	id;	/* 0x38 0x4a 0xc6 0x6d for IPL area */
		struct {
			u8 formatted;
			u8 readonly;
		} IdStorInfo;
	};
	u8	spare_ecc[2];
	u8	reserved1[2];
} SceNandSpare_t;

typedef enum {
	USER_ECC_IN_SPARE	= 0x01,
	NO_AUTO_USER_ECC	= 0x10,
	NO_AUTO_SPARE_ECC	= 0x20
} SceNandEccMode_t;

s32 sceNandIsBadBlock(u32 ppn);
s32 sceNandWriteAccess(u32 ppn, void *user, void *spare, u32 len, SceNandEccMode_t mode);
s32 sceNandSetScramble(u32 scramble);
s32 sceNandVerifyEcc(u8 *buf, u16 ecc);
s32 sceNandEraseAllBlock(void);
s32 sceNandDetectChipMakersBBM(u32 ppn);
s32 sceNandDumpWearBBMSize(void);
s32 sceNandUnlock(void);
s32 sceNandReadExtraOnly(u32, void*, u32);
s32 sceNandVerifyBlockWithRetry(u32 ppn, void *user, void *spare);
s32 sceNandWriteBlock(u32 ppn, void *user, void *spare);
//s32 sceNandReadAccess();
//s32 sceNandReset();
//s32 sceNandSetWriteProtect();
s32 sceNandCorrectEcc(u8 *buf, u16 ecc);
s32 sceNandWritePagesRawExtra(u32 ppn, void *user, void *spare, u32 len);
s32 sceNandEraseBlockWithRetry(u32 ppn);
s32 sceNandReadPages(u32 ppn, void *user, void *spare, u32 len);
s32 sceNandWritePages(u32 ppn, void *user, void *spare, u32 len);
s32 sceNandTestBlock(u32 ppn);
s32 sceNandLock(s32);
s32 sceNandGetPagesPerBlock(void);
s32 sceNandWriteBlockWithVerify(u32 ppn, void *user, void *spare);
//s32 sceNandCollectEcc();
s32 sceNandWritePagesRawAll(u32 ppn, void *user, void *spare, u32 len);
s32 sceNandGetTotalBlocks(void);
s32 sceNandDoMarkAsBadBlock(u32 ppn);
s32 sceNandReadBlockWithRetry(u32 ppn, void *user, void *spare);
s32 sceNandReadPagesRawAll(u32 ppn, void *user, void *spare, u32 len);
s32 sceNandGetPageSize(void);
s32 sceNandDetectChip(void);
s32 sceNandReadPagesRawExtra(u32 ppn, void *user, void *spare, u32 len);
//s32 sceNandReadStatus();
s32 sceNandEraseBlock(u32);
s32 sceNandCountChipMakersBBM(void);
s32 sceNandCalcEcc(u8 *buf);
//s32 sceNandReadId();

#endif	/* LOWIO_NAND_H */

