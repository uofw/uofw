# Copyright (C) The uOFW team
# See the file COPYING for copying permission.

TARGET = power
OBJS = battery.o clockFrequency.o idleTimer.o power.o powerSwitch.o scePowerSetGeEdramRefreshMode.o

LIBS = -lSysMemForKernel -lsceSuspendForKernel -lsceSysEventForKernel -lKDebugForKernel -lInterruptManagerForKernel \
       -lSysclibForKernel -lThreadManForKernel -lSysTimerForKernel -lInitForKernel -lsceDisplay_driver \
       -lsceSyscon_driver -lsceSysreg_driver -lsceDdr_driver -lsceGpio_driver -lsceGe_driver -lsceIdStorage_driver \
       -lUtilsForKernel -lsceCtrl_driver -lLoadExecForKernel

include ../../lib/build.mak