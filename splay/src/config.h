#pragma once

/// Bound below which pointer values are considered as NULL pointers
#define NULLPTR_BOUND 4096

/// Amount by which malloc'ed allocations should be increased
#define INCREASE_MALLOC_SIZE 1
// #define INCREASE_MALLOC_SIZE 0

/// Fail if a free call without witness is encountered (possible double free)
// #define CHECK_DOUBLE_FREE 1

/// Fail if a witness check fails.
/// Ensure non-crashing results otherwise.
// #define CRASH_ON_MISSING_WITNESS 1

/// The maximum length of backtraces that are printed
#define MAX_BACKTRACE_LENGTH 16

/// Dumps the entire allocation map of the splay tree when failing
#define DUMP_ALLOCATION_MAP_ON_FAIL 1

/// Enable potentially expensive and unsound lookups in the process memory
/// map on missing witness
#define USE_MEMORY_MAP 1

/// Consider only those regions that are backed by files (e.g. libraries,
/// mmapped files, ...)
#define USE_MEMORY_MAP_FILEBACKED_ONLY 1

/// Use nm to look up statically linked globals
#define INSERT_GLOBALS_FROM_BINARY 1

/// Enable inserting mmapped allocations into splay tree, this might be
/// problematic since munmap does not remove these allocations (which can lead
/// to spurious violations).
// #define ENABLE_MMAP_HOOK 1

/// Enable collecting and printing statistics at run time
// #define STATISTICS 1


/// Do not run anything and return early before calling the main function.
///   "Semantics is what makes programs slow"
// #define START_MAIN_EARLY_RETURN 1

#define STATS_COUNTER_DEFS "statistic_counters.def"

/// If defined, use this key to find a file to print runtime stats to
#define STATS_FILE "MI_STATS_FILE"

/// Enable Intel MPX bound checking on startup if defined to 1
// #define ENABLE_MPX 1

/// Disable crashing when a fatal error is discovered
// #define CONTINUE_ON_FATAL 1

/// Don't print anything (unless stats are enabled)
// #define SILENT 1

#define AL_NONE 0
#define AL_INPUT 1
#define AL_TREE 2
#define AL_TREE_COMPLEX 3

/// Debugging assertion level
///  AL_NONE:         no assertions
///  AL_INPUT:        input validation assertions
///  AL_TREE:         simple splay tree assertions
///  AL_TREE_COMPLEX: heavy-weight splay tree assertions
#define ASSERT_LEVEL AL_INPUT

/// print splay tree regularly if defined
// #define PRINT_TREE_INTERVAL 500000

/// Annotate tree nodes with their kind, slightly increases memory consumption
/// but improves error messages and allows further assertions
#define TREE_ANNOTATE_NODES 1

/// Enables very heavy-weight tracing of all memory operations into the defined
/// file if defined
// #define TRACER_FILE "mi-splay.trace"
