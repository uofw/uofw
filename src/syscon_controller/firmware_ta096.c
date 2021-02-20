/* Copyright (C) The uOFW team
   See the file COPYING for copying permission.
*/

/*
 *
 *          SYSCON controller firmware
 * 
 *              Motherboard TA-096
 *
 *       Renesas 78K0/KE2 - model D78F0534
 *
 */

/*
 *
 * Credits to:
 *	- Proxima for a set of function names 
 *	- [TODO] for dumping the SYSCON controller firmware 
 * 
 */

#include <common_imp.h>
#include <syscon.h>

#include "sfr.h"

/*
 * This constant specifies the base length of the data to be transmitted back to the SYSCON module.
 * This base length represents the first three members in the ::SceSysconPacket.tx[16] data buffer, which
 * are always sent back to the SYSCON module.
 */
#define SYSCON_CMD_TRANSMIT_DATA_BASE_LEN	3

const u32 g_baryonVersion = 0x00403000; // 0x010E

#define SYSCON_TIMESTAMP_DATE_PREFIX_LEN	8
#define SYSCON_TIMESTAMP_LEN				12

const u8 g_buildDate[] = "$Date:: 2011-05-09 20:45:25 +0900#$"; // 0x0112

void (*g_sysconCmdGetOps[])(void) = {
	exec_syscon_cmd_nop,
	exec_syscon_cmd_get_baryon_version,
	exec_syscon_cmd_get_digital_key,
	exec_syscon_cmd_get_analog,
	exec_syscon_cmd_nop,
	exec_syscon_cmd_get_tachyon_temp,
	exec_syscon_cmd_get_digital_key_analog,
	exec_syscon_cmd_get_kernel_digital_key,
	exec_syscon_cmd_get_kernel_digital_key_analog,
	exec_syscon_cmd_read_clock,
	exec_syscon_cmd_read_alarm,
	exec_syscon_cmd_get_power_supply_status,
	exec_syscon_cmd_get_tachyon_wdt_status,
	exec_syscon_cmd_get_batt_volt,
	exec_syscon_cmd_get_wake_up_factor,
	exec_syscon_cmd_get_wake_up_factor,
	exec_syscon_cmd_get_wake_up_req,
	exec_syscon_cmd_get_baryon_status_2,
	exec_syscon_cmd_get_timestamp,
	exec_syscon_cmd_get_video_cable
}; // 0x008A -- SIO10 communication

u16 g_wakeUpFactor; // 0xFC79
u8 g_watchdogTimerStatus; // 0xFC80

u8 g_ctrlAnalogDataX; // 0xFD16 
u8 g_ctrlAnalogDataY; // 0xFD17

/* Base value for the battery voltage on PSP series PSP-N1000 and PSP-E1000. */
u8 g_battVoltBase; // 0xFD46

u8 g_unkFE32; // 0xFE32
u16 g_unkFE3A; // 0xFE34

u32 g_clock; // 0xFE3C
u32 g_alarm; // 0xFE40

u8 g_unkFE45; // 0xFE45
u8 g_unkFE46; // 0xFE46

u8 g_unkFE4E; // 0xFE4E

u8 g_transmitData[12]; // 0xFE50 -- TODO: This could be a bigger size (up until size 13)

u8 g_curSysconCmdId; // 0xFE6E
u8 g_sysconCmdTransmitDataLength; // 0xFE71

u8 g_powerSupplyStatus; // 0xFE7A

u8 g_unkFE98; // 0xFE98

u8 g_baryonStatus2; // 0xFED2
u8 g_unkFED3; // 0xFED3

/* functions */

// sub_0660
void RESET(void)
{
}

// sub_06D6
void rotate_and_swap()
{
}

// sub_0725
void xor_bytes()
{
}

// sub_073A
void sub_073A(void)
{
	return;
}

// sub_073B
void sub_073B()
{
}

// sub_075D
void sub_075D()
{
}

// sub_075F
void main(void)
{

}

// sub_0818
void init_devices_1()
{
}

// sub_0932
void sub_0932(void)
{
}

// sub_0974
void sub_0974(void)
{
}

// sub_987
void sub_0987(void)
{
}

// sub_099A
void sub_099A()
{
}

//sub_09E1
void init_allegrex(void)
{
}

//sub_0A21
void sub_0A21(void)
{
}

// sub_0A5E
void sub_0A5E(void)
{
}

// sub_0A97
void sub_0A97(void)
{
}

// sub_0AB9
void start_allegrex_reset(void)
{
}

