# Copyright (C) 2011, 2012 The uOFW team
# See the file COPYING for copying permission.

#include "common_asm.h"

##
# sub_0000
##

    .globl sub_0000
sub_0000:
    mfc0  $v0, $11
    lui   $v1, 0x8000
    addu  $v1, $v1, $v0
    mtc0  $v1, $11
    bnez  $v0, loc_0024
    lui   $a3, %hi(intInfo+120)
    lw    $v0, %lo(intInfo+124)($a3)
    addiu $v0, $v0, 1
    sw    $v0, %lo(intInfo+124)($a3)

loc_0024:
    li    $v0, -1
    mfc0  $v1, $9
    jr    $ra
    sw    $v1, %lo(intInfo+120)($a3)

    nop # ?

##
# intrExcepHandler
##
    .globl intrExcepHandler
intrExcepHandler:
    nop
    nop
    cfc0 $v0, $13
    bnez $v0, loc_0218
    move $v1, $sp
    cfc0 $sp, $14

loc_0050:
    cache 0x18, 52($sp)
    cache 0x18, 64($sp)
    cache 0x18, 128($sp)
    sw    $v1, 180($sp)
    cfc0  $v0, $4
    cfc0  $v1, $5
    sw    $v0, 72($sp)
    sw    $v1, 76($sp)
    sw    $a0, 80($sp)
    sw    $a1, 84($sp)
    sw    $k0, 168($sp)
    sw    $k1, 172($sp)
    sw    $ra, 188($sp)
    cfc0  $ra, $2
    cfc0  $a1, $3
    sw    $ra, 52($sp)
    cfc0  $k0, $0
    cfc0  $k1, $23
    sw    $a1, 64($sp)
    andi  $v0, $ra, 0x18
    sra   $v1, $k0, 31                                                                                  
    sw    $k0, 56($sp)                                                                                  
    or    $v0, $v0, $v1
    beqz  $v0, loc_00000914
    sw    $k1, 60($sp)
    mfhi  $v0
    mflo  $v1
    sw    $v0, 44($sp)
    lui   $k1, 0x2000
    or    $k1, $k1, $ra
    sw    $v1, 48($sp)
    mtc0  $k1, $12
    cfc1  $v0, $31
    sw    $at, 68($sp)
    ctc1  $0,  $31
    ins   $ra, $0, 0, 5
    ins   $ra, $0, 28, 4
    sw    $a2, 88($sp)
    mtc0  $ra, $12
    bne   $k1, $ra, loc_00F8
    sw    $v0, 40($sp)
    ctc0  $sp, $8

loc_00F8:
    sw    $gp, 176($sp)
    cfc0  $gp, $13
    move  $k1, $sp
    bnez  $gp, loc_0138
    sw    $a3, 92($sp)
    lui   $sp, %hi(g_syscCtx)
    addiu $sp, $sp, %lo(g_syscCtx)
    cache 0x18, 0($sp)
    lui   $a3, %hi(intInfo)
    addiu $a2, $a3, 32

loop:
    lw    $a0, %lo(intInfo+ 0)($a3)
    lw    $at, %lo(intInfo+ 4)($a3)
    sw    $a0, %lo(intInfo+32)($a3)
    addiu $a3, $a3, 8
    bne   $a2, $a3, loop
    sw    $at, %lo(intInfo+28)($a3)

loc_0138:
    lui   $v0, 0xBC30
    lw    $v1, 24($v0)
    lw    $at, 40($v0)
    lw    $k0, 8($v0)
    lui   $a0, %hi(intInfo+64)
    ins   $v1, $at, 6, 2
    srl   $at, $at, 2
    ins   $v1, $at, 14, 2
    ins   $a0, $gp, 3, 2
    sw    $v1, %lo(intInfo+68)($a0)
    and   $a1, $ra, $a1
    andi  $v1, $a1, 0x9300
    sw    $k0, %lo(intInfo+64)($a0)
    bne   $0,  $v1, loc_01DC
    lui   $a0, %hi(intInfo+40)
    lw    $ra, 0($v0)
    lw    $v1, 16($v0)
    lw    $at, 32($v0)
    ins   $ra, $0,  0, 4
    ins   $v1, $at, 6, 2
    srl   $at, $at, 2
    ins   $v1, $at, 14, 2
    addiu $k0, $a0, 24
    move  $at, $0
    move  $a1, $0

loc_019C:
    lw    $a2, %lo(intInfo+40)($a0)
    lw    $a3, %lo(intInfo+44)($a0)
    and   $a2, $a2, $ra
    bne   $0, $a2, loc_0248
    and   $a3, $a3, $v1
    bne   $0, $a3, loc_0248
    addiu $a0, $a0, 8
    lw    $at, %lo(intInfo+32)($a0)
    bne   $a0, $k0, loc_019C
    lw    $a1, %lo(intInfo+36)($a0)

    sh    $0, 36($k1)
    or    $ra, $ra, $v1
    beqz  $ra, _InterruptManagerForKernel_551097F2
    move  $a0, $k1
    j     loc_00000978
    sw    $k1, 12($sp)

