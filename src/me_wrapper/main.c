/*
 *	ME Wrapper Module 6.60 clone
 *	Copyright (c) 2011 by mowglisanu <mowglisanu@gmain.com>
 *  Copyright (C) 2011 The uOFW team
 *  See the file COPYING for copying permission.
*/

#include <pspkernel.h>
#include <pspsysevent.h>
#include <pspsysmem_kernel.h>
#include "me_wrapper.h"

PSP_MODULE_INFO("sceMeCodecWrapper", 0x1007, 1, 9);//I'm not sure if the versions are correct

u32 module_sdk_version = 0x06060010;
int meStarted;

MERpc meRpc;
int interruptHandler(int a0, int SceMediaEngineRpcWait){
	sceKernelSetEventFlag(SceMediaEngineRpcWait, 1);
	return -1;
}
int initRpc(){
	meRpc.mutex = sceKernelCreateMutex("SceMediaEngineRpc", 0x101, 0, 0);
	if (meRpc.mutex < 0)
		return meRpc.mutex;
	meRpc.sema = sceKernelCreateSema("SceMediaEngineAvcPower" , 0x101, 1, 1, 0);
	if (meRpc.sema < 0)
		return meRpc.sema;
	meRpc.event = sceKernelCreateEventFlag("SceMediaEngineRpcWait", 0x201, 0, 0);
	if (meRpc.event < 0)
		return meRpc.event;
	sceSysregIntrEnd();
	int ret = sceKernelRegisterIntrHandler(PSP_MECODEC_INT, 2, (void*)&interruptHandler, (void*)meRpc.event, 0);
	if (ret < 0)
		return ret;
	return sceKernelEnableIntr(PSP_MECODEC_INT);
}
int sub_1026(){
	int ret = 0;
	volatile int* address = (int*)0xbfc00700;
	if (*address == 0){
		ret = sceKernelTryLockMutex(meRpc.mutex, 1);//check to see if the me is in use
		if (ret>=0){
			sceKernelUnlockMutex(meRpc.mutex, 1);
		}
	}
	else{
		if (*address != -1){
			*address = -2;
		}
	}
	return ret;
}
int sub_0x10005(){
	if (*(int*)0xBFC00718 != 0){
		sceSysconCtrlTachyonAvcPower(1);
		*(int*)0xBC100070 |= 4;
	}
	return sub_00001C30((void*)(0x80000000|((u32)SysMemForKernel_FDC97D28/*0xCDA3A2F7*/()&0x1fffffff)), 0);
}
int sub_16389(){
	volatile int *hwAddress = (int*)0xbfc00700;
	*hwAddress = -1;
	int* meTable = (int*)0xbfc00600;
	meTable[0] = 399;//copy SysMemForKernel_D3CA555C bytes on me main memory to SysMemForKernel_FDC97D28 then halt me
	meTable[2] = (int)SysMemForKernel_FDC97D28();
	meTable[3] = SysMemForKernel_D3CA555C();
	meTable[4] = 0;
	meTable[5] = 0;
	meTable[6] = 0;
	meTable[7] = 0;
	meTable[8] = 0;
	meTable[9] = 0;
	sceDdrFlush(5);
	sceSysregInterruptToOther();
	while (*hwAddress != -2);
	sceDdrFlush(8);
	sceSysregMeResetEnable();
	sceSysregMeBusClockDisable();
	*(int*)0xbfc00718 = *(int*)0xbc100070 & 4;
	sceKernelClearEventFlag(meRpc.event, 0);
	return 0;
}
int sub_0x100000(){
	 sceMeRpcLock();
	if (*(int*)0xbfc00718 == 0)
		sceSysconCtrlTachyonAvcPower(0);
	sceMeRpcUnlock();
	return 0;
}
int sub_0x1000020(){
	volatile int* address = (int*)0xbfc0070c;
	int first = *address;
	int second = *address;
	int* meTable = (int*)0xbfc00600;
	meTable[0] = 391;//same as sceMePower_driver_E9F69ACF
	meTable[2] = (first & 0x1ff0000)>>16;
	meTable[3] = second & 0x1ff;
	meTable[4] = 0;
	meTable[5] = 0;
	meTable[6] = 0;
	meTable[7] = 0;
	meTable[8] = 0;
	meTable[9] = 0;
	sceDdrFlush(5);
	sceSysregInterruptToOther();
	sceKernelWaitEventFlag(meRpc.event, 1, PSP_EVENT_WAITCLEAR, 0, 0);
	sceKernelUnlockMutex(meRpc.mutex, 1);
	return 0;
}
int sub_0x1000002(int arg){
	sceKernelLockMutex(meRpc.mutex, 1, 0);
	int argSr = arg >> 1;
	int val = *(int*)0xbfc00714;
	int comp1 = argSr < 133;
	int comp2 = argSr < 148;
	u32 swapBit0 = val^1;
	int isValEq1 = swapBit0 < 1;
	u32 swapBits0And1 = val^3;
	int notComp1 = comp1^1;
	int notComp2 = comp2^1;
	int isValEq3 = swapBits0And1 < 1;
	int result1 = notComp1 & isValEq1;
	int result2 = notComp2 & isValEq3;
	*(int*)0xbfc00710 = arg;
	int arg0 = 0;
	if (result1 == 0){//if (((arg >> 1) < 133) || (val != 1))
		if (result2 == 0){//if (((arg >> 1) < 148) || (val != 3))
			return 0;//if (((arg >> 1) < 148) || (val != 3) || (val != 1))
		}
		else{
			arg0 = 2;
		}
	}
	int* meTable = (int*)0xbfc00600;
	meTable[0] = 7;//sets 0xbfc00714 to arg0 then calls sceSysregMsifClkSelect(0, arg0) same as sceMePowerSelectAvcClock
	meTable[2] = arg0;//if (val==1 && ((arg>>1)>=133)) arg = 0, if (val==3 && ((arg>>1)>=133)) arg0 = 2
	meTable[3] = 0;
	meTable[4] = 0;
	meTable[5] = 0;
	meTable[6] = 0;
	meTable[7] = 0;
	meTable[8] = 0;
	meTable[9] = 0;
	sceDdrFlush(5);
	sceSysregInterruptToOther();
	sceKernelWaitEventFlag(meRpc.event, 1, PSP_EVENT_WAITCLEAR, 0, 0);
	return 0;	
}
int sub_0x1000f(){
	volatile int *hwAddress = (int*)0xbfc00700;
	while (*hwAddress != -4);
	if (*(int*)0xbfc00718 == 0){
		*(int*)0xbc100070 = *(int*)0xbc100070 & (~4);//clear 3rd bit
		sceSysregAvcResetEnable();
	}
	*(int*)0xbfc00700 = 0;
	return 0;
}
int eventHandler(int ev_id, char *ev_name, void *param, int *result){
	if (!meStarted)
		return 0;
	switch (ev_id){
		case 1026://phase1 going into standby
			return sub_1026();
		break;
		case 16389://phase0 going into standby
			return sub_16389();
		break;
		case 0x10005://phase0 comming out of standby
			return sub_0x10005();
		break;
		case 0x1000f://phase0 comming out of standby
			return sub_0x1000f();
		break;
		case 0x100000://phase1 comming out of standby
			return sub_0x100000();
		break;
		case 0x1000002://power stuff?
			sub_0x1000002(((int*)param)[1]);
		break;
		case 0x1000020://power stuff?
			sub_0x1000020();
		break;
	}
	return 0;
}
PspSysEventHandler SceMeRpc = {sizeof(PspSysEventHandler), "SceMeRpc", 0x01ffff00, &eventHandler, 0,0, NULL, {0,0,0,0,0,0,0,0,0}};