// sub_0AF1
void init_allegrex_3(void)
{
}

// sub_0B58
void sub_0B58(void)
{
}

// sub_0B5B
void sub_0B5B(void)
{
}

// sub_0B73
void poll_intrerfaces(void)
{
}

// sub_10A2
void sub_10A2(void)
{
}

// sub_10AC
void INTWT(void)
{
}

// sub_1272
void INTWTI(void)
{
}

// sub_1458
void INTTM51(void)
{
}

// sub_146D
void INTTMH0(void)
{
}

// sub_14A0
void INTPO(void)
{
}

// sub_14C2
void INTP1(void)
{
}

// sub_14DA
void INTP2(void)
{
}

// sub_14F5
void INTP5(void)
{
}

// sub_1502
void sub_1502(void)
{
}

// sub_1527
void sub_1527(void)
{
}

// sub_1536
/* Interrupt handler for serial interface CSI10. Called when a CSI10 communication has ended. */
void INTCSI10(void)
{
}

/* SYSCON [get] commands */

// sub_1921
void exec_syscon_cmd_nop(void)
{
	g_sysconCmdTransmitDataLength = SYSCON_CMD_TRANSMIT_DATA_BASE_LEN;
	g_curSysconCmdId = PSP_SYSCON_CMD_NOP;
}

// sub_1928
void exec_syscon_cmd_get_baryon_version(void)
{
	g_sysconCmdTransmitDataLength = SYSCON_CMD_TRANSMIT_DATA_BASE_LEN + 4;
	g_curSysconCmdId = PSP_SYSCON_CMD_GET_BARYON_VERSION;

	((u32 *)g_transmitData)[0] = g_baryonVersion;
}

// sub_1939
void transmit_data_set_digital_user_key_data(void)
{
	// PORT 7 seems to contain state values for the following buttons:
	//		UP, DOWN, LEFT, RIGHT
	//		TRIANGLE, CIRCLE, CROSS, SQUARE
	g_transmitData[0] = GET_PORT_VAL(7);


	// Port 4 seems to contain state values for the following buttons:
	//	SELECT, START
	//	L, R
	//
	// Port 2 seems to contain state values for the following buttons:
	//	SCE_CTRL_INTERCEPTED
	//  SCE_CTRL_HOLD
	u8 ctrlData2 = ((GET_PORT_VAL(2) << 4) & 0x30) | (GET_PORT_VAL(4) & 0xF);
	ctrlData2 |= 0x40; // Set 7th bit -- SCE_CTRL_WLAN_UP?
	ctrlData2 &= ~0x80; // clear 8th bit -- SCE_CTRL_REMOTE?

	g_transmitData[1] = ctrlData2;
}

// sub_1953
void transmit_data_set_digital_kernel_key_data(void)
{
	// g_transmitData[2] = buttons 0x0BF00000
	// VolUp, VolDown, Screen, Note (0xF)
	// Disc, MS, 0x08000000 (unknown) - 0xB

	g_transmitData[2] = GET_PORT_VAL(5) | 0xFC; // 1957

	//u8 isDiscPresent = (((g_unkFE45 >> 2) & 0x1) ^ 0xFF) & 0x1;
	u8 isDiscPresent = !((g_unkFE45 >> 2) & 0x1);

	// set 5th bit
	g_transmitData[2] &= ~0x10;
	g_transmitData[2] |= (isDiscPresent << 4); // 0x1962

	if (!(g_unkFE3A & 0x8)) // 0x1965
	{
		// clear 6th bit -- Memory stick not present
		g_transmitData[2] &= ~0x20; // 0x1968
	}

	// set 8th bit
	g_transmitData[2] &= ~0x80;
	g_transmitData[2] |= (g_unkFE46 >> 2) & 0x1; // 196A

	// unknown controller bits below
	//
	// g_transmitData[3] = buttons & 0x30000000
	// unknown 0x10000000, 0x20000000

	g_transmitData[3] = 0xFF; // 0x1970

	// set 1st bit
	g_transmitData[3] &= 0xFE;
	g_transmitData[3] |= (g_unkFE98 >> 2) & 0x1; // 0x1973 - 0x1976
}

// sub_197A
void exec_syscon_cmd_get_digital_key(void)
{
	g_sysconCmdTransmitDataLength = SYSCON_CMD_TRANSMIT_DATA_BASE_LEN + 2;
	g_curSysconCmdId = PSP_SYSCON_CMD_GET_DIGITAL_KEY;

	transmit_data_set_digital_user_key_data();
}

