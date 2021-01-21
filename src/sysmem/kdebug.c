#include <stdarg.h>
#include <sysmem_sysclib.h>

#include <common_imp.h>

#include "intr.h"
#include "start.h"

int kprnt(short *arg0, const char *fmt, va_list ap, int userMode);

// 14434
char kprnt_outbuf[260];
// 13BC4
int (*kprinthandler)(short*, const char*, va_list, int) = kprnt;
// 13BC8
void *kprintwork = (void*)kprnt_outbuf;
// 13BCC
int kprintf_putchar_initial = 1;
// 13BD0
int debug_echo_mode = 1;

// 14410
u32 gDipLo;
// 14414
u32 gDipHi;
// 14418
u32 g_uiCpTime;
// 1441C
int gUmdDvdInitialized;
// 14420
int gIsDvdMode;
// 14424
void *g_pSm1Ops;
// 14428
void (*kprintf_putchar_handler)(short*, int);
// 1442C
int (*debug_write_handler)();
// 14430
int (*debug_read_handler)();

int DipswSet(int reg, int val);
void SetIsDvdMode(void);

int DipswInit(u32 lo, u32 hi, u32 cpTime)
{
    gDipLo = lo;
    gDipHi = hi;
    g_uiCpTime = cpTime;
    return 0;
}

int sceKernelDipsw(u32 reg)
{
    if (reg >= 64)
        return SCE_ERROR_KERNEL_ERROR;
    if (reg >= 32) {
        // FC00
        return (gDipHi >> (reg - 32)) & 1;
    }
    return (gDipLo >> reg) & 1;
}

u32 sceKernelDipswAll()
{
    return gDipLo;
}

u32 sceKernelDipswLow32()
{
    return gDipLo;
}

u32 sceKernelDipswHigh32()
{
    return gDipHi;
}

int sceKernelDipswSet(u32 reg)
{
    return DipswSet(reg, 1);
}

int sceKernelDipswClear(u32 reg)
{
    return DipswSet(reg, 0);
}

int sceKernelDipswCpTime(void)
{
    return g_uiCpTime;
}

int sceKernelIsToolMode(void)
{
    if (((gDipLo >> 30) & 1) == 1 || ((gDipLo >> 28) & 1) == 1)
        return 1;
    return 0;
}

int sceKernelIsDevelopmentToolMode()
{
    return (gDipLo >> 30) & 1;
}

int sceKernelIsUMDMode()
{
    SetIsDvdMode();
    return (gIsDvdMode == 0);
}

int sceKernelIsDVDMode()
{
    SetIsDvdMode();
    return gIsDvdMode;
}

int sceKernelSm1RegisterOperations(void *arg)
{
    g_pSm1Ops = arg;
    if (arg != NULL)
    {
        g_GetGPI = *(int*)(arg + 28);
        g_SetGPO = *(int*)(arg + 32);
        g_GetPTRIG = *(int*)(arg + 52);
        g_SetPTRIG = *(int*)(arg + 56);
    }
    return 0;
}

void *sceKernelSm1ReferOperations()
{
    return g_pSm1Ops;
}

int DipswSet(int reg, int val)
{
    int oldK1 = pspShiftK1();
    if (reg < 0 || reg >= 64)
    {
        // FDA0 dup
        pspSetK1(oldK1);
        return SCE_ERROR_KERNEL_ERROR;
    }

    if (pspK1IsUserMode())
    {
        switch (reg) // jump table at 0x13858
        {
        case 10:
        case 11:
        case 12:
        case 17:
        case 18:
        case 19:
        case 20:
        case 24:
        case 49:
        case 51:
            break;

        default:
            // FDA0 dup
            pspSetK1(oldK1);
            return SCE_ERROR_KERNEL_ERROR;
        }
    }
    // (FDB0)
    // FDB4
    u32 *regPtr;
    if (reg >= 32) {
        regPtr = &gDipHi;
        reg -= 32;
    }
    else
        regPtr = &gDipLo;

    // FDCC
    int oldVar = (*regPtr >> reg) & 1;
    if (val == 0)
        *regPtr &= ~(1 << reg);
    else
        *regPtr |= (1 << reg);
    // FDF8
    pspSetK1(oldK1);
    return oldVar;
}

void SetIsDvdMode(void)
{
    if (gUmdDvdInitialized != 0)
        return;
    gUmdDvdInitialized = 1;
    if (sceKernelIsToolMode() && ((gDipLo >> 16) & 1) == 1) {
        // FE5C
        gIsDvdMode = 1;
    }
    else {
        // FE48
        gIsDvdMode = 0;
    }
}

