/****************************************************************************
	PSP IPL SYSCON Driver
****************************************************************************/
#include <psptypes.h>
//#include "kprintf.h"

#include "sysreg.h"
#include "syscon.h"

#define BYPASS_ERR_CHECK 1

#define REG32(ADDR) (*(vu32*)(ADDR))

/****************************************************************************
  initialize SYSCON HW
****************************************************************************/
int pspSyscon_init(void)
{
	sceSysregSpiClkSelect(0,1);

	sceSysregSpiClkEnable(0);

	//sceSysregSpiIoEnable(0)
	REG32(0xbc100078) |= (0x1000000<<0);

	// init SPI
	REG32(0xbe580000) = 0xcf;
	REG32(0xbe580004) = 0x04;
	REG32(0xbe580014) = 0;
	REG32(0xbe580024) = 0;

	// sceGpioPortClear
	REG32(0xbe24000c) = 0x08;

	// GPIO3 OUT
	REG32(0xbe240000) |= 0x08;
	// GPIO4 IN
	REG32(0xbe240000) &= ~0x10;

	// GpioSetIntrMode(4,3)
	REG32(0xbe240010) &= ~0x10;
	REG32(0xbe240014) &= ~0x10;
	REG32(0xbe240018) |=  0x10;
	REG32(0xbe240024) = 0x10;

#if 0
	pspSyscon_driver_Unkonow_7ec5a957(wram_0108);
	sceSyscon_driver_Unkonow_7bcc5eae();
	sceSysconGetPowerStatus(powersts);
#endif

	return 0;
}

/****************************************************************************
 SYSCON comminucation
****************************************************************************/
int Syscon_cmd(u8 *tx_buf,u8 *rx_buf)
{
	volatile u16 dmy;
	u8 *ptr;
	int result;
	u16 wdata;
	u8 bdata;
	int cnt;
	u8 sum;
	int i;

// Kprintf("SYSCON CMD %02X,%02X\n",tx_buf[0],tx_buf[1]);
retry:
	// calc & set TX sum
	sum = 0;
	cnt = tx_buf[1];
	for(i=0;i<cnt;i++)
		sum += tx_buf[i];
	tx_buf[cnt] = ~sum;

#if 1
	tx_buf[cnt+1] = 0xff;
#else
	// padding TX buf
	for(i=cnt+1;i<0x10;i++)
		tx_buf[i] = 0xff;
#endif

	// clear RX buf
	for(i=0x0f;i>=0;i--)
		rx_buf[i]=0xff;
//
//wait 5usec after GPIO3.fall
//
//	Syscon_wait(5);

	// sceKernelCpuSuspendIntr()

	// sceGpioPortRead();
	dmy = REG32(0xbe240004);
	// sceGpioPortClear(8)
	REG32(0xbe24000c) = 0x08;

	if(REG32(0xbe58000c) & 4)
	{
//Kprintf("RX DUMMY:");
		// clear prevouse data ?
		while( REG32(0xbe58000c) & 4)
		{
			dmy = REG32(0xbe580008);
//Kprintf("%04X:",dmy);
		}
//Kprintf("\n");
	}
	dmy = REG32(0xbe58000c);
;
	REG32(0xbe580020) = 3; // clear error status ?
;
	// TX data
//	cnt = tx_buf[1];
	ptr = tx_buf;
//Kprintf("TX DATA (%d):",cnt);

	for(i=0;i<(cnt+1);i+=2)
	{
		dmy = REG32(0xbe58000c);
//Kprintf("%04X ",(ptr[0]<<8)|ptr[1] );
		REG32(0xbe580008) = (ptr[0]<<8)|ptr[1];
		ptr += 2;
	}
//Kprintf("\n");
	REG32(0xbe580004) = 6; // RX mode ?

//Kprintf("set GPIO3\n");
	// sceGpioPortSet(8)
	REG32(0xbe240008) = 0x08;
//
//wait 5usec after GPIO3.fall
//
//	Syscon_wait(5);

//Kprintf("wait ACK %02X\n",REG32(0xbe240020)&0x18);
//--------------------------------------------------------------
//wait for GPIO4 raise
//--------------------------------------------------------------
// 		r2 = sceGpioQueryIntr(4)
	while( (REG32(0xbe240020) & 0x10)==0)
	{
//Kprintf("%02X ",REG32(0xbe240004)&0x18);
	}

	// GpioAcquireIntr(4)
	REG32(0xbe240024) = 0x10;

//--------------------------------------------------------------
//receive
//--------------------------------------------------------------

//Kprintf("receive\n");
	result = 0;

#if BYPASS_ERR_CHECK
#else
	// error check
	if( (REG32(0xbe58000c) & 4)==0)
	{
//Kprintf("SYSCON err 1\n");
		// error !
		rx_buf[0] = 0xff;
		result = -1;

		// r16[$4] |= 0x00100000 // error
		for(cnt=0x0f;cnt;cnt--);
	}

	// error check
	if( (REG32(0xbe58000c) & 1)==0)
	{
//Kprintf("SYSCON err 2\n");
		// r16[$4] |= $00200000 // error
		result = -1;
	}

	// error check ?
	if( REG32(0xbe580018) & 1)
	{
//Kprintf("SYSCON err 3\n");
		REG32(0xbe580020) = 1;
		// r16[$4] |= $00400000 // error
	}
#endif

//Kprintf("RX data:");
	// receive data
	ptr = rx_buf;
	for(i=0;i<0x10;i+=2)
	{
		if( (REG32(0xbe58000c) & 4)==0)
			break;

		wdata = REG32(0xbe580008);
		bdata = wdata>>8;
		if(i==0)
		{
			result = bdata; // 1st RX data : result ?
		}
		ptr[0] = bdata;
		ptr[1] = wdata & 0xff;
//Kprintf("%02X %02X ",ptr[0],ptr[1]);
		ptr+=2;
	}
//Kprintf("\n");
	REG32(0xbe580004) = 4;
;
	// sceGpioPortClear(8)
	REG32(0xbe24000c) = 0x08;
;
//Kprintf("check sum\n");
	// calc and check RX sum
	if(result>0)
	{
		cnt = rx_buf[1];
		if(cnt < 3)
		{
			result = -2;
		}
		else
		{
			ptr = rx_buf;
			sum = 0;
			for(i=0;i<cnt;i++)
				sum += *ptr++;

			if( (sum^0xff) != rx_buf[cnt])
			{
//Kprintf("SYSCON sum error %02X %02X\n",(sum^0xff),rx_buf[cnt]);
				result = -2; // check sum error
			}
		}
	}

//Kprintf("SYSCON RESULT %02X\n",rx_buf[2]);
	switch(rx_buf[2])
	{
	case 0x80: // SYSCON_RES_80
	case 0x81: // SYSCON_RES_81
		goto retry;
	}

	return result;
}

