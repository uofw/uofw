# Copyright (C) 2011, 2012, 2013 The uOFW team
# See the file COPYING for copying permission.

#include "common_asm.h"

    .text

    .globl initClearMem

# initClearMem(u32 *baseAddr, u32 size)
# zeros out memory range starting at "baseAddr"
# for "size" words.
    
    .ent initClearMem
initClearMem:
    move       $v0, $a0 # origAddr = baseAddr
	addu       $t0, $a0, $a1 # maxWordAddr = baseAddr + size
	addiu      $t0, $t0, -4 # maxWordAddr -= 0x4
loop1: 
	sw         $zr, 0($a0) # do { *baseAddr = 0; } 
	bne        $t0, $a0, loop1 # while (baseAddr++ != maxWordAddr);
	addiu      $a0, $a0, 4
loop2: 
	sw         $zr, 0($v0) # do { *origAddr = 0; }
	bne        $t0, $v0, loop2 # while (origAddr++ != maxWordAddr);
	addiu      $v0, $v0, 4
	break      0x0
    .end initClearMem


