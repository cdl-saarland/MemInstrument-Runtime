#include <stdlib.h>

#include <unistd.h>
#include <time.h>
#include <errno.h>

#include "config.h"
#include "statistics.h"

#include "fail_function.h"

#include "sleep_instr.h"

uint64_t inbounds_check_delay = 10000;
uint64_t deref_check_delay = 10000;
uint64_t get_bound_delay = 10000;
uint64_t stack_alloc_delay = 10000;
uint64_t heap_alloc_delay = 10000;
uint64_t global_alloc_delay = 10000;
uint64_t free_delay = 10000;

void __mi_config(uint64_t i, uint64_t x) {
    switch (i) {
        case 0:
            deref_check_delay = x;
            break;
        case 1:
            inbounds_check_delay = x;
            break;
        case 2:
            get_bound_delay = x;
            break;
        case 3:
            stack_alloc_delay = x;
            break;
        case 4:
            heap_alloc_delay = x;
            break;
        case 5:
            global_alloc_delay = x;
            break;
        case 6:
            free_delay = x;
            break;
    }
}

static void sleepFun(uint64_t nsecs) {
    if (nsecs == 0) {
        return;
    }
    struct timespec tspec0;
    struct timespec tspec1;
    tspec0.tv_sec  = nsecs / 1000000000l;
    tspec0.tv_nsec = nsecs % 1000000000l;

    struct timespec *treq = &tspec0;
    struct timespec *trem = &tspec1;

    while (nanosleep(treq, trem) != 0) {
        if (errno != EINTR) {
            __mi_fail_with_msg("Failed to sleep!");
        }
        // nanosleep might be interrupted, in which case it returns -1 and sets
        // errno to EINTR, then the remaining time is stored in treq.
        struct timespec *tmp = treq;
        treq = trem;
        trem = tmp;
    }
    fprintf(stderr, "finished sleeping!\n");
}

void __setup_splay(void) { }

void __splay_check_inbounds_named(void* witness, void* ptr, char* name) {
    (void) witness;
    (void) ptr;
    (void) name;
    STAT_INC(NumInboundsChecks);
    sleepFun(inbounds_check_delay);
}

void __splay_check_inbounds(void* witness, void* ptr) {
    __splay_check_inbounds_named(witness, ptr, NULL);
}

void __splay_check_dereference_named(void* witness, void* ptr, size_t sz, char* name) {
    (void) witness;
    (void) ptr;
    (void) sz;
    (void) name;
    STAT_INC(NumDerefChecks);
    sleepFun(deref_check_delay);
}

void __splay_check_dereference(void* witness, void* ptr, size_t sz) {
    __splay_check_dereference_named(witness, ptr, sz, NULL);
}

uintptr_t __splay_get_lower(void* witness) {
    (void) witness;
    STAT_INC(NumGetLower);
    sleepFun(get_bound_delay / 2);
    return 0;
}

void *__splay_get_lower_as_ptr(void* witness) {
    return (void*)__splay_get_lower(witness);
}

void* __splay_get_base(void* witness) {
    return __splay_get_lower_as_ptr(witness);
}

uintptr_t __splay_get_upper(void* witness) {
    (void) witness;
    STAT_INC(NumGetUpper);
    sleepFun(get_bound_delay / 2);
    return (uintptr_t)-1;
}

void *__splay_get_upper_as_ptr(void* witness) {
    return (void*)__splay_get_upper(witness);
}

uintptr_t __splay_get_maxbyteoffset(void* witness) {
    (void) witness;
    STAT_INC(NumGetUpper);
    sleepFun(get_bound_delay / 2);
    return (uintptr_t)-1;
}

void __splay_alloc_or_merge(void* ptr, size_t sz) {
    (void) ptr;
    (void) sz;
    STAT_INC(NumMergeAllocs);
    sleepFun(global_alloc_delay);
}

void __splay_alloc(void* ptr, size_t sz) {
    (void) ptr;
    (void) sz;
    STAT_INC(NumStrictAllocs);
    sleepFun(heap_alloc_delay);
}

void __splay_alloc_or_replace(void* ptr, size_t sz) {
    (void) ptr;
    (void) sz;
    STAT_INC(NumReplaceAllocs);
    sleepFun(stack_alloc_delay);
}

void __splay_free(void* ptr) {
    (void) ptr;
    STAT_INC(NumFrees);
    sleepFun(free_delay);
}
