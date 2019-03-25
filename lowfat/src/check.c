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
        return (uint64_t) ptr & (UINT64_MAX << (index + 3));
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

void __lowfat_check_deref(void *witness, void *ptr, size_t size) {
    STAT_INC(NumDerefChecks);
    if ((uint64_t) ptr - __lowfat_ptr_base(witness) + size > __lowfat_ptr_size(witness))
        __mi_fail_with_ptr("OOB dereference!", ptr);
}

void __lowfat_check_oob(void *witness, void *ptr) {
    STAT_INC(NumInboundsChecks);
    if ((uint64_t) ptr - __lowfat_ptr_base(witness) >= __lowfat_ptr_size(witness))
        __mi_fail_with_ptr("Pointer not inbounds!", ptr);
}