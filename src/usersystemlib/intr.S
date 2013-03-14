# Copyright (C) 2011, 2012, 2013 The uOFW team
# See the file COPYING for copying permission.

    .text
    .set noat
    .set noreorder

    .global sceKernelCpuSuspendIntr
    .global sceKernelCpuResumeIntr
    .global sceKernelCpuResumeIntrWithSync
    .global sceKernelIsCpuIntrSuspended
    .global sceKernelIsCpuIntrEnable
    .global sub_00000208

##
# Kernel_Library_092968F4
##

    .ent sceKernelCpuSuspendIntr
sceKernelCpuSuspendIntr:
    mfic        $v0, $0
    mtic        $0, $0
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    jr          $ra
    nop
    .end sceKernelCpuSuspendIntr

##
# Kernel_Library_5F10D406
##

    .ent sceKernelCpuResumeIntr
sceKernelCpuResumeIntr:
    mtic        $a0, $0
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    jr          $ra
    nop
    .end sceKernelCpuResumeIntr

##
# Kernel_Library_3B84732D
##

    .ent sceKernelCpuResumeIntrWithSync
sceKernelCpuResumeIntrWithSync:
    sync
    nop
    sync
    nop
    mtic        $a0, $0
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    jr          $ra
    nop
    .end sceKernelCpuResumeIntrWithSync

##
# Kernel_Library_47A0B729
##

    .ent sceKernelIsCpuIntrSuspended
sceKernelIsCpuIntrSuspended:
    jr          $ra
    sltiu       $v0, $a0, 1
    .end sceKernelIsCpuIntrSuspended

##
# Kernel_Library_B55249D2
##

    .ent sceKernelIsCpuIntrEnable
sceKernelIsCpuIntrEnable:
    mfic        $v0, $0
    jr          $ra
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    .end sceKernelIsCpuIntrEnable

##
# sub_00000208
##

    .ent sub_00000208
sub_00000208:
    jr          $ra
    syscall     0x0
    .end sub_00000208
