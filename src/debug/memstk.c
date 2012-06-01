/****************************************************************************
	PSP IPL MemoryStick Driver (read/write)

note:
	Supported MsProDuo Only.

****************************************************************************/

// debug switches
#define SHOW_READ_DATA     0
#define SHOW_ERR_MSG       0
#define SHOW_SECTOR_ACCESS 0
#define SHOW_STATUS_REG    0

// HW registers
#define IO_MEM_STICK_CMD *((volatile int*)(0xBD200030))
#define IO_MEM_STICK_DATA *((volatile int*)(0xBD200034))
#define IO_MEM_STICK_STATUS *((volatile int*)(0xBD200038))
#define IO_MEM_STICK_SYS *((volatile int*)(0xBD20003C))

// SYS bit
#define MSRST 0x8000

// STATUS bit 
#define MS_FIFO_RW     0x4000
#define MS_RDY         0x1000
#define MS_TIME_OUT    0x0100
#define MS_CRC_ERROR   0x0200

// MS command code
#define READ_PAGE_DATA  0x2000
#define READ_REG        0x4000
#define GET_INT         0x7000
#define SET_RW_REG_ADRS 0x8000
#define EX_SET_CMD      0x9000
#define WRITE_REG       0xB000
#define WRITE_PAGE_DATA 0xD000
#define SET_CMD         0xE000

// MS status bit
#define INT_REG_CED   0x80
#define INT_REG_ERR   0x40
#define INT_REG_BREQ  0x20
#define INT_REG_CMDNK 0x01
/*
;NORM_COMP       = (ced && !err)
;CMD_ERR_TER     = (ced && err)
;NORM_DATA_TRANS = (!err && breq)
;DATA_REQ_ERR    = (err && breq)
;CMD_EXE         = (!ced && !breq)
;CMD_NOT_EXE     = cmdnk
*/

/****************************************************************************
****************************************************************************/
static int ms_wait_ready(void)
{
	int status;
//Kprintf("ms_wait_ready\n");
	do{
		status = IO_MEM_STICK_STATUS;
	}while(!(status & MS_RDY));

	if (status & (MS_CRC_ERROR|MS_TIME_OUT))
	{
#if SHOW_ERR_MSG
Kprintf("err:ms_wait_ready %08X\n",status);
#endif
		return -1;
	}
	return 0;
}

/****************************************************************************
****************************************************************************/
static int send_data_and_sync(int arg1, int arg2)
{
  int ret;
  IO_MEM_STICK_DATA = arg1;
  IO_MEM_STICK_DATA = arg2;
  ret = ms_wait_ready();
  return ret;
}

/****************************************************************************
****************************************************************************/
static int ms_get_reg_int(void)
{
  int ret, dummy, status;

  IO_MEM_STICK_CMD = GET_INT | 0x1;

  do{
    status = IO_MEM_STICK_STATUS;
    if(status & MS_TIME_OUT)
    {
#if SHOW_ERR_MSG
Kprintf("err:get_reg_int timeout\n");
#endif
	 return -1;
	}
  }while(!(status & MS_FIFO_RW));

  ret = IO_MEM_STICK_DATA;
  dummy = IO_MEM_STICK_DATA;

  do{
    status = IO_MEM_STICK_STATUS;
    if(status & MS_TIME_OUT)
    {
#if SHOW_ERR_MSG
Kprintf("err:get_reg_int timeout\n");
#endif
	 return -1;
	}
  }while(!(status & MS_RDY));

  return ret & 0xff;
}

/****************************************************************************
****************************************************************************/
static int read_data(void *addr, int count)
{
  int i;
  int status;
  for(i = 0; i<count; i+= 4){
    do{
      status = IO_MEM_STICK_STATUS;
      if (status & MS_TIME_OUT) return -1;
    }while(!(status & MS_FIFO_RW));
    *((volatile int*)(addr + i)) = IO_MEM_STICK_DATA;

#if SHOW_READ_DATA
Kprintf("%08X ",*((volatile int*)(addr + i)));
if( (i%0x20) ==0x1c) Kprintf("\n");
#endif

  }
  return 0;
}

/****************************************************************************
****************************************************************************/
static int ms_get_reg(void *buffer, int reg){
  int ret;

//Kprintf("READ_REG\n");
  IO_MEM_STICK_CMD = READ_REG | reg;
  ret = read_data(buffer, reg);

  return ret;
}


/****************************************************************************
****************************************************************************/
static void ms_wait_unk1(void)
{
	while(!(IO_MEM_STICK_STATUS & 0x2000));
}

/****************************************************************************
****************************************************************************/
static void ms_wait_ced(void)
{
	int result;
//Kprintf("wait CED\n");
	do{
		result = ms_get_reg_int();
	}while((result < 0) || ( (result & INT_REG_CED) == 0));
}

