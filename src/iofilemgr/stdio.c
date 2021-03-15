/* Copyright (C) 2011, 2012 The uOFW team
   See the file COPYING for copying permission.
*/

#include <stdarg.h>

#include <common_imp.h>

#include "sysmem_kdebug.h"
#include "sysmem_kernel.h"
#include "sysmem_sysclib.h"
#include "iofilemgr_kernel.h"
#include "threadman_kernel.h"

#define STDIN  0
#define STDOUT 1
#define STDERR 2

int _sceTtyProxyDevRead(SceIoIob *iob, char *buf, int size);
int _sceTtyProxyDevWrite(SceIoIob *iob, const char *buf, int size);
int _sceTtyProxyDevInit(SceIoDeviceArg *dev);
int _sceTtyProxyDevExit(SceIoDeviceArg *dev);
int _sceTtyProxyDevOpen(SceIoIob *iob, char *file, int flags, SceMode mode);
int _sceTtyProxyDevClose(SceIoIob *iob);
SceOff _sceTtyProxyDevLseek();
int _sceTtyProxyDevIoctl(SceIoIob *iob, unsigned int cmd, void *indata, int inlen, void *outdata, int outlen);

SceIoDrvFuncs g_TtyOps = // 6948
{
    _sceTtyProxyDevInit, // 0BE4
    _sceTtyProxyDevExit, // 0BEC
    _sceTtyProxyDevOpen, // 0BF4
    _sceTtyProxyDevClose, // 0C0C
    _sceTtyProxyDevRead, // 0980
    _sceTtyProxyDevWrite, // 0A74
    _sceTtyProxyDevLseek, // 0C24
    _sceTtyProxyDevIoctl, // 0C34
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL
};

SceIoDrv g_TtyDevTbl = { "ttyproxy", 0x00000003, 0x00000001, "TTY2MsgPipe PROXY", &g_TtyOps }; // 69A0

int g_stdin = -1; // 6AD0
int g_stdout = -1; // 6AD4
int g_stderr = -1; // 6AD8

int g_debugRead; // 6AF0
int g_pipeList[6]; // 6AF4

int g_linePos; // 6C34

int sceTtyProxyInit();
int stdoutReset(int flags, SceMode mode);
int _sceKernelRegisterStdPipe(int fd, SceUID id);

// 0000
int StdioReInit()
{
    dbg_printf("Calling %s\n", __FUNCTION__);
    SceUID *fds[3];
    fds[0] = &g_stdin;
    fds[1] = &g_stdout;
    fds[2] = &g_stderr;
    // 0044
    int i;
    for (i = 0; i < 3; i++)
        *fds[i] = sceIoOpen("dummy_drv_iofile:", 3, 0x1FF);
    g_debugRead = 1;
    sceTtyProxyInit();
    return 0;
}

// 0094
int StdioInit()
{
    dbg_printf("Calling %s\n", __FUNCTION__);
    return StdioReInit();
}

int sceKernelStdin()
{
    dbg_printf("Calling %s\n", __FUNCTION__);
    return g_stdin;
}

int sceKernelStdout()
{
    dbg_printf("Calling %s\n", __FUNCTION__);
    return g_stdout;
}

int sceKernelStderr()
{
    dbg_printf("Calling %s\n", __FUNCTION__);
    return g_stderr;
}

int sceKernelStdoutReopen(const char *file, int flags, SceMode mode)
{
    dbg_printf("Calling %s\n", __FUNCTION__);
    if (g_stdout < 0)
        return 0x80020384;
    return sceIoReopen(file, flags, mode, g_stdout);
}

int sceKernelStderrReopen(const char *file, int flags, SceMode mode)
{
    dbg_printf("Calling %s\n", __FUNCTION__);
    if (g_stderr < 0)
        return 0x80020384;
    return sceIoReopen(file, flags, mode, g_stderr);
}

int sceKernelStdoutReset()
{
    dbg_printf("Calling %s\n", __FUNCTION__);
    int ret = stdoutReset(3, 0x1FF);
    if (ret < 0)
        return ret;
    g_stdout = ret;
    return 0;
}

