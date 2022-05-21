/* Copyright (C) 2011, 2012 The uOFW team
   See the file COPYING for copying permission.
*/
#include <common_header.h>

int sub_11860()
{
    return sub_11970(0x2000, 1); // enable EMCSM bus clock
}

int sub_9508(int arg0, char *flashName, char *lflashName, int arg3)
{
    a0 = *(int*)(arg0 + 4);
    s0 = arg3;
    if (a0 < 0)
        return SCE_ERROR_ERRNO_TOO_MANY_OPEN_SYSTEM_FILES
    if (a0 >= fatUnitsNumber)
        return SCE_ERROR_ERRNO_TOO_MANY_OPEN_SYSTEM_FILES;
    // 956C
    s1 = unkFatAddr2 + a0 * 628;
    if (*(int*)(s1 + 0) & 1 != 0)
        return SCE_ERROR_ERRNO_DEVICE_BUSY;
    bzero(s1, 628);
    if (s0 & 1 == 0)
        s0 = 0x4000003;
    else
        s0 = 0x4000001;
    *(int*)(s1 + 616) = 2;
    v1 = sub_C988(lflashName, s0, 0);
    *(int*)(s1 + 4) = v1;
    if (s0 & 2 == 0)
        *(int*)s1 |= 2;
    // 95F0
    if (v1 < 0)
        return v1;
    v0 = sub_9B34(s1);
    if (v0 < 0) {
        sub_CC10();
        return s0;
    }
    *(int*)(s1 + 36) = *(int*)(s1 + 24);
    v0 = sub_9BAC(s1, 11, sp, 2);
    if (v0 < 0) {
        sub_CC10();
        return v0;
    }
    v0 = sub_9BAC(s1, 13, sp + 2, 1);
    if (v0 < 0) {
        sub_CC10();
        return v0;
    }
    v0 = sub_9BAC(s1, 14, sp + 4, 2);
    if (v0 < 0) {
        sub_CC10();
        return v0;
    }
    v0 = sub_9BAC(s1, 16, sp + 6, 1);
    if (v0 < 0) {
        sub_CC10();
        return v0;
    }
    v0 = sub_9BAC(s1, 17, sp + 8, 2);
    if (v0 < 0) {
        sub_CC10();
        return v0;
    }
    v0 = sub_9BAC(s1, 19, sp + 10, 2);
    if (v0 < 0) {
        sub_CC10();
        return v0;
    }
    v0 = sub_9BAC(s1, 22, sp + 12, 2);
    if (v0 < 0) {
        sub_CC10();
        return v0;
    }
    t3 = *(short*)sp;
    a0 = 31 - clz(t3);
    if (t3 == (1 << (a0 & 0x1F)))
        a2 = a0;
    else
        a2 = -1;
    *(int*)(s1 + 32) = t3;
    *(int*)(s1 + 36) = a2;
    if (a2 - 9 >= 4) {
        sub_CC10();
        return SCE_ERROR_ERRNO_IO_ERROR;
    }
    a1 = *(char*)(sp + 2);
    s0 = clz(a1);
    t7 = t0 - s0;
    if (a1 == (1 << (t7 & 0x1F)))
        t6 = t7;
    else
        t6 = -1;
    if (t6 >= 8) {
        sub_CC10();
        return SCE_ERROR_ERRNO_IO_ERROR;
    }
    *(int*)(s1 + 76) = a1 << (a2 & 0x1F);
    *(int*)(s1 + 72) = a2;
    if (*(short*)(sp + 4) == 0) {
        sub_CC10();
        return SCE_ERROR_ERRNO_IO_ERROR;
    }
    v0 = *(char*)(sp + 6);
    *(int*)(s1 + 56) = v0;
    if (v0 < 2) {
        sub_CC10();
        return SCE_ERROR_ERRNO_IO_ERROR;
    }
    a1 = *(short*)(sp + 10);
    *(int*)(s1 + 64) = *(short*)(sp + 8);
    *(int*)(sp + 16) = a1;
    if (a1 == 0)
    {
        // 9994
        v0 = sub_9BAC(s1, 32, sp + 16, 4);
        if (v0 < 0) {
            sub_CC10();
            return v0;
        }
    }
    // 9788
    v1 = *(int*)(s1 + 24);
    a3 = *(int*)(s1 + 36);
    a0 = *(int*)(s1 + 16);
    if (a3 < v1) {
        // 9988
        a0 <<= ((v1 - a3) & 0x1F);
    }
    else
        a0 >>= ((a3 - v1) & 0x1F);
    // 97A4
    if (a0 < *(int*)(sp + 16)) {
        sub_CC10();
        return SCE_ERROR_ERRNO_IO_ERROR;
    }
    t7 = *(short*)(sp + 12);
    *(int*)(s1 + 52) = t7;
    if (t7 == 0)
    {
        v0 = sub_9BAC(s1, 36, sp + 20, 4);
        if (v0 < 0) {
            sub_CC10();
            return v0;
        }
        *(int*)(s1 + 52) = *(int*)(sp + 20);
    }
    // 97E8
    *(int*)s1 |= 8;
    v0 = sub_9BAC(s1, *(short*)sp - 2, sp + 14, 2);
    if (v0 < 0) {
        sub_CC10();
        return v0;
    }
    if (*(short*)(sp + 14) != 0xAA55) {
        sub_CC10();
        return SCE_ERROR_ERRNO_IO_ERROR;
    }
    // 9838
    v0 = *(short*)(sp + 4);
    *(int*)(s1 + 44) = v0;
    t5 = v0 + *(int*)(s1 + 52) * *(char*)(sp + 6);
    a1 = t5 + (*(int*)(s1 + 64) << 5) >> (*(int*)(s1 + 36) & 0x1F);
    *(int*)(s1 + 60) = t5;
    *(int*)(s1 + 68) = a1;
    // 9888
    a2 = (*(int*)(sp + 16) - a1) / *(char*)(sp + 2);
    if (*(int*)(s1 + 40) == 0)
    {
        v0 = 0xFFF;
        if (a2 >= 0xFF5)
        {
            v0 = 0xFFFF;
            if (a2 > 0xFFF4)
            {
                // 997C
                sub_CC10();
                return SCE_ERROR_ERRNO_IO_ERROR;
            }
        }
        // 98B4
        *(int*)(s1 + 40) = v0;
    }
    a1 = *(int*)(s1 + 40);
    // 98BC
    a0 = t0 << (a3 & 0x1F);
    *(int*)(s1 + 92) = -1;
    *(int*)(s1 + 96) = 2;
    *(int*)(s1 + 48) = a0;
    if (a1 == 0xFFF)
    {
        // 995C
        t5 = a0 << 1;
        // 9954
        *(int*)(s1 + 48) = (t5 * 0x55555556) - (t5 >> 31);
    }
    else if (a1 == 0xFFFF) {
        // 9954
        *(int*)(s1 + 48) = a0 >> 1;
    }
    // 98E8
    if (a2 < *(int*)(s1 + 48))
        *(int*)(s1 + 48) = a2;
    // 98F8
    if (*(int*)(s1 + 32) == 0x200)
    {
        // 992C
        v0 = sub_9BAC(s1, *(int*)(s1 + 44) << (a3 & 0x1F), s1 + 104, 512);
        if (v0 < 0) {
            sub_CC10();
            return v0;
        }
        *(int*)(s1 + 100) = *(int*)(s1 + 44);
    }
    else
        *(int*)(s1 + 100) = -1;
    // 9908
    if (s2 != 0)
        *(int*)(s1 + 0) |= 0x11;
    else
        *(int*)(s1 + 0) |= 1;
    // 9924
    return 0;
}

