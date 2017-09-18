#pragma once

#include "config.h"

#ifdef STATISTICS

#include <stddef.h>

extern const char* stats_file;

// Declare all statistics counters
#define STAT_ACTION(var, text) \
    extern size_t __##var;

#include "statistic_counters.def"

#undef STAT_ACTION


#define STAT_INC(var) { ++ __##var; }

void __print_stats(void);

#else

#define STAT_INC(var) {;}

#endif
