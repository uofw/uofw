#include "common_asm.h"

    .globl suspendIntr
suspendIntr:
    mfic $v0, $0
    mtic $zr, $0
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

