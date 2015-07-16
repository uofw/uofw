/* Copyright (C) 2011, 2012, 2013, 2014 The uOFW team
 See the file COPYING for copying permission.
 */

#include <common_imp.h>
#include <iofilemgr_kernel.h>
#include <threadman_kernel.h>
#include <sysmem_sysclib.h>

#define SCE_ERROR_PRIV_REQUIRED 0x80000023
#define SCE_NULL_POINTER 0x80550980
#define SCE_ERROR_READ_WRITE 0x80550981
#define SCE_ERROR_IO 0x80550982
#define SCE_ERROR_UNKNOWN_0 0x80550983
#define SCE_ERROR_UNKNOWN_1 0x80550984
#define SCE_ERROR_UNKNOWN_2 0x8055098

#define TIME_OUT 0x004C4B40

SCE_MODULE_INFO("sceNpInstall_Driver", SCE_MODULE_KIRK_SEMAPHORE_LIB | SCE_MODULE_KERNEL | SCE_MODULE_ATTR_EXCLUSIVE_START | SCE_MODULE_ATTR_EXCLUSIVE_LOAD, 1, 1);

/*
 * Structure for the act.dat file located in flash2
 * 
 * Act.dat FORMAT -> 
 * 	Version 	(0x0 - 0x4)
 * 	License Type(0x4 - 0x8)
 * 	Account ID	(0x8 - 0x10)
 * 	Data table	(0x10 - 0x1010)
 * 	Signature	(0x1010 - 0x1030)
 * 	Unknown 	(0x1030 - 0x1038)
 */
typedef struct ActDat {
	int version;
	int licenseType;
	int accountID[2];
} ActDat;

/*
 * Structure for a PSID
 */
typedef struct PspOpenPSID {
	unsigned char data[16];
} PspOpenPSID;

/*
 * .data / .bss section
 */
ActDat g_act = {
	.version = 0,
	.licenseType = 0,
	.accountID = {
		[0] = 0,
		[1] = 0,
	}
};

int g_0x9D0[2];
int g_fd;

/*
 * Headers
 */
extern int pspSdkGetK1();
extern int pspSdkSetK1(int);

extern int sceKernelTerminateThread(SceUID thid);

extern int SysclibForKernel_7DEE14DE(int, int, int, int); // __udivdi3?

extern int sceRtc_driver_CEEF238F(void *data);
extern int sceRtc_driver_89FA4262(void *, void *, int, int);

extern int sceOpenPSIDGetPSID(PspOpenPSID, int);

extern int scePspNpDrm_driver_EBB198ED(void *, void *);
extern int sceNpDrmVerifyAct(void *data);

extern int scePcactAuth1BB(int, PspOpenPSID, void *, void *, int, int);
extern int scePcactAuth2BB(int, int, void *);

s32 sceNpInstall_driver_5847D8C7(int a0, int a1, int a2, int a3)
		__attribute__((alias("sceNpInstall_user_5847D8C7")));

s32 sceNpInstall_driver_0B039B36(int a0, int a1, int a2)
		__attribute__((alias("sceNpInstall_user_0B039B36")));

s32 sceNpInstall_driver_91F9D50D(int a0)
		__attribute__((alias("sceNpInstall_user_91F9D50D")));

void sceNpInstall_driver_7AE4C8BC(void)
		__attribute__((alias("sceNpInstall_user_7AE4C8BC")));

s32 sub_0016C(int a0, int size, int a2, int a3);
s32 installActivationDat(int addr, int crypt, int addr2);
s32 storeAccountID(int addr);
u32 removeAct(void);
s32 registerNpDeactivationThread(void);

/*
 *  Address 0x00000000
 */
u32 module_start(SceSize args __attribute__((unused)),
		void *argp __attribute__((unused))) {
	return 0;
}

/*
 * Address 0x00000008
 */
u32 module_stop(SceSize args __attribute__((unused)),
		void *argp __attribute__((unused))) {
	return 0;
}

/*
 * sceNpInstallGetChallenge
 * Address 0x00000010
 * 
 * Verfies the data being passed
 */
s32 sceNpInstall_user_5847D8C7(int a0, int a1, int a2, int a3) {
	s32 res = SCE_ERROR_PRIV_REQUIRED;
	s32 oldk1 = pspShiftK1();
	//0x44
	if ((((a1 + 32) | a1) & (oldk1 << 11)) >= 0) {
		//0x4C
		if ((((a2 + 128) | a2) & (oldk1 << 11)) >= 0) {
			//0x5C
			if ((((a3 + 64) | a3) & (oldk1 << 11)) >= 0) {
				res = sub_0016C(a0, a1, a2, a3);
			}
			//0x70
		}
		//0x70
	}
	//0x70
	pspSetK1(oldk1);
	return res;
}

