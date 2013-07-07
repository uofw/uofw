#include <stdarg.h>

#include <common_imp.h>
#include <sysmem_sysclib.h>

#define CTYPE_DOWNCASE_LETTER 0x01
#define CTYPE_UPCASE_LETTER   0x02
#define CTYPE_CIPHER          0x04
#define CTYPE_TRANSPARENT     0x08
#define CTYPE_PUNCTUATION     0x10
#define CTYPE_CTRL            0x20
#define CTYPE_HEX_CIPHER      0x40

#define CTYPE_LETTER (CTYPE_DOWNCASE_LETTER | CTYPE_UPCASE_LETTER)

char g_ctypeTbl[] =
{
    0,
    CTYPE_CTRL, // 0
    CTYPE_CTRL,
    CTYPE_CTRL,
    CTYPE_CTRL,
    CTYPE_CTRL, // 4
    CTYPE_CTRL,
    CTYPE_CTRL,
    CTYPE_CTRL,
    CTYPE_CTRL, // 8
    CTYPE_TRANSPARENT,
    CTYPE_TRANSPARENT,
    CTYPE_TRANSPARENT,
    CTYPE_TRANSPARENT, // 12
    CTYPE_TRANSPARENT,
    CTYPE_CTRL,
    CTYPE_CTRL,
    CTYPE_CTRL, // 16
    CTYPE_CTRL,
    CTYPE_CTRL,
    CTYPE_CTRL,
    CTYPE_CTRL, // 20
    CTYPE_CTRL,
    CTYPE_CTRL,
    CTYPE_CTRL,
    CTYPE_CTRL, // 24
    CTYPE_CTRL,
    CTYPE_CTRL,
    CTYPE_CTRL,
    CTYPE_CTRL, // 28
    CTYPE_CTRL,
    CTYPE_CTRL,
    CTYPE_CTRL,
    CTYPE_TRANSPARENT | CTYPE_PUNCTUATION, // 32
    CTYPE_PUNCTUATION,
    CTYPE_PUNCTUATION,
    CTYPE_PUNCTUATION,
    CTYPE_PUNCTUATION, // 36
    CTYPE_PUNCTUATION,
    CTYPE_PUNCTUATION,
    CTYPE_PUNCTUATION,
    CTYPE_PUNCTUATION, // 40
    CTYPE_PUNCTUATION,
    CTYPE_PUNCTUATION,
    CTYPE_PUNCTUATION,
    CTYPE_PUNCTUATION, // 44
    CTYPE_PUNCTUATION,
    CTYPE_PUNCTUATION,
    CTYPE_PUNCTUATION,
    CTYPE_CIPHER, // 48
    CTYPE_CIPHER,
    CTYPE_CIPHER,
    CTYPE_CIPHER,
    CTYPE_CIPHER, // 52
    CTYPE_CIPHER,
    CTYPE_CIPHER,
    CTYPE_CIPHER,
    CTYPE_CIPHER, // 56
    CTYPE_CIPHER,
    CTYPE_PUNCTUATION,
    CTYPE_PUNCTUATION,
    CTYPE_PUNCTUATION, // 60
    CTYPE_PUNCTUATION,
    CTYPE_PUNCTUATION,
    CTYPE_PUNCTUATION,
    CTYPE_PUNCTUATION, // 64
    CTYPE_DOWNCASE_LETTER | CTYPE_HEX_CIPHER,
    CTYPE_DOWNCASE_LETTER | CTYPE_HEX_CIPHER,
    CTYPE_DOWNCASE_LETTER | CTYPE_HEX_CIPHER,
    CTYPE_DOWNCASE_LETTER | CTYPE_HEX_CIPHER, // 68
    CTYPE_DOWNCASE_LETTER | CTYPE_HEX_CIPHER,
    CTYPE_DOWNCASE_LETTER | CTYPE_HEX_CIPHER,
    CTYPE_DOWNCASE_LETTER,
    CTYPE_DOWNCASE_LETTER, // 72
    CTYPE_DOWNCASE_LETTER,
    CTYPE_DOWNCASE_LETTER,
    CTYPE_DOWNCASE_LETTER,
    CTYPE_DOWNCASE_LETTER, // 76
    CTYPE_DOWNCASE_LETTER,
    CTYPE_DOWNCASE_LETTER,
    CTYPE_DOWNCASE_LETTER,
    CTYPE_DOWNCASE_LETTER, // 80
    CTYPE_DOWNCASE_LETTER,
    CTYPE_DOWNCASE_LETTER,
    CTYPE_DOWNCASE_LETTER,
    CTYPE_DOWNCASE_LETTER, // 84
    CTYPE_DOWNCASE_LETTER,
    CTYPE_DOWNCASE_LETTER,
    CTYPE_DOWNCASE_LETTER,
    CTYPE_DOWNCASE_LETTER, // 88
    CTYPE_DOWNCASE_LETTER,
    CTYPE_DOWNCASE_LETTER,
    CTYPE_PUNCTUATION,
    CTYPE_PUNCTUATION, // 92
    CTYPE_PUNCTUATION,
    CTYPE_PUNCTUATION,
    CTYPE_PUNCTUATION,
    CTYPE_PUNCTUATION, // 96
    CTYPE_UPCASE_LETTER | CTYPE_HEX_CIPHER,
    CTYPE_UPCASE_LETTER | CTYPE_HEX_CIPHER,
    CTYPE_UPCASE_LETTER | CTYPE_HEX_CIPHER,
    CTYPE_UPCASE_LETTER | CTYPE_HEX_CIPHER, // 100
    CTYPE_UPCASE_LETTER | CTYPE_HEX_CIPHER,
    CTYPE_UPCASE_LETTER | CTYPE_HEX_CIPHER,
    CTYPE_UPCASE_LETTER,
    CTYPE_UPCASE_LETTER, // 104
    CTYPE_UPCASE_LETTER,
    CTYPE_UPCASE_LETTER,
    CTYPE_UPCASE_LETTER,
    CTYPE_UPCASE_LETTER, // 108
    CTYPE_UPCASE_LETTER,
    CTYPE_UPCASE_LETTER,
    CTYPE_UPCASE_LETTER,
    CTYPE_UPCASE_LETTER, // 112
    CTYPE_UPCASE_LETTER,
    CTYPE_UPCASE_LETTER,
    CTYPE_UPCASE_LETTER,
    CTYPE_UPCASE_LETTER, // 116
    CTYPE_UPCASE_LETTER,
    CTYPE_UPCASE_LETTER,
    CTYPE_UPCASE_LETTER,
    CTYPE_UPCASE_LETTER, // 120
    CTYPE_UPCASE_LETTER,
    CTYPE_UPCASE_LETTER,
    CTYPE_PUNCTUATION,
    CTYPE_PUNCTUATION, // 124
    CTYPE_PUNCTUATION,
    CTYPE_PUNCTUATION,
    CTYPE_CTRL
};

