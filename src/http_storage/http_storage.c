/* Copyright (C) 2011, 2012, 2013, 2014, 2015 The uOFW team
 See the file COPYING for copying permission.
 */

#include <common_imp.h>
#include <sysmem_kernel.h>
#include <sysmem_sysclib.h>
#include <iofilemgr_kernel.h>
#include <modulemgr_kernel.h>

#define PSP_O_RDONLY 0x0001
#define PSP_O_WRONLY 0x0002
#define PSP_O_RDWR (PSP_O_RDONLY | PSP_O_WRONLY)
#define PSP_O_NBLOCK 0x0004
#define PSP_O_DIROPEN 0x0008
#define PSP_O_APPEND 0x0100
#define PSP_O_CREAT 0x0200
#define PSP_O_TRUNC 0x0400
#define PSP_O_EXCL 0x0800
#define PSP_O_NOWAIT 0x8000
#define PSP_SEEK_SET 0
#define PSP_SEEK_CUR 1
#define PSP_SEEK_END 2

#define SEEK_END        2       /* Set file pointer to EOF plus "offset" */

SCE_MODULE_INFO("sceHttpStorage_Service", SCE_MODULE_KERNEL | SCE_MODULE_ATTR_EXCLUSIVE_START | SCE_MODULE_ATTR_EXCLUSIVE_LOAD, 1, 3);
SCE_MODULE_STOP("sceHttpStorage_driver_C59AC98A");
SCE_SDK_VERSION( SDK_VERSION);

extern int sceKernelUnloadModule(int);
extern int sceKernelStartModule(int, int, int, void *data, int);
extern int sceKernelLoadModule(const char *prx);
extern int sceKernelSearchModuleByName(const char *module);
extern int sceKernelStopModule(int, int, int, int, int); //Alias sceHttpStorageEnd

extern int sceChkreg_driver_59F8491D(char *buf); //sceChkregGetPsCode

u32 sceHttpStorage_driver_24AA94F4(int a0, int a1, int a2) __attribute__((alias("sceHttpStorageWrite")));
u32 sceHttpStorage_driver_2D8DAE58(int a0, int a1) __attribute__((alias("sceHttpStorageGetstat")));
u32 sceHttpStorage_driver_700AAD44(int a0, int a1, int a2) __attribute__((alias("sceHttpStorageOpen")));
u32 sceHttpStorage_driver_B33389CE(int a0) __attribute__((alias("sceHttpStorageLseek")));
u32 sceHttpStorage_driver_CDA3D8F6(int a0) __attribute__((alias("sceHttpStorageClose")));
u32 sceHttpStorage_driver_CDDF1103(int a0, int a1, int a2) __attribute__((alias("sceHttpStorageRead")));
int sceHttpStorage_driver_C59AC98A(void) __attribute__((alias("sceHttpStorageEnd")));

// .data - 0x9C0
SceUID g_fd[2];

/*
Subroutine sceHttpStorageOpen - Address 0x00000000 
Exported in sceHttpStorage_driver
Exported in sceHttpStorage
*/
u32 sceHttpStorageOpen(int a0, int a1, int a2) {
	s32 oldk1 = pspShiftK1();
	int s1 = a0 << 2;
	SceUID fd;
	//0x24
	if ((a0 < 2) != 0) {
		//0x48
		if (g_fd[0] != -1) {
			pspSetK1(oldk1);
			return 0x80000020;
		}
		a2 &= 0x1A4;
		//0x88
		if (a0 != 0) {
			//0x94
			if (a0 != 1) {
				fd = 0x8000010;
				goto end;
			}
			fd = sceIoOpen("flash1:/net/http/cookie.dat",
					((a1 & 0x02000603) | 0x04000000), (a2 & 0x000001A4));
			goto end;
		}
		fd = sceIoOpen("flash1:/net/http/auth.dat",
				((a1 & 0x02000603) | 0x04000000), (a2 & 0x000001A4));
		end:
		//0xA0
		if (fd > 0) {
			*((int *) (s1 + &g_fd[0])) = fd;
			pspSetK1(oldk1);
			return 0;
		}
		pspSetK1(oldk1);
		return fd;
	}
	//0x50
	pspSetK1(oldk1);
	return 0x80000100;
}

