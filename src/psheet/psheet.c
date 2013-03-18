#include <pspkernel.h>
#include <pspkerneltypes.h>

#define SCE_PSHEET_ERROR_SEMAID 0x80510108
#define SCE_PSHEET_ERROR_ILLEGAL_ADDresultS 0x80510109
#define SCE_PSHEET_ERROR_P 0x80510111
#define SCE_PSHEET_ERROR_P2 0x80510110
#define SCE_PSHEET_ERROR_P3 0x80510102
#define SCE_PSHEET_ERROR_P4 0x80510300
#define SCE_PSHEET_ERROR_P5 0x80510101
#define SCE_PSHEET_ERROR_P6 0x80510110
#define SCE_PSHEET_ERROR_P7 0x80510113
#define SCE_PSHEET_ERROR_P8 0x80510112

PSP_MODULE_INFO("scePsheet", SCE_MODULE_KERNEL | SCE_MODULE_SINGLE_LOAD | SCE_MODULE_SINGLE_START, 1, 5);

SceUID semaID = 0;			//00002a40
SceUID unk1 = 0;			//00002a44
SceUInt semaTimeout = 0;	//00002a48
int unk3 = 0;				//00002a4c
int buf[4] = {0,0,0,0};		//00002a50	-	00002a5c
int something = 0;			//00002a60

int g_unkP;				//0x00002A80
int cipher;				//0x00002F20
int cipher0;			//0x00002AA0
int cipher1;			//0x00002AA4
int p;					//0x00002E80
int p0;					//0x00002E84
int p1;					//0x00002E88
int p2;					//0x00002E8C
int p3;					//0x00002E90
int p4;					//0x00002EA0
int p5;					//0x00002EB0
int p6;					//0x00002EC0
int p7;					//0x00002EC8
int p8;					//0x00002ED0
int p9;					//0x00002ED4
int p10;				//0x00002ED8
int p11;				//0x00002EF0
int p12;				//0x00002F08
int p13;				//0x00002F20
int p14;				//0x00002F48
int p15;				//0x00002F4C
int p16;				//0x00003284
int p17;				//0x000032DC
int p18;				//0x00003804
int p19;				//0x00004804

//OK
//scePsheet_driver_302AB4B8 = sceDRMInstallInit
//sets up the initial DRM params.
//returns 0 on success, 0x80510109 on failure
int sceDRMInstallInit(int addresults, int size) {
	int crntAddresults, crntSize, k1;
	k1 = pspSdkGetK1();
	pspSdkSetK1(k1 << 11);
	crntAddresults = addresults;
	crntSize = size;
	sceKernelMemset(g_unkP, 0, 0x4D0);

	//Standard illegal addresults check
	if (((crntAddresults | crntSize | crntAddresults + crntSize) & k1) < 0)
			{
		pspSdkSetK1(k1);
		return SCE_PSHEET_ERROR_ILLEGAL_ADDresultS;
	}

	sub_01310(0, 1);
	sub_01410(0);
	buf[0] = addresults;
	buf[1] = size;

	if (((crntAddresults < 1) | (((0x0001FFFF < crntSize)) ^ 1)) != 0) {
		buf[0] = 0;
		buf[1] = 0;
		pspSdkSetK1(k1);
		return SCE_PSHEET_ERROR_ILLEGAL_ADDresultS;
	}

	pspSdkSetK1(k1);
	return 0;
}

//OK
//scePsheet_driver_15355B0E = sceDRMInstallGetPkgInfo
//determines what kind of package it is, based on hash comparison.
int sceDRMInstallGetPkgInfo(int addresults, int size, int a2)
{
	int crntAddresults, crntSize ,s2;
	SceUID s3;
	int k1 = pspSdkGetK1();
	crntAddresults = addresults;
	crntSize = size;
	s2 = a2;
	if(!(buf == 0))
		{
		if((addresults < 1 | size < 1) != 0)
			return SCE_PSHEET_ERROR_ILLEGAL_ADDresultS;
		if(a2 == 0)
			return SCE_PSHEET_ERROR_ILLEGAL_ADDresultS;
		pspSdkSetK1(k1 << 11);
		if(!(((k1<<11) & addresults) < 0))
			{
			if(!((((size+16) | size) & (k1 << 11)) < 0))
				{
				if(!(((a2+16) | a2 ) & (k1 << 11)) >= 0)
					{
					addresults = SCE_PSHEET_ERROR_ILLEGAL_ADDresultS;
					size = 0;
					goto end;
					}
					sub_1410(0);
					s3 = sub_14A8(addresults);
					addresults = s3;
					if(s3 < 0)
						{
						size = 0;
						addresults = s3;
						goto end;
						}
					s3 = sub_72C(addresults, size, s2);
					addresults = sceIoClose(s3);
					size = 0;
					goto end;
				}
				addresults = SCE_PSHEET_ERROR_ILLEGAL_ADDresultS;
				size = 0;
				goto end;
			}
			addresults = SCE_PSHEET_ERROR_ILLEGAL_ADDresultS;
			size = 0;
			end:
			sceKernelMemset(g_unkP, 0, 0x4D0);
			pspSdkSetK1(k1);
			return crntAddresults;
			}
	return SCE_PSHEET_ERROR_SEMAID;
}