int bcmp(void *s1, void *s2, int n)
{
    return memcmp(s1, s2, n);
}

void bcopy(void *src, void *dst, int n)
{
    memmove(dst, src, n);
}

void bzero(void *s, int n)
{
    memset(s, 0, n);
}

int toupper(int c)
{
    if ((g_ctypeTbl[c + 1] & CTYPE_UPCASE_LETTER) == 0)
        return c;
    return (char)c - 32;
}

int tolower(int c)
{
    if ((g_ctypeTbl[c + 1] & CTYPE_DOWNCASE_LETTER) == 0)
        return c;
    return (char)c + 32;
}

int look_ctype_table(int c)
{
    return g_ctypeTbl[c + 1];
}

char *index(char *s, int c)
{
    char match = (char)c;
    if (match == '\0')
        return NULL;
    char cur;
    do
    {
        cur = *s;
        if (cur == match)
            return s;
        s++;
    } while (cur != '\0');
    return NULL;
}

u64 __udivmoddi4(u64 arg01, u64 arg23, u64 *v)
{
    u64 or = arg01 | arg23;
    if (arg23 == 0)
        return 0;
    int shift = 0;
    // D288
    while ((or & 1) == 0) {
        or >>= 1;
        shift++;
    }
    // D2A4
    int shift2 = 0;
    if ((or & 0xFFFFFFFF00000000) != 0)
    {
        // D380
        for (;;)
        {
            // D3A8
            if (arg23 >= arg01 || (s64)arg23 < 0)
                break;
            arg23 <<= 1;
            shift2++;
        }
        // D3C4
        u64 t01 = (u64)1 << shift2;
        // D3FC
        u64 b12 = 0;
        // D414
        while (t01 != 0)
        {
            if (arg01 >= arg23)
            {
                // D42C
                arg01 -= arg23;
                b12 |= t01;
            }
            // D444
            t01 >>= 1;
            arg23 >>= 1;
        }
        // D468
        *v = arg01;
        return b12;
    }
    int div1, div2;
    div2 = arg23 >> shift;
    div1 = arg01 >> shift;
    // D318
    // D330
    u64 shifted = div1 % div2;
    // D368
    *v = shifted << shift;
    return div1 / div2;
}