loc_01DC:
    bitrev $v1, $v1
    clz    $v1, $v1
    ext    $k0, $v1, 2, 1
    bnez   $k0, loc_0208
    ins    $v1, $k0, 1, 1
    mfc0   $a1, $13
    li     $at, 1
    sllv   $at, $at, $v1
    subu   $a1, $a1, $at
    mtc0   $a1, $13
    addiu  $v1, $v1, 4

loc_0208:
    lw    $at, %lo(intInfo+32)($a0)
    lw    $a1, %lo(intInfo+36)($a0)
    j     loc_0270
    addiu $a0, $v1, 52

loc_0218:
    cfc0  $v0, $2
    andi  $v0, $v0, 0x18
    bnezl $v0, loc_022C
    cfc0  $sp, $14
    addiu $sp, $sp, -320

loc_022C:
    lui   $v0, %hi(g_stackMiddle)
    addiu $v0, $v0, %lo(g_stackMiddle)
    sltu  $v0, $sp, $v0
    beqz  $v0, loc_0050
    ins   $sp, $0, 0, 6
    j     loc_00000914
    nop

loc_0248:
    movn   $a3, $a2, $a2
    bitrev $a3, $a3
    clz    $a0, $a3
    addiu  $ra, $a0, 32
    movz   $a0, $ra, $a2
    addiu  $v1, $a0, -30
    sltiu  $v1, $v1, 2
    beqz   $v1, loc_0270
    sllv   $v1, $v1, $a0
    sw     $v1, 0($v0)

loc_0270:
    ori   $ra, $at, 0xF
    ins   $ra, $ra, 29, 1
    sw    $ra, 8($v0)
    ext   $k0, $a1, 6, 2                                                                                 
    ext   $at, $a1, 14, 2                                                                                
    ins   $k0, $at, 2, 2
    sw    $k0, 40($v0)
    ins   $a1, $0, 6, 2
    ins   $a1, $0, 14, 2
    sw    $a1, 24($v0)
    lui   $k0, %hi(intInfo+128)
    ins   $k0, $a0, 6, 10
    lw    $v0, %lo(intInfo+128)($k0)
    addiu $k0, $k0, %lo(intInfo+128)
    sw    $k1, 12($sp)
    sltiu $ra, $v0, 2
    bnez  $ra, loc_00000978
    andi  $ra, $v0, 0x3
    addiu $gp, $gp, 1
    ctc0  $gp, $13
    beqz  $ra, loc_02F0
    sh    $ra, 36($k1)
    sw    $t0, 96($k1)
    sw    $t1, 100($k1)
    sw    $t2, 104($k1)
    sw    $t3, 108($k1)
    sw    $t4, 112($k1)
    sw    $t5, 116($k1)
    sw    $t6, 120($k1)
    sw    $t7, 124($k1)
    sw    $t8, 160($k1)
    sw    $t9, 164($k1)

loc_02F0:
    lw    $a1, 8($k0)
    lw    $a2, 56($k1)

mod_02F8:
    nop

    move  $k1, $0
    slti  $ra, $a0, 64
    mtic  $ra, $0                                                                                      
    jalr  $v0
    lw    $gp, 4($k0)
    mtic  $0, $0

loc_00000314:
    lw    $a0, 48($k0)
    bltz  $a0, loc_03A4
    addiu $ra, $v0, 1
    beqz  $ra, loc_03A4
    addiu $ra, $v0, 2
    beqz  $ra, loc_036C
    lbu   $a0, 50($k0)
    lw    $k0, 40($k0)
    lw    $k1, 12($sp)
    sll   $v0, $v0, 6
    addu  $k0, $k0, $v0
    j     loc_02F0
    lw    $v0, 0($k0)

loc_0348:
    sll   $v0, $v0, 8
    ext   $v1, $v0, 10, 1
    ins   $v0, $v1, 12, 1
    ins   $v0, $0, 10, 1
    not   $v0, $v0
    mfc0  $v1, $12
    and   $v1, $v1, $v0
    j     loc_03A4
    mtc0  $v1, $12

loc_036C:
    li    $v0, 1
    addiu $a1, $a0, -64
    bgez  $a1, loc_0348
    sllv  $v0, $v0, $a0
    ext   $a1, $a0, 5, 1
    lui   $a2, %hi(intInfo+96)
    ins   $a2, $a1, 2, 1
    not   $v0, $v0
    addiu $a3, $a2, -96

loc_0390:
    lw     $a0, %lo(intInfo+88)($a2)
    addiu  $a2, $a2, -8
    and    $a0, $a0, $v0
    bne    $a2, $a3, loc_0390
    sw     $a0, %lo(intInfo+96)($a2)

loc_03A4:
    cfc0  $v1, $13
    lui   $ra, %hi(intInfo+64)
    addiu $ra, $ra, %lo(intInfo+64)
    addiu $v1, $v1, -1
    ctc0  $v1, $13
    addiu $k1, $ra, -40
    ins   $ra, $v1, 3, 2
    movz  $ra, $k1, $v1
    lw    $a0, 0($ra)
    lw    $a1, 4($ra)
    lui   $v0, 0xBC30
    ori   $a0, $a0, 0xF
    ins   $a0, $a0, 29, 1
    sw    $a0, 8($v0)
    ext   $a0, $a1, 6, 2
    ext   $a2, $a1, 14, 2
    ins   $a0, $a2, 2, 2
    sw    $a0, 40($v0)
    ins   $a1, $0, 6, 2
    ins   $a1, $0, 14, 2
    sw    $a1, 24($v0)
    bnez  $v1, loc_0478
    lw    $a0, 12($sp)

    .globl mod_0400
