#include "core.h"

#include "LFSizes.h"
#include "fail_function.h"

//===-------------------------- Stack Related -----------------------------===//

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

//===------------------------------ Utils ---------------------------------===//

int __lowfat_is_aligned(uint64_t value, uint64_t alignment) {
    return (value & (alignment - 1)) == 0;
}

int __lowfat_is_power_of_2(size_t value) {
    return value != 0 && (value & (value - 1)) == 0;
}
