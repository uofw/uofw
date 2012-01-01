/* Copyright (C) 2011, 2012 The uOFW team
   See the file COPYING for copying permission.
*/

void _start()
{
    mtc0(mfc0("Status") & 0xFFBFFFF9, "Status");
    pspKernelSetK1(0);
    sp = 0x88800000;
    sp -= 64;
    void (*start)(void) = sub_28F4;
    start();
}

void sub_1DF8(int arg0, int arg1, int arg2, int arg3)
{
    /* SP size: 2832 */
    t4 = arg0 + arg1;
    t3 = arg0;
    t2 = 5;
    a1 = sp - 2808;
    t9 = 0;
    t0 = 1;
    s3 = -1;
    at = sp + 56;
    s4 = t4 - 64;
    s4 &= 0xFFFFFFC0;
    s0 = *(char*)(a2 + 0);
    val = *(int*)(a2 + 4);
    t1 = (t1 & 0x00FFFFFF) | (val << 24); /* valid IF a2 is 4 byte-aligned */
    val = *(int*)a2;
    t1 = (t1 & 0xFF000000) | (val >> 8);
    t1 = ((t1 & 0xFF000000) >> 24) | ((t1 & 0x00FF0000) >>  8)
       | ((t1 & 0x0000FF00) <<  8) | ((t1 & 0x000000FF) << 24);

    if (s0 < 0)
    {
        // 26D4
        t1 += a0;
        v1 = t1 < t4;
        if (v1 == 0)
            return 0x80000104;
        // 26E4
        do
        {
            v1 = *(unsigned char*)(a2 + 5);
            t3++;
            *(char*)t3 = v1;
            a2++;
        } while (t3 != t1);
        a2--;
        // 2564
        if (a3 != 0)
            *(int*)a3 = a2 + 5;
        return t3 - a0;
    }
    s5 = (s0 & 0x18) >> 3;
    s5 = s5 << 4;
    s1 = 128;
    s5 = s1 - s5;
    s5 = (s5 & 0xFFFF00FF) | ((s5 << 8) & 0x0000FF00);
    s0 &= 7;
    s5 = (s5 & 0x0000FFFF) | ((s5 << 16) & 0xFFFF0000);

    // 1E6C
    do
    {
        *(int*)(a1 + 2808) = s5;
        a1 += 8;
        *(int*)(a1 + 2804) = s5;
    } while (a1 != sp);

    s5 = 255;
    // 1F54
    for (;;)
    {
        t9 = (t9 & 0xFFFFFCFF) | ((t3 << 8) & 0x3);
        v1 = t9 >> s0;
        v1 = v1 & 7;
        t9 = v1 << 8;
        t8 = t9 - v1;
        v1 = sp + t8;
        t9 = 1;
    
        // 1EC4
        do
        {
            t7 = v1 + t9;
            t5 = s3 >> 24;
            t6 = *(char*)(t7 - 1);
            t9 <<= 1;
            if (t5 == 0)
            {
                // 1E84
                s2 = *(unsigned char*)(a2 + 5);
                t1 <<= 8;
                a2++;
                t1 += s2;
                lo = s3 * t6;
                s2 = t6 >> 3;
                t6 = t6 - s2;
                t5 = lo;
                t8 = t1 < t5;
                s2 = t9 >> 8;
                s3 <<= 8;
            }
            else
            {
                s2 = s3 >> 8;
                lo = s2 * t6;
                s2 = t6 >> 3;
                t6 -= s2;
                t5 = lo;
                t8 = t1 < t5;
                s2 = t9 >> 8;
            }
    
            if (t8 == 0)
            {
                // 1EB4
                t1 -= t5;
                *(char*)(t7 - 1) = t6;
                s3 -= t5;
            }
            else
            {
                // 1EF8
                s3 = lo;
                t6 += 31;
                *(char*)(t7 - 1) = t6;
                t9++;
            }
        } while (s2 == 0);
        // 1F0C
        s1 = s3 >> 24;
        t5 = *(unsigned char*)(a1 + 2488);
        *(char*)t3 = t9;
        // "new2"
        char gotoNew2 = 0;
        if (s1 == 0)
        {
            // 1F88
            t8 = *(unsigned char*)(a2 + 5);
            t3++;
            t1 <<= 8;
            a2++;
            t1 += t8;
            lo = s3 * t5;
            s3 <<= 8;
        }
        else
        {
            // 1F1C
            v1 = s3 >> 8;
            lo = v1 * t5;
            t3++;
        }
        v1 = t5 >> 4;
        s1 = lo;
        t6 = t1 < s1;
        v1 = t5 - v1;
        if (t6 != 0)
        {
            for (;;)
            {
                // 1FB8
                t8 = -1;
                char next = 0;
                char goto21A0 = 0;
                char goto216C = 0;
                while (next == 0)
                {
                    v1 += 15;
                    t5 = *(unsigned char*)(a1 + 2496);
                    t9 = s1 >> 24;
                    s3 = lo;
                    if (t9 == 0)
                    {
                        // 24F0
                        s2 = *(unsigned char*)(a2 + 5);
                        *(char*)(a1 + 2488) = v1 & 0xFF;
                        t1 <<= 8;
                        a2++;
                        t1 += s2;
                        a1 += 8;
                        lo = s3 * t5;
                        s3 = s1 << 8;
                    }
                    else
                    {
                        *(char*)(a1 + 2488) = v1;
                        s2 = s3 >> 8;
                        lo = s2 * t5;
                        a1 += 8;
                    }
                    s2 = t5 >> 4;
                    s1 = lo;
                    t9 = t1 < s1;
                    v1 = t5 - s2;
                    if (t9 != 0)
                    {
                        // 2524
                        int tmp = t8;
                        t8++;
                        if (tmp == t2)
                        {
                            v1 += 15;
                            s3 = lo;
                            next = 1;
                        }
                    }
                    else
                    {
                        // 1FF4
                        t1 -= s1;
                        if (t8 < 0)
                        {
                            // 219C
                            *(char*)(a1 + 2488) = v1;
                            goto21A0 = 1;
                            next = 2;
                        }
                        else
                            next = 1;
                    }
                }
                
                if (next == 1)
                {
                    // 2000
                    *(char*)(a1 + 2488) = v1 & 0xFF;
                    t9 = t3 << (t8 & 0x1F);
                    s1 = t8 << 5;
                    s1 = (s1 & 0xFFFFFFE7) | ((t9 << 3) & 0x1C);
                    s1 = (s1 & 0xFFFFFFFE) | (a1 & 3);
                    s1 = sp + s1;
                    t9 = t8 - 3;
                    a1 = *(unsigned char*)(s1 + 2552);
                    v1 = s3 >> 24;
                    if (t9 >= 0)
                    {
                        s2 = *(unsigned char*)(s1 + 2576);
                        v0 = s3 >> 8;
                        if (v1 == 0)
                        {
                            // 2618
                            v1 = *(unsigned char*)(a2 + 5);
                            t1 <<= 8;
                            a2++;
                            t1 += v1;
                            lo = s3 * s2;
                            s3 <<= 8;
                        }
                        else
                            lo = v0 * s2;
                
                        // 203C
                        v0 = s2 >> 3;
                        s2 -= v0;
                        v1 = lo;
                        v0 = t1 < v1;
                        t6 = v0 + 2;
                        if (v0 != 0)
                        {
                            // 2668
                            s3 = lo;
                            s2 += 31;
                        }
                        else {
                            t1 -= v1;
                            s3 -= v1;
                        }
            
                        v1 = s3 >> 24;
                        if (t9 > 0)
                        {
                            v0 = s3 >> 8;
                            if (v1 == 0)
                            {
                                // 2680
                                v1 = *(char*)(a2 + 5);
                                t1 <<= 8;
                                a2++;
                                t1 += v1;
                                s3 <<= 8;
                                v0 = s3 >> 8;
                            }
            
                            // 206C
                            lo = v0 * s2;
                            v0 = s2 >> 3;
                            s2 -= v0;
                            t7 = lo;
                            v0 = t1 < t7;
                            t6 <<= 1;
                            if (v0 != 0)
                            {
                                // 2634
                                s3 = lo;
                                t6++;
                                s2 += 31;
                                v1 = s3 >> 24;
                            }
                            else
                            {
                                s3 -= t7;
                                v1 = s3 >> 24;
                                t1 -= t7;
                            }
                            if (t9 != t0)
                            {
                                t6 <<= 1;
                                if (v1 == 0)
                                {
                                    // 2650
                                    v1 = *(unsigned char*)(a2 + 5);
                                    t1 <<= 8;
                                    a2++;
                                    t1 += v1;
                                    s3 <<= 7;
                                }
                                else
                                    s3 >>= 1;
            
                                // 20A4
                                int first = 1;
                                do
                                {
                                    if (!first) {
                                        t6 <<= 1;
                                        s3 >>= 1;
                                    }
                                    v0 = t1 < s3;
                                    v1 = t1 - s3;
                                    if (v0 == 0)
                                        t1 = v1;
                                    t9--;
                                    t6 += v0;
                                    first = 0;
                                } while (t9 != t0);
                                v1 = s3 >> 24;
                            }
                        }
                        // 20C0
                        *(char*)(s1 + 2576) = s2;
                    }
                    else
                        t6 = 1;
            
                    // 20C4
                    v0 = s3 >> 8;
                    if (v1 == 0)
                    {
                        // 25CC
                        v1 = *(unsigned char*)(a2 + 5);
                        t1 <<= 8;
                        a2++;
                        t1 += v1;
                        lo = s3 * a1;
                        s3 <<= 8;
                    }
                    else
                        lo = v0 * a1;
            
                    v0 = a1 >> 3;
                    a1 -= v0;
                    v1 = lo;
                    v0 = t1 < v1;
                    t6 <<= 1;
                    if (v0 != 0)
                    {
                        // 25FC
                        a1 += 31;
                        *(char*)(s1 + 2552) = a1;
                        s3 = lo;
                        t6++;
                        if (t8 <= 0)
                            goto216C = 1;
                        else
                            a1 = *(unsigned char*)(s1 + 2560);
                    }
                    else
                    {
                        // 20E8
                        *(char*)(s1 + 2552) = a1;
                        t1 -= v1;
                        s3 -= v1;
                        if (t8 <= 0)
                            goto21A0 = 1;
                        a1 = *(unsigned char*)(s1 + 2560);
                    }
                }
                char goto21AC = 0;
                if (!goto216C)
                {
                    if (goto21A0)
                    {
                        // 21A0
                        s2 = 64;
                        s1 = 8;
                        t5 = sp + t8;
                        goto21AC = 1;
                    }
                    else
                    {
                        // 20FC
                        v1 = s3 >> 24;
                        v0 = s3 >> 8;
                        if (v1 == 0)
                        {
                            // 25B0
                            v1 = *(unsigned char*)(a2 + 5);
                            t1 <<= 8;
                            a2++;
                            t1 += v1;
                            lo = s3 * a1;
                            s3 <<= 8;
                        }
                        else
                            lo = v0 * a1;
                
                        // 210C
                        v0 = a1 >> 3;
                        a1 -= v0;
                        v1 = lo;
                        v0 = t1 < v1;
                        t6 <<= 1;
                        if (v0 != 0)
                        {
                            // 2594
                            a1 += 31;
                            *(char*)(s1 + 2560) = a1;
                            t6++;
                            s3 = lo;
                        }
                        else
                        {
                            *(char*)(s1 + 2560) = a1;
                            t1 -= v1;
                            s3 -= v1;
                        }
                
                        if (t8 != t0)
                        {
                            a1 = *(char*)(s1 + 2568);
            
                            // 2138
                            v1 = s3 >> 24;
                            v0 = s3 >> 8;
                            if (v1 == 0)
                            {
                                // 2538
                                v1 = *(unsigned char*)(a2 + 5);
                                t1 <<= 8;
                                a2++;
                                t1 += v1;
                                lo = s3 * a1;
                                s3 <<= 8;
                            }
                            else
                                lo = v0 * a1;
            
                            // 2148
                            v0 = a1 >> 3;
                            a1 -= v0;
                            v1 = lo;
                            v0 = t1 < v1;
                            t6 <<= 1;
                            if (v0 != 0)
                            {
                                // 2554
                                a1 += 31;
                                t6 += 1;
                                s3 = lo;
                                if (t6 == s5)
                                {
                                    // 2564
                                    if (a3 != 0)
                                        *(int*)a3 = a2 + 5;
                                    return t3 - a0;
                                }
                            }
                            else {
                                t1 -= v1;
                                s3 -= v1;
                            }
                            // 2168
                            *(char*)(s1 + 2568) = a1;
                        }
                    }
                }
            
                // 216C
                if (!goto21AC)
                {
                    t5 = at + t8;
                    s2 = 256;
                    s1 = 8;
                }
                // 21AC
                do
                {
                    v1 = s3 >> 24;
                    t9 = t5 + s1;
                    if (v1 == 0)
                    {
                        // 2184
                        v1 = *(unsigned char*)(a2 + 5);
                        t1 <<= 8;
                        a2++;
                        t1 += v1;
                        s3 <<= 8;
                    }
                    // 21B8
                    t8 = *(unsigned char*)(t9 + 2033);
                    v0 = s3 >> 8;
                    s1 <<= 1;
                    lo = v0 * t8;
                    v0 = t8 >> 3;
                    t7 = t8 - v0;
                    v1 = lo;
                    t8 = t1 < v1;
                    int tmp = t8;
                    t8 = s1 - s2;
                    char goto21AC = 0;
                    char goto2368 = 0;
                    if (tmp == 0)
                    {
                        // 269C
                        s3 -= v1;
                        t1 -= v1;
                        *(char*)(t9 + 2033) = t7;
                        if (t8 < 0)
                            goto21AC = 1;
                        else
                        {
                            if (t8 != 0)
                                t8 -= 8;
                            else
                            {
                                t7 = 0;
                                if (t5 != t3)
                                    goto2368 = 1;
                                else
                                    return 0x80000108;
                            }
                        }
                    }
                    else
                    {
                        lo = s3;
                        t7 += 31;
                        *(char*)(t9 + 2033) = t7;
                        s1 += 8;
                        if (t8 < 0)
                            goto21AC = 1;
                    }
                } while (!goto2368 && goto21AC);
            
                if (!goto2368)
                {
                    // 21F4
                    s1 = sp + t8;
                    t8 >>= 3;
                    t9 = t8 - 3;
                    t5 = *(int*)(s1 + 2344);
                    if (t9 >= 0)
                    {
                        v0 = s3 >> 24;
                        s2 = t5 >> 24;
                        if (v0 == 0)
                        {
                            // 246C
                            v1 = *(unsigned char*)(a2 + 5);
                            t1 <<= 8;
                            a2++;
                            t1 += v1;
                            lo = s3 * s2;
                            s3 <<= 8;
                        }
                        else {
                            v0 = s3 >> 8;
                            lo = v0 * s2;
                        }
                
                        // 2220
                        v0 = s2 >> 3;
                        s2 -= v0;
                        v1 = lo;
                        v0 = t1 < v1;
                        t7 = v0 + 2;
                        if (v0 != 0)
                        {
                            // 24BC
                            s3 = lo;
                            s2 += 31;
                        }
                        else {
                            t1 -= v1;
                            s3 -= v1;
                        }
                
                        if (t9 > 0)
                        {
                            v0 = s3 >> 24;
                            int tmp = v0;
                            v0 = s3 >> 8;
                            if (tmp == 0)
                            {
                                // 24D4
                                v1 = *(unsigned char*)(a2 + 5);
                                t1 <<= 8;
                                a2++;
                                t1 += v1;
                                s3 <<= 8;
                                v0 = s3 >> 8;
                            }
                
                            // 2250
                            lo = v0 * s2;
                            v0 = s2 >> 3;
                            s2 -= v0;
                            v1 = lo;
                            v0 = t1 < v1;
                            t7 <<= 1;
                            if (v0 != 0)
                            {
                                // 2488
                                s3 = lo;
                                t7++;
                                s2 += 31;
                            }
                            else {
                                t1 -= v1;
                                s3 -= v1;
                            }
                
                            if (t9 != t0)
                            {
                                v0 = s3 >> 24;
                                t7 <<= 1;
                                if (v0 == 0)
                                {
                                    // 24A4
                                    v1 = *(unsigned char*)(a2 + 5);
                                    t1 <<= 8;
                                    a2++;
                                    t1 += v1;
                                    s3 <<= 7;
                                }
                                else
                                    s3 >>= 1;
                
                                // 2288
                                int first = 1;
                                do
                                {
                                    if (!first) {
                                        t7 <<= 1;
                                        s3 >>= 1;
                                    }
                                    v0 = t1 < s3;
                                    v1 = t1 - s3;
                                    t9 -= 1;
                                    if (v0 == 0)
                                        t1 = v1;
                                    t7 += v0;
                                    first = 0;
                                } while (t9 != t0);
                            }
                        }
                
                        // 22A0
                        t5 = (t5 & 0x00FFFFFF) | ((s2 << 24) & 0xFF000000);
                    }
                    else
                        t7 = 1;
                
                    // 22A4
                    v0 = s3 >> 24;
                    s2 = t5 & 0xFF;
                    if (v0 == 0)
                    {
                        // 2450
                        v1 = *(unsigned char*)(a2 + 5);
                        t1 <<= 8;
                        a2++;
                        t1 += v1;
                        lo = s3 * s2;
                        s3 <<= 8;
                    }
                    else {
                        v0 = s3 >> 8;
                        lo = v0 * s2;
                    }
                
                    // 22B8
                    v0 = s2 >> 3;
                    s2 -= v0;
                    v1 = lo;
                    v0 = t1 < v1;
                    t7 <<= 1;
                    char goto2358 = 0;
                    if (v0 != 0)
                    {
                        // 241C
                        s2 += 31;
                        t5 = (t5 & 0xFFFFFF00) | (s2 & 0xFF);
                        s3 = lo;
                        if (t8 <= 0)
                            goto2358 = 1;
                        else
                            t7++;
                    }
                    else
                    {
                        t5 = (t5 & 0xFFFFFF00) | (s2 & 0xFF);
                        t1 -= v1;
                        s3 -= v;
                        if (t8 <= 0) {
                            t7--;
                            goto2358 = 1;
                        }
                    }
                
                    if (!goto2358)
                    {
                        // 22E0
                        v0 = s3 >> 24;
                        s2 = ((t5 >> 8) & 0xFF);
                        if (v0 == 0)
                        {
                            // 2434
                            v1 = *(unsigned char*)(a2 + 5);
                            t1 <<= 8;
                            a2++;
                            t1 += v1;
                            lo = s3 * s2;
                            s3 <<= 8;
                        }
                        else {
                            v0 = s3 >> 8;
                            lo = v0 * s2;
                        }
                
                        // 22F4
                        v0 = s2 >> 3;
                        s2 -= v0;
                        v1 = lo;
                        v0 = t1 < v1;
                        t7 <<= 1;
                        if (v0 != 0)
                        {
                            s2 += 31;
                            t5 = (t5 & 0xFFFF00FF) | ((s2 << 8) & 0xFF00);
                            s3 = lo;
                            if (t8 == t0)
                                goto2358 = 1;
                            else
                                t7++;
                        }
                        else
                        {
                            t5 = (t5 & 0xFFFF00FF) | ((s2 << 8) & 0xFF00);
                            t1 -= v1;
                            s3 -= v1;
                            if (t8 == t0) {
                                t7--;
                                goto2358 = 1;
                            }
                        }
                
                        if (!goto2358)
                        {
                            // 231C
                            v0 = s3 >> 24;
                            s2 = ((t5 >> 16) & 0xFF);
                            if (v0 == 0)
                            {
                                // 23D8
                                v1 = *(unsigned char*)(a2 + 5);
                                t1 <<= 8;
                                a2++;
                                t1 += v1;
                                lo = s3 * s2;
                                s3 <<= 8;
                            }
                            else {
                                v0 = s3 >> 8;
                                lo = v0 * s2;
                            }
                
                            // 2330
                            v0 = s2 >> 3;
                            s2 -= v0;
                            v1 = lo;
                            v0 = t1 < v1;
                            t7 <<= 1;
                            if (v0 != 0)
                            {
                                s2 += 31;
                                t5 = (t5 & 0xFF00FFFF) | ((s2 << 16) & 0xFF0000);
                                s3 = lo;
                            }
                            else
                            {
                                t5 = (t5 & 0xFF00FFFF) | ((s2 << 16) & 0xFF0000);
                                t1 -= v1;
                                s3 -= v1;
                                t7--;
                            }
                        }
                    }
                
                    // 2358
                    t9 = t3 - a0;
                    t9 = t7 < t9;
                    *(int*)(s1 + 2344) = t5;
                    if (t9 == 0)
                        return 0x80000108;
                }
            
                // 2368
                a1 = sp | 0x6;
                v0 = t3 + t6;
                s1 = s4 < v0;
                v1 = t3 - t7;
                if (s1 != 0)
                {
                    // 23B4
                    s1 = v0 < t4;
                    a1 = (a1 & 0xFFFFFFFE) | (v0 & 1);
                    if (s1 == 0)
                        return 0x80000104;
                }
                else
                {
                    CACHE(0x18, t3 + 63);
                    s1 = t7 < 3;
                    a1 = (a1 & 0xFFFFFFFE) | (v0 & 1);
                    if (s1 == 0)
                    {
                        t6 &= 0xFFFFFFFC;
                        t6 += t3;
            
                        // 2394
                        do
                        {
                            /* TODO */
                            t9 = lwl(v1 + 2);
                            t9 = lwr(v1 - 1);
                            swl(t9, t3 + 3);
                            v1 += 4;
                            swr(t9, t3 + 0);
                            t3 += 4;
                        } while ((t3 - 4) != t6);
            
                        // 1F74
                        t3 = v0;
                        t9 = *(unsigned char*)t3;
                        s1 = s3 >> 24;
                        t5 = *(unsigned char*)(a1 + 2488);
                        gotoNew2 = 1;
                    }
                }
                if (!gotoNew2)
                {
                    // 23C0
                    for (;;)
                    {
                        t9 = *(unsigned char*)(v1 - 1);
                        if (t3 == v0)
                            break;
                        *(char*)t3 = t9;
                        t3++;
                        v1++;
                    }
                    // 1F0C
                    s1 = s3 >> 24;
                    t5 = *(unsigned char*)(a1 + 2488);
                    *(char*)t3 = t9;
                }
            }
        }
        // 1F3C
        *(char*)(a1 + 2488) = v1;
        a1--;
        a1 = (a1 > sp) ? a1 : sp;
        t1 -= s1;
        s3 -= s1;
        if (t3 == t4)
            return 0x80000104;
    }
}

