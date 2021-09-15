#include "softboundcets-common.h"
#include "softboundcets-defines.h"
#include "softboundcets-spatial.h"
#include "softboundcets-temporal.h"

#include <assert.h>
#include <stdlib.h>
#include <unistd.h>
#if defined(__linux__)
#include <malloc.h>
#endif
#include <ctype.h>
#include <limits.h>
#include <string.h>
#include <sys/mman.h>
#if !defined(__FreeBSD__)
#include <execinfo.h>
#endif

extern char **environ;

__softboundcets_trie_entry_t **__softboundcets_trie_primary_table;

shadow_stack_ptr_type __softboundcets_shadow_stack_ptr = NULL;

shadow_stack_ptr_type __softboundcets_shadow_stack_max = NULL;

// Use meminstruments mechanism to get a useful stack trace
static const char *mi_prog_name = NULL;

const char *__get_prog_name(void) { return mi_prog_name; }

#if __SOFTBOUNDCETS_ENABLE_RT_STATS

static size_t rt_stats_sb_access_checks = 0;
static size_t rt_stats_sb_mem_checks = 0;
static size_t rt_stats_external_checks = 0;

void __rt_stat_inc_sb_access_check(void) { rt_stats_sb_access_checks += 1; }

void __rt_stat_inc_sb_mem_check(void) { rt_stats_sb_mem_checks += 1; }

void __rt_stat_inc_external_check(void) { rt_stats_external_checks += 1; }

static void __print_stats(void) {
    FILE *dest = fopen("mi_stats.txt", "a");
    if (!dest) {
        fprintf(stderr, "Failed to open stats file 'mi_stats.txt'");
        return;
    }
    fprintf(dest, "\n==================================================\n");
    fprintf(dest, "hacked softbound runtime stats for '%s':\n", mi_prog_name);

    fprintf(dest, "STAT  # of softbound access checks executed : %lu\n",
            rt_stats_sb_access_checks);
    fprintf(dest, "STAT  # of other softbound checks executed : %lu\n",
            rt_stats_sb_mem_checks);
    fprintf(dest, "STAT  # of external checks executed : %lu\n",
            rt_stats_external_checks);

    fprintf(dest, "==================================================\n");
    fclose(dest);
}

#endif

__SOFTBOUNDCETS_NORETURN void __softboundcets_abort() {
    fprintf(
        stderr,
        "\nSoftboundcets: Memory safety violation detected\n\nBacktrace:\n");

    // Based on code from the backtrace man page
    size_t size;
    void *array[100];

    fprintf(stderr, "\n#################### meminstrument --- backtrace start "
                    "####################\n");
    fprintf(stderr, "> executable: %s\n", __get_prog_name());
#if !defined(__FreeBSD__)
    size = backtrace(array, 100);
    backtrace_symbols_fd(array, size, fileno(stderr));
#endif
    fprintf(stderr, "#################### meminstrument --- backtrace end "
                    "######################\n");
    fprintf(stderr, "\n\n");

    abort();
}

__SOFTBOUNDCETS_NORETURN void __softboundcets_abort_with_msg(const char *str) {
    __softboundcets_printf(str);
    __softboundcets_abort();
}

static int softboundcets_initialized = 0;

void __softboundcets_init(void) {
    if (softboundcets_initialized != 0) {
        return; // already initialized, do nothing
    }

    softboundcets_initialized = 1;

    __softboundcets_debug_printf("Initializing softboundcets metadata space\n");

    assert(sizeof(__softboundcets_trie_entry_t) >= 16);

/* Allocating the temporal shadow space */
#if __SOFTBOUNDCETS_TEMPORAL || __SOFTBOUNDCETS_SPATIAL_TEMPORAL
    __softboundcets_temporal_initialize_datastructures();
#endif

    size_t shadow_stack_size =
        __SOFTBOUNDCETS_SHADOW_STACK_ENTRIES * sizeof(size_t);
    __softboundcets_shadow_stack_ptr =
        mmap(0, shadow_stack_size, PROT_READ | PROT_WRITE,
             SOFTBOUNDCETS_MMAP_FLAGS, -1, 0);
    assert(__softboundcets_shadow_stack_ptr != (void *)-1);

    // Keep track of the limit of the shadow stack
    __softboundcets_shadow_stack_max =
        __softboundcets_shadow_stack_ptr + __SOFTBOUNDCETS_SHADOW_STACK_ENTRIES;

    *__softboundcets_shadow_stack_ptr = 0; /* prev stack size */
    shadow_stack_ptr_type current_size_shadow_stack_ptr =
        __softboundcets_shadow_stack_ptr + 1;
    *(current_size_shadow_stack_ptr) = 0;

    size_t length_trie = (__SOFTBOUNDCETS_TRIE_PRIMARY_TABLE_ENTRIES) *
                         sizeof(__softboundcets_trie_entry_t *);

    __softboundcets_trie_primary_table =
        mmap(0, length_trie, PROT_READ | PROT_WRITE, SOFTBOUNDCETS_MMAP_FLAGS,
             -1, 0);
    assert(__softboundcets_trie_primary_table != (void *)-1);

    int *temp = malloc(1);
    __softboundcets_allocation_secondary_trie_allocate_range(0, (size_t)temp);
}

