# Copyright (C) 2011, 2012 The uOFW team
# See the file COPYING for copying permission.

include ../../lib/common.mak

PSPSDK = `psp-config --pspsdk-path`
CFLAGS  := -O1 -fno-toplevel-reorder -G0 -Wall -Wextra -fno-builtin -nostdlib -I$(PSPSDK)/include
LDFLAGS := -specs=../../lib/prxspecs -Wl,-q,-T../../lib/linkfile.prx
TARGET = ../../lib/libdebug.a
OBJS =	tff/diskio.o \
		tff/tff.o \
		debug.o \
		memstk.o \
		syscon.o \
		sysreg.o

all: $(TARGET)

%.o: %.c
	$(CC) -c $^ -o $@ $(CFLAGS)

$(TARGET): $(OBJS)
	$(AR) cru $@ $^
	$(RANLIB) $@

clean:
	-rm -f $(OBJS)

