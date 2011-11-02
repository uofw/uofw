#include "../global.h"

typedef struct
{
    int size;
    int opt1;
    int opt2;
    char *typeName; /* vsh, umdemu.. */
    int opt4;
    int opt5;
    int opt6;
    int opt7;
    int loadPspbtcnf; // Always set to 0x10000
    int opt9;
    int opt10;
    int opt11;
} RebootArgs2;

typedef struct
{
    int opt0;
    int opt1;
    char *fileName;
    RebootArgs2 *args2;
    int opt4;
    void *opt5;
    int opt6;
    int opt7;
} RebootArgs; 

static char g_encryptedBootPath[]   = "disc0:/PSP_GAME/SYSDIR/EBOOT.BIN"; // 0x3AE4
static char g_unencryptedBootPath[] = "disc0:/PSP_GAME/SYSDIR/BOOT.BIN"; // 0x3B18
static char g_gameStr[] = "game"; // 0x3B38
static char g_umdEmuStr[] = "umdemu"; // 0x3B40
static char g_mlnAppStr[] = "mlnapp"; // 0x3B38
static char g_vshStr[] = "vsh"; // 0x3B40

SceUID g_loadExecMutex; // 0xD3C0
SceUID g_loadExecCb; // 0xD3C4
int g_loadExecIsInited; // 0xD3C8
int g_valAreSet; // 0xD3CC
int g_val0, g_val1, g_val2; // 0xD3D0, 0xD3D4, 0xD3D8
void (*g_loadExecFunc)(); // 0xD3DC, some unknown callback

char **g_encryptedBootPathPtr = &g_encryptedBootPath; // 0xD390 [hardcoded??]

int *g_unkCbInfo[4]; // 0xD400

//#define EXEC_START 0x3E80 // TODO: store new executable in an array and set this as the executable start & end
//#define EXEC_END   0xD350

