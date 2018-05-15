#pragma once

#include <stdint.h>

// current implementation works like a stack (insertion and deletion as in LIFO)

// represents an element in a free list
typedef struct Element {
    void* addr;
    struct Element* prev;
} Element;

// adds an element to the free list for the region corresponding to index
void free_list_push(int index, void *addr);

// return current top element of the free list for the region corresponding to index
// if the free list is empty, returns NULL
void * free_list_pop(int index);
