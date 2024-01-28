/*
 * MemLmd Module PSP 02g fw 6.60 clone
 * Copyright (c) 2012 by Draan <draanPSP@gmail.com>
 * Copyright (C) 2012 The uOFW team
 *
 * Based on a work done by adrahil, FreePlay and jas0nuk
 * See the file COPYING for copying permission.
 */
#include <pspsdk.h>
#include <pspkernel.h>
#include <pspintrman.h>
#include <pspsysevent.h>

PSP_MODULE_INFO("sceMemlmd", 0x7007, 1, 28);

#define KIRK_SET_INCOMPLETE_PROCESSING_ERROR(p) _sw(p, 0xBDE00008)
#define KIRK_GET_INCOMPLETE_PROCESSING_ERROR() _lw(0xBDE00008)
#define KIRK_SET_PROCESSING_PHASE(p) _sw(p, 0xBDE0000C)
#define KIRK_SET_COMMAND(m) _sw(m, 0xBDE00010)
#define KIRK_GET_PROCESSING_RESULT() _lw(0xBDE00014)
#define KIRK_GET_PROCESSING_STATUS_PATTERN() _lw(0xBDE0001C)
#define KIRK_SET_INIT_ASYNC_PROCESSING_PATTERN(p) _sw(p, 0xBDE00020)
#define KIRK_SET_END_ASYNC_PROCESSING_PATTERN(p) _sw(p, 0xBDE00024)
#define KIRK_SET_END_PROCESSING_PATTERN(p) _sw(p, 0xBDE00028)
#define KIRK_SET_SOURCE_ADDR(a) _sw(a, 0xBDE0002C)
#define KIRK_SET_DEST_ADDR(a) _sw(a, 0xBDE00030)
#define ASM_SYNC() asm("sync")

#define SYSCONF_GET_BUS_CLOCK_MASK() _lw(0xBC100050)
#define SYSCONF_CHECK_BUS_CLOCK_ENABLED(bus) SYSCONF_GET_BUS_CLOCK_MASK() & bus
#define SYSCONF_SET_BUS_CLOCK_MASK(mask) _sw(mask, 0xBC100050)

#define KIRK_BUS_CLOCK (1 << 7)
#define KIRK_INTERRUPT 0x18

#define BUS_ENABLE 1
#define BUS_DISABLE 0

#define OP_IDLE 0
#define OP_ASYNC_SEMA 1
#define OP_SYNC_SEMA 2

#define SYSEVENT_STATE_RDY 0
#define SYSEVENT_STATE_BUSY 1

int InitSys();
int StopSys();
int Semaphore(u8* out_buf, int out_len, u8* in_buf, int in_len, int mode, int skip_size_check);
int SemaphoreAsync(u8* out_buf, int out_len, u8* in_buf, int in_len, int mode, int skip_size_check);
int sceUtilsBufferCopyByPolling(u8* outbuff, u8* inbuff, int cmd);
int sceUtilsBufferCopyWithRange(u8* outbuff, int outsize, u8* inbuff, int insize, int cmd);
int sceUtilsBufferCopyByPollingWithRange(u8* outbuff, int outsize, u8* inbuff, int insize, int cmd);
int CallSysEventHandler(int ev_id, char *ev_name, void *param, int *result);
int BusClock(int bus, int mode);

int sceKernelPowerLock(int val);
int sceKernelPowerUnlock(int val);

typedef struct {
    int operation; //0
    int utils_sema;	//4
    int utils_flag;	//8
    int result;	//C
    int status_pattern; //10
    int kirk_enabled; //14
} GlobalSysParam;

u32 tags[6] =
{
    0x00000000, //0x000022EC
    0x01000000, //0x000022F0
    0x4C9490F0, //0x000022F4
    0x4C9494F0, //0x000022F8
    0x4C9491F0, //0x000022FC
    0x4C9495F0 //0x00002300
}

//SIGNCHECK KEYS START
//0x00002304
u8 key_2304[16] =
{
    0x71, 0xF6, 0xA8, 0x31, 0x1E, 0xE0, 0xFF, 0x1E,
    0x50, 0xBA, 0x6C, 0xD2, 0x98, 0x2D, 0xD6, 0x2D
};

//0x00002314
u8 key_2314[16] =
{
    0xAA, 0x85, 0x4D, 0xB0, 0xFF, 0xCA, 0x47, 0xEB,
    0x38, 0x7F, 0xD7, 0xE4, 0x3D, 0x62, 0xB0, 0x10
};
//SIGNCHECK KEYS END

//0x00002324
u8 key_2324[16] =
{
    0x6A, 0x19, 0x71, 0xF3, 0x18, 0xDE, 0xD3, 0xA2,
    0x6D, 0x3B, 0xDE, 0xC7, 0xBE, 0x98, 0xE2, 0x4C
};

//0x000023B4
u8 key_23B4[16] =
{
    0x50, 0xCC, 0x03, 0xAC, 0x3F, 0x53, 0x1A, 0xFA,
    0x0A, 0xA4, 0x34, 0x23, 0x86, 0x61, 0x7F, 0x97
};

//0x00002444
u8 fat_seed[16] =
{
    0x37, 0xBC, 0x9C, 0x53, 0x1C, 0xCD, 0xC9, 0x7C,
    0x56, 0x80, 0x12, 0x66, 0x71, 0x49, 0x1C, 0x53
};

//0x00002454
u8 slim_seed[16] =
{
    0xFF, 0x61, 0x61, 0x89, 0xBB, 0x45, 0xB8, 0xBC,
    0x69, 0x7B, 0xD4, 0x6D, 0xF0, 0x6A, 0xC7, 0x43
};