typedef struct
{
    int u0, u4, u8;
    char *u12;
    int u16, u20;
    char *u24, *u28;
    int u32, u36, u40, u44;
} Opt;
Opt unkOpt; // 0x88612D80
char unkString1[256]; // 0x88612DB0
char unkString2[256]; // 0x88612EB0
char unkString3[256]; // 0x88612FB0
int unkMask1; // 0x88630C38
int unkMask2; // 0x88630C3C

// The main function, directly called by _start
int sub_28F4(int arg0, int arg1, int arg2, int arg3)
{
    mtc0(mfc0("Status") & 0xFFFFFFE0, "Status");
    memset(0x88612B80, 0, 0x886396E1 - 0x88612B80); // Empty undefined global variables (no idea how to do it?!?)
    int var = *(arg0 + 64);
    if (var & 0x40000000 == 0)
    {
        // 2B44
        sub_3804(0, 0, 0);
    }
    else
    {
        int addr3 = *(arg0 + 68);
        unkMask1 = var;
        unkMask2 = addr3;
        sub_3804(var, addr3, 0); // copy some values to globals
    }

    // 2980
    sub_49B8();
    setOutCharCb(putCharToUart4);
    setInStrCb(putStringToUart4Opt);
    setOutStrCb(getStringFromUart4Opt);
    sub_37F4(1);
    if (sub_3824(30) == 1)
    {
        // 2B1C
        sub_388C(56);
        sub_388C(57);
        sub_388C(58);
        sub_388C(59);
    }

    // 29D0
    Opt *addr2 = unkOpt;
    memcpy(addr2, arg1, 48); // copy args

    a1 = *(arg1 + 12);
    if (a1 != 0)
    {
        // 2B08
        strncpy(unkString1, a1, 256);
        unkOpt.u12 = unkString1;
    }
    else
        unkOpt.u12 = 0;

    // 29F8
    a1 = *(arg1 + 24);
    if (a1 != 0)
    {
        // 2AEC
        strcpy(unkString2, a1, 256);
        unkOpt.u24 = unkString2;
    }
    else
        unkOpt.u24 = 0;

    // 2A0C
    a1 = *(arg1 + 28);
    if (a1 != 0)
    {
        // 2AD0
        strncpy(unkString3, a1, 256);
        unkOpt.u28 = unkString3;
    }
    else
        unkOpt.u28 = 0;

    // 2A20
    Opt *addr = unkOpt;
    if (unkOpt.u32 & 0x10000 == 0) {
        // 2AC0
        *(arg0 + 56) = unkOpt.u28;
    }
    else
    {
        int a1 = 0;
        int a2 = 0;
        if (addr2 != 0)
        {
            a1 = unkOpt.u12;
            if (addr != 0)
                a2 = unkOpt.u24;
        }

        // 2A5C
        *(arg0 + 20) = 0x89000000;
        printf("key = 0x%08x, config = 0x%08x\n", a1, a2); // added in 6.38
        sub_4E38(arg0, a1, a2, unkOpt.u28); // load pspbtcnf & module files & decrypt them
        *(*(arg0 + 16) + *(arg0 + 12) * 32 - 12) = arg2;
    }

    sub_4FB4(arg0, arg3 + mfc0("Count")); // load modules + exec
    return 0;
}

