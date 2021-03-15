/* Copyright (C) 2011, 2012 The uOFW team
   See the file COPYING for copying permission.
*/
#include <common_header.h>

int unkVar = 0x89000000; // 0x886128E0

int sub_11994()
{
    return sub_11C8C(2, 1); // Activate USB IO?
}

void sub_11C8C(int arg0, int arg1)
{
    int origVar = *(int*)(0xBC100078);
    if (arg1 == 0)
        *(int*)(0xBC100078) = origVar ^ (origVar & arg0);
    else
        *(int*)(0xBC100078) = origVar | arg0;
    return (origVar & arg0) != 0;
}

int isInited; // 0x88630B00

int sub_26FC(int arg0, int arg1, int arg2, int arg3, int arg4)
{
    if (isInited == 0)
    {
        isInited = 1;
        v0 = sub_8124(0, arg2, arg3);
        if (v0 < 0)
            return SCE_ERROR_KERNEL_ERROR;
    }
    // 2768
    v0 = sub_822C(*(int*)(arg0 + 0));
    if (v0 < 0)
    {
        // 2864
        *(int)(arg0 + 8) = 0;
        if (arg1 == 1)
            return 0;
        return v0;
    }
    unkVar = (unkVar + 63) & 0xFFFFFFC0;
    s0 = sub_83E0(0, 0, 2);
    v0 = sub_83E0(0, 0, 0);
    if (s0 < 0)
    {
        // 2854
        sub_8344();
        return s0;
    }
    a0 = unkVar;
    if (a0 + s0 >= arg4)
    {
        // 2840
        sub_8344();
        return SCE_ERROR_KERNEL_ILLEGAL_SIZE;
    }
    *(int*)(arg0 + 4) = a0;
    // 27D8
    int count = 0;
    do
    {
        v0 = sub_83A0(a0, 0x80000000);
        count += v0;
        unkVar += v0;
        a0 = t3;
    } while (v0 >= 0x80000000);
    *(int*)(arg0 + 8) = count;
    v0 = sub_8344();
    if (v0 < 0)
        return v0;
    return 0;
}

// 2874
int sub_2874(int arg)
{
    *(int*)(arg + 76) = 0;
    unkVar = (unkVar + 63) & 0xFFFFFFC0;
    v0 = sub_82E4(/*0*/);
    if (v0 < 0)
        return SCE_ERROR_KERNEL_ERROR;
    v0 = sub_82C8(unkVar, *(int*)(arg + 72));
    if (v0 < 0)
        return SCE_ERROR_KERNEL_ERROR;
    *(int*)(arg + 76) = unkVar;
    return 0;
}

int unkVar1; // 0x88630C34

int sub_49B8()
{
    if (unkVar1 <= 0)
    {
        int firstIc = sub_3A84();
        int oldIc;
        *(0xBC100050) |= 0x4000;
        *(0xBC100058) |= 0x200;
        *(0xBC100078) |= 0x80000;
        sync();
        if (0x3D090 == 0)
            break(7); // ZOMG WE BROKE DA PSP

        oldIc = sub_3A84();
        *(0xBE4C0024) = 6;
        sub_3AB0(oldIc);
        *(0xBE4C0028) = 0;
        *(0xBE4C002C) = 96;
        *(0xBE4C0030) |= 0x301;
        sub_4D30(4);
        oldIc = sub_3A84();
        *(0xBE4C0044) |= 0x7FF;
        sub_3AB0(oldIc);
        sub_3AB0(firstIc);
        unkVar1++;
    }
    return 0;
}

// 4BD0
int putstringToUart4(char *s, int n)
{
    int i;
    // 4BDC
    for (i = 0; i < n; i++)
    {
        // 4BE0
        while (*(int*)(0xBE4C0018) & 0x20 != 0) // Wait UART4 to be ready
            ;
        *(int*)(0xBE4C0000) = *(s++);
    }
    return 0;
}

// 4C6C
int getStringFromUart4(char *s, int n)
{
    int count = 0;
    while (count < n)
    {
        char c;
        do
        {
            while (*(int*)(0xBE4C0018) & 0x10 != 0)
                ;
        } while ((c = *(int*)(0xBE4C0000) & 0xFF) < 0);
        *(s++) = c;
        count++;
    }
    return count;
}
       
// 4CE0
int putStringToUart4Opt(short unused, char *s, int n)
{
    return putStringToUart4(s, n);
}

// 4D00
int getStringFromUart4Opt(int unused, char *s, int n)
{
    return getStringFromUart4(s, n);
}

// 4DB0
int putCharToUart4(int arg0, int arg1)
{
    if (arg1 == 0x200) // Empty list of chars
    {
        // 4E30
        *(short*)(arg0 + 2) = 0;
        return;
    }
    if (arg1 == 0x201) // Force printing
    {
        // 4E1C
        if (*(short*)(arg0 + 2) <= 0)
            return;
        putStringToUart4Opt(*(short*)(arg0 + 0), arg0 + 4, *(short*)(arg0 + 2));
        return;
    }
    *(short*)(arg0 + *(short*)(arg0 + 2) + 4) = arg1;
    if ((*(short*)(arg0 + 2)++) == 0xFF) // Print 0xFF characters when reached
    {
        *(short*)(arg0 + 2) = 0;
        putStringToUart4Opt(*(short*)(arg0 + 0), arg0 + 4, 0xFF);
    }
}

int sub_5204(int arg0, int arg1, int arg2, int arg3, int arg4)
{
    return sub_26FC(arg0, arg1, arg2, arg3, arg4);
}

