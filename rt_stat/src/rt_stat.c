#include "rt_stat.h"

#include <assert.h>
#include <execinfo.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include <dlfcn.h>

#define MIRT_STATS_FILE "MIRT_STATS_FILE"

static const char *mi_prog_name = NULL;

static const char *mi_stats_file = NULL;

typedef struct {
    uint64_t val;
    const char *text;
} StatType;

typedef struct {
    StatType *table;
    uint64_t num_elems;
} StatTableType;

/**
 * A table of stat tables
 */
static StatTableType *__StatTable = NULL;

static uint64_t __NumStatTables = 0;

/**
 *  Allocate a new stat table and return its ID.
 *  Calls to accessor functions from one source have to use the same ID.
 */
uint64_t __mi_stat_init(uint64_t num) {

    if (__StatTable == NULL) {
        __StatTable = malloc(sizeof(StatTableType));
    } else {
        __StatTable =
            realloc(__StatTable, sizeof(StatTableType) * (__NumStatTables + 1));
    }
    ++__NumStatTables;

    StatTableType *newElem = &__StatTable[__NumStatTables - 1];

    newElem->num_elems = num;
    newElem->table = malloc(sizeof(StatType) * num);

    return __NumStatTables - 1;
}

void __mi_stat_init_entry(uint64_t tableID, uint64_t idx, const char *text) {
    assert(tableID < __NumStatTables);
    StatTableType *entry = &__StatTable[tableID];
    assert(idx < entry->num_elems);
    entry->table[idx].val = 0;
    entry->table[idx].text = text;
}

void __mi_stat_inc(uint64_t tableID, uint64_t idx) {
    assert(tableID < __NumStatTables);
    StatTableType *entry = &__StatTable[tableID];
    assert(idx < entry->num_elems);
    ++entry->table[idx].val;
}

void __mi_print_stats(void) {
    FILE *dest = stderr;
    if (mi_prog_name == NULL) {
        mi_prog_name = "[name not set]";
    }
    if (mi_stats_file) {
        dest = fopen(mi_stats_file, "a");
        if (!dest) {
            fprintf(stderr, "Failed to open stats file `%s'", mi_stats_file);
            return;
        }
    }
    fprintf(dest, "\n==================================================\n");

    if (__StatTable == NULL) {
        fprintf(dest, "no meminstrument stat table allocated for '%s'!\n",
                mi_prog_name);
    } else {
        fprintf(dest, "meminstrument runtime stats for '%s':\n", mi_prog_name);

        for (uint64_t i = 0; i < __NumStatTables; ++i) {
            StatTableType *entry = &__StatTable[i];
            uint64_t bnd = entry->num_elems;
            for (uint64_t j = 0; j < bnd; ++j) {
                fprintf(dest, "STAT  %s : %lu\n", entry->table[j].text,
                        entry->table[j].val);
            }
        }
    }
    fprintf(dest, "==================================================\n");
    fclose(dest);
}

typedef int (*fcn)(int (*main)(int, char **, char **), int argc, char **ubp_av,
                   void (*init)(void), void (*fini)(void),
                   void (*rtld_fini)(void), void(*stack_end));
int __libc_start_main(int (*main)(int, char **, char **), int argc,
                      char **ubp_av, void (*init)(void), void (*fini)(void),
                      void (*rtld_fini)(void), void(*stack_end)) {

    mi_prog_name = ubp_av[0];

#ifdef MIRT_STATS_FILE
    mi_stats_file = getenv(MIRT_STATS_FILE);
#endif

    if (atexit(__mi_print_stats) != 0) {
        fprintf(stderr,
                "meminstrument: Failed to register statistics printer!\n");
    }

    fcn start = (fcn)dlsym(RTLD_NEXT, "__libc_start_main");
    return (*start)(main, argc, ubp_av, init, fini, rtld_fini, stack_end);
}
