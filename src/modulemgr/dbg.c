#include <common_imp.h>

#include <iofilemgr_kernel.h>

#include <stdarg.h>
#include <stdio.h>

#define DEBUG_FILE "flash0:/test.log"

void init_flash(void)
{
    s32 status;

    status = sceIoUnassign("flash0:");
    if (status < 0) {
        dbg_fbfill(1, 255, 1);
        return;
    }
    status = sceIoAssign("flash0:", "lflash0:0,0", "flashfat0:", 0, NULL, 0);
    if (status < 0) {
        dbg_fbfill(1, 1, 255);
        return;
    }
    status = sceIoUnassign("flash1:");
    if (status < 0) {
        dbg_fbfill(1, 255, 1);
        return;
    }
    status = sceIoAssign("flash1:", "lflash0:0,1", "flashfat1:", 0, NULL, 0);
    if (status < 0) {
        dbg_fbfill(1, 1, 255);
        return;
    }
}

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

typedef void(*prnt_callback)(void *ctx, int ch);
int prnt(prnt_callback cb, void *ctx, const char *fmt, va_list args); /* don't use the SDK header because it's wrong: prnt returns an int */

int my_vsnprintf(char *str, int size, const char *format, va_list ap)
{
    SnprintfCtx ctx = { size - 1, 0, str };
    return prnt((prnt_callback)mysnprintf_char, &ctx, format, ap);
}

//int ms_append(const void *data, int size)
//{
//
//    FILE *file = NULL;
//
//    if ((file = fopen(DEBUG_FILE, "w+")) == NULL) {
//        dbg_fbfill(255, 1, 1);
//    }
//
//    int wrData = (int)fwrite(data, sizeof(char), size, file);
//    if (wrData < size) {
//        dbg_fbfill(1, 255, 1);
//    }
//    
//    fclose(file);
//
//    return 0;
//}

int io_append(const void *data, int size)
{
    SceUID id = sceIoOpen(DEBUG_FILE, SCE_O_WRONLY | SCE_O_CREAT | SCE_O_TRUNC, 0777);
    if (id < 0) {
        dbg_fbfill(255, 1, 1);
        return -1;
    }
    sceIoLseek(id, 0, SCE_SEEK_END);
    int count = sceIoWrite(id, data, size);

    sceIoClose(id);
    return count;
}

int(*fat_append)(const char*, int);

int dbgInit(int eraseLog) {
    init_flash();

    fat_append = (int (*)(const char*, int))io_append;

    if (eraseLog) {
        /*
        FATFS FileSystem;
        FIL FileObject;
        if (fmount(0, &FileSystem) == 0 && fopen(&FileObject, DEBUG_FILE, FA_CREATE_ALWAYS) == 0)
            fclose(&FileObject);
            */
    }
    return 0;
}

void dbgPrintf(const char *format, ...)
{
    if (fat_append == NULL)
        return;
    va_list ap;
    char buf[512];
    va_start(ap, format);
    int size = my_vsnprintf(buf, 512, format, ap);
    if (size > 0) {
        if (fat_append != NULL)
            fat_append(buf, size);
    }
    va_end(ap);
}