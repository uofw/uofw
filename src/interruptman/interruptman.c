/* Copyright (C) 2011, 2012 The uOFW team
   See the file COPYING for copying permission.
*/

#include "../global.h"

#include "../exceptionman/exceptionman.h"

#include "interruptman.h"

extern int sub_0038();
extern int sub_091C();
extern int sub_0A74();
extern int sub_3230();
extern int sub_0924();
extern int sub_0CF8();
extern int sub_0000();
extern int sub_3110();
extern int sub_3160();
// real name is memset but gcc always puts a warning about that
void *my_memset(void *dst, char c, int n);
void sub_2A40(int intrNum);

extern int sub_0CC0();
extern int sub_0864();
extern int sub_0ECC();
extern int sub_0EF8();
extern int sub_0F20();
extern int sub_0F58();
extern int sub_0FE4();
extern int sub_0FF0();
extern int sub_0F64();
extern int sub_0FA4();
extern int sub_0FF8();
extern int sub_1000();
extern int sub_1008();
extern int sub_0EC0();
extern int sub_1010();
int sceKernelCpuSuspendIntr();
void sceKernelCpuResumeIntr();
void sub_1030();
void sub_1050(int out[2]);
int sceKernelSuspendIntr(int arg0, int arg1);
void sub_2968(int intrNum);
void sub_29B0(int intrNum);
int sceKernelIsIntrContext();
int sceKernelRegisterIntrHandler(int intrNum, int arg1, void *func, int arg3, SceIntrHandler *handler);
int sceKernelEnableIntr(int intNum);

char intrMgrStr[] = "InterruptManager"; // 0x36F4
char **intrMgrStrPtr = (char**)&intrMgrStr; // 0x33E8

// 0x3458
PSP_MODULE_INFO("sceInterruptManager", 0x1007, 1, 9);

char heapName[] = "SceInterruptManager"; // 0x3778

int devkitVer = 0x06030710;

typedef struct CbMap
{
    struct CbMap *next;
    int unk1, unk2, unk3;
    int (*callbacks[64])(void);
} CbMap;

CbMap emptyMap = {NULL, 0, 0, 0, {}}; // 0x38D0

CbMap cbMap = // 0x37C0
{
    &emptyMap, 0, 0x00000100, 0x00000110,
    {
    sub_0CC0, sub_0CC0, sub_0864, sub_0CC0,
    sub_0CC0, sub_0CC0, sub_0CC0, sub_0CC0,
    sub_0ECC, sub_0EF8, sub_0F20, sub_0F58,
    sub_0FE4, sub_0FF0, sub_0F64, sub_0FA4,
    sub_0FF8, sub_1000, sub_1008, sub_0EC0,
    sub_1010, sub_0CC0, sub_0CC0, sub_0CC0,
    sub_0CC0, sub_0CC0, sub_0CC0, sub_0CC0,
    sub_0CC0, sub_0CC0, sub_0CC0, sub_0CC0,
    sub_0CC0, sub_0CC0, sub_0CC0, sub_0CC0,
    sub_0CC0, sub_0CC0, sub_0CC0, sub_0CC0,
    sub_0CC0, sub_0CC0, sub_0CC0, sub_0CC0,
    sub_0CC0, sub_0CC0, sub_0CC0, sub_0CC0,
    sub_0CC0, sub_0CC0, sub_0CC0, sub_0CC0,
    sub_0CC0, sub_0CC0, sub_0CC0, sub_0CC0,
    sub_0CC0, sub_0CC0, sub_0CC0, sub_0CC0,
    sub_0CC0, sub_0CC0, sub_0CC0, sub_0CC0
    }
};

int wut; // 0x3B80 -> ???

int globUnk[80]; // 0x57C0

typedef struct
{
    int grpsOpt[23]; // 0x5900 -> 0x595C
    char unused2; // 0x5960
    char opt2; // 0x5961
    char opt; // 0x5962   
    char unused3; // 0x5963
    int unkVar1; // 0x5964
    int unkVar2; // 0x5968
    int unused; // 0x596C
    int intrStack; // 0x5970
    int intrStackArg; // 0x5974
    int unkVar3; // 0x5978
    int unkVar4; // 0x597C
    Interrupt intr[68]; // 0x5980
    int subIntrMemoryPoolId; // 0x6A80
    int unk[2]; // 0x6A84 - 0x6A88
    int count; // 0x6A8C
    int compare; // 0x6A90
} InterruptInfo;

InterruptInfo intInfo; // 0x5900

int unkTab[4]; // 0x58F0

extern int mod_0400, mod_0468, mod_0E48, mod_0E50;

void InterruptManagerForKernel_077B9BEE()
{
    intInfo.unk[0] = 0;
    intInfo.unk[1] = 0;
}

void InterruptManagerForKernel_0F9D0289(int (*arg0)(), int (*arg1)())
{
    int oldIc = sceKernelCpuSuspendIntr();
    *(int*)(mod_0468) = 0x0C000000 + (((int)arg1 >> 2) & 0x3FFFFFF);
    CACHE(0x1A, mod_0468);
    CACHE(0x08, mod_0468);
    *(int*)(mod_0400) = 0x0C000000 + (((int)arg0 >> 2) & 0x3FFFFFF);
    CACHE(0x1A, mod_0400);
    CACHE(0x08, mod_0400);
    sceKernelCpuResumeIntr(oldIc);
}

int InterruptManagerForKernel_11981C80(int intrNum)
{
    int sp[2];
    if (intrNum >= 64)
        return 0x80020065;
    sub_1050(sp);
    return (sp[intrNum >> 5] >> (intrNum & 0x1F)) & 1;
}

