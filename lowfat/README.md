# Low Fat Pointers

This is a run-time implementation of the Low-Fat Pointers approach proposed by Duck & Yap. Philip Gebel contributed the heap protection (Publication 1.), and Tina Jung extended it with the stack and global variable protection (Publications 2.+3.).

## Configurations

This implementation only supports power-of-two allocation sizes. The smallest/largest allocation size is configurable, as well as the region sizes for heap/stack/globals.

### Config file options

The configuration file for Low-Fat Pointers is `lf_config.json`.

1) `HEAP_`-, `GLOBAL_`-, and `STACK_REGION_SIZE` define the amount of storage reserved for allocations of the each type per size.

2) `MIN_ALLOC_SIZE` defines the smallest allocation size to use. Must be a power of two.

3) `MAX_HEAP_`-, `MAX_STACK_`-, and `MAX_GLOBAL_ALLOC_SIZE` define the largest allocated object size which should be made low-fat. Allocations above this size are possible, but will be unprotected. Must be powers of two.

4) `STACK_SIZE` is the maximal size of the initial program stack. An error will be reported if the actual stack is larger than this boundary.

5) `SIZES_ADDRESS` and `MAGICS_ADDRESS` define the location for the arrays that contain sizes/magics (they are mmaped at these addresses).

6) `MAXIMAL_ADDRESS` defines the largest valid value for a pointer.

Usually, 1-3 are the only options you might want to reconfigure.

### Technical comment

The python script `generate_sizes_header_and_linker_script.py` reads the configuration and generates all files that depend on it.
For one, this is the linker script, which has to know all sizes to define proper sections.
It additionally defines a C header file, used by the Low-Fat run-time and the C++ Low-Fat pass.

## Publications

1) Duck, G. J., & Yap, R. H. (2016, March). Heap bounds protection with low fat pointers. In Proceedings of the 25th International Conference on Compiler Construction (pp. 132-142).

2) Duck, G. J., Yap, R. H., & Cavallaro, L. (2017, February). Stack Bounds Protection with Low Fat Pointers. In NDSS (Vol. 17, pp. 1-15).

3) Duck, G. J., & Yap, R. H. (2018). An extended low fat allocator API and applications. arXiv preprint arXiv:1804.04812.
