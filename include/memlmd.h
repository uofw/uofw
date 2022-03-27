/* Copyright (C) 2011, 2012, 2013 The uOFW team
   See the file COPYING for copying permission.
*/

/* 
 * memlmd.h
 */

#ifndef MEMLMD_H
#define	MEMLMD_H

#include "common_header.h"
    
/** The internal name of the MEMLMD module. */
#define MEMLMD_MODULE_NAME          "memlmd"

/**	
 * Decrypts a module. Asynced mode.
 * 
 * 
 * @param prx PRX buffer.
 * @param size Current size of PRX buffer.
 * @param newsize Size of PRX after decryption.
 * 
 * @return 0 on success.
 */
s32 memlmd_EF73E85B(u8 *prx, u32 size, u32 * newSize);

/**	
 * Checks the param against a magic value (unknown usage)
 * 
 * @param unk Unknown param.
 * 
 * @return ??.
 */
s32 memlmd_2AE425D2(u32 unk);

/**	
 * Checks the param against a magic value (unknown usage)
 * 
 * @param unk Unknown param.
 * 
 * @return ??.
 */
s32 memlmd_9D36A439(u32 unk);

/**	
 * Creates the internal set of scramble keys by XORing pre-compiled set with
 * seed provided by user
 * 
 * @param unk Unknown param. Completely not used, just pass something non-zero.
 * @param hashAddr Buffer with seeds to XOR with. Usually used hardware buffer 
 *                 at 0xBFC00200.
 * 
 * @return 0 on success.
 */
s32 memlmd_F26A33C3(u32 unk, vs32 *hashAddr);

/**	
 * Decrypts a module. Synced mode.
 * 
 * @param prx PRX buffer.
 * @param size Current size of PRX buffer.
 * @param newsize Size of PRX after decryption.
 * 
 * @return 0 on success.
 */
s32 memlmd_CF03556B(u8 *prx, u32 size, u32 * newSize);

/**	
 * Unsign a module. Asynced mode.
 * 
 * 
 * @param addr PRX buffer.
 * @param size Size of the PRX buffer.
 * 
 * @return 0 on success.
 */
s32 memlmd_6192F715(u8 *addr, u32 size);

/**	
 * Enables the bus of KIRK chip if possibile
 * 
 * @return 0 on success.
 */
s32 memlmd_2F3D7E2D(void);

/**
* Enables KIRK, then performs a command in asynchronised mode (refreshes
* CPU D cache for input and output buffer), and disables KIRK.
*
* @param outbuff Output buffer.
* @param outsize Output size.
* @param inbuff Input buffer.
* @param insize Input size.
* @param cmd Number of KIRK command to perform.
*
* @return 0 on success.
*/
int sceUtilsBufferCopyWithRange(u8* outbuff, int outsize, u8* inbuff, int insize, int cmd);

#endif	/* MEMLMD_H */