mod_0400:
    jal   sub_091C
    li    $k1, 3
    beqz  $v0, _InterruptManagerForKernel_551097F2
    lw    $a0, 12($sp)
    lb    $v0, 36($a0)
    bnez  $v0, loc_0444
    sh    $k1, 36($a0)
    sw    $t0, 96($a0)
    sw    $t1, 100($a0)
    sw    $t2, 104($a0)
    sw    $t3, 108($a0)
    sw    $t4, 112($a0)
    sw    $t5, 116($a0)
    sw    $t6, 120($a0)
    sw    $t7, 124($a0)
    sw    $t8, 160($a0)
    sw    $t9, 164($a0)

loc_0444:
    sw    $s0, 128($a0)                                                                                
    sw    $s1, 132($a0)
    sw    $s2, 136($a0)
    sw    $s3, 140($a0)
    sw    $s4, 144($a0)
    sw    $s5, 148($a0)
    sw    $s6, 152($a0)
    sw    $s7, 156($a0)
    sw    $fp, 184($a0)

    .globl mod_0468
mod_0468:
    jal   sub_091C

    move  $a1, $0
    j     _InterruptManagerForKernel_551097F2
    move  $a0, $v0

loc_0478:
    lb    $a2, 37($a0)
    beqz  $a2, _InterruptManagerForKernel_551097F2
    mfc0  $k0, $12
    ins   $k0, $a2, 29, 1
    mtc0  $k0, $12
    ctc0  $a0, $8
    lwc1  $0, 192($a0)
    lwc1  $1, 196($a0)
    lwc1  $2, 200($a0)
    lwc1  $3, 204($a0)
    lwc1  $4, 208($a0)
    lwc1  $5, 212($a0)
    lwc1  $6, 216($a0)
    lwc1  $7, 220($a0)
    lwc1  $8, 224($a0)
    lwc1  $9, 228($a0)
    lwc1  $10, 232($a0)
    lwc1  $11, 236($a0)
    lwc1  $12, 240($a0)
    lwc1  $13, 244($a0)
    lwc1  $14, 248($a0)
    lwc1  $15, 252($a0)
    lwc1  $16, 256($a0)
    lwc1  $17, 260($a0)
    lwc1  $18, 264($a0)
    lwc1  $19, 268($a0)
    lwc1  $20, 272($a0)
    lwc1  $21, 276($a0)
    lwc1  $22, 280($a0)
    lwc1  $23, 284($a0)
    lwc1  $24, 288($a0)
    lwc1  $25, 292($a0)
    lwc1  $26, 296($a0)
    lwc1  $27, 300($a0)
    lwc1  $28, 304($a0)
    lwc1  $29, 308($a0)
    lwc1  $30, 312($a0)
    lwc1  $31, 316($a0)

_InterruptManagerForKernel_551097F2:
    .globl InterruptManagerForKernel_551097F2
InterruptManagerForKernel_551097F2:
    lw    $sp, 180($a0)
    lb    $a2, 36($a0)
    lw    $k0, 60($a0)
    lw    $k1, 52($a0)
    mfc0  $v0, $12
    ctc0  $k0, $23
    xor   $k1, $k1, $v0
    ins   $k1, $0, 8, 8
    ins   $k1, $0, 19, 1
    xor   $k1, $k1, $v0
    lw    $k0, 40($a0)
    lui   $v0, 0x2000
    or    $v0, $v0, $k1
    mtc0  $v0, $12
    ctc1  $0, $31
    ctc1  $k0, $31
    cfc0  $k0, $8
    xor   $k0, $k0, $a0
    sltiu $k0, $k0, 1
    ins   $k1, $k0, 29, 1
    lw    $ra, 188($a0)
    andi  $a1, $a2, 0x1
    beqz  $a1, loc_0598
    lw    $gp, 176($a0)
    lw    $s0, 128($a0)
    lw    $s1, 132($a0)
    lw    $s2, 136($a0)
    lw    $s3, 140($a0)
    lw    $s4, 144($a0)
    lw    $s5, 148($a0)
    lw    $s6, 152($a0)
    lw    $s7, 156($a0)
    beq   $a1, $a2, loc_0650
    lw    $fp, 184($a0)

loc_0598:
    beqz  $a2, loc_05C8
    lw    $at, 68($a0)
    lw    $t0, 96($a0)
    lw    $t1, 100($a0)
    lw    $t2, 104($a0)
    lw    $t3, 108($a0)
    lw    $t4, 112($a0)                                                                                
    lw    $t5, 116($a0)
    lw    $t6, 120($a0)
    lw    $t7, 124($a0)
    lw    $t8, 160($a0)
    lw    $t9, 164($a0)