/*
 * sceNpInstallActivation
 * Address 0x00000088
 * 
 * Begins installation of the act.dat file.
 * 
 * @param addr		- the address to write act.dat information to
 * @param crypt 	- value to crypt it with?
 * @param addr2 	- Used for encryption process?
 */
s32 sceNpInstall_user_0B039B36(int addr, int crypt, int addr2) {
	s32 res = SCE_ERROR_PRIV_REQUIRED;
	s32 oldk1 = pspShiftK1();
	//0xC0
	if (((oldk1 << 11) & (((addr + crypt) | addr) | crypt)) >= 0) {
		//0xC8
		if ((((addr2 + 64) | addr2) & (oldk1 << 11)) >= 0) {
			res = installActivationDat(addr, crypt, addr2);
		}
		//0xDC
	}
	//0xDC
	pspSetK1(oldk1);
	return res;
}

/*
 * sceNpInstallCheckActivation
 * Address 0x000000F4
 * 
 * Stores the Account ID at the specified address
 * 
 * @param addr - the address to pass to store the Account ID to 
 */
s32 sceNpInstall_user_91F9D50D(int addr) {
	s32 res = SCE_ERROR_PRIV_REQUIRED;
	s32 oldk1 = pspShiftK1();
	//0x11C
	if ((((addr + 8) | addr) & (oldk1 << 11)) >= 0) {
		res = storeAccountID(addr);
	}
	//0x12C
	pspSetK1(oldk1);
	return res;
}

/*
 * sceNpInstallDeactivation
 * Address 0x00000140
 * 
 * Creates and runs the thread for act.dat deletion
 */
void sceNpInstall_user_7AE4C8BC(void) {
	s32 oldk1 = pspShiftK1();
	registerNpDeactivationThread();
	pspSetK1(oldk1);
	return;
}

/*
 * sub_0016C
 * Address 0x0000016C
 */
s32 sub_0016C(int a0, int size, int a2, int a3) {
	PspOpenPSID psid; 	// 0  - 16
	u8 data[64];		// 16 - 80
	s32 retStatus = SCE_NULL_POINTER;
	int s2 = 2;
	int fd = -1;
	s32 res = SCE_NULL_POINTER;

	//0x1B0
	if (((size < 1) | (a3 < 1)) != 0)
		return res; //SCE_ERROR_NULL_POINTER

	//0x1B8
	if (a2 == 0)
		return res; //SCE_ERROR_NULL_POINTER

	res = sceRtc_driver_CEEF238F(&data[48]);

	//0x1C8
	if (res < 0)
		return res;

	//0x1D4
	if (a0 == 1) {
		//0x238
		res = sceIoOpen("flash2:/act.dat", 0x04000001, 0);
		retStatus = SCE_ERROR_UNKNOWN_0;
		s2 = 1;
		fd = res;
		goto mem;
	}

	//0x1DC
	if (a0 == 0) {
		//0x1F8
		s2 = 0;
		//0x208
		if ((*((int *) data + 0xD) < g_0x9D0[1]) != 0)
			return SCE_ERROR_UNKNOWN_1;
		//0x210
		if (g_0x9D0[1] == *((int *) data + 0xD)) {
			//0x224
			if ((*((int *) data + 0xC) < g_0x9D0[0]) != 0)
				return SCE_ERROR_UNKNOWN_1;
			//0x27C
			goto mem;
		}
		//0x278
		goto mem;
	}
	//0x1E8
	if (a0 != 2)
		return SCE_NULL_POINTER;

	//0x274
	if (res < 0) {
		//0x27C
		mem: memset(&data[0], 0, 48); //memset
		data[0] = 16;
		data[1] = 0;
		memcpy(&data[16], (u8 *) size, 32);	//memcpy - Since value of size is not altered throughout the function, we don't need a 'tempSize' variable in the 2nd arg.
		u32 temp = 0xFF2BC000;
		res = SysclibForKernel_7DEE14DE(*((u32 *) data + 0xC) - temp,
				(*((u32 *) data + 0xD) - temp) - (*((u32 *) data + 0xC) < temp),
				0x3E8, 0);
		*((int *) data + 0xE) = res;
		*((int *) data + 0xF) = 0x00DCBFFE;
		res = sceOpenPSIDGetPSID(psid, 1);
		//0x2EC
		if (res < 0)
			return res;

		retStatus = scePcactAuth1BB(s2, psid, &data[0], &data[56], a2, a3);
		//0x314
		if (a0 == 1)
			retStatus = sceRtc_driver_89FA4262(&g_0x9D0[0], &data[48], 30, 0);
	}
	//0x334
	if (fd >= 0)
		sceIoClose(fd);
	return retStatus;
}