int sceKernelStderrReset()
{
    dbg_printf("Calling %s\n", __FUNCTION__);
    int ret = stdoutReset(3, 0x1FF);
    if (ret < 0)
        return ret;
    g_stderr = ret;
    return 0;
}

// 019C
int stdoutReset(int flags, SceMode mode)
{
    dbg_printf("Calling %s\n", __FUNCTION__);
    char openDummy = 0;
    if (sceKernelDipsw(58) == 1 && !sceKernelDipsw(59))
        openDummy = 1;
    else
        openDummy = 0;
    // 01CC
    if (openDummy)
    {  
        // 0214
        int ret = sceIoOpen("tty0:", flags, mode);
        if (ret >= 0)
            return ret;
    }
    // 01E8
    return sceIoOpen("dummy_drv_iofile:", flags, mode);
}

// 0244
void printf_char(void *ctx, int ch)
{   
    dbg_printf("Calling %s\n", __FUNCTION__);
    if (ch < 0x200) dbg_printf("print %c\n", ch);
    if (ch == 0x200) {
        *(short*)(ctx + 2) = 0;
        return;
    }
    if (ch == 0x201)
    {   
        // 031C
        short cnt = *(short*)(ctx + 2);
        if (cnt <= 0)
            return;

        if (sceKernelDipsw(59) == 1)
            sceKernelDebugWrite(*(short*)(ctx + 0), ctx + 4, *(short*)(ctx + 2));
        else
        {   
            short fd = *(short*)(ctx + 0);
            if (fd == STDOUT)
                fd = g_stdout;
            // 0348
            if (fd == STDERR)
                fd = g_stderr;
            // 0354
            sceIoWrite(fd, ctx + 4, *(short*)(ctx + 2));
        }
        return;
    }
    if (ch == '\n') {
        // 030C
        printf_char(ctx, '\r');
    }
    // 027C
    (*(short*)(ctx + 2))++;
    *(char*)(ctx + 3 + *(short*)(ctx + 2)) = ch;
    if (ch == '\200')
    {   
        short fd = *(short*)(ctx + 0);
        // 02AC
        if (sceKernelDipsw(59) == 1)
        {   
            // 02F8
            sceKernelDebugWrite(fd, ctx + 4, *(short*)(ctx + 2));
        }
        else
        {   
            if (fd == STDOUT)
                fd = g_stdout;
            // 02C8
            if (fd == STDERR)
                fd = g_stderr;
            // 02D4
            sceIoWrite(fd, ctx + 4, *(short*)(ctx + 2));
        }
        *(short*)(ctx + 2) = 0;
    }
}

int fdprintf(int fd, const char *fmt, ...)
{
    dbg_printf("Calling %s\n", __FUNCTION__);
    va_list ap;
    char sp[168];
    va_start(ap, fmt);
    *(short*)((int)sp + 0) = fd;
    *(short*)((int)sp + 2) = 0;
    int n = prnt(printf_char, sp, fmt, ap);
    va_end(ap);
    return n;
}

int printf(const char *fmt, ...)
{
    dbg_printf("Calling %s\n", __FUNCTION__);
    va_list ap;
    char sp[164];
    va_start(ap, fmt);
    *(short*)((int)sp + 0) = STDOUT;
    *(short*)((int)sp + 2) = 0;
    int n = prnt(printf_char, sp, fmt, ap);
    va_end(ap);
    return n;
}