char unkString4[256]; // 0x886130B0
int *resetVectorInfo; // 0x88630C44
int resetVectorAddr; // 0x88630C48
int unkMainVar; // 0x88638DE0

int sub_4E38(int arg0, int arg1)
{
    int count = *(arg0 + 28);
    if (count > 0)
    {
        v1 = *(arg0 + 32); // read some struct
        // 4E64
        do
        {
            a2 = *(u16*)(v1 + 8);
            count--;
            if (a2 == 8) {
                // 4FA8
                *(arg0 + 76) = *(v1 + 0);
            }
    
            // 4E70
            v1 += 28;
        } while (count != 0);
    }

    // 4E78
    v0 = *(arg0 + 76);
    if (v0 != 0)
        a1 = v0;
    else
        a1 = unkMainVar;
    unkMainVar = a1;
    sub_5EF8(sub_0DE4);
    sub_5F08(sub_15C4);
    sub_5F18(sub_1730);
    sub_5F28(sub_17E8);
    sub_5F38(sub_17FC);
    resetVectorInfo = arg0;
    *(arg0 + 60) = sub_52C4(arg1);
    if (sub_707C(arg0, unkString4, *(arg0 + 68), *(arg0 + 64)) == 0) { // load pspbtcnf.bin
        // 4F88
        sub_2874(arg0);
        return 0;
    }
    // 4F2C
    int ptr = resetVectorInfo;
    if (*(int*)(v1 + 4) > 0x2000000)
        *(int*)(0xBC100040) = (*(int*)(0xBC100040) & 0xFFFFFFFC) | 2;
    else
        *(int*)(0xBC100040) = (*(int*)(0xBC100040) & 0xFFFFFFFC) | 1;
    memset(0xBFC00000, 0, 0x1000);
    memcpy(0xBFC00000, &sub_1810, &sub_1810 + sizeof(sub_1810)); // NOTE: original uses 0x88601838 - 0x88601810 for size
    int ptr = resetVectorInfo;
    resetVectorAddr = 0xBFC00000;
    int (*resetVector)(int, int) = 0xBFC00000;
    resetVector(*(int*)(ptr + 0), *(int*)(ptr + 4));
    for (;;)
        ;
}

