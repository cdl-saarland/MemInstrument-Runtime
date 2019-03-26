#include <stdint.h>
#include <stddef.h>
#include <stdio.h>
#include "check.h"
#include "statistics.h"
#include "fail_function.h"

// the metadata table in the paper starts at 1, the regions used in this implementation start at 0, so subtract 1 after shift
uint64_t __lowfat_ptr_index(void *ptr) {
    return ((uint64_t) ptr >> REGION_SIZE_LOG) - 1;
}

uint64_t __lowfat_ptr_base(void *ptr) {
    uint64_t index = __lowfat_ptr_index(ptr);
    if (index < NUM_REGIONS)
        return (uint64_t) ptr & (UINT64_MAX << (index + MIN_PERMITTED_LF_SIZE_LOG));
    else
        return 0;
}

uint64_t __lowfat_ptr_size(void *ptr) {
    uint64_t index = __lowfat_ptr_index(ptr);
    if (index < NUM_REGIONS)
        return MIN_PERMITTED_LF_SIZE << index;
    else
        return UINT64_MAX;
}

void *__lowfat_get_lower_bound(void *ptr) {
    STAT_INC(NumGetLower)
    return (void*) __lowfat_ptr_base(ptr);
}

void *__lowfat_get_upper_bound(void *ptr) {
    STAT_INC(NumGetUpper)
    return (void*) (__lowfat_ptr_base(ptr) + __lowfat_ptr_size(ptr));
}

void __lowfat_check_dereference(void *witness, void *ptr, size_t size) {
    STAT_INC(NumDerefChecks);

    uint64_t index = __lowfat_ptr_index(witness);
    // optimized check: only need to compare base of ptr (+ size) and witness
    if (index < NUM_REGIONS && (((uintptr_t) ptr + size) ^ (uintptr_t) witness) >> (index + MIN_PERMITTED_LF_SIZE_LOG) != 0)
        __mi_fail_with_ptr("Out-of-bounds pointer dereference!", ptr);
}

void __lowfat_check_inbounds(void *witness, void *ptr) {
    STAT_INC(NumInboundsChecks);

    uint64_t index = __lowfat_ptr_index(witness);
    if (index < NUM_REGIONS && ((uintptr_t) ptr ^ (uintptr_t) witness) >> (index + MIN_PERMITTED_LF_SIZE_LOG) != 0)
        __mi_fail_with_ptr("Outflowing out-of-bounds pointer!", ptr);
}