//0x00002464
u8 sha1_whitelist[0x104] = //13 entries
{
    0x7B, 0xBC, 0x67, 0x1A, 0x0B, 0x98, 0xA8, 0xDC, 0x00, 0x72, 0x74, 0xB0, 0x32, 0x0D, 0x60, 0xF1, 0x09, 0x2E, 0x34, 0x05,
    0x93, 0x56, 0x47, 0x21, 0xB8, 0xE3, 0x89, 0x11, 0x04, 0x11, 0xC5, 0x1A, 0x94, 0xF5, 0x0A, 0x40, 0xF2, 0xD5, 0x85, 0xBE,
    0x95, 0xC1, 0x44, 0x0B, 0xAC, 0x02, 0xAE, 0x8D, 0x87, 0x6F, 0x18, 0x40, 0x92, 0xC4, 0xDA, 0xE7, 0xD7, 0xF1, 0x91, 0xF1,
    0x5B, 0x71, 0x04, 0xD4, 0xC1, 0xCD, 0x64, 0x33, 0xB4, 0x08, 0x78, 0x86, 0x5B, 0x11, 0x33, 0x63, 0x8F, 0x4F, 0xA8, 0x3E,
    0x83, 0xA1, 0x86, 0x2F, 0x24, 0x78, 0x28, 0x65, 0x95, 0xC1, 0xF5, 0xF8, 0x99, 0x9A, 0x4C, 0x64, 0xA1, 0x7A, 0x66, 0xC2,
    0x62, 0x61, 0xFB, 0x3B, 0xE3, 0x76, 0x60, 0xF1, 0xA7, 0xC7, 0x22, 0x27, 0x7A, 0x9A, 0x1D, 0x70, 0xE8, 0xA1, 0xA5, 0x9F,
    0x8E, 0x5D, 0x4D, 0x03, 0x06, 0xEF, 0x3B, 0xA7, 0xBB, 0x7A, 0x20, 0x6A, 0x26, 0xD6, 0x31, 0x76, 0xC8, 0xB5, 0xAA, 0x87,
    0x15, 0x72, 0xBE, 0x6E, 0xC3, 0x5C, 0x1C, 0xB1, 0x28, 0x31, 0x40, 0x1D, 0xF8, 0xF8, 0x18, 0xBB, 0x92, 0xCF, 0x26, 0x7D,
    0xCE, 0x9D, 0x02, 0xE8, 0xD0, 0x26, 0x76, 0x80, 0x4D, 0x6E, 0x69, 0xC0, 0xA1, 0x75, 0x84, 0x01, 0x21, 0x25, 0x70, 0x46,
    0x81, 0xDF, 0x67, 0x18, 0xAC, 0xF1, 0xF7, 0x62, 0xCC, 0x70, 0x90, 0xE4, 0xC6, 0x01, 0xD3, 0x5A, 0xB4, 0x98, 0xB5, 0xE2,
    0x66, 0xEE, 0x26, 0x79, 0xCB, 0x51, 0x51, 0xDA, 0x48, 0x06, 0x35, 0x8C, 0x66, 0x61, 0x54, 0xAC, 0x0E, 0xAF, 0x56, 0x86,
    0x5F, 0x6F, 0xF4, 0x01, 0xD9, 0x53, 0x0A, 0x53, 0xFB, 0xD9, 0xB5, 0xF1, 0x6F, 0x84, 0x5F, 0x71, 0x91, 0xF8, 0x6A, 0xA6,
    0xFE, 0x55, 0x0A, 0x9F, 0x24, 0xA8, 0x14, 0x13, 0x38, 0xA2, 0xE2, 0x96, 0xB6, 0xCD, 0xAC, 0x2B, 0x49, 0x2D, 0x6B, 0x2B
};

//0x00002580
volatile u8 *key_slim_destination = (volatile u8*)0xBFC00240;

//0x00002584
volatile u8 *key_fat_destination = (volatile u8*)0xBFC00220;

//0x00002588
u8* key_2588[] =
{
    0x85, 0x93, 0x1F, 0xED, 0x2C, 0x4D, 0xA4, 0x53,
    0x59, 0x9C, 0x3F, 0x16, 0xF3, 0x50, 0xDE, 0x46
};

//0x00002598
u8* key_2598[] =
{
    0xFA, 0x79, 0x09, 0x36, 0xE6, 0x19, 0xE8, 0xA4,
    0xA9, 0x41, 0x37, 0x18, 0x81, 0x02, 0xE9, 0xB3
};

//0x000025A8
volatile u8 *hwbuff_280 = (volatile u8*)0xBFC00280;

//0x000025AC
volatile u8 *hwbuff_A00 = (volatile u8*)0xBFC00A00;

//0x000025B0
volatile u8 *hwbuff_340 = (volatile u8*)0xBFC00340;

//0x000025B4
PspSysEventHandler sysevent_handler =
{
    sizeof(PspSysEventHandler),
    "SceUtils1024",
    0x00FFFF00,
    CallSysEventHandler,
    0,
    0,
    NULL,
    {0,0,0,0,0,0,0,0,0}
};

//0x22C0
const u32 module_sdk_version = 0x06060010;

//0x2568 "SceUtils1024"

//0x000025D0
GlobalSysParam system;

//0x00002600
u8 *prx_buff;
//0x00002604
u8 swbuff_2604[0x150];
//0x000027C0
u8 selftest_buffer[0x1c];

//0x00002780
u8 swbuff_2780[0x28];