/*
Subroutine sceHttpStorageClose - Address 0x000000E0 
Exported in sceHttpStorage_driver
Exported in sceHttpStorage
*/
u32 sceHttpStorageClose(int a0) {
	int s0 = 0;
	s32 oldk1 = pspShiftK1();
	SceUID *fd = (a0 << 2) + &g_fd[0];
	//0x110
	if ((a0 < 2) != 0) {
		//0x11C
		if (*((int *) fd) >= 0) {
			s0 = sceIoClose(*((SceUID *) fd));
			*((SceUID *) fd) = -1;
			pspSetK1(oldk1);
			return s0;
		}
		pspSetK1(oldk1);
		return s0;
	}
	pspSetK1(oldk1);
	return 0x80000100;
}

/*
Subroutine sceHttpStorageRead - Address 0x00000160 
Exported in sceHttpStorage_driver
Exported in sceHttpStorage
*/
u32 sceHttpStorageRead(SceUID a0, char a1, SceSize a2) {
	s32 oldk1 = pspShiftK1();
	//0x190
	if ((int) ((oldk1 << 11) & (((a1 + a2) | a1) | a2)) >= 0) {
		//0x1AC
		if ((a0 < 2) != 0) {
			//0x1C4
			if (*(int *) (((a0 << 2) + &g_fd[0])) != -1) {
				int bytesRead = sceIoRead(*((SceUID *) ((a0 << 2) + &g_fd[0])),
						&a1, a2);
				pspSetK1(oldk1);
				return bytesRead;
			}
			pspSetK1(oldk1);
			return 0x80000001;
		}
		pspSetK1(oldk1);
		return 0x80000100;
	}
	pspSetK1(oldk1);
	return 0x800001FF;
}

/*
Subroutine sceHttpStorageWrite - Address 0x000001F0 
Exported in sceHttpStorage_driver
Exported in sceHttpStorage
*/
u32 sceHttpStorageWrite(SceUID a0, const char a1, SceSize a2) {
	s32 oldk1 = pspShiftK1();
	//0x220
	if ((int) ((oldk1 << 11) & (((a1 + a2) | a1) | a2)) >= 0) {
		//0x23C
		if (((a0 << 2) + &g_fd) != 0) {
			//0x254
			if (*(int *) (((a0 << 2) + &g_fd[0])) != -1) {
				int bytesWritten = sceIoWrite(
						*((SceUID *) ((a0 << 2) + &g_fd[0])), &a1, a2);
				pspSetK1(oldk1);
				return bytesWritten;
			}
			pspSetK1(oldk1);
			return 0x80000001;
		}
		pspSetK1(oldk1);
		return 0x80000100;
	}
	pspSetK1(oldk1);
	return 0x800001FF;
}

/*
Subroutine sceHttpStorageLseek - Address 0x00000280 
Exported in sceHttpStorage_driver
Exported in sceHttpStorage
*/
u32 sceHttpStorageLseek(int a0) {
	s32 oldk1 = pspShiftK1();
	//0x2AC
	if ((a0 < 2) != 0) {
		//0x2C4
		if (*((int *) ((a0 << 2) + &g_fd[0])) != 0) {
			SceOff pos = sceIoLseek(*((int *) ((a0 << 2) + &g_fd[0])), 0,
			SEEK_END);
			pspSetK1(oldk1);
			return pos;
		}
		pspSetK1(oldk1);
		return 0x80000001;
	}
	pspSetK1(oldk1);
	return 0x80000100;
}

/*
Subroutine sceHttpStorageGetstat - Address 0x000002F0 
Exported in sceHttpStorage_driver
Exported in sceHttpStorage
*/
u32 sceHttpStorageGetstat(int a0, int a1) {
	s32 oldk1 = pspShiftK1();
	//0x310
	if (((oldk1 << 11) & a1) >= 0) {
		//0x318
		if (a0 != 0) {
			//0x320
			if (a0 != ((oldk1 << 11) & a1)) {
				pspSetK1(oldk1);
				return 0x80000100;
			}
			int stat = sceIoGetstat("flash1:/net/http/cookie.dat", (SceIoStat *) a1);
			pspSetK1(oldk1);
			return stat;
		}
		int stat = sceIoGetstat("flash1:/net/http/auth.dat", (SceIoStat *) a1);
		pspSetK1(oldk1);
		return stat;
	}
	pspSetK1(oldk1);
	return 0x800001FF;
}

