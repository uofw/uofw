/* Copyright (C) 2011 - 2015 The uOFW team
   See the file COPYING for copying permission.
*/

#include <common_imp.h>
#include <loadcore.h>
#include <modulemgr.h>
#include <modulemgr_kernel.h>

#include "modulemgr_int.h"

/**
* Find the id of the module from which this function is called
*
* @param modIdList A pointer which will hold the module id list
* @param size Maximum size of the returned buffer
* @param idCount The number of module ids in the list (can be greater than the number of returned ids)
*
* @return SCE_ERROR_OK on success, < 0 on error.
* @return SCE_ERROR_KERNEL_ILLEGAL_ADDR if the provided pointers can't be accessed from the current context or are NULL.
*/
// Subroutine sceKernelGetModuleIdList - Address 0x000041E8 - Aliases: ModuleMgrForKernel_303FAB7F
s32 sceKernelGetModuleIdList(SceUID *modIdList, SceSize size, u32 *idCount)
{
	s32 oldK1;
	s32 retVal;

	oldK1 = pspShiftK1();

	if (modIdList == NULL || idCount == NULL || !pspK1DynBufOk(modIdList, size) || !pspK1StaBufOk(idCount, sizeof(u32))) { // 0x00004200, 0x00004220, 0x00004238, 0x00004244 
		pspSetK1(oldK1);
		return SCE_ERROR_KERNEL_ILLEGAL_ADDR;
	}

	retVal = sceKernelGetModuleIdListForKernel(modIdList, size, idCount, pspK1IsUserMode());

	pspSetK1(oldK1);
	return retVal;
}

