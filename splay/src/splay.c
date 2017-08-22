#include <stddef.h>
#include <stdlib.h>
#include <stdio.h>
#include <assert.h>

#include "tree.h"

extern void *__libc_malloc(size_t size);
extern void __libc_free(void* p);

int hooks_active = 1;

Tree memTree;

void setupSplay(void) {
    memTree.root = NULL;
}

static void __splay_fail(void) {
    fprintf(stderr, "Memory safety violation!\n"
    "         / .'\n"
    "   .---. \\/\n"
    "  (._.' \\()\n"
    "   ^\"\"\"^\"\n");
    exit(73);
}

void __splay_check_access(void* witness, void* ptr, size_t sz) {
    Node* n = splayFind(&memTree, (uintptr_t)witness);
    if (n == NULL) {
        __splay_fail();
    }
    uintptr_t val = (uintptr_t)ptr;
    if (val < n->base || (val + sz) > n->bound) {
        __splay_fail();
    }
}

uintptr_t __splay_get_lower(void* witness) {
    Node* n = splayFind(&memTree, (uintptr_t)witness);
    if (n == NULL) {
        __splay_fail();
    }
    return n->base;
}

uintptr_t __splay_get_upper(void* witness) {
    Node* n = splayFind(&memTree, (uintptr_t)witness);
    if (n == NULL) {
        __splay_fail();
    }
    return n->bound;
}

void __splay_alloc(void* ptr, size_t sz) {
    uintptr_t val = (uintptr_t)ptr;
    splayInsert(&memTree, val, val + sz);
}

void __splay_free(void* ptr) {
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