// 0000
void decodeKL4E(char *dst, int size, char *src, int arg3)
{
    char *end = dst + size;
    char *curDst = dst;
    arg1 = sp - 2656;
    t9 = 0;
    s3 = -1;
    s4 = (end - 64) & 0xFFFFFFC0;
    s0 = src[0];
    t1 = *(int*)&src[1]; // unaligned reading
    t1 = wsbw(t1);
    if (s0 < 0)
    {
        // 744
        if (arg3 != 0)
            *arg3 = src + 5;
        return 0x80000108;
    }
    at = (s0 & 0x18) >> 2;
    at <<= 4;
    at = 128 - at;
    at = (at & 0xffff00ff) | ((at << 8) & 0xff00);
    s0 = s0 & 7;
    at = (at & 0x0000ffff) | ((at << 16) & 0xffff0000);

    // 06C: copy 'at' from sp to sp + 2652 included
    do
    {
        *(arg1 + 2656) = at;
        *(arg1 + 2660) = at;
        arg1 += 8;
    } while (arg1 != sp)
    at = 255;

    // 080
    for (;;)
    {
        t9 = (t9 & 0xfffff8ff) | ((curDst << 8) & 0x700);
        v1 = t9 >> (s0 & 0x1f);
        v1 &= 0x7;
        v1 = sp + v1 * 255;
        t9 = 1;
    
        // 09C
        do
        {
            t7 = v1 + t9;
            t5 = s3 >> 24;
            t6 = *(u8*)(t7 - 1);
            if (t5 == 0)
            {
                // 0EC
                s2 = *(u8*)(src + 5);
                t1 <<= 8;
                src++;
                t1 += s2;
                t9 <<= 1;
                lo = s3 * t6;
                s3 <<= 8;
            }
            else
            {
                t9 <<= 1;
                s2 = s3 >> 8;
                lo = s2 * t6;
            }
            s2 = t6 >> 3;
            t6 -= s2;
            t5 = lo;
            s2 = t9 >> 8;
    
            if (t1 >= t5)
            {
                // 120
                *(s8*)(t7 - 1) = t6;
                t1 -= t5;
                s3 -= t5;
            }
            else
            {
                // 0D0
                s3 = lo;
                t6 += 31;
                *(s8*)(t7 - 1) = t6;
                t9++;
            }
        } while (s2 == 0);
        // 0D0 == 120
        *(s8*)(curDst + 0) = t9;
    
        // 134
        for (;;)
        {
            s1 = s3 >> 24;
            t5 = *(u8*)(arg1 + 2336);
            v1 = s3 >> 8;
            if (s1 == 0)
            {
                // 17C
                t8 = *(u8*)(src + 5)
                curDst++;
                t1 <<= 8;
                src++;
                t1 += t8;
                lo = s3 * t5;
                s3 <<= 8;
            }
            else {
                lo = v1 * t5;
                curDst++;
            }
        
            v1 = t5 >> 4;
            s1 = lo;
            t5 -= v1;
            if (t1 < s1)
            {
                // 1AC
                s3 = t5 + 15;
                *(s8*)(arg1 + 2336) = s3;
                t8 = -1;
                s3 = lo;
                // 1BC
                int goto_3a4 = 0;
                for (;;)
                {  
                    t9 = s1 >> 24;
                    t5 = *(u8*)(arg1 + 2344);
                    if (t9 == 0)
                    {  
                        // 750
                        v1 = *(u8*)(src + 5);
                        t1 <<= 8;
                        src++;
                        t1 += v1;
                        lo = s3 * t5;                                                                                                                                         
                        s3 = s1 << 8;
                    }
                    else {
                        s2 = s3 >> 8;
                        lo = s2 * t5;
                    }
            
                    arg1 += 8;
                    s2 = t5 >> 4;
                    t5 -= s2;
                    s1 = lo;
                    v1 = t5 + 15;
                    if (t1 < s1)
                    {
                        // 784
                        t8++;
                        s3 = lo;
                        *(s8*)(arg1 + 2336) = v1;
                        if (t8 == 6) {
                            t5 = sp + t8;
                            break;
                        }
                        // CONTINUE
                    }
                    else
                    {
                        // 1EC
                        s3 -= s1;
                        t1 -= s1;
                        *(s8*)(arg1 + 2336) = t5;
                        t5 = sp + t8;
                        if (t8 < 0) {
                            // 3A0
                            arg1 = sp + 7;
                            goto_3a4 = 1;
                        }
                        break;
                    }
                }
            
                if (!goto_3a4)
                {
                    // 200
                    t9 = t8 - 3;
                    t6 = curDst << (t8 & 0x1f);
                    s1 = t8 << 5;
                    s1 = (s1 & 0xffffffe7) | ((t6 << 3) & 0x18);
                    s1 = (s1 & 0xfffffff8) | (arg1 & 3);
                    s1 += sp;
                    t6 = 1;
                    if (t9 >= 0)
                    {
                        s2 = *(u8*)(s1 + 2424)
                        v1 = s3 >> 24;
                        v0 = s3 >> 8;
                        if (v1 == 0)
                        {  
                            // 858
                            v1 = *(u8*)(src + 5);
                            t1 <<= 8;
                            src++;
                            t1 += v1;
                            lo = s3 * s2;
                            s3 <<= 8;
                        }
                        else
                            lo = v0 * s2;
                       
                        // 234
                        v0 = s2 >> 3;
                        s2 -= v0;
                        v1 = lo;
                        t6 = 2;
                        if (t1 < v1)
                        {
                            // 8A8
                            s3 = lo;
                            t6 = 3;
                            s2 += 31;
                        }
                        else {
                            t1 -= v1;
                            s3 -= v1;
                        }
            
                        if (t9 > 0)
                        {
                            v1 = s3 >> 24;
                            v0 = s3 >> 8;
            
                            if (v1 == 0)
                            {
                                // 8C4
                                v1 = *(u8*)(src + 5);
                                t1 <<= 8;
                                src++;
                                t1 += v1;
                                s3 <<= 8;
                                v0 = s3 >> 8;
                            }
            
                            // 264
                            lo = v0 * s2;
                            v0 = s2 >> 3;
                            s2 -= v0;
                            v1 = lo;
                            t6 <<= 1;
                            if (t1 < v1)
                            {
                                // 874
                                s3 = lo;
                                t6++;
                                s2 += 31;
                            }
                            else {
                                t1 -= v1;
                                s3 -= v1;
                            }
            
                            if (t9 != 1)
                            {
                                v1 = s3 >> 24;
                                t6 <<= 1;
                               
                                if (v1 == 0)
                                {  
                                    // 890
                                    v1 = *(u8*)(src + 5);
                                    t1 <<= 8;
                                    src++;
                                    t1 += v1;
                                    s3 <<= 7;
                                }
                                else
                                    s3 >>= 1;
                                // 29C
                                for (;;)
                                {  
                                    v1 = t1 - s3;
                                    t9--;
                                    if (t1 >= s3)
                                        t1 = v1;
                                    t6 += v0;
                                    if (t9 == 1)
                                        break;
                                    t6 <<= 1;
                                    s3 >>= 1;
                                }
                            }
                        }
                    }
                    else // 2B8
                        *(s8*)(s1 + 2424) = s2;
            
                    // 2BC
                    v1 = s3 >> 24;
                    arg1 = sp + 7;
                    s2 = *(u8*)(s1 + 2400);
                    v0 = s3 >> 8;
                    if (v1 == 0)
                    {  
                        // 80C
                        v1 = *(u8*)(src + 5);
                        t1 <<= 8;
                        src++;
                        t1 += v1;
                        lo = s3 * s2;
                        s3 <<= 8;
                    }   
                    else
                        lo = v0 * s2;
            
                    v0 = s2 >> 3;
                    s2 -= v0;
                    v1 = lo;
                    t6 <<= 1;
                    int stop = 0;
                    if (t1 < v1)
                    {  
                        // 83C
                        s2 += 31;
                        *(s8*)(s1 + 2400) = s2;
                        s3 = lo;
                        t6++;
                        if (t8 <= 0)
                            stop = 1;
                    }
                    else
                    {  
                        // 2EC
                        *(s8*)(s1 + 2400) = s2;
                        t1 -= v1;
                        s3 -= v1;
                        if (t8 <= 0)
                            goto_3a4 = 1;
                    }
                    
                    if (!stop && !goto_3a4)
                    {  
                        s2 = *(u8*)(s1 + 2408);
                       
                        // 300
                        v1 = s3 >> 24;
                        v0 = s3 >> 8;
                        if (v1 == 0)
                        {  
                            // 7F0
                            v1 = *(u8*)(src + 5);
                            t1 <<= 8;
                            src++;
                            t1 += v1;
                            lo = s3 * s2;
                            s3 <<= 8;
                        }
                        else
                            lo = v0 * s2;
                       
                        // 310
                        s2 -= (s2 >> 3);
                        v1 = lo;
                        t6 <<= 1;
                        if (t1 < v1)
                        {  
                            // 7D4
                            s2 += 31;
                            *(s8*)(s1 + 2408) = s2;
                            t6++;
                            s3 = lo;
                        }
                        else
                        {
                            *(s8*)(s1 + 2408) = s2;
                            t1 -= v1;
                            s3 -= v1;
                        }
                        if (t8 != 1)
                        {
                            s2 = *(u8*)(s1 + 2416);
            
                            // 33C
                            v1 = s3 >> 24;
                            v0 = s3 >> 8;
                            if (v1 == 0)
                            {
                                // 79C
                                v1 = *(u8*)(src + 5);
                                t1 <<= 8;
                                src++;
                                t1 += v1;
                                lo = s3 * s2;
                                s3 <<= 8;
                            }
                            else
                                lo = v0 * s2;
                           
                            // 34C
                            v0 = s2 >> 3;
                            s2 -= v0;
                            v1 = lo;
                            t6 <<= 1;
                            if (t1 < v1)
                            {  
                                // 7B8
                                s2 += 31;
                                *(s8*)(s1 + 2416) = s2;
                                t6++;
                                s3 = lo;
                                if (t6 == 0xFF) {
                                    // END (8E8)
                                    if (arg3 != 0)
                                        *arg3 = src + 5;
                                    // 8F4
                                    return curDst - dst;
                                }
                            }
                            else
                            {  
                                *(s8*)(s1 + 2416) = s2;
                                t1 -= v1;
                                s3 -= v1;
                            }
                        }
                    }
                   
                    if (!goto_3a4)
                    {  
                        // 370
                        t5 += 56;
                        s2 = 128;
                        s1 = 8;
                        goto_3ac = 1;
                    }
                }
                else
                {
                    // 3A4
                    s2 = 64;
                    s1 = 8;
                    goto_3ac = 1;
                }
                if (goto_3ac)
                {
                    // 3AC
                    for (;;)
                    {
                        v1 = s3 >> 24;
                        t9 = t5 + s1;
                        if (v1 == 0)
                        {
                            // 388
                            v1 = *(u8*)(src + 5);
                            t1 <<= 8;
                            src++;
                            t1 += v1;
                            s3 <<= 8;
                        }
                
                        // 3B8
                        t8 = *(u8*)(t9 + 2033);
                        v0 = s3 >> 8;
                        s1 <<= 1;
                        lo = v0 * t8;
                        v0 = t8 >> 3;
                        t7 = t8 - v0;
                        v1 = lo;
                        t8 = s1 - s2;
                        int goto_3f4 = 0;
                        int goto_574 = 0;
                        if (t1 >= v1)
                        {
                            // 71C
                            s3 -= v1;
                            t1 -= v1;
                            *(s8*)(t9 + 2033) = t7;
                            if (t8 >= 0)
                            {
                                t8 -= 8;
                                if (s1 == s2)
                                {
                                    v1 = curDst;
                                    t5 = curDst - dst;
                                    t6 += curDst;
                                    if (t5 != 0)
                                        goto_574 = 1;
                                    else
                                    {
                                        // 744
                                        if (arg3 != 0)
                                            *arg3 = src + 5;
                                        return 0x80000108;
                                    }
                                }
                                else
                                    goto_3f4 = 1;
                            }
                        }
                        else
                        {
                            s3 = lo;
                            t7 += 31;
                            *(s8*)(t9 + 2033) = t7;
                            s1 += 8;
                            if (t8 >= 0)
                                goto_3f4 = 1;
                        }
                
                        if (goto_3f4)
                        {
                            // 3F4
                            s1 = sp + t8;
                            t8 = (s32)t8 >> 3;
                            t5 = *(s1 + 2216);
                            t9 = t8 - 3;
                            t7 = 1;
                            if (t9 >= 0)
                            {  
                                v0 = s3 >> 24;
                                s2 = (t5 & 0xff000000) >> 24;
                                if (v0 == 0)
                                {  
                                    // 694
                                    v1 = *(u8*)(src + 5);
                                    t1 <<= 8;
                                    src++;
                                    t1 += v1;
                                    lo = s3 * s2;
                                    s3 <<= 8;
                                }
                                else {
                                    v0 = s3 >> 8;
                                    lo = v0 * s2;
                                }
                
                                // 420
                                v0 = s2 >> 3;
                                s2 -= v0;
                                v1 = lo;
                                t7 = 2;
                                if (t1 < v1)
                                {
                                    // 6E4
                                    s3 = lo;
                                    t7 = 3;
                                    s2 += 31;
                                }
                                else {
                                    t1 -= v1;
                                    s3 -= v1;
                                }
                
                                if (t9 > 0)
                                {
                                    v0 = s3 >> 8;
                                    if (s3 >> 24)
                                    {
                                        // 700
                                        v1 = *(u8*)(src + 5);
                                        t1 <<= 8;
                                        src++;
                                        t1 += v1;
                                        s3 <<= 8;
                                        v0 = s3 >> 8;
                                    }
                
                                    // 450
                                    lo = v0 * s2;
                                    v0 = s2 >> 3;
                                    s2 -= v0;
                                    v1 = lo;
                                    t7 <<= 1;
                
                                    if (t1 < v1)
                                    {
                                        // 6B0
                                        s3 = lo;
                                        t7++;
                                        s2 += 31;
                                    }
                                    else {
                                        t1 -= v1;
                                        s3 -= v1;
                                    }
                
                                    if (t9 != 1)
                                    {
                                        v0 = s3 >> 24;
                                        t7 <<= 1;
                                        if (v0 == 0)
                                        {
                                            // 6CC
                                            v1 = *(u8*)(src + 5);
                                            t1 <<= 8;
                                            src++;
                                            t1 += v1;
                                            s3 <<= 7;
                                        }
                                        else // 484
                                            s3 >>= 1;
                                        // 488
                                        for (;;)
                                        {
                                            v1 = t1 - s3;
                                            t9--;
                                            if (t1 >= s3)
                                                t1 = v1;
                                            t7 += v0;
                                            if (t9 == 1)
                                                break;
                                            t7 <<= 1;
                                            s3 >>= 1;
                                        }
                                    }
                                }
                            }
                            else {
                                // 4A4
                                t5 = (t5 & 0x00ffffff) | ((s2 << 24) & 0xff000000);
                            }
                            // 4A8
                            v0 = s3 >> 24;
                            s2 = t5 & 0xff;
                            if (v0 == 0)
                            {
                                // 678
                                v1 = *(u8*)(src + 5);
                                t1 <<= 8;
                                src++;
                                t1 += v1;
                                lo = s3 * s2;
                                s3 <<= 8;
                            }
                            else {
                                v0 = s3 >> 8;
                                lo = v0 * s2;
                            }
                   
                            // 4BC
                            v0 = s2 >> 3;
                            s2 -= v0;
                            v1 = lo;
                            t7 <<= 1;
                            int goto_55c = 0;
                            if (t1 < v1)
                            {
                                // 644
                                s2 += 31;
                                t5 = (t5 & 0xffffff00) | (s2 & 0x000000ff);
                                s3 = lo;
                                if (t8 <= 0)
                                    goto_55c = 1;
                                else
                                    t7++;
                            }   
                            else
                            {  
                                t5 = (t5 & 0xffffff00) | (s2 & 0xff);
                                t1 -= v1;
                                if (t8 <= 0) {
                                    t7--;
                                    goto_55c = 1;
                                }
                                else
                                    s3 -= v1;
                            }  
                            if (!goto_55c)
                            {  
                                // 4E4
                                v0 = s3 >> 24;
                                s2 = (t5 & 0xff00) >> 8;
                                if (v0 == 0)
                                {
                                    // 65C
                                    v1 = *(u8*)(src + 5);
                                    t1 <<= 8;
                                    src++;
                                    t1 += v1;
                                    lo = s3 * s2;
                                    s3 <<= 8;
                                }
                                else {
                                    v0 = s3 >> 8;
                                    lo = v0 * s2;
                                }
                
                                // 4F8
                                v0 = s2 >> 3;
                                s2 -= v0;
                                v1 = lo;
                                t7 <<= 1;
                                if (t1 < v1)
                                {
                                    // 62C
                                    s2 += 31;
                                    t5 = (t5 & 0xffff00ff) | ((s2 << 8) & 0x0000ff00);
                                    s3 = lo;
                                    if (t8 == 1)
                                        goto_55c = 1;
                                    else
                                        t7++;
                                }
                                else
                                {
                                    t5 = (t5 & 0xffff00ff) | ((s2 << 8) & 0x0000ff00);
                                    t1 -= v1;
                                    s3 -= v1;
                                    if (t8 == 1) {
                                        t7--;
                                        goto_55c = 1;
                                    }
                                }
                
                                if (!goto_55c)
                                {
                                    // 520
                                    v0 = s3 >> 24;
                                    s2 = (t5 & 0x00ff0000) >> 16;
                                    if (v0 == 0)
                                    {  
                                        // 600
                                        v1 = *(u8*)(src + 5);
                                        t1 <<= 8;
                                        src++;
                                        t1 += v1;
                                        lo = s3 * s2;
                                        s3 <<= 8;
                                    }
                                    else {
                                        v0 = s3 >> 8;
                                        lo = v0 * s2;
                                    }
                                    
                                    // 534
                                    v0 = s2 >> 3;
                                    s2 -= v0;
                                    v1 = lo;
                                    t7 <<= 1;
                                    if (t1 < v1)
                                    {  
                                        // 61C
                                        s2 += 31;
                                        t5 = (t5 & 0xff00ffff) | ((s2 << 16) & 0x00ff0000);
                                        s3 = lo;
                                    }
                                    else
                                    {  
                                        t5 = (t5 & 0xff00ffff) | ((s2 << 16) & 0x00ff0000);
                                        t1 -= v1;
                                        s3 -= v1;
                                        t7--;
                                    }
                                }
                            }
                            // 55C
                            *(s1 + 2216) = t5;
                            t5 = curDst - dst;
                            t6 += curDst;
                            if (t7 >= t5)
                            {
                                // 744
                                if (arg3 != 0)
                                    *arg3 = src + 5;
                                return 0x80000108;
                            }
                            v1 = curDst - t7;
                        }
                        else if (!goto_574)
                            continue;
                        break;
                    }
                
                    // 574
                    t9 = *(u8*)(v1 - 1);
                    if (t6 >= end)
                    {
                        // 8E0
                        if (arg3 != 0)
                            *arg3 = src + 5;
                        return 0x80000104;
                    }
                    arg1 = (arg1 & 0xfffffffe) | (t6 & 1);
                    if (end < s4)
                    {  
                        cache(0x18, curDst + 64);
                        // 5AC
                        s1 = curDst - v1;
                        if (t6 - curDst >= 4)
                        {
                            v0 = curDst;
                            if (s1 >= 3)
                            {
                                curDst = t6;
                                t6 = (t6 & 0xfffffffc) | (v0 & 3);
                                // 5D0
                                for (;;)
                                {  
                                    *(int*)v0 = *(int*)(v1 - 1); // unaligned R-W
                                    v1 += 4;
                                    if (t6 == v0)
                                    {  
                                        // 5F0
                                        t9 = *(u8*)curDst;
                                        if (curDst != end)
                                            break;
                                        // END (8E8)
                                        if (arg3 != 0)
                                            *arg3 = src + 5;
                                        return curDst - dst;
                                    }
                                    v0 += 4;
                                }
                            }
                        }
                    }
                    else
                    {  
                        // 590
                        do
                        {  
                            *(s8*)curDst = t9;
                            curDst++;
                            t9 = *(u8*)(v1);
                            v1++;
                        } while (curDst != t6)
                        *(s8*)t3 = t9;
                    }
                    continue; // 134
                }
            }
            break;
        }
        
        // 160
        *(s8*)(arg1 + 2336) = t5;
        if (t3 != end)
        {
            arg1--;
            arg1 = max(arg1, sp);
            t1 -= s1;
            s3 -= s1;
            continue; // 080
        }
        else
        {
            if (arg3 != 0)
                *arg3 = src + 5;
            return 0x80000104;
        }
    }
}