/**
* Get module information from id
*
* @param modId The module id
* @param pModInfo Pointer to SceKernelModuleInfo, content will be modified on success with info from the module
*
* @return SCE_ERROR_OK on success, < 0 on error.
* @return SCE_ERROR_KERNEL_CANNOT_BE_CALLED_FROM_INTERRUPT if function was called in an interruption.
* @return SCE_ERROR_KERNEL_ILLEGAL_ADDR if the provided pointer is NULL or can't be accessed from the current context.
* @return SCE_ERROR_KERNEL_ILLEGAL_SIZE if SDK version >= 2.80  and modInfo->size != sizeof(SceKernelModuleInfoV1) && modInfo->size != sizeof(*modInfo)
* @return SCE_ERROR_KERNEL_UNKNOWN_MODULE if module couldn't be found
* @return SCE_ERROR_KERNEL_CANNOT_GET_MODULE_INFO if you don't have the right to access information about this module
*/
// Subroutine sceKernelQueryModuleInfo - Address 0x00004270 - Aliases: ModuleMgrForKernel_22BDBEFF
s32 sceKernelQueryModuleInfo(SceUID modId, SceKernelModuleInfo *pModInfo)
{
	s32 oldK1;
	u32 sdkVersion;
	s32 intrState;
	SceModule *pMod;

	oldK1 = pspShiftK1();

	// Cannot be called from interrupt
	if (sceKernelIsIntrContext()) { // 0x0000429C
		pspSetK1(oldK1);
		return SCE_ERROR_KERNEL_CANNOT_BE_CALLED_FROM_INTERRUPT;
	}

	if (pModInfo == NULL || !pspK1StaBufOk(pModInfo, sizeof(*pModInfo))) { // 0x000042AC, 0x000042BC
		pspSetK1(oldK1);
		return SCE_ERROR_KERNEL_ILLEGAL_ADDR;
	}

	sdkVersion = sceKernelGetCompiledSdkVersion() & 0xFFFF0000; // 0x000042F4

	if (sdkVersion > 0x2070FFFF && pModInfo->size != sizeof(SceKernelModuleInfoV1) // 0x0000430C, 0x00004320
		&& pModInfo->size != sizeof(*pModInfo)) { // 0x00004324
		pspSetK1(oldK1);
		return SCE_ERROR_KERNEL_ILLEGAL_SIZE;
	}

	intrState = sceKernelLoadCoreLock(); // 0x00004334

	pMod = sceKernelFindModuleByUID(modId); // 0x00004340
	if (pMod == NULL) { // 0x00004348
		pspSetK1(oldK1);
		return SCE_ERROR_KERNEL_UNKNOWN_MODULE;
	}

	if (pspK1IsUserMode()) { // 0x00004350
		if (!(pMod->status & SCE_MODULE_USER_MODULE)) {
			pspSetK1(oldK1);
			return SCE_ERROR_KERNEL_CANNOT_GET_MODULE_INFO;
		}

		// TODO: find what 0x1E00 exactly represents
		if ((sceKernelGetUserLevel() == SCE_USER_LEVEL_USBWLAN && pMod->attribute & 0x1E00 != SCE_MODULE_USB_WLAN) // 0x00004368,0x00004370,0x000044C0
			|| (sceKernelGetUserLevel() == SCE_USER_LEVEL_MS && pMod->attribute & 0x1E00 != SCE_MODULE_MS) // 0x0000437C,0x00004388,0x000044A8
			|| (sceKernelGetUserLevel() == SCE_USER_LEVEL_APP && pMod->attribute & 0x1E00 != SCE_MODULE_APP) // 0x00004390,0x0000439C,0x00004490
			&& (pMod->attribute & 0x1E00) // 0x000043A8
			) {

			sceKernelLoadCoreUnlock(intrState);
			pspSetK1(oldK1);
			return SCE_ERROR_KERNEL_CANNOT_GET_MODULE_INFO;
		}
	}

	s32 modStatus = GET_MCB_STATUS(pMod->status);
	if (modStatus == MCB_STATUS_NOT_LOADED || modStatus == MCB_STATUS_LOADING || modStatus == MCB_STATUS_LOADED
		|| modStatus == MCB_STATUS_UNLOADED) { // 0x000043C0
		sceKernelLoadCoreUnlock(intrState);
		pspSetK1(oldK1);
		return SCE_ERROR_KERNEL_CANNOT_GET_MODULE_INFO;
	}

	pModInfo->nsegment = pMod->nSegments; // 0x000043C8

	for (int i = 0; i < SCE_KERNEL_MAX_MODULE_SEGMENT; i++) { // 0x000043F0
		pModInfo->segmentAddr[i] = pMod->segmentAddr[i]; // 0x000043E4
		pModInfo->segmentSize[i] = pMod->segmentSize[i]; // 0x000043F4
	}

	pModInfo->entryAddr = pMod->entryAddr; // 0x00004404
	pModInfo->gpValue = pMod->gpValue; // 0x0000440C
	pModInfo->textAddr = pMod->textAddr; // 0x00004414
	pModInfo->textSize = pMod->textSize; // 0x0000441C
	pModInfo->dataSize = pMod->dataSize; // 0x00004424
	pModInfo->bssSize = pMod->bssSize; // 0x00004430

	// If we have a v1 structure (less fields, size: 64)
	if (pModInfo->size != sizeof(SceKernelModuleInfo)) {
		sceKernelLoadCoreUnlock(intrState);
		pspSetK1(oldK1);
		return SCE_ERROR_OK;
	}

	// If we have a v2 structure (more fields, size: 96)
	// TODO: find what 0x1E00 exactly represents
	pModInfo->attribute = pMod->attribute & ~0x1E00;
	pModInfo->version[MODULE_VERSION_MINOR] = pMod->version[MODULE_VERSION_MINOR];
	pModInfo->version[MODULE_VERSION_MAJOR] = pMod->version[MODULE_VERSION_MAJOR];
	strncpy(pModInfo->modName, pModInfo->modName, SCE_MODULE_NAME_LEN);
	pModInfo->terminal = pMod->terminal;

	sceKernelLoadCoreUnlock(intrState);
	pspSetK1(oldK1);
	return SCE_ERROR_OK;
}

/**
* Find the id of the module from which this function is called
*
* @return The module id on success, < 0 on error.
* @return SCE_ERROR_KERNEL_CANNOT_BE_CALLED_FROM_INTERRUPT if function was called in an interruption.
* @return SCE_ERROR_KERNEL_ILLEGAL_ADDR if the provided pointer can't be accessed from the current context.
* @return SCE_ERROR_KERNEL_ERROR if module couldn't be found.
*/
// Subroutine ModuleMgrForUser_F0A26395 - Address 0x000058F8 - Aliases: ModuleMgrForKernel_CECA0FFC
s32 sceKernelGetModuleId(void)
{
	s32 oldK1;
	s32 retAddr;
	s32 intrState;
	s32 retVal;
	SceModule *pMod;

	oldK1 = pspShiftK1();
	retAddr = pspGetRa();

	if (sceKernelIsIntrContext()) { //0x0000450C
		pspSetK1(oldK1);
		return SCE_ERROR_KERNEL_CANNOT_BE_CALLED_FROM_INTERRUPT;
	}

	if (pspK1IsUserMode()) //0x00004520
		retAddr = sceKernelGetSyscallRA();

	if (!pspK1PtrOk(retAddr)) { //0x0000452C
		pspSetK1(oldK1);
		return SCE_ERROR_KERNEL_ILLEGAL_ADDR;
	}

	intrState = sceKernelLoadCoreLock();

	pMod = sceKernelFindModuleByAddress(retAddr); //0x00004544
	if (pMod == NULL)
		retVal = SCE_ERROR_KERNEL_ERROR;
	else
		retVal = pMod->modId;

	sceKernelLoadCoreUnlock(intrState); //0x0000455C

	pspSetK1(oldK1);
	return retVal;
}

