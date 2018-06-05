#include <stdlib.h>
#include <stdbool.h>

#include <assert.h>
#include <stdio.h>
#include <unistd.h>

#include "config.h"
#include "statistics.h"

#include "fail_function.h"

#include "tree.h"
#include "splay.h"

Tree __memTree;

_Noreturn
static void __mi_fail_wrapper(const char* msg, void* ptr, char* vmsg) {
#ifdef DUMP_ALLOCATION_MAP_ON_FAIL
    __dumpAllocationMap(stderr, &__memTree);
#endif
    if (vmsg) {
        __mi_fail_verbose_with_ptr(msg, ptr, vmsg);
    } else {
        __mi_fail_with_ptr(msg, ptr);
    }
}

static bool isNullPtr(uintptr_t ptr) {
    return ptr < NULLPTR_BOUND;
}

static void checkNullPtr(uintptr_t ptr, const char* errormsg) {
    if (isNullPtr(ptr)) {
        STAT_INC(NumFatalNullAllocations);
        __mi_fail_wrapper(errormsg, (void*)ptr, NULL);
    }
}

static bool accessMemoryMap(uintptr_t witness_val) {
#ifdef USE_MEMORY_MAP
    STAT_INC(NumMemMap);
    char buf[32];
    pid_t pid = getpid();
    snprintf(buf, 32, "/proc/%d/maps", pid);
    FILE *F = fopen(buf, "r");

    char* line;
    size_t sz = 0;

    uintptr_t lower, upper;
    bool success = false;

    for (; getline(&line, &sz, F) != -1; free(line), sz = 0) {
        /* fprintf(stderr, "%s", line); */
        uint64_t inode;

    // format: [lower]-[upper] [permissions] [offset] [device] [inode] [filename]
        if (sscanf(line, "%lx-%lx %*s %*s %*s %lu", &lower, &upper, &inode) != 3) {
            continue;
        }

#ifdef USE_MEMORY_MAP_FILEBACKED_ONLY
        if (inode == 0) {
            continue;
        }
#endif

        if (lower <= witness_val && witness_val < upper) {
            /* fprintf(stderr, "Found range [%lx - %lx] for %lx\n", lower, upper, witness_val); */
            STAT_INC(NumMemMapSucc);
            splayInsert(&__memTree, lower, upper, IB_ERROR);
            success = true;
            break;
        }
    }
    free(line);
    fclose(F);
    return success;
#else
    (void)witness_val;
    return false;
#endif
}

static Node* getNode(uintptr_t witness_val, const char *str) {
    Node* n = splayFind(&__memTree, witness_val);
    if (n == NULL) {
        if (accessMemoryMap(witness_val)) {
            n = splayFind(&__memTree, witness_val);
            assert(n);
            return n;
        }
        STAT_INC(NumNoWitness);
#ifdef CRASH_ON_MISSING_WITNESS
        __mi_fail_wrapper(str, (void*)witness_val, NULL);
#else
        (void)str;
#endif
        return NULL;
    }
    return n;
}

void __setup_splay(void) {
    splayInit(&__memTree);
}

void __splay_check_inbounds_named(void* witness, void* ptr, char* name) {
    STAT_INC(NumInboundsChecks);
    uintptr_t witness_val = (uintptr_t) witness;
    uintptr_t ptr_val = (uintptr_t) ptr;
    if (isNullPtr(ptr_val)) {
        if (isNullPtr(witness_val)) {
            STAT_INC(NumSuccessfulInboundsChecksNULL);
            return;
        }
        STAT_INC(NumFailedInboundsChecksNULL);
        __mi_fail_wrapper("Outflowing out-of-bounds pointer", (void*)ptr, name);
    }
    Node* n = getNode(witness_val, "Invariant check in unknown allocation");
    if (n == NULL) {
        return;
    }
    if (ptr_val < n->base || ptr_val >= n->bound) {
        // ignore the potential access size here
        STAT_INC(NumFailedInboundsChecksOOB);
        __mi_fail_wrapper("Outflowing out-of-bounds pointer", (void*)ptr, name);
    }
}

