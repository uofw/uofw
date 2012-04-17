# Copyright (C) 2011, 2012 The uOFW team
# See the file COPYING for copying permission.

    .text
    .set noat
    .set noreorder

    .globl sub_0000
sub_0000:
    lui        $a0, %hi(execNmiHandler)
    addiu      $a0, $a0, %lo(execNmiHandler)
    ctc0       $a0, $9
    lui        $a0, %hi(sub_0040)
    addiu      $a0, $a0, %lo(sub_0040)
    jr         $ra
    mtc0       $a0, $25

# At 0x0000001C
    # get threadman exception vector?
    .globl ExceptionManagerForKernel_96D47F7E
ExceptionManagerForKernel_96D47F7E:
    lui        $v0, %hi(sub_0070)
    jr         $ra
    addiu      $v0, $v0, %lo(sub_0070)
    nop        
    nop        
    nop        
    nop        
    nop        
    nop        

sub_0040:
    ctc0       $v0, $4
    ctc0       $v1, $5
    mfc0       $v0, $13
    mfc0       $v1, $12
    ctc0       $v0, $3
    ctc0       $v1, $2
    andi       $v0, $v0, 0x7C
    li         $v1, 32
    bne        $v0, $v1, loc_00000098
    mfc0       $v1, $14

    # 0068
    .globl syscallHandler
syscallHandler:
    j          syscallHandler
    ctc0       $v1, $0

sub_0070:
    ctc0       $v0, $4
    ctc0       $v1, $5
    mfc0       $v0, $13
    mfc0       $v1, $12
    ctc0       $v0, $3
    ctc0       $v1, $2
    lui        $v1, %hi(sub_0040)
    addiu      $v1, $v1, %lo(sub_0040)
    mtc0       $v1, $25
    mfc0       $v1, $14

loc_00000098:
    ctc0       $v1, $0

loc_0000009C:
    lui        $v1, %hi(ExcepManCB)
    ins        $v1, $v0, 0, 7
    lw         $v0, %lo(ExcepManCB)($v1)
    cfc0       $v1, $25
    bnezl      $v1, loc_000000BC # profiler
    sw         $v0, 12($v1)
    jr         $v0
    nop        

loc_000000BC:
    lw         $v1, 0($v1)
    lw         $v0, 0($v1)
    sw         $0, 0($v1)
    sync       
    cfc0       $v1, $25
    lw         $v1, 8($v1)
    bnez       $v1, loc_000000E0
    cfc0       $v1, $25
    sw         $v0, 4($v1)

loc_000000E0:
    lw         $v0, 8($v1)
    addiu      $v0, $v0, 1
    sw         $v0, 8($v1)
    lw         $v0, 12($v1)
    jr         $v0
    nop        

execNmiHandler:
    ctc0       $v1, $7
    mfc0       $v0, $13
    mfc0       $v1, $30
    ctc0       $v0, $20
    ctc0       $v1, $1
    mfc0       $v1, $12
    ctc0       $v1, $19
    li         $v0, 124
    b          loc_0000009C
    nop        

    .globl sub_0120
sub_0120:
    ctc0       $v0, $6
    mfc0       $v0, $22
    bne        $0, $v0, FUCK#ExceptionManagerForKernel_79454858
    nop        

    .globl sub_0130
sub_0130:
    cfc0       $v0, $9
    jr         $v0
    nop        
    nop        

    .globl sub_0140
sub_0140:
    nop        
    nop        
    nop        
    nop        
    nop        
    nop        
    nop        
    nop        

# At 0x00000160 
FUCK:
    .globl ExceptionManagerForKernel_79454858
ExceptionManagerForKernel_79454858:
    li         $v0, 92
    b          loc_0000009C
    nop        

    .globl sub_016C
sub_016C:
    nop        
    nop        

loc_00000174:
    b          loc_00000174
    nop        
    nop        

    # 0180
    .globl nmiInit
