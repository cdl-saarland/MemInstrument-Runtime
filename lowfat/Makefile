BUILD_DIR   		= build
SRC_DIR     		= src
TEST_DIR    		= test
SHARED_DIR  		= ../shared
GENERATED_HEADERS 	= ../include/meminstrument-rt

LF_CONF = lf_config.json
SIZES = ${GENERATED_HEADERS}/LFSizes.h
LINKER_SCRIPT = ${BUILD_DIR}/lowfat.ld

LIB_NAME = lowfat
DYN_LIB = ${BUILD_DIR}/lib${LIB_NAME}.so
STAT_LIB = ${BUILD_DIR}/lib${LIB_NAME}.a

LTO_BUILD_DIR = lto${BUILD_DIR}
LTO_LIB = ${LTO_BUILD_DIR}/lib${LIB_NAME}.a
LTO_LINKER_SCRIPT = ${LTO_BUILD_DIR}/lowfat.ld

SHARED_INCLUDE_DIR = ${SHARED_DIR}/include
SHARED_SRC_DIR = ${SHARED_DIR}/src

CLANG ?= clang
CLANG_FORMAT ?= clang-format

Q ?= @

CFLAGS  += -Wall -Wextra -O3 -D_GNU_SOURCE -I${SRC_DIR} -I${SHARED_INCLUDE_DIR} -I${GENERATED_HEADERS}
SHAREDFLAGS = -shared
LDFLAGS += -ldl

SRC=$(sort $(wildcard $(SRC_DIR)/*.c))
OBJ=$(SRC:$(SRC_DIR)/%.c=$(BUILD_DIR)/%.o)

SHARED_SRC=$(sort $(wildcard $(SHARED_SRC_DIR)/*.c))
SHARED_OBJ=$(SHARED_SRC:$(SHARED_SRC_DIR)/%.c=$(BUILD_DIR)/shared.%.o)

LTO_OBJECTS := $(subst $(SRC_DIR),$(LTO_BUILD_DIR),$(SRC:.c=.lto.o))
LTO_SHARED_OBJECTS := $(subst $(SHARED_SRC_DIR),$(LTO_BUILD_DIR),$(SHARED_SRC:.c=.lto.o))

DEP=$(OBJ:%.o=%.d) $(SHARED_OBJ:%.o=%.d)

ifneq ($(LLVM_GOLD),)
all: lto-static
endif

.PHONY: all clean

all: static dynamic

-include $(DEP)

test: test-dynamic test-static | build-dir

test-dynamic: dynamic ${BUILD_DIR}/test_dyn
	@echo "===> TEST DYNAMIC"
	LD_LIBRARY_PATH=${BUILD_DIR} ${BUILD_DIR}/test_dyn
	# LD_LIBRARY_PATH=${BUILD_DIR} LD_PRELOAD=${DYN_LIB} ${BUILD_DIR}/test_dyn

test-static: ${BUILD_DIR}/test_stat
	@echo "===> TEST STATIC"
	${BUILD_DIR}/test_stat

build-dir:
	@echo "===> Create build directory"
	$(Q)mkdir -p ${BUILD_DIR}

sizes-header-and-linker-script: | build-dir
	@echo "===> Generate sizes header and linker script"
	./generate_linker_script.py --config ${LF_CONF} --sizes ${SIZES} --scriptname ${LINKER_SCRIPT}

sizes-header-and-lto-linker-script: | lto-build-dir
	@echo "===> Generate sizes header and linker script"
	./generate_linker_script.py --config ${LF_CONF} --sizes ${SIZES} --lto-scriptname ${LTO_LINKER_SCRIPT}

static: sizes-header-and-linker-script ${STAT_LIB}

dynamic: sizes-header-and-linker-script ${DYN_LIB}

${STAT_LIB}: ${OBJ} ${SHARED_OBJ} | build-dir
	@echo "===> AR $@"
	$(Q)ar -rcs $@ $+

${DYN_LIB}: ${OBJ} ${SHARED_OBJ} | build-dir
	@echo "===> LD $@"
	$(Q)$(CLANG) -fPIC -MMD -o $@ ${CFLAGS} ${SHAREDFLAGS} ${LDFLAGS} $+

${BUILD_DIR}/shared.%.o : ${SHARED_SRC_DIR}/%.c | build-dir
	@echo "===> CC $@"
	$(Q)$(CLANG) -fPIC -MMD -o $@ -c ${CFLAGS} $<

${BUILD_DIR}/%.o : ${SRC_DIR}/%.c | build-dir
	@echo "===> CC $@"
	$(Q)$(CLANG) -fPIC -MMD -o $@ -c ${CFLAGS} $<

${BUILD_DIR}/test_dyn: ${TEST_DIR}/test.c dynamic
	@echo "===> CC/LD $@"
	$(Q)$(CLANG) -MMD -o $@ ${CFLAGS} ${LDFLAGS} -L${BUILD_DIR} -l${LIB_NAME} $<

${BUILD_DIR}/test_stat: ${TEST_DIR}/test.c static
	@echo "===> CC/LD $@"
	$(Q)$(CLANG) -MMD -o $@ ${CFLAGS} ${LDFLAGS} -L${BUILD_DIR} -l:lib${LIB_NAME}.a $<

lto-build-dir:
	@echo "===> LTO"
	@mkdir -p $(LTO_BUILD_DIR)

${LTO_OBJECTS}: $(SRC)
	@echo "===> CC LTO"
	$(foreach var,$(SRC),$(CLANG) $(CFLAGS) -flto -c $(var) -o ${subst $(SRC_DIR),$(LTO_BUILD_DIR),$(var:.c=.lto.o)};)

${LTO_SHARED_OBJECTS}: $(SHARED_SRC)
	@echo "===> CC LTO Shared"
	$(foreach var,$(SHARED_SRC),$(CLANG) $(CFLAGS) -flto -c $(var) -o ${subst $(SHARED_SRC_DIR),$(LTO_BUILD_DIR),$(var:.c=.lto.o)};)

lto-static: sizes-header-and-lto-linker-script lto-build-dir ${LTO_OBJECTS} ${LTO_SHARED_OBJECTS}
	@echo "===> MAKE LTO STATIC"
	ar --plugin=$(LLVM_GOLD) $(ARFLAGS) ${LTO_LIB} ${LTO_OBJECTS} ${LTO_SHARED_OBJECTS}

clean:
	@echo "===> CLEAN"
	$(Q)rm -rf ${BUILD_DIR}
	$(Q)rm -rf ${LTO_BUILD_DIR}
	$(Q)rm -f ${SIZES}

format:
	@echo "===> CLANG-FORMAT"
	$(Q)$(CLANG_FORMAT) -i $(sort $(wildcard $(SRC_DIR)/*.c))
	$(Q)$(CLANG_FORMAT) -i $(sort $(wildcard $(SRC_DIR)/*.h))
