#include <common_header.h>
#include <sysmem_sysevent.h>
#include <sysmem_sysclib.h>
#include <modulemgr_init.h>
#include <syscon.h>
#include <threadman_kernel.h>

SCE_MODULE_INFO("sceRTC_Service", SCE_MODULE_KERNEL | SCE_MODULE_ATTR_CANT_STOP | SCE_MODULE_ATTR_EXCLUSIVE_LOAD | SCE_MODULE_ATTR_EXCLUSIVE_START, 1, 11);
SCE_SDK_VERSION(SDK_VERSION);

#define TICKS_PER_SECOND  1000000

// .data - 0x000046B0 - flags 0x0003
SceSysEventHandler rtc_handler = {
    .size = sizeof(SceSysEventHandler),
    .name = "SceRtc",
    .typeMask = 0x00FFFF00,
    .handler = 0,
    .gp = 0,
    .busy = 0,
    .next = NULL,
    .reserved = {0, 0, 0, 0, 0, 0, 0, 0, 0}
};

// .bss - 112/0x70 bytes
typedef struct rtcContext {
    u32 aUnk;           // 0x46F0
    u32 bUnk;           // 0x46F4
    u32 cUnk;           // 0x46F8
    u32 dUnk;           // 0x46FC
    u32 currPresent;    // 0x4700 - Current internal tick
    u32 securePresent;  // 0x4704 - Current secure tick
    // size flag?
    u32 tickCleared;    // 0x4708 - Internal tick state?
    u32 szStatus;       // 0x470C - Flag for size type?
    u32 upperCTick;     // 0x4730 - Upper current tick
    u32 lowerCTick;     // 0x4734 - Lower current tick
    u32 upperSTick;     // 0x4738 - Upper secure tick
    u32 lowerSTick;     // 0x473C - Lower secure tick
    u32 upperUTick;     // 0x4740 - Upper unknown tick
    u32 lowerUTick;     // 0x4744 - Lower unknown tick
	u32 upperUnk;		// 0x4748 - Upper unknown
	u32 lowerUnk;		// 0x474C - Lower unknown
    u64 *internalTick;  // 0x4750
    SceUID rtc_cb;      // 0x475C
} rtcContext;

rtcContext ctx;

// DONE! TODO: Fix pointers.
// Address 0x00000000
// arg1 - chunk/byte size
// arg2 - struct for RTC
s32 secondaryCallback (int size, u32 *unk, void *arg) {
    (void)arg;

    u32 s0 = unk[5];
    u32 *s1 = unk;

    if(size == 4 * 1024) {
        sceRtcGetCurrentTick((u64*)(s0 + 16));
        sceRtc_C2DDBEB5((u64*)(s0 + 24));

        // status/enum type?
        if(s1[1] == 0) {
            return SCE_ERROR_OK;
        } else if(sceKernelApplicationType() != SCE_INIT_APPLICATION_GAME) {
            return SCE_ERROR_OK;
        } else {
            sceRtc_C2DDBEB5(ctx.internalTick);
            sceRtc_7D1FBED3(0);
            return SCE_ERROR_OK;
        }
    }

    if((size < (4 * 1024) + 1) == 0) {
        if(size != 1024 * 1024) {
            return SCE_ERROR_OK;
        } else {
            sceRtc_driver_852255B8();
            u32 *t5 = *s1[1];
            u32 t4 = t5[1]; // next pointer, linked list?
            // k1 shifted?
            if(t4 == 0x80000000) {
                ctx.tickCleared = 0;
                sceRtc_7D1FBED3(ctx.internalTick);
            }

            if(ctx.szStatus == 1) {
                s1 = ctx.szStatus;
            } else {
                sceKernelNotifyCallback(ctx.rtc_cb, sceRtcIsAlarmed());
                ctx.szStatus = 0;
            }
        }
    } else if(size == 1024 / 2) {
            ctx.szStatus = 1;
    }

    return SCE_ERROR_OK;
}

// DONE!
// Address 0x00000114
void primaryCallback (void *arg) {
    if(ctx.rtc_cb < 0) {
        return;
    }

    if(ctx.szStatus == 0) {
        sceKernelNotifyCallback(ctx.rtc_cb, arg);
        return;
    }

    if(ctx.szStatus == 1) {
        ctx.szStatus = 2;
    }
}

