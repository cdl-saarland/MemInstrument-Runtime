/**
 * This file contains the implementation of statistics counters for use in
 * instrumentation mechanisms.
 * Consider include/statistics.h for usage information.
 */

#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "config.h"

#include "statistics.h"

static const char *mi_prog_name = NULL;

#ifdef STATISTICS

static const char *mi_stats_file = NULL;

// Define all statistics counters
#define STAT_ACTION(var, text) size_t __##var = 0;

#include STATS_COUNTER_DEFS

#undef STAT_ACTION

static struct __StatEntry {
    size_t id;
    size_t *ptr;
    const char *text;
} __StatRegistry[] = {

// Register all statistics counters
#define STAT_ACTION(var, text)                                                 \
    (struct __StatEntry){__COUNTER__, &__##var, text},

#include STATS_COUNTER_DEFS

#undef STAT_ACTION
};

static size_t __NumStatEntries = __COUNTER__;

static void __print_stats(void) {
    FILE *dest = stderr;
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
        fprintf(dest, "STAT  %s : %lu\n", __StatRegistry[i].text,
                *__StatRegistry[i].ptr);
    }

    fprintf(dest, "==================================================\n");
    fclose(dest);
}
#endif

const char *__get_prog_name(void) { return mi_prog_name; }

static void set_prog_name(const char *n) {
    char actualpath[PATH_MAX + 1];
    if (realpath(n, actualpath)) {
        size_t len = strlen(actualpath);
        char *res = malloc(sizeof(char) * (len + 1));
        memcpy(res, actualpath, len + 1);
        mi_prog_name = res;
    } else {
        mi_prog_name = n;
    }
}

void __setup_statistics(const char *n) {
    set_prog_name(n);
#ifdef STATISTICS
    if (atexit(__print_stats) != 0) {
        fprintf(stderr,
                "meminstrument: Failed to register statistics printer!\n");
    }
#ifdef STATS_FILE
    mi_stats_file = getenv(STATS_FILE);
#endif
#endif
}