int fdputc(int c, int fd)
{
    dbg_printf("Calling %s\n", __FUNCTION__);
    char str[1] = {c};
    if (c == '\t')
    {
        // print tabulations, aligning the characters (a tabulation has a size of 8 spaces)
        // 0574
        if (sceKernelDipsw(59) == 1) {
            // 05E4
            sceKernelDebugWrite(fd, "        ", 8 - (g_linePos & 7));
        }
        else
        {
            if (fd == STDOUT)
                fd = g_stdout;
            // 0590
            if (fd == STDERR)
                fd = g_stderr;
            // 059C
            sceIoWrite(fd, "        ", 8 - (g_linePos & 7));
        }
        // 05C4
        g_linePos = (g_linePos & 0xFFFFFFF8) + 8;
    }
    else if (c == '\n')
    {
        // 0504
        if (sceKernelDipsw(59) == 1)
            sceKernelDebugWrite(fd, "\r\n", 2);
        else
        {
            if (fd == STDOUT)
                fd = g_stdout;
            // 0520
            if (fd == STDERR)
                fd = g_stderr;
            // 052C
            sceIoWrite(fd, "\r\n", 2);
        }
        // 0544
        g_linePos = 0;
    }
    else
    {
        if ((look_ctype_table(c) & 0x97) != 0)
            g_linePos++;
        // 048C
        if (sceKernelDipsw(59) == 1) {
            // 04F0
            sceKernelDebugWrite(fd, str, 1);
        }
        else
        {
            if (fd == STDOUT)
                fd = g_stdout;
            // 04A8
            if (fd == STDERR)
                fd = g_stderr;
            // 04B4
            sceIoWrite(fd, str, 1);
        }
    }
    return c;
}

int fdgetc(int fd)
{
    dbg_printf("Calling %s\n", __FUNCTION__);
    char buf[1];
    int ret;
    if (g_debugRead == 0) {
        // 0660
        ret = sceKernelDebugRead(fd, buf, 1);
    }
    else {
        // 0638
        ret = sceIoRead((fd != STDIN ? fd : g_stdin), buf, 1);
    }
    // 0644
    return (ret == 1 ? buf[0] : -1);
}

char *fdgets(char *s, int fd)
{
    dbg_printf("Calling %s\n", __FUNCTION__);
    char *end = s + 125;
    char *curS = s;
    for (;;)
    {
        char c = (char)fdgetc(fd);
        switch (c)
        {
        case '\026': // sync
            // 0808
            c = (char)fdgetc(fd);
            if (curS < end)
            {
                *(curS++) = c;
                if (sceKernelDebugEcho() != 0)
                    fdputc(c, (fd != STDIN ? fd : STDOUT));
            }
            else if (sceKernelDebugEcho() != 0)
                fdputc('\a', (fd != STDIN ? fd : STDOUT));
            break;
        case '\177': // DEL
        case '\b':
            // 0790
            // 0794
            if (s >= curS)
                break;
            curS--;
            if (sceKernelDebugEcho() != 0)
            {
                int realFd = (fd != STDIN ? fd : STDOUT);
                fdputc('\b', realFd);
                fdputc(' ' , realFd);
                fdputc('\b', realFd);
            }
            break;
        case -1:
            // 0740
            if (s == curS) {
                *curS = '\0';
                return NULL;
            }
            /* FALLTHRU */
        case '\n':
            /* FALLTHRU */
        case '\r':
            // 0748
            if (sceKernelDebugEcho() != 0)
                fdputc('\n', (fd != STDIN ? fd : STDOUT));
            *curS = '\0';
            return s;
        case '\t':
            c = ' ';
            /* FALLTHRU */
        default:
            // 06DC
            if ((look_ctype_table(c) & 0x97) != 0 && curS < end)
            {
                *(curS++) = c;
                if (sceKernelDebugEcho() != 0)
                    fdputc(c, (fd != STDIN ? fd : STDOUT));
            }
            else if (sceKernelDebugEcho() != 0)
                fdputc('\a', (fd != STDIN ? fd : STDOUT));
            break;
        }
    }
}

int putchar(int c)
{
    dbg_printf("Calling %s\n", __FUNCTION__);
    return fdputc(c, STDOUT);
}

int fdputs(const char *s, int fd)
{
    dbg_printf("Calling %s\n", __FUNCTION__);
    char c;
    if (s == NULL) // 089C
        s = "<NULL>";
    // 085C, 0880
    while ((c = *(s++)) != '\0')
        fdputc(c, fd);
    return 0;
}