int LoadExecForUser_362A956B()
{
    K1_BACKUP();
    SceUID cbId;
    SceKernelCallbackInfo cbInfo;
    int *argOpt;

    cbId = sceKernelCheckExitCallback();
    cbInfo.size = 56;
    int ret = sceKernelReferCallbackStatus(cbId, &cbInfo);
    if (ret < 0) {
        K1_RESET();
        return ret;
    }
    int *argm8 = cbInfo.common - 8;
    if (!K1_USER_PTR(argm8)) {
        K1_RESET();
        return 0x800200D3;
    }
    int pos  = *(int*)(argm8 + 0);
    int *unk = (int*)*(int*)(argm8 + 4);
    if ((unsigned int)pos >= 4) {
        K1_RESET();
        return 0x800200D2;
    }
    if (!K1_USER_PTR(unk))
        return 0x800200D3;
    K1_RESET();
    if (unk[0] < 12)
        return 0x800201BC;
    unk[1] = 0;
    unk[2] = -1;
    g_unkCbInfo[pos] = unk;
    return 0;
}

void sub_09D8(int *struct1, int *struct2)
{
    if (struct2[1] != 0)
    {
        a2 = struct1[8] + struct1[7] * 28;
        *(a2 + 0) = struct2[2];
        *(a2 + 4) = struct2[1];
        *(a2 + 8) = 256;
        struct1[8] = struct1[7];
        struct1[7]++;
    }

    // a3c
    if (struct2[4] == 0)
    {
        v0 = sceKernelGetChunk(0);
        if (v0 > 0)
        {
            *(sp + 0) = 56;
            sceKernelQueryMemoryBlockInfo(v0, sp);
            t0 = struct1[8] + struct1[7] * 28;
            *(t0 + 0) = *(sp + 40);
            *(t0 + 4) = *(sp + 44);
            *(t0 + 8) = 4;
            struct1[7]++;
        }
    }
    else
    {
        t6 = struct1[8] + struct1[7] * 28;
        *(t6 + 0) = struct2[5];
        *(t6 + 4) = struct2[4];
        *(t6 + 8) = 4;
        struct1[7]++;
    }

    if (struct2[9] == 0)
    {
        // b28
        if (sceKernelGetChunk(4) > 0)
        {
            s2 = struct1[8] + struct1[7] * 28;
            *(s2 + 0) = InitForKernel_D83A9BD7(s2 + 4);
            *(s2 + 8) = 1024;
            struct1[7]++;
        }
    }
    else
    {
        a2 = struct1[8] + struct1[7] * 28;
        *(a2 + 0) = struct2[10];
        *(a2 + 4) = struct2[9];
        *(a2 + 8) = 1024;
        struct1[7]++;
    }

    v0 = sceKernelGetGameInfo();
    if (*(v0 + 4) != 0)
    {
        a3 = struct1[8] + struct1[7] * 28;
        *(a3 + 0) = v0;
        *(a3 + 4) = 220;
        *(a3 + 8) = 32;
        struct1[7]++;
    }
}

void sub_0BBC(int *arg0)
{
    s4 = 0;
    s2 = 0x8B800000;
    if (sceKernelGetModel() == 0)
    {
        // E94
        if (sceKernelDipsw(10) == 1) {
            s2 = 0x8B800000;
            *(0xBC100040) = (*(0xBC100040) & 0xFFFFFFFC) | 2;
        }
        else
            s2 = 0x8A000000;
    }
    // C00 / end of E94
    s1 = 0;
    // C04
    a0 = -1;
    // C08
    do
    {
        v0 = sp + s1;
        s1++;
        *(s8*)v0 = a0;
    } while (s1 < 32);

    t2 = arg0[7];
    s1 = 0;
    if (t2 != 0)
    {
        t1 = arg0[8];
        t0 = t1;

        // C4C
        do
        {
            v1 = *(u16*)(t0 + 8);
            switch (v1)
            {
            case 32:
            // E5C
            case 128:
            // E7C
            case 256:
            case 1024:
            //
            case 64:
            case 4:
            // E4C
            case 8:
            case 1:
            case 2:
                // C7C
                a3 = 0;
                // C80
                a0 = 255;
                // C84
                a2 = sp + a3;
                // C88
                do
                {
                    t9 = *(u8*)a2;
                    v0 = t1 + t9 * 28;
                    int tmpcond = (*v0 & 0x1fffffff) >= (*t0 & 0x1fffffff);
                    if (tmpcond)
                    {
                        // E20
                        a1 = s4;
                        if ((s32)s4 >= (s32)a3)
                        {
                            // E28
                            do
                            {
                                v1 = a1 + sp;
                                v0 = *(u8*)v1;
                                a1--;
                                *(s8*)(v1 + 1) = v0;
                            } while (a1 >= a3);
                        }
                    }
                    if (tmpcond || t9 == a0)
                    {
                        // E40
                        s4++;
                        *(s8*)a2 = s1;
                        break;
                    }
                    a3++;
                    a2 = sp + a3;
                } while (a3 < 32);
                break;
            }

            // CCC
            s1++;
            // CD0
            t0 += 28;
        } while ((u32)s1 < (u32)t2);
    }

    // CDC
    s1 = 0;
    if (s4 > 0)
    {
        s6 = 128;
        t5 = sp + s1;

        // CF8
        do
        {
            s0 = arg0[8] + *(char*)t5 * 28;
            a0 = *(s0 + 8);
            v1 = a0 & 0xffff;
            a1 = *(s0 + 4);
            switch (v1)
            {
            case 32:
            // DF4
            case 128:
            case 256:
            case 4:
            case 0:
            // E10
            case 1024:
            // E10
            case 64:
            // DE4
            case 8:
                // D48
                a0 = a1 + 255;

                // D4C
                a0 &= 0xffffff00;
                s2 -= a0;
                v0 = sceKernelMemmove(s2, *(s0 + 0), *(s0 + 4));
                a0 = *(s0 + 8);
                if ((a0 & 0xffff) == s6) {
                    a0 = *(s0 + 0);
                    // DD0
                    a2 = *(s0 + 4);
                    a1 = 0;
                    v0 = sceKernelMemset(a0);
                    a0 = *(s0 + 8);
                }

                // D74
                *(s0 + 0) = s2;
                v1 = a0 & 0xffff;
                break;
            }

            // D7C
            if (v1 == 8)
            {
                t0 = *(s0 + 0);
                // DC8
                arg0[19] = t0;
            }
            // D84
            s1++;
            t5 = sp + s1;
        } while ((s32)s1 < (s32)s4);
    }
    arg0[18] = s2;
    return v0;
}

