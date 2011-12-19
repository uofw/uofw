/* Copyright (C) 2011 The uOFW team
   See the file COPYING for copying permission.
*/

#include "reboot.h"

typedef struct {
    char u0, u1, u2, u3;
    short u4;
    short u6;
    int u8;
} UnkStruct;

UnkStruct unkTab[] = { // 0x88612798
    { 0x00, 0x02, 0x10, 0x00, 0x0400, 0x0000, 0x01037398 },
    { 0x00, 0x02, 0x20, 0x00, 0x0400, 0x0000, 0x01037598 },
    { 0x00, 0x02, 0x20, 0x00, 0x0800, 0x0000, 0x01037698 },
    { 0x00, 0x02, 0x20, 0x00, 0x1000, 0x0000, 0x01037998 },
    { 0x00, 0x02, 0x20, 0x00, 0x2000, 0x0000, 0x0203E6EC },
    { 0x00, 0x02, 0x10, 0x00, 0x0400, 0x0000, 0x020373EC },
    { 0x00, 0x02, 0x20, 0x00, 0x0400, 0x0000, 0x020375EC },
    { 0x00, 0x02, 0x20, 0x00, 0x0800, 0x0000, 0x020376EC },
    { 0x00, 0x02, 0x20, 0x00, 0x1000, 0x0000, 0x020379EC },
    { 0x00, 0x02, 0x20, 0x00, 0x2000, 0x0000, 0x020371EC },
    { 0x00, 0x02, 0x20, 0x00, 0x4000, 0x0000, 0x0203DCEC },
    { 0x00, 0x02, 0x20, 0x00, 0x8000, 0x0000, 0x020139EC },
    { 0x00, 0x02, 0x10, 0x00, 0x0400, 0x0000, 0x020133EC },
    { 0x00, 0x02, 0x20, 0x00, 0x0400, 0x0000, 0x020135EC },
    { 0x00, 0x02, 0x20, 0x00, 0x0800, 0x0000, 0x020136EC },
    { 0x00, 0x02, 0x20, 0x00, 0x1000, 0x0000, 0x020178EC },
    { 0x00, 0x02, 0x20, 0x00, 0x2000, 0x0000, 0x02013520 },
    { 0x00, 0x02, 0x20, 0x00, 0x8000, 0x0000, 0x02013620 },
    { 0x00, 0x02, 0x20, 0x00, 0x1000, 0x0000, 0x02013920 },
    { 0x00, 0x02, 0x20, 0x00, 0x2000, 0x0000, 0x020135AD },
    { 0x00, 0x02, 0x20, 0x00, 0x8000, 0x0000, 0x020136AD },
    { 0x00, 0x02, 0x20, 0x00, 0x1000, 0x0000, 0x020139AD },
    { 0x00, 0x02, 0x20, 0x00, 0x2000, 0x0000, 0x00000000 }
};

int unkVar = 1; // 0x886128EC
int unkVar28F0 = 1; // 0x886128F0
char unkTab2[] = {0x00, 0x00, 0x00, 0x00, 0x01, 0x01, 0x07, 0xBC}; // 0x886128FC

typedef struct
{
    int u0, int u4, int u8, char u12, char u13, char u14, char u15;
} UnkStruct96D0;
UnkStruct96D0 unkStruct96D0;

int sub_10184()
{
    char sp[2];
    v0 = sub_F234(sp, 2);
    if (v0 < 0)
        return v0;
    unkStruct96D0.u8 = 0;
    unkStruct96D0.u13 = sp[1];
    a3 = 0;
    unkStruct96D0.u0 = 0;
    a1 = 0;
    unkStruct96D0.u4 = 0;
    unkStruct96D0.u12 = sp[0];
    // 101E0
    for (;;)
    {
        if (unkTab[a1].u0 != sp[0] || unkTab[a1].u1 != sp[1]) // 102A0
        {
            a1++;
            // 101F0
            if (a1 < 23)
                continue;
            if (a3 == 0) // ?
                return 0x80000004;
            break;
        }
    }
    // 1020C
    UnkStruct *unk = &unkTab[a1];
    unkStruct96D0.u4 = unk->u6;
    unkStruct96D0.u8 = unk->u8;
    unkStruct96D0.u14 = unk->u3;
    unkStruct96D0.u15 = unk->u2;
    unkStruct96D0.u0 = unk->u4;
    if (sub_113F0() <= 0x4FFFFF)
        unkStruct96D0.u14 = 1;
    // 10268
    if (unkStruct96D0.u0 != 512)
        return 0x80000003;
    if (unkStruct96D0.u4 != 32)
        return 0x80000003;
    return 0;
}

int sub_103C8(int arg0, int arg1, int arg2)
{
    int i;
    int ret;
    // 103FC
    for (i = 0; i < 4; i++)
    {
        ret = sub_10650(arg0);
        if (ret < 0)
            return ret;
        ret = sub_F930(arg0, arg1, arg2, unkstruct96D0.u4);
        // 10430
        if (ret < 0)
            return v0;
        ret = sub_10500(arg0, arg1, arg2);
        if (ret >= 0)
            return 0;
    }
    return ret;
}

int sub_1047C(int arg0, int arg1, int arg2)
{
    int i;
    int ret;
    for (i = 0; i < 4; i++)
        if ((ret = sub_F8EC(arg0, arg1, arg2, unkStruct96D0.u4)) >= 0)
            return 0;
    return ret;
}

char unkString90A8[512]; // 0x886390A8
char unkString92A8[8]; // 0x886392A8
int sub_10500(int arg0, int arg1, int arg2)
{
    int i = 0;
    int v = 0;
    if (unkStruct96D0.u4 <= 0)
        return 0;

    // 10558
    do
    {
        int failCount = 0;
        int ret;

        // 1056C
        do
        {
            ret = sub_F8EC(arg0 + i, unkString90A8, unkString92A8, 1);
            if (ret < 0)
            {
                // 10638
                failCount++;
                if (failCount >= 4)
                    return ret;
            }

            // 10588
            if (arg1 != 0)
            {
                a2 = unkStruct96D0.u0;;
                if (strncmp(unkString90A8, arg1 + a2 * i, a2) != 0)
                    return 0x80230006;
            }

            // 105B0
            if (arg2 != 0 && strncmp(unkString92A8, arg2 + v, 8) != 0)
                return 0x80230006;

            // 105D4
        } while (ret < 0);
        i++;
        v += 12;
    } while (i < unkStruct96D0.u4);
    return 0;
}

int sub_10650(int arg)
{
    int i;
    int v = unkStruct96D0.u4;
    int ret;
    // 10680
    if (arg % v != 0)
        return 0x80230008;

    // 1068C
    for (i = 0; i < 4; i++)
        if ((ret = sub_F640(arg)) >= 0)
            return 0;
    return ret;
}

int sub_106C4(int arg)
{
    // 106F4
    if (arg % unkStruct96D0.u4 != 0)
        return 0x80230008;
    // 10700
    do
    {
        s0++;
        v0 = sub_F998(arg, 0, sp, 1);
        if (v0 >= 0)
        {
            // 10740
            return (*(char*)(sp + 5) ^ 0xFF != 0);
        }
    } while (s0 < 4);
    return v0;
}

char unkBuffer92B4[512]; // 0x886392B4
char unkBuffer94B4[12]; // 0x886394B4
int sub_10750(int arg)
{
    a2 = unkStruct96D0.u4;
    // 10794
    if (arg % a2 != 0)
        return 0x80230008;
    memset(unkBuffer92B4, 0, 0x200);
    memset(unkBuffer94B4, 0xFF, 12);
    unkBuffer94B4[1] = 0xF0;

    int i, j;
    for (i = 0, j = 0; i < unkStruct96D0.u4; i++)
    {
        // 107F0
        for (; j < 4; j++)
            if (sub_F930(arg + i, unkBuffer92B4, unkBuffer94B4, 1) >= 0)
                break;
        // 1081C
    }
    return 0;
}

char unkBuffer94C0[512]; // 0x886394C0
char unkBuffer96C0[16]; // 0x886396C0
int sub_109DC(int arg)
{
    if (unkStruct96D0.u14 == 0)
        return 0;
    // 10A38
    int numOk = 0;
    char cond = 0;
    for (;;)
    {
        char retry = 0;
        int i;
        // 10A3C
        for (i = 0; i < 4; i++)
        {
            v0 = sub_F998(arg + numOk, unkBuffer94C0, unkBuffer96C0, 1);
            if (v0 >= 0)
            {
                if (unkBuffer96C0[5] != 0xFF)
                    cond = 1;
                if (cond != 0)
                    return 1;
                numOk++;
                if (numOk < unkStruct96D0.u14) {
                    retry = 1;
                    break;
                }
                return 0;
            }
        }
        if (!retry)
            return v0;
    }
}

int sub_10D1C()
{
    return unkStruct96D0.u0;
}

int sub_10D28()
{
    return unkStruct96D0.u4;
}

int sub_10D34()
{
    return unkStruct96D0.u8;
}

int sub_10DA0(int arg)
{
    t1 = *(char*)(arg + 0);
    v0 = *(char*)(arg + 1);
    s0 = *(char*)(arg + 2);
    t5 = *(char*)(arg + 3);
    t7 = *(char*)(arg + 4);
    t3 = t1 ^ v0;
    s2 = *(char*)(arg + 5);
    a1 = s0 ^ t3;
    s3 = *(char*)(arg + 6);
    t4 = t5 ^ a1;
    s1 = *(char*)(arg + 7);
    a0 = t7 ^ t4;
    t6 = t7 ^ s2;
    v1 = s2 ^ a0;
    t0 = s0 ^ t5;
    t2 = s3 ^ t6;
    a2 = s3 ^ v1;
    t9 = s1 ^ t2;
    t8 = s1 ^ a2;
    t6 = s3 ^ t0;
    a1 = t8 & 0xFF;
    t2 = v0 ^ t5;
    t8 = t4 & 0xFF;
    t0 = t7 ^ t3;
    v1 = s3 ^ t6;
    t4 = t9 & 0xFF;
    a2 = v1 & 0xFF;
    t6 = t1 ^ s0;
    v1 = 0x6996;
    t1 = s2 ^ t2;
    s0 = s2 ^ t0;
    v0 = t8 >> 4;
    t2 = a1 & 0xCC;
    t3 = t8 & 0xF;
    s2 = t4 & 0xF;
    t9 = t4 >> 4;
    t4 = v1 >> t9;
    a0 = v1 >> s2;
    t9 = s0 & 0xFF;
    t8 = a1 >> 4;
    a3 = v1 >> t3;
    t5 = a1 & 0xC;
    t3 = v1 >> v0;
    s0 = t2 >> 4;
    v0 = a1 & 0xF;
    t2 = t7 ^ t6;
    s2 = a2 & 0xF;
    t6 = s1 ^ t1;
    s1 = a2 >> 4;
    t1 = t6 & 0xFF;
    s0 = v1 >> s0;
    t6 = v1 >> t5;
    a2 = v1 >> s2;
    t5 = v1 >> v0;
    s2 = v1 >> s1;
    v0 = v1 >> t8;
    s1 = a1 & 0xAA;
    t8 = s3 ^ t2;
    a3 = a3 ^ t3;
    s3 = t9 >> 4;
    t3 = a0 ^ t4;
    t7 = (a1 >> 4) & 3;
    t4 = a3 & 3;
    t0 = t9 & 0xF;
    t2 = t8 ^ 0xFF;
    t9 = t6 ^ s0;
    t7 = v1 >> t7;
    s3 = v1 >> s3;
    a0 = t3 & 1;
    a2 = a2 ^ s2;
    t8 = s1 >> 4;
    t4 = v1 >> t4;
    s0 = t5 & 1;
    v0 = v0 & 1;
    s2 = t1 >> 4;
    t0 = v1 >> t0;
    t3 = a1 & 0xA;
    a3 = a3 & 1;
    t1 = t1 & 0xF;
    s1 = v1 >> t8;
    t4 = t4 ^ t7;
    t0 = t0 ^ s3;
    t8 = t9 & 1;
    t5 = s0 << 2;
    t1 = v1 >> t1;
    s2 = v1 >> s2;
    t7 = a1 & 0x55;
    t3 = v1 >> t3;
    t9 = v0 << 8;
    s0 = t2 >> 4;
    s3 = a0 << 11;
    a3 = a3 << 5;
    t2 = t2 & 0xF;
    a2 = a2 & 1;
    v0 = t9 | t5;
    a0 = s3 | a3;
    t9 = t1 ^ s2;
    t3 = t3 ^ s1;
    s3 = t7 >> 4;
    s1 = t4 & 1;
    s2 = t8 << 7;
    s0 = v1 >> s0;
    t8 = v1 >> t2;
    t5 = t0 & 1;
    a2 = a2 << 10;
    a1 = a1 & 5;
    a3 = v0 | s2;
    t6 = v1 >> a1;
    s2 = t8 ^ s0;
    s0 = v1 >> s3;
    t8 = a0 | a2;
    s3 = t3 & 1;
    a2 = s1 << 1;
    s1 = t9 & 1;
    t9 = t5 << 4;
    t7 = a3 | a2;
    t5 = t6 ^ s0;
    v1 = s2 & 1;
    s0 = s3 << 6;
    t6 = s1 << 9;
    t4 = t8 | t9;
    t0 = t7 | s0;
    a3 = v1 << 3;
    a2 = t4 | t6;
    t1 = t5 & 1;
    v1 = t0 | t1;
    a1 = a2 | a3;
    v0 = v1 | a1;
    return v0;
}

int sub_88610FE8(int arg0, int arg1)
{
    return sub_11004(arg0, arg1, 1);
}

int sub_11004(int arg0, int arg1, int arg2)
{
    char c1 = *(char*)(arg0 + 0);
    char c2 = *(char*)(arg0 + 1);
    char c3 = *(char*)(arg0 + 2);
    char c4 = *(char*)(arg0 + 3);
    char c5 = *(char*)(arg0 + 4);
    char c6 = *(char*)(arg0 + 5);
    char c7 = *(char*)(arg0 + 6);
    char c8 = *(char*)(arg0 + 7);

    t6 = c4 ^ (c3 ^ (c1 ^ c2));
    a0 = (c8 ^ (c7 ^ (c6 ^ (c5 ^ t6)))) & 0xFF;
    s4 = t6 & 0xFF;
    t6 = (c8 ^ (c7 ^ (c5 ^ c6))) & 0xFF;
    v0 = 0x6996;
    t0 = (c8 ^ (c7 ^ (c3 ^ c4))) & 0xFF;
    s4 = t6 & 0xF;

    v1 = v0 >> (s4 & 0x1F);
    t3 = (c6 ^ (c5 ^ c7)) & 0xFF;
    t4 = a0 >> 4;
    t1 = v0 >> (s4 & 0xF);
    s2 = (a0 & 0xCC) >> 4;
    t5 = v0 >> ((s4 >> 4) & 0x1F);
    s1 = c5 ^ (c1 ^ c3);
    a3 = a0 & 0xF;
    s0 = c8 ^ (c6 ^ (c2 ^ c4));
    t7 = a0 & 0xC;
    s3 = t0 >> 4;
    s4 = t0 & 0xF;
    t2 = s0 & 0xFF;
    s2 = v0 >> (s2 & 0x1F);
    s0 = v0 >> (t7 & 0x1F);
    t0 = v0 >> (s4 & 0x1F);
    t7 = v0 >> (a3 & 0x1F);
    s4 = v0 >> (s3 & 0x1F);
    a3 = v0 >> (t4 & 0x1F);
    t1 = t1 ^ t5;
    t4 = s5 ^ s1;
    t5 = v1 ^ (v0 >> ((t6 >> 4) & 0x1F));
    s5 = t3 >> 4;
    s3 = a0 & 0xAA;
    t6 = a0 & 3;
    s1 = (a0 >> 4) & 3;
    t3 = t3 & 0xF;
    s1 = v0 >> (s1 & 0x1F);
    s5 = v0 >> (s5 & 0x1F);
    s0 = s0 ^ s2;
    t0 = t0 ^ s4;
    t4 = t4 & 0xFF;
    t6 = v0 >> (t6 & 0x1F);
    s2 = t7 & 1;
    s4 = t2 >> 4;
    t3 = v0 >> (t3 & 0x1F);
    v1 = t5 & 1;
    s3 = s3 >> 4;
    t5 = a0 & 0xA;
    a3 = a3 & 1;
    t1 = t1 & 1;
    t2 = t2 & 0xF;
    t6 = t6 ^ s1;
    t3 = t3 ^ s5;
    s3 = v0 >> (s3 & 0x1F);
    s5 = v0 >> (t5 & 0x1F);
    t7 = s2 << 2;
    s4 = v0 >> (s4 & 0x1F);
    a3 = a3 << 8;
    s2 = t4 >> 4;
    t2 = v0 >> (t2 & 0x1F);
    t1 = t1 << 5;
    s1 = a0 & 0x55;
    s0 = s0 & 1;
    t0 = t0 & 1;
    v1 = v1 << 11;
    t4 = t4 & 0xF;
    t5 = s5 ^ s3;
    t2 = t2 ^ s4;
    s5 = a3 | t7;
    s4 = t6 & 1;
    s2 = v0 >> (s2 & 0x1F);
    t7 = t3 & 1;
    v1 = v1 | t1;
    s1 = s1 >> 4;
    t1 = v0 >> (t4 & 0x1F);
    s0 = s0 << 7;
    t0 = t0 << 10;
    a0 = a0 & 0x5;
    s3 = s5 | s0;
    s5 = t1 ^ s2;
    s0 = s4 << 1;
    s2 = v0 >> (a0 & 0x1F);
    t1 = v1 | t0;
    v0 = v0 >> (s1 & 0x1F);
    t0 = t5 & 1;
    s1 = t7 << 4;
    s4 = t2 & 1;
    t7 = s2 ^ v0;
    t6 = s5 & 1;
    s2 = s3 | s0;
    s0 = t1 | s1;
    s3 = t0 << 6;
    s1 = s4 << 9;
    v0 = s0 | s1;
    t3 = s2 | s3;
    t2 = t7 & 1;
    t1 = t6 << 3;
    a0 = v0 | t1;
    t0 = t3 | t2;
    v0 = t0 | a0;
    a3 = v0 ^ arg1;
    if (a3 == 0)
        return 0;
    s5 = a3 >> 6;
    a1 = s5 ^ a3;
    a3 = a1 & 0x3F;
    t1 = 63;
    a1 = 0;
    if (a3 == t1)
    {
        // 11294
        if (arg2 != 0) {
            a2 = c8 ^ 0x80;
            *(char*)(arg0 + 7) = a2 & 0xFF;
        }
        return -1;
    }

    // 11248
    do
    {
        s1 = a3 >> (a0 & 0x1F);
        a0++;
        s0 = s1 & 1;
        a1 += s0;
    } while (a0 < 6);

    if (a1 ^ 1 != 0)
        return -3;
    else
        return -2;
}

int unkVar4 = -1; // 0x88612AAC

int sub_113F0()
{
    int var = unkVar4;
    if (var == -1)
    {
        int ret;
        var = *(int*)(0xBC100040) & 0xFF000000;
        if (var != 0)
            ret = var >> 8;
        else
            ret = 0x10;
        unkVar4 = ret;
        return ret;
    }
    return var;
}

u64 sub_11548()
{
    return (*(int*)(0xBC100090) << 32) | *(int*)(0xBD100094);
}

int sub_17E8(unsigned int arg)
{
    return (arg & 0xFFFF0000) >= 0xF00A0000;
}

int sub_17FC(unsigned int arg)
{
    return (arg & 0xFFFF0000) >= 0xF0940000;
}