typedef struct
{
    int u0;
    int u60;
    int u96;
    int u608; // some counter
    // size: 620?
} LflashOpt;
LflashOpt lflashOpt; // 0x88638DE8

int sub_C28C()
{
    v0 = sub_DDA4(&lflashOpt); // initialize lflash
    if (v0 < 0)
        return v0;
    v0 = lflashOpt.u60;
    if (*(int*)(v0 + 8) < 4)
        return 0x80270001;
    return 0;
}

int unkHugeTab[1024]; // 0x886127A00

int sub_C2DC(int arg0, int arg1, int arg2, int arg3)
{
    int addr = unkHugeTab;
    if (addr == 0)
        return SCE_ERROR_ERRNO_NO_MEMORY;
    sub_E568(arg0, arg1, arg2, arg3, addr);
    return 0;
}

int lflashArg; // 0x88638DD4
int unkNandFlags; // 0x88638DE4
int sub_C934(int arg)
{
    if (unkNandFlags & 1 == 0)
    {
        lflashArg = arg;
        sub_C28C(/*sp*/); // initialize lflash
        unkNandFlags |= 1;
    }
    return 0;
}

FlashOpt nandOpt; // 0x88639050: unsure
int sub_C988(char *path, int arg1, int arg2)
{
    if (unkNandFlags & 2 != 0)
        return SCE_ERROR_ERRNO_DEVICE_BUSY;
    v0 = sub_C934(13); // if not already initialized, initialize lflash
    if (v0 != 0)
        return v0;
    sub_10D28();
    nandOpt.u0 = arg1;
    nandOpt.u4 = 0;
    nandOpt.u8 = 0;
    nandOpt.u12 = 0;
    nandOpt.u16 = 0;
    nandOpt.u20 = 0;
    nandOpt.u24 = 0;
    nandOpt.u28 = 0;
    nandOpt.u32 = 0;
    nandOpt.u36 = 0;
    sub_10D28();
    v0 = sub_C2DC(&nandOpt, strchr(path, ':') + 1, arg1, arg2);
    if (v0 != 0)
        return v0;
    unkNandFlags |= 2;
    return 0;
}