int module_start(int argc, char *argv[]){
	initRpc();
	sceLfatfsWaitReady();
	sceMeBootStart(2);
	*(int*)0xbfc00710 = (int)sceSysregPllGetFrequency();
	/* This is what makes mediaengine.prx hang on standby/poweroff */
	sceKernelRegisterSysEventHandler(&SceMeRpc);
	sceMePowerControlAvcPower(0);
	if (sceSysregGetTachyonVersion() > 0x4fffff){// > 01g
		PspSysmemPartitionInfo info;
		info.size = sizeof(PspSysmemPartitionInfo);
		if (sceKernelQueryMemoryPartitionInfo(4, &info) < 0){
			sceKernelAllocPartitionMemory(1, "old ME partition", 2, 0x100000, (void*)0x88300000);
		}
		else if (info.startaddr != 0x88300000){
			sceKernelAllocPartitionMemory(1, "old ME partition", 2, 0x100000, (void*)0x88300000);
		}
	}
	return 0;
}
int sceMeWrapperInit(int argc, char *argv[]) __attribute__ ((alias ("module_start")));

int sceMeWrapperEnd(){
	sceSysregVmeResetEnable();	
	sceSysregAvcResetEnable();	
	sceKernelUnregisterSysEventHandler(&SceMeRpc);
	return 0;
}	

int module_reboot_phase(int argc, char *argv[]){
	if (argc == 1){
		sceMeWrapperEnd();
	}
	return 0;
}

