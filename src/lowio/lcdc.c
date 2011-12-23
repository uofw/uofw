/* Copyright (C) 2011 The uOFW team
   See the file COPYING for copying permission.
*/

#include "../global.h"

#include "lcdc.h"

typedef struct {
    u16 fPrch; // front porch
    u16 bPrch; // back porch
    u16 sync; // sync width
    u16 res;
} SceLcdcModeline;

typedef struct // size: 28
{
    int id; // 0
    float sync; // 4, frequency of displaying one pixel
    char clk[2]; // 8
    char syncDiff; // 10
    char flags; // 11
    SceLcdcModeline x, y;
} SceLcdcDispMode;

typedef struct // size: 16
{
    int id;
    short xres, v1_6, v1_8, yres, v1_12, v1_14; // the unknown variables are stored in hwData[] on PSP Go (probably), instead of static values
} SceLcdcUnkElem2;

typedef struct
{
    int v2_0;
    int syncDiff;
    int v2_8;
    int unused1;
    int xbPrch, xsync, xfPrch, xres; // 16 - 28
    int ybPrch, ysync, yfPrch, yres; // 32 - 44
    int hpc; // 48
    int vpc; // 52
    int unused3[2];
    int shiftY, shiftX; // 64 - 68
    int realX, realY; // 72 - 76
    int v2_80; // 80
    int unused4[7];
    int v2_112;
} SceLcdcHwController;

typedef struct // size: 120
{
    int v3_0;
    int mobo;
    int v3_8;
    int scale;
    int dispId;
    int scaledXres;
    int yres;
    int v3_28;
    SceLcdcHwController *hwCtl; // 32
    SceLcdcDispMode disp; // 36
    int v3_64;
    int shiftX, shiftY;
    int realX, realY;
    int tabId;
    int v3_88;
    int hwData[7]; // data read from & sent to 0xBE140180 - 0xBE140198
} SceLcdc;

SceLcdc g_lcdc; // 0xD94C

// 0xC2B4
SceLcdcDispMode g_lcdcDispModeList[13] =
{
    { 0,  9000000.0f, {0, 3}, 0, 0, { 2, 41,  2, 480}, {10, 2,  2, 272}}, //  0
    { 1, 27000000.0f, {0, 1}, 1, 0, {16, 61, 61, 720}, { 9, 6, 30, 480}}, //  1
    { 2, 25175000.0f, {1, 1}, 1, 0, {32, 96, 32, 640}, {10, 2, 33, 480}}, //  2
    { 3, 27000000.0f, {0, 1}, 1, 0, {61, 96, 61, 640}, {10, 2, 33, 480}}, //  3
    { 4, 13500000.0f, {0, 2}, 1, 0, {16, 61, 61, 720}, { 5, 3, 14, 240}}, //  4
    { 5, 13500000.0f, {0, 2}, 1, 0, {16, 61, 61, 720}, { 6, 3, 14, 240}}, //  5
    { 6, 13500000.0f, {0, 2}, 1, 0, {16, 61, 61, 720}, { 3, 3, 14, 505}}, //  6
    {11,  9000000.0f, {0, 3}, 0, 0, { 3, 41,  2, 480}, {10, 2,  2, 272}}, //  7
    {12, 27000000.0f, {0, 1}, 1, 0, {16, 61, 61, 720}, {10, 6, 30, 480}}, //  8
    {14,  9000000.0f, {0, 2}, 0, 0, {10, 51,  4, 320}, { 5, 5,  5, 180}}, //  9
    {15, 18000000.0f, {1, 1}, 0, 0, { 2, 41,  2, 480}, {10, 2,  2, 272}}, // 10
    {16, 18000000.0f, {1, 1}, 0, 0, { 3, 41,  2, 480}, {10, 2,  2, 272}}, // 11
    {17, 13500000.0f, {2, 4}, 1, 7, {16, 61, 61, 720}, {10, 6, 30, 479}}  // 12
};

// 0xC420
SceLcdcUnkElem2 g_lcdcTab[8] = // ???
{   
    {1, 0x02BA, 0x0AFC, 0x0580, 0x0168, 0x0C18, 0x060C},
    {2, 0x0298, 0x0B90, 0x05C8, 0x0156, 0x0CB8, 0x065C},                                                                                     
    {3, 0x0274, 0x0C34, 0x061C, 0x0144, 0x0D70, 0x06B8},                                                                                     
    {4, 0x0251, 0x0CEC, 0x0678, 0x0132, 0x0E38, 0x071C},                                                                                     
    {5, 0x02BA, 0x0AFC, 0x0580, 0x01E0, 0x0910, 0x0488},
    {6, 0x0298, 0x0B90, 0x05C8, 0x01C8, 0x098C, 0x04C4},
    {7, 0x0274, 0x0C34, 0x061C, 0x01B0, 0x0A14, 0x0508},
    {8, 0x0251, 0x0CEC, 0x0678, 0x0198, 0x0AAC, 0x0554}
};

