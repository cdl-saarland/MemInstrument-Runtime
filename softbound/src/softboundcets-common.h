#ifndef SOFTBOUNDCETS_COMMON_H
#define SOFTBOUNDCETS_COMMON_H

#include "softboundcets-defines.h"

// 2^23 entries each will be 8 bytes each
static const size_t __SOFTBOUNDCETS_TRIE_PRIMARY_TABLE_ENTRIES =
    ((size_t)8 * (size_t)1024 * (size_t)1024);

static const size_t __SOFTBOUNDCETS_SHADOW_STACK_ENTRIES =
    ((size_t)128 * (size_t)32);

// each secondary entry has 2^ 22 entries
static const size_t __SOFTBOUNDCETS_TRIE_SECONDARY_TABLE_ENTRIES =
    ((size_t)4 * (size_t)1024 * (size_t)1024);

/* Layout of the shadow stack

  1) size of the previous stack frame
  2) size of the current stack frame
  3) base/bound/key/lock of each argument

  Allocation: read the current stack frames size, increment the
  shadow_stack_ptr by current_size + 2, store the previous size into
  the new prev value, calculate the allocation size and store in the
  new current stack size field; Deallocation: read the previous size,
  and decrement the shadow_stack_ptr */

void __softboundcets_allocate_shadow_stack_space(int num_pointer_args);

void __softboundcets_deallocate_shadow_stack_space();

__softboundcets_trie_entry_t *__softboundcets_trie_allocate();

// Store metadata about the `environ` variable
void __softboundcets_update_environment_metadata();

void __softboundcets_introspect_metadata(void *ptr, void *base, void *bound,
                                         int arg_no);

void __softboundcets_copy_metadata(void *dest, const void *from, size_t size);

/* Metadata store parameterized by the mode of checking */

#if __SOFTBOUNDCETS_SPATIAL

void __softboundcets_metadata_store(const void *addr_of_ptr, void *base,
                                    void *bound);

#elif __SOFTBOUNDCETS_TEMPORAL

void __softboundcets_metadata_store(void *addr_of_ptr, key_type key,
                                    lock_type lock);

#elif __SOFTBOUNDCETS_SPATIAL_TEMPORAL

void __softboundcets_metadata_store(void *addr_of_ptr, void *base, void *bound,
                                    key_type key, lock_type lock);

#endif

#if __SOFTBOUNDCETS_SPATIAL

void __softboundcets_metadata_load(void *addr_of_ptr, void **base,
                                   void **bound);

#elif __SOFTBOUNDCETS_TEMPORAL

void __softboundcets_metadata_load(void *addr_of_ptr, key_type *key,
                                   lock_type *lock);

#elif __SOFTBOUNDCETS_SPATIAL_TEMPORAL

void __softboundcets_metadata_load(void *addr_of_ptr, void **base, void **bound,
                                   key_type *key, lock_type *lock);

#endif

/******************************************************************************/

void __softboundcets_allocation_secondary_trie_allocate_range(void *initial_ptr,
                                                              size_t size);
void __softboundcets_allocation_secondary_trie_allocate(void *addr_of_ptr);

// Vector load/store support
#if __SOFTBOUNDCETS_SPATIAL_TEMPORAL
void __softboundcets_metadata_load_vector(void *addr_of_ptr, void **base,
                                          void **bound, key_type *key,
                                          lock_type *lock, int index);

void __softboundcets_metadata_store_vector(void *addr_of_ptr, void *base,
                                           void *bound, key_type key,
                                           lock_type lock, int index);
#endif

#endif // SOFTBOUNDCETS_COMMON_H
