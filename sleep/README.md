# Sleep

This mechanism does not provide memory safety guarantees. The sleep runtime can
be linked instead of splay, as it provides implementations for the same
functions. Instead of actually performing checks (and the like), it performs a
configured amount of volatile stores. This can give an estimate on how costly an
instrumentation is depending on the number of instructions performed. However,
the volatile stores tend to be more costly than existing solutions, such that
it is not really suitable for a lower bound.