nmiInit:
    lui        $v0, 0xBC10
    lui        $v1, 0xFFFF
    ori        $v1, $v1, 0xFFFF
    sw         $v1, 4($v0)
    lui        $v1, %hi(g_0D40)
    sw         $0, %lo(g_0D40)($v1)
    lui        $v1, %hi(g_0D44)
    jr         $ra
    sw         $0, %lo(g_0D44)($v1)

    # 01A4
    .globl nmiHandler
nmiHandler:
    nop        
    nop        
    lui        $v1, %hi(g_0D40)
    lw         $v0, %lo(g_0D40)($v1)
    bnez       $v0, loc_000004E0 # handler was already called!
    addi       $v0, $v0, 1
    sw         $v0, %lo(g_0D40)($v1)
    lui        $v0, %hi(g_0D44)
    sw         $0, %lo(g_0D44)($v0)
    move       $v0, $sp
    lui        $sp, %hi(g_stackCtx)
    addiu      $sp, $sp, %lo(g_stackCtx)
    sw         $at, 4($sp)
    sw         $a0, 16($sp)
    sw         $a1, 20($sp)
    sw         $a2, 24($sp)
    sw         $a3, 28($sp)
    sw         $t0, 32($sp)
    sw         $t1, 36($sp)
    sw         $t2, 40($sp)
    sw         $t3, 44($sp)
    sw         $t4, 48($sp)
    sw         $t5, 52($sp)
    sw         $t6, 56($sp)
    sw         $t7, 60($sp)
    sw         $s0, 64($sp)
    sw         $s1, 68($sp)
    sw         $s2, 72($sp)
    sw         $s3, 76($sp)
    sw         $s4, 80($sp)
    sw         $s5, 84($sp)
    sw         $s6, 88($sp)
    sw         $s7, 92($sp)
    sw         $t8, 96($sp)
    sw         $t9, 100($sp)
    sw         $k0, 104($sp)
    sw         $k1, 108($sp)
    sw         $gp, 112($sp)
    sw         $v0, 116($sp)
    sw         $fp, 120($sp)
    sw         $ra, 124($sp)
    mfhi       $v0
    mflo       $v1
    sw         $v0, 128($sp)
    sw         $v1, 132($sp)
    lui        $v0, 0xBC10
    lw         $s0, 0($v0)
    move       $s1, $0
    lui        $v0, 0xFFFF
    ori        $v0, $v0, 0xFFFF
    and        $v0, $v0, $s0
    beqz       $v0, loc_000002A0
    bitrev     $v1, $s0
    clz        $s2, $v1
    li         $v0, 0xFFFF
    and        $v0, $v0, $s0
    bnez       $v0, loc_000002F0
    li         $a0, 0
    addiu      $s2, $s2, -16
    lui        $v0, 0xFFFF
    and        $v0, $v0, $s0
    bnez       $v0, loc_000002F0
    li         $a0, 1

loc_000002A0:
    mfc0       $v0, $24
    andi       $v0, $v0, 0x1
    beqz       $v0, loc_000002E8
    nop        
    li         $s2, 16
    lui        $s1, 0xA7F0
    lw         $s1, 4($s1)
    lui        $v0, 0x2222
    ori        $v0, $v0, 0x2222
    move       $a1, $s1
    li         $a2, -1
    and        $a0, $s1, $v0
    bnez       $a0, loc_00000374
    li         $a0, 0
    sll        $v0, $v0, 1
    and        $a0, $s1, $v0
    bnez       $a0, loc_00000374
    li         $a0, 2

loc_000002E8:
    j          loc_000004DC
    nop        

loc_000002F0:
    lui        $v0, 0xBC10
    li         $at, 8
    beql       $s2, $at, loc_00000368
    ori        $v0, $v0, 0x10
    li         $at, 9
    beql       $s2, $at, loc_00000368
    ori        $v0, $v0, 0x14
    li         $at, 10
    beql       $s2, $at, loc_00000368
    ori        $v0, $v0, 0xA0
    li         $at, 11
    beql       $s2, $at, loc_00000368
    ori        $v0, $v0, 0xA4
    li         $at, 12
    beql       $s2, $at, loc_00000368
    ori        $v0, $v0, 0xC
    li         $at, 13
    beql       $s2, $at, loc_00000368
    ori        $v0, $v0, 0x88
    li         $at, 14
    beql       $s2, $at, loc_00000368
    ori        $v0, $v0, 0xE0
    li         $at, 15
    beql       $s2, $at, loc_00000368
    ori        $v0, $v0, 0xE4
    sll        $k0, $s2, 2
    li         $k1, 52
    subu       $k1, $k1, $k0
    lui        $v0, 0xBC10
    addu       $v0, $v0, $k1

