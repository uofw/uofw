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

extern void *memcpy(void *, void *, int);
extern int memcmp(void *, void *, int);
extern void* SysMemForKernel_5339A163();
extern int SysMemForKernel_374E8E66();
extern void* sceKernelMemset32(void*, int, void*);
extern int sceSysregAvcResetEnable();
extern int sceSysregAvcResetDisable();
extern int sceSysregInterruptToOther();
extern float sceSysregPllGetFrequency();
extern unsigned int sceSysregGetTachyonVersion();
extern int sceSysregIntrEnd();
extern int sceDdrFlush(int);
extern int sceKernelCreateMutex(char *, int, int, int);
extern int sceKernelTryLockMutex(int, int);
extern int sceKernelLockMutex(int, int, int);
extern int sceKernelUnlockMutex(int, int);
extern int sceKernelEnableIntr(int);
extern void sceSysconCtrlTachyonAvcPower(int);
extern int UtilsForKernel_6C6887EE(void*, void*, void*, void**);
extern void sceKernelIcacheInvalidateAll();
extern int sceWmd_driver_7A0E484C(void* , int, int*);
