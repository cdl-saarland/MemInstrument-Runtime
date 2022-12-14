#include "freelist.h"

#include "LFSizes.h"
#include "core.h"
#include "statistics.h"

#include <stdlib.h>
#include <sys/mman.h>

extern uint64_t __lowfat_page_size;

// represents an element in a free list
typedef struct Element {
    void *addr;
    struct Element *prev;
} Element;

// current top elements for every free list
static Element *__lowfat_free_list_tops[NUM_REGIONS];

void __lowfat_free_list_push(unsigned index, void *addr) {
    Element *current_top = __lowfat_free_list_tops[index];

    // Allocate a new element container for the free list.
    // We disable increments of stat counters for this to not mess up our
    // counts for non-low-fat mallocs.
    // Note: These allocations might introduce overhead that could be reduced.
    __mi_disable_stats();
    Element *new_top = malloc(sizeof(Element));
    __mi_enable_stats();

    new_top->addr = addr;
    new_top->prev = current_top;

    __lowfat_free_list_tops[index] = new_top;

    // if the allocation size of the freed address is a multiple of page size we
    // can return the physical space to the OS for any other size it would not
    // be feasible to track when memory could be returned
    size_t allocation_size = __lowfat_ptr_size(index);
    if (__lowfat_is_aligned(allocation_size, __lowfat_page_size)) {
        munmap(addr, allocation_size);
        // remap the space again as we might reuse it
        // NOTE: This does not PHYSICALLY use space yet
        mmap(addr, allocation_size, PROT_NONE,
             MAP_ANONYMOUS | MAP_SHARED | MAP_NORESERVE, -1, 0);
    }
}

void *__lowfat_free_list_pop(unsigned index) {
    Element *current_top = __lowfat_free_list_tops[index];

    if (current_top != NULL) {
        __lowfat_free_list_tops[index] = current_top->prev;
        void *addr = current_top->addr;
        free(current_top);

        // if the allocation size of a freed allocation is a multiple of page
        // size, its memory was re-mmaped after it has been freed and it must be
        // made readable and writable again
        size_t allocation_size = __lowfat_ptr_size(index);
        if (__lowfat_is_aligned(allocation_size, __lowfat_page_size))
            mprotect(addr, allocation_size, PROT_READ | PROT_WRITE);

        return addr;
    }

    return NULL;
}

int __lowfat_free_list_is_empty(unsigned index) {
    return __lowfat_free_list_tops[index] == NULL;
}