int kprnt(short *arg0, const char *fmt, va_list ap, int userMode)
{
    char str[20];
    void (*func)(short*, int) = kprintf_putchar_handler;
    *(short*)(arg0 + 0) = 1;
    int base = 10;
    *(short*)(arg0 + 2) = 0;
    int curArg = 0;
    s64 longVar;
    if (kprintf_putchar_handler == NULL || fmt == NULL)
        return 0;
    func(arg0, 512);
    char *hexNumChars = "0123456789abcdef";
    int numChars = 0;
    int stringLen;
    // FF00
    for (;;)
    {
        if (*fmt == '\0')
        {
            // 0FF94 dup
            func(arg0, 513);
            return numChars;
        }
        if (*fmt == '%')
        {
            int oldStrSize = 0;
            // FF38
            int attr = 0;
            int strSize = -1;
            int argWidth = 0;
            char sign = '\0';
            char *curStr;
            // FF48
            do
            {
                fmt++;
                // FF4C
                switch (*fmt) // jump table at 0x13900
                {
                case ' ':
                    // 0FF7C
                    if (sign == '\0')
                        sign = ' ';
                    continue;

                case '\0':
                    // 0FF94 dup
                    func(arg0, 513);
                    return numChars;

                case '#':
                    // 0FFD8
                    attr |= 8;
                    continue;

                case '*':
                    // 0FFE0
                    argWidth = va_arg(ap, int);
                    if (argWidth < 0) {
                        argWidth = -argWidth;
                        attr |= 0x10;
                    }
                    continue;

                case '-':
                    // 0FFF8
                    attr |= 0x10;
                    continue;

                case '+':
                    // 10000
                    sign = '+';
                    continue;

                case '.':
                    // 1000C
                    fmt++;
                    int val;
                    if (*fmt == '*')
                    {
                        // 10074
                        val = va_arg(ap, int);
                    }
                    else
                    {
                        val = 0;
                        // 10034
                        while (*fmt >= '0' && *fmt <= '9')
                            val = val * 10 + *(fmt++) - '0';
                        // 10064
                        fmt--;
                    }
                    // 10068
                    strSize = pspMax(val, -1);
                    continue;

                case '0':
                    // 10080
                    attr |= 0x20;
                    continue;

                case '1': case '2': case '3': case '4': case '5': case '6': case '7': case '8': case '9': {
                    // 10088
                    int val = 0;
                    // 10090
                    do
                    {
                        val = val * 10 + (*(fmt++) - '0');
                        if (*fmt < 0)
                            break;
                    } while (*fmt >= '0' && *fmt <= '9');
                    // 100CC
                    fmt--;
                    argWidth = val;
                }
                    continue;

                case 'D':
                    // 100D8
                    attr |= 1;
                    /* FALLTHRU */

                case 'd':
                case 'i':
                    // 100DC
                    if ((attr & 2) == 0)
                    {
                        // 103F4
                        curArg = va_arg(ap, int);
                        if (curArg < 0)
                        {
                            // 104A0
                            curArg = -curArg;
                            sign = '-';
                        }
                        // 10404
                        base = 10;
                        goto print_num2;
                    }
                    longVar = va_arg(ap, s64);
                    if (longVar < 0)
                    {
                        // 103C8
                        longVar = -longVar;
                        sign = '-';
                    }
                    // 10108
                    goto print_num1;

                case 'L':
                    // 104B4
                    attr |= 2;
                    continue;

                case 'O':
                    attr |= 1;
                    /* FALLTHRU */

                case 'o':
                    // 104C0
                    if ((attr & 2) == 0)
                    {
                        // 10500
                        curArg = va_arg(ap, int);
                        // 1050C dup
                        base = 8;
                    }
                    else
                    {
                        longVar = va_arg(ap, s64);
                        // 104EC dup
                        base = 8;
                    }
                    goto print_base;

                case 'U':
                    // 10514
                    attr |= 1;
                    /* FALLTHRU */

                case 'u':
                    // 10518
                    if ((attr & 2) == 0)
                    {
                        // 10548
                        curArg = va_arg(ap, int);
                        // 1050C dup
                        base = 10;
                    }
                    else
                    {
                        longVar = va_arg(ap, s64);
                        // 104EC dup
                        base = 10;
                    }
                    goto print_base;

                case 'X':
                    // 10558
                    hexNumChars = "0123456789ABCDEF";
                    /* FALLTHRU */

                case 'x':
                    // 10564
                    base = 16;
                    if ((attr & 2) == 0)
                    {
                        // 105B0
                        curArg = va_arg(ap, int);
                        if (((attr >> 3) & 1) != 0 && curArg != 0)
                        {
                            attr |= 0x40;
                            // 105A8 dup
                        }
                    }
                    else
                    {
                        longVar = va_arg(ap, s64);
                        if (longVar != 0 && ((attr >> 3) & 1) != 0)
                        {
                            attr |= 0x40;
                            // 105A8 dup
                        }
                    }
                    goto print_base;

                case 'c':
                    // 105D8
                    curStr = str;
                    stringLen = 1;
                    sign = '\0';
                    *str = (u8)va_arg(ap, int);
                    goto print_str;

                case 'h':
                    // 105F8
                    attr |= 0x4;
                    continue;

                case 'l':
                    // 10600
                    if ((attr & 1) == 0)
                        attr |= 1;
                    else
                        attr = (attr & 0xFFFFFFFE) | 2;
                    continue;

                case 'n':
                    // 10614
                    if ((attr & 1) == 0)
                    {
                        // 1064C
                        if ((attr & 4) == 0)
                        {
                            // 10680
                            int *ptr = va_arg(ap, int*);
                            if (userMode != 0 && !pspK1PtrOk(ptr))
                                return SCE_ERROR_KERNEL_ILLEGAL_ADDR;
                            // 1069C
                            *ptr = numChars;
                        }
                        else
                        {
                            short *ptr = va_arg(ap, short*);
                            if (userMode != 0 && !pspK1PtrOk(ptr))
                                return SCE_ERROR_KERNEL_ILLEGAL_ADDR;
                            // 10674
                            *ptr = numChars;
                        }
                    }
                    else
                    {
                        int *ptr = va_arg(ap, int*);
                        if (userMode != 0 && !pspK1PtrOk(ptr))
                            return SCE_ERROR_KERNEL_ILLEGAL_ADDR;
                        // 10640
                        *ptr = numChars;
                    }
                    break;

                case 'p':
                    // 106A8
                    curArg = va_arg(ap, int);
                    base = 16;
                    // 105A8 dup
                    goto print_base;

                case 's': {
                    // 106C0
                    curStr = va_arg(ap, char*);
                    if (curStr == NULL)
                    {
                        // 10748
                        curStr = "(null)";
                    }
                    // 106CC
                    if (userMode != 0 && !pspK1PtrOk(curStr))
                        return SCE_ERROR_KERNEL_ILLEGAL_ADDR;
                    // 106E4
                    if (strSize < 0)
                    {
                        // 1072C
                        stringLen = strlen(curStr);
                    }
                    else
                    {
                        char *s2 = memchr(curStr, '\0', strSize);
                        stringLen = strSize;
                        if (s2 != NULL)
                        {
                            stringLen = s2 - curStr;
                            if (strSize < s2 - curStr)
                                stringLen = strSize;
                        }
                    }
                    // 10724
                    sign = '\0';
                }
                    goto print_str;

                default:
                    // 10754
                    func(arg0, *fmt);
                    numChars++;
                    // FF2C dup
                    break;
                }
                break;

                print_base:
                // 104F0
                sign = '\0';
                if ((attr & 2) == 0)
                    goto print_num2;

                print_num1:
                // 1010C
                oldStrSize = strSize;
                if (strSize >= 0)
                    attr &= 0xFFFFFFDF;
                curStr = str + 21;
                if (longVar != 0 || strSize != 0)
                {
                    // 10144
                    char lastChar;
                    do
                    {
                        u32 mod = __umoddi3(longVar, base);
                        lastChar = hexNumChars[mod];
                        *(--curStr) = lastChar;
                        longVar = __udivdi3(longVar, base);
                    } while (longVar != 0);
                    hexNumChars = "0123456789abcdef";
                    if (((attr >> 3) & 1) != 0 && base == 8 && lastChar != '0') // 101D4 dup
                        *(--curStr) = '0';
                }
                goto print_num;

                print_num2:
                // 1040C
                oldStrSize = strSize;
                if (strSize >= 0)
                    attr &= 0xFFFFFFDF;
                curStr = str + 21;
                if (curArg != 0 || strSize != 0)
                {
                    char lastC;
                    // 1043C
                    do
                    {
                        curStr--;
                        // 1044C
                        curArg /= base;
                        *curStr = hexNumChars[curArg % base];
                        lastC = *curStr;
                    } while (strSize != 0);
                    hexNumChars = "0123456789abcdef";
                    if (((attr >> 3) & 1) != 0 && base == 8 && lastC != '0') // 101D4 dup
                        *(--curStr) = '0';
                }

                print_num:
                // 101E8
                // 101EC
                stringLen = &str[21] - curStr;

                print_str:
                // 101F4
                strSize = stringLen;
                if (sign != 0)
                    strSize = stringLen + 1;
                if ((attr & 0x40) != 0)
                    strSize += 2;
                int maxSize = pspMax(strSize, oldStrSize);
                int i;
                if ((attr & 0x30) == 0 && argWidth != 0)
                    for (i = 0; i < argWidth - maxSize; i++)
                        func(arg0, ' ');
                // (10268)
                // 1026C
                if (sign != '\0')
                {
                    // 103AC
                    func(arg0, sign);
                }
                // 10274
                if ((attr & 0x40) != 0)
                {
                    // 10384
                    func(arg0, '0');
                    func(arg0, *fmt);
                }
                // 1027C
                if ((attr & 0x30) == 0x20)
                {
                    // 1034C
                    for (i = 0; i < argWidth - maxSize; i++) // 10358
                        func(arg0, '0');
                }
                // (10288)
                // 1028C
                for (i = 0; i < oldStrSize - strSize; i++) // 1029C
                    func(arg0, '0');
                // 102C0
                for (i = stringLen - 1; i >= 0; i--) // 102D0
                    func(arg0, *(curStr++));
                // 102FC
                if ((attr & 0x10) != 0)
                    for (i = 0; i < argWidth - maxSize; i++) // 10318
                        func(arg0, ' ');
                // (10338)
                // 1033C
                numChars += pspMax(maxSize, argWidth);
            } while (0);
        }
        else
        {
            func(arg0, *fmt);
            numChars++;
            // FF2C dup
        }
        // FF30
        fmt++;
    }
}

