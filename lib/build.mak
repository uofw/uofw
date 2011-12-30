# Copyright (C) 2011 The uOFW team
# See the file COPYING for copying permission.

CC       = psp-gcc
CXX      = psp-g++
AS       = psp-gcc
FIXUP    = psp-fixup-imports

CFLAGS   := -I../../include -O1 -G0 -Wall -Wextra -Werror -nostdlib
LDFLAGS  := -specs=../../lib/prxspecs -Wl,-q,-T../../lib/linkfile.prx

# Setup default exports if needed
ifdef PRX_EXPORTS
EXPORT_OBJ=$(patsubst %.exp,%.o,$(PRX_EXPORTS))
else 
$(error You have to define PRX_EXPORTS in your Makefile)
endif

all: $(TARGET).prx

$(TARGET).elf: $(OBJS) $(EXPORT_OBJ)
	$(LINK.c) $^ $(LIBS) -o $@
	-$(FIXUP) $@

%.prx: %.elf
	../../utils/kprxgen/psp-kprxgen $< $@

%.c: %.exp
	../../utils/build-exports/psp-build-exports -b $< > $@

clean: 
	-rm -f $(TARGET).prx $(TARGET).elf $(EXPORT_OBJ) $(OBJS)

rebuild: clean all