typedef struct
{
    int u0;
    int u4;
    int u8;
    int u12;
    int u16;
    int u20;
} SomeStruct;

SomeStruct someStruct[] = // 0x88612A04
{
    { 0x00000800, 0x000007C0, 0x00000004, 0x000001F0, 0x000001E0, 0x00000780 },
    { 0x00001000, 0x00000FC0, 0x00000008, 0x000001F8, 0x000001E0, 0x00000F00 },
    { 0x00001000, 0x00000F80, 0x00000008, 0x000001F0, 0x000001E0, 0x00000F00 }
};

int sub_DDA4(int arg)
{
    if (sub_EFCC() < 0)
    {
        // DF0C
        printf("Assertion failed at %s:%s:%04d", "lflash.c", "Initialize", 2713);
        for (;;)
            ;
    }
    sub_F144(0);
    sub_F8A8();
    sub_F188();
    sub_F144(0);
    *(int*)(arg + 28) = sub_10D1C();
    sub_F188();
    *(int*)(arg + 32) = 31 - CLZ(*(int*)(arg + 28));
    sub_F144(0);
    *(int*)(arg + 36) = sub_10D28();
    s1 = sub_10D34();
    sub_F188();
    s2 = sub_EA54(lflashArg & 0xFFFF0000, s1, sub_3880(), sub_3874());
    if (s2 == 0x1020000)
        *(int*)(arg + 56) = 1;
    else if (s2 == 0x2020000)
        *(int*)(arg + 56) = 2;
    else
        *(int*)(arg + 56) = 0;
    // DE84
    t4 = *(int*)(arg + 36);
    *(int*)(arg + 40) = *(int*)(arg + 28) * t4;
    SomeStruct *unk = &someStruct[*(int*)(arg + 56)];
    *(int*)(arg + 60) = unk;
    *(int*)(arg + 44) = unk->u4 * t4;
    *(int*)(arg + 64) = 0;
    *(int*)(arg + 48) = unk->u20 * t4;
    return 0;
}