// 298C261F
int UnSupportIntr(int intrNum)
{
    if (intrNum >= 64)
        return 0x80020065;
    int oldIntr = sceKernelCpuSuspendIntr();
    sceKernelSuspendIntr(intrNum, 0);
    Interrupt *intr = &intInfo.intr[intrNum];
    intr->handler = 1;
    intr->v48 &= 0xFFFFFCFF;
    sceKernelCpuResumeIntr(oldIntr);
    return 0;
}

int InterruptManagerForKernel_30C08374()
{
    int ret;
    COP0_STATE_GET(ret, COP0_STATE_COUNT);
    return ret;
}

// 352FB341
int sceKernelGetUserIntrStack()
{
    if (intInfo.opt2 == 0)
        return intInfo.intrStack;
    int sp;
    COP0_CTRL_GET(sp, COP0_CTRL_SP_KERNEL);
    return *(int*)(sp + 180);
}

// 37BD0C9C
int sceKernelGetSyscallRA()
{
    int addr;
    COP0_CTRL_GET(addr, COP0_CTRL_TCB);
    if (addr != 0)
    {
        addr = *(int*)addr;
        if (addr != 0)
            return *(int*)(addr + 12);
    }
    return 0;
}

// 399FF74C
int sceKernelQuerySystemCall(int (*arg)())
{
    CbMap *map = &cbMap;
    // 3070
    do
    {
        int n1 = cbMap.unk3 - 16;
        int max = (n1 + (((u32)n1 >> 31) == 1) ? 3 : 0) >> 2;
        int i;
        // 3094
        for (i = 0; i < max; i++)
        {
            if (cbMap.callbacks[i] == arg)
            {
                // 30C0
                int n = cbMap.unk1;
                return ((n + (((u32)n >> 31) == 1) ? 3 : 0) >> 2) + i;
            }
        }
        // 30AC
        map = map->next;
    } while (map != NULL);
    return -1;
}

// 4E401A35
int sceKernelSetPrimarySyscallHandler(int arg0, int (*arg1)())
{
    int oldIntr = sceKernelCpuSuspendIntr();
    int shift = arg0 * 4;
    if (arg0 <= 0 || cbMap.unk2 + cbMap.unk3 < shift) {
        sceKernelCpuResumeIntr(oldIntr);
        return 0x80020038;
    }
    if (cbMap.callbacks[arg0] != cbMap.callbacks[0]) {
        sceKernelCpuResumeIntr(oldIntr);
        return 0x80020039;
    }
    // 2B68
    if ((int)arg1 >= 0)
    {
        // 2B8C
        sceKernelCpuResumeIntr(oldIntr);
        return 0x800200D3;
    }
    cbMap.callbacks[arg0] = arg1;
    CACHE(0x1A, &cbMap.callbacks[arg0]);
    sceKernelCpuResumeIntr(oldIntr);
    return 0;
}

// 57AA9A29
u64 sceKernelGetCpuClockCounterWide()
{
    int oldIntr = sceKernelCpuSuspendIntr();
    int v1 = intInfo.unkVar4;
    int st;
    COP0_STATE_GET(st, COP0_STATE_COUNT);
    if (st >= intInfo.unkVar3)
        st = intInfo.unkVar3;
    else
        v1++;
    // 2F68
    sceKernelCpuResumeIntr(oldIntr);
    return ((u64)v1 << 32) | st;
}

void InterruptManagerForKernel_6301702D(int arg0, int arg1)
{
    intInfo.unkVar2 = arg1;
    intInfo.unkVar1 = arg0;
}

int InterruptManagerForKernel_7539543F()
{
    if (intInfo.opt2 != 0)
    {
        int addr;
        COP0_CTRL_GET(addr, COP0_CTRL_TCB);
        if (addr != 0)
            return *(int*)addr;
    }
    return 0;
}

// 78D8F74D
int _sceKernelGetCpuClockCounterLow()
{
    return intInfo.unkVar3;
}

// 867DEBD1
int sceKernelSetIntrLevel(int intrNum, int num)
{
    if (intrNum >= 64)
        return 0x80020065;
    if (num < 1 || num >= 4)
        return 0x80020069;
    Interrupt *intr = &intInfo.intr[intrNum];
    int oldIntr = sceKernelCpuSuspendIntr();
    if (intr->handler == 0 || (intr->handler & 3) == 1)
    {
        // 1814
        sceKernelCpuResumeIntr(oldIntr);
        return 0x80020068;
    }
    if (((intr->v48 >> 11) & 1) != 0 && ((num ^ 2) != 0))
    {
        // 1800
        sceKernelCpuResumeIntr(oldIntr);
        return 0x80020069;
    }
    if (((intInfo.grpsOpt[(intrNum >> 5) + 24] >> (intrNum & 0x1F)) & 1) == 0)
    {
        // 17F4
        intr->v48 = (intr->v48 & 0xFFFFFCFF) | ((num << 8) & 0x00000300);
    }
    else
    {
        if (((intr->v48 >> 8) & 3) < num)
        {
            // 17E4
            sub_2968(intrNum);
        }
        // 17A8
        intr->v48 = (intr->v48 & 0xFFFFFCFF) | ((num << 8) & 0x00000300);
        sub_29B0(intrNum);
    }
    // 17B8
    sceKernelCpuResumeIntr(oldIntr);
    return 0;
}

// 9B434498
int _sceKernelGetCpuClockCounterHigh()
{
    return intInfo.unkVar4;
}

