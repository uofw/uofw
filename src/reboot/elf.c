#include "reboot.h"

#define PSP_TYPE_SYSMEM 0
#define PSP_TYPE_LOADCORE 1

Callback1 callback1 = NULL; // 0x886128C8
Callback2 callback2 = NULL; // 0x886128CC
Callback3 callback3 = NULL; // 0x886128D0
Callback4 callback4 = NULL; // 0x886128D4
Callback5 callback5 = NULL; // 0x886128D8
int unkVar = 0; //0x886128DC

typedef struct
{
    int u0, u4, u8, u12, u16;
    int u20, u24, u28, u32, u36;
    int (*runCallback)(int, int); // 40
    int u44, u48, u52, u56;
    int u60, u64, u68, u72, u76;
    int u80, u84, u88, u92, u96;
    int u100, u104, u108, u112, u116, u120;
    int u124, u128, u132, u136, u140, u144;
    int u148, u152, u156, u160, u164, u168;
    int u172, u176, u180, u184, u188;
} ElfInfo;
ElfInfo moduleInfos[2]; // 0x886132B0

// 544C
int loadModule(int arg0, int type, int arg2) // type: 0 = sysmem, 1 = loadcore
{
    ElfInfo info = moduleInfos[type];
    int sp[12];
    int ptr = *(int*)(void*)(arg0 + 16) + type * 32;
    memset(&info, 0, sizeof(info));
    info.u100 = 1;
    info.u36 = *(int*)(ptr + 12);
    info.u8 = 0;
    int ret = sub_5C7C(*(int*)(ptr + 4), &info, type, arg2);
    if (ret < 0)
    {
        // 55F0
        int ptr = resetVectorInfo;
        if (*(int*)(ptr + 4) > 0x2000000)
            *(int*)(0xBC100040) = (*(int*)(0xBC100040) & 0xFFFFFFFC) | 2;
        else
            *(int*)(0xBC100040) = (*(int*)(0xBC100040) & 0xFFFFFFFC) | 1;
        memset((void*)0xBFC00000, 0, 0x1000);
        memcpy((void*)0xBFC00000, &sub_1810, sizeof(sub_1810)); // NOTE: original uses 0x88601838 - 0x88601810 for size
        resetVectorAddr = 0xBFC00000;
        int (*resetVector)(int, int) = 0xBFC00000;
        resetVector(*(int*)(ptr + 0), *(int*)(ptr + 4));
        for (;;)
            ;
    }
    memset(*(int*)(ptr + 4), 0, *(int*)(ptr + 8));
    if (type == PSP_TYPE_LOADCORE)
    {
        // 5598
        *(int*)(arg2 + 64) = info.u120;
        *(int*)(arg2 + 68) = info.u124;
        *(int*)(arg2 + 80) = &moduleInfos[1];
        *(int*)(arg2 + 100) = sub_0DE4;
        *(int*)(arg2 + 104) = sub_15C4;
        *(int*)(arg2 + 92) = sub_1730;
        *(int*)(arg2 + 84) = sub_17E8;
        *(int*)(arg2 + 88) = sub_17FC;
        *(int*)(arg2 + 76) = &moduleInfos[0];
    }

    // 5504
    sub_0928();
    sub_01D4();
    *(int*)(arg0 + 8) = 2;
    if (type == PSP_TYPE_SYSMEM)
        info.runCallback(4, arg2);
    if (type == PSP_TYPE_LOADCORE)
    {
        // 5550
        sp[0] = arg0;
        sp[1] = arg2;
        pspKernelSetSp(*(int*)(arg2 + 72) - 64);
        return info.runCallback(8, sp);
    }
    return 0;
}

char unkString1[256]; // 0x88613430
char unkString2[256]; // 0x88613530

