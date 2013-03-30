/* Copyright (C) 2011, 2012, 2013 The uOFW team
   See the file COPYING for copying permission.
*/

#include "clockgen_int.h"

#include <sysmem_sysevent.h>
#include <threadman_kernel.h>

SCE_MODULE_INFO("sceClockgen_Driver", 0x1007, 1, 9);
SCE_MODULE_BOOTSTART("_sceClockgenModuleStart");
SCE_MODULE_REBOOT_BEFORE("_sceClockgenModuleRebootBefore");
SCE_SDK_VERSION(SDK_VERSION);

#define LEPTON_CLOCK    (1 << 4) //8
#define AUDIO_CLOCK     (1 << 5) //16

s32 sceI2cMasterTransmitReceive(u32, u8 *, s32, u32, u8 *, s32);
s32 sceI2cMasterTransmit(u32, u8 *, s32);
s32 sceI2cSetClock(s32, s32);

//0x000008F0
typedef struct {
    s32 mutex;        //0
    u32 protocol;     //4
    s8 reg[6];        //8
    u16 padding;      //E
} ClockGenContext;

ClockGenContext g_Cy27040 = {
    -1,
    0,
    {0, 0, 0, 0, 0, 0},
    0
};

//0x00000900
SceSysEventHandler g_ClockGenSysEv = {
    sizeof(SceSysEventHandler),
    "SceClockgen",
    0x00FFFF00,
    _sceClockgenSysEventHandler,
    0,
    0,
    NULL,
    {0, 0, 0, 0, 0, 0, 0, 0, 0}
};

//0x00000000
s32 sceClockgenSetup() //sceClockgen_driver_50F22765
{
    u8 table[16];
    u8 back[16];
    s32 ret;
    s32 i;

    if (g_Cy27040.protocol != 0) {
        for (i = 0; i < 3; i++) {
            table[0] = i - 128;

            //sceI2c_driver_47BDEAAA
            ret = sceI2cMasterTransmitReceive(210, table, 1, 210, back, 1);

            if (ret < 0) {
                return ret;
            }

            g_Cy27040.reg[i + 0] = back[0];
            g_Cy27040.reg[i + 3] = back[0];
        }
    }
    else {
        table[0] = 0;

        //sceI2c_driver_47BDEAAA
        ret = sceI2cMasterTransmitReceive(210, table, 1, 210, back, 16);

        if (ret < 0) {
            return ret;
        }

        if (back[0] == 0) {
            return SCE_ERROR_OK;
        }

        for (i = 0; (i < 3) && (i < back[0]); i++) {
            g_Cy27040.reg[i + 0] = back[i + 1];
            g_Cy27040.reg[i + 3] = back[i + 1];
        }
    }

    return SCE_ERROR_OK;
}

//0x000000F8
s32 sceClockgenSetSpectrumSpreading(s32 arg) //sceClockgen_driver_C9AF3102
{
    u32 reg_value;
    u32 ret_value;
    s32 res;

    if (arg < 0) {
        reg_value = g_Cy27040.reg[5] & 0x7;
        ret_value = arg;

        if ((g_Cy27040.reg[2] & 0x7) == reg_value) {
            return ret_value;
        }
    }
    else {
        switch(g_Cy27040.reg[0] & 0xF) {
        case 0x8:
        case 0xF:
        case 0x7:
        case 0x3:
        case 0x9:
        case 0xA:
            if (arg < 2) {
                reg_value = 0;
                ret_value = 0;
            }
            else if (arg < 7) {
                reg_value = 1;
                ret_value = 5;
            }
            else if (arg < 15) {
                reg_value = 2;
                ret_value = 10;
            }
            else if (arg < 25) {
                reg_value = 4;
                ret_value = 20;
            }
            else {
                reg_value = 6;
                ret_value = 30;
            }
            break;
        case 0x4:
            if (arg < 2) {
                reg_value = 0;
                ret_value = 0;
            }
            else if (arg < 7) {
                reg_value = 1;
                ret_value = 5;
            }
            else if (arg < 12) {
                reg_value = 2;
                ret_value = 10;
            }
            else if (arg < 17) {
                reg_value = 3;
                ret_value = 15;
            }
            else if (arg < 22) {
                reg_value = 4;
                ret_value = 20;
            }
            else if (arg < 27) {
                reg_value = 5;
                ret_value = 25;
            }
            else {
                reg_value = 6;
                ret_value = 30;
            }
            break;
        default:
            return 0x80000004;
        }
    }

    res = sceKernelLockMutex(g_Cy27040.mutex, 1, 0);
    if (res < 0) {
        return res;
    }

    g_Cy27040.reg[2] = (reg_value & 7) | (g_Cy27040.reg[2] & ~7);
    res = _cy27040_write_register(2, g_Cy27040.reg[2]);

    sceKernelUnlockMutex(g_Cy27040.mutex, 1);
    if (res < 0) {
        return res;
    }

    return ret_value;
}

//0x000002B4
s32 _sceClockgenModuleStart(SceSize args __attribute__((unused)), void *argp __attribute__((unused)))
{
    s32 mutexId;

    //sceI2c_driver_62C7E1E4
    sceI2cSetClock(4, 4);

    //ThreadManForKernel_B7D098C6
    mutexId = sceKernelCreateMutex("SceClockgen", 1, 0, 0);

    if (mutexId >= 0) {
        g_Cy27040.mutex = mutexId;

        //sceSysEventForKernel_CD9E4BB5
        sceKernelRegisterSysEventHandler(&g_ClockGenSysEv);
    }

    g_Cy27040.protocol = 1;
    sceClockgenSetup();

    return SCE_ERROR_OK;
}

