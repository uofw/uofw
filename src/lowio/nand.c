/* Copyright (C) 2011, 2012 The uOFW team
   See the file COPYING for copying permission.
*/

typedef struct {
	u8	user_ecc[3]; //0
	u8	reserved; //3
	u8	block_fmt; //4
	u8	block_stat; //5
	union {
		u16 lbn;
		struct {
			u8 idx;
			u8 ver;
		} IdStorId;
	}
	union {
		u32 	id;	/* 0x38 0x4a 0xc6 0x6d for IPL area */
		struct {
			u8 formatted;
			u8 readonly;
		} IdStorInfo;
	}
	u8	spare_ecc[2];
	u8	reserved[2];
} SceNandSpare_t;

typedef enum {
	USER_ECC_IN_SPARE	= 0x01,
	NO_AUTO_USER_ECC	= 0x10,
	NO_AUTO_SPARE_ECC	= 0x20
} SceNandEccMode_t;

typedef enum {
	STATE_INVALID			= -1,
	STATE_IDLE				= 0,
	STATE_WAIT_READ_IDLE	= 1,
	STATE_WAIT_WRITE_IDLE	= 2,
	STATE_WAIT_READ_READY	= 3,
	STATE_WAIT_WRITE_READY	= 4
} SceNandState_t;

typedef struct {
	int		mutex_id; //0
	int		event_id; //4
	int     clock_enabled; //8
	int		(*event_cb)(void); //C
	SceNandState_t state; //10
	int		status; //14
	int		len; //18
	int		ppn; //1C
	void   *user; //20
	void   *spare; //24
	SceNandEccMode_t mode; //28
	int		write_enabled; //2C
	int     write_enabled_timestamp; //30
	int		unk1; //34
	u32		resume; //38
	int		unk2; //3C
} SceNandInfo_t;

static SceNandInfo_t sceNandInfo; /* 0x0000D9C4 */

typedef struct {
	u32	ctrl; //0
	u32	stat; //4
	u32	cmd; //8
	u32	addr; //C
	u32	unk1; //10
	u32	rst; //14
	u32	unk2; //18
	u32	unk3; //1C
	u32	dma_addr; //20
	u32	dma_ctrl; //24
	u32	dma_stat; //28
	u32	unk4; //2C
	u32	unk5; //30
	u32	unk6; //34
	u32	dma_intr; //38
	u32	unk7; //3C
	u32	reserved1[112]; //1FC
	u32	resume; //200
	u32	reserved2[63]; //204
	u32	data; //300
} SceNandHwReg_t;

static SceNandHwReg_t *sceNandHwReg = (SceNandHwReg_t *) 0xBD101000;

static u32 g_scramble = 0; /* 0x0000DA00 */

/* 0x0000C784 */
static struct PspSysEventHandler nand_ev_handler = {
	.size = 0x40,
	.name = "SceNand",
	.type_mask = 0x00FFFF00,
	.handler = sceNandEventHandler,
};

typedef struct {
	u32 page_size; //0
	u32 pages_per_block; //4
	s32 total_blocks; //8
	u8 manufacture; //C
	u8 device; //D
	u8 maker_blocks; //E
} __attribute__((packed)) SceNandProperties_t;

static SceNandProperties_t sceNandProperties; /* 0x0000E02C */

typedef struct {
	u8 id[2]; //0
	u8 unk1; //2
	u8 maker_blocks; //3
	u16 page_size; //4
	u16 pages_per_block; //6
	s32 total_blocks; //8
} SceNandChipProperties_t;

static const SceNandChipProperties_t sceNandChips[] = {
	{ { 0x98, 0xE6}, 0x03, 0x01, 0x0200, 0x0010; 0x00000400 },
	{ { 0x98, 0x73}, 0x03, 0x01, 0x0200, 0x0020; 0x00000400 },
	{ { 0x98, 0x75}, 0x03, 0x01, 0x0200, 0x0020; 0x00000800 },
	{ { 0x98, 0x76}, 0x03, 0x01, 0x0200, 0x0020; 0x00001000 },
	{ { 0x98, 0x79}, 0x03, 0x01, 0x0200, 0x0020; 0x00002000 },
	{ { 0xEC, 0xE6}, 0x03, 0x02, 0x0200, 0x0010; 0x00000400 },
	{ { 0xEC, 0x73}, 0x03, 0x02, 0x0200, 0x0020; 0x00000400 },
	{ { 0xEC, 0x75}, 0x03, 0x02, 0x0200, 0x0020; 0x00000800 },
	{ { 0xEC, 0x76}, 0x03, 0x02, 0x0200, 0x0020; 0x00001000 },
	{ { 0xEC, 0x79}, 0x03, 0x02, 0x0200, 0x0020; 0x00002000 },
	{ { 0xEC, 0x71}, 0x03, 0x02, 0x0200, 0x0020; 0x00004000 },
	{ { 0xEC, 0xDC}, 0x03, 0x02, 0x0200, 0x0020; 0x00008000 },
	{ { 0xEC, 0x39}, 0x01, 0x02, 0x0200, 0x0010; 0x00000400 },
	{ { 0xEC, 0x33}, 0x01, 0x02, 0x0200, 0x0020; 0x00000400 },
	{ { 0xEC, 0x35}, 0x01, 0x02, 0x0200, 0x0020; 0x00000800 },
	{ { 0xEC, 0x36}, 0x01, 0x02, 0x0200, 0x0020; 0x00001000 },
	{ { 0xEC, 0x78}, 0x01, 0x02, 0x0200, 0x0020; 0x00002000 },
	{ { 0x20, 0x35}, 0x01, 0x02, 0x0200, 0x0020; 0x00000800 },
	{ { 0x20, 0x36}, 0x01, 0x02, 0x0200, 0x0020; 0x00001000 },
	{ { 0x20, 0x39}, 0x01, 0x02, 0x0200, 0x0020; 0x00002000 },
	{ { 0xAD, 0x35}, 0x01, 0x02, 0x0200, 0x0020; 0x00000800 },
	{ { 0xAD, 0x36}, 0x01, 0x02, 0x0200, 0x0020; 0x00001000 },
	{ { 0xAD, 0x39}, 0x01, 0x02, 0x0200, 0x0020; 0x00002000 },
}; /* 0x0000C4A0 */

