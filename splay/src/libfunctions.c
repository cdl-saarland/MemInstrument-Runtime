#include <stddef.h>
#include <stdlib.h>
#include <stdio.h>

#include "splay.h"

#include "statistics.h"
#include "config.h"

// for reading symbols from glibc (not portable)
#define __USE_GNU
#include <dlfcn.h>

// from glibc (not portable)
extern void *__libc_malloc(size_t size);
extern void *__libc_calloc(size_t nmemb, size_t size);
extern void *__libc_realloc(void *ptr, size_t size);
extern void __libc_free(void* p);

// TODO make threadsafe
static int hooks_active = 1;

typedef int (*start_main_type)(int *(main)(int, char **, char **), int argc, char **ubp_av, void (*init)(void), void (*fini) (void), void (*rtld_fini)(void), void (*stack_end));

static start_main_type start_main_found = NULL;

void initDynamicFunctions(void) {
    const char* libname = "libc.so";
    void* handle = dlopen(libname, RTLD_NOW | RTLD_LOCAL);
    char* msg = NULL;
    if ((msg = dlerror())) {
        fprintf(stderr, "Meminstrument: Error loading libc:\n%s", msg);
        exit(74);
    }

    start_main_found = (start_main_type)dlsym(handle, "__libc_start_main");

    if ((msg = dlerror())) {
        fprintf(stderr, "Meminstrument: Error finding libc symbols:\n%s", msg);
        exit(74);
    }
}

void* malloc(size_t size) {
    if (hooks_active) {
        hooks_active = 0;

        void* res = __libc_malloc(size);

        __splay_alloc(res, size);

        hooks_active = 1;
        return res;
    }
    return __libc_malloc(size);
}

void* calloc(size_t nmemb, size_t size) {
    if (hooks_active) {
        hooks_active = 0;

        void* res = __libc_calloc(nmemb, size);

        __splay_alloc(res, nmemb * size);

        hooks_active = 1;
        return res;
    }
    return __libc_calloc(nmemb, size);
}

void* realloc(void *ptr, size_t size) {
    if (hooks_active) {
        hooks_active = 0;

        __splay_free(ptr);

        void* res = __libc_realloc(ptr, size);

        __splay_alloc(res, size);

        hooks_active = 1;
        return res;
    }
    return __libc_realloc(ptr, size);
}

typedef int (*posix_memalign_type)(void**, size_t, size_t);

static posix_memalign_type found_posix_memalign;

typedef int memalign_type(void**, size_t, size_t);


/* int posix_memalign(void **memptr, size_t alignment, size_t size) { */
/*     posix_memalign_type fun = (posix_memalign_type)dlsym(RTLD_NEXT, "posix_memalign"); */
/*  */
/*     if (hooks_active) { */
/*         hooks_active = 0; */
/*  */
/*         int res = fun(memptr, size); */
/*  */
/*         if (!res) { */
/*             __splay_alloc(memptr, size); */
/*         } */
/*  */
/*         hooks_active = 1; */
/*         return res; */
/*     } */
/*     return fun(size); */
/* } */
/*  */
/* void *aligned_alloc(size_t alignment, size_t size); */
/* void *valloc(size_t size); */
/*  */
/* void *memalign(size_t alignment, size_t size); */
/* void *pvalloc(size_t size); */
/*  */

// TODO sbrk, reallocarray,...

void free(void* p) {
    if (hooks_active) {
        hooks_active = 0;

        __splay_free(p);

        __libc_free(p);

        hooks_active = 1;
        return;
    }
    __libc_free(p);
}

int __libc_start_main(int *(main) (int, char **, char **), int argc, char **ubp_av, void (*init)(void), void (*fini)(void), void (*rtld_fini)(void), void (* stack_end)) {

    initDynamicFunctions();

    __setup_splay();

    mi_prog_name = ubp_av[0];

    __splay_alloc(ubp_av, argc * sizeof(char*));

    for (int i = 0; i < argc; ++i) {
        char *c = ubp_av[i];
        size_t len = 0;
        while (*c != '\0') {
            len++;
            c++;
        }
        __splay_alloc(ubp_av[i], len * sizeof(char));
    }

#ifdef STATISTICS
    if (atexit(__print_stats) != 0) {
        fprintf(stderr, "meminstrument: Failed to register statistics printer!\n");
    }
#endif

    return (*start_main_found)(main, argc, ubp_av, init, fini, rtld_fini, stack_end);
}

