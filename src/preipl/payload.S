#include "common_asm.h"

# PRE-IPL payload
# This code runs at 0x80010000
# Its role is to initialize just enough hardware to decrypt and run the IPL
_start:
    # Run an init script which initializes AW (GE) Edram, KIRK, NAND and Syscon
    lui        $a0, %hi(init_script)
    addiu      $a0, $a0, %lo(init_script)
    jal        script_exec
    nop        
    # TODO: the 16 MSB of this register are unknown
    lui        $t0, %hi(HW_SYS_PLL_FREQ)
    lw         $t1, %lo(HW_SYS_PLL_FREQ)($t0)
    srl        $t1, $t1, 16
    beqz       $t1, enable_gpio_pin
    nop        
    # Enable audio clock out I/O (???)
    lw         $t1, %lo(HW_SYS_IO_ENABLE)($t0)
    ori        $t1, $t1, HW_SYS_IO_ENABLE_AUDIOCLKOUT
    b          check_jigkick
    sw         $t1, %lo(HW_SYS_IO_ENABLE)($t0)

enable_gpio_pin:
    # Enable GPIO pin 4
    lw         $t1, %lo(HW_SYS_GPIO_ENABLE)($t0)
    ori        $t1, $t1, HW_SYS_GPIO_ENABLE_PIN4
    sw         $t1, %lo(HW_SYS_GPIO_ENABLE)($t0)

check_jigkick:
    # sleep for 10 "cycles"
    li         $a0, 10
    jal        sleep
    sync
    # read GPIO pin 4
    lui        $t0, %hi(HW_GPIO_READ)
    lw         $t0, %lo(HW_GPIO_READ)($t0)
    andi       $t0, $t0, HW_GPIO_READ_PIN4

    # if the pin is zero, boot from NAND
    lui        $t1, %hi(nand_read_block)
    addiu      $t1, $t1, %lo(nand_read_block)
    lui        $t2, %hi(nand_init)
    addiu      $t2, $t2, %lo(nand_init)
    beqz       $t0, start_decrypt
    nop        

    # otherwise, boot from MS
    lui        $t1, %hi(memorystick_read_block)
    addiu      $t1, $t1, %lo(memorystick_read_block)
    lui        $t2, %hi(memorystick_init)
    addiu      $t2, $t2, %lo(memorystick_init)

    # start decrypting the IPL
start_decrypt:
    lui        $at, %hi(read_block)
    sw         $t1, %lo(read_block)($at)
    sw         $zr, %lo(page_counter)($at)
    jalr       $t2
    nop        
    move       $s7, $zr

decrypt_loop:
    # read one page from NAND or MS to 0xBFD00000
    lui        $t9, %hi(read_block)
    lw         $t9, %lo(read_block)($t9)
    lui        $a0, %hi(page_counter)
    lw         $a0, %lo(page_counter)($a0)
    jalr       $t9
    lui        $a1, 0xBFD0
    bltz       $v0, infinite_loop
    nop
    # decrypt the page in-place at 0xBFD00000
    lui        $a0, 0xBFD0
    jal        kirk_priv_decrypt
    move       $a1, $a0
    bnez       $v0, infinite_loop
    nop
    # each decrypted page starts with four words:
    # - one giving a destination address, which must be non-zero (otherwise no copy is performed)
    # - one giving the length of the decrypted data
    # - one which is non-zero if it is the last block
    # - one which must be equal to the 'checksum' of the previous block, returned by memcpy, which is just the sum of the copied words (or 0 for the first loop)
    # first check the checksum
    lui        $s0, 0xBFD0
    lw         $v0, 12($s0)
    bne        $v0, $s7, infinite_loop
    lw         $a0, 0($s0)
    # then, check if the destination address is non-zero (otherwise just skip the block)
    beqz       $a0, after_copy
    lw         $a2, 4($s0)
    # copy the block from 0xBFD00010 to the specified address
    jal        _memcpy
    addiu      $a1, $s0, 16
    move       $s7, $v0

