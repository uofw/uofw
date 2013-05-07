# Copyright (C) 2011, 2012, 2013 The uOFW team
# See the file COPYING for copying permission.

TARGET = init
OBJS = init_asm.o libcUtils.o gamePatching.o init.o
PATCH_OBJS = patch.o

DEBUG = 0
INSTALLER = 1

LIBS = -lSysMemForKernel -lsceSuspendForKernel -lKDebugForKernel -lInterruptManagerForKernel \
       -lSysclibForKernel -lThreadManForKernel -lInitForKernel -lStdioForKernel -lIoFileMgrForUser \
       -lModuleMgrForKernel -lModuleMgrForUser -lUtilsForKernel -lLoadCoreForKernel -lLoadExecForKernel

include ../../lib/build.mak

