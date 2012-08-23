/*
 *	ME Wrapper Module 6.60 clone
 *	Copyright (c) 2011 by mowglisanu <mowglisanu@gmain.com>
 *  Copyright (C) 2011 The uOFW team
 *  See the file COPYING for copying permission.
*/

#include <stdarg.h>

#include <common_imp.h>

#include "avcodec_audiocodec.h"
#include "interruptman.h"
#include "iofilemgr_kernel.h"
#include "lfatfs.h"
#include "lowio_ddr.h"
#include "lowio_sysreg.h"
#include "mesgled.h"
#include "syscon.h"
#include "sysmem_kernel.h"
#include "sysmem_sysevent.h"
#include "sysmem_sysclib.h"
#include "sysmem_utils_kernel.h"
#include "threadman_kernel.h"

#include "me_wrapper.h"

SCE_MODULE_INFO("sceMeCodecWrapper", SCE_MODULE_KERNEL | SCE_MODULE_NO_STOP | SCE_MODULE_SINGLE_LOAD 
                                     | SCE_MODULE_SINGLE_START, 1, 9); //I'm not sure if the versions are correct
SCE_SDK_VERSION(SDK_VERSION);

int meStarted;

MERpc meRpc;

int sceMeCore_driver_FA398D71(int cmd, ...);
int sceMeCore_driver_635397BB(int cmd, ...);

int interruptHandler(int a0 __attribute__((unused)), int SceMediaEngineRpcWait)
{
	sceKernelSetEventFlag(SceMediaEngineRpcWait, 1);
	return -1;
}

int initRpc()
{
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
	int ret = sceKernelRegisterIntrHandler(SCE_MECODEC_INT, 2, (void*)&interruptHandler, (void*)meRpc.event, 0);
	if (ret < 0)
		return ret;
	return sceKernelEnableIntr(SCE_MECODEC_INT);
}

int sub_1026()
{
	int ret = 0;
	volatile int* address = (int*)0xbfc00700;
	if (*address == 0)
	{
		ret = sceKernelTryLockMutex(meRpc.mutex, 1);//check to see if the me is in use
		if (ret >= 0)
			sceKernelUnlockMutex(meRpc.mutex, 1);
	}
	else if (*address != -1)
		*address = -2;
	return ret;
}

int sub_0x10005()
{
	if (*(int*)0xBFC00718 != 0) {
		sceSysconCtrlTachyonAvcPower(1);
		*(int*)0xBC100070 |= 4;
	}
	return sub_00001C30((void*)(0x80000000|(sceKernelGetMEeDramSaveAddr() & 0x1fffffff)), 0);
}

int sub_16389()
{
	volatile int *hwAddress = (int*)0xbfc00700;
	*hwAddress = -1;
	int* meTable = (int*)0xbfc00600;
	meTable[0] = 399;//copy sceKernelGetMEeDramSaveSize() bytes on me main memory to sceKernelGetMEeDramSaveAddr() then halt me
	meTable[2] = sceKernelGetMEeDramSaveAddr();
	meTable[3] = sceKernelGetMEeDramSaveSize();
	meTable[4] = 0;
	meTable[5] = 0;
	meTable[6] = 0;
	meTable[7] = 0;
	meTable[8] = 0;
	meTable[9] = 0;
	sceDdrFlush(5);
	sceSysregInterruptToOther();
	while (*hwAddress != -2)
	    ;
	sceDdrFlush(8);
	sceSysregMeResetEnable();
	sceSysregMeBusClockDisable();
	*(int*)0xbfc00718 = *(int*)0xbc100070 & 4;
	sceKernelClearEventFlag(meRpc.event, 0);
	return 0;
}

int sub_0x100000()
{
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
	sceKernelWaitEventFlag(meRpc.event, 1, SCE_EVENT_WAITCLEAR, 0, 0);
	sceKernelUnlockMutex(meRpc.mutex, 1);
	return 0;
}

