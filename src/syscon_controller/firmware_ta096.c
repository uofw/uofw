/* Copyright (C) The uOFW team
   See the file COPYING for copying permission.
*/

/*
 *
 *          SYSCON controller firmware
 * 
 *  Motherboard TA-096 - PSP Street (E-1000) - 11g
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
#include <ctrl.h>
#include <syscon.h>

#include "sfr.h"

#define SYSCON_STACK_LOWER_BOUND	0xFE20
#define SYSCON_STACK_UPPER_BOUND	0xFEDF

void sub_1070(void);
void sub_10A2(void);

/*
 * This constant specifies the base length of the data to be transmitted back to the SYSCON module.
 * This base length represents the first three members in the ::SceSysconPacket.tx[16] data buffer, which
 * are always sent back to the SYSCON module.
 */
#define SYSCON_CMD_TRANSMIT_DATA_BASE_LEN	3

/* Constants */

const u8 g_analogPadStartPositionY = SCE_CTRL_ANALOG_PAD_CENTER_VALUE; // 0x86
const u8 g_analogPadStartPositionX = SCE_CTRL_ANALOG_PAD_CENTER_VALUE; // 0x87

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

#define MAIN_OPERATIONS_ID_START		PSP_SYSCON_CMD_WRITE_CLOCK

void (*g_mainOperations[])(void) = {
	write_clock,
	set_usb_status,
	write_alarm,
	write_scratchpad,
	read_scratchpad,
	send_setparam,
	receive_setparam,
	main_op_nop,
	main_op_nop,
	main_op_nop,
	main_op_nop,
	main_op_nop,
	main_op_nop,
	main_op_nop,
	main_op_nop,
	main_op_nop,
	exec_syscon_cmd_0x30,
	ctrl_tachyon_wdt,
	reset_device,
	ctrl_analog_xy_polling,
	main_op_nop,
	power_standby,
	power_suspend,
	get_batt_volt_ad
}; // 0x00B0

u8 g_unkF400[0x1D0]; // 0xF400

/* SYSCON's internal scratchpad buffer size. */
#define SCRATCH_PAD_SIZE	32
/* SYSCON's internal scratchpad buffer. */
u8 g_scratchpad[SCRATCH_PAD_SIZE]; // 0xF7B0

typedef struct {
	u8 data[SCE_SYSCON_SET_PARAM_PAYLOAD_SIZE];
} SysconParamIdPayload;

#define SYSCON_SET_PARAM_NUM_SET_PARAMS		5
#define SYSCON_SET_PARAM_MAX_SET_PARAM_ID	(SYSCON_SET_PARAM_NUM_SET_PARAMS - 1)

SysconParamIdPayload g_setParamPayloads[SYSCON_SET_PARAM_NUM_SET_PARAMS]; // 0xF7D0

u16 g_unkFC40; // 0xFC40
u16 g_unkFC42; // 0xFC42
u16 g_unkFC44; // 0xFC44

u16 g_unkFC54; // 0xFC54
u16 *g_unkFC56; // 0xFC56
u16 g_unkFC58; // 0xFC58

u8 g_unkFC78; // 0xFC78

u16 g_wakeUpFactor; // 0xFC79

/*
 * This constant indicates that the watch dog timer is not currently counting. For example, this is the case
 * when the timer counter has counted downwards until it reached the value 0, at which point there is nothing
 * left to count. This can indicate that the watchdog timer counter could not be properly resetted.
 */
#define WATCHDOG_TIMER_STATUS_NOT_COUNTING		0
#define WATCHDOG_TIMER_STATUS_COUNTING			1
u8 g_watchdogTimerStatus; // 0xFC80
u8 g_watchdogTimerCounter; // 0xFC81
u8 g_watchdogTimerCounterResetValue; // 0xFC82
u8 g_unkFC83; // 0xFC83 -- TODO: Seems related to the watchdogTimer

u8 g_usbStatus; // 0xFC86

u8 g_mainOperationPayloadReceiveBuffer[9]; // 0xFCB0 -- Could be larger....

/*
 * The length of a command package sent to the SYSCON controller. A command package consists of
 * the command ID, the command package length and the actual command data.
 */
u8 g_mainOperationReceivedPackageLength; // 0xFCCB

u8 g_ctrlAnalogDataX; // 0xFD16 
u8 g_ctrlAnalogDataY; // 0xFD17

u8 g_battVoltADUnkVal; // 0xFD22

u8 g_unkFD29; // 0xFD29

/* Base value for the battery voltage on PSP series PSP-N1000 and PSP-E1000. */
u8 g_battVoltBase; // 0xFD46
u8 g_unkFD47; //0xFD47
u8 g_unkFD49; // 0xFD49

/* 0 = analog pad at bottom limit, 0xFF = analog pad at top limit. */
u8 g_curAnalogPadPositionY; // 0xFD4A

/* 0 = analog pad at right limit, 0xFF = analog pad at left limit. */
u8 g_curAnalogPadPositionX; // 0xFD4B

/* sreg variables below */

