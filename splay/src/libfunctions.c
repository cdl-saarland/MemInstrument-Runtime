#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>

#include "splay.h"

#include "enable_mpx.h"
#include "fail_function.h"

#include "config.h"
#include "statistics.h"

#include "tracer.h"

// for reading symbols from glibc (not portable)
#define __USE_GNU
#include <dlfcn.h>

// TODO make threadsafe
static _Thread_local int hooks_active = 1;

typedef int (*start_main_type)(int *(main)(int, char **, char **), int argc,
                               char **ubp_av, void (*init)(void),
                               void (*fini)(void), void (*rtld_fini)(void),
                               void(*stack_end));
static start_main_type start_main_found = NULL;

void *__libc_malloc(size_t);

typedef void *(*malloc_type)(size_t);
static malloc_type malloc_found = __libc_malloc;

typedef void (*free_type)(void *);
static free_type free_found = NULL;

typedef void *(*calloc_type)(size_t, size_t);
static calloc_type calloc_found = NULL;

typedef void *(*realloc_type)(void *, size_t);
static realloc_type realloc_found = NULL;

typedef int *(*memalign_type)(size_t, size_t);
static memalign_type memalign_found = NULL;

typedef void *(*aligned_alloc_type)(size_t, size_t);
static aligned_alloc_type aligned_alloc_found = NULL;

typedef int (*posix_memalign_type)(void **, size_t, size_t);
static posix_memalign_type posix_memalign_found = NULL;

typedef void *(*valloc_type)(size_t);
static valloc_type valloc_found = NULL;

typedef void *(*pvalloc_type)(size_t);
static pvalloc_type pvalloc_found = NULL;

typedef void *(*mmap_type)(void *, size_t, int, int, int, off_t);
static mmap_type mmap_found = NULL;

typedef int (*munmap_type)(void *, size_t);
static munmap_type munmap_found = NULL;

void initDynamicFunctions(void) {
    const char *libname = "libc.so.6";
    void *handle = dlopen(libname, RTLD_NOW | RTLD_LOCAL);
    char *msg = NULL;
    if ((msg = dlerror())) {
        fprintf(stderr, "Meminstrument: Error loading libc:\n%s\n", msg);
        exit(74);
    }

    start_main_found = (start_main_type)dlsym(handle, "__libc_start_main");
    malloc_found = (malloc_type)dlsym(handle, "malloc");
    free_found = (free_type)dlsym(handle, "free");
    calloc_found = (calloc_type)dlsym(handle, "calloc");
    realloc_found = (realloc_type)dlsym(handle, "realloc");
    memalign_found = (memalign_type)dlsym(handle, "memalign");
    posix_memalign_found = (posix_memalign_type)dlsym(handle, "posix_memalign");
    aligned_alloc_found = (aligned_alloc_type)dlsym(handle, "aligned_alloc");
    valloc_found = (valloc_type)dlsym(handle, "valloc");
    pvalloc_found = (pvalloc_type)dlsym(handle, "pvalloc");

    mmap_found = (mmap_type)dlsym(handle, "mmap");
    munmap_found = (munmap_type)dlsym(handle, "munmap");

    if ((msg = dlerror())) {
        fprintf(stderr, "Meminstrument: Error finding libc symbols:\n%s\n",
                msg);
        exit(74);
    }
}

void *malloc(size_t size) {
    if (hooks_active) {
        hooks_active = 0;

        void *res = malloc_found(size + INCREASE_MALLOC_SIZE);

        __splay_alloc(res, size);

        hooks_active = 1;
        return res;
    }
    return malloc_found(size);
}

void *calloc(size_t nmemb, size_t size) {
    if (hooks_active) {
        hooks_active = 0;

        void *res = calloc_found(nmemb, size + INCREASE_MALLOC_SIZE);

        __splay_alloc(res, nmemb * size);

        hooks_active = 1;
        return res;
    }
    return calloc_found(nmemb, size);
}

void *realloc(void *ptr, size_t size) {
    if (hooks_active) {
        hooks_active = 0;

        __splay_free(ptr);

        void *res = realloc_found(ptr, size + INCREASE_MALLOC_SIZE);

        __splay_alloc(res, size);

        hooks_active = 1;
        return res;
    }
    return realloc_found(ptr, size);
}

