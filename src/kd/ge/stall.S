# Copyright (C) 2011, 2012 The uOFW team
# See the file COPYING for copying permission.

    .text
    .set noat
    .set noreorder

    .globl sceGeListUpdateStallAddr
sceGeListUpdateStallAddr:
    lui   $v0, %hi(g_dlMask)
    lw    $v0, %lo(g_dlMask)($v0)
    lui   $v1, %hi(g_displayLists)
    addiu $v1, $v1, %lo(g_displayLists)
    xor   $v0, $v0, $a0 # pointer to the SceGeDisplayList
    subu  $v1, $v0, $v1
    sltiu $v1, $v1, 4096
    beqz  $v1, err_invalid_id # the display list isn't in g_displayLists
    ext   $t0, $a1, 0, 29 # uncached stall address
    mfic  $v1, $0 # disable interrupts
    mtic  $0,  $0
    lbu   $t1, 8($v0) # dl->state
    lui   $t3, 0xBD40
    lui   $t9, %hi(g_GeLogHandler)
    xori  $t2, $t1, 0x2
    bnez  $t2, not_running # dl->state != SCE_GE_DL_STATE_RUNNING
    lw    $t9, %lo(g_GeLogHandler)($t9) # logging function

    # dl->state == SCE_GE_DL_STATE_RUNNING
    # the display list is running: store stall address at 0xBD40010C (HW_GE_STALLADDR) and change dl->stall directly
    sw    $t0, 268($t3) # store stall address at 0xBD40010C
    sync
    bnez  $t9, logging
    sw    $a1, 24($v0) # dl->stall
    # reenable interrupts
    mtic  $v1, $0
    jr    $ra
    move  $v0, $0

not_running:
    xori  $t2, $t1, 0x1
    xori  $t3, $t1, 0x4
    movz  $t2, $0, $t3
    # dl->state != SCE_GE_DL_STATE_PAUSED && dl->state != SCE_GE_DL_STATE_QUEUED
    bnez  $t2, not_waiting
    nop

    # dl->state == SCE_GE_DL_STATE_PAUSED or dl->state == SCE_GE_DL_STATE_QUEUED:
    # update dl->stall but not the hardware register as another display list is running
    bnez  $t9, logging
    sw    $a1, 24($v0) # dl->stall
    # restore interrupts
    mtic  $v1, $0
    jr    $ra
    move  $v0, $0

logging:
    # basically run g_GeLogHandler(2, dlId, addr)
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
    # restore interrupts
    mtic  $v1, $0
    jr    $ra
    addiu $sp, $sp, 16

not_waiting:
    mtic   $v1, $0                                                                                      
    xori   $t2, $t1, 0x3
    bnez   $t2, err_invalid_id
    nop
    # dl->list == SCE_GE_DL_STATE_COMPLETED: can't update the stall address of a completed display list
    lui    $v0, 0x8000
    ori    $v0, $v0, 0x20
    jr     $ra
    nop

err_invalid_id:
    # invalid dl->state value!
    lui    $v0, 0x8000
    ori    $v0, $v0, 0x100
    jr     $ra
    nop

