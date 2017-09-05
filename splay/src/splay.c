#include <stddef.h>
#include <stdlib.h>
#include <stdio.h>
#include <assert.h>

// for reading symbols from glibc (not portable)
#define __USE_GNU
#include <dlfcn.h>

#include "tree.h"

#define NULLPTR_BASE 0
#define NULLPTR_BOUND 4096

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

void __splay_check_inbounds(void* witness, void* ptr) {
    uintptr_t witness_val = (uintptr_t) witness;
    uintptr_t ptr_val = (uintptr_t) ptr;
    if (ptr_val < NULLPTR_BOUND) {
        if (witness_val < NULLPTR_BOUND) {
            return;
        }
        splayFail("Outflowing out-of-bounds pointer", ptr);
    }
    Node* n = splayFind(&memTree, witness_val);
    if (n == NULL) {
        /* splayFail(ptr); */
        return;
    }
    if (ptr_val < n->base || ptr_val >= n->bound) {
        // ignore the potential access size here
        splayFail("Outflowing out-of-bounds pointer", ptr);
    }
}

void __splay_check_inbounds_named(void* witness, void* ptr, char* name) {
    uintptr_t witness_val = (uintptr_t) witness;
    uintptr_t ptr_val = (uintptr_t) ptr;
    if (ptr_val < NULLPTR_BOUND) {
        if (witness_val < NULLPTR_BOUND) {
            return;
        }
        splayFail("Outflowing out-of-bounds pointer", ptr);
    }
    Node* n = splayFind(&memTree, witness_val);
    if (n == NULL) {
        /* splayFail(ptr); */
        fprintf(stderr, "Inbounds check with non-existing witness for %p (%s)!\n", ptr, name);
        return;
    }
    if (ptr_val < n->base || ptr_val >= n->bound) {
        splayFail("Outflowing out-of-bounds pointer", ptr);
    }
}


void __splay_check_dereference(void* witness, void* ptr, size_t sz) {
    uintptr_t ptr_val = (uintptr_t) ptr;
    if (ptr_val < NULLPTR_BOUND) {
        splayFail("NULL dereference", ptr);
    }
    Node* n = splayFind(&memTree, (uintptr_t)witness);
    if (n == NULL) {
        /* splayFail(ptr); */
        return;
    }
    if (ptr_val < n->base || (ptr_val + sz) > n->bound) {
        splayFail("Out-of-bounds dereference", ptr);
    }
}

void __splay_check_dereference_named(void* witness, void* ptr, size_t sz, char* name) {
    uintptr_t ptr_val = (uintptr_t) ptr;
    if (ptr_val < NULLPTR_BOUND) {
        splayFail("NULL dereference", ptr);
    }
    Node* n = splayFind(&memTree, (uintptr_t)witness);
    if (n == NULL) {
        /* splayFail(ptr); */
        fprintf(stderr, "Access check with non-existing witness for %p (%s)!\n", ptr, name);
        return;
    }
    if (ptr_val < n->base || (ptr_val + sz) > n->bound) {
        splayFail("Out-of-bounds dereference", ptr);
    }
}

uintptr_t __splay_get_lower(void* witness) {
    Node* n = splayFind(&memTree, (uintptr_t)witness);
    if (n == NULL) {
        splayFail("Taking bounds of unknown allocation", witness);
        /* fprintf(stderr, "Check with non-existing witness!\n"); */
        return 0;
    }
    return n->base;
}

uintptr_t __splay_get_upper(void* witness) {
    Node* n = splayFind(&memTree, (uintptr_t)witness);
    if (n == NULL) {
        splayFail("Taking bounds of unknown allocation", witness);
        /* fprintf(stderr, "Check with non-existing witness!\n"); */
        return -1;
    }
    return n->bound;
}

void __splay_alloc_or_merge(void* ptr, size_t sz) {
    uintptr_t val = (uintptr_t)ptr;
    splayInsert(&memTree, val, val + sz, IB_EXTEND);
}

void __splay_alloc(void* ptr, size_t sz) {
    uintptr_t val = (uintptr_t)ptr;
    splayInsert(&memTree, val, val + sz, IB_ERROR);
}

void __splay_alloc_or_replace(void* ptr, size_t sz) {
    uintptr_t val = (uintptr_t)ptr;
    splayInsert(&memTree, val, val + sz, IB_REPLACE);
}

void __splay_free(void* ptr) {
    uintptr_t val = (uintptr_t)ptr;
    if (val == 0) {
        return;
    }
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

    fcn start = (fcn)dlsym(RTLD_NEXT, "__libc_start_main");
    return (*start)(main, argc, ubp_av, init, fini, rtld_fini, stack_end);
}