//0x00000000
int kirk7(u8 *prx, u32 size, u32 scramble_code, u32 sync)
{
    prx_buff = prx;
    ((u32 *) prx)[0] = 5;
    ((u32 *) prx)[3] = scramble_code;
    ((u32 *) prx)[1] = 0;
    ((u32 *) prx)[2] = 0;
    ((u32 *) prx)[4] = size;
    if(sync != 0)
    {
        return sceUtilsBufferCopyByPollingWithRange(prx, size+20, prx, size+20, 7);
    }
    return sceUtilsBufferCopyWithRange(prx, size+20, prx, size+20, 7);
}
//0x0000006C
//two versions: inline is used in check_whitelist, normal one is used in decrypter
static __inline__ int memcmp_inline(u8 *first, u8 *second, int size)
{
    int i;
    u8 *m1 = (u8*)first;
    u8 *m2 = (u8*)second;

    for (i = 0; i < size; i++)
    {
        if (m1[i] != m2[i])
            return m2[i] - m1[i];
    }
    return 0;
}

int memcmp(u8 *first, u8 *second, int size)
{
    int i;
    u8 *m1 = (u8*)first;
    u8 *m2 = (u8*)second;

    for (i = 0; i < size; i++)
    {
        if (m1[i] != m2[i])
            return m2[i] - m1[i];
    }
    return 0;
}

//0x000000A8
int check_whitelist(u8 *prx, u32 prx_size, u8 *blacklist_buff, u32 blacklist_size, u32 sync)
{
    //copy first 4 bytes of inbuff into hw register
    int i;
    for(i = 0; i < 4; i++)
    {
        hwbuff_340[i] = prx[i];
    }
    //null them
    _sw(0, (u32)prx);

    //generate SHA1 hash (into hw register)
    int ret;
    if(sync != 0)
    {
        ret = sceUtilsBufferCopyByPollingWithRange((u8*)hwbuff_A00, 0x14, prx, prx_size, 0xB);	
    } else {
        ret = sceUtilsBufferCopyWithRange((u8*)hwbuff_A00, 0x14, prx, prx_size, 0xB);
    }

    if(ret == 0)
    {
        //restore first 4 bytes from hw register (for futher processing probably)
        for(i = 0; i < 4; i++)
        {
            prx[i] = hwbuff_340[i];
        }
        int i;
        int num_hashes = blacklist_size/0x14;
        if(num_hashes == 0) return 0;
        for(i = 0; i < num_hashes; i++)
        {
            if(memcmp_inline((u8*)hwbuff_A00, blacklist_buff+i*0x14, 0x14) == 0) return 1;
        }
    }
    return 0;
}

//someone can help me on this one?

//0x0000020C
int sub_0000020C(u8 *prx, u32 size, u32 *newsize, u32 sync) //decrypter
{
    int ret;
    if(prx == NULL || newsize == NULL){
        ret = -201;
        goto exit4;
    }
    int size_check = (size < 0x160);
    if(size_check){
        ret = -201;
        goto exitC;
    }
    if(((u32)prx & 0x3F) != 0)
    {
        ret = -203;
        goto exit4;
    }
    if (((0x00220202 >> (((u32)prx >> 27) & 0x001F)) & 0x1) == 0)
    {
        ret = -204;
        goto exit4;
    }
    int a1;
    for(a1 = 0; a1 < 0x150; a1++) swbuff_2604[a1] = prx[a1];

    //TAG 0x4C9495F0
    if(memcmp(&tags[5], swbuff_2604+0xD0, 4) == 0)
    {
        s1 = key_slim_destination;
        s6 = 0x43;
        s5 = 4;
        fp = 0;
    } else
        //TAG 0x4C9491F0
        if(memcmp(&tags[4], swbuff_2604+0xD0, 4) == 0)
        {
            s1 = key_2588;
            s6 = 0x43;
            s5 = 4;
            fp = 0;
        } else
            //TAG 0x4C9494F0
            if(memcmp(&tags[3], swbuff_2604+0xD0, 4) == 0)
            {
                s1 = key_fat_destination;
                s6 = 0x43;
                s5 = 4;
                fp = 0;
            } else
                //TAG 0x4C9490F0
                if(memcmp(&tags[2], swbuff_2604+0xD0, 4) == 0)
                {
                    s1 = key_2598;
                    s6 = 0x43;
                    s5 = 4;
                    fp = 0;
                } else
                    //TAG 0x00000000
                    if(memcmp(&tags[0], swbuff_2604+0xD0, 4) == 0)
                    {
                        s1 = key_2324;
                        s6 = 0x42;
                        s5 = 0;
                        fp = 0;
                    } else
                        //TAG 0x01000000
                        if(memcmp(&tags[1], swbuff_2604+0xD0, 4) == 0)
                        {
                            s1 = key_23B4;
                            s6 = 0x43;
                            s5 = 0;
                            fp = 0;
                        } else {
                            ret = -301;
                            goto loc_00000E64;
                        }

    if(s5 != 0 && check != 0)
    {
        s7 = 0;
        s1 = 0;
        ret = -202;
        goto loc_000003D4;
    }

    a2 = 0;
    if(s5 != 0)
    {
        int a3;
        for(a3 = 0; a3 < 0x30; a3++) if(swbuff_2604[0xD4+a3] != 0) t0 = -302; goto loc_00000E64; //exit
    } else {
        if(check_whitelist(prx, size, sha1_whitelist, 0x104, sync) != 1)
        {
            s7 = 0;
            s1 = 0;
            ret = -305;
            goto loc_000003D4;
        }
    }
loc_000002FC:
    v0 = 0;
    v1 = swbuff_2604;
    v0 = _lb(v1+0xB3);
    a0 = _lb(v1+0xB2);
    a1 = _lb(v1+0xB1);
    a2 = _lb(v1+0xB0);

    v0 = (v0 << 24) | (a0 << 16) | (a1 << 8) | a2;
    v1 = size + 0xFEB0;
    _sw(v0, newsize);

    if((v1 < v0) != 0) ret = -206; s7 = 0; s1 = 0; goto exit;
    a1 = 0;
    s7 = 0;
    if(s5 != 0)
    {
        int a1, a3;
        for(a3 = 0; a3 < 9; a3++)
        {
            for(a1 = 0; a1 < 16; a1++)
                hwbuff_A00[0x14+(a3<<4)+a1] = s1[a1];
            hwbuff_A00[0x14+(a3<<4)] = a3;
        }
        v0 = 0;
        a2 = swbuff_2780;
        int a1;
        for(a1 = 0; a1 < 0x28; a1++)
            swbuff_2780[a1] = prx[0x104+a1];
        int a0;
        for(a0 = 0; a0 < 0x28; a0++)
            prx[0x104+a0] = 0;
        for(a1 = 0; a1 < 0x28; a1++)
            sp[a1] = swbuff_2780[a1];

        //C80 or something
        v1 = newsize+0xFFFFFFFC;
        v0 = 0;
        prxbuff = prx;
        s4 = sync;
        if(sync)
        {
            t0 = sceUtilsBufferCopyByPollingWithRange(prx, newsize, prx, newsize, 0xB);
        } else {
            t0 = sceUtilsBufferCopyWithRange(prx, newsize, prx, newsize, 0xB);
        }
    } else {
        int a1;
        for(a1 = 0; a1 < 0x90; a1++)
            hwbuff_A00[0x14+a1] = s1[a1];
    }
loc_00000370:
    t0 = kirk7(hwbuff_A00, 0x90, s6, sync);

    s1 = 0;
    if(v0 == 0) loc_000004C4;


loc_000003D4:
    int a0;
    for(a0 = 0; a0 < 16; a0++) key_fat_destination[a0] = 0;
    for(a0 = 0; a0 < 16; a0++) key_slim_destination[a0] = 0;
    for(a0 = 0; a0 < 0x90; a0++) hwbuff_280[a0] = 0;
    for(a0 = 0; a0 < 0x150; a0++) hwbuff_A00[a0] = 0;
    for(a0 = 0; a0 < 0xB4; a0++) hwbuff_340[a0] = 0;
    for(a0 = 0; a0 < 0x150; a0++) swbuff_2604[a0] = 0;
    return ret;
}