after_copy:
    # check if this is the last block
    lw         $t9, 8($s0)
    beqz       $t9, decrypt_another
    nop
    # it was the last block, now flush everything and jump to the IPL
    jal        dcacheWritebackInvalidateAll
    nop        
    jal        icacheWritebackInvalidateAll
    nop        
    jalr       $t9
    nop        

decrypt_another:
    lui        $at, %hi(page_counter)
    lw         $t0, %lo(page_counter)($at)
    addiu      $t0, $t0, 1
    b          decrypt_loop
    sw         $t0, %lo(page_counter)($at)

infinite_loop:
    b          infinite_loop
    nop

# ======================================================
# Initialize the NAND and find a valid page containing the IPL block indices
nand_init:
    move       $s0, $ra
    jal        nand_reset
    nop
    # Starting from 128 (and increasing by 32 each time), find if there is a valid page (based on spare data)
    li         $s1, 128

nand_init_valid_page:
    move       $a0, $s1
    lui        $a1, %hi(nand_ipl_block_tbl)
    addiu      $a1, $a1, %lo(nand_ipl_block_tbl)
    lui        $a2, %lo(nand_spare_data)
    addiu      $a2, $a2, %lo(nand_spare_data)
    jal        nand_read_page
    nop        
    bltz       $v0, nand_init_invalid_spare
    nop
    # check spare data; third word must be equal to 0x6DC64A38 (TODO find what this implies exactly)
    lw         $t0, 0($a2)
    lw         $t1, 4($a2)
    lw         $t2, 8($a2)
    lui        $t3, 0x6DC6
    ori        $t3, $t3, 0x4A38
    xor        $t0, $t1, $t3
    bnez       $t0, nand_init_invalid_spare
    nop        
    jr         $s0
    nop        

nand_init_invalid_spare:
    b          nand_init_valid_page
    addiu      $s1, $s1, 32

# ======================================================
# Read an IPL block from the NAND. Arguments: block index, and destination address
nand_read_block:
    lui        $at, %hi(save_ra)
    sw         $ra, %lo(save_ra)($at)
    sw         $a0, %lo(save_a0)($at) # note: the value is not restored so it's actually useless
    move       $s1, $a1
    # Compute in $s0 the page address from the block index, which is given by:
    # ((((u16*)nand_ipl_block_tbl)[a0 / 4] * 4) | (a0 & 3)) * 8
    # Basically an IPL block is composed of 8 pages, and 4 IPL blocks are stored consecutively in NAND,
    # and the table indicates the starting page index (divided by 32) for each group of 4 IPL blocks
    addiu      $t0, $at, %lo(nand_ipl_block_tbl)
    srl        $t1, $a0, 2
    sll        $t1, $t1, 1
    addu       $t0, $t0, $t1
    lhu        $t1, 0($t0)
    andi       $t0, $a0, 0x3
    sll        $t1, $t1, 2
    or         $t0, $t1, $t0
    sll        $s0, $t0, 3
    move       $s2, $zr

    # read 8 pages
nand_read_block_get_page:
    # read a page in the output buffer
    addu       $a0, $s0, $s2
    sll        $a1, $s2, 9
    addu       $a1, $s1, $a1
    lui        $a2, %hi(nand_spare_data)
    addiu      $a2, $a2, %lo(nand_spare_data)
    jal        nand_read_page
    nop
    bltz       $v0, nand_read_block_error
    nop
    # check if spare data is valid
    lw         $t0, 0($a2)
    lw         $t1, 4($a2)
    lw         $t2, 8($a2)
    lui        $t3, 0x6DC6
    ori        $t3, $t3, 0x4A38
    xor        $t0, $t1, $t3
    bnez       $t0, nand_read_block_error
    nop        
    # continue reading until we read 8 pages
    addiu      $s2, $s2, 1
    sltiu      $t0, $s2, 8
    bnez       $t0, nand_read_block_get_page
    nop        
    lui        $ra, %hi(save_ra)
    lw         $ra, %lo(save_ra)($ra)
    jr         $ra
    move       $v0, $zr

