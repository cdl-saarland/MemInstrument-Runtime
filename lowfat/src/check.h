#pragma once

#include "config.h"

extern size_t sizes[];
extern uint64_t inv_sizes[];

uint64_t __ptr_index(void *ptr);
uint64_t __lowfat_ptr_base(void *ptr);
uint64_t __lowfat_ptr_size(void *ptr);
void __lowfat_check_oob(void *ptr, uint64_t base, uint64_t size);