//0x00000320
s32 _sceClockgenModuleRebootBefore(SceSize args __attribute__((unused)), void *argp __attribute__((unused)))
{
    u32 reg5;
    u32 reg2;

    reg5 = g_Cy27040.reg[5];
    reg2 = g_Cy27040.reg[2];

    if ((reg2 & 7) != (reg5 & 7)) {
        _cy27040_write_register(2, (reg2 & ~7) | (reg5 & 7));
    }

    if (g_Cy27040.mutex >= 0) {
        //ThreadManForKernel_F8170FBE
        sceKernelDeleteMutex(g_Cy27040.mutex);

        g_Cy27040.mutex = -1;
    }

    //sceSysEventForKernel_D7D3FDCD
    sceKernelUnregisterSysEventHandler(&g_ClockGenSysEv);

    return SCE_ERROR_OK;
}

//0x00000398
s32 sceClockgenInit() //sceClockgen_driver_29160F5D
{
    s32 mutexId;

    sceI2cSetClock(4, 4);
    mutexId = sceKernelCreateMutex("SceClockgen", 1, 0, 0);

    if (mutexId < 0) {
        return mutexId;
    }

    g_Cy27040.mutex = mutexId;

    sceKernelRegisterSysEventHandler(&g_ClockGenSysEv);

    return SCE_ERROR_OK;
}

//0x000003EC
s32 sceClockgenEnd() //sceClockgen_driver_36F9B49D
{
    if (g_Cy27040.mutex >= 0) {
        sceKernelDeleteMutex(g_Cy27040.mutex);

        g_Cy27040.mutex = -1;
    }

    sceKernelUnregisterSysEventHandler(&g_ClockGenSysEv);

    return SCE_ERROR_OK;
}

//0x00000438
void sceClockgenSetProtocol(u32 prot) //sceClockgen_driver_3F6B7C6B
{
    g_Cy27040.protocol = prot;
}

//0x00000448
s32 sceClockgenGetRevision() //sceClockgen_driver_CE36529C
{
    return g_Cy27040.reg[0];
}

//0x00000454
s32 sceClockgenGetRegValue(u32 idx) //sceClockgen_driver_0FD28D8B
{
    if (idx >= 3) {
        return g_Cy27040.reg[idx];
    }

    return 0x80000102;
}

//0x0000047C
s32 sceClockgenAudioClkSetFreq(u32 freq) //sceClockgen_driver_DAB6E612
{
    if (freq == 44100) {
        return _sceClockgenSetControl1(1, 0);
    }
    else if (freq == 48000) {
        return _sceClockgenSetControl1(1, 1);
    }

    return 0x800001FE;
}

//0x000004BC
s32 sceClockgenAudioClkEnable() //sceClockgen_driver_A1D23B2C
{
    return _sceClockgenSetControl1(AUDIO_CLOCK, 1);
}

//0x000004DC
s32 sceClockgenAudioClkDisable() //sceClockgen_driver_DED4C698
{
    return _sceClockgenSetControl1(AUDIO_CLOCK, 0);
}

//0x000004FC
s32 sceClockgenLeptonClkEnable()  //sceClockgen_driver_7FF82F6F
{
    return _sceClockgenSetControl1(LEPTON_CLOCK, 1);
}

//0x0000051C
s32 sceClockgenLeptonClkDisable() //sceClockgen_driver_DBE5F283
{
    return _sceClockgenSetControl1(LEPTON_CLOCK, 0);
}

//0x0000053C
s32 _sceClockgenSysEventHandler( // FIXME
    s32 ev_id,
    char *ev_name __attribute__((unused)),
    void *param __attribute__((unused)),
    s32 *result __attribute__((unused)))
{
    if (ev_id == 0x10000) {
        //ThreadManForKernel_0DDCD2C9
        sceKernelTryLockMutex(g_Cy27040.mutex, 1);

        return SCE_ERROR_OK;
    }

    if ((0x10000 < ev_id) == 0) {
        return SCE_ERROR_OK;
    }

    if (ev_id == 0x100000) {
        g_Cy27040.reg[1] &= 0xFFFFFFF7;

        _cy27040_write_register(1, g_Cy27040.reg[1] & 0xF7);
        _cy27040_write_register(2, g_Cy27040.reg[2]);

        sceKernelUnlockMutex(g_Cy27040.mutex, 1);

        return SCE_ERROR_OK;
    }

    return SCE_ERROR_OK;
}

//0x000005DC
s32 _sceClockgenSetControl1(s32 bus, s32 mode)
{
    s32 ret;
    s32 ret2;
    s32 val;
    s32 val2;

    //ThreadManForKernel_B011B11F
    ret = sceKernelLockMutex(g_Cy27040.mutex, 1, 0);

    if (ret < 0) {
        return ret;
    }

    ret2 = g_Cy27040.reg[1] & bus;
    val = g_Cy27040.reg[1] | bus;
    val2 = g_Cy27040.reg[1] & ~bus;

    if (ret2 > 0) {
        ret = ret2;
    }

    if (mode != 0) {
        val2 = val & 0xFF;
    }

    if (val2 != g_Cy27040.reg[1]) {
        g_Cy27040.reg[1] = val2;

        _cy27040_write_register(1, val2);
    }

    //ThreadManForKernel_6B30100F
    sceKernelUnlockMutex(g_Cy27040.mutex, 1);

    return ret;
}

//0x00000680
s32 _cy27040_write_register(u8 regid, u8 val)
{
    u8 table[16];

    table[0] = regid - 128;
    table[1] = val;

    if (regid < 3) {
        s32 ret;

        //sceI2c_driver_8CBD8CCF
        ret = sceI2cMasterTransmit(210, table, 2);

        // SCE_ERROR_OK?
        if (ret < 0) {
            return ret;
        }
    }

    return 0x80000102;
}
