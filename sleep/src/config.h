#pragma once

// The name of a weakly linked symbol that is called if available before
// executing the main function.
#define INIT_CALLBACK __mi_init_callback__

#define MAX_BACKTRACE_LENGTH 16

// Enable collecting and printing statistics at run time
// #define STATISTICS 1

#define STATS_COUNTER_DEFS "statistic_counters.def"

// If defined, use this key to find a file to print runtime stats to
#define STATS_FILE "MI_STATS_FILE"

