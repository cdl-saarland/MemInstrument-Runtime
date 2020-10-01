#ifndef SOFTBOUNDCETS_SPATIAL_H
#define SOFTBOUNDCETS_SPATIAL_H

#include "softboundcets-defines.h"

// All functions in this header are there to support spatial memory safety.
// If the runtime is not configured to ensure spatial safety, these functions
// are not available.
#if __SOFTBOUNDCETS_SPATIAL_TEMPORAL || __SOFTBOUNDCETS_SPATIAL

//===----------------------------- Checks ---------------------------------===//

__WEAK_INLINE void __softboundcets_spatial_call_dereference_check(void *base,
                                                                  void *bound,
                                                                  void *ptr);

/// Both spatial load and store dereference checks make sure that the given
/// pointer (+ width) is in between base and bound.
__WEAK_INLINE void
__softboundcets_spatial_load_dereference_check(void *base, void *bound,
                                               void *ptr, size_t size_of_type);

__WEAK_INLINE void
__softboundcets_spatial_store_dereference_check(void *base, void *bound,
                                                void *ptr, size_t size_of_type);

//===-------------------- Shadow Stack Manipulation -----------------------===//

__WEAK_INLINE void *__softboundcets_load_base_shadow_stack(int arg_no);

__WEAK_INLINE void *__softboundcets_load_bound_shadow_stack(int arg_no);

__WEAK_INLINE void __softboundcets_store_base_shadow_stack(void *base,
                                                           int arg_no);

__WEAK_INLINE void __softboundcets_store_bound_shadow_stack(void *bound,
                                                            int arg_no);

//===------------------------------ MISC ----------------------------------===//

/// Bounds can be shrunk to detect intra-object overflows (e.g. from one field
/// in a struct to another), such that only a sub-object part is allowed to be
/// accessed.
/// Note that the C++ implementation will currently not do shrinking, e.g.,
/// these cases remain undetected and this function won't be called.
__WEAK_INLINE void
__softboundcets_shrink_bounds(void *new_base, void *new_bound, void *old_base,
                              void *old_bound, void **base_alloca,
                              void **bound_alloca);

#endif // Guard for spatial functions

#endif // SOFTBOUNDCETS_SPATIAL_H