loc_05C8:
    lw    $v0, 44($a0)
    lw    $v1, 48($a0)
    lw    $k0, 56($a0)
    mthi  $v0
    mtlo  $v1
    lw    $v0, 72($a0)
    lw    $v1, 76($a0)
    lw    $a1, 84($a0)
    lw    $a2, 88($a0)
    lw    $a3, 92($a0)
    ori   $k1, $k1, 0x2
    mtc0  $k1, $12
    mtc0  $k0, $14
    cfc0  $k1, $25
    beqz  $k1, loc_062C
    nop
    lw    $k0, 8($k1)
    beqz  $k0, loc_062C
    addiu $k0, $k0, -1
    bnez  $k0, loc_062C
    sw    $k0, 8($k1)
    lw    $k0, 4($k1)
    lw    $k1, 0($k1)
    sw    $k0, 0($k1)
    sync

loc_062C:
    lb    $k0, 37($a0)
    beqz  $k0, loc_0640
    cfc0  $k0, $13
    bnezl $k0, loc_0640
    ctc0  $0, $8

loc_0640:
    lw    $k0, 168($a0)
    lw    $k1, 172($a0)
    lw    $a0, 80($a0)
    eret

loc_0650:
    mtc0  $k1, $12
    lw    $k0, 168($a0)
    lw    $k1, 172($a0)
    jr    $ra
    li    $v0, 1

    .globl sceKernelIsIntrContext
sceKernelIsIntrContext:
    jr   $ra
    cfc0 $v0, $13

    .globl SaveThreadContext
SaveThreadContext:
    cache 0x18, 52($a0)
    cache 0x18, 128($a0)
    sw    $s0, 128($a0)
    sw    $s1, 132($a0)
    sw    $s2, 136($a0)
    sw    $s3, 140($a0)
    sw    $s4, 144($a0)
    sw    $s5, 148($a0)
    sw    $s6, 152($a0)
    sw    $s7, 156($a0)
    sw    $k0, 168($a0)
    sw    $k1, 172($a0)
    sw    $gp, 176($a0)
    sw    $sp, 180($a0)
    sw    $fp, 184($a0)
    sw    $ra, 188($a0)
    cfc0  $t4, $23
    li    $t5, 1
    sh    $t5, 36($a0)
    mfc0  $v1, $12
    lui   $v0, 0x2000
    or    $v0, $v0, $v1
    sw    $t4, 60($a0)
    mtc0  $v0, $12
    li    $v0, 0
    sw    $v1, 52($a0)
    cfc1  $t4, $31
    mtc0  $v1, $12
    jr    $ra
    sw    $t4, 40($a0)

    .globl sceKernelCallUserIntrHandler
sceKernelCallUserIntrHandler:
    mfc0       $t2, $12
    mtic       $0, $0
    lui        $t3, %hi(intInfo+112)
    lw         $t3, %lo(intInfo+112)
    addiu      $t1, $t1, -32
    addiu      $t3, $t3, -7552
    sltu       $v0, $t1, $t3
    bnez       $v0, loc_00000914
    lui        $v1, %hi(intInfo+97)
    lbu        $v0, %lo(intInfo+97)
    cache      0x18, -64($sp)
    move       $t3, $sp
    addiu      $v0, $v0, 1
    sb         $v0, %lo(intInfo+97)
    addiu      $sp, $sp, -400
    ins        $sp, $0, 0, 6
    cfc0       $v0, $15
    sw         $v0, 376($sp)
    cfc0       $v0, $14
    sw         $v0, 380($sp)
    cfc0       $v0, $16
    addiu      $v1, $sp, 388
    sw         $0, 0($v1)
    sw         $v0, 384($sp)
    ctc0       $v1, $16
    sw         $t3, 372($sp)
    lui        $at, 0xDEAD
    ori        $at, $at, 0xBEEF
    lui        $t4, %hi(intInfo+116)
    lw         $t4, %lo(intInfo+116)
    beqz       $t4, loc_076C
    move       $t5, $0
    lw         $t5, 0($t4)
    sw         $0, 0($t4)

loc_076C:
    sw         $t5, 392($sp)
    sw         $s0, 320($sp)
    move       $s0, $at
    sw         $s1, 324($sp)
    move       $s1, $at
    ext        $t3, $t2, 29, 1
    sw         $s2, 328($sp)
    move       $s2, $at
    cfc0       $t9, $23
    sw         $s3, 332($sp)
    move       $s3, $at                                                                                     
    beqz       $t3, loc_07AC
    sw         $s4, 336($sp)
    ctc1       $0, $31
    li         $t4, 3584
    ctc1       $t4, $31

