/* Copyright (C) The uOFW team
   See the file COPYING for copying permission.
*/

#ifndef COMMON_INCLUDED
# error "Only include common_imp.h or common_header.h!"
#endif

/*
 * Some macros for Allegrex (MIPS generally) opcodes
 */

#define ALLEGREX_MAKE_SYSCALL(n) (0x03FFFFFF & (((u32)(n) << 6) | 0x0000000C))
#define ALLEGREX_MAKE_J(f)       (0x08000000 | ((u32)(f)  & 0x0FFFFFFC))
#define ALLEGREX_MAKE_JR_RA      (0x03E00008)
#define ALLEGREX_MAKE_NOP        (0x00000000)

