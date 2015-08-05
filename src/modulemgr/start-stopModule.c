/* Copyright (C) 2011 - 2015 The uOFW team
   See the file COPYING for copying permission.
*/

#include <common_imp.h>
#include <modulemgr.h>
#include <modulemgr_kernel.h>
#include <modulemgr_options.h>

#include "loadModuleChecks_inline.h"
#include "modulemgr_int.h"

// Subroutine ModuleMgrForUser_50F0C1EC - Address 0x00003D98 - Aliases: ModuleMgrForKernel_3FF74DF1
s32 sceKernelStartModule(SceUID modId, SceSize args, const void *argp, s32 *pModResult,
	const SceKernelSMOption *pOpt)
{
	s32 oldK1;
	s32 status;
	SceModuleMgrParam modParams;

	oldK1 = pspShiftK1();

	if (sceKernelIsIntrContext()) { //0x00003DA4
		pspSetK1(oldK1);
		return SCE_ERROR_KERNEL_CANNOT_BE_CALLED_FROM_INTERRUPT;
	}

	if (argp != NULL && !pspK1DynBufOk(argp, args)) { //0x00003DE4, 0x00003DFC
		pspSetK1(oldK1);
		return SCE_ERROR_KERNEL_ILLEGAL_ADDR;
	}

	if (pModResult != NULL && !pspK1PtrOk(pModResult)) { //0x00003E18, 0x00003E10
		pspSetK1(oldK1);
		return SCE_ERROR_KERNEL_ILLEGAL_ADDR;
	}

	status = _checkSMOptionConditions(pOpt); //0x00003FB8 - 0x00003EAC
	if (status < 0) {
		pspSetK1(oldK1);
		return status;
	}

	pspClearMemory32(&modParams, sizeof(modParams)); //0x00003EBC

	// 0x00003EC8
	modParams.modeFinish = CMD_START_MODULE;
	modParams.modeStart = CMD_START_MODULE;
	modParams.modId = modId; // 0x00003ECC
	modParams.argSize = args;
	modParams.argp = argp;
	modParams.pStatus = pModResult;

	if (pOpt != NULL) { //0x00003EDC
		modParams.threadMpIdStack = pOpt->mpidstack;
		modParams.stackSize = pOpt->stacksize;
		modParams.threadPriority = pOpt->priority;
		modParams.threadAttr = pOpt->attribute;
	}
	else { //0x00003F14
		modParams.threadMpIdStack = 0;
		modParams.stackSize = 0;
		modParams.threadPriority = 0;
		modParams.threadAttr = 0;
	}
	status = _start_exe_thread(&modParams); //0x00003F04

	pspSetK1(oldK1);
	return status;
}

// Subroutine ModuleMgrForUser_D1FF982A - Address 0x00003F28 - Aliases: ModuleMgrForKernel_E5D6087B
s32 sceKernelStopModule(SceUID modId, SceSize args, const void *argp, s32 *pModResult, const SceKernelSMOption *pOpt)
{
	s32 oldK1;
	s32 status;
	u32 retAddr;
	SceModule *pMod;
	SceModuleMgrParam modParams;

	oldK1 = pspShiftK1();
	retAddr = pspGetRa();

	if (sceKernelIsIntrContext()) { //0x00003F68
		pspSetK1(oldK1);
		return SCE_ERROR_KERNEL_CANNOT_BE_CALLED_FROM_INTERRUPT;
	}

	if (argp != NULL && !pspK1DynBufOk(argp, args)) { //0x00003F7C, 0x00003F94
		pspSetK1(oldK1);
		return SCE_ERROR_KERNEL_ILLEGAL_ADDR;
	}

	if (pModResult != NULL && !pspK1StaBufOk(pModResult, sizeof(*pModResult))) { //0x00003F9C, 0x00003FB0
		pspSetK1(oldK1);
		return SCE_ERROR_KERNEL_ILLEGAL_ADDR;
	}

	status = _checkSMOptionConditions(pOpt); //0x00003FB8 - 0x0000404C
	if (status < 0) {
		pspSetK1(oldK1);
		return status;
	}

	pMod = sceKernelFindModuleByAddress(retAddr); //0x00004054
	if (pMod == NULL) {
		pspSetK1(oldK1);
		return SCE_ERROR_KERNEL_MODULE_CANNOT_STOP;
	}

	pspClearMemory32(&modParams, sizeof(modParams)); //0x00004078

	modParams.pMod = sceKernelGetModuleFromUID(modId); //0x00004080
	if (modParams.pMod && modParams.pMod->attribute & SCE_MODULE_ATTR_CANT_STOP) {
		pspSetK1(oldK1);
		return SCE_ERROR_KERNEL_MODULE_CANNOT_STOP;
	}

	// 0x000040A4
	modParams.modeFinish = CMD_STOP_MODULE;
	modParams.modeStart = CMD_STOP_MODULE;
	modParams.modId = modId; // 0x000040AC
	modParams.callerModId = pMod->modId; // 0x000040BC
	modParams.argSize = args;
	modParams.argp = argp;
	modParams.pStatus = pModResult;

	if (pOpt != NULL) { //0x000040C4
		modParams.threadMpIdStack = pOpt->mpidstack;
		modParams.stackSize = pOpt->stacksize;
		modParams.threadPriority = pOpt->priority;
		modParams.threadAttr = pOpt->attribute;
	}
	else { //0x000040FC
		modParams.threadMpIdStack = 0;
		modParams.stackSize = 0;
		modParams.threadPriority = 0;
		modParams.threadAttr = 0;
	}
	status = _start_exe_thread(&modParams); //0x000040EC

	pspSetK1(oldK1);
	return status;
}