int sub_37F4(int arg)
{
    unkVar28F0 = arg;
    return arg;
}

int unkVar1; // 0x88630C18
int unkVar2; // 0x88630C1C
int unkVar3; // 0x88630C20

int sub_3804(int arg0, int arg1, int arg2)
{
    unkVar1 = arg0;
    unkVar2 = arg1;
    unkVar3 = arg2;
    return 0;
}

int sub_3824(int arg)
{
    if (arg >= 64)
        return 0x80020001;
    else if (arg >= 32)
        return (unkVar2 >> (arg - 32)) & 1;
    else
        return (unkVar1 >> arg) & 1;
}

int sub_3874()
{
    return unkVar1;
}

int sub_3880()
{
    return unkVar2;
}

int sub_388C(int arg)
{
    return sub_397C(arg, 1);
}

int sub_397C(int arg0, int arg1)
{
    int oldK1 = pspKernelGetK1();
    pspKernelSetK1(oldK1 << 11);
    if (a0 >= 64) {
        pspKernelSetK1(oldK1);
        return 0x80020001;
    }
    if (pspKernelGetK1() >= 0)
        return func_39CC();
    var = a0 - 10;
    if (var >= 42) {
        pspKernelSetK1(oldK1);
        return 0x80020001;
    }
    switch (var)
    {
    case 0:
    case 1:
    case 2:
    case 7:
    case 8:
    case 9:
    case 10:
    case 14:
    case 39:
    case 41:
        pspKernelSetK1(oldK1);
        return 0x80020001;
    default:
        {
        int addr;
        int var;
        // 39CC
        if (arg0 < 32) {
            addr = &unkVar1;
            var = *addr;
        }
        else
        {
            addr = &unkVar2;
            arg0 -= 32;
            var = *addr;
        }

        // 39EC
        int t0 = (var >> arg0) & 1;
        if (arg1 == 0)
        {
            // 3A14
            var &= ~(1 << arg0);
        }
        else
            var |= (1 << arg0);

        *addr = var;
        pspKernelSetK1(oldK1);
        return t0;
        }
    }
}

int sub_5220(int arg0, int arg1)
{
    /* SP size: 416 */
    s0 = sp + 0x3F;
    s0 &= 0xFFFFFFC0; // ???
    if (*(int*)(arg0 + 8) < 353)
        return -1;
    // 527C
    memcpy(s0, *(int*)(arg0 + 4), 352);
    if (sub_5F48(s0, 352) < 0)
        return -1;
    // 52A8
    if (strncmp(arg1 + 16, s0 + 320, 16) != 0)
        return -1;
    return 0;
}

u64 sub_7470(int arg0, int arg1, int arg2, int arg3, int arg4)
{
    v1 = arg2 | arg3;
    t6 = arg4;
    t2 = arg0;
    t3 = arg1;
    t0 = arg0 | arg2;
    t4 = 0;
    t5 = 0;
    t1 = arg1 | arg3;
    if (v1 == 0)
        return 0;
    t4 = 0;
    // 74A0
    while (t0 & 1 == 0)
    {
        a1 = t1 << 31;
        v0 = t0 >> 1;
        t0 = v0 | a1;
        t1 >>= 1;
        t4++;
    }
    // 74BC
    t0 = t1;
    t1 = t0;
    t0 = 0;
    if (t1 != 0)
    {
        // 7598
        for (;;)
        {
            v1 = a3 >> 31;
            v0 = 0;
            t9 = a3 < t3;
            a0 = v1 | v0;
            v1 = a2 < t2;
            if (t9 == 0)
            {
                if (t3 != a3)
                    break;
                if (v1 == 0)
                    break;
            }
            // 75C0
            t7 = a2 >> 31;
            if (a0 != 0)
                break;
            t4 = a3 << 1;
            a3 = t4 | t7;
            a2 <<= 1;
            t0++;
        }
        // 75DC
        t9 = 0;
        t8 = 1;
        a1 = t0 << 26;
        if (a1 >= 0)
        {
            // 75FC
            t5 = t9 << (t0 & 0x1F);
            if (a1 != 0)
            {
                a1 = -t0;
                a1 = t8 >> (a1 & 0x1F);
                t5 |= a1;
            }
            // 7610
            t4 = t8 << (t0 & 0x1F);
        }
        else {
            t5 = t8 << (t0 & 0x1F);
            t4 = 0;
        }
        // 7614
        a1 = t4 | t5;
        t0 = t4;
        t1 = t5;
        t4 = 0;
        t5 = 0;
        // 762C
        while (a1 != 0)
        {
            t7 = t1 << 31;
            if (t3 >= a3 && (a3 != t3 || t2 >= a2)) // 7690
            {
                t9 = t2 < a2;
                // 7644
                t7 = t3 - a3;
                t2 -= a2;
                t3 = t7 - t9;
                t4 |= t0;
                t5 |= t1;
                t7 = t1 << 31;
            }
            // 765C
            a0 = t0 >> 1;
            t0 = a0 | t7;
            t1 >>= 1;
            t8 = a3 << 31;
            v1 = a2 >> 1;
            a1 = t0 | t1;
            a2 = v1 | t8;
            a3 >>= 1;
        }
        // 7680
        *(int*)(t6 + 0) = t2;
        *(int*)(t6 + 4) = t3;
        return (t5 << 32) | t4;
    }
    v0 = t4 << 26;
    if (v0 >= 0)
    {
        // 74E8
        t0 = a2 >> (t4 & 0x1F);
        if (v0 != 0)
        {
            v0 = -t4;
            v0 = a3 << (v0 & 0x1F);
            t0 |= v0;
        }
        // 74FC
        t1 = a3 >> (t4 & 0x1F);
    }
    else {
        t0 = a3 >> (t4 & 0x1F);
        t1 = 0;
    }

    // 7500
    a2 = t4 << 26;
    if (a2 >= 0)
    {
        // 7518
        a0 = t2 >> (t4 & 0x1F);
        if (a2 != 0)
        {
            a2 = -t4;
            a2 = t3 << (a2 & 0x1F);
            a0 |= a2;
        }
        // 752C
        a2 = t3 >> (t4 & 0x1F);
    }
    else {
        a0 = t3 >> (t4 & 0x1F);
        a1 = 0;
    }
    // 7530
    v1 = 0;
    t3 = 0;
    t5 = t3;
    // 7548
    v0 = a0 % t0;
    a2 = a0 / t0;
    t2 = t4 << 26;
    if (t2 >= 0)
    {
        // 7568
        t9 = v1 << (t4 & 0x1F);
        if (t2 != 0)
        {
            t2 = -t4;
            t2 = v0 << (t2 & 0x1F);
            t9 |= t2;
        }
        // 757C
        t8 = v0 << (t4 & 0x1F);
    }
    else {
        t9 = v0 << (t4 & 0x1F);
        t8 = 0;
    }
    // 7580
    *(int*)(t6 + 0) = t8;
    *(int*)(t6 + 4) = t9;
    return (t5 << 32) | a2;
}

int sub_76A0(int arg0, int arg1, int arg2, int arg3)
{
    v1 = arg2 | arg3;
    t2 = arg0;
    t3 = arg1;
    a0 = 0;
    a1 = 0;
    if (v1 == 0)
        return 0;
    a0 = 0;
    // 76C8
    while (a2 & 1 == 0)
    {
        t0 = a3 << 31;
        v0 = a2 >> 1;
        a2 = v0 | t0;
        a3 >>= 1;
        a0++;
    }
    // 76E4
    v0 = a0 << 26;
    if (v0 >= 0)
    {
        // 76FC
        t0 = t2 >> (a0 & 0x1F);
        if (v0 != 0)
        {
            v0 = -a0;
            v0 = t3 << (v0 & 0x1F);
            t0 |= v0;
        }
        // 7710
        t1 = t3 >> (a0 & 0x1F);
    }
    else {
        t0 = t3 >> (a0 & 0x1F);
        t1 = 0;
    }
    // 7714
    t5 = t1 | a3;
    t4 = t5;
    a0 = 0;
    t3 = t1;
    t1 = t4 | a0;
    t2 = t0;
    if (t1 == 0)
        return t2 / a2; // 773C

    // 7754
    t6 = t3;
    t0 = t2;
    t1 = t3;
    t4 = 0;
    if (t6 < 0)
    {
        // 7878
        t0 = 0;
        t1 = 0x80000000;
    }
    // 776C
    if (a3 < t1 || (t1 == a3 && a2 < t0)) // 7860
    {
        // 7774
        do
        {
            v1 = a3 << 1;
            // 7778
            a1 = a2 >> 31;
            a3 = v1 | a1;
            a2 <<= 1;
            t9 = a3 < t1;
            v1 = a2 < t0;
            t4++;
        } while (t9 != 0 || (t1 == a3 && v1 != 0))
    }
    // 77A4
    t7 = 0;
    t6 = a1;
    a1 = t4 << 26;
    if (a1 >= 0)
    {
        // 77C4
        t1 = t7 << (t4 & 0x1F);
        if (a1 != 0)
        {
            a1 = -t4;
            a1 = t6 << (a1 & 0x1F);
            t1 |= a1;
        }
        // 77D8
        t0 = t6 << (t4 & 0x1F);
    }
    else {
        t1 = t6 << (t4 & 0x1F);
        t0 = 0;
    }
    // 77DC
    v0 = t0 | t1;
    t4 = 0;
    t5 = 0;
    if (v0 == 0)
        return 0;

    // 77F0
    do
    {
        if (t3 >= a3 && (a3 != t3 || t2 >= a2)) // 7850
        {
            t8 = t2 < a2;
            // 7808
            t7 = t3 - a3;
            t2 -= a2;
            t3 = t7 - t8;
            t4 |= t0;
            t5 |= t1;
        }
        a0 = t1 << 31;
        // 7820
        t6 = t0 >> 1;
        t0 = t6 | a0;
        t1 >>= 1;
        a1 = a3 << 31;
        v1 = a2 >> 1;
        t9 = t0 | t1;
        a2 = v1 | a1;
        a3 >>= 1;
    } while (t9 != 0);
    return (t5 << 32) | t4;
}

u64 sub_7888(int arg0, int arg1, int arg2, int arg3)
{
    int sp[2];
    sub_7470(arg0, arg1, arg2, arg3, sp);
    return (sp[1] << 32) | sp[0];
}

char unkBuffer[256]; // unkBuffer

int sub_78AC(int arg0, int arg1, int arg2, int arg3, int arg4, int arg5, int arg6, int arg7)
{
    *(int*)(sp + 48) = arg0;
    *(int*)(sp + 52) = arg1;
    fp = arg4;
    s4 = arg2;
    s3 = arg3;
    s2 = -1;
    *(int*)(sp + 56) = arg5;
    *(int*)(sp + 60) = arg6;
    *(int*)(sp + 64) = arg7;
    v0 = sub_8DB8(arg2, arg4);
    s1 = v0;
    if (s1 == 0)
        return 0x80010018;
    a1 = *(int*)(s1 + 12);
    if (a1 == 0)
    {
        t4 = *(int*)(s4 + 64);
        // 7CD8
        v0 = t4 << 5;
    }
    else
    {
        s0 = 0;
        if (a1 | ~*(int*)(s4 + 40) < -10)
        {
            // 7CAC
            do
            {
                v0 = sub_8534(s4, a1);
                a1 = v0;
                s0++;
            } while ((v0 | ~*(int*)(s4 + 40)) < -10);
        }
        // 7940
        v0 = s0 * *(int*)(s4 + 76);
    }
    // 7948
    *(int*)(s1 + 16) = v0;
    v0 = *(int*)(s4 + 616);
    s5 = 229;
    char skip7B98 = 0;
    if (v0 != 1)
    {
        s6 = 15;
        s7 = -1;
        // 7964
        for (;;)
        {
            v0 = sub_8B74(s1, sp, 32, 1);
            s0 = v0;
            if (v0 < 0) {
                sub_8E54(s1);
                return s0;
            }
            t6 = v0 < 32;
            v0 = *(char*)sp;
            if (t6 != 0 || v0 == 0)
            {
                // (7B88)
                s2 = *(int*)(s1 + 12);
                // 7B8C
                *(int*)(s1 + 24) = 0;
                *(int*)(s1 + 28) = s2;
                *(int*)(s1 + 20) = 0;
                break;
            }
            if (v0 == s5)
                s2 = -1;
            else
            {
                v0 = *(char*)(sp + 11);
                if (v0 == s6)
                {
                    if (s2 == s7)
                    {
                        // 7B74
                        bzero(unkBuffer, 256);
                    }
                    // 7B5C
                    s2 = sub_8F9C(s4, unkBuffer, sp, s2);
                }
                else
                {
                    t7 = v0 & 8;
                    a2 = sp;
                    if (t7 == 0)
                    {
                        v0 = 0;
                        a1 = 1;
                        // 79B4
                        do
                        {
                            t2 = v0 >> 1;
                            t1 = v0 << 7;
                            a3 = *(char*)a2;
                            t0 = t1 | t2;
                            v1 = t0 & 0xFF;
                            t8 = v1 + a3;
                            s0 = a1 < 11;
                            v0 = t8 & 0xFF;
                            a2++;
                            a1++;
                        } while (s0 != 0);
                        if (s2 == v0)
                        {
                            // 79F0
                            sub_8EBC(s4, unkBuffer);
                            sub_90C8(s3, unkBuffer);
                            if (v0 != 0)
                            {
                                s3 = v0;
                                v1 = *(char*)s3;
                                skip7B98 = 1;
                                break;
                            }
                        }
                    }
                    s2 = -1;
                }
            }
            // 7964
        }
    }
    if (!skip7B98)
    {
        // 7B98
        s2 = sp + 32;
        v0 = sub_861C(s3, s2, 0, 1);
        a0 = *(char*)s3;
        a2 = (int)(char)a0;
        s0 = v0;
        if ((a2 != 0) && (a2 ^ 0x2F != 0))
        {
            // 7BD4
            do
            {
                s3++;
                a0 = *(char*)s3;
                t6 = (int)(char)a0;
            } while ((t6 != 0) && (t6 ^ 0x2F != 0));
        }
        // 7BF4
        t7 = 3;
        if (s0 == 0)
        {
            // 7CA0
            sub_8E54(s1);
            return 0x80010016;
        }
        if (s0 == t7)
        {
            s4 = *(int*)(sp + 56);
            // 7C88
            v1 = (int)(char)a0;
            if (s4 == 0) {
                sub_8E54(s1);
                return 0x80010002;
            }
            // 7C78
            *(int*)(*(int*)(sp + 56)) = (v1 == 0) ? fp : -1;
            sub_8E54(s1);
            return s0;
        }
        char goto7C64 = 0;
        // 7C04
        do
        {
            v0 = sub_8B74(s1, sp, 32, 1);
            s0 = v0;
            v0 = v0 < 32;
            if (s0 < 0) {
                sub_8E54(s1);
                return s0;
            }
            v1 = *(char*)sp;
            if (v0 != 0 || v1 == 0) {
                goto7C64 = 1;
                break;
            }
        } while (v1 == 229
              || *(char*)(sp + 11) & 8 != 0
              || sub_8B08(sp, s2) == 0);
        if (goto7C64)
        {
            // 7C64
            a0 = *(int*)(sp + 56);
            if (a0 == 0) {
                sub_8E54(s1);
                return 0x80010002;
            }
            v1 = *(char*)s3;
            *(int*)(*(int*)(sp + 56)) = (v1 == 0) ? fp : -1;
            sub_8E54(s1);
            return s0;
        }
        v1 = *(char*)s3;
    }
    // 7A20
    s0 = 0;
    if (v1 != 0 && (v1 != '/' || *(char*)(s3 + 1) != 0)) // 7ADC
    {
        t3 = *(char*)(sp + 11);
        // 7A38
        a2 = t3 & 0x10;
        if (a2 != 0)
        {
            // 7A80
            v0 = sub_8E54(s1);
            s1 = v0;
            t6 = *(int*)(sp + 26); // unaligned reading
            v0 = *(int*)(sp + 20); // unaligned reading
            a1 = (v0 << 16) | (t6 & 0xFFFF);
            // 7AB4
            s0 = sub_78AC(*(int*)(sp + 48), *(int*)(sp + 52), s4, s3 + 1, (a1 == 0) ? *(int*)(s4 + 84) : 1, *(int*)(sp + 56), *(int*)(sp + 60), *(int*)(sp + 64));
            sub_8E54(s1);
            return s0;
        }
        sub_8E54(s1);
        return s0;
    }
    // 7AE4
    memcpy(*(int*)(sp + 52), sp, 32);
    t2 = *(int*)(sp + 56);
    s5 = *(int*)(sp + 60);
    if (t2 != 0)
    {
        v0 = 0;
        if (fp != 0)
            v0 = *(int*)(s1 + 28);
        // 7B0C
        s2 = *(int*)(sp + 56);
        *(int*)sp = v0;
        s5 = *(int*)(sp + 60);
    }
    // 7B18
    s7 = *(int*)(sp + 64);
    if (s5 != 0)
    {
        if (fp != 0)
            v0 = *(int*)(s1 + 24);
        else
            v0 = *(int*)(s1 + 20);
        // 7B2C
        fp = *(int*)(sp + 60);
        s6 = v0 - 32;
        *(int*)fp = s6;
        s7 = *(int*)(sp + 64);
    }
    // 7B3C
    v1 = *(int*)(sp + 64);
    if (s7 == 0) {
        sub_8E54(s1);
        return s0;
    }
    v0 = *(int*)(s1 + 32);
    // 7B48
    *(int*)v1 = v0;
    sub_8E54(s1);
    return s0;
}

int sub_7CE0()
{
    int offset = 0;
    // 7D14
    int i;
    for (i = 0; i < fatUnitsNumber; i++)
    {
        int addr = unkFatAddr + offset;
        offset += 628;
        if (*(int*)addr & 1 != 0)
        {
            // 7D64
            sub_8434(addr);
        }
        // 7D30
    }
    // 7D3C
    sub_CB58();
    return 0;
}

typedef struct
{
    int u0, u4, u8, u12, u16, u20, u24, u28, u32, u36;
} FlashOpt;
FlashOpt flashOpt; // 0x88638D80
FlashOpt fatOpt; // 0x88638DA8

int sub_8124(int flashNum, int arg1, int arg2)
{
    char lflash[16], flash[16];
    if (unkFatMask & 2 != 0)
        return -1;
    v0 = sub_9A84(arg1, arg2); // init FAT options
    if (v0 != 0)
        return v0;
    // 81AC
    flashOpt.u0 = 1;
    flashOpt.u4 = 0;
    flashOpt.u8 = 0;
    flashOpt.u12 = 0;
    flashOpt.u16 = 0;
    flashOpt.u20 = 0;
    flashOpt.u24 = 0;
    flashOpt.u28 = 0;
    flashOpt.u32 = 0;
    flashOpt.u36 = 0;
    snprintf(lflash, 16, "lflash0:0,%d", flashNum);
    snprintf(flash,  16, "flashfat%d:", flashNum);
    v0 = sub_9508(&flashOpt, flash, lflash, 1, 0, 0);
    if (v0 != 0)
        return v0;
    unkFatMask |= 2;
    return 0;
}

int sub_822C(char *name)
{
    a2 = unkFatMask;
    if (a2 & 2 == 0)
        return -1;
    if (a2 & 4 != 0)
        return -1;
    fatOpt.u0 = 1;
    fatOpt.u4 = 0;
    fatOpt.u8 = 0;
    fatOpt.u12 = 0;
    fatOpt.u16 = 0;
    fatOpt.u20 = 0;
    fatOpt.u24 = 0;
    fatOpt.u28 = 0;
    fatOpt.u32 = 0;
    fatOpt.u36 = 0;
    v0 = sub_9158(&fatOpt, name, 1, 0);
    if (v0 != 0)
        return v0;
    unkFatMask |= 4;
    return 0;
}

