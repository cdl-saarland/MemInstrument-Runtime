SRC_DIR     = src
INCLUDE_DIR = include

CLANG_FORMAT ?= clang-format

format:
	@echo "===> CLANG-FORMAT"
	$(Q)$(CLANG_FORMAT) -i $(sort $(wildcard $(SRC_DIR)/*.c))
	$(Q)$(CLANG_FORMAT) -i $(sort $(wildcard $(INCLUDE_DIR)/*.h))

