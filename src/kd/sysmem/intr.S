#include "common_asm.h"

    .globl suspendIntr
suspendIntr:
    mfic $v0, $0
    mtic $zr, $0
    beqz $v0, end
    nop
    nop

wait:
    nop
    nop
    nop
    nop

end:
    jr $ra
    nop
    sync
    nop

    .globl resumeIntr
resumeIntr:
    mtic $a0, $0
    j wait
    nop


