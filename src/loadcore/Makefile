# Copyright (C) 2011, 2012 The uOFW team
# See the file COPYING for copying permission.

TARGET = loadcore
OBJS =  cache.o clibUtils.o hash.o interruptController.o loadcore.o loadelf.o module.o systable.o

LIBS = -lSysMemForKernel -lsceSuspendForKernel -lsceSysEventForKernel -lKDebugForKernel -lInterruptManagerForKernel \
      -lSysclibForKernel -lThreadManForKernel -lSysTimerForKernel -lInitForKernel -lsceDisplay_driver -lsceSyscon_driver

include ../../lib/build.mak

