#include "check.h"

#include "fail_function.h"
#include "sizes.h"
#include "statistics.h"

#include <stddef.h>
#include <stdint.h>
#include <stdio.h>

// the metadata table in the paper starts at 1, the regions used in this
// implementation start at 0, so subtract 1 after shift
uint64_t __lowfat_ptr_index(void *ptr) {
    return ((uint64_t)ptr >> REGION_SIZE_LOG) - 1;
}

uint64_t __lowfat_ptr_base(void *ptr, uint64_t index) {
    return (uint64_t)ptr & (UINT64_MAX << (index + MIN_PERMITTED_LF_SIZE_LOG));
}

uint64_t __lowfat_ptr_size(uint64_t index) {
    return MIN_PERMITTED_LF_SIZE << index;
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

void *__lowfat_get_lower_bound(void *ptr) {
    STAT_INC(NumGetLower)
    uint64_t index = __lowfat_ptr_index(ptr);
    if (index < NUM_REGIONS)
        return (void *)__lowfat_ptr_base(ptr, index);
    return (void *)WIDE_LOWER;
}

void *__lowfat_get_upper_bound(void *ptr) {
    STAT_INC(NumGetUpper)
    uint64_t index = __lowfat_ptr_index(ptr);
    if (index < NUM_REGIONS)
        return (void *)(__lowfat_ptr_base(ptr, index) +
                        __lowfat_ptr_size(index));

    return (void *)WIDE_UPPER;
}

void __lowfat_check_deref(void *witness, void *ptr, size_t size) {
    STAT_INC(NumDerefChecks);

    uint64_t index = __lowfat_ptr_index(witness);
    // optimized check: only need to compare base of ptr (+ size) and witness
    if (index < NUM_REGIONS) {
        STAT_INC(NumLowFatDerefChecks);
        if ((((uintptr_t)ptr + size) ^ (uintptr_t)witness) >>
                (index + MIN_PERMITTED_LF_SIZE_LOG) !=
            0) {
            __mi_debug_printf(
                "Error with\n\tPtr:\t%p, base %p, size %u, width %u\n", ptr,
                __lowfat_get_lower_bound(ptr),
                __lowfat_ptr_size(__lowfat_ptr_index(ptr)), size);
            __mi_debug_printf("\tWit:\t%p, base %p, size %u\n", witness,
                              __lowfat_get_lower_bound(witness),
                              __lowfat_ptr_size(index));
            __mi_fail_with_msg("Out-of-bounds pointer dereference!\n");
        }
    }
}

void __lowfat_check_oob(void *witness, void *ptr) {
    STAT_INC(NumInboundsChecks);

    uint64_t index = __lowfat_ptr_index(witness);
    if (index < NUM_REGIONS) {
        STAT_INC(NumLowFatInboundsChecks);
        if (((uintptr_t)ptr ^ (uintptr_t)witness) >>
                (index + MIN_PERMITTED_LF_SIZE_LOG) !=
            0) {
            __mi_debug_printf("Error with\n\tPtr:\t%p, base %p, size %u\n", ptr,
                              __lowfat_get_lower_bound(ptr),
                              __lowfat_ptr_size(__lowfat_ptr_index(ptr)));
            __mi_debug_printf("\tWit:\t%p, base %p, size %u\n", witness,
                              __lowfat_get_lower_bound(witness),
                              __lowfat_ptr_size(index));
            __mi_fail_with_msg("Outflowing out-of-bounds pointer!\n");
        }
    }
}