// 9E1C5490
int sceKernelResumeIntr(int intrNum, int arg1)
{
    if (intrNum >= 68)
        return 0x80020065;
    int oldIntr = sceKernelCpuSuspendIntr();
    int mask;
    if (intrNum == 64)
        mask = 0x100;
    else if (intrNum == 65)
        mask = 0x200;
    else if (intrNum == 66)
        mask = 0x1000;
    else if (intrNum == 67)
        mask = 0x8000;
    else
        mask = 0;
    // TODO: verify return value
    // 1B2C
    if (intInfo.intr[intrNum].handler != 0)
    {
        // 1BA8
        if (intrNum >= 64)
        {
            // 1BC8
            if (arg1 != 0)
            {
                int st;
                COP0_STATE_GET(st, COP0_STATE_STATUS);
                COP0_STATE_SET(COP0_STATE_STATUS, st | (mask & 0xFF00));
                sceKernelCpuResumeIntr(oldIntr);
                return 0;
            }
        }
        else
        {
            if (arg1 == 0)
            {
                // 1B98
                sub_2A40(intrNum);
            }
            else
                sub_29B0(intrNum);
            sceKernelCpuResumeIntr(oldIntr);
            return 0;
        }
    }
    else if (intrNum < 64)
    {
        // 1B98
        sub_2A40(intrNum);
        sceKernelCpuResumeIntr(oldIntr);
        return 0x80020068;
    }
    else
    {
        int st;
        COP0_STATE_GET(st, COP0_STATE_STATUS);
        COP0_STATE_SET(COP0_STATE_STATUS, st | ~(mask & 0xFF00));
        sceKernelCpuResumeIntr(oldIntr);
        return 0x80020068;
    }
    // 1B5C
    int st;
    COP0_STATE_GET(st, COP0_STATE_STATUS);
    COP0_STATE_SET(COP0_STATE_STATUS, st | ~(mask & 0xFF00));
    sceKernelCpuResumeIntr(oldIntr);
    return 0;
}

// A6A4981E
int *QueryInterruptManCB()
{
    return intInfo.grpsOpt;
}

// AA02AB07
int sceKernelSuspendIntr(int arg0, int arg1)
{
    int ret = 0;
    int mask;
    if (arg0 >= 68)
        return 0x80020065;
    int oldIc = sceKernelCpuSuspendIntr();
    if (arg0 == 64)
        mask = 0x100;
    else if (arg0 == 65)
        mask = 0x200;
    else if (arg0 == 66)
        mask = 0x1000;
    else if (arg0 == 67)
        mask = 0x8000;
    else
        mask = 0;
    // 19F8
    if (intInfo.intr[arg0].handler != 0)
    {
        // 1A78
        if (arg1 != 0)
        {
            if (arg0 >= 64)
            {
                // 1AA8
                int st;
                COP0_STATE_GET(st, COP0_STATE_STATUS);
                *(int*)arg1 = (st & 0xFF00) | mask;
            }
            else
                *(int*)arg1 = (intInfo.grpsOpt[(arg0 >> 5) + 24] >> (arg0 & 0x1F)) & 1;
        }
    }
    else
    {
        ret = 0x80020068;
        if (arg1 != 0)
            *(int*)arg1 = 0;
    }
    // 1A24
    if (arg0 < 64)
    {
        // 1A68
        sub_2A40(arg0);
    }
    else {
        int st;
        COP0_STATE_GET(st, COP0_STATE_STATUS);
        COP0_STATE_SET(COP0_STATE_STATUS, st & ~(mask & 0xFF00));
    }
    // 1A3C
    sceKernelCpuResumeIntr(oldIc);
    return ret;
}

// B1F5E99B
int QueryIntrHandlerInfo(int intrNum, int subIntrNum, int out)
{
    if (intrNum >= 68)
        return 0x80020065;
    if (*(int*)(out + 0) != 56)
        return 0x8002006B;
    int oldIntr = sceKernelCpuSuspendIntr();
    Interrupt *intr = &intInfo.intr[intrNum];
    char numSubIntr = intr->v48 & 0xFF;
    if (numSubIntr != 0)
    {
        if (subIntrNum < 0 || subIntrNum >= numSubIntr)
        {
            // (2918)
            // 291C
            if (subIntrNum != -1) {
                sceKernelCpuResumeIntr(oldIntr);
                return 0x80020065;
            }
        }
        else
        {
            if (intr->subIntrs == NULL)
                subIntrNum = 0;
            else
                intr = (Interrupt*)&intr->subIntrs[subIntrNum];
        }
    }
    // (2794)
    // 2798
    *(int*)(out + 8) = intr->arg;
    *(int*)(out + 4) = intr->handler;
    *(int*)(out + 12) = intr->loadCoreRes;
    *(short*)(out + 16) = intrNum;
    *(short*)(out + 20) = (intr->v48 >> 8) & 3;
    if (intrNum >= 64)
    {
        // 28D0
        int st;
        COP0_STATE_GET(st, COP0_STATE_STATUS);
        st &= 0xFF00;
        int mask;
        if (intrNum == 64)
            mask = 0x0100;
        else if (intrNum == 65)
            mask = 0x0200;
        else if (intrNum == 66)
            mask = 0x1000;
        else if (intrNum == 67)
            mask = 0x8000;
        else
            mask = 0;
        // 2908
        *(short*)(out + 22) = (st & mask) != 0;
    }
    else
    {
        char bit = (intInfo.grpsOpt[(intrNum >> 5) + 24] >> (intrNum & 0x1F)) & 1;
        *(short*)(out + 22) = bit;
        if ((bit & (~subIntrNum >> 31)) != 0)
        {
            if (intr->handler != 0)
            {
                // 287C
                if (intr->cb != NULL && intr->cb->cbSuspend != NULL && intr->cb->cbResume != NULL)
                {
                    int sp;
                    intr->cb->cbSuspend(intrNum, subIntrNum, &sp);
                    intr->cb->cbResume(intrNum, subIntrNum, sp);
                    *(short*)(out + 22) = (sp != 0);
                }
            }
            else
                *(short*)(out + 22) = 0;
        }
    }
    // 280C
    *(int*)(out + 32) = intr->u32;
    *(int*)(out + 36) = intr->u36;
    *(short*)(out + 18) = intr->v48 & 0xFF;
    *(int*)(out + 40) = intr->u16;
    *(int*)(out + 44) = intr->u20;
    *(int*)(out + 24) = intr->u12;
    *(int*)(out + 48) = intr->u24;
    *(int*)(out + 52) = intr->u28;
    sceKernelCpuResumeIntr(oldIntr);
    return 0;
}

