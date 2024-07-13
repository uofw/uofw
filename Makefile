ROOT_DIR = $(PWD)

export ROOT_DIR

MODULES=debug syscon \
kd/audio kd/chnnlsv kd/clockgen kd/codec kd/ctrl kd/exceptionman kd/ge kd/init kd/interruptman \
kd/iofilemgr kd/led kd/libatrac3plus kd/libparse_http kd/loadcore kd/loadexec kd/mediaman kd/me_wrapper kd/modulemgr \
kd/sysmem kd/systimer kd/usersystemlib kd/wlanfirm

all: $(MODULES)

utils:
	@$(MAKE) -C $@

$(MODULES): utils
	@$(MAKE) -C "src/$@"

clean:
	@$(foreach module, $(MODULES), $(MAKE) -C "src/$(module)" $@;)
	@$(MAKE) -C utils $@

mrproper: clean
	@$(MAKE) -C utils $@

.PHONY: utils clean mrproper

