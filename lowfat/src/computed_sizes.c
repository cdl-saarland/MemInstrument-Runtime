#include "core.h"

#include "LFSizes.h"
#include "fail_function.h"
#include "statistics.h"

#if MIRT_LF_COMPUTED_SIZE

//===------------------- Determine Base/Index/Size ------------------------===//

uint64_t __lowfat_ptr_index(void *ptr) {
    return ((uint64_t)ptr >> REGION_SIZE_LOG) - 1;
}

uintptr_t __lowfat_ptr_base(void *ptr, uint64_t index) {
    return (uintptr_t)ptr & (UINT64_MAX << (index + MIN_ALLOC_SIZE_LOG));
}

uintptr_t __lowfat_ptr_base_without_index(void *ptr) {
    uint64_t index = __lowfat_ptr_index(ptr);
    return (uintptr_t)ptr & (UINT64_MAX << (index + MIN_ALLOC_SIZE_LOG));
}

uint64_t __lowfat_ptr_size(uint64_t index) { return MIN_ALLOC_SIZE << index; }

uint64_t __lowfat_index_for_size(size_t size) {
    // We determine the index by counting leading zeros. A small size has many
    // leading zeros, and hence a large clz value. We subtract this from 64 to
    // get a small index for a large clz value. Additionally, we have to
    // consider that all allocations which are smaller than the smallest
    // allocation size are rounded up, and so also placed in region zero. The
    // succeeding region indices are thereby also shifted by this amount.
    int index = 64 - __builtin_clzll(size) - MIN_ALLOC_SIZE_LOG;

    if (__lowfat_is_power_of_2(
            size)) { // corner case if size is already a power 2
        index--;
    }
    return index < 0 ? 0 : (unsigned)index;
}

static __attribute__((__always_inline__)) int
__is_lowfat_index(uint64_t index) {
    return index < NUM_REGIONS;
}

int __is_lowfat(void *ptr) {
    return __is_lowfat_index(__lowfat_ptr_index(ptr));
}

uint64_t __lowfat_get_zero_based_index(uint64_t index) { return index; }

//===----------------------------- Checks ---------------------------------===//

void __lowfat_check_deref(void *witness_base, void *ptr, size_t size) {
    STAT_INC(NumDerefChecks);

    uint64_t index = __lowfat_ptr_index(witness_base);
    if (__is_lowfat_index(index)) {
        STAT_INC(NumLowFatDerefChecks);

        uint64_t alloc_size = __lowfat_ptr_size(index);
        if (
#ifdef MIRT_REPORT_PTR_OVERFLOW
            alloc_size < size ||
#endif
            (size_t)((uintptr_t)ptr - (uintptr_t)witness_base) >
                (size_t)(alloc_size - size)) {
            __mi_debug_printf(
                "Error with\n\tPtr:\t%p, base %p, size %u, width %u\n", ptr,
                __lowfat_get_lower_bound(ptr),
                __lowfat_ptr_size(__lowfat_ptr_index(ptr)), size);
            __mi_debug_printf("\tWit:\t %p, size %u\n", witness_base,
                              alloc_size);
            __mi_fail_with_msg("Out-of-bounds pointer dereference!\n");
        }
    }
}

void __lowfat_check_deref_inner_witness(void *witness, void *ptr, size_t size) {
    STAT_INC(NumDerefChecks);

    uint64_t index = __lowfat_ptr_index(witness);
    if (__is_lowfat_index(index)) {
        STAT_INC(NumLowFatDerefChecks);

        uint64_t alloc_size = __lowfat_ptr_size(index);
        uintptr_t alloc_base = __lowfat_ptr_base(witness, index);
        if (
#ifdef MIRT_REPORT_PTR_OVERFLOW
            alloc_size < size ||
#endif
            (size_t)((uintptr_t)ptr - alloc_base) >
                (size_t)(alloc_size - size)) {
            __mi_debug_printf(
                "Error with\n\tPtr:\t%p, base %p, size %u, width %u\n", ptr,
                __lowfat_get_lower_bound(ptr),
                __lowfat_ptr_size(__lowfat_ptr_index(ptr)), size);
            __mi_debug_printf("\tWit:\t%p, base %p, size %u\n", witness,
                              alloc_base, alloc_size);
            __mi_fail_with_msg("Out-of-bounds pointer dereference!\n");
        }
    }
}

void __lowfat_check_oob(void *witness, void *ptr) {
    STAT_INC(NumInboundsChecks);

    uint64_t index = __lowfat_ptr_index(witness);
    if (__is_lowfat_index(index)) {
        STAT_INC(NumLowFatInboundsChecks);

        uint64_t alloc_size = __lowfat_ptr_size(index);
        uintptr_t alloc_base = __lowfat_ptr_base(witness, index);
        __mi_debug_printf("Check with\n\tPtr:\t%p, base %p, size %u\n", ptr,
                          __lowfat_get_lower_bound(ptr),
                          __lowfat_ptr_size(__lowfat_ptr_index(ptr)));
        __mi_debug_printf("\tWit:\t%p, base %p, size %u\n", witness, alloc_base,
                          alloc_size);
        if ((size_t)((uintptr_t)ptr - alloc_base) > (size_t)(alloc_size - 1)) {
            __mi_debug_printf("Error with\n\tPtr:\t%p, base %p, size %u\n", ptr,
                              __lowfat_get_lower_bound(ptr),
                              __lowfat_ptr_size(__lowfat_ptr_index(ptr)));
            __mi_debug_printf("\tWit:\t%p, base %p, size %u\n", witness,
                              alloc_base, alloc_size);
            __mi_fail_with_msg("Outflowing out-of-bounds pointer!\n");
        }
    }
}
//===------------------------- Explicit Bounds ----------------------------===//

void *__lowfat_get_lower_bound(void *ptr) {
    STAT_INC(NumGetLower)
    uint64_t index = __lowfat_ptr_index(ptr);
    if (__is_lowfat_index(index)) {
        return (void *)__lowfat_ptr_base(ptr, index);
    }
    return (void *)WIDE_LOWER;
}

void *__lowfat_get_upper_bound(void *ptr) {
    STAT_INC(NumGetUpper)
    uint64_t index = __lowfat_ptr_index(ptr);
    if (__is_lowfat_index(index)) {
        return (void *)(__lowfat_ptr_base(ptr, index) +
                        __lowfat_ptr_size(index));
    }

    return (void *)WIDE_UPPER;
}

#endif