u8 g_1980[64] = {
  0xf4, 0x04, 0xa1, 0x3c, 0x64, 0x5f, 0x80, 0x4c, 0x4d, 0x4d, 0x1f, 0x43, 0x3a, 0xdf, 0x89, 0xe6,
  0x7f, 0x8e, 0xe1, 0x4a, 0x28, 0x90, 0x8a, 0x99, 0x13, 0xf4, 0x5d, 0xef, 0x04, 0x4a, 0x56, 0x94,
  0x5a, 0xcf, 0x09, 0xb6, 0xa6, 0xa6, 0xc3, 0x07, 0x4e, 0xbb, 0x79, 0x9c, 0xf2, 0xe7, 0x41, 0xf2,
  0xfb, 0xfd, 0xa4, 0x4c, 0x24, 0x1f, 0x35, 0x44, 0xc1, 0xb2, 0x32, 0x36, 0x1a, 0x53, 0xd8, 0x6e
};

u8 g_19C0[64] = {
  0xc2, 0x5a, 0xd7, 0x61, 0x70, 0x13, 0xd9, 0x96, 0x9d, 0x93, 0x31, 0xa6, 0x7f, 0x96, 0xf0, 0xee,
  0xb2, 0x2f, 0xb1, 0x40, 0xe0, 0x02, 0x54, 0x5b, 0x3a, 0x28, 0xc4, 0x5f, 0x29, 0x25, 0xdd, 0x80,
  0xf5, 0xc9, 0x91, 0x63, 0x58, 0x0b, 0xab, 0xf5, 0x40, 0xcf, 0xa8, 0xe1, 0x65, 0x14, 0xa0, 0x32,
  0x94, 0x87, 0x7e, 0x17, 0x42, 0xa0, 0x00, 0x75, 0xb6, 0xcd, 0x5b, 0x5d, 0x95, 0xf3, 0x80, 0x09
};

// 2.60 ipl
_start(...) // at 0x040F0000 
{
    *(s32*)0xBC100050 |= 0x7000;
    *(s32*)0xBC100078 |= 2;
    sp = 0x40FFF00;
    sub_040F0D70(0xBFC00040, 640, g_1980, g_19C0, 0x40F1AC0, 0x5040); // decrypt function
    t0 = 0x40F1AC0;
    t1 = 0x5040;
    t0 = t0 + t1;
    // 0074
    inline_memset4(t0, 0, 0x4100000 - t0); // memset by words
    cacheStuff1();
    s32 val = *(s32*)(0xBFC00FFC);
    pspCop0CtrlSet(17, val);
    if (val >= 0) {
        u32 addr = &cont;
        *(s32*)0xBFD00000 = 0x3C080000 | ((u32)addr >> 16); // lui $t0, addr>>16
        *(s32*)0xBFD00004 = 0x35080000 | (addr & 0xFFFF); // ori $t0, $t0, addr&0xffff
        *(s32*)0xBFD00008 = 0x01000008; // jr $t0
        *(s32*)0xBFD0000C = 0; // nop
        pspSync();
        *(s32*)0xBC10004C |= 2;
        // 00F0
        while (true) {
            pspSync();
        }
    }
    // 00F8
    inline_memset4(0xBFC00000, 0, 0x1000);
    // 0110
cont:
    pspCop0StateSet(COP0_STATE_STATUS, 0x60000000);
    pspCop0StateSet(COP0_STATE_CAUSE, 0);
    *(s32*)0xBC100004 = 0xFFFFFFFF;
    cacheStuff3();
    sp = 0x40FFF00;
    sub_040F07C0(0x4000000, 0xE0000, 0x40F1AC0, 0); // gunzip
    cacheStuff1();
    cacheStuff2();
    t9 = 0x4000000;
    sp = 0x40FFF00;
    t9();
}

void sub_017C()
{
    // 017C, dead code??
    at = a0 * 96;
    // 0188
    do {
        cond = (at != 0);
        at = at - 1;
    } while (cond);
    return;
}

