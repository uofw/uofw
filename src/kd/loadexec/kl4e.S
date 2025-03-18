#include "common_asm.h"

    .globl decodeKL4E
decodeKL4E:
    addiu      $sp, $sp, -2680
    sw         $s2, 2664($sp)
    addu       $t4, $a0, $a1
    addiu      $t3, $a0, 0
    sw         $s1, 2660($sp)
    li         $t2, 6
    addiu      $a1, $sp, -2656
    sw         $s0, 2656($sp)
    move       $t9, $zr
    li         $t0, 1
    sw         $s3, 2668($sp)
    li         $s3, -1
    sw         $s4, 2672($sp)
    addiu      $s4, $t4, -64
    ins        $s4, $zr, 0, 6
    lb         $s0, 0($a2)
    lwl        $t1, 4($a2)
    lwr        $t1, 1($a2)
    bltz       $s0, loc_00000744
    wsbw       $t1, $t1
    ext        $at, $s0, 3, 2
    sll        $at, $at, 4
    li         $s1, 128
    subu       $at, $s1, $at
    ins        $at, $at, 8, 8
    andi       $s0, $s0, 0x7
    ins        $at, $at, 16, 16

loc_0000006C:		
    sw         $at, 2656($a1)
    addiu      $a1, $a1, 8
    bne        $a1, $sp, loc_0000006C
    sw         $at, 2652($a1)
    li         $at, 255

loc_00000080:		
    ins        $t9, $t3, 8, 3
    srav       $v1, $t9, $s0
    andi       $v1, $v1, 0x7
    sll        $t9, $v1, 8
    subu       $t8, $t9, $v1
    addu       $v1, $sp, $t8
    li         $t9, 1

loc_0000009C:		
    addu       $t7, $v1, $t9
    srl        $t5, $s3, 24
    beqz       $t5, loc_000000EC
    lbu        $t6, -1($t7)
    sll        $t9, $t9, 1
    srl        $s2, $s3, 8
    mult       $s2, $t6
    srl        $s2, $t6, 3
    subu       $t6, $t6, $s2
    mflo       $t5
    sltu       $t8, $t1, $t5
    beqz       $t8, loc_00000120
    srl        $s2, $t9, 8

loc_000000D0:		
    mflo       $s3
    addiu      $t6, $t6, 31
    sb         $t6, -1($t7)
    beqz       $s2, loc_0000009C
    addiu      $t9, $t9, 1
    j          loc_00000134
    sb         $t9, 0($t3)

loc_000000EC:		
    lbu        $s2, 5($a2)
    sll        $t1, $t1, 8
    addiu      $a2, $a2, 1
    addu       $t1, $t1, $s2
    sll        $t9, $t9, 1
    mult       $s3, $t6
    sll        $s3, $s3, 8
    srl        $s2, $t6, 3
    subu       $t6, $t6, $s2
    mflo       $t5
    sltu       $t8, $t1, $t5
    bnez       $t8, loc_000000D0
    srl        $s2, $t9, 8

loc_00000120:		
    sb         $t6, -1($t7)
    subu       $t1, $t1, $t5
    beqz       $s2, loc_0000009C
    subu       $s3, $s3, $t5
    sb         $t9, 0($t3)

loc_00000134:		
    srl        $s1, $s3, 24
    lbu        $t5, 2336($a1)
    beqz       $s1, loc_0000017C
    srl        $v1, $s3, 8
    mult       $v1, $t5
    addiu      $t3, $t3, 1
    srl        $v1, $t5, 4
    mflo       $s1
    sltu       $t6, $t1, $s1
    bnez       $t6, loc_000001AC
    subu       $t5, $t5, $v1

loc_00000160:		
    beq        $t3, $t4, loc_000008E0
    sb         $t5, 2336($a1)
    addiu      $a1, $a1, -1
    max        $a1, $a1, $sp
    subu       $t1, $t1, $s1
    j          loc_00000080
    subu       $s3, $s3, $s1

