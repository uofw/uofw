	.file	1 "syscon.c"
	.section .mdebug.eabi32
	.previous
	.section .gcc_compiled_long32
	.previous
	.gnu_attribute 4, 2
 #APP
	    .set push
    .section .lib.ent.top, "a", @progbits
    .align 2
    .word 0
__lib_ent_top:
    .section .lib.ent.btm, "a", @progbits
    .align 2
__lib_ent_bottom:
    .word 0
    .section .lib.stub.top, "a", @progbits
    .align 2
    .word 0
__lib_stub_top:
    .section .lib.stub.btm, "a", @progbits
    .align 2
__lib_stub_bottom:
    .word 0
    .set pop
    .text

 #NO_APP
	.globl	module_info
	.section	.rodata.sceModuleInfo,"aw",@progbits
	.align	4
	.type	module_info, @object
	.size	module_info, 52
module_info:
	.half	4103
	.byte	11
	.byte	1
	.ascii	"sceSYSCON_Driver\000"
	.space	10
	.byte	0
	.word	_gp
	.word	__lib_ent_top
	.word	__lib_ent_bottom
	.word	__lib_stub_top
	.word	__lib_stub_bottom
	.globl	module_sdk_version
	.rdata
	.align	2
	.type	module_sdk_version, @object
	.size	module_sdk_version, 4
module_sdk_version:
	.word	101056528
	.globl	g_SysconEvName
	.data
	.align	2
	.type	g_SysconEvName, @object
	.size	g_SysconEvName, 10
g_SysconEvName:
	.ascii	"SceSyscon\000"
	.globl	g_SysconEv
	.align	2
	.type	g_SysconEv, @object
	.size	g_SysconEv, 64
g_SysconEv:
	.word	64
	.word	g_SysconEvName
	.word	16776960
	.word	_sceSysconSysEventHandler
	.word	0
	.word	0
	.word	0
	.word	0
	.word	0
	.word	0
	.word	0
	.word	0
	.word	0
	.word	0
	.word	0
	.word	0

	.comm	g_4E50,96,4

	.comm	g_4EB0,384,8
	.section	.rodata.str1.4,"aMS",@progbits,1
	.align	2
$LC0:
	.ascii	"Syscon Cmd remained\012\000"
	.text
	.align	2
	.globl	_sceSysconModuleRebootBefore
	.set	nomips16
	.ent	_sceSysconModuleRebootBefore
	.type	_sceSysconModuleRebootBefore, @function
_sceSysconModuleRebootBefore:
	.frame	$sp,8,$31		# vars= 0, regs= 1/0, args= 0, gp= 0
	.mask	0x80000000,-4
	.fmask	0x00000000,0
	.set	noreorder
	.set	nomacro
	addiu	$sp,$sp,-8
	sw	$31,4($sp)
	lui	$2,%hi(g_4EB0+376)
	lw	$2,%lo(g_4EB0+376)($2)
	beq	$2,$0,$L7
	li	$3,1			# 0x1

	jal	sceSyscon_driver_765775EB
	move	$4,$0

	li	$3,1			# 0x1
$L7:
	lui	$2,%hi(g_4EB0+48)
	sw	$3,%lo(g_4EB0+48)($2)
	move	$4,$0
	jal	sceSysconCmdSync
	li	$5,1			# 0x1

	beq	$2,$0,$L3
	lui	$4,%hi($LC0)

	jal	Kprintf
	addiu	$4,$4,%lo($LC0)

	j	$L6
	move	$4,$0

$L5:
	jal	sceKernelDelayThread
	li	$4,1000			# 0x3e8

	move	$4,$0
$L6:
	jal	sceSysconCmdSync
	li	$5,1			# 0x1

	bne	$2,$0,$L5
	nop

$L3:
	jal	sceSysconEnd
	nop

	move	$2,$0
	lw	$31,4($sp)
	j	$31
	addiu	$sp,$sp,8

	.set	macro
	.set	reorder
	.end	_sceSysconModuleRebootBefore
	.size	_sceSysconModuleRebootBefore, .-_sceSysconModuleRebootBefore
	.section	.rodata.str1.4
	.align	2
$LC1:
	.ascii	"SceSysconSync0\000"
	.text
	.align	2
	.globl	sceSysconInit
	.set	nomips16
	.ent	sceSysconInit
	.type	sceSysconInit, @function
sceSysconInit:
	.frame	$sp,16,$31		# vars= 0, regs= 3/0, args= 0, gp= 0
	.mask	0x80030000,-4
	.fmask	0x00000000,0
	.set	noreorder
	.set	nomacro
	addiu	$sp,$sp,-16
	sw	$31,12($sp)
	sw	$17,8($sp)
	sw	$16,4($sp)
	move	$4,$0
	jal	sceSysregSpiClkSelect
	li	$5,1			# 0x1

	jal	sceSysregSpiClkEnable
	move	$4,$0

	jal	sceSysregSpiIoEnable
	move	$4,$0

	li	$2,-1101529088			# 0xffffffffbe580000
	li	$3,207			# 0xcf
	sw	$3,0($2)
	li	$3,4			# 0x4
	sw	$3,4($2)
	sw	$0,20($2)
	sw	$0,36($2)
	lui	$17,%hi(g_4EB0)
	addiu	$4,$17,%lo(g_4EB0)
	move	$5,$0
	jal	memset
	li	$6,384			# 0x180

	lui	$16,%hi(g_4E50)
	addiu	$4,$16,%lo(g_4E50)
	move	$5,$0
	jal	memset
	li	$6,96			# 0x60

	addiu	$16,$16,%lo(g_4E50)
	li	$2,2			# 0x2
	sb	$2,13($16)
	li	$2,16			# 0x10
	sb	$2,12($16)
	addiu	$16,$17,%lo(g_4EB0)
	li	$2,1			# 0x1
	sw	$2,372($16)
	li	$2,20000			# 0x4e20
	sw	$2,60($16)
	li	$2,-1			# 0xffffffffffffffff
	sb	$2,88($16)
	sb	$2,64($16)
	sb	$2,65($16)
	sb	$2,66($16)
	sb	$2,67($16)
	sb	$2,68($16)
	sb	$2,74($16)
	sb	$2,75($16)
	sb	$2,76($16)
	sb	$2,81($16)
	sb	$2,82($16)
	sb	$2,83($16)
	sb	$2,84($16)
	sb	$2,85($16)
	sb	$2,86($16)
	sb	$2,87($16)
	li	$2,4000			# 0xfa0
	sw	$2,56($16)
	sw	$0,44($16)
	jal	sceGpioPortClear
	li	$4,8			# 0x8

	jal	sceKernelGetSystemTimeLow
	nop

	sw	$2,40($16)
	li	$4,3			# 0x3
	jal	sceGpioSetPortMode
	move	$5,$0

	li	$4,4			# 0x4
	jal	sceGpioSetPortMode
	li	$5,1			# 0x1

	li	$4,4			# 0x4
	jal	sceGpioSetIntrMode
	li	$5,3			# 0x3

	li	$4,4			# 0x4
	li	$5,4			# 0x4
	lui	$6,%hi(_sceSysconGpioIntr)
	addiu	$6,$6,%lo(_sceSysconGpioIntr)
	jal	sceKernelRegisterSubIntrHandler
	move	$7,$0

	li	$4,4			# 0x4
	jal	sceKernelEnableSubIntr
	li	$5,4			# 0x4

	lui	$4,%hi(g_SysconEv)
	jal	sceKernelRegisterSysEventHandler
	addiu	$4,$4,%lo(g_SysconEv)

	lui	$16,%hi(g_4EB0+332)
	addiu	$16,$16,%lo(g_4EB0+332)
$L9:
	jal	sceSysconGetBaryonVersion
	move	$4,$16

	bltz	$2,$L9
	nop

	lui	$16,%hi(g_4EB0+336)
	addiu	$16,$16,%lo(g_4EB0+336)
$L17:
	jal	sceSysconGetTimeStamp
	move	$4,$16

	bltz	$2,$L17
	lui	$2,%hi(g_4EB0+336)

	lb	$4,%lo(g_4EB0+336)($2)
	move	$2,$0
	beq	$4,$0,$L11
	move	$3,$0

	lui	$6,%hi(g_4EB0+336)
	addiu	$6,$6,%lo(g_4EB0+336)
$L12:
	move	$5,$2
	srl	$7,$2,31
	sll	$2,$3,1
	or	$2,$7,$2
	sll	$3,$5,1
	srl	$7,$3,30
	sll	$5,$2,2
	or	$5,$7,$5
	sll	$7,$3,2
	addu	$7,$3,$7
	sltu	$3,$7,$3
	addu	$2,$2,$5
	addu	$5,$3,$2
	sra	$2,$4,31
	addu	$7,$4,$7
	sltu	$4,$7,$4
	addu	$5,$2,$5
	addu	$3,$4,$5
	addiu	$6,$6,1
	lb	$4,0($6)
	bne	$4,$0,$L12
	move	$2,$7

$L11:
	lui	$4,%hi(g_4EB0)
	addiu	$4,$4,%lo(g_4EB0)
	sw	$2,360($4)
	sw	$3,364($4)
	lbu	$2,334($4)
	addiu	$3,$2,-48
	andi	$3,$3,0x00ff
	sltu	$3,$3,17
	beq	$3,$0,$L13
	andi	$2,$2,0xf0

	li	$3,-1			# 0xffffffffffffffff
	sb	$3,72($4)
	sb	$3,70($4)
	j	$L14
	sb	$3,71($4)

$L13:
	li	$3,64			# 0x40
	bne	$2,$3,$L15
	lui	$2,%hi(g_4EB0)

	addiu	$2,$2,%lo(g_4EB0)
	li	$3,-2			# 0xfffffffffffffffe
	sb	$3,72($2)
	li	$3,-1			# 0xffffffffffffffff
	sb	$3,71($2)
	j	$L14
	sb	$3,70($2)

$L15:
	addiu	$2,$2,%lo(g_4EB0)
	li	$3,-2			# 0xfffffffffffffffe
	sb	$3,72($2)
	sb	$3,70($2)
	sb	$3,71($2)
$L14:
	jal	_sceSysconInitPowerStatus
	nop

	lui	$4,%hi($LC1)
	addiu	$4,$4,%lo($LC1)
	li	$5,1			# 0x1
	move	$6,$0
	li	$7,1			# 0x1
	jal	sceKernelCreateSema
	move	$8,$0

	lui	$3,%hi(g_4EB0+380)
	sw	$2,%lo(g_4EB0+380)($3)
	move	$2,$0
	lw	$31,12($sp)
	lw	$17,8($sp)
	lw	$16,4($sp)
	j	$31
	addiu	$sp,$sp,16

	.set	macro
	.set	reorder
	.end	sceSysconInit
	.size	sceSysconInit, .-sceSysconInit
	.align	2
	.globl	_sceSysconInitPowerStatus
	.set	nomips16
	.ent	_sceSysconInitPowerStatus
	.type	_sceSysconInitPowerStatus, @function
_sceSysconInitPowerStatus:
	.frame	$sp,16,$31		# vars= 8, regs= 2/0, args= 0, gp= 0
	.mask	0x80010000,-4
	.fmask	0x00000000,0
	.set	noreorder
	.set	nomacro
	addiu	$sp,$sp,-16
	sw	$31,12($sp)
	sw	$16,8($sp)
	lui	$16,%hi(g_4EB0+356)
	addiu	$16,$16,%lo(g_4EB0+356)
$L22:
	jal	sceSysconGetPommelVersion
	move	$4,$16

	bltz	$2,$L22
	nop

$L33:
	jal	sceSysconGetPowerStatus
	move	$4,$sp

	bltz	$2,$L33
	lui	$2,%hi(g_4EB0+332)

	lhu	$4,%lo(g_4EB0+334)($2)
	andi	$2,$4,0xf0
	beql	$2,$0,$L36
	lw	$16,0($sp)

	li	$3,16			# 0x10
	bne	$2,$3,$L25
	lw	$3,0($sp)

	lw	$16,0($sp)
$L36:
	lui	$2,%hi(g_4EB0)
	addiu	$2,$2,%lo(g_4EB0)
	ext	$3,$16,9,1
	sb	$3,74($2)
	ext	$3,$16,12,1
	sb	$3,75($2)
	ext	$3,$16,20,1
	jal	_sceSysconGetPommelType
	sb	$3,76($2)

	li	$3,256			# 0x100
	bne	$2,$3,$L26
	lui	$2,%hi(g_4EB0)

	addiu	$2,$2,%lo(g_4EB0)
	ext	$3,$16,2,1
	sb	$3,82($2)
	ext	$16,$16,1,1
	j	$L27
	sb	$16,81($2)

$L26:
	addiu	$2,$2,%lo(g_4EB0)
	li	$3,1			# 0x1
	sb	$3,82($2)
	sb	$3,81($2)
$L27:
	lw	$3,0($sp)
	lui	$2,%hi(g_4EB0)
	addiu	$2,$2,%lo(g_4EB0)
	ext	$4,$3,3,1
	sb	$4,83($2)
	ext	$3,$3,19,1
	sb	$3,84($2)
	lbu	$3,90($2)
	ext	$3,$3,2,1
	sb	$3,85($2)
	sb	$0,73($2)
	sb	$0,77($2)
	sb	$0,78($2)
	sb	$0,79($2)
	sb	$0,80($2)
	j	$L28
	sb	$0,69($2)

$L25:
	lui	$2,%hi(g_4EB0)
	addiu	$2,$2,%lo(g_4EB0)
	ext	$5,$3,3,1
	sb	$5,74($2)
	ext	$5,$3,13,1
	sb	$5,75($2)
	ext	$5,$3,8,1
	sb	$5,76($2)
	ext	$5,$3,19,1
	sb	$5,78($2)
	ext	$5,$3,1,1
	sb	$5,83($2)
	li	$5,1			# 0x1
	sb	$5,84($2)
	ext	$6,$3,7,1
	sb	$6,85($2)
	sb	$5,81($2)
	andi	$4,$4,0xff
	addiu	$4,$4,-32
	sltu	$4,$4,2
	bne	$4,$0,$L29
	sb	$5,82($2)

	ext	$3,$3,21,1
	lui	$2,%hi(g_4EB0+80)
	j	$L30
	sb	$3,%lo(g_4EB0+80)($2)

$L29:
	lui	$2,%hi(g_4EB0+80)
	sb	$0,%lo(g_4EB0+80)($2)
$L30:
	li	$3,1			# 0x1
	lui	$2,%hi(g_4EB0+352)
	sw	$3,%lo(g_4EB0+352)($2)
	lui	$4,%hi(g_4E50)
	addiu	$4,$4,%lo(g_4E50)
	jal	sceSysconCmdExec
	move	$5,$0

	jal	_sceSysconGetPommelType
	nop

	slt	$2,$2,1280
	bne	$2,$0,$L31
	lui	$2,%hi(g_4EB0+77)

	lw	$3,0($sp)
	ext	$3,$3,18,1
	j	$L32
	sb	$3,%lo(g_4EB0+77)($2)

$L31:
	sb	$0,%lo(g_4EB0+77)($2)
$L32:
	lui	$2,%hi(g_4EB0)
	addiu	$2,$2,%lo(g_4EB0)
	lbu	$3,91($2)
	ext	$4,$3,4,1
	sb	$4,73($2)
	ext	$4,$3,1,1
	sb	$4,79($2)
	ext	$3,$3,2,1
	sb	$3,69($2)
$L28:
	move	$2,$0
	lw	$31,12($sp)
	lw	$16,8($sp)
	j	$31
	addiu	$sp,$sp,16

	.set	macro
	.set	reorder
	.end	_sceSysconInitPowerStatus
	.size	_sceSysconInitPowerStatus, .-_sceSysconInitPowerStatus
	.align	2
	.globl	sceSysconEnd
	.set	nomips16
	.ent	sceSysconEnd
	.type	sceSysconEnd, @function
sceSysconEnd:
	.frame	$sp,16,$31		# vars= 0, regs= 3/0, args= 0, gp= 0
	.mask	0x80030000,-4
	.fmask	0x00000000,0
	.set	noreorder
	.set	nomacro
	addiu	$sp,$sp,-16
	sw	$31,12($sp)
	sw	$17,8($sp)
	sw	$16,4($sp)
	lui	$2,%hi(g_4EB0)
	lw	$2,%lo(g_4EB0)($2)
	beq	$2,$0,$L38
	li	$17,65536			# 0x10000

	ori	$17,$17,0x86a0
	lui	$16,%hi(g_4EB0)
$L40:
	jal	sceKernelDelayThread
	move	$4,$17

	lw	$2,%lo(g_4EB0)($16)
	bne	$2,$0,$L40
	nop

$L38:
	lui	$4,%hi(g_SysconEv)
	jal	sceKernelUnregisterSysEventHandler
	addiu	$4,$4,%lo(g_SysconEv)

	li	$4,4			# 0x4
	jal	sceKernelDisableSubIntr
	li	$5,4			# 0x4

	li	$4,4			# 0x4
	jal	sceKernelReleaseSubIntrHandler
	li	$5,4			# 0x4

	move	$2,$0
	lw	$31,12($sp)
	lw	$17,8($sp)
	lw	$16,4($sp)
	j	$31
	addiu	$sp,$sp,16

	.set	macro
	.set	reorder
	.end	sceSysconEnd
	.size	sceSysconEnd, .-sceSysconEnd
	.align	2
	.globl	sceSysconResume
	.set	nomips16
	.ent	sceSysconResume
	.type	sceSysconResume, @function
sceSysconResume:
	.frame	$sp,16,$31		# vars= 0, regs= 3/0, args= 0, gp= 0
	.mask	0x80030000,-4
	.fmask	0x00000000,0
	.set	noreorder
	.set	nomacro
	addiu	$sp,$sp,-16
	sw	$31,12($sp)
	sw	$17,8($sp)
	sw	$16,4($sp)
	move	$16,$4
	move	$4,$0
	jal	sceSysregSpiClkSelect
	li	$5,1			# 0x1

	jal	sceSysregSpiClkEnable
	move	$4,$0

	jal	sceSysregSpiIoEnable
	move	$4,$0

	li	$2,-1101529088			# 0xffffffffbe580000
	li	$3,207			# 0xcf
	sw	$3,0($2)
	li	$3,4			# 0x4
	sw	$3,4($2)
	sw	$0,20($2)
	sw	$0,36($2)
	li	$4,3			# 0x3
	jal	sceGpioSetPortMode
	move	$5,$0

	li	$4,4			# 0x4
	jal	sceGpioSetPortMode
	li	$5,1			# 0x1

	li	$4,4			# 0x4
	jal	sceGpioSetIntrMode
	li	$5,3			# 0x3

	li	$4,4			# 0x4
	jal	sceKernelEnableSubIntr
	li	$5,4			# 0x4

	jal	sceKernelGetSystemTimeLow
	nop

	lui	$3,%hi(g_4EB0)
	addiu	$3,$3,%lo(g_4EB0)
	sw	$2,40($3)
	sb	$0,74($3)
	lw	$2,32($16)
	ext	$2,$2,21,1
	sb	$2,75($3)
	li	$2,1			# 0x1
	sb	$2,84($3)
	lw	$4,24($16)
	ext	$4,$4,1,1
	sb	$4,76($3)
	sb	$2,81($3)
	sb	$2,82($3)
	sb	$2,83($3)
	lw	$2,24($16)
	ext	$2,$2,2,1
	sb	$2,85($3)
	lw	$2,32($16)
	ext	$2,$2,15,1
	sb	$2,64($3)
	lw	$2,32($16)
	nor	$2,$0,$2
	ext	$2,$2,14,1
	sb	$2,65($3)
	lw	$2,32($16)
	nor	$2,$0,$2
	ext	$2,$2,14,1
	sb	$2,66($3)
	lw	$2,32($16)
	nor	$2,$0,$2
	ext	$2,$2,13,1
	sb	$2,67($3)
	lw	$2,32($16)
	ext	$2,$2,20,1
	sb	$2,68($3)
	lb	$2,70($3)
	slt	$2,$2,-1
	bne	$2,$0,$L43
	lui	$2,%hi(g_4EB0+70)

	lw	$3,32($16)
	ext	$3,$3,23,1
	sb	$3,%lo(g_4EB0+70)($2)
$L43:
	lui	$2,%hi(g_4EB0+71)
	lb	$2,%lo(g_4EB0+71)($2)
	slt	$2,$2,-1
	bne	$2,$0,$L44
	lui	$2,%hi(g_4EB0+71)

	lbu	$3,35($16)
	andi	$3,$3,0x1
	sb	$3,%lo(g_4EB0+71)($2)
$L44:
	lui	$2,%hi(g_4EB0+72)
	lb	$2,%lo(g_4EB0+72)($2)
	slt	$2,$2,-1
	bne	$2,$0,$L45
	lui	$2,%hi(g_4EB0+72)

	lw	$3,32($16)
	nor	$3,$0,$3
	ext	$3,$3,25,1
	sb	$3,%lo(g_4EB0+72)($2)
$L45:
	lbu	$17,64($16)
	lui	$2,%hi(g_4EB0+91)
	jal	_sceSysconGetPommelType
	sb	$17,%lo(g_4EB0+91)($2)

	slt	$2,$2,1280
	bne	$2,$0,$L46
	lui	$2,%hi(g_4EB0+77)

	lw	$3,24($16)
	ext	$3,$3,18,1
	j	$L47
	sb	$3,%lo(g_4EB0+77)($2)

$L46:
	sb	$0,%lo(g_4EB0+77)($2)
$L47:
	lui	$2,%hi(g_4EB0)
	addiu	$2,$2,%lo(g_4EB0)
	ext	$3,$17,1,1
	sb	$3,79($2)
	ext	$3,$17,2,1
	sb	$3,69($2)
	ext	$17,$17,4,1
	sb	$17,73($2)
	move	$2,$0
	lw	$31,12($sp)
	lw	$17,8($sp)
	lw	$16,4($sp)
	j	$31
	addiu	$sp,$sp,16

	.set	macro
	.set	reorder
	.end	sceSysconResume
	.size	sceSysconResume, .-sceSysconResume
	.align	2
	.globl	_sceSysconSysEventHandler
	.set	nomips16
	.ent	_sceSysconSysEventHandler
	.type	_sceSysconSysEventHandler, @function
_sceSysconSysEventHandler:
	.frame	$sp,8,$31		# vars= 0, regs= 1/0, args= 0, gp= 0
	.mask	0x80000000,-4
	.fmask	0x00000000,0
	.set	noreorder
	.set	nomacro
	addiu	$sp,$sp,-8
	li	$2,16399			# 0x400f
	beq	$4,$2,$L52
	sw	$31,4($sp)

	slt	$2,$4,16400
	beq	$2,$0,$L55
	li	$2,65536			# 0x10000

	li	$2,1026			# 0x402
	beql	$4,$2,$L50
	move	$4,$0

	li	$2,16392			# 0x4008
	bnel	$4,$2,$L61
	move	$2,$0

	j	$L59
	lui	$2,%hi(g_4EB0+376)

$L55:
	addiu	$2,$2,8
	beq	$4,$2,$L53
	nop

	li	$2,65536			# 0x10000
	addiu	$2,$2,15
	bnel	$4,$2,$L57
	move	$2,$0

	j	$L60
	lui	$2,%hi(g_4EB0+368)

$L50:
	jal	sceSysconCmdSync
	li	$5,1			# 0x1

	li	$3,-1			# 0xffffffffffffffff
	movz	$3,$0,$2
	j	$L49
	move	$2,$3

$L59:
	lw	$2,%lo(g_4EB0+376)($2)
	beq	$2,$0,$L62
	li	$4,4			# 0x4

	jal	sceSyscon_driver_765775EB
	move	$4,$0

	li	$4,4			# 0x4
$L62:
	jal	sceKernelDisableSubIntr
	li	$5,4			# 0x4

	j	$L49
	move	$2,$0

$L52:
	li	$3,1			# 0x1
	lui	$2,%hi(g_4EB0+368)
	sw	$3,%lo(g_4EB0+368)($2)
	j	$L49
	move	$2,$0

$L53:
	jal	sceSysconResume
	lw	$4,4($6)

	j	$L49
	move	$2,$0

$L60:
	sw	$0,%lo(g_4EB0+368)($2)
	move	$2,$0
$L57:
$L49:
$L61:
	lw	$31,4($sp)
	j	$31
	addiu	$sp,$sp,8

	.set	macro
	.set	reorder
	.end	_sceSysconSysEventHandler
	.size	_sceSysconSysEventHandler, .-_sceSysconSysEventHandler
	.align	2
	.globl	_sceSysconGpioIntr
	.set	nomips16
	.ent	_sceSysconGpioIntr
	.type	_sceSysconGpioIntr, @function
_sceSysconGpioIntr:
	.frame	$sp,48,$31		# vars= 8, regs= 10/0, args= 0, gp= 0
	.mask	0xc0ff0000,-4
	.fmask	0x00000000,0
	addiu	$sp,$sp,-48
	sw	$31,44($sp)
	sw	$fp,40($sp)
	sw	$23,36($sp)
	sw	$22,32($sp)
	sw	$21,28($sp)
	sw	$20,24($sp)
	sw	$19,20($sp)
	sw	$18,16($sp)
	sw	$17,12($sp)
	.set	noreorder
	.set	nomacro
	jal	sceKernelCpuSuspendIntr
	sw	$16,8($sp)
	.set	macro
	.set	reorder

	move	$23,$2
	lui	$2,%hi(g_4EB0)
	addiu	$3,$2,%lo(g_4EB0)
	li	$4,1			# 0x1
	sw	$4,44($3)
	lw	$16,%lo(g_4EB0)($2)
	sw	$0,%lo(g_4EB0)($2)
	lw	$2,4($16)
	andi	$2,$2,0x200
	bne	$2,$0,$L64
	.set	noreorder
	.set	nomacro
	jal	sceGpioPortClear
	li	$4,8			# 0x8
	.set	macro
	.set	reorder

$L64:
	jal	sceKernelGetSystemTimeLow
	move	$17,$2
	lui	$2,%hi(g_4EB0+40)
	sw	$17,%lo(g_4EB0+40)($2)
	.set	noreorder
	.set	nomacro
	jal	sceGpioAcquireIntr
	li	$4,4			# 0x4
	.set	macro
	.set	reorder

	.set	noreorder
	.set	nomacro
	jal	_sceSysconPacketEnd
	move	$4,$16
	.set	macro
	.set	reorder

	seb	$18,$2
	.set	noreorder
	.set	nomacro
	bltz	$18,$L65
	andi	$3,$18,0x00ff
	.set	macro
	.set	reorder

	lui	$2,%hi(g_4EB0)
	addiu	$2,$2,%lo(g_4EB0)
	sb	$3,90($2)
	lbu	$22,89($2)
	lw	$2,352($2)
	.set	noreorder
	.set	nomacro
	bnel	$2,$0,$L66
	andi	$18,$3,0xdf
	.set	macro
	.set	reorder

	.set	noreorder
	.set	nomacro
	j	$L147
	xor	$2,$18,$22
	.set	macro
	.set	reorder

$L65:
	lw	$3,4($16)
	li	$2,8388608			# 0x800000
	or	$2,$3,$2
	sw	$2,4($16)
	.set	noreorder
	.set	nomacro
	j	$L68
	move	$20,$0
	.set	macro
	.set	reorder

$L66:
	andi	$2,$22,0x20
	or	$18,$18,$2
	seb	$18,$18
	xor	$2,$18,$22
$L147:
	seb	$2,$2
	andi	$19,$2,0x00ff
	andi	$21,$18,0x00ff
	lui	$3,%hi(g_4EB0+89)
	.set	noreorder
	.set	nomacro
	bgez	$2,$L69
	sb	$21,%lo(g_4EB0+89)($3)
	.set	macro
	.set	reorder

	lui	$2,%hi(g_4EB0+212)
	lw	$2,%lo(g_4EB0+212)($2)
	.set	noreorder
	.set	nomacro
	beql	$2,$0,$L149
	lui	$2,%hi(g_4EB0+352)
	.set	macro
	.set	reorder

 #APP
 # 60 "../../include/common/registers.h" 1
	move $20, $gp
 # 0 "" 2
 #NO_APP
	lui	$3,%hi(g_4EB0+216)
	lw	$3,%lo(g_4EB0+216)($3)
 #APP
 # 61 "../../include/common/registers.h" 1
	move $gp, $3
 # 0 "" 2
 #NO_APP
	srl	$4,$18,31
	lui	$3,%hi(g_4EB0+220)
	.set	noreorder
	.set	nomacro
	jalr	$2
	lw	$5,%lo(g_4EB0+220)($3)
	.set	macro
	.set	reorder

 #APP
 # 61 "../../include/common/registers.h" 1
	move $gp, $20
 # 0 "" 2
 #NO_APP
$L69:
	lui	$2,%hi(g_4EB0+352)
$L149:
	lw	$2,%lo(g_4EB0+352)($2)
	.set	noreorder
	.set	nomacro
	bne	$2,$0,$L70
	lui	$2,%hi(g_4EB0+90)
	.set	macro
	.set	reorder

	or	$2,$19,$18
	andi	$2,$2,0x20
	.set	noreorder
	.set	nomacro
	beq	$2,$0,$L71
	move	$20,$0
	.set	macro
	.set	reorder

	lui	$2,%hi(g_4EB0+92)
	lw	$2,%lo(g_4EB0+92)($2)
	.set	noreorder
	.set	nomacro
	beql	$2,$0,$L150
	andi	$2,$19,0x10
	.set	macro
	.set	reorder

 #APP
 # 60 "../../include/common/registers.h" 1
	move $20, $gp
 # 0 "" 2
 #NO_APP
	lui	$3,%hi(g_4EB0+96)
	lw	$3,%lo(g_4EB0+96)($3)
 #APP
 # 61 "../../include/common/registers.h" 1
	move $gp, $3
 # 0 "" 2
 #NO_APP
	ext	$4,$18,5,1
	lui	$3,%hi(g_4EB0+100)
	.set	noreorder
	.set	nomacro
	jalr	$2
	lw	$5,%lo(g_4EB0+100)($3)
	.set	macro
	.set	reorder

 #APP
 # 61 "../../include/common/registers.h" 1
	move $gp, $20
 # 0 "" 2
 #NO_APP
	.set	noreorder
	.set	nomacro
	j	$L71
	move	$20,$0
	.set	macro
	.set	reorder

$L70:
	lbu	$2,%lo(g_4EB0+90)($2)
	andi	$2,$2,0x20
	.set	noreorder
	.set	nomacro
	beq	$2,$0,$L71
	move	$20,$0
	.set	macro
	.set	reorder

	lui	$2,%hi(g_4EB0+368)
	lw	$2,%lo(g_4EB0+368)($2)
	.set	noreorder
	.set	nomacro
	bne	$2,$0,$L150
	andi	$2,$19,0x10
	.set	macro
	.set	reorder

	lbu	$20,12($16)
	xori	$20,$20,0x10
	sltu	$20,$0,$20
$L71:
	andi	$2,$19,0x10
$L150:
	.set	noreorder
	.set	nomacro
	beq	$2,$0,$L72
	sw	$2,4($sp)
	.set	macro
	.set	reorder

	lui	$2,%hi(g_4EB0+104)
	lw	$2,%lo(g_4EB0+104)($2)
	.set	noreorder
	.set	nomacro
	beq	$2,$0,$L151
	andi	$fp,$19,0x8
	.set	macro
	.set	reorder

 #APP
 # 60 "../../include/common/registers.h" 1
	move $fp, $gp
 # 0 "" 2
 #NO_APP
	lui	$3,%hi(g_4EB0+108)
	lw	$3,%lo(g_4EB0+108)($3)
 #APP
 # 61 "../../include/common/registers.h" 1
	move $gp, $3
 # 0 "" 2
 #NO_APP
	ext	$4,$18,4,1
	lui	$3,%hi(g_4EB0+112)
	.set	noreorder
	.set	nomacro
	jalr	$2
	lw	$5,%lo(g_4EB0+112)($3)
	.set	macro
	.set	reorder

 #APP
 # 61 "../../include/common/registers.h" 1
	move $gp, $fp
 # 0 "" 2
 #NO_APP
$L72:
	andi	$fp,$19,0x8
$L151:
	.set	noreorder
	.set	nomacro
	beq	$fp,$0,$L152
	andi	$2,$19,0x1
	.set	macro
	.set	reorder

	lui	$2,%hi(g_4EB0+116)
	lw	$2,%lo(g_4EB0+116)($2)
	.set	noreorder
	.set	nomacro
	beql	$2,$0,$L152
	andi	$2,$19,0x1
	.set	macro
	.set	reorder

 #APP
 # 60 "../../include/common/registers.h" 1
	move $3, $gp
 # 0 "" 2
 #NO_APP
	sw	$3,0($sp)
	lui	$3,%hi(g_4EB0+120)
	lw	$3,%lo(g_4EB0+120)($3)
 #APP
 # 61 "../../include/common/registers.h" 1
	move $gp, $3
 # 0 "" 2
 #NO_APP
	xori	$4,$18,0x8
	ext	$4,$4,3,1
	lui	$3,%hi(g_4EB0+124)
	.set	noreorder
	.set	nomacro
	jalr	$2
	lw	$5,%lo(g_4EB0+124)($3)
	.set	macro
	.set	reorder

	lw	$2,0($sp)
 #APP
 # 61 "../../include/common/registers.h" 1
	move $gp, $2
 # 0 "" 2
 #NO_APP
	andi	$2,$19,0x1
$L152:
	.set	noreorder
	.set	nomacro
	beq	$2,$0,$L153
	andi	$2,$19,0x2
	.set	macro
	.set	reorder

	lui	$2,%hi(g_4EB0+128)
	lw	$2,%lo(g_4EB0+128)($2)
	.set	noreorder
	.set	nomacro
	beql	$2,$0,$L154
	lui	$2,%hi(g_4EB0+272)
	.set	macro
	.set	reorder

 #APP
 # 60 "../../include/common/registers.h" 1
	move $3, $gp
 # 0 "" 2
 #NO_APP
	sw	$3,0($sp)
	lui	$3,%hi(g_4EB0+132)
	lw	$3,%lo(g_4EB0+132)($3)
 #APP
 # 61 "../../include/common/registers.h" 1
	move $gp, $3
 # 0 "" 2
 #NO_APP
	andi	$4,$21,0x1
	lui	$3,%hi(g_4EB0+136)
	.set	noreorder
	.set	nomacro
	jalr	$2
	lw	$5,%lo(g_4EB0+136)($3)
	.set	macro
	.set	reorder

	lw	$2,0($sp)
 #APP
 # 61 "../../include/common/registers.h" 1
	move $gp, $2
 # 0 "" 2
 #NO_APP
	lui	$2,%hi(g_4EB0+272)
