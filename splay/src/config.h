#pragma once

/// Bound below which pointer values are considered as NULL pointers
#define NULLPTR_BOUND 4096

#define MAX_BACKTRACE_LENGTH 10

// Enable collecting and printing statistics at run time
// #define STATISTICS 1

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

#ifdef STATISTICS
#define STATS(x) do {x;} while (false);
#else
#define STATS(x) do {} while (false);
#endif

#ifdef ASSERTS
#define ASSERTION(x) do {x;} while (false);
#else
#define ASSERTION(x) do {} while (false);
#endif


#ifdef DBG
#define DEBUG(x) do {x;} while (false);
#else
#define DEBUG(x) do {} while (false);
#endif

