#include "../include/common_asm.h"

loc_00000000:
    move $t9, $ra
    sw $s0, 0($a0)
    sw $s1, 4($a0)
    sw $s2, 8($a0)
    sw $s3, 12($a0)
    sw $s4, 16($a0)
    sw $s5, 20($a0)
    sw $s6, 24($a0)
    sw $s7, 28($a0)
    sw $k0, 32($a0)
    sw $k1, 36($a0)
    sw $gp, 40($a0)
    sw $sp, 44($a0)
    sw $fp, 48($a0)
    sw $ra, 52($a0)
    mfc0 $v0, COP0_STATE_STATUS
    lui $t1, 0x2000
    or $t1, $v0, $t1
    mtc0 $t1, COP0_STATE_STATUS
    nop
    nop
    cfc1 $t0, $0
    cfc1 $t1, $31
    sw $t0, 56($a0)
    sw $t1, 60($a0)
    swc1 $0, 64($a0)
    swc1 $1, 68($a0)
    swc1 $2, 72($a0)
    swc1 $3, 76($a0)
    swc1 $4, 80($a0)
    swc1 $5, 84($a0)
    swc1 $6, 88($a0)
    swc1 $7, 92($a0)
    swc1 $8, 96($a0)
    swc1 $9, 100($a0)
    swc1 $10, 104($a0)
    swc1 $11, 108($a0)
    swc1 $12, 112($a0)
    swc1 $13, 116($a0)
    swc1 $14, 120($a0)
    swc1 $15, 124($a0)
    swc1 $16, 128($a0)
    swc1 $17, 132($a0)
    swc1 $18, 136($a0)
    swc1 $19, 140($a0)
    swc1 $20, 144($a0)
    swc1 $21, 148($a0)
    swc1 $22, 152($a0)
    swc1 $23, 156($a0)
    swc1 $24, 160($a0)
    swc1 $25, 164($a0)
    swc1 $26, 168($a0)
    swc1 $27, 172($a0)
    swc1 $28, 176($a0)
    swc1 $29, 180($a0)
    swc1 $30, 184($a0)
    swc1 $31, 188($a0)
    mtc0 $v0, COP0_STATE_STATUS
    nop
    nop
    addiu $t8, $a0, 192
    move $t0, $zr
    move $t1, $zr
    move $t2, $zr
    move $t3, $zr
    move $t4, $zr
    move $t5, $zr
    move $t6, $zr
    bal sub_00000304
    move $t7, $zr
    mfc0 $t0, COP0_STATE_BAD_VADDR
    mfc0 $t1, COP0_STATE_COUNT
    move $t2, $zr
    mfc0 $t3, COP0_STATE_COMPARE
    mfc0 $t4, COP0_STATE_STATUS
    mfc0 $t5, COP0_STATE_CAUSE
    mfc0 $t6, COP0_STATE_EPC
    bal sub_00000304
    mfc0 $t7, COP0_STATE_PRID
    move $t0, $zr
    move $t1, $zr
    move $t2, $zr
    move $t3, $zr
    move $t4, $zr
    mfc0 $t5, $21
    mfc0 $t6, $22
    bal sub_00000304
    move $t7, $zr
    mfc0 $t0, $24
    mfc0 $t1, COP0_STATE_EBASE
    move $t2, $zr
    move $t3, $zr
    mfc0 $t5, COP0_STATE_TAG_LO
    mfc0 $t6, COP0_STATE_TAG_HI
    bal sub_00000304
    mfc0 $t7, COP0_STATE_ERROR_PC
    cfc0 $t0, $0
    cfc0 $t1, $1
    cfc0 $t2, $2
    cfc0 $t3, $3
    cfc0 $t4, $4
    cfc0 $t5, $5
    cfc0 $t6, $6
    bal sub_00000304
    cfc0 $t7, $7
    cfc0 $t0, $8
    cfc0 $t1, $9
    cfc0 $t2, $10
    cfc0 $t3, $11
    cfc0 $t4, $12
    cfc0 $t5, $13
    cfc0 $t6, $14
    bal sub_00000304
    cfc0 $t7, $15
    cfc0 $t0, $16
    cfc0 $t1, $17
    cfc0 $t2, $18
    cfc0 $t3, $19
    cfc0 $t4, $20
    cfc0 $t5, $21
    cfc0 $t6, $22
    bal sub_00000304
    cfc0 $t7, $23
    cfc0 $t0, $24
    cfc0 $t1, $25
    cfc0 $t2, $26
    cfc0 $t3, $27
    cfc0 $t4, $28
    cfc0 $t5, $29
    cfc0 $t6, $30
    bal sub_00000304
    cfc0 $t7, $31
    mfc0 $v0, COP0_STATE_STATUS
    lui $t1, 0x4000
    or $t1, $v0, $t1
    mtc0 $t1, COP0_STATE_STATUS
    nop
    nop
    mfvc $t0, VFPU_PFXS
    mfvc $t1, VFPU_PFXT
    mfvc $t2, VFPU_PFXD
    mfvc $t3, VFPU_CC
    mfvc $t4, VFPU_INF4
    move $t5, $zr
    move $t6, $zr
    bal sub_00000304
    mfvc $t7, VFPU_REV
    mfvc $t0, VFPU_RCX0
    mfvc $t1, VFPU_RCX1
    mfvc $t2, VFPU_RCX2
    mfvc $t3, VFPU_RCX3
    mfvc $t4, VFPU_RCX4
    mfvc $t5, VFPU_RCX5
    mfvc $t6, VFPU_RCX6
    bal sub_00000304
    mfvc $t7, VFPU_RCX7
    sv.q       C000, 0($t8)
    sv.q       C010, 16($t8)
    sv.q       C020, 32($t8)
    sv.q       C030, 48($t8)
    sv.q       C100, 64($t8)
    sv.q       C110, 80($t8)
    sv.q       C120, 96($t8)
    sv.q       C130, 112($t8)
    sv.q       C200, 128($t8)
    sv.q       C210, 144($t8)
    sv.q       C220, 160($t8)
    sv.q       C230, 176($t8)
    sv.q       C300, 192($t8)
    sv.q       C310, 208($t8)
    sv.q       C320, 224($t8)
    sv.q       C330, 240($t8)
    sv.q       C400, 256($t8)
    sv.q       C410, 272($t8)
    sv.q       C420, 288($t8)
    sv.q       C430, 304($t8)
    sv.q       C500, 320($t8)
    sv.q       C510, 336($t8)
    sv.q       C520, 352($t8)
    sv.q       C530, 368($t8)
    sv.q       C600, 384($t8)
    sv.q       C610, 400($t8)
    sv.q       C620, 416($t8)
    sv.q       C630, 432($t8)
    sv.q       C700, 448($t8)
    sv.q       C710, 464($t8)
    sv.q       C720, 480($t8)
    sv.q       C730, 496($t8)
    mtc0 $v0, COP0_STATE_STATUS
    nop
    nop
    jr $t9
    move $v0, $zr

sub_00000304:
    sw $t0, 0($t8)
    sw $t1, 4($t8)
    sw $t2, 8($t8)
    sw $t3, 12($t8)
    sw $t4, 16($t8)
    sw $t5, 20($t8)
    sw $t6, 24($t8)
    sw $t7, 28($t8)
    jr $ra
    addiu $t8, $t8, 32

loc_0000032C:
    move $at, $zr
    move $v0, $zr
    move $v1, $zr
    move $a2, $zr
    move $a3, $zr
    lw $s0, 0($a0)
    lw $s1, 4($a0)
    lw $s2, 8($a0)
    lw $s3, 12($a0)
    lw $s4, 16($a0)
    lw $s5, 20($a0)
    lw $s6, 24($a0)
    lw $s7, 28($a0)
    lw $k0, 32($a0)
    lw $k1, 36($a0)
    lw $gp, 40($a0)
    lw $sp, 44($a0)
    lw $fp, 48($a0)
    lw $t9, 52($a0)
    mfc0 $t0, COP0_STATE_STATUS
    lui $t1, 0x2000
    or $t1, $t0, $t1
    mtc0 $t1, COP0_STATE_STATUS
    nop
    nop
    lw $t0, 60($a0)
    ctc1 $t0, $31
    lwc1 $0, 64($a0)
    lwc1 $1, 68($a0)
    lwc1 $2, 72($a0)
    lwc1 $3, 76($a0)
    lwc1 $4, 80($a0)
    lwc1 $5, 84($a0)
    lwc1 $6, 88($a0)
    lwc1 $7, 92($a0)
    lwc1 $8, 96($a0)
    lwc1 $9, 100($a0)
    lwc1 $10, 104($a0)
    lwc1 $11, 108($a0)
    lwc1 $12, 112($a0)
    lwc1 $13, 116($a0)
    lwc1 $14, 120($a0)
    lwc1 $15, 124($a0)
    lwc1 $16, 128($a0)
    lwc1 $17, 132($a0)
    lwc1 $18, 136($a0)
    lwc1 $19, 140($a0)
    lwc1 $20, 144($a0)
    lwc1 $21, 148($a0)
    lwc1 $22, 152($a0)
    lwc1 $23, 156($a0)
    lwc1 $24, 160($a0)
    lwc1 $25, 164($a0)
    lwc1 $26, 168($a0)
    lwc1 $27, 172($a0)
    lwc1 $28, 176($a0)
    lwc1 $29, 180($a0)
    lwc1 $30, 184($a0)
    lwc1 $31, 188($a0)
    addiu $t8, $a0, 192
    bal sub_00000604
    nop
    bal sub_00000604
    nop
    mtc0 $t4, COP0_STATE_STATUS
    mtc0 $t5, COP0_STATE_CAUSE
    mtc0 $t6, COP0_STATE_EPC
    bal sub_00000604
    nop
    bal sub_00000604
    nop
    mtc0 $t1, COP0_STATE_EBASE
    mtc0 $t4, COP0_STATE_TAG_LO
    mtc0 $t5, COP0_STATE_TAG_HI
    mtc0 $t6, COP0_STATE_ERROR_PC
    bal sub_00000604
    nop
    ctc0 $t0, $0
    ctc0 $t1, $1
    ctc0 $t2, $2
    ctc0 $t3, $3
    ctc0 $t4, $4
    ctc0 $t5, $5
    ctc0 $t6, $6
    bal sub_00000604
    ctc0 $t7, $7
    ctc0 $t0, $8
    ctc0 $t1, $9
    ctc0 $t2, $10
    ctc0 $t3, $11
    ctc0 $t4, $12
    ctc0 $t5, $13
    ctc0 $t6, $14
    bal sub_00000604
    ctc0 $t7, $15
    ctc0 $t0, $16
    ctc0 $t1, $17
    ctc0 $t2, $18
    ctc0 $t3, $19
    ctc0 $t4, $20
    ctc0 $t5, $21
    ctc0 $t6, $22
    bal sub_00000604
    ctc0 $t7, $23
    ctc0 $t0, $24
    ctc0 $t1, $25
    ctc0 $t2, $26
    ctc0 $t3, $27
    ctc0 $t4, $28
    ctc0 $t5, $29
    ctc0 $t6, $30
    ctc0 $t7, $31
    mfc0 $v0, COP0_STATE_STATUS
    lui $t1, 0x4000
    or $t1, $v0, $t1
    mtc0 $t1, COP0_STATE_STATUS
    nop
    nop
    bal sub_00000604
    nop
    mtvc $t0, VFPU_PFXS
    mtvc $t1, VFPU_PFXT
    mtvc $t2, VFPU_PFXD
    mtvc $t3, VFPU_CC
    bal sub_00000604
    mtvc $t4, VFPU_INF4
    mtvc $t0, VFPU_RCX0
    mtvc $t1, VFPU_RCX1
    mtvc $t2, VFPU_RCX2
    mtvc $t3, VFPU_RCX3
    mtvc $t4, VFPU_RCX4
    mtvc $t5, VFPU_RCX5
    mtvc $t6, VFPU_RCX6
    mtvc $t7, VFPU_RCX7
    lv.q       C000, 0($t8)
    lv.q       C010, 16($t8)
    lv.q       C020, 32($t8)
    lv.q       C030, 48($t8)
    lv.q       C100, 64($t8)
    lv.q       C110, 80($t8)
    lv.q       C120, 96($t8)
    lv.q       C130, 112($t8)
    lv.q       C200, 128($t8)
    lv.q       C210, 144($t8)
    lv.q       C220, 160($t8)
    lv.q       C230, 176($t8)
    lv.q       C300, 192($t8)
    lv.q       C310, 208($t8)
    lv.q       C320, 224($t8)
    lv.q       C330, 240($t8)
    lv.q       C400, 256($t8)
    lv.q       C410, 272($t8)
    lv.q       C420, 288($t8)
    lv.q       C430, 304($t8)
    lv.q       C500, 320($t8)
    lv.q       C510, 336($t8)
    lv.q       C520, 352($t8)
    lv.q       C530, 368($t8)
    lv.q       C600, 384($t8)
    lv.q       C610, 400($t8)
    lv.q       C620, 416($t8)
    lv.q       C630, 432($t8)
    lv.q       C700, 448($t8)
    lv.q       C710, 464($t8)
    lv.q       C720, 480($t8)
    lv.q       C730, 496($t8)
    mtc0 $v0, COP0_STATE_STATUS
    nop
    nop
    move $ra, $t9
    move $t0, $zr
    move $t1, $zr
    move $t2, $zr
    move $t3, $zr
    move $t4, $zr
    move $t5, $zr
    move $t6, $zr
    move $t7, $zr
    move $t8, $zr
    move $t9, $zr
    jr $ra
    move $v0, $a1

sub_00000604:
    lw $t0, 0($t8)
    lw $t1, 4($t8)
    lw $t2, 8($t8)
    lw $t3, 12($t8)
    lw $t4, 16($t8)
    lw $t5, 20($t8)
    lw $t6, 24($t8)
    lw $t7, 28($t8)
    jr $ra
    addiu $t8, $t8, 32

