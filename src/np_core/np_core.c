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

u32 sub_0045C(int, int, u32);
void sub_0055C();
u32 sub_00590(int, int);
u32 sub_005D8(int *, char *, int);
u32 sub_00658(char *, char *, int, int *, int);
u32 sub_00728(char *, int, char *, char *);
u32 sub_007B4(void *, int);
u32 sub_007F8(void *, int);
u32 sub_00840(void *, int);
u32 sub_008F8(int, char *, int);
u32 sub_0098C(char *, int);


/*
 * .data section
 */
int g_0xE30;
int g_0xE34;
int g_0xE38;

/*
 * Address 0x00000000 
 */
void sceNpCore_57E15796(void) {
	s32 oldk1 = pspShiftK1();
	sub_0055C();
	pspSetK1(oldk1);
}

/*
 * Address 0x0000002C
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
 * Address 0x00000084
 */
u32 sceNpCore_243690EE(int address, int size) {
	s32 oldk1 = pspShiftK1();
	//0xB0
	if (address != 0) {
		//0xB8
		if (((oldk1 << 11) & (((address + size) | address) | size)) >= 0) {
			u32 res = sub_00590(address, size);
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
 * Address 0x000000E8 
 */
u32 sceNpCore_8AFAB4A0(int a0, int address, int size) {
	s32 oldk1 = pspShiftK1();
	//0x11C
	if (((a0 < 1) | (size < 1)) == 0) {
		//0x124
		if ((((a0 + 12) | a0) & (a0 < 1)) >= 0) {
			//0x138
			if (((oldk1 << 11) & (((address + size) | address) | size)) >= 0) {
				u32 res = sub_005D8((int *) a0, (char *) address, size);
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
 * Address 0x00000168 
 */
u32 sceNpCore_5145344F(char *a0, int address, int size, int a3, int t0) {
	s32 oldk1 = pspShiftK1();
	//0x194
	if (address != 0) {
		//0x19C
		if (((oldk1 << 11) & (((address + size) | address) | size)) >= 0) {
			//0x1A4
			if (a3 != 0) {
				//0x1B8
				if (((oldk1 << 11) & (((a3 | t0) | a3) | t0)) >= 0) {
					u32 res = sub_00658(a0, (char *) address, size, (int *) a3,
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
 * Address 0x000001E8 
 */
u32 sceNpCore_9218ACF6(int address, int size, int a2, int a3) {
	s32 oldk1 = pspShiftK1();
	//0x214
	if (address != 0) {
		//0x21C
		if (((oldk1 << 11) & (((address + size) | address) | size)) >= 0) {
			//0x224
			if (a2 != 0) {
				//0x238
				if (((oldk1 << 11) & (((a2 + a3) | a2) | a3)) >= 0) {
					u32 res = sub_00728((char *) address, size, (char *) a2,
							(char *) a3);
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
 * Address 0x00000268 
 */
u32 sceNpCore_B13D27CA(int address, int size) {
	s32 oldk1 = pspShiftK1();
	//0x294
	if (address != 0) {
		//0x29C
		if (((oldk1 << 11) & (((address + size) | address) | size)) >= 0) {
			u32 res = sub_007B4((int *) address, size);
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
 * Address 0x000002CC 
 */
u32 sceNpCore_0366DAB6(int address, int size) {
	s32 oldk1 = pspShiftK1();
	//0x2F8
	if (address != 0) {
		//0x300
		if (((oldk1 << 11) & (((address + size) | address) | size)) >= 0) {
			u32 res = sub_007F8((int *) address, size);
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
 * Address 0x00000330 
 */
u32 sceNpCore_E7AED5A3(int address, int size) {
	s32 oldk1 = pspShiftK1();
	//0x35C
	if (address != 0) {
		//0x364
		if (((oldk1 << 11) & (((address + size) | address) | size)) >= 0) {
			u32 res = sub_00840((int *) address, size);
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
 * Address 0x00000394 
 */
u32 sceNpCore_04096629(int a0, int address, int size) {
	s32 oldk1 = pspShiftK1();
	//0x3C0
	if (address != 0) {
		//0x3C8
		if (((oldk1 << 11) & (((address + size) | address) | size)) >= 0) {
			u32 res = sub_008F8(a0, (char *)address, size);
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
 * Address 0x000003F8 
 */
u32 sceNpCore_515B65E8(int a0, int address, int size) {
	s32 oldk1 = pspShiftK1();
	//0x424
	if (address != 0) {
		//0x42C
		if (((oldk1 << 11) & (((address + size) | address) | size)) >= 0) {
			u32 res = sub_0045C(a0, address, size);
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
 * Address 0x0000045C 
 * StringAddress
 * StringSize
 */
u32 sub_0045C(int a0, int address, u32 size) {
	char *strPtr = (char *) ((a0 << 3) + 0x0E00);
	u32 retStatus = 0x80550203;
	u32 tempSize = size;
	int tempAddress = address;
	//0x490 - literal
	if ((a0 < 3) != 0) {
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
				retStatus = sub_0098C((char *)tempAddress, tempSize);
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
 * Address 0x0000055C
 */
void sub_0055C(void) {
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
 * Address 0x00000590 
 */
u32 sub_00590(int a0, int a1) {
	int *s0 = (int *) 0x0DE0;
	u32 res = 0x80550202;
	//0x5B0
	if ((*s0 < a1) != 0) {
		strcpy(((char *)a0), ((const char *)s0)); //(int, int) strcpy 
		res = *s0;
	}
	//0x5C4
	return res;
}

/*
 * TODO: Resolve address and redo
 * Address 0x000005D8 
 */
u32 sub_005D8(int *a0, char *str, int a2) {
	int *s0 = a0;
	int *s1 = (int *) ((s0[0] << 3) + 0x0D98);
	//0x60C
	if (((*a0 + 4) < 9) != 0) {
		//0x620
		if ((*str < a2) != 0) {
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
 * Address 0x00000658 
 */
u32 sub_00658(char *a0, char *str, int len, int *a3, int t0) {
	t0 = 0; //This is temporary to avoid compilation error
	char *tempStr = str;
	char *s1 = a0;
	int tempLen = len;
	char *s3 = (char *) ((a0[0] << 3) + a0);
	char *s5 = (char *) a3;
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
 * Address 0x00000728 
 */
u32 sub_00728(char *a0, int a1, char *a2, char *a3) {
	char *s1 = a3;
	char *s2 = a2;
	//0x754
	if ((*((int *) 0x0E20) < a1) != 0) {
		strcpy(a0, ((char *) 0x0E24));
		//0x768
		if ((*s1 < 3) == 0) {
			s2[0] = a3[0];
			s2[1] = a3[1];
			s2[2] = a3[2];
			return 0;
		}
		//0x7A8
	}
	//0x7A8
	return 0x80550202;
}

/*
 * Address 0x000007B4 
 */
u32 sub_007B4(void *data, int len) {
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
 * Address 0x000007F8 
 */
u32 sub_007F8(void *a0, int len) {
	char *s0 = ((char *) 0x0D90);
	//0x818
	if ((s0[0] < len) != 0) {
		strcpy(a0, s0 + 4);
	}
	//0x82C
	return 0x80550202;
}

/*
 * Address 0x00000840 
 */
u32 sub_00840(void *buf, int len) {
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
 * Address 0x000008F8 
 */
u32 sub_008F8(int a0, char * a1, int len) { //char a1 - > char * a1
	char *s3 = (char *) ((a0 << 3) + a0);
	//0x934
	if ((a0 < 3) != 0) {
		int res = strlen(s3); //use res as int instead of unsigned 32.
		//0x950
		if ((res < len) != 0) {
			strcpy(a1, s3);
			return ((a0 + 1) < 3) < 1;
		}
		//0x96C
		return 0x80550202;
	}
	//0x96C
	return 0x80550203;
}

/*
 * Address 0x0000098C
 */
u32 sub_0098C(char * a0, int a1) { //char a0 -> char * a0
	int data[4];
	u32 s0 = 0;
	int s1 = (int)a0 + 0x0E18; //int s1 = a0 + *((int *) 0x0E18);
	//0x9B8 - literal
	if ((a1 < (*((int *) 0x0E18) < 2)) == 0) {
		u32 res = strncmp(a0, ((const char*) 0x0E1C), 0);//one more arg - temporary 0
		//0x9E8
		if (res != 0) {
			return res;
		}
		//0x9F8
		s0 = strtol((char *)s1, (char **)data, 16);
		//0xA10
		if (s1 != data[0]) {
			//0xA18
			if (a0 == 0) {
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
