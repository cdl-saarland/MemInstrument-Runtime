#include "softboundcets-common.h"

#include "softboundcets-spatial.h"
#include "softboundcets-temporal.h"

#include <assert.h>
#include <limits.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>

extern __softboundcets_trie_entry_t **__softboundcets_trie_primary_table;

__WEAK_INLINE void
__softboundcets_allocate_shadow_stack_space(int num_pointer_args) {

    shadow_stack_ptr_type prev_stack_size_ptr =
        __softboundcets_shadow_stack_ptr + 1;
    size_t prev_stack_size = *prev_stack_size_ptr;

    ssize_t size = num_pointer_args * __SOFTBOUNDCETS_METADATA_NUM_FIELDS;

    // Make sure to not overflow the shadow stack
    // TODO this should rather extend the size of the shadow stack than throwing
    // an assertion
    assert(((uintptr_t)__softboundcets_shadow_stack_ptr <
            (uintptr_t)(__softboundcets_shadow_stack_max - prev_stack_size - 2 -
                        size)) &&
           "Shadow stack size exceeded. Try a larger value for "
           "__SOFTBOUNDCETS_SHADOW_STACK_ENTRIES.");

    __softboundcets_shadow_stack_ptr =
        __softboundcets_shadow_stack_ptr + prev_stack_size + 2;

    *__softboundcets_shadow_stack_ptr = prev_stack_size;
    shadow_stack_ptr_type current_stack_size_ptr =
        __softboundcets_shadow_stack_ptr + 1;

    *current_stack_size_ptr = size;
}

__WEAK_INLINE void __softboundcets_deallocate_shadow_stack_space() {

    shadow_stack_ptr_type reserved_space_ptr = __softboundcets_shadow_stack_ptr;

    size_t read_value = *reserved_space_ptr;

    assert(read_value >= 0 &&
           read_value <= __SOFTBOUNDCETS_SHADOW_STACK_ENTRIES);

    __softboundcets_shadow_stack_ptr =
        __softboundcets_shadow_stack_ptr - read_value - 2;
}

__WEAK_INLINE __softboundcets_trie_entry_t *__softboundcets_trie_allocate() {

    size_t length = (__SOFTBOUNDCETS_TRIE_SECONDARY_TABLE_ENTRIES) *
                    sizeof(__softboundcets_trie_entry_t);
    __softboundcets_trie_entry_t *secondary_entry = mmap(
        0, length, PROT_READ | PROT_WRITE, SOFTBOUNDCETS_MMAP_FLAGS, -1, 0);
    // assert(secondary_entry != (void*)-1);
    __softboundcets_debug_printf("snd trie table %p %lx\n", secondary_entry,
                                 length);
    return secondary_entry;
}

/* Metadata store parameterized by the mode of checking */

#if __SOFTBOUNDCETS_SPATIAL

