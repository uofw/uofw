/* Copyright (C) 2011, 2012, 2013 The uOFW team
   See the file COPYING for copying permission.
*/

/*
 * uOFW/trunk/src/clockgen/clockgen.c
 *
 * sceClockgen_Driver - a driver for the PSP's CY27040 clock generator.
 *
 * The clockgen driver main function is to supply users with a simple way
 * to enable/disable audio and lepton (a DSP managing the UMD reader).
 *
 * The CY27040 clock generator is managed using three hardware registers:
 *       reg0: revision
 *       reg1: clock control (audio state, lepton state)
 *       reg2: spread-spectrum setting
 * Their current state is stored in ClockgenContext.curReg.
 * Also, their initial state is stored in ClockgenContext.oldReg.
 * These three registers are accessed through the PSP's I2C bus.
 *
 */

#include "clockgen_int.h"

#include <sysmem_sysevent.h>
#include <threadman_kernel.h>
#include <lowio_i2c.h>

SCE_MODULE_INFO(
    "sceClockgen_Driver",
    SCE_MODULE_KERNEL |
    SCE_MODULE_ATTR_CANT_STOP | SCE_MODULE_ATTR_EXCLUSIVE_LOAD | SCE_MODULE_ATTR_EXCLUSIVE_START,
    1, 9
);
SCE_MODULE_BOOTSTART("_sceClockgenModuleStart");
SCE_MODULE_REBOOT_BEFORE("_sceClockgenModuleRebootBefore");
SCE_SDK_VERSION(SDK_VERSION);

/* The Cy27040's slave address used to communicate through the I2C bus */
#define PSP_CY27040_I2C_ADDR        (0xD2)

/* Retrieve all registers command. */
#define PSP_CY27040_CMD_ALL_REGS    (0)
/* Retrieve a register command. */
#define PSP_CY27040_CMD_REG(i)      ((i) - 128)

/* reg0: revision */
#define PSP_CY27040_REG_REVISION    (0)
/* reg1: clock control */
#define PSP_CY27040_REG_CLOCK       (1)
/* reg2: spread-spectrum control */
#define PSP_CY27040_REG_SS          (2)
/* Registers count */
#define PSP_CY27040_REG_COUNT       (3)

/* In reg1, used to manage the audio frequency */
#define PSP_CLOCK_AUDIO_FREQ    (1)
/* In reg1, used to enable/disable the lepton DSP */
#define PSP_CLOCK_LEPTON        (8)
/* In reg1, used to enable/disable audio */
#define PSP_CLOCK_AUDIO         (16)

//0x000008F0
typedef struct {
    /* The mutex id */
    SceUID mutex;
    /* Current protocol mode */
    SceBool protocol;
    /* Current state of registers */
    s8 curReg[PSP_CY27040_REG_COUNT];
    /* Initial state of registers */
    s8 oldReg[PSP_CY27040_REG_COUNT];
    /* Padding */
    u16 padding;
} ClockgenContext;

ClockgenContext g_Cy27040 = {
    .mutex = -1,
    .protocol = 0,
    .curReg = {0},
    .oldReg = {0}
};

//0x00000900
SceSysEventHandler g_ClockGenSysEv = {
    .size = sizeof(SceSysEventHandler),
    .name = "SceClockgen",
    .typeMask = 0x00FFFF00,
    .handler = _sceClockgenSysEventHandler,
    .gp = 0,
    .busy = 0,
    .next = NULL,
    .reserved = {0}
};

//0x00000000
s32 sceClockgenSetup() //sceClockgen_driver_50F22765
{
    u8 trsm[16];
    u8 recv[16];
    s32 ret;
    s32 i;

    /*
     * Retrieve the state of the hardware registers and
     * store them in curReg and oldReg.
     */

    if (g_Cy27040.protocol != 0) {
        /* One register at a time */

        for (i = 0; i < PSP_CY27040_REG_COUNT; i++) {
            trsm[0] = PSP_CY27040_CMD_REG(i);

            /* Send and receive data through the I2C bus. */

            //sceI2c_driver_47BDEAAA
            ret = sceI2cMasterTransmitReceive(
                PSP_CY27040_I2C_ADDR, trsm, 1,
                PSP_CY27040_I2C_ADDR, recv, 1
            );

            if (ret < 0) {
                return ret;
            }

            g_Cy27040.curReg[i] = recv[0];
            g_Cy27040.oldReg[i] = recv[0];
        }
    }
    else {
        /* All registers in a single command */

        trsm[0] = PSP_CY27040_CMD_ALL_REGS;

        /* Send and receive data through the I2C bus. */

        //sceI2c_driver_47BDEAAA
        ret = sceI2cMasterTransmitReceive(
            PSP_CY27040_I2C_ADDR, trsm, 1,
            PSP_CY27040_I2C_ADDR, recv, 16
        );

        if (ret < 0) {
            return ret;
        }

        for (i = 0; (i < PSP_CY27040_REG_COUNT) && (i < recv[0]); i++) {
            g_Cy27040.curReg[i] = recv[i + 1];
            g_Cy27040.oldReg[i] = recv[i + 1];
        }
    }

    return SCE_ERROR_OK;
}