char lcdEventName[] = "SceLcdc"; // 0xC134
// C744
SceSysEventHandler lcdcEvent = {0x40, lcdEventName, 0x00FFFF00, &eventHandler, 0, 0, NULL, {0, 0, 0, 0, 0, 0, 0, 0, 0}};

// 75F0
int sceLcdcInit()
{
    SceLcdcHwController *hwCtl = (SceLcdcHwController*)(0xBE140000);
    memset(&g_lcdc, 0, sizeof(g_lcdc));
    g_lcdc.v3_0 = 1;
    int v = sub_0768();
    sub_0A80();
    int model = sceSysreg_driver_E2A5D1EE();
    g_lcdc.mobo = model;
    // patch for newer versions
    if (model > 0x4FFFFF)
    {
        // 76B0
        if (model > 0x7FFFFF)
        {
            // 7720
            hwCtl = (SceLcdcHwController*)(0xBE140100);
            if (v == 0) {
                *(int*)(0xBE140000) = 0;
                *(int*)(0xBE140200) = 1;
            }
            // 7740
            g_lcdcDispModeList[8].clk[1] = 2;
            g_lcdc.v3_8 = 1;
            g_lcdcDispModeList[3].id = -1;
            g_lcdcDispModeList[6].clk[1] = 4;
            g_lcdc.scale = 1;
            g_lcdcDispModeList[1].clk[0] = 2;
            g_lcdcDispModeList[1].clk[1] = 2;
            g_lcdcDispModeList[2].id = -1;
            g_lcdcDispModeList[4].clk[0] = 2;
            g_lcdcDispModeList[4].clk[1] = 4;
            g_lcdcDispModeList[5].clk[0] = 2;
            g_lcdcDispModeList[5].clk[1] = 4;
            g_lcdcDispModeList[6].clk[0] = 2;
            *(int*)(0xBE140170) = 1;
        }
        else
        {
            hwCtl = (SceLcdcHwController*)(0xBE140000);
            g_lcdc.scale = 3;
            g_lcdc.v3_8 = 1;
            g_lcdcDispModeList[3].id    = -1;
            g_lcdcDispModeList[8].clk[1] = 1;
            *(int*)(0xBE140070) = 1;
            g_lcdcDispModeList[0].clk[1] = 1;
            g_lcdcDispModeList[1].clk[0] = 1;
            g_lcdcDispModeList[1].clk[1] = 1;
            g_lcdcDispModeList[2].id    = -1;
            g_lcdcDispModeList[4].clk[0] = 1;
            g_lcdcDispModeList[4].clk[1] = 1;
            g_lcdcDispModeList[5].clk[0] = 1;
            g_lcdcDispModeList[5].clk[1] = 1;
            g_lcdcDispModeList[6].clk[0] = 1;
            g_lcdcDispModeList[6].clk[1] = 1;
            g_lcdcDispModeList[7].clk[1] = 1;
            g_lcdcDispModeList[8].clk[0] = 1;
        }
    }
    else
        g_lcdc.scale = 1;
    // 766C
    g_lcdc.hwCtl = hwCtl;
    sceLcdc_driver_901B9073();
    g_lcdc.hwCtl->v2_0 |= 3;
    sceKernelRegisterSysEventHandler(&lcdcEvent);
    return 0;
}

