/* Copyright (C) 2011, 2012, 2013 The uOFW team
   See the file COPYING for copying permission.
*/

#ifndef COMMON_INCLUDED
# error "Only include common_imp.h or common_header.h!"
#endif

#define HW(addr) (*(vs32 *)(addr))
#define HWPTR(addr) ((vs32 *)(addr))

#define HW_RAM_SIZE         0xBC100040

#define HW_TIMER_0          0xBC500000
#define HW_TIMER_1          0xBC500010
#define HW_TIMER_2          0xBC500020
#define HW_TIMER_3          0xBC500030

#define HW_RESET_VECTOR     0xBFC00000

