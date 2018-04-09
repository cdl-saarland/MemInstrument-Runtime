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
#ifdef STATISTICS
#ifdef STATS_FILE
    mi_stats_file = getenv(STATS_FILE);
#endif
#endif
}

_Noreturn void __splay_fail_verbose(const char* msg, void *faultingPtr, const char* verb) {
    STAT_INC(NumFatalFails);

#ifdef SILENT
    // avoid compiler warnings
    (void)msg;
    (void)verb;
    (void)faultingPtr;
#endif

#ifdef CONTINUE_ON_FATAL
#ifndef SILENT
    fprintf(stderr, "FATAL: Memory safety violation! "
            "%s with pointer %p\n" , msg, faultingPtr);
#endif
#else
#ifndef SILENT
    fprintf(stderr, "Memory safety violation!\n"
        "         / .'\n"
        "   .---. \\/\n"
        "  (._.' \\()\n"
        "   ^\"\"\"^\"\n"
        "%s with pointer %p\n", msg, faultingPtr);
    if (verb) {
        fprintf(stderr, "at %s\n", verb);
    }
    fprintf(stderr, "\nBacktrace:\n");
    PRINTBACKTRACE;
#endif
    exit(73);
#endif
}

_Noreturn void __splay_fail(const char* msg, void *faultingPtr) {
    __splay_fail_verbose(msg, faultingPtr, NULL);
}

_Noreturn void __splay_fail_simple(void) {
    STAT_INC(NumFatalFails);

#ifdef CONTINUE_ON_FATAL
#ifndef SILENT
    fprintf(stderr, "FATAL: Memory safety violation!");
#endif
#else
#ifndef SILENT
    fprintf(stderr, "Memory safety violation!\n"
        "         / .'\n"
        "   .---. \\/\n"
        "  (._.' \\()\n"
        "   ^\"\"\"^\"\n");
    fprintf(stderr, "\nBacktrace:\n");
    PRINTBACKTRACE;
#endif
    exit(73);
#endif
}

void __splay_check_inbounds(void* witness, void* ptr) {
    STAT_INC(NumInboundsChecks);
    uintptr_t witness_val = (uintptr_t) witness;
    uintptr_t ptr_val = (uintptr_t) ptr;
    if (ptr_val < NULLPTR_BOUND) {
        if (witness_val < NULLPTR_BOUND) {
            STAT_INC(NumFailedInboundsChecksNULL);
            return;
        }
        STAT_INC(NumFailedInboundsChecksNULL);
        __splay_fail("Outflowing out-of-bounds pointer", ptr);
    }
    Node* n = splayFind(&memTree, witness_val);
    if (n == NULL) {
        STAT_INC(NumInboundsChecksNoWitness);
        return;
    }
    if (ptr_val < n->base || ptr_val >= n->bound) {
        // ignore the potential access size here
        STAT_INC(NumFailedInboundsChecksOOB);
        __splay_fail("Outflowing out-of-bounds pointer", ptr);
    }
}

void __splay_check_inbounds_named(void* witness, void* ptr, char* name) {
    STAT_INC(NumInboundsChecks);
    uintptr_t witness_val = (uintptr_t) witness;
    uintptr_t ptr_val = (uintptr_t) ptr;
    if (ptr_val < NULLPTR_BOUND) {
        if (witness_val < NULLPTR_BOUND) {
            return;
        }
        STAT_INC(NumFailedInboundsChecksNULL);
        __splay_fail_verbose("Outflowing out-of-bounds pointer", ptr, name);
    }
    Node* n = splayFind(&memTree, witness_val);
    if (n == NULL) {
        STAT_INC(NumInboundsChecksNoWitness);
#ifndef SILENT
        fprintf(stderr, "Inbounds check with non-existing witness for %p (%s)!\n", ptr, name);
#endif
        return;
    }
    if (ptr_val < n->base || ptr_val >= n->bound) {
        STAT_INC(NumFailedInboundsChecksOOB);
        __splay_fail_verbose("Outflowing out-of-bounds pointer", ptr, name);
    }
}


