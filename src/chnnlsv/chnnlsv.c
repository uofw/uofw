/* Copyright (C) 2011, 2012 The uOFW team
   See the file COPYING for copying permission.
*/
/**
 * Big thanks to Hykem for most of the info used here.
 * http://www.emunewz.net/forum/showthread.php?tid=3673
 */

#include <common_imp.h>
#include <threadman_kernel.h>
#include "chnnlsv.h"

SCE_MODULE_INFO("sceChnnlsv", SCE_MODULE_KERNEL | SCE_MODULE_KIRK_SEMAPHORE_LIB | SCE_MODULE_ATTR_EXCLUSIVE_LOAD
                              | SCE_MODULE_ATTR_EXCLUSIVE_START, 1, 4);
SCE_SDK_VERSION(SDK_VERSION);

//Subroutine _SdCrypt - Address 0x00000000
s32 _SdCrypt(void *buf, u8 *data, u32 size, u8 *data2, u8 *unk4, u32 mode)
{
	s32 ret;

	u32 scramble_code;

	u8 block1[16], block2[16];

	u32 i, j;

	for (i = 0; i < 16; i++) //0x00000030
		((u8 *)buf)[20 + i] = data2[i];

	if (mode == 6) { //0x00000050
		scramble_code = 100;

		for (i = 0; i < 16; i++) //0x000003AC
			((u8 *)buf)[20 + i] ^= SceSdKey6[i];

		ret = _kirk8(buf, 16);

		for (i = 0; i < 16; i++) //0x000003EC
			((u8 *)buf)[i] ^= SceSdKey5[i];
	} else if (mode == 4) { //0x00000058
		scramble_code = 87;

		for (i = 0; i < 16; i++) //0x00000330
			((u8 *)buf)[20 + i] ^= SceSdKey3[i];

		ret = _kirk8(buf, 16);

		for (i = 0; i < 16; i++) //0x00000370
			((u8 *)buf)[i] ^= SceSdKey2[i];
	} else if (mode == 2) { //0x00000060
		ret = _kirk8(buf, 16);

		scramble_code = 83;
	} else if (mode == 1) { //0x00000068
		ret = _kirk7(buf, 16, 4);

		scramble_code = 83;
	} else if (mode == 3) { //0x00000070
		scramble_code = 87;

		for (i = 0; i < 16; i++) //0x00000280
			((u8 *)buf)[20 + i] ^= SceSdKey3[i];

		ret = _kirk7(buf, 16, 14);

		for (i = 0; i < 16; i++) //0x000002C4
			((u8 *)buf)[i] ^= SceSdKey2[i];
	} else {
		scramble_code = 100;

		for (i = 0; i < 16; i++) //0x00000084
			((u8 *)buf)[20 + i] ^= SceSdKey6[i];

		ret = _kirk7(buf, 16, 18);

		for (i = 0; i < 16; i++) //0x000000C8
			((u8 *)buf)[i] ^= SceSdKey5[i];
	}

	if (ret == 0) { //0x000000EC
		for (i = 0; i < 16; i++) //0x000000F8
			block2[i] = ((u8 *)buf)[i];

		u32 unk4_int = ((u32 *)unk4)[0];

		if (unk4_int != 1) { //0x0000011C
			for (i = 0; i < 12; i++) //0x00000124
				block1[i] = block2[i];

			block1[12] = unk4[0] - 1;
			block1[13] = (unk4_int - 1) >> 8;
			block1[14] = (unk4_int - 1) >> 16;
			block1[15] = (unk4_int - 1) >> 24;
		} else {
			for (i = 0; i < 16; i++) //0x00000254
				block1[i] = 0;
		}

		if (size > 0) { //0x0000016C
			for (i = 20; i < (size + 20); i += 16) { //0x0000017C
				for (j = 0; j < 12; j++) //0x00000184
					((u8 *)buf)[i + j] = block2[j];

				((u8 *)buf)[i + 12] = unk4[0];
				((u8 *)buf)[i + 13] = unk4[1];
				((u8 *)buf)[i + 14] = unk4[2];
				((u8 *)buf)[i + 15] = unk4[3];
				((u32 *)unk4)[0]++;
			}
		}

		ret = _kirk7Xor(buf, size, block1, scramble_code);

		if (ret == 0) { //0x000001F0
			if (ret < (s32)size) { //0x000001FC
				for (i = 0; i < size; i++) //0x00000204
					data[i] ^= ((u8 *)buf)[i];
			}
		}
	}

	return ret;
}

