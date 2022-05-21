void sub_0000(s32, s32, s32);
s32 _sceSysconInitPowerStatus(void);
s32 _sceSysconSysEventHandler(s32 ev_id, char *ev_name, void* param, s32 *result);
s32 _sceSysconGpioIntr(s32 subIntr, s32 args, void *argp);
s32 _sceSysconCommonRead(s32 *ptr, s32 cmd);
s32 _sceSysconGetPommelVersion(void);
s32 _sceSysconCommonWrite(u32 val, u32 cmd, u32 size);
s32 _sceSysconSetCallback(SceSysconFunc func, void *argp, s32 cbId);
s32 _sceSysconPacketStart(SceSysconPacket *packet);
s32 _sceSysconPacketEnd(SceSysconPacket *packet);
s32 sub_2D08(u32 addr, u32 arg1, u32 arg2);
s32 sub_32D8(void);
s32 sub_3360(void);
s32 _sceSysconBatteryCommon(u32 cmd, s32 *ptr);
s32 sub_406C(s32 arg0);
s32 sub_4150(s32 arg0, s32 arg1, s32 arg2);
s32 _sceSysconGetPommelType(void);

