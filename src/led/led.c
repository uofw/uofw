/* Copyright (C) 2011, 2012 The uOFW team    
   See the file COPYING for copying permission.
*/

#define LED_MODE_OFF   0 
#define LED_MODE_ON    1 
#define LED_MODE_TIMED 2 
#define LED_MODE_DELAY 3 

#define LED_TYPE_MS   0 
#define LED_TYPE_WLAN 1 
#define LED_TYPE_BT   2

#define LED_GPIO_PORT_MS 0x40
#define LED_GPIO_PORT_WLAN 0x40
#define LED_GPIO_PORT_BT 0x01000000

PspSysEventHandler g_LedSysEv = //0E10
{
    sizeof(PspSysEventHandler),
    "SceLed",
    0x00FFFF00,
    _sceLedSysEventHandler
};

typedef struct 
{ 
  int unk0; 
  int oncnt; 
  int offcnt; 
  int endcnt; 
  int endstat; 
}SceLedParam; 

typedef struct 
{
    u8  gpioPort;
    u8  unk1;
    u8  unk2;
    u8  unk3;
    int unk4;
    int unk8;
    int unk12;
    int unk16;
    int unk20;
    int unk24;
    int unk28;
    int unk32;
    SceLedParam    *unk36;
    int unk40;
    int unk44;
    int unk48;
    int unk52;
    int unk56;
    int unk60;
    int unk64;
    int unk68;
    int unk72;
    int unk76;
}SceLED;


SceLED leds[3]; //E50
int g_00000F40;

int sceLedInit(void)
{    
    int i
    memset(&leds, 0, 0xF0);
    g_00000F40 = 2;
    leds[LED_TYPE_MS].gpioPort = 6;
    leds[LED_TYPE_MS].unk2 = 0;
    leds[LED_TYPE_WLAN].gpioPort = 7;
    leds[LED_TYPE_WLAN].unk2 = 1;
    
    int model = sceKernelGetModel();
    if((model - 1 < 2) || model == 4)
    {
        leds[LED_TYPE_WLAN].unk1 = 1;
    }
    if(((model - 4) < 2) || model == 7 || model == 9)
    {
        //loc_0000022C
        leds[LED_TYPE_BT].gpioPort = 24;
        leds[LED_TYPE_BT].unk2 = 3;
        if(model == 4)
            leds[LED_TYPE_BT].unk1 = 1;
        g_00000F40 = 3;
    }

    if(g_00000F40 >= 0)
    {
        for(i = 0; i < g_00000F40; i++)
        {
            //loc_000000D4
            if(sceGpioGetPortMode(leds[i].gpioPort) == 2)
            {
                //loc_00000210
                sceGpioSetPortMode(leds[i].gpioPort, 0);
                if(leds[i].unk1 == 0)
                {
                    //loc_000001AC
                    sceGpioPortClear(1 << leds[i].gpioPort);
                    continue;
                }
            }
            else if(i == LED_TYPE_BT)
            {
                //loc_000001BC
                int gpioPort = sceGpioPortRead();
                int btStatus = sceSysconGetBtPowerStatus();
                
                if(((btStatus == 0) ? 0 : (((gpioPort >> leds[i].gpioPort) & 1) ^ leds[i].unk1)) != 0)
                {
                    //loc_000001FC
                    sceLedSetMode(2, 1, 0);
                    continue;
                }
                else
                {
                    if(leds[i].unk1 == 0)
                    {
                        //loc_000001AC
                         sceGpioPortClear(1 << leds[i].gpioPort); //since i is 2 then this is 160, but I wanted it to be uniform
                         continue;
                    }
                }
            }
            else
            {
                if(leds[i].unk1 == 0)
                {
                    //loc_000001AC
                    sceGpioPortClear(1 << leds[i].gpioPort);
                    continue;                    
                }
            }
            
            //loc_000000F8
            /*
             * turn on memory card the first time around, WLAN the second time around
             * and bluetooth the third time around
             */
            sceGpioPortSet(1 << leds[i].gpioPort);
            //loc_00000104
        }
    }
    
    if(g_00000F40 >= 0)
    {
        for(i = 0; i < g_00000F40; i++)
        {
            sceSysconCtrlLED(leds[i].unk2, 1);
        }
    }
    
    sceKernelRegisterSubIntrHandler(PSP_VBLANK_INT, 25, _sceLedVblankInterrupt, NULL);
    sceKernelEnableSubIntr(PSP_VBLANK_INT, 25);
    sceKernelRegisterSysEventHandler(g_LedSysEv);
    
    return 0;
}

int sceLedEnd()
{
    int i;
    sceKernelUnregisterSysEventHandler(g_LedSysEv);
    sceKernelReleaseSubIntrHandler(PSP_VBLANK_INT, 25);
    
    if(g_00000F40 <= 0)
    {
        return 0;
    }
    
    for(i = 0; i < g_00000F40; i++)
    {
        if(i == LED_TYPE_BT)
        {
            //loc_00000324
            if(leds[i].unk4 == 1)
            {
                //loc_000002D4
                continue;
            }
            else if(leds[i].unk1 == 0)
            {
                //loc_00000348
                sceGpioPortClear(1 << leds[i].gpioPort);
                continue;
            }
        }
        else if(leds[i].unk1 == 0)
        {
            sceGpioPortClear(1 << leds[i].gpioPort);
            continue;
        }
        
        //loc_000002CC
        sceGpioPortSet(1 << leds[i].gpioPort);
    }
    
    return 0;
}

