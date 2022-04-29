#pragma once

#include <stdint.h>

// current implementation works like a stack (insertion and deletion as in LIFO)

// adds an element to the free list for the region corresponding to index
void __lowfat_free_list_push(unsigned index, void *addr);

// return current top element of the free list for the region corresponding to
// index if the free list is empty, returns NULL
void *__lowfat_free_list_pop(unsigned index);

// return 1 if empty, 0 otherwise
int __lowfat_free_list_is_empty(unsigned index);