int sub_82C8(int arg0, int arg1)
{
    return sub_CBDC(arg0, arg1);
}

int sub_82E4()
{
    if (unkFatMask & 2 == 0)
        return -1;
    v0 = sub_99B8(&flashOpt, /*0*/);
    if (v0 != 0)
        return v0;
    unkFatMask &= 0xFFFFFFFD;
    return 0;
}

int sub_8344()
{
    if (unkFatMask & 4 == 0)
        return -1;
    v0 = sub_93AC(&fatOpt);
    if (v0 != 0)
        return v0;
    unkFatMask &= 0xFFFFFFFB;
    return 0;
}

int sub_83A0(int arg0, int arg1)
{
    if (unkFatMask & 4 == 0)
        return -1;
    return sub_8B74(fatOpt.u4, arg0, arg1, 0);
}

int sub_83E0(int arg0, int arg1, int arg2)
{
    if (unkFatMask & 4 == 0)
        return (0xFFFFFFFF << 32) | 0xFFFFFFFF;
    v0 = sub_9450(&fatOpt, arg0, arg2);
    return ((v0 >> 31) << 32) | v0;
}

int sub_8434(int arg)
{
    a2 = *(int*)(arg + 0); // Options?
    if (a2 & 4 == 0)
        return 0;
    t0 = *(int*)(arg + 100);
    if (t0 >= 0)
    {
        if (a2 & 8 == 0)
        {
            // 84FC
            v0 = sub_9C6C(arg, (t0 + *(int*)(s1 + 52) * *(int*)(s1 + 80)) << (*(int*)(s1 + 36) & 0x1F), s1 + 104, 512);
            if (v0 < 0)
                return v0;
        }
        else
        {
            s0 = t0 + *(int*)(arg + 52) * (*(int*)(s1 + 56) - 1);
            s2 = arg + 104;
            // 849C
            while (s0 >= *(int*)(arg + 44))
            {
                v0 = sub_9C6C(arg, s0 << (*(int*)(s1 + 36) & 0x1F), s2, 512);
                if (v0 < 0)
                    return v0;
                s0 -= *(int*)(arg + 52);
            }
        }
        // 84D0
        a2 = *(int*)arg;
    }
    // 84D4
    *(int*)arg = a2 & 0xFFFFFFFB;
    return 0;
}

int sub_8534(int arg0, int arg1)
{
    if (arg1 < 2)
        return 0x80010005;
    if (arg1 >= *(int*)(arg0 + 48))
        return 0x80010005;
    v1 = *(int*)(a0 + 40)
    if (v1 == 0xFFF)
    {
        // 85CC
        v0 = sub_A004(a0, *(int*)(a0 + 44), arg1 + (s0 >> 1), sp, 2);
        if (v0 < 0)
            return v0;
        if (arg1 & 1 != 0)
        {
            // 860C
            return ((*(char*)(sp + 1) << 4) | (*(char*)(sp + 0) >> 4)) & *(int*)(arg0 + 40);
        }
        return (*(char*)(sp + 0) | ((*(char*)(sp + 1) & 0xF) << 8)) & *(int*)(arg0 + 40);
    }
    if (v1 != 0xFFFF)
        return -1;
    v0 = sub_A004(a0, *(int*)(a0 + 44), arg1 << 1, sp, 2);
    if (v0 < 0)
        return v0;
    t7 = *(int*)sp; // unaligned reading
    return (t7 & 0xFFFF) & *(int*)(arg0 + 40);
}

char charMap[] = // 0x88612904
{
    '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0',
    '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0',
    0x02, '!' , '\0', '#' , '$' , '%' , '&' , '\'', '(' , ')' , '\0', 0x01, 0x01, '-' , 0x02, '\0',
    '0' , '1' , '2' , '3' , '4' , '5' , '6' , '7' , '8' , '9' , '\0', 0x01, '\0', 0x01, '\0', '\0',
    '@' , 'A' , 'B' , 'C' , 'D' , 'E' , 'F' , 'G' , 'H' , 'I' , 'J' , 'K' , 'L' , 'M' , 'N' , 'O' ,
    'P' , 'Q' , 'R' , 'S' , 'T' , 'U' , 'V' , 'W' , 'X' , 'Y' , 'Z' , 0x01, '\0', 0x01, '^' , '_' ,
    '`' , 'A' , 'B' , 'C' , 'D' , 'E' , 'F' , 'G' , 'H' , 'I' , 'J' , 'K' , 'L' , 'M' , 'N' , 'O' ,
    'P' , 'Q' , 'R' , 'S' , 'T' , 'U' , 'V' , 'W' , 'X' , 'Y' , 'Z' , '{' , '\0', '}' , '~' , '\0',
    0x80, 0x81, 0x82, 0x83, 0x84, 0x85, 0x86, 0x87, 0x88, 0x89, 0x8A, 0x8B, 0x8C, 0x8D, 0x8E, 0x8F,
    0x90, 0x91, 0x92, 0x93, 0x94, 0x95, 0x96, 0x97, 0x98, 0x99, 0x9A, 0x9B, 0x9C, 0x9D, 0x9E, 0x9F,
    0xA0, 0xA1, 0xA2, 0xA3, 0xA4, 0xA5, 0xA6, 0xA7, 0xA8, 0xA9, 0xAA, 0xAB, 0xAC, 0xAD, 0xAE, 0xAF,
    0xB0, 0xB1, 0xB2, 0xB3, 0xB4, 0xB5, 0xB6, 0xB7, 0xB8, 0xB9, 0xBA, 0xBB, 0xBC, 0xBD, 0xBE, 0xBF,
    0xC0, 0xC1, 0xC2, 0xC3, 0xC4, 0xC5, 0xC6, 0xC7, 0xC8, 0xC9, 0xCA, 0xCB, 0xCC, 0xCD, 0xCE, 0xCF,
    0xD0, 0xD1, 0xD2, 0xD3, 0xD4, 0xD5, 0xD6, 0xD7, 0xD8, 0xD9, 0xDA, 0xDB, 0xDC, 0xDD, 0xDE, 0xDF,
    0xE0, 0xE1, 0xE2, 0xE3, 0xE4, 0xE5, 0xE6, 0xE7, 0xE8, 0xE9, 0xEA, 0xEB, 0xEC, 0xED, 0xEE, 0xEF,
    0xF0, 0xF1, 0xF2, 0xF3, 0xF4, 0xF5, 0xF6, 0xF7, 0xF8, 0xF9, 0xFA, 0xFB, 0xFC, 0xFD, 0xFE, 0xFF
}

int sub_861C(int arg0, int arg1, int arg2, int arg3)
{
    /* Some parser? */
    s4 = arg2;
    s3 = arg1;
    s1 = arg0;
    s0 = arg3;
    s2 = 1;
    memset(arg1, ' ', 11);
    if (s0 != 0)
        *(char*)(s3 + 11) = 0;
    // 8664
    a1 = *(char*)(s1 + 0);
    a2 = s1;
    int a0;
    for (a0 = 0; *(char*)a2 ^ 0x2F != 0 && *(char*)a2 != 0; a0++) // Count number of chars before '/' ?
        a2++;
    // 86A8 // . directory
    if (a0 == 1 && a1 & 0xFF == '.') { // 8AF0
        *(char*)s3 = v1;
        return (s4 < 2);
    }
    // 86B8 // .. directory
    if (a0 == 2 && *(char*)(s1 + 0) == '.' && *(char*)(s1 + 1) == '.') // 8AC8
    {
        *(char*)(s3 + 0) = '.';
        *(char*)(s3 + 1) = '.';
        return (s4 < 2);
    }
    a1 = (a0 > 0);
    // 86C4
    a2 = s1;
    a3 = 0;
    if (a1 == 0)
    {
        // 8AC0
        v0 = (a0 > 0);
    }
    else
    {
        // 86D4
        do
        {
            t2 = *(char*)a2;
            a2++;
            if ((t2 ^ '.' != 0) && (t2 ^ ' ' != 0))
                break;
            a3++;
        } while (a3 < a0);

        if (a3 >= a0)
            return 0;

        // 8424
        v0 = a3 < a0;
    }
    // 8728
    if (v0 == 0)
        return 0;
    a2 = s1;
    a3 = 0;
    if (a1 != 0)
    {
        do
        {
            t6 = *(char*)a2;
            a3++;
            a2++;
            if (charMap[t6] == 0)
                return 0;
        } while (a3 < a0);
    }
    // 876C
    a1 = 0;
    t2 = 0;
    a2 = s1 + 1;
    char skip = 0;
    if (a0 > 1)
    {
        a3 = a0 - 1;
        // 8790
        do
        {
            v0 = *(char*)a2;
            v1 = a2 + 1;
            if (v0 == '.')
            {
                // 8AB0
                if (a1 == 0)
                    a1 = v1;
            }
            if (v0 != ' ')
            {
                if (a1 != 0)
                    t2 = a1;
                a1 = 0;
            }
            a3--;
            a2 = v1;
        } while (a3 != 0);
        a3 = a0;
        if (t2 != 0)
        {
            t3 = a1 - t2;
            if (a1 == 0)
            {
                // 8A58
                a0 = t2 - s1;
                t3 = a3 - a0;
            }
            // 87C8
            a3 = 0;
            t1 = 8;
            v1 = (t3 > 0);
            if (t3 > 0)
            {
                // 87F4
                do
                {
                    t0 = *(char*)(t2 + a3);
                    v0 = s2 ^ 3;
                    a0 = (v0 != 0);
                    t8 = charMap[t0];
                    a2 = s3 + t1;
                    a1 = t8 & 0xFF;
                    t9 = a1 ^ t0;
                    t0 = (t9 != 0);
                    t9 = t0 & a0;
                    *(char*)a2 = t8;
                    t8 = 2;
                    if (t9 != 0)
                        s2 = t8;
                    if (a1 == 1)
                    {
                        // 8A4C
                        s2 = 3;
                        *(char*)a2 = '_';
                    }
                    else if (a1 == 2) {
                        // 8A40
                        *(char*)a2 = ' ';
                        s2 = 3;
                        t1--;
                    }
                    // 8834
                    a3++;
                    t1++;
                    t8 = t1 < 11;
                    v1 = a3 < t3;
                    a2 = v1 & t8;
                } while (a2 != 0);
            }
            // 8850
            a3 = 3;
            if (v1 != 0)
                s2 = a3;
            skip = 1;
        }
    }
    if (!skip)
    {
        // 8A64
        t6 = *(char*)(a2 - 1);
        t2 = a2 - 1;
        // 8A88
        while ((t6 == ' ') || (t6 == '.'))
        {
            t2--;
            t6 = *(char*)t2;
        }
    }
    // 8AA8
    t2++;
    // 885C
    a2 = s1;
    s1 = s1 < t2;
    t1 = 0;
    v1 = s1;
    if (s1 != 0)
    {
        // 888C
        do
        {
            s0 = *(char*)a2;
            v0 = s2 ^ 3;
            s1 = (v0 != 0);
            t9 = charMap[s0];
            a3 = s3 + t1;
            a1 = t9 & 0xFF;
            t8 = a1 ^ s0;
            a0 = (t8 != 0);
            t0 = a0 & s1;
            s0 = 2;
            *(char*)a3 = t9;
            if (t0 != 0)
                s2 = s0;
            if (a1 == 1)
            {
                // 8A34
                s2 = 3;
                *(char*)a3 = '_';
            }
            else if (a1 == 2)
            {
                *(char*)a3 = ' ';
                // 8A28
                s2 = 3;
                t1--;
            }
            // 88C8
            t1++;
            a2++;
            t0 = t1 < 8;
            v1 = a2 < t2;
            a1 = v1 & t0;
        } while (a1 != 0);
    }
    // 88E4
    a0 = 3;
    if (v1 != 0)
        s2 = a0;
    if (t1 == 0)
        *(char*)s3 = '_';
    // 88F8
    if (*(char*)s3 == 229)
    {
        // 8A20
        *(char*)s3 = 5;
    }
    // 8908
    if (s4 < 2)
        t4 = s2;
    else
        t4 = 0;
    if (s2 != a0)
        return t4;
    if (s4 == 0)
        return 3;
    a2 = sp + 6;
    t6 = sp < a2;
    t7 = (s4 != 0);
    s2 = t6 & t7;
    t0 = a2;
    if (s2 != 0)
    {
        a3 = 0x66666667;
        do
        {
            a2--;
            t8 = sp < a2;
            a1 = (((s4 * a3) >> 32) >> 2) - (s4 >> 31);
            t2 = ((a1 << 2) + a1) << 1;
            s1 = s4 - t2;
            s0 = (a1 != 0);
            a0 = s1 + 48;
            t9 = t8 & s0;
            s4 = a1;
            *(char*)a2 = a0;
        } while (t9 != 0);
    }
    // 8988
    if (s4 != 0)
        return 0;
    s2 = *(char*)(s3 + 7);
    a3 = 7;
    if (s2 == ' ')
    {
        // 8A00
        // 8A08
        do
        {
            a3--;
            t7 = s3 + a3;
            t6 = *(char*)t7;
        } while (t6 == ' ');
    }
    t8 = sp - a2;
    a0 = 7 - a3;
    s1 = t8 + 7;
    t9 = a0 < s1;
    t2 = s3 + a3;
    if (t9 != 0)
    {
        s0 = a2 - sp;
        a3 = s0 + 1;
        t2 = s3 + a3;
    }
    // 89C8
    a1 = a2 < t0;
    *(char*)t2 = '~';
    a3++;
    if (a1 != 0)
    {
        // 89DC
        do
        {
            t5 = *(char*)a2;
            a2++;
            t4 = s3 + a3;
            t3 = a2 < t0;
            *(char*)t4 = t5;
            a3++;
        } while (t3 != 0);
    }
    return 3;
}

int sub_8B08(int arg0, int arg1)
{
    int i;
    // 8B18
    for (i = 0; i < 8; i++)
        if (*(char*)(arg0 + i) != *(char*)(arg1 + i))
            return 0;
    // 8B44
    for (i = 0; i < 3; i++)
        if (*(char*)(arg0 + i + 8) != *(char*)(arg1 + i + 8))
            return 0;
    return 1;
}

int sub_8B74(int arg0, int arg1, int arg2, int arg3)
{
    s6 = 0;
    s0 = arg2;
    s4 = *(int*)(arg0 + 8);
    if (arg2 <= 0)
        return 0;
    do
    {
        a3 = *(int*)(arg0 + 16);
        if (arg3 != 0)
        {
            if (*(int*)(arg0 + 12) == *(int*)(s4 + 84))
                a3 += 64;
        }
        // 8BD4
        a1 = *(int*)(arg0 + 20);
        a3 -= a1;
        if (a3 < arg2)
        {
            if (a3 <= 0)
                return s6;
            s0 = a3;
        }
        // 8BF0
        char skip = 0;
        if (arg3 != 0)
        {
            a0 = *(int*)(arg0 + 12);
            a3 = *(int*)(s4 + 84);
            if (a0 == a3)
            {
                // 8CDC
                if (s0 != ' ')
                    return 0x80010005;
                if (a1 < 64)
                {
                    bzero(arg1, 32);
                    memset(arg1, ' ', 11);
                    *(char*)(arg1 + 11) = 16;
                    a1 = *(int*)(arg0 + 12);
                    s3 = arg1 + 28;
                    if (a1 == 0)
                    {
                        // 8DAC
                        v0 = *(int*)(s4 + 64) << 5;
                    }
                    else
                    {
                        int count = 0;
                        // 8D80
                        while (a1 | ~*(int*)(s4 + 40) < -10)
                        {
                            v0 = sub_8534(s4, a1);
                            a1 = v0;
                            count++;
                        }
                        // 8D40
                        v0 = count * *(int*)(s4 + 76);
                    }
                    // 8D48
                    *(int*)(arg0 + 16) = v0;
                    *(int*)s3 = v0; // unaligned writing
                    *(char*)(arg1 + 0) = ',';
                    v0 = *(int*)(arg0 + 20);
                    if (v0 > 0)
                    {
                        *(char*)(arg1 + 1) = ',';
                        v0 = *(int*)(arg0 + 20);
                    }
                    // 8D74
                    *(int*)(arg0 + 20) = v0 + s0;
                    return s0;
                }
            }
            // 8C08
            if (arg3 != 0 && a0 == 0)
            {
                // 8CB4
                v0 = sub_9E70(s4, 0, *(int*)(arg0 + 20), arg1, s0);
                if (v0 < 0)
                    return v0;
                skip = 1;
            }
        }
        if (!skip)
        {
            // 8C18
            v0 = sub_A188(arg0, arg3);
            if (v0 < 0)
                return v0;
            a3 = *(int*)(arg0 + 24);
            s0 = min(s0, *(int*)(s4 + 76) - a3);
            v0 = sub_9E70(s4, *(int*)(arg0 + 28), a3, arg1, s0);
            if (v0 < 0)
                return v0;
            *(int*)(arg0 + 24) += s0;
        }
        // 8C64
        arg2 -= s0;
        s6 += s0;
        arg1 += s0;
        *(int*)(arg0 + 20) += s0;
        s0 = arg2;
    } while (arg2 > 0);
    return s6;
}

int unkAddr1; // 0x88630C58
int unkAddr2; // 0x88630C5C

int sub_8DB8(int arg0, int arg1)
{
    s0 = unkFatCurAddr;
    if (s0 == 0)
        return 0;
    unkFatCurAddr = *(int*)(s0 + 0);
    bzero(s0, 84);
    *(int*)(s0 + 8) = arg0;
    *(int*)(arg0 + 8)++;
    *(int*)(s0 + 4) = unkAddr2;
    *(int*)(s0 + 12) = arg1;
    *(int*)(s0 + 28) = arg1;
    if (v1 == 0)
        unkAddr1 = s0;
    else
        *(int*)v1 = s0;
    // 8E24
    unkAddr2 = s0;
    *(int*)(s0 + 0) = 0;
    return s0;
}

int sub_8E54(int arg)
{
    if (arg == 0)
        return 0;
    a2 = *(int*)(arg + 8);
    a1 = *(int*)(arg + 0);
    *(int*)(a2 + 8)--;
    if (a1 == 0)
    {
        // 8EAC
        unkAddr2 = *(int*)(arg + 4);
    }
    else
        *(int*)(a1 + 4) = *(int*)(arg + 4);
    // 8E7C
    v0 = *(int*)(arg + 4);
    if (v0 == 0) // 8EA4
        unkAddr1 = a1;
    else
        *(int*)v0 = a1;
    // 8E8C
    *(int*)(arg + 0) = unkFatCurAddr;
    unkFatCurAddr = arg;
    return 0;
}

int sub_8EBC(int arg0, int arg1)
{
    if (*(int*)(arg0 + 616) == 1) {
        *(char*)arg1 = 0;
        return;
    }
    v1 = *(short*)arg1; // unaligned reading
    s0 = arg1;
    if (v1 != 0)
    {
        // 8F00
        do
        {
            if (*(int*)(arg0 + 616) == 2)
            {
                // 8F8C
                if (arg1 == 0)
                    *(char*)(s0++) = v1;
                else
                    *(char*)(s0++) = '?'
            }
            else
            {
                int (*func)(int, int) = *(int*)(arg0 + 620);
                v0 = func(v1, v1 & 0xFF00);
                if (v0 & 0xFF00 != 0)
                {
                    *(char*)s0 = v0 >> 8;
                    s0++;
                }
                // 8F30
                *(char*)s0 = v0;
                s0++;
            }
            // 8F38
            v1 = *(short*)arg1;
        } while (v1 != 0);
    }
    // 8F58
    *(char*)s0 = 0;
    return;
}

