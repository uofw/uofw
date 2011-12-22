    .text
    .set noat
    .set noreorder

    .globl sceGeListUpdateStallAddr
sceGeListUpdateStallAddr:
    lui   $v0, %hi(g_7604)
    lw    $v0, %lo(g_7604)($v0)
    lui   $v1, %hi(g_7640) # g_displayLists
    addiu $v1, $v1, %lo(g_7640)
    xor   $v0, $v0, $a0 # the SceGeDisplayList
    subu  $v1, $v0, $v1
    sltiu $v1, $v1, 4096
    beqz  $v1, err_invalid_id # the display list isn't in g_displayLists
    ext   $t0, $a1, 0, 29 # uncached stall address
    mfic  $v1, $0
    mtic  $0,  $0
    lbu   $t1, 8($v0) # dl->unk8
    lui   $t3, 0xBD40
    lui   $t9, %hi(g_6890)
    xori  $t2, $t1, 0x2
    bnez  $t2, mode_is_not_2 # dl->unk8 != 2
    lw    $t9, %lo(g_6890)($t9) # logging function

    ## when dl->unk8 == 2: store stall address at 0xBD40010C and change dl->stall
    sw    $t0, 268($t3) # store stall address at 0xBD40010C
    sync
    bnez  $t9, logging
    sw    $a1, 24($v0) # dl->stall
    mtic  $v1, $0
    jr    $ra
    move  $v0, $0

mode_is_not_2:
    xori  $t2, $t1, 0x1
    xori  $t3, $t1, 0x4
    movz  $t2, $0, $t3
    bnez  $t2, mode_is_not_1_2_4 # dl->unk8 != 1 && dl->unk8 != 4
    nop

    ## when dl->unk8 == 1 or dl->unk8 == 4: just change dl->stall
    bnez  $t9, logging
    sw    $a1, 24($v0) # dl->stall
    mtic  $v1, $0
    jr    $ra
    move  $v0, $0

logging:
    addiu $sp, $sp, -16
    sw    $ra, 0($sp)
    move  $a2, $a1
    move  $a1, $a0
    sw    $v1, 4($sp)
    li    $a0, 2
    sw    $k1, 8($sp)
    jalr  $t9
    srl   $k1, $k1, 16
    lw    $ra, 0($sp)
    lw    $v1, 4($sp)
    lw    $k1, 8($sp)
    move  $v0, $0
    mtic  $v1, $0
    jr    $ra
    addiu $sp, $sp, 16

mode_is_not_1_2_4:
    mtic   $v1, $0                                                                                      
    xori   $t2, $t1, 0x3
    bnez   $t2, err_invalid_id # dl->unk8 != 3
    nop
    lui    $v0, 0x8000
    ori    $v0, $v0, 0x20
    jr     $ra
    nop

err_invalid_id:
    lui    $v0, 0x8000
    ori    $v0, $v0, 0x100
    jr     $ra
    nop
    nop
    nop

