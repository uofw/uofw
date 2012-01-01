/* Copyright (C) 2011, 2012 The uOFW team
   See the file COPYING for copying permission.
*/

#include "reboot.h"

typedef int (*PrintfCallback)(int, int, int*, int);
PrintfCallback printfCb = writeFormatToCallback; // 0x886128E4
int unkArgs[65]; // 0x88630B14
int *printfArg = unkArgs; // 0x886128E8
OutCharCb outCharCb; // 0x88630B08
InStrCb inStrCb; // 0x88630B0C
OutStrCb outStrCb; // 0x88630B10

// 0038
int *wmemcpy(int *dst, int *src, int count)
{
    int *ret = dst;
    count &= 0xFFFFFFFC;
    if (count == 0)
        return ret;
    int align = count & 0xC;
    int *end = dst + count;
    if (align != 0)
    {
        int *alignEnd = dst + align;

        // 0054
        do
            *(dst++) = *(src++);
        while (dst != alignEnd);

        if (dst == end)
            return ret;
    }

    // 0070
    do
    {
        *(dst++) = *(src++);
        *(dst++) = *(src++);
        *(dst++) = *(src++);
        *(dst++) = *(src++);
    } while (dst != end);
    return ret;
}

// 00A4
int *wmemset(int *dst, int c, int size)
{
    int *ret, end;
    size &= 0xFFFFFFFC;
    ret = dst;
    if (size == 0)
        return ret;
    end = dst + size;
    if (size & 4 != 0)
    {
        *dst = c;
        dst += 4;
        if (dst == end)
            return ret;
    }

    // C8
    do
    {
        *(dst + 0) = c;
        *(dst + 1) = c;
        dst += 8;
    } while (dst != end);
    return ret;
}

// 0D21
int memcmp(char *s1, char *s2, int n)
{
    // D28
    int i;
    char c1, c2;
    for (i = 0; i < n; i++)
        if ((c1 = *(s1++)) != (c2 = *(s2++)))
            return c1 - c2;
    return 0;
}

// 0D58
int bcmp(int arg0, int arg1, int n)
{
    if ((arg1 == 0) || (n == 0))
        return 0;
    n /= 16;
    if (n == 0)
        return 0;
    arg0 += 320;
    // D80
    int i;
    for (i = 0; i < n; i++)
    {
        int addr1 = arg1 + i * 16;
        int addr2 = arg0;
        int ret = 0;
        // D94
        int j;
        for (j = 0; j < 16; j++)
        {
            char c1 = *(char*)(addr1++);
            char c2 = *(char*)(addr2++);
            if (c1 != c2)
            {
                // DDC
                ret = c1 - c2;
                break;
            }
        }
        // DB4
        i++;
        if (ret == 0)
            return 1;
    }
    return 0;
}

char hexCharsD[] = "0123456789abcdef"; // 0x88611D70
char hexCharsU[] = "0123456789ABCDEF"; // 0x88611D84
char nullStr[] = "(null)"; // 0x88611D98