u64 __udivdi3(u64 arg01, u64 arg23)
{
    int shift;
    u64 num, result;
    if (arg23 == 0)
        return 0;
    shift = 0;
    // D4B0
    for (shift = 0; (arg23 & 1) == 0; shift++)
        arg23 >>= 1;
    // D4CC
    num = arg01 >> shift;
    // D4FC
    arg01 = num;
    if (((num | arg23) >> 32) == 0) {
        // D524
        return (u32)arg01 / (u32)arg23;
    }
    // D53C
    num = arg01;
    if ((s64)arg01 < 0) {
        // D660
        num = -1;
    }
    // D554
    // (D55C)
    // D560
    for (shift = 0; arg23 < num; shift++)
        arg23 <<= 1;
    // D58C
    num = (u64)1 << shift;
    // D5C4
    result = 0;
    // D5D8
    while (num != 0)
    {
        if (arg01 >= arg23)
        {
            // D5F0
            arg01 -= arg23;
            result |= num;
        }
        // D608
        num >>= 1;
        arg23 >>= 1;
    }
    // D630
    return result;
}

u64 __umoddi3(u64 arg01, u64 arg23)
{
    u64 mod;
    __udivmoddi4(arg01, arg23, &mod);
    return mod;
}

void *memchr(const void *s, int c, int n)
{
    if (s != NULL)
    {
        // D6A8
        while ((n--) > 0)
        {
            if (*(char*)s == (c & 0xFF))
                return (void *)s;
            s++;
        }
    }
    return NULL;
}

int memcmp(const void *s1, const void *s2, int n)
{
    // D6D8
    while ((n--) > 0)
    {
        u8 a = *((u8*)s1++);
        u8 b = *((u8*)s2++);
        if (a != b)
        {
            // D700
            if (a >= b)
                return 1;
            return -1;
        }
    }
    return 0;
}

