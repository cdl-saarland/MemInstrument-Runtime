# Run-time for SoftBoundCETS (implemented in `MemInstrument`)

This repository is a fork from a modified version of the run-time of [SoftBound+CETS 3.9](https://github.com/santoshn/SoftBoundCETS-3.9).
The C-sources are compiled to a stand-alone library for usage with SoftBound-instrumented programs.

## Additional Requirements

* [Exuberant Ctags](http://ctags.sourceforge.net/) or [Universal Ctags](https://github.com/universal-ctags/ctags)

## Build instructions

A static and dynamic library (`libsoftboundcets_rt.a`/ `libsoftboundcets_rt.so`) can be built by simply running

```
make
```

in this directory.
The Makefile supports `make clean` to clean the build directory.

## Configurations

This library supports spatial as well as temporal memory safety checks.

### Safety configurations

It can be configured to ensure only spatial (1), only temporal (2) or both (3):

* `__SOFTBOUNDCETS_SPATIAL`
* `__SOFTBOUNDCETS_TEMPORAL`
* `__SOFTBOUNDCETS_SPATIAL_TEMPORAL`

**Remark:** The current Makefile will build it with spatial safety only, as this fits our use case.

This instrumentation provides standard library wrappers for two reasons:
1) To propagate metadata, e.g. to provide bounds for the pointer returned from a call to `malloc`.
2) To check safety guarantees, e.g. that `dest` is large enough to store `n` bytes when `memcpy(dest, src, n)` is called.

In case you want to disable 2), you can set `__SOFTBOUNDCETS_WRAPPER_CHECKS` to `0`.
This can be useful for testing the run-time overhead of these checks, but it gives fewer safety guarantees.

### Width zero checks

Some instrumented programs might execute checks on accesses of width zero.
If you are ok with out-of-bounds pointers from which zero byte are read or written to, use `__SOFTBOUNDCETS_ALLOWWIDTHZEROACCESS`.

Example which we encountered: `memcpy(dest, src, val)`, where `val` became `0` at some point and `src`/`dest` were out of bounds.

### Testing, Debugging, Statistics

The runtime libraries use common functionality for testing, debugging, and statistic counters. Please refer to the top-level `README.md` for general information about them.

The library can currently track three kinds of statistics:

* The number of load+store dereference checks executed at run-time.
* The number of memcpy+memset checks executed at run-time.
* The number of external checks executed (this will be zero for the ordinary SoftBound instrumentation) executed at run-time.

## Playing well with the instrumentation

The C++ part of SoftBound introduces calls to the run-time library to ensure memory safety. Which calls are introduced depends on the guarantees that should be given, i.e., temporal safety, spatial safety, or both.
In order to avoid configuring the C++ part and the C run-time differently, one cannot determine the configuration with command line arguments to the modified C++ compiler. The configuration of the C run-time is communicated to the LLVM pass via auto-generated headers, such that a misconfiguration is not possible (unless you modify the auto-generated headers manually, which you shouldn't).


## Publications

* Nagarakatte, S., Zhao, J., Martin, M. M., & Zdancewic, S. (2009, June). SoftBound: Highly compatible and complete spatial memory safety for C. In Proceedings of the 30th ACM SIGPLAN Conference on Programming Language Design and Implementation (pp. 245-258).

* Nagarakatte, S., Zhao, J., Martin, M. M., & Zdancewic, S. (2010, June). CETS: compiler enforced temporal safety for C. In Proceedings of the 2010 International Symposium on Memory Management (pp. 31-40).

* Nagarakatte, S., Martin, M. M., & Zdancewic, S. (2015). Everything you want to know about pointer-based checking. In 1st Summit on Advances in Programming Languages (SNAPL 2015). Schloss Dagstuhl-Leibniz-Zentrum fuer Informatik.

* Nagarakatte, S. G. (2012). Practical low-overhead enforcement of memory safety for C programs. University of Pennsylvania.

### Original authors' implementation

* LLVM 3.4-based SoftBoundCETS: https://github.com/santoshn/softboundcets-34
* (experimental) LLVM 3.9-based SoftBoundCETS: https://github.com/santoshn/SoftBoundCETS-3.9