// 57C0
void applySegmentRelocs(int arg0, int arg1, int arg2, int arg3, int arg4)
{
    a0 = 1;
    s7 = 0;
    s6 = arg3 + arg4;
    s2 = 0;
    s1 = a1;
    s0 = 0;

    v1 = 0;
    // 5804
    do
    {
        v1++;
        t0 = a0 << (v1 & 0x1F);
        if (v1 >= 33)
            return 0x80020001;
    } while (t0 < a2);

    t6 = v1;

    t6 = v1;

    // 5828
    v1 = *(short*)(a3 + 0);
    if (v1 != 0)
        return 0x80020001;
    v1 = *(char*)(a3 + 4);
    t5 = *(char*)(a3 + 2);
    s3 = *(char*)(a3 + 3);
    t8 = v1 & 0xFF;
    a3 += 4;
    a1 = unkString2;
    a0 = 0;

    // 5864
    while (a0 < t8)
    {
        a0++;
        *(char*)a1 = v1;
        a3++;
        a1++;
        v1 = *(char*)a3;
    }

    // 5888
    t9 = v1 & 0xFF;
    a1 = unkString1;
    a0 = 0;

    // 589C
    while (a0 < t9)
    {
        a0++;
        *(char*)a1 = v1;
        a3++;
        a1++;
        v1 = *(char*)a3;
    }

    // 58BC
    t3 = a3 < s6;
    s5 = t9;
    if (t3 == 0)
        return 0;
    s4 = -t5;

    // 58CC
    do
    {
        t0 = *(short*)a3;
        t1 = 16;
        t2 = t1 - t5;
        v1 = t0 << t2;
        a1 = v1 & 0xFFFF;
        a0 = s4 + 16;
        t4 = a1 >> a0;
        v1 = t4 & 0xFFFF;
        t7 = v1 < t8;
        t4 = a3 + 2;
        if (t7 == 0)
            return 0x80020001;
        t3 = unkString2[v1]
        v1 = t3 & 1;
        a0 = t5 + t6;
        if (v1 == 0)
        {
            // 5C28
            s0 = t1 - a0;
            v1 = t0 << (s0 & 0x1F);
            t7 = t5 - a0;
            s2 = v1 & 0xFFFF;
            t2 = t7 + 16;
            a1 = s2 >> (t2 & 0x1F);
            s2 = a1 & 0xFFFF;
            s0 = s2 < a2;
            if (s0 == 0)
                return 0x80020001;
            v1 = t3 & 6;
            s0 = t0 >> (a0 & 0x1F);
            if (v1 != 0)
            {
                t0 = 4;
                if (v1 != t0)
                    return 0x80020001;
                s0 = *(int*)t4; // unaligned reading
                t4 = a3 + 6;
            }
        }
        else
        {
            t7 = 1;
            a0 = t6 + t5;
            if (v1 != 1)
                return 0x80020001;
            t2 = a0 + s3;
            v0 = t1 - t2;
            t7 = t0 << (v0 & 0x1F);
            a1 = a0 - t2;
            t2 = t7 & 0xFFFF;
            t7 = a1 + 16;
            a1 = t2 << t7;
            t2 = a1 & 0xFFFF;
            t7 = t2 < t9;
            if (t7 == 0)
                return 0x80020001;
            t7 = t1 - a0;
            v1 = t0 << t7;
            a1 = t5 - a0;
            t7 = v1 & 0xFFFF;
            a0 = a1 + 16;
            a1 = t7 >> a0;
            a0 = s2 < a2;
            t7 = a1 & 0xFFFF;
            if (a0 == 0)
                return 0x80020001;
            a0 = t7 < a2;
            if (a0 != 0)
                return 0x80020001;
            v1 = t3 & 6;
            a1 = 2;
            t2 = unkString1[t2];
            if (v1 == a1)
            {
                v0 = s3 + t6;
                // 5BF8
                a0 = v0 + t5;
                v1 = a0 - t1;
                t1 = v1 + 16;
                t4 = *(short*)(a3 + 2);
                a1 = t0 >> (t1 & 0x1F);
                v1 = a1 << 16;
                t0 = v1 | t4;
                t1 = t0 << (a0 & 0x1F);
                t4 = t1 >> (a0 & 0x1F);
                s0 += t4;
                t4 = a3 + 4;
            }
            else if (v1 >= 3)
            {
                s0 = 4;
                // 5BE0
                if (v1 != s0)
                    return 0x80020001;
                s0 = *(int*)t4; // unaligned reading
                t4 = a3 + 6;
            }
            else
            {
                a0 = t0 << 16;
                if (v1 != 0)
                    return 0x80020001;
                a1 = s3 + t6;
                v1 = a1 + t5;
                t0 = v1 + 16;
                a3 = a0 >> t0;
                s0 += a3;
            }
        
            // 59D4
            t0 = s2 << 2;
            a1 = t0 + s1;
            a0 = *(int*)(a1 + 32);
            a3 = s0 < a0;
            if (a3 == 0)
                return 0x80020001;
            v1 = t3 & 0x38;
            t3 = 8;
            if (v1 == t3)
            {
                v1 = s5 ^ 0x6;
                // 5BD0
                if (v1 != 0)
                    s7 = 0;
            }
            else
            {
                s5 = v1 < 9;
                s7 = 16;
                if (s5 == 0)
                {
                    // 5BA4
                    if (v1 != s7)
                    {
                        // 5BB8
                        if (v1 == 24)
                            s7 = *(int*)t4; // unaligned reading
                        return 0x80020001;
                    }
                    s7 = *(short*)t4;
                    t4 += 2;
                }
                else
                {
                    s7 = 0;
                    if (v1 != 0)
                        return 0x80020001;
                }
            }
        
            a0 = t0 + s1;
            // 5A18
            t1 = *(int*)(a0 + 16);
            a3 = t7 << 2;
            a1 = a3 + s1;
            t7 = t2 < 8;
            a3 = s0 + t1;
            t1 = *(int*)(a1 + 0);
        
            if (t2 >= 8)
                return 0x80020001;
        
            switch (t2)
            {
            case 0:
                // 5A68
                break;
        
            case 1:
                // 5AA8
                a1 = *(int*)a3; // unaligned reading
                a0 = t0 + s1;
                s5 = *(int*)a0;
                t0 = a1;
                t0 &= 0x3F;
                v0 = s0 + s5;
                v0 &= 0xF0000000;
                t3 = t0 << 2;
                t7 = t3 | v0;
                v1 = t7 + t1;
                v0 = (v1 >> 2) & 0x3FFFFFF;
                a1 &= 0xFC000000;
                v0 |= a1;
                *(int*)a3 = v0; // unaligned writing
                break;
        
            case 2:
                // 5AE4
                v1 = *(int*)a3; // unaligned reading
                a0 = t0 + s1;
                t0 = *(int*)a0;
                v1 &= 0x3F;
                t3 = v1 << 2;
                s5 = s0 + t0;
                s5 &= 0xF0000000;
                t7 = t3 | s5;
                a1 = t7 + t1;
                v0 = (a1 >> 2) & 0x3FFFFFF;
                a1 = 0x8000000;
                v0 |= a1;
                *(int*)a3 = v0; // unaligned writing
                break;
        
            case 3:
                // 5B18
                v1 = *(int*)a3; // unaligned reading
                a0 = t0 + s1;
                s5 = *(int*)a0;
                v1 &= 0x3F;
                t3 = v1 << 2;
                v0 = s0 + s5;
                v0 &= 0xF0000000;
                t7 = t3 | v0;
                a1 = t7 + t1;
                v0 = (a1 >> 2) & 0x3FFFFFF;
                a1 = 0xC000000;
                v0 |= a1;
                *(int*)a3 = v0; // unaligned writing
                break;
        
            case 4:
            case 7:
                // 5B80
                a1 = *(int*)a3; // unaligned reading
                v0 = (int)(a1 & 0xFFFF);
                a0 = t1 + v0;
                v0 = a0 & 0xFFFF;
                a1 &= 0xFFFF0000;
                v0 |= a1;
                *(int*)a3 = v0; // unaligned writing
                break;
        
            case 5:
                // 5A54
                *(int*)a3 += t1; // unaligned RW
                break;
        
            case 6:
                // 5B4C
                t0 = *(int*)a3; // unaligned reading
                t3 = t0 << 16;
                s5 = (int)(s7 & 0xFFFF);
                t7 = t3 + s5;
                v1 = t7 + t1;
                t1 = v1 >> 15;
                a1 = t1 + 1;
                v0 = (a1 >> 1) & 0x3FFFFFF;
                a1 = *(int*)a3; // unaligned reading
                a1 &= 0xFFFF0000;
                v0 |= a1;
                *(int*)a3 = v0; // unaligned writing
                break;
            }
        
            s5 = t2;
        }
        // 5A6C
        a3 = t4;
    } while (t4 < s6);
    return 0;
}

