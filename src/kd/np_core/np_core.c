#include <common_imp.h>
#include <sysmem_sysclib.h>

SCE_MODULE_INFO("sceNpCore", SCE_MODULE_KERNEL | SCE_MODULE_ATTR_EXCLUSIVE_START | SCE_MODULE_ATTR_EXCLUSIVE_LOAD, 1, 1);

/*
 * Structure for a PSID
 */
typedef struct PspOpenPSID {
	unsigned char data[16];
} PspOpenPSID;

/*
 * Headers
 */

extern int SysMemForKernel_7FF2F35A(int);
extern int sceOpenPSIDGetPSID(PspOpenPSID, int);

u32 sub_0000045C(int, int, u32);
void sub_0000055C();
u32 sub_00000590(int, int);
u32 sub_000005D8(int *, char *, int);
u32 sub_00000658(char *, char *, int, int *, int);
u32 sub_00000728(char *, int, char *, char *);
u32 sub_000007B4(void *, int);
u32 sub_000007F8(void *, int);
u32 sub_00000840(void *, int);
u32 sub_000008F8(int, char *, int);
u32 sub_0000098C(char *, int);


/*
 * .data section
 */
int g_0xE30;
int g_0xE34;
int g_0xE38;

/*
 * Subroutine sceNpCore_57E15796 - Address 0x00000000 
 * Exported in sceNpCore
 */
void sceNpCore_57E15796(void) {
	s32 oldk1 = pspShiftK1();
	sub_0000055C();
	pspSetK1(oldk1);
}

/*
 * Subroutine sceNpCore_52440ABF - Address 0x0000002C 
 * Exported in sceNpCore
 */
u32 sceNpCore_52440ABF(int address) {
	s32 oldk1 = pspShiftK1();
	//0x4C
	if (address != 0) {
		//0x54
		if (((oldk1 << 11) & address) >= 0) {
			u32 res = SysMemForKernel_7FF2F35A(address); //shift again(overflows) but shifts back
			pspSetK1(oldk1);
			return res;
		}
		//0x5C
	}
	//0x5C
	pspSetK1(oldk1);
	return 0x80550003;
}

/*
 * Subroutine sceNpCore_243690EE - Address 0x00000084 
 * Exported in sceNpCore
 */
u32 sceNpCore_243690EE(int address, int size) {
	s32 oldk1 = pspShiftK1();
	//0xB0
	if (address != 0) {
		//0xB8
		if (((oldk1 << 11) & (((address + size) | address) | size)) >= 0) {
			u32 res = sub_00000590(address, size);
			pspSetK1(oldk1);
			return res;
		}
		//0xC0
	}
	//0xC0
	pspSetK1(oldk1);
	return 0x80550003;
}

/*
 * Subroutine sceNpCore_8AFAB4A0 - Address 0x000000E8 
 * Exported in sceNpCore
 */
u32 sceNpCore_8AFAB4A0(int arg0, int address, int size) {
	s32 oldk1 = pspShiftK1();
	//0x11C
	if (((arg0 < 1) | (size < 1)) == 0) {
		//0x124
		if ((((arg0 + 12) | arg0) & (arg0 < 1)) >= 0) {
			//0x138
			if (((oldk1 << 11) & (((address + size) | address) | size)) >= 0) {
				u32 res = sub_000005D8((int *) arg0, (char *) address, size);
				pspSetK1(oldk1);
				return res;
			}
			//0x140
		}
		//0x140
	}
	//0x140
	pspSetK1(oldk1);
	return 0x80550003;
}

/*
 * Subroutine sceNpCore_5145344F - Address 0x00000168 
 * Exported in sceNpCore
 */
