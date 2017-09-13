#include <stdlib.h>
#include <stdio.h>
#include <execinfo.h>
#include <unistd.h>

#include "config.h"
#include "statistics.h"

#include "tree.h"
#include "splay.h"

Tree memTree;

#define PRINTBACKTRACE {\
    void *buf[MAX_BACKTRACE_LENGTH];\
    int n = backtrace(buf, MAX_BACKTRACE_LENGTH);\
    backtrace_symbols_fd(buf, n, STDERR_FILENO);\
}

void __setup_splay(void) {
    splayInit(&memTree);
}

void __splay_fail(const char* msg, void *faultingPtr) {
    fprintf(stderr, "Memory safety violation!\n"
    "         / .'\n"
    "   .---. \\/\n"
    "  (._.' \\()\n"
    "   ^\"\"\"^\"\n"
    "%s with pointer %p\n"
    "\nBacktrace:\n", msg, faultingPtr);
    PRINTBACKTRACE;
    exit(73);
}

void __splay_check_inbounds(void* witness, void* ptr) {
    uintptr_t witness_val = (uintptr_t) witness;
    uintptr_t ptr_val = (uintptr_t) ptr;
    if (ptr_val < NULLPTR_BOUND) {
        if (witness_val < NULLPTR_BOUND) {
            return;
        }
        __splay_fail("Outflowing out-of-bounds pointer", ptr);
    }
    Node* n = splayFind(&memTree, witness_val);
    if (n == NULL) {
        /* __splay_fail(ptr); */
        return;
    }
    if (ptr_val < n->base || ptr_val >= n->bound) {
        // ignore the potential access size here
        __splay_fail("Outflowing out-of-bounds pointer", ptr);
    }
}

void __splay_check_inbounds_named(void* witness, void* ptr, char* name) {
    uintptr_t witness_val = (uintptr_t) witness;
    uintptr_t ptr_val = (uintptr_t) ptr;
    if (ptr_val < NULLPTR_BOUND) {
        if (witness_val < NULLPTR_BOUND) {
            return;
        }
        __splay_fail("Outflowing out-of-bounds pointer", ptr);
    }
    Node* n = splayFind(&memTree, witness_val);
    if (n == NULL) {
        /* __splay_fail(ptr); */
        fprintf(stderr, "Inbounds check with non-existing witness for %p (%s)!\n", ptr, name);
        return;
    }
    if (ptr_val < n->base || ptr_val >= n->bound) {
        __splay_fail("Outflowing out-of-bounds pointer", ptr);
    }
}


void __splay_check_dereference(void* witness, void* ptr, size_t sz) {
    uintptr_t ptr_val = (uintptr_t) ptr;
    if (ptr_val < NULLPTR_BOUND) {
        __splay_fail("NULL dereference", ptr);
    }
    Node* n = splayFind(&memTree, (uintptr_t)witness);
    if (n == NULL) {
        /* __splay_fail(ptr); */
        return;
    }
    if (ptr_val < n->base || (ptr_val + sz) > n->bound) {
        __splay_fail("Out-of-bounds dereference", ptr);
    }
}

void __splay_check_dereference_named(void* witness, void* ptr, size_t sz, char* name) {
    uintptr_t ptr_val = (uintptr_t) ptr;
    if (ptr_val < NULLPTR_BOUND) {
        __splay_fail("NULL dereference", ptr);
    }
    Node* n = splayFind(&memTree, (uintptr_t)witness);
    if (n == NULL) {
        /* __splay_fail(ptr); */
        fprintf(stderr, "Access check with non-existing witness for %p (%s)!\n", ptr, name);
        return;
    }
    if (ptr_val < n->base || (ptr_val + sz) > n->bound) {
        __splay_fail("Out-of-bounds dereference", ptr);
    }
}

uintptr_t __splay_get_lower(void* witness) {
    Node* n = splayFind(&memTree, (uintptr_t)witness);
    if (n == NULL) {
        __splay_fail("Taking bounds of unknown allocation", witness);
        /* fprintf(stderr, "Check with non-existing witness!\n"); */
        return 0;
    }
    return n->base;
}

uintptr_t __splay_get_upper(void* witness) {
    Node* n = splayFind(&memTree, (uintptr_t)witness);
    if (n == NULL) {
        __splay_fail("Taking bounds of unknown allocation", witness);
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
    if (!splayRemove(&memTree, val)) {
        __splay_fail("Double free", (void*)val);
    }
}
