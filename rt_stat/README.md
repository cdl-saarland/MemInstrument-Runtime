This is the run-time library for the `meminstrument` statistics instrumentation.

It allows to allocate a table of `uint64_t`-string-pairs with an arbitrary number
of entries, to initialize these entries separately and to increment the `uint64_t`
component of each entry.

At the end of execution of a program that is linked against this library, the
statistics table is printed either to stderr or, if a filename is specified via
the `MIRT_STATS_FILE` environment variable, to the denoted file.

This library is currently not threadsafe!