int puts(const char *s)
{
    dbg_printf("Calling %s\n", __FUNCTION__);
    char c;
    if (s == NULL) // 089C
        s = "<NULL>";
    // 085C, 0880
    while ((c = *(s++)) != '\0')
        fdputc(c, STDOUT);
    return 0;
}

int getchar(void)
{
    dbg_printf("Calling %s\n", __FUNCTION__);
    return fdgetc(STDIN);
}

char *gets(char *s)
{
    dbg_printf("Calling %s\n", __FUNCTION__);
    return fdgets(s, STDIN);
}

int sceKernelStdioRead()
{
    dbg_printf("Calling %s\n", __FUNCTION__);
    return SCE_ERROR_KERNEL_ERROR;
}

int sceKernelStdioLseek()
{
    dbg_printf("Calling %s\n", __FUNCTION__);
    return SCE_ERROR_KERNEL_ERROR;
}

void sceKernelStdioSendChar()
{
    dbg_printf("Calling %s\n", __FUNCTION__);
}

int sceKernelStdioWrite()
{
    dbg_printf("Calling %s\n", __FUNCTION__);
    return SCE_ERROR_KERNEL_ERROR;
}

int sceKernelStdioClose()
{
    dbg_printf("Calling %s\n", __FUNCTION__);
    return SCE_ERROR_KERNEL_ERROR;
}

int sceKernelStdioOpen()
{
    dbg_printf("Calling %s\n", __FUNCTION__);
    return SCE_ERROR_KERNEL_ERROR;
}

int _sceTtyProxyDevRead(SceIoIob *iob, char *buf, int size)
{
    dbg_printf("Calling %s\n", __FUNCTION__);
    int cnt;
    int oldK1 = pspShiftK1();
    int count = 0;
    int k1 = 0;
    if (sceIoGetIobUserLevel(iob) != 8)
        k1 = 24;
    // 09D0
    pspSetK1(k1);
    int size2 = g_pipeList[iob->fsNum + 3];
    SceUID id = g_pipeList[iob->fsNum + 0];
    int min;
    // 09F8
    do
    {
        min = size2;
        if (size2 >= size)
            min = size;
        int ret = sceKernelReceiveMsgPipe(id, buf, min, 1, &cnt, 0);
        if (ret < 0) {
            pspSetK1(oldK1);
            return ret;
        }
        size -= cnt;
        buf += cnt;
        count += cnt;
    } while (cnt >= min && size != 0);
    pspSetK1(oldK1);
    return count;
}

int _sceTtyProxyDevWrite(SceIoIob *iob, const char *buf, int size)
{
    dbg_printf("Calling %s\n", __FUNCTION__);
    int oldK1 = pspShiftK1();
    int count = 0;
    int curCnt;
    int k1 = 0;
    if (sceIoGetIobUserLevel(iob) != 8)
        k1 = 24;
    // 0AC4
    pspSetK1(k1);
    int size2 = g_pipeList[iob->fsNum + 3];
    SceUID id = g_pipeList[iob->fsNum + 0];
    // 0AE8
    do
    {
        int minSize = size2;
        if (size2 >= size)
            minSize = size;
        int ret = sceKernelSendMsgPipe(id, (void*)buf, minSize, 0, &curCnt, 0);
        if (ret < 0) {
            pspSetK1(oldK1);
            return ret;
        }
        size -= curCnt;
        count += curCnt;
        buf += curCnt;
    } while (size != 0);
    pspSetK1(oldK1);
    return count;
}

// 0B58
int sceTtyProxyInit()
{
    dbg_printf("Calling %s\n", __FUNCTION__);
    sceIoAddDrv(&g_TtyDevTbl);
    return 0;
}

int sceKernelRegisterStdoutPipe(SceUID id)
{
    dbg_printf("Calling %s\n", __FUNCTION__);
    int oldK1 = pspShiftK1();
    int ret = _sceKernelRegisterStdPipe(STDOUT, id);
    pspSetK1(oldK1);
    return ret;
}