/**************************sceMePower_driver****************************/
int sceMePower_driver_1862B784(int numerator, int denominator){
	return sceMeCore_driver_FA398D71(390, numerator, denominator);//setCpuGranuality
}
int sceMePower_driver_E9F69ACF(int numerator, int denominator){
	return sceMeCore_driver_FA398D71(391, numerator, denominator);//setBusGranuality,meSysregDmac0BusClockEnable,...,meSysregDmac0BusClockEnable
}
int sceMePowerSelectAvcClock(int arg0){
	return sceMeCore_driver_FA398D71(7, arg0);//meSysregMsifClkSelect(0, arg0)
}
int sceMePower_driver_984E2608(int enable){
	int index;
	if (enable){
		index = 387;//meSysregAwEdramBusClockEnable
	}
	else{
		index = 388;//meSysregAwEdramBusClockDisable
	}
	return sceMeCore_driver_FA398D71(index);

}
int sceMePowerControlAvcPower(int arg0){
	int ret = sceKernelWaitSema(meRpc.sema, 1, 0);
	if (ret < 0){
		return ret;
	}
	volatile int* hwAddress = (int*)0xbc100070;
	int val = *hwAddress & 4;
	if (arg0 == 0){
		if (val != 0){
			sceSysregAvcResetEnable();
			int intr = sceKernelCpuSuspendIntr();
			*hwAddress = *hwAddress & 0xfffffffb;//clear bit 2
			sceKernelCpuResumeIntr(intr);
			sceSysconCtrlTachyonAvcPower(0);
		}
	}
	else if (val == 0){
		sceSysconCtrlTachyonAvcPower(arg0);
		int intr = sceKernelCpuSuspendIntr();
		*hwAddress = *hwAddress | 4;
		sceKernelCpuResumeIntr(intr);
		sceMeCore_driver_FA398D71(8);
		sceSysregAvcResetDisable();
	}
	*(int*)0xbfc00718 = *hwAddress & 4;
	return sceKernelSignalSema(meRpc.sema, 1);
}
/**************************sceMeVideo_driver****************************/
int sub_000011E4(u32 *arg0, int error){
	if (error == 0x80000003) return -1;
	arg0[2] = error;
	return (error ^ -8)?-3:-4;//-8 = 0xfffffff8
//	if (error == -8) return -4;
//	return -3;
}
int sub_00001214(u32 *arg0, int val){//is val an error?
	if (val == 0x80000003) return -1;
	arg0[2] = val;
	return (val >= 0)?-3:-4;
}
int sceMeVideo_driver_C441994C(int arg0, u32 *arg1){
	if (0x5100601 < arg1[0]) return -2;
	if (arg1[4] == 0) return -1;
	int ret;
	if (arg0 == 0){
		ret = sceMeCore_driver_635397BB(0, arg1[4]);
		arg1[6] = ret;
		if (ret == 0){
			arg1[2] = -8;			
			arg1[8] = 0;
			return -3;
		}
		else{
			arg1[2] = 1;
			arg1[8] = ((u32*)arg1[4])[1];
			return 0;
		}
	}
	else if (arg0 == 1){
		u32 val = sceMeCore_driver_FA398D71(36);
		ret = sceMeCore_driver_FA398D71(35, arg1[11], arg1[12], arg1[13]);
		arg1[6] = val;
		arg1[8] = ret;
		if ((ret <= 0) || (val <= 0)){
			arg1[2] = 0x80000000;
			return -3;
		}
		else{
			arg1[2] = 0;
			return 0;
		}
	}
	return -1;
}
int unkVideo;
int sceMeVideo_driver_E8CD3C75(int arg0, u32 *arg1){
	if (0x5100601 < arg1[0]) return -2;
	arg1[2] = 0;
	int intr = sceKernelCpuSuspendIntr();//why?
	if (sceKernelGetCompiledSdkVersion() > 0x5FFFFFF){//for firmware 6.00 and above?
		if (unkVideo != 0){
			sceKernelCpuResumeIntr(intr);
			return -4;
		}
	}	
	if ((unkVideo >> arg0) & 1){
		sceKernelCpuResumeIntr(intr);
		return -4;
	}
	unkVideo = (1 << arg0) | unkVideo;
	if ((unkVideo & 3) == 3){
		sceKernelCpuResumeIntr(intr);
		return -4;
	}
	sceKernelCpuResumeIntr(intr);
	int ret;
	if (arg0 == 1){
		ret = sceMeCore_driver_635397BB(37, &arg1[3], arg1[5], arg1[6], 0);
		if (ret != 0){
			return sub_00001214(arg1, ret); 
		}
	}
	else if (arg0 == 0){
		ret = sceMeCore_driver_635397BB(1, &arg1[3], arg1[5], arg1[7]);
		if (ret < 0){
			return sub_000011E4(arg1, ret);
		}
		else{
			sceMePowerControlAvcPower(1);
		}		
	}
	else if (arg0 != 3){
		return -1;
	}
	sceMePower_driver_984E2608(1);
	return 0;
}
//decode
int sceMeVideo_driver_8768915D(int arg0, u32 *arg1){
	if (0x5100601 < arg1[0]) return -2;
	int ret;
	arg1[2] = 0;
	if (arg0 == 0){
		int count = arg1[15] - 1;
		if (count >= 4){
			return -1;
		}
		else{
			u32 *loc = (u32*)((u32)(count*44)+arg1[11]);
			if (arg1[14] != 0){
				u32 val = (u32)(count*328)+arg1[14];
				do{
					loc[9] = val;
					loc[10] = val + 164;
					val -= 328;
					loc -= 11;//44 bytes
					count -= 1;
				}
				while (count >= 0);
			}
			else{
				do{
					loc[9] = 0;
					loc[10] = 0;
					count -= 1;
					loc -= 11;//44 bytes
				}
				while (count >= 0);
			}
			sceKernelDcacheWritebackInvalidateRange((void*)arg1[11], arg1[15]*44);
			sceKernelDcacheWritebackInvalidateRange(&arg1[11], 48);
			ret = sceMeCore_driver_FA398D71(2, arg1[3], arg1[9], arg1[10], arg1[4], arg1[11], arg1[12], arg1[13], &arg1[11]);
			if (ret < 0){
				return sub_000011E4(arg1, ret);
			}
			return 0;
		}
	}
	else if (arg0 == 1){
		((u32*)arg1[4])[1] = arg1[10];
		((u32*)arg1[4])[0] = arg1[9];
		((u32*)arg1[4])[3] = 64;
		((u32*)arg1[4])[2] = arg1[14];
		((u32*)arg1[4])[4] = 0;
		sceKernelDcacheWritebackInvalidateRange((void*)arg1[4], 256);
		ret = sceMeCore_driver_FA398D71(32, arg1[3], arg1[4], arg1[7], arg1[8]);
		if (ret != 0){
			return sub_00001214(arg1, ret);
		}
		return 0;
	}
	return -1;
}
//Stop
int sceMeVideo_driver_4D78330C(int arg0, u32 *arg1){
	if (0x5100601 < arg1[0]) return -2;
	arg1[2] = 0;
	int ret;
	if (arg0 == 0){
		int count = (arg1[15] - 1);
		if ((u32)count >= 4){
			return -1;
		}
		else{
			u32 *loc = (u32*)(((u32)count*44)+arg1[11]);
			if (arg1[14] != 0){
				u32 val = ((u32)count*328)+arg1[14];
				u32 *loc = (u32*)(((u32)count*44)+arg1[11]);
				do{
					loc[9] = val;
					loc[10] = val + 164;
					val -= 328;
					count--;           
					loc -= 11;//44 bytes
				}
				while (count >= 0);
			}
			else{
				do{
					loc[9] = 0;
					loc[10] = 0;
					count--;           
					loc -= 11;//44 bytes
				}
				while (count >= 0);				
			}
			ret = sceMeCore_driver_635397BB(3, arg1[3], arg1[4], arg1[11]);
			if (ret < 0){
				return sub_000011E4(arg1, ret);
			}
			return 0;
		}
	}
	else if (arg0 == 1){
		ret = sceMeCore_driver_635397BB(33, arg1[3]);
		if (ret != 0){
			return sub_00001214(arg1, ret);
		}
		return 0;
	}
	return -1;
}
//Delete, FinishMJPEG
int sceMeVideo_driver_8DD56014(int arg0, u32 *arg1){
	if (0x5100601 < arg1[0]) return -2;
	int ret;
	int intr = sceKernelCpuSuspendIntr();
	if (((unkVideo >> arg0) & 1) == 0){
		sceKernelCpuResumeIntr(intr);
		return -4;
	}
	unkVideo = unkVideo - (1 << arg0);
	sceKernelCpuResumeIntr(intr);
	arg1[2] = 0;
	if (arg0 == 0){
		ret = sceMeCore_driver_635397BB(4, arg1[3]);
		if (ret < 0){
			return sub_000011E4(arg1, ret);
		}
		sceMePowerControlAvcPower(0);
	}
	else if (arg0 == 1){
		ret = sceMeCore_driver_635397BB(34, arg1[3]);
		if (ret != 0){
			return sub_00001214(arg1, ret);
		}
	}
	else if (arg0 != 3){
		return -1;
	}
	if (unkVideo == 0){
		sceMePower_driver_984E2608(0);
	}
	return 0;
}
//GetFrameCrop, SetMemory, ScanHeader, GetVersion, GetSEI, _893B32B1
int sceMeVideo_driver_6D68B223(int arg0, u32 arg1, u32 *arg2){
	if (0x5100601 < arg2[0]) return -2;
	int ret;
	switch (arg1){
		case 0:
		case 4:
		default:
			return -1;
		break;
		case 1:
			if (arg0 != 0){
				return -1;
			}
			arg2[2] = 0;
			ret = sceMeCore_driver_635397BB(5, arg2[3], arg2[4], arg2[16], arg2[17], arg2[18], arg2[19]);
			arg2[2] = 0;
			if (ret < 0){
				return sub_000011E4(arg2, ret);
			}
			return 0;
		break;
		case 2:
			if (arg0 != 1){
				return -1;
			}
			((u32*)arg2[4])[0] = arg2[9];
			((u32*)arg2[4])[1] = arg2[10];
			ret = sceMeCore_driver_635397BB(38, arg2[3], arg2[4]);
			if (ret != 0){
				arg2[11] = 0;
				arg2[12] = 0;
				arg2[13] = 0;
				return sub_00001214(arg2, ret);
			}
			arg2[2] = 0;
			arg2[11] = ((u32*)arg2[4])[12];
			arg2[12] = ((u32*)arg2[4])[13];
			arg2[13] = ((u32*)arg2[4])[14];
			return 0;
		break;
		case 3:
			if (arg0 == 0){
				sceMeCore_driver_635397BB(6, &arg2[1]);
				return 0;
			}
			else if (arg0 == 1){
				arg2[1] = sceMeCore_driver_FA398D71(39);
				return 0;
			}
			return -1;
		break;
		case 5:
			if (arg0 != 0){
				return -1 ;
			}
			arg2[2] = 0;
			ret = sceMeCore_driver_635397BB(9, arg2[3], arg2[20]);
			arg2[2] = 0;
			if (ret < 0){
				return sub_000011E4(arg2, ret);
			}
			return 0;
		break;
		case 6:
			if (arg0 != 0){
				return -1;
			}
			arg2[2] = 0;
			ret = sceMeCore_driver_635397BB(10, arg2[3], arg2[21]);
			arg2[2] = 0;
			if (ret < 0){
				return sub_000011E4(arg2, ret);
			}
			return 0;
		break;
		case 7:
			if (arg0 != 0){
				return -1;
			}
			arg2[2] = 0;
			ret = sceMeCore_driver_635397BB(11, arg2[3], arg2[22]);
			arg2[2] = 0;
			if (ret < 0){
				return sub_000011E4(arg2, ret);
			}
			return 0;
		break;
	}
	return 0xf00d;
}
int sceMeVideo_driver_21521BE5(int a0){
	return sceMeCore_driver_635397BB(16, a0);
}
/**************************sceMeAudio_driver****************************/
int sub_00001240(u32 codec, unsigned long *codec_buffer, int error){
	if (error == 0x80000003) return -1;
	u32 codecIndex = codec - 0x1000;
	if (codecIndex > 5) return -1;
	switch (codecIndex){
		case 0://at3+
			sceMeCore_driver_635397BB(106, &codec_buffer[2], codec_buffer[3]);
			if (error>=256){
				codec_buffer[9] = 0;
			}
			return error<256?-3:-4;
		break;
		case 1://at3
			sceMeCore_driver_635397BB(113, &codec_buffer[2], codec_buffer[3]);
			if (error>=256){
				codec_buffer[9] = 0;
			}
			return error<256?-3:-4;
		break;
		case 2://mp3
			codec_buffer[2] = (u32)error;
			return -3;
		break;
		case 3://aac
			sceMeCore_driver_635397BB(145, &codec_buffer[2], codec_buffer[3]);
			if (error>=256){
				codec_buffer[9] = 0;
			}
			return error<256?-3:-4;
		break;
//		case 4:
//			return -1;
		break;
		case 5://wma
			sceMeCore_driver_635397BB(230, &codec_buffer[2], codec_buffer[3]);
			if (error<0){
				codec_buffer[9] = 0;
			}
			return error>=0?-3:-4;
		break;
	}
	return -1;
}
//decode
int sceMeAudio_driver_9A9E21EE(u32 codec, unsigned long *codec_buffer){ //same -310C9CDA
	unsigned long *cb17 = &codec_buffer[17];
	unsigned long *cb16 = &codec_buffer[16];
	void *memory = (void*)codec_buffer[3];
	void *data = (void*)codec_buffer[6];
	u32 samples = codec_buffer[8];
	if (codec_buffer[0] > 0x05100601) return -2;
	if (codec_buffer[3] == 0) return 0x80000103;
	codec_buffer[9] = 0;
	codec_buffer[2] = 0;
	sceKernelDcacheWritebackInvalidateRange(codec_buffer, 104);
	u32 codecIndex = codec - 0x1000;
	if (codecIndex > 5) return -1;
	int ret = 0;
	switch (codecIndex){
		case 0:
			ret = sceMeCore_driver_FA398D71(96, codec_buffer);
		break;
		case 1:
			ret = sceMeCore_driver_FA398D71(112, codec_buffer);
		break;
		case 2:
			ret = sceMeCore_driver_FA398D71(140, codec_buffer);
		break;
		case 3:
//			ret = sceMeCore_driver_FA398D71(144, codec_buffer[6], &codec_buffer[7], codec_buffer[8], &codec_buffer[9], codec_buffer[3]);
			ret = sceMeCore_driver_FA398D71(144, data, &codec_buffer[7], samples, &codec_buffer[9], memory);
		break;
		default:
		case 4:
			return -1;
		break;
		case 5:
//			ret = sceMeCore_driver_FA398D71(229, codec_buffer[6], codec_buffer[15], &codec_buffer[16], codec_buffer[8], &codec_buffer[9], &codec_buffer[17], codec_buffer[3]);
			ret = sceMeCore_driver_FA398D71(229, data, codec_buffer[15], cb16, samples, &codec_buffer[9], cb17, memory);
		break;
	}
	if (ret < 0){
		return sub_00001240(codec, codec_buffer, ret);
	}
	return 0;
}
//check need memory
int sceMeAudio_driver_81956A0B(u32 codec, unsigned long *codec_buffer){
	if (0x5100601 < codec_buffer[0]) return -2;
	u32 codecIndex = codec - 0x1000;
	if (codecIndex > 5) return -1;
	int ret = 0;
	switch (codecIndex){
		case 0://at3+
			ret = sceMeCore_driver_635397BB(99, &codec_buffer[13], &codec_buffer[15], &codec_buffer[16], &codec_buffer[10]);
			if (ret >= 0){
				codec_buffer[2] = 0;
				ret = sceMeCore_driver_635397BB(102, codec_buffer[15], &codec_buffer[4], &codec_buffer[2]);
				if (ret == 0){
					return 0;
				}
				return -3;
			}
		break;
		case 1://at3
			ret = sceMeCore_driver_635397BB(114, 2, &codec_buffer[4], &codec_buffer[2]);
			if (ret == 0){
				return 0;
			}
			return -3;
		break;
		case 2://mp3
			ret = sceMeCore_driver_635397BB(138, 2, &codec_buffer[4], &codec_buffer[2]);
			if (ret == 0){
				return 0;
			}
			return -3;
		break;
		case 3://aac
			codec_buffer[4] = sceMeCore_driver_FA398D71(146);//=0x648c
			codec_buffer[2] = 0;
			return 0;
			break;
		case 4://not supported
			return -1;
		break;
		case 5://wma
			codec_buffer[4] = sceMeCore_driver_FA398D71(226);
			codec_buffer[2] = 0;
			return 0;
		break;
	}
	return sub_00001240(codec, codec_buffer, ret);
}
int unk[] = {//guessed, codec_buffer[10] should be 4 or 6, got 8744 from a snd0 file
			0x00000000,
			0x00000100,
			0x00000001,
			0x00000100,
			0x00000000,
			0x000000D4,
			0x00000001,
			0x000000D4,
			0x00000000,
			0x000000C0,
			0x00000001,
			0x000000C0,
			0x00000000,
			0x00000098,
			0x00000001,
			0x00000098,
			0x00000000,
			0x00000088,
			0x00000001,
			0x00000088,
			0x00000000,
			0x00000060,
			0x00000001,
			0x00000060,
			0x00000001,
			0x00000044,
			0x00000001,
			0x00000030,
			0x00000002,
			0x000000C0,
			0x00000002,
			0x00000098};