// BD2F1094
int sceKernelLoadExec(char *arg0, int arg1)
{
    int ret;
    K1_BACKUP();
    ret = sceKernelLockMutex(g_loadExecMutex, 1, 0);
    if (ret < 0) {
        K1_RESET();
        return ret;
    }
    int oldD384 = g_loadExecCb;
    g_loadExecCb = 0;
    g_valAreSet = g_val0 = g_val1 = g_val2 = 0;
    ret = sceKernelBootFrom();
    if (ret >= 48) // v0 == 48 || v0 >= 49
    {
        g_loadExecCb = oldD384;
        sceKernelUnlockMutex(g_loadExecMutex, 1);
        K1_RESET();
        return 0x80020149;
    }

    if ((ret == 0 && sceKernelIsToolMode() != 0) || ret == 32)
    {
        int var;
        // FB4
        if (sceKernelGetCompiledSdkVersion() != 0 && sceKernelGetAllowReplaceUmd(&var) == 0 && var != 0)
        {
            g_loadExecCb = oldD384;
            sceKernelUnlockMutex(g_loadExecMutex, 1);
            K1_RESET();
            return 0x80020149;
        }
        // FE0
        if (sceKernelIsIntrContext() != 0)
        {
            g_loadExecCb = oldD384;
            sceKernelUnlockMutex(g_loadExecMutex, 1);
            K1_RESET();
            return 0x80020064;
        }
        if (K1_USER_MODE())
        {
            g_loadExecCb = oldD384;
            sceKernelUnlockMutex(g_loadExecMutex, 1);
            K1_RESET();
            return 0x80020149;
        }
        if (arg0 == 0 || !K1_USER_PTR(arg0))
        {
            g_loadExecCb = oldD384;
            sceKernelUnlockMutex(g_loadExecMutex, 1);
            K1_RESET();
            return 0x800200D3;
        }
        if (arg1 != 0)
        {
            if (!K1_PTRSTATICSIZEOK(arg1, 16))
            {
                g_loadExecCb = oldD384;
                sceKernelUnlockMutex(g_loadExecMutex, 1);
                K1_RESET();
                return 0x800200D3;
            }
            int addr12 = *(int*)(arg1 + 12);
            if (addr12 != 0 && !K1_USER_PTR(addr12))
            {
                g_loadExecCb = oldD384;
                sceKernelUnlockMutex(g_loadExecMutex, 1);
                K1_RESET();
                return 0x800200D3;
            }
            int addr8 = *(int*)(arg1 + 8);
            int addr4 = *(int*)(arg1 + 4);
            // 1058
            if (addr8 != 0 && !K1_USER_BUF_DYN_SZ(addr8, addr4))
            {
                g_loadExecCb = oldD384;
                sceKernelUnlockMutex(g_loadExecMutex, 1);
                K1_RESET();
                return 0x800200D3;
            }
        }
        int s5;
        // 107C
        if (sceKernelGetChunk(3) < 0)
        {
            // 1220
            if (sceKernelIsDVDMode() == 0 && strcmp(arg0, g_unencryptedBootPath) == 0) {
                arg0 = *g_encryptedBootPathPtr;
                s5 = 273;
            }
            else
                s5 = 272;
        }
        else
        {
            int v1;
            if (strcmp(arg0, g_unencryptedBootPath) != 0)
            {
                // 120C
                s5 = 276;
                v1 = 274;
            }
            else
            {
                arg0 = *g_encryptedBootPathPtr;
                s5 = 277;
                v1 = 275;
            }
            // 10BC
            if (InitForKernel_EE67E450() != 80)
                s5 = v1;
        }
        // 10C0
        ret = sub_21E0(arg0, 0x208810, 0x208010);
        if (ret < 0)
        {
            g_loadExecCb = oldD384;
            sceKernelUnlockMutex(g_loadExecMutex, 1);
            K1_RESET();
            return ret;
        }
        if (sceKernelIsToolMode() != 0)
        {
            // 11BC
            SceIoStat stat;
            // TODO: verify?
            if (sceIoGetStat(arg0, &stat) >= 0
                && ((stat.st_size >> 32) >= 0 || stat.st_size > 0x1780000)) // 11EC
            {
                g_loadExecCb = oldD384;
                sceKernelUnlockMutex(g_loadExecMutex, 1);
                K1_RESET();
                return 0x80020001;
            }
        }
        RebootArgs2 args2;
        RebootArgs args;

        // 10F0
        args2.size = 48;
        args2.opt4 = 0;
        args2.opt5 = 0;
        args2.opt6 = 0;
        args2.opt7 = 0;
        args2.loadPspbtcnf = 0x10000;
        args2.opt9 = 0;
        args2.opt10 = 0;
        if (arg1 == 0)
        {
            // 11B0
            args2.opt1 = 0;
            args2.opt2 = 0;
        }
        else {
            args2.opt1 = *(int*)(arg1 + 4);
            args2.opt2 = *(int*)(arg1 + 8);
        }
        char *name;
        // 112C
        if ((s5 == 274) || (s5 == 275) || (s5 == 276) || (s5 == 277))
            name = g_umdEmuStr;
        else
            name = g_gameStr;
        // 116C
        args2.typeName = name;
        args2.opt11 = 0;

        args.opt0 = s5;
        args.opt1 = 0;
        args.fileName = arg0;
        args.args2 = &args2;
        args.opt4 = 0;
        args.opt5 = 0;
        args.opt6 = 0;
        args.opt7 = 0;
        ret = runExec(&args);
    }
    else
        ret = 0x80020149;

    g_loadExecCb = oldD384;
    sceKernelUnlockMutex(g_loadExecMutex, 1);
    K1_RESET();
    return ret;
}

