#ifndef TEMPORAL_H
#define TEMPORAL_H

#include "softboundcets-defines.h"

// All functions in this header are there to support temporal memory safety.
// If the runtime is not configured to ensure temporal safety, these functions
// are not available.

// Key/Lock are (currently) frequently used as arguments to metadata propagation
// functions. These always take four arguments independent of the configuration,
// so we need to make this function available even when temporal safety is not
// requested.
__WEAK_INLINE lock_type __softboundcets_get_global_lock();

#if __SOFTBOUNDCETS_SPATIAL_TEMPORAL || __SOFTBOUNDCETS_TEMPORAL

//===------------------- Data structure initialization --------------------===//

__WEAK_INLINE void __softboundcets_temporal_initialize_datastructures(void);

//===----------------------------- Checks ---------------------------------===//

#if __SOFTBOUNDCETS_SPATIAL_TEMPORAL
__WEAK_INLINE void
__softboundcets_temporal_load_dereference_check(void *pointer_lock, size_t key,
                                                void *base, void *bound);
#else
__WEAK_INLINE void
__softboundcets_temporal_load_dereference_check(void *pointer_lock, size_t key);
#endif

#if __SOFTBOUNDCETS_SPATIAL_TEMPORAL
__WEAK_INLINE void
__softboundcets_temporal_store_dereference_check(void *pointer_lock, size_t key,
                                                 void *base, void *bound);
#else
__WEAK_INLINE void
__softboundcets_temporal_store_dereference_check(void *pointer_lock,
                                                 size_t key);
#endif

//===-------------------- Shadow Stack Manipulation -----------------------===//

__WEAK_INLINE size_t __softboundcets_load_key_shadow_stack(int arg_no);

__WEAK_INLINE void *__softboundcets_load_lock_shadow_stack(int arg_no);

__WEAK_INLINE void __softboundcets_store_key_shadow_stack(size_t key,
                                                          int arg_no);

__WEAK_INLINE void __softboundcets_store_lock_shadow_stack(void *lock,
                                                           int arg_no);

//===----------------------- Allocation related  --------------------------===//

__WEAK_INLINE void __softboundcets_stack_memory_deallocation(size_t ptr_key);

__WEAK_INLINE void __softboundcets_memory_deallocation(void *ptr_lock,
                                                       size_t ptr_key);

__WEAK_INLINE void *__softboundcets_allocate_lock_location();

__WEAK_INLINE void __softboundcets_stack_memory_allocation(void **ptr_lock,
                                                           size_t *ptr_key);

__WEAK_INLINE void __softboundcets_memory_allocation(void *ptr, void **ptr_lock,
                                                     size_t *ptr_key);

//===---------------------- Free map operations  --------------------------===//

__WEAK_INLINE void __softboundcets_add_to_free_map(size_t ptr_key, void *ptr);

__WEAK_INLINE void __softboundcets_check_remove_from_free_map(size_t ptr_key,
                                                              void *ptr);

#endif // Guard for temporal functions

#endif // TEMPORAL_H