void *sceKernelMemcpy(void *dst, const void *src, u32 n)
{
    const char *curSrc = (const char*)src;
    char *curDst = (char*)dst;
    char *end = (char*)dst + n;

    asm(".set noat");

    if (dst == src)
        return dst;
    if (n >= 8)
    {
        // D754
        if (((int)dst & 3) != 0)
        {
            // D96C
            int align = 4 - ((int)dst & 3);
            asm("lwl $at, 3(%0)\n \
                 lwr $at, 0(%0)\n \
                 swr $at, 0(%1)" : : "r" (src), "r" (dst) : "at");
            curSrc = src + align;
            curDst = dst + align;
        }
        // D75C
        if ((((int)curDst >> 29) & 3) == 0 && n >= 128)
        {
            if (((int)curSrc & 3) != 0)
            {
                // D86C
                // D874
                while ((u32)curDst != UPALIGN64((int)curDst))
                {
                    asm("lwl $at, 3(%0)\n \
                         lwr $at, 0(%0)\n \
                         addiu %0, %0, 4\n \
                         addiu %1, %1, 4\n \
                         sw $at, -4(%1)" : : "r" (curSrc), "r" (curDst) : "at");
                }
                // D890
                // D894
                do
                {
                    pspCache(0x18, curDst);
                    int i;
                    for (i = 0; i < 4; i++)
                    {
                        asm("lwl $at, 3(%0)\n \
                             lwr $at, 0(%0)\n \
                             lwr $a3, 7(%0)\n \
                             lwr $a3, 4(%0)\n \
                             lwr $t0, 11(%0)\n \
                             lwr $t0, 8(%0)\n \
                             lwr $t1, 15(%0)\n \
                             lwr $t1, 12(%0)\n \
                             sw  $at, 0(%1)\n \
                             sw  $a3, 4(%1)\n \
                             sw  $t0, 8(%1)\n \
                             sw  $t1, 12(%1)" : : "r" (curSrc + i * 16), "r" (curDst + i * 16) : "at", "a3", "t0", "t1");
                        curSrc += 64;
                        curDst += 64;
                    }
                } while (curDst != (void*)((int)end & 0xFFFFFFC0));
            }
            else
            {
                // D784
                while ((u32)curDst != UPALIGN64((int)curDst))
                {
                    asm("lw $at, 0(%0)\n \
                         addiu %0, %0, 4\n \
                         sw $at, 0(%1)\n \
                         addiu %1, %1, 4" : : "r" (curSrc), "r" (curDst) : "at");
                }
                // D79C
                // D7A0
                do
                {
                    pspCache(0x18, curDst);
                    int i;
                    for (i = 0; i < 4; i++)
                    {
                        asm("lw $at, 0(%0)\n \
                             lw $a3, 4(%0)\n \
                             lw $t0, 8(%0)\n \
                             lw $t1, 12(%0)\n \
                             sw $at, 0(%1)\n \
                             sw $a3, 4(%1)\n \
                             sw $t0, 8(%1)\n \
                             sw $t1, 12(%1)" : : "r" (curSrc + i * 16), "r" (curDst + i * 16) : "at", "a3", "t0", "t1");
                        curSrc += 64;
                        curDst += 64;
                    }
                } while (curDst != (void*)((int)end & 0xFFFFFFC0));
            }
            // D830
            if ((void*)((int)end & 0xFFFFFFC0) == end)
                return dst;
            curDst = (void*)((int)end & 0xFFFFFFC0);
        }
        // D83C
        // D84C
        while (curDst != (void*)((int)end & 0xFFFFFFFC))
        {
            asm("lwl %0, 3(%1)\n \
                 lwr %0, 0(%1)" : "=r" (curDst) : "r" (curSrc));
            curDst += 4;
            curSrc += 4;
        }
        // D864
    }
    // D730
    // D738
    while (curDst != end)
        *(curDst++) = *(curSrc++);
    return dst;
}

void *memcpy(void *dst, const void *src, u32 n)
{
    if (dst == NULL)
        return NULL;
    return sceKernelMemcpy(dst, src, n);
}

void *memmove(void *dst, const void *src, int n)
{
    char *dst2 = dst;
    const char *src2 = src;
    if (dst == NULL)
        return NULL;
    if (dst < src)
    {
        // D9E4
        // D9EC
        while ((n--) > 0)
            *(dst2++) = *(src2++);
    }
    else
    {
        // D9C0
        while ((--n) >= 0)
            dst2[n] = src2[n];
    }
    return dst;
}