sceSuspendForKernel_67B59042:
    lui $a0, %hi(g_13BE0)
    addiu $a0, $a0, %lo(g_13BE0)
    j loc_00000000
    nop

sceSuspendForKernel_B2C9640B:
    move $a1, $a0
    lui $a0, %hi(g_13BE0)
    addiu $a0, $a0, %lo(g_13BE0)
    j loc_0000032C
    nop
    move $t9, $ra
    lui $ra, %hi(sub_1120)
    addiu $ra, $ra, %lo(sub_1120)
    j UtilsForUser_3FD3D324
    nop
    move $t9, $ra
    lui $ra, %hi(sub_1120)
    addiu $ra, $ra, %lo(sub_1120)

sceKernelDcacheWritebackAll:
    mfc0 $t8, $24
    andi $t8, $t8, 0x1
    bnez $t8, loc_00000714
    move $at, $ra

UtilsForUser_3FD3D324:
    mfic $v0, $0
    mtic $zr, $0
    mfc0 $t0, COP0_STATE_CONFIG
    li $t1, 2048
    ext $t0, $t0, 6, 3
    sllv $t1, $t1, $t0
    move $t0, $zr
    li $t6, -1

loc_000006A0:
    cache 0x10, 0($t0)
    mfc0 $t2, COP0_STATE_TAG_LO
    mfc0 $t3, COP0_STATE_TAG_HI
    move $t4, $t2
    xor $t5, $t3, $t6
    ext $t5, $t5, 0, 19
    movz $t2, $t3, $t5
    movz $t3, $t4, $t5
    ext $t4, $t2, 20, 1
    beqz $t4, loc_000006DC
    ext $t4, $t3, 20, 1
    sll $t5, $t2, 13
    or $t5, $t5, $t0
    cache 0x1A, 0($t5)
    move $t6, $t2

loc_000006DC:
    beqz $t4, loc_000006F0
    sll $t5, $t3, 13
    or $t5, $t5, $t0
    cache 0x1A, 0($t5)
    move $t6, $t3

loc_000006F0:
    mtic $v0, $0
    addiu $t0, $t0, 64
    mtic $zr, $0
    bne $t0, $t1, loc_000006A0
    nop
    mtic $v0, $0
    move $v0, $zr
    jr $ra
    sync

loc_00000714:
    jal UtilsForUser_3FD3D324
    nop
    j UtilsForUser_0939D45C
    move $ra, $at
    move $t9, $ra
    lui $ra, %hi(sub_1120)
    addiu $ra, $ra, %lo(sub_1120)
    j UtilsForUser_AF3766BB
    nop
    move $t9, $ra
    lui $ra, %hi(sub_1120)
    addiu $ra, $ra, %lo(sub_1120)

sceKernelDcacheWritebackInvalidateAll:
    mfc0 $t8, $24
    andi $t8, $t8, 0x1
    bnez $t8, loc_00000784
    move $at, $ra

UtilsForUser_AF3766BB:
    mfc0 $t0, COP0_STATE_CONFIG
    li $t1, 2048
    ext $t0, $t0, 6, 3
    sllv $t1, $t1, $t0
    move $t0, $zr

loc_00000768:
    cache 0x14, 0($t0)
    addiu $t0, $t0, 64
    bne $t0, $t1, loc_00000768
    cache 0x14, -64($t0)
    move $v0, $zr
    jr $ra
    sync

loc_00000784:
    jal UtilsForUser_AF3766BB
    nop
    j UtilsForUser_485E430A
    move $ra, $at
    move $t9, $ra
    lui $ra, %hi(sub_1120)
    addiu $ra, $ra, %lo(sub_1120)
    j UtilsForKernel_AA9AF5CF
    nop
    move $t9, $ra
    lui $ra, %hi(sub_1120)
    addiu $ra, $ra, %lo(sub_1120)

sceKernelDcacheInvalidateAll:
    mfc0 $t8, $24
    andi $t8, $t8, 0x1
    bnez $t8, loc_00000804
    move $at, $ra

UtilsForKernel_AA9AF5CF:
    mfc0 $t0, COP0_STATE_CONFIG
    li $t1, 4096
    ext $t0, $t0, 6, 3
    sllv $t1, $t1, $t0
    mfic $v0, $0
    mtic $zr, $0
    mtc0 $zr, COP0_STATE_TAG_LO
    mtc0 $zr, COP0_STATE_TAG_HI
    move $t0, $zr

loc_000007E8:
    addiu $t0, $t0, 64
    cache 0x11, -64($t0)
    bne $t0, $t1, loc_000007E8
    cache 0x13, -64($t0)
    mtic $v0, $0
    jr $ra
    move $v0, $zr

loc_00000804:
    jal UtilsForKernel_AA9AF5CF
    nop
    j UtilsForKernel_AF0C0173
    move $ra, $at
    move $t9, $ra
    lui $ra, %hi(sub_1120)
    addiu $ra, $ra, %lo(sub_1120)
    j UtilsForUser_004D4DEE
    nop
    move $t9, $ra
    lui $ra, %hi(sub_1120)
    addiu $ra, $ra, %lo(sub_1120)

sceKernelDcacheWritebackRange:
    mfc0 $t8, $24
    andi $t8, $t8, 0x1
    bnez $t8, loc_000008F0
    move $at, $ra

UtilsForUser_004D4DEE:
    srl $v0, $a1, 16
    bnez $v0, loc_0000087C
    addu $a2, $a0, $a1
    beq $a2, $a0, loc_00000868
    addiu $a2, $a2, 63
    ins $a2, $a0, 0, 6

loc_0000085C:
    addiu $a0, $a0, 64
    bne $a0, $a2, loc_0000085C
    cache 0x1A, -64($a0)

loc_00000868:
    jr $ra
    sync

loc_00000870:
    lui $v0, 0x8000
    jr $ra
    ori $v0, $v0, 0x104

loc_0000087C:
    ext $v0, $k1, 19, 3
    negu $v0, $v0
    and $v0, $v0, $a1
    bltz $v0, loc_00000870
    li $v0, -8192
    ins $a0, $zr, 0, 6
    subu $a1, $a1, $a0
    mfic $v1, $0
    move $t0, $v0

loc_000008A0:
    mtic $zr, $0
    addiu $v0, $v0, 64
    cache 0x10, 8128($v0)
    mfc0 $t2, COP0_STATE_TAG_LO
    mfc0 $t3, COP0_STATE_TAG_HI
    mtic $v1, $0
    ins $t0, $t2, 13, 19
    subu $t4, $t0, $a0
    sltu $t4, $t4, $a1
    bnezl $t4, loc_000008CC
    cache 0x1A, 0($t0)

loc_000008CC:
    ins $t0, $t3, 13, 19
    subu $t4, $t0, $a0
    sltu $t4, $t4, $a1
    bnezl $t4, loc_000008E0
    cache 0x1A, 0($t0)

loc_000008E0:
    bnez $v0, loc_000008A0
    move $t0, $v0
    jr $ra
    sync

loc_000008F0:
    move $t6, $a0
    jal UtilsForUser_004D4DEE
    move $t7, $a1
    bltz $v0, loc_00000870
    move $ra, $at
    move $a0, $t6
    j UtilsForUser_601E43E6
    move $a1, $t7
    move $t9, $ra
    lui $ra, %hi(sub_1120)
    addiu $ra, $ra, %lo(sub_1120)
    j UtilsForUser_1B0592A3
    nop
    move $t9, $ra
    lui $ra, %hi(sub_1120)
    addiu $ra, $ra, %lo(sub_1120)

sceKernelDcacheWritebackInvalidateRange:
    mfc0 $t8, $24
    andi $t8, $t8, 0x1
    bnez $t8, loc_000009EC
    move $at, $ra

UtilsForUser_1B0592A3:
    srl $v0, $a1, 16
    bnez $v0, loc_00000978
    addu $a2, $a0, $a1
    beq $a2, $a0, loc_00000964
    addiu $a2, $a2, 63
    ins $a2, $a0, 0, 6

loc_00000958:
    addiu $a0, $a0, 64
    bne $a0, $a2, loc_00000958
    cache 0x1B, -64($a0)

loc_00000964:
    jr $ra
    sync

loc_0000096C:
    lui $v0, 0x8000
    jr $ra
    ori $v0, $v0, 0x104

loc_00000978:
    ext $v0, $k1, 19, 3
    negu $v0, $v0
    and $v0, $v0, $a1
    bltz $v0, loc_0000096C
    li $v0, -8192
    ins $a0, $zr, 0, 6
    subu $a1, $a1, $a0
    mfic $v1, $0
    move $t0, $v0

loc_0000099C:
    mtic $zr, $0
    addiu $v0, $v0, 64
    cache 0x10, 8128($v0)
    mfc0 $t2, COP0_STATE_TAG_LO
    mfc0 $t3, COP0_STATE_TAG_HI
    mtic $v1, $0
    ins $t0, $t2, 13, 19
    subu $t4, $t0, $a0
    sltu $t4, $t4, $a1
    bnezl $t4, loc_000009C8
    cache 0x1B, 0($t0)

loc_000009C8:
    ins $t0, $t3, 13, 19
    subu $t4, $t0, $a0
    sltu $t4, $t4, $a1
    bnezl $t4, loc_000009DC
    cache 0x1B, 0($t0)

loc_000009DC:
    bnez $v0, loc_0000099C
    move $t0, $v0
    jr $ra
    sync

loc_000009EC:
    move $t6, $a0
    jal UtilsForUser_1B0592A3
    move $t7, $a1
    bltz $v0, loc_0000096C
    move $ra, $at
    move $a0, $t6
    j UtilsForUser_7C5DC253
    move $a1, $t7
    move $t9, $ra
    lui $ra, %hi(sub_1120)
    addiu $ra, $ra, %lo(sub_1120)
    j UtilsForKernel_157A383A
    nop
    move $t9, $ra
    lui $ra, %hi(sub_1120)
    addiu $ra, $ra, %lo(sub_1120)

UtilsForKernel_BFA98062:
    mfc0 $t8, $24
    andi $t8, $t8, 0x1
    bnez $t8, loc_00000ADC
    move $at, $ra

UtilsForKernel_157A383A:
    beqz $a1, loc_00000A68
    or $t0, $a0, $a1
    andi $t0, $t0, 0x3F
    bnez $t0, loc_00000A70
    li $t0, 0xC000
    sltu $t0, $a1, $t0
    beqz $t0, loc_00000A7C
    addu $a2, $a0, $a1

loc_00000A5C:
    addiu $a0, $a0, 64
    bne $a0, $a2, loc_00000A5C
    cache 0x19, -64($a0)

loc_00000A68:
    jr $ra
    move $v0, $zr

loc_00000A70:
    lui $v0, 0x8002
    jr $ra
    ori $v0, $v0, 0x44C

loc_00000A7C:
    bltzl $a1, loc_00000A84
    break 0x0

loc_00000A84:
    li $v0, 8128
    mfic $v1, $0

loc_00000A8C:
    mtic $zr, $0
    move $t0, $v0
    cache 0x10, 0($v0)
    mfc0 $t2, COP0_STATE_TAG_LO
    mfc0 $t3, COP0_STATE_TAG_HI
    mtic $v1, $0
    ins $t0, $t2, 13, 19
    subu $t4, $t0, $a0
    sltu $t4, $t4, $a1
    bnezl $t4, loc_00000AB8
    cache 0x19, 0($t0)

loc_00000AB8:
    ins $t0, $t3, 13, 19
    subu $t4, $t0, $a0
    sltu $t4, $t4, $a1
    bnezl $t4, loc_00000ACC
    cache 0x19, 0($t0)

loc_00000ACC:
    bnez $v0, loc_00000A8C
    addiu $v0, $v0, -64
    jr $ra
    move $v0, $zr

loc_00000ADC:
    move $t6, $a0
    jal UtilsForKernel_157A383A
    move $t7, $a1
    bltz $v0, loc_00000A70
    move $ra, $at
    move $a0, $t6
    j UtilsForKernel_F192F2EC
    move $a1, $t7
    move $t9, $ra
    lui $ra, %hi(sub_1120)
    addiu $ra, $ra, %lo(sub_1120)

UtilsForKernel_99134C3F:
    mfc0 $t0, COP0_STATE_CONFIG
    beqz $a1, loc_00000BA0
    li $t1, 4096
    ext $t0, $t0, 6, 3
    sllv $t1, $t1, $t0
    or $t0, $a0, $a1
    andi $t0, $t0, 0x3F
    bnez $t0, loc_00000BA8
    sltu $t0, $a1, $t1
    addu $a1, $a0, $a1
    beqzl $t0, loc_00000BB4
    addiu $a1, $a1, -1
    mfic $v0, $0
    mtic $zr, $0

loc_00000B40:
    srl $t0, $a0, 13
    cache 0x10, 0($a0)
    sll $t0, $t0, 13
    mfc0 $t1, COP0_STATE_TAG_LO
    mfc0 $t2, COP0_STATE_TAG_HI
    ext $t3, $t1, 20, 1
    ext $t4, $t2, 20, 1
    sll $t5, $t1, 13
    sll $t6, $t2, 13
    xor $t5, $t5, $t0
    xor $t6, $t6, $t0
    movz $t3, $zr, $t5
    movz $t4, $zr, $t6
    ext $t1, $t3, 20, 1
    ext $t2, $t4, 20, 1
    mtc0 $t1, COP0_STATE_TAG_LO
    mtc0 $t2, COP0_STATE_TAG_HI
    cache 0x11, 0($a0)
    mtic $v0, $0
    addiu $a0, $a0, 64
    mtic $zr, $0
    bne $a0, $a1, loc_00000B40
    nop
    mtic $v0, $0

