/* Copyright (C) The uOFW team
   See the file COPYING for copying permission.
*/

/*
 *
 *          SYSCON controller fimrware
 *
 *       Renesas/NES 78K0 - model D78F0534
 *
 */

#include <common_imp.h>

// 010E
u32 g_BaryonVersion = 0x00403000;

// 0112
char g_BuildDate[] = "$Date:: 2011-05-09 20:45:25 +0900#$";

void (*g_ops[])(void);

/* functions */

// sub_0660
void RESET(void)
{

}

// sub_06D6
void rotate_and_swap()
{

}

// sub_0725
void xor_bytes()
{

}

// sub_073A
void sub_073A()
{
	return;
}

// sub_073B
void sub_073B()
{

}

// sub_075D
void sub_075D()
{

}

// sub_075F
void main(void)
{

}

// sub_0818
void init_devices_1()
{

}

// sub_0932
void sub_0932(void)
{

}

// sub_0974
void sub_0974(void)
{

}

// sub_987
void sub_0987(void)
{

}

// sub_099A
void sub_099A()
{

}

//sub_09E1
void init_allegrex(void)
{

}

//sub_0A21
void sub_0A21(void)
{

}

// sub_0A5E
void sub_0A5E(void)
{

}

// sub_0A97
void sub_0A97(void)
{

}

// sub_0AB9
void start_allegrex_reset(void)
{

}

// sub_0AF1
void init_allegrex_3(void)
{

}

// sub_0B58
void sub_0B58(void)
{

}

// sub_0B5B
void sub_0B5B(void)
{

}

// sub_0B73
void poll_intrerfaces(void)
{

}

// sub_10A2
void sub_10A2(void)
{

}

// sub_10AC
void sub_10AC(void)
{

}

// sub_1D34
void response_manager(void)
{

}

// sub_1FD2
void sub_1FD2(void)
{

}

// sub_1FF7
void sub_1FF7()
{

}

// TODO: more functions here...

// sub_2654
void response_handler()
{

}

// TODO: more functions here...

// sub_5011
void memcpy(void *pSrc, void *pDst, u16 size)
{

}

// sub_5039
void memcmp()
{

}

// sub_5076
void xorloop_0x10()
{

}

// sub_50A7
void memset(void *s, int c, u16 n)
{
	// 0x50AC - 0x50BF
	while (n-- != 0)
	{
		*(u8 *)(s + n) = (u8)c;
	}
}

// TODO: more functions here...