loc_07AC:
    sw         $s5, 340($sp)
    move       $s5, $at
    move       $s4, $at
    sw         $s6, 344($sp)
    move       $s6, $at
    move       $v0, $at
    sw         $s7, 348($sp)
    move       $s7, $at
    move       $v1, $at
    sw         $k0, 352($sp)
    move       $k0, $0
    move       $t4, $at
    sw         $t9, 368($sp)
    move       $k1, $0
    move       $t5, $at
    sw         $fp, 356($sp)
    move       $fp, $at
    move       $t6, $at
    sw         $ra, 360($sp)
    move       $t8, $at
    move       $t7, $at
    sw         $t2, 364($sp)
    li         $t3, 140
    addiu      $ra, $t1, 16
    sw         $t3, 16($t1)
    mtlo       $at
    mthi       $at
    sw         $0, 20($t1)
    ctc0       $sp, $15
    ctc0       $sp, $14
    sw         $0, 24($t1)
    move       $t9, $at
    sw         $0, 28($t1)
    cache      0x8, 16($t1)
    cache      0x1A, 16($t1)
    lui        $t3, 0x2FFF                                                                                  
    ori        $t3, $t3, 0xFFE3
    and        $t2, $t2, $t3
    ori        $t2, $t2, 0x13
    mtc0       $t2, $12
    move       $t2, $at
    move       $t3, $at
    mtc0       $t0, $14
    sync
    move       $sp, $t1
    eret

    .globl sub_0864
sub_0864:
    cfc0       $t4, $13
    beqz       $t4, loc_00000914
    cfc0       $v0, $4
    cfc0       $sp, $15
    lui        $a1, %hi(intInfo+97)
    lbu        $a0, %lo(intInfo+97)
    lw         $t4, 364($sp)
    mfc0       $t5, $12
    lw         $t8, 368($sp)
    lw         $ra, 360($sp)
    lw         $s0, 320($sp)
    lw         $s1, 324($sp)
    lw         $s2, 328($sp)
    lw         $s3, 332($sp)
    lw         $s4, 336($sp)
    lw         $s5, 340($sp)
    lw         $s6, 344($sp)
    lw         $s7, 348($sp)
    lw         $k0, 352($sp)                                                                                
    lw         $fp, 356($sp)
    addiu      $a0, $a0, -1
    sb         $a0, %lo(intInfo+97)
    ctc0       $t8, $23
    move       $k1, $0
    lui        $t0, %hi(intInfo+116)
    lw         $t0, %lo(intInfo+116)
    beqz       $t0, loc_08D8
    lw         $t1, 392($sp)
    sw         $t1, 0($t0)

loc_08D8:
    lw         $t0, 376($sp)
    lw         $t1, 380($sp)
    lw         $t2, 384($sp)
    ctc0       $t0, $15
    ctc0       $t1, $14
    ctc0       $t2, $16
    lui        $at, 0xDFF7
    ori        $at, $at, 0xFF
    not        $t6, $at
    and        $t5, $t5, $t6
    and        $t4, $t4, $at
    or         $t4, $t4, $t5
    mtc0       $t4, $12
    jr         $ra
    lw         $sp, 372($sp)

loc_00000914:
    j          loc_00000914
    break      0x003, 0x3FF  # (0x00FFF)

    .globl sub_091C
sub_091C:
    jr    $ra
    move  $v0, $0

    .globl intrExcepHandler2
intrExcepHandler2:
    nop
    nop
    cfc0       $v1, $25
    beqz       $v1, loc_0000095C
    nop
    lw         $v0, 8($v1)
    beqz       $v0, loc_0000095C
    addiu      $v0, $v0, -1
    bnez       $v0, loc_0000095C
    sw         $v0, 8($v1)
    lw         $v0, 4($v1)
    lw         $v1, 0($v1)
    sw         $v0, 0($v1)
    sync

loc_0000095C:
    cfc0       $v1, $0                                                                                        
    mtc0       $v1, $14                                                                                       
    cfc0       $v1, $2                                                                                        
    mtc0       $v1, $12
    cfc0       $v0, $4
    cfc0       $v1, $5
    eret

loc_00000978:
    move       $sp, $k1
    lui        $v1, %hi(intrExcepHandler)
    lw         $v1, %lo(intrExcepHandler)($v1)
    lui        $at, %hi(intInfo+108)
    sw         $v1, %lo(intInfo+108)($at)
    sll        $v0, $gp, 3
    lui        $v1, %hi(intInfo+64)
    addiu      $v1, $v1, %lo(intInfo+64)
    addu       $v1, $v1, $v0
    lw         $a0, 0($v1)
    lw         $a1, 4($v1)
    lui        $v0, 0xBC30
    ori        $a0, $a0, 0xF
    ins        $a0, $a0, 29, 1
    sw         $a0, 8($v0)
    ext        $a0, $a1, 6, 2
    ext        $a2, $a1, 14, 2
    ins        $a0, $a2, 2, 2
    ins        $a1, $0, 6, 2
    ins        $a1, $0, 14, 2
    sw         $a1, 24($v0)
    sw         $a0, 40($v0)
    lw         $a1, 64($sp)
    lw         $a2, 56($sp)
    lw         $a3, 60($sp)
    ctc0       $a1, $3
    ctc0       $a2, $0
    ctc0       $a3, $23
    lw         $a0, 40($sp)
    mfc0       $v1, $12
    lui        $v0, 0x2000
    or         $v0, $v0, $v1
    mtc0       $v0, $12
    ctc1       $a0, $31
    lw         $a0, 52($sp)
    lui        $v0, 0xFFF7
    ori        $v0, $v0, 0xFF
    and        $a0, $a0, $v0
    nor        $v0, $0, $v0
    and        $v0, $v0, $v1
    or         $a0, $a0, $v0
    ctc0       $a0, $2
    lw         $at, 68($sp)
    lw         $a0, 80($sp)
    lw         $a1, 84($sp)
    lw         $a2, 88($sp)
    lw         $a3, 92($sp)
    lw         $k0, 168($sp)
    lw         $k1, 172($sp)
    lw         $gp, 176($sp)
    lw         $ra, 188($sp)
    lw         $v0, 44($sp)
    lw         $v1, 48($sp)
    mthi       $v0
    mtlo       $v1
    lw         $v0, 72($sp)
    lw         $v1, 76($sp)
    ctc0       $v0, $4
    ctc0       $v1, $5
    lui        $v0, %hi(intInfo+108)
    lw         $v0, %lo(intInfo+108)
    jr         $v0
    lw         $sp, 180($sp)

