/* Copyright (C) 2011 The uOFW team
   See the file COPYING for copying permission.
*/

int sceLcdcInit();
int sceLcdcResume();
int sceLcdc_driver_901B9073(void);
int eventHandler(int ev_id, char *ev_name, void *param, int *result);
int sub_7DD8();
int sceLcdcCheckMode(int id, int xres, int yres, int arg3);
int sceLcdcSetMode(int id, int xres, int yres, int arg3);
int sceLcdcGetMode(int *arg0, int *arg1, int *arg2, int *arg3);
int sceLcdcReset();
int sceLcdcInsertDisplay_default(int *arg0, int *arg1, int *arg2, int *arg3, int *arg4);
int sceLcdcReadHPC();
int sceLcdcReadVPC();
int sub_887C();
int sceLcdcEnd();
int sceLcdcEnable();
int sceLcdcDisable();
int sceLcdc_driver_AF7A82E6(int arg);
int sceLcdc_driver_AB309648(int arg);
int sceLcdcSetHOffset(int arg0, int arg1);
float sceLcdcGetLcdcClockFreq();
int sceLcdcGetCyclesPerPixel();
float sceLcdcGetPixelClockFreq();
float sceLcdcGetHsyncFreq();
float sceLcdcGetVsyncFreq();
int sceLcdcReadUnderflow();
int sceLcdc_driver_1630642D();