//0350
int _sceLedSysEventHandler(int ev_id, char *ev_name, void *param, int *result)
{
    if(ev_id == 0x400A)
    {
        //loc_000003A0
        _sceLedSuspend(param[1] == 2);
    }
    else if(ev_id == 0x100000)
    {
        //loc_00000378
        _sceLedResume(param[1][1] == 0x80000000);
    }
    
    return 0;
}

//03C4
int _sceLedVblankInterrupt()
{
    int i;
    
    if(g_00000F40 < 0)
    {
        return -1;
    }
    
    for(i = 0; i < g_00000F40; i++)
    {
        if(leds[i].unk4 == 3)
        {
            //loc_000004F8
            _sceLedSeqExec(leds[i]);
        }
    }
    
    for(i = 0; i < g_00000F40; i++)
    {
        int unk8 = leds[i].unk8;
        
        if(leds[i].unk20 < 0)
        {
            //loc_000004CC
            if(leds[i].unk20 == 0)
            {
                leds[i].unk8 = 0;
                leds[i].unk12 = 0;
                if(leds[i].unk24 != 0)
                {
                    leds[i].unk16 = 0;
                    leds[i].unk12 = 1;
                }
                else
                {
                    leds[i].unk16 = 1;
                }
            }
        }
        else
        {
            leds[i].unk20--;
        }
        
        if(((unk8 + 1) + ((leds[i].unk12) + (leds[i].unk16))) != 0)
        {
            leds[i].unk8++;
        }
        else
        {
            leds[i].unk8 = 0;
        }
        
        if((leds[i].unk8 < leds[i].unk12) == leds[i].unk1)
        {
            //loc_000004BC
            sceGpioPortClear(1 << leds[i].gpioPort);
        }
        else
        {
            sceGpioPortSet(1 << leds[i].gpioPort);
        }
    }
    
    return -1;
}

/* By artart78 */
int _sceLedSeqExec(SceLED *led)
{
    if (led->unk32 > 0)
    {
        if (--led->unk32 > 0)
            return 0;
    }
    // 0528
    char *ptr = led->unk36;
    while (ptr != NULL)
    {
        char cmd = *(ptr++);
        if (cmd < 0)
        {
            // 0690
            led->unk32 = cmd & 0x7F;
            led->unk36 = ptr;
            return 0;
        }
        switch (cmd)
        {
            case 0:
                // 0580
                led->unk36 = 0;
                return 0;
    
            case 1:
                // 056C
                led->unk40 = ptr;
                break;
    
            case 2:
                // 0588
                ptr = led->unk40;
                break;
    
            case 3:
                // 0590
                if (led->unk44 >= 4) {
                    led->unk36 = NULL;
                    return 0;
                }
                *(int*)(led + 52 + led->unk44 * 8) = *ptr;
                *(int*)(led + 48 + led->unk44 * 8) = (++ptr);
                led->unk44++;
                break;
    
            case 4:
                // 05D0
                if (led->unk44 <= 0) {
                    led->unk36 = 0;
                    return 0;
                }
                *(int*)(led + 44 + led->unk44 * 8)--;
                if (*(int*)(led + 44 + led->unk44 * 8) <= 0)
                {
                    // 0610
                    led->unk44--;
                }
                else
                    ptr = *(int*)(led + 48 + led->unk44 * 8);
                break;
    
            case 16:
                // 0620
                led->unk12 = 1;
                led->unk28 = 1;
                led->unk16 = 0;
                break;
    
            case 17:
                // 0634
                led->unk16 = 1;
                led->unk28 = 0;
                led->unk12 = 0;
                break;
    
            case 18:
                // 0648
                if (led->unk28 == 1)
                {
                    led->unk12 = 1;
                    // 0664
                    led->unk28 = 0;
                    led->unk16 = 0;
                }
                else
                {
                    led->unk16 = 1;
                    led->unk28 = 1;
                    led->unk12 = 0;
                }
                break;
    
            case 19:
                // 066C
                led->unk28 = 2;
                led->unk12 = *(ptr++);
                led->unk16 = *ptr;
                break;
    
            default:
                // 0618
                ptr = NULL;
                break;
        }
        
        // 0570
    }
    led->unk36 = ptr;
    return 0;
}

