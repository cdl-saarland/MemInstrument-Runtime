#include <stddef.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/mman.h>

// for reading symbols from glibc (not portable)
#define __USE_GNU
#include <dlfcn.h>
#include <stdint.h>

#include "statistics.h"
#include "config.h"

/*
 * framework to override standard c functions is taken from Fabian's libfunctions.c in the splay approach
 */

// supported object sizes for region based heap allocation (bigger size use original glibc functions)
size_t sizes[NUM_REGIONS] = {16, 32, 64, 128};

// pointers pointing to the next free memory space for each region
static void *regions[NUM_REGIONS];

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
        if (size > sizes[3])
            res = malloc_found(size);
        else {
            // get size and region index for size (round for non power of 2 sizes) by counting leading zeros
            // this currently only works if sizes[] only contains powers of 2
            int index = 64 -  __builtin_clzll(size) - 4; // -4 because the smallest size is 16 Bytes
            if (index < 0)
                index = 0;
            size_t allocation_size = sizes[index];
            res = regions[index];
            // allow read/write on allocated memory
            mprotect(res, allocation_size, PROT_READ | PROT_WRITE);

            // increase pointer in region for next allocation
            regions[index] += allocation_size;
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

        free_found(p);

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
