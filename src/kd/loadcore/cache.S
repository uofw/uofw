# Copyright (C) 2011, 2012, 2013 The uOFW team
# See the file COPYING for copying permission.

#include "common_asm.h"

    .text

    .global sceKernelDcacheWBinvAll
    .global sceKernelIcacheClearAll

##
# sub_0000744C
##

    .ent sceKernelDcacheWBinvAll
sceKernelDcacheWBinvAll:
    mfc0    $a2, COP0_STATE_CONFIG
    ext     $a1, $a2, 6, 3
    li      $a0, 1
    addiu   $v0, $a1, 12
    sllv    $a1, $a0, $v0
    srl     $a0, $a1, 1
    beqz    $a0, exit
    move    $v1, $zero

loc_0000746C:
    cache   0x14, 0($v1)
    cache   0x14, 0($v1)
    addiu   $v1, $v1, 64
    sltu    $a3, $v1, $a0 
    bnez    $a3, loc_0000746C

exit:
    jr      $ra
    nop
    .end sceKernelDcacheWBinvAll


##
# sub_0000748C
##
    .ent sceKernelIcacheClearAll
sceKernelIcacheClearAll:
    mfc0       $a2, COP0_STATE_CONFIG
    ext        $a1, $a2, 9, 3
    li         $a0, 1
    addiu      $v0, $a1, 12
    sllv       $a1, $a0, $v0
    srl        $a0, $a1, 1
    beqz       $a0, exit1
    move       $v1, $zero

loc_000074AC:		
    cache      0x4, 0($v1)
    cache      0x4, 0($v1)
    addiu      $v1, $v1, 64
    sltu       $a3, $v1, $a0
    bnez       $a3, loc_000074AC
    nop        

exit1:
    jr         $ra
    nop  
    .end sceKernelIcacheClearAll