// TODO: Finish. Fix values/sizeof
// Subroutine sceRtc_driver_852255B8 - Address 0x0000016C
// Exported in sceRtc_driver
s32 sceRtc_driver_852255B8(void) {
    //u32 s0[20];
    //int s1, s2, s3, s4, s5, s6, s7;
    // Secure RTC reset?
    sceSyscon_driver_EB277C88(8, 0x4758, sizeof(s32));

    return SCE_ERROR_OK;
}

// Subroutine sceRtc_driver_929620CE - Address 0x0000054C - Aliases: sceRtc_driver_9763C138
// Exported in sceRtc_driver
s32 sceRtc_driver_929620CE() {
    return SCE_ERROR_OK;

}

// Subroutine sceRtc_driver_17C26C00 - Address 0x00000858 - Aliases: sceRtc_driver_66054C2A
// Exported in sceRtc_driver
s32 sceRtcSetCurrentSecureTick() {
    return SCE_ERROR_OK;

}

// DONE! TODO: Fix dereference.
// Subroutine sceRtc_508BA64B - Address 0x00000A54 - Aliases: sceRtc_driver_508BA64B
// Exported in sceRtc
// Exported in sceRtc_driver
s32 sceRtc_508BA64B(u64 *tick) {
    u64 *oldTick = tick;
    int oldK1 = pspShiftK1();
    s32 res = SCE_ERROR_OK;
    s32 intr;

    if (oldK1 & (int)tick < 0) {
        res = SCE_ERROR_PRIV_REQUIRED;
    } else {
        intr = sceKernelCpuSuspendIntr();

        /* Validate pointer */
        if(oldTick == NULL) {
            ctx.tickCleared = 0;
            sceKernelCpuResumeIntr(intr);
            /* Validate upper and lower unknown tick values */
        } else if((u32)oldTick[0] | (u32)oldTick[1]) != 0) {
            s64 tWide = sceKernelGetSystemTimeWide();
            ctx.tickCleared = 0;
            ctx.upperUTick = oldTick[0] - tWide[0];
            ctx.lowerUTick = (oldTick[1] - tWide[1]) - (oldTick[0] < tWide[0]);
        }
        sceKernelCpuResumeIntr(intr);
    }

    pspSetK1(oldK1);
    return res;
}

// TODO: Finish
// Subroutine sceRtc_7D1FBED3 - Address 0x00000B28 - Aliases: sceRtc_driver_E09880CF
// Exported in sceRtc
// Exported in sceRtc_driver
s32 sceRtc_7D1FBED3(u64 *tick) {
    return SCE_ERROR_OK;
}

// DONE!
// Subroutine sceRtc_C2DDBEB5 - Address 0x00000D5C - Aliases:
// sceRtc_driver_366669D6
// Exported in sceRtc
// Exported in sceRtc_driver
s32 sceRtc_C2DDBEB5(u64 *tick) {
    s32 shiftedK1 = pspGetK1() << 11;
    u64 *oldTick = tick;
    s32 res = SCE_ERROR_OK;

    if(oldTick == NULL) {
        res = SCE_ERROR_INVALID_POINTER;
    } else if(shiftedK1 & (int)tick < 0) {
        res = SCE_ERROR_INVALID_POINTER;
    } else {
        (u32)oldTick[0] = ctx.upperUnk;
        (u32)oldTick[1] = ctx.lowerUnk;
    }

    return res;
}

// DONE!
// Subroutine sceRtc_81FCDA34 - Address 0x00000DA0 - Aliases: sceRtc_driver_CF76CFE5
// Exported in sceRtc
// Exported in sceRtc_driver
s32 sceRtcIsAlarmed(void) {
    u64 *tempTick;
    u32 s1 = 0;
    int oldK1 = pspShiftK1();
    s32 res = SCE_ERROR_OK;

    if((ctx.upperUnk | ctx.lowerUnk) != 0) {
        res = sceSysconIsAlarmed();
        /* The PSP alarm was triggered */
        if(res == 1) {
            sceRtcGetCurrentTick(tempTick);
            if((u32)tempTick[1] < ctx.lowerUnk) {
                res = SCE_ERROR_OK;
            } else if (ctx.lowerUnk != (u32)tempTick[1]){
                res = SCE_ERROR_OK;
            } else if ((u32)tempTick[0] < ctx.upperUnk) {
                res = SCE_ERROR_OK;
            } else {
                res = SCE_ERROR_OK;
            }
         }
    }

    pspSetK1(oldK1);
    return res;
}