/* sub_00008B30 */
static int
sceNandInit2(void)
{
	memset(&sceNandInfo, 0, 0x40); //SysclibForKernel_10F3BB61
	sceSysregEmcsmBusClockEnable(); //sceSysreg_driver_F97D9D73
	sceSysregEmcsmIoEnable(); //sceSysreg_driver_9DD1F821
	sceNandInfo.mutex_id = sceKernelCreateMutex("SceNand", 0x801, 0, 0); //ThreadManForKernel_B7D098C6
	sceNandInfo.event_id = sceKernelCreateEventFlag("SceNand", 1, 0, 0); //ThreadManForKernel_55C20A00
	sceNandInfo.clock_enabled = 0;
	KDebugForKernel_E892D9A1();
	sceNandInfo.event_cb = (void *) sceNandInfo.event_id;
	sceNandHwReg.dma_intr = 0x303;

	if (0x004FFFFF < sceSysregGetTachyonVersion()) //sceSysreg_driver_E2A5D1EE
		sceNandInfo.resume = 0x0B060309;
	else
		sceNandInfo.resume = 0x0B040205;

	sceNandHwReg.resume = sceNandInfo.resume;
	sceKernelRegisterIntrHandler(0x14, 2, sceNandIntrHandler, 0, 0); //InterruptManagerForKernel_58DD8978
	sceKernelEnableIntr(0x14); //InterruptManagerForKernel_4D6E7305
	sceKernelRegisterSysEventHandler(&nand_ev_handler); //sceSysEventForKernel_CD9E4BB5
	sceNandLock(0); //sceNand_driver_AE4438C7 

	if (sceNandIsReady()) { // sub_00008C88
		sceNandReset(0); //sceNand_driver_7AF7B77A 
		sceNandDetectChip(); //sceNand_driver_D897C343 
	} else
		sceNandInfo.unk1 = 1;

	sceNandUnlock(); //sceNand_driver_41FFA822

	return 0;
}

/* sub_00008C88 */
static int
sceNandIsReady(void)
{
	return sceNandHwReg->stat & 0x1;
}

/* 0x00008C98 */
static int
sceNandIntrHandler(void)
{
	u32 iflag;
	int ret;

	iflag = sceNandHwReg->dma_intr;
	sceNandHwReg->dma_intr = (((iflag & (iflag >> 8)) & 0x3) | 
								(iflag & (~0x2))) | 0x300;
	volatile __asm__("sync");

	switch (sceNandInfo.state) {
	case STATE_WAIT_READ_READY:
		ret = sceNandTransferDataFromNandBuf(sceNandInfo.mode,
				sceNandInfo.ppn,
				sceNandinfo.user,
				sceNandInfo.spare); //sub_00009A58
		sceNandInfo.status |= ret;
		sceNandInfo.ppn++;
		if (sceNandInfo.user)
			sceNandInfo.user += 0x200;
		if (sceNandInfo.spare)
			sceNandInfo.spare += (sceNandInfo.mode & USER_ECC_IN_SPARE) ?
				0x10 : 0xC;
		if (--sceNandInfo.len <= 0)
			break;
		if (!sceNandIsReady()) { //sub_00008C88
			sceNandHwReg->dma_intr &= ~0x00000203;

			if (!sceNandIsReady()) {
				sceNandInfo.state = STATE_WAIT_READ_IDLE;
				break;
			}
			sceNandHwReg->dma_intr = (sceNandHwReg->dma_intr & (~0x1))
				| 0x00000202;
		}
		sceNandInfo.state = STATE_WAIT_READ_READY;
		if (sceNandInfo.mode & NO_AUTO_USER_ECC)
			sceNandHwReg->ctrl &= ~0x00010000;
		else
			sceNandHwReg->ctrl |= 0x00010000;
		sceNandHwReg->dma_addr = sceNandInfo.ppn << 10;
		sceNandHwReg->dma_intr &= ~0x00000103;
		sceNandHwReg->dma_ctrl = 0x301;
		break;

	case STATE_WAIT_WRITE_READY:
		sceNandInfo.status |= sceNandHwReg->dma_stat > 0;
		sceNandInfo.ppn++;
		if (sceNandInfo.user)
			sceNandInfo.user += 0x200;
		if (sceNandInfo.spare)
			sceNandInfo.spare += (sceNandInfo.mode & USER_ECC_IN_SPARE) ?
				0x10 : 0xC;
		if (--sceNandInfo.len <= 0)
			break;
		if (!sceNandIsReady()) { //sub_00008C88
			sceNandHwReg->dma_intr &= ~0x00000203;

			if (!sceNandIsReady()) {
				sceNandInfo.state = STATE_WAIT_WRITE_IDLE;
				break;
			}
			sceNandHwReg->dma_intr = (sceNandHwReg->dma_intr & (~0x1))
				| 0x00000202;
		}
		sceNandInfo.state = STATE_WAIT_WRITE_READY;
		sceNandTransferDataToNandBuf(sceNandInfo.mode,
				sceNandInfo.ppn,
				sceNandInfo.user,
				sceNandInfo.space); //sub_00009C7C
		break;

	case STATE_WAIT_READ_IDLE:
		sceNandInfo.state = STATE_WAIT_READ_READY;
		if (sceNandInfo.mode & NO_AUTO_USER_ECC)
			sceNandHwReg->ctrl &= ~0x00010000;
		else
			sceNandHwReg->ctrl |= 0x00010000;
		sceNandHwReg->dma_addr = sceNandInfo.ppn << 10;
		sceNandHwReg->dma_intr &= ~0x00000103;
		sceNandHwReg->dma_ctrl = 0x301;
		break;

	case STATE_WAIT_WRITE_IDLE:
		sceNandInfo.state = STATE_WAIT_WRITE_READY;
		sceNandTransferDataToNandBuf(sceNandInfo.mode,
				sceNandInfo.ppn,
				sceNandInfo.user,
				sceNandInfo.spare);
		break;
	
	case STATE_IDLE:
	  break;

	case STATE_INVALID: /* FALL THRU */
	default:
		Kprintf("PANIC: EMC_SM interrupt occured!! (%08X, %08X)\n",
				sceNandInfo.state, iflag); //KDebugForKernel_84F370BC
		break;
	}

	if (sceNandInfo.len)
		return -1;
	sceKernelSetEventFlag(sceNandInfo.event_id, 1); //ThreadManForKernel_1FB15A32 
	sceNandInfo.state = STATE_INVALID;

	return -1;
}

/* sceNand_driver_84EE5D76 */
int
sceNandSetWriteProtect(int protect)
{
	u32 t = sceNandHwReg->stat ^ 0x80;
	int rv = t & 0x80 ? 1 : 0;

	if (!protect) {
		sceNandHwReg->stat = t | 0x00000080;
		while (sceNandHwReg->stat & 0x00000080 == 0);
		sceNandInfo.write_enabled = 1;
		sceNandInfo.write_enabled_timestamp =
				sceKernelGetSystemTimeLow(); //ThreadManForKernel_369ED59D
	} else
		sceNandHwReg->stat = t & 0xFFFFFF7F;

	return rv;
}

/* sceNand_driver_AE4438C7 */
int
sceNandLock(int mode)
{
	int ret;

	if ((ret = sceKernelLockMutex(sceNandInfo.mutex_id, 1, 2) < 0)) //ThreadManForKernel_B011B11F
		return ret;

	if (!sceNandInfo.clock_enabled) {
		sceNandInfo.clock_enabled = 1;
		sceSysregEmcsmBusClockEnable(); //sceSysreg_driver_F97D9D73
	}

	sceNandSetWriteProtect(!mode);

	return 0;
}

/* sceNand_driver_41FFA822 */
int
sceNandUnlock(void)
{
	sceNandInfo.unk9 = 0;
	sceNandSetWriteProtect(1); //sceNand_driver_84EE5D76

	if (!sceNandInfo.event_cb || (sceNandInfo.event_cb)() == 0) {
		sceSysregEmcsmBusClockDisable(); //sceSysreg_driver_2D0F7755
		sceNandInfo.clock_enabled = 0;
	}

	return sceKernelUnlockMutex(sceNandInfo.mutex_id, 1); //ThreadManForKernel_6B30100F
}

