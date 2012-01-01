/* Copyright (C) 2011, 2012 The uOFW team
   See the file COPYING for copying permission.
*/

#include <stdarg.h>

#include "../global.h"

#define STDIN  0
#define STDOUT 1
#define STDERR 2

const char g_dummy[] = "dummy_drv_iofile:"; // 6700
const char g_tty[] = "tty0:"; // 6714
const char g_return[] = "\r\n"; // 671C
const char g_tab[] = "        "; // 6720
const char g_null[] = "<NULL>"; // 672C
const char g_ttyDevName[] = "ttyproxy"; // 6734
const char g_ttyDevDesc[] = "TTY2MsgPipe PROXY"; // 6740
const char g_tty1[] = "ttyproxy1:"; // 6754
const char g_tty2[] = "ttyproxy2:"; // 6760

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

SceIoDrv g_TtyDevTbl = {g_ttyDevName, 0x00000003, 0x00000001, g_ttyDevDesc, g_TtyOps}; // 69A0

int g_stdin = -1; // 6AD0
int g_stdout = -1; // 6AD4
int g_stderr = -1; // 6AD8

int g_debugRead; // 6AF0
int *g_pipeList; // 6AF4

int g_linePos; // 6C34

// 0000
int StdioReInit()
{
    SceUID *fds[3];
    fds[0] = &g_stdin;
    fds[1] = &g_stdout;
    fds[2] = &g_stderr;
    // 0044
    int i;
    for (i = 0; i < 3; i++)
        *fds[i] = sceIoOpen(g_dummy, 3, 0x1FF);
    g_debugRead = 1;
    sceTtyProxyInit();
    return 0;
}

// 0094
int StdioInit()
{
    return StdioReInit();
}

int sceKernelStdin()
{
    return g_stdin;
}

int sceKernelStdout()
{
    return g_stdout;
}

int sceKernelStderr()
{
    return g_stderr;
}

int sceKernelStdoutReopen(const char *file, int flags, SceMode mode)
{
    if (g_stdout < 0)
        return 0x80020384;
    return sceIoReopen(file, flags, mode, g_stdout);
}

int sceKernelStderrReopen(const char *file, int flags, SceMode mode)
{
    if (g_stderr < 0)
        return 0x80020384;
    return sceIoReopen(file, flags, mode, g_stderr);
}

int sceKernelStdoutReset()
{
    int ret = stdoutReset(3, 0x1FF);
    if (ret < 0)
        return ret;
    g_stdout = ret;
    return 0;
}

int sceKernelStderrReset()
{
    int ret = stdoutReset(3, 0x1FF);
    if (ret < 0)
        return ret;
    g_stderr = ret;
    return 0;
}

// 019C
int stdoutReset(int flags, SceMode mode)
{
    char openDummy = 0;
    if (sceKernelDipsw(58) == 1 && !sceKernelDipsw(59))
        openDummy = 1;
    else
        openDummy = 0;
    // 01CC
    if (openDummy)
    {  
        // 0214
        int ret = sceIoOpen(g_tty, flags, mode);
        if (ret >= 0)
            return ret;
    }
    // 01E8
    return sceIoOpen(g_dummy, flags, mode);
}

// 0244
void printf_char(void *ctx, int ch)
{   
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
    va_list ap;
    int ret;
    char sp[168];
    va_start(ap, fmt);
    *(short*)((int)sp + 0) = fd;
    *(short*)((int)sp + 2) = 0;
    int n = prnt(printf_char, sp, fmt, ap);
    va_end(ap);
}

int printf(const char *fmt, ...)
{
    va_list ap;
    int ret;
    char sp[164];
    va_start(ap, fmt);
    *(short*)((int)sp + 0) = STDOUT;
    *(short*)((int)sp + 2) = 0;
    int n = prnt(printf_char, sp, fmt, ap);
    va_end(ap);
}