// Subroutine ModuleMgrForUser_D675EBB8 - Address 0x00004110 - Aliases: ModuleMgrForKernel_5805C1CA
s32 sceKernelSelfStopUnloadModule(s32 modStatus, SceSize args, void *argp)
{
	s32 oldK1;
	s32 retAddr;
	s32 status;

	oldK1 = pspShiftK1();
	retAddr = pspGetRa();

	if (sceKernelIsIntrContext()) { //0x0000414C
		pspSetK1(oldK1);
		return SCE_ERROR_KERNEL_CANNOT_BE_CALLED_FROM_INTERRUPT;
	}

	if (argp != NULL && !pspK1DynBufOk(argp, args)) { //0x00004158, 0x0000416C
		pspSetK1(oldK1);
		return SCE_ERROR_KERNEL_ILLEGAL_ADDR;
	}

	if (pspK1IsUserMode()) //0x00004174
		retAddr = sceKernelGetSyscallRA();

	if (!pspK1PtrOk(retAddr)) { //0x0000419C
		pspSetK1(oldK1);
		return SCE_ERROR_KERNEL_ILLEGAL_ADDR;
	}

	status = _SelfStopUnloadModule(modStatus, retAddr, args, argp, NULL, NULL); //0x000041A4

	pspSetK1(oldK1);
	return status;
}

// Subroutine ModuleMgrForUser_2E0911AA - Address 0x00005990 - Aliases: ModuleMgrForKernel_387E3CA9
s32 sceKernelUnloadModule(SceUID modId)
{
	s32 oldK1;
	s32 status;
	SceModuleMgrParam modParams;

	oldK1 = pspShiftK1(); // 0x0000599C

	// Cannot be called in an interruption
	if (sceKernelIsIntrContext()) { // 0x000059A8
		pspSetK1(oldK1);
		return SCE_ERROR_KERNEL_CANNOT_BE_CALLED_FROM_INTERRUPT;
	}

	pspClearMemory32(&modParams, sizeof(modParams)); //0x000059C8

	modParams.modeFinish = CMD_UNLOAD_MODULE; // 0x000059D8
	modParams.modeStart = CMD_UNLOAD_MODULE; // 0x000059DC
	modParams.modId = modId; // 0x000059E0
	modParams.argSize = 0; // 0x000059E4
	modParams.argp = NULL; // 0x000059E8
	modParams.pStatus = NULL; // 0x000059F0

	status = _start_exe_thread(&modParams); // 0x000059EC

	pspSetK1(oldK1);
	return status;
}

// Subroutine ModuleMgrForUser_8F2DF740 - Address 0x00005A14 - Aliases: ModuleMgrForKernel_EE6E8F49
s32 sceKernelStopUnloadSelfModuleWithStatus(s32 exitStatus, SceSize args, void *argp, s32 *pModResult, SceKernelSMOption *pOption)
{
	return _StopUnloadSelfModuleWithStatus(exitStatus, pspGetRa(), args, argp, pModResult, pOption);
}

// Subroutine ModuleMgrForUser_CC1D3699 - Address 0x00005A4C - Aliases: ModuleMgrForKernel_E97E0DB7
s32 sceKernelStopUnloadSelfModule(SceSize args, void *argp, s32 *pModResult, SceKernelSMOption *pOption)
{
	return _StopUnloadSelfModuleWithStatus(SCE_ERROR_OK, pspGetRa(), args, argp, pModResult, pOption);
}
