#include <stdlib.h>
#include <stdbool.h>

#include "config.h"
#include "statistics.h"

#include "fail_function.h"

#include "tree.h"
#include "splay.h"

Tree memTree;

static bool isNullPtr(uintptr_t ptr) {
    return ptr < NULLPTR_BOUND;
}

static void checkNullPtr(uintptr_t ptr, const char* errormsg) {
    if (isNullPtr(ptr)) {
        STAT_INC(NumFatalNullAllocations);
#ifdef DUMP_ALLOCATION_MAP_ON_FAIL
        __dumpAllocationMap(stderr, &memTree);
#endif
        __mi_fail_with_ptr(errormsg, (void*)ptr);
    }
}

void __setup_splay(void) {
    splayInit(&memTree);
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
#ifdef DUMP_ALLOCATION_MAP_ON_FAIL
        __dumpAllocationMap(stderr, &memTree);
#endif
        if (name) {
            __mi_fail_verbose_with_ptr("Outflowing out-of-bounds pointer", ptr, name);
        } else {
            __mi_fail_with_ptr("Outflowing out-of-bounds pointer", ptr);
        }
    }
    Node* n = splayFind(&memTree, witness_val);
    if (n == NULL) {
        STAT_INC(NumInboundsChecksNoWitness);
#ifdef CRASH_ON_MISSING_WITNESS_INVAR
        __mi_fail_with_ptr("Invariant check in unknown allocation", ptr);
#endif
        return;
    }
    if (ptr_val < n->base || ptr_val >= n->bound) {
        // ignore the potential access size here
        STAT_INC(NumFailedInboundsChecksOOB);
#ifdef DUMP_ALLOCATION_MAP_ON_FAIL
        __dumpAllocationMap(stderr, &memTree);
#endif
        if (name) {
            __mi_fail_verbose_with_ptr("Outflowing out-of-bounds pointer", ptr, name);
        } else {
            __mi_fail_with_ptr("Outflowing out-of-bounds pointer", ptr);
        }
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
#ifdef DUMP_ALLOCATION_MAP_ON_FAIL
        __dumpAllocationMap(stderr, &memTree);
#endif
        if (name) {
            __mi_fail_verbose_with_ptr("NULL dereference", ptr, name);
        } else {
            __mi_fail_with_ptr("NULL dereference", ptr);
        }
    }
    Node* n = splayFind(&memTree, (uintptr_t)witness);
    if (n == NULL) {
        STAT_INC(NumDerefChecksNoWitness);
#ifdef CRASH_ON_MISSING_WITNESS_DEREF
#ifdef DUMP_ALLOCATION_MAP_ON_FAIL
        __dumpAllocationMap(stderr, &memTree);
#endif
        __mi_fail_with_ptr("Dereference check in unknown allocation", ptr);
#endif
        return;
    }
    if (ptr_val < n->base || (ptr_val + sz) > n->bound) {
        STAT_INC(NumFailedDerefChecksOOB);
#ifdef DUMP_ALLOCATION_MAP_ON_FAIL
        __dumpAllocationMap(stderr, &memTree);
#endif
        if (name) {
            __mi_fail_verbose_with_ptr("Out-of-bounds dereference", ptr, name);
        } else {
            __mi_fail_with_ptr("Out-of-bounds dereference", ptr);
        }
    }
}

void __splay_check_dereference(void* witness, void* ptr, size_t sz) {
    __splay_check_dereference_named(witness, ptr, sz, NULL);
}

uintptr_t __splay_get_lower(void* witness) {
    STAT_INC(NumGetLower);
    Node* n = splayFind(&memTree, (uintptr_t)witness);
    if (n == NULL) {
        STAT_INC(NumGetLowerNoWitness);
#ifdef CRASH_ON_MISSING_WITNESS_BOUND
#ifdef DUMP_ALLOCATION_MAP_ON_FAIL
        __dumpAllocationMap(stderr, &memTree);
#endif
        __mi_fail_with_ptr("Taking bounds of unknown allocation", witness);
#endif
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
    Node* n = splayFind(&memTree, (uintptr_t)witness);
    if (n == NULL) {
        STAT_INC(NumGetUpperNoWitness);
#ifdef CRASH_ON_MISSING_WITNESS_BOUND
#ifdef DUMP_ALLOCATION_MAP_ON_FAIL
        __dumpAllocationMap(stderr, &memTree);
#endif
        __mi_fail_with_ptr("Taking bounds of unknown allocation", witness);
#endif
        return -1;
    }
    return n->bound;
}

void *__splay_get_upper_as_ptr(void* witness) {
    return (void*)__splay_get_upper(witness);
}

uintptr_t __splay_get_maxbyteoffset(void* witness) {
    STAT_INC(NumGetUpper);
    Node* n = splayFind(&memTree, (uintptr_t)witness);
    if (n == NULL) {
        STAT_INC(NumGetUpperNoWitness);
#ifdef CRASH_ON_MISSING_WITNESS_BOUND
#ifdef DUMP_ALLOCATION_MAP_ON_FAIL
        __dumpAllocationMap(stderr, &memTree);
#endif
        __mi_fail_with_ptr("Taking bounds of unknown allocation", witness);
#endif
        return 0;
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
    splayInsert(&memTree, val, val + sz, IB_EXTEND);
}

void __splay_alloc(void* ptr, size_t sz) {
    STAT_INC(NumStrictAllocs);
    uintptr_t val = (uintptr_t)ptr;
    checkNullPtr(val, "Allocation at nullptr (heap)");
    splayInsert(&memTree, val, val + sz, IB_ERROR);
}

void __splay_alloc_or_replace(void* ptr, size_t sz) {
    STAT_INC(NumReplaceAllocs);
    uintptr_t val = (uintptr_t)ptr;
    checkNullPtr(val, "Allocation at nullptr (replace)");
    splayInsert(&memTree, val, val + sz, IB_REPLACE);
}

void __splay_free(void* ptr) {
    STAT_INC(NumFrees);
    uintptr_t val = (uintptr_t)ptr;
    if (val == 0) {
        return;
    }

    bool success = splayRemove(&memTree, val);

    if (!success) {
        STAT_INC(NumDoubleFrees);
#ifdef CHECK_DOUBLE_FREE
#ifdef DUMP_ALLOCATION_MAP_ON_FAIL
        __dumpAllocationMap(stderr, &memTree);
#endif
        __mi_fail_with_ptr("Double free", (void*)val);
#endif
    }
}