loc_00000BA0:
    jr $ra
    move $v0, $zr

loc_00000BA8:
    lui $v0, 0x8002
    jr $ra
    ori $v0, $v0, 0x44C

loc_00000BB4:
    mfic $v0, $0
    mtic $zr, $0
    srl $t1, $t1, 1
    move $t0, $zr

loc_00000BC4:
    cache 0x10, 0($t0)
    mfc0 $t2, COP0_STATE_TAG_LO
    ext $t3, $t2, 19, 2
    xori $t3, $t3, 0x3
    bnez $t3, loc_00000BF8
    sll $t3, $t2, 13
    or $t3, $t3, $t0
    subu $t4, $a1, $t3
    subu $t5, $t3, $a0
    or $t4, $t4, $t5
    bltz $t4, loc_00000BF8
    ins $t2, $zr, 20, 1
    mtc0 $t2, COP0_STATE_TAG_LO

loc_00000BF8:
    mfc0 $t2, COP0_STATE_TAG_HI
    ext $t3, $t2, 19, 2
    xori $t3, $t3, 0x3
    beqz $t3, loc_00000C28
    sll $t3, $t2, 13
    or $t3, $t3, $t0
    subu $t4, $a1, $t3
    subu $t5, $t3, $a0
    or $t4, $t4, $t5
    bltz $t4, loc_00000C28
    ins $t2, $zr, 20, 1
    mtc0 $t2, COP0_STATE_TAG_HI

loc_00000C28:
    cache 0x11, 0($t0)
    mtic $v0, $0
    addiu $t0, $t0, 64
    mtic $zr, $0
    bne $t0, $t1, loc_00000BC4
    nop
    mtic $v0, $0
    move $v0, $zr
    jr $ra
    sync
    move $t9, $ra
    lui $ra, %hi(sub_1120)
    addiu $ra, $ra, %lo(sub_1120)
    j UtilsForUser_740DF7F0
    nop
    move $t9, $ra
    lui $ra, %hi(sub_1120)
    addiu $ra, $ra, %lo(sub_1120)

sceKernelDcacheProbe:
    mfc0 $t8, $24
    andi $t8, $t8, 0x1
    bnez $t8, loc_00000CD0
    move $at, $ra

UtilsForUser_740DF7F0:
    mfic $v0, $0
    mtic $zr, $0
    srl $t0, $a0, 13
    cache 0x10, 0($a0)
    sll $t0, $t0, 13
    mfc0 $t1, COP0_STATE_TAG_LO
    mfc0 $t2, COP0_STATE_TAG_HI
    mtic $v0, $0
    ext $t3, $t1, 19, 2
    ext $t4, $t2, 19, 2
    sll $t1, $t1, 13
    sll $t2, $t2, 13
    xor $t1, $t1, $t0
    xor $t2, $t2, $t0
    movn $t3, $zr, $t1
    movn $t4, $zr, $t2
    or $v0, $t3, $t4
    ext $t0, $v0, 1, 1
    jr $ra
    xor $v0, $v0, $t0

loc_00000CD0:
    jal UtilsForUser_740DF7F0
    nop
    jal UtilsForKernel_EC86074D
    move $v1, $v0
    move $ra, $at
    jr $ra
    max $v0, $v0, $v1
    move $t9, $ra
    lui $ra, %hi(sub_1120)
    addiu $ra, $ra, %lo(sub_1120)

sceKernelDcacheReadTagForUser:
    mfic $t0, $0
    mtic $zr, $0
    cache 0x10, 0($a0)
    mfc0 $v0, COP0_STATE_TAG_LO
    mfc0 $v1, COP0_STATE_TAG_HI
    ext $at, $v0, 18, 1
    movn $v0, $zr, $at
    ext $at, $v1, 18, 1
    movn $v1, $zr, $at
    cache 0x12, 0($a0)
    mfc0 $at, COP0_STATE_TAG_LO
    mtic $t0, $0
    jr $ra
    nop

sceKernelDcacheReadTag:
    mfic $t0, $0
    mtic $zr, $0
    cache 0x10, 0($a0)
    mfc0 $v0, COP0_STATE_TAG_LO
    mfc0 $v1, COP0_STATE_TAG_HI
    cache 0x12, 0($a0)
    mfc0 $at, COP0_STATE_TAG_LO
    mtic $t0, $0
    jr $ra
    nop
    move $t9, $ra
    lui $ra, %hi(sub_1120)
    addiu $ra, $ra, %lo(sub_1120)
    j UtilsForUser_DBBE9A46
    nop
    move $t9, $ra
    lui $ra, %hi(sub_1120)
    addiu $ra, $ra, %lo(sub_1120)

sceKernelDcacheProbeRange:
    mfc0 $t8, $24
    andi $t8, $t8, 0x1
    bnez $t8, loc_00000E50
    move $at, $ra

UtilsForUser_DBBE9A46:
    mfc0 $t0, COP0_STATE_CONFIG
    beqz $a1, loc_00000E40
    move $v0, $zr
    li $t1, 4096
    ext $t0, $t0, 6, 3
    sllv $t1, $t1, $t0
    addu $a1, $a0, $a1
    addiu $a1, $a1, -1
    ins $a0, $zr, 0, 6
    ori $a1, $a1, 0x3F
    mfic $v1, $0
    mtic $zr, $0
    srl $t1, $t1, 1
    move $t0, $zr

loc_00000DC0:
    cache 0x10, 0($t0)
    mfc0 $t2, COP0_STATE_TAG_LO
    ext $t3, $t2, 19, 1
    beqz $t3, loc_00000DF4
    sll $t3, $t2, 13
    or $t3, $t3, $t0
    subu $t4, $a1, $t3
    subu $t5, $t3, $a0
    or $t4, $t4, $t5
    bltz $t4, loc_00000E3C
    ext $t3, $t2, 20, 1
    bnez $t3, loc_00000E48
    nop

loc_00000DF4:
    mfc0 $t2, COP0_STATE_TAG_HI
    ext $t3, $t2, 19, 1
    beqz $t3, loc_00000E24
    sll $t3, $t2, 13
    or $t3, $t3, $t0
    subu $t4, $a1, $t3
    subu $t5, $t3, $a0
    or $t4, $t4, $t5
    bltz $t4, loc_00000E3C
    ext $t3, $t2, 20, 1
    bnez $t3, loc_00000E48
    nop

loc_00000E24:
    mtic $v1, $0
    addiu $t0, $t0, 64
    mtic $zr, $0
    bne $t0, $t1, loc_00000DC0
    nop
    li $v0, 1

loc_00000E3C:
    mtic $v1, $0

loc_00000E40:
    jr $ra
    nop

loc_00000E48:
    b loc_00000E3C
    li $v0, 2

loc_00000E50:
    move $t6, $a0
    jal UtilsForUser_DBBE9A46
    move $t7, $a1
    move $a0, $t6
    move $a1, $t7
    jal UtilsForKernel_2F80288E
    move $v1, $v0
    move $ra, $at
    jr $ra
    max $v0, $v0, $v1
    move $t9, $ra
    lui $ra, %hi(sub_1120)
    addiu $ra, $ra, %lo(sub_1120)
    j UtilsForUser_7333E539
    nop
    move $t9, $ra
    lui $ra, %hi(sub_1120)
    addiu $ra, $ra, %lo(sub_1120)

sceKernelIcacheInvalidateAll:
    mfc0 $t8, $24
    andi $t8, $t8, 0x1
    bnez $t8, loc_00000EE8
    move $at, $ra

UtilsForUser_7333E539:
    mfc0 $t0, COP0_STATE_CONFIG
    li $t1, 4096
    ext $t0, $t0, 9, 3
    sllv $t1, $t1, $t0
    mfic $v0, $0
    mtic $zr, $0
    mtc0 $zr, COP0_STATE_TAG_LO
    mtc0 $zr, COP0_STATE_TAG_HI
    move $t0, $zr

loc_00000ECC:
    addiu $t0, $t0, 64
    cache 0x1, -64($t0)
    bne $t0, $t1, loc_00000ECC
    cache 0x3, -64($t0)
    mtic $v0, $0
    jr $ra
    move $v0, $zr

loc_00000EE8:
    jal UtilsForUser_485E430A
    nop
    j UtilsForUser_7333E539
    move $ra, $at
    move $t9, $ra
    lui $ra, %hi(sub_1120)
    addiu $ra, $ra, %lo(sub_1120)
    j UtilsForKernel_43C9A8DB
    nop
    move $t9, $ra
    lui $ra, %hi(sub_1120)
    addiu $ra, $ra, %lo(sub_1120)

UtilsForKernel_C2DF770E:
    mfc0 $t8, $24
    andi $t8, $t8, 0x1
    bnez $t8, loc_00000FF8
    move $at, $ra

UtilsForKernel_43C9A8DB:
    mfc0 $t0, COP0_STATE_CONFIG
    beqz $a1, loc_00000F64
    li $t1, 4096
    ext $t0, $t0, 9, 3
    sllv $t1, $t1, $t0
    sltu $t0, $a1, $t1
    addu $a1, $a0, $a1
    addiu $a1, $a1, -1
    ins $a0, $zr, 0, 6
    beqz $t0, loc_00000F6C
    ori $a1, $a1, 0x3F
    addiu $a1, $a1, 1

loc_00000F58:
    addiu $a0, $a0, 64
    bne $a0, $a1, loc_00000F58
    cache 0x8, -64($a0)

loc_00000F64:
    jr $ra
    move $v0, $zr

loc_00000F6C:
    mfic $v0, $0
    mtic $zr, $0
    srl $t1, $t1, 1
    move $t0, $zr

loc_00000F7C:
    cache 0x0, 0($t0)
    mfc0 $t2, COP0_STATE_TAG_LO
    ext $t3, $t2, 20, 1
    beqz $t3, loc_00000FAC
    sll $t2, $t2, 13
    or $t2, $t2, $t0
    subu $t4, $a1, $t2
    subu $t5, $t2, $a0
    or $t4, $t4, $t5
    bltz $t4, loc_00000FAC
    nop
    cache 0x8, 0($t2)

loc_00000FAC:
    mfc0 $t2, COP0_STATE_TAG_HI
    ext $t3, $t2, 20, 1
    beqz $t3, loc_00000FD8
    sll $t2, $t2, 13
    or $t2, $t2, $t0
    subu $t4, $a1, $t2
    subu $t5, $t2, $a0
    or $t4, $t4, $t5
    bltz $t4, loc_00000FD8
    nop
    cache 0x8, 0($t2)

loc_00000FD8:
    mtic $v0, $0
    addiu $t0, $t0, 64
    mtic $zr, $0
    bne $t0, $t1, loc_00000F7C
    nop
    mtic $v0, $0
    jr $ra
    move $v0, $zr

loc_00000FF8:
    move $a2, $a0
    jal UtilsForUser_7C5DC253
    move $a3, $a1
    move $a0, $a2
    move $a1, $a3
    j UtilsForKernel_43C9A8DB
    move $ra, $at
    move $t9, $ra
    lui $ra, %hi(sub_1120)
    addiu $ra, $ra, %lo(sub_1120)
    j UtilsForUser_F0155BCA
    nop
    move $t9, $ra
    lui $ra, %hi(sub_1120)
    addiu $ra, $ra, %lo(sub_1120)
    lui $t0, 0xA000
    or $ra, $t0, $ra

sceKernelIcacheProbe:
    mfc0 $t8, $24
    andi $t8, $t8, 0x1
    bnez $t8, loc_00001098
    move $at, $ra

UtilsForUser_F0155BCA:
    mfic $v0, $0
    mtic $zr, $0
    nop
    cache 0x0, 0($a0)
    move $t5, $a0
    ins $t5, $zr, 0, 13
    mfc0 $t1, COP0_STATE_TAG_LO
    mfc0 $t2, COP0_STATE_TAG_HI
    mtic $v0, $0
    ext $t3, $t1, 19, 1
    ext $t4, $t2, 19, 1
    sll $t1, $t1, 13
    sll $t2, $t2, 13
    xor $t1, $t1, $t5
    xor $t2, $t2, $t5
    movn $t3, $zr, $t1
    movn $t4, $zr, $t2
    jr $ra
    or $v0, $t3, $t4

loc_00001098:
    jal UtilsForUser_F0155BCA
    nop
    jal UtilsForKernel_EC86074D
    move $v1, $v0
    move $ra, $at
    jr $ra
    or $v0, $v0, $v1
    move $t9, $ra
    lui $ra, %hi(sub_1120)
    addiu $ra, $ra, %lo(sub_1120)

UtilsForUser_5C7F2B1A:
    mfic $t0, $0
    mtic $zr, $0
    cache 0x0, 0($a0)
    mfc0 $v0, COP0_STATE_TAG_LO
    mfc0 $v1, COP0_STATE_TAG_HI
    ext $at, $v0, 18, 1
    movn $v0, $zr, $at
    ext $at, $v1, 18, 1
    movn $v1, $zr, $at
    cache 0x2, 0($a0)
    mfc0 $at, COP0_STATE_TAG_LO
    mtic $t0, $0
    jr $ra
    nop

UtilsForKernel_5C7F2B1A:
    mfic $t0, $0
    mtic $zr, $0
    cache 0x0, 0($a0)
    mfc0 $v0, COP0_STATE_TAG_LO
    mfc0 $v1, COP0_STATE_TAG_HI
    cache 0x2, 0($a0)
    mfc0 $at, COP0_STATE_TAG_LO
    mtic $t0, $0
    jr $ra
    nop

sub_1120:
    move $ra, $t9
    eret
    move $t9, $ra
    lui $ra, %hi(sub_1594)
    addiu $ra, $ra, %lo(sub_1594)

UtilsForUser_0939D45C:
    lui $t0, 0xA7F0
    lui $t1, 0xA000
    ori $t1, $t1, 0x3F
    lui $t2, 0xA7F8
    lui $t3, 0xFFF
    ori $t3, $t3, 0xFFC0
    li $t4, 8

