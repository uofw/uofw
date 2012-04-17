# Copyright (C) 2011, 2012 The uOFW team
# See the file COPYING for copying permission.

include ../../lib/common.mak

PSPSDK = `psp-config --pspsdk-path`
CFLAGS   := -I../../include -O1 -fno-toplevel-reorder -G0 -Wall -Wextra -Werror -fno-builtin -nostdlib -I$(PSPSDK)/include
LDFLAGS  := -L../../lib -specs=../../lib/prxspecs -Wl,-q,-T../../lib/linkfile.prx

# Setup default exports if needed
ifdef PRX_EXPORTS
EXPORT_OBJ=$(patsubst %.exp,%.o,$(PRX_EXPORTS))
else 
$(error You have to define PRX_EXPORTS in your Makefile)
endif

MODULE_STUBS=$(foreach mod,$(MODULES), $($(mod)_STUBS))

all: $(TARGET).prx

$(TARGET).elf: $(OBJS) $(EXPORT_OBJ)
	$(CC) $(CFLAGS) $(LDFLAGS) $^ $(LIBS) -o $@
	../../utils/fixup-imports/psp-fixup-imports $@

%.o: %.c
	$(CC) -c $^ -o $@ $(CFLAGS)

%.prx: %.elf
	#../../utils/kprxgen/psp-kprxgen $< $@
	psp-prxgen $< $@

$(PRX_EXPORTS:.exp=.c): $(PRX_EXPORTS)
	../../utils/build-exports/psp-build-exports -b $< > $@

clean: 
	-rm -f $(TARGET).prx $(TARGET).elf $(EXPORT_OBJ) $(OBJS)

rebuild: clean all

Makefile.exp: $(PRX_EXPORTS)
	cat ../../lib/build_stubs.mak > Makefile.exp
	../../utils/build-exports/psp-build-exports -k $< >> Makefile.exp

exp: Makefile.exp
	make -f Makefile.exp

exp-clean:
	make -f Makefile.exp clean
	-rm -f Makefile.exp