/****************************************************************************
	4bytes transmit command
****************************************************************************/
int pspSyscon_tx_dword(u32 param,u8 cmd,u8 tx_len)
{
	u8 tx_buf[0x10],rx_buf[0x10];

	tx_buf[0] = cmd;
	tx_buf[1] = tx_len;
	tx_buf[2] = (u8)param;
	tx_buf[3] = (u8)(param>>8);
	tx_buf[4] = (u8)(param>>16);
	tx_buf[5] = (u8)(param>>24);
	return Syscon_cmd(tx_buf,rx_buf);
}

/****************************************************************************
	4bytes result command
****************************************************************************/
int pspSyscon_rx_dword(u32 *param,u8 cmd)
{
	u8 tx_buf[0x10],rx_buf[0x10];
	int result;

	tx_buf[0] = cmd;
	tx_buf[1] = 2;

	result = Syscon_cmd(tx_buf,rx_buf);
	if(result>=0)
	{
		switch(rx_buf[1])
		{
		case 4: *param = rx_buf[3]; break;
		case 5: *param = rx_buf[3]|(rx_buf[4]<<8); break;
		case 6: *param = rx_buf[3]|(rx_buf[4]<<8)|(rx_buf[5]<<16); break;
		default:
			*param = rx_buf[3]|(rx_buf[4]<<8)|(rx_buf[5]<<16)|(rx_buf[6]<<24);
		}
	}
	return result;
}

/****************************************************************************
	non parameter command
****************************************************************************/
int pspSyscon_tx_noparam(u8 cmd)
{
	return pspSyscon_tx_dword(0,cmd,2);
}

/****************************************************************************
****************************************************************************/
u32 Syscon_wait(u32 usec)
{
	u32 i;
	vu32 dmy = 0;

	while(usec--)
	{
		for(i=0;i<10;i++)
		{
			dmy ^= REG32(0xbe240000);
		}
	}
	return dmy;
}

/****************************************************************************
  LED power controll
****************************************************************************/
int pspSysconCtrlLED(int sel,int is_on)
{
	u32 param;

	param = (is_on==0) ? 0xf0 : 0x00;
	if(sel==1)
	{
		param += 0x90;
	}
	else
	{
		param += 0x50;
		if(sel!=0)
		{
			param = (is_on==0) ? 0 : 0xf0;
			param = (-is_on);
			param += 0x30;
		}
	}
	return pspSyscon_tx_dword(param,0x47,0x03);
}

/****************************************************************************
  get CTRL value
****************************************************************************/
int _pspSysconGetCtrl2(u32 *ctrl,u8 *vol1,u8 *vol2)
{
	u8 tx_buf[0x10],rx_buf[0x10];
	int result;

	tx_buf[0] = 0x08;
	tx_buf[1] = 2;
	result = Syscon_cmd(tx_buf,rx_buf);
	*ctrl = rx_buf[3]|(rx_buf[4]<<8)|(rx_buf[5]<<16)|(rx_buf[6]<<24);
	*vol1  = rx_buf[7];
	*vol2  = rx_buf[8];
	return result;
}