//
//scePsheet_driver_34E68A41 = sceDRMInstallGetFileInfo
//determines what kind of file it is, based on hash comparison.
int sceDRMInstallGetFileInfo(char *file, int size, int a2, int a3)
{
	SceUID s0;
	int crntSize = size, s2 = a3, k1;
	SceUID s4;
	int s5 = a2;
	if(!(buf = 0))
	{
		if(file == NULL || size == 0 || a2 == NULL)
			return SCE_PSHEET_ERROR_ILLEGAL_ADDresultS;
		k1 = pspSdkGetK1();
		if(!(s2 < 0))
		{
			if(!(((k1<<11) & file) < 0))
			{
				if(!((((size+16) | size) & size) < 0))
				{
					if((((s2+264) | s2) & (k1<<11)) >= 0)
					{
						pspSdkSetK1(k1);
						s4 = sceIoOpen(file, IOASSIGN_RDONLY, 0);
						s0 = k1 << 11;
						pspSdkSetK1(s0);
						s0 = s4;
							if(!(s4 < 0))
							{
							sub_1410(0);
							sub_864(s4, size, s5, s2);
							sceIoClose(s4);
							goto end;
							}
						goto end;
					}
					s0 = SCE_PSHEET_ERROR_ILLEGAL_ADDresultS;
					goto end;
				}
				s0 = SCE_PSHEET_ERROR_ILLEGAL_ADDresultS;
				goto end;
			}
			pspSdkSetK1(k1 << 11);
			s0 = SCE_PSHEET_ERROR_ILLEGAL_ADDresultS;
			end:
			size = 0;
			sceKernelMemset(g_unkP, 0, 0x4D0);
			pspSdkSetK1(k1);
			return s0;
		}
		return SCE_PSHEET_ERROR_ILLEGAL_ADDresultS;
	}
	return SCE_PSHEET_ERROR_SEMAID;
}

int sceDRMInstallInstall(char a0[256], int a1, int a2, int *a3) {
	a3 = buf[0];		//buf pointer to buf
	char s0[256] = a0;	//buf
	int s1 = a1, s2 = a2, s3, s4 = -1, s5, s6, s7;
	if(a3 == 0)
		return SCE_PSHEET_ERROR_SEMAID;



	return 0;
}

//OK
int module_start(SceSize argc, void* argp)
{
	SceUID s0;
	SceUID *s1 = semaID;
	s0 = sceKernelCreateSema("ScePsheet1", 0, 1, 1, 0);
	unk1 = s0;
	if(!(s0 < 0))
		{
		sceDRMInstallInit(0,0);
		return 0;
		}	
	if(!(*(s1+4) <= 0))//unk1 <= 0
		{
		sceKernelDeleteSema(s1+4);
		}
	sceKernelMemset(s1, 0, 64);
	sceKernelMemset(g_unkP, 0, 0x4D0);
	return s0+1;//(s0-(-1))
}

//OK
//module_reboot_before
int module_reboot_before(SceSize args, void *argp) __attribute__((alias("module_stop")))
{
	SceUID *s0 = semaID;
	if(!(semaID < 0))
		{
		sceKernelDeleteSema(semaID);
		}
	if(!(unk1 <= 0))
		{
		sceKernelDeleteSema(unk1);
		}
	sceKernelMemset(s0, 0, 64);
	sceKernelMemset(g_unkP, 0, 0x4D0);
	return 0;
}

//OK
//scePsheet_driver_3CEC4078 = sceDRMInstallEnd
//cleans up the internal param struct
int sceDRMInstallEnd(void)
{
sceDRMInstallInit(0, 0);
return 0;
}

