#include <common_imp.h>
#include <iofilemgr_kernel.h>
#include <sysmem_kernel.h >

#define SCE_PSHEET_ERROR_SEMAID 0x80510108
#define SCE_PSHEET_ERROR_ILLEGAL_ADDRESS 0x80510109
#define SCE_PSHEET_ERROR_P 0x80510111
#define SCE_PSHEET_ERROR_P2 0x80510110
#define SCE_PSHEET_ERROR_P3 0x80510102
#define SCE_PSHEET_ERROR_P4 0x80510300
#define SCE_PSHEET_ERROR_P5 0x80510101
#define SCE_PSHEET_ERROR_P6 0x80510110
#define SCE_PSHEET_ERROR_P7 0x80510113
#define SCE_PSHEET_ERROR_P8 0x80510112
#define IS_KERNEL_ADDR(addr)  (addr < 0)

SCE_MODULE_INFO("scePsheet", SCE_MODULE_KERNEL | SCE_MODULE_ATTR_EXCLUSIVE_LOAD    | SCE_MODULE_ATTR_EXCLUSIVE_START  , 1, 5);
SCE_SDK_VERSION(SDK_VERSION);

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

int sceDRMInstallInit(int address, int size);
int sceDRMInstallGetPkgInfo(int address, int size, int a2);
int sceDRMInstallGetFileInfo(char *file, int size, int a2, int a3);
int sceDRMInstallInstall(char a0[256], int a1, int a2, int *a3);
int module_start(SceSize argc, void* argp);
int module_reboot_before(SceSize args, void *argp) __attribute__((alias("module_stop")));
int sceDRMInstallEnd(void);
void sceDRMInstallAbort(void);char *sub_72C(void *a0, int a1, void *a2);
int sub_864(int a0, int a1, int a2, void *a3);
int sub_009EC(int a0, int a1, int a2);
int getSemaIdStatus(SceUInt *sTimeout);
int getSemaIdStatusIncrement(SceUInt *sTimeout);
int getSemaIdStatusUsingTimeout(int timeout, int a1);
int getSemaIdStatusUsingBuf(void);
int sub_01410(int a0);
int sub_014A8(char *file);
int sub_015D8(SceUID a0, int a1);
int sub_017C0 (int arg1, int arg2, int arg3, int arg4, int arg5);
void sub_01998 (int arg1, int arg2, int arg3, int arg4);
void sub_01B3C (int arg1, int arg2, int arg3, int arg4, int arg5);
int sub_01CEC (int arg1, int arg2);
int sub_01E14 (char *dir, int a1);
int sub_01EF0(void *a0);
int sub_01F4C(int a0, int a1, int a2);
int sub_2000(int a0, int a1, int a2, int a3);
int sub_20C8(int a0, int a1, int a2); //Inside of sub_2000
int sub_2134(int a0, int a1, int a2, int a3, int t0);
int sub_21F0(SceUID fd, int offset, int a2, int a3, void *t0);


//scePsheet_driver_302AB4B8 = sceDRMInstallInit
//sets up the initial DRM params.
//returns 0 on success, 0x80510109 on failure
int sceDRMInstallInit(int address, int size) {
	s32 oldK1;
	oldK1 = pspShiftK1();
	sceKernelMemset((void *)g_unkP, 0, 0x4D0);

	//Standard illegal address check
	if (((address | size | address + size) & oldK1) < 0)		
	{
		pspSetK1(oldK1);
		return SCE_PSHEET_ERROR_ILLEGAL_ADDRESS;
	}

	sub_01310(0, 1);
	sub_01410(0);
	buf[0] = address;
	buf[1] = size;

	if (((address < 1) | (((0x0001FFFF < size)) ^ 1)) != 0) 
	{
		buf[0] = 0;
		buf[1] = 0;
		pspSetK1(oldK1);
		return SCE_PSHEET_ERROR_ILLEGAL_ADDRESS;
	}

	pspSetK1(oldK1);
	return 0;
}