/*
 * Attempts to generate and install act.dat into flash2
 * 
 * sub_00374
 * Address 0x00000374
 * 
 * @param addr		- the address to write act.dat information to
 * @param crypt 	- type of encryption to use?
 * @param addr2 	- Used for encryption process?
 */
s32 installActivationDat(int addr, int crypt, int addr2) {
	SceUID fd;
	SceUID tempFd;
	int *gActAddr = (int *) &g_act;
	int *data = (int *) (addr + 80);
	int *addr2Ptr = (int *) addr2;

	//0x3A4
	if (((addr < 1) || (crypt ^= 0x1090)) == 0)
		//0x3B4
		return SCE_NULL_POINTER;

	//0x3AC
	if (addr2 == 0)
		return SCE_NULL_POINTER;

	//0x3C0
	fd = scePcactAuth2BB(addr, addr2, gActAddr);
	//0x3D0
	if (fd >= 0) {
		fd = scePspNpDrm_driver_EBB198ED(data, gActAddr);
		//0x3E4
		if (fd >= 0) {
			fd = sceNpDrmVerifyAct(data);
			//0x3F4
			if (fd >= 0) {
				tempFd = sceIoOpen("flash2:/act.dat", 0x04000602, 0x1B6);
				//0x418
				if (tempFd >= 0) {
					/*
					 * Writes a newly generated act.dat into flash2
					 */
					fd = sceIoWrite(tempFd, data, 0x1038);
					//0x438
					if (fd != 4152) {
						//0x448
						if ((fd < 0) != 0)
							fd = SCE_ERROR_READ_WRITE;
					}
					//0x44C
				}
				//0x45C
				sceIoClose(tempFd);
				goto end;
			}
			//0x464
		}
		//0x464
	}
	/*
	 * 0x464
	 *
	 * Remove act.dat and clear memory locations
	 */
	sceIoRemove("flash2:/act.dat");
	end: memset(data, 0, 0x1040); //memset
	memset(addr2Ptr, 0, 64);	//memset
	memset((int *) &g_act, 0, 16); //memset
	return fd;
}

/* 
 * sub_004C4
 * Address 0x000004C4
 *
 * Gets the Account ID from act.dat in flash2 and stores it
 * into the desired address.
 *
 * @param addr - the address to store the Account ID to
 */
s32 storeAccountID(int addr) {
	ActDat act;
	s32 bytesRead = SCE_NULL_POINTER;
	SceUID fd;
	int *Ptr = (int *) addr;

	//0x4E0
	if (addr == 0)
		return bytesRead;	//SCE_ERROR_NULL_POINTER

	fd = sceIoOpen("flash2:/act.dat", 0x04000001, 0);

	//0x508
	if (fd >= 0) {
		bytesRead = sceIoRead(fd, &act, 16);
		sceIoClose(fd);
		//0x528
		if (bytesRead == 16) {
			//0x544
			Ptr[0] = act.accountID[0];
			Ptr[1] = act.accountID[1];
			return bytesRead;
		}
		//0x530
		if (bytesRead < 0) {
			//0x554
			return bytesRead;
		}
		//0x53C
		return SCE_ERROR_READ_WRITE;
	}
	return SCE_ERROR_IO;
}

/*
 * sub_00580
 * Address 0x00000580
 * 
 * Removes act.dat from flash2 and sets appropriate global flag
 */
u32 removeAct(void) {
	int fd = sceIoRemove("flash2:/act.dat");
	g_fd = fd;
	return 0;
}

/*
 * sub_005AC
 * Address 0x000005AC
 * 
 * Creates and starts a seperate thread for deleting act.dat from flash2
 */
s32 registerNpDeactivationThread(void) {
	int threadStatus;
	SceUID thid;

	thid = sceKernelCreateThread("SceNpDeactivation",
			(SceKernelThreadEntry) removeAct, 0x20, 0x4000, 0, NULL);
	//0x600
	if (thid >= 0) {
		g_fd = 0;
		threadStatus = sceKernelStartThread(thid, 0, NULL);
		//0x618
		if (threadStatus >= 0) {
			threadStatus = sceKernelWaitThreadEnd(thid, (SceUInt *) TIME_OUT);
			//0x628
			if (threadStatus >= 0) {
				return g_fd;
			}
			//0x638
		}
		//0x638
		sceKernelTerminateThread(thid);
		sceKernelDeleteThread(thid);
		return threadStatus;
	}
	//0x64C
	return thid;
}