int sub_0000020C(u8 *prx, u32 size, u32 *newsize, u32 sync) //decrypter
{
    int ret;
    if(prx == NULL || newsize == NULL){
        ret = -201;
        goto exit4;
    }
    if(size < 0x160){
        ret = -201;
        goto exitC;
    }
    if(((u32)prx & 0x3F) != 0)
    {
        ret = -203;
        goto exit4;
    }
    if (((0x00220202 >> (((u32)prx >> 27) & 0x001F)) & 0x1) == 0)
    {
        ret = -204;
        goto exit4;
    }
    a2 = swbuff_2064;

    int a1;
    for(a1 = 0; a1 < 0x150; a1++) swbuff_2064[a1] = prx[a1];
    v0 = 0;
    s1 = swbuff_2604+0xD0;
    a0 = &tag;
    a1 = s2;
    v0 = memcmp((u8*)tags+/*2300*/, swbuff_2604+0xD0, 4);
    fp = 0;
    if(v0 != 0) goto loc_00000EE4;
    s1 = key_slim_destination;
    s6 = 0x43;
    s5 = 4;

loc_000002E4:
    if(s5 != 0) goto loc_00000ED4;
loc_000002EC:
    v0 = 0;
    if(s5 != 0) goto loc_00000EA0;
loc_000002F4:
    a2 = 0;
    if(s5 == 0) goto loc_00000E70;


    //FOR EASIER LOOKING
loc_00000ED4:
    t0 = -202;
    if(s7 == 0) goto loc_000002EC;
    s7 = 0;
    goto loc_00000E68;
loc_00000E68:
    s1 = 0;
    goto exit;




loc_000003A8:
    int a1;
    for(a1 = 0; a1 < 0x150; a1++)
    {
        //it's u8!
        s0[a1] = swbuff_2604[a1];
    }
    _sw(0, s3);

loc_000003D4:
    int a0;
    for(a0 = 0; a0 < 16; a0++) key_fat_destination[a0] = 0;
    for(a0 = 0; a0 < 16; a0++) key_slim_destination[a0] = 0;
    for(a0 = 0; a0 < 0x90; a0++) hwbuff_280[a0] = 0;
    for(a0 = 0; a0 < 0x150; a0++) hwbuff_A00[a0] = 0;
    for(a0 = 0; a0 < 0xB4; a0++) hwbuff_340[a0] = 0;
    for(a0 = 0; a0 < 0x150; a0++) swbuff_2604[a0] = 0;
    return ret;

loc_00000E64:
    s7 = 0;
    s1 = 0;
    goto loc_000003D4;


loc_00000EE4:
    v0 = memcmp((u8*)tags+/*22FC*/, swbuff_2604+0xD0, 4);
    if(v0 != 0) v0 = 0; goto loc_00000F14;
    v0 = 0;
    s1 = key_2588;
loc_00000F04:
    s6 = 0x43;
    s5 = 4;
loc_00000F0C:
    fp = 0;
    goto loc_000002E4;

loc_00000F14:
    v0 = memcmp((u8*)tags+/*22F8*/, swbuff_2604+0xD0, 4);
    if(v0 != 0) v0 = 0; goto loc_00000F38;
    v0 = 0;
    s1 = key_fat_destination;
    goto loc_00000F04;

loc_00000F38:
    v0 = memcmp((u8*)tags+/*22F4*/, swbuff_2604+0xD0, 4);
    if(v0 != 0) v0 = 0; goto loc_00000F5C;
    v0 = 0;
    s1 = key_2598;
    goto loc_00000F04;

loc_00000F5C:
    v0 = memcmp((u8*)tags+/*22EC*/, swbuff_2604+0xD0, 4);
    if(v0 != 0) v0 = 0; goto loc_00000F88;
    v0 = 0;
    s1 = key_2324;
    s6 = 0x42;
loc_00000F80:
    s5 = 0;
    goto loc_00000F0C;

loc_00000F88:
    v0 = memcmp((u8*)tags+/*22F0*/, swbuff_2604+0xD0, 4);
    if(v0 != 0) v0 = 0; goto loc_00000FB0;
    s1 = key_23B4;
    s6 = 0x43;
    goto loc_00000F80;

loc_00000FB0:
    t0 = -301;
    fp = 0;
    goto loc_00000E64;


exit4:
    fp = 0;
    goto loc_00000E64;
exitC:
    ret = -202;
    goto exit4;
}