u32 sceNpCore_5145344F(char *arg0, int address, int size, int arg3, int t0) {
	s32 oldk1 = pspShiftK1();
	//0x194
	if (address != 0) {
		//0x19C
		if (((oldk1 << 11) & (((address + size) | address) | size)) >= 0) {
			//0x1A4
			if (arg3 != 0) {
				//0x1B8
				if (((oldk1 << 11) & (((arg3 | t0) | arg3) | t0)) >= 0) {
					u32 res = sub_00000658(arg0, (char *) address, size, (int *) arg3,
							t0);
					pspSetK1(oldk1);
					return res;
				}
				//0x1C0
			}
			//0x1C0
		}
		//0x1C4
	}
	//0x1C0
	pspSetK1(oldk1);
	return 0x80550003;
}

/*
 * Subroutine sceNpCore_9218ACF6 - Address 0x000001E8 
 * Exported in sceNpCore
 */
u32 sceNpCore_9218ACF6(int address, int size, int arg2, int arg3) {
	s32 oldk1 = pspShiftK1();
	//0x214
	if (address != 0) {
		//0x21C
		if (((oldk1 << 11) & (((address + size) | address) | size)) >= 0) {
			//0x224
			if (arg2 != 0) {
				//0x238
				if (((oldk1 << 11) & (((arg2 + arg3) | arg2) | arg3)) >= 0) {
					u32 res = sub_00000728((char *) address, size, (char *) arg2,
							(char *) arg3);
					pspSetK1(oldk1);
					return res;
				}
				//0x240
			}
			//0x240
		}
		//0x244
	}
	//0x240
	pspSetK1(oldk1);
	return 0x80550003;
}

/*
 * Subroutine sceNpCore_B13D27CA - Address 0x00000268 
 * Exported in sceNpCore
 */
u32 sceNpCore_B13D27CA(int address, int size) {
	s32 oldk1 = pspShiftK1();
	//0x294
	if (address != 0) {
		//0x29C
		if (((oldk1 << 11) & (((address + size) | address) | size)) >= 0) {
			u32 res = sub_000007B4((int *) address, size);
			pspSetK1(oldk1);
			return res;
		}
		//0x2BC
	}
	//0x2A4
	pspSetK1(oldk1);
	return 0x80550003;
}

/*
 * Subroutine sceNpCore_0366DAB6 - Address 0x000002CC 
 * Exported in sceNpCore
 */
u32 sceNpCore_0366DAB6(int address, int size) {
	s32 oldk1 = pspShiftK1();
	//0x2F8
	if (address != 0) {
		//0x300
		if (((oldk1 << 11) & (((address + size) | address) | size)) >= 0) {
			u32 res = sub_000007F8((int *) address, size);
			pspSetK1(oldk1);
			return res;
		}
		//0x308
	}
	//0x308
	pspSetK1(oldk1);
	return 0x80550003;
}

/*
 * Subroutine sceNpCore_E7AED5A3 - Address 0x00000330 
 * Exported in sceNpCore 
 */
u32 sceNpCore_E7AED5A3(int address, int size) {
	s32 oldk1 = pspShiftK1();
	//0x35C
	if (address != 0) {
		//0x364
		if (((oldk1 << 11) & (((address + size) | address) | size)) >= 0) {
			u32 res = sub_00000840((int *) address, size);
			pspSetK1(oldk1);
			return res;
		}
		//0x36C
	}
	//0x36C
	pspSetK1(oldk1);
	return 0x80550003;
}

/*
 * Subroutine sceNpCore_04096629 - Address 0x00000394 
 * Exported in sceNpCore (No arg0 check)
 */
u32 sceNpCore_04096629(int arg0, int address, int size) {
	s32 oldk1 = pspShiftK1();
	//0x3C0
	if (address != 0) {
		//0x3C8
		if (((oldk1 << 11) & (((address + size) | address) | size)) >= 0) {
			u32 res = sub_000008F8(arg0, (char *)address, size);
			pspSetK1(oldk1);
			return res;
		}
		//0x3D0
	}
	//0x3D0
	pspSetK1(oldk1);
	return 0x80550003;
}

/*
 * Subroutine sceNpCore_515B65E8 - Address 0x000003F8 
 * Exported in sceNpCore
 */