/*
Subroutine module_stop - Address 0x00000368 - Aliases: sceHttpStorageEnd
Exported in syslib
Exported in sceHttpStorage_driver
*/
int sceHttpStorageEnd(void) {
	SceUID *s0 = &g_fd[0];
	int i = 0;
	for (i = 0; i < 2; i++) {
		//0x394
		if (*((int *) s0) != -1) {
			sceIoClose(*((SceUID *) s0));
			*((int *) s0) = -1;
		}
		s0 += 4;
	}
	return 0;
}

/*
Subroutine module_reboot_before - Address 0x000003D0  
Exported in syslib
This does the exactly the same as module_stop
*/
int module_reboot_before(void) {
	SceUID *s0 = &g_fd[0];
	int i = 0;
	for (i = 0; i < 2; i++) {
		//0x3FC
		if (*((int *) s0) != -1) {
			sceIoClose(*((SceUID *) s0));
			*((int *) s0) = -1;
		}
		s0 += 4;
	}
	return 0;
}

/*
Subroutine sceHttpStorage_bridge_04EF00F8 - Address 0x00000438 
Exported in sceHttpStorage_bridge
*/
u32 sceHttpStorage_bridge_04EF00F8(int a0) {
	u8 buf[16];
	int res = 0x80000023;
	s32 oldk1 = pspShiftK1();
	//0x47C
	if ((((a0 + 8) | a0) & (oldk1 << 11)) >= 0) {
		res = sceKernelSearchModuleByName("sceChkreg");//scekernelfindmodulebyname?
		//0x4A4
		if ((u32) res != 0x8002012E) { //SCE_ERROR_KERNEL_UNKNOWN_MODULE
			//0x4AC
			if (res >= 0) {
				res = sceChkreg_driver_59F8491D((char *) a0);
				goto end;
			}
			pspSetK1(oldk1);
			return res;
		}
		res = sceKernelLoadModule("flash0:/kd/chkreg.prx");
		//0x52C
		if (res == 0) {
			res = sceKernelStartModule(res, 0, 0, buf, 0);
			//0x540
			if (res < 0) {
				goto end;
			}
			res = sceChkreg_driver_59F8491D((char *) a0);
			end:
			//0x4D0
			if (res >= 0) {
				sceKernelStopModule(res, 0, 0, 0, 0);
				res = sceKernelUnloadModule(res);
				pspSetK1(oldk1);
				return (res < 0) ? res : sceKernelUnloadModule(res);
			}
			pspSetK1(oldk1);
			return res;
		}
		pspSetK1(oldk1);
		return res;
	}
	pspSetK1(oldk1);
	return res;
}

/*
Subroutine sceHttpStorage_bridge_EFE06A20 - Address 0x00000550 
Exported in sceHttpStorage_bridge
*/
u32 sceHttpStorage_bridge_EFE06A20(int a0, int a1, int a2) {
	char *s0 = (char *) a1;
	s32 oldk1 = pspShiftK1();
	//0x57C
	if (a1 != 0) {
		//0x594
		if (((oldk1 << 11) & (((a1 + a2) | a1) | a2)) < 0) {
			pspSetK1(oldk1);
			return 0x80000103;
		}
		SceKernelGameInfo *res = sceKernelGetGameInfo();
		//0x5D0
		if (a0 != 2) {
			//0x5D8
			if ((a0 < 3) != 0) {
				//0x5E4
				if (a0 != 1) {
					pspSetK1(oldk1);
					return 0x800001FF;
				}
				//0x5FC
				if (a0 == 0) {
					//0x60C
					if (*((int *) res + 180) == 0) {
						strncpy((char *)s0, (char *)res + 68, 13);
						pspSetK1(oldk1);
						return 0;
					}
					strncpy((char *)s0, (char *)res + 180, 10);
					pspSetK1(oldk1);
					return 0;
				}
				pspSetK1(oldk1);
				return 0x80000104;
			}
			//0x644
			if (a0 != 3) {
				pspSetK1(oldk1);
				return 0x800001FF;
			}
			//0x650
			if ((a2 < 7) == 0) {
				strncpy((char *)s0, (char *)res + 88, 7);
				pspSetK1(oldk1);
				return 0;
			}
			pspSetK1(oldk1);
			return 0x80000104;
		}
		//0x664
		if ((a2 < 7) != 0) {
			pspSetK1(oldk1);
			return 0x80000104;
		}
		strncpy((char *)s0, (char *)res + 196, 7);
		pspSetK1(oldk1);
		return 0;
	}
	pspSetK1(oldk1);
	return 0x80000103;
}