int sub_8F9C(int arg0, int arg1, int arg2, int arg3)
{
    a2 = *(char*)arg2;
    a3 = a2 & 0x3F;
    if ((a3 == 0) || (a3 >= 10))
        return -1;
    if (a2 & 0x40 == 0 && arg3 != *(char*)(arg2 + 13)) // 90B8
        return -1;
    else
        arg3 = *(char*)(arg2 + 13);
    
    // 8FFC
    s0 = ((a2 & 0x3F) - 1) * 26
    v0 = sub_A240(arg0, arg1 + s0, arg2 + 1, 10);
    if (v0 < 0)
        return -1;
    s0 += v0;
    if (v0 != 0)
    {
        // 9074
        v0 = sub_A240(arg0, arg1 + s0, arg2 + 14, 12);
        if (v0 < 0)
            return -1;
        if (v0 == 0)
            return arg3;
        v0 = sub_A240(arg0, arg1 + s0 + v0, arg2 + 28, 4);
        if (v0 < 0)
            return -1;
    }
    return arg3;
}

int sub_90C8(int arg0, int arg1)
{
    v1 = *(char*)arg0;
    a3 = (int)(char)v1;
    if (a3 != 0 && *(char*)arg1 != 0 && *(char*)arg1 != '/')
    {
        // 90F4
        do
        {
            a1++;
            a0++;
            if (a3 != a2)
                return 0;
            v1 = *(char)a0;
            a2 = (int)(char)v1;
            a3 = a2;
            if (a2 == 0)
                break;
            v0 = *(char*)a1;
            a2 = v0;
            if (v0 == 0)
                break;
        } while (v0 != '/');
    }
    // 9124
    // 9128
    if (v1 != 0)
        return 0;
    t2 = *(char*)a1;
    if ((t2 == '/') || (t2 == 0))
        return a1;
    return 0;
}

int sub_9158(int arg0, char *name, int arg2, int arg3)
{
    int ptr1[2];
    int ptr2, ptr3;
    t0 = *(int*)(arg0 + 4);
    if (t0 < 0 || t0 >= fatUnitsNumber)
        return 0x80010018;
    // 91D0
    a2 = unkFatAddr + t0 * 628;
    if (*(int*)a2 & 1 == 0)
        return 0x80020321;
    // 9200
    s5 = a2;
    v0 = sub_A2C0(arg0, &ptr2, a2, name, &ptr3);
    if (v0 < 0)
    {
        // 9368
        if (v0 != 0x80010002)
            return v0;
        if (arg2 & 0x200 == 0)
            return v0;
        t1 = ptr3;
        if (t1 < 0)
            return v0;
        return sub_A558(arg0, name, arg2, arg3, s5, t1);
    }
    v1 = *(char*)(ptr2 + 47);
    if (v1 & 0x10 != 0)
    {
        // 9358
        sub_8E54(ptr2);
        return 0x80010015;
    }
    if (arg2 & 2 != 0)
    {
        if (v1 & 1 != 0)
        {
            // 9348
            sub_8E54(ptr2);
            return 0x80020142;
        }
        if (*(int*)s5 & 2 != 0)
        {
            // 9338
            sub_8E54(ptr2);
            return 0x8001001E;
        }
        if (arg2 & 0x400)
        {
            // 9304
            sub_A0F8(s5, *(int*)(ptr2 + 21));
            *(int*)(ptr2 + 28) = 0;
            *(int*)(ptr2 + 12) = 0;
            *(int*)(ptr2 + 16) = 0;
            v0 = sub_A6DC(ptr2, 0);
            if (v0 < 0)
                return v0;
        }
        // 925C
        if (arg2 & 0x100 != 0)
        {
            *(int*)(ptr2 + 28) = -1;
            *(int*)(ptr2 + 20) = *(int*)(ptr2 + 16);
        }
        // 9274
        v0 = sub_8434(s5);
        if (v0 < 0)
        {
            // 92F4
            sub_8E54(ptr2);
            return v0;
        }
        if (*(int*)(s5 + 88) != 0)
        {
            // 92C0
            ptr1[0] = *(int*)(s5 + 92);
            ptr1[1] = *(int*)(s5 + 96);
            sub_9C6C(s5, (*(int*)(s5 + 88) << (*(int*)(s5 + 36) & 0x1F)) + 488, ptr, 8);
        }
        // 9294
    }
    // 9298
    if (arg2 & 0x800 != 0)
    {
        // 92AC
        sub_8E54(ptr2);
        return 0x80010011;
    }
    *(int*)(arg0 + 16) = ptr2;
    return 0;
}

int sub_93AC(int arg)
{
    s1 = *(int*)(arg + 16);
    if (*(int*)(s1 + 80) != 0)
    {
        s0 = *(int*)(s1 + 8);
        v0 = sub_8434(s0);
        if (v0 < 0)
            return v0;
        if (*(int*)(s0 + 88) != 0)
        {
            int ptr[2];
            // 9424
            ptr[0] = *(int*)(s0 + 92);
            ptr[1] = *(int*)(s0 + 96);
            v0 = sub_9C6C(s0, (*(int*)(s0 + 88) << (*(int*)(s0 + 36) & 0x1F)) + 488, ptr, 8);
        }
        // 93F4
        v0 = sub_A6DC(s1, -1);
        if (v0 < 0)
            return v0;
    }
    // 9404
    sub_8E54(s1);
    return 0;
}

int sub_9450(int arg0, int arg1, int arg2)
{
    a0 = *(int*)(arg0 + 16);
    a3 = *(int*)(a0 + 20);
    if (arg2 == 1)
    {
        // 9500
        arg1 += a3;
    }
    else if (arg2 < 2)
    {
        // 94F0
        if (arg2 != 0)
            return 0x80010016;
    }
    else if (arg2 == 2)
    {
        // 9484
        arg1 += *(int*)(a0 + 16);
    }
    else
        return 0x80010016;

    // 9488
    if (arg1 < 0)
        return 0x80010016;
    if (*(int*)(a0 + 16) < arg1)
        return 0x80010016;
    *(int*)(a0 + 20) = arg1;
    if (*(int*)(a0 + 12) != 0)
    {
        a2 = *(int*)(*(int*)(a0 + 8) + 76) - 1;
        if (((a3 - ((a3 & a2) == 0)) ^ arg1) & ~a2 == 0)
        {
            t0 = arg1 & a2;
            *(int*)(a0 + 24) = t0;
            if (t0 != 0)
                return *(int*)(a0 + 20);
        }
        // 94E4
        *(int*)(a0 + 28) = -1;
    }
    return *(int*)(a0 + 20);
}

int sub_99B8(int arg)
{
    sub_7CE0();
    a0 = *(int*)(arg + 4);
    if (a0 < 0 || a0 >= fatUnitsNumber)
        return 0x80010018;
    // 9A04
    s0 = unkFatAddr + a0 * 628;
    if (*(int*)s0 & 1 == 0)
        return 0x80010013;
    if (*(int*)(s0 + 8) > 0)
        return 0x80010018;
    sub_CC10();
    v1 = unkFatVar1;
    // 9A5C
    while (v1 != 0)
    {
        if (*(int*)(v1 + 8) == s0)
            *(int*)(v1 + 16) = -1;
        // 9A64
        v1 = *(int*)v1;
    }
    // 9A74
    *(int*)s0 &= 0xFFFFFFFE;
    return 0;
}

int sub_9B34(int arg)
{
    int sp[21]; // TODO: check size
    v0 = sub_CC70(sp);
    if (v0 < 0)
        return 0x80010005;
    *(int*)(arg0 + 12) = sp[4];
    *(int*)(arg0 + 16) = sp[5];
    *(int*)(arg0 + 20) = sp[6];
    *(int*)(arg0 + 24) = sp[7];
    *(int*)(arg0 + 28) = sp[8];
    if (sp[8] & 1 != 0)
        *(int*)(arg0 + 0) &= 2;
    return 0;
}

int sub_9BAC(int arg0, int arg1, int arg2, int arg3)
{
    int ptr;
    s3 = arg1 >> 9;
    s2 = arg1 & 0x1FF;
    if (arg3 <= 0)
        return 0;
    // 9BEC
    do
    {
        s0 = min(512 - s2, arg3);
        v0 = sub_A9B4(&ptr, arg0, s3, 1);
        if (v0 < 0)
            return v0;
        arg3 -= s0;
        arg2 += s0;
        s3++;
        memcpy(arg2, *(int*)(ptr + 20) + s2, s0);
        s2 = 0;
    } while (arg3 > 0);
    return 0;
}

int sub_9C6C(int arg0, int arg1, int arg2, int arg3)
{
    a0 = *(int*)(arg0 + 24);
    s6 = 9 - a0;
    if (s6 < 0)
    {
        // 9D9C
        v1 = *(int*)(arg0 + 20);
        s3 = a1 >> (a0 & 0x1F);
        s6 = -s6;
        s3 = a1 & (v1 - 1);
        if (a3 <= 0)
            return 0;
    
        // 9DB4
        do
        {
            t4 = v1 - s1;
            s0 = min(t4, arg3);
            if (s1 <= 0 && s0 >= v1) // 9E60
                a3 = 0;
            else
                a3 = 1;
    
            int ptr;
            // 9DD8
            v0 = sub_A9B4(&ptr, arg0, s3 << (s6 & 0x1F), a3);
            if (v0 < 0)
                return v0;
            if (arg2 == 0)
            {
                // 9E44
                bzero(*(int*)(ptr + 20) + s1, s0);
            }
            else
            {
                arg2 += s0;
                memcpy(*(int*)(ptr + 20) + s1, arg2, s0);
            }
            arg3 -= s0;
    
            // 9E0C
            s1 = 0;
            v0 = sub_A8C8(arg0, *(int*)(ptr + 20), s3, 1);
            s3++;
            if (v0 < 0)
                return v0;
            v1 = *(int*)(arg0 + 20);
        } while (arg3 > 0);
    }
    else
    {
        s3 = a1 >> 9;
        s1 = a1 & 0x1FF;
        if (arg3 <= 0)
            return 0;
        fp = 1;
        a1 = 512 - s1;
        do
        {
            int ptr;
            s0 = min(a1, arg3);
            v1 = 0 < s1;
            a0 = s0 < 512;
            v0 = sub_A9B4(&ptr, arg0, s3, v1 | a0);
            if (v0 < 0)
                return v0;
            if (arg2 == 0)
            {
                // 9D80
                bzero(*(int*)(ptr + 20) + s1, s0, arg2);
            }
            else
            {
                arg2 += s0;
                memcpy(*(int*)(ptr + 20) + s1, arg2, s0);
            }

            // 9D18
            arg3 -= s0;
            v0 = sub_A8C8(arg0, *(int*)(ptr + 20), s3 << (s6 & 0x1F), fp << (s6 & 0x1F));
            s1 = 0;
            s3++;
            if (v0 < 0)
                return v0;
            a1 = 512;
        } while (arg3 > 0);
    }
    return 0;
}

int sub_9E70(int arg0, int arg1, int arg2, int arg3, int arg4)
{
    if (arg1 == 0)
    {
        // 9FE8
        arg2 -= 64;
        s3 = *(int*)(arg0 + 60);
        if (arg2 < 0)
            return 0x80010016;
    }
    else
    {
        arg2 += t0;
        if (*(int*)(arg0 + 76) < arg2)
            return 0x80010016;
        s3 = *(int*)(arg0 + 68) + (arg1 - 2) * *(int*)(arg0 + 72);
        if (arg3 != 0)
        {
            v1 = *(int*)(arg0 + 20);
            if (v1 == *(int*)(arg0 + 32) && arg4 >= arg3 && (arg3 & 3) == 0) // 9F30
            {
                t8 = arg2 & (v1 - 1);
                s5 = min(v1 - t8, arg4);
                if ((t8 > 0) && (s5 > 0))
                {
                    t0 = *(int*)(arg0 + 36);
                    // 9FB8
                    v0 = sub_9BAC(arg0, arg2 + (s3 << (t0 & 0x1F)), arg3, s5);
                    if (v0 <= 0)
                        return v0;
                    arg4 -= v0;
                    s3++;
                    arg2 += v0;
                    arg3 += v0;
                }
                s5 = arg4 >> (*(int*)(arg0 + 24) & 0x1F);
                if (s5 > 0)
                {
                    v0 = sub_A7F0(arg0, arg3, s3, s5);
                    if (v0 < 0)
                        return v0;
                    s3 += s5;
                    a2 = s5 << (*(int*)(arg0 + 24) & 0x1F);
                    arg4 -= a2;
                    arg2 += a2;
                    arg3 += a2;
                }
                // 9FA8
                if (arg4 <= 0)
                    return 0;
            }
        }
    }
    // (9EE8)
    // 9EEC
    return sub_9BAC(arg0, arg2 + (s3 << (*(int*)(arg0 + 36) & 0x1F)), arg3, arg4);
}

int sub_A004(int arg0, int arg1, int arg2, int arg3, int arg4)
{
    if (*(int*)(arg0 + 32) == 512)
    {
        // A090
        arg1 += arg2 >> 9;
        s2 = arg2 & 0x1FF;
        // A0A8
        while (arg4 > 0)
        {
            s0 = min(512 - s2, arg4);
            v0 = sub_AE38(arg0, arg1);
            if (v0 < 0)
                return v0;
            memcpy(arg3, arg0 + s2 + 104, s0);
            arg4 -= s0;
            arg1++;
            s2 = 0;
            arg3 += s0;
        }
        return arg4;
    }
    return sub_9BAC(arg0, (arg1 << (*(int*)(arg0 + 36) & 0x1F)) + arg2, a3, arg4);
}

int sub_A0F8(int arg0, int arg1)
{
    if (arg1 == 0)
        return 0;
    if (arg1 | ~*(int*)(arg0 + 40) < -10)
    {
        int ret = arg1;
        int newRet;
        // A148
        do
        {
            newRet = sub_8534(arg0, ret);
            v0 = sub_AEA8(arg0, ret);
            if (v0 < 0)
                return v0;
            ret = newRet;
        } while (ret | ~*(int*)(arg0 + 40) < -10);
    }
    return 0;
}

int sub_A188(int arg0, int arg1)
{
    int unk = *(int*)(arg0 + 8);
    if (arg1 != 0 && *(int*)(arg0 + 12) == 0)
        return 0;
    // A1B0
    if (*(int*)(arg0 + 28) <= 0)
    {
        v0 = *(int*)(arg0 + 20);
        // A22C
        *(int*)(arg0 + 24) = v0;
        *(int*)(arg0 + 28) = *(int*)(arg0 + 12);
        v1 = *(int*)(unk + 76);
    }
    else
    {
        v1 = *(int*)(unk + 76);
        v0 = *(int*)(arg0 + 24);
    }

    while (v0 >= v1)
    {
        v0 = sub_8534(unk, *(int*)(arg0 + 28));
        if (v0 | ~*(int*)(unk + 40) >= -10)
            return 0x80010005;
        *(int*)(arg0 + 28) = v0;
        *(int*)(arg0 + 24) = *(int*)(arg0 + 24) - *(int*)(unk + 76);
        v1 = *(int*)(unk + 76);
        v0 = *(int*)(arg0 + 24);
    }
}

int sub_A240(int arg0, int arg1, int arg2, int arg3)
{
    if (*(int*)(arg0 + 616) == 1)
        return -1;
    if (arg3 <= 0)
        return 0;
    // A25C
    for (;;)
    {
        v1 = *(short*)arg2; // unaligned reading
        t2 += 2;
        if (*(int*)(arg0 + 616) == 2 && v1 & 0xFF00 != 0) // A2B0
            return -1;
        // A27C
        *(char*)(arg1 + 0) = v1;
        *(char*)(arg1 + 1) = v1 >> 8;
        arg1 += 2;
        arg2 += 2;
        if (v1 == 0)
            return 0;
        if (t2 >= arg3)
            return t2;
    }
}
   
int sub_A2C0(int arg0, int arg1, int arg2, char *name, int arg4)
{
    s4 = 0;
    *(int*)(sp + 32) = 0;
    *(int*)(sp + 36) = 0;
    *(int*)(sp + 40) = 0;
    if (strcmp("/", name) == 0)
    {
        v1 = *(int*)(arg2 + 84);
        // A550
        s4 = 1;
    }
    else
    {
        if (*(name++) != '/')
            return 0x80010016;
        // A334
        while (*(name++) != '/')
            ;
        // A34C
        v0 = sub_78AC(arg0, sp, arg2, name, *(int*)(arg2 + 84), sp + 32, sp + 36, sp + 40);
        if (v0 < 0)
        {
            // A53C
            if (arg4 != 0)
                *(int*)arg4 = *(int*)(sp + 32);
            // A548
            return v0;
        }
        t0 = (*(int*)(sp + 20) << 16) | (*(int*)(sp + 26) & 0xFFFF); // 20: unaligned reading
        if (t0 == 0)
            v1 = *(int*)(arg2 + 84);
        else
            v1 = t0;
        // A39C
    }
    // A3A0
    s0 = sub_8DB8(arg2, v1);
    if (s0 == 0)
        return 0x80010018;
    v0 = *(int*)(sp + 32);
    if (arg4 != 0)
        *(int*)arg4 = v0;
    // A3C8
    *(int*)(s0 + 68) = v0;
    *(int*)(s0 + 72) = *(int*)(sp + 36);
    *(int*)(s0 + 76) = *(int*)(sp + 40);
    if (s4 != 0)
    {
        // A4A8
        memset(s0 + 36, ' ', 11);
        a1 = *(int*)(s0 + 12);
        *(char*)(s0 + 47) = 16;
        s3 = s0 + 64;
        if (a1 == 0)
        {
            // A530
            v0 = *(int*)(arg2 + 64) << 5;
        }
        else
        {
            // A504
            for (i = 0; (a1 | ~*(int*)(arg2 + 40)) < -10; i++)
                a1 = sub_8534(arg2, a1);
            // A4E8
            v0 = i * *(int*)(arg2 + 76);
        }
        // A4F0
        *(int*)(s0 + 16) = v0;
        *(int*)s3 = v0; // unaligned writing
        *(int*)arg1 = s0;
        return 0;
    }
    memcpy(s0 + 36, sp, 32);
    if (*(char*)(sp + 11) & 0x10 == 0)
    {
        // A498
        *(int*)(s0 + 16) = *(int*)(sp + 28); // unaligned reading
    }
    else
    {
        a1 = *(int*)(s0 + 12);
        if (a1 == 0)
        {
            // A490
            v0 = *(int*)(arg2 + 64) << 5;
        }
        else
        {
            int i;
            // A464
            for (i = 0; a1 | ~*(int*)(arg2 + 40) < -10; i++)
                a1 = sub_8534(arg2, a1);
            // A428
            v0 = i * *(int*)(arg2 + 76);
        }
        // A430
        *(int*)(s0 + 16) = v0;
    }
    // A434
    *(int*)arg1 = s0;
    return 0;
}

int unkUnusedVar1; // 0x88630C74