int sub_5C7C(int arg0, int arg1, int arg2, int arg3) // arg1: ELF buffer
{   
    if ((arg0 == 0) || (arg1 == 0)
     || (arg3 == 0) || (arg2 >> 31)
     || (arg2 >= 2))
        return 0x800200D2;

    *(arg1 + 28) = arg0;
    // 5CF4
    v0 = sub_6014(arg1);
    if (v0 < 0)
        return v0;
    s3 = sub_6284(arg1, arg2); // Get ELF memory size
    v0 = sub_60FC(arg1, sp + 64);
    if (v0 < 0)
        return v0;
    sub_6350(arg1, s3, arg2, arg3);
    if (v0 < 0)
        return v0;
    v0 = sub_6428(arg1);
    if (v0 < 0)
        return v0;
    v0 = sub_653C(arg1);
    if (v0 < 0)
        return v0;
    v0 = applyRelocs(arg1, sp); // Apply relocations
    if (v0 < 0)
        return v0;
    t3 = 0;
    t0 = *(int*)(arg1 + 28);
    t1 = *(int*)(arg1 + 132);
    v1 = *(int*)(t0 + 28);
    s3 = (t1 << 5) + t0 + v1;                                                                                                                                       
    v1 = s3 - 32;                                                                                                                                                   
    a1 = *(int*)(v1 + 20);
    a0 = *(int*)(v1 + 16);
    t2 = 0;
    if (a0 < a1)
    {
        t2 = a1 - a0;
        a1 = *(int*)(arg1 + 36) + *(int*)(v1 + 8);
        t3 = a1 + a0;
    }
    // 5DB0
    t0 = 0;
    if (t1 != 0)
    {
        a3 = sp;

        // 5DBC
        do
        {
            a1 = *(int*)(a3 + 16);
            a2 = *(int*)(a3 + 0);
            v1 = *(int*)(a3 + 32);
            if (a2 != a1)
            {
                s3 = v1;
                if (a2 >= a1)
                {
                    // 5EC4
                    s3 &= 0xFFFFFFFC;
                    a0 = a1 + s3;
                    a2 += s3;
                    // 5ED8
                    while (a1 < a0)
                    {                                                                                                                                             
                        a0 -= 4;
                        t9 = *(int*)a0;
                        a2 -= 4;
                        *(int*)a2 = t9;
                    }
                }
                else
                {
                    v1 &= 0xFFFFFFFC;
                    a0 = a1 + v1;
                    // 5DEC
                    while (a1 < a0)
                    {
                        t6 = *(int*)a1;
                        a1 += 4;
                        *(int*)a2 = t6;
                        a2 += 4;
                    }
                }
            }

            // (5E04)
            t0++;
            // 5E08
            a3 += 4;
        } while (t0 < t1);
    }

    // 5E14
    a3 = t3 & 3;
    if (t2 != 0)
    {
        a0 = t3;
        v1 = t2;
        if (a3 != 0)
        {
            // 5E28
            do
            {
                *(char*)a0 = 0;
                a0++;
                t1 = a0 & 3;
                v1--;
            } while (t1 != 0);
        }

        // 5E3C
        t2 = v1 + 3;
        v1 = t2 >> 2;
        v0 = a0;
        // 5E4C
        while (v1 != 0)
        {
            v1--;
            *(int*)v0 = 0;
            v0 += 4;
        }
    }

    // 5E5C
    if (arg2 == 0)
        return 0;
    if (*(int*)(arg1 + 84) == 0)
        return 0;
    a2 = *(int*)(arg1 + 48) - *(int*)(arg1 + 20);
    a0 = a2 + 0xFF;
    if (a2 > 0)
        a2 = (a0 + (((a0 >> 31) == 1) ? 0xFF : 0)) & 0xFFFFFF00;

    // 5E94
    v0 = *(arg3 + 60)(*(int*)(arg1 + 24), 0);
    if (v0 < 0)
        return v0;
    *(int*)(arg1 + 20) = *(int*)(arg1 + 48);
    return 0;
}