$L154:
	lw	$2,%lo(g_4EB0+272)($2)
	.set	noreorder
	.set	nomacro
	beql	$2,$0,$L153
	andi	$2,$19,0x2
	.set	macro
	.set	reorder

 #APP
 # 60 "../../include/common/registers.h" 1
	move $3, $gp
 # 0 "" 2
 #NO_APP
	sw	$3,0($sp)
	lui	$3,%hi(g_4EB0+276)
	lw	$3,%lo(g_4EB0+276)($3)
 #APP
 # 61 "../../include/common/registers.h" 1
	move $gp, $3
 # 0 "" 2
 #NO_APP
	andi	$4,$21,0x1
	lui	$3,%hi(g_4EB0+280)
	.set	noreorder
	.set	nomacro
	jalr	$2
	lw	$5,%lo(g_4EB0+280)($3)
	.set	macro
	.set	reorder

	lw	$2,0($sp)
 #APP
 # 61 "../../include/common/registers.h" 1
	move $gp, $2
 # 0 "" 2
 #NO_APP
	andi	$2,$19,0x2
$L153:
	.set	noreorder
	.set	nomacro
	beql	$2,$0,$L155
	andi	$19,$19,0x4
	.set	macro
	.set	reorder

	lui	$2,%hi(g_4EB0+200)
	lw	$2,%lo(g_4EB0+200)($2)
	.set	noreorder
	.set	nomacro
	beql	$2,$0,$L155
	andi	$19,$19,0x4
	.set	macro
	.set	reorder

 #APP
 # 60 "../../include/common/registers.h" 1
	move $3, $gp
 # 0 "" 2
 #NO_APP
	sw	$3,0($sp)
	lui	$3,%hi(g_4EB0+204)
	lw	$3,%lo(g_4EB0+204)($3)
 #APP
 # 61 "../../include/common/registers.h" 1
	move $gp, $3
 # 0 "" 2
 #NO_APP
	ext	$4,$18,1,1
	lui	$3,%hi(g_4EB0+208)
	.set	noreorder
	.set	nomacro
	jalr	$2
	lw	$5,%lo(g_4EB0+208)($3)
	.set	macro
	.set	reorder

	lw	$2,0($sp)
 #APP
 # 61 "../../include/common/registers.h" 1
	move $gp, $2
 # 0 "" 2
 #NO_APP
	andi	$19,$19,0x4
$L155:
	.set	noreorder
	.set	nomacro
	beql	$19,$0,$L156
	lbu	$2,12($16)
	.set	macro
	.set	reorder

	lui	$2,%hi(g_4EB0+188)
	lw	$2,%lo(g_4EB0+188)($2)
	.set	noreorder
	.set	nomacro
	beql	$2,$0,$L156
	lbu	$2,12($16)
	.set	macro
	.set	reorder

 #APP
 # 60 "../../include/common/registers.h" 1
	move $3, $gp
 # 0 "" 2
 #NO_APP
	sw	$3,0($sp)
	lui	$3,%hi(g_4EB0+192)
	lw	$3,%lo(g_4EB0+192)($3)
 #APP
 # 61 "../../include/common/registers.h" 1
	move $gp, $3
 # 0 "" 2
 #NO_APP
	ext	$4,$18,2,1
	lui	$3,%hi(g_4EB0+196)
	.set	noreorder
	.set	nomacro
	jalr	$2
	lw	$5,%lo(g_4EB0+196)($3)
	.set	macro
	.set	reorder

	lw	$2,0($sp)
 #APP
 # 61 "../../include/common/registers.h" 1
	move $gp, $2
 # 0 "" 2
 #NO_APP
	lbu	$2,12($16)
$L156:
	sltu	$2,$2,17
	.set	noreorder
	.set	nomacro
	beql	$2,$0,$L148
	lbu	$2,12($16)
	.set	macro
	.set	reorder

	lbu	$3,12($16)
	sll	$3,$3,2
	lui	$2,%hi($L81)
	addiu	$2,$2,%lo($L81)
	addu	$2,$2,$3
	lw	$2,0($2)
	j	$2
	.rdata
	.align	2
	.align	2
$L81:
	.word	$L68
	.word	$L68
	.word	$L78
	.word	$L68
	.word	$L68
	.word	$L68
	.word	$L78
	.word	$L78
	.word	$L78
	.word	$L68
	.word	$L68
	.word	$L79
	.word	$L68
	.word	$L68
	.word	$L68
	.word	$L68
	.word	$L80
	.text
$L80:
	lui	$2,%hi(g_4EB0+91)
	sb	$21,%lo(g_4EB0+91)($2)
	lbu	$2,31($16)
	xor	$2,$21,$2
	andi	$2,$2,0x2
	.set	noreorder
	.set	nomacro
	beq	$2,$0,$L82
	lui	$2,%hi(g_4EB0+236)
	.set	macro
	.set	reorder

	lw	$2,%lo(g_4EB0+236)($2)
	beq	$2,$0,$L82
 #APP
 # 60 "../../include/common/registers.h" 1
	move $22, $gp
 # 0 "" 2
 #NO_APP
	lui	$3,%hi(g_4EB0+240)
	lw	$3,%lo(g_4EB0+240)($3)
 #APP
 # 61 "../../include/common/registers.h" 1
	move $gp, $3
 # 0 "" 2
 #NO_APP
	ext	$4,$18,1,1
	lui	$3,%hi(g_4EB0+244)
	.set	noreorder
	.set	nomacro
	jalr	$2
	lw	$5,%lo(g_4EB0+244)($3)
	.set	macro
	.set	reorder

 #APP
 # 61 "../../include/common/registers.h" 1
	move $gp, $22
 # 0 "" 2
 #NO_APP
$L82:
	beq	$19,$0,$L83
 #APP
 # 60 "../../include/common/registers.h" 1
	move $19, $gp
 # 0 "" 2
 #NO_APP
	lui	$2,%hi(g_4EB0+264)
	lw	$2,%lo(g_4EB0+264)($2)
 #APP
 # 61 "../../include/common/registers.h" 1
	move $gp, $2
 # 0 "" 2
 #NO_APP
	lui	$3,%hi(g_4EB0)
	addiu	$3,$3,%lo(g_4EB0)
	lw	$2,260($3)
	ext	$4,$18,2,1
	.set	noreorder
	.set	nomacro
	jalr	$2
	lw	$5,268($3)
	.set	macro
	.set	reorder

 #APP
 # 61 "../../include/common/registers.h" 1
	move $gp, $19
 # 0 "" 2
 #NO_APP
$L83:
	.set	noreorder
	.set	nomacro
	beq	$fp,$0,$L157
	lw	$3,4($sp)
	.set	macro
	.set	reorder

	lui	$2,%hi(g_4EB0+89)
	lbu	$3,%lo(g_4EB0+89)($2)
	andi	$3,$3,0x20
	ext	$4,$21,3,1
	li	$2,1			# 0x1
	movn	$4,$2,$3
	lui	$2,%hi(g_4EB0+92)
	lw	$2,%lo(g_4EB0+92)($2)
	.set	noreorder
	.set	nomacro
	beq	$2,$0,$L157
	lw	$3,4($sp)
	.set	macro
	.set	reorder

 #APP
 # 60 "../../include/common/registers.h" 1
	move $19, $gp
 # 0 "" 2
 #NO_APP
	lui	$3,%hi(g_4EB0+96)
	lw	$3,%lo(g_4EB0+96)($3)
 #APP
 # 61 "../../include/common/registers.h" 1
	move $gp, $3
 # 0 "" 2
 #NO_APP
	lui	$3,%hi(g_4EB0+100)
	.set	noreorder
	.set	nomacro
	jalr	$2
	lw	$5,%lo(g_4EB0+100)($3)
	.set	macro
	.set	reorder

 #APP
 # 61 "../../include/common/registers.h" 1
	move $gp, $19
 # 0 "" 2
 #NO_APP
	lw	$3,4($sp)
$L157:
	.set	noreorder
	.set	nomacro
	beql	$3,$0,$L148
	lbu	$2,12($16)
	.set	macro
	.set	reorder

	lui	$2,%hi(g_4EB0+320)
	lw	$2,%lo(g_4EB0+320)($2)
	.set	noreorder
	.set	nomacro
	beql	$2,$0,$L148
	lbu	$2,12($16)
	.set	macro
	.set	reorder

 #APP
 # 60 "../../include/common/registers.h" 1
	move $19, $gp
 # 0 "" 2
 #NO_APP
	lui	$3,%hi(g_4EB0+324)
	lw	$3,%lo(g_4EB0+324)($3)
 #APP
 # 61 "../../include/common/registers.h" 1
	move $gp, $3
 # 0 "" 2
 #NO_APP
	ext	$4,$18,4,1
	lui	$3,%hi(g_4EB0+328)
	.set	noreorder
	.set	nomacro
	jalr	$2
	lw	$5,%lo(g_4EB0+328)($3)
	.set	macro
	.set	reorder

 #APP
 # 61 "../../include/common/registers.h" 1
	move $gp, $19
 # 0 "" 2
 #NO_APP
	.set	noreorder
	.set	nomacro
	j	$L148
	lbu	$2,12($16)
	.set	macro
	.set	reorder

$L78:
	lui	$2,%hi(g_4EB0)
	addiu	$2,$2,%lo(g_4EB0)
	lb	$3,64($2)
	lbu	$4,32($16)
	srl	$4,$4,7
	sb	$4,64($2)
	li	$2,-1			# 0xffffffffffffffff
	.set	noreorder
	.set	nomacro
	beq	$3,$2,$L158
	lui	$2,%hi(g_4EB0+140)
	.set	macro
	.set	reorder

	.set	noreorder
	.set	nomacro
	beql	$3,$4,$L159
	lui	$2,%hi(g_4EB0)
	.set	macro
	.set	reorder

$L158:
	lw	$2,%lo(g_4EB0+140)($2)
	.set	noreorder
	.set	nomacro
	beql	$2,$0,$L159
	lui	$2,%hi(g_4EB0)
	.set	macro
	.set	reorder

 #APP
 # 60 "../../include/common/registers.h" 1
	move $18, $gp
 # 0 "" 2
 #NO_APP
	lui	$3,%hi(g_4EB0+144)
	lw	$3,%lo(g_4EB0+144)($3)
 #APP
 # 61 "../../include/common/registers.h" 1
	move $gp, $3
 # 0 "" 2
 #NO_APP
	lui	$3,%hi(g_4EB0+148)
	.set	noreorder
	.set	nomacro
	jalr	$2
	lw	$5,%lo(g_4EB0+148)($3)
	.set	macro
	.set	reorder

 #APP
 # 61 "../../include/common/registers.h" 1
	move $gp, $18
 # 0 "" 2
 #NO_APP
	lui	$2,%hi(g_4EB0)
$L159:
	addiu	$2,$2,%lo(g_4EB0)
	lb	$3,65($2)
	lbu	$5,32($16)
	lb	$2,87($2)
	.set	noreorder
	.set	nomacro
	bgez	$2,$L89
	andi	$4,$2,0x00ff
	.set	macro
	.set	reorder

	xori	$4,$5,0x40
	ext	$4,$4,6,1
$L89:
	lui	$2,%hi(g_4EB0+65)
	sb	$4,%lo(g_4EB0+65)($2)
	li	$2,-1			# 0xffffffffffffffff
	.set	noreorder
	.set	nomacro
	beq	$3,$2,$L160
	lui	$2,%hi(g_4EB0+152)
	.set	macro
	.set	reorder

	.set	noreorder
	.set	nomacro
	beql	$3,$4,$L161
	lui	$2,%hi(g_4EB0)
	.set	macro
	.set	reorder

$L160:
	lw	$2,%lo(g_4EB0+152)($2)
	.set	noreorder
	.set	nomacro
	beql	$2,$0,$L161
	lui	$2,%hi(g_4EB0)
	.set	macro
	.set	reorder

 #APP
 # 60 "../../include/common/registers.h" 1
	move $18, $gp
 # 0 "" 2
 #NO_APP
	lui	$3,%hi(g_4EB0+156)
	lw	$3,%lo(g_4EB0+156)($3)
 #APP
 # 61 "../../include/common/registers.h" 1
	move $gp, $3
 # 0 "" 2
 #NO_APP
	lui	$3,%hi(g_4EB0+160)
	.set	noreorder
	.set	nomacro
	jalr	$2
	lw	$5,%lo(g_4EB0+160)($3)
	.set	macro
	.set	reorder

 #APP
 # 61 "../../include/common/registers.h" 1
	move $gp, $18
 # 0 "" 2
 #NO_APP
	lui	$2,%hi(g_4EB0)
$L161:
	addiu	$2,$2,%lo(g_4EB0)
	lb	$3,66($2)
	lbu	$5,32($16)
	lb	$2,88($2)
	.set	noreorder
	.set	nomacro
	bgez	$2,$L93
	andi	$4,$2,0x00ff
	.set	macro
	.set	reorder

	xori	$4,$5,0x40
	ext	$4,$4,6,1
$L93:
	lui	$2,%hi(g_4EB0+66)
	sb	$4,%lo(g_4EB0+66)($2)
	li	$2,-1			# 0xffffffffffffffff
	.set	noreorder
	.set	nomacro
	beq	$3,$2,$L162
	lui	$2,%hi(g_4EB0+248)
	.set	macro
	.set	reorder

	.set	noreorder
	.set	nomacro
	beql	$3,$4,$L163
	lbu	$4,32($16)
	.set	macro
	.set	reorder

$L162:
	lw	$2,%lo(g_4EB0+248)($2)
	.set	noreorder
	.set	nomacro
	beql	$2,$0,$L163
	lbu	$4,32($16)
	.set	macro
	.set	reorder

 #APP
 # 60 "../../include/common/registers.h" 1
	move $18, $gp
 # 0 "" 2
 #NO_APP
	lui	$3,%hi(g_4EB0+252)
	lw	$3,%lo(g_4EB0+252)($3)
 #APP
 # 61 "../../include/common/registers.h" 1
	move $gp, $3
 # 0 "" 2
 #NO_APP
	lui	$3,%hi(g_4EB0+256)
	.set	noreorder
	.set	nomacro
	jalr	$2
	lw	$5,%lo(g_4EB0+256)($3)
	.set	macro
	.set	reorder

 #APP
 # 61 "../../include/common/registers.h" 1
	move $gp, $18
 # 0 "" 2
 #NO_APP
	lbu	$4,32($16)
$L163:
	lui	$2,%hi(g_4EB0)
	addiu	$2,$2,%lo(g_4EB0)
	lb	$3,67($2)
	sb	$4,67($2)
	li	$2,-1			# 0xffffffffffffffff
	.set	noreorder
	.set	nomacro
	beq	$3,$2,$L96
	xori	$2,$4,0x20
	.set	macro
	.set	reorder

	ext	$2,$2,5,1
	.set	noreorder
	.set	nomacro
	beql	$3,$2,$L164
	lbu	$2,12($16)
	.set	macro
	.set	reorder

$L96:
	lui	$2,%hi(g_4EB0+164)
	lw	$2,%lo(g_4EB0+164)($2)
	.set	noreorder
	.set	nomacro
	beql	$2,$0,$L164
	lbu	$2,12($16)
	.set	macro
	.set	reorder

 #APP
 # 60 "../../include/common/registers.h" 1
	move $18, $gp
 # 0 "" 2
 #NO_APP
	lui	$3,%hi(g_4EB0+168)
	lw	$3,%lo(g_4EB0+168)($3)
 #APP
 # 61 "../../include/common/registers.h" 1
	move $gp, $3
 # 0 "" 2
 #NO_APP
	lui	$3,%hi(g_4EB0+172)
	.set	noreorder
	.set	nomacro
	jalr	$2
	lw	$5,%lo(g_4EB0+172)($3)
	.set	macro
	.set	reorder

 #APP
 # 61 "../../include/common/registers.h" 1
	move $gp, $18
 # 0 "" 2
 #NO_APP
	lbu	$2,12($16)
$L164:
	addiu	$2,$2,-7
	andi	$2,$2,0x00ff
	sltu	$2,$2,2
	.set	noreorder
	.set	nomacro
	beql	$2,$0,$L148
	lbu	$2,12($16)
	.set	macro
	.set	reorder

	lui	$2,%hi(g_4EB0)
	addiu	$2,$2,%lo(g_4EB0)
	lb	$3,68($2)
	lbu	$4,33($16)
	ext	$4,$4,4,1
	sb	$4,68($2)
	li	$2,-1			# 0xffffffffffffffff
	.set	noreorder
	.set	nomacro
	beq	$3,$2,$L165
	lui	$2,%hi(g_4EB0+176)
	.set	macro
	.set	reorder

	.set	noreorder
	.set	nomacro
	beql	$3,$4,$L166
	lui	$2,%hi(g_4EB0+70)
	.set	macro
	.set	reorder

$L165:
	lw	$2,%lo(g_4EB0+176)($2)
	.set	noreorder
	.set	nomacro
	beql	$2,$0,$L166
	lui	$2,%hi(g_4EB0+70)
	.set	macro
	.set	reorder

 #APP
 # 60 "../../include/common/registers.h" 1
	move $18, $gp
 # 0 "" 2
 #NO_APP
	lui	$3,%hi(g_4EB0+180)
	lw	$3,%lo(g_4EB0+180)($3)
 #APP
 # 61 "../../include/common/registers.h" 1
	move $gp, $3
 # 0 "" 2
 #NO_APP
	lui	$3,%hi(g_4EB0+184)
	.set	noreorder
	.set	nomacro
	jalr	$2
	lw	$5,%lo(g_4EB0+184)($3)
	.set	macro
	.set	reorder

 #APP
 # 61 "../../include/common/registers.h" 1
	move $gp, $18
 # 0 "" 2
 #NO_APP
	lui	$2,%hi(g_4EB0+70)
$L166:
	lb	$2,%lo(g_4EB0+70)($2)
	slt	$3,$2,-1
	.set	noreorder
	.set	nomacro
	bne	$3,$0,$L100
	lui	$3,%hi(g_4EB0+70)
	.set	macro
	.set	reorder

	lbu	$4,33($16)
	srl	$4,$4,7
	sb	$4,%lo(g_4EB0+70)($3)
	li	$3,-1			# 0xffffffffffffffff
	.set	noreorder
	.set	nomacro
	beql	$2,$3,$L167
	lui	$2,%hi(g_4EB0+284)
	.set	macro
	.set	reorder

	.set	noreorder
	.set	nomacro
	beq	$2,$4,$L100
	lui	$2,%hi(g_4EB0+284)
	.set	macro
	.set	reorder

$L167:
	lw	$2,%lo(g_4EB0+284)($2)
	.set	noreorder
	.set	nomacro
	beql	$2,$0,$L168
	lui	$2,%hi(g_4EB0+71)
	.set	macro
	.set	reorder

 #APP
 # 60 "../../include/common/registers.h" 1
	move $18, $gp
 # 0 "" 2
 #NO_APP
	lui	$3,%hi(g_4EB0+288)
	lw	$3,%lo(g_4EB0+288)($3)
 #APP
 # 61 "../../include/common/registers.h" 1
	move $gp, $3
 # 0 "" 2
 #NO_APP
	lui	$3,%hi(g_4EB0+292)
	.set	noreorder
	.set	nomacro
	jalr	$2
	lw	$5,%lo(g_4EB0+292)($3)
	.set	macro
	.set	reorder

 #APP
 # 61 "../../include/common/registers.h" 1
	move $gp, $18
 # 0 "" 2
 #NO_APP
$L100:
	lui	$2,%hi(g_4EB0+71)
$L168:
	lb	$2,%lo(g_4EB0+71)($2)
	slt	$3,$2,-1
	.set	noreorder
	.set	nomacro
	bne	$3,$0,$L102
	lui	$3,%hi(g_4EB0+71)
	.set	macro
	.set	reorder

	lbu	$4,34($16)
	andi	$4,$4,0x1
	sb	$4,%lo(g_4EB0+71)($3)
	li	$3,-1			# 0xffffffffffffffff
	.set	noreorder
	.set	nomacro
	beql	$2,$3,$L169
	lui	$2,%hi(g_4EB0+296)
	.set	macro
	.set	reorder

	.set	noreorder
	.set	nomacro
	beq	$2,$4,$L102
	lui	$2,%hi(g_4EB0+296)
	.set	macro
	.set	reorder

$L169:
	lw	$2,%lo(g_4EB0+296)($2)
	.set	noreorder
	.set	nomacro
	beql	$2,$0,$L170
	lui	$2,%hi(g_4EB0+72)
	.set	macro
	.set	reorder

 #APP
 # 60 "../../include/common/registers.h" 1
	move $18, $gp
 # 0 "" 2
 #NO_APP
	lui	$3,%hi(g_4EB0+300)
	lw	$3,%lo(g_4EB0+300)($3)
 #APP
 # 61 "../../include/common/registers.h" 1
	move $gp, $3
 # 0 "" 2
 #NO_APP
	lui	$3,%hi(g_4EB0+304)
	.set	noreorder
	.set	nomacro
	jalr	$2
	lw	$5,%lo(g_4EB0+304)($3)
	.set	macro
	.set	reorder

 #APP
 # 61 "../../include/common/registers.h" 1
	move $gp, $18
 # 0 "" 2
 #NO_APP
$L102:
	lui	$2,%hi(g_4EB0+72)
$L170:
	lb	$2,%lo(g_4EB0+72)($2)
	slt	$3,$2,-1
	.set	noreorder
	.set	nomacro
	bne	$3,$0,$L68
	lui	$3,%hi(g_4EB0+72)
	.set	macro
	.set	reorder

	lbu	$4,34($16)
	xori	$4,$4,0x2
	ext	$4,$4,1,1
	sb	$4,%lo(g_4EB0+72)($3)
	li	$3,-1			# 0xffffffffffffffff
	.set	noreorder
	.set	nomacro
	beql	$2,$3,$L171
	lui	$2,%hi(g_4EB0+308)
	.set	macro
	.set	reorder

	.set	noreorder
	.set	nomacro
	beq	$2,$4,$L68
	lui	$2,%hi(g_4EB0+308)
	.set	macro
	.set	reorder

$L171:
	lw	$2,%lo(g_4EB0+308)($2)
	.set	noreorder
	.set	nomacro
	beql	$2,$0,$L148
	lbu	$2,12($16)
	.set	macro
	.set	reorder

 #APP
 # 60 "../../include/common/registers.h" 1
	move $18, $gp
 # 0 "" 2
 #NO_APP
	lui	$3,%hi(g_4EB0+312)
	lw	$3,%lo(g_4EB0+312)($3)
 #APP
 # 61 "../../include/common/registers.h" 1
	move $gp, $3
 # 0 "" 2
 #NO_APP
	lui	$3,%hi(g_4EB0+316)
	.set	noreorder
	.set	nomacro
	jalr	$2
	lw	$5,%lo(g_4EB0+316)($3)
	.set	macro
	.set	reorder

 #APP
 # 61 "../../include/common/registers.h" 1
	move $gp, $18
 # 0 "" 2
 #NO_APP
	.set	noreorder
	.set	nomacro
	j	$L148
	lbu	$2,12($16)
	.set	macro
	.set	reorder

$L79:
	lui	$2,%hi(g_4EB0+352)
	lw	$2,%lo(g_4EB0+352)($2)
	.set	noreorder
	.set	nomacro
	beql	$2,$0,$L148
	lbu	$2,12($16)
	.set	macro
	.set	reorder

	lbu	$2,31($16)
	andi	$2,$2,0x10
	.set	noreorder
	.set	nomacro
	bne	$2,$0,$L105
	lui	$2,%hi(g_4EB0)
	.set	macro
	.set	reorder

	addiu	$2,$2,%lo(g_4EB0)
	lbu	$3,89($2)
	andi	$3,$3,0xdf
	.set	noreorder
	.set	nomacro
	j	$L106
	sb	$3,89($2)
	.set	macro
	.set	reorder

$L105:
	addiu	$2,$2,%lo(g_4EB0)
	lbu	$3,89($2)
	ori	$3,$3,0x20
	sb	$3,89($2)
$L106:
	lui	$2,%hi(g_4EB0+89)
	lbu	$2,%lo(g_4EB0+89)($2)
	xor	$3,$22,$2
	or	$3,$2,$3
	andi	$3,$3,0x20
	.set	noreorder
	.set	nomacro
	beql	$3,$0,$L148
	lbu	$2,12($16)
	.set	macro
	.set	reorder

	andi	$2,$2,0x20
	.set	noreorder
	.set	nomacro
	bne	$2,$0,$L107
	li	$4,1			# 0x1
	.set	macro
	.set	reorder

	lui	$2,%hi(g_4EB0+91)
	lbu	$4,%lo(g_4EB0+91)($2)
	ext	$4,$4,3,1
$L107:
	lui	$2,%hi(g_4EB0+92)
	lw	$2,%lo(g_4EB0+92)($2)
	.set	noreorder
	.set	nomacro
	beql	$2,$0,$L148
	lbu	$2,12($16)
	.set	macro
	.set	reorder

 #APP
 # 60 "../../include/common/registers.h" 1
	move $18, $gp
 # 0 "" 2
 #NO_APP
	lui	$3,%hi(g_4EB0+96)
	lw	$3,%lo(g_4EB0+96)($3)
 #APP
 # 61 "../../include/common/registers.h" 1
	move $gp, $3
 # 0 "" 2
 #NO_APP
	lui	$3,%hi(g_4EB0+100)
	.set	noreorder
	.set	nomacro
	jalr	$2
	lw	$5,%lo(g_4EB0+100)($3)
	.set	macro
	.set	reorder

 #APP
 # 61 "../../include/common/registers.h" 1
	move $gp, $18
 # 0 "" 2
 #NO_APP
$L68:
	lbu	$2,12($16)
$L148:
	sltu	$3,$2,32
	.set	noreorder
	.set	nomacro
	bne	$3,$0,$L172
	seb	$4,$2
	.set	macro
	.set	reorder

	lbu	$3,30($16)
	seb	$4,$3
	.set	noreorder
	.set	nomacro
	bgez	$4,$L172
	seb	$4,$2
	.set	macro
	.set	reorder

	sltu	$3,$3,130
	.set	noreorder
	.set	nomacro
	beql	$3,$0,$L173
	srl	$2,$2,5
	.set	macro
	.set	reorder

	lw	$3,4($16)
	andi	$4,$3,0x10
	.set	noreorder
	.set	nomacro
	bne	$4,$0,$L108
	seb	$4,$2
	.set	macro
	.set	reorder

	li	$2,262144			# 0x40000
	or	$3,$3,$2
	sw	$3,4($16)
	li	$21,-1			# 0xffffffffffffffff
	.set	noreorder
	.set	nomacro
	j	$L109
	li	$18,2			# 0x2
	.set	macro
	.set	reorder

$L108:
$L172:
	srl	$2,$2,5
$L173:
	slt	$3,$4,0
	movn	$2,$0,$3
	move	$3,$2
	lw	$4,0($16)
	sll	$5,$2,2
	lui	$2,%hi(g_4EB0)
	addiu	$2,$2,%lo(g_4EB0)
	addu	$2,$5,$2
	sw	$4,4($2)
	lw	$4,4($16)
	li	$2,-327680			# 0xfffffffffffb0000
	ori	$2,$2,0xffff
	and	$2,$4,$2
	sw	$2,4($16)
	lw	$2,0($16)
	.set	noreorder
	.set	nomacro
	bnel	$2,$0,$L174
	lw	$3,4($16)
	.set	macro
	.set	reorder

	addiu	$3,$3,4
	sll	$3,$3,2
	lui	$2,%hi(g_4EB0)
	addiu	$2,$2,%lo(g_4EB0)
	addu	$2,$3,$2
	sw	$0,4($2)
	lw	$3,4($16)
$L174:
	li	$2,-655360			# 0xfffffffffff60000
	ori	$2,$2,0xffff
	and	$2,$3,$2
	li	$3,524288			# 0x80000
	or	$2,$2,$3
	sw	$2,4($16)
	sw	$0,0($16)
	lw	$21,8($16)
	lw	$2,44($16)
	.set	noreorder
	.set	nomacro
	beq	$2,$0,$L109
	move	$18,$0
	.set	macro
	.set	reorder

 #APP
 # 60 "../../include/common/registers.h" 1
	move $19, $gp
 # 0 "" 2
 #NO_APP
	lw	$3,48($16)
 #APP
 # 61 "../../include/common/registers.h" 1
	move $gp, $3
 # 0 "" 2
 #NO_APP
	move	$4,$16
	.set	noreorder
	.set	nomacro
	jalr	$2
	lw	$5,52($16)
	.set	macro
	.set	reorder

	move	$18,$2
 #APP
 # 61 "../../include/common/registers.h" 1
	move $gp, $19
 # 0 "" 2
 #NO_APP
	.set	noreorder
	.set	nomacro
	beq	$2,$0,$L109
	li	$2,1			# 0x1
	.set	macro
	.set	reorder

	.set	noreorder
	.set	nomacro
	bne	$18,$2,$L112
	li	$2,2			# 0x2
	.set	macro
	.set	reorder

	lbu	$3,12($16)
	seb	$4,$3
	srl	$3,$3,5
	slt	$2,$4,0
	movn	$3,$0,$2
	move	$2,$3
	sll	$4,$3,2
	lui	$3,%hi(g_4EB0)
	addiu	$3,$3,%lo(g_4EB0)
	addu	$3,$4,$3
	lw	$3,4($3)
	.set	noreorder
	.set	nomacro
	bnel	$3,$0,$L114
	sll	$2,$2,2
	.set	macro
	.set	reorder

	addiu	$4,$2,4
	sll	$4,$4,2
	lui	$3,%hi(g_4EB0)
	addiu	$3,$3,%lo(g_4EB0)
	addu	$4,$4,$3
	sw	$16,4($4)
	sll	$2,$2,2
	addu	$3,$2,$3
	sw	$16,4($3)
	.set	noreorder
	.set	nomacro
	j	$L115
	sw	$0,0($16)
	.set	macro
	.set	reorder

$L114:
	lui	$4,%hi(g_4EB0)
	addiu	$4,$4,%lo(g_4EB0)
	addu	$2,$2,$4
	sw	$16,4($2)
	sw	$3,0($16)
$L115:
	lw	$3,4($16)
	li	$2,-655360			# 0xfffffffffff60000
	ori	$2,$2,0xffff
	and	$3,$3,$2
	li	$2,65536			# 0x10000
	or	$2,$3,$2
	sw	$2,4($16)
	.set	noreorder
	.set	nomacro
	j	$L109
	li	$21,-1			# 0xffffffffffffffff
	.set	macro
	.set	reorder

$L112:
	bne	$18,$2,$L109
	lbu	$3,12($16)
	seb	$4,$3
	srl	$3,$3,5
	slt	$2,$4,0
	movn	$3,$0,$2
	addiu	$2,$3,4
	sll	$4,$2,2
	lui	$3,%hi(g_4EB0)
	addiu	$3,$3,%lo(g_4EB0)
	addu	$3,$4,$3
	lw	$3,4($3)
	.set	noreorder
	.set	nomacro
	bnel	$3,$0,$L117
	sw	$16,0($3)
	.set	macro
	.set	reorder

$L117:
	sll	$2,$2,2
	lui	$3,%hi(g_4EB0)
	addiu	$3,$3,%lo(g_4EB0)
	addu	$2,$2,$3
	sw	$16,4($2)
	lw	$3,4($16)
	li	$2,-655360			# 0xfffffffffff60000
	ori	$2,$2,0xffff
	and	$3,$3,$2
	li	$2,65536			# 0x10000
	or	$2,$3,$2
	sw	$2,4($16)
	sw	$0,0($16)
	li	$21,-1			# 0xffffffffffffffff
$L109:
	.set	noreorder
	.set	nomacro
	beq	$20,$0,$L175
	lui	$2,%hi(g_4EB0+4)
	.set	macro
	.set	reorder

	li	$2,1			# 0x1
	.set	noreorder
	.set	nomacro
	beq	$18,$2,$L118
	lui	$2,%hi(g_4EB0+4)
	.set	macro
	.set	reorder

	lw	$2,%lo(g_4EB0+4)($2)
	.set	noreorder
	.set	nomacro
	bne	$2,$0,$L119
	lui	$3,%hi(g_4E50)
	.set	macro
	.set	reorder

	lui	$2,%hi(g_4EB0)
	addiu	$2,$2,%lo(g_4EB0)
	addiu	$19,$3,%lo(g_4E50)
	sw	$19,20($2)
	sw	$0,%lo(g_4E50)($3)
	.set	noreorder
	.set	nomacro
	j	$L130
	sw	$19,4($2)
	.set	macro
	.set	reorder

$L119:
	addiu	$5,$3,%lo(g_4E50)
	lui	$4,%hi(g_4EB0+4)
	sw	$5,%lo(g_4EB0+4)($4)
	sw	$2,%lo(g_4E50)($3)
$L118:
	lui	$2,%hi(g_4EB0+4)
$L175:
	lw	$19,%lo(g_4EB0+4)($2)
	.set	noreorder
	.set	nomacro
	bne	$19,$0,$L176
	lui	$2,%hi(g_4EB0+368)
	.set	macro
	.set	reorder

	lui	$2,%hi(g_4EB0+52)
	lw	$8,%lo(g_4EB0+52)($2)
	addiu	$6,$8,1
	move	$2,$6
	addiu	$8,$8,4
	lui	$7,%hi(g_4EB0)
	addiu	$7,$7,%lo(g_4EB0)
	move	$3,$2