int InterruptManagerForKernel_C8E4FF67(int intrNum, int subIntrNum, int setBit)
{
    if (sceKernelIsIntrContext() != 0)
        return 0x80020064;
    if (intrNum >= 68)
        return 0x80020065;
    int oldIntr = sceKernelCpuSuspendIntr();
    Interrupt *intr = &intInfo.intr[intrNum];
    if (intr->handler == 0 || (intr->handler & 3) == 1)
    {
        // 15F4
        sceKernelCpuResumeIntr(oldIntr);
        return 0x80020068;
    }
    if (subIntrNum < 0 || subIntrNum >= (intr->v48 & 0xFF))
    {
        // 1538
        sceKernelCpuResumeIntr(oldIntr);
        return 0x80020065;
    }
    // 156C
    SubInterrupt *subIntr = &intr->subIntrs[subIntrNum];
    subIntr->v48 = (subIntr->v48 & 0xFFFFFBFF) | (((setBit != 0) << 10) & 0x00000400);
    intr->v48 &= 0xFFFFF7FF;
    char num = intr->v48 & 0xFF;
    char v = (subIntr->v48 >> 10) & 1;
    // 15B4
    int i;
    for (i = 0; i < num; i++)
    {
        if (v != 0)
        {
            // 15E4
            intr->v48 |= 0x800;
            break;
        }
    }
    // 15C4
    if (((intr->v48 >> 11) & 1) != 0)
        sceKernelSetIntrLevel(intrNum, 2);
    sceKernelCpuResumeIntr(oldIntr);
    return 0;
}

// CA850281
void ReleaseContextHooks()
{
    InterruptManagerForKernel_0F9D0289(sub_091C, sub_091C);
}

// CBCBEF90
int sceKernelRegisterSystemCallTable(CbMap *newMap)
{
    if (newMap->next != NULL)
        return 0x80020036;
    int limit = newMap->unk2;
    if (limit < 0 || newMap->unk3 - 16 < limit) {
        // 2FF0
        return 0x80020037;
    }
    // 3008
    int oldIntr = sceKernelCpuSuspendIntr();
    CbMap *map = cbMap.next;
    CbMap *oldMap = &cbMap;
    // 302C
    while (map->unk1 != 0) {
        oldMap = map;
        map = map->next;
    }
    // 305C is useless?
    // 303C
    newMap->next = map;
    oldMap->next = newMap;
    COP0_CTRL_SET(COP0_CTRL_SC_TABLE, (int)cbMap.next);
    sceKernelCpuResumeIntr(oldIntr);
    return 0;
}

// D01EAA3F
int sceKernelSetIntrLogging(int intrNum, int arg1)
{
    if (intrNum >= 68)
        return 0x80020065;
    int oldIc = sceKernelCpuSuspendIntr();
    Interrupt *intr = &intInfo.intr[intrNum];
    if (arg1 == 0) {
        // 189C
        intr->v48 |= 0x00008000;
    }
    else
        intr->v48 &= 0xFFFF7FFF;
    // 1878
    sceKernelCpuResumeIntr(oldIc);
    return 0;
}

int InterruptManagerForKernel_D774BA45(int arg)
{
    return sceKernelSuspendIntr(arg, 0);
}

// E4B71544
int sceKernelClearIntrLogging(int intrNum)
{
    if (intrNum >= 68)
        return 0x80020065;
    int oldIntr = sceKernelCpuSuspendIntr();
    Interrupt *intr = &intInfo.intr[intrNum];
    intr->u16 = -1;
    intr->u20 = -1;
    intr->u24 = 0;
    intr->u28 = 0;
    intr->u32 = 0;
    intr->u36 = 0;
    intr->u12 = 0;
    sceKernelCpuResumeIntr(oldIntr);
    return 0;
}

int InterruptManagerForKernel_F987B1F0(int intrNum)
{
    if (sceKernelIsIntrContext() != 0)
        return 0x80020064;
    if (intrNum >= 68)
        return 0x80020065;
    int oldIntr = sceKernelCpuSuspendIntr();
    Interrupt *intr = &intInfo.intr[intrNum];
    if (intr->handler == 0) {
        sceKernelCpuResumeIntr(oldIntr);
        return 0x80020068;
    }
    // 1698
    if (((intr->v48 >> 8) & 3) != 0)
    {
        // 16CC
        intr->handler = 0;
        sceKernelSuspendIntr(intrNum, 0);
    }
    else
        intr->handler = 1;
    // 16B0
    intr->cb = NULL;
    if (intr->subIntrs != NULL)
        sceKernelFreeHeapMemory(intInfo.subIntrMemoryPoolId, intr->subIntrs);
    // 16C4
    intr->subIntrs = NULL;
    sceKernelCpuResumeIntr(oldIntr);
    return 0;
}

// FBCB2E5E
int SupportIntr(int intrNum)
{
    if (intrNum >= 64)
        return 0x80020065;
    int oldIntr = sceKernelCpuSuspendIntr();
    sceKernelSuspendIntr(intrNum, 0);
    Interrupt *intr = &intInfo.intr[intrNum];
    intr->handler = 0;
    intr->v48 |= 0x300;
    sceKernelCpuResumeIntr(oldIntr);
    return 0;
}

int InterruptManagerForKernel_FD524B3E(int set)
{
    intInfo.opt = (set == 0);
    return 0;
}