int LoadExecForUser_8ADA38D3(char *fileName, int arg1)
{
    RebootArgs args;
    RebootArgs2 args2;
    int *unkPtr;
    int *unkPtr2;
    SceUID fileId;
    int oldD384;

    K1_BACKUP();
    int ret = sceKernelLockMutex(g_loadExecMutex, 1, 0);
    if (ret < 0) {
        K1_RESET();
        return ret;
    }
    oldD384 = g_loadExecCb;
    g_loadExecCb = 0;
    g_valAreSet = 0;
    g_val0 = 0;
    g_val1 = 0;
    g_val2 = 0;

    if (sceKernelIsIntrContext() != 0)
    {
        g_loadExecCb = oldD384;
        sceKernelUnlockMutex(g_loadExecMutex, 1);
        K1_RESET();
        return 0x80020064;
    }
    if (K1_USER_MODE())
    {
        g_loadExecCb = oldD384;
        sceKernelUnlockMutex(g_loadExecMutex, 1);
        K1_RESET();
        return 0x80020149;
    }
    if (fileName == NULL || !K1_USER_PTR(fileName))
    {
        g_loadExecCb = oldD384;
        sceKernelUnlockMutex(g_loadExecMutex, 1);
        K1_RESET();
        return 0x800200D3;
    }

    if (arg1 != 0)
    {
        if (!K1_PTRSTATICSIZEOK(arg1, 16))
        {
            g_loadExecCb = oldD384;
            sceKernelUnlockMutex(g_loadExecMutex, 1);
            K1_RESET();
            return 0x800200D3;
        }
        int addr1 = *(int*)(arg1 + 12);
        if (addr1 != 0 && K1_USER_PTR(addr1))
        {
            g_loadExecCb = oldD384;
            sceKernelUnlockMutex(g_loadExecMutex, 1);
            K1_RESET();
            return 0x800200D3;
        }
        addr1 = *(int*)(arg1 + 8);
        int size = *(int*)(arg1 + 4);
    
        // 135C
        if (addr1 != 0 && !K1_USER_BUF_DYN_SZ(addr1, size))
        {
            g_loadExecCb = oldD384;
            sceKernelUnlockMutex(g_loadExecMutex, 1);
            K1_RESET();
            return 0x800200D3;
        }
    }

    // 1388
    ret = sub_21E0(fileName, 0x208813, 0x208013);
    if (ret < 0)
    {
        g_loadExecCb = oldD384;
        sceKernelUnlockMutex(g_loadExecMutex, 1);
        K1_RESET();
        return ret;
    }

    fileId = sceIoOpen(fileName, 0x4000001, 511);
    if (fileId < 0)
    {
        g_loadExecCb = oldD384;
        sceKernelUnlockMutex(g_loadExecMutex, 1);
        K1_RESET();
        return fileId;
    }
    ret = ModuleMgrForKernel_C3DDABEF(fileId, unkPtr, unkPtr2);
    if (ret < 0)
    {
        sceIoClose(fileId);
        g_loadExecCb = oldD384;
        sceKernelUnlockMutex(g_loadExecMutex, 1);
        K1_RESET();
        return ret;
    }
    args2.size = 48;
    args2.opt4 = 0;
    args2.opt5 = 0;
    args2.opt6 = 0;
    args2.opt7 = 0;
    args2.loadPspbtcnf = 0x10000;
    args2.opt9 = 0;
    args2.opt10 = 0;
    //
    if (arg1 == 0)
    {
        // 1528
        args2.opt1 = 0;
        args2.opt2 = 0;
    }
    else {
        args2.opt1 = *(int*)(arg1 + 4);
        args2.opt2 = *(int*)(arg1 + 8);
    }

    // 1414
    switch (sceKernelInitApitype() - 272)
    {
    case 0:
    case 1:
    case 16:
    case 17:
    case 18:
    case 80:
    case 81:
        args2.typeName = g_gameStr;
        break;

    case 2:
    case 3:
    case 4:
    case 5:
    case 19:
    case 20:
    case 21:
    case 22:
        args2.typeName = g_umdEmuStr;
        break;

    case 6:
    case 8:
        if (sceKernelGetChunk(3) < 0)
            args2.typeName = g_gameStr;
        else
            args2.typeName = g_umdEmuStr;

    case 96:
    case 97:
        args2.typeName = g_mlnAppStr;
        break;

    default:
        if (sceKernelDipsw(13) != 1)
            args2.typeName = g_gameStr;
        else
            args2.typeName = g_umdEmuStr;
        break;
    }

    int type;
    args2.opt11 = 0;
    if (InitForKernel_EE67E450() == 80)
        type = 280;
    else
        type = 278;

    // 1464
    args.opt0 = type;
    args.opt1 = type;
    args.fileName = fileName;
    args.args2 = &args2;
    args.opt4 = 0;
    args.opt5 = unkPtr;
    args.opt6 = unkPtr2[0];
    args.opt7 = unkPtr2[1];
    ret = runExec(&args);
    sceIoClose(fileId);
    g_loadExecCb = oldD384;
    sceKernelUnlockMutex(g_loadExecMutex, 1);
    K1_RESET();
    return ret;
}

int LoadExecForUser_D1FB50DC(int arg)
{
    RebootArgs2 args2;
    RebootArgs args;
    int ret, oldVar;

    K1_BACKUP();
    ret = sceKernelLockMutex(g_loadExecMutex, 1, 0);
    if (ret < 0) {
        K1_RESET();
        return ret;
    }
    
    oldVar = g_loadExecCb;
    g_loadExecCb = 0;
    if (sceKernelIsIntrContext() == 0)
    {   
        if (k1 < 0)
        {   
            args.opt0 = 0x210;
            args.opt1 = 0;
            args.opt2 = 0;
            args.args2 = &args2;
            args.opt4 = arg;
            args.opt5 = 0;
            args.opt6 = 0;
            args.opt7 = 0;
            
            args2.size = 48;
            args2.opt1 = 0;
            args2.opt2 = 0;
            args2.typeName = g_vshStr;
            args2.opt4 = 0;
            args2.opt5 = 0;
            args2.opt6 = 0;
            args2.opt7 = 0;
            args2.opt8 = 1;
            args2.opt9 = 0;
            args2.opt10 = 0;
            args2.opt11 = 0;
            
            ret = sub_20e4(&args);
        }
        else
            ret = 0x80020149;
    }
    else
        ret = 0x80020064;

    g_loadExecCb = oldVar;
    sceKernelUnlockMutex(g_loadExecMutex, 1);
    K1_RESET();
    return ret;
}

// 08F7166C
int sceKernelExitVSHVSH(RebootArgs2 *opt)
{
    RebootArgs2 args2;
    RebootArgs args;
    K1_BACKUP();

    if (sceKernelIsIntrContext() != 0) {
        K1_RESET();
        return 0x80020064;
    }
    if (K1_USER_MODE()) {
        K1_RESET();
        return 0x80020149;
    }

    // 16B0
    if (sceKernelGetUserLevel() != 4) {
        K1_RESET();
        return 0x80020149;
    }
    int ret = sub_2308(opt);
    if (ret < 0) {
        K1_RESET();
        return ret;
    }
    args2.size = 48;
    if (opt == NULL)
    {
        // 17B8
        args2.opt1 = 0;
        args2.opt2 = 0;
        args2.typeName = NULL;
        args2.opt4 = 0;
        args2.opt5 = 0;
        args2.opt6 = 0;
        args2.opt7 = 0;
        args2.loadPspbtcnf = 0x10000;
        args2.opt9 = 0;
        args2.opt10 = 0;
    }
    else
    {
        args2.opt1 = opt->opt1;
        args2.opt2 = opt->opt2;
        args2.typeName = opt->typeName;
        args2.opt4 = opt->opt4;
        args2.opt5 = opt->opt5;
        args2.opt6 = opt->opt6;
        args2.opt7 = opt->opt7;
        args2.loadPspbtcnf = opt->loadPspbtcnf;
        if (opt->size >= 48)
        {
            // 17A4
            args2.opt9 = opt->opt9;
            args2.opt10 = opt->opt10;
        }
        else {
            args2.opt9 = 0;
            args2.opt10 = 0;
        }
    }

    // 1738
    if (args2.typeName == NULL)
        args2.typeName = g_vshStr;

    // 1754
    args2.loadPspbtcnf |= 0x10000;
    args2.opt11 = 0;

    args.opt0 = 544;
    args.opt1 = 0;
    args.fileName = NULL;
    args.args2 = &args2;
    args.opt4 = 0;
    args.opt5 = 0;
    args.opt6 = 0;
    args.opt7 = 0;
    ret = runExec(&args);
    K1_RESET();
    return ret;
}

// 1F88A490
int sceKernelRegisterExitCallback(int arg) // alias: 4AC57943 in ForUser
{
    K1_BACKUP();
    int mtx = sceKernelLockMutex(g_loadExecMutex, 1, 0);
    if (mtx < 0) {
        K1_RESET();
        return mtx;
    }
    if (sceKernelGetThreadmanIdType(arg) == 8)
    {
        // 188C
        g_loadExecCb = arg;
        sceKernelUnlockMutex(g_loadExecMutex, 1);
        if (g_loadExecFunc != NULL && sceKernelGetCompiledSdkVersion() == 0) {
            // 18E0
            g_loadExecFunc();
        }

        // 18B4
        if (arg != 0 && g_loadExecIsInited == 1)
            sceKernelInvokeExitCallback();
        K1_RESET();
        return mtx;
    }
    sceKernelUnlockMutex(g_loadExecMutex, 1);
    if (sceKernelGetCompiledSdkVersion() <= 0x30904FF) {
        K1_RESET();
        return mtx;
    }
    K1_RESET();
    return 0x800200D2;
}

// 1F08547A
int sceKernelInvokeExitCallback()
{
    int ret;
    int var;
    ret = sceKernelLockMutex(g_loadExecMutex, 1, 0);
    if (ret < 0)
        return ret;
    var = g_loadExecCb;
    if (var != 0)
    {
        // 19A4
        if (sceKernelGetThreadmanIdType(var) != 8)
            g_loadExecCb = 0;
        // 19C4
        var = g_loadExecCb;
    }

    // 1944
    if (var != 0)
    {
        // 1984
        sceKernelPowerRebootStart(0);
        ret = sceKernelNotifyCallback(g_loadExecCb, 0);
    }
    else {
        g_loadExecIsInited = 1;
        ret = 0x8002014D;
    }

    // 1960
    sceKernelUnlockMutex(g_loadExecMutex, 1);
    return ret;
}

int LoadExecForKernel_BC26BEEF(RebootArgs2 *opt, int arg1)
{
    char *name;
    if (opt == NULL)
        return 0x80020324;
    name = opt->typeName; // TODO: check if 'opt' is really a pointer
    if (name == NULL)
        return 0x80020324;
    if (arg1 == 0)
    {
        // 1A34
        if ((strcmp(name, "game") == 0)
         || (strcmp(name, "vsh") == 0)
         || (strcmp(name, "updater") == 0))
            return 1;
        return 0;
    }
    if (arg1 != 1)
        return 0x80020324;
    return strcmp(name, "updater") != 0;
}