loc_00000368:
    lw         $a2, 0($v0)
    ext        $a1, $a2, 31, 1
    ins        $a2, $0, 0, 31

loc_00000374:
    cfc0       $a3, $0
    cfc0       $t0, $1
    cfc0       $v0, $18
    sll        $v1, $s2, 2
    addu       $k0, $v0, $v1
    lw         $s3, 0($k0)
    lui        $at, %hi(g_0D44)
    sw         $s3, %lo(g_0D44)($at)
    beqz       $s3, loc_000003D4
    nop        
    beqz       $s0, loc_000003AC
    lui        $k0, 0xBC10
    sw         $s0, 4($k0)
    sync       

loc_000003AC:
    beqz       $s1, loc_000003C4
    lui        $k0, 0xA7F8
    lw         $0, 0($k0)
    lui        $k0, 0xA7F0
    sw         $s1, 32($k0)
    sync       

loc_000003C4:
    lui        $sp, %hi(g_stackCtx)
    addiu      $sp, $sp, %lo(g_stackCtx)
    jalr       $s3
    nop        

loc_000003D4:
    lui        $sp, %hi(g_stackCtx)
    addiu      $sp, $sp, %lo(g_stackCtx)
    lw         $k0, 128($sp)
    lw         $k1, 132($sp)
    mthi       $k0
    mtlo       $k1
    lw         $a0, 16($sp)
    lw         $a1, 20($sp)
    lw         $a2, 24($sp)
    lw         $a3, 28($sp)
    lw         $t0, 32($sp)
    lw         $t1, 36($sp)
    lw         $t2, 40($sp)
    lw         $t3, 44($sp)
    lw         $t4, 48($sp)
    lw         $t5, 52($sp)
    lw         $t6, 56($sp)
    lw         $t7, 60($sp)
    lw         $s0, 64($sp)
    lw         $s1, 68($sp)
    lw         $s2, 72($sp)
    lw         $s3, 76($sp)
    lw         $s4, 80($sp)
    lw         $s5, 84($sp)
    lw         $s6, 88($sp)
    lw         $s7, 92($sp)
    lw         $t8, 96($sp)
    lw         $t9, 100($sp)
    lw         $k0, 104($sp)
    lw         $k1, 108($sp)
    lw         $gp, 112($sp)
    lw         $fp, 120($sp)
    lw         $ra, 124($sp)
    lw         $at, 4($sp)
    lw         $sp, 116($sp)
    lui        $v0, %hi(g_0D40)
    sw         $0, %lo(g_0D40)($v0)
    cfc0       $v1, $19
    lui        $v0, 0xFFBF
    ori        $v0, $v0, 0xFFFF
    and        $v1, $v1, $v0
    mtc0       $v1, $12
    lui        $v0, %hi(g_0D44)
    lw         $v1, %lo(g_0D44)($v0)
    beqz       $v1, loc_000004CC
    nop        
    mfc0       $v1, $12
    lui        $v0, 0xFFEF
    ori        $v0, $v0, 0xFFFF
    and        $v1, $v1, $v0
    mtc0       $v1, $12
    cfc0       $v1, $20
    mtc0       $v1, $13
    cfc0       $v1, $1
    mtc0       $v1, $30
    cfc0       $v0, $6
    cfc0       $v1, $7
    nop        
    nop        
    eret       
    nop        
    nop        

loc_000004CC:
    lui        $v0, %hi(nmiHandler)
    lw         $v0, %lo(nmiHandler)($v0)
    jr         $v0
    nop        

loc_000004DC:
    break      0x20000

loc_000004E0:
    break      0x20000