//Subroutine module_start - Address 0x00000418
s32 module_start(SceSize args __attribute__((unused)), void *argp __attribute__((unused)))
{
	s32 ret = 1;

	g_chnnlsv.sema1 = sceKernelCreateSema("SceChnnlsv1", 0, 1, 1, NULL);

	if (g_chnnlsv.sema1 > 0) { //0x00000460
		g_chnnlsv.sema2 = sceKernelCreateSema("SceChnnlsv2", 0, 1, 1, NULL);

		if (g_chnnlsv.sema2 < 1) //0x00000478
			sceKernelDeleteSema(g_chnnlsv.sema1);
		else
			ret = SCE_ERROR_OK;
	}

	return ret;
}

//Subroutine module_stop - Address 0x000004A4
s32 module_stop(SceSize args __attribute__((unused)), void *argp __attribute__((unused)))
{
	s32 ret = 1;

	SceUInt timeout = 1000000;

	if (sceKernelWaitSema(g_chnnlsv.sema1, 1, &timeout) == 0) { //0x000004D8
		sceKernelDeleteSema(g_chnnlsv.sema1);

		if (sceKernelWaitSema(g_chnnlsv.sema2, 1, &timeout) == 0) { //0x00000510
			sceKernelDeleteSema(g_chnnlsv.sema2);

			ret = SCE_ERROR_OK;
		}
	}

	return ret;
}

/**
 * Initialize the SceSdCtx2 struct and set the mode.
 *
 * @param ctx Pointer to the SceSdCtx2 struct
 * @param mode One of the modes whichs sets the scramble key for kirk.
 *
 * @return SCE_ERROR_OK on initialization success.
 * @return SCE_CHNNLSV_ERROR_ILLEGAL_ADDR if ctx cannot be accessed from the current context.
 *
 */
//Subroutine sceSdSetIndex - Address 0x00000528
s32 sceSdSetIndex(SceSdCtx2 *ctx, s32 mode)
{
	s32 oldK1 = pspShiftK1();

	s32 ret = SCE_CHNNLSV_ERROR_ILLEGAL_ADDR;

	u32 i;

	if (pspK1StaBufOk(ctx, sizeof(SceSdCtx2))) { //0x0000053C
		ctx->mode = mode;

		for (i = 0; i < 16; i++) //0x0000054C
			ctx->data[i] = 0;

		for (i = 0; i < 16; i++) //0x00000568
			ctx->key[i] = 0;

		ctx->size = 0;

		ret = SCE_ERROR_OK;
	}

	pspSetK1(oldK1);

	return ret;
}

/**
 * Generates a hash storing the result into ctx->data and updates ctx->key
 *
 * @param ctx Pointer to the SceSdCtx2 struct
 * @param data Pointer to the data used in hash generation
 * @param size The size of the data used for hash generation
 *
 * @return SCE_ERROR_OK on success
 * @return SCE_CHNNLSV_ERROR_ILLEGAL_ADDR if ctx/data cannot be accessed from the current context.
 * @return SCE_CHNNLSV_ERROR_SEMA_ERROR wait/signal sema error
 * @return SCE_CHNNLSV_ERROR_ILLEGAL_SIZE if ctx->size > 16
 *
 */