int sceLedSetMode(int led_type, int mode, SceLedParam *param)
{
    if(led_type < 0 || led_type >= g_00000F40)
        return 0x80000102;
    
    if(mode >= 4)
        return 0x80000107;
    
    leds[led_type].unk20 = -1;
    leds[led_type].unk36 = NULL;
    int old_mode = leds[led_type].unk4;
    leds[led_type].unk4 = mode;
    
    if(mode == LED_MODE_ON)
    {
        //loc_00000790
        leds[led_type].unk12 = mode;
        leds[led_type].unk16 = 0;
        leds[led_type].unk8 = 0;
    }
    else if(mode == LED_MODE_OFF)
    {
        //loc_00000778
        leds[led_type].unk16 = 1;
        leds[led_type].unk8 = 0;
        leds[led_type].unk12 = 0;
    }
    else if(mode == LED_MODE_TIMED)
    {
        //loc_0000074C
        if(old_mode != mode)
            leds[led_type].unk8 = 0;
            
        leds[led_type].unk12 = param.oncnt;
        leds[led_type].unk16 = param.offcnt;
        leds[led_type].unk20 = param.endcnt;
        leds[led_type].unk24 = param.endstat;
    }
    else if(mode == LED_MODE_DELAY)
    {
        //loc_0000072C
        if(old_mode != mode)
            leds[led_type].unk28 = old_mode;
        
        leds[led_type].unk32 = 1;
        leds[led_type].unk36 = &param;
        leds[led_type].unk44 = 0;
        leds[led_type].unk8 = 0;
        leds[led_type].unk40 = 0;
    }
    
    return 0;
}

int module_start
{
    sceLedInit();
    return 0;
}

int module_reboot_before()
{
    sceLedEnd();
    return 0;
}

int _sceLedSuspend(int arg0)
{
    sceKernelDisableSubIntr(PSP_VBLANK_INT, 25);
    
    if(arg0 == 0)
    {
        //loc_000008D8
        sceGpioPortClear(LED_GPIO_PORT_MS);
        sceGpioSetPortMode(6, 2);
        int ret = sceSysconCtrlLED(1, 0);
        
        if(ret == 0)
        {
            //loc_000009C4
            sceGpioPortClear(LED_GPIO_PORT_WLAN);
            sceGpioSetPortMode(6, 2);
        }
        else
        {
            if(leds[LED_TYPE_WLAN].unk1 == 1)
            {
                //loc_000009B0
                sceGpioPortClear(1 << leds[LED_TYPE_WLAN].gpioPort);
            }
            else
            {
                sceGpioPortSet(1 << leds[LED_TYPE_WLAN].gpioPort);
            }
        }
        
        if(leds[LED_TYPE_BT].unk2 == 0)
            return 0;
            
        sceSysconCtrlLED(3, 0);
        
        if(sceSysconGetBtPowerCtrl() == 0)
        {
            //loc_00000994
            sceGpioPortClear(LED_GPIO_PORT_BT);
            sceGpioSetPortMode(24, 2);
            
            return 0;
        }
        
        if(leds[LED_TYPE_BT].unk1 == 1)
        {
            //loc_00000980
            sceGpioPortClear(1 << leds[LED_TYPE_BT].gpioPort);
        }
        
        sceGpioPortSet(1 << leds[LED_TYPE_BT].gpioPort);
        return 0;
    }
    else
    {
        int i;
        if(g_00000F40 < 0)
            return 0;
            
        for(i = 0; i < g_00000F40; i++)
        {
            s1 = leds;
            v1 = leds;
            if(i == LED_TYPE_BT)
            {
                //loc_000008AC
                if(leds[i].unk4 == 1)
                {
                    continue;
                }
                
                if(leds[i].unk1 == 0)
                {
                    //loc_000008D0
                    sceGpioPortClear(1 << leds[i].gpioPort);
                    continue;
                }
            }
            else
            {
                if(leds[i].unk1 == 0)
                {
                    //loc_00000898
                    sceGpioPortClear(1 << leds[i].gpioPort);
                    continue;
                }
            }

            sceGpioPortSet(1 << leds[i].gpioPort);
        }
    }
}

int _sceLedResume(int arg0)
{
    int i;
    if(g_00000F40 < 0)
    {
        sceKernelEnableSubIntr(PSP_VBLANK_INT, 25);
    }
    
    s6 s1 = leds; //fp BT
    
    for(i = 0; i < g_00000F40; i++)
    {
        if(arg0 != 0)
        {
            if(i == LED_TYPE_BT)
            {
                //loc_00000AE0
                int gpioPort = sceGpioPortRead();
                int btStatus = sceSysconGetBtPowerStatus();
                
                if(((btStatus == 0) ? 0 : ((gpioPort >> leds[i].gpioPort) & 1) ^ leds[i].unk1) != 0)
                {
                    sceLedSetMode(2, 1, 0);
                }
                else
                {
                    sceLedSetMode(2, 0, 0);
                }
                
                sceGpioSetPortMode(leds[i].gpioPort, 0);
                sceSysconCtrlLED(leds[i].unk2, 1);
                
                continue;
            }
        }
        
        if(leds[i].unk1 == 0)
        {
            //loc_00000AD0
            sceGpioPortClear(1 << leds[i].gpioPort);
        }
        else
        {
            sceGpioPortSet(1 << leds[i].gpioPort);
        }
        
        sceGpioSetPortMode(leds[i].gpioPort, 0);
        sceSysconCtrlLED(leds[i].unk2, 1);
    }
}