$L178:
	slt	$5,$2,4
	addiu	$4,$2,-3
	movz	$3,$4,$5
	sll	$3,$3,2
	addu	$3,$3,$7
	lw	$3,4($3)
	.set	noreorder
	.set	nomacro
	beq	$3,$0,$L122
	addiu	$2,$2,1
	.set	macro
	.set	reorder

	lw	$2,4($3)
	ext	$2,$2,18,1
	.set	noreorder
	.set	nomacro
	beq	$2,$0,$L177
	lui	$2,%hi(g_4EB0+52)
	.set	macro
	.set	reorder

	lw	$4,56($3)
	subu	$4,$17,$4
	lw	$2,60($3)
	sltu	$2,$4,$2
	.set	noreorder
	.set	nomacro
	bne	$2,$0,$L177
	lui	$2,%hi(g_4EB0+52)
	.set	macro
	.set	reorder

	lui	$2,%hi(g_4EB0+372)
	lw	$2,%lo(g_4EB0+372)($2)
	.set	noreorder
	.set	nomacro
	bne	$2,$0,$L177
	lui	$2,%hi(g_4EB0+52)
	.set	macro
	.set	reorder

	lui	$2,%hi(g_4EB0+368)
	lw	$2,%lo(g_4EB0+368)($2)
	.set	noreorder
	.set	nomacro
	j	$L123
	movz	$3,$19,$2
	.set	macro
	.set	reorder

$L122:
	.set	noreorder
	.set	nomacro
	bnel	$2,$8,$L178
	move	$3,$2
	.set	macro
	.set	reorder

$L123:
	lui	$2,%hi(g_4EB0+52)
$L177:
	sw	$6,%lo(g_4EB0+52)($2)
	li	$2,3			# 0x3
	.set	noreorder
	.set	nomacro
	bne	$6,$2,$L125
	lui	$2,%hi(g_4EB0+52)
	.set	macro
	.set	reorder

	sw	$0,%lo(g_4EB0+52)($2)
$L125:
	beq	$3,$0,$L126
	move	$19,$3
$L130:
	lui	$2,%hi(g_4EB0+368)
$L176:
	lw	$2,%lo(g_4EB0+368)($2)
	.set	noreorder
	.set	nomacro
	beq	$2,$0,$L144
	li	$16,4			# 0x4
	.set	macro
	.set	reorder

	lui	$2,%hi(g_4EB0+60)
	lw	$16,%lo(g_4EB0+60)($2)
$L144:
	jal	sceKernelGetSystemTimeLow
	subu	$2,$2,$17
	sltu	$2,$16,$2
	.set	noreorder
	.set	nomacro
	beq	$2,$0,$L144
	lui	$2,%hi(g_4EB0)
	.set	macro
	.set	reorder

	sw	$19,%lo(g_4EB0)($2)
	.set	noreorder
	.set	nomacro
	jal	_sceSysconPacketStart
	move	$4,$19
	.set	macro
	.set	reorder

$L126:
	.set	noreorder
	.set	nomacro
	bne	$18,$0,$L179
	lui	$2,%hi(g_4EB0+44)
	.set	macro
	.set	reorder

	.set	noreorder
	.set	nomacro
	bltz	$21,$L179
	move	$4,$21
	.set	macro
	.set	reorder

	.set	noreorder
	.set	nomacro
	jal	sceKernelSignalSema
	li	$5,1			# 0x1
	.set	macro
	.set	reorder

	lui	$2,%hi(g_4EB0+44)
$L179:
	sw	$0,%lo(g_4EB0+44)($2)
	.set	noreorder
	.set	nomacro
	jal	sceKernelCpuResumeIntr
	move	$4,$23
	.set	macro
	.set	reorder

	li	$2,-1			# 0xffffffffffffffff
	lw	$31,44($sp)
	lw	$fp,40($sp)
	lw	$23,36($sp)
	lw	$22,32($sp)
	lw	$21,28($sp)
	lw	$20,24($sp)
	lw	$19,20($sp)
	lw	$18,16($sp)
	lw	$17,12($sp)
	lw	$16,8($sp)
	.set	noreorder
	.set	nomacro
	j	$31
	addiu	$sp,$sp,48
	.set	macro
	.set	reorder

	.end	_sceSysconGpioIntr
	.size	_sceSysconGpioIntr, .-_sceSysconGpioIntr
	.align	2
	.globl	sceSysconCmdExec
	.set	nomips16
	.ent	sceSysconCmdExec
	.type	sceSysconCmdExec, @function
sceSysconCmdExec:
	.frame	$sp,16,$31		# vars= 0, regs= 3/0, args= 0, gp= 0
	.mask	0x80030000,-4
	.fmask	0x00000000,0
	addiu	$sp,$sp,-16
	sw	$31,12($sp)
	sw	$17,8($sp)
	sw	$16,4($sp)
	move	$17,$4
	.set	noreorder
	.set	nomacro
	jal	sceKernelIsIntrContext
	move	$16,$5
	.set	macro
	.set	reorder

	.set	noreorder
	.set	nomacro
	bne	$2,$0,$L184
	li	$2,-2147483648			# 0xffffffff80000000
	.set	macro
	.set	reorder

	lui	$2,%hi(g_4EB0+368)
	lw	$2,%lo(g_4EB0+368)($2)
	bne	$2,$0,$L182
 #APP
 # 47 "../../include/common/inline.h" 1
	mfic $2, $0
 # 0 "" 2
 #NO_APP
	.set	noreorder
	.set	nomacro
	bne	$2,$0,$L189
	move	$4,$17
	.set	macro
	.set	reorder

	.set	noreorder
	.set	nomacro
	j	$L187
	li	$2,-2147483648			# 0xffffffff80000000
	.set	macro
	.set	reorder

$L182:
 #APP
 # 47 "../../include/common/inline.h" 1
	mfic $2, $0
 # 0 "" 2
 #NO_APP
	.set	noreorder
	.set	nomacro
	bne	$2,$0,$L186
	move	$4,$17
	.set	macro
	.set	reorder

$L189:
	move	$5,$16
	move	$6,$0
	.set	noreorder
	.set	nomacro
	jal	sceSysconCmdExecAsync
	move	$7,$0
	.set	macro
	.set	reorder

	.set	noreorder
	.set	nomacro
	bltz	$2,$L181
	move	$4,$17
	.set	macro
	.set	reorder

	.set	noreorder
	.set	nomacro
	jal	sceSysconCmdSync
	move	$5,$0
	.set	macro
	.set	reorder

	.set	noreorder
	.set	nomacro
	j	$L188
	lw	$31,12($sp)
	.set	macro
	.set	reorder

$L184:
	.set	noreorder
	.set	nomacro
	j	$L181
	addiu	$2,$2,48
	.set	macro
	.set	reorder

$L187:
	.set	noreorder
	.set	nomacro
	j	$L181
	addiu	$2,$2,49
	.set	macro
	.set	reorder

$L186:
	li	$2,-2147483648			# 0xffffffff80000000
	addiu	$2,$2,49
$L181:
	lw	$31,12($sp)
$L188:
	lw	$17,8($sp)
	lw	$16,4($sp)
	.set	noreorder
	.set	nomacro
	j	$31
	addiu	$sp,$sp,16
	.set	macro
	.set	reorder

	.end	sceSysconCmdExec
	.size	sceSysconCmdExec, .-sceSysconCmdExec
	.align	2
	.globl	sceSysconCmdExecAsync
	.set	nomips16
	.ent	sceSysconCmdExecAsync
	.type	sceSysconCmdExecAsync, @function
sceSysconCmdExecAsync:
	.frame	$sp,24,$31		# vars= 0, regs= 6/0, args= 0, gp= 0
	.mask	0x801f0000,-4
	.fmask	0x00000000,0
	addiu	$sp,$sp,-24
	sw	$31,20($sp)
	sw	$20,16($sp)
	sw	$19,12($sp)
	sw	$18,8($sp)
	sw	$17,4($sp)
	sw	$16,0($sp)
	move	$16,$4
	move	$17,$5
	move	$18,$6
	lui	$2,%hi(g_4EB0+48)
	lw	$2,%lo(g_4EB0+48)($2)
	.set	noreorder
	.set	nomacro
	bne	$2,$0,$L216
	move	$19,$7
	.set	macro
	.set	reorder

	andi	$2,$5,0x100
	.set	noreorder
	.set	nomacro
	bne	$2,$0,$L227
	move	$2,$0
	.set	macro
	.set	reorder

	lbu	$2,25($4)
	.set	noreorder
	.set	nomacro
	bgtzl	$2,$L217
	move	$4,$0
	.set	macro
	.set	reorder

	.set	noreorder
	.set	nomacro
	j	$L226
	li	$2,-1			# 0xffffffffffffffff
	.set	macro
	.set	reorder

$L192:
$L227:
	li	$5,-1			# 0xffffffffffffffff
	.set	noreorder
	.set	nomacro
	j	$L195
	li	$4,16			# 0x10
	.set	macro
	.set	reorder

$L217:
	move	$3,$0
	addu	$5,$16,$3
$L228:
	lbu	$5,12($5)
	addu	$4,$4,$5
	addiu	$3,$3,1
	slt	$5,$3,$2
	.set	noreorder
	.set	nomacro
	bnel	$5,$0,$L228
	addu	$5,$16,$3
	.set	macro
	.set	reorder

	addu	$3,$16,$2
	nor	$4,$0,$4
	sb	$4,12($3)
	addiu	$2,$2,1
	slt	$3,$2,16
	.set	noreorder
	.set	nomacro
	beql	$3,$0,$L192
	move	$2,$0
	.set	macro
	.set	reorder

$L197:
	li	$4,-1			# 0xffffffffffffffff
	addu	$3,$16,$2
$L229:
	sb	$4,12($3)
	addiu	$2,$2,1
	slt	$3,$2,16
	.set	noreorder
	.set	nomacro
	bne	$3,$0,$L229
	addu	$3,$16,$2
	.set	macro
	.set	reorder

	.set	noreorder
	.set	nomacro
	j	$L227
	move	$2,$0
	.set	macro
	.set	reorder

$L195:
	addu	$3,$16,$2
	addiu	$2,$2,1
	.set	noreorder
	.set	nomacro
	bne	$2,$4,$L195
	sb	$5,28($3)
	.set	macro
	.set	reorder

	jal	sceKernelCpuSuspendIntr
	move	$20,$2
	andi	$3,$17,0xffff
	li	$2,65536			# 0x10000
	or	$2,$3,$2
	sw	$2,4($16)
	li	$2,-1			# 0xffffffffffffffff
	sw	$2,8($16)
	sw	$18,44($16)
	sw	$0,0($16)
 #APP
 # 53 "../../include/common/registers.h" 1
	move $2, $gp
 # 0 "" 2
 #NO_APP
	sw	$2,48($16)
	lbu	$2,12($16)
	seb	$3,$2
	.set	noreorder
	.set	nomacro
	bltz	$3,$L198
	sw	$19,52($16)
	.set	macro
	.set	reorder

	srl	$2,$2,5
	.set	noreorder
	.set	nomacro
	beq	$2,$0,$L199
	lui	$3,%hi(g_4EB0+60)
	.set	macro
	.set	reorder

	lw	$3,%lo(g_4EB0+60)($3)
	.set	noreorder
	.set	nomacro
	j	$L200
	sw	$3,60($16)
	.set	macro
	.set	reorder

$L199:
	sw	$0,60($16)
$L200:
	sll	$4,$2,2
	lui	$3,%hi(g_4EB0)
	addiu	$3,$3,%lo(g_4EB0)
	addu	$3,$4,$3
	lw	$3,4($3)
	.set	noreorder
	.set	nomacro
	bne	$3,$0,$L201
	andi	$4,$17,0x1
	.set	macro
	.set	reorder

	addiu	$4,$2,4
	sll	$4,$4,2
	lui	$3,%hi(g_4EB0)
	addiu	$3,$3,%lo(g_4EB0)
	addu	$4,$4,$3
	sw	$16,4($4)
	sll	$4,$2,2
	addu	$3,$4,$3
	.set	noreorder
	.set	nomacro
	j	$L202
	sw	$16,4($3)
	.set	macro
	.set	reorder

$L201:
	.set	noreorder
	.set	nomacro
	bnel	$4,$0,$L203
	lw	$4,4($3)
	.set	macro
	.set	reorder

	addiu	$4,$2,4
	sll	$4,$4,2
	lui	$3,%hi(g_4EB0)
	addiu	$3,$3,%lo(g_4EB0)
	addu	$3,$4,$3
	lw	$4,4($3)
	sw	$16,0($4)
	.set	noreorder
	.set	nomacro
	j	$L202
	sw	$16,4($3)
	.set	macro
	.set	reorder

$L203:
	ext	$4,$4,17,2
	.set	noreorder
	.set	nomacro
	beql	$4,$0,$L204
	sw	$3,0($16)
	.set	macro
	.set	reorder

	lw	$4,0($3)
	sw	$4,0($16)
	.set	noreorder
	.set	nomacro
	bne	$4,$0,$L202
	sw	$16,0($3)
	.set	macro
	.set	reorder

	addiu	$4,$2,4
	sll	$4,$4,2
	lui	$3,%hi(g_4EB0)
	addiu	$3,$3,%lo(g_4EB0)
	addu	$3,$4,$3
	sw	$16,4($3)
$L204:
$L202:
	lui	$3,%hi(g_4EB0+368)
	lw	$3,%lo(g_4EB0+368)($3)
	.set	noreorder
	.set	nomacro
	beq	$3,$0,$L205
	lui	$3,%hi(g_4EB0)
	.set	macro
	.set	reorder

	lui	$2,%hi(g_4EB0)
	sw	$16,%lo(g_4EB0)($2)
	.set	noreorder
	.set	nomacro
	jal	_sceSysconPacketStart
	move	$4,$16
	.set	macro
	.set	reorder

	j	$L206
$L205:
	lw	$3,%lo(g_4EB0)($3)
	.set	noreorder
	.set	nomacro
	bne	$3,$0,$L207
	lui	$3,%hi(g_4EB0+44)
	.set	macro
	.set	reorder

	lw	$3,%lo(g_4EB0+44)($3)
	.set	noreorder
	.set	nomacro
	beq	$3,$0,$L208
	lui	$3,%hi(g_4EB0+4)
	.set	macro
	.set	reorder

$L207:
	.set	noreorder
	.set	nomacro
	jal	sceKernelCpuResumeIntr
	move	$4,$20
	.set	macro
	.set	reorder

	.set	noreorder
	.set	nomacro
	j	$L191
	move	$2,$0
	.set	macro
	.set	reorder

$L208:
	lw	$18,%lo(g_4EB0+4)($3)
	beq	$18,$0,$L209
	.set	noreorder
	.set	nomacro
	bne	$18,$16,$L209
	andi	$3,$17,0x2
	.set	macro
	.set	reorder

	movn	$18,$0,$3
$L209:
	beq	$2,$0,$L210
	.set	noreorder
	.set	nomacro
	beq	$18,$0,$L211
	andi	$3,$17,0x1
	.set	macro
	.set	reorder

	.set	noreorder
	.set	nomacro
	beql	$3,$0,$L230
	lui	$2,%hi(g_4EB0+372)
	.set	macro
	.set	reorder

$L211:
	sll	$2,$2,2
	lui	$3,%hi(g_4EB0)
	addiu	$3,$3,%lo(g_4EB0)
	addu	$2,$2,$3
	lw	$18,4($2)
	.set	noreorder
	.set	nomacro
	bne	$18,$16,$L210
	andi	$17,$17,0x2
	.set	macro
	.set	reorder

	bne	$17,$0,$L213
$L210:
	.set	noreorder
	.set	nomacro
	bnel	$18,$0,$L212
	lui	$2,%hi(g_4EB0+372)
	.set	macro
	.set	reorder

$L213:
	.set	noreorder
	.set	nomacro
	jal	sceKernelCpuResumeIntr
	move	$4,$20
	.set	macro
	.set	reorder

	.set	noreorder
	.set	nomacro
	j	$L191
	move	$2,$0
	.set	macro
	.set	reorder

$L212:
$L230:
	lw	$2,%lo(g_4EB0+372)($2)
	.set	noreorder
	.set	nomacro
	beq	$2,$0,$L215
	li	$16,4			# 0x4
	.set	macro
	.set	reorder

	lui	$2,%hi(g_4EB0+56)
	lw	$16,%lo(g_4EB0+56)($2)
$L215:
	lui	$17,%hi(g_4EB0)
$L219:
	jal	sceKernelGetSystemTimeLow
	addiu	$3,$17,%lo(g_4EB0)
	lw	$3,40($3)
	subu	$3,$2,$3
	sltu	$3,$16,$3
	.set	noreorder
	.set	nomacro
	beq	$3,$0,$L219
	lui	$2,%hi(g_4EB0)
	.set	macro
	.set	reorder

	sw	$18,%lo(g_4EB0)($2)
	.set	noreorder
	.set	nomacro
	jal	_sceSysconPacketStart
	move	$4,$18
	.set	macro
	.set	reorder

$L206:
	.set	noreorder
	.set	nomacro
	jal	sceKernelCpuResumeIntr
	move	$4,$20
	.set	macro
	.set	reorder

	.set	noreorder
	.set	nomacro
	j	$L191
	move	$2,$0
	.set	macro
	.set	reorder

$L216:
	li	$2,-2145058816			# 0xffffffff80250000
	addiu	$2,$2,3
$L191:
	lw	$31,20($sp)
	lw	$20,16($sp)
	lw	$19,12($sp)
	lw	$18,8($sp)
	lw	$17,4($sp)
	lw	$16,0($sp)
	.set	noreorder
	.set	nomacro
	j	$31
	addiu	$sp,$sp,24
	.set	macro
	.set	reorder

$L226:
	sb	$2,12($4)
	.set	noreorder
	.set	nomacro
	j	$L197
	li	$2,1			# 0x1
	.set	macro
	.set	reorder

$L198:
	.set	noreorder
	.set	nomacro
	j	$L199
	move	$2,$0
	.set	macro
	.set	reorder

	.end	sceSysconCmdExecAsync
	.size	sceSysconCmdExecAsync, .-sceSysconCmdExecAsync
	.align	2
	.globl	sceSysconCmdCancel
	.set	nomips16
	.ent	sceSysconCmdCancel
	.type	sceSysconCmdCancel, @function
sceSysconCmdCancel:
	.frame	$sp,16,$31		# vars= 0, regs= 3/0, args= 0, gp= 0
	.mask	0x80030000,-4
	.fmask	0x00000000,0
	.set	noreorder
	.set	nomacro
	addiu	$sp,$sp,-16
	sw	$31,12($sp)
	sw	$17,8($sp)
	sw	$16,4($sp)
	jal	sceKernelCpuSuspendIntr
	move	$17,$4

	lbu	$5,12($17)
	seb	$4,$5
	srl	$5,$5,5
	slt	$3,$4,0
	movn	$5,$0,$3
	sll	$3,$5,2
	lui	$4,%hi(g_4EB0)
	addiu	$4,$4,%lo(g_4EB0)
	addu	$3,$3,$4
	lw	$5,4($3)
	beql	$5,$0,$L242
	li	$16,-2147483648			# 0xffffffff80000000

	bnel	$17,$5,$L252
	lw	$3,0($5)

	j	$L250
	move	$3,$5

$L240:
	bnel	$17,$3,$L244
	move	$5,$3

	j	$L251
	lw	$4,4($3)

$L250:
	move	$5,$0
	lw	$4,4($3)
$L251:
	ext	$4,$4,17,1
	bne	$4,$0,$L245
	li	$16,-2147483648			# 0xffffffff80000000

	bnel	$5,$0,$L236
	lw	$4,0($3)

	lbu	$6,12($3)
	seb	$7,$6
	bltzl	$7,$L253
	lw	$6,0($3)

	srl	$4,$6,5
	sll	$4,$4,2
	lw	$6,0($3)
$L253:
	sll	$4,$4,2
	lui	$7,%hi(g_4EB0)
	addiu	$7,$7,%lo(g_4EB0)
	addu	$4,$4,$7
	j	$L238
	sw	$6,4($4)

$L236:
	sw	$4,0($5)
$L238:
	lw	$3,0($3)
	bne	$3,$0,$L233
	move	$16,$0

	lbu	$4,12($17)
	seb	$6,$4
	bltz	$6,$L239
	li	$3,4			# 0x4

	srl	$3,$4,5
	addiu	$3,$3,4
$L239:
	sll	$3,$3,2
	lui	$4,%hi(g_4EB0)
	addiu	$4,$4,%lo(g_4EB0)
	addu	$3,$3,$4
	sw	$5,4($3)
	j	$L233
	move	$16,$0

$L244:
	lw	$3,0($5)
$L252:
	bne	$3,$0,$L240
	li	$16,-2147483648			# 0xffffffff80000000

	j	$L233
	addiu	$16,$16,37

$L242:
	j	$L233
	addiu	$16,$16,37

$L245:
	addiu	$16,$16,33
$L233:
	jal	sceKernelCpuResumeIntr
	move	$4,$2

	move	$2,$16
	lw	$31,12($sp)
	lw	$17,8($sp)
	lw	$16,4($sp)
	j	$31
	addiu	$sp,$sp,16

	.set	macro
	.set	reorder
	.end	sceSysconCmdCancel
	.size	sceSysconCmdCancel, .-sceSysconCmdCancel
	.section	.rodata.str1.4
	.align	2
$LC2:
	.ascii	"SceSysconSync\000"
	.text
	.align	2
	.globl	sceSysconCmdSync
	.set	nomips16
	.ent	sceSysconCmdSync
	.type	sceSysconCmdSync, @function
sceSysconCmdSync:
	.frame	$sp,24,$31		# vars= 0, regs= 5/0, args= 0, gp= 0
	.mask	0x800f0000,-4
	.fmask	0x00000000,0
	.set	noreorder
	.set	nomacro
	addiu	$sp,$sp,-24
	sw	$31,20($sp)
	sw	$19,16($sp)
	sw	$18,12($sp)
	sw	$17,8($sp)
	sw	$16,4($sp)
	beq	$5,$0,$L255
	move	$17,$4

	li	$2,1			# 0x1
	bnel	$5,$2,$L270
	li	$16,-2147483648			# 0xffffffff80000000

	bnel	$4,$0,$L257
	lw	$2,4($4)

	lui	$2,%hi(g_4EB0+4)
	lw	$2,%lo(g_4EB0+4)($2)
	bne	$2,$0,$L256
	li	$16,1			# 0x1

	lui	$2,%hi(g_4EB0+8)
	lw	$2,%lo(g_4EB0+8)($2)
	bne	$2,$0,$L285
	move	$2,$16

	lui	$2,%hi(g_4EB0+12)
	lw	$2,%lo(g_4EB0+12)($2)
	bne	$2,$0,$L285
	move	$2,$16

	lui	$2,%hi(g_4EB0+16)
	lw	$16,%lo(g_4EB0+16)($2)
	j	$L256
	sltu	$16,$0,$16

$L257:
	ext	$2,$2,19,1
	bne	$2,$0,$L258
	li	$16,1			# 0x1

	j	$L285
	move	$2,$16

$L255:
	lui	$2,%hi(g_4EB0+368)
	lw	$2,%lo(g_4EB0+368)($2)
	beq	$2,$0,$L259
	nop

	lw	$2,4($4)
	ext	$2,$2,19,1
	beql	$2,$0,$L280
	lui	$16,%hi(g_4EB0)

	j	$L286
	lw	$3,4($17)

$L259:
	jal	sceKernelIsIntrContext
	nop

	bne	$2,$0,$L275
	li	$16,-2147483648			# 0xffffffff80000000

	jal	sceKernelCpuSuspendIntr
	nop

	bne	$2,$0,$L261
	move	$18,$2

	jal	sceKernelCpuResumeIntr
	move	$4,$0

	li	$16,-2147483648			# 0xffffffff80000000
	j	$L256
	addiu	$16,$16,49

$L261:
	lw	$2,4($17)
	ext	$2,$2,19,1
	beq	$2,$0,$L262
	lui	$2,%hi(g_4EB0+380)

	jal	sceKernelCpuResumeIntr
	move	$4,$18

	j	$L286
	lw	$3,4($17)

$L262:
	lw	$19,%lo(g_4EB0+380)($2)
	bgtz	$19,$L263
	lui	$2,%hi(g_4EB0+380)

	lui	$4,%hi($LC2)
	addiu	$4,$4,%lo($LC2)
	li	$5,1			# 0x1
	move	$6,$0
	li	$7,1			# 0x1
	jal	sceKernelCreateSema
	move	$8,$0

	bgez	$2,$L264
	move	$16,$2

	jal	sceKernelCpuResumeIntr
	move	$4,$18

	j	$L285
	move	$2,$16

$L263:
	sw	$0,%lo(g_4EB0+380)($2)
	sw	$19,8($17)
	jal	sceKernelCpuResumeIntr
	move	$4,$18

	move	$4,$19
	li	$5,1			# 0x1
	jal	sceKernelWaitSema
	move	$6,$0

	j	$L287
	lui	$2,%hi(g_4EB0+380)

$L264:
	sw	$2,8($17)
	jal	sceKernelCpuResumeIntr
	move	$4,$18

	move	$4,$16
	li	$5,1			# 0x1
	jal	sceKernelWaitSema
	move	$6,$0

	bne	$19,$16,$L266
	lui	$2,%hi(g_4EB0+380)

$L287:
	j	$L258
	sw	$19,%lo(g_4EB0+380)($2)

$L266:
	jal	sceKernelDeleteSema
	move	$4,$16

	j	$L286
	lw	$3,4($17)

$L280:
	li	$18,524288			# 0x80000
$L284:
	jal	sceKernelGetSystemTimeLow
	nop

	addiu	$3,$16,%lo(g_4EB0)
	lw	$3,40($3)
	subu	$3,$2,$3
	sltu	$3,$3,5
	bne	$3,$0,$L284
	nop

$L281:
	jal	sceGpioQueryIntr
	li	$4,4			# 0x4

	beq	$2,$0,$L281
	li	$4,4			# 0x4

	move	$5,$0
	jal	_sceSysconGpioIntr
	move	$6,$0

	lw	$2,4($17)
	and	$2,$2,$18
	beq	$2,$0,$L284
	nop

$L258:
	lw	$3,4($17)
$L286:
	li	$2,11534336			# 0xb00000
	and	$2,$3,$2
	bnel	$2,$0,$L276
	li	$16,-2145058816			# 0xffffffff80250000

	lbu	$3,30($17)
	seb	$2,$3
	bgezl	$2,$L288
	lbu	$3,12($17)

	li	$4,130			# 0x82
	beq	$3,$4,$L269
	li	$16,-2145058816			# 0xffffffff80250000

	j	$L256
	or	$16,$3,$16

$L269:
	lbu	$3,12($17)
$L288:
	sltu	$3,$3,32
	bne	$3,$0,$L256
	move	$16,$0

	bltz	$2,$L285
	move	$2,$16

	li	$16,-2145058816			# 0xffffffff80250000
	j	$L256
	addiu	$16,$16,4

$L270:
	j	$L256
	addiu	$16,$16,263

$L275:
	j	$L256
	addiu	$16,$16,48

$L276:
	addiu	$16,$16,2
$L256:
	move	$2,$16
$L285:
	lw	$31,20($sp)
	lw	$19,16($sp)
	lw	$18,12($sp)
	lw	$17,8($sp)
	lw	$16,4($sp)
	j	$31
	addiu	$sp,$sp,24

	.set	macro
	.set	reorder
	.end	sceSysconCmdSync
	.size	sceSysconCmdSync, .-sceSysconCmdSync
	.align	2
	.globl	_sceSysconCommonRead
	.set	nomips16
	.ent	_sceSysconCommonRead
	.type	_sceSysconCommonRead, @function
_sceSysconCommonRead:
	.frame	$sp,120,$31		# vars= 112, regs= 2/0, args= 0, gp= 0
	.mask	0x80010000,-4
	.fmask	0x00000000,0
	.set	noreorder
	.set	nomacro
	addiu	$sp,$sp,-120
	sw	$31,116($sp)
	sw	$16,112($sp)
	beq	$4,$0,$L291
	move	$16,$4

	li	$2,2			# 0x2
	sb	$2,13($sp)
	sb	$5,12($sp)
	move	$4,$sp
	jal	sceSysconCmdExec
	move	$5,$0

	bltz	$2,$L290
	lbu	$6,29($sp)

	addiu	$2,$6,-4
	andi	$2,$2,0x00ff
	sltu	$2,$2,4
	beq	$2,$0,$L292
	addiu	$4,$sp,96

	sw	$0,96($sp)
	addiu	$5,$sp,31
	jal	memcpy
	addiu	$6,$6,-3

	lw	$2,96($sp)
	sw	$2,0($16)
	j	$L290
	move	$2,$0

$L291:
	li	$2,-2147483648			# 0xffffffff80000000
	j	$L290
	addiu	$2,$2,259

$L292:
	li	$2,-2145058816			# 0xffffffff80250000
	addiu	$2,$2,1
$L290:
	lw	$31,116($sp)
	lw	$16,112($sp)
	j	$31
	addiu	$sp,$sp,120

	.set	macro
	.set	reorder
	.end	_sceSysconCommonRead
	.size	_sceSysconCommonRead, .-_sceSysconCommonRead
	.align	2
	.globl	_sceSysconModuleStart
	.set	nomips16
	.ent	_sceSysconModuleStart
	.type	_sceSysconModuleStart, @function
_sceSysconModuleStart:
	.frame	$sp,8,$31		# vars= 0, regs= 1/0, args= 0, gp= 0
	.mask	0x80000000,-4
	.fmask	0x00000000,0
	.set	noreorder
	.set	nomacro
	addiu	$sp,$sp,-8
	sw	$31,4($sp)
	jal	sceSysconInit
	nop

	move	$2,$0
	lw	$31,4($sp)
	j	$31
	addiu	$sp,$sp,8

	.set	macro
	.set	reorder
	.end	_sceSysconModuleStart
	.size	_sceSysconModuleStart, .-_sceSysconModuleStart
	.align	2
	.globl	sceSyscon_driver_90EAEA2B
	.set	nomips16
	.ent	sceSyscon_driver_90EAEA2B
	.type	sceSyscon_driver_90EAEA2B, @function
sceSyscon_driver_90EAEA2B:
	.frame	$sp,0,$31		# vars= 0, regs= 0/0, args= 0, gp= 0
	.mask	0x00000000,0
	.fmask	0x00000000,0
	.set	noreorder
	.set	nomacro
	lui	$2,%hi(g_4EB0)
	addiu	$2,$2,%lo(g_4EB0)
	sw	$5,60($2)
	sw	$4,56($2)
	j	$31
	move	$2,$0

	.set	macro
	.set	reorder
	.end	sceSyscon_driver_90EAEA2B
	.size	sceSyscon_driver_90EAEA2B, .-sceSyscon_driver_90EAEA2B
	.align	2
	.globl	sceSyscon_driver_755CF72B
	.set	nomips16
	.ent	sceSyscon_driver_755CF72B
	.type	sceSyscon_driver_755CF72B, @function
sceSyscon_driver_755CF72B:
	.frame	$sp,0,$31		# vars= 0, regs= 0/0, args= 0, gp= 0
	.mask	0x00000000,0
	.fmask	0x00000000,0
	.set	noreorder
	.set	nomacro
	beq	$4,$0,$L296
	lui	$2,%hi(g_4EB0+56)

	lw	$2,%lo(g_4EB0+56)($2)
	sw	$2,0($4)
$L296:
	beq	$5,$0,$L297
	nop

	lui	$2,%hi(g_4EB0+60)
	lw	$2,%lo(g_4EB0+60)($2)
	sw	$2,0($5)
$L297:
	j	$31
	move	$2,$0

	.set	macro
	.set	reorder
	.end	sceSyscon_driver_755CF72B
	.size	sceSyscon_driver_755CF72B, .-sceSyscon_driver_755CF72B
	.align	2
	.globl	sceSysconSuspend
	.set	nomips16
	.ent	sceSysconSuspend
	.type	sceSysconSuspend, @function
sceSysconSuspend:
	.frame	$sp,8,$31		# vars= 0, regs= 1/0, args= 0, gp= 0
	.mask	0x80000000,-4
	.fmask	0x00000000,0
	.set	noreorder
	.set	nomacro
	addiu	$sp,$sp,-8
	sw	$31,4($sp)
	lui	$2,%hi(g_4EB0+376)
	lw	$2,%lo(g_4EB0+376)($2)
	beq	$2,$0,$L300
	li	$4,4			# 0x4

	jal	sceSyscon_driver_765775EB
	move	$4,$0

	li	$4,4			# 0x4
$L300:
	jal	sceKernelDisableSubIntr
	li	$5,4			# 0x4

	move	$2,$0
	lw	$31,4($sp)
	j	$31
	addiu	$sp,$sp,8

	.set	macro
	.set	reorder
	.end	sceSysconSuspend
	.size	sceSysconSuspend, .-sceSysconSuspend
	.align	2
	.globl	sceSysconSetDebugHandlers
	.set	nomips16
	.ent	sceSysconSetDebugHandlers
	.type	sceSysconSetDebugHandlers, @function
sceSysconSetDebugHandlers:
	.frame	$sp,0,$31		# vars= 0, regs= 0/0, args= 0, gp= 0
	.mask	0x00000000,0
	.fmask	0x00000000,0
	.set	noreorder
	.set	nomacro
	lui	$2,%hi(g_4EB0+36)
	sw	$4,%lo(g_4EB0+36)($2)
	j	$31
	move	$2,$0

	.set	macro
	.set	reorder
	.end	sceSysconSetDebugHandlers
	.size	sceSysconSetDebugHandlers, .-sceSysconSetDebugHandlers
	.align	2
	.globl	sceSysconSetPollingMode
	.set	nomips16
	.ent	sceSysconSetPollingMode
	.type	sceSysconSetPollingMode, @function
sceSysconSetPollingMode:
	.frame	$sp,0,$31		# vars= 0, regs= 0/0, args= 0, gp= 0
	.mask	0x00000000,0
	.fmask	0x00000000,0
	.set	noreorder
	.set	nomacro
	lui	$2,%hi(g_4EB0+368)
	sw	$4,%lo(g_4EB0+368)($2)
	j	$31
	move	$2,$0

	.set	macro
	.set	reorder
	.end	sceSysconSetPollingMode
	.size	sceSysconSetPollingMode, .-sceSysconSetPollingMode
	.align	2
	.globl	sceSysconSetAffirmativeRertyMode
	.set	nomips16
	.ent	sceSysconSetAffirmativeRertyMode
	.type	sceSysconSetAffirmativeRertyMode, @function
