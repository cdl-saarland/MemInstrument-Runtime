#ifndef __SOFTBOUNDCETS_TWO_H__
#define __SOFTBOUNDCETS_TWO_H__

#include "softboundcets-defines.h"
#include "softboundcets-spatial.h"
#include "softboundcets-temporal.h"

#include <assert.h>
#include <limits.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <sys/mman.h>

static __attribute__((__constructor__)) void __softboundcets_global_init();

extern __NO_INLINE void __softboundcets_stub(void);

extern void __softboundcets_init(void);

void __softboundcets_global_init() {
    __softboundcets_init();
    __softboundcets_stub();
}

/* Layout of the shadow stack

  1) size of the previous stack frame
  2) size of the current stack frame
  3) base/bound/key/lock of each argument

  Allocation: read the current stack frames size, increment the
  shadow_stack_ptr by current_size + 2, store the previous size into
  the new prev value, calcuate the allocation size and store in the
  new current stack size field; Deallocation: read the previous size,
  and decrement the shadow_stack_ptr */

__WEAK_INLINE void
__softboundcets_allocate_shadow_stack_space(int num_pointer_args) {

    size_t *prev_stack_size_ptr = __softboundcets_shadow_stack_ptr + 1;
    size_t prev_stack_size = *((size_t *)prev_stack_size_ptr);

    __softboundcets_shadow_stack_ptr =
        __softboundcets_shadow_stack_ptr + prev_stack_size + 2;

    *((size_t *)__softboundcets_shadow_stack_ptr) = prev_stack_size;
    size_t *current_stack_size_ptr = __softboundcets_shadow_stack_ptr + 1;

    ssize_t size = num_pointer_args * __SOFTBOUNDCETS_METADATA_NUM_FIELDS;
    *((size_t *)current_stack_size_ptr) = size;
}

__WEAK_INLINE void __softboundcets_deallocate_shadow_stack_space() {

    size_t *reserved_space_ptr = __softboundcets_shadow_stack_ptr;

    size_t read_value = *((size_t *)reserved_space_ptr);

    assert((read_value >= 0 &&
            read_value <= __SOFTBOUNDCETS_SHADOW_STACK_ENTRIES));

    __softboundcets_shadow_stack_ptr =
        __softboundcets_shadow_stack_ptr - read_value - 2;
}

__WEAK_INLINE __softboundcets_trie_entry_t *__softboundcets_trie_allocate() {

    __softboundcets_trie_entry_t *secondary_entry;
    size_t length = (__SOFTBOUNDCETS_TRIE_SECONDARY_TABLE_ENTRIES) *
                    sizeof(__softboundcets_trie_entry_t);
    secondary_entry = __softboundcets_safe_mmap(
        0, length, PROT_READ | PROT_WRITE, SOFTBOUNDCETS_MMAP_FLAGS, -1, 0);
    // assert(secondary_entry != (void*)-1);
    // printf("snd trie table %p %lx\n", secondary_entry, length);
    return secondary_entry;
}

#if 0

//These are primary used to test and introspect  metadata during testing

__WEAK_INLINE void __softboundcets_print_metadata(void* base, void* bound, void* ptr, size_t key, size_t* lock){

  printf("[print metadata] ptr = %p, base=%p, bound=%p, key = %zd, lock = %p, *lock = %zd\n", ptr, base, bound, key, lock, *lock);

}

__WEAK_INLINE void __softboundcets_intermediate(char cmp1, char cmp2, char cmp3, size_t loaded_lock){

  printf("cmp = %d, cmp2 =%d cmp=%d, loaded_lock=%zd\n", cmp1, cmp2, cmp3, loaded_lock);

}

#endif

__WEAK_INLINE void __softboundcets_introspect_metadata(void *ptr, void *base,
                                                       void *bound,
                                                       int arg_no) {

    printf("[introspect_metadata]ptr=%p, base=%p, bound=%p, arg_no=%d\n", ptr,
           base, bound, arg_no);
}