int sub_A558(int arg0, char *name, int arg2, int arg3, int arg4, int arg5)
{
    char str[32];
    int ptr1, ptr2;
    bzero(str, 32);
    v0 = strrchr(name, '/');
    if (v0 == 0)
        t1 = name;
    else
        t1 = v0 + 1;
    if (arg3 & 0x80 == 0)
        str[11] = '!';
    else
        str[11] = ' ';
    // A5E0
    unkTab2[6] = 0x7BC;
    unkTab2[0] = 0;
    unkTab2[5] = 1;
    unkTab2[7] = 7;
    unkUnusedVar1 = 0;
    unkTab2[4] = 1;
    unkTab2[1] = 0;
    unkTab2[2] = 0;
    unkTab2[3] = 0;
    str[14] = 0;
    short v1 = (((*(short*)&unkTab2[6] - 0x7BC) << 9) | 0x21) & 0xFFFF; // unaligned reading
    char v2 = v1 >> 8;
    str[24] = v1;
    str[16] = v1;
    str[18] = v1;
    str[25] = v2;
    str[17] = v2;
    str[15] = 0;
    str[13] = 0;
    str[19] = v2;
    str[22] = 0;
    str[23] = 0;
    v0 = sub_AF34(arg4, arg5, str, &ptr1, &ptr2, t1);
    if (v0 < 0)
        return v0;
    s0 = sub_8DB8(arg4, 0);
    if (s0 == 0)
        return 0x80010018;
    memcpy(s0 + 36, str, 32);
    *(int*)(arg0 + 16) = s0;
    *(int*)(s0 + 68) = ptr1;
    *(int*)(s0 + 16) = 0;
    *(int*)(s0 + 72) = ptr2;
    return 0;
}

int sub_A6DC(int arg0, int arg1)
{
    s2 = *(int*)(arg0 + 8);
    sub_9E70(s2, *(int*)(arg0 + 68), *(int*)(arg0 + 72), sp, 32);
    if (v0 < 0)
        return v0;
    *(int*)(sp + 28) = *(int*)(arg0 + 16); // unaligned writing
    unkTab2[6] = 0xBC;
    a0 = (*(short*)&unkTab2[6] - 0x7BC) << 9 | 0x21; // unaligned reading
    unkTab2[0] = 0;
    unkTab2[5] = 1;
    unkTab2[7] = 7;
    unkUnusedVar1 = 0;
    *(char*)(sp + 25) = a0 >> 8;
    unkTab2[1] = 0;
    unkTab2[2] = 0;
    unkTab2[3] = 0;
    unkTab2[4] = 1;
    *(char*)(sp + 24) = a0;
    *(char*)(sp + 22) = 0;
    *(char*)(sp + 23) = 0;
    if (arg1 >= 0)
    {
        *(char*)(sp + 27) = arg1 >> 8;
        *(char*)(sp + 21) = arg1 >> 24;
        *(char*)(sp + 26) = arg1;
        *(char*)(sp + 20) = arg1 >> 16;
    }
    // A7C4
    v0 = sub_AC5C(s2, *(int*)(arg0 + 68), *(int*)(arg0 + 72), sp, 32);
    if (v0 < 0)
        return v0;
    return 0;
}

int sub_A7F0(int arg0, int arg1, int arg2, int arg3)
{
    s2 = (arg2 >> 31) | (arg3 < 1);
    t1 = arg2 + arg3;
    if (s2 != 0)
        return 0x80010005;
    if (*(int*)(arg0 + 16) < t1)
        return 0x80010005;
    // A864
    t1 = *(int*)(arg0 + 24);
    t0 = t1 << 26;
    if (t0 >= 0)
    {
        // A888
        a1 = (arg2 >> 31) << (t1 & 0x1F);
        if (t0 != 0)
        {
            t0 = -t1;
            t0 = arg2 >> (t0 & 0x1F);
            a1 |= t0;
        }
        // A89C
        a0 = arg2 << (t1 & 0x1F);
    }
    else {
        a1 = arg2 << (t1 & 0x1F);
        a0 = 0;
    }

    // A8A0
    v0 = sub_CCC4(a0, a1, 0);
    if (v0 < 0)
        return v0;
    v0 = sub_CD28(arg1, s1 << (*(int*)(arg0 + 24) & 0x1F));
    if (v0 < s2)
        return v0;
    return s2;
}

int sub_A8C8(int arg0, int arg1, int arg2, int arg3)
{
    if ((arg2 >> 31) == 1 || arg3 == 0)
        return 0x80010005;
    if (*(int*)(a0 + 16) < arg2 + arg3)
        return 0x80010005;;
    s2 = *(int*)(arg0 + 0) & 2;
    if (s2 != 0)
        return 0x8001001E;
    t3 = *(int*)(arg0 + 24);
    t0 = t3 << 26;
    if (t0 >= 0)
    {
        // A958
        if (t0 == 0)
            a1 = (arg2 >> 31) << t3;
        else
            a1 |= (arg2 >> -t3);
        // A96C
        a0 = arg2 << t3;
    }
    else {
        a1 = arg2 << t3;
        a0 = 0;
    }

    // A970
    v0 = sub_CCC4(a0, a1, 0);
    if (v0 < 0)
        return v0;
    a2 = *(int*)(s0 + 24);
    v0 = sub_CD78(arg1, arg3 << a2, a2);
    return (v0 < s2) ? v0 : s2;
}

int sub_A9B4(int arg0, int arg1, int arg2, int arg3)
{
    t0 = 0;
    s0 = unkFatVar1;
    // AA00
    while (s0 != 0)
    {
        if (arg1 == *(int*)(s0 +  8)
         && arg2 == *(int*)(s0 + 16)) // AC48
            return sub_B090(arg0, s0); // AA9C
        s0 = *(int*)s0;
        // AA0C
    }
    // AA14
    s0 = unkFatVar2;
    if (s0 == 0)
        return 0x80010005;
    s4 = 9 - *(int*)(arg1 + 24);
    if (s4 < 0)
    {
        s4 = -s4;
        s5 = 1 << (s4 & 0x1F);
        fp = 0 < s5;
        // AAFC
        do
        {
            s2 = 0;
            s1 = s0;
            if (fp == 0 || s0 >= unkFatVar4) // AC40
                a1 = 0 < s5;
            else
            {
                if (*(int*)(s0 + 12) & 1 != 0)
                    return 0x8001000B;
                // AB30
                do
                {
                    s2++;
                    a1 = s2 < s5;
                    s1 += 24;
                    if (a1 == 0)
                        break;
                } while (s1 < a0);
            }
            // AB50
            if (a1 == 0)
            {
                // AB70
                s1 = s0;
                if (fp != 0)
                {
                    s2 = s5;
                    // AB80
                    do
                    {
                        s2--;
                        *(int*)(s1 + 16) = -1;
                        *(int*)(s1 + 12) |= 1;
                        s1 += 24;
                    } while (s2 != 0);
                }
        
                // AB9C
                char skip = 0;
                if (arg3 != 0)
                {
                    // AC20
                    v0 = sub_A7F0(arg1, *(int*)(s0 + 20), arg2 >> (s4 & 0x1F), 1);
                    t0 = v0;
                    if (v0 < 0)
                        skip = 1;
                }
                if (!skip)
                {
                    // ABA4
                    s2 = 0;
                    s1 = s0;
                    if (fp == 0)
                        return t0;
                    s4 = arg2 & (0xFFFF << s4);
                    // ABBC
                    do
                    {
                        t3 = s4 + s2;
                        *(int*)(s1 + 8) = arg1;
                        t4 = t3 ^ arg2;
                        *(int*)(s1 + 16) = t3;
                        s2++;
                        *(int*)sp = t0;
                        sub_B090((t4 == 0) ? arg0 : 0, s1);
                        s1 += 24;
                        t0 = *(int*)sp;
                    } while (s2 < s5);
                }
                // ABF4
                s1 = s0;
                if (fp != 0)
                {
                    s2 = s5;
                    // AC00
                    do
                    {
                        s2--;
                        *(int*)(s1 + 12) &= 0xFFFFFFFC;
                        s1 += 24;
                    } while (s2 != 0);
                }
                return t0;
            }
            s0 = *(int*)(s0 + 4);
        } while (s0 != 0);
        return 0x80010005;
    }
    v1 = *(int*)(s0 + 12);
    if (v1 & 1 != 0)
        return 0x8001000B;
    *(int*)(s0 + 16) = -1;
    *(int*)(s0 + 12) = v1 | 1;
    if (arg3 != 0)
    {
        v0 = sub_A7F0(arg1, *(int*)(s0 + 20), 1 << (s4 & 0x1F), arg2 << (s4 & 0x1F));
        if (v0 < 0)
        {
            // AADC
            *(int*)(s0 + 12) &= 0xFFFFFFFC;
            return v0;
        }
    }

    // AA80
    *(int*)(s0 + 8) = arg1;
    *(int*)(s0 + 16) = arg2;
    *(int*)(s0 + 12) &= 0xFFFFFFFC;
    return sub_B090(arg0, s0); // AA9C
}

int sub_AC5C(int arg0, int arg1, int arg2, int arg3, int arg4)
{
    if (arg1 == 0)
    {
        // AE1C
        arg2 -= 64;
        arg1 = *(int*)(a0 + 60);
        if (arg2 < 0)
            return 0x80010016;
    }
    else
    {
        a3 = *(int*)(a0 + 76);
        arg2 += t0;
        if (a3 < arg2)
            return 0x80010016;
        arg1 = (arg1 - 2) * *(int*)(arg0 + 72) + *(int*)(arg0 + 68);
        if (arg3 != 0)
        {
            v1 = *(int*)(arg0 + 20);
            a1 = *(int*)(arg0 + 32);
            if (v1 == a1)
            {
                // AD1C
                if (arg4 >= a3 && (arg3 & 3) == 0)
                {
                    t8 = arg2 & (v1 - 1);
                    t9 = v1 - t8;
                    s5 = min(t9, arg4);
                    if ((t8 > 0) && (s5 > 0))
                    {
                        t0 = *(int*)(a0 + 36);
                        // ADEC
                        v0 = sub_9C6C(a0, arg2 + (arg1 << (t0 & 0x1F)), arg3, s5);
                        if (v0 <= 0)
                            return v0;
                        arg4 -= v0;
                        arg1++;
                        arg2 += v0;
                        arg3 += v0;
                    }
                    // AD54
                    a0 = *(int*)(arg0 + 24);
                    s5 = arg4 >> (a0 & 0x1F);
                    if (s5 > 0)
                    {
                        v1 = unkFatVar1;
                        a1 = arg1 + s5;
                        if (v1 != 0)
                        {
                            a2 = -1;
                            t1 = *(int*)(v1 + 8);
                            // AD78
                            do
                            {
                                t1 = *(int*)(v1 + 8);
                                if (t1 == arg0)
                                {
                                    t3 = *(int*)(v1 + 16);
                                    // ADD0
                                    if (t3 >= arg1 && t3 < a1)
                                        *(int*)(v1 + 16) = a2;
                                }
                                // AD80
                                v1 = *(int*)v1;
                            } while (v1 != 0);
                        }
                        // AD8C
                        v0 = sub_A8C8(arg0, arg3, arg1, s5);
                        if (v0 < 0)
                            return v0;
                        t4 = *(int*)(arg0 + 24);
                        arg1 += s5;
                        a1 = s5 << (t4 & 0x1F);
                        arg4 -= a1
                        arg2 += a1;
                        arg3 += a1;
                    }
                    // ADC0
                    if (arg4 <= 0)
                        return 0;
                }
            }
        }
    }
    // ACD4
    // ACD8
    return sub_9C6C(arg0, (arg1 << (*(int*)(arg0 + 36) & 0x1F)) + arg2, arg3, arg4);
}

void sub_AE38(int arg0, int arg1)
{
    if (arg1 == *(int*)(arg0 + 100))
        return 0;
    v0 = sub_8434(arg0);
    if (v0 < 0)
        return v0;
    *(int*)(arg0 + 100) = -1;
    sub_9BAC(arg0, arg1 << (*(int*)(arg0 + 36) & 0x1F), arg0 + 104, 512);
    if (v0 < 0)
        return v0;
    *(int*)(arg0 + 100) = arg1;
    return 0;
}

int sub_AEA8(int arg0, int arg1)
{
    if (arg1 < 2)
        return 0x80010005;
    if (arg1 >= *(int*)(arg0 + 48))
        return 0x80010005;
    v0 = sub_B0E4(arg0, arg1, 0);
    if (v0 < 0)
        return v0;
    if (arg1 < *(int*)(arg0 + 96))
        *(int*)(arg0 + 96) = arg1;
    if (*(int*)(arg0 + 92) != -1)
        *(int*)(arg0 + 92)++;
    return 0;
}

int sub_AF34(int arg0, int arg1, int arg2, int arg3, int arg4, int arg5)
{
    s0 = sub_8DB8(arg0, arg1);
    if (s0 == 0)
        return 0x80010018;
    a1 = *(int*)(s0 + 12);
    if (a1 == 0)
    {
        // B088
        v0 = *(int*)(arg0 + 64) << 5;
    }
    else
    {
        // B05C
        int i;
        for (i = 0; a1 | ~*(int*)(arg0 + 40) < -10; i++)
            a1 = sub_8534(arg0, a1);
        // AFB0
        v0 = i * *(int*)(arg0 + 76);
    }
    // AFB8
    *(int*)(s0 + 16) = v0;
    s1 = sub_B274(s0, arg1, arg2, arg5);
    if (s1 < 0) {
        sub_8E54(s0);
        return s1;
    }
    *(int*)(s0 + 20) -= 32;
    if (arg1 != 0)
        *(int*)(s0 + 24) -= 32;
    // AFF4
    if (arg3 != 0)
    {
        if (arg1 != 0)
            v0 = *(int*)(s0 + 28);
        else
            v0 = 0;
        // B008
        *(int*)arg3 = v0;
    }
    // B00C
    if (arg4 != 0)
    {
        if (arg1 != 0)
            v0 = *(int*)(s0 + 24);
        else
            v0 = *(int*)(s0 + 20);
        // B020
        *(int*)arg4 = v0;
    }
    // B02C
    sub_8E54(s0);
    return s1;
}

int sub_B090(int arg0, int arg1)
{
    if (arg0 != 0)
        *(int*)arg0 = arg1;
    v1 = *(int*)(a1 + 4);
    if (v1 == 0)
        return 0;
    a2 = *(int*)(a1 + 0);
    if (a2 == 0)
        unkFatVar1 = v1;
    else
        *(int*)(a2 + 4) = v1;
    // B0B4
    v0 = unkFatVar1;
    a0 = *(int*)(a1 + 4);
    *(int*)(v0 + 4) = a1;
    *(int*)(a0 + 0) = a2;
    *(int*)(a1 + 4) = 0;
    *(int*)(a1 + 0) = v0;
    unkFatVar1 = a1;
    return 0;
}

int sub_B0E4(int arg0, int arg1, int arg2)
{
    char sp[2]; // TODO: check size
    v1 = *(int*)(arg0 + 40);
    if (v1 == 4095)
    {
        // B20C
        v0 = sub_A004(arg0, *(int*)(arg0 + 44), arg1 + (arg1 >> 1), sp, 2);
        if (v0 < 0)
            return v0;
        if (arg1 & 1 != 0)
        {
            // B258
            sp[1] = (arg2 >> 4) & 0xFF;
            sp[0] = (sp[0] & 0xF) | ((arg2 << 4) & 0xF0);
        }
        else {
            sp[0] = arg2 & 0xFF;
            sp[1] = (sp[1] & 0xF0) | ((arg2 >> 8) & 0xF);
        }
        // B250
        arg1 += s3;
    }
    else
    {
        if (v1 != 0xFFFF)
            return -1;
        arg1 <<= 1;
        sp[1] = (a2 >> 8) & 0xFF;
        sp[0] = a2 & 0xFF;
    }
    // B130
    if (*(int*)(arg0 + 0) & 8 == 0)
    {
        // B1C8
        v0 = sub_B60C(arg0, *(int*)(arg0 + 44) + *(int*)(arg0 + 52) * *(int*)(arg0 + 80), arg1, sp, 2);
        if (v0 < 0)
            return v0;
        return 0;
    }
    s0 = *(int*)(arg0 + 44) + *(int*)(arg0 + 52) * (*(int*)(arg0 + 56) - 1);
    // B18C
    while (s0 >= *(int*)(arg0 + 44))
    {
        v0 = sub_B60C(arg0, s0, arg1, sp, 2);
        if (v0 < 0)
            return v0;
        s0 -= *(int*)(arg0 + 52);
    }
    return 0;
}

int sub_B274(int arg0, int arg1, int arg2, int arg3)
{
    fp = *(int*)(arg0 + 8);
    v1 = sub_861C(arg3, arg2, 0, 0);
    if (v1 == 0)
        return 0x80010016;
    if (v1 == 1)
    {
        // B588
        do
        {
            v0 = sub_8B74(arg0, sp, 32, 1);
            if (v0 < 0)
                return v0;
            if (v0 < 32)
            {
                // B5EC
                if (arg1 == 0)
                    return 0x8001001C;
                v0 = sub_B714(arg0, 1);
                if (v0 < 0)
                    return v0;
                continue;
            }
            t1 = *(char*)sp;
        } while ((t1 != 0) && (t1 == 0xE5));
        *(int*)(arg0 + 20) -= 32;
        if (arg1 != 0)
            *(int*)(arg0 + 24) -= 32;
    }
    else
    {
        s2 = 1;
        if (v1 == 3)
        {
            // B500
            // B504
            for (;;)
            {
                v0 = sub_861C(arg3, arg2, s2, 0);
                s6 = v0;
                if (s6 == 0)
                    return 0x80010016;
                // B528
                do
                {
                    v0 = sub_8B74(arg0, sp, 32, 1);
                    if (v0 < 0)
                        return v0;
                    if (v0 < 32 || *(char*)sp == 0)
                        break;
                } while (v0 == 229 || *(char*)(sp + 11) & 8 != 0 || sub_8B08(sp, arg2) == 0);
                s2++;
            }
        }
        // (B2F0)
        t1 = *(int*)(arg0 + 12);
        // B2F4
        *(int*)(arg0 + 24) = 0;
        s0 = 0;
        *(int*)(arg0 + 28) = t1;
        *(int*)(arg0 + 20) = 0;
        v0 = strlen(arg3);
        s6 = ((v0 + 12) * 0x4EC4EC4F) >> 34 + 1;
        // B32C
        for (;;)
        {
            v0 = sub_8B74(arg0, sp, 32, 1);
            if (v0 < 0)
                return v0;
            if (v0 < 32)
            {
                // B4E0
                if (arg1 == 0)
                    return 0x8001001C;
                v0 = sub_B714(arg0, 1);
                if (v0 < 0)
                    return v0;
            }
            else
            {
                t6 = *(char*)sp;
                s0++;
                if ((t6 == 0) || (t6 == 0xE5))
                {
                    if (s0 >= s6)
                        break;
                }
                else
                    s0 = 0;
            }
        }
        // B37C
        *(int*)(arg0 + 20) -= (s2 << 5) + 32;
        if (arg1 != 0)
        {
            *(int*)(arg0 + 24) -= 32;;
            if (a0 < 0)
            {
                // B4C8
                *(int*)(arg0 + 24) = a0 + *(int*)(fp + 76);
                *(int*)(arg0 + 28) = *(int*)(s1 + 32);
            }
        }
        // B3B0
        bzero(sp, 32);
        *(char*)(sp + 11) = 15;
        a1 = arg2;
        v0 = 0;
        a2 = 1;
        do
        {
            v0 = (((v0 << 7) | (v0 >> 1)) & 0xFF + *(char*)a1) & 0xFF;
            a1++;
            a2++;
        } while (a2 < 11);
        *(char*)(sp + 13) = v0;
        // B40C
        int i;
        for (i = 0; i < s2; i++)
        {
            t7 = (s2 - i) & 0x3F;
            if (i == 0)
                *(char*)sp = t7 | 0x40;
            else
                *(char*)sp = t7;
            // B434
            sub_B970(fp, sp, i, s2, arg3);
            v0 = sub_B7F4(arg0, sp, 32, 1);
            if (v0 < 0)
                return v0;
        }
        // B460
    }
    v0 = sub_A188(arg0, 1);
    if (v0 < 0)
        return v0;
    v0 = sub_B7F4(arg0, arg2, 32, 1);
    if (v0 < 0)
        return v0;
    return 0;
}

