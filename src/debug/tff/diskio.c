#include <string.h>
#include "tff.h"		/* Tiny-FatFs declarations */
#include "diskio.h"		/* Include file for user provided disk functions */
#include "../memstk.h"

DSTATUS disk_initialize (BYTE Drive)
{
	return 0;
}

DSTATUS disk_status (BYTE Drive)
{
	return 0;
}

DRESULT disk_read (
  BYTE Drive,
  BYTE* Buffer,
  DWORD SectorNumber,
  BYTE SectorCount
)
{
	while(SectorCount--)
	{
		if( pspMsReadSector(SectorNumber,Buffer) < 0) return RES_ERROR;
		SectorNumber++;
		Buffer+=0x200;
	}
	return RES_OK;
}

#if _FS_READONLY == 0

DRESULT disk_write (
  BYTE Drive,
  const BYTE* Buffer,
  DWORD SectorNumber,
  BYTE SectorCount
)
{
	while(SectorCount--)
	{
		if( pspMsWriteSector(SectorNumber,Buffer) < 0) return RES_ERROR;
		SectorNumber++;
		Buffer+=0x200;
	}
	return RES_OK;
}

DRESULT disk_ioctl (BYTE Drive, BYTE cmd, void* ptr)
{
	return RES_OK;
}

DWORD get_fattime (void)
{
	return 0;
}

#endif