// DONE! TODO: Fix this. Needs to be revised.
// Subroutine sceRtc_FB3B18CD - Address 0x00000E54 - Aliases: sceRtc_driver_530A903E
// Exported in sceRtc
// Exported in sceRtc_driver
s32 sceRtcRegisterCallback(SceUID id) {
    int oldK1 = pspShiftK1();
    u32 *oldID = id;
    s32 res = SCE_ERROR_OK;
    s16 temp = 2;
    SceSysmemUidCB **cbUID;
    s32 intr;

    if(sceKernelGetThreadmanIdType() != SCE_KERNEL_TMID_Callback) {
        res = SCE_ERROR_INVALID_ID;
    } else {
        SceUID uid = sceKernelGetUIDcontrolBlock(id, cbUID);
        a1 = pspGetK1() >> 31;
        a0 = 2;
        if(uid != SCE_ERROR_OK) {
            res = SCE_ERROR_PRIV_REQUIRED;
        } else {
            if((pspGetK1() >> 31) != 0) {
                temp = (u16)id[11] & 2);
            } else {
                res = SCE_ERROR_ALREADY;
            }

            intr = sceKernelCpuSuspendIntr();

            if(ctx.rtc_cb < 0) {
                ctx.rtc_cb = id;
                res = SCE_ERROR_OK;
                sceKernelNotifyCallback(oldID, sceRtcIsAlarmed());
            }

            sceKernelCpuResumeIntr(intr);
        }
    }

    pspSetK1(oldK1);
    return res;
}

// DONE! TODO: Fix dereference.
// Subroutine sceRtc_3F7AD767 - Address 0x00000F40 - Aliases: sceRtc_driver_3F7AD767
// Exported in sceRtc
// Exported in sceRtc_driver
s32 sceRtcGetCurrentTick(u64 *tick) {
    u64 *oldTick = tick;
    int oldK1 = pspShiftK1();
    s32 res;

    if(oldK1 & (int)tick < 0) {
        res = SCE_ERROR_PRIV_REQUIRED
        // is curr(Internal/Tick)Present?
    } else if(ctx.currPresent != 0) {
        s64 tWide = sceKernelGetSystemTimeWide();
        (u32)oldTick[0] = upperCTick + (u32)tWide[0];
        (u32)oldTick[1] = (lowerCTick + (u32)tWide[1]) + ((u32)oldTick[0] < (u32)tWide[0]);
        res = SCE_ERROR_OK;
    } else {
        (u32)oldTick[0] = 0xB3A14000;
        (u32)oldTick[1] = 0x00DDDEF8;
        res = SCE_ERROR_ERRNO_OPERATION_NOT_PERMITTED;
    }

    pspSetK1(oldK1);
    return res;
}

// DONE! TODO: Fix dereference.
// Subroutine sceRtc_driver_B44BDAED - Address 0x00000FFC - Aliases: sceRtc_driver_CEEF238F
// Exported in sceRtc
// Exported in sceRtc_driver
s32 sceRtcGetCurrentSecureTick(u64 *tick) {
    u64 *oldTick = tick;
    int oldK1 = pspShiftK1();
    s32 res;

    if(oldK1 & (int)tick < 0) {
        res = SCE_ERROR_PRIV_REQUIRED;
        // is currSecure(Internal/Tick)Present?
    } else if(ctx.securePresent != 0) {
        u64 tWide = sceKernelGetSystemTimeWide();
        (u32)oldTick[0] = upperSTick + (u32)tWide[0];
        (u32)oldTick[1] = (lowerSTick + (u32)tWide[1]) + ((u32)oldTick[0] < (u32)tWide[0]);
        res = SCE_ERROR_OK;
    } else {
        (u32)oldTick[0] = 0xB3A14000;
        (u32)oldTick[1] = 0x00DDDEF8;
        res = SCE_ERROR_ERRNO_OPERATION_NOT_PERMITTED;
    }

    pspSetK1(oldK1);
    return res;
}

// DONE! TODO: Fix dereference.
// Subroutine sceRtc_F5FCC995 - Address 0x000010B8 - Aliases: sceRtc_driver_ED15334F
// Exported in sceRtc
// Exported in sceRtc_driver
// sceClear(internal)Tick?
s32 sceRtc_F5FCC995(u64 *tick) {
    u64 *oldTick = tick;
    int oldK1 = pspShiftK1();
    s32 res;

    if(oldK1 & (int)tick < 0) {
        res = SCE_ERROR_PRIV_REQUIRED;
        // is curr/secure(Internal/Tick)cleared?
    } else if(ctx.tickCleared != 0) {
        u64 tWide = sceKernelGetSystemTimeWide();
        (u32)oldTick[0] = upperUTick + (u32)tWide[0];
        (u32)oldTick[1] = (lowerUTick + (u32)tWide[1]) + ((u32)oldTick[0] < (u32)tWide[0]);
        res = SCE_ERROR_OK;
    } else {
        (u32)oldTick[0] = 0;
        (u32)oldTick[1] = 0;
        res = SCE_ERROR_ERRNO_OPERATION_NOT_PERMITTED;
    }

    pspSetK1(oldK1);
    return res;
}

