#include "common_asm.h"

# Code running at 0xBFC00000
# This is the beginning of the Pre-IPL, which is stored in a ROM and mapped at 0xBFC00000 on reset.
# This first stage, called the "loader", will just handle exceptions if they are enabled (ie it's not on boot),
# and otherwise load the second stage of the Pre-IPL, the "payload", in the scratchpad, in order to run it
_start:
    # save $v0 in cop0
    ctc0       $v0, COP0_CTRL_V0
    # check NMI interrupt mask
    lui        $v0, %hi(HW_SYS_NMI_ENABLE_MASK)
    lw         $v0, %lo(HW_SYS_NMI_ENABLE_MASK)($v0)
    # in case interrupts are enabled, it means we're not at boot time but recovering from an interrupt
    bnez       $v0, handle_nmi
    nop
    # in case NMI are not enabled, unload the handler
    ctc0       $zr, COP0_CTRL_NMI_HANDLER
    jal        sceKernelL1IcacheInvalidateAll
    nop
    jal        sceKernelL1DcacheInvalidateAll
    nop
    # copy preipl payload to 0xA0010000 (scratchpad with kernel & uncached flags)
    lui        $a0, 0xA001
    lui        $a1, %hi(preipl_payload)
    addiu      $a1, $a1, %lo(preipl_payload)
    li         $a2, (preipl_payload_end - preipl_payload)

copy_preipl:
    lw         $t0, 0($a1)
    addiu      $a1, $a1, 4
    sw         $t0, 0($a0)
    addiu      $a2, $a2, -4
    bgtz       $a2, copy_preipl
    addiu      $a0, $a0, 4

    # jump to the copied payload
    lui        $sp, 0x8001
    ori        $sp, $sp, 0x3FF0
    lui        $t9, 0x8001
    jr         $t9
    sync

handle_nmi:
    cfc0       $v0, COP0_CTRL_NMI_HANDLER
    # in case no NMI handler is defined, use the default exception handler
    beqz       $v0, handle_exception
    nop
    jr         $v0
    nop

handle_exception:
    # restore $v0
    cfc0       $v0, COP0_CTRL_V0
    # set BEV = 0 (ie exception vectors switches from 0xBFC00200 to 0x
    mfc0       $k1, COP0_STATE_STATUS
    lui        $k0, 0xFFBF
    ori        $k0, $k0, 0xFFFF
    and        $k1, $k1, $k0
    mtc0       $k1, COP0_STATE_STATUS
    # get the address of the exception vector and jump there
    mfc0       $k0, COP0_STATE_EBASE
    jr         $k0
    nop

# TODO add enough nop's to get to 0xbfc00200
reset_vector_2: # exception vector for some exceptions (everything except reset, NMI & soft reset)
    b _start

# ======================================================
sceKernelL1IcacheInvalidateAll: # at 0xBFC00208
    # get IC, where primary I-cache size is 2^(12 + IC)
    mfc0       $t0, COP0_STATE_CONFIG
    li         $t1, 4096
    ext        $t0, $t0, 9, 3
    # t1 is the primary I-cache size (in bytes)
    sllv       $t1, $t1, $t0
    mtc0       $zr, COP0_STATE_TAG_LO
    mtc0       $zr, COP0_STATE_TAG_HI
    move       $t0, $zr

icache_loop:
    cache      0x1, 0($t0)
    cache      0x3, 0($t0)
    addiu      $t0, $t0, 64
    bne        $t0, $t1, loc_BFC00224
    nop        
    jr         $ra
    nop        

# ======================================================
sceKernelL1DcacheInvalidateAll: # at 0xBFC00240
    mfc0       $t0, COP0_STATE_CONFIG
    li         $t1, 4096
    ext        $t0, $t0, 6, 3
    sllv       $t1, $t1, $t0
    mtc0       $zr, COP0_STATE_TAG_LO
    mtc0       $zr, COP0_STATE_TAG_HI
    move       $t0, $zr

dcache_loop:
    cache      0x11, 0($t0)
    cache      0x13, 0($t0)
    addiu      $t0, $t0, 64
    bne        $t0, $t1, loc_BFC0025C
    nop        
    jr         $ra
    nop        

# 2 nop's until 0xBFC00280 (due to compilation?)
preipl_payload:
	# insert stage2 here
preipl_payload_end:
    # end of stage2

# TODO nop's until this string
#ifdef PREIPL_VER_1
.string "Copyright (C) 2004 Sony Computer Entertainment Inc. All rights reserved." # at 0xBFC00FB0
#else
#ifdef PREIPL_VER_2
.string "Copyright (C) 2004,2005 Sony Computer Entertainment Inc. All rights reserved." # at 0xBFC00F80
#else
.string "Copyright (C) 2004,2005,2007 Sony Computer Entertainment Inc. All rights reserved." # at 0xBFC00F80
#endif
#endif

# TODO 0xBFC0FFC
#ifdef PREIPL_VER_1
.word 0x20040420
#else
#ifdef PREIPL_VER_2
.word 0x20050104
#else
.word 0x20070910
#endif
#endif