int sceKernelPrintf(const char *fmt, ...) __attribute__((alias("KprintfForUser")));

int KprintfForUser(const char *fmt, ...)
{
    va_list ap;
    int oldK1 = pspShiftK1();
    int ret = 0;
    va_start(ap, fmt);
    if (!pspK1PtrOk(fmt)) {
        pspSetK1(oldK1);
        return SCE_ERROR_KERNEL_ILLEGAL_ADDR;
    }
    int oldIntr = suspendIntr();
    if (kprinthandler != NULL && sceKernelDipsw(56) != 0) // 1082C
        ret = kprinthandler(kprintwork, fmt, ap, 1);
    // 107F4
    resumeIntr(oldIntr);
    va_end(ap);
    pspSetK1(oldK1);
    return ret;
}

int Kprintf(const char *fmt, ...)
{
    va_list ap;
    int oldK1 = pspShiftK1();
    va_start(ap, fmt);
    int ret = 0;
    int oldIntr = suspendIntr();
    if (kprinthandler != NULL && sceKernelDipsw(56) != 0) // 10900
        ret = kprinthandler(kprintwork, fmt, ap, 0);
    // 108CC
    resumeIntr(oldIntr);
    va_end(ap);
    pspSetK1(oldK1);
    return ret;
}

void (*sceKernelGetDebugPutchar(void))(short*, int)
{
    if (kprintf_putchar_initial != 0)
        return NULL;
    return kprintf_putchar_handler;
}

