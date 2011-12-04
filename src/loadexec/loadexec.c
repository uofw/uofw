#include "../global.h"

/** Structure for LoadExecVSH* functions */
typedef struct
{
    /** Size of the structure in bytes */
    SceSize size;
    /** Size of the arguments string */
    SceSize args;
    /** Pointer to the arguments strings */
    void *argp;
    /** The key, usually "game", "updater" or "vsh" */
    const char *key;
    /** The size of the vshmain arguments */
    u32 vshmainArgs;
    /** vshmain arguments that will be passed to vshmain after the program has exited */
    void *vshmainArgp;
    /** "/kd/pspbtcnf_game.txt" or "/kd/pspbtcnf.txt" if not supplied (max. 256 chars) */
    char *configFile;
    /** An unknown string (max. 256 chars) probably used in 2nd stage of loadexec */
    u32 unk4;
    /** unknown flag default value = 0x10000 */
    u32 flags;
    u32 unkArgs;
    void *unkArgp;
    u32 opt11;
} SceKernelLoadExecVSHParam;

typedef struct
{
    int opt0;
    int opt1;
    char *fileName;
    SceKernelLoadExecVSHParam *args2;
    int opt4;
    void *opt5;
    int opt6;
    int opt7;
    int opt8;
} RebootArgs; 

typedef struct
{
    void *argp; // 0
    int args; // 4
    int unk8, unk12, unk16, unk20, unk24;
} SceKernelArgsStor;

