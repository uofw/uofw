/* Copyright (C) 2011, 2012 The uOFW team
   See the file COPYING for copying permission.
*/

#ifndef COMMON_INCLUDED
# error "Only include common_imp.h or common_header.h!"
#endif

#define HW(addr) (*(vs32 *)(addr))
#define HWPTR(addr) ((vs32 *)(addr))

#define HW_RESET_VECTOR     0xBFC00000
#define HW_RAM_SIZE         0xBC100040