// 7788
int sceLcdcResume()
{   
    sub_0774();
    sceSysreg_driver_39B115A7(0, *(char*)(0xC2BD));
    sub_0768();
    if (g_lcdc.mobo > 0x7FFFFF) {
        *(int*)(0xBE140000) = 0;
        *(int*)(0xBE140200) = 1;
    }
    // 77F4
    g_lcdc.hwCtl->syncDiff = g_lcdc.disp.syncDiff;
    if (g_lcdc.mobo > 0x7FFFFF)
    {   
        if ((*(int*)(0xBE1401A0) & 1) == 0 && (g_lcdc.disp.flags & 1) != 0) {
            // 79C4
            sub_7DD8();
        }
        // 7830
        *(int*)(0xBE1401A0) = g_lcdc.disp.flags & 0xFE;
        *(int*)(0xBE140180) = g_lcdc.hwData[0];
        *(int*)(0xBE140184) = g_lcdc.hwData[1];
        *(int*)(0xBE140188) = g_lcdc.hwData[2];
        *(int*)(0xBE14018C) = g_lcdc.hwData[3];
        *(int*)(0xBE140190) = g_lcdc.hwData[4];
        *(int*)(0xBE140194) = g_lcdc.hwData[5];
        *(int*)(0xBE140198) = g_lcdc.hwData[6];
    }
    // 7898
    g_lcdc.hwCtl->v2_8   = g_lcdc.v3_28;
    g_lcdc.hwCtl->xbPrch = g_lcdc.disp.x.bPrch;
    g_lcdc.hwCtl->xsync  = g_lcdc.disp.x.sync;
    g_lcdc.hwCtl->xfPrch = g_lcdc.disp.x.fPrch;
    g_lcdc.hwCtl->xres   = g_lcdc.disp.x.res;
    g_lcdc.hwCtl->ybPrch = g_lcdc.disp.y.bPrch;
    g_lcdc.hwCtl->ysync  = g_lcdc.disp.y.sync;
    g_lcdc.hwCtl->yfPrch = g_lcdc.disp.y.fPrch;
    g_lcdc.hwCtl->yres   = g_lcdc.disp.y.res;
    g_lcdc.hwCtl->shiftY = g_lcdc.shiftY;
    g_lcdc.hwCtl->shiftX = g_lcdc.shiftX;
    g_lcdc.hwCtl->realX  = g_lcdc.realX;
    g_lcdc.hwCtl->realY  = g_lcdc.realY;
    if (g_lcdc.mobo > 0x7FFFFF)
        *(int*)(0xBE1401A0) = g_lcdc.disp.flags;
    // 7924
    g_lcdc.hwCtl->v2_0 |= 3;
    if (g_lcdc.v3_8 != 0)
        g_lcdc.hwCtl->v2_112 = 1;
    // 7940
    sub_0774();
    if (g_lcdc.mobo > 0x7FFFFFFF)
    {   
        // 79A8
        sceSysreg_driver_39B115A7(g_lcdc.disp.clk[0], 1);
        *(int*)(0xBE1401B0) = g_lcdc.disp.clk[1] - 1;
    }
    else
        sceSysreg_driver_39B115A7(g_lcdc.disp.clk[0], g_lcdc.disp.clk[1]);
    // 7964
    sub_0768();
    if (g_lcdc.v3_0 != 0)
        sub_0A80();
    return 0;
}