typedef struct
{
    void *addr; // 0
    int unk4, unk8, unk12;
    void *addr1; // 16
    void *addr2; // 20
    int unk24;
    int curArgs; // 28
    SceKernelArgsStor *args; // 32
    int unk36, unk40;
    int model; // 44
    int unk48, unk52, unk56, unk60;
    int dipswLo; // 64
    int dipswHi; // 68
    int unk72, unk76, unk80;
    // ... ? size is max 0x1000
} SceKernelUnkStruct;

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
    char buf[2656];
    char *curBuf = buf;
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
    at = 0x80 - (((s0 & 0x18) >> 2) << 4);
    at = (at & 0xffff00ff) | ((at << 8) & 0xff00);
    s0 = s0 & 7;
    at = (at & 0x0000ffff) | ((at << 16) & 0xffff0000);

    // 06C
    int i;
    for (i = 0; i < 664; i += 2) {
        *((int*)curBuf)[i + 0] = at;
        *((int*)curBuf)[i + 1] = at;
    }

    // 080
    for (;;)
    {
        t9 = (t9 & 0xfffff8ff) | ((curDst << 8) & 0x700);
        v1 = buf + ((t9 >> (s0 & 0x1F)) & 7) * 255;
        t9 = 1;
    
        // 09C
        do
        {
            t7 = v1 + t9;
            t6 = *(u8*)(t7 - 1);
            if ((s3 >> 24) == 0)
            {
                // 0EC
                t1 <<= 8;
                t1 += *(u8*)(src + 5);
                src++;
                t9 <<= 1;
                lo = s3 * t6;
                s3 <<= 8;
            }
            else
            {
                t9 <<= 1;
                lo = (s3 >> 8) * t6;
            }
            t6 -= (t6 >> 3);
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
                *(s8*)(t7 - 1) = t6 + 31;
                t9++;
            }
        } while (s2 == 0);
        // 0D0 == 120
        *(s8*)(curDst + 0) = t9;
    
        // 134
        for (;;)
        {
            t5 = curBuf[2336];
            if ((s3 >> 24) == 0)
            {
                // 17C
                t1 <<= 8;
                t1 += *(u8*)(src + 5);
                src++;
                lo = s3 * t5;
                s3 <<= 8;
            }
            else
                lo = (s3 >> 8) * t5;
            curDst++;
        
            s1 = lo;
            t5 -= (t5 >> 4);
            if (t1 < s1)
            {
                // 1AC
                s3 = t5 + 15;
                curBuf[2336] = s3;
                t8 = -1;
                s3 = lo;
                // 1BC
                int goto_3a4 = 0;
                for (;;)
                {  
                    t5 = curBuf[2344];
                    if ((s1 >> 24) == 0)
                    {  
                        // 750
                        t1 <<= 8;
                        t1 += *(u8*)(src + 5);
                        lo = s3 * t5;
                        s3 = s1 << 8;
                        src++;
                    }
                    else
                        lo = (s3 >> 8) * t5;
            
                    curBuf += 8;
                    t5 -= (t5 >> 4);
                    s1 = lo;
                    v1 = t5 + 15;
                    if (t1 < s1)
                    {
                        // 784
                        t8++;
                        s3 = lo;
                        curBuf[2336] = v1;
                        if (t8 == 6) {
                            t5 = buf + t8;
                            break;
                        }
                        // CONTINUE
                    }
                    else
                    {
                        // 1EC
                        s3 -= s1;
                        t1 -= s1;
                        curBuf[2336] = t5;
                        t5 = buf + t8;
                        if (t8 < 0) {
                            // 3A0
                            curBuf = buf + 7;
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
                    s1 = (s1 & 0xfffffff8) | ((int)curBuf & 3);
                    s1 += buf;
                    t6 = 1;
                    if (t9 >= 0)
                    {
                        s2 = *(u8*)(s1 + 2424)
                        v1 = s3 >> 24;
                        v0 = s3 >> 8;
                        if (v1 == 0)
                        {  
                            // 858
                            t1 <<= 8;
                            t1 += *(u8*)(src + 5);
                            src++;
                            lo = s3 * s2;
                            s3 <<= 8;
                        }
                        else
                            lo = v0 * s2;
                       
                        // 234
                        s2 -= (s2 >> 3);
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
            
                            if (v1 == 0)
                            {
                                // 8C4
                                v1 = *(u8*)(src + 5);
                                t1 <<= 8;
                                src++;
                                t1 += v1;
                                s3 <<= 8;
                            }
            
                            // 264
                            lo = (s3 >> 8) * s2;
                            s2 -= (s2 >> 3);
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
                                t6 <<= 1;
                               
                                if ((s3 >> 24) == 0)
                                {  
                                    // 890
                                    t1 <<= 8;
                                    t1 += *(u8*)(src + 5);
                                    src++;
                                    s3 <<= 7;
                                }
                                else
                                    s3 >>= 1;
                                // 29C
                                for (;;)
                                {  
                                    t9--;
                                    if (t1 >= s3)
                                        t1 -= s3;
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
                    curBuf = buf + 7;
                    s2 = *(u8*)(s1 + 2400);
                    if ((s3 >> 24) == 0)
                    {  
                        // 80C
                        t1 <<= 8;
                        t1 += *(u8*)(src + 5);
                        src++;
                        lo = s3 * s2;
                        s3 <<= 8;
                    }   
                    else
                        lo = (s3 >> 8) * s2;
            
                    s2 -= (s2 >> 3);
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
                                t1 <<= 8;
                                t1 += *(u8*)(src + 5);
                                src++;
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
                        s1 <<= 1;
                        lo = (s3 >> 8) * t8;
                        t7 = t8 - (t8 >> 3);
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
                            s1 = buf + t8;
                            t8 = (s32)t8 >> 3;
                            t5 = *(s1 + 2216);
                            t9 = t8 - 3;
                            t7 = 1;
                            if (t9 >= 0)
                            {  
                                s2 = (t5 & 0xff000000) >> 24;
                                if ((s3 >> 24) == 0)
                                {  
                                    // 694
                                    v1 = *(u8*)(src + 5);
                                    t1 <<= 8;
                                    src++;
                                    t1 += v1;
                                    lo = s3 * s2;
                                    s3 <<= 8;
                                }
                                else
                                    lo = (s3 >> 8) * s2;
                
                                // 420
                                s2 -= (s2 >> 3);
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
                                    if (s3 >> 24)
                                    {
                                        // 700
                                        t1 <<= 8;
                                        t1 += *(u8*)(src + 5);
                                        src++;
                                        s3 <<= 8;
                                    }
                
                                    // 450
                                    lo = (s3 >> 8) * s2;
                                    s2 -= (s2 >> 3);
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
                                            t1 <<= 8;
                                            t1 += *(u8*)(src + 5);
                                            src++;
                                            s3 <<= 7;
                                        }
                                        else // 484
                                            s3 >>= 1;
                                        // 488
                                        for (;;)
                                        {
                                            t9--;
                                            if (t1 >= s3)
                                                t1 -= s3;
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
                            s2 = t5 & 0xff;
                            if ((s3 >> 24) == 0)
                            {
                                // 678
                                v1 = *(u8*)(src + 5);
                                t1 <<= 8;
                                src++;
                                t1 += v1;
                                lo = s3 * s2;
                                s3 <<= 8;
                            }
                            else
                                lo = (s3 >> 8) * s2;
                   
                            // 4BC
                            s2 -= (s2 >> 3);
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
                                s2 = (t5 & 0xff00) >> 8;
                                if ((s3 >> 24) == 0)
                                {
                                    // 65C
                                    t1 <<= 8;
                                    t1 += *(u8*)(src + 5);
                                    src++;
                                    lo = s3 * s2;
                                    s3 <<= 8;
                                }
                                else
                                    lo = (s3 >> 8) * s2;
                
                                // 4F8
                                s2 -= (s2 >> 3);
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
                                    s2 = (t5 & 0x00ff0000) >> 16;
                                    if ((s3 >> 24) == 0)
                                    {  
                                        // 600
                                        v1 = *(u8*)(src + 5);
                                        t1 <<= 8;
                                        src++;
                                        t1 += v1;
                                        lo = s3 * s2;
                                        s3 <<= 8;
                                    }
                                    else
                                        lo = (s3 >> 8) * s2;
                                    
                                    // 534
                                    s2 -= (s2 >> 3);
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
                    curBuf = (char*)(int)((int)curBuf & 0xfffffffe) | (t6 & 1);
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
        curBuf[2336] = t5;
        if (t3 != end)
        {
            curBuf--;
            curBuf = MAX((int)curBuf, (int)buf);
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

void sub_09D8(SceKernelUnkStruct *hwOpt, SceKernelLoadExecVSHParam *param)
{
    if (param->args != 0)
    {
        hwOpt->args[hwOpt->curArgs].argp = param->argp;
        hwOpt->args[hwOpt->curArgs].args = param->args;
        hwOpt->args[hwOpt->curArgs].unk8 = 256;
        hwOpt->unk40 = hwOpt->curArgs;
        hwOpt->curArgs++;
    }

    // a3c
    if (param->vshmainArgs == 0)
    {
        SceUID blkId = sceKernelGetChunk(0);
        if (blkId > 0)
        {
            SceKernelSysmemBlockInfo info;
            info.size = 56;
            sceKernelQueryMemoryBlockInfo(blkId, &info);
            hwOpt->args[hwOpt->curArgs].argp = info.unk40;
            hwOpt->args[hwOpt->curArgs].args = info.unk44;
            hwOpt->args[hwOpt->curArgs].unk8 = 4;
            hwOpt->curArgs++;
        }
    }
    else
    {
        hwOpt->args[hwOpt->curArgs].argp = param->vshmainArgp;
        hwOpt->args[hwOpt->curArgs].args = param->vshmainArgs;
        hwOpt->args[hwOpt->curArgs].unk8 = 4;
        hwOpt->curArgs++;
    }

    if (param->unkArgs == 0)
    {
        // b28
        if (sceKernelGetChunk(4) > 0)
        {
            hwOpt->args[hwOpt->curArgs].argp = InitForKernel_D83A9BD7(&hwOpt->args[hwOpt->curArgs].args);
            hwOpt->args[hwOpt->curArgs].unk8 = 1024;
            hwOpt->curArgs++;
        }
    }
    else
    {
        hwOpt->args[hwOpt->curArgs].argp = param->unkArgp;
        hwOpt->args[hwOpt->curArgs].args = param->unkArgs;
        hwOpt->args[hwOpt->curArgs].unk8 = 1024;
        hwOpt->curArgs++;
    }

    SceKernelGameInfo info = sceKernelGetGameInfo();
    if (info.unk4 != 0)
    {
        hwOpt->args[hwOpt->curArgs].argp = &info;
        hwOpt->args[hwOpt->curArgs].args = 220;
        hwOpt->args[hwOpt->curArgs].unk8 = 32;
        hwOpt->curArgs++;
    }
}

void sub_0BBC(SceKernelUnkStruct *hwOpt)
{
    char sp[32];
    int cur = 0;
    void *addr = (void*)0x8B800000;
    if (sceKernelGetModel() == 0)
    {
        // E94
        if (sceKernelDipsw(10) == 1) {
            addr = (void*)0x8B800000;
            *(int*)(0xBC100040) = (*(int*)(0xBC100040) & 0xFFFFFFFC) | 2;
        }
        else
            addr = (void*)0x8A000000;
    }
    // C00 / end of E94
    // C04
    // C08
    int i, j;
    for (i = 0; i < 32; i++)
        sp[i] = -1;

    // C4C
    for (i = 0; i < hwOpt->curArgs; i++)
    {
        SceKernelArgsStor *args = &hwOpt->args[i];
        switch (args->unk8 & 0xFFFF)
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
            // C80
            // C84
            // C88
            for (j = 0; j < 32; j++)
            {
                int tmpcond = (int)UCACHED(hwOpt->args[sp[j]].argp) >= (int)UCACHED(args->argp);
                if (tmpcond)
                {
                    // E20 / E28
                    int k;
                    for (k = cur; k >= j; k--)
                        sp[k + 1] = sp[k];
                }
                if (tmpcond || sp[j] == -1)
                {
                    // E40
                    cur++;
                    sp[j] = i;
                    break;
                }
            }
            break;
        }

        // CCC
        // CD0
    }

    // CDC / CF8
    for (i = 0; i < cur; i++)
    {
        SceKernelArgsStor *args = &hwOpt->args[sp[i]];
        switch (args->unk8 & 0xFFFF)
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
            // D48 / D4C
            addr -= UPALIGN256(args->args);
            sceKernelMemmove(addr, args->argp, args->args);
            if ((args->unk8 & 0xFFFF) == 0x80)
            {
                // DD0
                sceKernelMemset(args->argp, 0, args->args);
            }

            // D74
            args->argp = addr;
            break;
        }

        // D7C
        if ((args->unk8 & 0xFFFF) == 8)
        {
            // DC8
            hwOpt->unk76 = args->argp;
        }
        // D84
    }
    hwOpt->unk72 = addr;
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
        SceKernelLoadExecVSHParam args2;
        RebootArgs args;

        // 10F0
        args2.size = 48;
        args2.vshmainArgs = 0;
        args2.vshmainArgp = NULL;
        args2.configFile = NULL;
        args2.unk4 = 0;
        args2.flags = 0x10000;
        args2.unkArgs = 0;
        args2.unkArgp = NULL;
        if (arg1 == 0)
        {
            // 11B0
            args2.args = 0;
            args2.argp = NULL;
        }
        else {
            args2.args = *(int*)(arg1 + 4);
            args2.argp = *(int*)(arg1 + 8);
        }
        char *name;
        // 112C
        if ((s5 == 274) || (s5 == 275) || (s5 == 276) || (s5 == 277))
            name = g_umdEmuStr;
        else
            name = g_gameStr;
        // 116C
        args2.key = name;
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
    SceKernelLoadExecVSHParam args2;
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
    args2.vshmainArgs = 0;
    args2.vshmainArgp = NULL;
    args2.configFile = NULL;
    args2.unk4 = 0;
    args2.flags = 0x10000;
    args2.unkArgs = 0;
    args2.unkArgp = NULL;
    //
    if (arg1 == 0)
    {
        // 1528
        args2.args = 0;
        args2.argp = 0;
    }
    else {
        args2.args = *(int*)(arg1 + 4);
        args2.argp = *(int*)(arg1 + 8);
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
        args2.key = g_gameStr;
        break;

    case 2:
    case 3:
    case 4:
    case 5:
    case 19:
    case 20:
    case 21:
    case 22:
        args2.key = g_umdEmuStr;
        break;

    case 6:
    case 8:
        if (sceKernelGetChunk(3) < 0)
            args2.key = g_gameStr;
        else
            args2.key = g_umdEmuStr;

    case 96:
    case 97:
        args2.key = g_mlnAppStr;
        break;

    default:
        if (sceKernelDipsw(13) != 1)
            args2.key = g_gameStr;
        else
            args2.key = g_umdEmuStr;
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
    SceKernelLoadExecVSHParam args2;
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
        if (K1_USER_MODE())
        {   
            args.opt0 = 0x210;
            args.opt1 = 0;
            args.fileName = NULL;
            args.args2 = &args2;
            args.opt4 = arg;
            args.opt5 = 0;
            args.opt6 = 0;
            args.opt7 = 0;
            
            args2.size = 48;
            args2.args = 0;
            args2.argp = NULL;
            args2.key = g_vshStr;
            args2.vshmainArgs = 0;
            args2.vshmainArgp = NULL;
            args2.configFile = NULL;
            args2.unk4 = 0;
            args2.flags = 1;
            args2.unkArgs = 0;
            args2.unkArgp = NULL;
            args2.opt11 = 0;
            
            ret = sub_20FC(&args);
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
int sceKernelExitVSHVSH(SceKernelLoadExecVSHParam *opt)
{
    SceKernelLoadExecVSHParam args2;
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
        args2.args = 0;
        args2.argp = NULL;
        args2.key = NULL;
        args2.vshmainArgs = 0;
        args2.vshmainArgp = NULL;
        args2.configFile = NULL;
        args2.unk4 = 0;
        args2.flags = 0x10000;
        args2.unkArgs = 0;
        args2.unkArgp = NULL;
    }
    else
    {
        args2.args        = opt->args;
        args2.argp        = opt->argp;
        args2.key         = opt->key;
        args2.vshmainArgs = opt->vshmainArgs;
        args2.vshmainArgp = opt->vshmainArgp;
        args2.configFile  = opt->configFile;
        args2.unk4        = opt->unk4;
        args2.flags       = opt->flags;
        if (opt->size >= 48)
        {
            // 17A4
            args2.unkArgs = opt->unkArgs;
            args2.unkArgp = opt->unkArgp;
        }
        else {
            args2.unkArgs = 0;
            args2.unkArgp = NULL;
        }
    }

    // 1738
    if (args2.key == NULL)
        args2.key = g_vshStr;

    // 1754
    args2.flags |= 0x10000;
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

int LoadExecForKernel_BC26BEEF(SceKernelLoadExecVSHParam *opt, int arg1)
{
    char *name;
    if (opt == NULL)
        return 0x80020324;
    name = opt->key; // TODO: check if 'opt' is really a pointer
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
    return loadExecVSH(288, arg0, arg1, 0x10000);
}

// D4B49C4B
int sceKernelLoadExecVSHDiscUpdater(int arg0, int arg1)
{
    return loadExecVSH(289, arg0, arg1, 0x10000);
}

// 1B305B09
int sceKernelLoadExecVSHDiscDebug(int arg0, int arg1)
{
    if (sceKernelIsToolMode() != 0)
        return loadExecVSH(290, arg0, arg1, 0x10000);
    return 0x80020002;
}

int LoadExecForKernel_F9CFCF2F(int arg0, int arg1)
{
    return loadExecVSH(291, arg0, arg1, 0x10000);
}

int LoadExecForKernel_077BA314(int arg0, int arg1)
{
    return loadExecVSH(292, arg0, arg1, 0x10000);
}

int LoadExecForKernel_E704ECC3(int arg0, int arg1)
{
    return loadExecVSH(293, arg0, arg1, 0x10000);
}

int LoadExecForKernel_47A5A49C(int arg0, int arg1)
{
    return loadExecVSH(294, arg0, arg1, 0x10000);
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
    return loadExecVSH(320, arg0, arg1, 0x10000);
}

// D940C83C
int sceKernelLoadExecVSHMs2(int arg0, int arg1)
{
    return loadExecVSH(321, arg0, arg1, 0x10000);
}

// CC6A47D2
int sceKernelLoadExecVSHMs3(int arg0, int arg1)
{
    return loadExecVSH(322, arg0, arg1, 0x10000);
}

// 00745486
int sceKernelLoadExecVSHMs4(int arg0, int arg1)
{
    return loadExecVSH(323, arg0, arg1, 0x10000);
}

// 7CABED9B
int sceKernelLoadExecVSHMs5(int arg0, int arg1)
{
    return loadExecVSH(324, arg0, arg1, 0x10000);
}

int LoadExecForKernel_A6658F10(int arg0, int arg1)
{
    return loadExecVSH(325, arg0, arg1, 0x10000);
}

int LoadExecForKernel_16A68007(int arg0, int arg1)
{
    return loadExecVSH(337, arg0, arg1, 0x10000);
}

int LoadExecForKernel_032A7938(int arg0, int arg1)
{
    return loadExecVSH(338, arg0, arg1, 0x10000);
}

int LoadExecForKernel_40564748(int arg0, int arg1)
{
    return loadExecVSH(339, arg0, arg1, 0x10000);
}

int LoadExecForKernel_E1972A24(int arg0, int arg1)
{
    return loadExecVSH(340, arg0, arg1, 0x10000);
}

int LoadExecForKernel_C7C83B1E(int arg0, int arg1)
{
    return loadExecVSH(341, arg0, arg1, 0x10000);
}

int LoadExecForKernel_8C4679D3(int arg0, int arg1)
{
    return loadExecVSH(342, arg0, arg1, 0x10000);
}

int LoadExecForKernel_B343FDAB(int arg0, int arg1)
{
    return loadExecVSH(352, arg0, arg1, 0x10000);
}

int LoadExecForKernel_1B8AB02E(int arg0, int arg1)
{
    return loadExecVSH(353, arg0, arg1, 0x10000);
}

int LoadExecForKernel_C11E6DF1(int arg0, int arg1)
{
    return loadExecVSH(368, arg0, arg1, 0x10000);
}

int LoadExecForKernel_9BD32619(int arg0, int arg1)
{
    return loadExecVSH(369, arg0, arg1, 0x10000);
}

// C3474C2A
int sceKernelExitVSHKernel(SceKernelLoadExecVSHParam *arg)
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
    if (args->opt0 != 0x300) /* Run in a thread */
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
    if (comma != NULL)
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

int sub_2308(SceKernelLoadExecVSHParam *opt)
{
    K1_GET();
    if (opt != NULL)
    {
        if (!K1_USER_BUF_STA_SZ(opt, 16))
            return 0x800200D3;
        if (opt->key != NULL && !K1_USER_PTR(opt->key))
            return 0x800200D3;
        // 2328
        if (opt->configFile != NULL && !K1_USER_PTR(opt->configFile))
            return 0x800200D3;
        // 2344
        if (opt->unk4 != NULL && !K1_USER_PTR(opt->unk4))
            return 0x800200D3;
    }
    return 0;
}

// 2384
int loadExecVSH(int arg0, int arg1, int arg2, int arg3)
{
    K1_BACKUP();
    if (sceKernelIsIntrContext() == 0)
    {
        if (K1_USER_MODE())
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
            SceKernelLoadExecVSHParam args2;
            RebootArgs args;
        
            // 2488
            sub_29A4(&args2, arg3, arg2);
            args.opt0 = arg0;
            args.opt1 = 0;
            args.fileName = arg1;
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
        SceKernelLoadExecVSHParam args2;
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
        args.fileName = arg2;
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

int sub_26B0(int arg0, SceKernelLoadExecVSHParam *opt)
{
    SceKernelLoadExecVSHParam args2;
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
    ret = sub_2308(opt);
    if (ret < 0) {
        K1_RESET();
        return ret;
    }
    args2.size = 48;
    if (opt == NULL)
    {
        // 27E0
        args2.args = 0;
        args2.argp = NULL;
        args2.key = NULL;
        args2.vshmainArgs = 0;
        args2.vshmainArgp = NULL;
        args2.configFile = NULL;
        args2.unk4 = 0;
        args2.flags = 0x10000;
        args2.unkArgs = 0;
        args2.unkArgp = NULL;
    }
    else
    {
        args2.args = opt->args;
        args2.argp = opt->argp;
        args2.key = opt->key;
        args2.vshmainArgs = opt->vshmainArgs;
        args2.vshmainArgp = opt->vshmainArgp;
        args2.configFile = opt->configFile;
        args2.unk4 = opt->unk4;
        args2.flags = opt->flags;
        if (opt->size >= 48)
        {
            // 27D4
            args2.unkArgs = opt->unkArgs;
            args2.unkArgp = opt->unkArgp;
        }
        else {
            args2.unkArgs = 0;
            args2.unkArgp = NULL;
        }
    }

    // 2750
    if (args2.key == NULL)
        args2.key = g_vshStr;

    // 276C
    args.opt0 = arg0;
    args.opt1 = 0;
    args.fileName = NULL;
    args.args2 = &args2;
    args.opt4 = 0;
    args.opt5 = 0;
    args.opt6 = 0;
    args.opt7 = 0;

    args2.flags |= 0x10000;
    args2.opt11 = 0;
    ret = runExec(&args);
    K1_RESET();
    return ret;
}

int func_282C(SceKernelLoadExecVSHParam *arg)
{
    return sub_26B0(768, arg);
}

int runExecFromThread(int unk, RebootArgs *opt) // 2864
{
    int sp[20];
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
    opt->opt4 = &sp[8];

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
        sp[14] = str2;
    }

    ptr = sp[15];
    if (ptr != 0) {
        strncpy(str3, ptr, 256);
        sp[15] = str3;
    }

    sceKernelSetSystemStatus(0x40000);
    if (opt->opt0 != 0x300)
    {
        int ret = sceKernelPowerRebootStart(0);
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

int sub_2A64(RebootArgs *opt)
{
    int type = 0;
    int funcRet;
    SceKernelSysmemBlockInfo blkInfo;
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
    int rand = sceKernelGetInitialRandomValue();
    if (opt->opt0 != 0x300)
    {
        // 3218
        int ret = sceKernelRebootBeforeForUser(opt->args2);
        if (ret < 0)
            return ret;
    // 2AF8
        // 31F8
        ret = sceKernelRebootPhaseForKernel(3, opt->args2, 0, 0);
        if (ret < 0)
            return ret;
    // 2B00
        // 31D8
        ret = sceKernelRebootPhaseForKernel(2, opt->args2, 0, 0);
        if (ret < 0)
            return ret;
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
            SceUID id = sceKernelGetChunk(0);
            if (id > 0)
            {
                blkInfo.size = 56;
                sceKernelQueryMemoryBlockInfo(id, &blkinfo);
                opt->args2->opt5 = blkInfo.unk40;
            }
        }
    }

    // 2B30
    if (opt->opt0 != 0x300)
    {
        // 3174
        int ret = sceKernelRebootPhaseForKernel(1, opt->args2, 0, 0);
        if (ret < 0)
            return ret;
    // 2B38
        // 315C
        sceKernelRebootBeforeForKernel(opt->args2, 0, 0, 0);
    }

    // 2B40
    int ret = LoadCoreForKernel_F871EA38(EXEC_START, EXEC_END - EXEC_START);
    if (ret != 0)
    {
        sceKernelCpuSuspendIntr();
        *(int*)(0xBC100040) = (*(int*)(0xBC100040) & 0xFFFFFFFC) | 1;
        sceKernelMemset(0x88600000, 0, 0x200000);
        sceKernelMemset(EXEC_START, 0, EXEC_END);
        memset(0xBFC00000, 0, 4096);
        for (;;)
            ;
    }

    // 2BC4
    int st;
    COP0_STATE_GET(st, COP0_STATE_STATUS);
    COP0_STATE_SET(COP0_STATE_STATUS, st & 0xFFF7FFE0);
    // Skipped useless part

    // 2C58
    SceKernelUnkStruct *hwOpt = sub_32FC();
    hwOpt->unk24 |= type;
    if (type == 2)
    {
        // 2F4C
        if (opt->opt1 != 0)
        {  
            // 3134
            hwOpt->args[0].argp = opt->fileName;
            hwOpt->args[0].args = opt->opt1;
            hwOpt->args[0].unk8 = 1;
            hwOpt->curArgs = 1;
            hwOpt->unk36 = 0;
            // 2FFC
            sub_09D8(hwOpt, opt->args2);
        }
        else if (opt->fileName != NULL)
        {
            if ((opt->opt0 == 291) || (opt->opt0 == 293)
             || (opt->opt0 == 292) || (opt->opt0 == 294)
             || (opt->opt0 == 368) || (opt->opt0 == 369))
            {  
                // 2FAC
                // 2FB0
                hwOpt->args[0].argp = opt->args2->argp;
                hwOpt->args[0].args = strlen(opt->args2->argp) + 1;
                hwOpt->args[0].unk8 = 2;
                hwOpt->args[1].argp = opt->fileName;
                hwOpt->args[1].args = strlen(opt->fileName) + 1;
                hwOpt->args[1].unk8 = 64;
                hwOpt->curArgs = 2;
                hwOpt->unk36 = 0;
            }
            else
            {
                // 300C
                hwOpt->args[0].argp = opt->fileName;
                hwOpt->args[0].args = strlen(opt->fileName) + 1;
                hwOpt->args[0].unk8 = type;
                hwOpt->curArgs = 1;
                hwOpt->unk36 = 0;
                if ((opt->opt0 == 274) || (opt->opt0 == 275) || (opt->opt0 == 276) || (opt->opt0 == 277))
                {  
                    // 3064
                    SceUID id = sceKernelGetChunk(3);
                    funcRet = id;
                    if (id >= 0)
                    {  
                        void *addr = sceKernelGetBlockHeadAddr(id);
                        hwOpt->args[1].argp = addr;
                        hwOpt->args[1].args = strlen(addr) + 1;
                        hwOpt->args[1].unk8 = 64;
                        hwOpt->curArgs++;
                    }
                }
                // 30AC
                if ((opt->opt0 == 278) || (opt->opt0 == 280))
                {  
                    hwOpt->args[1].argp = opt->opt8;
                    hwOpt->args[1].args = 32;
                    hwOpt->args[1].unk8 = 128;
                    hwOpt->curArgs++;
                    SceUID id = sceKernelGetChunk(3);
                    funcRet = id;
                    if (id >= 0)
                    {  
                        void *addr = sceKernelGetBlockHeadAddr(id);
                        hwOpt->args[2].argp = addr;
                        hwOpt->args[2].args = strlen(addr) + 1;
                        hwOpt->args[2].unk8 = 64;
                        hwOpt->curArgs++;
                    }
                }
            }
            // 2FF4
            // 2FF8
            // 2FFC
            sub_09D8(hwOpt, opt->args2);
        }
    }
    else if (type == 1)
    {
        // 2E38
        int *ptr;
        if (opt->args2->vshmainArgs == 0)
        {  
            // 2ED4
            SceUID id = sceKernelGetChunk(0);
            if (id > 0)
            {  
                blkInfo.size = 56;
                sceKernelQueryMemoryBlockInfo(id, &blkInfo);
                hwOpt->args[hwOpt->curArgs].argp = blkInfo.unk40;
                hwOpt->args[hwOpt->curArgs].args = blkInfo.unk44;
                hwOpt->args[hwOpt->curArgs].unk8 = 256;
                hwOpt->unk40 = hwOpt->curArgs;
                hwOpt->curArgs++;
                ptr = hwOpt->args[hwOpt->curArgs].argp;
                ptr[0] = hwOpt->args[hwOpt->curArgs].args;
                ptr[1] = 32;
            }
        }
        else
        {  
            hwOpt->args[hwOpt->curArgs].argp = opt->args2->vshmainArgp;
            hwOpt->args[hwOpt->curArgs].args = opt->args2->vshmainArgs;
            hwOpt->args[hwOpt->curArgs].unk8 = 256;
            hwOpt->unk40 = hwOpt->curArgs;
            hwOpt->curArgs++;
            ptr = opt->args2->vshmainArgp;
            ptr[0] = opt->args2->vshmainArgs;
            ptr[1] = 32;
        }

        // 2E94
        ptr[2] = opt->opt4;
        if (opt->opt4 == 0)
        {  
            // 2EB4
            int v;
            if ((v = ptr[6]) == 0)
                if ((v = ptr[5]) == 0)
                    v = ptr[4];
            // 2ECC
            ptr[3] = v;
        }
        else
            ptr[3] = 0;
   
        // 2EA4
        ptr[6] = 0;
        ptr[4] = 0;
        ptr[5] = 0;
    }

    // 2C84
    if (g_valAreSet != 0)
    {
        hwOpt->args[hwOpt->curArgs].argp = g_val0;
        hwOpt->args[hwOpt->curArgs].args = g_val1;
        hwOpt->args[hwOpt->curArgs].unk8 = g_val2;
        hwOpt->curArgs++;
    }

    // 2CE0
    if ((opt->args2->flags & 0x10000) == 0)
        *(int*)(hwOpt->addr1 + hwOpt->unk12 * 32 - 12) = opt->opt0;

    // 2D04
    sub_0BBC(hwOpt, opt->args2);
    sceKernelSetDdrMemoryProtection(0x88400000, 0x400000, 12);
    if (opt->opt0 == 0x420)
        return funcRet;
    sceKernelMemset(0x88600000, 0, 0x200000);
    ret = decodeKL4E(0x88600000, 0x200000, EXEC_START + 4, 0);
    if (ret < 0)
    {
        // 2DD0
        sceKernelCpuSuspendIntr();
        *(int*)(0xBC100040) = (*(int*)(0xBC100040) & 0xFFFFFFFC) | 1;
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
    int (*reboot)(int, int, int, int) = 0x88600000;
    reboot(hwOpt, opt->args2, opt->opt0, rand);
}

SceKernelUnkStruct *sub_32FC()
{
    int addr1, args, addr4, addr5;
    SceKernelUnkStruct *opt;
    if (sceKernelGetModel() != 0)
        addr1 = 0x8B800000;
    else
        addr1 = 0x88400000;

    opt = (SceKernelUnkStruct*)((addr1 + 0x0004f) & 0xffffffc0);
    args = ((int)opt + 0x0103f) & 0xffffffc0;
    addr4 = (args + 0x1c03f) & 0xffffffc0;
    addr5 = (addr4 + 0x003bf) & 0xffffffc0;

    sceKernelMemset(addr1, 0, addr5 - addr1);

    opt->addr1   = args;
    opt->args   = addr4;
    opt->addr2   = addr5;
    opt->curArgs = 0;
    opt->unk24   = 0;
    opt->model   = sceKernelGetModel();
    opt->unk60   = 0;
    opt->dipswLo = sceKernelDipswLow32();
    opt->dipswHi = sceKernelDipswHigh32();
    opt->unk48   = 0;

    if (sceKernelDipsw(30) != 1)
        opt->dipswHi &= 0xFAFFFFFF;

    opt->addr  = 0x88000000;
    opt->unk4  = sceKernelSysMemRealMemorySize();
    opt->unk40 = -1;
    opt->unk36 = -1;
    opt->unk80 = sceKernelDipswCpTime();
    return opt;
}

