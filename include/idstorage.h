/* Copyright (C) The uOFW team
   See the file COPYING for copying permission.
*/

#include "common_header.h"

#define SCE_ID_STORAGE_LOOKUP_KEY_BARYON		0x4
#define SCE_ID_STORAGE_LOOKUP_KEY_MDDR			0x6

#define SCE_ID_STORAGE_LEAF_IDENT_SIZE			4

typedef struct {
	u8 ident[SCE_ID_STORAGE_LEAF_IDENT_SIZE]; // 0
	u32 type; // 4
	SceSize payloadLen; // 8
	u32 payloadHash; // 12
	u32 unk16; // 16
	u32 unk20; // 20
	u8 watchDog; // 24
	u8 isWlanSuppressChargingEnabled; // 25
	u16 forceSuspendBatteryCapacity; // 26
	u16 lowBatteryCapacity; // 28
	u8 backlightMaximumWlanActive; // 30
	u8 ledOffTiming; // 31
	u8 tachyonMaxVoltage; // 32
	u8 tachyonDefaultVoltage; // 33
	s16 cpuClockFreqLowerLimit; // 34
	s16 cpuClockFreqUpperLimit; // 36
	u16 cpuClockInitialFrequencyGameAppUpdater; // 38
	s16 busClockFreqLowerLimit; // 40
	s16 busClockFreqUpperLimit; // 42
	u16 busClockInitialFrequencyGameAppUpdater; // 44
	s16 pllClockFreqLowerLimit; // 46
	s16 pllClockFreqUpperLimit; // 48
	u16 pllClockInitialFrequencyGameAppUpdater; // 50
	u8 wlanExclusivePllClockLimit; // 52
	u8 unk53; // 53
	u8 bufRemainData[0x1CA]; // 54 - 511
} SceIdStorageLeafBaryon; // size = 512

typedef struct {
	u8 ident[SCE_ID_STORAGE_LEAF_IDENT_SIZE]; // 0
	u32 type; // 4
	SceSize payloadLen; // 8
	u32 payloadHash; // 12
	u8 unk16; // 16
	u8 unk17; // 17
	u8 unk18; // 18
	u8 ddrDefaultVoltage01g; // 19
	u8 ddrMaxVoltage01g; // 20
	u8 ddrDefaultStrength01g; // 21
	u8 ddrMaxStrength01g; // 22
	u8 ddrDefaultVoltage02gAndLater; // 23
	u8 ddrMaxVoltage02gAndLater; // 24
	u8 ddrDefaultStrength02gAndLater; // 25
	u8 ddrMaxStrength02gAndLater; // 26
	u8 bufRemainData[0x1E5]; // 27 - 511
} SceIdStorageLeafMDdr; // size = 512

s32 sceIdStorageLookup(u16 key, u32 offset, void *pBuf, u32 len);