nand_read_block_error:
    lui        $ra, %hi(save_ra)
    lw         $ra, %lo(save_ra)($ra)
    jr         $ra
    li         $v0, -1

# ======================================================
memorystick_init:
    j          memorystick_init_2
    nop        

# ======================================================
memorystick_read_block:
    lui        $at, %hi(save_ra)
    sw         $ra, %lo(save_ra)($at)
    move       $s0, $a0
    move       $s1, $a1
    move       $s2, $zr

memorystick_read_block_loop:
    sll        $a0, $s0, 3
    addu       $a0, $a0, $s2
    addiu      $a0, $a0, 16
    sll        $a1, $s2, 9
    addu       $a1, $a1, $s1
    jal        memorystick_read_page
    nop        
    bltz       $v0, memorystick_read_block_loop
    nop        
    addiu      $s2, $s2, 1
    sltiu      $t0, $s2, 8
    bnez       $t0, memorystick_read_block_loop
    nop        
    lui        $ra, %hi(save_ra)
    lw         $ra, %lo(save_ra)($ra)
    jr         $ra
    move       $v0, $zr

# ======================================================
icacheWritebackInvalidateAll:
    mfc0       $t0, Config
    li         $t1, 4096
    ext        $t0, $t0, 9, 3
    sllv       $t1, $t1, $t0
    mtc0       $zr, TagLo
    mtc0       $zr, TagHi
    move       $t0, $zr

icacheWritebackInvalidateAll_sub:
    cache      0x1, 0($t0)
    cache      0x3, 0($t0)
    addiu      $t0, $t0, 64
    bne        $t0, $t1, icacheWritebackInvalidateAll_sub
    nop        
    jr         $ra
    nop        

# ======================================================
dcacheWritebackInvalidateAll:
    mfc0       $t0, Config
    li         $t1, 2048
    ext        $t0, $t0, 6, 3
    sllv       $t1, $t1, $t0
    move       $t0, $zr

dcacheWritebackInvalidateAll_sub:
    cache      0x14, 0($t0)
    cache      0x14, 0($t0)
    addiu      $t0, $t0, 64
    bne        $t0, $t1, dcacheWritebackInvalidateAll_sub
    nop        
    jr         $ra
    sync       

# ======================================================
nand_reset:
    # run a reset command on the NAND
    lui        $t0, %hi(HW_NAND_COMMAND)
    li         $t1, HW_NAND_COMMAND_RESET
    sw         $t1, %lo(HW_NAND_COMMAND)($t0)

    # wait for the command to finish
nand_reset_wait:
    lw         $t1, %lo(HW_NAND_STATUS)($t0)
    andi       $t1, $t1, 0x1
    beqz       $t1, nand_reset_wait
    nop
    # reset NAND (?)
    li         $t1, 1
    sw         $t1, %lo(HW_NAND_RESET)($t0)
    jr         $ra
    sync       

# ======================================================
# Read a NAND page.
# Takes three arguments: page number, output address, and spare data
nand_read_page:
    lui        $t0, %hi(HW_NAND_STATUS)
    # wait until NAND is ready
nand_read_page_wait_busy:
    lw         $t1, %lo(HW_NAND_STATUS)($t0)
    andi       $t1, $t1, 0x1
    beqz       $t1, nand_read_page_wait_busy
    sll        $t1, $a0, 10
    # set the page number
    sw         $t1, %lo(HW_NAND_DMA_ADDRESS)($t0)
    # enable DMA transfer with both page data & spare data transfer enabled
    li         $t1, 0x301
    sw         $t1, %lo(HW_NAND_DMA_CONTROL)($t0)
    # wait until DMA transfer is ready
