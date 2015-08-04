MODULES=debug audio clockgen codec ctrl exceptionman ge init interruptman \
iofilemgr led libatrac3plus loadcore loadexec mediaman me_wrapper syscon \
sysmem systimer usersystemlib wlanfirm

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