//OK
//scePsheet_driver_3BA93CFA = ???
//unknown, just waits and signals a semaphore
int scePsheet_driver_3BA93CFA(int a0)
{
int k1 = pspSdkGetK1();
if((((a0+8) | a0) & (k1 << 11)) >= 0)
	{
	pspSdkSetK1(k1 << 11);
	int result = sub_122C(a0);
	pspSdkSetK1(k1);
	return result;
	}
pspSdkSetK1(k1);
return SCE_PSHEET_ERROR_ILLEGAL_ADDresultS;
}

//OK
//scePsheet_driver_226D9099 = sceDRMInstallAbort
//stops the encryption process.
void sceDRMInstallAbort(void)
{
int k1 = pspSdkGetK1();
pspSdkSetK1(k1 << 11);
sub_1410(1);
pspSdkSetK1(k1);
return;
}

char *sub_72C(void *a0, int a1, void *a2)
{
	int buf[4];
	void *s1;
	int s2, s3;
	void *s4 = g_unkP;
	s3 = a1;
	s2 = a0;
	s1 = a2;
	sceKernelMemset(s4, 0, 0x4D0);
	sceKernelMemset(s1, 0, sizeof(buf));
	buf = sub_15D8(s2,s3);
	if(!(buf < 0))
		{
		sceKernelMemset(buf, 0, sizeof(buf[0]));
		buf[0] = 0x01010100;
		buf[2] = 0x01050200;
		sub_1998(s2, s3, buf, 2);
		*s1[0] = (a0 + 0x448);
		*s1[1] = a1;
		sub_1B3C(s2, s3, buf[1], s1+8, 7);
		return 0;
		}
	if(!(*s4 != 0))
		{
		buf[0] = SCE_PSHEET_ERROR_P;
		}
	KDebugForKernel_84F370BC("%s: %s(): error 0x%08X\n", "bbox_api_raw.c", "get_package_info", buf[0]);
	if (buf[0] < 0)
		return 1;
	return buf;
}

//
int sub_864(int a0, int a1, int a2, void *a3)
{
	int buf[4];
	int s1, s2;
	char *s3;
	int s4;
	char *s5;
	void *s6 = g_unkP;
	s4 = a2;
	s3 = a3;
	s2 = a1;
	s1 = a0;
	sceKernelMemset(s6, 0, 0x4D0);
	sceKernelMemset(s3, 0, 0x108);
	buf[0] = sub_15D8(s1, s2);
	if(!(buf[0] < 0))
	{
		s5 = s3 + 8;
		sceKernelMemset(buf[0], 0, 8);
		buf[0] = 0x01010100;
		sub_1998(s1, s2, buf[0], 1);
		buf[0] = 0x80510109;
		if(!(s4 < buf[1]))
			goto Loc;
		buf[0] = sub_17C0(s1, s2, s4, s5, 0xFF);
		if(buf[0] < 0)
			goto Loc;
		buf[0] = strcmp(s5, &p8, "EBOOT.PBP", *(s6 + 0x40C));
		if(buf[0] != 0)
		{
			asm("lw $t5, 1032($s6)");
			asm("addiu $t2, $t5, 15");
			asm("ins $t2, $zr, 0, 4");
			asm("addiu $t4, $t2, 1023");
			asm("ins $t4, $zr, 0, 10");
			asm("srl $t3, $t4, 6");
			asm("addu $t0, $t2, $t3");
			asm("addiu $v0, $t0, 144");
			asm("sw $v0, 4($s3)");
			return 0;
		}
		asm("lw $v0, 1032($s6)");
		asm("sw $v0, 4($s3)");
		return 0;
	}
	s3 = 0;
	s2 = 0;
	s1 = 0;
	Loc:
	KDebugForKernel_84F370BC("%s: %s(): error 0x%08X\n", "bbox_api_raw.c", "get_file_info", buf[0]);
	if(buf[0] < 0)
		return 1;
	return SCE_PSHEET_ERROR_P;
}

int sub_009EC(int a0, int a1, int a2);

//OK
//Returns the status of the semaID
//sub_0122C
int getSemaIdStatus(SceUInt *sTimeout)
{
int resultult;
SceUInt *timeout = sTimeout;
resultult = sceKernelWaitSema(semaID, 1, 0);
	if(resultult < 0)
		return resultult;
buf = unk3;
resultult = sceKernelSignalSema(semaID, 1);
	if(resultult < 0)
		return resultult;
return 0;
}

