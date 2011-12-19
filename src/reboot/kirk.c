/* Copyright (C) 2011 The uOFW team
   See the file COPYING for copying permission.
*/

int hwAddr = 0xBFC00220; // 0x886128BC
int hwAddr2 = 0xBFC00280; // 0x886128C0
int hwAddr3 = 0xBFC00340; // 0x886128C4

int unkAddr; // 0x88612B80
char unkTab[336]; // 0x88612BC0
char unkTab2[32]; // 0x88612D40

int sub_0BB8(int arg0, int arg1, int arg2)
{
    int maxAddr = 1 << (((((mfc0("Config") >> 6) & 7) + 12) & 0x1F) - 1);
    // BD8
    int i;
    for (i = 0; i < maxAddr; i += 64) {
        cache(0x14, i);
        cache(0x14, i);
    }
    // BF0
    *(int*)(0xBDE00010) = arg2;
    *(int*)(0xBDE0002C) = arg1 & 0x1FFFFFFF;
    *(int*)(0xBDE00030) = arg0 & 0x1FFFFFFF;;
    *(int*)(0xBDE0000C) = 1;
    // C1C
    while (*(int*)(0xBDE0001C) & 0x33 == 0)
        ;
    if (*(int*)(0xBDE0001C) & 0x22 == 0)
    {
        if (*(int*)(0xBDE0001C) & 0x10 != 0)
        {
            // C7C
            *(int*)(0xBDE0000C) = 2;
            // C84
            while (*(int*)(0xBDE0001C) & 0x33 == 0)
                ;
            if (*(int*)(0xBDE0001C) & 2 != 0)
            {
                *(int*)(0xBDE00028) = *(int*)(0xBDE0001C);
                SYNC();
                return -1;
            }
        }
        else
        {
            int ret = *(int*)(0xBDE00014);
            *(int*)(0xBDE00028) = *(int*)(0xBDE0001C);
            SYNC();
            return ret;
        }
    }
    *(int*)(0xBDE00008) = 1;
    SYNC();
    *(int*)(0xBDE00028) = *(int*)(0xBDE0001C);
    SYNC();
    return -1;
}

int sub_0CC4(int arg0, int arg1, int arg2)
{
    *(int*)(arg0 + 0) = 5;
    *(int*)(arg0 + 12) = arg2;
    *(int*)(arg0 + 16) = arg1;
    *(int*)(arg0 + 4) = 0;
    *(int*)(arg0 + 8) = 0;
    unkAddr = arg0;
    v0 = sub_0BB8(arg0, arg0, 7);
    if (v0 != 0)
        return -1;
    return 0;
}

char magic[4] = {0xF0, 0x8A, 0x94, 0x4C}; // 0x88611E00

int unusedVar; // 0x88612B84

