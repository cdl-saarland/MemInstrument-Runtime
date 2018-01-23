#include "config.h"

#include <stdio.h>

const char* mi_prog_name = NULL;

#ifdef STATISTICS

#include "statistics.h"

const char* mi_stats_file = NULL;

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
    FILE* dest = stderr;
    if (mi_stats_file) {
        dest = fopen(mi_stats_file, "a");
        if (!dest) {
            fprintf(stderr, "Failed to open stats file '%s'", mi_stats_file);
            return;
        }
    }
    fprintf(dest, "\n==================================================\n");
    fprintf(dest, "meminstrument runtime stats for '%s':\n", mi_prog_name);

    for (size_t i = 0; i < __NumStatEntries; ++i) {
        fprintf(dest, "STAT  %s : %lu\n", __StatRegistry[i].text, *__StatRegistry[i].ptr);
    }

    fprintf(dest, "==================================================\n");
    fclose(dest);
}
#endif
