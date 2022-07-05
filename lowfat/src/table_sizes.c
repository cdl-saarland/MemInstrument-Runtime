#include "core.h"

#include "LFSizes.h"
#include "fail_function.h"
#include "lowfat_defines.h"
#include "statistics.h"

#include <stdio.h>

#if MIRT_LF_TABLE

//===------------------- Determine Base/Index/Size ------------------------===//

uint64_t __lowfat_ptr_index(void *ptr) {
    return (uint64_t)ptr >> REGION_SIZE_LOG;
}

uintptr_t __lowfat_ptr_base(void *ptr, uint64_t index) {
    return (uintptr_t)ptr & ((uint64_t *)MAGICS_ADDRESS)[index];
}

uintptr_t __lowfat_ptr_base_without_index(void *ptr) {
    uint64_t index = __lowfat_ptr_index(ptr);
    return (uintptr_t)ptr & ((uint64_t *)MAGICS_ADDRESS)[index];
}

uint64_t __lowfat_ptr_size(uint64_t index) {
    return ((uint64_t *)SIZES_ADDRESS)[index];
}

uint64_t __lowfat_index_for_size(size_t size) {
    // We determine the index by counting leading zeros. A small size has many
    // leading zeros, and hence a large clz value. We subtract this from 64 to
    // get a small index for a large clz value. Additionally, we have to
    // consider that all allocations which are smaller than the smallest
    // allocation size are rounded up, and so also placed in region one. The
    // succeeding region indices are thereby also shifted by this amount.
    int diff = 64 - __builtin_clzll(size);
    int index = diff - MIN_ALLOC_SIZE_LOG;

    // Handle all cases that map to the smallest allocation size
    if (index <= 0) {
        return 1;
    }

    if (!__lowfat_is_power_of_2(size)) {
        index++;
    }

    return index;
}

int __is_lowfat(void *ptr) {
    if (__lowfat_ptr_base_without_index(ptr)) {
        return 1;
    }
    return 0;
}

uint64_t __lowfat_get_zero_based_index(uint64_t index) { return index - 1; }

//===----------------------------- Checks ---------------------------------===//

void __lowfat_check_deref(void *witness_base, void *ptr, size_t size) {
    STAT_INC(NumDerefChecks);

#ifdef MIRT_STATISTICS
    if (__is_lowfat(ptr)) {
        STAT_INC(NumLowFatDerefChecks);
    }
#endif

    uint64_t index = __lowfat_ptr_index(witness_base);
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
        __mi_debug_printf("\tWit:\t %p, size %u\n", witness_base, alloc_size);
        __mi_fail_with_msg("Out-of-bounds pointer dereference!\n");
    }
}

void __lowfat_check_deref_inner_witness(void *witness, void *ptr, size_t size) {
    STAT_INC(NumDerefChecks);

#ifdef MIRT_STATISTICS
    if (__is_lowfat(ptr)) {
        STAT_INC(NumLowFatDerefChecks);
    }
#endif

    uint64_t index = __lowfat_ptr_index(witness);
    uint64_t alloc_size = __lowfat_ptr_size(index);
    uintptr_t alloc_base = __lowfat_ptr_base(witness, index);
    if (
#ifdef MIRT_REPORT_PTR_OVERFLOW
        alloc_size < size ||
#endif
        (size_t)((uintptr_t)ptr - alloc_base) > (size_t)(alloc_size - size)) {
        __mi_debug_printf(
            "Error with\n\tPtr:\t%p, base %p, size %u, width %u\n", ptr,
            __lowfat_get_lower_bound(ptr),
            __lowfat_ptr_size(__lowfat_ptr_index(ptr)), size);
        __mi_debug_printf("\tWit:\t%p, base %p, size %u\n", witness, alloc_base,
                          alloc_size);
        __mi_fail_with_msg("Out-of-bounds pointer dereference!\n");
    }
}

void __lowfat_check_oob(void *witness, void *ptr) {
    STAT_INC(NumInboundsChecks);

#ifdef MIRT_STATISTICS
    if (__is_lowfat(ptr)) {
        STAT_INC(NumLowFatInboundsChecks);
    }
#endif

    uint64_t index = __lowfat_ptr_index(witness);
    uint64_t alloc_size = __lowfat_ptr_size(index);
    uintptr_t alloc_base = __lowfat_ptr_base(witness, index);
    if ((size_t)((uintptr_t)ptr - alloc_base) > (size_t)(alloc_size - 1)) {
        __mi_debug_printf("Error with\n\tPtr:\t%p, base %p, size %u\n", ptr,
                          __lowfat_get_lower_bound(ptr),
                          __lowfat_ptr_size(__lowfat_ptr_index(ptr)));
        __mi_debug_printf("\tWit:\t%p, base %p, size %u\n", witness, alloc_base,
                          alloc_size);
        __mi_fail_with_msg("Outflowing out-of-bounds pointer!\n");
    }
}

//===------------------------- Explicit Bounds ----------------------------===//

void *__lowfat_get_lower_bound(void *ptr) {
    STAT_INC(NumGetLower)
    return (void *)__lowfat_ptr_base_without_index(ptr);
}

void *__lowfat_get_upper_bound(void *ptr) {
    STAT_INC(NumGetUpper)
    // TODO This could be faster in the non-fat case, maybe optimize it
    uint64_t index = __lowfat_ptr_index(ptr);
    uintptr_t base = __lowfat_ptr_base(ptr, index);
    if (base) {
        return (void *)(base + __lowfat_ptr_size(index));
    }

    return (void *)WIDE_UPPER;
}

#endif