void sceKernelRegisterDebugPutchar(void (*func)(short*, int))
{
    kprintf_putchar_handler = func;
    kprintf_putchar_initial = 0;
}

void sceKernelRegisterDebugPutcharByBootloader(void (*func)(short*, int))
{
    if (kprintf_putchar_initial != 0)
        kprintf_putchar_handler = func;
}

void sceKernelRegisterKprintfHandler(int (*func)(short*, const char*, va_list, int), void *param)
{
    kprinthandler = func;
    kprintwork = param;
}

int _CheckDebugHandler(void *ptr, u32 size)
{
    int ret = 0;
    if ((void*)kprintf_putchar_handler < ptr || (void*)kprintf_putchar_handler >= ptr + size)
    {
        // 109C4
        kprintf_putchar_handler = NULL;
        ret = SCE_ERROR_KERNEL_ERROR;
    }
    // 109D0
    if ((void*)kprinthandler < ptr || (void*)kprinthandler >= ptr + size)
    {
        kprinthandler = NULL;
        // 109E4
        return SCE_ERROR_KERNEL_ERROR;
    }
    return ret;
}

int sceKernelDebugWrite()
{
    int (*func)() = debug_write_handler;
    if (func == NULL)
        return SCE_ERROR_KERNEL_ERROR;
    return func();
}

int sceKernelRegisterDebugWrite(int (*func)())
{
    debug_write_handler = func;
    return 0;
}

int sceKernelDebugRead()
{
    int (*func)() = debug_read_handler;
    if (func == NULL)
        return SCE_ERROR_KERNEL_ERROR;
    return func();
}

int sceKernelRegisterDebugRead(int (*func)())
{
    debug_read_handler = func;
    return 0;
}

int sceKernelDebugEcho(void)
{
    return debug_echo_mode;
}

int sceKernelDebugEchoSet(int echo)
{
    debug_echo_mode = echo;
    return echo;
}

