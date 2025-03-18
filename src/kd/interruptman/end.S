# Copyright (C) 2011, 2012 The uOFW team
# See the file COPYING for copying permission.

#include "common_asm.h"

    .globl sceKernelCpuSuspendIntr
sceKernelCpuSuspendIntr:
    mfic $v0, $0
    mtic $zr, $0
    beqz $v0, loc_3164

loc_3154:
    nop
    nop
    nop
    nop
loc_3164:
    jr $ra
    nop

    .globl sceKernelCpuResumeIntrWithSync
sceKernelCpuResumeIntrWithSync:
    sync
    nop

    .globl sceKernelCpuResumeIntr
sceKernelCpuResumeIntr:
    mtic $a0, $0
    j loc_3154
    nop

sub_3230:
    addiu      $sp, $sp, -64
    sw         $t7, 8($sp)
    addiu      $t4, $sp, 64
    sw         $t6, 12($sp)
    mfc0       $t7, $21
    sw         $t4, 4($sp)
    srl        $t7, $t7, 2
    sw         $t7, 0($sp)
    ins        $t6, $0, 0, 1
    mtc0       $t6, $12
    sw         $a0, 16($sp)
    sw         $a1, 20($sp)
    sw         $a2, 24($sp)
    sw         $a3, 28($sp)
    sw         $t0, 32($sp)
    sw         $t1, 36($sp)
    sw         $t2, 40($sp)
    sw         $t3, 44($sp)
    sw         $t7, 48($sp)

mod_327C:
    nop

    sw         $t4, 52($sp)
    lw         $t7, 8($sp)
    lw         $t6, 12($sp)
    lw         $a0, 16($sp)
    lw         $a1, 20($sp)
    lw         $a2, 24($sp)
    lw         $a3, 28($sp)
    lw         $t0, 32($sp)
    lw         $t1, 36($sp)
    lw         $t2, 40($sp)
    lw         $t3, 44($sp)
    addiu      $sp, $sp, 48
    mtc0       $t6, $12
    jalr       $t7
    lui        $k1, 0x10
    mtic       $0, $0
    move       $a2, $v0
    move       $a3, $v1
    lw         $a0, 0($sp)
    lw         $a1, 4($sp)
    sw         $v0, 8($sp)

mod_32D4:
    nop

    sw         $v1, 12($sp)
    lw         $v0, 8($sp)
    lw         $v1, 12($sp)
    j          loc_00000DDC
    addiu      $sp, $sp, 16

    .globl InterruptManagerForKernel_2F3C4DCD
InterruptManagerForKernel_2F3C4DCD:
    lui        $t0, %hi(sub_3230)
    addiu      $t0, $t0, %lo(sub_3230)
    ext        $a0, $a0, 2, 26
    ext        $a1, $a1, 2, 26
    ext        $t0, $t0, 2, 26
    lui        $t1, 0xC00
    or         $a0, $a0, $t1
    or         $a1, $a1, $t1
    lui        $t1, 0xA00
    or         $t0, $t0, $t1
    lui        $at, %hi(mod_327C)
    sw         $a0, %lo(mod_327C)($at)
    cache      0x1B, %lo(mod_327C)($at)
    cache      0x8, %lo(mod_327C)($at)
    lui        $at, %hi(mod_32D4)
    sw         $a1, %lo(mod_32D4)($at)
    cache      0x1B, %lo(mod_32D4)($at)
    cache      0x8, %lo(mod_32D4)($at)
    lui        $at, %hi(mod_0DC4)
    sw         $t0, %lo(mod_0DC4)($at)
    cache      0x1B, %lo(mod_0DC4)($at)
    cache      0x8, %lo(mod_0DC4)($at)
    jr         $ra
    sync

    .globl InterruptManagerForKernel_02AC50D5
InterruptManagerForKernel_02AC50D5:
    lui        $at, %hi(mod_0DC4)
    sw         $0, %lo(mod_0DC4)($at)
    cache      0x1B, %lo(mod_0DC4)($at)                                                                         
    cache      0x8, %lo(mod_0DC4)($at)
    jr         $ra
    sync