//scePsheet_driver_15355B0E = sceDRMInstallGetPkgInfo
//determines what kind of package it is, based on hash comparison.
int sceDRMInstallGetPkgInfo(int address, int size, int a2) {
	int crntAddress;
	SceUID s3;
	s32 oldK1 = pspGetK1();
	crntAddress = address;
	if(!(buf == 0))
	{
		if(address == NULL || IS_KERNEL_ADDR(address) || size <= 0x1FFFF)
			return SCE_PSHEET_ERROR_ILLEGAL_ADDRESS;
		if(a2 == 0)
			return SCE_PSHEET_ERROR_ILLEGAL_ADDRESS;
		oldK1 = pspShiftK1();
		if(!(((oldK1<<11) & address) < 0))
		{
			if(!((((size+16) | size) & (oldK1 << 11)) < 0))
			{
				if(!(((a2+16) | a2 ) & (oldK1 << 11)) >= 0)
				{
					address = SCE_PSHEET_ERROR_ILLEGAL_ADDRESS;
					size = 0;
					goto end;
				}
				sub_1410(0);
				s3 = sub_14A8(address);
				address = s3;
				if(s3 < 0)
				{
					size = 0;
					address = s3;
					goto end;
				}
				s3 = sub_72C(address, size, a2);
				address = sceIoClose(s3);
				size = 0;
				goto end;
			}
			address = SCE_PSHEET_ERROR_ILLEGAL_ADDRESS;
			size = 0;
			goto end;
		}
		address = SCE_PSHEET_ERROR_ILLEGAL_ADDRESS;
		size = 0;
		end:
		sceKernelMemset(g_unkP, 0, 0x4D0);
		pspSetK1(oldK1);
		return crntAddress;
	}
	return SCE_PSHEET_ERROR_SEMAID;
}

//scePsheet_driver_34E68A41 = sceDRMInstallGetFileInfo
//determines what kind of file it is, based on hash comparison.
int sceDRMInstallGetFileInfo(char *file, int size, int a2, int a3) {
	SceUID s0;
	s32 oldK1;
	SceUID s4;
	if(!(buf = 0))
	{
		if(file == NULL || size == 0 || a2 == NULL)
			return SCE_PSHEET_ERROR_ILLEGAL_ADDRESS;
		oldK1 = pspGetK1();
		if(!(a3 < 0))
		{
			if(!(((oldK1<<11) & file) < 0))
			{
				if(!((((size+16) | size) & size) < 0))
				{
					if((((a3+264) | a3) & (oldK1<<11)) >= 0)
					{
						pspSetK1(oldK1);
						s4 = sceIoOpen(file, SCE_O_RDONLY , 0);
						s0 = oldK1 << 11;
						pspSdkSetK1(s0);
						s0 = s4;
							if(!(s4 < 0))
							{
							sub_1410(0);
							sub_864(s4, size, a2, a3);
							sceIoClose(s4);
							goto end;
							}
						goto end;
					}
					s0 = SCE_PSHEET_ERROR_ILLEGAL_ADDRESS;
					goto end;
				}
				s0 = SCE_PSHEET_ERROR_ILLEGAL_ADDRESS;
				goto end;
			}
			oldK1 = pspShiftK1();
			s0 = SCE_PSHEET_ERROR_ILLEGAL_ADDRESS;
			end:
			size = 0;
			sceKernelMemset(g_unkP, 0, 0x4D0);
			pspSetK1(oldK1);
			return s0;
		}
		return SCE_PSHEET_ERROR_ILLEGAL_ADDRESS;
	}
	return SCE_PSHEET_ERROR_SEMAID;
}

int sceDRMInstallInstall(char a0[256], int a1, int a2, int *a3)  {
	a3 = buf[0];		//buf pointer to buf
	if(a3 == 0)
		return SCE_PSHEET_ERROR_SEMAID;
	return 0;
}

int module_start(SceSize argc, void* argp) {
	SceUID s0;
	SceUID *s1 = semaID;
	s0 = sceKernelCreateSema("ScePsheet1", 0, 1, 1, 0);
	if(!(s0 < 0))
	{
		sceDRMInstallInit(0,0);
		return 0;
	}	
	if(!(*(s1+4) <= 0))//s0 <= 0
	{
		sceKernelDeleteSema(s1+4);
	}
	sceKernelMemset(s1, 0, 64);
	sceKernelMemset(g_unkP, 0, 0x4D0);
	return s0+1;//(s0-(-1))
}