void InterruptManagerForKernel_FD66372C(int arg)
{
    if (arg == 0)
        *(int*)(mod_0E48) = 0x42000018; // eret
    // 30F0
    if (arg == 1)
    {
        // 3104
        *(int*)(mod_0E50) = 0x00005821; // t3 = 0
    }
}

int module_bootstart()
{
    int oldIc = sceKernelCpuSuspendIntr();
    COP0_CTRL_SET(COP0_CTRL_IS_INTERRUPT, 0);
    COP0_CTRL_SET(COP0_CTRL_SP_KERNEL, (int)globUnk);
    COP0_CTRL_SET(COP0_CTRL_SP_USER, 0);
    my_memset(&intInfo, 0, sizeof(InterruptInfo));
    intInfo.opt = 1;
    sub_1030();
    ReleaseContextHooks();
    // 1150
    int i;
    for (i = 67; i >= 0; i--) {
        intInfo.intr[i].u16 = -1;
        intInfo.intr[i].u20 = -1;
    }
    sceKernelRegisterExceptionHandler(0, (void*)sub_0038);
    sceKernelRegisterPriorityExceptionHandler(0, 3, (void*)sub_0924);
    sceKernelRegisterExceptionHandler(8, (void*)sub_0CF8);
    sceKernelRegisterIntrHandler(67, 0, sub_0000, 0, 0);
    sceKernelRegisterSuspendHandler(29, sub_3110, 0);
    sceKernelRegisterResumeHandler(29, sub_3160, 0);
    sceKernelSetIntrLogging(67, 0);
    int st;
    COP0_STATE_GET(st, COP0_STATE_STATUS);
    COP0_STATE_SET(COP0_STATE_STATUS, (st & 0xFFFF00FF) | 0x0400);
    sceKernelEnableIntr(67);
    COP0_STATE_SET(COP0_STATE_COUNT, 0);
    COP0_STATE_SET(COP0_STATE_COMPARE, 0x80000000);
    sceKernelCpuResumeIntr(oldIc);
    int ret = sceKernelRegisterLibrary(intrMgrStrPtr); // This address contains a pointer to "InterruptManager"
    if (ret < 0)
        return ret;
    return 0;
}

int module_reboot_before()
{
    int st;
    COP0_STATE_GET(st, COP0_STATE_STATUS);
    COP0_STATE_SET(COP0_STATE_STATUS, st & 0xFFFF7BFF);
    InterruptManagerForKernel_F987B1F0(67);
    sceKernelReleaseExceptionHandler(0, (void*)sub_0038);
    sceKernelReleaseExceptionHandler(0, (void*)sub_0924);
    sceKernelReleaseExceptionHandler(8, (void*)sub_0CF8);
    return 0;
}

int QueryIntrHandlerInfoForUser()
{
    return 0x80020001;
}

void InterruptManagerForKernel_02314986()
{
    asm("mtic %0, $0" : : "r" (1));
}

void sceKernelCpuResumeIntr(int arg)
{
    asm("mtic %0, $0" : : "r" (arg));
    asm("nop");
    asm("nop");
    asm("nop");
    asm("nop");
    asm("nop");
    asm("nop");
    asm("nop");
}

void sceKernelCpuResumeIntrWithSync(int arg)
{
    asm("sync");
    asm("nop");
    asm("sync");
    asm("nop");
    asm("mtic %0, $0" : : "r" (arg));
    asm("nop");
    asm("nop");
    asm("nop");
    asm("nop");
    asm("nop");
    asm("nop");
    asm("nop");
}

// Exported as 092968F4
int sceKernelCpuSuspendIntr()
{
    int ret;
    asm("mfic %0, $0" : "=r" (ret));
    asm("mtic $0, $0");
    asm("nop");
    asm("nop");
    asm("nop");
    asm("nop");
    asm("nop");
    asm("nop");
    asm("nop");
    return ret;    
}

int sceKernelDisableSubIntr(int intrNum, int subIntrNum)
{
    if (intrNum >= 68)
        return 0x80020065;
    K1_BACKUP();
    int oldIntr = sceKernelCpuSuspendIntr();
    Interrupt *intr = &intInfo.intr[intrNum];
    if (subIntrNum < 0 || subIntrNum >= (intr->v48 & 0xFF)
     || (K1_USER_MODE() && (((intr->v48 >> 11) & 1) == 0
                              || ((intr->subIntrs[subIntrNum].v48 >> 10) & 1) == 0)))
    {
        // 231C
        sceKernelCpuResumeIntr(oldIntr);
        K1_RESET();
        return 0x80020065;
    }
    // 2354
    if (intr->handler == 0 || intr->cb == NULL || intr->cb->cbDisable == NULL)
    {
        // 2378
        sceKernelCpuResumeIntr(oldIntr);
        K1_RESET();
        return 0x80020068;
    }
    int ret = intr->cb->cbDisable(intrNum, subIntrNum);
    sceKernelCpuResumeIntr(oldIntr);
    K1_RESET();
    return ret;
}

int sceKernelEnableIntr(int intNum)
{
    int v;
    if (intNum >= 68)
        return 0x80020065;
    int oldIc = sceKernelCpuSuspendIntr();
    if (intInfo.intr[intNum].handler == 0) {
        sceKernelCpuResumeIntr(oldIc);
        return 0x80020068;
    }
    // 1928
    if (intNum < 64)
    {
        // 1978
        sub_29B0(intNum);
        sceKernelCpuResumeIntr(oldIc);
        return 0;
    }
    if (intNum == 64)
        v = 0x100;
    else if (intNum == 65)
        v = 0x200;
    else if (intNum == 66)
        v = 0x1000;
    else if ((intNum & 0x43) == 0)
        v = 0x8000;
    else
        v = 0;
    // 1960
    int st;
    COP0_STATE_GET(st, COP0_STATE_STATUS);
    COP0_STATE_SET(COP0_STATE_STATUS, st | (v & 0xFF00));
    sceKernelCpuResumeIntr(oldIc);
    return 0;
}

