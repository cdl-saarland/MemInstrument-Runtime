SRC_DIR     = src
INCLUDE_DIR = include

format:
	@echo "===> CLANG-FORMAT"
	$(Q)clang-format -i $(sort $(wildcard $(SRC_DIR)/*.c))
	$(Q)clang-format -i $(sort $(wildcard $(INCLUDE_DIR)/*.h))

