BUILD_DIR   = build
SRC_DIR     = src
TEST_DIR     = test

CC = clang

Q ?= @

CFLAGS  += -Wall -Wextra -std=c11
SHAREDFLAGS = -shared -fPIC -ldl -O2
LDFLAGS +=

.PHONY: clean

all: ${BUILD_DIR}/splay.so

test: ${BUILD_DIR}/splay.so ${BUILD_DIR}/test
	@echo "===> TEST"
	LD_PRELOAD=${BUILD_DIR}/splay.so ${BUILD_DIR}/test

${BUILD_DIR}/splay.so: ${BUILD_DIR}/splay.o ${BUILD_DIR}/tree.o
	$(Q)mkdir -p ${BUILD_DIR}
	@echo "===> LD $@"
	$(Q)$(CC) -o $@ ${CFLAGS} ${SHAREDFLAGS} ${LDFLAGS} $+

${BUILD_DIR}/%.o : ${SRC_DIR}/%.c
	$(Q)mkdir -p ${BUILD_DIR}
	@echo "===> CC $@"
	$(Q)$(CC) -o $@ -c -fPIC ${CFLAGS} $<


${BUILD_DIR}/test: ${TEST_DIR}/test.c
	$(Q)mkdir -p ${BUILD_DIR}
	@echo "===> CC/LD $@"
	$(Q)$(CC) -o $@ ${CFLAGS} ${LDFLAGS} $+

clean:
	@echo "===> CLEAN"
	$(Q)rm -rf ${BUILD_DIR}