##
# sub_0A74
##

sub_0A74:
    cache      0x18, -64($sp)
    addiu      $sp, $sp, -64
    lh         $a3, 48($k0)
    bgez       $a3, loc_00000B4C
    nop

loc_00000A88:
    sw         $a0, 0($sp)
    sw         $a1, 4($sp)
    sw         $a2, 8($sp)
    move       $a3, $v0
    sw         $v0, 12($sp)
    sw         $t0, 16($sp)
    sw         $t1, 20($sp)
    sw         $t2, 24($sp)
    sw         $t3, 28($sp)
    sw         $t4, 32($sp)
    sw         $t5, 36($sp)
    sw         $t6, 40($sp)
    sw         $t7, 44($sp)
    sw         $t8, 48($sp)
    sw         $t9, 52($sp)

mod_0AC4:
    nop

    cfc0       $t0, $13
    lw         $a0, 0($sp)
    lw         $a1, 4($sp)
    lw         $a2, 8($sp)
    lw         $v0, 12($sp)
    slti       $ra, $a0, 64
    mtic       $ra, $0
    jalr       $v0
    lw         $gp, 4($k0)
    mtic       $0, $0
    lw         $a0, 0($sp)
    lw         $a1, 4($sp)
    lw         $a2, 8($sp)
    move       $a3, $v0
    cfc0       $t0, $13

mod_0B04:
    nop

    sw         $v0, 0($sp)
    lh         $v0, 48($k0)
    bgez       $v0, loc_00000B70
    nop

loc_00000B18:
    lw         $v0, 0($sp)
    lw         $t0, 16($sp)
    lw         $t1, 20($sp)
    lw         $t2, 24($sp)
    lw         $t3, 28($sp)
    lw         $t4, 32($sp)
    lw         $t5, 36($sp)
    lw         $t6, 40($sp)
    lw         $t7, 44($sp)
    lw         $t8, 48($sp)                                                                                 
    lw         $t9, 52($sp)
    j          loc_00000314
    addiu      $sp, $sp, 64

loc_00000B4C:
    lui        $a3, %hi(intInfo+120)
    lw         $gp, %lo(intInfo+120)($a3)
    mfc0       $ra, $9
    lw         $v1, %lo(intInfo+124)($a3)
    sltu       $a3, $ra, $gp
    addu       $v1, $v1, $a3
    sw         $ra, 64($sp)
    j          loc_00000A88
    sw         $v1, 68($sp)

loc_00000B70:
    lui        $a3, %hi(intInfo+120)
    lw         $a2, %lo(intInfo+120)($a3)
    mfc0       $v0, $9
    lw         $v1, %lo(intInfo+124)($a3)
    sltu       $a2, $v0, $a2
    addu       $v1, $v1, $a2
    lw         $a0, 64($sp)
    lw         $a1, 68($sp)
    subu       $v1, $v1, $a1
    sltu       $a1, $v0, $a0
    subu       $v0, $v0, $a0
    subu       $v1, $v1, $a1
    lw         $a0, 16($k0)
    lw         $a1, 20($k0)
    sltu       $at, $a1, $v1
    bnez       $at, loc_00000BD0
    nop
    bne        $a1, $v1, loc_00000BC8
    nop
    sltu       $at, $a0, $v0
    bnez       $at, loc_00000BD0
    nop                                                                                                     

loc_00000BC8:
    sw         $v0, 16($k0)
    sw         $v1, 20($k0)

loc_00000BD0:
    lw         $a0, 24($k0)
    lw         $a1, 28($k0)
    sltu       $at, $v1, $a1
    bnez       $at, loc_00000C00
    nop
    bne        $v1, $a1, loc_00000BF8
    nop
    sltu       $at, $v0, $a0
    bnez       $at, loc_00000C00
    nop

loc_00000BF8:
    sw         $v0, 24($k0)
    sw         $v1, 28($k0)

loc_00000C00:
    lw         $a0, 32($k0)
    lw         $a1, 36($k0)
    addu       $a0, $a0, $v0
    sltu       $v0, $a0, $v0
    addu       $a1, $a1, $v1
    addu       $a1, $a1, $v0
    sw         $a0, 32($k0)                                                                                 
    sw         $a1, 36($k0)
    lw         $a2, 12($k0)
    lw         $a0, 48($k0)
    addiu      $a2, $a2, 1
    j          loc_00000B18
    sw         $a2, 12($k0)

    .globl InterruptManagerForKernel_A6978B70