sceSysconSetAffirmativeRertyMode:
	.frame	$sp,0,$31		# vars= 0, regs= 0/0, args= 0, gp= 0
	.mask	0x00000000,0
	.fmask	0x00000000,0
	.set	noreorder
	.set	nomacro
	lui	$2,%hi(g_4EB0+372)
	sw	$4,%lo(g_4EB0+372)($2)
	j	$31
	move	$2,$0

	.set	macro
	.set	reorder
	.end	sceSysconSetAffirmativeRertyMode
	.size	sceSysconSetAffirmativeRertyMode, .-sceSysconSetAffirmativeRertyMode
	.align	2
	.globl	_sceSysconGetBaryonVersion
	.set	nomips16
	.ent	_sceSysconGetBaryonVersion
	.type	_sceSysconGetBaryonVersion, @function
_sceSysconGetBaryonVersion:
	.frame	$sp,0,$31		# vars= 0, regs= 0/0, args= 0, gp= 0
	.mask	0x00000000,0
	.fmask	0x00000000,0
	.set	noreorder
	.set	nomacro
	lui	$2,%hi(g_4EB0+332)
	j	$31
	lw	$2,%lo(g_4EB0+332)($2)

	.set	macro
	.set	reorder
	.end	_sceSysconGetBaryonVersion
	.size	_sceSysconGetBaryonVersion, .-_sceSysconGetBaryonVersion
	.align	2
	.globl	_sceSysconGetBaryonTimeStamp
	.set	nomips16
	.ent	_sceSysconGetBaryonTimeStamp
	.type	_sceSysconGetBaryonTimeStamp, @function
_sceSysconGetBaryonTimeStamp:
	.frame	$sp,0,$31		# vars= 0, regs= 0/0, args= 0, gp= 0
	.mask	0x00000000,0
	.fmask	0x00000000,0
	.set	noreorder
	.set	nomacro
	lui	$2,%hi(g_4EB0)
	addiu	$2,$2,%lo(g_4EB0)
	lw	$3,364($2)
	j	$31
	lw	$2,360($2)

	.set	macro
	.set	reorder
	.end	_sceSysconGetBaryonTimeStamp
	.size	_sceSysconGetBaryonTimeStamp, .-_sceSysconGetBaryonTimeStamp
	.align	2
	.globl	_sceSysconGetPommelVersion
	.set	nomips16
	.ent	_sceSysconGetPommelVersion
	.type	_sceSysconGetPommelVersion, @function
_sceSysconGetPommelVersion:
	.frame	$sp,0,$31		# vars= 0, regs= 0/0, args= 0, gp= 0
	.mask	0x00000000,0
	.fmask	0x00000000,0
	.set	noreorder
	.set	nomacro
	lui	$2,%hi(g_4EB0+356)
	j	$31
	lw	$2,%lo(g_4EB0+356)($2)

	.set	macro
	.set	reorder
	.end	_sceSysconGetPommelVersion
	.size	_sceSysconGetPommelVersion, .-_sceSysconGetPommelVersion
	.align	2
	.globl	_sceSysconGetUsbPowerType
	.set	nomips16
	.ent	_sceSysconGetUsbPowerType
	.type	_sceSysconGetUsbPowerType, @function
_sceSysconGetUsbPowerType:
	.frame	$sp,0,$31		# vars= 0, regs= 0/0, args= 0, gp= 0
	.mask	0x00000000,0
	.fmask	0x00000000,0
	.set	noreorder
	.set	nomacro
	lui	$2,%hi(g_4EB0+332)
	lhu	$4,%lo(g_4EB0+334)($2)
	andi	$3,$4,0xf0
	beq	$3,$0,$L311
	move	$2,$0

	li	$5,16			# 0x10
	beq	$3,$5,$L311
	nop

	andi	$2,$4,0xff
	addiu	$2,$2,-32
	sltu	$2,$2,2
	xori	$2,$2,0x1
$L311:
	j	$31
	nop

	.set	macro
	.set	reorder
	.end	_sceSysconGetUsbPowerType
	.size	_sceSysconGetUsbPowerType, .-_sceSysconGetUsbPowerType
	.align	2
	.globl	sceSysconSetGSensorCallback
	.set	nomips16
	.ent	sceSysconSetGSensorCallback
	.type	sceSysconSetGSensorCallback, @function
sceSysconSetGSensorCallback:
	.frame	$sp,8,$31		# vars= 0, regs= 1/0, args= 0, gp= 0
	.mask	0x80000000,-4
	.fmask	0x00000000,0
	.set	noreorder
	.set	nomacro
	addiu	$sp,$sp,-8
	sw	$31,4($sp)
	jal	_sceSysconSetCallback
	li	$6,10			# 0xa

	lw	$31,4($sp)
	j	$31
	addiu	$sp,$sp,8

	.set	macro
	.set	reorder
	.end	sceSysconSetGSensorCallback
	.size	sceSysconSetGSensorCallback, .-sceSysconSetGSensorCallback
	.align	2
	.globl	sceSysconSetLowBatteryCallback
	.set	nomips16
	.ent	sceSysconSetLowBatteryCallback
	.type	sceSysconSetLowBatteryCallback, @function
sceSysconSetLowBatteryCallback:
	.frame	$sp,8,$31		# vars= 0, regs= 1/0, args= 0, gp= 0
	.mask	0x80000000,-4
	.fmask	0x00000000,0
	.set	noreorder
	.set	nomacro
	addiu	$sp,$sp,-8
	sw	$31,4($sp)
	jal	_sceSysconSetCallback
	move	$6,$0

	lw	$31,4($sp)
	j	$31
	addiu	$sp,$sp,8

	.set	macro
	.set	reorder
	.end	sceSysconSetLowBatteryCallback
	.size	sceSysconSetLowBatteryCallback, .-sceSysconSetLowBatteryCallback
	.align	2
	.globl	sceSysconSetPowerSwitchCallback
	.set	nomips16
	.ent	sceSysconSetPowerSwitchCallback
	.type	sceSysconSetPowerSwitchCallback, @function
sceSysconSetPowerSwitchCallback:
	.frame	$sp,8,$31		# vars= 0, regs= 1/0, args= 0, gp= 0
	.mask	0x80000000,-4
	.fmask	0x00000000,0
	.set	noreorder
	.set	nomacro
	addiu	$sp,$sp,-8
	sw	$31,4($sp)
	jal	_sceSysconSetCallback
	li	$6,1			# 0x1

	lw	$31,4($sp)
	j	$31
	addiu	$sp,$sp,8

	.set	macro
	.set	reorder
	.end	sceSysconSetPowerSwitchCallback
	.size	sceSysconSetPowerSwitchCallback, .-sceSysconSetPowerSwitchCallback
	.align	2
	.globl	sceSysconSetAlarmCallback
	.set	nomips16
	.ent	sceSysconSetAlarmCallback
	.type	sceSysconSetAlarmCallback, @function
sceSysconSetAlarmCallback:
	.frame	$sp,8,$31		# vars= 0, regs= 1/0, args= 0, gp= 0
	.mask	0x80000000,-4
	.fmask	0x00000000,0
	.set	noreorder
	.set	nomacro
	addiu	$sp,$sp,-8
	sw	$31,4($sp)
	jal	_sceSysconSetCallback
	li	$6,2			# 0x2

	lw	$31,4($sp)
	j	$31
	addiu	$sp,$sp,8

	.set	macro
	.set	reorder
	.end	sceSysconSetAlarmCallback
	.size	sceSysconSetAlarmCallback, .-sceSysconSetAlarmCallback
	.align	2
	.globl	sceSysconSetAcSupplyCallback
	.set	nomips16
	.ent	sceSysconSetAcSupplyCallback
	.type	sceSysconSetAcSupplyCallback, @function
sceSysconSetAcSupplyCallback:
	.frame	$sp,8,$31		# vars= 0, regs= 1/0, args= 0, gp= 0
	.mask	0x80000000,-4
	.fmask	0x00000000,0
	.set	noreorder
	.set	nomacro
	addiu	$sp,$sp,-8
	sw	$31,4($sp)
	jal	_sceSysconSetCallback
	li	$6,3			# 0x3

	lw	$31,4($sp)
	j	$31
	addiu	$sp,$sp,8

	.set	macro
	.set	reorder
	.end	sceSysconSetAcSupplyCallback
	.size	sceSysconSetAcSupplyCallback, .-sceSysconSetAcSupplyCallback
	.align	2
	.globl	sceSysconSetAcSupply2Callback
	.set	nomips16
	.ent	sceSysconSetAcSupply2Callback
	.type	sceSysconSetAcSupply2Callback, @function
sceSysconSetAcSupply2Callback:
	.frame	$sp,8,$31		# vars= 0, regs= 1/0, args= 0, gp= 0
	.mask	0x80000000,-4
	.fmask	0x00000000,0
	.set	noreorder
	.set	nomacro
	addiu	$sp,$sp,-8
	sw	$31,4($sp)
	jal	_sceSysconSetCallback
	li	$6,15			# 0xf

	lw	$31,4($sp)
	j	$31
	addiu	$sp,$sp,8

	.set	macro
	.set	reorder
	.end	sceSysconSetAcSupply2Callback
	.size	sceSysconSetAcSupply2Callback, .-sceSysconSetAcSupply2Callback
	.align	2
	.globl	sceSysconSetHPConnectCallback
	.set	nomips16
	.ent	sceSysconSetHPConnectCallback
	.type	sceSysconSetHPConnectCallback, @function
sceSysconSetHPConnectCallback:
	.frame	$sp,8,$31		# vars= 0, regs= 1/0, args= 0, gp= 0
	.mask	0x80000000,-4
	.fmask	0x00000000,0
	.set	noreorder
	.set	nomacro
	addiu	$sp,$sp,-8
	sw	$31,4($sp)
	jal	_sceSysconSetCallback
	li	$6,4			# 0x4

	lw	$31,4($sp)
	j	$31
	addiu	$sp,$sp,8

	.set	macro
	.set	reorder
	.end	sceSysconSetHPConnectCallback
	.size	sceSysconSetHPConnectCallback, .-sceSysconSetHPConnectCallback
	.align	2
	.globl	sceSysconSetHRPowerCallback
	.set	nomips16
	.ent	sceSysconSetHRPowerCallback
	.type	sceSysconSetHRPowerCallback, @function
sceSysconSetHRPowerCallback:
	.frame	$sp,8,$31		# vars= 0, regs= 1/0, args= 0, gp= 0
	.mask	0x80000000,-4
	.fmask	0x00000000,0
	.set	noreorder
	.set	nomacro
	addiu	$sp,$sp,-8
	sw	$31,4($sp)
	jal	_sceSysconSetCallback
	li	$6,8			# 0x8

	lw	$31,4($sp)
	j	$31
	addiu	$sp,$sp,8

	.set	macro
	.set	reorder
	.end	sceSysconSetHRPowerCallback
	.size	sceSysconSetHRPowerCallback, .-sceSysconSetHRPowerCallback
	.align	2
	.globl	sceSysconSetHRWakeupCallback
	.set	nomips16
	.ent	sceSysconSetHRWakeupCallback
	.type	sceSysconSetHRWakeupCallback, @function
sceSysconSetHRWakeupCallback:
	.frame	$sp,8,$31		# vars= 0, regs= 1/0, args= 0, gp= 0
	.mask	0x80000000,-4
	.fmask	0x00000000,0
	.set	noreorder
	.set	nomacro
	addiu	$sp,$sp,-8
	sw	$31,4($sp)
	jal	_sceSysconSetCallback
	li	$6,14			# 0xe

	lw	$31,4($sp)
	j	$31
	addiu	$sp,$sp,8

	.set	macro
	.set	reorder
	.end	sceSysconSetHRWakeupCallback
	.size	sceSysconSetHRWakeupCallback, .-sceSysconSetHRWakeupCallback
	.align	2
	.globl	sceSysconSetWlanSwitchCallback
	.set	nomips16
	.ent	sceSysconSetWlanSwitchCallback
	.type	sceSysconSetWlanSwitchCallback, @function
sceSysconSetWlanSwitchCallback:
	.frame	$sp,8,$31		# vars= 0, regs= 1/0, args= 0, gp= 0
	.mask	0x80000000,-4
	.fmask	0x00000000,0
	.set	noreorder
	.set	nomacro
	addiu	$sp,$sp,-8
	sw	$31,4($sp)
	jal	_sceSysconSetCallback
	li	$6,5			# 0x5

	lw	$31,4($sp)
	j	$31
	addiu	$sp,$sp,8

	.set	macro
	.set	reorder
	.end	sceSysconSetWlanSwitchCallback
	.size	sceSysconSetWlanSwitchCallback, .-sceSysconSetWlanSwitchCallback
	.align	2
	.globl	sceSysconSetWlanPowerCallback
	.set	nomips16
	.ent	sceSysconSetWlanPowerCallback
	.type	sceSysconSetWlanPowerCallback, @function
sceSysconSetWlanPowerCallback:
	.frame	$sp,8,$31		# vars= 0, regs= 1/0, args= 0, gp= 0
	.mask	0x80000000,-4
	.fmask	0x00000000,0
	.set	noreorder
	.set	nomacro
	addiu	$sp,$sp,-8
	sw	$31,4($sp)
	jal	_sceSysconSetCallback
	li	$6,9			# 0x9

	lw	$31,4($sp)
	j	$31
	addiu	$sp,$sp,8

	.set	macro
	.set	reorder
	.end	sceSysconSetWlanPowerCallback
	.size	sceSysconSetWlanPowerCallback, .-sceSysconSetWlanPowerCallback
	.align	2
	.globl	sceSysconSetBtSwitchCallback
	.set	nomips16
	.ent	sceSysconSetBtSwitchCallback
	.type	sceSysconSetBtSwitchCallback, @function
sceSysconSetBtSwitchCallback:
	.frame	$sp,8,$31		# vars= 0, regs= 1/0, args= 0, gp= 0
	.mask	0x80000000,-4
	.fmask	0x00000000,0
	.set	noreorder
	.set	nomacro
	addiu	$sp,$sp,-8
	sw	$31,4($sp)
	jal	_sceSysconSetCallback
	li	$6,13			# 0xd

	lw	$31,4($sp)
	j	$31
	addiu	$sp,$sp,8

	.set	macro
	.set	reorder
	.end	sceSysconSetBtSwitchCallback
	.size	sceSysconSetBtSwitchCallback, .-sceSysconSetBtSwitchCallback
	.align	2
	.globl	sceSysconSetBtPowerCallback
	.set	nomips16
	.ent	sceSysconSetBtPowerCallback
	.type	sceSysconSetBtPowerCallback, @function
sceSysconSetBtPowerCallback:
	.frame	$sp,8,$31		# vars= 0, regs= 1/0, args= 0, gp= 0
	.mask	0x80000000,-4
	.fmask	0x00000000,0
	.set	noreorder
	.set	nomacro
	addiu	$sp,$sp,-8
	sw	$31,4($sp)
	jal	_sceSysconSetCallback
	li	$6,12			# 0xc

	lw	$31,4($sp)
	j	$31
	addiu	$sp,$sp,8

	.set	macro
	.set	reorder
	.end	sceSysconSetBtPowerCallback
	.size	sceSysconSetBtPowerCallback, .-sceSysconSetBtPowerCallback
	.align	2
	.globl	sceSysconSetHoldSwitchCallback
	.set	nomips16
	.ent	sceSysconSetHoldSwitchCallback
	.type	sceSysconSetHoldSwitchCallback, @function
sceSysconSetHoldSwitchCallback:
	.frame	$sp,8,$31		# vars= 0, regs= 1/0, args= 0, gp= 0
	.mask	0x80000000,-4
	.fmask	0x00000000,0
	.set	noreorder
	.set	nomacro
	addiu	$sp,$sp,-8
	sw	$31,4($sp)
	jal	_sceSysconSetCallback
	li	$6,6			# 0x6

	lw	$31,4($sp)
	j	$31
	addiu	$sp,$sp,8

	.set	macro
	.set	reorder
	.end	sceSysconSetHoldSwitchCallback
	.size	sceSysconSetHoldSwitchCallback, .-sceSysconSetHoldSwitchCallback
	.align	2
	.globl	sceSysconSetUmdSwitchCallback
	.set	nomips16
	.ent	sceSysconSetUmdSwitchCallback
	.type	sceSysconSetUmdSwitchCallback, @function
sceSysconSetUmdSwitchCallback:
	.frame	$sp,8,$31		# vars= 0, regs= 1/0, args= 0, gp= 0
	.mask	0x80000000,-4
	.fmask	0x00000000,0
	.set	noreorder
	.set	nomacro
	addiu	$sp,$sp,-8
	sw	$31,4($sp)
	jal	_sceSysconSetCallback
	li	$6,7			# 0x7

	lw	$31,4($sp)
	j	$31
	addiu	$sp,$sp,8

	.set	macro
	.set	reorder
	.end	sceSysconSetUmdSwitchCallback
	.size	sceSysconSetUmdSwitchCallback, .-sceSysconSetUmdSwitchCallback
	.align	2
	.globl	sceSyscon_driver_374373A8
	.set	nomips16
	.ent	sceSyscon_driver_374373A8
	.type	sceSyscon_driver_374373A8, @function
sceSyscon_driver_374373A8:
	.frame	$sp,8,$31		# vars= 0, regs= 1/0, args= 0, gp= 0
	.mask	0x80000000,-4
	.fmask	0x00000000,0
	.set	noreorder
	.set	nomacro
	addiu	$sp,$sp,-8
	sw	$31,4($sp)
	jal	_sceSysconSetCallback
	li	$6,16			# 0x10

	lw	$31,4($sp)
	j	$31
	addiu	$sp,$sp,8

	.set	macro
	.set	reorder
	.end	sceSyscon_driver_374373A8
	.size	sceSyscon_driver_374373A8, .-sceSyscon_driver_374373A8
	.align	2
	.globl	sceSyscon_driver_B761D385
	.set	nomips16
	.ent	sceSyscon_driver_B761D385
	.type	sceSyscon_driver_B761D385, @function
sceSyscon_driver_B761D385:
	.frame	$sp,8,$31		# vars= 0, regs= 1/0, args= 0, gp= 0
	.mask	0x80000000,-4
	.fmask	0x00000000,0
	.set	noreorder
	.set	nomacro
	addiu	$sp,$sp,-8
	sw	$31,4($sp)
	jal	_sceSysconSetCallback
	li	$6,17			# 0x11

	lw	$31,4($sp)
	j	$31
	addiu	$sp,$sp,8

	.set	macro
	.set	reorder
	.end	sceSyscon_driver_B761D385
	.size	sceSyscon_driver_B761D385, .-sceSyscon_driver_B761D385
	.align	2
	.globl	sceSyscon_driver_26307D84
	.set	nomips16
	.ent	sceSyscon_driver_26307D84
	.type	sceSyscon_driver_26307D84, @function
sceSyscon_driver_26307D84:
	.frame	$sp,8,$31		# vars= 0, regs= 1/0, args= 0, gp= 0
	.mask	0x80000000,-4
	.fmask	0x00000000,0
	.set	noreorder
	.set	nomacro
	addiu	$sp,$sp,-8
	sw	$31,4($sp)
	jal	_sceSysconSetCallback
	li	$6,18			# 0x12

	lw	$31,4($sp)
	j	$31
	addiu	$sp,$sp,8

	.set	macro
	.set	reorder
	.end	sceSyscon_driver_26307D84
	.size	sceSyscon_driver_26307D84, .-sceSyscon_driver_26307D84
	.align	2
	.globl	sceSyscon_driver_6C388E02
	.set	nomips16
	.ent	sceSyscon_driver_6C388E02
	.type	sceSyscon_driver_6C388E02, @function
sceSyscon_driver_6C388E02:
	.frame	$sp,8,$31		# vars= 0, regs= 1/0, args= 0, gp= 0
	.mask	0x80000000,-4
	.fmask	0x00000000,0
	.set	noreorder
	.set	nomacro
	addiu	$sp,$sp,-8
	sw	$31,4($sp)
	jal	_sceSysconSetCallback
	li	$6,19			# 0x13

	lw	$31,4($sp)
	j	$31
	addiu	$sp,$sp,8

	.set	macro
	.set	reorder
	.end	sceSyscon_driver_6C388E02
	.size	sceSyscon_driver_6C388E02, .-sceSyscon_driver_6C388E02
	.align	2
	.globl	_sceSysconCommonWrite
	.set	nomips16
	.ent	_sceSysconCommonWrite
	.type	_sceSysconCommonWrite, @function
_sceSysconCommonWrite:
	.frame	$sp,104,$31		# vars= 96, regs= 1/0, args= 0, gp= 0
	.mask	0x80000000,-4
	.fmask	0x00000000,0
	addiu	$sp,$sp,-104
	sw	$31,100($sp)
	sb	$5,12($sp)
	sb	$6,13($sp)
	srl	$2,$4,8
	sb	$2,15($sp)
	srl	$2,$4,16
	sb	$2,16($sp)
	srl	$2,$4,24
	sb	$2,17($sp)
	sb	$4,14($sp)
	move	$4,$sp
	.set	noreorder
	.set	nomacro
	jal	sceSysconCmdExec
	move	$5,$0
	.set	macro
	.set	reorder

	move	$3,$0
 #APP
 # 19 "../../include/common/inline.h" 1
	min $2, $2, $3
 # 0 "" 2
 #NO_APP
	lw	$31,100($sp)
	.set	noreorder
	.set	nomacro
	j	$31
	addiu	$sp,$sp,104
	.set	macro
	.set	reorder

	.end	_sceSysconCommonWrite
	.size	_sceSysconCommonWrite, .-_sceSysconCommonWrite
	.align	2
	.globl	sceSysconGetBaryonStatus
	.set	nomips16
	.ent	sceSysconGetBaryonStatus
	.type	sceSysconGetBaryonStatus, @function
sceSysconGetBaryonStatus:
	.frame	$sp,0,$31		# vars= 0, regs= 0/0, args= 0, gp= 0
	.mask	0x00000000,0
	.fmask	0x00000000,0
	.set	noreorder
	.set	nomacro
	lui	$2,%hi(g_4EB0+89)
	j	$31
	lbu	$2,%lo(g_4EB0+89)($2)

	.set	macro
	.set	reorder
	.end	sceSysconGetBaryonStatus
	.size	sceSysconGetBaryonStatus, .-sceSysconGetBaryonStatus
	.align	2
	.globl	sceSysconGetBaryonStatus2
	.set	nomips16
	.ent	sceSysconGetBaryonStatus2
	.type	sceSysconGetBaryonStatus2, @function
sceSysconGetBaryonStatus2:
	.frame	$sp,0,$31		# vars= 0, regs= 0/0, args= 0, gp= 0
	.mask	0x00000000,0
	.fmask	0x00000000,0
	.set	noreorder
	.set	nomacro
	lui	$2,%hi(g_4EB0+91)
	j	$31
	lbu	$2,%lo(g_4EB0+91)($2)

	.set	macro
	.set	reorder
	.end	sceSysconGetBaryonStatus2
	.size	sceSysconGetBaryonStatus2, .-sceSysconGetBaryonStatus2
	.align	2
	.globl	sceSysconIsFalling
	.set	nomips16
	.ent	sceSysconIsFalling
	.type	sceSysconIsFalling, @function
sceSysconIsFalling:
	.frame	$sp,0,$31		# vars= 0, regs= 0/0, args= 0, gp= 0
	.mask	0x00000000,0
	.fmask	0x00000000,0
	.set	noreorder
	.set	nomacro
	lui	$2,%hi(g_4EB0+89)
	lbu	$2,%lo(g_4EB0+89)($2)
	j	$31
	srl	$2,$2,7

	.set	macro
	.set	reorder
	.end	sceSysconIsFalling
	.size	sceSysconIsFalling, .-sceSysconIsFalling
	.align	2
	.globl	sceSysconIsLowBattery
	.set	nomips16
	.ent	sceSysconIsLowBattery
	.type	sceSysconIsLowBattery, @function
sceSysconIsLowBattery:
	.frame	$sp,0,$31		# vars= 0, regs= 0/0, args= 0, gp= 0
	.mask	0x00000000,0
	.fmask	0x00000000,0
	.set	noreorder
	.set	nomacro
	lui	$3,%hi(g_4EB0)
	addiu	$3,$3,%lo(g_4EB0)
	lbu	$4,91($3)
	srl	$4,$4,3
	lbu	$2,89($3)
	srl	$2,$2,5
	or	$2,$4,$2
	j	$31
	andi	$2,$2,0x1

	.set	macro
	.set	reorder
	.end	sceSysconIsLowBattery
	.size	sceSysconIsLowBattery, .-sceSysconIsLowBattery
	.align	2
	.globl	sceSysconGetPowerSwitch
	.set	nomips16
	.ent	sceSysconGetPowerSwitch
	.type	sceSysconGetPowerSwitch, @function
sceSysconGetPowerSwitch:
	.frame	$sp,0,$31		# vars= 0, regs= 0/0, args= 0, gp= 0
	.mask	0x00000000,0
	.fmask	0x00000000,0
	.set	noreorder
	.set	nomacro
	lui	$2,%hi(g_4EB0+89)
	lbu	$2,%lo(g_4EB0+89)($2)
	j	$31
	ext	$2,$2,4,1

	.set	macro
	.set	reorder
	.end	sceSysconGetPowerSwitch
	.size	sceSysconGetPowerSwitch, .-sceSysconGetPowerSwitch
	.align	2
	.globl	sceSysconIsAlarmed
	.set	nomips16
	.ent	sceSysconIsAlarmed
	.type	sceSysconIsAlarmed, @function
sceSysconIsAlarmed:
	.frame	$sp,0,$31		# vars= 0, regs= 0/0, args= 0, gp= 0
	.mask	0x00000000,0
	.fmask	0x00000000,0
	.set	noreorder
	.set	nomacro
	lui	$2,%hi(g_4EB0+89)
	lbu	$2,%lo(g_4EB0+89)($2)
	xori	$2,$2,0x8
	j	$31
	ext	$2,$2,3,1

	.set	macro
	.set	reorder
	.end	sceSysconIsAlarmed
	.size	sceSysconIsAlarmed, .-sceSysconIsAlarmed
	.align	2
	.globl	sceSysconIsAcSupplied
	.set	nomips16
	.ent	sceSysconIsAcSupplied
	.type	sceSysconIsAcSupplied, @function
sceSysconIsAcSupplied:
	.frame	$sp,0,$31		# vars= 0, regs= 0/0, args= 0, gp= 0
	.mask	0x00000000,0
	.fmask	0x00000000,0
	.set	noreorder
	.set	nomacro
	lui	$2,%hi(g_4EB0+89)
	lbu	$2,%lo(g_4EB0+89)($2)
	j	$31
	andi	$2,$2,0x1

	.set	macro
	.set	reorder
	.end	sceSysconIsAcSupplied
	.size	sceSysconIsAcSupplied, .-sceSysconIsAcSupplied
	.align	2
	.globl	sceSysconGetHPConnect
	.set	nomips16
	.ent	sceSysconGetHPConnect
	.type	sceSysconGetHPConnect, @function
sceSysconGetHPConnect:
	.frame	$sp,0,$31		# vars= 0, regs= 0/0, args= 0, gp= 0
	.mask	0x00000000,0
	.fmask	0x00000000,0
	.set	noreorder
	.set	nomacro
	lui	$2,%hi(g_4EB0+64)
	j	$31
	lb	$2,%lo(g_4EB0+64)($2)

	.set	macro
	.set	reorder
	.end	sceSysconGetHPConnect
	.size	sceSysconGetHPConnect, .-sceSysconGetHPConnect
	.align	2
	.globl	sceSysconGetWlanSwitch
	.set	nomips16
	.ent	sceSysconGetWlanSwitch
	.type	sceSysconGetWlanSwitch, @function
sceSysconGetWlanSwitch:
	.frame	$sp,0,$31		# vars= 0, regs= 0/0, args= 0, gp= 0
	.mask	0x00000000,0
	.fmask	0x00000000,0
	.set	noreorder
	.set	nomacro
	lui	$2,%hi(g_4EB0+87)
	lb	$2,%lo(g_4EB0+87)($2)
	bgez	$2,$L342
	nop

	lui	$2,%hi(g_4EB0+65)
	lb	$2,%lo(g_4EB0+65)($2)
$L342:
	j	$31
	nop

	.set	macro
	.set	reorder
	.end	sceSysconGetWlanSwitch
	.size	sceSysconGetWlanSwitch, .-sceSysconGetWlanSwitch
	.align	2
	.globl	sceSyscon_driver_0B51E34D
	.set	nomips16
	.ent	sceSyscon_driver_0B51E34D
	.type	sceSyscon_driver_0B51E34D, @function
sceSyscon_driver_0B51E34D:
	.frame	$sp,0,$31		# vars= 0, regs= 0/0, args= 0, gp= 0
	.mask	0x00000000,0
	.fmask	0x00000000,0
	.set	noreorder
	.set	nomacro
	lui	$3,%hi(g_4EB0)
	addiu	$3,$3,%lo(g_4EB0)
	lb	$2,87($3)
	j	$31
	sb	$4,87($3)

	.set	macro
	.set	reorder
	.end	sceSyscon_driver_0B51E34D
	.size	sceSyscon_driver_0B51E34D, .-sceSyscon_driver_0B51E34D
	.align	2
	.globl	sceSysconGetBtSwitch
	.set	nomips16
	.ent	sceSysconGetBtSwitch
	.type	sceSysconGetBtSwitch, @function
sceSysconGetBtSwitch:
	.frame	$sp,0,$31		# vars= 0, regs= 0/0, args= 0, gp= 0
	.mask	0x00000000,0
	.fmask	0x00000000,0
	.set	noreorder
	.set	nomacro
	lui	$2,%hi(g_4EB0+88)
	lb	$2,%lo(g_4EB0+88)($2)
	bgez	$2,$L346
	nop

	lui	$2,%hi(g_4EB0+66)
	lb	$2,%lo(g_4EB0+66)($2)
$L346:
	j	$31
	nop

	.set	macro
	.set	reorder
	.end	sceSysconGetBtSwitch
	.size	sceSysconGetBtSwitch, .-sceSysconGetBtSwitch
	.align	2
	.globl	sceSyscon_driver_BADF1260
	.set	nomips16
	.ent	sceSyscon_driver_BADF1260
	.type	sceSyscon_driver_BADF1260, @function
sceSyscon_driver_BADF1260:
	.frame	$sp,0,$31		# vars= 0, regs= 0/0, args= 0, gp= 0
	.mask	0x00000000,0
	.fmask	0x00000000,0
	.set	noreorder
	.set	nomacro
	lui	$3,%hi(g_4EB0)
	addiu	$3,$3,%lo(g_4EB0)
	lb	$2,88($3)
	j	$31
	sb	$4,88($3)

	.set	macro
	.set	reorder
	.end	sceSyscon_driver_BADF1260
	.size	sceSyscon_driver_BADF1260, .-sceSyscon_driver_BADF1260
	.align	2
	.globl	sceSysconGetHoldSwitch
	.set	nomips16
	.ent	sceSysconGetHoldSwitch
	.type	sceSysconGetHoldSwitch, @function
sceSysconGetHoldSwitch:
	.frame	$sp,0,$31		# vars= 0, regs= 0/0, args= 0, gp= 0
	.mask	0x00000000,0
	.fmask	0x00000000,0
	.set	noreorder
	.set	nomacro
	lui	$2,%hi(g_4EB0+67)
	j	$31
	lb	$2,%lo(g_4EB0+67)($2)

	.set	macro
	.set	reorder
	.end	sceSysconGetHoldSwitch
	.size	sceSysconGetHoldSwitch, .-sceSysconGetHoldSwitch
	.align	2
	.globl	sceSysconGetUmdSwitch
	.set	nomips16
	.ent	sceSysconGetUmdSwitch
	.type	sceSysconGetUmdSwitch, @function
sceSysconGetUmdSwitch:
	.frame	$sp,0,$31		# vars= 0, regs= 0/0, args= 0, gp= 0
	.mask	0x00000000,0
	.fmask	0x00000000,0
	.set	noreorder
	.set	nomacro
	lui	$2,%hi(g_4EB0+68)
	j	$31
	lb	$2,%lo(g_4EB0+68)($2)

	.set	macro
	.set	reorder
	.end	sceSysconGetUmdSwitch
	.size	sceSysconGetUmdSwitch, .-sceSysconGetUmdSwitch
	.align	2
	.globl	sceSyscon_driver_248335CD
	.set	nomips16
	.ent	sceSyscon_driver_248335CD
	.type	sceSyscon_driver_248335CD, @function
sceSyscon_driver_248335CD:
	.frame	$sp,0,$31		# vars= 0, regs= 0/0, args= 0, gp= 0
	.mask	0x00000000,0
	.fmask	0x00000000,0
	.set	noreorder
	.set	nomacro
	lui	$2,%hi(g_4EB0+70)
	j	$31
	lb	$2,%lo(g_4EB0+70)($2)

	.set	macro
	.set	reorder
	.end	sceSyscon_driver_248335CD
	.size	sceSyscon_driver_248335CD, .-sceSyscon_driver_248335CD
	.align	2
	.globl	sceSyscon_driver_040982CD
	.set	nomips16
	.ent	sceSyscon_driver_040982CD
	.type	sceSyscon_driver_040982CD, @function
sceSyscon_driver_040982CD:
	.frame	$sp,0,$31		# vars= 0, regs= 0/0, args= 0, gp= 0
	.mask	0x00000000,0
	.fmask	0x00000000,0
	.set	noreorder
	.set	nomacro
	lui	$2,%hi(g_4EB0+71)
	j	$31
	lb	$2,%lo(g_4EB0+71)($2)

	.set	macro
	.set	reorder
	.end	sceSyscon_driver_040982CD
	.size	sceSyscon_driver_040982CD, .-sceSyscon_driver_040982CD
	.align	2
	.globl	sceSyscon_driver_97765E27
	.set	nomips16
	.ent	sceSyscon_driver_97765E27
	.type	sceSyscon_driver_97765E27, @function
sceSyscon_driver_97765E27:
	.frame	$sp,0,$31		# vars= 0, regs= 0/0, args= 0, gp= 0
	.mask	0x00000000,0
	.fmask	0x00000000,0
	.set	noreorder
	.set	nomacro
	lui	$2,%hi(g_4EB0+72)
	j	$31
	lb	$2,%lo(g_4EB0+72)($2)

	.set	macro
	.set	reorder
	.end	sceSyscon_driver_97765E27
	.size	sceSyscon_driver_97765E27, .-sceSyscon_driver_97765E27
	.align	2
	.globl	sceSysconGetHRPowerStatus
	.set	nomips16
	.ent	sceSysconGetHRPowerStatus
	.type	sceSysconGetHRPowerStatus, @function