void __splay_check_dereference(void* witness, void* ptr, size_t sz) {
    STAT_INC(NumDerefChecks);
    uintptr_t ptr_val = (uintptr_t) ptr;
    if (ptr_val < NULLPTR_BOUND) {
        STAT_INC(NumFailedDerefChecksNULL);
        __splay_fail("NULL dereference", ptr);
    }
    Node* n = splayFind(&memTree, (uintptr_t)witness);
    if (n == NULL) {
        STAT_INC(NumDerefChecksNoWitness);
        return;
    }
    if (ptr_val < n->base || (ptr_val + sz) > n->bound) {
        STAT_INC(NumFailedDerefChecksOOB);
        __splay_fail("Out-of-bounds dereference", ptr);
    }
}

void __splay_check_dereference_named(void* witness, void* ptr, size_t sz, char* name) {
    STAT_INC(NumDerefChecks);
    uintptr_t ptr_val = (uintptr_t) ptr;
    if (ptr_val < NULLPTR_BOUND) {
        STAT_INC(NumFailedDerefChecksNULL);
        __splay_fail_verbose("NULL dereference", ptr, name);
    }
    Node* n = splayFind(&memTree, (uintptr_t)witness);
    if (n == NULL) {
        STAT_INC(NumDerefChecksNoWitness);
#ifndef SILENT
        fprintf(stderr, "Access check with non-existing witness for %p (%s)!\n", ptr, name);
#endif
        return;
    }
    if (ptr_val < n->base || (ptr_val + sz) > n->bound) {
        STAT_INC(NumFailedDerefChecksOOB);
        __splay_fail_verbose("Out-of-bounds dereference", ptr, name);
    }
}

void *__splay_get_lower_as_ptr(void* witness) {
    return (void*)__splay_get_lower_as_ptr(witness);
}

uintptr_t __splay_get_lower(void* witness) {
    STAT_INC(NumGetLower);
    Node* n = splayFind(&memTree, (uintptr_t)witness);
    if (n == NULL) {
        STAT_INC(NumGetLowerNoWitness);
        __splay_fail("Taking bounds of unknown allocation", witness);
        /* fprintf(stderr, "Check with non-existing witness!\n"); */
        return 0;
    }
    return n->base;
}

void* __splay_get_base(void* witness) {
    return (void*)__splay_get_lower(witness);
}

uintptr_t __splay_get_maxbyteoffset(void* witness) {
    STAT_INC(NumGetLower);
    Node* n = splayFind(&memTree, (uintptr_t)witness);
    if (n == NULL) {
        STAT_INC(NumGetUpperNoWitness);
        __splay_fail("Taking bounds of unknown allocation", witness);
        /* fprintf(stderr, "Check with non-existing witness!\n"); */
        return 0;
    }
    return (n->bound - n->base) - 1;
}

void *__splay_get_upper_as_ptr(void* witness) {
    return (void*)__splay_get_upper_as_ptr(witness);
}

uintptr_t __splay_get_upper(void* witness) {
    STAT_INC(NumGetUpper);
    Node* n = splayFind(&memTree, (uintptr_t)witness);
    if (n == NULL) {
        STAT_INC(NumGetUpperNoWitness);
        __splay_fail("Taking bounds of unknown allocation", witness);
        /* fprintf(stderr, "Check with non-existing witness!\n"); */
        return -1;
    }
    return n->bound;
}

void __splay_alloc_or_merge(void* ptr, size_t sz) {
    STAT_INC(NumMergeAllocs);
    uintptr_t val = (uintptr_t)ptr;
    splayInsert(&memTree, val, val + sz, IB_EXTEND);
}

void __splay_alloc(void* ptr, size_t sz) {
    STAT_INC(NumStrictAllocs);
    uintptr_t val = (uintptr_t)ptr;
    splayInsert(&memTree, val, val + sz, IB_ERROR);
}

void __splay_alloc_or_replace(void* ptr, size_t sz) {
    STAT_INC(NumReplaceAllocs);
    uintptr_t val = (uintptr_t)ptr;
    splayInsert(&memTree, val, val + sz, IB_REPLACE);
}

void __splay_free(void* ptr) {
    STAT_INC(NumFrees);
    uintptr_t val = (uintptr_t)ptr;
    if (val == 0) {
        return;
    }
    if (!splayRemove(&memTree, val)) {
        STAT_INC(NumDoubleFrees);
        __splay_fail("Double free", (void*)val);
    }
}
