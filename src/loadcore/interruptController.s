# Copyright (C) 2011, 2012 The uOFW team
# See the file COPYING for copying permission.

    .text
    .set noat
    .set noreorder

    .global loadCoreCpuSuspendIntr
    .global loadCoreCpuResumeIntr
    .global sub_00003D84

##
# sub_00007680
##

    .ent loadCoreCpuSuspendIntr
loadCoreCpuSuspendIntr:
    mfic       $v0, $0
	mtic       $zero, $0
	beqz       $v0, loc_000076A4
	nop        
	nop        
	nop        
	nop        
	nop        
	nop        
loc_000076A4: 
	jr         $ra
	nop    
    .end loadCoreCpuSuspendIntr

##
# sub_000076B4
##

    .ent loadCoreCpuResumeIntr
loadCoreCpuResumeIntr:
    mtic       $a0, $0
	nop        
	nop        
	nop        
	nop        
	jr         $ra
	nop     
    .end loadCoreCpuResumeIntr

##
# sub_00003D84
##

    .ent sub_00003D84
sub_00003D84:
    break      0x0
    j          sub_00003D84
    nop
    .end sub_00003D84


