# Run-time for SoftBound (implemented in MemInstrument)

This repository is a fork from a modified version of the run-time of [SoftBound+CETS 3.9](https://github.com/santoshn/SoftBoundCETS-3.9).
The C-sources are compiled to a stand-alone library for usage with SoftBound-instrumented programs.

## Configurations

TODO debug

TODO currently hardcoded to spatial

## Build instructions

`make`

`make clean`

### Build with Link-Time Optimization (LTO)

TODO how to llvm-gold

```
export LLVM_GOLD=<PATH_TO>/LLVMgold.so
make
```