//0x00000FC4
/**
 * Decrypts a module. Asynced mode.
 *
 *
 * @param prx PRX buffer.
 * @param size Current size of PRX buffer.
 * @param newsize Size of PRX after decryption.
 *
 * @return 0 on success.
 */
int memlmd_EF73E85B(u8 * prx, unsigned int size, unsigned int * newsize) //DecryptPSP
{
    sceKernelPowerLock(0); //sceSuspendForKernel_EADB1BD7
    int ret = sub_0000020C(prx, size, newsize, 0);
    sceKernelPowerUnlock(0); //sceSuspendForKernel_3AEE7261
    return ret;
}

//0x00001028

//0x00001298

/**
 * Decrypts a module. Synced mode.
 *
 *
 * @param prx PRX buffer.
 * @param size Current size of PRX buffer.
 * @param newsize Size of PRX after decryption.
 *
 * @return 0 on success.
 */
int memlmd_CF03556B(u8 * prx, unsigned int size, unsigned int * newsize) //DecryptPSP
{
    sceKernelPowerLock(0);
    int ret = sub_0000020C(prx, size, newsize, 1);
    sceKernelPowerUnlock(0);
    return ret;
}

//0x0000108C
int kirk8(u8 *prx, u32 size, u32 sync)
{
    prx_buff = prx;
    ((u32 *) prx)[0] = 5;
    ((u32 *) prx)[3] = 0x100;
    ((u32 *) prx)[1] = 0;
    ((u32 *) prx)[2] = 0;
    ((u32 *) prx)[4] = size;
    if(sync != 0)
    {
        return sceUtilsBufferCopyByPollingWithRange(prx, size+20, prx, size+20, 8);
    }
    return sceUtilsBufferCopyWithRange(prx, size+20, prx, size+20, 8);
}

//0x000010F8
int unsign(u8 *prx, u32 arg, u32 sync)
{
    int ret;
    int i;

    if(prx == NULL)
    {
        ret = -201;
        goto exit;
    }
    if(arg < 0x160)
    {
        ret = -202;
        goto exit;
    }
    if(((u32)prx & 0x3F) != 0) //64 align
    {
        ret = -203;
        goto exit;
    }
    if (((0x00220202 >> (((u32)prx >> 27) & 0x001F)) & 0x1) == 0)
    {
        ret = -204;
        goto exit;
    }
    for(i = 0; i < 0xD0; i++)
        hwbuff_A00[i+0x14] = prx[i+0x80];

    for(i = 0; i < 0xD0; i++)
        hwbuff_A00[i+0x14] = hwbuff_A00[i+0x14] ^ key_2314[i&0xF];

    int ret2 = kirk8((u8*)hwbuff_A00, 0xD0, sync);
    if(ret2 == 0)
    {
        for(i = 0; i < 0xD0; i++)
            hwbuff_A00[i] = hwbuff_A00[i] ^ key_2304[i&0xF];

        for(i = 0; i < 0x90; i++)
            prx[i+0x80] = hwbuff_A00[i+0x40];

        for(i = 0; i < 0x40; i++)
            prx[i+0x110] = hwbuff_A00[i];

        ret = 0;
        goto exit;
    }
    if(ret2 < 0)
    {
        ret = -103;
        goto exit;
    }
    if(ret2 != 12) ret = -106;
    else ret = -107;

exit:
    //clean HW buffer
    for(i = 0; i < 0x164; i++) hwbuff_A00[i] = 0;
    return ret;

}

//0x00001298

/**
 * Usign a module. Asynced mode.
 *
 *
 * @param addr PRX buffer.
 * @param arg Unknown.
 *
 * @return 0 on success.
 */
int memlmd_6192F715(u8 *addr, u32 arg) //sceKernelCheckExecFile (it's rather memlmdUnsign)
{
    sceKernelPowerLock(0); //sceKernelPowerLock?
    int ret = unsign(addr, arg, 0);
    sceKernelPowerUnlock(0); //sceKernelPowerUnlock?
    return ret;
}

//0x000012EC

/**
 * Usign a module. Synced mode.
 *
 *
 * @param addr PRX buffer.
 * @param arg Unknown.
 *
 * @return 0 on success.
 */
int memlmd_EA94592C(unsigned char * addr, u32 arg)
{
    sceKernelPowerLock(0); //sceKernelPowerLock?
    int ret = unsign(addr, arg, 1);
    sceKernelPowerUnlock(0); //sceKernelPowerUnlock?
    return ret;
}

//0x00001340

/**
 * Creates the internal set of scramble keys by XORing pre-compiled set with
 * seed provided by user
 *
 * @param unk Unknown param. Completly not used, just pass something non-zero.
 * @param hash_addr Buffer with seeds to XOR with. Usually used hardware buffer
 * at 0xBFC00200
 *
 * @return 0 on success.
 */
int memlmd_F26A33C3(u32 unk, u8 *hash_addr) //initializeScrambleKey
{
    //no idea, really
    if(unk != 0) return -207;

    //check if hash_addr is not all zero
    int i;
    for(i = 0; i < 16; i++)
        if(hash_addr[i] != 0) break;
    if(i == 16) return -207;

    sceKernelPowerLock(0); //sceKernelPowerLock?
    for(i = 0; i < 16; i++)
    {
        key_slim_destination[i] = slim_seed[i] ^ hash_addr[i];
        key_fat_destination[i] = fat_seed[i] ^ hash_addr[i+16];
    }
    sceKernelPowerUnlock(0); //sceKernelPowerUnlock?
    return 0;
}