int LoadExecForKernel_DBD0CF1B(int arg0, int arg1, int arg2)
{
    g_valAreSet = 1;
    g_val0 = arg0;
    g_val1 = arg1;
    g_val2 = arg2;
    return 0;
}

// 2AC9954B
int sceKernelExitGameWithStatus()
{
    return LoadExecForUser_D1FB50DC(0);
}

// 05572A5F
int sceKernelExitGame()
{
    return LoadExecForUser_D1FB50DC(0);
}

// D8320A28
int sceKernelLoadExecVSHDisc(int arg0, int arg1)
{
    return sub_2384(288, arg0, arg1, 0x10000);
}

// D4B49C4B
int sceKernelLoadExecVSHDiscUpdater(int arg0, int arg1)
{
    return sub_2384(289, arg0, arg1, 0x10000);
}

// 1B305B09
int sceKernelLoadExecVSHDiscDebug(int arg0, int arg1)
{
    if (sceKernelIsToolMode() != 0)
        return sub_2384(290, arg0, arg1, 0x10000);
    return 0x80020002;
}

int LoadExecForKernel_F9CFCF2F(int arg0, int arg1)
{
    return sub_2384(291, arg0, arg1, 0x10000);
}

int LoadExecForKernel_077BA314(int arg0, int arg1)
{
    return sub_2384(292, arg0, arg1, 0x10000);
}

int LoadExecForKernel_E704ECC3(int arg0, int arg1)
{
    return sub_2384(293, arg0, arg1, 0x10000);
}

int LoadExecForKernel_47A5A49C(int arg0, int arg1)
{
    return sub_2384(294, arg0, arg1, 0x10000);
}

// BEF585EC
int sceKernelLoadExecBufferVSHUsbWlan(int arg0, int arg1, int arg2)
{
    return sub_2580(304, arg0, arg1, arg2, 0x10000);
}

// 2B8813AF
int sceKernelLoadExecBufferVSHUsbWlanDebug(int arg0, int arg1, int arg2)
{
    if (sceKernelIsToolMode() != 0)
        return sub_2580(305, arg0, arg1, arg2, 0x10000);
    return 0x80020002;
}

int LoadExecForKernel_87C3589C(int arg0, int arg1, int arg2)
{
    return sub_2580(306, arg0, arg1, arg2, 0x10000);
}

int LoadExecForKernel_7CAFE77F(int arg0, int arg1, int arg2)
{
    if (sceKernelIsToolMode != 0)
        return sub_2580(307, arg0, arg1, arg2, 0x10000);
    return 0x80020002;
}

// 4FB44D27
int sceKernelLoadExecVSHMs1(int arg0, int arg1)
{
    return sub_2384(320, arg0, arg1, 0x10000);
}

// D940C83C
int sceKernelLoadExecVSHMs2(int arg0, int arg1)
{
    return sub_2384(321, arg0, arg1, 0x10000);
}

// CC6A47D2
int sceKernelLoadExecVSHMs3(int arg0, int arg1)
{
    return sub_2384(322, arg0, arg1, 0x10000);
}

// 00745486
int sceKernelLoadExecVSHMs4(int arg0, int arg1)
{
    return sub_2384(323, arg0, arg1, 0x10000);
}

// 7CABED9B
int sceKernelLoadExecVSHMs5(int arg0, int arg1)
{
    return sub_2384(324, arg0, arg1, 0x10000);
}

int LoadExecForKernel_A6658F10(int arg0, int arg1)
{
    return sub_2384(325, arg0, arg1, 0x10000);
}

int LoadExecForKernel_16A68007(int arg0, int arg1)
{
    return sub_2384(337, arg0, arg1, 0x10000);
}

int LoadExecForKernel_032A7938(int arg0, int arg1)
{
    return sub_2384(338, arg0, arg1, 0x10000);
}

int LoadExecForKernel_40564748(int arg0, int arg1)
{
    return sub_2384(339, arg0, arg1, 0x10000);
}

int LoadExecForKernel_E1972A24(int arg0, int arg1)
{
    return sub_2384(340, arg0, arg1, 0x10000);
}

int LoadExecForKernel_C7C83B1E(int arg0, int arg1)
{
    return sub_2384(341, arg0, arg1, 0x10000);
}

int LoadExecForKernel_8C4679D3(int arg0, int arg1)
{
    return sub_2384(342, arg0, arg1, 0x10000);
}

int LoadExecForKernel_B343FDAB(int arg0, int arg1)
{
    return sub_2384(352, arg0, arg1, 0x10000);
}

int LoadExecForKernel_1B8AB02E(int arg0, int arg1)
{
    return sub_2384(353, arg0, arg1, 0x10000);
}

int LoadExecForKernel_C11E6DF1(int arg0, int arg1)
{
    return sub_2384(368, arg0, arg1, 0x10000);
}

int LoadExecForKernel_9BD32619(int arg0, int arg1)
{
    return sub_2384(369, arg0, arg1, 0x10000);
}

// C3474C2A
int sceKernelExitVSHKernel(RebootArgs2 *arg)
{
    return sub_26B0(512, arg);
}

int LoadExecForKernel_C540E3B3()
{
    return 0;
}

// 24114598
int sceKernelUnregisterExitCallback()
{
    int ret = sceKernelLockMutex(g_loadExecMutex, 1, 0);
    if (ret < 0)
        return ret;
    g_loadExecCb = 0;
    sceKernelUnlockMutex(g_loadExecMutex, 1);
    return ret;
}

// B57D0DEC
SceUID sceKernelCheckExitCallback()
{
    return g_loadExecCb;
}

int LoadExecForKernel_A5ECA6E3(void (*arg)())
{
    g_loadExecFunc = arg;
    return 0;
}

int module_bootstart()
{
    g_loadExecCb = 0;
    g_loadExecMutex = sceKernelCreateMutex("SceLoadExecMutex", 0x101, 0, 0);
    g_loadExecIsInited = 0;
    g_loadExecFunc = NULL;
    sceKernelSetRebootKernel(func_282C);
    return 0;
}

int runExec(RebootArgs *args) // 20FC
{
    int sp[8];
    if (args->opt0 ^ 0x300 != 0) /* Run in a thread */
    {
        int ret, threadEnd;
        SceUID id;
        sp[0] = 8;
        sp[1] = 1;
        ret = sceKernelGetModel();
        if (ret != 0 && sceKernelApplicationType() != 0x100)
            sp[1] = 8; // not in VSH
        
        id = sceKernelCreateThread("SceKernelLoadExecThread", runExecFromThread, 32, 0x8000, 0, sp /* options */);
        if (id < 0)
            return id;
        sceKernelStartThread(id, 40, args);
        threadEnd = sceKernelWaitThreadEnd(id, 0);
        sceKernelExitThread(0);
        return threadEnd;
    }
    return runExecFromThread(40, args);
}

int sub_21E0(char *name, int devcmd, int iocmd)
{
    if (strchr(name, '%') != NULL)
        return 0x8002014C;
    char *comma = strchr(name, ':');
    if (pos != 0)
    {
        int pos = comma - name;
        char string[32];
        if (pos >= 31)
            return 0x80020001;
        strncpy(string, name, pos + 1);
        string[pos + 1] = '\0';
        int ret = sceIoDevctl(string, devcmd, 0, 0, 0, 0);
        // 2260
        if (ret == 0)
            return 0;
    }

    // 226C
    SceUID fileId = sceIoOpen(name, 1, 0x1ff);
    if (fileId < 0)
        return 0;
    int ret = sceIoIoctl(fileId, iocmd, 0, 0, 0, 0);
    sceIoClose(fileId);
    if (ret < 0)
        return 0x80020147;
    return 0;
}

int sub_2308(RebootArgs2 *opt)
{
    if (opt != NULL)
    {
        if (!K1_PTRSTATICSIZEOK(arg, 16))
            return 0x800200D3;
        if (opt->typeName != NULL && !K1_USER_PTR(opt->typeName))
            return 0x800200D3;
        // 2328
        if (opt->opt6 != NULL && !K1_USER_PTR(opt->opt6))
            return 0x800200D3;
        // 2344
        if (opt->opt7 != NULL && !K1_USER_PTR(opt->opt7))
            return 0x800200D3;
    }
    return 0;
}

