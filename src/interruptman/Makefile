# Copyright (C) 2011 The uOFW team
# See the file COPYING for copying permission.

TARGET = interruptman
OBJS = start.o interruptman.o end.o imports.o

BUILD_PRX=1
PRX_EXPORTS=exports.exp

INCDIR = 
CFLAGS = -O1 -G0 -Wall -Werror -nostdlib
CXXFLAGS = $(CFLAGS) -fno-exceptions -fno-rtti
ASFLAGS = $(CFLAGS)

LIBDIR =

PSPSDK=$(shell psp-config --pspsdk-path)
include $(PSPSDK)/lib/build.mak

