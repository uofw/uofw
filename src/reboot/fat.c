#include "reboot.h"

int unkVar = -1; // 0x886128F4
int unkVar2 = -1; // 0x886128F8

int unkFatAddr; // 0x88630C4C
int fatUnitsNumber; // 0x88630C50
int unkFatAddr2; // 0x88630C54
int unkFatCurAddr; // 0x88630C60
int unkFatVar1; // 0x88630C64
int unkFatVar2; // 0x88630C68
int unkFatVar3; // 0x88630C6C
int unkFatVar4; // 0x88630C70
int unkFatMask; // 0x88630C78
int unkCounter; // 0x88638D7C

typedef struct
{
    // Size: 628
} UnitStruct;
UnitStruct unkUnitTable[52]; // 0x88630D7C; TODO: it isn't an exact value!! (actually, it's more than 52); total size: 0x8000

int sub_7D74(int arg0, int arg1)
{
    s1 = arg0 - 1;
    s0 = arg1 + 4;
    int numUnit = 4;
    int numNode = 16;
    int numBuf = 4;
    int unkSp12;
    // 7DB8
    while (s1 > 0)
    {
        a2 = 0;
        if (strncmp("-unit=", *(int*)(s0 + 0), 6) != 0)
        {
            // 802C
            if (strncmp("-node=", *(int*)(s0 + 0), 6) != 0)
            {
                // 80A0
                if (strncmp("-buf=", *(int*)(s0 + 0), 5) != 0)
                {
                    t3 = *(int*)(s0 + 0);
                    // 8110
                    if (*(char*)t3 != 0)
                        return 1;
                }
                else
                {
                    int count;
                    int addr = *(int*)(s0 + 0) + 5;
                    // 80CC
                    while (*(char*)addr != 0)
                    {
                        count *= 10;
                        count += *(char*)addr - '0';
                        if (*(char*)addr - '0' >= 10)
                            return 1;
                        addr++;
                    }
                    // 8108
                    numBuf = count;
                }
            }
            else
            {
                int count;
                int addr = *(int*)(s0 + 0) + 6;
                // 805C
                while (*(char*)addr != 0)
                {  
                    count *= 10;
                    count += *(char*)addr - '0';
                    if (*(char*)addr - '0' >= 10)
                        return 1;
                    addr++;
                }
                // 8098
                numNode = count;
            }
        }
        else
        {
            int count;                     
            int addr = *(int*)(s0 + 0) + 6;
            // 7DE0
            while (*(char*)addr != 0)
            {               
                count *= 10;           
                count += *(char*)addr - '0';
                if (*(char*)addr - '0' >= 10)
                    return 1;
                addr++;
            }      

            // 7E1C
            numUnit = count;
        }
        // 7E20
        s1--;
        s0 += 4;
    }
    // 7E2C
    if (numUnit < 4)
        return 1;
    if (numNode < 16)
        return 1;
    if (numBuf < 4)
        return 1;
    // 7E7C
    unkVar = 1;
    unkVar2 = 1;
    unkSp12 = 0;
    if ((unkCounter >= 2) || (numUnit * 628 + numNode * 84 + numBuf * 536 > 0x8000))
        for (;;) // 7EDC
            ;
    // 7EE4
    unkSp12 = unkUnitTable;
    v1 = unkSp12;
    unkCounter++;
    if (v1 == 0)
        return 1;
    a1 = v1;
    fatUnitsNumber = numUnit;
    a0 = numUnit - 1;
    unkFatAddr = v1;
    unkFatAddr2 = v1;
    if (numUnit > 0)
    {
        // 7F2C
        do
        {
            t0 = a0;
            *(int*)a1 = 0;
            a0--;
            a1 += 628;
        } while (t0 > 0);
    }
    // 7F48
    a0 = a1;
    unkFatCurAddr = a1;
    v1 = numNode - 1;
    unkSp12 = a1;
    if (numNode > 0)
    {
        do
        {
            v0 = a0 + 84;
            a2 = v1;
            if (v1 == 0)
                a1 = 0;
            else
                a1 = v0;
            // 7F74
            *(int*)a0 = a1;
            v1--;
            a0 = v0;
        } while (a2 > 0);
    }
    // 7F84
    t0 = a0;
    v1 = a0;
    unkSp12 = a0;
    a2 = a0 + numBuf * 40;
    a1 = numBuf - 1;
    if (numBuf > 0)
    {
        // 7FB4
        do
        {
            a0 = v1 + 24;
            a3 = a1;
            v0 = a0;
            if (a1 == 0) {
                unkFatVar2 = v1;
                v0 = 0;
            }
            // 7FCC
            *(int*)v1 = v0;
            a1--;
            v0 = v1 - 24;
            if (v1 == t0)
            {
                // 8020
                unkFatVar1 = v1;
                v0 = 0;
            }
            // 7FDC
            *(int*)(v1 + 20) = a2;
            a2 += 512;
            *(int*)(v1 + 4) = v0;
            *(int*)(v1 + 12) = 0;
            *(int*)(v1 + 16) = -1;
            v1 = a0;
        } while (a3 > 0);
    }
    // 7FF8
    unkFatVar3 = unkFatVar1;
    unkFatVar4 = unkFatVar2 + 24;
    unkSp12 = a2;
    return 0;
}

char defaultName[] = "fatfs"; // 0x8861209C
char defaultUnit[] = "unit=16"; // 0x886120A4
char defaultNode[] = "node=16"; // 0x886120B0
char defaultBuf[]  = "buf=4"; // 0x886120BC

int globDefaultOpt[4] = // 0x886120C4
{
    defaultName,
    defaultUnit,
    defaultNode,
    defaultBuf
};

int sub_9A84(int arg0, int arg1)
{
    int defaultOpt[4];
    memcpy(defaultOpt, globDefaultOpt, 16); // Copy default options
    if (unkFatMask & 1 == 0)
    {
        // 9AEC
        v0 = sub_C934(sub_CBC0(arg0, arg1) | 0xD);
        if (v0 != 0)
            return v0;
        v0 = sub_7D74(4, defaultOpt);
        if (v0 != 0)
            return v0;
        unkFatMask |= 1;
    }
    return 0;
}

