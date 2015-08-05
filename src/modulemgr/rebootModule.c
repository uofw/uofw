/* Copyright (C) 2011 - 2015 The uOFW team
   See the file COPYING for copying permission.
*/

#include <common_imp.h>
#include <loadcore.h>
#include <modulemgr.h>
#include <modulemgr_kernel.h>
#include <modulemgr_options.h>
#include <threadman_kernel.h>

#include "loadModuleChecks_inline.h"
#include "modulemgr_int.h"

// Subroutine ModuleMgrForKernel_CC873DFA - Address 0x000046E4
s32 sceKernelRebootBeforeForUser(void *arg)
{
	s32 oldGp;
	u32 modCount;
	s32 status;
	SceUID uidBlkId;
	SceUID *uidList;
	char threadArgs[16];

	oldGp = pspGetGp();
	sceKernelLockMutex(g_ModuleManager.semaId, 1, NULL); //0x00004724

	memcpy(threadArgs, arg, sizeof(threadArgs)); //0x00004734
	((u32 *)threadArgs)[0] = 16;

	uidBlkId = sceKernelGetModuleListWithAlloc(&modCount); //0x00004744
	if (uidBlkId < SCE_ERROR_OK)
		return uidBlkId;

	uidList = sceKernelGetBlockHeadAddr(uidBlkId); //0x00004754

	u32 i;
	SceModule *pMod;
	s32 threadMode;
	for (i = modCount - 1; i >= 0; i--) { //0x00004760
		pMod = sceKernelFindModuleByUID(uidList[i]); //0x00004774
		if (pMod == NULL || ((s32)pMod->moduleRebootBefore) == -1) //0x0000477C
			continue;

		if (GET_MCB_STATUS(pMod->status) != MCB_STATUS_STARTED || !(pMod->status & SCE_MODULE_USER_MODULE)) //0x0000479C - 0x00004830
			continue;

		s32 priority = pMod->moduleRebootBeforeThreadPriority;
		if (priority == -1)
			priority = SCE_KERNEL_MODULE_INIT_PRIORITY; //0x00004864

		s32 stackSize = pMod->moduleRebootBeforeThreadStacksize;
		if (stackSize == -1) //0x00004870
			stackSize = SCE_KERNEL_TH_DEFAULT_SIZE;

		s32 attr = pMod->moduleRebootBeforeThreadAttr;
		if (attr == -1) //0x00004874
			attr = SCE_KERNEL_TH_DEFAULT_ATTR;

		// TODO: Add proper define for 0x1E00
		switch (pMod->attribute & 0x1E00) {
		case SCE_MODULE_VSH:
			threadMode = SCE_KERNEL_TH_VSH_MODE;
			break;
		case SCE_MODULE_APP: //0x00004884
			threadMode = SCE_KERNEL_TH_APP_MODE;
			break;
		case SCE_MODULE_USB_WLAN: //0x00004890
			threadMode = SCE_KERNEL_TH_USB_WLAN_MODE;
			break;
		case SCE_MODULE_MS: //0x0000489C
			threadMode = SCE_KERNEL_TH_MS_MODE;
			break;
		default: //0x000048A4
			threadMode = SCE_KERNEL_TH_USER_MODE;
			break;
		}

		SceKernelThreadOptParam threadParams;
		threadParams.size = sizeof(threadParams); //0x000048AC
		threadParams.stackMpid = pMod->mpIdData; //0x000048CC

		status = _CheckUserModulePartition(pMod->mpIdData); // 0x000048BC - 0x000048E0
		if (status < SCE_ERROR_OK)
			threadParams.stackMpid = SCE_KERNEL_PRIMARY_USER_PARTITION;

		pspSetGp(pMod->gpValue); //0x00004900

		pMod->userModThid = sceKernelCreateThread("SceModmgrRebootBefore", pMod->moduleRebootBefore, priority,
			stackSize, threadMode | attr, &threadParams); //0x0000491C

		pspSetGp(oldGp);

		// TODO: Add proper structure for threadArgs
		status = sceKernelStartThread(pMod->userModThid, sizeof threadArgs, threadArgs); //0x00004934
		if (status == SCE_ERROR_OK)
			sceKernelWaitThreadEnd(pMod->userModThid, NULL); //0x000049AC

		sceKernelDeleteThread(pMod->userModThid); //0x00004944
		pMod->userModThid = -1;

		if (!sceKernelIsToolMode()) //0x00004954
			continue;

		status = sceKernelDipsw(25); //0x0000495C
		if (status == 1) //0x00004968
			continue;

		s32 sdkVersion = sceKernelGetCompiledSdkVersion(); //0x00004970
		if (sdkVersion < 0x03030000) //0x00004984
			continue;

		s32 checkSum = sceKernelSegmentChecksum(pMod);
		if (checkSum == pMod->segmentChecksum)
			continue;

		pspBreak(0);
		continue;
	}

	SceSysmemMemoryBlockInfo blkInfo;
	blkInfo.size = sizeof(SceSysmemMemoryBlockInfo);
	status = sceKernelQueryMemoryBlockInfo(uidBlkId, &blkInfo); //0x000047BC
	if (status < SCE_ERROR_OK) //0x000047C4
		return status;

	sceKernelMemset(blkInfo.addr, 0, blkInfo.memSize); //0x000047F0
	status = sceKernelFreePartitionMemory(uidBlkId);

	return status;
}

