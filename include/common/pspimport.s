
/* ent_len: 6 (default) or 5, the length in words of the stub library entry (5: no imported variables table,
   as in the original modules for the libraries which do not export variables). */
.macro IMPORT_START module, flags_ver, ent_len=6

	.set push
	.section .rodata.sceResident, "a"
	.word   0
__stub_modulestr_\module:
	.asciz  "\module"
	.align  2

	.section .lib.stub, "a", @progbits
	.global __stub_module_\module
__stub_module_\module:
	.word   __stub_modulestr_\module
	.word   \flags_ver
	.word   \ent_len
	.word   __executable_start
	.word   __executable_start
	.if \ent_len >= 6
	.word   __executable_start
	.endif

	.set pop
.endm

.macro IMPORT_FUNC module, funcid, funcname

	.set push
	.set noreorder

	.extern __stub_module_\module
	.section .sceStub.text, "ax", @progbits
	.globl  \funcname
	.type   \funcname, @function
	.ent    \funcname, 0
\funcname:
	.word   __stub_module_\module
	.word   \funcid
	.end    \funcname
	.size   \funcname, .-\funcname

	.section .rodata.sceNid, "a"
	.word   \funcid

	.set pop
.endm

.macro IMPORT_FUNC_WITH_ALIAS module, funcid, funcname, alias

	.set push
	.set noreorder

	.extern __stub_module_\module
	.section .sceStub.text, "ax", @progbits
	.globl  \alias
	.type   \alias, @function
\alias:
	.globl  \funcname
	.type   \funcname, @function
	.ent    \funcname, 0
\funcname:
	.word   __stub_module_\module
	.word   \funcid
	.end    \funcname
	.size   \funcname, .-\funcname

	.section .rodata.sceNid, "a"
	.word   \funcid

	.set pop
.endm

.macro IMPORT_VAR module, varid, varname

    .set push
    .set noreorder

    .extern __stub_module_\module
    .section .rodata.sceVstub, "a"
    .globl \varname
\varname:
    .word   __stub_module_\module
    .word   \varid

    .set pop
.endm