int sub_E568(int arg0, int arg1, int arg2, int arg3, int arg4)
{
    int ptr1, ptr2, ptr3;
    v0 = sub_E984(&lflashOpt);
    if (v0 < 0)
        return v0;
    if (arg1 == 0)
        return SCE_ERROR_ERRNO_INVALID_ARGUMENT;
    v0 = sub_EDFC(arg1, &ptr1, &ptr2, &ptr3);
    if (v0 < 0)
        return SCE_ERROR_ERRNO_INVALID_ARGUMENT;
    if (ptr >= 16)
        return SCE_ERROR_ERRNO_DEVICE_NOT_FOUND;
    v1 = lflashOpt.u0;
    a3 = arg2 & 0x800;
    if (v1 & 1 == 0)
    {
        // E6A4
        if (a1 < 0)
        {
            // E70C
            t9 = lflashOpt.u48;
            t8 = &lflashOpt.u96;
            lflashOpt.u0 = v1 | 8;
            lflashOpt.u96 = &lflashOpt.u0
            *(int*)(t8 + 12) = t9;
            *(int*)(t8 + 28) = 0;
            ptr2 = 0;
            *(int*)(t8 + 4) = 0;
            *(int*)(t8 + 8) = 0;
            *(int*)(t8 + 24) = 0;
        }
        else
            sub_EAB8(&lflashOpt, arg4, ptr3);
        // E6B8
        a3 = ptr2;
        v1 = a3 << 5;
        a1 = v1 + arg0;
        v0 = *(int*)(a1 + 96);
        if (v0 == 0)
            return SCE_ERROR_ERRNO_DEVICE_NOT_FOUND;
        if (arg2 & 0x800 == 0)
            lflashOpt.u0 |= 1;
        else
            lflashOpt.u0 |= 0x11;
        *(int*)(arg0 + 16) = a1 + 96;
        lflashOpt.u608++;
        return 0;
    }
    if (a3 != 0)
        return SCE_ERROR_ERRNO_DEVICE_BUSY;
    if (v1 & 0x10 != 0)
        return SCE_ERROR_ERRNO_DEVICE_BUSY;
    // E640
    if (a1 < 0)
    {
        // E688
        if (v1 & 8 == 0)
            return SCE_ERROR_ERRNO_INVALID_ARGUMENT;
        ptr2 = 0;
        a1 = 0;
    }
    else if (v1 & 8 != 0)
        return SCE_ERROR_ERRNO_INVALID_ARGUMENT;
    // E654
    t7 = &lflashOpt + (a1 << 5);
    if (*(int*)(t7 + 96) == 0)
        return SCE_ERROR_ERRNO_DEVICE_NOT_FOUND;
    *(int*)(arg0 + 16) = t7 + 96;
    lflashOpt.u608++;
    return 0;
}

int unkMegaTab[4094 * 3 + 4]; // 0x886179C0
int unkSmallTab[2]; // 0x886239B4
int *unkSmallTabPtr; // 0x88638DD8
int unkMegaTabCount; // 0x88638DDC

int sub_E984(int arg)
{
    v0 = *(int*)(arg + 64);
    if (v0 == 0)
    {
        v0 = sub_BEEC(a0);
        if (v0 < 0)
            return v0;
        unkMegaTabCount = 0;
        unkMegaTab[1] = 0;
        int i, x;
        for (i = 4094, x = 0; i >= 0; i--, x += 3)
            unkMegaTab[x + 1] = &unkMegaTab[x];
        unkMegaTabCount = 4095;
        unkSmallTabPtr = unkSmallTab;
        sub_D374();
        v0 = sub_BCF0(arg);
        if (v0 != 0)
            return v0;
        v0 = sub_BE34(arg);
        if (v0 != 0)
            return v0;
    }
    *(int*)(arg + 64)++;
    return 0;
}