//Subroutine sceSdRemoveValue - Address 0x0000058C
s32 sceSdRemoveValue(SceSdCtx2 *ctx, u8 *data, u32 size)
{
	s32 oldK1 = pspShiftK1();

	s32 ret = SCE_CHNNLSV_ERROR_ILLEGAL_ADDR;

	u32 scramble_code;

	u32 i;

	//0x000005DC & 0x000005F0
	if (pspK1DynBufOk(data, size) && pspK1StaBufOk(ctx, sizeof(SceSdCtx2))) {
		if (sceKernelWaitSema(g_chnnlsv.sema1, 1, NULL) == 0) { //0x00000644
			if (ctx->size < 17) { //0x00000658
				if ((ctx->size + size) < 17) { //0x00000668
					if (size != 0) { //0x000007E0
						for (i = 0; i < size; i++) //0x000007E8
							ctx->key[ctx->size + i] = data[i];
					}

					ctx->size += size;
					ret = SCE_ERROR_OK;
				} else {
					if (ctx->mode == 6) //0x00000674
						scramble_code = 17;
					else if (ctx->mode == 4) //0x00000680
						scramble_code = 13;
					else if (ctx->mode == 2) //0x0000068C
						scramble_code = 5;
					else if (ctx->mode == 1) //0x00000698
						scramble_code = 3;
					else {
						scramble_code = 12;

						if ((ctx->mode ^ 3) != 0)
							scramble_code = 16;
					}

					if (ctx->size != 0) { //0x000006B0
						for (i = 0; i < ctx->size; i++) //0x000006C0
							g_chnnlsv.kirk_buf2[i] = ctx->key[i];
					}

					u32 oldsize = ctx->size;
					u32 tempsize = (ctx->size + size) & 0xF;
					tempsize = (tempsize == 0) ? 16 : tempsize;
					ctx->size = tempsize;
					u32 newsize = size - tempsize;

					if (tempsize != 0) { //0x00000700
						for (i = 0; i < tempsize; i++) //0x00000714
							ctx->key[i] = data[newsize + i];
					}

					ret = SCE_ERROR_OK;

					if (newsize != 0) { //0x00000730
						for (i = 0; i < newsize; i++) {
							if (oldsize == 2048) { //0x00000750
								ret = _kirk4Xor(g_chnnlsv.kirk_buf1, 2048, ctx->data, scramble_code);

								if (ret != 0) //0x000007D0
									goto signalsema;

								oldsize = 0;
							}

							g_chnnlsv.kirk_buf2[oldsize] = data[i];
							oldsize++;
						}
					}

					if (oldsize != 0) //0x0000077C
						ret = _kirk4Xor(g_chnnlsv.kirk_buf1, oldsize, ctx->data, scramble_code);
				}
			} else
				ret = SCE_CHNNLSV_ERROR_ILLEGAL_SIZE;

signalsema:
			if (sceKernelSignalSema(g_chnnlsv.sema1, 1) != 0) //0x0000078C
				ret = SCE_CHNNLSV_ERROR_SEMA_ERROR;
		} else
			ret = SCE_CHNNLSV_ERROR_SEMA_ERROR;
	}

	pspSetK1(oldK1);

	return ret;
}

/**
 * Generates a hash based on the context collected by sceSdRemoveValue,
 * the results of which are stored into the SAVEDATA_PARAMS field of PARAM.SFO
 *
 * @param ctx Pointer to the SceSdCtx2 struct
 * @param hash The end result of the hash generated by this function is stored here
 * @param key If provided, this key will also be used in the encryption process
 *
 * @return SCE_ERROR_OK on success
 * @return SCE_CHNNLSV_ERROR_ILLEGAL_ADDR if ctx/hash/key cannot be accessed from the current context.
 * @return SCE_CHNNLSV_ERROR_SEMA_ERROR wait/signal sema error
 * @return SCE_CHNNLSV_ERROR_ILLEGAL_SIZE if ctx->size > 16
 *
 */