int sub_5678(int arg0, int arg1)
{
    int v = -1;
    if (*(int*)arg0 != 0x5053507E)
        return -1;
    // 56D0
    s0 = arg0 + 208;
    sub_17FC((*(char*)(arg0 + 208) << 24) | (*(char*)(arg0 + 209) << 16) | (*(char*)(arg0 + 210) << 8) | *(char*)(arg0 + 211));
    if (callback2 != NULL && callback2(arg0, arg1) != 0)
        return -1;
    // 5728
    if (sub_17FC((*(char*)(arg0 + 208) << 24) | (*(char*)(arg0 + 209) << 16) | (*(char*)(arg0 + 210) << 8) | *(char*)(arg0 + 211)) == 0)
        return -1;
    if (callback3 == NULL)
        return 0x8002013A;
    if (callback3(0, 0xBFC00200) != 0)
        return -1;
    if (callback1 == NULL)
        return 0x8002013A;
    if (callback1(arg0, arg1, &v) != 0)
        return -1;
    return v;
}

int sub_5EF8(Callback1 arg)
{
    callback1 = arg;
    return 0;
}

int sub_5F08(Callback2 arg)
{
    callback2 = arg;
    return 0;
}

int sub_5F18(Callback3 arg)
{
    callback3 = arg;
    return 0;
}

int sub_5F28(Callback4 arg)
{
    callback4 = arg;
    return 0;
}

int sub_5F38(Callback5 arg)
{
    callback5 = arg;
    return 0;
}

int sub_5F48(int arg0, int arg1)
{
    if (callback2 == NULL)
        return -1;
    if (callback2(arg0, arg1) != 0)
        return -1;
    return 0;
}

/* ELF file header */
typedef struct
{
    char e_ident[16];          //  0
    Elf32_Half    e_type;      // 16
    Elf32_Half    e_machine;   // 18
    Elf32_Word    e_version;   // 20
    Elf32_Addr    e_entry;     // 24
    Elf32_Off     e_phoff;     // 28
    Elf32_Off     e_shoff;     // 32
    Elf32_Word    e_flags;     // 36
    Elf32_Half    e_ehsize;    // 40
    Elf32_Half    e_phentsize; // 42
    Elf32_Half    e_phnum;     // 44
    Elf32_Half    e_shentsize; // 46
    Elf32_Half    e_shnum;     // 48
    Elf32_Half    e_shstrndx;  // 50
} Elf32_Ehdr;

/* Segment header */
typedef struct
{
    Elf32_Word p_type;   //  0
    Elf32_Off  p_offset; //  4
    Elf32_Addr p_vaddr;  //  8
    Elf32_Addr p_paddr;  // 12
    Elf32_Word p_filesz; // 16
    Elf32_Word p_memsz;  // 20
    Elf32_Word p_flags;  // 24
    Elf32_Word p_align;  // 28
} Elf32_Phdr;

// 6014
int sub_6014(int arg)
{   
    *(int*)(arg + 72) = 0;
    int bufAddr = *(int*)(arg + 28);
    int header = *(int*)(bufAddr + 0);
    if (header == 0x4543537E) // ~SCE
        return 0x80020148;
    if (bufAddr & 0x3F != 0)
        return 0x80020148;
    if (header != 0x5053507E) // ~PSP
        return 0x80020148;
    if (*(int*)(bufAddr + 52) >= 0)
        return 0x80020148;
    if (*(char*)(bufAddr + 124) == 0)
        return 0x80020148;

    short unk = *(unsigned short*)(bufAddr + 6);
    if (unk != 0)
    {   
        if (unk & 0xE != 0)
            return 0x80020148;
        if (unk & 0xF0FE != 0)
            return 0x80020148;
        unk &= 0xF00;
        if (unk == 0)
            return 0x80020148;
        if (unk != 512)
            return 0x80020148;
    }
    
    // 60CC
    *(int*)(arg + 72) = 1;
    *(int*)(arg + 16) = *(int*)(bufAddr + 44);
    *(short*)(arg + 88) = *(short*)(bufAddr + 4);
    *(int*)(arg + 92) = *(int*)(bufAddr + 40);
    *(short*)(arg + 90) = *(short*)(bufAddr + 6);
    return 0;
}

// 60FC
int sub_60FC(int arg0, int arg1)
{   
    char *file;
    if (*(int*)(arg0 + 72) & 1 == 0)
        return 0;
    if (*(int*)(arg0 + 8) != 0)
        return 0x80020148;
    file = (char*)(arg0 + 28);
    if (file[124] == 0 || file[124] == 2)
    {   
        // 617C
        if (callback1 == NULL
         || callback2 == NULL
         || callback3 == NULL)
            return 0x8002013A;

        // 61B4
        int ret = callback2(file, *(int*)(arg0 + 16));
        if (ret != 0)
            return 0x80020148;

        if (sub_17FC((file[208] << 24)
                   | (file[209] << 16)
                   | (file[210] <<  8)
                   | (file[211] <<  0)) == 0)
            return 0x80020148;

        // ?????
        if (sub_17FC((file[208] << 24)
                   | (file[209] << 16)
                   | (file[210] <<  8)
                   | (file[211] <<  0)) != 0)
        {   
            int ret = callback3(0, 0xBFC00200);
            if (ret != 0)
                return ret;
        }
        // 625C
        ret = callback1(file, *(int*)(arg0 + 16), arg1);
        *(int*)(arg0 + 72) |= 2;
        return ret;
    }
    return 0x80020148;
}