// sub_1984
void exec_syscon_cmd_get_analog(void)
{
	g_sysconCmdTransmitDataLength = SYSCON_CMD_TRANSMIT_DATA_BASE_LEN + 2;
	g_curSysconCmdId = PSP_SYSCON_CMD_GET_ANALOG;

	g_transmitData[0] = g_ctrlAnalogDataX;
	g_transmitData[1] = g_ctrlAnalogDataY;
}

// sub_1995
void exec_syscon_cmd_get_tachyon_temp(void)
{
	g_sysconCmdTransmitDataLength = SYSCON_CMD_TRANSMIT_DATA_BASE_LEN + 2;
	g_curSysconCmdId = PSP_SYSCON_CMD_GET_TACHYON_TEMP;

	/* This command has been retired in later firmwares. */
	g_transmitData[0] = 0;
	g_transmitData[1] = 0;
}

// sub_19A0
void exec_syscon_cmd_get_digital_key_analog(void)
{
	g_sysconCmdTransmitDataLength = SYSCON_CMD_TRANSMIT_DATA_BASE_LEN + 4;
	g_curSysconCmdId = PSP_SYSCON_CMD_GET_DIGITAL_KEY_ANALOG;

	transmit_data_set_digital_user_key_data();

	g_transmitData[2] = g_ctrlAnalogDataX;
	g_transmitData[3] = g_ctrlAnalogDataY;
}

// sub_19B4
void exec_syscon_cmd_get_kernel_digital_key(void)
{
	g_sysconCmdTransmitDataLength = SYSCON_CMD_TRANSMIT_DATA_BASE_LEN + 4;
	g_curSysconCmdId = PSP_SYSCON_CMD_GET_KERNEL_DIGITAL_KEY;

	transmit_data_set_digital_user_key_data();
	transmit_data_set_digital_kernel_key_data();
}

// sub_19C1
void exec_syscon_cmd_get_kernel_digital_key_analog(void)
{
	g_sysconCmdTransmitDataLength = SYSCON_CMD_TRANSMIT_DATA_BASE_LEN + 6;
	g_curSysconCmdId = PSP_SYSCON_CMD_GET_KERNEL_DIGITAL_KEY_ANALOG;

	transmit_data_set_digital_user_key_data();
	transmit_data_set_digital_kernel_key_data();

	g_transmitData[4] = g_ctrlAnalogDataX;
	g_transmitData[5] = g_ctrlAnalogDataY;
}

// sub_19D8
void exec_syscon_cmd_read_clock(void)
{
	g_sysconCmdTransmitDataLength = SYSCON_CMD_TRANSMIT_DATA_BASE_LEN + 4;
	g_curSysconCmdId = PSP_SYSCON_CMD_READ_CLOCK;

	((u32 *)g_transmitData)[0] = g_clock;
}

// sub_19E7
void exec_syscon_cmd_read_alarm(void)
{
	g_sysconCmdTransmitDataLength = SYSCON_CMD_TRANSMIT_DATA_BASE_LEN + 4;
	g_curSysconCmdId = PSP_SYSCON_CMD_READ_ALARM;

	((u32 *)g_transmitData)[0] = g_alarm;
}

// sub_19F6
void exec_syscon_cmd_get_power_supply_status(void)
{
	g_sysconCmdTransmitDataLength = SYSCON_CMD_TRANSMIT_DATA_BASE_LEN + 1;
	g_curSysconCmdId = PSP_SYSCON_CMD_GET_POWER_SUPPLY_STATUS;

	g_transmitData[0] = g_powerSupplyStatus;
}

// sub_1A01
void exec_syscon_cmd_get_tachyon_wdt_status(void)
{
	g_sysconCmdTransmitDataLength = SYSCON_CMD_TRANSMIT_DATA_BASE_LEN + 1;
	g_curSysconCmdId = PSP_SYSCON_CMD_GET_TACHYON_WDT_STATUS;

	g_transmitData[0] = g_watchdogTimerStatus;
}

// sub_1A0D
void exec_syscon_cmd_get_batt_volt(void)
{
	g_sysconCmdTransmitDataLength = SYSCON_CMD_TRANSMIT_DATA_BASE_LEN + 1;
	g_curSysconCmdId = PSP_SYSCON_CMD_GET_BATT_VOLT;

	g_transmitData[0] = g_battVoltBase;
}

// sub_1A19
void exec_syscon_cmd_get_wake_up_factor(void)
{
	g_sysconCmdTransmitDataLength = SYSCON_CMD_TRANSMIT_DATA_BASE_LEN + 2;
	g_curSysconCmdId = PSP_SYSCON_CMD_GET_WAKE_UP_FACTOR;

	((u16 *)g_transmitData)[0] = g_wakeUpFactor;
}

