#pragma once

#include <stddef.h>

// Declare all statistics counters
#define STAT_ACTION(var, text) \
    extern size_t __##var;

#include "statistic_counters.def"

#undef STAT_ACTION


#define STAT_INC(var) { ++ __##var; }

void __print_stats(void);