int sub_6284(int arg0, int arg1)
{
    char *file;
    int start, end;

    if (arg1 == 0)
        return 0;

    file = *(int*)(arg0 + 28); // get file

    if (*(int*)file != *(int*)"~PSP") // Normal ELF file
    {
        Elf32_Ehdr *header = (Elf32_Ehdr*)file;
        // 62F8
        int i = header->e_phnum;
        Elf32_Phdr *prog = file + header->e_phoff;
        start = prog->p_vaddr;
        end = 0;
        while (i != 0)
        {
            // 6314
            if (prog->p_type == 1)
            {
                // 6330
                int progEnd = prog->p_vaddr + prog->p_memsz;
                if (progEnd > end)
                    end = progEnd;
                if (prog->p_vaddr < start)
                    start = prog->p_vaddr;
            }
            // 6320
            prog++;
            i--;
        }
    }
    else
    {
        // ~PSP file, no idea how it works
        int num = *(char*)(arg0 + 39);
        int i;
        char *file;

        start = *(int*)(arg0 + 68);
        end = 0;
        i = 0;
        file = arg0;
        while (i < num)
        {
            // 62C0
            unsigned int progStart = *(int*)(file + 68);
            unsigned int progSize  = *(int*)(file + 84);
            unsigned int progEnd   = progStart + progSize;

            if (progEnd > end)
                end = progEnd;
            if (progStart < start)
                start = progStart;

            file += 4;
            i++;
        }
    }
    return end - start;
}

char loadCoreStr[] = "sceLoaderCore"; // 0x88611E44

// 6350
int sub_6350(int arg0, int arg1, int arg2, int arg3)
{   
    if (arg2 != 0)
    {   
        int cb1, cb2, val, ret;
        // 6394
        cb1 = *(int*)(arg3 + 52);
        if (cb1 == 0)
            return -1;
        cb2 = *(int*)(arg3 + 56);
        if (cb2 == 0)
            return -1;
        if (*(int*)(arg3 + 60) == 0)
            return -1;

        val = unkVar;
        if (val < arg1) {
            unkVar = arg1;
            val = arg1;
        }
        
        // 63D8
        if (val == 0)
            return -1;
        int (*func)(int, int, int, int, int) = cb1;
        ret = func(1, loadCoreStr, 0, val, 0);
        if (ret < 0)
            return ret;

        val = unkVal;
        *(int*)(arg0 + 24) = ret;
        *(int*)(arg0 + 20) = val;
        int (*func)(int, int) = cb2;
        *(int*)(arg0 + 36) = func(v0, val);
        return 0;
    }
    unkVar = *(int*)(arg3 + 84);
    return 0;
}

// 6428
char kl4eStr[] = "KL4E"; // 0x88611E54

int sub_6428(int arg)
{   
    char *buf = *(int*)(arg + 28);
    int i, ret, var;

    if (*(int*)(arg + 36))
        return -1;
    if (*(int*)(arg + 72) & 1 == 0) {
        *(arg + 84) = 0;
        return 0;
    }
    
    var = *(unsigned short*)(arg + 90);
    if (var & 0x200 == 0) {
        *(int*)(arg + 84) = 0;
        return 0;
    }
    if (var & 0xF00 != 0x200)
        return 0x80020148;

    // 6484
    for (i = 0; i < 4; i++)
    {   
        if (buf[i] != kle4eStr[i])
        {   
            buf += 128;
            
            // 64A8
            for (i = 0; i < 4; i++)
                if (buf[i] != kl4eStr[i])
                    return 0x80020148;
        }
    }
    
    ret = sub_1DF8(*(int*)(arg + 36), *(int*)(arg + 92), buf + 4, 0);
    if (ret < 0)
        return ret;
    if (ret != *(int*)(arg + 92))
        return 0x80020148;

    *(int*)(arg + 84) = 1;
    *(int*)(arg + 28) = *(int*)(arg + 36);
    *(int*)(arg + 96) = 1;
    return 0;
}

// 653C
void sub_653C(int arg)
{   
    int bufPtr = *(arg + 28);
    if (*(short*)(bufPtr + 4) != 257)
        return 0x80020148;

    // 6584
    if (*(int*)(bufPtr + 16) != 0x8FFA0)
        return 0x80020148;
    if (*(int*)(bufPtr + 28) < 52)
        return 0x80020148;
    if ((unsigned int)(*(int*)(bufPtr + 32)) - 1 < 51)
        return 0x80020148;

    v0 = sub_68E8(bufPtr, arg);
    if (v0 < 0)
        return v0;
    v0 = sub_6A44(bufPtr, arg);
    if (v0 < 0)
        return v0;

    a0 = *(int*)(arg + 76);
    *(int*)(arg + 40) = *(int*)(bufPtr + 24);
    if (a0 == 0)
        return 0x80020148;
    v1 = bufPtr + a0;
    *(int*)(arg + 80) = v1;
    t9 = *(int*)(v1 + 36);
    t8 = *(int*)(v1 + 44);
    a3 = *(short*)(arg + 88) | (*(short*)(v1 + 0) & 0xE1FF);
    *(int*)(arg + 116) = *(int*)(v1 + 40) - t9;
    *(int*)(arg + 124) = *(int*)(v1 + 48) - t8;
    *(int*)(arg + 112) = t9;
    *(int*)(arg + 120) = t8;
    *(short*)(arg + 88) = a3;
    if (*(int*)(arg + 72) & 1 == 0)
        return 0;
    if (a3 & 0x1E00 != 0x1000)
        return 0x80020148;
    return 0;
}