/* sceNand_driver_7AF7B77A */
int
sceNandReset(char *status)
{
	int ret;
	char c;

	sceNandHwReg->dma_intr &= ~0x1;
	sceNandHwReg->cmd = 0x000000FF;

	if ((ret = sceNandWaitIntr() < 0)) //sub_00009EEC
		return ret;

	sceNandHwReg->cmd = 0x70;
	c = sceNandHwReg->data;
	if (status)
		status[0] = c;
	if (c & 0x1) {
		sceNandHwReg->rst = 0x1;
		return 0x80230001;
	}

	return 0;
}

/* sceNand_driver_FCDF7610 */
int
sceNandReadId(char *id, int len)
{
	int i;

	sceNandHwReg->cmd = 0x90;
	sceNandHwReg->addr = 0;

	if (id) {
		for (i = 0; i < len; i++)
			p[i] = sceNandHwReg->data;
	}

	sceNandHwReg->rst = 0x1;

	return 0;
}

/* sceNand_driver_766756EF */
int
sceNandReadAccess(u32 ppn, 
		void *user, 
		void *spare, 
		u32 len, 
		SceNandEccMode_t mode)
{
	int ret;
	u32 intr;

	if (len > 32)
		return 0x80000104;
	if (((ppn & 0x1F) + len) > 32)
		return 0x80230008;
	if ((user | spare) & 0x3)
		return 0x80000103;

	intr = sceKernelCpuSuspendIntr(); //InterruptManagerForKernel_092968F4

	sceNandInfo.status = 0;
	sceNandInfo.len = len;
	sceNandInfo.ppn = ppn;
	sceNandInfo.user = user;
	sceNandInfo.spare = spare;
	sceNandInfo.mode = mode;

	if (!sceNandIsReady()) { //sub_00008C88
		sceNandHwReg->dma_intr &= ~0x00000203;

		if (!sceNandIsReady())
			sceNandInfo.state = STATE_WAIT_READ_IDLE;
		else
			sceNandHwReg->dma_intr = (sceNandHwReg->dma_intr & (~0x1))
				| 0x00000202;
	} else {
		sceNandInfo.state = STATE_WAIT_READ_READY;
		if (mode & NO_AUTO_USER_ECC)
			sceNandHwReg->ctrl &= ~0x00010000;
		else
			sceNandHwReg->ctrl |= 0x00010000;

		sceNandHwReg->dma_addr = ppn << 10;
		sceNandHwReg->dma_intr &= ~0x00000103;
		sceNandHwReg->dma_ctrl = 0x00000301;
	}

	sceKernelCpuResumeIntrWithSync(intr); //InterruptManagerForKernel_3B84732D
	if ((ret = sceKernelWaitEventFlag(sceNandInfo.event_id, 1, 33, 0, 0)) < 0) //ThreadManForKernel_402FCF22
		return ret;

	if (!(sceNandInfo.status & 0x1))
		return 0x80230003;
	if (sceNandInfo.status & 0x2)
		return 0x80230009;

	return 0;
}

/* sceNand_driver_0ADC8686 */
int
sceNandWriteAccess(u32 ppn,
		void *user, 
		void *spare, 
		u32 len, 
		SceNandEccMode_t mode)
{
	int ret;
	u32 intr;

	if (len > 32)
		return 0x80000104;
	if (((ppn & 0x1F) + len) > 32)
		return 0x80230008;
	if (spare & 0x3)
		return 0x80000103;

	if ((user == NULL && (mode & NO_AUTO_USER_ECC) == 0) ||
		(spare == NULL && (mode & NO_AUTO_USER_ECC) == 0))
		return 0x80000107;

	if ((ret = sceNandWaitIntr()) < 0) //sub_00009EEC
		return ret;

	intr = sceKernelCpuSuspendIntr(); //InterruptManagerForKernel_092968F4

	sceNandInfo.status = 0;
	sceNandInfo.len = len;
	sceNandInfo.ppn = ppn;
	sceNandInfo.user = user;
	sceNandInfo.spare = spare;
	sceNandInfo.mode = mode;

	if (!sceNandIsReady()) { //sub_00008C88
		sceNandHwReg->dma_intr &= ~0x00000203;

		if (!sceNandIsReady())
			sceNandInfo.state = STATE_WAIT_READ_IDLE;
		else
			sceNandHwReg->dma_intr = (sceNandHwReg->dma_intr & (~0x1))
				| 0x00000202;
	} else {
		sceNandInfo.state = STATE_WAIT_WRITE_READY;
		sceNandTransferDataToNandBuf(mode, ppn, user, space); //sub_00009C7C
	}

	sceKernelCpuResumeIntrWithSync(intr); //InterruptManagerForKernel_3B84732D
	if ((ret = sceKernelWaitEventFlag(sceNandInfo.event_id, 1, 33, 0, 0)) < 0) //ThreadManForKernel_402FCF22
		return ret;

	if (sceNandInfo.status)
		return 0x80230005;

	return 0;
}

/* sceNand_driver_EB0A0022 */
int
sceNandEraseBlock(u32 ppn)
{
	int ret;
	int st;

	if ((ret = sceNandWaitIntr()) < 0) //sub_00009EEC
		return ret;

	if (sceNandInfo.write_enabled) {
		sceNandInfo.write_enabled = 0;
		if ((sceNandInfo.write_enabled_timestamp - 
					sceKernelGetSystemTimeLow()) < 0xA) //ThreadManForKernel_369ED59D
			sceKernelDelayThread(0xA); //ThreadManForKernel_CEADEB47
	}

	sceNandHwReg->cmd = 0x60;
	sceNandHwReg->addr = ppn << 10;
	sceNandHwReg->dma_intr &= ~0x1;
	sceNandHwReg->cmd = 0xD0;

	if ((ret = sceNandWaitIntr()) < 0) //sub_00009EEC
		return ret;

	sceNandHwReg->cmd = 0x70;
	st = sceNandHwReg->data & 0xFF;
	sceNandHWReg->rst = 0x1;

	if (st >= 0)
		return 0x80230007;
	if (st & 0x1)
		return 0x80230004;

	return 0;
}

/* sceNand_driver_5182C394 */
int
sceNandReadExtraOnly(u32 ppn, void *spare, u32 len)
{
	int i, j, ret;
	char *dst;

	if (len > 32)
		return 0x80000104;
	if (((ppn & 0x1F) + len) > 32)
		return 0x80230008;
	if (spare == NULL)
		return 0x80000103;

	ret = 0;
	if (len > 0) {
		dst = spare;
		for (i = 0; i < len, i++, ppn++) {
			if ((ret = sceNandIntr()) < 0) //sub_00009EEC
				break;
			sceNandHwReg->cmd = 0x50;
			sceNandHwReg->addr = ppn << 10;
			for (j = 0; j < 4; j++, dst += 4) {
				dst[0] = scdNandHwReg->data & 0xFF;
				dst[1] = (scdNandHwReg->data >> 8) & 0xFF;
				dst[2] = (scdNandHwReg->data >> 16) & 0xFF;
				dst[3] = (scdNandHwReg->data >> 24) & 0xFF;
			}
		}
	}
	sceNandHwReg->rst = 0x1;

	return ret;
}