nand_read_page_wait_dma:
    lw         $t1, %lo(HW_NAND_DMA_CONTROL)($t0)
    andi       $t1, $t1, 0x1
    bnez       $t1, nand_read_page_wait_dma
    nop        
    # check if the status is zero (ie no error), otherwise return -1
    lw         $t1, %lo(HW_NAND_DMA_STATUS)($t0)
    bnez       $t1, nand_read_page_exit
    li         $v0, -1
    lui        $t0, %hi(HW_NAND_DMA_BUFFER)
    # copy the spare data to the last argument
    lw         $t1, %lo(HW_NAND_DMA_SPARE0)($t0)
    lw         $t2, %lo(HW_NAND_DMA_SPARE1)($t0)
    lw         $v0, %lo(HW_NAND_DMA_SPARE2)($t0)
    sw         $t1, 0($a2)
    sw         $t2, 4($a2)
    sw         $v0, 8($a2)
    # copy 512 bytes of data to the output buffer
    move       $t1, $a1
    li         $v0, 512
nand_read_page_copy:
    lw         $t2, 0($t0)
    addiu      $v0, $v0, -4
    addiu      $t0, $t0, 4
    sw         $t2, 0($t1)
    bnez       $v0, nand_read_page_copy
    addiu      $t1, $t1, 4
    # return zero

nand_read_page_exit:
    jr         $ra
    nop        

# ======================================================
memorystick_init_2:
    move       $t8, $ra
    # enable memorystick clock, bus clock, I/O, and reset
    lui        $a0, %hi(memorystick_init_script)
    addiu      $a0, $a0, %lo(memorystick_init_script)
    jal        script_exec
    nop
    # reset memory stick
    lui        $t9, %hi(HW_MEMORYSTICK_SYS)
    li         $t1, HW_MEMORYSTICK_SYS_RESET
    sw         $t1, %lo(HW_MEMORYSTICK_SYS)($t9)
    # wait for the memorystick interface to have reset
memorystick_init_wait_reset:
    lw         $t1, %lo(HW_MEMORYSTICK_SYS)($t9)
    andi       $t2, $t1, HW_MEMORYSTICK_SYS_RESET
    bnez       $t2, memorystick_init_wait_reset
    nop        
    jal        memorystick_check_register
    nop        
    jal        memorystick_check_status
    nop        

memorystick_wait_interrupt:
    jal        memorystick_get_interrupt
    nop        
    bltz       $v0, memorystick_wait_interrupt
    nop        
    andi       $v0, $v0, 0x80 # TODO find what this means
    beqz       $v0, memorystick_wait_interrupt
    nop        
    jr         $t8
    move       $v0, $zr

# ======================================================
memorystick_read_page:
    move       $t6, $a1
    move       $t8, $ra
    lui        $t9, %hi(HW_MEMORYSTICK_START_TPC)
    li         $at, 0x9007 # ex set cmd, size 7
    sw         $at, %lo(HW_MEMORYSTICK_START_TPC)($t9)
    wsbw       $a1, $a0
    srl        $a1, $a1, 8
    srl        $t1, $a0, 24
    sll        $t1, $t1, 24
    lui        $a0, 0x1
    ori        $a0, $a0, 0x20
    jal        memorystick_run_tpc # TODO run TPC with a0 = 0x10020 | ((a0 >> 24) << 24), a1 = wsbw(a0) >> 8
    or         $a0, $a0, $t1
    bltz       $v0, memorystick_read_page_error
    nop        
    jal        memorystick_wait
    nop        
memorystick_read_page_wait_interrupt:
    jal        memorystick_get_interrupt
    nop        
    bltz       $v0, memorystick_read_page_error
    andi       $t0, $v0, 0x20 # TODO
    beqz       $t0, memorystick_read_page_wait_interrupt
    andi       $t0, $v0, 0x40 # TODO
    bnez       $t0, memorystick_read_page_error
    nop        
    li         $at, 8704
    sw         $at, %lo(HW_MEMORYSTICK_START_TPC)($t9)
    move       $a0, $t6
    jal        memorystick_read_tpc
    li         $a1, 512
    bltz       $v0, memorystick_read_page_error
    nop        
    jal        memorystick_check_status
    nop        
    bltz       $v0, memorystick_read_page_error
    nop        
    jal        memorystick_wait
    nop        
    b          memorystick_wait_interrupt
    nop        

