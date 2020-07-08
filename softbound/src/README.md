# Run-time for SoftBound (implemented in MemInstrument)

This repository is a fork from a modified version of the run-time of [SoftBound+CETS 3.9](https://github.com/santoshn/SoftBoundCETS-3.9).
The C-sources are compiled to a stand-alone library for usage with SoftBound-instrumented programs.

## Build instructions

A static library (`libsoftboundcets_rt.a`) can be build by simply running

```
make
```

in this directory.
The Makefile supports `make clean` to clean the build directory.

### Build with Link-Time Optimization (LTO)

The library can be build LTO friendly.
The [LLVMgold plugin](http://llvm.org/docs/GoldPlugin.html) is necessary to do so.
Follow the instructions on "How to build it".
The steps on "Build the LLVMgold plugin" will generate `LLVMgold.so`, which you should find in the `lib` directory of your LLVM build.
You have to set the environment variable `LLVM_GOLD` to this library, and make will automatically build the additional library `lto/libsoftboundcets_rt.a`, which can be used with LTO.

```
export LLVM_GOLD=<PATH_TO>/LLVMgold.so
make
```

## Configurations

This library supports spatial as well as temporal memory safety checks.

All the below listed configuration can be used by inserting them into the `CFLAGS` line in the `Makefile`.

### Safety configurations

It can be configured to ensure only spatial (1), only temporal (2) or both (3):

* `-D__SOFTBOUNDCETS_SPATIAL`
* `-D__SOFTBOUNDCETS_TEMPORAL`
* `-D__SOFTBOUNDCETS_SPATIAL_TEMPORAL`

**Remark:** The current Makefile will build it with spatial safety only, as this fits our use case.

### Debugging

If you encounter issues with this library and you are eager to debug it, you can build it with `-D__SOFTBOUNDCETS_DEBUG`.
Be aware that this can cause a lot of output, especially on memory intense programs.

### Statistics

The library can currently track three kinds of statistics:

* The number of load+store dereference checks executed at run-time.
* The number of memcpy+memset checks executed at run-time.
* The number of external checks executed (this will be zero for the ordinary SoftBound instrumentation) executed at run-time.

Statistic tracking can be enabled with `-D ENABLE_RT_STATS`.
The results can be found in the `mi_stats.txt` file.

Note that statistic tracking influences the program execution time and should therefore be disabled for performance runs.

## Playing well with the instrumentation

Currently, crashes will occur when the instrumentation is configured to ensure spatial+temporal, but the run-time is configured for something else. Make sure to configure them the same way.

TODO automate this, the run-time can simply generate a header (or reuse the defined wrapper header) to ensure this doesn't happen.