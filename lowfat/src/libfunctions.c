#include <stddef.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/mman.h>

// for reading symbols from glibc (not portable)
#define __USE_GNUS
#include <dlfcn.h>
#include <stdint.h>

#include "statistics.h"
#include "config.h"

/*
 * framework to override standard c functions is taken from Fabian's libfunctions.c in the splay approach
 */
 
uintptr_t _ptr_index(void *ptr);

// represents an element in a free list 
struct free_list_element {
    void *addr;
    struct free_list_element *next;
};
typedef struct free_list_element freed_node;

// supported object sizes for region based heap allocation (bigger size use original glibc functions)
// TODO make this easier to configure
size_t sizes[NUM_REGIONS] = {16, 32, 64, 128};

// pointers pointing to the next free memory space for each region
static void *regions[NUM_REGIONS];

// free list for every region
static freed_node *free_lists[NUM_REGIONS];


// TODO make threadsafe
static int hooks_active = 0;

typedef int (*start_main_type)(int *(main)(int, char **, char **), int argc, char **ubp_av, void (*init)(void), void (*fini) (void), void (*rtld_fini)(void), void (*stack_end));
static start_main_type start_main_found = NULL;


void* __libc_malloc(size_t);

typedef void*(*malloc_type)(size_t);
static malloc_type malloc_found = __libc_malloc;

typedef void(*free_type)(void*);
static free_type free_found = NULL;

typedef void*(*calloc_type)(size_t, size_t);
static calloc_type calloc_found = NULL;

typedef void*(*realloc_type)(void*, size_t);
static realloc_type realloc_found = NULL;

typedef void*(*aligned_alloc_type)(size_t, size_t);
static aligned_alloc_type aligned_alloc_found = NULL;

/* typedef int (*posix_memalign_type)(void**, size_t, size_t); */
/* static posix_memalign_type posix_memalign_found; */

/* typedef int memalign_type(void**, size_t, size_t); */
/* static memalign_type found_memalign; */

void initDynamicFunctions(void) {
    const char* libname = "libc.so.6";
    void* handle = dlopen(libname, RTLD_NOW | RTLD_LOCAL);
    char* msg = NULL;
    if ((msg = dlerror())) {
        fprintf(stderr, "Meminstrument: Error loading libc:\n%s\n", msg);
        exit(74);
    }

    start_main_found = (start_main_type)dlsym(handle, "__libc_start_main");
    malloc_found = (malloc_type)dlsym(handle, "malloc");
    free_found = (free_type)dlsym(handle, "free");
    calloc_found = (calloc_type)dlsym(handle, "calloc");
    realloc_found = (realloc_type)dlsym(handle, "realloc");
    aligned_alloc_found = (aligned_alloc_type)dlsym(handle, "aligned_alloc");
    /* posix_memalign_found = (posix_memalign_type)dlsym(handle, "posix_memalign"); */
    /* memalign_found = (memalign_type)dlsym(handle, "memalign"); */

    if ((msg = dlerror())) {
        fprintf(stderr, "Meminstrument: Error finding libc symbols:\n%s\n", msg);
        exit(74);
    }
}

void* malloc(size_t size) {
    if (hooks_active) {
        hooks_active = 0;

        void* res;
        // use glibc malloc for sizes that are too large (-> non low fat pointer)
        if (size > sizes[NUM_REGIONS - 1])
            res = malloc_found(size);
        else {
            // get size and region index for size (round for non power of 2 sizes) by counting leading zeros
            // this currently only works if sizes[] only contains powers of 2
            // TODO 32/64 depending on system
            int index = 64 -  __builtin_clzll(size) - 5; // -5 because the smallest size is 16 Bytes
            if (index < 0)
                index = 0;

            // first check free list for corresponding region
            freed_node *current = free_lists[index];
            if (current != NULL) {
                res = current->addr;
                free_lists[index] = current->next;
            }
            // otherwise use fresh space
            else {
                size_t allocation_size = sizes[index];
                res = regions[index];

                // increase pointer in region to point to fresh space for next allocation
                regions[index] += allocation_size;

                // allow read/write on allocated memory (only required for page aligned addresses)
                // TODO page alignment test only works for 4KB (0xFFF) pages, should be more general
                if (((uintptr_t) res & 0xFFF) == 0)
                    mprotect(res, allocation_size, PROT_READ | PROT_WRITE);
            }
        }

        hooks_active = 1;
        return res;
    }
    return malloc_found(size);
}

void* calloc(size_t nmemb, size_t size) {
    if (hooks_active) {
        hooks_active = 0;

        void* res = calloc_found(nmemb, size);

        hooks_active = 1;
        return res;
    }
    return calloc_found(nmemb, size);
}

void* realloc(void *ptr, size_t size) {
    if (hooks_active) {
        hooks_active = 0;

        void* res = realloc_found(ptr, size);

        hooks_active = 1;
        return res;
    }
    return realloc_found(ptr, size);
}

/* int posix_memalign(void **memptr, size_t alignment, size_t size) { */
/*     if (hooks_active) { */
/*         hooks_active = 0; */
/*  */
/*         int res = posix_memalign_found(memptr, alignment, size); */
/*  */
/*         if (!res) { */
/*             __splay_alloc(memptr, size); */
/*         } */
/*  */
/*         hooks_active = 1; */
/*         return res; */
/*     } */
/*     return posix_memalign_found(memptr, alignment, size); */
/* } */

void *aligned_alloc(size_t alignment, size_t size) {
    if (hooks_active) {
        hooks_active = 0;

        void* res = aligned_alloc_found(alignment, size);

        hooks_active = 1;
        return res;
    }
    return aligned_alloc_found(alignment, size);
}

/* void *valloc(size_t size); */
/*  */
/* void *memalign(size_t alignment, size_t size); */
/* void *pvalloc(size_t size); */


// TODO sbrk, reallocarray,...

void free(void* p) {
    if (hooks_active) {
        hooks_active = 0;

        // add freed address to free list for corresponding region (LIFO principle)
        uintptr_t index = _ptr_index(p);
        if (index > 0 && index < NUM_REGIONS) {
            freed_node *new_freed = malloc(sizeof(freed_node));
            new_freed->addr = p;
            new_freed->next = NULL;

            freed_node *current = free_lists[index];
            if (current == NULL)
                free_lists[index] = new_freed;
            else
                current->next = new_freed;
        }

        hooks_active = 1;
        return;
    }
    free_found(p);
}

#if ENABLE_MPX
void enable_mpx(void);
#endif

int __libc_start_main(int *(main) (int, char **, char **), int argc, char **ubp_av, void (*init)(void), void (*fini)(void), void (*rtld_fini)(void), void (* stack_end)) {

    // create regions for each size
    for (int i = 0; i < NUM_REGIONS; i++) {
        size_t region_address = (i+1) * REGION_SIZE;
        regions[i] = mmap((void *) region_address, REGION_SIZE, PROT_NONE, MAP_ANONYMOUS | MAP_SHARED | MAP_NORESERVE, -1, 0);
    }

    // get original functions from dynamic linker
    initDynamicFunctions();

    // set up statistics counters etc.
    __setup_statistics(ubp_av[0]);

#if ENABLE_MPX
    // call magic code to enable the intel mpx extension
    enable_mpx();
#endif

    // enable our malloc etc. replacements
    hooks_active = 1;

    // call the actual main function
    return (*start_main_found)(main, argc, ubp_av, init, fini, rtld_fini, stack_end);
}
