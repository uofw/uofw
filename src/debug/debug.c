#include <stdarg.h>
#include <pspkerneltypes.h>

#include "memstk.h"
#include "syscon.h"
#include "tff/tff.h"

#define DEBUG_FILE "uofw/log.txt"

typedef struct {
    int size;
    int written;
    char *str;
} SnprintfCtx;

void snprintf_char(SnprintfCtx *ctx, int c)
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

int vsnprintf(char *str, int size, const char *format, va_list ap)
{   
    SnprintfCtx ctx = { size - 1, 0, str };
    return prnt((prnt_callback)snprintf_char, &ctx, format, ap);
}

int ms_append(const char *path, const void *data, int size)
{
    FATFS FileSystem;
    FIL FileObject;
    WORD written;
    int num_write;
   
    if (f_mount(0, &FileSystem) != 0)
        return -1;
   
    if (f_open(&FileObject, path, FA_WRITE | FA_OPEN_ALWAYS) != 0)
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

void dbg_init(void)
{
    pspSyscon_init();
    pspSysconCtrlLED(0,1);
    pspSysconCtrlLED(1,1);
    pspSysconCtrlMsPower(1);
    pspMsInit();
    ms_append("test.txt", "test", 4);
}

void dbg_printf(const char *format, ...)
{
    va_list ap;
    char buf[512];
    va_start(ap, format);
    int size = vsnprintf(buf, 512, format, ap);
    if (size > 0)
        ms_append(DEBUG_FILE, buf, size - 1);
    va_end(ap);
}

