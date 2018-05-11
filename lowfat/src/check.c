#include <stdint.h>
#include <stddef.h>
#include "fail_function.h"
#include "check.h"
#include "config.h"

// the metadata table in the paper starts at 1, the arrays used in this implementation start at 0, so shift is 33 instead of 32
uintptr_t _ptr_index(void *ptr) {
    return (uintptr_t) ptr >> 33;
}

uintptr_t _ptr_base(void *ptr) {
    return (uintptr_t) ptr & (UINT64_MAX << _ptr_index(ptr) + 3);
}

uintptr_t _ptr_size(void *ptr) {
    return sizes[_ptr_index(ptr)];
}

void __check_deref(void *ptr, uintptr_t base, uintptr_t size) {
    if ((uintptr_t) ptr - base >= size)
        __mi_fail_with_ptr("OOB dereference!", ptr);
}