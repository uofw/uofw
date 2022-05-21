#ifndef SCR_PRINTF_H
#define SCR_PRINTF_H

void pspDebugScreenInit(void);
void pspDebugScreenInitEx(void *vram_base, int mode, int setup);
void pspDebugScreenPrintf(const char *fmt, ...) __attribute__((format(printf,1,2)));
void pspDebugScreenPutChar(int x, int y, u32 color, u8 ch);
int pspDebugScreenPrintData(const char *buff, int size);
void pspDebugScreenSetXY(int x, int y);

#endif

