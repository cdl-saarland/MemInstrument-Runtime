CLANG := /home/tina/Build/softbound_debug_gold/bin/betterclang

# Use spatial checks only
CFLAGS := -Wall -pedantic -fPIC -O3 -D__SOFTBOUNDCETS_SPATIAL -D_GNU_SOURCE
ARFLAGS := -rcs
LINK_LIBS := -luuid -ldl -lcrypt

LIB_NAME := softboundcets_rt
DYN_LIB_NAME := lib${LIB_NAME}.so
STAT_LIB_NAME := lib${LIB_NAME}.a

SOURCES := $(wildcard *.c)
OBJECTS := $(SOURCES:.c=.o)

LTO_DIR := lto
LTO_OBJECTS := $(addprefix $(LTO_DIR)/, $(SOURCES:.c=.lto.o))

all: static gen_compile_commands

# If LLVM_GOLD is set, also build a library for use with LTO
#
# Note that the name of the library is hardcoded in the compiler. Thus, we call
# it the same, but put it in the lto/ subdirectory. To use it, pass
# -L/path/to/softboundcets-lib/lto to the compiler, and use -flto during
# compilation and linking
ifneq ($(LLVM_GOLD),)
all: static_lto
endif

# Write a compile_commands.json file
%.compdb_entry: %.c
	@echo "    {" > $@
	@echo "        \"command\": \"$(CLANG) $(CFLAGS) -c $<\","  >> $@
	@echo "        \"directory\": \"$(CURDIR)\","               >> $@
	@echo "        \"file\": \"$<\""                            >> $@
	@echo "    },"                                              >> $@

COMPDB_ENTRIES = $(addsuffix .compdb_entry, $(basename $(SOURCES)))

compile_commands.json: $(COMPDB_ENTRIES)
	@echo "[" > $@.tmp
	@cat $^ >> $@.tmp
	@sed '$$d' < $@.tmp > $@
	@echo "    }" >> $@
	@echo "]" >> $@
	@rm $@.tmp

gen_compile_commands: compile_commands.json
	@for f in $(COMPDB_ENTRIES); do rm $$f; done

# Compile without LTO static or dynamic
$(OBJECTS): $(SOURCES)
	$(foreach var,$(SOURCES),$(CLANG) $(CFLAGS) -c $(var) -o $(var:.c=.o);)

static: $(OBJECTS)
	ar $(ARFLAGS) ${STAT_LIB_NAME} $(OBJECTS)

dynamic: $(OBJECTS)
	$(CLANG) -shared -o ${DYN_LIB_NAME} $(OBJECTS) ${LINK_LIBS}

# Compile with LTO
lto_dir:
	@echo "LTO enabled, building with -flto..."
	@mkdir $(LTO_DIR)

${LTO_OBJECTS}: $(SOURCES)
	$(foreach var,$(SOURCES),$(CLANG) $(CFLAGS) -flto -c $(var) -o $(addprefix $(LTO_DIR)/, $(var:.c=.lto.o));)

static_lto: lto_dir ${LTO_OBJECTS}
	ar --plugin=$(LLVM_GOLD) $(ARFLAGS) lto/${STAT_LIB_NAME} ${LTO_OBJECTS}

clean:
	rm -rf compile_commands.json *.o *.a *.so *~ lto/