// 68E8
void sub_68E8(int arg0, int arg1)
{   
    t3 = *(short*)(arg0 + 44);
    t4 = arg1;
    a1 = t3 & 0xFFFF;
    a3 = arg0 + *(int*)(arg0 + 28);
    t5 = 0;
    t0 = 0;
    if (a1 != 0)
    {   
        a0 = a3;
        
        // 6910
        do
        {   
            v1 = *(int*)(a0 + 4);
            t0++;
            if (v1 != 0 && ((v1 > 0x2000000) || (*(int*)(a0 + 16) + v1 > 0x2000000)))
                return 0x80020148;
            
            // 694C
            a0 += 32;
        } while (t0 < a1);
    }
    
    // 6954
    t1 = t3 & 0xFFFF;
    if (t1 <= 0)
        return 0x80020148;

    // 6978
    while (*(int*)(a3 + 0) != 1)
    {   
        t1--;
        a3 += 32;
        if (t1 <= 0)
            return 0x80020148;
    }
    
    // 698C
    if (t1 <= 0)
        return 0x80020148;

    if (*(int*)(a3 + 0) == 1)
    {   
        v0 = *(int*)(a3 + 12)
        // 69B4
        if (v0 >= 0)
            return 0x80020148;
        v1 = *(int*)(a3 + 8);
        if (v0 == v1)
            return 0x80020148;
        v0 &= 1;
        t2 = v1;
        t0 = 0;
        *(int*)(t4 + 76) = v0;
        // 69E0
        while ((t0++) < t1 && *(int*)(a3 + 0) == 1)
        {                                                                                                                                                         
            t6 = *(int*)(a3 + 8);
            t3 = *(int*)(a3 + 28);
            a1 = t6 + *(int*)(a3 + 20);
            a3 += 32;
            if (t5 < a1)
                t5 = a1;
            if (t6 < t2)
                t2 = t6;
            if (*(int*)(t4 + 188) < t3)
                *(int*)(t4 + 188) = t3;
        }
        // 6A34
        *(int*)(t4 + 48) = t5 - t2;
        return 0;
    }
    return 0x80020148;
}

// 6A44
void sub_6A44(int arg0, int arg1)
{   
    t3 = *(short*)(a0 + 48);
    t1 = *(int*)(a0 + 32);
    v1 = t3 & 0xFFFF;
    a3 = arg0 + t1;
    if (v1 != 0)
    {   
        a0 = *(short*)(a0 + 50);
        v0 = a0 & 0xFFFF;
        t6 = a0 & 0xFFFF;
        if (v0 != 0 && v0 >= v1)
            return 0x80020148;

        // 6A94
        t4 = t6 * 40 + a3;
        a2 = t3 & 0xFFFF;
        t2 = *(int*)(t4 + 20);
        v1 = arg0 + *(int*)(t4 + 16);
        *(int*)(a1 + 128) = v1;
        s1 = 0;
        if (a2 != 0)
        {   
            s0 = a3;
            // 6AC8
            do
            {   
                t0 = *(int*)(s0 + 0);
                a0 = 0x2000000;
                if (t2 < t0)
                {   
                    // 6D38
                    printf("%4d/%4d: max 0x%08x < sh_name 0x%08x\n", s1, a2, t2, t0);
                    return 0x80020148;
                }
                a3 = *(int*)(s0 + 16);
                if (a0 < a3)
                {   
                    // 6C84
                    // Strings stored in module's memory
                    printf("ehdr->e_shoff     0x%08x\n", *(int*)(arg0 + 32));
                    printf("%4d/%4d: sh_name      0x%08x\n", s1, *(short*)(arg0 + 48), *(int*)(s0 + 0));
                    printf("%4d: sh_type      0x%08x\n", s1, *(int*)(s0 +  4));
                    printf("%4d: sh_flags     0x%08x\n", s1, *(int*)(s0 +  8));
                    printf("%4d: sh_offset    0x%08x\n", s1, *(int*)(s0 + 16));
                    printf("%4d: sh_size      0x%08x\n", s1, *(int*)(s0 + 20));
                    printf("%4d: sh_addralign 0x%08x\n", s1, *(int*)(s0 + 32));
                    printf("%4d: sh_entsize   0x%08x\n", s1, *(int*)(s0 + 36));
                    printf("%4d: 0x%08x < sh_offset 0x%08x\n", s1, 0x2000000, *(int*)(s0 + 16));
                    return 0x80020148;
                }
                a3 += *(int*)(s0 + 20);
                s0 += 40;
                if (a0 < a3)
                {                                                                                                                                                   
                    // 6C64                                                                                                                                         
                    printf("%4d: 0x%08x < sh_offset+sh_size 0x%08x\n", s1, 0x2000000, a3);
                    return 0x80020148;
                }
                s1++;
            } while (s1 < a2);
        }
    }

    // 6B08
    *(int*)(a1 + 52) = 0;
    *(int*)(a1 + 56) = 0;                                                                                                                                         
    *(int*)(a1 + 60) = 0;
    if (t1 != 0)
    {
        // 6BBC
        v0 = t3 & 0xFFFF;
        a3 = arg0 + t1;
        if (v0 == 0)
            return 0;
        s1 = v0;
        // 6BE0
        do
        {
            char goto6BF8 = 0;
            if (*(int*)(a3 + 4) == 1)
            {
                v0 = *(int*)(a3 + 8);
                // 6C28
                if (v0 == 6 || v0 == 2)
                {
                    // 6C54
                    *(int*)(a1 + 52) += *(int*)(a3 + 20);
                    goto6BF8 = 1;
                }

                if (v0 == 3)
                {
                    *(int*)(a1 + 56) += *(int*)(a3 + 20);
                    goto6BF8 = 1;
                }
            }

            if (!goto6BF8)
            {
                // 6BF0 & 6C0C
                if (*(int*)(a3 + 4) == 8 && *(int*)(a3 + 8) == 3)
                    *(int*)(a1 + 60) += *(int*)(a3 + 20);
            }
            // 6BF8
            s1--;

            // 6BFC
            a3 += 40;
        } while (s1 != 0);
        return 0;
    }
    t1 = *(int*)(arg0 + 28);
    s1 = 0;
    a0 = arg0 + t1;

    // 6B34
    while (s1 < *(short*)(arg0 + 44))
    {   
        if (*(int*)(a0 + 0) == 1)
        {   
            // 6B70
            if (*(int*)(a0 + 24) == 0)
            {   
                // 6BAC
                v1 = *(int*)(a0 + 16);                                                                                                                            
                *(int*)(a1 + 56) += v1;
            }
            else
            {
                v1 = *(int*)(a0 + 16);
                *(int*)(a1 + 52) += v1;
            }
            // 6B8C
            t7 = *(int*)(a0 + 20);
            if (v1 < t7)
                *(int*)(a1 + 60) += t7 - v1;
        }

        // 6B40
        s1++;
        a0 += 32;
    } while (s1 < a3);
    return 0;
}

