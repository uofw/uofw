# Copyright (C) 2011, 2016 The uOFW team
# See the file COPYING for copying permission.

TARGET = modulemgr
OBJS = chunk.o init.o loadModule.o modulemgr.o override.o queryModule.o rebootModule.o start-stopModule.o

INSTALLER = 0

LIBS = -lSysMemForKernel -lKDebugForKernel -lInterruptManagerForKernel -lUtilsForKernel \
       -lSysclibForKernel -lThreadManForKernel -lIoFileMgrForKernel -lLoadCoreForKernel

include $(ROOT_DIR)/lib/build.mak