typedef struct
{
    int u0, u4, u8, u12, u16;
    int u20, u24, u28, u32, u36;
    int u40, u44, u48, u52, u56;
    int u60, u64, u68, u72, u76;
    int u80, u84, u88, u92, u96;
    int u100, u104, u108, u112, u116, u120;
} ModuleOpt;
ModuleOpt sysMemOpt; // 0x886131B0
ModuleOpt loadCoreOpt; // 0x88613230

int sub_4FB4(int arg0, int arg1)
{
    memset(sysMemOpt, 0, sizeof(sysMemOpt));
    memset(loadCoreOpt, 0, sizeof(loadCoreOpt));
    sysMemOpt.u96 = *(int*)(arg0 + 4);
    sysMemOpt.u20 = *(int*)(arg0 + 44);
    sysMemOpt.u12 = 3;
    sysMemOpt.u24 = *(int*)(arg0 + 60);
    sysMemOpt.u16 = 6;
    sysMemOpt.u8 = 0;
    sysMemOpt.u60 = *(int*)(arg0 + 64) | unkMask1;
    t1 = *(int*)(s1 + 16);
    s3 = t1 + 32;
    sysMemOpt.u4 = *(int*)(arg0 + 0);
    sysMemOpt.u64 = *(int*)(arg0 + 68) | unkMask2;
    sysMemOpt.u100 = arg1;
    sysMemOpt.u68 = *(int*)(arg0 + 80);
    sysMemOpt.u72 = *(int*)(arg0 + 28);
    *(int*)(t1 + 12) = *(int*)(arg0 + 0);
    sysMemOpt.u76 = *(int*)(arg0 + 32);
    sysMemOpt.u80 = putCharToUart4;
    sysMemOpt.u84 = *(int*)(s3 + 8);
    sysMemOpt.u88 = loadCoreOpt;
    int oldIntr = sub_3A84();
    loadModule(arg0, 0, sysMemOpt); // load sysmem
    loadCoreOpt.u48 = arg1;
    *(int*)(s3 + 12) = loadCoreOpt.u40;
    loadModule(arg0, 1, loadCoreOpt); // load loadcore (doesn't return)

    // if it goes there, then it's HEAVY fail
    sub_3AB0(oldIntr);
    // 5118
    int ptr = resetVectorInfo;
    if (*(int*)(v1 + 4) > 0x2000000)
        *(int*)(0xBC100040) = (*(int*)(0xBC100040) & 0xFFFFFFFC) | 2;
    else
        *(int*)(0xBC100040) = (*(int*)(0xBC100040) & 0xFFFFFFFC) | 1;
    memset(0xBFC00000, 0, 0x1000);
    memcpy(0xBFC00000, &sub_1810, &sub_1810 + sizeof(sub_1810)); // NOTE: original uses 0x88601838 - 0x88601810 for size
    int ptr = resetVectorInfo;
    resetVectorAddr = 0xBFC00000;
    int (*resetVector)(int, int) = 0xBFC00000;
    resetVector(*(int*)(ptr + 0), *(int*)(ptr + 4));
    for (;;)
        ;
}

