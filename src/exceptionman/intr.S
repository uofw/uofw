# Copyright (C) 2011, 2012 The uOFW team
# See the file COPYING for copying permission.

    .text
    .set noat
    .set noreorder

    .globl suspendIntr
suspendIntr:
    mfic $v0, $0
    mtic  $0, $0
    beqz $v0, ret
    nop
    nop

retWithNop:
    nop
    nop
    nop
    nop

ret:
    jr $ra
    nop
    sync
    nop

    .globl resumeIntr
resumeIntr:
    mtic $a0, $0
    j    retWithNop
    nop