/**
* Find the id of the module whose codeAddr belongs to
*
* @param codeAddr An address inside the module
*
* @return The module id on success, < 0 on error.
* @return SCE_ERROR_KERNEL_CANNOT_BE_CALLED_FROM_INTERRUPT if function was called in an interruption.
* @return SCE_ERROR_KERNEL_ILLEGAL_ADDR if the provided pointer can't be accessed from the current context.
* @return SCE_ERROR_KERNEL_UNKNOWN_MODULE if module couldn't be found.
*/
// Subroutine sceKernelGetModuleIdByAddress - Address 0x00004598 - Aliases: ModuleMgrForKernel_433D5287
s32 sceKernelGetModuleIdByAddress(const void *codeAddr)
{
	s32 oldK1;
	s32 intrState;
	s32 retVal;
	SceModule *pMod;

	oldK1 = pspShiftK1();

	if (sceKernelIsIntrContext()) { // 0x000045B4
		pspSetK1(oldK1);
		return SCE_ERROR_KERNEL_CANNOT_BE_CALLED_FROM_INTERRUPT;
	}

	if (!pspK1PtrOk(codeAddr)) { // 0x000045D0
		pspSetK1(oldK1);
		return SCE_ERROR_KERNEL_ILLEGAL_ADDR;
	}

	intrState = sceKernelLoadCoreLock(); // 0x000045D8

	pMod = sceKernelFindModuleByAddress(codeAddr); // 0x000045E4
	if (pMod == NULL)
		retVal = SCE_ERROR_KERNEL_UNKNOWN_MODULE;
	else
		retVal = pMod->modId;

	sceKernelLoadCoreUnlock(intrState); // 0x00004600

	pspSetK1(oldK1);
	return retVal;
}

/**
* Find the offset from the start of the TEXT segment of the module whose codeAddr belongs to
*
* @param codeAddr An address inside the module
* @param pGP A pointer to a location where the GP offset will be stored
*
* @return SCE_ERROR_OK and sets pGP on success, < 0 on error.
* @return SCE_ERROR_KERNEL_ILLEGAL_ADDR if the provided pointer can't be accessed from the current context.
* @return SCE_ERROR_KERNEL_UNKNOWN_MODULE if module couldn't be found.
*/
// Subroutine sceKernelGetModuleGPByAddress - Address 0x00004628 
s32 sceKernelGetModuleGPByAddress(const void *codeAddr, u32 *pGP)
{
	s32 oldK1;
	s32 intrState;
	s32 retVal;
	SceModule *pMod;

	oldK1 = pspShiftK1(); // 0x0000463C

	if (!pspK1PtrOk(codeAddr) || pGP == NULL || !pspK1PtrOk(pGP)) { // 0x00004660, 0x00004670, 0x0000467C
		pspSetK1(oldK1);
		return SCE_ERROR_KERNEL_ILLEGAL_ADDR;
	}

	intrState = sceKernelLoadCoreLock(); // 0x00004684

	pMod = sceKernelFindModuleByAddress(codeAddr); // 0x00004698
	if (pMod == NULL) // 0x000046A0
		retVal = SCE_ERROR_KERNEL_UNKNOWN_MODULE; // 0x0000469C
	else {
		retVal = SCE_ERROR_OK; // 0x000046AC
		*pGP = pMod->gpValue; // 0x000046B0
	}

	sceKernelLoadCoreUnlock(intrState); // 0x000046B4

	pspSetK1(oldK1);
	return retVal;
}

// Subroutine ModuleMgrForKernel_D86DD11B - Address 0x00005A80
s32 sceKernelSearchModuleByName(const char *name)
{
	s32 oldK1;
	SceModule *pMod;

	oldK1 = pspShiftK1();

	if (!pspK1PtrOk(name)) {
		pspSetK1(oldK1);
		return SCE_ERROR_KERNEL_ILLEGAL_ADDR;
	}

	pMod = sceKernelFindModuleByName(name); // 0x00005AA8

	pspSetK1(oldK1);
	return (pMod != NULL) ? pMod->modId : SCE_ERROR_KERNEL_UNKNOWN_MODULE;
}

// Subroutine ModuleMgrForKernel_12F99392 - Address 0x00005AE0
s32 sceKernelSearchModuleByAddress(u32 addr)
{
	SceModule *pMod;

	pMod = sceKernelFindModuleByAddress(addr); // 0x00005AE8

	return (pMod != NULL) ? pMod->modId : SCE_ERROR_KERNEL_UNKNOWN_MODULE;
}