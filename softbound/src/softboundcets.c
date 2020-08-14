//=== runtime/softboundcets.c --- Provides the Runtime Setup -------*- C -*===//
// Copyright (c) 2016 Santosh Nagarakatte. All rights reserved.

// Developed by: Santosh Nagarakatte, Rutgers University
//               http://www.cs.rutgers.edu/~santosh.nagarakatte/softbound/

// The  SoftBoundCETS project had contributions from:
// Santosh Nagarakatte, Rutgers University,
// Milo M K Martin, University of Pennsylvania,
// Steve Zdancewic, University of Pennsylvania,
// Jianzhou Zhao, University of Pennsylvania

// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to
// deal with the Software without restriction, including without limitation the
// rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
// sell copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:

//   1. Redistributions of source code must retain the above copyright notice,
//      this list of conditions and the following disclaimers.

//   2. Redistributions in binary form must reproduce the above copyright
//      notice, this list of conditions and the following disclaimers in the
//      documentation and/or other materials provided with the distribution.

//   3. Neither the names of its developers nor the names of its
//      contributors may be used to endorse or promote products
//      derived from this software without specific prior written
//      permission.

// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL THE
// CONTRIBUTORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
// FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
// WITH THE SOFTWARE.
//===---------------------------------------------------------------------===//

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#if defined(__linux__)
#include <malloc.h>
#endif
#include <ctype.h>
#include <limits.h>
#include <stdarg.h>
#include <string.h>
#include <sys/mman.h>
#if !defined(__FreeBSD__)
#include <execinfo.h>
#endif

#include "softboundcets-common.h"
#include "softboundcets-defines.h"
#include "softboundcets-spatial.h"
#include "softboundcets-temporal.h"

// Use meminstruments mechanism to get a useful stack trace
static const char *mi_prog_name = NULL;

__softboundcets_trie_entry_t **__softboundcets_trie_primary_table;

size_t *__softboundcets_shadow_stack_ptr = NULL;

/* key 0 means not used, 1 is for  globals*/
size_t __softboundcets_deref_check_count = 0;
size_t *__softboundcets_global_lock = 0;

void *malloc_address = NULL;

const char *__get_prog_name(void) { return mi_prog_name; }

#if ENABLE_RT_STATS

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

#if NOERRORS
void __softboundcets_abort() {}
#else
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
#endif

static int softboundcets_initialized = 0;

__NO_INLINE void __softboundcets_stub(void) { return; }
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

    *((size_t *)__softboundcets_shadow_stack_ptr) = 0; /* prev stack size */
    size_t *current_size_shadow_stack_ptr =
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

static void softboundcets_init_ctype() {
#if defined(__linux__)

    char *ptr;
    char *base_ptr;

    ptr = (void *)__ctype_b_loc();
    base_ptr = (void *)(*(__ctype_b_loc()));
    __softboundcets_allocation_secondary_trie_allocate(base_ptr);

#if __SOFTBOUNDCETS_SPATIAL
    __softboundcets_metadata_store(ptr, ((char *)base_ptr - 129),
                                   ((char *)base_ptr + 256));

#elif __SOFTBOUNDCETS_TEMPORAL
    __softboundcets_metadata_store(ptr, 1, __softboundcets_global_lock);

#elif __SOFTBOUNDCETS_SPATIAL_TEMPORAL
    __softboundcets_metadata_store(ptr, ((char *)base_ptr - 129),
                                   ((char *)base_ptr + 256), 1,
                                   __softboundcets_global_lock);

#endif

#endif // __linux ends
}

void __softboundcets_printf(const char *str, ...) {
#if NOERRORS
#else
    va_list args;

    va_start(args, str);
    vfprintf(stderr, str, args);
    va_end(args);
#endif
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

extern int softboundcets_pseudo_main(int argc, char **argv);

int main(int argc, char **argv) {

    // Setup for backtrace recovery
    set_prog_name(argv[0]);

#if ENABLE_RT_STATS
    if (atexit(__print_stats) != 0) {
        fprintf(stderr, "sb(hacked): Failed to register statistics printer!\n");
    }
#endif

    char **new_argv = argv;
    int i;
    char *temp_ptr;
    int return_value;
#if __SOFTBOUNDCETS_TEMPORAL || __SOFTBOUNDCETS_SPATIAL_TEMPORAL
    size_t argv_key;
    void *argv_loc;
#endif

    int *temp = malloc(1);
    malloc_address = temp;
    __softboundcets_allocation_secondary_trie_allocate_range(0, (size_t)temp);

#if __SOFTBOUNDCETS_TEMPORAL || __SOFTBOUNDCETS_SPATIAL_TEMPORAL
    __softboundcets_stack_memory_allocation(&argv_loc, &argv_key);
#endif

#if defined(__linux__)
    mallopt(M_MMAP_MAX, 0);
#endif

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

    softboundcets_init_ctype();

    /* Santosh: Real Nasty hack because C programmers assume argv[argc]
     * to be NULL. Also this NUll is a pointer, doing + 1 will make the
     * size_of_type to fail
     */
    temp_ptr = ((char *)&new_argv[argc]) + 8;

    /* &new_argv[0], temp_ptr, argv_key, argv_loc * the metadata */

    __softboundcets_allocate_shadow_stack_space(2);

#if __SOFTBOUNDCETS_SPATIAL

    __softboundcets_store_base_shadow_stack(&new_argv[0], 1);
    __softboundcets_store_bound_shadow_stack(temp_ptr, 1);

#elif __SOFTBOUNDCETS_TEMPORAL

    __softboundcets_store_key_shadow_stack(argv_key, 1);
    __softboundcets_store_lock_shadow_stack(argv_loc, 1);

#elif __SOFTBOUNDCETS_SPATIAL_TEMPORAL

    __softboundcets_store_base_shadow_stack(&new_argv[0], 1);
    __softboundcets_store_bound_shadow_stack(temp_ptr, 1);
    __softboundcets_store_key_shadow_stack(argv_key, 1);
    __softboundcets_store_lock_shadow_stack(argv_loc, 1);

#endif

    return_value = softboundcets_pseudo_main(argc, new_argv);
    __softboundcets_deallocate_shadow_stack_space();

#if __SOFTBOUNDCETS_TEMPORAL || __SOFTBOUNDCETS_SPATIAL_TEMPORAL
    __softboundcets_stack_memory_deallocation(argv_key);
#endif
    return return_value;
}