// 901B9073
int sceLcdcCheckMode(void) // load current display specs into g_lcdc, from the hardware
{   
    g_lcdc.disp.syncDiff = g_lcdc.hwCtl->syncDiff & 0xFF;
    if (g_lcdc.mobo > 0x7FFFFF)
    {   
        g_lcdc.disp.flags = *(int*)(0xBE1401A0) & 0xFF;
        int i;
        for (i = 0; i < 7; i++)
            g_lcdc.hwData[i] = *(int*)(0xBE140180 + i * 4);
    }
    // 7A5C
    g_lcdc.v3_28 = g_lcdc.hwCtl->v2_8;
    int v28 = g_lcdc.hwCtl->xres;
    int v32 = g_lcdc.hwCtl->ybPrch;
    int v36 = g_lcdc.hwCtl->ysync;
    int v40 = g_lcdc.hwCtl->yfPrch;
    int v44 = g_lcdc.hwCtl->yres;
    g_lcdc.disp.x.bPrch = g_lcdc.hwCtl->xbPrch & 0xFFFF;
    g_lcdc.shiftY = g_lcdc.hwCtl->shiftY;
    g_lcdc.disp.x.sync = g_lcdc.hwCtl->xsync & 0xFFFF;
    g_lcdc.shiftX = g_lcdc.hwCtl->shiftX;
    g_lcdc.disp.x.fPrch = g_lcdc.hwCtl->xfPrch & 0xFFFF;
    int v72 = g_lcdc.hwCtl->realX;
    g_lcdc.realX = v72;
    int v76 = g_lcdc.hwCtl->realY;
    g_lcdc.disp.x.res = v28 & 0xFFFF;
    g_lcdc.disp.y.bPrch = v32 & 0xFFFF;
    g_lcdc.disp.y.sync = v36 & 0xFFFF;
    g_lcdc.disp.y.fPrch = v40 & 0xFFFF;
    g_lcdc.disp.y.res = v44 & 0xFFFF;
    g_lcdc.disp.sync = 9000000.0f; // 0xC1B4
    g_lcdc.realY = v76;
    if (g_lcdc.disp.syncDiff != 0)
    {
        // 7D20
        if (g_lcdc.hwData[0] == 1)
        {   
            // 7D3C
            g_lcdc.scaledXres = g_lcdc.hwData[1];
            g_lcdc.yres = g_lcdc.hwData[2];
        }
        else {
            g_lcdc.yres = v76;
            g_lcdc.scaledXres = v72;
        }
    }
    else {
        g_lcdc.yres = v44 & 0xFFFF;
        g_lcdc.scaledXres = v28 & 0xFFFF;
    }
    // 7AE8
    int hw = *(int*)(0xBC100060);
    char hwBit = (hw >> 22) & 1;
    g_lcdc.disp.clk[0] = hwBit;
    switch (hw & 0x300000)
    {
    case 0x100000:
        // 7D18
        g_lcdc.disp.clk[1] = 1;
        break;
    case 0x200000:
        g_lcdc.disp.clk[1] = 2;
        break;
    case 0x300000:
        g_lcdc.disp.clk[1] = 1;
        g_lcdc.disp.clk[0] = 2;
        break;
    case 0:
        g_lcdc.disp.clk[1] = 3;
        break;
    }
    // 7B24
    if (g_lcdc.mobo > 0x7FFFFF)
        g_lcdc.disp.clk[1] *= (*(int*)(0xBE1401B0) + 1);
    // 7B5C
    int xLine = g_lcdc.disp.x.fPrch + g_lcdc.disp.x.bPrch + g_lcdc.disp.x.sync + g_lcdc.disp.x.res;
    int zoom  = g_lcdc.scale;
    int yLine = g_lcdc.disp.y.fPrch + g_lcdc.disp.y.bPrch + g_lcdc.disp.y.sync + g_lcdc.disp.y.res;
    char param = g_lcdc.disp.flags & 7;
    // 7BAC
    int i;
    for (i = 0; i < 13; i++)
    {
        SceLcdcDispMode *cur = &g_lcdcDispModeList[i];
        if (xLine == (cur->x.fPrch + cur->x.bPrch + cur->x.sync + cur->x.res) * zoom)
        {
            // 7C74
            if (yLine == cur->y.fPrch + cur->y.bPrch + cur->y.sync + cur->y.res
             && cur->y.bPrch == hwBit
             && param == cur->flags)
            {
                g_lcdc.disp.sync = cur->sync;
                g_lcdc.dispId = cur->id;
                if (cur->id != 0 && g_lcdc.disp.syncDiff == 0) {
                    // 7CD8
                    g_lcdc.disp.syncDiff = (g_lcdc.disp.x.sync / zoom) - v36;
                }
                break;
            }
        }
        // 7BDC
    }
    // 7BE8
    g_lcdc.v3_88 = 0;
    g_lcdc.tabId = 0;
    if (g_lcdc.hwData[0] == 1)
    {
        // 7C18
        // 7C30
        for (i = 0; i < 8; i++)
        {
            SceLcdcUnkElem2 *cur = &g_lcdcTab[i];
            if (g_lcdc.hwData[6] == cur->v1_12)
            {
                // 7C68
                g_lcdc.tabId = cur->id;
                break;
            }
        }
        // 7C50
        if (g_lcdc.tabId != 0)
            return 0;
        g_lcdc.v3_88 = 2;
    }
    else if ((g_lcdc.disp.flags & 0x20) != 0)
        g_lcdc.v3_88 = 1;
    // (7C0C)
    return 0;
}

// 7D50
int eventHandler(int ev_id, char *ev_name, void *param, int *result)
{
    if (ev_id == 0x04004)
    {
        // 7DB0
        if (((int*)param)[1] != 2) {
            sub_0A8C();
            sub_0774();
        }
    }
    else if (ev_id == 0x10004)
    {
        // 7D80
        if (((u32*)((int*)param)[1])[1] == 0x80000000)
            sceLcdc_driver_901B9073();
        else
            sceLcdcResume();
    }
    return 0;
}

