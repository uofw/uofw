#include <common_imp.h>

SCE_MODULE_INFO("sceMlnBridge_MSApp_Driver", 0x1006, 1, 1);
SCE_MODULE_BOOTSTART("sceMlnBridge_msapp_driver_0ED6A564");
SCE_MODULE_STOP("sceMlnBridge_msapp_driver_C41F1B67");

// Headers
extern int sceRtc_driver_CEEF238F(void *);

int sub_000003E0(const char *path, const char *pathType, void *data, int);
int sub_00000318(char *);

/*
  Subroutine sceMlnBridge_msapp_D527DEB0 - Address 0x00000000 
  Exported in sceMlnBridge_msapp
 */
s32 sceMlnBridge_msapp_D527DEB0(char *arg0, int arg1) {
	char data[16];
	s32 res = 0x80000023;

	s32 oldK1 = pspShiftK1(); //s1
	int arg3 = 0; //Recheck

	//0x34
	if (((oldK1 << 11) & ((arg3 | (int)arg0) | arg1)) >= 0) {
		//0x40
		if ((arg1 < 4) == 0) {
			*((int *) data) = 0;
			data[4] = 0;
			res = sub_000003E0("/CONFIG/SYSTEM/LOCK", "password", data, 5);
			//0x68 - literal
			if (res != 0) {
				pspSetK1(oldK1);
				return -1;
			}
			//0x78
			if (arg0[0] != data[0]) {
				pspSetK1(oldK1);
				return -1;
			}
			//0xA0
			//0xA4 - literal
			if (arg0[1] != data[1]) {
				pspSetK1(oldK1);
				return -1;
			}
			//0xB4 - literal
			if (arg0[2] != data[2]) {
				pspSetK1(oldK1);
				return -1;
			}
			//0xC8 - j
			pspSetK1(oldK1);
			return ((arg0[3] ^ data[3]) == 0) ? 0 : -1;
		}
		//0xD0
		res = 0x80000104;
	}
	//0xE0
	pspSetK1(oldK1);
	return res;
}

/*
 Subroutine module_start - Address 0x000000F0 - Aliases: sceMlnBridge_msapp_driver_0ED6A564
 Exported in syslib
 Exported in sceMlnBridge_msapp_driver
 */
u32 sceMlnBridge_msapp_driver_0ED6A564(s32 argc __attribute__((unused)),
		void *argp __attribute__((unused))) {
	return SCE_ERROR_OK;
}

/*
 Subroutine module_stop - Address 0x000000F8 - Aliases: sceMlnBridge_msapp_driver_C41F1B67
 Exported in syslib
 Exported in sceMlnBridge_msapp_driver
 */
u32 sceMlnBridge_msapp_driver_C41F1B67(s32 argc __attribute__((unused)),
		void *argp __attribute__((unused))) {
	return SCE_ERROR_OK;
}

/*
 Subroutine sceMlnBridge_msapp_3811BA77 - Address 0x00000100
 Exported in sceMlnBridge_msapp
 */
s32 sceMlnBridge_msapp_3811BA77(int address) {
	s32 res = 0x80000023;
	s32 oldK1 = pspShiftK1();
	//0x128
	if ((((address + 8) | address) & (oldK1 << 11)) >= 0) {
		res = sceRtc_driver_CEEF238F((int *) address);	// (void *) -> ?
	}
	//0x138
	pspSetK1(oldK1);
	return res;
}

/*
 Subroutine sceMlnBridge_msapp_F02B9478 - Address 0x0000014C 
 Exported in sceMlnBridge_msapp
 0x00000814: "flash0:/vsh/module/mlncmn.prx"
 0x00000834: "flash0:/vsh/module/mcore.prx"
 */
s32 sceMlnBridge_msapp_F02B9478(u32 arg0) {
	s32 res = -1;
	char *mod_path[] = { "flash0:/vsh/module/mlncmn.prx",
			"flash0:/vsh/module/mcore.prx" };
	s32 oldK1 = pspGetK1();
	//0x1B4
	if ((arg0 < 4) != 0) {
		oldK1 = pspShiftK1();
		res = sub_00000318((char *)mod_path);
		pspSetK1(oldK1);
	}
	//0x1C8
	return -1;
}

/*
 ok
 Subroutine sceMlnBridge_msapp_CC6037D7 - Address 0x000001D8 
 Exported in sceMlnBridge_msapp
 "flash0:/kd/np_commerce2_store.prx" ref
 */
s32 sceMlnBridge_msapp_CC6037D7(u32 arg0) {
	arg0 = 0; // Temporary - to get rid of compiler error
	s32 res;
	char mod_path[] = { "flash0:/kd/np_commerce2_store.prx" };
	s32 oldK1 = pspShiftK1();
	res = sub_00000318(mod_path);
	pspSetK1(oldK1);
	return res;
}

/*
 Subroutine sceMlnBridge_msapp_494B3B0B - Address 0x00000220 
 Exported in sceMlnBridge_msapp
 */
 
/*
 Subroutine sceMlnBridge_msapp_0398DEFF - Address 0x00000284  
 Exported in sceMlnBridge_msapp
*/

/*
 Subroutine sceMlnBridge_msapp_7AD66017 - Address 0x000002B4 
 Exported in sceMlnBridge_msapp
*/

/*
 Subroutine sub_00000318 - Address 0x00000318 
*/

/*
 Subroutine sub_000003E0 - Address 0x000003E0 
*/
