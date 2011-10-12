void sub_01D4()
{
    if (mfc0($24) & 1 != 0) {
        sub_01E4();
        return sub_18E4();
    }
    return sub_01E4();
}

int sub_01E4()
{
    int end = 0x1000 << ((mfc0(Config) >> 6) & 7);
    int addr = 0;
    // 01F8
    do
    {
        cache(0x14, addr);
        cache(0x14, addr);
        addr += 64;
    } while (addr != end);
    sync();
    return 0;
}

int sub_0928()
{
    if (mfc0($24) & 1 != 0)
        sub_18E4(); // 0978

    // 0938
    int end = 0x1000 << ((mfc0(Config) >> 9) & 7);
    int ic = mfic();
    mtic(0);
    mtc0(0, TagLo);
    mtc0(0, TagHi);
    int addr = 0;
    // 95C
    while (addr != end)
    {
        cache(1, addr);
        cache(3, addr);
        addr += 64;
    }
    mtic(ic);
    return 0;
}

int sub_11970(int arg0, int arg1)
{
    int origVar = *(int*)(0xBC100050);
    if (arg1 == 0)
        *(int*)(0xBC100050) = origVar ^ (origVar & arg0); // Clear bits
    else
        *(int*)(0xBC100050) = origVar | arg0; // Add bits

    return (origVar & arg0) != 0;
}

void sub_1810(int arg0, int arg1)
{
    int addr = arg0;
    int end = arg0 + arg1;
    // 181C
    while (addr != end) {
        *(int*)addr = 0;
        addr += 4;
    }
    // 1828
    while (arg0 != end) {
        *(int*)arg0 = 0;
        arg0 += 4;
    }
    BREAK(0);
}

int sub_18E4()
{
    int i, j;
    int addr = 0xA7F80000;
    // 1900
    for (i = 8; i != 0; i--)
    {
        // 1904
        for (j = 4; j != 0; j--) {
            *(int*)0xA7F00000 = (*(int*)addr & 0x0FFFFFC0) | 0xB000003F;
            addr += 0x800;
        }
        addr -= 0x1FFC;
    }

    // 1950
    for (i = 8; i != 0; i--)
    {
        *(int*)(0xA7F00000) = 0x7000003F;
        *(int*)(0xA7F00000) = 0x7000103F;
        *(int*)(0xA7F00000) = 0x7000203F;
        *(int*)(0xA7F00000) = 0x7000303F;
    }
    (void)*(int*)(0xA7F00000);
    return 0;
}

int sub_1B7C(int arg)
{
    int test;
    if (arg > 0)
        test = 0x004C004C;
    else
        test = 0x00004C4C;

    if (((test >> ((arg >> 26) & 0x1F)) & 1) == 0)
        return 0;

    arg = (arg & 0x07FFFFFF) | ((arg > 0) << 27);
    int addr = 0xA7F80000 | (((arg >> 6) & 0xFF) << 5);
    int i = 8;
    // 1BC4
    while ((i--) != 0)
    {
        int var = *(int*)addr;
        int var2;
        if (((var & 0xFFFFFC0) ^ arg) != 0)
            var2 = 0;
        else
            var2 = (var >> 28) & 1;
        if (var2 != 0)
            return ((var >> 29) & 1) + 1;
        addr += 4;
    }
    return 0;
}

int sub_3A84()
{
    int ret;
    asm("mfic %0, $0;"
        "mtic $0, $0;"
        "nop;"
        "nop;"
        "nop;"
        "nop;"
        "nop;"
        "nop;"
        "nop;"
      : "=r" (ret));
    return ret;
}

void sub_3AB0(int ic)
{
    asm("mtic %0, $0;"
        "nop;"
        "nop;"
        "nop;"
        "nop;"
        "nop;"
        "nop;"
        "nop;"
      :
      : "r" (ic));
}

void sub_4D30(int arg)
{
    if (arg != 0)
    {
        // 4D4C
        arg -= 4;
        *(int*)(0xBE4C002C) |= 0x10;
        if (arg >= 25)
        {
            // 4D9C
            *(int*)(0xBE4C0034) = 0;
        }
        else
        {
            switch (arg)
            {
            case 4:
                *(int*)(0xBE4C0034) = 9; // 4D80
                break;

            case 12:
                *(int*)(0xBE4C0034) = 18; // 4D8C
                break;

            case 20:
                *(int*)(0xBE4C0034) = 27; // 4D94
                break;

            case 24:
                *(int*)(0xBE4C0034) = 36; // 4DA8    
                break;

            default:
                *(int*)(0xBE4C0034) = 0; // 4D9C
                break;
            }
        }
    }
    else
        *(int*)(0xBE4C002C) &= 0xFFFFFFEF;
}

void sub_5174()
{
    int ptr = resetVectorInfo;
    if (*(int*)(ptr + 4) > 0x2000000)
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