//0x000000F8
s32 sceClockgenSetSpectrumSpreading(s32 mode) //sceClockgen_driver_C9AF3102
{
    s32 regSS;
    s32 ret;
    s32 res;

    if (mode < 0) {
        /* Invalid argument, try to restore the initial spread-spectrum state. */

        regSS = g_Cy27040.oldReg[PSP_CY27040_REG_SS] & 0x7;
        ret = mode;

        if (regSS == (g_Cy27040.curReg[PSP_CY27040_REG_SS] & 0x7)) {
            return ret;
        }
    }
    else {
        /* Choose a value according to the Cy27040 revision. */

        switch(g_Cy27040.curReg[PSP_CY27040_REG_REVISION] & 0xF) {
        case 0x8:
        case 0xF:
        case 0x7:
        case 0x3:
        case 0x9:
        case 0xA:
            if (mode < 2) {
                regSS = 0;
                ret = 0;
            }
            else if (mode < 7) {
                regSS = 1;
                ret = 5;
            }
            else if (mode < 15) {
                regSS = 2;
                ret = 10;
            }
            else if (mode < 25) {
                regSS = 4;
                ret = 20;
            }
            else {
                regSS = 6;
                ret = 30;
            }
            break;
        case 0x4:
            if (mode < 2) {
                regSS = 0;
                ret = 0;
            }
            else if (mode < 7) {
                regSS = 1;
                ret = 5;
            }
            else if (mode < 12) {
                regSS = 2;
                ret = 10;
            }
            else if (mode < 17) {
                regSS = 3;
                ret = 15;
            }
            else if (mode < 22) {
                regSS = 4;
                ret = 20;
            }
            else if (mode < 27) {
                regSS = 5;
                ret = 25;
            }
            else {
                regSS = 6;
                ret = 30;
            }
            break;
        default:
            return SCE_ERROR_NOT_SUPPORTED;
        }
    }

    /* Try to update the spread-spectrum register. */

    res = sceKernelLockMutex(g_Cy27040.mutex, 1, 0);
    if (res < 0) {
        return res;
    }

    g_Cy27040.curReg[PSP_CY27040_REG_SS] = (regSS & 7) | (g_Cy27040.curReg[PSP_CY27040_REG_SS] & ~7);
    res = _cy27040_write_register(PSP_CY27040_REG_SS, g_Cy27040.curReg[PSP_CY27040_REG_SS]);

    sceKernelUnlockMutex(g_Cy27040.mutex, 1);
    if (res < 0) {
        return res;
    }

    return ret;
}

//0x000002B4
s32 _sceClockgenModuleStart(SceSize args __attribute__((unused)), void *argp __attribute__((unused)))
{
    sceClockgenInit();

    g_Cy27040.protocol = 1;
    sceClockgenSetup();

    return SCE_ERROR_OK;
}

//0x00000320
s32 _sceClockgenModuleRebootBefore(SceSize args __attribute__((unused)), void *argp __attribute__((unused)))
{
    s32 oldRegSS;
    s32 curRegSS;

    oldRegSS = g_Cy27040.oldReg[PSP_CY27040_REG_SS];
    curRegSS = g_Cy27040.curReg[PSP_CY27040_REG_SS];

    /* Spread-spectrum mode changed since its initial state, restore it */

    if ((curRegSS & 7) != (oldRegSS & 7)) {
        _cy27040_write_register(PSP_CY27040_REG_SS, (curRegSS & ~7) | (oldRegSS & 7));
    }

    sceClockgenEnd();

    return SCE_ERROR_OK;
}

//0x00000398
s32 sceClockgenInit() //sceClockgen_driver_29160F5D
{
    s32 mutexId;

    /* Set the I2C bus speed */

    //sceI2c_driver_62C7E1E4
    sceI2cSetClock(4, 4);

    /* Create the mutex and register the sysevent handler */

    //ThreadManForKernel_B7D098C6
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
    /* Delete the mutex and unregister the sysevent handler */

    if (g_Cy27040.mutex >= 0) {
        sceKernelDeleteMutex(g_Cy27040.mutex);

        g_Cy27040.mutex = -1;
    }

    sceKernelUnregisterSysEventHandler(&g_ClockGenSysEv);

    return SCE_ERROR_OK;
}

//0x00000438
s32 sceClockgenSetProtocol(u32 prot) //sceClockgen_driver_3F6B7C6B
{
    g_Cy27040.protocol = prot;

    return SCE_ERROR_OK;
}

