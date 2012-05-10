/* Copyright (C) 2011, 2012 The uOFW team
   See the file COPYING for copying permission.
*/

#include "common.h"

/* syscon hardware controller transfer modes. */
#define SYSCON_CTRL_TRANSFER_DATA_DIGITAL_ONLY    7
#define SYSCON_CTRL_TRANSFER_DATA_DIGITAL_ANALOG  8

typedef struct SceSysconPacket SceSysconPacket;

struct SceSysconPacket
{
    u8 unk00[4]; //0 -- (0x00,0x00,0x00,0x00)
    u8 unk04[2]; //4 -- (arg2)
    u8 status; //6
    u8 unk07; //7 -- (0x00)
    u8 unk08[4]; //8 -- (0xff,0xff,0xff,0xff)
    /** transmit data. */
    u8 tx_cmd; //12 -- command code
    u8 tx_len; //13 -- number of transmit bytes
    u8 tx_data[14]; //14 -- transmit parameters
    /** receive data. */
    u8 rx_sts; //28 --  generic status
    u8 rx_len; //29 --  receive length
    u8 rx_response; //30 --  response code(tx_cmd or status code)
    u8 rx_data[9]; //31 --  receive parameters
    u32 unk28; //40
    /** user callback (when finish an access?) */
    void (*callback)(SceSysconPacket *, u32); //44
    u32 callback_r28; //48
    u32 callback_arg2; //52 -- arg2 of callback (arg4 of sceSycconCmdExec)
    u8 unk38[13]; //56
    u8 old_sts; //69 -- old rx_sts
    u8 cur_sts; //70 --  current rx_sts
    u8 unk47[24]; //71
}; //size of SceSysconPacket: 96

int sceSysconCtrlTachyonAvcPower(int);
void sceSyscon_driver_B72DDFD2(int);
int sceSyscon_driver_97765E27();
int sceSysconCmdExecAsync(SceSysconPacket *, int, int (*)(), int);

