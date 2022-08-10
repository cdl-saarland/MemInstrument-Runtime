#pragma once

#include "lowfat_defines.h"

#include <stddef.h>
#include <stdint.h>

//===----------------------------------------------------------------------===//
//                         Determine Base/Index/Size
//===----------------------------------------------------------------------===//

// Determine the region index for the given pointer.
uint64_t __lowfat_ptr_index(void *ptr) __INTERNAL_FUNCTIONS_ATTRIBUTES;

// Determine the base for the given pointer, which has region index `index`.
uintptr_t __lowfat_ptr_base(void *ptr,
                            uint64_t index) __INTERNAL_FUNCTIONS_ATTRIBUTES;

// Determine the base for the given pointer (includes calculating the region
// index first).
uintptr_t
__lowfat_ptr_base_without_index(void *ptr) __INTERNAL_FUNCTIONS_ATTRIBUTES;

// Determine the size of the object allocated at the given region index.
uint64_t __lowfat_ptr_size(uint64_t index) __INTERNAL_FUNCTIONS_ATTRIBUTES;

// Determine the index for the given size.
uint64_t __lowfat_index_for_size(size_t size) __INTERNAL_FUNCTIONS_ATTRIBUTES;

// Returns 1 if the pointer is low-fat, zero otherwise
int __is_lowfat(void *ptr) __INTERNAL_FUNCTIONS_ATTRIBUTES;

//===----------------------------------------------------------------------===//
//                              Stack Related
//===----------------------------------------------------------------------===//

// Look up the size of the allocation with the given LowFat index.
uint64_t __lowfat_lookup_stack_size(int index) __INTERNAL_FUNCTIONS_ATTRIBUTES;

// Look up the offset for the given LowFat index.
int64_t __lowfat_lookup_stack_offset(int index) __INTERNAL_FUNCTIONS_ATTRIBUTES;

// Determine the proper alignment to place ptr on the stack.
void *__lowfat_compute_aligned(void *ptr,
                               int index) __INTERNAL_FUNCTIONS_ATTRIBUTES;

// Compute the mirrored pointer for a stack allocation.
void *__lowfat_get_mirror(void *ptr,
                          int64_t offset) __INTERNAL_FUNCTIONS_ATTRIBUTES;

//===----------------------------------------------------------------------===//
//                                  Checks
//===----------------------------------------------------------------------===//

// Check whether a (size-byte) access to the ptr is within bounds of the same
// allocation as witness, where witness points to the base of the allocation.
void __lowfat_check_deref(void *witness, void *ptr,
                          size_t size) __CHECK_ATTRIBUTES;

// Check whether a (one-byte) access to the ptr is within bounds of the same
// allocation as witness, with witness pointing somewhere into an allocation.
void __lowfat_check_deref_inner_witness(void *witness, void *ptr,
                                        size_t size) __CHECK_ATTRIBUTES;

// Check whether a (one-byte) access to the ptr is within bounds of the same
// allocation as witness.
void __lowfat_check_oob(void *witness, void *ptr) __CHECK_ATTRIBUTES;

//===----------------------------------------------------------------------===//
//                              Explicit Bounds
//===----------------------------------------------------------------------===//

// Determine the lower/upper bound value for the given pointer.
// Return wide bounds for non-fat pointers.
void *__lowfat_get_lower_bound(void *ptr) __INTERNAL_FUNCTIONS_ATTRIBUTES;
void *__lowfat_get_upper_bound(void *ptr) __INTERNAL_FUNCTIONS_ATTRIBUTES;

//===----------------------------------------------------------------------===//
//                                  Utils
//===----------------------------------------------------------------------===//

// For some data structures, we only care about low-fat regions and hence we
// want to start indexing into them at zero. Depending on the implementation,
// this either corresponds directly to the region index, or, if region zero is a
// special region, we need to shift it by one.
uint64_t
__lowfat_get_zero_based_index(uint64_t index) __INTERNAL_FUNCTIONS_ATTRIBUTES;

// Determine whether the given value is `alignment` aligned.
// Returns 1 if value is a multiple of alignment, and 0 otherwise.
int __lowfat_is_aligned(uint64_t value,
                        uint64_t alignment) __INTERNAL_FUNCTIONS_ATTRIBUTES;

// Returns 1 if value is a power of two, and 0 otherwise.
int __lowfat_is_power_of_2(size_t value) __INTERNAL_FUNCTIONS_ATTRIBUTES;