sub_040F01A0(...) {} // at 0x040F01A0  // gzip-related

sub_040F0470(...) {} // at 0x040F0470  // gzip-related

sub_040F048C(...) {} // at 0x040F048C  // gzip-related

sub_040F0518(...) {} // at 0x040F0518  // gzip-related

sub_040F05A4(...) {} // at 0x040F05A4  // gzip-related

sub_040F05A8(...) {} // at 0x040F05A8  // gzip-related

sub_040F06C0(...) {} // at 0x040F06C0  // gzip-related

sub_040F07C0(...) {} // at 0x040F07C0  // gzip-related

sub_040F089C(...) {} // at 0x040F089C  // gzip-related

sub_040F09F0(...) {} // at 0x040F09F0  // gzip-related

void cacheStuff2() // at 0x040F0ACC 
{
    t1 = 0x1000 << ((pspCop0StateGet(COP0_STATE_CONFIG) >> 9) & 7);
    pspCop0StateSet(COP0_STATE_TAG_LO, 0);
    pspCop0StateSet(COP0_STATE_TAG_HI, 0);
    t0 = 0;
    // 0AE8
    do {
        pspCache(0x1, t0);
        pspCache(0x3, t0);
        t0 = t0 + 64;
    } while (t0 != t1);
}

void cacheStuff3() // at 0x040F0B04 
{
    t1 = 0x1000 << ((pspCop0StateGet(COP0_STATE_CONFIG) >> 6) & 7);
    pspCop0StateSet(COP0_STATE_TAG_LO, 0);
    pspCop0StateSet(COP0_STATE_TAG_HI, 0);
    t0 = 0;
    // 0B20
    do {
        pspCache(0x11, t0);
        pspCache(0x13, t0);
        t0 = t0 + 64;
    } while (t0 != t1);
}

// never called function
void cacheStuff4() // at 0x040F0B3C
{
    t1 = 0x800 << ((pspCop0StateGet(COP0_STATE_CONFIG) >> 6) & 7);
    t0 = 0;
    // 0B50
    do {
        pspCache(0x14, t0);
        pspCache(0x14, t0);
        t0 = t0 + 64;
    } while (t0 != t1);
    pspSync();
}

// cache stuff
void cacheStuff1() // at 0x040F0B6C 
{
    t1 = 0x800 << ((pspCop0StateGet(COP0_STATE_STATUS) >> 6) & 7);
    t0 = 0;
    // 0B80
    do {
        pspCache(0x10, t0);
        t2 = pspCop0StateGet(COP0_STATE_TAG_LO);
        t3 = pspCop0StateGet(COP0_STATE_TAG_HI);
        if ((t2 >> 20) & 1 != 0) {
            pspCache(0x1A, (t2 << 13) | t0);
        }
        // 0BA4
        if ((t3 >> 20) & 1 != 0) {
            pspCache(0x1A, (t3 << 13) | t0);
        }
        // 0BB4
        t0 = t0 + 64;
    } while (t0 != t1);
    pspSync();
}

// like memset but where n == 0 still copies 1 byte
void *memset_(void *s, int c, s32 n) // at 0x040F0BC8 
{
    void *ret = s;
    // 0BCC
    do {
        *(s++) = c;
    } while ((--n) > 0);
    return ret;
}

// like memcpy but where n == 0 still copies 1 byte
void *memcpy_(void *dest, const void *src, s32 n) // at 0x040F0BE4
{
    void *ret = dest;
    // 0BE8
    do {
        *(dest++) = *(src++);
    } while ((--n) > 0);
    return ret;
}

typedef struct { // size 112
    u32 h[8]; // 0
    s32 unk2; // 32
    s16 unk3, unk4; // 36, 38
    s32 unk5, unk6; // 40, 44
    u8 buf[64];
} Sha256Context; // inspired by SceKernelUtilsSha1Context