int sceKernelRegisterStderrPipe(SceUID id)
{
    dbg_printf("Calling %s\n", __FUNCTION__);
    int oldK1 = pspShiftK1();
    int ret = _sceKernelRegisterStdPipe(STDERR, id);
    pspSetK1(oldK1);
    return ret;
}

// 0BE4
int _sceTtyProxyDevInit(SceIoDeviceArg *dev __attribute__((unused)))
{
    dbg_printf("Calling %s\n", __FUNCTION__);
    return 0;
}

// 0BEC
int _sceTtyProxyDevExit(SceIoDeviceArg *dev __attribute__((unused)))
{
    dbg_printf("Calling %s\n", __FUNCTION__);
    return 0;
}

int _sceTtyProxyDevOpen(SceIoIob *iob, char *file __attribute__((unused)), int flags __attribute__((unused)), SceMode mode __attribute__((unused)))
{
    dbg_printf("Calling %s\n", __FUNCTION__);
    if (iob->fsNum < 3)
        return 0;
    return 0x80010006;
}

int _sceTtyProxyDevClose(SceIoIob *iob)
{
    dbg_printf("Calling %s\n", __FUNCTION__);
    if (iob->fsNum < 3)
        return 0;
    return 0x80010006;
}

SceOff _sceTtyProxyDevLseek()
{
    dbg_printf("Calling %s\n", __FUNCTION__);
    return SCE_ERROR_KERNEL_BAD_FILE_DESCRIPTOR;
}

int _sceTtyProxyDevIoctl(SceIoIob *iob, unsigned int cmd, void *indata __attribute__((unused)), int inlen __attribute__((unused)), void *outdata __attribute__((unused)), int outlen __attribute__((unused)))
{
    dbg_printf("Calling %s\n", __FUNCTION__);
    if (cmd != 0x00134002)
        return SCE_ERROR_KERNEL_INVALID_ARGUMENT;
    int oldK1 = pspShiftK1();
    int k1 = 24;
    if (sceIoGetIobUserLevel(iob) == 8)
        k1 = 0;
    pspSetK1(k1);
    sceKernelCancelMsgPipe(g_pipeList[iob->fsNum], 0, 0);
    pspSetK1(oldK1);
    return 0;
}

// 0CBC
int _sceKernelRegisterStdPipe(int fd, SceUID id)
{
    dbg_printf("Calling %s\n", __FUNCTION__);
    if (id < 0)
    {  
        // 0DD8
        int *addr = &g_pipeList[fd];
        addr[0] = -1;
        addr[3] = 0;
        if (fd == STDOUT) {
            // 0E1C
            return sceKernelStdoutReset();
        }
        else if (fd == STDERR)
            return sceKernelStderrReset();
        return 0;
    }
    if (sceKernelGetThreadmanIdType(id) == 7)
        return SCE_ERROR_KERNEL_ILLEGAL_ARGUMENT;
    SceSysmemUidCB *blk;
    if (sceKernelGetUIDcontrolBlock(id, &blk) != 0)
        return SCE_ERROR_KERNEL_ILLEGAL_PERMISSION;
    if (pspK1IsUserMode() && (blk->PARENT0->attr & 2) != 0)
        return SCE_ERROR_KERNEL_ILLEGAL_PERMISSION;
    SceKernelMppInfo mpp;
    mpp.size = 56;
    int ret = sceKernelReferMsgPipeStatus(id, &mpp);
    if (ret < 0)
        return ret;
    if (STDOUT == 1) {
        // 0DB8
        ret = sceKernelStdoutReopen("ttyproxy1:", 2, 0x1FF);
    }
    else if (STDERR == 2) {
        // 0DA0
        ret = sceKernelStderrReopen("ttyproxy2:", 2, 0x1FF);
    }
    // 0D58
    if (ret < 0)
        return ret;
    int *addr = &g_pipeList[fd];
    addr[3] = mpp.bufSize;
    addr[0] = id;
    return 0;
}