int sub_EAB8(int arg0, int arg1, int arg2)
{
    s6 = 0;
    s5 = sp;
    s1 = arg0 + 96;
    v0 = sub_11548();
    *(int*)(sp + 84) = v0 >> 32;
    *(int*)(sp + 80) = v0;
    if (sub_CFBC(arg0, 0, arg1) >= 0)
    {
        a2 = arg1 + 36;
        if (*(int*)(arg1 + 510) & 0xFFFF == 0xAA55) // unaligned reading
        {
            char gotoEB40 = 0;
            // EB90
            a3 = arg1 + 64;
            if (*(char*)(a2 + 2) == ')')
            {
                // EDA8
                int i;
                s0 = sp + 64;
                // EDB0
                for (i = 0; i < 8; i++)
                {
                    t3 = *(char*)(a2 + i + 18);
                    *(char*)(s0 + i) = (t3 != 0) ? t3 : ' ';
                }
                if (strncmp("FAT12   ", s0, 8) == 0 // 0x88612178
                 || strncmp("FAT16   ", s0, 8) == 0) // ED90 ; 0x88612184
                    gotoEB40 = 1;
            }
            else if (*(char*)(a3 + 2) == ')')
            {
                s0 = sp + 64;
                int i;
                // ED60
                for (i = 0; i < 8; i++)
                {
                    t9 = *(char*)(a3 + i + 18);
                    *(char*)(s0 + i) = (t9 != 0) ? t9 : ' ';
                }
                // ED90
                if (strncmp("FAT32   ", s0, 8) == 0) // 0x8861216C
                    gotoEB40 = 1;
            }
            if (!gotoEB40)
            {
                v1 = arg1 + 446;
                // EBB0
                *(int*)(sp + 88) = v1;
                s2 = 0;
                memcpy(sp, v1, 64);
                // EBC8
                char gotoEBE8 = 0;
                do
                {
                    a0 = *(char*)(s5 + 4);
                    if (a0 != 0)
                    {
                        // EBF8
                        if ((a0 == 5) || (a0 == 15))
                        {
                            // EC5C
                            s7 = *(int*)(s5 + 8); // unaligned reading
                            fp = s7;
                            if (sub_CFBC(arg0, s7, arg1) >= 0)
                            {
                                // EC7C
                                do
                                {
                                    if (*(int*)(arg1 + 510) & 0xFFFF != 0xAA55) // unaligned reading
                                        break;
                                    t9 = *(int*)(sp + 88);
                                    t1 = *(char*)(t9 + 4);
                                    if ((t1 == 5) | (t1 == 0))
                                        break;
                                    if (t1 == 15)
                                        break;
                                    *(int*)s1 = arg0;
                                    s6++;
                                    t3 = *(int*)(sp + 88);
                                    *(int*)(s1 + 4) = *(char*)(t3 + 4);
                                    *(int*)(s1 + 8) = fp + *(int*)(arg1 + 454); // unaligned reading
                                    *(int*)(s1 + 12) = *(int*)(arg1 + 458); // unaligned reading
                                    s1 += 32;
                                    sub_CA80(arg0, s1, *(int*)(sp + 80), *(int*)(sp + 84), s6);
                                    if (s6 >= 16) {
                                        gotoEBE8 = 1;
                                        break;
                                    }
                                    t6 = *(char*)(arg1 + 466);
                                    if ((t6 != 5) && (t6 != 15))
                                        break;
                                    fp = s7 + *(int*)(arg1 + 470); // unaligned reading
                                } while (sub_CFBC(arg0, fp, arg1) >= 0);
                            }
                        }
                        else
                        {
                            *(int*)(s1 + 4) = a0;
                            *(int*)(s1 + 12) = *(int*)(s5 + 12); // unaligned reading
                            *(int*)(s1 + 8) = *(int*)(s5 + 8); // unaligned reading
                            s6++;
                            *(int*)(s1 + 0) = arg0;
                            s1 += 32;
                            sub_CA80(arg0, s1, *(int*)(sp + 80), *(int*)(sp + 84), s6);
                        }
                    }
                    if (gotoEBE8)
                        break;
                    s2++;
                    // EBD8
                    s5 += 16;
                } while ((s2 < 4) && (s6 < 16));
                // EBE8
                if (s6 > 0)
                    return;
            }
        }
    }
    // EB40
    *(int*)(s1 + 4) = 0;
    *(int*)(s1 + 8) = 0;
    *(int*)(s1 + 0) = arg0;
    *(int*)(s1 + 28) = 0;
    *(int*)(s1 + 12) = *(int*)(arg0 + 48) - 1;
    *(int*)(s1 + 24) = 0;
    return;
}

void sub_EDFC(int arg0, int arg1, int arg2, int arg3)
{
    char *str;
    *(int*)arg3 = 0;
    *(int*)arg2 = 0;
    *(int*)arg1 = 0;
    str = sub_EEA8(arg0, arg1);
    if (str[0] == '\0')
        return 0;
    if (str[0] != ',')
        return -1;
    if (strcmp("single", &str[1]) != 0) // strcmp
    {
        // EE8C
        char *str2 = sub_EEA8(&str[1], arg2);
        if (str2[0] != '\0')
            return -1;
        return 0;
    }
    *(int*)arg3 = 1;
    return 0;
}

typedef struct
{
    int u0, u4, u8, u12, u16, u20, u24, u28, u32, u36, u40, u44;
} NandOpt;
NandOpt nandOpt2; // 0x88639078