__METADATA_INLINE void __softboundcets_metadata_store(const void *addr_of_ptr,
                                                      void *base, void *bound) {

#elif __SOFTBOUNDCETS_TEMPORAL

__METADATA_INLINE void __softboundcets_metadata_store(const void *addr_of_ptr,
                                                      key_type key,
                                                      lock_type lock) {

#elif __SOFTBOUNDCETS_SPATIAL_TEMPORAL

__METADATA_INLINE void __softboundcets_metadata_store(const void *addr_of_ptr,
                                                      void *base, void *bound,
                                                      key_type key,
                                                      lock_type lock) {
#endif

    size_t ptr = (size_t)addr_of_ptr;
    size_t primary_index;
    __softboundcets_trie_entry_t *trie_secondary_table;

    primary_index = (ptr >> 25);
    trie_secondary_table = __softboundcets_trie_primary_table[primary_index];

#if !__SOFTBOUNDCETS_PREALLOCATE_TRIE
    if (trie_secondary_table == NULL) {
        trie_secondary_table = __softboundcets_trie_allocate();
        __softboundcets_trie_primary_table[primary_index] =
            trie_secondary_table;
    }
    __softboundcets_debug_printf(
        "\t[metadata_store not prealloced] addr_of_ptr=%p, primary_index=%zx, "
        "trie_secondary_table=%p\n ",
        addr_of_ptr, primary_index, trie_secondary_table);
    assert(trie_secondary_table != NULL);
#endif

    size_t secondary_index = ((ptr >> 3) & 0x3fffff);
    __softboundcets_trie_entry_t *entry_ptr =
        &trie_secondary_table[secondary_index];

#if __SOFTBOUNDCETS_SPATIAL

    entry_ptr->base = base;
    entry_ptr->bound = bound;

    __softboundcets_debug_printf(
        "\t[metadata_store] addr_of_ptr=%p (points to %p), base=%p, bound=%p, "
        "primary_index=%zx, secondary_index=%zx, trie_entry_addr=%p\n",
        addr_of_ptr, addr_of_ptr ? *(void **)addr_of_ptr : NULL, base, bound,
        primary_index, secondary_index, entry_ptr);

#elif __SOFTBOUNDCETS_TEMPORAL

    entry_ptr->key = key;
    entry_ptr->lock = lock;

#elif __SOFTBOUNDCETS_SPATIAL_TEMPORAL

    entry_ptr->base = base;
    entry_ptr->bound = bound;
    entry_ptr->key = key;
    entry_ptr->lock = lock;

#endif

    return;
}

#if __SOFTBOUNDCETS_SPATIAL

__METADATA_INLINE void __softboundcets_metadata_load(const void *addr_of_ptr,
                                                     void **base,
                                                     void **bound) {

#elif __SOFTBOUNDCETS_TEMPORAL

__METADATA_INLINE void __softboundcets_metadata_load(const void *addr_of_ptr,
                                                     key_type *key,
                                                     lock_type *lock) {

#elif __SOFTBOUNDCETS_SPATIAL_TEMPORAL

__METADATA_INLINE void __softboundcets_metadata_load(const void *addr_of_ptr,
                                                     void **base, void **bound,
                                                     key_type *key,
                                                     lock_type *lock) {

#endif

    size_t ptr = (size_t)addr_of_ptr;

    size_t primary_index = (ptr >> 25);
    __softboundcets_trie_entry_t *trie_secondary_table =
        __softboundcets_trie_primary_table[primary_index];

#if !__SOFTBOUNDCETS_PREALLOCATE_TRIE
    if (trie_secondary_table == NULL) {

#if __SOFTBOUNDCETS_SPATIAL || __SOFTBOUNDCETS_SPATIAL_TEMPORAL
        *((void **)base) = 0;
        *((void **)bound) = 0;
#endif
#if __SOFTBOUNDCETS_TEMPORAL || __SOFTBOUNDCETS_SPATIAL_TEMPORAL
        *((key_type *)key) = 0;
        *((lock_type *)lock) = 0;
#endif
        return;
    }
#endif /* PREALLOCATE_ENDS */

    /* MAIN SOFTBOUNDCETS LOAD WHICH RUNS ON THE NORMAL MACHINE */
    size_t secondary_index = ((ptr >> 3) & 0x3fffff);
    __softboundcets_trie_entry_t *entry_ptr =
        &trie_secondary_table[secondary_index];

#if __SOFTBOUNDCETS_SPATIAL || __SOFTBOUNDCETS_SPATIAL_TEMPORAL
    *((void **)base) = entry_ptr->base;
    *((void **)bound) = entry_ptr->bound;

    __softboundcets_debug_printf(
        "\t[metadata_load] addr_of_ptr=%p (points to %p), base=%p, bound=%p, "
        "primary_index=%zx, secondary_index=%zx, trie_entry_addr=%p\n",
        addr_of_ptr, addr_of_ptr ? *(void **)addr_of_ptr : NULL, *base, *bound,
        primary_index, secondary_index, entry_ptr);

#endif
#if __SOFTBOUNDCETS_TEMPORAL || __SOFTBOUNDCETS_SPATIAL_TEMPORAL
    *((key_type *)key) = entry_ptr->key;
    *((lock_type *)lock) = (lock_type *)entry_ptr->lock;
#endif
}

__METADATA_INLINE
void __softboundcets_copy_metadata(void *dest, const void *from, size_t size) {

    __softboundcets_debug_printf("[Copy metadata] dest=%p, from=%p, size=%zx\n",
                                 dest, from, size);

    size_t from_ptr = (size_t)from;

    if (from_ptr % 8 != 0) {
        // TODO should this maybe raise an error if the size is larger than 8?
        // (likely to crash later on in case pointers are involved...)
        return;
    }

    size_t dest_ptr = (size_t)dest;
    size_t dest_ptr_end = dest_ptr + size;

    size_t from_ptr_end = from_ptr + size;

    size_t dest_primary_index_begin = (dest_ptr >> 25);
    size_t dest_primary_index_end = (dest_ptr_end >> 25);

    size_t from_primary_index_begin = (from_ptr >> 25);
    size_t from_primary_index_end = (from_ptr_end >> 25);

    if ((from_primary_index_begin != from_primary_index_end) ||
        (dest_primary_index_begin != dest_primary_index_end)) {

        size_t from_sizet = from_ptr;
        size_t dest_sizet = dest_ptr;

        size_t trie_size = size;
        size_t index = 0;

        for (index = 0; index < trie_size; index = index + 8) {

            size_t temp_from_pindex = (from_sizet + index) >> 25;
            size_t temp_to_pindex = (dest_sizet + index) >> 25;

            size_t dest_secondary_index =
                (((dest_sizet + index) >> 3) & 0x3fffff);
            size_t from_secondary_index =
                (((from_sizet + index) >> 3) & 0x3fffff);

            __softboundcets_trie_entry_t *temp_from_strie =
                __softboundcets_trie_primary_table[temp_from_pindex];

            // In case no pointer data is stored in this memory area, we don't
            // need to copy any metadata
            if (temp_from_strie == NULL) {
                continue;
            }

            __softboundcets_trie_entry_t *temp_to_strie =
                __softboundcets_trie_primary_table[temp_to_pindex];

            if (temp_to_strie == NULL) {
                temp_to_strie = __softboundcets_trie_allocate();
                __softboundcets_trie_primary_table[temp_to_pindex] =
                    temp_to_strie;
            }

            void *dest_entry_ptr = &temp_to_strie[dest_secondary_index];
            void *from_entry_ptr = &temp_from_strie[from_secondary_index];

            memcpy(dest_entry_ptr, from_entry_ptr,
                   sizeof(__softboundcets_trie_entry_t));
        }
        return;
    }

    // This is the simple case: In both source and dest, only entries of one
    // secondary table are affected. This allows for a single memcpy
    // transferring all the data.

    __softboundcets_trie_entry_t *trie_secondary_table_dest_begin =
        __softboundcets_trie_primary_table[dest_primary_index_begin];
    __softboundcets_trie_entry_t *trie_secondary_table_from_begin =
        __softboundcets_trie_primary_table[from_primary_index_begin];

    // In case no pointer data is stored in this memory area, we don't need to
    // copy any metadata
    if (trie_secondary_table_from_begin == NULL)
        return;

    // Allocate the secondary trie for the destination in case it does not yet
    // exist
    if (trie_secondary_table_dest_begin == NULL) {
        trie_secondary_table_dest_begin = __softboundcets_trie_allocate();
        __softboundcets_trie_primary_table[dest_primary_index_begin] =
            trie_secondary_table_dest_begin;
    }

    // Compute the secondary indices
    size_t dest_secondary_index = ((dest_ptr >> 3) & 0x3fffff);
    size_t from_secondary_index = ((from_ptr >> 3) & 0x3fffff);

    assert(dest_secondary_index < __SOFTBOUNDCETS_TRIE_SECONDARY_TABLE_ENTRIES);
    assert(from_secondary_index < __SOFTBOUNDCETS_TRIE_SECONDARY_TABLE_ENTRIES);

    // Copy the secondary table from src to dest
    void *dest_entry_ptr =
        &trie_secondary_table_dest_begin[dest_secondary_index];
    void *from_entry_ptr =
        &trie_secondary_table_from_begin[from_secondary_index];

    memcpy(dest_entry_ptr, from_entry_ptr,
           sizeof(__softboundcets_trie_entry_t) * (size >> 3));
    return;
}

/******************************************************************************/

__WEAK_INLINE void
__softboundcets_allocation_secondary_trie_allocate_range(void *initial_ptr,
                                                         size_t size) {

#if !__SOFTBOUNDCETS_PREALLOCATE_TRIE
    return;
#endif

    void *addr_of_ptr = initial_ptr;
    size_t start_addr_of_ptr = (size_t)addr_of_ptr;
    size_t start_primary_index = start_addr_of_ptr >> 25;

    size_t end_addr_of_ptr = (size_t)((char *)initial_ptr + size);
    size_t end_primary_index = end_addr_of_ptr >> 25;

    for (; start_primary_index <= end_primary_index; start_primary_index++) {

        __softboundcets_trie_entry_t *trie_secondary_table =
            __softboundcets_trie_primary_table[start_primary_index];
        if (trie_secondary_table == NULL) {
            trie_secondary_table = __softboundcets_trie_allocate();
            __softboundcets_trie_primary_table[start_primary_index] =
                trie_secondary_table;
        }
    }
}

__WEAK_INLINE void
__softboundcets_allocation_secondary_trie_allocate(void *addr_of_ptr) {

    /* URGENT: THIS FUNCTION REQUIRES REWRITE */

#if !__SOFTBOUNDCETS_PREALLOCATE_TRIE
    return;
#endif

    size_t ptr = (size_t)addr_of_ptr;
    size_t primary_index = (ptr >> 25);
    //  size_t secondary_index = ((ptr >> 3) & 0x3fffff);

    __softboundcets_trie_entry_t *trie_secondary_table =
        __softboundcets_trie_primary_table[primary_index];

    if (trie_secondary_table == NULL) {
        trie_secondary_table = __softboundcets_trie_allocate();
        __softboundcets_trie_primary_table[primary_index] =
            trie_secondary_table;
    }

    __softboundcets_trie_entry_t *trie_secondary_table_second_entry =
        __softboundcets_trie_primary_table[primary_index + 1];

    if (trie_secondary_table_second_entry == NULL) {
        __softboundcets_trie_primary_table[primary_index + 1] =
            __softboundcets_trie_allocate();
    }

    if (primary_index != 0 &&
        (__softboundcets_trie_primary_table[primary_index - 1] == NULL)) {
        __softboundcets_trie_primary_table[primary_index - 1] =
            __softboundcets_trie_allocate();
    }

    return;
}

//===----------------------------------------------------------------------===//
//                    VarArg Proxy Object Handling
//===----------------------------------------------------------------------===//

shadow_stack_ptr_type *__softboundcets_allocate_va_arg_proxy(int arg_no) {
    shadow_stack_ptr_type *proxy = malloc(sizeof(shadow_stack_ptr_type *));

    // Calculate the location to the arg_no metadata
    size_t offset = 2 + arg_no * __SOFTBOUNDCETS_METADATA_NUM_FIELDS;
    shadow_stack_ptr_type loc = __softboundcets_shadow_stack_ptr + offset;

    // Store the value in the proxy object
    *proxy = loc;

    __softboundcets_debug_printf("Allocated proxy %p, pointing to %p (current "
                                 "shadow stack height: %p)\n",
                                 proxy, loc, __softboundcets_shadow_stack_ptr);
    return proxy;
}

#if __SOFTBOUNDCETS_SPATIAL
__METADATA_INLINE void
__softboundcets_next_va_arg_metadata(shadow_stack_ptr_type *va_arg_proxy,
                                     void **base, void **bound) {
#elif __SOFTBOUNDCETS_TEMPORAL
__METADATA_INLINE void
__softboundcets_next_va_arg_metadata(shadow_stack_ptr_type *va_arg_proxy,
                                     key_type *key, lock_type *lock) {
#elif __SOFTBOUNDCETS_SPATIAL_TEMPORAL
__METADATA_INLINE void
__softboundcets_next_va_arg_metadata(shadow_stack_ptr_type *va_arg_proxy,
                                     void **base, void **bound, key_type *key,
                                     lock_type *lock) {
#endif
    __softboundcets_debug_printf("Proxy %p (current shadow stack height: %p)\n",
                                 *va_arg_proxy,
                                 __softboundcets_shadow_stack_ptr);
    // Load the metadata
#if __SOFTBOUNDCETS_SPATIAL || __SOFTBOUNDCETS_SPATIAL_TEMPORAL
    *base = *((void **)(*va_arg_proxy + __BASE_INDEX));
    *bound = *((void **)(*va_arg_proxy + __BOUND_INDEX));
    __softboundcets_debug_printf("Loaded base %p, loaded bound %p\n", *base,
                                 *bound);
#endif

#if __SOFTBOUNDCETS_TEMPORAL || __SOFTBOUNDCETS_SPATIAL_TEMPORAL
    *key = *((key_type *)(*va_arg_proxy + __KEY_INDEX));
    *lock = *((lock_type *)(*va_arg_proxy + __LOCK_INDEX));
    __softboundcets_debug_printf("Loaded key %zx, loaded lock %p\n", *key,
                                 *lock);
#endif

    // Change the state of the proxy object to point to the next entry
    *va_arg_proxy = *va_arg_proxy + __SOFTBOUNDCETS_METADATA_NUM_FIELDS;
}

shadow_stack_ptr_type *
__softboundcets_copy_va_arg_proxy(shadow_stack_ptr_type *to_copy) {
    shadow_stack_ptr_type *proxy_copy = malloc(sizeof(to_copy));

    // Store the value of the other pointer in the proxy object
    *proxy_copy = *to_copy;

    __softboundcets_debug_printf("Allocated proxy copy %p, points to %p "
                                 "(current shadow stack height: %p)\n",
                                 proxy_copy, *to_copy,
                                 __softboundcets_shadow_stack_ptr);

    return proxy_copy;
}

void __softboundcets_free_va_arg_proxy(shadow_stack_ptr_type *va_arg_proxy) {
    __softboundcets_debug_printf("Free proxy %p\n", va_arg_proxy);
    free(va_arg_proxy);
}

__WEAK_INLINE shadow_stack_ptr_type *
__softboundcets_load_proxy_shadow_stack(int arg_no) {

    assert(arg_no >= 0);
    size_t count = 2 + arg_no * __SOFTBOUNDCETS_METADATA_NUM_FIELDS;
    shadow_stack_ptr_type proxy_ptr = __softboundcets_shadow_stack_ptr + count;
    shadow_stack_ptr_type *proxy = *((shadow_stack_ptr_type **)proxy_ptr);
    __softboundcets_debug_printf(
        "Proxy loaded from shadow stack (location %i): %p (pointing to %p)\n",
        arg_no, proxy, proxy ? *proxy : NULL);
    return proxy;
}

__WEAK_INLINE void
__softboundcets_store_proxy_shadow_stack(shadow_stack_ptr_type *proxy,
                                         int arg_no) {

    assert(arg_no >= 0);
    __softboundcets_debug_printf(
        "Proxy to store to shadow stack (location %i): %p (pointing to %p)\n",
        arg_no, proxy, proxy ? *proxy : NULL);
    size_t count = 2 + arg_no * __SOFTBOUNDCETS_METADATA_NUM_FIELDS;
    shadow_stack_ptr_type **store_loc =
        (shadow_stack_ptr_type **)(__softboundcets_shadow_stack_ptr + count);

#if __SOFTBOUNDCETS_DEBUG
    // Override the other stack entries to indicate that they are invalid to use
#if __SOFTBOUNDCETS_SPATIAL || __SOFTBOUNDCETS_SPATIAL_TEMPORAL
    *((void **)(store_loc + __BOUND_INDEX)) = (void *)INVALID_STACK_VALUE;
#endif

#if __SOFTBOUNDCETS_TEMPORAL || __SOFTBOUNDCETS_SPATIAL_TEMPORAL
    *((lock_type *)(store_loc + __LOCK_INDEX)) = (lock_type)INVALID_STACK_VALUE;
#endif

#if __SOFTBOUNDCETS_SPATIAL_TEMPORAL
    *((key_type *)(store_loc + __KEY_INDEX)) = (key_type)INVALID_STACK_VALUE;
#endif

#endif

    *(store_loc) = proxy;
}

void __softboundcets_proxy_metadata_store(const void *addr_of_ptr,
                                          shadow_stack_ptr_type *proxy) {

// This lazily uses the other functions to store the proxy object.
// Technically, the stores of invalid values to bound/(key)/lock are not
// necessary (though a good idea for debugging/finding issues in this run-time).
// However, checking if the arguments are not invalid within the metadata store
// function would affect instrumented programs that do not even use varargs,
// which is not a good solution. The alternative is to copy paste all the code
// from the metadata store here (similar to the load) and adapt it.
#if __SOFTBOUNDCETS_SPATIAL
    __softboundcets_metadata_store(addr_of_ptr, (void *)proxy,
                                   (void *)INVALID_MEM_VALUE);
#elif __SOFTBOUNDCETS_TEMPORAL
    __softboundcets_metadata_store(addr_of_ptr, (key_type)proxy,
                                   (lock_type)INVALID_MEM_VALUE);
#elif __SOFTBOUNDCETS_SPATIAL_TEMPORAL
    __softboundcets_metadata_store(
        addr_of_ptr, (void *)proxy, (void *)INVALID_MEM_VALUE,
        (key_type)INVALID_MEM_VALUE, (lock_type)INVALID_MEM_VALUE);
#endif
}

void __softboundcets_proxy_metadata_load(const void *addr_of_ptr,
                                         shadow_stack_ptr_type **proxy) {
    size_t ptr = (size_t)addr_of_ptr;
    __softboundcets_trie_entry_t *trie_secondary_table;

    size_t primary_index = (ptr >> 25);
    trie_secondary_table = __softboundcets_trie_primary_table[primary_index];

#if !__SOFTBOUNDCETS_PREALLOCATE_TRIE
    if (trie_secondary_table == NULL) {
        *proxy = 0;
        return;
    }
#endif /* PREALLOCATE_ENDS */

    /* MAIN SOFTBOUNDCETS LOAD WHICH RUNS ON THE NORMAL MACHINE */
    size_t secondary_index = ((ptr >> 3) & 0x3fffff);
    __softboundcets_trie_entry_t *entry_ptr =
        &trie_secondary_table[secondary_index];

#if __SOFTBOUNDCETS_SPATIAL
    *proxy = (void *)entry_ptr->base;
#if __SOFTBOUNDCETS_DEBUG
    assert(entry_ptr->bound == (void *)INVALID_MEM_VALUE);
#endif

    __softboundcets_debug_printf(
        "\t[metadata_proxy_load] addr_of_ptr=%p (points to %p), proxy=%p "
        "primary_index=%zx, secondary_index=%zx, trie_entry_addr=%p\n",
        addr_of_ptr, addr_of_ptr ? *(void **)addr_of_ptr : NULL, *proxy,
        primary_index, secondary_index, entry_ptr);

#elif __SOFTBOUNDCETS_TEMPORAL
    *proxy = (shadow_stack_ptr_type *)entry_ptr->key;
#if __SOFTBOUNDCETS_DEBUG
    assert(entry_ptr->lock == (lock_type)INVALID_MEM_VALUE);
#endif

#elif __SOFTBOUNDCETS_SPATIAL_TEMPORAL
    *proxy = (shadow_stack_ptr_type *)entry_ptr->base;
#if __SOFTBOUNDCETS_DEBUG
    assert(entry_ptr->bound == (void *)INVALID_MEM_VALUE);
    assert(entry_ptr->key == (key_type)INVALID_MEM_VALUE);
    assert(entry_ptr->lock == (lock_type)INVALID_MEM_VALUE);
#endif

#endif
}