loc_0000017C:		
    lbu        $t8, 5($a2)
    addiu      $t3, $t3, 1
    sll        $t1, $t1, 8
    addiu      $a2, $a2, 1
    addu       $t1, $t1, $t8
    mult       $s3, $t5
    sll        $s3, $s3, 8
    srl        $v1, $t5, 4
    mflo       $s1
    sltu       $t6, $t1, $s1
    beqz       $t6, loc_00000160
    subu       $t5, $t5, $v1

loc_000001AC:		
    addiu      $s3, $t5, 15
    sb         $s3, 2336($a1)
    li         $t8, -1
    mflo       $s3

loc_000001BC:		
    srl        $t9, $s1, 24
    beqz       $t9, loc_00000750
    lbu        $t5, 2344($a1)
    addiu      $a1, $a1, 8
    srl        $s2, $s3, 8
    mult       $s2, $t5
    srl        $s2, $t5, 4
    subu       $t5, $t5, $s2
    mflo       $s1
    sltu       $t9, $t1, $s1
    bnez       $t9, loc_00000784
    addiu      $v1, $t5, 15

loc_000001EC:		
    subu       $s3, $s3, $s1
    subu       $t1, $t1, $s1
    sb         $t5, 2336($a1)
    bltz       $t8, loc_000003A0
    addu       $t5, $sp, $t8

loc_00000200:		
    addiu      $t9, $t8, -3
    sllv       $t6, $t3, $t8
    sll        $s1, $t8, 5
    ins        $s1, $t6, 3, 2
    ins        $s1, $a1, 0, 3
    addu       $s1, $sp, $s1
    bltz       $t9, loc_000002BC
    li         $t6, 1
    lbu        $s2, 2424($s1)
    srl        $v1, $s3, 24
    beqz       $v1, loc_00000858
    srl        $v0, $s3, 8
    mult       $v0, $s2

loc_00000234:		
    srl        $v0, $s2, 3
    subu       $s2, $s2, $v0
    mflo       $v1
    sltu       $v0, $t1, $v1
    bnez       $v0, loc_000008A8
    li         $t6, 2
    subu       $t1, $t1, $v1
    blez       $t9, loc_000002B8
    subu       $s3, $s3, $v1
    srl        $v1, $s3, 24
    beqz       $v1, loc_000008C4
    srl        $v0, $s3, 8

loc_00000264:		
    mult       $v0, $s2
    srl        $v0, $s2, 3
    subu       $s2, $s2, $v0
    mflo       $v1
    sltu       $v0, $t1, $v1
    bnez       $v0, loc_00000874
    sll        $t6, $t6, 1
    subu       $t1, $t1, $v1
    beq        $t9, $t0, loc_000002B8
    subu       $s3, $s3, $v1
    srl        $v1, $s3, 24
    beqz       $v1, loc_00000890
    sll        $t6, $t6, 1

loc_00000298:		
    srl        $s3, $s3, 1

loc_0000029C:		
    sltu       $v0, $t1, $s3
    subu       $v1, $t1, $s3
    addiu      $t9, $t9, -1
    movz       $t1, $v1, $v0
    addu       $t6, $t6, $v0
    bnel       $t9, $t0, loc_00000298
    sll        $t6, $t6, 1

loc_000002B8:		
    sb         $s2, 2424($s1)

loc_000002BC:		
    srl        $v1, $s3, 24
    addiu      $a1, $sp, 7
    lbu        $s2, 2400($s1)
    beqz       $v1, loc_0000080C
    srl        $v0, $s3, 8
    mult       $v0, $s2
    srl        $v0, $s2, 3
    subu       $s2, $s2, $v0
    mflo       $v1
    sltu       $v0, $t1, $v1
    bnez       $v0, loc_0000083C
    sll        $t6, $t6, 1

loc_000002EC:		
    sb         $s2, 2400($s1)
    subu       $t1, $t1, $v1
    blez       $t8, loc_000003A4
    subu       $s3, $s3, $v1
    lbu        $s2, 2408($s1)