// 2BE4
int writeFormatToCallback(int *arg0, char *format, int *args, int arg3)
{
    *(short*)(arg0 + 0) = 1;
    s5 = arg0;
    *(int*)(sp + 56) = 10;
    *(int*)(sp + 32) = a3;
    *(short*)(a0 + 2) = 0;
    *(int*)(sp + 40) = 0;
    *(int*)(sp + 48) = 0;
    *(int*)(sp + 52) = 0;
    if (outCharCb == NULL)
        return 0;
    if (format == NULL)
        return 0;
    *(int*)(sp + 84) = args;
    s4 = format;
    outCharCb(a0, 0x200); // Empty buffer
    *(int*)(sp + 76) = hexCharsD;
    *(int*)(sp + 36) = 0;
    t1 = *(int*)(sp + 84);
    // 2C80
    for (;;)
    {
        if (*format == '\0')
        {
            // 2D10
            outCharCb(arg0, 0x201); // Force writing
            return *(int*)(sp + 36);
        }
        if (*format == '%')
        {
            *(int*)(sp + 60) = 0;
            // 2CB8
            t0 = 0;
            s1 = -1;
            *(int*)(sp + 68) = 0;
            *(int*)(sp + 72) = 0;
            // 2CC8
            format++;
            // 2CCC
            for (;;)
            {
                a1 = *(char*)s4;
                v1 = (int)(char)a1;
                a2 = a1;
                char goto2CB0 = 0;
                char goto2E88 = 0;
                char goto2F50 = 0;
                char goto2F64 = 0;
                char goto2F70 = 0;
                char goto3184 = 0;
                char goto3264 = 0;
                char goto3268 = 0;
                char goto3284 = 0;
                char goto3320 = 0;
                switch (v1)
                {
                case '\0':
                    // 2D10
                    outCharCb(arg0, 0x201); // Force writing
                    return *(int*)(sp + 36);
                case ' ':
                    // 2CF8
                    if (*(int*)(sp + 72) == 0)
                        *(int*)(sp + 72) = ' ';
                    break;
                case '#':
                    t0 |= 8;
                    break;
                case '(':
                    // _2D5C
                    v0 = *(int*)t1;
                    t1 += 4;
                    *(int*)(sp + 68) = v0;
                    if (v0 >= 0)
                        break;
                    s0 = v0;
                    *(int*)(sp + 68) = s0;
                case '+':
                    // 2D74
                    t0 |= 0x10;
                    break;
                case ')':
                    // 2D7C
                    *(int*)(sp + 72) = '+';
                    break;
                case ',':
                    // 2D88
                    s4++;
                    a2 = *(char*)sp;
                    s1 = '*';
                    s3 = (int)(char)a2;
                    if (s3 == s1)
                    {
                        // 2DF0
                        s0 = *(int*)t1;
                        t1 += 4;
                    }
                    else
                    {
                        // 2DB0
                        while ((a2 - '0') & 0xFF < 10)
                        {
                            s4++;
                            t4 = (int)(char)a2;
                            a2 = *(char*)s4;
                            s0 = (s0 * 10) + (t4 - '0');
                        }
                        // 2DE0
                        s4--;
                    }
                    // 2DE4
                    s1 = max(s0, -1);
                    break;
                case '0':
                    // 2DFC
                    t0 |= 0x20;
                    break;
                case '1': case '2': case '3':
                case '4': case '5': case '6':
                case '7': case '8': case '9':
                    s0 = 0;
                    // 2E0C
                    do
                    {
                        s4++;
                        a1 = *(char*)s4;
                        a2 = (int)(char)a2 * 11;
                        t6 = (int)(char)a1;
                        s0 = a2 - '0';
                        a2 = a1;
                        if (t6 < 0)
                            break;
                    } while ((a1 - '0') & 0xFF < 10);
                    // 2E48
                    s4--;
                    *(int*)(sp + 68) = s0;
                    break;
                case 'l':
                    // 3378
                    fp = t0 & 1;
                    if (fp == 0) {
                        t0 |= 1;
                        break;
                    }
                    t0 &= 0xFFFFFFFE;
                case 'L':
                    // 322C
                    t0 |= 2;
                    break;
                case 'O':
                    t0 |= 1;
                case 'o':
                    // 3238
                    a0 = t0 & 2;
                    if (a0 == 0)
                    {
                        t7 = *(int*)t1;
                        // 3278
                        v0 = 8;
                        t1 += 4;
                        *(int*)(sp + 40) = t7;
                        goto3284 = 1;
                    }
                    t6 = t1 + 7;
                    t6 &= 0xFFFFFFF8;
                    s2 = *(int*)(t6 + 0);
                    s3 = *(int*)(t6 + 4);
                    t1 = t6 + 8;
                    a3 = 8;
                    *(int*)(sp + 48) = s2;
                    *(int*)(sp + 52) = s3;
                    goto3264 = 1;
                    break;
                case 'U':
                    t0 |= 1;
                case 'u':
                    // 3290
                    a0 = t0 & 2;
                    if (a0 == 0)
                    {
                        v1 = *(int*)t1;
                        // 32C0
                        v0 = 10;
                        t1 += 4;
                        *(int*)(sp + 40) = v1;
                        goto3284 = 1;
                    }
                    t4 = t1 + 7;
                    t4 &= 0xFFFFFFF8;
                    t6 = *(int*)(t4 + 0);
                    t7 = *(int*)(t4 + 4);
                    t1 = t4 + 8;
                    *(int*)(sp + 48) = t6;
                    *(int*)(sp + 52) = t7;
                    a3 = 10;
                    goto3264 = 1;
                    break;
                case 'X':
                    // 32D0
                    *(int*)(sp + 76) = hexCharsU;
                case 'x':
                    // 32DC
                    a2 = 16;
                    a0 = t0 & 2;
                    *(int*)(sp + 56) = a2;
                    if (a0 == 0)
                    {
                        // 3328
                        t2 = *(int*)t1;
                        t8 = (t0 >> 3) & 1;
                        *(int*)(sp + 40) = t2;
                        fp = *(int*)(sp + 40);
                        t9 = (fp != 0);
                        s7 = t8 & t9;
                        t1 += 4;
                        if (s7 == 0)
                            goto3268 = 1;
                        else {
                            t0 |= 0x40;
                            goto3320 = 1;
                        }
                    }
                    else
                    {
                        s0 = t1 + 7;
                        s0 &= 0xFFFFFFF8;
                        a2 = *(int*)(s0 + 0);
                        a3 = *(int*)(s0 + 4);
                        t1 = s0 + 8;
                        t7 = a2 | a3;
                        *(int*)(sp + 48) = a2;
                        *(int*)(sp + 52) = a3;
                        v1 = (t0 >> 3) & 1;
                        if (t7 == 0 || v1 == 0)
                            goto3268 = 1;
                        else {
                            t0 |= 0x40;
                            goto3320 = 1;
                        }
                    }
                    break;
                case 'c':
                    // 3350
                    s1 = *(char*)t1;
                    t2 = 1;
                    s2 = sp;
                    t1 += 4;
                    *(int*)(sp + 64) = t2;
                    *(int*)(sp + 72) = 0;
                    *(char*)sp = s1;
                    goto2F70 = 1;
                    break;
                case 'D':
                    t0 |= 1;
                case 'd': case 'i':
                    // 2E58
                    a3 = t0 & 2;
                    if (a3 == 0)
                    {
                        t5 = *(int*)t1;
                        // 316C
                        *(int*)(sp + 40) = t5;
                        v0 = *(int*)(sp + 40);
                        t1 += 4;
                        if (v0 < 0)
                        {
                            // 3218
                            a2 = -v0;
                            t6 = '-';
                            *(int*)(sp + 40) = a2;
                            *(int*)(sp + 72) = t6;
                        }
                        // 317C
                        *(int*)(sp + 56) = 10;
                        goto3184 = 1;
                    }
                    else
                    {
                        t3 = t1 + 7;
                        t3 &= 0xFFFFFFF8;
                        a0 = *(int*)(t3 + 0);
                        a1 = *(int*)(t3 + 4);
                        *(int*)(sp + 48) = a0;
                        *(int*)(sp + 52) = a1;
                        t1 = t3 + 8;
                        if (a1 < 0)
                        {
                            // 3140
                            a1 = a0;
                            a3 = a1;
                            a0 = -a2;
                            a1 = -a3;
                            v0 = (a0 != 0);
                            a1 -= v0;
                            v0 = '-';
                            *(int*)(sp + 48) = a0;
                            *(int*)(sp + 52) = a1;
                            *(int*)(sp + 72) = v0;
                        }
                        // 2E84
                        a0 = *(int*)(sp + 48);
                        goto2E88 = 1;
                    }
                    break;
                case 'h':
                    // 3370
                    t0 |= 4;
                    break;
                case 'n':
                    // 338C
                    s0 = t0 & 1;
                    t9 = t0 & 4;
                    if (s0 == 0)
                    {
                        // 33C4
                        t3 = *(int*)(sp + 32);
                        if (t9 == 0)
                        {
                            // 33F8
                            a0 = *(int*)t1;
                            t1 += 4;
                            if (t3 != 0 && k1 & a0 < 0)
                                return 0x800200D3;
                            // 3414
                            v1 = *(int*)(sp + 36);
                            *(int*)a0 = v1;
                        }
                        else
                        {
                            fp = *(int*)(sp + 32);
                            a0 = *(int*)t1;
                            t1 += 4;
                            if (fp != 0 && k1 & a0 < 0)
                                return 0x800200D3;
                            // 33EC
                            a3 = *(int*)(sp + 36);
                            *(short*)a0 = a3;
                        }
                    }
                    else
                    {
                        t0 = *(int*)(sp + 32);
                        a0 = *(int*)t1;
                        t1 += 4;
                        if (t0 != 0 && k1 & a0 < 0)
                            return 0x800200D3;
                        // 33B8
                        t8 = *(int*)(sp + 36);
                        *(int*)a0 = t8;
                    }
                    goto2CB0 = 1;
                    break;
                case 'p':
                    // 3420
                    s3 = *(int*)t1;
                    a0 = 16;
                    t1 += 4;
                    *(int*)(sp + 40) = s3;
                    *(int*)(sp + 56) = a0;
                    goto3320 = 1;
                    break;
                case 's':
                    // 3438
                    s2 = *(int*)t1;
                    t8 = *(int*)(sp + 32);
                    t9 = nullStr;
                    t1 += 4;
                    if (s2 == 0)
                        s2 = t9;
                    if (t8 != 0 && k1 & s2 < 0)
                        return 0x800200D3;
                    // 3464
                    if (s1 < 0)
                    {
                        // 34AC
                        *(int*)(sp + 80) = t0;
                        *(int*)(sp + 84) = t1;
                        v0 = strlen(a0);
                        *(int*)(sp + 64) = v0;
                        t1 = *(int*)(sp + 84);
                        t0 = *(int*)(sp + 80);
                    }
                    else
                    {
                        *(int*)(sp + 80) = t0;
                        *(int*)(sp + 84) = t1;
                        v0 = strnchr(s2, 0, s1);
                        *(int*)(sp + 84) = s1;
                        t0 = *(int*)(sp + 80);
                        t1 = *(int*)(sp + 84);
                        if (v0 != 0)
                        {
                            t3 = v0 - s2;
                            a3 = s1 < t3;
                            *(int*)(sp + 64) = t3;
                            if (a3 != 0)
                                *(int*)(sp + 64) = s1;
                        }
                    }
                    // 34A4
                    *(int*)(sp + 72) = 0;
                    goto2F70 = 1;
                    break;
                default:
                    // 34C8
                    *(int*)(sp + 84) = t1;
                    outCharCb((int)(char)a1, arg0);
                    *(int*)(sp + 36)++;
                    // 2CAC
                    t1 = *(int*)(sp + 84);
                    goto2CB0 = 1;
                    break;
                }
                if (goto3284)
                {
                    // 3284
                    *(int*)(sp + 56) = 8;
                    goto3268 = 1;
                }
                if (goto3264)
                {
                    // 3264
                    *(int*)(sp + 56) = a3;
                    goto3268 = 1;
                }
                if (goto3320)
                {
                    // 3320
                    a0 = t0 | 2;
                    goto3268 = 1;
                }
                if (goto3268)
                {
                    // 3268
                    *(int*)(sp + 72) = 0;
                    if (a0 == 0)
                        goto3184 = 1;
                    else {
                        a0 = *(int*)(sp + 48);
                        goto2E88 = 1;
                    }
                }
                if (goto3184)
                {
                    // 3184
                    t5 = *(int*)(sp + 40);
                    t3 = t0;
                    t6 = (s1 != 0);
                    t4 = (t5 != 0);
                    t3 &= 0xFFFFFFDF;
                    a0 = (s1 < 0);
                    a3 = t4 | t6;
                    *(int*)(sp + 60) = s1;
                    if (a0 == 0)
                        t0 = t3;
                    s2 = sp + 21;
                    if (a3 == 0)
                        goto2F64 = 1;
                    else
                    {
                        // 31B4
                        do
                        {
                            a1 = *(int*)(sp + 56);
                            a2 = *(int*)(sp + 40);
                            s2--;
                            // 31C4
                            v0 = *(int*)(sp + 76);
                            t7 = a2 % a1;
                            v1 = v0 + t7;
                            a0 = *(char*)v1;
                            s1 = a2 / a1;
                            *(int*)(sp + 40) = s1;
                            *(char*)s2 = a0;
                        } while (s1 != 0);
                        t9 = a1 ^ 8;
                        t8 = (t9 == 0);
                        s7 = (t0 >> 3) & 1;
                        s3 = hexCharsD;
                        s0 = s7 & t8;
                        *(int*)(sp + 76) = s3;
                        v0 = (int)(char)a0;
                        if (s0 != 0)
                            goto2F50 = 1;
                        else
                            goto2F64 = 1;
                    }
                }
                if (goto2E88)
                {
                    // 2E88
                    t4 = *(int*)(sp + 52);
                    s2 = t0;
                    a3 = (s1 != 0);
                    t3 = a0 | t4;
                    a1 = (t3 != 0);
                    s2 &= 0xFFFFFFDF;
                    t2 = (s1 < 0);
                    fp = a1 | a3;
                    if (t2 == 0)
                        t0 = s2;
                    *(int*)(sp + 60) = s1;
                    s2 = sp + 21;
                    if (fp == 0)
                        goto2F64 = 1;
                    else
                    {
                        t6 = *(int*)(sp + 56);
                        s7 = t6 >> 31;
                        // 2EC0
                        do
                        {
                            s1 = *(int*)(sp + 56);
                            *(int*)(sp + 80) = t0;
                            s2--;
                            *(int*)(sp + 84) = t1;
                            v0 = sub_7888(*(int*)(sp + 48), *(int*)(sp + 52), s1, s7);
                            t7 = *(int*)(sp + 76);
                            v1 = t7 + v0;
                            s3 = *(char*)v1;
                            *(char*)s2 = s3;
                            v0,v1 = sub_76A0(*(int*)(sp + 48), *(int*)(sp + 52), s1, s7);
                            *(int*)(sp + 48) = v0;
                            *(int*)(sp + 52) = v1;
                            t0 = *(int*)(sp + 48);
                            t1 = *(int*)(sp + 52);
                            t5 = t0 | t1;
                            t0 = *(int*)(sp + 80);
                            t1 = *(int*)(sp + 84);
                        } while (t5 != 0);
                        t9 = *(int*)(sp + 56);
                        s0 = (t0 >> 3) & 1;
                        v0 = hexCharsD;
                        t8 = t9 ^ 8;
                        s7 = (t8 == 0);
                        a2 = s0 & s7;
                        *(int*)(sp + 76) = v0;
                        if (a2 == 0)
                            goto2F64 = 1;
                        else {
                            v0 = (int)(char)s3;
                            goto2F50 = 1;
                        }
                    }
                }
                if (goto2F50)
                {
                    // 2F50
                    v1 = '0';
                    if (v0 != v1)
                    {
                        s2--;
                        *(char*)s2 = v1;
                    }
                    goto2F64 = 1;
                }
                if (goto2F64)
                {
                    // 2F64
                    t2 = sp - s2;
                    // 2F68
                    s3 = t2 + 21;
                    *(int*)(sp + 64) = s3;
                    goto2F70 = 1;
                }
                if (goto2F70)
                {
                    // 2F70
                    fp = *(int*)(sp + 64);
                    t5 = *(int*)(sp + 72);
                    t3 = *(int*)(sp + 60);
                    a1 = fp + 1;
                    s1 = fp;
                    if (t5 != 0)
                        s1 = a1;
                    a1 = *(int*)(sp + 68);
                    fp = t0 & 0x30;
                    s7 = t0 & 0x40;
                    t4 = s1 + 2;
                    a0 = (fp == 0);
                    t6 = (a1 != 0);
                    if (s7 != 0)
                        s1 = t4;
                    a3 = a0 & t6;
                    s3 = max(s1, t3);
                    if (a3 != 0 && s3 < a1)
                    {
                        s0 = a1 - s3;
                        // 2FC0
                        do
                        {
                            *(int*)(sp + 80) = t0;
                            *(int*)(sp + 84) = t1;
                            s0--;
                            outCharCb(arg0, ' ');
                            t0 = *(int*)(sp + 80);
                            t1 = *(int*)(sp + 84);
                        } while (s0 != 0);
                    }
                    // (2FE4)
                    a3 = *(int*)(sp + 72);
                    // 2FE8
                    if (a3 != 0)
                    {
                        *(int*)(sp + 80) = t0;
                        // 3124
                        *(int*)(sp + 84) = t1;
                        outCharCb(arg0, a3);
                        t1 = *(int*)(sp + 84);
                        t0 = *(int*)(sp + 80);
                    }
                    // 2FF0
                    if (s7 != 0)
                    {
                        *(int*)(sp + 80) = t0;
                        // 30FC
                        *(int*)(sp + 84) = t1;
                        outCharCb(arg0, '0');
                        outCharCb(arg0, *(char*)s4);
                        t1 = *(int*)(sp + 84);
                        t0 = *(int*)(sp + 80);
                    }
                    // 2FF8
                    v1 = *(int*)(sp + 68);
                    if (fp == ' ')
                    {
                        // 30C4
                        a2 = s3 < v1;
                        s0 = v1 - s3;
                        if (a2 != 0)
                        {
                            // 30D0
                            do
                            {
                                *(int*)(sp + 80) = t0;
                                *(int*)(sp + 84) = t1;
                                s0--;
                                outCharCb(arg0, '0');
                                t0 = *(int*)(sp + 80);
                                t1 = *(int*)(sp + 84);
                            } while (s0 != 0);
                        }
                    }
                    // (3004)
                    a0 = *(int*)(sp + 60);
                    // 3008
                    s0 = a0 - s1;
                    if (s1 < a0)
                    {
                        // 3018
                        do
                        {
                            *(int*)(sp + 80) = t0;
                            *(int*)(sp + 84) = t1;
                            s0--;
                            outCharCb(arg0, '0');
                            t0 = *(int*)(sp + 80);
                            t1 = *(int*)(sp + 84);
                        } while (s0 != 0);
                    }
                    // 303C
                    v0 = *(int*)(sp + 64);
                    s0 = v0 - 1;
                    // 304C
                    while (s0 >= 0)
                    {
                        *(int*)(sp + 80) = t0;
                        s0--;
                        *(int*)(sp + 84) = t1;
                        s2++;
                        outCharCb(arg0, *(char*)s2);
                        t0 = *(int*)(sp + 80);
                        t1 = *(int*)(sp + 84);
                    }
                    s2 = t0 & 0x10;
                    // 3078
                    if (s2 != 0)
                    {
                        a1 = *(int*)(sp + 68);
                        s0 = a2 - s3;
                        if (s3 < a2)
                        {
                            // 3090
                            do
                            {
                                *(int*)(sp + 84) = t1;
                                s0--;
                                outCharCb(arg0, ' ');
                                t1 = *(int*)(sp + 84);
                            } while (s0 != 0);
                        }
                    }
                    t9 = *(int*)(sp + 68);
                    // 30B0
                    s7 = *(int*)(sp + 36);
                    t8 = max(s3, t9);
                    s0 = s7 + t8;
                    *(int*)(sp + 36) = s0;
                    goto2CB0 = 1;
                }
                if (goto2CB0)
                    break;
                // 2CC8
                s4++;
            }
        }
        else
        {
            *(int*)(sp + 84) = t1;
            outCharCb(arg0, a1);
            *(int*)(sp + 36)++;
            // 2CAC
            t1 = *(int*)(sp + 84);
        }
        // 2CB0
        s4++;
    }
}

