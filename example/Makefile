BUILD_DIR   = build
SRC_DIR     = src
SHARED_DIR  = ../shared

LIB_NAME = example
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
ALL_CFLAGS := $(CFLAGS) -D_GNU_SOURCE -I${SRC_DIR} -I${SHARED_INCLUDE_DIR}
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

clean:
	@echo "===> CLEAN"
	$(Q)rm -rf ${BUILD_DIR}

format:
	@echo "===> CLANG-FORMAT"
	$(Q)$(CLANG_FORMAT) -i $(sort $(wildcard $(SRC_DIR)/*.c))
	$(Q)$(CLANG_FORMAT) -i $(sort $(wildcard $(SRC_DIR)/*.h))