//Subroutine sceSdGetLastIndex - Address 0x00000824
s32 sceSdGetLastIndex(SceSdCtx2 *ctx, u8 *hash, u8 *key)
{
	s32 oldK1 = pspShiftK1();

	s32 ret = SCE_CHNNLSV_ERROR_ILLEGAL_ADDR;

	u8 block1[16], block2[16];

	u8 xorbyte, shiftbyte1, shiftbyte2;

	u32 scramble_code;

	u32 i;

	//0x00000870, 0x00000884 & 0x00000898
	if (pspK1StaBufOk(hash, 16) && pspK1StaBufOk(key, 16) && pspK1StaBufOk(ctx, sizeof(SceSdCtx2))) {
		if (sceKernelWaitSema(g_chnnlsv.sema1, 1, NULL) == 0) { //0x000008EC
			if (ctx->size < 17) { //0x000008FC
				if (ctx->mode == 6) //0x0000090C
					scramble_code = 17;
				else if (ctx->mode == 4) //0x00000918
					scramble_code = 13;
				else if (ctx->mode == 2) //0x00000924
					scramble_code = 5;
				else if (ctx->mode == 1) //0x00000930
					scramble_code = 3;
				else {
					scramble_code = 12;

					if ((ctx->mode ^ 3) != 0)
						scramble_code = 16;
				}

				for (i = 0; i < 16; i++) //0x00000950
					g_chnnlsv.kirk_buf2[i] = 0;

				ret = _kirk4(g_chnnlsv.kirk_buf1, 16, scramble_code);

				if (ret == 0) { //0x0000097C
					for (i = 0; i < 16; i++) //0x0000098C
						block1[i] = g_chnnlsv.kirk_buf2[i];

					xorbyte = (block1[0] & 0x80) ? 135 : 0;

					for (i = 0; i < 15; i++) { //0x000009C0
						shiftbyte1 = block1[i] << 1;
						shiftbyte2 = block1[i + 1] >> 7;
						block1[i] = shiftbyte1 | shiftbyte2;
					}

					shiftbyte1 = block1[15] << 1;
					block1[15] = shiftbyte1 ^ xorbyte;

					if (ctx->size != 16) { //0x000009FC
						xorbyte = (block1[0] & 0x80) ? 135 : 0;

						for (i = 0; i < 15; i++) { //0x00000A1C
							shiftbyte1 = block1[i] << 1;
							shiftbyte2 = block1[i + 1] >> 7;
							block1[i] = shiftbyte1 | shiftbyte2;
						}

						shiftbyte1 = block1[15] << 1;
						block1[15] = shiftbyte1 ^ xorbyte;

						ctx->key[ctx->size] = -128;

						if ((ctx->size + 1) < 16) { //0x00000A64
							for (i = (ctx->size + 1); i < 16; i++) //0x00000A74
								ctx->key[i] = 0;
						}
					}

					for (i = 0; i < 16; i++) //0x00000A90
						ctx->key[i] ^= block1[i];

					for (i = 0; i < 16; i++) //0x00000ABC
						g_chnnlsv.kirk_buf2[i] = ctx->key[i];

					for (i = 0; i < 16; i++) //0x00000AE4
						block2[i] = ctx->data[i];

					ret = _kirk4Xor(g_chnnlsv.kirk_buf1, 16, block2, scramble_code);

					if (ret == 0) { //0x00000B18
						if ((ctx->mode - 3) < 2) { //0x00000B2C
							for (i = 0; i < 16; i++) //0x00000B40
								block2[i] ^= SceSdKey1[i];
						} else if ((ctx->mode - 5) < 2) { //0x00000D20
							for (i = 0; i < 16; i++) //0x00000D34
								block2[i] ^= SceSdKey4[i];
						}

						u32 xor1 = (ctx->mode ^ 2) < 1;
						u32 xor2 = ((ctx->mode ^ 4) < 1) | xor1;

						//0x00000B78 & 0x00000B84
						if ((xor2 != 0) || (ctx->mode == 6)) {
							for (i = 0; i < 16; i++) //0x00000CA0
								g_chnnlsv.kirk_buf2[i] = block2[i];

							ret = _kirk5(g_chnnlsv.kirk_buf1, 16);

							if (ret != 0) //0x00000CD0
								goto signalsema;

							ret = _kirk4(g_chnnlsv.kirk_buf1, 16, scramble_code);

							if (ret != 0) //0x00000CE8
								goto signalsema;

							for (i = 0; i < 16; i++) //0x00000CF8
								block2[i] = g_chnnlsv.kirk_buf2[i];
						}

						if (key != 0) { //0x00000B8C
							for (i = 0; i < 16; i++) //0x00000B98
								block2[i] ^= key[i];

							for (i = 0; i < 16; i++) //0x00000BC0
								g_chnnlsv.kirk_buf2[i] = block2[i];

							ret = _kirk4(g_chnnlsv.kirk_buf1, 16, scramble_code);

							if (ret != 0) //0x00000BF4
								goto signalsema;

							for (i = 0; i < 16; i++) //0x00000C04
								block2[i] = g_chnnlsv.kirk_buf2[i];
						}

						for (i = 0; i < 16; i++) //0x00000C24
							hash[i] = block2[i];

						for (i = 0; i < 16; i++) //0x00000C44
							ctx->data[i] = 0;

						for (i = 0; i < 16; i++) //0x00000C60
							ctx->key[i] = 0;

						ctx->size = 0;
						ctx->mode = 0;
						ret = SCE_ERROR_OK;
					}
				}
			} else
				ret = SCE_CHNNLSV_ERROR_ILLEGAL_SIZE;

signalsema:
			if (sceKernelSignalSema(g_chnnlsv.sema1, 1) != 0) //0x00000C80
				ret = SCE_CHNNLSV_ERROR_SEMA_ERROR;
		} else
			ret = SCE_CHNNLSV_ERROR_SEMA_ERROR;
	}

	pspSetK1(oldK1);

	return ret;
}