// Subroutine ModuleMgrForKernel_9B7102E2 - Address 0x000049BC
s32 sceKernelRebootPhaseForKernel(SceSize args, void *argp, s32 arg3, s32 arg4)
{
	SceUID uidBlkId;
	SceUID *uidList;
	SceModule *pMod;
	s32 modCount;
	s32 status;

	uidBlkId = sceKernelGetModuleListWithAlloc(&modCount); //0x000049F8
	if (uidBlkId < SCE_ERROR_OK)
		return uidBlkId;

	uidList = sceKernelGetBlockHeadAddr(uidBlkId); //0x00004A08

	s32 i;
	for (i = modCount - 1; i >= 0; i--) { //0x00004A14 - 0x00004A64
		pMod = sceKernelFindModuleByUID(uidList[i]); //0x00004A34
		if (pMod == NULL || ((s32)pMod->moduleRebootPhase) == -1) //0x00004A3C, 0x00004A48
			continue;

		if (GET_MCB_STATUS(pMod->status) != MCB_STATUS_STARTED || (pMod->status & SCE_MODULE_USER_MODULE)) //0x00004A58 - 0x00004B04
			continue;

		// TODO: Re-define moduleRebootPhase (it currently is a SceKernelThreadEntry)
		pMod->moduleRebootPhase(args, argp, arg3, arg4); //0x00004B0C

		if (!sceKernelIsToolMode()) //0x00004B1C
			continue;

		status = sceKernelDipsw(25); //0x00004B24
		if (status == 1) //0x00004B30
			continue;

		s32 sdkVersion = sceKernelGetCompiledSdkVersion(); //0x00004B38
		if (sdkVersion < 0x03030000) //0x00004B44
			continue;

		s32 checkSum = sceKernelSegmentChecksum(pMod); //0x00004B4C
		if (checkSum == pMod->segmentChecksum) //0x00004B58
			continue;

		pspBreak(0);
		continue;
	}

	status = ClearFreePartitionMemory(uidBlkId); // 0x00004A74 - 0x00004AB4
	return ((status > SCE_ERROR_OK) ? SCE_ERROR_OK : status);
}

// ModuleMgrForKernel_5FC3B3DA - Address 0x00004B6C
s32 sceKernelRebootBeforeForKernel(void *argp, s32 arg2, s32 arg3, s32 arg4)
{
	SceUID uidBlkId;
	SceUID *uidList;
	SceModule *pMod;
	s32 modCount;
	s32 status;

	uidBlkId = sceKernelGetModuleListWithAlloc(&modCount); //0x00004BA8
	if (uidBlkId < SCE_ERROR_OK)
		return uidBlkId;

	uidList = sceKernelGetBlockHeadAddr(uidBlkId); //0x00004BB8

	s32 i;
	for (i = modCount - 1; i >= 0; i--) { //0x00004BC4 - 0x00004C10
		pMod = sceKernelFindModuleByUID(uidList[i]); //0x00004BE4
		if (pMod == NULL || ((s32)pMod->moduleRebootBefore) == -1) //0x00004BEC, 0x00004BF8
			continue;

		if (GET_MCB_STATUS(pMod->status) != MCB_STATUS_STARTED || (pMod->status & SCE_MODULE_USER_MODULE)) //0x00004C08,  0x00004CB0
			continue;

		// TODO: Re-define moduleRebootBefore (it currently is a SceKernelThreadEntry)
		pMod->moduleRebootBefore(argp, arg2, arg3, arg4); //0x00004CB8
	}

	status = ClearFreePartitionMemory(uidBlkId); // 0x00004C24 - 0x00004C60
	return ((status > SCE_ERROR_OK) ? SCE_ERROR_OK : status);
}