// TODO: Finish
// Subroutine sceRtc_203CEB0D - Address 0x0000116C - Aliases: sceRtc_driver_7C6E9610
// Exported in sceRtc
// Exported in sceRtc_driver
// Doesnt use stack at all.
s32 sceRtcGetLastReincarnatedTime(u64 *tick) {
    u64 *oldTick = tick;
    int oldK1 = pspShiftK1();
    s32 res;

    if(oldK1 & (int)tick < 0) {
        res = SCE_ERROR_PRIV_REQUIRED;
    } else if(ctx.tickCleared == 0) {

    }

    pspSetK1(oldK1);
    return res;
}

// Subroutine sceRtc_62685E98 - Address 0x00001250 - Aliases: sceRtc_driver_E98FEC46
// Exported in sceRtc
// Exported in sceRtc_driver
s32 sceRtcGetLastAdjustedTime() {
    return SCE_ERROR_OK;
}

// DONE!
// Subroutine module_start - Address 0x00001308
s32 module_start(s32 argc, void *argp) {
    (void)argc;
    (void)argp;

    rtcContext *rtc = (rtcContext *)ctx;

    /* Initialize the data. */
    memset(rtc, 0, sizeof(RTC));
    ctx.rtc_cb = -1;

    sceRtc_driver_852255B8();
    sub_03BCC ();
    sceSysconSetsecondaryCallback(secondaryCallback, NULL);
    sceKernelRegisterSysEventHandler(&rtc_handler);

    /* Check to see if application is a PSP game. */
    if(sceKernelApplicationType() == SCE_INIT_APPLICATION_GAME) {
        sceRtc_7D1FBED3();
    }

    return SCE_ERROR_OK;
}

// DONE!
// Subroutine module_reboot_before - Address 0x000013AC
s32 module_reboot_before(s32 argc, void *argp) {
    (void)argc;
    (void)argp;

    sceRtc_7D1FBED3(0);
    sceSysconSetsecondaryCallback(primaryCallback, NULL);
    sceKernelUnregisterSysEventHandler(&rtc_handler);

    return SCE_ERROR_OK;
}

// DONE!
// Subroutine sceRtc_driver_912BEE56 - Address 0x000013E4
// Exported in sceRtc_driver
s32 sceRtcInit(void) {
    rtcContext *rtc = (rtcContext *)ctx;

    /* Initialize the data. */
    memset(rtc, 0, sizeof(RTC));
    ctx.rtc_cb = -1;

    sceRtc_driver_852255B8();
    sub_03BCC ();
    sceSysconSetsecondaryCallback(primaryCallback, NULL);
    sceKernelRegisterSysEventHandler(&rtc_handler);

    return SCE_ERROR_OK;
}

// DONE!
// Subroutine sceRtc_driver_CE27DE2F - Address 0x00001464
// Exported in sceRtc_driver
s32 sceRtcEnd(void) {
    sceSysconSetsecondaryCallback(secondaryCallback, NULL);
    sceKernelUnregisterSysEventHandler(&rtc_handler);
    return SCE_ERROR_OK;
}

// DONE! - Guess it was never implemented :(
// Subroutine sceRtc_driver_9CC2797E - Address 0x00001494
// Exported in sceRtc_driver
s32 sceRtcSuspend(void) {
    return SCE_ERROR_OK;
}

// DONE!
// Subroutine sceRtc_driver_48D07D70 - Address 0x0000149C
// Exported in sceRtc_driver
s32 sceRtcResume(void) {
    sceRtc_driver_852255B8();
    return SCE_ERROR_OK;
}

// DONE!
// Subroutine sceRtc_driver_1C1859DF - Address 0x000014BC
// Exported in sceRtc_driver
//sceRtc(reset/set/clear)(flags/(upper/lower)ticks)?
void sceRtc_driver_1C1859DF(void) {
    ctx.tickCleared = 0;
    ctx.currPresent = 0;
    ctx.securePresent = 0;
}

