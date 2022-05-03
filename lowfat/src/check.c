#include "check.h"

#include "LFSizes.h"
#include "fail_function.h"
#include "statistics.h"

#include <stddef.h>
#include <stdint.h>
#include <stdio.h>

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

// TODO think about making the stack functions computation based rather than
// performing the lookup (as index/base/size)
uint64_t __lowfat_lookup_stack_size(int index) { return STACK_SIZES[index]; }

int64_t __lowfat_lookup_stack_offset(int index) { return STACK_OFFSETS[index]; }

void *__lowfat_compute_aligned(void *ptr, int index) {
    void *alignedPtr = (void *)((uintptr_t)ptr & STACK_MASKS[index]);
    __mi_debug_printf("Pointer %p aligned is %p\n", ptr, alignedPtr);
    return alignedPtr;
}

void *__lowfat_get_mirror(void *ptr, int64_t offset) {
    void *mirror = (void *)((uintptr_t)ptr + offset);
    __mi_debug_printf("Mirror of %p is %p\n", ptr, mirror);
    return mirror;
}

int __is_lowfat(void *ptr) {
    if (__lowfat_ptr_base_without_index(ptr)) {
        return 1;
    }
    return 0;
}

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

void __lowfat_check_deref(void *witness_base, void *ptr, size_t size) {
    STAT_INC(NumDerefChecks);

#ifdef MIRT_STATISTICS
    if (__is_lowfat(ptr)) {
        STAT_INC(NumLowFatDerefChecks);
    }
#endif

    uint64_t index = __lowfat_ptr_index(witness_base);
    uint64_t alloc_size = __lowfat_ptr_size(index);
    if ( // alloc_size < size ||
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
    if ( // alloc_size < size ||
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