//0x00001414

/**
 * Memsets to 0 the scramble key buffers.
 *
 * @param unk Unknown param. Completly not used, just pass something non-zero.
 *
 * @return 0.
 */
int memlmd_76B7E315(u32 unk) //clearScrambleKey
{
    if(unk != 0) return -207;
    sceKernelPowerLock(0);
    int i;
    for(i = 0; i < 16; i++)
    {
        key_slim_destination[i] = 0;
        key_fat_destination[i] = 0;
    }
    sceKernelPowerUnlock(0);
    return 0;
}

//0x00001478
int module_start(SceSize args, void *argp)
{
    return InitSys();
}

//0x00001494
int module_reboot_before(SceSize args, void *argp)
{
    return StopSys();
}

//0x000014B0

/**
 * Checks the param against a magic value (unknown usage)
 *
 * @param unk Unknown param.
 *
 * @return ??.
 */
int memlmd_2AE425D2(u32 unk)
{
    u32 val = 0xF009FFFF;
    unk &= 0xFFFF0000;
    return (val < unk);
}

//0x000014C4

/**
 * Checks the param against a magic value (unknown usage)
 *
 * @param unk Unknown param.
 *
 * @return ??.
 */
int memlmd_9D36A439(u32 unk)
{
    u32 val = 0xF093FFFF;
    unk &= 0xFFFF0000;
    return (val < unk);
}

//0x000014D8
int InterruptHandler(int unk1, GlobalSysParam *my_sys)
{
    if(my_sys->operation == OP_ASYNC_SEMA) {
        my_sys->result = KIRK_GET_PROCESSING_RESULT();
        my_sys->operation = OP_IDLE;
        my_sys->status_pattern = KIRK_GET_PROCESSING_STATUS_PATTERN();
        KIRK_SET_END_PROCESSING_PATTERN(KIRK_GET_PROCESSING_STATUS_PATTERN());
        ASM_SYNC();
        sceKernelSetEventFlag(my_sys->utils_flag, 1); //ThreadManForKernel_1FB15A32
    } else {
        KIRK_SET_END_PROCESSING_PATTERN(KIRK_GET_PROCESSING_STATUS_PATTERN());
        ASM_SYNC();
    }
    return -1;
}

//0x00001558
int BusClock(int bus, int mode)
{
    unsigned int intr = sceKernelCpuSuspendIntr();
    u32 bus_mask = SYSCONF_GET_BUS_CLOCK_MASK();
    u32 result_mask;
    if(mode == BUS_ENABLE)
    {
        result_mask = bus_mask | bus;
    } else {
        result_mask = bus_mask & ~bus;
    }
    SYSCONF_SET_BUS_CLOCK_MASK(result_mask);
    sceKernelCpuResumeIntrWithSync(intr);
    return bus_mask;
}

//0x00015B0
int SystemSuspendHandler() {
    sceKernelDisableIntr(KIRK_INTERRUPT); //InterruptManagerForKernel_D774BA45
    if (system.operation == OP_ASYNC_SEMA) {
        while((KIRK_GET_PROCESSING_STATUS_PATTERN() & 0x33) == 0) { /* wait for kirk to finish */ };

        system.result = KIRK_GET_PROCESSING_RESULT();
        system.status_pattern = KIRK_GET_PROCESSING_STATUS_PATTERN();
        system.operation = OP_IDLE;
        KIRK_SET_END_PROCESSING_PATTERN(KIRK_GET_PROCESSING_STATUS_PATTERN());
        ASM_SYNC();
        sceKernelSetEventFlag(system.utils_flag, 1); //ThreadManForKernel_1FB15A32
    }

    if (!SYSCONF_CHECK_BUS_CLOCK_ENABLED(KIRK_BUS_CLOCK)) {
        system.kirk_enabled = 0;
    }
    else {
        system.kirk_enabled = 1;
    }
    BusClock(KIRK_BUS_CLOCK, BUS_DISABLE);
    return 0;
}

//0x0000167C
int SystemResumeHandler() {
    BusClock(KIRK_BUS_CLOCK, BUS_ENABLE);

    KIRK_SET_INCOMPLETE_PROCESSING_ERROR(1);
    while(KIRK_GET_INCOMPLETE_PROCESSING_ERROR() == 1) { /* wait for kirk to finish */ }
    sceKernelEnableIntr(KIRK_INTERRUPT); //InterruptManagerForKernel_4D6E7305

    Semaphore(selftest_buffer, 0x1C, selftest_buffer, 0x1C, 0xF, 0);

    if (system.kirk_enabled != 1) {
        BusClock(KIRK_BUS_CLOCK, BUS_DISABLE);
    }

    return 0;
}

//0x00001708
int CallSysEventHandler(int ev_id, char *ev_name, void *param, int *result)
{
    if(ev_id == 0x4000)
    {
        return SystemSuspendHandler();
    }
    else if(ev_id == 0x10000)
    {
        return SystemResumeHandler();
    }
    return 0;
}

//0x00001754
int StopSys()
{
    sceKernelUnregisterSysEventHandler(&sysevent_handler); //sceSysEventForKernel_D7D3FDCD
    sceKernelReleaseIntrHandler(KIRK_INTERRUPT); //InterruptManagerForKernel_F987B1F0
    sceKernelSignalSema(system.utils_sema, 1); //ThreadManForKernel_3F53E640
    BusClock(KIRK_BUS_CLOCK, BUS_ENABLE);
    return 0;
}

