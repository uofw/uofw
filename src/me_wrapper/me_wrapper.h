/*
 *	ME Wrapper Module 6.60 clone
 *	Copyright (c) 2011 by mowglisanu <mowglisanu@gmain.com>
 *  Copyright (C) 2011 The uOFW team
 *  See the file COPYING for copying permission.
*/
typedef struct{
	int mutex;
	int event;
	int sema;
}MERpc;

#define PSP_O_UNKNOWN0 0x04000000

int sceMeRpcLock();
int sceMeRpcUnlock();
int sub_00001C30(void* data, int wait);
int sceMeBootStart(unsigned int bootCode);
int sceMePowerControlAvcPower(int arg0);

extern void me_boot_code_end();
extern void me_boot_code();