// 666C
void applyRelocs(int arg0, int arg1) // 666C
{
    t5 = arg1;
    s1 = arg0;
    t6 = *(int*)(a0 + 28);
    v1 = *(int*)(t6 + 28);
    t1 = *(short*)(t6 + 44);
    s0 = t6 + v1;
    while (*(int*)s0 != 1 && t1 > 0)
    {
        // 66AC
        t1--;
        s0 += 32;
    }

    // 66C0
    v1 = *(int*)(s0 + 4);
    t3 = *(int*)(s1 + 36);
    a3 = *(int*)(s0 + 16);
    a2 = *(int*)(s0 + 28);
    t7 = 1;
    t0 = t6 + v1;
    *(int*)(t5 + 0) = t3;
    t4 = 1;
    *(int*)(t5 + 16) = t0;
    *(int*)(t5 + 32) = a3;
    *(int*)(t5 + 48) = a2;
    if (t1 <= 1)
        return 0x80020148;
    t8 = *(int*)(s0 + 32);
    t3 = 32;
    if (t8 == t7)
    {
        // 687C
        t0 = t5 + 4;
        do
        {
            a2 = t3 + s0;
            // 6888
            a0 = *(int*)(s1 + 36);
            t2 = *(int*)(a2 + 4);
            v1 = *(int*)(a2 + 8);
            t9 = *(int*)(a2 + 28);
            a3 = *(int*)(a2 + 16);
            t4++;
            a1 = t4 << 5;
            t3 = a1;
            v0 = a1 + s0;
            a2 = a0 + v1;
            a1 = t6 + t2;
            t2 = t4 < t1;
            *(int*)(t0 + 0) = a2;
            t7++;
            *(int*)(t0 + 16) = a1;
            *(int*)(t0 + 32) = a3;
            *(int*)(t0 + 48) = t9;
            t0 += 4;
            if (t2 == 0)
                return 0x80020148;
            t9 = *(int*)v0;
        } while (t9 == 1);
    }

    // 6700
    t0 = t4 << 5;
    if (t2 == 0)
        return 0x80020148;
    t2 = s0 + t0;
    t0 = *(int*)(t2 + 0);
    t3 = 0x8FFFFF60 + t0;
    t1 = t3 < 2;
    if (t1 == 0)
        return 0x80020148;
    *(int*)(s1 + 132) = t4;
    a1 = 1;
    *(int*)(s1 + 136) = *(int*)(t5 + 0);
    *(int*)(s1 + 152) = *(int*)(s0 + 20);
    *(int*)(s1 + 172) = (int*)(t5 + 48);
    v1 = a1 << 2;

    // 674C
    do
    {
        t1 = a1 << 5;
        t9 = a1 < t4;
        a3 = v1 + t5;
        a2 = t1 + s0;
        a0 = v1 + s1;
        if (t9 == 0)
        {
            // 6860
            *(int*)(a0 + 172) = 0;
            *(int*)(a0 + 136) = 0;
            *(int*)(a0 + 152) = 0;
        }
        else
        {
            *(int*)(a0 + 136) = *(int*)(a3 + 0);
            *(int*)(a0 + 152) = *(int*)(a2 + 20);
            *(int*)(a0 + 172) = *(int*)(a3 + 48);
        }

        // 677C
        a1++;
        a3 = a1 < 4;
        v1 = a1 << 2;
    } while (a3 != 0);
    t4 = *(int*)(t5 + 0);
    a3 = *(int*)(s1 + 40);
    t3 = *(int*)(s1 + 112);
    t1 = *(int*)(s1 + 120);
    t8 = a3 + t4;
    *(int*)(s1 + 40) = t8;
    t4 = 0x700000A0;
    v1 = *(int*)(t5 + 0);
    a2 = t3 + v1;
    *(int*)(s1 + 112) = a2;
    a1 = *(int*)(t5 + 0);
    t9 = t1 + a2;
    *(int*)(s1 + 120) = t9;
    // Apply section relocations
    if (t0 == t4)
    {
        // 683C
        t0 = *(int*)(t2 + 16);
        applySectionRelocs(s1, t5, t6 + *(int*)(t2 + 4), t0 >> 3, t0);
    }
    // Apply segment relocations
    else
    {
        if (t0 != 0x700000A1)
            return 0x80020001;
        v0 = applySegmentRelocs(s1, s5, t7, t6 + *(int*)(t2 + 4), *(int*)(t2 + 16), *(int*)(t2 + 24));
    }

    // 67F4
    if (v0 < 0)
        return v0;
    *(int*)(s1 + 80) = *(int*)(s1 + 36) + *(int*)(s1 + 76) - *(int*)(s0 + 4);
    return 0;
}