loc_00000300:		
    srl        $v1, $s3, 24
    beqz       $v1, loc_000007F0
    srl        $v0, $s3, 8
    mult       $v0, $s2

loc_00000310:		
    srl        $v0, $s2, 3
    subu       $s2, $s2, $v0
    mflo       $v1
    sltu       $v0, $t1, $v1
    bnez       $v0, loc_000007D4
    sll        $t6, $t6, 1
    sb         $s2, 2408($s1)
    subu       $t1, $t1, $v1
    beq        $t8, $t0, loc_00000370
    subu       $s3, $s3, $v1
    lbu        $s2, 2416($s1)

loc_0000033C:		
    srl        $v1, $s3, 24
    beqz       $v1, loc_0000079C
    srl        $v0, $s3, 8
    mult       $v0, $s2

loc_0000034C:		
    srl        $v0, $s2, 3
    subu       $s2, $s2, $v0
    mflo       $v1
    sltu       $v0, $t1, $v1
    bnez       $v0, loc_000007B8
    sll        $t6, $t6, 1
    sb         $s2, 2416($s1)
    subu       $t1, $t1, $v1
    subu       $s3, $s3, $v1

loc_00000370:		
    addiu      $t5, $t5, 56
    li         $s2, 128
    li         $s1, 8
    srl        $v1, $s3, 24
    bnez       $v1, loc_000003B8
    addu       $t9, $t5, $s1

loc_00000388:		
    lbu        $v1, 5($a2)
    sll        $t1, $t1, 8
    addiu      $a2, $a2, 1
    addu       $t1, $t1, $v1
    j          loc_000003B8
    sll        $s3, $s3, 8

loc_000003A0:		
    addiu      $a1, $sp, 7

loc_000003A4:		
    li         $s2, 64
    li         $s1, 8

loc_000003AC:		
    srl        $v1, $s3, 24
    beqz       $v1, loc_00000388
    addu       $t9, $t5, $s1

loc_000003B8:		
    lbu        $t8, 2033($t9)
    srl        $v0, $s3, 8
    sll        $s1, $s1, 1
    mult       $v0, $t8
    srl        $v0, $t8, 3
    subu       $t7, $t8, $v0
    mflo       $v1
    sltu       $t8, $t1, $v1
    beqz       $t8, loc_0000071C
    subu       $t8, $s1, $s2
    mflo       $s3
    addiu      $t7, $t7, 31
    sb         $t7, 2033($t9)
    bltz       $t8, loc_000003AC
    addiu      $s1, $s1, 8

loc_000003F4:		
    addu       $s1, $sp, $t8
    sra        $t8, $t8, 3
    lw         $t5, 2216($s1)
    addiu      $t9, $t8, -3
    bltz       $t9, loc_000004A8
    li         $t7, 1
    srl        $v0, $s3, 24
    beqz       $v0, loc_00000694
    ext        $s2, $t5, 24, 8
    srl        $v0, $s3, 8
    mult       $v0, $s2

loc_00000420:		
    srl        $v0, $s2, 3
    subu       $s2, $s2, $v0
    mflo       $v1
    sltu       $v0, $t1, $v1
    bnez       $v0, loc_000006E4
    li         $t7, 2
    subu       $t1, $t1, $v1
    blez       $t9, loc_000004A4
    subu       $s3, $s3, $v1
    srl        $v0, $s3, 24
    beqz       $v0, loc_00000700
    srl        $v0, $s3, 8

loc_00000450:		
    mult       $v0, $s2
    srl        $v0, $s2, 3
    subu       $s2, $s2, $v0
    mflo       $v1
    sltu       $v0, $t1, $v1
    bnez       $v0, loc_000006B0
    sll        $t7, $t7, 1
    subu       $t1, $t1, $v1
    beq        $t9, $t0, loc_000004A4
    subu       $s3, $s3, $v1
    srl        $v0, $s3, 24
    beqz       $v0, loc_000006CC
    sll        $t7, $t7, 1

loc_00000484:		
    srl        $s3, $s3, 1