__METADATA_INLINE
void __softboundcets_copy_metadata(void *dest, void *from, size_t size) {

    //  printf("dest=%p, from=%p, size=%zx\n", dest, from, size);

    size_t dest_ptr = (size_t)dest;
    size_t dest_ptr_end = dest_ptr + size;

    size_t from_ptr = (size_t)from;
    size_t from_ptr_end = from_ptr + size;

    if (from_ptr % 8 != 0) {
        // printf("dest=%p, from=%p, size=%zx\n", dest, from, size);
        return;
        //    from_ptr = from_ptr %8;
        //    dest_ptr = dest_ptr %8;
    }

    //  printf("dest=%p, from=%p, size=%zx\n", dest, from, size);
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

#ifdef __SOFTBOUNDCETS_SPATIAL
            memcpy(dest_entry_ptr, from_entry_ptr, 16);
#elif __SOFTBOUNDCETS_TEMPORAL
            memcpy(dest_entry_ptr, from_entry_ptr, 16);
#elif __SOFTBOUNDCETS_SPATIAL_TEMPORAL
            memcpy(dest_entry_ptr, from_entry_ptr, 32);
#else
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

#ifdef __SOFTBOUNDCETS_SPATIAL

    memcpy(dest_entry_ptr, from_entry_ptr, 16 * (size >> 3));
#elif __SOFTBOUNDCETS_TEMPORAL

    memcpy(dest_entry_ptr, from_entry_ptr, 16 * (size >> 3));
#elif __SOFTBOUNDCETS_SPATIAL_TEMPORAL

    memcpy(dest_entry_ptr, from_entry_ptr, 32 * (size >> 3));
#else

    memcpy(dest_entry_ptr, from_entry_ptr, 32 * (size >> 3));
#endif
    return;
}

/* Memcopy check, different variants based on spatial, temporal and
   spatial+temporal modes
*/

#ifdef __SOFTBOUNDCETS_SPATIAL
__WEAK_INLINE void __softboundcets_memcopy_check(void *dest, void *src,
                                                 size_t size, void *dest_base,
                                                 void *dest_bound,
                                                 void *src_base,
                                                 void *src_bound) {
#if ENABLE_RT_STATS
    __rt_stat_inc_sb_mem_check();
#endif

    if (size >= LONG_MAX)
        __softboundcets_abort();

    if (dest < dest_base || (char *)dest > ((char *)dest_bound - size) ||
        (size > (size_t)dest_bound)) {
#if NOERRORMISSINGBOUNDS
        if (dest_bound == NULL) {
            return;
        }
#endif
        __softboundcets_printf(
            "In MemCopy Dest Check, base=%p, bound=%p, size=%zx\n", dest_base,
            dest_bound, size);
        __softboundcets_abort();
    }

    if (src < src_base || (char *)src > ((char *)src_bound - size) ||
        (size > (size_t)src_bound)) {
#if NOERRORMISSINGBOUNDS
        if (src_bound == NULL) {
            return;
        }
#endif
        __softboundcets_printf(
            "In MemCopy Src Check, base=%p, bound=%p, size=%zx\n", src_base,
            src_bound, size);
        __softboundcets_abort();
    }
}
#elif __SOFTBOUNDCETS_TEMPORAL

__WEAK_INLINE void __softboundcets_memcopy_check(void *dest, void *src,
                                                 size_t size, size_t dest_key,
                                                 void *dest_lock,
                                                 size_t src_key,
                                                 void *src_lock) {
#if ENABLE_RT_STATS
    __rt_stat_inc_sb_mem_check();
#endif

    if (size >= LONG_MAX)
        __softboundcets_abort();

    if (dest_key != *((size_t *)(dest_lock))) {
        __softboundcets_abort();
    }

    if (src_key != *((size_t *)(src_lock))) {
        __softboundcets_abort();
    }
}

#elif __SOFTBOUNDCETS_SPATIAL_TEMPORAL