loc_00001150:
    li $t5, 4

loc_00001154:
    lw $t6, 0($t2)
    and $t6, $t6, $t3
    or $t6, $t6, $t1
    sw $t6, 0($t0)
    addiu $t5, $t5, -1
    bnez $t5, loc_00001154
    addiu $t2, $t2, 2048
    addiu $t4, $t4, -1
    bnez $t4, loc_00001150
    addiu $t2, $t2, -8188
    lui $t1, 0x6000
    ori $t1, $t1, 0x3F
    lui $t2, 0x6000
    ori $t2, $t2, 0x103F
    lui $t3, 0x6000
    ori $t3, $t3, 0x203F
    lui $t4, 0x6000
    ori $t4, $t4, 0x303F
    li $t5, 8

loc_000011A0:
    sw $t1, 0($t0)
    sw $t2, 0($t0)
    sw $t3, 0($t0)
    sw $t4, 0($t0)
    addiu $t5, $t5, -1
    bnez $t5, loc_000011A0
    nop
    lw $zr, 0($t0)
    jr $ra
    move $v0, $zr
    move $t9, $ra
    lui $ra, %hi(sub_1594)
    addiu $ra, $ra, %lo(sub_1594)

UtilsForUser_485E430A:
    lui $t0, 0xA7F0
    lui $t1, 0xB000
    ori $t1, $t1, 0x3F
    lui $t2, 0xA7F8
    lui $t3, 0xFFF
    ori $t3, $t3, 0xFFC0
    li $t4, 8

loc_000011F0:
    li $t5, 4

loc_000011F4:
    lw $t6, 0($t2)
    and $t6, $t6, $t3
    or $t6, $t6, $t1
    sw $t6, 0($t0)
    addiu $t5, $t5, -1
    bnez $t5, loc_000011F4
    addiu $t2, $t2, 2048
    addiu $t4, $t4, -1
    bnez $t4, loc_000011F0
    addiu $t2, $t2, -8188
    lui $t1, 0x7000
    ori $t1, $t1, 0x3F
    lui $t2, 0x7000
    ori $t2, $t2, 0x103F
    lui $t3, 0x7000
    ori $t3, $t3, 0x203F
    lui $t4, 0x7000
    ori $t4, $t4, 0x303F
    li $t5, 8

loc_00001240:
    sw $t1, 0($t0)
    sw $t2, 0($t0)
    sw $t3, 0($t0)
    sw $t4, 0($t0)
    addiu $t5, $t5, -1
    bnez $t5, loc_00001240
    nop
    lw $zr, 0($t0)
    jr $ra
    move $v0, $zr
    move $t9, $ra
    lui $ra, %hi(sub_1594)
    addiu $ra, $ra, %lo(sub_1594)

UtilsForKernel_AF0C0173:
    lui $t0, 0xA7F0
    lui $t1, 0x5000
    ori $t1, $t1, 0x3F
    lui $t2, 0x5000
    ori $t2, $t2, 0x103F
    lui $t3, 0x5000
    ori $t3, $t3, 0x203F
    lui $t4, 0x5000
    ori $t4, $t4, 0x303F
    li $t5, 8

loc_0000129C:
    sw $t1, 0($t0)
    sw $t2, 0($t0)
    sw $t3, 0($t0)
    sw $t4, 0($t0)
    addiu $t5, $t5, -1
    bnez $t5, loc_0000129C
    nop
    lw $zr, 0($t0)
    jr $ra
    move $v0, $zr
    move $t9, $ra
    lui $ra, %hi(sub_1594)
    addiu $ra, $ra, %lo(sub_1594)

UtilsForUser_601E43E6:
    lui $v0, 0x2000
    lui $v1, 0xA000
    j loc_00001334
    nop
    move $t9, $ra
    lui $ra, %hi(sub_1594)
    addiu $ra, $ra, %lo(sub_1594)

UtilsForUser_7C5DC253:
    lui $v0, 0x3000
    lui $v1, 0xB000
    j loc_00001334
    nop
    move $t9, $ra
    lui $ra, %hi(sub_1594)
    addiu $ra, $ra, %lo(sub_1594)

UtilsForKernel_F192F2EC:
    or $t0, $a0, $a1
    andi $t0, $t0, 0x3F
    bnez $t0, loc_00001328
    nop
    lui $v0, 0x1000
    lui $v1, 0x9000
    j loc_00001334
    nop

loc_00001328:
    lui $v0, 0x8002
    jr $ra
    ori $v0, $v0, 0x44C

loc_00001334:
    li $t0, 19532
    lui $t1, 0x4C
    ori $t1, $t1, 0x4C
    slt $t2, $zr, $a0
    movn $t0, $t1, $t2
    ext $t1, $a0, 26, 5
    srlv $t0, $t0, $t1
    andi $t0, $t0, 0x1
    beqz $t0, loc_00001458
    move $t0, $a0
    beqz $a1, loc_00001458
    addu $t1, $a0, $a1
    ins $t0, $zr, 27, 5
    ins $t1, $zr, 27, 5
    ins $t0, $t2, 27, 1
    ins $t1, $t2, 27, 1
    lui $t8, 0xA7F0
    andi $t2, $t0, 0x3F
    beqz $t2, loc_0000139C
    ins $t0, $zr, 0, 6
    or $t3, $t0, $v1
    sw $t3, 0($t8)
    addu $a1, $a1, $t2
    addiu $a1, $a1, -64
    blez $a1, loc_00001454
    addiu $t0, $t0, 64

loc_0000139C:
    andi $t2, $t1, 0x3F
    beqz $t2, loc_000013B4
    ins $t1, $zr, 0, 6
    or $t3, $t1, $v1
    sw $t3, 0($t8)
    subu $a1, $a1, $t2

loc_000013B4:
    srl $a1, $a1, 6
    beqz $a1, loc_00001454
    sltiu $t2, $a1, 2048
    beqz $t2, loc_000013FC
    nop

loc_000013C8:
    li $t2, 64
    min $t2, $t2, $a1
    addiu $t3, $t2, -1
    or $t3, $t3, $t0
    or $t3, $t3, $v1
    sw $t3, 0($t8)
    sll $t3, $t2, 6
    subu $a1, $a1, $t2
    bnez $a1, loc_000013C8
    addu $t0, $t0, $t3
    lw $zr, 0($t8)
    jr $ra
    move $v0, $zr

loc_000013FC:
    lui $t2, 0xA7F8
    li $t3, 2048
    lui $t4, 0xFFF
    ori $t4, $t4, 0xFFC0

loc_0000140C:
    lw $t5, 0($t2)
    and $t6, $t5, $v0
    beqz $t6, loc_00001448
    and $t5, $t5, $t4
    sltu $t6, $t5, $t0
    bnez $t6, loc_00001448
    subu $t6, $t1, $t5
    blez $t6, loc_00001448
    srl $t6, $t6, 6
    li $t7, 64
    min $t6, $t6, $t7
    addiu $t6, $t6, -1
    or $t5, $t5, $v1
    ins $t5, $t6, 0, 6
    sw $t5, 0($t8)

loc_00001448:
    addiu $t3, $t3, -1
    bnez $t3, loc_0000140C
    addiu $t2, $t2, 4

loc_00001454:
    lw $zr, 0($t8)

loc_00001458:
    jr $ra
    move $v0, $zr
    move $t9, $ra
    lui $ra, %hi(sub_1594)
    addiu $ra, $ra, %lo(sub_1594)

UtilsForKernel_EC86074D:
    li $t0, 19532
    lui $t1, 0x4C
    ori $t1, $t1, 0x4C
    slt $t2, $zr, $a0
    movn $t0, $t1, $t2
    ext $t1, $a0, 26, 5
    srlv $t0, $t0, $t1
    andi $t0, $t0, 0x1
    beqz $t0, loc_000014D8
    move $v0, $zr
    ins $a0, $zr, 27, 5
    ins $a0, $t2, 27, 1
    ext $t0, $a0, 6, 8
    lui $t1, 0xA7F8
    ins $t1, $t0, 5, 8
    li $t0, 8
    lui $t2, 0xFFF
    ori $t2, $t2, 0xFFC0

loc_000014B4:
    lw $t3, 0($t1)
    ext $t4, $t3, 28, 1
    and $t5, $t3, $t2
    xor $t5, $t5, $a0
    movn $t4, $zr, $t5
    bnez $t4, loc_000014E0
    addiu $t0, $t0, -1
    bnez $t0, loc_000014B4
    addiu $t1, $t1, 4

loc_000014D8:
    jr $ra
    li $v0, 0

loc_000014E0:
    ext $v0, $t3, 29, 1
    jr $ra
    addiu $v0, $v0, 1
    move $t9, $ra
    lui $ra, %hi(sub_1594)
    addiu $ra, $ra, %lo(sub_1594)

UtilsForKernel_2F80288E:
    li $t0, 19532
    lui $t1, 0x4C
    ori $t1, $t1, 0x4C
    slt $t2, $zr, $a0
    movn $t0, $t1, $t2
    ext $t1, $a0, 26, 5
    srlv $t0, $t0, $t1
    andi $t0, $t0, 0x1
    beqz $t0, loc_0000158C
    move $v0, $zr
    beqz $a1, loc_0000158C
    move $t0, $a0
    addu $t1, $a0, $a1
    lui $t3, 0xFFF
    ori $t3, $t3, 0xFFC0
    and $t0, $t0, $t3
    and $t1, $t1, $t3
    ins $t0, $t2, 27, 1
    ins $t1, $t2, 27, 1
    lui $t2, 0xA7F8
    li $t4, 2048
    li $v0, 0

loc_00001550:
    lw $t5, 0($t2)
    ext $t6, $t5, 28, 1
    and $t7, $t5, $t3
    sltu $t8, $t7, $t0
    sltu $t7, $t1, $t7
    or $t7, $t7, $t8
    movn $t6, $zr, $t7
    ext $t5, $t5, 29, 1
    movn $t5, $zr, $t7
    addu $t6, $t5, $t6
    bnez $t5, loc_0000158C
    or $v0, $v0, $t6
    addiu $t4, $t4, -1
    bnez $t4, loc_00001550
    addiu $t2, $t2, 4

loc_0000158C:
    jr $ra
    nop

sub_1594:
    move $ra, $t9
    eret

UtilsForKernel_A6B0A6B8:
    mfc0 $v0, $24
    jr $ra
    nop

UtilsForKernel_39FFB756:
    lui $t0, 0xBFC0
    ori $t0, $t0, 0xF00
    lui $t1, %hi(sub_15E8)
    addiu $t1, $t1, %lo(sub_15E8)
    li $t2, 256

loc_000015BC:
    lw $t3, 0($t1)
    addiu $t1, $t1, 4
    addiu $t2, $t2, -4
    sw $t3, 0($t0)
    bnez $t2, loc_000015BC
    addiu $t0, $t0, 4
    sync
    lui $t0, 0xBFC0
    ori $t0, $t0, 0xF00
    jr $t0
    nop

sub_15E8:
    mfic $t6, $0
    mtic $zr, $0
    mfc0 $t0, COP0_STATE_CONFIG
    li $t1, 2048
    ext $t0, $t0, 6, 3
    sllv $t1, $t1, $t0
    move $t0, $zr

loc_00001604:
    cache 0x14, 0($t0)
    cache 0x14, 0($t0)
    addiu $t0, $t0, 64
    bne $t0, $t1, loc_00001604
    nop
    mfc0 $t0, $24
    sync
    andi $t0, $t0, 0x1
    beqz $t0, loc_0000168C
    nop
    lui $t0, 0xA7F0
    lui $t1, 0x7000
    ori $t1, $t1, 0x3F
    lui $t2, 0x7000
    ori $t2, $t2, 0x103F
    lui $t3, 0x7000
    ori $t3, $t3, 0x203F
    lui $t4, 0x7000
    ori $t4, $t4, 0x303F
    li $t5, 8

loc_00001654:
    sw $t1, 0($t0)
    sw $t2, 0($t0)
    sw $t3, 0($t0)
    sw $t4, 0($t0)
    addiu $t5, $t5, -1
    bnez $t5, loc_00001654
    nop
    lui $t1, 0xF000
    sw $t1, 0($t0)
    lw $zr, 0($t0)
    lui $t1, 0xFFFF
    ori $t1, $t1, 0xFFFF
    sw $t1, 32($t0)
    sync

loc_0000168C:
    mtc0 $zr, $24
    nop
    nop
    mtc0 $a0, $24
    nop
    nop
    nop
    move $v0, $zr
    mfc0 $t0, $24
    xor $t0, $t0, $a0
    andi $t0, $t0, 0x1
    bnezl $t0, loc_000016DC
    li $v0, -1
    andi $t0, $a0, 0x1
    beqz $t0, loc_000016DC
    nop
    lui $t0, 0xA7F0
    lui $t1, 0xF000
    sw $t1, 0($t0)
    lw $zr, 0($t0)

loc_000016DC:
    sync
    jr $ra
    mtic $t6, $0

UtilsForKernel_66463BEC:
    sync
    lui $v0, 0xBD00
    andi $a0, $a0, 0x5
    sw $a0, 4($v0)

loc_000016F8:
    lw $v1, 4($v0)
    and $v1, $v1, $a0
    bnez $v1, loc_000016F8
    nop
    jr $ra
    move $v0, $zr

sub_00001710:
    move $at, $ra
    lw $t0, 0($a0)
    lw $t1, 4($a0)
    lw $t2, 8($a0)
    lw $t3, 12($a0)
    lui $t7, %hi(sub_17B0)
    addiu $t7, $t7, %lo(sub_17B0)
    li $t8, 0
    lui $t9, %hi(g_13298)
    addiu $t9, $t9, %lo(g_13298)
    li $a2, 0
    li $a3, 4
    lui $t6, 0xA0F
    ori $t6, $t6, 0x1419