// 35D0
int printf(int arg0, int arg1, int arg2, int arg3, int arg4, int arg5, int arg6, int arg7)
{
    int oldK1 = pspKernelGetK1();
    int ic;
    int tab[7];
    int ret = 0;
    pspKernelSetK1(oldK1 << 11);
    tab[0] = arg1;
    tab[1] = arg2;
    tab[2] = arg3;
    tab[3] = arg4;
    tab[4] = arg5;
    tab[5] = arg6;
    tab[6] = arg7;
    ic = sub_3A84();
    if (printfCb != NULL && sub_3824(56) != 0)
        ret = printfCb(printfArg, arg0, tab, 0);

    sub_3AB0(ic);
    pspKernelSetK1(oldK1);
    return ret;
}

// 36B4
void setOutCharCb(OutCharCb cb)
{
    outCharCb = cb;
    unkVar = 0;
}

// 3798
int setInStrCb(InStrCb cb)
{
    inStrCb = arg;
    return 0;
}

// 37D8
int setOutStrCb(OutStrCb cb)
{
    outStrCb = cb;
    return 0;
}

// 3B64
int bzero(int addr, int n)
{
    return sub_3C88(addr, 0, n);
}

char globUnk[] = // 0x8861245D
{ 0x20, 0x20, 0x20,
  0x20, 0x20, 0x20, 0x20,
  0x20, 0x20, 0x08, 0x08,
  0x08, 0x08, 0x08, 0x20,
  0x20, 0x20, 0x20, 0x20,
  0x20, 0x20, 0x20, 0x20,
  0x20, 0x20, 0x20, 0x20,
  0x20, 0x20, 0x20, 0x20,
  0x20, 0x18, 0x10, 0x10, 
  0x10, 0x10, 0x10, 0x10,
  0x10, 0x10, 0x10, 0x10,
  0x10, 0x10, 0x10, 0x10,
  0x10, 0x04, 0x04, 0x04,
  0x04, 0x04, 0x04, 0x04,
  0x04, 0x04, 0x04, 0x10,
  0x10, 0x10, 0x10, 0x10,
  0x10, 0x10, 0x41, 0x41,
  0x41, 0x41, 0x41, 0x41,
  0x01, 0x01, 0x01, 0x01,
  0x01, 0x01, 0x01, 0x01,
  0x01, 0x01, 0x01, 0x01,
  0x01, 0x01, 0x01, 0x01,
  0x01, 0x01, 0x01, 0x01,
  0x10, 0x10, 0x10, 0x10,
  0x10, 0x10, 0x42, 0x42,
  0x42, 0x42, 0x42, 0x42,
  0x02, 0x02, 0x02, 0x02,
  0x02, 0x02, 0x02, 0x02,
  0x02, 0x02, 0x02, 0x02,
  0x02, 0x02, 0x02, 0x02,
  0x02, 0x02, 0x02, 0x02,
  0x10, 0x10, 0x10, 0x10,
  0x20 };