u8 g_unkFE30; // 0xFE30
u8 g_unkFE31; // 0xFE31
u8 g_unkFE32; // 0xFE32
u8 g_isMainOperationRequestExist; // 0xFE33

u8 g_unkFE35; // 0xFE35 -- TODO: Bit 1 is set when the watchdog timer counter has reached value [0] (= finished counting)

u16 g_unkFE3A; // 0xFE3A

u32 g_clock; // 0xFE3C
u32 g_alarm; // 0xFE40

u8 g_unkFE45; // 0xFE45
u8 g_unkFE46; // 0xFE46

u8 g_mainOperationId; // 0xFE4C

#define MAIN_OPERATION_RESULT_STATUS_UNKNOWN_0x80		0x80
#define MAIN_OPERATION_RESULT_STATUS_UNKNOWN_0x81		0x81 // TODO: perhaps a [retry] indicator?
#define MAIN_OPERATION_RESULT_STATUS_SUCCESS			0x82
#define MAIN_OPERATION_RESULT_STATUS_ERROR				0x83
#define MAIN_OPERATION_RESULT_STATUS_NOT_IMPLEMENTED	0x84
#define MAIN_OPERATION_RESULT_STATUS_INVALID_ID			0x87
u8 g_mainOperationResultStatus; // 0xFE4E

u8 g_unkFE4F; // 0xFE4F -- TODO: Seems similar to g_mainOperationResultStatus

u8 g_transmitData[12]; // 0xFE50
u8 g_mainOperationPayloadTransmitBuffer[12]; // 0xFE5C

u8 g_curSysconCmdId; // 0xFE6E
u8 g_sysconCmdTransmitDataLength; // 0xFE71

u8 g_mainOperationTransmitPackageLength; // 0xFE72

u16 g_unkFE76; // 0xFE76 -- TODO: Might be a flag indicating what needs to be updated (i.e. new USB status set).

u16 g_unkFE78; // 0xFE78

u8 g_powerSupplyStatus; // 0xFE7A

#define SYSCON_CTRL_ANALOG_SAMPLING_ENABLED		0x02
u8 g_unkFE96; // 0xFE96
u8 g_unkFE98; // 0xFE98

u16 g_setParam1PayloadData1; // 0xFEAE
u16 g_setParam1PayloadData0; // 0xFEB0
u16 g_setParam1PayloadData3; // 0xFEB2
u16 g_setParam1PayloadData2; // 0xFEB4

u8 g_unkFEB6; // 0xFEB6

u8 g_unkFECF; // 0xFECF

u8 g_baryonStatus2; // 0xFED2
u8 g_unkFED3; // 0xFED3
/* functions */

void RESET(void) __attribute__((noreturn));

// sub_0660
/*
 * Run when a [reset] signal is generated. A [reset] signal is generated, for example, when
 *		- the device is turned on
 *		- the watchdog timer detected a a loop (counter wasn't resetted)
 * 
 * Initializes state such as the stack pointer (and clears the stack area), the current analog pad data
 * and clears global variables (so that we start fresh).
 */