// DONE!
// Subroutine sceRtc_driver_DFF30673 - Address 0x000014D8
// Exported in sceRtc_driver
//sceRtc(set)(flags/(upper/lower)ticks)?
s32 sceRtc_driver_DFF30673(u32 a0, u32 a1, u32 a2, u32 a3) {
    u32 res = sceKernelCpuSuspendIntr();
    ctx.cUnk = a3;
    ctx.bUnk = a1;
    ctx.dUnk = a2;
    ctx.aUnk = a0;
    sceKernelCpuResumeIntr(res);
    return SCE_ERROR_OK;
}

// DONE!
// Subroutine sceRtc_6A676D2D - Address 0x00001544 - Aliases: sceRtc_driver_7D8E37E1
// Exported in sceRtc
// Exported in sceRtc_driver
s32 sceRtcUnregisterCallback(SceUID cb) {
    s32 res = SCE_ERROR_NOT_FOUND;
    if(ctx.rtc_cb == cb) {
        ctx.rtc_cb = -1;
        res = SCE_ERROR_OK;
    }

    return res;
}

// DONE! TODO: Revise and verify.
// Subroutine sceRtc_4CFA57B0 - Address 0x00001574 - Aliases: sceRtc_driver_4CFA57B0
// Exported in sceRtc
// Exported in sceRtc_driver
s32 sceRtcGetCurrentClock(pspTime *time, int tz) {
    int oldK1 = pspShiftK1();
    pspTime *oldTime = (pspTime *) time;
    int oldTZ = tz;
    s32 res = SCE_ERROR_OK;

    if(oldK1 & (int)time < 0) {
        res = SCE_ERROR_PRIV_REQUIRED;
    } else {
        sceRtcGetCurrentTick(oldTime);
        sceRtcTickAddMinutes(oldTime, oldTime, oldTZ, oldTZ >>  31);
        sceRtcSetTick(oldTime, oldTime);
    }

    pspSetK1(oldK1);
    return res;
}

// DONE!
// Subroutine sceRtc_E7C27D1B - Address 0x00001604 - Aliases: sceRtc_driver_9012B140
// Exported in sceRtc
// Exported in sceRtc_driver
s32 sceRtcGetCurrentClockLocalTime(pspTime *time) {
    int oldK1 = pspShiftK1();

    if(oldK1 & (int)time < 0) {
        pspSetK1(oldK1);
        return SCE_ERROR_PRIV_REQUIRED;
    }

    pspSetK1(oldK1);
    // Upper and Lower time?
    return sceRtcGetCurrentClock(time, ctx.cUnk + rtc.dUnk);
}

// TODO: Finish.
// Subroutine sceRtc_011F03C1 - Address 0x00001660 - Aliases: sceRtc_029CA3B3, sceRtc_driver_011F03C1, sceRtc_driver_C0F36B91
// Exported in sceRtc
// Exported in sceRtc_driver
s32 sceRtcGetAccumulativeTime() {
    s64 tWide = sceKernelGetSystemTimeWide();


    return SCE_ERROR_OK;
}

// DONE!
// Subroutine sceRtc_34885E0D - Address 0x000016F8 - Aliases: sceRtc_driver_4E267E02
// Exported in sceRtc
// Exported in sceRtc_driver
void sceRtcConvertUtcToLocalTime(u64 *tickUTC, const u64 *tickLocal) {
    u32 res = ctx.dUnk + ctx.cUnk;
    sceRtc_driver_77138347 (tickUTC, tickLocal, res, (res >> 31));
}

// DONE!
// Subroutine sceRtc_779242A2 - Address 0x00001734 - Aliases: sceRtc_driver_3E66CB7E
// Exported in sceRtc
// Exported in sceRtc_driver
void sceRtcConvertLocalTimeToUTC(const u64 *tickLocal, u64 *tickUTC) {
    u32 res = -(ctx.dUnk + ctx.cUnk);
    sceRtc_driver_77138347 (tickUTC, tickLocal, res, (res >> 31));
}

// TODO: Finish
// Subroutine sceRtc_42307A17 - Address 0x00001774 - Aliases: sceRtc_driver_00F66D06
// Exported in sceRtc
// Exported in sceRtc_driver
s32 sceRtcIsLeapYear(int year) {
    int oldYear = year >> 31;
    /* Validate the year */
    if(year <= 0) {
        return SCE_ERROR_INVALID_ARGUMENT;
    }
    year *= 1374389535;

    return SCE_ERROR_OK;
}