//OK
//Returns the status of the semaID while also incrementing the Timeout
//sub_012A0
int getSemaIdStatusIncrement(SceUInt *sTimeout)
{
int resultult;
SceUInt *timeout = sTimeout;
resultult = sceKernelWaitSema(semaID, 1, 0);
	if(resultult < 0)
		return resultult;
semaTimeout += timeout;
resultult = sceKernelSignalSema(semaID, 1);
	if(resultult < 0)
		return resultult;
return 0;
}

//OK
//Signals a sema and returns a resultult
//sub_01310
int getSemaIdStatusUsingTimeout(int timeout, int a1)
{
int resultult, s1 = a1, s2 = timeout;
resultult = sceKernelWaitSema(semaID, 1, 0);
	if(resultult < 0)
		return resultult;
unk3 = s1;
semaTimeout = s2;
resultult = sceKernelSignalSema(semaID, 1);
	if(resultult < 0)
		return resultult;
return 0;
}

//OK
//sub_01388
int getSemaIdStatusUsingBuf(void)
{
int resultult = 0, lckdresultult, s2;
lckdresultult = sceKernelWaitSema(semaID, 1, 0);
	if(lckdresultult < 0)
	{
		Loc:
		KDebugForKernel_84F370BC("%s: %s(): error 0x%08X\n", "bbox_api_raw.c", "get_state", lckdresultult);
		return lckdresultult;
	}
lckdresultult = sceKernelSignalSema(semaID, 1);
resultult = buf[3];
	if(lckdresultult < 0)
		goto Loc;
return resultult;
}

//OK
//sub_01410
int sub_01410(int a0)
{
int resultult = 0, lckdresultult, s2 = a0;
lckdresultult = sceKernelWaitSema(semaID, 1, 0);
	if(lckdresultult < 0)
	{
		Loc:
	    KDebugForKernel_84F370BC ("%s: %s(): error 0x%08X\n", "bbox_api_raw.c", "set_state", lckdresultult);
	    return lckdresultult;
	}
resultult = buf[3];
buf[3] = s2;
lckdresultult = sceKernelSignalSema(semaID, 1);
	if(lckdresultult < 0)
		goto Loc;
return resultult;
}

//OK
//sub_014A8
int sub_014A8(char *file)
{
SceUID result;
char *s1 = file;
result = strncmp(file, "ms0:/", 5);
if(!(result != 0))
	{
	result = sceIoOpen(s1, IOASSIGN_RDONLY, 0);
	s1 = result;
		if(!(result < 0))
		{
			result = sceIoLseek32(result, 0, 2);
			if(result < 56)
				goto Loc;
			s1 = result;
			result = sub_021F0(s1, 0, result - 0x16, &something, 0);
			if(result < 0)
				{
				goto Loc;
				}
			result = sceIoLseek32(s1, 0, 0);
			return s1;
		}
		Loc:
		if(!(s1 < 0))
		{
			//sceIoClose(s1, 0, 2);
		}
		result = ((((!s1) >> 0x0000001F) | (((s1 ^ 0x80510101) < 0x00000001)))) ? SCE_PSHEET_ERROR_P2 : s1;
		KDebugForKernel_84F370BC("%s: %s(): error 0x%08X\n", "bbox_access.c", "open_and_mac_check", result);
		return result;
	}
return SCE_PSHEET_ERROR_ILLEGAL_ADDresultS;
}