InterruptManagerForKernel_A6978B70:
    lui        $t0, %hi(sub_0A74)
    addiu      $t0, $t0, %lo(sub_0A74)
    ext        $a0, $a0, 2, 26
    ext        $a1, $a1, 2, 26
    ext        $t0, $t0, 2, 26
    lui        $t1, 0xC00
    or         $a0, $a0, $t1
    or         $a1, $a1, $t1
    lui        $t1, 0xA00
    or         $t0, $t0, $t1
    lui        $at, %hi(mod_0AC4)
    sw         $a0, %lo(mod_0AC4)($at)
    cache      0x1B, %lo(mod_0AC4)($at)
    cache      0x8, %lo(mod_0AC4)($at)
    lui        $at, %hi(mod_0B04)
    sw         $a1, %lo(mod_0B04)($at)
    cache      0x1B, %lo(mod_0B04)($at)
    cache      0x8, %lo(mod_0B04)($at)
    lui        $at, %hi(mod_02F8)
    sw         $t0, %lo(mod_02F8)($at)
    cache      0x1B, %lo(mod_02F8)($at)
    cache      0x8, %lo(mod_02F8)($at)
    jr         $ra
    sync

    .globl InterruptManagerForKernel_1373F20E
InterruptManagerForKernel_1373F20E:
    lui        $at, %hi(mod_02F8)
    sw         $0, %lo(mod_02F8)
    cache      0x1B, %lo(mod_02F8)
    cache      0x8, %lo(mod_02F8)
    jr         $ra
    sync

    nop # probably alignment
    nop
    nop
    nop
    nop

_sub_0CC0:
    .globl sub_0CC0
sub_0CC0:
    cfc0       $v0, $0
    mtc0       $v0, $14
    lui        $v0, %hi(syscallExcepHandler)
    lw         $v0, %lo(syscallExcepHandler)
    jr         $v0
    nop

    .globl sceKernelGetSyscallRA
sceKernelGetSyscallRA:
    cfc0       $v0, $16
    beqz       $v0, loc_00000CF0
    nop
    lw         $v0, 0($v0)
    bnezl      $v0, loc_00000CF0
    lw         $v0, 12($v0)

loc_00000CF0:
    jr         $ra
    nop

    .globl syscallExcepHandler
syscallExcepHandler:
    nop
    nop
    cfc0       $v0, $0
    cfc0       $v1, $3
    mfc0       $t7, $21
    addiu      $v0, $v0, 4
    slti       $at, $t7, 256
    beqz       $at, loc_00000D34
    ctc0       $t7, $23
    bltz       $v1, loc_00000E98
    lui        $t4, %hi(g_syscallTable + 0x20000010)
    addu       $t4, $t4, $t7
    lw         $t7, %lo(g_syscallTable + 0x10)($t4)
    jr         $t7
    mtc0       $v0, $14

loc_00000D34:
    cfc0       $t4, $12
    srl        $v1, $v1, 31
    movn       $v0, $ra, $v1

loc_00000D40:
    lw         $t5, 4($t4)
    mtc0       $v0, $14
    lw         $t6, 8($t4)
    beqz       $t5, _sub_0CC0
    subu       $t5, $t7, $t5
    sltu       $at, $t5, $t6
    beqzl      $at, loc_00000D40
    lw         $t4, 0($t4)                                                                                  
    addu       $t4, $t4, $t5
    lw         $t7, 16($t4)
    cfc0       $t5, $2
    ext        $v1, $t5, 4, 1
    beqz       $v1, loc_00000E98
    cfc0       $t4, $15
    bgez       $t7, loc_00000E98
    sw         $sp, -24($t4)
    addiu      $sp, $t4, -32
    cfc0       $t6, $4
    sw         $t6, 24($sp)
    li         $t4, 3584
    cfc0       $at, $14
    sw         $t5, 0($sp)
    addiu      $t6, $t5, -18
    ins        $t5, $v1, 29, 1
    sw         $t4, 40($at)
    mtc0       $t5, $12
    cfc1       $t5, $31
    sw         $t5, 20($sp)
    ctc1       $0, $31
    sw         $k1, 16($sp)
    cfc0       $k1, $16
    sw         $v0, 4($sp)
    ctc1       $t4, $31
    sw         $sp, 0($k1)

    .globl mod_0DC4
mod_0DC4:
    nop

    sw         $ra, 12($sp)
    mtc0       $t6, $12
    jalr       $t7
    lui        $k1, 0x10
    mtic       $0, $0

    .globl loc_00000DDC
loc_00000DDC:
    lui        $t7, 0xCCCC
    ori        $t7, $t7, 0xCCCC
    lui        $t6, 0xBC00
    sw         $t7, 0($t6)
    sw         $t7, 4($t6)
    lw         $t0, 0($sp)
    lw         $t1, 4($sp)
    lw         $k1, 16($sp)
    lw         $ra, 12($sp)
    mfc0       $t4, $12
    mtc0       $t1, $14
    lui        $t9, 0x2008
    ori        $t9, $t9, 0xFF00
    cfc0       $t1, $14
    lw         $t2, 20($sp)
    xor        $t3, $t0, $t4
    and        $t3, $t9, $t3
    xor        $t0, $t3, $t0
    ins        $t9, $t0, 0, 29
    mtc0       $t9, $12
    sw         $t2, 40($t1)
    ctc1       $0, $31
    ctc1       $t2, $31
    cfc0       $t9, $16
    sw         $0, 0($t9)
    mtc0       $t0, $12
    lw         $sp, 8($sp)

    .globl mod_0E48
