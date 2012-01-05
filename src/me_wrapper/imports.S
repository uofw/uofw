        .set noreorder

#include "pspstub.s"

	STUB_START	"ThreadManForKernel",0x00010011,0x00040005
	STUB_FUNC     0xB7D098C6,sceKernelCreateMutex
	STUB_FUNC     0xB011B11F,sceKernelLockMutex
    STUB_FUNC     0x6B30100F,sceKernelUnlockMutex
    STUB_FUNC     0x0DDCD2C9,sceKernelTryLockMutex
	STUB_END
	STUB_START	"sceWmd_driver",0x00090011,0x00010005
	STUB_FUNC	  0x7A0E484C,sceWmd_driver_7A0E484C
	STUB_END
	STUB_START	"sceDdr_driver",0x00010011,0x00010005
	STUB_FUNC	  0x87D86769,sceDdrFlush
	STUB_END
	STUB_START	"sceSyscon_driver",0x00010011,0x00010005
	STUB_FUNC	  0x9BC5E33B,sceSysconCtrlTachyonAvcPower
	STUB_END
	STUB_START	"sceLFatFs_driver",0x00010011,0x00010005
	STUB_FUNC	  0x933F6E29,sceLfatfsWaitReady
	STUB_END	
	STUB_START	"UtilsForKernel",0x00090011,0x00010005
	STUB_FUNC	  0x6C6887EE,UtilsForKernel_6C6887EE
	STUB_END	
	STUB_START	"SysMemForKernel",0x00010011,0x00040005
	STUB_FUNC	  0xB4F00CB5,sceKernelGetCompiledSdkVersion
	STUB_FUNC	  0xFDC97D28,SysMemForKernel_FDC97D28
	STUB_FUNC	  0xD3CA555C,SysMemForKernel_D3CA555C
	STUB_FUNC	  0x22A114DC,sceKernelMemset32
	STUB_FUNC	  0xC4EEAF20,sceKernelQueryMemoryPartitionInfo
	STUB_FUNC	  0x7158CE7E,sceKernelAllocPartitionMemory
	STUB_END	
	STUB_START	"sceSysreg_driver",0x00010011,0x000c0005
	STUB_FUNC	  0x9BC5E33B,sceSysregGetTachyonVersion
	STUB_FUNC	  0xA5CC6025,sceSysregPllGetFrequency
	STUB_FUNC	  0x7AA8A8BE,sceSysregIntrEnd
	STUB_FUNC	  0x457FEBA9,sceSysregMeResetEnable
	STUB_FUNC	  0x48F1C4AD,sceSysregMeResetDisable
	STUB_FUNC	  0x1A27B224,sceSysregVmeResetEnable
	STUB_FUNC	  0xB73D3619,sceSysregVmeResetDisable
	STUB_FUNC	  0x0AE8E549,sceSysregAvcResetEnable
	STUB_FUNC	  0x55FF02E9,sceSysregAvcResetDisable
	STUB_FUNC	  0x38527743,sceSysregMeBusClockEnable
	STUB_FUNC	  0xC4C21CAB,sceSysregMeBusClockDisable
	STUB_FUNC	  0x61FAE917,sceSysregInterruptToOther
	STUB_END	
	