memorystick_read_page_error:
    jr         $t8
    li         $v0, -1

# ======================================================
memorystick_run_tpc:
    sw         $a0, %lo(HW_MEMORYSTICK_TPC)($t9)
    b          memorystick_check_status
    sw         $a1, %lo(HW_MEMORYSTICK_TPC)($t9)

# ======================================================
memorystick_read_tpc:
    lw         $t1, %lo(HW_MEMORYSTICK_STATUS)($t9)
    andi       $t2, $t1, HW_MEMORYSTICK_STATUS_TIMEOUT
    bnez       $t2, memorystick_read_tpc_error
    andi       $t2, $t1, HW_MEMORYSTICK_STATUS_FIFO_RW
    beqz       $t2, memorystick_read_tpc
    nop        
    lw         $v0, %lo(HW_MEMORYSTICK_TPC)($t9)
    addiu      $a1, $a1, -4
    sw         $v0, 0($a0)
    bgtz       $a1, memorystick_read_tpc
    addiu      $a0, $a0, 4
    jr         $ra
    move       $v0, $zr

memorystick_read_tpc_error:
    jr         $ra
    li         $v0, -1

# ======================================================
# Wait until MemoryStick is ready, then check if it did not timeout or receive a CRC error
# Return -1 in case of error, 0 otherwise
memorystick_check_status:
    lw         $t1, %lo(HW_MEMORYSTICK_STATUS)($t9)
    andi       $v0, $t1, HW_MEMORYSTICK_STATUS_READY
    beqz       $v0, memorystick_check_status
    andi       $v0, $t1, (HW_MEMORYSTICK_STATUS_TIMEOUT & HW_MEMORYSTICK_CRC_ERROR)
    bnez       $v0, memorystick_check_status_error
    nop        
    jr         $ra
    move       $v0, $zr
memorystick_check_status_error:
    jr         $ra
    li         $v0, -1

# ======================================================
memorystick_check_register:
    move       $t7, $ra
    # Set the r/w register address
    lui        $t9, %hi(HW_MEMORYSTICK_START_TPC)
    li         $at, 0x8004 # rw register address (0x8000), size 4
    sw         $at, %lo(HW_MEMORYSTICK_START_TPC)($t9)
    lui        $at, 0x610 # 0x06100800 (TODO what does this value mean?)
    ori        $at, $at, 0x800
    sw         $at, %lo(HW_MEMORYSTICK_TPC)($t9)
    sw         $zr, %lo(HW_MEMORYSTICK_TPC)($t9)
    jal        memorystick_check_status
    nop        
    bltz       $v0, memorystick_check_register
    nop        
    lui        $a0, %hi(memorystick_reg_0)
    addiu      $a0, $a0, %lo(memorystick_reg_0)
    li         $a1, 8
    jal        memorystick_read_reg
    nop        
    lui        $a0, %hi(memorystick_reg_0)
    lw         $a0, %lo(memorystick_reg_0)($a0)
    lui        $a1, %hi(memorystick_reg_1)
    lw         $a1, %lo(memorystick_reg_1)($a1)
    srl        $at, $a0, 16
    andi       $at, $at, 0x15
    # TODO meaning of (a0 >> 16) & 0x15
    bnez       $at, memorystick_check_register_error
    nop        
    jr         $t7
    nop        

memorystick_check_register_error:
    jr         $t7
    li         $v0, -1

# ======================================================
memorystick_read_reg:
    li         $at, 0x4000 # TPC command to read reg, size 0
    or         $at, $at, $a1
    sw         $at, %lo(HW_MEMORYSTICK_START_TPC)($t9)
    b          memorystick_read_tpc
    nop        

# ======================================================
memorystick_get_interrupt:
    li         $at, 0x7001 # get interrupt, size 1
    sw         $at, %lo(HW_MEMORYSTICK_START_TPC)($t9)