int sub_B60C(int arg0, int arg1, int arg2, int arg3, int arg4)
{
    if (*(int*)(arg0 + 32) != 512) {
        v1 = arg1 << (*(int*)(arg0 + 36) & 0x1F);
        return sub_9C6C(arg0, v1 + arg2, arg3, arg4);
    }
    // B688
    arg1 += arg2 >> 9;
    arg2 &= 0x1FF;
    if (arg1 >= *(int*)(arg0 + 44) + *(int*)(arg0 + 52))
        return t0;
    for (;;)
    {
        arg1++;
        if (arg4 <= 0)
            return arg4;
        v0 = sub_AE38(arg0, arg1);
        if (v0 < 0)
            return v0;
        t2 = min(512 - arg2, arg4);
        arg4 -= t2;
        arg2 = 0;
        arg3 += t2;
        memcpy(arg0 + arg2 + 104, arg3, t2);
        *(int*)arg0 |= 4;
    }
}
    
int sub_B714(int arg0, int arg1)
{
    s0 = *(int*)(arg0 + 8);
    if (arg1 != 0 && *(int*)(arg0 + 12) == 0)
        return 0x8001001C;
    // B750
    v0 = sub_BA7C(s0);
    s2 = v0;
    if (v0 < 0)
        return v0;
    if (arg1 != 0)
    {
        v0 = sub_AC5C(s0, v0, 0, 0, *(int*)(s0 + 76));
        if (v0 < 0) {
            sub_AEA8(s0, s2);
            return v0;
        }
    }
    v0 = sub_B0E4(s0, *(int*)(arg0 + 28), s2);
    if (v0 < 0) {
        sub_AEA8(s0, s2);
        return v0;
    }
    if (arg1 != 0)
       *(int*)(arg0 + 16) += *(int*)(s0 + 76)
    return 0;
}

int sub_B7F4(int arg0, int arg1, int arg2, int arg3)
{
    int count = 0;
    s0 = arg2;
    s5 = *(int*)(arg0 + 8);
    if (a2 <= 0)
        return 0;
    // B840
    do
    {
        if (arg3 == 0 || *(int*)(arg0 + 12) != 0)
        {
            // B904
            v0 = sub_A188(arg0, arg3);
            if (v0 < 0)
            {
                // B958
                v0 = sub_B714(arg0, arg3);
                if (v0 < 0)
                    return v0;
            }
            // B914
            t3 = *(int*)(arg0 + 24);
            t1 = *(int*)(s5 + 76) - t3;
            if (t1 < arg2)
                s0 = t1;
            v0 = sub_AC5C(s5, *(int*)(arg0 + 28), t3, arg1, s0);
            if (v0 < 0)
                return v0;
            *(int*)(arg0 + 24) += s0;
        }
        else
        {
            a2 = *(int*)(arg0 + 20);
            v0 = *(int*)(arg0 + 16) - a2 + 64;
            if (v0 < arg2)
            {
                if (v0 <= 0)
                    return 0;
                s0 = v0;
            }
            // B87C
            if (s0 != 32)
                return 0x80010005;
            if (a2 < 64)
                return 0x80010005;
            v0 = sub_AC5C(s5, 0, a2, arg1, 32);
            if (v0 < 0)
                return v0;
        }
        // B8B0
        arg2 -= s0;
        count += s0;
        arg1 += s0;
        *(int*)(arg0 + 20) += s0;
        s0 = arg2;
    } while (arg2 > 0);
    return count;
}

void sub_B970(int arg0, int arg1, int arg2, int arg3, int arg4)
{
    int i;
    short c;
    int size = (arg3 - arg2 - 1) * 13
    int count = strlen(arg4) + 1 - size;
    arg4 -= size;

    char *outPtr = arg1 + 1;
    // B9CC
    for (i = 10; i > 0; i -= 2)
    {
        if (count > 0)
        {
            c = *(char*)arg4;
            count--;
            arg4++;
        }
        else
            c = 0xFFFF;
        // B9E4
        outPtr[0] = c;
        outPtr[1] = c >> 8;
        outPtr += 2;
    }
    outPtr = arg1 + 14;
    // BA00
    for (i = 12; i > 0; i -= 2)
    {
        if (count > 0)
        {
            c = *(char*)arg4;
            count--;
            arg4++;
        }
        else
            c = 0xFFFF;
        // BA18
        outPtr[0] = c;
        outPtr[1] = c >> 8;
        outPtr += 2;
    }
    outrPtr = arg1 + 28;
    for (i = 4; i > 0; i -= 2)
    {
        if (count > 0)
        {
            c = *(char*)arg4;
            count--;
            arg4++;
        }
        else
            c = 0xFFFF;
        // BA4C
        outPtr[0] = c;
        outPtr[1] = c >> 8;
        outPtr += 2;
    }
}

int sub_BA7C(int arg)
{
    // BAA8
    int i;
    for (i = *(int*)(arg + 96); i < *(int*)(arg + 48); i++)
    {
        v0 = sub_8534(arg, i);
        if (v0 < 0)
            return v0;
        if (v0 == 0)
        {
            // BAFC
            v0 = sub_B0E4(arg, i, 0xFFF8);
            if (v0 < 0)
                return v0;
            *(int*)(arg + 96)++;
            if (*(int*)(arg + 92) != -1)
                *(int*)(arg + 92)--;
            return i;
        }
    }
    // BAD8
    *(int*)(arg + 96) = 2;
    return 0x8001001C;
}
                                                                                                                                                                      
void sub_BB34(char *arg)
{
    arg[10] |= 0x80;
}

char sub_BB48(char *arg)
{
    arg[10] |= 0x60;
    return arg[10];
}

char sub_BB5C(char *arg)
{
    arg[10] |= 0x10;
    return arg[10];
}

void sub_BB70(char *arg)
{
    arg[11] = -1;
}

short sub_BB7C(char *arg)
{
    return (arg[2] << 8) | arg[3];
}

void sub_BB90(char *arg0, int arg1)
{
    arg0[3] = arg1 & 0xFF;
    arg0[2] = (arg1 >> 8) & 0xFF;
}

short sub_BBA0(int arg0, int arg1)
{
    t2 = *(int*)(arg0 + 60);
    t3 = *(int*)(t2 + 16);
    t1 = arg1 / t3;
    // BBC0
    if (t1 >= *(int*)(t2 + 8))
        return -1;
    t5 = *(int*)(arg0 + 52) + t1 * 28;
    v0 = *(int*)(t5 + 12) + (arg1 - *(short*)(t5 + 4)) * 2;
    return *(short*)v0;
}

int sub_BC04(int arg0, int arg1, int arg2)
{
    if (arg2 < *(short*)(arg1 + 6))
        return 0x80270001;
    s3 = arg2 & 0xFFFF;
    if (arg2 < *(short*)(arg1 + 10))
    {
        // BC78
        sub_F144(1);
        int ret = sub_F640(*(int*)(arg0 + 36) * (s3 + 64));
        sub_F188();
        if (ret < 0)
        {
            // BCC0
            sub_F144(1);
            sub_10750(*(int*)(arg0 + 36) * (s3 + 64));
            sub_F188();
            return ret;
        }
        else {
            sub_D500(arg0, arg1, arg2);
            return 0;
        }
    }
    return 0x80270001;
}

int sub_BCF0(int arg)
{
    v1 = *(int*)(arg + 60);
    *(int*)(arg + 52) = unkHugeTab;
    if (unkHugeTab == NULL)
        return 0x80270001;
    memset(unkHugeTab, 0, *(int*)(v1 + 8) * 28);
    a0 = unkMainVar;
    if (a0 != 0 && sub_BF64(a0) == 0) // BE1C
        return 0;
    // BD58
    unkMainVar = 0;
    s0 = *(int*)(arg + 52);
    int j;
    // BD78
    for (j = 0; j < *(int*)(*(int*)(arg + 60) + 8); j++)
    {
        v0 = sub_D9A4(arg, j, s0);
        s0 += 28;
        if (v0 == 0x80270001 || v0 != 0)
        {
            t5 = *(int*)(arg + 60);
            // BDD4
            a1 = *(int*)(arg + 52);
            v0 = *(int*)(t5 + 8);
            s0 = a1;
            int i;
            // BDEC
            for (i = 0; i < v0; i++)
            {
                if (*(int*)(s0 + 12) != 0)
                    *(int*)(s0 + 12) = 0;
                // BE00
                s0 += 28;
            }
            // BE08
            if (a1 != 0)
                *(int*)(arg + 52) = 0;
            return 0x80270001;
        }
    }
    return 0;
}

int sub_BE34(int arg)
{
    int someStruct = *(int*)(arg + 52);
    int i;
    for (i = 0; i < *(int*)( *(int*)(arg + 60) + 8 ); i++)
    {
        v0 = sub_DB8C(arg, someStruct);
        if (v0 < 0)
            return v0;
        someStruct += 28;
    }
    return 0;
}

int sub_BEB8(int arg0, int arg1, int arg2)
{
    return memcpy(*(int*)(arg0 + 12) + arg1 * *(int*)(arg0 + 28),
                    arg2, *(int*)(arg0 + 28));
}

int unkHugeTab[0x1080]; // 0x88613640
int unkHugeTab2[0x180]; // 0x88617840
int unkHugeTab3[0x1010]; // 0x886239C0
int unkHugeTab4[0x40]; // 0x88628A00
int unkHugeTab5[0x2000]; // 0x88628B00

int sub_BEEC(int arg)
{
    *(int*)(arg + 24) = unkHugeTab3;
    *(short*)(arg + 6) = -1;
    *(int*)(arg + 52) = unkHugeTab4;
    *(int*)(arg + 12) = unkHugeTab5;
    *(int*)(arg + 16) = unkHugeTab2;
    *(int*)(arg + 20) = unkHugeTab;
    *(short*)(arg + 4) = -1;
    return 0;
}

int sub_BF3C()
{
    return sub_C2DC(0, 0, 0, 0);
}

int sub_BF64(int arg)
{
    if (arg == 0)
        return 0x80270110;
    if (*(int*)arg != 0x5C2F430B)
        return 0x80270110;
    if (*(int*)(arg + 52) != 0x78A4DB53)
        return 0x80270110;
    if (*(int*)(arg + 60) == sub_E3C8(arg))
        return 0x80270111;
    unkMegaTabCount = 0;
    unkMegaTab[1] = 0;
    int i;
    int decal = 0;
    for (i = 4094; i >= 0; i--)
    {
        unkMegaTab[decal + 4] = &unkMegaTab[decal];
        decal += 12;
    }
    a3 = unkMegaTab[15];
    s4 = 4095;
    unkMegaTabCount = 4095;
    arg += 64;
    unkSmallTabPtr = unkSmallTab;
    s2 = lflashOpt.u52;
    if (*(int*)(a3 + 8) != 0)
    {
        int j;
        // C07C
        do
        {
            sub_D8F0(&lflashOpt, s2, s4);
            s3 = *(short*)arg;
            arg += 2;
            // C0C0
            int i;
            for (i = 0; i < s3; i++)
            {
                sub_D500(&lflashOpt, s2, *(short*)arg);
                arg += 2;
            }
            v0 = lflashOpt.u60;
            s4++;
            s2 += 28;
        } while (s4 < *(int*)(v0 + 8));
        return 0;
    }
    return 0;
}

int sub_C318(int arg)
{
    if (arg == 0)
    {
        // C3D4
        v0 = sub_DF30(&lflashOpt);
        return (v0 < arg) ? v0 : arg;
    }
    a0 = *(int*)(arg + 16);
    if (a0 == 0)
        return 0x80010013;
    if (*(int*)(a0 + 0) == 0)
        return 0x80010013;
    v1 = lflashOpt.u0;
    if (v1 & 1 == 0)
        return 0x80010013;
    // C380
    a3 = lflashOpt.u608 - 1;
    lflashOpt.u608 = a3;
    if (a3 <= 0)
    {
        // C3B0
        lflashOpt.u0 = v1 & 0xFFE4;
        v0 = sub_DF30(&lflashOpt);
        if (v0 < 0)
            return v0;
    }
    // C390
    if (lflashOpt.u64 != 0)
        lflashOpt.u64--;
    v0 = sub_D590(&lflashOpt);
    if (v0 < 0)
        return v0;
    return 0;
}

int sub_C3E4(int arg0, int arg1, int arg2)
{
    s1 = *(int*)(arg0 + 16);
    t6 = *(int*)(arg0 + 24) + arg2;
    a3 = t6 < arg2;
    t2 = *(int*)(arg0 + 28) + a3;
    t5 = (unsigned int)(t2 >> 31) >> 23;
    s0 = t6 + t5;
    t3 = s0 < t5;
    s4 = t6 & 0x1FF;
    t7 = t2 + t3;
    t3 = *(int*)(s1 + 12);
    t5 = s0 >> 9;
    t6 = t7 << 23;
    t1 = (s4 > 0) + (t5 | t6);
    if (t1 >= t3)
        return 0x8001000D;
    int cnt;
    // C4B0
    v0 = sub_E73C(arg0, &cnt);
    if (v0 < 0)
        return v0;
    v1 = lflashOpt.u28;
    if (arg2 & (v1 - 1)!= 0)
        return 0x80010016;

    // C4E8
    s0 = arg2 / v1;
    v0 = sub_D590(&lflashOpt);
    if (v0 < 0)
        return v0;

    // C504
    while (s0 > 0)
    {
        sub_CFBC(&lflashOpt, cnt, s1);
        cnt++;
        s1 += lflashOpt.u28;
        s0--;
    }
    v0 = *(int*)(arg0 + 24) + arg2;
    *(int*)(arg0 + 24) = v0;
    *(int*)(arg0 + 28) += (v0 < arg2);
    return arg2;
}

u64 sub_C6C0(int arg0, int arg1, int arg2, int arg3, int arg4)
{
    t4 = *(int*)(arg0 + 16);
    if (t0 == 1)
    {
        t2 = *(int*)(arg0 + 24) + arg2;
        a1 = t2 < a2;
        v0 = *(int*)(arg0 + 28) + a3;
        t3 = v0 + a1;
    }
    else
    {
        if (t0 >= 2)
        {
            if (t0 == 2)
                return 0xFFFFFFFF80010016;
            return 0xFFFFFFFF80010016;
        }
        // C708
        t2 = a2;
        t3 = a3;
        if (t0 != 0)
            return 0xFFFFFFFF80010016;
    }

    // C714
    if (t3 < 0)
        return 0xFFFFFFFF80010016;
    v1 = (int)t3 >> 31;
    t8 = (unsigned int)v1 >> 23;
    t5 = *(int*)(t4 + 12);
    t0 = (((t2 & 0x1FF) | t3) > 0) + ((t5 >> 9) | ((t3 + ((t2 + t8) < t8)) << 23));
    if (t0 >= t5)
        return 0xFFFFFFFF80010016;
    *(int*)(arg0 + 24) = t2;
    *(int*)(arg0 + 28) = t3;
    return (t3 << 32) | t2;
}

int sub_C7A0(int arg0, int arg1, int arg2, int arg3, int arg4, int arg5)
{
    int addr = *(int*)(arg0 + 16);
    if (arg1 != 0x3D001)
        return 0x80010016;
    if (arg5 < 68)
        return 0x80010016;
    if (arg4 == 0)
        return 0x80010016;

    bzero(arg4, 68);
    *(int*)(arg4 + 16) = *(int*)(addr + 4);
    *(int*)(arg4 + 20) = *(int*)(addr + 12);
    *(int*)(arg4 + 24) = lflashOpt.u28;
    *(int*)(arg4 + 28) = lflashOpt.u32;
    return 0;
}

int sub_CA80(int arg0, int arg1, int arg2, int arg3, int arg4)
{
    v0 = ~(arg4 << 1) + 1;
    v1 = (a3 >> (v0 & 0x1F)) | (a3 << (32 - (v0 & 0x1F)));
    v0 = a2 ^ v1;
    a0 = (lflashArg & (1 << (arg4 & 0x1F))) & 0xFF;
    v1 = 0;
    if (a0 != 0)
    {
        t0 = 0xC4536DE6;
        v1 = ~arg4 + 1;
        if (v0 == 0)
            v0 = (t0 >> (v1 & 0x1F)) | (t0 << (32 - (v1 & 0x1F)));
        // CAD0
        v1 = v0;
    }
    // CAD4
    *(int*)(a1 + 24) = v1;
    t1 = 0;
    if (a0 != 0)
    {
        t8 = arg4 * 3;
        t7 = (a3 >> (t8 & 0x1F)) | (a3 << (32 - (t8 & 0x1F)));
        t0 = 0xE3701A7B;
        a0 = a2 ^ t7;
        t1 = 0x3C22812A;
        if (arg4 != 3)
        {
            t1 = a0 ^ t0;
            if (t1 == 0)
                t1 = (t0 >> (arg4 & 0x1F)) | (t0 << (32 - (arg4 & 0x1F)));
        }
    }
    // CB18
    *(int*)(a1 + 28) = t1;
    if (t1 == 0)
        *(int*)(a1 + 24) = 0;
    return 0;
}

int sub_CB58()
{
    a0 = &lflashOpt;
    if (a0 == 0)
        return 0;
    if (*(int*)(a0 + 64) != 0)
        return sub_D590(a0); // CB8C
    return 0;
}

int sub_CBC0(int arg0, int arg1)
{
    return sub_DD24(arg0, arg1);
}

int sub_CBDC(int arg0, int arg1)
{
    return sub_E438(&lflashOpt, unkHugeTab4, arg0, arg1, 0x80010000);
}

int sub_CC10()
{
    if (unkNandFlags & 2 == 0)
        return 0x80010016;
    v0 = sub_C318(&nandOpt);
    if (v0 != 0)
        return v0;
    unkNandFlags &= 0xFFFFFFFD;
    return 0;
}

int sub_CC70(int arg)
{
    if (unkNandFlags & 2 == 0)
        return 0x80010016;
    return sub_C7A0(&nandOpt, 0x3D001, 0, 0, arg, 68);
}

u64 sub_CCC4(int arg0, int arg1, int arg2)
{
    if ((unkNandFlags & 2) != 0)
        return sub_C6C0(&nandOpt, arg1, arg0, arg1, arg2);
    else
        return 0x80010016;
}

int sub_CD28(int arg0, int arg1)
{
    if (unkNandFlags & 2 == 0)
        return 0x80010016;
    return sub_C3E4(&nandOpt, arg0, arg1);
}

void sub_CD78()
{
    return 0x80010086;
}

int sub_CFBC(int arg0, int arg1, int arg2)
{
    s5 = sub_D1B4(arg0, arg1);
    s1 = *(int*)(arg0 + 36);
    // D004
    a0 = *(int*)(arg0 + 44);
    short a1 = -1;
    if (arg1 < a0)
    {
        // D024
        a1 = arg1 / s1;
    }
    // D02C
    short v1 = -1;
    if (a1 != -1)
        v1 = sub_BBA0(arg0, a1);

    // D044
    if (v1 == -1)
        s0 = 0x80270001;
    else
        s0 = v1 * s1 + (arg1 % s1);

    // D064
    if (sub_F144(0) >= 0)
        sub_F8DC(s5);

    // D080
    s0 = sub_F958(s0 + (*(int*)(arg0 + 36) << 6), arg2, sp, 1);
    sub_F188();
    if (s0 == 0x80230009)
        return 0x80230009;
    else if (s0 == 0x80230003)
        return 0x80230003;
    else if (s0 == 0) // D0F4
        return 0;
    return s0;
}