int sub_7DD8()
{
    sceSysreg_driver_24FE340D(0);
    sceSysreg_driver_325B9873(0, 4);
    sceSysreg_driver_69CC777F(0);
    // 7DFC
    while ((*(int*)(0xBE700020) & 4) != 0)
        ;
    *(int*)(0xBE700000) = 0;
    // 7E18
    while ((*(int*)(0xBE700020) & 4) != 0)
        ;
    // 7E2C
    while ((*(int*)(0xBE700020) & 2) != 0)
        ;
    *(int*)(0xBE700004) = 0;
    // 7E44
    while ((*(int*)(0xBE700020) & 8) != 0)
        ;
    *(int*)(0xBE70000C) = -1;
    // 7E60
    while ((*(int*)(0xBE700020) & 0x10) != 0)
        ;
    *(int*)(0xBE700010) = 0;
    // 7E78
    while ((*(int*)(0xBE700020) & 0x20) != 0)
        ;
    *(int*)(0xBE700014) = 0;
    // 7E90
    while ((*(int*)(0xBE700020) & 0x3A) != 0)
        ;
    *(int*)(0xBE700001) = 1;
    // 7EAC
    while ((*(int*)(0xBE700020) & 4) != 0)
        ;
    sceSysreg_driver_24FE340D(0);
    sceSysreg_driver_325B9873(0, 6);
    sceSysreg_driver_69CC777F(0);
    *(int*)(0xBE700024) = 1;
    return 0;
}

// ACD0815F
int sceLcdcCheckMode(int id, int xres, int yres, int arg3)
{
    if (arg3 >= 4)
        return 0x80000108;
    // 7F1C
    int i;
    SceLcdcDispMode *dispMode = NULL;
    for (i = 0; i < 13; i++)
    {
        SceLcdcDispMode *cur = &g_lcdcDispModeList[i];
        if (cur->id == id) {
            dispMode = cur;
            break;
        }
    }
    // 7F40
    if (dispMode == NULL)
        return 0x8000107;
    if (xres >= 480)
    {
        // 7FC0
        if (dispMode->x.res < xres)
            return 0x80000104;
    }
    else if (dispMode->x.res >= 480)
        return 0x80000104;
    // 7F6C
    if ((xres & 3) != 0)
        return 0x80000104;
    if (yres >= 272)
    {
        // 7FA8
        if (dispMode->y.res < yres)
            return 0x80000104;
    }
    else if (dispMode->y.res >= 272)
        return 0x80000104;
    return 0;
}