char gameStr[] = "game"; // 0x88611E08
char vshStr[] = "vsh"; // 0x88611E10
char updaterStr[] = "updater"; // 0x88611E14
char popsStr[] = "pops"; // 0x88611E1C
char licenseStr[] = "licensegame"; // 0x88611E24
char appStr[] = "app"; // 0x88611E30
char umdStr[] = "umdemu"; // 0x88611E34
char mlnappStr[] = "mlnapp"; // 0x88611E3C

int sub_52C4(char *str)
{
    if (strncmp(str, gameStr, strlen(gameStr)) != 0)
    {
        // 531C
        if (strncmp(str, vshStr, strlen(vshStr)) == 0)
            return 2;
        if (strncmp(str, updateStr, strlen(updateStr)) == 0)
            return 3;
        if (strncmp(str, popsStr, strlen(popsStr)) == 0)
            return 4;
        if (strncmp(str, licenseStr, strlen(licenseStr)) == 0)
            return 5;
        if (strncmp(str, appStr, strlen(appStr)) == 0)
            return 6;
        if (strncmp(str, umdStr, strlen(umdStr)) == 0)
            return 7;
        if (strncmp(str, mlnappStr, strlen(mlnappStr)) == 0)
            return 8;
        return 0;
    }
    return 1;
}

