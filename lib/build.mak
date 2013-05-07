# Copyright (C) 2011, 2012 The uOFW team
# See the file COPYING for copying permission.

include ../../lib/common.mak

PSPSDK = $(shell psp-config --pspsdk-path)
CFLAGS := -I../../include -O1 -fno-toplevel-reorder -G0 -Wall -Wextra -Werror -fno-builtin-bcopy -fno-builtin-bzero -fno-builtin-strchr -fno-builtin-printf -fno-builtin-puts -fno-builtin-putchar -nostdlib -I$(PSPSDK)/include
CFLAGS_S := -I../../include
LDFLAGS := -L../../lib -specs=../../lib/prxspecs -Wl,-q,-T../../lib/linkfile.prx -L$(PSPSDK)/lib

FIXUP_IMPORTS = ../../utils/fixup-imports/psp-fixup-imports
BUILD_EXPORTS = ../../utils/build-exports/psp-build-exports
PRXGEN = psp-prxgen

PRX_EXPORTS = exports.exp
EXPORT_OBJ=$(patsubst %.exp,%.o,$(PRX_EXPORTS))
EXPORT_C=$(PRX_EXPORTS:.exp=.c)

ifeq ($(DEBUG),1)
CFLAGS += -DDEBUG
LIBS := -ldebug -lpspdebug $(LIBS) -lSysclibForKernel -lsceDisplay -lsceGe_user -lIoFileMgrForKernel -lsceSyscon_driver
endif
ifeq ($(INSTALLER),1)
CFLAGS += -DINSTALLER
OBJS := $(PATCH_OBJS) $(OBJS)
endif

MODULE_STUBS=$(foreach mod,$(MODULES), $($(mod)_STUBS))

all: $(TARGET).prx

$(TARGET).elf: $(OBJS) $(EXPORT_OBJ)
	$(CC) $(CFLAGS) $(LDFLAGS) $^ $(LIBS) -o $@
	$(FIXUP_IMPORTS) $@

%.o: %.c
	$(CC) -c $^ -o $@ $(CFLAGS)

%.o: %.S
	$(CC) -c $^ -o $@ $(CFLAGS_S)

%.prx: %.elf
	$(PRXGEN) $< $@

$(PRX_EXPORTS:.exp=.c): $(PRX_EXPORTS)
	$(BUILD_EXPORTS) -b $< > $@

clean: 
	-$(RM) $(TARGET).prx $(TARGET).elf $(EXPORT_OBJ) $(EXPORT_C) $(OBJS)

rebuild: clean all

Makefile.exp: $(PRX_EXPORTS)
	cat ../../lib/build_stubs.mak > Makefile.exp
	$(BUILD_EXPORTS) -k $< >> Makefile.exp

exp: Makefile.exp
	make -f Makefile.exp

exp-clean: Makefile.exp
	make -f Makefile.exp clean
	-$(RM) Makefile.exp