int sub_2384(int arg0, int arg1, int arg2, int arg3)
{
    K1_BACKUP();
    if (sceKernelIsIntrContext() == 0)
    {
        if (k1 < 0)
        {
            int iocmd, devcmd;
            // 23EC
            if (sceKernelGetUserLevel() != 4) {
                K1_RESET();
                return 0x80020149;
            }
            if (arg1 == 0) {
                K1_RESET();
                return 0x800200D3;
            }
            if (!K1_USER_PTR(arg1)) {
                K1_RESET();
                return 0x800200D3;
            }
            if (sub_2308(arg2) < 0) {
                K1_RESET();
                return 0x800200D3;
            }
            switch (arg0 - 288)
            {
            case 0:
            case 1:
            case 2:
                devcmd = 0x208811;
                iocmd  = 0x208011;
                break;

            case 3:
            case 4:
            case 5:
            case 6:
            case 80:
            case 81:
                devcmd = 0x208814;
                iocmd  = 0x208014;
                break;

            case 32:
            case 33:
            case 35:
            case 36:
            case 37:
            case 49:
            case 50:
            case 52:
            case 53:
            case 54:
            case 64:
            case 65:
                devcmd = 0x208813;
                iocmd  = 0x208013;
                break;

            default:
                K1_RESET();
                return 0x80020001;
            }

            int ret = sub_21E0(arg1, devcmd, iocmd);
            if (ret < 0) {
                K1_RESET();
                return ret;
            }
            if (arg0 == 290)
            {
                // 24D0
                if (sceKernelIsToolMode() != 0)
                {
                    SceIoStat stat;
                    if (sceIoGetStat(arg1, &stat) >= 0)
                    {
                        if (stat.st_ctime <= 0)
                        {
                            // 2514
                            if (stat.st_ctime == 0 && stat.st_size > 0x1780000) {
                                K1_RESET();
                                return 0x80020001;
                            }
                        }
                        else {
                            K1_RESET();
                            return 0x80020001;
                        }
                    }
                }
            }
            RebootArgs2 args2;
            RebootArgs args;
        
            // 2488
            sub_29A4(&args2, arg3, arg2);
            args.opt0 = arg0;
            args.opt1 = 0;
            args.opt2 = arg1;
            args.args2 = &args2;
            args.opt4 = 0;
            args.opt5 = 0;
            args.opt6 = 0;
            args.opt7 = 0;
            ret = runExec(&args);
            K1_RESET();
            return ret;
        }
        else {
            K1_RESET();
            return 0x80020149;
        }
    }
    else {
        K1_RESET();
        return 0x80020064;
    }
}

int sub_2580(int arg0, int arg1, int arg2, int arg3, int arg4)
{
    K1_BACKUP();
    if (sceKernelIsIntrContext() != 0) {
        K1_RESET();
        return 0x80020064;
    }

    if (K1_USER_MODE())
    {
        int ret;
        RebootArgs args;
        RebootArgs2 args2;
        // 25F4
        if (sceKernelGetUserLevel() != 4) {
            K1_RESET();
            return 0x80020149;
        }
        if (arg1 == 0) {
            K1_RESET();
            return 0x8002014B;
        }
        if (arg2 != 0 && !K1_USER_BUF_DYN_SZ(arg1, arg2)) {
            K1_RESET();
            return 0x800200D3;
        }

        // 263C
        ret = sub_2308(arg3);
        if (ret < 0) {
            K1_RESET();
            return ret;
        }
        sub_29A4(&args2, arg4, arg3);
        args.opt0 = arg0;
        args.opt1 = arg1;
        args.opt2 = arg2;
        args.args2 = &args2;
        args.opt4 = 0;
        args.opt5 = 0;
        args.opt6 = 0;
        args.opt7 = 0;
        ret = runExec(&args);
        K1_RESET();
        return ret;
    }
    else {
        K1_RESET();
        return 0x80020149;
    }
}

int sub_26B0(int arg0, RebootArgs2 *opt)
{
    RebootArgs2 args2;
    RebootArgs args;
    int ret;
    K1_BACKUP();
   
    if (arg0 ^ 0x300 != 0 && sceKernelIsIntrContext() != 0) { // 2810
        K1_RESET();
        return 0x80020064;
    }

    // 26C4
    if (K1_USER_MODE()) {
        K1_RESET();
        return 0x80020149;
    }
    ret = sub_2308(arg1);
    if (ret < 0) {
        K1_RESET();
        return ret;
    }
    args2.size = 48;
    if (arg1 == NULL)
    {
        // 27E0
        args2.opt1 = 0;
        args2.opt2 = 0;
        args2.typeName = NULL;
        args2.opt4 = 0;
        args2.opt5 = 0;
        args2.opt6 = 0;
        args2.opt7 = 0;
        args2.loadPspbtcnf = 0x10000;
        args2.opt9 = 0;
        args2.opt10 = 0;
    }
    else
    {
        args2.opt1 = opt->opt1;
        args2.opt2 = opt->opt2;
        args2.typeName = opt->typeName;
        args2.opt4 = opt->opt4;
        args2.opt5 = opt->opt5;
        args2.opt6 = opt->opt6;
        args2.opt7 = opt->opt7;
        args2.loadPspbtcnf = opt->loadPspbtcnf;
        if (opt->size >= 48)
        {
            // 27D4
            args2.opt9 = opt->opt9;
            args2.opt10 = opt->opt10;
        }
        else {
            args2.opt9 = 0;
            args2.opt10 = 0;
        }
    }

    // 2750
    if (args2.typeName == NULL)
        args2.typeName = g_vshStr;

    // 276C
    args.opt0 = arg0;
    args.opt1 = 0;
    args.opt2 = 0;
    args.args2 = &args2;
    args.opt4 = 0;
    args.opt5 = 0;
    args.opt6 = 0;
    args.opt7 = 0;

    args2.loadPspbtcnf |= 0x10000;
    args2.opt11 = 0;
    ret = runExec(&args);
    K1_RESET();
    return ret;
}

int func_282C(RebootArgs2 *arg)
{
    return sub_26B0(768, arg);
}

int runExecFromThread(int unk, RebootArgs *opt) // 2864
{
    int sp[216];
    char str1[256], str2[256], str3[256];
    char *ptr;
    if (opt == NULL)
        return 0x80020001;
    sp[0] = 32;
    sp[2] = opt->opt6;
    sp[3] = opt->opt7;

    if (opt->opt5) {
        memcpy(&sp[4], opt->opt5, 16);
        memset(opt->opt5, 0, 16);
    }

    opt->opt8 = sp;

    memcpy(&sp[8], opt->args2, 48);
    opt->opt4 = s0;

    ptr = sp[11];
    if (ptr != NULL) {
        strncpy(str1, ptr, 256);
        sp[11] = str1;
    }
    else if (sp[14] == 0)
        return 0x80020001;

    ptr = sp[14];
    if (ptr != NULL) {
        strncpy(str2, ptr, 256);
        *(sp[14]) = str2;
    }

    ptr = sp[15];
    if (ptr != 0) {
        strncpy(str3, ptr, 256);
        *(sp[15]) = str3;
    }

    sceKernelSetSystemStatus(0x40000);
    if (opt->opt0 != 0x300)
    {
        ret = sceKernelPowerRebootStart(0);
        if (ret < 0)
            return ret;
    }
    return sub_2A64(opt);
}

void sub_29A4(int *dst, int arg1, int *src)
{
    dst[0] = 48;
    dst[8] = arg1;
    if (src == NULL)
    {
        // 2A24
        dst[1] = 0;
        dst[2] = 0;
        dst[3] = 0;
        dst[4] = 0;
        dst[5] = 0;
        dst[6] = 0;
        dst[7] = 0;
        dst[9] = 0;
        dst[10] = 0;
    }
    else
    {
        dst[1] = src[1];
        dst[2] = src[2];
        dst[3] = src[3];
        dst[4] = src[4];
        dst[5] = src[5];
        dst[6] = src[6];
        dst[7] = src[7];
        if (src[0] >= 48)
        {
            // 2A10
            dst[9] = src[9];
            dst[10] = src[10];
        }
        else {
            dst[9] = 0;
            dst[10] = 0;
        }
    }

    // 29F0
    if (dst[3] == 0)
        dst[3] = g_gameStr;

    // 2A08
    dst[11] = 0;
    return;
}

