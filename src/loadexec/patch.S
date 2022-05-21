#include "common_asm.h"

	.text
    .globl decodeKL4EPtr
decodeKL4EPtr:
	.word decodeKL4E
	.word decodeKL4E
	.word loadExecVSH