/**
 * The main key generating function, 1 for encryption, 2 for decryption
 *
 * @param ctx Pointer to the SceSdCtx1 struct
 * @param mode Different public keys/kirk commands will be used depending on the mode specified
 * @param genMode Specify whether encryption (1) or decryption (1) should be used
 * @param data Pointer to some data used for encryption/decryption
 * @param key If specified, this key will be used as the private key for encryption/decryption
 *
 * @return SCE_ERROR_OK on success
 * @return SCE_CHNNLSV_ERROR_ILLEGAL_ADDR if ctx/data/key cannot be accessed from the current context.
 * @return SCE_CHNNLSV_ERROR_SEMA_ERROR wait/signal sema error
 *
 */
//Subroutine sceSdCreateList - Address 0x00000D60
s32 sceSdCreateList(SceSdCtx1 *ctx, int mode, int genMode, u8 *data, u8 *key)
{
	s32 oldK1 = pspShiftK1();

	s32 ret = SCE_CHNNLSV_ERROR_ILLEGAL_ADDR;

	u32 i;

	//0x00000DB0, 0x00000DC4 & 0x00000DD8
	if (pspK1StaBufOk(data, 16) && pspK1StaBufOk(key, 16) && pspK1StaBufOk(ctx, sizeof(SceSdCtx1))) {
		if (sceKernelWaitSema(g_chnnlsv.sema2, 1, NULL) == 0) { //0x00000E18
			ctx->mode = mode;
			ctx->unk4 = 1;

			if (genMode == 2) { //0x00000E3C
				for (i = 0; i < 16; i++) //0x000011B8
					ctx->data[i] = data[i];

				if (key != 0) { //0x000011D4
					for (i = 0; i < 16; i++) //0x000011DC
						ctx->data[i] ^= key[i];
				}

				ret = SCE_ERROR_OK;
			} else if (genMode == 1) { //0x00000E44
				ret = _kirk14(g_chnnlsv.kirk_buf3);

				if (ret == 0) { //0x00000E70
					for (i = 15; (int)i >= 0; i--) //0x00000E7C
						g_chnnlsv.kirk_buf4[i] = g_chnnlsv.kirk_buf3[i];

					for (i = 12; i < 16; i++) //0x00000E98
						g_chnnlsv.kirk_buf4[i] = 0;

					if (ctx->mode == 6) { //0x00000EB8
						for (i = 0; i < 16; i++) //0x0000113C
							g_chnnlsv.kirk_buf4[i] ^= SceSdKey5[i];

						ret = _kirk5(g_chnnlsv.kirk_buf3, 16);

						for (i = 0; i < 16; i++) //0x00001184
							g_chnnlsv.kirk_buf4[i] ^= SceSdKey6[i];
					} else if (ctx->mode == 4) { //0x00000EC0
						for (i = 0; i < 16; i++) //0x000010B4
							g_chnnlsv.kirk_buf4[i] ^= SceSdKey2[i];

						ret = _kirk5(g_chnnlsv.kirk_buf3, 16);

						for (i = 0; i < 16; i++) //0x000010FC
							g_chnnlsv.kirk_buf4[i] ^= SceSdKey3[i];
					} else if (ctx->mode == 2) { //0x00000EC8
						ret = _kirk5(g_chnnlsv.kirk_buf4, 16);
					} else if (ctx->mode == 1) { //0x00000ED0
						ret = _kirk4(g_chnnlsv.kirk_buf4, 16, 4);
					} else if (ctx->mode == 3) { //0x00000ED8
						for (i = 0; i < 16; i++) //0x00000FF4
							g_chnnlsv.kirk_buf4[i] ^= SceSdKey2[i];

						ret = _kirk4(g_chnnlsv.kirk_buf3, 16, 14);

						for (i = 0; i < 16; i++) //0x00001040
							g_chnnlsv.kirk_buf4[i] ^= SceSdKey3[i];
					} else {
						for (i = 0; i < 16; i++) //0x00000EEC
							g_chnnlsv.kirk_buf4[i] ^= SceSdKey5[i];

						ret = _kirk4(g_chnnlsv.kirk_buf3, 16, 18);

						for (i = 0; i < 16; i++) //0x00000F38
							g_chnnlsv.kirk_buf4[i] ^= SceSdKey6[i];
					}

					if (ret == 0) { //0x00000F60
						for (i = 0; i < 16; i++) //0x00000F6C
							ctx->data[i] = g_chnnlsv.kirk_buf4[i];

						for (i = 0; i < 16; i++) //0x00000F90
							data[i] = g_chnnlsv.kirk_buf4[i];

						if (key != 0) { //0x00000FB0
							for (i = 0; i < 16; i++) //0x00000FB8
								ctx->data[i] ^= key[i];
						}
					}
				}
			}

			if (sceKernelSignalSema(g_chnnlsv.sema2, 1) != 0)
				ret = SCE_CHNNLSV_ERROR_SEMA_ERROR;
		} else
			ret = SCE_CHNNLSV_ERROR_SEMA_ERROR;
	}

	pspSetK1(oldK1);

	return ret;
}