int fdputc(int c, int fd)
{
    char str[1] = {c};
    if (c == '\t')
    {
        // print tabulations, aligning the characters (a tabulation has a size of 8 spaces)
        // 0574
        if (sceKernelDipsw(59) == 1) {
            // 05E4
            sceKernelDebugWrite(fd, g_tab, 8 - (g_linePos & 7)); // "        "
        }
        else
        {
            if (fd == STDOUT)
                fd = g_stdout;
            // 0590
            if (fd == STDERR)
                fd = g_stderr;
            // 059C
            sceIoWrite(fd, g_tab, 8 - (g_linePos & 7));
        }
        // 05C4
        g_linePos = (g_linePos & 0xFFFFFFF8) + 8;
    }
    else if (c == '\n')
    {
        // 0504
        if (sceKernelDipsw(59) == 1)
            sceKernelDebugWrite(fd, g_return, sizeof(g_return));
        else
        {
            if (fd == STDOUT)
                fd = g_stdout;
            // 0520
            if (fd == STDERR)
                fd = g_stderr;
            // 052C
            sceIoWrite(fd, g_return, sizeof(g_return));
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
        case '\n':
        case '\r':
            // 0748
            if (sceKernelDebugEcho() != 0)
                fdputc('\n', (fd != STDIN ? fd : STDOUT));
            *curS = '\0';
            return s;
        case '\t':
            c = ' ';
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
    return fdputc(c, STDOUT);
}

int fdputs(const char *s, int fd)
{
    char c;
    if (s == NULL) // 089C
        s = g_null;
    // 085C, 0880
    while ((c = *(s++)) != '\0')
        fdputc(c, fd);
    return 0;
}

int puts(const char *s)
{
    char c;
    if (s == NULL) // 089C
        s = g_null;
    // 085C, 0880
    while ((c = *(s++)) != '\0')
        fdputc(c, STDOUT);
    return 0;
}

int getchar(void)
{
    return fdgetc(STDIN);
}

char *gets(char *s)
{
    return fdgets(s, STDIN);
}

int sceKernelStdioRead()
{
    return 0x80020001;
}

int sceKernelStdioLseek()
{
    return 0x80020001;
}

void sceKernelStdioSendChar()
{
}

int sceKernelStdioWrite()
{
    return 0x80020001;
}

int sceKernelStdioClose()
{
    return 0x80020001;
}

int sceKernelStdioOpen()
{
    return 0x80020001;
}

int _sceTtyProxyDevRead(void *iob, void *buf, int size)
{
    int cnt;
    K1_BACKUP();
    int count = 0;
    int k1 = 0;
    if (sceIoGetIobUserLevel(iob) != 8)
        k1 = 24;
    // 09D0
    SET_REG(K1, k1);
    int unk1 = g_pipeList[*(int*)(iob + 4) + 3];
    int unk2 = g_pipeList[*(int*)(iob + 4) + 0];
    // 09F8
    do
    {
        int min = unk1;
        if (unk1 >= size)
            min = size;
        int ret = sceKernelReceiveMsgPipe(unk2, buf, min, 1, &cnt, 0);
        if (ret < 0) {
            K1_RESET();
            return ret;
        }
        size -= cnt;
        buf += cnt;
        count += cnt;
    } while (cnt >= min && size != 0);
    K1_RESET();
    return count;
}

int _sceTtyProxyDevWrite(void *iob, void *buf, int size)
{
    K1_BACKUP();
    int count = 0;
    int curCnt;
    int k1 = 0;
    if (sceIoGetIobUserLevel(iob) != 8)
        k1 = 24;
    // 0AC4
    SET_REG(K1, k1);
    int size2 = g_pipeList[*(int*)(iob + 4) + 3];
    SceUID id = g_pipeList[*(int*)(iob + 4) + 0];
    // 0AE8
    do
    {
        int minSize = size2;
        if (size2 >= size)
            minSize = size;
        int ret = sceKernelSendMsgPipe(id, buf, minSize, 0, &curCnt, 0);
        if (ret < 0) {
            K1_RESET();
            return ret;
        }
        size -= curCnt;
        count += curCnt;
        buf += curCnt;
    } while (size != 0);
    K1_RESET();
    return count;
}

// 0B58
int sceTtyProxyInit()
{
    sceIoAddDrv(g_TtyOps);
    return 0;
}

int sceKernelRegisterStdoutPipe(SceUID id)
{
    K1_BACKUP();
    int ret = _sceKernelRegisterStdPipe(STDOUT, id);
    K1_RESET();
    return ret;
}

int sceKernelRegisterStderrPipe(SceUID id)
{
    K1_BACKUP();
    int ret = _sceKernelRegisterStdPipe(STDERR, id);
    K1_RESET();
    return ret;
}

// 0BE4
int _sceTtyProxyDevInit()
{
    return 0;
}

// 0BEC
int _sceTtyProxyDevExit()
{
    return 0;
}

int _sceTtyProxyDevOpen(void *iob)
{
    if (*(int*)(iob + 4) < 3)
        return 0;
    return 0x80010006;
}

int _sceTtyProxyDevClose(void *iob)
{
    if (*(int*)(iob + 4) < 3)
        return 0;
    return 0x80010006;
}

long long _sceTtyProxyDevLseek()
{
    return 0x80020323;
}

int _sceTtyProxyDevIoctl(void *iob, int cmd)
{
    if (cmd != 0x00134002)
        return 0x80020324;
    K1_BACKUP();
    int k1 = 24;
    if (sceIoGetIobUserLevel(iob) == 8)
        k1 = 0;
    SET_REG(K1, k1);
    sceKernelCancelMsgPipe(g_pipeList[*(int*)(iob + 4)], 0, 0);
    K1_RESET();
    return 0;
}

// 0CBC
int _sceKernelRegisterStdPipe(int fd, SceUID id)
{
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
        return 0x800200D2;
    SceSysmemUIDControlBlock blk;
    if (sceKernelGetUIDcontrolBlock(id, &blk) != 0)
        return 0x800200D1;
    K1_GET();
    if (K1_USER_MODE() && (blk.parent->attribute & 2) != 0)
        return 0x800200D1;
    SceKernelMppInfo mpp;
    mpp.size = 56;
    int ret = sceKernelReferMsgPipeStatus(id, &mpp);
    if (ret < 0)
        return ret;
    if (STDOUT == 1) {
        // 0DB8
        ret = sceKernelStdoutReopen(g_tty1, 2, 0x1FF);
    }
    else if (STDERR == 2) {
        // 0DA0
        ret = sceKernelStderrReopen(g_tty2, 2, 0x1FF);
    }
    // 0D58
    if (ret < 0)
        return ret;
    int *addr = &g_pipeList[fd];
    addr[3] = mpp.bufSize;
    addr[0] = id;
    return 0;
}