void sub_2A64(RebootArgs *opt)
{
    int type = 0;
    switch (opt->opt0)
    {
    case 1056:

    case 368:

    case 369:

    case 340:
    case 341:
    case 342:

    case 352:

    case 353:

    case 337:
    case 338:

    case 323:
    case 324:
    case 325:

    case 320:
    case 321:

    case 304:
    case 305:
    case 306:
    case 307:

    case 288:
    case 289:

    case 290:
    case 291:
    case 292:
    case 293:
    case 294:

    case 280:

    case 256:

    case 272:
    case 273:
    case 274:
    case 275:
    case 276:
    case 277:
    case 278:
        // (2AE4)
        type = 2;
        break;
    case 544:
    case 768:
    case 512:
    case 528:
        // 32CC
        type = 1;
        break;
    default:
        return 0x80020001;
    }

    // 2AE8
    s1 = opt->opt0 & 0x300;
    fp = sceKernelGetInitialRandomValue();
    if (s1 != 0)
    {
        // 3218
        v0 = sceKernelRebootBeforeForUser(opt->args2);
        if (v0 < 0)
            return v0;
    // 2AF8
        // 31F8
        v0 = sceKernelRebootPhaseForKernel(3, opt->args2, 0, 0);
        if (v0 < 0)
            return v0;
    // 2B00
        // 31D8
        v0 = sceKernelRebootPhaseForKernel(2, opt->args2, 0, 0);
        if (v0 < 0)
            return v0;
    }
    // 2B0C
    if (opt->opt0 != 0x300) {
        // 31C8
        sceKernelSuspendAllUserThreads();
    }

    // 2B1C
    sceKernelSetSystemStatus(0x40020)
    if (type == 1)
    {
        // 3194
        if (opt->args2->opt4 == 0)
        {
            v0 = sceKernelGetChunk(0);
            if (v0 > 0)
            {
                *(sp + 0) = 56;
                sceKernelQueryMemoryBlockInfo(v0, sp);
                t3 = *(sp + 40);
                opt->args2->opt5 = t3;
            }
        }
    }

    // 2B30
    if (s1 != 0)
    {
        // 3174
        v0 = sceKernelRebootPhaseForKernel(1, opt->args2, 0, 0);
        if (v0 < 0)
            return v0;
    // 2B38
        // 315C
        sceKernelRebootBeforeForKernel(opt->args2, 0, 0, 0);
    }

    // 2B40
    v0 = LoadCoreForKernel_F871EA38(EXEC_START, EXEC_END - EXEC_START);
    if (v0 != 0)
    {
        sceKernelCpuSuspendIntr();
        *(0xBC100040) = (*(0xBC100040) & 0xFFFFFFFC) | 1;
        v0 = sceKernelMemset(0x88600000, 0, 0x200000);
        v0 = sceKernelMemset(EXEC_START, 0, EXEC_END);
        memset(0xBFC00000, 0, 4096);
        for (;;)
            ;
    }

    // 2BC4
    mtc0(mfc0(Status) & 0xFFF7FFE0, Status);
    // Skipped useless part

    // 2C58
    v0 = sub_32FC();
    s2 = v0;
    *(v0 + 24) |= type;
    if (type == 2)
    {
        // 2F4C
        if (opt->opt1 != 0)
        {  
            t0 = *(v0 + 32);
            // 3134
            *(t0 + 0) = opt->opt2;
            *(t0 + 4) = opt->opt1;
            *(t0 + 8) = 1;
            *(s2 + 28) = 1;
            *(s2 + 36) = 0;
            // 2FFC
            sub_09D8(v0, opt->args2);
        }
        else if (opt->opt2 != 0)
        {
            if ((opt->opt0 == 291) || (opt->opt0 == 293)
             || (opt->opt0 == 292) || (opt->opt0 == 294)
             || (opt->opt0 == 368) || (opt->opt0 == 369))
            {  
                // 2FAC
                s1 = *(s2 + 32);
           
                // 2FB0
                a0 = opt->args2->opt2;
                s0 = s1 + 28;
                *(s1 + 0) = a0;
                *(s1 + 4) = strlen(a0) + 1;
                *(s1 + 8) = 2;
                *(s0 + 0) = opt->opt2;
                *(s0 + 4) = strlen(opt->opt2) + 1;
                *(s0 + 8) = 64;
                *(s2 + 28) = 2;
                *(s2 + 36) = 0;
            }
            else
            {
                s1 = *(v0 + 32);
                // 300C
                *(s1 + 0) = opt->opt2;
                *(s1 + 4) = strlen(opt->opt2) + 1;
                *(s1 + 8) = type;
                *(s2 + 28) = 1;
                *(s2 + 36) = 0;
                if ((opt->opt0 == 274) || (opt->opt0 == 275) || (opt->opt0 == 276) || (opt->opt0 == 277))
                {  
                    // 3064
                    v0 = sceKernelGetChunk(3);
                    s4 = v0;
                    if (v0 >= 0)
                    {  
                        v0 = sceKernelGetBlockHeadAddr(v0);
                        s1 += 28;
                        *(s1 + 0) = v0;
                        *(s1 + 4) = strlen(v0) + 1;
                        *(s1 + 8) = 64;
                        *(s2 + 28)++;
                    }
                }
                // 30AC
                if ((opt->opt0 == 278) || (opt->opt0 == 280))
                {  
                    s1 += 28;
                    *(s1 + 0) = opt->opt8;
                    *(s1 + 4) = 32;
                    *(s1 + 8) = 128;
                    *(s2 + 28)++;
                    v0 = sceKernelGetChunk(3);
                    s4 = v0;
                    if (v0 >= 0)
                    {  
                        v0 = sceKernelGetBlockHeadAddr(v0);
                        s6 = s1 + 28;
                        *(s6 + 0) = v0;
                        *(s6 + 4) = strlen(v0) + 1;
                        *(s6 + 8) = 64;
                        *(s2 + 28)++;
                    }
                }
            }
            // 2FF4
            // 2FF8
            // 2FFC
            sub_09D8(s2, opt->args2);
        }
    }
    else if (type == 1)
    {
        a0 = opt->args2->opt4;
        // 2E38
        s0 = 0;
        if (a0 == 0)
        {  
            // 2ED4
            v0 = sceKernelGetChunk(0);
            if (v0 > 0)
            {  
                *sp = 56;
                sceKernelQueryMemoryBlockInfo(v0, sp);
                s6 = *(s2 + 32) + *(s2 + 28) * 28;
                *(s6 + 0) = *(sp + 40);
                *(s6 + 8) = 256;
                *(s6 + 4) = *(sp + 44);
                *(s2 + 40) = *(s2 + 28);
                *(s2 + 28)++;
                t7 = *(s6 + 4);
                s0 = *(s6 + 0);
                *(s0 + 0) = t7;
                *(s0 + 4) = 32;
            }
        }
        else
        {  
            t4 = opt->args2->opt5;
            t1 = *(s2 + 32) + *(v0 + 28) * 28;
            *(t1 + 0) = t4;
            *(t1 + 8) = 256;
            s0 = t4;
            *(t1 + 4) = opt->args2->opt4;
            *(s2 + 40) = *(s2 + 28);
            *(s2 + 28)++;
            *(t4 + 4) = 32;
            *(t4 + 0) = opt->args2->opt4;
        }

        // 2E94
        t4 = opt->opt4;
        *(s0 + 8) = t4;
        if (t4 == 0)
        {  
            // 2EB4
            v0 = *(s0 + 24);
            if (v0 == 0)
            {  
                v0 = *(s0 + 20);
                if (v0 == 0)
                    v0 = *(s0 + 16);
            }
            // 2ECC
            *(s0 + 12) = v0;
        }
        else
            *(s0 + 12) = 0;
   
        // 2EA4
        *(s0 + 24) = 0;
        *(s0 + 16) = 0;
        *(s0 + 20) = 0;
    }

    // 2C84
    if (g_valAreSet != 0)
    {
        t7 = *(s2 + 32) + *(s2 + 28) * 28;
        *(t7 + 0) = g_val0;
        *(t7 + 4) = g_val1;
        *(t7 + 8) = g_val2;
        *(s2 + 28)++;
    }

    // 2CE0
    if (opt->args2->opt8 & 0x10000 == 0) {
        t0 = *(s2 + 16) + (*(s2 + 12) << 5);
        *(t0 - 12) = opt->arg0;
    }

    // 2D04
    sub_0BBC(s2, opt->args2);
    sceKernelSetDdrMemoryProtection(0x88400000, 0x400000, 12);
    if (opt->opt0 == 0x420)
        return s4;
    sceKernelMemset(0x88600000, 0, 0x200000);
    v0 = decodeKL4E(0x88600000, 0x200000, EXEC_START + 4, 0);
    if (v0 < 0)
    {
        // 2DD0
        sceKernelCpuSuspendIntr();
        *(0xBC100040) = (*(0xBC100040) & 0xFFFFFFFC) | 1;
        sceKernelMemset(0x88600000, 0, 0x200000);
        sceKernelMemset(EXEC_START, 0, EXEC_END - EXEC_START);
        memset(0xBFC00000, 0, 4096);
        for (;;)
            ;
    }
    sceKernelDcacheWritebackAll();
    sceKernelIcacheInvalidateAll();
    UtilsForKernel_39FFB756(0);
    Kprintf("***** reboot start *****\n");
    Kprintf("\n\n\n");
    a0 = s2;
    a1 = arg_3;
    a2 = arg_0;
    a3 = fp;
    jump(0x88600000);
}

void sub_32FC()
{
    int addr1, addr2, addr3, addr4, addr5;
    if (sceKernelGetModel() != 0)
        addr1 = 0x8B800000;
    else
        addr1 = 0x88400000;

    addr2 = (addr1 + 0x0004f) & 0xffffffc0;
    addr3 = (addr2 + 0x0103f) & 0xffffffc0;
    addr4 = (addr3 + 0x1c03f) & 0xffffffc0;
    addr5 = (addr4 + 0x003bf) & 0xffffffc0;

    sceKernelMemset(addr1, 0, addr5 - addr1);

    *(addr2 + 16) = addr3;
    *(addr2 + 32) = addr4;
    *(addr2 + 20) = addr5;
    *(addr2 + 28) = 0;
    *(addr2 + 24) = 0;
    *(addr2 + 44) = sceKernelGetModel();
    *(addr2 + 60) = 0;
    *(addr2 + 64) = sceKernelDipswLow32();
    *(addr2 + 68) = sceKernelDipswHigh32();
    *(addr2 + 48) = 0;

    if (sceKernelDipsw(30) != 1)
        *(addr2 + 68) &= 0xFAFFFFFF;

    *(addr2 + 0) = 0x88000000;
    *(addr2 + 4) = SysMemForKernel_6D9E2DD6();
    *(addr2 + 40) = -1;
    *(addr2 + 36) = -1;
    *(addr2 + 80) = KDebugForKernel_568DCD25();
    return addr2;
}