memorystick_get_interrupt_wait_fifo:
    lw         $t1, %lo(HW_MEMORYSTICK_STATUS)($t9)
    andi       $t2, $t1, HW_MEMORYSTICK_STATUS_TIMEOUT
    bnez       $t2, memorystick_get_interrupt_error
    andi       $t2, $t1, HW_MEMORYSTICK_STATUS_FIFO_RW
    beqz       $t2, memorystick_get_interrupt_wait_fifo
    nop        
    lw         $v0, %lo(HW_MEMORYSTICK_TPC)($t9)
    lw         $zr, %lo(HW_MEMORYSTICK_TPC)($t9)
memorystick_get_interrupt_wait_ready:
    lw         $t1, %lo(HW_MEMORYSTICK_STATUS)($t9)
    andi       $t2, $t1, HW_MEMORYSTICK_STATUS_TIMEOUT
    bnez       $t2, memorystick_get_interrupt_error
    andi       $t2, $t1, HW_MEMORYSTICK_STATUS_READY
    beqz       $t2, memorystick_get_interrupt_wait_ready
    nop        
    jr         $ra
    andi       $v0, $v0, 0xFF

memorystick_get_interrupt_error:
    jr         $ra
    li         $v0, -1

# ======================================================
# Wait for some something related to the memory stick
memorystick_wait:
    lw         $t1, %lo(HW_MEMORYSTICK_STATUS)($t9)
    andi       $t2, $t1, HW_MEMORYSTICK_STATUS_UNK13 # TODO
    beqz       $t2, memorystick_wait
    nop        
    jr         $ra
    nop        

# ======================================================
# Execute KIRK command 0x01 on given source & destination buffers
kirk_priv_decrypt:
    lui        $t9, %hi(HW_KIRK_COMMAND)
    li         $t0, HW_KIRK_COMMAND_PRIV_DEC
    sw         $t0, %lo(HW_KIRK_COMMAND)($t9)
    ext        $t0, $a1, 0, 29
    sw         $t0, %lo(HW_KIRK_SRC_BUF)($t9)
    ext        $t0, $a0, 0, 29
    sw         $t0, %lo(HW_KIRK_DST_BUF)($t9)
    li         $t0, HW_KIRK_START_PHASE1
    sw         $t0, %lo(HW_KIRK_START)($t9)

    # wait for the operation to finish
kirk_priv_decrypt_wait:
    lw         $t0, %lo(HW_KIRK_STATUS)($t9)
    andi       $t0, $t0, 0x11
    beqz       $t0, kirk_priv_decrypt_wait
    andi       $t1, $t0, 0x10

    # if it needs phase2, go there
    bnez       $t1, kirk_priv_decrypt_phase2
    sw         $t0, %lo(HW_KIRK_COMMAND_END)($t9)
    jr         $ra
    lw         $v0, %lo(HW_KIRK_COMMAND_RESULT)($t9)

kirk_priv_decrypt_phase2:
    li         $t0, HW_KIRK_START_PHASE2
    sw         $t0, %lo(HW_KIRK_START)($t9)

kirk_priv_decrypt_phase2_wait:
    lw         $t0, %lo(HW_KIRK_STATUS)($t9)
    andi       $t0, $t0, HW_KIRK_STATUS_PHASE2_FINISH
    beqz       $t0, kirk_priv_decrypt_phase2_wait
    li         $v0, -1
    sw         $t0, %lo(HW_KIRK_COMMAND_END)($t9)
    jr         $ra
    sync       

# ======================================================
# Memcpy returning a 'checksum' which is just the XOR of the copied words
_memcpy:
    move       $v0, $zr

_memcpy_inner:
    lw         $v1, 0($a1)
    addiu      $a1, $a1, 4
    addiu      $a2, $a2, -4
    sw         $v1, 0($a0)
    addu       $v0, $v0, $v1
    bgtz       $a2, _memcpy_inner
    addiu      $a0, $a0, 4
    jr         $ra
    nop        

