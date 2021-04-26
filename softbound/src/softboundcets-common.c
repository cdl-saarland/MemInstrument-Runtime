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

    size_t *prev_stack_size_ptr = __softboundcets_shadow_stack_ptr + 1;
    size_t prev_stack_size = *((size_t *)prev_stack_size_ptr);

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

    *((size_t *)__softboundcets_shadow_stack_ptr) = prev_stack_size;
    size_t *current_stack_size_ptr = __softboundcets_shadow_stack_ptr + 1;

    *((size_t *)current_stack_size_ptr) = size;
}

__WEAK_INLINE void __softboundcets_deallocate_shadow_stack_space() {

    size_t *reserved_space_ptr = __softboundcets_shadow_stack_ptr;

    size_t read_value = *((size_t *)reserved_space_ptr);

    assert(read_value >= 0 &&
           read_value <= __SOFTBOUNDCETS_SHADOW_STACK_ENTRIES);

    __softboundcets_shadow_stack_ptr =
        __softboundcets_shadow_stack_ptr - read_value - 2;
}

__WEAK_INLINE __softboundcets_trie_entry_t *__softboundcets_trie_allocate() {

    __softboundcets_trie_entry_t *secondary_entry;
    size_t length = (__SOFTBOUNDCETS_TRIE_SECONDARY_TABLE_ENTRIES) *
                    sizeof(__softboundcets_trie_entry_t);
    secondary_entry = mmap(0, length, PROT_READ | PROT_WRITE,
                           SOFTBOUNDCETS_MMAP_FLAGS, -1, 0);
    // assert(secondary_entry != (void*)-1);
    __softboundcets_debug_printf("snd trie table %p %lx\n", secondary_entry,
                                 length);
    return secondary_entry;
}

__WEAK_INLINE void __softboundcets_introspect_metadata(void *ptr, void *base,
                                                       void *bound,
                                                       int arg_no) {

    __softboundcets_printf(
        "[introspect_metadata]ptr=%p, base=%p, bound=%p, arg_no=%d\n", ptr,
        base, bound, arg_no);
}

__METADATA_INLINE
void __softboundcets_copy_metadata(void *dest, const void *from, size_t size) {

    __softboundcets_debug_printf("[Copy metadata] dest=%p, from=%p, size=%zx\n",
                                 dest, from, size);

    size_t dest_ptr = (size_t)dest;
    size_t dest_ptr_end = dest_ptr + size;

    size_t from_ptr = (size_t)from;
    size_t from_ptr_end = from_ptr + size;

    if (from_ptr % 8 != 0) {
        return;
        //    from_ptr = from_ptr %8;
        //    dest_ptr = dest_ptr %8;
    }

    __softboundcets_trie_entry_t *trie_secondary_table_dest_begin;
    __softboundcets_trie_entry_t *trie_secondary_table_from_begin;

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

            if (temp_from_strie == NULL) {
                temp_from_strie = __softboundcets_trie_allocate();
                __softboundcets_trie_primary_table[temp_from_pindex] =
                    temp_from_strie;
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

#if __SOFTBOUNDCETS_SPATIAL
            memcpy(dest_entry_ptr, from_entry_ptr, 16);
#elif __SOFTBOUNDCETS_TEMPORAL
            memcpy(dest_entry_ptr, from_entry_ptr, 16);
#elif __SOFTBOUNDCETS_SPATIAL_TEMPORAL
            memcpy(dest_entry_ptr, from_entry_ptr, 32);
#endif
        }
        return;
    }

    trie_secondary_table_dest_begin =
        __softboundcets_trie_primary_table[dest_primary_index_begin];
    trie_secondary_table_from_begin =
        __softboundcets_trie_primary_table[from_primary_index_begin];

    if (trie_secondary_table_from_begin == NULL)
        return;

    if (trie_secondary_table_dest_begin == NULL) {
        trie_secondary_table_dest_begin = __softboundcets_trie_allocate();
        __softboundcets_trie_primary_table[dest_primary_index_begin] =
            trie_secondary_table_dest_begin;
    }

    size_t dest_secondary_index = ((dest_ptr >> 3) & 0x3fffff);
    size_t from_secondary_index = ((from_ptr >> 3) & 0x3fffff);

    assert(dest_secondary_index < __SOFTBOUNDCETS_TRIE_SECONDARY_TABLE_ENTRIES);
    assert(from_secondary_index < __SOFTBOUNDCETS_TRIE_SECONDARY_TABLE_ENTRIES);

    void *dest_entry_ptr =
        &trie_secondary_table_dest_begin[dest_secondary_index];
    void *from_entry_ptr =
        &trie_secondary_table_from_begin[from_secondary_index];

