#include "softboundcets-spatial.h"

#include "fail_function.h"

#include <assert.h>

#if __SOFTBOUNDCETS_SPATIAL_TEMPORAL || __SOFTBOUNDCETS_SPATIAL
//===----------------------------- Checks ---------------------------------===//

__WEAK_INLINE void __softboundcets_spatial_call_dereference_check(
    const void *base, const void *bound, const void *ptr) {

    STAT_INC(CallCheck);

    __mi_debug_printf("In Call Dereference Check, base=%p, bound=%p, ptr=%p\n",
                      base, bound, ptr);
    if ((uintptr_t)base != (uintptr_t)bound ||
        (uintptr_t)ptr != (uintptr_t)base) {
#if __SOFTBOUNDCETS_NOERRORMISSINGBOUNDS
        if (base == NULL) {
            return;
        }
#endif

        __mi_debug_printf(
            "Error in Call Dereference Check, base=%p, bound=%p, ptr=%p\n",
            base, bound, ptr);

        __mi_fail_with_msg("Invalid function pointer dereference!\n");
    }
}

__WEAK_INLINE void __softboundcets_spatial_dereference_check(
    const void *base, const void *bound, const void *ptr, size_t size_of_type) {

    STAT_INC(SpatialAccessCheck);

    __mi_debug_printf("In Dereference Check, base=%p, "
                      "bound=%p, ptr=%p, size_of_type=%zx\n",
                      base, bound, ptr, size_of_type);

#ifdef MIRT_STATISTICS
    if (bound == (void *)WIDE_UPPER) {
        STAT_INC(WideBoundsCheck);
    }
#endif

    // Convert all pointers to uintptr_t, such that no undefined behavior
    // occurs in case `ptr` happens not to point to the same object as `base`
    // and `bound` do.
    // If size is larger than our bound, the access width is certainly larger
    // than the allocation and therefore invalid. This also ensures that the
    // subtraction will result in a meaningful value.
    if (
#ifdef MIRT_REPORT_PTR_OVERFLOW
        (uintptr_t)size_of_type > (uintptr_t)bound ||
#endif
        (uintptr_t)ptr < (uintptr_t)base ||
        (uintptr_t)ptr > ((uintptr_t)bound - (uintptr_t)size_of_type)) {
#if __SOFTBOUNDCETS_NOERRORMISSINGBOUNDS
        if (base == NULL) {
            return;
        }
#endif
#if __SOFTBOUNDCETS_ALLOWWIDTHZEROACCESS
        if (size_of_type == 0) {
            return;
        }
#endif
        __mi_debug_printf("Error in Dereference Check, base=%p, "
                          "bound=%p, ptr=%p, size_of_type=%zx\n",
                          base, bound, ptr, size_of_type);
        __mi_fail_with_msg("Out-of-bounds pointer dereference!\n");
    }
}

//===-------------------- Shadow Stack Manipulation -----------------------===//

__WEAK_INLINE void *__softboundcets_load_base_shadow_stack(int arg_no) {

    assert(arg_no >= 0);
    size_t count =
        2 + arg_no * __SOFTBOUNDCETS_METADATA_NUM_FIELDS + __BASE_INDEX;
    shadow_stack_ptr_type base_ptr = __softboundcets_shadow_stack_ptr + count;
    void *base = *((void **)base_ptr);
    __mi_debug_printf("Base loaded from shadow stack (location %i): %p\n",
                      arg_no, base);
    return base;
}

__WEAK_INLINE void *__softboundcets_load_bound_shadow_stack(int arg_no) {

    assert(arg_no >= 0);
    size_t count =
        2 + arg_no * __SOFTBOUNDCETS_METADATA_NUM_FIELDS + __BOUND_INDEX;
    shadow_stack_ptr_type bound_ptr = __softboundcets_shadow_stack_ptr + count;
    void *bound = *((void **)bound_ptr);
    __mi_debug_printf("Bound loaded from shadow stack (location %i): %p\n",
                      arg_no, bound);
    return bound;
}

__WEAK_INLINE void __softboundcets_store_base_shadow_stack(void *base,
                                                           int arg_no) {

    assert(arg_no >= 0);
    __mi_debug_printf("Base to store to shadow stack (location %i): %p\n",
                      arg_no, base);
    size_t count =
        2 + arg_no * __SOFTBOUNDCETS_METADATA_NUM_FIELDS + __BASE_INDEX;
    void **base_ptr = (void **)(__softboundcets_shadow_stack_ptr + count);

    *(base_ptr) = base;
}

__WEAK_INLINE void __softboundcets_store_bound_shadow_stack(void *bound,
                                                            int arg_no) {

    assert(arg_no >= 0);
    __mi_debug_printf("Bound to store to shadow stack (location %i): %p\n",
                      arg_no, bound);
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