//0x00000448
s32 sceClockgenGetRevision() //sceClockgen_driver_CE36529C
{
    return g_Cy27040.curReg[PSP_CY27040_REG_REVISION];
}

//0x00000454
s32 sceClockgenGetRegValue(u32 idx) //sceClockgen_driver_0FD28D8B
{
    if (idx >= PSP_CY27040_REG_COUNT) {
        return SCE_ERROR_INVALID_INDEX;
    }

    return g_Cy27040.curReg[idx];
}

//0x0000047C
s32 sceClockgenAudioClkSetFreq(u32 freq) //sceClockgen_driver_DAB6E612
{
    /* Lower sample rates are supported because they are divisible by these */

    if (freq == 44100) {
        return _sceClockgenSetControl1(PSP_CLOCK_AUDIO_FREQ, 0);
    }
    else if (freq == 48000) {
        return _sceClockgenSetControl1(PSP_CLOCK_AUDIO_FREQ, 1);
    }

    return SCE_ERROR_INVALID_VALUE;
}

//0x000004BC
s32 sceClockgenAudioClkEnable() //sceClockgen_driver_A1D23B2C
{
    return _sceClockgenSetControl1(PSP_CLOCK_AUDIO, 1);
}

//0x000004DC
s32 sceClockgenAudioClkDisable() //sceClockgen_driver_DED4C698
{
    return _sceClockgenSetControl1(PSP_CLOCK_AUDIO, 0);
}

//0x000004FC
s32 sceClockgenLeptonClkEnable()  //sceClockgen_driver_7FF82F6F
{
    return _sceClockgenSetControl1(PSP_CLOCK_LEPTON, 1);
}

//0x0000051C
s32 sceClockgenLeptonClkDisable() //sceClockgen_driver_DBE5F283
{
    return _sceClockgenSetControl1(PSP_CLOCK_LEPTON, 0);
}

//0x0000053C
s32 _sceClockgenSysEventHandler(
    s32 ev_id,
    char *ev_name __attribute__((unused)),
    void *param __attribute__((unused)),
    s32 *result __attribute__((unused)))
{
    if (ev_id == 0x10000) {
        /* Sleep event? Spread-spectrum and clock control are blocked. */

        //ThreadManForKernel_0DDCD2C9
        sceKernelTryLockMutex(g_Cy27040.mutex, 1);

        return SCE_ERROR_OK;
    }

    if (ev_id < 0x10000) {
        /* Unknown event. May be used for debugging. */

        return SCE_ERROR_OK;
    }

    if (ev_id == 0x100000) {
        /* Resume event? Lepton is disabled. Other settings are restored and controls are unblocked. */

        g_Cy27040.curReg[PSP_CY27040_REG_CLOCK] &= ~PSP_CLOCK_LEPTON;

        _cy27040_write_register(PSP_CY27040_REG_CLOCK, g_Cy27040.curReg[PSP_CY27040_REG_CLOCK]);
        _cy27040_write_register(PSP_CY27040_REG_SS, g_Cy27040.curReg[PSP_CY27040_REG_SS]);

        sceKernelUnlockMutex(g_Cy27040.mutex, 1);
    }

    return SCE_ERROR_OK;
}

//0x000005DC
s32 _sceClockgenSetControl1(s32 bus, SceBool mode)
{
    s32 ret;
    s32 regClk;

    //ThreadManForKernel_B011B11F
    ret = sceKernelLockMutex(g_Cy27040.mutex, 1, 0);

    if (ret < 0) {
        return ret;
    }

    regClk = g_Cy27040.curReg[PSP_CY27040_REG_CLOCK];
    ret = (regClk & bus) > 0;

    if (!mode) {
        /* Force bit zero */

        regClk &= ~bus;
    }
    else {
        /* Force bit one */

        regClk |= bus;
    }

    /* Register has changed, update it. */

    if (regClk != g_Cy27040.curReg[PSP_CY27040_REG_CLOCK]) {
        g_Cy27040.curReg[PSP_CY27040_REG_CLOCK] = regClk;

        _cy27040_write_register(PSP_CY27040_REG_CLOCK, regClk);
    }

    //ThreadManForKernel_6B30100F
    sceKernelUnlockMutex(g_Cy27040.mutex, 1);

    return ret;
}

//0x00000680
s32 _cy27040_write_register(u8 idx, u8 val)
{
    u8 trsm[16];
    s32 ret;

    /* First byte: command, second byte: value */

    trsm[0] = PSP_CY27040_CMD_REG(idx);
    trsm[1] = val;

    if (idx >= PSP_CY27040_REG_COUNT) {
        return SCE_ERROR_INVALID_INDEX;
    }

    /* Send data through the I2C bus. */

    //sceI2c_driver_8CBD8CCF
    ret = sceI2cMasterTransmit(
        PSP_CY27040_I2C_ADDR, trsm, 2
    );

    if (ret < 0) {
        return ret;
    }

    return SCE_ERROR_OK;
}