int prnt(prnt_callback cb, void *ctx, const char *fmt, va_list args)
{
    if (fmt == NULL)
        return 0;
    cb(ctx, 512);
    char *ciphers = "0123456789abcdef";
    int count = 0;
    char string[21];

    // DA74
    for (;;)
    {
        if (*fmt == '\0')
        {
            // DB08 dup
            cb(ctx, 513);
            return count;
        }
        if (*fmt == '%')
        {
            int flag = 0;
            // DAAC
            int precision = -1;
            int usedPrecision = 0;
            int numAlign = 0; // with %* or %0123456789
            int sign = '\0';
            int base;
            int number;
            int stringLen;
            char *s;
            int len;
            // DABC
            do
            {
                fmt++;
                // DAC0
                switch (*fmt) // jump table at 0x13674
                {
                case ' ':
                    // DAF0
                    if (sign == '\0')
                        sign = ' ';
                    continue;

                case '\0':
                    // DB08 dup
                    cb(ctx, 513);
                    return count;

                case '#':
                    // DB48
                    flag |= 8;
                    continue;

                case '*':
                    // DB58
                    numAlign = va_arg(args, int);
                    if (numAlign >= 0)
                        continue;
                    numAlign = -numAlign;
                case '-':
                    // DB70
                    flag |= 0x10;
                    continue;

                case '+':
                    // DB80
                    sign = '+';
                    continue;

                case '.':
                    // DB8C
                    precision = 0;
                    if (*(++fmt) == '*')
                    {
                        // DBF8
                        precision = va_arg(args, int);
                    }
                    else
                    {
                        for (;;)
                        {
                            if (*fmt < 0)
                                break;
                            if ((look_ctype_table(*fmt) & CTYPE_CIPHER) == 0)
                                break;
                            precision = precision * 10 + *(fmt++) - '0';
                        }
                        // (DBE8)
                        fmt--;
                    }
                    // DBEC
                    precision = pspMax(precision, -1);
                    continue;

                case '0':
                    // DC04
                    flag |= 0x20;
                    continue;

                case '1': case '2': case '3':
                case '4': case '5': case '6':
                case '7': case '8': case '9': {
                    // DC14
                    int cur = 0;
                    // DC18
                    for (;;)
                    {
                        cur = cur * 10 + *fmt - '0';
                        if (*(++fmt) < 0)
                            break;
                        if ((look_ctype_table(*fmt) & CTYPE_CIPHER) == 0)
                            break;
                    }
                    // DC58
                    fmt--;
                    numAlign = cur;
                }
                    continue;

                case 'D':
                    // DC64
                    flag |= 1;
                case 'd':
                case 'i': {
                    // DC70
                    base = 10;
                    if ((flag & 2) == 0)
                    {
                        // DF00
                        number = va_arg(args, int);
                        if (number < 0)
                        {
                            // DFBC
                            number = -number;
                            sign = '-';
                        }
                        goto print_base;
                    }
                    s64 num = va_arg(args, s64);
                    if (num < 0)
                    {
                        // DEE4
                        num = -num;
                        sign = '-';
                    }
                    // DC98
                    if (precision >= 0)
                        flag &= 0xFFFFFFDF;
                    s = &string[21];
                    usedPrecision = precision;
                    if (precision != 0)
                    {
                        // DCCC
                        while (num != 0) {
                            *(--s) = ciphers[__umoddi3(num, 10)];
                            num = __udivdi3(num, 10);
                        }
                        ciphers = "0123456789abcdef";
                    }
                    // DD30
                    stringLen = &string[21] - s;
                }
                    goto print_string;

                case 'L':
                    // DFD4
                    flag |= 2;
                    continue;

                case 'O':
                    // DFE0
                    flag |= 1;
                case 'o':
                    // DFEC
                    base = 8;
                    goto print_base_unsigned;

                case 'U':
                    // E028
                    flag |= 1;
                case 'u':
                    // E034
                    base = 10;
                    goto print_base_unsigned;

                case 'X':
                    // E044
                    ciphers = "0123456789ABCDEF";
                case 'x':
                    // E050
                    base = 16;
                    if ((flag & 2) == 0)
                    {
                        // E0A8
                        number = va_arg(args, int);
                        if (((flag >> 3) & 1) && number != 0)
                            flag |= 0x40;
                    }
                    else
                    {
                        u64 num = va_arg(args, u64);
                        if (num != 0 && ((flag >> 3) & 1) != 0)
                            flag |= 0x40;
                    }
                    // E010
                    // E014
                    sign = '\0';
                    goto print_base;

                case 'c':
                    // E0D8
                    s = string;
                    stringLen = 1;
                    sign = '\0';
                    string[0] = (char)va_arg(args, int);
                    goto print_string;

                case 'h':
                    // E0F8
                    flag |= 4;
                    continue;

                case 'l':
                    // E104
                    if ((flag & 1) == 0) {
                        // E124
                        flag |= 1;
                    }
                    else
                        flag = (flag & 0xFFFFFFFE) | 2;
                    continue;

                case 'n':
                    // E12C
                    if ((flag & 1) != 0 || (flag & 4) == 0) {
                        // E160
                        *va_arg(args, int*) = count;
                    }
                    else
                        *va_arg(args, short*) = count;
                    break;

                case 'p':
                    // E170
                    base = 16;
                    // E01C dup
                    number = va_arg(args, int);
                    // E010
                    // E014
                    sign = '\0';
                    goto print_base;

                case 's':
                    // E17C
                    s = va_arg(args, char*);
                    if (s == NULL) {
                        // E1DC
                        s = "(null)";
                    }
                    // E188
                    if (precision < 0) {
                        // E1C8
                        stringLen = strlen(s);
                    }
                    else
                    {
                        char *cur = memchr(s, '\0', precision);
                        stringLen = precision;
                        if (cur != NULL)
                        {
                            stringLen = cur - s;
                            if (precision < cur - s)
                                stringLen = precision;
                        }
                    }
                    // E1C0
                    sign = '\0';
                    goto print_string;

                default:
                    // E1E8
                    cb(ctx, *fmt);
                    count++;
                    // DAA0 dup
                    break;
                }
                break;

                print_base_unsigned:
                // DFF8
                if ((flag & 2) == 0) {
                    // E01C dup
                    number = va_arg(args, int);
                }
                else
                {
                    (void)va_arg(args, u64);
                }
                // E010
                // E014
                sign = '\0';

                print_base:
                // DF18
                usedPrecision = precision;
                s = &string[21];
                if (flag >= 0)
                    flag &= 0xFFFFFFDF;
                if (number != 0 || precision != 0)
                {
                    // DF48
                    // DF50
                    char lastC;
                    do
                    {
                        lastC = ciphers[number % base];
                        number /= base;
                        *(--s) = lastC;
                    } while (number != 0);
                    ciphers = "0123456789abcdef";
                    if (((flag >> 3) & 1) && base == 8 && lastC != '0')
                        *(--s) = '0';
                }
                // DD30
                stringLen = &string[21] - s;

                print_string:
                // DD38
                len = stringLen;
                if (sign != '\0')
                    len = stringLen + 1;
                if ((flag & 0x40) != 0)
                    len += 2;
                int maxSize = pspMax(len, usedPrecision);
                int i;
                if ((flag & 0x30) == 0 && numAlign != 0)
                {
                    // DD8C
                    for (i = 0; i < numAlign - maxSize; i++)
                        cb(ctx, ' ');
                }
                // DDA8
                // DDAC
                if (sign != '\0')
                {
                    // DED0
                    cb(ctx, sign);
                }
                // DDB4
                if (number != 0)
                {
                    // DEB0
                    cb(ctx, '0');
                    cb(ctx, *fmt);
                }
                // DDBC
                if ((flag & 0x30) == 0x20)
                {
                    // DE7C, DE8C
                    for (i = 0; i < numAlign - maxSize; i++)
                        cb(ctx, '0');
                }
                // DDCC
                // DDD8
                for (i = 0; i < usedPrecision - len; i++)
                    cb(ctx, '0');
                // DDF4
                // DE04
                for (i = 0; i < stringLen; i++)
                    cb(ctx, *(s++));
                // DE28
                if ((flag & 0x10) != 0)
                {
                    // DE48
                    for (i = 0; i < numAlign - maxSize; i++)
                        cb(ctx, ' ');
                }
                // DE68
                // DE6C
                count += pspMax(maxSize, numAlign);
            } while (0);
        }
        else
        {
            cb(ctx, *fmt);
            count++;
            // DAA0 dup
        }
        // DAA4
        fmt++;
    }
}