char sub_3BCC(int arg) // ???????????
{
    return globUnk[arg];
}

// 3BE8
int memchr(char *s, char c, int n)
{
    if (s == NULL)
        return 0;
    // 3BFC
    while ((n--) > 0)
    {
        if (*s == c)
            return s;
        s++;
    }
    return 0;
}

// 3C20
char *memcpy(char *dst, char *src, int count)
{
    char *ptr;
    if (dst == NULL)
        return 0;
    if ((((u32)dst | (u32)src) | count) & 3 == 0) // aligned
    {
        // 3C78
        return wmemcpy(dst, src, count);
    }

    ptr = dst;
    // 3C50
    for (; count != 0; count--)
        *(ptr++) = *(src++);
    return dst;
}

// 3C88
int memset(void *s, int c, int n)
{
    void *origPtr = s;
    if (s == NULL)
        return s;

    if (c == 0 && ((s | n) & 3) == 0) // Faster version when filling with 0, on aligned addresses?
    {
        // 3CE8
        return wmemset(s, c, n);
    }
    
    // 3CBC
    c &= 0xFF;
    while (n > 0)
    {
        n--;
        *(s8*)addr = c;
        addr++;
    }

    return origAddr;
}

// 3D6C
int strncmp(char *s1, char *s2, int cnt)
{
    while (cnt > 0)
    {
        char c1 = *(s1++);
        char c2 = *(s2++);
        if (c1 != c2)
        {
            if (c1 >= c2)
                return 1;
            else
                return -1;
        }
        cnt--;
    }
    return 0;
}