// sub_1A2A
void exec_syscon_cmd_get_wake_up_req(void)
{
	g_sysconCmdTransmitDataLength = SYSCON_CMD_TRANSMIT_DATA_BASE_LEN + 1;
	g_curSysconCmdId = PSP_SYSCON_CMD_GET_WAKE_UP_REQ;

	/* This command appears to be retired now. */
	g_transmitData[0] = -1;
}

// sub_1A34
void exec_syscon_cmd_get_baryon_status_2(void)
{
	g_sysconCmdTransmitDataLength = SYSCON_CMD_TRANSMIT_DATA_BASE_LEN + 1;
	g_curSysconCmdId = PSP_SYSCON_CMD_GET_STATUS2;

	g_transmitData[0] = g_baryonStatus2; // 0x1A3C

	// Clear bits (TODO: probably some status bits)
	g_unkFE32 &= ~0x20; // 1A3E
	g_unkFED3 &= ~0x4; // 1A40
}

// sub_1A43
void exec_syscon_cmd_get_timestamp(void)
{
	g_sysconCmdTransmitDataLength = SYSCON_CMD_TRANSMIT_DATA_BASE_LEN + SYSCON_TIMESTAMP_LEN;
	g_curSysconCmdId = PSP_SYSCON_CMD_GET_TIMESTAMP;

	/*
	 * Obtain the timestamp from the build date. We only take the date and hour/minute of the day.
	 * I.e. given a build date string "$Date:: 2011-05-09 20:45:25 +0900#$", we return the string
	 * "201105092045".
	 */

	u8 i = 0;
	u8 curDateIndex = SYSCON_TIMESTAMP_DATE_PREFIX_LEN;
	while (i < SYSCON_TIMESTAMP_LEN) // 0x1A51
	{
		/* Check if we are looking at a numerical character. */
		if (g_buildDate[curDateIndex] < '0' || g_buildDate[curDateIndex] > '9') // 0x1A53 - 0x1A6D 
		{
			/* Skip any non-numerical characters. */
			curDateIndex++; // 0x1A83
			continue;
		}
		
		/* Save the numerical character in our transmit buffer. */
		g_transmitData[i++] = g_buildDate[curDateIndex++];
	}
}

// sub_1A88
void exec_syscon_cmd_get_video_cable(void)
{
	g_sysconCmdTransmitDataLength = SYSCON_CMD_TRANSMIT_DATA_BASE_LEN + 1;
	g_curSysconCmdId = PSP_SYSCON_CMD_GET_VIDEO_CABLE;

	/* No video cable support for TA-096. */
	g_transmitData[0] = 0;
}

// sub_1A92
void sub_1A92(void)
{
	g_unkFE4E = 0x84;
}

/* SYSCON [set] commands */

// TODO: More functions here

// sub_1D34
void response_manager(void)
{

}

// sub_1FD2
void sub_1FD2(void)
{

}

// sub_1FF7
void sub_1FF7()
{

}

// TODO: more functions here...

// sub_2654
void response_handler()
{

}

// TODO: more functions here...

// sub_5011
void memcpy(const void *pSrc, void *pDst, u16 n)
{
	while (n-- != 0)
	{
		*(u8 *)(pDst + n) = *(u8 *)(pSrc + n);
	}
}

// sub_5039
// A modified memcpy, where a return value of 0 means s1 == s2 and a return value of -1 means s1 != s2.
u8 memcmp(const void *s1, const void *s2, u16 n)
{
	u8 cmpResult = 0; // 0x5041
	while (n-- != 0)
	{
		if (*(u8 *)(s1 + n) == *(u8 *)(s2 + n)) // 0x5065
		{
			continue;
		}

		cmpResult = -1; // 0x5069
	}

	return cmpResult;
}

// sub_5076
// Pairwise XOR's the first 10 bytes of s1 with s2 and overwrites the first 10 bytes s2 with the resulting bytes
void xorloop_0x10(const void *s1, void *s2)
{
	for (int i = 0xF; i >= 0; i--)
	{
		*(u8 *)(s2 + i) ^= *(u8 *)(s1 + i); // 0x50A0
	}
}

// sub_50A7
void memset(void *s, int c, u16 n)
{
	// 0x50AC - 0x50BF
	while (n-- != 0)
	{
		*(u8 *)(s + n) = (u8)c;
	}
}

// TODO: more functions here...


