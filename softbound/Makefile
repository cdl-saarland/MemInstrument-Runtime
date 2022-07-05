CLANG ?= clang
CLANG_FORMAT ?= clang-format

BUILD_DIR := build
SRC_DIR := src
SHARED_DIR := ../shared
SHARED_SRC_DIR := ${SHARED_DIR}/src

# Use wrapper checks, spatial safety only and ignore the LLVM tooling by default
CFLAGS += -D__SOFTBOUNDCETS_SPATIAL -D__SOFTBOUNDCETS_WRAPPER_CHECKS -DMIRT_IGNORE_LLVM_TOOLING
# Add recommended flags
CFLAGS += -Wall -pedantic -O3
# Mandatory flags which should not be overwritable (fPIC is required for the dynamic library)
ALL_CFLAGS :=  -fPIC -I${SRC_DIR} -I${SHARED_DIR}/include -D_GNU_SOURCE $(CFLAGS)
ARFLAGS := -rcs
LINK_LIBS := -luuid -ldl -lcrypt

LIB_NAME := softboundcets_rt
DYN_LIB_NAME := lib${LIB_NAME}.so
STAT_LIB_NAME := lib${LIB_NAME}.a

SHARED_SOURCES := $(wildcard ${SHARED_SRC_DIR}/*.c)
SHARED_OBJECTS := $(subst $(SHARED_SRC_DIR),$(BUILD_DIR),$(SHARED_SOURCES:.c=.o))

SOURCES := $(wildcard ${SRC_DIR}/*.c)
HEADERS := $(wildcard ${SRC_DIR}/*.h)
OBJECTS := $(subst $(SRC_DIR),$(BUILD_DIR),$(SOURCES:.c=.o))

LTO_DIR := ltobuild
LTO_OBJECTS := $(subst $(SRC_DIR),$(LTO_DIR),$(SOURCES:.c=.lto.o))
LTO_SHARED_OBJECTS := $(subst $(SHARED_SRC_DIR),$(LTO_DIR),$(SHARED_SOURCES:.c=.lto.o))

all: static dynamic exports

exports: cxx-header

# Automatically build the lto compatible library if gold is available
ifdef LLVM_GOLD
all: lto-static
endif

# Write a compile_commands.json file
# This code is a modified version of https://gist.github.com/JayKickliter/f4e1945abe1d3bbbe3263640a3669e3c
%.compdb_entry: %.c
	@echo "    {" > $@
	@echo "        \"command\": \"$(CLANG) $(ALL_CFLAGS) -c $<\"," 	>> $@
	@echo "        \"directory\": \"$(CURDIR)\","                 	>> $@
	@echo "        \"file\": \"$<\""                            	>> $@
	@echo "    },"                                              	>> $@

COMPDB_ENTRIES = $(addsuffix .compdb_entry, $(basename $(SOURCES)))

${BUILD_DIR}/compile_commands.json: $(COMPDB_ENTRIES)
	@echo "===> GENERATE compile_commands.json"
	@echo "[" > $@.tmp
	@cat $^ >> $@.tmp
	@sed '$$d' < $@.tmp > $@
	@echo "    }" >> $@
	@echo "]" >> $@
	@rm $@.tmp

build-dir:
	@mkdir -p ${BUILD_DIR}

gen-compile-commands: build-dir ${BUILD_DIR}/compile_commands.json
	@for f in $(COMPDB_ENTRIES); do rm $$f; done
	@echo "'compile_commands.json' sucessfully generated"

# Auto-generate C++ header
cxx-header: gen-compile-commands
	@echo "===> GENERATE C++ header"
	./generate_available_wrapper.py
	./generate_run_time_config_info.py

# Compile without LTO static or dynamic
$(OBJECTS): $(SOURCES)
	@echo "===> CC"
	$(foreach var,$(SOURCES),$(CLANG) $(ALL_CFLAGS) -c $(var) -o ${subst $(SRC_DIR),$(BUILD_DIR),$(var:.c=.o)};)

$(SHARED_OBJECTS): $(SHARED_SOURCES)
	@echo "===> CC Shared"
	@mkdir -p ${BUILD_DIR}
	$(foreach var,$(SHARED_SOURCES),$(CLANG) $(ALL_CFLAGS) -c $(var) -o ${subst $(SHARED_SRC_DIR),$(BUILD_DIR),$(var:.c=.o)};)

static: build-dir $(OBJECTS) $(SHARED_OBJECTS) exports
	@echo "===> MAKE STATIC"
	ar $(ARFLAGS) ${BUILD_DIR}/${STAT_LIB_NAME} $(OBJECTS) $(SHARED_OBJECTS)

dynamic: $(OBJECTS) $(SHARED_OBJECTS) exports
	@echo "===> MAKE DYNAMIC"
	$(CLANG) -shared -o ${BUILD_DIR}/${DYN_LIB_NAME} $(OBJECTS) $(SHARED_OBJECTS) ${LINK_LIBS}

# Compile with LTO
lto-build-dir:
	@echo "===> LTO"
	@mkdir -p $(LTO_DIR)

${LTO_OBJECTS}: $(SOURCES)
	@echo "===> CC LTO"
	$(foreach var,$(SOURCES),$(CLANG) $(ALL_CFLAGS) -flto -c $(var) -o ${subst $(SRC_DIR),$(LTO_DIR),$(var:.c=.lto.o)};)

${LTO_SHARED_OBJECTS}: $(SHARED_SOURCES)
	@echo "===> CC LTO Shared"
	$(foreach var,$(SHARED_SOURCES),$(CLANG) $(ALL_CFLAGS) -flto -c $(var) -o ${subst $(SHARED_SRC_DIR),$(LTO_DIR),$(var:.c=.lto.o)};)

gold-available:
ifndef LLVM_GOLD
	$(error Cannot find the shared gold library. Set LLVM_GOLD to build the lto libs)
endif

lto-static: gold-available lto-build-dir ${LTO_OBJECTS} ${LTO_SHARED_OBJECTS} exports
	@echo "===> MAKE LTO STATIC"
	ar --plugin=$(LLVM_GOLD) $(ARFLAGS) $(LTO_DIR)/${STAT_LIB_NAME} ${LTO_OBJECTS} ${LTO_SHARED_OBJECTS}

format:
	@echo "===> CLANG-FORMAT"
	$(CLANG_FORMAT) -i $(sort $(SOURCES))
	$(CLANG_FORMAT) -i $(sort $(HEADERS))

clean:
	@echo "===> CLEAN"
	rm -rf ${BUILD_DIR}/ $(LTO_DIR)/