/**
 * The main encryption/decryption function, the size of "data" must be 16 byte aligned
 *
 * @param ctx Pointer to the SceSdCtx1 struct
 * @param data Pointer to the data used in the encryption/decryption process
 * @param size The size of "data"
 *
 * @return SCE_ERROR_OK on success
 * @return SCE_CHNNLSV_ERROR_ILLEGAL_ADDR if ctx/data cannot be accessed from the current context.
 * @return SCE_CHNNLSV_ERROR_SEMA_ERROR wait/signal sema error
 * @return SCE_CHNNLSV_ERROR_ILLEGAL_ALIGNMENT_SIZE if the size of "data" is not 16 byte aligned
 *
 */
//Subroutine sceSdSetMember - Address 0x00001208
s32 sceSdSetMember(SceSdCtx1 *ctx, u8 *data, u32 size)
{
	s32 oldK1 = pspShiftK1();

	s32 ret = SCE_CHNNLSV_ERROR_ILLEGAL_ADDR;

	//0x00001258 & 0x0000126C
	if (pspK1DynBufOk(data, size) && pspK1StaBufOk(ctx, sizeof(SceSdCtx1))) {
		if (sceKernelWaitSema(g_chnnlsv.sema2, 1, NULL) == 0) { //0x000012C0
			if (size != 0) { //0x00001338
				if ((size & 0xF) == 0) { //0x000012D4
					u32 length = 0;

					while (size >= 2048) { //0x000012E0
						ret = _SdCrypt(g_chnnlsv.kirk_buf3, data + length, 2048, ctx->data, (u8 *)&ctx->unk4, ctx->mode);

						size -= 2048;
						length += 2048;

						if (ret != 0) //0x0000131C
							goto signalsema;
					}

					if (size != 0) //0x0000132C
						ret = _SdCrypt(g_chnnlsv.kirk_buf3, data + length, size, ctx->data, (u8 *)&ctx->unk4, ctx->mode);
				} else
					ret = SCE_CHNNLSV_ERROR_ILLEGAL_ALIGNMENT_SIZE;
			} else
				ret = SCE_ERROR_OK;

signalsema:
			if (sceKernelSignalSema(g_chnnlsv.sema2, 1) != 0) //0x00001338
				ret = SCE_CHNNLSV_ERROR_SEMA_ERROR;
		} else
			ret = SCE_CHNNLSV_ERROR_SEMA_ERROR;
	}

	pspSetK1(oldK1);

	return ret;
}