//
int sub_015D8(SceUID a0, int a1)
{
int result, s1;
SceUID s2 = a0;
void *s3 = g_unkP, s4 = a1;
int temp;
result = sceIoLseek32(s2, 0, 0);
	if(result < 0)
	{
		exit:
		KDebugForKernel_84F370BC("%s: %s(): error 0x%08X\n", "bbox_access.c", "get_core_header",result);
		if (result < 0)
			s4 = 1;
		if(s4 != 0)
		{
			temp = result;
			return result;
		}
		return SCE_PSHEET_ERROR_P6;
	}
result = sceIoRead(s2, s3, 40);
	if(result < 40)
		goto exit;
s1 = *(s3+32);
	if(*(s3+36) < (s1 + 96))
		goto exit;
*(s3+1228) = *(s3+36) - s1;
*(s3+1228) = result;
*(s3+1228) = s1;
	if(sub_021F0(s2, s1, 80, s4, 0) < 0)
	{
		result = SCE_PSHEET_ERROR_P7;
		goto exit;
	}
	result = sceIoLseek32(s2, s1, 0);
	if(s1 < result)
		goto exit;
s1 = s3 + 1040;
result = sceIoRead(s2, s1, 96);
	if(result < 0)
		goto exit;

result = sub_02134(s3 + 1056, 64, 0, &something, s1);
	if(result < 0)
		goto exit;
	if(!(*(s3 + 1088) == 1))
		goto exit;
s1 = *(s3 + 1096);
result = SCE_PSHEET_ERROR_P8;
	if(*(s3 + 1092) == 0)
		return 0;
result = KDebugForKernel_47570AC5();
temp = 2;
	if(result < 0)
	{
		temp = s1;
		goto exit;
	}
result = *(s3 + 1092) & temp;
	if(result == 0)
	{
		KDebugForKernel_84F370BC("target arch: 0x%08x\n", *(s3 + 1092));
		result = SCE_PSHEET_ERROR_P8;
		goto exit;
	}
	if(*(s3+1092) != 2)
	{
		temp = 0;
		s2 = s3 = 0;
		return 0;
	}
KDebugForKernel_84F370BC("Package for PSP Development TOOL\n");
return 0;
}

int sub_017C0 (int arg1, int arg2, int arg3, int arg4, int arg5);
void sub_01998 (int arg1, int arg2, int arg3, int arg4);
void sub_01B3C (int arg1, int arg2, int arg3, int arg4, int arg5);
int sub_01CEC (int arg1, int arg2);
int sub_01E14 (char *dir, int a1);
int sub_01EF0(void *a0);

int sub_01F4C(int a0, int a1, int a2)
{
int s0 = a0, s1 = a1, s2 = 0x00002F08;
int result;
//result = sceAmctrl_driver_1CCB66D2(s2, 2, 1,, a2, 0); //Where and how is a3 set???
	if(!(result < 0))
	{
	result = sceAmctrl_driver_0785C974(s2, s0, s1);
		if(!(result < 0))
		{
		result = sceAmctrl_driver_9951C50F(s2);
			if(!(result < 0))
			{
				return 0;
			}
		goto Loc;
		}
	goto Loc;
	}
Loc:
KDebugForKernel_84F370BC("%s: %s(): error 0x%08X\n\n", "encrypt_wrapper.c", "buf_encrypt", result);
return result;
}

int sub_2000(int a0, int a1, int a2, int a3)
{
int s0 = a0, s1 = a1, *s2 = p13, s3 = a2, s4 = a3;
int result = sceAmctrl_driver_525B8218(s2, 2);//sceDrmBBMacUpdate(void *mkey, u8 *buf, int buf[1])
	if(!(result < 0))
	{
		result = sceAmctrl_driver_58163FBE(s2, s0, s1);
		if(result < 0)
		{
			goto exit;
		}
		result = sceAmctrl_driver_EF95A213(s2, s4, s3);
		if(result < 0)
		{
			goto exit;
		}
		return 0;
	}
exit:
KDebugForKernel_84F370BC("%s: %s(): error 0x%08X\n\n", "encrypt_wrapper.c", "create_mac", result);
return SCE_PSHEET_ERROR_P3;
}

//Inside of sub_2000
int sub_20C8(int a0, int a1, int a2);

int sub_2134(int a0, int a1, int a2, int a3, int t0)
{
int s0 = a0, s1 = a1;
int *s2 = cipher;
int result = sceAmctrl_driver_1CCB66D2(s2, 1, 2, t0, a3, a2);//sceDrmBBCipherInit(void *ckey, int type, int mode, u8 *header_key, u8 *version_key, u32 seed)
	if(!(result < 0))
	{
		result = sceAmctrl_driver_0785C974(s2, s0, s1);//sceDrmBBCipherUpdate(void *ckey, u8 *buf, int buf[1])
		if(result < 0)
		{
			goto exit;
		}
		result = sceAmctrl_driver_9951C50F(s2);
		if(result < 0)
		{
			goto exit;
		}
		return 0;
	}
exit:
KDebugForKernel_84F370BC("%s: %s(): error 0x%08X\n\n", "decrypt_wrapper.c", "drmbb_decrypt", result);
return SCE_PSHEET_ERROR_P3;
}

int sub_21F0(SceUID fd, int offset, int a2, int a3, void *t0);