// 42BA9D50
int sceLcdcSetMode(int id, int xres, int yres, int arg3)
{   
    int ret = sceLcdcCheckMode(id, xres, yres, arg3);
    if (ret < 0)
        return ret;
    // 8044
    int i;
    SceLcdcDispMode *disp = NULL;
    for (i = 0; i < 13; i++)
    {   
        SceLcdcDispMode *cur = &g_lcdcDispModeList[i];
        if (cur->id == id) {
            disp = cur;
            break;
        }
    }
    // 8068
    int oldIntr = sceKernelCpuSuspendIntr();
    g_lcdc.scaledXres = xres * g_lcdc.scale;
    g_lcdc.yres = yres;
    g_lcdc.v3_28 = arg3;
    g_lcdc.dispId = id;
    if (*(short*)g_lcdc.disp.clk != *(short*)disp->clk)
    {   
        sub_0774();
        if (g_lcdc.mobo > 0x7FFFFF)
        {   
            // 85FC
            sceSysreg_driver_39B115A7(disp->clk[0], 1);
            *(int*)(0xBE1401B0) = disp->clk[1] - 1;
        }
        else
            sceSysreg_driver_39B115A7(disp->clk[0], disp->clk[1]);
        // 80B8
        sub_0768();
    }
    // 80C0
    g_lcdc.disp = *disp;
    if (g_lcdc.scale != 1)
    {   
        g_lcdc.disp.x.bPrch *= g_lcdc.scale;
        g_lcdc.disp.x.res   *= g_lcdc.scale;
        g_lcdc.disp.x.sync  *= g_lcdc.scale;
        g_lcdc.disp.x.fPrch *= g_lcdc.scale;
    }
    // 8148
    if (g_lcdc.disp.syncDiff == 1 && g_lcdc.disp.x.res != g_lcdc.scaledXres)
        g_lcdc.disp.syncDiff = 0;
    // 8158
    if (g_lcdc.v3_88 == 1 && (g_lcdc.disp.flags & 1) != 0) {
        // 85D0
        g_lcdc.disp.flags |= 0x20;
    }
    // 816C
    g_lcdc.hwData[0] = 0;
    if (g_lcdc.disp.syncDiff != 0)
    {
        // 8390
        if (g_lcdc.disp.x.res <= 480 || g_lcdc.tabId == 0)
        {
            // 8438
            // 843C
            if (g_lcdc.v3_88 == 2)
            {
                // 8560
                u32 diff1 = g_lcdc.disp.y.res - g_lcdc.yres;
                u32 diff2 = g_lcdc.disp.x.res - g_lcdc.scaledXres;
                g_lcdc.hwData[0] = 1;
                g_lcdc.shiftY = (int)(diff1 + (diff1 >> 31)) >> 1;
                g_lcdc.hwData[1] = g_lcdc.scaledXres;
                g_lcdc.shiftX = (int)(diff2 + (diff2 >> 31)) >> 1;
                g_lcdc.hwData[2] = g_lcdc.yres;
                g_lcdc.realX = g_lcdc.scaledXres;
                g_lcdc.hwData[3] = 0x800;
                g_lcdc.realY = g_lcdc.yres;
                g_lcdc.hwData[4] = 0x800;
                g_lcdc.hwData[5] = 0x1000;
                g_lcdc.hwData[6] = 0x1000;
            }
            else
            {
                u32 diff1 = g_lcdc.disp.y.res - g_lcdc.yres;
                u32 diff2 = g_lcdc.disp.x.res - g_lcdc.scaledXres;
                int v1 = (s32)(diff1 + (diff1 >> 31)) >> 1;
                if (id == 6)
                    v1 = 0;
                int v2 = (s32)(diff2 + (diff2 >> 31)) >> 1;
                if (v1 == 0)
                {
                    int corr = 2;
                    if ((g_lcdc.disp.flags & 1) == 0)
                        corr = 1;
                    g_lcdc.disp.y.sync -= corr;
                    g_lcdc.disp.y.res += corr;
                }
                // 84AC
                int align = g_lcdc.scale << 3;
                v2 = (v2 / align) * align;
                if (v2 >= 240)
                {
                    int diff = v2 - 240;
                    g_lcdc.disp.x.res -= diff;
                    v2 = 240;
                    g_lcdc.disp.x.sync += diff;
                    if (g_lcdc.disp.x.sync > 0xFF)
                    {
                        g_lcdc.disp.x.sync = 0xFF;
                        int corr = g_lcdc.disp.x.sync - 0xFF;
                        if (g_lcdc.mobo > 0x4FFFFF) {
                            g_lcdc.disp.x.bPrch += corr;
                            corr = 0;
                        }
                        // 8534
                        g_lcdc.disp.x.res += corr;
                    }
                }
                // 8544
                g_lcdc.shiftY = v1;
                g_lcdc.realY = g_lcdc.yres;
                g_lcdc.shiftX = v2;
                g_lcdc.realX = g_lcdc.scaledXres;
            }
        }
        else
        {
            SceLcdcUnkElem2 *elem2 = &g_lcdcTab[g_lcdc.tabId - 1];
            u32 diff1 = g_lcdc.disp.x.res - elem2->xres;
            u32 diff2 = g_lcdc.disp.y.res - elem2->yres;
            g_lcdc.hwData[0] = 1;
            g_lcdc.shiftY = (s32)(diff2 + (diff2 >> 31)) >> 1;
            g_lcdc.hwData[1] = g_lcdc.scaledXres;
            g_lcdc.shiftX = (s32)(diff1 + (diff1 >> 31)) >> 1;
            g_lcdc.hwData[2] = g_lcdc.yres;
            g_lcdc.realX = elem2->xres;
            g_lcdc.hwData[3] = elem2->v1_8;
            g_lcdc.realY = elem2->yres;
            g_lcdc.hwData[4] = elem2->v1_14;
            g_lcdc.hwData[5] = elem2->v1_6;
            g_lcdc.hwData[6] = elem2->v1_12;
        }
    }
    else
    {
        g_lcdc.shiftY = 0;
        int corr = 0;
        g_lcdc.shiftX = 0;
        if (g_lcdc.dispId != 0)
            corr = g_lcdc.v3_64 * g_lcdc.scale;
        // 81A0
        g_lcdc.disp.y.fPrch += corr;
        g_lcdc.disp.x.sync -= corr;
        if (g_lcdc.yres < g_lcdc.disp.y.res)
        {
            corr = *(short*)&g_lcdc.yres;
            g_lcdc.disp.y.res = corr;
            g_lcdc.disp.y.fPrch += g_lcdc.disp.y.res - corr;
        }
    }                                                                                                                                       
    // 81E8
    // 81EC
    g_lcdc.hwCtl->syncDiff = g_lcdc.disp.syncDiff;
    if (g_lcdc.mobo > 0x7FFFFF)
    {
        if ((*(int*)(0xBE1401A0) & 1) == 0 && (g_lcdc.disp.flags & 1) != 0) {
            // 8380
            sub_7DD8();
        }
        // 8238
        *(int*)(0xBE1401A0) = g_lcdc.disp.flags & 0xFE;
        *(int*)(0xBE140180) = g_lcdc.hwData[0];
        *(int*)(0xBE140184) = g_lcdc.hwData[1];
        *(int*)(0xBE140188) = g_lcdc.hwData[2];
        *(int*)(0xBE14018C) = g_lcdc.hwData[3];
        *(int*)(0xBE140190) = g_lcdc.hwData[4];
        *(int*)(0xBE140194) = g_lcdc.hwData[5];
        *(int*)(0xBE140198) = g_lcdc.hwData[6];
    }
    // 82A0
    g_lcdc.hwCtl->v2_8   = g_lcdc.v3_28;
    g_lcdc.hwCtl->xbPrch = g_lcdc.disp.x.bPrch;
    g_lcdc.hwCtl->xsync  = g_lcdc.disp.x.sync;
    g_lcdc.hwCtl->xfPrch = g_lcdc.disp.x.fPrch;
    g_lcdc.hwCtl->xres   = g_lcdc.disp.x.res;
    g_lcdc.hwCtl->ybPrch = g_lcdc.disp.y.bPrch;
    g_lcdc.hwCtl->ysync  = g_lcdc.disp.y.sync;
    g_lcdc.hwCtl->yfPrch = g_lcdc.disp.y.fPrch;
    g_lcdc.hwCtl->yres   = g_lcdc.disp.y.res;
    g_lcdc.hwCtl->shiftY = g_lcdc.shiftY;
    g_lcdc.hwCtl->shiftX = g_lcdc.shiftX;
    g_lcdc.hwCtl->realX  = g_lcdc.realX;
    g_lcdc.hwCtl->realY  = g_lcdc.realY;
    if (g_lcdc.mobo > 0x7FFFFF)
        *(int*)(0xBE1401A0) = g_lcdc.disp.flags;
    // 8330
    if (id == 14)
    {
        // 8374
        g_lcdc.v3_8 = 0;
        g_lcdc.hwCtl->v2_112 = 0;
    }
    // 8338
    sceKernelCpuResumeIntr(oldIntr);
    return 0;
}