/* sub_00009814 */
int
sceNandInit(void)
{
	sceNandInit2(); //sub_00008B30
	return 0;
}

/* sub_00009834 */
int
sceNandEnd(void)
{
	sceNandLock(0); //sceNand_driver_AE4438C7
	sceKernelUnregisterSysEventHandler(&nand_ev_handler); //sceSysEventForKernel_D7D3FDCD 
	sceKernelDeleteMutex(sceNandInfo.mutex_id); //ThreadManForKernel_F8170FBE
	sceKernelDeleteEventFlag(SceNandInfo.event_id); //ThreadManForKernel_EF9E4C70
	sceKernelReleaseIntrHandler(0x14); //InterruptManagerForKernel_F987B1F0

	return 0;
}

/* sceNand_driver_E41A11DE */
int
sceNandReadStatus(void)
{
	int st;

	sceNandHwReg->cmd = 0x70;
	st = sceNandHwReg->data & 0xFF;
	sceNandHwReg->rst = 0x1;

	return st;
}

/* sceNand_driver_0BEE8F36 */ 
/* Description copied from SilverSpring's blog:
 *
 * This is used to set the encryption seed for lfat decryption (on 3.00+) 
 * and for idstorage (on slim). After correctly setting the seed, 
 * all (per-page) reads to the nand will be read decrypted (otherwise 
 * raw reads to the nand will just return rubbish). 
 */
int
sceNandSetScramble(u32 scrmb)
{
	g_scramble = scrmb;
	return 0;
}

/* sceNand_driver_89BDCA08 */
int
sceNandReadPages(u32 ppn, void *user, void *spare, u32 len)
{
	int ret;

	ret = sceNandReadAccess(ppn, user, spare, len, 0); //sceNand_driver_766756EF
	if (ret == 0x80230009 && !spare)
		ret = 0;

	return ret;
}

/* sceNand_driver_8AF0AB9F */
int
sceNandWritePages(u32 ppn, void *user, void *spare, u32 len)
{
	SceNandEccMode_t mode = 0x0;

	if (!user)
		mode |= NO_AUTO_USER_ECC;
	if (!spare)
		mode |= NO_AUTO_SPARE_ECC;

	return sceNandWriteAccess(ppn, user, spare, len, mode); //sceNand_driver_0ADC8686
}

/* sceNand_driver_E05AE88D */
int
sceNandReadPagesRawExtra(u32 ppn, void *user, void *spare, u32 len)
{
	return sceNandReadAccess(ppn, user, spare, len, NO_AUTO_SPARE_ECC); //sceNand_driver_766756EF
}

/* sceNand_driver_8932166A */
int
sceNandWritePagesRawExtra(u32 ppn, void *user, void *spare, u32 len)
{
	SceNandEccMode_t mode = NO_AUTO_SPARE_ECC;

	if (!user)
		mode |= NO_AUTO_USER_ECC;

	return sceNandWriteAccess(ppn, user, spare, len, mode); //sceNand_driver_0ADC8686
}

/* sceNand_driver_C478C1DE */
int
sceNandReadPagesRawAll(u32 ppn, void *user, void *spare, u32 len)
{
	return sceNandReadAccess(ppn, user, spare, len,
			USER_ECC_IN_SPARE | No_AUTO_USER_ECC | NO_AUTO_SPARE_ECC); //sceNand_driver_766756EF
}

/* sceNand_driver_BADD5D46 */
int
sceNandWritePagesRawAll(u32 ppn, void *user, void *spare, u32 len)
{
	return sceNandWriteAccess(ppn, user, spare, len,
			USER_ECC_IN_SPARE | No_AUTO_USER_ECC | NO_AUTO_SPARE_ECC); //sceNand_driver_0ADC8686
}

/* 0x000099B0 */
static int
sceNandEventHandler(int ev_id, char *ev_name, void *param, int *result)
{
	if (ev_id == 0x4004)
		return 0;
	if (ev_id != 0x00010004)
		return 0;

	sceSysregEmcsmBusClockEnable(); //sceSysreg_driver_F97D9D73
	sceSysregEmcsmIoEnable(); //sceSysreg_driver_9DD1F821
	sceNandHwReq->dma_intr = 0x303;
	sceNandHwReg->resume = sceNandInfo.resume;
	sceNandSetWriteProtect(0); //sceNand_driver_84EE5D76
	if (sceNandInfo.event_cb && sceNandInfo.event_cb()) {
		sceNandInfo.clock_enabled = 1;
		return 0;
	}
	sceNandInfo.clock_enabled = 0;
	sceSysregEmcsmBusClockDisable(); //sceSysreg_driver_2D0F7755

	return 0;
}

#define __bitrev(__w) do {\
	__asm__ ("bitrev (%1), (%1)::"r"((__w))");\
} while (0)

/* sub_00009A58 */
static int
sceNandTransferDataFromNandBuf(SceNandEccMode_t mode,
		u32 ppn,
		void *user,
		void *spare)
{
	int ret = 0;
	u32 t0, t1, t2, t3;
	u32 scrmb, key;
	u32 eccbuf[4];
	u32 *_user, *_spare, *end, *nand_buf;

	nand_buf = (u32 *) 0x9FF00000;

	if (user) {
		_user = user;

		__asm__ ("cache 0x19, 0x0(%1);
				  cache 0x19, 0x40(%1);
				  cache 0x19, 0x80(%1);
				  cache 0x19, 0xC0(%1);
				  cache 0x19, 0x100(%1);
				  cache 0x19, 0x140(%1);
				  cache 0x19, 0x180(%1);
				  cache 0x19, 0x1C0(%1);"
				  ::"r"(nand_buf));

		if ((0xD3 >> (((u32) _user) & 0x7)) & 0x1) {
			t0 = ((u32) _user) & 0x3F;
			if (t0 == 0)
				__asm__ ("cache 0x18, 0x0(%1)"::"r"(nand_buf));
			__asm__ ("cache 0x18, 0x40(%1);
					  cache 0x18, 0x80(%1);
					  cache 0x18, 0xC0(%1);
					  cache 0x18, 0x100(%1);
					  cache 0x18, 0x140(%1);
					  cache 0x18, 0x180(%1);"
					  ::"r"(_user));
			if (t0 == 0)
				__asm__ ("cache 0x18, 0x1C0(%1)"::"r"(_user));
		}

		if (g_scramble) {
			scrmb = g_scramble >> 21;
			key = (ppn >> 17) ^ (scrmb * 7);
			nand_buf = (u32 *) (((u32 ) nand_buf & (~0xF8)) | ((ppn ^ scrmb) & 0xF8));
			end = _user + 0x200 / 4;

			do {
				t0 = nand_buf[0];
				t1 = nand_buf[1];
				t2 = nand_buf[2];
				t3 = nand_buf[3];
				nand_buf += 0x10 / 4;
				t0 -= key;
				key += t0;
				_user[0] = t0;
				t1 -= key;
				key ^= t1;
				_user[1] = t1;
				t2 -= key;
				key -= t2;
				_user[2] = t2;
				t3 -= key;
				key -= t3;
				_user[3] = t3;
				key += scrmb;
				_user += 0x10 / 4;
				nand_buf = (u32 *) (((u32) nand_buf) | 0x100);
				__bitrev(key);
			} while (_user != end);
		} else {
			for (i = 0; i < 128; i++)
				_user[i] = nand_buf[i];
		}
	}
	
	if (mode & NO_AUTO_USER_ECC == 0 && sceNandHwReg->dma_stat != 0)
		ret = 1;

	eccbuf[0] = _lw(0xBFF00800);
	eccbuf[1] = _lw(0xBFF00900);
	eccbuf[2] = _lw(0xBFF00904);
	eccbuf[3] = _lw(0xBFF00908);

	if (mode & NO_AUTO_SPARE_ECC == 0) {
		if (sceNandCorrectEcc(&eccbuf[1],
					((_lb(eccbuf + 0xD) << 8) |
					 (_lb(eccbuf + 0xC)) & 0xFFF)) == -3) //sceNand_driver_88CC9F72
			ret |= 0x2;
	}

	if (spare) {
		_spare = spare;

		if (mode & USER_ECC_IN_SPARE) {
			_spare[0] = eccbuf[0];
			_spare[1] = eccbuf[1];
			_spare[2] = eccbuf[2];
			_spare[3] = eccbuf[3];
		} else {
			_spare[0] = eccbuf[1];
			_spare[1] = eccbuf[2];
			_spare[2] = eccbuf[3];
		}
	}

	return ret;
}

