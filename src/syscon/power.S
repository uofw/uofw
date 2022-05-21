# Copyright (C) 2011, 2012 The uOFW team
# See the file COPYING for copying permission.

#include <common_asm.h>

    .globl sub_0000
sub_0000:
    move       $s0, $a0
    move       $s1, $a1
    bnez       $a2, loc_0094
    nop        
    lui        $t0, 0xBD00
    li         $t1, 5
    sw         $t1, 4($t0)

loc_001C:
    lw         $t1, 4($t0)
    andi       $t1, $t1, 0xF
    bnez       $t1, loc_001C
    nop        

loc_002C:
    lw         $t1, 0($t0)
    andi       $t1, $t1, 0x200
    bnez       $t1, loc_002C
    nop        
    li         $a0, 2
    move       $a1, $zr
    move       $a2, $zr

loc_0048:
    lw         $t1, 32($t0)
    ext        $t1, $t1, 16, 1
    bnez       $t1, loc_0048
    nop        
    andi       $t1, $a0, 0x1
    sll        $t1, $t1, 10
    sll        $t2, $a1, 16
    or         $t1, $t1, $t2
    andi       $t2, $a2, 0xC7FF
    or         $t1, $t1, $t2
    sw         $t1, 36($t0)
    sync       
    ori        $t1, $a0, 0x8000
    sw         $t1, 32($t0)
    sync       

loc_0084:
    lw         $t1, 32($t0)
    ext        $t1, $t1, 16, 1
    bnez       $t1, loc_0084
    nop        

loc_0094:
    lui        $t0, 0xBC10
    li         $t1, 131
    sw         $t1, 104($t0)
    lw         $t1, 152($t0)
    ext        $t1, $t1, 11, 3
    li         $t2, 11
    subu       $t2, $t2, $t1
    addiu      $a0, $s0, 16
    lui        $t1, 0x1
    ori        $t1, $t1, 0x542
    sw         $t1, 0($a0)
    sb         $t2, 4($a0)
    addiu      $t2, $t2, 72
    andi       $t2, $t2, 0xFF
    xori       $t2, $t2, 0xFF
    sb         $t2, 5($a0)
    bal        sub_0110
    move       $a1, $zr
    bnez       $s1, loc_00FC
    nop        
    lui        $t0, 0xBC10
    lw         $t1, 120($t0)
    lui        $t2, 0xFFF3
    ori        $t2, $t2, 0xFFFD
    and        $t1, $t1, $t2
    sw         $t1, 120($t0)

loc_00FC:
    move       $a0, $s0
    bal        sub_0110
    li         $a1, 1

loc_0108:
    b          loc_0108
    halt       

sub_0110:
    move       $t3, $ra
    move       $t4, $a0
    lui        $t0, 0xBE24
    li         $t1, 8
    sw         $t1, 12($t0)

loc_0124:
    lw         $t1, 4($t0)
    andi       $t1, $t1, 0x8
    bnez       $t1, loc_0124
    nop        
    bal        sub_0204
    li         $a0, 5
    lui        $t0, 0xBE58

loc_0140:
    lw         $t1, 12($t0)
    andi       $t1, $t1, 0x4
    bnezl      $t1, loc_0140
    lw         $t1, 8($t0)
    li         $t1, 3
    sw         $t1, 32($t0)
    move       $t5, $t4
    lhu        $t2, 0($t5)
    srl        $t2, $t2, 8
    andi       $t1, $t2, 0x1
    subu       $t2, $t2, $t1

loc_016C:
    lhu        $t1, 0($t5)
    wsbh       $t1, $t1
    addiu      $t5, $t5, 2
    addiu      $t2, $t2, -2
    bgez       $t2, loc_016C
    sw         $t1, 8($t0)
    li         $t1, 6
    sw         $t1, 4($t0)
    lui        $t0, 0xBE24
    li         $t1, 8
    sw         $t1, 8($t0)
    sync       

loc_019C:
    lw         $t1, 32($t0)
    andi       $t1, $t1, 0x10
    beqz       $t1, loc_019C
    nop        
    li         $t1, 8
    sw         $t1, 12($t0)
    li         $t1, 16
    sw         $t1, 36($t0)
    sync       
    bal        sub_0204
    li         $a0, 4000
    bnez       $a1, loc_01FC
    nop        
    lui        $t0, 0xBE58
    lw         $t1, 8($t0)
    lw         $t1, 8($t0)

loc_01DC:
    lw         $t2, 12($t0)
    andi       $t2, $t2, 0x4
    bnezl      $t2, loc_01DC
    lw         $t2, 8($t0)
    ext        $t1, $t1, 8, 8
    xori       $t1, $t1, 0x82
    bnez       $t1, loc_0124
    nop        

loc_01FC:
    jr         $t3
    nop        

sub_0204:
    sll        $at, $a0, 1
    addu       $at, $at, $a0
    sll        $at, $at, 5

loc_0210:
    bnez       $at, loc_0210
    addiu      $at, $at, -1
    jr         $ra
    nop        