// DONE! TODO: Fix pointer ref.
// Subroutine sceRtc_05EF322C - Address 0x000017DC - Aliases: sceRtc_driver_1DAB3CF3
// Exported in sceRtc
// Exported in sceRtc_driver
s32 sceRtcGetDaysInMonth (int year, int month) {
    int oldMonth = month;
    s32 res;

    // Confirm ASM.
    if((year < 1 || oldMonth < 1) != 0) {
        res = SCE_ERROR_INVALID_ARGUMENT;
    } else if(oldMonth < 13) {
        if(sceRtc_42307A17() == SCE_ERROR_OK) {
            res = *((s8*) (oldMonth + 0x000043B4))[11];
        }
    }

    return res;
}

// TODO: Finish and verify.
// Subroutine sceRtc_4B1B5E82 - Address 0x0000184C - Aliases: sceRtc_driver_4B1B5E82
// Exported in sceRtc
// Exported in sceRtc_driver
s32 sceRtcCheckValid(const pspTime *date) {
    pspTime *pDate = date;

    /* Validate the year */
    if(pDate->year == 0) {
        return -1;
    }

    return SCE_ERROR_OK;
}

// Subroutine sceRtc_36075567 - Address 0x00001938 - Aliases: sceRtc_driver_A4A5BF1B
// Exported in sceRtc
// Exported in sceRtc_driver
s32 sceRtcGetDosTime(pspTime *date, u32 dosTime) {

    return SCE_ERROR_OK;
}

// Subroutine sceRtc_E1C93E47 - Address 0x00001B34 - Aliases: sceRtc_driver_94225550
// Exported in sceRtc
// Exported in sceRtc_driver
s32 sceRtcGetTime64_t() {

    return SCE_ERROR_OK;
}

// Subroutine sceRtc_27C4594C - Address 0x00001C3C - Aliases: sceRtc_driver_E86D8FC0
// Exported in sceRtc
// Exported in sceRtc_driver
s32 sceRtcGetTime_t(const pspTime *date, time_t *time) {

    return SCE_ERROR_OK;
}

// Subroutine sceRtc_7ED29E40 - Address 0x00001D74 - Aliases: sceRtc_driver_7ED29E40, sceRtc_driver_E7B3ABF4
// Exported in sceRtc
// Exported in sceRtc_driver
s32 sceRtcSetTick(pspTime *date, const u64 *tick) {

    return SCE_ERROR_OK;
}

// TODO: Finish/Fix.
// Subroutine sceRtc_6FF40ACC - Address 0x000020B0 - Aliases: sceRtc_driver_6FF40ACC
// Exported in sceRtc
// Exported in sceRtc_driver
s32 sceRtcGetTick(const pspTime * date, u64 * tick) {
    s32 s0, s1;
    u64 *oldTick = tick;    //s2/s3
    s32 oldK1 = pspShiftK1();

    /* Validate address of tick */
    if(pspGetK1() & (int)tick < 0) {
        return SCE_ERROR_PRIV_REQUIRED;
    }

    return SCE_ERROR_OK;
}

// Subroutine sceRtc_DBF74F1B - Address 0x00002354 - Aliases: sceRtc_driver_E45726F6
// Exported in sceRtc
// Exported in sceRtc_driver
s32 sceRtcTickAddMonths(u64 *destTick, const u64 *srcTick, int numMonths) {

    return SCE_ERROR_OK;
}

// Subroutine sceRtc_42842C77 - Address 0x00002450 - Aliases: sceRtc_driver_AAAE90FF
// Exported in sceRtc
// Exported in sceRtc_driver
s32 sceRtcTickAddYears(u64 *destTick, const u64 *srcTick, int numYears) {

    return SCE_ERROR_OK;
}

// DONE!
// Subroutine sceRtc_C41C2853 - Address 0x00002500 - Aliases: sceRtc_driver_C41C2853, sceRtc_driver_C66D9686
// Exported in sceRtc
// Exported in sceRtc_driver
s32 sceRtcGetTickResolution() {
    return TICKS_PER_SECOND;
}

// Subroutine sceRtc_57726BC1 - Address 0x0000250C - Aliases: sceRtc_driver_321A839A
// Exported in sceRtc
// Exported in sceRtc_driver
s32 sceRtcGetDayOfWeek(int year, int month, int day) {

    return SCE_ERROR_OK;
}