/* sub_00009C7C */
static int
sceNandTransferDataToNandBuf(SceNandEccMode_t mode,
		u32 ppn,
		void *user,
		void *spare)
{
	u32 eccbuf[2];
	u32 t0, t1, t2, t3;
	u32 scrmb, key;
	u32 eccbuf[4];
	u32 *_user, *_spare, *end, *nand_buf;
	u32 eccval;
	unsigned char *c;

	nand_buf = (u32 *) 0x9FF00000;

	__asm__ ("cache 0x18, 0x0(%1);
			  cache 0x18, 0x40(%1);
			  cache 0x18, 0x80(%1);
			  cache 0x18, 0xC0(%1);
			  cache 0x18, 0x100(%1);
			  cache 0x18, 0x140(%1);
			  cache 0x18, 0x180(%1);
			  cache 0x18, 0x1C0(%1);"
			  ::"r"(nand_buf));

	if (user) {
		_user = user;

		if (g_scramble) {
			scrmb = g_scramble >> 21;
			key = (ppn >> 17) ^ (scrmb * 7);
			nand_buf = (u32 *) (((u32 ) nand_buf & (~0xF8)) | ((ppn ^ scrmb) & 0xF8));
			end = _user + 0x200 / 4;

			do {
				t0 = _user[0];
				t1 = _user[1];
				t2 = _user[2];
				t3 = _user[3];
				_user += 0x10 / 4;
				nand_buf[0] = t0 + key;
				key += t0;
				nand_buf[1] = t1 + key;
				key ^= t1;
				nand_buf[2] = t2 + key;
				key -= t2;
				nand_buf[3] = t3 + key;
				key += t3;
				key += scrmb;
				nand_buf += 0x10 / 4;
				nand_buf = (u32 *) (((u32) nand_buf) | 0x100);
				__bitrev(key);
			} while (_user != end);

		} else {
			for (i = 0; i < 128; i++)
				nand_buf[i] = _user[i];
		}
	} else {
			for (i = 0; i < 128; i++)
				nand_buf[i] = 0xFFFFFFFF;
	}

	nand_buf = (u32 *) 0x9FF00000;
	__asm__ ("cache 0x1B, 0x0(%1);
			  cache 0x1B, 0x40(%1);
			  cache 0x1B, 0x80(%1);
			  cache 0x1B, 0xC0(%1);
			  cache 0x1B, 0x100(%1);
			  cache 0x1B, 0x140(%1);
			  cache 0x1B, 0x180(%1);
			  cache 0x1B, 0x1C0(%1);"
			  ::"r"(nand_buf));

	if (mode & NO_AUTO_USER_ECC) {
		if ((mode & USER_ECC_IN_SPARE) && spare) {
			c = spare;
			eccval = (c[2] << 16) | (c[1] << 8) | c[0];
		} else
			eccval = 0xFFFFFFFF;
		_sw(eccval, 0xBFF00800);
		sceNandHwReg->ctrl &= ~0x00020000;
	} else
		sceNandHwReg->ctrl |= 0x00020000;

	if (spare) {
		_spare = spare;
		if (mode & USER_ECC_IN_SPARE)
			_spare++;
		if (mode & NO_AUTO_SPARE_ECC) {
			_sw(_spare[0], 0xBFF00900);
			_sw(_spare[1], 0xBFF00904);
			_sw(_spare[2], 0xBFF00908);
		} else {
			eccbuf[0] = _spare[0];
			eccbuf[1] = _spare[1];
			eccval = sceNandCalcEcc(eccbuf); //sceNand_driver_EF55F193  
			eccval |= 0xFFFFF000;
			_sw(eccbuf[0], 0xBFF0900);
			_sw(eccbuf[1], 0xBFF0904);
			_sw(eccval, 0xBFF0908);
		}
	} else {
		_sw(0xFFFFFFFF, 0xBFF00900);
		_sw(0xFFFFFFFF, 0xBFF00904);
	}

	sceNandHwReg->dma_addr = ppn << 10;
	sceNandHwReg->dma_intr &= ~0x00000103;
	sceNandHwReg->dma_ctr; = 0x00000303;

	return 0;
}

/* sub_00009EEC */
int
sceNandWaitIntr(void)
{
	volatile int i;

	if (sceNandIsReady()) //sub_00008C88
		return 0;

	if (sceKernelIsIntrContext()) { //InterruptManagerForKernel_FE28C6D9
		while (!sceNandIsReady())
			for (i = 0; i < 999; i++);
		return 0;
	}

	intr = sceKernelCpuSuspendIntr(); //InterruptManagerForKernel_092968F4
	sceNandInfo.state = STATE_IDLE;
	sceNandInfo.len = 0;

	if (sceNandIsReady()) { //sub_00008C88
		sceNandInfo.state = STATE_INVALID;
		sceKernelCpuResumeIntrWithSync(intr); //InterruptManagerForKernel_3B84732D
		return 0;
	}

	sceNandHwReg->dma_intr &= 0x00000203;
	if (sceNandIsReady()) { //sub_00008C88
		sceNandHwReg->dma_intr = (sceNandHwReg->dma_intr & (~0x1))
			| 0x00000202;
		sceNandInfo.state = STATE_INVALID;
		sceKernelCpuResumeIntrWithSync(intr); //InterruptManagerForKernel_3B84732D
		return 0;
	}
	sceKernelCpuResumeIntrWithSync(intr); //InterruptManagerForKernel_3B84732D
	sceKernelWaitEventFlag(sceNandInfo.event_id, 1, 33, 0, 0); //ThreadManForKernel_402FCF22

	return 0;
}

