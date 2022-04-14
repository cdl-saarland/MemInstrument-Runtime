BUILD_DIR   = build
SRC_DIR     = src
TEST_DIR    = test
SHARED_DIR  = ../shared

LIB_NAME = sleep
DYN_LIB = ${BUILD_DIR}/lib${LIB_NAME}.so
STAT_LIB = ${BUILD_DIR}/lib${LIB_NAME}.a

SHARED_INCLUDE_DIR = ${SHARED_DIR}/include
SHARED_SRC_DIR = ${SHARED_DIR}/src

CLANG ?= clang
CLANG_FORMAT ?= clang-format

Q ?= @

# Add recommended flags
CFLAGS += -Wall -Wextra -O3
# Mandatory flags which should not be overwritable
ALL_CFLAGS := $(CFLAGS) -fno-strict-aliasing -D_GNU_SOURCE -I${SRC_DIR} -I${SHARED_INCLUDE_DIR}
SHAREDFLAGS = -shared
LDFLAGS += -ldl

SRC=$(sort $(wildcard $(SRC_DIR)/*.c))
OBJ=$(SRC:$(SRC_DIR)/%.c=$(BUILD_DIR)/%.o)

SHARED_SRC=$(sort $(wildcard $(SHARED_SRC_DIR)/*.c))
SHARED_OBJ=$(SHARED_SRC:$(SHARED_SRC_DIR)/%.c=$(BUILD_DIR)/shared.%.o)

DEP=$(OBJ:%.o=%.d) $(SHARED_OBJ:%.o=%.d)

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

${STAT_LIB}: ${OBJ} ${SHARED_OBJ}
	$(Q)mkdir -p ${BUILD_DIR}
	@echo "===> AR $@"
	$(Q)ar -rcs $@ $+

${DYN_LIB}: ${OBJ} ${SHARED_OBJ}
	$(Q)mkdir -p ${BUILD_DIR}
	@echo "===> LD $@"
	$(Q)$(CLANG) -fPIC -MMD -o $@ ${ALL_CFLAGS} ${SHAREDFLAGS} ${LDFLAGS} $+

${BUILD_DIR}/shared.%.o : ${SHARED_SRC_DIR}/%.c
	$(Q)mkdir -p ${BUILD_DIR}
	@echo "===> CC $@"
	$(Q)$(CLANG) -fPIC -MMD -o $@ -c ${ALL_CFLAGS} $<

${BUILD_DIR}/%.o : ${SRC_DIR}/%.c
	$(Q)mkdir -p ${BUILD_DIR}
	@echo "===> CC $@"
	$(Q)$(CLANG) -fPIC -MMD -o $@ -c ${ALL_CFLAGS} $<

${BUILD_DIR}/test_dyn: ${TEST_DIR}/test.c dynamic
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
