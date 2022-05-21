TARGETS=kprxgen fixup-imports build-exports basic-decompiler

all: $(TARGETS)

$(TARGETS):
	@$(MAKE) -C $@

clean mrproper:
	@$(foreach target, $(TARGETS), $(MAKE) -C $(target) $@;)

.PHONY: $(TARGETS) clean mrproper