/* sceNand_driver_D897C343 */
int
sceNandDetectChip(void)
{
	u8 id[16];
	int ret;
	int i;

	if ((ret = sceNandReadId(id, 2)) < 0) //sceNand_driver_FCDF7610
		return ret;

	sceNandProperties.total_blocks = 0;
	sceNandProperties.device = id[1];
	sceNandProperties.page_size = 0;
	sceNandProperties.pages_per_block = 0;
	sceNandProperties.manufacturer = id[0];

	for (i = 0; i < sizeof(sceNandChips) / sizeof(SceNandChipProperties_t); i++) {
		if (id[0] == sceNandChips[i].id[0] &&
				id[1] = sceNandChips[i].id[1]) {
			sceNandProperties.pages_per_block = sceNandChips[i].pages_per_block;
			sceNandProperties.total_blocks = sceNandChips[i].total_blocks;
			sceNandProperties.maker_blocks = sceNandChips[i].make_blocks;
			sceNandProperties.page_size = sceNandChips[i].page_size;
			if (0x004FFFFF >= sceSysregGetTachyonVersion()) //sceSysreg_driver_E2A5D1EE
				sceNandProperties.maker_blocks = 1;

			if (sceNandProperties.page_size != 0x200 ||
					sceNandProperties.pages_per_block != 0x20)
				return 0x80000003;

			return 0;
		}
	}

	return 0x80000004;
}

/* sceNand_driver_B2B021E5 */
int
sceNandWriteBlockWithVerify(u32 ppn, void *user, void spare)
{
	int ret;
	int i = 0;

	do {
		ret = sceNandEraseBlockWithRetry(ppn); //sceNand_driver_8933B2E0
		i++;
		if (ret >= 0)
			ret = sceNandWritePages(ppn, 
					user, 
					spare, 
					sceNandProperties.pages_per_block); //sceNand_driver_8AF0AB9F 
		if (ret < 0)
			return ret;
		if ((ret = sceNandVerifyBlockWithRetry(ppn, 
						user, 
						spare)) >= 0) //sceNand_driver_5AC02755 
			return 0;
	} while (i < 4);

	return ret;
}

/* sceNand_driver_C32EA051 */
int
sceNandReadBlockWithRetry(u32 ppn, void *user, void *spare)
{
	int ret;
	int i = 0;

	do {
		if ((ret = sceNandReadPages(ppn, 
						user, 
						spare, 
						sceNandProperties.pages_per_block)) >= 0) //sceNand_driver_89BDCA08 
			return ret;
		i++;
	} while (i < 4);

	return ret;
}

/* sceNand_driver_5AC02755 */
int
sceNandVerifyBlockWithRetry(u32 ppn, void *user, void *spare)
{
	static char page_user[0x200]; /* 0x0000DA04 */
	static char page_spare[8]; /* 0x0000DC04 */

	u32 offset = 0;
	int i, j, ret;

	if (sceNandProperties.pages_per_block <= 0)
		return 0;

	j = 0;
	do {
		for (i = 0; i < sceNandProperties.pages_per_block; i++) {
			ret = sceNandReadPages(ppn + i, page_user, page_spare, 1);
			if (ret < 0)
				break;
			if (user) {
				if (memcmp(page_user,
						(void *) ((u32) user + sceNandProperties.page_size * i),
						sceNandProperties.page_size))
					return 0x80230006;
			}
			if (spare) {
				if (memcmp(page_spare, (void *) ((u32) spare + offset), 8))
					return 0x80230006;
			}
			offset += 0xC;
		}
		j++;
	} while (j < 4);

	return ret;
}

/* sceNand_driver_8933B2E0 */
int
sceNandEraseBlockWithRetry(u32 ppn)
{
	int i, ret;

	if (ppn % sceNandProperties.pages_per_block)
		return 0x80230008;

	i = 0;
	do {
		ret = sceNandEraseBlock(ppn); //sceNand_driver_EB0A0022
	} while (++i < 4 && ret < 0);

	return ret;
}

/* sceNand_driver_01F09203 */
int
sceNandIsBadBlock(u32 ppn)
{
	SceNandSpare_t spare;
	int ret, i;

	if (ppn % sceNandProperties.pages_per_block)
		return 0x80230008;

	i = 0;
	do {
		if ((ret = sceNandReadPagesRawAll(ppn, 0, &spare, 1)) >= 0) {
			if (spare.block_stat = 0xFF)
				return 0;
			else
				return 1;
		}
		i++;
	} while (i < 4);

	return ret;
}

/* sceNand_driver_C29DA136 */
int
sceNandDoMarkAsBadBlock(u32 ppn)
{
	static char page_user[0x200]; /* 0x0000DC10 */
	static char page_spare[8]; /* 0x0000DE10 */

	int i, ret, retry;

	if (ppn % sceNandProperties.pages_per_block)
		return 0x80230008;

	if (sceNandProperties.pages_per_block <= 0)
		return 0;

	memset(page_user, 0, 0x200); //SysclibForKernel_10F3BB61
	memset(page_spare, 0xFF, 0xC);
	page_spare[1] = 0xF0;
	retry = 0; /* should be set within for loop? */

	for (i = 0; i < sceNandProperties.pages_per_block; i++) {
		do {
			if (sceNandWritePages(ppn + i,
						page_user,
						page_spare,
						1) >= 0) //sceNand_driver_8AF0AB9F
				break;
		} while (retry++ < 4);
	}

	return 0;
}

/* sceNand_driver_3F76BC21 */
int
sceNandDumpWearBBMSize(void)
{
	int i, ret, bad;

	if (sceNandproperties.total_blocks <= 0)
		return 0;

	bad = 0;
	for (i = 0; i < sceNandProperties.total_blocks; i++) {
		if ((ret =
				sceNandDetectChipMakersBBM(i * sceNandProperties.pages_per_blocks)) < 0) //sceNand_driver_2FF6081B
			return ret;
		if (ret == 1)
			bad++;
		else {
			if (sceNandTestBlock(i * sceNandProperties.pages_per_blocks) < 0) {
				sceNandDoMarkAsBadBlock(i * sceNandProperties.pages_per_blocks); //sceNand_driver_C29DA136
				bad++;
			}
		}
	}

	return bad;
}

/* sceNand_driver_EBA0E6C6 */
int
sceNandCountChipMakersBBM(void)
{
	int i, bad, ret;

	if (sceNandproperties.total_blocks <= 0)
		return 0;

	bad = 0;
	for (i = 0; i < sceNandProperties.total_blocks; i++) {
		if ((ret =
				sceNandDetectChipMakersBBM(i * sceNandProperties.pages_per_blocks)) < 0) //sceNand_driver_2FF6081B
			return ret;
		if (ret == 1)
			bad++;
	}

	return bad;
}

/* sceNand_driver_2FF6081B */
int
sceNandDetectChipMakersBBM(u32 ppn)
{
	static char page_user[0x200]; /* 0x0000DE1C */
	static SceNandSpare_t spare; /* 0x0000E01C */

	int retry, offset;

	if (sceNandProperties.maker_blocks == 0)
		return 0;

	retry = 0;
	offset = 0;

	do {
		if ((ret = sceNandReadPagesRawAll(ppn + offset,
						page_user,
						&spare,
						1)) >= 0) { //sceNand_driver_C478C1DE
			if (spare.block_stat != 0xFF)
				return 1;
			offset++;
			retry = 0;
			if (offset >= sceNandProperties.maker_blocks)
				return 0;
		}
	} while (retry++ < 4);

	return ret;
}