char btcnfPath[] = "/kd/pspbtcnf.bin";

int sub_707C(int arg0, int arg1, int arg2, int arg3)
{
    int sp[8];
    sp[0] = btcnfPath;
    sub_5204(sp, 0, arg2, arg3, *(int*)(arg0 + 72));
    if (v0 < 0)
        return -1;
    if (sp[2] == 0)
        return -1;
    s0 = sub_5678(sp[1], sp[2], 1); // read?
    if (s0 < 0)
        sub_5174("_loadprx_by_pspbtcnf_bin", 647); // 7190
    sp[2] = s0;
    // 710C
    if (*(int*)(sp[1]) != 0x0F803001) // OMFG it's decrypted pspbtcnf's header
    {
        // 7174
        sub_5174("_loadprx_by_pspbtcnf_bin", 671);
        return 0x80020001;
    }
    if (sub_71A8(sp[1], s0, arg0, arg1, sp[1], arg2, arg3) < 0) // parse pspbtcnf!!
    {
        // 715C
        sub_5174("_loadprx_by_pspbtcnf_bin", 676);
    }
    // 7148
    memset(sp[1], 0, sp[2]);
    return 0;
}

// arg0 & arg4: pointer to pspbtcnf's content
// arg1: size?
int sub_71A8(int arg0, int arg1, int arg2, int arg3, int arg4, int arg5, int arg6)
{
    fp = 0;
    s3 = 0;
    t3 = *(int*)(arg4 + 20);
    *(int*)(sp + 16) = arg5;
    s6 = arg0 + *(int*)(arg4 + 48);
    s2 = arg0 + *(int*)(arg4 + 16);
    *(int*)(sp + 20) = arg6;
    s1 = arg0 + *(int*)(arg4 + 32);
    // 720C
    while (s3 < t3)
    {
        *(int*)(s2 + 12) += s6;
        t3 = *(int*)(arg4 + 20);
        if (*(int*)(arg2 + 60) == *(int*)(s2 + 8))
        {
            // 7468
            fp = *(int*)(s2 + 4);
            break;
        }
        s3++;
        s2 += 32;
    }
    // 7238
    if (s3 == t3) {
        // 7450
        sub_5174("type2", 343);
    }
    a1 = *(int*)(s2 + 12);
    // 7244
    if (a1 != 0) {
        // 7440
        strncpy(arg3, a1, 256);
    }
    *(int*)(arg2 + 56) = 0;
    // 7250
    *(int*)(arg2 + 12) = 0;
    s1 += *(short*)(s2 + 2) << 5;
    s0 = *(int*)(arg2 + 16);
    int i;
    for (i = 0; i < *(short*)(s2 + 0); i++)
    {
        a1 = *(int*)(s1 + 8);
        if ((fp & a1) & 0xFFF != 0)
        {
            // 72D4
            t5 = s6 + *(int*)s1;
            *(int*)s1 = t5;
            *(int*)(sp + 0) = t5;
            v0 = sub_5204(sp, ((a1 & 0xFF) == 4), *(int*)(sp + 16), *(int*)(sp + 20), *(int*)(arg2 + 72));
            if (v0 != 0)
            {
                // 7414
                *(int*)(s0 + 8) = 0;
                *(int*)(s0 + 16) |= 0x80000000;
                sub_5174("type2", 516);
            }
            else
            {
                *(int*)(s0 + 16) = 0;
                *(int*)(s0 + 8) = *(int*)(sp + 8);
                *(int*)(s0 + 20) = 0;
                *(int*)(s0 + 24) = 0;
                *(int*)(s0 + 28) = 0;
                *(int*)(s0 + 0) = *(int*)s1;
                a0 = *(int*)(s1 + 8) & 0xFF;
                *(int*)(s0 + 4) = *(int*)(sp + 4);
                if (a0 == 0x20000)
                {
                    // 740C (7404)
                    *(int*)(s0 + 16) = 1;
                }
                else
                {
                    if (a0 > 0x20000)
                    {
                        // 73E0
                        if (a0 != 4) {
                            *(int*)(s0 + 16) = 0;
                            // 7350
                            sub_5174("type2", 484);
                        }
                        else
                        {
                            if (*(char*)(arg2 + 24) == 0)
                                *(int*)(s0 + 20) = 528;
                            // 7400 (7404)
                            *(int*)(s0 + 16) |= 3;
                        }
                    }
                    else
                    {
                        *(int*)(s0 + 16) = 0;
                        if (a0 != 0x10000)
                        {
                            // 7350
                            sub_5174("type2", 484);
                        }
                    }
                }
                // 7360
                if (*(int*)(s1 + 8) < 0) {
                    // 73D4
                    *(int*)(s0 + 16) |= 0x100;
                }
                // 736C
                if (*(int*)(s0 + 8) != 0 || *(int*)(s1 + 8) & 0xFF != 4)
                {
                    // 7388
                    v0 = sub_5220(s0, s1);
                    if (v0 < 0)
                    {
                        // 73BC
                        sub_5174("type2", 502);
                    }
                }
                // 739C
                *(int*)(s0 + 16) |= 0x100;
                s0 += 32;
                *(int*)(arg2 + 12)++;
            }
        }
        // 728C
        s1 += 32;
    }
    return 0;
}