// Subroutine sceRtc_F006F264 - Address 0x000025D4 - Aliases: sceRtc_driver_74772CCC
// Exported in sceRtc
// Exported in sceRtc_driver
s32 sceRtcSetDosTime(pspTime *date, u32 dosTime) {

    return SCE_ERROR_OK;
}

// Subroutine sceRtc_7ACE4C04 - Address 0x00002638 - Aliases: sceRtc_driver_CEF8FE8E
// Exported in sceRtc
// Exported in sceRtc_driver
s32 sceRtcSetWin32FileTime(pspTime *date, u64 *win32Time) {

    return SCE_ERROR_OK;
}

// Subroutine sceRtc_1909C99B - Address 0x000026DC - Aliases: sceRtc_driver_CF4E0EE0
// Exported in sceRtc
// Exported in sceRtc_driver
s32 sceRtcSetTime64_t() {

    return SCE_ERROR_OK;
}

// DONE!
// Subroutine sceRtc_3A807CC8 - Address 0x00002778 - Aliases: sceRtc_driver_40B07E72
// Exported in sceRtc
// Exported in sceRtc_driver
void sceRtcSetTime_t(pspTime *date, const time_t time) {
    sceRtcSetTime64_t(date, time, time, 0);
}

// DONE! This has a 4th argument, current SDK call incomplete.
// Doesn't use the stack.
// Subroutine sceRtc_44F45E05 - Address 0x00002798 - Aliases: sceRtc_driver_44F45E05
// Exported in sceRtc
// Exported in sceRtc_driver
s32 sceRtcTickAddTicks(u64 *destTick, const u64 *srcTick, u64 numMS, u64 unk) {
    int oldK1 = pspShiftK1();
    u64 *oldTick = destTick;
    s32 res;

    if(oldK1 & (int)destTick < 0) {
        res = SCE_ERROR_PRIV_REQUIRED;
    } else if(oldK1 & (int)srcTick >= 0) {
        (u32)oldTick[0] = (u32)srcTick[0] + (u32)numMS[0];
        (u32)oldTick[1] = ((u32)srcTick[1] + unk) + ((u32)oldTick[0] < numMS);
        res = SCE_ERROR_OK;
    }

    pspSetK1(oldK1);
    return res;
}

// DONE! This has a 4th argument, current SDK call incomplete.
// Doesn't use the stack.
// Subroutine sceRtc_26D25A5D - Address 0x000027F4 - Aliases: sceRtc_driver_B84AC7D7
// Exported in sceRtc
// Exported in sceRtc_driver
s32 sceRtcTickAddMicroseconds(u64 *destTick, const u64 *srcTick, u64 numMS, u64 unk) {
    sceRtcTickAddTicks(destTick, srcTick, numMS, unk);
    return SCE_ERROR_OK;
}

// This has a 4th argument, current SDK call incomplete.
// Subroutine sceRtc_F2A4AFE5 - Address 0x00002810 - Aliases: sceRtc_driver_89FA4262, sceRtc_driver_F2A4AFE5
// Exported in sceRtc
// Exported in sceRtc_driver
s32 sceRtcTickAddSeconds(u64 *destTick, const u64 *srcTick, u64 numSecs) {
    sceRtcTickAddTicks();
    return SCE_ERROR_OK;
}

// This has a 4th argument, current SDK call incomplete.
// Subroutine sceRtc_E6605BCA - Address 0x00002848 - Aliases: sceRtc_driver_77138347
// Exported in sceRtc
// Exported in sceRtc_driver
s32 sceRtcTickAddMinutes(u64 *destTick, const u64 *srcTick, u64 numMins) {

    return SCE_ERROR_OK;
}

// Subroutine sceRtc_26D7A24A - Address 0x00002880 - Aliases: sceRtc_driver_8413CADC
// Exported in sceRtc
// Exported in sceRtc_driver
s32 sceRtcTickAddHours(u64 *destTick, const u64 *srcTick, u64 numHours) {

    return SCE_ERROR_OK;
}

// Subroutine sceRtc_E51B4B7A - Address 0x000028C0 - Aliases: sceRtc_driver_CB0538FD
// Exported in sceRtc
// Exported in sceRtc_driver
s32 sceRtcTickAddDays(u64 *destTick, const u64 *srcTick, int numDays) {

    return SCE_ERROR_OK;
}

// Subroutine sceRtc_CF3A2CA8 - Address 0x00002914 - Aliases: sceRtc_driver_80F21937
// Exported in sceRtc
// Exported in sceRtc_driver
s32 sceRtcTickAddWeeks(u64 *destTick, const u64 *srcTick, int numWeeks) {

    return SCE_ERROR_OK;
}

