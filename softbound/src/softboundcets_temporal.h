#ifndef SOFTBOUNDCETS_TEMPORAL_H
#define SOFTBOUNDCETS_TEMPORAL_H

#include "softboundcets_defines.h"

// All functions in this header are there to support temporal memory safety.
// If the runtime is not configured to ensure temporal safety, these functions
// are not available.

// Key/Lock are (currently) frequently used as arguments to metadata propagation
// functions. These always take four arguments independent of the configuration,
// so we need to make this function available even when temporal safety is not
// requested.
lock_type __softboundcets_get_global_lock();

#if __SOFTBOUNDCETS_SPATIAL_TEMPORAL || __SOFTBOUNDCETS_TEMPORAL

//===------------------- Data structure initialization --------------------===//

void __softboundcets_temporal_initialize_datastructures(void);

//===----------------------------- Check ----------------------------------===//

void __softboundcets_temporal_dereference_check(lock_type pointer_lock,
                                                key_type key);

//===-------------------- Shadow Stack Manipulation -----------------------===//

key_type __softboundcets_load_key_shadow_stack(int arg_no);

lock_type __softboundcets_load_lock_shadow_stack(int arg_no);

void __softboundcets_store_key_shadow_stack(key_type key, int arg_no);

void __softboundcets_store_lock_shadow_stack(lock_type lock, int arg_no);

//===----------------------- Allocation related  --------------------------===//

void __softboundcets_stack_memory_deallocation(key_type ptr_key);

void __softboundcets_memory_deallocation(lock_type ptr_lock, key_type ptr_key);

void *__softboundcets_allocate_lock_location();

void __softboundcets_stack_memory_allocation(lock_type *ptr_lock,
                                             key_type *ptr_key);

void __softboundcets_memory_allocation(void *ptr, lock_type *ptr_lock,
                                       key_type *ptr_key);

//===---------------------- Free map operations  --------------------------===//

void __softboundcets_add_to_free_map(key_type ptr_key, void *ptr);

void __softboundcets_check_remove_from_free_map(key_type ptr_key, void *ptr);

#endif // Guard for temporal functions

#endif // SOFTBOUNDCETS_TEMPORAL_H