# ======================================================
# Script execution: store hardware commands as data to save some code space
# Each command is composed of two words (unless the command is 'stop'): an
# address whose 4 MSB have been replaced by the command op (and will be
# replaced by 0xB as the destination address), and a value
# The op can be 0 (store value at destination), 1 (enable flags), 2 (disable
# flags), 3 (wait while flag is set), 4 (wait while flag is not set), 5 (sleep).
# Newer preipl versions add command 6 (read a hardware register n times).
# Any other operand interrupts script execution.
script_exec:
    move       $t0, $a0
    move       $t9, $ra

cmd_loop:
    lw         $a0, 0($t0)
    # t1 = a0 >> 28: this is the operand
    srl        $t1, $a0, 28
    # a0 = 0xB0000000 | (a0 & 0x0FFFFFFF): this is the destination address
    sll        $a0, $a0, 4
    srl        $a0, $a0, 4
    lui        $at, 0xB000
    or         $a0, $a0, $at
    lw         $a1, 4($t0)
    # t1 == 0: *a0 = a1
    beqz       $t1, cmd_store
    addiu      $t2, $t1, -1
    # t1 == 1: *a0 |= a1
    beqz       $t2, cmd_or
    addiu      $t2, $t1, -2
    # t1 == 2: *a0 &= a1
    beqz       $t2, cmd_and
    addiu      $t2, $t1, -3
    # t1 == 3: while (*a0 & a1 != 0) {}
    beqz       $t2, cmd_wait
    move       $at, $zr
    addiu      $t2, $t2, -4
    # t1 == 4: while (*a0 & a1 == 0) {}
    beqz       $t2, cmd_wait
    not        $at, $zr
    addiu      $t2, $t1, -5
    # t1 == 5: sleep(a1)
    beqz       $t2, cmd_sleep
#ifndef PREIPL_VER_1
    # t1 == 6: while (--a1 > 0) { (void)*a0; }
    addiu      $t2, $t1, -6
    beqz       $t2, cmd_repeatread
#endif
    nop
    # otherwise, exit
    jr         $t9
    nop        

cmd_sleep:
    # sleep(a1)
    jal        sleep
    move       $a0, $a1

cmd_continue:
    b          cmd_loop
    addiu      $t0, $t0, 8

cmd_store:
    # *a0 = a1
    b          cmd_continue
    sw         $a1, 0($a0)

cmd_or:
    # *a0 |= a1
    lw         $t1, 0($a0)
    or         $t1, $t1, $a1
    b          cmd_continue
    sw         $t1, 0($a0)

cmd_and:
    # *a0 &= a1
    lw         $t1, 0($a0)
    and        $t1, $t1, $a1
    b          cmd_continue
    sw         $t1, 0($a0)

cmd_wait:
    # while ((*a0 ^ at) & a1 != 0) {}
    lw         $t1, 0($a0)
    xor        $t1, $t1, $at
    and        $t1, $t1, $a1
    bnez       $t1, cmd_wait
    nop        
    b          cmd_continue
    nop        

#ifndef PREIPL_VER_1
cmd_repeatread:
    # while (--a1 > 0) { (void)*a0; }
    lw         $t1, 0($a0)
    addiu      $a1, $a1, -1
    bnez       $a1, cmd_repeatread
    nop
    b          cmd_continue
    nop

# ======================================================
# wait for about 100 * (first argument) CPU cycles (make 96 loops)
sleep:
    # at = a0 * 15
    sll        $at, $a0, 1
    addu       $at, $at, $a0
    sll        $at, $at, 5

sleep_loop:
    bnez       $at, sleep_loop
    addiu      $at, $at, -1
    jr         $ra
    nop        

# note: nothing at 0x80010784

save_ra: # at 0x80010800
    .word 0
save_a0: # at 0x80010804
    .word 0

read_block: # at 0x80010808
    .word 0

page_counter: # at 0x8001080C
    .word 0

nand_spare_data: # at 0x80010810
    .word 0
    .word 0
    .word 0