sceSysconGetHRPowerStatus:
	.frame	$sp,0,$31		# vars= 0, regs= 0/0, args= 0, gp= 0
	.mask	0x00000000,0
	.fmask	0x00000000,0
	.set	noreorder
	.set	nomacro
	lui	$2,%hi(g_4EB0+89)
	lbu	$2,%lo(g_4EB0+89)($2)
	j	$31
	ext	$2,$2,2,1

	.set	macro
	.set	reorder
	.end	sceSysconGetHRPowerStatus
	.size	sceSysconGetHRPowerStatus, .-sceSysconGetHRPowerStatus
	.align	2
	.globl	sceSysconGetHRWakeupStatus
	.set	nomips16
	.ent	sceSysconGetHRWakeupStatus
	.type	sceSysconGetHRWakeupStatus, @function
sceSysconGetHRWakeupStatus:
	.frame	$sp,0,$31		# vars= 0, regs= 0/0, args= 0, gp= 0
	.mask	0x00000000,0
	.fmask	0x00000000,0
	.set	noreorder
	.set	nomacro
	lui	$2,%hi(g_4EB0+91)
	lbu	$2,%lo(g_4EB0+91)($2)
	j	$31
	ext	$2,$2,2,1

	.set	macro
	.set	reorder
	.end	sceSysconGetHRWakeupStatus
	.size	sceSysconGetHRWakeupStatus, .-sceSysconGetHRWakeupStatus
	.align	2
	.globl	sceSysconGetWlanPowerStatus
	.set	nomips16
	.ent	sceSysconGetWlanPowerStatus
	.type	sceSysconGetWlanPowerStatus, @function
sceSysconGetWlanPowerStatus:
	.frame	$sp,0,$31		# vars= 0, regs= 0/0, args= 0, gp= 0
	.mask	0x00000000,0
	.fmask	0x00000000,0
	.set	noreorder
	.set	nomacro
	lui	$2,%hi(g_4EB0+89)
	lbu	$2,%lo(g_4EB0+89)($2)
	j	$31
	ext	$2,$2,1,1

	.set	macro
	.set	reorder
	.end	sceSysconGetWlanPowerStatus
	.size	sceSysconGetWlanPowerStatus, .-sceSysconGetWlanPowerStatus
	.align	2
	.globl	sceSysconGetBtPowerStatus
	.set	nomips16
	.ent	sceSysconGetBtPowerStatus
	.type	sceSysconGetBtPowerStatus, @function
sceSysconGetBtPowerStatus:
	.frame	$sp,0,$31		# vars= 0, regs= 0/0, args= 0, gp= 0
	.mask	0x00000000,0
	.fmask	0x00000000,0
	.set	noreorder
	.set	nomacro
	lui	$2,%hi(g_4EB0+91)
	lbu	$2,%lo(g_4EB0+91)($2)
	j	$31
	ext	$2,$2,1,1

	.set	macro
	.set	reorder
	.end	sceSysconGetBtPowerStatus
	.size	sceSysconGetBtPowerStatus, .-sceSysconGetBtPowerStatus
	.align	2
	.globl	sceSyscon_driver_DF20C984
	.set	nomips16
	.ent	sceSyscon_driver_DF20C984
	.type	sceSyscon_driver_DF20C984, @function
sceSyscon_driver_DF20C984:
	.frame	$sp,0,$31		# vars= 0, regs= 0/0, args= 0, gp= 0
	.mask	0x00000000,0
	.fmask	0x00000000,0
	.set	noreorder
	.set	nomacro
	lui	$2,%hi(g_4EB0+91)
	lbu	$2,%lo(g_4EB0+91)($2)
	j	$31
	ext	$2,$2,4,1

	.set	macro
	.set	reorder
	.end	sceSyscon_driver_DF20C984
	.size	sceSyscon_driver_DF20C984, .-sceSyscon_driver_DF20C984
	.align	2
	.globl	sceSysconGetLeptonPowerCtrl
	.set	nomips16
	.ent	sceSysconGetLeptonPowerCtrl
	.type	sceSysconGetLeptonPowerCtrl, @function
sceSysconGetLeptonPowerCtrl:
	.frame	$sp,0,$31		# vars= 0, regs= 0/0, args= 0, gp= 0
	.mask	0x00000000,0
	.fmask	0x00000000,0
	.set	noreorder
	.set	nomacro
	lui	$2,%hi(g_4EB0+74)
	j	$31
	lb	$2,%lo(g_4EB0+74)($2)

	.set	macro
	.set	reorder
	.end	sceSysconGetLeptonPowerCtrl
	.size	sceSysconGetLeptonPowerCtrl, .-sceSysconGetLeptonPowerCtrl
	.align	2
	.globl	sceSysconGetMsPowerCtrl
	.set	nomips16
	.ent	sceSysconGetMsPowerCtrl
	.type	sceSysconGetMsPowerCtrl, @function
sceSysconGetMsPowerCtrl:
	.frame	$sp,0,$31		# vars= 0, regs= 0/0, args= 0, gp= 0
	.mask	0x00000000,0
	.fmask	0x00000000,0
	.set	noreorder
	.set	nomacro
	lui	$2,%hi(g_4EB0+75)
	j	$31
	lb	$2,%lo(g_4EB0+75)($2)

	.set	macro
	.set	reorder
	.end	sceSysconGetMsPowerCtrl
	.size	sceSysconGetMsPowerCtrl, .-sceSysconGetMsPowerCtrl
	.align	2
	.globl	sceSysconGetWlanPowerCtrl
	.set	nomips16
	.ent	sceSysconGetWlanPowerCtrl
	.type	sceSysconGetWlanPowerCtrl, @function
sceSysconGetWlanPowerCtrl:
	.frame	$sp,0,$31		# vars= 0, regs= 0/0, args= 0, gp= 0
	.mask	0x00000000,0
	.fmask	0x00000000,0
	.set	noreorder
	.set	nomacro
	lui	$2,%hi(g_4EB0+76)
	j	$31
	lb	$2,%lo(g_4EB0+76)($2)

	.set	macro
	.set	reorder
	.end	sceSysconGetWlanPowerCtrl
	.size	sceSysconGetWlanPowerCtrl, .-sceSysconGetWlanPowerCtrl
	.align	2
	.globl	sceSysconGetHddPowerCtrl
	.set	nomips16
	.ent	sceSysconGetHddPowerCtrl
	.type	sceSysconGetHddPowerCtrl, @function
sceSysconGetHddPowerCtrl:
	.frame	$sp,0,$31		# vars= 0, regs= 0/0, args= 0, gp= 0
	.mask	0x00000000,0
	.fmask	0x00000000,0
	.set	noreorder
	.set	nomacro
	lui	$2,%hi(g_4EB0+77)
	j	$31
	lb	$2,%lo(g_4EB0+77)($2)

	.set	macro
	.set	reorder
	.end	sceSysconGetHddPowerCtrl
	.size	sceSysconGetHddPowerCtrl, .-sceSysconGetHddPowerCtrl
	.align	2
	.globl	sceSysconGetDvePowerCtrl
	.set	nomips16
	.ent	sceSysconGetDvePowerCtrl
	.type	sceSysconGetDvePowerCtrl, @function
sceSysconGetDvePowerCtrl:
	.frame	$sp,0,$31		# vars= 0, regs= 0/0, args= 0, gp= 0
	.mask	0x00000000,0
	.fmask	0x00000000,0
	.set	noreorder
	.set	nomacro
	lui	$2,%hi(g_4EB0+78)
	j	$31
	lb	$2,%lo(g_4EB0+78)($2)

	.set	macro
	.set	reorder
	.end	sceSysconGetDvePowerCtrl
	.size	sceSysconGetDvePowerCtrl, .-sceSysconGetDvePowerCtrl
	.align	2
	.globl	sceSysconGetBtPowerCtrl
	.set	nomips16
	.ent	sceSysconGetBtPowerCtrl
	.type	sceSysconGetBtPowerCtrl, @function
sceSysconGetBtPowerCtrl:
	.frame	$sp,0,$31		# vars= 0, regs= 0/0, args= 0, gp= 0
	.mask	0x00000000,0
	.fmask	0x00000000,0
	.set	noreorder
	.set	nomacro
	lui	$2,%hi(g_4EB0+79)
	j	$31
	lb	$2,%lo(g_4EB0+79)($2)

	.set	macro
	.set	reorder
	.end	sceSysconGetBtPowerCtrl
	.size	sceSysconGetBtPowerCtrl, .-sceSysconGetBtPowerCtrl
	.align	2
	.globl	sceSysconGetUsbPowerCtrl
	.set	nomips16
	.ent	sceSysconGetUsbPowerCtrl
	.type	sceSysconGetUsbPowerCtrl, @function
sceSysconGetUsbPowerCtrl:
	.frame	$sp,0,$31		# vars= 0, regs= 0/0, args= 0, gp= 0
	.mask	0x00000000,0
	.fmask	0x00000000,0
	.set	noreorder
	.set	nomacro
	lui	$2,%hi(g_4EB0+80)
	j	$31
	lb	$2,%lo(g_4EB0+80)($2)

	.set	macro
	.set	reorder
	.end	sceSysconGetUsbPowerCtrl
	.size	sceSysconGetUsbPowerCtrl, .-sceSysconGetUsbPowerCtrl
	.align	2
	.globl	sceSysconGetTachyonVmePowerCtrl
	.set	nomips16
	.ent	sceSysconGetTachyonVmePowerCtrl
	.type	sceSysconGetTachyonVmePowerCtrl, @function
sceSysconGetTachyonVmePowerCtrl:
	.frame	$sp,0,$31		# vars= 0, regs= 0/0, args= 0, gp= 0
	.mask	0x00000000,0
	.fmask	0x00000000,0
	.set	noreorder
	.set	nomacro
	lui	$2,%hi(g_4EB0+81)
	j	$31
	lb	$2,%lo(g_4EB0+81)($2)

	.set	macro
	.set	reorder
	.end	sceSysconGetTachyonVmePowerCtrl
	.size	sceSysconGetTachyonVmePowerCtrl, .-sceSysconGetTachyonVmePowerCtrl
	.align	2
	.globl	sceSysconGetTachyonAwPowerCtrl
	.set	nomips16
	.ent	sceSysconGetTachyonAwPowerCtrl
	.type	sceSysconGetTachyonAwPowerCtrl, @function
sceSysconGetTachyonAwPowerCtrl:
	.frame	$sp,0,$31		# vars= 0, regs= 0/0, args= 0, gp= 0
	.mask	0x00000000,0
	.fmask	0x00000000,0
	.set	noreorder
	.set	nomacro
	lui	$2,%hi(g_4EB0+82)
	j	$31
	lb	$2,%lo(g_4EB0+82)($2)

	.set	macro
	.set	reorder
	.end	sceSysconGetTachyonAwPowerCtrl
	.size	sceSysconGetTachyonAwPowerCtrl, .-sceSysconGetTachyonAwPowerCtrl
	.align	2
	.globl	sceSysconGetTachyonAvcPowerCtrl
	.set	nomips16
	.ent	sceSysconGetTachyonAvcPowerCtrl
	.type	sceSysconGetTachyonAvcPowerCtrl, @function
sceSysconGetTachyonAvcPowerCtrl:
	.frame	$sp,0,$31		# vars= 0, regs= 0/0, args= 0, gp= 0
	.mask	0x00000000,0
	.fmask	0x00000000,0
	.set	noreorder
	.set	nomacro
	lui	$2,%hi(g_4EB0+83)
	j	$31
	lb	$2,%lo(g_4EB0+83)($2)

	.set	macro
	.set	reorder
	.end	sceSysconGetTachyonAvcPowerCtrl
	.size	sceSysconGetTachyonAvcPowerCtrl, .-sceSysconGetTachyonAvcPowerCtrl
	.align	2
	.globl	sceSysconGetLcdPowerCtrl
	.set	nomips16
	.ent	sceSysconGetLcdPowerCtrl
	.type	sceSysconGetLcdPowerCtrl, @function
sceSysconGetLcdPowerCtrl:
	.frame	$sp,0,$31		# vars= 0, regs= 0/0, args= 0, gp= 0
	.mask	0x00000000,0
	.fmask	0x00000000,0
	.set	noreorder
	.set	nomacro
	lui	$2,%hi(g_4EB0+84)
	j	$31
	lb	$2,%lo(g_4EB0+84)($2)

	.set	macro
	.set	reorder
	.end	sceSysconGetLcdPowerCtrl
	.size	sceSysconGetLcdPowerCtrl, .-sceSysconGetLcdPowerCtrl
	.align	2
	.globl	sceSysconGetHRPowerCtrl
	.set	nomips16
	.ent	sceSysconGetHRPowerCtrl
	.type	sceSysconGetHRPowerCtrl, @function
sceSysconGetHRPowerCtrl:
	.frame	$sp,0,$31		# vars= 0, regs= 0/0, args= 0, gp= 0
	.mask	0x00000000,0
	.fmask	0x00000000,0
	.set	noreorder
	.set	nomacro
	lui	$2,%hi(g_4EB0+85)
	j	$31
	lb	$2,%lo(g_4EB0+85)($2)

	.set	macro
	.set	reorder
	.end	sceSysconGetHRPowerCtrl
	.size	sceSysconGetHRPowerCtrl, .-sceSysconGetHRPowerCtrl
	.align	2
	.globl	sceSysconGetWlanLedCtrl
	.set	nomips16
	.ent	sceSysconGetWlanLedCtrl
	.type	sceSysconGetWlanLedCtrl, @function
sceSysconGetWlanLedCtrl:
	.frame	$sp,0,$31		# vars= 0, regs= 0/0, args= 0, gp= 0
	.mask	0x00000000,0
	.fmask	0x00000000,0
	.set	noreorder
	.set	nomacro
	lui	$2,%hi(g_4EB0+86)
	j	$31
	lb	$2,%lo(g_4EB0+86)($2)

	.set	macro
	.set	reorder
	.end	sceSysconGetWlanLedCtrl
	.size	sceSysconGetWlanLedCtrl, .-sceSysconGetWlanLedCtrl
	.align	2
	.globl	_sceSysconSetCallback
	.set	nomips16
	.ent	_sceSysconSetCallback
	.type	_sceSysconSetCallback, @function
_sceSysconSetCallback:
	.frame	$sp,16,$31		# vars= 0, regs= 4/0, args= 0, gp= 0
	.mask	0x80070000,-4
	.fmask	0x00000000,0
	addiu	$sp,$sp,-16
	sw	$31,12($sp)
	sw	$18,8($sp)
	sw	$17,4($sp)
	sw	$16,0($sp)
	move	$17,$4
	move	$18,$5
	.set	noreorder
	.set	nomacro
	jal	sceKernelCpuSuspendIntr
	move	$16,$6
	.set	macro
	.set	reorder

	sll	$3,$16,2
	sll	$16,$16,4
	subu	$16,$16,$3
	lui	$3,%hi(g_4EB0)
	addiu	$3,$3,%lo(g_4EB0)
	addu	$16,$16,$3
	sw	$17,92($16)
	sw	$18,100($16)
 #APP
 # 53 "../../include/common/registers.h" 1
	move $3, $gp
 # 0 "" 2
 #NO_APP
	sw	$3,96($16)
	.set	noreorder
	.set	nomacro
	jal	sceKernelCpuResumeIntr
	move	$4,$2
	.set	macro
	.set	reorder

	move	$2,$0
	lw	$31,12($sp)
	lw	$18,8($sp)
	lw	$17,4($sp)
	lw	$16,0($sp)
	.set	noreorder
	.set	nomacro
	j	$31
	addiu	$sp,$sp,16
	.set	macro
	.set	reorder

	.end	_sceSysconSetCallback
	.size	_sceSysconSetCallback, .-_sceSysconSetCallback
	.align	2
	.globl	_sceSysconPacketStart
	.set	nomips16
	.ent	_sceSysconPacketStart
	.type	_sceSysconPacketStart, @function
_sceSysconPacketStart:
	.frame	$sp,8,$31		# vars= 0, regs= 2/0, args= 0, gp= 0
	.mask	0x80010000,-4
	.fmask	0x00000000,0
	.set	noreorder
	.set	nomacro
	addiu	$sp,$sp,-8
	sw	$31,4($sp)
	sw	$16,0($sp)
	jal	sceKernelGetSystemTimeLow
	move	$16,$4

	sw	$2,56($16)
	lui	$2,%hi(g_4EB0+36)
	lw	$2,%lo(g_4EB0+36)($2)
	beq	$2,$0,$L373
	nop

	lw	$2,4($2)
	jalr	$2
	move	$4,$16

$L373:
	jal	sceGpioPortRead
	nop

	jal	sceGpioPortClear
	li	$4,8			# 0x8

	li	$2,-1101529088			# 0xffffffffbe580000
	lw	$2,12($2)
	andi	$2,$2,0x4
	beq	$2,$0,$L374
	li	$2,-1101529088			# 0xffffffffbe580000

$L379:
	lw	$3,8($2)
	lw	$3,12($2)
	andi	$3,$3,0x4
	bne	$3,$0,$L379
	nop

$L374:
	li	$2,-1101529088			# 0xffffffffbe580000
	lw	$3,8($2)
	li	$3,3			# 0x3
	sw	$3,32($2)
	lw	$2,4($16)
	ext	$2,$2,18,1
	bnel	$2,$0,$L376
	lbu	$3,12($16)

	move	$2,$16
	move	$3,$0
	li	$4,-1101529088			# 0xffffffffbe580000
$L378:
	lw	$5,12($4)
	lbu	$5,12($2)
	sll	$5,$5,8
	lbu	$6,13($2)
	or	$5,$5,$6
	sw	$5,8($4)
	addiu	$3,$3,2
	lbu	$5,13($16)
	addiu	$5,$5,1
	slt	$5,$3,$5
	bne	$5,$0,$L378
	addiu	$2,$2,2

	j	$L382
	li	$3,6			# 0x6

$L376:
	sll	$3,$3,8
	ori	$3,$3,0x2
	li	$2,-1101529088			# 0xffffffffbe580000
	sw	$3,8($2)
	lbu	$3,12($16)
	addiu	$3,$3,2
	sll	$3,$3,8
	xori	$3,$3,0xffff
	sw	$3,8($2)
	li	$3,6			# 0x6
$L382:
	li	$2,-1101529088			# 0xffffffffbe580000
	sw	$3,4($2)
	jal	sceGpioPortSet
	li	$4,8			# 0x8

	lw	$3,4($16)
	li	$2,131072			# 0x20000
	or	$2,$3,$2
	sw	$2,4($16)
	move	$2,$0
	lw	$31,4($sp)
	lw	$16,0($sp)
	j	$31
	addiu	$sp,$sp,8

	.set	macro
	.set	reorder
	.end	_sceSysconPacketStart
	.size	_sceSysconPacketStart, .-_sceSysconPacketStart
	.align	2
	.globl	_sceSysconPacketEnd
	.set	nomips16
	.ent	_sceSysconPacketEnd
	.type	_sceSysconPacketEnd, @function
_sceSysconPacketEnd:
	.frame	$sp,16,$31		# vars= 0, regs= 3/0, args= 0, gp= 0
	.mask	0x80030000,-4
	.fmask	0x00000000,0
	.set	noreorder
	.set	nomacro
	addiu	$sp,$sp,-16
	sw	$31,12($sp)
	sw	$17,8($sp)
	sw	$16,4($sp)
	move	$16,$4
	li	$2,-1101529088			# 0xffffffffbe580000
	lw	$2,12($2)
	andi	$2,$2,0x4
	bne	$2,$0,$L384
	move	$17,$0

	li	$2,-1			# 0xffffffffffffffff
	sb	$2,28($4)
	lw	$3,4($4)
	li	$2,1048576			# 0x100000
	or	$2,$3,$2
	sw	$2,4($4)
	sb	$0,29($4)
	li	$17,-1			# 0xffffffffffffffff
$L384:
	li	$2,-1101529088			# 0xffffffffbe580000
	lw	$2,12($2)
	andi	$2,$2,0x1
	bne	$2,$0,$L402
	li	$2,-1101529088			# 0xffffffffbe580000

	lw	$3,4($16)
	li	$2,2097152			# 0x200000
	or	$2,$3,$2
	sw	$2,4($16)
	li	$17,-1			# 0xffffffffffffffff
	li	$2,-1101529088			# 0xffffffffbe580000
$L402:
	lw	$2,24($2)
	andi	$2,$2,0x1
	beq	$2,$0,$L403
	li	$2,-1101529088			# 0xffffffffbe580000

	li	$3,1			# 0x1
	sw	$3,32($2)
	lw	$3,4($16)
	li	$2,4194304			# 0x400000
	or	$2,$3,$2
	sw	$2,4($16)
	li	$2,-1101529088			# 0xffffffffbe580000
$L403:
	lw	$2,12($2)
	andi	$2,$2,0x4
	beq	$2,$0,$L387
	move	$6,$16

	move	$2,$16
	li	$4,2			# 0x2
	j	$L388
	li	$5,-1101529088			# 0xffffffffbe580000

$L390:
	lw	$3,12($5)
	addiu	$4,$4,2
	andi	$3,$3,0x4
	beq	$3,$0,$L387
	addiu	$2,$2,2

$L388:
	lw	$3,8($5)
	bne	$6,$2,$L389
	andi	$3,$3,0xffff

	srl	$17,$3,8
	sb	$17,28($2)
	j	$L390
	sb	$3,29($2)

$L389:
	srl	$7,$3,8
	sb	$7,28($2)
	sb	$3,29($2)
	slt	$3,$4,16
	bne	$3,$0,$L390
	nop

$L387:
	li	$3,4			# 0x4
	li	$2,-1101529088			# 0xffffffffbe580000
	sw	$3,4($2)
	jal	sceGpioPortClear
	li	$4,8			# 0x8

	lui	$2,%hi(g_4EB0+36)
	lw	$2,%lo(g_4EB0+36)($2)
	beq	$2,$0,$L391
	nop

	lw	$2,8($2)
	jalr	$2
	move	$4,$16

$L391:
	bltzl	$17,$L404
	lw	$3,4($16)

	lbu	$6,29($16)
	sltu	$2,$6,3
	bne	$2,$0,$L396
	sltu	$2,$6,16

	beql	$2,$0,$L397
	li	$17,-2			# 0xfffffffffffffffe

	beq	$6,$0,$L393
	move	$3,$0

	blez	$6,$L393
	move	$5,$6

	move	$2,$0
$L394:
	addu	$4,$16,$2
	lbu	$4,28($4)
	addu	$3,$4,$3
	addiu	$2,$2,1
	slt	$4,$2,$5
	bne	$4,$0,$L394
	andi	$3,$3,0xff

$L393:
	addu	$6,$16,$6
	lbu	$2,28($6)
	nor	$3,$0,$3
	andi	$3,$3,0xff
	xor	$3,$2,$3
	li	$2,-2			# 0xfffffffffffffffe
	j	$L392
	movn	$17,$2,$3

$L396:
	li	$17,-2			# 0xfffffffffffffffe
$L397:
$L392:
	lw	$3,4($16)
$L404:
	li	$2,-196608			# 0xfffffffffffd0000
	ori	$2,$2,0xffff
	and	$2,$3,$2
	sw	$2,4($16)
	move	$2,$17
	lw	$31,12($sp)
	lw	$17,8($sp)
	lw	$16,4($sp)
	j	$31
	addiu	$sp,$sp,16

	.set	macro
	.set	reorder
	.end	_sceSysconPacketEnd
	.size	_sceSysconPacketEnd, .-_sceSysconPacketEnd
	.align	2
	.globl	sceSysconGetTimeStamp
	.set	nomips16
	.ent	sceSysconGetTimeStamp
	.type	sceSysconGetTimeStamp, @function
sceSysconGetTimeStamp:
	.frame	$sp,104,$31		# vars= 96, regs= 2/0, args= 0, gp= 0
	.mask	0x80010000,-4
	.fmask	0x00000000,0
	.set	noreorder
	.set	nomacro
	addiu	$sp,$sp,-104
	sw	$31,100($sp)
	sw	$16,96($sp)
	beq	$4,$0,$L407
	move	$16,$4

	li	$2,17			# 0x11
	sb	$2,12($sp)
	li	$2,2			# 0x2
	sb	$2,13($sp)
	move	$4,$sp
	jal	sceSysconCmdExec
	move	$5,$0

	bltz	$2,$L406
	move	$4,$16

	addiu	$5,$sp,31
	jal	memcpy
	li	$6,12			# 0xc

	sb	$0,12($16)
	j	$L406
	move	$2,$0

$L407:
	li	$2,-2147483648			# 0xffffffff80000000
	addiu	$2,$2,259
$L406:
	lw	$31,100($sp)
	lw	$16,96($sp)
	j	$31
	addiu	$sp,$sp,104

	.set	macro
	.set	reorder
	.end	sceSysconGetTimeStamp
	.size	sceSysconGetTimeStamp, .-sceSysconGetTimeStamp
	.align	2
	.globl	sceSysconWriteScratchPad
	.set	nomips16
	.ent	sceSysconWriteScratchPad
	.type	sceSysconWriteScratchPad, @function
sceSysconWriteScratchPad:
	.frame	$sp,104,$31		# vars= 96, regs= 1/0, args= 0, gp= 0
	.mask	0x80000000,-4
	.fmask	0x00000000,0
	addiu	$sp,$sp,-104
	addiu	$2,$6,-1
	sltu	$2,$2,2
	.set	noreorder
	.set	nomacro
	bne	$2,$0,$L409
	sw	$31,100($sp)
	.set	macro
	.set	reorder

	li	$2,4			# 0x4
	.set	noreorder
	.set	nomacro
	beq	$6,$2,$L409
	li	$2,8			# 0x8
	.set	macro
	.set	reorder

	.set	noreorder
	.set	nomacro
	bne	$6,$2,$L412
	li	$2,-2147483648			# 0xffffffff80000000
	.set	macro
	.set	reorder

$L409:
	.set	noreorder
	bne	$6,$0,1f
	divu	$0,$4,$6
	break	7
	.set	reorder
1:
	mfhi	$2
	.set	noreorder
	.set	nomacro
	bne	$2,$0,$L413
	li	$2,-2147483648			# 0xffffffff80000000
	.set	macro
	.set	reorder

	addu	$2,$6,$4
	sltu	$2,$2,33
	.set	noreorder
	.set	nomacro
	beq	$2,$0,$L414
	li	$2,35			# 0x23
	.set	macro
	.set	reorder

	sb	$2,12($sp)
	addiu	$2,$6,3
	sb	$2,13($sp)
	sll	$4,$4,2
	andi	$4,$4,0x00ff
	addiu	$7,$6,-8
	sltu	$7,$7,2
	li	$2,3			# 0x3
	ext	$3,$6,1,8
	movz	$2,$3,$7
	or	$2,$2,$4
	sb	$2,14($sp)
	.set	noreorder
	.set	nomacro
	jal	memcpy
	addiu	$4,$sp,15
	.set	macro
	.set	reorder

	move	$4,$sp
	.set	noreorder
	.set	nomacro
	jal	sceSysconCmdExec
	move	$5,$0
	.set	macro
	.set	reorder

	move	$3,$0
 #APP
 # 19 "../../include/common/inline.h" 1
	min $2, $2, $3
 # 0 "" 2
 #NO_APP
	.set	noreorder
	.set	nomacro
	j	$L416
	lw	$31,100($sp)
	.set	macro
	.set	reorder

$L412:
	.set	noreorder
	.set	nomacro
	j	$L410
	addiu	$2,$2,260
	.set	macro
	.set	reorder

$L413:
	.set	noreorder
	.set	nomacro
	j	$L410
	addiu	$2,$2,258
	.set	macro
	.set	reorder

$L414:
	li	$2,-2147483648			# 0xffffffff80000000
	addiu	$2,$2,258
$L410:
	lw	$31,100($sp)
$L416:
	.set	noreorder
	.set	nomacro
	j	$31
	addiu	$sp,$sp,104
	.set	macro
	.set	reorder

	.end	sceSysconWriteScratchPad
	.size	sceSysconWriteScratchPad, .-sceSysconWriteScratchPad
	.align	2
	.globl	sceSysconReadScratchPad
	.set	nomips16
	.ent	sceSysconReadScratchPad
	.type	sceSysconReadScratchPad, @function
sceSysconReadScratchPad:
	.frame	$sp,112,$31		# vars= 96, regs= 3/0, args= 0, gp= 0
	.mask	0x80030000,-4
	.fmask	0x00000000,0
	.set	noreorder
	.set	nomacro
	addiu	$sp,$sp,-112
	sw	$31,108($sp)
	sw	$17,104($sp)
	sw	$16,100($sp)
	move	$17,$5
	addiu	$2,$6,-1
	sltu	$2,$2,2
	bne	$2,$0,$L418
	move	$16,$6

	li	$2,4			# 0x4
	beq	$6,$2,$L418
	li	$2,8			# 0x8

	bne	$6,$2,$L421
	li	$2,-2147483648			# 0xffffffff80000000

$L418:
	bne	$16,$0,1f
	divu	$0,$4,$16
	break	7
1:
	mfhi	$2
	bne	$2,$0,$L422
	li	$2,-2147483648			# 0xffffffff80000000

	addu	$2,$16,$4
	sltu	$2,$2,33
	beq	$2,$0,$L423
	li	$2,36			# 0x24

	sb	$2,12($sp)
	li	$2,3			# 0x3
	sb	$2,13($sp)
	srl	$2,$16,1
	xori	$3,$2,0x4
	sltu	$3,$0,$3
	sll	$4,$4,2
	or	$4,$3,$4
	andi	$2,$2,0x00ff
	li	$3,3			# 0x3
	movz	$2,$3,$4
	sb	$2,14($sp)
	move	$4,$sp
	jal	sceSysconCmdExec
	move	$5,$0

	bltz	$2,$L419
	move	$4,$17

	addiu	$5,$sp,31
	jal	memcpy
	move	$6,$16

	j	$L419
	move	$2,$0

$L421:
	j	$L419
	addiu	$2,$2,260

$L422:
	j	$L419
	addiu	$2,$2,258

$L423:
	li	$2,-2147483648			# 0xffffffff80000000
	addiu	$2,$2,258
$L419:
	lw	$31,108($sp)
	lw	$17,104($sp)
	lw	$16,100($sp)
	j	$31
	addiu	$sp,$sp,112

	.set	macro
	.set	reorder
	.end	sceSysconReadScratchPad
	.size	sceSysconReadScratchPad, .-sceSysconReadScratchPad
	.align	2
	.globl	sceSysconSendSetParam
	.set	nomips16
	.ent	sceSysconSendSetParam
	.type	sceSysconSendSetParam, @function
sceSysconSendSetParam:
	.frame	$sp,112,$31		# vars= 96, regs= 3/0, args= 0, gp= 0
	.mask	0x80030000,-4
	.fmask	0x00000000,0
	addiu	$sp,$sp,-112
	sw	$31,108($sp)
	sw	$17,104($sp)
	sw	$16,100($sp)
	move	$17,$4
	.set	noreorder
	.set	nomacro
	jal	_sceSysconGetBaryonVersion
	move	$16,$5
	.set	macro
	.set	reorder

	ext	$3,$2,16,8
	slt	$3,$3,18
	.set	noreorder
	.set	nomacro
	beq	$3,$0,$L426
	li	$3,1			# 0x1
	.set	macro
	.set	reorder

	andi	$2,$2,0xff
	bne	$2,$3,$L427
	jal	_sceSysconGetBaryonTimeStamp
	sltu	$4,$3,47
	.set	noreorder
	.set	nomacro
	beql	$4,$0,$L434
	li	$2,37			# 0x25
	.set	macro
	.set	reorder

	li	$4,46			# 0x2e
	.set	noreorder
	.set	nomacro
	bne	$3,$4,$L427
	li	$3,-1354235904			# 0xffffffffaf480000
	.set	macro
	.set	reorder

	ori	$3,$3,0x829a
	sltu	$2,$2,$3
	.set	noreorder
	.set	nomacro
	beq	$2,$0,$L434
	li	$2,37			# 0x25
	.set	macro
	.set	reorder

$L427:
	.set	noreorder
	.set	nomacro
	bne	$17,$0,$L431
	li	$2,37			# 0x25
	.set	macro
	.set	reorder

	sb	$2,12($sp)
	li	$2,10			# 0xa
	sb	$2,13($sp)
	addiu	$4,$sp,14
	move	$5,$16
	.set	noreorder
	.set	nomacro
	jal	memcpy
	li	$6,8			# 0x8
	.set	macro
	.set	reorder

	.set	noreorder
	.set	nomacro
	j	$L432
	move	$4,$sp
	.set	macro
	.set	reorder

$L426:
	li	$2,37			# 0x25
$L434:
	sb	$2,12($sp)
	li	$2,11			# 0xb
	sb	$2,13($sp)
	addiu	$4,$sp,14
	move	$5,$16
	.set	noreorder
	.set	nomacro
	jal	memcpy
	li	$6,8			# 0x8
	.set	macro
	.set	reorder

	sb	$17,22($sp)
	move	$4,$sp
$L432:
	.set	noreorder
	.set	nomacro
	jal	sceSysconCmdExec
	move	$5,$0
	.set	macro
	.set	reorder

	move	$3,$0
 #APP
 # 19 "../../include/common/inline.h" 1
	min $2, $2, $3
 # 0 "" 2
 #NO_APP
	.set	noreorder
	.set	nomacro
	j	$L433
	lw	$31,108($sp)
	.set	macro
	.set	reorder

$L431:
	li	$2,-2145058816			# 0xffffffff80250000
	addiu	$2,$2,17
	lw	$31,108($sp)
$L433:
	lw	$17,104($sp)
	lw	$16,100($sp)
	.set	noreorder
	.set	nomacro
	j	$31
	addiu	$sp,$sp,112
	.set	macro
	.set	reorder

	.end	sceSysconSendSetParam
	.size	sceSysconSendSetParam, .-sceSysconSendSetParam
	.align	2
	.globl	sceSysconReceiveSetParam
	.set	nomips16
	.ent	sceSysconReceiveSetParam
	.type	sceSysconReceiveSetParam, @function