loc_00001748:
    jalr $t7
    addu $t4, $a1, $a2
    lw $t4, 0($t4)
    addu $a2, $a2, $a3
    andi $a2, $a2, 0x3C
    lw $t5, 0($t9)
    addiu $t9, $t9, 4
    addu $v0, $v0, $t4
    addu $v0, $v0, $t5
    addu $t0, $t0, $v0
    andi $t4, $t6, 0x1F
    rotr $t6, $t6, 8
    rotrv $t0, $t0, $t4
    addu $t0, $t0, $t1
    move $v0, $t3
    move $t3, $t2
    move $t2, $t1
    move $t1, $t0
    move $t0, $v0
    addiu $t8, $t8, 1
    andi $v0, $t8, 0xF
    bnez $v0, loc_00001748
    nop
    addiu $t7, $t7, 16
    jr $t7
    nop

sub_17B0:
    xor $v0, $t2, $t3
    and $v0, $t1, $v0
    jr $ra
    xor $v0, $t3, $v0
    lui $t6, 0xC12
    ori $t6, $t6, 0x171B
    li $a2, 4
    li $a3, 20
    b loc_00001748
    addiu $t7, $t7, 24
    xor $v0, $t1, $t2
    and $v0, $t3, $v0
    jr $ra
    xor $v0, $t2, $v0
    lui $t6, 0x910
    ori $t6, $t6, 0x151C
    li $a2, 20
    li $a3, 12
    b loc_00001748
    addiu $t7, $t7, 24
    xor $v0, $t1, $t2
    jr $ra
    xor $v0, $v0, $t3
    nop
    lui $t6, 0xB11
    ori $t6, $t6, 0x161A
    li $a2, 0
    li $a3, 28
    b loc_00001748
    addiu $t7, $t7, 24
    nor $v0, $zr, $t3
    or $v0, $t1, $v0
    jr $ra
    xor $v0, $t2, $v0
    lw $t4, 0($a0)
    lw $t5, 4($a0)
    lw $t6, 8($a0)
    lw $t7, 12($a0)
    addu $t4, $t4, $t0
    sw $t4, 0($a0)
    addu $t5, $t5, $t1
    addu $t6, $t6, $t2
    sw $t5, 4($a0)
    addu $t7, $t7, $t3
    move $ra, $at
    sw $t6, 8($a0)
    jr $ra
    sw $t7, 12($a0)

sub_00001870:
    addiu $sp, $sp, -64
    sw $ra, 0($sp)
    sw $s0, 4($sp)
    sw $s1, 8($sp)
    sw $s2, 12($sp)
    sw $s3, 16($sp)
    sw $s4, 20($sp)
    sw $s5, 24($sp)
    lw $s0, 0($a1)
    lw $s1, 4($a1)
    lw $s2, 8($a1)
    lw $s3, 12($a1)
    lw $s4, 16($a1)
    jal sub_000018E8
    move $s5, $a1
    sw $s0, 0($s5)
    sw $s1, 4($s5)
    sw $s2, 8($s5)
    sw $s3, 12($s5)
    sw $s4, 16($s5)
    lw $ra, 0($sp)
    lw $s0, 4($sp)
    lw $s1, 8($sp)
    lw $s2, 12($sp)
    lw $s3, 16($sp)
    lw $s4, 20($sp)
    lw $s5, 24($sp)
    jr $ra
    addiu $sp, $sp, 64
    nop

sub_000018E8:
    move $t0, $s0
    move $t1, $s1
    move $t2, $s2
    move $t3, $s3
    move $t4, $s4
    move $v0, $zr

loc_00001900:
    andi $t5, $v0, 0x3C
    sltiu $t6, $v0, 64
    addu $t9, $a0, $t5
    bnez $t6, loc_00001958
    lw $t5, 0($t9)
    addiu $t6, $v0, 8
    addiu $t7, $v0, 32
    addiu $t8, $v0, 52
    andi $t6, $t6, 0x3C
    andi $t7, $t7, 0x3C
    andi $t8, $t8, 0x3C
    addu $t6, $a0, $t6
    addu $t7, $a0, $t7
    lw $t6, 0($t6)
    addu $t8, $a0, $t8
    lw $t7, 0($t7)
    xor $t5, $t5, $t6
    lw $t8, 0($t8)
    xor $t7, $t7, $t8
    xor $t5, $t5, $t7
    rotr $t5, $t5, 31
    sw $t5, 0($t9)

loc_00001958:
    rotr $t6, $t0, 27
    addu $t5, $t5, $t4
    addu $t5, $t5, $t6
    sltiu $t6, $v0, 80
    bnez $t6, loc_000019A0
    sltiu $t6, $v0, 160
    bnez $t6, loc_000019C0
    sltiu $t6, $v0, 240
    bnez $t6, loc_000019E0
    xor $t6, $t1, $t2
    lui $t8, 0xCA62
    xor $t6, $t6, $t3
    ori $t8, $t8, 0xC1D6
    j loc_00001A00
    addiu $v0, $v0, 4
    nop
    nop
    nop

loc_000019A0:
    and $t6, $t1, $t2
    nor $t7, $zr, $t1
    lui $t8, 0x5A82
    and $t7, $t7, $t3
    ori $t8, $t8, 0x7999
    or $t6, $t6, $t7
    j loc_00001A00
    addiu $v0, $v0, 4

loc_000019C0:
    xor $t6, $t1, $t2
    lui $t8, 0x6ED9
    xor $t6, $t6, $t3
    ori $t8, $t8, 0xEBA1
    j loc_00001A00
    addiu $v0, $v0, 4
    nop
    nop

loc_000019E0:
    and $t6, $t1, $t2
    and $t7, $t1, $t3
    lui $t8, 0x8F1B
    or $t6, $t6, $t7
    and $t7, $t2, $t3
    ori $t8, $t8, 0xBCDC
    or $t6, $t6, $t7
    addiu $v0, $v0, 4

loc_00001A00:
    addu $t6, $t6, $t8
    move $t4, $t3
    move $t3, $t2
    sll $t7, $t1, 30
    srl $t1, $t1, 2
    sltiu $t8, $v0, 320
    or $t2, $t7, $t1
    move $t1, $t0
    bnez $t8, loc_00001900
    addu $t0, $t5, $t6
    addu $s0, $t0, $s0
    addu $s1, $t1, $s1
    addu $s2, $t2, $s2
    addu $s3, $t3, $s3
    jr $ra
    addu $s4, $t4, $s4

sub_00001A40:
    addiu $sp, $sp, -256
    sw $s0, 0($sp)
    sw $s1, 4($sp)
    sw $s2, 8($sp)
    sw $s3, 12($sp)
    sw $s4, 16($sp)
    sw $s5, 20($sp)
    sw $s6, 24($sp)
    sw $ra, 28($sp)
    move $s5, $a0
    sll $s6, $a1, 3
    addiu $a0, $sp, 128
    lui $t0, %hi(g_13398)
    addiu $t0, $t0, %lo(g_13398)
    lw $s0, 0($t0)
    andi $t2, $s5, 0xF
    lw $s1, 4($t0)
    andi $t3, $a1, 0x3F
    lw $s2, 8($t0)
    or $t2, $t2, $t3
    lw $s3, 12($t0)
    lw $s4, 16($t0)
    bnez $t2, loc_00001B28
    nop
    beqz $a1, loc_00001AE8
    nop
    nop
    nop

loc_00001AB0:
    li $t0, 16
    move $t1, $a0

loc_00001AB8:
    lw $t2, 0($s5)
    addiu $t0, $t0, -1
    addiu $s5, $s5, 4
    wsbw $t2, $t2
    sw $t2, 0($t1)
    bnez $t0, loc_00001AB8
    addiu $t1, $t1, 4
    addiu $a1, $a1, -64
    jal sub_000018E8
    nop
    bnez $a1, loc_00001AB0
    nop

loc_00001AE8:
    li $v0, 16
    move $t0, $a0

loc_00001AF0:
    addiu $v0, $v0, -4
    sw $zr, 0($t0)
    sw $zr, 4($t0)
    sw $zr, 8($t0)
    sw $zr, 12($t0)
    bnez $v0, loc_00001AF0
    addiu $t0, $t0, 16
    sw $s6, 60($a0)
    lui $t0, 0x8000
    jal sub_000018E8
    sw $t0, 0($a0)
    j loc_00001C88
    nop
    nop

loc_00001B28:
    sltiu $v0, $a1, 64
    bnez $v0, loc_00001B70
    nop
    li $t0, 16
    move $t1, $a0

loc_00001B3C:
    lwr $t2, 0($s5)
    lwl $t2, 3($s5)
    addiu $t0, $t0, -1
    addiu $s5, $s5, 4
    wsbw $t2, $t2
    sw $t2, 0($t1)
    bnez $t0, loc_00001B3C
    addiu $t1, $t1, 4
    jal sub_000018E8
    nop
    j loc_00001B28
    addiu $a1, $a1, -64
    nop

loc_00001B70:
    li $v0, 16
    move $t0, $a0

loc_00001B78:
    addiu $v0, $v0, -4
    sw $zr, 0($t0)
    sw $zr, 4($t0)
    sw $zr, 8($t0)
    sw $zr, 12($t0)
    bnez $v0, loc_00001B78
    addiu $t0, $t0, 16
    sltiu $v0, $a1, 56
    move $t0, $s5
    move $t2, $a1
    bnez $v0, loc_00001C20
    nop
    move $t1, $a0
    nop

loc_00001BB0:
    lbu $t3, 0($t0)
    addiu $t2, $t2, -1
    addiu $t0, $t0, 1
    sb $t3, 0($t1)
    bnez $t2, loc_00001BB0
    addiu $t1, $t1, 1
    li $t3, 128
    sb $t3, 0($t1)
    li $v0, 16
    move $t0, $a0

loc_00001BD8:
    lw $t1, 0($t0)
    addiu $v0, $v0, -1
    wsbw $t1, $t1
    sw $t1, 0($t0)
    bnez $v0, loc_00001BD8
    addiu $t0, $t0, 4
    jal sub_000018E8
    nop
    addiu $a1, $a1, -64
    li $v0, 16
    move $t0, $a0

loc_00001C04:
    addiu $v0, $v0, -4
    sw $zr, 0($t0)
    sw $zr, 4($t0)
    sw $zr, 8($t0)
    sw $zr, 12($t0)
    bnez $v0, loc_00001C04
    addiu $t0, $t0, 16

loc_00001C20:
    bgtz $a1, loc_00001C38
    sw $s6, 60($a0)
    sltiu $t0, $a1, 1
    sll $t0, $t0, 31
    j loc_00001C80
    sw $t0, 0($a0)

loc_00001C38:
    move $t0, $s5
    move $t1, $a0

loc_00001C40:
    lbu $t3, 0($t0)
    addiu $a1, $a1, -1
    addiu $t0, $t0, 1
    sb $t3, 0($t1)
    bnez $a1, loc_00001C40
    addiu $t1, $t1, 1
    li $t0, 128
    sb $t0, 0($t1)
    li $v0, 14
    move $t0, $a0

loc_00001C68:
    lw $t1, 0($t0)
    addiu $v0, $v0, -1
    wsbw $t1, $t1
    sw $t1, 0($t0)
    bnez $v0, loc_00001C68
    addiu $t0, $t0, 4

loc_00001C80:
    jal sub_000018E8
    nop

loc_00001C88:
    wsbw $s0, $s0
    wsbw $s1, $s1
    wsbw $s2, $s2
    wsbw $s3, $s3
    wsbw $s4, $s4
    swr $s0, 0($a2)
    swl $s0, 3($a2)
    swr $s1, 4($a2)
    swl $s1, 7($a2)
    swr $s2, 8($a2)
    swl $s2, 11($a2)
    swr $s3, 12($a2)
    swl $s3, 15($a2)
    swr $s4, 16($a2)
    swl $s4, 19($a2)
    lw $s0, 0($sp)
    lw $s1, 4($sp)
    lw $s2, 8($sp)
    lw $s3, 12($sp)
    lw $s4, 16($sp)
    lw $s5, 20($sp)
    lw $s6, 24($sp)
    lw $ra, 28($sp)
    move $v0, $zr
    jr $ra
    addiu $sp, $sp, 256

sub_00001CF0:
    lw $a1, 0($a0)
    addiu $a3, $a0, 4
    sltiu $t4, $a1, 624
    movz $a1, $zr, $t4
    sll $t0, $a1, 2
    sltiu $t4, $a1, 623
    addu $t0, $a3, $t0
    sltiu $t3, $a1, 227
    beqz $t4, loc_00001DA0
    lw $t1, 0($t0)
    addiu $a1, $a1, 1
    lw $t2, 4($t0)
    lui $t4, 0x8000
    sw $a1, 0($a0)

loc_00001D28:
    srl $t6, $t1, 11
    nor $t5, $zr, $t4
    xor $v0, $t1, $t6
    and $t1, $t1, $t4
    lui $t7, 0x9D2C
    and $t5, $t2, $t5
    ori $t7, $t7, 0x5680
    sll $t6, $v0, 7
    lui $t4, 0x9908
    and $t6, $t6, $t7
    lui $t7, 0xEFC6
    xor $v0, $v0, $t6
    or $t1, $t1, $t5
    sll $t6, $v0, 15
    ori $t4, $t4, 0xB0DF
    and $t6, $t6, $t7
    andi $t2, $t2, 0x1
    xor $v0, $v0, $t6
    srl $t1, $t1, 1
    addiu $t5, $t0, -908
    addiu $t7, $t0, 1588
    xor $t4, $t1, $t4
    movn $t5, $t7, $t3
    movn $t1, $t4, $t2
    lw $t5, 0($t5)
    srl $t6, $v0, 18
    xor $t1, $t1, $t5
    xor $v0, $v0, $t6
    jr $ra
    sw $t1, 0($t0)

