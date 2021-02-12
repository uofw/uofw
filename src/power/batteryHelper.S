# Copyright (C) The uOFW team
# See the file COPYING for copying permission.

#include <common_asm.h>

# In the original code, this code is inlined. However, I find it simpler
# to just put it inside its own .S file. Yes, we now have a JAL call which
# isn't originally in place, but I find this code better to maintain when it's
# in its own file.
    .globl sub_4918
sub_4918:
    cfc0       $t0, $30
    bnez       $t0, loc_00004938
    lui        $t6, 0x0
    lw         $v1, 4($t6)
    beqz       $v1, loc_00004938
    li         $t7, 1
    sw         $t7, 4($v1)
    sw         $zr, 8($v1)
loc_00004938:
    cfc0       $t1, $31
    bnez       $t1, loc_end
#    lw         $v1, 4($sp) # This already belongs to the continue statement to which we do not jump here
    lui        $t8, 0x0
    lw         $v1, 0($t8)
#    beqzl      $v1, loc_end
#    lw         $v1, 4($sp) # This already belongs to the continue statement to which we do not jump here
    beqz       $v1, loc_end
    li         $t9, 1
    sw         $t9, 4($v1)
#    j          loc_0000474 # In the original code, this is the "continue;" statement.
    sw         $zr, 8($v1)
loc_end: # inserted by me
    jr         $ra