/* sceNand_driver_2674CFFE */
int
sceNandEraseAllBlock(void)
{
	int i;

	if (sceNandProperties.total_blocks <= 0)
		return 0;

	for (i = 0; i < sceNandProperties.total_blocks; i++) {
		ret = sceNandEraseBlockWithRetry(i * sceNandProperties.pages_per_block); //sceNand_driver_8933B2E0
		if (ret < 0)
			bad++;
		if (ret == 0x80230007)
			return 0x80230007;
	}

	return bad;
}

/* sceNand_driver_9B2AC433 */
int
sceNandTestBlock(u32 ppn)
{
	u8 user_pages[0x4000]; /* too much space on stack! */
	u8 spare_pages[0x200];
	int ret, i, j;
	u8 *c;
	u32 *p;

	if (ppn % sceNandProperties.pages_per_block)
		return 0x80230008;
	if ((ret = sceNandEraseBlockWithRetry(ppn)) < 0)
		return ret;
	if ((ret = sceNandReadPagesRawAll(ppn,
			user_buf,
			spare_buf,
			sceNandProperties.pages_per_block)) < 0) //sceNand_driver_C478C1DE
		return ret;

	if (sceNandProperties.pages_per_block > 0) {
		p = (u32 *) user_pages;
		c = (u8 *) spare_pages;

		for (i = 0; i < sceNandProperties.pages_per_block; i++) {
			for (j = 0; j < 0x200 / 4; j++) {
				if (p[j] != 0xFFFFFFFF)
					return 0x80230004;
			}
			for (j = 4; j < 0x10; j++) {
				if (c[j] != 0xFF)
					return 0x80230004;
			}
			p += 0x200 / 4;
			c += 0x10;
		}
	}

	for (i = 0; i < 2; i++) {
		if (i == 0) {
			memset(user_pages, 0, 0x4000); //SysclibForKernel_10F3BB61
			memset(spare_pages, 0, 0x180);
		} else if (i == 1) {
			memset(user_pages, 0xFF, 0x4000);
			memset(spare_pages, 0xFF, 0x180);
		}
		if ((ret = sceNandWriteBlockWithVerify(ppn,
						user_pages,
						spare_pages)) < 0) //sceNand_driver_B2B021E5
			break;
	}

	sceNandEraseBlockWithRetry(ppn); //sceNand_driver_8933B2E0

	return ret;
}

/* sceNand_driver_CE9843E6 */
int
sceNandGetPageSize(void)
{
	return sceNandProperties.page_size;
}

/* sceNand_driver_B07C41D4 */
int
sceNandGetPagesPerBlock(void)
{
	return sceNandProperties.pages_per_block;
}

/* sceNand_driver_C1376222 */
int
sceNandGetTotalBlocks(void)
{
	return sceNandProperties.total_blocks;
}

/* sceNand_driver_716CD2B2 */
int
sceNandWriteBlock(u32 ppn, void *user, void *spare)
{
	int ret;

	if ((ret = sceNandEraseBlockWithRetry(ppn)) < 0) //sceNand_driver_8933B2E0
		return ret;

	return sceNandWritePages(ppn,
			user,
			spare,
			sceNandProperties.pages_per_block); //sceNand_driver_8AF0AB9F
}

/* sceNand_driver_EF55F193 */
int
sceNandCalcEcc(u8 *buf)
{
	int t0, t1, t2, t3,t4, t5, t6, t7, t8;
	int s0, s1, s2, s3, s4, s5;
	int a0, a1, a2, a3;

	t1 = buf[0];
	v0 = buf[1];
	s0 = buf[2];
	t5 = buf[3];
	t7 = buf[4];
	t3 = t1 ^ v0;
	s2 = buf[5];
	a1 = s0 ^ t3;
	s3 = buf[6];
	t4 = t5 ^ a1;
	s1 = buf[7];
	a0 = t7 ^ t4;
	t6 = t7 ^ s2;
	v1 = s2 ^ a0;
	t0 = s0 ^ t5;
	t2 = s3 ^ t6;
	a2 = s3 ^ v1;
	t9 = s1 ^ t2;
	t8 = s1 ^ a2;
	t6 = s3 ^ t0;
	a1 = t8 & 0xFF;
	t2 = v0 ^ t5;
	t8 = t4 & 0xFF;
	t0 = t7 ^ t3;
	v1 = s1 ^ t6;
	t4 = t9 & 0xFF;
	a2 = v1 & 0xFF;
	t6 = t1 ^ s0;
	v1 = 0x6996;
	t1 = s2 ^ t2;
	s0 = s2 ^ t0;
	v0 = t8 >> 4;
	t2 = a1 & 0xCC;
	t3 = t8 & 0xF;
	s2 = t4 & 0xF;
	t9 = t4 >> 4;
	t4 = v1 >> t9;
	a0 = v1 >> s2;
	t9 = s0 & 0xFF;
	t8 = a1 >> 4;
	a3 = v1 >> t3;
	t5 = a1 & 0xC;
	t3 = v1 >> v0;
	s0 = t2 >> 4;
	v0 = a1 & 0xF;
	t2 = t7 ^ t6;
	s2 = a2 & 0xF;
	t6 = s1 ^ t1;
	s1 = a2 >> 4;
	t1 = t6 & 0xFF;
	s0 = v1 >> s0;
	t6 = v1 >> t5;
	a2 = v1 >> s2;
	t5 = v1 >> v0;
	s2 = v1 >> s1;
	v0 = v1 >> t8;
	s1 = a1 & 0xAA;
	t8 = s2 ^ t2;
	a3 = a3 ^ t3;
	s3 = t9 >> 4;
	t3 = a0 ^ t4;
	t7 = (a1 & 0xC) >> 2;
	t4 = a1 & 0x3;
	t0 = t9 & 0xF;
	t2 = t8 & 0xFF;
	t9 = t6 ^ s0;
	t7 = v1 >> t7;
	s3 = v1 >> s3;
	a0 = t3 & 0x1;
	a2 = a2 ^ s2;
	t8 = s1 >> 4;
	t4 = v1 >> t4;
	s0 = t5 & 0x1;
	v0 = v0 & 0x1;
	s2 = t1 >> 4;
	t0 = v1 >> t0;
	t3 = a1 & 0xA;
	a3 = a3 & 0x1;
	t1 = t1 & 0xF;
	s1 = v1 >> t8;
	t4 = t4 ^ t7;
	t0 = t0 ^ s3;
	t8 = t9 & 0x1;
	t5 = s0 << 2;
	t1 = v1 >> t1;
	s2 = v1 >> s2;
	t7 = a1 & 0x55;
	t3 = v1 >> t3;
	t9 = v0 << 8;
	s0 = t2 >> 4;
	s3 = a0 << 1;
	a3 = a3 << 5;
	t2 = t2 & 0xF;
	a2 = a2 & 0x1;
	v0 = t9 | t5;
	a0 = s3 | a3;
	t9 = t1 ^ s2;
	t3 = t3 ^ s1;
	s4 = t7 >> 4;
	s1 = t4 & 0x1;
	s2 = t8 << 7;
	s0 = v1 >> s0;
	t8 = v1 >> t2;
	t5 = t0 & 0x1;
	a2 = a2 << 10;
	a1 = a1 & 0x5;
	a3 = v0 | s2;
	t6 = v1 >> a1;
	s2 = t8 ^ s0;
	s0 = v1 >> s3;
	t8 = a0 | a2;
	s1 = t9 & 0x1;
	t9 = t5 << 4;
	t7 = a3 | a2;
	t5 = t6 ^ s0;
	v1 = s2 & 0x1;
	s0 = s3 << 6;
	t6 = s1 << 9;
	t4 = t8 | t9;
	t0 = t7 | s0;
	a3 = v1 << 3;
	a2 = t4 | t6;
	t1 = t5 & 0x1;
	v1 = t0 | t1;
	a1 = a2 | a3;
	
	return v1 | a1;
}

