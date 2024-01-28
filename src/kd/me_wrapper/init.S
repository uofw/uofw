# Utopia ME Wrapper Module 6.60 clone
# Copyright (c) 2011 by mowglisanu <mowglisanu@gmain.com>
# Copyright (C) 2011 The uOFW team
#  See the file COPYING for copying permission.

#include <machine/regdef.h>

#define Status    $12
#define Cause    $13
#define Config    $16
#define TagLo    $28
#define TagHi    $29

.global me_boot_code_end
.global me_boot_code
.global decompressAndRunMeImage
.set noreorder
.ent me_boot_code
me_boot_code:
lui        v0, 0xBC10
lw         v0, 0(v0)
beqz       v0, reset_start
nop
cfc0       v0, $9
jr         v0
nop
nop
reset_start:
li         t1, 0                     #;rewritten so t1 is the same as first 2 bits of 0xBC100040 on sc
lui        a0, 0x0                   #;
ori        a0, a0, 0x0               #;rewritten so a0 is address of compressed me image
lui        a3, 0xBC10                #;
sw         t1, 64(a3)                #;*0xBC100040 = [0,1,2,3]
li         t1, 7                     #;
sw         t1, 80(a3)                #;*0xBC100050 = 7
li         t1, -1                    #;
sw         t1, 4(a3)                 #;*0xBC100004 = 0
sync                                 #;
mfc0       k0, Config                #;k0 = Config
mtc0       zero, TagLo               #;TagLo = 0
mtc0       zero, TagHi               #;TagHi = 0
ext        t0, k0, 9, 3              #;t0 = k0.bits[9..11]
li         t9, 2048                  #;t9 = 2048
sllv       k1, t9, t0                #;k1 = 2048 << [0..7]
cacheLoop1:                          #;
addi       k1, k1, -64               #;k1 -= 64
bnez       k1, cacheLoop1            #;if (k1 != 0) goto 0x881F8460
cache      0x1, 0(k1)                #;cache
                                     #;
ext        t0, k0, 6, 3              #;t0 = k0.bits[6..8]
sllv       k1, t9, t0                #;k0 = 2048 << [0..7]
cacheLoop2:                          #;
addi       k1, k1, -64               #;k1 -= 64
bnez       k1, cacheLoop2            #;if (k1 != 0) 0x881F8474
cache      0x11, 0(k1)               #;cache
                                     #;
mtc0       zero, Cause               #;Cause = 0
lui        k0, 0x2000                #;
mtc0       k0, Status                #;Status = 0x20000000
lui        k0, 0xBCC0                #;
li         t1, 1                     #;
sw         t1, 16(k0)                #;*0xBCC00010 = 1
hwLoop:                              #;
lw         t0, 16(k0)                #;t0 = *0xBCC00010
andi       t0, t0, 0x1               #;t0 = *0xBCC00010 and 1
beq        t0, t1, hwLoop            #;if ([0,1] == 1) goto 0x881F8498
nop                                  #;
                                     #;
li         t1, 1                     #;t1 = 1
sw         t1, 112(k0)               #;*0xBCC00070 = 1
li         t1, 8                     #;
sw         t1, 48(k0)                #;*0xBCC00030 = 8
li         t1, 2                     #;
sw         t1, 64(k0)                #;*0xBCC00040 = 2
la         ra, decompressAndRunMeImage
lui        sp, 0x8840
jr         ra
sync
me_boot_code_end:
.end me_boot_code