/****************************************************************************
	PSP IPL SYSREG Driver
****************************************************************************/

#include <common.h>

#define REG32(ADDR) (*(vu32*)(ADDR))
#define SYNC() asm(" sync; nop"::)

u32 sceSysregSpiClkSelect(int a1,int a2)
{
	u32 shift;
	u32 in,out;

	shift = a1<<2;

	in = REG32(0xbc100064);
	out  = in & ~(7<<shift);
	out |= a2<<shift;
	REG32(0xbc100064) = out;
	return (in>shift) & 7;
}

u32 sceSysregSpiClkEnable(u32 bit)
{
	u32 in , out;
	u32 mask = (1<<bit);
	u32 a2 = 1;

	// 1128(mask,1)
	in = REG32(0xbc100058);
	out = (in & (~mask) );
	if(a2) out |= mask;
	REG32(0xbc100058) = out;
	return in & mask;
}
