#pragma once

#include "config.h"

extern size_t sizes[];

#ifdef FAST_BASE
extern uint64_t inv_sizes[];
#endif

uint64_t __lowfat_ptr_index(void *ptr);
uint64_t __lowfat_ptr_base(void *ptr);
uint64_t __lowfat_ptr_size(void *ptr);

// interface for LLVM
void *__lowfat_get_lower_bound(void *ptr);
void *__lowfat_get_upper_bound(void *ptr);
void __lowfat_check_oob(void *witness, void *ptr);