/**
 * Initialize the SceSdCtx1 struct.
 *
 * @param ctx Pointer to the SceSdCtx1 struct
 *
 * @return SCE_ERROR_OK on initialization success.
 * @return SCE_CHNNLSV_ERROR_ILLEGAL_ADDR if ctx cannot be accessed from the current context.
 *
 */
//Subroutine sceSdCleanList - Address 0x00001380
s32 sceSdCleanList(SceSdCtx1 *ctx)
{
	s32 oldK1 = pspShiftK1();

	s32 ret = SCE_CHNNLSV_ERROR_ILLEGAL_ADDR;

	u32 i;

	if (pspK1StaBufOk(ctx, sizeof(SceSdCtx1))) { //0x00001394
		for (i = 0; i < 16; i++) //0x000013A0
			ctx->data[i] = 0;

		ctx->unk4 = 0;
		ctx->mode = 0;
		ret = SCE_ERROR_OK;
	}

	pspSetK1(oldK1);

	return ret;
}

//Subroutine _kirk4 - Address 0x000013C8
s32 _kirk4(void *buf, u32 size, u32 scramble_code)
{
	((u32 *)buf)[3] = scramble_code;
	((u32 *)buf)[0] = 4;
	((u32 *)buf)[1] = 0;
	((u32 *)buf)[2] = 0;
	((u32 *)buf)[4] = size;

	size += 20;

	s32 ret = SCE_CHNNLSV_ERROR_KIRK_IV_ERROR;

	if (sceUtilsBufferCopyWithRange(buf, size, buf, size, 4) == 0)
		ret = SCE_ERROR_OK;

	return ret;
}

//Subroutine _kirk7 - Address 0x00001418
s32 _kirk7(void *buf, u32 size, u32 scramble_code)
{
	((u32 *)buf)[3] = scramble_code;
	((u32 *)buf)[0] = 5;
	((u32 *)buf)[1] = 0;
	((u32 *)buf)[2] = 0;
	((u32 *)buf)[4] = size;

	size += 20;

	s32 ret = SCE_CHNNLSV_ERROR_KIRK_IV_ERROR;

	if (sceUtilsBufferCopyWithRange(buf, size, buf, size, 7) == 0)
		ret = SCE_ERROR_OK;

	return ret;
}

