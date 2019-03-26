#pragma once

#include "config.h"

uint64_t __lowfat_ptr_index(void *ptr);
uint64_t __lowfat_ptr_base(void *ptr);
uint64_t __lowfat_ptr_size(void *ptr);

// interface for LLVM
void *__lowfat_get_lower_bound(void *ptr);
void *__lowfat_get_upper_bound(void *ptr);
void __lowfat_check_dereference(void *witness, void *ptr, size_t size);
void __lowfat_check_inbounds(void *witness, void *ptr);