#pragma once

/// Bound below which pointer values are considered as NULL pointers
#define NULLPTR_BOUND 4096

/// Fail if a free call without witness is encountered (possible double free)
#define CHECK_DOUBLE_FREE 1

/// Fail if a dereference check without witness is encountered.
/// Do nothing otherwise
#define CRASH_ON_MISSING_WITNESS_DEREF 1

/// Fail if an invariant check without witness is encountered.
/// Do nothing otherwise
#define CRASH_ON_MISSING_WITNESS_INVAR 1

/// Fail if a requirement for bounds without witness is encountered.
/// Return wide bounds otherwise
#define CRASH_ON_MISSING_WITNESS_BOUND 1

#define MAX_BACKTRACE_LENGTH 16

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

