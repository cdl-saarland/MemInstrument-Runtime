#include "softboundcets-common.h"
#include "softboundcets-defines.h"
#include "softboundcets-spatial.h"
#include "softboundcets-temporal.h"

#include "fail_function.h"
#include "statistics.h"

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

static int softboundcets_initialized = 0;

void __softboundcets_init(void) {
    if (softboundcets_initialized != 0) {
        return; // already initialized, do nothing
    }

    softboundcets_initialized = 1;

    __mi_debug_printf("Initializing softboundcets metadata space\n");

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

void __softboundcets_update_environment_metadata() {
    char *const *envPtr = environ;
    while (*envPtr != NULL) {
        // Store the length for individual environment variable
#if __SOFTBOUNDCETS_SPATIAL
        __softboundcets_metadata_store(envPtr, *envPtr,
                                       *envPtr + strlen(*envPtr) + 1);
#elif __SOFTBOUNDCETS_TEMPORAL
        __mi_fail_with_msg(
            "Missing implementation for enviornment variable access");
#elif __SOFTBOUNDCETS_SPATIAL_TEMPORAL
        __mi_fail_with_msg(
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
    __mi_fail_with_msg(
        "Missing implementation for enviornment variable access");
#elif __SOFTBOUNDCETS_SPATIAL_TEMPORAL
    __mi_fail_with_msg(
        "Missing implementation for enviornment variable access");
#endif
}

extern int softboundcets_pseudo_main(int argc, char **argv);

int main(int argc, char **argv) {

    // Setup for backtrace recovery and statistics if requested
    __setup_statistics(argv[0]);

    if (__is_llvm_tooling_function(argv[0])) {
        return softboundcets_pseudo_main(argc, argv);
    }

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
    __mi_debug_printf("Call the actual main...\n");
    return_value = softboundcets_pseudo_main(argc, new_argv);
    __softboundcets_deallocate_shadow_stack_space();

#if __SOFTBOUNDCETS_TEMPORAL || __SOFTBOUNDCETS_SPATIAL_TEMPORAL
    __softboundcets_stack_memory_deallocation(argv_key);
#endif
    return return_value;
}