__WEAK_INLINE void
__softboundcets_memcopy_check(void *dest, void *src, size_t size,
                              void *dest_base, void *dest_bound, void *src_base,
                              void *src_bound, size_t dest_key, void *dest_lock,
                              size_t src_key, void *src_lock) {
#if ENABLE_RT_STATS
    __rt_stat_inc_sb_mem_check();
#endif

#ifndef __NOSIM_CHECKS

    /* printf("dest=%zx, src=%zx, size=%zx, ulong_max=%zx\n",  */
    /*        dest, src, size, ULONG_MAX); */
    if (size >= LONG_MAX)
        __softboundcets_abort();

    if (dest < dest_base || (char *)dest > ((char *)dest_bound - size) ||
        (size > (size_t)dest_bound))
        __softboundcets_abort();

    if (src < src_base || (char *)src > ((char *)src_bound - size) ||
        (size > (size_t)dest_bound))
        __softboundcets_abort();

    if (dest_key != *((size_t *)(dest_lock))) {
        __softboundcets_abort();
    }

    if (src_key != *((size_t *)(src_lock))) {
        __softboundcets_abort();
    }

#endif
}
#else

__WEAK_INLINE void
__softboundcets_memcopy_check(void *dest, void *src, size_t size,
                              void *dest_base, void *dest_bound, void *src_base,
                              void *src_bound, size_t dest_key, void *dest_lock,
                              size_t src_key, void *src_lock) {
#if ENABLE_RT_STATS
    __rt_stat_inc_sb_mem_check();
#endif

    printf("not handled\n");
    __softboundcets_abort();
}
#endif

/* Memset check, different variants based on spatial, temporal and
   spatial+temporal modes */

#ifdef __SOFTBOUNDCETS_SPATIAL
__WEAK_INLINE void __softboundcets_memset_check(void *dest, size_t size,
                                                void *dest_base,
                                                void *dest_bound) {
#if ENABLE_RT_STATS
    __rt_stat_inc_sb_mem_check();
#endif

    if (size >= LONG_MAX)
        __softboundcets_abort();

    if (dest < dest_base || (char *)dest > ((char *)dest_bound - size) ||
        (size > (size_t)dest_bound))
        __softboundcets_abort();
}
#elif __SOFTBOUNDCETS_TEMPORAL

__WEAK_INLINE void __softboundcets_memset_check(void *dest, size_t size,
                                                size_t dest_key,
                                                void *dest_lock) {
#if ENABLE_RT_STATS
    __rt_stat_inc_sb_mem_check();
#endif

    if (size >= LONG_MAX)
        __softboundcets_abort();

    if (size >= LONG_MAX)
        __softboundcets_abort();

    if (dest_key != *((size_t *)(dest_lock))) {
        __softboundcets_abort();
    }
}

#elif __SOFTBOUNDCETS_SPATIAL_TEMPORAL

__WEAK_INLINE void __softboundcets_memset_check(void *dest, size_t size,
                                                void *dest_base,
                                                void *dest_bound,
                                                size_t dest_key,
                                                void *dest_lock) {
#if ENABLE_RT_STATS
    __rt_stat_inc_sb_mem_check();
#endif

    if (size >= LONG_MAX)
        __softboundcets_abort();

    if (dest < dest_base || (char *)dest > ((char *)dest_bound - size) ||
        (size > (size_t)dest_bound))
        __softboundcets_abort();

    if (dest_key != *((size_t *)(dest_lock))) {
        __softboundcets_abort();
    }
}
#else

__WEAK_INLINE void __softboundcets_memset_check(void *dest, size_t size,
                                                void *dest_base,
                                                void *dest_bound,
                                                size_t dest_key,
                                                void *dest_lock) {
#if ENABLE_RT_STATS
    __rt_stat_inc_sb_mem_check();
#endif

    printf("not handled\n");
    __softboundcets_abort();
}
#endif

/* Metadata store parameterized by the mode of checking */

#ifdef __SOFTBOUNDCETS_SPATIAL