mod_0E48:
    lui        $at, 0xDEAD

    ori        $at, $at, 0xBEEF

    .globl mod_0E50
mod_0E50:
    move       $t3, $at

    move       $t4, $at
    move       $t9, $at
    move       $t2, $at
    mtlo       $at
    mthi       $at
    move       $t0, $at
    move       $t1, $at
    move       $a0, $at
    move       $a1, $at
    move       $a2, $at
    move       $a3, $at
    move       $t5, $at
    move       $t6, $at
    move       $t7, $at
    move       $t8, $at
    eret

loc_00000E98:
    j          loc_00000E98
    break      0x003, 0x3FE  # (0x00FFE)

    # OHNOES alignment :O :O
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    nop

##
# Mini-funcs
##

    .globl sub_0EC0
sub_0EC0:
    mfc0       $v0, $12
    ext        $v0, $v0, 30, 1
    eret

    .globl sub_0ECC
sub_0ECC:
    mfic       $v0, $0
    mtic       $0, $0
    nop
    nop
    nop
    nop
    nop
    nop                                                                                                        
    nop                                                                                                        
    nop
    eret

    .globl sub_0EF8
sub_0EF8:
    mtic       $a0, $0
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    eret

    .globl sub_0F20
sub_0F20:
    sync
    nop
    sync
    nop
    mtic       $a0, $0
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    eret

    .globl sub_0F58
sub_0F58:
    xori       $v0, $a0, 0x1
    andi       $v0, $v0, 0x1
    eret

    .globl sub_0F64
sub_0F64:
    mfic       $v0, $0
    mtic       $0, $0
    nop                                                                                                     
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    mfc0       $a0, $12
    lui        $a1, 0x40
    or         $a0, $a0, $a1
    mtc0       $a0, $12
    eret
    nop

    .globl sub_0FA4
sub_0FA4:
    mfc0       $v0, $12
    lui        $v1, 0xFFBF
    ori        $v1, $v1, 0xFFFF
    and        $v0, $v0, $v1
    mtc0       $v0, $12
    nop
    mtic       $a0, $0
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    eret

    .globl sub_0FE4
sub_0FE4:
    cfc0       $v0, $13
    eret
    nop

    .globl sub_0FF0
sub_0FF0:
    mfic       $v0, $0
    eret

    .globl sub_0FF8
sub_0FF8:
    mfc0       $v0, $9
    eret

    .globl sub_1000
sub_1000:
    mfc0       $v0, $15
    eret

    .globl sub_1008
sub_1008:
    mfc0       $v0, $22
    eret                                                                                                    

    .globl sub_1010
sub_1010:
    mfc0       $v0, $12
    andi       $v0, $v0, 0x18
    slti       $v0, $v0, 1
    eret

    .globl sub_1020
sub_1020:
    mtc0       $ra, $14
    lui        $v0, 0x8002
    addiu      $v0, $v0, 314
    eret

    .globl sub_1030
sub_1030:
    lui        $at, 0xBC30
    lui        $v0, 0x2000
    ori        $v0, $v0, 0xF
    sw         $v0, 8($at)
    sw         $0, 24($at)
    sw         $0, 40($at)
    jr         $ra
    sync

    .globl sub_1050
sub_1050:
    lui        $at, 0xBC30
    lw         $v0, 4($at)
    lw         $v1, 20($at)
    lw         $at, 36($at)
    ins        $v0, $0, 0, 4
    ins        $v0, $0, 29, 1
    sw         $v0, 0($a0)
    ins        $v1, $at, 6, 2
    srl        $at, $at, 2
    ins        $v1, $at, 14, 2
    jr         $ra
    sw         $v1, 4($a0)

    .globl sub_1080
sub_1080:
    lui        $at, 0xBC30
    lw         $v0, 8($at)
    lw         $v1, 24($at)
    lw         $at, 40($at)
    sw         $v0, 0($a0)
    ins        $v1, $at, 6, 2
    srl        $at, $at, 2
    ins        $v1, $at, 14, 2
    jr         $ra
    sw         $v1, 4($a0)

    .globl sub_10A8
sub_10A8:
    lw         $v0, 0($a0)
    lw         $v1, 4($a0)
    lui        $at, 0x2000
    ori        $at, $at, 0xF
    or         $v0, $v0, $at
    lui        $at, 0xBC30
    sw         $v0, 8($at)
    ext        $v0, $v1, 6, 2
    ext        $a0, $v1, 14, 2
    ins        $v0, $a0, 2, 2
    ins        $v1, $0, 6, 2
    ins        $v1, $0, 14, 2
    sw         $v1, 24($at)
    sw         $v0, 40($at)
    jr         $ra
    sync

