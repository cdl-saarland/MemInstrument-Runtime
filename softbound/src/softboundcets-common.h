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
  the new prev value, calcuate the allocation size and store in the
  new current stack size field; Deallocation: read the previous size,
  and decrement the shadow_stack_ptr */

__WEAK_INLINE void
__softboundcets_allocate_shadow_stack_space(int num_pointer_args);

__WEAK_INLINE void __softboundcets_deallocate_shadow_stack_space();

__WEAK_INLINE __softboundcets_trie_entry_t *__softboundcets_trie_allocate();

__WEAK_INLINE void __softboundcets_introspect_metadata(void *ptr, void *base,
                                                       void *bound, int arg_no);

__METADATA_INLINE
void __softboundcets_copy_metadata(void *dest, void *from, size_t size);

/* Memcopy check, different variants based on spatial, temporal and
   spatial+temporal modes
*/

extern __softboundcets_trie_entry_t **__softboundcets_trie_primary_table;
#if __SOFTBOUNDCETS_SPATIAL
__WEAK_INLINE void __softboundcets_memcopy_check(void *dest, void *src,
                                                 size_t size, void *dest_base,
                                                 void *dest_bound,
                                                 void *src_base,
                                                 void *src_bound);
#elif __SOFTBOUNDCETS_TEMPORAL

__WEAK_INLINE void __softboundcets_memcopy_check(void *dest, void *src,
                                                 size_t size, size_t dest_key,
                                                 void *dest_lock,
                                                 size_t src_key,
                                                 void *src_lock);

#elif __SOFTBOUNDCETS_SPATIAL_TEMPORAL

__WEAK_INLINE void
__softboundcets_memcopy_check(void *dest, void *src, size_t size,
                              void *dest_base, void *dest_bound, void *src_base,
                              void *src_bound, size_t dest_key, void *dest_lock,
                              size_t src_key, void *src_lock);

#endif

/* Memset check, different variants based on spatial, temporal and
   spatial+temporal modes */

#if __SOFTBOUNDCETS_SPATIAL
__WEAK_INLINE void __softboundcets_memset_check(void *dest, size_t size,
                                                void *dest_base,
                                                void *dest_bound);
#elif __SOFTBOUNDCETS_TEMPORAL

__WEAK_INLINE void __softboundcets_memset_check(void *dest, size_t size,
                                                size_t dest_key,
                                                void *dest_lock);

#elif __SOFTBOUNDCETS_SPATIAL_TEMPORAL

__WEAK_INLINE void __softboundcets_memset_check(void *dest, size_t size,
                                                void *dest_base,
                                                void *dest_bound,
                                                size_t dest_key,
                                                void *dest_lock);

#endif

/* Metadata store parameterized by the mode of checking */

#if __SOFTBOUNDCETS_SPATIAL

__METADATA_INLINE void __softboundcets_metadata_store(void *addr_of_ptr,
                                                      void *base, void *bound);

#elif __SOFTBOUNDCETS_TEMPORAL

__METADATA_INLINE void __softboundcets_metadata_store(void *addr_of_ptr,
                                                      size_t key, void *lock);

#elif __SOFTBOUNDCETS_SPATIAL_TEMPORAL

__METADATA_INLINE void __softboundcets_metadata_store(void *addr_of_ptr,
                                                      void *base, void *bound,
                                                      size_t key, void *lock);

#endif

#if __SOFTBOUNDCETS_SPATIAL

__METADATA_INLINE void __softboundcets_metadata_load(void *addr_of_ptr,
                                                     void **base, void **bound);

#elif __SOFTBOUNDCETS_TEMPORAL

__METADATA_INLINE void __softboundcets_metadata_load(void *addr_of_ptr,
                                                     size_t *key, void **lock);

#elif __SOFTBOUNDCETS_SPATIAL_TEMPORAL

__METADATA_INLINE void __softboundcets_metadata_load(void *addr_of_ptr,
                                                     void **base, void **bound,
                                                     size_t *key, void **lock);

#endif

/******************************************************************************/

__WEAK_INLINE void
__softboundcets_allocation_secondary_trie_allocate_range(void *initial_ptr,
                                                         size_t size);
__WEAK_INLINE void
__softboundcets_allocation_secondary_trie_allocate(void *addr_of_ptr);

// Vector load/store support
#if __SOFTBOUNDCETS_SPATIAL_TEMPORAL
__METADATA_INLINE void
__softboundcets_metadata_load_vector(void *addr_of_ptr, void **base,
                                     void **bound, size_t *key, void **lock,
                                     int index);

__METADATA_INLINE void
__softboundcets_metadata_store_vector(void *addr_of_ptr, void *base,
                                      void *bound, size_t key, void *lock,
                                      int index);
#endif

#endif // SOFTBOUNDCETS_COMMON_H