int sceKernelEnableSubIntr(int intrNum, int subIntrNum)
{
    if (intrNum >= 68)
        return 0x80020065;
    K1_BACKUP();
    int oldIntr = sceKernelCpuSuspendIntr();
    Interrupt *intr = &intInfo.intr[intrNum];
    if (subIntrNum < 0 || subIntrNum >= (intr->v48 & 0xFF)
     || (K1_USER_MODE() && (((intr->v48 >> 11) & 1) == 0
                              || ((intr->subIntrs[subIntrNum].v48 >> 10) & 1) == 0)))
    {
        // 2210
        sceKernelCpuResumeIntr(oldIntr);
        K1_RESET();
        return 0x80020065;
    }
    // 2248
    if (intr->handler == 0 || intr->cb == NULL || intr->cb->cbEnable == NULL)
    {
        // 226C
        sceKernelCpuResumeIntr(oldIntr);
        K1_RESET();
        return 0x80020068;
    }
    // 2274
    int ret = intr->cb->cbEnable(intrNum, subIntrNum);
    sceKernelCpuResumeIntr(oldIntr);
    K1_RESET();
    return ret;
}

int sceKernelIsIntrContext()
{
    int isIntr;
    COP0_CTRL_GET(isIntr, COP0_CTRL_IS_INTERRUPT);
    return isIntr;
}

int sceKernelIsSubInterruptOccured(int intrNum, int subIntrNum)
{
    if (intrNum >= 68)
        return 0x80020065;
    K1_BACKUP();
    int oldIntr = sceKernelCpuSuspendIntr();
    Interrupt *intr = &intInfo.intr[intrNum];
    if (subIntrNum < 0 || subIntrNum >= (intr->v48 & 0xFF)
     || (K1_USER_MODE() && (((intr->v48 >> 11) & 1) == 0
                              || ((intr->subIntrs[subIntrNum].v48 >> 10) & 1) == 0)))
    {
        // 2688
        sceKernelCpuResumeIntr(oldIntr);
        K1_RESET();
        return 0x80020065;
    }
    // 26C0
    if (intr->handler == 0 || intr->cb == NULL || intr->cb->cbIsOccured == NULL)
    {
        // 26E4
        sceKernelCpuResumeIntr(oldIntr);
        K1_RESET();
        return 0x80020068;
    }
    // 26EC
    int ret = intr->cb->cbIsOccured(intrNum, subIntrNum);
    sceKernelCpuResumeIntr(oldIntr);
    K1_RESET();
    return ret;
}

// 58DD8978
int sceKernelRegisterIntrHandler(int intrNum, int arg1, void *func, int arg3, SceIntrHandler *handler)
{
    if (sceKernelIsIntrContext() != 0)
        return 0x80020064;
    if (intrNum >= 68)
        return 0x80020065;
    if (handler != NULL && handler->size != 12)
        return 0x8002006B;
    // 12B8
    int oldIc = sceKernelCpuSuspendIntr();
    if (handler != NULL && handler->size > 0 && intInfo.subIntrMemoryPoolId == 0)
    {
        // 1468
        int ret = sceKernelCreateHeap(1, 1, 0x800, heapName); // "SceInterruptManager"
        if (ret > 0)
            intInfo.subIntrMemoryPoolId = ret;
        if (ret < 0) {
            sceKernelCpuResumeIntr(oldIc);
            return ret;
        }
    }
    // 12E4
    Interrupt *intr = &intInfo.intr[intrNum];
    if (intr->handler != 0 && (intr->handler & 3) != 1) {
        sceKernelCpuResumeIntr(oldIc);
        return 0x80020067;
    }
    // 1354
    if ((int)func >= 0) {
        sceKernelCpuResumeIntr(oldIc);
        return 0x80020065;
    }
    intr->subIntrs = NULL;
    // 1368
    intr->cb = NULL;
    int size;
    if (handler == NULL || (size = handler->attr) <= 0)
    {
        // 144C
        if (intr->subIntrs != NULL) {
            sceKernelFreeHeapMemory(intInfo.subIntrMemoryPoolId, intr->subIntrs); // Free memory space
            intr->subIntrs = NULL;
        }
    }
    else
    {
        SubInterrupt *subIntrs = sceKernelAllocHeapMemory(intInfo.subIntrMemoryPoolId, size); // Allocate space to store the subIntrs
        if (subIntrs != NULL)
            my_memset(subIntrs, 0, sizeof(*subIntrs));
        // 13A4
        intr->subIntrs = subIntrs;
        if (subIntrs == NULL)
        {
            // 1440
            sceKernelCpuResumeIntr(oldIc);
            return 0x80020190;
        }
        intr->cb = handler->cb;
    }
    // 13B4
    sceKernelSuspendIntr(intrNum, 0);
    int unk2;
    if (intr->handler != 1 && intrNum < 64)
        unk2 = 3;
    else
        unk2 = 0;
    // 13E0
    if (arg1 != 0)
        arg1 = 2;
    intr->v48 = (intr->v48 & 0xFFFFFCFF) | ((unk2 << 8) & 0x300);
    intr->handler = arg1 | (int)func;
    intr->loadCoreRes = sceKernelGetModuleGPByAddressForKernel(func);
    intr->arg = arg3;
    *(char*)&intr->v48 = 0;
    if (handler != NULL)
        *(char*)&intr->v48 = *(char*)&handler->attr;
    // 1420
    intr->v48 = ((intr->v48 & 0x7FFF7FFF) | ((intInfo.opt << 15) & 0x8000)) & 0x7FFFFFFF;
    *(char*)(&intr->v48 + 2) = intrNum;
    sceKernelCpuResumeIntr(oldIc);
    return 0;
}