int sub_0DE4(int arg0, int arg1, int arg2)
{
    char goto14FC = 0;
    t0 = -2;
    s5 = arg2;
    s3 = arg1;
    s0 = arg0;
    s4 = unusedVar;
    s1 = hwAddr;
    if ((arg0 != 0) && (arg2 != 0)
      && arg1 >= 352
      && arg0 & 0x3F == 0
      && ((arg0 >> 27) & 3) ^ 1 == 0)
    {
        if (memcmp(magic, a0 + 208, 4) != 0)
        {
            // 1538
            a1 = hwAddr;
            // 1540
            for (i = 0; i < 16; i++)
                *(char*)(a1 + i) = 0;
            t0 = -3;
        }
        else
        {
            // E84
            int i;
            char skip = 0;
            for (i = 0; i < 56; i++)
            {
                t0 = -3;
                if (*(char*)(arg0 + 212 + i) != 0) {
                    skip = 1;
                    break;
                }
            }
            if (!skip)
            {
                v0 = bcmp(arg0, s4, 0);
                if (v0 != 1)
                {
                    v0 = (*(char*)(arg0 + 179) << 24) | (*(char*)(arg0 + 178) << 16) | (*(char*)(arg0 + 177) << 8) | *(char*)(arg0 + 176);
                    v1 = s3 - 336;
                    v1 = v1 < v0;
                    *(int*)s5 = v0;
                    t0 = -2;
                    if (v1 == 0)
                    {
                        // F0C
                        for (i = 0; i < 336; i++)
                            unkTab[i] = *(char*)(s0 + i);
                        // F34
                        for (i = 0; i < 9; i++)
                        {
                            v0 = hwAddr2;
                            t0 = i << 4;
                            v0 += t0;
                            // F44
                            int j;
                            for (j = 0; j < 16; j++)
                                *(char*)(v0 + 20 + j) = *(char*)(s1 + j);
                            v0 = hwAddr2;
                            v0 += t0;
                            *(char*)(v0 + 20) = i;
                        }
                        // F80
                        for (i = 0; i < 16; i++)
                            *(char*)(s1 + i) = 0;
                        v0 = sub_0CC4(hwAddr2, 144, 67);
                        t0 = -1;
                        if (v0 < 0)
                            goto14FC = 1;
                        else
                        {
                            // FBC
                            for (i = 0; i < 92; i++)
                                *(char*)(s0 + i) = unkTab[208 + i];
                            s2 = s0 + 92;
                            a2 = s2;
                            // FEC
                            for (i = 0; i < 16; i++)
                                *(char*)(a2 + i) = unkTab[320 + i];
                            a2 += 16;
                            // 1018
                            for (i = 0; i < 20; i++)
                                *(char*)(a2 + i) = unkTab[300 + i];
                            a2 += 20;
                            // 1044
                            for (i = 0; i < 48; i++)
                                *(char*)(a2 + i) = unkTab[128 + i];
                            a2 += 48;
                            // 1070
                            for (i = 0; i < 16; i++)
                                *(char*)(a2 + i) = unkTab[192 + i];
                            a2 += 16;
                            // 109C
                            for (i = 0; i < 16; i++)
                                *(char*)(a2 + i) = unkTab[176 + i];
                            a2 += 16;
                            // 10C4
                            for (i = 0; i < 128; i++)
                                *(char*)(a2 + i) = unkTab[i];
                            v0 = hwAddr3;
                            // 10F0
                            for (i = 0; i < 96; i++)
                                *(char*)(v0 + 20 + i) = *(char*)(s2 + i);
                            v0 = sub_0CC4(hwAddr3, 96, 67);
                            t0 = -1;
                            if (v0 < 0)
                                goto14FC = 1;
                            else
                            {
                                // 1130
                                a2 = hwAddr3;
                                for (i = 0; i < 96; i++)
                                    *(char*)(s2 + i) = *(char*)(a2 + i);
                                // 1158
                                for (i = 0; i < 20; i++)
                                    *(char*)(hwAddr3 + i) = *(char*)(s0 + 108 + i);
                                // 117C
                                for (i = 0; i < 16; i++)
                                    *(char*)(s0 + 112 + i) = *(char*)(s2 + i);
                                // 11A8
                                for (i = 0; i < 32; i++)
                                    unkTab2[i] = *(char*)(s0 + 60 + i);
                                // 11D0
                                for (i = 0; i < 32; i++)
                                    *(char*)(s0 + 80 + i) = unkTab2[i];
                                // 11F4
                                for (i = 0; i < 56; i++)
                                    *(char*)(s0 + 24 + i) = 0;
                                unkAddr = s0;
                                // 1218
                                for (i = 0; i < 4; i++)
                                    *(char*)(s0 + 4 + i) = *(char*)(s0 + i);
                                v1 = unkAddr;
                                a3 = hwAddr2;
                                *(int*)v1 = 332;
                                // 124C
                                for (i = 0; i < 16; i++)
                                    *(char*)(s0 + 8 + i) = *(char*)(a3 + i);
                                a1 = hwAddr2;
                                // 1270
                                for (i = 0; i < 16; i++)
                                    *(char*)(a1 + i) = 0;
                                v0 = sub_0BB8(s0, s0, 11);
                                t0 = -1;
                                if (v0 != 0)
                                    goto14FC = 1;
                                else
                                {
                                    v1 = hwAddr3;
                                    v0 = s0;
                                    t0 = 0;
                                    // 12B0
                                    for (i = 0; i < 20; i++)
                                    {
                                        a3 = *(char*)v0;
                                        a2 = *(char*)v1;
                                        v0++;
                                        v1++;
                                        if (a3 != a2)
                                        {
                                            // 1530
                                            t0 = a3 - a2;
                                            break;
                                        }
                                    }
                                    // 12D0
                                    a3 = 0;
                                    if (t0 != 0)
                                    {
                                        // 1528
                                        t0 = -3;
                                        goto14FC = 1;
                                    }
                                    else
                                    {
                                        // 12D8
                                        for (i = 0; i < 64; i++)
                                            *(char*)(hwAddr3 + 20 + i) = *(char*)(s0 + 128 + i) ^ *(char*)(hwAddr2 + 16 + i);
                                        // 1314
                                        for (i = 0; i < 64; i++)
                                            *(char*)(hwAddr2 + 16 + i) = 0;
                                        v0 = sub_0CC4(hwAddr3, 64, 67);
                                        t0 = -1;
                                        if (v0 < 0)
                                            goto14FC = 1;
                                        else
                                        {
                                            // 1344
                                            for (i = 63; i >= 0; i--)
                                                *(char*)(s0 + 64 + a3) = *(char*)(hwAddr3 + a3) ^ *(char*)(hwAddr2 + 80 + a3);
                                            v0 = hwAddr2;
                                            // 137C
                                            for (i = 0; i < 64; i++)
                                                *(char*)(v0 + 80 + i) = 0;
                                            // 139C
                                            for (i = 0; i < 32; i++)
                                                *(char*)(s0 + 128 + i) = unkTab2[i];
                                            // 13C0
                                            for (i = 0; i < 16; i++)
                                                *(char*)(s0 + 160 + i) = 0;
                                            *(char*)(s0 + 164) = 1;
                                            *(char*)(s0 + 160) = 1;
                                            // 13EC
                                            for (i = 0; i < 16; i++)
                                                *(char*)(s0 + 176 + i) = *(char*)(s0 + 192 + i);
                                            for (i = 0; i < 16; i++)
                                                *(char*)(s0 + 192 + i) = 0;
                                            v0 = sub_0BB8(s0, s0 + 64, 1);
                                            t0 = -3;
                                            if (v0 != 0)
                                                goto14FC = 1;
                                            else
                                            {
                                                a3 = *(int*)s5;
                                                v0 = a3 < 336;
                                                t0 = 0;
                                                if (v0 != 0)
                                                {
                                                    v0 = 336;
                                                    v1 = s0 + a3;
                                                    if (v0 != a3)
                                                    {
                                                        do
                                                        {
                                                            *(char*)(v1 + t0) = 0;
                                                            t0++;
                                                        } while (*(int*)s5 + t0 - 336 != 0);
                                                        t0 = 0;
                                                    }
                                                }
                                            }
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }
    }
    if (goto14FC)
    {
        // 14FC
        // 1504
        for (i = 0; i < 336; i++)
            *(char*)(s0 + i) = unkTab[i];
        *(int*)s5 = 0;
    }
    // 147C
    // 1480
    for (i = 0; i < 16; i++)
        *(char*)(s1 + i) = 0;
    a1 = hwAddr2;
    // 149C
    for (i = 0; i < 164; i++)
        *(char*)(a1 + i) = 0;
    a1 = hwAddr3;
    for (i = 0; i < 180; i++)
        *(char*)(a1 + i) = 0;
    return t0;
}

int sub_1568(int arg0, int arg1)
{
    *(int*)(arg0 + 0) = 5;
    *(int*)(arg0 + 12) = 256;
    *(int*)(arg0 + 16) = arg1;
    *(int*)(arg0 + 4) = 0;
    *(int*)(arg0 + 8) = 0;
    unkAddr = arg0;
    v0 = sub_0BB8(arg0, arg0, 8);
    if (v0 != 0)
        return -1;
    return 0;
}

char shift1[16] = {0x71, 0xF6, 0xA8, 0x31,
                   0x1E, 0xE0, 0xFF, 0x1E,
                   0x50, 0xBA, 0x6C, 0xD2,
                   0x98, 0x2D, 0xD6, 0x2D}; // 0x88611DD0
char shift2[16] = {0xAA, 0x85, 0x4D, 0xB0,
                   0xFF, 0xCA, 0x47, 0xEB,
                   0x38, 0x7F, 0xD7, 0xE4,
                   0x3D, 0x62, 0xB0, 0x10}; // 0x88611DE0

int sub_15C4(int arg0, int arg1)
{
    if (arg0 == 0)
        return -2;
    if (arg1 < 352)
        return -2;
    if (arg0 & 0x3F != 0)
        return -2;
    if (((arg0 >> 27) & 3) ^ 1 != 0)
        return -2;
    // 1618
    int i;
    for (i = 0; i < 208; i++)
        unkTab[i + 20] = *(char*)(arg0 + 128 + i);
    // 1640
    for (i = 0; i < 208; i++)
        unkTab[i + 20] ^= shift2[i & 0xF];
    v0 = sub_1568(unkTab, 208);
    if (v0 < 0)
        return v0;
    // 1694
    for (i = 0; i < 208; i++)
        unkTab[i] ^= shift1[i & 0xF];
    // 16C8
    for (i = 0; i < 144; i++)
        *(char*)(arg0 + 128 + i) = unkTab[64 + i];
    // 16F0
    for (i = 0; i < 64; i++)
        *(char*)(arg0 + 272 + i) = unkTab[i];
    return 0;
}

char xorKey[] = {0x2E, 0xD3, 0xD3, 0x81,
                 0xC6, 0x69, 0x57, 0x2E,
                 0xE6, 0xDB, 0xCB, 0x27,
                 0x0E, 0x94, 0x1F, 0xA2}; // 0x88611DF0

int sub_1730(int arg0, int arg1)
{
    if (arg0 != 0)
        return -2;
    // 1744
    int i;
    for (i = 0; i < 16; i++)
        if (*(char*)(arg1 + i) != 0)
            break;
    // 1764
    if (i == 16)
        return -2;
    for (i = 0; i < 16; i++)
        *(char*)(hwAddr + i) = xorKey[16] ^ *(char*)(arg1 + i);
    return 0;
}