#if __SOFTBOUNDCETS_SPATIAL
    memcpy(dest_entry_ptr, from_entry_ptr, 16 * (size >> 3));
#elif __SOFTBOUNDCETS_TEMPORAL
    memcpy(dest_entry_ptr, from_entry_ptr, 16 * (size >> 3));
#elif __SOFTBOUNDCETS_SPATIAL_TEMPORAL
    memcpy(dest_entry_ptr, from_entry_ptr, 32 * (size >> 3));
#endif
    return;
}

/* Metadata store parameterized by the mode of checking */

#if __SOFTBOUNDCETS_SPATIAL

__METADATA_INLINE void __softboundcets_metadata_store(const void *addr_of_ptr,
                                                      void *base, void *bound) {

#elif __SOFTBOUNDCETS_TEMPORAL

__METADATA_INLINE void __softboundcets_metadata_store(void *addr_of_ptr,
                                                      key_type key,
                                                      lock_type lock) {

#elif __SOFTBOUNDCETS_SPATIAL_TEMPORAL

__METADATA_INLINE void __softboundcets_metadata_store(void *addr_of_ptr,
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
        "\t[metadata_store not prealloced] addr_of_ptr=%zx, primary_index=%zx, "
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
        "\t[metadata_store] addr_of_ptr=%p, base=%p, bound=%p, "
        "primary_index=%zx, secondary_index=%zx, trie_entry_addr=%p\n",
        addr_of_ptr, base, bound, primary_index, secondary_index, entry_ptr);

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

__METADATA_INLINE void
__softboundcets_metadata_load(void *addr_of_ptr, void **base, void **bound) {

#elif __SOFTBOUNDCETS_TEMPORAL

__METADATA_INLINE void __softboundcets_metadata_load(void *addr_of_ptr,
                                                     key_type *key,
                                                     lock_type *lock) {

#elif __SOFTBOUNDCETS_SPATIAL_TEMPORAL

__METADATA_INLINE void __softboundcets_metadata_load(void *addr_of_ptr,
                                                     void **base, void **bound,
                                                     key_type *key,
                                                     lock_type *lock) {

#endif

    size_t ptr = (size_t)addr_of_ptr;
    __softboundcets_trie_entry_t *trie_secondary_table;

    // assert(__softboundcetswithss_trie_primary_table[primary_index] ==
    // trie_secondary_table);

    size_t primary_index = (ptr >> 25);
    trie_secondary_table = __softboundcets_trie_primary_table[primary_index];

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
        "\t[metadata_load] addr_of_ptr=%p, base=%p, bound=%p, "
        "primary_index=%zx, secondary_index=%zx, trie_entry_addr=%p\n",
        addr_of_ptr, *base, *bound, primary_index, secondary_index, entry_ptr);

#endif
#if __SOFTBOUNDCETS_TEMPORAL || __SOFTBOUNDCETS_SPATIAL_TEMPORAL
    *((key_type *)key) = entry_ptr->key;
    *((lock_type *)lock) = (lock_type *)entry_ptr->lock;
#endif
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

// Vector load/store support
#if __SOFTBOUNDCETS_SPATIAL_TEMPORAL
__METADATA_INLINE void
__softboundcets_metadata_load_vector(void *addr_of_ptr, void **base,
                                     void **bound, key_type *key,
                                     lock_type *lock, int index) {

    size_t val = index * 8;
    size_t addr = (size_t)addr_of_ptr;
    addr = addr + val;

    __softboundcets_metadata_load((void *)addr, base, bound, key, lock);
}

__METADATA_INLINE void
__softboundcets_metadata_store_vector(void *addr_of_ptr, void *base,
                                      void *bound, key_type key, lock_type lock,
                                      int index) {
    size_t val = index * 8;
    size_t addr = (size_t)addr_of_ptr;
    addr = addr + val;

    __softboundcets_metadata_store((void *)addr, base, bound, key, lock);
}

#endif