loc_00001DA0:
    lw $t2, 4($a0)
    lui $t4, 0x8000
    b loc_00001D28
    sw $zr, 0($a0)
    nop
    nop
    nop
    nop

sceKernelGetGPI:
    lui $v0, %hi(g_13B40)
    lw $v0, %lo(g_13B40)($v0)
    beqz $v0, loc_00001DD8
    lui $a0, %hi(g_13B44)
    jr $v0
    lw $a0, %lo(g_13B44)($a0)

loc_00001DD8:
    jr $ra
    move $v0, $zr

sceKernelSetGPO:
    lui $v0, %hi(g_13B48)
    lw $v0, %lo(g_13B48)($v0)
    beqz $v0, loc_00001DD8
    lui $v1, %hi(g_13B4C)
    lw $v1, %lo(g_13B4C)($v1)
    nor $a1, $zr, $a0
    and $a1, $a1, $v1
    jr $v0
    and $a0, $a0, $v1

sceKernelGetGPO:
    lui $v0, %hi(g_13B48)
    lw $v0, %lo(g_13B48)($v0)
    beqz $v0, loc_00001DD8
    move $a0, $zr
    jr $v0
    move $a1, $zr

sceKernelSetGPIMask:
    lui $t0, %hi(g_13B44)
    nor $t1, $zr, $a0
    lw $v0, %lo(g_13B44)($t0)
    and $v0, $v0, $t1
    or $v0, $v0, $a1
    jr $ra
    sw $v0, %lo(g_13B44)($t0)

sceKernelSetGPOMask:
    lui $t0, %hi(g_13B4C)
    nor $t1, $zr, $a0
    lw $v0, %lo(g_13B4C)($t0)
    and $v0, $v0, $t1
    or $v0, $v0, $a1
    jr $ra
    sw $v0, %lo(g_13B4C)($t0)
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    nop

sceKernelSetPTRIG:
    lui $v0, %hi(g_13B84)
    lw $v0, %lo(g_13B84)($v0)
    beqz $v0, loc_00001E9C
    lui $v1, %hi(g_13B88)
    lw $v1, %lo(g_13B88)($v1)
    jr $v0
    and $a0, $a0, $v1

loc_00001E9C:
    jr $ra
    move $v0, $zr

sceKernelGetPTRIG:
    lui $v0, %hi(g_13B80)
    lw $v0, %lo(g_13B80)($v0)
    beqz $v0, loc_00001E9C
    nop
    jr $v0
    nop

sceKernelSetPTRIGMask:
    lui $v0, %hi(g_13B88)
    jr $ra
    sw $a0, %lo(g_13B88)($v0)

sceKernelPutUserLog:
    lui $v0, %hi(g_13BC0)
    lw $v0, %lo(g_13BC0)($v0)
    beqz $v0, loc_00001EE0
    nop
    jr $v0
    nop

loc_00001EE0:
    jr $ra
    move $v0, $zr

sceKernelRegisterUserLogHandler:
    lui $v0, %hi(g_13BC0)
    jr $ra
    sw $a0, %lo(g_13BC0)($v0)

sceKernelDeflateDecompress:
    addiu $sp, $sp, -1344
    sw $s4, 1228($sp)
    addu $a1, $a0, $a1
    move $t6, $a0
    sw $s5, 1232($sp)
    addiu $s5, $a1, -64
    ins $s5, $zr, 0, 6
    sw $ra, 1236($sp)

loc_00001F14:
    andi $t9, $a2, 0x3
    subu $a2, $a2, $t9
    lw $t8, 0($a2)
    sll $t9, $t9, 3
    addiu $t9, $t9, -32

loc_00001F28:
    addiu $t9, $t9, 3
    blez $t9, loc_00001F4C
    rotrv $v0, $t8, $t9
    addiu $a2, $a2, 4
    lw $t8, 0($a2)
    sllv $v0, $v0, $t9
    addiu $t9, $t9, -32
    ins $v0, $t8, 0, 16
    rotrv $v0, $v0, $t9

loc_00001F4C:
    ext $v1, $v0, 30, 2
    beqz $v1, loc_00002500
    ext $t2, $v0, 29, 1
    lui $t7, %hi(g_133C0)
    addiu $t0, $v1, -2
    beqz $t0, loc_00001FE0
    sh $t2, 1240($sp)
    bgtz $t0, loc_00002560
    addiu $t2, $sp, -108

loc_00001F70:
    lw $t3, %lo(g_133C0)($t7)
    addiu $t2, $t2, 4
    addiu $t7, $t7, 4
    bne $t2, $sp, loc_00001F70
    sw $t3, 104($t2)
    li $t1, 0
    li $s4, 144

loc_00001F8C:
    sh $t1, 108($t2)
    addiu $t0, $t1, 144
    addiu $t1, $t1, 1
    sh $t0, 412($t2)
    bne $t1, $s4, loc_00001F8C
    addiu $t2, $t2, 2
    addiu $s4, $t7, 16

loc_00001FA8:
    lw $t3, 13248($t7)
    addiu $t2, $t2, 4
    addiu $t7, $t7, 4
    bne $s4, $t7, loc_00001FA8
    sw $t3, 104($t2)
    addiu $s4, $t7, 64

loc_00001FC0:
    lw $t3, 13248($t7)
    addiu $t2, $t2, 4
    addiu $t7, $t7, 4
    bne $s4, $t7, loc_00001FC0
    sw $t3, 388($t2)
    sh $t3, 666($sp)
    j loc_000020A0
    sh $t3, 636($sp)

loc_00001FE0:
    addiu $t9, $t9, 14
    blez $t9, loc_00002004
    rotrv $v0, $t8, $t9
    addiu $a2, $a2, 4
    lw $t8, 0($a2)
    sllv $v0, $v0, $t9
    addiu $t9, $t9, -32
    ins $v0, $t8, 0, 16
    rotrv $v0, $v0, $t9

loc_00002004:
    swl $v0, 1261($sp)
    ext $s4, $v0, 28, 4
    addiu $t3, $sp, 1212
    addiu $t0, $t7, -4
    addu $s4, $s4, $t7

loc_00002018:
    beq $t0, $s4, loc_00002060
    addiu $t0, $t0, 1
    lb $t2, 13257($t0)
    addiu $t9, $t9, 3
    blez $t9, loc_00002048
    rotrv $v0, $t8, $t9
    addiu $a2, $a2, 4
    lw $t8, 0($a2)
    sllv $v0, $v0, $t9
    addiu $t9, $t9, -32
    ins $v0, $t8, 0, 16
    rotrv $v0, $v0, $t9

loc_00002048:
    srl $v0, $v0, 29
    beqz $v0, loc_00002018
    ins $t2, $v0, 9, 6
    addiu $t3, $t3, 2
    j loc_00002018
    sh $t2, 58($t3)

loc_00002060:
    jal sub_00002474
    addiu $s4, $sp, 1212
    lh $t5, 1260($sp)
    beqz $t0, loc_00002560
    addiu $s4, $sp, 0
    ext $t5, $t5, 2, 5
    li $at, 0
    jal sub_000022B4
    addiu $t5, $t5, 257
    beqz $t0, loc_00002560
    lh $t5, 1260($sp)
    addiu $s4, $sp, 636
    ext $t5, $t5, 7, 5
    li $at, 288
    jal sub_000022B4
    addiu $t5, $t5, 289

loc_000020A0:
    lb $t5, 636($sp)
    lhu $t1, 30($sp)
    j loc_000020BC
    lb $t4, 0($sp)

loc_000020B0:
    beq $a0, $a1, loc_00002578
    addiu $a0, $a0, 1

loc_000020B8:
    sb $v0, -1($a0)

loc_000020BC:
    jal sub_00002258
    addiu $t0, $sp, 2
    addiu $t0, $sp, 638
    bgez $v0, loc_000020B0
    bitrev $t2, $v0
    bltzal $t2, sub_000021B0
    sra $s4, $v0, 4
    lhu $t1, 666($sp)
    subu $at, $a0, $s4
    srlv $v0, $t8, $t9
    jal sub_00002210
    subu $t9, $t9, $t5
    sltu $v1, $s5, $at
    bnez $v1, loc_0000256C
    clz $t2, $v0
    cache 0x18, 63($a0)

loc_000020FC:
    addiu $t2, $t2, -30
    bltzal $t2, sub_00002194
    subu $s4, $a0, $v0
    sltu $t2, $t6, $s4
    beqz $t2, loc_00002560
    lwr $v0, -1($s4)
    lhu $t1, 30($sp)
    bnez $v1, loc_00002154
    subu $v1, $a0, $s4
    sltiu $v1, $v1, 3
    bnez $v1, loc_00002154
    addiu $v1, $a0, -3
    move $a0, $at
    ins $at, $v1, 0, 2

loc_00002134:
    lwl $v0, 2($s4)
    swr $v0, 3($v1)
    addiu $v1, $v1, 4
    beq $v1, $at, loc_000020BC
    swl $v0, 2($v1)
    addiu $s4, $s4, 4
    j loc_00002134
    lwr $v0, -1($s4)

loc_00002154:
    addiu $a0, $a0, 1

loc_00002158:
    sb $v0, -1($a0)
    addiu $a0, $a0, 1
    beq $at, $a0, loc_000020B8
    lbu $v0, 0($s4)
    j loc_00002158
    addiu $s4, $s4, 1

loc_00002170:
    addiu $a2, $a2, 4
    lw $t8, 0($a2)
    sllv $v0, $v0, $t9
    addiu $t9, $t9, -32
    ins $v0, $t8, 0, 16
    rotrv $v0, $v0, $t9
    srlv $v0, $v0, $t2
    jr $ra
    subu $s4, $s4, $v0

sub_00002194:
    bltz $v0, loc_00002560
    subu $t9, $t9, $t2
    bgtz $t9, loc_00002170
    rotrv $v0, $t8, $t9
    srlv $v0, $v0, $t2
    jr $ra
    subu $s4, $s4, $v0

sub_000021B0:
    sra $t2, $t2, 28
    beq $s4, $t2, loc_000021D0
    subu $t9, $t9, $t2
    bgtz $t9, loc_00002170
    rotrv $v0, $t8, $t9
    srlv $v0, $v0, $t2
    jr $ra
    subu $s4, $s4, $v0

loc_000021D0:
    addiu $v0, $v0, 1
    beqz $v0, loc_00002560
    lh $t0, 1240($sp)
    beqz $t0, loc_00001F28
    addu $t9, $t9, $t2

loc_000021E4:
    beqz $a3, loc_000021FC
    subu $v0, $a0, $t6
    addiu $t9, $t9, 39
    srl $t1, $t9, 3
    addu $a2, $a2, $t1
    sw $a2, 0($a3)

loc_000021FC:
    lw $ra, 1236($sp)
    lw $s5, 1232($sp)
    lw $s4, 1228($sp)
    jr $ra
    addiu $sp, $sp, 1344

sub_00002210:
    blez $t9, loc_00002268
    sllv $v0, $v0, $t5

loc_00002218:
    addiu $a2, $a2, 4
    lw $t8, 0($a2)
    sllv $v0, $v0, $t9
    addiu $t9, $t9, -32
    ins $v0, $t8, 0, 16
    rotrv $v0, $v0, $t9
    j loc_00002268
    ins $v0, $zr, 0, 16

loc_00002238:
    addu $v0, $v0, $t0
    jr $ra
    lh $v0, 58($v0)

loc_00002244:
    lhu $t2, 0($t0)
    addu $v0, $v0, $t0
    subu $v0, $v0, $t2
    jr $ra
    lh $v0, 60($v0)

sub_00002258:
    srlv $v0, $t8, $t9
    subu $t9, $t9, $t4
    bgtz $t9, loc_00002218
    sllv $v0, $v0, $t4

loc_00002268:
    bitrev $v0, $v0
    sltu $t2, $v0, $t1
    bnez $t2, loc_00002238
    addu $v0, $v0, $v0

loc_00002278:
    beqz $t9, loc_000022A0
    lhu $v1, 30($t0)
    srlv $t2, $t8, $t9
    ins $v0, $t2, 0, 1
    addiu $t9, $t9, 1

loc_0000228C:
    sltu $v1, $v0, $v1
    bnez $v1, loc_00002244
    addu $v0, $v0, $v0
    j loc_00002278
    addiu $t0, $t0, 2

loc_000022A0:
    addiu $a2, $a2, 4
    lw $t8, 0($a2)
    li $t9, -31
    j loc_0000228C
    ins $v0, $t8, 0, 1

sub_000022B4:
    sw $zr, 56($s4)
    sw $zr, 44($s4)
    move $t3, $s4
    mthi $ra
    lb $t4, 1212($sp)
    sw $zr, 52($s4)

loc_000022CC:
    andi $t0, $at, 0x1FF

loc_000022D0:
    subu $t0, $t0, $t5
    bgez $t0, loc_0000239C
    lhu $t1, 1242($sp)
    jal sub_00002258
    addiu $t0, $sp, 1214
    addiu $t0, $v0, -16
    blez $t0, loc_0000232C
    li $v1, 7
    min $t2, $t0, $v1
    addu $t9, $t9, $t2
    blez $t9, loc_00002318
    rotrv $v0, $t8, $t9
    addiu $a2, $a2, 4
    lw $t8, 0($a2)
    sllv $v0, $v0, $t9
    addiu $t9, $t9, -32
    ins $v0, $t8, 0, 16
    rotrv $v0, $v0, $t9

loc_00002318:
    negu $t2, $t2
    srlv $v0, $v0, $t2
    addu $t0, $t0, $v0

loc_00002324:
    j loc_000022CC
    addu $at, $at, $t0