int sub_EFCC()
{
    nandOpt2.u0 = 0;
    nandOpt2.u4 = 0;
    nandOpt2.u8 = 0;
    nandOpt2.u12 = 0;
    nandOpt2.u16 = 0;
    nandOpt2.u20 = 0;
    nandOpt2.u24 = 0;
    nandOpt2.u28 = 0;
    nandOpt2.u32 = 0;
    nandOpt2.u36 = 0;
    nandOpt2.u40 = 0;
    nandOpt2.u44 = 0;
    sub_11860(); // Enable EMCSM bus clock?
    sub_11994(); // Enable USB I/O??
    *(int*)(0xBD101038) = 0x303;
    if (sub_113F0() > 0x4FFFFF)
        nandOpt2.u40 = 0x0B060309; // F0BC
    else
        nandOpt2.u40 = 0x0B040205;
    // F054
    *(int*)(0xBD101200) = nandOpt2.u40;
    sub_F144(0);
    if (sub_F0C4() != 0)
    {
        // F0A4
        sub_F198(0);
        sub_10184();
    }
    else
        nandOpt2.u36 = 1;
    // F084
    sub_F188();
    return 0;
}

int sub_F0C4()
{
    return *(0xBD101004) & 1;
}

int sub_F0D4(int arg)
{
    int var = *(0xBD101004);
    int ret = ((*(0xBD101004) ^ 0x80) & 0x80) >> 7;
    if (arg == 0) // Disable writing to the NAND
    {
        // F0FC
        *(0xBD101004) = var | 0x80;
        // F108
        while ((*(0xBD101004) & 0x80) == 0)
            ;
        
        // F120
        int i = 0;
        while (i < 10)
        {
            int j = 999;
            while ((--j) >= 0)
                ;
            i++;
        }
    }
    else // Enable writing to the NAND
        *(0xBD101004) = var & 0xFFFFFF7F;
    return ret;
}

int unkNeverSet; // 0x8863909C
int sub_F144(int arg)
{
    if (unkNeverSet != 0)
        return 0x80230001;
    if (arg != 0)
        sub_F0D4(0); // Disable writing to NAND
    else
        sub_F0D4(1);
    return 0;
}

int sub_F198(int arg)
{
    *(int*)(0xBD101038) &= 0xFFFFFFFC;
    *(int*)(0xBD101008) = 0xFF; // Reset
    // F1C8
    while (sub_F0C4() == 0) // Wait NAND to be ready
    {
        int i = 999;
        while ((--i) >= 0)
            ;
    }
    *(int*)(0xBD101008) = 0x70; // Read status
    a0 = *(int*)(0xBD101300); // Get status
    if (arg != 0)
        *(char*)arg = a0;
    // F208
    *(int*)(0xBD101014) = 1; // Clear buffer?
    if (a0 & 1 == 0)
        return 0;
    else
        return 0x80230001;
}

int sub_F234(int arg0, int arg1)
{
    int i;
    *(int*)(0xBD101008) = 0x90;
    *(int*)(0xBD10100C) = 0;
    for (i = 0; i < arg1; i++) // F274
        if (arg0 != 0)
            *(char*)(arg0 + i) = *(int*)(0xBD101300) & 0xFF;
    // F27C
    *(int*)(0xBD101014) = 1;
    return 0;
}

int sub_F72C(int arg0, int arg1, int arg2)
{
    if (arg2 >= 33)
        return SCE_ERROR_INVALID_SIZE;
    if ((arg2 + (arg0 & 0x1F)) >= 33)
        return 0x80230008;
    if (arg1 == 0)
        return SCE_ERROR_INVALID_POINTER;
    int i;
    // F794
    for (i = 0; i < arg2; i++)
    {
        while (sub_F0C4() == 0)
        {
            int j = 999;
            // F830
            while ((--j) >= 0)
                ;
        }
        *(int*)(0xBD101008) = 0x50;
        *(int*)(0xBD10100C) = arg0 << 10;
        // F7BC
        int j;
        for (j = 3; j >= 0; j--)
        {
            t1 = *(int*)(0xBD101300);
            *(char*)(arg1 + 0) = (t1 >>  0) & 0xFF;
            *(char*)(arg1 + 1) = (t1 >>  8) & 0xFF;
            *(char*)(arg1 + 2) = (t1 >> 16) & 0xFF;
            *(char*)(arg1 + 3) = (t1 >> 24) & 0xFF;
            arg1 += 4;
        }
        arg0++;
    }
    // F7FC
    *(int*)(0xBD101014) = 1;
    return 0;
}

int sub_F8A8()
{
    int ret;
    *(0xBD101008) = 0x70;
    ret = *(0xBD101300) & 0xFF;
    *(0xBD101014) = 1;
    return ret;
}