// Subroutine sceRtc_9ED0AE87 - Address 0x00002968 - Aliases: sceRtc_driver_281144FE
// Exported in sceRtc
// Exported in sceRtc_driver
s32 sceRtcCompareTick(const u64 *tick1, const u64 *tick2) {

    return SCE_ERROR_OK;
}

// Subroutine sceRtc_DFBC5F16 - Address 0x000029D4 - Aliases: sceRtc_driver_C3A806EE
// Exported in sceRtc
// Exported in sceRtc_driver
s32 sceRtcParseDateTime(u64 *destTick, const char *dateString) {

    return SCE_ERROR_OK;
}

// Subroutine sceRtc_28E1E988 - Address 0x00002F24 - Aliases: sceRtc_driver_BDA60897
// Exported in sceRtc
// Exported in sceRtc_driver
s32 sceRtc_28E1E988() {
    return SCE_ERROR_OK;

}

// Subroutine sceRtc_28E1E988 - Address 0x00002F24 - Aliases: sceRtc_driver_BDA60897
s32 sub_00003190() {
    return SCE_ERROR_OK;
}

// Subroutine sub_00003214 - Address 0x00003214
s32 sub_00003214() {
    return SCE_ERROR_OK;
}

// Subroutine sub_00003390 - Address 0x00003390
s32 sub_00003390() {
    return SCE_ERROR_OK;
}

// Subroutine sub_000033D8 - Address 0x000033D8
s32 sub_000033D8() {
    return SCE_ERROR_OK;
}

// Subroutine sub_00003478 - Address 0x00003478
s32 sub_00003478() {
    return SCE_ERROR_OK;
}

// Subroutine sceRtc_C663B3B9 - Address 0x000034D4 - Aliases: sceRtc_driver_1A86F5FD
// Exported in sceRtc
// Exported in sceRtc_driver
s32 sceRtcFormatRFC2822() {
    return SCE_ERROR_OK;
}

// Subroutine sceRtc_0498FB3C - Address 0x00003770 - Aliases: sceRtc_driver_1FCE9E23
// Exported in sceRtc
// Exported in sceRtc_driver
s32 sceRtcFormatRFC3339() {
    return SCE_ERROR_OK;
}

// Subroutine sub_00003948 - Address 0x00003948
s32 sub_00003948() {
    return SCE_ERROR_OK;
}

// Subroutine sub_000039B0 - Address 0x000039B0
s32 sub_000039B0() {
    return SCE_ERROR_OK;
}

// Subroutine sceRtc_7DE6711B - Address 0x00003A70 - Aliases: sceRtc_driver_27FAEC90
// Exported in sceRtc
// Exported in sceRtc_driver
s32 sceRtcFormatRFC2822LocalTime() {
    return SCE_ERROR_OK;
}

// DONE! TODO: Can be revised.
// Subroutine sceRtc_27F98543 - Address 0x00003AD8 - Aliases: sceRtc_driver_8DED141A
// Exported in sceRtc
// Exported in sceRtc_driver
s32 sceRtcFormatRFC3339LocalTime(char *pszDateTime, const u64 *pUtc) {
    int oldK1 = pspShiftK1();

    if(oldK1 & (int)pszDateTime < 0) {
        pspSetK1(oldK1);
        return SCE_ERROR_PRIV_REQUIRED;
    }

    if(oldK1 & (int)pUtc >= 0) {
        pspSetK1(oldK1);
        return sceRtcFormatRFC3339(pszDateTime, pUtc, ctx.dUnk + ctx.cUnk);
    }

    pspSetK1(oldK1);
    return SCE_ERROR_PRIV_REQUIRED;
}

// Address - 0x00003B40
void getTimeOfDayHandler (int arg1, int arg2) {

}

// DONE!
// Subroutine sub_00003BCC - Address 0x00003BCC
void registerFunctions(void) {
    sceKernelRegisterRtcFunc(clockHandler, timeHandler, getTimeOfDayHandler, sceRtcGetCurrentSecureTick());
}

// DONE!
// Address 0x00003C04
u64 clockHandler (void) {
    u64 tick;
    sceRtcGetCurrentTick(tick);
    return tick;
}

// TODO: Finish.
// Address 0x00003C24
void timeHandler (int arg1){
    //u32 s0[8] = (u32)arg1;

}