sceSysconReceiveSetParam:
	.frame	$sp,112,$31		# vars= 96, regs= 3/0, args= 0, gp= 0
	.mask	0x80030000,-4
	.fmask	0x00000000,0
	.set	noreorder
	.set	nomacro
	addiu	$sp,$sp,-112
	sw	$31,108($sp)
	sw	$17,104($sp)
	sw	$16,100($sp)
	move	$16,$4
	jal	_sceSysconGetBaryonVersion
	move	$17,$5

	ext	$3,$2,16,8
	slt	$3,$3,18
	beql	$3,$0,$L442
	li	$2,38			# 0x26

	andi	$2,$2,0xff
	li	$3,1			# 0x1
	bne	$2,$3,$L437
	nop

	jal	_sceSysconGetBaryonTimeStamp
	nop

	sltu	$4,$3,47
	beql	$4,$0,$L442
	li	$2,38			# 0x26

	li	$4,46			# 0x2e
	bne	$3,$4,$L437
	li	$3,-1354235904			# 0xffffffffaf480000

	ori	$3,$3,0x829a
	sltu	$2,$2,$3
	beq	$2,$0,$L436
	li	$2,38			# 0x26

$L437:
	bne	$16,$0,$L441
	li	$2,38			# 0x26

	sb	$2,12($sp)
	li	$2,2			# 0x2
	j	$L440
	sb	$2,13($sp)

$L436:
$L442:
	sb	$2,12($sp)
	li	$2,3			# 0x3
	sb	$2,13($sp)
	sb	$16,14($sp)
$L440:
	move	$4,$sp
	jal	sceSysconCmdExec
	move	$5,$0

	bltz	$2,$L439
	move	$4,$17

	addiu	$5,$sp,31
	jal	memcpy
	li	$6,8			# 0x8

	j	$L439
	move	$2,$0

$L441:
	li	$2,-2145058816			# 0xffffffff80250000
	addiu	$2,$2,17
$L439:
	lw	$31,108($sp)
	lw	$17,104($sp)
	lw	$16,100($sp)
	j	$31
	addiu	$sp,$sp,112

	.set	macro
	.set	reorder
	.end	sceSysconReceiveSetParam
	.size	sceSysconReceiveSetParam, .-sceSysconReceiveSetParam
	.align	2
	.globl	sceSysconCtrlTachyonWDT
	.set	nomips16
	.ent	sceSysconCtrlTachyonWDT
	.type	sceSysconCtrlTachyonWDT, @function
sceSysconCtrlTachyonWDT:
	.frame	$sp,8,$31		# vars= 0, regs= 1/0, args= 0, gp= 0
	.mask	0x80000000,-4
	.fmask	0x00000000,0
	.set	noreorder
	.set	nomacro
	addiu	$sp,$sp,-8
	slt	$2,$4,128
	beq	$2,$0,$L446
	sw	$31,4($sp)

	ori	$2,$4,0x80
	movz	$2,$0,$4
	move	$4,$2
	li	$5,49			# 0x31
	jal	_sceSysconCommonWrite
	li	$6,3			# 0x3

	j	$L448
	lw	$31,4($sp)

$L446:
	li	$2,-2147483648			# 0xffffffff80000000
	addiu	$2,$2,510
	lw	$31,4($sp)
$L448:
	j	$31
	addiu	$sp,$sp,8

	.set	macro
	.set	reorder
	.end	sceSysconCtrlTachyonWDT
	.size	sceSysconCtrlTachyonWDT, .-sceSysconCtrlTachyonWDT
	.align	2
	.globl	sceSysconResetDevice
	.set	nomips16
	.ent	sceSysconResetDevice
	.type	sceSysconResetDevice, @function
sceSysconResetDevice:
	.frame	$sp,16,$31		# vars= 0, regs= 4/0, args= 0, gp= 0
	.mask	0x80070000,-4
	.fmask	0x00000000,0
	.set	noreorder
	.set	nomacro
	addiu	$sp,$sp,-16
	sw	$31,12($sp)
	sw	$18,8($sp)
	sw	$17,4($sp)
	sw	$16,0($sp)
	move	$16,$4
	li	$2,1			# 0x1
	bne	$4,$2,$L450
	move	$18,$5

	beq	$5,$2,$L451
	li	$2,2			# 0x2

	beq	$5,$2,$L451
	li	$16,65			# 0x41

	j	$L454
	li	$17,-2147483648			# 0xffffffff80000000

$L450:
	ori	$2,$4,0x80
	movn	$16,$2,$5
$L451:
	move	$4,$16
	li	$5,50			# 0x32
	jal	_sceSysconCommonWrite
	li	$6,3			# 0x3

	bltz	$2,$L452
	move	$17,$2

	li	$2,4			# 0x4
	bne	$16,$2,$L455
	move	$2,$17

	bne	$18,$0,$L456
	lw	$31,12($sp)

	jal	sceSysconGetWlanLedCtrl
	nop

	li	$3,1			# 0x1
	bnel	$2,$3,$L455
	move	$2,$17

	li	$4,1			# 0x1
	jal	sceSysconCtrlLED
	li	$5,1			# 0x1

	j	$L455
	move	$2,$17

$L454:
	addiu	$17,$17,263
$L452:
	move	$2,$17
$L455:
	lw	$31,12($sp)
$L456:
	lw	$18,8($sp)
	lw	$17,4($sp)
	lw	$16,0($sp)
	j	$31
	addiu	$sp,$sp,16

	.set	macro
	.set	reorder
	.end	sceSysconResetDevice
	.size	sceSysconResetDevice, .-sceSysconResetDevice
	.align	2
	.globl	sceSyscon_driver_12518439
	.set	nomips16
	.ent	sceSyscon_driver_12518439
	.type	sceSyscon_driver_12518439, @function
sceSyscon_driver_12518439:
	.frame	$sp,8,$31		# vars= 0, regs= 2/0, args= 0, gp= 0
	.mask	0x80010000,-4
	.fmask	0x00000000,0
	.set	noreorder
	.set	nomacro
	addiu	$sp,$sp,-8
	sw	$31,4($sp)
	sw	$16,0($sp)
	jal	_sceSysconGetBaryonVersion
	move	$16,$4

	sra	$2,$2,16
	andi	$2,$2,0xf0
	slt	$2,$2,48
	bne	$2,$0,$L458
	li	$2,65536			# 0x10000

	li	$3,53			# 0x35
	sb	$3,16320($2)
	li	$3,-1			# 0xffffffffffffffff
	sb	$3,16325($2)
	li	$3,4			# 0x4
	sb	$3,16321($2)
	srl	$3,$16,8
	addiu	$4,$16,57
	addu	$4,$4,$3
	nor	$4,$0,$4
	sb	$4,16324($2)
	sb	$16,16322($2)
	j	$L459
	sb	$3,16323($2)

$L458:
	li	$3,53			# 0x35
	sb	$3,16320($2)
	li	$3,-1			# 0xffffffffffffffff
	sb	$3,16323($2)
	li	$3,2			# 0x2
	sb	$3,16321($2)
	li	$3,-56			# 0xffffffffffffffc8
	sb	$3,16322($2)
$L459:
	li	$4,65536			# 0x10000
	addiu	$4,$4,16320
	move	$5,$0
	jal	sub_2D08
	move	$6,$0

	move	$2,$0
	lw	$31,4($sp)
	lw	$16,0($sp)
	j	$31
	addiu	$sp,$sp,8

	.set	macro
	.set	reorder
	.end	sceSyscon_driver_12518439
	.size	sceSyscon_driver_12518439, .-sceSyscon_driver_12518439
	.align	2
	.globl	sceSysconPowerSuspend
	.set	nomips16
	.ent	sceSysconPowerSuspend
	.type	sceSysconPowerSuspend, @function
sceSysconPowerSuspend:
	.frame	$sp,24,$31		# vars= 0, regs= 5/0, args= 0, gp= 0
	.mask	0x800f0000,-4
	.fmask	0x00000000,0
	.set	noreorder
	.set	nomacro
	addiu	$sp,$sp,-24
	sw	$31,20($sp)
	sw	$19,16($sp)
	sw	$18,12($sp)
	sw	$17,8($sp)
	sw	$16,4($sp)
	move	$18,$4
	jal	sceKernelSm1ReferOperations
	move	$19,$5

	move	$16,$2
	beq	$2,$0,$L461
	move	$17,$0

	lw	$2,16($2)
	jalr	$2
	nop

	li	$3,21325			# 0x534d
	bne	$2,$3,$L461
	nop

	lw	$2,16($16)
	jalr	$2
	nop

	slt	$17,$2,6
$L461:
	jal	_sceSysconGetBaryonVersion
	nop

	sra	$2,$2,16
	andi	$2,$2,0xf0
	slt	$2,$2,48
	bne	$2,$0,$L462
	li	$2,65536			# 0x10000

	li	$3,54			# 0x36
	sb	$3,16320($2)
	li	$3,-1			# 0xffffffffffffffff
	sb	$3,16325($2)
	li	$3,4			# 0x4
	sb	$3,16321($2)
	srl	$3,$18,8
	addiu	$4,$18,58
	addu	$4,$4,$3
	nor	$4,$0,$4
	sb	$4,16324($2)
	sb	$18,16322($2)
	j	$L463
	sb	$3,16323($2)

$L462:
	li	$3,54			# 0x36
	sb	$3,16320($2)
	addiu	$3,$18,57
	nor	$3,$0,$3
	sb	$3,16323($2)
	li	$3,3			# 0x3
	sb	$3,16321($2)
	sb	$18,16322($2)
$L463:
	li	$4,65536			# 0x10000
	addiu	$4,$4,16320
	move	$5,$17
	jal	sub_2D08
	move	$6,$19

	move	$2,$0
	lw	$31,20($sp)
	lw	$19,16($sp)
	lw	$18,12($sp)
	lw	$17,8($sp)
	lw	$16,4($sp)
	j	$31
	addiu	$sp,$sp,24

	.set	macro
	.set	reorder
	.end	sceSysconPowerSuspend
	.size	sceSysconPowerSuspend, .-sceSysconPowerSuspend
	.align	2
	.globl	sub_2D08
	.set	nomips16
	.ent	sub_2D08
	.type	sub_2D08, @function
sub_2D08:
	.frame	$sp,16,$31		# vars= 0, regs= 4/0, args= 0, gp= 0
	.mask	0x80070000,-4
	.fmask	0x00000000,0
	.set	noreorder
	.set	nomacro
	addiu	$sp,$sp,-16
	sw	$31,12($sp)
	sw	$18,8($sp)
	sw	$17,4($sp)
	sw	$16,0($sp)
	move	$16,$4
	move	$17,$5
	jal	sceKernelCpuSuspendIntr
	move	$18,$6

	lui	$2,%hi(sub_0000)
	addiu	$2,$2,%lo(sub_0000)
	li	$4,65536			# 0x10000
	move	$5,$2
	lui	$6,%hi(_sceSysconModuleRebootBefore)
	addiu	$6,$6,%lo(_sceSysconModuleRebootBefore)
	jal	memcpy
	subu	$6,$6,$2

	jal	sceKernelDcacheWritebackAll
	nop

	jal	sceKernelIcacheInvalidateAll
	nop

	move	$4,$16
	move	$5,$17
	li	$2,65536			# 0x10000
	jalr	$2
	move	$6,$18

	move	$2,$0
	lw	$31,12($sp)
	lw	$18,8($sp)
	lw	$17,4($sp)
	lw	$16,0($sp)
	j	$31
	addiu	$sp,$sp,16

	.set	macro
	.set	reorder
	.end	sub_2D08
	.size	sub_2D08, .-sub_2D08
	.align	2
	.globl	sceSysconNop
	.set	nomips16
	.ent	sceSysconNop
	.type	sceSysconNop, @function
sceSysconNop:
	.frame	$sp,104,$31		# vars= 96, regs= 1/0, args= 0, gp= 0
	.mask	0x80000000,-4
	.fmask	0x00000000,0
	.set	noreorder
	.set	nomacro
	addiu	$sp,$sp,-104
	sw	$31,100($sp)
	sb	$0,12($sp)
	li	$2,2			# 0x2
	sb	$2,13($sp)
	move	$4,$sp
	jal	sceSysconCmdExec
	move	$5,$0

	lw	$31,100($sp)
	j	$31
	addiu	$sp,$sp,104

	.set	macro
	.set	reorder
	.end	sceSysconNop
	.size	sceSysconNop, .-sceSysconNop
	.align	2
	.globl	sceSysconGetBaryonVersion
	.set	nomips16
	.ent	sceSysconGetBaryonVersion
	.type	sceSysconGetBaryonVersion, @function
sceSysconGetBaryonVersion:
	.frame	$sp,8,$31		# vars= 0, regs= 1/0, args= 0, gp= 0
	.mask	0x80000000,-4
	.fmask	0x00000000,0
	.set	noreorder
	.set	nomacro
	addiu	$sp,$sp,-8
	sw	$31,4($sp)
	jal	_sceSysconCommonRead
	li	$5,1			# 0x1

	lw	$31,4($sp)
	j	$31
	addiu	$sp,$sp,8

	.set	macro
	.set	reorder
	.end	sceSysconGetBaryonVersion
	.size	sceSysconGetBaryonVersion, .-sceSysconGetBaryonVersion
	.align	2
	.globl	sceSysconGetGValue
	.set	nomips16
	.ent	sceSysconGetGValue
	.type	sceSysconGetGValue, @function
sceSysconGetGValue:
	.frame	$sp,0,$31		# vars= 0, regs= 0/0, args= 0, gp= 0
	.mask	0x00000000,0
	.fmask	0x00000000,0
	.set	noreorder
	.set	nomacro
	li	$2,-2147483648			# 0xffffffff80000000
	j	$31
	addiu	$2,$2,4

	.set	macro
	.set	reorder
	.end	sceSysconGetGValue
	.size	sceSysconGetGValue, .-sceSysconGetGValue
	.align	2
	.globl	sceSysconGetPowerSupplyStatus
	.set	nomips16
	.ent	sceSysconGetPowerSupplyStatus
	.type	sceSysconGetPowerSupplyStatus, @function
sceSysconGetPowerSupplyStatus:
	.frame	$sp,8,$31		# vars= 0, regs= 1/0, args= 0, gp= 0
	.mask	0x80000000,-4
	.fmask	0x00000000,0
	.set	noreorder
	.set	nomacro
	addiu	$sp,$sp,-8
	sw	$31,4($sp)
	jal	_sceSysconCommonRead
	li	$5,11			# 0xb

	lw	$31,4($sp)
	j	$31
	addiu	$sp,$sp,8

	.set	macro
	.set	reorder
	.end	sceSysconGetPowerSupplyStatus
	.size	sceSysconGetPowerSupplyStatus, .-sceSysconGetPowerSupplyStatus
	.align	2
	.globl	sceSysconGetFallingDetectTime
	.set	nomips16
	.ent	sceSysconGetFallingDetectTime
	.type	sceSysconGetFallingDetectTime, @function
sceSysconGetFallingDetectTime:
	.frame	$sp,0,$31		# vars= 0, regs= 0/0, args= 0, gp= 0
	.mask	0x00000000,0
	.fmask	0x00000000,0
	.set	noreorder
	.set	nomacro
	li	$2,-2147483648			# 0xffffffff80000000
	j	$31
	addiu	$2,$2,4

	.set	macro
	.set	reorder
	.end	sceSysconGetFallingDetectTime
	.size	sceSysconGetFallingDetectTime, .-sceSysconGetFallingDetectTime
	.align	2
	.globl	sceSysconGetWakeUpFactor
	.set	nomips16
	.ent	sceSysconGetWakeUpFactor
	.type	sceSysconGetWakeUpFactor, @function
sceSysconGetWakeUpFactor:
	.frame	$sp,8,$31		# vars= 0, regs= 1/0, args= 0, gp= 0
	.mask	0x80000000,-4
	.fmask	0x00000000,0
	.set	noreorder
	.set	nomacro
	addiu	$sp,$sp,-8
	sw	$31,4($sp)
	jal	_sceSysconCommonRead
	li	$5,14			# 0xe

	lw	$31,4($sp)
	j	$31
	addiu	$sp,$sp,8

	.set	macro
	.set	reorder
	.end	sceSysconGetWakeUpFactor
	.size	sceSysconGetWakeUpFactor, .-sceSysconGetWakeUpFactor
	.align	2
	.globl	sceSysconGetWakeUpReq
	.set	nomips16
	.ent	sceSysconGetWakeUpReq
	.type	sceSysconGetWakeUpReq, @function
sceSysconGetWakeUpReq:
	.frame	$sp,8,$31		# vars= 0, regs= 1/0, args= 0, gp= 0
	.mask	0x80000000,-4
	.fmask	0x00000000,0
	.set	noreorder
	.set	nomacro
	addiu	$sp,$sp,-8
	sw	$31,4($sp)
	jal	_sceSysconCommonRead
	li	$5,15			# 0xf

	lw	$31,4($sp)
	j	$31
	addiu	$sp,$sp,8

	.set	macro
	.set	reorder
	.end	sceSysconGetWakeUpReq
	.size	sceSysconGetWakeUpReq, .-sceSysconGetWakeUpReq
	.align	2
	.globl	sceSysconGetVideoCable
	.set	nomips16
	.ent	sceSysconGetVideoCable
	.type	sceSysconGetVideoCable, @function
sceSysconGetVideoCable:
	.frame	$sp,8,$31		# vars= 0, regs= 2/0, args= 0, gp= 0
	.mask	0x80010000,-4
	.fmask	0x00000000,0
	.set	noreorder
	.set	nomacro
	addiu	$sp,$sp,-8
	sw	$31,4($sp)
	sw	$16,0($sp)
	jal	_sceSysconGetBaryonVersion
	move	$16,$4

	sra	$2,$2,16
	andi	$2,$2,0xf0
	beq	$2,$0,$L475
	li	$3,16			# 0x10

	bne	$2,$3,$L476
	move	$4,$16

$L475:
	sw	$0,0($16)
	li	$2,-2147483648			# 0xffffffff80000000
	j	$L477
	addiu	$2,$2,4

$L476:
	jal	_sceSysconCommonRead
	li	$5,18			# 0x12

$L477:
	lw	$31,4($sp)
	lw	$16,0($sp)
	j	$31
	addiu	$sp,$sp,8

	.set	macro
	.set	reorder
	.end	sceSysconGetVideoCable
	.size	sceSysconGetVideoCable, .-sceSysconGetVideoCable
	.align	2
	.globl	sceSysconReadClock
	.set	nomips16
	.ent	sceSysconReadClock
	.type	sceSysconReadClock, @function
sceSysconReadClock:
	.frame	$sp,8,$31		# vars= 0, regs= 1/0, args= 0, gp= 0
	.mask	0x80000000,-4
	.fmask	0x00000000,0
	.set	noreorder
	.set	nomacro
	addiu	$sp,$sp,-8
	sw	$31,4($sp)
	jal	_sceSysconCommonRead
	li	$5,9			# 0x9

	lw	$31,4($sp)
	j	$31
	addiu	$sp,$sp,8

	.set	macro
	.set	reorder
	.end	sceSysconReadClock
	.size	sceSysconReadClock, .-sceSysconReadClock
	.align	2
	.globl	sceSysconWriteClock
	.set	nomips16
	.ent	sceSysconWriteClock
	.type	sceSysconWriteClock, @function
sceSysconWriteClock:
	.frame	$sp,8,$31		# vars= 0, regs= 1/0, args= 0, gp= 0
	.mask	0x80000000,-4
	.fmask	0x00000000,0
	.set	noreorder
	.set	nomacro
	addiu	$sp,$sp,-8
	sw	$31,4($sp)
	li	$5,32			# 0x20
	jal	_sceSysconCommonWrite
	li	$6,6			# 0x6

	lw	$31,4($sp)
	j	$31
	addiu	$sp,$sp,8

	.set	macro
	.set	reorder
	.end	sceSysconWriteClock
	.size	sceSysconWriteClock, .-sceSysconWriteClock
	.align	2
	.globl	sceSysconReadAlarm
	.set	nomips16
	.ent	sceSysconReadAlarm
	.type	sceSysconReadAlarm, @function
sceSysconReadAlarm:
	.frame	$sp,8,$31		# vars= 0, regs= 1/0, args= 0, gp= 0
	.mask	0x80000000,-4
	.fmask	0x00000000,0
	.set	noreorder
	.set	nomacro
	addiu	$sp,$sp,-8
	sw	$31,4($sp)
	jal	_sceSysconCommonRead
	li	$5,10			# 0xa

	lw	$31,4($sp)
	j	$31
	addiu	$sp,$sp,8

	.set	macro
	.set	reorder
	.end	sceSysconReadAlarm
	.size	sceSysconReadAlarm, .-sceSysconReadAlarm
	.align	2
	.globl	sceSysconWriteAlarm
	.set	nomips16
	.ent	sceSysconWriteAlarm
	.type	sceSysconWriteAlarm, @function
sceSysconWriteAlarm:
	.frame	$sp,8,$31		# vars= 0, regs= 1/0, args= 0, gp= 0
	.mask	0x80000000,-4
	.fmask	0x00000000,0
	.set	noreorder
	.set	nomacro
	addiu	$sp,$sp,-8
	sw	$31,4($sp)
	li	$5,34			# 0x22
	jal	_sceSysconCommonWrite
	li	$6,6			# 0x6

	lw	$31,4($sp)
	j	$31
	addiu	$sp,$sp,8

	.set	macro
	.set	reorder
	.end	sceSysconWriteAlarm
	.size	sceSysconWriteAlarm, .-sceSysconWriteAlarm
	.align	2
	.globl	sceSysconSetUSBStatus
	.set	nomips16
	.ent	sceSysconSetUSBStatus
	.type	sceSysconSetUSBStatus, @function
sceSysconSetUSBStatus:
	.frame	$sp,104,$31		# vars= 96, regs= 1/0, args= 0, gp= 0
	.mask	0x80000000,-4
	.fmask	0x00000000,0
	.set	noreorder
	.set	nomacro
	addiu	$sp,$sp,-104
	sw	$31,100($sp)
	andi	$4,$4,0x00ff
	li	$2,33			# 0x21
	sb	$2,12($sp)
	li	$2,3			# 0x3
	sb	$2,13($sp)
	sb	$4,14($sp)
	andi	$5,$4,0x3
	move	$4,$sp
	jal	sceSysconCmdExec
	sltu	$5,$5,1

	lw	$31,100($sp)
	j	$31
	addiu	$sp,$sp,104

	.set	macro
	.set	reorder
	.end	sceSysconSetUSBStatus
	.size	sceSysconSetUSBStatus, .-sceSysconSetUSBStatus
	.align	2
	.globl	sceSysconGetTachyonWDTStatus
	.set	nomips16
	.ent	sceSysconGetTachyonWDTStatus
	.type	sceSysconGetTachyonWDTStatus, @function
sceSysconGetTachyonWDTStatus:
	.frame	$sp,8,$31		# vars= 0, regs= 1/0, args= 0, gp= 0
	.mask	0x80000000,-4
	.fmask	0x00000000,0
	.set	noreorder
	.set	nomacro
	addiu	$sp,$sp,-8
	sw	$31,4($sp)
	jal	_sceSysconCommonRead
	li	$5,12			# 0xc

	lw	$31,4($sp)
	j	$31
	addiu	$sp,$sp,8

	.set	macro
	.set	reorder
	.end	sceSysconGetTachyonWDTStatus
	.size	sceSysconGetTachyonWDTStatus, .-sceSysconGetTachyonWDTStatus
	.align	2
	.globl	sceSysconCtrlAnalogXYPolling
	.set	nomips16
	.ent	sceSysconCtrlAnalogXYPolling
	.type	sceSysconCtrlAnalogXYPolling, @function
sceSysconCtrlAnalogXYPolling:
	.frame	$sp,8,$31		# vars= 0, regs= 1/0, args= 0, gp= 0
	.mask	0x80000000,-4
	.fmask	0x00000000,0
	.set	noreorder
	.set	nomacro
	addiu	$sp,$sp,-8
	sw	$31,4($sp)
	seb	$4,$4
	li	$5,51			# 0x33
	jal	_sceSysconCommonWrite
	li	$6,3			# 0x3

	lw	$31,4($sp)
	j	$31
	addiu	$sp,$sp,8

	.set	macro
	.set	reorder
	.end	sceSysconCtrlAnalogXYPolling
	.size	sceSysconCtrlAnalogXYPolling, .-sceSysconCtrlAnalogXYPolling
	.align	2
	.globl	sceSysconCtrlHRPower
	.set	nomips16
	.ent	sceSysconCtrlHRPower
	.type	sceSysconCtrlHRPower, @function
sceSysconCtrlHRPower:
	.frame	$sp,8,$31		# vars= 0, regs= 2/0, args= 0, gp= 0
	.mask	0x80010000,-4
	.fmask	0x00000000,0
	.set	noreorder
	.set	nomacro
	addiu	$sp,$sp,-8
	sw	$31,4($sp)
	sw	$16,0($sp)
	seb	$16,$4
	move	$4,$16
	li	$5,52			# 0x34
	jal	_sceSysconCommonWrite
	li	$6,3			# 0x3

	bltz	$2,$L486
	lui	$3,%hi(g_4EB0+85)

	sb	$16,%lo(g_4EB0+85)($3)
$L486:
	lw	$31,4($sp)
	lw	$16,0($sp)
	j	$31
	addiu	$sp,$sp,8

	.set	macro
	.set	reorder
	.end	sceSysconCtrlHRPower
	.size	sceSysconCtrlHRPower, .-sceSysconCtrlHRPower
	.align	2
	.globl	sceSysconCtrlPower
	.set	nomips16
	.ent	sceSysconCtrlPower
	.type	sceSysconCtrlPower, @function
sceSysconCtrlPower:
	.frame	$sp,8,$31		# vars= 0, regs= 1/0, args= 0, gp= 0
	.mask	0x80000000,-4
	.fmask	0x00000000,0
	.set	noreorder
	.set	nomacro
	addiu	$sp,$sp,-8
	sw	$31,4($sp)
	andi	$5,$5,0x1
	sll	$5,$5,23
	ext	$4,$4,0,22
	or	$4,$5,$4
	li	$5,69			# 0x45
	jal	_sceSysconCommonWrite
	li	$6,5			# 0x5

	lw	$31,4($sp)
	j	$31
	addiu	$sp,$sp,8

	.set	macro
	.set	reorder
	.end	sceSysconCtrlPower
	.size	sceSysconCtrlPower, .-sceSysconCtrlPower
	.align	2
	.globl	sceSysconCtrlLED
	.set	nomips16
	.ent	sceSysconCtrlLED
	.type	sceSysconCtrlLED, @function
sceSysconCtrlLED:
	.frame	$sp,16,$31		# vars= 0, regs= 3/0, args= 0, gp= 0
	.mask	0x80030000,-4
	.fmask	0x00000000,0
	.set	noreorder
	.set	nomacro
	addiu	$sp,$sp,-16
	sw	$31,12($sp)
	sw	$17,8($sp)
	sw	$16,4($sp)
	li	$2,1			# 0x1
	beq	$4,$2,$L491
	move	$16,$5

	beq	$4,$0,$L492
	li	$17,64			# 0x40

	li	$2,2			# 0x2
	beq	$4,$2,$L496
	li	$2,3			# 0x3

	bne	$4,$2,$L501
	li	$2,-2147483648			# 0xffffffff80000000

	j	$L502
	nop

$L491:
	lui	$2,%hi(g_4EB0+86)
	sb	$5,%lo(g_4EB0+86)($2)
	j	$L492
	li	$17,128			# 0x80

$L502:
	jal	_sceSysconGetPommelType
	li	$17,16			# 0x10

	slt	$2,$2,768
	beq	$2,$0,$L492
	li	$2,-2147483648			# 0xffffffff80000000

	j	$L503
	addiu	$2,$2,4

$L496:
	li	$17,32			# 0x20
$L492:
	beq	$16,$0,$L494
	move	$4,$0

	jal	_sceSysconGetBaryonVersion
	nop

	sra	$2,$2,16
	andi	$2,$2,0xf0
	beq	$2,$0,$L494
	li	$4,16			# 0x10

	xori	$4,$2,0x10
	li	$3,16			# 0x10
	li	$5,1			# 0x1
	movn	$3,$5,$4
	move	$4,$3
$L494:
	or	$4,$4,$17
	li	$5,71			# 0x47
	jal	_sceSysconCommonWrite
	li	$6,3			# 0x3

	j	$L504
	lw	$31,12($sp)

$L501:
	addiu	$2,$2,258
$L503:
	lw	$31,12($sp)
$L504:
	lw	$17,8($sp)
	lw	$16,4($sp)
	j	$31
	addiu	$sp,$sp,16

	.set	macro
	.set	reorder
	.end	sceSysconCtrlLED
	.size	sceSysconCtrlLED, .-sceSysconCtrlLED
	.align	2
	.globl	sceSysconCtrlDvePower
	.set	nomips16
	.ent	sceSysconCtrlDvePower
	.type	sceSysconCtrlDvePower, @function
sceSysconCtrlDvePower:
	.frame	$sp,8,$31		# vars= 0, regs= 2/0, args= 0, gp= 0
	.mask	0x80010000,-4
	.fmask	0x00000000,0
	.set	noreorder
	.set	nomacro
	addiu	$sp,$sp,-8
	sw	$31,4($sp)
	sw	$16,0($sp)
	jal	_sceSysconGetPommelType
	seb	$16,$4

	sltu	$3,$2,768
	bne	$3,$0,$L507
	sltu	$3,$2,769

	bne	$3,$0,$L506
	move	$2,$0

	move	$4,$16
	li	$5,82			# 0x52
	jal	_sceSysconCommonWrite
	li	$6,3			# 0x3

	bltz	$2,$L506
	lui	$3,%hi(g_4EB0+78)

	j	$L506
	sb	$16,%lo(g_4EB0+78)($3)

$L507:
	li	$2,-2147483648			# 0xffffffff80000000
	addiu	$2,$2,4
$L506:
	lw	$31,4($sp)
	lw	$16,0($sp)
	j	$31
	addiu	$sp,$sp,8

	.set	macro
	.set	reorder
	.end	sceSysconCtrlDvePower
	.size	sceSysconCtrlDvePower, .-sceSysconCtrlDvePower
	.align	2
	.globl	sceSyscon_driver_765775EB
	.set	nomips16
	.ent	sceSyscon_driver_765775EB
	.type	sceSyscon_driver_765775EB, @function
sceSyscon_driver_765775EB:
	.frame	$sp,8,$31		# vars= 0, regs= 2/0, args= 0, gp= 0
	.mask	0x80010000,-4
	.fmask	0x00000000,0
	.set	noreorder
	.set	nomacro
	addiu	$sp,$sp,-8
	sw	$31,4($sp)
	sw	$16,0($sp)
	lui	$2,%hi(g_4EB0+332)
	lbu	$2,%lo(g_4EB0+334)($2)
	addiu	$2,$2,-48
	sltu	$2,$2,2
	beq	$2,$0,$L510
	move	$16,$4

	jal	sub_406C
	nop

	j	$L516
	lw	$31,4($sp)

$L510:
	bne	$4,$0,$L512
	lui	$2,%hi(g_4EB0+376)

	lw	$3,%lo(g_4EB0+376)($2)
	bne	$3,$0,$L513
	move	$2,$0

	j	$L516
	lw	$31,4($sp)

$L512:
	lw	$2,%lo(g_4EB0+376)($2)
	beq	$2,$0,$L517
	andi	$4,$16,0x1

	jal	sceSysconGetBtPowerStatus
	nop

	bne	$2,$0,$L515
	li	$2,-2145058816			# 0xffffffff80250000

$L513:
	andi	$4,$16,0x1
$L517:
	ori	$4,$4,0x80
	li	$5,83			# 0x53
	jal	_sceSysconCommonWrite
	li	$6,3			# 0x3

	bltz	$2,$L511
	lui	$3,%hi(g_4EB0+376)

	j	$L511
	sw	$16,%lo(g_4EB0+376)($3)

$L515:
	addiu	$2,$2,5
$L511:
	lw	$31,4($sp)
$L516:
	lw	$16,0($sp)
	j	$31
	addiu	$sp,$sp,8

	.set	macro
	.set	reorder
	.end	sceSyscon_driver_765775EB
	.size	sceSyscon_driver_765775EB, .-sceSyscon_driver_765775EB
	.align	2
	.globl	sceSysconCtrlCharge
	.set	nomips16
	.ent	sceSysconCtrlCharge
	.type	sceSysconCtrlCharge, @function
sceSysconCtrlCharge:
	.frame	$sp,8,$31		# vars= 0, regs= 2/0, args= 0, gp= 0
	.mask	0x80010000,-4
	.fmask	0x00000000,0
	.set	noreorder
	.set	nomacro
	addiu	$sp,$sp,-8
	sw	$31,4($sp)
	sw	$16,0($sp)
	jal	_sceSysconGetBaryonVersion
	andi	$16,$4,0x00ff

	sra	$2,$2,16
	andi	$2,$2,0xf0
	beq	$2,$0,$L519
	li	$3,16			# 0x10

	beq	$2,$3,$L519
	move	$4,$16

	li	$5,86			# 0x56
	jal	_sceSysconCommonWrite
	li	$6,3			# 0x3

	j	$L522
	lw	$31,4($sp)

$L519:
	bne	$16,$0,$L521
	nop

	jal	sub_3360
	nop

	j	$L522
	lw	$31,4($sp)

$L521:
	jal	sub_32D8
	nop

	lw	$31,4($sp)
$L522:
	lw	$16,0($sp)
	j	$31
	addiu	$sp,$sp,8

	.set	macro
	.set	reorder
	.end	sceSysconCtrlCharge
	.size	sceSysconCtrlCharge, .-sceSysconCtrlCharge
	.align	2
	.globl	sceSysconCtrlTachyonAvcPower
	.set	nomips16
	.ent	sceSysconCtrlTachyonAvcPower
	.type	sceSysconCtrlTachyonAvcPower, @function