// 1B7A47F5
int sceLcdcGetMode(int *arg0, int *arg1, int *arg2, int *arg3)
{
    if (arg0 != NULL)
        *arg0 = g_lcdc.dispId;
    // 8630
    if (arg1 != NULL)
        *arg1 = g_lcdc.scaledXres / g_lcdc.scale;
    // 8654
    if (arg2 != NULL)
        *arg2 = g_lcdc.yres;
    // 8664
    if (arg3 != NULL)
        *arg3 = g_lcdc.v3_28;
    return 0;
}

// 362939F0
int sceLcdcReset()
{
    int oldIntr = sceKernelCpuSuspendIntr();
    int old = g_lcdc.hwCtl->v2_0;
    g_lcdc.hwCtl->v2_0 = old & 0xFFFFFFFE;
    if (g_lcdc.mobo > 0x7FFFFF)
    {
        // wait 64ms?
        // 86FC
        int first = ThreadManForKernel_369ED59D();
        // 8708
        while (ThreadManForKernel_369ED59D() - first < 64)
            ;
    }
    // 86D0
    g_lcdc.hwCtl->v2_0 = old;
    sceKernelCpuResumeIntr(oldIntr);
    return 0;
}

// 17C9E4D7
int sceLcdcInsertDisplay_default(int *arg0, int *arg1, int *arg2, int *arg3, int *arg4)
{
    if (arg0 != NULL)
        *arg0 = g_lcdc.disp.y.fPrch;
    // 8738
    if (arg1 != NULL)
        *arg1 = g_lcdc.disp.y.bPrch;
    // 8748
    if (arg2 != NULL)
        *arg2 = g_lcdc.disp.y.sync;
    // 8758
    if (arg3 != NULL)
        *arg3 = g_lcdc.disp.y.fPrch + g_lcdc.disp.y.bPrch + g_lcdc.disp.y.sync;
    // 877C
    if (arg4 != NULL)
        *arg4 = g_lcdc.disp.y.fPrch + g_lcdc.disp.y.bPrch + g_lcdc.disp.y.sync + g_lcdc.disp.y.res;
    return 0;
}

