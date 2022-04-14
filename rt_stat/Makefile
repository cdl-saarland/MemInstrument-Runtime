BUILD_DIR   = build
SRC_DIR     = src
TEST_DIR     = test

LIB_NAME = rt_stat
DYN_LIB = ${BUILD_DIR}/lib${LIB_NAME}.so
STAT_LIB = ${BUILD_DIR}/lib${LIB_NAME}.a

CLANG ?= clang
CLANG_FORMAT ?= clang-format

Q ?= @

# Add recommended flags
CFLAGS += -Wall -Wextra -std=c11 -O3
# Mandatory flags which should not be overwritable
ALL_CFLAGS := $(CFLAGS) -fPIC -D_GNU_SOURCE
SHAREDFLAGS = -shared
LDFLAGS += -ldl

SRC=$(sort $(wildcard $(SRC_DIR)/*.c))
OBJ=$(SRC:$(SRC_DIR)/%.c=$(BUILD_DIR)/%.o)

DEP=$(OBJ:%.o=%.d)

.PHONY: all clean

all: static dynamic

-include $(DEP)

test: test-dynamic test-static

test-dynamic: dynamic ${BUILD_DIR}/test_dyn
	@echo "===> TEST DYNAMIC"
	LD_LIBRARY_PATH=${BUILD_DIR} ${BUILD_DIR}/test_dyn
	# LD_LIBRARY_PATH=${BUILD_DIR} LD_PRELOAD=${DYN_LIB} ${BUILD_DIR}/test_dyn

test-static: ${BUILD_DIR}/test_stat
	@echo "===> TEST STATIC"
	${BUILD_DIR}/test_stat

static: ${STAT_LIB}

dynamic: ${DYN_LIB}

${STAT_LIB}: ${OBJ}
	$(Q)mkdir -p ${BUILD_DIR}
	@echo "===> AR $@"
	$(Q)ar -rcs $@ $+

${DYN_LIB}: ${OBJ}
	$(Q)mkdir -p ${BUILD_DIR}
	@echo "===> LD $@"
	$(Q)$(CLANG) -MMD -o $@ ${ALL_CFLAGS} ${SHAREDFLAGS} ${LDFLAGS} $+

${BUILD_DIR}/%.o : ${SRC_DIR}/%.c
	$(Q)mkdir -p ${BUILD_DIR}
	@echo "===> CC $@"
	$(Q)$(CLANG) -MMD -o $@ -c ${ALL_CFLAGS} $<


${BUILD_DIR}/test_dyn: ${TEST_DIR}/test.c
	$(Q)mkdir -p ${BUILD_DIR}
	@echo "===> CC/LD $@"
	$(Q)$(CLANG) -MMD -o $@ ${ALL_CFLAGS} ${LDFLAGS} -L${BUILD_DIR} -l${LIB_NAME} $<

${BUILD_DIR}/test_stat: ${TEST_DIR}/test.c static
	$(Q)mkdir -p ${BUILD_DIR}
	@echo "===> CC/LD $@"
	$(Q)$(CLANG) -MMD -o $@ ${ALL_CFLAGS} ${LDFLAGS} -L${BUILD_DIR} -l:lib${LIB_NAME}.a $<

clean:
	@echo "===> CLEAN"
	$(Q)rm -rf ${BUILD_DIR}

format:
	@echo "===> CLANG-FORMAT"
	$(Q)$(CLANG_FORMAT) -i $(sort $(wildcard $(SRC_DIR)/*.c))
	$(Q)$(CLANG_FORMAT) -i $(sort $(wildcard $(SRC_DIR)/*.h))