// 3DB4
int strcmp(char *s1, char *s2)
{
    if (s1 != NULL && s2 != NULL)
    {
        // 3DE4
        while (*s1 == *s2)
        {
            // 3E04
            // 3E08
            if (*s1 == '\0')
                return 0;
            s1++;
            s2++;
        }
        return *s1 - *s2;
    }
    if (s1 == s2)
        return 0;
    if (s1 != 0)
        return 1;
    return -1;
}

// 3E34
int strlen(char *str)
{
    int count = 0;
    if (str == NULL)
        return 0;

    while (*(str++) != '\0')
        count++;
    return count;
}

// 3E68
int strchr(char *s, char c)
{
    if (s == NULL)
        return 0;
    do
    {
        if (*s == c)
            return s;
    } while (*(s++) != 0);
    return 0;
}

// 3E98
int strncmp(char *s1, char *s2, int count)
{
    if (s1 != NULL && s2 != NULL)
    {
        count--;
        // 3EC8
        if (count < 0)
            return 0;

        a3 = *s2;
        v1 = *s1;
        s2++;
        if (v1 == a3)
        {
            a3 = v1;
            // 3F00
            do
            {
                count--;
                s1++;
                if (a3 == 0)
                    return 0;
                if (count < 0)
                    return 0;
                t2 = *s1;
                s2++;
                a3 = t2;
            } while (t2 == *s2);
        }
        // 3EE0
        if (count < 0)
            return 0;
        return s1[0] - s2[-1];
    }
    if (s1 == s2)
        return 0;
    if (s1 != NULL)
        return 1;
    else
        return -1;
}

// 3F34
int strncpy(char *dst, char *src, int count)
{
    char *dstPtr = dst;
    if ((dst == NULL) || (src == NULL))
        return 0;
    int i = 0;
    if (count > 0)
    {
        // 3F58
        do
        {
            char c = *(src++);
            *(dstPtr++) = c;
            if (c == '\0')
            {
                // 3F84 / 3F94
                while ((++i) < count)
                    *(dstPtr++) = '\0';
                return dst;
            }
            i++;
        } while (i < count);
    }
    return dst;
}

// 3FB0
int strrchr(char *s, char c)
{
    char *s2 = s;
    if (s == NULL)
        return 0;
    // 3FC0
    while (*(s2++) != 0)
        ;
    // 3FD4
    do
        if (*(--s2) == c)
            return s2;
    while (s < s2);
    return 0;
}

char hexCharsD2[] = "0123456789abcdef"; // 0x88611DA0
char hexCharsU2[] = "0123456789ABCDEF"; // 0x88611DB4
char nullStr2[] = "(null)"; // 0x88611DC8

