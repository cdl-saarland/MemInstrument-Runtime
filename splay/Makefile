BUILD_DIR   = build
SRC_DIR     = src
TEST_DIR     = test

LIB_NAME = splay
DYN_LIB = ${BUILD_DIR}/lib${LIB_NAME}.so
STAT_LIB = ${BUILD_DIR}/lib${LIB_NAME}.a

Q ?= @

CFLAGS  += -Wall -Wextra -g -O3
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

build_dir:
	$(Q)mkdir -p ${BUILD_DIR}

${STAT_LIB}: ${OBJ}
	@echo "===> AR $@"
	$(Q)ar -rcs $@ $+

${DYN_LIB}: ${OBJ}
	@echo "===> LD $@"
	$(Q)$(CC) -fPIC -MMD -o $@ ${CFLAGS} ${SHAREDFLAGS} ${LDFLAGS} $+

${BUILD_DIR}/%.o : ${SRC_DIR}/%.c build_dir
	@echo "===> CC $@"
	$(Q)$(CC) -fPIC -MMD -o $@ -c ${CFLAGS} $<


${BUILD_DIR}/test_dyn: ${TEST_DIR}/test.c dynamic build_dir
	@echo "===> CC/LD $@"
	$(Q)$(CC) -MMD -o $@ ${CFLAGS} ${LDFLAGS} -L${BUILD_DIR} -l${LIB_NAME} $<

${BUILD_DIR}/test_stat: ${TEST_DIR}/test.c static build_dir
	@echo "===> CC/LD $@"
	$(Q)$(CC) -MMD -o $@ ${CFLAGS} ${LDFLAGS} -L${BUILD_DIR} -l:lib${LIB_NAME}.a $<

clean:
	@echo "===> CLEAN"
	$(Q)rm -rf ${BUILD_DIR}