loc_0000232C:
    beqz $t0, loc_00002348
    addiu $t2, $t3, 2
    ins $at, $v0, 9, 6
    sh $at, 60($t3)
    addiu $at, $at, 1
    j loc_000022CC
    movn $t3, $t2, $v0

loc_00002348:
    blez $at, loc_00002560
    addiu $t9, $t9, 2
    blez $t9, loc_00002370
    rotrv $v0, $t8, $t9
    addiu $a2, $a2, 4
    lw $t8, 0($a2)
    sllv $v0, $v0, $t9
    addiu $t9, $t9, -32
    ins $v0, $t8, 0, 16
    rotrv $v0, $v0, $t9

loc_00002370:
    srl $v0, $v0, 30
    andi $t0, $at, 0x1FF
    beq $t0, $at, loc_00002324
    addiu $t0, $v0, 3
    addu $t0, $t0, $at

loc_00002384:
    sh $at, 60($t3)
    addiu $at, $at, 1
    bne $at, $t0, loc_00002384
    addiu $t3, $t3, 2
    j loc_000022D0
    andi $t0, $at, 0x1FF

loc_0000239C:
    sw $zr, 48($s4)
    bnez $t0, loc_00002560
    li $ra, 32767
    sw $zr, 32($s4)
    addiu $t1, $s4, 2
    li $at, -1
    sw $zr, 40($s4)
    move $t4, $s4
    move $t5, $s4

loc_000023C0:
    lh $v0, 60($t4)
    addiu $t4, $t4, 2
    sh $v0, 634($t4)
    bne $t4, $t3, loc_000023C0
    min $ra, $ra, $v0
    sw $zr, 36($s4)
    andi $ra, $ra, 0x7E00

loc_000023DC:
    addiu $ra, $ra, 512
    move $v1, $s4

loc_000023E4:
    beq $t3, $t5, loc_0000245C
    li $t2, -1

loc_000023EC:
    lhu $v0, 636($v1)
    beq $t3, $v1, loc_000023DC
    min $v0, $v0, $ra
    beq $v0, $ra, loc_000023EC
    addiu $v1, $v1, 2
    addiu $at, $at, 1
    sh $t2, 634($v1)
    srl $t4, $v0, 9
    rotr $v0, $v0, 9
    bgtz $v0, loc_00002420
    ext $v0, $v0, 23, 9
    ins $t7, $v0, 1, 6
    lh $v0, 13308($t7)

loc_00002420:
    sh $v0, 60($t5)
    beq $t0, $t4, loc_000023E4
    addiu $t5, $t5, 2
    subu $t2, $t0, $t4
    beqz $t0, loc_00002454
    move $t0, $t4
    sh $at, 28($t1)
    rotrv $at, $at, $t2
    addu $t2, $t2, $t2
    subu $t1, $t1, $t2
    subu $t2, $t5, $at
    subu $t2, $t2, $at
    subu $t2, $t1, $t2

loc_00002454:
    j loc_000023E4
    sh $t2, -2($t1)

loc_0000245C:
    mfhi $ra

loc_00002460:
    srlv $v0, $at, $t0
    bnez $v0, loc_00002560
    li $at, -1
    jr $ra
    sh $at, 28($t1)

sub_00002474:
    sw $zr, 32($s4)
    move $t0, $zr
    move $t5, $s4
    sw $zr, 36($s4)
    addiu $t1, $s4, 2
    li $at, -1
    sw $zr, 40($s4)

loc_00002490:
    beq $t3, $t5, loc_00002460
    addiu $t4, $t5, 2
    addiu $at, $at, 1
    lh $v0, 60($t5)

loc_000024A0:
    lh $v1, 60($t4)
    beq $t3, $t4, loc_000024BC
    addiu $t4, $t4, 2
    max $t2, $v1, $v0
    sh $t2, 58($t4)
    j loc_000024A0
    min $v0, $v1, $v0

loc_000024BC:
    srl $t4, $v0, 9
    andi $v0, $v0, 0x1FF
    sh $v0, 60($t5)
    beq $t0, $t4, loc_00002490
    addiu $t5, $t5, 2
    subu $t2, $t0, $t4
    beqz $t0, loc_000024F8
    move $t0, $t4
    sh $at, 28($t1)
    rotrv $at, $at, $t2
    addu $t2, $t2, $t2
    subu $t1, $t1, $t2
    subu $t2, $t5, $at
    subu $t2, $t2, $at
    subu $t2, $t1, $t2

loc_000024F8:
    j loc_00002490
    sh $t2, -2($t1)

loc_00002500:
    negu $t0, $t9
    srl $t0, $t0, 3
    subu $a2, $a2, $t0
    lwl $t0, 7($a2)
    lwr $t0, 4($a2)
    andi $v1, $t0, 0xFFFF
    addu $v1, $v1, $a0
    sltu $t1, $a1, $v1
    bnez $t1, loc_00002578
    not $t1, $t0
    rotr $t1, $t1, 16
    bne $t1, $t0, loc_00002560
    nop
    beq $v1, $a0, loc_00002550
    nop

loc_0000253C:
    lbu $t1, 8($a2)
    addiu $a2, $a2, 1
    addiu $a0, $a0, 1
    bne $v1, $a0, loc_0000253C
    sb $t1, -1($a0)

loc_00002550:
    bnez $t2, loc_000021E4
    li $t9, 32
    j loc_00001F14
    addiu $a2, $a2, 8

loc_00002560:
    lui $v0, 0x8000
    j loc_000021FC
    ori $v0, $v0, 0x108

loc_0000256C:
    sltu $s4, $a1, $at
    beqz $s4, loc_000020FC
    nop

loc_00002578:
    lui $v0, 0x8000
    j loc_000021FC
    ori $v0, $v0, 0x104

UtilsForKernel_6C6887EE:
    addiu $sp, $sp, -2832
    sw $s2, 2816($sp)
    addu $t4, $a0, $a1
    addiu $t3, $a0, 0
    sw $s1, 2812($sp)
    li $t2, 5
    addiu $a1, $sp, -2808
    sw $s0, 2808($sp)
    move $t9, $zr
    li $t0, 1
    sw $s3, 2820($sp)
    li $s3, -1
    addiu $at, $sp, 56
    sw $s4, 2824($sp)
    addiu $s4, $t4, -64
    ins $s4, $zr, 0, 6
    lb $s0, 0($a2)
    lwl $t1, 4($a2)
    lwr $t1, 1($a2)
    sw $s5, 2828($sp)
    bltz $s0, loc_00002E60
    wsbw $t1, $t1
    ext $s5, $s0, 3, 2
    sll $s5, $s5, 4
    li $s1, 128
    subu $s5, $s1, $s5
    ins $s5, $s5, 8, 8
    andi $s0, $s0, 0x7
    ins $s5, $s5, 16, 16

loc_000025F8:
    sw $s5, 2808($a1)
    addiu $a1, $a1, 8
    bne $a1, $sp, loc_000025F8
    sw $s5, 2804($a1)
    j loc_000026E0
    li $s5, 255

loc_00002610:
    lbu $s2, 5($a2)
    sll $t1, $t1, 8
    addiu $a2, $a2, 1
    addu $t1, $t1, $s2
    mult $s3, $t6
    srl $s2, $t6, 3
    subu $t6, $t6, $s2
    mflo $t5
    sltu $t8, $t1, $t5
    bnez $t8, loc_00002684
    srl $s2, $t9, 8
    sll $s3, $s3, 8

loc_00002640:
    subu $t1, $t1, $t5
    sb $t6, -1($t7)
    bnez $s2, loc_00002698
    subu $s3, $s3, $t5

loc_00002650:
    addu $t7, $v1, $t9
    srl $t5, $s3, 24
    lbu $t6, -1($t7)
    beqz $t5, loc_00002610
    sll $t9, $t9, 1
    srl $s2, $s3, 8
    mult $s2, $t6
    srl $s2, $t6, 3
    subu $t6, $t6, $s2
    mflo $t5
    sltu $t8, $t1, $t5
    beqz $t8, loc_00002640
    srl $s2, $t9, 8

loc_00002684:
    mflo $s3
    addiu $t6, $t6, 31
    sb $t6, -1($t7)
    beqz $s2, loc_00002650
    addiu $t9, $t9, 1

loc_00002698:
    srl $s1, $s3, 24
    lbu $t5, 2488($a1)
    beqz $s1, loc_00002714
    sb $t9, 0($t3)

loc_000026A8:
    srl $v1, $s3, 8
    mult $v1, $t5
    addiu $t3, $t3, 1
    srl $v1, $t5, 4
    mflo $s1
    sltu $t6, $t1, $s1
    bnez $t6, loc_00002744
    subu $v1, $t5, $v1

loc_000026C8:
    sb $v1, 2488($a1)
    addiu $a1, $a1, -1
    max $a1, $a1, $sp
    subu $t1, $t1, $s1
    beq $t3, $t4, loc_00002E54
    subu $s3, $s3, $s1

loc_000026E0:
    ins $t9, $t3, 8, 3
    srav $v1, $t9, $s0
    andi $v1, $v1, 0x7
    sll $t9, $v1, 8
    subu $t8, $t9, $v1
    addu $v1, $sp, $t8
    j loc_00002650
    li $t9, 1

loc_00002700:
    move $t3, $v0
    lbu $t9, 0($t3)
    srl $s1, $s3, 24
    bnez $s1, loc_000026A8
    lbu $t5, 2488($a1)

loc_00002714:
    lbu $t8, 5($a2)
    addiu $t3, $t3, 1
    sll $t1, $t1, 8
    addiu $a2, $a2, 1
    addu $t1, $t1, $t8
    mult $s3, $t5
    sll $s3, $s3, 8
    srl $v1, $t5, 4
    mflo $s1
    sltu $t6, $t1, $s1
    beqz $t6, loc_000026C8
    subu $v1, $t5, $v1

loc_00002744:
    li $t8, -1

loc_00002748:
    addiu $v1, $v1, 15
    lbu $t5, 2496($a1)
    srl $t9, $s1, 24
    beqz $t9, loc_00002C7C
    mflo $s3
    sb $v1, 2488($a1)
    srl $s2, $s3, 8
    mult $s2, $t5
    addiu $a1, $a1, 8
    srl $s2, $t5, 4
    mflo $s1
    sltu $t9, $t1, $s1
    bnez $t9, loc_00002CB0
    subu $v1, $t5, $s2

loc_00002780:
    subu $s3, $s3, $s1
    bltz $t8, loc_00002928
    subu $t1, $t1, $s1

loc_0000278C:
    sb $v1, 2488($a1)
    sllv $t9, $t3, $t8
    sll $s1, $t8, 5
    ins $s1, $t9, 3, 2
    ins $s1, $a1, 0, 3
    addu $s1, $sp, $s1
    addiu $t9, $t8, -3
    lbu $a1, 2552($s1)
    srl $v1, $s3, 24
    bltzl $t9, loc_00002850
    li $t6, 1
    lbu $s2, 2576($s1)
    beqz $v1, loc_00002DA4
    srl $v0, $s3, 8
    mult $v0, $s2

loc_000027C8:
    srl $v0, $s2, 3
    subu $s2, $s2, $v0
    mflo $v1
    sltu $v0, $t1, $v1
    bnez $v0, loc_00002DF4
    addiu $t6, $v0, 2
    subu $t1, $t1, $v1
    subu $s3, $s3, $v1
    blez $t9, loc_0000284C
    srl $v1, $s3, 24
    beqz $v1, loc_00002E0C
    srl $v0, $s3, 8

loc_000027F8:
    mult $v0, $s2
    srl $v0, $s2, 3
    subu $s2, $s2, $v0
    mflo $t7
    sltu $v0, $t1, $t7
    bnez $v0, loc_00002DC0
    sll $t6, $t6, 1
    subu $s3, $s3, $t7
    srl $v1, $s3, 24
    beq $t9, $t0, loc_0000284C
    subu $t1, $t1, $t7
    beqz $v1, loc_00002DDC

loc_00002828:
    sll $t6, $t6, 1

loc_0000282C:
    srl $s3, $s3, 1

loc_00002830:
    sltu $v0, $t1, $s3
    subu $v1, $t1, $s3
    movz $t1, $v1, $v0
    addiu $t9, $t9, -1
    bne $t9, $t0, loc_00002828
    addu $t6, $t6, $v0
    srl $v1, $s3, 24

loc_0000284C:
    sb $s2, 2576($s1)

loc_00002850:
    beqz $v1, loc_00002D58
    srl $v0, $s3, 8
    mult $v0, $a1
    srl $v0, $a1, 3
    subu $a1, $a1, $v0
    mflo $v1
    sltu $v0, $t1, $v1
    bnez $v0, loc_00002D88
    sll $t6, $t6, 1

loc_00002874:
    sb $a1, 2552($s1)
    subu $t1, $t1, $v1
    blez $t8, loc_0000292C
    subu $s3, $s3, $v1
    lbu $a1, 2560($s1)

loc_00002888:
    srl $v1, $s3, 24
    beqz $v1, loc_00002D3C
    srl $v0, $s3, 8
    mult $v0, $a1

loc_00002898:
    srl $v0, $a1, 3
    subu $a1, $a1, $v0
    mflo $v1
    sltu $v0, $t1, $v1
    bnez $v0, loc_00002D20
    sll $t6, $t6, 1
    sb $a1, 2560($s1)
    subu $t1, $t1, $v1
    beq $t8, $t0, loc_000028F8
    subu $s3, $s3, $v1
    lbu $a1, 2568($s1)

loc_000028C4:
    srl $v1, $s3, 24
    beqz $v1, loc_00002CC4
    srl $v0, $s3, 8
    mult $v0, $a1

loc_000028D4:
    srl $v0, $a1, 3
    subu $a1, $a1, $v0
    mflo $v1
    sltu $v0, $t1, $v1
    bnez $v0, loc_00002CE0
    sll $t6, $t6, 1
    subu $t1, $t1, $v1
    subu $s3, $s3, $v1

