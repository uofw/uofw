# Copyright (C) 2011, 2012 The uOFW team
# See the file COPYING for copying permission.

TARGET = loadexec
OBJS = kl4e.o loadexec.o
PATCH_OBJS = patch.o

DEBUG = 0
INSTALLER = 1

LIBS = -lSysMemForKernel -lsceSuspendForKernel -lKDebugForKernel -lLoadCoreForKernel -lInterruptManagerForKernel -lSysclibForKernel -lThreadManForKernel -lIoFileMgrForKernel -lModuleMgrForKernel -lInitForKernel -lUtilsForKernel

include $(ROOT_DIR)/lib/build.mak