loc_00000488:		
    sltu       $v0, $t1, $s3
    subu       $v1, $t1, $s3
    addiu      $t9, $t9, -1
    movz       $t1, $v1, $v0
    addu       $t7, $t7, $v0
    bnel       $t9, $t0, loc_00000484
    sll        $t7, $t7, 1

loc_000004A4:		
    ins        $t5, $s2, 24, 8

loc_000004A8:		
    srl        $v0, $s3, 24
    beqz       $v0, loc_00000678
    ext        $s2, $t5, 0, 8
    srl        $v0, $s3, 8
    mult       $v0, $s2

loc_000004BC:		
    srl        $v0, $s2, 3
    subu       $s2, $s2, $v0
    mflo       $v1
    sltu       $v0, $t1, $v1
    bnez       $v0, loc_00000644
    sll        $t7, $t7, 1
    ins        $t5, $s2, 0, 8
    subu       $t1, $t1, $v1
    blez       $t8, loc_00000558
    subu       $s3, $s3, $v1

loc_000004E4:		
    srl        $v0, $s3, 24
    beqz       $v0, loc_0000065C
    ext        $s2, $t5, 8, 8
    srl        $v0, $s3, 8
    mult       $v0, $s2

loc_000004F8:		
    srl        $v0, $s2, 3
    subu       $s2, $s2, $v0
    mflo       $v1
    sltu       $v0, $t1, $v1
    bnez       $v0, loc_0000062C
    sll        $t7, $t7, 1
    ins        $t5, $s2, 8, 8
    subu       $t1, $t1, $v1
    beq        $t8, $t0, loc_00000558
    subu       $s3, $s3, $v1

loc_00000520:		
    srl        $v0, $s3, 24
    beqz       $v0, loc_00000600
    ext        $s2, $t5, 16, 8
    srl        $v0, $s3, 8
    mult       $v0, $s2

loc_00000534:		
    srl        $v0, $s2, 3
    subu       $s2, $s2, $v0
    mflo       $v1
    sltu       $v0, $t1, $v1
    bnez       $v0, loc_0000061C
    sll        $t7, $t7, 1
    ins        $t5, $s2, 16, 8
    subu       $t1, $t1, $v1
    subu       $s3, $s3, $v1

loc_00000558:		
    addiu      $t7, $t7, -1

loc_0000055C:		
    sw         $t5, 2216($s1)
    subu       $t5, $t3, $a0
    sltu       $t9, $t7, $t5
    beqz       $t9, loc_00000744
    addu       $t6, $t3, $t6
    subu       $v1, $t3, $t7

loc_00000574:		
    sltu       $t9, $t6, $t4
    beqz       $t9, loc_000008E0
    lbu        $t9, -1($v1)
    ins        $a1, $t6, 0, 1
    sltu       $s1, $t4, $s4
    bnezl      $s1, loc_000005AC
    cache      0x18, 64($t3)

loc_00000590:		
    addiu      $t3, $t3, 1
    sb         $t9, -1($t3)
    addiu      $v1, $v1, 1
    bne        $t3, $t6, loc_00000590
    lbu        $t9, -1($v1)
    j          loc_00000134
    sb         $t9, 0($t3)

loc_000005AC:		
    subu       $s1, $t6, $t3
    slti       $s1, $s1, 4
    bnez       $s1, loc_00000590
    subu       $s1, $t3, $v1
    slti       $s1, $s1, 3
    bnez       $s1, loc_00000590
    addiu      $v0, $t3, 0
    move       $t3, $t6
    ins        $t6, $v0, 0, 2

loc_000005D0:		
    lwl        $t9, 2($v1)
    lwr        $t9, -1($v1)
    swl        $t9, 3($v0)
    swr        $t9, 0($v0)
    beq        $t6, $v0, loc_000005F0
    addiu      $v1, $v1, 4
    j          loc_000005D0
    addiu      $v0, $v0, 4

loc_000005F0:		
    bne        $t3, $t4, loc_00000134
    lbu        $t9, 0($t3)
    j          loc_000008E8
    subu       $v0, $t3, $a0

