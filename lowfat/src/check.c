#include <stdint.h>
#include <stddef.h>
#include <stdio.h>
#include "fail_function.h"
#include "check.h"

// the metadata table in the paper starts at 1, the arrays used in this implementation start at 0, so subtract 1 after shift
uint64_t __lowfat_ptr_index(void *ptr) {
    return ((uint64_t) ptr >> REGION_SIZE_LOG) - 1;
}

uint64_t __lowfat_ptr_base(void *ptr) {
    uint64_t index = __lowfat_ptr_index(ptr);
    if (index < NUM_REGIONS) {
#if defined(POW_2_SIZES)
        return (uint64_t) ptr & (UINT64_MAX << (index + 3));
#elif defined(FAST_BASE)
        return (uint64_t) ((((__uint128_t) ptr * (__uint128_t) inv_sizes[index]) >> 64U) * sizes[index]);
#else
        uint64_t size = __ptr_size(ptr);
        return ((uint64_t) ptr / size) * size;
#endif
    } else
        return 0;
}

uint64_t __lowfat_ptr_size(void *ptr) {
    uint64_t index = __lowfat_ptr_index(ptr);
    if (index < NUM_REGIONS)
        return sizes[index];
    else
        return UINT64_MAX;
}

void *__lowfat_get_lower_bound(void *ptr) {
    return (void*) __lowfat_ptr_base(ptr);
}

void *__lowfat_get_upper_bound(void *ptr) {
    return (void*) (__lowfat_ptr_base(ptr) + __lowfat_ptr_size(ptr));
}

void __lowfat_check_oob(void *witness, void *ptr) {
    if ((uint64_t) ptr - __lowfat_ptr_base(witness) >= __lowfat_ptr_size(witness))
        __mi_fail_with_ptr("OOB dereference!", ptr);
}