#include <stddef.h>
#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <execinfo.h>
#include <unistd.h>

// for reading symbols from glibc (not portable)
#define __USE_GNU
#include <dlfcn.h>

#include "tree.h"

#define MAX_BACKTRACE_LENGTH 10
#define PRINTBACKTRACE {\
    void *buf[MAX_BACKTRACE_LENGTH];\
    int n = backtrace(buf, MAX_BACKTRACE_LENGTH);\
    backtrace_symbols_fd(buf, n, STDERR_FILENO);\
}

// from glibc (not portable)
extern void *__libc_malloc(size_t size);
extern void *__libc_calloc(size_t nmemb, size_t size);
extern void *__libc_realloc(void *ptr, size_t size);
extern void __libc_free(void* p);

int hooks_active = 1;

Tree memTree;

void setupSplay(void) {
    splayInit(&memTree);
}

static void __splay_fail(void *faultingPtr) {
    fprintf(stderr, "Memory safety violation!\n"
    "         / .'\n"
    "   .---. \\/\n"
    "  (._.' \\()\n"
    "   ^\"\"\"^\"\n"
    "Using out-of-bounds pointer %p\n"
    "\nBacktrace:\n", faultingPtr);
    PRINTBACKTRACE;
    exit(73);
}

void __splay_check_access(void* witness, void* ptr, size_t sz) {
    /* fprintf(stderr, "splay_check\n"); */
    Node* n = splayFind(&memTree, (uintptr_t)witness);
    if (n == NULL) {
        /* __splay_fail(ptr); */
        /* fprintf(stderr, "Check with non-existing witness!\n"); */
        return;
    }
    uintptr_t val = (uintptr_t)ptr;
    if (val < n->base || (val + sz) > n->bound) {
        __splay_fail(ptr);
    }
}

uintptr_t __splay_get_lower(void* witness) {
    Node* n = splayFind(&memTree, (uintptr_t)witness);
    if (n == NULL) {
        /* __splay_fail(ptr); */
        /* fprintf(stderr, "Check with non-existing witness!\n"); */
        return 0;
    }
    return n->base;
}

uintptr_t __splay_get_upper(void* witness) {
    Node* n = splayFind(&memTree, (uintptr_t)witness);
    if (n == NULL) {
        /* __splay_fail(ptr); */
        /* fprintf(stderr, "Check with non-existing witness!\n"); */
        return -1;
    }
    return n->bound;
}

void __splay_alloc_global(void* ptr, size_t sz) {
    /* fprintf(stderr, "splay_alloc\n"); */
    uintptr_t val = (uintptr_t)ptr;
    splayInsert(&memTree, val, val + sz, true);
}

void __splay_alloc(void* ptr, size_t sz) {
    /* fprintf(stderr, "splay_alloc\n"); */
    uintptr_t val = (uintptr_t)ptr;
    splayInsert(&memTree, val, val + sz, false);
}

void __splay_free(void* ptr) {
    /* fprintf(stderr, "splay_free\n"); */
    uintptr_t val = (uintptr_t)ptr;
    splayRemove(&memTree, val);
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

// TODO memalign, sbrk, reallocarray,...

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

typedef int (*fcn)(int *(main)(int, char **, char **), int argc, char **ubp_av, void (*init)(void), void (*fini) (void), void (*rtld_fini)(void), void (*stack_end));
int __libc_start_main(int *(main) (int, char **, char **), int argc, char **ubp_av, void (*init)(void), void (*fini)(void), void (*rtld_fini)(void), void (* stack_end)) {
    setupSplay();
    fcn start = (fcn)dlsym(RTLD_NEXT, "__libc_start_main");
    return (*start)(main, argc, ubp_av, init, fini, rtld_fini, stack_end);
}