void __softboundcets_printf(const char *str, ...) {
    va_list args;

    va_start(args, str);
    vfprintf(stderr, str, args);
    va_end(args);
}

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

void __softboundcets_update_environment_metadata() {
    char *const *envPtr = environ;
    while (*envPtr != NULL) {
        // Store the length for individual environment variable
#if __SOFTBOUNDCETS_SPATIAL
        __softboundcets_metadata_store(envPtr, *envPtr,
                                       *envPtr + strlen(*envPtr) + 1);
#elif __SOFTBOUNDCETS_TEMPORAL
        __softboundcets_abort_with_msg(
            "Missing implementation for enviornment variable access");
#elif __SOFTBOUNDCETS_SPATIAL_TEMPORAL
        __softboundcets_abort_with_msg(
            "Missing implementation for enviornment variable access");
#endif
        envPtr++;
    }
    // Determine the address one past the terminating NULL
    envPtr++;

#if __SOFTBOUNDCETS_SPATIAL
    // Store the valid range of the environment
    __softboundcets_metadata_store(&environ, environ,
                                   environ + (envPtr - environ));
#elif __SOFTBOUNDCETS_TEMPORAL
    __softboundcets_abort_with_msg(
        "Missing implementation for enviornment variable access");
#elif __SOFTBOUNDCETS_SPATIAL_TEMPORAL
    __softboundcets_abort_with_msg(
        "Missing implementation for enviornment variable access");
#endif
}

extern int softboundcets_pseudo_main(int argc, char **argv);

int main(int argc, char **argv) {

    // Setup for backtrace recovery
    set_prog_name(argv[0]);

#if __SOFTBOUNDCETS_LLVM_TESTSUITE
    char *sub = strstr(argv[0], "timeit");
    if (sub) {
        int retval = softboundcets_pseudo_main(argc, argv);
        return retval;
    }
#endif

#if __SOFTBOUNDCETS_ENABLE_RT_STATS
    if (atexit(__print_stats) != 0) {
        fprintf(stderr, "sb(hacked): Failed to register statistics printer!\n");
    }
#endif

    char **new_argv = argv;
    int i;
    char *temp_ptr;
    int return_value;
#if __SOFTBOUNDCETS_TEMPORAL || __SOFTBOUNDCETS_SPATIAL_TEMPORAL
    key_type argv_key;
    lock_type argv_loc;
#endif

    int *temp = malloc(1);
    __softboundcets_allocation_secondary_trie_allocate_range(0, (size_t)temp);

#if __SOFTBOUNDCETS_TEMPORAL || __SOFTBOUNDCETS_SPATIAL_TEMPORAL
    __softboundcets_stack_memory_allocation(&argv_loc, &argv_key);
#endif

#if defined(__linux__)
    mallopt(M_MMAP_MAX, 0);
#endif

    __softboundcets_update_environment_metadata();

    for (i = 0; i < argc; i++) {

#if __SOFTBOUNDCETS_SPATIAL

        __softboundcets_metadata_store(&new_argv[i], new_argv[i],
                                       new_argv[i] + strlen(new_argv[i]) + 1);

#elif __SOFTBOUNDCETS_TEMPORAL

        __softboundcets_metadata_store(&new_argv[i], argv_key, argv_loc);

#elif __SOFTBOUNDCETS_SPATIAL_TEMPORAL

        __softboundcets_metadata_store(&new_argv[i], new_argv[i],
                                       new_argv[i] + strlen(new_argv[i]) + 1,
                                       argv_key, argv_loc);

#endif
    }

    /* Santosh: Real Nasty hack because C programmers assume argv[argc]
     * to be NULL. Also this NUll is a pointer, doing + 1 will make the
     * size_of_type to fail
     */
    temp_ptr = ((char *)&new_argv[argc]) + 8;

    /* &new_argv[0], temp_ptr, argv_key, argv_loc * the metadata */

    __softboundcets_allocate_shadow_stack_space(1);

#if __SOFTBOUNDCETS_SPATIAL

    __softboundcets_store_base_shadow_stack(&new_argv[0], 0);
    __softboundcets_store_bound_shadow_stack(temp_ptr, 0);

#elif __SOFTBOUNDCETS_TEMPORAL

    __softboundcets_store_key_shadow_stack(argv_key, 0);
    __softboundcets_store_lock_shadow_stack(argv_loc, 0);

#elif __SOFTBOUNDCETS_SPATIAL_TEMPORAL

    __softboundcets_store_base_shadow_stack(&new_argv[0], 0);
    __softboundcets_store_bound_shadow_stack(temp_ptr, 0);
    __softboundcets_store_key_shadow_stack(argv_key, 0);
    __softboundcets_store_lock_shadow_stack(argv_loc, 0);

#endif
    __softboundcets_debug_printf("Call the actual main...\n");
    return_value = softboundcets_pseudo_main(argc, new_argv);
    __softboundcets_deallocate_shadow_stack_space();

#if __SOFTBOUNDCETS_TEMPORAL || __SOFTBOUNDCETS_SPATIAL_TEMPORAL
    __softboundcets_stack_memory_deallocation(argv_key);
#endif
    return return_value;
}
