#include "freelist.h"

#include "sizes.h"

#include <stdlib.h>
#include <sys/mman.h>

extern uint64_t page_size;

int is_aligned(uint64_t value, uint64_t alignment);
uint64_t __lowfat_ptr_size(uint64_t index);

// current top elements for every free list
Element *free_list_tops[NUM_REGIONS];

void free_list_push(unsigned index, void *addr) {
    Element *current_top = free_list_tops[index];

    Element *new_top = malloc(sizeof(Element));
    new_top->addr = addr;
    new_top->prev = current_top;

    free_list_tops[index] = new_top;

    // if the allocation size of the freed address is a multiple of page size we
    // can return the physical space to the OS for any other size it would not
    // be feasible to track when memory could be returned
    size_t allocation_size = __lowfat_ptr_size(index);
    if (is_aligned(allocation_size, page_size)) {
        munmap(addr, allocation_size);
        // remap the space again as we might reuse it
        // NOTE: This does not PHYSICALLY use space yet
        mmap(addr, allocation_size, PROT_NONE,
             MAP_ANONYMOUS | MAP_SHARED | MAP_NORESERVE, -1, 0);
    }
}

void *free_list_pop(unsigned index) {
    Element *current_top = free_list_tops[index];

    if (current_top != NULL) {
        free_list_tops[index] = current_top->prev;
        void *addr = current_top->addr;
        free(current_top);

        // if the allocation size of a freed allocation is a multiple of page
        // size, its memory was re-mmaped after it has been freed and it must be
        // made readable and writable again
        size_t allocation_size = __lowfat_ptr_size(index);
        if (is_aligned(allocation_size, page_size))
            mprotect(addr, allocation_size, PROT_READ | PROT_WRITE);

        return addr;
    }

    return NULL;
}

int free_list_is_empty(unsigned index) { return free_list_tops[index] == NULL; }