u32 sceNpCore_515B65E8(int arg0, int address, int size) {
	s32 oldk1 = pspShiftK1();
	//0x424
	if (address != 0) {
		//0x42C
		if (((oldk1 << 11) & (((address + size) | address) | size)) >= 0) {
			u32 res = sub_0000045C(arg0, address, size);
			pspSetK1(oldk1);
			return res;
		}
		//0x434
	}
	//0x434
	pspSetK1(oldk1);
	return 0x80550003;
}

/* 
 * TODO: Resolve address
 * Subroutine sub_0000045C - Address 0x0000045C 
 * StringAddress
 * StringSize
 */
u32 sub_0000045C(int arg0, int address, u32 size) {
	char *strPtr = (char *) ((arg0 << 3) + 0x0E00);
	u32 retStatus = 0x80550203;
	u32 tempSize = size;
	int tempAddress = address;
	//0x490 - literal
	if ((arg0 < 3) != 0) {
		retStatus = strlen(strPtr);
		//0x4E0
		retStatus = strncmp("X-I-5-Version", strPtr, retStatus + 1);
		if (retStatus != 0) {
			retStatus = strlen(strPtr);
			//0x4F4 - literal
			if (tempSize == retStatus) {
				//0x544
				//0x54C
				if (strncmp(strPtr, (char *) tempAddress, tempSize) == 0) {
					return retStatus;
				}
			}
			retStatus = strlen(strPtr);
			//0x520
			if (strncmp("X-I-5-Status", strPtr, retStatus + 1) == 0) {
				retStatus = sub_0000098C((char *)tempAddress, tempSize);
				return (retStatus == 0) ? 0x8055030E : retStatus;
			}
			//0x4A0
			return retStatus;
		}
		//0x4A4
	}
	//0x4A0
	return retStatus;
}

/*
 * Subroutine sub_0000055C - Address 0x0000055C 
 */
void sub_0000055C(void) {
	int *s0 = (int *) g_0xE30;
	int res = SysMemForKernel_7FF2F35A(*s0);
	//0x578
	if (res >= 0) {
		s0[8] = 0;
	}
	//0x580
	return;
}

/*
 * TODO: Resolve address
 * Subroutine sub_00000590 - Address 0x00000590
 */
u32 sub_00000590(int arg0, int arg1) {
	int *s0 = (int *) 0x0DE0;
	u32 res = 0x80550202;
	//0x5B0
	if ((*s0 < arg1) != 0) {
		strcpy(((char *)arg0), ((const char *)s0)); //(int, int) strcpy 
		res = *s0;
	}
	//0x5C4
	return res;
}

/*
 * TODO: Resolve address and redo
 * Subroutine sub_000005D8 - Address 0x000005D8
 */
u32 sub_000005D8(int *arg0, char *str, int arg2) {
	int *s0 = arg0;
	int *s1 = (int *) ((s0[0] << 3) + 0x0D98);
	//0x60C
	if (((*arg0 + 4) < 9) != 0) {
		//0x620
		if ((*str < arg2) != 0) {
			strcpy(str, str + 4);
			return *((int *) ((s0[1] << 3) + s1));
		}
		//0x640
		return 0x80550202;
	}
	//0x640
	return 0x80550203;
}

/*
 * TODO: Resolve address & "t0" as unused 
 * Subroutine sub_00000658 - Address 0x00000658 
 */
u32 sub_00000658(char *arg0, char *str, int len, int *arg3, int t0) {
	t0 = 0; //This is temporary to avoid compilation error
	char *tempStr = str;
	char *s1 = arg0;
	int tempLen = len;
	char *s3 = (char *) ((arg0[0] << 3) + arg0);
	char *s5 = (char *) arg3;
	//0x6A4
	if ((s1[0] < 3) != 0) {
		int currLen = strlen(s3);
		//0x6C0
		if ((currLen < tempLen) != 0) {
			strcpy(tempStr, s3);
			currLen = strlen(s3 + 4);
			//0x6E4
			if (currLen != 0) {
				strcpy(s5, s3 + 4);
				return ((s1[0] + 1) < 3) < 1;
			}
			//0x700
			return 0x80550202;
		}
		//0x700
		return 0x80550202;
	}
	//0x700
	return 0x80550203;
}

