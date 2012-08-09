/* Copyright (C) 2011, 2012 The uOFW team
   See the file COPYING for copying permission.
*/

/* 
 * memlmd.h
 */

#ifndef MEMLMD_H
#define	MEMLMD_H

#ifdef	__cplusplus
extern "C" {
#endif

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
s32 memlmd_EF73E85B(u8 *prx, u32 size, u32 *newsize);

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
s32 memlmd_F26A33C3(u32 unk, u8 *hashAddr);

/**	
 * Decrypts a module. Synced mode.
 * 
 * @param prx PRX buffer.
 * @param size Current size of PRX buffer.
 * @param newsize Size of PRX after decryption.
 * 
 * @return 0 on success.
 */
s32 memlmd_CF03556B(u8 *prx, u32 size, u32 *newsize);

/**	
 * Enables the bus of KIRK chip if possibile
 * 
 * @return 0 on success.
 */
s32 memlmd_2F3D7E2D(void);


#ifdef	__cplusplus
}
#endif

#endif	/* MEMLMD_H */

