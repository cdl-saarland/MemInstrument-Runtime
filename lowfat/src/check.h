#pragma once

#include <stddef.h>
#include <stdint.h>

uint64_t __lowfat_ptr_index(void *ptr)
    __attribute__((__weak__, __always_inline__));
uint64_t __lowfat_ptr_base(void *ptr, uint64_t index)
    __attribute__((__weak__, __always_inline__));
uint64_t __lowfat_ptr_size(uint64_t index)
    __attribute__((__weak__, __always_inline__));
uint64_t __lowfat_lookup_stack_size(int index);
int64_t __lowfat_lookup_stack_offset(int index);
void *__lowfat_compute_aligned(void *ptr, int index);
void *__lowfat_get_mirror(void *ptr, int64_t offset);

/*** interface for LLVM ***/

// lower/upper bound functions for Tina's tool PICO
void *__lowfat_get_lower_bound(void *ptr)
    __attribute__((__weak__, __always_inline__));
void *__lowfat_get_upper_bound(void *ptr)
    __attribute__((__weak__, __always_inline__));

void __lowfat_check_deref(void *witness, void *ptr, size_t size)
    __attribute__((__weak__, __always_inline__));
void __lowfat_check_oob(void *witness, void *ptr)
    __attribute__((__weak__, __always_inline__));