void *memalign(size_t alignment, size_t size) {
    if (hooks_active) {
        hooks_active = 0;

        void *res = memalign_found(alignment, size + INCREASE_MALLOC_SIZE);

        __splay_alloc(res, size);

        hooks_active = 1;
        return res;
    }
    return memalign_found(alignment, size);
}

int posix_memalign(void **memptr, size_t alignment, size_t size) {
    if (hooks_active) {
        hooks_active = 0;

        int res = posix_memalign_found(memptr, alignment, size + INCREASE_MALLOC_SIZE);

        if (!res && *memptr) {
            __splay_alloc(*memptr, size);
        }

        hooks_active = 1;
        return res;
    }
    return posix_memalign_found(memptr, alignment, size);
}

void *aligned_alloc(size_t alignment, size_t size) {
    if (hooks_active) {
        hooks_active = 0;

        void *res = aligned_alloc_found(alignment, size + INCREASE_MALLOC_SIZE);

        __splay_alloc(res, size);

        hooks_active = 1;
        return res;
    }
    return aligned_alloc_found(alignment, size);
}

void *valloc(size_t size) {
    if (hooks_active) {
        hooks_active = 0;

        void *res = valloc_found(size + INCREASE_MALLOC_SIZE);

        __splay_alloc(res, size);

        hooks_active = 1;
        return res;
    }
    return valloc_found(size);
}

void *pvalloc(size_t size) {
    if (hooks_active) {
        hooks_active = 0;
        // we do not support this function, since it changes the allocation
        // size to a multiple of the page size
        __mi_fail_with_msg("Call to unsupported function \"pvalloc\"");
    }
    return valloc_found(size);
}

// TODO sbrk, reallocarray,...

void free(void *p) {
    if (hooks_active) {
        hooks_active = 0;

        __splay_free(p);

        free_found(p);

        hooks_active = 1;
        return;
    }
    free_found(p);
}

void *mmap(void *addr, size_t size, int prot, int flags, int fildes,
           off_t off) {
#ifdef ENABLE_MMAP_HOOK
    if (hooks_active) {
        hooks_active = 0;

        void *res = mmap_found(addr, size, prot, flags, fildes, off);

        __splay_alloc_or_replace(res, size);
        // enable replacing since munmap does not remove mappings from the tree.

        hooks_active = 1;
        return res;
    }
#endif
    return mmap_found(addr, size, prot, flags, fildes, off);
}

int munmap(void *addr, size_t size) {
    // TODO the semantics of munmap is weirder than that...
    /* if (hooks_active) { */
    /*     hooks_active = 0; */
    /*  */
    /*     __splay_free(addr); */
    /*  */
    /*     int res = munmap_found(addr, size); */
    /*  */
    /*     hooks_active = 1; */
    /*     return res; */
    /* } */
    return munmap_found(addr, size);
}

int __libc_start_main(int *(main)(int, char **, char **), int argc,
                      char **ubp_av, void (*init)(void), void (*fini)(void),
                      void (*rtld_fini)(void), void(*stack_end)) {
    hooks_active = 0;

    // get original functions from dynamic linker
    initDynamicFunctions();

    // set up statistics counters etc.
    __setup_statistics(ubp_av[0]);

    tracerInit(ubp_av[0]);

    // set up splay tree
    __setup_splay();

    // insert globals from the symbol table
    __splay_insertGlobalsFromBinary();

    // splay register argv array
    __splay_alloc(ubp_av, argc * sizeof(char *));

    // splay register argv content arrays
    for (int i = 0; i < argc; ++i) {
        char *c = ubp_av[i];
        size_t len = 1; // include the NULL byte
        while (*c != '\0') {
            len++;
            c++;
        }
        __splay_alloc(ubp_av[i], len * sizeof(char));
    }

#if ENABLE_MPX
    // call magic code to enable the intel mpx extension
    enable_mpx();
#endif

    // enable our malloc etc. replacements
    hooks_active = 1;

    // call the actual main function
    return (*start_main_found)(main, argc, ubp_av, init, fini, rtld_fini,
                               stack_end);
}