// 3FF8
int readFormatFromCallback(int arg0, int arg1, int arg2, int arg3)
{
    v0 = 0;
    s6 = arg0;
    s5 = arg1;
    *(int*)(sp + 36) = 0;
    if (arg2 == 0)
        return 0;
    *(int*)(sp + 64) = arg3;
    s3 = arg2;
    void (*func)(int, int) = s6;
    func(arg1, 512);
    *(int*)(sp + 60) = hexCharsD2;
    *(int*)(sp + 32) = 0;
    t0 = *(int*)(sp + 64);
    // 4060
    for (;;)
    {
        a1 = *(char*)s3;
        v1 = '%'
        if (a1 == 0)
        {
            // 40F0
            func(s5, 513);
            return *(int*)(sp + 32);
        }
        if (a1 == v1)
        {
            *(int*)(sp + 44) = 0;
            // 4098
            s4 = -1;
            *(int*)(sp + 40) = 0;
            *(int*)(sp + 52) = 0;
            *(int*)(sp + 56) = 0;
            // 40A8
            s3++;
            // 40AC
            for (;;)
            {
                a1 = *(char*)s3;
                v1 = (int)(char)a1;
                a0 = a1;
                char goto4318 = 0;
                char goto4504 = 0;
                char goto45E4 = 0;
                char goto4608 = 0;
                char goto4600 = 0;
                char goto4324 = 0;
                char goto4090 = 0;
                switch (v1)
                {
                case '\0':
                    // 40F0
                    func(s5, 513);
                    return *(int*)(sp + 32);
                case ' ':
                    // 40D8
                    if (*(int*)(sp + 56) == 0)
                        *(int*)(sp + 56) = ' ';
                    break;
                case '#':
                    // 4130
                    v0 = *(int*)(sp + 44) | 8;
                    // 4138
                    *(int*)(sp + 44) = v0;
                    break;
                case '*':
                    // 4140
                    v0 = *(int*)t0;
                    t0 += 4;
                    *(int*)(sp + 52) = v0;
                    if (v0 >= 0)
                        break;
                    t5 = -v0;
                    *(int*)(sp + 52) = t5;
                case '-':
                    // 4158
                    v1 = *(int*)(sp + 44) | 0x10;
                    // 4160
                    *(int*)(sp + 44) = v1;
                    break;
                case '+':
                    // 4168
                    *(int*)(sp + 56) = 43;
                    break;
                case '.':
                    // 4174
                    s3++;
                    a0 = *(char*)s3;
                    s1 = '*';
                    s2 = (int)(char)a0;
                    s0 = 0;
                    if (s2 == s1)
                    {
                        // 41E0
                        s0 = *(int*)t0;
                        t0 += 4;
                        // 41D4
                        s4 = max(s0, -1);
                    }
                    else
                    {
                        // 418C
                        for (;;)
                        {
                            a0 = (int)(char)a0;
                            if (a0 < 0)
                                break;
                            *(int*)(sp + 64) = t0;
                            v0 = sub_3BCC(a0);
                            t0 = *(int*)(sp + 64);
                            if (v0 & 4 == 0)
                                break;
                            fp = s0 << 2;
                            t8 = *(char*)s3;
                            t9 = fp + s0;
                            s3++;
                            s0 = t9 << 1;
                            a0 = *(char*)s3;
                            s7 = s0 + t8;
                            s0 = s7 - 48;
                        }
                        // 41D0
                        s3--;
                        // 41D4
                        s4 = max(s0, -1);
                    }
                    break;
                case '0':
                    // 41EC
                    a1 = *(int*)(sp + 44) | 0x20;
                    // 41F4
                    *(int*)(sp + 44) = a1;
                    break;
                case '1': case '2': case '3':
                case '4': case '5': case '6':
                case '7': case '8': case '9':
                    // 41FC
                    s0 = 0;
                    // 4200
                    for (;;)
                    {
                        a1 = s0 << 2;
                        s3++;
                        t2 = a1 + s0;
                        a1 = *(char*)s3;
                        a3 = t2 << 1;
                        t1 = (int)(char)a0;
                        a2 = a3 + t1;
                        s0 = a2 - 48;
                        if (a1 < 0)
                            break;
                        *(int*)(sp = 64) = t0;
                        v0 = sub_3BCC(a1);
                        t3 = v0 & 4;
                        t0 = *(int*)(sp + 64);
                        if (t3 == 0)
                            break;
                        a0 = *(char*)s3;
                    }
                    // 4244
                    s3--;
                    *(int*)(sp + 52) = s0;
                    break;
                case 'D':
                    // 4250
                    *(int*)(sp + 44) |= 1;
                case 'd':
                case 'i':
                    // 425C
                    fp = *(int*)(sp + 44);
                    t9 = fp & 2;
                    a2 = '\n';
                    if (t9 == 0)
                    {
                        // 44EC
                        a3 = *(int*)t0;
                        t0 += 4;
                        *(int*)(sp + 36) = a3;
                        a0 = *(int*)(sp + 36);
                        v1 = (a0 != 0);
                        if (a0 < 0)
                        {
                            // 45A8
                            t2 = -a0;
                            t3 = 45;
                            v1 = (t2 != 0);
                            *(int*)(sp + 36) = t2;
                            *(int*)(sp + 56) = t3;
                        }
                        goto4504 = 1;
                    }
                    else
                    {
                        v1 = t0 + 7;
                        v1 &= 0xFFFFFFF8;
                        s0 = *(int*)(v1 + 0);
                        s1 = *(int*)(v1 + 4);
                        t0 = v1 + 8;
                        if (s1 < 0)
                        {
                            // 44D0
                            s0 = -s0;
                            s1 = -s1;
                            v0 = (s0 != 0);
                            s1 -= v0;
                            *(int*)(sp + 56) = 45;
                        }
                        // 4284
                        t2 = *(int*)(sp + 44);
                        s2 = *(int*)(sp + 44);
                        t4 = s0 | s1;
                        a3 = (t4 != 0);
                        a1 = (s4 != 0);
                        t2 &= 0xFFFFFFDF;
                        a0 = s4 < 0;
                        if (a0 == 0)
                            s2 = t2;
                        t1 = t3 | a1;
                        *(int*)(sp + 44) = s2;
                        s2 = sp + 21;
                        *(int*)(sp + 40) = s4;
                        if (t1 != 0)
                        {
                            // 42B8
                            do
                            {
                                *(int*)(sp + 64) = t0;
                                v0,v1 = sub_7888(s0, s1, '\n', 0);
                                s2--;
                                t5 = *(int*)(sp + 60);
                                s4 = t5 + v0;
                                t0 = *(char*)s4;
                                *(char*)s2 = t0;
                                v0,v1 = sub_76A0(s0, s1, '\n', 0);
                                s1 = v1;
                                v1 = v0 | v1;
                                s0 = v0;
                                t0 = *(int*)(sp + 64);
                            } while (v1 != 0);
                            *(int*)(sp + 60) = hexCharsD2;
                        }
                        goto4318 = 1;
                    }
                    break;
                case 'L':
                    // 45C0
                    v0 = *(int*)(sp + 44) | 2;
                    // 4138
                    *(int*)(sp + 44) = v0;
                    break;
                case 'O':
                    // 45CC
                    *(int*)(sp + 44) |= 1;
                case 'o':
                    // 45D8
                    t9 = *(int*)(sp + 44);
                    a2 = 8;
                    v0 = t9 & 2;
                    goto45E4 = 1;
                    break;
                case 'U':
                    // 4614
                    *(int*)(sp + 44) |= 1;
                case 'u':
                    // 4620
                    a2 = *(int*)(sp + 44);
                    v0 = a2 & 2;
                    a2 = 10;
                    goto45E4 = 1;
                    break;
                case 'X':
                    // 4630
                    *(int*)(sp + 60) = hexCharsU2;
                case 'x':
                    // 463C
                    v1 = *(int*)(sp + 44);
                    t6 = v1 & 2;
                    a2 = 16;
                    if (t6 == 0)
                    {
                        // 4694
                        v1 = *(int*)t0;
                        a0 = *(int*)(sp + 44);
                        *(int*)(sp + 36) = v1;
                        fp = (a0 >> 3) & 1;
                        a3 = *(int*)(sp + 36);
                        v1 = (a3 != 0);
                        t9 = fp & v1;
                        t0 += 4;
                        if (t9 != 0)
                        {
                            t1 = a0 | 0x40;
                            *(int*)(sp + 44) = t1;
                        }
                    }
                    else
                    {
                        s7 = t0 + 7;
                        s7 &= 0xFFFFFFF8;
                        s0 = *(int*)(s7 + 0);
                        s1 = *(int*)(s7 + 4);
                        t0 = s7 + 8;
                        t7 = s0 | s1;
                        v1 = (v1 >> 3) & 1;
                        if (t7 == 0)
                        {
                            // 468C
                            v0 = *(int*)(sp + 36);
                            // 45FC
                            v1 = (v0 != 0);
                        }
                        else
                        {
                            v0 = *(int*)(sp + 36);
                            if (v1 == 0)
                            {
                                // 45FC
                                v1 = (v0 != 0);
                            }
                            else
                            {
                                t8 = *(int*)(sp + 44);
                                s1 = *(int*)(sp + 36);
                                s0 = t8 | 0x40;
                                v1 = (s1 != 0);
                                *(int*)(sp + 44) = s0;
                            }
                        }
                    }
                    goto4600 = 1;
                    break;
                case 'c':
                    // 46C4
                    s4 = *(char*)t0;
                    s7 = 1;
                    s2 = sp;
                    t0 += 4;
                    *(int*)(sp + 48) = s7;
                    *(int*)(sp + 56) = 0;
                    *(char*)(sp + 0) = s4;
                    goto4324 = 1;
                    break;
                case 'h':
                    // 46E4
                    t5 = *(int*)(sp + 44);
                    v1 = t5 | 4;
                    // 4160
                    *(int*)(sp + 44) = v1;
                    break;
                case 'l':
                    // 46F0
                    a0 = *(int*)(sp + 44);
                    t6 = a0 & 1;
                    s1 = *(int*)(sp + 44);
                    if (t6 == 0)
                    {
                        // 4710
                        a1 = s1 | 1;
                        // 41F4
                        *(int*)(sp + 44) = a1;
                    }
                    else
                    {
                        a0 &= 0xFFFFFFFE;
                        t7 = a0 | 2;
                        *(int*)(sp + 44) = t7;
                    }
                    break;
                case 'n':
                    // 4718
                    if (*(int*)(sp + 44) & 1 != 0 || *(int*)(sp + 44) & 4 == 0) {
                        s4 = *(int*)t0;
                        // 474C
                        s7 = *(int*)(sp + 32);
                        t0 += 4;
                        *(int*)s4 = s7;
                    }
                    else
                    {
                        t6 = *(int*)t0;
                        t7 = *(int*)(Sp + 32);
                        t0 += 4;
                        *(short*)t6 = t7 & 0xFFFF;
                    }
                    goto4090 = 1;
                    break;
                case 'p':
                    // 475C
                    v0 = *(int*)t0;
                    a2 = 16;
                    goto4608 = 1;
                    break;
                case 's':
                    // 4768
                    s2 = *(int*)t0;
                    v1 = nullStr2;
                    t0 += 4;
                    if (s2 == 0)
                        s2 = v1;
                    if (s4 < 0)
                    {
                        // 47BC
                        *(int*)(sp + 64) = t0;
                        v0 = sub_3E34(s2);
                        *(int*)(sp + 48) = v0;
                        t0 = *(int*)(sp + 64);
                    }
                    else
                    {
                        *(int*)(sp + 64) = t0;
                        memchr(s2, 0, s4);
                        *(int*)(sp + 48) = s4;
                        t0 = *(int*)(sp + 64);
                        if (v0 != 0)
                        {
                            t1 = v0 - s2;
                            if (s4 < t1)
                                *(int*)(sp + 48) = s4;
                            else
                                *(int*)(sp + 48) = t1;
                        }
                    }
                    // 47B4
                    *(int*)(sp + 56) = 0;
                    goto4324 = 1;
                    break;
                default:
                    // 47D4
                    *(int*)(sp + 64) = t0;
                    a1 = (int)(char)a1;
                    func(s5, a1);
                    *(int*)(sp + 32)++;
                    // 408C
                    t0 = *(int*)(sp + 64);
                    goto4090 = 1;
                    break;
                }
                if (goto45E4)
                {
                    // 45E4
                    if (v0 == 0) {
                        v0 = *(int*)(t0 + 0);
                        goto4608 = 1;
                    }
                    else
                    {
                        v0 = *(int*)(sp + 36);
                        a1 = t0 + 7;
                        a1 &= 0xFFFFFFF8;
                        t0 = a1 + 8;
                        // 45FC
                        v1 = (v0 != 0);
                        goto4600 = 1;
                    }
                }
                if (goto4608)
                {
                    // 4608
                    t0 += 4;
                    *(int*)(sp + 36) = v0;
                    // 45FC
                    v1 = (v0 != 0);
                    goto4600 = 1;
                }
                if (goto4600)
                {
                    // 4600
                    *(int*)(sp + 56) = 0;
                    goto4504 = 1;
                }
                if (goto4504)
                {
                    // 4504
                    t3 = *(int*)(sp + 44);
                    t4 = (s4 != 0);
                    a1 = s4 < 0;
                    *(int*)(sp + 40) = s4;
                    t3 &= 0xFFFFFFDF;
                    t2 = v1 | t4;
                    s4 = *(int*)(sp + 44);
                    a2 = sp + 21;
                    if (a1 == 0)
                        s4 = t3;
                    *(int*)(sp + 44) = s4;
                    if (t2 != 0)
                    {
                        // 453C
                        do
                        {
                            s7 = *(int*)(sp + 36);
                            v0 = *(int*)(sp + 60);
                            s2--;
                            t7 = s7 % a2;
                            t6 = v0 + t7;
                            a0 = *(char*)t6;
                            t5 = s7 / a2;
                            *(int*)(sp + 36) = t5;
                            *(char*)s2 = a0;
                        } while (t5 != 0);
                        t9 = *(int*)(sp + 44);
                        fp = a2 ^ 8;
                        t8 = (fp == 0);
                        s0 = (t9 >> 3) & 1;
                        s1 = hexCharsD2;
                        a2 = s0 & t8;
                        *(int*)(sp + 60) = s1;
                        if (a2 != 0)
                        {
                            a3 = (int)(char)a0;
                            v1 = 48;
                            if (a3 != v1) {
                                s2--;
                                *(char*)s2 = v1;
                            }
                        }
                    }
                    goto4318 = 1;
                }
                if (goto4318)
                {
                    // 4318
                    s7 = sp - s2;
                    // 431C
                    *(int*)(sp + 48) = s7 + 21;
                    goto4324 = 1;
                }
                if (goto4324)
                {
                    // 4324
                    t2 = *(int*)(sp + 48);
                    v0 = *(int*)(sp + 44);
                    t1 = *(int*)(sp + 56);
                    a1 = *(int*)(sp + 52);
                    fp = t2 + 1;
                    s1 = t2;
                    if (t1 != 0)
                        s1 = fp;
                    s0 = *(int*)(sp + 40);
                    fp = v0 & 0x30;
                    s7 = v0 & 0x40;
                    t8 = s1 + 2;
                    t9 = (fp == 0);
                    a3 = (a1 != 0);
                    if (s7 != 0)
                        s1 = t8;
                    a2 = t9 & a3;
                    s4 = max(s1, s0);
                    if (a2 != 0 || s4 < a1)
                    {
                        s0 = a1 - s4;
                        // 4378
                        do
                        {
                            *(int*)(sp + 64) = t0;
                            s0--;
                            func(s5, 32);
                            t0 = *(int*)(sp + 64);
                        } while (s0 != 0);
                    }
                    // (4394)
                    v1 = *(int*)(sp + 56);
                    // 4398
                    if (v1 != 0)
                    {
                        *(int*)(sp + 64) = t0;
                        // 44BC
                        func(s5, v1);
                        t0 = *(int*)(sp + 64);
                    }
                    // 43A0
                    if (s7 != 0)
                    {
                        *(int*)(sp + 64) = t0;
                        // 449C
                        func(s5, 48);
                        func(s5, *(char*)s3);
                        t0 = *(int*)(sp + 64);
                    }
                    // 43A8
                    a0 = *(int*)(sp + 52);
                    if (fp == ' ')
                    {
                        // 446C
                        a1 = s4 < a0;
                        s0 = a0 - s4;
                        if (a1 != 0)
                        {
                            // 4478
                            do
                            {
                                *(int*)(sp + 64) = t0;
                                s0--;
                                func(s5, 48);
                                t0 = *(int*)(sp + 64);
                            } while (s0 != 0);
                        }
                    }
                    // (43B4)
                    a1 = *(int*)(sp + 40);
                    // 43B8
                    s0 = s1;
                    s1 = s1 < a1;
                    s0 = a1 - s0;
                    if (s1 != 0)
                    {
                        // 43C8
                        do
                        {
                            *(int*)(sp + 64) = t0;
                            s0--;
                            func(s5, 48);
                            t0 = *(int*)(sp + 64);
                        } while (s0 != 0);
                    }
                    // 43E4
                    t4 = *(int*)(sp + 48);
                    s0 = t4 - 1;
                    // 43F4
                    while (s0 >= 0)
                    {
                        *(int*)(sp + 64) = t0;
                        s0--;
                        func(s5, *(char*)s2);
                        s2++;
                        t0 = *(int*)(sp + 64);
                    }
                    t5 = *(int*)(sp + 44);
                    // 4418
                    s2 = t5 & 0x10;
                    s7 = *(int*)(sp + 52);
                    if (s2 != 0)
                    {
                        a0 = *(int*)(sp + 52);
                        v1 = s4 < a0;
                        if (v1 != 0)
                        {
                            s0 = a0 - s4;
                            // 4438
                            do
                            {
                                *(int*)(sp + 64) = t0;
                                s0--;
                                func(s5, 32);
                                t0 = *(int*)(sp + 64);
                            } while (s0 != 0);
                            s7 = *(int*)(sp + 52);
                        }
                    }
                    // (4458)
                    t6 = *(int*)(sp + 32);
                    // 445C
                    t7 = max(s4, s7);
                    s4 = t6 + t7;
                    *(int*)(sp + 32) = s4;
                    goto4090 = 1;
                }
                if (goto4090)
                    break;
                // 40A8
                s3++;
            }
        }
        else
        {
            *(int*)(sp + 64) = t0;
            func(s5, a1);
            a0 = *(int*)(sp + 32);
            v0 = a0 + 1;
            *(int*)(sp + 32) = v0;
            // 408C
            t0 = *(int*)(sp + 64);
        }
        // 4090
        s3++;
    }
}

// 4840
int snprintf(int arg0, int arg1, int arg2, int arg3, int arg4, int arg5, int arg6, int arg7)
{
    int sp[3];
    int sp2[5];
    sp2[0] = arg3;
    sp2[1] = arg4;
    sp2[2] = arg5;
    sp2[3] = arg6;
    sp2[4] = arg7;
    sp[0] = arg1 - 1;
    sp[1] = 0;
    sp[2] = arg0;
    return sub_3FF8(sub_48F0, sp, arg2, sp2);
}

void sub_48F0(int arg0, int arg1)
{
    if (arg1 >= 0x100 || *(int*)(a0 + 4) >= *(int*)(a0 + 0)) {
        // 4930
        *(char*)(*(int*)(a0 + 8)) = '\0';
        return;
    }
    *(char*)(*(int*)(a0 + 8)) = a1;
    *(int*)(a0 + 8)++;
    *(int*)(a0 + 4)++;
}

