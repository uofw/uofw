/* Copyright (C) The uOFW team
   See the file COPYING for copying permission.
*/

#include "common_header.h"

#ifndef FIRMWARE_H
#define FIRMWARE_H

/*
 * This constant specifies the base length of the data to be transmitted back to the SYSCON module.
 * This base length represents the first three members in the ::SceSysconPacket.tx[16] data buffer, which
 * are always sent back to the SYSCON module.
 */
#define SYSCON_CMD_TRANSMIT_DATA_BASE_LEN    3

 /*
  * This constant indicates that the watch dog timer is not currently counting. For example, this is the case
  * when the timer counter has counted downwards until it reached the value 0, at which point there is nothing
  * left to count. This can indicate that the watchdog timer counter could not be properly resetted.
  */
#define WATCHDOG_TIMER_STATUS_NOT_COUNTING    0
#define WATCHDOG_TIMER_STATUS_COUNTING        1

#define MAIN_OPERATION_RESULT_STATUS_UNKNOWN_0x80     0x80
#define MAIN_OPERATION_RESULT_STATUS_UNKNOWN_0x81     0x81 // TODO: perhaps a [retry] indicator -- perhaps SYSCON_BUSY?
#define MAIN_OPERATION_RESULT_STATUS_SUCCESS          0x82
#define MAIN_OPERATION_RESULT_STATUS_ERROR            0x83 // TODO: More like: Invalid_Size
#define MAIN_OPERATION_RESULT_STATUS_NOT_SUPPORTED    0x84
#define MAIN_OPERATION_RESULT_STATUS_INVALID_ID       0x87

#define MAIN_OPERATIONS_ID_START          PSP_SYSCON_CMD_WRITE_CLOCK
#define PERIPHERAL_OPERATIONS_ID_START    PSP_SYSCON_CMD_GET_POMMEL_VERSION

/* SYSCON's internal scratchpad buffer size. */
#define SCRATCH_PAD_SIZE    32

/*
* Macros to obtain SYSCON's scratchpad destination ("index") and size of data to read/write from the received
* encoded data (first byte of g_mainOperationPayloadReceiveBuffer).
*/

#define SYSCON_SCRATCHPAD_GET_DST(enc)             (((enc) >> 2) & 0x3F)
#define SYSCON_SCRATCHPAD_GET_DATA_SIZE(enc)       ((enc) & 0x3)
#define SYSCON_SCRATCHPAD_SET_DATA_SIZE(enc, s)    (((enc) & ~0x3) | ((s) & 0x3))

#endif // FIRMWARE_H