// CDC84CB0
int sceLcdcReadHPC()
{
    int oldIntr = sceKernelCpuSuspendIntr();
    int i = 0;
    i++;
    // 87DC
    int ret;
    do
    {
        ret = -1;
        if (i >= 101)
            break;
        i++;
        ret = g_lcdc.hwCtl->hpc;
    } while (ret != g_lcdc.hwCtl->hpc);
    // 87F8
    sceKernelCpuResumeIntr(oldIntr);
    return ret;
}

// 697BDBF2
int sceLcdcReadVPC()
{
    int oldIntr = sceKernelCpuSuspendIntr();
    int i = 0;
    i++;
    // 8840
    int ret;
    do
    {
        ret = -1;
        if (i >= 4)
            break;
        i++;
        ret = g_lcdc.hwCtl->vpc;
    } while (ret != g_lcdc.hwCtl->vpc);
    // 885C
    sceKernelCpuResumeIntr(oldIntr);
    return ret;
}

// 887C
int _sceLcdcModuleStart()
{
    sceLcdcInit();
    return 0;
}

// 889C
int sceLcdcEnd()
{
    sceKernelUnregisterSysEventHandler(&lcdcEvent);
    return 0;
}

// 3652E6A4
int sceLcdcEnable()
{
    sub_0A80();
    g_lcdc.v3_0 = 1;
    return 0;
}

// B75F6DE4
int sceLcdcDisable()
{
    sub_0A8C();
    g_lcdc.v3_0 = 0;
    return 0;
}

int sceLcdc_driver_AF7A82E6(int arg) // only in new firmwares (= only randomized NID)
{
    if (arg >= 9)
        return 0x80000107;
    g_lcdc.tabId = arg;
    return 0;
}

int sceLcdc_driver_AB309648(int arg) // only in new firmwares (= only randomized NID)
{
    if (arg >= 3)
        return 0x80000107;
    g_lcdc.v3_88 = arg;
    return 0;
}

// E3BD61DA
int sceLcdcSetHOffset(int arg0, int arg1)
{
    int oldIntr = sceKernelCpuSuspendIntr();
    int diff = arg1 - g_lcdc.v3_64;
    g_lcdc.v3_64 = arg1;
    if (g_lcdc.dispId != 0 && g_lcdc.disp.syncDiff == 0)
    {
        int m = diff * g_lcdc.scale;
        int d1 = g_lcdc.disp.x.fPrch - m;
        int d2 = g_lcdc.disp.x.sync + m;
        g_lcdc.hwCtl->xfPrch = d1 & 0xFFFF;
        g_lcdc.disp.x.sync = d2;
        g_lcdc.hwCtl->xres = g_lcdc.disp.x.res;
        g_lcdc.disp.x.fPrch = d1;
    }
    // 89D4
    sceKernelCpuResumeIntr(oldIntr);
    return 0;
}

// 193609C8
float sceLcdcGetLcdcClockFreq()
{
    return g_lcdc.disp.sync * (float)g_lcdc.scale;
}

// 11DE58F3
int sceLcdcGetCyclesPerPixel()
{
    return g_lcdc.scale;
}

// 73E048CF
float sceLcdcGetPixelClockFreq()
{
    return g_lcdc.disp.sync;
}

// 7683D978
float sceLcdcGetHsyncFreq()
{
    float numPix = g_lcdc.disp.x.fPrch + g_lcdc.disp.x.bPrch + g_lcdc.disp.x.sync + g_lcdc.disp.x.res;
    return (g_lcdc.disp.sync * (float)g_lcdc.scale) / numPix;
}

// F9DED56
float sceLcdcGetVsyncFreq()
{
    int x = g_lcdc.disp.x.fPrch + g_lcdc.disp.x.bPrch + g_lcdc.disp.x.sync + g_lcdc.disp.x.res;
    int y = g_lcdc.disp.y.fPrch + g_lcdc.disp.y.bPrch + g_lcdc.disp.y.sync + g_lcdc.disp.y.res;
    if ((g_lcdc.disp.flags & 5) == 5)
        x >>= 1;
    return (g_lcdc.disp.sync * (float)g_lcdc.scale) / (float)(x * y);
}

// 92A62D9B
int sceLcdcReadUnderflow()
{
    int v = g_lcdc.hwCtl->v2_80;
    if (v == 1)
        g_lcdc.hwCtl->v2_80 = v;
    return v;
}

// 1630642D
int sceLcdc_driver_1630642D() // in new firmwares (= only randomized NID)
{
    int v = *(int*)(0xBE700004);
    if (v != *(int*)(0xBE700004))
        return sceLcdc_driver_1630642D();
    return (v ^ 1) & 1;
}