void __splay_check_inbounds(void* witness, void* ptr) {
    __splay_check_inbounds_named(witness, ptr, NULL);
}

void __splay_check_dereference_named(void* witness, void* ptr, size_t sz, char* name) {
    STAT_INC(NumDerefChecks);
    uintptr_t ptr_val = (uintptr_t) ptr;
    if (isNullPtr(ptr_val)) {
        STAT_INC(NumFailedDerefChecksNULL);
        __mi_fail_wrapper("NULL dereference", (void*)ptr, name);
    }
    Node* n = getNode((uintptr_t)witness, "Dereference check in unknown allocation");
    if (n == NULL) {
        return;
    }
    if (ptr_val < n->base || (ptr_val + sz) > n->bound) {
        STAT_INC(NumFailedDerefChecksOOB);
        __mi_fail_wrapper("Out-of-bounds dereference", (void*)ptr, name);
    }
}

void __splay_check_dereference(void* witness, void* ptr, size_t sz) {
    __splay_check_dereference_named(witness, ptr, sz, NULL);
}

uintptr_t __splay_get_lower(void* witness) {
    STAT_INC(NumGetLower);
    Node* n = getNode((uintptr_t)witness, "Taking bounds of unknown allocation");
    if (n == NULL) {
        return 0;
    }
    return n->base;
}

void *__splay_get_lower_as_ptr(void* witness) {
    return (void*)__splay_get_lower(witness);
}

void* __splay_get_base(void* witness) {
    return __splay_get_lower_as_ptr(witness);
}

uintptr_t __splay_get_upper(void* witness) {
    STAT_INC(NumGetUpper);
    Node* n = getNode((uintptr_t)witness, "Taking bounds of unknown allocation");
    if (n == NULL) {
        return (uintptr_t)-1;
    }
    return n->bound;
}

void *__splay_get_upper_as_ptr(void* witness) {
    return (void*)__splay_get_upper(witness);
}

uintptr_t __splay_get_maxbyteoffset(void* witness) {
    STAT_INC(NumGetUpper);
    Node* n = getNode((uintptr_t)witness, "Taking bounds of unknown allocation");
    if (n == NULL) {
        return (uintptr_t)-1;
    }
    return (n->bound - n->base) - 1;
}

void __splay_alloc_or_merge(void* ptr, size_t sz) {
    STAT_INC(NumMergeAllocs);
    uintptr_t val = (uintptr_t)ptr;

    if (val == (uintptr_t)0) {
        // This means that a global variable is located at a nullptr,
        // apparently this can happen if no definition for a weak symbol exists.
        // We just note this in our statistics and do nothing else.
        STAT_INC(NumGlobalNullAllocations);
        return;
    }

    checkNullPtr(val, "Allocation at nullptr (merge)");
    splayInsert(&__memTree, val, val + sz, IB_EXTEND);
}

void __splay_alloc(void* ptr, size_t sz) {
    STAT_INC(NumStrictAllocs);
    uintptr_t val = (uintptr_t)ptr;
    checkNullPtr(val, "Allocation at nullptr (heap)");
    splayInsert(&__memTree, val, val + sz, IB_ERROR);
}

void __splay_alloc_or_replace(void* ptr, size_t sz) {
    STAT_INC(NumReplaceAllocs);
    uintptr_t val = (uintptr_t)ptr;
    checkNullPtr(val, "Allocation at nullptr (replace)");
    splayInsert(&__memTree, val, val + sz, IB_REPLACE);
}

void __splay_free(void* ptr) {
    STAT_INC(NumFrees);
    uintptr_t val = (uintptr_t)ptr;
    if (val == 0) {
        return;
    }

    bool success = splayRemove(&__memTree, val);

    if (!success) {
        STAT_INC(NumDoubleFrees);
#ifdef CHECK_DOUBLE_FREE
        __mi_fail_wrapper("Double free", (void*)val, NULL);
#endif
    }
}
