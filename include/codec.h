/* Copyright (C) 2011, 2012 The uOFW team
   See the file COPYING for copying permission.
*/

/**
 * @author artart78
 * @version 6.60
 *
 * The codec.prx module RE'ing.
 */

#include "common_header.h"

int sceCodecOutputEnable(int arg0, int arg1);
int sceCodecSetOutputVolume(int reg);
int sceCodecSetHeadphoneVolume(int arg0);
int sceCodecSetSpeakerVolume(int arg0);
int sceCodecSetVolumeOffset(char arg0);
int sceCodecOutputDisable(void);
int sceCodec_driver_FCA6D35B(int freq);
int sceCodec_driver_6FFC0FA4(char arg0);
int sceCodec_driver_A88FD064(int arg0, int arg1, int arg2, int arg3, int arg4, int arg5);
int sceCodec_driver_277DFFB6(void);
void sceCodec_driver_376399B6(int enable);
int sceCodecSelectVolumeTable(int arg0);
int sceCodec_driver_E61A4623(void);
int sceCodec_driver_FC355DE0(void);