char *rindex(const char *s, char c)
{
    const char *cur = s;
    if (s == NULL)
        return NULL;
    while (*(cur++) != '\0')
        ;
    // E22C
    do
        if (*(--cur) == c)
            return (char *)cur;
    while (s < cur);
    return 0;
}

int sprintf(char *str, const char *format, ...)
{
    va_list ap;
    char *ctx[1];
    va_start(ap, format);
    ctx[0] = str;
    int ret = prnt((prnt_callback)sprintf_char, (void*)ctx, format, ap);
    va_end(ap);
    return ret;
}

int snprintf(char *str, u32 size, const char *format, ...)
{
    va_list ap;
    int ctx[3];
    va_start(ap, format);
    ctx[0] = size - 1;
    ctx[1] = 0;
    ctx[2] = (int)str;
    int ret = prnt((prnt_callback)snprintf_char, ctx, format, ap);
    va_end(ap);
    return ret;
}

void sprintf_char(int *ctx, int c)
{
    if (c >= 256)
        *((char*)*ctx) = '\0';
    else {
        *((char*)*ctx) = c;
        ctx[0]++;
    }
}

void snprintf_char(int *ctx, int c)
{
    if (c >= 256 || ctx[1] >= ctx[0])
        *((char*)ctx[2]) = '\0';
    else
    {
        *((char*)ctx[2]) = c;
        ctx[2]++;
        ctx[1]++;
    }
}