__METADATA_INLINE void __softboundcets_metadata_store(void *addr_of_ptr,
                                                      void *base, void *bound) {

#elif __SOFTBOUNDCETS_TEMPORAL

__METADATA_INLINE void __softboundcets_metadata_store(void *addr_of_ptr,
                                                      size_t key, void *lock) {

#elif __SOFTBOUNDCETS_SPATIAL_TEMPORAL

__METADATA_INLINE void __softboundcets_metadata_store(void *addr_of_ptr,
                                                      void *base, void *bound,
                                                      size_t key, void *lock) {

#else

__METADATA_INLINE void __softboundcets_metadata_store(void *addr_of_ptr,
                                                      void *base, void *bound,
                                                      size_t key, void *lock) {

#endif

    size_t ptr = (size_t)addr_of_ptr;
    size_t primary_index;
    __softboundcets_trie_entry_t *trie_secondary_table;
    //  __softboundcets_trie_entry_t** trie_primary_table =
    //  __softboundcets_trie_primary_table;

    primary_index = (ptr >> 25);
    trie_secondary_table = __softboundcets_trie_primary_table[primary_index];

    if (!__SOFTBOUNDCETS_PREALLOCATE_TRIE) {
        if (trie_secondary_table == NULL) {
            trie_secondary_table = __softboundcets_trie_allocate();
            __softboundcets_trie_primary_table[primary_index] =
                trie_secondary_table;
        }
        //    __softboundcetswithss_printf("addr_of_ptr=%zx, primary_index =%zx,
        //    trie_secondary_table=%p\n", addr_of_ptr, primary_index,
        //    trie_secondary_table);
        assert(trie_secondary_table != NULL);
    }

    size_t secondary_index = ((ptr >> 3) & 0x3fffff);
    __softboundcets_trie_entry_t *entry_ptr =
        &trie_secondary_table[secondary_index];

#ifdef __SOFTBOUNDCETS_SPATIAL

    entry_ptr->base = base;
    entry_ptr->bound = bound;
    if (__SOFTBOUNDCETS_DEBUG) {
        __softboundcets_printf(
            "\t[metadata_store] addr_of_ptr=%p, base=%p, bound=%p, "
            "primary_index=%zx, secondary_index=%zx, trie_entry_addr=%p\n",
            addr_of_ptr, base, bound, primary_index, secondary_index,
            entry_ptr);
    }
#elif __SOFTBOUNDCETS_TEMPORAL

    entry_ptr->key = key;
    entry_ptr->lock = lock;

#elif __SOFTBOUNDCETS_SPATIAL_TEMPORAL

    entry_ptr->base = base;
    entry_ptr->bound = bound;
    entry_ptr->key = key;
    entry_ptr->lock = lock;

#else

    entry_ptr->base = base;
    entry_ptr->bound = bound;
    entry_ptr->key = key;
    entry_ptr->lock = lock;

#endif

    return;
}

#ifdef __SOFTBOUNDCETS_SPATIAL_TEMPORAL

__WEAK_INLINE void *__softboundcets_metadata_map(void *addr_of_ptr) {

    size_t ptr = (size_t)addr_of_ptr;
    __softboundcets_trie_entry_t *trie_secondary_table;
    size_t primary_index = (ptr >> 25);
    trie_secondary_table = __softboundcets_trie_primary_table[primary_index];

#if 0
    /* unnecessary control flow causes performance overhead */
    /* this can cause segfaults with uninitialized pointer reads from memory */
    if(trie_secondary_table == NULL){
      trie_secondary_table = __softboundcets_trie_allocate();
      __softboundcets_trie_primary_table[primary_index] = trie_secondary_table;
    }

#endif

    size_t secondary_index = ((ptr >> 3) & 0x3fffff);
    __softboundcets_trie_entry_t *entry_ptr =
        &trie_secondary_table[secondary_index];

    return (void *)entry_ptr;
}

__WEAK_INLINE void *__softboundcets_metadata_load_base(void *address) {

    __softboundcets_trie_entry_t *entry_ptr =
        (__softboundcets_trie_entry_t *)address;
    return entry_ptr->base;
}

__WEAK_INLINE void *__softboundcets_metadata_load_bound(void *address) {

    __softboundcets_trie_entry_t *entry_ptr =
        (__softboundcets_trie_entry_t *)address;
    return entry_ptr->bound;
}

__WEAK_INLINE size_t __softboundcets_metadata_load_key(void *address) {

    __softboundcets_trie_entry_t *entry_ptr =
        (__softboundcets_trie_entry_t *)address;
    return entry_ptr->key;
}

__WEAK_INLINE void *__softboundcets_metadata_load_lock(void *address) {

    __softboundcets_trie_entry_t *entry_ptr =
        (__softboundcets_trie_entry_t *)address;
    return entry_ptr->lock;
}

#endif

#ifdef __SOFTBOUNDCETS_SPATIAL

__METADATA_INLINE void
__softboundcets_metadata_load(void *addr_of_ptr, void **base, void **bound) {

#elif __SOFTBOUNDCETS_TEMPORAL

__METADATA_INLINE void __softboundcets_metadata_load(void *addr_of_ptr,
                                                     size_t *key, void **lock) {

#elif __SOFTBOUNDCETS_SPATIAL_TEMPORAL

__METADATA_INLINE void __softboundcets_metadata_load(void *addr_of_ptr,
                                                     void **base, void **bound,
                                                     size_t *key, void **lock) {

#else

__METADATA_INLINE void __softboundcets_metadata_load(void *addr_of_ptr,
                                                     void **base, void **bound,
                                                     size_t *key, void **lock) {

#endif

    size_t ptr = (size_t)addr_of_ptr;
    __softboundcets_trie_entry_t *trie_secondary_table;
    //    __softboundcets_trie_entry_t** trie_primary_table =
    //    __softboundcets_trie_primary_table;

    // assert(__softboundcetswithss_trie_primary_table[primary_index] ==
    // trie_secondary_table);

    size_t primary_index = (ptr >> 25);
    trie_secondary_table = __softboundcets_trie_primary_table[primary_index];

    if (!__SOFTBOUNDCETS_PREALLOCATE_TRIE) {
        if (trie_secondary_table == NULL) {

#ifdef __SOFTBOUNDCETS_SPATIAL
            *((void **)base) = 0;
            *((void **)bound) = 0;
#elif __SOFTBOUNDCETS_TEMPORAL
            *((size_t *)key) = 0;
            *((size_t *)lock) = 0;

#elif __SOFTBOUNDCETS_SPATIAL_TEMPORAL

            *((void **)base) = 0;
            *((void **)bound) = 0;
            *((size_t *)key) = 0;
            *((size_t *)lock) = 0;

#else
            *((void **)base) = 0;
            *((void **)bound) = 0;
            *((size_t *)key) = 0;
            *((size_t *)lock) = 0;
#endif
            return;
        }
    } /* PREALLOCATE_ENDS */

    /* MAIN SOFTBOUNDCETS LOAD WHICH RUNS ON THE NORMAL MACHINE */
    size_t secondary_index = ((ptr >> 3) & 0x3fffff);
    __softboundcets_trie_entry_t *entry_ptr =
        &trie_secondary_table[secondary_index];

#ifdef __SOFTBOUNDCETS_SPATIAL
    *((void **)base) = entry_ptr->base;
    *((void **)bound) = entry_ptr->bound;

#elif __SOFTBOUNDCETS_TEMPORAL
    *((size_t *)key) = entry_ptr->key;
    *((void **)lock) = (void *)entry_ptr->lock;

#elif __SOFTBOUNDCETS_SPATIAL_TEMPORAL

    *((void **)base) = entry_ptr->base;
    *((void **)bound) = entry_ptr->bound;
    *((size_t *)key) = entry_ptr->key;
    *((void **)lock) = (void *)entry_ptr->lock;

#else

    *((void **)base) = entry_ptr->base;
    *((void **)bound) = entry_ptr->bound;
    *((size_t *)key) = entry_ptr->key;
    *((void **)lock) = (void *)entry_ptr->lock;

#endif
    return;
}
/******************************************************************************/

__WEAK_INLINE void
__softboundcets_allocation_secondary_trie_allocate_range(void *initial_ptr,
                                                         size_t size) {

    if (!__SOFTBOUNDCETS_PREALLOCATE_TRIE)
        return;

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

    if (!__SOFTBOUNDCETS_PREALLOCATE_TRIE)
        return;

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
                                     void **bound, size_t *key, void **lock,
                                     int index) {

    size_t val = index * 8;
    size_t addr = (size_t)addr_of_ptr;
    addr = addr + val;

    __softboundcets_metadata_load((void *)addr, base, bound, key, lock);
}

__METADATA_INLINE void
__softboundcets_metadata_store_vector(void *addr_of_ptr, void *base,
                                      void *bound, size_t key, void *lock,
                                      int index) {
    size_t val = index * 8;
    size_t addr = (size_t)addr_of_ptr;
    addr = addr + val;

    __softboundcets_metadata_store((void *)addr, base, bound, key, lock);
}
#endif

#endif