loc_00000600:		
    lbu        $v1, 5($a2)
    sll        $t1, $t1, 8
    addiu      $a2, $a2, 1
    addu       $t1, $t1, $v1
    mult       $s3, $s2
    j          loc_00000534
    sll        $s3, $s3, 8

loc_0000061C:		
    addiu      $s2, $s2, 31
    ins        $t5, $s2, 16, 8
    j          loc_0000055C
    mflo       $s3

loc_0000062C:		
    addiu      $s2, $s2, 31
    ins        $t5, $s2, 8, 8
    beq        $t8, $t0, loc_0000055C
    mflo       $s3
    j          loc_00000520
    addiu      $t7, $t7, 1

loc_00000644:		
    addiu      $s2, $s2, 31
    ins        $t5, $s2, 0, 8
    blez       $t8, loc_0000055C
    mflo       $s3
    j          loc_000004E4
    addiu      $t7, $t7, 1

loc_0000065C:		
    lbu        $v1, 5($a2)
    sll        $t1, $t1, 8
    addiu      $a2, $a2, 1
    addu       $t1, $t1, $v1
    mult       $s3, $s2
    j          loc_000004F8
    sll        $s3, $s3, 8

loc_00000678:		
    lbu        $v1, 5($a2)
    sll        $t1, $t1, 8
    addiu      $a2, $a2, 1
    addu       $t1, $t1, $v1
    mult       $s3, $s2
    j          loc_000004BC
    sll        $s3, $s3, 8

loc_00000694:		
    lbu        $v1, 5($a2)
    sll        $t1, $t1, 8
    addiu      $a2, $a2, 1
    addu       $t1, $t1, $v1
    mult       $s3, $s2
    j          loc_00000420
    sll        $s3, $s3, 8

loc_000006B0:		
    mflo       $s3
    addiu      $t7, $t7, 1
    beq        $t9, $t0, loc_000004A4
    addiu      $s2, $s2, 31
    srl        $v0, $s3, 24
    bnez       $v0, loc_00000484
    sll        $t7, $t7, 1

loc_000006CC:		
    lbu        $v1, 5($a2)
    sll        $t1, $t1, 8
    addiu      $a2, $a2, 1
    addu       $t1, $t1, $v1
    j          loc_00000488
    sll        $s3, $s3, 7

loc_000006E4:		
    mflo       $s3
    li         $t7, 3
    blez       $t9, loc_000004A4
    addiu      $s2, $s2, 31
    srl        $v0, $s3, 24
    bnez       $v0, loc_00000450
    srl        $v0, $s3, 8

loc_00000700:		
    lbu        $v1, 5($a2)
    sll        $t1, $t1, 8
    addiu      $a2, $a2, 1
    addu       $t1, $t1, $v1
    sll        $s3, $s3, 8
    j          loc_00000450
    srl        $v0, $s3, 8

loc_0000071C:		
    subu       $s3, $s3, $v1
    subu       $t1, $t1, $v1
    bltz       $t8, loc_000003AC
    sb         $t7, 2033($t9)
    bnez       $t8, loc_000003F4
    addiu      $t8, $t8, -8
    move       $v1, $t3
    subu       $t5, $t3, $a0
    bnez       $t5, loc_00000574
    addu       $t6, $t3, $t6

loc_00000744:		
    lui        $v0, 0x8000
    j          loc_000008E8
    ori        $v0, $v0, 0x108

loc_00000750:		
    lbu        $v1, 5($a2)
    sll        $t1, $t1, 8
    addiu      $a2, $a2, 1
    addu       $t1, $t1, $v1
    addiu      $a1, $a1, 8
    mult       $s3, $t5
    sll        $s3, $s1, 8
    srl        $s2, $t5, 4
    mflo       $s1
    sltu       $t9, $t1, $s1
    beqz       $t9, loc_000001EC
    subu       $t5, $t5, $s2
    addiu      $v1, $t5, 15