/*
 * Subroutine sub_00000728 - Address 0x00000728
 */
u32 sub_00000728(char *arg0, int arg1, char *arg2, char *arg3) {
	char *s1 = arg3;
	char *s2 = arg2;
	//0x754
	if ((*((int *) 0x0E20) < arg1) != 0) {
		strcpy(arg0, ((char *) 0x0E24));
		//0x768
		if ((*s1 < 3) == 0) {
			s2[0] = arg3[0];
			s2[1] = arg3[1];
			s2[2] = arg3[2];
			return 0;
		}
		//0x7A8
	}
	//0x7A8
	return 0x80550202;
}

/*
 * Subroutine sub_000007B4 - Address 0x000007B4 
 */
u32 sub_000007B4(void *data, int len) {
	u32 res = 0x80550202;
	//0x7D4
	if ((len < 50) == 0) {
		res = sprintf(data, "https://auth.%s.ac.playstation.net/nav/auth",
				0x0E30);
	}
	//0x7E8
	return res;
}

/*
 * Subroutine sub_000007F8 - Address 0x000007F8 
 */
u32 sub_000007F8(void *arg0, int len) {
	char *s0 = ((char *) 0x0D90);
	//0x818
	if ((s0[0] < len) != 0) {
		strcpy(arg0, s0 + 4);
	}
	//0x82C
	return 0x80550202;
}

/*
 * Subroutine sub_00000840 - Address 0x00000840
 */
u32 sub_00000840(void *buf, int len) {
	PspOpenPSID psid;
	char *s0 = buf; //u8 * s0 = *s3 =  *buf
	int i = 0;
	u32 res = 0x80550202;
	//0x868
	if ((len < 65) == 0) {
		res = sceOpenPSIDGetPSID(psid, 1);
		//0x87C
		if (res) { //since comparison of unsigned expression >= 0 is always true - use if (res) { instead of if (res >= 0) {
			//0x88C
			for (i = 0; i < 16; i++) {
				sprintf(&s0[i], "%02x", psid.data[i]); //(char *) s0[i];
			}
			//0x8B0
			res = sprintf(((char *)s0), "%016llx%016llx", 0, 0, 0, 0);
			res = (s0 + res) - (char *)buf;
		}
		//0x8D8
		return res;
	}
	//0x8D8
	return res;
}

/*
 * Subroutine sub_000008F8 - Address 0x000008F8 
 */
u32 sub_000008F8(int arg0, char * arg1, int len) { //char arg1 - > char * arg1
	char *s3 = (char *) ((arg0 << 3) + arg0);
	//0x934
	if ((arg0 < 3) != 0) {
		int res = strlen(s3); //use res as int instead of unsigned 32.
		//0x950
		if ((res < len) != 0) {
			strcpy(arg1, s3);
			return ((arg0 + 1) < 3) < 1;
		}
		//0x96C
		return 0x80550202;
	}
	//0x96C
	return 0x80550203;
}

/*
 * Subroutine sub_0000098C - Address 0x0000098C
 */
u32 sub_0000098C(char * arg0, int arg1) { //char arg0 -> char * arg0
	int data[4];
	u32 s0 = 0;
	int s1 = (int)arg0 + 0x0E18; //int s1 = arg0 + *((int *) 0x0E18);
	//0x9B8 - literal
	if ((arg1 < (*((int *) 0x0E18) < 2)) == 0) {
		u32 res = strncmp(arg0, ((const char*) 0x0E1C), 0);//one more arg - temporary 0
		//0x9E8
		if (res != 0) {
			return res;
		}
		//0x9F8
		s0 = strtol((char *)s1, (char **)data, 16);
		//0xA10
		if (s1 != data[0]) {
			//0xA18
			if (arg0 == 0) {
				return 0x80550201;
			}
			//0x9F0
			return res | 0x80550400;
		}
		//0x9F0
		return 0x80550400;
	}
	//0x9C0
	return 0;
}