//Subroutine _kirk5 - Address 0x00001468
s32 _kirk5(void *buf, u32 size)
{
	((u32 *)buf)[0] = 4;
	((u32 *)buf)[3] = 256;
	((u32 *)buf)[1] = 0;
	((u32 *)buf)[2] = 0;
	((u32 *)buf)[4] = size;

	size += 20;

	s32 ret = SCE_CHNNLSV_ERROR_KIRK_IV_FUSE_ERROR;

	if (sceUtilsBufferCopyWithRange(buf, size, buf, size, 5) == 0)
		ret = SCE_ERROR_OK;

	return ret;
}

//Subroutine _kirk8 - Address 0x000014BC
s32 _kirk8(void *buf, u32 size)
{
	((u32 *)buf)[0] = 5;
	((u32 *)buf)[3] = 256;
	((u32 *)buf)[1] = 0;
	((u32 *)buf)[2] = 0;
	((u32 *)buf)[4] = size;

	size += 20;

	s32 ret = SCE_CHNNLSV_ERROR_KIRK_IV_FUSE_ERROR;

	if (sceUtilsBufferCopyWithRange(buf, size, buf, size, 8) == 0)
		ret = SCE_ERROR_OK;

	return ret;
}

//Subroutine _kirk4Xor - Address 0x00001510
s32 _kirk4Xor(void *buf, u32 size, u8 *data, u32 scramble_code)
{
	s32 ret;

	u32 i;

	for (i = 0; i < 16; i++) //0x00001538
		((u8 *)buf)[20 + i] ^= data[i];

	ret = _kirk4(buf, size, scramble_code);

	if (ret == 0) { //0x00001568
		for (i = 0; i < 16; i++) //0x00001578
			data[i] = ((u8 *)buf)[size + 4 + i];
	}

	return ret;
}

//Subroutine _kirk7Xor - Address 0x000015B0
s32 _kirk7Xor(void *buf, u32 size, u8 *data, u32 scramble_code)
{
	s32 ret;

	u32 i;

	u8 block[16];

	for (i = 0; i < 16; i++) //0x000015D4
		block[i] = ((u8 *)buf)[size + 4 + i];

	ret = _kirk7(buf, size, scramble_code);

	if (ret == 0) { //0x000015F8
		for (i = 0; i < 16; i++) //0x00001604
			((u8 *)buf)[i] ^= data[i];

		for (i = 0; i < 16; i++) //0x0000162C
			data[i] = block[i];
	}

	return ret;
}

//NOTE: seems to be unused.
//Subroutine _kirk5Xor - Address 0x00001660
s32 _kirk5Xor(void *buf, u32 size, u8 *data)
{
	s32 ret;

	u32 i;

	for (i = 0; i < 16; i++) //0x00001684
		((u8 *)buf)[20 + i] ^= data[i];

	ret = _kirk5(buf, size);

	if (ret == 0) { //0x000016B4
		for (i = 0; i < 16; i++) //0x000016C4
			data[i] = ((u8 *)buf)[size + i];
	}

	return ret;
}

//NOTE: seems to be unused.
//Subroutine _kirk8Xor - Address 0x000016FC
s32 _kirk8Xor(void *buf, u32 size, u8 *data)
{
	s32 ret;

	u32 i;

	u8 block[16];

	for (i = 0; i < 16; i++) //0x0000171C
		block[i] = ((u8 *)buf)[size + 4 + i];

	ret = _kirk8(buf, size);

	if (ret == 0) { //0x00001740
		for (i = 0; i < 16; i++) //0x0000174C
			((u8 *)buf)[i] ^= data[i];

		for (i = 0; i < 16; i++) //0x00001774
			data[i] = block[i];
	}

	return ret;
}

//Subroutine _kirk14 - Address 0x000017A8
s32 _kirk14(void *buf)
{
	s32 ret = SCE_CHNNLSV_ERROR_KIRK_14_ERROR;

	if (sceUtilsBufferCopyWithRange(buf, 20, 0, 0, 14) == 0)
		ret = SCE_ERROR_OK;

	return ret;
}