int sceKernelRegisterSubIntrHandler(int intrNum, int subIntrNum, void *handler, void *arg)
{
    if (intrNum >= 68)
        return 0x80020065;
    K1_BACKUP();
    int oldIntr = sceKernelCpuSuspendIntr();
    Interrupt *intr = &intInfo.intr[intrNum];
    if (intr->handler == 0) {
        sceKernelCpuResumeIntr(oldIntr);
        K1_RESET();
        return 0x80020068;
    }
    // 1ED4
    if (((int)handler >> 31) == 0 && K1_USER_MODE()) // 200C
    {
        sceKernelCpuResumeIntr(oldIntr);
        K1_RESET();
        return 0x80020065;
    }
    else
    {
        if (intInfo.intrStack == 0)
            return 0x80020065;
        if (((intr->v48 >> 11) & 1) == 0)
        {
            sceKernelCpuResumeIntr(oldIntr);
            K1_RESET();
            return 0x80020065;
        }
    }
    // 1F00
    if (subIntrNum < 0 || subIntrNum >= (intr->v48 & 0xFF)) {
        sceKernelCpuResumeIntr(oldIntr);
        K1_RESET();
        return 0x80020065;
    }
    SubInterrupt *subIntr = &intr->subIntrs[subIntrNum];
    if (subIntr->handler != 0)
    {
        sceKernelCpuResumeIntr(oldIntr);
        K1_RESET();
        return 0x80020067;
    }
    // 1F3C
    if (((int)handler >> 31) == 0 && (((subIntr->v48 & 0xFF) >> 10) & 1) == 0)
    {
        sceKernelCpuResumeIntr(oldIntr);
        K1_RESET();
        return 0x80020065;
    }
    // 1F60
    if (intr->cb != NULL && intr->cb->cbRegBefore != NULL)
    {
        // 1FEC
        int ret = intr->cb->cbRegBefore(intrNum, subIntrNum, handler, arg);
        if (ret != 0)
        {
            sceKernelCpuResumeIntr(oldIntr);
            K1_RESET();
            return ret;
        }
    }
    subIntr->handler = (int)handler;
    // 1F78
    subIntr->loadCoreRes = sceKernelGetModuleGPByAddressForKernel(handler);
    *(char*)&subIntr->v48 = 0;
    subIntr->arg = (int)arg;
    subIntr->v48 = (subIntr->v48 & 0xFFFF7FFF) | (intr->v48 & 0x00008000) | 0x80000300;
    if (intr->cb != NULL && intr->cb->cbRegAfter != NULL)
    {
        // 1FD4
        int ret = intr->cb->cbRegAfter(intrNum, subIntrNum, handler, arg);
        if (ret != 0)
        {
            // (moved 1FC4)
            subIntr->handler = 0;
            sceKernelCpuResumeIntr(oldIntr);
            K1_RESET();
            return ret;
        }
    }
    sceKernelCpuResumeIntr(oldIntr);
    K1_RESET();
    return 0;
}

int sceKernelRegisterUserSpaceIntrStack(int addr, int size, int arg2)
{
    if (size != 0x2000)
        return 0x80000104;
    int oldIntr = sceKernelCpuSuspendIntr();
    if (intInfo.intrStack != 0) {
        sceKernelCpuResumeIntr(oldIntr);
        return 0x8002006D;
    }
    if (addr < 0 || arg2 < 0) {
        sceKernelCpuResumeIntr(oldIntr);
        return 0x8002006C;
    }
    intInfo.intrStackArg = arg2;
    intInfo.intrStack = addr + 0x2000;
    // 2EF8
    sceKernelCpuResumeIntr(oldIntr);
    return 0;
}

int sceKernelReleaseSubIntrHandler(int intrNum, int subIntrNum)
{
    if (intrNum >= 68)
        return 0x80020065;
    K1_BACKUP();
    int oldIntr = sceKernelCpuSuspendIntr();
    Interrupt *intr = &intInfo.intr[intrNum];
    if (intr->handler == 0)
    {
        // 2108
        sceKernelCpuResumeIntr(oldIntr);
        K1_RESET();
        return 0x80020068;
    }
    if (subIntrNum < 0 || subIntrNum >= (intr->v48 & 0xFF))
    {
        // 20A0
        sceKernelCpuResumeIntr(oldIntr);
        K1_RESET();
        return 0x80020065;
    }
    // 20E0
    SubInterrupt *subIntr = &intr->subIntrs[subIntrNum];
    if (subIntr->handler == 0 || (subIntr->handler < 0 && K1_USER_MODE()))
    {
        // 2108
        sceKernelCpuResumeIntr(oldIntr);
        K1_RESET();
        return 0x80020068;
    }
    // 2110
    if (intr->cb != NULL && intr->cb->cbRelBefore != NULL)
    {
        // 2160
        int ret = intr->cb->cbRelBefore(intrNum, subIntrNum);
        if (ret != 0)
        {
            sceKernelCpuResumeIntr(oldIntr);
            K1_RESET();
            return ret;
        }
    }
    // 2128
    int oldHandler = subIntr->handler;
    subIntr->handler = 0;
    if (intr->cb != NULL && intr->cb->cbRelAfter != NULL)
    {
        // 2150
        int ret = intr->cb->cbRelAfter(intrNum, subIntrNum);
        // (moved 2140)
        if (ret != 0)
        {
            subIntr->handler = oldHandler;
            sceKernelCpuResumeIntr(oldIntr);
            K1_RESET();
            return ret;
        }
    }
    sceKernelCpuResumeIntr(oldIntr);
    K1_RESET();
    return 0;
}