//0x0000179C
int InitSys()
{
    int ret;
    system.operation = OP_IDLE;

    ret = sceKernelRegisterIntrHandler(KIRK_INTERRUPT, 1, &InterruptHandler, &system, 0);
    if (ret != 0) return 1;

    ret = sceKernelEnableIntr(KIRK_INTERRUPT);
    if (ret != 0) {
        sceKernelReleaseIntrHandler(KIRK_INTERRUPT);
        return 1;
    }

    ret = sceKernelRegisterSysEventHandler(&sysevent_handler);
    if (ret != 0) {
        sceKernelDisableIntr(KIRK_INTERRUPT);
        sceKernelReleaseIntrHandler(KIRK_INTERRUPT);
        return 1;
    }

    system.utils_sema = sceKernelCreateSema("SceUtils1024", 0, 1, 1, 0);
    if (system.utils_sema <= 0) {
        sceKernelUnregisterSysEventHandler(&sysevent_handler);
        sceKernelDisableIntr(KIRK_INTERRUPT);
        sceKernelReleaseIntrHandler(KIRK_INTERRUPT);
        return 1;
    }

    system.utils_flag = sceKernelCreateEventFlag("SceUtils1024", 0, 0, 0);
    if (system.utils_flag <= 0) {
        sceKernelUnregisterSysEventHandler(&sysevent_handler);
        sceKernelDisableIntr(KIRK_INTERRUPT);
        sceKernelReleaseIntrHandler(KIRK_INTERRUPT);
        sceKernelDeleteSema(system.utils_sema);
        return 1;
    }

    int i;
    for (i = 0; i < 0x1C; i++)
        selftest_buffer[i] = 0;

    ret = sceUtilsBufferCopyByPolling(selftest_buffer, selftest_buffer, 0xF);
    if (ret != 0) {
        sceKernelUnregisterSysEventHandler(&sysevent_handler);
        sceKernelDisableIntr(KIRK_INTERRUPT);
        sceKernelReleaseIntrHandler(KIRK_INTERRUPT);
        sceKernelDeleteSema(system.utils_sema);
        sceKernelDeleteEventFlag(system.utils_flag);
        return 1;
    }

    return 0;
}

//0x00001928
int Semaphore(u8* out_buf, int out_len, u8* in_buf, int in_len, int mode, int skip_size_check)
{
    system.operation = OP_SYNC_SEMA;

    if (!skip_size_check){
        if (out_buf != in_buf) {
            sceKernelDcacheWritebackInvalidateRange(out_buf, out_len); //UtilsForKernel_34B9FA9E
            sceKernelDcacheWritebackInvalidateRange(in_buf, in_len);
        } else {
            if (in_len >= out_len) out_len = in_len;
        }
        sceKernelDcacheWritebackInvalidateRange(out_buf, out_len);
    }
    else {
        sceKernelDcacheWritebackInvalidateAll(); //UtilsForKernel_B435DEC5
    }
    u32 in = ((u32)in_buf) & 0x1FFFFFFF;
    u32 out = ((u32)out_buf) & 0x1FFFFFFF;

    KIRK_SET_COMMAND(mode);
    KIRK_SET_SOURCE_ADDR(in);
    KIRK_SET_DEST_ADDR(out);
    KIRK_SET_PROCESSING_PHASE(1);

    while((KIRK_GET_PROCESSING_STATUS_PATTERN() & 0x33) == 0){}; //wait kirk for finish

    if((KIRK_GET_PROCESSING_STATUS_PATTERN() & 0x22) == 0)
    {
        if((KIRK_GET_PROCESSING_STATUS_PATTERN() & 0x10) == 0)
        {
            return KIRK_GET_PROCESSING_RESULT();
        } else {
            KIRK_SET_PROCESSING_PHASE(2);

            while((KIRK_GET_PROCESSING_STATUS_PATTERN() & 0x33) == 0){};

            if((KIRK_GET_PROCESSING_STATUS_PATTERN() & 0x2) == 0)
            {
                KIRK_SET_INCOMPLETE_PROCESSING_ERROR(1);
            }
        }
    } else {
        KIRK_SET_INCOMPLETE_PROCESSING_ERROR(1);
    }

    system.operation = OP_IDLE;
    KIRK_SET_END_PROCESSING_PATTERN(KIRK_GET_PROCESSING_STATUS_PATTERN());
    ASM_SYNC();
    return -1;
}

//0x00001AA0
int SemaphoreAsync(u8* out_buf, int out_len, u8* in_buf, int in_len, int mode, int skip_size_check)
{
    int intr, ret;

    if (!skip_size_check) {
        if (out_buf != in_buf) {
            sceKernelDcacheWritebackInvalidateRange(out_buf, out_len);
            sceKernelDcacheWritebackInvalidateRange(in_buf, in_len);
        }
        else {
            if (in_len > out_len) out_len = in_len;
        }
        sceKernelDcacheWritebackInvalidateRange(out_buf, out_len);
    }
    else {
        sceKernelDcacheWritebackInvalidateAll();
    }

    u32 in = ((u32)in_buf) & 0x1FFFFFFF;
    u32 out = ((u32)out_buf) & 0x1FFFFFFF;

    intr = sceKernelCpuSuspendIntr();

    KIRK_SET_COMMAND(mode);
    system.operation = OP_ASYNC_SEMA;
    KIRK_SET_SOURCE_ADDR(in);
    KIRK_SET_DEST_ADDR(out);
    KIRK_SET_INIT_ASYNC_PROCESSING_PATTERN(0x33);
    KIRK_SET_PROCESSING_PHASE(1);

    sceKernelCpuResumeIntrWithSync(intr);

    ret = sceKernelWaitEventFlag(system.utils_flag, 1, 0x21, 0, 0);
    if (ret != 0) return -1;

    KIRK_SET_END_ASYNC_PROCESSING_PATTERN(0x33);

    if ((system.status_pattern & 0x22) != 0){
        KIRK_SET_INCOMPLETE_PROCESSING_ERROR(1);
        ASM_SYNC();
        return -1;
    }

    if ((system.status_pattern & 0x10) != 0){
        intr = sceKernelCpuSuspendIntr();
        KIRK_SET_INIT_ASYNC_PROCESSING_PATTERN(2);
        system.operation = OP_ASYNC_SEMA;
        KIRK_SET_PROCESSING_PHASE(2);
        sceKernelCpuResumeIntrWithSync(intr);

        ret = sceKernelWaitEventFlag(system.utils_flag, 1, 0x21, 0, 0);
        if (ret != 0) return -1;

        KIRK_SET_END_ASYNC_PROCESSING_PATTERN(2);

        if ((system.status_pattern & 0x2) != 0) return -1;

        KIRK_SET_INCOMPLETE_PROCESSING_ERROR(1);
        ASM_SYNC();
        return -1;
    }

    return system.result;
}

