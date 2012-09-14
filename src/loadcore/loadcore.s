# Copyright (C) 2011, 2012 The uOFW team
# See the file COPYING for copying permission.

    .text
    .set noat
    .set noreorder

    .globl loadCoreClearMem
    .globl sceLoadCorePrimarySyscallHandler

    .global g_ToolBreakMode

##
# sub_00000000
##

# loadCoreClearMem(u32 *baseAddr, u32 size)
# zeros out memory range starting at "baseAddr"
# for "size" words.
    
    .ent loadCoreClearMem
loadCoreClearMem:
    move       $v0, $a0 # origAddr = baseAddr
	addu       $t0, $a0, $a1
	addiu      $t0, $t0, -4 # maxWordAddr = (baseAddr + size) - 0x4
loop1:		; Refs: 0x00000010 
	sw         $zr, 0($a0) # do { *baseAddr = 0; } 
	bne        $t0, $a0, loop1 # while (baseAddr++ != maxWordAddr);
	addiu      $a0, $a0, 4
loop2:		; Refs: 0x0000001C 
	sw         $zr, 0($v0) # do { *origAddr = 0; }
	bne        $t0, $v0, loop2 # while (origAddr++ != maxWordAddr);
	addiu      $v0, $v0, 4
	break      0x0
    .end loadCoreClearMem

##
# sub_00000028
##

    .ent sceLoadCorePrimarySyscallHandler
sceLoadCorePrimarySyscallHandler:
    lui        $v0, %hi(g_ToolBreakMode)    
	lw         $v0, %lo(g_ToolBreakMode)($v0) # Data ref 0x000083C4 
	beq        $zr, $v0, error # if (g_ToolBreakMode == 0) 
	nop        
	lui        $v0, 0x1
	addiu      $v0, $v0, -32576 # Data ref 0x000080C0
	cfc0       $v1, $16 # Get currentTCB
	sw         $v0, 0($v1) # and save it
	cfc0       $v1, $2 # Get COP 0's Status register
	sw         $v1, 0($v0) # and save it
	mfc0       $v1, EPC # Get the memory address of the recently executed SYSCALL exception.
	addiu      $v1, $v1, -4 # Set EPC (Exception Program Counter) to address of jr $ra instruction right below the current SYSCALL
	mtc0       $v1, EPC
	sw         $v1, 4($v0) # save the new EPC register value
	sw         $sp, 8($v0) # Save $sp
	sw         $ra, 12($v0) # Save $ra
	sw         $k1, 16($v0) # Save $k1
	cfc1       $v1, $31
	sw         $v1, 20($v0) # Save COP 0's control register 31
	cfc0       $v1, $4 # Save COP 0's control register 4
	sw         $v1, 24($v0)
	break      0x10000 # ?
	nop        
error:
	lui        $v0, 0x8002
	ori        $v0, $v0, 0x13A # return SCE_ERROR_KERNEL_LIBRARY_IS_NOT_LINKED
	mtc0       $ra, EPC # Set EPC to the return address + 0x8 to resume
	nop                 # execution from the recently call to a stub
	nop        
	eret       # jump to the value in EPC, thus resume execution of the 
	nop        # running application.
    .end sceLoadCorePrimarySyscallHandler