loc_000028F4:
    sb $a1, 2568($s1)

loc_000028F8:
    addu $t5, $at, $t8
    li $s2, 256
    li $s1, 8
    srl $v1, $s3, 24
    bnez $v1, loc_00002944
    addu $t9, $t5, $s1

loc_00002910:
    lbu $v1, 5($a2)
    sll $t1, $t1, 8
    addiu $a2, $a2, 1
    addu $t1, $t1, $v1
    j loc_00002944
    sll $s3, $s3, 8

loc_00002928:
    sb $v1, 2488($a1)

loc_0000292C:
    li $s2, 64
    li $s1, 8
    addu $t5, $sp, $t8

loc_00002938:
    srl $v1, $s3, 24
    beqz $v1, loc_00002910
    addu $t9, $t5, $s1

loc_00002944:
    lbu $t8, 2033($t9)
    srl $v0, $s3, 8
    sll $s1, $s1, 1
    mult $v0, $t8
    srl $v0, $t8, 3
    subu $t7, $t8, $v0
    mflo $v1
    sltu $t8, $t1, $v1
    beqz $t8, loc_00002E28
    subu $t8, $s1, $s2
    mflo $s3
    addiu $t7, $t7, 31
    sb $t7, 2033($t9)
    bltz $t8, loc_00002938
    addiu $s1, $s1, 8

loc_00002980:
    addu $s1, $sp, $t8
    sra $t8, $t8, 3
    addiu $t9, $t8, -3
    lw $t5, 2344($s1)
    bltzl $t9, loc_00002A30
    li $t7, 1
    srl $v0, $s3, 24
    beqz $v0, loc_00002BF8
    ext $s2, $t5, 24, 8
    srl $v0, $s3, 8
    mult $v0, $s2

loc_000029AC:
    srl $v0, $s2, 3
    subu $s2, $s2, $v0
    mflo $v1
    sltu $v0, $t1, $v1
    bnez $v0, loc_00002C48
    addiu $t7, $v0, 2
    subu $t1, $t1, $v1
    blez $t9, loc_00002A2C
    subu $s3, $s3, $v1
    srl $v0, $s3, 24
    beqz $v0, loc_00002C60
    srl $v0, $s3, 8

loc_000029DC:
    mult $v0, $s2
    srl $v0, $s2, 3
    subu $s2, $s2, $v0
    mflo $v1
    sltu $v0, $t1, $v1
    bnez $v0, loc_00002C14
    sll $t7, $t7, 1
    subu $t1, $t1, $v1
    beq $t9, $t0, loc_00002A2C
    subu $s3, $s3, $v1
    srl $v0, $s3, 24
    beqz $v0, loc_00002C30

loc_00002A0C:
    sll $t7, $t7, 1

loc_00002A10:
    srl $s3, $s3, 1

loc_00002A14:
    sltu $v0, $t1, $s3
    subu $v1, $t1, $s3
    addiu $t9, $t9, -1
    movz $t1, $v1, $v0
    bne $t9, $t0, loc_00002A0C
    addu $t7, $t7, $v0

loc_00002A2C:
    ins $t5, $s2, 24, 8

loc_00002A30:
    srl $v0, $s3, 24
    beqz $v0, loc_00002BDC
    ext $s2, $t5, 0, 8
    srl $v0, $s3, 8
    mult $v0, $s2

loc_00002A44:
    srl $v0, $s2, 3
    subu $s2, $s2, $v0
    mflo $v1
    sltu $v0, $t1, $v1
    bnez $v0, loc_00002BA8
    sll $t7, $t7, 1
    ins $t5, $s2, 0, 8
    subu $t1, $t1, $v1
    blez $t8, loc_00002AE0
    subu $s3, $s3, $v1

loc_00002A6C:
    srl $v0, $s3, 24
    beqz $v0, loc_00002BC0
    ext $s2, $t5, 8, 8
    srl $v0, $s3, 8
    mult $v0, $s2

loc_00002A80:
    srl $v0, $s2, 3
    subu $s2, $s2, $v0
    mflo $v1
    sltu $v0, $t1, $v1
    bnez $v0, loc_00002B90
    sll $t7, $t7, 1
    ins $t5, $s2, 8, 8
    subu $t1, $t1, $v1
    beq $t8, $t0, loc_00002AE0
    subu $s3, $s3, $v1

loc_00002AA8:
    srl $v0, $s3, 24
    beqz $v0, loc_00002B64
    ext $s2, $t5, 16, 8
    srl $v0, $s3, 8
    mult $v0, $s2

loc_00002ABC:
    srl $v0, $s2, 3
    subu $s2, $s2, $v0
    mflo $v1
    sltu $v0, $t1, $v1
    bnez $v0, loc_00002B80
    sll $t7, $t7, 1
    ins $t5, $s2, 16, 8
    subu $t1, $t1, $v1
    subu $s3, $s3, $v1

loc_00002AE0:
    addiu $t7, $t7, -1

loc_00002AE4:
    subu $t9, $t3, $a0
    sltu $t9, $t7, $t9
    beqz $t9, loc_00002E48
    sw $t5, 2344($s1)

loc_00002AF4:
    ori $a1, $sp, 0x6
    addu $v0, $t3, $t6
    sltu $s1, $s4, $v0
    bnez $s1, loc_00002B40
    subu $v1, $t3, $t7
    cache 0x18, 63($t3)
    slti $s1, $t7, 3
    bnez $s1, loc_00002B4C
    ins $a1, $v0, 0, 1
    ins $t6, $zr, 0, 2
    addu $t6, $t3, $t6

loc_00002B20:
    lwl $t9, 2($v1)
    lwr $t9, -1($v1)
    swl $t9, 3($t3)
    addiu $v1, $v1, 4
    beq $t3, $t6, loc_00002700
    swr $t9, 0($t3)
    j loc_00002B20
    addiu $t3, $t3, 4

loc_00002B40:
    sltu $s1, $v0, $t4
    beqz $s1, loc_00002E54
    ins $a1, $v0, 0, 1

loc_00002B4C:
    beq $t3, $v0, loc_00002698
    lbu $t9, -1($v1)
    sb $t9, 0($t3)
    addiu $t3, $t3, 1
    j loc_00002B4C
    addiu $v1, $v1, 1

loc_00002B64:
    lbu $v1, 5($a2)
    sll $t1, $t1, 8
    addiu $a2, $a2, 1
    addu $t1, $t1, $v1
    mult $s3, $s2
    j loc_00002ABC
    sll $s3, $s3, 8

loc_00002B80:
    addiu $s2, $s2, 31
    ins $t5, $s2, 16, 8
    j loc_00002AE4
    mflo $s3

loc_00002B90:
    addiu $s2, $s2, 31
    ins $t5, $s2, 8, 8
    beq $t8, $t0, loc_00002AE4
    mflo $s3
    j loc_00002AA8
    addiu $t7, $t7, 1

loc_00002BA8:
    addiu $s2, $s2, 31
    ins $t5, $s2, 0, 8
    blez $t8, loc_00002AE4
    mflo $s3
    j loc_00002A6C
    addiu $t7, $t7, 1

loc_00002BC0:
    lbu $v1, 5($a2)
    sll $t1, $t1, 8
    addiu $a2, $a2, 1
    addu $t1, $t1, $v1
    mult $s3, $s2
    j loc_00002A80
    sll $s3, $s3, 8

loc_00002BDC:
    lbu $v1, 5($a2)
    sll $t1, $t1, 8
    addiu $a2, $a2, 1
    addu $t1, $t1, $v1
    mult $s3, $s2
    j loc_00002A44
    sll $s3, $s3, 8

loc_00002BF8:
    lbu $v1, 5($a2)
    sll $t1, $t1, 8
    addiu $a2, $a2, 1
    addu $t1, $t1, $v1
    mult $s3, $s2
    j loc_000029AC
    sll $s3, $s3, 8

loc_00002C14:
    mflo $s3
    addiu $t7, $t7, 1
    beq $t9, $t0, loc_00002A2C
    addiu $s2, $s2, 31
    srl $v0, $s3, 24
    bnez $v0, loc_00002A10
    sll $t7, $t7, 1

loc_00002C30:
    lbu $v1, 5($a2)
    sll $t1, $t1, 8
    addiu $a2, $a2, 1
    addu $t1, $t1, $v1
    j loc_00002A14
    sll $s3, $s3, 7

loc_00002C48:
    mflo $s3
    blez $t9, loc_00002A2C
    addiu $s2, $s2, 31
    srl $v0, $s3, 24
    bnez $v0, loc_000029DC
    srl $v0, $s3, 8

loc_00002C60:
    lbu $v1, 5($a2)
    sll $t1, $t1, 8
    addiu $a2, $a2, 1
    addu $t1, $t1, $v1
    sll $s3, $s3, 8
    j loc_000029DC
    srl $v0, $s3, 8

loc_00002C7C:
    lbu $s2, 5($a2)
    sb $v1, 2488($a1)
    sll $t1, $t1, 8
    addiu $a2, $a2, 1
    addu $t1, $t1, $s2
    addiu $a1, $a1, 8
    mult $s3, $t5
    sll $s3, $s1, 8
    srl $s2, $t5, 4
    mflo $s1
    sltu $t9, $t1, $s1
    beqz $t9, loc_00002780
    subu $v1, $t5, $s2

loc_00002CB0:
    bne $t8, $t2, loc_00002748
    addiu $t8, $t8, 1
    addiu $v1, $v1, 15
    j loc_0000278C
    mflo $s3

loc_00002CC4:
    lbu $v1, 5($a2)
    sll $t1, $t1, 8
    addiu $a2, $a2, 1
    addu $t1, $t1, $v1
    mult $s3, $a1
    j loc_000028D4
    sll $s3, $s3, 8

loc_00002CE0:
    addiu $a1, $a1, 31
    addiu $t6, $t6, 1
    bne $t6, $s5, loc_000028F4
    mflo $s3

loc_00002CF0:
    subu $v0, $t3, $a0
    beqz $a3, loc_00002D00
    addiu $a2, $a2, 5
    sw $a2, 0($a3)

loc_00002D00:
    lw $s5, 2828($sp)
    lw $s4, 2824($sp)
    lw $s3, 2820($sp)
    lw $s2, 2816($sp)
    lw $s1, 2812($sp)
    lw $s0, 2808($sp)
    jr $ra
    addiu $sp, $sp, 2832

loc_00002D20:
    addiu $a1, $a1, 31
    sb $a1, 2560($s1)
    addiu $t6, $t6, 1
    beq $t8, $t0, loc_000028F8
    mflo $s3
    j loc_000028C4
    lbu $a1, 2568($s1)

loc_00002D3C:
    lbu $v1, 5($a2)
    sll $t1, $t1, 8
    addiu $a2, $a2, 1
    addu $t1, $t1, $v1
    mult $s3, $a1
    j loc_00002898
    sll $s3, $s3, 8

loc_00002D58:
    lbu $v1, 5($a2)
    sll $t1, $t1, 8
    addiu $a2, $a2, 1
    addu $t1, $t1, $v1
    mult $s3, $a1
    sll $s3, $s3, 8
    srl $v0, $a1, 3
    subu $a1, $a1, $v0
    mflo $v1
    sltu $v0, $t1, $v1
    beqz $v0, loc_00002874
    sll $t6, $t6, 1

loc_00002D88:
    addiu $a1, $a1, 31
    sb $a1, 2552($s1)
    mflo $s3
    blez $t8, loc_000028F8
    addiu $t6, $t6, 1
    j loc_00002888
    lbu $a1, 2560($s1)

loc_00002DA4:
    lbu $v1, 5($a2)
    sll $t1, $t1, 8
    addiu $a2, $a2, 1
    addu $t1, $t1, $v1
    mult $s3, $s2
    j loc_000027C8
    sll $s3, $s3, 8

loc_00002DC0:
    mflo $s3
    addiu $t6, $t6, 1
    addiu $s2, $s2, 31
    beq $t9, $t0, loc_0000284C
    srl $v1, $s3, 24
    bnez $v1, loc_0000282C
    sll $t6, $t6, 1

loc_00002DDC:
    lbu $v1, 5($a2)
    sll $t1, $t1, 8
    addiu $a2, $a2, 1
    addu $t1, $t1, $v1
    j loc_00002830
    sll $s3, $s3, 7

loc_00002DF4:
    mflo $s3
    addiu $s2, $s2, 31
    blez $t9, loc_0000284C
    srl $v1, $s3, 24
    bnez $v1, loc_000027F8
    srl $v0, $s3, 8

loc_00002E0C:
    lbu $v1, 5($a2)
    sll $t1, $t1, 8
    addiu $a2, $a2, 1
    addu $t1, $t1, $v1
    sll $s3, $s3, 8
    j loc_000027F8
    srl $v0, $s3, 8

loc_00002E28:
    subu $s3, $s3, $v1
    subu $t1, $t1, $v1
    bltz $t8, loc_00002938
    sb $t7, 2033($t9)
    bnezl $t8, loc_00002980
    addiu $t8, $t8, -8
    bne $t5, $t3, loc_00002AF4
    move $t7, $zr

loc_00002E48:
    lui $v0, 0x8000
    j loc_00002D00
    ori $v0, $v0, 0x108

loc_00002E54:
    lui $v0, 0x8000
    j loc_00002D00
    ori $v0, $v0, 0x104

loc_00002E60:
    addu $t1, $t1, $a0
    sltu $v1, $t1, $t4
    beqz $v1, loc_00002E54
    nop

loc_00002E70:
    lbu $v1, 5($a2)
    addiu $t3, $t3, 1
    beq $t3, $t1, loc_00002CF0
    sb $v1, 0($t3)
    j loc_00002E70
    addiu $a2, $a2, 1