char *strcat(char *dst, const char *src)
{
    if (dst == NULL || src == NULL)
        return NULL;
    int dstLen = strlen(dst);
    int srcLen = strlen(src);
    if (dst + dstLen == src + srcLen)
        return NULL;
    memcpy(dst + dstLen, src, srcLen + 1);
    return dst;
}

char *strchr(const char *s, char c)
{
    if (s == NULL)
        return NULL;
    // E3F4
    do
    {
        if (*s == c)
            return (char *)s;
        s++;
    } while (*s != '\0');
    return NULL;
}

int strcmp(const char *s1, const char *s2)
{
    if (s1 != NULL && s2 != NULL)
    {
        // E444, E464, E468
        while (*(s1++) == *(s2++))
            if (*(s1 - 1) == '\0')
                return 0;
        // E458
        return *(s1 - 1) - *(s2 - 1);
    }
    if (s1 == s2)
        return 0;
    if (s1 != NULL)
        return 1;
    return -1;
}

char *strcpy(char *dest, const char *src)
{
    char *curDest = dest;
    if (dest == NULL || src == NULL)
        return NULL;
    // E4AC
    char c;
    do
    {
        c = *(src++);
        *(curDest++) = c;
    } while (c != '\0');
    return dest;
}

int strtol(const char *nptr, char **endptr, int base)
{
    int sign = +1;
    int num = 0;
    if (nptr == NULL)
        return 0;
    // E508
    while ((look_ctype_table(*nptr) & CTYPE_TRANSPARENT) != 0)
        nptr++;
    // E524, E6C8, E6CC
    while (*nptr == '-') {
        nptr++;
        sign = -sign;
    }
    // E534
    if (base < 2 || base > 36)
        base = 0;
    if (*nptr == '0')
    {
        // E61C
        nptr++;
        if (*nptr == 'x' || *nptr == 'X')
        {
            // E6A0
            if (base == 0 || base == 16) {
                nptr++;
                base = 16;
            }
        }
        else if (*nptr == 'b' || *nptr == 'B')
        {
            // E668
            // E66C
            if (base == 2 || base == 0) {
                nptr++;
                base = 2;
            }
        }
        else if (base == 0 || base == 8) // E64C
            base = 8;
    }
    else
    {
        if (toupper(*nptr) == '0')
        {
            nptr++;
            // E610
            base = 8;
        }
    }
    // E560
    if (base == 0)
        base = 10;
    // E568
    for (;;)
    {
        int cipher = *nptr - '0';
        nptr++;
        if ((look_ctype_table(*nptr) & CTYPE_CIPHER) == 0)
        {
            // E5E4
            cipher = 9999999;
            if ((look_ctype_table(*nptr) & CTYPE_LETTER) != 0)
                cipher = tolower(*nptr) - 'a' + 10;
        }
        // E588
        if (base < cipher)
        {
            // E5AC
            if (endptr != NULL)
                *endptr = (char*)nptr - 1;
            // E5B8
            return num * sign;
        }
        // E59C
        num = num * base + cipher;
    }
}