int sub_0x1000002(int arg)
{
	sceKernelLockMutex(meRpc.mutex, 1, 0);
	int argSr = arg >> 1;
	int val = *(int*)0xbfc00714;
	*(int*)0xbfc00710 = arg;
	int arg0 = 0;
	if (argSr < 133 || val != 1)
	{
		if (argSr >= 148 && val == 3)
			arg0 = 2;
		else
			return 0;
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
	sceKernelWaitEventFlag(meRpc.event, 1, SCE_EVENT_WAITCLEAR, 0, 0);
	return 0;	
}

int sub_0x1000f()
{
	volatile int *hwAddress = (int*)0xbfc00700;
	while (*hwAddress != -4)
	    ;
	if (*(int*)0xbfc00718 == 0){
		*(int*)0xbc100070 = *(int*)0xbc100070 & (~4);//clear 3rd bit
		sceSysregAvcResetEnable();
	}
	*(int*)0xbfc00700 = 0;
	return 0;
}

int eventHandler(int ev_id, char *ev_name __attribute__((unused)), void *param, int *result __attribute__((unused)))
{
	if (!meStarted)
		return 0;
	switch (ev_id){
		case 0x402://phase1 going into standby
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

SceSysEventHandler SceMeRpc = {sizeof(SceSysEventHandler), "SceMeRpc", 0x01ffff00, &eventHandler, 0,0, NULL, {0,0,0,0,0,0,0,0,0}};

int module_start(int argc __attribute__((unused)), void *argp __attribute__((unused)))
{
	initRpc();
	sceLfatfsWaitReady();
	sceMeBootStart(2);
	*(int*)0xbfc00710 = (int)sceSysregPllGetFrequency();
	/* This is what makes mediaengine.prx hang on standby/poweroff */
	sceKernelRegisterSysEventHandler(&SceMeRpc);
	sceMePowerControlAvcPower(0);
	if (sceSysregGetTachyonVersion() > 0x4fffff) // > 01g
	{
		SceSysmemPartitionInfo info;
		info.size = sizeof(SceSysmemPartitionInfo);
		if (sceKernelQueryMemoryPartitionInfo(4, &info) < 0 || info.startaddr != 0x88300000)
			sceKernelAllocPartitionMemory(1, "old ME partition", 2, 0x100000, (void*)0x88300000);
	}
	return 0;
}

int sceMeWrapperInit(int argc, char *argv[]) __attribute__ ((alias ("module_start")));

int sceMeWrapperEnd()
{
	sceSysregVmeResetEnable();	
	sceSysregAvcResetEnable();	
	sceKernelUnregisterSysEventHandler(&SceMeRpc);
	return 0;
}	

int module_reboot_phase(int argc, void *argp __attribute__((unused)))
{
	if (argc == 1)
		sceMeWrapperEnd();
	return 0;
}

/**************************sceMePower_driver****************************/
int sceMePower_driver_1862B784(int numerator, int denominator)
{
	return sceMeCore_driver_FA398D71(390, numerator, denominator);//setCpuGranuality
}

int sceMePower_driver_E9F69ACF(int numerator, int denominator)
{
	return sceMeCore_driver_FA398D71(391, numerator, denominator);//setBusGranuality,meSysregDmac0BusClockEnable,...,meSysregDmac0BusClockEnable
}

int sceMePowerSelectAvcClock(int arg0)
{
	return sceMeCore_driver_FA398D71(7, arg0);//meSysregMsifClkSelect(0, arg0)
}

int sceMePower_driver_984E2608(int enable)
{
	int index;
	if (enable)
		index = 387;//meSysregAwEdramBusClockEnable
	else
		index = 388;//meSysregAwEdramBusClockDisable
	return sceMeCore_driver_FA398D71(index);
}

int sceMePowerControlAvcPower(int arg0)
{
	int ret = sceKernelWaitSema(meRpc.sema, 1, 0);
	if (ret < 0)
		return ret;
	volatile int* hwAddress = (int*)0xbc100070;
	int val = *hwAddress & 4;
	if (arg0 == 0)
	{
		if (val != 0)
		{
			sceSysregAvcResetEnable();
			int intr = sceKernelCpuSuspendIntr();
			*hwAddress = *hwAddress & 0xfffffffb;//clear bit 2
			sceKernelCpuResumeIntr(intr);
			sceSysconCtrlTachyonAvcPower(0);
		}
	}
	else if (val == 0)
	{
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
int sub_000011E4(u32 *arg0, int error)
{
	if (error == (int)0x80000003)
	    return -1;
	arg0[2] = error;
	return (error != -8) ? -3 : -4;
}

int sub_00001214(u32 *arg0, int error)
{
	if (error == (int)0x80000003)
	    return -1;
	arg0[2] = error;
	return (error >= 0) ? -3 : -4;
}

int sceMeVideo_driver_C441994C(int arg0, u32 *arg1)
{
	if (arg1[0] != 0x05100601)
	    return -2;
	if (arg1[4] == 0)
	    return -1;
	int ret;
	if (arg0 == 0)
	{
		ret = sceMeCore_driver_635397BB(0, arg1[4]);
		arg1[6] = ret;
		if (ret == 0)
		{
			arg1[2] = -8;			
			arg1[8] = 0;
			return -3;
		}
		else
		{
			arg1[2] = 1;
			arg1[8] = ((u32*)arg1[4])[1];
			return 0;
		}
	}
	else if (arg0 == 1)
	{
		u32 val = sceMeCore_driver_FA398D71(36);
		ret = sceMeCore_driver_FA398D71(35, arg1[11], arg1[12], arg1[13]);
		arg1[6] = val;
		arg1[8] = ret;
		if (ret <= 0 || val <= 0) {
			arg1[2] = 0x80000000;
			return -3;
		}
		else {
			arg1[2] = 0;
			return 0;
		}
	}
	return -1;
}

int unkVideo;
int sceMeVideo_driver_E8CD3C75(int arg0, u32 *arg1)
{
	if (arg1[0] != 0x05100601)
	    return -2;
	arg1[2] = 0;
	int intr = sceKernelCpuSuspendIntr();
	if (sceKernelGetCompiledSdkVersion() > 0x5FFFFFF && unkVideo != 0) {//for firmware 6.00 and above?
		sceKernelCpuResumeIntr(intr);
		return -4;
	}	
	if ((unkVideo >> arg0) & 1) {
		sceKernelCpuResumeIntr(intr);
		return -4;
	}
	unkVideo = (1 << arg0) | unkVideo;
	if ((unkVideo & 3) == 3) {
		sceKernelCpuResumeIntr(intr);
		return -4;
	}
	sceKernelCpuResumeIntr(intr);
	int ret;
	if (arg0 == 1)
	{
		ret = sceMeCore_driver_635397BB(37, &arg1[3], arg1[5], arg1[6], 0);
		if (ret != 0)
			return sub_00001214(arg1, ret); 
	}
	else if (arg0 == 0)
	{
		ret = sceMeCore_driver_635397BB(1, &arg1[3], arg1[5], arg1[7]);
		if (ret < 0)
			return sub_000011E4(arg1, ret);
		else
			sceMePowerControlAvcPower(1);
	}
	else if (arg0 != 3)
		return -1;
	sceMePower_driver_984E2608(1);
	return 0;
}

//decode
int sceMeVideo_driver_8768915D(int arg0, u32 *arg1)
{
	if (arg1[0] != 0x05100601)
	    return -2;
	int ret;
	arg1[2] = 0;
	if (arg0 == 0)
	{
		int count = arg1[15] - 1;
		if (count >= 4)
			return -1;
		else
		{
			u32 *loc = (u32*)((u32)(count*44)+arg1[11]);
			if (arg1[14] != 0)
			{
				u32 val = (u32)(count*328)+arg1[14];
				do
				{
					loc[9] = val;
					loc[10] = val + 164;
					val -= 328;
					loc -= 11;//44 bytes
					count -= 1;
				} while (count >= 0);
			}
			else
			{
				do
				{
					loc[9] = 0;
					loc[10] = 0;
					count -= 1;
					loc -= 11;//44 bytes
				} while (count >= 0);
			}
			sceKernelDcacheWritebackInvalidateRange((void*)arg1[11], arg1[15]*44);
			sceKernelDcacheWritebackInvalidateRange(&arg1[11], 48);
			ret = sceMeCore_driver_FA398D71(2, arg1[3], arg1[9], arg1[10], arg1[4], arg1[11], arg1[12], arg1[13], &arg1[11]);
			if (ret < 0)
				return sub_000011E4(arg1, ret);
			return 0;
		}
	}
	else if (arg0 == 1)
	{
		((u32*)arg1[4])[1] = arg1[10];
		((u32*)arg1[4])[0] = arg1[9];
		((u32*)arg1[4])[3] = 64;
		((u32*)arg1[4])[2] = arg1[14];
		((u32*)arg1[4])[4] = 0;
		sceKernelDcacheWritebackInvalidateRange((void*)arg1[4], 256);
		ret = sceMeCore_driver_FA398D71(32, arg1[3], arg1[4], arg1[7], arg1[8]);
		if (ret != 0)
			return sub_00001214(arg1, ret);
		return 0;
	}
	return -1;
}

//Stop
int sceMeVideo_driver_4D78330C(int arg0, u32 *arg1)
{
	if (arg1[0] != 0x05100601)
	    return -2;
	arg1[2] = 0;
	int ret;
	if (arg0 == 0)
	{
		int count = (arg1[15] - 1);
		if ((u32)count >= 4)
			return -1;
		else
		{
			u32 *loc = (u32*)(((u32)count*44)+arg1[11]);
			if (arg1[14] != 0)
			{
				u32 val = ((u32)count*328)+arg1[14];
				u32 *loc = (u32*)(((u32)count*44)+arg1[11]);
				do
				{
					loc[9] = val;
					loc[10] = val + 164;
					val -= 328;
					count--;           
					loc -= 11;//44 bytes
				} while (count >= 0);
			}
			else
			{
				do
				{
					loc[9] = 0;
					loc[10] = 0;
					count--;           
					loc -= 11;//44 bytes
				} while (count >= 0);				
			}
			ret = sceMeCore_driver_635397BB(3, arg1[3], arg1[4], arg1[11]);
			if (ret < 0)
				return sub_000011E4(arg1, ret);
			return 0;
		}
	}
	else if (arg0 == 1)
	{
		ret = sceMeCore_driver_635397BB(33, arg1[3]);
		if (ret != 0)
			return sub_00001214(arg1, ret);
		return 0;
	}
	return -1;
}

//Delete, FinishMJPEG
int sceMeVideo_driver_8DD56014(int arg0, u32 *arg1)
{
	if (arg1[0] != 0x05100601)
	    return -2;
	int ret;
	int intr = sceKernelCpuSuspendIntr();
	if (((unkVideo >> arg0) & 1) == 0){
		sceKernelCpuResumeIntr(intr);
		return -4;
	}
	unkVideo = unkVideo - (1 << arg0);
	sceKernelCpuResumeIntr(intr);
	arg1[2] = 0;
	if (arg0 == 0)
	{
		ret = sceMeCore_driver_635397BB(4, arg1[3]);
		if (ret < 0)
			return sub_000011E4(arg1, ret);
		sceMePowerControlAvcPower(0);
	}
	else if (arg0 == 1)
	{
		ret = sceMeCore_driver_635397BB(34, arg1[3]);
		if (ret != 0)
			return sub_00001214(arg1, ret);
	}
	else if (arg0 != 3)
		return -1;
	if (unkVideo == 0)
		sceMePower_driver_984E2608(0);
	return 0;
}

//GetFrameCrop, SetMemory, ScanHeader, GetVersion, GetSEI, _893B32B1
int sceMeVideo_driver_6D68B223(int arg0, u32 arg1, u32 *codec_buffer)
{
	if (codec_buffer[0] != 0x05100601)
	    return -2;
	int ret;
	switch (arg1)
	{
		case 1:
			if (arg0 != 0){
				return -1;
			}
			codec_buffer[2] = 0;
			ret = sceMeCore_driver_635397BB(5, codec_buffer[3], codec_buffer[4], codec_buffer[16], codec_buffer[17], codec_buffer[18], codec_buffer[19]);
			codec_buffer[2] = 0;
			if (ret < 0){
				return sub_000011E4(codec_buffer, ret);
			}
			return 0;
		break;
		case 2:
			if (arg0 != 1){
				return -1;
			}
			((u32*)codec_buffer[4])[0] = codec_buffer[9];
			((u32*)codec_buffer[4])[1] = codec_buffer[10];
			ret = sceMeCore_driver_635397BB(38, codec_buffer[3], codec_buffer[4]);
			if (ret != 0){
				codec_buffer[11] = 0;
				codec_buffer[12] = 0;
				codec_buffer[13] = 0;
				return sub_00001214(codec_buffer, ret);
			}
			codec_buffer[2] = 0;
			codec_buffer[11] = ((u32*)codec_buffer[4])[12];
			codec_buffer[12] = ((u32*)codec_buffer[4])[13];
			codec_buffer[13] = ((u32*)codec_buffer[4])[14];
			return 0;
		break;
		case 3:
			if (arg0 == 0){
				sceMeCore_driver_635397BB(6, &codec_buffer[1]);
				return 0;
			}
			else if (arg0 == 1){
				codec_buffer[1] = sceMeCore_driver_FA398D71(39);
				return 0;
			}
			return -1;
		break;
		case 5:
			if (arg0 != 0){
				return -1 ;
			}
			codec_buffer[2] = 0;
			ret = sceMeCore_driver_635397BB(9, codec_buffer[3], codec_buffer[20]);
			codec_buffer[2] = 0;
			if (ret < 0){
				return sub_000011E4(codec_buffer, ret);
			}
			return 0;
		break;
		case 6:
			if (arg0 != 0){
				return -1;
			}
			codec_buffer[2] = 0;
			ret = sceMeCore_driver_635397BB(10, codec_buffer[3], codec_buffer[21]);
			codec_buffer[2] = 0;
			if (ret < 0){
				return sub_000011E4(codec_buffer, ret);
			}
			return 0;
		break;
		case 7:
			if (arg0 != 0){
				return -1;
			}
			codec_buffer[2] = 0;
			ret = sceMeCore_driver_635397BB(11, codec_buffer[3], codec_buffer[22]);
			codec_buffer[2] = 0;
			if (ret < 0){
				return sub_000011E4(codec_buffer, ret);
			}
			return 0;
		break;
		default:
			return -1;
		break;
	}
}

int sceMeVideo_driver_21521BE5(int a0)
{
	return sceMeCore_driver_635397BB(16, a0);
}

/**************************sceMeAudio_driver****************************/
int sub_00001240(int codec, SceAudiocodecCodec *info, int error)
{
	if (error == (int)0x80000003 || codec < 0x1000 || codec >= 0x1006)
	    return -1;
	switch (codec)
	{
		case 0x1000: //at3+
			sceMeCore_driver_635397BB(106, &info->err, info->edramAddr);
			if (error >= 256)
			    info->unk36 = 0;
			return error < 256 ? -3 : -4;

		case 0x1001://at3
			sceMeCore_driver_635397BB(113, &info->err, info->edramAddr);
			if (error >= 256)
			    info->unk36 = 0;
			return error < 256 ? -3 : -4;

		case 0x1002://mp3
			info->err = error;
			return -3;

		case 0x1003://aac
			sceMeCore_driver_635397BB(145, &info->err, info->edramAddr);
			if (error >= 256)
			    info->unk36 = 0;
			return error < 256 ? -3 : -4;

		case 0x1005://wma
			sceMeCore_driver_635397BB(230, &info->err, info->edramAddr);
			if (error < 0)
			    info->unk36 = 0;
			return error >=0 ? -3 : -4;
	}
	return -1;
}

//decode
int sceMeAudio_driver_9A9E21EE(u32 codec, SceAudiocodecCodec *info) //same -310C9CDA
{
	if (info->unk0 != 0x05100601)
	    return -2;
	if (info->edramAddr == 0)
	    return 0x80000103;
	info->unk36 = 0;
	info->err = 0;
	sceKernelDcacheWritebackInvalidateRange(info, 104);
	int ret = 0;
	if (codec < 0x1000 || codec >= 0x1006)
	    return -1;
	switch (codec)
	{
		case 0x1000: // AT3+
			ret = sceMeCore_driver_FA398D71(96, info);
		break;
		case 0x1001: // AT3
			ret = sceMeCore_driver_FA398D71(112, info);
		break;
		case 0x1002:
			ret = sceMeCore_driver_FA398D71(140, info);
		break;
		case 0x1003:
			ret = sceMeCore_driver_FA398D71(144, info->inBuf, &info->unk28, info->outBuf, &info->unk36, info->edramAddr);
		break;
		case 0x1005:
			ret = sceMeCore_driver_FA398D71(229, info->inBuf, info->unk60, &info->unk64, info->outBuf, &info->unk36, &info->unk68, info->edramAddr);
		break;
		default:
			return -1;
	}
	if (ret < 0)
		return sub_00001240(codec, info, ret);
	return 0;
}

//check need memory
int sceMeAudio_driver_81956A0B(u32 codec, SceAudiocodecCodec *info)
{
	if (info->unk0 != 0x05100601)
	    return -2;
	int ret = 0;
	if (codec < 0x1000 || codec >= 0x1006)
	    return -1;
	switch (codec)
	{
		case 0x1000: //at3+
			ret = sceMeCore_driver_635397BB(99, &info->unk52, &info->unk60, &info->unk64, &info->unk40);
			if (ret >= 0)
			{
				info->err = 0;
				ret = sceMeCore_driver_635397BB(102, info->unk60, &info->neededMem, &info->err);
				if (ret == 0)
					return 0;
				return -3;
			}
			break;

		case 0x1001: //at3
			ret = sceMeCore_driver_635397BB(114, 2, &info->neededMem, &info->err);
			if (ret == 0)
				return 0;
			return -3;
		break;

		case 0x1002: //mp3
			ret = sceMeCore_driver_635397BB(138, 2, &info->neededMem, &info->err);
			if (ret == 0)
				return 0;
			return -3;

		case 0x1003: //aac
			info->neededMem = sceMeCore_driver_FA398D71(146);
			info->err = 0;
			return 0;

		case 0x1005: //wma
			info->neededMem = sceMeCore_driver_FA398D71(226);
			info->err = 0;
			return 0;
		
		default:
			return -1;
	}
	return sub_00001240(codec, info, ret);
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
int sceMeAudio_driver_6AD33F60(u32 codec, SceAudiocodecCodec *info)
{
    if (info->unk0 != 0x05100601)
	    return -2;
	info->err = 0;
	if (codec < 0x1000 || codec >= 0x1006)
		return -1;
	int ret = 0;
	switch (codec)
	{
		case 0x1000://at3+
			ret = sceMeCore_driver_635397BB(99, &info->unk52, &info->unk60, &info->unk64, &info->unk40); // set options (sets unk52, unk60, unk64 with the help of unk40)
			if (ret >= 0){
				int val = info->unk60; // check http://wiki.multimedia.cx/index.php?title=ATRAC3plus#Multichannel_ATRAC3plus_.28ATRAC-X.29 : it's probably this!
				switch (val){
					case 0:
					default :
						info->err = 514;
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
				info->unk72 = 2;
				ret = sceMeCore_driver_635397BB(103, info->unk52, info->unk60, info->unk64, 2, info->edramAddr); // setup channels, probably
				if (ret >= 0){
					ret = sceMeCore_driver_635397BB(105, &info->unk44, info->edramAddr); // probably sets unk44
					if (ret >= 0){
						ret = sceMeCore_driver_635397BB(100, &info->unk68, info->edramAddr); // probably sets unk68
						if (ret >= 0){
							ret = sceMeCore_driver_FA398D71(104, info->unk20, info->edramAddr); // check unk20 (where is it set???)
						}
					}
				}
			}
		break;
		case 0x1001://at3
			*(int*)&info->unk44 = 44100;
			info->unk48 = unk[*(int*)&info->unk40 * 2 + 1];
			ret = sceMeCore_driver_635397BB(115, unk[*(int*)&info->unk40 * 2], 44100, unk[*(int*)&info->unk40 * 2 + 1], info->edramAddr);
			if (*(int*)&info->unk40 >= 14 && *(int*)&info->unk40 < 16)
				info->unk52 = 2;
		break;
		case 0x1002://mp3
			ret = sceMeCore_driver_635397BB(139, info->edramAddr);
		break;
		case 0x1003://aac
			switch (*(int*)&info->unk40){
				case 96000:
				case 88200:
				case 64000:
				case 48000:
				case 44100:
				case 32000:
				case 24000:
				case 22050:
				case 16000:
				case 11050:
				case 8000:
					ret = sceMeCore_driver_635397BB(147, *(int*)&info->unk40, info->edramAddr);
					if (ret >= 0)
					{
						if (*(short*)&info->unk44 != 0)
						{
							ret = sceMeCore_driver_635397BB(149, info->unk44, info->edramAddr);
							if (ret >= 0 && info->unk45 != 0)
								ret = sceMeCore_driver_635397BB(151, 0, 2, 0, info->edramAddr);
						}
						else if (info->unk45 != 0)
							ret = sceMeCore_driver_635397BB(151, 0, 2, 0, info->edramAddr);
					}
				break;
				default:
					return -1;
				break;
			}			
		break;
		case 0x1004://not used
			return -1;
		break;
		case 0x1005://wma
			ret = sceMeCore_driver_635397BB(227, &info->unk40, info->edramAddr);
		break;
	}
	if (ret < 0)
		return sub_00001240(codec, info, ret);
	return ret;	
}

//probably for umd(only at3/+)
int sceMeAudio_driver_B57F033A(u32 codec, SceAudiocodecCodec *info)
{
	if (info->unk0 != 0x05100601)
	    return -1;
	info->err = 0;
	int ret;
	switch (codec)
	{
	case 0x1000:
		ret = sceMeCore_driver_635397BB(99, &info->unk52, &info->unk60, &info->unk64, &info->unk40);
		if (ret >= 0)
		{
			info->err = 0;
			if (info->unk60 != -1)
				return -1;
			info->unk72 = info->unk60;
			ret = sceMeCore_driver_635397BB(103, info->unk52, 1, info->unk64, 1, info->edramAddr);
			if (ret >= 0)
			{
				ret = sceMeCore_driver_635397BB(105, &info->unk44, info->edramAddr);
				if (ret >= 0)
				{
					ret = sceMeCore_driver_635397BB(100, &info->unk68, info->edramAddr);
					if (ret >= 0)
						ret = sceMeCore_driver_FA398D71(104, info->unk20, info->edramAddr);
				}
			}
		}
		break;

    case 0x1001:
        if (*(int*)&info->unk40 < 14 || *(int*)&info->unk40 >= 16)
			return -1;
		info->unk52 = 1;
		info->unk48 = unk[*(int*)&info->unk40 * 2 + 1];
		ret = sceMeCore_driver_635397BB(115, unk[*(int*)&info->unk40 * 2], 44100, unk[*(int*)&info->unk40 * 2 + 1], info->edramAddr);
		break;
	
	default:
		return -1;
	}
	if (ret < 0)
		return sub_00001240(codec, info, ret);
	return ret;
}
//get info?
int sceMeAudio_driver_C300D466(u32 codec, u32 arg1, SceAudiocodecCodec *info)
{
	if (info->unk0 != 0x05100601)
		return -2;
	info->err = 0;
	int ret;
	switch (arg1)
	{
	case 3: {
		u32 codecIndex = codec - 0x1000;
		int index;
		switch(codecIndex)
		{
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
			case 5:
				index = 225;
			break;
			default:
				return -1;
			break;
		}
		ret = sceMeCore_driver_FA398D71(index);
		info->unk4 = ret;
		return ret;
	}
	case 4: {
		if (codec != 4096) {
			return -1;
		}
		//at3+ checkneed mem
		ret = sceMeCore_driver_635397BB(99, &info->unk52, &info->unk60, &info->unk64, &info->unk40);
		if (ret < 0) {
			return sub_00001240(codec, info, ret);
		}
		return ret;
	}
	case 2: {
		if (codec != 4098){
			return -1;
		}
		ret = sceMeCore_driver_635397BB(137, info->unk40, info->edramAddr);
		if (ret == 0) {
			ret = sceMeCore_driver_635397BB(130, info->inBuf, &info->unk44, &info->unk56, info->edramAddr);
		}
		if (ret < 0){
			return sub_00001240(codec, info, ret);
		}
		return ret;
	}
	default:
    	return -1;
    }
}

/**************************sceMeMemory_driver****************************/
void *sceMeMalloc(int size)
{
	return (void*)sceMeCore_driver_FA398D71(384, size);
}

void *sceMeCalloc(int num, int size)
{
	return (void*)sceMeCore_driver_FA398D71(386, num, size);
}

void sceMeFree(void *ptr)
{
	sceMeCore_driver_FA398D71(385, ptr);
}
/**************************sceMeCore_driver****************************/
int sceMeRpcLock()
{
	return sceKernelLockMutex(meRpc.mutex, 1, 0);
}

int sceMeRpcUnlock()
{
	return sceKernelUnlockMutex(meRpc.mutex, 1);
}

//me*img.img contains 2 sets of compressed data(except maybe sd), code and contents of main memory
__attribute__((noreturn)) void decompressAndRunMeImage(void *data)
{
	void *end = data;
	int ret;
	if (0x3fffff < ((u32)data & ~0x88000000)){
		((void (*)(void*))0x88300000)(end);
	}
	*(int*)0xbfc00700 = -4;
	if (memcmp(data, "KL4E", 4) == 0){
		ret = UtilsForKernel_6C6887EE((void*)0x88300000, (u32)data & ~0x88300000, data + 4, &end);//decompress meimg.img
		if (ret >= 0){
			sceKernelDcacheWritebackInvalidateAll()	;
			sceKernelIcacheInvalidateAll();
			((void (*)(void*))0x88300000)(end);
		}
	}
	sceKernelMemset32(end, 0, 0x88400000 - (u32)end);
	*(int*)0xBFC00700 = -5;	
	*(int*)0xBFC00710 = -2;
	pspHalt();
	for (;;)
	    ;
}

int decrypt(void *data, int size)
{
	int ret, newSize;
	ret = sceWmd_driver_7A0E484C(data, size, &newSize);
	if (ret < 0) {
		return ret;
	}
	return newSize;
}

int sub_00001C30(void* data, int wait)
{
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

int sceMeBootStart(u32 arg)
{
	if (arg >= 5)
	    return 0x80000102;
	int genArg = arg;
	u32 tachyon = sceSysregGetTachyonVersion();
	if (tachyon > 0x4fffff && genArg != 2) // > 01g
		genArg = 3;
	if (genArg == meStarted)
		return 0;
	int ret = sceMeRpcLock();
	if (ret < 0)
		return ret;
	if (((genArg == 3) && (meStarted == 2)) || ((genArg == 2) && (meStarted == 3))){ // 2 and 3 are both meimg.img so don't "reboot" when switching between them
			meStarted = genArg;
			sceMeRpcUnlock();
			sceMeCore_driver_FA398D71(389, genArg != 2);
			return 0;
	}
	char *meImage;
	if (tachyon > 0x4fffff) //not first gen
		meImage = "flash0:/kd/resource/me_t2img.img";
	else if (genArg == 1)
		meImage = "flash0:/kd/resource/me_sdimg.img";
	else if (genArg == 4)
		meImage = "flash0:/kd/resource/me_blimg.img";
	else
		meImage = "flash0:/kd/resource/meimg.img";
	SceUID fd = sceIoOpen(meImage, SCE_O_UNKNOWN0|SCE_O_RDONLY, 0);
	if (fd < 0){
		sceMeRpcUnlock();
		return fd;
	}
	int size = sceIoLseek(fd, 0, SCE_SEEK_END);
	sceIoLseek(fd, 0, SCE_SEEK_SET);
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
		return (read < 0) ? read : (int)0x80000022;
	}
	decrypt(address, read);
	sub_00001C30(address, 1);
	meStarted = genArg;
	sceMeRpcUnlock();
	sceMeCore_driver_FA398D71(389, genArg != 2);
	return 0;
}

int sceMeCore_driver_635397BB(int cmd, ...)
{
    va_list ap;
    va_start(ap, cmd);
	sceKernelDcacheWritebackInvalidateAll();
	int ret = sceMeCore_driver_FA398D71(cmd, va_arg(ap, int), va_arg(ap, int), va_arg(ap, int), va_arg(ap, int), va_arg(ap, int), va_arg(ap, int), va_arg(ap, int), va_arg(ap, int));
	va_end(ap);
	return ret;
}

int sceMeCore_driver_FA398D71(int cmd, ...)
{
    va_list ap;
    va_start(ap, cmd);
	int ret = sceKernelLockMutex(meRpc.mutex, 1, 0);
	if (ret < 0)
		return ret;
	int* meTable = (int*)0xbfc00600;
	meTable[0] = cmd;
	meTable[2] = va_arg(ap, int);
	meTable[3] = va_arg(ap, int);
	meTable[4] = va_arg(ap, int);
	meTable[5] = va_arg(ap, int);
	meTable[6] = va_arg(ap, int);
	meTable[7] = va_arg(ap, int);
	meTable[8] = va_arg(ap, int);
	meTable[9] = va_arg(ap, int);
	va_end(ap);
	sceDdrFlush(5);
	sceSysregInterruptToOther();
	sceKernelWaitEventFlag(meRpc.event, 1, SCE_EVENT_WAITCLEAR, 0, 0);
	ret = meTable[10];
	sceKernelUnlockMutex(meRpc.mutex, 1);
	return ret;
}

int sceMeCore_driver_905A7500()
{
	return unkVideo;
}