loc_00000784:		
    addiu      $t8, $t8, 1
    mflo       $s3
    bne        $t8, $t2, loc_000001BC
    sb         $v1, 2336($a1)
    j          loc_00000200
    addu       $t5, $sp, $t8

loc_0000079C:		
    lbu        $v1, 5($a2)
    sll        $t1, $t1, 8
    addiu      $a2, $a2, 1
    addu       $t1, $t1, $v1
    mult       $s3, $s2
    j          loc_0000034C
    sll        $s3, $s3, 8

loc_000007B8:		
    addiu      $s2, $s2, 31
    sb         $s2, 2416($s1)
    addiu      $t6, $t6, 1
    bne        $t6, $at, loc_00000370
    mflo       $s3
    j          loc_000008E8
    subu       $v0, $t3, $a0

loc_000007D4:		
    addiu      $s2, $s2, 31
    sb         $s2, 2408($s1)
    addiu      $t6, $t6, 1
    beq        $t8, $t0, loc_00000370
    mflo       $s3
    j          loc_0000033C
    lbu        $s2, 2416($s1)

loc_000007F0:		
    lbu        $v1, 5($a2)
    sll        $t1, $t1, 8
    addiu      $a2, $a2, 1
    addu       $t1, $t1, $v1
    mult       $s3, $s2
    j          loc_00000310
    sll        $s3, $s3, 8

loc_0000080C:		
    lbu        $v1, 5($a2)
    sll        $t1, $t1, 8
    addiu      $a2, $a2, 1
    addu       $t1, $t1, $v1
    mult       $s3, $s2
    sll        $s3, $s3, 8
    srl        $v0, $s2, 3
    subu       $s2, $s2, $v0
    mflo       $v1
    sltu       $v0, $t1, $v1
    beqz       $v0, loc_000002EC
    sll        $t6, $t6, 1

loc_0000083C:		
    addiu      $s2, $s2, 31
    sb         $s2, 2400($s1)
    mflo       $s3
    blez       $t8, loc_00000370
    addiu      $t6, $t6, 1
    j          loc_00000300
    lbu        $s2, 2408($s1)

loc_00000858:		
    lbu        $v1, 5($a2)
    sll        $t1, $t1, 8
    addiu      $a2, $a2, 1
    addu       $t1, $t1, $v1
    mult       $s3, $s2
    j          loc_00000234
    sll        $s3, $s3, 8

loc_00000874:		
    mflo       $s3
    addiu      $t6, $t6, 1
    beq        $t9, $t0, loc_000002B8
    addiu      $s2, $s2, 31
    srl        $v1, $s3, 24
    bnez       $v1, loc_00000298
    sll        $t6, $t6, 1

loc_00000890:		
    lbu        $v1, 5($a2)
    sll        $t1, $t1, 8
    addiu      $a2, $a2, 1
    addu       $t1, $t1, $v1
    j          loc_0000029C
    sll        $s3, $s3, 7

loc_000008A8:		
    mflo       $s3
    li         $t6, 3
    blez       $t9, loc_000002B8
    addiu      $s2, $s2, 31
    srl        $v1, $s3, 24
    bnez       $v1, loc_00000264
    srl        $v0, $s3, 8

loc_000008C4:		
    lbu        $v1, 5($a2)
    sll        $t1, $t1, 8
    addiu      $a2, $a2, 1
    addu       $t1, $t1, $v1
    sll        $s3, $s3, 8
    j          loc_00000264
    srl        $v0, $s3, 8

loc_000008E0:		
    lui        $v0, 0x8000
    ori        $v0, $v0, 0x104

loc_000008E8:		
    beqz       $a3, loc_000008F4
    addiu      $a2, $a2, 5
    sw         $a2, 0($a3)

loc_000008F4:		
    lw         $s4, 2672($sp)
    lw         $s3, 2668($sp)
    lw         $s2, 2664($sp)
    lw         $s1, 2660($sp)
    lw         $s0, 2656($sp)
    jr         $ra
    addiu      $sp, $sp, 2680