void sub_D1B4(int arg0, int arg1)
{
    int i;
    int addr = arg0 + 96;
    for (i = 0; i < 16; i++)
    {
        if (*(int*)(addr + 4) != 0 && arg1 >= *(int*)(addr + 8))
        {
            if (arg1 < (a2 + *(int*)(a0 + 36)))
                return *(int*)(addr + 24);
            if (arg1 < (a2 + *(int*)(addr + 12)))
                return *(int*)(addr + 28);
        }
        addr += 32;
    }
    return 0;
}

int sub_D224(int arg0, short arg1, int arg2)
{
    if (sub_F144(0) >= 0)
        sub_F8DC(arg2);

    // D268
    s0 = sub_1047C(*(int*)(arg0 + 36) * (arg1 + 64), *(int*)(arg0 + 12), *(int*)(arg0 + 16));
    sub_F188();
    if (s0 < 0)
    {
        // D2B0
        if (s0 == 0x80230003) { // D2E0
            sub_BB7C(*(int*)(arg0 + 16));
            return 0x80230003;
        }
        if (s0 == 0x80230009) { // D2CC
            sub_BB7C(*(int*)(arg0 + 16));
            return 0x80230009;
        }
        return s0;
    }
    return 0;
}

int sub_D374()
{
    int i = 0;
    char data;
    for (;;)
    {
        sub_F144(0); // Disable writing to NAND (with test)
        i++;
        data = sub_F8A8(); // Get the read status
        sub_F188(); // Set some global variable
        if (data < 0)
            return 0;
        if (i >= 6)
            return 0x80270001;
        sub_F144(1); // Enable writing to NAND (with test)
        sub_F0D4(0); // Enable writing to NAND (without test)
        sub_F188(); // Set the global variable again
    }
}

int sub_D3F4(int arg0, int arg1, int arg2)
{
    int ret;
    int i;
    s5 = sub_D1B4(arg0, (arg2 & 0xFFFF) * *(int*)(arg0 + 36));
    s0 = (arg1 & 0xFFFF) * *(int*)(arg0 + 36);
    s1 = *(int*)(arg0 + 16);
    for (i = 0; i < *(int*)(arg0 + 36); i++)
    {
        // D4B8
        sub_BB34(s1);
        sub_BB48(s1);
        sub_BB5C(s1);
        sub_BB70(s1);
        sub_BB90(s1, s4);
        i++;
        s1 += 12;
    }
    // D450
    v0 = sub_F144(1);
    if (v0 >= 0)
        v0 = sub_F8DC(s5);
    // D46C
    ret = sub_103C8(s0 + (*(int*)(arg0 + 36) << 6), *(int*)(arg0 + 12), *(int*)(arg0 + 16));
    sub_F188();
    if (ret < 0)
        return ret;
    return 0;
}

int sub_D500(int arg0, int arg1, short arg2)
{
    a3 = unkSmallTabPtr;
    if (a3 == 0)
        return 0x80270001;
    unkSmallTabPtr = *(int*)(a3 + 4);
    unkMegaTabCount--;
    // D534
    *(int*)(a3 + 4) = 0;
    *(int*)(a3 + 0) = 0;
    *(short*)(a3 + 8) = arg2;
    v0 = *(int*)(arg1 + 24);
    if (*(int*)(arg1 + 20) != 0 || v0 != 0)
    {
        *(int*)(a3 + 0) = v0;
        // D580
        a3 = *(int*)(arg1 + 24);
        *(int*)(a3 + 4) = a3;
        *(int*)(arg1 + 24) = a3;
    }
    else {
        *(int*)(arg1 + 24) = a3;
        *(int*)(arg1 + 20) = a3;
    }
    // D564
    *(int*)(arg1 + 16)++;
    return 0;
}

int sub_D590(int arg)
{
    v1 = *(short*)(arg + 4);
    if (v1 != 0xFFFF)
    {
        s1 = sub_D1B4(arg, v1 * *(int*)(arg + 36));
    
        int i;
        // D5E4
        for (i = 0; i < 12; i++)
            *(char*)(sp + i) = -1;
    
        *(char*)(sp + 10) &= 0xEF;
        if (sub_F144(1) >= 0)
            sub_F8DC(s1);
    
        // D62C
        sub_F974(*(int*)(arg + 36) * (*(short*)(arg + 6) + 64), 0, sp, 1);
        s4 = 0;
        sub_F188();
        t3 = *(int*)(arg + 60);
        // D664
        a2 = *(short*)(arg + 6) / *(int*)(t3 + 12);
        if (a2 < *(int*)(t3 + 8))
        {
            // D680
            s4 = *(int*)(arg + 52) + a2 * 28;
        }
    
        // D698
        if (s4 == 0)
            return 0x80270001;
        // D6A4
        for (;;)
        {
            v0 = sub_E83C(arg, s4);
            s1 = v0;
            s3 = v0 & 0xFFFF;
            if (v0 == 0xFFFF)
                return 0x80270001;
            sub_F144(1);
            s0 = sub_F640(*(int*)(arg + 36) * (s3 + 64));
            sub_F188();
            if (s0 >= 0 && sub_D3F4(arg, s1, *(short*)(arg + 4)) >= 0)
                break;
    
            // D710
            sub_F144(1);
            sub_10750(*(int*)(arg + 36) * (s3 + 64));
            sub_F188();
        }
        // D740
        sub_E7E8(s4, *(short*)(arg + 4), s1);
        sub_BC04(arg, s4, *(short*)(s2 + 6));
    }

    // D760
    *(short*)(arg + 6) = -1;
    *(short*)(arg + 4) = -1;
    return 0;
}

int sub_D794(int arg0, int arg1, int arg2)
{
    s6 = sub_D1B4(arg0, arg1);
    short v1 = -1;
    if (arg1 < *(int*)(arg0 + 44)) {
        // D7F4
        v1 = arg1 / *(int*)(arg0 + 36);
    }

    // D7FC
    if (v1 == -1)
        return 0x80270001;
    if (*(short*)(arg0 + 4) != v1)
    {
        v0 = sub_D590(arg0);
        if (v0 < 0)
            return v0;
        short s3 = sub_BBA0(arg0, v1);
        if (s3 == -1)
            return 0x80270001;
        v0 = sub_D224(arg0, s3, s6);
        if (v0 != 0x80230003 && v0 != 0x80230003 && v0 < 0) // D8BC, D8D8
            return v0;
        *(short*)(arg0 + 4) = v1;
        // D860
        *(short*)(arg0 + 6) = s3;
        if (v0 < 0)
            return v0;
    }
    // D86C, D880
    sub_BEB8(arg0, arg1 % *(int*)(arg0 + 36), arg2);
    return 0;
}

int unkVar3 = 0x80010000; // 0x88612A50

int sub_D8F0(int arg0, int arg1, int arg2)
{
    t0 = *(int*)(arg0 + 60);
    *(int*)(arg1 + 0) = arg2;
    t3 = *(int*)(t0 + 16);
    t2 = *(int*)(t0 + 12);
    a3 = t3 * arg2;
    a0 = t2 * arg2;
    *(short*)(arg1 + 8) = a3 + t3;
    *(short*)(arg1 + 10) = a0 + t2;
    *(short*)(arg1 + 4) = a3;
    *(short*)(arg1 + 6) = a0;
    if (*(int*)(arg1 + 12) == 0)
    {
        *(int*)(arg1 + 12) = unkVar3 + a3 * 2;
        if (arg2 == 0)
            return 0x80270001;
    }

    // D950
    if (unkMainVar == 0 && *(int*)(t0 + 16) != 0)
    {
        a0 = *(int*)(arg1 + 12);
        // D974
        do
        {
            v0 = (v0 + 1) & 0xFFFF;
            *(short*)(a0 + 0) = -1;
            a0 += 2;
        } while (v0 < a2);
    }
    *(short*)(arg1 + 24) = 0;
    // D990
    *(short*)(arg1 + 16) = 0;
    *(short*)(arg1 + 20) = 0;
    return 0;
}

int sub_D9A4(int arg0, int arg1, int arg2)
{
    sub_D8F0(arg0, arg2, arg1);
    s5 = 0xFFFF;
    short i;
    for (i = *(short*)(arg2 + 6); i < *(short*)(arg2 + 10); i++)
    {
        sub_F144(0);
        sub_F8DC(0);
        s4 = sub_106C4(*(int*)(arg0 + 36) * (i + 64));
        sub_F188();
        if (s4 == 0)
        {
            v1 = *(int*)(arg0 + 36);
            // DA64
            char gotoDB64 = 0;
            char gotoDA28 = 0;
            char sp[16];
            v0 = sub_E8C8(arg0, i * v1, sp, 0);
            if (v0 == 0x80230003)
                gotoDB64 = 1;
            else
            {
                v1 = sp[10];
                if (v1 & 0x80 == 0 || v1 & 0x60 != 0x60)
                    gotoDA28 = 1;
                else
                {
                    v0 = sub_BB7C(sp);
                    s0 = v0;
                    if (v0 == s5
                     || *(short*)(arg2 + 4) > v0
                     || *(short*)(arg2 + 8) <= v0)
                        gotoDB64 = 1;
                    else
                    {
                        a1 = 0xFFFF;
                        if (arg2 != 0)
                        {
                            t7 = (v0 - *(short*)(arg2 + 4)) * 2 + *(int*)(arg2 + 12);
                            a1 = *(short*)t7;
                        }
                        // DAF4
                        s4 = a1 & 0xFFFF;
                        if (s4 != s5)
                        {
                            char sp2[16];
                            a3 = *(int*)(arg0 + 36);
                            sub_E8C8(arg0, s4 * a3, sp2, 0);
                            v1 = sp[10] & 0x10;
                            if ((v1 == (sp2[10] & 0x10) && v1 != 0) || (i < s4)) // DB7C
                                gotoDB64 = 1;
                            else {
                                // DB40
                                sub_BC04(arg0, arg2, s4);
                            }
                        }
                    }
                }
            }
            if (gotoDB64) {
                // DB64
                sub_BC04(arg0, arg2, i);
            }
            else if (!gotoDA28) {
                // DB54
                sub_E7E8(arg2, s0, i);
            }
        }
        // DA28
    }
    return 0;
}

int sub_DB8C(int arg0, int arg1)
{
    if (*(int*)(arg1 + 16) == 0)
        return 0x8001001C;
    // DBE4
    short i;
    for (i = *(short*)(arg1 + 4); i < *(short*)(arg1 + 8); i++)
    {
        v0 = 0xFFFF;
        if (arg1 != 0)
        {
            t0 = *(int*)(arg1 + 12) + (i - *(short*)(arg1 + 4)) * 2;
            v0 = *(short*)t0;
        }
        // DC08
        s2 = v0;
        if (s2 == 0xFFFF)
        {
            // DC5C
            v0 = sub_E83C(arg, arg1);
            s3 = v0;
            s5 = v0 & 0xFFFF;
            if (v0 == s2)
                return 0x8001001C;
            sub_D224(arg, v0, 0);
            sub_F144(1);
            sub_F640(*(int*)(arg + 36) * (s5 + 64));
            sub_F188();
            v0 = sub_D3F4(arg, s3, i);
            if (v0 != 0)
            {
                // DCF4
                sub_F144(1);
                sub_10750(*(int*)(arg + 36) * (s5 + 64));
                sub_F188();
                return v0;
            }
            sub_E7E8(arg1, i, s3);
            if (*(int*)(arg1 + 16) == 0)
                return 0x80270001;
            a2 = *(short*)(arg1 + 8);
        }
        // DC14
    }
    return 0;
}

int sub_DD24(int arg0, int arg1)
{
    if (sub_113F0() > 0x4FFFFF)
        return 0x1020000;
    if (((arg1 >> 30) & 1) == 1 || ((arg1 >> 28) & 1) == 1)
    {
        if ((arg0 >> 31) == 1)
            return 0x1010000;
        if (((arg0 >> 30) & 1) == 0)
            return 0x2020000;
        else
            return 0x1020000;
    }
    return 0x1010000;
}

void sub_DF30(int arg)
{
    sub_D590(arg);
    if (*(int*)(arg + 64) != 0)
        *(int*)(arg + 64)--;
    return 0;
}

int sub_E3C8(int arg)
{
    int num = 0;
    // E3D4
    int i;
    for (i = 0; i < 56; i += 4)
        num += *(int*)(arg + i);
    a1 = *(int*)(a0 + 8);
    v0 = *(int*)(a0 + 56);
    v1 = *(short*)v0;
    for (i = 0; i < a1; i += 2)
        num += v1;
    // E410
    v1 = *(int*)(a0 + 12);
    a0 = *(short*)(a0 + 64);
    // E420
    for (i = 0; i < v1; i += 2)
        num += a0;
    return num;
}

int sub_E438(int arg0, int arg1, int arg2, int arg3, int arg4)
{
    t4 = arg2 + 64;
    if (arg3 < t4)
        return 0x80270112;
    *(int*)(arg2 + 0) = 0x5C2F430B;
    t2 = *(int*)(arg0 + 56);
    t3 = *(int*)(arg0 + 60);
    *(int*)(arg0 + 56) = arg4;
    a2 = t4;
    *(int*)(arg2 + 4) = t2;
    *(int*)(arg2 + 52) = 0x78A4DB53;
    *(int*)(arg2 + 8) = *(int*)(t3 + 16) * *(int*)(t3 + 8) * 2;
    if (*(int*)(t3 + 8) != 0)
    {
        t0 = arg2 + 20;
        // E4B0
        do
        {
            *(int*)t0 = *(int*)(a1 + 16);
            if (a3 < a2 + 8)
                return 0x80270112;
            a0 = *(int*)(a1 + 20);
            t8 = a2 + 2;
            *(short*)a2 = t2;
            a2 = t8 + 2;
            t1 = a2 + *(int*)(a1 + 16) * 2;
            *(short*)t8 = *(short*)(a1 + 16);
            if (a3 < t1)
                return 0x80270112;
            if (a0 != 0)
            {
                do
                {
                    t5 = *(short*)(a0 + 8);
                    a0 = *(int*)(a0 + 4);
                    *(short*)a2 = t5;
                    a2 += 2;
                } while (a0 != 0);
            }
            // E520
            t2++;
            t0 += 4;
            a1 += 28;
        } while (t2 < *(int*)(t3 + 8));
    }
    // E534
    a1 = a2 - t4;
    *(int*)(arg2 + 16) = a1 + 64;
    *(int*)(arg2 + 12) = a1;
    *(int*)(arg2 + 60) = sub_E3C8(arg2);
    return *(int*)(arg2 + 16);
}

int sub_E73C(int arg0, int arg1)
{
    t0 = *(int*)(arg0 + 16);
    a2 = &lflashOpt;
    if (t0 == 0)
        return 0x80010013;
    a1 = *(int*)t0;
    if (a1 == 0)
        return 0x80010013;
    v1 = lflashOpt.u0;
    if (v1 & 1 == 0)
        return 0x80010013;
    if (v1 & 2 != 0)
        return 0x80010005;
    if (v1 & 8 != 0)
        return 0x80010016;
    t5 = *(int*)(a2 + 32);
    t6 = *(int*)(a0 + 24);
    t7 = *(int*)(a0 + 28);
    t4 = *(int*)(t0 + 8);
    t0 = t5 << 26;
    if (t0 >= 0)
    {
        // E7C0
        v0 = (unsigned int)t6 >> (t5 & 0x1F);
        if (t0 != 0)
        {
            t0 = -t5;
            t0 = t7 << (t0 & 0x1F);
            v0 |= t0;
        }
        // E7D4
        v1 = t7 >> t5;
    }
    else {
        v0 = t7 >> (t5 & 0x1F);
        v1 = t7 >> 31;
    }
    // E7D8
    *(int*)arg1 = t4 + v0;
    return v1 << 32; // ?
}

int sub_E7E8(int arg0, int arg1, short arg2)
{
    if (arg0 == 0)
        return 0x80270001;
    short v = *(short*)(arg0 + 4);
    if (arg1 < v)
        return 0x80270001;
    if (arg1 >= *(short*)(arg0 + 8))
        return 0x80270001;
    *(short*)( *(int*)(arg0 + 12)    +    (arg1 - v) * 2 ) = arg2;
    return 0;
}

int sub_E83C(int arg0, int arg1)
{
    v1 = *(int*)(arg1 + 16);
    a3 = v1 - 1;
    if (v1 == 0)
        return 1;
    a0 = *(int*)(arg1 + 20);
    v0 = unkHugeTab3;
    t1 = a0 < v0;
    v0 = *(int*)(a0 + 4);
    v1 = unkMegaTab;
    t0 = a0 < v1;
    a2 = *(short*)(a0 + 8);
    v1 = a0;
    if (v0 != 0) {
        *(int*)v0 = 0;
        v1 = *(int*)(arg1 + 20);
    }

    // E880
    *(int*)(arg1 + 16) = a3;
    *(int*)(arg1 + 20) = *(int*)(v1 + 4);
    if (a3 == 0) {
        *(int*)(arg1 + 24) = 0;
        *(int*)(arg1 + 20) = 0;
    }
    
    // E898
    if (t0 != 0)
        return a2;
    if (t1 != 0)
        return a2;
    *(int*)(a0 + 4) = unkSmallTabPtr;
    unkMegaTabCount++;
    unkSmallTabPtr = a0;
    return a2;
}

int sub_E8C8(int arg0, int arg1, int arg2, int arg3)
{
    char data[16];
    if (sub_F144(0) >= 0)
        sub_F8DC(arg3);
    // E908
    int ret = sub_F72C(arg1 + *(int*)(arg0 + 36) * 64, data, 1);
    sub_F188();
    memcpy(arg2, &data[4], 12);
    if (ret < 0)
    {
        if (ret == 0x80230003)
            return 0x80230003;
        if (ret == 0x80230009)
            return 0x80230009;
        return ret;
    }
    return 0;
}

int sub_EA54(int arg0, int arg1, int arg2, int arg3)
{
    if (arg0 == 0)
    {
        // EAA8
        arg0 = sub_DD24(arg2, arg3);
    }
    // EA74
    if (arg0 == 0x2000000)
    {
        if (arg1 ^ 0x1000 != 0)
            return 0x1010000;
        else
            return 0x1020000;
    }
    return arg0;
}

int sub_EEA8(int arg0, int arg1)
{
    *(int*)arg1 = 0;
    a1 = *(char*)arg0;
    t3 = 10;
    t2 = 0;
    if ((a1 != '-') && (a1 != '+'))
    {
        // EFC4
        a3 = *(char*)arg0;
    }
    else
    {
        a2 = a1;
        // EEE0
        do
        {
            arg0++;
            a3 = *(char*)arg0;
            v1 = a3;
            if (a2 != '-')
                t2 ^= 1;
            a2 = v1;
        } while ((v1 == '-') || (v1 == '+'));
    }

    // EF14
    if (a3 == '0')
    {
        v0 = *(char*)(arg0 + 1);
        // EF94
        if (v0 != 0)
        {
            a3 = v0;
            t3 = 8;
            arg0++;
            if (v0 == 'x')
            {
                arg0++;
                a3 = *(char*)arg0;
                t3 = 16;
            }
        }
    }
    // (EF24)
    // EF28
    t1 = 0;
    if (a3 == 0)
        return arg0;
    // EF30
    do
    {
        int cipher = (a3 - '0') & 0xFF; // a1
        int letter = (a3 - 'a') & 0xFF; // a0
        a1 = a0 < 6;
        a0 = a3 - '0';
        if (cipher >= 10)
        {
            a0 = a3 - 'a' + 10;
            if (a1 == 0)
                break;
        }
        // EF5C
        if (a0 >= t3)
            break;
        arg0++;
        a2 = *(char)arg0;
        t1 = t1 * t3 + a0;
        a3 = a2;
    } while (a2 != 0);
    // EF80
    t3 = -t1;
    if (t2 != 0)
        t1 = t3;
    *(int*)arg1 = t1;
    return arg0;
}