/****************************************************************************
	setup & check status register
****************************************************************************/
static int ms_check_status(void)
{
	int ret;
	unsigned char sts[8];

	//set rw reg addrs of type reg (?)
	IO_MEM_STICK_CMD = SET_RW_REG_ADRS | 0x4;
#if 1
	ret = send_data_and_sync(0x06100800,0x00000000);
#else
	IO_MEM_STICK_DATA = 0x06100800;
	IO_MEM_STICK_DATA = 0x00000000;
	ret = ms_wait_ready();
#endif
	if (ret != 0) return -1;

	ms_get_reg(sts, 8);
#if SHOW_STATUS_REG
Kprintf("STATUS %02X CHK[%02X] %02X %02X PRO[%02X] %02X %02X %02X \n",sts[0],sts[1],sts[2],sts[3],sts[4],sts[5],sts[6],sts[7]);
#endif

#if 1
	if(sts[4] != 0x01)
	{
#if SHOW_ERR_MSG
Kprintf("PRE-IPL Supported MS Pro only!\n");
#endif
		return -1;
	}

	if( (sts[2] & 0x15) != 0) return -1;
#else
	// status 0 to 7
	u32 val_a = *((volatile int*)(&sts[0]));
	u32 val_b = *((volatile int*)(&sts[4]));

	// STS[02]
	if ((val_a >> 16) & 0x15 != 0) return -1;
#endif
	return 0;
}

/****************************************************************************
****************************************************************************/
int pspMsInit(void)
{
//Kprintf("_ms_init\n");

  //initialize the hardware
    int i;
	*((volatile int*)(0xBC100054)) |= 0x00000100;
	*((volatile int*)(0xBC100050)) |= 0x00000400;
	*((volatile int*)(0xBC100078)) |= 0x00000010;
	*((volatile int*)(0xBC10004C)) &= 0xFFFFFEFF;

//Kprintf("reset\n");

	//reset the controller
	IO_MEM_STICK_SYS = MSRST;
	while(IO_MEM_STICK_SYS & MSRST);

//Kprintf("check status\n");
	ms_check_status();
	ms_wait_ready();
	ms_wait_ced();

	return 0;
}

/****************************************************************************
****************************************************************************/
int pspMsReadSector(int sector, void *addr)
{
	int ret;

#if SHOW_SECTOR_ACCESS
Kprintf("ms_read_sector(%08X,%08X)\n",sector,addr);
#endif

/*
MS format
SYS_PARAM_REG		(0x10)
BLOCK_ADD_REG2		(0x11)
BLOCK_ADD_REG1		(0x12)
BLOCK_ADD_REG0		(0x13)
CMD_PARAM_REG		(0x14)
PAGE_ADD_REG		(0x15)
OVER_WR_FLAG_REG	(0x16)

MSPro format
//size = 1;
buf[0] = mode; // 0x10
buf[1] = (size & 0xFF00) >> 8;
buf[2] = (size & 0xFF);
buf[3] = (address & 0xFF000000) >> 24;
buf[4] = (address & 0x00FF0000) >> 16;
buf[5] = (address & 0x0000FF00) >> 8;
buf[6] = (address & 0x000000FF);
buf[7] = 0x00;

*/
  //send a command with 8 bytes of params, reverse endian. (0x200001XX 0xYYYYYY00) => READ_DATA
  IO_MEM_STICK_CMD = EX_SET_CMD | 0x7;
  ret = send_data_and_sync(0x010020 | (sector>>24)<<24, ((sector>>16)&0xff) | (sector&0xff00) | ((sector<<16)&0xff0000) );
  if (ret < 0) return -1;

  ms_wait_unk1();

//Kprintf("wait BREQ\n");
  do{
    ret = ms_get_reg_int();
    if (ret < 0) return -1;
  }while((ret & INT_REG_BREQ) == 0);

  if (ret & INT_REG_ERR)
  {
#if SHOW_ERR_MSG
Kprintf("err:ms wait int\n");
#endif
     return -1;
  }

//Kprintf("READ_PAGE_DATA\n");

	//send command to read data and get the data.
	IO_MEM_STICK_CMD = READ_PAGE_DATA | 512;
	ret = read_data(addr, 512);
	if (ret < 0) return -1;

	if (ms_wait_ready() < 0) return -1;
	ms_wait_unk1();
	ms_wait_ced();

  return 0;
}

/****************************************************************************
	write sector data
****************************************************************************/
static int write_data(void *addr, int count)
{
	int i;
	int status;
	for(i = 0; i<count; i+= 4)
	{
		do{
			status = IO_MEM_STICK_STATUS;
			if (status & MS_TIME_OUT) return -3;
		}while(!(status & MS_FIFO_RW));
		IO_MEM_STICK_DATA = *((volatile int*)(addr + i));
	}
	return 0;
}

/****************************************************************************
	write sector
****************************************************************************/
int pspMsWriteSector(int sector, void *addr)
{
	int ret;

	//send a command with 8 bytes of params, reverse endian. (0x210001XX0xYYYYYY00) => WRITE_DATA
	IO_MEM_STICK_CMD = EX_SET_CMD | 0x7;
	ret = send_data_and_sync(0x010021 | (sector>>24)<<24,
	((sector>>16)&0xff) | (sector&0xff00) | ((sector<<16)&0xff0000) );
	if (ret < 0) return -1;

	ms_wait_unk1();

	do{
		ret = ms_get_reg_int();
		if (ret < 0) return -1;
	}while((ret & INT_REG_BREQ) == 0);

	if (ret & INT_REG_ERR)
	{
		return -1;
	}

	//send command to write data and get the data.
	IO_MEM_STICK_CMD = WRITE_PAGE_DATA | 512;
	ret = write_data(addr, 512);
	if (ret < 0) return -1;

	if (ms_wait_ready() < 0) return -1;
	ms_wait_unk1();
	ms_wait_ced();

	return 0;
}
