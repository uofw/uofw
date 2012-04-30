#ifndef __PSP_MS__
#define __PSP_MS__

int pspMsInit(void);
int pspMsReadSector(int sector, void *addr);
int pspMsWriteSector(int sector, void *addr);

#endif