int unkVar90A4; // 0x886390A4
int sub_F188()
{
    unkVar90A4 = 0;
    return 0;
}

int sub_F28C(int arg0, int arg1, int arg2, int arg3)
{
    int ret;
    if (arg3 >= 33)
        return 0x80000104;
    if (((arg0 & 0x1F) + arg3) >= 33)
        return 0x80230008;
    if ((arg1 | arg2) & 3 != 0)
        return 0x80000103;
    nandOpt2.u8 = arg3;
    nandOpt2.u16 = arg1;
    nandOpt2.u20 = arg2;
    nandOpt2.u24 = arg3;
    nandOpt2.u4 = 0;
    nandOpt2.u12 = arg0;
    if ((ret = sub_F0C4()) == 0)
    {
        // F40C
        *(int*)(0xBD101038) &= 0xFDFC;
        if ((ret = sub_F0C4()) == 0) {
            // F450
            nandOpt2.u0 = 1;
        }
        else
            *(int*)(0xBD101038) = (*(int*)(0xBD101038) & 0xFFFFFFFE) | 0x0202;
    }

    if (ret != 0)
    {
        // F310
        nandOpt2.u0 = 2;
        if (arg3 & 0x10 == 0)
            v0 = *(int*)(0xBD101000) | 0x10000; // F3F8
        else
            v0 = *(int*)(0xBD101000) & 0xFFFEFFFF;

        // F328
        *(int*)(0xBD101000) = v0;
        *(int*)(0xBD101020) = arg0 << 10;
        *(int*)(0xBD101038) &= 0xFEFC;
        *(int*)(0xBD101024) = 0x0301;
    }

    // F360
    SYNC();
    // F374
    while ((nandOpt2.u0 != -1))
    {
        t6 = *(int*)(0xBD101038);
        if (((t6 >> 8) & t6) & 3 != 0)
        {
            // F3E8
            sub_FC40(/*0, 0, 0*/);
        }
        // F39C
    }

    // F3A4
    a0 = nandOpt2.u4;
    if (a0 & 1 != 0)
        return 0x80230003;
    if (a0 & 2 == 0)
        return 0x80230009;
    return 0;
}

int sub_F458(int arg0, int arg1, int arg2, int arg3, int arg4)
{
    v1 = arg3 < 33;
    s4 = arg1;
    s3 = arg0;
    s2 = arg4;
    s1 = arg2;
    s0 = arg3;
    if (v1 == 0)
        return 0x80000104;
    if ((arg0 & 0x1F) + s0 >= 33)
        return 0x80230008;
    if (arg2 & 3 != 0)
        return 0x80000103;
    if ((arg1 == 0) && ((arg4 & 0x10) == 0)) // F62C
        return 0x80000107;

    // F4CC
    if ((arg2 == 0) && ((arg4 & 0x20) == 0)) // F618
        return 0x80000107;

    // F4D4
    while (sub_F0C4() == 0)
    {
        v1 = 999;
        // F4E8
        while ((--v1) >= 0)
            ;
    }

    // F4F8
    nandOpt2.u8 = s0;
    nandOpt2.u24 = s2;
    nandOpt2.u4 = 0;
    nandOpt2.u12 = s3;
    nandOpt2.u16 = s4;
    nandOpt2.u20 = s1;
    if (sub_F0C4() == 0)
    {
        // F5CC
        *(int*)(0xBD101038) &= 0xFDFC;
        if (sub_F0C4() == 0) {
            // F610
            nandOpt2.u0 = 2;
        }
        else
        {
            *(int*)(0xBD101038) = (*(int*)(0xBD101038) & 0xFFFFFFFE) | 0x202;
            // F524
            nandOpt2.u0 = 4;
            sub_F9D0(s2, s3, s4, s1);
        }
    }
    else
    {
        // F524
        nandOpt2.u0 = 4;
        sub_F9D0(s2, s3, s4, s1);
    }

    // F53C
    SYNC();
    // F550
    while (nandOpt2.u0 != -1)
    {
        v0 = *(int*)(0xBD101038);
        if (((v0 >> 8) & v0) & 3 != 0) {
            // F5BC
            sub_FC40(/*0, 0, 0*/);
       }
    }

    // F580
    if (nandOpt2.u4 != 0)
        return 0x80230005;
    return 0;
}

int sub_F640(int arg)
{
    // F650
    while (sub_F0C4() == 0)
    {
        int i = 999;
        // F664
        while ((--i) >= 0)
            ;
    }

    // F674
    *(0xBD101008) = 0x60;
    *(0xBD10100C) = arg << 10;
    *(0xBD101038) &= 0xFFFFFFFE;
    *(0xBD101008) = 0xD0;

    while (sub_F0C4() == 0)
    {
        int i = 999;
        while ((--i) >= 0)
            ;
    }

    // F6D0
    *(0xBD101008) = 0x70;
    char data = *(0xBD101300);
    *(0xBD101014) = 1;
    if (data < 0)
    {
        // F71C
        if (v1 & 1 == 0)
            return 0;
        else
            return 0x80230004;
    }
    return 0x80230007;
}

int sub_F8DC(int arg)
{
    unkVar90A4 = arg;
    return 0;
}

int sub_F8EC(int arg0, int arg1, int arg2, int arg3)
{
    int ret = sub_F28C(arg0, arg1, arg2, arg3, 0);
    if ((arg2 == 0) && (ret == 0x80320009))
        return ret;
    return 0;
}

int sub_F930(int arg0, int arg1, int arg2, int arg3)
{
    char v = 0;
    if (arg1 == 0)
        v |= 0x10;
    if (arg2 == 0)
        v |= 0x20;
    return sub_F458(arg0, arg1, arg2, arg3, v);
}

int sub_F958(int arg0, int arg1, int arg2, int arg3)
{
    return sub_F28C(arg0, arg1, arg2, arg3, 0x20);
}

int sub_F974(int arg0, int arg1, int arg2, int arg3)
{
    return sub_F458(arg0, arg1, arg2, arg3, (arg1 == 0) ? 48 : 32);
}

int sub_F998(int arg0, int arg1, int arg2, int arg3)
{
    return sub_F28C(arg0, arg1, arg2, arg3, 0x31);
}

int sub_F9D0(int arg0, int arg1, int arg2, int arg3)
{
    t6 = arg0;
    s0 = arg1;
    cache(0x18, 0x9FF00000);
    cache(0x18, 0x9FF00040);
    cache(0x18, 0x9FF00080);
    cache(0x18, 0x9FF000C0);
    cache(0x18, 0x9FF00100);
    cache(0x18, 0x9FF00140);
    cache(0x18, 0x9FF00180);
    cache(0x18, 0x9FF001C0);
    if (a2 == 0)
    {
        // FC20
        v1 = -1;
        v0 = 127;
        // FC28
        do
        {
            v0--;
            *(int*)t5 = v1;
            t5 += 4;
        } while (v0 >= 0);
    }
    else
    {
        a0 = unkVar90A4;
        a1 = 0;
        if (a0 == 0)
        {
            // FBFC
            do
            {
                t3 = *(int*)a2;
                a1++;
                a0 = a1 < 128;
                *(int*)t5 = t3;
                a2 += 4;
                t5 += 4;
            } while (a0 != 0);
        }
        else
        {
            t8 = (a0 >> 21) | (a0 << 11);
            t2 = t8 << 3;
            v1 = t2 - t8;
            t1 = (s0 >> 17) | (s0 << 15);
            t0 = s0 ^ t8;
            t7 = t1 ^ v1;
            t5 = (t5 & 0xFFFFFE0F) | ((t0 << 4) & 0x1F0);
            a1 = a2 + 512;

            // FA3C
            do
            {
                t0 = *(int*)(a2 +  0);
                t1 = *(int*)(a2 +  4);
                t2 = *(int*)(a2 +  8);
                t3 = *(int*)(a2 + 12);
                a2 += 16;
                t4 = t0 + t7;
                t7 += t0;
                *(int*)(t5 +  0) = t4;
                t4 = t1 + t7;
                t7 ^= t1;
                *(int*)(t5 +  4) = t4;
                t4 = t2 + t7;
                t7 -= t2;
                *(int*)(t5 +  8) = t4;
                t4 = t3 + t7;
                t7 += t3;
                *(int*)(t5 + 12) = t4;
                t7 += t8;
                t5 += 16;
                t5 &= 0xFFFFFDFF;
                t7 = BITREV(t7);
            } while (a2 != a1);
        }
    }
    // FA98
    cache(0x1B, 0x9FF00000);
    cache(0x1B, 0x9FF00040);
    cache(0x1B, 0x9FF00080);
    cache(0x1B, 0x9FF000C0);
    cache(0x1B, 0x9FF00100);
    cache(0x1B, 0x9FF00140);
    cache(0x1B, 0x9FF00180);
    cache(0x1B, 0x9FF001C0);
    a2 = t6 & 0x10;
    t5 = t6 & 1;
    if (a2 == 0)
    {
        // FBE8
        t2 = *(int*)(0xBD101000);
        a0 = 0x20000;
        v0 = t2 | a0;
    }
    else
    {
        t9 = 0 < a3;
        t4 = t5 & t9;
        v0 = -1;
        if (t4 != 0)
        {
            v1 = *(char*)(a3 + 2);
            t1 = *(char*)(a3 + 1);
            a1 = *(char*)(a3 + 0);
            t8 = v1 << 16;
            t0 = t1 << 8;
            t7 = t8 | t0;
            v0 = t7 | a1;
        }

        // FAF0
        *(int*)(0xBFF00800) = v0;
        v0 = *(int*)(0xBD101000);
        v0 &= 0xFFFDFFFF;
    }

    // FB04
    *(int*)(0xBD101000) = v0;
    t3 = t6 & 0x20;
    if (a3 == 0)
    {
        // FBCC
        v0 = -1;
        *(int*)(0xBFF00900) = v0;
        *(int*)(0xBFF00904) = v0;
    }
    else
    {
        a2 = t6 & 1;
        t6 = a3 + 4;
        if (a2 != 0)
            a3 = t6;
        if (t3 == 0)
        {
            // FB8C
            int tab[2];
            tab[0] = *(int*)(a3 + 0);
            tab[1] = *(int*)(a3 + 4);
            v0 = sub_10DA0(tab) | 0xF000;
            *(int*)(0xBFF00900) = tab[0];
            *(int*)(0xBFF00904) = tab[1];
        }
        else
        {
            *(int*)(0xBFF00900) = *(int*)(a3 + 0);
            *(int*)(0xBFF00904) = *(int*)(a3 + 4);
            v0 = *(int*)(a3 + 8);
        }
    }
    // FB44
    *(int*)(0xBFF00908) = v0;
    *(int*)(0xBD101020) = s0 << 10;
    *(int*)(0xBD101038) &= 0xFEFC;
    *(int*)(0xBD101024) = 0x303;
    return 0;
}

int sub_FC40()
{
    a1 = *(int*)(0xBD101038);
    *(int*)(0xBD101038) = ((a1 & 0xFFFFFFFC) | ((a1 & (a1 >> 8)) & 3)) | 0x300;
    SYNC();
    switch (nandOpt2.u0)
    {
    case 3:
        // FE80
        v0 = sub_FF60(nandOpt2.u24, nandOpt2.u12, nandOpt2.u16, nandOpt2.u20);
        a0 = nandOpt2.u16;
        nandOpt2.u4 |= v0;
        nandOpt2.u12++;
        if (a0 != 0)
            nandOpt2.u16 = a0 + 0x200;
        // FEBC
        v1 = nandOpt2.u20;
        if (v1 != 0)
        {
            if (nandOpt2.u24 & 1 == 0)
                v0 = v1 + 12;
            else
                v0 = v1 + 16;

            // FEDC
            nandOpt2.u20 = v0;
        }
        // FEE4
        s0 = nandOpt2.u8 - 1;
        nandOpt2.u8 = s0;
        if (s0 > 0)
        {
            if (sub_F0C4() == 0)
            {
                // FF1C
                *(int*)(0xBD101038) &= 0xFDFC;
                if (sub_F0C4() == 0) {
                    nandOpt2.u0 = 1;
                    break;
                }
                *(int*)(0xBD101038) = (*(int*)(0xBD101038) & 0xFFFFFFFE) | 0x202;
            }
            // FF04
            nandOpt2.u0 = 3;
            v1 = nandOpt2.u24 & 0x10;
            a0 = nandOpt2.u12;
            /* /!\ TODO: multiple FD20 */
            // FD74 / FD34
            if (v1 == 0)
                *(int*)(0xBD101000) |= 0x10000;
            else
                *(int*)(0xBD101000) &= 0xFFFEFFFF;
            *(int*)(0xBD101020) = a0 << 10;
            *(int*)(0xBD101038) &= 0xFEFC;
            *(int*)(0xBD101024) = 0x0301;
        }
        break;

    case 4:
        // FD88
        a3 = nandOpt2.u16;
        nandOpt2.u4 |= (*(int*)(0xBD101028) > 0);
        nandOpt2.u12++;
        if (a3 != 0)
            nandOpt2.u16 = a3 + 0x200;
        // FDC8
        v1 = nandOpt2.u20;
        if (v1 != 0)
        {
            if (nandOpt2.u24 & 1 == 0)
                nandOpt2.u20 = v1 + 12; // FE78
            else
                nandOpt2.u20 = v1 + 16;
        }
        // FDF0
        a1 = nandOpt2.u8 - 1;
        nandOpt2.u8 = a1;
        if (a1 <= 0)
            break;
        if (sub_F0C4() == 0)
        {
            // FE2C
            *(int*)(0xBD101038) &= 0xFDFC;
            if (sub_F0C4() == 0) {
                nandOpt2.u0 = 2;
                break;
            }
            *(int*)(0xBD101038) = (*(int*)(0xBD101038) & 0xFFFFFFFE) | 0x202;
        }
        nandOpt2.u0 = 4;
        sub_F9D0(nandOpt2.u24, nandOpt2.u12, nandOpt2.u16, nandOpt2.u20);
        break;

    case 1:
        // FD0C
        a0 = nandOpt2.u12;
        v1 = nandOpt2.u24 & 0x10;
        nandOpt2.u0 = 3;
        /* /!\ TODO: multiple FD20 */
        // FD74 / FD34
        if (v1 == 0)
            *(int*)(0xBD101000) |= 0x10000;
        else
            *(int*)(0xBD101000) &= 0xFFFEFFFF;
        *(int*)(0xBD101020) = a0 << 10;
        *(int*)(0xBD101038) &= 0xFEFC;
        *(int*)(0xBD101024) = 0x0301;
        break;

    case 2:
        // FCE4
        nandOpt2.u0 = 4;
        sub_F9D0(nandOpt2.u24, nandOpt2.u12, nandOpt2.u16, nandOpt2.u20);
        break;
    }
    
    // FCBC
    if (nandOpt2.u8 == 0)
        nandOpt2.u0 = -1;
    return 0;
}

int sub_FF60(int arg0, int arg1, int arg2, int arg3)
{
    s2 = 0;
    if (arg2 != 0)
    {
        a3 = arg2;
        cache(0x19, 0x9FF00000);
        cache(0x19, 0x9FF00040);
        cache(0x19, 0x9FF00080);
        cache(0x19, 0x9FF000C0);
        cache(0x19, 0x9FF00100);
        cache(0x19, 0x9FF00140);
        cache(0x19, 0x9FF00180);
        cache(0x19, 0x9FF001C0);
        if ((0xD3 >> (arg2 >> 29)) & 1 != 0)
        {
            t0 = arg2 & 0x3F;
            if (t0 == 0)
                cache(0x18, arg2);
            // FFD4
            cache(0x18, arg2 + 0x040);
            cache(0x18, arg2 + 0x080);
            cache(0x18, arg2 + 0x0C0);
            cache(0x18, arg2 + 0x100);
            cache(0x18, arg2 + 0x140);
            cache(0x18, arg2 + 0x180);
            if (t0 == 0)
                cache(0x18, arg2 + 0x1C0);
        }

        // FFF8
        a0 = unkVar90A4;
        v1 = 127;
        if (a0 == 0)
        {
            // 10164
            do
            {
                v1--;
                *(int*)a3 = *(int*)t4;
                t4 += 4;
                a3 += 4;
            } while (v1 >= 0);
        }
        else
        {
            t6 = (a0 >> 21) | (a0 << 11);
            t3 = (t6 << 3) - t6;
            t0 = arg1 ^ t6;
            t2 = (arg1 >> 17) | (arg1 << 15);
            a2 = t2 ^ t3;
            t4 = (t4 & 0xFFFFFE0F) | ((t0 << 4) & 0x1F0);
            t5 = a3 + 512;
            // 10024
            do
            {
                t0 = *(int*)(t4 +  0);
                t1 = *(int*)(t4 +  4);
                t2 = *(int*)(t4 +  8);
                t3 = *(int*)(t4 + 12);
                t4 += 16;
                t0 -= a2;
                a2 += t0;
                *(int*)(a3 +  0) = t0;
                t1 -= a2;
                a2 ^= t1;
                *(int*)(a3 +  4) = t1;
                t2 -= a2;
                a2 -= t2;
                *(int*)(a3 +  8) = t2;
                t3 -= a2;
                a2 += t3;
                *(int*)(a3 + 12) = t3;
                a2 += t6;
                a3 += 16;
                t4 &= 0xFFFFFDFF;
                a2 = BITREV(a2);
            } while (a3 != t5);
        }
    }
    // 10080
    if (arg0 & 0x10 == 0 && *(int*)(0xBD101028) != 0)
        s2 = 1;
    // 10098
    *(int*)(sp +  0) = *(int*)(0xBFF00800);
    *(int*)(sp +  4) = *(int*)(0xBFF00900);
    *(int*)(sp +  8) = *(int*)(0xBFF00904);
    *(int*)(sp + 12) = *(int*)(0xBFF00908);
    if (arg0 & 0x20 == 0)
    {
        // 10134
        t7 = *(char*)(sp + 13);
        t3 = *(char*)(sp + 12);
        v0 = sub_10FE8(sp + 4, ((t7 << 8) | t3) & 0xFFF);
        if (v0 == 0xFFFD)
            s2 |= 2;
    }

    // 100D0
    if (arg3 != 0)
    {
        if (arg0 & 1 == 0)
        {
            // 1011C
            *(int*)(arg3 + 0) = *(int*)(sp +  4);
            *(int*)(arg3 + 4) = *(int*)(sp +  8);
            *(int*)(arg3 + 8) = *(int*)(sp + 12);
        }
        else
        {
            *(int*)(arg3 +  0) = *(int*)(sp +  0);
            *(int*)(arg3 +  4) = *(int*)(sp +  4);
            *(int*)(arg3 +  8) = *(int*)(sp +  8);
            *(int*)(arg3 + 12) = *(int*)(sp + 12);
        }
    }
    return s2;
}