void sub_040F0C08(u8 *in1, u32 inSize1, u8 *in2, u32 inSize2, u8 *out) // at 0x040F0C08 
{
    Sha256Context ctx; // sp
    u8 buf1[32]; // sp + 112
    u8 buf2[64]; // sp + 144
    if (inSize2 == 0 || in2 == NULL) {
        sha256Digest(in1, inSize1, out);
        return;
    }
    // 0C78
    memset_(buf2, 0, 64);
    if (inSize1 > 64) {
        // 0D5C
        sha256digest(in1, arg1, buf2);
    } else {
        memcpy_(buf2, in1, arg1);
    }
    // 0CAC
    for (s32 i = 0; i < 64; i++) {
        buf2[i] ^= 0x36;
    }
    sha256BlockInit(&ctx);
    sha256BlockUpdate(&ctx, buf2, 64);
    sha256BlockUpdate(&ctx, in2, inSize2);
    sha256BlockResult(&ctx, buf1);
    // 0D04
    for (s32 i = 0; i < 64; i++) {
        buf2[i] ^= 0x6A;
    }
    sha256BlockInit(&ctx);
    sha256BlockUpdate(&ctx, buf2, 64);
    sha256BlockUpdate(&ctx, buf1, 32);
    sha256BlockResult(&ctx, out);
}

//sub_040F0D70(0xBFC00040, 640, 0x40F1980, 0x40F19C0, 0x40F1AC0, 0x5040);
void sub_040F0D70(void *preipl, u32 preiplSize, void *unk1, void *unk2, void *encryptedImg, s32 encryptedSize) // at 0x040F0D70 
{
    SceKernelUtilsMt19937Context ctx; // sp..sp+2500
    u32 hash[8]; // sp + 2512
    u32 buf1[16]; // sp + 2544
    u32 buf2[8]; // sp + 2608
    sub_040F0C08(unk1, 64, preipl, preiplSize, hash);
    ctx.count = 0;
    // 0DCC
    for (s32 i = 0; i < 616; i += 8) {
        inline_wmemcpy(&ctx.state[i], hash, 8);
    }
    inline_wmemset(hash, 0, 8);
    // 0E40
    for (s32 i = 0; i < 623; i++) {
        mt19937UInt(&ctx);
    }
    s32 *decryptBuf = encryptedImg;
    // 0E68
    for (s32 i = 0; i < encryptedSize; i += 32) {
        // 0E6C
        for (s32 i = 0; i < 16; i++) {
            buf1[i] = mt19937UInt(&ctx);
        }
        sub_040F0C08(unk2, 64, buf1, 64, buf2);
        // 0EA4
        for (s32 i = 0; i < 8; i++) {
            *(decryptBuf++) ^= buf2[i];
        }
    }
}

sha256BlockUpdate(Sha256Context *ctx, u8 *data, u32 size) {} // at 0x040F0F00 

sha256BlockResult(Sha256Context *ctx, u8 *digest) {} // at 0x040F10D0 

s32 sha256Digest(u8 *data, u32 size, u8 *digest) // at 0x040F12CC 
{
    Sha256Context ctx;
    // basically an inlined sha256BlockInit()
    ctx.h[0] = 0x6A09E667;
    ctx.h[1] = 0xBB67AE85;
    ctx.h[2] = 0x3C6EF372;
    ctx.h[3] = 0xA54FF53A;
    ctx.h[4] = 0x510E527F;
    ctx.h[5] = 0x9B05688C;
    ctx.h[6] = 0x1F83D9AB;
    ctx.h[7] = 0x5BE0CD19;
    ctx.unk3 = 0;
    ctx.unk4 = 0;
    ctx.unk5 = 0;
    ctx.unk6 = 0;
    sha256BlockUpdate(&ctx, data, size);
    sha256BlockResult(&ctx, digest);
    return 0;
}

sha256BlockInit(Sha256Context *ctx) {} // at 0x040F1388 

sub_040F1420(...) {} // at 0x040F1420  // sha256 stuff

mt19937UInt(...) {} // at 0x040F1890 

