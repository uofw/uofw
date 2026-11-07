# Copyright (C) 2021 The uOFW team
# See the file COPYING for copying permission.

TARGET = usbcam
OBJS = usbcam.o

#DEBUG = 1

LIBS = -lsceUsbBus_driver -lsceUsbAcc_internal -lSysMemForKernel -lInterruptManagerForKernel \
       -lKDebugForKernel -lSysclibForKernel -lThreadManForKernel -lUtilsForKernel -lDmacManForKernel \
       -lsceSuspendForKernel -lsceDdr_driver

include $(ROOT_DIR)/lib/build.mak