sceSysconCtrlTachyonAvcPower:
	.frame	$sp,8,$31		# vars= 0, regs= 2/0, args= 0, gp= 0
	.mask	0x80010000,-4
	.fmask	0x00000000,0
	.set	noreorder
	.set	nomacro
	addiu	$sp,$sp,-8
	sw	$31,4($sp)
	sw	$16,0($sp)
	jal	_sceSysconGetPommelType
	seb	$16,$4

	slt	$2,$2,768
	li	$3,2			# 0x2
	li	$4,8			# 0x8
	movz	$4,$3,$2
	jal	sceSysconCtrlPower
	move	$5,$16

	bltz	$2,$L525
	lui	$3,%hi(g_4EB0+83)

	sb	$16,%lo(g_4EB0+83)($3)
$L525:
	lw	$31,4($sp)
	lw	$16,0($sp)
	j	$31
	addiu	$sp,$sp,8

	.set	macro
	.set	reorder
	.end	sceSysconCtrlTachyonAvcPower
	.size	sceSysconCtrlTachyonAvcPower, .-sceSysconCtrlTachyonAvcPower
	.align	2
	.globl	sub_32D8
	.set	nomips16
	.ent	sub_32D8
	.type	sub_32D8, @function
sub_32D8:
	.frame	$sp,112,$31		# vars= 104, regs= 1/0, args= 0, gp= 0
	.mask	0x80000000,-4
	.fmask	0x00000000,0
	.set	noreorder
	.set	nomacro
	addiu	$sp,$sp,-112
	sw	$31,108($sp)
	li	$2,79			# 0x4f
	sb	$2,12($sp)
	li	$2,2			# 0x2
	sb	$2,14($sp)
	li	$2,3			# 0x3
	sb	$2,13($sp)
	move	$4,$sp
	jal	sceSysconCmdExec
	move	$5,$0

	bltz	$2,$L528
	addiu	$4,$sp,96

	sw	$0,96($sp)
	addiu	$5,$sp,31
	jal	memcpy
	li	$6,2			# 0x2

	lw	$4,96($sp)
	andi	$4,$4,0xfdff
	sw	$4,96($sp)
	sll	$4,$4,8
	ori	$4,$4,0x2
	li	$5,78			# 0x4e
	jal	_sceSysconCommonWrite
	li	$6,5			# 0x5

$L528:
	lw	$31,108($sp)
	j	$31
	addiu	$sp,$sp,112

	.set	macro
	.set	reorder
	.end	sub_32D8
	.size	sub_32D8, .-sub_32D8
	.align	2
	.globl	sub_3360
	.set	nomips16
	.ent	sub_3360
	.type	sub_3360, @function
sub_3360:
	.frame	$sp,112,$31		# vars= 104, regs= 1/0, args= 0, gp= 0
	.mask	0x80000000,-4
	.fmask	0x00000000,0
	.set	noreorder
	.set	nomacro
	addiu	$sp,$sp,-112
	sw	$31,108($sp)
	li	$2,79			# 0x4f
	sb	$2,16($sp)
	li	$2,2			# 0x2
	sb	$2,18($sp)
	li	$2,3			# 0x3
	sb	$2,17($sp)
	addiu	$4,$sp,4
	jal	sceSysconCmdExec
	move	$5,$0

	bltz	$2,$L530
	move	$4,$sp

	sw	$0,0($sp)
	addiu	$5,$sp,35
	jal	memcpy
	li	$6,2			# 0x2

	lw	$4,0($sp)
	ori	$4,$4,0x200
	sw	$4,0($sp)
	andi	$4,$4,0xffff
	sll	$4,$4,8
	ori	$4,$4,0x2
	li	$5,78			# 0x4e
	jal	_sceSysconCommonWrite
	li	$6,5			# 0x5

$L530:
	lw	$31,108($sp)
	j	$31
	addiu	$sp,$sp,112

	.set	macro
	.set	reorder
	.end	sub_3360
	.size	sub_3360, .-sub_3360
	.align	2
	.globl	sceSysconGetPommelVersion
	.set	nomips16
	.ent	sceSysconGetPommelVersion
	.type	sceSysconGetPommelVersion, @function
sceSysconGetPommelVersion:
	.frame	$sp,8,$31		# vars= 0, regs= 1/0, args= 0, gp= 0
	.mask	0x80000000,-4
	.fmask	0x00000000,0
	.set	noreorder
	.set	nomacro
	addiu	$sp,$sp,-8
	sw	$31,4($sp)
	jal	_sceSysconCommonRead
	li	$5,64			# 0x40

	lw	$31,4($sp)
	j	$31
	addiu	$sp,$sp,8

	.set	macro
	.set	reorder
	.end	sceSysconGetPommelVersion
	.size	sceSysconGetPommelVersion, .-sceSysconGetPommelVersion
	.align	2
	.globl	sceSyscon_driver_FB148FB6
	.set	nomips16
	.ent	sceSyscon_driver_FB148FB6
	.type	sceSyscon_driver_FB148FB6, @function
sceSyscon_driver_FB148FB6:
	.frame	$sp,8,$31		# vars= 0, regs= 1/0, args= 0, gp= 0
	.mask	0x80000000,-4
	.fmask	0x00000000,0
	.set	noreorder
	.set	nomacro
	addiu	$sp,$sp,-8
	sw	$31,4($sp)
	jal	_sceSysconCommonRead
	li	$5,65			# 0x41

	lw	$31,4($sp)
	j	$31
	addiu	$sp,$sp,8

	.set	macro
	.set	reorder
	.end	sceSyscon_driver_FB148FB6
	.size	sceSyscon_driver_FB148FB6, .-sceSyscon_driver_FB148FB6
	.align	2
	.globl	sceSysconCtrlVoltage
	.set	nomips16
	.ent	sceSysconCtrlVoltage
	.type	sceSysconCtrlVoltage, @function
sceSysconCtrlVoltage:
	.frame	$sp,16,$31		# vars= 0, regs= 3/0, args= 0, gp= 0
	.mask	0x80030000,-4
	.fmask	0x00000000,0
	.set	noreorder
	.set	nomacro
	addiu	$sp,$sp,-16
	sw	$31,12($sp)
	sw	$17,8($sp)
	sw	$16,4($sp)
	move	$16,$4
	jal	_sceSysconGetPommelType
	move	$17,$5

	li	$3,256			# 0x100
	beq	$2,$3,$L538
	andi	$17,$17,0xffff

	slt	$2,$16,4
	bne	$2,$0,$L538
	nop

	slt	$2,$16,6
	bnel	$2,$0,$L536
	li	$2,-2147483648			# 0xffffffff80000000

$L538:
	sll	$4,$17,8
	andi	$16,$16,0xff
	or	$4,$4,$16
	li	$5,66			# 0x42
	jal	_sceSysconCommonWrite
	li	$6,5			# 0x5

	j	$L537
	lw	$31,12($sp)

$L536:
	addiu	$2,$2,4
	lw	$31,12($sp)
$L537:
	lw	$17,8($sp)
	lw	$16,4($sp)
	j	$31
	addiu	$sp,$sp,16

	.set	macro
	.set	reorder
	.end	sceSysconCtrlVoltage
	.size	sceSysconCtrlVoltage, .-sceSysconCtrlVoltage
	.align	2
	.globl	sceSysconGetGSensorVersion
	.set	nomips16
	.ent	sceSysconGetGSensorVersion
	.type	sceSysconGetGSensorVersion, @function
sceSysconGetGSensorVersion:
	.frame	$sp,0,$31		# vars= 0, regs= 0/0, args= 0, gp= 0
	.mask	0x00000000,0
	.fmask	0x00000000,0
	.set	noreorder
	.set	nomacro
	li	$2,-2147483648			# 0xffffffff80000000
	j	$31
	addiu	$2,$2,4

	.set	macro
	.set	reorder
	.end	sceSysconGetGSensorVersion
	.size	sceSysconGetGSensorVersion, .-sceSysconGetGSensorVersion
	.align	2
	.globl	sceSysconCtrlGSensor
	.set	nomips16
	.ent	sceSysconCtrlGSensor
	.type	sceSysconCtrlGSensor, @function
sceSysconCtrlGSensor:
	.frame	$sp,0,$31		# vars= 0, regs= 0/0, args= 0, gp= 0
	.mask	0x00000000,0
	.fmask	0x00000000,0
	.set	noreorder
	.set	nomacro
	li	$2,-2147483648			# 0xffffffff80000000
	j	$31
	addiu	$2,$2,4

	.set	macro
	.set	reorder
	.end	sceSysconCtrlGSensor
	.size	sceSysconCtrlGSensor, .-sceSysconCtrlGSensor
	.align	2
	.globl	sceSysconGetPowerStatus
	.set	nomips16
	.ent	sceSysconGetPowerStatus
	.type	sceSysconGetPowerStatus, @function
sceSysconGetPowerStatus:
	.frame	$sp,8,$31		# vars= 0, regs= 1/0, args= 0, gp= 0
	.mask	0x80000000,-4
	.fmask	0x00000000,0
	.set	noreorder
	.set	nomacro
	addiu	$sp,$sp,-8
	sw	$31,4($sp)
	jal	_sceSysconCommonRead
	li	$5,70			# 0x46

	lw	$31,4($sp)
	j	$31
	addiu	$sp,$sp,8

	.set	macro
	.set	reorder
	.end	sceSysconGetPowerStatus
	.size	sceSysconGetPowerStatus, .-sceSysconGetPowerStatus
	.align	2
	.globl	sceSysconWritePommelReg
	.set	nomips16
	.ent	sceSysconWritePommelReg
	.type	sceSysconWritePommelReg, @function
sceSysconWritePommelReg:
	.frame	$sp,8,$31		# vars= 0, regs= 1/0, args= 0, gp= 0
	.mask	0x80000000,-4
	.fmask	0x00000000,0
	.set	noreorder
	.set	nomacro
	addiu	$sp,$sp,-8
	sw	$31,4($sp)
	andi	$5,$5,0xffff
	ins	$4,$5,8,24
	li	$5,72			# 0x48
	jal	_sceSysconCommonWrite
	li	$6,5			# 0x5

	lw	$31,4($sp)
	j	$31
	addiu	$sp,$sp,8

	.set	macro
	.set	reorder
	.end	sceSysconWritePommelReg
	.size	sceSysconWritePommelReg, .-sceSysconWritePommelReg
	.align	2
	.globl	sceSysconReadPommelReg
	.set	nomips16
	.ent	sceSysconReadPommelReg
	.type	sceSysconReadPommelReg, @function
sceSysconReadPommelReg:
	.frame	$sp,104,$31		# vars= 96, regs= 2/0, args= 0, gp= 0
	.mask	0x80010000,-4
	.fmask	0x00000000,0
	.set	noreorder
	.set	nomacro
	addiu	$sp,$sp,-104
	sw	$31,100($sp)
	sw	$16,96($sp)
	move	$16,$5
	li	$2,73			# 0x49
	sb	$2,12($sp)
	li	$2,3			# 0x3
	sb	$2,13($sp)
	sb	$4,14($sp)
	move	$4,$sp
	jal	sceSysconCmdExec
	move	$5,$0

	bltz	$2,$L544
	move	$4,$16

	sw	$0,0($16)
	addiu	$5,$sp,31
	jal	memcpy
	li	$6,2			# 0x2

	move	$2,$0
$L544:
	lw	$31,100($sp)
	lw	$16,96($sp)
	j	$31
	addiu	$sp,$sp,104

	.set	macro
	.set	reorder
	.end	sceSysconReadPommelReg
	.size	sceSysconReadPommelReg, .-sceSysconReadPommelReg
	.align	2
	.globl	sceSysconGetPowerError
	.set	nomips16
	.ent	sceSysconGetPowerError
	.type	sceSysconGetPowerError, @function
sceSysconGetPowerError:
	.frame	$sp,0,$31		# vars= 0, regs= 0/0, args= 0, gp= 0
	.mask	0x00000000,0
	.fmask	0x00000000,0
	.set	noreorder
	.set	nomacro
	bnel	$4,$0,$L546
	sw	$0,0($4)

$L546:
	j	$31
	move	$2,$0

	.set	macro
	.set	reorder
	.end	sceSysconGetPowerError
	.size	sceSysconGetPowerError, .-sceSysconGetPowerError
	.align	2
	.globl	sceSysconCtrlLeptonPower
	.set	nomips16
	.ent	sceSysconCtrlLeptonPower
	.type	sceSysconCtrlLeptonPower, @function
sceSysconCtrlLeptonPower:
	.frame	$sp,8,$31		# vars= 0, regs= 2/0, args= 0, gp= 0
	.mask	0x80010000,-4
	.fmask	0x00000000,0
	.set	noreorder
	.set	nomacro
	addiu	$sp,$sp,-8
	sw	$31,4($sp)
	sw	$16,0($sp)
	seb	$16,$4
	move	$4,$16
	li	$5,75			# 0x4b
	jal	_sceSysconCommonWrite
	li	$6,3			# 0x3

	bltz	$2,$L548
	lui	$3,%hi(g_4EB0+74)

	sb	$16,%lo(g_4EB0+74)($3)
$L548:
	lw	$31,4($sp)
	lw	$16,0($sp)
	j	$31
	addiu	$sp,$sp,8

	.set	macro
	.set	reorder
	.end	sceSysconCtrlLeptonPower
	.size	sceSysconCtrlLeptonPower, .-sceSysconCtrlLeptonPower
	.align	2
	.globl	sceSysconCtrlMsPower
	.set	nomips16
	.ent	sceSysconCtrlMsPower
	.type	sceSysconCtrlMsPower, @function
sceSysconCtrlMsPower:
	.frame	$sp,8,$31		# vars= 0, regs= 2/0, args= 0, gp= 0
	.mask	0x80010000,-4
	.fmask	0x00000000,0
	.set	noreorder
	.set	nomacro
	addiu	$sp,$sp,-8
	sw	$31,4($sp)
	sw	$16,0($sp)
	seb	$16,$4
	move	$4,$16
	li	$5,76			# 0x4c
	jal	_sceSysconCommonWrite
	li	$6,3			# 0x3

	bltz	$2,$L550
	lui	$3,%hi(g_4EB0+75)

	sb	$16,%lo(g_4EB0+75)($3)
$L550:
	lw	$31,4($sp)
	lw	$16,0($sp)
	j	$31
	addiu	$sp,$sp,8

	.set	macro
	.set	reorder
	.end	sceSysconCtrlMsPower
	.size	sceSysconCtrlMsPower, .-sceSysconCtrlMsPower
	.align	2
	.globl	sceSysconCtrlWlanPower
	.set	nomips16
	.ent	sceSysconCtrlWlanPower
	.type	sceSysconCtrlWlanPower, @function
sceSysconCtrlWlanPower:
	.frame	$sp,16,$31		# vars= 0, regs= 3/0, args= 0, gp= 0
	.mask	0x80030000,-4
	.fmask	0x00000000,0
	.set	noreorder
	.set	nomacro
	addiu	$sp,$sp,-16
	sw	$31,12($sp)
	sw	$17,8($sp)
	sw	$16,4($sp)
	seb	$17,$4
	move	$4,$17
	li	$5,77			# 0x4d
	jal	_sceSysconCommonWrite
	li	$6,3			# 0x3

	bltz	$2,$L552
	move	$16,$2

	lui	$2,%hi(g_4EB0+76)
	beq	$17,$0,$L552
	sb	$17,%lo(g_4EB0+76)($2)

	jal	sceSysconGetWlanLedCtrl
	nop

	beq	$2,$0,$L553
	move	$2,$16

	li	$4,1			# 0x1
	jal	sceSysconCtrlLED
	li	$5,1			# 0x1

$L552:
	move	$2,$16
$L553:
	lw	$31,12($sp)
	lw	$17,8($sp)
	lw	$16,4($sp)
	j	$31
	addiu	$sp,$sp,16

	.set	macro
	.set	reorder
	.end	sceSysconCtrlWlanPower
	.size	sceSysconCtrlWlanPower, .-sceSysconCtrlWlanPower
	.align	2
	.globl	sceSysconCtrlHddPower
	.set	nomips16
	.ent	sceSysconCtrlHddPower
	.type	sceSysconCtrlHddPower, @function
sceSysconCtrlHddPower:
	.frame	$sp,8,$31		# vars= 0, regs= 2/0, args= 0, gp= 0
	.mask	0x80010000,-4
	.fmask	0x00000000,0
	.set	noreorder
	.set	nomacro
	addiu	$sp,$sp,-8
	sw	$31,4($sp)
	sw	$16,0($sp)
	jal	_sceSysconGetPommelType
	seb	$16,$4

	slt	$2,$2,1280
	bne	$2,$0,$L556
	move	$4,$16

	li	$5,74			# 0x4a
	jal	_sceSysconCommonWrite
	li	$6,3			# 0x3

	bltz	$2,$L555
	lui	$3,%hi(g_4EB0+77)

	j	$L555
	sb	$16,%lo(g_4EB0+77)($3)

$L556:
	li	$2,-2147483648			# 0xffffffff80000000
	addiu	$2,$2,4
$L555:
	lw	$31,4($sp)
	lw	$16,0($sp)
	j	$31
	addiu	$sp,$sp,8

	.set	macro
	.set	reorder
	.end	sceSysconCtrlHddPower
	.size	sceSysconCtrlHddPower, .-sceSysconCtrlHddPower
	.align	2
	.globl	sceSysconCtrlBtPower
	.set	nomips16
	.ent	sceSysconCtrlBtPower
	.type	sceSysconCtrlBtPower, @function
sceSysconCtrlBtPower:
	.frame	$sp,8,$31		# vars= 0, regs= 2/0, args= 0, gp= 0
	.mask	0x80010000,-4
	.fmask	0x00000000,0
	.set	noreorder
	.set	nomacro
	addiu	$sp,$sp,-8
	sw	$31,4($sp)
	sw	$16,0($sp)
	seb	$16,$4
	lui	$2,%hi(g_4EB0+376)
	lw	$3,%lo(g_4EB0+376)($2)
	bne	$3,$0,$L558
	move	$2,$0

	andi	$4,$16,0x1
	li	$5,83			# 0x53
	jal	_sceSysconCommonWrite
	li	$6,3			# 0x3

	bltz	$2,$L558
	lui	$3,%hi(g_4EB0+79)

	sb	$16,%lo(g_4EB0+79)($3)
$L558:
	lw	$31,4($sp)
	lw	$16,0($sp)
	j	$31
	addiu	$sp,$sp,8

	.set	macro
	.set	reorder
	.end	sceSysconCtrlBtPower
	.size	sceSysconCtrlBtPower, .-sceSysconCtrlBtPower
	.align	2
	.globl	sceSysconCtrlUsbPower
	.set	nomips16
	.ent	sceSysconCtrlUsbPower
	.type	sceSysconCtrlUsbPower, @function
sceSysconCtrlUsbPower:
	.frame	$sp,8,$31		# vars= 0, regs= 2/0, args= 0, gp= 0
	.mask	0x80010000,-4
	.fmask	0x00000000,0
	.set	noreorder
	.set	nomacro
	addiu	$sp,$sp,-8
	sw	$31,4($sp)
	sw	$16,0($sp)
	jal	_sceSysconGetBaryonVersion
	seb	$16,$4

	ext	$2,$2,16,8
	andi	$3,$2,0xf0
	beq	$3,$0,$L562
	li	$4,16			# 0x10

	beql	$3,$4,$L563
	li	$2,-2147483648			# 0xffffffff80000000

	addiu	$2,$2,-32
	andi	$2,$2,0x00ff
	sltu	$2,$2,2
	bne	$2,$0,$L564
	move	$4,$16

	li	$5,85			# 0x55
	jal	_sceSysconCommonWrite
	li	$6,3			# 0x3

	bltz	$2,$L561
	lui	$3,%hi(g_4EB0+80)

	j	$L561
	sb	$16,%lo(g_4EB0+80)($3)

$L562:
	li	$2,-2147483648			# 0xffffffff80000000
	j	$L561
	addiu	$2,$2,4

$L563:
	j	$L561
	addiu	$2,$2,4

$L564:
	li	$2,-2147483648			# 0xffffffff80000000
	addiu	$2,$2,4
$L561:
	lw	$31,4($sp)
	lw	$16,0($sp)
	j	$31
	addiu	$sp,$sp,8

	.set	macro
	.set	reorder
	.end	sceSysconCtrlUsbPower
	.size	sceSysconCtrlUsbPower, .-sceSysconCtrlUsbPower
	.align	2
	.globl	sceSysconPermitChargeBattery
	.set	nomips16
	.ent	sceSysconPermitChargeBattery
	.type	sceSysconPermitChargeBattery, @function
sceSysconPermitChargeBattery:
	.frame	$sp,8,$31		# vars= 0, regs= 1/0, args= 0, gp= 0
	.mask	0x80000000,-4
	.fmask	0x00000000,0
	.set	noreorder
	.set	nomacro
	addiu	$sp,$sp,-8
	sw	$31,4($sp)
	jal	sceSysconCtrlCharge
	li	$4,1			# 0x1

	lw	$31,4($sp)
	j	$31
	addiu	$sp,$sp,8

	.set	macro
	.set	reorder
	.end	sceSysconPermitChargeBattery
	.size	sceSysconPermitChargeBattery, .-sceSysconPermitChargeBattery
	.align	2
	.globl	sceSysconForbidChargeBattery
	.set	nomips16
	.ent	sceSysconForbidChargeBattery
	.type	sceSysconForbidChargeBattery, @function
sceSysconForbidChargeBattery:
	.frame	$sp,8,$31		# vars= 0, regs= 1/0, args= 0, gp= 0
	.mask	0x80000000,-4
	.fmask	0x00000000,0
	.set	noreorder
	.set	nomacro
	addiu	$sp,$sp,-8
	sw	$31,4($sp)
	jal	sceSysconCtrlCharge
	move	$4,$0

	lw	$31,4($sp)
	j	$31
	addiu	$sp,$sp,8

	.set	macro
	.set	reorder
	.end	sceSysconForbidChargeBattery
	.size	sceSysconForbidChargeBattery, .-sceSysconForbidChargeBattery
	.align	2
	.globl	sceSysconCtrlTachyonVmePower
	.set	nomips16
	.ent	sceSysconCtrlTachyonVmePower
	.type	sceSysconCtrlTachyonVmePower, @function
sceSysconCtrlTachyonVmePower:
	.frame	$sp,8,$31		# vars= 0, regs= 2/0, args= 0, gp= 0
	.mask	0x80010000,-4
	.fmask	0x00000000,0
	.set	noreorder
	.set	nomacro
	addiu	$sp,$sp,-8
	sw	$31,4($sp)
	sw	$16,0($sp)
	jal	_sceSysconGetPommelType
	seb	$16,$4

	li	$3,256			# 0x100
	bne	$2,$3,$L569
	li	$4,2			# 0x2

	jal	sceSysconCtrlPower
	move	$5,$16

	bltz	$2,$L568
	lui	$3,%hi(g_4EB0+81)

	j	$L568
	sb	$16,%lo(g_4EB0+81)($3)

$L569:
	li	$2,-2147483648			# 0xffffffff80000000
	addiu	$2,$2,4
$L568:
	lw	$31,4($sp)
	lw	$16,0($sp)
	j	$31
	addiu	$sp,$sp,8

	.set	macro
	.set	reorder
	.end	sceSysconCtrlTachyonVmePower
	.size	sceSysconCtrlTachyonVmePower, .-sceSysconCtrlTachyonVmePower
	.align	2
	.globl	sceSysconCtrlTachyonAwPower
	.set	nomips16
	.ent	sceSysconCtrlTachyonAwPower
	.type	sceSysconCtrlTachyonAwPower, @function
sceSysconCtrlTachyonAwPower:
	.frame	$sp,8,$31		# vars= 0, regs= 2/0, args= 0, gp= 0
	.mask	0x80010000,-4
	.fmask	0x00000000,0
	.set	noreorder
	.set	nomacro
	addiu	$sp,$sp,-8
	sw	$31,4($sp)
	sw	$16,0($sp)
	jal	_sceSysconGetPommelType
	seb	$16,$4

	li	$3,256			# 0x100
	bne	$2,$3,$L572
	li	$4,4			# 0x4

	jal	sceSysconCtrlPower
	move	$5,$16

	bltz	$2,$L571
	lui	$3,%hi(g_4EB0+82)

	j	$L571
	sb	$16,%lo(g_4EB0+82)($3)

$L572:
	li	$2,-2147483648			# 0xffffffff80000000
	addiu	$2,$2,4
$L571:
	lw	$31,4($sp)
	lw	$16,0($sp)
	j	$31
	addiu	$sp,$sp,8

	.set	macro
	.set	reorder
	.end	sceSysconCtrlTachyonAwPower
	.size	sceSysconCtrlTachyonAwPower, .-sceSysconCtrlTachyonAwPower
	.align	2
	.globl	sceSysconCtrlLcdPower
	.set	nomips16
	.ent	sceSysconCtrlLcdPower
	.type	sceSysconCtrlLcdPower, @function
sceSysconCtrlLcdPower:
	.frame	$sp,8,$31		# vars= 0, regs= 2/0, args= 0, gp= 0
	.mask	0x80010000,-4
	.fmask	0x00000000,0
	.set	noreorder
	.set	nomacro
	addiu	$sp,$sp,-8
	sw	$31,4($sp)
	sw	$16,0($sp)
	jal	_sceSysconGetPommelType
	seb	$16,$4

	slt	$2,$2,768
	beq	$2,$0,$L575
	li	$4,524288			# 0x80000

	jal	sceSysconCtrlPower
	move	$5,$16

	bltz	$2,$L574
	lui	$3,%hi(g_4EB0+84)

	j	$L574
	sb	$16,%lo(g_4EB0+84)($3)

$L575:
	li	$2,-2147483648			# 0xffffffff80000000
	addiu	$2,$2,4
$L574:
	lw	$31,4($sp)
	lw	$16,0($sp)
	j	$31
	addiu	$sp,$sp,8

	.set	macro
	.set	reorder
	.end	sceSysconCtrlLcdPower
	.size	sceSysconCtrlLcdPower, .-sceSysconCtrlLcdPower
	.align	2
	.globl	sceSysconGetGSensorCarib
	.set	nomips16
	.ent	sceSysconGetGSensorCarib
	.type	sceSysconGetGSensorCarib, @function
sceSysconGetGSensorCarib:
	.frame	$sp,0,$31		# vars= 0, regs= 0/0, args= 0, gp= 0
	.mask	0x00000000,0
	.fmask	0x00000000,0
	.set	noreorder
	.set	nomacro
	li	$2,-2147483648			# 0xffffffff80000000
	j	$31
	addiu	$2,$2,4

	.set	macro
	.set	reorder
	.end	sceSysconGetGSensorCarib
	.size	sceSysconGetGSensorCarib, .-sceSysconGetGSensorCarib
	.align	2
	.globl	sceSysconSetGSensorCarib
	.set	nomips16
	.ent	sceSysconSetGSensorCarib
	.type	sceSysconSetGSensorCarib, @function
sceSysconSetGSensorCarib:
	.frame	$sp,0,$31		# vars= 0, regs= 0/0, args= 0, gp= 0
	.mask	0x00000000,0
	.fmask	0x00000000,0
	.set	noreorder
	.set	nomacro
	li	$2,-2147483648			# 0xffffffff80000000
	j	$31
	addiu	$2,$2,4

	.set	macro
	.set	reorder
	.end	sceSysconSetGSensorCarib
	.size	sceSysconSetGSensorCarib, .-sceSysconSetGSensorCarib
	.align	2
	.globl	sceSysconWritePolestarReg
	.set	nomips16
	.ent	sceSysconWritePolestarReg
	.type	sceSysconWritePolestarReg, @function
sceSysconWritePolestarReg:
	.frame	$sp,8,$31		# vars= 0, regs= 1/0, args= 0, gp= 0
	.mask	0x80000000,-4
	.fmask	0x00000000,0
	.set	noreorder
	.set	nomacro
	addiu	$sp,$sp,-8
	sw	$31,4($sp)
	andi	$5,$5,0xffff
	ins	$4,$5,8,24
	li	$5,78			# 0x4e
	jal	_sceSysconCommonWrite
	li	$6,5			# 0x5

	lw	$31,4($sp)
	j	$31
	addiu	$sp,$sp,8

	.set	macro
	.set	reorder
	.end	sceSysconWritePolestarReg
	.size	sceSysconWritePolestarReg, .-sceSysconWritePolestarReg
	.align	2
	.globl	sceSysconReadPolestarReg
	.set	nomips16
	.ent	sceSysconReadPolestarReg
	.type	sceSysconReadPolestarReg, @function
sceSysconReadPolestarReg:
	.frame	$sp,104,$31		# vars= 96, regs= 2/0, args= 0, gp= 0
	.mask	0x80010000,-4
	.fmask	0x00000000,0
	.set	noreorder
	.set	nomacro
	addiu	$sp,$sp,-104
	sw	$31,100($sp)
	sw	$16,96($sp)
	move	$16,$5
	li	$2,79			# 0x4f
	sb	$2,12($sp)
	li	$2,3			# 0x3
	sb	$2,13($sp)
	sb	$4,14($sp)
	move	$4,$sp
	jal	sceSysconCmdExec
	move	$5,$0

	bltz	$2,$L580
	move	$4,$16

	sw	$0,0($16)
	addiu	$5,$sp,31
	jal	memcpy
	li	$6,2			# 0x2

	move	$2,$0
$L580:
	lw	$31,100($sp)
	lw	$16,96($sp)
	j	$31
	addiu	$sp,$sp,104

	.set	macro
	.set	reorder
	.end	sceSysconReadPolestarReg
	.size	sceSysconReadPolestarReg, .-sceSysconReadPolestarReg
	.align	2
	.globl	sceSysconWriteGSensorReg
	.set	nomips16
	.ent	sceSysconWriteGSensorReg
	.type	sceSysconWriteGSensorReg, @function
sceSysconWriteGSensorReg:
	.frame	$sp,0,$31		# vars= 0, regs= 0/0, args= 0, gp= 0
	.mask	0x00000000,0
	.fmask	0x00000000,0
	.set	noreorder
	.set	nomacro
	li	$2,-2147483648			# 0xffffffff80000000
	j	$31
	addiu	$2,$2,4

	.set	macro
	.set	reorder
	.end	sceSysconWriteGSensorReg
	.size	sceSysconWriteGSensorReg, .-sceSysconWriteGSensorReg
	.align	2
	.globl	sceSysconReadGSensorReg
	.set	nomips16
	.ent	sceSysconReadGSensorReg
	.type	sceSysconReadGSensorReg, @function
sceSysconReadGSensorReg:
	.frame	$sp,0,$31		# vars= 0, regs= 0/0, args= 0, gp= 0
	.mask	0x00000000,0
	.fmask	0x00000000,0
	.set	noreorder
	.set	nomacro
	li	$2,-2147483648			# 0xffffffff80000000
	j	$31
	addiu	$2,$2,4

	.set	macro
	.set	reorder
	.end	sceSysconReadGSensorReg
	.size	sceSysconReadGSensorReg, .-sceSysconReadGSensorReg
	.align	2
	.globl	sceSysconBatteryGetStatusCap
	.set	nomips16
	.ent	sceSysconBatteryGetStatusCap
	.type	sceSysconBatteryGetStatusCap, @function
sceSysconBatteryGetStatusCap:
	.frame	$sp,112,$31		# vars= 96, regs= 3/0, args= 0, gp= 0
	.mask	0x80030000,-4
	.fmask	0x00000000,0
	.set	noreorder
	.set	nomacro
	addiu	$sp,$sp,-112
	sw	$31,108($sp)
	sw	$17,104($sp)
	sw	$16,100($sp)
	move	$17,$4
	move	$16,$5
	li	$2,97			# 0x61
	sb	$2,12($sp)
	li	$2,2			# 0x2
	sb	$2,13($sp)
	move	$4,$sp
	jal	sceSysconCmdExec
	move	$5,$0

	bltz	$2,$L587
	lw	$31,108($sp)

	beq	$17,$0,$L585
	lbu	$2,31($sp)

	sw	$2,0($17)
$L585:
	beq	$16,$0,$L584
	move	$2,$0

	lbu	$2,17($sp)
	sll	$2,$2,8
	lbu	$3,16($sp)
	or	$2,$2,$3
	sw	$2,0($16)
	move	$2,$0
$L584:
	lw	$31,108($sp)
$L587:
	lw	$17,104($sp)
	lw	$16,100($sp)
	j	$31
	addiu	$sp,$sp,112

	.set	macro
	.set	reorder
	.end	sceSysconBatteryGetStatusCap
	.size	sceSysconBatteryGetStatusCap, .-sceSysconBatteryGetStatusCap
	.align	2
	.globl	sceSysconBatteryGetInfo
	.set	nomips16
	.ent	sceSysconBatteryGetInfo
	.type	sceSysconBatteryGetInfo, @function
sceSysconBatteryGetInfo:
	.frame	$sp,104,$31		# vars= 96, regs= 2/0, args= 0, gp= 0
	.mask	0x80010000,-4
	.fmask	0x00000000,0
	.set	noreorder
	.set	nomacro
	addiu	$sp,$sp,-104
	sw	$31,100($sp)
	sw	$16,96($sp)
	move	$16,$4
	li	$2,109			# 0x6d
	sb	$2,12($sp)
	li	$2,2			# 0x2
	sb	$2,13($sp)
	move	$4,$sp
	jal	sceSysconCmdExec
	move	$5,$0

	bltz	$2,$L591
	lw	$31,100($sp)

	beq	$16,$0,$L589
	move	$2,$0

	lbu	$2,35($sp)
	sw	$2,12($16)
	lbu	$2,32($sp)
	sll	$2,$2,8
	lbu	$3,31($sp)
	or	$2,$2,$3
	sw	$2,0($16)
	lbu	$2,33($sp)
	sw	$2,4($16)
	lbu	$2,34($sp)
	sw	$2,8($16)
	move	$2,$0
$L589:
	lw	$31,100($sp)
$L591:
	lw	$16,96($sp)
	j	$31
	addiu	$sp,$sp,104

	.set	macro
	.set	reorder
	.end	sceSysconBatteryGetInfo
	.size	sceSysconBatteryGetInfo, .-sceSysconBatteryGetInfo
	.align	2
	.globl	sceSysconGetBattVolt
	.set	nomips16
	.ent	sceSysconGetBattVolt
	.type	sceSysconGetBattVolt, @function