// 6D54
int applySectionRelocs(int arg0, int arg1, int arg2, int arg3, int arg4)
{
    s6 = a0;
    s5 = 0;
    s4 = a3;
    s2 = a1;
    s1 = a2;
    if (a3 <= 0)
        return 0;
    do
    {
        v1 = *(int*)(s1 + 4);
        s3 = 1;
        s0 = *(char*)(s1 + 4);
        a0 = (v1 >> 8) & 0xFF;
        a1 = v1 >> 16;
        a2 = *(int*)(s6 + 132);
        t0 = a0 << 2;
        v1 = t0 + s2;
        a3 = *(int*)s1;
        newa2 = *(int*)(v1 + 32);
        if (a0 < 0
         || (a0 >= a2) || (a1 >> 31)
         || a1 >= a2
         || a3 >= newa2) {
            printf("../loadcore/loadelf_boot.c:%s:internal error !\n", "ApplyPspRelSection");
            return 0x80020001;
        }
        a2 = newa2;
        t5 = *(int*)(v1 + 16);
        v0 = a1 << 2;
        t4 = v0 + s2;
        t3 = s0 < 9;
        a1 = a3 + t5;
        t2 = *(int*)t4;

        /* /!\ a1 is unaligned && __attribute__ ((packed))  (lwl & lwr are used) */
    
        switch (s0)
        {
        case 0:
            t3 = 8;
            break;
    
        case 1:
            // 6E18
            a2 = *(int*)a1;
            t8 = (int)(short)(a2 & 0xFFFF);
            s0 = t2 + t8;
            v0 = s0 & 0xFFFF;
            a2 &= 0xFFFF0000;
            v0 |= a2;
            *(int*)a1 = v0;
            t3 = 8;
            break;
    
        case 2:
            t9 = *(int*)a1;
            v0 = t9 + t2;
            *(int*)a1 = v0;
            t3 = 8;
            break;
    
        case 4:
            // 6E8C
            a2 = *(int*)a1;
            a0 = t0 + s2;
            t4 = *(int*)a0;
            v1 = a2;
            v1 &= 0x3F;
            t3 = a3 + t4;
            t1 = v1 << 2;
            t3 &= 0xF0000000;
            t0 = t1 | t3;
            a3 = t0 + t2;
            v0 = (a3 >> 2) & 0x3FFFFFF;
            a2 &= 0xFC000000;
            v0 |= a2;
            *(int*)a1 = v0;
            t3 = 8;
            break;
    
        case 5:
            // 6EC4
            t5 = *(int*)a1; // /!\ unaligned reading
            t0 = s5 + 1;
            a2 = t0 < s4;
            t1 = t5 << 16;
            t3 = 8;
            if (a2 != 0)
            {
                a0 = s1 + 8;
                v0 = *(char*)(a0 + 4);
                a1 = 5;
                t6 = s3 << 3;
                if (v0 == a1)
                {
                    // 6F84
                    a2 = t6 + s1;
                    a3 = 5;
                    // 6F8C
                    do
                    {
                        v1 = *(char*)(a0 + 5);
                        t9 = *(int*)(a0 + 0);
                        t4 = v1 << 2;
                        t3 = t4 + s2;
                        t8 = *(int*)(t3 + 16);
                        s0 = t9 + t8;
                        v0 = *(int*)s0; // /!\ unaligned reading
                        t0++;
                        s3++;
                        t7 = t0 < s4;
                        a2 += 8;
                        t3 = s3 << 3;
                        if (t7 == 0)
                            break;
                        t5 = *(char*)(a2 + 4);
                        a0 = a2;
                    } while (t5 == a3);
                }
            }
            // (6EF4)
            t4 = t3 + s1;
            // 6EF8
            t9 = *(char*)(t4 + 5);
            t6 = *(int*)(t4 + 0);
            t8 = t9 << 2;
            s0 = t8 + s2;
            t7 = *(int*)(s0 + 16);
            v0 = t6 + t7;
            a1 = *(int*)v0; // /!\ unaligned reading
            a2 = (int)(short)(a1 & 0xFFFF);
            a0 = t1 + a2;
            t0 = a0 + t2;
            a3 = t0 >> 15;
            t2 = a3 + 1;
            t1 = (t2 >> 1) & 0xFFFF;
            t0 = 0;
            if (s3 > 0)
            {
                a3 = s1;
    
                // 6F3C
                do
                {
                    s0 = *(char*)(a3 + 5);
                    a1 = *(int*)(a3 + 0);
                    t7 = s0 << 2;
                    t6 = t7 + s2;
                    a2 = *(int*)(t6 + 16);
                    t5 = a1 + a2;
                    a0 = *(int*)t5; // /!\ unaligned reading
                    a0 &= 0xFFFF0000;
                    t2 = a0 | t1;
                    *(int*)t5 = t2;
                    t0++;
                    v1 = t0 < s3;
                    a3 += 8;
                } while (v1 != 0);
            }
            break;
    
        case 6:
            // 6FD8
            t1 = *(int*)a1;
            t0 = (int)(short)(t1 & 0xFFFF);
            a3 = t2 + t0;
            t3 = a3 & 0xFFFF;
            t1 &= 0xFFFF0000;
            v0 = t1 | t3;
            *(int*)a1 = v0;
            t3 = 8;
            break;
    
        case 8:
            // 702C
            printf("********************\n");
            printf("PSP cannot load this image\n");
            printf("unacceptable relocation type: 0x%x\n", s0);
            t3 = 8;
            break;
    
        default:
            printf("********************\n");
            printf("PSP cannot load this image\n");
            printf("unacceptable relocation type: 0x%x\n", s0);
            return 0x80020148;
        }
    
        // (6E40)
        s5 += s3;
        // 6E44
        s3 = s5 < s4;
        s1 += t3;
    } while (s3 != 0);
    return 0;
}