//0x00001C48

/**
 * Enables KIRK, then performs a command in asynchronised mode (refreshes
 * whole CPU D cache), and disables KIRK.
 *
 * @param outbuff Output buffer.
 * @param outsize Output size.
 * @param inbuff Input buffer.
 * @param insize Input size.
 * @param cmd Number of KIRK command to perform.
 *
 * @return 0 on success.
 */
int sceUtilsBufferCopy(u8* outbuff, u8* inbuff, int cmd) //semaphore_00EEC06A
{
    int func_ret = -1;
    int ret = sceKernelPollSema(system.utils_sema, 1); //ThreadManForKernel_58B1F937
    if(ret == 0)
    {
        BusClock(KIRK_BUS_CLOCK, BUS_ENABLE);
        func_ret = SemaphoreAsync(outbuff, 0, inbuff, 0, cmd, 1);
        BusClock(KIRK_BUS_CLOCK, BUS_DISABLE);
        ret = sceKernelSignalSema(system.utils_sema, 1); //ThreadManForKernel_3F53E640
        if(ret != 0) func_ret = -1;
    }
    return func_ret;
}

//0x00001D04

/**
 * Enables KIRK, then performs a command in synchronised mode (refreshes
 * whole CPU D cache), and disables KIRK.
 *
 * @param outbuff Output buffer.
 * @param outsize Output size.
 * @param inbuff Input buffer.
 * @param insize Input size.
 * @param cmd Number of KIRK command to perform.
 *
 * @return 0 on success.
 */
int sceUtilsBufferCopyByPolling(u8* outbuff, u8* inbuff, int cmd) //semaphore_8EEB7BF2
{
    int func_ret = -1;
    int ret = sceKernelPollSema(system.utils_sema, 1);
    if(ret == 0)
    {
        BusClock(KIRK_BUS_CLOCK, BUS_ENABLE);
        func_ret = Semaphore(outbuff, 0, inbuff, 0, cmd, 1);
        BusClock(KIRK_BUS_CLOCK, BUS_DISABLE);
        ret = sceKernelSignalSema(system.utils_sema, 1);
        if(ret != 0) func_ret = -1;
    }
    return func_ret;
}

//0x00001DBC

/**
 * Enables KIRK, then performs a command in asynchronised mode (refreshes
 * CPU D cache for input and output buffer), and disables KIRK.
 *
 * @param outbuff Output buffer.
 * @param outsize Output size.
 * @param inbuff Input buffer.
 * @param insize Input size.
 * @param cmd Number of KIRK command to perform.
 *
 * @return 0 on success.
 */
int sceUtilsBufferCopyWithRange(u8* outbuff, int outsize, u8* inbuff, int insize, int cmd) //semaphore_4C537C72
{
    int func_ret = -1;
    int ret = 0;sceKernelPollSema(system.utils_sema, 1);
    if(ret == 0)
    {
        BusClock(KIRK_BUS_CLOCK, BUS_ENABLE);
        func_ret = SemaphoreAsync(outbuff, outsize, inbuff, insize, cmd, 0);
        BusClock(KIRK_BUS_CLOCK, BUS_DISABLE);
        ret = sceKernelSignalSema(system.utils_sema, 1);
        if(ret != 0) func_ret = -1;
    }
    return func_ret;
}

//0x00001E90

/**
 * Enables KIRK, then performs a command in synchronised mode (refreshes
 * CPU D cache for input and output buffer), and disables KIRK.
 *
 * @param outbuff Output buffer.
 * @param outsize Output size.
 * @param inbuff Input buffer.
 * @param insize Input size.
 * @param cmd Number of KIRK command to perform.
 *
 * @return 0 on success.
 */
int sceUtilsBufferCopyByPollingWithRange(u8* outbuff, int outsize, u8* inbuff, int insize, int cmd) //semaphore_77E97079
{
    int func_ret = -1;
    int ret = sceKernelPollSema(system.utils_sema, 1);
    if(ret == 0)
    {
        BusClock(KIRK_BUS_CLOCK, BUS_ENABLE);
        func_ret = Semaphore(outbuff, outsize, inbuff, insize, cmd, 0);
        BusClock(KIRK_BUS_CLOCK, BUS_DISABLE);
        ret = sceKernelSignalSema(system.utils_sema, 1);
        if(ret != 0) func_ret = -1;
    }
    return func_ret;
}

//0x00001F60

/**
 * Enables the bus of KIRK chip if possibile
 *
 *
 * @return 0 on success.
 */
int memlmd_2F3D7E2D()
{
    int ret = sceKernelPollSema(system.utils_sema, 1);
    if(ret == 0)
    {
        BusClock(KIRK_BUS_CLOCK, BUS_ENABLE);
        ret = sceKernelSignalSema(system.utils_sema, 1);
        if(ret == 0) return 0;
    }
    return -1;
}