sceSysconGetBattVolt:
	.frame	$sp,16,$31		# vars= 8, regs= 2/0, args= 0, gp= 0
	.mask	0x80010000,-4
	.fmask	0x00000000,0
	.set	noreorder
	.set	nomacro
	addiu	$sp,$sp,-16
	sw	$31,12($sp)
	sw	$16,8($sp)
	jal	_sceSysconGetBaryonVersion
	move	$16,$4

	ext	$2,$2,16,8
	li	$3,42			# 0x2a
	beq	$2,$3,$L593
	andi	$2,$2,0xf0

	li	$3,48			# 0x30
	beq	$2,$3,$L593
	li	$3,64			# 0x40

	bne	$2,$3,$L595
	li	$2,-2147483648			# 0xffffffff80000000

$L593:
	move	$4,$sp
	jal	_sceSysconCommonRead
	li	$5,13			# 0xd

	bltz	$2,$L594
	lw	$3,0($sp)

	sll	$2,$3,1
	sll	$3,$3,3
	addu	$2,$2,$3
	sll	$3,$2,2
	addu	$2,$2,$3
	sw	$2,0($16)
	j	$L594
	move	$2,$0

$L595:
	addiu	$2,$2,4
$L594:
	lw	$31,12($sp)
	lw	$16,8($sp)
	j	$31
	addiu	$sp,$sp,16

	.set	macro
	.set	reorder
	.end	sceSysconGetBattVolt
	.size	sceSysconGetBattVolt, .-sceSysconGetBattVolt
	.align	2
	.globl	sceSysconGetBattVoltAD
	.set	nomips16
	.ent	sceSysconGetBattVoltAD
	.type	sceSysconGetBattVoltAD, @function
sceSysconGetBattVoltAD:
	.frame	$sp,24,$31		# vars= 8, regs= 3/0, args= 0, gp= 0
	.mask	0x80030000,-4
	.fmask	0x00000000,0
	.set	noreorder
	.set	nomacro
	addiu	$sp,$sp,-24
	sw	$31,20($sp)
	sw	$17,16($sp)
	sw	$16,12($sp)
	move	$17,$4
	jal	_sceSysconGetBaryonVersion
	move	$16,$5

	ext	$2,$2,16,8
	li	$3,42			# 0x2a
	beq	$2,$3,$L597
	andi	$2,$2,0xf0

	li	$3,48			# 0x30
	beq	$2,$3,$L597
	li	$3,64			# 0x40

	bne	$2,$3,$L600
	li	$2,-2147483648			# 0xffffffff80000000

$L597:
	move	$4,$sp
	jal	_sceSysconCommonRead
	li	$5,55			# 0x37

	bltz	$2,$L602
	lw	$31,20($sp)

	beq	$17,$0,$L599
	lbu	$3,2($sp)

	sll	$2,$3,1
	sll	$3,$3,3
	addu	$2,$2,$3
	sll	$3,$2,2
	addu	$2,$2,$3
	sw	$2,0($17)
$L599:
	beq	$16,$0,$L598
	move	$2,$0

	lhu	$2,0($sp)
	sw	$2,0($16)
	j	$L598
	move	$2,$0

$L600:
	addiu	$2,$2,4
$L598:
	lw	$31,20($sp)
$L602:
	lw	$17,16($sp)
	lw	$16,12($sp)
	j	$31
	addiu	$sp,$sp,24

	.set	macro
	.set	reorder
	.end	sceSysconGetBattVoltAD
	.size	sceSysconGetBattVoltAD, .-sceSysconGetBattVoltAD
	.align	2
	.globl	sceSysconBatteryNop
	.set	nomips16
	.ent	sceSysconBatteryNop
	.type	sceSysconBatteryNop, @function
sceSysconBatteryNop:
	.frame	$sp,104,$31		# vars= 96, regs= 1/0, args= 0, gp= 0
	.mask	0x80000000,-4
	.fmask	0x00000000,0
	addiu	$sp,$sp,-104
	sw	$31,100($sp)
	li	$2,96			# 0x60
	sb	$2,12($sp)
	li	$2,2			# 0x2
	sb	$2,13($sp)
	move	$4,$sp
	.set	noreorder
	.set	nomacro
	jal	sceSysconCmdExec
	move	$5,$0
	.set	macro
	.set	reorder

	move	$3,$0
 #APP
 # 19 "../../include/common/inline.h" 1
	min $2, $2, $3
 # 0 "" 2
 #NO_APP
	lw	$31,100($sp)
	.set	noreorder
	.set	nomacro
	j	$31
	addiu	$sp,$sp,104
	.set	macro
	.set	reorder

	.end	sceSysconBatteryNop
	.size	sceSysconBatteryNop, .-sceSysconBatteryNop
	.align	2
	.globl	sceSysconBatteryGetTemp
	.set	nomips16
	.ent	sceSysconBatteryGetTemp
	.type	sceSysconBatteryGetTemp, @function
sceSysconBatteryGetTemp:
	.frame	$sp,8,$31		# vars= 0, regs= 1/0, args= 0, gp= 0
	.mask	0x80000000,-4
	.fmask	0x00000000,0
	.set	noreorder
	.set	nomacro
	addiu	$sp,$sp,-8
	sw	$31,4($sp)
	move	$5,$4
	jal	_sceSysconBatteryCommon
	li	$4,98			# 0x62

	lw	$31,4($sp)
	j	$31
	addiu	$sp,$sp,8

	.set	macro
	.set	reorder
	.end	sceSysconBatteryGetTemp
	.size	sceSysconBatteryGetTemp, .-sceSysconBatteryGetTemp
	.align	2
	.globl	sceSysconBatteryGetVolt
	.set	nomips16
	.ent	sceSysconBatteryGetVolt
	.type	sceSysconBatteryGetVolt, @function
sceSysconBatteryGetVolt:
	.frame	$sp,8,$31		# vars= 0, regs= 1/0, args= 0, gp= 0
	.mask	0x80000000,-4
	.fmask	0x00000000,0
	.set	noreorder
	.set	nomacro
	addiu	$sp,$sp,-8
	sw	$31,4($sp)
	move	$5,$4
	jal	_sceSysconBatteryCommon
	li	$4,99			# 0x63

	lw	$31,4($sp)
	j	$31
	addiu	$sp,$sp,8

	.set	macro
	.set	reorder
	.end	sceSysconBatteryGetVolt
	.size	sceSysconBatteryGetVolt, .-sceSysconBatteryGetVolt
	.align	2
	.globl	sceSysconBatteryGetElec
	.set	nomips16
	.ent	sceSysconBatteryGetElec
	.type	sceSysconBatteryGetElec, @function
sceSysconBatteryGetElec:
	.frame	$sp,8,$31		# vars= 0, regs= 1/0, args= 0, gp= 0
	.mask	0x80000000,-4
	.fmask	0x00000000,0
	.set	noreorder
	.set	nomacro
	addiu	$sp,$sp,-8
	sw	$31,4($sp)
	move	$5,$4
	jal	_sceSysconBatteryCommon
	li	$4,100			# 0x64

	lw	$31,4($sp)
	j	$31
	addiu	$sp,$sp,8

	.set	macro
	.set	reorder
	.end	sceSysconBatteryGetElec
	.size	sceSysconBatteryGetElec, .-sceSysconBatteryGetElec
	.align	2
	.globl	sceSysconBatteryGetRCap
	.set	nomips16
	.ent	sceSysconBatteryGetRCap
	.type	sceSysconBatteryGetRCap, @function
sceSysconBatteryGetRCap:
	.frame	$sp,8,$31		# vars= 0, regs= 1/0, args= 0, gp= 0
	.mask	0x80000000,-4
	.fmask	0x00000000,0
	.set	noreorder
	.set	nomacro
	addiu	$sp,$sp,-8
	sw	$31,4($sp)
	move	$5,$4
	jal	_sceSysconBatteryCommon
	li	$4,101			# 0x65

	lw	$31,4($sp)
	j	$31
	addiu	$sp,$sp,8

	.set	macro
	.set	reorder
	.end	sceSysconBatteryGetRCap
	.size	sceSysconBatteryGetRCap, .-sceSysconBatteryGetRCap
	.align	2
	.globl	sceSysconBatteryGetCap
	.set	nomips16
	.ent	sceSysconBatteryGetCap
	.type	sceSysconBatteryGetCap, @function
sceSysconBatteryGetCap:
	.frame	$sp,8,$31		# vars= 0, regs= 1/0, args= 0, gp= 0
	.mask	0x80000000,-4
	.fmask	0x00000000,0
	.set	noreorder
	.set	nomacro
	addiu	$sp,$sp,-8
	sw	$31,4($sp)
	move	$5,$4
	jal	_sceSysconBatteryCommon
	li	$4,102			# 0x66

	lw	$31,4($sp)
	j	$31
	addiu	$sp,$sp,8

	.set	macro
	.set	reorder
	.end	sceSysconBatteryGetCap
	.size	sceSysconBatteryGetCap, .-sceSysconBatteryGetCap
	.align	2
	.globl	sceSysconBatteryGetFullCap
	.set	nomips16
	.ent	sceSysconBatteryGetFullCap
	.type	sceSysconBatteryGetFullCap, @function
sceSysconBatteryGetFullCap:
	.frame	$sp,8,$31		# vars= 0, regs= 1/0, args= 0, gp= 0
	.mask	0x80000000,-4
	.fmask	0x00000000,0
	.set	noreorder
	.set	nomacro
	addiu	$sp,$sp,-8
	sw	$31,4($sp)
	move	$5,$4
	jal	_sceSysconBatteryCommon
	li	$4,103			# 0x67

	lw	$31,4($sp)
	j	$31
	addiu	$sp,$sp,8

	.set	macro
	.set	reorder
	.end	sceSysconBatteryGetFullCap
	.size	sceSysconBatteryGetFullCap, .-sceSysconBatteryGetFullCap
	.align	2
	.globl	sceSysconBatteryGetIFC
	.set	nomips16
	.ent	sceSysconBatteryGetIFC
	.type	sceSysconBatteryGetIFC, @function
sceSysconBatteryGetIFC:
	.frame	$sp,8,$31		# vars= 0, regs= 1/0, args= 0, gp= 0
	.mask	0x80000000,-4
	.fmask	0x00000000,0
	.set	noreorder
	.set	nomacro
	addiu	$sp,$sp,-8
	sw	$31,4($sp)
	move	$5,$4
	jal	_sceSysconBatteryCommon
	li	$4,104			# 0x68

	lw	$31,4($sp)
	j	$31
	addiu	$sp,$sp,8

	.set	macro
	.set	reorder
	.end	sceSysconBatteryGetIFC
	.size	sceSysconBatteryGetIFC, .-sceSysconBatteryGetIFC
	.align	2
	.globl	sceSysconBatteryGetLimitTime
	.set	nomips16
	.ent	sceSysconBatteryGetLimitTime
	.type	sceSysconBatteryGetLimitTime, @function
sceSysconBatteryGetLimitTime:
	.frame	$sp,8,$31		# vars= 0, regs= 1/0, args= 0, gp= 0
	.mask	0x80000000,-4
	.fmask	0x00000000,0
	.set	noreorder
	.set	nomacro
	addiu	$sp,$sp,-8
	sw	$31,4($sp)
	move	$5,$4
	jal	_sceSysconBatteryCommon
	li	$4,105			# 0x69

	lw	$31,4($sp)
	j	$31
	addiu	$sp,$sp,8

	.set	macro
	.set	reorder
	.end	sceSysconBatteryGetLimitTime
	.size	sceSysconBatteryGetLimitTime, .-sceSysconBatteryGetLimitTime
	.align	2
	.globl	sceSysconBatteryGetStatus
	.set	nomips16
	.ent	sceSysconBatteryGetStatus
	.type	sceSysconBatteryGetStatus, @function
sceSysconBatteryGetStatus:
	.frame	$sp,16,$31		# vars= 8, regs= 2/0, args= 0, gp= 0
	.mask	0x80010000,-4
	.fmask	0x00000000,0
	.set	noreorder
	.set	nomacro
	addiu	$sp,$sp,-16
	sw	$31,12($sp)
	sw	$16,8($sp)
	move	$16,$4
	li	$4,106			# 0x6a
	jal	_sceSysconBatteryCommon
	move	$5,$sp

	bltz	$2,$L615
	lw	$31,12($sp)

	beq	$16,$0,$L613
	move	$2,$0

	lw	$2,0($sp)
	sw	$2,0($16)
	move	$2,$0
$L613:
	lw	$31,12($sp)
$L615:
	lw	$16,8($sp)
	j	$31
	addiu	$sp,$sp,16

	.set	macro
	.set	reorder
	.end	sceSysconBatteryGetStatus
	.size	sceSysconBatteryGetStatus, .-sceSysconBatteryGetStatus
	.align	2
	.globl	sceSysconBatteryGetCycle
	.set	nomips16
	.ent	sceSysconBatteryGetCycle
	.type	sceSysconBatteryGetCycle, @function
sceSysconBatteryGetCycle:
	.frame	$sp,8,$31		# vars= 0, regs= 1/0, args= 0, gp= 0
	.mask	0x80000000,-4
	.fmask	0x00000000,0
	.set	noreorder
	.set	nomacro
	addiu	$sp,$sp,-8
	sw	$31,4($sp)
	move	$5,$4
	jal	_sceSysconBatteryCommon
	li	$4,107			# 0x6b

	lw	$31,4($sp)
	j	$31
	addiu	$sp,$sp,8

	.set	macro
	.set	reorder
	.end	sceSysconBatteryGetCycle
	.size	sceSysconBatteryGetCycle, .-sceSysconBatteryGetCycle
	.align	2
	.globl	sceSysconBatteryGetSerial
	.set	nomips16
	.ent	sceSysconBatteryGetSerial
	.type	sceSysconBatteryGetSerial, @function
sceSysconBatteryGetSerial:
	.frame	$sp,8,$31		# vars= 0, regs= 1/0, args= 0, gp= 0
	.mask	0x80000000,-4
	.fmask	0x00000000,0
	.set	noreorder
	.set	nomacro
	addiu	$sp,$sp,-8
	sw	$31,4($sp)
	move	$5,$4
	jal	_sceSysconBatteryCommon
	li	$4,108			# 0x6c

	lw	$31,4($sp)
	j	$31
	addiu	$sp,$sp,8

	.set	macro
	.set	reorder
	.end	sceSysconBatteryGetSerial
	.size	sceSysconBatteryGetSerial, .-sceSysconBatteryGetSerial
	.align	2
	.globl	sceSysconBatteryGetTempAD
	.set	nomips16
	.ent	sceSysconBatteryGetTempAD
	.type	sceSysconBatteryGetTempAD, @function
sceSysconBatteryGetTempAD:
	.frame	$sp,8,$31		# vars= 0, regs= 1/0, args= 0, gp= 0
	.mask	0x80000000,-4
	.fmask	0x00000000,0
	.set	noreorder
	.set	nomacro
	addiu	$sp,$sp,-8
	sw	$31,4($sp)
	move	$5,$4
	jal	_sceSysconBatteryCommon
	li	$4,110			# 0x6e

	lw	$31,4($sp)
	j	$31
	addiu	$sp,$sp,8

	.set	macro
	.set	reorder
	.end	sceSysconBatteryGetTempAD
	.size	sceSysconBatteryGetTempAD, .-sceSysconBatteryGetTempAD
	.align	2
	.globl	sceSysconBatteryGetVoltAD
	.set	nomips16
	.ent	sceSysconBatteryGetVoltAD
	.type	sceSysconBatteryGetVoltAD, @function
sceSysconBatteryGetVoltAD:
	.frame	$sp,8,$31		# vars= 0, regs= 1/0, args= 0, gp= 0
	.mask	0x80000000,-4
	.fmask	0x00000000,0
	.set	noreorder
	.set	nomacro
	addiu	$sp,$sp,-8
	sw	$31,4($sp)
	move	$5,$4
	jal	_sceSysconBatteryCommon
	li	$4,111			# 0x6f

	lw	$31,4($sp)
	j	$31
	addiu	$sp,$sp,8

	.set	macro
	.set	reorder
	.end	sceSysconBatteryGetVoltAD
	.size	sceSysconBatteryGetVoltAD, .-sceSysconBatteryGetVoltAD
	.align	2
	.globl	sceSysconBatteryGetElecAD
	.set	nomips16
	.ent	sceSysconBatteryGetElecAD
	.type	sceSysconBatteryGetElecAD, @function
sceSysconBatteryGetElecAD:
	.frame	$sp,8,$31		# vars= 0, regs= 1/0, args= 0, gp= 0
	.mask	0x80000000,-4
	.fmask	0x00000000,0
	.set	noreorder
	.set	nomacro
	addiu	$sp,$sp,-8
	sw	$31,4($sp)
	move	$5,$4
	jal	_sceSysconBatteryCommon
	li	$4,112			# 0x70

	lw	$31,4($sp)
	j	$31
	addiu	$sp,$sp,8

	.set	macro
	.set	reorder
	.end	sceSysconBatteryGetElecAD
	.size	sceSysconBatteryGetElecAD, .-sceSysconBatteryGetElecAD
	.align	2
	.globl	sceSysconBatteryGetTotalElec
	.set	nomips16
	.ent	sceSysconBatteryGetTotalElec
	.type	sceSysconBatteryGetTotalElec, @function
sceSysconBatteryGetTotalElec:
	.frame	$sp,8,$31		# vars= 0, regs= 1/0, args= 0, gp= 0
	.mask	0x80000000,-4
	.fmask	0x00000000,0
	.set	noreorder
	.set	nomacro
	addiu	$sp,$sp,-8
	sw	$31,4($sp)
	move	$5,$4
	jal	_sceSysconBatteryCommon
	li	$4,113			# 0x71

	lw	$31,4($sp)
	j	$31
	addiu	$sp,$sp,8

	.set	macro
	.set	reorder
	.end	sceSysconBatteryGetTotalElec
	.size	sceSysconBatteryGetTotalElec, .-sceSysconBatteryGetTotalElec
	.align	2
	.globl	sceSysconBatteryGetChargeTime
	.set	nomips16
	.ent	sceSysconBatteryGetChargeTime
	.type	sceSysconBatteryGetChargeTime, @function
sceSysconBatteryGetChargeTime:
	.frame	$sp,8,$31		# vars= 0, regs= 1/0, args= 0, gp= 0
	.mask	0x80000000,-4
	.fmask	0x00000000,0
	.set	noreorder
	.set	nomacro
	addiu	$sp,$sp,-8
	sw	$31,4($sp)
	move	$5,$4
	jal	_sceSysconBatteryCommon
	li	$4,114			# 0x72

	lw	$31,4($sp)
	j	$31
	addiu	$sp,$sp,8

	.set	macro
	.set	reorder
	.end	sceSysconBatteryGetChargeTime
	.size	sceSysconBatteryGetChargeTime, .-sceSysconBatteryGetChargeTime
	.align	2
	.globl	_sceSysconBatteryCommon
	.set	nomips16
	.ent	_sceSysconBatteryCommon
	.type	_sceSysconBatteryCommon, @function
_sceSysconBatteryCommon:
	.frame	$sp,112,$31		# vars= 96, regs= 3/0, args= 0, gp= 0
	.mask	0x80030000,-4
	.fmask	0x00000000,0
	.set	noreorder
	.set	nomacro
	addiu	$sp,$sp,-112
	sw	$31,108($sp)
	sw	$17,104($sp)
	sw	$16,100($sp)
	move	$16,$4
	jal	_sceSysconGetBaryonVersion
	move	$17,$5

	sra	$2,$2,16
	andi	$4,$2,0xff
	li	$3,42			# 0x2a
	beq	$4,$3,$L629
	li	$3,48			# 0x30

	andi	$2,$2,0xf0
	beq	$2,$3,$L630
	li	$3,64			# 0x40

	beq	$2,$3,$L631
	move	$4,$sp

	sb	$16,12($sp)
	li	$2,2			# 0x2
	sb	$2,13($sp)
	jal	sceSysconCmdExec
	move	$5,$0

	bltz	$2,$L624
	li	$3,4			# 0x4

	lbu	$2,29($sp)
	bne	$2,$3,$L625
	li	$3,5			# 0x5

	j	$L626
	lbu	$2,31($sp)

$L625:
	bne	$2,$3,$L627
	li	$3,6			# 0x6

	lbu	$2,32($sp)
	sll	$2,$2,8
	lbu	$3,31($sp)
	or	$2,$2,$3
	j	$L626
	seh	$2,$2

$L627:
	bne	$2,$3,$L628
	li	$3,7			# 0x7

	lbu	$3,32($sp)
	sll	$3,$3,16
	lbu	$2,33($sp)
	sll	$2,$2,24
	or	$2,$3,$2
	sra	$2,$2,8
	lbu	$3,31($sp)
	j	$L626
	or	$2,$2,$3

$L628:
	bne	$2,$3,$L632
	lbu	$3,34($sp)

	sll	$3,$3,24
	lbu	$2,33($sp)
	sll	$2,$2,16
	or	$2,$3,$2
	lbu	$3,31($sp)
	or	$2,$2,$3
	lbu	$3,32($sp)
	sll	$3,$3,8
	or	$2,$2,$3
$L626:
	sw	$2,0($17)
	j	$L624
	move	$2,$0

$L629:
	li	$2,-2147483648			# 0xffffffff80000000
	j	$L624
	addiu	$2,$2,4

$L630:
	li	$2,-2147483648			# 0xffffffff80000000
	j	$L624
	addiu	$2,$2,4

$L631:
	li	$2,-2147483648			# 0xffffffff80000000
	j	$L624
	addiu	$2,$2,4

$L632:
	li	$2,-2145058816			# 0xffffffff80250000
	addiu	$2,$2,1
$L624:
	lw	$31,108($sp)
	lw	$17,104($sp)
	lw	$16,100($sp)
	j	$31
	addiu	$sp,$sp,112

	.set	macro
	.set	reorder
	.end	_sceSysconBatteryCommon
	.size	_sceSysconBatteryCommon, .-_sceSysconBatteryCommon
	.align	2
	.globl	sceSysconCtrlTachyonVoltage
	.set	nomips16
	.ent	sceSysconCtrlTachyonVoltage
	.type	sceSysconCtrlTachyonVoltage, @function
sceSysconCtrlTachyonVoltage:
	.frame	$sp,24,$31		# vars= 8, regs= 4/0, args= 0, gp= 0
	.mask	0x80070000,-4
	.fmask	0x00000000,0
	.set	noreorder
	.set	nomacro
	addiu	$sp,$sp,-24
	sw	$31,20($sp)
	sw	$18,16($sp)
	sw	$17,12($sp)
	sw	$16,8($sp)
	move	$18,$4
	jal	sceSysconGetPowerStatus
	move	$4,$sp

	bltz	$2,$L634
	move	$16,$2

	jal	_sceSysconGetBaryonVersion
	nop

	sra	$2,$2,16
	andi	$2,$2,0xf0
	beql	$2,$0,$L647
	lw	$16,0($sp)

	li	$3,16			# 0x10
	beq	$2,$3,$L635
	lw	$2,0($sp)

	andi	$2,$2,0x2
	beq	$2,$0,$L636
	li	$17,2			# 0x2

	j	$L645
	li	$4,1			# 0x1

$L635:
	lw	$16,0($sp)
$L647:
	andi	$17,$16,0x8
	li	$2,8			# 0x8
	movn	$2,$0,$17
	jal	_sceSysconGetPommelType
	move	$17,$2

	li	$3,256			# 0x100
	bne	$2,$3,$L639
	andi	$3,$16,0x4

	ori	$2,$17,0x4
	movz	$17,$2,$3
	andi	$16,$16,0x2
	ori	$2,$17,0x2
	movz	$17,$2,$16
$L639:
	beq	$17,$0,$L637
	li	$4,1			# 0x1

$L636:
	move	$4,$17
	jal	sceSysconCtrlPower
	li	$5,1			# 0x1

	bgez	$2,$L644
	move	$16,$2

	j	$L646
	move	$2,$16

$L637:
$L645:
	jal	sceSysconCtrlVoltage
	move	$5,$18

	move	$16,$2
$L634:
	move	$2,$16
$L646:
	lw	$31,20($sp)
	lw	$18,16($sp)
	lw	$17,12($sp)
	lw	$16,8($sp)
	j	$31
	addiu	$sp,$sp,24

$L644:
	li	$4,1			# 0x1
	jal	sceSysconCtrlVoltage
	move	$5,$18

	move	$16,$2
	move	$4,$17
	jal	sceSysconCtrlPower
	move	$5,$0

	j	$L646
	move	$2,$16

	.set	macro
	.set	reorder
	.end	sceSysconCtrlTachyonVoltage
	.size	sceSysconCtrlTachyonVoltage, .-sceSysconCtrlTachyonVoltage
	.align	2
	.globl	sub_406C
	.set	nomips16
	.ent	sub_406C
	.type	sub_406C, @function
sub_406C:
	.frame	$sp,112,$31		# vars= 96, regs= 3/0, args= 0, gp= 0
	.mask	0x80030000,-4
	.fmask	0x00000000,0
	.set	noreorder
	.set	nomacro
	addiu	$sp,$sp,-112
	sw	$31,108($sp)
	sw	$17,104($sp)
	sw	$16,100($sp)
	bne	$4,$0,$L649
	move	$16,$4

	lui	$2,%hi(g_4EB0+376)
	lw	$3,%lo(g_4EB0+376)($2)
	bne	$3,$0,$L651
	move	$2,$0

	j	$L657
	lw	$31,108($sp)

$L649:
	lui	$2,%hi(g_4EB0+376)
	lw	$2,%lo(g_4EB0+376)($2)
	beq	$2,$0,$L658
	move	$4,$0

	jal	sceSysconGetBtPowerStatus
	nop

	bne	$2,$0,$L655
	move	$4,$0

$L658:
	li	$5,150			# 0x96
	jal	sub_4150
	li	$6,150			# 0x96

	bltz	$2,$L657
	lw	$31,108($sp)

$L651:
	li	$2,42			# 0x2a
	sb	$2,12($sp)
	li	$2,4			# 0x4
	sb	$2,13($sp)
	li	$2,1			# 0x1
	sb	$2,14($sp)
	sb	$16,15($sp)
	move	$4,$sp
	jal	sceSysconCmdExec
	move	$5,$0

	bltz	$2,$L653
	move	$17,$2

	lui	$2,%hi(g_4EB0+376)
	sw	$16,%lo(g_4EB0+376)($2)
	bne	$16,$0,$L650
	move	$2,$17

$L653:
	move	$4,$0
	move	$5,$0
	jal	sub_4150
	move	$6,$0

	j	$L650
	move	$2,$17

$L655:
	li	$2,-2145058816			# 0xffffffff80250000
	addiu	$2,$2,5
$L650:
	lw	$31,108($sp)
$L657:
	lw	$17,104($sp)
	lw	$16,100($sp)
	j	$31
	addiu	$sp,$sp,112

	.set	macro
	.set	reorder
	.end	sub_406C
	.size	sub_406C, .-sub_406C
	.align	2
	.globl	sub_4150
	.set	nomips16
	.ent	sub_4150
	.type	sub_4150, @function
sub_4150:
	.frame	$sp,104,$31		# vars= 96, regs= 1/0, args= 0, gp= 0
	.mask	0x80000000,-4
	.fmask	0x00000000,0
	.set	noreorder
	.set	nomacro
	addiu	$sp,$sp,-104
	sw	$31,100($sp)
	li	$2,41			# 0x29
	sb	$2,12($sp)
	li	$2,7			# 0x7
	sb	$2,13($sp)
	li	$2,21			# 0x15
	sb	$2,14($sp)
	li	$2,1			# 0x1
	sb	$2,15($sp)
	sb	$4,16($sp)
	sb	$5,17($sp)
	sb	$6,18($sp)
	move	$4,$sp
	jal	sceSysconCmdExec
	move	$5,$0

	bltz	$2,$L660
	lbu	$4,29($sp)

	li	$3,3			# 0x3
	beq	$4,$3,$L660
	move	$2,$0

	li	$2,-2147483648			# 0xffffffff80000000
	addiu	$2,$2,260
$L660:
	lw	$31,100($sp)
	j	$31
	addiu	$sp,$sp,104

	.set	macro
	.set	reorder
	.end	sub_4150
	.size	sub_4150, .-sub_4150
	.align	2
	.globl	_sceSysconGetPommelType
	.set	nomips16
	.ent	_sceSysconGetPommelType
	.type	_sceSysconGetPommelType, @function
_sceSysconGetPommelType:
	.frame	$sp,8,$31		# vars= 0, regs= 1/0, args= 0, gp= 0
	.mask	0x80000000,-4
	.fmask	0x00000000,0
	.set	noreorder
	.set	nomacro
	addiu	$sp,$sp,-8
	sw	$31,4($sp)
	jal	_sceSysconGetBaryonVersion
	nop

	ext	$4,$2,16,8
	andi	$3,$4,0xf0
	bne	$3,$0,$L663
	li	$5,16			# 0x10

	ext	$2,$2,8,8
	xori	$2,$2,0x1
	li	$4,256			# 0x100
	movz	$4,$0,$2
	j	$L664
	move	$2,$4

$L663:
	beq	$3,$5,$L664
	li	$2,512			# 0x200

	addiu	$2,$4,-32
	andi	$2,$2,0x00ff
	sltu	$2,$2,2
	bne	$2,$0,$L665
	nop

	li	$2,32			# 0x20
	bne	$3,$2,$L701
	li	$2,41			# 0x29

	sltu	$2,$4,34
	bne	$2,$0,$L701
	li	$2,41			# 0x29

	sltu	$2,$4,38
	bne	$2,$0,$L665
	nop

	li	$2,41			# 0x29
$L701:
	bnel	$4,$2,$L667
	li	$2,32			# 0x20

$L665:
	jal	_sceSysconGetPommelVersion
	nop

	xori	$2,$2,0x120
	li	$4,769			# 0x301
	li	$3,768			# 0x300
	movz	$4,$3,$2
	j	$L664
	move	$2,$4

$L667:
	bne	$3,$2,$L668
	li	$2,48			# 0x30

	sltu	$2,$4,34
	beql	$2,$0,$L669
	sltu	$2,$4,38

	j	$L696
	li	$2,64			# 0x40

$L668:
	beq	$3,$2,$L671
	li	$2,64			# 0x40

$L696:
	bne	$3,$2,$L702
	li	$2,32			# 0x20

	j	$L697
	addiu	$5,$4,-48

$L669:
	bne	$2,$0,$L697
	addiu	$5,$4,-48

	sltu	$5,$4,41
	bne	$5,$0,$L664
	li	$2,1024			# 0x400

$L671:
	addiu	$5,$4,-48
$L697:
	andi	$5,$5,0x00ff
	sltu	$5,$5,2
	bne	$5,$0,$L664
	li	$2,1024			# 0x400

	li	$2,32			# 0x20
$L702:
	bne	$3,$2,$L673
	li	$2,48			# 0x30

	sltu	$2,$4,34
	beql	$2,$0,$L674
	sltu	$2,$4,44

	j	$L698
	li	$2,64			# 0x40

$L673:
	beq	$3,$2,$L676
	li	$2,64			# 0x40

$L698:
	bne	$3,$2,$L703
	li	$2,32			# 0x20

	j	$L699
	addiu	$5,$4,-50

$L674:
	bne	$2,$0,$L699
	addiu	$5,$4,-50

	sltu	$5,$4,46
	bne	$5,$0,$L664
	li	$2,1280			# 0x500

$L676:
	addiu	$5,$4,-50
$L699:
	andi	$5,$5,0x00ff
	sltu	$5,$5,14
	bne	$5,$0,$L664
	li	$2,1280			# 0x500

	li	$2,32			# 0x20
$L703:
	bne	$3,$2,$L704
	li	$5,48			# 0x30

	sltu	$2,$4,34
	beq	$2,$0,$L679
	sltu	$2,$4,46

	li	$5,48			# 0x30
$L704:
	beq	$3,$5,$L664
	li	$2,-1			# 0xffffffffffffffff

	li	$2,64			# 0x40
	bne	$3,$2,$L695
	li	$2,32			# 0x20

	j	$L700
	xori	$3,$3,0x40

$L679:
	bne	$2,$0,$L705
	li	$2,-1			# 0xffffffffffffffff

	sltu	$4,$4,48
	bne	$4,$0,$L664
	li	$2,1536			# 0x600

	j	$L664
	li	$2,-1			# 0xffffffffffffffff

$L695:
	bnel	$3,$2,$L700
	xori	$3,$3,0x40

	sltu	$4,$4,34
	beq	$4,$0,$L664
	li	$2,-1			# 0xffffffffffffffff

	li	$4,48			# 0x30
	beq	$3,$4,$L664
	xori	$3,$3,0x40

$L700:
	li	$4,-1			# 0xffffffffffffffff
	li	$2,1536			# 0x600
	movn	$2,$4,$3
$L664:
$L705:
	lw	$31,4($sp)
	j	$31
	addiu	$sp,$sp,8

	.set	macro
	.set	reorder
	.end	_sceSysconGetPommelType
	.size	_sceSysconGetPommelType, .-_sceSysconGetPommelType
	.align	2
	.globl	sceSysconGetDigitalKey
	.set	nomips16
	.ent	sceSysconGetDigitalKey
	.type	sceSysconGetDigitalKey, @function
sceSysconGetDigitalKey:
	.frame	$sp,104,$31		# vars= 96, regs= 2/0, args= 0, gp= 0
	.mask	0x80010000,-4
	.fmask	0x00000000,0
	.set	noreorder
	.set	nomacro
	addiu	$sp,$sp,-104
	sw	$31,100($sp)
	sw	$16,96($sp)
	move	$16,$4
	li	$2,2			# 0x2
	sb	$2,12($sp)
	sb	$2,13($sp)
	move	$4,$sp
	jal	sceSysconCmdExec
	move	$5,$0

	bltz	$2,$L708
	lw	$31,100($sp)

	lbu	$2,31($sp)
	sb	$2,0($16)
	lbu	$2,32($sp)
	sb	$2,1($16)
	move	$2,$0
	lw	$31,100($sp)
$L708:
	lw	$16,96($sp)
	j	$31
	addiu	$sp,$sp,104

	.set	macro
	.set	reorder
	.end	sceSysconGetDigitalKey
	.size	sceSysconGetDigitalKey, .-sceSysconGetDigitalKey
	.globl	module_start
	module_start = _sceSysconModuleStart
	.globl	module_bootstart
	module_bootstart = _sceSysconModuleStart
	.globl	module_reboot_before
	module_reboot_before = _sceSysconModuleRebootBefore
	.ident	"GCC: (GNU) 4.6.3"