void RESET(void)
{
	/* Set the register bank RB0 (0xFEF8 - 0xFEFF) as the work register set. */
	// SEL RB0

	/* Set the Stack pointer. */
	// MOVW SP SYSCON_STACK_LOWER_BOUND

	hdwinit();

	g_unkFC54 = 0;
	g_unkFC40 = 0;
	g_unkFC44 = 0;
	g_unkFC42 = 1;
	g_unkFC56 = &g_unkFC58;

	/* Clear stack content. */
	u8 *pStackLower = (u8 *)SYSCON_STACK_LOWER_BOUND;
	while (*pStackLower != (u8 *)(SYSCON_STACK_UPPER_BOUND + 1))
	{
		*pStackLower++ = 0;
	}

	/* ROM data copy: copy external variables having initial value. */

	/* Set default analog pad data. */

	// 0x0689 - 0x0699
	g_curAnalogPadPositionY = g_analogPadStartPositionY;
	g_curAnalogPadPositionX = g_analogPadStartPositionX;

	/* Initialize external variables which don't have initial values (initialize to 0) */

	u8 *pStartAddr = &g_unkFC78;
	u8 *pEndAddr = &g_curAnalogPadPositionY;
	while (pStartAddr != pEndAddr) // 0x069B - 0x06A8
	{
		*pStartAddr = 0;
	}

	/* ROM copy: Copy sreg variables which have initial values. */

	// Note: Logic from 0x6AA - 0x6BC does not do anything, hence omitted here
	// (no sreg vars with initial values)

	/* Initialize sreg variables which don't have initial values (initialize to 0) */

	pStartAddr = &g_unkFE30;
	pEndAddr = &g_unkFEB6;
	while (pStartAddr != pEndAddr) // 0x06BC - 0x06C9
	{
		*pStartAddr = 0;
	}

	/* Call our [main] function now that we have set up the SYSCON state. */
	main();

	/* Cleanup if necessary. */
	exit(0);

	for (;;);
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
void hdwinit(void)
{
	return;
}

// sub_073B
void exit(u16 status)
{
	(void)status;
}

// sub_075D
void sub_075D()
{
}

// sub_075F
void main(void)
{
	/* Specify the ROM and RAM sizes */
	IMS = 0x06;
	IXS = 0x0A;

	WDTE = WATCHDOG_TIMER_ENABLE_REGISTER_RESET_WATCHDOG_TIMER;

	if (WDTE != 0) // 0x076B & 9x076D
	{
		P6 &= ~0x4; // 0x076F
		PM6 &= ~0x4; // 0x076F

		TMHMD1 = 0x50;
		CMP01 = 0xC;

		MK0H |= 0x8; // 0x077A
		IF0H &= ~0x8; // 0x077D

		TMHMD1 |= 0x80; // 0x0780

		/* Wait for bit 3 of Interrupt request flag register 0 to be set. */
		while (!(IF0H & 0x8))
			;;

		TMHMD1 &= ~0x80;
		P6 |= 0x4; // 0x076F
	}

loc_78E:

	// Wait for bit 0 of Port 12 to be set
	while (!(P12 & 0x1)) // 0x078E
	{
		/* Reset the watchdog timer. */
		WDTE = WATCHDOG_TIMER_ENABLE_REGISTER_RESET_WATCHDOG_TIMER;
	}

	/* Initialize hardware. */

	init_devices_1(0); // 0x0799
	sub_0932();
	sub_099A(0);
	sub_0987();

	init_battery_message();
	sub_2804();
	sub_429C();
	sub_5791();

	init_allegrex_session();
	init_pandora_secret_keys();

	if (!(P12 & 0x1)) // 0x07BA
	{
		goto loc_78E;
	}

	/* Copy function sub_1070 to RAM */
	memcpy(sub_1070, g_unkF400, (u32)sub_10A2 - (u32)sub_1070); // 0x07C1 - 0x7CF

	PM14 &= ~0x20;

	/* Enable interrupts */
	EI();

	/* Poll various interfaces and hardware components for new requests to handle. */
	for (;;)
	{
		/* Reset the watchdog timer. */
		WDTE = WATCHDOG_TIMER_ENABLE_REGISTER_RESET_WATCHDOG_TIMER;

		if (g_unkFE31 != 0x5) // 0x07DC & 07DF
		{
			allegrex_handshake();
			sub_56D2();
			battery_handshake_handler();
			response_handler();
			battery_serial_handler();
		}

		/* Check if an operation to handle has arrived. */
		if (g_isMainOperationRequestExist) // 0x07F0
		{
			/* An operation to handle has arrived. Run it now. */

			g_isMainOperationRequestExist = SCE_FALSE; // 0x07F5

			g_mainOperations[g_mainOperationId - MAIN_OPERATIONS_ID_START](); // 0x07F5 - 0x80C
		}

		sub_3F10();
		poll_interrfaces();
	}
}

// sub_0818
void init_devices_1(u16 arg0)
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
void sub_099A(u8 arg0)
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
void poll_interrfaces(void)
{
}

// sub_1070
void sub_1070(void)
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
	/*
	 * Port 7:
	 *
	 * SQUARE, CROSS, CIRCLE, TRIANGLE | LEFT, DOWN, RIGHT, UP
	 * 
	 * Port 4:
	 * 
	 * START, R, L, SELECT
	 * 
	 * Port 2:
	 * 
	 * SCE_CTRL_HOLD, SCE_CTRL_INTERCEPTED
	 */

	g_transmitData[0] = P7;

	u8 ctrlData2 = ((P2 << 4) & 0x30) | (P4 & 0xF);
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

	g_transmitData[2] = P5 | 0xFC; // 1957

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
void main_op_nop(void)
{
	g_mainOperationResultStatus = MAIN_OPERATION_RESULT_STATUS_NOT_IMPLEMENTED;
}

/* SYSCON [set] commands */

// sub_1A96
void write_clock(void)
{
	g_mainOperationTransmitPackageLength = SYSCON_CMD_TRANSMIT_DATA_BASE_LEN;

	/* Disable interrupts. */
	DI();

	/* Set the new clock value. */
	g_clock = *(u32 *)g_mainOperationPayloadReceiveBuffer;

	/*
	 * Watchdog timer operates with the subsystem clock frequency. Update the watchdog timer config
	 * to use the new clock setting.
	 */

	/* Stop and clear the watchdog timer counter. */
	WTM = 0xC0;

	/* Enable and start the watchdog timer counter. */
	WTM = 0xC3;

	/* Enable interrupts. */
	EI();

	g_mainOperationResultStatus = MAIN_OPERATION_RESULT_STATUS_SUCCESS;
}

// sub_1AB1
void set_usb_status(void)
{
	g_mainOperationTransmitPackageLength = SYSCON_CMD_TRANSMIT_DATA_BASE_LEN;

	/* Set the new USB status. */
	g_usbStatus = g_mainOperationPayloadReceiveBuffer[0];

	// TODO: Perhaps setting a flag here that a new USB status has been set.
	g_unkFE76 |= 0x2;

	g_mainOperationResultStatus = MAIN_OPERATION_RESULT_STATUS_SUCCESS;
}

// sub_1AC0
void write_alarm(void)
{
	g_mainOperationTransmitPackageLength = SYSCON_CMD_TRANSMIT_DATA_BASE_LEN;

	/* Disable interrupts. */
	DI();

	/* Set the new alarm value. */
	g_alarm = *(u32 *)g_mainOperationPayloadReceiveBuffer;

	/* Enable interrupts. */
	EI();

	g_mainOperationResultStatus = MAIN_OPERATION_RESULT_STATUS_SUCCESS;
}

/* 
 * Macros to obtain SYSCON's scratchpad destination ("index") and size of data to read/write from the received
 * encoded data (first byte of g_mainOperationPayloadReceiveBuffer).
 */

#define SYSCON_SCRATCHPAD_GET_DST(enc)					(((enc) >> 2) & 0x3F)
#define SYSCON_SCRATCHPAD_GET_DATA_SIZE(enc)			((enc) & 0x3)
#define SYSCON_SCRATCHPAD_SET_DATA_SIZE(enc, s)			(((enc) & ~0x3) | ((s) & 0x3))

// sub_1AD5
void write_scratchpad(void)
{
	u8 dstAndSizeEnc = g_mainOperationPayloadReceiveBuffer[0]; // 0x1ADA

	/* Check if the specified scratchpad destination is valid. */
	if (SYSCON_SCRATCHPAD_GET_DST(dstAndSizeEnc) >= SCRATCH_PAD_SIZE) // 0x1AE3 & 0x1AE5
	{
		g_mainOperationResultStatus = MAIN_OPERATION_RESULT_STATUS_ERROR;
		return;
	}

	/*
	 * The size of the content which should be written to the scratchpad is at most 0x8 bytes.
	 * That specified size is encoded in the lower 2 bit of the [dstAndSizeEnc] value as such:
	 *		0 = size 0x1
	 *		1 = size 0x2
	 *		2 = size 0x4
	 *		3 = size 0x8
	 * 
	 * Below we "unpack" the size from the [dstAndSizeEnc] value.
	 */
	u8 size = 1;

	/* As long as our "size counter" has not yet reached 0, we have to double the size. */
	while (SYSCON_SCRATCHPAD_GET_DATA_SIZE(dstAndSizeEnc) & 0x3) // 0x1AEA - 0x1B08
	{
		/* Increase our size counter 1 time. */
		size += size; // 0x1AF5

		/* Only decrease the "size counter" in the lower 2 bits. */
		SYSCON_SCRATCHPAD_SET_DATA_SIZE(dstAndSizeEnc, SYSCON_SCRATCHPAD_GET_DATA_SIZE(dstAndSizeEnc) - 1); // 0x1AF8 - 0x1B06 
	}

	/* Verify that we don't write more data to the scratchpad buffer than it can contain. */
	if ((SYSCON_SCRATCHPAD_GET_DST(dstAndSizeEnc) + size) > SCRATCH_PAD_SIZE) // 0x1B11 - 0x1B15
	{
		g_mainOperationResultStatus = MAIN_OPERATION_RESULT_STATUS_ERROR;
		return;
	}

	/*
	 * Now that we have decoded the size of the data to write and verified that we will only write
	 * inside the limits of the scratchpad buffer, let's actually write the data to SYSCON's scratchpad.
	 */
	while (size-- > 0)
	{
		/*
		 * We need to access the element at [size + 1] because the data to write to the scratchpad
		 * is located starting at g_mainOperationPayloadReceiveBuffer[1].
		 */
		u8 nScratchpadData = g_mainOperationPayloadReceiveBuffer[size + 1]; // 0x1B25

		g_scratchpad[SYSCON_SCRATCHPAD_GET_DST(dstAndSizeEnc) + size] = nScratchpadData;
	}

	g_mainOperationTransmitPackageLength = SYSCON_CMD_TRANSMIT_DATA_BASE_LEN;
	g_mainOperationResultStatus = MAIN_OPERATION_RESULT_STATUS_SUCCESS;
}

// sub_1B4C
void read_scratchpad(void)
{
	u8 dstAndSizeEnc = g_mainOperationPayloadReceiveBuffer[0]; // 0x1B51

	/* Check if the specified scratchpad destination is valid. */
	if (SYSCON_SCRATCHPAD_GET_DST(dstAndSizeEnc) >= SCRATCH_PAD_SIZE) // 0x1B56 - 0x1B5C
	{
		g_mainOperationResultStatus = MAIN_OPERATION_RESULT_STATUS_ERROR;
		return;
	}

	/*
	 * The size of the content which should be read from the scratchpad is at most 0x8 bytes.
	 * That specified size is encoded in the lower 2 bit of the [dstAndSizeEnc] value as such:
	 *		0 = size 0x1
	 *		1 = size 0x2
	 *		2 = size 0x4
	 *		3 = size 0x8
	 *
	 * Below we "unpack" the size from the [dstAndSizeEnc] value.
	 */
	u8 size = 1;

	/* As long as our "size counter" has not yet reached 0, we have to double the size. */
	while (SYSCON_SCRATCHPAD_GET_DATA_SIZE(dstAndSizeEnc) & 0x3) // 0x1B61 - 0x1B7F
	{
		/* Increase our size counter 1 time. */
		size += size; // 0x1B6C

		/* Only decrease the "size counter" in the lower 2 bits. */
		SYSCON_SCRATCHPAD_SET_DATA_SIZE(dstAndSizeEnc, SYSCON_SCRATCHPAD_GET_DATA_SIZE(dstAndSizeEnc) - 1); // 0x1B6F - 0x1B7D 
	}

	/* Verify that we don't read data outside the bounds the of the scratchpad buffer. */
	if ((SYSCON_SCRATCHPAD_GET_DST(dstAndSizeEnc) + size) > SCRATCH_PAD_SIZE) // 0x1B81 - 0x1B8C
	{
		g_mainOperationResultStatus = MAIN_OPERATION_RESULT_STATUS_ERROR;
		return;
	}

	/* Set the size of the data which will be transmitted. */
	g_mainOperationTransmitPackageLength = SYSCON_CMD_TRANSMIT_DATA_BASE_LEN + size; // 0x1B91

	/*
	 * Now that we have decoded the size of the data to read and verified that we will only read
	 * inside the limits of the scratchpad buffer, let's actually read the data from SYSCON's scratchpad.
	 */
	while (size-- > 0) // 0x1B93 - 0x1BB3
	{
		u8 scratchpadData = g_scratchpad[SYSCON_SCRATCHPAD_GET_DST(dstAndSizeEnc) + size]; // 0x1B9B - 1BA9
		g_mainOperationPayloadTransmitBuffer[size] = scratchpadData; // 0x1BB1
	}

	g_mainOperationResultStatus = MAIN_OPERATION_RESULT_STATUS_SUCCESS;
}

/* This constant defines the default setParam ID if no ID has been specified. */
#define SYSCON_SET_PARAM_ID_DEFAULT		SCE_SYSCON_SET_PARAM_POWER_BATTERY_SUSPEND_CAPACITY

// sub_1BC5
void send_setparam(void)
{
	if ((g_mainOperationReceivedPackageLength == 10
		|| (g_mainOperationReceivedPackageLength == 11 && g_mainOperationPayloadReceiveBuffer[8] == 0 /* setParam ID 0 */))
		&& g_mainOperationPayloadReceiveBuffer[6] >= 10) // 0x1BCB & 0x1BCF & 0x1BD6 & 0x1BDD
	{
		// loc_1BE1

		/* Set payload for setParam with ID 0. */

		u8 i;
		for (i = 0; i < SCE_SYSCON_SET_PARAM_PAYLOAD_SIZE; i++) // 0x1BE1 - 0x1BF9
		{
			g_setParamPayloads[0].data[i] = g_mainOperationPayloadReceiveBuffer[i];
		}

		g_mainOperationTransmitPackageLength = SYSCON_CMD_TRANSMIT_DATA_BASE_LEN;
		g_mainOperationResultStatus = MAIN_OPERATION_RESULT_STATUS_SUCCESS;
	}
	else if (g_mainOperationReceivedPackageLength == 11 && g_mainOperationPayloadReceiveBuffer[8] == 1) // 0x1C07 & 0x1C0F
	{
		/* Verify if the payload data for setParam ID 1 is correct. */

		u16 payload = ((u16 *)g_mainOperationPayloadReceiveBuffer)[0]; // 0x1C11
		if (payload <= 3264 || payload > 18560) // 0x1C14 - 0x1C19
		{
			g_mainOperationTransmitPackageLength = SYSCON_CMD_TRANSMIT_DATA_BASE_LEN;
			g_mainOperationResultStatus = MAIN_OPERATION_RESULT_STATUS_SUCCESS;

			return;
		}

		payload = ((u16 *)g_mainOperationPayloadReceiveBuffer)[1]; // 0x1C22
		if (payload < 46912 || payload >= 62208) // 0x1C28 & 0x1C2D
		{
			g_mainOperationTransmitPackageLength = SYSCON_CMD_TRANSMIT_DATA_BASE_LEN;
			g_mainOperationResultStatus = MAIN_OPERATION_RESULT_STATUS_SUCCESS;

			return;
		}

		payload = ((u16 *)g_mainOperationPayloadReceiveBuffer)[2]; // 0x1C2F
		if (payload <= 3264 || payload > 18560) // 0x1C32 & 0x1C39 & 1C3E
		{
			g_mainOperationTransmitPackageLength = SYSCON_CMD_TRANSMIT_DATA_BASE_LEN;
			g_mainOperationResultStatus = MAIN_OPERATION_RESULT_STATUS_SUCCESS;

			return;
		}

		payload = ((u16 *)g_mainOperationPayloadReceiveBuffer)[3]; // 1C40
		if (payload < 46912 || payload >= 62208) // 0x1C43 & 0x1C48
		{
			g_mainOperationTransmitPackageLength = SYSCON_CMD_TRANSMIT_DATA_BASE_LEN;
			g_mainOperationResultStatus = MAIN_OPERATION_RESULT_STATUS_SUCCESS;

			return;
		}

		/* Payload data is correct. Set it now. */

		u8 i;
		for (i = 0; i < SCE_SYSCON_SET_PARAM_PAYLOAD_SIZE; i++) // 0x1C4F - 0x1C69
		{
			g_setParamPayloads[1].data[i] = g_mainOperationPayloadReceiveBuffer[i];
		}

		// loc_1C6B

		/* Create copy of the data (which won't be modified) for setParam with ID 1. */

		g_setParam1PayloadData1 = ((u16 *)g_setParamPayloads[1].data)[1];
		g_setParam1PayloadData0 = ((u16 *)g_setParamPayloads[1].data)[0];
		g_setParam1PayloadData3 = ((u16 *)g_setParamPayloads[1].data)[3];
		g_setParam1PayloadData2 = ((u16 *)g_setParamPayloads[1].data)[2];

		g_mainOperationTransmitPackageLength = SYSCON_CMD_TRANSMIT_DATA_BASE_LEN;
		g_mainOperationResultStatus = MAIN_OPERATION_RESULT_STATUS_SUCCESS;
	}
	else if (g_mainOperationReceivedPackageLength != 11 
		|| g_mainOperationPayloadReceiveBuffer[8] > 3) // 0x1C8A & 0x1C90
	{
		/* 
		 * We only support packages which have either length 10 or length 11 (an additional byte for the
		 * setParam ID).
		 * 
		 * We also do not support setting param values for the [SCE_SYSCON_SET_PARAM_POWER_BATTERY_TTC] ID
		 * in a PSP model equipped with the TA-096 motherboard. This setParam is only supported on PSP models
		 * of the PSP-2000 series (Motherboards TA-085v1 - TA-090v1).
		 */
		g_mainOperationResultStatus = MAIN_OPERATION_RESULT_STATUS_ERROR;
	}
	else
	{
		/* Write the specified data for the remaining setParams 2 or 3. */

		u8 setParamId = g_mainOperationPayloadReceiveBuffer[8];

		u8 i;
		for (i = 0; i < SCE_SYSCON_SET_PARAM_PAYLOAD_SIZE; i++) // 0x1C4F - 0x1C69
		{
			g_setParamPayloads[setParamId].data[i] = g_mainOperationPayloadReceiveBuffer[i];
		}

		if (setParamId == 2) // 0x1CC0 & 0x1CC2
		{
			g_unkFE78 |= 0x1; // 0x1CC4
		}

		g_mainOperationTransmitPackageLength = SYSCON_CMD_TRANSMIT_DATA_BASE_LEN;
		g_mainOperationResultStatus = MAIN_OPERATION_RESULT_STATUS_SUCCESS;
	}
}

// sub_1CD8
void receive_setparam(void)
{
	u8 setParamId;

	/* Set default setParam ID. */
	setParamId = SYSCON_SET_PARAM_ID_DEFAULT; // 0x1CD9

	/* Check if a setParam ID has been set. */
	if (g_mainOperationReceivedPackageLength == 3) // 0x1CE0
	{
		/* Obtain the specified setParam ID. */
		setParamId = g_mainOperationPayloadReceiveBuffer[0];
	}

	if (setParamId > SYSCON_SET_PARAM_MAX_SET_PARAM_ID) // 0x1CED
	{
		g_mainOperationResultStatus = MAIN_OPERATION_RESULT_STATUS_INVALID_ID;
		return;
	}

	if (setParamId == 1) // 0x1CF4
	{
		((u16 *)&g_setParamPayloads[1].data)[1] = g_setParam1PayloadData1;
		((u16 *)&g_setParamPayloads[1].data)[0] = g_setParam1PayloadData0;
		((u16 *)&g_setParamPayloads[1].data)[3] = g_setParam1PayloadData3;
		((u16 *)&g_setParamPayloads[1].data)[2] = g_setParam1PayloadData2;
	}

	u8 i;
	for (i = 0; i < SCE_SYSCON_SET_PARAM_PAYLOAD_SIZE; i++) // 0x1D0A - 0x1D25
	{
		g_mainOperationPayloadTransmitBuffer[i] = g_setParamPayloads[setParamId].data[i];
	}

	g_mainOperationTransmitPackageLength = SYSCON_CMD_TRANSMIT_DATA_BASE_LEN + SCE_SYSCON_SET_PARAM_PAYLOAD_SIZE;
	g_mainOperationResultStatus = MAIN_OPERATION_RESULT_STATUS_SUCCESS;
}

// sub_1D34
void exec_syscon_cmd_0x30(void)
{
}

// sub_1FD2
void ctrl_tachyon_wdt(void)
{
	if (g_mainOperationPayloadReceiveBuffer[0] > 0x80) // 0x1FD4 & 0x1FD7
	{
		g_watchdogTimerStatus = WATCHDOG_TIMER_STATUS_COUNTING;

		g_watchdogTimerCounterResetValue = g_mainOperationPayloadReceiveBuffer[0] + g_mainOperationPayloadReceiveBuffer[0];
		g_watchdogTimerCounter = g_mainOperationPayloadReceiveBuffer[0] + g_mainOperationPayloadReceiveBuffer[0];
	}
	else
	{
		// loc_1FEB

		g_watchdogTimerStatus = WATCHDOG_TIMER_STATUS_NOT_COUNTING;
	}

	g_mainOperationTransmitPackageLength = SYSCON_CMD_TRANSMIT_DATA_BASE_LEN;
	g_mainOperationResultStatus = MAIN_OPERATION_RESULT_STATUS_SUCCESS;
}

// sub_1FF7
void reset_device(void )
{
	u8 resetType;

	g_mainOperationTransmitPackageLength = SYSCON_CMD_TRANSMIT_DATA_BASE_LEN; // 0x1FFC

	resetType = g_mainOperationPayloadReceiveBuffer[0]; // 0x1FFF
	if (!(resetType & 0x1)) // 0x2004
	{
		// loc_205D
		if (!(resetType & 0x2))
		{
			g_mainOperationResultStatus = MAIN_OPERATION_RESULT_STATUS_INVALID_ID;
			return;
		}

		if (resetType & 0x80) // 0x2065 & 0x2066
		{
			P6 &= ~0x8;
			P0 &= ~0x4;
		}
		else
		{
			// loc_206F

			if (!(g_unkFE45 & 0x4)) // 0x206F
			{
				P2 |= 0x4;
			}

			P3 |= 0x8;
		}
		g_mainOperationResultStatus = MAIN_OPERATION_RESULT_STATUS_SUCCESS;
		return;
	}

	// 0x2007

	/* (Reset type & 0x1) == 1 */

	/* Mask interrupt for input pin 2 (P31) (rising edge). */
	MK0L |= MK0L_INTR_MASK_FLAG_P2; // 0x2007

	if (!(MK0H & MK0L_INTR_MASK_FLAG_P1) || g_unkFE4F == 0x81) // 200A & 0x200E & 0x2011
	{
		g_isMainOperationRequestExist = SCE_TRUE; // 0x2013
		return;
	}

	/* Disable interrupts. */
	DI();

	P0 &= ~0x20;

	g_unkFE4F = 0x80;
	g_mainOperationResultStatus = MAIN_OPERATION_RESULT_STATUS_UNKNOWN_0x80;

	if (resetType & 0x40 && g_unkFC83 != 0) // 0x2022 - 0x2026
	{
		g_watchdogTimerStatus = WATCHDOG_TIMER_STATUS_NOT_COUNTING; // 0x2032

		/* Set serial clock I/O pin of serial interface CSI11. */
		P0 |= 0x10;
	}
	else
	{
		// loc_2039
		g_watchdogTimerStatus = WATCHDOG_TIMER_STATUS_COUNTING;
	}

	// loc_203E

	init_allegrex_session();

	g_wakeUpFactor = ((g_wakeUpFactor & 0xC4) | 0x4) & 0x7F; // 0x2041 - 0x204D

	/* Clear interrupt request flag for interrupt INTCSI10 */
	IF0H &= ~IF0H_INTR_REQ_FLAG_CSI10;
	P0 |= 0x20;

	/* Enable interrupts. */
	EI();

	/* Clear interrupt request flag for input pin 3 (P31)  */
	IF0L &= ~IF0L_INTR_REQ_FLAG_P2;

	/* Remove applied interrupt masks. */
	MK0L &= ~MK0L_INTR_MASK_FLAG_P2;
}

// sub_2081
/* Enable/disable analog pad sampling. */
void ctrl_analog_xy_polling(void)
{
	u8 unk1;
	u8 samplingMode;

	g_mainOperationTransmitPackageLength = SYSCON_CMD_TRANSMIT_DATA_BASE_LEN;

	unk1 = g_mainOperationPayloadReceiveBuffer[1];
	if (unk1 & 0x80 
		|| g_mainOperationReceivedPackageLength == 3) // 0x2088 & 0x208E
	{
		/* Check if analog pad sampling should be enabled. */
		samplingMode = g_mainOperationPayloadReceiveBuffer[0]; // 0x2092
		if (samplingMode & SCE_CTRL_INPUT_DIGITAL_ANALOG 
			&& !(g_unkFE96 & SYSCON_CTRL_ANALOG_SAMPLING_ENABLED)) // 0x2095 & 0x2098
		{
			/* Enable analog pad sampling. */

			g_unkFE96 |= SYSCON_CTRL_ANALOG_SAMPLING_ENABLED; // 209B
			sub_42CB();
		}

		// loc_20A0

		samplingMode = g_mainOperationPayloadReceiveBuffer[0] >> 1; // 0x20A3
		samplingMode = g_mainOperationPayloadReceiveBuffer[0]; // 0x20A4
		if (samplingMode & 0x4) // 0x20A9
		{
			g_unkFD29 = 0; // 0x20AE
			g_unkFE96 |= 0x80; // 0x20B1
			g_unkFE98 |= 0x4; // 0x20B3
		}
	}

	// loc_20B5

	unk1 = g_mainOperationPayloadReceiveBuffer[1];
	if ((unk1 & 0x80) && (g_mainOperationReceivedPackageLength != 3)) // 0x20B9 & 0x20BF
	{
		g_mainOperationResultStatus = MAIN_OPERATION_RESULT_STATUS_SUCCESS;
		return;
	}

	if (g_mainOperationReceivedPackageLength == 3) // 0x20C3 - 0x20C8
	{
		/* Flip samplingMode bits */
		g_mainOperationPayloadReceiveBuffer[0] = ~g_mainOperationPayloadReceiveBuffer[0]; // 0x20CD - 0x20D0
	}

	/* Check if analog sampling needs to be disabled. */

	samplingMode = g_mainOperationPayloadReceiveBuffer[0]; // 0x20C3

	/*
	 * As we have flipped the bits earlier, we actually test here if samplingMode has been set to
	 * ::SCE_CTRL_INPUT_DIGITAL_ONLY.
	 */
	if (samplingMode & 0x1) // 20D4
	{
		/* Disable analog pad sampling. */

		g_unkFE96 &= ~SYSCON_CTRL_ANALOG_SAMPLING_ENABLED; // 0x20D7
		g_ctrlAnalogDataX = SCE_CTRL_ANALOG_PAD_CENTER_VALUE;
		g_ctrlAnalogDataY = SCE_CTRL_ANALOG_PAD_CENTER_VALUE;
	}

	// TODO: Not sure what this is about, samplingMode doesn't appear to be used any further, nor is it
	// a return value.
	samplingMode = g_mainOperationPayloadReceiveBuffer[0] >> 1;

	g_mainOperationResultStatus = MAIN_OPERATION_RESULT_STATUS_SUCCESS;
}

// sub_20E9
void sub_20E9(void)
{
}

// sub_20F3
void power_standby(void)
{
}

// sub_20F9
void power_suspend(void)
{
}

// sub_2107
void get_batt_volt_ad(void)
{
	g_mainOperationTransmitPackageLength = SYSCON_CMD_TRANSMIT_DATA_BASE_LEN;

	if (g_unkFD49 == 0x80) // 0x210F
	{
		if (g_powerSupplyStatus & SCE_SYSCON_POWER_SUPPLY_STATUS_BATTERY_EQUIPPED) // 0x2111
		{
			g_unkFD47 = 0;
			g_unkFECF |= 0x2;
			g_unkFD49 = 0x81;
		}
		else
		{
			/* No battery is equipped. */

			// loc_2123
			g_mainOperationTransmitPackageLength = SYSCON_CMD_TRANSMIT_DATA_BASE_LEN + 3;

			g_mainOperationPayloadTransmitBuffer[0] = 0;
			g_mainOperationPayloadTransmitBuffer[1] = 0;
			g_mainOperationPayloadTransmitBuffer[2] = 0;

			g_mainOperationResultStatus = MAIN_OPERATION_RESULT_STATUS_SUCCESS;
		}
	}
	else if (g_unkFD49 == 0x82) // 0x2137
	{
		// loc_2134

		g_mainOperationTransmitPackageLength = SYSCON_CMD_TRANSMIT_DATA_BASE_LEN + 3;

		((u16 *)g_mainOperationPayloadTransmitBuffer)[0] = g_battVoltADUnkVal;
		g_mainOperationPayloadTransmitBuffer[2] = g_battVoltBase;

		g_unkFD49 = 0x80;
		g_mainOperationResultStatus = MAIN_OPERATION_RESULT_STATUS_SUCCESS;
	}

	// loc_2155
	if (g_unkFD49 == 0x81)
	{
		g_isMainOperationRequestExist = SCE_TRUE;
		g_mainOperationResultStatus = MAIN_OPERATION_RESULT_STATUS_UNKNOWN_0x81;
	}
}

// TODO: more functions here...

// sub_2654
void response_handler()
{

}

// sub_42CB
void sub_42CB(void)
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

// sub_50C4
void generate_challenge(void)
{
}

// sub_5103
void final_key_encryption_cbc(u8 *key1, u8 *key2, u8 *data)
{
}

// sub_5190
void allegrex_handshake(void)
{
}

// sub_54E5
void init_allegrex_session(void)
{
}

// sub_5516
void battery_serial_handler(void)
{
}

// sub_55CF
void read_secure_flash(u16 flashOffset)
{
}

// sub_5637
void init_pandora_secret_keys(void)
{
}

// sub_56D2
void sub_56D2(void)
{
}

// sub_5791
void sub_5791(void)
{
}

// TODO: more functions here...


