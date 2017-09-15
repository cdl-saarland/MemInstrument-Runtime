#include "config.h"

#ifdef STATISTICS

#include "statistics.h"

#include <stdio.h>

// Define all statistics counters
#define STAT_ACTION(var, text) \
    size_t __##var = 0;

#include "statistic_counters.def"

#undef STAT_ACTION

static struct __StatEntry { size_t id; size_t* ptr; const char* text; }
    __StatRegistry[] = {

// Register all statistics counters
#define STAT_ACTION(var, text) \
    (struct __StatEntry){ __COUNTER__, & __##var, text },

#include "statistic_counters.def"

#undef STAT_ACTION
};

static size_t __NumStatEntries = __COUNTER__;\

void __print_stats(void) {
    fprintf(stderr, "\n==================================================\n");
    fprintf(stderr, "meminstrument runtime stats:\n");

    for (size_t i = 0; i < __NumStatEntries; ++i) {
        fprintf(stderr, "STAT  %s : %lu\n", __StatRegistry[i].text, *__StatRegistry[i].ptr);
    }

    fprintf(stderr, "==================================================\n");
}
#endif