nand_ipl_block_tbl: # at 0x8001081C
    .skip 512

memorystick_reg_0: # at 0x80010A1C
    .word 0

memorystick_reg_1: # at 0x80010A20
    .word 0

# note: nothing at 0x80010A24

init_script: # at 0x80010A80
    # 0xBC100058 |= 0x00800000 Enable GPIO clock
    .word 0x1C100058
    .word 0x00800000
    # 0xBC100050 |= 0x0000608E Enable bus clock for AW (which is GE) (RegA, RegB, Edram), KIRK, NAND (EMCSM) and APB (for syscon?)
    .word 0x1C100050
    .word 0x0000608E
#ifndef PREIPL_VER_1
    # 0xBC100050 &= ~0x8E      Disable bus clock for AW (RegA, RegB, Edram) and KIRK
    .word 0x2C100050
    .word 0xFFFFFF71
#endif
    # 0xBC10004C &= ~0x408     Clear reset for AW and KIRK
    .word 0x2C10004C
    .word 0xFFFFFBF7
#ifndef PREIPL_VER_1
    # 0xBC100050 |= 0x0000008E Re-enable bus clock for AW (RegA, RegB, Edram) and KIRK
    .word 0x1C100050
    .word 0x0000008E
#endif
    # 0xBC100078 |= 2          IO enable NAND (EMCSM)
    .word 0x1C100078
    .word 0x00000002
    # 0xBE240000 &= ~0x10      Disable GPIO pin 4 output
    .word 0x2E240000
    .word 0xFFFFFFEF
    # 0xBE240040 |= 0x10       Enable GPIO pin 4 input
    .word 0x1E240040
    .word 0x00000010
#ifndef PREIPL_VER_1
    # for (i = 0; i < 0x800; i++) { (void)*0xBC100040; }  Read 0xBC100040 (TODO why?)
    .word 0x6C100040
    .word 0x00000800
#endif
    # sleep(1)                 Wait
    .word 0x50000000
    .word 0x00000001
    # 0xBD500010 = 1           Initialize GE Edram
    .word 0x0D500010
    .word 0x00000001
    # while (*0xBD500010 & 1 != 0) {}   Wait for the Edram to be initialized
    .word 0x3D500010
    .word 0x00000001
    # 0xBD500040 = 1           Finish initializing GE Edram
    .word 0x0D500040
    .word 0x00000001
    # end
    .word 0xF0000000

memorystick_init_script: # at 0x80010AD4, or 0x80010AEC for ver2
    # 0xBC100054 |= 0x100  Enable clock for memorystick interface 1
    .word 0x1C100054
    .word 0x00000100
    # 0xBC100050 |= 0x400  Enable bus clock for memorystick interface 1
    .word 0x1C100050
    .word 0x00000400
#ifdef PREIPL_VER_1
    # 0xBC100078 |= 0x10   Enable I/O for memorystick interface 1
    .word 0x1C100078
    .word 0x00000010
    # 0xBC10004C &= ~0x100 Reset memorystick interface 1
    .word 0x2C10004C
    .word 0xFFFFFEFF
#else
    # 0xBC100050 &= ~0x400 Disable bus clock for memorystick interface 1
    .word 0x2C100050
    .word 0xFFFFFBFF
    # 0xBC100054 &= ~0x100 Disable clock for memorystick interface 1
    .word 0x2C100054
    .word 0xFFFFFEFF
    # 0xBC10004C &= ~0x100 Reset memorystick interface 1
    .word 0x2C10004C
    .word 0xFFFFFEFF
    # 0xBC100054 |= 0x100  Re-enable clock for memorystick interface 1
    .word 0x1C100054
    .word 0x00000100
    # 0xBC100050 |= 0x400  Re-enable bus clock for memorystick interface 1
    .word 0x1C100050
    .word 0x00000400
    # 0xBC100078 |= 0x10   Enable I/O for memorystick interface 1
    .word 0x1C100078
    .word 0x00000010
#endif
    # end
    .word 0xF0000000

