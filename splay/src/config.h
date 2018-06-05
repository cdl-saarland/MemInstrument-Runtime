#pragma once

/// Bound below which pointer values are considered as NULL pointers
#define NULLPTR_BOUND 4096

/// Fail if a free call without witness is encountered (possible double free)
#define CHECK_DOUBLE_FREE 1

/// Fail if a witness check fails.
/// Ensure non-crashing results otherwise.
#define CRASH_ON_MISSING_WITNESS 1

#define MAX_BACKTRACE_LENGTH 16

#define DUMP_ALLOCATION_MAP_ON_FAIL 1

// Enable potentially expensive and unsound lookups in the process memory
// map on missing witness
#define USE_MEMORY_MAP 1

// Consider only those regions that are backed by files (e.g. libraries,
// mmapped files, ...)
#define USE_MEMORY_MAP_FILEBACKED_ONLY 1

// Enable inserting mmapped allocations into splay tree, this might be
// problematic since munmap does not remove these allocations (which can lead to
// spurious violations).
// #define ENABLE_MMAP_HOOK 1

// Enable collecting and printing statistics at run time
// #define STATISTICS 1

#define STATS_COUNTER_DEFS "statistic_counters.def"

// If defined, use this key to find a file to print runtime stats to
#define STATS_FILE "MI_STATS_FILE"

// Enable Intel MPX bound checking on startup if defined to 1
// #define ENABLE_MPX 1

// Disable crashing when a fatal error is discovered
// #define CONTINUE_ON_FATAL 1

// Don't print anything (unless stats are enabled)
// #define SILENT 1

// Debugging assertions for splay tree
// #define ASSERTS 1
// #define DBG 1
// #define PRINT_TREE_INTERVAL 500000

#define TREE_ANNOTATE_NODES 1