//module_reboot_before
int module_reboot_before(SceSize args, void *argp) __attribute__((alias("module_stop"))) {
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

//scePsheet_driver_3CEC4078 = sceDRMInstallEnd
//cleans up the internal param struct
int sceDRMInstallEnd(void) {
	sceDRMInstallInit(0, 0);
	return 0;
}

//scePsheet_driver_3BA93CFA = ???
//unknown, just waits and signals a semaphore
int scePsheet_driver_3BA93CFA(int a0) {
	s32 oldK1 = pspGetK1();
	if((((a0+8) | a0) & (oldK1 << 11)) >= 0)
	{
		oldK1 = pspShiftK1();
		int result = sub_122C(a0);
		pspSetK1(oldK1);
		return result;
	}
	pspSetK1(oldK1);
	return SCE_PSHEET_ERROR_ILLEGAL_ADDRESS;
}

//scePsheet_driver_226D9099 = sceDRMInstallAbort
//stops the encryption process.
void sceDRMInstallAbort(void) {
	int oldK1 = pspShiftK1();
	sub_1410(1);
	pspSetK1(oldK1);
	return;
}

char *sub_72C(void *a0, int a1, void *a2) {
	int buf[4];
	void *s1;
	void *s4 = g_unkP;
	s1 = a2;
	sceKernelMemset(s4, 0, 0x4D0);
	sceKernelMemset(s1, 0, sizeof(buf));
	buf = sub_15D8(a0,a1);
	if(!(buf < 0))
	{
		sceKernelMemset(buf, 0, sizeof(buf[0]));
		buf[0] = 0x01010100;
		buf[2] = 0x01050200;
		sub_1998(a0, a1, buf, 2);
		*s1[0] = (a0 + 0x448);
		*s1[1] = a1;
		sub_1B3C(a0, a1, buf[1], s1+8, 7);
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

int sub_864(int a0, int a1, int a2, void *a3) {
	int buf[4];
	char *s3;
	char *s5;
	void *s6 = g_unkP;
	s3 = a3;
	sceKernelMemset(s6, 0, 0x4D0);
	sceKernelMemset(s3, 0, 0x108);
	buf[0] = sub_15D8(a0, a1);
	if(!(buf[0] < 0))
	{
		s5 = s3 + 8;
		sceKernelMemset(buf[0], 0, 8);
		buf[0] = 0x01010100;
		sub_1998(a0, a1, buf[0], 1);
		buf[0] = 0x80510109;
		if(!(a2 < buf[1]))
			goto Loc;
		buf[0] = sub_17C0(a0, a1, a2, s5, 0xFF);
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
	a1 = 0;
	a0 = 0;
	Loc:
	KDebugForKernel_84F370BC("%s: %s(): error 0x%08X\n", "bbox_api_raw.c", "get_file_info", buf[0]);
	if(buf[0] < 0)
		return 1;
	return SCE_PSHEET_ERROR_P;
}

//Returns the status of the semaID
//sub_0122C
int getSemaIdStatus(SceUInt *sTimeout) {
	int resultult;
	resultult = sceKernelWaitSema(semaID, 1, 0);
	if(resultult < 0)
		return resultult;
	buf = unk3;
	resultult = sceKernelSignalSema(semaID, 1);
	if(resultult < 0)
		return resultult;
	return 0;
}

//Returns the status of the semaID while also incrementing the Timeout
//sub_012A0
int getSemaIdStatusIncrement(SceUInt *sTimeout) {
	int resultult;
	resultult = sceKernelWaitSema(semaID, 1, 0);
	if(resultult < 0)
		return resultult;
	semaTimeout += sTimeout;
	resultult = sceKernelSignalSema(semaID, 1);
	if(resultult < 0)
		return resultult;
	return 0;
}

//Signals a sema and returns a resultult
//sub_01310
int getSemaIdStatusUsingTimeout(int timeout, int a1) {
	int resultult;
	resultult = sceKernelWaitSema(semaID, 1, 0);
	if(resultult < 0)
		return resultult;
	unk3 = a1;
	semaTimeout = timeout;
	resultult = sceKernelSignalSema(semaID, 1);
	if(resultult < 0)
		return resultult;
	return 0;
}

//sub_01388
int getSemaIdStatusUsingBuf(void) {
	int resultult = 0, lckdresultult;
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

//sub_01410
int sub_01410(int a0) {
	int resultult = 0, lckdresultult;
	lckdresultult = sceKernelWaitSema(semaID, 1, 0);
	if(lckdresultult < 0)
	{
		Loc:
	    KDebugForKernel_84F370BC ("%s: %s(): error 0x%08X\n", "bbox_api_raw.c", "set_state", lckdresultult);
	    return lckdresultult;
	}
	resultult = buf[3];
	buf[3] = a0;
	lckdresultult = sceKernelSignalSema(semaID, 1);
	if(lckdresultult < 0)
		goto Loc;
	return resultult;
}

//sub_014A8
int sub_014A8(char *file) {
	SceUID result;
	result = strncmp(file, "ms0:/", 5);
	if(!(result != 0))
	{
		result = sceIoOpen(file, SCE_O_RDONLY , 0);
		file = result;
		if(!(result < 0))
		{
			result = sceIoLseek32(result, 0, 2);
			if(result < 56)
				goto Loc;
			file = result;
			result = sub_021F0(file, 0, result - 0x16, &something, 0);
			if(result < 0)
			{
				goto Loc;
			}
			result = sceIoLseek32(file, 0, 0);
			return file;
		}
		Loc:
		if(!(file < 0))
		{
			//sceIoClose(file, 0, 2);
		}
		result = ((((!file) >> 0x0000001F) | (((file ^ 0x80510101) < 0x00000001)))) ? SCE_PSHEET_ERROR_P2 : file;
		KDebugForKernel_84F370BC("%s: %s(): error 0x%08X\n", "bbox_access.c", "open_and_mac_check", result);
		return result;
	}
	return SCE_PSHEET_ERROR_ILLEGAL_ADDRESS;
}

//
//sub_014A8
int sub_015D8(SceUID a0, int a1) {
	int result, s1;
	void *s3 = g_unkP;
	int temp;
	result = sceIoLseek32(a0, 0, 0);
	if(result < 0)
	{
		exit:
		KDebugForKernel_84F370BC("%s: %s(): error 0x%08X\n", "bbox_access.c", "get_core_header",result);
		if (result < 0)
			a1 = 1;
		if(a1 != 0)
		{
			temp = result;
			return result;
		}
		return SCE_PSHEET_ERROR_P6;
	}

	result = sceIoRead(a0, s3, 40);
	if(result < 40)
		goto exit;
	s1 = *(s3+32);
	if(*(s3+36) < (s1 + 96))
		goto exit;
	*(s3+1228) = *(s3+36) - s1;
	*(s3+1228) = result;
	*(s3+1228) = s1;
	if(sub_021F0(a0, s1, 80, a1, 0) < 0)
	{
		result = SCE_PSHEET_ERROR_P7;
		goto exit;
	}
	result = sceIoLseek32(a0, s1, 0);
	if(s1 < result)
		goto exit;
	s1 = s3 + 1040;
	result = sceIoRead(a0, s1, 96);
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
		a0 = s3 = 0;
		return 0;
	}
	KDebugForKernel_84F370BC("Package for PSP Development TOOL\n");
	return 0;
}

int sub_01F4C(int a0, int a1, int a2) {
	int s2 = 0x00002F08;
	int result;
	//result = sceAmctrl_driver_1CCB66D2(s2, 2, 1,, a2, 0); //Where and how is a3 set???
	if(!(result < 0))
	{
		result = sceAmctrl_driver_0785C974(s2, a0, a1);
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

int sub_2000(int a0, int a1, int a2, int a3) {
	int *s2 = p13;
	int result = sceAmctrl_driver_525B8218(s2, 2);//sceDrmBBMacUpdate(void *mkey, u8 *buf, int buf[1])
	if(!(result < 0))
	{
		result = sceAmctrl_driver_58163FBE(s2, a0, a1);
		if(result < 0)
		{
			goto exit;
		}
		result = sceAmctrl_driver_EF95A213(s2, a3, a2);
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

int sub_2134(int a0, int a1, int a2, int a3, int t0) {
	int *s2 = cipher;
	int result = sceAmctrl_driver_1CCB66D2(s2, 1, 2, t0, a3, a2);//sceDrmBBCipherInit(void *ckey, int type, int mode, u8 *header_key, u8 *version_key, u32 seed)
	if(!(result < 0))
	{
		result = sceAmctrl_driver_0785C974(s2, a0, a1);//sceDrmBBCipherUpdate(void *ckey, u8 *buf, int buf[1])
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