//init
int sceMeAudio_driver_6AD33F60(u32 codec, unsigned long *codec_buffer){
	if (0x05100601 < codec_buffer[0]) return -2;
	u32 codecIndex = codec - 0x1000;
	codec_buffer[2] = 0;
	if (codecIndex > 5) return -1;
	int ret = 0;
	switch (codecIndex){
		case 0://at3+
			ret = sceMeCore_driver_635397BB(99, &codec_buffer[13], &codec_buffer[15], &codec_buffer[16], &codec_buffer[10]);
			if (ret >= 0){
				int val = codec_buffer[15];
				switch (val){
					case 0:
					default :
						codec_buffer[2] = 514;
						return -3;
					break;
					case 1:
						val = 2;
					break;
					case 2:
					case 3:
					case 4:
					break;
					case 5:
					case 6:
					case 7:
						val++;
					break;
				}
				codec_buffer[18] = 2;
				ret = sceMeCore_driver_635397BB(103, codec_buffer[13], codec_buffer[15], codec_buffer[16], 2, codec_buffer[3]);
				if (ret >= 0){
					ret = sceMeCore_driver_635397BB(105, &codec_buffer[11], codec_buffer[3]);
					if (ret >= 0){
						ret = sceMeCore_driver_635397BB(100, &codec_buffer[17], codec_buffer[3]);
						if (ret >= 0){
							ret = sceMeCore_driver_FA398D71(104, codec_buffer[5], codec_buffer[3]);
						}
					}
				}
			}
		break;
		case 1://at3
			codec_buffer[11] = 44100;
			codec_buffer[12] = unk[codec_buffer[10]*2+1];
			ret = sceMeCore_driver_635397BB(115, unk[codec_buffer[10]*2], 44100, unk[codec_buffer[10]*2+1], codec_buffer[3]);
			if ((codec_buffer[10]-14)<2){
				codec_buffer[13] = 2;
			}
		break;
		case 2://mp3
			ret = sceMeCore_driver_635397BB(139, codec_buffer[3]);
		break;
		case 3://aac
			switch (codec_buffer[10]){
				case 96000:
				case 88200:
				case 64000:
				case 48000:
				case 44100:
				case 32000:
				case 24000:
				case 22050:
				case 16000:
//				case 12000: not supported
				case 11050:
				case 8000:
					ret = sceMeCore_driver_635397BB(147, codec_buffer[10], codec_buffer[3]);
					if (ret >= 0){					
						if (((short*)codec_buffer)[22] != 0){
							ret = sceMeCore_driver_635397BB(149, ((u8*)codec_buffer)[44], codec_buffer[3]);
							if (ret >= 0){
								if (((char*)codec_buffer)[45] != 0){
									ret = sceMeCore_driver_635397BB(151, 0, 2, 0, codec_buffer[3]);
								}
							}
						}
						else{
							if (((char*)codec_buffer)[45] != 0){
								ret = sceMeCore_driver_635397BB(151, 0, 2, 0, codec_buffer[3]);
							}
						}
					}
				break;
				default:
					return -1;
				break;
			}			
		break;
		case 4://not used
			return -1;
		break;
		case 5://wma
			ret = sceMeCore_driver_635397BB(227, &codec_buffer[10], codec_buffer[3]);
		break;
	}
	if (ret < 0){
		return sub_00001240(codec, codec_buffer, ret);
	}
	return ret;	
}
//probably for umd(only at3/+)
int sceMeAudio_driver_B57F033A(u32 codec, unsigned long *codec_buffer){
	if (0x5100601 < codec_buffer[0]) return -2;
	codec_buffer[2] = 0;
	int ret;
	if (codec == 0x1000){
		ret = sceMeCore_driver_635397BB(99, &codec_buffer[13], &codec_buffer[15], &codec_buffer[16], &codec_buffer[10]);
		if (ret >= 0){
			codec_buffer[2] = 0;
			if (codec_buffer[15] != -1){
				return -1;
			}
			codec_buffer[18] = codec_buffer[15];
			ret = sceMeCore_driver_635397BB(103, codec_buffer[13], 1, codec_buffer[16], 1, codec_buffer[3]);
			if (ret >= 0){
				ret = sceMeCore_driver_635397BB(105, &codec_buffer[11], codec_buffer[3]);
				if (ret >= 0){
					ret = sceMeCore_driver_635397BB(100, &codec_buffer[17], codec_buffer[3]);
					if (ret >= 0){
						ret = sceMeCore_driver_FA398D71(104, codec_buffer[5], codec_buffer[3]);
					}
				}
			}
		}
	}
	else if (codec == 0x1001){
		if ((u32)(codec_buffer[10]-14) >= 2){
			return -1;
		}
		codec_buffer[13] = 1;
		codec_buffer[12] = unk[codec_buffer[10]*2+1];
		ret = sceMeCore_driver_635397BB(115, unk[codec_buffer[10]*2], 44100, unk[codec_buffer[10]*2+1], codec_buffer[3]);
	}
	else{
		return -1;
	}
	if (ret < 0){
		return sub_00001240(codec, codec_buffer, ret);
	}
	return ret;
}
//get info?
int sceMeAudio_driver_C300D466(u32 codec, u32 arg1, unsigned long *codec_buffer){
	if (0x5100601 < codec_buffer[0]) return -2;
	codec_buffer[2] = 0;
	int ret;
	if (arg1 == 3){
		u32 codecIndex = codec - 0x1000;
		int index;
		switch(codecIndex){
			case 0:
				index = 97;
			break;
			case 1:
				index = 116;
			break;
			case 2:
				index = 129;
			break;
			case 3:
				index = 148;
			break;
			case 4:
			default:
				return -1;
			break;
			case 5:
				index = 225;
			break;
		}
		ret = sceMeCore_driver_FA398D71(index);
		codec_buffer[1] = ret;
		return ret;
	}
	else if (arg1 >= 4){
		if (arg1 != 4){
			return -1;
		}
		if (codec != 4096){
			return -1;
		}
		//at3+ checkneed mem
		ret = sceMeCore_driver_635397BB(99, &codec_buffer[13], &codec_buffer[15], &codec_buffer[16], &codec_buffer[10]);
		if (ret < 0){
			return sub_00001240(codec, codec_buffer, ret);
		}
		return ret;
	}
	else if (arg1 == 2){
		if (codec != 4098){
			return -1;
		}
		ret = sceMeCore_driver_635397BB(137, codec_buffer[10], codec_buffer[3]);
		if (ret == 0){
			ret = sceMeCore_driver_635397BB(130, codec_buffer[6], &codec_buffer[11], &codec_buffer[14], codec_buffer[3]);
		}
		if (ret < 0){
			return sub_00001240(codec, codec_buffer, ret);
		}
		return ret;
	}
	return -1;
}
/**************************sceMeMemory_driver****************************/
void *sceMeMalloc(int size){
	return (void*)sceMeCore_driver_FA398D71(384, size);
}
void *sceMeCalloc(int num, int size){
	return (void*)sceMeCore_driver_FA398D71(386, num, size);
}
void sceMeFree(void *ptr){
	sceMeCore_driver_FA398D71(385, ptr);
}
/**************************sceMeCore_driver****************************/
int sceMeRpcLock(){
	return sceKernelLockMutex(meRpc.mutex, 1, 0);
}
int sceMeRpcUnlock(){
	return sceKernelUnlockMutex(meRpc.mutex, 1);
}
//me*img.img contains 2 sets of compressed data(except maybe sd), code and contents of main memory
__attribute__((noreturn)) void decompressAndRunMeImage(void *data){
	void *end = data;
	int ret;
	if (0x3fffff < ((u32)data & ~0x88000000)){
		((void (*)(void*))0x88300000)(end);
	}
	*(int*)0xbfc00700 = -4;
	if (memcmp(data, "KL4E", 4) == 0){
		ret = UtilsForKernel_6C6887EE((void*)0x88300000, (void*)((u32)data & ~0x88300000), data + 4, &end);//decompress meimg.img
		if (ret >= 0){
			sceKernelDcacheWritebackInvalidateAll()	;
			sceKernelIcacheInvalidateAll();
			((void (*)(void*))0x88300000)(end);
		}
	}
	sceKernelMemset32(end, 0, (void*)0x88400000 - (u32)end);
	*(int*)0xBFC00700 = -5;	
	*(int*)0xBFC00710 = -2;
	//dead end
	asm volatile("haltLoop:\n"
				 ".word 0x70000000\n"
				 "b haltLoop\n"
				 "nop\n");
}
int decrypt(void *data, int size){
	int ret, newSize;
	ret = sceWmd_driver_7A0E484C(data, size, &newSize);
	if (ret < 0){
		return ret;
	}
	return newSize;
}
int sub_00001C30(void* data, int wait){
	sceSysregMeResetEnable();
	sceSysregMeBusClockEnable();
	int size = (u32)me_boot_code_end - (u32)me_boot_code;
	memcpy((void*)0xbfc00040, (void*)me_boot_code, size);// me_boot_code same
	*(int*)0xbfc00060 = *(int*)0xbfc00060 | (*(int*)0xbc100040 & 3);//li $t, [0,1,2,3]
	*(int*)0xbfc00064 = *(int*)0xbfc00064 | ((u32)data >> 16);
	*(int*)0xbfc00068 = *(int*)0xbfc00068 | ((u32)data & 0xffff);
	sceKernelDcacheWritebackInvalidateAll();
	sceDdrFlush(4);
	if (wait){
		*(int*)0xbfc0070c = 0x1ff01ff;
		*(int*)0xbfc00714 = 2;
	}
	volatile int *hwAddress = (int*)0xbfc00700;
	*hwAddress = -3;
	sceSysregVmeResetDisable();//stuff rearranged
	sceSysregMeResetDisable();
	if (wait){
		while (*hwAddress == -3){
			sceKernelDelayThread(1000);
		}
	}
	*hwAddress = 0;
	sceSysregAvcResetDisable();
	sceDdrFlush(8);
	return 0;
}
int sceMeBootStart(u32 arg){
	if (arg >= 5) return 0x80000102;
	int genArg = arg;
	u32 tachyon = sceSysregGetTachyonVersion();
	if (tachyon > 0x4fffff){// > 01g
		if (genArg != 2){
			genArg = 3;
		}	
//		genArg ^= 2;
//		genArg = genArg?3:2;
	}
	if (genArg == meStarted){
		return 0;
	}
	int ret =  sceMeRpcLock();
	if (ret < 0){
		return ret;
	}
	if (((genArg == 3)&&(meStarted == 2)) || ((genArg == 2)&&(meStarted == 3))){
			meStarted = genArg;
			sceMeRpcUnlock();
			sceMeCore_driver_FA398D71(389, (u32)(genArg^2)>0);//genArg==2?0:1
			return 0;
	}
	char *meImage;
	if (tachyon > 0x4fffff){//not first gen
		meImage = "flash0:/kd/resource/me_t2img.img";
	}
	else if (genArg == 1){
		meImage = "flash0:/kd/resource/me_sdimg.img";
	}
	else if (genArg == 4){
		meImage = "flash0:/kd/resource/me_blimg.img";
	}
	else{
		meImage = "flash0:/kd/resource/meimg.img";
	}
	SceUID fd = sceIoOpen(meImage, PSP_O_UNKNOWN0|PSP_O_RDONLY, 0);
	if (fd < 0){
		sceMeRpcUnlock();
		return fd;
	}
	int size = sceIoLseek(fd, 0, PSP_SEEK_END);
	sceIoLseek(fd, 0, PSP_SEEK_SET);
	if (size > 0x63FFF){
		sceIoClose(fd);
		sceMeRpcUnlock();
		return 0x80000022;
	}
	sceSysregMeResetEnable();
	void *address = (void*)((u32)0x883ff000 - (size & 0xffffffc0));
	int read = sceIoRead(fd, address, size);
	sceIoClose(fd);
	if (read != size){
		sceMeRpcUnlock();
		return (read<0)?read:0x80000022;
	}
	decrypt(address, read);
	sub_00001C30(address, 1);
	meStarted = genArg;
	sceMeRpcUnlock();
	sceMeCore_driver_FA398D71(389, ((u32)(genArg^2))>0);
	return 0;
}
int sceMeCore_driver_635397BB(int index, int arg0, int arg1, int arg2, int arg3, int arg4, int arg5, int arg6, int arg7){
	sceKernelDcacheWritebackInvalidateAll();
	return sceMeCore_driver_FA398D71(index, arg0, arg1, arg2, arg3, arg4, arg5, arg6, arg7);
}
int sceMeCore_driver_FA398D71(int index, int arg0, int arg1, int arg2, int arg3, int arg4, int arg5, int arg6, int arg7){
	int ret = sceKernelLockMutex(meRpc.mutex, 1, 0);
	if (ret < 0){
		return ret;
	}
	int* meTable = (int*)0xbfc00600;
	meTable[0] = index;
	meTable[2] = arg0;
	meTable[3] = arg1;
	meTable[4] = arg2;
	meTable[5] = arg3;
	meTable[6] = arg4;
	meTable[7] = arg5;
	meTable[8] = arg6;
	meTable[9] = arg7;
	sceDdrFlush(5);
	sceSysregInterruptToOther();
	sceKernelWaitEventFlag(meRpc.event, 1, PSP_EVENT_WAITCLEAR, 0, 0);
	ret = meTable[10];
	sceKernelUnlockMutex(meRpc.mutex, 1);
	return ret;
}
int sceMeCore_driver_905A7500(){
	return unkVideo;
}

