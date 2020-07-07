all: static dynamic

# Use spatial checks only
CFLAGS=-Wall -pedantic -fPIC -O3 -D__SOFTBOUNDCETS_SPATIAL -D_GNU_SOURCE #-D__SOFTBOUNDCETS_DEBUG
ARFLAGS=-rcs

LIB_NAME=softboundcets_rt
DYN_LIB_NAME = lib${LIB_NAME}.so
STAT_LIB_NAME = lib${LIB_NAME}.a

# If LLVM_GOLD is set, also build a library for use with LTO
#
# Note that the name of the library is hardcoded in the compiler. Thus, we call
# it the same, but put it in the lto/ subdirectory. To use it, pass
# -L/path/to/softboundcets-lib/lto to the compiler, and use -flto during
# compilation and linking
ifneq ($(LLVM_GOLD),)
all: static_lto
endif

CLANG=/home/tina/Build/softbound_debug_gold/bin/betterclang

softboundcets_rt: softboundcets.h softboundcets-checks.c softboundcets.c softboundcets-wrappers.c
	$(CLANG) $(CFLAGS) -c softboundcets-checks.c -o softboundcets-checks.o
	$(CLANG) $(CFLAGS) -c softboundcets.c -o softboundcets.o
	$(CLANG) $(CFLAGS) -c softboundcets-wrappers.c -o softboundcets-wrappers.o

static: softboundcets_rt
	ar $(ARFLAGS) ${STAT_LIB_NAME} softboundcets.o softboundcets-checks.o softboundcets-wrappers.o

dynamic: softboundcets_rt
	$(CLANG) -shared -o ${DYN_LIB_NAME} softboundcets.o softboundcets-checks.o softboundcets-wrappers.o -luuid -ldl -lcrypt

softboundcets_rt_lto: softboundcets.h softboundcets-checks.c softboundcets.c softboundcets-wrappers.c
	mkdir lto
	$(CLANG) $(CFLAGS) -flto -c softboundcets-checks.c -o lto/softboundcets-checks.lto.o
	$(CLANG) $(CFLAGS) -flto -c softboundcets.c -o lto/softboundcets.lto.o
	$(CLANG) $(CFLAGS) -flto -c softboundcets-wrappers.c -o lto/softboundcets-wrappers.lto.o

static_lto: softboundcets_rt_lto
	ar --plugin=$(LLVM_GOLD) $(ARFLAGS) lto/${STAT_LIB_NAME} lto/softboundcets.lto.o lto/softboundcets-checks.lto.o lto/softboundcets-wrappers.lto.o

clean:
	rm -rf *.o *.a *.so *~ lto/