int sceKernelResumeSubIntr(int intrNum, int subIntrNum, int arg2)
{
    if (intrNum >= 0)
        return 0x80020065;
    K1_BACKUP();
    int oldIntr = sceKernelCpuSuspendIntr();
    Interrupt *intr = &intInfo.intr[intrNum];
    if (subIntrNum < 0 || subIntrNum >= (intr->v48 & 0xFF)
     || (K1_USER_MODE() && (((intr->v48 >> 11) & 1) == 0
                              || ((intr->subIntrs[subIntrNum].v48 >> 10) & 1) == 0)))
    {
        // 2574
        sceKernelCpuResumeIntr(oldIntr);
        K1_RESET();
        return 0x80020065;
    }
    // 25B0
    if (intr->handler == 0 || intr->cb == NULL || intr->cb->cbResume == NULL)
    {
        // 25D4
        sceKernelCpuResumeIntr(oldIntr);
        K1_RESET();
        return 0x80020068;
    }
    int ret = intr->cb->cbResume(intrNum, subIntrNum, arg2);
    sceKernelCpuResumeIntr(oldIntr);
    K1_RESET();
    return ret;
}

// 0x12B95762
int sceKernelSuspendSubIntr(int intrNum, int subIntrNum, int *arg2)
{
    if (intrNum >= 68)
        return 0x80020065;
    K1_BACKUP();
    if (!K1_USER_PTR(arg2))
    {
        // 24C4
        K1_RESET();
        return 0x800200D3;
    }
    int oldIntr = sceKernelCpuSuspendIntr();
    Interrupt *intr = &intInfo.intr[intrNum];
    if (subIntrNum < 0 || subIntrNum >= (intr->v48 & 0xFF)
     || (K1_USER_MODE() && (((intr->v48 >> 11) & 1) == 0
                              || ((intr->subIntrs[subIntrNum].v48 >> 10) & 1) == 0)))
    {
        // 2440
        sceKernelCpuResumeIntr(oldIntr);
        K1_RESET();
        return 0x80020065;
    }
    // 247C
    if (intr->handler == 0 || intr->cb == NULL || intr->cb->cbSuspend == NULL)
    {
        // 24A0
        if (arg2 != NULL)
            *arg2 = 0;
        sceKernelCpuResumeIntr(oldIntr);
        K1_RESET();
        return 0x80020068;
    }
    
    int ret = intr->cb->cbSuspend(intrNum, subIntrNum, arg2);
    sceKernelCpuResumeIntr(oldIntr);
    K1_RESET();
    return ret;
}

int sub_091C()
{
    return 0;
}

void sub_1030()
{
    *(int*)(0xBC300008) = 0x2000000F;
    *(int*)(0xBC300018) = 0;
    *(int*)(0xBC300028) = 0;
    SYNC();
}

void sub_1050(int out[2])
{
    int v1 = *(int*)(0xBC300014);
    int v2 = *(int*)(0xBC300024);
    out[0] = *(int*)(0xBC300004) & 0xFDFFFFF0;
    out[1] = (v1         & 0xFFFF3F3F)
           | ((v2 <<  6) & 0x000000C0)
           | ((v2 << 12) & 0x0000C000);
}

void sub_1080(int *arg)
{
    int mask = *(int*)(0xBC300018);
    int unk = *(int*)(0xBC300028);
    arg[0] = *(int*)(0xBC300008);
    arg[1] = ( mask       & 0xFFFF3F3F) 
           | ((unk <<  6) & 0x000000C0) 
           | ((unk << 12) & 0x0000C000);
}

void sub_10A8(int *arg)
{
    *(int*)(0xBC300008) = arg[0] | 0x2000000F;
    *(int*)(0xBC300018) = arg[1] & 0xFFFF3F3F;
    *(int*)(0xBC300028) = ((arg[1] >> 6) & 0x3) | ((arg[1] >> 12) & 0xC);
    SYNC();
}

void *my_memset(void *dstVoid, char c, int n)
{
    char *dst = (char*)dstVoid;
    if (dst != NULL)
    {
        // 2950
        char *actDst = dst;
        while ((n--) > 0)
            *(actDst++) = c;
    }
    return dst;
}

void sub_2968(int intrNum)
{
    int mask = ~(1 << (intrNum & 0x1F));
    int *ptr = &intInfo.grpsOpt[intrNum >> 5];
    int i = 4;
    // 2990
    while ((i--) >= 0) {
        *ptr &= mask;
        ptr += 2;
    }
}

void sub_29B0(int intrNum)
{
    int val = (intInfo.intr[intrNum].v48 >> 8) & 3;
    int mask = 1 << (intrNum & 0x1F);
    int *ptr = &intInfo.grpsOpt[(intrNum >> 5) + (val << 1)];
    // 29F8
    while ((val++) < 4) {
        *ptr |= mask;
        ptr += 2;
    }
    // 2A14
    if (sceKernelIsIntrContext() == 0) {
        // 2A30
        sub_10A8(&intInfo.grpsOpt[6]);
    }
}

void sub_2A40(int intrNum)
{
    int *ptr = &intInfo.grpsOpt[intrNum >> 5];
    int mask = ~(1 << (intrNum & 0x1F));
    int i = 11;
    // 2A78
    while ((--i) >= 0) {
        *ptr &= mask;
        ptr += 2;
    }
    int sp[2];
    sub_1080(sp);
    sp[intrNum >> 5] &= mask;
    sub_10A8(sp);
}

int sub_3110()
{
    int sp[2];
    COP0_STATE_GET(intInfo.compare, COP0_STATE_COMPARE);
    COP0_STATE_GET(intInfo.count, COP0_STATE_COUNT);
    sub_1080(intInfo.unk);
    my_memset(sp, 0, 8);
    sub_10A8(sp);
    return 0;
}

int sub_3160()
{
    sub_10A8(intInfo.unk);
    COP0_STATE_SET(COP0_STATE_COMPARE, intInfo.compare);
    COP0_STATE_SET(COP0_STATE_COUNT, intInfo.count);
    return 0;
}