u32 strtoul(char *nptr, char **endptr, int base)
{
    int num = 0;
    if (nptr == NULL)
        return 0;
    // E71C
    while ((look_ctype_table(*nptr) & CTYPE_TRANSPARENT) == 0)
        nptr++;
    // E738
    if (base < 2 || base > 36)
        base = 0;
    if (base == 0)
        base = 10;
    if (*nptr == '0')
    {
        // E810
        nptr++;
        if (*nptr == 'x' || *nptr == 'X')
        {
            // E85C
            nptr++;
            base = 16;
        }
        else if (*nptr == 'b' || *nptr == 'B')
        {
            // E83C
            nptr++;
            base = 2;
        }
    }
    else if (toupper(*nptr) == 'O')
    {
        nptr++;
        // E804
        base = 8;
    }
    // E76C
    for (;;)
    {
        // E770
        int cipher = *nptr - '0';
        nptr++;
        if ((look_ctype_table(*nptr) & CTYPE_CIPHER) == 0)
        {
            // E7D8
            cipher = 9999999;
            if ((look_ctype_table(*nptr) & CTYPE_LETTER) != 0)
                cipher = tolower(*nptr) - 'a' + 10;
        }
        // E78C
        if (cipher >= base)
        {
            // E7A8
            if (endptr != NULL)
                *endptr = nptr - 1;
            // E7B4
            return num;
        }
        num = num * base + cipher;
    }
}

int strncmp(const char *s1, const char *s2, int n)
{
    if (s1 != NULL && s2 != NULL)
    {
        // E89C
        if ((--n) < 0)
            return 0;
        // E8D0, E8D4
        while (*(s1++) == *(s2++))
            if (*s1 == '\0' || (--n) < 0)
                return 0;
        // E8B4
        if (n < 0)
            return 0;
        return *(s1 - 1) - *(s2 - 1);
    }
    if (s1 == s2)
        return 0;
    if (s1 != NULL)
        return 1;
    return -1;
}

char *strncpy(char *dest, const char *src, int n)
{
    char *curDst = dest;
    if (dest == NULL || src == NULL)
        return 0;
    int i;
    // E92C
    for (i = 0; i < n; i++)
    {
        char c = *(src++);
        *(curDst++) = c;
        if (c == '\0')
        {
            // E958, E968
            while ((++i) < n)
                *(curDst++) = '\0';
            // E97C
            break;
        }
    }
    // E94C
    return dest;
}

u32 strnlen(const char *s, int maxlen)
{
    int len = 0;
    if (s == NULL || maxlen == 0)
        return 0;
    // E9A4
    while (*(s++) != '\0')
    {
        len++;
        if (len == maxlen)
            break;
    }
    return len;
}

u32 strlen(const char *s)
{
    int len = 0;
    if (s == NULL)
        return 0;
    // E9E0
    while (*(s++) != '\0')
        len++;
    return len;
}

char *strrchr(char *s, int c)
{
    char *curS = s;
    if (s == NULL)
        return NULL;
    // EA0C
    while (*(curS++) != '\0')
        ;
    curS--;
    // EA20
    do
    {
        if (*curS == (char)c)
            return curS;
    } while (s < (curS--));
    return NULL;
}

char *strpbrk(char *s, const char *accept)
{
    // EA58
    while (*(s++) != '\0')
    {
        const char *cur = accept;
        // EA68
        while (*cur != '\0')
            if (*(cur++) == *s)
                return s;
        // EA80
    }
    return NULL;
}

char *strstr(char *haystack, const char *needle)
{
    if (*needle == 0)
        return haystack;
    if (*haystack == 0)
        return NULL;
    // EAB8
    do
    {
        char *curHaystack = haystack;
        const char *curNeedle = needle;
        char n, h;
        // EAC0
        do
        {
            n = *(curNeedle++);
            if (n == '\0')
                return haystack;
            h = *(curHaystack++);
            if (h == '\0')
                return NULL;
        } while (n == h);
    } while (*(haystack++) != '\0');
    return NULL;
}

