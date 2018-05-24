#include <stdlib.h>
#include "freelist.h"
#include "config.h"

// current top elements for every free list
Element* free_list_tops[NUM_REGIONS];

void free_list_push(int index, void* addr) {
    Element* current_top = free_list_tops[index];

    Element* new_top = malloc(sizeof(Element));
    new_top->addr = addr;
    new_top->prev = current_top;

    free_list_tops[index] = new_top;
}

void* free_list_pop(int index) {
    Element* current_top = free_list_tops[index];

    if (current_top != NULL) {
        free_list_tops[index] = current_top->prev;
        void* addr = current_top->addr;
        free(current_top);
        return addr;
    }

    return NULL;
}

int free_list_is_empty(int index) {
    return free_list_tops[index] == NULL;
}
