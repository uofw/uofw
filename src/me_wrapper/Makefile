TARGET = me_wrapper

OBJS = main.o init.o imports.o

BUILD_PRX = 1
PRX_EXPORTS = exports.exp

INCDIR =
CFLAGS = -O2 -G0 -Wall
CXXFLAGS = $(CFLAGS) -fno-exceptions -fno-rtti
ASFLAGS = $(CFLAGS)

USE_KERNEL_LIBC = 1
USE_KERNEL_LIBS = 1

LIBDIR =
LDFLAGS = -mno-crt0 -nostartfiles
LIBS = -nodefaultlibs -nostdlib

PSPSDK=$(shell psp-config --pspsdk-path)
include $(PSPSDK)/lib/build.mak