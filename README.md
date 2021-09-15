# Instrumentation Mechanisms

This repository contains the run-time libraries for various memory safety instrumentations.

## Build instructions

The instrumentation mechanisms and their build process are designed for/tested under Linux only.

### Requirements

* [Clang](https://clang.llvm.org/)
* [GNU Make](https://www.gnu.org/software/make/)

All instrumentation mechanisms are build when executing

```
make
```

in this directory.
The generated libraries can be found in `lib`.
You can also selectively build only some mechanisms by entering their directory and executing `make`.
The Makefile supports a `make clean` target to clean up old builds.
Some instrumentations offer more targets, see their `README.md` for more details.

## Structure

### Run-Time Libraries

This gives a brief overview of the mechanisms, for more details please refer to the `README.md` files in the subdirectory of each mechanism.

* `splay`

    Implementation of the splay tree based memory safety instrumentation proposed by Jones & Kelly.

* `softbound`

    Extended implementation of SoftBoundCETS as proposed by Nagarakatte et al.

* `lowfat`

    Implementation of Low Fat Pointers as proposed by Duck & Yap.

* `sleep`

    This mechanism does not provide any memory safety guarantees, but rather only executes stores to a volatile variable to spent time ("sleep").

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

## Contributions

The splay/sleep/rt_stat implementations are due to **Fabian Ritter**.

The lowfat implementation is due to **Philip Gebel**.

The SoftBoundCETS implementation is an adaption by **Tina Jung**, and was originally developed by:

   **Santosh Nagarakatte**, Rutgers University,

   **Milo M K Martin**, University of Pennsylvania,

   **Steve Zdancewic**, University of Pennsylvania, and

   **Jianzhou Zhao**, University of Pennsylvania.
