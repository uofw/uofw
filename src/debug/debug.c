#include <stdarg.h>
#include <stdio.h>

#define DEBUG

#include "memstk.h"
#include "syscon.h"
#include "tff/tff.h"

#define DEBUG_FILE "uofw/log.txt"
#define IO_DEBUG_FILE "ms0:/" DEBUG_FILE

#include <common_imp.h>

#include <display.h>
#include <iofilemgr_kernel.h>
#include <syscon.h>

#include "scr_printf.h"

typedef struct {
    int size;
    int written;
    char *str;
} SnprintfCtx;

void mysnprintf_char(SnprintfCtx *ctx, int c)
{
    if (c >= 256 || ctx->written >= ctx->size)
        *ctx->str = '\0';
    else {
        *(ctx->str++) = c;
        ctx->written++;
    }
}

typedef void (*prnt_callback)(void *ctx, int ch);
int prnt(prnt_callback cb, void *ctx, const char *fmt, va_list args); /* don't use the SDK header because it's wrong: prnt returns an int */

int my_vsnprintf(char *str, int size, const char *format, va_list ap)
{   
    SnprintfCtx ctx = { size - 1, 0, str };
    return prnt((prnt_callback)mysnprintf_char, &ctx, format, ap);
}

int ms_append(const void *data, int size)
{
    FATFS FileSystem;
    FIL FileObject;
    WORD written;
    int num_write;
   
    if (f_mount(0, &FileSystem) != 0)
        return -1;
   
    if (f_open(&FileObject, DEBUG_FILE, FA_WRITE | FA_OPEN_ALWAYS) != 0)
        return -1;
   
    f_lseek(&FileObject, FileObject.fsize); /* append to file (move to its end) */
    while (size)
    {
        num_write = size > 0x8000 ? 0x8000 : size;
        if (f_write(&FileObject, data, num_write, &written) != 0 || num_write != written)
            return -1;
        data += num_write;
        size -= num_write;
    }
    f_close(&FileObject);

    return 0;
}

int io_append(const void *data, int size)
{
    SceUID id = sceIoOpen(IO_DEBUG_FILE, SCE_O_CREAT | SCE_O_WRONLY, 0777);
    if (id < 0)
        return -1;
    sceIoLseek(id, 0, SEEK_END);
    int count = sceIoWrite(id, data, size);
    sceIoClose(id);
    return count;
}

int (*fat_append)(const void*, int);
int (*fb_append)(const char*, int);

void dbg_init(int eraseLog, FbMode fbMode, FatMode fatMode)
{
    switch (fatMode) {
    case FAT_HARDWARE:
    case FAT_AFTER_SYSCON:
        if (fatMode == FAT_HARDWARE)
        {
            pspSyscon_init();
            pspSysconCtrlLED(0,1);
            pspSysconCtrlLED(1,1);
            pspSysconCtrlMsPower(1);
        }
        else
        {
            sceSysconCtrlLED(0,1);
            sceSysconCtrlLED(1,1);
            sceSysconCtrlMsPower(1);
        }
        pspMsInit();
    case FAT_NOINIT:
        fat_append = ms_append;
        if (eraseLog)
        {
            FATFS FileSystem;
            FIL FileObject;
            if (f_mount(0, &FileSystem) == 0 && f_open(&FileObject, DEBUG_FILE, FA_CREATE_ALWAYS) == 0)
                f_close(&FileObject);
        }
        break;

    case FAT_AFTER_FATMS:
        fat_append = io_append;
        if (eraseLog)
        {
            SceUID fd;
            if ((fd = sceIoOpen(IO_DEBUG_FILE, SCE_O_CREAT | SCE_O_TRUNC, 0777)) > 0)
                sceIoClose(fd);
        }
        break;

    default:
        fat_append = NULL;
        break;
    }

    switch (fbMode) {
    case FB_HARDWARE:
        pspDebugScreenInitEx((void*)0x44000000, PSP_DISPLAY_PIXEL_FORMAT_8888, 0);
        fb_append = pspDebugScreenPrintData;
        break;

    case FB_AFTER_DISPLAY:
        pspDebugScreenInit();
        fb_append = pspDebugScreenPrintData;
        break;

    default:
        fb_append = NULL;
        break;
    }
}

void dbg_printf(const char *format, ...)
{
    if (fat_append == NULL && fb_append == NULL)
        return;
    va_list ap;
    char buf[512];
    va_start(ap, format);
    int size = my_vsnprintf(buf, 512, format, ap);
    if (size > 0) {
        if (fat_append != NULL) 
            fat_append(buf, size);
        if (fb_append != NULL)
            fb_append(buf, size);
    }
    va_end(ap);
}

void dbg_puts(const char *str)
{
    if (fat_append == NULL && fb_append == NULL)
        return;
    int count = 0;
    const char *curStr = str;
    while (*(curStr++) != '\0')
        count++;

    if (fat_append != NULL)
        fat_append(str, count);
    if (fb_append != NULL)
        fb_append(str, count);
}

