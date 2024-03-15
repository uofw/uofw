# Copyright (C) The uOFW team
# See the file COPYING for copying permission.

#include <common_asm.h>

    .text

    # Below are helper variables introduced to easily access specific g_PowerFreq fields
    .global g_pBusClockFrequencyInt # pointer to g_PowerFreq.busClockFrequencyInt
    .global g_pGeEdramRefreshMode # pointer to g_PowerFreq.geEdramRefreshMode

# uofw note: The ASM of this fucntion seems to use some non-trivial arithmetic optimizations.
# I wrote a C-equivalent version of the ASM code and checked our compiler output with the
# original ASM. I am not confident that they are functionally identical.
# As such, until I have "demystified" these arithmetic optimizations in C, I am feeling more
# confident to just use the original ASM here.
#
# You can find the attempted C-equivalent function in clockFrequency.c (function commented out).
#
# I have modified the existing ASM code to be able to access both g_PowerFreq.busClockFrequencyInt and
# g_PowerFreq.geEdramRefreshMode in the ASM without having to bother about their actual offset in the 
# ScePowerFreq struct. Where in the original ASM these fields are accessed directly, I am using another
# level of indirection here (specifying a _pointer_ to those fields) to access them. As such, I have added
# two additional [lw] instructions in the code and changed an [sw] offset.

    .global scePowerSetGeEdramRefreshMode
#s32 scePowerSetGeEdramRefreshMode(s32 geEdramRefreshMode)
scePowerSetGeEdramRefreshMode:
    addiu      $sp, $sp, -16
    sw         $s2, 8($sp)
    sw         $s1, 4($sp)
    move       $s1, $a0 # $s1 = geEdramRefreshMode
    sw         $ra, 12($sp)
    jal        sceKernelCpuSuspendIntr # sceKernelCpuSuspendIntr()
    sw         $s0, 0($sp)
    move       $s2, $v0 # $s2 = sceKernelCpuSuspendIntr() -- s32 intrState;
    li         $a3, 6 # refreshParam3 = 6
    li         $a1, 1 # refreshParam1 = 1
    bnez       $s1, loc_00003ECC # if geEdramRefreshMode != 0
    li         $a2, 8 # refreshParam2 = 2
    lui        $v1, %hi(g_pBusClockFrequencyInt) # indirect load of g_PowerFreq.busClockFrequencyInt here
    lw         $a0, %lo(g_pBusClockFrequencyInt)($v1)
    lw         $a0, 0($a0) # load g_PowerFreq.busClockFrequencyInt into $a0
    slti       $v0, $a0, 75
    bnezl      $v0, loc_00003F0C # if g_PowerFreq.busClockFrequencyInt < 75
    slti       $a3, $a0, 50
    # compute new value for refreshParam1
    li         $a2, 8000
    mult       $a0, $a2
    li         $a2, 1
    mflo       $v0
    addiu      $a1, $v0, -75
    sra        $t1, $a1, 31
    srl        $t0, $t1, 22
    addu       $a0, $a1, $t0
    sra        $a1, $a0, 10 # refreshParam1 = ...

loc_00003ECC:
    jal        sceGeEdramSetRefreshParam # sceGeEdramSetRefreshParam(geEdramRefreshMode, refreshParam1, refreshParam2, refreshParam3)
    move       $a0, $s1
    move       $s0, $v0
    jal        sceKernelCpuResumeIntr # status = sceKernelCpuResumeIntr(intrState);
    move       $a0, $s2
    bltz       $s0, loc_00003EF4 # if (status < SCE_ERROR_OK) -> return status;
    move       $v0, $s0
    lui        $s2, %hi(g_pGeEdramRefreshMode) # indirect store to g_PowerFreq.geEdramRefreshMode here.
    lw         $s2, %lo(g_pGeEdramRefreshMode)($s2) # g_PowerFreq.geEdramRefreshMode = geEdramRefreshMode;
    sw         $s1, 0($s2) # g_PowerFreq.geEdramRefreshMode = geEdramRefreshMode; 
    move       $v0, $zr # return SCE_ERROR_OK

loc_00003EF4:
    lw         $ra, 12($sp)
    lw         $s2, 8($sp)
    lw         $s1, 4($sp)
    lw         $s0, 0($sp)
    jr         $ra
    addiu      $sp, $sp, 16

# g_PowerFreq.busClockFrequencyInt < 75
loc_00003F0C:
    bnez       $a3, loc_00003F40 # if (g_PowerFreq.busClockFrequencyInt < 50)
    slti       $s0, $a0, 25
    li         $t7, 8000
    mult       $a0, $t7
    li         $a3, 3 # refreshParam3 = 3
    li         $a2, 2 # refreshParam2 = 2
    mflo       $t6
    addiu      $t3, $t6, -75
    sra        $t5, $t3, 31
    srl        $t4, $t5, 23
    addu       $t2, $t3, $t4
    j          loc_00003ECC
    sra        $a1, $t2, 9 # refreshParam1 = ...

# g_PowerFreq.busClockFrequencyInt < 50
loc_00003F40:
    bnezl      $s0, loc_00003F84 # if (g_PowerFreq.busClockFrequencyInt < 25)
    li         $a2, 8000
    li         $v1, 8000
    mult       $a0, $v1
    lui        $t1, 0xBFA0
    ori        $a0, $t1, 0x2FE9
    li         $a3, 2 # refreshParam3 = 2
    li         $a2, 3 # refreshParam2 = 3
    mflo       $t0
    addiu      $a1, $t0, -75
    mult       $a1, $a0
    sra        $a0, $a1, 31
    mfhi       $t9
    addu       $t8, $t9, $a1
    sra        $v1, $t8, 8

loc_00003F7C:
    j          loc_00003ECC
    subu       $a1, $v1, $a0 # refreshParam1 = ...

# g_PowerFreq.busClockFrequencyInt < 25
loc_00003F84:
    mult       $a0, $a2
    lui        $v0, 0xBFA0
    ori        $t5, $v0, 0x2FE
    li         $a3, 1 # refreshParam3 = 1
    li         $a2, 6 # refreshParam2 = 6
    mflo       $t6
    addiu      $t4, $t6, -75
    mult       $t4, $t5
    sra        $a0, $t4, 31
    mfhi       $t3
    addu       $t2, $t3, $t4
    j          loc_00003F7C
    sra        $v1, $t2, 7

