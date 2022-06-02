# Instrumentation Mechanisms

This repository contains the run-time libraries for various memory safety instrumentations.

## Build instructions

The instrumentation mechanisms and their build process are designed for/tested under Linux only.

### Requirements

* [Clang](https://clang.llvm.org/)
* [GNU Make](https://www.gnu.org/software/make/)
* Individual mechanisms may require more dependencies, see their `README.md`

All instrumentation mechanisms are built when executing

```
make
```

in this directory.
The generated libraries can be found in `lib`.
You can also selectively build only some mechanisms by entering their directory and executing `make`.
The Makefile supports a `make clean` target to clean up old builds.
Some instrumentations offer more targets, see their `README.md` for more details.

### Build with Link-Time Optimization (LTO)

The libraries can be built LTO friendly.
The [LLVMgold plugin](http://llvm.org/docs/GoldPlugin.html) is necessary to do so.
Follow the instructions on "How to build it".
The steps on "Build the LLVMgold plugin" will generate `LLVMgold.so`, which you should find in the `lib` directory of your LLVM build.
You have to set the environment variable `LLVM_GOLD` to this library, and make will automatically build the additional library in `ltobuild`, which can be used with LTO.

```
export LLVM_GOLD=<PATH_TO>/LLVMgold.so
make
```

## Structure

### Run-Time Libraries

This gives a brief overview of the mechanisms, for more details please refer to the `README.md` files in the subdirectory of each mechanism.

* `splay`

    Implementation of the splay-tree-based memory safety instrumentation proposed by Jones & Kelly.

* `softbound`

    Extended implementation of SoftBoundCETS as proposed by Nagarakatte et al.

* `lowfat`

    Implementation of Low Fat Pointers as proposed by Duck & Yap.

* `sleep`

    This mechanism does not provide any memory safety guarantees, but rather only executes stores to a volatile variable to spend time ("sleep").

* `rt_stat`

    This mechanism does not provide any memory safety guarantees, but rather only prints collected statistics.

### Utilities

* `include`

    The `include` folder contains auto-generated headers that are used by our C++ development.
    These headers communicate, for example, how the C run-time is configured.

* `lib`

    The `lib` folder contains symlinks to all (static/shared) libraries built.

* `shared`

    The `shared` folder contains functionality that is shared between the mechanisms, such as error reporting and statistics functions.

## Configuration

* `MIRT_REPORT_PTR_OVERFLOW`:
While computing a runtime check, it might happen that a pointer overflow occurs.

    A maliciously crafted pointer could be close the maximal available pointer value, and adding the access width to this value would result in an overflow and not trigger a memory safety violation.
    A related problem is that the access width might be very large (e.g., for memcpy), and thereby trigger an overflow.
    Usually, an access to such a large "pointer value" causes an error reported by the OS.

    The checks of Low-Fat Pointer and SoftBoundCETS can be extended to check for this corner case with `MIRT_REPORT_PTR_OVERFLOW`. Note that this is costly in terms of runtime: Upon every memory access, an additional comparison is required.

## Testing

In case you want to use the [llvm testing infrastructure](https://llvm.org/docs/lnt/quickstart.html), make sure to compile this library with `MIRT_IGNORE_LLVM_TOOLING`.
This avoids instrumenting the time measurement tools, which are built with the same compiler as the benchmarks.

## Debugging

If you encounter issues with this library and you are eager to debug it, you can build it with `MIRT_DEBUG`.
Be aware that this can cause a lot of output.

## Statistics

Statistic tracking can be enabled with `MIRT_STATISTICS`.
The results can be found in the `mi_stats.txt` file.

Note that statistic tracking influences the program execution time and should therefore be disabled for performance runs.

## Contributions

The splay/sleep/rt_stat implementations were contributed by **Fabian Ritter**.

The lowfat implementation (heap protection) was contributed by **Philip Gebel** and extended by the stack and global variable protection by **Tina Jung**.

The SoftBoundCETS implementation is an adaption by **Tina Jung**, and was originally developed by:

   **Santosh Nagarakatte**, Rutgers University,

   **Milo M K Martin**, University of Pennsylvania,

   **Steve Zdancewic**, University of Pennsylvania, and

   **Jianzhou Zhao**, University of Pennsylvania.