/* sceNand_driver_18B78661 */
int
sceNandVerifyEcc(u8 *buf, u16 ecc)
{
	return sceNandCorrectEcc2(buf, ecc, 0);
}

/* sceNand_driver_88CC9F72 */
int
sceNandCorrectEcc(u8 *buf, u16 ecc)
{
	return sceNandCorrectEcc2(buf, ecc, 1);
}

/* sub_0000AD70 */
int
sceNandCorrectEcc2(u8 *buf, u16 ecc, int mode)
{
	int t0, t1, t2, t3,t4, t5, t6, t7, t8;
	int s0, s1, s2, s3, s4, s5;
	int a0, a1, a2, a3;

	s3 = buf[0];
	v0 = buf[1];
	t4 = buf[2];
	t2 = buf[3];
	s0 = buf[4];
	t7 = s3 ^ v0;
	s1 = buf[5];
	t9 = t4 ^ t7;
	s5 = buf[6];
	t6 = t2 ^ t9;
	v1 = s0 ^ t6;
	t9 = buf[7];
	a3 = s0 ^ s1;
	t5 = s1 ^ v1;
	s2 = t4 ^ t2;
	t0 = s5 ^ t5;
	t3 = s5 ^ a3;
	t5 = s5 ^ s2;
	s4 = t9 ^ t0;
	a3 = t9 ^ t3;
	a0 = s4 & 0xFF;
	s2 = v0 ^ t2;
	s4 = t6 & 0xFF;
	t3 = s0 ^ t7;
	t6 = a3 ^ 0xFF;
	t7 = t9 ^ t5;
	v0 = 0x6996;
	t0 = t7 & 0xFF;
	a3 = s4 >> 4;
	t7 = s3 ^ t4;
	t5 = s4 & 0x0F;
	s3 = s1 ^ s2;
	s4 = t6 & 0x0F;
	s2 = s1 ^ t3;
	t4 = t6 >> 4;
	s1 = a0 & 0xCC;
	t6 = v0 >> t4;
	v1 = v0 >> s4;
	t3 = s2 & 0xFF;
	t4 = a0 >> 4;
	t1 = v0 >> t5;
	s2 = s1 >> 4;
	t5 = v0 >> a3;
	s1 = s0 ^ t7;
	a3 = a0 & 0x0F;
	s0 = t9 ^ s3;
	t7 = a0 & 0x0C;
	s3 = t0 >> 4;
	s4 = t0 & 0x0F;
	t2 = s0 & 0xFF;
	s2 = v0 >> s2;
	s0 = v0 >> t7;
	t0 = v0 >> s4;
	t7 = v0 >> a3;
	s4 = v0 >> s3;
	a3 = v0 >> t4;
	t1 = t1 ^ t5;
	t4 = s5 ^ s1;
	t5 = v1 ^ t6;
	s5 = t3 >> 4;
	s3 = a0 & 0xAA;
	t6 = a0 & 0x03;
	s1 = (a0 >> 2) & 0x03;
	t3 = t3 & 0x0F;
	s1 = v0 >> s1;
	s5 = v0 >> s5;
	s0 = s0 ^ s2;
	t0 = t0 ^ s4;
	t4 = t4 & 0xFF;
	t6 = v0 >> t6;
	s2 = t7 & 0x01;
	s4 = t2 >> 4;
	t3 = v0 >> t3;
	v1 = t5 & 0x01;
	s3 = s3 >> 4;
	t5 = a0 & 0x0A;
	a3 = a3 & 0x01;
	s0 = s0 ^ s2;
	t0 = t0 ^ s4;
	t4 = t4 & 0xFF;
	t6 = v0 >> t6;
	s2 = t7 & 0x01;
	s4 = t2 >> 4;
	t3 = v0 >> t3;
	v1 = t5 & 0x01;
	s3 = s3 >> 4;
	t5 = a0 & 0x0A;
	a3 = a3 & 0x01;
	t1 = t1 & 0x01;
	t2 = t2 & 0x0F;
	t6 = t6 ^ s1;
	t3 = t3 ^ s5;
	s3 = v0 >> s3;
	s5 = v0 >> t5;
	t7 = s2 << 2;
	s4 = v0 >> s4;
	a3 = a3 << 8;
	s2 = t4 >> 4;
	t2 = v0 >> t2;
	t1 = t1 << 5;
	s1 = a0 & 0x55;
	s0 = s0 & 0x01;
	t0 = t0 & 0x01;
	v1 = v1 << 12;
	t4 = t4 & 0x0F;
	t5 = s5 ^ s3;
	t2 = t2 ^ s4;
	s5 = a3 | t7;
	s4 = t6 & 0x01;
	s2 = v0 >> s2;
	t7 = t3 & 0x01;
	v1 = v1 | t1;
	s1 = s1 >> 4;
	t1 = v0 >> t4;
	s0 = s0 << 7;
	t0 = t0 << 10;
	a0 = a0 & 0x05;
	s3 = s5 | s0;
	s5 = t1 ^ s2;
	s0 = s4 << 1;
	s2 = v0 >> a0;
	t1 = v1 | t0;
	v0 = v0 >> s1;
	t0 = t5 & 0x01;
	s1 = t7 << 4;
	s4 = t2 & 0x01;
	t7 = s2 ^ v0;
	t6 = s5 & 0x01;
	s2 = s3 | s0;
	s0 = t1 | s1;
	s3 = t0 << 6;
	s1 = s4 << 9;
	v0 = s0 | s1;
	t3 = s2 | s3;
	t2 = t7 & 0x01;
	t1 = t6 << 3;
	t0 = t3 | t2;
	a0 = v0 | t1;
	v0 = t0 | a0;
	a3 = v0 ^ ecc;

	if (a3 == 0)
		return 0;

	a0 = a3 >> 6;
	a1 = a0 ^ a3;
	a3 = a1 & 0x3F;
	a1 = 0;
	a0 = 0;

	if (a3 != 0x3F) {
		do {
			s0 = a3 >> a0;
			s5 = s0 & 0x01;
			a1 += s5;
		} while (++a0 < 6);

		return (a1 ^ 0x1) ? 0xFFFFFFFD : 0xFFFFFFFE;
	}

	if (mode != 0)
		buf[7] = buf[7] ^ 0x80;

	return -1;
}
