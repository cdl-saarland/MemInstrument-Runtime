#include "softboundcets-spatial.h"

#include <assert.h>

#if __SOFTBOUNDCETS_SPATIAL_TEMPORAL || __SOFTBOUNDCETS_SPATIAL
//===----------------------------- Checks ---------------------------------===//

__WEAK_INLINE void __softboundcets_spatial_call_dereference_check(void *base,
                                                                  void *bound,
                                                                  void *ptr) {

#ifndef __NOSIM_CHECKS
    if ((base != bound) && (ptr != base)) {
#if NOERRORMISSINGBOUNDS
        if (base == NULL) {
            return;
        }
#endif

        __softboundcets_debug_printf(
            "In Call Dereference Check, base=%p, bound=%p, ptr=%p\n", base,
            bound, ptr);

        __softboundcets_abort();
    }
#endif
}

__WEAK_INLINE void
__softboundcets_spatial_load_dereference_check(void *base, void *bound,
                                               void *ptr, size_t size_of_type) {
#if ENABLE_RT_STATS
    __rt_stat_inc_sb_access_check();
#endif

    __softboundcets_debug_printf(
        "In Load Dereference Check, base=%p, bound=%p, "
        "ptr=%p, size_of_type=%zx, ptr+size=%p\n",
        base, bound, ptr, size_of_type, (char *)ptr + size_of_type);

    if ((ptr < base) || ((void *)((char *)ptr + size_of_type) > bound)) {
#if NOERRORMISSINGBOUNDS
        if (base == NULL) {
            return;
        }
#endif

        __softboundcets_printf(
            "Error in Load Dereference Check, base=%p, "
            "bound=%p, ptr=%p, size_of_type=%zx, ptr+size=%p\n",
            base, bound, ptr, size_of_type, (char *)ptr + size_of_type);
        __softboundcets_abort();
    }
}

__WEAK_INLINE void __softboundcets_spatial_store_dereference_check(
    void *base, void *bound, void *ptr, size_t size_of_type) {
#if ENABLE_RT_STATS
    __rt_stat_inc_sb_access_check();
#endif

    __softboundcets_debug_printf(
        "In Store Dereference Check, base=%p, bound=%p, "
        "ptr=%p, size_of_type=%zx, ptr+size=%p\n",
        base, bound, ptr, size_of_type, (char *)ptr + size_of_type);

    if ((ptr < base) || ((void *)((char *)ptr + size_of_type) > bound)) {
#if NOERRORMISSINGBOUNDS
        if (base == NULL) {
            return;
        }
#endif
        __softboundcets_printf(
            "Error in Store Dereference Check, base=%p, "
            "bound=%p, ptr=%p, size_of_type=%zx, ptr+size=%p\n",
            base, bound, ptr, size_of_type, (char *)ptr + size_of_type);
        __softboundcets_abort();
    }
}

//===-------------------- Shadow Stack Manipulation -----------------------===//

__WEAK_INLINE void *__softboundcets_load_base_shadow_stack(int arg_no) {
    assert(arg_no >= 0);
    size_t count =
        2 + arg_no * __SOFTBOUNDCETS_METADATA_NUM_FIELDS + __BASE_INDEX;
    size_t *base_ptr = (__softboundcets_shadow_stack_ptr + count);
    void *base = *((void **)base_ptr);
    return base;
}

__WEAK_INLINE void *__softboundcets_load_bound_shadow_stack(int arg_no) {

    assert(arg_no >= 0);
    size_t count =
        2 + arg_no * __SOFTBOUNDCETS_METADATA_NUM_FIELDS + __BOUND_INDEX;
    size_t *bound_ptr = (__softboundcets_shadow_stack_ptr + count);

    void *bound = *((void **)bound_ptr);
    return bound;
}

__WEAK_INLINE void __softboundcets_store_base_shadow_stack(void *base,
                                                           int arg_no) {

    assert(arg_no >= 0);
    size_t count =
        2 + arg_no * __SOFTBOUNDCETS_METADATA_NUM_FIELDS + __BASE_INDEX;
    void **base_ptr = (void **)(__softboundcets_shadow_stack_ptr + count);

    *(base_ptr) = base;
}

__WEAK_INLINE void __softboundcets_store_bound_shadow_stack(void *bound,
                                                            int arg_no) {

    assert(arg_no >= 0);
    size_t count =
        2 + arg_no * __SOFTBOUNDCETS_METADATA_NUM_FIELDS + __BOUND_INDEX;
    void **bound_ptr = (void **)(__softboundcets_shadow_stack_ptr + count);

    *(bound_ptr) = bound;
}

//===------------------------------ MISC ----------------------------------===//

__WEAK_INLINE void
__softboundcets_shrink_bounds(void *new_base, void *new_bound, void *old_base,
                              void *old_bound, void **base_alloca,
                              void **bound_alloca) {

    *(base_alloca) = new_base < old_base ? old_base : new_base;
    *(bound_alloca) = new_bound > old_bound ? old_bound : new_bound;
}

#endif
