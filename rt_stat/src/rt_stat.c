#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <execinfo.h>
#include <unistd.h>

#define __USE_GNU
#include <dlfcn.h>

#define STATS_FILE "MI_STATS_FILE"

static const char* mi_prog_name = NULL;

static const char* mi_stats_file = NULL;

static uint64_t __NumStatEntries = 0;

typedef struct {
    uint64_t val;
    const char* text;
} StatType;

static StatType* __StatTable = NULL;

void __mi_stat_init(uint64_t num) {
    free(__StatTable);
    __NumStatEntries = num;

    __StatTable = malloc(sizeof(StatType) * num);
}

void __mi_stat_init_entry(uint64_t idx, const char* text) {
    __StatTable[idx].val = 0;
    __StatTable[idx].text = text;
}

void __mi_stat_inc(uint64_t idx) {
    ++__StatTable[idx].val;
}

void __mi_print_stats(void) {
    FILE* dest = stderr;
    if (mi_stats_file) {
        dest = fopen(mi_stats_file, "a");
        if (!dest) {
            fprintf(stderr, "Failed to open stats file `%s'", mi_stats_file);
            return;
        }
    }
    fprintf(dest, "\n==================================================\n");

    if (__StatTable == NULL) {
        fprintf(dest, "no meminstrument stat table allocated for `%s'!\n", mi_prog_name);
    } else {
        fprintf(dest, "meminstrument runtime stats for `%s':\n", mi_prog_name);

        for (size_t i = 0; i < __NumStatEntries; ++i) {
            fprintf(dest, "STAT  %s : %lu\n", __StatTable[i].text, __StatTable[i].val);
        }
    }
    fprintf(dest, "==================================================\n");
    fclose(dest);
}

typedef int (*fcn)(int *(main)(int, char **, char **), int argc, char **ubp_av, void (*init)(void), void (*fini) (void), void (*rtld_fini)(void), void (*stack_end));
int __libc_start_main(int *(main) (int, char **, char **), int argc, char **ubp_av, void (*init)(void), void (*fini)(void), void (*rtld_fini)(void), void (* stack_end)) {

    mi_prog_name = ubp_av[0];

#ifdef STATS_FILE
    mi_stats_file = getenv(STATS_FILE);
#endif

    if (atexit(__mi_print_stats) != 0) {
        fprintf(stderr, "meminstrument: Failed to register statistics printer!\n");
    }

    fcn start = (fcn)dlsym(RTLD_NEXT, "__libc_start_main");
    return (*start)(main, argc, ubp_av, init, fini, rtld_fini, stack_end);
}

