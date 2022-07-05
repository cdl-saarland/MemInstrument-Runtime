BUILD_DIR   = build
SRC_DIR     = src
TEST_DIR    = test
SHARED_DIR  = ../shared

LIB_NAME = splay
DYN_LIB = ${BUILD_DIR}/lib${LIB_NAME}.so
STAT_LIB = ${BUILD_DIR}/lib${LIB_NAME}.a

LTO_BUILD_DIR = lto${BUILD_DIR}
LTO_LIB = ${LTO_BUILD_DIR}/lib${LIB_NAME}.a

SHARED_INCLUDE_DIR = ${SHARED_DIR}/include
SHARED_SRC_DIR = ${SHARED_DIR}/src

CLANG ?= clang
CLANG_FORMAT ?= clang-format

Q ?= @

# Ignore LLVM tooling files by default
CFLAGS += -DMIRT_IGNORE_LLVM_TOOLING
# Add recommended flags
CFLAGS += -Wall -Wextra -O3
# Mandatory flags which should not be overwritable
ALL_CFLAGS := $(CFLAGS) -D_GNU_SOURCE -fno-strict-aliasing -I${SRC_DIR} -I${SHARED_INCLUDE_DIR}
SHAREDFLAGS = -shared
LDFLAGS += -ldl

SRC=$(sort $(wildcard $(SRC_DIR)/*.c))
OBJ=$(SRC:$(SRC_DIR)/%.c=$(BUILD_DIR)/%.o)

SHARED_SRC=$(sort $(wildcard $(SHARED_SRC_DIR)/*.c))
SHARED_OBJ=$(SHARED_SRC:$(SHARED_SRC_DIR)/%.c=$(BUILD_DIR)/shared.%.o)

LTO_OBJECTS := $(subst $(SRC_DIR),$(LTO_BUILD_DIR),$(SRC:.c=.lto.o))
LTO_SHARED_OBJECTS := $(subst $(SHARED_SRC_DIR),$(LTO_BUILD_DIR),$(SHARED_SRC:.c=.lto.o))

DEP=$(OBJ:%.o=%.d) $(SHARED_OBJ:%.o=%.d)

# Automatically build the lto compatible library if gold is available
ifdef LLVM_GOLD
all: lto-static
endif

.PHONY: all clean format

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
	$(Q)$(CLANG) -MMD -o $@ ${ALL_CFLAGS} $< -L${BUILD_DIR} -l${LIB_NAME} ${LDFLAGS}

${BUILD_DIR}/test_stat: ${TEST_DIR}/test.c static
	$(Q)mkdir -p ${BUILD_DIR}
	@echo "===> CC/LD $@"
	$(Q)$(CLANG) -MMD -o $@ ${ALL_CFLAGS} $< -L${BUILD_DIR} -l:lib${LIB_NAME}.a ${LDFLAGS}

lto-build-dir:
	@echo "===> LTO"
	@mkdir -p $(LTO_BUILD_DIR)

${LTO_OBJECTS}: $(SRC)
	@echo "===> CC LTO"
	$(foreach var,$(SRC),$(CLANG) $(ALL_CFLAGS) -flto -c $(var) -o ${subst $(SRC_DIR),$(LTO_BUILD_DIR),$(var:.c=.lto.o)};)

${LTO_SHARED_OBJECTS}: $(SHARED_SRC)
	@echo "===> CC LTO Shared"
	$(foreach var,$(SHARED_SRC),$(CLANG) $(ALL_CFLAGS) -flto -c $(var) -o ${subst $(SHARED_SRC_DIR),$(LTO_BUILD_DIR),$(var:.c=.lto.o)};)

gold-available:
ifndef LLVM_GOLD
	$(error Cannot find the shared gold library. Set LLVM_GOLD to build the lto libs)
endif

lto-static: gold-available lto-build-dir ${LTO_OBJECTS} ${LTO_SHARED_OBJECTS}
	@echo "===> MAKE LTO STATIC"
	ar --plugin=$(LLVM_GOLD) $(ARFLAGS) ${LTO_LIB} ${LTO_OBJECTS} ${LTO_SHARED_OBJECTS}

clean:
	@echo "===> CLEAN"
	$(Q)rm -rf ${BUILD_DIR}
	$(Q)rm -rf ${LTO_BUILD_DIR}

format:
	@echo "===> CLANG-FORMAT"
	$(Q)$(CLANG_FORMAT) -i $(sort $(wildcard $(SRC_DIR)/*.c))
	$(Q)$(CLANG_FORMAT) -i $(sort $(wildcard $(SRC_DIR)/*.h))
