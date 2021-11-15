/**
 * This file contains the implementation of statistics counters for use in
 * instrumentation mechanisms.
 * Consider include/statistics.h for usage information.
 */

#include <libgen.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "config.h"

#include "statistics.h"

static const char *mi_prog_name = NULL;

#ifdef MIRT_STATISTICS

static const char *mi_stats_file = NULL;

// Define all statistics counters
#define STAT_ACTION(var, text) size_t __##var = 0;

#include MIRT_STATS_COUNTER_DEFS

#undef STAT_ACTION

static struct __StatEntry {
    size_t id;
    size_t *ptr;
    const char *text;
} __StatRegistry[] = {

// Register all statistics counters
#define STAT_ACTION(var, text)                                                 \
    (struct __StatEntry){__COUNTER__, &__##var, text},

#include MIRT_STATS_COUNTER_DEFS

#undef STAT_ACTION
};

static size_t __NumStatEntries = __COUNTER__;

static void __print_stats(void) {
    FILE *dest = stderr;
    if (mi_stats_file) {
        dest = fopen(mi_stats_file, "a");
        if (!dest) {
            fprintf(stderr, "Failed to open stats file '%s'!\n", mi_stats_file);
            perror("Error while opening");
            abort();
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
#ifdef MIRT_STATISTICS
    if (atexit(__print_stats) != 0) {
        fprintf(stderr,
                "meminstrument: Failed to register statistics printer!\n");
    }
#ifdef STATS_FILE_ENV
    mi_stats_file = getenv(STATS_FILE_ENV);
#else
#ifdef MIRT_STATS_FILE
    mi_stats_file = MIRT_STATS_FILE;
#else
    char *file_name = "/mi_stats.txt";

    // Copy the program name (as dirname might modify its argument)
    size_t prog_name_len = strlen(mi_prog_name);
    char *copied_prog_name = malloc(prog_name_len + 1);
    strncpy(copied_prog_name, mi_prog_name, prog_name_len);
    copied_prog_name[prog_name_len] = '\0';

    // Get the path to the binary
    char *folder = dirname(copied_prog_name);

    // Generate the full statistics file name
    char *target = malloc(strlen(folder) + strlen(file_name) + 1);
    strncpy(target, folder, strlen(folder));
    strncat(target, file_name, strlen(file_name));
    mi_stats_file = target;

    // Free the copied program name after folder is also no longer required
    free(copied_prog_name);
#endif
#endif
#endif
}
