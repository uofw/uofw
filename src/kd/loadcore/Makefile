# Copyright (C) 2011, 2012 The uOFW team
# See the file COPYING for copying permission.

TARGET = loadcore
OBJS = cache.o clibUtils.o hash.o interruptController.o loadcore.o loadcore_asm.o loadelf.o module.o systable.o

LIBS = -lSysMemForKernel -lKDebugForKernel -lUtilsForKernel -lInterruptManagerForKernel -lmemlmd -lsceMesgLed_driver

include $(ROOT_DIR)/lib/build.mak

